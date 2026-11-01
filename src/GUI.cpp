// GUI.cpp: reconstructed from Tibia.exe 7.72 (0x495480-0x4a29df; the
// 0x4a29e0 deleting destructor is TWindow's).
//
// The window system's base classes (TComponent, TElement, TWindow), skin
// drawing, the timers, the hotkey table and the skin properties. Functions
// without line evidence come first, in binary order (the gap before the
// first asserted function: the compiler-generated constructors of
// SkinProperties and its blocks, Container.h code and the small functions
// without exception frame), then the asserted ones in line order. The
// TComponent functions that the binary has only as inline copies in earlier
// objects (GetNext, GetParent, ...) are in GUI.h.

#include "GUI.h"
#include "Skins.h"
#include "sysinc.h"
#include "external.h"

#include <sstream>

// Data, in .bss order.
static int NumberOfHotkeys;                    // 0x5ccf98, used entries of Hotkeys
static vector<TTimer *> Timers(0, 0, 2, NULL); // 0x5ccfa4, initialiser 0x559e30 (constructor 0x4973f0)
TSkinProperties SkinProperties;                // 0x5ccfc8, initialiser 0x559e50 (constructor 0x496800)
static vector<THotkey> Hotkeys(0, 50, 10);     // 0x5ce4f0, initialiser 0x559e60 (constructor 0x4976f0)
// UpdateTimers' static LastTime is at 0x5ce518 (guard 0x5ce51c).

// --- Functions without exception frame (no line evidence) ------------------

// 0x496000
int TTimer::GetTime()
{
	return Remaining / 100;
}

// 0x496020
bool TTimer::IsExpired()
{
	return !(Active && Remaining != 0);
}

// 0x496040. The deleting destructor 0x496520 inlines it.
TComponent::~TComponent()
{
	if (DrawingList != NULL)
		delete[] DrawingList;
}

// 0x496060
void TComponent::SetPosition(int _OffsetX, int _OffsetY)
{
	OffsetX = _OffsetX;
	OffsetY = _OffsetY;
}

// 0x496080
TWindow::TWindow()
{
	FirstChild = NULL;
	Capture = NULL;
}

// 0x4960b0
bool TWindow::IsChild(TComponent *Child)
{
	for (TComponent *Search = FirstChild; Search != NULL; Search = Search->Next) {
		if (Search == Child)
			return true;
	}
	return false;
}

// 0x4960e0
TComponent *TWindow::GetLastChild()
{
	if (FirstChild == NULL)
		return NULL;
	TComponent *Search = FirstChild;
	while (Search->Next != NULL)
		Search = Search->Next;
	return Search;
}

// 0x496100
TElement::TElement()
{
}

// 0x496130. Identical code to any two-member constructor, so the linker may
// have folded others into it.
TCallback::TCallback(TComponent *Receiver, int Event)
{
	this->Receiver = Receiver;
	this->Event = Event;
}

// 0x496150. Called last by InitControl.
void InitHotkeys()
{
	NumberOfHotkeys = 0;
}

// 0x496160, the one copy the linker kept of many identical empty functions
// (folded). ExitControl calls this one first; TComponent::Arrange and
// PositionChanged and TElement::StopDragging have the same body.
void ExitHotkeys()
{
}

// --- Skins ----------------------------------------------------------------

// 0x497a00, lines 42-86. Tiles the skin over width x height; srcx and srcy
// cut that much off the first column and row. The assertions come before the
// try block (it starts at state 35).
void DrawSkin(int Surface, int x, int y, int width, int height, int skin, int srcx, int srcy)
{
	if (skin == 0 || width == 0 || height == 0)
		return;
	TASSERT1(x>=0, x);
	TASSERT1(y>=0, y);
	TASSERT1(In(skin,0,LASTSKIN-1), skin);
	TASSERT1(Skins[skin].width>0, Skins[skin].width);
	TASSERT1(Skins[skin].height>0, Skins[skin].height);
	TASSERT1(width>0, width);
	TASSERT1(height>0, height);
	TBEGIN
	if (Skins[skin].width == width && Skins[skin].height == height) {
		FUN_00490c60(Surface, x, y, 4, Skins[skin].x, Skins[skin].y, Skins[skin].width, Skins[skin].height);
	} else {
		int PieceHeight = 0;
		int dy = 0;
		for (int h = height; h > 0; h -= PieceHeight) {
			int dx = 0;
			int PieceWidth;
			for (int w = width; w > 0; w -= PieceWidth) {
				int SourceX, SourceY;
				if (w == width && srcx > 0) {
					PieceWidth = Skins[skin].width - srcx;
					SourceX = Skins[skin].x + srcx;
				} else {
					SourceX = Skins[skin].x;
					PieceWidth = Skins[skin].width;
				}
				if (w < PieceWidth)
					PieceWidth = w;
				if (h == height && srcy > 0) {
					SourceY = Skins[skin].y + srcy;
					PieceHeight = Skins[skin].height - srcy;
				} else {
					SourceY = Skins[skin].y;
					PieceHeight = Skins[skin].height;
				}
				if (PieceHeight > h)
					PieceHeight = h;
				FUN_00490c60(Surface, dx + x, dy + y, 4, SourceX, SourceY, PieceWidth, PieceHeight);
				dx += PieceWidth;
			}
			dy += PieceHeight;
		}
	}
	TEND1(skin)
}

// 0x4a1690, lines 99-160. DrawSkin with a clipping rectangle, which is
// passed on to the picture function. The assertions come before the try
// block (it starts at state 15).
void DrawSkinClipped(int Surface, int x, int y, int width, int height, int skin, int ClipX, int ClipY,
	int ClipWidth, int ClipHeight, int skinoffsetx, int skinoffsety)
{
	if (skin == 0 || width == 0 || height == 0)
		return;
	TASSERT1(In(skin,0,LASTSKIN-1), skin);
	TASSERT1(width>0, width);
	TASSERT1(height>0, height);
	TBEGIN
	if (Skins[skin].width < 1 || Skins[skin].height < 1) {
		std::stringstream s;
		s << "some assertions failed " << "(surface: " << Surface << " x: " << x << " y: " << y
			<< " width: " << width << " height: " << height << " skin: " << skin
			<< " skinwidth: " << Skins[skin].width << " skinheight: " << Skins[skin].height
			<< " skinoffsetx: " << skinoffsetx << " skinoffsety: " << skinoffsety
			<< " cliprect: " << ClipX << "/" << ClipY << "/" << ClipWidth << "/" << ClipHeight << ")";
		TERROR(s.str().c_str());
	}
	if (Skins[skin].width == width && Skins[skin].height == height) {
		FUN_004910e0(Surface, x, y, 4, Skins[skin].x, Skins[skin].y, Skins[skin].width, Skins[skin].height,
			ClipX, ClipY, ClipWidth, ClipHeight);
	} else {
		int PieceHeight = 0;
		int dy = 0;
		for (int h = height; h > 0; h -= PieceHeight) {
			int dx = 0;
			int PieceWidth;
			for (int w = width; w > 0; w -= PieceWidth) {
				int SourceX, SourceY;
				if (w == width && skinoffsetx > 0) {
					PieceWidth = Skins[skin].width - skinoffsetx;
					SourceX = Skins[skin].x + skinoffsetx;
				} else {
					PieceWidth = Skins[skin].width;
					SourceX = Skins[skin].x;
				}
				if (w < PieceWidth)
					PieceWidth = w;
				if (h == height && skinoffsety > 0) {
					SourceY = Skins[skin].y + skinoffsety;
					PieceHeight = Skins[skin].height - skinoffsety;
				} else {
					SourceY = Skins[skin].y;
					PieceHeight = Skins[skin].height;
				}
				if (PieceHeight > h)
					PieceHeight = h;
				FUN_004910e0(Surface, dx + x, dy + y, 4, SourceX, SourceY, PieceWidth, PieceHeight,
					ClipX, ClipY, ClipWidth, ClipHeight);
				dx += PieceWidth;
			}
			dy += PieceHeight;
		}
	}
	TEND1(skin)
}

// --- Skin properties ------------------------------------------------------

