// Differential test: calls functions in the original Tibia.exe image and the
// reconstructed ones on the same inputs and compares the results.
//
// Tibia.exe is copied to its base address (see RunChild and MapImage): no
// imports are bound and no code runs except the functions called here. Only functions that do not
// touch imports are called. The original C runtime is not initialised, so
// statustest.cpp redirects the image's malloc and free to the test's own heap
// (PatchJump) before it calls functions that allocate.
//
// Usage: difftest.exe [path to Tibia.exe]

#include "difftest.h"
#include "../../src/Utils.h"
#include "../../src/TXTEA.h"

int Failures = 0;
int Checks = 0;

static uint32 Seed = 12345;
uint32 Random()
{
	Seed = Seed * 1103515245 + 12345;
	return Seed >> 8;
}

// Original functions.
typedef void (__cdecl *XTEAFn)(uint32 *Block, const uint32 *Key);
typedef int (__cdecl *CharFn)(int c);
typedef void (__cdecl *HSVFn)(double H, double S, double V, int *R, int *G, int *B);
typedef void (__cdecl *OutfitFn)(int Color, int *R, int *G, int *B);
typedef void (__cdecl *StrFn)(char *s);
typedef int (__cdecl *CompareFn)(const char *s1, const char *s2, int Count);
typedef void (__cdecl *CopyFn)(char *Destination, const char *Source, int Size);
typedef void (__cdecl *TrimFn)(char *Destination, const char *Source);
typedef bool (__cdecl *ShortenFn)(char *Text, int MaxLength);
typedef void (__cdecl *FormatFn)(int Number, char *Buffer);
// thiscall members through __fastcall: ECX = this, EDX unused, rest on the stack.
typedef void (__fastcall *BufInitFn)(void *This, void *, uint8 *Data, int Size);
typedef uint32 (__fastcall *BufGetFn)(void *This, void *);
typedef void (__fastcall *BufGetStringFn)(void *This, void *, char *Buffer, int MaxLength);
typedef void (__fastcall *BufGetBytesFn)(void *This, void *, uint8 *Buffer, int Count);
typedef void (__fastcall *BufPutFn)(void *This, void *, uint32 Data);
typedef void (__fastcall *BufPutStringFn)(void *This, void *, const char *s);
typedef void (__fastcall *BufPutBytesFn)(void *This, void *, const uint8 *Buffer, int Count);
typedef void (__fastcall *XTEAKeyFn)(void *This, void *, const uint8 *Key);
typedef void (__fastcall *XTEABlockFn)(void *This, void *, uint8 *Block);

void RandomText(char *Text, int Length)
{
	static const char Chars[] = "aAzZ09 \t\n\r\f\v.,-_\xe4\xf6\xfc\xc4\xd6\xdc\xdf\xe9\xc9\xff\xc0\xde\xdf\xe0";
	for (int i = 0; i < Length; i++)
		Text[i] = Chars[Random() % (sizeof(Chars) - 1)];
	Text[Length] = 0;
}

static void TestXTEA()
{
	XTEAFn Encrypt = At<XTEAFn>(0x508cd0);
	XTEAFn Decrypt = At<XTEAFn>(0x508e00);
	XTEAKeyFn SetKey = At<XTEAKeyFn>(0x508ca0);
	XTEABlockFn EncryptBlock = At<XTEABlockFn>(0x508f60);
	XTEABlockFn DecryptBlock = At<XTEABlockFn>(0x508f80);
	for (int t = 0; t < 2000; t++) {
		uint8 Key[16];
		for (int i = 0; i < 16; i++)
			Key[i] = (uint8)Random();
		uint32 Plain[2] = {Random() ^ (Random() << 16), Random() ^ (Random() << 16)};

		uint32 Original[2] = {Plain[0], Plain[1]};
		Encrypt(Original, (const uint32 *)Key);
		TXTEA Cipher;
		Cipher.SetKey(Key);
		uint32 Ours[2] = {Plain[0], Plain[1]};
		Cipher.EncryptBlock((uint8 *)Ours);
		CHECK(Ours[0] == Original[0] && Ours[1] == Original[1], "XTEA encrypt differs");

		// The original object: vtable, then the key.
		uint32 Object[5] = {0, 0, 0, 0, 0};
		SetKey(Object, 0, Key);
		uint32 Wrapped[2] = {Plain[0], Plain[1]};
		EncryptBlock(Object, 0, (uint8 *)Wrapped);
		CHECK(Wrapped[0] == Original[0] && Wrapped[1] == Original[1], "TXTEA::EncryptBlock differs");

		Decrypt(Original, (const uint32 *)Key);
		Cipher.DecryptBlock((uint8 *)Ours);
		DecryptBlock(Object, 0, (uint8 *)Wrapped);
		CHECK(Original[0] == Plain[0] && Ours[0] == Plain[0] && Ours[1] == Plain[1] && Wrapped[0] == Plain[0],
			"XTEA decrypt differs");
	}
}

