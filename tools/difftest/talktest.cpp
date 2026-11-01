// Differential tests of Talk.cpp and its Talk.h/Container.h COMDATs.
// Only mapped functions run: no imports, entry point or original client loader.
#include "difftest.h"
#include "../../src/Talk.h"
#include "../../src/Utils.h"
#include "../../src/external.h"
#include <new>
#include <stdarg.h>
#include <vector>

typedef void (__cdecl *VoidFn)();
typedef bool (__cdecl *BoolFn)();
typedef int (__cdecl *IntFn)();
typedef void (__cdecl *SetBoolFn)(bool);
typedef void (__fastcall *ThisFn)(void *, void *);
typedef void (__fastcall *ThisIntFn)(void *, void *, int);
typedef void (__fastcall *ThisLineFn)(void *, void *, TTalkLine *);
typedef void (__fastcall *ThisTextFn)(void *, void *, int, const char *, int);
typedef bool (__fastcall *ThisBoolFn)(void *, void *);
typedef bool (__fastcall *ThisSetBoolFn)(void *, void *, bool);
typedef TTalkLine *(__fastcall *GetLineFn)(void *, void *, int);
typedef TTalkLine *(__fastcall *NextFn)(void *, void *);
typedef void (__fastcall *LineFn)(void *, void *, uint32, int, const char *, int);
typedef void (__fastcall *NamedLineFn)(void *, void *, uint32, int, const char *, const char *, int);
typedef void (__fastcall *VectorFn)(void *, void *, int, int, int, TTalkFilter *);
typedef TTalkBuffer *(__cdecl *CreateFn)(int, int, const char *, int);
typedef void (__cdecl *DeleteFn)(TTalkBuffer *);
typedef bool (__fastcall *MatchFn)(void *, void *, int, const char *, int);

static __time32_t FixedTime = 1700000000;
static uint8 SavedClock[5];
static void *TestClockFunction;
static __time32_t __cdecl ImageTime(__time32_t *Time)
{
	if (Time != NULL)
		*Time = FixedTime;
	return FixedTime;
}
static tm *__cdecl ImageLocaltime(const __time32_t *Time) { return _localtime32(Time); }
static int __cdecl ImageSprintf(char *Buffer, const char *Format, ...)
{
	va_list Args;
	va_start(Args, Format);
	int Result = vsprintf(Buffer, Format, Args);
	va_end(Args);
	return Result;
}
static int __cdecl ImageFprintf(FILE *File, const char *Format, ...)
{
	va_list Args;
	va_start(Args, Format);
	int Result = vfprintf(File, Format, Args);
	va_end(Args);
	return Result;
}
static void PatchTalk()
{
	PatchImage();
	PatchJump(0x5121ea, (void *)ImageTime);
	PatchJump(0x513eb1, (void *)ImageLocaltime);
	PatchJump(0x511d6b, (void *)ImageSprintf);
	PatchJump(0x512173, (void *)ImageFprintf);
	DWORD Protection;
	void *ClockFunction = (void *)_time32;
	TestClockFunction = ClockFunction;
	memcpy(SavedClock, ClockFunction, 5);
	CHECK(VirtualProtect(ClockFunction, 5, PAGE_EXECUTE_READWRITE, &Protection) != 0, "patch test CRT clock");
	PatchJump((uint32)(uintptr_t)ClockFunction, (void *)ImageTime);
	FlushInstructionCache(GetCurrentProcess(), ClockFunction, 5);
	VirtualProtect(ClockFunction, 5, Protection, &Protection);
	for (uint32 Address : {0x71ef70u, 0x71ef90u, 0x71efb0u, 0x71efd0u})
		if (Global<TTalkFilter **>(Address + 28) == NULL)
			At<VectorFn>(0x4fd6e0)((void *)(uintptr_t)Address, NULL, 0, Address == 0x71efd0 ? 4 : 9,
				Address == 0x71efd0 ? 5 : 10, NULL);
}
static bool SameText(const char *Ours, const char *Original)
{
	return (Ours == NULL) == (Original == NULL) && (Ours == NULL || strcmp(Ours, Original) == 0);
}
static void CheckLine(const TTalkLine *Ours, const TTalkLine *Original)
{
	CHECK(Ours->StatementID == Original->StatementID && Ours->Mode == Original->Mode
		&& Ours->Time == Original->Time, "statement scalars");
	CHECK(strcmp(Ours->Addressee, Original->Addressee) == 0, "statement addressee");
	CHECK(SameText(Ours->Line, Original->Line) && SameText(Ours->WithoutTime, Original->WithoutTime)
		&& SameText(Ours->Text, Original->Text), "statement text: '%s', original '%s'", Ours->Line, Original->Line);
	CHECK(Ours->WithoutTime - Ours->Line == Original->WithoutTime - Original->Line
		&& Ours->Text - Ours->Line == Original->Text - Original->Line, "statement offsets");
}
static int LineIndex(const TTalkBuffer *Buffer, const TTalkLine *Line)
{
	int Index = 0;
	for (TTalkLine *Search = Buffer->first; Search != NULL; Search = Search->Next, Index++)
		if (Search == Line)
			return Index;
	return -1;
}
static void CheckBuffer(const TTalkBuffer *Ours, const TTalkBuffer *Original)
{
	CHECK(Ours->ReceiveTalkMode == Original->ReceiveTalkMode && Ours->SendTalkMode == Original->SendTalkMode,
		"buffer modes");
	CHECK(Ours->lines == Original->lines && Ours->limit == Original->limit
		&& Ours->Changed == Original->Changed && Ours->Closed == Original->Closed, "buffer state");
	CHECK(memcmp(&Ours->ShowMessages, &Original->ShowMessages, 6) == 0, "buffer flags");
	CHECK(LineIndex(Ours, Ours->cursor) == LineIndex(Original, Original->cursor), "cursor index");
	TTalkLine *OurLine = Ours->first, *OriginalLine = Original->first;
	int Count = 0;
	while (OurLine != NULL && OriginalLine != NULL && Count <= Ours->lines) {
		CheckLine(OurLine, OriginalLine);
		CHECK(LineIndex(Ours, OurLine->Previous) == LineIndex(Original, OriginalLine->Previous), "previous link");
		OurLine = OurLine->Next; OriginalLine = OriginalLine->Next;
		Count++;
	}
	CHECK(OurLine == NULL && OriginalLine == NULL && Count == Ours->lines, "linked list length");
	CHECK(LineIndex(Ours, Ours->last) == LineIndex(Original, Original->last), "last line");
}
static TTalkLine *OriginalLine(uint32 ID, int Mode, const char *Name, const char *Text, int DiffTime)
{
	TTalkLine *Line = (TTalkLine *)malloc(sizeof(TTalkLine));
	memset(Line, 0xa5, sizeof(TTalkLine));
	if (Name == NULL)
		At<LineFn>(0x5044b0)(Line, NULL, ID, Mode, Text, DiffTime);
	else
		At<NamedLineFn>(0x5048a0)(Line, NULL, ID, Mode, Name, Text, DiffTime);
	return Line;
}
static void FreeLine(TTalkLine *Line) { free(Line->Line); free(Line); }

