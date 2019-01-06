#pragma once

#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/ip/tcp.hpp>

#include "async_windows_file_io.h"

namespace boost::asio::ssl {
	class context;
	class verify_context;
}

struct AbstractFileDeliver
{
	typedef _STD function<void()> handler;

	AbstractFileDeliver(HANDLE OpenedFileOnDisk, boost::asio::io_context& io_ctx, boost::asio::ssl::context& ssl_ctx,
		handler&& CompletionHandler, uintmax_t ConnectID);
	AbstractFileDeliver(AbstractFileDeliver const&) = delete;
	AbstractFileDeliver& operator=(AbstractFileDeliver const&) = delete;
	virtual ~AbstractFileDeliver() {}

	void start(boost::asio::ip::tcp::resolver::iterator endpoint_iterator);
protected:
	bool verify_certificate(bool preverified, boost::asio::ssl::verify_context& ctx);

	void handle_connect(const boost::system::error_code& error);

	void handle_handshake(const boost::system::error_code& error);

	virtual void start_delivering() = 0;

	using ssl_sock = boost::asio::ssl::stream<boost::asio::ip::tcp::socket>;
	enum { KB500 = 512000 };
	AsyncWindowsFileIO	FileOnDisk;
	ssl_sock			Socket;
	char				Buffer[KB500];
	handler				Handler;
	uintmax_t			ConnectID;
	bool				DoneFlag;
};
typedef _STD unique_ptr<AbstractFileDeliver> AbstractFileDeliverPtr_t;