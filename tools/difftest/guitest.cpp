// Differential tests of GUI.cpp against the original code.
//
// - SkinProperties: the original constructor (0x496800) and ours on memory
//   filled with 0xcc (the untouched ints must stay), then the original
//   InitSkinProperties (0x498160) and ours, compared byte for byte.
// - DrawSkin, DrawSkinClipped and TComponent::Draw: the picture functions of
//   graphics.cpp (0x490c60 DrawPicturePart, 0x4910e0 DrawPicturePartClipped)
//   are recorders on both sides (ours redirected while TestGUI runs).
// - The window classes: two mirrored trees, one of original objects (made by
//   the image's constructors 0x496080 and 0x496100; every call through their
//   vtables runs the original code), one of ours. The leaves are probes of
//   our own class that log every call they get and answer mouse events from
//   a random mask; both trees get the same random operations, which are
//   checked first so that no assertion fails, and the trees, the return
//   values, the logs and the help state are compared after each one. The
//   original objects get a copy of their vtable whose destructor slot logs
//   the deletion and then calls the original deleting destructor.
// - Timers (the image's timer vector constructed by 0x4973f0) and hotkeys
//   (0x4976f0), with probes as receivers.
// Failing assertions are not exercised.

#include "difftest.h"
#include "../../src/GUI.h"
#include "../../src/graphics.h"
#include "../../src/Skins.h"
#include "../../src/Help.h"
#include <map>
#include <new>
#include <string>
#include <vector>

enum { OURS = 0, ORIGINAL = 1 };
static std::string Log[2];

static void Logf(int Side, const char *Format, ...)
{
	char Buffer[256];
	va_list Args;
	va_start(Args, Format);
	vsnprintf(Buffer, sizeof(Buffer), Format, Args);
	va_end(Args);
	Log[Side] += Buffer;
}

// --- graphics.cpp recorders -------------------------------------------------

// Our DrawPicturePart and DrawPicturePartClipped (graphics.cpp, tested in
// graphicstest.cpp) jump to these recorders while TestGUI runs.
static void OurPicture(int Surface, int x, int y, int PictureNumber, int SourceX, int SourceY, int Width,
	int Height)
{
	Logf(OURS, "P%d,%d,%d,%d,%d,%d,%d,%d;", Surface, x, y, PictureNumber, SourceX, SourceY, Width, Height);
}

static void __cdecl ImagePicture(int Surface, int x, int y, int PictureNumber, int SourceX, int SourceY,
	int Width, int Height)
{
	Logf(ORIGINAL, "P%d,%d,%d,%d,%d,%d,%d,%d;", Surface, x, y, PictureNumber, SourceX, SourceY, Width, Height);
}

static void OurClippedPicture(int Surface, int x, int y, int PictureNumber, int SourceX, int SourceY, int Width,
	int Height, TRect Clip)
{
	Logf(OURS, "C%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d;", Surface, x, y, PictureNumber, SourceX, SourceY, Width,
		Height, Clip.x, Clip.y, Clip.width, Clip.height);
}

// Redirects one of our functions to a recorder (Restore = false) or puts
// back its first five bytes, as talktest.cpp does with the CRT clock.
static uint8 SavedPicture[2][5];
static void RedirectOurs(int Index, void *Function, void *Recorder, bool Restore)
{
	DWORD Protection;
	VirtualProtect(Function, 5, PAGE_EXECUTE_READWRITE, &Protection);
	if (Restore) {
		memcpy(Function, SavedPicture[Index], 5);
	} else {
		memcpy(SavedPicture[Index], Function, 5);
		PatchJump((uint32)(uintptr_t)Function, Recorder);
	}
	FlushInstructionCache(GetCurrentProcess(), Function, 5);
	VirtualProtect(Function, 5, Protection, &Protection);
}

static void __cdecl ImageClippedPicture(int Surface, int x, int y, int PictureNumber, int SourceX, int SourceY,
	int Width, int Height, int ClipX, int ClipY, int ClipWidth, int ClipHeight)
{
	Logf(ORIGINAL, "C%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d;", Surface, x, y, PictureNumber, SourceX, SourceY,
		Width, Height, ClipX, ClipY, ClipWidth, ClipHeight);
}

static void CheckLogs(const char *What)
{
	Checks++;
	if (Log[OURS] != Log[ORIGINAL]) {
		Failures++;
		if (Failures <= 20)
			printf("FAIL %s: log\n  ours     %.400s\n  original %.400s\n", What, Log[OURS].c_str(),
				Log[ORIGINAL].c_str());
	}
	Log[OURS].clear();
	Log[ORIGINAL].clear();
}

// Skins with a positive size (DrawSkin asserts it).
static std::vector<int> GoodSkins;

static void PrepareSkins()
{
	InitSkins();
	Global<int>(0x71c450) = NumberOfSkins;
	Global<TSkin *>(0x71c454) = Skins;
	GoodSkins.clear();
	for (int i = 1; i < NumberOfSkins && i < LASTSKIN; i++) {
		if (i != 152 && Skins[i].width > 0 && Skins[i].height > 0) // 152 is left uninitialised
			GoodSkins.push_back(i);
	}
}

static int RandomSkin() { return GoodSkins[Random() % GoodSkins.size()]; }

// --- SkinProperties -------------------------------------------------------

typedef void (__fastcall *ThisFn)(void *This, void *);

