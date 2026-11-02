// Differential tests of ContentWindows.cpp against the original code.
//
// Every round builds one kind of window on both sides, from the image's code
// and from ours: the inventory and the small inventory, the containers, the
// skill, battle, trade and VIP windows (the creators), the automap, the
// status bars and the buttons window (the constructors). Each lives in a
// column (DialogWindow.cpp's TDialogWindow, the image's or ours) as in the
// game. Both get the same random operations: sizes, Arrange, drawing, mouse
// presses, releases, double and right clicks with and without modifiers, the
// wheel, hovering and help mode at random points and over the slots, rows and
// icons, the windows' events and the members of the classes (Scroll, GetMark,
// GetSlotAt, GetIconAt, GetIcon, GetLine, GetOpponentAt, GetBuddyAt,
// GetOfferAt, Update, UpdateUpButton, the trade buttons), and changes of the
// world the windows show. After each one the logs of every callee and a dump
// of both window trees (geometry, drawing lists, captures and the members of
// every class, the DialogElements objects with their arguments and state) are
// compared; at the end both columns are deleted and the logs compared again.
//
// TComponent, TElement, TWindow, TCallback and DialogWindow.cpp's classes are
// the image's for the original objects and ours for ours. The
// DialogElements.cpp classes are the shared fakes (consolewindowtest.cpp,
// contextmenustest.cpp, mainwindowtest.cpp, dialogwindowtest.cpp and the new
// ones below) on both sides: the image's constructors and members jump to
// them; they report every object to this test (difftest.h's ElementCreated),
// which gives it a test vtable. Every other callee (the player's data, the
// inventory, the creatures, the preferences, the map, the objects' flags,
// Help.cpp, the hotkeys, the text and picture drawing, the surface and
// Communication.cpp's Send functions) is a logging fake answering from a
// random world, for both sides. The image's operator new and ours fill new
// memory with the same byte pattern, so uninitialised members agree. Failing
// assertions are not exercised: the inputs keep to what the game does (the
// trade buttons get the counter offer only while they wait, positions are
// inside the windows).

#include "difftest.h"
#include "elementfakes.h"
#include "../../src/ContentWindows.h"
#include "../../src/Communication.h"
#include "../../src/ContextMenus.h"
#include "../../src/Creatures.h"
#include "../../src/GUI.h"
#include "../../src/Help.h"
#include "../../src/Inventory.h"
#include "../../src/Map.h"
#include "../../src/Objects.h"
#include "../../src/Preferences.h"
#include "../../src/Skins.h"
#include "../../src/Status.h"
#include "../../src/Surface.h"
#include "testsurface.h"
#include "../../src/Talk.h"
#include "../../src/Utils.h"
#include "../../src/graphics.h"
#include <map>
#include <new>
#include <stdarg.h>
#include <string>
#include <vector>

static_assert(sizeof(TButtonsWindow) == 0x58 && offsetof(TButtonsWindow, LogoutButton) == 0x54, "TButtonsWindow layout");
static_assert(sizeof(TAutomapView) == 0x24 && sizeof(TCompassRose) == 0x28, "automap parts");
static_assert(sizeof(TAutomapWindow) == 0x80 && offsetof(TAutomapWindow, Changed) == 0x60
	&& offsetof(TAutomapWindow, ViewX) == 0x64 && offsetof(TAutomapWindow, Zoom) == 0x7c, "TAutomapWindow layout");
static_assert(sizeof(TCapacityLabel) == 0x24 && sizeof(TSoulLabel) == 0x24, "labels");
static_assert(sizeof(TPlayerStates) == 0x2c && offsetof(TPlayerStates, MaxIcons) == 0x28, "TPlayerStates layout");
static_assert(sizeof(TSmallInventoryWindow) == 0x44 && sizeof(TInventoryView) == 0x2c, "inventory windows");
static_assert(sizeof(TCombatControls) == 0x44 && offsetof(TCombatControls, SecureButton) == 0x40, "TCombatControls layout");
static_assert(sizeof(TStatusWindow) == 0x4c, "TStatusWindow layout");
static_assert(sizeof(TSubWindowBody) == 0x30 && offsetof(TSubWindowBody, Minimized) == 0x2c, "TSubWindowBody layout");
static_assert(sizeof(TContainerWindowBody) == 0x44 && offsetof(TContainerWindowBody, HasParent) == 0x40,
	"TContainerWindowBody layout");
static_assert(sizeof(TSkillsWindowBody) == 0x3c && offsetof(TSkillsWindowBody, Lines) == 0x38, "TSkillsWindowBody layout");
static_assert(sizeof(TBattleWindowBody) == 0x58 && offsetof(TBattleWindowBody, MouseOverText) == 0x38,
	"TBattleWindowBody layout");
static_assert(sizeof(TBuddylistWindowBody) == 0x3c && offsetof(TBuddylistWindowBody, MouseOverText) == 0x38,
	"TBuddylistWindowBody layout");
static_assert(sizeof(TTradeWindowBody) == 0x3c && sizeof(TTradeOffers) == 0x34 && sizeof(TTradeNames) == 0x38
	&& sizeof(TTradeButtons) == 0x4c && offsetof(TTradeButtons, Callback) == 0x44, "trade layouts");
static_assert(sizeof(TRadioButtonGroup) == 0x28 && offsetof(TRadioButtonGroup, Count) == 0x24, "TRadioButtonGroup layout");
static_assert(offsetof(TSkinButton, RadioButtonGroup) == 0x38 && sizeof(TTextToggleButton) == 0x84
	&& offsetof(TTextToggleButton, State) == 0x78 && sizeof(TLabel) == 0x4c && sizeof(TSkinIcon) == 0x28,
	"DialogElements layouts");
static_assert(sizeof(TMarkMenuData) == 0x80, "TMarkMenuData is 0x80 bytes");

// --- Logs and names ------------------------------------------------------------------

enum { OURS = 0, ORIGINAL = 1 };
static int Side;
static bool Testing; // the fakes log only while this test runs
static std::string Log[2];

static void Logf(const char *Format, ...)
{
	if (!Testing)
		return;
	char Buffer[1024];
	va_list Args;
	va_start(Args, Format);
	vsnprintf(Buffer, sizeof(Buffer), Format, Args);
	va_end(Args);
	Log[Side] += Buffer;
}

static void *VtableOf(const void *Object) { return *(void **)Object; }
static void SetVtable(void *Object, void *Vtable) { *(void **)Object = Vtable; }
template <class T> static void *Member(T Function) { return *(void **)&Function; }

// The objects of each side by role; the kinds of objects by vtable; what the
// element fakes were made with.
static std::map<const void *, std::string> Roles[2];
static std::map<void *, std::string> Kinds[2];
static std::map<const void *, std::string> Arguments[2];
static std::map<const void *, std::string> LabelTexts[2];
static int Serial[2];

static bool IsRealElement(const std::string &Kind) { return Kind == "ObjectIcon" || Kind == "Separator"; }
static std::string Name(int s, const void *Object)
{
	if (Object == NULL)
		return "0";
	// The real separators and object icons may reuse the memory of a fake
	// (a stale role).
	auto K = Kinds[s].find(VtableOf(Object));
	if (K != Kinds[s].end() && IsRealElement(K->second))
		return K->second;
	auto R = Roles[s].find(Object);
	if (R != Roles[s].end())
		return R->second;
	if (K != Kinds[s].end())
		return K->second;
	return "?";
}
static std::string Name(const void *Object) { return Name(Side, Object); }
static void Role(const void *Object, const char *Base)
{
	char Buffer[64];
	sprintf(Buffer, "%s%d", Base, Serial[Side]++);
	Roles[Side][Object] = Buffer;
}
static std::string KindOf(int s, const void *Object)
{
	auto K = Kinds[s].find(VtableOf(Object));
	return K == Kinds[s].end() ? "?" : K->second;
}

// --- DialogElements.cpp: the shared fakes report here -------------------------------------

void (*ElementCreated)(void *Object, const char *Kind, const char *Arguments);
void ReportElement(void *Object, const char *Kind, const char *Format, ...)
{
	if (ElementCreated == NULL)
		return;
	char Buffer[512];
	va_list Args;
	va_start(Args, Format);
	vsnprintf(Buffer, sizeof(Buffer), Format, Args);
	va_end(Args);
	ElementCreated(Object, Kind, Buffer);
}

// The new fakes of the DialogElements.cpp members ContentWindows.cpp uses.
TFakeTextButton::TFakeTextButton(int x, int y, const char *_Text, TCallback Callback, const char *Tooltip)
{
	memset(Data24, 0, sizeof(Data24));
	SetPosition(x, y);
	SetSize(60, 20);
	ReportElement(this, "TextButton", "%d,%d,%s,@%p/%d,%s", x, y, _Text, (void *)Callback.Receiver, Callback.Event,
		Tooltip == NULL ? "-" : Tooltip);
}
TFakeTextToggleButton::TFakeTextToggleButton(int x, int y, const char *_Text, int Skin1, int Skin2, TCallback Callback,
	const char *_Tooltip1, const char *_Tooltip2, bool _State)
	: TFakeTextButton(x, y, _Text, Skin1, Skin2, Callback, NULL)
{
	State = _State;
	Tooltip1 = _Tooltip1;
	Tooltip2 = _Tooltip2;
	SetPosition(x, y);
	SetSize(60, 20);
	ReportElement(this, "TextToggle", "%d,%d,%s,%d,%d,@%p/%d,%s,%s,%d", x, y, _Text, Skin1, Skin2,
		(void *)Callback.Receiver, Callback.Event, _Tooltip1, _Tooltip2, _State ? 1 : 0);
}
TFakeLabel::TFakeLabel(int x, int y, int Width, int Height, TFont Font, const char *_Text, bool Param)
{
	memset(Data24, 0, sizeof(Data24));
	SetPosition(x, y);
	SetSize(Width, Height);
	ReportElement(this, "Label", "%d,%d,%d,%d,%d,%d,%d,%d,%d,%s,%d", x, y, Width, Height, Font.FontNumber, Font.Color.R,
		Font.Color.G, Font.Color.B, Font.Alignment, _Text, Param ? 1 : 0);
}
void TFakeLabel::SetText(const char *_Text, bool Param)
{
	Logf("SetText%s,%s,%d;", Name(this).c_str(), _Text, Param ? 1 : 0);
	if (Testing)
		LabelTexts[Side][this] = _Text;
}
TFakeSkinIcon::TFakeSkinIcon(int _Skin)
{
	Skin = _Skin;
	SetSize(Skins[_Skin].width, Skins[_Skin].height);
	ReportElement(this, "SkinIcon", "%d", _Skin);
}
void TFakeSkinButton::SetRadioButtonGroup(TRadioButtonGroup *Group)
{
	Logf("Radio%s;", Name(this).c_str());
	RadioButtonGroup = Group;
	Group->Buttons(Group->Count) = (TSkinButton *)this;
	Group->Count++;
}
// The range (Data00[0], Data00[1]) comes from consolewindowtest.cpp's SetRange.
void TFakeScrollBarRange::SetValue(int _Value)
{
	int Minimum = ((int *)Data00)[0], Maximum = ((int *)Data00)[1];
	Logf("SetValue%d;", _Value);
	if (_Value > Maximum)
		_Value = Maximum;
	if (_Value < Minimum)
		_Value = Minimum;
	Value = _Value;
}

// Both sides' SetRange (the shared one logs only in consolewindowtest.cpp).
static void __fastcall FakeSetRange(TFakeScrollBarRange *Range, void *, int Minimum, int Maximum, int Value, int Visible)
{
	Logf("Range%d,%d,%d,%d;", Minimum, Maximum, Value, Visible);
	((int *)Range->Data00)[0] = Minimum;
	((int *)Range->Data00)[1] = Maximum;
	if (Value > Maximum)
		Value = Maximum;
	if (Value < Minimum)
		Value = Minimum;
	Range->Value = Value;
}

// The test vtables the element fakes get.
static int ScrollMinimumHeight;
class TCWTestElement : public TElement {
public:
	void Draw(int Surface, int x, int y) { Logf("D%s,%d,%d,%d;", Name(this).c_str(), Surface, x, y); }
	bool LeftButtonDown(int x, int y, int Flags) { Logf("P%s,%d,%d,%d;", Name(this).c_str(), x, y, Flags); return false; }
	bool LeftButtonUp(int x, int y, int Flags) { Logf("U%s,%d,%d,%d;", Name(this).c_str(), x, y, Flags); return false; }
};
class TCWTestLabel : public TFakeLabel {
public:
	TCWTestLabel() : TFakeLabel(TFont(), "") {}
	void Draw(int Surface, int x, int y) { Logf("D%s,%d,%d,%d;", Name(this).c_str(), Surface, x, y); }
};
class TCWTestScrollBar : public TFakeScrollBar {
public:
	TCWTestScrollBar() : TFakeScrollBar(TRect(0, 0, 0, 0), 0, 0, 0, 0, 0) {}
	~TCWTestScrollBar() {}
	int getMinimumHeight() { return ScrollMinimumHeight; }
	void Draw(int Surface, int x, int y) { Logf("D%s,%d,%d,%d,%dx%d;", Name(this).c_str(), Surface, x, y, Width, Height); }
	bool MouseWheel(int x, int y, int Code)
	{
		Logf("W%s,%d,%d,%d;", Name(this).c_str(), x, y, Code);
		int Minimum = ((int *)Range->Data00)[0], Maximum = ((int *)Range->Data00)[1];
		int Value = Range->Value + (Code == 1 ? -3 : 3);
		if (Value >= Minimum && Value <= Maximum)
			Range->Value = Value;
		return Code == 1;
	}
};
static void *ElementVtable, *ScrollVtable, *LabelVtable;

