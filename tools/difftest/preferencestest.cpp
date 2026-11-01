// Differential tests of Preferences.cpp against the original code. Uses the
// image patches of statustest.cpp (malloc/free redirected), Talk.cpp
// for the real ignore-list reset, and the image's atol redirected to ours.

#include "difftest.h"
#include "../../src/Preferences.h"
#include "../../src/Talk.h"
// The original atol needs the locale data of its uninitialised CRT.
static long __cdecl ImageAtol(const char *Text) { return atol(Text); }

typedef void (__fastcall *VectorCtorFn)(void *This, void *, int Min, int Max, int Block);
typedef void (__fastcall *VectorInitCtorFn)(void *This, void *, int Min, int Max, int Block, int Init);

typedef void (__cdecl *SetBoolFn)(bool);
typedef bool (__cdecl *GetBoolFn)();
typedef void (__cdecl *SetIntFn)(int);
typedef int (__cdecl *GetIntFn)();

struct TBoolOption { uint32 Set, Get; void (*OurSet)(bool); bool (*OurGet)(); const char *Name; };
struct TIntOption { uint32 Set, Get; void (*OurSet)(int); int (*OurGet)(); const char *Name; };

static const TBoolOption BoolOptions[] = {
	{0x4db2b0, 0x4db2a0, SetTestServerLogin, GetTestServerLogin, "TestServer"},
	{0x4db300, 0x4db3c0, SetWindowsMousepointer, GetWindowsMousepointer, "WindowsMousepointer"},
	{0x4db310, 0x4db3d0, SetLightEffects, GetLightEffects, "LightEffects"},
	{0x4db320, 0x4db3e0, SetTextualEffects, GetTextualEffects, "TextualEffects"},
	{0x4db340, 0x4db400, SetNoStretching, GetNoStretching, "NoStretching"},
	{0x4db360, 0x4db400, SetDefaultNoStretching, GetNoStretching, "NoStretching (default)"},
	{0x4db530, 0x4db540, SetBuddyHideOffline, GetBuddyHideOffline, "BuddyHideOffline"},
	{0x4db570, 0x4db5c0, SetInfoMessages, GetInfoMessages, "InfoMessages"},
	{0x4db580, 0x4db5d0, SetEventMessages, GetEventMessages, "EventMessages"},
	{0x4db590, 0x4db5e0, SetStatusMessages, GetStatusMessages, "StatusMessages"},
	{0x4db5a0, 0x4db5f0, SetTimeStamps, GetTimeStamps, "TimeStamps"},
	{0x4db5b0, 0x4db600, SetPrivateMessages, GetPrivateMessages, "PrivateMessages"},
	{0x4db620, 0x4db640, SetClassicControl, GetClassicControl, "ClassicControl"},
	{0x4db630, 0x4db650, SetAutoChaseOff, GetAutoChaseOff, "AutoChaseOff"},
	{0x4db680, 0x4db6b0, SetSecureMode, GetSecureMode, "SecureMode"},
	{0x4db6c0, 0x4db6d0, SetShowNewHints, GetShowNewHints, "ShowNewHints"},
	{0x4db700, 0x4db6e0, SetLevelBar, GetLevelBar, "LevelBar"},
	{0x4db710, 0x4db6f0, SetMagLevelBar, GetMagLevelBar, "MagLevelBar"},
};

static const TIntOption IntOptions[] = {
	{0x4db2c0, 0x4db3a0, SetViewMode, GetViewMode, "ViewMode"},
	{0x4db330, 0x4db3f0, SetCreatureInfo, GetCreatureInfo, "CreatureInfo"},
	{0x4db380, 0x4db410, SetConfineFramerate, GetConfineFramerate, "ConfineFramerate"},
	{0x4db390, 0x4db420, SetEngine, GetEngine, "Engine"},
	{0x4db4b0, 0x4db4c0, SetConsoleHeight, GetConsoleHeight, "ConsoleHeight"},
	{0x4db4d0, 0x4db4e0, SetAutomapZoom, GetAutomapZoom, "AutomapZoom"},
	{0x4db660, 0x4db690, SetAttackMode, GetAttackMode, "AttackMode"},
	{0x4db670, 0x4db6a0, SetChaseMode, GetChaseMode, "ChaseMode"},
};

