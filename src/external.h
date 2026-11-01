// Functions and data from source files that are not reconstructed yet.
// Each entry gives its address in Tibia.exe and where it sits. Move a
// declaration into the real header when its file is reconstructed.

#pragma once

#include "common.h"
#include "Talk.h"

// First 16 bytes of the last received packet, at 0x5d1644 (owner unknown).
extern uint8 PacketHeader[16];

// Objects.cpp: a reference to an object (12 bytes, passed by value). Type
// is named in MapWindow.cpp's "SrcObj.Type!=CREATURE" (CREATURE is 99);
// the other field names are ours. The default constructor clears the object:
// Map.cpp's TMapPoint, ten TObjects and plain ints, has a compiler-generated
// constructor (0x4b0b40) that zeroes exactly the ten objects. Written with
// default member initializers so that the type stays an aggregate.
// MapWindow.cpp compares objects with numbers ("SrcObj!=0", "Obj!=CREATURE")
// and prints one as the int value of TEND1(Obj): the original converts a
// TObject to its Type.
struct TObject {
	int Type = 0;
	int Data1 = 0;
	int Data2 = 0;
	operator int() const { return Type; }
};

// sysinc.cpp
#include "sysinc.h"

// Library region: Henry Spencer's regexp ("regexp too big", "corrupted program").
struct regexp;
regexp *regcomp(char *exp);                     // 0x50fa40, the result is malloc'd
int regexec(regexp *prog, char *string);        // 0x50f360, 1 on a match

// --- added for Control.cpp ---

struct TFullscreenMode; // Preferences.h
// Functions in Communication.cpp, Status.cpp and Preferences.cpp keep their
// FUN_ names until those files are merged; so do the other callees whose
// purpose is not certain. Names from the binary: curEngine, ModalWindow,
// GetActionState and GetAccountCharacters (assertion and catch strings), the
// TSurface methods ExitSurfaces, TestEngine and InitSurfaces and the function
// SurfaceExitInstance (error messages in ChangeEngine). The other class,
// member and variable names are ours; member offsets are from the reads.

// GUI.cpp: the window classes (TComponent, TElement, TWindow); GUI.h includes
// graphics.h, which has the graphics types TRect, TRGBColor and TFont.
#include "GUI.h"

// ConsoleWindow.cpp: the console (ConsoleWindow.h). Its DialogElements.cpp
// parts are below.
#include "ConsoleWindow.h"

// DialogElements.cpp: the two resizers of a TSubWindow (vtable 0x5612b0,
// 0x38 bytes); parameter name _ToResize from the assertion (line 74), the
// other three are flags (+0x34-+0x36). The class name is ours.
class TResizer : public TElement {
public:
	TResizer(TComponent *_ToResize, bool Param2, bool Param3, bool Param4); // 0x453be0, line 74
	uint8 Data24[0x38 - 0x24];
};

// DialogElements.cpp: a button drawn with two skins (pressed and released)
// that sends its callback (0x44 bytes, constructor 0x466860; vtable not
// known yet), TSubWindow's minimize button and the console's buttons. The
// class name is ours; the last parameter is 0 in ConsoleWindow.cpp and
// ContentWindows.cpp. A button of a radio group ("RadioButtonGroup==NULL",
// DialogElements.cpp line 1239) shows as pressed while it is the group's
// selection.
struct TRadioButtonGroup;
class TSkinButton : public TElement {
public:
	TSkinButton(int x, int y, int _ButtonUp, int _ButtonDown, TCallback Callback, const char *Tooltip,
		int Param);                                  // 0x466860
	void SetSkins(int _ButtonUp, int _ButtonDown);   // 0x458b20, lines 1228-1229
	void SetRadioButtonGroup(TRadioButtonGroup *Group); // 0x458de0, line 1239
	int ButtonUp;    // +0x24
	int ButtonDown;  // +0x28
	uint8 Data2C[0x38 - 0x2c];
	TRadioButtonGroup *RadioButtonGroup; // +0x38
	const char *Tooltip; // +0x3c
	uint8 Data40[0x44 - 0x40];
};

