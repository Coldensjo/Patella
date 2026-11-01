// Differential tests of Help.cpp against the original code.
//
// Needs the Preferences state of preferencestest.cpp (the known hints) and
// redirects the image's _snprintf. Error paths (unknown handles and hint
// numbers) are not exercised.

#include "difftest.h"
#include "../../src/Help.h"
#include "../../src/Preferences.h"
#include <stdarg.h>
#include <set>
#include <string>
#include <iterator>
#include <vector>

static int __cdecl HelpSnprintf(char *Buffer, size_t Count, const char *Format, ...)
{
	va_list Args;
	va_start(Args, Format);
	int Result = _vsnprintf(Buffer, Count, Format, Args);
	va_end(Args);
	return Result;
}

typedef const char *(__cdecl *TextFn)(int);
typedef void (__fastcall *VectorCtorFn)(void *This, void *, int Min, int Max, int Block);
typedef void (__cdecl *VoidFn)();
typedef int (__cdecl *IntFn)();
typedef void (__cdecl *Int2Fn)(int, int);

static void ResetBoth()
{
	SetDefaultPreferences();
	At<VoidFn>(0x4db720)();
	InitHelp();
	At<VoidFn>(0x4a2ae0)();
}

static void CompareQueues(const char *What, int x, int y, int z)
{
	std::vector<int> A, B;
	CHECK(GetEventHelp() == At<IntFn>(0x4a2b00)(), "GetEventHelp after %s (%d,%d,%d)", What, x, y, z);
	for (int Number; (Number = PopEventHelp()) != 0;)
		A.push_back(Number);
	for (int Number; (Number = At<IntFn>(0x4a2b20)()) != 0;)
		B.push_back(Number);
	CHECK(A == B, "event help queue after %s (%d,%d,%d): %d and %d entries", What, x, y, z, (int)A.size(), (int)B.size());
}