static void CheckAllOptions(const char *When)
{
	for (auto &O : BoolOptions)
		CHECK(O.OurGet() == At<GetBoolFn>(O.Get)(), "%s after %s", O.Name, When);
	for (auto &O : IntOptions)
		CHECK(O.OurGet() == At<GetIntFn>(O.Get)(), "%s after %s", O.Name, When);
	CHECK(memcmp(GetFullscreenMode(), At<TFullscreenMode *(__cdecl *)()>(0x4db3b0)(), sizeof(TFullscreenMode)) == 0,
		"FullscreenMode after %s", When);
	int A[5], B[5];
	bool MA, MB;
	GetWindowedMode(&A[0], &A[1], &A[2], &A[3], &MA);
	At<void (__cdecl *)(int *, int *, int *, int *, bool *)>(0x4db470)(&B[0], &B[1], &B[2], &B[3], &MB);
	CHECK(memcmp(A, B, 4 * sizeof(int)) == 0 && MA == MB, "WindowedMode after %s", When);
	GetBuddySortmethods(A);
	At<void (__cdecl *)(int *)>(0x4db510)(B);
	CHECK(memcmp(A, B, 3 * sizeof(int)) == 0, "BuddySortmethods after %s", When);
	CHECK(strcmp(GetMotD(), At<const char *(__cdecl *)()>(0x4db560)()) == 0, "MotD after %s", When);
	for (int i = 0; i < SKILLS; i++)
		CHECK(GetSkillBar(i) == At<bool (__cdecl *)(int)>(0x4dd940)(i), "SkillBar %d after %s", i, When);
	for (int i = 0; i < HOTKEYS; i++) {
		bool AutoA = false, AutoB = false;
		const char *TextA = GetHotkey(i, &AutoA);
		const char *TextB = At<const char *(__cdecl *)(int, bool *)>(0x4dc7e0)(i, &AutoB);
		CHECK(strcmp(TextA, TextB) == 0 && AutoA == AutoB, "hotkey %d after %s", i, When);
	}
}

static void CheckLists(const char *When)
{
	typedef int (__cdecl *ListFn)(int **);
	struct { uint32 Original; int (*Ours)(int **); const char *Name; } Lists[] = {
		{0x4dbd90, GetOpenDialogs, "OpenDialogs"}, {0x4dbde0, GetOpenChannels, "OpenChannels"},
	};
	for (auto &L : Lists) {
		int *A = NULL, *B = NULL;
		int CountA = L.Ours(&A);
		int CountB = At<ListFn>(L.Original)(&B);
		CHECK(CountA == CountB && (CountA == 0 ? A == NULL && B == NULL : memcmp(A, B, CountA * sizeof(int)) == 0),
			"%s after %s", L.Name, When);
		delete[] A;
		free(B);
	}
	for (int Hint = 0; Hint < 40; Hint++)
		CHECK(IsKnownHint(Hint) == At<bool (__cdecl *)(int)>(0x4dbe50)(Hint), "IsKnownHint(%d) after %s", Hint, When);
	for (int ID = 0; ID < 20; ID++)
		CHECK(GetContentWindowSize(ID) == At<int (__cdecl *)(int)>(0x4dbd50)(ID), "GetContentWindowSize(%d)", ID);
	for (int ID = 1; ID < 30; ID++) {
		CHECK(FindBuddy(ID) == At<int (__cdecl *)(int)>(0x4dce20)(ID), "FindBuddy(%d) after %s", ID, When);
		CHECK(GetBuddyIcon(ID) == At<int (__cdecl *)(int)>(0x4dd4c0)(ID), "GetBuddyIcon(%d)", ID);
		CHECK(GetBuddyNotify(ID) == At<bool (__cdecl *)(int)>(0x4dd7c0)(ID), "GetBuddyNotify(%d)", ID);
		const char *A = GetBuddyDescription(ID);
		const char *B = At<const char *(__cdecl *)(int)>(0x4dd640)(ID);
		CHECK((A == NULL) == (B == NULL) && (A == NULL || strcmp(A, B) == 0), "GetBuddyDescription(%d)", ID);
	}
}