// DialogElements.h: the buttons of a radio group (0x28 bytes; the type name
// is ours). The constructor and the destructor are inline (ContentWindows.cpp's
// TCombatControls; the out-of-line vector constructor is 0x425e70); Selected
// is left uninitialised.
struct TRadioButtonGroup {
	TRadioButtonGroup() : Buttons(0, 0, 10, NULL) { Count = 0; }
	TSkinButton *Selected;         // +0x00
	vector<TSkinButton *> Buttons; // +0x04
	int Count;                     // +0x24
};

// DialogElements.cpp: a button with two states (vtable 0x561cd0, 0x50
// bytes, constructor 0x4671e0), the console's server message button; the
// tooltips belong to the two states. Class name ours.
class TToggleButton : public TSkinButton {
public:
	TToggleButton(int x, int y, int _ButtonUp, int _ButtonDown, TCallback Callback, const char *Tooltip1,
		const char *Tooltip2, int Param);            // 0x4671e0
	bool State;      // +0x44
	uint8 Data45[0x50 - 0x45];
};

// DialogElements.cpp: a button that steps through States states (vtable
// 0x560e10, 0x58 bytes), the console's volume. Its constructor 0x4672d0 is a
// varargs one (__cdecl, this pushed first): a skin pair and a callback per
// state. Reset (0x452790) goes back to state 0 and its skins. Class name ours.
class TMultiStateButton : public TSkinButton {
public:
	TMultiStateButton(int x, int y, const char *Tooltip, int States, ...); // 0x4672d0
	void Reset();                                    // 0x452790
	int State;       // +0x44
	int States;      // +0x48
	int *ButtonUps;  // +0x4c
	int *ButtonDowns; // +0x50
	uint8 Data54[0x58 - 0x54];
};

// DialogElements.cpp: a scroll bar (vtable 0x561f70, 0x38 bytes,
// constructor 0x469a80 with its area and, in ConsoleWindow.cpp, four zeros
// and true). Range (+0x2c) holds the position. Class and member names ours.
class TScrollBarRange {
public:
	void SetRange(int Minimum, int Maximum, int Value, int Visible); // 0x460450
	void SetValue(int _Value);       // 0x460710, kept within the range
	uint8 Data00[0x30];
	int Value;       // +0x30
};
// The last parameter is the step of a click (1 in ConsoleWindow.cpp, 10 or a
// part of a line or row in ContentWindows.cpp).
class TScrollBar : public TWindow {
public:
	TScrollBar(TRect Area, int Param2, int Param3, int Param4, int Param5, int Step); // 0x469a80
	TScrollBarRange *Range; // +0x2c
	uint8 Data30[0x38 - 0x30];
};

// ConsoleWindow.obj's copy of the text table (external.h of MainWindow.obj
// and MapWindow.obj above and below; names ours).
extern const char *TextCloseChannel;        // 0x5c044c, "Close this Channel (Ctrl+E)"
extern const char *TextShowServerMessages;  // 0x5c0450, "Show server messages in this channel (Ctrl+M)"
extern const char *TextHideServerMessages;  // 0x5c0454, "Hide server messages in this channel (Ctrl+M)"
extern const char *TextIgnorePlayers;       // 0x5c0458, "Ignore and unignore other players (Ctrl+I)"
extern const char *TextOpenNewChannel;      // 0x5c045c, "Open new channel (Ctrl+O)"
extern const char *TextAdjustVolume;        // 0x5c0460, "Adjust volume"
extern const char *TextMoveChannelsRight;   // 0x5c0464, "Move channels to the right"
extern const char *TextMoveChannelsLeft;    // 0x5c0468, "Move channels to the left"
extern const char *TextWaitForGamemaster;   // 0x5c05f8, "Please wait patiently for a gamemaster to reply."
extern const char *TextRequestSubmitted;    // 0x5c05fc, "You have already submitted a request. ..."
extern const char *TextHelpChannelWelcome;  // 0x5c0604, "Welcome to the help channel! ..."
extern const char *TextReportRuleViolation; // 0x5c0664, "Report Rule Violation"

