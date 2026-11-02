// Differential tests of Player.cpp and Surface.cpp against the original code.
//
// Only the two text decoders can run in the child; everything else in
// Player.cpp needs MFC, the window system or the original's iostreams.
// Player.cpp is linked against the stubs below, which are never called.

#include "difftest.h"
#include "../../src/Player.h"
#include "../../src/external.h"
#include "../../src/MainFrm.h"

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "shell32.lib")

// --- Link stubs for Player.cpp (MFC shim declarations and callees that are
// not part of the test build). None of them is called by the tests.

#define STUB { abort(); }
// For MainFrm.cpp.
CWinApp *AfxGetApp() STUB
int CWnd::OnCreate(LPCREATESTRUCT) STUB
BOOL CWnd::OnSetCursor(CWnd *, UINT, UINT) STUB
void CWnd::OnClose() STUB
void CWnd::OnDestroy() STUB
void CWnd::OnSize(UINT, int, int) STUB
CFrameWnd::CFrameWnd() { m_hWnd = NULL; }
CFrameWnd::~CFrameWnd() {}
BOOL CWnd::GetWindowPlacement(WINDOWPLACEMENT *) const STUB
BOOL CWnd::SetWindowPlacement(const WINDOWPLACEMENT *) STUB
BOOL CWnd::ModifyStyle(DWORD, DWORD, UINT) STUB
BOOL CWnd::ModifyStyleEx(DWORD, DWORD, UINT) STUB
BOOL CWnd::SetWindowPos(const CWnd *, int, int, int, int, UINT) STUB
LRESULT CWnd::Default() STUB
CWnd *PASCAL CWnd::FromHandle(HWND) STUB
const CWnd CWnd::wndTopMost = CWnd();
const CWnd CWnd::wndNoTopMost = CWnd();
BOOL CWnd::DestroyWindow() STUB
BOOL CWnd::PreCreateWindow(CREATESTRUCT &) STUB
void CWnd::MoveWindow(int, int, int, int, BOOL) STUB
bool CloseRequest() STUB
const char *GetResolvedIPAddress() STUB
void ConnectToLoginServer() STUB
void Disconnect(bool) STUB
void ExitControl() STUB
void GetErrorMessage(char *, int, int) STUB
void InitControl(bool) STUB
void InitWindow(HWND, void (*)(), void (*)(), void (*)(int), void (*)(const char *)) STUB
void KeyPress(int) STUB
void LeftButtonDoubleClick(int, int, int) STUB
void LeftButtonDown(int, int, int) STUB
void LeftButtonUp(int, int, int) STUB
void MouseWheel(int, int, int) STUB
void Resize(int, int) STUB
void RightButtonDown(int, int, int) STUB
void RightButtonUp(int, int, int) STUB
void SetConnectionTimeout(int) STUB
void SetServerError(int) STUB
void SetServerIPAddress(const char *) STUB
void TimerEvent() STUB
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





void OutputControlErrorInformations(std::ostream &) STUB
int DAT_005c2564;
// The constructor and the slots graphics.cpp calls are graphicstest.cpp's recorders, those
// MapWindow.cpp calls mapwindowtest.cpp's.
void TSurfaceGL::SetPixel(int, int, int, TRGBColor) STUB
void TSurfaceGL::FillRectangleClipped(int, TRect, TRGBColor, TRect) STUB
void TSurfaceGL::DrawHorizontalLine(int, int, int, int, TRGBColor) STUB
void TSurfaceGL::DrawVerticalLine(int, int, int, int, TRGBColor) STUB
void TSurfaceGL::DrawFrame(int, TRect, int, TRGBColor) STUB
void TSurfaceGL::BlitRectClipped(int, int, int, int, int, int, int, int, TRect) STUB
void TSurfaceGL::StretchBlitUnclipped(int, int, int, int, int, int, int, int, int, int, TRGBColor, TRGBColor) STUB
bool TSurfaceGL::ClipRect(TRect, RECT *, RECT *) STUB
void TSurfaceGL::ResetFrameTimer() STUB
void TSurfaceGL::WaitForNextFrame() STUB
void TSurfaceGL::UpdateWindowRect() STUB
void TSurfaceGL::InitDisplay(int, TFullscreenMode *) STUB
void TSurfaceGL::ExitDisplay() STUB
void TSurfaceGL::CreateSurfaces() STUB
void TSurfaceGL::ResetDisplay(int, TFullscreenMode *) STUB
void TSurfaceGL::RestoreSurfaces(int, TFullscreenMode *) STUB
void TSurfaceGL::FinishFrame() STUB
bool TSurfaceGL::Init(HWND *, int, TFullscreenMode *, bool) STUB
int TSurfaceGL::GetFullscreenModes(TFullscreenMode **) STUB
bool TSurfaceGL::IsUnavailable() STUB
int TSurfaceGL::GetSurfaceMemory(int) STUB
void TSurfaceGL::SetFramerate(int) STUB
double TSurfaceGL::GetFPS() STUB
void TSurfaceGL::InitSurfaces() STUB
void TSurfaceGL::ExitSurfaces() STUB
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

// MainFrm.cpp's TranslateKey: every virtual key, both extended flags and
// every modifier state (KeyFlags at +0x200 in the original frame).
static void TestTranslateKey()
{
	alignas(8) static uint8 Ours[sizeof(CMainFrame)];
	static uint8 Original[0x204];
	CMainFrame *Frame = (CMainFrame *)Ours;
	typedef int (__fastcall *KeyFn)(void *, void *, UINT, UINT, UINT);
	for (int Flags = 0; Flags < 8; Flags++) {
		int KeyFlags = ((Flags & 1) ? KEY_SHIFT : 0) | ((Flags & 2) ? KEY_CONTROL : 0) | ((Flags & 4) ? KEY_ALT : 0);
		Frame->KeyFlags = KeyFlags;
		*(int *)(Original + 0x200) = KeyFlags;
		for (UINT nChar = 0; nChar < 0x120; nChar++) {
			for (UINT nFlags = 0; nFlags < 0x400; nFlags += 0x80) {
				int A = Frame->TranslateKey(nChar, 1, nFlags);
				int B = At<KeyFn>(0x4a8210)(Original, NULL, nChar, 1, nFlags);
				CHECK(A == B, "TranslateKey(%u, %u) with %x: %x, original %x", nChar, nFlags, KeyFlags, A, B);
			}
		}
	}
}

void TestPlayer()
{
	TestSurface();
	TestTranslateKey();
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
