// Differential tests of Objects.cpp against the original code.
//
// The image's malloc and free (PatchImage) and the stdio functions that its
// TReadBinaryFile uses (fopen, fread, fclose) are redirected to ours, so the
// original loads real files. Both sides load the real Tibia.dat (next to
// Tibia.exe, if it is there) and random well-formed .dat files; then every
// accessor is called on both with valid arguments. Failing assertions are
// not exercised: the original would need its exception machinery.

#include "difftest.h"
#include "../../src/Objects.h"
#include <direct.h>
#include <string>
#include <vector>

static_assert(sizeof(TProperties) == 0x4c, "TProperties is 0x4c bytes");
static_assert(sizeof(array<TProperties>) == 12, "array<T> is 12 bytes");

// --- Replacements patched into the image -----------------------------------

static FILE *__cdecl ImageFopen(const char *Name, const char *Mode) { return fopen(Name, Mode); }
static int __cdecl ImageFclose(FILE *File) { return fclose(File); }
static size_t __cdecl ImageFread(void *Buffer, size_t Size, size_t Count, FILE *File) { return fread(Buffer, Size, Count, File); }

static void PatchObjects()
{
	static bool Done = false;
	if (Done)
		return;
	Done = true;
	PatchImage();
	PatchJump(0x51227f, (void *)ImageFopen);
	PatchJump(0x512122, (void *)ImageFclose);
	PatchJump(0x513a29, (void *)ImageFread);
}

// --- Original functions --------------------------------------------------------

typedef void (__cdecl *VoidFn)();
typedef void (__cdecl *LoadFn)(const char *FileName);
typedef int (__cdecl *IntFn)(int);
typedef bool (__cdecl *FlagFn)(int, int);
typedef int (__cdecl *Sprite8Fn)(int, int, int, int, int, int, int, int);
typedef int (__cdecl *Sprite6Fn)(int, int, int, int, int, int);
typedef int (__cdecl *Sprite5Fn)(int, int, int, int, int);
typedef void (__cdecl *PatternFn)(TObject *, int *, int *);

static array<TProperties> *&OriginalTable(int Kind)
{
	static const uint32 Address[4] = {0x719d98, 0x719d94, 0x719d90, 0x719d8c};
	return Global<array<TProperties> *>(Address[Kind]);
}

static array<TProperties> *&OurTable(int Kind)
{
	array<TProperties> **Tables[4] = {&ObjectProperties, &OutfitProperties, &EffectProperties, &MissileProperties};
	return *Tables[Kind];
}

static const char *KindName[4] = {"object", "outfit", "effect", "missile"};

static int SpriteCount(const TProperties &E)
{
	return E.Width * E.Height * E.Layers * E.PatternWidth * E.PatternHeight * E.PatternDepth * E.Phases;
}

// Compares the four tables of both sides entry by entry.
static void CompareTables(const char *What)
{
	CHECK(DatSignature == Global<uint32>(0x719d9c), "%s: signature 0x%x, original 0x%x", What, DatSignature,
		Global<uint32>(0x719d9c));
	for (int k = 0; k < 4; k++) {
		array<TProperties> *Ours = OurTable(k), *Original = OriginalTable(k);
		CHECK((Ours == NULL) == (Original == NULL), "%s: %s table presence differs", What, KindName[k]);
		if (Ours == NULL || Original == NULL)
			continue;
		CHECK(Ours->min == Original->min && Ours->d == Original->d, "%s: %s table %d+%d, original %d+%d",
			What, KindName[k], Ours->min, Ours->d, Original->min, Original->d);
		if (Ours->min != Original->min || Ours->d != Original->d)
			continue;
		for (int i = 0; i < Ours->d; i++) {
			TProperties A = Ours->entry[i], B = Original->entry[i];
			bool Same = SpriteCount(A) == SpriteCount(B)
				&& memcmp(A.Sprites, B.Sprites, SpriteCount(A) * sizeof(uint16)) == 0;
			A.Sprites = B.Sprites = NULL;
			Same = Same && memcmp(&A, &B, sizeof(A)) == 0;
			CHECK(Same, "%s: %s %d differs", What, KindName[k], Ours->min + i);
		}
	}
}

