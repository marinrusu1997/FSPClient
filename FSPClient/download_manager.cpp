#include "download_manager.h"

#include <AtlBase.h>
#include <atlconv.h>

#include <filesystem>
#include <sstream>
#include <algorithm>

#include "protocol.h"

#include "stream_file_deliver.h"
#include "compressed_file_deliver.h"

#include "stream_file_receiver.h"
#include "compressed_file_receiver.h"

#define UPLOAD_DEADLINE_TIME_POSIX_SECONDS	boost::posix_time::seconds(40)
#define ENTER_CRITICAL_SECTION(Mutex)		{	_STD lock_guard<_STD mutex> guard((Mutex));			
#define EXIT_CRITICAL_SECTION				}

/// DOWNLOAD TRANSACTION	------------------------------------------------------------------------------------------------------------------------------------------------
download_manager::download_transaction::download_transaction()
	: LocalFilePath(),
	State(pending),
	FileReceiverPtr(nullptr)
{}

download_manager::download_transaction::download_transaction(_STD string&& LocalFilePath)
	: LocalFilePath(_STD move(LocalFilePath)),
	State(pending),
	FileReceiverPtr(nullptr)
{}

_NODISCARD const bool download_manager::download_transaction::IsDownloadingAlready(const _STD string_view LocalFilePath) const noexcept
{
	return this->State == downloading && this->LocalFilePath == LocalFilePath;
}

/// UPLOAD TRANSACTION	------------------------------------------------------------------------------------------------------------------------------------------------

download_manager::upload_transaction::upload_transaction()
	: State(pending),
	FileDeliverPtr(nullptr),
	FileName()
{}

download_manager::upload_transaction::upload_transaction(_STD string&& FileName)
	: State(pending),
	FileDeliverPtr(nullptr),
	FileName(_STD move(FileName))
{}


/// DOWNLOAD MANAGER	-------------------------------------------------------------------------------------------------------------------------------------------------
Q_DECLARE_METATYPE(uintmax_t);
Q_DECLARE_METATYPE(AbstractFileReceiver::DownloadStatus);

download_manager::download_manager(aio::io_context& io_ctx)
	: io_ctx_(io_ctx),
	ssl_ctx_(boost::asio::ssl::context::sslv23),
	uploads_prohibited_(false)
{
	qRegisterMetaType<uintmax_t>();
	qRegisterMetaType<AbstractFileReceiver::DownloadStatus>();
}

_NODISCARD const bool download_manager::IsFileDownloadingAlready(const _STD string_view LocalPath) const noexcept
{
	return _STD find_if(download_transactions_.begin(), download_transactions_.end(), [&](const auto& pair) {return pair.second.IsDownloadingAlready(LocalPath); })
		!= download_transactions_.end();
}

_NODISCARD const _STD forward_list<_STD string_view> download_manager::GetSupportedCompressions() const
{
	//_STD forward_list<_STD string_view> compressions;
	//_STD transform(
	//	_STD begin(compressors),
	//	_STD end(compressors),
	//	_STD front_inserter(compressions),
	//	[](const auto& pair) {return fsp::protocol::compressions::Compression(pair.first); }
	//);
	//return compressions;

	return { fsp::protocol::compressions::ZLIB, fsp::protocol::compressions::GZIP };
}

void													download_manager::SetUploadDenyStatus(bool DenyFlag) noexcept
{
	this->uploads_prohibited_.store(DenyFlag, _STD memory_order_release);
}

compression												CompressionPolicyResolver(uintmax_t FileSize) noexcept
{
	if (FileSize > 104857600) // 100 MB
		return compression::zlib;
	else
		return compression::nocompression;
}

download_manager::UploadTransactionInfo					download_manager::BeginUploadTransaction(uintmax_t ID, _STD string&& FileName)
{
	std::error_code ec;
	auto FileSize = _STD filesystem::file_size(FileName, ec);
	if (FileSize == 0 && !ec)
		return { false };

	auto Compression_ = CompressionPolicyResolver(FileSize);

	ENTER_CRITICAL_SECTION(uploads_mutex_)
		this->upload_transactions_.emplace(ID, upload_transaction{ _STD move(FileName) });
		return { true, Compression_, FileSize };
	EXIT_CRITICAL_SECTION
}

