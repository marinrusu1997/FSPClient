#include "compressed_file_receiver.h"
#include "abstract_compressor.h"
#include <boost/bind.hpp>

#define _ASIO	::boost::asio::
#define _BOOST	::boost::
#define _FSP	::fsp::protocol::

#define CHECK_FOR_CANCELLED_DOWNLOAD				if (IsCancelled) { Socket.shutdown(); _BOOST system::error_code ec; Socket.lowest_layer().close(ec); OnCancelled(); return; }
#define CHECK_FOR_DOWNLOAD_COMPLETION				BytesWrottedToDisck == FileSize ? OnSuccess() : OnError();
#define CHECK_FOR_IO_OPERATION_STATUS(error_code)	if ((error_code)) { CHECK_FOR_DOWNLOAD_COMPLETION	return;	}

void CompressedFileReceiver::start_receiving()
{
	assert(Decompressor != nullptr);

	CHECK_FOR_CANCELLED_DOWNLOAD

	_BOOST system::error_code ec;
	_ASIO read(Socket, _ASIO buffer(Buffer, _FSP COMPRESSED_CHUNK_HEADER_LENGTH), ec);
	CHECK_FOR_IO_OPERATION_STATUS(ec)

	Buffer[_FSP COMPRESSED_CHUNK_HEADER_LENGTH] = (_STD byte)'\0';
	_ASIO async_read(Socket, _ASIO buffer(Buffer, _STD atoll((const char*)Buffer)), 
		_BOOST bind(&CompressedFileReceiver::handle_read, this, _1, _2));
}

void CompressedFileReceiver::handle_read(const boost::system::error_code& error, size_t bytes_transferred)
{
	if (!error || bytes_transferred > 0)
	{
		try {
			DecompressedChunk = Decompressor->decompress({(const char*)Buffer, bytes_transferred });
			FileOnDisk.async_write(_BOOST bind(&CompressedFileReceiver::handle_write, this, _1, _2), 
				_ASIO buffer(DecompressedChunk, DecompressedChunk.length()));
		}
		catch (_STD exception const& e) {
			CHECK_FOR_DOWNLOAD_COMPLETION
		}
	}
	else
		CHECK_FOR_DOWNLOAD_COMPLETION
}

void CompressedFileReceiver::handle_write(const boost::system::error_code& error, const size_t bytes_wroted)
{
	if (!error)
	{
		BytesWrottedToDisck += bytes_wroted;
		CHECK_FOR_CANCELLED_DOWNLOAD
		AbstractFileReceiver::OnChunkWrottedOnDisk();

		_BOOST system::error_code ec;
		_ASIO read(Socket, _ASIO buffer(Buffer, _FSP COMPRESSED_CHUNK_HEADER_LENGTH), ec);
		CHECK_FOR_IO_OPERATION_STATUS(ec)

		Buffer[_FSP COMPRESSED_CHUNK_HEADER_LENGTH] = (_STD byte)'\0';
		_ASIO async_read(Socket, _ASIO buffer(Buffer, _STD atoll((const char*)Buffer)),
			_BOOST bind(&CompressedFileReceiver::handle_read, this, _1, _2));
	}
	else
		CHECK_FOR_DOWNLOAD_COMPLETION
}