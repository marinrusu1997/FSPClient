#pragma once

#include <memory>
#include <queue>
#include <boost/asio/io_context.hpp>

#include "simple_timer.h"

class delayed_invoker final
{
public:
	delayed_invoker() = default;
	delayed_invoker(delayed_invoker const&) = delete;
	delayed_invoker& operator=(delayed_invoker const&) = delete;

	template <class Func>
	void enqueue_task(Func&& task)
	{
		this->qeued_tasks_.push(_STD make_shared<item<Func>>(std::forward<Func>(task)));
	}

	template<class Func>
	void delay_task(Func&& task, simple_timer::duration delay, boost::asio::io_context& ctx)
	{
		auto timer_ = _STD make_shared<simple_timer>(ctx);
		timer_->set_handler([this, timer = timer_->shared_from_this(), task = std::move(std::forward<Func>(task)) ](const auto& ec) {
			if (ec != boost::asio::error::operation_aborted && *timer)
				task();
			const_cast<_STD shared_ptr<simple_timer>&>(timer).reset();
		});
		timer_->start(delay);
	}

	void execute_enqueued_tasks()
	{
		while (!qeued_tasks_.empty())
		{
			auto task(qeued_tasks_.front());
			qeued_tasks_.pop();
			task->execute_(task);
		}
	}

private:
	struct item_base
	{
		void(*execute_)(std::shared_ptr<item_base>&);
	};

	template <class Func>
	struct item : item_base
	{
		item(Func f) : function_(std::move(f))
		{
			execute_ = [](std::shared_ptr<item_base>& p)
			{
				Func tmp(std::move(static_cast<item*>(p.get())->function_));
				p.reset();
				tmp();
			};
		}

		Func function_;
	};

	_STD queue<_STD shared_ptr<item_base>>		qeued_tasks_;
};