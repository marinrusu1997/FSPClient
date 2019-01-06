#include "request_manager.h"
#include "connection.h"
#include "protocol.h"
#include "message.h"
#include "fsp_commands.h"
#include "fsp_headers.h"
#include "fsp_responses.h"
#include "fsp_content_type_formats.h"
#include "fsp_querry_types.h"

#define BOOST_TIME_TO_WAIT_FOR_PRELIMINARY_RESPONSE boost::posix_time::seconds(20)

namespace prot = fsp::protocol;
namespace msg = fsp::protocol::message;
namespace bld = fsp::protocol::message::builders;
namespace cmd = fsp::protocol::commands;
namespace hdr = fsp::protocol::headers;
namespace rpl = fsp::protocol::responses;
namespace ctf = fsp::protocol::content_type_formats;

_NODISCARD _STD string request_manager::DownloadInfo::CSVCompressions() const noexcept 
{
	_STD string csv;
	for(const auto& Compression : this->SupportedCompressions)
		csv.append(Compression).append(fsp::protocol::CSV_DELIMITER_STR);
	if (!csv.empty())
		csv.pop_back();
	return csv;
}


request_manager::request_manager(connection& connection)
	:	connection_(connection), 
		backups_(BOOST_TIME_TO_WAIT_FOR_PRELIMINARY_RESPONSE)
{
	const bool result = connect(&backups_, &requests_backup_manager::NoCommandReply, this, &request_manager::on_no_command_reply);
	assert(result == true);
}

void request_manager::on_signin(const std::string_view nickname, const std::string_view pswd)
{
	++RequestIDCounter;
	this->connection_.start_write(
		this->backups_.BackupAndStartTimer(
			RequestIDCounter, /// Key
			bld::StringMessageBuilder()
			.SetCommand(cmd::SIGNIN)
			.SetHeader(hdr::CommandId, _STD to_string(RequestIDCounter))
			.SetHeader(hdr::Nickname, nickname)
			.SetHeader(hdr::Password, pswd)
			.SetEndOfProtocolHeader()
			.Build(), /// Value
			this->connection_.get_io_context() /// Context of Execution
		)
	);
}

void request_manager::on_signup(const std::string_view nickname, const std::string_view pswd)
{
	++RequestIDCounter;
	this->connection_.start_write(
		this->backups_.BackupAndStartTimer(
			RequestIDCounter,
			bld::StringMessageBuilder()
			.SetCommand(cmd::SIGNUP)
			.SetHeader(hdr::CommandId, _STD to_string(RequestIDCounter))
			.SetHeader(hdr::Nickname, nickname)
			.SetHeader(hdr::Password, pswd)
			.SetEndOfProtocolHeader()
			.Build(), /// Value
			this->connection_.get_io_context() /// Context of Execution
		)
	);
}

void request_manager::on_logout()
{
	++RequestIDCounter;
	this->connection_.start_write(
		this->backups_.BackupAndStartTimer(
			RequestIDCounter,
			bld::StringMessageBuilder()
			.SetCommand(cmd::LOGOUT)
			.SetHeader(hdr::CommandId, _STD to_string(RequestIDCounter))
			.SetEndOfProtocolHeader()
			.Build(), /// Value
			this->connection_.get_io_context() /// Context of Execution
		)
	);
}

void request_manager::on_delete_account()
{
	++RequestIDCounter;
	this->connection_.start_write(
		this->backups_.BackupAndStartTimer(
			RequestIDCounter,
			bld::StringMessageBuilder()
			.SetCommand(cmd::DELETE_ACCOUNT)
			.SetHeader(hdr::CommandId, _STD to_string(RequestIDCounter))
			.SetEndOfProtocolHeader()
			.Build(), /// Value
			this->connection_.get_io_context() /// Context of Execution
		)
	);
}