void TestPreferences()
{
	PatchImage();
	At<VectorInitCtorFn>(0x4fd6e0)((void *)(uintptr_t)0x71efb0, 0, 0, 9, 10, 0);
	PatchJump(0x51262a, (void *)ImageAtol);
	// The original's static initialisers (0x55a050-0x55a0d0) did not run.
	At<VectorCtorFn>(0x4dbe90)((void *)(uintptr_t)0x71c324, 0, 0, 9, 10);
	At<VectorInitCtorFn>(0x453300)((void *)(uintptr_t)0x71c348, 0, 0, 9, 10, -1);
	At<VectorCtorFn>(0x4a2b90)((void *)(uintptr_t)0x71c368, 0, 0, 9, 10);
	At<VectorCtorFn>(0x4dc170)((void *)(uintptr_t)0x71c388, 0, 0, 9, 10);
	At<VectorCtorFn>(0x4a2b90)((void *)(uintptr_t)0x71c430, 0, 0, 9, 10);

	CheckAllOptions("start");
	IgnoreName("reset probe", true);
	At<void (__cdecl *)(const char *, bool)>(0x501940)("reset probe", true);
	SetDefaultPreferences();
	At<void (__cdecl *)()>(0x4db720)();
	CHECK(GetIgnoreListText() == NULL && At<char *(__cdecl *)()>(0x501010)() == NULL, "Talk list reset");
	CheckAllOptions("SetDefaultPreferences");

	for (int t = 0; t < 20000; t++) {
		char Text[300];
		switch (Random() % 16) {
		case 0: case 1: {
			const TBoolOption &O = BoolOptions[Random() % (sizeof(BoolOptions) / sizeof(BoolOptions[0]))];
			bool Value = (Random() & 1) != 0;
			O.OurSet(Value);
			At<SetBoolFn>(O.Set)(Value);
			break;
		}
		case 2: {
			const TIntOption &O = IntOptions[Random() % (sizeof(IntOptions) / sizeof(IntOptions[0]))];
			int Value = (int)(Random() ^ (Random() << 16));
			O.OurSet(Value);
			At<SetIntFn>(O.Set)(Value);
			break;
		}
		case 3: {
			TFullscreenMode Mode = {(int)Random(), (int)Random(), (int)Random(), (int)Random()};
			SetFullscreenMode(&Mode);
			At<void (__cdecl *)(const TFullscreenMode *)>(0x4db2d0)(&Mode);
			int x = (int)Random(), y = (int)Random(), w = (int)Random(), h = (int)Random();
			bool Max = (Random() & 1) != 0;
			SetWindowedMode(x, y, w, h, Max);
			At<void (__cdecl *)(int, int, int, int, bool)>(0x4db430)(x, y, w, h, Max);
			int Methods[3] = {(int)Random() % 3, (int)Random() % 3, (int)Random() % 3};
			SetBuddySortmethods(Methods);
			At<void (__cdecl *)(const int *)>(0x4db4f0)(Methods);
			break;
		}
		case 4: {
			int Number = (int)(Random() % HOTKEYS);
			RandomText(Text, (int)(Random() % HOTKEY_LENGTH));
			bool Auto = (Random() & 1) != 0;
			SetHotkey(Number, Text, Auto);
			At<void (__cdecl *)(int, const char *, bool)>(0x4dc450)(Number, Text, Auto);
			break;
		}
		case 5: {
			int Identifier = (int)(Random() % 50);
			if (Random() & 1) {
				AddOpenDialog(Identifier);
				At<SetIntFn>(0x4dc950)(Identifier);
			} else {
				AddOpenChannel(Identifier);
				At<SetIntFn>(0x4dccb0)(Identifier);
			}
			break;
		}
		case 6:
			if (Random() % 8 == 0) {
				ClearOpenDialogs();
				At<void (__cdecl *)()>(0x4db550)();
			}
			if (Random() % 8 == 0) {
				ClearOpenChannels();
				At<void (__cdecl *)()>(0x4db610)();
			}
			break;
		case 7: {
			int Hint = (int)(Random() % 40);
			AddKnownHint(Hint);
			At<SetIntFn>(0x4dbe30)(Hint);
			break;
		}
		case 8: {
			int ID = (int)(Random() % 20), Size = (int)Random();
			SetContentWindowSize(ID, Size);
			At<void (__cdecl *)(int, int)>(0x4dbcd0)(ID, Size);
			break;
		}
		case 9: case 10: {
			int ID = 1 + (int)(Random() % 29), Icon = (int)(Random() % 10);
			RandomText(Text, (int)(Random() % BUDDYDESCRIPTION_LENGTH));
			bool Notify = (Random() & 1) != 0;
			SetBuddy(ID, Icon, Text, Notify);
			At<void (__cdecl *)(int, int, const char *, bool)>(0x4dcfb0)(ID, Icon, Text, Notify);
			break;
		}
		case 11: {
			int ID = 1 + (int)(Random() % 29);
			DeleteBuddy(ID);
			At<SetIntFn>(0x4dd360)(ID);
			break;
		}
		case 12: {
			char Body[100];
			RandomText(Body, (int)(Random() % 60));
			switch (Random() % 4) {
			case 0: snprintf(Text, sizeof(Text), "%d\n%s", (int)(Random() % 5), Body); break;
			case 1: snprintf(Text, sizeof(Text), "%s", Body); break;
			case 2: snprintf(Text, sizeof(Text), "  -%u\n%s", Random(), Body); break;
			default: snprintf(Text, sizeof(Text), "x\n%s", Body); break;
			}
			bool A = SetMotD(Text);
			bool B = At<bool (__cdecl *)(const char *)>(0x4dcac0)(Text);
			CHECK(A == B, "SetMotD(\"%s\")", Text);
			break;
		}
		case 13: {
			int Skill = (int)(Random() % SKILLS);
			bool Value = (Random() & 1) != 0;
			SetSkillBar(Skill, Value);
			At<void (__cdecl *)(int, bool)>(0x4ddaa0)(Skill, Value);
			break;
		}
		default:
			break;
		}
		if (t % 50 == 0) {
			CheckAllOptions("random changes");
			CheckLists("random changes");
		}
	}
	CheckAllOptions("the end");
	CheckLists("the end");
}

