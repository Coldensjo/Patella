// Differential tests of Map.cpp against the original code.
//
// Both sides load the real Tibia.dat (objects, effects), get their static
// objects constructed (the image's by calling its constructors 0x4b0bc0,
// 0x4b0d70 and 0x4b1410, since its static initialisers never run) and are
// initialised with InitMap; then the same random sequence of map, effect,
// light and automap operations runs on both, and every public accessor is
// compared at every map point. The image's stdio (PatchScript) works on real
// files: each side has its own directory of automap files "<bx><by><bz>.map",
// generated beforehand for every block the test reaches (opening a missing
// file raises an exception, which the original cannot do here), and the files
// both write are compared at the end. Inputs never fail an assertion; the
// default text buffer is NULL on both sides, so no console message is
// written.

#include "difftest.h"
#include "../../src/Map.h"
#include "../../src/Objects.h"
#include "../../src/Talk.h"
#include <direct.h>
#include <string>
#include <vector>

static_assert(sizeof(TMapPoint) == 0xac, "TMapPoint is 0xac bytes");
static_assert(sizeof(TEffect) == 0x44, "TEffect is 0x44 bytes");
static_assert(sizeof(TMark) == 0x70, "TMark is 0x70 bytes");
static_assert(sizeof(vector<TMark>) == 0x8c, "vector<TMark> is 0x8c bytes");
static_assert(sizeof(TAutomapBlock) == 0x200a8, "TAutomapBlock is 0x200a8 bytes");
static_assert(sizeof(TPathNode) == 0x20, "TPathNode is 0x20 bytes");
static_assert(sizeof(matrix3d<int>) == 0x1c, "matrix3d is 0x1c bytes");

// --- Original functions ----------------------------------------------------

typedef void (__cdecl *VoidFn)();
typedef void (__cdecl *LoadFn)(const char *FileName);
typedef void (__cdecl *Int3Fn)(int, int, int);
typedef void (__cdecl *Ptr3Fn)(int *, int *, int *);
typedef int (__cdecl *IntFn)();
typedef int (__cdecl *IntIntFn)(int);
typedef bool (__cdecl *BoolFn)();
typedef void (__cdecl *Int2Fn)(int, int);
typedef void (__cdecl *Int1Fn)(int);
typedef TRGBColor *(__cdecl *ColorFn)(TRGBColor *);
typedef TRGBColor *(__cdecl *ColorAtFn)(TRGBColor *, int, int, int);
typedef TObject *(__cdecl *ObjAtFn)(TObject *, int, int, int);
typedef TObject *(__cdecl *ObjNextFn)(TObject *);
typedef TObject *(__cdecl *ObjPosFn)(TObject *, int, int, int, int);
typedef TObject *(__cdecl *ObjRefFn)(TObject *, int, int, int, int *);
typedef void (__cdecl *PutFn)(int, int, int, TObject);
typedef void (__cdecl *PutAtFn)(int, int, int, int, TObject);
typedef bool (__cdecl *DeleteFn)(int, int, int, int);
typedef int (__cdecl *Int3RetFn)(int, int, int);
typedef bool (__cdecl *Bool3Fn)(int, int, int);
typedef bool (__cdecl *Bool4Fn)(int, int, int, bool);
typedef void (__cdecl *Clear4Fn)(int, int, int, bool);
typedef char *(__cdecl *TextFn)(int);
typedef void (__cdecl *Effect4Fn)(int, int, int, int);
typedef void (__cdecl *TextEffectFn)(int, int, int, int, const char *);
typedef void (__cdecl *MissileFn)(int, int, int, int, int, int, int);
typedef void (__cdecl *MarksFn)(int, int, int, vector<TMark> *, int *);
typedef void (__cdecl *AddMarkFn)(TCoordinate *, const char *, int);
typedef void (__cdecl *DeleteMarkFn)(TCoordinate *, int);
typedef int (__cdecl *PathFn)(int, int, int, int, int, int, bool, int *, int *, bool);
typedef void (__cdecl *InitFn)(void (*)(int));
typedef void *(__fastcall *Ctor2Fn)(void *This, void *, int, int);
typedef void *(__fastcall *Ctor6Fn)(void *This, void *, int, int, int, int, int, int);
typedef void *(__fastcall *Ctor0Fn)(void *This, void *);

