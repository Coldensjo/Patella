// Differential tests of ConsoleWindow.cpp against the original code.
//
// Every round builds two consoles, one from the image's constructor (0x422410)
// and one from ours, in memory with the same contents, sizes them and opens
// the default channel, as MainWindow.cpp does. Both get the same random
// operations: sizes, drawing (with Talk.cpp's change flag), keys (typing,
// sending the edit line with every kind of command and addressee, the
// selection keys, hotkeys, the history, Tab and Shift+Tab), the console's
// events, opening, adding and closing channels, the rule violation report, the
// server message switch, saving a channel to a file, mouse events in the tab
// bar (clicks and drags) and the text (selections, double and right clicks,
// the wheel), the tab bar's scroll events, the fake buttons' callbacks, and
// new and removed lines in the channels. After each one the logs and a dump of
// both window trees (geometry, drawing lists, captures and every member of
// the five classes, the formatted lines of the text windows with their talk
// lines, the hidden tabs, the fakes' state) are compared; at the end both are
// deleted and the logs compared again.
//
// TComponent, TElement, TWindow and TCallback are the image's for the
// original objects and ours for ours. The DialogElements.cpp classes the
// console creates (the edit line, the scroll bar and the three kinds of
// buttons) are the fakes below on both sides: the image's constructors and
// non-virtual members jump to them. They are also the definitions the other
// tests link (contextmenustest.cpp's edit line, dialogwindowtest.cpp's skin
// buttons) and log only while this test runs. The text buffers are test
// subclasses of Talk.cpp's TTalkBuffer, one set per side with the same lines
// (the image calls their virtual members and its own copies of GetNext,
// GetLine and Write). The other callees (Talk.cpp's functions, the
// preferences, Status.cpp, Help.cpp, the hotkeys, the text measurement and
// output, DrawSkin, the surface, the creatures and Communication.cpp's Send
// functions) are logging fakes for both sides. Failing assertions are not
// exercised: the inputs keep to what the game does (a channel to write in
// has the data its talk mode needs, a dragged tab stays in the bar, Talk.cpp's
// changes are followed by the console's change flag).

#include "difftest.h"
#include "../../src/ConsoleWindow.h"
#include "../../src/Communication.h"
#include "../../src/Creatures.h"
#include "../../src/GUI.h"
#include "../../src/Help.h"
#include "../../src/Preferences.h"
#include "../../src/Skins.h"
#include "../../src/Status.h"
#include "../../src/Surface.h"
#include "../../src/Talk.h"
#include "../../src/Utils.h"
#include "../../src/graphics.h"
#include "../../src/external.h"
#include <direct.h>
#include <map>
#include <new>
#include <stdarg.h>
#include <string>
#include <time.h>
#include <vector>

static_assert(sizeof(TLineFormat) == 0x28, "TLineFormat is 0x28 bytes");
static_assert(offsetof(TLineFormat, FontNumber) == 0x1c && offsetof(TLineFormat, LineEnd) == 0x24, "TLineFormat layout");
static_assert(sizeof(TTextWindow) == 0x94, "TTextWindow is 0x94 bytes");
static_assert(offsetof(TTextWindow, Lines) == 0x28 && offsetof(TTextWindow, CurrentLineNumber) == 0x6c,
	"TTextWindow::Lines, CurrentLineNumber");
static_assert(offsetof(TTextWindow, SelectedText) == 0x8c && offsetof(TTextWindow, AtEnd) == 0x90, "TTextWindow layout");
static_assert(sizeof(TConsole) == 0x38 && offsetof(TConsole, ScrollToEnd) == 0x34, "TConsole layout");
static_assert(sizeof(TTab) == 0x58 && offsetof(TTab, Callback) == 0x4c && offsetof(TTab, Shortened) == 0x54,
	"TTab layout");
static_assert(sizeof(TTabBar) == 0x40 && offsetof(TTabBar, Busy) == 0x3c && offsetof(TTabBar, Changed) == 0x3d,
	"TTabBar layout");
static_assert(sizeof(TConsoleWindow) == 0x6c && offsetof(TConsoleWindow, History) == 0x48
	&& offsetof(TConsoleWindow, TalkMode) == 0x68, "TConsoleWindow layout");
static_assert(sizeof(TEditLine) == 0x68 && offsetof(TEditLine, Text) == 0x2c && offsetof(TEditLine, Data60) == 0x60,
	"TEditLine layout");
static_assert(sizeof(TSkinButton) == 0x44 && sizeof(TToggleButton) == 0x50 && offsetof(TToggleButton, State) == 0x44,
	"button layouts");
static_assert(sizeof(TMultiStateButton) == 0x58, "TMultiStateButton is 0x58 bytes");
static_assert(sizeof(TScrollBar) == 0x38 && offsetof(TScrollBar, Range) == 0x2c && offsetof(TScrollBarRange, Value) == 0x30,
	"scroll bar layout");

// --- Logs and names ------------------------------------------------------------------

enum { OURS = 0, ORIGINAL = 1 };
static int Side;
static bool Testing; // the DialogElements fakes log only while this test runs
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

// The objects of each side by role; the kinds of objects by vtable.
static std::map<const void *, std::string> Roles[2];
static std::map<void *, std::string> Kinds[2];
static int Serial[2];

static std::string Name(int s, const void *Object);
static std::string Name(const void *Object) { return Name(Side, Object); }
static void Role(const void *Object, const char *Base)
{
	char Buffer[64];
	sprintf(Buffer, "%s%d", Base, Serial[Side]++);
	Roles[Side][Object] = Buffer;
}

// ConsoleWindow.obj's copy of the text table (external.h).
const char *TextCloseChannel = "Close this Channel (Ctrl+E)";
const char *TextShowServerMessages = "Show server messages in this channel (Ctrl+M)";
const char *TextHideServerMessages = "Hide server messages in this channel (Ctrl+M)";
const char *TextIgnorePlayers = "Ignore and unignore other players (Ctrl+I)";
const char *TextOpenNewChannel = "Open new channel (Ctrl+O)";
const char *TextAdjustVolume = "Adjust volume";
const char *TextMoveChannelsRight = "Move channels to the right";
const char *TextMoveChannelsLeft = "Move channels to the left";
const char *TextWaitForGamemaster = "Please wait patiently for a gamemaster to reply.";
const char *TextRequestSubmitted = "You have already submitted a request. Please be patient.";
const char *TextHelpChannelWelcome = "Welcome to the help channel! Feel free to ask questions concerning client "
	"controls, general game play, use of accounts and the official homepage. In-depth questions about the content "
	"of the game will not be answered. Experienced players will be glad to help you to the best of their knowledge. "
	"Keep in mind that this is not a chat channel for general conversations. Therefore please limit your "
	"statements to relevant questions and answers.";
const char *TextReportRuleViolation = "Report Rule Violation";

// --- DialogElements.cpp: the fakes of the classes the console creates ------------------

static void *TestEditVtable, *TestScrollVtable, *TestButtonVtable, *TestToggleVtable, *TestMultiVtable;
static int ScrollMinimumHeight;

TEditLine::TEditLine(TRect Area, int MaxLength, int Param)
{
	Text = new char[256];
	Text[0] = 0;
	memset(Data30, 0, sizeof(Data30));
	Data5C = -1;
	Data60 = 0;
	memset(Data64, 0, sizeof(Data64));
	Data66 = false;
	memset(Data67, 0, sizeof(Data67));
	if (Testing) {
		Role(this, "Edit");
		Logf("NEdit%d,%d,%d,%d,%d,%d;", Area.x, Area.y, Area.Width, Area.Height, MaxLength, Param);
		SetVtable(this, TestEditVtable);
	}
}
void TEditLine::SetActive(bool Active)
{
	Logf("EActive%d;", Active ? 1 : 0);
	Data30[0x58 - 0x30] = Active ? 1 : 0;
	if (!Active)
		Data60 = 0;
}
void TEditLine::SetText(const char *_Text)
{
	Logf("ESet%s;", _Text);
	strncpy(Text, _Text, 255);
	Text[255] = 0;
	Data5C = -1;
	Data60 = 0;
}
void TEditLine::Clear()
{
	Logf("EClear;");
	Text[0] = 0;
	Data5C = -1;
	Data60 = 0;
}
void TEditLine::UnknownA8() { Logf("EA8;"); }
void TEditLine::UnknownAC() { Logf("EAC;"); }
void TEditLine::UnknownB0() { Logf("EB0;"); }
void TEditLine::UnknownB4() { Logf("EB4;"); }
void TEditLine::SetReadOnly(bool ReadOnly)
{
	Logf("EReadOnly%d;", ReadOnly ? 1 : 0);
	Data66 = ReadOnly;
}
void TEditLine::UnknownBC() { Logf("EBC;"); }
void TEditLine::SetFont(TFont Font)
{
	Logf("EFont%d,%d,%d,%d,%d;", Font.FontNumber, Font.Color.R, Font.Color.G, Font.Color.B, Font.Alignment);
}
// The console's edit line: typing appends printable characters, Backspace
// deletes one.
class TTestEditLine : public TEditLine {
public:
	TTestEditLine() : TEditLine(TRect(0, 0, 0, 0), 255, 0) {}
	~TTestEditLine() { delete[] Text; }
	void Draw(int Surface, int x, int y) { Logf("DEdit%d,%d,%d,%dx%d;", Surface, x, y, Width, Height); }
	void KeyPress(int Key)
	{
		Logf("EKey%d;", Key);
		size_t Length = strlen(Text);
		if (Key >= 32 && Key < 127 && Length < 255) {
			Text[Length] = (char)Key;
			Text[Length + 1] = 0;
		} else if (Key == 8 && Length > 0) {
			Text[Length - 1] = 0;
		}
	}
	void HandleEvent(int Event) { Logf("EEvent%d;", Event); }
};