// --- Tibia.cfg loader and saver ------------------------------------------------
//
// Runs after TestScript: uses its stdio redirection (PatchScript). Only valid
// files are generated; a parse error would throw inside the original.

#include <string>
#include <stdarg.h>

static void __cdecl ImageFree2(void *Block) { free(Block); }
static char *__cdecl ImageStrtok(char *Text, const char *Delimiters) { return strtok(Text, Delimiters); }
static int __cdecl ImageSnprintf(char *Buffer, size_t Count, const char *Format, ...)
{
	va_list Args;
	va_start(Args, Format);
	int Result = _vsnprintf(Buffer, Count, Format, Args);
	va_end(Args);
	return Result;
}

static std::string ReadWholeFile(const char *Name, bool *Exists)
{
	std::string Text;
	FILE *File = fopen(Name, "rb");
	*Exists = File != NULL;
	if (File == NULL)
		return Text;
	int c;
	while ((c = getc(File)) != EOF)
		Text += (char)c;
	fclose(File);
	return Text;
}

static std::string CfgNumber(bool Negative = false)
{
	char Help[16];
	int Value = (int)(Random() % 4 == 0 ? Random() % 100000 : Random() % 50);
	if (Negative && Random() % 4 == 0)
		Value = -Value;
	sprintf(Help, "%d", Value);
	return Help;
}

static std::string CfgYes()
{
	switch (Random() % 4) {
	case 0: return "no";
	case 1: return "YES"; // identifiers are lower-cased
	case 2: return "maybe";
	default: return "yes";
	}
}

static std::string CfgString(int MaxLength)
{
	static const char Chars[] = "abc XYZ 019 .,:!?()=-#'";
	std::string s = "\"";
	int n = (int)(Random() % (MaxLength + 1));
	for (int i = 0; i < n; i++) {
		switch (Random() % 14) {
		case 0: s += "\\\""; break;
		case 1: s += "\\\\"; break;
		case 2: s += "\\n"; break;
		default: s += Chars[Random() % (sizeof(Chars) - 1)]; break;
		}
	}
	return s + "\"";
}

static std::string CfgName()
{
	std::string s = "\"";
	int n = 1 + (int)(Random() % 12);
	for (int i = 0; i < n; i++)
		s += (char)((i == 0 ? 'A' : 'a') + Random() % 26);
	return s + "\"";
}

static std::string CfgList(std::string (*Item)())
{
	std::string s = "{";
	int n = (int)(Random() % 6);
	for (int i = 0; i < n; i++) {
		if (i > 0)
			s += Random() % 8 ? "," : " ";
		s += Item();
	}
	return s + "}";
}

