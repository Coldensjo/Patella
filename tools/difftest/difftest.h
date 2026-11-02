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
void TestInventory();
void TestSysinc(); // sysinctest.cpp, in sysinctest.exe only
void TestTalk();
void TestObjects(const char *ExePath); // objectstest.cpp, loads Tibia.dat next to Tibia.exe
void TestCreatures(const char *ExePath); // creaturestest.cpp, loads Tibia.dat next to Tibia.exe
void TestGUI();
void TestMap(const char *ExePath);     // maptest.cpp, also loads Tibia.dat
void TestGraphics(const char *ExePath); // graphicstest.cpp, loads Tibia.spr, Tibia.pic and Tibia.dat
void TestDialogWindow();                 // dialogwindowtest.cpp, after TestGUI (restores its patches)
void TestContextMenus();                 // contextmenustest.cpp, after TestDialogWindow (restores its patches)
void TestConsoleWindow();                // consolewindowtest.cpp, after TestContextMenus (restores its patches)
void TestContentWindows();               // contentwindowstest.cpp, after TestConsoleWindow (restores its patches)
void TestDialogElements();               // dialogelementstest.cpp, after TestContentWindows (restores its patches)
void TestDialogs();                      // dialogstest.cpp, after TestDialogElements (restores its patches)
void TestSurfaceDX9();                   // surfacedx9test.cpp, before TestMapWindow (restores its patches)
void TestMapWindow(const char *ExePath); // mapwindowtest.cpp, loads Tibia.dat

// dialogwindowtest.cpp defines the shared fakes of the classes DialogWindow.cpp
// uses from files that are not reconstructed (TSubWindowBody's members,
// TResizer, TSkinButton::SetSkins); each test that uses them sets these hooks.
class TComponent;
class TResizer;
class TSkinButton;
class TSubWindowBody;
struct TDialogFakes {
	void (*Body)(TSubWindowBody *Body, const char *Call);   // the test bodies' Minimize ("A0"), Maximize ("A4"), the container body's UpdateUpButton ("Up")
	bool (*BodyAccepts)(TSubWindowBody *Body);              // unused
	void (*NewResizer)(TResizer *Resizer, TComponent *ToResize, bool Param2, bool Param3, bool Param4);
	void (*SetSkins)(TSkinButton *Button, int ButtonUp, int ButtonDown);
};
extern TDialogFakes DialogFakes;
// dialogwindowtest.cpp: the fake of the container body's UpdateUpButton
// (0x42e4e0, called by TSubWindow::Arrange); dialogwindowtest.cpp and
// mainwindowtest.cpp redirect the image's and ours to it.
void __fastcall FakeUpdateUpButton(void *Body, void *);
// contentwindowstest.cpp: while it is set (that test), the shared fakes of the
// DialogElements.cpp classes (the constructors in consolewindowtest.cpp,
// contextmenustest.cpp, mainwindowtest.cpp and contentwindowstest.cpp)
// report every object they make, with its kind and the arguments.
extern void (*ElementCreated)(void *Object, const char *Kind, const char *Arguments);
void ReportElement(void *Object, const char *Kind, const char *Format, ...);
void TestMainWindow();                   // mainwindowtest.cpp, after TestMapWindow (patches for good as well)
