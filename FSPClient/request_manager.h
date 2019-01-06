#pragma once

#include <QtCore/qobject.h>
#include "requests_backup_manager.h"
#include "fsp_ssl_versions.h"
#include <forward_list>
#include <set>

class connection;

namespace fsp::protocol::message {
	struct message;
}

using fsp::protocol::ssl_versions::version;

class request_manager final : public QObject
{
	Q_OBJECT
public:
	struct DownloadInfo
	{
		_STD string_view						FileOwner;
		_STD string_view						FilePath;
		version									SslVersion;
		_STD forward_list<_STD string_view>		SupportedCompressions;

		_NODISCARD _STD string CSVCompressions() const noexcept;
	};
signals:
	void internal_server_err();

	void authentication_status(const char * msg);

	void logout_done();

	void request_failure(const std::string);	
public slots:
	void on_signin(const std::string_view, const std::string_view);
	void on_signup(const std::string_view, const std::string_view);
	void on_logout();
	void on_delete_account();

	void on_push_files(std::set<std::string> const& file_list);
	void on_file_added(const std::string_view);
	void on_file_removed(const std::string_view);
	void on_file_renamed(const std::string_view, const std::string_view);
private slots:
	void on_no_command_reply(const unsigned long long);
public:
	request_manager(connection& conn);
	void	handle_reply(fsp::protocol::message::message& Reply);
	void	DownloadFileQuerry(const DownloadInfo&);
	void	set_server_address(_STD string&& ServerIPAddress) { this->ServerIPAddress = _STD move(ServerIPAddress); }
private:

	void handle_signin(const std::string_view ReplyCode, const uint64_t RequestId);
	void handle_signup(const std::string_view ReplyCode, const uint64_t RequestId);
	void handle_delete_account(const std::string_view ReplyCode, const uint64_t RequestId);
	void handle_push_files(const std::string_view ReplyCode, const uint64_t RequestId);
	void handle_querry(const std::string_view ReplyCode, const uint64_t RequestId);
	void handle_download_querry(fsp::protocol::message::message& Reply);
	
	connection&											connection_;
	requests_backup_manager								backups_;
	_STD string											ServerIPAddress;
	inline static requests_backup_manager::req_id_key_t	RequestIDCounter = 1;
};


