// Differential tests of Creatures.cpp against the original code.
//
// The map, path finder and help functions that Creatures.cpp calls are not
// reconstructed yet. Both sides get the same fakes: the image's copies are
// patched to jump here, the reconstructed side links the definitions below.
// The fakes read a shared, randomly filled screen; the help hints are
// recorded per side and compared. The image's time, sprintf, _stricmp,
// _strdup and stdio are redirected to the test's CRT, and the test CRT's
// _time32 to the same fixed clock while the tests run. Objects come from
// the real Tibia.dat (both loaders), Preferences, Status and Talk are the
// linked reconstructions against the image's own copies.
//
// The creature table and the VIP list are compared byte for byte: the
// reconstructed tables are found through AddCreature's first slot and
// GetBuddyListName(0). Failing assertions are not exercised.

#include "difftest.h"
#include "../../src/Help.h"
#include "../../src/Map.h"
#include "../../src/Creatures.h"
#include "../../src/Objects.h"
#include "../../src/Preferences.h"
#include "../../src/Status.h"
#include "../../src/Talk.h"
#include <ctype.h>
#include <stdarg.h>
#include <string>
#include <vector>

enum { OURS = 0, ORIGINAL = 1 };

// --- Shared fake world -----------------------------------------------------

static int CenterX = 1000, CenterY = 1000, CenterZ = 7; // GetMapPosition
static int FakeLevel = 3;                                // GetMapLevel
static int FieldType[16][16];                            // GetFirstObject, by screen x/y
static bool FieldFree[16][16];                           // IsFieldPassable
static int FieldHeight[16][16];                          // GetFieldHeight
static int PathResult, PathCount, PathDirections[10];    // FindPath
struct TPathCall {
	int Args[6];
	bool Exact, Diagonal;
};
static std::vector<TPathCall> PathCalls[2];
static std::vector<int> Hints[2];
static std::vector<std::string> MonsterHints[2];
static __time32_t FixedTime = 1700000000;

static void GetCenter(int *x, int *y, int *z) { *x = CenterX; *y = CenterY; *z = CenterZ; }
static TObject Field(int x, int y)
{
	TObject Object;
	Object.Type = FieldType[x & 15][y & 15];
	Object.Data1 = 0;
	Object.Data2 = 0;
	return Object;
}
static void ToScreen(int *x, int *y, int *z)
{
	*x = *x - CenterX + 8;
	*y = *y - CenterY + 6;
	*z = *z == CenterZ ? FakeLevel : *z + 50;
}
static int Path(int Side, int x, int y, int z, int DestX, int DestY, int DestZ, bool Exact, int *Steps,
	int *Directions, bool Diagonal)
{
	TPathCall Call = {{x, y, z, DestX, DestY, DestZ}, Exact, Diagonal};
	PathCalls[Side].push_back(Call);
	if (PathResult > 0) {
		*Steps = PathCount;
		for (int i = 0; i < PathCount; i++)
			Directions[i] = PathDirections[i];
	}
	return PathResult;
}

// Our side's Map.cpp functions are redirected to these fakes (PatchOwn) in
// PatchCreatures; maptest.cpp runs before this test, so nothing needs them
// afterwards.
static void __cdecl FakeGetMapPosition(int *x, int *y, int *z) { GetCenter(x, y, z); }
static int __cdecl FakeGetMapLevel() { return FakeLevel; }
static TObject __cdecl FakeGetFirstObject(int rx, int ry, int rz) { (void)rz; return Field(rx, ry); }
static bool __cdecl FakeIsFieldPassable(int x, int y, int z, bool Param4) { (void)z; (void)Param4; return FieldFree[x & 15][y & 15]; }
static int __cdecl FakeGetFieldHeight(int x, int y, int z) { (void)z; return FieldHeight[x & 15][y & 15]; }
static void __cdecl FakeAbsoluteToRelative(int *x, int *y, int *z) { ToScreen(x, y, z); }
static int __cdecl FakeFindPath(int x, int y, int z, int DestX, int DestY, int DestZ, bool Exact, int *Steps,
	int *Directions, bool Diagonal)
{
	return Path(OURS, x, y, z, DestX, DestY, DestZ, Exact, Steps, Directions, Diagonal);
}
// Help.cpp is linked into the test, so our side's AddEventHelp and
// CheckEventHelpName are redirected to these recorders (PatchOwn) while the
// creature tests run.
static void __cdecl OurHint(int Value) { Hints[OURS].push_back(Value); }
static void __cdecl OurMonsterHint(const char *Name) { MonsterHints[OURS].push_back(Name); }

static void PatchOwn(void *Function, void *Target)
{
	DWORD Protection;
	VirtualProtect(Function, 5, PAGE_EXECUTE_READWRITE, &Protection);
	PatchJump((uint32)(uintptr_t)Function, Target);
	VirtualProtect(Function, 5, Protection, &Protection);
	FlushInstructionCache(GetCurrentProcess(), Function, 5);
}