// --- Creature callbacks ------------------------------------------------------

static std::vector<int> Removed[2];
static void OurCallback(int CreatureID) { Removed[0].push_back(CreatureID); }
static void __cdecl OriginalCallback(int CreatureID) { Removed[1].push_back(CreatureID); }

// Coverage counters for the summary line.
static int PathResults[8];      // FindPath results -4..2, for the summary
static int StatusSeen[4];       // effect statuses met by CompareEffects

// --- Directories -----------------------------------------------------------------

static std::string Dir[2];
static void Side(int s) { _chdir(Dir[s].c_str()); }

static const int BlockX[2] = {1, 2};
static const int LevelMin = 5, LevelMax = 9;

static void Put(std::string &Data, uint32 Value, int Bytes)
{
	for (int i = 0; i < Bytes; i++)
		Data += (char)(Value >> (8 * i));
}

// A well-formed automap file: colours, speeds and some marks.
static std::string RandomMapFile(int bx, int by)
{
	std::string Data;
	for (int i = 0; i < 0x10000; i++)
		Data += (char)(Random() % 216);
	for (int i = 0; i < 0x10000; i++) {
		int r = Random() % 20;
		Data += (char)(r == 0 ? 255 : r == 1 ? 250 : 90 + Random() % 150);
	}
	if (Random() % 4 != 0) {
		int Count = Random() % 5;
		Put(Data, Count, 4);
		for (int i = 0; i < Count; i++) {
			bool Deleted = Random() % 5 == 0;
			Put(Data, Deleted ? 0 : bx * 256 + Random() % 256, 4);
			Put(Data, Deleted ? 0 : by * 256 + Random() % 256, 4);
			Put(Data, Random() % 20, 4);
			char Text[40];
			int Length = Random() % 30;
			RandomText(Text, Length);
			Put(Data, Length, 2);
			Data.append(Text, Length);
		}
	}
	return Data;
}

static void WriteBoth(const std::string &Name, const std::string &Data)
{
	for (int s = 0; s < 2; s++) {
		FILE *File = fopen((Dir[s] + Name).c_str(), "wb");
		fwrite(Data.data(), 1, Data.size(), File);
		fclose(File);
	}
}

static bool ReadFileData(const std::string &Name, std::string &Data)
{
	FILE *File = fopen(Name.c_str(), "rb");
	if (File == NULL)
		return false;
	Data.clear();
	char Buffer[4096];
	size_t n;
	while ((n = fread(Buffer, 1, sizeof(Buffer), File)) > 0)
		Data.append(Buffer, n);
	fclose(File);
	return true;
}

static std::string MapFileName(int bx, int by, int bz)
{
	char Name[32];
	sprintf(Name, "%03d%03d%02d.map", bx, by, bz);
	return Name;
}

static void CompareFiles(const char *What)
{
	for (int bx : BlockX)
		for (int by : BlockX)
			for (int bz = LevelMin; bz <= LevelMax; bz++) {
				std::string A, B, Name = MapFileName(bx, by, bz);
				bool HaveA = ReadFileData(Dir[0] + Name, A), HaveB = ReadFileData(Dir[1] + Name, B);
				CHECK(HaveA && HaveB && A == B, "%s: file %s differs (%d/%d bytes)", What, Name.c_str(),
					(int)A.size(), (int)B.size());
			}
}

// --- Comparisons -------------------------------------------------------------------

static bool Same(const TObject &A, const TObject &B)
{
	return A.Type == B.Type && A.Data1 == B.Data1 && A.Data2 == B.Data2;
}

