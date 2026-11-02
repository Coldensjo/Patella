// Differential tests of the error paths (issue #22): TASSERT, TERROR,
// PROGRAM_ERROR throws and the TEND wrapping of the original code against the
// reconstructed code, through imageeh.cpp, plus TMyException's messages and
// the report HandleException writes to error.txt.
//
// The messages are compared in full except for the line numbers: our sources
// do not keep the original's lines. Every "<file> <line>" of a message is
// compared with the line replaced, and each of our lines must stand for one
// original line and each original line for one of ours over the whole run
// (ER_LINES=1 prints the pairs).
//
// DIFFTEST_ONLY=Errors runs this test alone; ER_TRACE=1 prints every compared
// exception, ER_SEED=<n> changes the random inputs.

#include "imageeh.h"
#include "../../src/Utils.h"
#include "../../src/Player.h"
#include "../../src/Surface.h"
#include "../../src/Control.h"
#include "../../src/sysinc.h"
#include <direct.h>
#include <time.h>
#include <map>
#include <vector>

// Own random numbers, so the later tests' inputs stay the same.
static uint32 ErSeed = 22;
static uint32 ErRandom()
{
	ErSeed = ErSeed * 1103515245 + 12345;
	return ErSeed >> 8;
}

static bool Trace = false;
static int Compared = 0;      // exceptions compared
static int ComparedThrows = 0; // of them, calls that threw on both sides

// --- Comparing messages ---------------------------------------------------

typedef std::map<std::string, std::map<int, int>> TLineMap;
static TLineMap OursToOriginal, OriginalToOurs;

// "<file> <line>" at Start (a message's start or a "reason:\n"): the
// position of the line number and its end, or false.
static bool FindLine(const std::string &Text, size_t Start, size_t *Number, size_t *End)
{
	size_t Space = Start;
	while (Space < Text.size() && Text[Space] != ' ' && Text[Space] != '\n')
		Space++;
	if (Space == Start || Space >= Text.size() || Text[Space] != ' ')
		return false;
	size_t Digits = Space + 1;
	if (Digits < Text.size() && Text[Digits] == '-')
		Digits++;
	size_t Stop = Digits;
	while (Stop < Text.size() && Text[Stop] >= '0' && Text[Stop] <= '9')
		Stop++;
	if (Stop == Digits || (Stop < Text.size() && Text[Stop] != ':'))
		return false;
	*Number = Space + 1;
	*End = Stop;
	return true;
}

struct TLineRef {
	std::string File;
	int Line;
};

// Replaces the line numbers of every "<file> <line>: " that starts the
// message or follows ", reason:\n", and of the origin "<file> <line>".
static std::string WithoutLines(const std::string &Text, std::vector<TLineRef> *Lines)
{
	std::string Result;
	size_t Position = 0;
	size_t Start = 0;
	for (;;) {
		size_t Number, End;
		if (FindLine(Text, Start, &Number, &End)) {
			Result.append(Text, Position, Number - Position);
			Result += "#";
			TLineRef Ref = {Text.substr(Start, Number - 1 - Start), atoi(Text.c_str() + Number)};
			Lines->push_back(Ref);
			Position = End;
		}
		size_t Next = Text.find(", reason:\n", (std::max)(Position, Start));
		if (Next == std::string::npos)
			break;
		Start = Next + 10;
	}
	Result.append(Text, Position, std::string::npos);
	return Result;
}

