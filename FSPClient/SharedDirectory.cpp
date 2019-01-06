#include "SharedDirectory.h"
#include "fs_utils.h"
#include "KeysForConfigSettings.h"
#include "KeysForQSettings.h"

#include <QtCore/qsettings.h>
#include <QtWidgets/qfiledialog.h>
#include <QtWidgets/qmessagebox.h>


bool DirectoryContainsFiles(_STD filesystem::path const& DirPath)
{
	using namespace _STD filesystem;
	bool contains = false;
	recursive_directory_iterator iter(DirPath);

	for(const auto& path : iter)
		if (!is_directory(path))
		{
			contains = true;
			break;
		}

	return contains;
}

Q_DECLARE_METATYPE(SharedDirectory::Change);

SharedDirectory::SharedDirectory() : 
	directory_()
{
	this->renamed_buffer_.Event_ = Change::EventType::RENAMED;
	qRegisterMetaType<SharedDirectory::Change>();
}

SharedDirectory::~SharedDirectory()
{
	QSettings(persistency::qsettings::APP_NAME_V, persistency::qsettings::ORG_NAME_V)
		.setValue(persistency::qsettings::SH_DIR_NAME_K, QString(directory_.path().string().data()));
}


_NODISCARD bool SharedDirectory::try_make_absolute(const std::filesystem::path& relative, std::filesystem::path& absolute) const
{
	auto p = directory_.path() / relative;
	if (std::filesystem::exists(p) && p.is_absolute())
	{
		absolute = p;
		return true;
	}
	return false;
}

void SharedDirectory::change()
{
	this->directory_.assign(RetrieveDirectoryNameFromQFileDialog());
}

void SharedDirectory::start_monitoring()
{
	this->watcher_.monitor(this->directory_.path().string(), true, [this](const auto& File, const auto Event) {
		using namespace _STD filesystem;
		_STD filesystem::path path(File);

		auto&& PathStr = relative(path, this->directory_.path()).string();
#ifndef LINUX
		_STD replace(PathStr.begin(), PathStr.end(), '\\', fsp::protocol::PATH_SEPARATOR_CHR);
#endif // !LINUX

		switch (Event)
		{
		case DirectoryWatcher::event::ADDED:
		{
			//assert(exists(path));
			if (!is_directory(path))
			{
				emit this->Changed(Change{ PathStr , "", Change::EventType::ADDED , Change::FileType::FILE });
			}	
		}
		break;
		case DirectoryWatcher::event::REMOVED:
		{
			emit this->Changed(Change{ PathStr,"",Change::EventType::REMOVED, Change::FileType::UNKNOWN });
		}
		break;
		case DirectoryWatcher::event::RENAMED_OLD_NAME:
		{
			this->renamed_buffer_.Original_ = _STD move(PathStr);
		}
		break;
		case DirectoryWatcher::event::RENAMED_NEW_NAME:
		{
			this->renamed_buffer_.Renamed_ = _STD move(PathStr);
			//assert(exists(path));
			if (!is_directory(path))
			{
				this->renamed_buffer_.File_ = Change::FileType::FILE;
				emit this->Changed(_STD move(this->renamed_buffer_));
			}
			else if (DirectoryContainsFiles(path))
			{
				this->renamed_buffer_.File_ = Change::FileType::DIRECTORY;
				emit this->Changed(_STD move(this->renamed_buffer_));
			}
		}
		break;
		}
	});
}

void SharedDirectory::SetDirectory()
{
	_STD string DirectoryName;
	if (auto dir_name = QSettings(persistency::qsettings::APP_NAME_V, persistency::qsettings::ORG_NAME_V)
		.value(persistency::qsettings::SH_DIR_NAME_K); !dir_name.isNull())
	{
		DirectoryName = dir_name.toString().toStdString();
		std::filesystem::path p(DirectoryName);
		if (std::filesystem::exists(p) && std::filesystem::is_directory(p))
			goto set_dir;
		else
			goto get_dir;
	}
	else {
	get_dir:
		DirectoryName = RetrieveDirectoryNameFromQFileDialog();
	}
set_dir:
	this->directory_.assign(DirectoryName);
}

_NODISCARD const _STD string SharedDirectory::RetrieveDirectoryNameFromQFileDialog()
{
	_STD string DirectoryName;
	QFileDialog	fileDialog;
	fileDialog.setWindowTitle("Select/Create folder which you want to share");
	fileDialog.setFileMode(QFileDialog::FileMode::DirectoryOnly);
	fileDialog.setViewMode(QFileDialog::ViewMode::Detail);
	fileDialog.setAcceptMode(QFileDialog::AcceptMode::AcceptOpen);
	fileDialog.setOption(QFileDialog::Option::ShowDirsOnly);
	while (true)
	{
		if (fileDialog.exec())
		{
			if (auto directories = fileDialog.selectedFiles(); !directories.empty())
			{
				for (auto& directory_selected : directories) {
					DirectoryName = directory_selected.toStdString();
					if (IsCurrentUserOwner(DirectoryName))
						return DirectoryName;
					else
						QMessageBox("Invalid selection", "Please select directory which you own!", QMessageBox::Icon::Critical, QMessageBox::Button::Ok, 0, 0).exec();
				}
			}
			else
			{
				QMessageBox("Invalid selection", "Please select valid directory", QMessageBox::Icon::Warning, QMessageBox::Button::Ok, 0, 0).exec();
			}
		}
		else
		{
			QMessageBox("Invalid selection", "Please select valid directory", QMessageBox::Icon::Warning, QMessageBox::Button::Ok, 0, 0).exec();
		}
	}
	throw _STD exception("No Shared Folder selected");
}