static void TestSkinProperties()
{
	static uint8 Ours[sizeof(TSkinProperties)];
	uint8 *Original = (uint8 *)(uintptr_t)0x5ccfc8;
	CHECK(sizeof(TSkinProperties) == 0x1528, "sizeof(TSkinProperties) 0x%x", (unsigned)sizeof(TSkinProperties));
	memset(Ours, 0xcc, sizeof(Ours));
	memset(Original, 0xcc, 0x1528);
	new (Ours) TSkinProperties; // default-initialisation: the ints stay untouched
	At<ThisFn>(0x496800)(Original, NULL);
	for (int i = 0; i < 0x1528; i += 4) {
		CHECK(memcmp(Ours + i, Original + i, 4) == 0, "SkinProperties constructor +0x%x: %08x, original %08x", i,
			*(uint32 *)(Ours + i), *(uint32 *)(Original + i));
	}
	// As at start-up: zeroed static storage, the constructor, then InitControl's call.
	memset(&SkinProperties, 0, sizeof(SkinProperties));
	memset(Original, 0, 0x1528);
	new (&SkinProperties) TSkinProperties;
	At<ThisFn>(0x496800)(Original, NULL);
	InitSkinProperties();
	At<void (__cdecl *)()>(0x498160)();
	for (int i = 0; i < 0x1528; i += 4) {
		CHECK(memcmp((uint8 *)&SkinProperties + i, Original + i, 4) == 0,
			"InitSkinProperties +0x%x: %d, original %d", i, *(int *)((uint8 *)&SkinProperties + i),
			*(int *)(Original + i));
	}
}

// --- Skin drawing ---------------------------------------------------------

typedef void (__cdecl *DrawSkinFn)(int, int, int, int, int, int, int, int);
typedef void (__cdecl *DrawSkinClippedFn)(int, int, int, int, int, int, int, int, int, int, int, int);

static int RandomSize(int Skin)
{
	switch (Random() % 4) {
	case 0: return Skins[Skin].width;
	case 1: return 1 + Random() % 8;
	default: return 1 + Random() % 200;
	}
}

static void TestDrawSkin()
{
	for (int t = 0; t < 20000; t++) {
		int Skin = Random() % 16 == 0 ? 0 : RandomSkin();
		int Surface = Random() % 3;
		int x = Random() % 500, y = Random() % 500;
		int Width = Skin != 0 ? RandomSize(Skin) : 5;
		int Height = Skin != 0 ? (Random() % 4 == 0 ? Skins[Skin].height : RandomSize(Skin)) : 5;
		if (Random() % 20 == 0)
			Width = 0;
		if (Random() % 20 == 0)
			Height = 0;
		int SrcX = (int)(Random() % 8) - 2, SrcY = (int)(Random() % 8) - 2;
		if (Skin != 0 && (SrcX >= Skins[Skin].width || SrcY >= Skins[Skin].height))
			SrcX = SrcY = 0; // a non-positive piece width would never end the loop
		if (Random() % 2 == 0) {
			DrawSkin(Surface, x, y, Width, Height, Skin, SrcX, SrcY);
			At<DrawSkinFn>(0x497a00)(Surface, x, y, Width, Height, Skin, SrcX, SrcY);
			CheckLogs("DrawSkin");
		} else {
			int Clip[4] = {(int)(Random() % 100), (int)(Random() % 100), (int)(Random() % 300), (int)(Random() % 300)};
			DrawSkinClipped(Surface, x, y, Width, Height, Skin, Clip[0], Clip[1], Clip[2], Clip[3], SrcX, SrcY);
			At<DrawSkinClippedFn>(0x4a1690)(Surface, x, y, Width, Height, Skin, Clip[0], Clip[1], Clip[2], Clip[3],
				SrcX, SrcY);
			CheckLogs("DrawSkinClipped");
		}
	}
}

// --- Window trees ---------------------------------------------------------

// A leaf that logs every call it gets. Mouse events are answered from Mask.
class TProbe : public TElement {
public:
	TProbe(int Side, int Id) : Side(Side), Id(Id), Mask(0), MinimumWidth(0), MinimumHeight(0) {}
	~TProbe() { Logf(Side, "~%d;", Id); }
	void SetSize(int w, int h) { Logf(Side, "%d.S%d,%d;", Id, w, h); TElement::SetSize(w, h); }
	void SetPosition(int x, int y) { Logf(Side, "%d.P%d,%d;", Id, x, y); TElement::SetPosition(x, y); }
	int getMinimumWidth() { Logf(Side, "%d.mw;", Id); return MinimumWidth; }
	int getMinimumHeight() { Logf(Side, "%d.mh;", Id); return MinimumHeight; }
	void Draw(int Surface, int x, int y) { Logf(Side, "%d.D%d,%d,%d;", Id, Surface, x, y); }
	void Arrange() { Logf(Side, "%d.A;", Id); }
	bool MouseMove(int x, int y) { Logf(Side, "%d.M%d,%d;", Id, x, y); return (Mask & 1) != 0; }
	bool LeftButtonDown(int x, int y, int f) { Logf(Side, "%d.LD%d,%d,%d;", Id, x, y, f); return (Mask & 2) != 0; }
	bool LeftButtonUp(int x, int y, int f) { Logf(Side, "%d.LU%d,%d,%d;", Id, x, y, f); return (Mask & 4) != 0; }
	bool LeftButtonDoubleClick(int x, int y, int f) { Logf(Side, "%d.LC%d,%d,%d;", Id, x, y, f); return (Mask & 8) != 0; }
	bool RightButtonUp(int x, int y, int f) { Logf(Side, "%d.RU%d,%d,%d;", Id, x, y, f); return (Mask & 16) != 0; }
	bool MouseWheel(int x, int y, int d) { Logf(Side, "%d.W%d,%d,%d;", Id, x, y, d); return (Mask & 32) != 0; }
	void MouseHover(int x, int y) { Logf(Side, "%d.H%d,%d;", Id, x, y); }
	bool HelpMouseMove(int x, int y)
	{
		Logf(Side, "%d.HM%d,%d;", Id, x, y);
		if (Mask & 64) {
			if (Side == OURS)
				SetHelpHandle(Id);
			else
				At<void (__cdecl *)(int)>(0x4a2a70)(Id);
		}
		return (Mask & 128) != 0;
	}
	void KeyPress(int Key) { Logf(Side, "%d.K%d;", Id, Key); }
	void HandleEvent(int Event) { Logf(Side, "%d.E%d;", Id, Event); }
	void PositionChanged() { Logf(Side, "%d.PC;", Id); }
	void SetCapture(TComponent *C) { Logf(Side, "%d.SC;", Id); TElement::SetCapture(C); }
	void ReleaseCapture(TComponent *C) { Logf(Side, "%d.RC;", Id); TElement::ReleaseCapture(C); }
	void StopDragging() { Logf(Side, "%d.SD;", Id); }
	int Side, Id, Mask, MinimumWidth, MinimumHeight;
};

