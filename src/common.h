// Shared definitions for the reconstructed Tibia 7.72 client.
//
// The error macros reproduce the client's error-trace pattern as seen in the
// binary. Their names are ours; the strings they produce (expression text,
// "assertion failed", "exception occurred", "unknown exception occurred",
// file and line) match the original.

#pragma once

#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#ifdef TIBIA_MFC_SHIM
#include "compat/mfc_shim.h"
#else
#include <afxwin.h>
#include <afxsock.h>
#endif
#include <string>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;

// Range check used in many assertions, e.g. "In(CommandType,0,255)".
inline bool In(int Value, int Min, int Max)
{
	return Value >= Min && Value <= Max;
}

// Keeps *Value between Min and Max, in either order. The name is ours. The
// out-of-line copy 0x425920 is ContentWindows.obj's (TTradeWindowBody::SetSize
// inlines it); DialogElements.cpp calls it (0x45d8a3 and four more).
inline void Limit(int *Value, int Min, int Max)
{
	if (Min > Max) {
		if (*Value < Max)
			*Value = Max;
		if (*Value > Min)
			*Value = Min;
	} else {
		if (*Value < Min)
			*Value = Min;
		if (*Value > Max)
			*Value = Max;
	}
}

// Thrown by value (ThrowInfo at 0x57b3b4).
#include "TMyException.h"

// The client's second exception type: an error code thrown by value
// (ThrowInfo 0x57b3c4, ".?AW4PROGRAM_ERROR@@"). The enumerator names are
// unknown; values seen so far: 6 (AfxSocketInit failed, InitNetwork).
enum PROGRAM_ERROR {
	PROGRAM_ERROR_SOCKET_INIT = 6, // our name
};

#define TMYEXCEPTION(Kind, N1, V1, N2, V2, Last) \
	TMyException(__FILE__, __LINE__, Kind, N1, (int)(V1), N2, (int)(V2), Last)

// Assertions record the expression and up to two values with their source text.
#define TASSERT(Expr) \
	if (!(Expr)) throw TMYEXCEPTION("assertion failed", "", 0, "", 0, #Expr)
#define TASSERT1(Expr, V1) \
	if (!(Expr)) throw TMYEXCEPTION("assertion failed", #V1, V1, "", 0, #Expr)
#define TASSERT2(Expr, V1, V2) \
	if (!(Expr)) throw TMYEXCEPTION("assertion failed", #V1, V1, #V2, V2, #Expr)

#define TERROR(Message) \
	throw TMYEXCEPTION(Message, "", 0, "", 0, "")
#define TERROR1(Message, V1) \
	throw TMYEXCEPTION(Message, #V1, V1, "", 0, "")
#define TERROR2(Message, V1, V2) \
	throw TMYEXCEPTION(Message, #V1, V1, #V2, V2, "")
// An error with a text as its reason (Script.cpp: the file name), in the
// place of an assertion's expression: "<Message>, reason:\n<Reason>".
#define TERROR_REASON(Message, Reason) \
	throw TMYEXCEPTION(Message, "", 0, "", 0, Reason)
#define TERROR1_REASON(Message, V1, Reason) \
	throw TMYEXCEPTION(Message, #V1, V1, "", 0, Reason)

// Most functions wrap their body in TBEGIN ... TEND. The three handlers are
// visible in the binary as catch blocks after each function: a PROGRAM_ERROR
// passes through unchanged, a TMyException is wrapped with the listed
// parameters, and anything else becomes "unknown exception occurred".
#define TBEGIN try {
#define TEND_HANDLERS(N1, V1, N2, V2) \
	} catch (PROGRAM_ERROR Error_) { \
		throw Error_; \
	} catch (TMyException &Exception_) { \
		throw TMYEXCEPTION("exception occurred", N1, V1, N2, V2, Exception_); \
	} catch (...) { \
		throw TMYEXCEPTION("unknown exception occurred", N1, V1, N2, V2, ""); \
	}
#define TEND TEND_HANDLERS("", 0, "", 0)
#define TEND1(V1) TEND_HANDLERS(#V1, V1, "", 0)
#define TEND2(V1, V2) TEND_HANDLERS(#V1, V1, #V2, V2)