static std::string CfgSkill() { char Help[4]; sprintf(Help, "%d", (int)(Random() % SKILLS)); return Help; }
static std::string CfgHint() { return CfgNumber(); }

static std::string CfgLine()
{
	switch (Random() % 41) {
	case 0: return "TestServer = " + CfgYes();
	case 1: return "ServerAddress = " + CfgString(20);
	case 2: return "ServerPort = " + CfgNumber();
	case 3: return "FullscreenMode = (" + CfgNumber() + "," + CfgNumber() + "," + CfgNumber() + ")";
	case 4: return "WindowsMousepointer = " + CfgYes();
	case 5: return "LightEffects = " + CfgYes();
	case 6: return "TextualEffects = " + CfgYes();
	case 7: return "CreatureInfo = " + CfgNumber(true);
	case 8: return "NoStretching = " + CfgYes();
	case 9: return "ConfineFramerate = " + CfgNumber();
	case 10: return "Engine = " + CfgNumber(true);
	case 11: case 12: {
		char Help[8];
		sprintf(Help, "%d", (int)(Random() % HOTKEYS));
		std::string Text = CfgString(Random() % 4 ? 40 : 300);
		if (Random() % 3 == 0)
			Text.insert(Text.size() - 1, "\\n");
		return std::string("Hotkey = (") + Help + "," + Text + ")";
	}
	case 13: return "WindowedMode = (" + CfgNumber(true) + "," + CfgNumber(true) + "," + CfgNumber() + "," + CfgNumber() + "," + CfgYes() + ")";
	case 14: return "ConsoleHeight = " + CfgNumber();
	case 15: return "AutomapZoom = " + CfgNumber(true);
	case 16: return "ContentWindow = (" + CfgNumber() + "," + CfgNumber() + ")";
	case 17: return "BuddySortmethods = (" + CfgNumber() + "," + CfgNumber() + "," + CfgNumber() + ")";
	case 18: return "BuddyHideOffline = " + CfgYes();
	case 19: case 20: { char Help[8]; sprintf(Help, "%d", (int)(Random() % 12) - 1); return std::string("OpenDialogs = ") + Help; }
	case 21: return "LastMotD = " + CfgNumber();
	case 22: return "InfoMessages = " + CfgYes();
	case 23: return "EventMessages = " + CfgYes();
	case 24: return "LoginMessages = " + CfgYes();
	case 25: return "StatusMessages = " + CfgYes();
	case 26: return "TimeStamps = " + CfgYes();
	case 27: return "PrivateMessages = " + CfgYes();
	case 28: return "OpenChannel = " + CfgNumber();
	case 29: return "ClassicControl = " + CfgYes();
	case 30: return "AutoChaseOff = " + CfgYes();
	case 31: return "AttackMode = " + CfgNumber();
	case 32: { char Help[4]; sprintf(Help, "%d", (int)(Random() % 4)); return std::string("ChaseMode = ") + Help; }
	case 33: return "SecureMode = " + CfgNumber();
	case 34: case 35: {
		char Help[8];
		sprintf(Help, "%d", Random() % 5 == 0 ? 0 : (int)(1 + Random() % 30));
		return std::string("Buddy = (") + Help + "," + CfgNumber() + "," + CfgString(60) + "," + CfgYes() + ")";
	}
	case 36: return "White = " + CfgList(CfgName);
	case 37: return "Ignore = " + CfgList(CfgName);
	case 38: return "KnownHints = " + CfgList(CfgHint);
	case 39: return "SkillBars = " + CfgList(CfgSkill);
	default: return Random() % 2 ? "ShowNewHints = " + CfgYes() : (Random() % 2 ? "LevelBar = " : "MagLevelBar = ") + CfgYes();
	}
}

static void CheckTalkLists(const char *When)
{
	char *A = GetIgnoreListText();
	char *B = At<char *(__cdecl *)()>(0x501010)();
	CHECK((A == NULL) == (B == NULL) && (A == NULL || strcmp(A, B) == 0), "ignore list after %s", When);
	free(A);
	free(B);
	A = GetBuddyIgnoreText();
	B = At<char *(__cdecl *)()>(0x5002d0)();
	CHECK((A == NULL) == (B == NULL) && (A == NULL || strcmp(A, B) == 0), "white list after %s", When);
	free(A);
	free(B);
}