void request_manager::on_push_files(_STD set<_STD string> const& file_list)
{
	++RequestIDCounter;
	
	bld::StringMessageBuilder Message = bld::StringMessageBuilder()
										.SetCommand(cmd::PUSH_FILES)
										.SetHeader(hdr::CommandId, _STD to_string(RequestIDCounter));
	if (!file_list.empty())
	{
		const auto&& BODY = bld::build_body(file_list, ctf::content_type_format::CSV_LIST);
		Message
			.SetHeader(hdr::ContentType, ctf::CSV_LIST)
			.SetHeader(hdr::ContentLength, _STD to_string(BODY.size()))
			.SetEndOfProtocolHeader()
			.SetBody(BODY);
	}
	else
	{
		Message.SetEndOfProtocolHeader();
	}

	this->connection_.start_write(
		this->backups_.BackupAndStartTimer(
			RequestIDCounter,	
			Message.Build(),
			this->connection_.get_io_context()
		)
	);
}

void request_manager::on_file_added(const _STD string_view FileName)
{
	++RequestIDCounter;
	this->connection_.start_write(
		this->backups_.BackupAndStartTimer(
			RequestIDCounter,
			bld::StringQuerryMessage()
			.SetRequestID(_STD to_string(RequestIDCounter))
			.SetQuerryType(fsp::protocol::headers::querries::ADD_FILE)
			.SetAditionalHeaders(fsp::protocol::headers::FileName, FileName)
			.SetEndOfProtocolHeader()
			.build(),
			this->connection_.get_io_context()
		)
	);
}

void request_manager::on_file_removed(const std::string_view FileName)
{
	++RequestIDCounter;
	this->connection_.start_write(
		this->backups_.BackupAndStartTimer(
			RequestIDCounter,
			bld::StringQuerryMessage()
			.SetRequestID(_STD to_string(RequestIDCounter))
			.SetQuerryType(hdr::querries::REMOVE_PATH)
			.SetAditionalHeaders(hdr::FileName,FileName)
			.SetEndOfProtocolHeader()
			.build(),
			this->connection_.get_io_context()
		)
	);
}

void request_manager::on_file_renamed(const std::string_view OriginalPath, const std::string_view NewName)
{
	++RequestIDCounter;
	this->connection_.start_write(
		this->backups_.BackupAndStartTimer(
			RequestIDCounter,
			bld::StringQuerryMessage()
			.SetRequestID(_STD to_string(RequestIDCounter))
			.SetQuerryType(hdr::querries::RENAME_PATH)
			.SetAditionalHeaders(hdr::FileName,OriginalPath)
			.SetAditionalHeaders(hdr::FileNewName,NewName)
			.SetEndOfProtocolHeader()
			.build(),
			this->connection_.get_io_context()
		)
	);
}

void request_manager::DownloadFileQuerry(const DownloadInfo& DownloadInfo)
{
	++RequestIDCounter;
	this->connection_.start_write(
		this->backups_.BackupAndStartTimer(
			RequestIDCounter,
			bld::StringQuerryMessage()
			.SetRequestID(_STD to_string(RequestIDCounter))
			.SetQuerryType(hdr::querries::DOWNLOAD_FILE)
			.SetAditionalHeaders(hdr::FileOwner, DownloadInfo.FileOwner)
			.SetAditionalHeaders(hdr::FileName, DownloadInfo.FilePath)
			.SetAditionalHeaders(hdr::SslVersion,fsp::protocol::ssl_versions::Version(DownloadInfo.SslVersion))
			.SetAditionalHeaders(hdr::Compressions,DownloadInfo.CSVCompressions())
			.SetEndOfProtocolHeader()
			.build(),
			this->connection_.get_io_context()
		)
	);
	connection_.DownloadManager().BeginDownloadTransaction(RequestIDCounter,
		_STD move(_STD string(DownloadInfo.FileOwner).append(DownloadInfo.FilePath)));
}