// 0x498160. Called by InitControl after InitSkins. The exception tables have
// the states of a TBEGIN/TEND but no try block (the compiler removed it: the
// body cannot throw); the catch line is unknown. The binary stores the
// values in another order; they are written here by offset.
void InitSkinProperties()
{
	TBEGIN
	SkinProperties.CursorHotspot1.x = 0;
	SkinProperties.CursorHotspot1.y = 0;
	SkinProperties.CursorHotspot2.x = 4;
	SkinProperties.CursorHotspot2.y = 9;
	SkinProperties.CursorHotspot3.x = 6;
	SkinProperties.CursorHotspot3.y = 10;
	SkinProperties.CursorHotspot4.x = 9;
	SkinProperties.CursorHotspot4.y = 9;
	SkinProperties.CursorHotspot5.x = 9;
	SkinProperties.CursorHotspot5.y = 9;
	SkinProperties.CursorHotspot6.x = 6;
	SkinProperties.CursorHotspot6.y = 6;
	SkinProperties.TooltipFont.FontNumber = 1;
	SkinProperties.TooltipFont.Color.R = 64;
	SkinProperties.TooltipFont.Color.G = 64;
	SkinProperties.TooltipFont.Color.B = 64;
	SkinProperties.TooltipFont.Unknown10 = 0;
	SkinProperties.TooltipBorder = 4;
	SkinProperties.TooltipBackground.R = 192;
	SkinProperties.TooltipBackground.G = 192;
	SkinProperties.TooltipBackground.B = 192;
	SkinProperties.TooltipFrame.R = 0;
	SkinProperties.TooltipFrame.G = 0;
	SkinProperties.TooltipFrame.B = 0;
	SkinProperties.Data060.R = 248;
	SkinProperties.Data060.G = 248;
	SkinProperties.Data060.B = 248;
	SkinProperties.Data06C.x = 224;
	SkinProperties.Data06C.y = 64;
	SkinProperties.Data06C.Width = 64;
	SkinProperties.Data06C.Height = 64;
	SkinProperties.Data07C.x = 224;
	SkinProperties.Data07C.y = 64;
	SkinProperties.Data07C.Width = 248;
	SkinProperties.Data07C.Height = 164;
	SkinProperties.Data08C.x = 164;
	SkinProperties.Data08C.y = 180;
	SkinProperties.Data08C.Width = 248;
	SkinProperties.Data08C.Height = 180;
	SkinProperties.Data09C = 40;
	SkinProperties.Data0A0.x = 4;
	SkinProperties.Data0A0.y = 4;
	SkinProperties.Data0A8.x = -4;
	SkinProperties.Data0A8.y = -4;
	SkinProperties.Data0B0.FontNumber = 2;
	SkinProperties.Data0B0.Color.R = 224;
	SkinProperties.Data0B0.Color.G = 224;
	SkinProperties.Data0B0.Color.B = 224;
	SkinProperties.Data0B0.Unknown10 = 0;
	SkinProperties.Data0C4 = 8;
	SkinProperties.Data0C8.R = 128;
	SkinProperties.Data0C8.G = 128;
	SkinProperties.Data0C8.B = 128;
	SkinProperties.Data0D4 = 2;
	SkinProperties.Style0D8.Data00 = 86;
	SkinProperties.Style0D8.Data04 = 140;
	SkinProperties.Style0D8.Data08.x = 60;
	SkinProperties.Style0D8.Data08.y = -240;
	SkinProperties.Style0D8.Data10.R = 0;
	SkinProperties.Style0D8.Data10.G = 0;
	SkinProperties.Style0D8.Data10.B = 0;
	SkinProperties.Style0D8.Data1C.x = 60;
	SkinProperties.Style0D8.Data1C.y = 0;
	SkinProperties.Style0D8.Data24.x = 90;
	SkinProperties.Style0D8.Data24.y = 0;
	SkinProperties.Style0D8.Data2C.x = 120;
	SkinProperties.Style0D8.Data2C.y = 0;
	SkinProperties.Style0D8.Data34.x = 30;
	SkinProperties.Data134 = 10;
	SkinProperties.Data138 = 5;
	SkinProperties.Data13C = 5;
	SkinProperties.Data140.x = -35;
	SkinProperties.Data140.y = 5;
	SkinProperties.Data148.x = 32;
	SkinProperties.Data148.y = 32;
	SkinProperties.Data150 = 43;
	SkinProperties.Data154 = 43;
	SkinProperties.Data158 = 322;
	SkinProperties.Data15C = 447;
	SkinProperties.Data160.x = 4;
	SkinProperties.Data160.y = 4;
	SkinProperties.Data168.x = -4;
	SkinProperties.Data168.y = -4;
	SkinProperties.Data170 = 4;
	SkinProperties.Data174 = 3;
	SkinProperties.Data178 = 3;
	SkinProperties.Data17C = 400;
	SkinProperties.Data180 = 340;
	SkinProperties.Data184 = 176;
	SkinProperties.Data188.R = 0;
	SkinProperties.Data188.G = 0;
	SkinProperties.Data188.B = 0;
	SkinProperties.Style194.Data00.x = 4;
	SkinProperties.Style194.Data00.y = 2;
	SkinProperties.Style194.Data00.Width = 12;
	SkinProperties.Style194.Data00.Height = 12;
	SkinProperties.Style194.Data10.x = -15;
	SkinProperties.Style194.Data10.y = 2;
	SkinProperties.Style194.Data18.x = -28;
	SkinProperties.Style194.Data18.y = 2;
	SkinProperties.Style194.Data20.x = -39;
	SkinProperties.Style194.Data20.y = 2;
	SkinProperties.Style194.Data28.x = 20;
	SkinProperties.Style194.Data28.y = 4;
	SkinProperties.Style194.Data30.x = -41;
	SkinProperties.Style194.Data30.y = 10;
	SkinProperties.Style194.Data38.FontNumber = 4;
	SkinProperties.Style194.Data38.Color.R = 144;
	SkinProperties.Style194.Data38.Color.G = 144;
	SkinProperties.Style194.Data38.Color.B = 144;
	SkinProperties.Style194.Data38.Unknown10 = 0;
	SkinProperties.Style194.Data4C = 19;
	SkinProperties.Style194.Data50.x = 4;
	SkinProperties.Style194.Data50.y = 15;
	SkinProperties.Style194.Data58.x = -4;
	SkinProperties.Style194.Data58.y = -4;
	SkinProperties.Style194.Data60.x = 3;
	SkinProperties.Style194.Data60.y = -16;
	SkinProperties.Style194.Data68.x = 13;
	SkinProperties.Style194.Data68.y = 13;
	SkinProperties.Style194.Data70.x = 0;
	SkinProperties.Style194.Data70.y = -4;
	SkinProperties.Style194.Data78.x = 0;
	SkinProperties.Style194.Data78.y = 0;
	SkinProperties.Style194.Data80.R = 56;
	SkinProperties.Style194.Data80.G = 56;
	SkinProperties.Style194.Data80.B = 56;
	SkinProperties.Data220 = 117;
	SkinProperties.Data224.x = 8;
	SkinProperties.Data224.y = 4;
	SkinProperties.Data224.Width = 108;
	SkinProperties.Data224.Height = 108;
	SkinProperties.Data234.x = 9;
	SkinProperties.Data234.y = 5;
	SkinProperties.Data234.Width = 106;
	SkinProperties.Data234.Height = 106;
	SkinProperties.Data244.x = 124;
	SkinProperties.Data244.y = 71;
	SkinProperties.Data244.Width = 124;
	SkinProperties.Data244.Height = 50;
	SkinProperties.Data254.x = 147;
	SkinProperties.Data254.y = 50;
	SkinProperties.Data254.Width = 147;
	SkinProperties.Data254.Height = 71;
	SkinProperties.Data264.x = 124;
	SkinProperties.Data264.y = 92;
	SkinProperties.Data264.Width = 124;
	SkinProperties.Data264.Height = 5;
	SkinProperties.Data274 = 170;
	SkinProperties.Data278.x = 124;
	SkinProperties.Data278.y = 19;
	SkinProperties.Data278.Width = 8;
	SkinProperties.Data278.Height = 4;
	SkinProperties.Style288.Data00 = 108;
	SkinProperties.Style288.Data04 = 163;
	SkinProperties.Style288.Data08.x = 0;
	SkinProperties.Style288.Data08.y = 0;
	SkinProperties.Style288.Data10.x = 38;
	SkinProperties.Style288.Data10.y = 1;
	SkinProperties.Style288.Data10.Width = 1;
	SkinProperties.Style288.Data10.Height = 15;
	SkinProperties.Style288.Data20.x = 75;
	SkinProperties.Style288.Data20.y = 15;
	SkinProperties.Style288.Data20.Width = 38;
	SkinProperties.Style288.Data20.Height = 38;
	SkinProperties.Style288.Data30.x = 75;
	SkinProperties.Style288.Data30.y = 52;
	SkinProperties.Style288.Data30.Width = 1;
	SkinProperties.Style288.Data30.Height = 52;
	SkinProperties.Style288.Data40.x = 38;
	SkinProperties.Style288.Data40.y = 75;
	SkinProperties.Style288.Data40.Width = 38;
	SkinProperties.Style288.Data40.Height = 112;
	SkinProperties.Style288.Data50.x = 1;
	SkinProperties.Style288.Data50.y = 89;
	SkinProperties.Style288.Data50.Width = 75;
	SkinProperties.Style288.Data50.Height = 89;
	SkinProperties.Style288.Data60.x = 37;
	SkinProperties.Style288.Data60.y = 0;
	SkinProperties.Style288.Data60.Width = 34;
	SkinProperties.Style288.Data60.Height = 34;
	SkinProperties.Style288.Data70.x = 0;
	SkinProperties.Style288.Data70.y = 14;
	SkinProperties.Style288.Data70.Width = 34;
	SkinProperties.Style288.Data70.Height = 34;
	SkinProperties.Style288.Data80.x = 74;
	SkinProperties.Style288.Data80.y = 14;
	SkinProperties.Style288.Data80.Width = 34;
	SkinProperties.Style288.Data80.Height = 34;
	SkinProperties.Style288.Data90.x = 37;
	SkinProperties.Style288.Data90.y = 37;
	SkinProperties.Style288.Data90.Width = 34;
	SkinProperties.Style288.Data90.Height = 34;
	SkinProperties.Style288.DataA0.x = 74;
	SkinProperties.Style288.DataA0.y = 51;
	SkinProperties.Style288.DataA0.Width = 34;
	SkinProperties.Style288.DataA0.Height = 34;
	SkinProperties.Style288.DataB0.x = 0;
	SkinProperties.Style288.DataB0.y = 51;
	SkinProperties.Style288.DataB0.Width = 34;
	SkinProperties.Style288.DataB0.Height = 34;
	SkinProperties.Style288.DataC0.x = 37;
	SkinProperties.Style288.DataC0.y = 74;
	SkinProperties.Style288.DataC0.Width = 34;
	SkinProperties.Style288.DataC0.Height = 34;
	SkinProperties.Style288.DataD0.x = 37;
	SkinProperties.Style288.DataD0.y = 111;
	SkinProperties.Style288.DataD0.Width = 34;
	SkinProperties.Style288.DataD0.Height = 34;
	SkinProperties.Style288.DataE0.x = 0;
	SkinProperties.Style288.DataE0.y = 88;
	SkinProperties.Style288.DataE0.Width = 34;
	SkinProperties.Style288.DataE0.Height = 34;
	SkinProperties.Style288.DataF0.x = 74;
	SkinProperties.Style288.DataF0.y = 88;
	SkinProperties.Style288.DataF0.Width = 34;
	SkinProperties.Style288.DataF0.Height = 34;
	SkinProperties.Style288.Data100 = 88;
	SkinProperties.Style288.Data104 = 89;
	SkinProperties.Style288.Data108 = 90;
	SkinProperties.Style288.Data10C = 91;
	SkinProperties.Style288.Data110 = 93;
	SkinProperties.Style288.Data114 = 92;
	SkinProperties.Style288.Data118 = 94;
	SkinProperties.Style288.Data11C = 95;
	SkinProperties.Style288.Data120 = 96;
	SkinProperties.Style288.Data124 = 97;
	SkinProperties.Style288.Data128.x = 74;
	SkinProperties.Style288.Data128.y = 124;
	SkinProperties.Style288.Data128.Width = 34;
	SkinProperties.Style288.Data128.Height = 21;
	SkinProperties.Style288.Data138.x = 0;
	SkinProperties.Style288.Data138.y = 124;
	SkinProperties.Style288.Data138.Width = 34;
	SkinProperties.Style288.Data138.Height = 21;
	SkinProperties.Style288.Data148.x = 0;
	SkinProperties.Style288.Data148.y = 147;
	SkinProperties.Style288.Data148.Width = 108;
	SkinProperties.Style288.Data148.Height = 13;
	SkinProperties.Style3E0.Data00.x = 17;
	SkinProperties.Style3E0.Data00.y = 2;
	SkinProperties.Style3E0.Data00.Width = 32;
	SkinProperties.Style3E0.Data00.Height = 9;
	SkinProperties.Style3E0.Data10.FontNumber = 3;
	SkinProperties.Style3E0.Data10.Color.R = 255;
	SkinProperties.Style3E0.Data10.Color.G = 255;
	SkinProperties.Style3E0.Data10.Color.B = 255;
	SkinProperties.Style3E0.Data10.Unknown10 = 1;
	SkinProperties.Style3E0.Data24.x = 17;
	SkinProperties.Style3E0.Data24.y = 11;
	SkinProperties.Style3E0.Data24.Width = 32;
	SkinProperties.Style3E0.Data24.Height = 9;
	SkinProperties.Style3E0.Data34.FontNumber = 4;
	SkinProperties.Style3E0.Data34.Color.R = 176;
	SkinProperties.Style3E0.Data34.Color.G = 176;
	SkinProperties.Style3E0.Data34.Color.B = 176;
	SkinProperties.Style3E0.Data34.Unknown10 = 1;
	SkinProperties.Style428.Data00.x = 17;
	SkinProperties.Style428.Data00.y = 2;
	SkinProperties.Style428.Data00.Width = 32;
	SkinProperties.Style428.Data00.Height = 9;
	SkinProperties.Style428.Data10.FontNumber = 3;
	SkinProperties.Style428.Data10.Color.R = 255;
	SkinProperties.Style428.Data10.Color.G = 255;
	SkinProperties.Style428.Data10.Color.B = 255;
	SkinProperties.Style428.Data10.Unknown10 = 1;
	SkinProperties.Style428.Data24.x = 17;
	SkinProperties.Style428.Data24.y = 11;
	SkinProperties.Style428.Data24.Width = 32;
	SkinProperties.Style428.Data24.Height = 9;
	SkinProperties.Style428.Data34.FontNumber = 4;
	SkinProperties.Style428.Data34.Color.R = 176;
	SkinProperties.Style428.Data34.Color.G = 176;
	SkinProperties.Style428.Data34.Color.B = 176;
	SkinProperties.Style428.Data34.Unknown10 = 1;
	SkinProperties.Data470.x = 2;
	SkinProperties.Data470.y = 2;
	SkinProperties.Data470.Width = 104;
	SkinProperties.Data470.Height = 9;
	SkinProperties.Data480.x = 2;
	SkinProperties.Data480.y = 2;
	SkinProperties.Data480.Width = 90;
	SkinProperties.Data480.Height = 9;
	SkinProperties.Data490 = 9;
	SkinProperties.Data494 = 10;
	SkinProperties.Data498.FontNumber = 2;
	SkinProperties.Data498.Color.R = 192;
	SkinProperties.Data498.Color.G = 192;
	SkinProperties.Data498.Color.B = 192;
	SkinProperties.Data498.Unknown10 = 2;
	SkinProperties.Data4AC = 134;
	SkinProperties.Data4B0 = 43;
	SkinProperties.Data4B4.x = 0;
	SkinProperties.Data4B4.y = 0;
	SkinProperties.Data4BC.x = 0;
	SkinProperties.Data4BC.y = 20;
	SkinProperties.Data4C4.x = 0;
	SkinProperties.Data4C4.y = 40;
	SkinProperties.Data4CC.x = 23;
	SkinProperties.Data4CC.y = 0;
	SkinProperties.Data4D4.x = 23;
	SkinProperties.Data4D4.y = 20;
	SkinProperties.Data4D4.Width = 23;
	SkinProperties.Data4D4.Height = 40;
	SkinProperties.Data4E4.x = 0;
	SkinProperties.Data4E4.y = 62;
	SkinProperties.Data4EC.x = 0;
	SkinProperties.Data4EC.y = 86;
	SkinProperties.Data4F4.x = 0;
	SkinProperties.Data4F4.y = 108;
	SkinProperties.Data4FC = 42;
	SkinProperties.Data500 = 111;
	SkinProperties.Data504.x = 0;
	SkinProperties.Data504.y = 0;
	SkinProperties.Data50C.x = 20;
	SkinProperties.Data50C.y = 0;
	SkinProperties.Data514.x = 40;
	SkinProperties.Data514.y = 0;
	SkinProperties.Data51C.x = 0;
	SkinProperties.Data51C.y = 22;
	SkinProperties.Data524.x = 20;
	SkinProperties.Data524.y = 22;
	SkinProperties.Data524.Width = 40;
	SkinProperties.Data524.Height = 22;
	SkinProperties.Data534.x = 68;
	SkinProperties.Data534.y = 0;
	SkinProperties.Data53C.x = 68;
	SkinProperties.Data53C.y = 22;
	SkinProperties.Data53C.Width = 90;
	SkinProperties.Data53C.Height = 22;
	SkinProperties.Data54C = 64;
	SkinProperties.Data550.x = 56;
	SkinProperties.Data550.y = 4;
	SkinProperties.Data550.Width = 8;
	SkinProperties.Data550.Height = 4;
	SkinProperties.Data560.x = 22;
	SkinProperties.Data560.y = 4;
	SkinProperties.Data560.Width = 32;
	SkinProperties.Data560.Height = 21;
	SkinProperties.Data570.x = 22;
	SkinProperties.Data570.y = 25;
	SkinProperties.Data570.Width = 32;
	SkinProperties.Data570.Height = 21;
	SkinProperties.Data580.x = 22;
	SkinProperties.Data580.y = 48;
	SkinProperties.Data580.Width = 94;
	SkinProperties.Data580.Height = 13;
	SkinProperties.Data590.x = 22;
	SkinProperties.Data590.y = 4;
	SkinProperties.Data590.Width = 32;
	SkinProperties.Data590.Height = 42;
	SkinProperties.StatusWindow.Data00 = 32;
	SkinProperties.StatusWindow.Data04.x = 9;
	SkinProperties.StatusWindow.Data04.y = 5;
	SkinProperties.StatusWindow.Data04.Width = 9;
	SkinProperties.StatusWindow.Data04.Height = 18;
	SkinProperties.StatusWindow.HitpointsBar.x = 26;
	SkinProperties.StatusWindow.HitpointsBar.y = 4;
	SkinProperties.StatusWindow.HitpointsBar.Width = 90;
	SkinProperties.StatusWindow.HitpointsBar.Height = 11;
	SkinProperties.StatusWindow.ManaBar.x = 26;
	SkinProperties.StatusWindow.ManaBar.y = 17;
	SkinProperties.StatusWindow.ManaBar.Width = 90;
	SkinProperties.StatusWindow.ManaBar.Height = 11;
	SkinProperties.StatusWindow.Data34.x = 127;
	SkinProperties.StatusWindow.Data34.y = 5;
	SkinProperties.StatusWindow.Data34.Width = 39;
	SkinProperties.StatusWindow.Data34.Height = 9;
	SkinProperties.StatusWindow.Data44.x = 127;
	SkinProperties.StatusWindow.Data44.y = 18;
	SkinProperties.StatusWindow.Data44.Width = 39;
	SkinProperties.StatusWindow.Data44.Height = 9;
	SkinProperties.StatusWindow.Data54.FontNumber = 4;
	SkinProperties.StatusWindow.Data54.Color.R = 176;
	SkinProperties.StatusWindow.Data54.Color.G = 176;
	SkinProperties.StatusWindow.Data54.Color.B = 176;
	SkinProperties.StatusWindow.Data54.Unknown10 = 0;
	SkinProperties.Data608 = 26;
	SkinProperties.Data60C.x = 8;
	SkinProperties.Data60C.y = 3;
	SkinProperties.Data60C.Width = 45;
	SkinProperties.Data60C.Height = 3;
	SkinProperties.Data61C.x = 82;
	SkinProperties.Data61C.y = 3;
	SkinProperties.Data61C.Width = 124;
	SkinProperties.Data61C.Height = 3;
	SkinProperties.Data62C = 4;
	SkinProperties.Data630.x = 7;
	SkinProperties.Data630.y = 4;
	SkinProperties.Data638.x = 34;
	SkinProperties.Data638.y = 34;
	SkinProperties.Data638.Width = 8;
	SkinProperties.Data638.Height = 5;
	SkinProperties.Data648 = 37;
	SkinProperties.Data64C = 37;
	SkinProperties.Data650 = 41;
	SkinProperties.Data654.FontNumber = 4;
	SkinProperties.Data654.Color.R = 176;
	SkinProperties.Data654.Color.G = 176;
	SkinProperties.Data654.Color.B = 176;
	SkinProperties.Data654.Unknown10 = 0;
	SkinProperties.Data668 = 10;
	SkinProperties.Data66C = 10;
	SkinProperties.Data670.x = 192;
	SkinProperties.Data670.y = 0;
	SkinProperties.Data678.x = 0;
	SkinProperties.Data678.y = 0;
	SkinProperties.Data680.x = 192;
	SkinProperties.Data680.y = 0;
	SkinProperties.Style688.Data00 = 22;
	SkinProperties.Style688.Data04.x = 2;
	SkinProperties.Style688.Data04.y = 2;
	SkinProperties.Style688.Data04.Width = 18;
	SkinProperties.Style688.Data04.Height = 18;
	SkinProperties.Style688.Data14.x = 23;
	SkinProperties.Style688.Data14.y = 2;
	SkinProperties.Style688.Data14.Width = 110;
	SkinProperties.Style688.Data14.Height = 11;
	SkinProperties.Style688.Data24 = 13;
	SkinProperties.Style688.Data28.x = 23;
	SkinProperties.Style688.Data28.y = 14;
	SkinProperties.Style688.Data28.Width = 122;
	SkinProperties.Style688.Data28.Height = 5;
	SkinProperties.Style688.Data38.x = 1;
	SkinProperties.Style688.Data38.y = 1;
	SkinProperties.Style688.Data38.Width = 20;
	SkinProperties.Style688.Data38.Height = 20;
	SkinProperties.Style688.Data48.R = 0;
	SkinProperties.Style688.Data48.G = 0;
	SkinProperties.Style688.Data48.B = 0;
	SkinProperties.Style688.Data54.FontNumber = 4;
	SkinProperties.Style688.Data54.Color.R = 176;
	SkinProperties.Style688.Data54.Color.G = 176;
	SkinProperties.Style688.Data54.Color.B = 176;
	SkinProperties.Style688.Data54.Unknown10 = 0;
	SkinProperties.Style688.Data68.R = 71;
	SkinProperties.Style688.Data68.G = 71;
	SkinProperties.Style688.Data68.B = 71;
	SkinProperties.Style6FC.Data00 = 78;
	SkinProperties.Style6FC.Data04.x = 0;
	SkinProperties.Style6FC.Data04.y = 0;
	SkinProperties.Style6FC.Data0C.x = 0;
	SkinProperties.Style6FC.Data0C.y = 12;
	SkinProperties.Style6FC.Data14.x = 3;
	SkinProperties.Style6FC.Data14.y = 2;
	SkinProperties.Style6FC.Data14.Width = 71;
	SkinProperties.Style6FC.Data14.Height = 9;
	SkinProperties.Style6FC.Data24.x = 82;
	SkinProperties.Style6FC.Data24.y = 2;
	SkinProperties.Style6FC.Data24.Width = 71;
	SkinProperties.Style6FC.Data24.Height = 9;
	SkinProperties.Style6FC.Data34.x = 0;
	SkinProperties.Style6FC.Data34.y = -2;
	SkinProperties.Style6FC.Data3C.x = 0;
	SkinProperties.Style6FC.Data3C.y = 2;
	SkinProperties.Style6FC.Data44.FontNumber = 3;
	SkinProperties.Style6FC.Data44.Color.R = 255;
	SkinProperties.Style6FC.Data44.Color.G = 255;
	SkinProperties.Style6FC.Data44.Color.B = 255;
	SkinProperties.Style6FC.Data44.Unknown10 = 0;
	SkinProperties.Style6FC.Data58.x = 0;
	SkinProperties.Style6FC.Data58.y = 12;
	SkinProperties.Style6FC.Data60.x = 0;
	SkinProperties.Style6FC.Data60.y = -26;
	SkinProperties.Style6FC.Data68.x = 3;
	SkinProperties.Style6FC.Data68.y = 3;
	SkinProperties.Style6FC.Data68.Width = 34;
	SkinProperties.Style6FC.Data68.Height = 34;
	SkinProperties.Style6FC.Data78.x = 82;
	SkinProperties.Style6FC.Data78.y = 3;
	SkinProperties.Style6FC.Data78.Width = 34;
	SkinProperties.Style6FC.Data78.Height = 34;
	SkinProperties.Style6FC.Data88.x = 4;
	SkinProperties.Style6FC.Data88.y = 4;
	SkinProperties.Style6FC.Data88.Width = 83;
	SkinProperties.Style6FC.Data88.Height = 4;
	SkinProperties.Style6FC.Data98 = 32;
	SkinProperties.Style6FC.Data9C = 36;
	SkinProperties.Style6FC.DataA0 = 36;
	SkinProperties.Style6FC.DataA4 = 2;
	SkinProperties.Style6FC.DataA8.x = 77;
	SkinProperties.Style6FC.DataA8.y = 0;
	SkinProperties.Style6FC.DataB0.x = 2;
	SkinProperties.Style6FC.DataB0.y = 0;
	SkinProperties.Style6FC.DataB8.x = 0;
	SkinProperties.Style6FC.DataB8.y = -26;
	SkinProperties.Style6FC.DataC0.x = 0;
	SkinProperties.Style6FC.DataC0.y = 0;
	SkinProperties.Style6FC.DataC8.x = 74;
	SkinProperties.Style6FC.DataC8.y = 4;
	SkinProperties.Style6FC.DataC8.Width = 121;
	SkinProperties.Style6FC.DataC8.Height = 4;
	SkinProperties.Style6FC.DataD8.x = 4;
	SkinProperties.Style6FC.DataD8.y = 4;
	SkinProperties.Style6FC.DataD8.Width = 113;
	SkinProperties.Style6FC.DataD8.Height = 20;
	SkinProperties.Style6FC.DataE8.x = 0;
	SkinProperties.Style6FC.DataE8.y = 0;
	SkinProperties.Style6FC.DataF0.x = 0;
	SkinProperties.Style6FC.DataF0.y = 2;
	SkinProperties.Style6FC.DataF8.FontNumber = 3;
	SkinProperties.Style6FC.DataF8.Color.R = 255;
	SkinProperties.Style6FC.DataF8.Color.G = 255;
	SkinProperties.Style6FC.DataF8.Color.B = 255;
	SkinProperties.Style6FC.DataF8.Unknown10 = 1;
	SkinProperties.BuddylistWindow.Data00 = 32;
	SkinProperties.BuddylistWindow.Data04 = 16;
	SkinProperties.BuddylistWindow.Logo.x = 2;
	SkinProperties.BuddylistWindow.Logo.y = 2;
	SkinProperties.BuddylistWindow.Logo.Width = 12;
	SkinProperties.BuddylistWindow.Logo.Height = 12;
	SkinProperties.BuddylistWindow.Data18.x = 20;
	SkinProperties.BuddylistWindow.Data18.y = 4;
	SkinProperties.BuddylistWindow.Data18.Width = 132;
	SkinProperties.BuddylistWindow.Data18.Height = 13;
	SkinProperties.BuddylistWindow.Data28.FontNumber = 4;
	SkinProperties.BuddylistWindow.Data28.Color.R = 224;
	SkinProperties.BuddylistWindow.Data28.Color.G = 224;
	SkinProperties.BuddylistWindow.Data28.Color.B = 224;
	SkinProperties.BuddylistWindow.Data28.Unknown10 = 0;
	SkinProperties.BuddylistWindow.Data3C.x = 96;
	SkinProperties.BuddylistWindow.Data3C.y = 248;
	SkinProperties.BuddylistWindow.Data44.x = 96;
	SkinProperties.BuddylistWindow.Data44.y = 248;
	SkinProperties.BuddylistWindow.Data44.Width = 96;
	SkinProperties.BuddylistWindow.Data44.Height = 96;
	SkinProperties.BuddylistWindow.Data54.R = 248;
	SkinProperties.BuddylistWindow.Data54.G = 248;
	SkinProperties.BuddylistWindow.Data54.B = 248;
	SkinProperties.Data868 = 10;
	SkinProperties.Data86C = 14;
	SkinProperties.Data870 = 12;
	SkinProperties.Data874 = 38;
	SkinProperties.Data878 = 12;
	SkinProperties.Data87C = 14;
	SkinProperties.Data880 = 12;
	SkinProperties.Data884 = 38;
	SkinProperties.Data888 = 12;
	SkinProperties.Buttons.Data00.FontNumber = 3;
	SkinProperties.Buttons.Data00.Color.R = 255;
	SkinProperties.Buttons.Data00.Color.G = 255;
	SkinProperties.Buttons.Data00.Color.B = 255;
	SkinProperties.Buttons.Data00.Unknown10 = 1;
	SkinProperties.Buttons.Data14.FontNumber = 3;
	SkinProperties.Buttons.Data14.Color.R = 192;
	SkinProperties.Buttons.Data14.Color.G = 192;
	SkinProperties.Buttons.Data14.Color.B = 192;
	SkinProperties.Buttons.Data14.Unknown10 = 1;
	SkinProperties.Buttons.Data28 = 1;
	SkinProperties.Buttons.Texture.x = 2;
	SkinProperties.Buttons.Texture.y = 2;
	SkinProperties.Data8C0 = 100;
	SkinProperties.Style8C4.Data00.FontNumber = 4;
	SkinProperties.Style8C4.Data00.Color.R = 224;
	SkinProperties.Style8C4.Data00.Color.G = 224;
	SkinProperties.Style8C4.Data00.Color.B = 224;
	SkinProperties.Style8C4.Data00.Unknown10 = 0;
	SkinProperties.Style8C4.Data14.FontNumber = 4;
	SkinProperties.Style8C4.Data14.Color.R = 255;
	SkinProperties.Style8C4.Data14.Color.G = 255;
	SkinProperties.Style8C4.Data14.Color.B = 255;
	SkinProperties.Style8C4.Data14.Unknown10 = 0;
	SkinProperties.Style8C4.Data28.FontNumber = 4;
	SkinProperties.Style8C4.Data28.Color.R = 160;
	SkinProperties.Style8C4.Data28.Color.G = 160;
	SkinProperties.Style8C4.Data28.Color.B = 255;
	SkinProperties.Style8C4.Data28.Unknown10 = 0;
	SkinProperties.CheckBox.Item.x = 6;
	SkinProperties.CheckBox.Item.y = 5;
	SkinProperties.CheckBox.Data08.x = 24;
	SkinProperties.CheckBox.Data08.y = 4;
	SkinProperties.CheckBox.Data10.x = -10;
	SkinProperties.CheckBox.Data10.y = -6;
	SkinProperties.CheckBox.Data18.FontNumber = 4;
	SkinProperties.CheckBox.Data18.Color.R = 176;
	SkinProperties.CheckBox.Data18.Color.G = 176;
	SkinProperties.CheckBox.Data18.Color.B = 176;
	SkinProperties.CheckBox.Data18.Unknown10 = 0;
	SkinProperties.CheckBox.Data2C.FontNumber = 4;
	SkinProperties.CheckBox.Data2C.Color.R = 112;
	SkinProperties.CheckBox.Data2C.Color.G = 112;
	SkinProperties.CheckBox.Data2C.Color.B = 112;
	SkinProperties.CheckBox.Data2C.Unknown10 = 0;
	SkinProperties.Data940 = 1;
	SkinProperties.Style944.Data00.R = 54;
	SkinProperties.Style944.Data00.G = 54;
	SkinProperties.Style944.Data00.B = 54;
	SkinProperties.Style944.Data0C.R = 71;
	SkinProperties.Style944.Data0C.G = 71;
	SkinProperties.Style944.Data0C.B = 71;
	SkinProperties.Style944.Data18.R = 255;
	SkinProperties.Style944.Data18.G = 255;
	SkinProperties.Style944.Data18.B = 255;
	SkinProperties.Style944.Data24.FontNumber = 4;
	SkinProperties.Style944.Data24.Color.R = 224;
	SkinProperties.Style944.Data24.Color.G = 224;
	SkinProperties.Style944.Data24.Color.B = 224;
	SkinProperties.Style944.Data24.Unknown10 = 0;
	SkinProperties.Style944.Data38.FontNumber = 4;
	SkinProperties.Style944.Data38.Color.R = 176;
	SkinProperties.Style944.Data38.Color.G = 176;
	SkinProperties.Style944.Data38.Color.B = 176;
	SkinProperties.Style944.Data38.Unknown10 = 0;
	SkinProperties.Style944.Data4C.R = 128;
	SkinProperties.Style944.Data4C.G = 128;
	SkinProperties.Style944.Data4C.B = 128;
	SkinProperties.Data99C = 3;
	SkinProperties.Style9A0.Data00.R = 64;
	SkinProperties.Style9A0.Data00.G = 64;
	SkinProperties.Style9A0.Data00.B = 64;
	SkinProperties.Style9A0.Data0C.FontNumber = 4;
	SkinProperties.Style9A0.Data0C.Color.R = 176;
	SkinProperties.Style9A0.Data0C.Color.G = 176;
	SkinProperties.Style9A0.Data0C.Color.B = 176;
	SkinProperties.Style9A0.Data0C.Unknown10 = 0;
	SkinProperties.Style9A0.Data20 = 3;
	SkinProperties.Style9A0.Data24.R = 112;
	SkinProperties.Style9A0.Data24.G = 112;
	SkinProperties.Style9A0.Data24.B = 112;
	SkinProperties.Style9A0.Data30.FontNumber = 4;
	SkinProperties.Style9A0.Data30.Color.R = 255;
	SkinProperties.Style9A0.Data30.Color.G = 255;
	SkinProperties.Style9A0.Data30.Color.B = 255;
	SkinProperties.Style9A0.Data30.Unknown10 = 0;
	SkinProperties.Data9E4 = 1;
	SkinProperties.Style9E8.Data00 = 106;
	SkinProperties.Style9E8.Data04.x = 18;
	SkinProperties.Style9E8.Data04.y = 0;
	SkinProperties.Style9E8.Data0C.x = -82;
	SkinProperties.Style9E8.Data0C.y = 18;
	SkinProperties.Style9E8.Data14.R = 0;
	SkinProperties.Style9E8.Data14.G = 0;
	SkinProperties.Style9E8.Data14.B = 0;
	SkinProperties.Style9E8.Data20.x = 18;
	SkinProperties.Style9E8.Data20.y = 23;
	SkinProperties.Style9E8.Data28.x = -20;
	SkinProperties.Style9E8.Data28.y = -4;
	SkinProperties.Style9E8.Data30.x = 16;
	SkinProperties.Style9E8.Data30.y = 55;
	SkinProperties.Style9E8.Data38.x = -20;
	SkinProperties.Style9E8.Data38.y = -4;
	SkinProperties.Style9E8.Data40.x = 16;
	SkinProperties.Style9E8.Data40.y = 4;
	SkinProperties.Style9E8.Data48.x = 21;
	SkinProperties.Style9E8.Data48.y = -4;
	SkinProperties.Style9E8.Data50.x = -22;
	SkinProperties.Style9E8.Data50.y = 0;
	SkinProperties.Style9E8.Data58.x = 0;
	SkinProperties.Style9E8.Data58.y = -82;
	SkinProperties.Style9E8.Data60.x = 0;
	SkinProperties.Style9E8.Data60.y = 5;
	SkinProperties.Style9E8.Data68.x = -20;
	SkinProperties.Style9E8.Data68.y = -64;
	SkinProperties.Style9E8.Data70.x = 0;
	SkinProperties.Style9E8.Data70.y = -48;
	SkinProperties.Style9E8.Data78.x = 0;
	SkinProperties.Style9E8.Data78.y = -16;
	SkinProperties.Style9E8.Data80.x = 0;
	SkinProperties.Style9E8.Data80.y = -32;
	SkinProperties.Style9E8.Data88.x = 0;
	SkinProperties.Style9E8.Data88.y = 5;
	SkinProperties.Style9E8.Data90.x = 5;
	SkinProperties.Style9E8.Data90.y = -17;
	SkinProperties.Style9E8.Data98.x = -5;
	SkinProperties.Style9E8.Data98.y = 128;
	SkinProperties.Style9E8.DataA0.x = 128;
	SkinProperties.Style9E8.DataA0.y = 128;
	SkinProperties.Style9E8.DataA8.FontNumber = 4;
	SkinProperties.Style9E8.DataA8.Color.R = 160;
	SkinProperties.Style9E8.DataA8.Color.G = 160;
	SkinProperties.Style9E8.DataA8.Color.B = 255;
	SkinProperties.Style9E8.DataA8.Unknown10 = 0;
	SkinProperties.StyleAA4.Data00 = 96;
	SkinProperties.StyleAA4.Data04 = 18;
	SkinProperties.StyleAA4.Data08.x = 48;
	SkinProperties.StyleAA4.Data08.y = 4;
	SkinProperties.StyleAA4.Data10 = 4;
	SkinProperties.StyleAA4.Data14.FontNumber = 2;
	SkinProperties.StyleAA4.Data14.Color.R = 224;
	SkinProperties.StyleAA4.Data14.Color.G = 224;
	SkinProperties.StyleAA4.Data14.Color.B = 224;
	SkinProperties.StyleAA4.Data14.Unknown10 = 1;
	SkinProperties.StyleAA4.Data28.FontNumber = 2;
	SkinProperties.StyleAA4.Data28.Color.R = 128;
	SkinProperties.StyleAA4.Data28.Color.G = 128;
	SkinProperties.StyleAA4.Data28.Color.B = 128;
	SkinProperties.StyleAA4.Data28.Unknown10 = 1;
	SkinProperties.StyleAA4.Data3C.FontNumber = 2;
	SkinProperties.StyleAA4.Data3C.Color.R = 248;
	SkinProperties.StyleAA4.Data3C.Color.G = 96;
	SkinProperties.StyleAA4.Data3C.Color.B = 96;
	SkinProperties.StyleAA4.Data3C.Unknown10 = 1;
	SkinProperties.StyleAA4.Data50.FontNumber = 2;
	SkinProperties.StyleAA4.Data50.Color.R = 248;
	SkinProperties.StyleAA4.Data50.Color.G = 248;
	SkinProperties.StyleAA4.Data50.Color.B = 248;
	SkinProperties.StyleAA4.Data50.Unknown10 = 1;
	SkinProperties.StyleB08.Data00 = 162;
	SkinProperties.StyleB08.Data04 = 68;
	SkinProperties.StyleB08.Data08.x = 5;
	SkinProperties.StyleB08.Data08.y = 5;
	SkinProperties.StyleB08.Data10.x = -5;
	SkinProperties.StyleB08.Data10.y = 10;
	SkinProperties.StyleB08.Data18.FontNumber = 4;
	SkinProperties.StyleB08.Data18.Color.R = 144;
	SkinProperties.StyleB08.Data18.Color.G = 144;
	SkinProperties.StyleB08.Data18.Color.B = 144;
	SkinProperties.StyleB08.Data18.Unknown10 = 1;
	SkinProperties.StyleB08.Data2C.x = -56;
	SkinProperties.StyleB08.Data2C.y = -30;
	SkinProperties.StyleB08.Data34.x = -109;
	SkinProperties.StyleB08.Data34.y = -30;
	SkinProperties.StyleB08.Data3C.x = -162;
	SkinProperties.StyleB08.Data3C.y = -30;
	SkinProperties.StyleB08.Data44.x = 13;
	SkinProperties.StyleB08.Data44.y = -40;
	SkinProperties.StyleB08.Data4C.x = -13;
	SkinProperties.StyleB08.Data4C.y = 2;
	SkinProperties.StyleB08.Data54.x = 18;
	SkinProperties.StyleB08.Data54.y = 32;
	SkinProperties.StyleB08.Data5C.x = -18;
	SkinProperties.StyleB08.Data5C.y = -60;
	SkinProperties.StyleB08.Data64.FontNumber = 4;
	SkinProperties.StyleB08.Data64.Color.R = 192;
	SkinProperties.StyleB08.Data64.Color.G = 192;
	SkinProperties.StyleB08.Data64.Color.B = 192;
	SkinProperties.StyleB08.Data64.Unknown10 = 0;
	SkinProperties.StyleB08.Data78.FontNumber = 4;
	SkinProperties.StyleB08.Data78.Color.R = 192;
	SkinProperties.StyleB08.Data78.Color.G = 192;
	SkinProperties.StyleB08.Data78.Color.B = 192;
	SkinProperties.StyleB08.Data78.Unknown10 = 0;
	SkinProperties.DataB94 = 200;
	SkinProperties.DataB98 = 86;
	SkinProperties.DataB9C.x = 0;
	SkinProperties.DataB9C.y = 2;
	SkinProperties.DataB9C.Width = 107;
	SkinProperties.DataB9C.Height = 14;
	SkinProperties.DataBAC.x = -86;
	SkinProperties.DataBAC.y = 0;
	SkinProperties.DataBB4.x = 0;
	SkinProperties.DataBB4.y = 16;
	SkinProperties.DataBBC.x = 0;
	SkinProperties.DataBBC.y = 31;
	SkinProperties.DataBBC.Width = 107;
	SkinProperties.DataBBC.Height = 14;
	SkinProperties.DataBCC.x = -86;
	SkinProperties.DataBCC.y = 29;
	SkinProperties.DataBD4.x = 0;
	SkinProperties.DataBD4.y = 16;
	SkinProperties.DataBDC.x = 0;
	SkinProperties.DataBDC.y = 58;
	SkinProperties.DataBE4.x = 0;
	SkinProperties.DataBE4.y = 28;
	SkinProperties.DataBEC = 13;
	SkinProperties.DataBF0 = 200;
	SkinProperties.DataBF4 = 202;
	SkinProperties.DataBF8.x = 0;
	SkinProperties.DataBF8.y = 0;
	SkinProperties.DataC00.x = 0;
	SkinProperties.DataC00.y = 14;
	SkinProperties.DataC08.x = 0;
	SkinProperties.DataC08.y = 15;
	SkinProperties.DataC10.x = 0;
	SkinProperties.DataC10.y = 146;
	SkinProperties.DataC18.x = 0;
	SkinProperties.DataC18.y = 174;
	SkinProperties.DataC20.x = 0;
	SkinProperties.DataC20.y = 28;
	SkinProperties.DataC28 = 200;
	SkinProperties.DataC2C = 44;
	SkinProperties.DataC30.x = 45;
	SkinProperties.DataC30.y = 32;
	SkinProperties.DataC38.x = 0;
	SkinProperties.DataC38.y = 12;
	SkinProperties.DataC38.Width = 45;
	SkinProperties.DataC38.Height = 2;
	SkinProperties.DataC48.x = 0;
	SkinProperties.DataC48.y = 28;
	SkinProperties.DataC50.x = 0;
	SkinProperties.DataC50.y = 0;
	SkinProperties.DataC58 = 384;
	SkinProperties.DataC5C = 133;
	SkinProperties.DataC60.x = 0;
	SkinProperties.DataC60.y = 0;
	SkinProperties.DataC60.Width = 68;
	SkinProperties.DataC60.Height = 68;
	SkinProperties.DataC70 = 8;
	SkinProperties.DataC74.R = 71;
	SkinProperties.DataC74.G = 71;
	SkinProperties.DataC74.B = 71;
	SkinProperties.DataC80.x = 13;
	SkinProperties.DataC80.y = 72;
	SkinProperties.DataC88.x = 81;
	SkinProperties.DataC88.y = 0;
	SkinProperties.DataC88.Width = 81;
	SkinProperties.DataC88.Height = 24;
	SkinProperties.DataC98.x = 81;
	SkinProperties.DataC98.y = 48;
	SkinProperties.DataC98.Width = 81;
	SkinProperties.DataC98.Height = 72;
	SkinProperties.DataCA8.x = 137;
	SkinProperties.DataCA8.y = 0;
	SkinProperties.DataCB0 = 13;
	SkinProperties.DataCB4 = 13;
	SkinProperties.DataCB8.x = 0;
	SkinProperties.DataCB8.y = 105;
	SkinProperties.DataCC0 = 250;
	SkinProperties.DataCC4 = 177;
	SkinProperties.DataCC8.x = 0;
	SkinProperties.DataCC8.y = 0;
	SkinProperties.DataCC8.Width = 32;
	SkinProperties.DataCC8.Height = 32;
	SkinProperties.DataCD8.R = 71;
	SkinProperties.DataCD8.G = 71;
	SkinProperties.DataCD8.B = 71;
	SkinProperties.DataCE4.x = 45;
	SkinProperties.DataCE4.y = 0;
	SkinProperties.DataCEC.x = 0;
	SkinProperties.DataCEC.y = 42;
	SkinProperties.DataCF4.x = 0;
	SkinProperties.DataCF4.y = 45;
	SkinProperties.DataCFC.x = 0;
	SkinProperties.DataCFC.y = 132;
	SkinProperties.DataD04 = 250;
	SkinProperties.DataD08 = 177;
	SkinProperties.DataD0C.x = 0;
	SkinProperties.DataD0C.y = 0;
	SkinProperties.DataD0C.Width = 32;
	SkinProperties.DataD0C.Height = 32;
	SkinProperties.DataD1C.x = 45;
	SkinProperties.DataD1C.y = 0;
	SkinProperties.DataD24.x = 0;
	SkinProperties.DataD24.y = 28;
	SkinProperties.DataD2C.x = 0;
	SkinProperties.DataD2C.y = 45;
	SkinProperties.DataD34.x = 0;
	SkinProperties.DataD34.y = 132;
	SkinProperties.StyleD3C.Data00 = 498;
	SkinProperties.StyleD3C.Data04 = 272;
	SkinProperties.StyleD3C.Data08.x = 0;
	SkinProperties.StyleD3C.Data08.y = 0;
	SkinProperties.StyleD3C.Data08.Width = 242;
	SkinProperties.StyleD3C.Data08.Height = 22;
	SkinProperties.StyleD3C.Data18.R = 0;
	SkinProperties.StyleD3C.Data18.G = 35;
	SkinProperties.StyleD3C.Data18.B = 0;
	SkinProperties.StyleD3C.Data24.x = 14;
	SkinProperties.StyleD3C.Data24.y = 0;
	SkinProperties.StyleD3C.Data24.Width = 50;
	SkinProperties.StyleD3C.Data24.Height = 242;
	SkinProperties.StyleD3C.Data34.x = 104;
	SkinProperties.StyleD3C.Data34.y = 161;
	SkinProperties.StyleD3C.Data34.Width = 159;
	SkinProperties.StyleD3C.Data34.Height = 208;
	SkinProperties.StyleD3C.Data44.x = 159;
	SkinProperties.StyleD3C.Data44.y = 0;
	SkinProperties.StyleD3C.Data44.Width = 158;
	SkinProperties.StyleD3C.Data44.Height = 146;
	SkinProperties.StyleD3C.Data54.x = 22;
	SkinProperties.StyleD3C.Data54.y = 0;
	SkinProperties.StyleD3C.Data5C.x = 197;
	SkinProperties.StyleD3C.Data5C.y = 0;
	SkinProperties.StyleD3C.Data64.x = 14;
	SkinProperties.StyleD3C.Data64.y = 0;
	SkinProperties.StyleD3C.Data64.Width = 215;
	SkinProperties.StyleD3C.Data64.Height = 242;
	SkinProperties.StyleD3C.Data74.x = 22;
	SkinProperties.StyleD3C.Data74.y = 0;
	SkinProperties.StyleD3C.Data7C.x = 250;
	SkinProperties.StyleD3C.Data7C.y = 242;
	SkinProperties.StyleD3C.Data7C.Width = 22;
	SkinProperties.StyleD3C.Data7C.Height = 192;
	SkinProperties.StyleD3C.Data8C.x = 0;
	SkinProperties.StyleD3C.Data8C.y = 0;
	SkinProperties.StyleD3C.Data94.x = 256;
	SkinProperties.StyleD3C.Data94.y = 0;
	SkinProperties.StyleD3C.Data9C.x = 242;
	SkinProperties.StyleD3C.Data9C.y = 22;
	SkinProperties.StyleD3C.Data9C.Width = 256;
	SkinProperties.StyleD3C.Data9C.Height = 35;
	SkinProperties.StyleD3C.DataAC.x = 0;
	SkinProperties.StyleD3C.DataAC.y = 14;
	SkinProperties.StyleD3C.DataAC.Width = 256;
	SkinProperties.StyleD3C.DataAC.Height = 50;
	SkinProperties.StyleD3C.DataBC.x = 242;
	SkinProperties.StyleD3C.DataBC.y = 104;
	SkinProperties.StyleD3C.DataBC.Width = 417;
	SkinProperties.StyleD3C.DataBC.Height = 159;
	SkinProperties.StyleD3C.DataCC.x = 464;
	SkinProperties.StyleD3C.DataCC.y = 159;
	SkinProperties.StyleD3C.DataCC.Width = 256;
	SkinProperties.StyleD3C.DataCC.Height = 158;
	SkinProperties.StyleD3C.DataDC.x = 146;
	SkinProperties.StyleD3C.DataDC.y = 22;
	SkinProperties.StyleD3C.DataDC.Width = 256;
	SkinProperties.StyleD3C.DataDC.Height = 197;
	SkinProperties.StyleD3C.DataEC.x = 0;
	SkinProperties.StyleD3C.DataEC.y = 14;
	SkinProperties.StyleD3C.DataF4.x = 256;
	SkinProperties.StyleD3C.DataF4.y = 215;
	SkinProperties.StyleD3C.DataF4.Width = 242;
	SkinProperties.StyleD3C.DataF4.Height = 22;
	SkinProperties.DataE40 = 250;
	SkinProperties.DataE44 = 161;
	SkinProperties.DataE48.x = 0;
	SkinProperties.DataE48.y = 29;
	SkinProperties.DataE50.x = 0;
	SkinProperties.DataE50.y = 132;
	SkinProperties.DataE58.x = 0;
	SkinProperties.DataE58.y = 0;
	SkinProperties.DataE60.x = 0;
	SkinProperties.DataE60.y = 28;
	SkinProperties.StyleE68.Data00 = 190;
	SkinProperties.StyleE68.Data04 = 111;
	SkinProperties.StyleE68.Data08.FontNumber = 4;
	SkinProperties.StyleE68.Data08.Color.R = 176;
	SkinProperties.StyleE68.Data08.Color.G = 176;
	SkinProperties.StyleE68.Data08.Color.B = 176;
	SkinProperties.StyleE68.Data08.Unknown10 = 1;
	SkinProperties.StyleE68.Data1C.FontNumber = 4;
	SkinProperties.StyleE68.Data1C.Color.R = 176;
	SkinProperties.StyleE68.Data1C.Color.G = 176;
	SkinProperties.StyleE68.Data1C.Color.B = 176;
	SkinProperties.StyleE68.Data1C.Unknown10 = 0;
	SkinProperties.StyleE68.Data30 = 8;
	SkinProperties.StyleE68.Data34.x = 0;
	SkinProperties.StyleE68.Data34.y = 0;
	SkinProperties.StyleE68.Data3C.x = 0;
	SkinProperties.StyleE68.Data3C.y = 70;
	SkinProperties.StyleE68.Data44.x = 0;
	SkinProperties.StyleE68.Data44.y = 91;
	SkinProperties.StyleE68.Data4C.x = 0;
	SkinProperties.StyleE68.Data4C.y = 20;
	SkinProperties.StyleE68.Data54.x = 0;
	SkinProperties.StyleE68.Data54.y = 74;
	SkinProperties.StyleE68.Data5C.x = 0;
	SkinProperties.StyleE68.Data5C.y = 2;
	SkinProperties.DataECC = 200;
	SkinProperties.DataED0 = 163;
	SkinProperties.DataED4.x = 0;
	SkinProperties.DataED4.y = 0;
	SkinProperties.DataEDC.x = 0;
	SkinProperties.DataEDC.y = 14;
	SkinProperties.DataEE4.x = 0;
	SkinProperties.DataEE4.y = 15;
	SkinProperties.DataEEC.x = 0;
	SkinProperties.DataEEC.y = 104;
	SkinProperties.DataEF4.x = 0;
	SkinProperties.DataEF4.y = 132;
	SkinProperties.DataEFC.x = 0;
	SkinProperties.DataEFC.y = 14;
	SkinProperties.DataF04.x = 0;
	SkinProperties.DataF04.y = 147;
	SkinProperties.DataF0C.x = 0;
	SkinProperties.DataF0C.y = 16;
	SkinProperties.DataF14 = 225;
	SkinProperties.DataF18 = 31;
	SkinProperties.DataF1C.x = 0;
	SkinProperties.DataF1C.y = 15;
	SkinProperties.DataF24.x = 0;
	SkinProperties.DataF24.y = 16;
	SkinProperties.DataF2C.x = 0;
	SkinProperties.DataF2C.y = 0;
	SkinProperties.DataF34.x = 0;
	SkinProperties.DataF34.y = 14;
	SkinProperties.DataF3C = 250;
	SkinProperties.DataF40 = 87;
	SkinProperties.DataF44.x = 0;
	SkinProperties.DataF44.y = 71;
	SkinProperties.DataF4C.x = 0;
	SkinProperties.DataF4C.y = 16;
	SkinProperties.DataF54.x = 0;
	SkinProperties.DataF54.y = 0;
	SkinProperties.DataF5C.x = 0;
	SkinProperties.DataF5C.y = 70;
	SkinProperties.DataF64 = 200;
	SkinProperties.DataF68 = 31;
	SkinProperties.DataF6C.x = 0;
	SkinProperties.DataF6C.y = 15;
	SkinProperties.DataF74.x = 0;
	SkinProperties.DataF74.y = 16;
	SkinProperties.DataF7C.x = 0;
	SkinProperties.DataF7C.y = 0;
	SkinProperties.DataF84.x = 0;
	SkinProperties.DataF84.y = 14;
	SkinProperties.StyleF8C.Data00 = 250;
	SkinProperties.StyleF8C.Data04 = 174;
	SkinProperties.StyleF8C.Data08.x = 0;
	SkinProperties.StyleF8C.Data08.y = 0;
	SkinProperties.StyleF8C.Data10.x = 0;
	SkinProperties.StyleF8C.Data10.y = 14;
	SkinProperties.StyleF8C.Data18.x = 0;
	SkinProperties.StyleF8C.Data18.y = 15;
	SkinProperties.StyleF8C.Data20.x = 0;
	SkinProperties.StyleF8C.Data20.y = 2;
	SkinProperties.StyleF8C.Data28.x = 0;
	SkinProperties.StyleF8C.Data28.y = 32;
	SkinProperties.StyleF8C.Data30.x = 0;
	SkinProperties.StyleF8C.Data30.y = 14;
	SkinProperties.StyleF8C.Data38[0].x = 0;
	SkinProperties.StyleF8C.Data38[0].y = 47;
	SkinProperties.StyleF8C.Data38[1].x = 20;
	SkinProperties.StyleF8C.Data38[1].y = 47;
	SkinProperties.StyleF8C.Data38[2].x = 40;
	SkinProperties.StyleF8C.Data38[2].y = 47;
	SkinProperties.StyleF8C.Data38[3].x = 60;
	SkinProperties.StyleF8C.Data38[3].y = 47;
	SkinProperties.StyleF8C.Data38[4].x = 80;
	SkinProperties.StyleF8C.Data38[4].y = 47;
	SkinProperties.StyleF8C.Data38[5].x = 100;
	SkinProperties.StyleF8C.Data38[5].y = 47;
	SkinProperties.StyleF8C.Data38[6].x = 120;
	SkinProperties.StyleF8C.Data38[6].y = 47;
	SkinProperties.StyleF8C.Data38[7].x = 140;
	SkinProperties.StyleF8C.Data38[7].y = 47;
	SkinProperties.StyleF8C.Data38[8].x = 160;
	SkinProperties.StyleF8C.Data38[8].y = 47;
	SkinProperties.StyleF8C.Data38[9].x = 180;
	SkinProperties.StyleF8C.Data38[9].y = 47;
	SkinProperties.StyleF8C.Data38[10].x = 200;
	SkinProperties.StyleF8C.Data38[10].y = 47;
	SkinProperties.StyleF8C.Data90.x = 0;
	SkinProperties.StyleF8C.Data90.y = 76;
	SkinProperties.StyleF8C.Data98.x = 0;
	SkinProperties.StyleF8C.Data98.y = 14;
	SkinProperties.StyleF8C.DataA0.x = 0;
	SkinProperties.StyleF8C.DataA0.y = 91;
	SkinProperties.StyleF8C.DataA8.x = 0;
	SkinProperties.StyleF8C.DataA8.y = 48;
	SkinProperties.StyleF8C.DataB0.x = 0;
	SkinProperties.StyleF8C.DataB0.y = 152;
	SkinProperties.StyleF8C.DataB8.x = 0;
	SkinProperties.StyleF8C.DataB8.y = 22;
	SkinProperties.Data104C = 250;
	SkinProperties.Data1050 = 29;
	SkinProperties.Data1054.R = 0;
	SkinProperties.Data1054.G = 0;
	SkinProperties.Data1054.B = 0;
	SkinProperties.Data1060.x = 14;
	SkinProperties.Data1074.x = 0;
	SkinProperties.Data1074.y = 18;
	SkinProperties.Data107C.x = 0;
	SkinProperties.Data107C.y = 11;
	SkinProperties.Style1084.Data00 = 250;
	SkinProperties.Style1084.Data04 = 185;
	SkinProperties.Style1084.Data08.x = 0;
	SkinProperties.Style1084.Data08.y = 0;
	SkinProperties.Style1084.Data10.x = 0;
	SkinProperties.Style1084.Data10.y = 33;
	SkinProperties.Style1084.Data18.x = 0;
	SkinProperties.Style1084.Data18.y = 66;
	SkinProperties.Style1084.Data20.x = 0;
	SkinProperties.Style1084.Data20.y = 99;
	SkinProperties.Style1084.Data28.x = 0;
	SkinProperties.Style1084.Data28.y = 132;
	SkinProperties.Style1084.Data30.x = 0;
	SkinProperties.Style1084.Data30.y = 165;
	SkinProperties.Style1084.Data38.x = 99;
	SkinProperties.Style1084.Data38.y = 0;
	SkinProperties.Style1084.Data38.Width = 99;
	SkinProperties.Style1084.Data38.Height = 33;
	SkinProperties.Style1084.Data48.x = 99;
	SkinProperties.Style1084.Data48.y = 66;
	SkinProperties.Style1084.Data48.Width = 99;
	SkinProperties.Style1084.Data48.Height = 99;
	SkinProperties.Style1084.Data58.x = 99;
	SkinProperties.Style1084.Data58.y = 132;
	SkinProperties.Style1084.Data58.Width = 99;
	SkinProperties.Style1084.Data58.Height = 165;
	SkinProperties.Data10EC = 200;
	SkinProperties.Data10F0 = 119;
	SkinProperties.Data10F4.x = 0;
	SkinProperties.Data10F4.y = 0;
	SkinProperties.Data10FC.x = 0;
	SkinProperties.Data10FC.y = 14;
	SkinProperties.Data1104.x = 0;
	SkinProperties.Data1104.y = 15;
	SkinProperties.Data110C.x = 0;
	SkinProperties.Data110C.y = 104;
	SkinProperties.Style1114.Data00 = 300;
	SkinProperties.Style1114.Data04 = 346;
	SkinProperties.Style1114.Data08.x = 0;
	SkinProperties.Style1114.Data08.y = 0;
	SkinProperties.Style1114.Data10.x = 0;
	SkinProperties.Style1114.Data10.y = 14;
	SkinProperties.Style1114.Data18.x = 0;
	SkinProperties.Style1114.Data18.y = 15;
	SkinProperties.Style1114.Data20.x = 0;
	SkinProperties.Style1114.Data20.y = 16;
	SkinProperties.Style1114.Data28.x = 0;
	SkinProperties.Style1114.Data28.y = 92;
	SkinProperties.Style1114.Data30.x = 0;
	SkinProperties.Style1114.Data30.y = 14;
	SkinProperties.Style1114.Data38.x = 0;
	SkinProperties.Style1114.Data38.y = 107;
	SkinProperties.Style1114.Data40.x = 0;
	SkinProperties.Style1114.Data40.y = 77;
	SkinProperties.Style1114.Data48.x = 0;
	SkinProperties.Style1114.Data48.y = 199;
	SkinProperties.Style1114.Data50.x = 0;
	SkinProperties.Style1114.Data50.y = 14;
	SkinProperties.Style1114.Data58.x = 0;
	SkinProperties.Style1114.Data58.y = 214;
	SkinProperties.Style1114.Data60.x = 0;
	SkinProperties.Style1114.Data60.y = 49;
	SkinProperties.Style1114.Data68.x = 0;
	SkinProperties.Style1114.Data68.y = 315;
	SkinProperties.Style1114.Data70.x = 0;
	SkinProperties.Style1114.Data70.y = 14;
	SkinProperties.Style1114.Data78.x = 0;
	SkinProperties.Style1114.Data78.y = 330;
	SkinProperties.Style1114.Data80.x = 0;
	SkinProperties.Style1114.Data80.y = 16;
	SkinProperties.Style1114.Data88.x = 0;
	SkinProperties.Style1114.Data88.y = 46;
	SkinProperties.Style1114.Data90.x = 0;
	SkinProperties.Style1114.Data90.y = 14;
	SkinProperties.Style1114.Data98.x = 0;
	SkinProperties.Style1114.Data98.y = 61;
	SkinProperties.Style1114.DataA0.x = 0;
	SkinProperties.Style1114.DataA0.y = 16;
	SkinProperties.Style1114.DataA8.x = 0;
	SkinProperties.Style1114.DataA8.y = 278;
	SkinProperties.Style1114.DataB0.x = 0;
	SkinProperties.Style1114.DataB0.y = 22;
	SkinProperties.Data11CC = 250;
	SkinProperties.Data11D0 = 97;
	SkinProperties.Data11D4.x = 0;
	SkinProperties.Data11D4.y = 0;
	SkinProperties.Data11DC.x = 0;
	SkinProperties.Data11DC.y = 14;
	SkinProperties.Data11E4.x = 0;
	SkinProperties.Data11E4.y = 15;
	SkinProperties.Data11EC.x = 0;
	SkinProperties.Data11EC.y = 16;
	SkinProperties.Data11F4.x = 0;
	SkinProperties.Data11F4.y = 46;
	SkinProperties.Data11FC.x = 0;
	SkinProperties.Data11FC.y = 14;
	SkinProperties.Data1204[0].x = 0;
	SkinProperties.Data1204[0].y = 61;
	SkinProperties.Data1204[1].x = 20;
	SkinProperties.Data1204[1].y = 61;
	SkinProperties.Data1204[2].x = 40;
	SkinProperties.Data1204[2].y = 61;
	SkinProperties.Data1204[3].x = 60;
	SkinProperties.Data1204[3].y = 61;
	SkinProperties.Data1204[4].x = 80;
	SkinProperties.Data1204[4].y = 61;
	SkinProperties.Data1204[5].x = 100;
	SkinProperties.Data1204[5].y = 61;
	SkinProperties.Data1204[6].x = 120;
	SkinProperties.Data1204[6].y = 61;
	SkinProperties.Data1204[7].x = 0;
	SkinProperties.Data1204[7].y = 81;
	SkinProperties.Data1204[8].x = 20;
	SkinProperties.Data1204[8].y = 81;
	SkinProperties.Data1204[9].x = 40;
	SkinProperties.Data1204[9].y = 81;
	SkinProperties.Data1204[10].x = 60;
	SkinProperties.Data1204[10].y = 81;
	SkinProperties.Data1204[11].x = 80;
	SkinProperties.Data1204[11].y = 81;
	SkinProperties.Data1204[12].x = 100;
	SkinProperties.Data1204[12].y = 81;
	SkinProperties.Data1204[13].x = 120;
	SkinProperties.Data1204[13].y = 81;
	SkinProperties.Style1274.Data00 = 250;
	SkinProperties.Style1274.Data04 = 202;
	SkinProperties.Style1274.Data08.x = 0;
	SkinProperties.Style1274.Data08.y = 0;
	SkinProperties.Style1274.Data10.x = 0;
	SkinProperties.Style1274.Data10.y = 33;
	SkinProperties.Style1274.Data18.x = 0;
	SkinProperties.Style1274.Data18.y = 66;
	SkinProperties.Style1274.Data20.x = 0;
	SkinProperties.Style1274.Data20.y = 99;
	SkinProperties.Style1274.Data28.x = 0;
	SkinProperties.Style1274.Data28.y = 132;
	SkinProperties.Style1274.Data30.x = 0;
	SkinProperties.Style1274.Data30.y = 182;
	SkinProperties.Style1274.Data38.x = 99;
	SkinProperties.Style1274.Data38.y = 0;
	SkinProperties.Style1274.Data38.Width = 99;
	SkinProperties.Style1274.Data38.Height = 33;
	SkinProperties.Style1274.Data48.x = 99;
	SkinProperties.Style1274.Data48.y = 66;
	SkinProperties.Style1274.Data48.Width = 99;
	SkinProperties.Style1274.Data48.Height = 99;
	SkinProperties.Style1274.Data58.x = 99;
	SkinProperties.Style1274.Data58.y = 132;
	SkinProperties.Style1274.Data58.Width = 99;
	SkinProperties.Style1274.Data58.Height = 182;
	SkinProperties.Style1274.Data68.x = 0;
	SkinProperties.Style1274.Data68.y = 165;
	SkinProperties.Style1274.Data70.x = 0;
	SkinProperties.Style1274.Data70.y = 2;
	SkinProperties.Data12EC = 200;
	SkinProperties.Data12F0 = 154;
	SkinProperties.Data12F4.x = 0;
	SkinProperties.Data12F4.y = 0;
	SkinProperties.Data12FC.x = 0;
	SkinProperties.Data12FC.y = 22;
	SkinProperties.Data1304.x = 0;
	SkinProperties.Data1304.y = 33;
	SkinProperties.Data130C.x = 0;
	SkinProperties.Data130C.y = 22;
	SkinProperties.Data1314.x = 0;
	SkinProperties.Data1314.y = 66;
	SkinProperties.Data131C.x = 0;
	SkinProperties.Data131C.y = 22;
	SkinProperties.Data1324.x = 0;
	SkinProperties.Data1324.y = 132;
	SkinProperties.Data132C.x = 0;
	SkinProperties.Data132C.y = 22;
	SkinProperties.Data1334.x = 0;
	SkinProperties.Data1334.y = 99;
	SkinProperties.Data133C.x = 0;
	SkinProperties.Data133C.y = 22;
	SkinProperties.Data1344 = 200;
	SkinProperties.Data1348 = 185;
	SkinProperties.Data134C.x = 0;
	SkinProperties.Data134C.y = 0;
	SkinProperties.Data1354.x = 0;
	SkinProperties.Data1354.y = 22;
	SkinProperties.Data135C.x = 0;
	SkinProperties.Data135C.y = 50;
	SkinProperties.Data1364.x = 0;
	SkinProperties.Data1364.y = 104;
	SkinProperties.Data136C.x = 0;
	SkinProperties.Data136C.y = 35;
	SkinProperties.Data1374.x = 0;
	SkinProperties.Data1374.y = 14;
	SkinProperties.Data137C.x = -86;
	SkinProperties.Data137C.y = 167;
	SkinProperties.Data1384.x = 0;
	SkinProperties.Data1384.y = 171;
	SkinProperties.Data138C.x = 0;
	SkinProperties.Data138C.y = 14;
	SkinProperties.Style1394.Data00 = 200;
	SkinProperties.Style1394.Data04 = 213;
	SkinProperties.Style1394.Data08.x = 0;
	SkinProperties.Style1394.Data08.y = 0;
	SkinProperties.Style1394.Data10.x = 0;
	SkinProperties.Style1394.Data10.y = 15;
	SkinProperties.Style1394.Data18.x = 70;
	SkinProperties.Style1394.Data18.y = 15;
	SkinProperties.Style1394.Data18.Width = 140;
	SkinProperties.Style1394.Data18.Height = 15;
	SkinProperties.Style1394.Data28.x = 0;
	SkinProperties.Style1394.Data28.y = 51;
	SkinProperties.Style1394.Data30.x = 0;
	SkinProperties.Style1394.Data30.y = 22;
	SkinProperties.Style1394.Data38.x = 0;
	SkinProperties.Style1394.Data38.y = 86;
	SkinProperties.Style1394.Data40.x = 0;
	SkinProperties.Style1394.Data40.y = 22;
	SkinProperties.Style1394.Data48.x = 0;
	SkinProperties.Style1394.Data48.y = 121;
	SkinProperties.Style1394.Data50.x = 0;
	SkinProperties.Style1394.Data50.y = 22;
	SkinProperties.Style1394.Data58.x = 0;
	SkinProperties.Style1394.Data58.y = 181;
	SkinProperties.Style1394.Data60.x = 0;
	SkinProperties.Style1394.Data60.y = 12;
	SkinProperties.Style1394.Data68.x = 0;
	SkinProperties.Style1394.Data68.y = 166;
	SkinProperties.Style1394.Data70.x = 0;
	SkinProperties.Style1394.Data70.y = 199;
	SkinProperties.Data140C = 260;
	SkinProperties.Data1410 = 162;
	SkinProperties.Data1414.x = 0;
	SkinProperties.Data1414.y = 0;
	SkinProperties.Data141C.x = 0;
	SkinProperties.Data141C.y = 22;
	SkinProperties.Data1424.x = 0;
	SkinProperties.Data1424.y = 35;
	SkinProperties.Data142C.x = 0;
	SkinProperties.Data142C.y = 22;
	SkinProperties.Data1434.x = 0;
	SkinProperties.Data1434.y = 70;
	SkinProperties.Data143C.x = 0;
	SkinProperties.Data143C.y = 22;
	SkinProperties.Data1444.x = 0;
	SkinProperties.Data1444.y = 105;
	SkinProperties.Data144C.x = 0;
	SkinProperties.Data144C.y = 22;
	SkinProperties.Data1454.x = 0;
	SkinProperties.Data1454.y = 140;
	SkinProperties.Data145C.x = 0;
	SkinProperties.Data145C.y = 22;
	SkinProperties.Data1464 = 200;
	SkinProperties.Data1468 = 22;
	SkinProperties.Data146C.x = 0;
	SkinProperties.Data146C.y = 0;
	SkinProperties.Data1474.x = 0;
	SkinProperties.Data1474.y = 22;
	SkinProperties.Data147C = 200;
	SkinProperties.Data1480 = 198;
	SkinProperties.Data1484.x = 0;
	SkinProperties.Data1484.y = 15;
	SkinProperties.Data148C.x = 0;
	SkinProperties.Data148C.y = 104;
	SkinProperties.Data1494.R = 0;
	SkinProperties.Data1494.G = 0;
	SkinProperties.Data1494.B = 0;
	SkinProperties.Data14A0.x = 14;
	SkinProperties.Data14A0.y = 0;
	SkinProperties.Data14A8.x = 147;
	SkinProperties.Data14A8.y = 0;
	SkinProperties.Data14B0.x = 16;
	SkinProperties.Data14B0.y = 0;
	SkinProperties.Data14B8.x = 132;
	SkinProperties.Data14B8.y = 0;
	SkinProperties.Data14C0.x = 14;
	SkinProperties.Data14C0.y = 0;
	SkinProperties.Data14C8.x = 176;
	SkinProperties.Data14C8.y = 0;
	SkinProperties.Data14D0.x = 22;
	SkinProperties.Data14D0.y = 255;
	SkinProperties.Data14D0.Width = 255;
	SkinProperties.Data14D0.Height = 255;
	SkinProperties.Data14E0 = 475;
	SkinProperties.Data14E4 = 284;
	SkinProperties.Data14E8.x = 0;
	SkinProperties.Data14E8.y = 0;
	SkinProperties.Data14F0.x = 0;
	SkinProperties.Data14F0.y = 56;
	SkinProperties.Data14F8.x = 0;
	SkinProperties.Data14F8.y = 57;
	SkinProperties.Data1500.x = 0;
	SkinProperties.Data1500.y = 174;
	SkinProperties.Data1508.x = 0;
	SkinProperties.Data1508.y = 253;
	SkinProperties.Data1510.x = 0;
	SkinProperties.Data1510.y = 14;
	SkinProperties.Data1518.x = 0;
	SkinProperties.Data1518.y = 268;
	SkinProperties.Data1520.x = 0;
	SkinProperties.Data1520.y = 16;
	TEND
}