TScrollBar::TScrollBar(TRect Area, int Param2, int Param3, int Param4, int Param5, bool Param6)
{
	Range = new TScrollBarRange;
	memset(Range, 0, sizeof(*Range));
	memset(Data30, 0, sizeof(Data30));
	SetSize(Area.Width, Area.Height);
	if (Testing) {
		Role(this, "Scroll");
		Logf("NScroll%d,%d,%d,%d,%d,%d,%d,%d,%d;", Area.x, Area.y, Area.Width, Area.Height, Param2, Param3, Param4,
			Param5, Param6 ? 1 : 0);
		SetVtable(this, TestScrollVtable);
	}
}
// The range is kept in Data00 for the wheel; the value stays inside it.
void TScrollBarRange::SetRange(int Minimum, int Maximum, int _Value, int Visible)
{
	Logf("Range%d,%d,%d,%d;", Minimum, Maximum, _Value, Visible);
	((int *)Data00)[0] = Minimum;
	((int *)Data00)[1] = Maximum;
	if (_Value > Maximum)
		_Value = Maximum;
	if (_Value < Minimum)
		_Value = Minimum;
	Value = _Value;
}
class TTestScrollBar : public TScrollBar {
public:
	TTestScrollBar() : TScrollBar(TRect(0, 0, 0, 0), 0, 0, 0, 0, false) {}
	~TTestScrollBar() { delete Range; }
	int GetMinimumHeight() { return ScrollMinimumHeight; }
	void Draw(int Surface, int x, int y) { Logf("DScroll%d,%d,%d,%dx%d;", Surface, x, y, Width, Height); }
	bool MouseWheel(int x, int y, int Direction)
	{
		Logf("WScroll%d,%d,%d;", x, y, Direction);
		int Minimum = ((int *)Range->Data00)[0], Maximum = ((int *)Range->Data00)[1];
		int Value = Range->Value + (Direction == 1 ? -1 : 1);
		if (Value >= Minimum && Value <= Maximum)
			Range->Value = Value;
		return true;
	}
};

// The button's callback is kept in Data2C (the test presses buttons).
TSkinButton::TSkinButton(int x, int y, int _ButtonUp, int _ButtonDown, TCallback Callback, const char *Tooltip,
	int Param)
{
	ButtonUp = _ButtonUp;
	ButtonDown = _ButtonDown;
	memset(Data2C, 0, sizeof(Data2C));
	memcpy(Data2C, &Callback, sizeof(Callback));
	SetPosition(x, y);
	SetSize(16, 16);
	if (Testing) {
		Role(this, "Button");
		Logf("NButton%d,%d,%d,%d,%s,%d,%s,%d;", x, y, _ButtonUp, _ButtonDown, Name(Callback.Receiver).c_str(),
			Callback.Event, Tooltip == NULL ? "-" : Tooltip, Param);
		SetVtable(this, TestButtonVtable);
	}
}
static TCallback &ButtonCallback(TSkinButton *Button) { return *(TCallback *)Button->Data2C; }
TToggleButton::TToggleButton(int x, int y, int _ButtonUp, int _ButtonDown, TCallback Callback, const char *Tooltip1,
	const char *Tooltip2, int Param)
	: TSkinButton(x, y, _ButtonUp, _ButtonDown, Callback, Tooltip1, Param)
{
	State = false;
	memset(Data45, 0, sizeof(Data45));
	if (Testing) {
		Logf("NToggle%s;", Tooltip2);
		SetVtable(this, TestToggleVtable);
	}
}
static void InitStates(TMultiStateButton *Button, int States, va_list Arguments)
{
	Button->States = States;
	Button->ButtonUps = new int[States + 1];
	Button->ButtonDowns = new int[States + 1];
	for (int i = 0; i < States; i++) {
		Button->ButtonUps[i] = va_arg(Arguments, int);
		Button->ButtonDowns[i] = va_arg(Arguments, int);
		TCallback Callback = va_arg(Arguments, TCallback);
		Logf("State%d,%d,%s,%d;", Button->ButtonUps[i], Button->ButtonDowns[i], Name(Callback.Receiver).c_str(),
			Callback.Event);
	}
}
TMultiStateButton::TMultiStateButton(int x, int y, const char *Tooltip, int _States, ...)
	: TSkinButton(x, y, 0, 0, TCallback(), Tooltip, 0)
{
	State = 0;
	memset(Data54, 0, sizeof(Data54));
	va_list Arguments;
	va_start(Arguments, _States);
	InitStates(this, _States, Arguments);
	va_end(Arguments);
	if (Testing)
		SetVtable(this, TestMultiVtable);
}
void TMultiStateButton::Reset()
{
	Logf("Reset%s;", Name(this).c_str());
	State = 0;
}
class TTestSkinButtonC : public TSkinButton {
public:
	TTestSkinButtonC() : TSkinButton(0, 0, 0, 0, TCallback(), NULL, 0) {}
	void Draw(int Surface, int x, int y)
	{
		Logf("D%s,%d,%d,%d,%d,%d;", Name(this).c_str(), Surface, x, y, ButtonUp, ButtonDown);
	}
};
class TTestToggleButton : public TToggleButton {
public:
	TTestToggleButton() : TToggleButton(0, 0, 0, 0, TCallback(), NULL, NULL, 0) {}
	void Draw(int Surface, int x, int y) { Logf("D%s,%d,%d,%d,%d;", Name(this).c_str(), Surface, x, y, State ? 1 : 0); }
};
class TTestMultiStateButton : public TMultiStateButton {
public:
	TTestMultiStateButton() : TMultiStateButton(0, 0, NULL, 0) {}
	~TTestMultiStateButton()
	{
		delete[] ButtonUps;
		delete[] ButtonDowns;
	}
	void Draw(int Surface, int x, int y) { Logf("D%s,%d,%d,%d,%d;", Name(this).c_str(), Surface, x, y, State); }
};

// The image's constructors build the same objects.
static void *__fastcall ImageEditLine(void *This, void *, TRect Area, int MaxLength, int Param)
{
	return new (This) TEditLine(Area, MaxLength, Param);
}
static void *__fastcall ImageScrollBar(void *This, void *, TRect Area, int P2, int P3, int P4, int P5, bool P6)
{
	return new (This) TScrollBar(Area, P2, P3, P4, P5, P6);
}
static void *__fastcall ImageSkinButton(void *This, void *, int x, int y, int Up, int Down, TCallback Callback,
	const char *Tooltip, int Param)
{
	return new (This) TSkinButton(x, y, Up, Down, Callback, Tooltip, Param);
}
static void *__fastcall ImageToggleButton(void *This, void *, int x, int y, int Up, int Down, TCallback Callback,
	const char *Tooltip1, const char *Tooltip2, int Param)
{
	return new (This) TToggleButton(x, y, Up, Down, Callback, Tooltip1, Tooltip2, Param);
}
// 0x4672d0 is a varargs constructor: __cdecl with this as its first argument.
static void *__cdecl ImageMultiStateButton(void *This, int x, int y, const char *Tooltip, int States, ...)
{
	TMultiStateButton *Button = new (This) TMultiStateButton(x, y, Tooltip, 0);
	delete[] Button->ButtonUps;
	delete[] Button->ButtonDowns;
	va_list Arguments;
	va_start(Arguments, States);
	InitStates(Button, States, Arguments);
	va_end(Arguments);
	return Button;
}

// --- The text buffers ------------------------------------------------------------------

struct TBufferSpec {
	int Recv, Send;
	bool Default, RuleViolation, RuleViolationChannel, PrivateChannel, Writable;
	int ChannelID;
	const char *Addressee;
	const char *Caption;
};

// A channel; the same buffer exists once per side (Index).
class TConsoleBuffer : public TTalkBuffer {
public:
	TConsoleBuffer(int _Index, const TBufferSpec &_Spec) : TTalkBuffer(_Spec.Recv), Index(_Index), Spec(_Spec)
	{
		SendTalkMode = _Spec.Send;
		Writable = _Spec.Writable;
		Closed = false;
	}
	bool CanWrite() { Logf("B%d.Can;", Index); return Writable; }
	bool SetWriteEnabled(bool Value) { Logf("B%d.SetW%d;", Index, Value ? 1 : 0); Writable = Value; return Value; }
	bool isDefault() { Logf("B%d.Def;", Index); return Spec.Default; }
	// The default channel and the gamemasters' rule violation channel (TRuleViolationChannel).
	bool AcceptMessages() { Logf("B%d.Acc;", Index); return Spec.Default || Spec.RuleViolationChannel; }
	bool IsRuleViolation() { Logf("B%d.RV;", Index); return Spec.RuleViolation; }
	bool IsRuleViolationChannel() { Logf("B%d.RVC;", Index); return Spec.RuleViolationChannel; }
	bool IsPrivateChannel() { Logf("B%d.Priv;", Index); return Spec.PrivateChannel; }
	const char *GetAddressee() { Logf("B%d.Addr;", Index); return Spec.Addressee; }
	int GetChannelID() { Logf("B%d.Chan;", Index); return Spec.ChannelID; }
	int Index;
	TBufferSpec Spec;
	bool Writable;
};
static std::vector<TConsoleBuffer *> Buffers[2];
static std::vector<bool> Deleted[2];