// Our side's windows and plain elements log their deletion.
class TOurWindow : public TWindow {
public:
	TOurWindow(int Id) : Id(Id) {}
	~TOurWindow() { Logf(OURS, "~%d;", Id); }
	int Id;
};
class TOurElement : public TElement {
public:
	TOurElement(int Id) : Id(Id) {}
	~TOurElement() { Logf(OURS, "~%d;", Id); }
	int Id;
};

enum { WINDOW, ELEMENT, PROBE };
struct TNode {
	int Kind;
	bool Alive;
	TComponent *Side[2];
};
static std::vector<TNode> Nodes;
static std::map<TComponent *, int> Index[2];

// Copies of the original vtables whose destructor slot logs first.
static void *WindowVtable[40], *ElementVtable[33];
static void __fastcall OriginalWindowDestructor(void *This, void *, int Flags);
static void __fastcall OriginalElementDestructor(void *This, void *, int Flags);

static void LogOriginalDeletion(void *This)
{
	std::map<TComponent *, int>::iterator It = Index[ORIGINAL].find((TComponent *)This);
	Logf(ORIGINAL, "~%d;", It != Index[ORIGINAL].end() ? It->second : -1);
}

static void __fastcall OriginalWindowDestructor(void *This, void *, int Flags)
{
	LogOriginalDeletion(This);
	At<void (__fastcall *)(void *, void *, int)>(0x4a29e0)(This, NULL, Flags);
}

static void __fastcall OriginalElementDestructor(void *This, void *, int Flags)
{
	LogOriginalDeletion(This);
	At<void (__fastcall *)(void *, void *, int)>(0x496550)(This, NULL, Flags);
}

static void AddNode(int Kind)
{
	int Id = (int)Nodes.size();
	TNode N;
	N.Kind = Kind;
	N.Alive = true;
	if (Kind == WINDOW) {
		void *Memory = operator new(0x2c);
		At<ThisFn>(0x496080)(Memory, NULL);
		*(void ***)Memory = WindowVtable;
		N.Side[ORIGINAL] = (TComponent *)Memory;
		N.Side[OURS] = new TOurWindow(Id);
	} else if (Kind == ELEMENT) {
		void *Memory = operator new(0x24);
		At<ThisFn>(0x496100)(Memory, NULL);
		*(void ***)Memory = ElementVtable;
		N.Side[ORIGINAL] = (TComponent *)Memory;
		N.Side[OURS] = new TOurElement(Id);
	} else {
		N.Side[ORIGINAL] = new TProbe(ORIGINAL, Id);
		N.Side[OURS] = new TProbe(OURS, Id);
		int Mask = (int)(Random() & 0xff), Mw = (int)(Random() % 4 == 0 ? Random() % 60 : 0);
		int Mh = (int)(Random() % 4 == 0 ? Random() % 60 : 0);
		for (int s = 0; s < 2; s++) {
			TProbe *P = (TProbe *)N.Side[s];
			P->Mask = Mask;
			P->MinimumWidth = Mw;
			P->MinimumHeight = Mh;
		}
	}
	// Some components get a drawing list.
	if (Random() % 3 == 0) {
		int Length = 1 + Random() % 4;
		TDrawing Entries[4];
		for (int i = 0; i < Length; i++) {
			TDrawing &E = Entries[i];
			E.skin = Random() % 5 == 0 ? 0 : RandomSkin();
			E.x = Random() % 50;
			E.y = Random() % 50;
			bool Whole = Random() % 2 == 0;
			E.width = Whole ? 0 : 1 + Random() % 60;
			E.height = Whole ? 0 : 1 + Random() % 60;
			E.srcx = E.srcy = 0;
		}
		for (int s = 0; s < 2; s++) {
			N.Side[s]->DrawingList = new TDrawing[Length];
			memcpy(N.Side[s]->DrawingList, Entries, sizeof(TDrawing) * Length);
			N.Side[s]->DrawingListLength = Length;
		}
	}
	int x = (int)(Random() % 120) - 20, y = (int)(Random() % 120) - 20;
	int w = Random() % 100, h = Random() % 100;
	for (int s = 0; s < 2; s++) {
		N.Side[s]->OffsetX = x;
		N.Side[s]->OffsetY = y;
		N.Side[s]->Width = w;
		N.Side[s]->Height = h;
		Index[s][N.Side[s]] = Id;
	}
	Nodes.push_back(N);
}

static int IndexOf(int s, TComponent *C)
{
	if (C == NULL)
		return -1;
	std::map<TComponent *, int>::iterator It = Index[s].find(C);
	return It == Index[s].end() ? -2 : It->second;
}