static void MapLines(const char *What, const std::vector<TLineRef> &Ours, const std::vector<TLineRef> &Original)
{
	for (size_t i = 0; i < Ours.size() && i < Original.size(); i++) {
		if (Ours[i].File != Original[i].File)
			continue; // the messages differ; reported there
		const std::string &File = Ours[i].File;
		std::map<int, int> &Forward = OursToOriginal[File];
		std::map<int, int> &Backward = OriginalToOurs[File];
		auto F = Forward.find(Ours[i].Line);
		auto B = Backward.find(Original[i].Line);
		CHECK((F == Forward.end() || F->second == Original[i].Line) && (B == Backward.end() || B->second == Ours[i].Line),
			"Errors %s: our %s line %d stands for the original's line %d, earlier for %d; the original's line %d earlier for our %d",
			What, File.c_str(), Ours[i].Line, Original[i].Line, F == Forward.end() ? -1 : F->second, Original[i].Line,
			B == Backward.end() ? -1 : B->second);
		Forward[Ours[i].Line] = Original[i].Line;
		Backward[Original[i].Line] = Ours[i].Line;
	}
}

// Compares what both sides threw.
static void Compare(const char *What, const TThrown &Ours, const TThrown &Original)
{
	Compared++;
	if (Original.Kind != TThrown::NOTHING)
		ComparedThrows++;
	if (Trace)
		printf("Errors %s: %s\n", What, Original.Describe().c_str());
	std::vector<TLineRef> OurLines, OriginalLines;
	TThrown A = Ours, B = Original;
	A.Message = WithoutLines(Ours.Message, &OurLines);
	A.Origin = WithoutLines(Ours.Origin + ":", &OurLines);
	B.Message = WithoutLines(Original.Message, &OriginalLines);
	B.Origin = WithoutLines(Original.Origin + ":", &OriginalLines);
	CHECK(A == B && OurLines.size() == OriginalLines.size(), "Errors %s: ours %s, original %s", What,
		Ours.Describe().c_str(), Original.Describe().c_str());
	MapLines(What, OurLines, OriginalLines);
}

#define COMPARE(What, OurCall, ImageCall) Compare(What, OurThrow([&] { OurCall; }), ImageThrow([&] { ImageCall; }))

// --- TMyException.cpp -----------------------------------------------------

typedef void (__fastcall *ExceptionCtorFn)(void *This, void *, const void *File, int Line, const void *Kind,
	const void *Name1, int Value1, const void *Name2, int Value2, const void *Last);
typedef void (__fastcall *ThisFn)(void *This, void *);

struct TImageException {
	uint8 Bytes[0x3c];
	TImageException() { memset(Bytes, 0, sizeof(Bytes)); }
	~TImageException() { At<ThisFn>(0x508300)(Bytes, NULL); }
	std::string Message() const { return ImageString(Bytes + 0x04); }
	std::string Origin() const { return ImageString(Bytes + 0x20); }
};

// A VC7.1 std::string of the image for the length of a call.
struct TImageString {
	uint8 Bytes[0x1c];
	explicit TImageString(const std::string &Text) { MakeImageString(Bytes, Text.c_str()); }
	~TImageString() { FreeImageString(Bytes); }
};

static std::string RandomPart(int MaxLength)
{
	static const char Chars[] = "abcXYZ019 _.,:()=-\\/\n\xe4\xdf";
	std::string Text;
	int Length = (int)(ErRandom() % (MaxLength + 1));
	for (int i = 0; i < Length; i++)
		Text += Chars[ErRandom() % (sizeof(Chars) - 1)];
	return Text;
}

static std::string RandomFile()
{
	static const char *const Files[] = {
		"C:\\Tibia\\Source\\Map.cpp", "c:/cipsoft/client/GUI.cpp", "Utils.cpp", "..\\Container.h", "dir\\", "dir/",
		"", "/", "\\x", "a/b\\c.cpp", "TMyException.cpp", "C:\\a/b\\very long file name of more than sixteen.cpp",
	};
	if (ErRandom() % 4 == 0)
		return RandomPart(40);
	return Files[ErRandom() % (sizeof(Files) / sizeof(Files[0]))];
}

static int RandomValue()
{
	switch (ErRandom() % 4) {
	case 0: return (int)(ErRandom() % 20) - 5;
	case 1: return (int)(ErRandom() ^ (ErRandom() << 16));
	case 2: return ErRandom() % 2 == 0 ? INT_MIN : INT_MAX;
	default: return (int)(ErRandom() % 100000);
	}
}