// Replaces "@<pointer>" in the reported arguments by the object's name.
static std::string NamePointers(const char *Text)
{
	std::string Out;
	for (const char *p = Text; *p != 0; p++) {
		if (*p == '@') {
			void *Pointer = NULL;
			int Length = 0;
			if (sscanf(p + 1, "%p%n", &Pointer, &Length) == 1) {
				Out += Name(Pointer);
				p += Length;
				continue;
			}
		}
		Out += *p;
	}
	return Out;
}
static void HookElement(void *Object, const char *Kind, const char *Args)
{
	if (!Testing)
		return;
	std::string Text = NamePointers(Args);
	Logf("N%s(%s);", Kind, Text.c_str());
	Role(Object, Kind);
	Arguments[Side][Object] = std::string(Kind) + "(" + Text + ")";
	if (strcmp(Kind, "ScrollBar") == 0) {
		SetVtable(Object, ScrollVtable);
	} else if (strcmp(Kind, "Label") == 0) {
		SetVtable(Object, LabelVtable);
	} else {
		SetVtable(Object, ElementVtable);
	}
}
static void HookSetSkins(TSkinButton *Button, int ButtonUp, int ButtonDown)
{
	Logf("Skins%s,%d,%d;", Name(Button).c_str(), ButtonUp, ButtonDown);
	Button->ButtonUp = ButtonUp;
	Button->ButtonDown = ButtonDown;
}
static void HookResizer(TResizer *Resizer, TComponent *ToResize, bool P2, bool P3, bool P4)
{
	if (!Testing)
		return;
	Role(Resizer, "Resizer");
	Logf("NResizer%s,%d,%d,%d;", Name(ToResize).c_str(), P2 ? 1 : 0, P3 ? 1 : 0, P4 ? 1 : 0);
	SetVtable(Resizer, ElementVtable);
}

// --- The world and the fakes of the callees ----------------------------------------------

enum { NCONTAINERS = 4, NBUDDIES = 12, NOPPONENTS = 12 };
struct TContentWorld {
	int ActionState, MouseX, MouseY;
	int Health, MaxHealth, Mana, MaxMana, Capacity, Soul, Experience, Level, LevelProgress, MagicLevel,
		MagicProgress, SkillValue[7], SkillProgress[7];
	bool States[9]; // poisoned, burning, mana shield, paralysed, skull, fight, drunk, electrified, hasted
	int PlayerSkull;
	bool LevelBar, MagLevelBar, SkillBar[7], ClassicControl, SecureMode[2], BlinkState, InParty;
	int AttackMode[2], ChaseMode[2], AutomapZoom, WindowSize[10];
	TObject Body[11];
	int ObjectCount[NCONTAINERS], ContainerCapacity[NCONTAINERS];
	TObject Contents[NCONTAINERS][24], ContainerObject[NCONTAINERS];
	bool HasParent[NCONTAINERS], ContainerOpen[NCONTAINERS];
	int FreeContainer;
	int OwnCount, CounterCount;
	TObject Own[12], Counter[12];
	int Opponents;
	uint32 AttackID[2], FollowID; // the values the windows set have one copy per side
	int Buddies;
	bool BuddyOnline[NBUDDIES], BuddyNew[NBUDDIES];
	int BuddyIcon[NBUDDIES];
	int PlayerX, PlayerY, PlayerZ;
	bool AutomapChanged, SurfaceLost;
	int Destination;
	int DragIndex, DragX, DragY, DragZ, UseIndex, UseX, UseY, UseZ, TradeIndex, TradeX, TradeY, TradeZ;
	TObject DragObject, UseObject, TradeObject;
};
static TContentWorld World;
static TCreature Player, OpponentList[NOPPONENTS];
static const char *const ContainerNames[NCONTAINERS] = {"Backpack", "Bag", "A very long container name", "Box"};
static const char *const BuddyNames[NBUDDIES] = {"Seymour", "Xa", "Mr Very Long Name Indeed", "Bubble", "Tester",
	"Cip", "Averylongnamewithoutanyblank", "Eva", "Ana", "Zed", "Q", "Jolly Roger"};
static const char *const OpponentNames[NOPPONENTS] = {"Rat", "Cave Rat", "A Very Long Monster Name", "Seymour",
	"Troll", "Xa", "Demon", "Mr Very Long Name Indeed", "Orc", "Dragon Lord", "Bug", "Wolf"};

static int __cdecl FakeGetActionState() { Logf("AState;"); return World.ActionState; }
static int __cdecl FakeGetMouseX() { Logf("MX;"); return World.MouseX; }
static int __cdecl FakeGetMouseY() { Logf("MY;"); return World.MouseY; }
static TObject *__cdecl FakeGetDragObject(int *Index, int *x, int *y, int *z)
{
	Logf("GetDrag;");
	*Index = World.DragIndex;
	*x = World.DragX;
	*y = World.DragY;
	*z = World.DragZ;
	return &World.DragObject;
}
static TObject *__cdecl FakeGetUseObject(int *Index, int *x, int *y, int *z)
{
	Logf("GetUse;");
	*Index = World.UseIndex;
	*x = World.UseX;
	*y = World.UseY;
	*z = World.UseZ;
	return &World.UseObject;
}
static TObject *__cdecl FakeGetTradeObject(int *Index, int *x, int *y, int *z)
{
	Logf("GetTrade;");
	*Index = World.TradeIndex;
	*x = World.TradeX;
	*y = World.TradeY;
	*z = World.TradeZ;
	return &World.TradeObject;
}
static void __cdecl FakeSetDragObject(TObject Obj, int Index, int x, int y, int z)
{
	Logf("SetDrag%d,%d,%d,%d,%d,%d,%d,%d;", Obj.Type, Obj.Data, Obj.Data2, Index, x, y, z, 0);
}
static void __cdecl FakeSetUseObject(TObject Obj, int Index, int x, int y, int z)
{
	Logf("SetUse%d,%d,%d,%d,%d,%d,%d;", Obj.Type, Obj.Data, Obj.Data2, Index, x, y, z);
}
static void __cdecl FakeSetTradeObject(TObject Obj, int Index, int x, int y, int z)
{
	Logf("SetTrade%d,%d,%d,%d,%d,%d,%d;", Obj.Type, Obj.Data, Obj.Data2, Index, x, y, z);
}
static void __cdecl FakeSetTradeInfo(int Info) { Logf("TradeInfo%d;", Info); }
static void __cdecl FakeSetDragTarget(int x, int y, int z) { Logf("DragTarget%d,%d,%d;", x, y, z); }
static void __cdecl FakeSetMouseOverText(const char *Text) { Logf("Over%s;", Text); }
static void __cdecl FakeAddMessage(int Type, INT_PTR Param1, INT_PTR Param2)
{
	if (Type == 59) {
		TMarkMenuData *Data = (TMarkMenuData *)Param1;
		Logf("Msg59,%d,%d,%d,%d,%d,%d,%s,%s;", Data->Coordinate.x, Data->Coordinate.y, Data->Coordinate.z, Data->Mark.x,
			Data->Mark.y, Data->Mark.Type, Data->Mark.Text, Name(Data->Window).c_str());
		delete Data;
	} else if (Type == 60) {
		Logf("Msg60,%s,%d;", Name((void *)Param1).c_str(), (int)Param2);
	} else {
		Logf("Msg%d,%d,%d;", Type, (int)Param1, (int)Param2);
	}
}
static void __cdecl FakeSetStatusMessage(const char *Message, bool Permanent) { Logf("Status%s,%d;", Message, Permanent ? 1 : 0); }
static bool __cdecl FakeGetBlinkState() { Logf("Blink;"); return World.BlinkState; }

static TObject *__cdecl FakeGetBodyObject(int Position) { Logf("Body%d;", Position); return &World.Body[Position]; }
static int __cdecl FakeGetContainerObjectCount(int Container) { Logf("CCount%d;", Container); return World.ObjectCount[Container]; }
static int __cdecl FakeGetContainerCapacity(int Container) { Logf("CCap%d;", Container); return World.ContainerCapacity[Container]; }
static TObject *__cdecl FakeGetContainerContent(TObject *Result, int Container, int Position)
{
	Logf("CContent%d,%d;", Container, Position);
	*Result = World.Contents[Container][Position % 24];
	return Result;
}
static TObject *__cdecl FakeGetContainerObject(TObject *Result, int Container)
{
	Logf("CObject%d;", Container);
	*Result = World.ContainerObject[Container];
	return Result;
}
static char *__cdecl FakeGetContainerName(int Container) { Logf("CName%d;", Container); return (char *)ContainerNames[Container]; }
static bool __cdecl FakeGetContainerHasParent(int Container) { Logf("CParent%d;", Container); return World.HasParent[Container]; }
static bool __cdecl FakeIsContainerOpen(int Container) { Logf("COpen%d;", Container); return World.ContainerOpen[Container]; }
static int __cdecl FakeGetFreeContainer() { Logf("Free;"); return World.FreeContainer; }
static int __cdecl FakeGetOwnOfferCount() { Logf("OwnCount;"); return World.OwnCount; }
static int __cdecl FakeGetCounterOfferCount() { Logf("CounterCount;"); return World.CounterCount; }
static TObject *__cdecl FakeGetOwnOfferObject(TObject *Result, int Position)
{
	Logf("OwnObj%d;", Position);
	*Result = World.Own[Position % 12];
	return Result;
}
static TObject *__cdecl FakeGetCounterOfferObject(TObject *Result, int Position)
{
	Logf("CounterObj%d;", Position);
	*Result = World.Counter[Position % 12];
	return Result;
}
static char *__cdecl FakeGetOwnOfferName() { Logf("OwnName;"); return (char *)"Tester"; }
static char *__cdecl FakeGetCounterOfferName() { Logf("CounterName;"); return (char *)"Mr Very Long Name Indeed"; }
static void __cdecl FakeClearOffers() { Logf("ClearOffers;"); }

static int __cdecl FakeGetNumberOfOpponents() { Logf("Opponents;"); return World.Opponents; }
static TCreature *__cdecl FakeGetOpponent(int Number)
{
	Logf("Opponent%d;", Number);
	return Number >= 0 && Number < World.Opponents ? &OpponentList[Number] : NULL;
}
static uint32 __cdecl FakeGetAttackID() { Logf("AttackID;"); return World.AttackID[Side]; }
static uint32 __cdecl FakeGetFollowID() { Logf("FollowID;"); return World.FollowID; }
static void __cdecl FakeSetAttackID(uint32 ID)
{
	Logf("SetAttack%u;", ID);
	World.AttackID[Side] = ID;
}
static void __cdecl FakeSetHighlightID(uint32 ID) { Logf("Highlight%u;", ID); }
static bool __cdecl FakeIsPlayerInParty() { Logf("InParty;"); return World.InParty; }
static int __cdecl FakeGetNumberOfBuddies() { Logf("Buddies;"); return World.Buddies; }
static int __cdecl FakeGetBuddyListIcon(int Position) { Logf("BIcon%d;", Position); return World.BuddyIcon[Position]; }
static const char *__cdecl FakeGetBuddyListName(int Position) { Logf("BName%d;", Position); return BuddyNames[Position]; }
static bool __cdecl FakeGetBuddyListOnline(int Position) { Logf("BOnline%d;", Position); return World.BuddyOnline[Position]; }
static bool __cdecl FakeGetBuddyListNew(int Position) { Logf("BNew%d;", Position); return World.BuddyNew[Position]; }
static uint32 __cdecl FakeGetBuddyListID(int Position) { Logf("BID%d;", Position); return 0x10000100 + Position; }
static uint32 __cdecl FakeGetPlayerID() { Logf("PID;"); return Player.CreatureID; }
static TCreature *__cdecl FakeGetCreature(uint32 ID) { Logf("Cr%u;", ID); return ID == Player.CreatureID ? &Player : NULL; }

#define WORLD_INT(Fn, Field) static int __cdecl Fake##Fn() { Logf(#Fn ";"); return World.Field; }
WORLD_INT(GetPlayerHealth, Health)
WORLD_INT(GetPlayerMaxHealth, MaxHealth)
WORLD_INT(GetPlayerMana, Mana)
WORLD_INT(GetPlayerMaxMana, MaxMana)
WORLD_INT(GetPlayerCapacity, Capacity)
WORLD_INT(GetPlayerSoul, Soul)
WORLD_INT(GetPlayerExperience, Experience)
WORLD_INT(GetPlayerLevel, Level)
WORLD_INT(GetPlayerLevelProgress, LevelProgress)
WORLD_INT(GetPlayerMagicLevel, MagicLevel)
WORLD_INT(GetPlayerMagicLevelProgress, MagicProgress)
WORLD_INT(GetAttackMode, AttackMode[Side])
WORLD_INT(GetChaseMode, ChaseMode[Side])
WORLD_INT(GetAutomapZoom, AutomapZoom)
#define WORLD_STATE(Fn, i) static bool __cdecl Fake##Fn() { Logf(#Fn ";"); return World.States[i]; }
WORLD_STATE(IsPlayerPoisoned, 0)
WORLD_STATE(IsPlayerBurning, 1)
WORLD_STATE(IsPlayerManaShielded, 2)
WORLD_STATE(IsPlayerParalyzed, 3)
WORLD_STATE(HasPlayerSkull, 4)
WORLD_STATE(IsPlayerInFight, 5)
WORLD_STATE(IsPlayerDrunk, 6)
WORLD_STATE(IsPlayerElectrified, 7)
WORLD_STATE(IsPlayerHasted, 8)
static int __cdecl FakeGetPlayerSkillValue(int Skill) { Logf("SkillV%d;", Skill); return World.SkillValue[Skill]; }
static int __cdecl FakeGetPlayerSkillProgress(int Skill) { Logf("SkillP%d;", Skill); return World.SkillProgress[Skill]; }
static bool __cdecl FakeGetLevelBar() { Logf("LevelBar;"); return World.LevelBar; }
static bool __cdecl FakeGetMagLevelBar() { Logf("MagBar;"); return World.MagLevelBar; }
static bool __cdecl FakeGetSkillBar(int Skill) { Logf("SkillBar%d;", Skill); return World.SkillBar[Skill]; }
static bool __cdecl FakeGetClassicControl() { Logf("Classic;"); return World.ClassicControl; }
static bool __cdecl FakeGetSecureMode() { Logf("GetSecure;"); return World.SecureMode[Side]; }
static void __cdecl FakeSetAttackMode(int Value)
{
	Logf("SetAttackMode%d;", Value);
	World.AttackMode[Side] = Value;
}
static void __cdecl FakeSetChaseMode(int Value)
{
	Logf("SetChase%d;", Value);
	World.ChaseMode[Side] = Value;
}
static void __cdecl FakeSetSecureMode(bool Value)
{
	Logf("SetSecure%d;", Value ? 1 : 0);
	World.SecureMode[Side] = Value;
}
static void __cdecl FakeSetAutomapZoom(int Value) { Logf("SetZoom%d;", Value); }
static int __cdecl FakeGetContentWindowSize(int ID) { Logf("GetCWSize%d;", ID); return World.WindowSize[ID]; }
static void __cdecl FakeSetContentWindowSize(int ID, int Size) { Logf("SetCWSize%d,%d;", ID, Size); }

