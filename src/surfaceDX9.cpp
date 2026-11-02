// surfaceDX9.cpp: reconstructed from Tibia.exe 7.72 (0x4efd10-0x4f46ef).
//
// The Direct3D 9 surface. Every drawing operation is a textured quad
// (two triangles) in a dynamic vertex buffer; the buffer is drawn into the
// current render target when the texture or the target changes, when it is
// full, and at the end of the frame. Rectangles, lines and pixels are colour
// fills. The surfaces are render-target or dynamic textures (Surface9[]),
// their level 0 the render targets; SCREEN_SURFACE is the back buffer.
//
// Only the functions with assertions have line numbers (496-2055); they are
// in binary order, so the file is in binary order. Seven vtable slots have no
// code here: the linker kept an identical copy from another object
// (SurfaceDX.obj's SetFramerateLimit 0x4eb000, SurfaceGL.obj's DrawFrame,
// DrawFrameClipped, BlitClipped, GetCacheSurfaces, IsUnavailable, GetFPS);
// they are at the end. The display mode enumeration's callback is folded
// into SurfaceDX.obj's copy 0x4ec810 in the same way.

#include "common.h"
#include <initguid.h>
#include "surfaceDX9.h"
#include "GUI.h"
#include "Preferences.h"
#include "external.h"

// Assertions whose text names a variable this file does not have: the
// original's FromRect and ToRect are rectangles with members width and
// height (TRect's members are Width and Height here); the binary passes
// their members separately, so the slots keep TSurface's int parameters.
#define TASSERT_TEXT(Expr, Text) \
	if (!(Expr)) throw TMYEXCEPTION("assertion failed", "", 0, "", 0, Text)
#define TASSERT2_TEXT(Expr, Text, N1, V1, N2, V2) \
	if (!(Expr)) throw TMYEXCEPTION("assertion failed", N1, V1, N2, V2, Text)
#define TASSERTB1(Expr, V1, Bug) \
	if (!(Expr)) throw TMYEXCEPTION("assertion failed", #V1, V1, "", 0, #Expr " " Bug)

typedef IDirect3D9 *(WINAPI *TDirect3DCreate9)(UINT SDKVersion);

// Data, in address order (.data, then .bss).
static int CurrentTargetNumber = -1;                  // 0x5c2574
static D3DPRESENT_PARAMETERS PresentParameters;       // 0x71d170
static D3DCOLOR ColorKey[NUMBER_OF_SURFACES_DX9];     // 0x71d1a8, 0 when the surface has none
static TVertexDX9 *ShadeVertices;                     // 0x71d200, ShadeBuffer while it is locked
static TVertexDX9 *Vertices;                          // 0x71d204, VertexBuffer, locked between draws
static int SurfaceCreated[NUMBER_OF_SURFACES_DX9];    // 0x71d208, indexed by NumberOfVideoSurfaces
static int CurrentTexture;                            // 0x71d260
static int NumberOfVertices;                          // 0x71d26c
static int NumberOfShadeVertices;                     // 0x71d270
static IDirect3DSurface9 *TempSurface;                // 0x71d274, 64x64 in system memory (ApplyColorKey)
static TDirect3DCreate9 Direct3DCreate9Function;      // 0x71d278
static HMODULE Direct3D9Module;                       // 0x71d27c, never set (IsDirect3D9Installed does not keep the module)
static uint32 FrameCount;                             // 0x71d280
static double LastFPSTime;                            // 0x71d288

// 0x4efd10. Surface.cpp's engine selection asks this before it tests the
// engine. The module handle is not kept.
bool IsDirect3D9Installed()
{
	HMODULE Module = LoadLibraryA("d3d9.dll");
	if (Module == NULL)
		return false;
	Direct3DCreate9Function = (TDirect3DCreate9)GetProcAddress(Module, "Direct3DCreate9");
	return Direct3DCreate9Function != NULL;
}

// 0x4efd40. Creates a windowed device on the default adapter and releases it
// again. Odd behaviour kept: when CreateDevice fails but returns a device,
// the device is not released.
bool TSurfaceDX9::TestEngine()
{
	TSurface::TestEngine();
	if (Direct3DCreate9Function == NULL) {
		IsDirect3D9Installed();
		if (Direct3DCreate9Function == NULL)
			return false;
	}
	Direct3D = Direct3DCreate9Function(D3D_SDK_VERSION);
	if (Direct3D == NULL)
		return false;
	D3DPRESENT_PARAMETERS Parameters;
	memset(&Parameters, 0, sizeof(Parameters));
	Parameters.Windowed = TRUE;
	Parameters.SwapEffect = D3DSWAPEFFECT_COPY;
	Parameters.BackBufferFormat = D3DFMT_UNKNOWN;
	Parameters.BackBufferWidth = GetSystemMetrics(SM_CXSCREEN);
	Parameters.BackBufferHeight = GetSystemMetrics(SM_CYSCREEN);
	Parameters.BackBufferCount = 1;
	Parameters.Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
	Parameters.MultiSampleType = D3DMULTISAMPLE_NONE;
	Parameters.MultiSampleQuality = 0;
	Parameters.hDeviceWindow = hWindow;
	Parameters.EnableAutoDepthStencil = FALSE;
	Parameters.FullScreen_RefreshRateInHz = 0;
	Parameters.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
	if (Direct3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWindow,
			D3DCREATE_HARDWARE_VERTEXPROCESSING, &Parameters, &Device) == D3D_OK && Device != NULL) {
		Device->Release();
		Device = NULL;
		if (Direct3D != NULL) {
			Direct3D->Release();
			Direct3D = NULL;
		}
		return true;
	}
	if (Direct3D != NULL) {
		Direct3D->Release();
		Direct3D = NULL;
	}
	return false;
}

// 0x4efe70. Six vertices of a rectangle in the shade buffer, the colours at
// its corners; the texture coordinates are relative to TEMP_SURFACE. There
// is no check for a full buffer.
void TSurfaceDX9::AddShadeQuad(int Left, int Top, int Right, int Bottom, D3DCOLOR TopLeft, D3DCOLOR TopRight,
	D3DCOLOR BottomRight, D3DCOLOR BottomLeft)
{
	double Left_u = (Left + 0.5) / Rect[TEMP_SURFACE].right;
	double Right_u = (Right + 0.5) / Rect[TEMP_SURFACE].right;
	double Top_v = (Top + 0.5) / Rect[TEMP_SURFACE].bottom;
	double Bottom_v = (Bottom + 0.5) / Rect[TEMP_SURFACE].bottom;
	float x1 = (float)Left;
	float y1 = (float)Top;
	float x2 = (float)Right;
	float y2 = (float)Bottom;
	TVertexDX9 *V = &ShadeVertices[NumberOfShadeVertices];
	V[0].x = x1; V[0].y = y1; V[0].z = 1.0f; V[0].rhw = 1.0f; V[0].Color = TopLeft;
	V[0].u = (float)Left_u; V[0].v = (float)Top_v;
	V[1].x = x2; V[1].y = y1; V[1].z = 1.0f; V[1].rhw = 1.0f; V[1].Color = TopRight;
	V[1].u = (float)Right_u; V[1].v = (float)Top_v;
	V[2].x = x2; V[2].y = y2; V[2].z = 1.0f; V[2].rhw = 1.0f; V[2].Color = BottomRight;
	V[2].u = (float)Right_u; V[2].v = (float)Bottom_v;
	V[3].x = x2; V[3].y = y2; V[3].z = 1.0f; V[3].rhw = 1.0f; V[3].Color = BottomRight;
	V[3].u = (float)Right_u; V[3].v = (float)Bottom_v;
	V[4].x = x1; V[4].y = y2; V[4].z = 1.0f; V[4].rhw = 1.0f; V[4].Color = BottomLeft;
	V[4].u = (float)Left_u; V[4].v = (float)Bottom_v;
	V[5].x = x1; V[5].y = y1; V[5].z = 1.0f; V[5].rhw = 1.0f; V[5].Color = TopLeft;
	V[5].u = (float)Left_u; V[5].v = (float)Top_v;
	NumberOfShadeVertices += 6;
}

