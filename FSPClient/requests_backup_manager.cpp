#include "requests_backup_manager.h"
#include "protocol.h"

Q_DECLARE_METATYPE(requests_backup_manager::req_id_key_t);

requests_backup_manager::requests_backup_manager(simple_timer::duration	deadline) : deadline_(deadline)
{
	qRegisterMetaType<requests_backup_manager::req_id_key_t>();
}

requests_backup_manager::~requests_backup_manager() noexcept
{
	for (const auto&[RequestID, RequestTimerPair] : backedup_requests_)
		RequestTimerPair.second->stop();
}

_NODISCARD const _STD string&	requests_backup_manager::BackupAndStartTimer(req_id_key_t const& RequestID, _STD string& Request, boost::asio::io_context& ContextForTimer) noexcept
{
	auto timer = _STD make_shared <simple_timer> (ContextForTimer);
	timer->set_handler([this, RequestID = RequestID, self = timer->shared_from_this()](const auto& error_code)
			{
				if (error_code != boost::asio::error::operation_aborted && *self)
					emit const_cast<requests_backup_manager*>(this)->NoCommandReply(RequestID);
				const_cast<timer_ptr_t&>(self).reset();
			}
	);

	this->backedup_requests_.emplace(RequestID,_STD pair{ _STD move(Request), _STD move(timer) });
	this->backedup_requests_[RequestID].second->start(this->deadline_);
	return this->backedup_requests_[RequestID].first; 
}

_NODISCARD const bool			requests_backup_manager::IsRequestIDValid(req_id_key_t const& RequestID) const noexcept
{
	return this->backedup_requests_.find(RequestID) != this->backedup_requests_.end();
}

const _STD string&				requests_backup_manager::GetAndRestartTimer(req_id_key_t const& RequestID) noexcept
{
	this->backedup_requests_[RequestID].second->restart();
	return this->backedup_requests_[RequestID].first;
}

_NODISCARD const _STD string	requests_backup_manager::GetAssociatedCommand(req_id_key_t const& RequestID) const noexcept
{
	if (const auto&& iter = this->backedup_requests_.find(RequestID); iter != this->backedup_requests_.end())
		return iter->second.first.substr(0, iter->second.first.find(fsp::protocol::LINE_SEPARATOR));
	else
		return "";
}

void							requests_backup_manager::StopTimerForAssociatedCommand(req_id_key_t const& RequestID) noexcept
{
	this->backedup_requests_[RequestID].second->stop();
}

const size_t					requests_backup_manager::Remove(req_id_key_t const& RequestID) noexcept
{
	return this->backedup_requests_.erase(RequestID);
}