void TestHelp()
{
	PatchImage();
	PatchJump(0x51233d, (void *)HelpSnprintf);
	At<VectorCtorFn>(0x4a2b90)((void *)(uintptr_t)0x5ce53c, NULL, 0, 9, 10);

	// Help bubble texts.
	static const int Handles[] = {10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 1000, 1001, 1002,
		1100, 1101, 1102, 1103, 1104, 1105, 1106, 1107, 1108, 1109, 1110, 1111, 1112};
	for (int Handle : Handles) {
		std::string A = GetHelpText(Handle);
		std::string B = At<TextFn>(0x4a2e70)(Handle);
		CHECK(A == B, "GetHelpText(%d): %d and %d characters", Handle, (int)A.size(), (int)B.size());
	}
	for (int Number = 1; Number <= 14; Number++) {
		CHECK(strcmp(GetEventHelpTitle(Number), At<TextFn>(0x4a3b60)(Number)) == 0, "GetEventHelpTitle(%d)", Number);
		CHECK(strcmp(GetEventHelpText(Number), At<TextFn>(0x4a3fd0)(Number)) == 0, "GetEventHelpText(%d)", Number);
	}

	// The help area and handle.
	for (int i = 0; i < 2000; i++) {
		int a = (int)Random() - 0x800000, b = (int)Random() - 0x800000;
		switch (Random() % 4) {
		case 0: SetHelpPosition(a, b); At<Int2Fn>(0x4a2a00)(a, b); break;
		case 1: MoveHelpPosition(a % 100, b % 100); At<Int2Fn>(0x4a2a20)(a % 100, b % 100); break;
		case 2: SetHelpSize(a, b); At<Int2Fn>(0x4a2a50)(a, b); break;
		default: {
			int Handle = (int)(Random() % 5);
			SetHelpHandle(Handle);
			At<void (__cdecl *)(int)>(0x4a2a70)(Handle);
			break;
		}
		}
		int A[4], B[4];
		GetHelpArea(&A[0], &A[1], &A[2], &A[3]);
		At<void (__cdecl *)(int *, int *, int *, int *)>(0x4a2a90)(&B[0], &B[1], &B[2], &B[3]);
		CHECK(memcmp(A, B, sizeof(A)) == 0, "GetHelpArea");
		CHECK(GetHelpHandle() == At<IntFn>(0x4a2ac0)(), "GetHelpHandle");
		if (Random() % 3 == 0)
			CHECK(GetHelpHandleChanged() == At<bool (__cdecl *)()>(0x4a2ad0)(), "GetHelpHandleChanged");
	}

	// Event helps from NPC names.
	static const char *Names[] = {"Cipfried", "Seymour", "Obi", "Norma", "Al Dee", "Tom", "Dixi", "Lily",
		"cipfried", "Tom ", "Al", "", "Dixie", "Lil"};
	for (int Round = 0; Round < 200; Round++) {
		ResetBoth();
		int n = 1 + (int)(Random() % 6);
		for (int i = 0; i < n; i++) {
			const char *Name = Names[Random() % (sizeof(Names) / sizeof(Names[0]))];
			CheckEventHelpName(Name);
			At<void (__cdecl *)(const char *)>(0x4a3460)(Name);
		}
		CompareQueues("CheckEventHelpName", n, 0, 0);
	}

	// Event helps from positions: every bound of the binary's tests and
	// their neighbours, on all floors that occur.
	static const int Bounds[] = {0x7d5d, 0x7d65, 0x7dd5, 0x7cbf, 0x7dc0, 0x7d3f, 0x7e00, 0x7e6d, 0x7e75, 0x7ded,
		0x7df5, 0x7e64, 0x7e6c, 0x7c22, 0x7c2a, 0x81bd, 0x81c5, 0x7c42, 0x7c4a, 0x80b9, 0x80c1, 0x7d48, 0x7d50,
		0x7e57, 0x7e5b, 0x7dda, 0x7e55, 0x7ddc, 0x7de0, 0x8044, 0x8048, 0x8046, 0x7e4e, 0x7e53, 0x7c2b, 0x7c27,
		0x7c29, 0x818e, 0x8192, 0x7c45, 0x808e, 0x7d4b, 0x7d4e, 0x8097, 0x80a3, 0x7d4a, 0x80fc, 0x7d3c, 0x7f85,
		0x7ca3, 0x7f84, 0x7cac, 0x7cad, 0x7fa7, 0x7fac, 0x7fab, 0x7bc3, 0x7bc9, 0x81bc, 0x81c0, 0x7ec8, 0x8164,
		0x8168, 0x8053, 0x8166, 0x8051, 0x8161, 0x816c, 0x804b, 0x804c, 0x8165, 0x804f, 0x7f6d, 0x7f79, 0x7fec,
		0x7e46, 0x8042, 0x7e4f, 0x7e52, 0x7e51, 0x8050};
	std::set<int> Values;
	for (int Bound : Bounds)
		for (int d = -2; d <= 2; d++)
			Values.insert(Bound + d);
	for (int x : Values) {
		for (int y : Values) {
			for (int z = 4; z <= 9; z++) {
				ResetBoth();
				CheckEventHelpPosition(x, y, z);
				At<void (__cdecl *)(int, int, int)>(0x4a3690)(x, y, z);
				// Only compare when something was queued or on a sample, to
				// keep the run short.
				if (GetEventHelp() != 0 || At<IntFn>(0x4a2b00)() != 0 || Random() % 16 == 0)
					CompareQueues("CheckEventHelpPosition", x, y, z);
			}
		}
	}

	// Several positions in a row accumulate in the queue.
	ResetBoth();
	for (int i = 0; i < 3000; i++) {
		auto It = Values.begin();
		std::advance(It, Random() % Values.size());
		int x = *It;
		It = Values.begin();
		std::advance(It, Random() % Values.size());
		int y = *It;
		int z = 5 + (int)(Random() % 4);
		CheckEventHelpPosition(x, y, z);
		At<void (__cdecl *)(int, int, int)>(0x4a3690)(x, y, z);
	}
	CompareQueues("a walk", 0, 0, 0);
	ResetBoth();
}
