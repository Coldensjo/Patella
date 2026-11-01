// Preferences.cpp: reconstructed from Tibia.exe 7.72 (0x4db2a0-0x4ddbfc).
//
// The functions without line evidence (0x4db2a0-0x4dc170, the gap after
// Player.cpp) come first, in binary order; they use only Preferences data
// (0x719e70-0x71c44f, after Player.cpp's last global at 0x719e6c, and
// 0x5c2558-0x5c255f in .data). The asserted functions follow in line order,
// which is also their binary order. None of them has a try block.
//
// The option loader and saver (0x4ddc00, 0x4de890) and a few helpers after
// them use the same data and are probably the end of this file, but they
// lie in the gap before Script.cpp, which is not settled yet.

#include "Preferences.h"
#include "Talk.h"
#include "external.h"

// Data, in .data/.bss order.
static int WindowedWidth = 600;               // 0x5c2558, "WindowedMode = (x,y,Width,Height,Maximized)"
static int WindowedHeight = 400;              // 0x5c255c
static bool SkillBars[SKILLS];                // 0x719e70, "SkillBars = {...}"
static bool MagLevelBar;                      // 0x719e77
static bool LevelBar;                         // 0x719e78
static bool ShowNewHints;                     // 0x719e79
static int NumberOfKnownHints;                // 0x719e7c, "KnownHints = {...}"
static int NumberOfBuddyEntries;              // 0x719e80, "Buddy = (...)", deleted entries included
static bool SecureMode;                       // 0x719e84
static int ChaseMode;                         // 0x719e88
static int AttackMode;                        // 0x719e8c
static bool AutoChaseOff;                     // 0x719e90
static bool ClassicControl;                   // 0x719e91
static int NumberOfOpenChannels;              // 0x719e94, "OpenChannel = ..."
static bool PrivateMessages;                  // 0x719e98
static bool TimeStamps;                       // 0x719e99
static bool StatusMessages;                   // 0x719e9a
static bool EventMessages;                    // 0x719e9b
static bool InfoMessages;                     // 0x719e9c
static char *MotD;                            // 0x719ea0, text of the message of the day
static int LastMotD;                          // 0x719ea4, its number
static int NumberOfOpenDialogs;               // 0x719ea8, "OpenDialogs = ..."
static bool BuddyHideOffline;                 // 0x719eac
static int BuddySortmethods[3];               // 0x719eb0
static int NumberOfContentWindows;            // 0x719ebc, "ContentWindow = (...)"
static int AutomapZoom;                       // 0x719ec0
static int ConsoleHeight;                     // 0x719ec4
static bool WindowedMaximized;                // 0x719ec8
static bool HotkeyAutoSend[HOTKEYS];          // 0x719ecc, saved as a '\n' after the text
static char Hotkeys[HOTKEYS][HOTKEY_LENGTH];  // 0x719ef0, "Hotkey = (Number,Text)"
static int Engine;                            // 0x71c2f0, -1 = not chosen
static int ConfineFramerate;                  // 0x71c2f4
static int NoStretching;                      // 0x71c2f8, 0 or 1, 2 = not set yet
static int CreatureInfo;                      // 0x71c2fc
static bool TextualEffects;                   // 0x71c300
static bool LightEffects;                     // 0x71c301
static bool WindowsMousepointer;              // 0x71c302
static TFullscreenMode FullscreenMode;        // 0x71c304, Width 0 = not set
static int ViewMode;                          // 0x71c314, "ViewMode" in a MainFrm.cpp catch block
static bool TestServerLogin;                  // 0x71c318, "TestServer = yes"
// 0x71c319 is a flag of the loader/saver ("save Tibia.cfg"), not used here.
static int WindowedX;                         // 0x71c31c
static int WindowedY;                         // 0x71c320
static vector<TContentWindow> ContentWindows(0, 9, 10); // 0x71c324, initialiser at 0x55a050
static vector<int> OpenDialogs(0, 9, 10, -1);  // 0x71c348, initialiser at 0x55a070
static vector<int> OpenChannels(0, 9, 10);     // 0x71c368, initialiser at 0x55a090
static vector<TBuddy> Buddies(0, 9, 10);       // 0x71c388, initialiser at 0x55a0b0
static vector<int> KnownHints(0, 9, 10);       // 0x71c430, initialiser at 0x55a0d0