static const TBufferSpec Specs[] = {
	// Recv, Send, Default, RV, RVC, Priv, Writable, ChannelID, Addressee, Caption
	{1, 1, true, false, false, false, true, -1, NULL, "Default"},
	{5, 5, false, false, false, false, true, 4, NULL, "Game-Chat"},
	{5, 5, false, false, false, false, true, 5, NULL, "Trade"},
	{5, 5, false, false, false, false, false, 7, NULL, "Help"},
	{4, 4, false, false, false, false, true, -1, "Mr Very Long Name", "Mr Very Long Name"},
	{4, 4, false, false, false, false, true, -1, "Seymour", "Seymour"},
	{5, 5, false, false, false, true, true, 0x1234, NULL, "Tester's Channel"},
	{5, 5, false, false, false, true, false, 0x2345, NULL, "Bubble's Channel"},
	{7, 8, false, true, false, false, true, -1, NULL, "Report Rule Violation"},
	{8, 7, false, true, false, false, true, -1, "Xa", "Xa's Report"},
	{6, 6, false, false, true, false, false, 3, NULL, "Rule Violations"},
	{1, 1, false, false, false, false, true, -1, NULL, "A very long channel name that does not fit"},
	{10, 10, false, false, false, false, true, 9, NULL, "Gamemasters"},
};
enum { NSPECS = sizeof(Specs) / sizeof(Specs[0]) };

static const char *const Texts[] = {"Hello", "a b c d e f g h i j k l m n o p q r s t u v w x y z",
	"Averyveryverylongwordwithoutanyblanksinitatalltobreakitsomewhere and more",
	"line one\nline two\nline three", "  spaces  around  ", "x", "Two words\n\nempty line before",
	"Hello there, how are you doing today? Fine, thanks. And you?", "\xe4\xf6\xfc \xc4\xd6\xdc \xdf"};
enum { NTEXTS = sizeof(Texts) / sizeof(Texts[0]) };

static TTalkLine *CloneLine(const TTalkLine *Line)
{
	TTalkLine *Copy = (TTalkLine *)malloc(sizeof(TTalkLine));
	memcpy(Copy, Line, sizeof(TTalkLine));
	size_t Size = strlen(Line->Line) + 1;
	Copy->Line = (char *)malloc(Size);
	memcpy(Copy->Line, Line->Line, Size);
	Copy->WithoutTime = Copy->Line + (Line->WithoutTime - Line->Line);
	Copy->Text = Copy->Line + (Line->Text - Line->Line);
	Copy->Previous = Copy->Next = NULL;
	return Copy;
}

// A new channel on both sides (the same index), with a few lines.
static void AddRandomLines(int Index, int Count);
static int NewBufferPair(int SpecIndex)
{
	int Index = (int)Buffers[0].size();
	for (int s = 0; s < 2; s++) {
		Buffers[s].push_back(new TConsoleBuffer(Index, Specs[SpecIndex]));
		Deleted[s].push_back(false);
	}
	AddRandomLines(Index, Random() % 4);
	return Index;
}
static void AddRandomLines(int Index, int Count)
{
	static const int Modes[] = {1, 2, 3, 4, 5, 7, 8, 18, 19, 20, 22};
	for (int i = 0; i < Count; i++) {
		int Mode = Modes[Random() % 11];
		const char *Text = Texts[Random() % NTEXTS];
		TTalkLine *Line = Random() % 2 == 0 ? new TTalkLine(Random() % 1000, Mode, Text, 0)
			: new TTalkLine(Random() % 1000, Mode, Random() % 2 ? "Xa" : "Mr Very Long Name", Text, 0);
		bool Quiet = Testing;
		Testing = false;
		Buffers[OURS][Index]->Append(Line);
		Buffers[ORIGINAL][Index]->Append(CloneLine(Line));
		Testing = Quiet;
	}
}
static int LineIndex(const TTalkBuffer *Buffer, const TTalkLine *Line)
{
	int i = 0;
	for (TTalkLine *L = Buffer->first; L != NULL; L = L->Next, i++) {
		if (L == Line)
			return i;
	}
	return -1;
}

// --- The world and the fakes of the callees ----------------------------------------------

struct TConsoleWorld {
	int ActionNumber, ActionState;
	bool ConsoleChanged, Throttled, OwnPrivate, TimeStamps;
	const char *Account, *Password;
	int HotkeyNumber;
	bool HotkeyAuto;
	const char *HotkeyText;
	int OpenChannels[3], OpenCount;
	bool CreateFails;   // CreateTextBuffer returns NULL, GetTextBuffer an existing buffer
	int PendingSpec;    // the spec of the next CreateTextBuffer
	int Lookup;         // the buffer GetTextBuffer returns (-1 NULL)
	int LookupQueue[2], LookupCount, LookupNext; // answers before Lookup
};
static TConsoleWorld World;

static TCreature Player;

static TConsoleBuffer *Buffer(int Index) { return Index < 0 ? NULL : Buffers[Side][Index]; }
static int BufferIndex(const TTalkBuffer *TextBuffer)
{
	for (size_t i = 0; i < Buffers[Side].size(); i++) {
		if (Buffers[Side][i] == TextBuffer)
			return (int)i;
	}
	return -1;
}