// 0x4f02a0. Draws the quads collected so far into the current target.
void TSurfaceDX9::FlushVertices()
{
	if (NumberOfVertices > 0) {
		VertexBuffer->Unlock();
		Device->BeginScene();
		Device->DrawPrimitive(D3DPT_TRIANGLELIST, 0, NumberOfVertices / 3);
		Device->EndScene();
		VertexBuffer->Lock(0, 0, (void **)&Vertices, D3DLOCK_DISCARD);
		NumberOfVertices = 0;
	}
}

// 0x4f0330. SCREEN_SURFACE's texture is ScreenTexture.
void TSurfaceDX9::SetTexture(int Surface)
{
	if (CurrentTexture != Surface) {
		FlushVertices();
		Device->SetTexture(0, Surface == SCREEN_SURFACE ? ScreenTexture : Surface9[Surface]);
		CurrentTexture = Surface;
	}
}

// 0x4f0380
void TSurfaceDX9::SetRenderTarget(int Surface)
{
	if (CurrentTargetNumber != Surface) {
		FlushVertices();
		Device->SetRenderTarget(0, RenderTarget[Surface]);
		CurrentTargetNumber = Surface;
	}
	CurrentTarget = RenderTarget[Surface];
}

// 0x4f03e0
int TSurfaceDX9::IsSurfaceCreated(int Surface)
{
	return SurfaceCreated[Surface];
}

// 0x4f03f0
TSurfaceDX9::~TSurfaceDX9()
{
	NumberOfVideoSurfaces = 0;
}

// 0x4f0410 (the same code in all three engines; this copy is kept).
bool TSurfaceDX9::ClipRect(TRect Clip, RECT *Rect, RECT *Source)
{
	int ClipRight = Clip.x + Clip.Width;
	int ClipBottom = Clip.y + Clip.Height;
	if (Rect->left < ClipRight && Rect->top < ClipBottom && Rect->right > Clip.x && Rect->bottom > Clip.y) {
		if (Rect->left < Clip.x) {
			Source->left += Clip.x - Rect->left;
			Rect->left = Clip.x;
		}
		if (Rect->top < Clip.y) {
			Source->top += Clip.y - Rect->top;
			Rect->top = Clip.y;
		}
		if (Rect->right > ClipRight) {
			Source->right += Clip.Width - Rect->right + Clip.x;
			Rect->right = ClipRight;
		}
		if (Rect->bottom > ClipBottom) {
			Source->bottom += Clip.Height - Rect->bottom + Clip.y;
			Rect->bottom = ClipBottom;
		}
		return true;
	}
	return false;
}

// 0x4f04d0 (shared with the other engines)
void TSurfaceDX9::InitFramerate()
{
	double Now = timeGetTime();
	StartTime = Now;
	FramerateLimit = 0xffff;
	NextFrameTime = Now;
	FPS = 0.0;
	FrameTime = 1000.0 / 0xffff;
}

// 0x4f0510. Measures the frame rate once a second and sleeps until the next
// frame is due; a frame more than a millisecond late moves the schedule.
void TSurfaceDX9::LimitFramerate()
{
	double Now = timeGetTime();
	FrameCount++;
	if (Now - LastFPSTime > 1000.0) {
		FPS = (FrameCount * 1000) / (Now - LastFPSTime);
		FrameCount = 0;
		LastFPSTime = Now;
	}
	NextFrameTime += FrameTime;
	if (NextFrameTime > Now)
		Sleep((int)(NextFrameTime - Now));
	else if (Now - NextFrameTime > 1.0)
		NextFrameTime = Now - 1.0;
}

// 0x4f05d0
void TSurfaceDX9::UpdateWindowRect()
{
	if (ViewMode == WINDOWED_MODE) {
		GetClientRect(hWindow, &Rect[PRIMARY_SURFACE]);
		GetClientRect(hWindow, &Rect[SCREEN_SURFACE]);
		GetClientRect(hWindow, &WindowRect);
		ClientToScreen(hWindow, (POINT *)&WindowRect.left);
		ClientToScreen(hWindow, (POINT *)&WindowRect.right);
	}
}

// 0x4f0630
void TSurfaceDX9::SetPixel(int Surface, int x, int y, TRGBColor Color)
{
	RECT R = {x, y, x + 1, y + 1};
	SetRenderTarget(Surface);
	FlushVertices();
	Device->ColorFill(CurrentTarget, &R, D3DCOLOR_XRGB(Color.R, Color.G, Color.B));
}

// 0x4f06b0
void TSurfaceDX9::FillRectangle(int Surface, TRect Rect, TRGBColor Color)
{
	RECT R = {Rect.x, Rect.y, Rect.x + Rect.Width, Rect.y + Rect.Height};
	if (R.top != R.bottom && R.left != R.right) {
		SetRenderTarget(Surface);
		FlushVertices();
		Device->ColorFill(CurrentTarget, &R, D3DCOLOR_XRGB(Color.R, Color.G, Color.B));
		FrameBlits++;
		FramePixels += Rect.Height * Rect.Width;
	}
}

// 0x4f0770. ClipRect's second rectangle is the stack slot of Rect, which it
// moves as if it were a RECT; nothing reads it afterwards, and the pixel
// count uses the size before clipping.
void TSurfaceDX9::FillRectangleClipped(int Surface, TRect Rect, TRGBColor Color, TRect Clip)
{
	RECT R = {Rect.x, Rect.y, Rect.x + Rect.Width, Rect.y + Rect.Height};
	if (R.top != R.bottom && R.left != R.right) {
		int Pixels = Rect.Height * Rect.Width;
		if (ClipRect(Clip, &R, (RECT *)&Rect)) {
			SetRenderTarget(Surface);
			FlushVertices();
			Device->ColorFill(CurrentTarget, &R, D3DCOLOR_XRGB(Color.R, Color.G, Color.B));
			FrameBlits++;
			FramePixels += Pixels;
		}
	}
}

// 0x4f0870
void TSurfaceDX9::DrawHorizontalLine(int Surface, int x, int y, int Width, TRGBColor Color)
{
	RECT R = {x, y, x + Width, y + 1};
	if (R.top != R.bottom && R.left != R.right) {
		SetRenderTarget(Surface);
		FlushVertices();
		Device->ColorFill(CurrentTarget, &R, D3DCOLOR_XRGB(Color.R, Color.G, Color.B));
		FrameBlits++;
		FramePixels += Width;
	}
}

// 0x4f0920
void TSurfaceDX9::DrawVerticalLine(int Surface, int x, int y, int Height, TRGBColor Color)
{
	RECT R = {x, y, x + 1, y + Height};
	if (R.top != R.bottom && R.left != R.right) {
		SetRenderTarget(Surface);
		FlushVertices();
		Device->ColorFill(CurrentTarget, &R, D3DCOLOR_XRGB(Color.R, Color.G, Color.B));
		FrameBlits++;
		FramePixels += Height;
	}
}

