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
7h. SurfaceDX.cpp with its trailing DirectDraw functions — **done** (issue #9, GPT-6.1-sol with Claude Opus 5.5 delegates; PR for review).
7i. surfaceDX9.cpp and the surfaceDX9/SurfaceGL gap — **done** (issue #10, Claude Opus 5.5; the gap is SurfaceGL.obj, merged with #9).
7j. SurfaceGL.cpp with its glh_extensions.h code (the surfaceDX9/SurfaceGL gap) — **done** (issue #11, Claude Opus 5.5).
7k. The library region and its third-party code (Henry Spencer's regexp) — **done** (issue #15, Claude Opus 5.5).
8. Creatures.cpp and the Control/Creatures, Creatures/DialogElements gaps — **done** (Claude Opus 5.5 delegate, reviewed, merged; the second gap is DialogElements.cpp + Container.h).
9. Talk.cpp and the gaps next to it — **done** (ChatGPT gpt-6.1-sol, sent back once for names/shims, then merged).
10. The remaining files in link order, smallest first. Done: sysinc.cpp, Help.cpp, Inventory.cpp, MainFrm.cpp, GUI.cpp, graphics.cpp (delegates, merged).

## Current item

The library region done (issue #15, November 2, 2026, branch issue-15): every library function has a names.tsv row,
the regexp is src/Extern/regexp. Next: the other open issues; see GitHub (#30: the atoi callers).

## Resources (issue #17)

src/res/Patella.rc + resource.h rebuild all 55 entries of Tibia.exe's .rsrc byte for byte (types, IDs,
languages). `tools\build-resources.cmd` regenerates the pictures (tools/extract_resources.py Tibia.exe
build/res; not committed), compiles build\res\Patella.res with rc.exe and compares it with Tibia.exe
(tools/compare_resources.py: 0 differences). For #16: compile Patella.rc with /I build\res, link the .res,
do not include MFC's afxres.rc, link with /MANIFEST:NO, then run compare_resources.py Tibia.exe Patella.exe.
Details and the users of every ID: notes/resources.md.

## Delegates (running)

None.

## Open questions

- None yet.

## Last difftest

14,298,227 checks against the original + 67 in sysinctest.exe, 0 failures; 3,201 vlong checks, 0 failures
(issue #15; Regexp: 229,958, SurfaceDX: 1,213,366, SurfaceDX9: 58,682, SurfaceGL: 53,082; every other test's total as before).

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
- SurfaceDX.cpp (issue #9): the engine uses DirectDraw4/Surface4 and
  Direct3D3/Device3/Viewport3 despite its DirectX 5 name. TSurface is 0x70
  bytes (8-byte alignment), TSurfaceDX is 0x250. Base Window is now hWindow;
  the +0x50/+0x54 color-key, +0x64 attachment and +0x9c recreation slots
  have typed names. Keep the shared +0xac name SetFramerateLimit.
- Shared surface statistics span 0x71d0bc-0x71d0f4; private DirectDraw
  SurfaceMemory starts at 0x71d0f8. CacheSurfaceWidth/Height start at 256,
  InitSurfaces sets them to 320. Do not duplicate these globals for DX9/GL.
  Source ownership is inferred; their common binary use is verified.
- DIFFTEST_ONLY=SurfaceDX runs surfacedxtest.cpp alone (DX_TRACE=1 prints
  phase coverage, DX_LOGS=<file> writes compared logs). All code/IAT patches
  are restored. Never read the uninitialized DDSCAPS2 words after dwCaps.
  The display-mode callback intentionally tests the refresh flag in
  ddpfPixelFormat.dwFlags rather than the descriptor's dwFlags.
- tools/ehinfo.py accepts interleaved argument loads in the SEH prologue
  and validates FuncInfo magic (needed by SurfaceDX's stretch wrappers).
- surfaceDX9.cpp (issue #10): surfaceDX9.h includes the SDK's d3d9.h, which is empty after SurfaceDX.h's
  DirectX 6 d3d.h (DIRECT3D_VERSION 0x700): never include both; Surface.h declares IsDirect3D9Installed and
  CreateSurfaceDX9 for Surface.cpp. TSurfaceDX9's own members start at +0x70 (Ready); its surfaces are
  textures (Surface9[]) whose level 0 is RenderTarget[].
- SurfaceGL.cpp (#11): its object starts at 0x4f46f0 with glh_extensions.h code; six of its functions
  (0x4f9d00, 0x4fa110, 0x4fa2c0, 0x4fab70, 0x4fb5e0, 0x4fb5f0) are also the DX5/DX9 slots, written in those
  files too. The display-mode callback and the frame timer are the engines' identical copies as well.
- surfacedx9test.cpp fakes the Windows functions by writing the fakes into the image's import slots and
  difftest.exe's own (OwnImportSlot), restored at the end; its fake COM objects are generic (a vtable of traps
  with the used slots filled in, MakeVtbl), reusable for SurfaceGL.cpp's test. Fake texture memory must
  be large enough for every write (a too small test texture corrupted the heap once). DIFFTEST_ONLY=SurfaceDX9
  runs it alone (DX9_TRACE=1, DX9_LOGS=<file>, DX9_ROUNDS=<n>).
- TRect's members may be width/height in the original (issue #29).
- Unknown-owner data (issue #12, notes/decisions.md): every object that includes the text header has
  a full copy of its table (147 entries, 0x24c bytes, "You are poisoned" ... "Report Rule Violation");
  the object whose code reads an entry owns that copy (Player.obj 0x5c22cc, MapWindow.obj 0x5c2064,
  Control.obj 0x5c0b34). The window objects' unused NoCallback sits at the end of their .bss, one
  __xc initialiser per object (Control.obj 0x5c4558, Creatures.obj 0x5cc418). The "engine N" value
  is Surface.cpp's CommandLineEngine (Surface.h).
- SurfaceGL.cpp (#11): the XOR code tables right after its last .rdata string (0x5733a4/0x5733c4) were
  given to sysinc.cpp (low: file-scope data comes first in an object's .rdata); if SurfaceGL.cpp turns
  out to define file-scope data after its functions, move them there.
- SurfaceGL.cpp (issue #11): SurfaceGL.h includes the SDK's GL/gl.h (no conflict with either DirectX header;
  Surface.cpp includes it for new TSurfaceGL and IsOpenGLWorking). NVIDIA's glh_extensions.h and
  glh_genext.h are reconstructed in src/Extern/glh (glh_genext.h generated from the binary by
  build/scratch/gl/genext.py); only SurfaceGL.cpp may define GLH_EXT_SINGLE_FILE. TSurface's +0x04/+0x08/+0x80
  are SelectSurface, SetTextureFilter and AddDirtyRect(TRect) (overridden only by SurfaceGL.cpp), +0x0c is
  DeviceContext and +0x28 ScreenRect. MainFrm.cpp's DisplayChanged (0x5d1670) is global (Init reads it).
- The tests' recording surface is tools/difftest/testsurface.h's TTestSurface (formerly external.h's
  TSurfaceGL sketch): a new window test derives from it; the slots graphicstest.cpp and mapwindowtest.cpp
  define are the recorders, the others abort.
- difftest.exe links SurfaceGL.cpp but not opengl32/glu32: surfacegltest.cpp provides the OpenGL import
  symbols (__imp__glXxx@n via /alternatename to its GLSlot_ variables) and gluOrtho2D. A test can find a
  static of ours (no name outside its file) by making it take a sentinel value and searching difftest.exe's
  writable sections (surfacegltest.cpp's FindWord/FindChange). DIFFTEST_ONLY=SurfaceGL runs it alone
  (GL_TRACE=1, GL_LOGS=<file>, GL_ROUNDS=<n>); it has its own random generator.
- Library region (issue #15): names.tsv has a row for each of the 1,102 functions from 0x50eae8 (kind stl, the
  library first in the note: "CRT: MSVC 7.1 libcmt.lib", "MFC 7.1: nafxcw.lib", "C++ library: MSVC 7.1 libcpmt.lib",
  "ATL 7.1 header code in MFC 7.1", import/delay-load thunks, "third-party: Henry Spencer regexp"); file-ranges.tsv
  has the blocks and tools/triage.py writes them into symbols.tsv's file column (a block that names a game file
  makes its rows kind game: Inventory.cpp's initialiser 0x559ea0). The CRT names drop one leading underscore of
  Ghidra's (memmove, _lock); 0x51262a is atoi (the sources' atol calls: #30).
- tools/callcheck.py counts a library-region name only when Ghidra left the function unnamed and the file's
  pseudocode calls it by that name (Ghidra's own names such as _memcpy stay in the pseudocode).
- src/Extern/regexp is Henry Spencer's regexp (C, K&R): include "Extern/regexp/regexp.h" (extern "C"
  prototypes). regerrorhandler (the image's 0x71f034) is NULL in the client; regexptest.cpp sets both sides'.
  DIFFTEST_ONLY=Regexp runs regexptest.cpp alone (RX_SEED, RX_ROUNDS, RX_TRACE=1 prints the coverage);
  it has its own random numbers and restores the image's hook. compile-check also compiles src/Extern/regexp/*.c.