static void ComparePoint(int rx, int ry, int rz, const char *What)
{
	for (int p = 0; p < OBJECTS_PER_POINT; p++) {
		TObject A = GetMapObject(rx, ry, rz, p), B;
		At<ObjPosFn>(0x4b6580)(&B, rx, ry, rz, p);
		CHECK(Same(A, B), "%s: object %d,%d,%d/%d: %d/%d/%d, original %d/%d/%d", What, rx, ry, rz, p,
			A.Type, A.Data1, A.Data2, B.Type, B.Data1, B.Data2);
	}
	TObject A = GetFirstObject(rx, ry, rz), B;
	At<ObjAtFn>(0x4b6140)(&B, rx, ry, rz);
	for (int n = 0; n < 12; n++) {
		CHECK(Same(A, B), "%s: iteration %d at %d,%d,%d", What, n, rx, ry, rz);
		if (A.Type == 0 || B.Type == 0)
			break;
		A = GetNextObject();
		At<ObjNextFn>(0x4b6510)(&B);
	}
	int e1 = GetFirstEffect(rx, ry, rz), e2 = At<Int3RetFn>(0x4ba100)(rx, ry, rz);
	for (int n = 0; n < EFFECTS + 1; n++) {
		CHECK(e1 == e2, "%s: effect %d at %d,%d,%d: %d, original %d", What, n, rx, ry, rz, e1, e2);
		if (e1 == -1 || e2 == -1)
			break;
		e1 = GetNextEffect();
		e2 = At<IntFn>(0x4ba540)();
	}
	CHECK(FieldRefreshRequested(rx, ry, rz) == At<Bool3Fn>(0x4be750)(rx, ry, rz), "%s: refresh %d,%d,%d", What, rx, ry, rz);
	CHECK(GetFieldHeight(rx, ry, rz) == At<Int3RetFn>(0x4bdf30)(rx, ry, rz), "%s: height %d,%d,%d", What, rx, ry, rz);
	CHECK(IsFieldPassable(rx, ry, rz, false) == At<Bool4Fn>(0x4be130)(rx, ry, rz, false), "%s: passable %d,%d,%d", What, rx, ry, rz);
	CHECK(IsFieldPassable(rx, ry, rz, true) == At<Bool4Fn>(0x4be130)(rx, ry, rz, true), "%s: passable/height %d,%d,%d", What, rx, ry, rz);
	CHECK(IsFieldThrowable(rx, ry, rz) == At<Bool3Fn>(0x4be390)(rx, ry, rz), "%s: throwable %d,%d,%d", What, rx, ry, rz);
	CHECK(IsFieldTranslucent(rx, ry, rz) == At<Bool3Fn>(0x4be570)(rx, ry, rz), "%s: translucent %d,%d,%d", What, rx, ry, rz);
	struct { TObject (*Ours)(int, int, int, int &); uint32 Original; const char *Name; } Top[] = {
		{GetTopMoveObject, 0x4b6a00, "move"}, {GetTopLookObject, 0x4b6ee0, "look"},
		{GetTopUseObject, 0x4b73a0, "use"}, {GetTopMultiUseObject, 0x4b78d0, "multiuse"},
	};
	for (auto &T : Top) {
		int P1 = -7, P2 = -7;
		TObject O1 = T.Ours(rx, ry, rz, P1), O2;
		At<ObjRefFn>(T.Original)(&O2, rx, ry, rz, &P2);
		CHECK(Same(O1, O2) && P1 == P2, "%s: top %s object %d,%d,%d: %d at %d, original %d at %d", What, T.Name,
			rx, ry, rz, O1.Type, P1, O2.Type, P2);
	}
}