// 0x4db2a0
bool GetTestServerLogin()
{
	return TestServerLogin;
}

// 0x4db2b0
void SetTestServerLogin(bool Value)
{
	TestServerLogin = Value;
}

// 0x4db2c0
void SetViewMode(int ViewMode)
{
	::ViewMode = ViewMode;
}

// 0x4db2d0
void SetFullscreenMode(const TFullscreenMode *Mode)
{
	FullscreenMode = *Mode;
}

// 0x4db300
void SetWindowsMousepointer(bool Value)
{
	WindowsMousepointer = Value;
}

// 0x4db310
void SetLightEffects(bool Value)
{
	LightEffects = Value;
}

// 0x4db320
void SetTextualEffects(bool Value)
{
	TextualEffects = Value;
}

// 0x4db330
void SetCreatureInfo(int Value)
{
	CreatureInfo = Value;
}

// 0x4db340
void SetNoStretching(bool Value)
{
	NoStretching = Value ? 1 : 0;
}

// 0x4db360. Only takes effect while the option has never been set.
void SetDefaultNoStretching(bool Value)
{
	if (NoStretching == 2)
		NoStretching = Value ? 1 : 0;
}

// 0x4db380
void SetConfineFramerate(int Value)
{
	ConfineFramerate = Value;
}

// 0x4db390
void SetEngine(int Value)
{
	Engine = Value;
}

// 0x4db3a0
int GetViewMode()
{
	return ViewMode;
}

// 0x4db3b0
TFullscreenMode *GetFullscreenMode()
{
	return &FullscreenMode;
}

// 0x4db3c0
bool GetWindowsMousepointer()
{
	return WindowsMousepointer;
}

// 0x4db3d0
bool GetLightEffects()
{
	return LightEffects;
}

// 0x4db3e0
bool GetTextualEffects()
{
	return TextualEffects;
}

// 0x4db3f0
int GetCreatureInfo()
{
	return CreatureInfo;
}

// 0x4db400. "Not set yet" (2) reads as false.
bool GetNoStretching()
{
	return NoStretching == 1;
}

// 0x4db410
int GetConfineFramerate()
{
	return ConfineFramerate;
}

// 0x4db420
int GetEngine()
{
	return Engine;
}

// 0x4db430
void SetWindowedMode(int x, int y, int Width, int Height, bool Maximized)
{
	WindowedX = x;
	WindowedY = y;
	WindowedWidth = Width;
	WindowedHeight = Height;
	WindowedMaximized = Maximized;
}

// 0x4db470
void GetWindowedMode(int *x, int *y, int *Width, int *Height, bool *Maximized)
{
	*x = WindowedX;
	*y = WindowedY;
	*Width = WindowedWidth;
	*Height = WindowedHeight;
	*Maximized = WindowedMaximized;
}

// 0x4db4b0
void SetConsoleHeight(int Value)
{
	ConsoleHeight = Value;
}

// 0x4db4c0
int GetConsoleHeight()
{
	return ConsoleHeight;
}

// 0x4db4d0
void SetAutomapZoom(int Value)
{
	AutomapZoom = Value;
}

// 0x4db4e0
int GetAutomapZoom()
{
	return AutomapZoom;
}

// 0x4db4f0
void SetBuddySortmethods(const int *Methods)
{
	BuddySortmethods[0] = Methods[0];
	BuddySortmethods[1] = Methods[1];
	BuddySortmethods[2] = Methods[2];
}