// 0x4f09d0. Writes RGB data into a cache surface's texture; the pixels of the
// surface's colour key become transparent. Below SPRITE_SURFACE the data goes
// into ScreenTexture at (x, y) and the whole 106x106 texture is blitted to
// AUTOMAP_SURFACE with point filtering, whatever ToSurface is (the key is
// still ToSurface's).
void TSurfaceDX9::WriteImage(int ToSurface, int x, int y, int Width, int Height, uint8 *Data)
{
	D3DLOCKED_RECT Locked;
	if (ToSurface < SPRITE_SURFACE) {
		RECT R;
		SetRect(&R, 0, 0, 106, 106);
		ScreenTexture->LockRect(0, &Locked, &R, D3DLOCK_NOSYSLOCK);
		D3DCOLOR *Bits = (D3DCOLOR *)Locked.pBits;
		for (int i = 0; i < Height; i++) {
			for (int j = 0; j < Width; j++) {
				D3DCOLOR Color = D3DCOLOR_XRGB(Data[0], Data[1], Data[2]);
				if (Color == ColorKey[ToSurface])
					Color = Data[0] << 16 | Data[1] << 8 | Data[2];
				Data += 3;
				Bits[Locked.Pitch / 4 * (i + y) + j + x] = Color;
			}
		}
		ScreenTexture->UnlockRect(0);
		Device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
		Device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
		BlitRect(AUTOMAP_SURFACE, 0, 0, SCREEN_SURFACE, TRect(0, 0, 106, 106));
		FlushVertices();
		Device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
		Device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
		return;
	}
	Surface9[ToSurface]->LockRect(0, &Locked, &Rect[ToSurface], D3DLOCK_NOSYSLOCK);
	D3DCOLOR *Bits = (D3DCOLOR *)Locked.pBits;
	for (int i = 0; i < Height; i++) {
		for (int j = 0; j < Width; j++) {
			D3DCOLOR Color = D3DCOLOR_XRGB(Data[0], Data[1], Data[2]);
			if (Color == ColorKey[ToSurface])
				Color = Data[0] << 16 | Data[1] << 8 | Data[2];
			Data += 3;
			Bits[Locked.Pitch / 4 * (i + y) + j + x] = Color;
		}
	}
	Surface9[ToSurface]->UnlockRect(0);
}

// 0x4f0c60. Makes the pixels of ColorKey in the top left 64x64 of Surface
// transparent (white with alpha 0) through the system-memory TempSurface and
// writes the result to STRETCH_SURFACE; the source rectangle of that copy is
// Rect[Surface].
void TSurfaceDX9::ApplyColorKey(int Surface, TRGBColor ColorKey)
{
	Device->GetRenderTargetData(RenderTarget[Surface], TempSurface);
	RECT R = {0, 0, 64, 64};
	D3DLOCKED_RECT Locked;
	TempSurface->LockRect(&Locked, &R, D3DLOCK_NOSYSLOCK);
	D3DCOLOR *Bits = (D3DCOLOR *)Locked.pBits;
	D3DCOLOR Key = D3DCOLOR_XRGB(ColorKey.R, ColorKey.G, ColorKey.B);
	for (int i = 0; i < 64; i++) {
		for (int j = 0; j < 64; j++) {
			if (Bits[Locked.Pitch / 4 * i + j] == Key)
				Bits[Locked.Pitch / 4 * i + j] = 0xffffff;
		}
	}
	TempSurface->UnlockRect();
	Device->UpdateSurface(TempSurface, &this->Rect[Surface], RenderTarget[STRETCH_SURFACE], NULL);
}

// 0x4f0d50. Copies TEMP_SURFACE to PRIMARY_SURFACE and draws the shaded
// rectangles into TEMP_SURFACE.
void TSurfaceDX9::FlushShade()
{
	StretchBlitRect(PRIMARY_SURFACE, TRect(0, 0, Rect[TEMP_SURFACE].right, Rect[TEMP_SURFACE].bottom),
		TEMP_SURFACE, TRect(0, 0, Rect[TEMP_SURFACE].right, Rect[TEMP_SURFACE].bottom));
	SetTexture(PRIMARY_SURFACE);
	SetRenderTarget(TEMP_SURFACE);
	Device->SetStreamSource(0, ShadeBuffer, 0, sizeof(TVertexDX9));
	Device->BeginScene();
	Device->DrawPrimitive(D3DPT_TRIANGLELIST, 0, NumberOfShadeVertices / 3);
	Device->EndScene();
	NumberOfShadeVertices = 0;
	Device->SetStreamSource(0, VertexBuffer, 0, sizeof(TVertexDX9));
}

// 0x4f0e50. MapWindow.cpp's light: only on TEMP_SURFACE. The corner colours
// are top left, top right, bottom left, bottom right.
void TSurfaceDX9::ShadeRectangles(int Surface, int Count, TRect *Rects, TRGBColor *Colors)
{
	if (Surface == TEMP_SURFACE && Direct3D != NULL) {
		ShadeBuffer->Lock(0, 0, (void **)&ShadeVertices, D3DLOCK_DISCARD);
		for (int i = 0; i < Count; i++) {
			TRect *R = &Rects[i];
			TRGBColor *C = &Colors[i * 4];
			AddShadeQuad(R->x, R->y, R->x + R->Width, R->y + R->Height,
				D3DCOLOR_XRGB(C[0].R, C[0].G, C[0].B), D3DCOLOR_XRGB(C[1].R, C[1].G, C[1].B),
				D3DCOLOR_XRGB(C[3].R, C[3].G, C[3].B), D3DCOLOR_XRGB(C[2].R, C[2].G, C[2].B));
		}
		ShadeBuffer->Unlock();
		if (NumberOfShadeVertices > 0)
			FlushShade();
	}
}

// 0x4f0f80. The binary compares ViewMode with WINDOWED_MODE before Present
// and does not use the result (both modes present the same way).
void TSurfaceDX9::FinishFrame()
{
	if (ViewMode != 0) {
		LimitFramerate();
		ScreenWidth = Rect[SCREEN_SURFACE].right;
		ScreenHeight = Rect[SCREEN_SURFACE].bottom;
		LastFramePixels = FramePixels;
		LastFrameBlits = FrameBlits;
		FrameStateChanges = 0;
		FrameBlits = 0;
		FramePixels = 0;
		FlushVertices();
		Device->Present(&Rect[SCREEN_SURFACE], NULL, NULL, NULL);
		FrameBlits++;
		FramePixels += (WindowRect.bottom - WindowRect.top) * (WindowRect.right - WindowRect.left);
	}
}

