// Surface.cpp: reconstructed from Tibia.exe 7.72 (0x4eaa80-0x4eae6f).
//
// The first part of the Status/SurfaceDX gap: the current graphics engine,
// the surface object of that engine, the engine selection at start-up, and
// the base class TSurface. The file name is inferred (see Surface.h). No
// function has line evidence, so they are in binary order. The DirectDraw
// surface code after 0x4eae70 is SurfaceDX.cpp's.

#include "Surface.h"
#include "Preferences.h"
#include "external.h"

// Data, in .bss order.
static HWND SurfaceWindow;          // 0x71d0c0, the main window, from SurfaceInitInstance
int curEngine;                      // 0x71d0c4
static uint8 AvailableEngines;      // 0x71d0c8, bit 0 OpenGL works, bit 1 DirectX 9 passed TestEngine
static TSurface *Surface;           // 0x71d0cc

// 0x4eaa80. Written to error.txt as "Graphic Engine".
int GetCurrentEngine()
{
	return curEngine;
}

// 0x4eaa90. Creates the surface of the current engine on first use.
TSurface *GetSurface()
{
	if (curEngine == 1) {
		if (Surface == NULL)
			Surface = new TSurfaceGL;
	} else if (curEngine == 2) {
		if (Surface == NULL)
			Surface = FUN_004f1430();
	} else if (curEngine == 0 && Surface == NULL) {
		Surface = FUN_004eca00();
	}
	return Surface;
}

// 0x4eab60
void SurfaceExitInstance()
{
	if (Surface != NULL)
		delete Surface;
	Surface = NULL;
}

// 0x4eab80. The default test only takes the main window.
bool TSurface::TestEngine()
{
	Window = SurfaceWindow;
	return true;
}

// 0x4eab90
TSurface::TSurface()
{
	Data38 = 0;
	Data3C = 0;
	Data40 = 0;
	Flag26 = false;
}

// 0x4eabb0
void TSurface::Unknown50(int a, int b, int c, int d)
{
	Unknown54(a, b, c, d);
}

// 0x4eabd0
TSurface::~TSurface()
{
}

// 0x4eabe0
uint8 GetAvailableEngines()
{
	return AvailableEngines;
}

// 0x4eabf0
void TSurface::Unknown98(int a, int b, int c, int d, int e)
{
}

// 0x4eac00: Unknown04 and Unknown08 share this copy (identical code).
void TSurface::Unknown04(int Value)
{
}

void TSurface::Unknown08(int Value)
{
}

// 0x4eac10
void TSurface::Unknown80(int a, int b, int c, int d)
{
}

// The linker kept the copy of this empty body from another object
// (0x496160, folded with other empty functions).
void TSurface::Unknown64()
{
}

// 0x4eac20. Chooses the engine at start-up and stores it (Preferences).
// An "engine N" on the command line (0-2) is taken as it is, without
// detection. Otherwise the engine from Tibia.cfg counts if it is 0-2 and
// available: DirectX 9 when installed and its TestEngine passes, OpenGL when
// an accelerated pixel format exists and works; the first engine found
// available is taken when Tibia.cfg has none, DirectX 5 is the fallback.
// Odd behaviour kept: a failed DirectX 9 test also drops the engine chosen
// in Tibia.cfg, and when ChoosePixelFormat fails the function returns
// before storing the engine.
void SurfaceInitInstance(HWND *Window)
{
	SurfaceWindow = *Window;
	bool Chosen = false;
	if (DAT_005c2564 >= 0 && DAT_005c2564 < 3) {
		curEngine = DAT_005c2564;
		SetEngine(curEngine);
		return;
	}
	int Engine = curEngine = GetEngine();
	if (Engine >= 0 && Engine < 3)
		Chosen = true;
	try {
		if (FUN_004efd10()) {
			curEngine = 2;
			GetSurface()->Window = *Window;
			if (GetSurface()->TestEngine()) {
				if (Surface != NULL)
					delete Surface;
				AvailableEngines |= 2;
				Surface = NULL;
				if (!Chosen) {
					curEngine = 2;
					Chosen = true;
				} else {
					curEngine = Engine;
				}
			} else {
				if (Surface != NULL)
					delete Surface;
				Surface = NULL;
				Chosen = false;
				curEngine = Engine;
			}
		}
	} catch (...) {
		throw (PROGRAM_ERROR)12;
	}
	HDC Context = GetDC(*Window);
	PIXELFORMATDESCRIPTOR Format = {
		sizeof(PIXELFORMATDESCRIPTOR), 1,
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER | PFD_DEPTH_DONTCARE,
		PFD_TYPE_RGBA, 32,
	};
	// The binary has the unwind states of a try block here but no handler:
	// ChoosePixelFormat and DescribePixelFormat are C functions, which the
	// compiler assumes cannot throw, so it removed the catch (its body is
	// unknown).
	try {
		int Number = ChoosePixelFormat(Context, &Format);
		if (Number == 0)
			return;
		DescribePixelFormat(Context, Number, sizeof(PIXELFORMATDESCRIPTOR), &Format);
	} catch (...) {
		throw (PROGRAM_ERROR)12;
	}
	if ((Format.dwFlags & PFD_GENERIC_ACCELERATED) == 0 && (Format.dwFlags & PFD_GENERIC_FORMAT) == 0
			&& FUN_004f9710(Context)) {
		AvailableEngines |= 1;
		if (!Chosen) {
			curEngine = 1;
			Chosen = true;
		}
	}
	if (Chosen && curEngine == 2 && (AvailableEngines & 2) == 0)
		curEngine = 1;
	if (!Chosen || (curEngine == 1 && (AvailableEngines & 1) == 0))
		curEngine = 0;
	SetEngine(curEngine);
}
