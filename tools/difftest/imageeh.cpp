// Exceptions of the original code in the difftest child (issue #22); see
// imageeh.h.

#include "imageeh.h"
#include <vector>
#include <typeinfo>
#include <intrin.h>

static const uint32 IMAGE_BASE = 0x400000;
static const uint32 IMAGE_END = 0x728000;

// --- Patches that are undone by DisableImageExceptions --------------------

struct TSavedBytes {
	uint32 Address;
	int Size;
	uint8 Old[8];
};
static std::vector<TSavedBytes> Saved;

static void PatchBytes(uint32 Address, const void *Bytes, int Size)
{
	TSavedBytes S;
	S.Address = Address;
	S.Size = Size;
	memcpy(S.Old, (void *)(uintptr_t)Address, Size);
	Saved.push_back(S);
	memcpy((void *)(uintptr_t)Address, Bytes, Size);
}

static void PatchJumpSaved(uint32 Address, void *Target)
{
	uint8 Code[5];
	Code[0] = 0xe9;
	*(int32_t *)(Code + 1) = (int32_t)((uintptr_t)Target - (Address + 5));
	PatchBytes(Address, Code, 5);
}

void PatchImageJump(uint32 Address, void *Target)
{
	PatchJumpSaved(Address, Target);
	FlushInstructionCache(GetCurrentProcess(), (void *)(uintptr_t)Address, 5);
}

// Our own code and import table are write-protected.
static void PatchOwnBytes(void *Address, const void *Bytes, int Size)
{
	DWORD Old;
	VirtualProtect(Address, Size, PAGE_EXECUTE_READWRITE, &Old);
	PatchBytes((uint32)(uintptr_t)Address, Bytes, Size);
	FlushInstructionCache(GetCurrentProcess(), Address, Size);
}

void PatchOwnCode(void *Function, void *Target)
{
	uint8 Code[5];
	Code[0] = 0xe9;
	*(int32_t *)(Code + 1) = (int32_t)((uintptr_t)Target - ((uintptr_t)Function + 5));
	PatchOwnBytes(Function, Code, 5);
}

void PatchOwnImport(const char *Dll, const char *Name, void *Target)
{
	uint8 *Base = (uint8 *)GetModuleHandleA(NULL);
	IMAGE_NT_HEADERS32 *NT = (IMAGE_NT_HEADERS32 *)(Base + ((IMAGE_DOS_HEADER *)Base)->e_lfanew);
	IMAGE_DATA_DIRECTORY &Directory = NT->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
	for (IMAGE_IMPORT_DESCRIPTOR *D = (IMAGE_IMPORT_DESCRIPTOR *)(Base + Directory.VirtualAddress); D->Name != 0; D++) {
		if (_stricmp((const char *)Base + D->Name, Dll) != 0)
			continue;
		uint32 *Names = (uint32 *)(Base + D->OriginalFirstThunk);
		uint32 *Slots = (uint32 *)(Base + D->FirstThunk);
		for (int i = 0; Names[i] != 0; i++) {
			if ((Names[i] & 0x80000000) == 0 && strcmp((const char *)Base + Names[i] + 2, Name) == 0) {
				PatchOwnBytes(&Slots[i], &Target, 4);
				return;
			}
		}
	}
	printf("imageeh: difftest.exe does not import %s!%s\n", Dll, Name);
}

// The import slot of Dll!Name in the image's import address table.
static uint32 *ImportSlot(const char *Dll, const char *Name)
{
	uint8 *Base = (uint8 *)(uintptr_t)IMAGE_BASE;
	IMAGE_NT_HEADERS32 *NT = (IMAGE_NT_HEADERS32 *)(Base + ((IMAGE_DOS_HEADER *)Base)->e_lfanew);
	IMAGE_DATA_DIRECTORY &Directory = NT->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
	for (IMAGE_IMPORT_DESCRIPTOR *D = (IMAGE_IMPORT_DESCRIPTOR *)(Base + Directory.VirtualAddress); D->Name != 0; D++) {
		if (_stricmp((const char *)Base + D->Name, Dll) != 0)
			continue;
		uint32 *Names = (uint32 *)(Base + D->OriginalFirstThunk);
		uint32 *Slots = (uint32 *)(Base + D->FirstThunk);
		for (int i = 0; Names[i] != 0; i++) {
			if ((Names[i] & 0x80000000) == 0 && strcmp((const char *)Base + Names[i] + 2, Name) == 0)
				return &Slots[i];
		}
	}
	return NULL;
}