// 0x4f1040. Releases the device and its surfaces (only while DirectDraw
// exists). SCREEN_SURFACE's texture slot is released before the loop.
void TSurfaceDX9::ExitDevice()
{
	if (DirectDraw == NULL)
		return;
	if (TempSurface != NULL) {
		TempSurface->Release();
		TempSurface = NULL;
	}
	if (ScreenTexture != NULL) {
		ScreenTexture->Release();
		ScreenTexture = NULL;
	}
	if (VertexBuffer != NULL) {
		VertexBuffer->Release();
		VertexBuffer = NULL;
	}
	if (ShadeBuffer != NULL) {
		ShadeBuffer->Release();
		ShadeBuffer = NULL;
	}
	if (Surface9[SCREEN_SURFACE] != NULL) {
		Surface9[SCREEN_SURFACE]->Release();
		Surface9[SCREEN_SURFACE] = NULL;
	}
	for (int i = 0; i < NUMBER_OF_SURFACES_DX9; i++) {
		if (Surface9[i] != NULL) {
			Surface9[i]->Release();
			Surface9[i] = NULL;
			SurfaceCreated[i] = 0;
		}
		if (RenderTarget[i] != NULL) {
			RenderTarget[i]->Release();
			RenderTarget[i] = NULL;
		}
	}
	NumberOfCacheSurfaces = 0;
	if (Device != NULL) {
		Device->Release();
		Device = NULL;
	}
	if (Direct3D != NULL) {
		Direct3D->Release();
		Direct3D = NULL;
	}
	if (Direct3D9Module != NULL) {
		FreeLibrary(Direct3D9Module);
		Direct3D9Module = NULL;
		Direct3DCreate9Function = NULL;
	}
	ViewMode = 0;
}

// 0x4f1160. ExitDevice cleans up when switching fails; an exception of
// ExitDevice itself is swallowed.
void TSurfaceDX9::ChangeDisplayMode(int VMode, TFullscreenMode *GMode)
{
	try {
		SetDisplayMode(VMode, GMode);
		InitDevice();
	} catch (PROGRAM_ERROR Error) {
		try {
			ExitDevice();
		} catch (...) {
		}
		throw Error;
	}
}

// 0x4f11f0. Odd behaviour kept: a device that can be reset is reset, then
// released (ExitDevice) and the surface is set back to its start state
// (InitSurfaces); Lost stays set. The parameters are not used.
void TSurfaceDX9::CheckDevice(int ViewMode, TFullscreenMode *Resolution)
{
	if (Device->TestCooperativeLevel() == D3DERR_DEVICENOTRESET) {
		Lost = true;
		Device->Reset(&PresentParameters);
		ExitDevice();
		InitSurfaces();
		return;
	}
	Lost = false;
}

// 0x4f1240. Switches to VMode (with the resolution Resolution in full-screen
// mode) only when something changes, or always with Force.
bool TSurfaceDX9::Init(HWND *Window, int VMode, TFullscreenMode *Resolution, bool Force)
{
	Ready = false;
	if (Force) {
		ExitDevice();
		hWindow = *Window;
		ChangeDisplayMode(VMode, Resolution);
	} else if (VMode == WINDOWED_MODE) {
		switch (ViewMode) {
		case FULLSCREEN_MODE:
			ExitDevice();
			// fall through
		case 0:
			hWindow = *Window;
			ChangeDisplayMode(WINDOWED_MODE, NULL);
			break;
		case WINDOWED_MODE:
			if (hWindow != *Window)
				hWindow = *Window;
			UpdateWindowRect();
			break;
		}
	} else if (VMode == FULLSCREEN_MODE) {
		if (ViewMode != FULLSCREEN_MODE || Resolution->Width != ModeWidth || Resolution->Height != ModeHeight
				|| Resolution->Depth != ModeDepth) {
			ExitDevice();
			hWindow = *Window;
			ChangeDisplayMode(FULLSCREEN_MODE, Resolution);
		}
	}
	CheckDevice(VMode, Resolution);
	return true;
}

// The display mode callback of EnumDisplayModes (folded into SurfaceDX.obj's
// identical copy 0x4ec810): collects the modes of at least 640x480 and 16 bits
// in a growing array, counted in the surface's NumberOfModes. Odd behaviour
// kept: the refresh rate is taken when the pixel format's flags (not the
// description's) have DDSD_REFRESHRATE.
static HRESULT WINAPI EnumModesCallback(DDSURFACEDESC2 *Description, void *Context)
{
	TFullscreenMode **Modes = (TFullscreenMode **)Context;
	if (*Modes == NULL)
		GetSurface()->NumberOfModes = 0;
	int Width = (Description->dwFlags & DDSD_WIDTH) ? Description->dwWidth : 0;
	int Height = (Description->dwFlags & DDSD_HEIGHT) ? Description->dwHeight : 0;
	int Depth = (Description->ddpfPixelFormat.dwFlags & DDPF_RGB) ? Description->ddpfPixelFormat.dwRGBBitCount : 0;
	int Frequency = (Description->ddpfPixelFormat.dwFlags & DDSD_REFRESHRATE) ? Description->dwRefreshRate : 0;
	if (Width >= 640 && Height >= 480 && Depth >= 16) {
		TFullscreenMode *NewModes = new TFullscreenMode[GetSurface()->NumberOfModes + 1];
		TFullscreenMode *OldModes = *Modes;
		if (OldModes != NULL) {
			for (int i = 0; i < GetSurface()->NumberOfModes; i++)
				NewModes[i] = OldModes[i];
			delete[] OldModes;
		}
		NewModes[GetSurface()->NumberOfModes].Width = Width;
		NewModes[GetSurface()->NumberOfModes].Height = Height;
		NewModes[GetSurface()->NumberOfModes].Depth = Depth;
		NewModes[GetSurface()->NumberOfModes].Frequency = Frequency;
		GetSurface()->NumberOfModes++;
		*Modes = NewModes;
	}
	return DDENUMRET_OK;
}

// 0x4f1320. The caller deletes the array.
int TSurfaceDX9::GetFullscreenModes(TFullscreenMode **Modes)
{
	*Modes = NULL;
	if (DirectDraw->EnumDisplayModes(0, NULL, Modes, EnumModesCallback) == DD_OK)
		return NumberOfModes;
	if (*Modes != NULL)
		delete[] *Modes;
	*Modes = NULL;
	return 0;
}

// 0x4f1370. Odd behaviour kept: SurfaceArea.left is set only for surface 0,
// its other members always (an else without braces, by the look of it).
void TSurfaceDX9::SetSurfaceRect(int Surface, int x, int y, int Width, int Height)
{
	if (Surface != 0) {
		Rect[Surface].left = x;
		Rect[Surface].top = y;
		Rect[Surface].right = x + Width;
		Rect[Surface].bottom = y + Height;
	} else
		SurfaceArea.left = x;
	SurfaceArea.top = y;
	SurfaceArea.right = x + Width;
	SurfaceArea.bottom = y + Height;
}

// 0x4f13e0 (shared with SurfaceDX.cpp)
bool TSurfaceDX9::Unknown9C(int Value)
{
	return Ready;
}

// 0x4f13f0
bool TSurfaceDX9::CanShadeRectangles()
{
	return Direct3D != NULL;
}

// 0x4f1400
void TSurfaceDX9::ExitSurfaces()
{
	ExitDevice();
	if (DirectDraw != NULL) {
		DirectDraw->Release();
		DirectDraw = NULL;
	}
}

// Inline (CreateSurfaceDX9 has its code).
TSurfaceDX9::TSurfaceDX9()
{
	VertexBuffer = NULL;
	ShadeBuffer = NULL;
	Direct3D = NULL;
	Device = NULL;
	ScreenTexture = NULL;
}

// 0x4f1430. GetSurface creates the DirectX 9 surface with it.
TSurface *CreateSurfaceDX9()
{
	return new TSurfaceDX9;
}