static void TestMyException()
{
	for (int t = 0; t < 2000; t++) {
		std::string File = RandomFile(), Kind = RandomPart(30), Name1 = ErRandom() % 2 ? RandomPart(20) : "",
			Name2 = ErRandom() % 2 ? RandomPart(20) : "", Last = ErRandom() % 2 ? RandomPart(40) : "";
		int Line = RandomValue(), Value1 = RandomValue(), Value2 = RandomValue();
		TMyException Ours(File, Line, Kind, Name1, Value1, Name2, Value2, Last);
		TImageException Original;
		{
			TImageString F(File), K(Kind), N1(Name1), N2(Name2), L(Last);
			At<ExceptionCtorFn>(0x508400)(Original.Bytes, NULL, F.Bytes, Line, K.Bytes, N1.Bytes, Value1, N2.Bytes, Value2,
				L.Bytes);
		}
		CHECK(Ours.GetMessage() == Original.Message() && Ours.GetOrigin() == Original.Origin(),
			"TMyException(\"%s\", %d, \"%s\", ...): \"%s\" (\"%s\"), original \"%s\" (\"%s\")", File.c_str(), Line,
			Kind.c_str(), Ours.GetMessage().c_str(), Ours.GetOrigin().c_str(), Original.Message().c_str(),
			Original.Origin().c_str());

		// The wrapping constructor on both.
		std::string File2 = RandomFile(), Kind2 = RandomPart(20), Name3 = ErRandom() % 2 ? RandomPart(10) : "",
			Name4 = ErRandom() % 2 ? RandomPart(10) : "";
		int Line2 = RandomValue(), Value3 = RandomValue(), Value4 = RandomValue();
		TMyException OurOuter(File2, Line2, Kind2, Name3, Value3, Name4, Value4, Ours);
		TImageException OriginalOuter;
		{
			TImageString F(File2), K(Kind2), N1(Name3), N2(Name4);
			At<ExceptionCtorFn>(0x5086a0)(OriginalOuter.Bytes, NULL, F.Bytes, Line2, K.Bytes, N1.Bytes, Value3, N2.Bytes,
				Value4, Original.Bytes);
		}
		CHECK(OurOuter.GetMessage() == OriginalOuter.Message() && OurOuter.GetOrigin() == OriginalOuter.Origin(),
			"TMyException(\"%s\", %d, \"%s\", ..., inner): \"%s\" (\"%s\"), original \"%s\" (\"%s\")", File2.c_str(), Line2,
			Kind2.c_str(), OurOuter.GetMessage().c_str(), OurOuter.GetOrigin().c_str(), OriginalOuter.Message().c_str(),
			OriginalOuter.Origin().c_str());
	}
}

// --- Player.cpp: HandleException's error.txt and HandleProgramError -------

static std::string FatalText, FatalCaption; // the last FatalError call of either side
// FatalError ends the program (exit), and the original's callers rely on it:
// the code after their call is something else. The fake raises an exception
// of its own, which UntilFatalError catches around the call.
static const DWORD FATAL_ERROR_CALLED = 0xe0fa7a1e;
static void __fastcall FakeFatalError(void *, void *, const char *Text, const char *Caption)
{
	FatalText = Text;
	FatalCaption = Caption;
	RaiseException(FATAL_ERROR_CALLED, EXCEPTION_NONCONTINUABLE, 0, NULL);
}
static void UntilFatalErrorCall(void (*Function)(void *), void *Context)
{
	__try {
		Function(Context);
	} __except (GetExceptionCode() == FATAL_ERROR_CALLED ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
	}
}
template <class F> static void UntilFatalError(F Function)
{
	UntilFatalErrorCall(ThrowThunk<F>, &Function);
}
static const char *FixedDate = "Thu Jun 08 12:34:56 2006\n";
static uint32 __cdecl FakeImageTime(uint32 *Time)
{
	if (Time != NULL)
		*Time = 1149770096;
	return 1149770096;
}
static __time64_t __cdecl FakeTime64(__time64_t *Time)
{
	if (Time != NULL)
		*Time = 1149770096;
	return 1149770096;
}
static char *__cdecl FakeCtime(const void *) { return (char *)FixedDate; }
static OSVERSIONINFOA FakeVersion;
static BOOL FakeVersionResult;
static BOOL WINAPI FakeGetVersionEx(OSVERSIONINFOA *Version)
{
	if (FakeVersionResult)
		memcpy(Version, &FakeVersion, sizeof(FakeVersion));
	return FakeVersionResult;
}
static void __cdecl IgnoreStream(void *) {}
static const char *__cdecl FakeErrorText(int Code)
{
	static char Text[40];
	snprintf(Text, sizeof(Text), "error text %d", Code);
	return Text;
}
static const char *__cdecl FakeErrorHint(int Code)
{
	static char Text[40];
	snprintf(Text, sizeof(Text), "hint %d\r\nline 2", Code);
	return Text;
}

