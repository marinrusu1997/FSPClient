#pragma once
#include <filesystem>
#include <string>
#include <algorithm>
#include <QtCore/qobject.h>
#include "DirectoryWatcher.h"
#include "protocol.h"

class SharedDirectory : public QObject
{
	Q_OBJECT

public:
	struct Change
	{
		enum class EventType : int8_t {ADDED, REMOVED, RENAMED};
		enum class FileType  : int8_t {FILE, DIRECTORY, UNKNOWN};

		_STD string	Original_;
		_STD string	Renamed_;
		EventType	Event_;
		FileType	File_;

		Change() = default;
		Change(_STD string const& Original, _STD string const& Renamed,EventType event, FileType file)
			:	Original_(_STD move(Original)),
				Renamed_(_STD move(Renamed)),
				Event_(event),
				File_(file)
		{}
	};
signals:
	void Changed(const SharedDirectory::Change);
public:
	SharedDirectory();
	SharedDirectory(SharedDirectory const&) = delete;
	SharedDirectory& operator=(SharedDirectory const&) = delete;

	~SharedDirectory();

	template<class StringSequenceContainer>
	auto									loadFilesString(StringSequenceContainer& container)
	{
		using namespace std::filesystem;
		recursive_directory_iterator iter(directory_.path());
		for (const auto & path : iter)
			if (!is_directory(path))
			{
				auto&& PathStr = relative(path, directory_.path()).string();
#ifndef LINUX
				_STD replace(PathStr.begin(), PathStr.end(), '\\', fsp::protocol::PATH_SEPARATOR_CHR);
#endif // !LINUX
				container.emplace(PathStr);
			}
	}

	void									SetDirectory();

	auto&									path() const noexcept { return directory_.path(); }

	_NODISCARD bool							try_make_absolute(const std::filesystem::path& relative, std::filesystem::path& absolute) const;

	void									change();

	void									start_monitoring();
private:
	
	_NODISCARD const _STD string RetrieveDirectoryNameFromQFileDialog();

	_STD filesystem::directory_entry	directory_;
	DirectoryWatcher					watcher_;
	Change								renamed_buffer_;
};