static void CompareEffects(const char *What)
{
	for (int i = 0; i < EFFECTS; i++) {
		int Status = GetEffectStatus(i);
		if (Status >= 0 && Status < 4)
			StatusSeen[Status]++;
		CHECK(Status == At<IntIntFn>(0x4b1440)(i), "%s: effect %d status", What, i);
		if (Status == 1) {
			CHECK(GetGraphicalEffectType(i) == At<IntIntFn>(0x4b15a0)(i), "%s: effect %d type", What, i);
			CHECK(GetGraphicalEffectPhase(i) == At<IntIntFn>(0x4b1860)(i), "%s: effect %d phase", What, i);
		} else if (Status == 2) {
			CHECK(GetTextualEffectColor(i) == At<IntIntFn>(0x4b1b20)(i), "%s: text %d colour", What, i);
			CHECK(strcmp(GetTextualEffectText(i), At<TextFn>(0x4b1de0)(i)) == 0, "%s: text %d", What, i);
			CHECK(GetTextualEffectPhase(i) == At<IntIntFn>(0x4b20a0)(i), "%s: text %d phase", What, i);
		} else if (Status == 3) {
			CHECK(GetMissileEffectType(i) == At<IntIntFn>(0x4b2360)(i), "%s: missile %d type", What, i);
			CHECK(GetMissileEffectDirection(i) == At<IntIntFn>(0x4b2620)(i), "%s: missile %d direction %d/%d", What,
				i, GetMissileEffectDirection(i), At<IntIntFn>(0x4b2620)(i));
			CHECK(GetMissileEffectX(i) == At<IntIntFn>(0x4b28e0)(i), "%s: missile %d x %d/%d", What, i,
				GetMissileEffectX(i), At<IntIntFn>(0x4b28e0)(i));
			CHECK(GetMissileEffectY(i) == At<IntIntFn>(0x4b2bb0)(i), "%s: missile %d y", What, i);
		}
	}
}

static void CompareState(const char *What)
{
	int A[3], B[3];
	GetMapPosition(&A[0], &A[1], &A[2]);
	At<Ptr3Fn>(0x4b08c0)(&B[0], &B[1], &B[2]);
	CHECK(memcmp(A, B, sizeof(A)) == 0, "%s: position", What);
	CHECK(GetMapLevel() == At<IntFn>(0x4b08f0)(), "%s: level", What);
	for (int rz = 0; rz < MAPSIZE_Z; rz++)
		for (int ry = 0; ry < MAPSIZE_Y; ry++)
			for (int rx = 0; rx < MAPSIZE_X; rx++)
				ComparePoint(rx, ry, rz, What);
	CompareEffects(What);
	CHECK(Removed[0] == Removed[1], "%s: removed creatures differ (%d/%d)", What, (int)Removed[0].size(),
		(int)Removed[1].size());
	TRGBColor C1 = GetAmbientColor(), C2;
	At<ColorFn>(0x4b0af0)(&C2);
	CHECK(C1.R == C2.R && C1.G == C2.G && C1.B == C2.B, "%s: ambient colour", What);
	CHECK(GetAutomapChanged() == At<BoolFn>(0x4b09d0)(), "%s: automap changed", What);
}

// --- Operations ----------------------------------------------------------------------

static int MaxObject, MaxEffect;

static TObject RandomObject()
{
	TObject Obj;
	if (Random() % 6 == 0) {
		Obj.Type = CREATURE;
		Obj.Data1 = 0x40000000 + Random() % 1000;
	} else {
		Obj.Type = 100 + Random() % (MaxObject - 99);
		Obj.Data1 = Random() % 256;
	}
	Obj.Data2 = Random() % 3 == 0 ? (int)(Random() % 40) : 0;
	return Obj;
}

static void RandomPosition()
{
	int x = 400 + Random() % 201, y = 400 + Random() % 201, z = LevelMin + Random() % (LevelMax - LevelMin + 1);
	SetMapPosition(x, y, z);
	At<Int3Fn>(0x4b08a0)(x, y, z);
}

