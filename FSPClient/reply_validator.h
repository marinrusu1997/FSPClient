#pragma once

namespace fsp::protocol::message {
	struct message;
}

namespace fsp::protocol::impl::validators {
	struct reply_validator final
	{
		[[nodiscard]] static const char* validate(fsp::protocol::message::message& reply) noexcept;
	};
}