#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_MainWindow.h"

#include "connection.h"

class QLabel;
class QProgressBar;

class MainWindow : public QMainWindow
{
	Q_OBJECT

signals:
	void logout();
	void delete_account();

private slots:
	void OnSharedDirectoryContentChanged(const SharedDirectory::Change);

	void on_request_failure(const std::string_view);
	void onInternetConnectionError(std::string_view description);
	void onInternallServerError();

	void OnUserRegisteredFilesNotification(const std::string_view, const std::shared_ptr<std::set<std::string>>);
	void OnUserLoggedOutNotification(const std::string_view);
	void OnUserAddedNewFileNotification(const std::string_view, const std::string_view);
	void OnUserDeletedPathNotification(const std::string_view, const std::string_view);
	void OnUserRenamedPathNotification(const std::string_view, const std::string_view, const std::string_view);
	void OnFilesOfOtherUsersNotification(const notification_handler::files_of_users_t);

	void OnDownload(const std::string_view LocalPath, const AbstractFileReceiver::DownloadStatus Status, const unsigned char Progress);

	void on_logout_done();

	void on_logout_action_triggered(bool checked);
	void on_delete_acc_action_triggered(bool);
	void on_download_action_triggered(bool);

	void OnTreeViewContextMenuRequested(const QPoint & pos);
public:
	MainWindow(boost::asio::io_context& io_context, QWidget *parent = Q_NULLPTR);

	auto& Connection() { return connection_to_fsp_server_; }
	void send_directory_to_server();
private:
	Ui::MainWindowForm	ui;	
	connection			connection_to_fsp_server_;
	QLabel				*foot_label_;
	QProgressBar		*foot_progress_bar_;
};