// Marks the nodes whose deletion the logs report.
static void CollectDeletions()
{
	for (int s = 0; s < 2; s++) {
		const std::string &L = Log[s];
		for (size_t p = L.find('~'); p != std::string::npos; p = L.find('~', p + 1)) {
			int Id = atoi(L.c_str() + p + 1);
			if (Id >= 0 && Id < (int)Nodes.size() && Nodes[Id].Side[s] != NULL) {
				Index[s].erase(Nodes[Id].Side[s]);
				Nodes[Id].Side[s] = NULL;
			}
		}
	}
	for (size_t i = 0; i < Nodes.size(); i++) {
		if (Nodes[i].Alive && (Nodes[i].Side[0] == NULL || Nodes[i].Side[1] == NULL)) {
			CHECK(Nodes[i].Side[0] == NULL && Nodes[i].Side[1] == NULL, "node %d deleted on one side only", (int)i);
			Nodes[i].Alive = false;
		}
	}
}

static void CompareTrees(const char *What)
{
	CollectDeletions();
	for (size_t i = 0; i < Nodes.size(); i++) {
		if (!Nodes[i].Alive)
			continue;
		TComponent *A = Nodes[i].Side[OURS], *B = Nodes[i].Side[ORIGINAL];
		Checks++;
		bool Same = IndexOf(OURS, A->Parent) == IndexOf(ORIGINAL, B->Parent)
			&& IndexOf(OURS, A->Next) == IndexOf(ORIGINAL, B->Next) && A->OffsetX == B->OffsetX
			&& A->OffsetY == B->OffsetY && A->Width == B->Width && A->Height == B->Height;
		if (Nodes[i].Kind == WINDOW) {
			TWindow *WA = (TWindow *)A, *WB = (TWindow *)B;
			Same = Same && IndexOf(OURS, WA->FirstChild) == IndexOf(ORIGINAL, WB->FirstChild)
				&& IndexOf(OURS, WA->Capture) == IndexOf(ORIGINAL, WB->Capture);
		}
		if (!Same) {
			Failures++;
			if (Failures <= 20)
				printf("FAIL %s: node %d differs (parent %d/%d next %d/%d pos %d,%d/%d,%d size %d,%d/%d,%d)\n", What,
					(int)i, IndexOf(OURS, A->Parent), IndexOf(ORIGINAL, B->Parent), IndexOf(OURS, A->Next),
					IndexOf(ORIGINAL, B->Next), A->OffsetX, A->OffsetY, B->OffsetX, B->OffsetY, A->Width, A->Height,
					B->Width, B->Height);
		}
	}
	int Handles[2] = {GetHelpHandle(), At<int (__cdecl *)()>(0x4a2ac0)()};
	int Area[2][4];
	GetHelpArea(&Area[0][0], &Area[0][1], &Area[0][2], &Area[0][3]);
	At<void (__cdecl *)(int *, int *, int *, int *)>(0x4a2a90)(&Area[1][0], &Area[1][1], &Area[1][2], &Area[1][3]);
	CHECK(Handles[0] == Handles[1] && memcmp(Area[0], Area[1], sizeof(Area[0])) == 0, "%s: help state", What);
	CheckLogs(What);
}

// --- Preconditions (checked on our tree, the same as the original's) -------

static TComponent *Ours(int i) { return Nodes[i].Side[OURS]; }

static bool IsAncestorOrSelf(TComponent *A, TComponent *C)
{
	for (; C != NULL; C = C->Parent)
		if (C == A)
			return true;
	return false;
}

// Every parent above C captures the child on the way to C.
static bool ChainOK(TComponent *C)
{
	for (; C->Parent != NULL; C = C->Parent)
		if (((TWindow *)C->Parent)->Capture != C)
			return false;
	return true;
}

static bool ChainFree(TComponent *C)
{
	for (; C != NULL; C = C->Parent)
		if (((TWindow *)C)->Capture != NULL)
			return false;
	return true;
}

static int RandomAlive(int Kind = -1)
{
	std::vector<int> List;
	for (size_t i = 0; i < Nodes.size(); i++)
		if (Nodes[i].Alive && (Kind < 0 || Nodes[i].Kind == Kind))
			List.push_back((int)i);
	return List.empty() ? -1 : List[Random() % List.size()];
}

static int RandomChild(int w)
{
	std::vector<int> List;
	for (TComponent *C = ((TWindow *)Ours(w))->FirstChild; C != NULL; C = C->Next)
		List.push_back(IndexOf(OURS, C));
	return List.empty() ? -1 : List[Random() % List.size()];
}

// Removing C (also by deleting it) releases the capture if C holds it.
static bool CanRemove(int w, int c)
{
	TWindow *W = (TWindow *)Ours(w);
	return W->Capture != Ours(c) || ChainOK(W);
}

static bool CanFitToChildren(int w)
{
	TWindow *W = (TWindow *)Ours(w);
	if (W->FirstChild == NULL)
		return true;
	int Width = 0, Height = 0;
	for (TComponent *C = W->FirstChild; C != NULL; C = C->Next) {
		if (Width <= C->OffsetX + C->Width)
			Width = C->OffsetX + C->Width;
		if (Height <= C->Height + C->OffsetY)
			Height = C->Height + C->OffsetY;
	}
	return Width > 0 && Height > 0;
}

// No drawing list entry may lie left of or above the surface ("dialog
// origin outside main window", DrawSkin's assertions).
static bool CanDraw(TComponent *C, int x, int y)
{
	for (int i = 0; i < C->DrawingListLength; i++)
		if (C->DrawingList[i].x + x < 0 || C->DrawingList[i].y + y < 0)
			return false;
	if (dynamic_cast<TWindow *>(C) != NULL) {
		for (TComponent *Child = ((TWindow *)C)->FirstChild; Child != NULL; Child = Child->Next)
			if (!CanDraw(Child, Child->OffsetX + x, Child->OffsetY + y))
				return false;
	}
	return true;
}

static int Coordinate() { return (int)(Random() % 160) - 30; }

static int Executed[30];