// --- Accessors -------------------------------------------------------------

static bool HasFlag(const TProperties &E, int Flag)
{
	return (E.Flags[Flag / 8] & (1 << (Flag % 8))) != 0;
}

struct TIntAccessor {
	uint32 Address;
	int (*Ours)(int);
	int Flag; // -1: no flag needed
	const char *Name;
};

static const TIntAccessor ObjectAccessors[] = {
	{0x4d1f30, GetObjectWaypoints, BANK, "GetObjectWaypoints"},
	{0x4d20d0, GetObjectBrightness, LIGHT, "GetObjectBrightness"},
	{0x4d2270, GetObjectLightColor, LIGHT, "GetObjectLightColor"},
	{0x4d2410, GetObjectShiftX, SHIFT, "GetObjectShiftX"},
	{0x4d25b0, GetObjectShiftY, SHIFT, "GetObjectShiftY"},
	{0x4d2750, GetObjectElevation, HEIGHT, "GetObjectElevation"},
	{0x4d28f0, GetObjectAutomapColor, AUTOMAP, "GetObjectAutomapColor"},
	{0x4d2a90, GetObjectLensHelp, LENSHELP, "GetObjectLensHelp"},
	{0x4d2c30, GetObjectWidth, -1, "GetObjectWidth"},
	{0x4d2dd0, GetObjectHeight, -1, "GetObjectHeight"},
	{0x4d2f70, GetObjectExactSize, -1, "GetObjectExactSize"},
	{0x4d3110, GetObjectLayers, -1, "GetObjectLayers"},
	{0x4d32b0, GetObjectPatternWidth, -1, "GetObjectPatternWidth"},
	{0x4d3450, GetObjectPatternHeight, -1, "GetObjectPatternHeight"},
	{0x4d35f0, GetObjectPatternDepth, -1, "GetObjectPatternDepth"},
	{0x4d3790, GetObjectPhases, -1, "GetObjectPhases"},
};
static const TIntAccessor OutfitAccessors[] = {
	{0x4d3cc0, GetOutfitBrightness, LIGHT, "GetOutfitBrightness"},
	{0x4d3e60, GetOutfitLightColor, LIGHT, "GetOutfitLightColor"},
	{0x4d4000, GetOutfitShiftX, SHIFT, "GetOutfitShiftX"},
	{0x4d41a0, GetOutfitShiftY, SHIFT, "GetOutfitShiftY"},
	{0x4d4340, GetOutfitWidth, -1, "GetOutfitWidth"},
	{0x4d44e0, GetOutfitExactSize, -1, "GetOutfitExactSize"},
	{0x4d4680, GetOutfitHeight, -1, "GetOutfitHeight"},
	{0x4d4820, GetOutfitLayers, -1, "GetOutfitLayers"},
	{0x4d49c0, GetOutfitPhases, -1, "GetOutfitPhases"},
};
static const TIntAccessor EffectAccessors[] = {
	{0x4d4ef0, GetEffectBrightness, LIGHT, "GetEffectBrightness"},
	{0x4d5090, GetEffectLightColor, LIGHT, "GetEffectLightColor"},
	{0x4d5230, GetEffectWidth, -1, "GetEffectWidth"},
	{0x4d53d0, GetEffectHeight, -1, "GetEffectHeight"},
	{0x4d5570, GetEffectLayers, -1, "GetEffectLayers"},
	{0x4d5710, GetEffectPhases, -1, "GetEffectPhases"},
};
static const TIntAccessor MissileAccessors[] = {
	{0x4d5c40, GetMissileBrightness, LIGHT, "GetMissileBrightness"},
	{0x4d5de0, GetMissileLightColor, LIGHT, "GetMissileLightColor"},
	{0x4d5f80, GetMissileWidth, -1, "GetMissileWidth"},
	{0x4d6120, GetMissileHeight, -1, "GetMissileHeight"},
	{0x4d62c0, GetMissileLayers, -1, "GetMissileLayers"},
};

