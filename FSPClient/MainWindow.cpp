#include "MainWindow.h"
#include "request_manager.h"
#include <filesystem>
#include <algorithm>
#include <QtWidgets/qmessagebox.h>
#include <QtWidgets/qlabel.h>
#include <QtWidgets/qprogressbar.h>
#include "fsp_commands.h"

struct TreeWidgetManager
{
	static void make_tree(QTreeWidgetItem* parent,const std::string_view owner, std::set<std::string> const& paths)
	{
		parent->setText(0, owner.data());
		for (const auto& path : paths)
			add(path, parent);
		parent->sortChildren(0, Qt::SortOrder::AscendingOrder);
	}

	static void add_path(QTreeWidgetItem *parent, const std::string_view Path)
	{
		add(Path.data(), parent);
	}

	static void remove_path(QTreeWidgetItem *parent, const std::string_view Path)
	{
		std::list<std::string> PathComponents;
		boost::split(PathComponents, Path, [](const auto c) {return c == fsp::protocol::PATH_SEPARATOR_CHR; });
		PathComponents.remove_if([](const auto& file) {return file.empty(); });
		if (PathComponents.empty())
			return;
		QTreeWidgetItem *NodeToDelete = nullptr;
		do_find_path(parent, PathComponents, NodeToDelete);
		if (NodeToDelete)
			delete NodeToDelete;
	}

	static void rename_path(QTreeWidgetItem *parent, const std::string_view OldPath, const std::string_view NewPath)
	{
		std::list<std::string> PathComponents;
		boost::split(PathComponents, OldPath, [](const auto c) {return c == fsp::protocol::PATH_SEPARATOR_CHR; });
		PathComponents.remove_if([](const auto& file) {return file.empty(); });
		if (PathComponents.empty())
			return;
		QTreeWidgetItem *NodeToRename = nullptr;
		do_find_path(parent, PathComponents, NodeToRename);
		if (NodeToRename)
		{	
			auto&& FileName = _STD filesystem::path(NewPath).filename().string();
			if (FileName.empty())
				return;
			NodeToRename->setText(0,FileName.c_str());
		}
	}

	static void my_path(QTreeWidgetItem *Node, _STD string& Path, _STD string& UserName)
	{
		auto UserNode{ path_to_me(Node, Path) };
		Path.append(Node->text(0).toStdString());
		UserName =  UserNode->text(0).toStdString();
	}
private:
	static void add(const std::string& path, QTreeWidgetItem* parent)
	{
		std::list<std::string> tokens;
		boost::split(tokens, path, [](const auto c) {return c == fsp::protocol::PATH_SEPARATOR_CHR ; });
		tokens.remove_if([](const auto& file) {return file.empty(); });
		bool done = false;
		do_add(parent, tokens,done);
	}

	static void do_add(QTreeWidgetItem* parent, std::list<std::string>& path_tokens, bool& done)
	{
		// defense guard, also stops recursion
		if (path_tokens.empty())
			return;

		QString currentDirFileName(path_tokens.front().data());

		for (auto i = 0; i < parent->childCount(); ++i) // go though all childs
		{
			auto root = parent->child(i); //get current root
			if (root->text(0) == currentDirFileName) // if we found a match
			{
				path_tokens.pop_front(); // we are done processing this one
				if (path_tokens.empty()) { //is this last file?
					auto file = new QTreeWidgetItem(root);
					file->setText(0, currentDirFileName); //add this file
					done = true; // this should stop for loop from previous calls
					return; //we are done inserting this path
				} 
				else {
					do_add(root, path_tokens, done); // go find next subdirectory
				}
			}
			if (done)
				break;
		}
		// when we come back from previous recursions, if file was inserted, tokens should pe empty
		if (path_tokens.empty())
			return;

		// if we are there then we didn't found the dir, we need to add it and start next do_add
		auto dir_or_file = new QTreeWidgetItem(parent);
		dir_or_file->setText(0, currentDirFileName);

		path_tokens.pop_front(); // we ar done processing this token

		do_add(dir_or_file, path_tokens,done); // go add next parts of path
	}