// 0x4f14c0. Six vertices of a textured quad (white unless the colours say
// otherwise); a full buffer is drawn.
void TSurfaceDX9::AddQuad(int Left, int Top, int Right, int Bottom, double Left_u, double Right_u, double Top_v,
	double Bottom_v, D3DCOLOR TopLeft, D3DCOLOR TopRight, D3DCOLOR BottomRight, D3DCOLOR BottomLeft)
{
	TVertexDX9 *V = &Vertices[NumberOfVertices];
	V[0].x = (float)Left; V[0].y = (float)Top; V[0].z = 1.0f; V[0].rhw = 1.0f; V[0].Color = TopLeft;
	V[0].u = (float)Left_u; V[0].v = (float)Top_v;
	V[1].x = (float)Right; V[1].y = (float)Top; V[1].z = 1.0f; V[1].rhw = 1.0f; V[1].Color = TopRight;
	V[1].u = (float)Right_u; V[1].v = (float)Top_v;
	V[2].x = (float)Right; V[2].y = (float)Bottom; V[2].z = 1.0f; V[2].rhw = 1.0f; V[2].Color = BottomRight;
	V[2].u = (float)Right_u; V[2].v = (float)Bottom_v;
	V[3].x = (float)Right; V[3].y = (float)Bottom; V[3].z = 1.0f; V[3].rhw = 1.0f; V[3].Color = BottomRight;
	V[3].u = (float)Right_u; V[3].v = (float)Bottom_v;
	V[4].x = (float)Left; V[4].y = (float)Bottom; V[4].z = 1.0f; V[4].rhw = 1.0f; V[4].Color = BottomLeft;
	V[4].u = (float)Left_u; V[4].v = (float)Bottom_v;
	V[5].x = (float)Left; V[5].y = (float)Top; V[5].z = 1.0f; V[5].rhw = 1.0f; V[5].Color = TopLeft;
	V[5].u = (float)Left_u; V[5].v = (float)Top_v;
	NumberOfVertices += 6;
	if (NumberOfVertices == 9600)
		FlushVertices();
}

// 0x4f18d0. StretchBlit without clipping; an unscaled one is a Blit. The
// colours are not used for a scaled one.
void TSurfaceDX9::StretchBlitUnclipped(int ToSurface, int x, int y, int Width, int Height, int FromSurface,
	int FromX, int FromY, int FromWidth, int FromHeight, TRGBColor ColorKey, TRGBColor Highlight)
{
	if (Width == FromWidth && Height == FromHeight) {
		Blit(ToSurface, x, y, FromSurface, FromX, FromY, FromWidth, FromHeight, ColorKey);
		return;
	}
	int Right = x + Width;
	int Bottom = y + Height;
	int FromRight = FromX + FromWidth;
	int FromBottom = FromY + FromHeight;
	SetRenderTarget(ToSurface);
	SetTexture(FromSurface);
	AddQuad(x, y, Right, Bottom, (FromX + 0.5) / Rect[FromSurface].right,
		(FromRight + 0.5) / Rect[FromSurface].right, (FromY + 0.5) / Rect[FromSurface].bottom,
		(FromBottom + 0.5) / Rect[FromSurface].bottom, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff);
}

// 0x4f1a40
void TSurfaceDX9::StretchBlitRect(int ToSurface, TRect Destination, int FromSurface, TRect Source)
{
	SetRenderTarget(ToSurface);
	SetTexture(FromSurface);
	AddQuad(Destination.x, Destination.y, Destination.x + Destination.Width, Destination.y + Destination.Height,
		(Source.x + 0.5) / Rect[FromSurface].right, (Source.x + Source.Width + 0.5) / Rect[FromSurface].right,
		(Source.y + 0.5) / Rect[FromSurface].bottom, (Source.y + Source.Height + 0.5) / Rect[FromSurface].bottom,
		0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff);
	FrameBlits++;
	FramePixels += Destination.Height * Destination.Width;
}

// 0x4f1b70. ScreenTexture (SCREEN_SURFACE) is always mapped as a whole.
void TSurfaceDX9::BlitRectClipped(int ToSurface, int x, int y, int FromSurface, TRect Source, TRect Clip)
{
	RECT To = {x, y, x + Source.Width, y + Source.Height};
	RECT From = {Source.x, Source.y, Source.x + Source.Width, Source.y + Source.Height};
	if (ClipRect(Clip, &To, &From)) {
		SetRenderTarget(ToSurface);
		SetTexture(FromSurface);
		double Left_u, Right_u, Top_v, Bottom_v;
		if (FromSurface == SCREEN_SURFACE) {
			Left_u = 0.0;
			Right_u = 1.0;
			Top_v = 0.0;
			Bottom_v = 1.0;
		} else {
			Left_u = (From.left + 0.5) / Rect[FromSurface].right;
			Right_u = (From.right + 0.5) / Rect[FromSurface].right;
			Top_v = (From.top + 0.5) / Rect[FromSurface].bottom;
			Bottom_v = (From.bottom + 0.5) / Rect[FromSurface].bottom;
		}
		AddQuad(To.left, To.top, To.right, To.bottom, Left_u, Right_u, Top_v, Bottom_v,
			0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff);
		FrameBlits++;
		FramePixels += Source.Height * Source.Width;
	}
}

// 0x4f1cd0. The start state: no window, no device; the surface rectangles,
// the cache surface size 512x512, DirectDraw 4 for the modes and the stretch
// capabilities, the frame clock and the limit from the options. Odd
// behaviour kept: the first DirectDraw object is never released.
void TSurfaceDX9::InitSurfaces()
{
	hWindow = NULL;
	ViewMode = 0;
	DirectDraw = NULL;
	Direct3D = NULL;
	Device = NULL;
	CacheSurfaceWidth = 512;
	CacheSurfaceHeight = 512;
	CurrentTarget = NULL;
	for (int i = 0; i < NUMBER_OF_SURFACES_DX9; i++) {
		Surface9[i] = NULL;
		RenderTarget[i] = NULL;
		SurfaceCreated[i] = 0;
		ColorKey[i] = 0;
	}
	SetRect(&Rect[TEMP_SURFACE], 0, 0, 640, 480);
	SetRect(&Rect[STRETCH_SURFACE], 0, 0, 64, 64);
	SetRect(&Rect[CACHE_SURFACE], 0, 0, 512, 384);
	SetRect(&Rect[AUTOMAP_SURFACE], 0, 0, 106, 106);
	for (int i = SPRITE_SURFACE; i < NUMBER_OF_SURFACES_DX9; i++) {
		Rect[i].left = 0;
		Rect[i].top = 0;
		Rect[i].right = CacheSurfaceWidth;
		Rect[i].bottom = CacheSurfaceHeight;
	}
	IDirectDraw *DirectDraw1;
	if (DirectDrawCreate(NULL, &DirectDraw1, NULL) != DD_OK)
		throw (PROGRAM_ERROR)7;
	if (DirectDraw1->QueryInterface(IID_IDirectDraw4, (void **)&DirectDraw) != DD_OK)
		throw (PROGRAM_ERROR)7;
	DDCAPS DriverCaps, EmulationCaps;
	memset(&DriverCaps, 0, sizeof(DriverCaps));
	DriverCaps.dwSize = sizeof(DDCAPS);
	memset(&EmulationCaps, 0, sizeof(EmulationCaps));
	EmulationCaps.dwSize = sizeof(DDCAPS);
	if (DirectDraw->GetCaps(&DriverCaps, &EmulationCaps) == DD_OK) {
		if ((DriverCaps.dwFXCaps & DDFXCAPS_BLTSHRINKX) && (DriverCaps.dwFXCaps & DDFXCAPS_BLTSTRETCHX)
				&& (DriverCaps.dwFXCaps & DDFXCAPS_BLTSHRINKY) && (DriverCaps.dwFXCaps & DDFXCAPS_BLTSTRETCHY))
			CanStretch = true;
		else
			CanStretch = false;
	} else {
		CanStretch = false;
	}
	SetDefaultNoStretching(CanStretch == false);
	InitFramerate();
	SetFramerateLimit(GetConfineFramerate());
}