static void __cdecl FakeGetMapPosition(int *x, int *y, int *z)
{
	Logf("MapPos;");
	*x = World.PlayerX;
	*y = World.PlayerY;
	*z = World.PlayerZ;
}
static bool __cdecl FakeGetAutomapChanged() { Logf("AMChanged;"); return World.AutomapChanged; }
// The automap's colours are a function of the point (not logged: 11236 calls a draw).
static TRGBColor *__cdecl FakeGetAutomapColor(TRGBColor *Color, int x, int y, int z)
{
	uint32 h = (uint32)(x * 73856093) ^ (uint32)(y * 19349663) ^ (uint32)(z * 83492791);
	Color->R = h & 0xff;
	Color->G = (h >> 8) & 0xff;
	Color->B = (h >> 16) & 0xff;
	return Color;
}
// A few marks per block, some near the player.
static void __cdecl FakeGetAutomapMarks(int BlockX, int BlockY, int z, vector<TMark> *Marks, int *Count)
{
	Logf("Marks%d,%d,%d;", BlockX, BlockY, z);
	uint32 h = (uint32)(BlockX * 7 + BlockY * 13 + z * 3 + World.PlayerX % 5);
	int n = 1 + (int)(h % 5);
	for (int i = 0; i < n; i++) {
		TMark &M = (*Marks)(i);
		uint32 g = h * 31 + i * 17;
		if (g % 3 != 0) {
			M.x = World.PlayerX - 6 + (int)(g % 13);
			M.y = World.PlayerY - 6 + (int)((g / 13) % 13);
		} else {
			M.x = BlockX * 256 + (int)(g % 256);
			M.y = BlockY * 256 + (int)((g / 3) % 256);
		}
		if (g % 7 == 0)
			M.x = M.y = 0;
		if (g % 11 == 0)
			M.y = 0;
		M.Type = (int)(g % 20);
		sprintf(M.Text, g % 3 == 0 ? "" : "Mark %u of block %d,%d", g % 1000, BlockX, BlockY);
		// Next to the map's corner, the first mark lies on y 0 below the player.
		if (i == 0 && World.PlayerY < 7) {
			M.x = World.PlayerX;
			M.y = 0;
			strcpy(M.Text, "On the edge");
		}
	}
	*Count = n;
}
static int __cdecl FakeSetPlayerDestination(int x, int y, int z, bool Exact, bool Diagonal)
{
	Logf("Dest%d,%d,%d,%d,%d;", x, y, z, Exact ? 1 : 0, Diagonal ? 1 : 0);
	return World.Destination;
}
static bool __cdecl FakeGetObjectFlag(int Type, int Flag)
{
	Logf("Flag%d,%d;", Type, Flag);
	return (Type * 7 + Flag * 3) % 4 == 0;
}

static void __cdecl FakeSetHelpHandle(int Handle) { Logf("HelpH%d;", Handle); }
static void __cdecl FakeSetHelpSize(int Width, int Height) { Logf("HelpS%d,%d;", Width, Height); }
static void __cdecl FakeMoveHelpPosition(int dx, int dy) { Logf("HelpM%d,%d;", dx, dy); }
static void __cdecl FakeAddHotkey(int Key, TCallback Callback)
{
	Logf("AddHK%x,%s,%d;", Key, Name(Callback.Receiver).c_str(), Callback.Event);
}
static void __cdecl FakeRemoveHotkey(int Key) { Logf("RemHK%x;", Key); }

static void __cdecl FakeDrawPicturePart(int Surface, int x, int y, int Picture, int srcx, int srcy, int Width, int Height)
{
	Logf("Part%d,%d,%d,%d,%d,%d,%d,%d;", Surface, x, y, Picture, srcx, srcy, Width, Height);
}
static void __cdecl FakeDrawPicturePartClipped(int Surface, int x, int y, int Picture, int srcx, int srcy, int Width,
	int Height, TRect Clip)
{
	Logf("PartC%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d;", Surface, x, y, Picture, srcx, srcy, Width, Height, Clip.x, Clip.y,
		Clip.width, Clip.height);
}
static void __cdecl FakeDrawObject(int Surface, int x, int y, int Size, TObject Obj, TRGBColor Highlight, TRect Clip,
	TFont Font, bool ShowCount)
{
	Logf("Obj%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d;", Surface, x, y, Size, Obj.Type, Obj.Data,
		Obj.Data2, Highlight.R, Highlight.G, Highlight.B, Clip.x, Clip.y, Clip.width, Clip.height, Font.FontNumber,
		Font.Color.R, Font.Color.G, Font.Color.B, Font.Alignment, ShowCount ? 1 : 0);
}
static void __cdecl FakeDrawOutfit(int Surface, int x, int y, int Size, int Outfit, int *Colors, TRGBColor Highlight,
	TRect Clip, TFont Font)
{
	Logf("Outfit%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d;", Surface, x, y, Size, Outfit,
		Colors[0], Colors[1], Colors[2], Colors[3], Highlight.R, Highlight.G, Highlight.B, Clip.x, Clip.y, Clip.width,
		Clip.height, Font.FontNumber, Font.Color.R, Font.Color.G, Font.Color.B, Font.Alignment,
		GetSurface()->Flag26 ? 1 : 0);
}
static void __cdecl FakePrintFormattedText(int Surface, int x, int y, TFont Format, const char *Text)
{
	Logf("PrintF%d,%d,%d,%d,%d,%d,%d,%d,%s;", Surface, x, y, Format.FontNumber, Format.Color.R, Format.Color.G,
		Format.Color.B, Format.Alignment, Text);
}
static void __cdecl FakePrintFormattedTextClipped(int Surface, int x, int y, TFont Format, const char *Text, TRect Clip)
{
	Logf("PrintFC%d,%d,%d,%d,%d,%d,%d,%d,%s,%d,%d,%d,%d;", Surface, x, y, Format.FontNumber, Format.Color.R,
		Format.Color.G, Format.Color.B, Format.Alignment, Text, Clip.x, Clip.y, Clip.width, Clip.height);
}
static void __cdecl FakePrintTextClipped(int Surface, int x, int y, int Font, TRGBColor Color, const char *Text,
	int Alignment, TRect Clip)
{
	Logf("PrintC%d,%d,%d,%d,%d,%d,%d,%s,%d,%d,%d,%d,%d;", Surface, x, y, Font, Color.R, Color.G, Color.B, Text, Alignment,
		Clip.x, Clip.y, Clip.width, Clip.height);
}
static int CharWidth(char c) { return 4 + (uint8)c % 4; }
static int __cdecl FakeGetFittingTextLength(int Font, const char *Text, int Width)
{
	Logf("Fit%d,%d;", Font, Width);
	int i = 0, Sum = 0;
	while (Text[i] != 0 && Sum + CharWidth(Text[i]) <= Width)
		Sum += CharWidth(Text[i++]);
	return i;
}
static int __cdecl FakeGetFontHeight(int Font) { Logf("FontH%d;", Font); return 11 + Font % 2; }
static int __cdecl FakeGetFontLineHeight(int Font) { Logf("LineH%d;", Font); return 13 + Font % 3; }

// Communication.cpp: the Send functions no other test defines (ours), and the
// fakes both sides use.
static void __cdecl FakeSendCloseContainer(int Container) { Logf("SCloseCont%d;", Container); }
static void __cdecl FakeSendContainerUp(int Container) { Logf("SUp%d;", Container); }
static void __cdecl FakeSendFightModes() { Logf("SFight;"); }
static void __cdecl FakeSendInspectTrade(int Side_, int Position) { Logf("SInspect%d,%d;", Side_, Position); }
static void __cdecl FakeSendAcceptTrade() { Logf("SAccept;"); }
static void __cdecl FakeSendRejectTrade() { Logf("SReject;"); }
static void __cdecl FakeSendMoveObject(int x, int y, int z, int Type, int Position, int dx, int dy, int dz, int Count)
{
	Logf("SMove%d,%d,%d,%d,%d,%d,%d,%d,%d;", x, y, z, Type, Position, dx, dy, dz, Count);
}
static void __cdecl FakeSendUseObjectWith(int x1, int y1, int z1, int Type1, int Position1, int x2, int y2, int z2,
	int Type2, int Position2)
{
	Logf("SUseWith%d,%d,%d,%d,%d,%d,%d,%d,%d,%d;", x1, y1, z1, Type1, Position1, x2, y2, z2, Type2, Position2);
}
static void __cdecl FakeSendUseObject(int x, int y, int z, int Type, int Position, int Container)
{
	Logf("SUse%d,%d,%d,%d,%d,%d;", x, y, z, Type, Position, Container);
}
static void __cdecl FakeSendLook(int x, int y, int z, int Type, int Position)
{
	Logf("SLook%d,%d,%d,%d,%d;", x, y, z, Type, Position);
}
static void __cdecl FakeSendUseObjectOnCreature(int x, int y, int z, int Type, int Position, uint32 ID)
{
	Logf("SUseOn%d,%d,%d,%d,%d,%u;", x, y, z, Type, Position, ID);
}
static void __cdecl FakeSendRequestTrade(int x, int y, int z, int Type, int Position, uint32 ID)
{
	Logf("SRequestTrade%d,%d,%d,%d,%d,%u;", x, y, z, Type, Position, ID);
}
static void __cdecl FakeSendAttack(uint32 ID) { Logf("SAttack%u;", ID); }
static void __cdecl FakeSendOpenPrivateConversation(const char *Name_) { Logf("SPrivate%s;", Name_); }
static void __cdecl FakeSendWalk() { Logf("SWalk;"); }
static void __cdecl FakeSendStop() { Logf("SStop;"); }
void SendCloseContainer(int Container) { FakeSendCloseContainer(Container); }
void SendContainerUp(int Container) { FakeSendContainerUp(Container); }
void SendFightModes() { FakeSendFightModes(); }
void SendInspectTrade(int Side_, int Position) { FakeSendInspectTrade(Side_, Position); }
void SendAcceptTrade() { FakeSendAcceptTrade(); }
void SendRejectTrade() { FakeSendRejectTrade(); }

// The surface: every call ContentWindows.cpp makes is logged.
class TContentSurface : public TTestSurface {
public:
	void SetPixel(int Surface, int x, int y, TRGBColor C)
	{
		Logf("Pixel%d,%d,%d,%d,%d,%d;", Surface, x, y, C.R, C.G, C.B);
	}
	void FillRectangleClipped(int Surface, TRect R, TRGBColor C, TRect Clip)
	{
		Logf("FillC%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d;", Surface, R.x, R.y, R.width, R.height, C.R, C.G, C.B, Clip.x,
			Clip.y, Clip.width, Clip.height);
	}
	void DrawHorizontalLine(int Surface, int x, int y, int Width, TRGBColor C)
	{
		Logf("HLine%d,%d,%d,%d,%d,%d,%d;", Surface, x, y, Width, C.R, C.G, C.B);
	}
	void DrawFrame(int Surface, TRect R, int LineWidth, TRGBColor C)
	{
		Logf("Frame%d,%d,%d,%d,%d,%d,%d,%d,%d;", Surface, R.x, R.y, R.width, R.height, LineWidth, C.R, C.G, C.B);
	}
	void DrawFrameClipped(int Surface, TRect R, int LineWidth, TRGBColor C, TRect Clip)
	{
		Logf("FrameC%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d;", Surface, R.x, R.y, R.width, R.height, LineWidth, C.R, C.G,
			C.B, Clip.x, Clip.y, Clip.width, Clip.height);
	}
	void WriteImage(int ToSurface, int x, int y, int Width, int Height, uint8 *Data)
	{
		uint32 Hash = 0;
		for (int i = 0; i < Width * Height * 3; i++)
			Hash = Hash * 31 + Data[i];
		Logf("Image%d,%d,%d,%d,%d,%08x;", ToSurface, x, y, Width, Height, Hash);
	}
	void BlitRect(int ToSurface, int x, int y, int FromSurface, TRect S)
	{
		Logf("Blit%d,%d,%d,%d,%d,%d,%d,%d;", ToSurface, x, y, FromSurface, S.x, S.y, S.width, S.height);
	}
	bool SurfaceRecreated(int Surface)
	{
		Logf("Lost%d;", Surface);
		return World.SurfaceLost;
	}
};
static TContentSurface *Surface;
static TSurface *__cdecl FakeGetSurface() { return Surface; }

