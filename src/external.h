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

// --- added for Communication.cpp ---

// Temporary binary-layout views, to be moved into Creatures.cpp/Map.cpp/Talk.cpp headers.
struct TCommunicationObject {
	int Type;
	uint32 Data;
	uint32 Extra;
};

struct TCommunicationCreature {
	uint32 CreatureID;                 // +0x00
	char Name[32];                     // +0x04
	int RealX;                         // +0x24
	int RealY;                         // +0x28
	int RealZ;                         // +0x2c
	int DeltaX;                        // +0x30
	int DeltaY;                        // +0x34
	uint8 Unknown38[24];
	int Direction;                     // +0x50
	uint8 Unknown54[12];
	int OutfitType;                    // +0x60
	int Colors[4];                     // +0x64
	int LightStrength;                 // +0x74
	int LightColor;                    // +0x78
	uint8 Unknown7c[8];
	int Health;                        // +0x84
	int Speed;                         // +0x88
	bool Visible;                      // +0x8c
	uint8 Unknown8d[3];
	int Skull;                         // +0x90
	int Shield;                        // +0x94

	void FUN_0044b820(int Skull, bool Delay);           // 0x44b820, Creatures.cpp (leading gap)
	void FUN_0044bc70(int dx, int dy, int GroundSpeed); // 0x44bc70, Creatures.cpp (leading gap)
	void FUN_0044bf20(int Mark);                        // 0x44bf20, Creatures.cpp
};

struct TCommunicationStatement {
	uint8 Unknown00[20];
	char Name[30];                     // +0x14
};

struct TCommunicationChannel {
	void **VTable;                     // +0x00, close at +0x10, remove statement at +0x20
	uint8 Unknown04[12];
	TCommunicationStatement *First;    // +0x10
	uint32 Unknown14;
	TCommunicationStatement *Cursor;   // +0x18

	TCommunicationStatement *FUN_00503920(); // 0x503920, Talk.cpp; EAX is the advanced cursor
};