	static void do_find_path(QTreeWidgetItem *Parent, std::list<std::string>& PathComponents, QTreeWidgetItem*& FoundNode)
	{
		if (PathComponents.empty())
		{
			FoundNode = Parent;
			return;
		}

		QTreeWidgetItem *NextLevel = nullptr;
		auto count = Parent->childCount();
		QString CurrentComponent(PathComponents.front().data());
		for(auto i = 0; i < count; i++)
			if (Parent->child(i)->text(0) == CurrentComponent)
			{
				NextLevel = Parent->child(i);
				break;
			}
		if (NextLevel)
		{
			PathComponents.pop_front();
			do_find_path(NextLevel, PathComponents, FoundNode);
		}
		return;
	}

	static QTreeWidgetItem* path_to_me(QTreeWidgetItem *Node, _STD string& Path)
	{
		QTreeWidgetItem *CurrentParrent = Node->parent();
		while (CurrentParrent->parent())
		{
			Path.append("/");
			auto CurrentPathComponent{ CurrentParrent->text(0).toStdString() };
			Path.append(CurrentPathComponent.rbegin(), CurrentPathComponent.rend());
			CurrentParrent = CurrentParrent->parent();
		}
		Path.append("/");
		_STD reverse(Path.begin(), Path.end());
		return CurrentParrent;
	}
};


MainWindow::MainWindow(boost::asio::io_context& io_ctx, QWidget *parent) 
	: QMainWindow(parent), connection_to_fsp_server_(io_ctx)
{
	ui.setupUi(this);
	bool isConnectionSuccessfull = false;

	foot_label_			= new QLabel(this);
	foot_progress_bar_	= new QProgressBar(this);
	foot_progress_bar_->setTextVisible(false);

	ui.statusBar->addPermanentWidget(foot_label_);
	ui.statusBar->addPermanentWidget(foot_progress_bar_);

	// signal GUI -> slot Network I/O
	isConnectionSuccessfull = connect(this, &MainWindow::logout, &connection_to_fsp_server_.RequestManager(), &request_manager::on_logout);
	assert(isConnectionSuccessfull);
	isConnectionSuccessfull = connect(this, &MainWindow::delete_account, &connection_to_fsp_server_.RequestManager(), &request_manager::on_delete_account);
	assert(isConnectionSuccessfull);
	// signal Network -> slot GUI
	isConnectionSuccessfull = connect(&connection_to_fsp_server_, &connection::InternetConnectionError, this, &MainWindow::onInternetConnectionError);
	assert(isConnectionSuccessfull);
	// signal Network I/O -> slot GUI
	isConnectionSuccessfull = connect(&connection_to_fsp_server_.RequestManager(), &request_manager::logout_done, this, &MainWindow::on_logout_done);
	assert(isConnectionSuccessfull);
	isConnectionSuccessfull = connect(&connection_to_fsp_server_.RequestManager(), &request_manager::request_failure, this, &MainWindow::on_request_failure);
	assert(isConnectionSuccessfull);
	// signal Network I/O Notification -> slot GUI
	isConnectionSuccessfull = connect(&connection_to_fsp_server_.NotificationHandler(), &notification_handler::UserLoggedOutNotification, this, &MainWindow::OnUserLoggedOutNotification);
	assert(isConnectionSuccessfull);
	isConnectionSuccessfull = connect(&connection_to_fsp_server_.NotificationHandler(), &notification_handler::UserRegisteredFilesNotification, this, &MainWindow::OnUserRegisteredFilesNotification);
	assert(isConnectionSuccessfull);
	isConnectionSuccessfull = connect(&connection_to_fsp_server_.NotificationHandler(), &notification_handler::UserAddedNewFileNotification, this, &MainWindow::OnUserAddedNewFileNotification);
	assert(isConnectionSuccessfull);
	isConnectionSuccessfull = connect(&connection_to_fsp_server_.NotificationHandler(), &notification_handler::UserDeletedPathNotification, this, &MainWindow::OnUserDeletedPathNotification);
	assert(isConnectionSuccessfull);
	isConnectionSuccessfull = connect(&connection_to_fsp_server_.NotificationHandler(), &notification_handler::UserRenamedPathNotification, this, &MainWindow::OnUserRenamedPathNotification);
	assert(isConnectionSuccessfull);
	isConnectionSuccessfull = connect(&connection_to_fsp_server_.NotificationHandler(), &notification_handler::FilesOfOtherUsersNotification, this, &MainWindow::OnFilesOfOtherUsersNotification);
	assert(isConnectionSuccessfull);
	// signal Network I/O Download -> slot GUI
	isConnectionSuccessfull = connect(&connection_to_fsp_server_.DownloadManager(), &download_manager::download_test_signal, this, &MainWindow::OnDownload);
	assert(isConnectionSuccessfull);

	// signal HardDisk I/O -> slot GUI
	isConnectionSuccessfull = connect(&connection_to_fsp_server_.SharedDir(), &SharedDirectory::Changed, this, &MainWindow::OnSharedDirectoryContentChanged);
	assert(isConnectionSuccessfull);
		// signal GUI -> slot GUI
	isConnectionSuccessfull = connect(ui.actionLog_Out, &QAction::triggered, this, &MainWindow::on_logout_action_triggered);
	assert(isConnectionSuccessfull);
	isConnectionSuccessfull = connect(ui.actionDelete, &QAction::triggered, this, &MainWindow::on_delete_acc_action_triggered);
	assert(isConnectionSuccessfull);
	isConnectionSuccessfull = connect(ui.actionDownload, &QAction::triggered, this, &MainWindow::on_download_action_triggered);
	assert(isConnectionSuccessfull);
	isConnectionSuccessfull = connect(ui.users_filesTW, &QTreeWidget::customContextMenuRequested, this, &MainWindow::OnTreeViewContextMenuRequested);
	assert(isConnectionSuccessfull);

	ui.users_filesTW->setColumnCount(1);
	ui.users_filesTW->setSortingEnabled(true);
	ui.users_filesTW->setContextMenuPolicy(Qt::CustomContextMenu);
}

