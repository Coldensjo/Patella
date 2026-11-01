// Functions and data from source files that are not reconstructed yet.
// Each entry gives its address in Tibia.exe and where it sits. Move a
// declaration into the real header when its file is reconstructed.

#pragma once

#include "common.h"

// First 16 bytes of the last received packet, at 0x5d1644 (owner unknown).
extern uint8 PacketHeader[16];

// Objects.cpp: a reference to an object (12 bytes, passed by value). Type
// is named in MapWindow.cpp's "SrcObj.Type!=CREATURE" (CREATURE is 99);
// the other field names are ours.
struct TObject {
	int Type;
	int Data1;
	int Data2;
};

// Talk.cpp, line 982: frees the strings of a list (called when the options are reset).
void FUN_00501400();                            // 0x501400

// sysinc.cpp and the gap before it.
int GetMilliseconds();                          // 0x4fcb90, timeGetTime() minus the start time
bool RemoveFile(const char *FileName);          // 0x4fcbc0, true when the file was removed
void SetClipboardText(char *textdata);          // 0x4fd1b0, sysinc.cpp lines 139-143

// Library region: Henry Spencer's regexp ("regexp too big", "corrupted program").
struct regexp;
regexp *regcomp(char *exp);                     // 0x50fa40, the result is malloc'd
int regexec(regexp *prog, char *string);        // 0x50f360, 1 on a match
