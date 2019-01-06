#include "heartbeat.h"
#include "connection.h"
#include "protocol.h"

#define SEND_HEARTBEAT_INTERVAL std::chrono::seconds((FSP_SECONDS_INTERVAL_TO_SEND_HEARTBEAT))

heartbeat::heartbeat(connection& conn) 
	:	connection_(conn),
		timer_(conn.get_io_context(), std::chrono::steady_clock::now() + SEND_HEARTBEAT_INTERVAL),
		IsHeartBeatAcknowledged(false)
{}

heartbeat::~heartbeat()
{
	this->timer_.cancel();
}

void heartbeat::start() noexcept
{
	connection_.start_write(fsp::protocol::HEARTBEAT_MESSAGE);
	timer_.async_wait([this](const auto& ec) {this->tick(ec); });
}

void heartbeat::acknowledge_heartbeat() noexcept
{
	IsHeartBeatAcknowledged = true;
}

void heartbeat::tick(const boost::system::error_code& ec) noexcept
{
	if (ec == boost::asio::error::operation_aborted)
		return;

	if (this->IsHeartBeatAcknowledged)
	{
		this->IsHeartBeatAcknowledged = false;
		this->connection_.start_write(fsp::protocol::HEARTBEAT_MESSAGE);
		this->timer_.expires_at(timer_.expires_at() + SEND_HEARTBEAT_INTERVAL);
		this->timer_.async_wait([this](const auto& ec) {this->tick(ec); });
	}
	else
	{
		this->timer_.expires_from_now();
		emit this->NoHeartBeat();
	}
}
