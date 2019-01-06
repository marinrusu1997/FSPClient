#include "compressed_file_deliver.h"
#include "abstract_compressor.h"

#include <boost/bind.hpp>
#include <iomanip>

#include "protocol.h"

#define _ASIO ::boost::asio::
#define _FSP ::fsp::protocol::

#define CHECK_IO_OPERATION_STATUS(error_code)		if ((error_code))	{	Handler();	return;	}
#define CHECK_FILE_DELIVERING_STATUS				if ((DoneFlag))		{	Handler();	return;	}

void CompressedFileDeliver::start_delivering()
{
	FileOnDisk.async_read(boost::bind(&CompressedFileDeliver::handle_read, this, _1, _2), _ASIO buffer(Buffer));
}

void CompressedFileDeliver::handle_read(const boost::system::error_code& error, size_t bytes_transferred)
{
	if (!error || error == _ASIO error::eof && bytes_transferred > 0)
	{
		if (error == _ASIO error::eof)
			DoneFlag = true;

		CompressedChunk = Compressor->compress({ Buffer ,bytes_transferred });

		_STD stringstream sstream;
		sstream << _STD setw(_FSP COMPRESSED_CHUNK_HEADER_LENGTH) << _STD setfill('0') << CompressedChunk.length();
		_STD string compr_len = sstream.str();

		boost::system::error_code ec;
		_ASIO write(Socket, _ASIO buffer(compr_len, compr_len.length()), ec);
		CHECK_IO_OPERATION_STATUS(ec)

		_ASIO async_write(Socket, _ASIO buffer(CompressedChunk, CompressedChunk.length()), 
			boost::bind(&CompressedFileDeliver::handle_write, this, _1, _2));
	}
	else
	{
		Handler();
	}
}

void CompressedFileDeliver::handle_write(const boost::system::error_code& error, const size_t bytes_wroted)
{
	if (!error)
	{
		CHECK_FILE_DELIVERING_STATUS
		FileOnDisk.async_read(boost::bind(&CompressedFileDeliver::handle_read, this, _1, _2), 
			_ASIO buffer(Buffer));
	}
	else
	{
		Handler();
	}
}