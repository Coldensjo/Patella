// The shared fakes of the DialogElements.cpp classes for the tests of the window
// files (dialogwindowtest.cpp, contextmenustest.cpp, consolewindowtest.cpp,
// contentwindowstest.cpp, mapwindowtest.cpp, mainwindowtest.cpp).
//
// Before DialogElements.cpp was reconstructed its classes were sketches, and
// those tests defined the sketches' members as fakes. Now the fakes are
// these mirror classes: the sketches' layouts and virtual slots (the real
// classes' sizes, the members the window files use at the real offsets),
// renamed. PatchElementFakes redirects the image's constructors and members
// and our real ones (constructors through their decorated names) to them, so
// that both sides build the same fakes; each test calls it with its own
// Redirect (undone with its other patches). The members that are inline in
// DialogElements.h (TObjectIcon's and TSeparator's constructors,
// TMultiStateButton::Reset) are inlined into our callers and cannot be
// redirected: they run for real on both sides (the image's for the image's
// objects).
//
// Each fake is defined in the test file that introduced it.

#pragma once

#include "common.h"
#include "GUI.h"
#include "DialogElements.h"

// dialogwindowtest.cpp (reports through difftest.h's DialogFakes).
class TFakeResizer : public TElement {
public:
	TFakeResizer(TComponent *_ToResize, bool Param2, bool Param3, bool Param4);
	uint8 Data24[0x38 - 0x24];
};

// consolewindowtest.cpp: the button keeps its callback in Data2C.
class TFakeSkinButton : public TElement {
public:
	TFakeSkinButton(int x, int y, int _ButtonUp, int _ButtonDown, TCallback Callback, const char *Tooltip, int Param);
	void SetSkins(int _ButtonUp, int _ButtonDown);       // dialogwindowtest.cpp
	void SetRadioButtonGroup(TRadioButtonGroup *Group);  // contentwindowstest.cpp
	int ButtonUp;    // +0x24
	int ButtonDown;  // +0x28
	uint8 Data2C[0x38 - 0x2c];
	TRadioButtonGroup *RadioButtonGroup; // +0x38
	const char *Tooltip; // +0x3c
	uint8 Data40[0x44 - 0x40];
};
class TFakeToggleButton : public TFakeSkinButton {
public:
	TFakeToggleButton(int x, int y, int _ButtonUp, int _ButtonDown, TCallback Callback, const char *Tooltip1,
		const char *Tooltip2, int Param);
	bool State;      // +0x44
	uint8 Data45[0x50 - 0x45];
};
class TFakeMultiStateButton : public TFakeSkinButton {
public:
	TFakeMultiStateButton(int x, int y, const char *Tooltip, int _States, ...);
	int State;        // +0x44
	int States;       // +0x48
	int *ButtonUps;   // +0x4c
	int *ButtonDowns; // +0x50
	uint8 Data54[0x58 - 0x54];
};
class TFakeScrollBarRange {
public:
	void SetRange(int Minimum, int Maximum, int _Value, int Visible); // consolewindowtest.cpp
	void SetValue(int _Value);                                        // contentwindowstest.cpp
	uint8 Data00[0x30];
	int Value;       // +0x30
};
class TFakeScrollBar : public TWindow {
public:
	TFakeScrollBar(TRect Area, int Param2, int Param3, int Param4, int Param5, int Step);
	TFakeScrollBarRange *Range; // +0x2c
	uint8 Data30[0x38 - 0x30];
};
class TFakeEditLine : public TWindow {
public:
	TFakeEditLine(TRect Area, int MaxLength, int Param);
	void SetActive(bool Active);
	bool HasSelection() { return Data5C != -1 && Data60 != 0; }
	virtual void SetText(const char *Text);        // +0xa0
	virtual void Clear();                          // +0xa4
	virtual void UnknownA8(); virtual void UnknownAC(); virtual void UnknownB0(); virtual void UnknownB4();
	virtual void SetDisabled(bool Disabled);       // +0xb8
	virtual void UnknownBC();
	virtual void SetFont(TFont Font);              // +0xc0
	char *Text;      // +0x2c
	uint8 Data30[0x5c - 0x30];
	int Data5C;      // +0x5c
	int Data60;      // +0x60
	uint8 Data64[2];
	bool Data66;     // +0x66
	uint8 Data67[0x68 - 0x67];
};

// contextmenustest.cpp
class TFakeFormattedText : public TWindow {
public:
	TFakeFormattedText(const char *_Text);
	uint8 Data2C[0x38 - 0x2c];
};

// mainwindowtest.cpp and contentwindowstest.cpp (the second text button, the
// text toggle button, the label's area constructor and SetText, the skin icon).
class TFakeBorder : public TWindow {
public:
	TFakeBorder(TWindow *_Window, int Param);
	TFakeBorder(TWindow *_Window, int Skin1, int Skin2, int Skin3, int Skin4, int Skin5, int Skin6, int Skin7,
		int Skin8, int Skin9, int Border);
	void FitToWindow();
	uint8 Data2C[0x68 - 0x2c];
};
class TFakeTextButton : public TElement {
public:
	TFakeTextButton(int x, int y, const char *_Text, int Skin1, int Skin2, TCallback Callback, const char *Param);
	TFakeTextButton(int x, int y, const char *_Text, TCallback Callback, const char *Tooltip);
	uint8 Data24[0x78 - 0x24];
};
class TFakeTextToggleButton : public TFakeTextButton {
public:
	TFakeTextToggleButton(int x, int y, const char *_Text, int Skin1, int Skin2, TCallback Callback,
		const char *Tooltip1, const char *Tooltip2, bool _State);
	bool State;               // +0x78
	const char *Tooltip1;     // +0x7c
	const char *Tooltip2;     // +0x80
};
class TFakeLabel : public TElement {
public:
	TFakeLabel(TFont Font, const char *_Text);
	TFakeLabel(int x, int y, int Width, int Height, TFont Font, const char *_Text, bool Param);
	virtual void SetText(const char *_Text, bool Param); // +0x84
	uint8 Data24[0x4c - 0x24];
};
class TFakeSkinIcon : public TElement {
public:
	TFakeSkinIcon(int Skin);
	int Skin;        // +0x24
};

// mapwindowtest.cpp: the hint button is never made.
class TFakeBlinkingButton : public TElement {
public:
	TFakeBlinkingButton(int x, int y, int Skin1, int Skin2, int Skin3, int Skin4, TCallback Callback,
		const char *Tooltip);
	uint8 Data24[0x5c - 0x24];
};

// Redirects the image's and our constructors and members to the fakes
// (elementfakes.cpp).
void PatchElementFakes(void (*Redirect)(void *Function, void *Target));
