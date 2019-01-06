#pragma once
#include <map>
#include <string>
#include "inttypes.h"
#include "simple_timer.h"

#include <QtCore/qobject.h>

class requests_backup_manager final : public QObject
{
	Q_OBJECT
public:
	typedef uint64_t req_id_key_t;
signals:
	void NoCommandReply(const unsigned long long);
public:
	requests_backup_manager(simple_timer::duration	deadline);
	requests_backup_manager(requests_backup_manager const&) = delete;
	requests_backup_manager& operator=(requests_backup_manager const&) = delete;
	~requests_backup_manager() noexcept;

	_NODISCARD const _STD string&	BackupAndStartTimer(req_id_key_t const& RequestID,_STD string& Request, boost::asio::io_context& ContextForTimer) noexcept;
	_NODISCARD const bool			IsRequestIDValid(req_id_key_t const& RequestID) const noexcept;
	const _STD string&				GetAndRestartTimer(req_id_key_t const& RequestID) noexcept;
	_NODISCARD const _STD string	GetAssociatedCommand(req_id_key_t const& RequestID) const noexcept;
	void							StopTimerForAssociatedCommand(req_id_key_t const& RequestID) noexcept;
	const size_t					Remove(req_id_key_t const& RequestID) noexcept;
private:
	typedef _STD shared_ptr<simple_timer>			timer_ptr_t;
	typedef _STD pair <_STD string, timer_ptr_t>	backup_t;
	typedef _STD map  <req_id_key_t, backup_t >		requests_map;

	requests_map			backedup_requests_;
	simple_timer::duration	deadline_;
};