void													download_manager::StartUploadTransaction(uintmax_t ID, boost::asio::ip::tcp::resolver::iterator&& EndPoint, AbstractCompressor *Compressor)
{
	upload_transaction *TransactionToStart = nullptr;

	ENTER_CRITICAL_SECTION(uploads_mutex_)
		if (auto&& iter = this->upload_transactions_.find(ID); iter != this->upload_transactions_.end())
			TransactionToStart = &iter->second;
	EXIT_CRITICAL_SECTION

	if (TransactionToStart == nullptr)
		return;

	HANDLE FileHandle = CreateFile(CA2W(TransactionToStart->GetFileName().c_str()),
		GENERIC_READ,
		NULL,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
		NULL);
	if (FileHandle == INVALID_HANDLE_VALUE)
	{
		this->EndUploadTransaction(ID);
		return;
	}

	auto CompletionHandler = [this, ID = ID]() { this->EndUploadTransaction(ID); };

	AbstractFileDeliverPtr_t FileDeliver(nullptr);

	if (!Compressor)
		FileDeliver = _STD make_unique<StreamFileDeliver>(FileHandle, this->io_ctx_, this->ssl_ctx_,
			_STD move(CompletionHandler), ID);
	else
		FileDeliver = _STD make_unique<CompressedFileDeliver>(FileHandle, this->io_ctx_, this->ssl_ctx_,
			_STD move(CompletionHandler), ID, Compressor);

	TransactionToStart->SetFileDeliverPtr(_STD move(FileDeliver));
	TransactionToStart->GetFileDeliverPtr()->start(_STD move(EndPoint));
}

void													download_manager::EndUploadTransaction(uintmax_t ID)
{
	ENTER_CRITICAL_SECTION(uploads_mutex_)
		this->upload_transactions_.erase(ID);
	EXIT_CRITICAL_SECTION
}

void													download_manager::BeginDownloadTransaction(uintmax_t ID, _STD string&& LocalFilePath)
{
	this->download_transactions_.emplace(ID, download_transaction{ _STD move(LocalFilePath) });
}

void													download_manager::StartDownloadTransaction(const uintmax_t ID, const compression TransferCompression, const uintmax_t FileSize,
	boost::asio::ip::tcp::resolver::iterator EndPoint, const uintmax_t ConnectID)
{
	if (auto&& iter = this->download_transactions_.find(ID); iter != this->download_transactions_.end())
	{
		const auto& LocalFilePath = iter->second.GetLocalFilePath();
		_STD filesystem::create_directories(_STD filesystem::path(LocalFilePath).parent_path());

		HANDLE LocalFileHandle = CreateFile(
			CA2W(LocalFilePath.c_str()),
			GENERIC_WRITE,
			NULL,
			NULL,
			CREATE_ALWAYS,
			FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
			NULL
		);

		if (LocalFileHandle == INVALID_HANDLE_VALUE)
		{
			emit download_test_signal(LocalFilePath, AbstractFileReceiver::DownloadStatus::Cancelled, 0);
			assert(this->StopDownloadTransaction(ID));
			return;
		}

		auto DownloadHandler = [this, LocalFilePath = LocalFilePath, ID = ID]
		(const auto DownloadStatus, const auto RelativeAmountDownloaded) {
			switch (DownloadStatus)
			{
			case AbstractFileReceiver::DownloadStatus::Downloading:
				emit this->download_test_signal(LocalFilePath, DownloadStatus, RelativeAmountDownloaded);
				break;
			case AbstractFileReceiver::DownloadStatus::NetworkError:
				[[fallthrough]];
			case AbstractFileReceiver::DownloadStatus::Cancelled:
				[[fallthrough]];
			case AbstractFileReceiver::DownloadStatus::Done:
				emit this->download_test_signal(LocalFilePath, DownloadStatus, RelativeAmountDownloaded);
				assert(this->StopDownloadTransaction(ID));
				break;
			default:
				break;
			}

		};

		AbstractFileReceiverPtr_t FileReceiver(nullptr);

		if (TransferCompression == fsp::protocol::compressions::compression::nocompression)
		{
			FileReceiver = _STD make_unique<StreamFileReceiver>
				(LocalFileHandle, this->io_ctx_, FileSize, this->ssl_ctx_, ConnectID, _STD move(DownloadHandler));
		}
		if (TransferCompression == fsp::protocol::compressions::compression::zlib)
		{
			FileReceiver = _STD make_unique<CompressedFileReceiver>
				(LocalFileHandle, this->io_ctx_, FileSize, this->ssl_ctx_,
					ConnectID, _STD move(DownloadHandler), GetCompressor(TransferCompression));
		}
		assert(FileReceiver != nullptr);

		iter->second.SetFileReceiver(_STD move(FileReceiver));
		iter->second.GetFileReceiver()->start(_STD move(EndPoint));
	}
}

_NODISCARD const size_t									download_manager::StopDownloadTransaction(uintmax_t ID)
{
	return this->download_transactions_.erase(ID);
}

_NODISCARD const bool									download_manager::CancelDownloadTransaction(uintmax_t ID)
{
	if (auto&& iter = this->download_transactions_.find(ID); iter != this->download_transactions_.end())
	{
		iter->second.GetFileReceiver()->cancel();
	}
	return false;
}