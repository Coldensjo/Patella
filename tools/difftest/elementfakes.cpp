// The redirects of the DialogElements.cpp fakes (elementfakes.h).

#include "elementfakes.h"
#include <new>
#include <stdint.h>

template <class T> static void *Member(T Function) { return *(void **)&Function; }

// Our constructors (a constructor has no address in C++).
extern "C" void OurResizerConstructor();
extern "C" void OurSkinButtonConstructor();
extern "C" void OurToggleButtonConstructor();
extern "C" void OurMultiStateButtonConstructor();
extern "C" void OurScrollBarConstructor();
extern "C" void OurEditLineConstructor();
extern "C" void OurFormattedTextConstructor();
extern "C" void OurBorderConstructor1();
extern "C" void OurBorderConstructor2();
extern "C" void OurTextButtonConstructor1();
extern "C" void OurTextButtonConstructor2();
extern "C" void OurTextToggleButtonConstructor();
extern "C" void OurLabelConstructor();
extern "C" void OurLabelAreaConstructor();
extern "C" void OurSkinIconConstructor();
extern "C" void OurBlinkingButtonConstructor();
#pragma comment(linker, "/alternatename:_OurResizerConstructor=??0TResizer@@QAE@PAVTComponent@@_N11@Z")
#pragma comment(linker, "/alternatename:_OurSkinButtonConstructor=??0TSkinButton@@QAE@HHHHUTCallback@@PBD_N@Z")
#pragma comment(linker, "/alternatename:_OurToggleButtonConstructor=??0TToggleButton@@QAE@HHHHUTCallback@@PBD1_N@Z")
#pragma comment(linker, "/alternatename:_OurMultiStateButtonConstructor=??0TMultiStateButton@@QAA@HHPBDHHHUTCallback@@ZZ")
#pragma comment(linker, "/alternatename:_OurScrollBarConstructor=??0TScrollBar@@QAE@UTRect@@HHHHH@Z")
#pragma comment(linker, "/alternatename:_OurEditLineConstructor=??0TEditLine@@QAE@UTRect@@H_N@Z")
#pragma comment(linker, "/alternatename:_OurFormattedTextConstructor=??0TFormattedText@@QAE@PBD@Z")
#pragma comment(linker, "/alternatename:_OurBorderConstructor1=??0TBorder@@QAE@PAVTWindow@@H@Z")
#pragma comment(linker, "/alternatename:_OurBorderConstructor2=??0TBorder@@QAE@PAVTWindow@@HHHHHHHHHH@Z")
#pragma comment(linker, "/alternatename:_OurTextButtonConstructor1=??0TTextButton@@QAE@HHPBDHHUTCallback@@0@Z")
#pragma comment(linker, "/alternatename:_OurTextButtonConstructor2=??0TTextButton@@QAE@HHPBDUTCallback@@0@Z")
#pragma comment(linker, "/alternatename:_OurTextToggleButtonConstructor=??0TTextToggleButton@@QAE@HHPBDHHUTCallback@@00_N@Z")
#pragma comment(linker, "/alternatename:_OurLabelConstructor=??0TLabel@@QAE@UTFont@@PBD@Z")
#pragma comment(linker, "/alternatename:_OurLabelAreaConstructor=??0TLabel@@QAE@HHHHUTFont@@PBD_N@Z")
#pragma comment(linker, "/alternatename:_OurSkinIconConstructor=??0TSkinIcon@@QAE@H@Z")
#pragma comment(linker, "/alternatename:_OurBlinkingButtonConstructor=??0TBlinkingButton@@QAE@HHHHHHUTCallback@@PBD@Z")

