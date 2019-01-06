#ifndef _FSP_HEART_BEAT_HEADER_
#define _FSP_HEART_BEAT_HEADER_

#include <boost/asio/steady_timer.hpp>
#include <QtCore/qobject.h>

class connection;

#ifdef FSP_SECONDS_INTERVAL_TO_SEND_HEARTBEAT
#undef FSP_SECONDS_INTERVAL_TO_SEND_HEARTBEAT
#endif
#define FSP_SECONDS_INTERVAL_TO_SEND_HEARTBEAT 60

struct heartbeat final : QObject
{
	Q_OBJECT
signals:
	void NoHeartBeat();
public:
	heartbeat(connection& conn);
	heartbeat(heartbeat const&) = delete;
	heartbeat& operator=(heartbeat const&) = delete;
	~heartbeat();

	void start() noexcept;
	void acknowledge_heartbeat() noexcept;
private:
	void tick(const boost::system::error_code&) noexcept;

	connection&					connection_;
	boost::asio::steady_timer	timer_;
	bool						IsHeartBeatAcknowledged;
};

#endif
