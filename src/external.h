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

// DialogElements.cpp: the dialog controls are in DialogElements.h, which its
// users include directly (it includes this header for TObject).

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

// The engines' surface classes (surfaceDX9.cpp, SurfaceGL.cpp; SurfaceDX.cpp's
// TSurfaceDX is in SurfaceDX.h).
class TSurfaceGL : public TSurface {       // 0x498 bytes
public:
	TSurfaceGL();                          // 0x4fb670
	void SetPixel(int Surface, int x, int y, TRGBColor Color);
	void FillRectangleClipped(int Surface, TRect Rect, TRGBColor Color, TRect Clip);
	void FillRectangle(int Surface, TRect Rect, TRGBColor Color);
	void DrawHorizontalLine(int Surface, int x, int y, int Width, TRGBColor Color); void DrawVerticalLine(int Surface, int x, int y, int Height, TRGBColor Color);
	void DrawFrameClipped(int Surface, TRect Rect, int LineWidth, TRGBColor Color, TRect Clip);
	void DrawFrame(int Surface, TRect Rect, int LineWidth, TRGBColor Color);
	void WriteImage(int ToSurface, int x, int y, int Width, int Height, uint8 *Data);
	void WriteMaskedImage(int ToSurface, int x, int y, int Width, int Height, uint8 *Data, uint8 *Mask,
		TRGBColor *Colors);
	void BlitRect(int ToSurface, int x, int y, int FromSurface, TRect Source);
	void Blit(int ToSurface, int x, int y, int FromSurface, int FromX, int FromY, int Width, int Height,
		TRGBColor ColorKey);
	void BlitRectClipped(int ToSurface, int x, int y, int FromSurface, int FromX, int FromY, int Width,
		int Height, TRect Clip);
	void BlitClipped(int ToSurface, int x, int y, int FromSurface, int FromX, int FromY, int Width,
		int Height, TRGBColor ColorKey, TRect Clip);
	void StretchBlitRect(int ToSurface, TRect Destination, int FromSurface, TRect Source);
	void StretchBlitUnclipped(int ToSurface, int x, int y, int Width, int Height, int FromSurface, int FromX,
		int FromY, int FromWidth, int FromHeight, TRGBColor ColorKey, TRGBColor Highlight);
	void StretchBlit(int ToSurface, int x, int y, int Width, int Height, int FromSurface, int FromX,
		int FromY, int FromWidth, int FromHeight, TRGBColor ColorKey, TRGBColor Highlight, TRect Clip);
	void ShadeRectangles(int Surface, int Count, TRect *Rects, TRGBColor *Colors);
	void SetSourceColorKey(int Surface, TRGBColor ColorKey); bool ClipRect(TRect Clip, RECT *Destination, RECT *Source);
	void ResetFrameTimer(); void WaitForNextFrame(); void UpdateWindowRect();
	void InitDisplay(int VMode, TFullscreenMode *GMode); void ExitDisplay(); void CreateSurfaces();
	void ResetDisplay(int VMode, TFullscreenMode *GMode); void RestoreSurfaces(int ViewMode, TFullscreenMode *Mode);
	void FinishFrame();
	bool Init(HWND *Window, int ViewMode, TFullscreenMode *Resolution, bool Force);
	int GetFullscreenModes(TFullscreenMode **Modes); int GetCacheSurfaces(); void GetSurfaceRect(int Surface, TRect *Rect);
	bool SurfaceRecreated(int Surface);
	bool IsUnavailable(); bool CanShadeRectangles(); int GetSurfaceMemory(int Index); void SetFramerate(int Framerate); double GetFPS();
	void InitSurfaces(); void ExitSurfaces();
	uint8 DataGL[0x498 - 0x70];
};
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
