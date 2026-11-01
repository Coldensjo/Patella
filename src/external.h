// Functions and data from source files that are not reconstructed yet.
// Each entry gives its address in Tibia.exe and where it sits. Move a
// declaration into the real header when its file is reconstructed.

#pragma once

#include "common.h"
#include "Talk.h"

// First 16 bytes of the last received packet, at 0x5d1644 (owner unknown).
extern uint8 PacketHeader[16];

// Objects.cpp: a reference to an object (12 bytes, passed by value). Type
// is named in MapWindow.cpp's "SrcObj.Type!=CREATURE" (CREATURE is 99),
// Data in Dialogs.cpp's "In(DraggedObject.Data,1,100)" (the count); Data2 is
// ours. The default constructor clears the object:
// Map.cpp's TMapPoint, ten TObjects and plain ints, has a compiler-generated
// constructor (0x4b0b40) that zeroes exactly the ten objects. Written with
// default member initializers so that the type stays an aggregate.
// MapWindow.cpp compares objects with numbers ("SrcObj!=0", "Obj!=CREATURE")
// and prints one as the int value of TEND1(Obj): the original converts a
// TObject to its Type.
struct TObject {
	int Type = 0;
	int Data = 0;
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

// The engines' surface classes (SurfaceDX.cpp, surfaceDX9.cpp, SurfaceGL.cpp).
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
	int GetFullscreenModes(TFullscreenMode **Modes); int GetCacheSurfaces(); void GetSurfaceRect(int Surface, TRect *Rect);
	bool Unknown9C(int Value);
	bool IsUnavailable(); bool CanShadeRectangles(); void UnknownA8(); void SetFramerateLimit(int Limit); double GetFPS();
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

// Dialogs.cpp: the dialogs are in Dialogs.h, which its users include directly
// (it needs DialogElements.h and Map.h). Dialogs.obj's copy of the text table
// (0x5c123c-0x5c1484, then its regular expression pointer 0x5c1488 and its copy
// of DefaultFontColor 0x5c148c; names ours), the entries Dialogs.cpp reads:
extern const char *TextSwitchToDirectX5;              // 0x5c1310, "Switch to DirectX 5 Engine"
extern const char *TextSwitchToDirectX9;              // 0x5c1314, "Switch to DirectX 9 Engine"
extern const char *TextSwitchToOpenGL;                // 0x5c1318, "Switch to OpenGL Engine"
extern const char *TextAccountNumber;                 // 0x5c1324, "Account number:"
extern const char *TextPassword;                      // 0x5c1328, "Password:"
extern const char *TextNoAccountYet;                  // 0x5c132c, "If you don't have\nan account yet:"
extern const char *TextSelectCharacter;               // 0x5c1330, "Select character:"
extern const char *TextTryingToReconnect;             // 0x5c1334, "Trying to reconnect in %d %s."
extern const char *TextSetNumberOfItems;              // 0x5c1338, "Set the number of\nitems you want to move:"
extern const char *TextChooseOutfit;                  // 0x5c133c, "Choose an outfit and determine ..."
extern const char *TextSelectChannel;                 // 0x5c1340, "Select a channel:"
extern const char *TextOpenPrivateChannel;            // 0x5c1344, "Open a private message channel:"
extern const char *TextEnterCharacterName;            // 0x5c1348, "Please enter a character name:"
extern const char *TextStateRuleViolation;            // 0x5c134c, "Please state the rule violation in one\nclear sentence ..."
extern const char *TextReportBugsOnly;                // 0x5c1350, "Please use this dialog only to report bugs. ..."
extern const char *TextSelectType;                    // 0x5c1354, "Select a type:"
extern const char *TextEnterCharacterName2;           // 0x5c1358, "Please enter a character name:"
extern const char *TextEnterShortDescription;         // 0x5c135c, "Enter a short description:"
extern const char *TextDownloadingPictures;           // 0x5c1364, "Downloading new pictures"
extern const char *TextDownloadingObjectData;         // 0x5c1368, "Downloading new object data"
extern const char *TextDownloadingObjectGraphics;     // 0x5c136c, "Downloading new object graphics"
extern const char *TextClientInfo;                    // 0x5c1370, "Click here to get\ninfo on the client."
extern const char *TextShowPreviousHints;             // 0x5c1374, "Show previous hints"
extern const char *TextAskTutor;                      // 0x5c1378, "Ask a tutor for help"
extern const char *TextReportRuleViolations;          // 0x5c137c, "Report rule violations"
extern const char *TextReadManual;                    // 0x5c1380, "Read the manual on\nthe website"
extern const char *TextReadFAQ;                       // 0x5c1384, "Read the FAQ on\nthe website."
extern const char *TextAvailableHints;                // 0x5c1388, "Available hints:"
extern const char *TextChangeGeneralOptions;          // 0x5c138c, "Change general\ngame options"
extern const char *TextChangeGraphicsOptions;         // 0x5c1390, "Change graphics and\nperformance settings"
extern const char *TextCustomiseConsole;              // 0x5c1394, "Customise the console"
extern const char *TextChangeNetworkSettings;         // 0x5c1398, "Change network settings"
extern const char *TextEditHotkeys;                   // 0x5c139c, "Edit your hotkey texts"
extern const char *TextShowMotD;                      // 0x5c13a0, "Show the most recent\nMessage of the Day"
extern const char *TextAdvancedSettings;              // 0x5c13ac, "Advanced settings"
extern const char *TextAvailableResolutions;          // 0x5c13b0, "Available resolutions:"
extern const char *TextSelectEngine;                  // 0x5c13b4, "Select graphics engine:"
extern const char *TextAvailableHotkeys;              // 0x5c13b8, "Available hotkeys:"
extern const char *TextEditHotkeyText;                // 0x5c13bc, "Edit hotkey text:"
extern const char *TextClientVersion;                 // 0x5c13c0, "Tibia Client\nVersion 7.72 ..."
extern const char *TextOfficialWebsite;               // 0x5c13c4, "Official Website"
extern const char *TextCharactersToIgnore;            // 0x5c13c8, "Characters you wish to ignore:"
extern const char *TextGlobalIgnoreSettings;          // 0x5c13cc, "Global settings for ignoring messages:"
extern const char *TextCharactersToAllow;             // 0x5c13d0, "Characters you allow to message you:"
extern const char *TextGlobalAllowSettings;           // 0x5c13d4, "Global settings for allowing messages:"
extern const char *TextEnterOneNamePerLine;           // 0x5c13d8, "Enter one name per line."
extern const char *TextItIsEmpty;                     // 0x5c13dc, "It is empty."
extern const char *TextItIsCurrentlyEmpty;            // 0x5c13e0, "It is currently empty."
extern const char *TextYouReadTheFollowing;           // 0x5c13e4, "You read the following."
extern const char *TextYouCanEnterNewText;            // 0x5c13e8, "You can enter new text."
extern const char *TextYouReadWrittenBy;              // 0x5c13ec, "You read the following, written by"
extern const char *TextSelectType2;                   // 0x5c13fc, "Select a type:"
extern const char *TextEnterShortDescription2;        // 0x5c1400, "Enter a short description:"
extern const char *TextClientCrashed;                 // 0x5c1410, "At your last visit in Tibia your client crashed ..."
extern const char *TextTellUsWhatYouDid;              // 0x5c1414, "Please tell us what you were doing just before the crash:\n"
extern const char *TextRequestAlreadySubmitted;       // 0x5c1420, "You have already submitted a request. Please close ..."
extern const char *TextNoPrivateChannelWithYourself;  // 0x5c1428, "You cannot set up a private message channel with yourself."
extern const char *TextNoPlayerName;                  // 0x5c143c, "No playername given."
extern const char *TextNoReason;                      // 0x5c1440, "No reason has been selected."
extern const char *TextNoAction;                      // 0x5c1444, "No action has been selected."
extern const char *TextNoComment;                     // 0x5c1448, "You must enter a comment."
extern const char *TextNoStatement;                   // 0x5c144c, "No statement has been selected."
extern const char *TextCannotReportStatement;         // 0x5c1450, "You cannot report this statement."
extern const char *TextTibiaURL;                      // 0x5c1458, "http://www.tibia.com/"
extern const char *TextManualURL;                     // 0x5c145c, "http://www.tibia.com/guide/?subtopic=manual"
extern const char *TextFAQURL;                        // 0x5c1460, "http://www.tibia.com/guide/?subtopic=faq"
extern const char *TextOptionsURL;                    // 0x5c1464, "...manual&section=options"
extern const char *TextGraphicsOptionsURL;            // 0x5c1468, "...options#graphicsoptions"
extern const char *TextAdvancedGraphicsURL;           // 0x5c146c, "...options#graphicsoptions (the same text)"
extern const char *TextConsoleOptionsURL;             // 0x5c1470, "...options#consoleoptions"
extern const char *TextNetworkOptionsURL;             // 0x5c1474, "...options#networkoptions"


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