static void CompareSaved(const char *When)
{
	SavePreferences();
	bool HaveA, HaveB;
	std::string A = ReadWholeFile("Tibia.cfg", &HaveA);
	remove("Tibia.cfg");
	At<void (__cdecl *)()>(0x4de890)();
	std::string B = ReadWholeFile("Tibia.cfg", &HaveB);
	remove("Tibia.cfg");
	CHECK(HaveA == HaveB && A == B, "Tibia.cfg written after %s differs (%d and %d bytes)", When, (int)A.size(), (int)B.size());
}

void TestPreferencesFile()
{
	PatchScript();
	PatchJump(0x523812, (void *)ImageFree2);
	PatchJump(0x513544, (void *)ImageStrtok);
	PatchJump(0x51233d, (void *)ImageSnprintf);
	// The original's Talk filter lists (static initialisers, see talktest.cpp).
	for (uint32 Address : {0x71ef70u, 0x71ef90u, 0x71efb0u, 0x71efd0u})
		if (Global<void *>(Address + 28) == NULL)
			At<VectorInitCtorFn>(0x4fd6e0)((void *)(uintptr_t)Address, NULL, 0, Address == 0x71efd0 ? 4 : 9,
				Address == 0x71efd0 ? 5 : 10, 0);
	char OldDirectory[MAX_PATH], Help[MAX_PATH];
	GetCurrentDirectoryA(MAX_PATH, OldDirectory);
	GetTempPathA(MAX_PATH, Help);
	std::string Directory = std::string(Help) + "tibia-difftest-cfg";
	CreateDirectoryA(Directory.c_str(), NULL);
	SetCurrentDirectoryA(Directory.c_str());
	remove("Tibia.cfg");

	// Before InitPreferences nothing is written.
	CompareSaved("no InitPreferences");

	for (int Round = 0; Round < 400; Round++) {
		std::string Text;
		int n = (int)(Random() % 60);
		for (int i = 0; i < n; i++) {
			if (Random() % 10 == 0)
				Text += "# comment\n";
			Text += CfgLine();
			Text += Random() % 6 ? "\r\n" : " ";
		}
		FILE *File = fopen("Tibia.cfg", "wb");
		fwrite(Text.data(), 1, Text.size(), File);
		fclose(File);
		if (getenv("DIFFTEST_TRACE") != NULL) {
			fprintf(stderr, "cfg round %d\n", Round);
			FILE *Copy = fopen("last.cfg", "wb");
			fwrite(Text.data(), 1, Text.size(), Copy);
			fclose(Copy);
		}
		InitPreferences();
		At<void (__cdecl *)()>(0x4ddc00)();
		CHECK(Global<bool>(0x71c319), "PreferencesLoaded not set");
		CheckAllOptions("InitPreferences");
		CheckLists("InitPreferences");
		CheckTalkLists("InitPreferences");
		CompareSaved("InitPreferences");
		// Saving and loading again must give the same file.
		if (Round % 10 == 0) {
			bool Trace = getenv("DIFFTEST_TRACE") != NULL;
			SavePreferences();
			if (Trace) fprintf(stderr, "ours init\n");
			InitPreferences();
			remove("Tibia.cfg");
			if (Trace) fprintf(stderr, "original save\n");
			At<void (__cdecl *)()>(0x4de890)();
			if (Trace) fprintf(stderr, "original init\n");
			At<void (__cdecl *)()>(0x4ddc00)();
			if (Trace) fprintf(stderr, "done\n");
			CheckAllOptions("a second InitPreferences");
			CheckLists("a second InitPreferences");
			CompareSaved("a second InitPreferences");
		}
	}

	// ExitPreferences frees the MotD and saves.
	SetMotD("7\nToday's news");
	At<bool (__cdecl *)(const char *)>(0x4dcac0)("7\nToday's news");
	ExitPreferences();
	bool HaveA, HaveB;
	std::string A = ReadWholeFile("Tibia.cfg", &HaveA);
	remove("Tibia.cfg");
	At<void (__cdecl *)()>(0x4df520)();
	std::string B = ReadWholeFile("Tibia.cfg", &HaveB);
	remove("Tibia.cfg");
	CHECK(HaveA && HaveB && A == B, "ExitPreferences output differs");
	CheckAllOptions("ExitPreferences");

	SetCurrentDirectoryA(OldDirectory);
	RemoveDirectoryA(Directory.c_str());
}
