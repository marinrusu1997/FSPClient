#include "stream_file_receiver.h"
#include <boost/bind.hpp>

#define _ASIO	::boost::asio::
#define _BOOST	::boost::

#define CHECK_FOR_CANCELLED_DOWNLOAD if (IsCancelled) { Socket.shutdown(); _BOOST system::error_code ec; Socket.lowest_layer().close(ec); OnCancelled(); return; }
#define CHECK_FOR_DOWNLOAD_COMPLETION	BytesWrottedToDisck == FileSize ? OnSuccess() : OnError();

void StreamFileReceiver::start_receiving()
{
	CHECK_FOR_CANCELLED_DOWNLOAD
	_ASIO async_read(Socket, _ASIO buffer(Buffer), _BOOST bind(&StreamFileReceiver::handle_read, this, _1, _2));
}

void StreamFileReceiver::handle_read(const boost::system::error_code& error, size_t bytes_transferred)
{
	if (!error || bytes_transferred > 0)
		FileOnDisk.async_write(_BOOST bind(&StreamFileReceiver::handle_write, this, _1, _2), _ASIO buffer(Buffer, bytes_transferred));
	else
		CHECK_FOR_DOWNLOAD_COMPLETION
}

void StreamFileReceiver::handle_write(const boost::system::error_code& error, const size_t bytes_wroted)
{
	if (!error)
	{
		BytesWrottedToDisck += bytes_wroted;
		CHECK_FOR_CANCELLED_DOWNLOAD
		AbstractFileReceiver::OnChunkWrottedOnDisk();
		_ASIO async_read(Socket, _ASIO buffer(Buffer), _BOOST bind(&StreamFileReceiver::handle_read, this, _1, _2));
	}
	else
		CHECK_FOR_DOWNLOAD_COMPLETION
}