// The clock (fixed), the image's printf (the automap's debugging output),
// sprintf and _snprintf.
static __time32_t __cdecl FakeTime(__time32_t *Time)
{
	if (Time != NULL)
		*Time = 1700000000;
	return 1700000000;
}
static int __cdecl ImagePrintf(const char *Format, ...)
{
	char Buffer[512];
	va_list Args;
	va_start(Args, Format);
	int Result = vsnprintf(Buffer, sizeof(Buffer), Format, Args);
	va_end(Args);
	Logf("Printf%s", Buffer);
	return Result;
}
static int __cdecl ImageSprintf(char *Buffer, const char *Format, ...)
{
	va_list Args;
	va_start(Args, Format);
	int Result = vsprintf(Buffer, Format, Args);
	va_end(Args);
	return Result;
}
static int __cdecl ImageSnprintf(char *Buffer, size_t Count, const char *Format, ...)
{
	va_list Args;
	va_start(Args, Format);
	int Result = _vsnprintf(Buffer, Count, Format, Args);
	va_end(Args);
	return Result;
}
// Our printf on our side (the automap's MouseHover).
static int __cdecl FakePrintf(const char *Format, ...)
{
	char Buffer[512];
	va_list Args;
	va_start(Args, Format);
	int Result = vsnprintf(Buffer, sizeof(Buffer), Format, Args);
	va_end(Args);
	Logf("Printf%s", Buffer);
	return Result;
}

// New memory of both sides gets the same byte pattern.
static uint8 Pattern = 0xab;
static void *__cdecl PatternNew(size_t Size)
{
	void *Memory = malloc(Size == 0 ? 1 : Size);
	memset(Memory, Pattern, Size);
	return Memory;
}

// Patches that are undone at the end.
struct TSaved {
	uint8 *Code;
	uint8 Bytes[5];
};
static std::vector<TSaved> SavedCode;
static void Redirect(void *Function, void *Target)
{
	TSaved S;
	S.Code = (uint8 *)Function;
	DWORD Protection;
	VirtualProtect(Function, 5, PAGE_EXECUTE_READWRITE, &Protection);
	memcpy(S.Bytes, Function, 5);
	PatchJump((uint32)(uintptr_t)Function, Target);
	VirtualProtect(Function, 5, Protection, &Protection);
	FlushInstructionCache(GetCurrentProcess(), Function, 5);
	SavedCode.push_back(S);
}
static void RestoreAll()
{
	for (size_t i = SavedCode.size(); i-- > 0;) {
		DWORD Protection;
		VirtualProtect(SavedCode[i].Code, 5, PAGE_EXECUTE_READWRITE, &Protection);
		memcpy(SavedCode[i].Code, SavedCode[i].Bytes, 5);
		VirtualProtect(SavedCode[i].Code, 5, Protection, &Protection);
		FlushInstructionCache(GetCurrentProcess(), SavedCode[i].Code, 5);
	}
	SavedCode.clear();
}

static void PatchCallees()
{
	struct { uint32 Image; void *Ours; void *Fake; } Table[] = {
		{0x4e6e30, (void *)GetActionState, (void *)FakeGetActionState},
		{0x4e6e60, (void *)GetMouseX, (void *)FakeGetMouseX},
		{0x4e6e70, (void *)GetMouseY, (void *)FakeGetMouseY},
		{0x4e7030, (void *)GetDragObject, (void *)FakeGetDragObject},
		{0x4e7190, (void *)GetUseObject, (void *)FakeGetUseObject},
		{0x4e7150, (void *)GetTradeObject, (void *)FakeGetTradeObject},
		{0x4e6f70, (void *)SetDragObject, (void *)FakeSetDragObject},
		{0x4e70f0, (void *)SetUseObject, (void *)FakeSetUseObject},
		{0x4e70a0, (void *)SetTradeObject, (void *)FakeSetTradeObject},
		{0x4e7140, (void *)SetTradeInfo, (void *)FakeSetTradeInfo},
		{0x4e6fc0, (void *)SetDragTarget, (void *)FakeSetDragTarget},
		{0x4e72e0, (void *)SetMouseOverText, (void *)FakeSetMouseOverText},
		{0x4e9020, (void *)AddMessage, (void *)FakeAddMessage},
		{0x4ff500, (void *)SetStatusMessage, (void *)FakeSetStatusMessage},
		{0x4e7370, (void *)GetBlinkState, (void *)FakeGetBlinkState},
		{0x4a4880, (void *)GetBodyObject, (void *)FakeGetBodyObject},
		{0x4a5bd0, (void *)GetContainerObjectCount, (void *)FakeGetContainerObjectCount},
		{0x4a6790, (void *)GetContainerCapacity, (void *)FakeGetContainerCapacity},
		{0x4a6150, (void *)GetContainerContent, (void *)FakeGetContainerContent},
		{0x4a5e80, (void *)GetContainerObject, (void *)FakeGetContainerObject},
		{0x4a64e0, (void *)GetContainerName, (void *)FakeGetContainerName},
		{0x4a6a40, (void *)GetContainerHasParent, (void *)FakeGetContainerHasParent},
		{0x4a5220, (void *)IsContainerOpen, (void *)FakeIsContainerOpen},
		{0x4a6cf0, (void *)GetFreeContainer, (void *)FakeGetFreeContainer},
		{0x4a4530, (void *)GetOwnOfferCount, (void *)FakeGetOwnOfferCount},
		{0x4a4540, (void *)GetCounterOfferCount, (void *)FakeGetCounterOfferCount},
		{0x4a7a00, (void *)GetOwnOfferObject, (void *)FakeGetOwnOfferObject},
		{0x4a7b90, (void *)GetCounterOfferObject, (void *)FakeGetCounterOfferObject},
		{0x4a7760, (void *)GetOwnOfferName, (void *)FakeGetOwnOfferName},
		{0x4a78b0, (void *)GetCounterOfferName, (void *)FakeGetCounterOfferName},
		{0x4a4520, (void *)ClearOffers, (void *)FakeClearOffers},
		{0x44d9f0, (void *)GetNumberOfOpponents, (void *)FakeGetNumberOfOpponents},
		{0x44dc70, (void *)GetOpponent, (void *)FakeGetOpponent},
		{0x44bb50, (void *)GetAttackID, (void *)FakeGetAttackID},
		{0x44bb60, (void *)GetFollowID, (void *)FakeGetFollowID},
		{0x44bb00, (void *)SetAttackID, (void *)FakeSetAttackID},
		{0x44bb40, (void *)SetHighlightID, (void *)FakeSetHighlightID},
		{0x44d6f0, (void *)IsPlayerInParty, (void *)FakeIsPlayerInParty},
		{0x44fba0, (void *)GetNumberOfBuddies, (void *)FakeGetNumberOfBuddies},
		{0x450330, (void *)GetBuddyListIcon, (void *)FakeGetBuddyListIcon},
		{0x44feb0, (void *)GetBuddyListName, (void *)FakeGetBuddyListName},
		{0x450020, (void *)GetBuddyListOnline, (void *)FakeGetBuddyListOnline},
		{0x450190, (void *)GetBuddyListNew, (void *)FakeGetBuddyListNew},
		{0x44fd40, (void *)GetBuddyListID, (void *)FakeGetBuddyListID},
		{0x44b9c0, (void *)GetPlayerID, (void *)FakeGetPlayerID},
		{0x44c6e0, (void *)GetCreature, (void *)FakeGetCreature},
		{0x44b9d0, (void *)GetPlayerHealth, (void *)FakeGetPlayerHealth},
		{0x44b9e0, (void *)GetPlayerMaxHealth, (void *)FakeGetPlayerMaxHealth},
		{0x44ba20, (void *)GetPlayerMana, (void *)FakeGetPlayerMana},
		{0x44ba30, (void *)GetPlayerMaxMana, (void *)FakeGetPlayerMaxMana},
		{0x44ba70, (void *)GetPlayerCapacity, (void *)FakeGetPlayerCapacity},
		{0x44ba40, (void *)GetPlayerSoul, (void *)FakeGetPlayerSoul},
		{0x44b9f0, (void *)GetPlayerExperience, (void *)FakeGetPlayerExperience},
		{0x44ba00, (void *)GetPlayerLevel, (void *)FakeGetPlayerLevel},
		{0x44ba10, (void *)GetPlayerLevelProgress, (void *)FakeGetPlayerLevelProgress},
		{0x44ba50, (void *)GetPlayerMagicLevel, (void *)FakeGetPlayerMagicLevel},
		{0x44ba60, (void *)GetPlayerMagicLevelProgress, (void *)FakeGetPlayerMagicLevelProgress},
		{0x44d1c0, (void *)GetPlayerSkillValue, (void *)FakeGetPlayerSkillValue},
		{0x44d320, (void *)GetPlayerSkillProgress, (void *)FakeGetPlayerSkillProgress},
		{0x44ba80, (void *)IsPlayerPoisoned, (void *)FakeIsPlayerPoisoned},
		{0x44ba90, (void *)IsPlayerBurning, (void *)FakeIsPlayerBurning},
		{0x44bac0, (void *)IsPlayerManaShielded, (void *)FakeIsPlayerManaShielded},
		{0x44bad0, (void *)IsPlayerParalyzed, (void *)FakeIsPlayerParalyzed},
		{0x44d6d0, (void *)HasPlayerSkull, (void *)FakeHasPlayerSkull},
		{0x44baf0, (void *)IsPlayerInFight, (void *)FakeIsPlayerInFight},
		{0x44bab0, (void *)IsPlayerDrunk, (void *)FakeIsPlayerDrunk},
		{0x44baa0, (void *)IsPlayerElectrified, (void *)FakeIsPlayerElectrified},
		{0x44bae0, (void *)IsPlayerHasted, (void *)FakeIsPlayerHasted},
		{0x4db690, (void *)GetAttackMode, (void *)FakeGetAttackMode},
		{0x4db660, (void *)SetAttackMode, (void *)FakeSetAttackMode},
		{0x4db6a0, (void *)GetChaseMode, (void *)FakeGetChaseMode},
		{0x4db670, (void *)SetChaseMode, (void *)FakeSetChaseMode},
		{0x4db6b0, (void *)GetSecureMode, (void *)FakeGetSecureMode},
		{0x4db680, (void *)SetSecureMode, (void *)FakeSetSecureMode},
		{0x4db4e0, (void *)GetAutomapZoom, (void *)FakeGetAutomapZoom},
		{0x4db4d0, (void *)SetAutomapZoom, (void *)FakeSetAutomapZoom},
		{0x4dbd50, (void *)GetContentWindowSize, (void *)FakeGetContentWindowSize},
		{0x4dbcd0, (void *)SetContentWindowSize, (void *)FakeSetContentWindowSize},
		{0x4db6e0, (void *)GetLevelBar, (void *)FakeGetLevelBar},
		{0x4db6f0, (void *)GetMagLevelBar, (void *)FakeGetMagLevelBar},
		{0x4dd940, (void *)GetSkillBar, (void *)FakeGetSkillBar},
		{0x4db640, (void *)GetClassicControl, (void *)FakeGetClassicControl},
		{0x4b08c0, (void *)GetMapPosition, (void *)FakeGetMapPosition},
		{0x4b09d0, (void *)GetAutomapChanged, (void *)FakeGetAutomapChanged},
		{0x4b4370, (void *)GetAutomapColor, (void *)FakeGetAutomapColor},
		{0x4b4680, (void *)GetAutomapMarks, (void *)FakeGetAutomapMarks},
		{0x451400, (void *)SetPlayerDestination, (void *)FakeSetPlayerDestination},
		{0x4d1c40, (void *)(bool (*)(int, int))GetObjectFlag, (void *)FakeGetObjectFlag},
		{0x4a2a70, (void *)SetHelpHandle, (void *)FakeSetHelpHandle},
		{0x4a2a50, (void *)SetHelpSize, (void *)FakeSetHelpSize},
		{0x4a2a20, (void *)MoveHelpPosition, (void *)FakeMoveHelpPosition},
		{0x4a10a0, (void *)AddHotkey, (void *)FakeAddHotkey},
		{0x4a12a0, (void *)RemoveHotkey, (void *)FakeRemoveHotkey},
		{0x490c60, (void *)DrawPicturePart, (void *)FakeDrawPicturePart},
		{0x4910e0, (void *)DrawPicturePartClipped, (void *)FakeDrawPicturePartClipped},
		{0x494ca0, (void *)DrawObject, (void *)FakeDrawObject},
		{0x495080, (void *)DrawOutfit, (void *)FakeDrawOutfit},
		{0x493f60, (void *)PrintFormattedText, (void *)FakePrintFormattedText},
		{0x4941d0, (void *)PrintFormattedTextClipped, (void *)FakePrintFormattedTextClipped},
		{0x4934e0, (void *)PrintTextClipped, (void *)FakePrintTextClipped},
		{0x491f50, (void *)GetFittingTextLength, (void *)FakeGetFittingTextLength},
		{0x491710, (void *)GetFontHeight, (void *)FakeGetFontHeight},
		{0x48b430, (void *)GetFontLineHeight, (void *)FakeGetFontLineHeight},
		{0x4eaa90, (void *)GetSurface, (void *)FakeGetSurface},
		// Communication.cpp: ours are this file's definitions (NULL) or other tests' (redirected).
		{0x405ef0, NULL, (void *)FakeSendCloseContainer},
		{0x4060a0, NULL, (void *)FakeSendContainerUp},
		{0x407eb0, NULL, (void *)FakeSendFightModes},
		{0x4051f0, NULL, (void *)FakeSendInspectTrade},
		{0x4053b0, NULL, (void *)FakeSendAcceptTrade},
		{0x405550, NULL, (void *)FakeSendRejectTrade},
		{0x404db0, (void *)SendMoveObject, (void *)FakeSendMoveObject},
		{0x4058f0, (void *)SendUseObjectWith, (void *)FakeSendUseObjectWith},
		{0x4056f0, (void *)SendUseObject, (void *)FakeSendUseObject},
		{0x4065e0, (void *)SendLook, (void *)FakeSendLook},
		{0x405b20, (void *)SendUseObjectOnCreature, (void *)FakeSendUseObjectOnCreature},
		{0x405000, (void *)SendRequestTrade, (void *)FakeSendRequestTrade},
		{0x408080, (void *)SendAttack, (void *)FakeSendAttack},
		{0x407800, (void *)SendOpenPrivateConversation, (void *)FakeSendOpenPrivateConversation},
		{0x414700, (void *)SendWalk, (void *)FakeSendWalk},
		{0x414a10, (void *)SendStop, (void *)FakeSendStop},
		// The CRT: the fixed clock, the printf (ours too), sprintf, _snprintf and
		// operator new and new[] (the pattern).
		{0x5121ea, (void *)_time32, (void *)FakeTime},
		{0x512292, NULL, (void *)FakePrintf},
		{0x511d6b, NULL, (void *)ImageSprintf},
		{0x51233d, NULL, (void *)ImageSnprintf},
		{0x5237e7, (void *)(void *(__cdecl *)(size_t))operator new, (void *)PatternNew},
		{0x523817, (void *)(void *(__cdecl *)(size_t))operator new[], (void *)PatternNew},
		{0x460450, Member(&TScrollBarRange::SetRange), (void *)FakeSetRange},
	};
	// DialogElements.cpp: both sides build the shared fakes (elementfakes.cpp; the
	// separators and object icons are real, their constructors are inline);
	// SetRange is this file's FakeSetRange (patched over the shared one).
	PatchElementFakes(Redirect);
	for (auto &T : Table) {
		Redirect((void *)(uintptr_t)T.Image, T.Fake);
		if (T.Ours != NULL)
			Redirect(T.Ours, T.Fake);
	}
}

