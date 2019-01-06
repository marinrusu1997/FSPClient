#include "querry_manager.h"
#include "connection.h"
#include "fsp_headers.h"
#include "fsp_responses.h"
#include "fsp_ssl_versions.h"

using fsp::protocol::message::builders::StringReplyMessage;
namespace hdr = fsp::protocol::headers;
namespace rsp = fsp::protocol::responses;
namespace ssl = fsp::protocol::ssl_versions;

querry_manager::querry_manager(connection& c)
	: connection_(c)
{}

void querry_manager::resolve_querry(fsp::protocol::message::message& Querry)
{
	const auto& CommandId = Querry[hdr::CommandId];
	
	auto& DownloadManager = connection_.DownloadManager();
	if (DownloadManager.GetSslVersion() != ssl::Version(Querry[hdr::SslVersion]))
	{
		connection_.start_write_mv(_STD move(StringReplyMessage().SetReplyCode(rsp::SSL_VERSION_INCOMPATIBLE).SetRequestID(CommandId).build()));
		return;
	}

	if (connection_.DownloadManager().AreUploadsDenied())
	{
		connection_.start_write_mv(_STD move(StringReplyMessage().SetReplyCode(rsp::DOWNLOAD_DENY).SetRequestID(CommandId).build()));
		return;
	}

	if (auto UploadInfo = DownloadManager.BeginUploadTransaction(_STD stoull(CommandId),_STD move(connection_.SharedDir().path().string().append(_STD move(Querry[hdr::FileName])))); !UploadInfo.Succes)
	{
		connection_.start_write_mv(_STD move(StringReplyMessage().SetReplyCode(rsp::DOWNLOAD_DENY).SetRequestID(CommandId).build()));
		return;
	}
	else
	{
		connection_.start_write_mv(_STD move(StringReplyMessage()
			.SetReplyCode(rsp::DOWNLOAD_FILE_QUERRY_APPROVED)
			.SetRequestID(CommandId)
			.SetAditionalHeaders(hdr::FileSize, _STD to_string(UploadInfo.FileSize))
			.SetAditionalHeaders(hdr::Compressions, fsp::protocol::compressions::Compression(UploadInfo.Compression))
			.SetEndOfProtocolHeader()
			.build()));
		return;
	}
}