static void TestHelpers()
{
	for (int Mode = 1; Mode <= 25; Mode++) {
		TRGBColor Ours, Original;
		GetTalkColor(&Ours, Mode);
		TRGBColor *Returned = At<TRGBColor *(__cdecl *)(TRGBColor *, int)>(0x4fd9e0)(&Original, Mode);
		CHECK(Returned == &Original && memcmp(&Ours, &Original, sizeof(Ours)) == 0, "talk color %d", Mode);
	}
	struct { uint32 Set, Get; void (*OurSet)(bool); bool (*OurGet)(); } Flags[] = {
		{0x4fd4e0, 0x4fd4f0, SetIgnoreEnabled, GetIgnoreEnabled},
		{0x4fd500, 0x4fd510, SetBuddyIgnore, GetBuddyIgnore},
		{0x4fd520, 0x4fd530, SetIgnoreListEnabled, GetIgnoreListEnabled}
	};
	for (auto &Flag : Flags)
		for (bool Value : {false, true}) {
			Flag.OurSet(Value); At<SetBoolFn>(Flag.Set)(Value);
			CHECK(Flag.OurGet() == At<BoolFn>(Flag.Get)(), "Talk flag");
		}
	for (int Round = 0; Round < 30; Round++) {
		SetTalkChanged(); At<VoidFn>(0x4fd4c0)();
		CHECK(GetTalkChanged() == At<BoolFn>(0x4fd440)() && !GetTalkChanged() && !At<BoolFn>(0x4fd440)(), "talk changed latch");
		SetConsoleChanged(); At<VoidFn>(0x4fd590)();
		CHECK(GetConsoleChanged() == At<BoolFn>(0x4fd580)() && !GetConsoleChanged() && !At<BoolFn>(0x4fd580)(), "console changed latch");
		for (int Delta : {0, 999, 1000, 1001}) {
			SetTestClock(10000);
			SetTalkThrottle(); At<VoidFn>(0x4fd540)();
			SetTestClock(10000 + Delta);
			CHECK(IsTalkThrottled() == At<BoolFn>(0x4fd550)(), "throttle %d", Delta);
			CHECK(IsTalkThrottled() == At<BoolFn>(0x4fd550)(), "throttle repeated");
		}
	}
	for (bool Permanent : {false, true}) {
		SetStatusMessage("status", Permanent);
		At<void (__cdecl *)(const char *, bool)>(0x4ff500)("status", Permanent);
		for (int Tick = 0; Tick < 70; Tick++) {
			CHECK(SameText(GetStatusMessage(), At<const char *(__cdecl *)()>(0x4fd4d0)()), "status timer %d", Tick);
			UpdateStatusMessage(); At<VoidFn>(0x4ff7d0)();
			CHECK(GetTalkChanged() == At<BoolFn>(0x4fd440)(), "status changed");
		}
	}
	SetStatusMessage(NULL, false); At<void (__cdecl *)(const char *, bool)>(0x4ff500)(NULL, false);
	CHECK(GetStatusMessage() == NULL && At<const char *(__cdecl *)()>(0x4fd4d0)() == NULL, "clear status");
	const char *Edges[] = {"", "\n", "\n\n", "a\nb\n", "                                        ",
		"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMN", "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa", "abc\tdef\rghi"};
	for (int Round = 0; Round < 4000; Round++) {
		char Text[900];
		if (Round < 8)
			strcpy(Text, Edges[Round]);
		else {
			RandomText(Text, Random() % 899);
			for (char *Search = Text; *Search != 0; Search++)
				if (Random() % 13 == 0)
					*Search = '\n';
		}
		TTalkbox Ours, Original;
		memset(&Ours, 0xa5, sizeof(Ours));
		Ours.Lines = Round < 8 ? 0 : Random() % 12;
		Original = Ours;
		WrapTalkbox(&Ours, Text);
		At<void (__cdecl *)(TTalkbox *, const char *)>(0x4fecc0)(&Original, Text);
		CHECK(memcmp(&Ours, &Original, sizeof(Ours)) == 0, "WrapTalkbox round %d", Round);
	}
}
static void TestFilters()
{
	for (int Round = 0; Round < 6000; Round++) {
		TTalkFilter Filter = {};
		Filter.Mode = Random() % 27;
		Filter.Channel = (int)(Random() % 10) - 1;
		strcpy(Filter.Addressee, Round % 3 == 0 ? "" : "Alice");
		int Mode = Random() % 30;
		int Channel = (int)(Random() % 10) - 1;
		const char *Name = Round % 3 == 0 ? NULL : Round % 3 == 1 ? "ALICE" : "Bob";
		CHECK(Filter.Matches(Mode, Name, Channel) == At<MatchFn>(0x4ff810)(&Filter, NULL, Mode, Name, Channel), "filter match");
	}
	vector<TTalkFilter *> Ours(0, 4, 5, NULL);
	alignas(4) uint8 OriginalData[32] = {};
	At<VectorFn>(0x4fd6e0)(OriginalData, NULL, 0, 4, 5, NULL);
	vector<TTalkFilter *> &Original = *(vector<TTalkFilter *> *)OriginalData;
	for (int Count = 0; Count < 65; Count++) {
		char Name[30]; sprintf(Name, "filter%d", Count);
		TTalkBuffer *Buffer = (TTalkBuffer *)(uintptr_t)(Count + 1);
		AddTalkFilter(Count % 9, Name, Count, Buffer, Ours, (Count & 1) != 0);
		At<void (__cdecl *)(int, const char *, int, TTalkBuffer *, void *, bool)>(0x4ffe40)(
			Count % 9, Name, Count, Buffer, &Original, (Count & 1) != 0);
		CHECK(memcmp(&Ours, &Original, 21) == 0, "filter vector layout %d", Count);
		int Index = FindTextBuffer(Buffer, Ours);
		CHECK(Index == At<int (__cdecl *)(TTalkBuffer *, void *)>(0x4ffb90)(Buffer, &Original), "find buffer");
		CHECK(FindTalkFilter(Count % 9, Name, Count, Ours)
			== At<int (__cdecl *)(int, const char *, int, void *)>(0x4ff9c0)(Count % 9, Name, Count, &Original), "find filter");
		CHECK(Ours(Index)->Mode == Original(Index)->Mode && Ours(Index)->Channel == Original(Index)->Channel
			&& Ours(Index)->Permanent == Original(Index)->Permanent
			&& strcmp(Ours(Index)->Addressee, Original(Index)->Addressee) == 0, "new filter");
	}
	for (int Index = Ours.min; Index <= Ours.max; Index++) { free(Ours(Index)); free(Original(Index)); }
	// vector<T>'s destructor deletes the entries again at the end of the function.
	delete[] Ours.entry; Ours.entry = NULL; free(Original.entry);
	const char *Names[] = {"Alice", "Bob", "alice", "Carol", "", "Daniel"};
	for (int Round = 0; Round < 2500; Round++) {
		const char *Name = Names[Random() % 6];
		bool Permanent = (Random() & 1) != 0;
		int Mode = Random() % 27;
		switch (Random() % 17) {
		case 0: AddBuddyIgnore(Name, Permanent); At<void (__cdecl *)(const char *, bool)>(0x5006f0)(Name, Permanent); break;
		case 1: IgnoreName(Name, Permanent); At<void (__cdecl *)(const char *, bool)>(0x501940)(Name, Permanent); break;
		case 2: IgnoreMode(Mode); At<void (__cdecl *)(int)>(0x5017b0)(Mode); break;
		case 3: UnignoreName(Name); At<void (__cdecl *)(const char *)>(0x502010)(Name); break;
		case 4: UnignoreMode(Mode); At<void (__cdecl *)(int)>(0x501e70)(Mode); break;
		case 5: AddTemporaryIgnore(Name); At<void (__cdecl *)(const char *)>(0x500c60)(Name); break;
		case 6: RemoveTemporaryIgnore(Name); At<void (__cdecl *)(const char *)>(0x500e40)(Name); break;
		case 7: ClearBuddyIgnore(); At<VoidFn>(0x500520)(); break;
		case 8: ClearTemporaryIgnore(); At<VoidFn>(0x500a90)(); break;
		case 9: ClearIgnoreList(); At<VoidFn>(0x501400)(); break;
		case 10: SetIgnoreEnabled(Permanent); At<SetBoolFn>(0x4fd4e0)(Permanent); break;
		case 11: SetBuddyIgnore(Permanent); At<SetBoolFn>(0x4fd500)(Permanent); break;
		case 12: SetIgnoreListEnabled(Permanent); At<SetBoolFn>(0x4fd520)(Permanent); break;
		case 13: AddIgnoreFilter(Mode, Name, Permanent); At<void (__cdecl *)(int, const char *, bool)>(0x5015d0)(Mode, Name, Permanent); break;
		case 14: RemoveIgnoreFilter(Mode, Name); At<void (__cdecl *)(int, const char *)>(0x501ca0)(Mode, Name); break;
		case 15: ShowPermanentIgnoreWarning(); At<VoidFn>(0x5076e0)(); break;
		case 16: ClearDefaultTextBuffer(); At<VoidFn>(0x5021a0)(); break;
		}
		CHECK(IsModeIgnored(Mode) == At<bool (__cdecl *)(int)>(0x501260)(Mode), "ignored mode");
		CHECK(IsPermanentIgnore(Name) == At<bool (__cdecl *)(const char *)>(0x501ae0)(Name), "permanent ignore");
		CHECK(IsPermanentBuddyIgnore(Name) == At<bool (__cdecl *)(const char *)>(0x5008d0)(Name), "permanent buddy ignore");
		for (int TestMode : {0, 1, 4, 9, 10, 11, 12, 18, 26})
			CHECK(IsTalkIgnored(TestMode, Name, -1)
				== At<bool (__cdecl *)(int, const char *, int)>(0x500060)(TestMode, Name, -1), "ignore decision");
		char *OurBuddy = GetBuddyIgnoreText(), *OriginalBuddy = At<char *(__cdecl *)()>(0x5002d0)();
		char *OurIgnore = GetIgnoreListText(), *OriginalIgnore = At<char *(__cdecl *)()>(0x501010)();
		CHECK(SameText(OurBuddy, OriginalBuddy) && SameText(OurIgnore, OriginalIgnore), "serialized filters");
		free(OurBuddy); free(OriginalBuddy); free(OurIgnore); free(OriginalIgnore);
		CheckBuffer(DefaultTextBuffer, Global<TTalkBuffer *>(0x71ef68));
	}
}