static void TreeOperation()
{
	int Op = Random() % 30;
	int w = RandomAlive(WINDOW);
	if (w < 0)
		return;
	TWindow *W[2] = {(TWindow *)Nodes[w].Side[0], (TWindow *)Nodes[w].Side[1]};
	char What[64];
	snprintf(What, sizeof(What), "operation %d on %d", Op, w);
	int x = Coordinate(), y = Coordinate(), f = Random() % 0x400;
	static const char *Trace = getenv("GUITEST_TRACE");
	if (Trace != NULL && Trace[0] == '2')
		printf("%s\n", What);
	switch (Op) {
	case 0: case 1: case 2: { // InsertChild, AppendChild, InsertChildAfter
		int c = RandomAlive();
		if (c < 0 || Ours(c)->Parent != NULL || IsAncestorOrSelf(Ours(c), Ours(w)))
			return;
		int p = Op == 2 && Random() % 4 != 0 ? RandomChild(w) : -1;
		for (int s = 0; s < 2; s++) {
			if (Op == 0)
				W[s]->InsertChild(Nodes[c].Side[s]);
			else if (Op == 1)
				W[s]->AppendChild(Nodes[c].Side[s]);
			else
				W[s]->InsertChildAfter(p < 0 ? NULL : Nodes[p].Side[s], Nodes[c].Side[s]);
		}
		break;
	}
	case 3: case 4: { // RemoveChild, DeleteChild
		int c = RandomChild(w);
		if (c < 0 || !CanRemove(w, c) || (Op == 4 && Random() % 3 != 0))
			return;
		for (int s = 0; s < 2; s++) {
			if (Op == 3)
				W[s]->RemoveChild(Nodes[c].Side[s]);
			else
				W[s]->DeleteChild(Nodes[c].Side[s]);
		}
		break;
	}
	case 5: { // SwapChildren
		int a = RandomChild(w), b = RandomChild(w);
		if (a < 0 || a == b || ((TWindow *)Ours(w))->Capture == Ours(a) || ((TWindow *)Ours(w))->Capture == Ours(b))
			return;
		for (int s = 0; s < 2; s++)
			W[s]->SwapChildren(Nodes[a].Side[s], Nodes[b].Side[s]);
		break;
	}
	case 6: { // IsChild, GetFirstChild, GetLastChild, GetPrevious, GetNext, GetParent, GetCapture
		int c = RandomAlive();
		int R[2][7];
		for (int s = 0; s < 2; s++) {
			TComponent *C = Nodes[c].Side[s];
			R[s][0] = W[s]->IsChild(C);
			R[s][1] = IndexOf(s, W[s]->GetFirstChild());
			R[s][2] = IndexOf(s, W[s]->GetLastChild());
			R[s][3] = C->Parent != NULL ? IndexOf(s, C->GetPrevious()) : 0;
			R[s][4] = IndexOf(s, C->GetNext());
			R[s][5] = IndexOf(s, C->GetParent());
			R[s][6] = IndexOf(s, W[s]->GetCapture());
		}
		CHECK(memcmp(R[0], R[1], sizeof(R[0])) == 0, "%s: queries %d,%d,%d,%d,%d,%d,%d original %d,%d,%d,%d,%d,%d,%d",
			What, R[0][0], R[0][1], R[0][2], R[0][3], R[0][4], R[0][5], R[0][6], R[1][0], R[1][1], R[1][2], R[1][3],
			R[1][4], R[1][5], R[1][6]);
		break;
	}
	case 7: { // SetPosition, SetSize of any component
		int c = RandomAlive();
		int Width = Random() % 120, Height = Random() % 120;
		bool Size = Random() % 2 == 0;
		for (int s = 0; s < 2; s++) {
			if (Size)
				Nodes[c].Side[s]->SetSize(Width, Height);
			else
				Nodes[c].Side[s]->SetPosition(x, y);
		}
		break;
	}
	case 8: case 9: { // SetChildPosition, SetChildArea
		int c = RandomChild(w);
		if (c < 0 || Ours(w)->Width <= 0 || Ours(w)->Height <= 0)
			return;
		int Width = (int)(Random() % 100) - 40, Height = (int)(Random() % 100) - 40;
		if (Op == 9) { // the resulting size must not be negative (SetSize asserts it)
			int cx = x < 0 ? Ours(w)->Width + x : x, cy = y < 0 ? Ours(w)->Height + y : y;
			if ((Width < 1 ? Ours(w)->Width - cx + Width : Width) < 0 || (Height < 1 ? Ours(w)->Height - cy + Height : Height) < 0)
				return;
		}
		for (int s = 0; s < 2; s++) {
			if (Op == 8)
				W[s]->SetChildPosition(Nodes[c].Side[s], x, y);
			else
				W[s]->SetChildArea(Nodes[c].Side[s], TRect(x, y, Width, Height));
		}
		break;
	}
	case 10: { // FitToChild (the child need not be one)
		int c = RandomAlive();
		if (c < 0 || Ours(c)->Width <= 0 || Ours(c)->Height <= 0)
			return;
		int Width = (int)(Random() % 60) - 30, Height = (int)(Random() % 60) - 30;
		bool Notify = Random() % 2 == 0;
		for (int s = 0; s < 2; s++)
			W[s]->FitToChild(Nodes[c].Side[s], TRect(x, y, Width, Height), Notify);
		break;
	}
	case 11: case 12: { // FitToChildren, Fit
		if (!CanFitToChildren(w))
			return;
		int B[4] = {(int)(Random() % 40) - 20, (int)(Random() % 40) - 20, (int)(Random() % 40) - 20,
			(int)(Random() % 40) - 20};
		bool Notify = Random() % 2 == 0;
		for (int s = 0; s < 2; s++) {
			if (Op == 11)
				W[s]->FitToChildren(TRect(B[0], B[1], B[2], B[3]), Notify);
			else
				W[s]->Fit(Notify);
		}
		break;
	}
	case 13: { // ClipChildren
		if (W[OURS]->Capture != NULL && !ChainOK(W[OURS]))
			return;
		bool Strict = Random() % 3 == 0;
		for (int s = 0; s < 2; s++)
			W[s]->ClipChildren(Strict);
		break;
	}
	case 14: case 15: case 16: case 17: case 18: case 19: { // mouse events
		int R[2];
		int Direction = 1 + Random() % 6;
		for (int s = 0; s < 2; s++) {
			switch (Op) {
			case 14: R[s] = W[s]->MouseMove(x, y); break;
			case 15: R[s] = W[s]->LeftButtonDown(x, y, f); break;
			case 16: R[s] = W[s]->LeftButtonUp(x, y, f); break;
			case 17: R[s] = W[s]->LeftButtonDoubleClick(x, y, f); break;
			case 18: R[s] = W[s]->RightButtonUp(x, y, f); break;
			case 19: R[s] = W[s]->MouseWheel(x, y, Direction); break;
			}
		}
		CHECK(R[0] == R[1], "%s: returned %d, original %d", What, R[0], R[1]);
		break;
	}
	case 20: { // MouseHover, HelpMouseMove
		int R[2] = {0, 0};
		bool Help = Random() % 2 == 0;
		if (Help) {
			SetHelpPosition(5, 7);
			At<void (__cdecl *)(int, int)>(0x4a2a00)(5, 7);
		}
		for (int s = 0; s < 2; s++) {
			if (Help)
				R[s] = W[s]->HelpMouseMove(x, y);
			else
				W[s]->MouseHover(x, y);
		}
		CHECK(R[0] == R[1], "%s: returned %d, original %d", What, R[0], R[1]);
		break;
	}
	case 21: { // SetCapture on a window or an element
		int c = RandomAlive();
		TComponent *C = Ours(c);
		bool IsWindow = Nodes[c].Kind == WINDOW;
		if (IsWindow ? !ChainFree(C) : (C->Parent == NULL || !ChainFree(C->Parent)))
			return;
		int Component = RandomChild(IsWindow ? c : w);
		for (int s = 0; s < 2; s++)
			Nodes[c].Side[s]->SetCapture(Component < 0 ? NULL : Nodes[Component].Side[s]);
		break;
	}
	case 22: { // ReleaseCapture
		int c = RandomAlive();
		TComponent *C = Ours(c);
		if (Nodes[c].Kind == WINDOW) {
			if (!ChainOK(C))
				return;
			int Component = IndexOf(OURS, ((TWindow *)C)->Capture);
			for (int s = 0; s < 2; s++)
				Nodes[c].Side[s]->ReleaseCapture(Component < 0 ? NULL : Nodes[Component].Side[s]);
		} else {
			if (C->Parent == NULL || ((TWindow *)C->Parent)->Capture != C || !ChainOK(C->Parent))
				return;
			for (int s = 0; s < 2; s++)
				Nodes[c].Side[s]->ReleaseCapture(NULL);
		}
		break;
	}
	case 23: { // StopDragging
		int c = RandomAlive();
		for (int s = 0; s < 2; s++)
			Nodes[c].Side[s]->StopDragging();
		break;
	}
	case 24: { // Draw, Arrange
		int Surface = Random() % 3;
		int dx = 100 + Random() % 100, dy = 100 + Random() % 100;
		if (!CanDraw(Ours(w), dx, dy))
			return;
		bool Arrange = Random() % 4 == 0;
		for (int s = 0; s < 2; s++) {
			if (Arrange)
				W[s]->Arrange();
			else
				W[s]->Draw(Surface, dx, dy);
		}
		break;
	}
	case 25: { // a new node
		AddNode(Random() % 3);
		break;
	}
	case 26: { // delete a whole tree
		if (Ours(w)->Parent != NULL || Random() % 4 != 0)
			return;
		for (int s = 0; s < 2; s++)
			delete Nodes[w].Side[s];
		break;
	}
	case 27: { // PositionChanged, KeyPress, HandleEvent reach the components unchanged
		int c = RandomAlive();
		int Value = Random() % 1000;
		for (int s = 0; s < 2; s++) {
			Nodes[c].Side[s]->PositionChanged();
			Nodes[c].Side[s]->KeyPress(Value);
			Nodes[c].Side[s]->HandleEvent(Value);
		}
		break;
	}
	default: { // getMinimumWidth/Height, the element defaults
		int c = RandomAlive();
		int R[2][10];
		SetHelpHandle(9);
		At<void (__cdecl *)(int)>(0x4a2a70)(9);
		for (int s = 0; s < 2; s++) {
			TComponent *C = Nodes[c].Side[s];
			R[s][0] = C->getMinimumWidth();
			R[s][1] = C->getMinimumHeight();
			if (Nodes[c].Kind == ELEMENT) {
				R[s][2] = C->MouseMove(x, y);
				R[s][3] = C->LeftButtonDown(x, y, f);
				R[s][4] = C->LeftButtonUp(x, y, f);
				R[s][5] = C->LeftButtonDoubleClick(x, y, f);
				R[s][6] = C->RightButtonUp(x, y, f);
				R[s][7] = C->MouseWheel(x, y, 2);
				C->MouseHover(x, y);
				R[s][8] = C->HelpMouseMove(x, y);
				R[s][9] = 0;
			} else {
				for (int i = 2; i < 10; i++)
					R[s][i] = 0;
			}
		}
		CHECK(memcmp(R[0], R[1], sizeof(R[0])) == 0, "%s: element defaults", What);
		break;
	}
	}
	Executed[Op]++;
	CompareTrees(What);
}

