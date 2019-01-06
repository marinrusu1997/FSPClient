#pragma once

#include "ReadDirectoryChanges.h"
#include <thread>
#include <functional>
#include <string>

class DirectoryWatcher final
{
public:
	enum class event : int8_t
	{
		ADDED,
		REMOVED,
		RENAMED_OLD_NAME,
		RENAMED_NEW_NAME,
		UNKNOWN
	};

	DirectoryWatcher();
	DirectoryWatcher(DirectoryWatcher const&) = delete;
	DirectoryWatcher& operator=(DirectoryWatcher const&) = delete;
	~DirectoryWatcher();

	void monitor(const _STD string& dir, bool wSubT, _STD function<void(const _STD string&, const event)> callback);

	void stop() noexcept;
private:

	CReadDirectoryChanges										reader_;
	HANDLE														Win32Handles_[2];
	_STD thread													monitoring_thread_;
	bool														stopped_;
	_STD function<void(const _STD string&, const event)>		callback_;
};