struct TAccessorSet {
	const TIntAccessor *List;
	int Count;
	uint32 FlagAddress;
	bool (*OurFlag)(int, int);
};

static const TAccessorSet Sets[4] = {
	{ObjectAccessors, sizeof(ObjectAccessors) / sizeof(ObjectAccessors[0]), 0x4d1c40, GetObjectFlag},
	{OutfitAccessors, sizeof(OutfitAccessors) / sizeof(OutfitAccessors[0]), 0x4d3af0, GetOutfitFlag},
	{EffectAccessors, sizeof(EffectAccessors) / sizeof(EffectAccessors[0]), 0x4d4d20, GetEffectFlag},
	{MissileAccessors, sizeof(MissileAccessors) / sizeof(MissileAccessors[0]), 0x4d5a70, GetMissileFlag},
};

static int Pick(int Max) // 1..Max
{
	return 1 + (int)(Random() % (uint32)Max);
}

static void TestEntry(int Kind, int Number, const TProperties &E)
{
	const TAccessorSet &S = Sets[Kind];
	for (int f = 0; f < 32; f++) {
		bool A = S.OurFlag(Number, f), B = At<FlagFn>(S.FlagAddress)(Number, f);
		CHECK(A == B, "%s flag %d of %d: %d, original %d", KindName[Kind], f, Number, A, B);
	}
	for (int a = 0; a < S.Count; a++) {
		const TIntAccessor &F = S.List[a];
		if (F.Flag >= 0 && !HasFlag(E, F.Flag))
			continue;
		int A = F.Ours(Number), B = At<IntFn>(F.Address)(Number);
		CHECK(A == B, "%s(%d): %d, original %d", F.Name, Number, A, B);
	}
	if (SpriteCount(E) <= 0)
		return;
	for (int t = 0; t < 4; t++) {
		int X = Pick(E.Width), Y = Pick(E.Height), Layer = Pick(E.Layers), Phase = Pick(E.Phases);
		int A = 0, B = 0;
		switch (Kind) {
		case 0: {
			int PX = Pick(E.PatternWidth), PY = Pick(E.PatternHeight), PZ = Pick(E.PatternDepth);
			A = GetObjectSpriteNumber(Number, X, Y, Layer, PX, PY, PZ, Phase);
			B = At<Sprite8Fn>(0x4d3930)(Number, X, Y, Layer, PX, PY, PZ, Phase);
			break;
		}
		case 1: {
			int Direction = Pick(E.PatternWidth) - 1;
			A = GetOutfitSpriteNumber(Number, X, Y, Layer, Direction, Phase);
			B = At<Sprite6Fn>(0x4d4b60)(Number, X, Y, Layer, Direction, Phase);
			break;
		}
		case 2:
			A = GetEffectSpriteNumber(Number, X, Y, Layer, Phase);
			B = At<Sprite5Fn>(0x4d58b0)(Number, X, Y, Layer, Phase);
			break;
		case 3: {
			if (E.PatternWidth < 3 || E.PatternHeight < 3)
				return;
			int Direction = (int)(Random() % 8);
			A = GetMissileSpriteNumber(Number, X, Y, Layer, Direction);
			B = At<Sprite5Fn>(0x4d6460)(Number, X, Y, Layer, Direction);
			break;
		}
		}
		CHECK(A == B, "%s %d sprite number: %d, original %d", KindName[Kind], Number, A, B);
	}
}