// DialogWindow.cpp: TDialogWindow, TContentWindow and TSubWindow.
#include "DialogWindow.h"

// MainWindow.cpp: the main game window, the modal window (MainWindow.h).
#include "MainWindow.h"

// ContextMenus.cpp: the help bubble and the context menus are in
// ContextMenus.h, which its users include themselves (it needs Map.h, which
// includes this header).

// Surface.cpp: curEngine, TSurface and the surface functions (Surface.h).
#include "Surface.h"

// The engines' surface classes (SurfaceDX.cpp, surfaceDX9.cpp, SurfaceGL.cpp).
class TSurfaceGL : public TSurface {       // 0x498 bytes
public:
	TSurfaceGL();                          // 0x4fb670
	void SetPixel(int Surface, int x, int y, TRGBColor Color);
	void FillRectangleClipped(int Surface, TRect Rect, TRGBColor Color, TRect Clip);
	void FillRectangle(int Surface, TRect Rect, TRGBColor Color);
	void DrawHorizontalLine(int Surface, int x, int y, int Width, TRGBColor Color); void Unknown1C();
	void DrawFrameClipped(int Surface, TRect Rect, int LineWidth, TRGBColor Color, TRect Clip);
	void DrawFrame(int Surface, TRect Rect, int LineWidth, TRGBColor Color);
	void WriteImage(int ToSurface, int x, int y, int Width, int Height, uint8 *Data);
	void WriteMaskedImage(int ToSurface, int x, int y, int Width, int Height, uint8 *Data, uint8 *Mask,
		TRGBColor *Colors);
	void BlitRect(int ToSurface, int x, int y, int FromSurface, TRect Source);
	void Blit(int ToSurface, int x, int y, int FromSurface, int FromX, int FromY, int Width, int Height,
		TRGBColor ColorKey);
	void Unknown38();
	void BlitClipped(int ToSurface, int x, int y, int FromSurface, int FromX, int FromY, int Width,
		int Height, TRGBColor ColorKey, TRect Clip);
	void StretchBlitRect(int ToSurface, TRect Destination, int FromSurface, TRect Source); void Unknown44();
	void StretchBlit(int ToSurface, int x, int y, int Width, int Height, int FromSurface, int FromX,
		int FromY, int FromWidth, int FromHeight, TRGBColor ColorKey, TRGBColor Highlight, TRect Clip);
	void ShadeRectangles(int Surface, int Count, TRect *Rects, TRGBColor *Colors);
	void Unknown54(int Surface, TRGBColor ColorKey); void Unknown58(); void Unknown5C(); void Unknown60();
	void Unknown68(); void Unknown6C(); void Unknown70(); void Unknown74(); void Unknown78();
	void Unknown7C(); void FinishFrame();
	bool Init(HWND *Window, int ViewMode, TFullscreenMode *Resolution, bool Force);
	void Unknown8C(); int GetCacheSurfaces(); void GetSurfaceRect(int Surface, TRect *Rect);
	bool Unknown9C(int Value);
	bool IsUnavailable(); bool CanShadeRectangles(); void UnknownA8(); void UnknownAC(); double GetFPS();
	void InitSurfaces(); void ExitSurfaces();
	uint8 DataGL[0x498 - 0x44];
};
TSurface *FUN_004eca00();               // 0x4eca00, SurfaceDX.cpp: creates the DirectX 5 surface
TSurface *FUN_004f1430();               // 0x4f1430, surfaceDX9.cpp: creates the DirectX 9 surface
bool FUN_004efd10();                    // 0x4efd10, between SurfaceDX.cpp and surfaceDX9.cpp: DirectX 9 is installed
bool FUN_004f9710(HDC Context);         // 0x4f9710, between surfaceDX9.cpp and SurfaceGL.cpp: OpenGL works on the context

