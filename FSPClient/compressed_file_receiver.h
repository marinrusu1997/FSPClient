#pragma once
#include "abstract_file_receiver.h"
namespace fsp::protocol::compressors {
		struct AbstractCompressor;
}

struct CompressedFileReceiver final : AbstractFileReceiver
{
	CompressedFileReceiver(HANDLE OpenedFileOnDisk, boost::asio::io_context& io_ctx, uintmax_t FileSize,
		boost::asio::ssl::context& ssl_ctx, uintmax_t ConnectID, DownloadHandler_t&& DownloadHandler,
		fsp::protocol::compressors::AbstractCompressor * Decompressor)
		:	AbstractFileReceiver(OpenedFileOnDisk, io_ctx, FileSize, ssl_ctx, ConnectID, _STD move(DownloadHandler)),
			Decompressor(Decompressor),
			DecompressedChunk()
	{}
private:
	void start_receiving() override;

	void handle_read(const boost::system::error_code& error, size_t bytes_transferred);

	void handle_write(const boost::system::error_code& error, const size_t bytes_wroted);

	fsp::protocol::compressors::AbstractCompressor *Decompressor;
	_STD string										DecompressedChunk;
};