// 0x4f1eb0, line 496
void TSurfaceDX9::SetColorKey(int ToSurface, TRGBColor Key)
{
	TASSERT1(In(ToSurface,0,NUMBER_OF_SURFACES_DX9-1), ToSurface);
	ColorKey[ToSurface] = D3DCOLOR_XRGB(Key.R, Key.G, Key.B);
}

// 0x4f2040, lines 982-983. A 32x32 sprite with a colour mask: with a mask,
// the pixels whose mask is yellow, red, green or blue become the grey value
// of the pixel tinted with Colors[0-3]; without one, every pixel that is not
// Colors[1] is tinted with Colors[0]. The other pixels are copied, opaque
// unless they are the surface's colour key.
void TSurfaceDX9::WriteMaskedImage(int ToSurface, int x, int y, int Width, int Height, uint8 *Data,
	uint8 *Mask, TRGBColor *Colors)
{
	TASSERT(ToSurface!=SCREEN_SURFACE);
	TASSERT(x>=0 && y>=0 && Width>0 && Height>0);
	int Size = Width * Height;
	uint8 *Image = new uint8[Size * 4];
	uint8 *Pixel = Image;
	for (int i = 0; i < Size; i++) {
		uint8 *From = &Data[i * 3];
		int Index;
		if (Mask != NULL) {
			uint8 *M = &Mask[i * 3];
			if (M[0] == 0xff) {
				if (M[1] == 0xff) {
					if (M[2] != 0)
						goto Copy;
					Index = 0;
				} else {
					if (M[1] != 0 || M[2] != 0)
						goto Copy;
					Index = 1;
				}
			} else {
				if (M[0] != 0)
					goto Copy;
				if (M[1] == 0xff) {
					if (M[2] != 0)
						goto Copy;
					Index = 2;
				} else {
					if (M[1] != 0 || M[2] != 0xff)
						goto Copy;
					Index = 3;
				}
			}
		} else {
			if (From[0] == Colors[1].R && From[1] == Colors[1].G && From[2] == Colors[1].B)
				goto Copy;
			Index = 0;
		}
		{
			int Grey = (From[1] * 59 + From[0] * 30 + From[2] * 11) / 100;
			TRGBColor *C = &Colors[Index];
			Pixel[0] = (uint8)(C->R * Grey / 256);
			Pixel[1] = (uint8)(C->G * Grey / 256);
			Pixel[2] = (uint8)(C->B * Grey / 256);
			Pixel[3] = 0xff;
		}
		goto Next;
Copy:
		Pixel[0] = From[0];
		Pixel[1] = From[1];
		Pixel[2] = From[2];
		Pixel[3] = D3DCOLOR_XRGB(From[0], From[1], From[2]) != ColorKey[ToSurface] ? 0xff : 0;
Next:
		Pixel += 4;
	}
	D3DLOCKED_RECT Locked;
	Surface9[ToSurface]->LockRect(0, &Locked, &Rect[ToSurface], D3DLOCK_NOSYSLOCK);
	D3DCOLOR *Bits = (D3DCOLOR *)Locked.pBits;
	Pixel = Image;
	for (int i = 0; i < Height; i++) {
		for (int j = 0; j < Width; j++) {
			Bits[Locked.Pitch / 4 * (i + y) + j + x] = Pixel[3] << 24 | Pixel[0] << 16 | Pixel[1] << 8 | Pixel[2];
			Pixel += 4;
		}
	}
	Surface9[ToSurface]->UnlockRect(0);
	delete[] Image;
}

// 0x4f2620, lines 1199-1204. A scaled blit goes through TEMP_SURFACE (the
// source with a one-pixel border of Highlight, or skin 185 while Flag26 is
// set) and STRETCH_SURFACE; Highlight is the colour key of the result.
void TSurfaceDX9::StretchBlit(int ToSurface, int x, int y, int Width, int Height, int FromSurface, int FromX,
	int FromY, int FromWidth, int FromHeight, TRGBColor ColorKey, TRGBColor Highlight, TRect Clip)
{
	if (Width == FromWidth && Height == FromHeight) {
		BlitClipped(ToSurface, x, y, FromSurface, FromX, FromY, FromWidth, FromHeight, ColorKey, Clip);
		return;
	}
	TASSERT(FromSurface!=TEMP_SURFACE);
	TASSERT(FromSurface!=STRETCH_SURFACE);
	TASSERT(ToSurface!=TEMP_SURFACE);
	TASSERT(ToSurface!=STRETCH_SURFACE);
	TASSERT_TEXT(FromWidth + 1 <= Rect[TEMP_SURFACE].right && FromHeight + 1 <= Rect[TEMP_SURFACE].bottom,
		"FromRect.width+1<=Rect[TEMP_SURFACE].right && FromRect.height+1<=Rect[TEMP_SURFACE].bottom");
	TASSERT_TEXT(Width <= Rect[STRETCH_SURFACE].right && Height <= Rect[STRETCH_SURFACE].bottom,
		"ToRect.width<=Rect[STRETCH_SURFACE].right && ToRect.height<=Rect[STRETCH_SURFACE].bottom");
	if (Flag26)
		DrawSkin(TEMP_SURFACE, 0, 0, FromWidth + 1, FromHeight + 1, 185, 0, 0);
	else
		FillRectangle(TEMP_SURFACE, TRect(0, 0, FromWidth + 1, FromHeight + 1), Highlight);
	Blit(TEMP_SURFACE, 0, 0, FromSurface, FromX, FromY, FromWidth, FromHeight, ColorKey);
	StretchBlitRect(STRETCH_SURFACE, TRect(0, 0, Width, Height), TEMP_SURFACE, TRect(0, 0, FromWidth, FromHeight));
	if (!Flag26) {
		FlushVertices();
		ApplyColorKey(STRETCH_SURFACE, Highlight);
	}
	BlitClipped(ToSurface, x, y, STRETCH_SURFACE, 0, 0, Width, Height, Highlight, Clip);
}

// 0x4f2e20, line 1302. Clipped to the target surface.
void TSurfaceDX9::Blit(int ToSurface, int x, int y, int FromSurface, int FromX, int FromY, int Width,
	int Height, TRGBColor ColorKey)
{
	TASSERT2_TEXT(Width > 0 && Height > 0, "FromRect.width>0 && FromRect.height>0",
		"FromRect.width", Width, "FromRect.height", Height);
	TRect Clip;
	GetSurfaceRect(ToSurface, &Clip);
	BlitClipped(ToSurface, x, y, FromSurface, FromX, FromY, Width, Height, ColorKey, Clip);
}

// 0x4f3040, line 1315
void TSurfaceDX9::BlitRect(int ToSurface, int x, int y, int FromSurface, TRect Source)
{
	TASSERT2_TEXT(Source.Width > 0 && Source.Height > 0, "FromRect.width>0 && FromRect.height>0",
		"FromRect.width", Source.Width, "FromRect.height", Source.Height);
	TRect Clip;
	GetSurfaceRect(ToSurface, &Clip);
	BlitRectClipped(ToSurface, x, y, FromSurface, Source, Clip);
}