static void TestCoordinates()
{
	for (int t = 0; t < 2000; t++) {
		int x = Random() % 2000 - 500, y = Random() % 2000 - 500, z = Random() % 20 - 2;
		int A[3] = {x, y, z}, B[3] = {x, y, z};
		if (t % 50 == 0) {
			int px = Random() % 1000, py = Random() % 1000, pz = Random() % 16;
			SetMapPosition(px, py, pz);
			At<Int3Fn>(0x4b08a0)(px, py, pz);
		}
		RelativeToAbsolute(&A[0], &A[1], &A[2]);
		At<Ptr3Fn>(0x4b0910)(&B[0], &B[1], &B[2]);
		CHECK(memcmp(A, B, sizeof(A)) == 0, "RelativeToAbsolute(%d,%d,%d)", x, y, z);
		A[0] = B[0] = x; A[1] = B[1] = y; A[2] = B[2] = z;
		AbsoluteToRelative(&A[0], &A[1], &A[2]);
		At<Ptr3Fn>(0x4b0960)(&B[0], &B[1], &B[2]);
		CHECK(memcmp(A, B, sizeof(A)) == 0, "AbsoluteToRelative(%d,%d,%d)", x, y, z);
		CHECK(GetMapLevel() == At<IntFn>(0x4b08f0)(), "GetMapLevel");
	}
}

static void TestLight()
{
	SetAmbientLight(-1, 0);
	At<Int2Fn>(0x4b09e0)(-1, 0);
	for (int t = 0; t < 400; t++) {
		int Brightness = Random() % 256, Color = Random() % 216;
		SetAmbientLight(Brightness, Color);
		At<Int2Fn>(0x4b09e0)(Brightness, Color);
		int Steps = Random() % 300;
		for (int i = 0; i < Steps; i++) {
			UpdateAmbientLight();
			At<VoidFn>(0x4b0a70)();
		}
		CHECK(GetAmbientBrightness() == At<IntFn>(0x4b5610)(), "ambient brightness");
		TRGBColor C1 = GetAmbientColor(), C2;
		At<ColorFn>(0x4b0af0)(&C2);
		CHECK(C1.R == C2.R && C1.G == C2.G && C1.B == C2.B, "ambient colour %d,%d,%d / %d,%d,%d", C1.R, C1.G, C1.B,
			C2.R, C2.G, C2.B);
		if (t % 40 == 0) {
			SetAmbientLight(-1, Color);
			At<Int2Fn>(0x4b09e0)(-1, Color);
		}
	}
}

