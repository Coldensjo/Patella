// Shared helpers of the differential tests (see difftest.cpp).

#pragma once

#include "../../src/common.h"
#include <stdio.h>

extern int Failures;
extern int Checks;

#define CHECK(Cond, ...) do { Checks++; if (!(Cond)) { Failures++; if (Failures <= 20) { printf("FAIL %s:%d: ", __FILE__, __LINE__); printf(__VA_ARGS__); printf("\n"); } } } while (0)

uint32 Random();
void RandomText(char *Text, int Length);

template <class T> static T At(uint32 Address) { return (T)(uintptr_t)Address; }
template <class T> static T &Global(uint32 Address) { return *(T *)(uintptr_t)Address; }

// Overwrites the start of a function in the image with a jump to one of ours.
// The image is a private copy, so this changes nothing on disk.
inline void PatchJump(uint32 Address, void *Target)
{
	uint8 *Code = (uint8 *)(uintptr_t)Address;
	Code[0] = 0xe9;
	*(int32_t *)(Code + 1) = (int32_t)((uintptr_t)Target - (Address + 5));
}

// statustest.cpp: redirects the image's malloc/free and the helpers that
// would reach Windows; safe to call more than once.
void PatchImage();
void SetTestClock(int Value);

void TestSkins();
void TestStatus();
void TestPreferences();
void TestScript();
void PatchScript(); // scripttest.cpp: the image's stdio, remove, rename and sprintf
void TestPreferencesFile();
void TestPlayer();
void TestHelp();
void TestSysinc(); // sysinctest.cpp, in sysinctest.exe only
void TestTalk();
void TestObjects(const char *ExePath); // objectstest.cpp, loads Tibia.dat next to Tibia.exe