// 0x4f3240, lines 1465-1468. Full-screen mode takes GMode's size if
// DirectDraw lists it, otherwise the last mode listed (written back into
// GMode, without its frequency). Odd behaviour kept: with no modes at all it
// reads before the (NULL) array.
void TSurfaceDX9::SetDisplayMode(int VMode, TFullscreenMode *GMode)
{
	TASSERT(DirectDraw!=NULL);
	TASSERT(hWindow!=NULL);
	TASSERT(VMode==WINDOWED_MODE || VMode==FULLSCREEN_MODE);
	TASSERT(VMode==WINDOWED_MODE || GMode!=NULL);
	if (VMode == WINDOWED_MODE) {
		GetClientRect(hWindow, &Rect[PRIMARY_SURFACE]);
		GetClientRect(hWindow, &Rect[SCREEN_SURFACE]);
		GetClientRect(hWindow, &WindowRect);
		ClientToScreen(hWindow, (POINT *)&WindowRect.left);
		ClientToScreen(hWindow, (POINT *)&WindowRect.right);
	} else {
		TFullscreenMode *Modes;
		int Number = GetFullscreenModes(&Modes);
		int i;
		for (i = 0; i < Number; i++) {
			if (GMode->Width == Modes[i].Width && GMode->Height == Modes[i].Height)
				break;
		}
		if (i == Number) {
			GMode->Width = Modes[Number - 1].Width;
			GMode->Height = Modes[Number - 1].Height;
			GMode->Depth = Modes[Number - 1].Depth;
		}
		if (Modes != NULL)
			delete[] Modes;
		ModeWidth = GMode->Width;
		ModeHeight = GMode->Height;
		ModeDepth = GMode->Depth;
		SetRect(&Rect[PRIMARY_SURFACE], 0, 0, ModeWidth, ModeHeight);
		SetRect(&Rect[SCREEN_SURFACE], 0, 0, ModeWidth, ModeHeight);
		SetRect(&WindowRect, 0, 0, ModeWidth, ModeHeight);
	}
	ViewMode = VMode;
}

// 0x4f3740, lines 1639-1647. Creates Direct3D, the device for ViewMode and
// every surface. PROGRAM_ERRORs: 8 below 16 bits per pixel on the desktop,
// 12 without Direct3D or device, 14-17 for the first textures, 18 for a
// video cache texture. Odd behaviour kept: a failed alpha blending state
// returns without the rest (and without an error); the device's other states
// are not checked; the description for GetDisplayMode has only its size set;
// the video cache loop has a second way out for textures from the 25th on,
// which cannot happen with 16.
void TSurfaceDX9::InitDevice()
{
	TASSERT(DirectDraw!=NULL);
	TASSERT(Direct3D==NULL);
	TASSERT(Surface9[PRIMARY_SURFACE]==NULL);
	TASSERT(Surface9[SCREEN_SURFACE]==NULL);
	TASSERT(Surface9[TEMP_SURFACE]==NULL);
	TASSERT(Surface9[CACHE_SURFACE]==NULL);
	TASSERT(Surface9[AUTOMAP_SURFACE]==NULL);
	TASSERT(Surface9[SPRITE_SURFACE]==NULL);
	TASSERT(ViewMode==WINDOWED_MODE || ViewMode==FULLSCREEN_MODE);
	NumberOfVideoSurfaces = 0;
	DDSURFACEDESC2 Description;
	Description.dwSize = sizeof(DDSURFACEDESC2);
	DirectDraw->GetDisplayMode(&Description);
	if (Description.ddpfPixelFormat.dwRGBBitCount < 16)
		throw (PROGRAM_ERROR)8;
	if (Direct3DCreate9Function == NULL) {
		IsDirect3D9Installed();
		if (Direct3DCreate9Function == NULL)
			throw (PROGRAM_ERROR)12;
	}
	Direct3D = Direct3DCreate9Function(D3D_SDK_VERSION);
	if (Direct3D == NULL) {
		Direct3D = NULL;
		DAT_0071d0bc = 1;
		throw (PROGRAM_ERROR)12;
	}
	memset(&PresentParameters, 0, sizeof(PresentParameters));
	if (ViewMode == WINDOWED_MODE) {
		PresentParameters.Windowed = TRUE;
		PresentParameters.SwapEffect = D3DSWAPEFFECT_COPY;
		PresentParameters.BackBufferFormat = D3DFMT_UNKNOWN;
		PresentParameters.BackBufferWidth = GetSystemMetrics(SM_CXSCREEN);
		PresentParameters.BackBufferHeight = GetSystemMetrics(SM_CYSCREEN);
		PresentParameters.MultiSampleQuality = 0;
		PresentParameters.MultiSampleType = D3DMULTISAMPLE_NONE;
		PresentParameters.BackBufferCount = 1;
		PresentParameters.hDeviceWindow = hWindow;
		PresentParameters.EnableAutoDepthStencil = FALSE;
		PresentParameters.Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
		PresentParameters.FullScreen_RefreshRateInHz = 0;
		PresentParameters.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
	} else if (ViewMode == FULLSCREEN_MODE) {
		PresentParameters.Windowed = FALSE;
		PresentParameters.SwapEffect = D3DSWAPEFFECT_COPY;
		PresentParameters.BackBufferFormat = D3DFMT_A8R8G8B8;
		PresentParameters.BackBufferWidth = ModeWidth;
		PresentParameters.BackBufferHeight = ModeHeight;
		PresentParameters.MultiSampleQuality = 0;
		PresentParameters.MultiSampleType = D3DMULTISAMPLE_NONE;
		PresentParameters.BackBufferCount = 1;
		PresentParameters.hDeviceWindow = hWindow;
		PresentParameters.EnableAutoDepthStencil = FALSE;
		PresentParameters.Flags = D3DPRESENTFLAG_LOCKABLE_BACKBUFFER;
		PresentParameters.FullScreen_RefreshRateInHz = 0;
		PresentParameters.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
	}
	if (Direct3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWindow, D3DCREATE_HARDWARE_VERTEXPROCESSING,
			&PresentParameters, &Device) != D3D_OK || Device == NULL)
		throw (PROGRAM_ERROR)12;
	Device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &RenderTarget[SCREEN_SURFACE]);
	Device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	Device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	if (Device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE) != D3D_OK)
		return;
	if (Device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA) != D3D_OK)
		return;
	if (Device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA) != D3D_OK)
		return;
	Device->SetRenderState(D3DRS_LIGHTING, FALSE);
	Device->SetRenderState(D3DRS_ZENABLE, FALSE);
	Device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
	Device->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
	Device->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
	Device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
	Device->CreateVertexBuffer(9600 * sizeof(TVertexDX9), D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY,
		D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1, D3DPOOL_DEFAULT, &VertexBuffer, NULL);
	Device->CreateVertexBuffer(9600 * sizeof(TVertexDX9), D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY,
		D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1, D3DPOOL_DEFAULT, &ShadeBuffer, NULL);
	Device->CreateOffscreenPlainSurface(64, 64, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &TempSurface, NULL);

	if (Device->CreateTexture(Rect[TEMP_SURFACE].right, Rect[TEMP_SURFACE].bottom, 1, D3DUSAGE_RENDERTARGET,
			D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &Surface9[PRIMARY_SURFACE], NULL) != D3D_OK)
		throw (PROGRAM_ERROR)14;
	SurfaceCreated[NumberOfVideoSurfaces++] = 1;
	Surface9[PRIMARY_SURFACE]->GetSurfaceLevel(0, &RenderTarget[PRIMARY_SURFACE]);
	Device->ColorFill(RenderTarget[PRIMARY_SURFACE], NULL, 0xffff0000);

	if (Device->CreateTexture(106, 106, 1, D3DUSAGE_DYNAMIC, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &ScreenTexture,
			NULL) != D3D_OK)
		throw (PROGRAM_ERROR)14;
	SurfaceCreated[NumberOfVideoSurfaces++] = 1;

	if (Device->CreateTexture(Rect[TEMP_SURFACE].right, Rect[TEMP_SURFACE].bottom, 1, D3DUSAGE_RENDERTARGET,
			D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &Surface9[TEMP_SURFACE], NULL) != D3D_OK)
		throw (PROGRAM_ERROR)14;
	SurfaceCreated[NumberOfVideoSurfaces++] = 1;
	Surface9[TEMP_SURFACE]->GetSurfaceLevel(0, &RenderTarget[TEMP_SURFACE]);

	if (Device->CreateTexture(Rect[STRETCH_SURFACE].right, Rect[STRETCH_SURFACE].bottom, 1, D3DUSAGE_RENDERTARGET,
			D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &Surface9[STRETCH_SURFACE], NULL) != D3D_OK)
		throw (PROGRAM_ERROR)15;
	SurfaceCreated[NumberOfVideoSurfaces++] = 1;
	Surface9[STRETCH_SURFACE]->GetSurfaceLevel(0, &RenderTarget[STRETCH_SURFACE]);

	if (Device->CreateTexture(Rect[CACHE_SURFACE].right, Rect[CACHE_SURFACE].bottom, 1, D3DUSAGE_RENDERTARGET,
			D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &Surface9[CACHE_SURFACE], NULL) != D3D_OK)
		throw (PROGRAM_ERROR)16;
	SurfaceCreated[NumberOfVideoSurfaces++] = 1;
	Surface9[CACHE_SURFACE]->GetSurfaceLevel(0, &RenderTarget[CACHE_SURFACE]);

	if (Device->CreateTexture(Rect[AUTOMAP_SURFACE].right, Rect[AUTOMAP_SURFACE].bottom, 1, D3DUSAGE_RENDERTARGET,
			D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &Surface9[AUTOMAP_SURFACE], NULL) != D3D_OK)
		throw (PROGRAM_ERROR)17;
	SurfaceCreated[NumberOfVideoSurfaces++] = 1;
	Surface9[AUTOMAP_SURFACE]->GetSurfaceLevel(0, &RenderTarget[AUTOMAP_SURFACE]);

	for (int i = 0; i < 16; i++) {
		int Surface = SPRITE_SURFACE + i;
		if (Device->CreateTexture(Rect[Surface].right, Rect[Surface].bottom, 1, D3DUSAGE_DYNAMIC,
				D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &Surface9[Surface], NULL) != D3D_OK) {
			if (i < 24)
				throw (PROGRAM_ERROR)18;
			break;
		}
		SurfaceCreated[NumberOfVideoSurfaces++] = 1;
		Surface9[Surface]->GetSurfaceLevel(0, &RenderTarget[Surface]);
		NumberOfCacheSurfaces++;
	}
	Lost = false;
	Ready = true;
	VertexBuffer->Lock(0, 0, (void **)&Vertices, D3DLOCK_DISCARD);
	Device->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1);
	Device->SetStreamSource(0, VertexBuffer, 0, sizeof(TVertexDX9));
}

