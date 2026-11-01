// Differential tests of Player.cpp and Surface.cpp against the original code.
//
// Only the two text decoders can run in the child; everything else in
// Player.cpp needs MFC, the window system or the original's iostreams.
// Player.cpp is linked against the stubs below, which are never called.

#include "difftest.h"
#include "../../src/Player.h"
#include "../../src/external.h"

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

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
TSurfaceGL::TSurfaceGL() STUB
void TSurfaceGL::Unknown0C() STUB
void TSurfaceGL::Unknown10() STUB
void TSurfaceGL::FillRectangle(int, TRect, TRGBColor) STUB
void TSurfaceGL::Unknown18() STUB
void TSurfaceGL::Unknown1C() STUB
void TSurfaceGL::Unknown20() STUB
void TSurfaceGL::DrawFrame(int, TRect, int, TRGBColor) STUB
void TSurfaceGL::Unknown28() STUB
void TSurfaceGL::Unknown2C() STUB
void TSurfaceGL::Unknown30() STUB
void TSurfaceGL::Unknown34() STUB
void TSurfaceGL::Unknown38() STUB
void TSurfaceGL::Unknown3C() STUB
void TSurfaceGL::Unknown40() STUB
void TSurfaceGL::Unknown44() STUB
void TSurfaceGL::Unknown48() STUB
void TSurfaceGL::Unknown4C() STUB
void TSurfaceGL::Unknown54(int, int, int, int) STUB
void TSurfaceGL::Unknown58() STUB
void TSurfaceGL::Unknown5C() STUB
void TSurfaceGL::Unknown60() STUB
void TSurfaceGL::Unknown68() STUB
void TSurfaceGL::Unknown6C() STUB
void TSurfaceGL::Unknown70() STUB
void TSurfaceGL::Unknown74() STUB
void TSurfaceGL::Unknown78() STUB
void TSurfaceGL::Unknown7C() STUB
void TSurfaceGL::FinishFrame() STUB
bool TSurfaceGL::Init(HWND *, int, TFullscreenMode *, bool) STUB
void TSurfaceGL::Unknown8C() STUB
void TSurfaceGL::Unknown90() STUB
void TSurfaceGL::Unknown94() STUB
bool TSurfaceGL::Unknown9C(int) STUB
bool TSurfaceGL::IsUnavailable() STUB
void TSurfaceGL::UnknownA4() STUB
void TSurfaceGL::UnknownA8() STUB
void TSurfaceGL::UnknownAC() STUB
double TSurfaceGL::GetFPS() STUB
void TSurfaceGL::InitSurfaces() STUB
void TSurfaceGL::ExitSurfaces() STUB
TSurface *FUN_004eca00() STUB
TSurface *FUN_004f1430() STUB
bool FUN_004efd10() STUB
bool FUN_004f9710(HDC) STUB
const char *GetErrorText(int) STUB
const char *GetErrorHint(int) STUB

const char *DAT_005c249c;
void Redraw(bool) STUB
void MouseMove(int, int) STUB
void ProcessEvents() STUB
void ShutdownConnection() STUB
void SetTestServerAddress(const char *) STUB
void SetTestServerPort(int) STUB

typedef void (__cdecl *TextFn)(char *);

// Surface.cpp's getters of curEngine and AvailableEngines (0x71d0c4, 0x71d0c8).
static void TestSurface()
{
	for (int Engine = -2; Engine < 5; Engine++) {
		curEngine = Engine;
		Global<int>(0x71d0c4) = Engine;
		CHECK(GetCurrentEngine() == At<int (__cdecl *)()>(0x4eaa80)(), "GetCurrentEngine %d", Engine);
	}
	CHECK(GetAvailableEngines() == 0 && At<uint8 (__cdecl *)()>(0x4eabe0)() == 0, "GetAvailableEngines");
}

void TestPlayer()
{
	TestSurface();
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
