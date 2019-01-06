#pragma once
#include <atomic>

#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/ip/tcp.hpp>

#include "async_windows_file_io.h"
#include "protocol.h"

struct AbstractFileReceiver 
{
	enum class DownloadStatus : uint8_t
	{
		Downloading,
		NetworkError,
		Cancelled,
		Done
	};

	typedef _STD function<void(const DownloadStatus, const uint8_t)> DownloadHandler_t;

	AbstractFileReceiver(HANDLE OpenedFileOnDisk, boost::asio::io_context& io_ctx, uintmax_t FileSize,
		boost::asio::ssl::context& ssl_ctx, uintmax_t ConnectID, DownloadHandler_t&& DownloadHandler);
	AbstractFileReceiver(AbstractFileReceiver const&) = delete;
	AbstractFileReceiver& operator=(AbstractFileReceiver const&) = delete;
	virtual ~AbstractFileReceiver() {}

	void start(boost::asio::ip::tcp::resolver::iterator endpoint_iterator);
	void cancel() noexcept { IsCancelled.store(true, _STD memory_order::memory_order_release); }
protected:
	using ssl_sock = boost::asio::ssl::stream<boost::asio::ip::tcp::socket>;

	bool verify_certificate(bool preverified, boost::asio::ssl::verify_context& ctx);

	void handle_connect(const boost::system::error_code& error);

	void handle_handshake(const boost::system::error_code& error);

	virtual void start_receiving() = 0;

	_NODISCARD const uint8_t RelativeAmountDownloaded() const noexcept
	{
		return (uint8_t)(((float)BytesWrottedToDisck / FileSize) * 100);
	}
	void	OnError() const 
	{ 
		DownloadHandler(DownloadStatus::NetworkError,RelativeAmountDownloaded()); 
	}
	void	OnSuccess() const 
	{
		DownloadHandler(DownloadStatus::Done, RelativeAmountDownloaded());
	}
	void	OnChunkWrottedOnDisk() const
	{
		DownloadHandler(DownloadStatus::Downloading, RelativeAmountDownloaded());
	}
	void	OnCancelled() const
	{
		DownloadHandler(DownloadStatus::Cancelled, RelativeAmountDownloaded());
	}

	uintmax_t			ConnectID;
	AsyncWindowsFileIO	FileOnDisk;
	uintmax_t			FileSize;
	uintmax_t			BytesWrottedToDisck;
	ssl_sock			Socket;
	_STD atomic_bool	IsCancelled;
	DownloadHandler_t	DownloadHandler;
	_STD byte			Buffer[fsp::protocol::MAX_COMPRESSED_SEGMENT_LENGTH];
};

typedef _STD unique_ptr<AbstractFileReceiver>	AbstractFileReceiverPtr_t;
