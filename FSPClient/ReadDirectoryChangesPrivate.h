#pragma once

#define _CRT_SECURE_NO_DEPRECATE

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN            // Exclude rarely-used stuff from Windows headers
#endif
#include <windows.h>

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // some CString constructors will be explicit

#include <atlstr.h>
#include <vector>

class CReadDirectoryChanges;

namespace ReadDirectoryChangesPrivate
{

	class CReadChangesServer;

	///////////////////////////////////////////////////////////////////////////

	// All functions in CReadChangesRequest run in the context of the worker thread.
	// One instance of this object is created for each call to AddDirectory().
	class CReadChangesRequest final
	{
	public:
		CReadChangesRequest(CReadChangesServer* pServer, LPCTSTR sz, BOOL b, DWORD dw, DWORD size);

		~CReadChangesRequest();

		bool OpenDirectory();

		void BeginRead();

		// The dwSize is the actual number of bytes sent to the APC.
		void BackupBuffer(DWORD dwSize)
		{
			// We could just swap back and forth between the two
			// buffers, but this code is easier to understand and debug.
			memcpy(&m_BackupBuffer[0], &m_Buffer[0], dwSize);
		}

		void ProcessNotification();

		void RequestTermination()
		{
			::CancelIo(m_hDirectory);
			::CloseHandle(m_hDirectory);
			m_hDirectory = nullptr;
		}

		CReadChangesServer* m_pServer;

	protected:

		static VOID CALLBACK NotificationCompletion(
			DWORD dwErrorCode,							// completion code
			DWORD dwNumberOfBytesTransfered,			// number of bytes transferred
			LPOVERLAPPED lpOverlapped);					// I/O information buffer

	// Parameters from the caller for ReadDirectoryChangesW().
		DWORD		m_dwFilterFlags;
		BOOL		m_bIncludeChildren;
		CStringW	m_wstrDirectory;

		// Result of calling CreateFile().
		HANDLE		m_hDirectory;

		// Required parameter for ReadDirectoryChangesW().
		OVERLAPPED	m_Overlapped;

		// Data buffer for the request.
		// Since the memory is allocated by malloc, it will always
		// be aligned as required by ReadDirectoryChangesW().
		_STD vector<BYTE> m_Buffer;

		// Double buffer strategy so that we can issue a new read
		// request before we process the current buffer.
		_STD vector<BYTE> m_BackupBuffer;
	};

	///////////////////////////////////////////////////////////////////////////

	// All functions in CReadChangesServer run in the context of the worker thread.
	// One instance of this object is allocated for each instance of CReadDirectoryChanges.
	// This class is responsible for thread startup, orderly thread shutdown, and shimming
	// the various C++ member functions with C-style Win32 functions.
	class CReadChangesServer final
	{
	public:
		CReadChangesServer(CReadDirectoryChanges* pParent)
		{
			m_bTerminate = false; m_nOutstandingRequests = 0; m_pBase = pParent;
		}

		static unsigned int WINAPI ThreadStartProc(LPVOID arg)
		{
			CReadChangesServer* pServer = (CReadChangesServer*)arg;
			pServer->Run();
			return 0;
		}

		// Called by QueueUserAPC to start orderly shutdown.
		static void CALLBACK TerminateProc(__in  ULONG_PTR arg)
		{
			CReadChangesServer* pServer = (CReadChangesServer*)arg;
			pServer->RequestTermination();
		}

		// Called by QueueUserAPC to add another directory.
		static void CALLBACK AddDirectoryProc(__in  ULONG_PTR arg)
		{
			CReadChangesRequest* pRequest = (CReadChangesRequest*)arg;
			pRequest->m_pServer->AddDirectory(pRequest);
		}

		CReadDirectoryChanges* m_pBase;

		volatile DWORD m_nOutstandingRequests;

	protected:

		void Run()
		{
			while (m_nOutstandingRequests || !m_bTerminate)
			{
				DWORD rc = ::SleepEx(INFINITE, true);
			}
		}

		void AddDirectory(CReadChangesRequest* pBlock)
		{
			if (pBlock->OpenDirectory())
			{
				::InterlockedIncrement(&pBlock->m_pServer->m_nOutstandingRequests);
				m_pBlocks.push_back(pBlock);
				pBlock->BeginRead();
			}
			else
				delete pBlock;
		}

		void RequestTermination()
		{
			m_bTerminate = true;

			for (DWORD i = 0; i < m_pBlocks.size(); ++i)
			{
				// Each Request object will delete itself.
				m_pBlocks[i]->RequestTermination();
			}

			m_pBlocks.clear();
		}

		_STD vector<CReadChangesRequest*> m_pBlocks;

		bool m_bTerminate;
	};

}