// Text table (owner unknown, .data 0x5c0c80-0x5c0d7c).
extern const char *TextCharacterListLoading; // 0x5c0ce8
extern const char *TextConnectingGameWorld;  // 0x5c0cec
extern const char *TextWaitForCharacterList; // 0x5c0cf0
extern const char *TextLinkCopyWarning;      // 0x5c0cfc
extern const char *TextNeverGiveAccount;     // 0x5c0d00
extern const char *TextRuleViolations;       // 0x5c0d78
// MapWindow.obj's copy of the same table (0x5c2180-0x5c22ac, the texts start
// earlier): every object that includes the text header has its own.
extern const char *TextReceiveHint;          // 0x5c2130, "Receive hint"
extern const char *TextSelectPlayerToTrade;  // 0x5c2258, "Select a player to trade with."

// MainWindow.obj's copy of the text table.
extern const char *TextNoBugReportRight;     // 0x5c1ffc, "You are not authorised to send bug reports."
extern const char *TextAccountURL;           // 0x5c2048, "http://www.tibia.com/home/?subtopic=account"

// Communication.cpp

// DialogElements.cpp: a frame of nine skins around one window (vtable
// 0x561358, 0x68 bytes); MainWindow.cpp's side bar and start window frame.
class TBorder : public TWindow {
public:
	TBorder(TWindow *_Window, int Param);             // 0x4547f0, line 166
	TBorder(TWindow *_Window, int Skin1, int Skin2, int Skin3, int Skin4, int Skin5, int Skin6,
		int Skin7, int Skin8, int Skin9, int Border); // 0x454e30, line 217
	void FUN_00455500();                              // 0x455500
	uint8 Data2C[0x68 - 0x2c];
};
// DialogElements.cpp: a button with a text (vtable 0x561c40, 0x78 bytes).
// The second constructor uses the skins 12 and 13 (ContentWindows.cpp).
class TTextButton : public TElement {
public:
	TTextButton(int x, int y, const char *_Text, int Skin1, int Skin2, TCallback Callback,
		int Param);                                   // 0x466bc0, lines 1378-1379
	TTextButton(int x, int y, const char *_Text, TCallback Callback, const char *Tooltip); // 0x466ed0, lines 1390-1391
	uint8 Data24[0x78 - 0x24];
};
// DialogElements.cpp: a text button that shows whether its window is open
// (vtable 0x561d58, 0x84 bytes; class name ours): ContentWindows.cpp's
// TButtonsWindow sets State while it draws; the tooltips are those of
// the two states.
class TTextToggleButton : public TTextButton {
public:
	TTextToggleButton(int x, int y, const char *_Text, int Skin1, int Skin2, TCallback Callback,
		const char *Tooltip1, const char *Tooltip2, bool _State); // 0x467230
	bool State;               // +0x78
	const char *Tooltip1;     // +0x7c
	const char *Tooltip2;     // +0x80
};
// DialogElements.cpp: a text (vtable 0x560bb8, 0x4c bytes). The second
// constructor takes an area; SetText (+0x84, 0x455e40) changes the text.
class TLabel : public TElement {
public:
	TLabel(TFont Font, const char *_Text);            // 0x465b40, line 376
	TLabel(int x, int y, int Width, int Height, TFont Font, const char *_Text, bool Param); // 0x465510, line 337
	virtual void SetText(const char *_Text, bool Param); // +0x84, 0x455e40
	uint8 Data24[0x4c - 0x24];
};
// DialogElements.cpp: one skin (vtable 0x5614f8, 0x28 bytes, sized to it;
// class name ours), the icons of ContentWindows.cpp's sub-windows.
class TSkinIcon : public TElement {
public:
	TSkinIcon(int Skin);                              // 0x457710, line 1002
	int Skin;        // +0x24
};
// DialogElements.cpp: an object (vtable 0x560d88, 0x50 bytes).
class TObjectIcon : public TElement {
public:
	TObjectIcon(TObject Obj, const TRGBColor *Color, int Param); // 0x4526c0
	uint8 Data24[0x50 - 0x24];
};