void MainWindow::send_directory_to_server()
{
	auto& SharedDirectory = connection_to_fsp_server_.SharedDir();
	SharedDirectory.SetDirectory();
	QString text("Your current shared directory is  ");
	text
		.append(SharedDirectory.path().string().data())
		.append("\n")
		.append("Would you like to change this directory?");
	QMessageBox DialogAskingForChangeSharedDir("Shared Directory", text, QMessageBox::Icon::Question, QMessageBox::Button::Yes, QMessageBox::Button::No, 0);
	connect(&DialogAskingForChangeSharedDir, &QMessageBox::finished, [this, SharedDirectory = &SharedDirectory ](int result) {
		if (result == QMessageBox::Button::Yes)
			SharedDirectory->change();
		});
	DialogAskingForChangeSharedDir.exec();
	
	std::set<std::string> file_list;
	SharedDirectory.loadFilesString(file_list);
	connection_to_fsp_server_.RequestManager().on_push_files(file_list);
	SharedDirectory.start_monitoring();
	this->setWindowTitle(windowTitle().append(" ").append(SharedDirectory.path().filename().string().c_str()));
}

void  MainWindow::OnDownload(const std::string_view LocalPath, const AbstractFileReceiver::DownloadStatus Status, const unsigned char Progress)
{
	QString DisplayText (LocalPath.data());
	switch (Status)
	{
	case AbstractFileReceiver::DownloadStatus::Downloading:
		break;
	case AbstractFileReceiver::DownloadStatus::Cancelled:
	{
		DisplayText += " ";
		DisplayText += "Cancelled";
	}
	break;
	case AbstractFileReceiver::DownloadStatus::NetworkError:
	{
		DisplayText += " ";
		DisplayText += "Network Error";
	}
	break;
	case AbstractFileReceiver::DownloadStatus::Done:
	{
		DisplayText += " ";
		DisplayText += "Done";
	}
	break;
	default:
		break;
	}
	foot_label_->setText(DisplayText);
	foot_progress_bar_->setValue(Progress);
}

void MainWindow::OnSharedDirectoryContentChanged(const SharedDirectory::Change ChangeNotification)
{
	switch (ChangeNotification.Event_)
	{
	case SharedDirectory::Change::EventType::ADDED:
	{
		connection_to_fsp_server_.RequestManager().on_file_added(ChangeNotification.Original_);
	}
	break;
	case SharedDirectory::Change::EventType::REMOVED:
	{
		connection_to_fsp_server_.RequestManager().on_file_removed(ChangeNotification.Original_);
	}
	break;
	case SharedDirectory::Change::EventType::RENAMED:
	{
		connection_to_fsp_server_.RequestManager().on_file_renamed(ChangeNotification.Original_, _STD filesystem::path(_STD move(ChangeNotification.Renamed_)).filename().string());
	}
	break;
	}
}

