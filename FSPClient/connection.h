#pragma once

#include <string_view>
#include <memory>
#include <forward_list>

#include <boost/asio/ip/tcp.hpp>

#include "mutable_buffer.h"
#include "msg_parser.h"

#include "request_manager.h"
#include "download_manager.h"
#include "querry_manager.h"
#include "notification_handler.h"
#include "delayed_invoker.h"
#include "heartbeat.h"

#include "SharedDirectory.h"

#include <QtCore/qobject.h>

class connection final : public QObject
{
	Q_OBJECT

signals:
	void		InternetConnectionError(std::string description);
private slots:
	void		OnNoHeartBeat();
public:
	explicit	connection(boost::asio::io_context&	io_context_);

	void		start(_STD forward_list<_STD string> const&  addresses, const _STD string_view port);

	void		start_write(const _STD string_view BufferView);

	void		start_write_mv(_STD string&& MovableBuffer);

	auto&		RequestManager() noexcept { return request_manager_; }

	auto&		NotificationHandler() noexcept { return notification_handler_; }

	auto&		DownloadManager() noexcept { return download_manager_; }

	auto&		SharedDir() noexcept { return directory_; }

	auto&		get_io_context() { return stream_.get_io_context(); }
private:
	
	typedef	_STD forward_list<boost::asio::ip::tcp::resolver::iterator>	endpoints_collection_t;
	typedef endpoints_collection_t::iterator							endpoint_iterator_t;

	void async_send(const _STD string_view BufferView, const size_t Offset);
	void async_send(_STD string&& MovableBuffer, const size_t Offset);

	bool write_guard();
	void write_error(const boost::system::error_code& err_code, const _STD string_view BufferView);
	//implementing read interface
	void start_read();
	bool read_guard();
	void read_action();
	void read_error(const boost::system::error_code& err_code);

	void start_connect(endpoint_iterator_t endpoint_iter, const boost::system::error_code& ec);
	void handle_connect(const boost::system::error_code& ec, endpoint_iterator_t endpoint_iter);
	void handle_connect_error(const boost::system::error_code& ec);
	void handle_connect_success();

	/// Stream on which we will send/receive messages
	boost::asio::ip::tcp::socket					stream_;
	/// Buffer for storing of the bytes which comes over network
	mutable_buffer<char, static_cast<size_t>(5120)>	read_buffer_;
	/// Protocol parser
	fsp::protocol::message::parsers::message_parser parser_;
	/// Buffer for assembly of messageds which comes over network
	fsp::protocol::message::message					message_buffer_;
	/// Request manager for this connection
	request_manager									request_manager_;
	/// Download manager
	download_manager								download_manager_;
	/// Notification handler for this connection
	notification_handler							notification_handler_;
	/// Querry manager
	querry_manager									querry_manager_;
	/// Flag which indicates that connection is stopped
	bool											isAlive_;
	/// Endpoints back-up in case of disconnect
	endpoints_collection_t							endpoints_;
	/// Timer for delayed connect
	delayed_invoker									invoker_;
	/// Heartbeat mechanism
	heartbeat										heartbeat_;
	/// Shared directory
	SharedDirectory									directory_;
};