// Our printf is a fake only while our side runs (CHECK prints with it).
static uint8 PrintfBytes[5];
static void FakeOurPrintf(bool On)
{
	void *Function = (void *)printf;
	DWORD Protection;
	VirtualProtect(Function, 5, PAGE_EXECUTE_READWRITE, &Protection);
	if (On) {
		memcpy(PrintfBytes, Function, 5);
		PatchJump((uint32)(uintptr_t)Function, (void *)FakePrintf);
	} else {
		memcpy(Function, PrintfBytes, 5);
	}
	VirtualProtect(Function, 5, Protection, &Protection);
	FlushInstructionCache(GetCurrentProcess(), Function, 5);
}

// --- The windows ------------------------------------------------------------------------

typedef void *(__fastcall *CtorFn)(void *, void *);
typedef void *(__fastcall *ContentCtorFn)(void *, void *, int, bool);
typedef void *(__fastcall *ButtonsCtorFn)(void *, void *, TDialogWindow *, TMainWindow *);
typedef TWindow *(__cdecl *CreateFn)(TMainWindow *);
typedef TWindow *(__cdecl *CreateIntFn)(int);
typedef TWindow *(__cdecl *Create0Fn)();
typedef int (__fastcall *IntFn)(void *, void *);
typedef int (__fastcall *IntIntFn)(void *, void *, int);
typedef int (__fastcall *XYFn)(void *, void *, int, int);
typedef int (__fastcall *SlotFn)(void *, void *, int, int, bool);
typedef void (__fastcall *VoidFn)(void *, void *);
typedef void (__fastcall *BoolArgFn)(void *, void *, bool);
typedef void (__fastcall *IntArgFn)(void *, void *, int);
typedef void (__fastcall *PointerArgFn)(void *, void *, void *);
typedef bool (__fastcall *LineFn)(void *, void *, int, char *, int *, int *);
typedef void (__fastcall *OfferFn)(void *, void *, int, int, int *, int *);

// The receiver of the windows' callbacks (the main window in the game).
class TContentReceiver : public TWindow {
public:
	void HandleEvent(int Event) { Logf("REvent%d;", Event); }
};
static TContentReceiver *Receiver;

enum { SC_INVENTORY, SC_SMALL_INVENTORY, SC_AUTOMAP, SC_STATUS, SC_BUTTONS, SC_CONTAINER, SC_SKILLS, SC_BATTLE, SC_TRADE, SC_BUDDIES, NSCENARIOS };
static const char *const ScenarioNames[NSCENARIOS] = {"inventory", "small inventory", "automap", "status", "buttons",
	"container", "skills", "battle", "trade", "buddies"};
static int Scenario, ContainerNumber;
static TDialogWindow *Column[2], *Checked[2];
static TContentWindow *Window[2];

// Runs F for our side and then for the image's.
template <class F> static void Both(F Function)
{
	for (int s = 0; s < 2; s++) {
		Side = s;
		if (s == OURS) {
			// An exception on our side would end the child without a message.
			FakeOurPrintf(true);
			try {
				Function(s);
			} catch (TMyException &E) {
				FakeOurPrintf(false);
				printf("ContentWindows: our side threw: %s (%s)\n", E.GetMessage().c_str(), E.GetOrigin().c_str());
				fflush(stdout);
				abort();
			} catch (...) {
				FakeOurPrintf(false);
				printf("ContentWindows: our side threw\n");
				fflush(stdout);
				abort();
			}
			FakeOurPrintf(false);
		} else {
			Function(s);
		}
	}
	Side = OURS;
}

static TDialogWindow *NewColumn(int s)
{
	if (s == OURS)
		return new TDialogWindow;
	return (TDialogWindow *)At<CtorFn>(0x4856c0)(PatternNew(sizeof(TDialogWindow)), NULL);
}
static TContentWindow *NewContentWindow(int s, int Identifier)
{
	if (s == OURS)
		return new TContentWindow(Identifier, false);
	return (TContentWindow *)At<ContentCtorFn>(0x4854c0)(PatternNew(sizeof(TContentWindow)), NULL, Identifier, false);
}

static TContentWindow *Create(int s)
{
	TMainWindow *Main = (TMainWindow *)(TWindow *)Receiver;
	bool Ours = s == OURS;
	switch (Scenario) {
	case SC_INVENTORY:
		return Ours ? CreateInventoryWindow(Main) : (TContentWindow *)At<CreateFn>(0x4362b0)(Main);
	case SC_SMALL_INVENTORY:
		return Ours ? CreateSmallInventoryWindow(Main) : (TContentWindow *)At<CreateFn>(0x436510)(Main);
	case SC_AUTOMAP:
		return Ours ? new TAutomapWindow : (TContentWindow *)At<CtorFn>(0x4368c0)(PatternNew(0x80), NULL);
	case SC_STATUS:
		return Ours ? new TStatusWindow : (TContentWindow *)At<CtorFn>(0x438310)(PatternNew(0x4c), NULL);
	case SC_BUTTONS:
		return Ours ? new TButtonsWindow(Checked[s], Main)
			: (TContentWindow *)At<ButtonsCtorFn>(0x426480)(PatternNew(0x58), NULL, Checked[s], Main);
	case SC_CONTAINER:
		return Ours ? CreateContainerWindow(ContainerNumber) : (TContentWindow *)At<CreateIntFn>(0x43c520)(ContainerNumber);
	case SC_SKILLS:
		return Ours ? CreateSkillsWindow() : (TContentWindow *)At<Create0Fn>(0x43c920)();
	case SC_BATTLE:
		return Ours ? CreateBattleWindow() : (TContentWindow *)At<Create0Fn>(0x43cc90)();
	case SC_TRADE:
		return Ours ? CreateTradeWindow() : (TContentWindow *)At<Create0Fn>(0x43d000)();
	case SC_BUDDIES:
		return Ours ? CreateBuddylistWindow() : (TContentWindow *)At<Create0Fn>(0x43d390)();
	}
	return NULL;
}

static const char *const WindowKinds[] = {"CW", "Sub", "Column", "Window", "Buttons", "Automap", "SmallInv", "InvView",
	"Combat", "Status", "ContainerBody", "SkillsBody", "BattleBody", "BuddyBody", "TradeBody", "TradeOffers",
	"TradeNames", "TradeButtons", "Receiver"};
static bool IsWindowKind(const std::string &Kind)
{
	for (const char *K : WindowKinds) {
		if (Kind == K)
			return true;
	}
	return false;
}

// The first object of a kind in a tree (depth first; a minimized
// sub-window's body is not a child).
static TComponent *Find(int s, TComponent *Root, const char *Kind)
{
	if (Root == NULL)
		return NULL;
	std::string K = KindOf(s, Root);
	if (K == Kind)
		return Root;
	if (!IsWindowKind(K))
		return NULL;
	for (TComponent *Child = ((TWindow *)Root)->FirstChild; Child != NULL; Child = Child->Next) {
		TComponent *Found = Find(s, Child, Kind);
		if (Found != NULL)
			return Found;
	}
	if (K == "Sub") {
		TSubWindow *W = (TSubWindow *)Root;
		if (W->Body != NULL && W->Body->Parent != W)
			return Find(s, W->Body, Kind);
	}
	return NULL;
}
static TComponent *Find(int s, const char *Kind) { return Find(s, Column[s], Kind); }

// --- Dumps ---------------------------------------------------------------------------------

static void Add(std::string &Out, const char *Format, ...)
{
	char Buffer[1024];
	va_list Args;
	va_start(Args, Format);
	vsnprintf(Buffer, sizeof(Buffer), Format, Args);
	va_end(Args);
	Out += Buffer;
}

static std::string ChildName(int s, TWindow *Parent, TComponent *Child)
{
	std::string N = Name(s, Child);
	if (Child != NULL && Child->Parent != Parent)
		N += "(out)";
	return N;
}
static void DumpGroup(int s, std::string &Out, const char *Label, TRadioButtonGroup *G)
{
	Add(Out, " %s sel %s count %d [", Label, Name(s, G->Selected).c_str(), G->Count);
	for (int i = 0; i < G->Count; i++)
		Add(Out, "%s%s", i > 0 ? "," : "", Name(s, G->Buttons.entry[i - G->Buttons.start]).c_str());
	Add(Out, "]");
}

