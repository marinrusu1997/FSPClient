#pragma once
#include "abstract_file_deliver.h"

struct StreamFileDeliver final : AbstractFileDeliver
{
	StreamFileDeliver(HANDLE OpenedFileOnDisk, boost::asio::io_context& io_ctx, boost::asio::ssl::context& ssl_ctx,
		handler&& CompletionHandler, uintmax_t ConnectID)
		: AbstractFileDeliver(OpenedFileOnDisk, io_ctx, ssl_ctx, _STD move(CompletionHandler), ConnectID)
	{}

private:
	void start_delivering() override;

	void handle_read(const boost::system::error_code& error, size_t bytes_transferred);

	void handle_write(const boost::system::error_code& error, const size_t bytes_wroted);
};