int FUN_0044b870(); // 0x44b870, Control.cpp|Creatures.cpp
void FUN_0044b880(int *x, int *y, int *z); // 0x44b880, Control.cpp|Creatures.cpp
void FUN_0044b8c0(); // 0x44b8c0, Control.cpp|Creatures.cpp
void FUN_0044b8e0(); // 0x44b8e0, Control.cpp|Creatures.cpp
void FUN_0044b900(int Beat); // 0x44b900, Control.cpp|Creatures.cpp
bool FUN_0044b910(uint32 CreatureID); // 0x44b910, Control.cpp|Creatures.cpp
void FUN_0044b920(uint32 CreatureID); // 0x44b920, Control.cpp|Creatures.cpp
void FUN_0044b930(int Health, int MaxHealth); // 0x44b930, Control.cpp|Creatures.cpp
void FUN_0044b950(int Level, int Percent); // 0x44b950, Control.cpp|Creatures.cpp
void FUN_0044b970(int Mana, int MaxMana); // 0x44b970, Control.cpp|Creatures.cpp
void FUN_0044b990(int Soul); // 0x44b990, Control.cpp|Creatures.cpp
void FUN_0044b9a0(int Capacity); // 0x44b9a0, Control.cpp|Creatures.cpp
void FUN_0044b9b0(int Icons); // 0x44b9b0, Control.cpp|Creatures.cpp
uint32 FUN_0044b9c0(); // 0x44b9c0, Control.cpp|Creatures.cpp
void FUN_0044bb00(uint32 CreatureID); // 0x44bb00, Control.cpp|Creatures.cpp
void FUN_0044bb20(uint32 CreatureID); // 0x44bb20, Control.cpp|Creatures.cpp
uint32 FUN_0044bb50(); // 0x44bb50, Control.cpp|Creatures.cpp
uint32 FUN_0044bb60(); // 0x44bb60, Control.cpp|Creatures.cpp
int FUN_0044c3c0(int Step); // 0x44c3c0, Creatures.cpp
TCommunicationCreature *FUN_0044c530(uint32 RemoveID, uint32 CreatureID); // 0x44c530, Creatures.cpp
TCommunicationCreature *FUN_0044c6e0(uint32 CreatureID); // 0x44c6e0, Creatures.cpp
void FUN_0044ca60(int Skull); // 0x44ca60, Creatures.cpp
void FUN_0044cc00(uint32 Experience, int Level, int Percent); // 0x44cc00, Creatures.cpp
void FUN_0044cf10(int Skill, int Level, int Percent); // 0x44cf10, Creatures.cpp
void FUN_0044eab0(uint32 CharacterID, const char *Name, bool Online); // 0x44eab0, Creatures.cpp
void FUN_0044f000(uint32 CharacterID, bool Online); // 0x44f000, Creatures.cpp
void FUN_004516e0(); // 0x4516e0, Creatures.cpp
bool FUN_004518b0(int Direction); // 0x4518b0, Creatures.cpp
void FUN_00451db0(TCommunicationCreature *Creature); // 0x451db0, Creatures.cpp
void FUN_00452320(int x, int y, int z); // 0x452320, Creatures.cpp
void FUN_004a3690(int x, int y, int z); // 0x4a3690, Help.cpp
void FUN_004a4520(); // 0x4a4520, Help.cpp|Inventory.cpp
void FUN_004a4550(int Position, TCommunicationObject Object); // 0x4a4550, Inventory.cpp
void FUN_004a46d0(int Position); // 0x4a46d0, Inventory.cpp
void FUN_004a4b00(int Container, TCommunicationObject Object, const char *Name, int Capacity, bool HasParent); // 0x4a4b00, Inventory.cpp
void FUN_004a4f70(int Container); // 0x4a4f70, Inventory.cpp
bool FUN_004a5220(int Container); // 0x4a5220, Inventory.cpp
void FUN_004a5380(int Container, TCommunicationObject Object); // 0x4a5380, Inventory.cpp
void FUN_004a5630(int Container, int Position, TCommunicationObject Object); // 0x4a5630, Inventory.cpp
bool FUN_004a58e0(int Container, int Position); // 0x4a58e0, Inventory.cpp
void FUN_004a6eb0(const char *Name); // 0x4a6eb0, Inventory.cpp
bool FUN_004a7230(const char *Name); // 0x4a7230, Inventory.cpp
void FUN_004a7460(TCommunicationObject Object); // 0x4a7460, Inventory.cpp
void FUN_004a75e0(TCommunicationObject Object); // 0x4a75e0, Inventory.cpp
void FUN_004a7f00(int Container, TCommunicationObject Object); // 0x4a7f00, Inventory.cpp
void FUN_004b08a0(int x, int y, int z); // 0x4b08a0, MainWindow.cpp|Map.cpp
void FUN_004b08c0(int *x, int *y, int *z); // 0x4b08c0, MainWindow.cpp|Map.cpp
int FUN_004b08f0(); // 0x4b08f0, MainWindow.cpp|Map.cpp
void FUN_004b0910(int *x, int *y, int *z); // 0x4b0910, MainWindow.cpp|Map.cpp
void FUN_004b0960(int *x, int *y, int *z); // 0x4b0960, MainWindow.cpp|Map.cpp
void FUN_004b09e0(int Brightness, int Color); // 0x4b09e0, MainWindow.cpp|Map.cpp
TCommunicationObject FUN_004b6140(int rx, int ry, int rz); // 0x4b6140, Map.cpp
TCommunicationObject FUN_004b6510(); // 0x4b6510, Map.cpp
TCommunicationObject FUN_004b6580(int rx, int ry, int rz, int Position); // 0x4b6580, Map.cpp
void FUN_004b88f0(int rx, int ry, int rz, TCommunicationObject Object); // 0x4b88f0, Map.cpp
void FUN_004b8ec0(int rx, int ry, int rz, TCommunicationObject Object); // 0x4b8ec0, Map.cpp
bool FUN_004b9420(int rx, int ry, int rz, int Position); // 0x4b9420, Map.cpp
void FUN_004b9940(int rx, int ry, int rz, int Position, TCommunicationObject Object); // 0x4b9940, Map.cpp
void FUN_004bab40(int rx, int ry, int rz, int Effect); // 0x4bab40, Map.cpp
void FUN_004bad00(int rx, int ry, int rz, int Color, const char *Text); // 0x4bad00, Map.cpp
void FUN_004bb0c0(int sx, int sy, int sz, int dx, int dy, int dz, int Effect); // 0x4bb0c0, Map.cpp
void FUN_004bc720(int rx, int ry, int rz, bool KeepCreatures); // 0x4bc720, Map.cpp
void FUN_004bccb0(); // 0x4bccb0, Map.cpp
void FUN_004bd560(int dx, int dy); // 0x4bd560, Map.cpp
void FUN_004bdb70(int dz); // 0x4bdb70, Map.cpp
void FUN_004bdd70(int rx, int ry, int rz); // 0x4bdd70, Map.cpp
void FUN_004beb90(int rx, int ry); // 0x4beb90, Map.cpp
bool FUN_004d1c40(int ObjectType, int Flag); // 0x4d1c40, Objects.cpp
int FUN_004d1f30(int ObjectType); // 0x4d1f30, Objects.cpp
bool FUN_004e3100(const char *Name, const uint8 *Data, int Size, uint32 Signature); // 0x4e3100, Script.cpp
bool FUN_004e3500(const char *Name, const uint8 *Data, int Size); // 0x4e3500, Script.cpp
void FUN_004fd4c0(); // 0x4fd4c0, sysinc.cpp|Talk.cpp
void FUN_004fd540(); // 0x4fd540, sysinc.cpp|Talk.cpp
void FUN_004fd590(); // 0x4fd590, sysinc.cpp|Talk.cpp
void FUN_004ff500(const char *Text, bool Permanent); // 0x4ff500, Talk.cpp
bool FUN_00500060(int Mode, const char *Name, int Channel); // 0x500060, Talk.cpp
TCommunicationChannel *FUN_005025c0(int Type, const char *Name, int ChannelID); // 0x5025c0, Talk.cpp
void FUN_005052e0(int Mode, const char *Name, int x, int y, int z, const char *Text); // 0x5052e0, Talk.cpp
void FUN_00505ac0(int Mode, const char *Text); // 0x505ac0, Talk.cpp
void FUN_00505f10(TCommunicationChannel *Channel, uint32 StatementID, const char *Text, int Mode, const char *Name, int Time); // 0x505f10, Talk.cpp
void FUN_005061f0(uint32 StatementID, const char *Text, int Mode, const char *Name, int ChannelID, int Time); // 0x5061f0, Talk.cpp
TCommunicationChannel *FUN_00507bc0(int Type, bool Open, const char *Name, int ChannelID); // 0x507bc0, Talk.cpp

bool FieldRefreshRequested(int rx, int ry, int rz); // 0x4be750, Map.cpp
extern uint32 DAT_005cc758; // 0x5cc758, graphics.cpp; Tibia.pic signature
extern uint32 DAT_005ccb64; // 0x5ccb64, graphics.cpp; Tibia.spr signature
extern uint32 DAT_00719d9c; // 0x719d9c, Objects.cpp; Tibia.dat signature
extern int DAT_00719da0; // 0x719da0, Map.cpp/Inventory.cpp shared empty-object type; owner uncertain
extern const char *PTR_s_Your_request_has_been_closed__005c0348; // 0x5c0348, Talk.cpp
extern const char *PTR_s_The_channel_has_been_closed__You_005c034c; // 0x5c034c, Talk.cpp