// DialogElements.cpp: an edit line (vtable 0x560e98, 0x68 bytes, class name
// ours, "_Edit" in ContextMenus.cpp), the console's Input. HandleEvent takes
// 16 copy, 17 cut, 18 paste, 19 delete, 20 select all. The constructor takes
// its area and the maximum length (255 in the console). Slot and member
// names are ours.
class TEditLine : public TWindow {
public:
	TEditLine(TRect Area, int MaxLength, int Param); // 0x467b50
	void SetActive(bool Active);     // 0x45bc80: +0x58; clears the selection when false
	// DialogElements.h inline (out-of-line copy 0x417520 in ConsoleWindow.obj).
	bool HasSelection() { return Data5C != -1 && Data60 != 0; }
	virtual void SetText(const char *Text);        // +0xa0
	virtual void Clear();                          // +0xa4
	virtual void UnknownA8(); virtual void UnknownAC(); virtual void UnknownB0(); virtual void UnknownB4();
	virtual void SetReadOnly(bool ReadOnly);       // +0xb8
	virtual void UnknownBC();
	virtual void SetFont(TFont Font);              // +0xc0
	char *Text;      // +0x2c
	uint8 Data30[0x5c - 0x30];
	int Data5C;      // +0x5c, -1 without a selection
	int Data60;      // +0x60, the selection's length (0 for none)
	uint8 Data64[2];
	bool Data66;     // +0x66, read only: no cut, paste or delete
	uint8 Data67[0x68 - 0x67];
};

// DialogElements.cpp: a text with formatting tokens (vtable 0x560c40, 0x38
// bytes), the help bubble's text. Class name ours.
class TFormattedText : public TWindow {
public:
	TFormattedText(const char *_Text);    // 0x465e60, line 508
	uint8 Data2C[0x38 - 0x2c];
};

// DialogElements.cpp: a line between groups of entries (vtable 0x5610b0,
// 0x28 bytes). Class name ours; the flag is true in ContextMenus.cpp.
class TSeparator : public TElement {
public:
	TSeparator(bool Horizontal);          // 0x452cb0
	bool Horizontal;                      // +0x24
};

