#pragma once

#include <map>

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // some CString constructors will be explicit
#include <atlstr.h>

#include "ThreadSafeQueue.h"

typedef _STD pair<DWORD, CStringW> TDirectoryChangeNotification;

namespace ReadDirectoryChangesPrivate
{
	class CReadChangesServer;
}

///////////////////////////////////////////////////////////////////////////


/// <summary>
/// Track changes to filesystem directories and report them
/// to the caller via a thread-safe queue.
/// </summary>
/// <remarks>
/// <para>
/// All functions in CReadDirectoryChangesServer run in
/// the context of the calling thread.
/// </para>
/// <example><code>
/// 	CReadDirectoryChanges changes;
/// 	changes.AddDirectory(_T("C:\\"), false, dwNotificationFlags);
///
///		const HANDLE handles[] = { hStopEvent, changes.GetWaitHandle() };
///
///		while (!bTerminate)
///		{
///			::MsgWaitForMultipleObjectsEx(
///				_countof(handles),
///				handles,
///				INFINITE,
///				QS_ALLINPUT,
///				MWMO_INPUTAVAILABLE | MWMO_ALERTABLE);
///			switch (rc)
///			{
///			case WAIT_OBJECT_0 + 0:
///				bTerminate = true;
///				break;
///			case WAIT_OBJECT_0 + 1:
///				// We've received a notification in the queue.
///				{
///					DWORD dwAction;
///					CStringW wstrFilename;
///					changes.Pop(dwAction, wstrFilename);
///					wprintf(L"%s %s\n", ExplainAction(dwAction), wstrFilename);
///				}
///				break;
///			case WAIT_OBJECT_0 + _countof(handles):
///				// Get and dispatch message
///				break;
///			case WAIT_IO_COMPLETION:
///				// APC complete.No action needed.
///				break;
///			}
///		}
/// </code></example>
/// </remarks>
class CReadDirectoryChanges final
{
public:
	CReadDirectoryChanges(int nMaxChanges = 1000);
	~CReadDirectoryChanges();

	void Init();
	void Terminate();

	/// <summary>
	/// Add a new directory to be monitored.
	/// </summary>
	/// <param name="wszDirectory">Directory to monitor.</param>
	/// <param name="bWatchSubtree">True to also monitor subdirectories.</param>
	/// <param name="dwNotifyFilter">The types of file system events to monitor, such as FILE_NOTIFY_CHANGE_ATTRIBUTES.</param>
	/// <param name="dwBufferSize">The size of the buffer used for overlapped I/O.</param>
	/// <remarks>
	/// <para>
	/// This function will make an APC call to the worker thread to issue a new
	/// ReadDirectoryChangesW call for the given directory with the given flags.
	/// </para>
	/// </remarks>
	void AddDirectory(LPCTSTR wszDirectory, BOOL bWatchSubtree, DWORD dwNotifyFilter, DWORD dwBufferSize = 16384);

	/// <summary>
	/// Return a handle for the Win32 Wait... functions that will be
	/// signaled when there is a queue entry.
	/// </summary>
	HANDLE GetWaitHandle() { return m_Notifications.GetWaitHandle(); }

	bool Pop(DWORD& dwAction, CStringW& wstrFilename);

	// "Push" is for usage by ReadChangesRequest.  Not intended for external usage.
	void Push(DWORD dwAction, CStringW& wstrFilename);

	// Check if the queue overflowed. If so, clear it and return true.
	bool CheckOverflow();

	unsigned int GetThreadId() { return m_dwThreadId; }

protected:
	ReadDirectoryChangesPrivate::CReadChangesServer* m_pServer;

	HANDLE m_hThread;

	unsigned int m_dwThreadId;

	CThreadSafeQueue<TDirectoryChangeNotification> m_Notifications;
};