// One random operation on both sides; the choices only use our state, which
// equals the original's as long as nothing has failed.
static void RandomOperation()
{
	int rx = Random() % MAPSIZE_X, ry = Random() % MAPSIZE_Y, rz = Random() % MAPSIZE_Z;
	int Count = 0;
	while (Count < OBJECTS_PER_POINT && GetMapObject(rx, ry, rz, Count).Type != 0)
		Count++;
	switch (Random() % 22) {
	case 0: case 1: case 2: case 3: {
		TObject Obj = RandomObject();
		AddObject(rx, ry, rz, Obj);
		At<PutFn>(0x4b88f0)(rx, ry, rz, Obj);
		break;
	}
	case 4: case 5:
		if (Count < OBJECTS_PER_POINT) {
			TObject Obj = RandomObject();
			AppendObject(rx, ry, rz, Obj);
			At<PutFn>(0x4b8ec0)(rx, ry, rz, Obj);
		}
		break;
	case 6: case 7:
		if (Count > 0) {
			int p = Random() % Count;
			bool A = DeleteObject(rx, ry, rz, p), B = At<DeleteFn>(0x4b9420)(rx, ry, rz, p);
			CHECK(A == B, "DeleteObject result");
		}
		break;
	case 8:
		if (Count > 0) {
			int p = Random() % Count;
			TObject Obj = RandomObject();
			ChangeObject(rx, ry, rz, p, Obj);
			At<PutAtFn>(0x4b9940)(rx, ry, rz, p, Obj);
		}
		break;
	case 9: {
		bool Effects = Random() % 2 == 0;
		ClearField(rx, ry, rz, Effects);
		At<Clear4Fn>(0x4bc720)(rx, ry, rz, Effects);
		break;
	}
	case 10:
		RequestFieldRefresh(rx, ry, rz);
		At<Int3Fn>(0x4bdd70)(rx, ry, rz);
		break;
	case 11: case 12: {
		int Type = 1 + Random() % MaxEffect;
		int x = Random() % 4 == 0 ? Random() % 30 - 6 : rx, y = Random() % 4 == 0 ? Random() % 26 - 6 : ry;
		int z = Random() % 4 == 0 ? Random() % 12 - 2 : rz;
		AddGraphicalEffect(x, y, z, Type);
		At<Effect4Fn>(0x4bab40)(x, y, z, Type);
		break;
	}
	case 13: {
		char Text[12];
		RandomText(Text, Random() % 10);
		int Color = Random() % 216;
		AddTextualEffect(rx, ry, rz, Color, Text);
		At<TextEffectFn>(0x4bad00)(rx, ry, rz, Color, Text);
		break;
	}
	case 14: case 15: {
		int Level = GetMapLevel();
		int sz = Random() % 4 == 0 ? Random() % 8 : Level, dz = Random() % 4 == 0 ? Random() % 8 : Level;
		int dx = Random() % 4 == 0 ? Random() % 30 - 6 : rx + Random() % 9 - 4;
		int dy = Random() % 4 == 0 ? Random() % 26 - 6 : ry + Random() % 9 - 4;
		int Type = 1 + Random() % 40;
		AddMissileEffect(rx, ry, sz, dx, dy, dz, Type);
		At<MissileFn>(0x4bb0c0)(rx, ry, sz, dx, dy, dz, Type);
		break;
	}
	case 16: case 17:
		SetTestClock(Random() % 2 == 0 ? 100000 + Random() % 3000 : 100000);
		ProcessEffects();
		At<VoidFn>(0x4bba70)();
		ProcessMissileEffects();
		At<VoidFn>(0x4bbd40)();
		break;
	case 18: {
		int dx = Random() % 3 - 1, dy = Random() % 3 - 1;
		ShiftMap(dx, dy);
		At<Int2Fn>(0x4bd560)(dx, dy);
		break;
	}
	case 19: {
		int dz = Random() % 5 - 2;
		ShiftMapLevel(dz);
		At<Int1Fn>(0x4bdb70)(dz);
		break;
	}
	case 20:
		if (Random() % 10 == 0) {
			ClearMap();
			At<VoidFn>(0x4bccb0)();
		}
		break;
	case 21:
		RandomPosition();
		break;
	}
}

// --- Automap -----------------------------------------------------------------------------