static TRGBColor *__cdecl FakeGetTalkColor(TRGBColor *Color, int Mode)
{
	Logf("Color%d;", Mode);
	Color->R = Mode * 10;
	Color->G = Mode * 3;
	Color->B = 200;
	return Color;
}
static bool __cdecl FakeGetConsoleChanged() { Logf("ConsChanged;"); return World.ConsoleChanged; }
static bool __cdecl FakeIsTalkThrottled() { Logf("Throttled;"); return World.Throttled; }
static TTalkBuffer *__cdecl FakeCreateTextBuffer(int Recv, int Send, const char *Name, int Channel)
{
	Logf("Create%d,%d,%s,%d;", Recv, Send, Name == NULL ? "-" : Name, Channel);
	if (World.CreateFails)
		return NULL;
	// One new buffer per call and side; the indices agree because both sides make the same calls.
	int Index = (int)Buffers[Side].size();
	Buffers[Side].push_back(new TConsoleBuffer(Index, Specs[World.PendingSpec]));
	Deleted[Side].push_back(false);
	return Buffers[Side][Index];
}
static TTalkBuffer *__cdecl FakeGetTextBuffer(int Mode, const char *Name, int Channel)
{
	Logf("GetBuf%d,%s,%d;", Mode, Name == NULL ? "-" : Name, Channel);
	if (World.LookupNext < World.LookupCount)
		return Buffer(World.LookupQueue[World.LookupNext++]);
	return Buffer(World.Lookup);
}
static void __cdecl FakeDeleteTextBuffer(TTalkBuffer *TextBuffer)
{
	int Index = BufferIndex(TextBuffer);
	Logf("DelBuf%d;", Index);
	if (Index >= 0)
		Deleted[Side][Index] = true;
}
static void __cdecl FakeAddTextBufferLine(TTalkBuffer *Target, uint32 ID, const char *Text, int Mode, const char *Name,
	int Time)
{
	Logf("AddLine%d,%u,%s,%d,%s,%d;", BufferIndex(Target), ID, Text, Mode, Name == NULL ? "-" : Name, Time);
}
static bool __cdecl FakeHasOwnPrivateChannel() { Logf("OwnPriv;"); return World.OwnPrivate; }
static void __cdecl FakeAddChannelNotice(const char *Addressee, const char *Text, int Mode, const char *Name, int Channel)
{
	Logf("Notice%s,%s,%d,%s,%d;", Addressee, Text, Mode, Name == NULL ? "-" : Name, Channel);
}
static void __cdecl FakeSetStatusMessage(const char *Message, bool Permanent)
{
	Logf("Status%s,%d;", Message, Permanent ? 1 : 0);
}
static bool __cdecl FakeGetTimeStamps() { return World.TimeStamps; }
static int CharWidth(char c) { return 4 + (uint8)c % 4; }
static int __cdecl FakeGetFittingTextLength(int Font, const char *Text, int Width)
{
	Logf("Fit%d,%d;", Font, Width);
	int i = 0, Sum = 0;
	while (Text[i] != 0 && Sum + CharWidth(Text[i]) <= Width)
		Sum += CharWidth(Text[i++]);
	return i;
}
static int __cdecl FakeGetTextPartWidth(int Font, const char *Text, int Length)
{
	Logf("PartW%d,%d;", Font, Length);
	int Sum = 0;
	for (int i = 0; i < Length; i++)
		Sum += CharWidth(Text[i]);
	return Sum;
}
static int __cdecl FakeGetFontHeight(int Font) { Logf("FontH%d;", Font); return 11 + Font % 2; }
static int __cdecl FakeGetFontLineHeight(int Font) { Logf("LineH%d;", Font); return 13 + Font % 3; }
static void __cdecl FakePrintTextPart(int Surface, int x, int y, int Font, TRGBColor Color, const char *Text, int Length,
	int Alignment)
{
	Logf("Print%d,%d,%d,%d,%d,%d,%d,%.*s,%d;", Surface, x, y, Font, Color.R, Color.G, Color.B, Length, Text, Alignment);
}
static void __cdecl FakePrintFormattedText(int Surface, int x, int y, TFont Format, const char *Text)
{
	Logf("PrintF%d,%d,%d,%d,%d,%d,%d,%d,%s;", Surface, x, y, Format.FontNumber, Format.Color.R, Format.Color.G,
		Format.Color.B, Format.Alignment, Text);
}
// DrawSkin is real on both sides (ours is inlined into TComponent::Draw); the
// picture parts it draws are logged.
static void __cdecl FakeDrawPicturePart(int Surface, int x, int y, int PictureNumber, int srcx, int srcy, int Width,
	int Height)
{
	Logf("Part%d,%d,%d,%d,%d,%d,%d,%d;", Surface, x, y, PictureNumber, srcx, srcy, Width, Height);
}
static int __cdecl FakeGetActionNumber() { return World.ActionNumber; }
static int __cdecl FakeGetActionState() { Logf("AState;"); return World.ActionState; }
static void __cdecl FakeAddMessage(int Type, INT_PTR Param1, INT_PTR Param2)
{
	Logf("Msg%d", Type);
	if (Type == 0xc)
		Logf(",%s", (const char *)Param1);
	else
		Logf(",%d", (int)Param1);
	Logf(",%d;", (int)Param2);
}
static void __cdecl FakeCopyToClipboard(const char *Text) { Logf("Clip%s;", Text); }
static void __cdecl FakeSetMouseOverText(const char *Text) { Logf("Over%s;", Text); }
static void __cdecl FakeSetHelpHandle(int Handle) { Logf("HelpH%d;", Handle); }
static void __cdecl FakeSetHelpSize(int Width, int Height) { Logf("HelpS%d,%d;", Width, Height); }
static void __cdecl FakeMoveHelpPosition(int dx, int dy) { Logf("HelpM%d,%d;", dx, dy); }
static void __cdecl FakeAddHotkey(int Key, TCallback Callback)
{
	Logf("AddHK%x,%s,%d;", Key, Name(Callback.Receiver).c_str(), Callback.Event);
}
static void __cdecl FakeRemoveHotkey(int Key) { Logf("RemHK%x;", Key); }
static const char *__cdecl FakeGetHotkey(int Number, bool *AutoSend)
{
	Logf("GetHK%d;", Number);
	if (Number != World.HotkeyNumber)
		return NULL;
	*AutoSend = World.HotkeyAuto;
	return World.HotkeyText;
}
static int __cdecl FakeGetOpenChannels(int **List)
{
	Logf("GetOpen;");
	if (World.OpenCount > 0) {
		*List = new int[World.OpenCount];
		for (int i = 0; i < World.OpenCount; i++)
			(*List)[i] = World.OpenChannels[i];
	}
	return World.OpenCount;
}
static void __cdecl FakeClearOpenChannels() { Logf("ClearOpen;"); }
static void __cdecl FakeAddOpenChannel(int Channel) { Logf("AddOpen%d;", Channel); }
static void __cdecl FakeSetConsoleHeight(int Height) { Logf("SetCH%d;", Height); }
static const char *__cdecl FakeGetAccountNumberString() { Logf("Acc;"); return World.Account; }
static const char *__cdecl FakeGetAccountPassword() { Logf("Pw;"); return World.Password; }
static uint32 __cdecl FakeGetPlayerID() { Logf("PID;"); return Player.CreatureID; }
static TCreature *__cdecl FakeGetCreature(uint32 ID) { Logf("Cr%u;", ID); return ID == Player.CreatureID ? &Player : NULL; }
static void __cdecl FakeSendTalk1(int Mode, const char *Text) { Logf("Talk%d,%s;", Mode, Text); }
static void __cdecl FakeSendTalk2(int Mode, const char *Addressee, const char *Text) { Logf("TalkTo%d,%s,%s;", Mode, Addressee, Text); }
static void __cdecl FakeSendTalk3(int Mode, int Channel, const char *Text) { Logf("TalkIn%d,%d,%s;", Mode, Channel, Text); }
static void __cdecl FakeSendJoinChannel(int Channel) { Logf("Join%d;", Channel); }
static void __cdecl FakeSendLeaveChannel(int Channel) { Logf("Leave%d;", Channel); }
static void __cdecl FakeSendCloseRuleViolation(const char *Name) { Logf("CloseRV%s;", Name); }
static void __cdecl FakeSendCancelRuleViolation() { Logf("CancelRV;"); }
static void __cdecl FakeSendRequestChannels() { Logf("ReqChannels;"); }
static void __cdecl FakeSendInvite(const char *Name) { Logf("Invite%s;", Name); }
static void __cdecl FakeSendExclude(const char *Name) { Logf("Exclude%s;", Name); }
static void HookSetSkins(TSkinButton *Button, int ButtonUp, int ButtonDown)
{
	Logf("Skins%s,%d,%d;", Name(Button).c_str(), ButtonUp, ButtonDown);
	Button->ButtonUp = ButtonUp;
	Button->ButtonDown = ButtonDown;
}

// Communication.cpp is not in the test: our side's definitions of the Send
// functions no other test defines.
void SendTalk(int Mode, const char *Text) { FakeSendTalk1(Mode, Text); }
void SendTalk(int Mode, const char *Addressee, const char *Text) { FakeSendTalk2(Mode, Addressee, Text); }
void SendTalk(int Mode, int ChannelID, const char *Text) { FakeSendTalk3(Mode, ChannelID, Text); }
void SendLeaveChannel(int ChannelID) { FakeSendLeaveChannel(ChannelID); }
void SendCancelRuleViolation() { FakeSendCancelRuleViolation(); }
void SendRequestChannels() { FakeSendRequestChannels(); }

class TConsoleSurface : public TSurfaceGL {
public:
	void FillRectangle(int Surface, TRect R, TRGBColor C)
	{
		Logf("Fill%d,%d,%d,%d,%d,%d,%d,%d;", Surface, R.x, R.y, R.Width, R.Height, C.R, C.G, C.B);
	}
};
static TConsoleSurface *Surface;
static TSurface *__cdecl FakeGetSurface() { return Surface; }

// The clock (fixed on both sides), localtime, ctime and fprintf of the image.
static __time32_t __cdecl FakeTime(__time32_t *Time)
{
	if (Time != NULL)
		*Time = 1700000000;
	return 1700000000;
}
static tm *__cdecl ImageLocaltime(const __time32_t *Time) { return _localtime32(Time); }
static char *__cdecl ImageCtime(const __time32_t *Time) { return _ctime32(Time); }
static int __cdecl ImageFprintf(FILE *File, const char *Format, ...)
{
	va_list Args;
	va_start(Args, Format);
	int Result = vfprintf(File, Format, Args);
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
		{0x4fd9e0, (void *)GetTalkColor, (void *)FakeGetTalkColor},
		{0x4fd580, (void *)GetConsoleChanged, (void *)FakeGetConsoleChanged},
		{0x4fd550, (void *)IsTalkThrottled, (void *)FakeIsTalkThrottled},
		{0x507bc0, (void *)CreateTextBuffer, (void *)FakeCreateTextBuffer},
		{0x5025c0, (void *)GetTextBuffer, (void *)FakeGetTextBuffer},
		{0x507fa0, (void *)DeleteTextBuffer, (void *)FakeDeleteTextBuffer},
		{0x505f10, (void *)AddTextBufferLine, (void *)FakeAddTextBufferLine},
		{0x502880, (void *)HasOwnPrivateChannel, (void *)FakeHasOwnPrivateChannel},
		{0x506720, (void *)AddChannelNotice, (void *)FakeAddChannelNotice},
		{0x4ff500, (void *)SetStatusMessage, (void *)FakeSetStatusMessage},
		{0x4db5f0, (void *)GetTimeStamps, (void *)FakeGetTimeStamps},
		{0x491f50, (void *)GetFittingTextLength, (void *)FakeGetFittingTextLength},
		{0x491b80, (void *)GetTextPartWidth, (void *)FakeGetTextPartWidth},
		{0x491710, (void *)GetFontHeight, (void *)FakeGetFontHeight},
		{0x48b430, (void *)GetFontLineHeight, (void *)FakeGetFontLineHeight},
		{0x492e30, (void *)PrintTextPart, (void *)FakePrintTextPart},
		{0x493f60, (void *)PrintFormattedText, (void *)FakePrintFormattedText},
		{0x490c60, (void *)DrawPicturePart, (void *)FakeDrawPicturePart},
		{0x4eaa90, (void *)GetSurface, (void *)FakeGetSurface},
		{0x4e6e00, (void *)GetActionNumber, (void *)FakeGetActionNumber},
		{0x4e6e30, (void *)GetActionState, (void *)FakeGetActionState},
		{0x4e9020, (void *)AddMessage, (void *)FakeAddMessage},
		{0x4ea5b0, (void *)CopyToClipboard, (void *)FakeCopyToClipboard},
		{0x4e72e0, (void *)SetMouseOverText, (void *)FakeSetMouseOverText},
		{0x4a2a70, (void *)SetHelpHandle, (void *)FakeSetHelpHandle},
		{0x4a2a50, (void *)SetHelpSize, (void *)FakeSetHelpSize},
		{0x4a2a20, (void *)MoveHelpPosition, (void *)FakeMoveHelpPosition},
		{0x4a10a0, (void *)AddHotkey, (void *)FakeAddHotkey},
		{0x4a12a0, (void *)RemoveHotkey, (void *)FakeRemoveHotkey},
		{0x4dc7e0, (void *)GetHotkey, (void *)FakeGetHotkey},
		{0x4dbde0, (void *)GetOpenChannels, (void *)FakeGetOpenChannels},
		{0x4db610, (void *)ClearOpenChannels, (void *)FakeClearOpenChannels},
		{0x4dccb0, (void *)AddOpenChannel, (void *)FakeAddOpenChannel},
		{0x4db4b0, (void *)SetConsoleHeight, (void *)FakeSetConsoleHeight},
		{0x4e7230, (void *)GetAccountNumberString, (void *)FakeGetAccountNumberString},
		{0x4e7240, (void *)GetAccountPassword, (void *)FakeGetAccountPassword},
		{0x44b9c0, (void *)GetPlayerID, (void *)FakeGetPlayerID},
		{0x44c6e0, (void *)GetCreature, (void *)FakeGetCreature},
		// Communication.cpp: ours are this file's definitions (NULL) or other tests' (redirected).
		{0x4067c0, NULL, (void *)FakeSendTalk1},
		{0x406b40, NULL, (void *)FakeSendTalk2},
		{0x406f90, NULL, (void *)FakeSendTalk3},
		{0x407650, NULL, (void *)FakeSendLeaveChannel},
		{0x407d10, NULL, (void *)FakeSendCancelRuleViolation},
		{0x407300, NULL, (void *)FakeSendRequestChannels},
		{0x4074a0, (void *)SendJoinChannel, (void *)FakeSendJoinChannel},
		{0x407b60, (void *)SendCloseRuleViolation, (void *)FakeSendCloseRuleViolation},
		{0x408580, (void *)SendInviteToPrivateChannel, (void *)FakeSendInvite},
		{0x408730, (void *)SendExcludeFromPrivateChannel, (void *)FakeSendExclude},
		// The fixed clock (our CRT's as well, for SaveChannel and the history lines).
		{0x5121ea, (void *)_time32, (void *)FakeTime},
	};
	for (auto &T : Table) {
		Redirect((void *)(uintptr_t)T.Image, T.Fake);
		if (T.Ours != NULL)
			Redirect(T.Ours, T.Fake);
	}
	// DialogElements.cpp: the image's constructors build the fakes, its members are the fakes.
	Redirect((void *)(uintptr_t)0x467b50, (void *)ImageEditLine);
	Redirect((void *)(uintptr_t)0x469a80, (void *)ImageScrollBar);
	Redirect((void *)(uintptr_t)0x466860, (void *)ImageSkinButton);
	Redirect((void *)(uintptr_t)0x4671e0, (void *)ImageToggleButton);
	Redirect((void *)(uintptr_t)0x4672d0, (void *)ImageMultiStateButton);
	Redirect((void *)(uintptr_t)0x45bc80, Member(&TEditLine::SetActive));
	Redirect((void *)(uintptr_t)0x452790, Member(&TMultiStateButton::Reset));
	Redirect((void *)(uintptr_t)0x460450, Member(&TScrollBarRange::SetRange));
	Redirect((void *)(uintptr_t)0x458b20, Member(&TSkinButton::SetSkins));
	// The image's CRT reaches Windows: localtime, ctime, fprintf and _snprintf.
	Redirect((void *)(uintptr_t)0x513eb1, (void *)ImageLocaltime);
	Redirect((void *)(uintptr_t)0x5121d1, (void *)ImageCtime);
	Redirect((void *)(uintptr_t)0x512173, (void *)ImageFprintf);
	Redirect((void *)(uintptr_t)0x51233d, (void *)ImageSnprintf);
}

