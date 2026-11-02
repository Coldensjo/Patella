// Differential test of Container.h's templates (issue #13): every instance of
// vector<T>, array<T>, matrix<T> and matrix3d<T> that the image has is called
// next to ours on the same inputs, and both objects and all their entries are
// compared byte for byte after every call. The image's and our operator new[]
// fill new memory with the same random byte pattern, so entries that neither
// side initialises agree too. Also the other header functions that have a
// path the image can run (TLineFormat's constructor, TBuffer::SetSize,
// TChannelTalkBuffer::GetChannelID).
//
// Not run: the error paths (the assertions of the constructors throw, the
// index errors of array, matrix and matrix3d build a std::stringstream; the
// image's C++ runtime cannot do either in the test child), and GUI.h's
// GetFirstChild/GetLastChild and TRuleViolationChannel::AddText, which only
// throw.
//
// DIFFTEST_ONLY=Container runs this test alone.

#include "difftest.h"
#include "../../src/Container.h"
#include "../../src/ConsoleWindow.h"
#include "../../src/Dialogs.h"
#include "../../src/DialogElements.h"
#include "../../src/GUI.h"
#include "../../src/Map.h"
#include "../../src/Objects.h"
#include "../../src/Preferences.h"
#include "../../src/Status.h"
#include "../../src/Talk.h"
#include "../../src/Utils.h"
#include <algorithm>
#include <new>
#include <stddef.h>
#include <string.h>
#include <vector>

