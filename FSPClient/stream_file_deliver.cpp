#include "stream_file_deliver.h"
#include <boost/bind.hpp>

#define _ASIO ::boost::asio::

#define CHECK_FILE_DELIVERING_STATUS	if (DoneFlag) { Handler(); return; }

void StreamFileDeliver::start_delivering()
{
	FileOnDisk.async_read(boost::bind(&StreamFileDeliver::handle_read, this, _1, _2), _ASIO buffer(Buffer));
}

void StreamFileDeliver::handle_read(const boost::system::error_code& error, size_t bytes_transferred)
{
	if (!error || error == _ASIO error::eof && bytes_transferred > 0)
	{
		if (error == _ASIO error::eof)
			DoneFlag = true;
		_ASIO async_write(Socket, _ASIO buffer(Buffer, bytes_transferred), 
			boost::bind(&StreamFileDeliver::handle_write, this, _1, _2));
	}
	else
	{
		Handler();
	}
}

void StreamFileDeliver::handle_write(const boost::system::error_code& error, const size_t bytes_wroted)
{
	if (!error)
	{
		CHECK_FILE_DELIVERING_STATUS
		FileOnDisk.async_read(boost::bind(&StreamFileDeliver::handle_read, this, _1, _2), _ASIO buffer(Buffer));
	}
	else
	{
		Handler();
	}
}