// --- Timers ---------------------------------------------------------------

// 0x49af20, line 851. Takes the first free entry.
void AddTimer(TTimer *Timer)
{
	TBEGIN
	int i = 0;
	while (Timers(i) != NULL)
		i++;
	Timers(i) = Timer;
	TEND
}

// 0x49b0d0, line 868
void RemoveTimer(TTimer *Timer)
{
	int i;
	TBEGIN
	for (i = Timers.min; i <= Timers.max; i++) {
		if (Timers(i) == Timer) {
			Timers(i) = NULL;
			return;
		}
	}
	TEND1(i)
}

// 0x4a27c0, line 887. Called by ProcessEvents. The first call only starts
// the clock.
void UpdateTimers()
{
	int i;
	TBEGIN
	static int LastTime = GetMilliseconds();
	int elapsed = GetMilliseconds() - LastTime;
	LastTime = GetMilliseconds();
	for (i = Timers.min; i <= Timers.max; i++) {
		if (Timers(i) != NULL && !Timers(i)->IsExpired())
			Timers(i)->Update(elapsed);
	}
	TEND1(i)
}

// 0x49b290, line 905. A time of 0 leaves the timer inactive.
TTimer::TTimer(int Time, TCallback Callback)
{
	this->Callback = Callback;
	TBEGIN
	Remaining = Time * 100;
	if (Remaining == 0) {
		Active = false;
	} else {
		AddTimer(this);
		Active = true;
	}
	TEND
}

