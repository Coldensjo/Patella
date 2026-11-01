// Minimal stand-in for the MFC declarations the reconstruction uses, for
// compile checks on machines without MFC (tools\compile-check.cmd defines
// TIBIA_MFC_SHIM when afxwin.h is missing). Declarations only: it is not
// linkable and does not reproduce MFC's layout beyond m_hSocket.

#pragma once

#include <winsock2.h>
#include <windows.h>

class CObject {
public:
	virtual ~CObject() {}
};

class CAsyncSocket : public CObject {
public:
	CAsyncSocket();
	virtual ~CAsyncSocket();
	BOOL Create(UINT nSocketPort = 0, int nSocketType = SOCK_STREAM,
		long lEvent = FD_READ | FD_WRITE | FD_OOB | FD_ACCEPT | FD_CONNECT | FD_CLOSE,
		LPCTSTR lpszSocketAddress = NULL);
	BOOL Connect(LPCTSTR lpszHostAddress, UINT nHostPort);
	virtual void Close();
	virtual int Receive(void *lpBuf, int nBufLen, int nFlags = 0);
	virtual int Send(const void *lpBuf, int nBufLen, int nFlags = 0);
	virtual void OnReceive(int nErrorCode);
	virtual void OnSend(int nErrorCode);
	virtual void OnOutOfBandData(int nErrorCode);
	virtual void OnAccept(int nErrorCode);
	virtual void OnConnect(int nErrorCode);
	virtual void OnClose(int nErrorCode);

	SOCKET m_hSocket;
};

BOOL AfxSocketInit(WSADATA *lpwsaData = NULL);

// --- added for Player.cpp: the application and frame window classes ---

class CCmdTarget : public CObject {
};

class CWnd : public CCmdTarget {
public:
	BOOL ShowWindow(int nCmdShow);
	void UpdateWindow() { ::UpdateWindow(m_hWnd); }
	HWND GetSafeHwnd() const { return this == NULL ? NULL : m_hWnd; }

	HWND m_hWnd;
};

struct CCreateContext;

class CFrameWnd : public CWnd {
public:
	static const RECT rectDefault;
	virtual BOOL Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle = WS_OVERLAPPEDWINDOW,
		const RECT &rect = rectDefault, CWnd *pParentWnd = NULL, LPCTSTR lpszMenuName = NULL,
		DWORD dwExStyle = 0, CCreateContext *pContext = NULL);
};

class CWinThread : public CCmdTarget {
public:
	virtual BOOL InitInstance();
	virtual int Run();
	virtual BOOL OnIdle(LONG lCount);

	CWnd *m_pMainWnd;
};

class CWinApp : public CWinThread {
public:
	CWinApp(LPCTSTR lpszAppName = NULL);

	LPTSTR m_lpCmdLine;
};

CWnd *AfxGetMainWnd();
BOOL AfxRegisterClass(WNDCLASS *lpWndClass);
HINSTANCE AfxGetInstanceHandle();
HINSTANCE AfxGetResourceHandle();
WNDPROC AfxGetAfxWndProc();

#define DECLARE_MESSAGE_MAP() \
protected: \
	static const void *GetThisMessageMap(); \
	virtual const void *GetMessageMap() const;
#define BEGIN_MESSAGE_MAP(theClass, baseClass) \
	const void *theClass::GetMessageMap() const { return GetThisMessageMap(); } \
	const void *theClass::GetThisMessageMap() { static const int Entries[] = {
#define END_MESSAGE_MAP() 0 }; return Entries; }