// The constructors of both sides build the fakes (thiscall: this in ECX).
static void *__fastcall NewResizer(void *This, void *, TComponent *ToResize, bool P2, bool P3, bool P4)
{
	return new (This) TFakeResizer(ToResize, P2, P3, P4);
}
static void *__fastcall NewSkinButton(void *This, void *, int x, int y, int Up, int Down, TCallback Callback,
	const char *Tooltip, int Param)
{
	return new (This) TFakeSkinButton(x, y, Up, Down, Callback, Tooltip, Param);
}
static void *__fastcall NewToggleButton(void *This, void *, int x, int y, int Up, int Down, TCallback Callback,
	const char *Tooltip1, const char *Tooltip2, int Param)
{
	return new (This) TFakeToggleButton(x, y, Up, Down, Callback, Tooltip1, Tooltip2, Param);
}
// consolewindowtest.cpp: __cdecl with this as its first argument.
void *__cdecl NewMultiStateButton(void *This, int x, int y, const char *Tooltip, int States, ...);
static void *__fastcall NewScrollBar(void *This, void *, TRect Area, int P2, int P3, int P4, int P5, int P6)
{
	return new (This) TFakeScrollBar(Area, P2, P3, P4, P5, P6);
}
static void *__fastcall NewEditLine(void *This, void *, TRect Area, int MaxLength, int Param)
{
	return new (This) TFakeEditLine(Area, MaxLength, Param);
}
static void *__fastcall NewFormattedText(void *This, void *, const char *Text)
{
	return new (This) TFakeFormattedText(Text);
}
static void *__fastcall NewBorder1(void *This, void *, TWindow *W, int P) { return new (This) TFakeBorder(W, P); }
static void *__fastcall NewBorder2(void *This, void *, TWindow *W, int S1, int S2, int S3, int S4, int S5, int S6,
	int S7, int S8, int S9, int B)
{
	return new (This) TFakeBorder(W, S1, S2, S3, S4, S5, S6, S7, S8, S9, B);
}
static void *__fastcall NewTextButton1(void *This, void *, int x, int y, const char *Text, int S1, int S2, TCallback C,
	const char *P)
{
	return new (This) TFakeTextButton(x, y, Text, S1, S2, C, P);
}
static void *__fastcall NewTextButton2(void *This, void *, int x, int y, const char *Text, TCallback C,
	const char *Tooltip)
{
	return new (This) TFakeTextButton(x, y, Text, C, Tooltip);
}
static void *__fastcall NewTextToggleButton(void *This, void *, int x, int y, const char *Text, int S1, int S2,
	TCallback C, const char *T1, const char *T2, bool State)
{
	return new (This) TFakeTextToggleButton(x, y, Text, S1, S2, C, T1, T2, State);
}
static void *__fastcall NewLabel(void *This, void *, TFont Font, const char *Text)
{
	return new (This) TFakeLabel(Font, Text);
}
static void *__fastcall NewLabelArea(void *This, void *, int x, int y, int Width, int Height, TFont Font,
	const char *Text, bool Param)
{
	return new (This) TFakeLabel(x, y, Width, Height, Font, Text, Param);
}
static void *__fastcall NewSkinIcon(void *This, void *, int Skin) { return new (This) TFakeSkinIcon(Skin); }
static void *__fastcall NewBlinkingButton(void *This, void *, int x, int y, int S1, int S2, int S3, int S4,
	TCallback C, const char *Tooltip)
{
	return new (This) TFakeBlinkingButton(x, y, S1, S2, S3, S4, C, Tooltip);
}

void PatchElementFakes(void (*Redirect)(void *Function, void *Target))
{
	struct { uint32 Image; void *Ours; void *Fake; } Table[] = {
		{0x453be0, (void *)OurResizerConstructor, (void *)NewResizer},
		{0x466860, (void *)OurSkinButtonConstructor, (void *)NewSkinButton},
		{0x4671e0, (void *)OurToggleButtonConstructor, (void *)NewToggleButton},
		{0x4672d0, (void *)OurMultiStateButtonConstructor, (void *)NewMultiStateButton},
		{0x469a80, (void *)OurScrollBarConstructor, (void *)NewScrollBar},
		{0x467b50, (void *)OurEditLineConstructor, (void *)NewEditLine},
		{0x465e60, (void *)OurFormattedTextConstructor, (void *)NewFormattedText},
		{0x4547f0, (void *)OurBorderConstructor1, (void *)NewBorder1},
		{0x454e30, (void *)OurBorderConstructor2, (void *)NewBorder2},
		{0x466bc0, (void *)OurTextButtonConstructor1, (void *)NewTextButton1},
		{0x466ed0, (void *)OurTextButtonConstructor2, (void *)NewTextButton2},
		{0x467230, (void *)OurTextToggleButtonConstructor, (void *)NewTextToggleButton},
		{0x465b40, (void *)OurLabelConstructor, (void *)NewLabel},
		{0x465510, (void *)OurLabelAreaConstructor, (void *)NewLabelArea},
		{0x457710, (void *)OurSkinIconConstructor, (void *)NewSkinIcon},
		{0x466b20, (void *)OurBlinkingButtonConstructor, (void *)NewBlinkingButton},
		{0x45bc80, Member(&TEditLine::SetActive), Member(&TFakeEditLine::SetActive)},
		{0x460450, Member(&TScrollBarRange::SetRange), Member(&TFakeScrollBarRange::SetRange)},
		{0x460710, Member(&TScrollBarRange::SetValue), Member(&TFakeScrollBarRange::SetValue)},
		{0x458b20, Member(&TSkinButton::SetSkins), Member(&TFakeSkinButton::SetSkins)},
		{0x458de0, Member(&TSkinButton::SetRadioButtonGroup), Member(&TFakeSkinButton::SetRadioButtonGroup)},
		{0x455500, Member(&TBorder::FitToWindow), Member(&TFakeBorder::FitToWindow)},
	};
	for (auto &T : Table) {
		Redirect((void *)(uintptr_t)T.Image, T.Fake);
		Redirect(T.Ours, T.Fake);
	}
}