// --- The consoles ------------------------------------------------------------------------

typedef void *(__fastcall *CtorFn)(void *, void *);
typedef void (__fastcall *VoidFn)(void *, void *);
typedef bool (__fastcall *BoolFn)(void *, void *);
typedef void (__fastcall *BoolArgFn)(void *, void *, bool);
typedef void (__fastcall *BufferStringFn)(void *, void *, TTalkBuffer *, const char *);
typedef TTalkBuffer *(__fastcall *OpenChannelFn)(void *, void *, int, int, const char *, int, const char *);
typedef void (__fastcall *StringFn)(void *, void *, const char *);
typedef TTalkBuffer *(__fastcall *BufferFn)(void *, void *);
typedef const char *(__fastcall *TextFn)(void *, void *);
typedef TTalkLine *(__fastcall *TalkLineFn)(void *, void *, int);
typedef void (__fastcall *PointerFn)(void *, void *, void *);

static TConsoleWindow *Console[2];
static TConsoleWindow *C(int s) { return Console[s]; }

static std::string Name(int s, const void *Object)
{
	if (Object == NULL)
		return "0";
	auto R = Roles[s].find(Object);
	if (R != Roles[s].end())
		return R->second;
	auto K = Kinds[s].find(VtableOf(Object));
	if (K != Kinds[s].end()) {
		if (K->second == "Tab") {
			char Buffer[32];
			int Saved = Side;
			Side = s;
			sprintf(Buffer, "Tab%d", BufferIndex(((const TTab *)Object)->TextBuffer));
			Side = Saved;
			return Buffer;
		}
		return K->second;
	}
	return "?";
}

static void NameParts(int s)
{
	TConsoleWindow *W = C(s);
	Roles[s][W] = "Console";
	Roles[s][W->TabBar] = "Bar";
	Roles[s][W->TabBar->LeftTabs] = "Left";
	Roles[s][W->TabBar->RightTabs] = "Right";
	Roles[s][W->CurrentConsole] = "View";
	Roles[s][W->CurrentConsole->TextWindow] = "Text";
	Roles[s][W->Input] = "Input";
	Roles[s][W->CurrentConsole->ScrollBar] = "Scroll";
	Roles[s][W->VolumeButton] = "Volume";
	Roles[s][W->CloseButton] = "Close";
	Roles[s][W->ServerMessagesButton] = "ServerMsgs";
	Roles[s][W->IgnoreButton] = "Ignore";
	Roles[s][W->NewChannelButton] = "NewChannel";
	Roles[s][W->MoveRightButton] = "MoveRight";
	Roles[s][W->MoveLeftButton] = "MoveLeft";
}

// Runs F for our side and then for the image's.
template <class F> static void Both(F Function)
{
	for (int s = 0; s < 2; s++) {
		Side = s;
		if (s == OURS) {
			// An exception on our side would end the child without a message.
			try {
				Function(s);
			} catch (TMyException &E) {
				printf("ConsoleWindow: our side threw: %s (%s)\n", E.GetMessage().c_str(), E.GetOrigin().c_str());
				fflush(stdout);
				abort();
			} catch (...) {
				printf("ConsoleWindow: our side threw\n");
				fflush(stdout);
				abort();
			}
		} else {
			Function(s);
		}
	}
	Side = OURS;
}

static char Pattern;
static void Build()
{
	Pattern = (char)Random();
	World.OpenCount = Random() % 4;
	for (int i = 0; i < World.OpenCount; i++)
		World.OpenChannels[i] = Random() % 20;
	Both([&](int s) {
		Serial[s] = 0;
		void *Memory = operator new(sizeof(TConsoleWindow));
		memset(Memory, Pattern, sizeof(TConsoleWindow));
		Roles[s][Memory] = "Console";
		if (s == OURS)
			Console[s] = new (Memory) TConsoleWindow;
		else
			Console[s] = (TConsoleWindow *)At<CtorFn>(0x422410)(Memory, NULL);
		NameParts(s);
	});
}

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

// TTabBar::Changed is uninitialised until the first tab or size.
static bool DumpChanged;

static void DumpComponent(int s, std::string &Out, TComponent *Component, int Depth);
static void DumpChildren(int s, std::string &Out, TWindow *Window, int Depth)
{
	for (TComponent *Child = Window->FirstChild; Child != NULL; Child = Child->Next)
		DumpComponent(s, Out, Child, Depth + 1);
}

static void DumpText(int s, std::string &Out, TTextWindow *T)
{
	int Saved = Side;
	Side = s;
	Add(Out, " buf %d min %d max %d cur %d inv %d last %d sel %d,%d-%d,%d %d atend %d text %s",
		BufferIndex(T->TextBuffer), T->Lines.min, T->Lines.max, T->CurrentLineNumber, T->InvisibleLines,
		T->LastLineNumber, T->SelectionStartLine, T->SelectionStartOffset, T->SelectionEndLine, T->SelectionEndOffset,
		T->Selecting ? 1 : 0, T->AtEnd ? 1 : 0, T->SelectedText == NULL ? "-" : T->SelectedText);
	for (int i = T->Lines.min; i <= T->Lines.max; i++) {
		TLineFormat &F = T->Lines.entry[i - T->Lines.start];
		if (F.TextLine == NULL) {
			Add(Out, " [%d:%s,%d,%d,%d,%d,%d,%d,%d,%d]", i, F.LineText == NULL ? "-" : "?", F.LineLength,
				F.OriginalOffset, F.Indent, F.Color.R, F.Color.G, F.Color.B, F.FontNumber, F.LineEnd ? 1 : 0);
		} else if (LineIndex(T->TextBuffer, F.TextLine) < 0) {
			// A talk line removed since the lines were formatted (freed; not read).
			Add(Out, " [%d:gone,%d,%d,%d]", i, F.LineLength, F.OriginalOffset, F.Indent);
		} else {
			Add(Out, " [%d:%d@%d,%d,%d,%d,%d,%d,%d,%d,%d]", i, LineIndex(T->TextBuffer, F.TextLine),
				(int)(F.LineText - F.TextLine->Line), F.LineLength, F.OriginalOffset, F.Indent, F.Color.R, F.Color.G,
				F.Color.B, F.FontNumber, F.LineEnd ? 1 : 0);
		}
	}
	if (T->TextBuffer != NULL) {
		TTalkBuffer *B = T->TextBuffer;
		Add(Out, " flags %d%d%d%d%d%d lines %d cursor %d", B->Changed ? 1 : 0, B->Closed ? 1 : 0, B->Complete ? 1 : 0,
			B->LinesChanged ? 1 : 0, B->Locked ? 1 : 0, B->ShowMessages ? 1 : 0, B->lines, LineIndex(B, B->cursor));
	}
	Side = Saved;
}