// Replacements patched into the image.
static void __cdecl ImageGetCenter(int *x, int *y, int *z) { GetCenter(x, y, z); }
static int __cdecl ImageGetLevel() { return FakeLevel; }
static TObject __cdecl ImageField(int x, int y, int z) { (void)z; return Field(x, y); }
static bool __cdecl ImageFieldFree(int x, int y, int z, bool Param4) { (void)z; (void)Param4; return FieldFree[x & 15][y & 15]; }
static int __cdecl ImageFieldHeight(int x, int y, int z) { (void)z; return FieldHeight[x & 15][y & 15]; }
static void __cdecl ImageToScreen(int *x, int *y, int *z) { ToScreen(x, y, z); }
static int __cdecl ImagePath(int x, int y, int z, int DestX, int DestY, int DestZ, bool Exact, int *Steps,
	int *Directions, bool Diagonal)
{
	return Path(ORIGINAL, x, y, z, DestX, DestY, DestZ, Exact, Steps, Directions, Diagonal);
}
static void __cdecl ImageHint(int Value) { Hints[ORIGINAL].push_back(Value); }
static void __cdecl ImageMonsterHint(const char *Name) { MonsterHints[ORIGINAL].push_back(Name); }
static __time32_t __cdecl ImageTime(__time32_t *Time)
{
	if (Time != NULL)
		*Time = FixedTime;
	return FixedTime;
}
static int __cdecl ImageSprintf(char *Buffer, const char *Format, ...)
{
	va_list Args;
	va_start(Args, Format);
	int Result = vsprintf(Buffer, Format, Args);
	va_end(Args);
	return Result;
}
static int __cdecl ImageStricmp(const char *a, const char *b) { return _stricmp(a, b); }
static char *__cdecl ImageStrdup(const char *s) { char *t = (char *)malloc(strlen(s) + 1); strcpy(t, s); return t; }
static FILE *__cdecl ImageFopen(const char *Name, const char *Mode) { return fopen(Name, Mode); }
static int __cdecl ImageFclose(FILE *File) { return fclose(File); }
static size_t __cdecl ImageFread(void *Buffer, size_t Size, size_t Count, FILE *File) { return fread(Buffer, Size, Count, File); }

static uint8 SavedClock[5];
static void *ClockFunction;

static void PatchClock(bool On)
{
	DWORD Protection;
	VirtualProtect(ClockFunction, 5, PAGE_EXECUTE_READWRITE, &Protection);
	if (On)
		PatchJump((uint32)(uintptr_t)ClockFunction, (void *)ImageTime);
	else
		memcpy(ClockFunction, SavedClock, 5);
	FlushInstructionCache(GetCurrentProcess(), ClockFunction, 5);
	VirtualProtect(ClockFunction, 5, Protection, &Protection);
}

static void PatchCreatures()
{
	PatchImage();
	PatchJump(0x4b08c0, (void *)ImageGetCenter);
	PatchJump(0x4b08f0, (void *)ImageGetLevel);
	PatchJump(0x4b6140, (void *)ImageField);
	PatchJump(0x4be130, (void *)ImageFieldFree);
	PatchJump(0x4bdf30, (void *)ImageFieldHeight);
	PatchJump(0x4b0960, (void *)ImageToScreen);
	PatchJump(0x4bee00, (void *)ImagePath);
	PatchJump(0x4a32e0, (void *)ImageHint);
	PatchJump(0x4a3460, (void *)ImageMonsterHint);
	PatchOwn((void *)AddEventHelp, (void *)OurHint);
	PatchOwn((void *)CheckEventHelpName, (void *)OurMonsterHint);
	PatchOwn((void *)GetMapPosition, (void *)FakeGetMapPosition);
	PatchOwn((void *)GetMapLevel, (void *)FakeGetMapLevel);
	PatchOwn((void *)GetFirstObject, (void *)FakeGetFirstObject);
	PatchOwn((void *)IsFieldPassable, (void *)FakeIsFieldPassable);
	PatchOwn((void *)GetFieldHeight, (void *)FakeGetFieldHeight);
	PatchOwn((void *)AbsoluteToRelative, (void *)FakeAbsoluteToRelative);
	PatchOwn((void *)FindPath, (void *)FakeFindPath);
	PatchJump(0x5121ea, (void *)ImageTime);
	PatchJump(0x511d6b, (void *)ImageSprintf);
	PatchJump(0x512539, (void *)ImageStricmp);
	PatchJump(0x51491c, (void *)ImageStrdup);
	PatchJump(0x51227f, (void *)ImageFopen);
	PatchJump(0x512122, (void *)ImageFclose);
	PatchJump(0x513a29, (void *)ImageFread);
	ClockFunction = (void *)_time32;
	memcpy(SavedClock, ClockFunction, 5);
	PatchClock(true);
}

// --- The original functions ------------------------------------------------