// 0x4db510
void GetBuddySortmethods(int *Methods)
{
	Methods[0] = BuddySortmethods[0];
	Methods[1] = BuddySortmethods[1];
	Methods[2] = BuddySortmethods[2];
}

// 0x4db530
void SetBuddyHideOffline(bool Value)
{
	BuddyHideOffline = Value;
}

// 0x4db540
bool GetBuddyHideOffline()
{
	return BuddyHideOffline;
}

// 0x4db550
void ClearOpenDialogs()
{
	NumberOfOpenDialogs = 0;
}

// 0x4db560
const char *GetMotD()
{
	if (MotD == NULL)
		return "No current information.";
	return MotD;
}

// 0x4db570
void SetInfoMessages(bool Value)
{
	InfoMessages = Value;
}

// 0x4db580
void SetEventMessages(bool Value)
{
	EventMessages = Value;
}

// 0x4db590
void SetStatusMessages(bool Value)
{
	StatusMessages = Value;
}

// 0x4db5a0
void SetTimeStamps(bool Value)
{
	TimeStamps = Value;
}

// 0x4db5b0
void SetPrivateMessages(bool Value)
{
	PrivateMessages = Value;
}

// 0x4db5c0
bool GetInfoMessages()
{
	return InfoMessages;
}

// 0x4db5d0
bool GetEventMessages()
{
	return EventMessages;
}

// 0x4db5e0
bool GetStatusMessages()
{
	return StatusMessages;
}

// 0x4db5f0
bool GetTimeStamps()
{
	return TimeStamps;
}

// 0x4db600
bool GetPrivateMessages()
{
	return PrivateMessages;
}

// 0x4db610
void ClearOpenChannels()
{
	NumberOfOpenChannels = 0;
}

// 0x4db620
void SetClassicControl(bool Value)
{
	ClassicControl = Value;
}

// 0x4db630
void SetAutoChaseOff(bool Value)
{
	AutoChaseOff = Value;
}

// 0x4db640
bool GetClassicControl()
{
	return ClassicControl;
}

// 0x4db650
bool GetAutoChaseOff()
{
	return AutoChaseOff;
}

// 0x4db660
void SetAttackMode(int Value)
{
	AttackMode = Value;
}

// 0x4db670
void SetChaseMode(int Value)
{
	ChaseMode = Value;
}

// 0x4db680. A byte, although it is saved as a number like AttackMode.
void SetSecureMode(bool Value)
{
	SecureMode = Value;
}

// 0x4db690
int GetAttackMode()
{
	return AttackMode;
}

// 0x4db6a0
int GetChaseMode()
{
	return ChaseMode;
}

// 0x4db6b0
bool GetSecureMode()
{
	return SecureMode;
}

// 0x4db6c0
void SetShowNewHints(bool Value)
{
	ShowNewHints = Value;
}

// 0x4db6d0
bool GetShowNewHints()
{
	return ShowNewHints;
}

// 0x4db6e0
bool GetLevelBar()
{
	return LevelBar;
}

// 0x4db6f0
bool GetMagLevelBar()
{
	return MagLevelBar;
}

// 0x4db700
void SetLevelBar(bool Value)
{
	LevelBar = Value;
}

// 0x4db710
void SetMagLevelBar(bool Value)
{
	MagLevelBar = Value;
}

