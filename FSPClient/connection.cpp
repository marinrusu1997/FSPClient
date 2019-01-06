#include "connection.h"
#include "reply_validator.h"
#include "notification_validator.h"
#include "querry_validator.h"
#include "fsp_commands.h"
#include "fsp_responses.h"

using namespace fsp::protocol::impl::validators;

#define RECONNECT_DELAY_SECONDS	15

connection::connection(boost::asio::io_context&	io_context_) :
	stream_(io_context_),
	request_manager_(*this),
	download_manager_(io_context_),
	notification_handler_(download_manager_),
	isAlive_(false),
	heartbeat_(*this),
	querry_manager_(*this)
{
	const bool IsQtConnectMade = connect(&heartbeat_, &heartbeat::NoHeartBeat, this, &connection::OnNoHeartBeat);
	assert(IsQtConnectMade);
}

void connection::start(_STD forward_list<_STD string> const&  addresses, const _STD string_view port)
{
	boost::asio::ip::tcp::resolver resolver(stream_.get_io_context());
	for (const auto& address : addresses)
		endpoints_.push_front(resolver.resolve(boost::asio::ip::tcp::resolver::query(address.data(), port.data())));

	start_connect(endpoints_.begin(), boost::system::error_code()); // dummy ec
}

void connection::start_connect(endpoint_iterator_t endpoint_iter, const boost::system::error_code& ec)
{
	if (endpoint_iter != endpoints_.end())
	{
		// Start the asynchronous connect operation.
		stream_.async_connect(endpoint_iter->operator*().endpoint(),
			[this, endpoint_iter = endpoint_iter](const auto& ec)
			{
				this->handle_connect(ec, const_cast<endpoint_iterator_t&>(endpoint_iter));
			}
		);
	}
	else
	{
		// There are no more endpoints to try. Handle the connect error
		handle_connect_error(ec);
	}
}

void connection::handle_connect(const boost::system::error_code& ec, endpoint_iterator_t endpoint_iter)
{
	if (ec)
	{
		// We need to close the socket used in the previous connection attempt
		// before starting a new one.
		stream_.close();

		// Try the next available endpoint.
		start_connect(++endpoint_iter, ec);
	}
	else
	{
		this->request_manager_.set_server_address((*(*endpoint_iter)).endpoint().address().to_string());
		this->notification_handler_.set_server_address((*(*endpoint_iter)).endpoint().address().to_string());
		handle_connect_success();
	}
}

void connection::handle_connect_error(const boost::system::error_code& ec)
{
	// first neet to close socket, in case we need to reconect. nvm better close it
	if (stream_.is_open())
		stream_.close();
	// anounce observers in the sistem that there is a problem with internet connection
	emit InternetConnectionError(ec.message() + " " + ec.category().name() + " " + _STD to_string(ec.value()));
	if (ec.category() == boost::asio::error::system_category)
	{
		if (ec.value() != boost::asio::error::already_connected)
		{
			invoker_.delay_task([this]() {this->start_connect(endpoints_.begin(), boost::system::error_code()); }, boost::posix_time::seconds(RECONNECT_DELAY_SECONDS), get_io_context());
		}
	}	
}

void connection::handle_connect_success()
{
	emit InternetConnectionError("Connection successfull");
	// from now on connection is alive
	isAlive_ = true;
	// execute first tasks that werent sent, due to connection problems
	invoker_.execute_enqueued_tasks();
	// start heartbeat mechanism
	heartbeat_.start();
	// start read responses & notifications from server
	start_read();
}

void connection::OnNoHeartBeat()
{
	isAlive_ = false;
	emit InternetConnectionError("No HeartBeat from the server. Trying to reconnect...");
	if (stream_.is_open())
		stream_.close();
	invoker_.delay_task([this]() {this->start_connect(endpoints_.begin(), boost::system::error_code()); }, boost::posix_time::seconds(RECONNECT_DELAY_SECONDS), get_io_context());
}

void connection::start_write(const _STD string_view BufferView)
{
	if (write_guard())
	{
		async_send(BufferView, 0);
	}
}

void connection::async_send(const _STD string_view BufferView, const size_t Offset)
{
	assert(Offset < BufferView.size());
	const auto data = BufferView.data() + Offset;
	const auto size = BufferView.size() - Offset;

	stream_.async_send(boost::asio::buffer(data,size),
		[this, BufferView = BufferView, CurrentOffset = Offset]
	(const auto& ec, const auto& BytesTransferred)
		{
			if (!ec)
			{
				if (const_cast<size_t&>(CurrentOffset) += BytesTransferred; CurrentOffset != BufferView.size())
				{
					this->async_send(BufferView, CurrentOffset);
				}
			}
			else
			{
				write_error(ec,BufferView);
			}
		}
	);
}

void connection::start_write_mv(_STD string&& MovableBuffer)
{
	if (write_guard())
	{
		async_send(_STD move(MovableBuffer), 0);
	}
}