typedef void (__cdecl *VoidFn)();
typedef int (__cdecl *IntFn)();
typedef bool (__cdecl *BoolFn)();
typedef void (__cdecl *SetFn)(int);
typedef void (__cdecl *Set2Fn)(int, int);
typedef void (__cdecl *Set3Fn)(int, int, int);
typedef int (__cdecl *GetFn)(int);
typedef bool (__cdecl *TestFn)(int);
typedef const char *(__cdecl *NameFn)(int);
typedef TCreature *(__cdecl *AddFn)(uint32, uint32);
typedef TCreature *(__cdecl *CreatureFn)(uint32);
typedef TCreature *(__cdecl *OpponentFn)(int);
typedef bool (__cdecl *CreatureTestFn)(TCreature *);
typedef void (__cdecl *CreatureDoFn)(TCreature *);
typedef void (__cdecl *PositionFn)(int *, int *, int *);
typedef int (__cdecl *DestinationFn)(int, int, int, bool, bool);
typedef bool (__cdecl *PairFn)(int, int);
typedef void (__cdecl *SwapFn)(int, int);
typedef void (__cdecl *AddBuddyFn)(uint32, const char *, bool);
typedef void (__cdecl *BuddyStateFn)(uint32, bool);
typedef void (__cdecl *EditBuddyFn)(int, int, const char *, bool);
typedef bool (__cdecl *NameTestFn)(const char *);
typedef void (__cdecl *SetBoolFn)(bool);
typedef void (__cdecl *SetBuddyFn)(int, int, const char *, bool);
typedef void (__cdecl *FetchFn)(int *, INT_PTR *, INT_PTR *);
typedef const char *(__cdecl *StatusFn)();
typedef void (__cdecl *LoadFn)(const char *);
// thiscall members through __fastcall: ECX = this, EDX unused, rest on the stack.
typedef TRGBColor *(__fastcall *ColorFn)(void *, void *, TRGBColor *);
typedef bool (__fastcall *MemberTestFn)(void *, void *);
typedef int (__fastcall *MemberGetFn)(void *, void *);
typedef int (__fastcall *MemberGet1Fn)(void *, void *, int);
typedef void (__fastcall *MemberSet1Fn)(void *, void *, int);
typedef void (__fastcall *MemberSkullFn)(void *, void *, int, bool);
typedef void (__fastcall *MemberSet3Fn)(void *, void *, int, int, int);

static TCreature *const OriginalCreatures = (TCreature *)(uintptr_t)0x5c68b0;
static TBuddyEntry *const OriginalBuddies = (TBuddyEntry *)(uintptr_t)0x5c4570;
static TCreature *OurCreatures;
static TBuddyEntry *OurBuddies;

static void CheckRecords(const char *What)
{
	CHECK(Hints[OURS] == Hints[ORIGINAL], "%s: help hints differ (%d, original %d)", What,
		(int)Hints[OURS].size(), (int)Hints[ORIGINAL].size());
	CHECK(MonsterHints[OURS] == MonsterHints[ORIGINAL], "%s: monster hints differ (%d, original %d)", What,
		(int)MonsterHints[OURS].size(), (int)MonsterHints[ORIGINAL].size());
	bool Same = PathCalls[OURS].size() == PathCalls[ORIGINAL].size();
	for (size_t i = 0; Same && i < PathCalls[OURS].size(); i++) {
		const TPathCall &A = PathCalls[OURS][i], &B = PathCalls[ORIGINAL][i];
		Same = memcmp(A.Args, B.Args, sizeof(A.Args)) == 0 && A.Exact == B.Exact && A.Diagonal == B.Diagonal;
	}
	CHECK(Same, "%s: path finder calls differ", What);
	for (int s = 0; s < 2; s++) {
		Hints[s].clear();
		MonsterHints[s].clear();
		PathCalls[s].clear();
	}
	const char *A = GetStatusMessage(), *B = At<StatusFn>(0x4fd4d0)();
	CHECK((A == NULL) == (B == NULL) && (A == NULL || strcmp(A, B) == 0), "%s: status message differs", What);
}

static void CheckPlayer(const char *What)
{
	static const struct { uint32 Address; int (*Ours)(); const char *Name; } Getters[] = {
		{0x44b870, GetNumberOfPlayerSteps, "GetNumberOfPlayerSteps"}, {0x44b9d0, GetPlayerHealth, "GetPlayerHealth"},
		{0x44b9e0, GetPlayerMaxHealth, "GetPlayerMaxHealth"}, {0x44b9f0, GetPlayerExperience, "GetPlayerExperience"},
		{0x44ba00, GetPlayerLevel, "GetPlayerLevel"}, {0x44ba10, GetPlayerLevelProgress, "GetPlayerLevelProgress"},
		{0x44ba20, GetPlayerMana, "GetPlayerMana"}, {0x44ba30, GetPlayerMaxMana, "GetPlayerMaxMana"},
		{0x44ba40, GetPlayerSoul, "GetPlayerSoul"}, {0x44ba50, GetPlayerMagicLevel, "GetPlayerMagicLevel"},
		{0x44ba60, GetPlayerMagicLevelProgress, "GetPlayerMagicLevelProgress"},
		{0x44ba70, GetPlayerCapacity, "GetPlayerCapacity"}, {0x44fba0, GetNumberOfBuddies, "GetNumberOfBuddies"},
	};
	for (auto &G : Getters)
		CHECK(G.Ours() == At<IntFn>(G.Address)(), "%s: %s", What, G.Name);
	static const struct { uint32 Address; bool (*Ours)(); } States[] = {
		{0x44ba80, IsPlayerPoisoned}, {0x44ba90, IsPlayerBurning}, {0x44baa0, IsPlayerElectrified},
		{0x44bab0, IsPlayerDrunk}, {0x44bac0, IsPlayerManaShielded}, {0x44bad0, IsPlayerParalyzed},
		{0x44bae0, IsPlayerHasted}, {0x44baf0, IsPlayerInFight},
	};
	for (auto &S : States)
		CHECK(S.Ours() == At<BoolFn>(S.Address)(), "%s: state bit at 0x%x", What, S.Address);
	CHECK(GetPlayerID() == (uint32)At<IntFn>(0x44b9c0)() && GetAttackID() == (uint32)At<IntFn>(0x44bb50)()
		&& GetFollowID() == (uint32)At<IntFn>(0x44bb60)() && GetHighlightID() == (uint32)At<IntFn>(0x44bb70)(),
		"%s: IDs", What);
	int Steps = GetNumberOfPlayerSteps();
	for (int i = 0; i < Steps; i++)
		CHECK(GetPlayerStep(i) == At<GetFn>(0x44c3c0)(i), "%s: GetPlayerStep(%d)", What, i);
	int x1, y1, z1, x2, y2, z2;
	GetPlayerPosition(&x1, &y1, &z1);
	At<PositionFn>(0x44b880)(&x2, &y2, &z2);
	CHECK(x1 == x2 && y1 == y2 && z1 == z2, "%s: GetPlayerPosition", What);
	for (int i = 0; i < SKILLS; i++)
		CHECK(GetPlayerSkillValue(i) == At<GetFn>(0x44d1c0)(i) && GetPlayerSkillProgress(i) == At<GetFn>(0x44d320)(i),
			"%s: skill %d", What, i);
}