void request_manager::on_no_command_reply(const unsigned long long RequestID)
{
	this->connection_.start_write(this->backups_.GetAndRestartTimer(RequestID));
}

void request_manager::handle_reply(fsp::protocol::message::message& Reply)
{

	const auto RequestID	= std::stoull(Reply[hdr::CommandId]);
	const auto& ReplyCode	= Reply[hdr::ReplyCode];

	const auto&& BackedUpCommand = backups_.GetAssociatedCommand(RequestID);

	if (BackedUpCommand == cmd::QUERRY)
	{
		if (ReplyCode == rpl::DOWNLOAD_DENY ||
			ReplyCode == rpl::DOWNLOAD_FILE_QUERRY_APPROVED)
		{
			handle_download_querry(Reply);
			return;
		}
		handle_querry(ReplyCode, RequestID);
		return;
	}

	if (BackedUpCommand == cmd::SIGNIN)
	{
		handle_signin(ReplyCode, RequestID);
		return;
	}

	if (BackedUpCommand == cmd::SIGNUP)
	{
		handle_signup(ReplyCode, RequestID);
		return;
	}

	if (BackedUpCommand == cmd::LOGOUT && ReplyCode == rpl::LOGOUT_CMD_ACCEPTED)
	{
		emit logout_done();
	}

	if (BackedUpCommand == cmd::DELETE_ACCOUNT)
	{
		handle_delete_account(ReplyCode, RequestID);
		return;
	}

	if (BackedUpCommand == cmd::PUSH_FILES)
	{
		handle_push_files(ReplyCode,RequestID);
		return;
	}
}

void request_manager::handle_download_querry(fsp::protocol::message::message& Reply)
{
	const auto RequestID = std::stoull(Reply[hdr::CommandId]);
	const auto& ReplyCode = Reply[hdr::ReplyCode];

	if (ReplyCode == rpl::DOWNLOAD_DENY)
	{
		assert(backups_.Remove(RequestID));
		assert(connection_.DownloadManager().StopDownloadTransaction(RequestID));
		emit this->request_failure(rpl::responses.find(ReplyCode)->second.data());
		return;
	}

	if (ReplyCode == rpl::DOWNLOAD_FILE_QUERRY_APPROVED)
	{
		assert(backups_.Remove(RequestID));

		compression DesiredCompression;
		assert(fsp::protocol::compressions::Compression(Reply[hdr::Compressions], DesiredCompression));

		connection_.DownloadManager().StartDownloadTransaction(
			RequestID,
			DesiredCompression,
			_STD stoull(Reply[hdr::FileSize]),
			boost::asio::ip::tcp::resolver(connection_.DownloadManager().GetIOContext()).resolve(boost::asio::ip::tcp::resolver::query(ServerIPAddress,
				Reply[hdr::DownloadPort])),
			_STD stoull(Reply[hdr::DownloadTransactionID])
			);
		return;
	}
}

void request_manager::handle_querry(const std::string_view ReplyCode, const uint64_t RequestId)
{
	if (ReplyCode == rpl::QUERRY_CMD_ACCEPTED)
	{
		backups_.StopTimerForAssociatedCommand(RequestId);
		return;
	}

	if (ReplyCode == rpl::ADD_FILE_SUCCESSFULL ||
		ReplyCode == rpl::REMOVE_FILE_SUCCESSFULL)
	{
		assert(backups_.Remove(RequestId) == 1);
		return;
	}

	if (ReplyCode == rpl::REMOVE_FILE_FAILED)
	{
		assert(backups_.Remove(RequestId) == 1);
		emit request_failure(rpl::responses.find(ReplyCode)->second.data());
		return;
	}

	if (ReplyCode == rpl::INTERNAL_SERVER_ERR)
	{
		assert(backups_.Remove(RequestId) == 1);
		emit internal_server_err();
		return;
	}
}

