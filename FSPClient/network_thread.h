#pragma once
#include <boost/asio/io_context.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/thread.hpp>

#define NUMBER_OF_IO_THREADS_FOR_IO_CTX_RUN 2

struct network_thread
{
	/// Constructor which starts network_thread
	network_thread() :
		io_context_(),
		io_work_(io_context_)
	{
		for (int i = 0; i < NUMBER_OF_IO_THREADS_FOR_IO_CTX_RUN; i++)
			this->io_threads_.create_thread([this]() {this->io_context_.run(); });
	}

	network_thread(network_thread const&) = delete;
	network_thread& operator=(network_thread const&) = delete;
	~network_thread() 
	{
		this->io_context_.stop();
		this->io_threads_.join_all();
	}

	boost::asio::io_context&	context()				{ return io_context_; }
private:
	/// Context on which all async I/O will be scheduled
	boost::asio::io_context							io_context_;
	/// We need some work so io_context_.run() will not terminate prematurely
	boost::asio::io_context::work					io_work_;
	/// Thread on which completion handlers will be invoked and run() method will be called
	boost::thread_group								io_threads_;
};