static void CheckCreatures(const char *What)
{
	CHECK(memcmp(OurCreatures, OriginalCreatures, CREATURES * sizeof(TCreature)) == 0, "%s: creature table differs", What);
	int Number = GetNumberOfOpponents();
	CHECK(Number == At<IntFn>(0x44d9f0)(), "%s: GetNumberOfOpponents", What);
	CHECK(memcmp(OurCreatures, OriginalCreatures, CREATURES * sizeof(TCreature)) == 0, "%s: battle list differs", What);
	for (int i = 0; i <= Number; i++) {
		TCreature *A = GetOpponent(i), *B = At<OpponentFn>(0x44dc70)(i);
		CHECK((A == NULL ? -1 : (int)(A - OurCreatures)) == (B == NULL ? -1 : (int)(B - OriginalCreatures)),
			"%s: GetOpponent(%d)", What, i);
	}
}

static void CheckBuddies(const char *What)
{
	if (OurBuddies != NULL)
		CHECK(memcmp(OurBuddies, OriginalBuddies, BUDDIES * sizeof(TBuddyEntry)) == 0, "%s: VIP list differs", What);
	CHECK(GetNumberOfBuddies() == At<IntFn>(0x44fba0)(), "%s: GetNumberOfBuddies", What);
}

// --- Creature members ------------------------------------------------------

static void TestMembers(int Slot)
{
	TCreature *A = &OurCreatures[Slot], *B = &OriginalCreatures[Slot];
	TRGBColor C1 = A->GetHealthColor(), C2;
	At<ColorFn>(0x44b740)(B, NULL, &C2);
	CHECK(C1.R == C2.R && C1.G == C2.G && C1.B == C2.B, "GetHealthColor(%d)", A->Health);
	CHECK(A->IsMarked() == At<MemberTestFn>(0x44b800)(B, NULL), "IsMarked");
	CHECK(A->GetDeltaX() == At<MemberGetFn>(0x44bbf0)(B, NULL), "GetDeltaX");
	CHECK(A->GetDeltaY() == At<MemberGetFn>(0x44bc10)(B, NULL), "GetDeltaY");
	CHECK(A->GetDrawDirection() == At<MemberGetFn>(0x44bc30)(B, NULL), "GetDrawDirection");
	int Phases = 1 + (int)(Random() % 8);
	CHECK(A->GetAnimationPhase(Phases) == At<MemberGet1Fn>(0x44c0d0)(B, NULL, Phases), "GetAnimationPhase(%d)", Phases);
	CHECK(IsOpponent(A) == At<CreatureTestFn>(0x44d740)(B), "IsOpponent");
}

static uint32 RandomID(int Count, uint32 *IDs)
{
	if (Count > 0 && Random() % 4 != 0)
		return IDs[Random() % Count];
	return Random() % 3 == 0 ? 1 + Random() % 1000 : 0x40000000 + Random() % 1000;
}

// Fills the screen fakes; the bank types are real Tibia.dat objects.
static std::vector<int> Banks, NonBanks;
static void RandomScreen()
{
	for (int x = 0; x < 16; x++) {
		for (int y = 0; y < 16; y++) {
			int r = (int)(Random() % 10);
			FieldType[x][y] = r == 0 ? 0 : r < 8 ? Banks[Random() % Banks.size()] : NonBanks[Random() % NonBanks.size()];
			FieldFree[x][y] = Random() % 4 != 0;
			FieldHeight[x][y] = (int)(Random() % 4);
		}
	}
	FakeLevel = (int)(Random() % 8);
}

static void RandomPath()
{
	static const int Results[] = {-5, -4, -3, -2, -1, 0, 1, 1, 2, 2};
	PathResult = Results[Random() % 10];
	PathCount = 1 + (int)(Random() % 10);
	for (int i = 0; i < 10; i++)
		PathDirections[i] = (int)(Random() % 10);
}