// ContentWindows.obj's copy of the text table (0x5c0678-0x5c08c3; names
// ours). ContentWindows.cpp itself is in ContentWindows.h, which its users
// include directly (it needs Map.h, which includes this header).
extern const char *TextPoisoned;              // 0x5c0678, "You are poisoned"
extern const char *TextOnFire;                // 0x5c067c, "You are on fire"
extern const char *TextMagicShield;           // 0x5c0680, "You are protected by a magic shield"
extern const char *TextParalysed;             // 0x5c0684, "You are paralysed"
extern const char *TextHasted;                // 0x5c0688, "You are hasted"
extern const char *TextNoLogoutInFight;       // 0x5c068c, "You may not logout during a fight"
extern const char *TextDrunk;                 // 0x5c0690, "You are drunk"
extern const char *TextStruckByEnergy;        // 0x5c0694, "You are struck by magical energy"
extern const char *TextMayBeAttacked;         // 0x5c0698, "You may be attacked by other players"
extern const char *TextPartyMember;           // 0x5c069c, "You are a member of a party"
extern const char *TextAttackedUnmarked;      // 0x5c06a0, "You have attacked an unmarked player"
extern const char *TextKilledTooMany;         // 0x5c06a4, "You have killed too many unmarked players"
extern const char *TextMaximiseInventory;     // 0x5c06c8, "Maximise inventory"
extern const char *TextMinimiseInventory;     // 0x5c06cc, "Minimise inventory"
extern const char *TextCloseWindow;           // 0x5c06d0, "Close this window"
extern const char *TextMaximiseOrMinimise;    // 0x5c06d4, "Maximise or minimise window"
extern const char *TextShowHigherContainer;   // 0x5c06d8, "Show higher container"
extern const char *TextOpenSkillsWindow;      // 0x5c06dc, "Open skills window (Ctrl+S)"
extern const char *TextCloseSkillsWindow;     // 0x5c06e0, "Close skills window (Ctrl+S)"
extern const char *TextOpenBattleWindow;      // 0x5c06e4, "Open battle window (Ctrl+B)"
extern const char *TextCloseBattleWindow;     // 0x5c06e8, "Close battle window (Ctrl+B)"
extern const char *TextOpenVIPWindow;         // 0x5c06ec, "Open VIP window (Ctrl+P)"
extern const char *TextCloseVIPWindow;        // 0x5c06f0, "Close VIP window (Ctrl+P)"
extern const char *TextLeaveGame;             // 0x5c06f4, "Leave the game (Ctrl+Q or Ctrl+L)"
extern const char *TextCentreAroundPlayer;    // 0x5c06f8, "Centre around player"
extern const char *TextShowHigherFloor;       // 0x5c06fc, "Show higher floor (Alt+PageUp)"
extern const char *TextShowLowerFloor;        // 0x5c0700, "Show lower floor (Alt+PageDown)"
extern const char *TextZoomIn;                // 0x5c0704, "Zoom in (Alt+End)"
extern const char *TextZoomOut;               // 0x5c0708, "Zoom out (Alt+Home)"
extern const char *TextScrollAutomap;         // 0x5c070c, "Scroll automap (Alt+Cursor)"
extern const char *TextOffensiveFighting;     // 0x5c0710, "Offensive fighting"
extern const char *TextBalancedFighting;      // 0x5c0714, "Balanced fighting"
extern const char *TextDefensiveFighting;     // 0x5c0718, "Defensive fighting"
extern const char *TextStandWhileFighting;    // 0x5c071c, "Stand while fighting"
extern const char *TextChaseOpponent;         // 0x5c0720, "Chase opponent"
extern const char *TextCannotAttackUnmarked;  // 0x5c0724, "You cannot attack unmarked characters"
extern const char *TextCanAttackAny;          // 0x5c0728, "You can attack any character"
extern const char *TextStopCurrentAction;     // 0x5c072c, "Stop current action (ESC)"
extern const char *TextOpenOptions;           // 0x5c0730, "Open options"
extern const char *TextOpenHelpMenu;          // 0x5c0734, "Open help menu"
extern const char *TextCancelOffer;           // 0x5c0738, "Cancel offer"
extern const char *TextRejectOffer;           // 0x5c073c, "Reject offer"
extern const char *TextAcceptOffer;           // 0x5c0740, "Accept offer"
extern const char *TextPercentToGo;           // 0x5c0748, "You have %d percent to go"
extern const char *TextWaitForCounterOffer;   // 0x5c0758, "Please wait for a\ncounteroffer"
extern const char *TextWaitForPartner;        // 0x5c075c, "Please wait for \nyour partner to accept."

// DialogElements.cpp: a button that blinks until it is pressed (vtable
// 0x561b90, 0x5c bytes; base constructor 0x466860). MapWindow.cpp's hint
// button. The base class and the member layout are not known yet.
class TBlinkingButton : public TElement {
public:
	TBlinkingButton(int x, int y, int Skin1, int Skin2, int Skin3, int Skin4, TCallback Callback,
		const char *Tooltip); // 0x466b20
	uint8 Data24[0x5c - 0x24];
};

// Dialogs.cpp: each creates a dialog and returns it as the new modal window.
TWindow *FUN_0047f8a0(const char *Title, const char *Text);             // 0x47f8a0, message box
TWindow *FUN_0047faf0(const char *Title, const char *Text);             // 0x47faf0
TWindow *FUN_0047fd30(const char *Title, const char *Text, TWindow *Parent); // 0x47fd30, "Abort"
TWindow *FUN_0047ff60(const char *Title, const char *Text, INT_PTR Param);   // 0x47ff60, "Abort"
TWindow *FUN_004801c0(const char *Title, INT_PTR Param1, INT_PTR Param2);    // 0x4801c0, "Abort"
TWindow *FUN_004803f0(const char *Title, const char *Text, TWindow *Parent, int Param4,
	int Param5);                                                         // 0x4803f0