// ER_WATCHDOG=<seconds>: prints where the test thread is after that time
// (for hangs in the image's code).
static HANDLE MainThread;
static DWORD WINAPI Watchdog(void *Seconds)
{
	Sleep((DWORD)(uintptr_t)Seconds * 1000);
	SuspendThread(MainThread);
	CONTEXT Context;
	Context.ContextFlags = CONTEXT_FULL;
	GetThreadContext(MainThread, &Context);
	printf("watchdog: eip %08lx esp %08lx ebp %08lx\nwatchdog: stack", Context.Eip, Context.Esp, Context.Ebp);
	for (int i = 0; i < 64; i++)
		printf(" %08x", ((uint32 *)(uintptr_t)Context.Esp)[i]);
	printf("\n");
	fflush(stdout);
	TerminateProcess(GetCurrentProcess(), 5);
	return 0;
}

static std::string ReadFile(const char *Name)
{
	std::string Text;
	FILE *File = fopen(Name, "rb");
	if (File == NULL)
		return "(no file)";
	char Buffer[4096];
	size_t Count;
	while ((Count = fread(Buffer, 1, sizeof(Buffer), File)) > 0)
		Text.append(Buffer, Count);
	fclose(File);
	return Text;
}

// The image's ofstream (basic_filebuf) reads the buffer pointers of its FILE
// directly (_ptr, _cnt, _base of VC7.1's FILE), so it cannot get one of our
// CRT's FILEs as scripttest.cpp's PatchScript gives it: it gets a VC7.1 FILE
// without a buffer (_ptr and _cnt 0, so every character goes through fputc or
// fwrite), which holds our FILE.
struct TImageFile {
	char *_ptr;
	int _cnt;
	char *_base;
	int _flag;
	int _file;
	int _charbuf;
	int _bufsiz;
	char *_tmpfname;
	FILE *Real;
};
static TImageFile *__cdecl ImageFileOpen(const char *Name, const char *Mode)
{
	FILE *Real = fopen(Name, Mode);
	if (Real == NULL)
		return NULL;
	TImageFile *File = new TImageFile();
	File->Real = Real;
	return File;
}
static int __cdecl ImageFileClose(TImageFile *File)
{
	int Result = fclose(File->Real);
	delete File;
	return Result;
}
static int __cdecl ImageFilePutc(int c, TImageFile *File) { return fputc(c, File->Real); }
static size_t __cdecl ImageFileWrite(const void *Buffer, size_t Size, size_t Count, TImageFile *File)
{
	return fwrite(Buffer, Size, Count, File->Real);
}
static int __cdecl ImageFileFlush(TImageFile *File) { return fflush(File != NULL ? File->Real : NULL); }
static int __cdecl ImageFileSeek(TImageFile *File, long Offset, int Origin) { return fseek(File->Real, Offset, Origin); }

static void WriteEarlierReport()
{
	FILE *File = fopen("error.txt", "wb");
	fputs("earlier report\r\n", File);
	fclose(File);
}