static void TestCreatureTable()
{
	ResetCreatures();
	At<VoidFn>(0x4509e0)();
	SetBeat(50);
	At<SetFn>(0x44b900)(50);
	// The first creature takes slot 0 on both sides: the player.
	uint32 IDs[CREATURES];
	int Count = 0;
	uint32 Player = 0x10000 + Random() % 1000;
	OurCreatures = AddCreature(0, Player);
	CHECK(At<AddFn>(0x44c530)(0, Player) == OriginalCreatures, "AddCreature slot 0");
	SetPlayerID(Player);
	At<SetFn>(0x44b920)(Player);
	IDs[Count++] = Player;
	for (TCreature *T : {&OurCreatures[0], &OriginalCreatures[0]}) {
		strcpy(T->Name, "Player");
		T->RealX = CenterX;
		T->RealY = CenterY;
		T->RealZ = CenterZ;
		T->Health = 100;
		T->Speed = 220;
		T->Visible = true;
	}
	CheckCreatures("player");

	char What[64];
	for (int t = 0; t < 40000; t++) {
		int Op = (int)(Random() % 22);
		int Slot = (int)(Random() % CREATURES);
		TCreature *A = &OurCreatures[Slot], *B = &OriginalCreatures[Slot];
		sprintf(What, "creature op %d", Op);
		switch (Op) {
		case 0: { // new creature
			if (Count >= 140)
				break;
			uint32 Remove = Random() % 3 == 0 && Count > 1 ? IDs[1 + Random() % (Count - 1)] : Random() % 4 == 0 ? 12345 : 0;
			uint32 New = RandomID(0, IDs);
			TCreature *P = AddCreature(Remove, New), *Q = At<AddFn>(0x44c530)(Remove, New);
			CHECK(P - OurCreatures == Q - OriginalCreatures, "AddCreature slot");
			if (P - OurCreatures != 0 && P - OurCreatures < CREATURES - 1) {
				bool Known = false;
				for (int i = 0; i < Count; i++)
					if (IDs[i] == Remove && Remove != 0) {
						IDs[i] = New;
						Known = true;
					}
				if (!Known)
					IDs[Count++] = New;
			}
			break;
		}
		case 1: { // GetCreature
			uint32 ID = RandomID(Count, IDs);
			if (ID == 0)
				break;
			TCreature *P = GetCreature(ID), *Q = At<CreatureFn>(0x44c6e0)(ID);
			CHECK((P == NULL ? -1 : (int)(P - OurCreatures)) == (Q == NULL ? -1 : (int)(Q - OriginalCreatures)), "GetCreature");
			break;
		}
		case 2: // fields set by Communication.cpp
			if (Slot == 0)
				break;
			{
				int Health = (int)(Random() % 110) - 5, Outfit = (int)(Random() % 3), Color = (int)(Random() % 3);
				bool Visible = Random() % 4 != 0;
				int Speed = (int)(Random() % 400) - 20, Shield = (int)(Random() % 6);
				for (TCreature *T : {A, B}) {
					T->Health = Health;
					T->OutfitType = Outfit;
					T->Colors[0] = Color;
					T->Visible = Visible;
					T->Speed = Speed;
					T->Shield = Shield;
					sprintf(T->Name, "Creature %d", Slot);
				}
			}
			break;
		case 3: { // SetPosition
			if (A->CreatureID == 0)
				break;
			int x = CenterX + (int)(Random() % 21) - 10, y = CenterY + (int)(Random() % 17) - 8;
			int z = CenterZ + (Random() % 5 == 0 ? 1 : 0);
			A->SetPosition(x, y, z);
			At<MemberSet3Fn>(0x452320)(B, NULL, x, y, z);
			break;
		}
		case 4: { // SetSkull
			int Skull = (int)(Random() % 3);
			bool Delay = Random() % 2 != 0;
			A->SetSkull(Skull, Delay);
			At<MemberSkullFn>(0x44b820)(B, NULL, Skull, Delay);
			break;
		}
		case 5: // UpdateSkulls
			FixedTime += (int)(Random() % 4);
			if (Random() % 20 == 0)
				FixedTime -= 10;
			UpdateSkulls();
			At<VoidFn>(0x44c860)();
			break;
		case 6: { // SetMark
			if (A->CreatureID == 0)
				break;
			int Mark = (int)(Random() % 256);
			A->SetMark(Mark);
			At<MemberSet1Fn>(0x44bf20)(B, NULL, Mark);
			break;
		}
		case 7: { // StartMovement
			if (A->CreatureID == 0)
				break;
			int dx = (int)(Random() % 3) - 1, dy = (int)(Random() % 3) - 1;
			int Waypoints = Random() % 5 == 0 ? (int)(Random() % 3) - 1 : 100 + (int)(Random() % 200);
			if (Slot == 0 && Random() % 2 == 0) {
				// The step DoPlayerStep would have predicted.
				int sx, sy, sz;
				GetPlayerPosition(&sx, &sy, &sz);
				dx = sx - CenterX;
				dy = sy - CenterY;
			}
			A->StartMovement(dx, dy, Waypoints);
			At<MemberSet3Fn>(0x44bc70)(B, NULL, dx, dy, Waypoints);
			break;
		}
		case 8: // MoveCreatures
			SetTestClock(GetMilliseconds() + (int)(Random() % 120));
			RandomScreen();
			RandomPath();
			CHECK(MoveCreatures() == At<BoolFn>(0x451ab0)(), "MoveCreatures");
			break;
		case 9: { // HideCreature
			uint32 ID = RandomID(Count, IDs);
			if (ID == 0)
				break;
			HideCreature(ID);
			At<SetFn>(0x44c8c0)(ID);
			break;
		}
		case 10:
			if (Random() % 20 == 0) {
				HideAllCreatures();
				At<VoidFn>(0x44b8e0)();
			}
			break;
		case 11: // CheckMonsterHint
			if (A->CreatureID == 0)
				break;
			CheckMonsterHint(A);
			At<CreatureDoFn>(0x451db0)(B);
			break;
		case 12: { // player data
			int a = (int)(Random() % 2000), b = (int)(Random() % 2000);
			switch (Random() % 8) {
			case 0: SetPlayerHealth(a, b); At<Set2Fn>(0x44b930)(a, b); break;
			case 1: SetPlayerMagicLevel(a, b); At<Set2Fn>(0x44b950)(a, b); break;
			case 2: SetPlayerMana(a, b); At<Set2Fn>(0x44b970)(a, b); break;
			case 3: SetPlayerSoul(a); At<SetFn>(0x44b990)(a); break;
			case 4: SetPlayerCapacity(a); At<SetFn>(0x44b9a0)(a); break;
			case 5: a = (int)Random(); SetPlayerState(a); At<SetFn>(0x44b9b0)(a); break;
			case 6: SetAttackID(a); At<SetFn>(0x44bb00)(a); break;
			case 7: SetFollowID(a); At<SetFn>(0x44bb20)(a); break;
			}
			if (Random() % 10 == 0) {
				int Beat = 1 + (int)(Random() % 100);
				SetBeat(Beat);
				At<SetFn>(0x44b900)(Beat);
			}
			if (Random() % 3 == 0) {
				SetHighlightID(b);
				At<SetFn>(0x44bb40)(b);
			}
			if (Random() % 10 == 0) {
				SetPlayerHealth(0, 100);
				At<Set2Fn>(0x44b930)(0, 100);
			} else if (GetPlayerHealth() == 0) {
				SetPlayerHealth(100, 100);
				At<Set2Fn>(0x44b930)(100, 100);
			}
			break;
		}
		case 13: { // experience and skills
			int Experience = Random() % 4 == 0 ? 0 : (int)(Random() % 100000) - 10;
			int Level = (int)(Random() % 25), Progress = (int)(Random() % 102) - 1;
			if (Random() % 10 == 0)
				Level = 0;
			SetPlayerExperience(Experience, Level, Progress);
			At<Set3Fn>(0x44cc00)(Experience, Level, Progress);
			int Skill = (int)(Random() % SKILLS), Value = (int)(Random() % 30);
			Progress = (int)(Random() % 101);
			SetPlayerSkill(Skill, Value, Progress);
			At<Set3Fn>(0x44cf10)(Skill, Value, Progress);
			CHECK(strcmp(GetSkillName(Skill), At<NameFn>(0x44d480)(Skill)) == 0, "GetSkillName(%d)", Skill);
			break;
		}
		case 14: { // NotifySkull and the player's flags
			int Flag = (int)(Random() % 3);
			NotifySkull(Flag);
			At<SetFn>(0x44ca60)(Flag);
			CHECK(HasPlayerSkull() == At<BoolFn>(0x44d6d0)(), "HasPlayerSkull");
			CHECK(IsPlayerInParty() == At<BoolFn>(0x44d6f0)(), "IsPlayerInParty");
			CHECK(GetPlayerPartyFlag() == At<IntFn>(0x44d720)(), "GetPlayerPartyFlag");
			for (int i = 0; i < 3; i++) {
				uint32 ID = Random() % 3 == 0 ? Random() : RandomID(Count, IDs);
				CHECK(IsPlayer(ID) == At<TestFn>(0x44b910)(ID), "IsPlayer(%u)", ID);
			}
			break;
		}
		case 15: // the player's own creature
			A = &OurCreatures[0];
			B = &OriginalCreatures[0];
			{
				int Skull = (int)(Random() % 3), Shield = (int)(Random() % 6);
				bool Moving = Random() % 2 != 0;
				for (TCreature *T : {A, B}) {
					T->Skull = Skull;
					T->Shield = Shield;
					T->Moving = Moving;
				}
			}
			break;
		case 16: { // SetPlayerDestination
			RandomScreen();
			RandomPath();
			int x, y, z;
			GetPlayerPosition(&x, &y, &z);
			x += (int)(Random() % 21) - 10;
			y += (int)(Random() % 17) - 8;
			if (Random() % 6 == 0)
				z += 1;
			bool Exact = Random() % 2 != 0, Diagonal = Random() % 2 != 0;
			CHECK(SetPlayerDestination(x, y, z, Exact, Diagonal) == At<DestinationFn>(0x451400)(x, y, z, Exact, Diagonal),
				"SetPlayerDestination");
			break;
		}
		case 17: { // CancelPlayerWalk
			RandomScreen();
			RandomPath();
			int Direction = (int)(Random() % 4);
			CHECK(CancelPlayerWalk(Direction) == At<TestFn>(0x4518b0)(Direction), "CancelPlayerWalk");
			break;
		}
		case 18:
			if (Random() % 4 == 0) {
				StopPlayer();
				At<VoidFn>(0x4516e0)();
			} else if (Random() % 4 == 0) {
				ClearPlayerSteps();
				At<VoidFn>(0x44b8c0)();
			}
			break;
		case 19: { // FindPlayerPath
			RandomScreen();
			RandomPath();
			CHECK(FindPlayerPath() == At<IntFn>(0x4510f0)(), "FindPlayerPath");
			break;
		}
		case 20: { // the camera follows the confirmed position
			int dx = (int)(Random() % 3) - 1, dy = (int)(Random() % 3) - 1;
			CenterX += dx;
			CenterY += dy;
			break;
		}
		case 21:
			TestMembers(Slot);
			break;
		}
		CheckRecords(What);
		CheckCreatures(What);
		CheckPlayer(What);
		if (Slot != 0)
			TestMembers(Slot);
		if (Failures > 20)
			break;
	}
}