static void TestPatterns(int Type, const TProperties &E)
{
	struct { uint32 Address; void (*Ours)(TObject &, int *, int *); bool Usable; const char *Name; } Functions[] = {
		{0x4d6800, GetCumulativePattern, HasFlag(E, CUMULATIVE), "GetCumulativePattern"},
		{0x4d6b20, GetLiquidPattern, HasFlag(E, LIQUIDPOOL) || HasFlag(E, LIQUIDCONTAINER), "GetLiquidPattern"},
		{0x4d6df0, GetHangPattern, HasFlag(E, HANG), "GetHangPattern"},
	};
	static const int Data[] = {-1000, -101, -9, -5, -4, -3, -1, 0, 1, 2, 3, 4, 5, 9, 10, 24, 25, 49, 50, 100, 255, 1 << 20};
	for (auto &F : Functions) {
		if (!F.Usable)
			continue;
		for (int t = 0; t < 24; t++) {
			TObject Obj;
			Obj.Type = Type;
			Obj.Data = t < 22 ? Data[t] : (int)(Random() ^ (Random() << 16));
			Obj.Data2 = t % 4 == 0 ? HOOKSOUTH : t % 4 == 1 ? HOOKEAST : (int)(Random() % 40) - 5;
			int X1 = -7, Y1 = -7, X2 = -7, Y2 = -7;
			F.Ours(Obj, &X1, &Y1);
			At<PatternFn>(F.Address)(&Obj, &X2, &Y2);
			CHECK(X1 == X2 && Y1 == Y2, "%s(%d, %d, %d): %d,%d original %d,%d", F.Name, Type, Obj.Data,
				Obj.Data2, X1, Y1, X2, Y2);
		}
	}
}

static void TestAccessors()
{
	for (int k = 0; k < 4; k++) {
		array<TProperties> *T = OurTable(k);
		for (int i = 0; i < T->d; i++) {
			TestEntry(k, T->min + i, T->entry[i]);
			if (k == 0)
				TestPatterns(T->min + i, T->entry[i]);
		}
	}
	for (int f = 0; f < 32; f++) {
		bool A = GetObjectFlag(CREATURE, f), B = At<FlagFn>(0x4d1c40)(CREATURE, f);
		CHECK(A == B, "GetObjectFlag(CREATURE, %d): %d, original %d", f, A, B);
	}
}

// --- Random .dat files -------------------------------------------------------

static void Put16(std::string &s, int v)
{
	s += (char)(v & 0xff);
	s += (char)((v >> 8) & 0xff);
}

static void RandomEntry(std::string &s, bool Wild)
{
	static const int Known[] = {BANK, WRITE, WRITEONCE, LIGHT, SHIFT, HEIGHT, AUTOMAP, LENSHELP,
		CUMULATIVE, LIQUIDPOOL, LIQUIDCONTAINER, HANG, HOOKSOUTH, HOOKEAST, UNPASS};
	int Flags = (int)(Random() % 12);
	for (int f = 0; f < Flags; f++) {
		int Flag;
		switch (Random() % 4) {
		case 0: Flag = (int)(Random() % 32); break;
		case 1: Flag = Wild ? (int)(Random() % 255) : (int)(Random() % 30); break;
		default: Flag = Known[Random() % (sizeof(Known) / sizeof(Known[0]))]; break;
		}
		s += (char)Flag;
		switch (Flag) {
		case BANK: case WRITE: case WRITEONCE: case HEIGHT: case AUTOMAP: case LENSHELP:
			Put16(s, (int)Random());
			break;
		case LIGHT: case SHIFT:
			Put16(s, (int)Random());
			Put16(s, (int)Random());
			break;
		}
	}
	s += (char)0xff;
	int Width = Pick(2), Height = Pick(2);
	s += (char)Width;
	s += (char)Height;
	if (Width > 1 || Height > 1)
		s += (char)(Random() % 256);
	int Layers = Pick(2), PW = Pick(4), PH = Pick(4), PD = Pick(2), Phases = Pick(3);
	s += (char)Layers;
	s += (char)PW;
	s += (char)PH;
	s += (char)PD;
	s += (char)Phases;
	int Count = Width * Height * Layers * PW * PH * PD * Phases;
	for (int i = 0; i < Count; i++)
		Put16(s, (int)Random());
}