void request_manager::handle_delete_account(const std::string_view ReplyCode, const uint64_t RequestId)
{
	if (ReplyCode == rpl::DELETE_ACCOUNT_CMD_ACCEPTED)
	{
		backups_.StopTimerForAssociatedCommand(RequestId);
		return;
	}
	if (ReplyCode == rpl::DELETING_ACCOUNT_SUCCESSFULL)
	{
		assert(backups_.Remove(RequestId) == 1);
		return;
	}
	if (ReplyCode == rpl::REMOVE_ACCOUNT_FAILED)
	{
		assert(backups_.Remove(RequestId) == 1);
		emit request_failure(rpl::responses.find(rpl::REMOVE_ACCOUNT_FAILED)->second.data());
		return;
	}
	if (ReplyCode == rpl::INTERNAL_SERVER_ERR)
	{
		assert(backups_.Remove(RequestId) == 1);
		emit internal_server_err();
		return;
	}
}

void request_manager::handle_signin(const std::string_view ReplyCode, const uint64_t RequestId)
{
	if (ReplyCode == rpl::SIGNIN_CMD_ACCEPTED)
	{
		backups_.StopTimerForAssociatedCommand(RequestId);
		return;
	}
	if (ReplyCode == rpl::LOGIN_SUCCESSFULL)
	{
		assert(backups_.Remove(RequestId) == 1);
		emit authentication_status(nullptr);
		return;
	}
	if (ReplyCode == rpl::USER_ALREADY_LOGGED)
	{
		assert(backups_.Remove(RequestId) == 1);
		emit authentication_status(rpl::responses.find(rpl::USER_ALREADY_LOGGED)->second.data());
		return;
	}
	if (ReplyCode == rpl::NICKNAME_INCORRECT)
	{
		assert(backups_.Remove(RequestId) == 1);
		emit authentication_status(rpl::responses.find(rpl::NICKNAME_INCORRECT)->second.data());
		return;
	}
	if (ReplyCode == rpl::PASSWORD_INCORRECT)
	{
		assert(backups_.Remove(RequestId) == 1);
		emit authentication_status(rpl::responses.find(rpl::PASSWORD_INCORRECT)->second.data());
		return;
	}
	if (ReplyCode == rpl::INTERNAL_SERVER_ERR)
	{
		assert(backups_.Remove(RequestId) == 1);
		emit internal_server_err();
		return;
	}	
}

void request_manager::handle_signup(const std::string_view ReplyCode, const uint64_t RequestId)
{
	if (ReplyCode == rpl::SIGNUP_CMD_ACCEPTED)
	{
		backups_.StopTimerForAssociatedCommand(RequestId);
		return;
	}
	if (ReplyCode == rpl::REGISTERING_SUCCESSFULL)
	{
		assert(backups_.Remove(RequestId) == 1);
		emit authentication_status(nullptr);
		return;
	}
	if (ReplyCode == rpl::USER_ALREADY_REGISTERED)
	{
		assert(backups_.Remove(RequestId) == 1);
		emit authentication_status(rpl::responses.find(rpl::USER_ALREADY_REGISTERED)->second.data());
		return;
	}
	if (ReplyCode == rpl::INTERNAL_SERVER_ERR)
	{
		assert(backups_.Remove(RequestId) == 1);
		emit internal_server_err();
		return;
	}
}

void request_manager::handle_push_files(const std::string_view ReplyCode, const uint64_t RequestId)
{
	if (ReplyCode == rpl::PUSH_FILES_CMD_ACCEPTED)
	{
		backups_.StopTimerForAssociatedCommand(RequestId);
		return;
	}

	if (ReplyCode == rpl::PUSH_FILES_SUCCESS)
	{
		assert(backups_.Remove(RequestId) == 1);
		return;
	}

	if (ReplyCode == rpl::INTERNAL_SERVER_ERR)
	{
		assert(backups_.Remove(RequestId) == 1);
		emit internal_server_err();
		return;
	}
}