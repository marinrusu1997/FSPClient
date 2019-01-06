#include "reply_validator.h"
#include "message.h"
#include "protocol.h"
#include "fsp_responses.h"

using namespace fsp::protocol::headers;
using namespace fsp::protocol;
using namespace fsp::protocol::responses;
using namespace fsp::protocol::impl::validators;

_NODISCARD const char* reply_validator::validate(fsp::protocol::message::message& reply) noexcept
{
	if (reply.have_header(CommandId)) {			
		return CMDID_HDR_REQUIRED;
	}
	if (reply.have_header(ReplyCode)) {	
		return RPL_CODE_HDR_REQUIRED;
	}

	return nullptr;
}