// --- VIP list --------------------------------------------------------------

static void RandomName(char *Name)
{
	static const char *Parts[] = {"al", "Bo", "ce", "Da", "ex", "Fi", "go", "Ha", "a", "B", " ", "'"};
	Name[0] = 0;
	int n = 1 + (int)(Random() % 5);
	for (int i = 0; i < n; i++)
		strcat(Name, Parts[Random() % 12]);
}

static void TestBuddies()
{
	ResetCreatures();
	At<VoidFn>(0x4509e0)();
	OurBuddies = NULL;
	ClearMessages();
	At<VoidFn>(0x4e76e0)();
	uint32 IDs[64];
	for (int i = 0; i < 64; i++)
		IDs[i] = 1 + Random() % 100000;
	char What[64];
	for (int t = 0; t < 30000; t++) {
		int Op = (int)(Random() % 14);
		uint32 ID = IDs[Random() % 64];
		sprintf(What, "buddy op %d", Op);
		switch (Op) {
		case 0:
		case 1: { // AddBuddy
			char Name[32]; // at most 64 different IDs, so the list never fills
			RandomName(Name);
			bool Online = Random() % 2 != 0;
			AddBuddy(ID, Name, Online);
			At<AddBuddyFn>(0x44eab0)(ID, Name, Online);
			if (OurBuddies == NULL)
				OurBuddies = (TBuddyEntry *)(GetBuddyListName(0) - 4);
			break;
		}
		case 2:
		case 3: { // SetBuddyState
			bool Online = Random() % 2 != 0;
			SetBuddyState(ID, Online);
			At<BuddyStateFn>(0x44f000)(ID, Online);
			break;
		}
		case 4: { // EditBuddy
			char Description[32];
			RandomName(Description);
			if (Random() % 3 == 0)
				Description[0] = 0;
			int Icon = (int)(Random() % 4);
			bool Notify = Random() % 2 != 0;
			EditBuddy(ID, Icon, Description, Notify);
			At<EditBuddyFn>(0x44f230)(ID, Icon, Description, Notify);
			break;
		}
		case 5: { // RemoveBuddy
			if (Random() % 3 != 0)
				break;
			bool Known = GetBuddyPosition(ID) != -1;
			RemoveBuddy(ID);
			At<SetFn>(0x44f5e0)(ID);
			if (!Known)
				break;
			int Type[2];
			INT_PTR P1[2], P2[2];
			FetchMessage(&Type[OURS], &P1[OURS], &P2[OURS]);
			At<FetchFn>(0x4e75f0)(&Type[ORIGINAL], &P1[ORIGINAL], &P2[ORIGINAL]);
			CHECK(Type[OURS] == 0x1a && Type[ORIGINAL] == 0x1a && P2[OURS] == 0 && P2[ORIGINAL] == 0
				&& strcmp((char *)P1[OURS], (char *)P1[ORIGINAL]) == 0, "RemoveBuddy message");
			if (Type[OURS] == 0x1a)
				free((void *)P1[OURS]);
			if (Type[ORIGINAL] == 0x1a)
				free((void *)P1[ORIGINAL]);
			break;
		}
		case 6: { // SetBuddySortMethod
			int Method = (int)(Random() % 3);
			SetBuddySortMethod(Method);
			At<SetFn>(0x44f810)(Method);
			int A[3], B[3];
			GetBuddySortmethods(A);
			At<void (__cdecl *)(int *)>(0x4db510)(B);
			CHECK(memcmp(A, B, sizeof(A)) == 0, "stored sort methods");
			break;
		}
		case 7: { // HideOfflineBuddies
			bool Hide = Random() % 3 == 0;
			HideOfflineBuddies(Hide);
			At<SetBoolFn>(0x44fa00)(Hide);
			break;
		}
		case 8: { // list accessors
			// GetNumberOfBuddies counts the online buddies when the offline
			// ones are hidden, and AddBuddy can leave that count too high;
			// the whole list is NumberOfBuddies long.
			bool Hidden = GetBuddyHideOffline();
			SetBuddyHideOffline(false);
			At<SetBoolFn>(0x4db530)(false);
			int Number = GetNumberOfBuddies();
			CHECK(Number == At<IntFn>(0x44fba0)(), "NumberOfBuddies");
			SetBuddyHideOffline(Hidden);
			At<SetBoolFn>(0x4db530)(Hidden);
			for (int p = 0; p < Number; p++) {
				CHECK(GetBuddyListID(p) == (uint32)At<GetFn>(0x44fd40)(p), "GetBuddyListID(%d)", p);
				CHECK(strcmp(GetBuddyListName(p), At<NameFn>(0x44feb0)(p)) == 0, "GetBuddyListName(%d)", p);
				CHECK(GetBuddyListOnline(p) == At<TestFn>(0x450020)(p), "GetBuddyListOnline(%d)", p);
				CHECK(GetBuddyListNew(p) == At<TestFn>(0x450190)(p), "GetBuddyListNew(%d)", p);
				CHECK(GetBuddyListIcon(p) == At<GetFn>(0x450330)(p), "GetBuddyListIcon(%d)", p);
				const char *A = GetBuddyListDescription(p), *B = At<NameFn>(0x4504a0)(p);
				CHECK((A == NULL) == (B == NULL) && (A == NULL || strcmp(A, B) == 0), "GetBuddyListDescription(%d)", p);
				CHECK(GetBuddyListNotify(p) == At<TestFn>(0x450740)(p), "GetBuddyListNotify(%d)", p);
			}
			break;
		}
		case 9: { // GetBuddyPosition, IsBuddy
			CHECK(GetBuddyPosition(ID) == At<GetFn>(0x44bb80)(ID), "GetBuddyPosition");
			char Name[32];
			if (OurBuddies != NULL && GetBuddyPosition(ID) >= 0 && Random() % 2 == 0) {
				strcpy(Name, OurBuddies[GetBuddyPosition(ID)].Name);
				for (char *c = Name; *c; c++)
					if (Random() % 2 == 0)
						*c = (char)toupper((uint8)*c);
			} else {
				RandomName(Name);
			}
			CHECK(IsBuddy(Name) == At<NameTestFn>(0x44bbb0)(Name), "IsBuddy(%s)", Name);
			break;
		}
		case 10: { // CompareBuddies, SwapBuddies, SortBuddy, SortBuddies
			HideOfflineBuddies(false);
			At<SetBoolFn>(0x44fa00)(false);
			int Number = GetNumberOfBuddies();
			if (Number < 2)
				break;
			int i = (int)(Random() % Number), j = (int)(Random() % Number);
			if (i == j)
				break;
			CHECK(CompareBuddies(i, j) == At<PairFn>(0x44de20)(i, j), "CompareBuddies(%d,%d)", i, j);
			switch (Random() % 3) {
			case 0: SwapBuddies(i, j); At<SwapFn>(0x44e260)(i, j); break;
			case 1: SortBuddy(i); At<SetFn>(0x44e7d0)(i); break;
			case 2: SortBuddies(); At<VoidFn>(0x44e600)(); break;
			}
			break;
		}
		case 11: // the action number for GetBuddyListNew
			IncrementActionNumber();
			At<VoidFn>(0x4e6e10)();
			break;
		case 12: // preferences entry behind the list
			if (Random() % 4 == 0) {
				int Icon = (int)(Random() % 4);
				bool Notify = Random() % 2 != 0;
				SetBuddy((int)ID, Icon, "note", Notify);
				At<SetBuddyFn>(0x4dcfb0)((int)ID, Icon, "note", Notify);
			}
			break;
		case 13:
			if (Random() % 50 == 0) {
				ResetCreatures();
				At<VoidFn>(0x4509e0)();
			}
			break;
		}
		CheckRecords(What);
		CheckBuddies(What);
		if (Failures > 20)
			break;
	}
	InitCreatures();
	At<VoidFn>(0x452190)();
	CheckBuddies("InitCreatures");
	CheckPlayer("InitCreatures");
}