static void TestAutomap()
{
	RandomPosition();
	for (int t = 0; t < 3000; t++) {
		int Op = Random() % 8;
		switch (Op) {
		case 0: case 1: {
			int rx = Random() % MAPSIZE_X, ry = Random() % MAPSIZE_Y;
			Side(0); UpdateAutomap(rx, ry);
			Side(1); At<Int2Fn>(0x4beb90)(rx, ry);
			break;
		}
		case 2: case 3: {
			int x = Random() % 8 == 0 ? -(int)(Random() % 100) - 1 : 256 + Random() % 512;
			int y = Random() % 8 == 0 ? -(int)(Random() % 100) - 1 : 256 + Random() % 512;
			int z = LevelMin + Random() % (LevelMax - LevelMin + 1);
			Side(0); TRGBColor A = GetAutomapColor(x, y, z);
			Side(1); TRGBColor B; At<ColorAtFn>(0x4b4370)(&B, x, y, z);
			CHECK(A.R == B.R && A.G == B.G && A.B == B.B, "GetAutomapColor(%d,%d,%d)", x, y, z);
			break;
		}
		case 4: {
			int bx = BlockX[Random() % 2], by = BlockX[Random() % 2], bz = LevelMin + Random() % (LevelMax - LevelMin + 1);
			vector<TMark> M1(0, 0, 10), M2(0, 0, 10);
			int C1 = -5, C2 = -5;
			Side(0); GetAutomapMarks(bx, by, bz, &M1, &C1);
			Side(1); At<MarksFn>(0x4b4680)(bx, by, bz, &M2, &C2);
			CHECK(C1 == C2, "GetAutomapMarks count %d/%d", C1, C2);
			for (int i = 0; i < C1 && C1 == C2; i++)
				CHECK(memcmp(&M1(i), &M2(i), sizeof(TMark)) == 0, "mark %d of %d,%d,%d", i, bx, by, bz);
			break;
		}
		case 5: {
			TCoordinate Where = {256 + (int)(Random() % 512), 256 + (int)(Random() % 512),
				LevelMin + (int)(Random() % (LevelMax - LevelMin + 1))};
			char Text[40];
			RandomText(Text, Random() % 30);
			int Type = Random() % 20;
			Side(0); AddAutomapMark(Where, Text, Type);
			Side(1); At<AddMarkFn>(0x4b4f60)(&Where, Text, Type);
			break;
		}
		case 6: {
			int bx = BlockX[Random() % 2], by = BlockX[Random() % 2], bz = LevelMin + Random() % (LevelMax - LevelMin + 1);
			vector<TMark> M(0, 0, 10), M2(0, 0, 10);
			int Count = 0, Count2 = 0;
			Side(0); GetAutomapMarks(bx, by, bz, &M, &Count);
			Side(1); At<MarksFn>(0x4b4680)(bx, by, bz, &M2, &Count2);
			if (Count == 0)
				break;
			TMark &Mark = M(Random() % Count);
			TCoordinate Where = {Mark.x, Mark.y, 0};
			// Deleted marks are at 0,0, whose block has no file here.
			if (Random() % 4 == 0 || (Mark.x == 0 && Mark.y == 0)) {
				Where.x = bx * 256 + Random() % 256;
				Where.y = by * 256 + Random() % 256;
			}
			Side(0); DeleteAutomapMark(Where, bz);
			Side(1); At<DeleteMarkFn>(0x4b5410)(&Where, bz);
			break;
		}
		case 7: {
			int sx = 400 + Random() % 201, sy = 400 + Random() % 201, sz = LevelMin + Random() % (LevelMax - LevelMin + 1);
			int dx = sx + Random() % 25 - 12, dy = sy + Random() % 25 - 12, dz = sz;
			int r = Random() % 20;
			if (r == 0)
				dx = sx + 111 + Random() % 5;
			else if (r == 1)
				dz = sz + (Random() % 2 == 0 ? 1 : -1);
			else if (r == 2)
				dx = sx, dy = sy + Random() % 3 - 1;
			bool MustReach = Random() % 2 == 0, ForceDiagonal = Random() % 2 == 0;
			int S1 = -9, S2 = -9, D1[12], D2[12];
			for (int i = 0; i < 12; i++)
				D1[i] = D2[i] = -1;
			Side(0); int R1 = FindPath(sx, sy, sz, dx, dy, dz, MustReach, &S1, D1, ForceDiagonal);
			Side(1); int R2 = At<PathFn>(0x4bee00)(sx, sy, sz, dx, dy, dz, MustReach, &S2, D2, ForceDiagonal);
			if (R1 >= -4 && R1 <= 2)
				PathResults[R1 + 4]++;
			CHECK(R1 == R2 && S1 == S2 && memcmp(D1, D2, sizeof(D1)) == 0,
				"FindPath(%d,%d,%d -> %d,%d,%d): %d/%d steps, original %d/%d steps", sx, sy, sz, dx, dy, dz, R1, S1,
				R2, S2);
			break;
		}
		}
		if (t % 500 == 499)
			RandomPosition();
	}
}

// --- Entry ------------------------------------------------------------------------------