// 0x4a1dc0, the deleting destructor; this destructor is inlined into it.
TTimer::~TTimer()
{
	RemoveTimer(this);
	Active = false;
}

// 0x4a1df0, line 947. The callback is sent on every update, not only when
// the time is up.
void TTimer::Update(int elapsed)
{
	TBEGIN
	if (Active) {
		if (Remaining > 0) {
			Remaining -= elapsed;
			if (Remaining < 0)
				Remaining = 0;
		}
		Callback.Send();
		if (Remaining == 0) {
			RemoveTimer(this);
			Active = false;
		}
	}
	TEND1(elapsed)
}

// --- TComponent -------------------------------------------------------------

// 0x49b450, line 993
void TComponent::InsertChild(TComponent *Child)
{
	TERROR("not possible");
}

// 0x49b570, line 1001
void TComponent::AppendChild(TComponent *Child)
{
	TERROR("not possible");
}

// 0x49b690, line 1009
void TComponent::RemoveChild(TComponent *Child)
{
	TERROR("not possible");
}

// 0x49b7b0, line 1017
void TComponent::DeleteChild(TComponent *Child)
{
	TERROR("not possible");
}

// 0x49b8d0, line 1025
void TComponent::InsertChildAfter(TComponent *Previous, TComponent *Child)
{
	TERROR("not possible");
}