static void TestWindows()
{
	SetHelpPosition(0, 0);
	At<void (__cdecl *)(int, int)>(0x4a2a00)(0, 0);
	SetHelpHandle(0);
	At<void (__cdecl *)(int)>(0x4a2a70)(0);
	memcpy(WindowVtable, (void *)(uintptr_t)0x564528, sizeof(WindowVtable));
	WindowVtable[0] = (void *)OriginalWindowDestructor;
	memcpy(ElementVtable, (void *)(uintptr_t)0x5645c8, sizeof(ElementVtable));
	ElementVtable[0] = (void *)OriginalElementDestructor;
	for (int Round = 0; Round < 40; Round++) {
		Nodes.clear();
		Index[0].clear();
		Index[1].clear();
		for (int i = 0; i < 12; i++)
			AddNode(i < 4 ? WINDOW : Random() % 3);
		for (int t = 0; t < 3000; t++) {
			try {
				TreeOperation();
			} catch (TMyException &E) {
				printf("GUI: exception in round %d step %d: %s / %s\n", Round, t, E.GetMessage().c_str(), E.GetOrigin().c_str());
				exit(3);
			}
		}
		// Delete what is left, roots first (their children go with them).
		for (size_t i = 0; i < Nodes.size(); i++) {
			if (Nodes[i].Alive && Nodes[i].Side[OURS]->Parent == NULL) {
				for (int s = 0; s < 2; s++)
					delete Nodes[i].Side[s];
				CompareTrees("final deletion");
			}
		}
	}
}