TWindow *FUN_00480640(const char *Title, const char *Text, TWindow *Parent); // 0x480640, "Logout"
TWindow *FUN_004808b0(void (*OpenURL)(const char *URL));                // 0x4808b0, "Official Website"
TWindow *FUN_00480cd0();                                                // 0x480cd0, "Move Objects"
TWindow *FUN_00480f40(void (*OpenURL)(const char *URL));                // 0x480f40, "Enter Game"
TWindow *FUN_00481160(INT_PTR Timeout);                                 // 0x481160, "Select Character"
TWindow *FUN_00481370(INT_PTR Param1, INT_PTR Param2, INT_PTR Param3);  // 0x481370
TWindow *FUN_004817c0(INT_PTR Param1, INT_PTR Param2);                  // 0x4817c0, "Edit List"
TWindow *FUN_004819f0(INT_PTR Param);                                   // 0x4819f0, "Select Outfit"
TWindow *FUN_00481bf0();                                                // 0x481bf0, "Reporting"
TWindow *FUN_00481e00(INT_PTR Param1, INT_PTR *Param2, INT_PTR Param3); // 0x481e00, "New Channel"
TWindow *FUN_00482010();                // 0x482010, "Invite player to private chat channel"
TWindow *FUN_00482210();                // 0x482210, "Exclude player from private chat channel"
TWindow *FUN_00482410();                // 0x482410, "Edit Ignore List and White List"
TWindow *FUN_00482640();                // 0x482640, "Report Rule Violation"
TWindow *FUN_00482870();                // 0x482870, "Add to VIP list"
TWindow *FUN_00482a70(INT_PTR CharacterID);                             // 0x482a70, "Change player in VIP list"
TWindow *FUN_00482c80(INT_PTR Param1, INT_PTR DataSize);                // 0x482c80, "Updating ... Please wait"
TWindow *FUN_00482ea0(void (*OpenURL)(const char *URL));                // 0x482ea0
TWindow *FUN_004830c0();                                                // 0x4830c0, "Hints for Tibia"
TWindow *FUN_004832e0(INT_PTR Param1, INT_PTR Param2);                  // 0x4832e0, "Edit Mark"
TWindow *FUN_00483500();                                                // 0x483500, "Options"
TWindow *FUN_00483730(void (*OpenURL)(const char *URL));                // 0x483730, "General Options"
TWindow *FUN_00483950(void (*OpenURL)(const char *URL), void (*SetDisplayMode)(int Mode),
	TWindow *Parent);                                                   // 0x483950, "Graphics Options"
TWindow *FUN_00483b80(void (*OpenURL)(const char *URL), INT_PTR Param); // 0x483b80, "Advanced Graphics Options"
TWindow *FUN_00483dc0(void (*OpenURL)(const char *URL));                // 0x483dc0, "Console Options"
TWindow *FUN_00483fe0(void (*OpenURL)(const char *URL));                // 0x483fe0, "Network Options"
TWindow *FUN_00484200(void (*OpenURL)(const char *URL));                // 0x484200, "Hotkey Options"
TWindow *FUN_00484400();                                                // 0x484400, "Error Report"
TWindow *FUN_00485290(INT_PTR Param1, INT_PTR Param2);                  // 0x485290, "Rule Violation"


// Map.cpp

// Objects.cpp

// Preferences.cpp


// Script.cpp

// Status.cpp. Action state: 0 idle, 1 left button down, 2 right button down,
// 3 both down, 4 and 5 one of both released, 7 and 8 set by events 0x2c and
// 0x2d, 9 help mode, 10 dialog open, 11 context menu open.


// --- added for Player.cpp ---

#include <ostream>

// MainFrm.cpp (MainFrm.h)
#include "MainFrm.h"

// Owner unknown (.data after Status.cpp's 0x5c2560): set by "engine N" on
// the command line, read by SurfaceInitInstance; -1 = none.
extern int DAT_005c2564;                // 0x5c2564


// Owner unknown: a table of message texts in .data (0x5c2480-0x5c2517).
extern const char *DAT_005c249c;        // 0x5c249c, "The Tibia client encountered a critical error ..."