template <class T> static void *CodeAddress(T Member)
{
	return *(void **)&Member;
}

static void TestPlayer()
{
	PatchScript(); // sprintf (num_put) and the rest of the image's stdio
	PatchImageJump(0x51227f, (void *)ImageFileOpen);
	PatchImageJump(0x512122, (void *)ImageFileClose);
	PatchImageJump(0x5133da, (void *)ImageFilePutc);
	PatchImageJump(0x51338e, (void *)ImageFileWrite);
	PatchImageJump(0x5131de, (void *)ImageFileFlush);
	PatchImageJump(0x5134e3, (void *)ImageFileSeek);
	PatchImageJump(0x4d7d80, (void *)FakeFatalError);
	PatchOwnCode(CodeAddress(&CPlayerApp::FatalError), (void *)FakeFatalError);
	PatchImageJump(0x5121ea, (void *)FakeImageTime);
	PatchOwnCode((void *)_time64, (void *)FakeTime64);
	PatchImageJump(0x5121d1, (void *)FakeCtime);
	PatchOwnCode((void *)_ctime64, (void *)FakeCtime);
	PatchImageJump(0x445900, (void *)IgnoreStream); // OutputControlErrorInformations (Control.cpp)
	PatchOwnCode((void *)(void (*)(std::ostream &))OutputControlErrorInformations, (void *)IgnoreStream);
	PatchImageJump(0x4fcbf0, (void *)FakeErrorText);
	PatchOwnCode((void *)GetErrorText, (void *)FakeErrorText);
	PatchImageJump(0x4fcc90, (void *)FakeErrorHint);
	PatchOwnCode((void *)GetErrorHint, (void *)FakeErrorHint);
	BindImageImport("KERNEL32.dll", "GetVersionExA", (void *)FakeGetVersionEx);
	PatchOwnImport("KERNEL32.dll", "GetVersionExA", (void *)FakeGetVersionEx);

	char Directory[MAX_PATH];
	_getcwd(Directory, sizeof(Directory));
	std::string Work = std::string(getenv("TEMP") != NULL ? getenv("TEMP") : ".") + "\\errortest";
	_mkdir(Work.c_str());
	_chdir(Work.c_str());
	alignas(16) static uint8 OurApp[sizeof(CPlayerApp) + 16];
	static uint8 ImageApp[0x200];
	for (int t = 0; t < 300; t++) {
		std::string File = RandomFile(), Kind = RandomPart(30), Name1 = ErRandom() % 2 ? RandomPart(20) : "",
			Last = RandomPart(60);
		int Line = RandomValue(), Value1 = RandomValue();
		TMyException Ours(File, Line, Kind, Name1, Value1, "", 0, Last);
		TImageException Original;
		{
			TImageString F(File), K(Kind), N1(Name1), N2(""), L(Last);
			At<ExceptionCtorFn>(0x508400)(Original.Bytes, NULL, F.Bytes, Line, K.Bytes, N1.Bytes, Value1, N2.Bytes, 0,
				L.Bytes);
		}
		memset(&FakeVersion, 0, sizeof(FakeVersion));
		FakeVersion.dwOSVersionInfoSize = sizeof(FakeVersion);
		FakeVersion.dwMajorVersion = ErRandom() % 11;
		FakeVersion.dwMinorVersion = ErRandom() % 4;
		FakeVersion.dwBuildNumber = ErRandom() % 30000;
		FakeVersion.dwPlatformId = ErRandom() % 3;
		// At most 29 characters: with 30 or more the original prints its
		// 30-byte buffer without a terminator (whatever follows on the stack).
		std::string Pack = ErRandom() % 3 == 0 ? "" : RandomPart(29);
		strcpy(FakeVersion.szCSDVersion, Pack.c_str());
		FakeVersionResult = ErRandom() % 8 != 0;
		int Engine = (int)(ErRandom() % 5) - 1;
		bool Terminating = ErRandom() % 10 == 0;
		bool Existing = ErRandom() % 3 == 0; // append to an earlier report

		memset(ImageApp, 0, sizeof(ImageApp));
		ImageApp[0xa0] = Terminating;
		Global<int>(0x71d0c4) = Engine; // curEngine
		remove("error.txt");
		if (Existing)
			WriteEarlierReport();
		if (Trace)
			printf("Errors: HandleException %d\n", t);
		FatalText = FatalCaption = "(none)";
		UntilFatalError([&] { At<void (__fastcall *)(void *, void *, void *)>(0x4d9bd0)(ImageApp, NULL, Original.Bytes); });
		std::string OriginalFile = ReadFile("error.txt"), OriginalText = FatalText, OriginalCaption = FatalCaption;
		bool OriginalTerminating = ImageApp[0xa0] != 0;

		memset(OurApp, 0, sizeof(OurApp));
		CPlayerApp *App = (CPlayerApp *)OurApp;
		App->Terminating = Terminating;
		curEngine = Engine;
		remove("error.txt");
		if (Existing)
			WriteEarlierReport();
		FatalText = FatalCaption = "(none)";
		UntilFatalError([&] { App->HandleException(Ours); });
		std::string OurFile = ReadFile("error.txt");
		CHECK(OurFile == OriginalFile, "HandleException: error.txt\n%s\noriginal\n%s", OurFile.c_str(), OriginalFile.c_str());
		CHECK(FatalText == OriginalText && FatalCaption == OriginalCaption && App->Terminating == OriginalTerminating,
			"HandleException: FatalError(\"%s\", \"%s\"), original (\"%s\", \"%s\")", FatalText.c_str(),
			FatalCaption.c_str(), OriginalText.c_str(), OriginalCaption.c_str());
		if (Trace && t < 3)
			printf("Errors: error.txt\n%s\n", OurFile.c_str());

		// HandleProgramError: the message of FatalError.
		int Code = (int)(ErRandom() % 20) - 2;
		memset(ImageApp, 0, sizeof(ImageApp));
		ImageApp[0xa0] = Terminating;
		FatalText = FatalCaption = "(none)";
		UntilFatalError([&] { At<void (__fastcall *)(void *, void *, int)>(0x4d9910)(ImageApp, NULL, Code); });
		OriginalText = FatalText;
		OriginalCaption = FatalCaption;
		App->Terminating = Terminating;
		FatalText = FatalCaption = "(none)";
		UntilFatalError([&] { App->HandleProgramError((PROGRAM_ERROR)Code); });
		CHECK(FatalText == OriginalText && FatalCaption == OriginalCaption,
			"HandleProgramError(%d): FatalError(\"%s\", \"%s\"), original (\"%s\", \"%s\")", Code, FatalText.c_str(),
			FatalCaption.c_str(), OriginalText.c_str(), OriginalCaption.c_str());
	}
	remove("error.txt");
	_chdir(Directory);
}

