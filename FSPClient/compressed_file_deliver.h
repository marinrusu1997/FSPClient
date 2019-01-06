#pragma once

#include "abstract_file_deliver.h"

namespace fsp::protocol::compressors {
	struct AbstractCompressor;
}

struct CompressedFileDeliver final : AbstractFileDeliver
{
	CompressedFileDeliver(HANDLE OpenedFileOnDisk, boost::asio::io_context& io_ctx, boost::asio::ssl::context& ssl_ctx,
		handler&& CompletionHandler, uintmax_t ConnectID, fsp::protocol::compressors::AbstractCompressor *Compressor)
		: AbstractFileDeliver(OpenedFileOnDisk, io_ctx, ssl_ctx, _STD move(CompletionHandler), ConnectID),
		Compressor(Compressor)
	{}
private:
	_STD string										CompressedChunk;
	fsp::protocol::compressors::AbstractCompressor	*Compressor;

	void start_delivering() override;

	void handle_read(const boost::system::error_code& error, size_t bytes_transferred);

	void handle_write(const boost::system::error_code& error, const size_t bytes_wroted);
};