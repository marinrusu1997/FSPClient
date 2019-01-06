#include "abstract_file_receiver.h"
#include <boost/asio/connect.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/bind.hpp>

#define _ASIO	::boost::asio::
#define _FSP	::fsp::protocol::

#define CHECK_IO_OPERATION_STATUS(error_code)	if ((error_code)) {	OnError();	return;	}

AbstractFileReceiver::AbstractFileReceiver(HANDLE OpenedFileOnDisk, boost::asio::io_context& io_ctx,
	uintmax_t FileSize, boost::asio::ssl::context& ssl_ctx, uintmax_t ConnectID, DownloadHandler_t&& DownloadHandler)
	:	ConnectID{	ConnectID	},
		FileOnDisk{ io_ctx,OpenedFileOnDisk },
		FileSize{ FileSize },
		BytesWrottedToDisck{ 0 },
		Socket{	io_ctx,ssl_ctx	},
		IsCancelled{	false	},
		DownloadHandler(_STD move(DownloadHandler))
{
	Socket.set_verify_mode(_ASIO ssl::verify_peer);
	Socket.set_verify_callback(
		boost::bind(&AbstractFileReceiver::verify_certificate, this, _1, _2));
}

void AbstractFileReceiver::start(boost::asio::ip::tcp::resolver::iterator endpoint_iterator)
{
	_ASIO async_connect(Socket.lowest_layer(), endpoint_iterator,
		boost::bind(&AbstractFileReceiver::handle_connect, this,
			_ASIO placeholders::error));
}

bool AbstractFileReceiver::verify_certificate(bool preverified, boost::asio::ssl::verify_context& ctx)
{
	char subject_name[256];
	X509* cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
	X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);

	return true;
}

void AbstractFileReceiver::handle_connect(const boost::system::error_code& error)
{
	if (!error)
	{
		Socket.async_handshake(_ASIO ssl::stream_base::client,
			boost::bind(&AbstractFileReceiver::handle_handshake, this,
				_ASIO placeholders::error));
	}
	else
	{
		OnError();
	}
}

void AbstractFileReceiver::handle_handshake(const boost::system::error_code& error)
{
	if (!error)
	{
		_STD stringstream sstream;
		sstream
			<< _FSP RECEIVER_CODE_STR
			<< _FSP DOWNLOAD_TRANSACTION_DELIM_STR
			<< _STD to_string(ConnectID)
			<< _FSP LINE_SEPARATOR;
		auto ConnectMessage = sstream.str();

		boost::system::error_code ec;

		_ASIO write(Socket, _ASIO buffer(ConnectMessage.data(), ConnectMessage.length()), ec);
		CHECK_IO_OPERATION_STATUS(ec)

		start_receiving();
	}
	else
	{
		OnError();
	}
}