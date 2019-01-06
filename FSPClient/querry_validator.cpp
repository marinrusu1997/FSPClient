#include "querry_validator.h"
#include "message.h"
#include "fsp_headers.h"
#include "fsp_responses.h"
#include "fsp_querry_types.h"
#include "fsp_ssl_versions.h"

using namespace fsp::protocol::headers;
using namespace fsp::protocol::impl::validators;
using namespace fsp::protocol::responses;
using namespace fsp::protocol::headers::querries;

_NODISCARD const char * const querry_validator::validate(fsp::protocol::message::message& querry) noexcept
{
	if (!querry.have_header(CommandId))
		return CMDID_HDR_REQUIRED;

	if (!querry.have_header(QuerryType))
		return QUERRY_TYPE_HDR_REQUIRED;

	const auto& QuerryType_ = querry[QuerryType];

	if (QuerryType_ == DOWNLOAD_FILE && !querry.have_header(FileName))
		return FILE_NAME_HDR_REQUIRED;

	if (QuerryType == querries::DOWNLOAD_FILE && !querry.have_header(SslVersion))
		return SSL_VERSION_HDR_REQUIRED;

	if (QuerryType == querries::DOWNLOAD_FILE && !fsp::protocol::ssl_versions::IsSslVersionValid(querry[SslVersion]))
		return INVALID_SSL_VERSION;

	return nullptr;
}