static void TestCharacters()
{
	struct { uint32 Address; int (*Ours)(int); const char *Name; } Functions[] = {
		{0x508fa0, IsSpace, "IsSpace"}, {0x508fd0, IsAlpha, "IsAlpha"}, {0x509020, IsDigit, "IsDigit"},
		{0x509040, ToLower, "ToLower"}, {0x509060, ToUpper, "ToUpper"},
	};
	for (auto &F : Functions) {
		CharFn Original = At<CharFn>(F.Address);
		for (int c = -300; c < 600; c++)
			CHECK(F.Ours(c) == Original(c), "%s(%d): %d, original %d", F.Name, c, F.Ours(c), Original(c));
	}
}

static void TestColors()
{
	HSVFn HSV = At<HSVFn>(0x509080);
	OutfitFn Outfit = At<OutfitFn>(0x509280);
	for (int Color = -5; Color < 140; Color++) {
		int r1, g1, b1, r2, g2, b2;
		GetOutfitColor(Color, &r1, &g1, &b1);
		Outfit(Color, &r2, &g2, &b2);
		CHECK(r1 == r2 && g1 == g2 && b1 == b2, "GetOutfitColor(%d): %d,%d,%d original %d,%d,%d",
			Color, r1, g1, b1, r2, g2, b2);
	}
	for (int t = 0; t < 20000; t++) {
		double H = (Random() % 1001) / 1000.0, S = (Random() % 1001) / 1000.0, V = (Random() % 1001) / 1000.0;
		if (t % 7 == 0) S = 0.0;
		if (t % 11 == 0) V = 0.0;
		int r1, g1, b1, r2, g2, b2;
		HSVToRGB(H, S, V, &r1, &g1, &b1);
		HSV(H, S, V, &r2, &g2, &b2);
		CHECK(r1 == r2 && g1 == g2 && b1 == b2, "HSVToRGB(%g,%g,%g): %d,%d,%d original %d,%d,%d",
			H, S, V, r1, g1, b1, r2, g2, b2);
	}
}