// --- TComponent::Draw on its own --------------------------------------------

static void TestComponentDraw()
{
	typedef void (__fastcall *DrawFn)(void *, void *, int, int, int);
	for (int t = 0; t < 5000; t++) {
		TElement P;
		int Length = Random() % 6;
		P.DrawingList = Length > 0 ? new TDrawing[Length] : NULL;
		P.DrawingListLength = Length;
		for (int i = 0; i < Length; i++) {
			TDrawing &E = P.DrawingList[i];
			E.skin = Random() % 4 == 0 ? 0 : RandomSkin();
			E.x = (int)(Random() % 60) - 10;
			E.y = (int)(Random() % 60) - 10;
			bool Whole = Random() % 2 == 0;
			E.width = Whole ? 0 : (Random() % 8 == 0 ? 0 : 1 + Random() % 80);
			E.height = Whole ? 0 : 1 + Random() % 80;
			E.srcx = (int)(Random() % 4) - 1;
			E.srcy = (int)(Random() % 4) - 1;
			if (E.skin != 0 && (E.srcx >= Skins[E.skin].width || E.srcy >= Skins[E.skin].height))
				E.srcx = E.srcy = 0;
		}
		int Surface = Random() % 3, x = 10 + Random() % 100, y = 10 + Random() % 100;
		P.TComponent::Draw(Surface, x, y);
		At<DrawFn>(0x4a1fb0)(&P, NULL, Surface, x, y);
		CheckLogs("TComponent::Draw");
	}
}

// --- Timers ---------------------------------------------------------------

static void TestTimers()
{
	typedef void (__fastcall *TimerCtorFn)(void *, void *, int, TCallback);
	typedef void (__fastcall *TimerUpdateFn)(void *, void *, int);
	typedef int (__fastcall *TimerIntFn)(void *, void *);
	typedef void (__fastcall *VectorInitFn)(void *, void *, int, int, int, void *);
	At<VectorInitFn>(0x4973f0)((void *)(uintptr_t)0x5ccfa4, NULL, 0, 0, 2, NULL);
	TProbe *Receivers[2][4];
	for (int s = 0; s < 2; s++)
		for (int i = 0; i < 4; i++)
			Receivers[s][i] = new TProbe(s, 100 + i);
	int Clock = 1000;
	SetTestClock(Clock);
	UpdateTimers(); // starts both clocks
	At<void (__cdecl *)()>(0x4a27c0)();
	std::vector<TTimer *> Timers[2];
	for (int t = 0; t < 4000; t++) {
		int Op = Random() % 6;
		if (Op <= 1) {
			int Time = Random() % 8 == 0 ? 0 : (int)(Random() % 30);
			int r = Random() % 5, Event = Random() % 100;
			TCallback C[2] = {TCallback(r < 4 ? Receivers[0][r] : NULL, Event),
				TCallback(r < 4 ? Receivers[1][r] : NULL, Event)};
			Timers[OURS].push_back(new TTimer(Time, C[OURS]));
			void *Memory = operator new(0x14);
			At<TimerCtorFn>(0x49b290)(Memory, NULL, Time, C[ORIGINAL]);
			Timers[ORIGINAL].push_back((TTimer *)Memory);
		} else if (Op == 2 && !Timers[OURS].empty()) {
			size_t i = Random() % Timers[OURS].size();
			delete Timers[OURS][i];
			At<void (__fastcall *)(void *, void *, int)>(0x4a1dc0)(Timers[ORIGINAL][i], NULL, 1);
			for (int s = 0; s < 2; s++)
				Timers[s].erase(Timers[s].begin() + i);
		} else if (Op == 3 && !Timers[OURS].empty()) {
			size_t i = Random() % Timers[OURS].size();
			int Elapsed = (int)(Random() % 3000) - 100;
			Timers[OURS][i]->Update(Elapsed);
			At<TimerUpdateFn>(0x4a1df0)(Timers[ORIGINAL][i], NULL, Elapsed);
		} else {
			Clock += Random() % 700;
			SetTestClock(Clock);
			UpdateTimers();
			At<void (__cdecl *)()>(0x4a27c0)();
		}
		for (size_t i = 0; i < Timers[OURS].size(); i++) {
			TTimer *A = Timers[OURS][i], *B = Timers[ORIGINAL][i];
			CHECK(A->Remaining == B->Remaining && A->Active == B->Active, "timer %d: %d,%d original %d,%d", (int)i,
				A->Remaining, A->Active, B->Remaining, B->Active);
			CHECK(A->GetTime() == At<TimerIntFn>(0x496000)(B, NULL), "TTimer::GetTime");
			CHECK(A->IsExpired() == ((At<TimerIntFn>(0x496020)(B, NULL) & 0xff) != 0), "TTimer::IsExpired");
		}
		vector<TTimer *> &Original = *(vector<TTimer *> *)(uintptr_t)0x5ccfa4;
		CHECK(Original.min == 0, "timer vector min");
		CheckLogs("timers");
	}
	for (size_t i = 0; i < Timers[OURS].size(); i++) {
		delete Timers[OURS][i];
		At<void (__fastcall *)(void *, void *, int)>(0x4a1dc0)(Timers[ORIGINAL][i], NULL, 1);
	}
	CheckLogs("timer deletion");
	for (int s = 0; s < 2; s++)
		for (int i = 0; i < 4; i++)
			delete Receivers[s][i];
	Log[0].clear();
	Log[1].clear();
}