// --- Utils.cpp --------------------------------------------------------------

typedef void (__cdecl *StrFn)(char *s);
typedef int (__cdecl *CompareFn)(const char *s1, const char *s2, int Count);
typedef void (__cdecl *CopyFn)(char *Destination, const char *Source, int Size);
typedef void (__cdecl *TrimFn)(char *Destination, const char *Source);
typedef bool (__cdecl *ShortenFn)(char *Text, int MaxLength);
typedef void (__cdecl *FormatFn)(int Number, char *Buffer);
typedef void (__fastcall *BufInitFn)(void *This, void *, uint8 *Data, int Size);
typedef uint32 (__fastcall *BufGetFn)(void *This, void *);
typedef void (__fastcall *BufGetStringFn)(void *This, void *, char *Buffer, int MaxLength);
typedef char *(__fastcall *BufNewStringFn)(void *This, void *);
typedef void (__fastcall *BufGetBytesFn)(void *This, void *, uint8 *Buffer, int Count);
typedef void (__fastcall *BufPutFn)(void *This, void *, uint32 Data);
typedef void (__fastcall *BufPutStringFn)(void *This, void *, const char *s);
typedef void (__fastcall *BufPutBytesFn)(void *This, void *, const uint8 *Buffer, int Count);
typedef void (__fastcall *BufSetSizeFn)(void *This, void *, int NewSize);