static void TestStrings()
{
	StrFn Lower = At<StrFn>(0x5093d0);
	StrFn Capitalize = At<StrFn>(0x509540);
	CompareFn Compare = At<CompareFn>(0x509710);
	CopyFn Copy = At<CopyFn>(0x509a60);
	TrimFn Trim = At<TrimFn>(0x509d10);
	ShortenFn Shorten = At<ShortenFn>(0x50a040);
	for (int t = 0; t < 5000; t++) {
		char A[64], B[64], C[64], D[64];
		int Length = Random() % 40;
		RandomText(A, Length);
		strcpy(B, A);
		LowerString(A);
		Lower(B);
		CHECK(strcmp(A, B) == 0, "LowerString differs");

		RandomText(A, Length);
		strcpy(B, A);
		CapitalizeString(A);
		Capitalize(B);
		CHECK(strcmp(A, B) == 0, "CapitalizeString differs");

		RandomText(A, Length);
		strcpy(B, A);
		if (Random() % 3 == 0)
			for (int i = 0; B[i]; i++)
				if (Random() % 4 == 0) B[i] = (char)ToUpper(B[i]);
		if (Random() % 4 == 0 && Length > 0)
			B[Random() % Length] = (char)Random();
		int Count = Random() % 45;
		CHECK(CompareStrings(A, B, Count) == Compare(A, B, Count), "CompareStrings differs");

		int Size = 1 + Random() % 50;
		memset(C, 'x', sizeof(C));
		memset(D, 'x', sizeof(D));
		CopyString(C, A, Size);
		Copy(D, A, Size);
		CHECK(memcmp(C, D, sizeof(C)) == 0, "CopyString differs");

		memset(C, 'x', sizeof(C));
		memset(D, 'x', sizeof(D));
		TrimString(C, A);
		Trim(D, A);
		CHECK(memcmp(C, D, sizeof(C)) == 0, "TrimString differs");

		strcpy(C, A);
		strcpy(D, A);
		int MaxLength = Random() % 45;
		bool r1 = ShortenString(C, MaxLength);
		bool r2 = Shorten(D, MaxLength);
		CHECK(r1 == r2 && strcmp(C, D) == 0, "ShortenString(\"%s\", %d) differs", A, MaxLength);
	}
}

static void TestFormatNumber()
{
	FormatFn Format = At<FormatFn>(0x50bce0);
	__try {
		char Probe[32];
		Format(1234567, Probe);
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		printf("skip FormatNumber: the original sprintf needs the original CRT\n");
		return;
	}
	for (int t = 0; t < 2000; t++) {
		int Number = t < 1000 ? t * t * 37 : (int)(Random() & 0x7fffffff);
		char A[32], B[32];
		FormatNumber(Number, A);
		Format(Number, B);
		CHECK(strcmp(A, B) == 0, "FormatNumber(%d): \"%s\" original \"%s\"", Number, A, B);
	}
}

static void TestBuffers()
{
	BufInitFn ReadInit = At<BufInitFn>(0x50a200);
	BufGetFn GetByte = At<BufGetFn>(0x50a4b0), GetWord = At<BufGetFn>(0x50a610), GetQuad = At<BufGetFn>(0x50a780);
	BufGetStringFn GetStr = At<BufGetStringFn>(0x50a900);
	BufGetBytesFn GetBytes = At<BufGetBytesFn>(0x50ad50);
	BufInitFn WriteInit = At<BufInitFn>(0x50b030);
	BufPutFn PutByte = At<BufPutFn>(0x50b2e0), PutWord = At<BufPutFn>(0x50b450), PutQuad = At<BufPutFn>(0x50b5e0);
	BufPutStringFn PutStr = At<BufPutStringFn>(0x50b780);
	BufPutBytesFn PutBytes = At<BufPutBytesFn>(0x50ba00);

	for (int t = 0; t < 2000; t++) {
		uint8 Ours[1024], Original[1024];
		memset(Ours, 0xcd, sizeof(Ours));
		memset(Original, 0xcd, sizeof(Original));
		TWriteBuffer W(Ours, sizeof(Ours));
		uint32 WO[3];
		WriteInit(WO, 0, Original, sizeof(Original));
		// Random sequence of writes, at most ~800 bytes.
		uint32 Ops[40], Values[40];
		char Texts[40][32];
		int OpCount = Random() % 40;
		for (int i = 0; i < OpCount; i++) {
			Ops[i] = Random() % 5;
			Values[i] = Random() ^ (Random() << 16);
			RandomText(Texts[i], Random() % 20);
			switch (Ops[i]) {
			case 0: W.PutByte((uint8)Values[i]); PutByte(WO, 0, (uint8)Values[i]); break;
			case 1: W.PutWord((uint16)Values[i]); PutWord(WO, 0, (uint16)Values[i]); break;
			case 2: W.PutQuad(Values[i]); PutQuad(WO, 0, Values[i]); break;
			case 3: W.PutString(Texts[i]); PutStr(WO, 0, Texts[i]); break;
			case 4: W.PutBytes((const uint8 *)Texts[i], 7); PutBytes(WO, 0, (const uint8 *)Texts[i], 7); break;
			}
		}
		CHECK(memcmp(Ours, Original, sizeof(Ours)) == 0 && W.Position == (int)WO[2], "TWriteBuffer differs");

		TReadBuffer R(Ours, W.Position);
		uint32 RO[3];
		ReadInit(RO, 0, Original, WO[2]);
		for (int i = 0; i < OpCount; i++) {
			char A[32], B[32];
			uint8 X[8], Y[8];
			int MaxLength = (int)(Random() % 24) - 2;
			if (MaxLength == 0) MaxLength = -1;
			switch (Ops[i]) {
			case 0: CHECK(R.GetByte() == (uint8)GetByte(RO, 0), "GetByte differs"); break;
			case 1: CHECK(R.GetWord() == (uint16)GetWord(RO, 0), "GetWord differs"); break;
			case 2: CHECK(R.GetQuad() == GetQuad(RO, 0), "GetQuad differs"); break;
			case 3:
				memset(A, 'x', sizeof(A));
				memset(B, 'x', sizeof(B));
				R.GetString(A, MaxLength);
				GetStr(RO, 0, B, MaxLength);
				CHECK(memcmp(A, B, sizeof(A)) == 0, "GetString(MaxLength %d) differs", MaxLength);
				break;
			case 4:
				R.GetBytes(X, 7);
				GetBytes(RO, 0, Y, 7);
				CHECK(memcmp(X, Y, 7) == 0, "GetBytes differs");
				break;
			}
			CHECK(R.Position == (int)RO[2], "read position differs");
		}
	}
}