void TestCreatures(const char *ExePath)
{
	int Before = Checks, BeforeFailures = Failures;
	PatchCreatures();
	std::string Dat = ExePath;
	size_t Slash = Dat.find_last_of("\\/");
	Dat = (Slash == std::string::npos ? std::string() : Dat.substr(0, Slash + 1)) + "Tibia.dat";
	FILE *Probe = fopen(Dat.c_str(), "rb");
	if (Probe == NULL) {
		printf("note: %s not found, Creatures tests skipped\n", Dat.c_str());
		PatchClock(false);
		return;
	}
	fclose(Probe);
	LoadObjects(Dat.c_str());
	At<LoadFn>(0x4d7380)(Dat.c_str());
	for (int Type = 100; Type < 1500; Type++) {
		bool Bank = GetObjectFlag(Type, BANK);
		CHECK(Bank == At<bool (__cdecl *)(int, int)>(0x4d1c40)(Type, BANK), "bank flag %d", Type);
		(Bank ? Banks : NonBanks).push_back(Type);
	}
	SetTestClock(100000);
	TestCreatureTable();
	TestBuddies();
	ReleaseObjects();
	At<VoidFn>(0x4d70c0)();
	PatchClock(false);
	printf("Creatures: %d checks, %d failures\n", Checks - Before, Failures - BeforeFailures);
}
