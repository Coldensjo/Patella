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
	// +0x38: Blit without a colour key, clipped (surfaceDX9.cpp; its BlitRect and BlitClipped call it).
	virtual void BlitRectClipped(int ToSurface, int x, int y, int FromSurface, TRect Source, TRect Clip) = 0;
	virtual void BlitClipped(int ToSurface, int x, int y, int FromSurface, int FromX, int FromY,
		int Width, int Height, TRGBColor ColorKey, TRect Clip) = 0;   // +0x3c
	virtual void StretchBlitRect(int ToSurface, TRect Destination, int FromSurface,
		TRect Source) = 0;                                            // +0x40
	// +0x44: StretchBlit without a clip rectangle (surfaceDX9.cpp).
	virtual void StretchBlitUnclipped(int ToSurface, int x, int y, int Width, int Height, int FromSurface,
		int FromX, int FromY, int FromWidth, int FromHeight, TRGBColor ColorKey, TRGBColor Highlight) = 0;
	virtual void StretchBlit(int ToSurface, int x, int y, int Width, int Height, int FromSurface,
		int FromX, int FromY, int FromWidth, int FromHeight, TRGBColor ColorKey, TRGBColor Highlight,
		TRect Clip) = 0;                                              // +0x48
	// +0x4c: shades Count rectangles with the colours of their four corners
	// (top left, top right, bottom left, bottom right); MapWindow.cpp's light.
	virtual void ShadeRectangles(int Surface, int Count, TRect *Rects, TRGBColor *Colors) = 0;
	virtual void Unknown50(int Surface, TRGBColor ColorKey);         // +0x50, calls +0x54; graphics.cpp sets a cache surface's colour key
	virtual void SetColorKey(int Surface, TRGBColor ColorKey) = 0;  // +0x54 (surfaceDX9.cpp: the key of a surface)
	// +0x58: clips Rect (left, top, right, bottom) to Clip and moves Source by the same amounts; false
	// when nothing is left (surfaceDX9.cpp; the copy is shared by all three engines).
	virtual bool ClipRect(TRect Clip, RECT *Rect, RECT *Source) = 0;
	virtual void InitFramerate() = 0;                                 // +0x5c, the frame clock (InitSurfaces)
	virtual void LimitFramerate() = 0;                                // +0x60, counts the frame, sleeps to the limit (FinishFrame)
	virtual void Unknown64();                                         // +0x64, empty
	virtual void UpdateWindowRect() = 0;                              // +0x68, the client rectangles in windowed mode
	virtual void SetDisplayMode(int VMode, TFullscreenMode *GMode) = 0; // +0x6c (surfaceDX9.cpp assertions: VMode, GMode)
	virtual void ExitDevice() = 0;                                    // +0x70, releases the device and its surfaces
	virtual void InitDevice() = 0;                                    // +0x74, creates them for ViewMode
	virtual void ChangeDisplayMode(int VMode, TFullscreenMode *GMode) = 0; // +0x78, +0x6c and +0x74, +0x70 on an error
	virtual void CheckDevice(int ViewMode, TFullscreenMode *Resolution) = 0; // +0x7c, restores a lost device (Init)
	virtual void Unknown80(int a, int b, int c, int d);               // +0x80, empty
	virtual void FinishFrame() = 0;                                   // +0x84
	virtual bool Init(HWND *Window, int ViewMode, TFullscreenMode *Resolution, bool Force) = 0; // +0x88
	virtual int GetFullscreenModes(TFullscreenMode **Modes) = 0;      // +0x8c, the modes and their number (Dialogs.cpp)
	virtual int GetCacheSurfaces() = 0;                               // +0x90, the number of video cache surfaces
	virtual void GetSurfaceRect(int Surface, TRect *Rect) = 0;        // +0x94
	virtual void SetSurfaceRect(int Surface, int x, int y, int Width, int Height); // +0x98, empty here (surfaceDX9.cpp sets Rect[Surface])
	virtual bool Unknown9C(int Value) = 0;                            // +0x9c
	virtual bool IsUnavailable() = 0;                                 // +0xa0
	virtual bool CanShadeRectangles() = 0;                            // +0xa4, MapWindow.cpp draws the light with +0x4c only when true
	virtual int IsSurfaceCreated(int Surface) = 0;                    // +0xa8, the surface was created (surfaceDX9.cpp)
	virtual void SetFramerateLimit(int Limit) = 0;                    // +0xac, Dialogs.cpp; 201 or above is no limit
	virtual double GetFPS() = 0;                                      // +0xb0
	virtual void InitSurfaces() = 0;                                  // +0xb4
	virtual void ExitSurfaces() = 0;                                  // +0xb8
	virtual bool TestEngine();                                        // +0xbc
	uint8 GetAvailableEngines();                                      // 0x4eabe0, the engines found at start-up (bit 0 OpenGL, bit 1 DirectX 9)

	// Names from surfaceDX9.cpp's assertions: hWindow, ViewMode (WINDOWED_MODE 1, FULLSCREEN_MODE 2;
	// 0 before Init); the others are ours, from surfaceDX9.cpp's uses.
	int Data04;          // +0x04
	HWND hWindow;        // +0x08, the main window (TestEngine, SurfaceInitInstance)
	int Data0C;          // +0x0c
	int ViewMode;        // +0x10
	int ModeWidth;       // +0x14, the full-screen mode (SetDisplayMode)
	int ModeHeight;      // +0x18
	int ModeDepth;       // +0x1c
	int Data20;          // +0x20
	bool Lost;           // +0x24, the device is lost (IsUnavailable)
	bool CanStretch;     // +0x25, DirectDraw can shrink and stretch (InitSurfaces)
	bool Flag26;         // +0x26, cleared by the constructor; StretchBlit fills its background with skin 185
	uint8 Data27[0x11];  // +0x27
	int NumberOfModes;   // +0x38, cleared by the constructor; counted by the mode enumeration
	int Data3C;          // +0x3c, cleared by the constructor
	int NumberOfCacheSurfaces; // +0x40, cleared by the constructor (GetCacheSurfaces)
};

int GetCurrentEngine();
TSurface *GetSurface();
void SurfaceExitInstance();
void SurfaceInitInstance(HWND *Window);

// Frame statistics and cache sizes the engines share. Surface.obj's data by
// address (between curEngine's 0x71d0c0-0x71d0cc and SurfaceDX.cpp's data);
// the engines write them, nothing reads the counters of the last frame.
// Names ours (low); added by surfaceDX9.cpp (issue #10).
extern int FrameBlits;            // 0x71d0d0, drawing operations of the current frame
extern int FramePixels;           // 0x71d0d4, pixels drawn in the current frame
extern int FrameStateChanges;     // 0x71d0d8, cleared by FinishFrame (SurfaceGL.cpp counts)
extern int ScreenWidth;           // 0x71d0dc, set by FinishFrame
extern int ScreenHeight;          // 0x71d0e0
extern int LastFrameBlits;        // 0x71d0e8
extern int LastFramePixels;       // 0x71d0ec
extern int NumberOfVideoSurfaces; // 0x71d0f0, the surfaces created so far
extern int CacheSurfaceWidth;     // 0x5c2568 (.data, 256), set by InitSurfaces
extern int CacheSurfaceHeight;    // 0x5c256c (.data, 256)