static void DumpComponent(int s, std::string &Out, TComponent *Component, int Depth)
{
	Add(Out, "\n%*s%s %d,%d %dx%d", Depth * 2, "", Name(s, Component).c_str(), Component->OffsetX, Component->OffsetY,
		Component->Width, Component->Height);
	Add(Out, " parent %s", Name(s, Component->Parent).c_str());
	for (int i = 0; i < Component->DrawingListLength && Component->DrawingList != NULL; i++) {
		TDrawing &D = Component->DrawingList[i];
		Add(Out, " {%d,%d,%d,%d,%d,%d,%d}", D.skin, D.x, D.y, D.width, D.height, D.srcx, D.srcy);
	}
	auto K = Kinds[s].find(VtableOf(Component));
	std::string Kind = K == Kinds[s].end() ? "?" : K->second;
	if (Kind == "Console") {
		TConsoleWindow *W = (TConsoleWindow *)Component;
		Add(Out, " cap %s default %s switch %d hist %d mode %d", Name(s, W->Capture).c_str(),
			Name(s, W->DefaultTab).c_str(), W->SwitchTime, W->HistoryPosition, W->TalkMode);
		for (TTalkLine *L = W->History->first; L != NULL; L = L->Next)
			Add(Out, " h(%u,%d,%s,%s)", L->StatementID, L->Mode, L->Line, L->Text);
		Add(Out, " hcount %d", W->History->lines);
		// The buttons that are not children.
		TComponent *Parts[] = {W->VolumeButton, W->CloseButton, W->ServerMessagesButton, W->MoveRightButton,
			W->MoveLeftButton};
		for (TComponent *P : Parts) {
			if (P->Parent != W)
				Add(Out, " out %s %d,%d", Name(s, P).c_str(), P->OffsetX, P->OffsetY);
		}
		Add(Out, " volstate %d servermsgs %d", W->VolumeButton->State, W->ServerMessagesButton->State ? 1 : 0);
	} else if (Kind == "Bar") {
		TTabBar *B = (TTabBar *)Component;
		Add(Out, " cap %s moved %s cur %s busy %d changed %d", Name(s, B->Capture).c_str(), Name(s, B->Moved).c_str(),
			Name(s, B->CurrentTab).c_str(), B->Busy ? 1 : 0, DumpChanged ? (B->Changed ? 1 : 0) : -1);
		DumpChildren(s, Out, B, Depth);
		Add(Out, "\n%*sleft", Depth * 2 + 2, "");
		DumpChildren(s, Out, B->LeftTabs, Depth + 1);
		Add(Out, "\n%*sright", Depth * 2 + 2, "");
		DumpChildren(s, Out, B->RightTabs, Depth + 1);
		return;
	} else if (Kind == "View") {
		TConsole *V = (TConsole *)Component;
		Add(Out, " cap %s toend %d value %d", Name(s, V->Capture).c_str(), V->ScrollToEnd ? 1 : 0, V->ScrollBar->Range->Value);
	} else if (Kind == "Text") {
		DumpText(s, Out, (TTextWindow *)Component);
	} else if (Kind == "Tab") {
		TTab *T = (TTab *)Component;
		Add(Out, " '%s' '%s' lines %d drag %d,%d,%d,%d active %d unread %d blink %d cb %s/%d short %d", T->Caption,
			T->FullCaption, T->LineCount, T->Dragging, T->DragX, T->DragY, T->DragRight, T->Active ? 1 : 0,
			T->Unread ? 1 : 0, T->BlinkEnd, Name(s, T->Callback.Receiver).c_str(), T->Callback.Event,
			T->Shortened ? 1 : 0);
	} else if (Kind == "Edit") {
		TEditLine *E = (TEditLine *)Component;
		Add(Out, " '%s' %d,%d,%d,%d", E->Text, E->Data5C, E->Data60, E->Data66 ? 1 : 0, E->Data30[0x58 - 0x30]);
	} else if (Kind == "Button" || Kind == "Toggle" || Kind == "Multi") {
		TSkinButton *B = (TSkinButton *)Component;
		Add(Out, " skins %d,%d", B->ButtonUp, B->ButtonDown);
	}
	if (Kind == "Console" || Kind == "View" || Kind == "Window")
		DumpChildren(s, Out, (TWindow *)Component, Depth);
}

static std::string Dump(int s)
{
	std::string Out;
	DumpComponent(s, Out, C(s), 0);
	return Out;
}

static const char *Stage = "";
static void Compare(const char *What)
{
	std::string D0 = Dump(OURS), D1 = Dump(ORIGINAL);
	CHECK(Log[OURS] == Log[ORIGINAL], "ConsoleWindow %s (%s): logs differ\n  ours: %s\n  orig: %s", What, Stage,
		Log[OURS].c_str(), Log[ORIGINAL].c_str());
	CHECK(D0 == D1, "ConsoleWindow %s (%s): windows differ\n  ours: %s\n  orig: %s", What, Stage, D0.c_str(), D1.c_str());
	Log[OURS].clear();
	Log[ORIGINAL].clear();
}

// --- Operations -----------------------------------------------------------------------------

static int TabCount(int s)
{
	int Count = 0;
	TTabBar *B = C(s)->TabBar;
	TWindow *Parts[] = {B, B->LeftTabs, B->RightTabs};
	for (TWindow *P : Parts) {
		for (TComponent *Child = P->FirstChild; Child != NULL; Child = Child->Next)
			Count++;
	}
	return Count;
}
static TTalkBuffer *CurrentBuffer(int s) { return C(s)->CurrentConsole->TextWindow->TextBuffer; }

// The buffers that have a tab (OURS's view; the image's must agree).
static std::vector<int> OpenBuffers()
{
	std::vector<int> List;
	int Saved = Side;
	Side = OURS;
	TTabBar *B = C(OURS)->TabBar;
	TWindow *Parts[] = {B, B->LeftTabs, B->RightTabs};
	for (TWindow *P : Parts) {
		for (TComponent *Child = P->FirstChild; Child != NULL; Child = Child->Next)
			List.push_back(BufferIndex(((TTab *)Child)->TextBuffer));
	}
	Side = Saved;
	return List;
}

static void RandomWorld()
{
	World.ActionNumber += 1 + Random() % 8;
	World.ActionState = Random() % 4 == 0 ? 11 : 0;
	World.Throttled = Random() % 3 == 0;
	World.OwnPrivate = Random() % 2 == 0;
	World.TimeStamps = Random() % 2 == 0;
	World.HotkeyNumber = Random() % 36;
	World.HotkeyAuto = Random() % 2 == 0;
	static const char *const Hotkeys[] = {"hi", "#y help", "*Seymour* hi there", "", "exura"};
	World.HotkeyText = Hotkeys[Random() % 5];
	static const char *const Accounts[] = {"1234567", "999"};
	static const char *const Passwords[] = {"secret", "", "pw"};
	World.Account = Accounts[Random() % 2];
	World.Password = Passwords[Random() % 3];
}

static void Draw()
{
	int Surface = Random() % 3, x = Random() % 30, y = Random() % 30;
	Both([&](int s) { C(s)->Draw(Surface, x, y); });
}

static const char *const Inputs[] = {"hello", "#w whisper this", "#y YELL", "#s say", "#b broadcast", "#a anon",
	"#c channel call", "#d anon call", "#i Seymour", "#x Seymour", "#I", "*Seymour* private", "*Tester* to me",
	"@Xa@ gm message", "$Xa$ anon message", "*unclosed", "** empty", "  ", "", "my account 1234567",
	"secret password", "#q unknown", "hi pw there", "a long message with many many words in it to send",
	"*Mr Very Long Name* hi"};
enum { NINPUTS = sizeof(Inputs) / sizeof(Inputs[0]) };

static void SetInput(const char *Text)
{
	if (getenv("CW_TRACE") != NULL)
		fprintf(stderr, "    input '%s' in buffer %d (spec caption %s, mode %d)\n", Text,
			BufferIndex(C(OURS)->CurrentConsole->TextWindow->TextBuffer),
			((TConsoleBuffer *)C(OURS)->CurrentConsole->TextWindow->TextBuffer)->Spec.Caption,
			C(OURS)->CurrentConsole->TextWindow->TextBuffer->SendTalkMode);
	Both([&](int s) {
		strcpy(C(s)->Input->Text, Text);
		C(s)->Input->Data5C = -1;
		C(s)->Input->Data60 = 0;
	});
}

static void Key()
{
	static const int Keys[] = {'a', 'b', ' ', 'Z', 8, 13, 13, 9, 0x209, 0x461, 0x510, 0x463, 0x311, 0x478, 0x511,
		0x100, 0x105, 0x10b, 0x300, 0x50a, 0x10f, 0x10f, 0x10c, 0x10c, 0x30f, 0x10d, 0x41, 0x1b};
	int Code = Keys[Random() % (sizeof(Keys) / sizeof(Keys[0]))];
	if (Code == 13 && Random() % 2 == 0)
		SetInput(Inputs[Random() % NINPUTS]);
	Both([&](int s) { C(s)->KeyPress(Code); });
}

static void Event()
{
	static const int Events[] = {201, 202, 203, 204, 205, 206, 207, 208, 210, 211, 212, 213, 403, 212, 213};
	int Code = Events[Random() % (sizeof(Events) / sizeof(Events[0]))];
	Both([&](int s) { C(s)->HandleEvent(Code); });
}

