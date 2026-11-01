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
struct TObject {
	int Type = 0;
	int Data1 = 0;
	int Data2 = 0;
};

// sysinc.cpp
#include "sysinc.h"

// Library region: Henry Spencer's regexp ("regexp too big", "corrupted program").
struct regexp;
regexp *regcomp(char *exp);                     // 0x50fa40, the result is malloc'd
int regexec(regexp *prog, char *string);        // 0x50f360, 1 on a match

// --- added for Communication.cpp ---

extern uint32 DAT_005cc758; // 0x5cc758, graphics.cpp; Tibia.pic signature
extern uint32 DAT_005ccb64; // 0x5ccb64, graphics.cpp; Tibia.spr signature

// --- added for Control.cpp ---

struct TFullscreenMode; // Preferences.h
// Functions in Communication.cpp, Status.cpp and Preferences.cpp keep their
// FUN_ names until those files are merged; so do the other callees whose
// purpose is not certain. Names from the binary: curEngine, ModalWindow,
// GetActionState and GetAccountCharacters (assertion and catch strings), the
// TSurface methods ExitSurfaces, TestEngine and InitSurfaces and the function
// SurfaceExitInstance (error messages in ChangeEngine). The other class,
// member and variable names are ours; member offsets are from the reads.

// GUI.cpp: the window classes (TComponent, TElement, TWindow), and for now
// the graphics types TRect, TRGBColor and TFont (GUI.h).
#include "GUI.h"

// ConsoleWindow.cpp. Not instantiated here; the arrays stand for the members
// before the ones used.
struct TConsoleData34 {
	int Data00[11];
	int Data2C; // +0x2c, passed to the console text context menu
};

class TConsoleWindow {
public:
	void FUN_00423060(TTalkBuffer *TextBuffer, const char *Title); // 0x423060, lines 1977-1988
	TTalkBuffer *FUN_00425070(int RecvMode, int SendMode, INT_PTR Param3, INT_PTR Param4,
		INT_PTR Param5);                                            // 0x425070, line 1965
	void FUN_00425330(INT_PTR Param);                               // 0x425330, lines 2033-2044

	int Data00[13];
	TConsoleData34 *Data34; // +0x34
};

// MainWindow.cpp: the main game window (0x5d16b0).
class TMainWindow : public TWindow {
public:
	void FUN_004ac9e0(int Container);  // 0x4ac9e0, line 511
	void FUN_004acdb0(int Container);  // 0x4acdb0
	void FUN_004acf60();               // 0x4acf60, line 576
	void FUN_004ad650(int Value);      // 0x4ad650
	void FUN_004af5e0(int InGame);     // 0x4af5e0, 1 entering the game, 0 leaving it
	void FUN_004afec0();               // 0x4afec0, lines 560-561
	void FUN_004b0230();               // 0x4b0230

	int Data2C[4];                     // +0x2c, after TWindow's members
	TConsoleWindow *Console;           // +0x3c
};

// Owner unknown (after Control.cpp's data); also used by GUI and MainWindow code.
extern TWindow *ModalWindow;      // 0x5d16ac, name from an assertion string
extern TMainWindow *MainWindow;   // 0x5d16b0

// ContextMenus.cpp: the context menus and the help bubble, all modal windows.
class TContextMenu : public TWindow {
public:
	void FUN_00442110(TWindow *Parent); // 0x442110, lines 257-268 ("newX>=0")
};
class THelpBubble : public TContextMenu {         // size 0x30
public:
	THelpBubble(const char *HelpText);             // 0x441d60, line 106
};
class TObjectContextMenu : public TContextMenu {  // size 0x70; "Attack", "Follow"
public:
	TObjectContextMenu(int x, int y, int Width, int Height); // 0x443130, lines 560-643
};
class TCreatureContextMenu : public TContextMenu { // size 0x38; battle list
public:
	TCreatureContextMenu(INT_PTR CreatureID, int x, int y, int Width, int Height); // 0x443960, line 845
};
class TBuddyContextMenu : public TContextMenu {   // size 0x38; VIP list
public:
	TBuddyContextMenu(INT_PTR _CharacterID, int x, int y, int Width, int Height); // 0x443e20
};
class TTextContextMenu : public TContextMenu {    // size 0x3c; console text
public:
	TTextContextMenu(int Unknown, INT_PTR _LineNumber, int x, int y, int Width,
		int Height);                                 // 0x444160, lines 1056-1057
};
class TRequestContextMenu : public TContextMenu { // size 0x3c; "Process %s", "Remove %s"
public:
	TRequestContextMenu(TConsoleWindow *_ConsoleWindow, INT_PTR _LineNumber, int x, int y,
		int Width, int Height);                      // 0x444720, line 1181
};
class TChannelContextMenu : public TContextMenu { // size 0x3c; "Clear Window", "Save Window"
public:
	TChannelContextMenu(TConsoleWindow *_ConsoleWindow, int x, int y, int Width,
		int Height);                                 // 0x444a70, lines 1248-1253
};
class TEditContextMenu : public TContextMenu {    // size 0x38; "Paste", "Select All"
public:
	TEditContextMenu(INT_PTR _Edit, int x, int y, int Width, int Height); // 0x444eb0, line 1324
};
class TMarkContextMenu : public TContextMenu {    // size 0xb4; "Set Mark", "Edit Mark"
public:
	TMarkContextMenu(INT_PTR _Data, int x, int y, int Width, int Height); // 0x4451e0, line 1379
};
class TSkillContextMenu : public TContextMenu {   // size 0x38; "Hide Level", "Show Club Fighting"
public:
	TSkillContextMenu(INT_PTR Data, int x, int y, int Width, int Height); // 0x4454f0, line 1442
};