// Line 1033 has no function in the binary (removed by the linker).

// 0x49b9f0, line 1041
void TComponent::SwapChildren(TComponent *Child1, TComponent *Child2)
{
	TERROR("not possible");
}

// 0x49bb10, line 1049
bool TComponent::IsChild(TComponent *Child)
{
	TERROR("not possible");
}

// 0x49bc30, lines 1057-1066. The assertion comes before the try block.
TComponent *TComponent::GetPrevious()
{
	TASSERT(Parent!=NULL);
	TBEGIN
	TComponent *Search = Parent->GetFirstChild();
	while (Search != NULL) {
		if (Search->GetNext() == this)
			return Search;
		Search = Search->GetNext();
	}
	return NULL;
	TEND
}

// 0x49bee0, lines 1092-1093. No try block.
void TComponent::SetSize(int _Width, int _Height)
{
	TASSERT1(_Width>=0, _Width);
	TASSERT1(_Height>=0, _Height);
	Height = _Height;
	Width = _Width;
}

// 0x4a1fb0, lines 1118-1163. Draws the drawing list at (x, y); entries
// without a size draw the whole skin once.
void TComponent::Draw(int Surface, int x, int y)
{
	int i = 0;
	TBEGIN
	for (; i < DrawingListLength; i++) {
		if (DrawingList[i].skin == 0)
			continue;
		TASSERT1(DrawingList[i].skin<NumberOfSkins, DrawingList[i].skin);
		if (DrawingList[i].width == 0 && DrawingList[i].height == 0) {
			TSkin *Skin = &Skins[DrawingList[i].skin];
			FUN_00490c60(Surface, DrawingList[i].x + x, DrawingList[i].y + y, 4, Skin->x, Skin->y, Skin->width,
				Skin->height);
			continue;
		}
		if (DrawingList[i].x + x < 0 || DrawingList[i].y + y < 0) {
			std::stringstream s;
			const char *Name = " (CONSOLETEXT_CORNER_TOPRIGHT)";
			if (DrawingList[i].skin != 170)
				Name = "";
			s << "dialog origin outside main window " << "(surface: " << Surface << " x: " << x << " y: " << y
				<< " DrawingListLength: " << DrawingListLength << " i: " << i << " DrawingList.Skin: "
				<< DrawingList[i].skin << Name << " DrawingList.x: " << DrawingList[i].x
				<< " DrawingList.y: " << DrawingList[i].y << " DrawingList.width: " << DrawingList[i].width
				<< " DrawingList.height: " << DrawingList[i].height << " DrawingList.srcx: "
				<< DrawingList[i].srcx << " DrawingList.srcy: " << DrawingList[i].srcy << ")";
			TERROR(s.str().c_str());
		}
		DrawSkin(Surface, DrawingList[i].x + x, DrawingList[i].y + y, DrawingList[i].width,
			DrawingList[i].height, DrawingList[i].skin, DrawingList[i].srcx, DrawingList[i].srcy);
	}
	TEND2(Surface, i)
}

