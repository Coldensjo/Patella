# Reconstruction progress

Read this and `git log` first when resuming.

## Backlog

1. Script.cpp — **done** (commit "Reconstruct Script.cpp").
2. Preferences/Script gap (0x4ddc00-0x4df6ca) — **done**: loader/saver/ExitPreferences in Preferences.cpp, inline code in Script.h.
3. Player.cpp, incl. the error.txt reader (0x4da140) — **done**.
4. Small gaps — **done**: ContextMenus/Control (Control.cpp), start/Communication
   (InitCommunication + STL), Status/SurfaceDX (Surface.cpp + SurfaceDX.cpp range).
5. Objects.cpp and the Objects/Player gap — **done** (Claude Opus 5.5 delegate, reviewed, merged). The gap is all Player.obj.
6. Map.cpp and the gaps on both sides — **done** (Claude Opus 5.5 delegate, third attempt, reviewed, merged).
7. MapWindow.cpp and the MapWindow/Network gap — **done** (issue #1, Claude Opus 5.5; the gap is Network.cpp).
7a. MainWindow.cpp and the MainFrm/MainWindow gap — **done** (issue #2, Claude Opus 5.5).
7b. DialogWindow.cpp and the Dialogs/DialogWindow gap — **done** (issue #3, Claude Opus 5.5).
7c. ContextMenus.cpp and the ContentWindows/ContextMenus gap — **done** (issue #4, Claude Opus 5.5).
7d. ConsoleWindow.cpp and the gaps on both sides — **done** (issue #5, Claude Opus 5.5; the second gap is ContentWindows.obj).
7e. ContentWindows.cpp — **done** (issue #6, Claude Opus 5.5; the gap before it was settled in issue #5).
7f. DialogElements.cpp and the DialogElements/Dialogs gap — **done** (issue #7, Claude Opus 5.5; the gap is Dialogs.obj).
7g. Dialogs.cpp with its leading run of header code — **done** (issue #8, Claude Opus 5.5).
8. Creatures.cpp and the Control/Creatures, Creatures/DialogElements gaps — **done** (Claude Opus 5.5 delegate, reviewed, merged; the second gap is DialogElements.cpp + Container.h).
9. Talk.cpp and the gaps next to it — **done** (ChatGPT gpt-6.1-sol, sent back once for names/shims, then merged).
10. The remaining files in link order, smallest first. Done: sysinc.cpp, Help.cpp, Inventory.cpp, MainFrm.cpp, GUI.cpp, graphics.cpp (delegates, merged).

## Current item

SurfaceDX.cpp in progress (issue #9, November 2, 2026), isolated worktree:
the full DirectDraw object is reconstructed through 0x4efd0f; comprehensive
fake-COM tests and final validation are in progress. Delivery is a PR, not
a direct push to main. Another agent owns Dialogs.cpp (#8); its checkout
and uncommitted changes are untouched. Remaining surface engines keep their
own issues; the gap 0x4efd10-0x4f1ea4 is now assigned to surfaceDX9.cpp.

## Delegates (running)

SurfaceDX differential-test coverage (fake DirectDraw/Direct3D and Win32 effects).

## Open questions

- None yet.

## Last difftest

12,744,375 checks against the original + 67 in sysinctest.exe, 0 failures; 3,201 vlong checks, 0 failures
(after issue #8; Dialogs: 98,889).

## Notes for later files

- The Bash tool expands `\n` inside heredocs and sed replacements: write
  C/C++ escapes with the Edit/Write tools, not through shell commands.
- tools/callcheck.py now resolves member calls named `Class_method` in
  names.tsv (own class first, else a unique method name).
- Map.cpp (delegate branch): TObject now clears itself (default member
  initializers), TRGBColor has constructors (palette index 0x425950, R/G/B,
  default), vector<T> has its destructor, Container.h has matrix<T> and
  matrix3d<T>. Communication.cpp's map code uses TObject; TCommunicationObject
  is left for the Inventory.cpp calls. The Map/MapWindow gap has names.tsv
  rows (TMapWindow_*) for MapWindow.cpp.
- The difftest image's static initialisers never run: call the original
  constructors of a file's static objects explicitly (maptest.cpp does it
  for Map.cpp's), and keep inputs away from paths that raise exceptions in
  the original (e.g. opening a missing file).
- tools\difftestuild.cmd can fail once without a message (a difftest.exe
  still locked from the previous run); kill difftest.exe and run it again.
- Run tools\difftest\build.cmd from PowerShell (cmd /c ...). Launched from the
  Git Bash tool it sometimes ends with exit 1 and no output, while the same
  build passes from PowerShell; build.cmd sets a private TEMP per checkout
  because the tests write to fixed folders under %TEMP%.
- MapWindow.cpp: TObject converts to int (Type), so TEND1(Obj) and
  comparisons like "SrcObj!=0" compile as in the original; an rvalue TObject
  now also matches GetObjectFlag(int, int). Status.cpp's Set/Get*Object take
  (Index, x, y, z). The surface slots +0x20, +0x30, +0x40, +0x4c, +0xa4 have
  names and parameters (Surface.h); a test that needs them defines the
  TSurfaceGL methods (mapwindowtest.cpp), the others stay playertest.cpp's
  stubs.
- MainWindow.cpp creates TMapFrame (MapWindow.h) with operator new(0x40); its
  deleting destructor is folded into TStartWindow's 0x4adc40 (MainWindow.obj).
- The text header's table is copied into every object that includes it; use
  the copy of the object at hand (external.h lists Control.obj's and
  MapWindow.obj's entries).
- mapwindowtest.cpp's PatchWorld redirects many functions of our Map, Creatures,
  Status, Preferences, Talk, Help, graphics, GUI and Inventory code for good;
  keep it the last test in difftest.cpp or restore them.
- A difftest crash that depends on the environment (TEMP's length) is a
  heap error that the memory layout hides: talktest.cpp had a double delete
  (fixed). _heapchk() between the tests finds the first one that breaks the
  heap.
- MainWindow.cpp: the windows it creates from DialogElements.cpp are sketches in external.h
  (TBorder, TTextButton, TLabel, TObjectIcon; TDialogWindow and TSubWindow are real since issue #3,
  TConsoleWindow since issue #5, the ContentWindows.cpp windows and creators since issue #6);
  replace them with the real classes and check MainWindow.cpp's calls (notes/decisions.md).
- mainwindowtest.cpp makes the not-reconstructed windows fakes for both sides: the image's
  constructors jump to fastcall wrappers that placement-new the fake, whose constructor then sets
  the vtable of a test subclass (extra overrides, same layout). Reuse it for the window files; when
  one of those files is reconstructed, its fakes in mainwindowtest.cpp must go (duplicate
  definitions). DIFFTEST_ONLY=MainWindow runs that test alone.
- SkinProperties.Style0D8 is now five TGUIPoints (+0x10-+0x37) and a TRect at +0x38.
- DialogWindow.cpp (issue #3): the content windows of the dialog columns are TContentWindow (base,
  0x4854c0) and TSubWindow (with a body); ContentWindows.cpp's side-bar windows derive from
  TContentWindow and its creators size their windows before Rearrange(16) (MakeRoom calls
  Work->SetSize before Work is a child). Preferences.h's stored size is TContentWindowSize now.
- The shared fakes of TResizer, TSkinButton::SetSkins and the container body's UpdateUpButton
  (FakeUpdateUpButton) are in dialogwindowtest.cpp, reporting through difftest.h's TDialogFakes
  hooks; when DialogElements.cpp is reconstructed, those definitions must go (duplicates) and the
  tests that use them (dialogwindowtest.cpp, mainwindowtest.cpp, contentwindowstest.cpp) need the
  real classes or new fakes.
  DIFFTEST_ONLY=DialogWindow runs the dialog window test alone; it restores every patch it makes.
- tools/callcheck.py resolves `Class::method(` (with TWindow -> TComponent) and does not match
  `x->Name(` to a free function; differences counted in older notes were made without that.
- Python on Windows writes CRLF in text mode: pass newline='\n' when an edit script rewrites a file
  (and write such scripts with the Write tool: the Bash tool expands backslash escapes in them).
- ContextMenus.cpp (issue #4): ContextMenus.h needs Map.h (TMark, TCoordinate), which includes
  external.h, so external.h cannot include it; its users include it directly (Control.cpp).
  (The console's classes are real since issue #5; contextmenustest.cpp fakes their methods on both
  sides.) When DialogElements.cpp (TEditLine,
  TFormattedText 0x465e60, TSeparator 0x452cb0) or ContentWindows.cpp (TSkillsWindowBody) are
  reconstructed, contextmenustest.cpp's definitions of their methods and constructors must go
  (duplicates), and its fakes must move to the real classes. DIFFTEST_ONLY=ContextMenus runs the test
  alone; it restores every patch it makes.
- Redirecting "our" copy of a function that a test file defines itself can patch the fake into a jump
  to itself: the linker folds a definition that only calls the fake with the fake. Redirect only the
  image's address for such functions (contextmenustest.cpp's Send* and IsClipboardEmpty).
- A menu's Show shrinks the last entry each time it runs; tests must not call it again once the last
  entry would drop below one pixel (both versions fail).
- ConsoleWindow.cpp (issue #5): ConsoleWindow.h is included by external.h (it needs only GUI.h and Talk.h;
  the DialogElements classes it points to are forward declarations). consolewindowtest.cpp defines the shared
  fakes of the DialogElements classes the console creates (TEditLine, TScrollBar/TScrollBarRange,
  TSkinButton's constructor, TToggleButton, TMultiStateButton); they log only while that test runs, and
  contextmenustest.cpp and dialogwindowtest.cpp link them. When DialogElements.cpp is reconstructed, those
  definitions must go (duplicates) and the three tests need the real classes or new fakes.
- mainwindowtest.cpp reaches our TConsoleWindow constructor through its decorated name
  (#pragma comment(linker, "/alternatename:...")): a constructor has no address in C++. The same works for
  any constructor a fake must replace.
- Test files must not define classes (with inline members or vtables) of the same name: the linker keeps one
  COMDAT, and an object of the other file's class gets the wrong vtable (consolewindowtest.cpp's first buffer
  class called contextmenustest.cpp's TTestBuffer methods).
- Talk.cpp's TTalkBuffer::Remove(int) only finds line 0 (the binary never increments its counter); tests that
  remove other lines use Remove(TTalkLine *).
- Our DrawSkin is inlined into TComponent::Draw (GUI.cpp): patching DrawSkin does not reach the drawing lists;
  fake DrawPicturePart instead and let DrawSkin run on both sides (consolewindowtest.cpp).
- tools/callcheck.py splits functions at every comment line that starts with "// 0x"; write continuation lines
  so that they do not.
- tools/triage.py lets header evidence win over file-ranges.tsv (as assertion evidence does).
- ContentWindows.cpp (issue #6): ContentWindows.h needs Map.h (TMark), so its users include it directly
  (MainWindow.cpp, DialogWindow.cpp, ContextMenus.cpp). TSubWindowBody's virtuals are inline in it
  (Minimize, Maximize, IsMinimized, GetMaximumHeight, LeftButtonDoubleClick returning true); a test body
  derived from it must implement GetInitialHeight and GetContentHeight, and a body that MainWindow.cpp's
  OpenContainer gets needs a first new virtual at +0xb8 (the container body's Update).
- The shared DialogElements fakes report every object they make through difftest.h's ElementCreated
  (contentwindowstest.cpp sets it; "@%p" in the arguments is replaced by the object's name). New fakes
  of the members ContentWindows.cpp uses are in contentwindowstest.cpp (TTextButton's second constructor,
  TTextToggleButton, TLabel's area constructor and SetText, TSkinIcon, SetRadioButtonGroup,
  TScrollBarRange::SetValue). mainwindowtest.cpp's and contextmenustest.cpp's fakes log and register only
  while their tests run (Running). When DialogElements.cpp is reconstructed, all of them must go.
- contentwindowstest.cpp makes the image's operator new and ours fill new memory with the same byte
  pattern (members the original leaves uninitialised then agree, also in objects the creators make);
  world values a window sets and reads in the same operation need one copy per side; never call
  Random() inside Both (the sides would diverge), and decide test inputs before Both from our side's
  state. Our printf is faked only while our side runs (CHECK prints with it).
- SkinProperties: Style288 has arrays (Data10[10] points, Data60[10] rects, Data100[10] ints), PlayerStates
  is a block at +0x470, Style4AC/Style4FC are TCombatControlsProperties, Data670/Data67C and
  BuddylistWindow.Data3C/Data48 are colours (issue #6). TSurface's +0x0c, +0x10, +0x18 have names.
- DIFFTEST_ONLY=ContentWindows runs contentwindowstest.cpp alone (CT_TRACE=1 traces rounds and actions,
  CT_LOGS=<file> writes every compared log, for checking the coverage).
- DialogElements.cpp (issue #7): DialogElements.h includes external.h (for TObject), Status.h and
  Skins.h; its users include it directly. The window tests' DialogElements fakes are mirror classes of
  the old sketches (tools/difftest/elementfakes.h, TFake*): elementfakes.cpp's PatchElementFakes
  redirects the image's and our constructors (decorated names through /alternatename) and non-virtual
  members to them; each of those tests calls it with its own Redirect. TObjectIcon's and TSeparator's
  constructors and TMultiStateButton::Reset are inline in DialogElements.h (our callers inline them), so
  they run for real on both sides. A new window test that needs element fakes calls PatchElementFakes;
  one that needs real elements (dialogelementstest.cpp) must run while no fake is patched.
- GUI.h's +0x3c/+0x40 are getMinimumWidth/getMinimumHeight (DialogElements.cpp's assertions).
- Surface.h's +0x1c is DrawVerticalLine; SkinProperties.CheckBox.Data08 is a TRect.
- tools/ehinfo.py also reads the prologue PUSH -1; MOV EAX, FS:[0]; PUSH stub.
- Dialogs.cpp (next window file): the DialogElements/Dialogs gap 0x46b790-0x46c0ce is Dialogs.obj's
  leading run (Dialogs.h's TLabel subclasses and other inline constructors and destructors, Container.h's
  vector of a 31-byte element); Dialogs.cpp calls the DialogElements.h inline copies (SetOutfit,
  TSkinButton::SetEnabled, TListBox::GetSelection/SetEditable, TFontListBox, TEditText,
  THorizontalScrollBar with its callback, TLink with OpenURL).
- DIFFTEST_ONLY=DialogElements runs dialogelementstest.cpp alone (DE_TRACE=1, DE_LOGS=<file>).
- Dialogs.cpp (issue #8): Dialogs.h includes DialogElements.h, Map.h and Preferences.h; its users include it
  directly (Control.cpp). The creators return the frame TDialog (a TWindow) for ModalWindow. TDialog's body is
  a TComponent (labels are bodies too). TObject's second member is Data (assertion text), TBorder frames a
  TComponent, TListBox has GetNumberOfLines (inline), TSurface has GetFullscreenModes (+0x8c),
  SetFramerateLimit (+0xac) and the member GetAvailableEngines.
- SkinProperties: build/scratch/dg8/retype.py relayouts blocks from a table (offset, type, name), checks the
  constructor-zeroed bytes and rewrites every use by offset; reuse it for later retypes.
- The original's SetChildArea takes a TRect by value (callers build a 16-byte block); GUI.h keeps the five
  ints (same stack layout; issue #27).
- The image's iostreams cannot run in the test child (the CRT's static initialisers are not run and call
  imports): functions that build a std::stringstream in the image (the error report dialog's constructor,
  Container.h's index errors) cannot be called on the image side.
- dialogstest.cpp's dump prints every member word with pointers named (tree components, receivers, timers)
  and strings only for the members listed as strings; values with two 0xab bytes are pattern, not pointers.
  Event 31 only with a timer, event 11 of the hotkey dialog only with a selection.
- DIFFTEST_ONLY=Dialogs runs dialogstest.cpp alone (DG_TRACE=1, DG_LOGS=<file>).
