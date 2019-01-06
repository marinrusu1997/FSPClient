#pragma once

namespace fsp::protocol::message {
	struct message;
}

namespace fsp::protocol::impl::validators {
	struct querry_validator final
	{
		[[nodiscard]] static const char * const validate(fsp::protocol::message::message& querry) noexcept;
	};
}