void connection::async_send(_STD string&& MovableBuffer, const size_t Offset)
{
	assert(Offset < MovableBuffer.size());
	const auto data = MovableBuffer.data() + Offset;
	const auto size = MovableBuffer.size() - Offset;

	stream_.async_send(boost::asio::buffer(data, size),
		[this, MovableBuffer = MovableBuffer, CurrentOffset = Offset]
	(const auto& ec, const auto& BytesTransferred)
		{
			if (!ec)
			{
				if (const_cast<size_t&>(CurrentOffset) += BytesTransferred; CurrentOffset != MovableBuffer.size())
				{
					this->async_send(_STD move(const_cast<decltype(MovableBuffer)&>(MovableBuffer)), CurrentOffset);
				}
			}
			else
			{
				write_error(ec, MovableBuffer);
			}
		}
	);
}

void connection::start_read()
{
	if (read_guard())
	{
		namespace prsr = fsp::protocol::message::parsers;
		using namespace fsp::protocol::message::builders;
		using namespace fsp::protocol::responses;
		using namespace fsp::protocol;

		stream_.async_read_some(boost::asio::buffer(this->read_buffer_.buffer()),
			[this](const auto& ec,const auto& bytes_transferred) {
				if (!ec)
				{
					this->read_buffer_.current_size(bytes_transferred);

					bool SyntacticErrorSendBefore = false;

					while (this->read_buffer_.can_read())
					{
						auto&&[result, iter] = this->parser_.parse(this->message_buffer_, this->read_buffer_.begin_read_iter(), this->read_buffer_.end_read_iter());

						this->read_buffer_.consume(iter);

						if (result == prsr::message_parser::good) 
						{
							this->read_action();
							SyntacticErrorSendBefore = false;
						}
						else if (result == prsr::message_parser::bad && SyntacticErrorSendBefore == false) 
						{
							start_write(StringReplyMessage().SetReplyCode(SYNTACTIC_ERROR).SetRequestID(CMD_ID_WHEN_NO_CMDID_PRESENT).SetEndOfProtocolHeader().build());
							SyntacticErrorSendBefore = true;
						}
						else if (result == prsr::message_parser::indeterminate) {
							this->start_read();
							return;
						}

						this->message_buffer_.clear();
					}

					this->start_read();
				}
				else
				{
					this->read_error(ec);
				}
			});
	}
}

bool connection::write_guard()
{
	if (!isAlive_)
	{
		emit InternetConnectionError("Connection is not alive");
		return false;
	}
	else
	{
		return true;
	}
}

/// WARNING STUB! NEED PROPER HANDLING
void connection::write_error(const boost::system::error_code& ec, const _STD string_view)
{
	isAlive_ = false;
	handle_connect_error(ec);
	// intended for debug
	/*emit InternetConnectionError(ec.message() + " " + ec.category().name() + " " + _STD to_string(ec.value()));
	if (ec.category() == boost::asio::error::system_category)
	{
		if (ec.value() == boost::asio::error::not_connected)
		{
			invoker_.enqueue_task(
				[this, BufferView = BufferView]()
			{
				this->async_send(BufferView, 0);
			}
			);
			handle_connect_error(ec);
		}
	}*/
}

bool connection::read_guard()
{
	if (!isAlive_)
	{
		emit InternetConnectionError("Connection is not alive");
		return false;
	}
	else
	{
		return true;
	}
}


void connection::read_action()
{
	const auto&  ReceivedCommand = message_buffer_.command();
	if (ReceivedCommand == fsp::protocol::commands::HEARTBEAT)
	{
		this->heartbeat_.acknowledge_heartbeat();
		return;
	}
	if (ReceivedCommand == fsp::protocol::commands::REPLY && reply_validator::validate(message_buffer_) != nullptr)
	{
		request_manager_.handle_reply(message_buffer_);
		return;
	}
	if (ReceivedCommand == fsp::protocol::commands::NOTIFICATION && notification_validator::validate(message_buffer_))
	{
		notification_handler_.handle_notification(message_buffer_);
		return;
	}
	if (ReceivedCommand == fsp::protocol::commands::QUERRY)
	{
		if (const auto response = querry_validator::validate(message_buffer_); response != nullptr)
		{
			start_write_mv(_STD move(fsp::protocol::message::builders::StringReplyMessage().SetReplyCode(response).SetRequestID(message_buffer_[fsp::protocol::headers::CommandId]).build()));
			return;
		}
		querry_manager_.resolve_querry(message_buffer_);
		return;
	}
}

/// STUB TO DO PROPER ERROR HANDLING
void connection::read_error(const boost::system::error_code& ec)
{
	// intended for debug
	emit InternetConnectionError(ec.message() + " " + ec.category().name() + " " + _STD to_string(ec.value()));
}


