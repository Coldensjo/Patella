// Surface.h: the graphics engine and the surface base class.
//
// The file name Surface.cpp is inferred: the code lies between Status.cpp and
// SurfaceDX.cpp in the alphabetical link order and is the engine-independent
// part of the surface classes (SurfaceDX.cpp, surfaceDX9.cpp, SurfaceGL.cpp
// derive from TSurface). Names from the binary: curEngine,
// SurfaceExitInstance, and the methods TestEngine, InitSurfaces and
// ExitSurfaces (ChangeEngine's error messages); the rest are ours.

#pragma once

#include "common.h"
struct TRect;
struct TRGBColor;
struct TFullscreenMode;

// Engines: 0 DirectX 5 (SurfaceDX.cpp), 1 OpenGL (SurfaceGL.cpp), 2 DirectX 9
// (surfaceDX9.cpp).
extern int curEngine; // 0x71d0c4

// The vtable is at 0x56ef78. Slots with _purecall in the binary are pure;
// the others have the default bodies in Surface.cpp. Unknown slots keep
// their offsets; their parameters are known only where a default body
// shows them (by its RET n). The slots graphics.cpp calls (+0x28, +0x2c,
// +0x34, +0x3c, +0x48, +0x50, +0x90, +0x94) have their parameters from those
// calls (ToSurface and FromSurface from SurfaceGL.cpp's assertions; the other
// names are ours); +0x50 gets a colour key by value (the callers copy a
// TRGBColor to the stack) and passes it on to +0x54. Surfaces 6 and up are
// the video cache surfaces.
class TSurface {
public:
	TSurface();
	virtual ~TSurface();                                              // +0x00
	virtual void Unknown04(int Value);                                // +0x04, empty
	virtual void Unknown08(int Value);                                // +0x08, empty
	virtual void SetPixel(int Surface, int x, int y, TRGBColor Color) = 0; // +0x0c, ContentWindows.cpp's automap cross
	virtual void FillRectangleClipped(int Surface, TRect Rect, TRGBColor Color, TRect Clip) = 0; // +0x10, ContentWindows.cpp's bars
	virtual void FillRectangle(int Surface, TRect Rect, TRGBColor Color) = 0; // +0x14
	virtual void DrawHorizontalLine(int Surface, int x, int y, int Width, TRGBColor Color) = 0; // +0x18, ContentWindows.cpp's separator line
	virtual void DrawVerticalLine(int Surface, int x, int y, int Height, TRGBColor Color) = 0; // +0x1c, DialogElements.cpp's edit line cursor
	virtual void DrawFrameClipped(int Surface, TRect Rect, int LineWidth, TRGBColor Color,
		TRect Clip) = 0;                                              // +0x20
	virtual void DrawFrame(int Surface, TRect Rect, int LineWidth, TRGBColor Color) = 0; // +0x24
	virtual void WriteImage(int ToSurface, int x, int y, int Width, int Height, uint8 *Data) = 0; // +0x28
	virtual void WriteMaskedImage(int ToSurface, int x, int y, int Width, int Height, uint8 *Data,
		uint8 *Mask, TRGBColor *Colors) = 0;                         // +0x2c
	virtual void BlitRect(int ToSurface, int x, int y, int FromSurface, TRect Source) = 0; // +0x30
	virtual void Blit(int ToSurface, int x, int y, int FromSurface, int FromX, int FromY, int Width,
		int Height, TRGBColor ColorKey) = 0;                          // +0x34
	virtual void Unknown38() = 0;
	virtual void BlitClipped(int ToSurface, int x, int y, int FromSurface, int FromX, int FromY,
		int Width, int Height, TRGBColor ColorKey, TRect Clip) = 0;   // +0x3c
	virtual void StretchBlitRect(int ToSurface, TRect Destination, int FromSurface,
		TRect Source) = 0;                                            // +0x40
	virtual void Unknown44() = 0;
	virtual void StretchBlit(int ToSurface, int x, int y, int Width, int Height, int FromSurface,
		int FromX, int FromY, int FromWidth, int FromHeight, TRGBColor ColorKey, TRGBColor Highlight,
		TRect Clip) = 0;                                              // +0x48
	// +0x4c: shades Count rectangles with the colours of their four corners
	// (top left, top right, bottom left, bottom right); MapWindow.cpp's light.
	virtual void ShadeRectangles(int Surface, int Count, TRect *Rects, TRGBColor *Colors) = 0;
	virtual void Unknown50(int Surface, TRGBColor ColorKey);         // +0x50, calls +0x54; graphics.cpp sets a cache surface's colour key
	virtual void Unknown54(int Surface, TRGBColor ColorKey) = 0;
	virtual void Unknown58() = 0;
	virtual void Unknown5C() = 0;
	virtual void Unknown60() = 0;
	virtual void Unknown64();                                         // +0x64, empty
	virtual void Unknown68() = 0;
	virtual void Unknown6C() = 0;
	virtual void Unknown70() = 0;
	virtual void Unknown74() = 0;
	virtual void Unknown78() = 0;
	virtual void Unknown7C() = 0;
	virtual void Unknown80(int a, int b, int c, int d);               // +0x80, empty
	virtual void FinishFrame() = 0;                                   // +0x84
	virtual bool Init(HWND *Window, int ViewMode, TFullscreenMode *Resolution, bool Force) = 0; // +0x88
	virtual void Unknown8C() = 0;
	virtual int GetCacheSurfaces() = 0;                               // +0x90, the number of video cache surfaces
	virtual void GetSurfaceRect(int Surface, TRect *Rect) = 0;        // +0x94
	virtual void Unknown98(int a, int b, int c, int d, int e);        // +0x98, empty
	virtual bool Unknown9C(int Value) = 0;                            // +0x9c
	virtual bool IsUnavailable() = 0;                                 // +0xa0
	virtual bool CanShadeRectangles() = 0;                            // +0xa4, MapWindow.cpp draws the light with +0x4c only when true
	virtual void UnknownA8() = 0;
	virtual void UnknownAC() = 0;
	virtual double GetFPS() = 0;                                      // +0xb0
	virtual void InitSurfaces() = 0;                                  // +0xb4
	virtual void ExitSurfaces() = 0;                                  // +0xb8
	virtual bool TestEngine();                                        // +0xbc

	int Data04;          // +0x04
	HWND Window;         // +0x08, the main window (TestEngine, SurfaceInitInstance)
	int Data0C[6];       // +0x0c
	uint8 Data24[2];     // +0x24
	bool Flag26;         // +0x26, cleared by the constructor
	uint8 Data27[0x11];  // +0x27
	int Data38;          // +0x38, cleared by the constructor
	int Data3C;          // +0x3c, cleared by the constructor
	int Data40;          // +0x40, cleared by the constructor
};

int GetCurrentEngine();
TSurface *GetSurface();
void SurfaceExitInstance();
uint8 GetAvailableEngines();
void SurfaceInitInstance(HWND *Window);
