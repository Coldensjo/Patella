// Differential tests of sysinc.cpp against the original code.
//
// Built into its own executable (sysinctest.exe, difftest.cpp with
// DIFFTEST_SYSINC): the main difftest replaces GetMilliseconds, RemoveFile
// and SetClipboardText with recorders, which would clash with sysinc.cpp.
// Only the PROGRAM_ERROR texts are pure; the rest needs the clipboard, the
// file system or the original's uninitialised CRT (srand).

#include "difftest.h"
#include "../../src/sysinc.h"

typedef const char *(__cdecl *TextFn)(int);

void TestSysinc()
{
	for (int Error = -3; Error < 30; Error++) {
		const char *A = GetErrorText(Error);
		const char *B = At<TextFn>(0x4fcbf0)(Error);
		CHECK(strcmp(A, B) == 0, "GetErrorText(%d) \"%s\", original \"%s\"", Error, A, B);
		A = GetErrorHint(Error);
		B = At<TextFn>(0x4fcc90)(Error);
		CHECK(strcmp(A, B) == 0, "GetErrorHint(%d) \"%s\", original \"%s\"", Error, A, B);
	}
	CHECK(At<TextFn>(0x4fcbf0)(1000000) == At<TextFn>(0x4fcbf0)(-1000000), "default texts");
}
