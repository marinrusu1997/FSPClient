#pragma once
#include <map>

#include <forward_list>
#include <atomic>
#include <mutex>

#include <boost/asio/io_context.hpp>
#include <boost/asio/ssl/context.hpp>

#include <QtCore/qobject.h>

#include "abstract_file_deliver.h"
#include "abstract_file_receiver.h"

#include "fsp_ssl_versions.h"
#include "fsp_compressors.h"

#pragma comment (lib, "crypt32")

namespace aio = boost::asio;
using fsp::protocol::ssl_versions::version;
using fsp::protocol::compressions::compression;
using namespace fsp::protocol::compressors;

// WARNING!!!		THERE MAYBE SHOULD BE USED A MUTEX, DUE TO 2 IO THREADS SO 2 DOWNLOAD COMPLETION HANDLERS MAY INVOKE AT THE SAME TIME COMPLETION HANDLER

class download_manager : public QObject
{
	Q_OBJECT
signals:
	void download(const uintmax_t, const AbstractFileReceiver::DownloadStatus, const uint8_t);
	/* this is intended only for testing purposes */
	void download_test_signal(const std::string, const AbstractFileReceiver::DownloadStatus, const unsigned char);
public:

	struct UploadTransactionInfo
	{
		bool		Succes;
		compression Compression;
		uintmax_t	FileSize;
	};

	download_manager(aio::io_context& io_ctx);

	_NODISCARD const bool									IsFileDownloadingAlready(const _STD string_view LocalPath) const noexcept;
	_NODISCARD const version								GetSslVersion() const noexcept { return version::sslv23; }
	_NODISCARD const _STD forward_list<_STD string_view>	GetSupportedCompressions() const;
	auto&													GetIOContext() noexcept { return io_ctx_; }
	_NODISCARD const bool									AreUploadsDenied() const noexcept { return uploads_prohibited_.load(_STD memory_order_acquire); }
	void													SetUploadDenyStatus(bool DenyFlag) noexcept;

	UploadTransactionInfo									BeginUploadTransaction(uintmax_t ID, _STD string&& FileName);
	void													StartUploadTransaction(uintmax_t ID, boost::asio::ip::tcp::resolver::iterator&& EndPoint,
		AbstractCompressor *Compressor);
	void													BeginDownloadTransaction(uintmax_t ID, _STD string&& LocalFilePath);
	void													StartDownloadTransaction(const uintmax_t ID,const compression TransferCompression,const uintmax_t FileSize,
																	boost::asio::ip::tcp::resolver::iterator EndPoint, const uintmax_t ConnectID);
	_NODISCARD const size_t									StopDownloadTransaction(uintmax_t ID);
private:
	void													EndUploadTransaction(uintmax_t ID);
	_NODISCARD const bool									CancelDownloadTransaction(uintmax_t ID);
	fsp::protocol::compressors::AbstractCompressor*			GetCompressor(compression c) const noexcept { return fsp::protocol::compressors::GetCompressorPointer(c); }

	struct upload_transaction
	{
		enum state
		{
			pending,
			uploading,
			error
		};

		upload_transaction();
		upload_transaction(_STD string&& FileName);

		const auto&				GetFileDeliverPtr() const noexcept { assert(FileDeliverPtr != nullptr); return FileDeliverPtr; }
		void					SetFileDeliverPtr(AbstractFileDeliverPtr_t FileDeliver) { FileDeliverPtr = _STD move(FileDeliver); }
		const auto&				GetFileName() const noexcept { return FileName; }
	private:
		state						State;
		_STD string					FileName;
		AbstractFileDeliverPtr_t	FileDeliverPtr;
	};

	struct download_transaction
	{
		enum state
		{
			pending,
			downloading,
			canceled,
			done,
			error
		};

		download_transaction();
		download_transaction(_STD string&& LocalFilePath);

		_NODISCARD const bool	IsDownloadingAlready(const _STD string_view LocalFilePath) const noexcept;
		const auto&				GetLocalFilePath() const noexcept { return LocalFilePath; }
		const auto&				GetFileReceiver() const noexcept { assert(FileReceiverPtr != nullptr); return FileReceiverPtr; }
		void					SetFileReceiver(AbstractFileReceiverPtr_t FileReceiver) { this->FileReceiverPtr = _STD move(FileReceiver); }
	private:
		_STD string							LocalFilePath;
		state								State;
		AbstractFileReceiverPtr_t			FileReceiverPtr;
	};

	_STD map<uintmax_t,	download_transaction>	download_transactions_;
	_STD mutex									uploads_mutex_;
	_STD map<uintmax_t, upload_transaction	>	upload_transactions_;

	aio::ssl::context				ssl_ctx_;
	aio::io_context&				io_ctx_;
	_STD atomic_bool				uploads_prohibited_;
};