// 0x4db720. Not reset: the windowed position and size, and the hotkey texts
// past their first byte. MotD is cleared without being freed.
void SetDefaultPreferences()
{
	memset(HotkeyAutoSend, 0, sizeof(HotkeyAutoSend));
	ViewMode = 1;
	FullscreenMode.Width = 0;
	WindowsMousepointer = true;
	LightEffects = true;
	TextualEffects = true;
	CreatureInfo = 2;
	NoStretching = 2;
	ConfineFramerate = 0;
	Engine = -1;
	for (int i = 0; i < HOTKEYS; i++)
		Hotkeys[i][0] = 0;
	WindowedMaximized = false;
	ConsoleHeight = 0;
	AutomapZoom = 2;
	NumberOfContentWindows = 0;
	BuddySortmethods[0] = 2;
	BuddySortmethods[1] = 1;
	BuddySortmethods[2] = 0;
	BuddyHideOffline = false;
	NumberOfOpenDialogs = 0;
	LastMotD = 0;
	MotD = NULL;
	InfoMessages = false;
	EventMessages = true;
	StatusMessages = false;
	TimeStamps = false;
	PrivateMessages = true;
	NumberOfOpenChannels = 0;
	ClassicControl = false;
	AutoChaseOff = true;
	AttackMode = 2;
	ChaseMode = 0;
	SecureMode = true;
	NumberOfBuddyEntries = 0;
	NumberOfKnownHints = 0;
	ShowNewHints = true;
	LevelBar = true;
	MagLevelBar = true;
	for (int i = 0; i < SKILLS; i++)
		SkillBars[i] = true;
	TestServerLogin = false;
	ClearIgnoreList();
}

// 0x4db880: vector<TContentWindow>::operator() (Container.h).
// 0x4dba60: vector<TBuddy>::operator() (Container.h).

// 0x4dbcd0. Updates the entry for ID or appends one.
void SetContentWindowSize(int ID, int Size)
{
	for (int i = 0; i < NumberOfContentWindows; i++) {
		if (ContentWindows(i).ID == ID) {
			ContentWindows(i).Size = Size;
			return;
		}
	}
	ContentWindows(NumberOfContentWindows).ID = ID;
	ContentWindows(NumberOfContentWindows).Size = Size;
	NumberOfContentWindows++;
}

// 0x4dbd50. 0 for an unknown ID.
int GetContentWindowSize(int ID)
{
	for (int i = 0; i < NumberOfContentWindows; i++) {
		if (ContentWindows(i).ID == ID)
			return ContentWindows(i).Size;
	}
	return 0;
}

// 0x4dbd90. Returns a new array in *List (left unchanged when there are
// none) and the number of entries.
int GetOpenDialogs(int **List)
{
	if (NumberOfOpenDialogs == 0)
		return 0;
	*List = new int[NumberOfOpenDialogs];
	for (int i = 0; i < NumberOfOpenDialogs; i++)
		(*List)[i] = OpenDialogs(i);
	return NumberOfOpenDialogs;
}

// 0x4dbde0. Like GetOpenDialogs.
int GetOpenChannels(int **List)
{
	if (NumberOfOpenChannels == 0)
		return 0;
	*List = new int[NumberOfOpenChannels];
	for (int i = 0; i < NumberOfOpenChannels; i++)
		(*List)[i] = OpenChannels(i);
	return NumberOfOpenChannels;
}

// 0x4dbe30. Does not check for duplicates.
void AddKnownHint(int Hint)
{
	KnownHints(NumberOfKnownHints) = Hint;
	NumberOfKnownHints++;
}

// 0x4dbe50
bool IsKnownHint(int Hint)
{
	for (int i = 0; i < NumberOfKnownHints; i++) {
		if (KnownHints(i) == Hint)
			return true;
	}
	return false;
}

// 0x4dbe90: vector<TContentWindow>::vector(min, max, block) (Container.h).
// 0x4dc170: vector<TBuddy>::vector(min, max, block) (Container.h).

// 0x4dc450, lines 223-225
void SetHotkey(int Number, const char *Text, bool AutoSend)
{
	TASSERT1(In(Number,0,HOTKEYS-1), Number);
	TASSERT(Text!=NULL);
	TASSERT1(strlen(Text)<HOTKEY_LENGTH, strlen(Text));
	strcpy(Hotkeys[Number], Text);
	HotkeyAutoSend[Number] = AutoSend;
}

// 0x4dc7e0, line 235
const char *GetHotkey(int Number, bool *AutoSend)
{
	TASSERT1(In(Number,0,HOTKEYS-1), Number);
	*AutoSend = HotkeyAutoSend[Number];
	return Hotkeys[Number];
}

