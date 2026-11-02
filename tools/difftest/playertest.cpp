// Differential tests of Player.cpp and Surface.cpp against the original code.
//
// Only the two text decoders can run in the child; everything else in
// Player.cpp needs MFC, the window system or the original's iostreams.
// Player.cpp is linked against the stubs below, which are never called.

#include "difftest.h"
#include "../../src/Player.h"
#include "../../src/MainFrm.h"
#include "../../src/Surface.h"
#include "../../src/sysinc.h" // TitleCode, ClassNameCode (defined below for difftest.exe)

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "shell32.lib")

// --- Link stubs for Player.cpp (MFC shim declarations and callees that are
// not part of the test build). None of them is called by the tests.

// Each stub has a body of its own (the function's name): the linker folds
// identical functions, and mainfrmtest.cpp redirects the Control.cpp stubs
// one by one. The MFC members MainFrm.cpp calls are mainfrmtest.cpp's fakes,
// Network.cpp is compiled into networktest.cpp (issue #37).
[[noreturn]] static void StubCalled(const char *Name)
{
	printf("stub %s called\n", Name);
	abort();
}
#define STUB { StubCalled(__FUNCTION__); }
// For MainFrm.cpp.
bool CloseRequest() STUB
void ExitControl() STUB
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
HINSTANCE AfxGetResourceHandle() STUB
WNDPROC AfxGetAfxWndProc() STUB





void OutputControlErrorInformations(std::ostream &) STUB
// Not folded into one function: errortest.cpp redirects each to its own fake.
const char *GetErrorText(int) { printf("GetErrorText stub\n"); abort(); }
const char *GetErrorHint(int) { printf("GetErrorHint stub\n"); abort(); }

// sysinc.cpp's halves of the encoded window title and class name (sysinc.cpp is not
// linked into difftest.exe).
const int TitleCode[8] = {68, 75, 129, 81, 98, 78, 96, 141};
const int ClassNameCode[11] = {57, 193, 225, 58, 140, 118, 195, 214, 239, 244, 137};
void Redraw(bool) STUB
void MouseMove(int, int) STUB
void ProcessEvents() STUB

typedef void (__cdecl *TextFn)(char *);

// Surface.cpp's getters of curEngine and AvailableEngines (0x71d0c4, 0x71d0c8).
static void TestSurface()
{
	for (int Engine = -2; Engine < 5; Engine++) {
		curEngine = Engine;
		Global<int>(0x71d0c4) = Engine;
		CHECK(GetCurrentEngine() == At<int (__cdecl *)()>(0x4eaa80)(), "GetCurrentEngine %d", Engine);
	}
	// A member that does not read this (the binary passes the surface in ECX).
	TSurface *Surface = NULL;
	CHECK(Surface->GetAvailableEngines() == 0 && At<uint8 (__cdecl *)()>(0x4eabe0)() == 0, "GetAvailableEngines");
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