// Every import slot the image has not bound otherwise (it still holds the
// RVA of its name) calls a trap that names the import and ends the child: an
// error path that reaches Windows unexpectedly is reported, not a jump to a
// random address.
static const char *TrapNames[512];
static uint8 *TrapCode;
static void __cdecl ImportTrap(uint32 Index, uint32 ReturnAddress)
{
	printf("crash: the image called the unbound import %s from 0x%08x (imageeh.cpp)\n",
		Index < 512 && TrapNames[Index] != NULL ? TrapNames[Index] : "?", ReturnAddress - 6);
	const uint32 *Stack = &ReturnAddress + 2; // the caller's stack: return addresses show its callers
	printf("crash: stack");
	for (int i = 0; i < 24; i++)
		printf(" %08x", Stack[i]);
	printf("\n");
	fflush(stdout);
	TerminateProcess(GetCurrentProcess(), 0xc0000005);
}

static void TrapUnboundImports()
{
	uint8 *Base = (uint8 *)(uintptr_t)IMAGE_BASE;
	IMAGE_NT_HEADERS32 *NT = (IMAGE_NT_HEADERS32 *)(Base + ((IMAGE_DOS_HEADER *)Base)->e_lfanew);
	IMAGE_DATA_DIRECTORY &Directory = NT->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
	if (TrapCode == NULL)
		TrapCode = (uint8 *)VirtualAlloc(NULL, 512 * 16, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	static char NameBuffer[512][64];
	int Count = 0;
	for (IMAGE_IMPORT_DESCRIPTOR *D = (IMAGE_IMPORT_DESCRIPTOR *)(Base + Directory.VirtualAddress); D->Name != 0; D++) {
		uint32 *Names = (uint32 *)(Base + D->OriginalFirstThunk);
		uint32 *Slots = (uint32 *)(Base + D->FirstThunk);
		for (int i = 0; Names[i] != 0 && Count < 512; i++) {
			if (Slots[i] != Names[i])
				continue; // bound by a test
			snprintf(NameBuffer[Count], sizeof(NameBuffer[Count]), "%s!%s", (const char *)Base + D->Name,
				(Names[i] & 0x80000000) != 0 ? "(ordinal)" : (const char *)Base + Names[i] + 2);
			TrapNames[Count] = NameBuffer[Count];
			// push [esp] (the return address); push Count; call ImportTrap
			uint8 *Stub = TrapCode + Count * 16;
			Stub[0] = 0xff; Stub[1] = 0x34; Stub[2] = 0x24;
			Stub[3] = 0x68; *(uint32 *)(Stub + 4) = (uint32)Count;
			Stub[8] = 0xe8; *(int32_t *)(Stub + 9) = (int32_t)((uintptr_t)ImportTrap - ((uintptr_t)Stub + 13));
			void *Target = Stub;
			PatchBytes((uint32)(uintptr_t)&Slots[i], &Target, 4);
			Count++;
		}
	}
}

void BindImageImport(const char *Dll, const char *Name, void *Target)
{
	uint32 *Slot = ImportSlot(Dll, Name);
	if (Slot == NULL) {
		printf("imageeh: the image does not import %s!%s\n", Dll, Name);
		return;
	}
	PatchBytes((uint32)(uintptr_t)Slot, &Target, 4);
}

// --- The image's CRT state -------------------------------------------------

// _getptd (0x51689a) allocates the per-thread data (0x8c bytes) at the first
// call and keeps it in fiber-local storage; the child has one thread, so one
// block, set up as _getptd sets up a new one.
static uint8 ImagePtd[0x8c + 0x100];

static void *__cdecl ImageGetPtd()
{
	return ImagePtd;
}

static void ResetImagePtd()
{
	memset(ImagePtd, 0, sizeof(ImagePtd));
	*(uint32 *)(ImagePtd + 0x00) = GetCurrentThreadId(); // _tid
	*(uint32 *)(ImagePtd + 0x04) = 0xffffffff;           // _thandle
	*(uint32 *)(ImagePtd + 0x14) = 1;                    // _holdrand
	*(uint32 *)(ImagePtd + 0x54) = 0x5c3b88;             // ptlocinfo: __initiallocinfo
}

static bool Enabled = false;
static void *Dispatcher;

static bool InImage(uint32 Address)
{
	return Address >= IMAGE_BASE && Address < IMAGE_END;
}

// Windows calls no exception handler of the image: RtlDispatchException and
// RtlUnwind check every handler (RtlIsValidHandler, the SafeSEH check), and
// with DEP on, which /NXCOMPAT makes permanent, a handler outside a mapped
// image is refused (the image is private memory, no loader mapped it); the
// dispatch ends with STATUS_BAD_STACK or as an unhandled exception. So the
// handler of every registration of the image on the thread's chain is
// replaced by ImageHandlerTrampoline before Windows dispatches an exception:
// a function of difftest.exe that is in its SafeSEH table (imageehseh.asm)
// and calls the original handler (the image's __CxxFrameHandler thunk of that
// function, or its _except_handler3). A vectored handler, which runs before
// the frame-based dispatch, does it for every exception while the image's
// exceptions are enabled; registrations made later (a catch block's frames)
// are replaced at the next exception, before anything can unwind them.
struct TRegistration {
	TRegistration *Next;
	EXCEPTION_DISPOSITION (__cdecl *Handler)(EXCEPTION_RECORD *, void *, CONTEXT *, void *);
};
typedef EXCEPTION_DISPOSITION (__cdecl *THandler)(EXCEPTION_RECORD *, void *, CONTEXT *, void *);

// The original handler of each replaced registration, by its address (a
// stack address: a later frame at the same address writes its own handler,
// which is replaced and recorded again at the next exception).
struct TReplaced {
	TRegistration *Registration;
	THandler Handler;
};
static TReplaced Replaced[256];
static int ReplacedCount = 0;

extern "C" EXCEPTION_DISPOSITION __cdecl ImageHandlerTrampoline(EXCEPTION_RECORD *Record, void *Registration,
	CONTEXT *Context, void *DispatcherContext)
{
	for (int i = 0; i < ReplacedCount; i++) {
		if (Replaced[i].Registration == Registration)
			return Replaced[i].Handler(Record, Registration, Context, DispatcherContext);
	}
	printf("crash: no handler recorded for the registration 0x%p (imageeh.cpp)\n", Registration);
	return ExceptionContinueSearch;
}

static void ReplaceHandler(TRegistration *R)
{
	int Free = -1;
	for (int i = 0; i < ReplacedCount; i++) {
		if (Replaced[i].Registration == R) {
			Free = i;
			break;
		}
		// Entries below the current stack pointer belong to frames that are gone.
		if (Free < 0 && (uintptr_t)Replaced[i].Registration < (uintptr_t)_AddressOfReturnAddress())
			Free = i;
	}
	if (Free < 0) {
		if (ReplacedCount == 256) {
			printf("crash: more than 256 registrations of the image (imageeh.cpp)\n");
			fflush(stdout);
			TerminateProcess(GetCurrentProcess(), 0xc0000005);
		}
		Free = ReplacedCount++;
	}
	Replaced[Free].Registration = R;
	Replaced[Free].Handler = R->Handler;
	R->Handler = ImageHandlerTrampoline;
}

static LONG CALLBACK RedirectImageHandlers(EXCEPTION_POINTERS *Info)
{
	if (!Enabled)
		return EXCEPTION_CONTINUE_SEARCH;
	// EH_SEH=1 names every exception that is not a C++ throw (what a
	// catch (...) reports as "unknown exception occurred").
	static const bool TraceSEH = getenv("EH_SEH") != NULL;
	if (TraceSEH && Info->ExceptionRecord->ExceptionCode != 0xe06d7363)
		printf("eh: exception 0x%08lx at %p\n", Info->ExceptionRecord->ExceptionCode,
			Info->ExceptionRecord->ExceptionAddress);
	if (TraceSEH && Info->ExceptionRecord->ExceptionCode == 0xe06d7363
		&& Info->ExceptionRecord->NumberParameters >= 3) {
		// ThrowInfo -> CatchableTypeArray -> CatchableType -> TypeDescriptor's name (x86: pointers)
		uint32 ThrowInfo = (uint32)Info->ExceptionRecord->ExceptionInformation[2];
		uint32 Array = ThrowInfo != 0 ? *(uint32 *)(uintptr_t)(ThrowInfo + 12) : 0;
		uint32 Type = Array != 0 ? *(uint32 *)(uintptr_t)(*(uint32 *)(uintptr_t)(Array + 4) + 4) : 0;
		const char *Name = Type != 0 ? (const char *)(uintptr_t)(Type + 8) : "?";
		if (strcmp(Name, ".?AVTMyException@@") != 0 && strcmp(Name, ".?AW4PROGRAM_ERROR@@") != 0)
			printf("eh: C++ exception %s (ThrowInfo 0x%x) at %p\n", Name, ThrowInfo,
				Info->ExceptionRecord->ExceptionAddress);
	}
	for (TRegistration *R = (TRegistration *)__readfsdword(0); R != (TRegistration *)(intptr_t)-1; R = R->Next) {
		if (InImage((uint32)(uintptr_t)R->Handler))
			ReplaceHandler(R);
	}
	return EXCEPTION_CONTINUE_SEARCH;
}

void EnableImageExceptions()
{
	if (Enabled)
		return;
	Enabled = true;
	PatchImage(); // malloc and free (operator new calls malloc)
	ResetImagePtd();
	PatchJumpSaved(0x51689a, (void *)ImageGetPtd);
	// The child has one thread: the CRT's and the C++ library's locks are
	// no-ops (their critical sections were never initialised).
	static const uint8 Ret[] = {0xc3};
	static const uint8 LockitCtor[] = {0x8b, 0xc1, 0xc2, 0x04, 0x00}; // mov eax, ecx; ret 4
	PatchBytes(0x515463, Ret, 1);        // _lock
	PatchBytes(0x5153ae, Ret, 1);        // _unlock
	PatchBytes(0x52213b, LockitCtor, 5); // std::_Lockit::_Lockit(int)
	PatchBytes(0x52215e, Ret, 1);        // std::_Lockit::~_Lockit
	PatchBytes(0x522c09, Ret, 1);        // _Mtxinit (each stream buffer has a mutex)
	PatchBytes(0x522c14, Ret, 1);        // _Mtxdst
	PatchBytes(0x522c1f, Ret, 1);        // _Mtxlock
	PatchBytes(0x522c2a, Ret, 1);        // _Mtxunlock
	HMODULE Kernel = GetModuleHandleA("kernel32.dll");
	static const char *const KernelImports[] = {
		"RaiseException", "RtlUnwind", "IsBadReadPtr", "IsBadWritePtr", "IsBadCodePtr", "VirtualQuery",
		"GetLastError", "SetLastError", "GetCurrentThreadId", "InterlockedIncrement", "InterlockedDecrement",
		"InterlockedExchange",
	};
	for (const char *Name : KernelImports)
		BindImageImport("KERNEL32.dll", Name, (void *)GetProcAddress(Kernel, Name));
	TrapUnboundImports();
	Dispatcher = AddVectoredExceptionHandler(1, RedirectImageHandlers);
	ReplacedCount = 0;
	FlushInstructionCache(GetCurrentProcess(), NULL, 0);
}

void DisableImageExceptions()
{
	if (!Enabled)
		return;
	for (size_t i = Saved.size(); i-- > 0;) {
		DWORD Old;
		VirtualProtect((void *)(uintptr_t)Saved[i].Address, Saved[i].Size, PAGE_EXECUTE_READWRITE, &Old);
		memcpy((void *)(uintptr_t)Saved[i].Address, Saved[i].Old, Saved[i].Size);
		VirtualProtect((void *)(uintptr_t)Saved[i].Address, Saved[i].Size, Old, &Old);
	}
	Saved.clear();
	RemoveVectoredExceptionHandler(Dispatcher);
	FlushInstructionCache(GetCurrentProcess(), NULL, 0);
	Enabled = false;
}

// --- Strings and exception objects ----------------------------------------

// VC7.1 basic_string<char>: allocator +0x00, _Bx (buffer of 16 or pointer)
// +0x04, _Mysize +0x14, _Myres +0x18.
std::string ImageString(const void *String)
{
	const uint8 *S = (const uint8 *)String;
	uint32 Size = *(const uint32 *)(S + 0x14);
	uint32 Reserved = *(const uint32 *)(S + 0x18);
	const char *Text = Reserved < 16 ? (const char *)(S + 4) : *(const char *const *)(S + 4);
	return std::string(Text, Size);
}

void MakeImageString(void *String, const char *Text)
{
	uint8 *S = (uint8 *)String;
	uint32 Size = (uint32)strlen(Text);
	memset(S, 0, 0x1c);
	*(uint32 *)(S + 0x14) = Size;
	if (Size < 16) {
		memcpy(S + 4, Text, Size + 1);
		*(uint32 *)(S + 0x18) = 15;
	} else {
		char *Buffer = At<char *(__cdecl *)(size_t)>(0x512d18)(Size + 1);
		memcpy(Buffer, Text, Size + 1);
		*(char **)(S + 4) = Buffer;
		*(uint32 *)(S + 0x18) = Size;
	}
}

void FreeImageString(void *String)
{
	uint8 *S = (uint8 *)String;
	if (*(uint32 *)(S + 0x18) >= 16)
		At<void (__cdecl *)(void *)>(0x51165d)(*(char **)(S + 4)); // the image's free
	memset(S, 0, 0x1c);
}

std::string TThrown::Describe() const
{
	char Text[64];
	switch (Kind) {
	case NOTHING: return "nothing thrown";
	case PROGRAMERROR: snprintf(Text, sizeof(Text), "PROGRAM_ERROR %d", Code); return Text;
	case MYEXCEPTION: return "TMyException \"" + Message + "\" (origin \"" + Origin + "\")";
	default: return "other exception " + Type;
	}
}

TThrown ThrownOf(const TMyException &Exception)
{
	TThrown Result;
	Result.Kind = TThrown::MYEXCEPTION;
	Result.Type = typeid(TMyException).raw_name();
	Result.Message = Exception.GetMessage();
	Result.Origin = Exception.GetOrigin();
	return Result;
}

// --- The boundary ----------------------------------------------------------

// What the filter copies out of the image's exception before the stack is
// unwound (the thrown object lives in the throwing frame).
struct TRawThrow {
	bool Caught;
	TThrown *Result;
};

static int ImageFilter(EXCEPTION_POINTERS *Info, TRawThrow *Raw)
{
	const EXCEPTION_RECORD *Record = Info->ExceptionRecord;
	if (Record->ExceptionCode != 0xe06d7363 || Record->NumberParameters < 3
	 || Record->ExceptionInformation[0] != 0x19930520 || !InImage((uint32)Record->ExceptionInformation[2]))
		return EXCEPTION_CONTINUE_SEARCH;
	const uint8 *Object = (const uint8 *)Record->ExceptionInformation[1];
	const uint32 *ThrowInfo = (const uint32 *)Record->ExceptionInformation[2];
	const uint32 *Types = (const uint32 *)(uintptr_t)ThrowInfo[3];
	const uint32 *Catchable = (const uint32 *)(uintptr_t)Types[1];
	const char *Name = (const char *)(uintptr_t)Catchable[1] + 8;
	TThrown &Result = *Raw->Result;
	Result.Type = Name;
	if (strcmp(Name, ".?AVTMyException@@") == 0) {
		Result.Kind = TThrown::MYEXCEPTION;
		Result.Message = ImageString(Object + 0x04);
		Result.Origin = ImageString(Object + 0x20);
	} else if (strcmp(Name, ".?AW4PROGRAM_ERROR@@") == 0) {
		Result.Kind = TThrown::PROGRAMERROR;
		Result.Code = *(const int *)Object;
	} else {
		Result.Kind = TThrown::OTHER;
	}
	// Nothing in the image catches it: destroy it here, as a catch block's end
	// would (pmfnUnwind, thiscall).
	if (ThrowInfo[1] != 0)
		At<void (__fastcall *)(const void *, void *)>(ThrowInfo[1])(Object, NULL);
	Raw->Caught = true;
	return EXCEPTION_EXECUTE_HANDLER;
}

static void ImageThrowSEH(void (*Function)(void *), void *Context, TRawThrow *Raw)
{
	__try {
		Function(Context);
	} __except (ImageFilter(GetExceptionInformation(), Raw)) {
	}
}

TThrown ImageThrowCall(void (*Function)(void *), void *Context)
{
	TThrown Result;
	TRawThrow Raw = {false, &Result};
	ImageThrowSEH(Function, Context, &Raw);
	return Result;
}

// --- Comparing what both sides threw (moved from errortest.cpp) ----------

static TLineMap OursToOriginal, OriginalToOurs;
int ThrownCompared = 0, ThrownThrew = 0;
const TLineMap &ThrownLineMap() { return OursToOriginal; }

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

static void MapLines(const char *Test, const char *What, const std::vector<TLineRef> &Ours, const std::vector<TLineRef> &Original)
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
			"%s %s: our %s line %d stands for the original's line %d, earlier for %d; the original's line %d earlier for our %d",
			Test, What, File.c_str(), Ours[i].Line, Original[i].Line, F == Forward.end() ? -1 : F->second, Original[i].Line,
			B == Backward.end() ? -1 : B->second);
		Forward[Ours[i].Line] = Original[i].Line;
		Backward[Original[i].Line] = Ours[i].Line;
	}
}