static void Open()
{
	int Spec = 1 + Random() % (NSPECS - 1);
	World.PendingSpec = Spec;
	World.CreateFails = Random() % 4 == 0 && !Buffers[OURS].empty();
	// When CreateTextBuffer fails, GetTextBuffer finds an existing buffer (one
	// with a tab or one without).
	World.Lookup = World.CreateFails ? (int)(Random() % Buffers[OURS].size()) : -1;
	if (World.CreateFails && Deleted[OURS][World.Lookup])
		World.CreateFails = false;
	const char *Name = Specs[Spec].Addressee;
	const char *Title = Specs[Spec].Caption;
	int Channel = Specs[Spec].ChannelID;
	Both([&](int s) {
		if (s == OURS)
			C(s)->OpenChannel(Specs[Spec].Recv, Specs[Spec].Send, Name, Channel, Title);
		else
			At<OpenChannelFn>(0x425070)(C(s), NULL, Specs[Spec].Recv, Specs[Spec].Send, Name, Channel, Title);
	});
	// A new buffer gets lines on both sides.
	if (!World.CreateFails)
		AddRandomLines((int)Buffers[OURS].size() - 1, Random() % 5);
}

static void AddExisting()
{
	// A buffer made directly (Control.cpp's message 0x32) or one with a tab.
	int Index = Random() % 2 == 0 || Buffers[OURS].empty() ? NewBufferPair(1 + Random() % (NSPECS - 1))
		: (int)(Random() % Buffers[OURS].size());
	if (Deleted[OURS][Index])
		return;
	const char *Title = Buffers[OURS][Index]->Spec.Caption;
	Both([&](int s) {
		if (s == OURS)
			C(s)->AddChannel(Buffers[s][Index], Title);
		else
			At<BufferStringFn>(0x423060)(C(s), NULL, Buffers[s][Index], Title);
	});
}

static void Close()
{
	Both([&](int s) {
		if (s == OURS)
			C(s)->CloseChannel();
		else
			At<VoidFn>(0x4234b0)(C(s), NULL);
	});
}

static void Report()
{
	// Either the player's request is open (GetTextBuffer finds it) or a new
	// one is opened (GetTextBuffer finds nothing, then CreateTextBuffer's).
	World.PendingSpec = 8;
	World.CreateFails = false;
	int Existing = -1;
	std::vector<int> List = OpenBuffers();
	for (int b : List) {
		if (Buffers[OURS][b]->Spec.Recv == 7 && Buffers[OURS][b]->Spec.RuleViolation)
			Existing = b;
	}
	static const char *const Questions[] = {"How do I open a channel?", "x", "Help! Somebody is spamming."};
	const char *Question = Questions[Random() % 3];
	int New = (int)Buffers[OURS].size();
	Both([&](int s) {
		World.LookupQueue[0] = Existing;
		World.LookupQueue[1] = New;
		World.LookupCount = Existing >= 0 ? 1 : 2;
		World.LookupNext = 0;
		if (s == OURS)
			C(s)->ReportRuleViolation(Question);
		else
			At<StringFn>(0x425330)(C(s), NULL, Question);
	});
	World.LookupCount = 0;
}

static void ServerMessages()
{
	if (((TConsoleBuffer *)CurrentBuffer(OURS))->Spec.Default)
		return;
	if (Random() % 2 == 0) {
		bool Results[2];
		Both([&](int s) {
			Results[s] = s == OURS ? C(s)->GetServerMessages() : At<BoolFn>(0x41f1c0)(C(s), NULL);
		});
		CHECK(Results[OURS] == Results[ORIGINAL], "ConsoleWindow GetServerMessages %d, original %d", Results[OURS],
			Results[ORIGINAL]);
	} else {
		bool Show = Random() % 2 == 0;
		Both([&](int s) {
			if (s == OURS)
				C(s)->SetServerMessages(Show);
			else
				At<BoolArgFn>(0x41f470)(C(s), NULL, Show);
		});
	}
}

static void TextBufferQuery()
{
	int Results[2];
	Both([&](int s) {
		TTalkBuffer *B = s == OURS ? C(s)->GetTextBuffer() : At<BufferFn>(0x41f7f0)(C(s), NULL);
		Results[s] = BufferIndex(B);
	});
	CHECK(Results[OURS] == Results[ORIGINAL], "ConsoleWindow GetTextBuffer %d, original %d", Results[OURS],
		Results[ORIGINAL]);
}

static std::string TempDir;
static std::string ReadFile(const std::string &Path)
{
	std::string Text;
	FILE *File = fopen(Path.c_str(), "rb");
	if (File == NULL)
		return "(none)";
	char Buffer[512];
	size_t n;
	while ((n = fread(Buffer, 1, sizeof(Buffer), File)) > 0)
		Text.append(Buffer, n);
	fclose(File);
	return Text;
}
static void Save()
{
	TTab *Tab = C(OURS)->TabBar->CurrentTab;
	std::string File = std::string(Tab->getCaption()) + ".txt";
	char Old[512];
	_getcwd(Old, sizeof(Old));
	_chdir(TempDir.c_str());
	std::string Contents[2];
	bool Results[2];
	Both([&](int s) {
		remove(File.c_str());
		Results[s] = s == OURS ? C(s)->SaveChannel() : At<BoolFn>(0x420090)(C(s), NULL);
		Contents[s] = ReadFile(File);
		remove(File.c_str());
	});
	_chdir(Old);
	CHECK(Results[OURS] == Results[ORIGINAL], "ConsoleWindow SaveChannel %d, original %d", Results[OURS],
		Results[ORIGINAL]);
	CHECK(Contents[OURS] == Contents[ORIGINAL], "ConsoleWindow SaveChannel file\n  ours: %s\n  orig: %s",
		Contents[OURS].c_str(), Contents[ORIGINAL].c_str());
}

static void Active()
{
	bool A = Random() % 2 == 0;
	Both([&](int s) {
		if (s == OURS)
			C(s)->SetActive(A);
		else
			At<BoolArgFn>(0x41e960)(C(s), NULL, A);
	});
}

// New or removed lines in a channel (Talk.cpp), then the console's change flag.
static void TalkChanges()
{
	std::vector<int> List = OpenBuffers();
	int Index = List[Random() % List.size()];
	if (Random() % 3 != 0 || Buffers[OURS][Index]->lines == 0) {
		AddRandomLines(Index, 1 + Random() % 3);
	} else {
		// By pointer: Talk.cpp's Remove(int) only finds line 0.
		int Line = Random() % Buffers[OURS][Index]->lines;
		bool Quiet = Testing;
		Testing = false;
		for (int s = 0; s < 2; s++) {
			TTalkLine *L = Buffers[s][Index]->first;
			for (int i = 0; i < Line; i++)
				L = L->Next;
			Buffers[s][Index]->Remove(L);
		}
		Testing = Quiet;
	}
	World.ConsoleChanged = true;
	Draw();
	World.ConsoleChanged = false;
}

// A point in a part of the console (from our side's geometry).
static void PointIn(TComponent *Part, int &x, int &y)
{
	int px = 0, py = 0;
	for (TComponent *P = Part; P != NULL && P != C(OURS); P = P->Parent) {
		px += P->OffsetX;
		py += P->OffsetY;
	}
	x = px + (Part->Width > 0 ? (int)(Random() % (Part->Width + 4)) - 2 : 0);
	y = py + (Part->Height > 0 ? (int)(Random() % (Part->Height + 4)) - 2 : 0);
}

static void Mouse(int Kind, int x, int y, int Flags)
{
	int Direction = 1 + Random() % 2;
	Both([&](int s) {
		TConsoleWindow *W = C(s);
		switch (Kind) {
		case 0: W->LeftButtonDown(x, y, Flags); break;
		case 1: W->MouseMove(x, y); break;
		case 2: W->LeftButtonUp(x, y, Flags); break;
		case 3: W->LeftButtonDoubleClick(x, y, Flags); break;
		case 4: W->RightButtonUp(x, y, Flags); break;
		case 5: W->MouseWheel(x, y, Direction); break;
		case 6: W->HelpMouseMove(x, y); break;
		case 7: W->MouseHover(x, y); break;
		}
	});
}

// A drag: a press in the tab bar (no tab is hidden, so that the dragged tab
// stays in the bar) or in the text, a few moves, the release.
static void Drag()
{
	TTabBar *Bar = C(OURS)->TabBar;
	bool InBar = Random() % 2 == 0 && Bar->LeftTabs->FirstChild == NULL && Bar->RightTabs->FirstChild == NULL;
	int x, y;
	PointIn(InBar ? (TComponent *)Bar : (TComponent *)C(OURS)->CurrentConsole->TextWindow, x, y);
	Mouse(0, x, y, 0);
	Compare("press");
	int Moves = Random() % 5;
	for (int i = 0; i < Moves; i++) {
		x += (int)(Random() % 81) - 40;
		y += (int)(Random() % 21) - 10;
		Mouse(1, x, y, 0);
		Compare("drag");
	}
	Mouse(2, x, y, 0);
}

static void Click()
{
	TComponent *Parts[] = {C(OURS)->TabBar, C(OURS)->CurrentConsole->TextWindow, C(OURS), C(OURS)->Input,
		C(OURS)->CurrentConsole};
	int x, y;
	PointIn(Parts[Random() % 5], x, y);
	int Kind = 3 + Random() % 5;
	Mouse(Kind, x, y, 0);
	// A double click is followed by the release (a tab takes it as a press).
	if (Kind == 3) {
		Compare("double click");
		Mouse(2, x, y, 0);
	}
}

