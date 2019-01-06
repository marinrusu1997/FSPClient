#include "notification_handler.h"
#include "message.h"
#include "fsp_commands.h"
#include "fsp_notification_types.h"
#include "download_manager.h"

namespace msg = fsp::protocol::message;
namespace cmd = fsp::protocol::commands;
namespace hdr = fsp::protocol::headers;

Q_DECLARE_METATYPE(std::shared_ptr<std::set<std::string>>);
Q_DECLARE_METATYPE(notification_handler::files_of_users_t);

notification_handler::notification_handler(download_manager& manager_)
	: DownloadManager(manager_)
{
	qRegisterMetaType<std::shared_ptr<std::set<std::string>>>();
	qRegisterMetaType<notification_handler::files_of_users_t>();
}

void notification_handler::handle_notification(msg::message& notification)
{
	const auto& NotificationType = notification[hdr::NotificationType];

	if (NotificationType == hdr::notifications::USER_REQUESTED_FILE_DOWNLOAD)
	{
		/// HANDLE THIS SHIT SOMEHOW
		using namespace fsp::protocol::compressors;
		using namespace fsp::protocol::compressions;
		using namespace boost::asio::ip;

		compression c;
		if (!Compression(notification[hdr::Compressions], c))
			return;

		DownloadManager.StartUploadTransaction
		(
			_STD stoull(notification[hdr::DownloadTransactionID]),
			tcp::resolver(DownloadManager.GetIOContext())
			.resolve(tcp::resolver::query(ServerIPAddress, notification[hdr::DownloadPort])),
			GetCompressorPointer(c)
		);
		return;
	}

	if (NotificationType == hdr::notifications::USER_REGISTERED_FILES) 
	{
		auto files = std::make_shared<std::set<std::string>>();
		notification.body(*files);
		emit this->UserRegisteredFilesNotification(std::move(notification[hdr::FileOwner]),std::move(files));
		return;
	}

	if (NotificationType == hdr::notifications::USER_LOGGED_OUT)
	{
		emit this->UserLoggedOutNotification(std::move(notification[hdr::FileOwner]));
		return;
	}

	if (NotificationType == hdr::notifications::USER_ADDED_NEW_FILE)
	{
		emit this->UserAddedNewFileNotification(_STD move(notification[hdr::FileOwner]), _STD move(notification[hdr::FileName]));
		return;
	}

	if (NotificationType == hdr::notifications::USER_DELETED_FILE)
	{
		emit this->UserDeletedPathNotification(_STD move(notification[hdr::FileOwner]), _STD move(notification[hdr::FileName]));
		return;
	}

	if (NotificationType == hdr::notifications::USER_RENAMED_FILE)
	{
		emit this->UserRenamedPathNotification(_STD move(notification[hdr::FileOwner]), _STD move(notification[hdr::FileName]), _STD move(notification[hdr::FileNewName]));
		return;
	}

	if (NotificationType == hdr::notifications::FILES_OF_OTHER_USERS)
	{
		auto files_of_users = std::make_shared<std::map<std::string, std::set<std::string>>>();
		notification.body_to_map(*files_of_users);
		emit this->FilesOfOtherUsersNotification(std::move(files_of_users));
		return;
	}

}