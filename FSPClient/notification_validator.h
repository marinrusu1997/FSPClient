#ifndef _FSP_NOTIFICATION_VALIDATOR_HEADER_
#define _FSP_NOTIFICATION_VALIDATOR_HEADER_

namespace fsp::protocol::message {
	struct message;
}

namespace fsp::protocol::impl::validators {
	struct notification_validator final
	{
		[[nodiscard]] static const bool validate(fsp::protocol::message::message& notification) noexcept;
	};
}

#endif // !_FSP_NOTIFICATION_VALIDATOR_HEADER_