namespace ContainerTest {

typedef void (__fastcall *Ctor2Fn)(void *This, void *, int a, int b);
typedef void (__fastcall *Ctor3Fn)(void *This, void *, int Min, int Max, int Block);
typedef void (__fastcall *Ctor4Fn)(void *This, void *, int a, int b, int c, int d);
typedef void (__fastcall *Ctor6Fn)(void *This, void *, int a, int b, int c, int d, int e, int f);
typedef void *(__fastcall *At1Fn)(void *This, void *, int i);
typedef void *(__fastcall *At2Fn)(void *This, void *, int x, int y);
typedef void *(__fastcall *At3Fn)(void *This, void *, int x, int y, int z);
typedef void (__fastcall *DtorFn)(void *This, void *);

// Its own random numbers: the shared Random() stays where it was, so the
// tests after this one get the same inputs as without it.
static uint32 Seed = 13;
static uint32 Rnd()
{
	Seed = Seed * 1103515245 + 12345;
	return Seed >> 8;
}

// New memory of both sides gets the same byte pattern.
static uint8 Pattern = 0xab;
static void *__cdecl PatternNew(size_t Size)
{
	void *Memory = malloc(Size == 0 ? 1 : Size);
	memset(Memory, Pattern, Size);
	return Memory;
}

struct TSaved {
	uint8 *Code;
	uint8 Bytes[5];
};
static std::vector<TSaved> SavedCode;
static void Redirect(void *Function, void *Target)
{
	TSaved S;
	S.Code = (uint8 *)Function;
	DWORD Protection;
	VirtualProtect(Function, 5, PAGE_EXECUTE_READWRITE, &Protection);
	memcpy(S.Bytes, Function, 5);
	PatchJump((uint32)(uintptr_t)Function, Target);
	VirtualProtect(Function, 5, Protection, &Protection);
	FlushInstructionCache(GetCurrentProcess(), Function, 5);
	SavedCode.push_back(S);
}
static void RestoreAll()
{
	for (size_t i = SavedCode.size(); i-- > 0;) {
		DWORD Protection;
		VirtualProtect(SavedCode[i].Code, 5, PAGE_EXECUTE_READWRITE, &Protection);
		memcpy(SavedCode[i].Code, SavedCode[i].Bytes, 5);
		VirtualProtect(SavedCode[i].Code, 5, Protection, &Protection);
		FlushInstructionCache(GetCurrentProcess(), SavedCode[i].Code, 5);
	}
	SavedCode.clear();
}

static void RandomBytes(void *Data, size_t Size)
{
	for (size_t i = 0; i < Size; i++)
		((uint8 *)Data)[i] = (uint8)Rnd();
}

// --- vector<T> ---------------------------------------------------------------------

// The image's copies of one vector<T> instance. Init is the 4-byte initial
// value of the initialized constructors (Ctor4).
struct TVectorInstance {
	const char *Name;
	uint32 Ctor3;  // vector(min, max, block), 0 if the image has none
	uint32 Ctor4;  // vector(min, max, block, init), 0 if the image has none
	uint32 At;     // operator()
	uint32 Dtor;   // out-of-line ~vector, 0 if inline only
};

// Everything but the entry pointer, then the whole allocated array.
template <class T> static bool SameVector(vector<T> *Ours, vector<T> *Image, const char *Name, const char *What)
{
	size_t Head = offsetof(vector<T>, entry);
	bool Same = memcmp(Ours, Image, Head) == 0;
	CHECK(Same, "%s %s: min %d/%d max %d/%d start %d/%d space %d/%d block %d/%d initialized %d/%d", Name, What,
		Ours->min, Image->min, Ours->max, Image->max, Ours->start, Image->start, Ours->space, Image->space,
		Ours->block, Image->block, Ours->initialized, Image->initialized);
	if (!Same)
		return false;
	Same = memcmp(Ours->entry, Image->entry, Ours->space * sizeof(T)) == 0;
	CHECK(Same, "%s %s: entries differ (space %d)", Name, What, Ours->space);
	return Same;
}

template <class T> static void TestVector(const TVectorInstance &Instance, int Rounds)
{
	CHECK(offsetof(vector<T>, entry) + sizeof(T *) == sizeof(vector<T>), "%s: entry is not the last member", Instance.Name);
	for (int Round = 0; Round < Rounds; Round++) {
		Pattern = (uint8)Rnd();
		int Min = (int)(Rnd() % 11) - 5;
		int Max = Min + (int)(Rnd() % 6);
		int Block = (int)(Rnd() % 5);
		bool WithInit = Instance.Ctor4 != 0 && (Instance.Ctor3 == 0 || Rnd() % 2 == 0);
		uint32 Init = Rnd() % 4 == 0 ? 0 : Rnd();
		vector<T> *Ours = (vector<T> *)PatternNew(sizeof(vector<T>));
		vector<T> *Image = (vector<T> *)PatternNew(sizeof(vector<T>));
		// The entries get another pattern than the object, so that an entry
		// set from init (whose bytes the constructor leaves) differs from one
		// that is not.
		Pattern = (uint8)(Pattern + 1 + Rnd() % 255);
		if constexpr (sizeof(T) == 4) {
			// The initialized vectors have 4-byte elements (int and pointers).
			if (WithInit) {
				T Value;
				memcpy(&Value, &Init, sizeof(T));
				new (Ours) vector<T>(Min, Max, Block, Value);
				At<Ctor4Fn>(Instance.Ctor4)(Image, NULL, Min, Max, Block, (int)Init);
			}
		}
		if (!WithInit) {
			new (Ours) vector<T>(Min, Max, Block);
			At<Ctor3Fn>(Instance.Ctor3)(Image, NULL, Min, Max, Block);
		}
		bool Same = SameVector(Ours, Image, Instance.Name, WithInit ? "vector(min, max, block, init)" : "vector(min, max, block)");
		// Indices inside, above and below the used range; the arrays stay below 64 KB.
		int Operations = 1 + (int)(Rnd() % 40);
		for (int Op = 0; Same && Op < Operations; Op++) {
			int Kind = (int)(Rnd() % 10);
			int Spread = 2 * Ours->space + 3;
			int i;
			if (Kind < 4)
				i = Ours->min + (int)(Rnd() % (uint32)(Ours->max - Ours->min + 1));
			else if (Kind < 7)
				i = Ours->max + 1 + (int)(Rnd() % (uint32)Spread);
			else
				i = Ours->min - 1 - (int)(Rnd() % (uint32)Spread);
			int Low = (std::min)(i, Ours->min), High = (std::max)(i, Ours->max);
			if ((High - Low + 1) * 4 * (int)sizeof(T) > 65536)
				continue;
			T *a = &(*Ours)(i);
			T *b = (T *)At<At1Fn>(Instance.At)(Image, NULL, i);
			CHECK(a - Ours->entry == b - Image->entry, "%s operator()(%d): entry %d, original %d", Instance.Name, i,
				(int)(a - Ours->entry), (int)(b - Image->entry));
			if (Rnd() % 3 != 0) {
				uint8 Value[sizeof(T)];
				RandomBytes(Value, sizeof(T));
				memcpy(a, Value, sizeof(T));
				memcpy(b, Value, sizeof(T));
			}
			Same = SameVector(Ours, Image, Instance.Name, "operator()");
		}
		// The destructor: ours inline, the image's out-of-line copy where it has one.
		Ours->~vector<T>();
		if (Instance.Dtor != 0)
			At<DtorFn>(Instance.Dtor)(Image, NULL);
		else
			free(Image->entry);
		free(Ours);
		free(Image);
	}
}

// --- array<T>, matrix<T>, matrix3d<T> ----------------------------------------------

template <class T> static void TestArray(const char *Name, uint32 Ctor, uint32 AtFunction, int MaxEntries, int Rounds)
{
	for (int Round = 0; Round < Rounds; Round++) {
		Pattern = (uint8)Rnd();
		int Min = (int)(Rnd() % 201) - 100;
		int Max = Min + (int)(Rnd() % (uint32)MaxEntries);
		array<T> *Ours = (array<T> *)PatternNew(sizeof(array<T>));
		array<T> *Image = (array<T> *)PatternNew(sizeof(array<T>));
		new (Ours) array<T>(Min, Max);
		At<Ctor2Fn>(Ctor)(Image, NULL, Min, Max);
		bool Same = Ours->min == Image->min && Ours->d == Image->d;
		CHECK(Same, "%s array(%d, %d): min %d/%d d %d/%d", Name, Min, Max, Ours->min, Image->min, Ours->d, Image->d);
		if (Same) {
			Same = memcmp(Ours->entry, Image->entry, Ours->d * sizeof(T)) == 0;
			CHECK(Same, "%s array(%d, %d): entries differ", Name, Min, Max);
		}
		CHECK(Ours->low() == Min && Ours->high() == Max, "%s low/high %d %d", Name, Ours->low(), Ours->high());
		for (int Op = 0; Same && Op < 30; Op++) {
			int i = Min + (int)(Rnd() % (uint32)(Max - Min + 1));
			T *a = &(*Ours)(i);
			T *b = (T *)At<At1Fn>(AtFunction)(Image, NULL, i);
			CHECK(a - Ours->entry == b - Image->entry, "%s operator()(%d)", Name, i);
		}
		Ours->~array<T>();
		free(Image->entry);
		free(Ours);
		free(Image);
	}
}

template <class T> static void TestMatrix(const char *Name, uint32 Ctor, uint32 AtFunction, uint32 Dtor, int MaxSide,
	int Rounds)
{
	for (int Round = 0; Round < Rounds; Round++) {
		Pattern = (uint8)Rnd();
		int XMin = (int)(Rnd() % 201) - 100, YMin = (int)(Rnd() % 201) - 100;
		int XMax = XMin + (int)(Rnd() % (uint32)MaxSide), YMax = YMin + (int)(Rnd() % (uint32)MaxSide);
		matrix<T> *Ours = (matrix<T> *)PatternNew(sizeof(matrix<T>));
		matrix<T> *Image = (matrix<T> *)PatternNew(sizeof(matrix<T>));
		new (Ours) matrix<T>(XMin, XMax, YMin, YMax);
		At<Ctor4Fn>(Ctor)(Image, NULL, XMin, XMax, YMin, YMax);
		size_t Head = offsetof(matrix<T>, entry);
		bool Same = memcmp(Ours, Image, Head) == 0;
		CHECK(Same, "%s matrix(%d, %d, %d, %d): header differs", Name, XMin, XMax, YMin, YMax);
		if (Same) {
			Same = memcmp(Ours->entry, Image->entry, Ours->dx * Ours->dy * sizeof(T)) == 0;
			CHECK(Same, "%s matrix(%d, %d, %d, %d): entries differ", Name, XMin, XMax, YMin, YMax);
		}
		for (int Op = 0; Same && Op < 30; Op++) {
			int x = XMin + (int)(Rnd() % (uint32)(XMax - XMin + 1));
			int y = YMin + (int)(Rnd() % (uint32)(YMax - YMin + 1));
			T *a = &(*Ours)(x, y);
			T *b = (T *)At<At2Fn>(AtFunction)(Image, NULL, x, y);
			CHECK(a - Ours->entry == b - Image->entry, "%s operator()(%d, %d)", Name, x, y);
		}
		Ours->~matrix<T>();
		At<DtorFn>(Dtor)(Image, NULL);
		free(Ours);
		free(Image);
	}
}

template <class T> static void TestMatrix3d(const char *Name, uint32 Ctor, uint32 AtFunction, int MaxSide, int Rounds)
{
	for (int Round = 0; Round < Rounds; Round++) {
		Pattern = (uint8)Rnd();
		int Low[3], High[3];
		for (int k = 0; k < 3; k++) {
			Low[k] = (int)(Rnd() % 41) - 20;
			High[k] = Low[k] + (int)(Rnd() % (uint32)MaxSide);
		}
		matrix3d<T> *Ours = (matrix3d<T> *)PatternNew(sizeof(matrix3d<T>));
		matrix3d<T> *Image = (matrix3d<T> *)PatternNew(sizeof(matrix3d<T>));
		new (Ours) matrix3d<T>(Low[0], High[0], Low[1], High[1], Low[2], High[2]);
		At<Ctor6Fn>(Ctor)(Image, NULL, Low[0], High[0], Low[1], High[1], Low[2], High[2]);
		size_t Head = offsetof(matrix3d<T>, entry);
		bool Same = memcmp(Ours, Image, Head) == 0;
		CHECK(Same, "%s matrix3d: header differs", Name);
		if (Same) {
			Same = memcmp(Ours->entry, Image->entry, Ours->dx * Ours->dy * Ours->dz * sizeof(T)) == 0;
			CHECK(Same, "%s matrix3d: entries differ", Name);
		}
		for (int Op = 0; Same && Op < 30; Op++) {
			int x = Low[0] + (int)(Rnd() % (uint32)(High[0] - Low[0] + 1));
			int y = Low[1] + (int)(Rnd() % (uint32)(High[1] - Low[1] + 1));
			int z = Low[2] + (int)(Rnd() % (uint32)(High[2] - Low[2] + 1));
			T *a = &(*Ours)(x, y, z);
			T *b = (T *)At<At3Fn>(AtFunction)(Image, NULL, x, y, z);
			CHECK(a - Ours->entry == b - Image->entry, "%s operator()(%d, %d, %d)", Name, x, y, z);
		}
		Ours->~matrix3d<T>();
		free(Image->entry);
		free(Ours);
		free(Image);
	}
}

// --- The other header functions ------------------------------------------------------

typedef void (__fastcall *LineFormatFn)(void *This, void *, TTalkLine *TextLine, const char *LineText, int LineLength,
	int OriginalOffset, int Indent, int FontNumber, TRGBColor Color, bool LineEnd);
typedef void (__fastcall *SetSizeFn)(void *This, void *, int NewSize);
typedef int (__fastcall *GetChannelIDFn)(void *This, void *);

static void TestHeaderInlines()
{
	// ConsoleWindow.h's TLineFormat constructor 0x418020 (valid arguments only).
	for (int Round = 0; Round < 200; Round++) {
		Pattern = (uint8)Rnd();
		TTalkLine *Line = (TTalkLine *)(uintptr_t)(1 + Rnd());
		const char *Text = (const char *)(uintptr_t)(1 + Rnd());
		int Length = 1 + (int)(Rnd() % 100), Offset = (int)(Rnd() % 100), Indent = (int)(Rnd() % 50);
		int Font = (int)(Rnd() % 8);
		TRGBColor Color((int)(Rnd() % 256), (int)(Rnd() % 256), (int)(Rnd() % 256));
		bool End = Rnd() % 2 == 0;
		TLineFormat *Ours = (TLineFormat *)PatternNew(sizeof(TLineFormat));
		TLineFormat *Image = (TLineFormat *)PatternNew(sizeof(TLineFormat));
		new (Ours) TLineFormat(Line, Text, Length, Offset, Indent, Font, Color, End);
		At<LineFormatFn>(0x418020)(Image, NULL, Line, Text, Length, Offset, Indent, Font, Color, End);
		CHECK(memcmp(Ours, Image, sizeof(TLineFormat)) == 0, "TLineFormat(%d, %d, %d, %d)", Length, Offset, Indent, Font);
		free(Ours);
		free(Image);
	}
	// Utils.h's TBuffer::SetSize 0x4ca300 (sizes >= 0 only).
	for (int Round = 0; Round < 200; Round++) {
		TBuffer Ours, Image;
		RandomBytes(&Ours, sizeof(Ours));
		Image = Ours;
		int Size = Rnd() % 4 == 0 ? 0 : (int)(Rnd() & 0x7fffffff);
		Ours.SetSize(Size);
		At<SetSizeFn>(0x4ca300)(&Image, NULL, Size);
		CHECK(memcmp(&Ours, &Image, sizeof(TBuffer)) == 0, "TBuffer::SetSize(%d)", Size);
	}
	// Talk.h's TChannelTalkBuffer::GetChannelID 0x507060 (folded with
	// DialogElements.h's TListBox::GetNumberOfLines) on our objects.
	for (int Round = 0; Round < 50; Round++) {
		int Channel = (int)Rnd();
		TChannelTalkBuffer Buffer(0, Channel);
		CHECK(At<GetChannelIDFn>(0x507060)(&Buffer, NULL) == Buffer.GetChannelID(), "GetChannelID %d", Channel);
	}
}

static void Run()
{
	PatchImage();
	// operator new[] of both sides (and the image's operator new) fill with the pattern.
	Redirect((void *)(uintptr_t)0x523817, (void *)PatternNew);
	Redirect((void *)(uintptr_t)0x5237e7, (void *)PatternNew);
	Redirect((void *)(void *(__cdecl *)(size_t))operator new[], (void *)PatternNew);
	int Before = Checks;

	// One entry per instance in the image (Container.h's list).
	TestVector<TLineFormat>({"vector<TLineFormat>", 0x417aa0, 0, 0x417710, 0}, 300);
	TestVector<TSkinButton *>({"vector<TSkinButton *>", 0, 0x425e70, 0x452ac0, 0x452c50}, 300);
	TestVector<TMark>({"vector<TMark>", 0x426170, 0, 0x425b20, 0x425b10}, 200);
	TestVector<int>({"vector<int>", 0x4a2b90, 0x453300, 0x452ac0, 0x452c50}, 600);
	TestVector<TListBoxLine>({"vector<TListBoxLine>", 0x453600, 0, 0x452eb0, 0x452c60}, 300);
	TestVector<TIgnoreEntry>({"vector<TIgnoreEntry>", 0x46c0d0, 0, 0x46bb70, 0x46bb60}, 300);
	TestVector<THotkey>({"vector<THotkey>", 0x4976f0, 0, 0x496570, 0}, 300);
	TestVector<TTimer *>({"vector<TTimer *>", 0, 0x4973f0, 0x452ac0, 0}, 300);
	TestVector<TContentWindowSize>({"vector<TContentWindowSize>", 0x4dbe90, 0, 0x4db880, 0}, 300);
	TestVector<TBuddy>({"vector<TBuddy>", 0x4dc170, 0, 0x4dba60, 0}, 200);
	TestVector<TMessage>({"vector<TMessage>", 0x4e7f50, 0, 0x4e7390, 0}, 300);
	TestVector<TErrorReport>({"vector<TErrorReport>", 0x4e8230, 0, 0x4e7860, 0}, 60);
	TestVector<TTalkFilter *>({"vector<TTalkFilter *>", 0, 0x4fd6e0, 0x452ac0, 0}, 300);

	TestArray<TMapPoint>("array<TMapPoint>", 0x4b0bc0, 0x4b5900, 50, 100);
	TestArray<TProperties>("array<TProperties>", 0x4cfa30, 0x4d1a10, 50, 100);
	TestMatrix<TPathNode>("matrix<TPathNode>", 0x4b1100, 0x4b5e90, 0x4b0b20, 30, 100);
	TestMatrix3d<int>("matrix3d<int>", 0x4b0d70, 0x4b5b40, 12, 100);

	TestHeaderInlines();
	RestoreAll();
	printf("Container: %d checks\n", Checks - Before);
}

} // namespace ContainerTest

void TestContainer()
{
	ContainerTest::Run();
}
