// Exceptions of the original code in the difftest child (issue #22).
//
// EnableImageExceptions makes the image's own C++ exception machinery work:
// its throw (_CxxThrowException, RaiseException), its frame handler
// (__CxxFrameHandler, catch blocks, unwinding), and the C++ library code its
// TMyException constructors need (std::stringstream with its locale). The
// image's code then throws, catches and wraps (TEND) as in the client;
// ImageThrow catches what leaves the image at the boundary and converts it
// into a TThrown, OurThrow does the same for the reconstructed code.

#pragma once

#include "difftest.h"
#include <map>
#include <string>

// What a call threw, comparable between both sides.
struct TThrown {
	enum TKind { NOTHING, MYEXCEPTION, PROGRAMERROR, OTHER } Kind;
	std::string Type;    // the decorated name of the thrown type, ".?AVTMyException@@"
	std::string Message; // TMyException::GetMessage()
	std::string Origin;  // TMyException::GetOrigin()
	int Code;            // the PROGRAM_ERROR value
	TThrown() : Kind(NOTHING), Code(0) {}
	bool operator==(const TThrown &Other) const
	{
		return Kind == Other.Kind && Type == Other.Type && Message == Other.Message && Origin == Other.Origin
			&& Code == Other.Code;
	}
	bool operator!=(const TThrown &Other) const { return !(*this == Other); }
	std::string Describe() const; // for failure reports
};

// Binds the imports the image's exception and iostream code calls, patches its
// locks and per-thread data; DisableImageExceptions undoes every change.
void EnableImageExceptions();
void DisableImageExceptions();

// Patches that DisableImageExceptions undoes as well: a jump at the start of
// an image function, a jump at the start of one of our functions, and an
// entry of difftest.exe's import table.
void PatchImageJump(uint32 Address, void *Target);
void PatchOwnCode(void *Function, void *Target);
void PatchOwnImport(const char *Dll, const char *Name, void *Target);
void BindImageImport(const char *Dll, const char *Name, void *Target); // an import slot of the image

// The image's TMyException object (vtable, Message +0x04, Origin +0x20, both
// VC7.1 std::strings of 0x1c bytes) and its strings.
std::string ImageString(const void *String);
void MakeImageString(void *String, const char *Text); // a VC7.1 string with the image's malloc
void FreeImageString(void *String);

// Calls Function(Context) and returns what escaped from the image.
TThrown ImageThrowCall(void (*Function)(void *), void *Context);
template <class F> static void ThrowThunk(void *Context) { (*(F *)Context)(); }
template <class F> TThrown ImageThrow(F Function) { return ImageThrowCall(ThrowThunk<F>, &Function); }

// Compares what both sides threw (errortest.cpp's comparison, shared since
// issue #34): kind, type, PROGRAM_ERROR value, message and origin exactly,
// except the line numbers. Every "<file> <line>: " that starts the message or
// follows ", reason:\n", and the origin's, is compared with the line replaced,
// and over the whole run each of our lines must stand for one original line
// and each original line for one of ours (one map for all tests). Failures
// are reported as "<Test> <What>: ...". One check per call and one per line
// pair; ThrownCompared and ThrownThrew count the calls and those in which the
// original threw.
void CompareThrown(const char *Test, const char *What, const TThrown &Ours, const TThrown &Original);
extern int ThrownCompared, ThrownThrew;
typedef std::map<std::string, std::map<int, int>> TLineMap;
const TLineMap &ThrownLineMap(); // file -> our line -> the original's line

// The error round of a window or surface test (issue #34): the constructor
// enables the image's exceptions, Compare compares one pair of calls,
// Finish disables them again and prints "<Test> errors: N checks, M
// exceptions compared (K calls that threw)". Its checks are counted in
// Checks like every other, reported apart from the test's own count.
// Random() runs from a seed of the round's own (EH_SEED=<n> changes it) and
// gets its old state back in Finish: the round may use the test's random
// world builders, and the later tests' inputs stay the same.
// EH_TRACE=1 prints every comparison.
struct TErrorRound {
	const char *Test;
	int Checks0, Compared0, Threw0;
	bool Trace, Trace2;
	uint32 SavedSeed;
	explicit TErrorRound(const char *Test);
	void Compare(const char *What, const TThrown &Ours, const TThrown &Original);
	void Calling(const char *What, const char *Side)
	{
		if (Trace2)
			printf("%s errors: calling %s %s\n", Test, Side, What);
	}
	void Finish();
};
// The two sides run one after the other (ours first); EH_TRACE=2 names each
// call before it runs.
#define ROUND_COMPARE(Round, What, OurCall, ImageCall) \
	do { \
		(Round).Calling(What, "ours"); \
		TThrown Ours_ = OurThrow([&] { OurCall; }); \
		(Round).Calling(What, "the original"); \
		(Round).Compare(What, Ours_, ImageThrow([&] { ImageCall; })); \
	} while (0)

// A function that starts with a jump now is a test's fake (earlier tests
// redirect some functions for good): such comparisons are skipped.
bool Redirected(const void *Code);
bool ImageRedirected(uint32 Address);

// The same for our code: catches PROGRAM_ERROR, TMyException and the rest.
TThrown ThrownOf(const TMyException &Exception);
template <class F> TThrown OurThrow(F Function)
{
	TThrown Result;
	try {
		Function();
	} catch (PROGRAM_ERROR Error) {
		Result.Kind = TThrown::PROGRAMERROR;
		Result.Type = ".?AW4PROGRAM_ERROR@@";
		Result.Code = (int)Error;
	} catch (TMyException &Exception) {
		Result = ThrownOf(Exception);
	} catch (...) {
		Result.Kind = TThrown::OTHER;
	}
	return Result;
}