// --- TWindow ----------------------------------------------------------------

// 0x49c190, lines 1193-1194. Inserts the child at the front. No try block.
void TWindow::InsertChild(TComponent *Child)
{
	TASSERT(Child!=NULL);
	TASSERT(Child->Parent==NULL);
	Child->Next = FirstChild;
	FirstChild = Child;
	Child->Parent = this;
}

// 0x49c430, lines 1206-1207. No try block. The first child's Next is not
// reset.
void TWindow::AppendChild(TComponent *Child)
{
	TASSERT(Child!=NULL);
	TASSERT(Child->Parent==NULL);
	if (FirstChild == NULL) {
		FirstChild = Child;
	} else {
		TComponent *Search = FirstChild;
		while (Search->Next != NULL)
			Search = Search->Next;
		Search->Next = Child;
		Child->Next = NULL;
	}
	Child->Parent = this;
}

// 0x49c6f0, line 1231
void TWindow::DeleteChild(TComponent *Child)
{
	TBEGIN
	RemoveChild(Child);
	delete Child;
	TEND
}

// 0x49c890, lines 1239-1260. The assertions before the try block. A child
// holding the capture stops dragging and releases it through itself (with
// NULL), which reaches this window's ReleaseCapture by its parent.
void TWindow::RemoveChild(TComponent *Child)
{
	TASSERT(Child!=NULL);
	TASSERT(Child->Parent==this);
	TBEGIN
	if (Child == Capture) {
		Child->StopDragging();
		Child->ReleaseCapture(NULL);
	}
	if (Child == FirstChild) {
		FirstChild = Child->Next;
	} else {
		TComponent *Search = FirstChild;
		for (;;) {
			TASSERT(Search!=NULL);
			if (Search->Next == Child)
				break;
			Search = Search->Next;
		}
		Search->Next = Child->Next;
	}
	Child->Parent = NULL;
	Child->Next = NULL;
	TEND
}