void MainWindow::on_request_failure(const _STD string_view FailureMessage)
{
	QMessageBox("Request Failure", FailureMessage.data(), QMessageBox::Icon::Warning, QMessageBox::Button::Close, 0, 0).exec();
}

void MainWindow::onInternallServerError()
{
	QMessageLogger().fatal("Internal Server Error");
}

void MainWindow::onInternetConnectionError(std::string_view description)
{
	QMessageBox("Internet Connection Error", description.data(), QMessageBox::Icon::Critical, QMessageBox::Button::Retry, 0, 0).exec();
}

void MainWindow::OnUserLoggedOutNotification(const std::string_view Username)
{
	const auto&& nodes = ui.users_filesTW->findItems(Username.data(), Qt::MatchFlags::enum_type::MatchExactly);
	for (const auto& node : nodes)
		delete node;
}

void MainWindow::OnUserRegisteredFilesNotification(const std::string_view Username, const std::shared_ptr<std::set<std::string>> Paths)
{
	TreeWidgetManager::make_tree(new QTreeWidgetItem(ui.users_filesTW), Username, *Paths);
}

void MainWindow::OnUserAddedNewFileNotification(const std::string_view Username, const std::string_view Path)
{
	const auto&& nodes = ui.users_filesTW->findItems(Username.data(), Qt::MatchFlags::enum_type::MatchExactly);
	for (const auto& node : nodes)
		TreeWidgetManager::add_path(node, Path);
}

void MainWindow::OnUserDeletedPathNotification(const std::string_view Username, const std::string_view Path)
{
	const auto&& nodes = ui.users_filesTW->findItems(Username.data(), Qt::MatchFlags::enum_type::MatchExactly);
	for (const auto node : nodes)
		TreeWidgetManager::remove_path(node, Path);
}

void MainWindow::OnUserRenamedPathNotification(const std::string_view Username, const std::string_view OldPathName, const std::string_view NewName)
{
	const auto&& nodes = ui.users_filesTW->findItems(Username.data(), Qt::MatchFlags::enum_type::MatchExactly);
	for (const auto node : nodes)
		TreeWidgetManager::rename_path(node,OldPathName,NewName);
}

void MainWindow::OnFilesOfOtherUsersNotification(const notification_handler::files_of_users_t UsersPaths)
{
	for (const auto&[Username, Paths] : *UsersPaths)
		TreeWidgetManager::make_tree(new QTreeWidgetItem(ui.users_filesTW), Username, Paths);
}

void MainWindow::on_logout_done()
{
	this->close();
}

void MainWindow::OnTreeViewContextMenuRequested(const QPoint & pos)
{
	QMenu menu(this);
	menu.addAction(ui.actionDownload);

	ui.actionDownload->setData(QVariant(pos));

	menu.exec(ui.users_filesTW->mapToGlobal(pos));
}

void MainWindow::on_download_action_triggered(bool)
{
	QTreeWidgetItem *clickedItem = ui.users_filesTW->itemAt(ui.actionDownload->data().toPoint());
	if (clickedItem != nullptr)
	{
		if (clickedItem->childCount() != 0)
		{
			QMessageBox("Invalid Selection", "Please select a file, not a directory", QMessageBox::Icon::Warning, QMessageBox::Button::Ok, 0, 0).exec();
			return;
		}
		else {
			_STD string Path, UserName;
			TreeWidgetManager::my_path(clickedItem,Path,UserName);
			if (!connection_to_fsp_server_.DownloadManager().IsFileDownloadingAlready(_STD string(UserName).append(Path)))
			{
				connection_to_fsp_server_.RequestManager().DownloadFileQuerry(
					{	UserName,
						Path, 
						connection_to_fsp_server_.DownloadManager().GetSslVersion(),
						connection_to_fsp_server_.DownloadManager().GetSupportedCompressions()
					}
				);
			}
			else
				QMessageBox("Download", "File is downloading already", QMessageBox::Icon::Warning, QMessageBox::Button::Ok, 0, 0).exec();
		}
	}
}

void MainWindow::on_logout_action_triggered(bool)
{
	emit logout();
}

void MainWindow::on_delete_acc_action_triggered(bool)
{
	emit delete_account();
}