void TestMap(const char *ExePath)
{
	int Before = Checks, BeforeFailures = Failures;
	PatchScript();
	TTalkBuffer *SavedBuffer = DefaultTextBuffer;
	TTalkBuffer *SavedImageBuffer = Global<TTalkBuffer *>(0x71ef68);
	DefaultTextBuffer = NULL;
	Global<TTalkBuffer *>(0x71ef68) = NULL;

	std::string Dat = ExePath;
	size_t Slash = Dat.find_last_of("\\/");
	Dat = (Slash == std::string::npos ? std::string() : Dat.substr(0, Slash + 1)) + "Tibia.dat";
	FILE *Probe = fopen(Dat.c_str(), "rb");
	if (Probe == NULL) {
		printf("note: %s not found, Map.cpp not tested\n", Dat.c_str());
		return;
	}
	fclose(Probe);
	LoadObjects(Dat.c_str());
	At<LoadFn>(0x4d7380)(Dat.c_str());
	MaxObject = ObjectProperties->high();
	MaxEffect = EffectProperties->high();

	// The image's static objects of Map.cpp.
	At<Ctor2Fn>(0x4b0bc0)((void *)(uintptr_t)0x5d4c18, NULL, 0, 0x7df);
	At<Ctor6Fn>(0x4b0d70)((void *)(uintptr_t)0x5d4c24, NULL, 0, 17, 0, 13, 0, 7);
	for (int i = 0; i < 10; i++)
		At<Ctor0Fn>(0x4b1410)((void *)(uintptr_t)(0x5d4c40 + i * 0x200a8), NULL);

	char Saved[MAX_PATH];
	_getcwd(Saved, MAX_PATH);
	char Help[MAX_PATH];
	GetTempPathA(MAX_PATH, Help);
	const char *Names[2] = {"tibia-maptest-ours\\", "tibia-maptest-original\\"};
	for (int s = 0; s < 2; s++) {
		Dir[s] = std::string(Help) + Names[s];
		CreateDirectoryA(Dir[s].c_str(), NULL);
	}
	for (int bx : BlockX)
		for (int by : BlockX)
			for (int bz = LevelMin; bz <= LevelMax; bz++)
				WriteBoth(MapFileName(bx, by, bz), RandomMapFile(bx, by));

	InitMap(OurCallback);
	At<InitFn>(0x4bf620)(OriginalCallback);
	SetTestClock(100000);
	CompareState("InitMap");
	TestCoordinates();
	TestLight();
	RandomPosition();
	for (int Round = 0; Round < 12; Round++) {
		for (int t = 0; t < 600; t++)
			RandomOperation();
		CompareState("operations");
	}
	TestAutomap();
	CompareState("automap");
	Side(0); ExitMap();
	Side(1); At<VoidFn>(0x4b5760)();
	CompareFiles("ExitMap");

	// Again from the saved files: InitMap empties the block cache.
	InitMap(OurCallback);
	At<InitFn>(0x4bf620)(OriginalCallback);
	RandomPosition();
	TestAutomap();
	Side(0); ExitMap();
	Side(1); At<VoidFn>(0x4b5760)();
	CompareFiles("ExitMap after reloading");

	_chdir(Saved);
	for (int s = 0; s < 2; s++) {
		for (int bx : BlockX)
			for (int by : BlockX)
				for (int bz = LevelMin; bz <= LevelMax; bz++)
					remove((Dir[s] + MapFileName(bx, by, bz)).c_str());
		RemoveDirectoryA(Dir[s].c_str());
	}
	ExitObjects();
	At<VoidFn>(0x4d79b0)();
	DefaultTextBuffer = SavedBuffer;
	Global<TTalkBuffer *>(0x71ef68) = SavedImageBuffer;
	printf("Map: %d checks, %d failures (paths -4..2: %d %d %d %d %d %d %d; effects seen: %d graphical, %d texts, %d missiles)\n",
		Checks - Before, Failures - BeforeFailures, PathResults[0], PathResults[1], PathResults[2], PathResults[3],
		PathResults[4], PathResults[5], PathResults[6], StatusSeen[1], StatusSeen[2], StatusSeen[3]);
}
