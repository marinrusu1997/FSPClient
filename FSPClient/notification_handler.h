#pragma once

#include <QtCore/qobject.h>
#include <set>
#include <map>
#include <string>
#include <memory>

namespace fsp::protocol::message {
	struct message;
}

class download_manager;

class notification_handler final : public QObject
{
	Q_OBJECT
public:
	notification_handler(download_manager&);
	typedef std::shared_ptr<std::map<std::string, std::set<std::string>>> files_of_users_t;

	void handle_notification(fsp::protocol::message::message& notification);
	void set_server_address(_STD string&& ServerIPAddress) { this->ServerIPAddress = _STD move(ServerIPAddress); }
signals:
	void UserRegisteredFilesNotification(const std::string,const std::shared_ptr<std::set<std::string>>);
	void UserLoggedOutNotification(const std::string);
	void UserAddedNewFileNotification(const std::string, const std::string);
	void UserDeletedPathNotification(const std::string, const std::string);
	void UserRenamedPathNotification(const std::string, const std::string, const std::string);
	void FilesOfOtherUsersNotification(const notification_handler::files_of_users_t);

private:
	_STD string ServerIPAddress;
	download_manager&	DownloadManager;
};