static void TestTalkboxes()
{
	ResetTalk(); At<VoidFn>(0x504d70)();
	SetPrivateMessages(true); At<SetBoolFn>(0x4db5b0)(true);
	AddTalkbox(1, "Alice", 1, 2, 3, "seed");
	At<void (__cdecl *)(int, const char *, int, int, int, const char *)>(0x5052e0)(1, "Alice", 1, 2, 3, "seed");
	TTalkbox *OurBoxes = GetTalkbox(1);
	TTalkbox *OriginalBoxes = (TTalkbox *)(uintptr_t)0x71dde8;
	SetTestClock(10000);
	UpdateTalkboxes(); At<VoidFn>(0x4ff130)();
	int Clock = 10000;
	int Modes[] = {1, 2, 3, 4, 9, 11, 16, 17};
	for (int Round = 0; Round < 6000; Round++) {
		int Choice = Random() % 12;
		if (Choice < 7) {
			char Text[500]; RandomText(Text, Random() % 499);
			int Mode = Modes[Random() % 8];
			const char *Speaker = Round % 2 == 0 ? "Alice" : "Bob";
			int x = Random() % 3, y = Random() % 3, z = Random() % 2;
			AddTalkbox(Mode, Speaker, x, y, z, Text);
			At<void (__cdecl *)(int, const char *, int, int, int, const char *)>(0x5052e0)(Mode, Speaker, x, y, z, Text);
		} else if (Choice == 7) {
			int Mode = Round % 3 == 0 ? 18 : Round % 3 == 1 ? 19 : 22;
			AddMessageTalkbox(Mode, "information\nsecond line");
			At<void (__cdecl *)(int, const char *)>(0x505ac0)(Mode, "information\nsecond line");
		} else if (Choice == 8) {
			int Position = Random() % 10;
			RemoveTalkbox(Position); At<void (__cdecl *)(int)>(0x4fe960)(Position);
		} else if (Choice == 9) {
			RemoveOldestTalkbox(); At<VoidFn>(0x4ff330)();
		} else if (Choice == 10) {
			Clock += Random() % 1500; SetTestClock(Clock);
			UpdateTalkboxes(); At<VoidFn>(0x4ff130)();
		} else {
			bool Enabled = (Random() & 1) != 0;
			SetPrivateMessages(Enabled); At<SetBoolFn>(0x4db5b0)(Enabled);
		}
		CHECK(memcmp(OurBoxes, OriginalBoxes, sizeof(TTalkbox) * 10) == 0, "talkbox state round %d", Round);
		CHECK(GetTalkChanged() == At<BoolFn>(0x4fd440)(), "talkbox changed");
		int Count = GetNumberOfTalkboxes();
		CHECK(Count == At<IntFn>(0x4fd450)(), "talkbox count");
		for (int Number = 1; Number <= Count; Number++) {
			TTalkbox *Original = At<TTalkbox *(__cdecl *)(int)>(0x4fdd80)(Number);
			CHECK(GetTalkbox(Number) - OurBoxes == Original - OriginalBoxes, "talkbox lookup");
			CHECK(GetTalkboxMode(Number) == At<int (__cdecl *)(int)>(0x4fdf10)(Number), "talkbox mode");
			int OurPosition[3], OriginalPosition[3];
			GetTalkboxPosition(Number, &OurPosition[0], &OurPosition[1], &OurPosition[2]);
			At<void (__cdecl *)(int, int *, int *, int *)>(0x4fe0b0)(Number, &OriginalPosition[0], &OriginalPosition[1], &OriginalPosition[2]);
			CHECK(memcmp(OurPosition, OriginalPosition, sizeof(OurPosition)) == 0, "talkbox position");
			int Left = Random() % 100, Top = Random() % 100;
			SetTalkboxRectangle(Number, Left, Top, 80, 30);
			At<void (__cdecl *)(int, int, int, int, int)>(0x4fef70)(Number, Left, Top, 80, 30);
			int OurRect[4], OriginalRect[4];
			GetTalkboxRectangle(Number, &OurRect[0], &OurRect[1], &OurRect[2], &OurRect[3]);
			At<void (__cdecl *)(int, int *, int *, int *, int *)>(0x4fe2b0)(Number, &OriginalRect[0], &OriginalRect[1], &OriginalRect[2], &OriginalRect[3]);
			CHECK(memcmp(OurRect, OriginalRect, sizeof(OurRect)) == 0, "talkbox rectangle");
			TRGBColor OurColor, OriginalColor;
			GetTalkboxColor(&OurColor, Number);
			At<TRGBColor *(__cdecl *)(TRGBColor *, int)>(0x4fe470)(&OriginalColor, Number);
			CHECK(memcmp(&OurColor, &OriginalColor, sizeof(OurColor)) == 0, "talkbox color");
			CHECK(GetTalkboxLines(Number) == At<int (__cdecl *)(int)>(0x4fe620)(Number), "talkbox lines");
			for (int Line = 0; Line < GetTalkboxLines(Number); Line++)
				CHECK(strcmp(GetTalkboxLine(Number, Line), At<const char *(__cdecl *)(int, int)>(0x4fe7c0)(Number, Line)) == 0, "talkbox line");
		}
		CHECK(FindFreeTalkbox() == At<IntFn>(0x505120)(), "free talkbox");
	}
}
static void TestLinesAndBuffers()
{
	for (int Round = 0; Round < 1500; Round++) {
		char Text[700], Name[80];
		RandomText(Text, Random() % 699); RandomText(Name, Random() % 79);
		uint32 ID = Random(); int Mode = Random() % 27;
		int DiffTime = (int)(Random() % 200000) - 100000;
		bool Named = (Round & 1) != 0;
		TTalkLine *Original = OriginalLine(ID, Mode, Named ? Name : NULL, Text, DiffTime);
		TTalkLine *Ours = Named ? new TTalkLine(ID, Mode, Name, Text, DiffTime) : new TTalkLine(ID, Mode, Text, DiffTime);
		CheckLine(Ours, Original);
		for (bool Stamps : {false, true}) {
			SetTimeStamps(Stamps); At<SetBoolFn>(0x4db5a0)(Stamps);
			CHECK(strcmp(Ours->GetText(), At<const char *(__fastcall *)(void *, void *)>(0x4fd6c0)(Original, NULL)) == 0, "GetText");
		}
		FreeLine(Ours); FreeLine(Original);
	}
	alignas(4) uint8 OurData[80], OriginalData[80];
	memset(OurData, 0x55, sizeof(OurData)); memset(OriginalData, 0x55, sizeof(OriginalData));
	TTalkBuffer *Ours = new (OurData) TTalkBuffer(1);
	TTalkBuffer *Original = (TTalkBuffer *)OriginalData;
	At<ThisIntFn>(0x4fd5a0)(Original, NULL, 1);
	CHECK(memcmp(OurData + 4, OriginalData + 4, 76) == 0, "base constructor including untouched bytes");
	Ours->Closed = Original->Closed = false;
	for (int Round = 0; Round < 2500; Round++) {
		int Choice = Random() % 10;
		if (Choice < 4) {
			char Text[100]; RandomText(Text, Random() % 99);
			int Mode = Random() % 27;
			if (Choice == 0) {
				Ours->AddText(Mode, Text, 42);
				At<ThisTextFn>(0x506930)(Original, NULL, Mode, Text, 42);
			} else {
				TTalkLine *OurLine = new TTalkLine(Round, Mode, "Alice", Text, 42);
				TTalkLine *OtherLine = OriginalLine(Round, Mode, "Alice", Text, 42);
				Ours->Append(OurLine); At<ThisLineFn>(0x502c00)(Original, NULL, OtherLine);
			}
		} else if (Choice == 4 && Ours->lines > 0) {
			int Index = Random() % Ours->lines;
			TTalkLine *OurLine = Ours->GetLine(Index);
			TTalkLine *OtherLine = At<GetLineFn>(0x503670)(Original, NULL, Index);
			Ours->Remove(OurLine); At<ThisLineFn>(0x502f00)(Original, NULL, OtherLine);
		} else if (Choice == 5 && Ours->lines > 0) {
			Ours->Remove(0); At<ThisIntFn>(0x503210)(Original, NULL, 0);
		} else if (Choice == 6) {
			Ours->Clear(); At<ThisFn>(0x502a60)(Original, NULL);
		} else if (Choice == 7) {
			Ours->Enabled = Original->Enabled = (Random() & 1) != 0;
			Ours->limit = Original->limit = Random() % 10;
		} else if (Choice == 8 && Ours->lines > 0) {
			int Index = Random() % Ours->lines;
			TTalkLine *OurLine = Ours->GetLine(Index);
			TTalkLine *OtherLine = At<GetLineFn>(0x503670)(Original, NULL, Index);
			CheckLine(OurLine, OtherLine);
			CHECK((Ours->GetNext() == NULL) == (At<NextFn>(0x503920)(Original, NULL) == NULL), "GetNext");
		} else if (Choice == 9) {
			Ours->Close(); At<ThisFn>(0x4fd600)(Original, NULL);
		}
		CheckBuffer(Ours, Original);
		CHECK(Ours->GetChanged() == At<ThisBoolFn>(0x4fd5f0)(Original, NULL), "buffer changed latch");
		CHECK(Ours->GetClosed() == At<ThisBoolFn>(0x4fd6b0)(Original, NULL), "buffer closed latch");
		CHECK(Ours->isDefault() == At<ThisBoolFn>(0x4fd620)(Original, NULL), "isDefault");
		CHECK(Ours->AcceptMessages() == At<ThisBoolFn>(0x4fd630)(Original, NULL), "AcceptMessages");
		CHECK(Ours->IsRuleViolation() == At<ThisBoolFn>(0x4fd640)(Original, NULL), "base predicate");
		bool Value = (Random() & 1) != 0;
		Ours->SetSelected(Value); At<ThisSetBoolFn>(0x4fd660)(Original, NULL, Value);
		Ours->SetLocked(Value); At<ThisSetBoolFn>(0x4fd670)(Original, NULL, Value);
		CHECK(Ours->GetSelected() == At<ThisBoolFn>(0x4fd650)(Original, NULL)
			&& Ours->GetLocked() == At<ThisBoolFn>(0x4fd680)(Original, NULL), "buffer lock/select");
		CHECK(Ours->GetReceiveTalkMode() == At<int (__fastcall *)(void *, void *)>(0x4fd690)(Original, NULL)
			&& Ours->GetSendTalkMode() == At<int (__fastcall *)(void *, void *)>(0x4fd6a0)(Original, NULL), "inline modes");
	}
	Ours->Enabled = Original->Enabled = true;
	Ours->limit = Original->limit = 100;
	Ours->AddText(1, "saved text", 0); At<ThisTextFn>(0x506930)(Original, NULL, 1, "saved text", 0);
	for (bool Stamps : {false, true}) {
		SetTimeStamps(Stamps); At<SetBoolFn>(0x4db5a0)(Stamps);
		FILE *OurFile = tmpfile(), *OtherFile = tmpfile();
		CHECK(OurFile != NULL && OtherFile != NULL, "temporary console logs");
		if (OurFile != NULL && OtherFile != NULL) {
			CHECK(Ours->Write(OurFile) == At<int (__fastcall *)(void *, void *, FILE *)>(0x503a70)(Original, NULL, OtherFile), "Write result");
			rewind(OurFile); rewind(OtherFile);
			char OurText[4096] = {}, OtherText[4096] = {};
			fread(OurText, 1, 4095, OurFile); fread(OtherText, 1, 4095, OtherFile);
			CHECK(strcmp(OurText, OtherText) == 0, "console log output");
		}
		if (OurFile != NULL) fclose(OurFile);
		if (OtherFile != NULL) fclose(OtherFile);
	}
	Ours->~TTalkBuffer(); At<ThisFn>(0x506920)(Original, NULL);
	CheckBuffer(Ours, Original);
}

