// Functions and data from source files that are not reconstructed yet.
// Each entry gives its address in Tibia.exe and where it sits. Move a
// declaration into the real header when its file is reconstructed.

#pragma once

#include "common.h"

// Between Player.cpp and Preferences.cpp. Returns the "Test Server Login" option (0x71c318).
bool GetTestServerLogin();                      // 0x4db2a0

// Between Script.cpp and Status.cpp. Connection state at 0x71c588: 0 offline,
// 1 resolving the login server name, 2 connecting to a login server,
// 4 talking to the login server, 5 connecting to the game server; the
// game uses higher values (see ReceivePackets in Control.cpp).
void SetConnectionState(int State);             // 0x4e71e0
int GetConnectionState();                       // 0x4e71f0
int GetActiveCharacter();                       // 0x4e72c0, name from an assertion string

// Status.cpp: the character list (0x54-byte entries at 0x71c54c).
const char *GetCharacterIPAddress(int Number);  // 0x4ea090, line 740
int GetCharacterPort(int Number);               // 0x4ea200, line 749

// Status.cpp, 63 callers. (1, title, text) shows a message box and takes
// ownership of text; (10, 300, 0) and (0x2b, 0, 0) are other events.
void FUN_004e9020(int Type, INT_PTR Param1, INT_PTR Param2); // 0x4e9020

// First 16 bytes of the last received packet, at 0x5d1644 (owner unknown).
extern uint8 PacketHeader[16];
