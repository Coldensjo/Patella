// Differential tests of Player.cpp against the original code.
//
// Only the two text decoders can run in the child; everything else in
// Player.cpp needs MFC, the window system or the original's iostreams.
// Player.cpp is linked against the stubs below, which are never called.

#include "difftest.h"
#include "../../src/Player.h"
#include "../../src/external.h"

#pragma comment(lib, "user32.lib")

// --- Link stubs for Player.cpp (MFC shim declarations and callees that are
// not part of the test build). None of them is called by the tests.

#define STUB { abort(); }
BOOL CWnd::ShowWindow(int) STUB
const RECT CFrameWnd::rectDefault = {0, 0, 0, 0};
BOOL CFrameWnd::Create(LPCTSTR, LPCTSTR, DWORD, const RECT &, CWnd *, LPCTSTR, DWORD, CCreateContext *) STUB
BOOL CWinThread::InitInstance() STUB
int CWinThread::Run() STUB
BOOL CWinThread::OnIdle(LONG) STUB
CWinApp::CWinApp(LPCTSTR) { m_pMainWnd = NULL; m_lpCmdLine = NULL; }
CWnd *AfxGetMainWnd() STUB
BOOL AfxRegisterClass(WNDCLASS *) STUB
HINSTANCE AfxGetInstanceHandle() STUB
HINSTANCE AfxGetResourceHandle() STUB
WNDPROC AfxGetAfxWndProc() STUB
CMainFrame::CMainFrame(bool) STUB
void CMainFrame::OutputErrorInformations(std::ostream &) STUB
void FUN_004a88f0() STUB
void FUN_004a8af0(int) STUB
void FUN_004aa1d0() STUB
void OutputControlErrorInformations(std::ostream &) STUB
void FUN_0048da50() STUB
int DAT_005c2564;
int FUN_004eaa80() STUB
void FUN_004eac20(HWND *) STUB
TSurface *FUN_004eaa90() STUB
const char *FUN_004fcbf0(int) STUB
const char *FUN_004fcc90(int) STUB
const char *DAT_005c249c;
void Redraw(bool) STUB
void MouseMove(int, int) STUB
void ProcessEvents() STUB
void ShutdownConnection() STUB
void SetTestServerAddress(const char *) STUB
void SetTestServerPort(int) STUB

typedef void (__cdecl *TextFn)(char *);

void TestPlayer()
{
	struct { uint32 Original; void (*Ours)(char *); const char *Expected; } Decoders[] = {
		{0x4d9140, GetWindowTitle, "Tibia   "},
		{0x4d92b0, GetWindowClassName, "TibiaClient"},
	};
	for (auto &D : Decoders) {
		char A[64], B[64];
		memset(A, 0x55, sizeof(A));
		memset(B, 0x55, sizeof(B));
		D.Ours(A);
		At<TextFn>(D.Original)(B);
		CHECK(memcmp(A, B, sizeof(A)) == 0 && strcmp(A, D.Expected) == 0, "decoded text '%s', original '%s'", A, B);
	}
}