// 0x4dc950, line 393
void AddOpenDialog(int Identifier)
{
	TASSERT1(Identifier>=0, Identifier);
	OpenDialogs(NumberOfOpenDialogs) = Identifier;
	NumberOfOpenDialogs++;
}

// 0x4dcac0, line 425. Text is "<number>\n<message>"; without a line break
// nothing changes. Returns true when the number differs from the last one.
bool SetMotD(const char *Text)
{
	TASSERT(Text != NULL);
	const char *Break = strchr(Text, '\n');
	if (Break == NULL)
		return false;
	int Number = atol(Text);
	bool New = Number != LastMotD;
	LastMotD = Number;
	if (MotD != NULL)
		delete[] MotD;
	MotD = new char[strlen(Break + 1) + 1];
	strcpy(MotD, Break + 1);
	return New;
}

// 0x4dccb0, line 551
void AddOpenChannel(int Identifier)
{
	TASSERT1(Identifier>=0, Identifier);
	OpenChannels(NumberOfOpenChannels) = Identifier;
	NumberOfOpenChannels++;
}

// 0x4dce20, line 680. Index of the buddy entry, -1 if there is none.
int FindBuddy(int CharacterID)
{
	TASSERT1(CharacterID!=0, CharacterID);
	for (int i = 0; i < NumberOfBuddyEntries; i++) {
		if (Buddies(i).CharacterID == CharacterID)
			return i;
	}
	return -1;
}

// 0x4dcfb0, lines 691-693. Deleted entries are not reused.
void SetBuddy(int CharacterID, int Icon, const char *Description, bool Notify)
{
	TASSERT1(CharacterID!=0, CharacterID);
	TASSERT(Description!=NULL);
	TASSERT1(strlen(Description)<BUDDYDESCRIPTION_LENGTH, strlen(Description));
	int Number = FindBuddy(CharacterID);
	if (Number == -1)
		Number = NumberOfBuddyEntries++;
	TBuddy *Buddy = &Buddies(Number);
	Buddy->CharacterID = CharacterID;
	Buddy->Icon = Icon;
	strcpy(Buddy->Description, Description);
	Buddy->Notify = Notify;
}

// 0x4dd360, line 711. Only marks the entry as free.
void DeleteBuddy(int CharacterID)
{
	TASSERT1(CharacterID!=0, CharacterID);
	int Number = FindBuddy(CharacterID);
	if (Number != -1)
		Buddies(Number).CharacterID = 0;
}

// 0x4dd4c0, line 722
int GetBuddyIcon(int CharacterID)
{
	TASSERT1(CharacterID!=0, CharacterID);
	int Number = FindBuddy(CharacterID);
	if (Number == -1)
		return 0;
	return Buddies(Number).Icon;
}

// 0x4dd640, line 733. NULL for an unknown character.
const char *GetBuddyDescription(int CharacterID)
{
	TASSERT1(CharacterID!=0, CharacterID);
	int Number = FindBuddy(CharacterID);
	if (Number == -1)
		return NULL;
	return Buddies(Number).Description;
}

// 0x4dd7c0, line 744
bool GetBuddyNotify(int CharacterID)
{
	TASSERT1(CharacterID!=0, CharacterID);
	int Number = FindBuddy(CharacterID);
	if (Number == -1)
		return false;
	return Buddies(Number).Notify;
}

// 0x4dd940, line 825
bool GetSkillBar(int SkillNumber)
{
	TASSERT1(In(SkillNumber,0,SKILLS-1), SkillNumber);
	return SkillBars[SkillNumber];
}

// 0x4ddaa0, line 850
void SetSkillBar(int SkillNumber, bool Value)
{
	TASSERT1(In(SkillNumber,0,SKILLS-1), SkillNumber);
	SkillBars[SkillNumber] = Value;
}