static void TestUtils()
{
	char A[64] = "Text", B[64] = "Text";
	COMPARE("LowerString(NULL)", LowerString(NULL), At<StrFn>(0x5093d0)(NULL));
	COMPARE("CapitalizeString(NULL)", CapitalizeString(NULL), At<StrFn>(0x509540)(NULL));
	COMPARE("CompareStrings(NULL, s)", CompareStrings(NULL, A, 3), At<CompareFn>(0x509710)(NULL, B, 3));
	COMPARE("CompareStrings(s, NULL)", CompareStrings(A, NULL, 3), At<CompareFn>(0x509710)(B, NULL, 3));
	COMPARE("CopyString(NULL, s)", CopyString(NULL, A, 3), At<CopyFn>(0x509a60)(NULL, B, 3));
	COMPARE("CopyString(s, NULL)", CopyString(A, NULL, 3), At<CopyFn>(0x509a60)(B, NULL, 3));
	COMPARE("TrimString(NULL, s)", TrimString(NULL, A), At<TrimFn>(0x509d10)(NULL, B));
	COMPARE("TrimString(s, NULL)", TrimString(A, NULL), At<TrimFn>(0x509d10)(B, NULL));
	COMPARE("ShortenString(NULL)", ShortenString(NULL, 5), At<ShortenFn>(0x50a040)(NULL, 5));
	COMPARE("FormatNumber(n, NULL)", FormatNumber(5, NULL), At<FormatFn>(0x50bce0)(5, NULL));
	for (int t = 0; t < 50; t++) {
		int Number = -(int)(ErRandom() % 100000) - 1;
		if (t == 0)
			Number = INT_MIN;
		COMPARE("FormatNumber(negative)", FormatNumber(Number, A), At<FormatFn>(0x50bce0)(Number, B));
	}

	// The buffers at random positions near their ends, in both orders of
	// the checks (the pointer first).
	uint8 Data[64], OriginalData[64];
	for (int i = 0; i < 64; i++)
		Data[i] = OriginalData[i] = (uint8)ErRandom();
	COMPARE("TReadBuffer(NULL)", TReadBuffer(NULL, 4), uint32 O[3]; At<BufInitFn>(0x50a200)(O, NULL, NULL, 4));
	COMPARE("TReadBuffer(size<0)", TReadBuffer(Data, -1), uint32 O[3]; At<BufInitFn>(0x50a200)(O, NULL, OriginalData, -1));
	COMPARE("TWriteBuffer(NULL)", TWriteBuffer(NULL, 4), uint32 O[3]; At<BufInitFn>(0x50b030)(O, NULL, NULL, 4));
	COMPARE("TWriteBuffer(size<0)", TWriteBuffer(Data, -7), uint32 O[3]; At<BufInitFn>(0x50b030)(O, NULL, OriginalData, -7));
	for (int t = 0; t < 400; t++) {
		int Size = (int)(ErRandom() % 12);
		int Position = (int)(ErRandom() % 14);
		if (Position > Size + 1)
			Position = Size;
		int Operation = (int)(ErRandom() % 13);
		int Count = (int)(ErRandom() % 8) - 1;
		int MaxLength = (int)(ErRandom() % 10) - 2;
		Data[Position % 64] = OriginalData[Position % 64] = (uint8)(ErRandom() % 12); // a string length
		Data[(Position + 1) % 64] = OriginalData[(Position + 1) % 64] = 0;
		TReadBuffer R(Data, Size);
		TWriteBuffer W(Data, Size);
		R.Position = W.Position = Position;
		uint32 O[3] = {(uint32)(uintptr_t)OriginalData, (uint32)Size, (uint32)Position};
		char Text[16], OriginalText[16];
		uint8 Bytes[16], OriginalBytes[16];
		const char *Put = Count < 0 ? NULL : "abcdefg" + (7 - Count);
		char What[64];
		snprintf(What, sizeof(What), "buffer operation %d (size %d, position %d, count %d)", Operation, Size, Position, Count);
		switch (Operation) {
		case 0: COMPARE(What, R.GetByte(), At<BufGetFn>(0x50a4b0)(O, NULL)); break;
		case 1: COMPARE(What, R.GetWord(), At<BufGetFn>(0x50a610)(O, NULL)); break;
		case 2: COMPARE(What, R.GetQuad(), At<BufGetFn>(0x50a780)(O, NULL)); break;
		case 3:
			COMPARE(What, R.GetString(Count < 0 ? NULL : Text, MaxLength),
				At<BufGetStringFn>(0x50a900)(O, NULL, Count < 0 ? NULL : OriginalText, MaxLength));
			break;
		case 4: COMPARE(What, delete[] R.GetString(), At<BufNewStringFn>(0x50abb0)(O, NULL)); break;
		case 5:
			COMPARE(What, R.GetBytes(Count == 0 ? NULL : Bytes, Count < 0 ? 3 : Count),
				At<BufGetBytesFn>(0x50ad50)(O, NULL, Count == 0 ? NULL : OriginalBytes, Count < 0 ? 3 : Count));
			break;
		case 6: COMPARE(What, W.PutByte(1), At<BufPutFn>(0x50b2e0)(O, NULL, 1)); break;
		case 7: COMPARE(What, W.PutWord(2), At<BufPutFn>(0x50b450)(O, NULL, 2)); break;
		case 8: COMPARE(What, W.PutQuad(3), At<BufPutFn>(0x50b5e0)(O, NULL, 3)); break;
		case 9: COMPARE(What, W.PutString(Put), At<BufPutStringFn>(0x50b780)(O, NULL, Put)); break;
		case 10:
			COMPARE(What, W.PutBytes(Count == 0 ? NULL : Bytes, Count < 0 ? 3 : Count),
				At<BufPutBytesFn>(0x50ba00)(O, NULL, Count == 0 ? NULL : OriginalBytes, Count < 0 ? 3 : Count));
			break;
		default: COMPARE(What, R.SetSize(Count), At<BufSetSizeFn>(0x4ca300)(O, NULL, Count)); break;
		}
		CHECK(R.Position == (int)O[2] || W.Position == (int)O[2], "%s: position differs", What);
	}
}