static void DumpComponent(int s, std::string &Out, TComponent *C, int Depth)
{
	Add(Out, "\n%*s%s %d,%d %dx%d parent %s", Depth * 2, "", Name(s, C).c_str(), C->OffsetX, C->OffsetY, C->Width,
		C->Height, Name(s, C->Parent).c_str());
	for (int i = 0; i < C->DrawingListLength && C->DrawingList != NULL; i++) {
		TDrawing &D = C->DrawingList[i];
		Add(Out, " {%d,%d,%d,%d,%d,%d,%d}", D.skin, D.x, D.y, D.width, D.height, D.srcx, D.srcy);
	}
	std::string Kind = KindOf(s, C);
	auto A = Arguments[s].find(C);
	if (A != Arguments[s].end() && !IsRealElement(Kind)) {
		Add(Out, " %s", A->second.c_str());
		const std::string &Made = A->second;
		if (Made.compare(0, 10, "SkinButton") == 0 || Made.compare(0, 12, "ToggleButton") == 0) {
			TSkinButton *B = (TSkinButton *)C;
			Add(Out, " skins %d,%d group %d", B->ButtonUp, B->ButtonDown, B->RadioButtonGroup != NULL ? 1 : 0);
			if (Made.compare(0, 12, "ToggleButton") == 0)
				Add(Out, " state %d", ((TToggleButton *)C)->State ? 1 : 0);
		} else if (Made.compare(0, 10, "TextToggle") == 0) {
			Add(Out, " state %d", ((TTextToggleButton *)C)->State ? 1 : 0);
		} else if (Made.compare(0, 5, "Label") == 0) {
			auto T = LabelTexts[s].find(C);
			if (T != LabelTexts[s].end())
				Add(Out, " text %s", T->second.c_str());
		} else if (Made.compare(0, 9, "ScrollBar") == 0) {
			TFakeScrollBar *B = (TFakeScrollBar *)C;
			Add(Out, " range %d,%d value %d", ((int *)B->Range->Data00)[0], ((int *)B->Range->Data00)[1], B->Range->Value);
		}
		return;
	}
	bool Content = Kind == "CW" || Kind == "Sub" || Kind == "SmallInv" || Kind == "Automap" || Kind == "Status"
		|| Kind == "Buttons";
	if (IsWindowKind(Kind))
		Add(Out, " cap %s", Name(s, ((TWindow *)C)->Capture).c_str());
	if (Content) {
		TContentWindow *W = (TContentWindow *)C;
		Add(Out, " id %d bg %d,%d drag %d %d,%d,%d", W->Identifier, W->Background ? 1 : 0, W->BackgroundY,
			W->Dragging ? 1 : 0, W->DragX, W->DragY, W->DragBottom);
	}
	if (Kind == "Sub") {
		TSubWindow *W = (TSubWindow *)C;
		Add(Out, " min %d %d %d body %s up %s close %s minbutton %s icon %s title %s", W->IsMinimized ? 1 : 0,
			W->RestoreHeight, W->Data68, ChildName(s, W, W->Body).c_str(), ChildName(s, W, W->UpButton).c_str(),
			ChildName(s, W, W->CloseButton).c_str(), ChildName(s, W, W->MinimizeButton).c_str(),
			ChildName(s, W, W->Icon).c_str(), ChildName(s, W, W->Title).c_str());
		if (W->Body != NULL && W->Body->Parent != W) {
			// A minimized window's body is not a child.
			Add(Out, "\n%*shidden body", Depth * 2 + 2, "");
			DumpComponent(s, Out, W->Body, Depth + 2);
		}
	} else if (Kind == "Column") {
		TDialogWindow *D = (TDialogWindow *)C;
		Add(Out, " area %d,%d,%d,%d dragged %s busy %d free %d,%d", D->Area.x, D->Area.y, D->Area.width, D->Area.height,
			Name(s, D->Dragged).c_str(), D->Busy ? 1 : 0, D->FreeHeight, D->UpdateFreeHeight ? 1 : 0);
	} else if (Kind == "Buttons") {
		TButtonsWindow *W = (TButtonsWindow *)C;
		Add(Out, " check %s %s %s %s %s", Name(s, W->CheckObject).c_str(), Name(s, W->SkillsButton).c_str(),
			Name(s, W->BattleButton).c_str(), Name(s, W->BuddyButton).c_str(), Name(s, W->LogoutButton).c_str());
	} else if (Kind == "Automap") {
		TAutomapWindow *W = (TAutomapWindow *)C;
		Add(Out, " parts %s %s %s %s %s %s %s changed %d centred %d view %d,%d,%d player %d,%d,%d zoom %d",
			Name(s, W->View).c_str(), Name(s, W->CentreButton).c_str(), Name(s, W->FloorUpButton).c_str(),
			Name(s, W->FloorDownButton).c_str(), Name(s, W->ZoomInButton).c_str(), Name(s, W->ZoomOutButton).c_str(),
			Name(s, W->CompassRose).c_str(), W->Changed ? 1 : 0, W->Centred ? 1 : 0, W->ViewX, W->ViewY, W->ViewZ,
			W->PlayerX, W->PlayerY, W->PlayerZ, W->Zoom);
	} else if (Kind == "Rose") {
		Add(Out, " skin %d", ((TCompassRose *)C)->RoseSkin);
	} else if (Kind == "States") {
		TPlayerStates *P = (TPlayerStates *)C;
		Add(Out, " large %d max %d", P->IsLarge ? 1 : 0, P->MaxIcons);
	} else if (Kind == "Combat") {
		TCombatControls *W = (TCombatControls *)C;
		Add(Out, " large %d", W->IsLarge ? 1 : 0);
		DumpGroup(s, Out, "attack", W->AttackModes);
		DumpGroup(s, Out, "chase", W->ChaseModes);
		Add(Out, " stand %s chase %s secure %s", Name(s, W->StandButton).c_str(), Name(s, W->ChaseButton).c_str(),
			Name(s, W->SecureButton).c_str());
	} else if (Kind == "Status") {
		TStatusWindow *W = (TStatusWindow *)C;
		Add(Out, " data %d,%d", W->Data44, W->Data48);
	} else if (Kind == "ContainerBody") {
		TContainerWindowBody *B = (TContainerWindowBody *)C;
		Add(Out, " minimized %d scroll %s container %d count %d capacity %d parent %d", B->Minimized ? 1 : 0,
			Name(s, B->ScrollBar).c_str(), B->Container, B->ObjectCount, B->Capacity, B->HasParent ? 1 : 0);
	} else if (Kind == "SkillsBody") {
		TSkillsWindowBody *B = (TSkillsWindowBody *)C;
		Add(Out, " minimized %d menu %s scroll %s lines %d", B->Minimized ? 1 : 0, Name(s, B->ContextMenu).c_str(),
			Name(s, B->ScrollBar).c_str(), B->Lines);
	} else if (Kind == "BattleBody") {
		TBattleWindowBody *B = (TBattleWindowBody *)C;
		Add(Out, " minimized %d scroll %s opponents %d over '%.31s'", B->Minimized ? 1 : 0, Name(s, B->ScrollBar).c_str(),
			B->NumberOfOpponents, B->MouseOverText);
	} else if (Kind == "BuddyBody") {
		TBuddylistWindowBody *B = (TBuddylistWindowBody *)C;
		Add(Out, " minimized %d scroll %s buddies %d over '%s'", B->Minimized ? 1 : 0, Name(s, B->ScrollBar).c_str(),
			B->NumberOfBuddies, B->MouseOverText == NULL ? "(null)" : B->MouseOverText);
	} else if (Kind == "TradeBody") {
		TTradeWindowBody *B = (TTradeWindowBody *)C;
		Add(Out, " minimized %d parts %s %s %s", B->Minimized ? 1 : 0, Name(s, B->Buttons).c_str(),
			Name(s, B->Offers).c_str(), Name(s, B->Names).c_str());
	} else if (Kind == "TradeOffers") {
		TTradeOffers *W = (TTradeOffers *)C;
		Add(Out, " scroll %s separator %s", Name(s, W->ScrollBar).c_str(), Name(s, W->Separator).c_str());
	} else if (Kind == "TradeNames") {
		TTradeNames *W = (TTradeNames *)C;
		Add(Out, " own %s counter %s separator %s", Name(s, W->OwnName).c_str(), Name(s, W->CounterName).c_str(),
			Name(s, W->Separator).c_str());
	} else if (Kind == "TradeButtons") {
		TTradeButtons *W = (TTradeButtons *)C;
		Add(Out, " cancel %s accept %s wait %s separator %s callback %s/%d", ChildName(s, W, W->CancelButton).c_str(),
			ChildName(s, W, W->AcceptButton).c_str(), ChildName(s, W, W->WaitLabel).c_str(),
			Name(s, W->Separator).c_str(), Name(s, W->Callback.Receiver).c_str(), W->Callback.Event);
	}
	if (IsWindowKind(Kind)) {
		for (TComponent *Child = ((TWindow *)C)->FirstChild; Child != NULL; Child = Child->Next)
			DumpComponent(s, Out, Child, Depth + 1);
	}
}

static std::string Dump(int s)
{
	std::string Out;
	DumpComponent(s, Out, Column[s], 0);
	return Out;
}

static const char *Stage = "";
static FILE *LogFile; // CT_LOGS=<file>: every compared log, for checking the coverage
static void Compare(const char *What)
{
	if (LogFile != NULL)
		fprintf(LogFile, "%s %s: %s\n", ScenarioNames[Scenario], Stage, Log[OURS].c_str());
	std::string D0 = Dump(OURS), D1 = Dump(ORIGINAL);
	CHECK(Log[OURS] == Log[ORIGINAL], "ContentWindows %s (%s, %s): logs differ\n  ours: %s\n  orig: %s", What,
		ScenarioNames[Scenario], Stage, Log[OURS].c_str(), Log[ORIGINAL].c_str());
	CHECK(D0 == D1, "ContentWindows %s (%s, %s): windows differ\n  ours: %s\n  orig: %s", What, ScenarioNames[Scenario],
		Stage, D0.c_str(), D1.c_str());
	Log[OURS].clear();
	Log[ORIGINAL].clear();
}
static void CompareResults(const std::string *Results, const char *What)
{
	CHECK(Results[OURS] == Results[ORIGINAL], "ContentWindows %s (%s): %s, original %s", What, ScenarioNames[Scenario],
		Results[OURS].c_str(), Results[ORIGINAL].c_str());
}

// --- The world ------------------------------------------------------------------------------

static TObject RandomObject(bool MayBeEmpty)
{
	TObject Obj;
	static const int Types[] = {100, 101, 1987, 2148, 2152, 2160, 3031, 1988, 2000, 3976, 2543};
	if (MayBeEmpty && Random() % 4 == 0)
		return Obj;
	Obj.Type = Types[Random() % 11];
	Obj.Data = Random() % 3 == 0 ? 1 + Random() % 100 : Random() % 2;
	Obj.Data2 = Random() % 3;
	return Obj;
}

static void RandomWorld(bool Big)
{
	World.ActionState = Random() % 3 != 0 ? 0 : 6 + Random() % 3;
	World.MouseX = (int)(Random() % 260) - 10;
	World.MouseY = (int)(Random() % 400) - 10;
	World.BlinkState = Random() % 2 == 0;
	World.AutomapChanged = Random() % 3 == 0;
	World.SurfaceLost = Random() % 6 == 0;
	World.Destination = Random() % 3;
	World.DragIndex = Random() % 3;
	World.DragX = Random() % 3 == 0 ? 0xffff : 1000 + Random() % 5;
	World.DragY = Random() % 3 == 0 ? (int)(Random() % 11) : (Random() % 2 == 0 ? 0x40 + (int)(Random() % NCONTAINERS) : 1000);
	World.DragZ = Random() % 4 == 0 ? (int)(Random() % 20) : 7;
	World.DragObject = RandomObject(false);
	World.UseIndex = Random() % 3;
	World.UseX = 1000 + Random() % 3;
	World.UseY = 1000;
	World.UseZ = 7;
	World.UseObject = RandomObject(false);
	World.TradeIndex = Random() % 2;
	World.TradeX = 0xffff;
	World.TradeY = Random() % 11;
	World.TradeZ = 0;
	World.TradeObject = RandomObject(false);
	World.ClassicControl = Random() % 2 == 0;
	World.FreeContainer = Random() % 16;
	World.AttackID[0] = World.AttackID[1] = Random() % 3 == 0 ? OpponentList[Random() % NOPPONENTS].CreatureID : 0;
	World.FollowID = Random() % 3 == 0 ? OpponentList[Random() % NOPPONENTS].CreatureID : 0;
	if (!Big)
		return;
	World.MaxHealth = 100 + Random() % 500;
	World.Health = Random() % 4 == 0 ? World.MaxHealth + (int)(Random() % 10) : (int)(Random() % World.MaxHealth);
	World.MaxMana = Random() % 4 == 0 ? 0 : 1 + Random() % 800;
	World.Mana = World.MaxMana == 0 ? 0 : (int)(Random() % (World.MaxMana + 5));
	World.Capacity = Random() % 3000;
	World.Soul = Random() % 200;
	World.Experience = Random() % 3 == 0 ? (int)(Random() % 100000000) : (int)(Random() % 5000);
	World.Level = 1 + Random() % 300;
	World.LevelProgress = Random() % 101;
	World.MagicLevel = Random() % 90;
	World.MagicProgress = Random() % 101;
	for (int i = 0; i < 7; i++) {
		World.SkillValue[i] = 10 + Random() % 120;
		World.SkillProgress[i] = Random() % 101;
		World.SkillBar[i] = Random() % 2 == 0;
	}
	for (int i = 0; i < 9; i++)
		World.States[i] = Random() % 3 == 0;
	Player.PKFlag = Random() % 3 == 0 ? (int)(Random() % 6) : 0;
	World.LevelBar = Random() % 2 == 0;
	World.MagLevelBar = Random() % 2 == 0;
	World.SecureMode[0] = World.SecureMode[1] = Random() % 2 == 0;
	World.InParty = Random() % 2 == 0;
	World.AttackMode[0] = World.AttackMode[1] = 1 + Random() % 3;
	World.ChaseMode[0] = World.ChaseMode[1] = Random() % 2;
	static const int Zooms[] = {1, 2, 4, 8, 3, 0};
	World.AutomapZoom = Zooms[Random() % 6];
	for (int i = 0; i < 10; i++)
		World.WindowSize[i] = Random() % 3 == 0 ? 0 : 20 + (int)(Random() % 200);
	for (int i = 0; i < 11; i++)
		World.Body[i] = RandomObject(true);
	for (int c = 0; c < NCONTAINERS; c++) {
		World.ContainerCapacity[c] = Random() % 4 == 0 ? 36 : 1 + (int)(Random() % 24);
		World.ObjectCount[c] = (int)(Random() % (World.ContainerCapacity[c] + 1));
		for (int i = 0; i < 24; i++)
			World.Contents[c][i] = RandomObject(Random() % 6 == 0);
		World.ContainerObject[c] = RandomObject(false);
		World.HasParent[c] = Random() % 2 == 0;
		World.ContainerOpen[c] = Random() % 5 != 0;
	}
	World.OwnCount = Random() % 13;
	World.CounterCount = Random() % 13;
	for (int i = 0; i < 12; i++) {
		World.Own[i] = RandomObject(false);
		World.Counter[i] = RandomObject(false);
	}
	World.Opponents = Random() % (NOPPONENTS + 1);
	for (int i = 0; i < NOPPONENTS; i++) {
		TCreature &C = OpponentList[i];
		C.OutfitType = 128 + i;
		for (int j = 0; j < 4; j++)
			C.Colors[j] = (int)(Random() % 133);
		C.Health = Random() % 101;
		C.PKFlag = Random() % 3 == 0 ? (int)(Random() % 6) : 0;
		C.PARTYFlag = Random() % 3 == 0 ? (int)(Random() % 6) : 0;
		C.SkullTime = Random() % 3 == 0 ? 0 : 1700000000 - (int)(Random() % 8);
		C.RealX = 1000 + i;
		C.RealY = 1000 - i;
		C.RealZ = 7;
	}
	World.Buddies = Random() % (NBUDDIES + 1);
	for (int i = 0; i < NBUDDIES; i++) {
		World.BuddyOnline[i] = Random() % 2 == 0;
		World.BuddyNew[i] = Random() % 4 == 0;
		World.BuddyIcon[i] = Random() % 3 == 0 ? 0 : (int)(Random() % 11);
	}
	World.PlayerX = 32000 + (int)(Random() % 600);
	World.PlayerY = 31500 + (int)(Random() % 600);
	World.PlayerZ = Random() % 16;
	// Sometimes next to the map's corner, where marks with a coordinate 0 are in the view.
	if (Random() % (Scenario == SC_AUTOMAP ? 3 : 8) == 0) {
		World.PlayerX = 20 + (int)(Random() % 40);
		World.PlayerY = (int)(Random() % 7);
	}
}