// Tibia.exe has no relocations, so it must sit at its base 0x400000, but a
// running 32-bit process already has heaps and code-page tables there. So the
// harness starts a suspended copy of itself, reserves 0x400000-0x728000 in it
// before any of its start-up code runs, and resumes it. The copy (--child)
// then copies Tibia.exe's sections into that range. No loader code, import
// binding or entry point of Tibia.exe runs.
static const uintptr_t TIBIA_BASE = 0x400000;
static const SIZE_T TIBIA_SIZE = 0x328000; // SizeOfImage of Tibia.exe 7.72

static int RunChild(int argc, char **argv)
{
	char CommandLine[2 * MAX_PATH];
	char Self[MAX_PATH];
	GetModuleFileNameA(NULL, Self, sizeof(Self));
	snprintf(CommandLine, sizeof(CommandLine), "\"%s\" --child \"%s\"", Self, argc > 1 ? argv[1] : "C:\\Tibia\\Tibia.exe");
	STARTUPINFOA Startup = {sizeof(Startup)};
	Startup.dwFlags = STARTF_USESTDHANDLES;
	Startup.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
	Startup.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
	Startup.hStdError = GetStdHandle(STD_ERROR_HANDLE);
	PROCESS_INFORMATION Process;
	if (!CreateProcessA(Self, CommandLine, NULL, NULL, TRUE, CREATE_SUSPENDED, NULL, NULL, &Startup, &Process)) {
		printf("cannot start the test process (error %lu)\n", GetLastError());
		return 2;
	}
	void *Reserved = VirtualAllocEx(Process.hProcess, (void *)TIBIA_BASE, TIBIA_SIZE,
		MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	if (Reserved != (void *)TIBIA_BASE) {
		printf("cannot reserve 0x%x in the test process (error %lu)\n", (unsigned)TIBIA_BASE, GetLastError());
		TerminateProcess(Process.hProcess, 2);
		return 2;
	}
	ResumeThread(Process.hThread);
	WaitForSingleObject(Process.hProcess, INFINITE);
	DWORD ExitCode = 2;
	GetExitCodeProcess(Process.hProcess, &ExitCode);
	CloseHandle(Process.hThread);
	CloseHandle(Process.hProcess);
	return (int)ExitCode;
}

static bool MapImage(const char *Path)
{
	MEMORY_BASIC_INFORMATION Info;
	VirtualQuery((void *)TIBIA_BASE, &Info, sizeof(Info));
	if (Info.AllocationBase != (void *)TIBIA_BASE || Info.State != MEM_COMMIT || Info.Type != MEM_PRIVATE) {
		printf("0x%x was not reserved for the image\n", (unsigned)TIBIA_BASE);
		return false;
	}
	FILE *File = fopen(Path, "rb");
	if (File == NULL) {
		printf("cannot open %s\n", Path);
		return false;
	}
	static uint8 Data[4 << 20];
	size_t Size = fread(Data, 1, sizeof(Data), File);
	fclose(File);
	IMAGE_NT_HEADERS32 *NT = (IMAGE_NT_HEADERS32 *)(Data + ((IMAGE_DOS_HEADER *)Data)->e_lfanew);
	if (NT->OptionalHeader.ImageBase != TIBIA_BASE || NT->OptionalHeader.SizeOfImage != TIBIA_SIZE) {
		printf("%s is not the expected Tibia.exe 7.72 image\n", Path);
		return false;
	}
	uint8 *Image = (uint8 *)TIBIA_BASE;
	memcpy(Image, Data, NT->OptionalHeader.SizeOfHeaders);
	IMAGE_SECTION_HEADER *Section = IMAGE_FIRST_SECTION(NT);
	for (int i = 0; i < NT->FileHeader.NumberOfSections; i++, Section++) {
		if (Section->PointerToRawData + Section->SizeOfRawData <= Size)
			memcpy(Image + Section->VirtualAddress, Data + Section->PointerToRawData, Section->SizeOfRawData);
	}
	return true;
}

// Reports where a crash happened; the original code has no symbols.
static LONG CALLBACK ReportCrash(EXCEPTION_POINTERS *Info)
{
	DWORD Code = Info->ExceptionRecord->ExceptionCode;
	if (Code == EXCEPTION_ACCESS_VIOLATION || Code == EXCEPTION_ILLEGAL_INSTRUCTION
	 || Code == EXCEPTION_STACK_OVERFLOW || Code == EXCEPTION_INT_DIVIDE_BY_ZERO) {
		printf("crash: exception 0x%08lx at 0x%p, eip 0x%08lx esp 0x%08lx\n", Code,
			Info->ExceptionRecord->ExceptionAddress, (unsigned long)Info->ContextRecord->Eip,
			(unsigned long)Info->ContextRecord->Esp);
		if (Code == EXCEPTION_ACCESS_VIOLATION)
			printf("crash: %s 0x%08lx\n", Info->ExceptionRecord->ExceptionInformation[0] ? "writing" : "reading",
				(unsigned long)Info->ExceptionRecord->ExceptionInformation[1]);
		uint32 *Stack = (uint32 *)(uintptr_t)Info->ContextRecord->Esp;
		for (int i = 0; i < 12; i++) // return addresses show the caller
			printf("crash: [esp%+d] 0x%08x\n", i * 4, Stack[i]);
		printf("crash: ebp 0x%08lx\n", (unsigned long)Info->ContextRecord->Ebp);
	}
	return EXCEPTION_CONTINUE_SEARCH;
}

int main(int argc, char **argv)
{
	if (argc < 2 || strcmp(argv[1], "--child") != 0)
		return RunChild(argc, argv);
	setvbuf(stdout, NULL, _IONBF, 0); // keep the output of a crashed run
	AddVectoredExceptionHandler(1, ReportCrash);
	if (!MapImage(argc > 2 ? argv[2] : "C:\\Tibia\\Tibia.exe"))
		return 2;
	TestXTEA();
	TestCharacters();
	TestColors();
	TestStrings();
	TestFormatNumber();
	TestBuffers();
	TestSkins();
	TestStatus();
	TestPreferences();
	TestScript();
	TestObjects(argc > 2 ? argv[2] : "C:\\Tibia\\Tibia.exe");
	printf("%d checks, %d failures\n", Checks, Failures);
	return Failures == 0 ? 0 : 1;
}