void TestErrors()
{
	Trace = getenv("ER_TRACE") != NULL && atoi(getenv("ER_TRACE")) != 0;
	if (getenv("ER_SEED") != NULL)
		ErSeed = strtoul(getenv("ER_SEED"), NULL, 10);
	int Before = Checks;
	if (getenv("ER_WATCHDOG") != NULL) {
		DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), &MainThread, 0, FALSE,
			DUPLICATE_SAME_ACCESS);
		CreateThread(NULL, 0, Watchdog, (void *)(uintptr_t)atoi(getenv("ER_WATCHDOG")), 0, NULL);
	}
	EnableImageExceptions();
	if (Trace)
		printf("Errors: %s\n", "TestMyException");
	TestMyException();
	if (Trace)
		printf("Errors: %s\n", "TestPlayer");
	TestPlayer();
	if (Trace)
		printf("Errors: %s\n", "TestUtils");
	TestUtils();
	DisableImageExceptions();
	if (getenv("ER_LINES") != NULL) {
		for (auto &File : OursToOriginal)
			for (auto &Line : File.second)
				printf("Errors line: %s %d -> %d\n", File.first.c_str(), Line.first, Line.second);
	}
	printf("Errors: %d checks, %d exceptions compared (%d calls that threw)\n", Checks - Before, Compared, ComparedThrows);
}
