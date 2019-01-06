#include "notification_validator.h"
#include "message.h"
#include "fsp_headers.h"
#include "fsp_responses.h"
#include "fsp_notification_types.h"

namespace hdr = fsp::protocol::headers;
namespace rsp = fsp::protocol::responses;

using namespace fsp::protocol::impl::validators;

_NODISCARD const bool notification_validator::validate(fsp::protocol::message::message& notification) noexcept
{
	if (!notification.have_header(hdr::NotificationType))
		return false;

	const auto& NotificationType = notification[hdr::NotificationType];

	if (!hdr::notifications::IsNotificationTypeValid(NotificationType))
		return false;

	if (NotificationType == hdr::notifications::USER_REQUESTED_FILE_DOWNLOAD
		&& (!notification.have_header(hdr::DownloadPort)
			|| !notification.have_header(hdr::DownloadTransactionID)
			|| !notification.have_header(hdr::Compressions)))
		return false;

	if ( (NotificationType == hdr::notifications::USER_ADDED_NEW_FILE || 
		NotificationType == hdr::notifications::USER_DELETED_FILE ||
		NotificationType == hdr::notifications::USER_RENAMED_FILE) &&
		(!notification.have_header(hdr::FileName) ||
			!notification.have_header(hdr::FileOwner)))
		return false;

	if (NotificationType == hdr::notifications::USER_RENAMED_FILE &&
		!notification.have_header(hdr::FileNewName))
		return false;

	if (NotificationType == hdr::notifications::USER_LOGGED_OUT && !notification.have_header(hdr::FileOwner))
		return false;

	if (NotificationType == hdr::notifications::USER_REGISTERED_FILES)
	{
		if (!notification.have_header(hdr::FileOwner))
			return false;
		if (notification.have_header(hdr::ContentLength) && !notification.have_header(hdr::ContentType))
			return false;
	}

	
	if (NotificationType == hdr::notifications::FILES_OF_OTHER_USERS &&
		notification.have_header(hdr::ContentLength) &&
		!notification.have_header(hdr::ContentType))
		return false;

	return true;
}