// Surface.cpp: curEngine, TSurface and the surface functions (Surface.h).
#include "Surface.h"

// The engines' surface classes (SurfaceDX.cpp, surfaceDX9.cpp, SurfaceGL.cpp).
class TSurfaceGL : public TSurface {       // 0x498 bytes
public:
	TSurfaceGL();                          // 0x4fb670
	void Unknown0C(); void Unknown10(); void FillRectangle(int Surface, TRect Rect, TRGBColor Color);
	void Unknown18(); void Unknown1C(); void Unknown20();
	void DrawFrame(int Surface, TRect Rect, int LineWidth, TRGBColor Color);
	void Unknown28(); void Unknown2C(); void Unknown30(); void Unknown34(); void Unknown38();
	void Unknown3C(); void Unknown40(); void Unknown44(); void Unknown48(); void Unknown4C();
	void Unknown54(int a, int b, int c, int d); void Unknown58(); void Unknown5C(); void Unknown60();
	void Unknown68(); void Unknown6C(); void Unknown70(); void Unknown74(); void Unknown78();
	void Unknown7C(); void FinishFrame();
	bool Init(HWND *Window, int ViewMode, TFullscreenMode *Resolution, bool Force);
	void Unknown8C(); void Unknown90(); void Unknown94(); bool Unknown9C(int Value);
	bool IsUnavailable(); void UnknownA4(); void UnknownA8(); void UnknownAC(); double GetFPS();
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

// Communication.cpp

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

// Between DialogWindow.cpp and graphics.cpp
void FUN_0048b4a0();                   // 0x48b4a0

// graphics.cpp
void FUN_00490c60(int Surface, int x, int y, int PictureNumber, int SourceX, int SourceY,
	int Width, int Height);            // 0x490c60, line 1164
int FUN_00491880(int FontNumber, const char *Text); // 0x491880, lines 1352-1353; text height
void FUN_004922c0();                   // 0x4922c0
void FUN_00492490();                   // 0x492490
void FUN_00493af0(int Surface, int x, int y, int FontNumber, TRGBColor Color, const char *Text,
	int Param7);                       // 0x493af0, lines 1282-1284
void FUN_00493f60(int Surface, int x, int y, TFont Font, const char *Text); // 0x493f60, line 1295
int FUN_00494570(int FontNumber, const char *Text); // 0x494570, lines 1373-1374; text width
void FUN_00494900();                   // 0x494900
void FUN_00494af0(int MemoryCacheBytes); // 0x494af0

// graphics.cpp, added for GUI.cpp's DrawSkinClipped (kept apart from the
// lines above, which the graphics.cpp reconstruction replaces)
void FUN_004910e0(int Surface, int x, int y, int PictureNumber, int SourceX, int SourceY, int Width,
	int Height, int ClipX, int ClipY, int ClipWidth, int ClipHeight); // 0x4910e0, lines 1191-1193; clipped

// Between MainFrm.cpp and MainWindow.cpp
void FUN_004ac090();                   // 0x4ac090

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

// MainWindow.cpp: creates the main game window with the frame's callbacks.
void FUN_004b05b0(int Width, int Height, void (*ExitProgram)(), void (*SetDisplayMode)(int Mode),
	void (*OpenURL)(const char *URL));  // 0x4b05b0

// graphics.cpp
void FUN_0048da50();                    // 0x48da50, after the surfaces are created

// Owner unknown (.data after Status.cpp's 0x5c2560): set by "engine N" on
// the command line, read by SurfaceInitInstance; -1 = none.
extern int DAT_005c2564;                // 0x5c2564


// Owner unknown: a table of message texts in .data (0x5c2480-0x5c2517).
extern const char *DAT_005c249c;        // 0x5c249c, "The Tibia client encountered a critical error ..."
