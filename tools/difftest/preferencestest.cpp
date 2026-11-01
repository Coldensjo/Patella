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