// --- Operations -----------------------------------------------------------------------------

static int RandomFlags()
{
	static const int Flags[] = {0, 0, 0, 0x200, 0x400, 0x600, 0x800};
	return Flags[Random() % 7];
}

// A point in a component: random, or over one of its slots, rows or icons.
static void RandomPoint(TComponent *C, const std::string &Kind, int &x, int &y)
{
	int Width = C->Width > 0 ? C->Width : 1, Height = C->Height > 0 ? C->Height : 1;
	x = (int)(Random() % Width);
	y = (int)(Random() % Height);
	if (Random() % 2 != 0)
		return;
	int i = Random() % 12;
	if (Kind == "InvView") {
		TRect &Area = SkinProperties.Style288.Data60[i % 10];
		x = Area.x + (int)(Random() % (Area.width + 2));
		y = Area.y + (int)(Random() % (Area.height + 2));
	} else if (Kind == "ContainerBody") {
		x = SkinProperties.Data630.x + i % SkinProperties.Data62C * SkinProperties.Data648 + (int)(Random() % 36);
		y = SkinProperties.Data630.y + i / SkinProperties.Data62C * SkinProperties.Data64C + (int)(Random() % 36);
	} else if (Kind == "BattleBody") {
		y = i * SkinProperties.Style688.Data00 + (int)(Random() % SkinProperties.Style688.Data00);
	} else if (Kind == "BuddyBody") {
		y = i * SkinProperties.BuddylistWindow.Data04 + (int)(Random() % SkinProperties.BuddylistWindow.Data04);
	} else if (Kind == "States") {
		x = SkinProperties.PlayerStates.Data00.x + i * SkinProperties.PlayerStates.Data24 + (int)(Random() % 6);
		y = SkinProperties.PlayerStates.Data00.y + (int)(Random() % 6);
	} else if (Kind == "AutomapView" || Kind == "Automap") {
		// Near the centre of the view, where the marks next to the player are.
		x = 53 + (int)(Random() % 41) - 20;
		y = 53 + (int)(Random() % 41) - 20;
		if (Kind == "Automap") {
			x += SkinProperties.Data234.x;
			y += SkinProperties.Data234.y;
		}
	} else if (Kind == "Rose") {
		static const int Points[][2] = {{0, 0}, {0, -12}, {0, 12}, {-12, 0}, {12, 0}, {6, 6}, {-5, 5}, {5, -5}};
		x = C->Width / 2 + Points[i % 8][0];
		y = C->Height / 2 + Points[i % 8][1];
	}
	if (x >= Width)
		x = Width - 1;
	if (y >= Height)
		y = Height - 1;
	if (x < 0)
		x = 0;
	if (y < 0)
		y = 0;
}

// The objects of the round a mouse event or Arrange can go to.
static const char *const TargetKinds[] = {"CW", "Sub", "SmallInv", "Automap", "Status", "Buttons", "AutomapView",
	"Rose", "States", "InvView", "Combat", "ContainerBody", "SkillsBody", "BattleBody", "BuddyBody", "TradeBody",
	"TradeOffers", "TradeNames", "TradeButtons", "Capacity", "Soul"};
static const char *RandomTarget()
{
	for (int Try = 0; Try < 40; Try++) {
		const char *Kind = TargetKinds[Random() % (sizeof(TargetKinds) / sizeof(TargetKinds[0]))];
		if (Find(OURS, Kind) != NULL)
			return Kind;
	}
	return NULL;
}

static void Mouse(TComponent *Root[2], const std::string &Kind)
{
	int x, y;
	RandomPoint(Root[OURS], Kind, x, y);
	int Flags = RandomFlags();
	int What = Random() % 9;
	static const int Codes[] = {1, 2, 6};
	int Code = Codes[Random() % 3];
	Both([&](int s) {
		TComponent *C = Root[s];
		switch (What) {
		case 0:
		case 1: {
			bool Taken = C->LeftButtonDown(x, y, Flags);
			Logf("=%d;", Taken ? 1 : 0);
			// A press is always followed by a release (a drag when What is 1).
			if (What == 1)
				Logf("=%d;", C->MouseMove(x + 7, y + 9) ? 1 : 0);
			Logf("=%d;", C->LeftButtonUp(x + What * 7, y + What * 9, Flags) ? 1 : 0);
			break;
		}
		case 2:
			Logf("=%d;", C->LeftButtonUp(x, y, Flags) ? 1 : 0);
			break;
		case 3:
			Logf("=%d;", C->RightButtonUp(x, y, Flags) ? 1 : 0);
			break;
		case 4:
			Logf("=%d;", C->LeftButtonDoubleClick(x, y, Flags) ? 1 : 0);
			Logf("=%d;", C->LeftButtonUp(x, y, Flags) ? 1 : 0);
			break;
		case 5:
			Logf("=%d;", C->MouseWheel(x, y, Code) ? 1 : 0);
			break;
		case 6:
			C->MouseHover(x, y);
			break;
		case 7:
			Logf("=%d;", C->HelpMouseMove(x, y) ? 1 : 0);
			break;
		case 8:
			Logf("=%d;", C->MouseMove(x, y) ? 1 : 0);
			break;
		}
	});
}

static void MouseOnTarget()
{
	const char *Kind = RandomTarget();
	if (Kind == NULL)
		return;
	TComponent *Root[2] = {Find(OURS, Kind), Find(ORIGINAL, Kind)};
	// The hidden body of a minimized window gets no mouse events.
	if (Root[OURS]->Parent == NULL)
		return;
	Mouse(Root, Kind);
}
static void MouseOnWindow()
{
	TComponent *Root[2] = {Window[OURS], Window[ORIGINAL]};
	Mouse(Root, KindOf(OURS, Window[OURS]));
}

static void Draw()
{
	int Surface = Random() % 3, x = Random() % 40, y = Random() % 40;
	bool Whole = Random() % 2 == 0;
	Both([&](int s) {
		if (Whole)
			Column[s]->Draw(Surface, x, y);
		else
			Window[s]->Draw(Surface, x, y);
	});
}

static void Size()
{
	int Width = 150 + Random() % 100, Height = 10 + Random() % 300;
	if (Random() % 3 == 0)
		Width = SkinProperties.Data184;
	bool Columns = Random() % 4 == 0;
	Both([&](int s) {
		if (Columns)
			Column[s]->SetSize(Width, 200 + Height);
		else
			Window[s]->SetSize(Width, Height);
	});
}

static void Arrange()
{
	const char *Kind = Random() % 3 == 0 ? NULL : RandomTarget();
	Both([&](int s) {
		TComponent *C = Kind == NULL ? (TComponent *)Window[s] : Find(s, Kind);
		C->Arrange();
	});
}

// The trade buttons may get the counter offer only while they wait, the
// accept only while "Accept" is shown.
static void Events()
{
	std::string Kind;
	int Event = 0;
	switch (Random() % 5) {
	case 0: {
		static const int List[] = {3, 4, 106, 107, 108, 109, 110, 111, 112, 5, 0, 113};
		Kind = "Automap";
		Event = List[Random() % 12];
		break;
	}
	case 1:
		Kind = "Combat";
		Event = 99 + Random() % 8;
		break;
	case 2:
		Kind = "ContainerBody";
		Event = 2 + Random() % 2;
		break;
	case 3: {
		Kind = "TradeBody";
		TTradeWindowBody *B = (TTradeWindowBody *)Find(OURS, "TradeBody");
		if (B == NULL)
			return;
		// The counter offer while waiting, the accept while "Accept" is shown.
		Event = Random() % 4 == 0 ? 7 : B->Buttons->WaitLabel != NULL ? 21 : B->Buttons->AcceptButton != NULL ? 1 : 7;
		break;
	}
	case 4:
		Kind = "Sub";
		Event = Random() % 2 == 0 ? 9 : 11;
		break;
	}
	if (Find(OURS, Kind.c_str()) == NULL)
		return;
	Both([&](int s) { Find(s, Kind.c_str())->HandleEvent(Event); });
}

static int MarksFound; // GetMark queries that found a mark (ours)
static void Queries()
{
	std::string Results[2];
	char Buffer[256];
	switch (Scenario) {
	case SC_INVENTORY: {
		if (Random() % 2 == 0) {
			int x, y;
			TComponent *V = Find(OURS, "InvView");
			RandomPoint(V, "InvView", x, y);
			Both([&](int s) {
				TInventoryView *View = (TInventoryView *)Find(s, "InvView");
				sprintf(Buffer, "%d", s == OURS ? View->GetSlotAt(x, y) : At<XYFn>(0x42b450)(View, NULL, x, y));
				Results[s] = Buffer;
			});
			CompareResults(Results, "GetSlotAt");
			return;
		}
	}
	// fall through: the inventories have player states
	case SC_SMALL_INVENTORY: {
		if (Scenario == SC_SMALL_INVENTORY && Random() % 2 == 0) {
			// A drop on the small inventory's area (a pile asks for the count).
			TComponent *W = Window[OURS];
			int x = SkinProperties.Data590.x + (int)(Random() % (SkinProperties.Data590.width + 2));
			int y = SkinProperties.Data590.y + (int)(Random() % (SkinProperties.Data590.height + 2));
			int Flags = RandomFlags();
			World.ActionState = 6;
			if (Random() % 2 == 0) {
				World.DragObject.Type = 2543;
				World.DragObject.Data = 2 + Random() % 50;
			}
			if (x >= W->Width || y >= W->Height)
				return;
			Both([&](int s) { Logf("=%d;", Window[s]->LeftButtonUp(x, y, Flags) ? 1 : 0); });
			return;
		}
		int x, y, What = Random() % 3, Position = Random() % 9;
		TComponent *P = Find(OURS, "States");
		RandomPoint(P, "States", x, y);
		Both([&](int s) {
			TPlayerStates *States = (TPlayerStates *)Find(s, "States");
			int Result;
			if (What == 0)
				Result = s == OURS ? States->GetNumberOfIcons() : At<IntFn>(0x4295b0)(States, NULL);
			else if (What == 1)
				Result = s == OURS ? States->GetIconAt(x, y) : At<XYFn>(0x4297a0)(States, NULL, x, y);
			else
				Result = s == OURS ? States->GetIcon(Position) : At<IntIntFn>(0x429990)(States, NULL, Position);
			sprintf(Buffer, "%d", Result);
			Results[s] = Buffer;
		});
		CompareResults(Results, "player states");
		return;
	}
	case SC_AUTOMAP: {
		int x, y, Direction = Random() % 6;
		RandomPoint(Find(OURS, "AutomapView"), "AutomapView", x, y);
		if (Random() % 4 == 0) {
			x = (int)(Random() % 120) - 7;
			y = (int)(Random() % 120) - 7;
		}
		int What = Random() % 3;
		bool Mark = What == 0;
		TAutomapWindow *Ours = (TAutomapWindow *)Window[OURS];
		if (What == 2 && World.PlayerY < 7 && Ours->Centred && Random() % 2 == 0) {
			// Over the mark on the edge (TAutomapWindow::MouseHover needs both coordinates).
			x = 53 + (int)(Random() % 5) - 2;
			y = 53 - World.PlayerY * Ours->Zoom / 2 + (int)(Random() % 5) - 2;
		}
		Both([&](int s) {
			TAutomapWindow *W = (TAutomapWindow *)Window[s];
			if (What == 2) {
				// The window's hover over the view (the mark texts).
				W->MouseHover(x + W->View->OffsetX, y + W->View->OffsetY);
			} else if (Mark) {
				TMark M = W->View->GetMark(x, y);
				sprintf(Buffer, "%d,%d,%d,%s", M.x, M.y, M.Type, M.Text);
				if (s == OURS && (M.x != 0 || M.y != 0))
					MarksFound++;
				Results[s] = Buffer;
			} else if (s == OURS) {
				W->Scroll(Direction);
			} else {
				At<IntArgFn>(0x4274c0)(W, NULL, Direction);
			}
		});
		CompareResults(Results, "automap");
		return;
	}
	case SC_CONTAINER: {
		int x, y, What = Random() % 4;
		bool Occupied = Random() % 2 == 0;
		TComponent *B = Find(OURS, "ContainerBody");
		RandomPoint(B, "ContainerBody", x, y);
		Both([&](int s) {
			TContainerWindowBody *Body = (TContainerWindowBody *)Find(s, "ContainerBody");
			if (What == 0) {
				sprintf(Buffer, "%d", s == OURS ? Body->GetSlotAt(x, y, Occupied)
					: At<SlotFn>(0x42e2c0)(Body, NULL, x, y, Occupied));
				Results[s] = Buffer;
			} else if (What == 1) {
				if (s == OURS)
					Body->Update();
				else
					At<VoidFn>(0x439100)(Body, NULL);
			} else if (What == 2) {
				if (s == OURS)
					Body->UpdateUpButton();
				else
					At<VoidFn>(0x42e4e0)(Body, NULL);
			} else {
				sprintf(Buffer, "%d,%d,%d,%d", Body->GetInitialHeight(), Body->GetMaximumHeight(),
					Body->GetContentHeight(), Body->IsMinimized() ? 1 : 0);
				Results[s] = Buffer;
			}
		});
		CompareResults(Results, "container body");
		return;
	}
	case SC_SKILLS: {
		int Line = 1 + Random() % 15;
		bool Heights = Random() % 3 == 0;
		Both([&](int s) {
			TSkillsWindowBody *Body = (TSkillsWindowBody *)Find(s, "SkillsBody");
			if (Heights) {
				sprintf(Buffer, "%d,%d,%d,%d", Body->GetInitialHeight(), Body->GetMaximumHeight(),
					Body->GetContentHeight(), Body->getMinimumHeight());
				Results[s] = Buffer;
				return;
			}
			char Text[100] = "";
			int Value = -7, Progress = -7;
			bool Result = s == OURS ? Body->GetLine(Line, Text, Value, Progress)
				: At<LineFn>(0x42f6a0)(Body, NULL, Line, Text, &Value, &Progress);
			sprintf(Buffer, "%d,%s,%d,%d", Result ? 1 : 0, Result ? Text : "", Value, Progress);
			Results[s] = Buffer;
		});
		CompareResults(Results, "GetLine");
		return;
	}
	case SC_BATTLE:
	case SC_BUDDIES: {
		const char *Kind = Scenario == SC_BATTLE ? "BattleBody" : "BuddyBody";
		int x, y;
		TComponent *B = Find(OURS, Kind);
		RandomPoint(B, Kind, x, y);
		bool Heights = Random() % 3 == 0;
		Both([&](int s) {
			TSubWindowBody *Body = (TSubWindowBody *)Find(s, Kind);
			if (Heights) {
				sprintf(Buffer, "%d,%d,%d,%d", Body->GetInitialHeight(), Body->GetMaximumHeight(),
					Body->GetContentHeight(), Body->getMinimumHeight());
			} else if (Scenario == SC_BATTLE) {
				sprintf(Buffer, "%d", s == OURS ? ((TBattleWindowBody *)Body)->GetOpponentAt(x, y)
					: At<XYFn>(0x430be0)(Body, NULL, x, y));
			} else {
				sprintf(Buffer, "%d", s == OURS ? ((TBuddylistWindowBody *)Body)->GetBuddyAt(x, y)
					: At<XYFn>(0x4360c0)(Body, NULL, x, y));
			}
			Results[s] = Buffer;
		});
		CompareResults(Results, Kind);
		return;
	}
	case SC_TRADE: {
		int What = Random() % 5, x, y;
		bool Cancel = Random() % 2 == 0;
		TComponent *O = Find(OURS, "TradeOffers");
		RandomPoint(O, "TradeOffers", x, y);
		TTradeWindowBody *OurBody = (TTradeWindowBody *)Find(OURS, "TradeBody");
		bool Waiting = OurBody->Buttons->WaitLabel != NULL, Accepting = OurBody->Buttons->AcceptButton != NULL;
		Both([&](int s) {
			TTradeWindowBody *Body = (TTradeWindowBody *)Find(s, "TradeBody");
			if (What == 0) {
				int Side_ = -7, Position = -7;
				if (s == OURS)
					Body->Offers->GetOfferAt(x, y, &Side_, &Position);
				else
					At<OfferFn>(0x432230)(Body->Offers, NULL, x, y, &Side_, &Position);
				sprintf(Buffer, "%d,%d", Side_, Position);
				Results[s] = Buffer;
			} else if (What == 1) {
				sprintf(Buffer, "%d", s == OURS ? Body->Offers->GetContentHeight() : At<IntFn>(0x431b40)(Body->Offers, NULL));
				Results[s] = Buffer;
			} else if (What == 2) {
				if (s == OURS)
					Body->Buttons->SetCancelButton(Cancel);
				else
					At<BoolArgFn>(0x433720)(Body->Buttons, NULL, Cancel);
			} else if (What == 3) {
				sprintf(Buffer, "%d,%d,%d,%d", Body->GetInitialHeight(), Body->GetMaximumHeight(),
					Body->GetContentHeight(), Body->getMinimumHeight());
				Results[s] = Buffer;
			} else {
				// The partner's offer (as event 21) or the accept (as event 1), when allowed.
				TTradeButtons *B = Body->Buttons;
				if (Waiting) {
					if (s == OURS)
						B->ShowAcceptButton(Body);
					else
						At<PointerArgFn>(0x433940)(B, NULL, Body);
				} else if (Accepting) {
					if (s == OURS)
						B->Wait();
					else
						At<VoidFn>(0x433c40)(B, NULL);
				}
			}
		});
		CompareResults(Results, "trade");
		return;
	}
	default:
		return;
	}
}