template <class Function> static Function Virtual(TTalkBuffer *Buffer, int Offset)
{
	return (Function)(*(void ***)Buffer)[Offset / 4];
}
static void TestRouting()
{
	struct { int Receive, Send; const char *Name; int Channel; } Cases[] = {
		{1, 1, NULL, -1}, {4, 4, "Alice", -1}, {5, 5, NULL, 2},
		{5, 5, NULL, 8}, {7, 8, "Reporter", -1}, {6, 0, NULL, 3}
	};
	std::vector<TTalkBuffer *> Ours, Original;
	for (auto &Case : Cases) {
		TTalkBuffer *OurBuffer = CreateTextBuffer(Case.Receive, Case.Send, Case.Name, Case.Channel);
		TTalkBuffer *OtherBuffer = At<CreateFn>(0x507bc0)(Case.Receive, Case.Send, Case.Name, Case.Channel);
		CHECK(OurBuffer != NULL && OtherBuffer != NULL, "CreateTextBuffer receive %d send %d", Case.Receive, Case.Send);
		if (OurBuffer == NULL || OtherBuffer == NULL)
			return;
		OurBuffer->Closed = OtherBuffer->Closed = false;
		OurBuffer->ShowMessages = OtherBuffer->ShowMessages = true;
		CheckBuffer(OurBuffer, OtherBuffer);
		CHECK(OurBuffer->GetReceiveTalkMode() == Case.Receive && OtherBuffer->ReceiveTalkMode == Case.Receive,
			"factory receive mode");
		CHECK(OurBuffer->GetSendTalkMode() == Virtual<int (__fastcall *)(void *, void *)>(OtherBuffer, 0x50)(OtherBuffer, NULL), "factory send mode");
		CHECK(SameText(OurBuffer->GetAddressee(), Virtual<const char *(__fastcall *)(void *, void *)>(OtherBuffer, 0x54)(OtherBuffer, NULL)), "factory addressee");
		CHECK(OurBuffer->GetChannelID() == Virtual<int (__fastcall *)(void *, void *)>(OtherBuffer, 0x58)(OtherBuffer, NULL), "factory channel ID");
		CHECK((CreateTextBuffer(Case.Receive, Case.Send, Case.Name, Case.Channel) == NULL)
			== (At<CreateFn>(0x507bc0)(Case.Receive, Case.Send, Case.Name, Case.Channel) == NULL), "duplicate factory");
		int LookupChannel = Case.Receive == 6 ? -1 : Case.Channel;
		CHECK(GetTextBuffer(Case.Receive, Case.Name, LookupChannel) == OurBuffer
			&& At<TTalkBuffer *(__cdecl *)(int, const char *, int)>(0x5025c0)(Case.Receive, Case.Name, LookupChannel) == OtherBuffer,
			"buffer lookup");
		Ours.push_back(OurBuffer); Original.push_back(OtherBuffer);
	}
	for (size_t Index = 0; Index < Ours.size(); Index++) {
		TTalkBuffer *OurBuffer = Ours[Index], *OtherBuffer = Original[Index];
		for (int Offset : {0x0c, 0x28, 0x2c, 0x30, 0x34, 0x38}) {
			bool OurValue = false;
			switch (Offset) {
			case 0x0c: OurValue = OurBuffer->CanWrite(); break;
			case 0x28: OurValue = OurBuffer->isDefault(); break;
			case 0x2c: OurValue = OurBuffer->AcceptMessages(); break;
			case 0x30: OurValue = OurBuffer->IsRuleViolation(); break;
			case 0x34: OurValue = OurBuffer->IsRuleViolationChannel(); break;
			case 0x38: OurValue = OurBuffer->IsPrivateChannel(); break;
			}
			CHECK(OurValue == Virtual<ThisBoolFn>(OtherBuffer, Offset)(OtherBuffer, NULL), "factory virtual %x", Offset);
		}
		for (int Count = 0; Count < 15; Count++) {
			const char *Name = Count % 2 == 0 ? "Alice" : "Bob";
			OurBuffer->SetWriteEnabled(false);
			Virtual<ThisSetBoolFn>(OtherBuffer, 0x10)(OtherBuffer, NULL, false);
			CHECK(!OurBuffer->Closed && !OtherBuffer->Closed, "write disabling is not Close");
			OurBuffer->Append(new TTalkLine(Count, 7, Name, "report", 0));
			Virtual<ThisLineFn>(OtherBuffer, 0x14)(OtherBuffer, NULL, OriginalLine(Count, 7, Name, "report", 0));
			if (!OurBuffer->IsRuleViolationChannel()) {
				OurBuffer->AddText(19, "notice", 3);
				Virtual<ThisTextFn>(OtherBuffer, 0x18)(OtherBuffer, NULL, 19, "notice", 3);
			}
			CheckBuffer(OurBuffer, OtherBuffer);
			CHECK(OurBuffer->CanWrite() == Virtual<ThisBoolFn>(OtherBuffer, 0x0c)(OtherBuffer, NULL), "write-enabled override");
		}
		OurBuffer->Remove(0); Virtual<ThisIntFn>(OtherBuffer, 0x1c)(OtherBuffer, NULL, 0);
		CheckBuffer(OurBuffer, OtherBuffer);
		OurBuffer->Remove(OurBuffer->last); Virtual<ThisLineFn>(OtherBuffer, 0x20)(OtherBuffer, NULL, OtherBuffer->last);
		CheckBuffer(OurBuffer, OtherBuffer);
		OurBuffer->Clear(); Virtual<ThisFn>(OtherBuffer, 0x24)(OtherBuffer, NULL);
		CheckBuffer(OurBuffer, OtherBuffer);
	}
	for (int Round = 0; Round < 500; Round++) {
		int Mode = Random() % 27;
		int Channel = Round % 2 == 0 ? 2 : 8;
		const char *Name = Round % 2 == 0 ? "Alice" : "Bob";
		if (Round % 5 == 0) Name = NULL;
		bool Enabled = (Random() & 1) != 0;
		SetEventMessages(Enabled); At<SetBoolFn>(0x4db580)(Enabled);
		SetStatusMessages(!Enabled); At<SetBoolFn>(0x4db590)(!Enabled);
		SetInfoMessages(Enabled); At<SetBoolFn>(0x4db570)(Enabled);
		AddConsoleText(Round, "routed text", Mode, Name, Channel, 100);
		At<void (__cdecl *)(uint32, const char *, int, const char *, int, int)>(0x5061f0)(Round, "routed text", Mode, Name, Channel, 100);
		AddChannelNotice("notice", "channel event", Mode, Name, Channel);
		At<void (__cdecl *)(const char *, const char *, int, const char *, int)>(0x506720)("notice", "channel event", Mode, Name, Channel);
		AddTextBufferLine(DefaultTextBuffer, Round, "direct", Mode, Name, 300);
		At<void (__cdecl *)(TTalkBuffer *, uint32, const char *, int, const char *, int)>(0x505f10)(
			Global<TTalkBuffer *>(0x71ef68), Round, "direct", Mode, Name, 300);
		CHECK(GetConsoleChanged() == At<BoolFn>(0x4fd580)(), "console changed after routing");
		for (size_t Index = 0; Index < Ours.size(); Index++) {
			CheckBuffer(Ours[Index], Original[Index]);
			bool Selected = Random() % 4 == 0;
			bool Locked = Random() % 4 == 0;
			Ours[Index]->SetSelected(Selected); Virtual<ThisSetBoolFn>(Original[Index], 0x40)(Original[Index], NULL, Selected);
			Ours[Index]->SetLocked(Locked); Virtual<ThisSetBoolFn>(Original[Index], 0x44)(Original[Index], NULL, Locked);
		}
		CheckBuffer(DefaultTextBuffer, Global<TTalkBuffer *>(0x71ef68));
		CHECK(HasOwnPrivateChannel() == At<BoolFn>(0x502880)(), "own private channel");
		if (Round % 5 == 0) {
			ClearTextBuffer(Ours[0]); At<void (__cdecl *)(TTalkBuffer *)>(0x502340)(Original[0]);
			ClearDefaultTextBuffer(); At<VoidFn>(0x5021a0)();
		}
	}
	for (size_t Index = 0; Index < Ours.size(); Index++) {
		DeleteTextBuffer(Ours[Index]); At<DeleteFn>(0x507fa0)(Original[Index]);
	}
	TTalkBuffer *RuleChannel = CreateTextBuffer(6, 1, NULL, 4);
	TTalkBuffer *RuleChannelOriginal = At<CreateFn>(0x507bc0)(6, 1, NULL, 4);
	CHECK(RuleChannel != NULL && RuleChannelOriginal != NULL, "rule-violation factory send mode 1");
	if (RuleChannel != NULL && RuleChannelOriginal != NULL) {
		RuleChannel->SetSelected(true);
		Virtual<ThisSetBoolFn>(RuleChannelOriginal, 0x40)(RuleChannelOriginal, NULL, true);
		CHECK(RuleChannel->GetSelected() && RuleChannelOriginal->Selected, "console selection slot");
		for (const char *Name : {"Alice", "Bob", "Carol"}) {
			AddTextBufferLine(RuleChannel, 1, "request", 6, Name, 0);
			At<void (__cdecl *)(TTalkBuffer *, uint32, const char *, int, const char *, int)>(0x505f10)
				(RuleChannelOriginal, 1, "request", 6, Name, 0);
		}
		RuleChannel->cursor = RuleChannel->first;
		RuleChannelOriginal->cursor = RuleChannelOriginal->first;
		TTalkLine *OurStatement = RuleChannel->first;
		TTalkLine *OtherStatement = RuleChannelOriginal->first;
		while (OurStatement != NULL && OtherStatement != NULL) {
			TTalkLine *OurCurrent = OurStatement;
			TTalkLine *OtherCurrent = OtherStatement;
			OurStatement = RuleChannel->GetNext();
			OtherStatement = At<NextFn>(0x503920)(RuleChannelOriginal, NULL);
			CHECK(SameText(OurCurrent->Addressee, OtherCurrent->Addressee), "rule-violation statement view");
			if (CompareStrings(OurCurrent->Addressee, "aLiCe", -1) == 0)
				RuleChannel->Remove(OurCurrent);
			if (CompareStrings(OtherCurrent->Addressee, "aLiCe", -1) == 0)
				Virtual<ThisLineFn>(RuleChannelOriginal, 0x20)(RuleChannelOriginal, NULL, OtherCurrent);
		}
		CHECK(OurStatement == NULL && OtherStatement == NULL, "rule-violation cursor end");
		CHECK(RuleChannel->lines == 2 && RuleChannelOriginal->lines == 2, "rule-violation matching removal");
		CheckBuffer(RuleChannel, RuleChannelOriginal);
		DeleteTextBuffer(RuleChannel); At<DeleteFn>(0x507fa0)(RuleChannelOriginal);
	}
	DeleteTextBuffer(DefaultTextBuffer); At<DeleteFn>(0x507fa0)(Global<TTalkBuffer *>(0x71ef68));
	CHECK(!HasOwnPrivateChannel() && !At<BoolFn>(0x502880)(), "deleted console buffers");
	ResetTalk(); At<VoidFn>(0x504d70)();
	CheckBuffer(DefaultTextBuffer, Global<TTalkBuffer *>(0x71ef68));
}
void TestTalk()
{
	int Before = Checks, BeforeFailures = Failures;
	PatchTalk();
	InitTalk(); At<VoidFn>(0x504f30)();
	DefaultTextBuffer->Closed = Global<TTalkBuffer *>(0x71ef68)->Closed = false;
	CheckBuffer(DefaultTextBuffer, Global<TTalkBuffer *>(0x71ef68));
	TestHelpers();
	TestFilters();
	TestTalkboxes();
	TestLinesAndBuffers();
	TestRouting();
	ExitTalk(); At<VoidFn>(0x507a00)();
	CHECK(DefaultTextBuffer == NULL && Global<TTalkBuffer *>(0x71ef68) == NULL, "ExitTalk");
	DWORD Protection;
	VirtualProtect(TestClockFunction, 5, PAGE_EXECUTE_READWRITE, &Protection);
	memcpy(TestClockFunction, SavedClock, 5);
	FlushInstructionCache(GetCurrentProcess(), TestClockFunction, 5);
	VirtualProtect(TestClockFunction, 5, Protection, &Protection);
	printf("Talk: %d checks, %d failures\n", Checks - Before, Failures - BeforeFailures);
}