// --- Hotkeys and callbacks ----------------------------------------------------

static void TestHotkeys()
{
	typedef void (__fastcall *VectorFn)(void *, void *, int, int, int);
	typedef void (__cdecl *AddFn)(int, TCallback);
	typedef void (__cdecl *KeyFn)(int);
	typedef bool (__cdecl *CheckFn)(int);
	typedef void (__fastcall *SendFn)(void *, void *);
	typedef void (__fastcall *CallbackCtorFn)(void *, void *, TComponent *, int);
	At<VectorFn>(0x4976f0)((void *)(uintptr_t)0x5ce4f0, NULL, 0, 50, 10);
	InitHotkeys();
	At<void (__cdecl *)()>(0x496150)();
	TProbe *Receivers[2][3];
	for (int s = 0; s < 2; s++)
		for (int i = 0; i < 3; i++)
			Receivers[s][i] = new TProbe(s, 200 + i);
	for (int t = 0; t < 30000; t++) {
		int Key = Random() % 4 == 0 ? (int)(Random() % 0x800) : (int)(0x41 + Random() % 30) | (int)(Random() % 4 << 9);
		int Op = Random() % 10;
		if (Op < 3) {
			int r = Random() % 4, Event = Random() % 300;
			TCallback C(r < 3 ? Receivers[OURS][r] : NULL, Event);
			TCallback D;
			At<CallbackCtorFn>(0x496130)(&D, NULL, r < 3 ? Receivers[ORIGINAL][r] : NULL, Event);
			AddHotkey(Key, C);
			At<AddFn>(0x4a10a0)(Key, D);
		} else if (Op < 5) {
			RemoveHotkey(Key);
			At<KeyFn>(0x4a12a0)(Key);
		} else if (Op < 9) {
			bool A = CheckHotkey(Key);
			bool B = (At<int (__cdecl *)(int)>(0x4a1480)(Key) & 0xff) != 0;
			CHECK(A == B, "CheckHotkey(0x%x) %d, original %d", Key, A, B);
		} else {
			int r = Random() % 4, Event = Random() % 300;
			TCallback C(r < 3 ? Receivers[OURS][r] : NULL, Event);
			TCallback D(r < 3 ? Receivers[ORIGINAL][r] : NULL, Event);
			C.Send();
			At<SendFn>(0x4a0f00)(&D, NULL);
		}
		CheckLogs("hotkeys");
		if (t % 3000 == 0) {
			for (int Key = 0; Key < 0x800; Key++) {
				bool A = CheckHotkey(Key);
				bool B = (At<int (__cdecl *)(int)>(0x4a1480)(Key) & 0xff) != 0;
				CHECK(A == B, "CheckHotkey(0x%x) %d, original %d", Key, A, B);
			}
			CheckLogs("hotkey sweep");
		}
	}
	// The table itself, through the callbacks it calls.
	for (int Key = 0; Key < 0x800; Key++) {
		bool A = CheckHotkey(Key);
		bool B = (At<int (__cdecl *)(int)>(0x4a1480)(Key) & 0xff) != 0;
		CHECK(A == B, "CheckHotkey(0x%x) %d, original %d", Key, A, B);
	}
	CheckLogs("hotkey table");
	for (int s = 0; s < 2; s++)
		for (int i = 0; i < 3; i++)
			delete Receivers[s][i];
	Log[0].clear();
	Log[1].clear();
}

void TestGUI()
{
	PatchImage();
	PatchJump(0x490c60, (void *)ImagePicture);
	PatchJump(0x4910e0, (void *)ImageClippedPicture);
	RedirectOurs(0, (void *)DrawPicturePart, (void *)OurPicture, false);
	RedirectOurs(1, (void *)DrawPicturePartClipped, (void *)OurClippedPicture, false);
	PrepareSkins();
	int Before = Checks, BeforeFailures = Failures;
	TestSkinProperties();
	TestDrawSkin();
	TestComponentDraw();
	TestWindows();
	TestTimers();
	TestHotkeys();
	int Least = Executed[0];
	for (int i = 1; i < 30; i++)
		Least = Executed[i] < Least ? Executed[i] : Least;
	printf("GUI: %d checks, %d failures (each of the 30 window operations at least %d times)\n", Checks - Before,
		Failures - BeforeFailures, Least);
	ExitSkins();
	Global<TSkin *>(0x71c454) = NULL;
	RedirectOurs(0, (void *)DrawPicturePart, NULL, true);
	RedirectOurs(1, (void *)DrawPicturePartClipped, NULL, true);
}