static int ActionCount[12];
static void Action()
{
	RandomWorld(Random() % 6 == 0);
	int A = Random() % 12;
	ActionCount[A]++;
	if (getenv("CT_TRACE") != NULL)
		fprintf(stderr, "    kind %d\n", A);
	switch (A) {
	case 0: Size(); Stage = "size"; break;
	case 1: Arrange(); Stage = "arrange"; break;
	case 2: case 3: Draw(); Stage = "draw"; break;
	case 4: MouseOnWindow(); Stage = "mouse on the window"; break;
	case 5: case 6: case 7: MouseOnTarget(); Stage = "mouse on a part"; break;
	case 8: Events(); Stage = "event"; break;
	case 9: case 10: Queries(); Stage = "query"; break;
	case 11: RandomWorld(true); Draw(); Stage = "new world"; break;
	}
}

static void Build()
{
	Pattern = (uint8)Random();
	Scenario = Random() % NSCENARIOS;
	ContainerNumber = Random() % NCONTAINERS;
	bool Has[3] = {Random() % 2 == 0, Random() % 2 == 0, Random() % 2 == 0};
	ScrollMinimumHeight = Random() % 60;
	Both([&](int s) {
		Serial[s] = 0;
		Roles[s][Receiver] = "Receiver";
		Column[s] = NewColumn(s);
		Roles[s][Column[s]] = "Column";
		Checked[s] = NewColumn(s);
		Roles[s][Checked[s]] = "Checked";
		static const int Identifiers[3] = {6, 7, 9};
		for (int i = 0; i < 3; i++) {
			if (Has[i]) {
				TContentWindow *W = NewContentWindow(s, Identifiers[i]);
				Roles[s][W] = "Other";
				Checked[s]->AppendChild(W);
			}
		}
		Column[s]->SetSize(SkinProperties.Data184, 600);
		Window[s] = Create(s);
		Column[s]->Rearrange(16, Window[s]);
	});
}

void TestContentWindows()
{
	int Before = Checks, BeforeFailures = Failures;
	if (getenv("CT_TRACE") != NULL)
		setvbuf(stdout, NULL, _IONBF, 0);
	PatchImage();
	InitSkins();
	Global<int>(0x71c450) = NumberOfSkins;
	Global<TSkin *>(0x71c454) = Skins;
	InitSkinProperties();
	At<void (__cdecl *)()>(0x498160)();
	Surface = new TContentSurface;
	Receiver = new TContentReceiver;
	Player.CreatureID = 0x10000001;
	strcpy(Player.Name, "Tester");
	for (int i = 0; i < NOPPONENTS; i++) {
		OpponentList[i].CreatureID = i == 3 || i == 5 ? 0x10000010 + i : 0x40000010 + i;
		strcpy(OpponentList[i].Name, OpponentNames[i]);
	}
	RandomWorld(true);

	// The vtables of the test classes, from instances.
	{
		TCWTestElement *E = new TCWTestElement;
		ElementVtable = VtableOf(E);
		delete E;
		TCWTestScrollBar *S = new TCWTestScrollBar;
		ScrollVtable = VtableOf(S);
		delete S->Range;
		delete S;
		TCWTestLabel *L = new TCWTestLabel;
		LabelVtable = VtableOf(L);
		delete L;
	}
	for (int s = 0; s < 2; s++) {
		Kinds[s][ElementVtable] = "Element";
		Kinds[s][ScrollVtable] = "Scroll";
		Kinds[s][LabelVtable] = "Element";
	}
	static const struct { uint32 Vtable; const char *Kind; } Images[] = {
		{0x563e38, "CW"}, {0x563ed8, "Sub"}, {0x563f80, "Column"}, {0x564528, "Window"}, {0x55e968, "Buttons"},
		{0x55ea08, "Automap"}, {0x55e688, "AutomapView"}, {0x55eaf0, "Rose"}, {0x55e578, "Capacity"},
		{0x55e600, "Soul"}, {0x55eb98, "States"}, {0x55ec98, "SmallInv"}, {0x55ed38, "InvView"},
		{0x55e880, "Combat"}, {0x55f500, "Status"}, {0x55eed0, "ContainerBody"}, {0x55efa8, "SkillsBody"},
		{0x55e710, "BattleBody"}, {0x55e7c8, "BuddyBody"}, {0x55f370, "TradeBody"}, {0x55f138, "TradeOffers"},
		{0x55f200, "TradeNames"}, {0x55f2a0, "TradeButtons"},
	};
	for (auto &I : Images)
		Kinds[ORIGINAL][(void *)(uintptr_t)I.Vtable] = I.Kind;
	// The real separators and object icons (inline constructors).
	Kinds[ORIGINAL][(void *)(uintptr_t)0x5610b0] = "Separator";
	Kinds[ORIGINAL][(void *)(uintptr_t)0x560d88] = "ObjectIcon";
	{
		TSeparator *Separator = new TSeparator(true);
		TRGBColor Color;
		TObjectIcon *Icon = new TObjectIcon(TObject(), &Color, false);
		Kinds[OURS][VtableOf(Separator)] = "Separator";
		Kinds[OURS][VtableOf(Icon)] = "ObjectIcon";
		delete Separator;
		delete Icon;
	}
	PatchCallees();
	DialogFakes.SetSkins = HookSetSkins;
	DialogFakes.NewResizer = HookResizer;
	{
		// Our vtables, from objects made quietly (the fakes do not log).
		TDialogWindow *D = new TDialogWindow;
		TContentWindow *C = new TContentWindow(1, false);
		TWindow *W = new TWindow;
		TSubWindow *Sub = CreateSkillsWindow();
		TAutomapWindow *A = new TAutomapWindow;
		TCapacityLabel *Cap = new TCapacityLabel;
		TSoulLabel *Soul = new TSoulLabel;
		TPlayerStates *P = new TPlayerStates(true);
		TSmallInventoryWindow *Small = new TSmallInventoryWindow;
		TInventoryView *V = new TInventoryView(NULL);
		TCombatControls *Combat = new TCombatControls(NULL, true);
		TStatusWindow *Status = new TStatusWindow;
		TButtonsWindow *B = new TButtonsWindow(D, NULL);
		TContainerWindowBody *CB = new TContainerWindowBody(0);
		TBattleWindowBody *BB = new TBattleWindowBody;
		TBuddylistWindowBody *VB = new TBuddylistWindowBody;
		TTradeWindowBody *TB = new TTradeWindowBody;
		struct { void *Object; const char *Kind; } List[] = {
			{D, "Column"}, {C, "CW"}, {W, "Window"}, {Sub, "Sub"}, {Sub->Body, "SkillsBody"}, {A, "Automap"},
			{A->View, "AutomapView"}, {A->CompassRose, "Rose"}, {Cap, "Capacity"}, {Soul, "Soul"}, {P, "States"},
			{Small, "SmallInv"}, {V, "InvView"}, {Combat, "Combat"}, {Status, "Status"}, {B, "Buttons"},
			{CB, "ContainerBody"}, {BB, "BattleBody"}, {VB, "BuddyBody"}, {TB, "TradeBody"},
			{TB->Offers, "TradeOffers"}, {TB->Names, "TradeNames"}, {TB->Buttons, "TradeButtons"},
		};
		for (auto &L : List)
			Kinds[OURS][VtableOf(L.Object)] = L.Kind;
		delete TB;
		delete VB;
		delete BB;
		delete CB;
		delete B;
		delete Status;
		delete Combat;
		delete V;
		delete Small;
		delete P;
		delete Soul;
		delete Cap;
		delete A;
		delete Sub;
		delete W;
		delete C;
		delete D;
	}
	Kinds[OURS][VtableOf(Receiver)] = "Receiver";
	Kinds[ORIGINAL][VtableOf(Receiver)] = "Receiver";
	ElementCreated = HookElement;
	Testing = true;
	LogFile = getenv("CT_LOGS") != NULL ? fopen(getenv("CT_LOGS"), "w") : NULL;

	int Rounds = 600;
	for (int Round = 0; Round < Rounds; Round++) {
		if (getenv("CT_TRACE") != NULL)
			fprintf(stderr, "round %d\n", Round);
		RandomWorld(true);
		Stage = "constructor";
		Build();
		if (getenv("CT_TRACE") != NULL)
			fprintf(stderr, "  scenario %s\n", ScenarioNames[Scenario]);
		Compare("constructor");
		int Actions = 20 + Random() % 40;
		for (int i = 0; i < Actions; i++) {
			if (getenv("CT_TRACE") != NULL)
				fprintf(stderr, "  action %d\n", i);
			Action();
			Compare("action");
		}
		Stage = "destructor";
		Both([&](int s) {
			delete Column[s];
			delete Checked[s];
			Column[s] = Checked[s] = NULL;
		});
		CHECK(Log[OURS] == Log[ORIGINAL], "ContentWindows destructor (%s): logs differ\n  ours: %s\n  orig: %s",
			ScenarioNames[Scenario], Log[OURS].c_str(), Log[ORIGINAL].c_str());
		Log[OURS].clear();
		Log[ORIGINAL].clear();
		for (int s = 0; s < 2; s++) {
			Roles[s].clear();
			Arguments[s].clear();
			LabelTexts[s].clear();
		}
	}

	Testing = false;
	if (LogFile != NULL)
		fclose(LogFile);
	ElementCreated = NULL;
	DialogFakes.SetSkins = NULL;
	DialogFakes.NewResizer = NULL;
	RestoreAll();
	delete Receiver;
	delete Surface;
	ExitSkins();
	Global<TSkin *>(0x71c454) = NULL;
	int Least = ActionCount[0];
	for (int i = 1; i < 12; i++)
		Least = ActionCount[i] < Least ? ActionCount[i] : Least;
	printf("ContentWindows: %d checks, %d failures (each action at least %d times, %d marks found)\n",
		Checks - Before, Failures - BeforeFailures, Least, MarksFound);
}
