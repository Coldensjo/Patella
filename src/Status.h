// Status.cpp: the client's shared state. Account, character list and
// connection state; the message queue that the network code and the GUI use
// to talk to the main loop; mouse and drag state; the player's rights;
// error reports read from error.txt; the clipboard link check.
//
// Names are ours unless a comment says they come from the binary.

#pragma once

#include "common.h"
#include "Container.h"
#include "Objects.h" // TObject

// "In(rightid,0,RIGHT_ANY-1)"; CheckRight treats RIGHT_ANY as "any right".
#define RIGHT_ANY 50

// "In(Type,MESSAGE_NONE,MESSAGE_STARTPATCHER)". Other values seen: 1 shows
// a message box (Param1 title, Param2 text, which the queue owns), 10 with
// Param1 300 follows a failed game server connection.
#define MESSAGE_NONE 0
#define MESSAGE_STARTPATCHER 72

// One queued message (12 bytes). Type is named in the assertions.
struct TMessage {
	int Type;
	INT_PTR Param1;
	INT_PTR Param2;
};

// One entry of the character list (0x54 bytes). The 30-byte limits come
// from "strlen(Name)<30" and "strlen(World)<30".
struct TCharacter {
	char Name[30];       // +0x00
	char World[30];      // +0x1e
	uint32 IP;           // +0x3c, as received
	char IPAddress[16];  // +0x40, dotted text
	int Port;            // +0x50
};

// One error report from error.txt (0xa5a bytes, byte aligned). Date is
// 26 bytes, the size of an asctime() string.
struct TErrorReport {
	char Title[64];      // +0x000
	char Date[26];       // +0x040
	char Text[2048];     // +0x05a, lines separated by '\n'
	char Comment[512];   // +0x85a, "Comment: " and the player's text

	TErrorReport()
	{
		memset(Title, 0, sizeof(Title));
		memset(Date, 0, sizeof(Date));
		memset(Text, 0, sizeof(Text));
		memset(Comment, 0, sizeof(Comment));
	}
};

bool CheckAnyRight();
void ClearRights();
int GetActionNumber();
void IncrementActionNumber();
// The action state of no action (name from MainWindow.cpp's
// "GetActionState()==ACTIONSTATE_NONE"; the other states are numbers: 1 left
// button down, 2 right button down, 3 both down, 4 and 5 one of both
// released, 7 and 8 set by events 0x2c and 0x2d, 9 help mode, 10 dialog
// open, 11 context menu open).
#define ACTIONSTATE_NONE 0
void SetActionState(int State);
int GetActionState();
void SetMousePosition(int x, int y);
int GetMouseX();
int GetMouseY();
void SetMouseCursor(int Cursor);
int GetMouseCursor();
void SetPendingAction(int State, int Number);
int GetPendingActionState();
int GetPendingActionNumber();
void SetWindowSize(int Width, int Height);
int GetWindowWidth();
int GetWindowHeight();
void SetDragStart(int x, int y);
void SetDragObject(TObject Obj, int Index, int x, int y, int z);
void SetDragTarget(int x, int y, int z);
void CheckDragStart(int x, int y);
TObject *GetDragObject(int *Index, int *x, int *y, int *z);
void GetDragTarget(int *x, int *y, int *z);
void SetTradeObject(TObject Obj, int Index, int x, int y, int z);
void SetUseObject(TObject Obj, int Index, int x, int y, int z);
void SetTradeInfo(int Info);
TObject *GetTradeObject(int *Index, int *x, int *y, int *z);
TObject *GetUseObject(int *Index, int *x, int *y, int *z);
int GetTradeInfo();
void SetConnectionState(int State);
int GetConnectionState();
void SetProgress(int Progress);
int GetProgress();
int GetAccountNumber();
const char *GetAccountNumberString();
const char *GetAccountPassword();
void SetAccountCharacters(int Number);
void SetPremiumDays(int Days);
int GetAccountCharacters();
int GetPremiumDays();
int GetActiveCharacter();
bool GetGamemaster();
void SetMouseOverText(const char *Text);
void SetMouseOverTextPosition(int x, int y);
const char *GetMouseOverText();
void GetMouseOverTextPosition(int *x, int *y);
void UpdateBlinkState();
bool GetBlinkState();
void DeleteErrorFile();
int PeekMessageType();
void FetchMessage(int *Type, INT_PTR *Param1, INT_PTR *Param2);
void ClearMessages();
void ClearErrorReports();
void AddErrorReport(const char *Title);
void SetErrorReportDate(const char *Date);
void AddErrorReportText(const char *Text);
void SetErrorReportComment(const char *Comment);
int GetNumberOfErrorReports();
const char *GetErrorReportTitle(int Number);
const char *GetErrorReportDate(int Number);
const char *GetErrorReportText(int Number);
const char *GetErrorReportComment(int Number);
void ExitStatus();
void SetRight(int rightid, int Value);
bool CheckRight(int rightid, int Bit);
const char *GetRightName(int rightid);
const char *GetActionName(int actionid);
void AddMessage(int Type, INT_PTR Param1, INT_PTR Param2);
void AddMessageFirst(int Type, INT_PTR Param1, INT_PTR Param2);
void SetAccount(int Number, const char *Password);
void SetCharacter(int Number, const char *Name, const char *World, const uint8 *Address, int Port);
void SetActiveCharacter(int Number);
const char *GetCharacterName(int Number);
const char *GetCharacterWorld(int Number);
const char *GetCharacterIPAddress(int Number);
int GetCharacterPort(int Number);
void CheckClipboardText(const char *Text, int Length);
void CopyToClipboard(const char *text);
void CopyTaintedClipboardText();
void InitStatus(bool Gamemaster, void (*linkwarningeventhandler)());
