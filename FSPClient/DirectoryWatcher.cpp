#include "DirectoryWatcher.h"
#include <filesystem>
#include <strsafe.h>

void ErrorExit(LPTSTR lpszFunction)
{
	// Retrieve the system error message for the last-error code

	LPVOID lpMsgBuf;
	LPVOID lpDisplayBuf;
	DWORD dw = GetLastError();

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dw,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0, NULL);

	// Display the error message and exit the process

	lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
		(lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR));
	StringCchPrintf((LPTSTR)lpDisplayBuf,
		LocalSize(lpDisplayBuf) / sizeof(TCHAR),
		TEXT("%s failed with error %d: %s"),
		lpszFunction, dw, lpMsgBuf);
	MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);

	LocalFree(lpMsgBuf);
	LocalFree(lpDisplayBuf);
	ExitProcess(dw);
}

#define MONITOR_HANDLE_POSITION		0
#define STOP_EVENT_HANDLE_POSITION	1

DirectoryWatcher::event WIN32ACTION_TO_DIRECTORY_EVENT(DWORD action)
{
	switch (action)
	{
	case FILE_ACTION_ADDED:
		return DirectoryWatcher::event::ADDED;
	case FILE_ACTION_REMOVED:
		return DirectoryWatcher::event::REMOVED;
	case FILE_ACTION_RENAMED_OLD_NAME:
		return DirectoryWatcher::event::RENAMED_OLD_NAME;
	case FILE_ACTION_RENAMED_NEW_NAME:
		return DirectoryWatcher::event::RENAMED_NEW_NAME;
	default:
		return DirectoryWatcher::event::UNKNOWN;
	}
}

DirectoryWatcher::DirectoryWatcher() : stopped_(false)
{};

DirectoryWatcher::~DirectoryWatcher()
{
	this->stop();
	if (this->monitoring_thread_.joinable())
		this->monitoring_thread_.join();
	CloseHandle(this->Win32Handles_[STOP_EVENT_HANDLE_POSITION]);
}

void DirectoryWatcher::monitor(const _STD string& dir, bool wSubT, _STD function<void(const _STD string&, const event)> callback)
{
	reader_.AddDirectory(ATL::CA2W(dir.c_str()), wSubT, FILE_NOTIFY_CHANGE_FILE_NAME |
		FILE_NOTIFY_CHANGE_DIR_NAME);

	HANDLE hStopWatchingEvent = CreateEvent(
		NULL,               // default security attributes
		TRUE,               // manual-reset event
		FALSE,              // initial state is nonsignaled
		TEXT("StopDirectoryWatcherEvent")  // object name
	);

	if (hStopWatchingEvent == NULL)
		ErrorExit(const_cast<wchar_t*>(_T("DirectoryWatcher::monitor() -> CreateEvent()")));

	Win32Handles_[MONITOR_HANDLE_POSITION]		= reader_.GetWaitHandle();
	Win32Handles_[STOP_EVENT_HANDLE_POSITION]	= hStopWatchingEvent;

	this->callback_ = _STD move(callback);
	this->monitoring_thread_ = _STD thread([this]() {
		while (!this->stopped_)
		{
			switch (::WaitForMultipleObjectsEx(_countof(this->Win32Handles_), this->Win32Handles_, false, INFINITE, true))
			{
			case WAIT_OBJECT_0 + 0:
				// We've received a notification in the queue.
			{
				DWORD dwAction;
				CStringW wstrFilename;
				if (!this->reader_.CheckOverflow())
				{
					this->reader_.Pop(dwAction, wstrFilename);
					this->callback_((_STD string)ATL::CW2A(wstrFilename), WIN32ACTION_TO_DIRECTORY_EVENT(dwAction));
				}
			}
			break;
			case WAIT_OBJECT_0 + 1:
				// hStop was signaled. Stop loop
				this->stopped_ = true;
				break;
			case WAIT_IO_COMPLETION:
				// Nothing to do.
				break;
			}
		}
	});
}

void DirectoryWatcher::stop() noexcept
{
	if (!this->stopped_)
	{
		if (!SetEvent(this->Win32Handles_[STOP_EVENT_HANDLE_POSITION]))
		{
			ErrorExit(const_cast<wchar_t*>(TEXT("DirectoryWatcher::stop() -> SetEvent()")));
		}
	}
}