static std::string RandomDat(bool Wild)
{
	std::string s;
	uint32 Signature = Random() ^ (Random() << 16);
	for (int i = 0; i < 4; i++)
		s += (char)(Signature >> (8 * i));
	int Objects = 100 + (int)(Random() % 40), Outfits = Pick(8), Effects = Pick(8), Missiles = Pick(8);
	Put16(s, Objects);
	Put16(s, Outfits);
	Put16(s, Effects);
	Put16(s, Missiles);
	for (int i = 100; i <= Objects; i++)
		RandomEntry(s, Wild);
	for (int i = 0; i < Outfits + Effects + Missiles; i++)
		RandomEntry(s, Wild);
	return s;
}

static void WriteFile(const std::string &Name, const std::string &Data)
{
	FILE *File = fopen(Name.c_str(), "wb");
	fwrite(Data.data(), 1, Data.size(), File);
	fclose(File);
}

static void ExitBoth(const char *What)
{
	ExitObjects();
	At<VoidFn>(0x4d79b0)();
	for (int k = 0; k < 4; k++)
		CHECK(OurTable(k) == NULL && OriginalTable(k) == NULL, "%s leaves the %s table", What, KindName[k]);
}

void TestObjects(const char *ExePath)
{
	PatchObjects();
	CHECK(memcmp(&NoObject, (void *)(uintptr_t)0x719da0, sizeof(TObject)) == 0, "NoObject differs");

	// The real Tibia.dat, next to Tibia.exe.
	std::string Dat = ExePath;
	size_t Slash = Dat.find_last_of("\\/");
	Dat = (Slash == std::string::npos ? std::string() : Dat.substr(0, Slash + 1)) + "Tibia.dat";
	FILE *Probe = fopen(Dat.c_str(), "rb");
	if (Probe != NULL) {
		fclose(Probe);
		LoadObjects(Dat.c_str());
		At<LoadFn>(0x4d7380)(Dat.c_str());
		CompareTables("Tibia.dat");
		TestAccessors();
		ExitBoth("ExitObjects");
	} else {
		printf("note: %s not found, testing generated files only\n", Dat.c_str());
	}

	char Help[MAX_PATH];
	GetTempPathA(MAX_PATH, Help);
	std::string TempDir = std::string(Help) + "tibia-objectstest\\";
	CreateDirectoryA(TempDir.c_str(), NULL);
	std::string Name = TempDir + "Tibia.dat";

	// Random files through LoadObjects, which first releases the old tables.
	for (int t = 0; t < 60; t++) {
		WriteFile(Name, RandomDat(t % 3 == 2));
		LoadObjects(Name.c_str());
		At<LoadFn>(0x4d7380)(Name.c_str());
		CompareTables("random .dat");
		if (t % 3 != 2)
			TestAccessors(); // flags above 31 change the fields; compared above only
	}
	ExitBoth("ExitObjects");

	// InitObjects and ReloadObjects read "Tibia.dat" in the current directory.
	char Saved[MAX_PATH];
	_getcwd(Saved, MAX_PATH);
	_chdir(TempDir.c_str());
	WriteFile(Name, RandomDat(false));
	InitObjects();
	At<VoidFn>(0x4d77f0)();
	CompareTables("InitObjects");
	WriteFile(Name, RandomDat(false));
	ReloadObjects();
	At<VoidFn>(0x4d7b40)();
	CompareTables("ReloadObjects");
	TestAccessors();
	ExitBoth("ExitObjects after ReloadObjects");
	ReleaseObjects();
	At<VoidFn>(0x4d70c0)();
	_chdir(Saved);

	remove(Name.c_str());
	RemoveDirectoryA(TempDir.c_str());
}