void CompareThrown(const char *Test, const char *What, const TThrown &Ours, const TThrown &Original)
{
	ThrownCompared++;
	if (Original.Kind != TThrown::NOTHING)
		ThrownThrew++;
	std::vector<TLineRef> OurLines, OriginalLines;
	TThrown A = Ours, B = Original;
	A.Message = WithoutLines(Ours.Message, &OurLines);
	A.Origin = WithoutLines(Ours.Origin + ":", &OurLines);
	B.Message = WithoutLines(Original.Message, &OriginalLines);
	B.Origin = WithoutLines(Original.Origin + ":", &OriginalLines);
	CHECK(A == B && OurLines.size() == OriginalLines.size(), "%s %s: ours %s, original %s", Test, What,
		Ours.Describe().c_str(), Original.Describe().c_str());
	// Only a comparison that matched maps lines: one mismatch must not make
	// every later comparison of the same lines fail.
	if (A == B && OurLines.size() == OriginalLines.size())
		MapLines(Test, What, OurLines, OriginalLines);
}

bool Redirected(const void *Code)
{
	return *(const uint8 *)Code == 0xe9;
}
bool ImageRedirected(uint32 Address)
{
	return Redirected((const void *)(uintptr_t)Address);
}

TErrorRound::TErrorRound(const char *_Test) : Test(_Test), Checks0(Checks), Compared0(ThrownCompared), Threw0(ThrownThrew)
{
	Trace = getenv("EH_TRACE") != NULL && atoi(getenv("EH_TRACE")) != 0;
	Trace2 = Trace && atoi(getenv("EH_TRACE")) >= 2;
	SavedSeed = GetRandomSeed();
	uint32 Seed = 34;
	for (const char *p = Test; *p != 0; p++)
		Seed = Seed * 31 + (uint8)*p;
	if (getenv("EH_SEED") != NULL)
		Seed ^= strtoul(getenv("EH_SEED"), NULL, 10);
	SetRandomSeed(Seed);
	EnableImageExceptions();
}

void TErrorRound::Compare(const char *What, const TThrown &Ours, const TThrown &Original)
{
	if (Trace)
		printf("%s errors %s: %s\n", Test, What, Original.Describe().c_str());
	CompareThrown(Test, What, Ours, Original);
}

void TErrorRound::Finish()
{
	DisableImageExceptions();
	SetRandomSeed(SavedSeed);
	if (getenv("EH_LINES") != NULL) { // the lines seen so far (every test's): our line -> the original's
		for (auto &File : OursToOriginal)
			for (auto &Line : File.second)
				printf("%s errors line: %s %d -> %d\n", Test, File.first.c_str(), Line.first, Line.second);
	}
	printf("%s errors: %d checks, %d exceptions compared (%d calls that threw)\n", Test, Checks - Checks0,
		ThrownCompared - Compared0, ThrownThrew - Threw0);
}
