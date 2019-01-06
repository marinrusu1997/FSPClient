#include "abstract_file_deliver.h"
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/verify_context.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/bind.hpp>

#include "protocol.h"

#define _ASIO	::boost::asio::
#define _FSP	::fsp::protocol::
#define CHECK_IO_OPERATION_STATUS(error_code)  if ((error_code)) {	Handler(); return;	}

AbstractFileDeliver::AbstractFileDeliver(HANDLE OpenedFileOnDisk, _ASIO io_context& io_ctx, _ASIO ssl::context& ssl_ctx, handler&& CompletionHandler, uintmax_t ConnectID)
	:	FileOnDisk(io_ctx, OpenedFileOnDisk),
		Socket(io_ctx, ssl_ctx),
		Handler(_STD move(CompletionHandler)),
		ConnectID(ConnectID),
		DoneFlag(false)
{
	Socket.set_verify_mode(_ASIO ssl::verify_peer);
	Socket.set_verify_callback(
		boost::bind(&AbstractFileDeliver::verify_certificate, this, _1, _2));
}

void AbstractFileDeliver::start(boost::asio::ip::tcp::resolver::iterator endpoint_iterator)
{
	_ASIO async_connect(Socket.lowest_layer(), endpoint_iterator,
		boost::bind(&AbstractFileDeliver::handle_connect, this,
			_ASIO placeholders::error));
}

bool AbstractFileDeliver::verify_certificate(bool preverified, _ASIO ssl::verify_context& ctx)
{
	char subject_name[256];
	X509* cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
	X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);

	return true;
}

void AbstractFileDeliver::handle_connect(const boost::system::error_code& error)
{
	if (!error)
	{
		Socket.async_handshake(_ASIO ssl::stream_base::client,
			boost::bind(&AbstractFileDeliver::handle_handshake, this,
				_ASIO placeholders::error));
	}
	else
	{
		Handler();
	}
}

void AbstractFileDeliver::handle_handshake(const boost::system::error_code& error)
{
	if (!error)
	{
		_STD stringstream sstream;
		sstream
			<< _FSP SENDER_CODE_STR
			<< _FSP DOWNLOAD_TRANSACTION_DELIM_STR
			<< _STD to_string(ConnectID)
			<< _FSP LINE_SEPARATOR;
		auto ConnectMessage = sstream.str();

		boost::system::error_code ec;

		_ASIO write(Socket, _ASIO buffer(ConnectMessage.data(), ConnectMessage.length()), ec);
		CHECK_IO_OPERATION_STATUS(ec)

		_ASIO read(Socket, boost::asio::buffer(Buffer, 1), ec);
		CHECK_IO_OPERATION_STATUS(ec)

		start_delivering();
	}
	else
	{
		Handler();
	}
}