// 0x49cd00, lines 1269-1287. Inserts the child after Previous, at the front
// when Previous is NULL. The first two assertions are before the try block.
void TWindow::InsertChildAfter(TComponent *Previous, TComponent *Child)
{
	TASSERT(Child!=NULL);
	TASSERT(Child->Parent==NULL);
	TBEGIN
	if (Previous == NULL) {
		InsertChild(Child);
		return;
	}
	TComponent *Search = FirstChild;
	TASSERT(Search!=NULL);
	while (Search != NULL && Search != Previous)
		Search = Search->Next;
	TASSERT(Search!=NULL);
	Child->Parent = this;
	Child->Next = Search->Next;
	Search->Next = Child;
	TEND
}

// 0x49d200, lines 1295-1347. The first five assertions are before the try
// block. Odd behaviour kept: two children that are not neighbours are
// removed and inserted again after their own predecessors, so they keep
// their places.
void TWindow::SwapChildren(TComponent *Child1, TComponent *Child2)
{
	TASSERT(Child1!=NULL);
	TASSERT(Child2!=NULL);
	TASSERT(Child1->Parent==this);
	TASSERT(Child2->Parent==this);
	TASSERT(FirstChild!=NULL);
	TBEGIN
	TComponent *Before1 = NULL;
	TComponent *Search1 = FirstChild;
	while (Search1 != NULL && Search1 != Child1) {
		Before1 = Search1;
		Search1 = Search1->Next;
	}
	TASSERT(Search1!=NULL);
	TComponent *Before2 = NULL;
	TComponent *Search2 = FirstChild;
	while (Search2 != NULL && Search2 != Child2) {
		Before2 = Search2;
		Search2 = Search2->Next;
	}
	TASSERT(Search2!=NULL);
	TASSERT(Before1!=Before2);
	TComponent *After2 = Search2->Next;
	if (After2 == Search1) {
		TComponent *After1 = Search1->Next;
		if (FirstChild == Search2)
			FirstChild = Search1;
		else
			Before2->Next = Search1;
		Search1->Next = Search2;
		Search2->Next = After1;
	} else if (Search1->Next == Search2) {
		if (FirstChild == Search1)
			FirstChild = Search2;
		else
			Before1->Next = Search2;
		Search2->Next = Search1;
		Search1->Next = After2;
	} else {
		RemoveChild(Search1);
		RemoveChild(Search2);
		InsertChildAfter(Before1, Search1);
		InsertChildAfter(Before2, Search2);
	}
	TEND
}

// 0x49da60, line 1389
void TWindow::SetPosition(int _OffsetX, int _OffsetY)
{
	TBEGIN
	OffsetX = _OffsetX;
	OffsetY = _OffsetY;
	for (TComponent *Child = FirstChild; Child != NULL; Child = Child->Next)
		Child->PositionChanged();
	TEND2(_OffsetX, _OffsetY)
}

// 0x4a2590. Deletes the children. The deleting destructor is 0x4a29e0.
TWindow::~TWindow()
{
	while (FirstChild != NULL)
		DeleteChild(FirstChild);
}

// 0x4a2600, line 1408. The window's own drawing list, then the children.
void TWindow::Draw(int Surface, int x, int y)
{
	TBEGIN
	TComponent::Draw(Surface, x, y);
	for (TComponent *Child = FirstChild; Child != NULL; Child = Child->Next)
		Child->Draw(Surface, Child->OffsetX + x, Child->OffsetY + y);
	TEND1(Surface)
}

// 0x49dc20, line 1424
void TWindow::Arrange()
{
	TBEGIN
	for (TComponent *Child = FirstChild; Child != NULL; Child = Child->Next)
		Child->Arrange();
	TEND
}

// 0x49ddc0, line 1448. The mouse functions go to the capture if there is
// one, else to the first child under the point that takes the event, in
// child coordinates.
bool TWindow::MouseMove(int x, int y)
{
	TBEGIN
	if (Capture != NULL)
		return Capture->MouseMove(x - Capture->OffsetX, y - Capture->OffsetY);
	for (TComponent *Child = FirstChild; Child != NULL; Child = Child->Next) {
		if (In(x - Child->OffsetX, 0, Child->Width - 1) && In(y - Child->OffsetY, 0, Child->Height - 1)
		 && Child->MouseMove(x - Child->OffsetX, y - Child->OffsetY))
			return true;
	}
	return false;
	TEND2(x, y)
}