static void Press()
{
	// A button's callback (the arrows send their event to the tab bar).
	int Which = Random() % 7;
	Both([&](int s) {
		TConsoleWindow *W = C(s);
		TSkinButton *Buttons[] = {W->CloseButton, W->ServerMessagesButton, W->IgnoreButton, W->NewChannelButton,
			W->MoveRightButton, W->MoveLeftButton, W->IgnoreButton};
		TSkinButton *B = Buttons[Which];
		if (B == W->CloseButton && W->TabBar->CurrentTab == W->DefaultTab)
			return;
		ButtonCallback(B).Send();
	});
}

static void BarEvent()
{
	static const int Events[] = {5, 6, 25, 26};
	int Code = Events[Random() % 4];
	Both([&](int s) { C(s)->TabBar->HandleEvent(Code); });
}

static void TextQueries()
{
	int What = Random() % 4;
	int Line = (int)(Random() % 12) - 1;
	std::string Results[2];
	Both([&](int s) {
		TTextWindow *T = C(s)->CurrentConsole->TextWindow;
		char Buffer[64];
		switch (What) {
		case 0: {
			const char *Text = s == OURS ? T->GetSelectedText() : At<TextFn>(0x419bd0)(T, NULL);
			Results[s] = Text == NULL ? "(null)" : Text;
			break;
		}
		case 1:
			Results[s] = (s == OURS ? T->SelectAll() : At<BoolFn>(0x4199e0)(T, NULL)) ? "1" : "0";
			break;
		case 2: {
			TTalkLine *L = s == OURS ? T->GetTalkLine(Line) : At<TalkLineFn>(0x419030)(T, NULL, Line);
			sprintf(Buffer, "%d", L == NULL ? -2 : LineIndex(T->TextBuffer, L));
			Results[s] = Buffer;
			break;
		}
		case 3:
			Results[s] = (s == OURS ? T->HasSelection() : At<BoolFn>(0x417590)(T, NULL)) ? "1" : "0";
			break;
		}
	});
	CHECK(Results[OURS] == Results[ORIGINAL], "ConsoleWindow text query %d: %s, original %s", What,
		Results[OURS].c_str(), Results[ORIGINAL].c_str());
}

static void Size()
{
	int Width = 200 + Random() % 400, Height = 106 + Random() % 200;
	Both([&](int s) { C(s)->SetSize(Width, Height); });
}

static int ActionCount[24];
static void Action()
{
	RandomWorld();
	int A = Random() % 22;
	ActionCount[A]++;
	if (getenv("CW_TRACE") != NULL)
		fprintf(stderr, "    kind %d\n", A);
	switch (A) {
	case 0: Size(); Stage = "size"; break;
	case 1: Both([&](int s) { C(s)->Arrange(); }); Stage = "arrange"; break;
	case 2: case 3: World.ConsoleChanged = Random() % 4 == 0; Draw(); World.ConsoleChanged = false; Stage = "draw"; break;
	case 4: case 5: Key(); Stage = "key"; break;
	case 6: SetInput(Inputs[Random() % NINPUTS]); Both([&](int s) { C(s)->KeyPress(13); }); Stage = "send"; break;
	case 7: Event(); Stage = "event"; break;
	case 8: if (TabCount(OURS) < 12) Open(); Stage = "open"; break;
	case 9: if (TabCount(OURS) < 12) AddExisting(); Stage = "add"; break;
	case 10: Close(); Stage = "close"; break;
	case 11: Report(); Stage = "report"; break;
	case 12: ServerMessages(); Stage = "server messages"; break;
	case 13: TextBufferQuery(); Stage = "text buffer"; break;
	case 14: Save(); Stage = "save"; break;
	case 15: Active(); Stage = "active"; break;
	case 16: TalkChanges(); Stage = "talk"; break;
	case 17: Drag(); Stage = "drag"; break;
	case 18: Click(); Stage = "click"; break;
	case 19: Press(); Stage = "press"; break;
	case 20: BarEvent(); Stage = "bar"; break;
	case 21: TextQueries(); Stage = "text"; break;
	}
}

void TestConsoleWindow()
{
	int Before = Checks, BeforeFailures = Failures;
	if (getenv("CW_TRACE") != NULL)
		setvbuf(stdout, NULL, _IONBF, 0);
	PatchImage();
	PatchScript();
	InitSkins();
	Global<int>(0x71c450) = NumberOfSkins;
	Global<TSkin *>(0x71c454) = Skins;
	InitSkinProperties();
	At<void (__cdecl *)()>(0x498160)();
	Surface = new TConsoleSurface;
	TempDir = std::string(getenv("TEMP") != NULL ? getenv("TEMP") : ".") + "\\consolewindow";
	_mkdir(TempDir.c_str());
	Player.CreatureID = 0x10000001;
	strcpy(Player.Name, "Tester");

	// The vtables of the fakes, from instances.
	{
		TTestEditLine *E = new TTestEditLine;
		TestEditVtable = VtableOf(E);
		delete E;
		TTestScrollBar *S = new TTestScrollBar;
		TestScrollVtable = VtableOf(S);
		delete S;
		TTestSkinButtonC *B = new TTestSkinButtonC;
		TestButtonVtable = VtableOf(B);
		delete B;
		TTestToggleButton *T = new TTestToggleButton;
		TestToggleVtable = VtableOf(T);
		delete T;
		TTestMultiStateButton *M = new TTestMultiStateButton;
		TestMultiVtable = VtableOf(M);
		delete M;
		TWindow *W = new TWindow;
		Kinds[OURS][VtableOf(W)] = "Window";
		delete W;
	}
	for (int s = 0; s < 2; s++) {
		Kinds[s][TestEditVtable] = "Edit";
		Kinds[s][TestScrollVtable] = "Scroll";
		Kinds[s][TestButtonVtable] = "Button";
		Kinds[s][TestToggleVtable] = "Toggle";
		Kinds[s][TestMultiVtable] = "Multi";
	}
	Kinds[ORIGINAL][(void *)(uintptr_t)0x55e1a0] = "Console";
	Kinds[ORIGINAL][(void *)(uintptr_t)0x55dcd8] = "Bar";
	Kinds[ORIGINAL][(void *)(uintptr_t)0x55dd80] = "Text";
	Kinds[ORIGINAL][(void *)(uintptr_t)0x55dfc0] = "View";
	Kinds[ORIGINAL][(void *)(uintptr_t)0x55e098] = "Tab";
	Kinds[ORIGINAL][(void *)(uintptr_t)0x564528] = "Window";
	PatchCallees();
	DialogFakes.SetSkins = HookSetSkins;
	{
		// Our vtables, from a console and a tab made quietly.
		World.OpenCount = 0;
		TConsoleWindow *W = new TConsoleWindow;
		Kinds[OURS][VtableOf(W)] = "Console";
		Kinds[OURS][VtableOf(W->TabBar)] = "Bar";
		Kinds[OURS][VtableOf(W->CurrentConsole)] = "View";
		Kinds[OURS][VtableOf(W->CurrentConsole->TextWindow)] = "Text";
		TConsoleBuffer *B = new TConsoleBuffer(-1, Specs[1]);
		TTab *T = new TTab("x", B, TCallback());
		Kinds[OURS][VtableOf(T)] = "Tab";
		delete T;
		delete W;
	}
	Testing = true;

	int Rounds = 300;
	for (int Round = 0; Round < Rounds; Round++) {
		if (getenv("CW_TRACE") != NULL)
			fprintf(stderr, "round %d\n", Round);
		RandomWorld();
		World.ConsoleChanged = false;
		Stage = "constructor";
		ScrollMinimumHeight = Random() % 120;
		Build();
		DumpChanged = false;
		Compare("constructor");
		DumpChanged = true;
		// MainWindow.cpp: the size, then the default channel.
		Size();
		int Default = NewBufferPair(0);
		Both([&](int s) {
			if (s == OURS)
				C(s)->OpenDefaultChannel(Buffers[s][Default], "Default");
			else
				At<BufferStringFn>(0x422e70)(C(s), NULL, Buffers[s][Default], "Default");
		});
		Stage = "default channel";
		Compare("default channel");
		int Actions = 20 + Random() % 40;
		for (int i = 0; i < Actions; i++) {
			if (getenv("CW_TRACE") != NULL)
				fprintf(stderr, "  action %d\n", i);
			Action();
			Compare("action");
		}
		Stage = "destructor";
		Both([&](int s) {
			delete C(s);
			Console[s] = NULL;
		});
		CHECK(Log[OURS] == Log[ORIGINAL], "ConsoleWindow destructor: logs differ\n  ours: %s\n  orig: %s",
			Log[OURS].c_str(), Log[ORIGINAL].c_str());
		Log[OURS].clear();
		Log[ORIGINAL].clear();
		for (int s = 0; s < 2; s++) {
			Roles[s].clear();
			// The buffers stay allocated (the tabs' destructors only report them).
			Buffers[s].clear();
			Deleted[s].clear();
		}
	}

	Testing = false;
	DialogFakes.SetSkins = NULL;
	RestoreAll();
	delete Surface;
	ExitSkins();
	Global<TSkin *>(0x71c454) = NULL;
	int Least = ActionCount[0];
	for (int i = 1; i < 22; i++)
		Least = ActionCount[i] < Least ? ActionCount[i] : Least;
	printf("ConsoleWindow: %d checks, %d failures (each action at least %d times)\n", Checks - Before,
		Failures - BeforeFailures, Least);
}
