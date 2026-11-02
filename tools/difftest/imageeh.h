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

// The image's TMyException object (vtable, Message +0x04, Origin +0x20, both
// VC7.1 std::strings of 0x1c bytes) and its strings.
std::string ImageString(const void *String);
void MakeImageString(void *String, const char *Text); // a VC7.1 string with the image's malloc

// Calls Function(Context) and returns what escaped from the image.
TThrown ImageThrowCall(void (*Function)(void *), void *Context);
template <class F> static void ThrowThunk(void *Context) { (*(F *)Context)(); }
template <class F> TThrown ImageThrow(F Function) { return ImageThrowCall(ThrowThunk<F>, &Function); }

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