// 0x49dff0, line 1472
bool TWindow::LeftButtonDown(int x, int y, int Flags)
{
	TBEGIN
	if (Capture != NULL)
		return Capture->LeftButtonDown(x - Capture->OffsetX, y - Capture->OffsetY, Flags);
	for (TComponent *Child = FirstChild; Child != NULL; Child = Child->Next) {
		if (In(x - Child->OffsetX, 0, Child->Width - 1) && In(y - Child->OffsetY, 0, Child->Height - 1)
		 && Child->LeftButtonDown(x - Child->OffsetX, y - Child->OffsetY, Flags))
			return true;
	}
	return false;
	TEND2(x, y)
}

// 0x49e230, line 1496
bool TWindow::LeftButtonUp(int x, int y, int Flags)
{
	TBEGIN
	if (Capture != NULL)
		return Capture->LeftButtonUp(x - Capture->OffsetX, y - Capture->OffsetY, Flags);
	for (TComponent *Child = FirstChild; Child != NULL; Child = Child->Next) {
		if (In(x - Child->OffsetX, 0, Child->Width - 1) && In(y - Child->OffsetY, 0, Child->Height - 1)
		 && Child->LeftButtonUp(x - Child->OffsetX, y - Child->OffsetY, Flags))
			return true;
	}
	return false;
	TEND2(x, y)
}

// 0x49e470, line 1520
bool TWindow::LeftButtonDoubleClick(int x, int y, int Flags)
{
	TBEGIN
	if (Capture != NULL)
		return Capture->LeftButtonDoubleClick(x - Capture->OffsetX, y - Capture->OffsetY, Flags);
	for (TComponent *Child = FirstChild; Child != NULL; Child = Child->Next) {
		if (In(x - Child->OffsetX, 0, Child->Width - 1) && In(y - Child->OffsetY, 0, Child->Height - 1)
		 && Child->LeftButtonDoubleClick(x - Child->OffsetX, y - Child->OffsetY, Flags))
			return true;
	}
	return false;
	TEND2(x, y)
}

// 0x49e6b0, line 1544
bool TWindow::RightButtonUp(int x, int y, int Flags)
{
	TBEGIN
	if (Capture != NULL)
		return Capture->RightButtonUp(x - Capture->OffsetX, y - Capture->OffsetY, Flags);
	for (TComponent *Child = FirstChild; Child != NULL; Child = Child->Next) {
		if (In(x - Child->OffsetX, 0, Child->Width - 1) && In(y - Child->OffsetY, 0, Child->Height - 1)
		 && Child->RightButtonUp(x - Child->OffsetX, y - Child->OffsetY, Flags))
			return true;
	}
	return false;
	TEND2(x, y)
}

// 0x49e8f0, line 1568
bool TWindow::MouseWheel(int x, int y, int Direction)
{
	TBEGIN
	if (Capture != NULL)
		return Capture->MouseWheel(x - Capture->OffsetX, y - Capture->OffsetY, Direction);
	for (TComponent *Child = FirstChild; Child != NULL; Child = Child->Next) {
		if (In(x - Child->OffsetX, 0, Child->Width - 1) && In(y - Child->OffsetY, 0, Child->Height - 1)
		 && Child->MouseWheel(x - Child->OffsetX, y - Child->OffsetY, Direction))
			return true;
	}
	return false;
	TEND2(x, y)
}

// 0x49eb30, line 1588. No capture check; only the first child under the
// point gets it.
void TWindow::MouseHover(int x, int y)
{
	TBEGIN
	for (TComponent *Child = FirstChild; Child != NULL; Child = Child->Next) {
		if (In(x - Child->OffsetX, 0, Child->Width - 1) && In(y - Child->OffsetY, 0, Child->Height - 1)) {
			Child->MouseHover(x - Child->OffsetX, y - Child->OffsetY);
			return;
		}
	}
	TEND2(x, y)
}

// 0x49ed10, line 1611. No capture check. The child that explains itself
// moves the help area by its offset; without one the help handle is cleared.
bool TWindow::HelpMouseMove(int x, int y)
{
	TBEGIN
	for (TComponent *Child = FirstChild; Child != NULL; Child = Child->Next) {
		if (In(x - Child->OffsetX, 0, Child->Width - 1) && In(y - Child->OffsetY, 0, Child->Height - 1)
		 && Child->HelpMouseMove(x - Child->OffsetX, y - Child->OffsetY)) {
			MoveHelpPosition(Child->OffsetX, Child->OffsetY);
			return true;
		}
	}
	SetHelpHandle(0);
	return false;
	TEND2(x, y)
}

// 0x49ef10, lines 1633-1638. The assertion is before the try block. The
// parents up to the main window capture this window in turn.
void TWindow::SetCapture(TComponent *Component)
{
	TASSERT(Capture==NULL);
	TBEGIN
	Capture = Component;
	if (Parent != NULL)
		Parent->SetCapture(this);
	TEND
}

// 0x49f1a0, lines 1648-1653. The assertion is before the try block.
void TWindow::ReleaseCapture(TComponent *Component)
{
	TASSERT(Capture==Component);
	TBEGIN
	Capture = NULL;
	if (Parent != NULL)
		Parent->ReleaseCapture(this);
	TEND
}

// 0x49f430, line 1668. Does not release the parents' capture.
void TWindow::StopDragging()
{
	TBEGIN
	if (Capture != NULL) {
		Capture->StopDragging();
		Capture = NULL;
	}
	TEND
}

// 0x49f5d0, lines 1679-1690. Negative coordinates count from the right and
// bottom edges, sizes below 1 are relative to the space left. The
// assertions are before the try block.
void TWindow::SetChildArea(TComponent *Child, int x, int y, int width, int height)
{
	TASSERT(Child->Parent==this);
	TASSERT1(Width>0, Width);
	TASSERT1(Height>0, Height);
	TBEGIN
	if (x < 0)
		x = Width + x;
	if (y < 0)
		y = Height + y;
	Child->SetPosition(x, y);
	if (width < 1)
		width = (Width - x) + width;
	if (height < 1)
		height = (Height - y) + height;
	Child->SetSize(width, height);
	TEND
}

// 0x49fa40, lines 1700-1708. The assertions are before the try block.
void TWindow::SetChildPosition(TComponent *Child, int x, int y)
{
	TASSERT(Child->Parent==this);
	TASSERT1(Width>0, Width);
	TASSERT1(Height>0, Height);
	TBEGIN
	if (x < 0)
		x = Width + x;
	if (y < 0)
		y = Height + y;
	Child->SetPosition(x, y);
	TEND
}

// 0x49fe70, lines 1724-1738. Sizes the window to the child placed at
// (x, y), with negative width and height as margins after it, and not below
// the minimum size. Notify chooses the virtual SetSize; otherwise only the
// size fields change. The assertions are before the try block.
void TWindow::FitToChild(TComponent *Child, int x, int y, int width, int height, bool Notify)
{
	TASSERT(Child!=NULL);
	TASSERT1(Child->getWidth()>0, Child->getWidth());
	TASSERT1(Child->getHeight()>0, Child->getHeight());
	TBEGIN
	int NewWidth = Child->Width + (x > 0 ? x : 0) + (width < 0 ? -width : 0);
	int NewHeight = (y > 0 ? y : 0) + Child->Height + (height < 0 ? -height : 0);
	int MinimumWidth = GetMinimumWidth();
	if (NewWidth < MinimumWidth)
		NewWidth = MinimumWidth;
	int MinimumHeight = GetMinimumHeight();
	if (NewHeight < MinimumHeight)
		NewHeight = MinimumHeight;
	if (Notify)
		SetSize(NewWidth, NewHeight);
	else
		TComponent::SetSize(NewWidth, NewHeight);
	TEND
}

// 0x4a0300, lines 1772-1794. Moves the children so that their bounding box
// starts at (|Left|, |Top|) and sizes the window to it plus |Right| and
// |Bottom|, not below the minimum size. Nothing happens without children.
void TWindow::FitToChildren(int Left, int Top, int Right, int Bottom, bool Notify)
{
	TBEGIN
	if (FirstChild == NULL)
		return;
	int LeastX = INT_MAX;
	int LeastY = INT_MAX;
	int Width = 0;
	int Height = 0;
	TComponent *Child;
	for (Child = FirstChild; Child != NULL; Child = Child->GetNext()) {
		if (Child->OffsetX <= LeastX)
			LeastX = Child->OffsetX;
		if (Child->OffsetY <= LeastY)
			LeastY = Child->OffsetY;
		if (Width <= Child->OffsetX + Child->Width)
			Width = Child->OffsetX + Child->Width;
		if (Height <= Child->Height + Child->OffsetY)
			Height = Child->Height + Child->OffsetY;
	}
	TASSERT1(Width>0, Width);
	TASSERT1(Height>0, Height);
	TASSERT1(LeastX<INT_MAX, LeastX);
	TASSERT1(LeastY<INT_MAX, LeastY);
	int dx = abs(Left) - LeastX;
	int dy = abs(Top) - LeastY;
	Width = Width + abs(Right) + abs(Left);
	Height = Height + abs(Bottom) + abs(Top);
	for (Child = FirstChild; Child != NULL; Child = Child->GetNext())
		Child->SetPosition(Child->OffsetX + dx, Child->OffsetY + dy);
	int MinimumWidth = GetMinimumWidth();
	if (Width < MinimumWidth)
		Width = MinimumWidth;
	int MinimumHeight = GetMinimumHeight();
	if (Height < MinimumHeight)
		Height = MinimumHeight;
	if (Notify)
		SetSize(Width, Height);
	else
		TComponent::SetSize(Width, Height);
	TEND
}

// 0x4a0920, line 1860. Cuts the children to the window. A child that
// sticks out is deleted when Strict is set (or when nothing of it is left),
// else moved and resized to the visible part. The checks against the right
// and bottom edges use the old position.
void TWindow::ClipChildren(bool Strict)
{
	TBEGIN
	if (FirstChild != NULL) {
		TComponent *Next = FirstChild;
		while (Next != NULL) {
			TComponent *Child = Next;
			Next = Child->GetNext();
			int x = Child->OffsetX;
			int NewHeight = Child->Height;
			int y = Child->OffsetY;
			int OldWidth = Child->Width;
			bool Changed = x < 0;
			int NewWidth = OldWidth;
			if (Changed) {
				NewWidth = Child->OffsetX + OldWidth;
				x = 0;
			}
			bool Remove = Changed && Strict;
			int NewY = y;
			if (y < 0) {
				Remove = Strict || Remove;
				NewHeight = Child->Height + y;
				NewY = 0;
			}
			Changed = y < 0 || Changed;
			if (Width < Child->OffsetX + OldWidth) {
				if (Strict)
					Remove = true;
				NewWidth = Width - Child->OffsetX;
				Changed = true;
			}
			if (Height < Child->Height + y) {
				if (Strict)
					Remove = true;
				NewHeight = Height - y;
				Changed = true;
			}
			if (NewWidth < 0 || NewHeight < 0 || Remove) {
				DeleteChild(Child);
			} else if (Changed) {
				Child->SetPosition(x, NewY);
				Child->SetSize(NewWidth, NewHeight);
			}
		}
	}
	TEND
}

// --- TElement -------------------------------------------------------------

// 0x4a0bc0, line 1893. Captures this element in the parent; the argument is
// not used.
void TElement::SetCapture(TComponent *Component)
{
	TBEGIN
	if (GetParent() != NULL)
		GetParent()->SetCapture(this);
	TEND
}

// 0x4a0d60, line 1905
void TElement::ReleaseCapture(TComponent *Component)
{
	TBEGIN
	if (GetParent() != NULL)
		GetParent()->ReleaseCapture(this);
	TEND
}

// --- TCallback --------------------------------------------------------------

// 0x4a0f00, line 1931
void TCallback::Send()
{
	TBEGIN
	if (Receiver != NULL)
		Receiver->HandleEvent(Event);
	TEND
}

// --- Hotkeys --------------------------------------------------------------

// 0x4a10a0, line 1965. Replaces the callback of a key that is already there.
void AddHotkey(int Key, TCallback Callback)
{
	int i;
	TBEGIN
	for (i = 0; i < NumberOfHotkeys; i++) {
		if (Key == Hotkeys(i).Key) {
			Hotkeys(i).Callback = Callback;
			return;
		}
	}
	Hotkeys(NumberOfHotkeys).Key = Key;
	Hotkeys(NumberOfHotkeys).Callback = Callback;
	NumberOfHotkeys++;
	TEND1(i)
}

// 0x4a12a0, lines 1983. The last entry takes the place of the removed one.
void RemoveHotkey(int Key)
{
	int i;
	TBEGIN
	for (i = 0; i < NumberOfHotkeys; i++) {
		if (Key == Hotkeys(i).Key) {
			Hotkeys(i) = Hotkeys(NumberOfHotkeys - 1);
			NumberOfHotkeys--;
			return;
		}
	}
	TEND2(Key, i)
}

// 0x4a1480, line 2004. Called by Control.cpp's KeyPress when no modal window
// is open. Flag 0x200 is ignored and lower-case letters count as upper case.
bool CheckHotkey(int Key)
{
	int i;
	TBEGIN
	if ((Key & 0x200) != 0)
		Key -= 0x200;
	if ((Key & 0x1ff) > 0x60 && (Key & 0x1ff) < 0x7b)
		Key -= 0x20;
	for (i = 0; i < NumberOfHotkeys; i++) {
		if (Key == Hotkeys(i).Key) {
			Hotkeys(i).Callback.Send();
			return true;
		}
	}
	return false;
	TEND2(Key, i)
}