// 0x4f4560, line 2055
void TSurfaceDX9::GetSurfaceRect(int Surface, TRect *Rect)
{
	TASSERTB1(In(Surface,0,NUMBER_OF_SURFACES_DX9-1), Surface, "[bug0000290]");
	Rect->x = this->Rect[Surface].left;
	Rect->y = this->Rect[Surface].top;
	Rect->Width = this->Rect[Surface].right - this->Rect[Surface].left;
	Rect->Height = this->Rect[Surface].bottom - this->Rect[Surface].top;
}

// The slots whose code the linker took from another object (identical
// copies): SurfaceGL.obj's 0x4fa2c0, 0x4fa110, 0x4fab70, 0x4fb5e0, 0x4fb5f0,
// 0x4f9d00 and SurfaceDX.obj's 0x4eb000.

// +0x20 (0x4fa2c0). A frame thinner than twice the line is filled.
void TSurfaceDX9::DrawFrameClipped(int Surface, TRect Rect, int LineWidth, TRGBColor Color, TRect Clip)
{
	if (Rect.Width < 2 * LineWidth || Rect.Height < 2 * LineWidth) {
		FillRectangleClipped(Surface, Rect, Color, Clip);
		return;
	}
	FillRectangleClipped(Surface, TRect(Rect.x, Rect.y, Rect.Width - LineWidth, LineWidth), Color, Clip);
	FillRectangleClipped(Surface, TRect(Rect.x + LineWidth, Rect.y + Rect.Height - LineWidth, Rect.Width - LineWidth,
		LineWidth), Color, Clip);
	FillRectangleClipped(Surface, TRect(Rect.x, Rect.y + LineWidth, LineWidth, Rect.Height - LineWidth), Color, Clip);
	FillRectangleClipped(Surface, TRect(Rect.x + Rect.Width - LineWidth, Rect.y, LineWidth, Rect.Height - LineWidth),
		Color, Clip);
}

// +0x24 (0x4fa110)
void TSurfaceDX9::DrawFrame(int Surface, TRect Rect, int LineWidth, TRGBColor Color)
{
	if (Rect.Width < 2 * LineWidth || Rect.Height < 2 * LineWidth) {
		FillRectangle(Surface, Rect, Color);
		return;
	}
	FillRectangle(Surface, TRect(Rect.x, Rect.y, Rect.Width - LineWidth, LineWidth), Color);
	FillRectangle(Surface, TRect(Rect.x + LineWidth, Rect.y + Rect.Height - LineWidth, Rect.Width - LineWidth,
		LineWidth), Color);
	FillRectangle(Surface, TRect(Rect.x, Rect.y + LineWidth, LineWidth, Rect.Height - LineWidth), Color);
	FillRectangle(Surface, TRect(Rect.x + Rect.Width - LineWidth, Rect.y, LineWidth, Rect.Height - LineWidth), Color);
}

// +0x3c (0x4fab70). The colour key is not used.
void TSurfaceDX9::BlitClipped(int ToSurface, int x, int y, int FromSurface, int FromX, int FromY, int Width,
	int Height, TRGBColor ColorKey, TRect Clip)
{
	BlitRectClipped(ToSurface, x, y, FromSurface, TRect(FromX, FromY, Width, Height), Clip);
}

// +0x90 (0x4fb5e0)
int TSurfaceDX9::GetCacheSurfaces()
{
	return NumberOfCacheSurfaces;
}

// +0xa0 (0x4fb5f0)
bool TSurfaceDX9::IsUnavailable()
{
	return Lost;
}

// +0xac (0x4eb000). Limits from 10 to 200 frames per second; others mean no
// limit (0xffff).
void TSurfaceDX9::SetFramerateLimit(int Limit)
{
	if (Limit < 10 || Limit >= 201)
		FramerateLimit = 0xffff;
	else
		FramerateLimit = Limit;
	FrameTime = 1000.0 / FramerateLimit;
}

// +0xb0 (0x4f9d00)
double TSurfaceDX9::GetFPS()
{
	return FPS;
}
