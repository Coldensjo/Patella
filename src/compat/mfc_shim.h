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
