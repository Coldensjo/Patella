# Decisions made without asking

Judgement calls made during the reconstruction instead of stopping to ask,
with the evidence, for review. Confidence: high, medium, low.

## Script.cpp

- **Class and member names from CipSoft's game server** (medium). The client
  binary has no names for the script and binary-file classes. Its tokenizer is
  the same state machine as the game server's `TReadScriptFile::nextToken`
  (same states, the same special characters 'L', 'G', 'N', 'I' for `<=`, `>=`,
  `<>`, `->`, the same token kinds), and the client's TERROR names the state
  variable `State`, as the server does. So the classes are `TReadScriptFile`,
  `TWriteScriptFile`, `TReadBinaryFile`, `TWriteBinaryFile` and the methods use
  the server's lower-case names (`open`, `nextToken`, `getIdentifier`,
  `readNumber`, `writeLn`, ...), not the TitleCase that notes/reconstruction.md
  asks for invented names: these are taken, not invented. Fields (`Token`,
  `File`, `Filename`, `Line`, `String`, `Bytes`, `Number`, `CoordX/Y/Z`,
  `Special`) and the `TOKEN` enumerators likewise. In names.tsv members are
  `Class_method`.
- **MAX_IDENT_LENGTH 20 and MAX_STRING_LENGTH 1000** (high for the values,
  low for the names): from the checks in nextToken and the 1000-byte buffer.
- **Patch functions named ApplyPatch, ApplyFullPatch, InstallPatch** (low).
  0x4e3100 applies a difference patch received from the login server,
  0x4e3500 writes a complete file, 0x4e3610 (lines 879-883) replaces the file
  by the patched copy. Names are ours.
- **Inline header functions go to Script.h** (high). The constructors,
  destructors and `close()` functions at 0x4df550-0x4df6c9 sit in the
  Preferences/Script gap, before Script.cpp's first function. They are COMDAT
  copies from Preferences.obj (the Tibia.cfg loader and saver are the first
  users in link order), and Script.cpp's own ApplyPatch calls 0x4df6b0 from its
  catch block. Which one of `close()` and the destructor each copy is follows
  the loader (it calls 0x4df5d0 then 0x4df5b0 at the end of the file) and
  ApplyPatch (0x4df6b0 in the catch handler, which closes, and again in the
  continuation, which destroys). Identical bodies were folded by the linker:
  0x4df690 is both binary-file constructors, 0x4df6b0 both `close()`s and
  destructors, 0x4df660 both `close()` and the destructor of TWriteScriptFile.
- **Source order**: line order; TReadBinaryFile::readString (line 644, placed
  by the compiler after InstallPatch at 0x4e3a10) is put between readQuad and
  readBytes. The four `read*` helpers of TReadScriptFile (0x4e3920-0x4e39d0) have
  no line evidence and stay at the end, where the binary has them.
- **error()'s "(unknown)"** is written as a local array because the binary
  copies the text to the stack.
- **nextToken's "string too long" check** is written at the end of the loop
  body; the binary jumps straight into the switch on the first pass. The
  error calls are noreturn for the compiler, so the code after them does not
  show the original loop shape.

## Talk.cpp

Integration naming audit, November 1, 2026, by ChatGPT gpt-6.1-sol. No
original function spellings are present in these entries' assertion/catch
strings, so the TitleCase names below are inferred from behavior and callers.
Confidence is **medium** for the chosen name/role and **low** for its exact
original spelling; the field offsets, flags, arguments and vtable slots are
**high** confidence from instructions and differential tests. Assertion/catch
identifiers (Speaker, TargetBuffer, ReceiveTalkMode, Addressee, etc.) are kept.

| Address | Chosen name | Confidence | Behavior |
|---|---|---|---|
| 0x4fd4c0 | SetTalkChanged | medium; spelling low | Sets the talkbox/status redraw latch |
| 0x4fd540 | SetTalkThrottle | medium; spelling low | Restarts the ignored-talk throttle for 1000 milliseconds |
| 0x4fd590 | SetConsoleChanged | medium; spelling low | Sets the console redraw latch |
| 0x5052e0 | AddTalkbox | medium; spelling low | Adds or merges positioned speech/private/broadcast text with its speaker prefix |
| 0x505ac0 | AddMessageTalkbox | medium; spelling low | Adds queued event/admin text or replaces the information talkbox |
| 0x4ff130 | UpdateTalkboxes | medium; spelling low | Expires talkboxes using elapsed milliseconds since the previous update |
| 0x4ff500 | SetStatusMessage | medium; spelling low | Copies or clears the status message and sets its permanent/50-tick lifetime |
| 0x4ff7d0 | UpdateStatusMessage | medium; spelling low | Decrements the status-message tick counter and requests redraw on expiration |
| 0x500060 | IsTalkIgnored | medium; spelling low | Tests the ignore filters after temporary and buddy-name exemptions |
| 0x500c60 | AddTemporaryIgnore | medium; spelling low | Replaces the temporary name exemption used when buddy ignoring is enabled |
| 0x500e40 | RemoveTemporaryIgnore | medium; spelling low | Removes the temporary name exemption |
| 0x501400 | ClearIgnoreList | medium; spelling low | Frees every ignore filter and clears its vector entry; called by SetDefaultPreferences |
| 0x5076e0 | ShowPermanentIgnoreWarning | medium; spelling low | Appends the permanent-ignore warning to the default console when named permanent filters exist |
| 0x505f10 | AddTextBufferLine | medium; spelling low | Constructs a statement with optional addressee and appends it to the supplied text buffer |
| 0x5061f0 | AddConsoleText | medium; spelling low | Routes statements through console filters or broadcasts messages according to preferences |
| 0x5025c0 | GetTextBuffer | medium; spelling low | Looks up the console text buffer matching receive mode, addressee and channel |
| 0x504d70 | ResetTalk | medium; spelling low | Clears all talkboxes, status text and the default text buffer and resets talk order |
| 0x504f30 | InitTalk | medium; spelling low | Initializes ignore flags, resets talk and allocates the default text buffer |
| 0x507a00 | ExitTalk | medium; spelling low | Clears ignore/exemption lists and destroys the default text buffer |
| 0x507bc0 | CreateTextBuffer | medium; spelling low | Creates the matching concrete buffer and registers its filter; send mode is an integer |
| 0x503920 | TTalkBuffer::GetNext | medium; spelling low | Advances the non-null statement cursor and returns the advanced cursor |


- **Leading sysinc/Talk gap** (high). 0x4fd440-0x4fd59f is Talk.cpp:
  fourteen helpers use the same status message, ten talkboxes, console-change
  flag and ignore flags at 0x71dbd0-0x71ef6c as asserted Talk functions. None
  uses sysinc's clock storage at 0x71dbcc. 0x55a130 initializes that sysinc
  clock; the next four initializers, 0x55a140/160/180/1a0, construct Talk's
  filter vectors at 0x71ef70/90/b0/d0. No function in this gap goes to sysinc.cpp.
- **Header assignments** (high for ownership/layout, low for invented member
  names). 0x4fd5a0-0x4fd6df contains the inline buffer constructor, vtable
  accessors and TTalkLine::GetText, not standalone sysinc routines. The
  0x573738 vtable and its five derived vtables identify the slots. Ghidra
  missed most tiny accessors; names.tsv records them nevertheless. The
  addressee getter 0x506eb0, channel-ID getter 0x507060, write-enabled
  setter/getter 0x507260/270, private-channel predicate 0x5079f0, and
  always-throwing 0x5075c0 (Talk.h line 321) likewise belong to Talk.h.
  Identical true/false/null/-1 accessors folded into earlier objects stay
  inline, not additional C++ translation-unit functions.
- **Container.h** (high). 0x4fd6e0 is the initialized four-byte pointer
  vector constructor, with Container.h lines 355/356, space>0 and block>=0.
  Its existing vector<T> implementation is reused and its instantiation
  documented in Container.h. operator() folds with vector<int> at 0x452ac0.
  The static initializers pass (0,9,10,NULL) for the first three lists and
  (0,4,5,NULL) for ConsoleFilterList. No STL implementation is reconstructed.
- **Trailing Talk/TMyException region** (high). Talk's final catch at
  0x5081b5 ends at exclusive 0x508261. Through 0x50826f there is only INT3
  alignment. 0x508270 is VC7.1 std::string::find_last_of, already marked
  stl and owned by TMyException.cpp; 0x508350 is its existing STL substring
  constructor. Nothing additional is assigned to Talk.cpp or sysinc.cpp.
- **Ghidra function splits** (high). 0x502f70 is a branch target inside
  Remove(TTalkLine*) at 0x502f00; 0x5036c0 is a loop block of GetLine at
  0x503670. Neither has an entry prologue, independent stack frame, or callable
  signature. They have names.tsv labels but no duplicate reimplementation.
  ehinfo's scan can also mistake the following function's prologue for the
  frame of the four-byte getter at 0x507060 and the tail-call destructor at
  0x506920; disassembly establishes that both have no frame of their own.
- **Names and source types** (low for original spelling; high for tested
  layouts). TTalkBuffer, TTalkLine, TTalkFilter, the five derived class names,
  flag names and helpers not present in assertion strings are ours. Talk.h
  now owns the full types, the shared TALK_MODE enum and all Talk declarations.
  TTextBuffer/TCommunicationChannel/TCommunicationStatement views and all Talk
  declarations were removed from external.h. It includes Talk.h only for its
  unfinished ConsoleWindow declarations. TRGBColor stays in external.h;
  Talk.h forward-declares it for its pointer-only color interfaces.
- **One definition per entry** (high). CreateTextBuffer is the only definition
  of 0x507bc0, and TTalkBuffer::GetNext the only definition of 0x503920.
  Both compatibility shims and all Talk-to-TCommunicationChannel casts are
  removed. Every names.tsv address is unique; reviewed Talk rows are edited
  in place and now describe each function's behavior.
- **Factory's second parameter** (high). 0x507bc0 takes integer SendTalkMode,
  not bool. ReceiveRuleViolationChannel pushes 1 at 0x4115d6 and calls the
  factory at 0x4115dd. Its source now uses (6,1,NULL,ChannelID), preserving
  the binary request, including the fallback lookup and event 0x32. Equal
  modes and full integer send modes remain differentially tested.
- **Old channel-view correction** (high). First +0x10, Cursor +0x18 and the
  statement Name +0x14 exactly match TTalkBuffer::first/cursor and
  TTalkLine::Addressee; static assertions enforce them in Communication.cpp.
  The old view's comment calling slot +0x10 close was wrong: that slot is
  SetWriteEnabled(bool), not Close (slot +0x08). ReceiveCancelRuleViolation,
  ReceiveLockRuleViolation and ReceiveCloseChannel all pass false to +0x10;
  those calls now use SetWriteEnabled(false) without changing Closed.
  Removal remains virtual Remove(TTalkLine*) at +0x20, and event 0x31's
  Unknown40(1) becomes SetSelected(true), the same +0x40 slot. See the full
  caller/instruction audit and regression coverage in notes/Talk.md.
- **Source order** (high where lines exist). The compiler placed the free
  slot finder (line 300), speech/message insertion (352-474), permanent-ignore
  warning (1144-1149), routing (1179-1279), factory/deletion (1361-1383), and
  buffer/line constructors late in the binary. They are placed by source
  lines, not address. Helpers without line evidence stay with their binary
  neighbours; header COMDATs are in the header.
- **Exact oddities retained** (high). Remove(int)'s loop never increments i
  (0x503330-0x50333d), so only index zero can succeed on a nonempty list;
  nonzero indices run to the Search!=NULL assertion. GetNext returns the
  advanced cursor. Base/private/channel constructors leave Closed untouched;
  only the rule-violation constructors clear it. Zero/negative line limits
  are not normalized. Named-line insertion uses its time offset; insertion
  without an addressee discards the caller's offset. Ignore filters never
  match modes 9/10/11/12. IgnoreEnabled gates exceptions, not the final ignore
  list itself. HasUnreadConsoleText is an inferred, imperfect name: the
  actual predicate is selected AND IsPrivateChannel(), not selected AND
  locked; differential testing caught and corrected that distinction.
- **Allocation and destruction** (high for binary behavior, low for original
  source syntax). Talk uses nothrow allocations to preserve VC7's null-return
  new branches. A null filter allocation is stored as a null list entry; a
  null line allocation still reaches virtual Append. Unchecked string
  allocations remain unchecked. Objects allocated with new are deliberately
  released with free, as in the image. The buffer destructor is nonvirtual,
  resets to the base vtable and clears through the base destructor; its
  noexcept(false) is a modern-language adaptation so Clear exceptions can
  still reach the caller's TEND. Limits, padding and uninitialized fields
  are not repaired.
- **ABI and field types** (high for widths/offsets, low for original source
  declarations). Colors use an explicit output pointer and return it, which
  reproduces the hidden-return-buffer x86 ABI without guessing the original
  struct-by-value syntax. TTalkLine::Time is a signed 32-bit time_t, not
  modern time_t. Flags at +0x24..+0x29 have inferred semantic names; their
  byte values, vtable access and six class sizes are checked. Write's original
  catch calls its FILE* stack slot bytes; an INT_PTR alias retains that name.
- **Differential harness** (high). The Preferences test's old Talk-reset
  counter is replaced with the real implementation on both sides; its
  original ignore vector is initialized before SetDefaultPreferences.
  PatchImage is reused; Talk additionally redirects 32-bit time/localtime,
  sprintf and fprintf. The test executable's own _time32 is temporarily
  redirected to the same fixed clock, restored afterwards, to avoid
  second-boundary races. Only explicitly invoked mapped functions execute;
  the original executable's loader, imports and entry point never run.
  Error/exception and allocation-failure paths are not invoked in the image.
- **Metadata correction** (high). The old automatic names extraction
  mistook the 0x502f70 explanatory comment for a function and appended
  BuddyIgnoreList. The erroneous duplicate is removed; the single remaining
  TTalkBuffer_Remove_line_block row identifies an internal block of 0x502f00,
  not a function or global. No separate source definition is added.

See notes/Talk.md for exception-table checks, switch tables, all remaining
callcheck differences, caller integration and differential coverage.

## Objects.cpp

- **The whole Objects/Player gap (0x4d7cd0-0x4d913e) is Player.obj** (high).
  It starts at 0x4d7cd0 (not found by Ghidra), `mov eax, 0x56dde4; ret`:
  the GetMessageMap of BEGIN_MESSAGE_MAP, slot 10 of the vtable 0x56de08
  whose overrides 0x4d94c0, 0x4da3e0 and 0x4dae60 are Player.cpp functions.
  The application object it belongs to is at 0x719dc0, among Player.cpp's
  data (0x719db0-0x719e6c), and its static initialiser (0x55a030) comes
  right after Network.cpp's (0x55a010). Everything after it is either the
  class's constructor (0x4d7cf0), compiler-generated destructors, the MFC
  inline AfxGetMainWnd, or <fstream>/<string> code (filebuf, codecvt,
  getline, string::compare) called only from Player.cpp (0x4d9700,
  0x4d9bd0, 0x4da140); FatalError (0x4d7d80) is called only from Player.cpp.
  Nothing in the gap belongs to Objects.cpp. 0x4d9110 is TMyException.h's
  GetOrigin, kept in Player.obj. The STL/MFC functions got names.tsv rows of
  kind stl; the three Player.cpp functions (0x4d7cd0, 0x4d7cf0, 0x4d7d80) got
  game rows and are left for Player.cpp.
- **Application class name CPlayerApp, FatalError** (low). MFC's wizard names
  the class after the project; the project folder is c:\mud\tibiaplayer and
  the file Player.cpp. FatalError is ours: it shuts the connection down,
  leaves fullscreen, hides the main window, shows a message box and calls
  exit(1), each step in its own try/catch(...).
- **NoObject at 0x719da0 is Objects.cpp's** (low). A 12-byte TObject in .bss
  that is never written and is read by Communication, Inventory and Map as
  the empty object. It lies between Objects.cpp's last global (0x719d9c) and
  Player.cpp's first (0x719db0); the object system is the likelier owner.
  The name is ours.
- **Container.h array<T>** (class name low, members high). The four tables
  are 12-byte objects {min, d, entry} with a constructor at line 152
  ("d>0") and operator() at line 178 ("index out of range (original i: ...,
  i: ..., min: ..., d: ...)"); "ObjectProperties->low()" and "->high()"
  name the bounds. Written as operator() returning T&, like vector<T>
  (Container.h already has it so). The destructor `delete[] entry` is
  inferred from ReleaseObjects, which frees entry right before the array.
  Put before vector<T> in Container.h, as in the original line order.
- **TProperties and field names** (low for the struct name, high for Width,
  Height, Layers, PatternWidth, PatternHeight, PatternDepth, Phases and
  Flags, which are in the assertions). ExactSize, Sprites, Waypoints,
  MaxLength, Brightness, LightColor, ShiftX/Y, Elevation, AutomapColor and
  LensHelp are ours; Waypoints and MaxLength follow the game server.
- **Flag names** (medium for 1-20, low for 22, 23, 26, 27). BANK,
  CUMULATIVE, LIQUIDCONTAINER, LIQUIDPOOL, HANG, LIGHT, SHIFT, HEIGHT,
  AUTOMAP and LENSHELP are in the assertions; their bit numbers come from
  the loader's switch and the accessors. The others follow the game server
  and the usual 7.x .dat flag order. CREATURE (99) is from MapWindow.cpp.
- **Function names** (medium): GetObject*/GetOutfit*/GetEffect*/GetMissile*
  after GetObjectFlag (in the assertions); the tables follow the loader's
  order (objects from 100, outfits, effects, missiles), checked by which
  global each accessor passes in ECX. GetEntry* are static: only this file
  calls them. ReleaseObjects (low), LoadObjects, InitObjects, ExitObjects and
  ReloadObjects (from their callers in Control.cpp: InitControl,
  ExitControl, event 0x45 after patching Tibia.dat).
- **Parameter passing** (low). Entry is a reference ("Entry.Flags"); the
  pattern functions take `TObject &Obj` (the callers pass an address, and
  "GetObjectFlag(Obj,CUMULATIVE)" needs an overload for objects, written as
  an inline in Objects.h); LoadProperties takes pointers. The outfit
  sprite's 5th parameter is called Direction (PatternX = Direction + 1).
- **Obj.Data2 holds the hook flag** (medium): GetHangPattern compares the
  third TObject field with 18 and 19 (HOOKSOUTH, HOOKEAST).
- **Source order**: line order. LoadProperties (line 939) is placed by the
  compiler after GetEntrySpriteNumber, and ExitObjects (1023) before
  ReloadObjects (1012). The line gaps 70-91 and 281-303 fit one more accessor
  each (probably of MaxLength, the only field without one); they are not in
  the binary (removed by the linker) and not reconstructed.
- **TReadBinaryFile::readWord returns a 16-bit type in the original** (high
  for the observation). Every caller in Objects.cpp zero-extends AX
  (`movzx`), so the original returns uint16; Script.h declares `int`. The
  value is the same (at most 0xffff). Resolved at the merge: Script.h's
  readWord now returns uint16.
- **InitObjects' catch (TMyException &)**: the binary's handler has a catch
  object, so the original named the reference without using it; it is
  unnamed here because modern MSVC warns (C4101). PROGRAM_ERROR 1 has no
  enumerator (name unknown); common.h is unchanged.
- **TObject stays in external.h** (Status.h uses it); Objects.h includes
  external.h for it.

## Preferences/Script gap (Tibia.cfg loader and saver)

- **0x4ddc00-0x4df53f is Preferences.cpp** (high): the loader, saver and
  0x4df520 use only Preferences data (0x719e70-0x71c44f and 0x5c2558), plus
  the flag 0x71c319 that only they touch; Script.cpp has no globals. They
  follow SetSkillBar (line 850) in the binary and have no line evidence, so
  they end the file in binary order.
- **Names InitPreferences, SavePreferences, ExitPreferences, PreferencesLoaded**
  (medium): InitControl calls the loader where the other modules are
  initialised, ExitControl calls 0x4df520 where the others exit, and the
  Init/Exit pairs of the other files (InitObjects/ExitObjects) are named so.
  The flag is set after loading whether or not that failed, and the saver
  writes only when it is set: "loaded" rather than "save requested".
- **TWriteScriptFile's destructor** (high, corrects the Script.cpp entry):
  the saver calls close() (0x4df660) and then 0x4df6b0, so the destructor is
  only `if (File) fclose(File); File = NULL;`, folded with the binary files'
  close(), not close() itself. Script.h fixed.
- **No yes/no helper**: the comparison with "yes" and the "yes"/"no" choice
  are written out at every use. The binary's code differs per site (the
  buddy's Notify keeps the full strcmp result), which a shared helper would
  not produce.
- **The OpenDialogs duplicate check reads the vector's fields** (`min`,
  `max`, `entry[i]`) directly: the binary loops from min to max-1 and indexes
  entry without the start offset, which operator() would subtract. Written
  that way.
- **Buffer sizes**: the loader's identifier copy is 20 bytes (CopyString
  limit 0x14); the saver's hotkey buffer is written as HOTKEY_LENGTH+1 (the
  `_snprintf` limit 0x101; the stack slot is 264 bytes with padding).

## Player.cpp

- **Member functions of CPlayerApp** (medium): FatalError, HandleProgramError,
  HandleException and ReadErrorFile are written as members. HandleProgramError
  and HandleException read `this` (+0xa0, +0x1c); FatalError gets `this` in
  ECX from both callers but does not use it; ReadErrorFile is called with
  ECX still holding `this` (low: it could be a free function).
- **Names** (low unless noted): Terminating (+0xa0) and RunPatch (+0xa1, Run
  starts patch.exe; set elsewhere through the application pointer),
  GetWindowTitle/GetWindowClassName (the decoded "Tibia   " and
  "TibiaClient"), CountTibiaWindows (the EnumWindows callback at 0x4d9420,
  missed by Ghidra), NumberOfTibiaWindows, LastMousePosition,
  LastMouseCursor. OutputControlErrorInformations (0x445900) and
  CMainFrame::OutputErrorInformations (0x4a8360) are named after Player.cpp's
  catch texts (high).
- **The XOR code tables at 0x5733a4/0x5733c4** are in another object's
  .rdata (owner unknown; the key tables at 0x56dd98/0x56ddb8 are Player's,
  next to its vtable). Both are written as static tables in Player.cpp.
- **The critical-error text pointer 0x5c249c** is part of a table of UI
  texts (0x5c2480-0x5c2517) whose owner is unknown; declared in external.h
  as DAT_005c249c.
- **Source order**: no line evidence for the message map, constructor,
  FatalError, CountTibiaWindows, the error handlers and ReadErrorFile; they
  are in binary order around the decoders (62, 75); InitInstance (255-347),
  Run (383-408) and OnIdle (456) end the file in line order (the compiler put
  OnIdle at 0x4d94c0).
- **The top-level handlers of InitInstance and Run** are written out (not a
  macro): PROGRAM_ERROR shows its code, a TMyException is wrapped ("exception
  occurred"), std::bad_alloc, std::out_of_range, exception, const char* and
  ... become TERRORs, and an outer catch of TMyException writes error.txt.
  Their catch types are from the exception tables.
- **std::exception** stands for VC7.1's global `exception` (the binary's type
  name is .?AVexception@@).
- **Odd behaviour kept**: "host" and "port" on the command line are parsed and
  dropped; InitInstance returns TRUE after an error; the 30-character
  service pack buffer may lack its terminator; error.txt's last dashes have
  no line break; HandleProgramError/HandleException set Terminating only when
  it is already set.
- **Tests**: only the two decoders are pure. Player.cpp is linked into the
  difftest against stubs for MFC and its callees (playertest.cpp).

## ContextMenus/Control gap (0x445900-0x445aeb)

- **Both functions are Control.cpp** (high): 0x445900 prints Control.cpp's
  LastPacketTypes ring (0x5c4510/0x5c4514) and is named by Player.cpp's
  catch text "OutputControlErrorInformations"; the catch blocks of 0x4459b0
  start Control.cpp's assigned range (0x445aec) and name line 93 with
  Width and Height.
- **Resize** (low): our name; MainFrm.cpp calls it with the new client size
  (and with 0,0 after switching to DirectX 9 fullscreen).
- Not differentially tested: OutputControlErrorInformations writes through
  the original's iostream code, and Resize needs the window objects.

## start/Communication gap (0x401000-0x40434f)

- **0x401000 is Communication.cpp's InitCommunication** (high for the file,
  low for the name): it clears PatchData (0x5c4344), which only
  Communication.cpp uses, and InitControl calls it among the other Init
  functions. It is the first function of the image, before the STL code.
- **0x401010-0x404341 is STL** (high): string, stringstream, streambuf,
  locale, ctype, codecvt and num_put code with no game data or game calls
  (checked by scanning every function for game globals, game callees and
  strings); Communication.obj, the first object in link order, is the first
  to instantiate it. Rows of kind stl with generic names std_<address>
  (low): naming each template member was not needed for the reconstruction.

## Status/SurfaceDX gap: Surface.cpp (0x4eaa80-0x4eae6f) and SurfaceDX.cpp

- **A file Surface.cpp** (name low, split high): 0x4eaa80-0x4eae6f has its
  own data (0x71d0c0-0x71d0cc: the main window, curEngine, the available
  engines, the surface object), defines TSurface (vtable 0x56ef78, mostly
  _purecall slots) and creates the engine surfaces; "Surface" sorts between
  Status and SurfaceDX in the link order. Issue #9 revises the data split:
  engine-shared data extends from 0x71d0bc through 0x71d0f4, while private
  SurfaceDX data starts at 0x71d0f8 (see SurfaceDX.cpp below).
- **0x4eae70-0x4ed19f is SurfaceDX.cpp** (high): TSurfaceDX's vtable
  (0x56f038) follows TSurface's, its destructor 0x4eae90 sets it and calls
  ~TSurface, and the DirectX 5 factory 0x4eca00 is what GetSurface calls for
  engine 0. Recorded in file-ranges.tsv; issue #9 reconstructs this object.
- **TSurface moved from external.h to Surface.h** with the binary's shape:
  pure slots where the vtable has _purecall, empty default bodies for +0x04,
  +0x08, +0x64, +0x80, +0x98, a forwarder at +0x50 (to +0x54), TestEngine
  (+0xbc) storing the main window. Slot parameter lists are known only from
  the default bodies' RET n (int placeholders). The data members are offsets
  with placeholder names except Window (+0x08). TSurfaceGL is declared in
  external.h (constructor 0x4fb670, 0x498 bytes) because GetSurface creates
  it with new.
- **Names** (low): GetCurrentEngine, GetSurface, GetAvailableEngines,
  SurfaceInitInstance (after the binary's SurfaceExitInstance),
  SurfaceWindow, AvailableEngines.
- **The removed try block in SurfaceInitInstance**: the unwind map has two
  states around ChoosePixelFormat/DescribePixelFormat without a try-block
  entry; the compiler drops handlers that only guard C functions. The catch
  body is unknown; it is written like the first one (PROGRAM_ERROR 12), low.
- **0x5c2564** (the "engine N" value) stays DAT_005c2564 in external.h: it is
  in .data after Status.cpp's 0x5c2560, written by Player.cpp and read here;
  its owner is not clear.

## sysinc.cpp and the SurfaceGL/sysinc gap

- **The whole gap 0x4fcb90-0x4fce4f is sysinc.cpp** (high): the functions use
  only sysinc.cpp's data (the clipboard buffer 0x71dbc4/0x71dbc8 that the
  asserted functions grow and fill, the start time 0x71dbcc with its static
  initialiser 0x55a130). 0x4fcd50-0x4fce4f is ATL header code (the code
  page lookup _AtlGetConversionACP with its InterlockedExchange init, AtlThrow
  for E_OUTOFMEMORY, a block-chain free) kept in sysinc.obj: rows of kind
  stl, generic names (low).
- **Names** (low unless noted): FileExists, IsClipboardEmpty, GetErrorText,
  GetErrorHint (medium), InitSysinc, ExitSysinc, ResizeClipboardBuffer,
  GetClipboardText, ClipboardBuffer, ClipboardBufferSize, StartTime. textdata
  and help are from the assertions; textdata is the locked clipboard memory,
  so the parameter of SetClipboardText is Text (the earlier external.h
  declaration had called it textdata).
- **SetClipboardText returns bool** (high: it returns 1/0); the earlier
  declaration was void.
- **0x4fd02c is not a function**: Ghidra split GetClipboardText at the branch
  after OpenClipboard.
- **Odd behaviour kept**: the clipboard buffer comes from new and is
  released with free; removing a CR moves one byte past the data.
- **Tests**: GetErrorText/GetErrorHint run in a separate sysinctest.exe,
  because difftest.exe gives Status.cpp recorders for GetMilliseconds,
  RemoveFile and SetClipboardText. The rest needs the clipboard or the
  original's CRT (srand).

## Help.cpp and the gaps on both sides

- **0x4a2a00-0x4a2b88 is Help.cpp** (high): the functions use only Help.cpp's
  data: 0x5ce520-0x5ce538 next to the EventHelps vector 0x5ce53c (static
  initialiser 0x559e80, right after GUI.cpp's at 0x559e60; GUI.cpp's data ends
  at 0x5ce51c) and the asserted AddEventHelp uses the same count. 0x4a29e0 is
  a compiler-generated deleting destructor of a GUI.cpp class (stl row).
- **The gap after Help.cpp**: 0x4a4440 (GetCurrentHelpText) is Help.cpp's;
  0x4a4460-0x4a4546 use Inventory.cpp's data and are recorded as
  Inventory.cpp, not reconstructed yet.
- **Names** (medium/low): the help-area and handle functions, GetEventHelp,
  PopEventHelp, AddEventHelp (the "hints" are Preferences' KnownHints),
  CheckEventHelpName, CheckEventHelpPosition, GetEventHelpTitle/Text,
  GetCurrentHelpText, the data names. EVENTHELPS = 14 from the assertions.
- **CheckEventHelpPosition** is written as independent tests with the
  binary's hex bounds; the binary merged tests with a common x and returns
  early where no later test can match. Verified on every bound and its
  neighbours on floors 4-9 (difftest).
- **GetHelpText** is one switch; the binary splits it into >1000, ==1000 and
  the 10-23 table. Texts were copied from the decompiled strings and are
  compared byte for byte.
- **The vector<int> destructor** (atexit 0x55a2b0 frees entry) is not in
  Container.h yet; behaviour at process exit only.

## Inventory.cpp and the Inventory/MainFrm gap

- **Struct and field names** (low unless from the assertions): TContainer
  (IsOpen and Count are in the assertions), Object, Name, Capacity,
  HasParent, Objects; TOffer (OwnOffer.Count and CounterOffer.Count are in
  the assertions) with Name and Objects. CONTAINER_OBJECTS = 36 is our name
  (the binary's limit 0x24); BODY_POSITIONS, OPEN_CONTAINERS and
  OBJECTS_PER_OFFER are from the assertions. Layouts from the offsets
  (container 0x1ec bytes; offer 0x4d4, CounterOffer is 0x4d8 after OwnOffer).
- **Function names** (medium/low): ours, after their callers in
  Communication.cpp; GetContainerObject is the container itself,
  GetContainerContent an object in it.
- **The gap after Help.cpp (0x4a4460-0x4a4546) is Inventory.cpp** (high):
  only Inventory data. Placed first, in binary order.
- **The Inventory/MainFrm gap (0x4a8210-0x4a8646) is MainFrm.cpp** (high):
  CMainFrame members (the key translation reads the frame at +0x200; the
  constructor and OutputErrorInformations are called on the main window by
  Player.cpp). Not reconstructed yet.
- **Callers** now pass TObject (Type, Data1, Data2) where Communication.cpp
  built TCommunicationObject literals for Inventory functions; the layout
  is the same.

## MainFrm.cpp and the MainFrm/MainWindow gap

- **The message map** is decoded from the AFX_MSGMAP_ENTRY table at
  0x56c030 (24 entries, in the binary's order). OnKeyDown/OnSysKeyDown and
  OnKeyUp/OnSysKeyUp share one address each (identical tail calls to
  KeyDown/KeyUp, folded). WM_DISPLAYCHANGE and WM_USER+1 are ON_MESSAGE
  entries; WM_USER+1 is the WSAAsyncGetHostByName reply (OnHostResolved,
  our name).
- **Member layout** (offsets from the reads): +0xd0 the mouse cursor at
  construction, +0xd4 a WINDOWPLACEMENT (0x5239d8/0x5239ef are
  CWnd::Get/SetWindowPlacement; its rcNormalPosition is the +0xf0 rectangle
  OnDestroy saves), +0x100 64 ints cleared and never used, +0x200 the held
  modifiers (KeyFlags). Names ours.
- **PacketHeader (0x5d1644) is MainFrm.cpp's** (medium): the constructor
  clears it and OutputErrorInformations prints it; Network.cpp writes it.
  The other data 0x5d1654-0x5d169c (the frame pointer, cursors, button
  flags, the class name buffer, the wheel remainder) are MainFrm's too.
- **Free functions** ExitProgram, CloseProgram, SetDisplayMode, OpenURL and
  UpdateMouseCursor (names low; OpenURL and SetDisplayMode after
  Control.cpp's callback parameter names) use the global MainFrame and are
  passed as callbacks to InitWindow and the main window.
- **Eliminated try blocks**: ExitProgram, AddFrameSize and KeyUp have the
  unwind states of a TBEGIN/TEND but no try entry; written with TBEGIN/TEND.
- **Control.cpp's RightButtonDown** now takes (x, y, Flags) like the other
  button functions: MainFrm passes them; the reconstruction had declared it
  without parameters.
- **OnMouseWheel's catch** names point.x/point.y as in the binary.
- **The gap after MainFrm.cpp (0x4ac010-0x4ac20d) is MainWindow.cpp**
  (high): a window class destructor and Draw plus 0x4ac090, which deletes
  MainWindow and ModalWindow.
- **Tests**: TranslateKey is compared for all keys, flags and modifiers; the
  rest needs MFC and the window system. MainFrm.cpp is linked into the
  difftest against stubs.

## Order of the GUI files

- **GUI.cpp before MainWindow.cpp** (judgement): the backlog says smallest
  first, but MainWindow.cpp and the other window files derive from GUI.cpp's
  TWindow and its subclasses, whose layouts and vtables are only partly known
  in external.h. Reconstructing the base first avoids writing the derived
  classes against guessed layouts.

## Creatures.cpp

Reconstructed by Claude Opus 5.5, November 1, 2026.

- **Leading gap 0x44b740-0x44bde0 is Creatures.cpp** (high). Every function
  uses Creatures data (VIP list 0x5c4560-0x5c67d7, player data
  0x5c67d8-0x5c68a8, creature table 0x5c68b0-0x5cc417) or a TCreature member
  offset, and nothing of Control.cpp (its last global is 0x5c4554). No code
  outside 0x44b740-0x4524df touches 0x5c4558-0x5cc417, so every global is
  static. 0x44bc70 is the line-142 function; its PROGRAM_ERROR catch 0x44bdcd
  was the last gap entry. Nothing in either gap goes to Control.cpp.
- **Trailing gap 0x4524e0-0x45399a is DialogElements.obj** (high for the
  object, low for the split between DialogElements.cpp and a header). Its
  functions use vtables 0x560bb8-0x5611e0 and are called from DialogElements.cpp
  (0x45af40-0x46b950) and later objects, never from Creatures.cpp; 0x453930
  has DialogElements.cpp line-42 catch blocks. The no-EH functions at the
  start of each object (as in Creatures.obj) suggest these are DialogElements.cpp's
  own functions; they are left unnamed for that file's reconstruction, except
  the Container.h instances: vector<int>::operator() 0x452ac0 and the
  vector<int>(..., init) constructor 0x453300 (named before), the vector<int>
  and vector<TDialogText> destructors 0x452c50/0x452c60, and
  vector<TDialogText> 0x452eb0/0x453600 (0x18-byte element: a pointer, then
  4 and the colour at 0x5c1230 and 0; element constructor 0x452c70, not found
  by Ghidra). The element type name is ours (low). The template's destructor
  (`delete[] entry`) is documented in Container.h but not added: adding it
  changes the exit behaviour of Status/Preferences/Talk statics (correct per
  the binary's atexit handlers, but outside this file); open question.
- **Static initialisers** (low, not used as evidence): 0x559df0 constructs an
  8-byte (0,0) object at 0x5c4558, just before Creatures data, and
  0x559e00-0x559e20 three at 0x5cc418-0x5cc42f just after it (0x5cc420 is used
  by Dialogs.cpp). The same constructor (0x496130) builds such objects in
  several objects; which object owns 0x5c4558 is not settled.
- **Source order** (medium for the rule, low for the gap functions' place).
  The binary order is the line order except that a function calling a
  function defined further down comes after the last of them: lines 278-592,
  InitCreatures (1366, calls ResetCreatures at 1414) and SetPosition (108,
  calls CheckMonsterHint at 573) follow ResetCreatures. The leaf functions
  without exception frame (0x44b740-0x44bc6f) precede line 142, so they are
  in the source before it; they keep the binary order, with SetPosition
  (line 108) put before the three TCreature accessors that come last
  (GetDeltaX, GetDeltaY, GetDrawDirection). IsPlayerInParty reads the
  shield itself instead of calling GetPlayerPartyFlag, which comes after it.
- **Names from the binary** (high): GetCreature, GetPlayerID,
  GetPlayerPartyFlag (assertions in ContextMenus.cpp/ConsoleWindow.cpp),
  CreatureID (the member catch blocks read `*this`), NumberOfPlayerSteps,
  CurrentPlayerStep, NumberOfBuddies, BUDDIES, SKILLS, Opponent and the
  parameter names. All other function, class (TCreature, TBuddyEntry),
  member and global names are ours (medium where the behaviour is clear,
  low for IsPlayer* state bits, HighlightID, UpdateSkulls, NotifySkull,
  HasPlayerSkull, IsPlayerInParty, GetBuddyListNew, CheckMonsterHint and the
  path flags Exact/Diagonal, whose meaning in the path finder is unclear).
  TCreature keeps the old view's field names (RealX..., DeltaX, Colors...).
  The VIP list accessors by position are GetBuddyList* because Preferences.h
  already has GetBuddyIcon/GetBuddyDescription/GetBuddyNotify by character;
  0x44bb80 is GetBuddyPosition because Preferences.h has FindBuddy(int) (an
  overload named FindBuddy was silently chosen over it by EditBuddy; found
  by the difftest). In names.tsv members are TCreature_Method.
- **Help and map callees keep FUN_ names** (high): FUN_004a32e0 (hint
  numbers 7, 8, 9, 10, 13, 14), FUN_004a3460 (monster name), and the Map.cpp
  functions FUN_004b08c0, FUN_004b08f0, FUN_004b0960, FUN_004b6140,
  FUN_004be130, FUN_004bdf30 and the path finder FUN_004bee00 (new
  declarations in external.h under "added for Creatures.cpp"; the Map.cpp
  agent may rename them).
- **Dropped try blocks** (medium): AddCreature (16 EH states, no try block)
  and UpdateSkulls (11 states, none) are written with TBEGIN/TEND: their
  states match an assertion plus a TEND, and the compiler removes a try block
  whose body cannot throw. Their catch lines and names are unknown; written
  as plain TEND.
- **"[bug0000038]" / "[bug0000289]"** in two assertion texts: written with a
  local TASSERT1_BUG/TASSERT2_BUG macro in Creatures.cpp that appends the
  bug number to the stringized expression (common.h unchanged).
- **Odd behaviour kept** (high, from the instructions): AddCreature
  overwrites slot 149 when no slot matches and keeps a reused slot's name,
  position and battle list links; AddBuddy does not decrement the online
  count when an online entry is updated as offline, so GetNumberOfBuddies
  (online count with offline buddies hidden) can exceed the list length;
  SetPlayerDestination adds the unconfirmed step twice in its second
  comparison; HasPlayerSkull, IsPlayerInParty and GetPlayerPartyFlag do not
  check for a missing player creature; SetPlayerSkill does not check
  SkillNumber; SortBuddy moves an entry past equal ones; CheckMonsterHint
  has no z check.
- **Types** (medium): CreatureID and CharacterID are uint32 where the binary
  compares unsigned (0x40000000); SkullTime is `__time32_t` and the clock
  `_time32` (VC7.1's 32-bit time_t, as in Talk.cpp); GetHealthColor returns
  TRGBColor by value (hidden pointer, `ret 4`). The IsPlayer* state bits are
  declared bool (the binary returns `(State >> n) & 1` in EAX).
- **Switches checked against the jump tables** (high): GetSkillName
  (0x44d6a8, cases 0-6), DoPlayerStep (0x4510c8, step directions 1-8),
  FindPlayerPath (0x4513f0, results -4..-1); CompareBuddies' sort-method
  switch is a sub/dec chain.
- **Call sites fixed** (high): Communication.cpp's ReadCreature called
  0x452320 as a free function; it is TCreature::SetPosition on the creature
  (`mov ecx,esi` at 0x40cb9a). The TCommunicationCreature view's offsets all
  match TCreature (static_asserts now on TCreature); its comments were
  right. Control.cpp passes HideCreature to FUN_004bf620.
- **Difftest** (high): creaturestest.cpp compares every function of the
  file on random operation sequences, with the creature table and VIP list
  compared byte for byte, against fakes for the map, path finder and help
  functions on both sides; assertion-failure paths are not exercised. A test
  that indexes the VIP list must use the full length, see the online-count
  behaviour above.
## Map.cpp

Reconstructed by Claude Opus 5.5 (delegate), November 1, 2026.

- **MainWindow/Map gap (0x4b08a0-0x4b0b10) is Map.cpp** (high). The nine
  functions use only Map.cpp data: the centre position 0x5d16e8-0x5d16f0
  (also written by ShiftMap and InitMap), the automap flag 0x5d16cc (set by
  AddAutomapMark, UpdateAutomap, InitMap), the light 0x5d16c4/c8 and
  0x7152d0-0x7152e4; line 1784 asserts AmbienteActualBrightness (0x5d16c4).
  Map.cpp's data is 0x5d16c0-0x7152e7: its static initialisers 0x559f40,
  0x559f60, 0x559f90 construct 0x5d4c18, 0x5d4c24 and 0x5d4c40, and the next
  one, 0x559fc0, constructs 0x7152e8 (MapWindow). 0x5d16b4 (an 8-byte object
  initialised by 0x559f30 and never used) lies before Map's first variable
  0x5d16c0 and is left to MainWindow.cpp (low). 0x5d16f4 is unused.
  Ghidra missed four functions of the object: 0x4b0880 (TPathNode's inline
  constructor, called only by matrix<TPathNode>'s constructor; it starts the
  object, before the gap), 0x4b0b30 (TAutomapBlock's destructor, from the
  atexit iterator 0x55a2e0), 0x4b0b40 (TMapPoint's constructor, passed to the
  eh vector constructor iterator by array<TMapPoint>) and 0x4b1410
  (TAutomapBlock's constructor, initialiser 0x559f90). 0x4b0b20 is
  matrix<TPathNode>'s destructor (Container.h), called only by FindPath's
  unwind funclet. The gap functions have no line evidence and open the file
  in binary order.
- **Map/MapWindow gap (0x4bf860-0x4bfb0b) is MapWindow.cpp** (high): the
  destructor, scalar deleting destructor and constructor of the class with
  vtable 0x56cf38, whose constructor 0x4bfa00 owns the catch blocks of
  MapWindow.cpp line 130; 0x4bf8d0/0x4bf8e0 (missed by Ghidra) are its vtable
  slots +0x3c/+0x40 returning 240 and 176; 0x4bf8f0 and 0x4bf930 work on
  member arrays of 18x14 fields. Only names.tsv rows (the class name
  TMapWindow and the member names are ours, low).
- **Container.h matrix<T> and matrix3d<T>** (high for the code, low for the
  class names). Lines 217-253 and 282-328, between array (152-178) and
  vector (353 on); xmin, ymin, zmin, dx, dy, dz, x, y, z are in the
  assertions and messages. matrix3d's error has "[bug0000013]" as its
  expression, matrix's has none. Both have destructors: matrix's out-of-line
  copy is 0x4b0b20, and the atexit thunks 0x55a2c0/0x55a2d0 free the entries
  of the static array and matrix3d.
- **vector<T> gets its destructor** (high): delete[] entry, inlined in the
  atexit thunks of all static vectors (Talk's 0x55a390-0x55a3c0, those of
  Preferences and Status at 0x55a320-0x55a380) and in TAutomapBlock's
  destructor 0x4b0b30, which frees Marks.entry (+0x200a0 = +0x20018 + 0x88).
  Container.h had no destructor; the earlier static vectors now free their
  entries at exit like the original.
- **TObject's default constructor clears it** (medium). TMapPoint's
  constructor 0x4b0b40 zeroes exactly the ten TObjects and nothing else,
  which is what a compiler-generated constructor does when TObject has a
  constructor that clears its fields. Written as default member initializers
  in external.h, so TObject stays an aggregate (brace initialisation still
  works). There is no static initialiser for the TObject globals (NoObject,
  the objects of Status.cpp): the original compiler apparently dropped
  constructors that only store zeros into .bss (low for that explanation).
- **TRGBColor constructors** (medium for TRGBColor(int Color), low for the
  others). 0x425950 (between ConsoleWindow.cpp and ContentWindows.cpp) takes
  ECX and a palette index, stores R, B, G and returns this: a constructor.
  GetAutomapColor calls it on its return slot; SetAmbientLight has it inlined
  (same store order). Defined inline in external.h's TRGBColor (the header
  it belongs to is not reconstructed); a default constructor and a
  three-value constructor (GetAutomapColor's black, and Control.cpp's
  "= {200, 200, 200}") were added so that the type stays usable as before.
- **Colours by value** (medium). GetAmbientColor and GetAutomapColor return
  TRGBColor by value (hidden return pointer, the same ABI as Talk's explicit
  output pointer); the constructor call on the return slot shows a returned
  temporary.
- **Callers switched from TCommunicationObject to TObject** (high): the
  layout is the same (Type, Data, Extra = Type, Data1, Data2, 12 bytes).
  Communication.cpp's map code (ReadField, ReceiveAddObject,
  ReceiveChangeObject, ReceiveDeleteObject, ReceiveMoveCreature) now uses
  TObject; TCommunicationObject remains only for the Inventory.cpp calls, so
  its external.h comment (shared with TCommunicationCreature) was left as it
  is.
- **Names** (low unless noted). From the binary: MAPSIZE_X/Y/Z,
  OBJECTS_PER_POINT, EFFECTS, Point, Effect(s), Block, Coordinate,
  EffectNumber, EffectIteratorField/Count, AmbienteActualBrightness, the
  fields ObjectCount, EffectNumber, Status, Type, Next, Previous, bx, by, bz,
  x, y, and FindPath with its parameters (StartX ... ForceDiagonal, from its
  error message; high). AmbienteBrightness, AmbienteColor and
  AmbienteActualColor follow the binary's "Ambiente" spelling. Ours:
  TMapPoint (the assertions say "Point"), TEffect, TMark, TCoordinate,
  TAutomapBlock, TPathNode, the globals Fields and FieldNumber, and all
  function names. GetObject is called GetMapObject because windows.h defines
  GetObject as a macro. The four GetTop*Object names guess the purpose from
  the flags they test (UNMOVE: move; FORCEUSE, LIQUIDPOOL: use); MapWindow.cpp
  will tell.
- **Static functions**: the functions without callers outside Map.obj
  (SortObjects, InsertObject, GetEffectIteratorNumber, NewEffect,
  DeleteEffect, the six ShiftMap* helpers, GetFieldAutomapInfo, the automap
  file and cache functions, IsAutomapBlocked, GetAutomapSpeed) are static,
  as Objects.cpp's GetEntry* are.
- **Parameter forms** (medium): the GetTop*Object functions take int
  &Position (the catch prints "Position" with the value read through the
  pointer); AddAutomapMark takes TCoordinate &Coordinate ("Coordinate.x");
  DeleteAutomapMark reads only x and y of its first parameter (the callers
  pass an x/y pair and the level separately; written as TCoordinate &, low);
  GetAutomapMarks takes vector<TMark> * and int * (low).
- **Assertions with bug numbers**: expression texts such as
  "In(rx,0,MAPSIZE_X-1) [bug0000013]" are written with local TASSERTB macros
  that append the tag to the stringised expression. All assertion texts of
  the file were compared with the binary's strings.
- **Try blocks from the exception tables**: InsertObject's try starts after
  its five assertions (states 25-25), although the point lookup precedes it;
  SaveAutomapBlock's starts before its early return and its assertions
  (states 0-38); the others follow the usual pattern. LoadAutomapBlock and
  SaveAutomapBlock have inner try blocks whose catch (TMyException &)
  handlers wrap the error ("failed to write colors" in both, sic, "failed to
  load flags", "failed to write flag count", "failed to write flag" with i)
  and an outer one that reports a damaged file. FindPath has a single try
  with only a TMyException & handler (no TEND trio).
- **Locals that the catch blocks print**: GetAutomapBlock keeps Position in
  bx's stack slot (written with Position declared before TBEGIN); ClearField
  sets i to -1 after the object loop (kept); ProcessEffects,
  ProcessMissileEffects, ExitMap and DeleteAutomapMark declare i before
  TBEGIN so that TEND1(i) can print it (the binary's slot is uninitialised
  before the loop, ours is 0).
- **Exact oddities kept**: AddMissileEffect's duration is 150 ms times
  (int)sqrt(sqrt(dx*dx + dy*dy)) (two fsqrt; dx*dx in double, dy*dy in int);
  only missiles on the centre's level are added; effect, text and missile
  data overlap (union); DeleteObject returns true when 9 objects remain;
  AddObject silently drops an object on a full point while AppendObject
  raises an error; ShiftMapLevel clears levels 5-7 after moving up; an
  automap file without marks is accepted (eof); GetAutomapMarks takes block
  coordinates in parameters named x and y; FindPath's error message has no
  opening parenthesis, and its open-list removal assumes the node is present
  (a missing node is cut after the last one; an empty list would crash);
  IsFieldPassable treats piles of more than two HEIGHT objects as passable.
- **Source order**: line order, the gap functions first (binary order);
  GetNextObject between lines 171 and 197 and ClearMap between 913 and 945,
  as in the binary. GetAmbientBrightness (1784) stays in line order near the
  end, apart from the other light functions at the start.
- **callcheck** (45 of 75 functions differ, all explained): the Container.h
  operator() calls (Fields(...), FieldNumber(...), Nodes(...), Marks(...))
  and the TRGBColor constructor are not lexical calls of names; the binary
  inlines GetMapLevel, GetNextObject, AbsoluteToRelative and
  RelativeToAbsolute in some callers; Ghidra split LoadAutomapBlock,
  SaveAutomapBlock and ShiftMapLevel, so their file calls and ClearField are
  counted under 0x4b3242, 0x4b3988 and 0x4bdbe0; the file member calls
  (open, readQuad, ...) are not resolved because several classes have
  methods of these names; AddMissileEffect's _ftol is a cast; the two
  DeleteEffect calls of ProcessEffects share one call site in the binary.
- **Tests** (maptest.cpp): the image's static constructors are called
  explicitly (its initialisers never run); each side works in its own
  directory of generated automap files; missing and damaged files are not
  tested (the original would raise an exception), so deleted marks (at 0,0,
  block 0,0) are never looked up. FindPath reached every result except -4
  (no path).

## graphics.cpp

Reconstructed by Claude Opus 5.5 (delegate; a first delegate's unreviewed
WIP, checked and finished by a second one), November 1, 2026.

- **DialogWindow/graphics gap (0x48b380-0x48b547)** (high for the split).
  DialogWindow.cpp ends with the catch block 0x48b287 (line 297, to 0x48b334).
  0x48b380 and 0x48b3b0 are TRGBColor's operator== and operator!= (`ret 0xc`,
  ECX = this, the other colour by value; operator!= negates the inlined ==):
  out-of-line copies of inline members, kept in graphics.obj because it is
  the first object in link order that calls them (FindVideoMaskedSprite
  0x48e09e, GetVideoPicture 0x48f1df; the only other caller is SurfaceGL.cpp
  0x4fbe47). file-ranges.tsv gives them to graphics.h (low: inferred because
  TRGBColor, TRect and TFont are the types of graphics.cpp's interface; GUI.cpp
  put them in GUI.h until graphics.cpp was merged, and they now are in
  graphics.h, which GUI.h includes). The rest, 0x48b3f0-0x48b54f,
  is graphics.cpp: 0x48b3f0 is TVideoCacheItem's constructor (missed by
  Ghidra; pushed to the eh vector constructor iterator by InitVideoCache at
  0x48db0b); 0x48b430, 0x48b460 and 0x48b4a0 use only graphics.cpp data
  (Fonts, NumberOfFonts, the memory cache and its hash table, the tables).
  GetFontLineHeight is called only from other objects, but its first caller
  (0x41a070) precedes graphics.obj in link order, so it is not an inline copy
  but graphics.cpp's own function. The three have no exception frame and no
  line evidence and open the file in binary order (as in Creatures.cpp).
- **Graphics/GUI gap**: not reconstructed (the GUI.cpp delegate settles it).
  No graphics.cpp function was found in it: 0x495480-0x495700 construct
  objects with TFonts whose colour comes from GUI.obj's copy at 0x5c1954, and
  0x496150 clears 0x5ccf98, which only GUI.cpp code uses. graphics.cpp's
  data ends at 0x5ccf97 and its code at 0x49547f (padding after the last
  catch block 0x4953c3).
- **Names** (from the binary, high): FileName, PATH_MAX, SpriteNumber,
  NumberOfSprites, MaskNumber, PictureNumber, NumberOfPictures, TileX, TileY,
  Picture.TilesX/TilesY, Buffer, Position, MemoryCacheSize, VideoCacheSize,
  Item.Type, EMPTY_ITEM, HashValue, HASH_TABLE_SIZE, FontNumber,
  NumberOfFonts, FontName, CharsPerLine, CharWidth, CharHeight, Surface,
  Width, Height, Text, Length, Alignment, LEFT_ALIGN, CENTER_ALIGN,
  RIGHT_ALIGN, Format.Alignment (so TFont's third field is Alignment, renamed
  from Unknown10), MemoryCacheBytes. Ours: every function name (medium where
  the behaviour is clear, low for the cache helpers Touch/Find/Remove/Insert
  and for GetFittingTextLength, GetTextPosition and GetFontLineHeight), the
  types TSprite, TPicture, TFontInfo, TMemoryCacheItem, TVideoCacheItem, the
  other fields and globals, SPRITE_ITEM and PICTURE_ITEM. PrintText* rather
  than DrawText* because windows.h defines DrawText. The values PATH_MAX 1024
  and HASH_TABLE_SIZE 100 are from the checks (`< 0x400`, `% 100`).
- **Surface slots** (medium for the parameters, low for the names): the
  calls give the parameters of TSurface +0x28 WriteImage, +0x2c
  WriteMaskedImage, +0x34 Blit, +0x3c BlitClipped, +0x48 StretchBlit, +0x90
  GetCacheSurfaces and +0x94 GetSurfaceRect; ToSurface and FromSurface are
  SurfaceGL.cpp's assertion names. +0x50 (Unknown50, whose default body calls
  +0x54) takes a surface and a colour key: every caller copies a TRGBColor to
  the stack (`sub esp,0xc`), so it was changed from four ints to
  (int Surface, TRGBColor ColorKey), and +0x54 likewise (same ABI). Only
  engines 1 and 2 (OpenGL and DirectX 9) get the call.
- **TVideoCacheItem's overlapping fields** (medium): masked sprites keep the
  mask number and four colours at +0x14/+0x18, pictures and font characters
  TileX, TileY and the text colour at +0x14/+0x18/+0x1c (the Find and Get
  functions read them so). Written as unions. Its constructor 0x48b3f0 clears
  the four colours and then the text colour, in that order; written as a
  user constructor with these stores. A C++03 union cannot hold a TRGBColor
  with a constructor, so the original declaration was probably different
  (low); the stores are what the binary shows.
- **TRGBColor's default constructor zeroes** (as GUI.cpp found): the binary
  zeroes default-constructed colours, also here (DrawOutfit's local array, the
  `new TSprite[]` and `new TPicture[]` loops, which leave Offset and the tile
  sizes alone). So TSprite and TPicture have no constructors of their own and
  DrawOutfit just declares its array; the WIP's explicit zeroing (written
  against the old non-zeroing TRGBColor()) was removed in the merge with
  GUI.cpp. TFont's third field is Alignment (was Unknown10 in GUI.h).
- **Dropped code with exception states left** (medium): FindMemorySprite,
  FindMemoryPicture, the four FindVideo* functions (11 states, no try block),
  GetTextHeight and GetFittingTextLength (21) and GetTextPartWidth (26) are
  written with TBEGIN/TEND whose try blocks the compiler removed (nothing in
  them throws). DrawPicturePart has 26 states and a try at 15-15, that is
  three assertions before TBEGIN, but only the PictureNumber one is in the
  code: it returns early when Width or Height is not positive, and the two
  assertions `Width>0` and `Height>0` after that return (as in
  DrawPicturePartClipped, lines 1192-1193) were removed as always true. Their
  line numbers are unknown.
- **Try blocks from the exception tables**: the four file functions have an
  inner try (states 11-12; the TReadBinaryFile is state 12) whose
  catch (TMyException &) throws PROGRAM_ERROR 2 (LoadSprites), 4
  (LoadSprite), 3 (LoadPictures) or 5 (LoadPictureTile); LoadPictureTile's
  tile assertions (lines 289-290) are inside it (states 11-22), so a bad tile
  becomes PROGRAM_ERROR 5 too. DrawMaskedSprite and DrawMaskedSpriteScaled
  have their assertions inside the try (states 0-10). DrawSprite,
  GetFontHeight and the Touch/Remove/Insert functions have no try block. No
  function has a switch (no jump table in 0x48b380-0x49547f).
- **Exact oddities kept**: the files are closed twice (close() and the
  destructor); LoadSprite and LoadPictureTile open the file for every sprite
  or tile and write past the buffer on malformed runs; GetFontLineHeight
  maps numbers below 0 and above NumberOfFonts to font 3 but reads before the
  table for 0; RemoveVideoItem ignores position 0, so a reused slot 0 stays in
  its old hash chain (the video Find functions give up after
  VideoCacheSize+2 steps); FindVideoPicture matches font characters of the
  picture (no colour test) and FindVideoFontChar matches a plain tile for
  colour 0,0,0; GetVideoPicture tests the colour key of Pictures[PictureNumber]
  (the next picture; past the end for the last one) against 1,1,1 and does
  the same in both branches; a blank advances text by the width of character
  index 12 (','); InitFont never looks at column 0 or row 0 and lets a blank
  character take part in the Height maximum with -1; DrawSpriteClipped skips
  negative sprite numbers, the other DrawSprite* only 0; DrawObject's count
  buffer holds three digits; DrawPicturePartClipped fails its assertions on
  an empty part where DrawPicturePart returns.
- **Data not static** (judgement): only SpriteSignature and
  PictureSignature are used by another file (Communication.cpp), but all
  graphics.cpp globals are declared in graphics.h so that graphicstest.cpp
  can compare the caches, hash tables and LRU lists directly (Objects.h
  exports its property tables the same way). They are in .bss order with
  their addresses; 0x5cc75c is unused.
- **Header structure**: graphics.h holds TRect, TRGBColor (with the two
  operators), DefaultFontColor and TFont, moved from GUI.h, and includes only
  common.h (TObject, a parameter of DrawObject, is forward-declared). GUI.h
  includes graphics.h; external.h includes GUI.h and declares no graphics.cpp
  function; Communication.cpp, Control.cpp and Player.cpp include graphics.h
  (the WIP's `#include "graphics.h"` inside external.h made the two headers
  include each other). GUI.cpp calls DrawPicturePart and
  DrawPicturePartClipped by name now; the latter's clip is a TRect, which the
  binary's DrawSkinClipped builds on the stack (four stores after
  `sub esp,0x10`), so the call passes TRect(ClipX, ClipY, ClipWidth,
  ClipHeight). guitest.cpp keeps its picture recorders: our two functions
  jump to them while TestGUI runs (and are restored after it), as talktest.cpp
  does with the CRT clock.
- **Source order**: line order; the three gap functions first, the Find
  functions (no lines) between TouchMemoryItem (466) and RemoveMemoryItem
  (522) and between TouchVideoItem (644) and RemoveVideoItem (762), and
  GetTextPosition (no frame) after GetFittingTextLength (1410), where the
  binary has them. The compiler placed InitFont and InitFonts (348-403) after
  ReloadSprites, GetTextWidth (1373) before ReloadPictures, and DrawObject
  and DrawOutfit (1100, 1137) at the end.
- **callcheck** (10 of 56 functions differ, all explained): the four file
  functions' TReadBinaryFile constructor, open and close calls are not
  resolved (several classes have methods of these names; 0x4df6b0 is both
  close() and the destructor, as in Map.cpp); InitVideoCache's eh vector
  constructor iterator 0x4175e0 is compiler-generated; the TRGBColor
  operators in FindVideoMaskedSprite and GetVideoPicture are not lexical
  calls; the Unknown50 calls (0x4eabb0) are virtual calls in the binary.
- **Tests** (graphicstest.cpp): the image's malloc/free and stdio are
  redirected (PatchScript); both sides load the real Tibia.spr and Tibia.pic,
  and every sprite and every picture tile is decoded on both sides and
  compared. The surface is a recording TSurfaceGL shared by both sides (the
  image's surface pointer 0x71d0cc is set to it); after every operation the
  logs of surface calls (with the tiles written), the memory and video
  caches, hash tables, LRU lists and font tables are compared. Small caches
  make items be replaced all the time, so the slot-0 behaviour is reached.
  Objects and outfits use the real Tibia.dat. Every function is tested
  except the assertion-failure paths; InitGraphics, ReloadSprites and
  ReloadPictures run in the directory of the data files.
## GUI.cpp

Reconstructed by Claude Opus 5.5 (two delegates: the first stopped on a Claude
API rate limit, WIP f6161d4; the second checked all of it, corrected it and
finished it), November 1, 2026.

- **The graphics/GUI gap 0x495480-0x4979ff is GUI.cpp and Container.h**
  (high); nothing in it goes to graphics.cpp. Evidence: GUI.cpp's static
  initialisers are 0x559e30 (Timers 0x5ccfa4, vector<TTimer*>(0,0,2,NULL)
  0x4973f0), 0x559e50 (SkinProperties 0x5ccfc8, constructor 0x496800) and
  0x559e60 (Hotkeys 0x5ce4f0, vector<THotkey>(0,50,10) 0x4976f0, missed by
  Ghidra); its data is 0x5ccf98-0x5ce51c (graphics.cpp's ends at 0x5ccf94,
  Help.cpp's starts at 0x5ce520). The gap functions are the compiler-generated
  constructors of the SkinProperties blocks (called only by 0x496800), the
  TWindow/TElement constructors whose vtables 0x564528/0x5645c8 hold the
  asserted GUI.cpp functions, ~TComponent and the deleting destructors of
  vtables 0x5644a0/0x5645c8, the TTimer getters, TCallback's constructor,
  InitHotkeys (clears 0x5ccf98) and the empty 0x496160. Container.h:
  vector<THotkey>::operator() 0x496570 and the two vector constructors.
  0x4a29e0 (in the Help.cpp range) is TWindow's deleting destructor
  (vtable 0x564528 slot 0); its names.tsv row was renamed from
  GUI_scalar_deleting_dtor.
- **Classes and vtables** (high for the layouts, medium for the class
  names). TComponent (vtable 0x5644a0, 0x24 bytes) is abstract: slots
  +0x4c-+0x68 and +0x78-+0x80 are _purecall (0x512a0d) and pure here.
  TElement (0x5645c8, no children, 0x24 bytes) and TWindow (0x564528, child
  list FirstChild/Next and Capture, 0x2c bytes, six more slots +0x84-+0x9c)
  derive from it; TTimer (0x5646e8, 0x14 bytes) has only its destructor
  virtual. Every slot of the three vtables is declared at its offset and was
  compared with the binary. The inline slots whose only copies are in
  earlier objects (GetNext 0x417680, GetParent, TWindow::GetFirstChild,
  GetCapture, Fit, GetMinimumWidth/Height 0x417a20, the TElement mouse
  answers 0x4259f0/0x425a00/0x41acb0/0x417a30/0x417a40, and
  TComponent::GetFirstChild/GetLastChild, which throw "not possible" at
  GUI.h lines 681/682) are defined in GUI.h and have names.tsv rows.
  TComponent's seven child functions throw "not possible" (lines 993-1049;
  line 1033 has no function, the linker dropped it).
- **FitToChildren and FitToChild take a TRect by value** (high for the
  16-byte block, medium for the type): Fit (0x4176c0) and every caller
  (0x441f4b, 0x442f58, 0x47fb59; FitToChild at 0x4882c0) build the margins
  with sub esp,0x10 and four stores, which MSVC does for a struct argument,
  not for four ints (the WIP had four ints; the stack layout is the same, so
  the difftest could not tell). SetChildArea and SetChildPosition get plain
  pushes at all their call sites. The only known 16-byte struct is TRect;
  its x, y, Width, Height are the left, top, right, bottom margins. The
  SkinProperties members passed this way (+0x0a0, Style194+0x50) became
  TRects.
- **Names from the binary**: SkinProperties and its member paths
  (PlayerStates.NumberOfIcons, BuddylistWindow.Logo, Buttons.Texture,
  CheckBox.Item, StatusWindow.HitpointsBar/ManaBar), DrawingList,
  DrawingListLength, NumberOfSkins, Parent, FirstChild, Capture, Component,
  getWidth/getHeight, the parameter and local names in the assertions and
  catch blocks (_OffsetX, _Width, Child, Child1/2, Search, Search1/2,
  Before1/2, LeastX/Y, skin, srcx, srcy, skinoffsetx/y, Surface, elapsed,
  Key, i). OffsetX/OffsetY and Width/Height follow from _OffsetX and _Width.
  Ours (medium where the behaviour is clear, low otherwise): TComponent,
  TElement, TTimer, TCallback, THotkey, TDrawing, TGUIPoint, the
  SkinProperties block types and their DataXXX members, DefaultFontColor and
  every function name (SetChildArea, FitToChild, FitToChildren, ClipChildren,
  PositionChanged, HandleEvent, Send, AddTimer, UpdateTimers, AddHotkey,
  CheckHotkey, DrawSkinClipped, InitSkinProperties, ...). The old external.h
  names GUIStyle and TGUIStyle are replaced by SkinProperties/TSkinProperties.
  PlayerStates' block constructor was inlined, so its boundaries are unknown
  and NumberOfIcons stays the direct member Data490 (low).
- **Graphics types in GUI.h** (judgement): TRect, TRGBColor and TFont belong
  to graphics.cpp's header, but GUI.h needs them complete, so they moved from
  external.h into GUI.h until graphics.cpp is merged. (Done in the merge with graphics.cpp: they are in graphics.h now,
  which GUI.h includes; see "graphics.cpp".) Their default
  constructors zero them (the SkinProperties constructors zero every such
  member); TFont() is font 4 with colour 224,224,224, read from a copy that
  every object constructing a TFont has (GUI 0x5c1954, DialogElements
  0x5c1230, an earlier object 0x5c066c), so DefaultFontColor is a header
  constant. Main's TRGBColor(int Color) palette constructor (Map.cpp) was
  merged into GUI.h's TRGBColor.
- **The empty function 0x496160** (low for the names): ExitControl calls it
  five times; following InitControl's order backwards they are ExitHotkeys
  (InitHotkeys is called last), ExitCommunication, ExitHelp, ExitCreatures
  and ExitInventory, each an empty function in its own file;
  TComponent::Arrange/PositionChanged and TElement::StopDragging fold into
  the same copy. KeyPress and HandleEvent fold into 0x4eac00.
- **Try blocks from the exception tables** (high): the assertions come before
  TBEGIN where the try starts above state 0 (DrawSkin 35, DrawSkinClipped 15,
  GetPrevious 5, RemoveChild 10, InsertChildAfter 10, SwapChildren 25,
  SetCapture/ReleaseCapture 5, SetChildArea/SetChildPosition/FitToChild 15);
  SetSize, InsertChild and AppendChild have assertions and no try; the seven
  "not possible" functions share one 5-state table without try;
  InitSkinProperties has 11 states and no try block (TBEGIN/TEND around a
  body that cannot throw, as in Creatures.cpp); UpdateTimers' extra state
  is its static LastTime. No function has a switch (no jump tables).
- **Odd behaviour kept** (high): SwapChildren of two children that are not
  neighbours removes them and inserts each after its own predecessor, so
  nothing changes; AppendChild does not clear Next when the list was empty;
  RemoveChild of the capturing child stops its drag and releases the capture
  through the child (with NULL); TWindow::StopDragging does not release the
  capture of the parents; TElement::SetCapture ignores its argument;
  MouseHover and HelpMouseMove ignore the capture, MouseHover goes only to
  the first child under the point; TTimer::Update sends the callback on every
  update, not only at the end; a timer of time 0 is never registered;
  CheckHotkey drops flag 0x200 and folds lower-case letters; RemoveHotkey
  moves the last entry into the gap; ClipChildren compares the right and
  bottom edges with the old position; FitToChildren does nothing without
  children and takes all margins as absolute values.
- **Error texts**: DrawSkinClipped's "some assertions failed (surface: ...)"
  and TComponent::Draw's "dialog origin outside main window (surface: ...)"
  are built with a stringstream as in the binary; the stream pieces were
  compared with the binary's strings (" x: ", " y: ", " i: ", "/", ")").
- **callcheck** (23 of 63 functions differ, all explained): virtual calls
  are lexical names in our source but indirect calls in the decompile
  (RemoveChild, InsertChild, InsertChildAfter, StopDragging, DeleteChild,
  GetParent; Child->MouseMove etc. also resolve to Control.cpp's free
  functions of the same names, GetMinimumWidth/Height to a MapWindow row);
  Timers(i) and Hotkeys(i) are Container.h operator() calls (0x452ac0,
  0x496570); IsExpired is inlined in UpdateTimers; the qualified calls
  TComponent::SetSize and TComponent::Draw are not resolved by the script;
  the stringstream constructor 0x403a10; Ghidra split the six mouse
  functions' child loops (rows *_block, not functions).
- **Difftest** (guitest.cpp, 4,823,996 checks): SkinProperties' constructor
  on 0xcc-filled memory and InitSkinProperties, byte for byte; DrawSkin,
  DrawSkinClipped and TComponent::Draw against recorders of the two
  graphics.cpp picture functions; two mirrored window trees (original
  objects made by the image's constructors, ours) under 30 kinds of random
  operations (each run at least 230 times), comparing trees, captures,
  return values, call logs of probe children and the help state; timers and
  hotkeys with probe receivers. Assertion failures are not exercised.
- **Left for graphics.cpp**: the two picture functions keep their FUN_ names
  (FUN_00490c60, FUN_004910e0); FUN_004910e0's declaration was added to
  external.h in its own block so the graphics.cpp lines stay untouched.

## MapWindow.cpp

Reconstructed by Claude Opus 5.5 (issue #1), November 1, 2026.

- **Ranges** (high). The Map/MapWindow gap 0x4bf860-0x4bf9ff holds the
  out-of-line copies of TMapWindow's inline members (destructor, the two
  minimum sizes, IsFieldVisible, SetLightColor, the deleting destructor):
  they open the object before the constructor (line 130), as the inline
  copies of the other files do. The MapWindow/Network gap 0x4ca0e0-0x4ca2ff
  is Network.cpp (file-ranges.tsv): InitServerList and the setters use only
  Network data (0x7152f0 on), 0x4ca2d0/0x4ca2f0 are TSocket's deleting and
  implicit destructor (a jmp to CAsyncSocket::~CAsyncSocket), 0x4ca2c0 is
  MFC's inline CObject::operator delete (PASCAL, ret 4), called only by
  unwind funclets of new-expressions. MapWindow.obj's data: the TCallback
  0x7152e8 (static initialiser 0x559fc0, TCallback(NULL, 0), never used;
  written as a static of ours, NoCallback) and, in .data before Network's
  0x5c22c4, its copy of the text header's table (the strings up to "Report
  Rule Violation" at 0x5c22ac, then DefaultFontColor's copy 0x5c22b4). Every
  object that includes the text header has such a copy (Control.obj's is
  external.h's 0x5c0c80 table); the two texts MapWindow uses are declared in
  external.h with this object's addresses (TextReceiveHint 0x5c2130,
  TextSelectPlayerToTrade 0x5c2258).
- **Two classes** (high for the layouts, low for the names). TMapWindow
  (vtable 0x56cf38, 0x92d8 bytes) is the map view; TMapFrame (vtable
  0x56d0b8, 0x40 bytes, created by MainWindow.cpp with operator new(0x40))
  holds it at +0x2c, keeps it at 15:11 and draws the border. The names
  TMapWindow (the file's name for the view) and TMapFrame are ours. Every
  vtable slot was compared with TWindow's: TMapWindow overrides SetSize, the
  minimum sizes, Draw, Arrange, LeftButtonDown/Up, RightButtonUp,
  HelpMouseMove and HandleEvent; TMapFrame SetSize, the minimum sizes and
  Arrange. TMapFrame's destructor is the implicit one (deleting destructor
  0x4adc40 in MainWindow.obj, which calls the folded 0x443e10, a jmp to
  ~TWindow).
- **Member layout** (high for offsets and types, low for the names except
  MAP_WIDTH/MAP_HEIGHT): the creature names (150 entries of ID and position),
  the textual effects to draw (200 entries of number, position and field),
  Light[18][14] (TRGBColor: the constructor clears it with the inline
  TRGBColor constructor before the try block), the map position, MaxLevel,
  CoverLevel[18][14] (the highest level with a FULLBANK ground), the scroll
  offset (the player's walk offset + 32), the 16x12 tile cache (TCacheField,
  0x8c bytes) and the hint button.
- **TObject converts to int** (medium). Draw's TEND1(Obj) prints "Obj" with
  the value of Obj.Type, and the assertions "SrcObj!=0", "TradeObject!=0" and
  "Obj!=CREATURE" compare an object with a number while the code tests its
  Type. external.h's TObject got `operator int() const { return Type; }`.
- **Status.cpp's object slots take the index first** (high). LeftButtonDown
  calls SetDragObject(Obj, Position, x, y, z), and the getters' first output
  goes to the Position argument of SendMoveObject, SendLook and the other
  messages. The parameters of Set/GetDragObject, Set/GetTradeObject and
  Set/GetUseObject and the variables behind them were renamed by address
  (DragX at 0x71c5c8 is DragIndex now, and so on); the code is unchanged.
- **TCreature's PKFlag and PARTYFlag** (high): the errors "unknown creature PK
  flag" (Creature->PKFlag, +0x90) and "unknown creature party flag"
  (Creature->PARTYFlag, +0x94) name them; Creatures.h's Skull and Shield were
  renamed (Communication.cpp, Creatures.cpp, creaturestest.cpp).
- **SkinProperties +0x60-+0x9b are five colours** (high for the types, low for
  the names): the values InitSkinProperties stores there are (248,248,248),
  (224,64,64), (64,224,64), (248,164,164), (180,248,180), and DrawField reads
  them as the frames of the creature under the mouse, attacked, followed, and
  attacked or followed under the mouse. GUI.h had a TRGBColor and three TRects
  (same size); InitSkinProperties' stores were rewritten member by member.
- **TDrawing constructors** (medium): `new TDrawing[8]` in TMapFrame's
  constructor zeroes the entries in a loop (an inline default constructor),
  and Arrange assigns whole entries built from seven values.
- **Surface slots** (high for the arguments, low for the names): +0x20
  DrawFrameClipped(Surface, TRect, LineWidth, TRGBColor, TRect Clip), +0x30
  BlitRect(ToSurface, x, y, FromSurface, TRect Source), +0x40
  StretchBlitRect(ToSurface, TRect, FromSurface, TRect), +0x4c
  ShadeRectangles(Surface, Count, TRect *, TRGBColor *) with four corner
  colours per rectangle, +0xa4 CanShadeRectangles(). +0x9c(2) returning true
  makes Draw forget the tile cache (the surface lost its contents, low).
- **FULLBANK = 30** (low for the name): the Tibia.dat flag after LENSHELP;
  Draw takes the highest level whose first object has it as the field's
  cover level. **EVENTHELPS** moved from Help.cpp to Help.h
  ("In(Hint,1,EVENTHELPS)").
- **Names of Map.cpp's GetTop*Object** (medium now): LeftButtonDown drags the
  "move" object, RightButtonUp makes the "look" object the trade (look) object
  and the "use" object the use object, LeftButtonUp in action state 7 uses
  the use object with the "multi-use" object, and HelpMouseMove shows the
  look object's help.
- **Limit** (low): a clamp between two bounds in either order appears inlined
  about thirty times (with constant bounds only one branch is left); the
  binary has no copy. Written as a static inline function of MapWindow.cpp.
  **TERROR1B** (local macro): the party flag error has "[bug0001106]" as its
  expression, like Map.cpp's TASSERTB.
- **Odd behaviour kept** (high): the light of a graphical effect is added
  with the field's level as its radius and its brightness as its level (the
  arguments are swapped; the missile light has them right); Draw reads
  Light[][] of a creature's field without checking it is on the map;
  HelpMouseMove asks GetObjectFlag(Type, LENSHELP) even when the look object
  is empty (type 0 fails the assertion); CacheSprite and FlushField draw the
  matching sprites of a tile again over it without clearing it; a party
  shield of type 1 is drawn only while the player is in no party; the classic
  control attacks only when the use object is not FORCEUSE; only creatures
  on the player's level get names; HandleEvent(27) without a hint button
  fails the assertion of RemoveChild.
- **Arithmetic**: TMapFrame::SetSize's 15:11 height is floor(width *
  0.73333335f) (0x512b50 is floor: its error path reports operation 11,
  _FpCodeFloor; the product is a float multiplication, fild/fmul m32). x87
  and SSE give the same floor here: the float constant is slightly above
  11/15, so the product is either a whole number plus a tiny excess or at
  least 1/15 away from one. AddLightSource's distance is
  (int)sqrt((double)dx * dx + dy * dy), as in Map.cpp.
- **Try blocks from the exception tables** (high): Draw (0-136) has the inner
  blocks of lines 477, 495, 527 (TEND1(Obj) each, with their own TObject),
  550, 757 (with 599 inside the creature-name loop, one try per iteration),
  776, 815 (with 794 inside the loop) and 830; DrawField (0-76) those of lines
  874, 884, 927, 987 (TEND2(i, Creatures[i]->OutfitType), i declared before
  the try), 1054 (TEND1(EffectNumber)) and 1060. AddLightSource's inner try
  has a single TMyException & handler that wraps the error with a
  stringstream text. The assertions of DrawObject, DrawCreature and
  DrawLightMask come before TBEGIN; GetCacheField and TMapFrame::Arrange have
  no try block. Switches: ArrangeTalkboxes' modes (tables 0x4c12b4/0x4c12c4:
  4; 9, 11, 18, 19; 22), Draw's party and PK flags (0x4ca0bc, 0x4ca0cc).
- **Source order**: line order (TMapWindow 130-1545, TMapFrame 1564-1606, its
  Arrange without lines last, where the binary has it). The binary emits the
  functions in a different order (the asserted helpers after HandleEvent, the
  mouse handlers after TMapFrame, Draw last).
- **SetChildArea** (note only): Arrange passes the hint button's area as one
  16-byte block (sub esp,0x10 and four stores) while GUI.h's SetChildArea
  takes four ints; the stack is the same. GUI.cpp's decision found plain
  pushes elsewhere, so the signature was left.
- **callcheck** (14 of 26 functions differ, all explained): constructor calls
  (TWindow's, TCallback's, TMapWindow's, TBlinkingButton's, the vector
  constructor iterator of DrawCreature's colours) are not lexical; qualified
  TWindow:: calls are not resolved; virtual calls (Arrange, SetChildArea,
  RightButtonUp, HandleEvent, InsertChild) are indirect in the decompile;
  LeftButtonDown/LeftButtonUp/RightButtonUp and DrawObject also resolve to
  Control.cpp's and graphics.cpp's free functions of these names; the
  compiler merged call sites (SetAttackID/SendAttack/GetAttackID/AddMessage in
  RightButtonUp, SetHelpSize in HelpMouseMove, DrawSkin in Draw); four of the
  eight IsFieldVisible calls of DrawFieldSprite are inlined; floor, _ftol and
  the stream operators are not in names.tsv.
- **Tests** (mapwindowtest.cpp, about 116,000 checks): one fake world for both
  sides (map objects and effects, creatures, talkboxes, preferences, event
  helps, action state; TCreature's GetDeltaX/Y and GetDrawDirection are fakes
  because they read Creatures.cpp's player step), logs of every drawing,
  surface, Communication, Status, Talk, Help and hotkey call, and the windows
  compared byte for byte after each of 24 kinds of random operations
  (constructor, the inline members against their out-of-line copies, the
  cache, sprites, objects, creatures, fields, whole frames with talkboxes,
  the mouse handlers in every action state, HandleEvent, the hint button's
  Arrange and removal, TMapFrame). Every message, send and drawing kind of
  the file was reached (counts in the summary line). Not exercised: the error
  paths and the hint button's creation (TBlinkingButton is
  DialogElements.cpp's). playertest.cpp's TSurfaceGL stubs for the new slots
  moved to mapwindowtest.cpp as recorders.
- **talktest.cpp fixed** (high): TestFilters deleted the filter vector's
  entries itself and vector<T>'s destructor (since Map.cpp) deleted them
  again. The double delete corrupted the heap only with some memory layouts:
  with this file's test added, build.cmd's longer TEMP path made difftest
  end with 0xc0000374 and no output. The test now clears the pointer.

## MainWindow.cpp

Reconstructed by Claude Opus 5.5 (issue #2), November 1, 2026.

- **Range and data** (high). The MainFrm/MainWindow gap 0x4ac010-0x4ac20d is
  MainWindow.cpp: TMainWindow's destructor (vtable 0x56c498), 0x4ac090 (it
  deletes MainWindow 0x5d16b0 and ModalWindow 0x5d16ac: ExitMainWindow,
  called by ExitControl), the deleting destructor and Draw, whose catch
  blocks (line 127) end the range. The object's data is 0x5d16a0-0x5d16bb:
  the three frame callbacks (OpenURL, SetDisplayMode, ExitProgram, written
  only by InitMainWindow), ModalWindow and MainWindow (both names from
  Control.cpp's assertions) and the unused TCallback 0x5d16b4, built by the
  static initialiser 0x559f30 between MainFrm's 0x559f20 (0x5d1674) and
  Map's 0x559f40, like MapWindow.cpp's (written as a static NoCallback;
  medium). The two texts it uses are its copy of the text table (external.h:
  TextNoBugReportRight 0x5c1ffc, TextAccountURL 0x5c2048; names ours).
  Ghidra missed 0x4ae420 (TSplitter::ReleaseCapture, inline: Dragging=false,
  jmp TElement::ReleaseCapture). 0x4adc40, so far TMapFrame's deleting
  destructor, is TStartWindow's (vtable 0x56c5c0 slot 0); TMapFrame's was
  folded into it. TSplitter's deleting destructor is the folded 0x429590
  (ContentWindows.obj; it calls the jmp-to-~TComponent 0x4672c0).
- **Three classes** (high for the layouts, low for the names). TMainWindow
  (0x4c bytes; MapFrame +0x2c, SideBar +0x30, DialogWindow +0x34 (name from
  "DialogWindow!=NULL"), SidePanel +0x38, Console +0x3c, Splitter +0x40,
  StartWindow +0x44, InGame +0x48), TStartWindow (0x30, Frame +0x2c) and
  TSplitter (a TElement, 0x30: Dragging +0x24, DragX +0x28, DragY +0x2c).
  Slots from the vtables: TMainWindow overrides +0x34, +0x44, +0x6c, +0x70;
  TStartWindow +0x34, +0x44, +0x70; TSplitter +0x48-+0x54 and +0x7c.
- **Function names** (ours, low unless the binary gives them): ChangeMode
  (0x4af5e0; its bool parameter is InGame), ResizeConsole (parameter
  ConsoleHeight from the catch block), GetConsoleWindowHeight (not
  GetConsoleHeight: Preferences.cpp has that name and ChangeMode calls it),
  Walk (dx, dy from the catch block), OpenContainer/CloseContainer
  (Container), OpenTrade/UpdateTrade/CloseTrade, OpenContentWindow and
  CloseContentWindow (Identifier, "unknown identifier"), SetConsoleActive,
  InitMainWindow, ExitMainWindow. TRADE_WINDOW (8) and ACTIONSTATE_NONE (0,
  added to Status.h) are named in assertions; the other identifiers stay
  numbers (6 "Skills" and 7 "Battle" from ContentWindows.cpp's titles, 9,
  0x40 plus a container number; 1-5 the side-bar windows).
- **Source order** (medium). Line order. The binary has a second ascending
  run after the splitter (KeyPress 203, HandleEvent 308-339, ChangeMode 463,
  OpenTrade 560, CloseTrade 590, the constructor 85, InitMainWindow 822),
  which the lines put in between. The destructor and ExitMainWindow (no
  lines) follow the constructor, before Draw, where the binary has them;
  GetConsoleWindowHeight sits between 475 and 503 and TSplitter::Arrange
  between 749 and 780, as in the binary.
- **No try blocks left** (high). GetConsoleWindowHeight (0x4ac7c0) and
  TSplitter::Arrange (0x4ae430) have exception frames with 11 states and no
  try block, sharing FuncInfo 0x5a9d48: a TBEGIN/TEND whose body cannot
  throw (the compiler dropped the try block and its catch blocks). Written
  with TBEGIN ... TEND; the TEND variant is not visible.
- **Exception tables** (high), checked with ehinfo.py: OpenContainer try
  states 5-7 (TASSERT(DialogWindow!=NULL) before TBEGIN, two new-expressions
  inside), OpenTrade 10-10 (both assertions before TBEGIN), the constructors
  1-1 and TStartWindow's 1-8 (after the base constructor; seven
  new-expressions), ChangeMode 0-13 (thirteen new-expressions), HandleEvent
  0-10 (two assertions), InitMainWindow 0-6, Draw's inner try 1-1 with a
  catch(...) that throws PROGRAM_ERROR 12. Jump tables checked against the
  binary: 0x4ad48c (OpenContentWindow, 6-9), 0x4aecd4 (KeyPress,
  0x10c-0x119), 0x4af424/0x4af458, 0x4af478/0x4af494 and 0x4af55c/0x4af574
  (HandleEvent's three ranges, with byte tables), 0x4afea8 (ChangeMode, 1-5).
- **Odd behaviour kept**: ChangeMode clears every child pointer except
  SideBar (out of the game it points to the deleted frame); the member
  pointers are uninitialised until the constructor's ChangeMode(false), which
  is safe only because FirstChild is NULL; KeyPress ignores every key outside
  connection state 8; HandleEvent and OpenContentWindow use DialogWindow
  without a NULL check (the events that need it come only from the game);
  TSplitter::MouseMove resizes the global MainWindow, not its parent; the
  splitter's DragX/DragY stay uninitialised until the first press; x has no
  name in TSplitter::MouseMove's catch block (TEND1(y)); event 0x21 falls
  through into event 2 (CopyTaintedClipboardText, then SetActionState(0)).
- **Evaluation order** (medium). SetSize reads Splitter->Height before the
  two minimum heights (the load comes first in the binary; written as a
  local). ChangeMode calls Preferences' GetConsoleHeight once (a local, not
  a macro-style max). The quit question of event 0x1fb is a new INT_PTR[4]
  (operator new[] 0x523817; Control.cpp's message 6 deletes it with
  delete[]).
- **SkinProperties.Style0D8 retyped** (medium). MainWindow.cpp reads
  +0x10-+0x37 (0x5cd0b0-0x5cd0d4) as five x/y pairs, the positions of "Enter
  Game", "Options", "Info", "Exit Game" and "Access Account"
  (InitSkinProperties stores (0,0), (0,60), (0,90), (0,120), (0,30));
  Data00/Data04 are the button panel's size, Data08 its position. The old
  TRGBColor at +0x10 and the points at +0x1c-+0x34 were guesses; GUI.cpp's
  stores are rewritten with the same offsets and values. +0x38-+0x47 is not
  used here; guitest.cpp's constructor check showed it must be zeroed, so it
  is a TRect (a guess: any type with a constructor would do). Data134 (the
  frame border), Data138 (the splitter height) and Data184 (the side bar
  width) got comments.
- **External sketches** (low). TConsoleWindow now derives from TWindow (0x6c
  bytes; Data34 kept for Control.cpp). New in external.h, for the files that
  are not reconstructed: TDialogWindow (vtable 0x563f80, 0x4c bytes; slot
  +0xa0 takes (16, window) to add one and (32, NULL) to rearrange; 0x486400
  finds a sub-window by identifier, 0x4865b0 stores the open ones; the inline
  hasContentWindow from the assertion text), TSubWindow and TSubWindowBody
  (the name TContentWindow is taken by Preferences.h's stored sizes; getBody
  is +0x44, the body's slots +0xa8 and +0xb8 are used), TBorder (vtable
  0x561358, 0x68 bytes, constructors 0x4547f0 and 0x454e30, parameter
  _Window), TTextButton (0x466bc0, _Text), TLabel (0x465b40), TObjectIcon
  (0x4526c0), TSideWindow3/4/5 and the ContentWindows.cpp creators (FUN_
  names kept). FUN_004362b0 and FUN_00436510 take the main window as their
  argument (push esi; Ghidra dropped it).
- **callcheck** (16 of 29 functions differ, all explained): the constructor
  calls of new-expressions, base constructors and destructors and TCallback
  temporaries are not lexical calls; TWindow::Draw, TWindow::SetSize and
  TWindow::InsertChild are names.tsv's TWindow_Draw, TComponent_SetSize and
  TWindow_InsertChild; virtual calls (SetChildPosition, ReleaseCapture,
  SetSize, Console->KeyPress, which the tool matches to Control.cpp's
  KeyPress) are indirect in the binary; the compiler merged the calls of
  Walk in KeyPress, of AddMessage, OpenContentWindow and CloseContentWindow
  in HandleEvent and of AddMessage in TStartWindow::HandleEvent;
  hasContentWindow is inlined; InitMainWindow's GetActionState count in the
  pseudocode includes the assertion text.
- **Tests** (mainwindowtest.cpp, about 14,000 checks over 6,000 random
  operations): the windows of the files that are not reconstructed are
  fakes on both sides (the image's constructors and members jump to the same
  code as ours), every other callee is a logging fake (PatchOwn for ours);
  the real map frames get a logging vtable right after construction and lose
  their map view, so nothing reaches the map. Both main windows get the same
  mode changes, sizes, drawing, keys, events, splitter drags, mouse events,
  containers, trade and content windows, start window events and
  ExitMainWindow/InitMainWindow; after each one the logs and a dump of both
  window trees are compared. A mutation check (three changed lines, then
  two) made thousands of checks fail. Not exercised: the error paths. It
  runs after mapwindowtest.cpp and patches for good as well;
  DIFFTEST_ONLY=MainWindow runs it alone. playertest.cpp lost its
  InitMainWindow stub.

## DialogWindow.cpp

Reconstructed by Claude Opus 5.5 (issue #3), November 1, 2026.

- **Range and data** (high). The Dialogs/DialogWindow gap 0x4854b0-0x48582f
  is DialogWindow.cpp: Dialogs.cpp's last catch block ends at 0x4854ad;
  0x4854b0 (missed by Ghidra) is slot +0x3c of TDialogWindow's vtable
  0x563f80, 0x485690 (also missed) slot +0x40 of TSubWindow's 0x563ed8; the
  other gap functions are the constructors and destructors of the three
  classes and 0x4857b0 (line 63). The object's only datum is the unused
  TCallback 0x5cc428 (initialiser 0x559e20, after Dialogs.cpp's 0x5cc420 at
  0x559e10; medium, written as a static NoCallback like MainWindow.cpp's).
  The file ends at 0x48b37f (graphics.h's operator== follows).
- **Three classes** (high for layouts and vtables, low-medium for names).
  TDialogWindow (0x4c bytes, vtable 0x563f80, a new slot +0xa0 Rearrange):
  Area (a TRect at +0x2c: the constructor stores the four ints twice, the
  member constructor and the body), Dragged +0x3c, Busy +0x40, FreeHeight
  +0x44, UpdateFreeHeight +0x48. TContentWindow (0x44 bytes, vtable
  0x563e38): Identifier, BackgroundY, Background, Dragging, DragX, DragY,
  DragBottom (+0x40, set and never read here). TSubWindow (0x70 bytes,
  vtable 0x563ed8, new slots +0xa0 Minimize and +0xa4 Maximize): Body +0x44,
  two TResizers, the up, close and minimize buttons, Icon, Title, IsMinimized
  +0x64, Data68 (cleared, not used here), RestoreHeight +0x6c. The vtables
  end where the next begins (0x563e38+0xa0, 0x563ed8+0xa8). Names: the base
  class is TContentWindow because the binary's inline
  `DialogWindow->hasContentWindow(TRADE_WINDOW)` looks up the column's
  children (so 0x486400 is GetContentWindow), and SetContentWindowSize
  stores their sizes ("ContentWindow" in Tibia.cfg); Preferences.h's stored
  size struct, ours, became TContentWindowSize. The derived class keeps the
  name TSubWindow of MainWindow.cpp's sketch (getBody, Body). Member and
  function names are ours except IsMinimized, Body, _Body, _Identifier,
  Search, Work, DeltaHeight, Strategy and the catch-block parameters. The
  side-bar windows of ContentWindows.cpp (external.h TSideWindow3/4/5)
  derive from TContentWindow (their constructors call 0x4854c0).
- **No inline header code** (medium). The six functions without frames or
  lines at the start of the object (GetMinimumWidth, TContentWindow's
  constructor, ~TSubWindow, GetFrameHeight, TSubWindow::GetMinimumHeight,
  TDialogWindow's constructor) are written in DialogWindow.cpp in the
  binary's order. GetFrameHeight (0x485670) and TContentWindow's constructor
  are called by ContentWindows.obj, earlier in the link order: a header
  copy would have been kept from there, so they are not header code. The
  same is assumed for the others. TSubWindow's constructor inlines the base
  constructor (auto-inlining, as in Utils.cpp).
- **Source order** (medium). Line order. The binary puts Arrange (169),
  Shrink (377-394), Grow (505) and Rearrange (271-297) at the end of the
  object: each calls a function defined further down (GetFreeHeight 524,
  the strategy helpers), the pattern noted for Creatures.cpp.
- **Exception tables** (high), checked with ehinfo.py: every function with a
  try block has states 0-0 except Draw of TContentWindow 0-10 (two
  assertions), DragChild, Shrink and TSubWindow::Arrange 0-5 (one each),
  Rearrange 0-15 (two assertions and the TERROR), TSubWindow's constructor
  1-8 (after the base class; an assertion and two new-expressions), Minimize
  and Maximize 5-5 (assertion before TBEGIN). The constructors of
  TDialogWindow and the destructor have frames without try blocks (the base
  class unwinding). TDialogWindow::SetSize's and TContentWindow::ReleaseCapture's
  catch blocks name no parameter (TEND). Jump table checked: Rearrange's
  0x48b338 with the byte table 0x48b354 (Strategy-1, 0-31): 1, 2, 4, 8, 16,
  32, everything else the default ("unknown strategy").
- **Odd behaviour kept**: the free-space background is nine skins with two
  odd sizes (the bottom left skin 28 is as wide as skin 26, the right edge
  31 as wide as skin 135); TSubWindow's right edge starts at the top edge's
  height; FitChildren clears Busy itself; Shrink tests a second
  GetFreeHeight that cannot be positive; Rearrange 4 lays the children out
  twice; Minimize calls the body's +0xa0 before the body NULL check;
  HideUpButton compares IsChild with true; MakeRoom calls Work->SetSize
  before Work is a child (the creators size their windows first); a
  minimized window shorter or taller than its frame makes Shrink fail its
  assertion or loop for ever (the game's Style194.Data4C is the frame height).
- **External sketches** (low). TSubWindowBody (ContentWindows.cpp) gained
  the comments of +0xa0/+0xa4 and the container body's 0x42e4e0 (FUN_ name
  kept: it shows or hides the window's up button); new: TResizer
  (DialogElements.cpp 0x453be0, vtable 0x5612b0, 0x38 bytes, "_ToResize"),
  TSkinButton::SetSkins (0x458b20, "_ButtonUp", "_ButtonDown"); both have
  low-confidence names.tsv rows so that callcheck resolves them.
- **MainWindow.cpp** changed only in names and casts: GetContentWindow
  returns a TContentWindow, cast to TSubWindow where the body is used
  (OpenContainer, UpdateTrade) as the original must; the setters and
  StoreOpenDialogs replace the FUN_ names, UnknownA0 is Rearrange.
- **callcheck** (23 of 47 functions differ, all explained): constructors of
  base classes, ~TWindow and new-expressions (TResizer) are not lexical
  calls; the virtual calls (Rearrange, DeleteChild, GetPrevious, GetParent,
  SetSize, Arrange, GetMinimumHeight, ReleaseCapture, SetChildArea,
  SetChildPosition, FitToChild, HandleEvent, Minimize, Maximize,
  AppendChild) are indirect in the binary; IsChild in the destructor is a
  direct call the tool cannot resolve (two classes have that name); the
  compiler merged Rearrange's PositionChildren calls (7 in the source, 4
  call sites). tools/callcheck.py now resolves `Class::method(` calls
  (with TWindow -> TComponent for SetSize) and no longer matches a member
  call `x->Name(` to a free function of that name (Control.cpp's MouseMove,
  LeftButtonDown, ...); MainWindow.cpp and MapWindow.cpp went from 16 and 14
  differences to 14 and 13 with it.
- **Tests** (dialogwindowtest.cpp, about 24,500 checks over 12,000 random
  operations in 8 rounds): two columns of the image's classes and two of
  ours, one with sub-windows and one with plain content windows; bodies,
  buttons, icons, titles and resizers are shared test objects (the image's
  0x453be0, 0x458b20 and 0x42e4e0 jump to the same code), DrawSkin,
  DrawPicturePart, FillRectangle, SetContentWindowSize and AddOpenDialog log
  on both sides; the logs, results and full dumps of both trees are compared
  after every operation; all patches are undone at the end. A mutation check
  (five changes, one at a time) failed each run. mainwindowtest.cpp lost its
  fakes of the three classes: the dialog windows are real on both sides
  (the ContentWindows.cpp creators make the image's or our TSubWindow and
  TContentWindow), the dumps show their members. The shared fakes live in
  dialogwindowtest.cpp with hooks (difftest.h's TDialogFakes).
  DIFFTEST_ONLY=DialogWindow runs the test alone. Not exercised: the error
  paths.

## ContextMenus.cpp

Reconstructed by Claude Opus 5.5 (issue #4), November 1, 2026.

- **Range** (high). The ContentWindows/ContextMenus gap 0x43d6f0-0x43d86b is
  ContextMenus.cpp: 0x43d6f0 (no frame, no line) and its deleting form
  0x43d710 are the skill menu's destructor (they store the vtable 0x55f5f0
  whose constructor 0x4454f0 is line 1442 and clear the skill window's +0x30),
  0x43d730 is THelpBubble::SetSize (catch blocks at 0x43d86c name line 142);
  every caller of the gap's functions is in ContextMenus.cpp. The object ends
  at 0x4458ff (Control.cpp's 0x445900 follows, already settled). No data of
  its own (all texts are string literals).
- **NoCallback** (high for the datum, medium for the form; issue #26). Besides
  the texts, the object has one datum: the unused TCallback 0x5c4508,
  constructed as TCallback(NULL, 0) (0x496130) by the static initialiser
  0x559de0 (no atexit; its only reference is the initialiser). It sits right
  after ContentWindows.obj's 0x5c4500 (initialiser 0x559dd0) and before
  Control.obj's .bss (0x5c4510). Written as a static NoCallback after the
  includes, like DialogWindow.cpp's.
- **Classes** (high for layouts and vtables, low-medium for names).
  THelpBubble (vtable 0x55f8b8, 0x30 bytes) is a TWindow, not a
  TContextMenu as Control.cpp's sketch had it: its constructor calls TWindow's
  directly and Control.cpp places it only in action state 9 (help).
  Control.cpp's FUN_00442110 is THelpBubble::Place. TContextMenu (0x55f730,
  0x34 bytes: EntryWidth, Separator, Empty), TContextMenuEntry (0x55f6a8, 0x74
  bytes: Text[50], Shortcut[20], TCallback) and nine subclasses, sizes from
  Control.cpp's new-expressions and the constructors. All vtables have 40
  slots (no new virtuals). Only the skill menu has a destructor of its own;
  the others use the folded 0x4adc40, the entry the folded 0x429590 (kept in
  ContentWindows.obj, names.tsv kind stl). Class names are Control.cpp's
  sketches (ours) plus TContextMenuEntry; member names from the assertions
  (LookedObject, LookedObjectX, UsedObjectX, UsedObjectY, LastEntry) and the
  catch-block parameters (_CharacterID, _TextWindow, _LineNumber,
  _ConsoleWindow, _Edit, _Data, _SkillsWindow); the rest is ours. The menus'
  last two constructor parameters (MainWindow's width and height) are named
  ParentWidth/ParentHeight (ours).
- **Inline header functions** (medium). THelpBubble's GetMinimumWidth and
  GetMinimumHeight (0x4420f0, 0x442100, missed by Ghidra) sit right after its
  constructor, TContextMenu::HandleEvent (0x43e9f0, missed as well; it only
  calls SetActionState(0)) right after TContextMenu's: inline virtuals are
  emitted with the vtable, which comes with the constructor. They are written
  in ContextMenus.h. The two Arrange functions (no frame, no line) are in the
  .cpp where the binary has them (after SetSize and after TContextMenu's
  constructor), ~TSkillContextMenu at the top of the file.
- **Source order** (medium). Line order. The binary moves the help bubble's
  constructor (106) and Place (257), AddEntry (506) and Show (486) behind the
  event handlers, followed by all menu constructors: each calls a function
  defined further down (Creatures.cpp's pattern).
- **Exception tables and switches** (high), checked with ehinfo.py: the
  assertions before TBEGIN in the help bubble's constructor (6-8), AddEntry
  (15-16), the text menu (11-11: 1056 and 1057), the request, edit, mark and
  skill menus (6-6) and the channel menu (6-11, its second assertion inside);
  the constructors' try blocks start at state 1 (the base class). Place has
  states 0-30: six assertions, the second of the left-corner case (line 263,
  newY, the same register as newX) was removed by the compiler, which also
  skips 267/268 on that path; written as in the source. KeyPress's catch
  prints Key as the constant 27 (constant propagation). Jump tables checked:
  0x43fd98/0x43fdfc (object events), 0x440350/0x44038c (creature),
  0x4408d8 (VIP, inside a compare tree), 0x44163c (edit), 0x4419ec (mark),
  0x441d34/0x441d48 (skill), 0x443940 and 0x443df8 (party flags 0-4).
- **Event numbers** (high for the values): written in decimal (10001 Look,
  ..., 10100 message, 10200 paste, 10300 VIP, 10400 marks, 10500 skill bars);
  the ranges are clearly decimal. No enumerator names are known, so none are
  invented. The case order follows the binary's code order.
- **Odd behaviour kept**: Show adds an empty entry to a menu without entries
  after the layout loop (it keeps its own width and 0,0); every Show shrinks
  the last entry again; the rotate entry tests the flags of the used object
  even without one; "Invite to Party" is formatted with an unused name
  argument (left out in our sprintf: the compiler warns about it, the output
  is the same); the text and request menus test the addressee array against
  NULL; THelpBubble::SetSize keeps the larger of minimum and maximum when the
  minimum is above it; the help bubble's frame uses skins 52-60, the menu's
  207-215 with the middle 207; the channel menu leaves ShowServerMessages
  unset for the default channel; event 10402 (walk to a mark) has a case but
  no entry; unknown events do nothing in the text and request menus and are
  errors elsewhere. Callees whose names do not fit their use here are kept
  (Talk.cpp's HasUnreadConsoleText decides "Invite to private chat", i.e.
  whether the player has a private channel; worth a review there).
- **Text buffers** (low). Char buffers of the menus are 50 bytes (the entry
  text limit), 200 for the ignore message (_snprintf's limit) and 220 for the
  report title; the binary's frames are not exact enough to give more.
- **Data types changed elsewhere**: SkinProperties.Data160/Data168 (two
  TGUIPoints) are one TRect Data160 (the help bubble's text margins, passed
  by value as one 16-byte block like Data0A0); GUI.cpp's InitSkinProperties
  writes .Width/.Height, byte-identical. Map.cpp's DeleteAutomapMark takes
  TMark &Deleted (its only caller passes the menu's mark; it was a
  low-confidence TCoordinate &). New external.h sketches: TTextWindow (the
  console's text view with TextBuffer at +0x24 and four FUN_ methods),
  TConsoleWindow's five FUN_ methods used here, TSkillsWindowBody (ContextMenu
  at +0x30), TEditLine (+0x5c, +0x60, +0x66), TFormattedText (0x465e60),
  TSeparator (0x452cb0); TConsoleData34::Data2C is a TTextWindow *. All
  names low confidence. external.h no longer includes a ContextMenus header:
  ContextMenus.h needs Map.h, which includes external.h (Control.cpp includes
  it directly).
- **callcheck** (24 of 36 functions differ, all explained): base class
  constructors and ~TWindow, new-expressions (entries, separators, the
  formatted text, TTalkLine) and member initialisers (TCallback) are not
  lexical calls; virtual calls (GetMinimumWidth/Height, Arrange, SetChildArea,
  FitToChildren, SetSize, the TTalkBuffer queries and Clear) are indirect in
  the binary or resolved to the own class by the tool; the compiler merged
  call sites (AddEntry in the constructors, SendAttack, AddMessage,
  CopyToClipboard); the text menu's assertion strings contain
  "GetCreature(GetPlayerID())".
- **Tests** (contextmenustest.cpp, 32,980 checks over 3,000 windows):
  every window kind built from the image's and our constructors in memory
  with the same contents, then random draws, clicks, right clicks, keys,
  events, Arrange, SetSize/Place/FitsIn (help bubble) and Show/AddEntry/
  AddSeparator; logs of every callee (logging fakes on both sides answering
  from a random world) and dumps of both windows compared after each step,
  then the destructors. A mutation check (six changes, one at a time) failed
  each run. DIFFTEST_ONLY=ContextMenus runs it alone; it restores every patch.
  Not exercised: the error paths, and inputs the original fails on as well
  (a second Show that shrinks the last entry below 1 pixel, a separator as
  the last child).

## ConsoleWindow.cpp

Reconstructed by Claude Opus 5.5 (issue #5), November 1, 2026.

- **Range** (high for the first gap, medium for the second). The
  Communication/ConsoleWindow gap 0x4174d0-0x41864c is ConsoleWindow.obj:
  Communication.cpp's last catch block ends at 0x4174b6; 0x4174d0-0x41851f is
  one run of out-of-line inline copies (common.h's In, graphics.h's
  TRGBColor(), GUI.h's TDrawing() and the TComponent/TElement/TWindow slots
  of the console's five vtables, which Communication.obj does not use;
  DialogElements.h's TEditLine::HasSelection 0x417520; ConsoleWindow.h's
  TTextWindow::HasSelection 0x417590, the inline destructors of TTabBar and
  TTextWindow with their deleting forms, TLineFormat's constructor (lines
  47-54 of ConsoleWindow.h) and Container.h's vector<TLineFormat>), then
  TTextWindow's constructor 0x418520 and MouseMove 0x4185b0 (line 165, whose
  PROGRAM_ERROR catch 0x418639 was the gap's last entry). In and TRGBColor()
  are assigned with the run (low: nothing ties them to ConsoleWindow.obj
  except their place). The ConsoleWindow/ContentWindows gap 0x425920-0x425e6f
  is ContentWindows.obj: ConsoleWindow.cpp's last catch block ends at
  0x425918; 0x425a10/0x425a40 are slots of a ContentWindows vtable,
  0x425a70/0x425ad0/0x425e00 destructors of ContentWindows classes (vtables
  0x55e710, 0x55e7c8, 0x55e880, whose constructors are ContentWindows.cpp's)
  with their deleting forms; the inline copies before them (the clamp 0x425920
  called only by DialogElements.cpp, TRGBColor(int) 0x425950, TElement's
  MouseMove 0x4259f0 and the folded true-returning slots 0x425a00) belong to
  the same run, and ConsoleWindow.obj needs no TElement::MouseMove (both its
  TElement classes override it). 0x425b10 is vector<TMark>'s destructor
  (names.tsv). The ContentWindows functions keep their FUN_ names for its
  issue. ConsoleWindow.obj's vtables are 0x55dcd8-0x55e1a0; its data are its
  copy of DefaultFontColor (0x5c066c) and of the text table
  (0x5c044c-0x5c0664, external.h; names ours). tools/triage.py now lets
  header evidence win over file-ranges.tsv as assertion evidence does (the
  three header functions of the first gap stay header code; four Container.h
  functions of older ranges changed from "range" to "header", same file).
- **NoCallback** (high for the datum, medium for the form; issue #26). The
  object's only .bss datum is the unused TCallback 0x5c4390, constructed as
  TCallback(NULL, 0) (0x496130) by the static initialiser 0x559dc0 (between
  Communication.obj's 0x559da0 and ContentWindows.obj's 0x559dd0; no atexit;
  its only reference is the initialiser). It follows Communication.obj's .bss
  (0x5c4340-0x5c438f) and comes before ContentWindows.obj's (0x5c4398).
  Written as a static NoCallback after the includes, like DialogWindow.cpp's.
- **Classes** (high for layouts and vtables, low-medium for names).
  TConsoleWindow (0x6c bytes, vtable 0x55e1a0; Data30 is never used),
  TTabBar (0x40, 0x55dcd8, a new slot +0xa0 Rearrange), TTab (0x58,
  0x55e098), TConsole (0x38, 0x55dfc0; the implicit destructor, its deleting
  form folded into 0x4adc40), TTextWindow (0x94, 0x55dd80, new slots +0x84
  FormatLines and +0x88 GetPosition) and the struct TLineFormat (0x28). The
  class names are ours: the variables CurrentConsole, CurrentTab, DefaultTab
  and NewTab gave TConsole and TTab; TTextWindow keeps the name of
  ContextMenus.cpp's sketch ("console text window" in TConsole::Arrange's
  error message). Member names from the binary: LineText, InvisibleLines,
  Input, TextBuffer (and getFirstLineNumber, getLastLineNumber,
  getTextBuffer, getCaption); the rest are ours. Ghidra missed four slots:
  0x417700 (GetMinimumHeight of TTab and TTabBar, folded: the tab height),
  0x417a50 and 0x41acc0 (the ReleaseCapture of TTextWindow and TTab: they
  clear Selecting/Dragging first) and 0x41de70
  (TConsoleWindow::GetMinimumHeight); they and the destructors of TTabBar and
  TTextWindow are written inline in ConsoleWindow.h, TTextWindow's
  constructor (no line, no try block) at the top of the .cpp, where the
  binary has it (DialogWindow.cpp's precedent).
- **Function names** (ours, low-medium unless the binary gives them):
  SetTextBuffer, FormatLines, GetTalkLine, GetLineFormat,
  GetSelectionStart/End, SelectAll, GetSelectedText, GetPosition,
  HasSelection; TTab's SetActive (parameter _Active from the catch block);
  TTabBar's SetCurrentTab, GetContainer ("failed to get composite",
  "Container"), FindTab, ShowTab, StoreOpenChannels, Rearrange;
  TConsoleWindow's SetActive (MainWindow.cpp's SetConsoleActive,
  "Active?1:0"), UpdateButtons, OpenDefaultChannel, OpenChannel (RecvMode,
  SendMode from the catch block), AddChannel, CloseChannel,
  ReportRuleViolation (Question), Get/SetServerMessages, GetTextBuffer,
  SendInput, AddToHistory, ParseInput, SaveChannel. Events are written in
  decimal (201-213, 403), AddMessage's numbers in hex.
- **Source order** (medium). Line order. The binary moves TTextWindow's Draw
  (84), Arrange (148), LeftButtonDoubleClick, RightButtonUp and
  HandleEvent, TConsole::Arrange, TTab's LeftButtonDown and RightButtonUp,
  TTabBar's constructor, SetCurrentTab, DeleteChild and RemoveChild and most
  of TConsoleWindow behind the rest: each calls a function defined further
  down (Creatures.cpp's pattern).
- **Exception tables and switches** (high), checked with ehinfo.py (every
  catch is the standard trio): no try block in TLineFormat's constructor (25
  states, five assertions), SetCurrentLineNumber, GetServerMessages and
  GetTextBuffer (0x4193d0 and 0x41f7f0 share one FuncInfo) and TTextWindow's
  constructor (the base class only); assertions before TBEGIN in TTab's
  constructor (11-11) and Draw (10-10), SetCurrentTab (5-10),
  SetServerMessages (10-10) and AddChannel (5-11); the constructors' try
  blocks start at state 1. TTextWindow::Arrange has five states without code
  (0-5): the folded assertion of an inlined SetCurrentLineNumber(-1), written
  as that call (medium; the binary stores CurrentLineNumber directly).
  TConsole::Arrange's error builds a std::stringstream (states 0-7). Jump
  tables checked: ParseInput 0x420048/0x420070 ('A'-'Y'),
  TTextWindow::HandleEvent 0x421430 (16-20), SendInput 0x4241c8/0x4241e4
  (talk modes 0-15; 6 and 12 go to the default), KeyPress 0x424ad4
  (0x100-0x10b), TConsoleWindow::HandleEvent 0x424f68/0x424fa0 (201-403;
  209 is the default).
- **Odd behaviour kept**: FormatLines does not reset LineEnd between the
  parts of a talk line and loops for ever in a width without room for a
  character; TTextWindow::Arrange loops for ever on a line without a format;
  a buffer whose lines were removed is neither drawn nor formatted by Draw
  until the next Arrange (its LinesChanged is dropped); GetSelectedText does
  not test a missing format in a one-line selection; GetPosition asks for the
  bottom line's height twice; LeftButtonDoubleClick tests the character
  before the length; TTabBar's constructor runs Arrange before Busy is set
  and never sets Changed; a dragged tab can be pushed out of the bar by a
  later layout (the next move then fails its assertion); TConsole::Arrange
  places the scroll bar with the skin 173 width added and taken away;
  TTab::~TTab leaves the player's own private channel only when the server
  closed it; ReportRuleViolation shows an open request without event 201;
  AddChannel asserts NewTab after using it; the ignore button's skin shows
  the talk throttle; SendInput cuts the addressee inside the edit line's
  text after clearing the line; ParseInput's "unknown message separator"
  cannot happen.
- **Names changed elsewhere** (binary evidence): Talk.h's
  TTalkBuffer::IsDefault is isDefault (the assertions
  "CurrentConsole->getTextBuffer()->isDefault()==false", lines 2061 and
  2072); Talk.cpp's HasUnreadConsoleText (0x502880) is HasOwnPrivateChannel:
  SendInput sends "#I"/"#X" invitations only when it is true, and the
  function looks for a selected private channel (the one Control.cpp's
  message 0x31 marks as the player's); ContextMenus.cpp and the tests are
  renamed. Talk.h gained the inline TTalkBuffer::GetFirst/GetLast (cursor =
  first/last, returned; FormatLines and AddToHistory have the pattern
  inlined).
- **SkinProperties.Style9E8 retyped** (high for the offsets; the types are
  guesses, its constructor 0x495b70 zeroes +0x04-+0xa7): five TRects (the tab
  bar, an unused one, the edit line, an unused one, the current channel),
  seven TGUIPoints (the button positions), a TRect (the text window in
  TConsole), a TRGBColor (the selection) and the font; GUI.cpp's stores are
  rewritten with the same values and offsets (guitest.cpp compares them byte
  by byte).
- **DialogElements.cpp sketches** (external.h, low): TEditLine is a TWindow
  (vtable 0x560e98, 0x68 bytes, constructor 0x467b50 with an area, 255 and
  0; SetActive 0x45bc80; the inline HasSelection; slots +0xa0 SetText, +0xa4
  Clear, +0xb8 SetReadOnly, +0xc0 SetFont; Text at +0x2c); TSkinButton got
  its constructor 0x466860 and members; new: TToggleButton (0x4671e0),
  TMultiStateButton (0x4672d0, a varargs constructor: __cdecl with this
  pushed first; Reset 0x452790), TScrollBar (0x469a80) with its
  TScrollBarRange (SetRange 0x460450, Value at +0x30). All have
  low-confidence names.tsv rows.
- **Callers** changed in names only: MainWindow.cpp (SetActive,
  OpenDefaultChannel), Control.cpp (OpenChannel with the name and caption as
  strings, AddChannel, ReportRuleViolation, CurrentConsole->TextWindow),
  ContextMenus.cpp (GetTalkLine, HasSelection (inline now; the binary calls
  its out-of-line copy), GetSelectedText, SelectAll, the console's methods,
  CurrentConsole->TextWindow); each call checked against decomp/clean.
- **callcheck** (55 of 76 functions differ, all explained): base class
  constructors and destructors, new-expressions (TTab, TTextWindow, the
  DialogElements objects, TTalkBuffer, TTalkLine), TLineFormat and TCallback
  temporaries are not lexical calls; Container.h's operator() (Lines(i)) is
  not seen; virtual calls (GetPosition, ReleaseCapture, Arrange,
  FormatLines, Rearrange, the TTabBar child functions, SetChildPosition,
  SetChildArea, SetSize, GetParent, GetPrevious, HandleEvent and the
  TTalkBuffer inline virtuals) are indirect in the binary; ambiguous member
  names (SetActive, InsertChild, IsChild, SetSize, GetNext, Clear,
  GetTextBuffer) are not resolved or resolved to another class; HasSelection
  is inlined in the selection functions and SetCurrentLineNumber in
  TTextWindow::Arrange; the compiler merged call sites (GetTextPartWidth in
  Draw, SendLeaveChannel in ~TTab, AddMessage and SetCurrentTab in
  HandleEvent); the pseudocode of SendInput and ReportRuleViolation counts
  the assertion texts "GetCreature(GetPlayerID())". (A comment line starting
  with "// 0x" splits a function for the tool; ConsoleWindow.cpp avoids
  them.)
- **Tests** (consolewindowtest.cpp, 30,928 checks over 300 rounds of 20-60
  random operations): two consoles from the image's and our constructors in
  memory with the same contents, sized and given the default channel; then
  sizes, drawing, keys (typing, sending every kind of command, the selection
  keys, hotkeys, the history), events, opening, adding and closing channels,
  the report, the server message switch, saving a channel (the files are
  compared), mouse presses, drags, double and right clicks and the wheel in
  the tab bar and the text, the tab bar's events, the fake buttons'
  callbacks and Talk.cpp changes; logs and dumps of both trees (every member,
  the formatted lines with their talk lines, the hidden tabs) compared after
  each step, then the destructors. The DialogElements classes are fakes on
  both sides (also the definitions that contextmenustest.cpp and
  dialogwindowtest.cpp link); the text buffers are test subclasses of
  TTalkBuffer, one set per side with the same lines; the image's clock,
  localtime, ctime and fprintf and our CRT's _time32 are patched; DrawSkin is
  real on both sides (ours is inlined into TComponent::Draw) and the picture
  parts are logged. A mutation check (eight changes, one at a time) failed
  each run. DIFFTEST_ONLY=ConsoleWindow runs it alone; it restores every
  patch. mainwindowtest.cpp keeps a fake console on both sides: the image's
  constructor and ours (reached through its decorated name with
  /alternatename) build a TWindow of the same size, SetActive and
  OpenDefaultChannel log. contextmenustest.cpp's console methods are fastcall
  fakes of the real methods on both sides (TTextWindow::HasSelection runs on
  the test's selection). Lesson: two test files must not define classes of
  the same name (the console test's first buffer class got
  contextmenustest.cpp's TTestBuffer vtable: the linker keeps one COMDAT).
  Not exercised: the error paths and inputs the original fails on as well (a
  width without room for a character, a dragged tab pushed out of the bar,
  a talk mode without its addressee or channel).

## ContentWindows.cpp

Reconstructed by Claude Opus 5.5 (issue #6), November 1, 2026.

- **Range** (high). ContentWindows.obj is 0x425920-0x43d6ef: the
  ConsoleWindow/ContentWindows gap 0x425920-0x425e6f (settled in issue #5; its
  file-ranges.tsv note now names every function), the assertion and between
  functions up to CreateBuddylistWindow (0x43d390, line 280) and its catch
  blocks; ContextMenus.cpp follows at 0x43d6f0. No header code at the end. The
  run of inline copies at the start: common.h's Limit (0x425920, see below),
  graphics.h's TRGBColor(int), Map.h's TMark() (0x4259a0, missed by Ghidra:
  the constructor the eh vector constructor iterator of vector<TMark>'s
  constructor 0x426170 takes), ContentWindows.h's inline virtuals (0x4259b0
  TTradeWindowBody::Draw, 0x4259c0-0x4259e0 and 0x425b00 TSubWindowBody,
  0x425a10/0x425a40 TContainerWindowBody, all missed by Ghidra and found
  through the vtables), GUI.h's TElement slots, Container.h's vector<TMark>,
  the inline destructors of TBattleWindowBody, TBuddylistWindowBody and
  TCombatControls with their deleting forms, Container.h's
  vector<TSkinButton *>(min, max, block, init) (0x425e70, TRadioButtonGroup's
  inline constructor) and vector<TMark>(min, max, block). 0x434000 (missed by
  Ghidra, no frame) is TTradeWindowBody::GetContentHeight, written in the
  .cpp where the binary has it (between the destructor, line 3048, and
  SetSize, line 3066). 0x427697 is a Ghidra split of Scroll's jump table
  (names.tsv row).
- **Data** (high). .bss 0x5c4398-0x5c4507: the automap's mouse-over buffer
  (char[100]), the skill window's (char[256], "You have %d percent to go")
  and the object's NoCallback (0x5c4500, static initialiser 0x559dd0 between
  ConsoleWindow.obj's 0x559dc0 for 0x5c4390 and ContextMenus.obj's 0x559de0
  for 0x5c4508), which TTradeButtons' constructor copies into its Callback.
  The other two are unused and not in ConsoleWindow.cpp and ContextMenus.cpp
  yet (issue #26). ContentWindows.obj's copy of the text table is 0x5c0678-0x5c08c3
  (external.h, names ours; the regular expression pointer 0x5c08c4 and the
  copy of DefaultFontColor 0x5c08c8 close it, as in ConsoleWindow.obj's copy
  0x5c041c-0x5c0677), ContextMenus.obj's starts at 0x5c08d4.
- **Classes** (high for layouts and vtables, low-medium for names). Nineteen
  classes, sizes from the new-expressions and constructors, vtables from
  vtscan.py: TButtonsWindow (identifier 5, 0x55e968, 0x58), TAutomapWindow (3,
  0x55ea08, 0x80) with TAutomapView (0x55e688, 0x24, a new slot +0x84 GetMark
  returning a TMark by value) and TCompassRose (0x55eaf0, 0x28),
  TCapacityLabel (0x55e578) and TSoulLabel (0x55e600), TPlayerStates
  (0x55eb98, 0x2c), TSmallInventoryWindow (2, 0x55ec98, 0x44), TInventoryView
  (0x55ed38, 0x2c), TCombatControls (0x55e880, 0x44), TStatusWindow (4,
  0x55f500, 0x4c), the bodies TContainerWindowBody (0x55eed0, 0x44, a new
  slot +0xb8 Update), TSkillsWindowBody (0x55efa8, 0x3c), TBattleWindowBody
  (0x55e710, 0x58), TBuddylistWindowBody (0x55e7c8, 0x3c), TTradeWindowBody
  (0x55f370, 0x3c) with TTradeOffers (0x55f138, 0x34), TTradeNames (0x55f200,
  0x38) and TTradeButtons (0x55f2a0, 0x4c). The identifier 1 window is a
  plain TContentWindow made by CreateInventoryWindow. external.h's comments
  on TSideWindow3/4/5 were right (sizes and vtables). Names from the binary:
  TStatusWindow (SkinProperties.StatusWindow), TPlayerStates
  (SkinProperties.PlayerStates), BuddylistWindow (TBuddylistWindowBody),
  RadioButtonGroup (TRadioButtonGroup), the parameters _CheckObject,
  _RoseSkin, islarge, _Container, EventComponent, NumberOfLine, Value,
  Progress, Position, Direction, Code, the member Lines and the locals
  ScrollbarX, destx/desty, DestX/DestY, ItemX/ItemY and skin; TSkillsWindowBody
  keeps the name of the earlier sketch ("_SkillsWindow"). The other class,
  member and function names (and the Create* creators) are ours.
- **TSubWindowBody** (medium). The base of the bodies has no vtable of its
  own: its constructor is inline (Minimized=false right after TWindow's
  constructor; the subclasses' vtables are stored after it, the base one
  was dropped). Its virtuals are inline: Minimize (+0xa0) and Maximize (+0xa4)
  set Minimized, IsMinimized (+0xa8) returns it, GetMaximumHeight (+0xb0)
  returns INT_MAX, and LeftButtonDoubleClick (+0x58) returns true (the folded
  0x425a00 in every body vtable but the VIP list's: a double click on a body
  does not minimize the window; found by the test). GetInitialHeight (+0xac,
  the creators size the windows with it) and GetContentHeight (+0xb4) are
  pure (every body overrides them). The sketch's UnknownA0-B8 and
  FUN_0042e4e0 are gone: TSubWindow calls Minimize/Maximize, TSubWindow::Arrange
  calls TContainerWindowBody::UpdateUpButton (cast, as the original must),
  MainWindow.cpp's OpenContainer calls the container body's Update (+0xb8)
  and IsMinimized (+0xa8, so far "the body allows it").
- **Limit** (low for the name, medium for the header). 0x425920 clamps
  *Value to Min..Max in either order. ContentWindows.cpp inlines it in
  TTradeWindowBody::SetSize (with std::min/std::max of the maximum and
  minimum heights, each read twice), DialogElements.cpp calls the
  out-of-line copy. A header both include: common.h, next to In.
- **Source order** (medium). Line order (the creators first, lines 106-280).
  The binary moves the creators, the constructors of the bodies,
  TAutomapWindow, TStatusWindow, TSmallInventoryWindow's neighbours and
  TInventoryView's mouse handlers, TPlayerStates' Draw and MouseHover and
  TAutomapWindow::HandleEvent to the end of the object (each calls functions
  defined further down, Creatures.cpp's pattern). Functions without lines
  where the binary has them: TInventoryView::GetSlotAt after HelpMouseMove
  (1344), ~TSkillsWindowBody after its constructor (1980).
- **Exception tables and switches** (high), checked with ehinfo.py: the
  assertions before TBEGIN in TButtonsWindow's constructor (6-10),
  TPlayerStates::GetIcon (5-10), TSkillsWindowBody::GetLine (5-15), both
  Draws of the trade parts (10-10) and ShowAcceptButton (5-6); the
  constructors' try blocks start at state 1; TCompassRose's constructor has
  only the assertion's states; TInventoryView::GetSlotAt and
  ~TSkillsWindowBody have TBEGIN/TEND frames without try blocks; the inline
  ~TBattleWindowBody has the base class's state only. Catch blocks without
  names are TEND (the HelpMouseMove functions, GetOpponentAt, ...); the
  mouse wheels name Code. Jump tables checked: Scroll 0x4276e0 (1-4),
  TPlayerStates::GetIcon 0x429e78 (0-9) and the skull 0x429ea0 (1-4),
  TCombatControls::HandleEvent 0x42c1c8 (100-105), GetLine 0x42fc44 (1-15),
  TAutomapWindow::HandleEvent 0x43716c with the byte table 0x437194 (3, 4,
  106-112), TPlayerStates::MouseHover 0x4378f8 (skins 228-239), the battle
  list's party shields 0x43adf8 and skulls 0x43ae08 (1-4).
- **Odd behaviour kept**: the automap's MouseHover prints its buffer to
  stdout and takes a mark only when both coordinates are not 0; at zoom 1 the
  automap's centre is rounded inside the pixel loop; the container rows start
  at Data630.x in GetInitialHeight/GetContentHeight; a container slot cut at
  the top edge shows the top of its picture; TTradeOffers::GetContentHeight
  adds Data68.x; the skill lines with a bar are 10 pixels taller in Draw but
  9 in MouseHover and GetContentHeight; TBuddylistWindowBody::GetBuddyAt does
  not test y; ShowAcceptButton and Wait delete the wait text or the accept
  button without a NULL check; TCombatControls leaves a radio group without a
  selection for an unknown mode (TRadioButtonGroup::Selected is uninitialised);
  the party shield 248 is drawn only while the player is in no party;
  TStatusWindow's Data44/Data48 (100) are never read; TTradeButtons' +0x2c,
  +0x30 are never used.
- **SkinProperties retyped** (medium; GUI.cpp's stores rewritten by offset,
  guitest.cpp compares the bytes): Style288's Data10-Data50 are ten
  TGUIPoints Data10[10] (the inventory's objects), Data60-DataF0 ten TRects
  Data60[10] (the slot areas), Data100-Data124 ten ints Data100[10] (the empty
  slots' skins); Data470-Data494 are the block PlayerStates (its constructor
  is inlined; the assertion names PlayerStates.NumberOfIcons; the extent is
  a guess: the members TPlayerStates reads); Data4AC-Data548 are two
  TCombatControlsProperties blocks Style4AC/Style4FC (height, width, nine
  button positions; TCombatControls' constructor copies a whole block by
  value); Data670-Data687 are two TRGBColors (the skill bars) and
  BuddylistWindow's Data3C/Data44 two TRGBColors Data3C/Data48 (online and
  offline buddies; Data54 is a new buddy). The other blocks are used as
  GUI.h has them (some TRects and TGUIPoints are used as other shapes;
  comments in ContentWindows.cpp).
- **Surface slots named** (medium): +0x0c SetPixel(Surface, x, y, Color), +0x10
  FillRectangleClipped(Surface, Rect, Color, Clip), +0x18
  DrawHorizontalLine(Surface, x, y, Width, Color), from the arguments pushed
  here; external.h's TSurfaceGL and playertest.cpp's stubs follow.
- **DialogElements.cpp sketches** (external.h, low): TSkinButton gained
  RadioButtonGroup (+0x38), Tooltip (+0x3c) and SetRadioButtonGroup (0x458de0,
  "RadioButtonGroup==NULL"); new: TRadioButtonGroup (0x28 bytes: Selected,
  vector<TSkinButton *> Buttons, Count; inline constructor and destructor),
  TTextButton's second constructor (0x466ed0, skins 12/13 and a tooltip),
  TTextToggleButton (0x467230, vtable 0x561d58, 0x84 bytes, State +0x78; it
  passes its own uninitialised Tooltip member to TTextButton, DialogElements.cpp's
  business), TLabel's second constructor (0x465510, an area) and SetText
  (+0x84, 0x455e40), TSkinIcon (0x457710), TScrollBarRange::SetValue
  (0x460710); TScrollBar's last parameter is a step (int; 1 in
  ConsoleWindow.cpp, which now passes 1 instead of true). All have
  low-confidence names.tsv rows.
- **Callers** changed in names and casts: MainWindow.cpp (the creators,
  TAutomapWindow, TStatusWindow, TButtonsWindow, OpenContainer's Update and
  IsMinimized), DialogWindow.cpp (Minimize, Maximize, UpdateUpButton),
  ContextMenus.cpp/.h (includes ContentWindows.h; TMarkMenuData::Window is a
  TAutomapWindow *), Control.cpp unchanged (TSkillsWindowBody is a forward
  declaration there). ContentWindows.h needs Map.h (TMark), so like
  ContextMenus.h it is included directly by its users.
- **callcheck** (81 of 140 functions differ, all explained): constructors of
  new-expressions and base classes, TCallback temporaries, the inline
  TRadioButtonGroup and TRGBColor(int) and ~TWindow are not lexical calls;
  members called without qualification in constructors (SetSize,
  InsertChild, Arrange) are direct calls in the binary that the tool does
  not resolve; virtual calls (GetParent, Arrange, GetMark, IsMinimized,
  GetContentHeight, GetMaximumHeight, GetMinimumHeight, SetChildPosition,
  SetChildArea, SetText, RightButtonUp, which the tool matches to Control.cpp's)
  are indirect in the binary; hasContentWindow and Limit are inlined;
  Offers->GetContentHeight is resolved to the body's own; the compiler merged
  call sites (SendMoveObject, SetChaseMode, AddMessage, SendUseObject,
  SetStatusMessage, Scroll); Ghidra dropped the GetMouseX/GetMouseY calls in
  the container body's Draw.
- **Tests** (contentwindowstest.cpp, 52,506 checks in the full run over 600 rounds of
  20-60 operations): each round builds one of the ten kinds of windows from
  the image's code and ours (the seven creators, the automap, status and
  buttons windows' constructors) inside a real column (the image's and our
  TDialogWindow), then random sizes, Arrange, drawing, mouse presses (with
  drags), releases, double and right clicks with the modifier flags, the
  wheel, hovering and help mode over random points and the slots, rows,
  icons, marks and the compass rose, the windows' events, the classes'
  members (Scroll, GetMark, GetSlotAt, GetIconAt, GetIcon, GetLine,
  GetOpponentAt, GetBuddyAt, GetOfferAt, Update, UpdateUpButton, the trade
  buttons, the bodies' heights) and changes of the world; logs of every
  callee and dumps of both trees (every member, the DialogElements objects
  with their arguments and state) are compared after each, the logs of the
  destructors at the end. The DialogElements classes are the shared fakes on
  both sides; they now report every object through difftest.h's
  ElementCreated (consolewindowtest.cpp's scroll bar and buttons,
  contextmenustest.cpp's separator, mainwindowtest.cpp's text button, label
  and object icon, and this file's new fakes), and the test gives each a test
  vtable. The image's operator new and ours fill memory with the same
  pattern. The world values the windows set (attack target, modes) have one
  copy per side. A mutation check (fourteen changes, one at a time) failed
  each run. DIFFTEST_ONLY=ContentWindows runs it alone; it restores every
  patch. Other tests: dialogwindowtest.cpp's body overrides the real inline
  slots and redirects UpdateUpButton (both sides) to the shared
  FakeUpdateUpButton; mainwindowtest.cpp redirects the creators and the three
  side-bar constructors (decorated names) to its fakes, its body has a +0xb8
  Update; contextmenustest.cpp uses a same-layout stand-in for the skill
  window body and logs only while it runs; mainwindowtest.cpp's element fakes
  register only while it runs. Not exercised: the error paths, the button
  callbacks (dumped, not sent) and inputs the original fails on as well (the
  counter offer twice, help mode over a hidden body).

## DialogElements.cpp

Reconstructed by Claude Opus 5.5 (issue #7), November 1, 2026.

- **Range** (high). DialogElements.obj is 0x4524e0-0x46b773: the leading run of
  header code settled in an earlier issue (now every function named: the
  inline accessors, destructors with their deleting forms, the inline
  constructors of TObjectIcon, TSeparator and TProgressBar, Container.h's
  vector<int> and vector<TListBoxLine> with their destructors 0x452c50 and
  0x452c60), the line-numbered functions (lines 42-4301) and between them the
  functions without lines where the binary has them; the last catch block
  (TEditText's constructor, line 2466) ends at 0x46b773. The binary moves the
  functions that call functions defined further down to the end of the object
  (the labels', buttons', check box's, edit line's, scroll bars' and list boxes'
  constructors, TFormattedText::Arrange, TCheckBox::SetEnabled, several
  TEditLine, TEditText and TListBox members, TLink's constructor); the source
  follows the line numbers. TEditText's compiler-generated destructor and its
  deleting form (0x46b540, 0x46b520) sit before its constructor (emitted with
  the vtable). 0x469144 is a Ghidra split of TEditText::SetText 0x469120.
- **DialogElements/Dialogs gap** (high): Dialogs.obj's leading run, recorded in
  file-ranges.tsv as Dialogs.cpp (0x46b790-0x46bb5f, 0x46bdf0-0x46c0cf) and
  Container.h (0x46bb60-0x46bdef): inline constructors of Dialogs.h's TLabel
  subclasses (vtables 0x562110-0x562370, whose own slots 0x46db80, 0x46dd40,
  0x46e0e0, ... are Dialogs.cpp code and which Dialogs.cpp also constructs),
  the reconnect text (Dialogs.obj's text table), inline destructors whose
  vtables Dialogs.cpp constructs (0x4733dd, 0x479a70, ...), a label destructor
  that jumps to ~TLabel, the deleting destructors, and the vector of a 31-byte
  element (destructor 0x46bb60, operator() 0x46bb70; its constructor 0x46c0d0
  was Container.h already). Not reconstructed here.
- **Data** (high). DialogElements.obj has no statics, no static initialiser
  (0x559df0 is Control.obj's NoCallback 0x5c4558, 0x559e00 Creatures.obj's
  0x5cc418, 0x559e10 Dialogs.obj's 0x5cc420) and uses none of its copy of the
  text table (0x5c0d8c-0x5c1228, its regular expression pointer 0x5c122c, then
  its DefaultFontColor 0x5c1230, which TFont's constructor reads). Its code
  reads only SkinProperties, Skins and NumberOfSkins.
- **Classes** (high for layouts, vtables and sizes; names see below). 28
  classes and two helper types; sizes from the new-expressions, vtables from
  vtscan.py, the overrides from comparing every vtable with TElement's and
  TWindow's (build/scratch/de7/vtdiff.py): TSeparator (0x5610b0, 0x28),
  TResizer (0x5612b0, 0x38), TBorder (0x561358, 0x68), TLabel (0x560bb8, 0x4c,
  new slot +0x84 SetText), TFormattedText (0x560c40, 0x38, +0xa0 SetText),
  TLink (0x560ce8, 0x3c), TOutfitIcon (0x561970, 0x48), TSkinIcon (0x5614f8,
  0x28), TObjectIcon (0x560d88, 0x50), TSkinButton (0x5619f8, 0x44, +0x84
  IsPressed) with TColorButton (0x561a80, 0x50), TIconButton (0x561b08, 0x48),
  TBlinkingButton (0x561b90, 0x5c), TTextButton (0x561c40, 0x78, +0x88
  SetText), TToggleButton (0x561cd0, 0x50), TTextToggleButton (0x561d58,
  0x84), TMultiStateButton (0x560e10, 0x58), TRadioButtonGroup (inline),
  TCheckBox (0x561de8, 0x34, +0xa0/+0xa4), TEditLine (0x560e98, 0x68, slots
  +0xa0-+0xc4), TPasswordEditLine (0x560f60, 0x6c), TEditText (0x562040,
  0x9c, new +0xc8 SetActive and +0xcc SetText(text, KeepPosition)),
  TFocusGroup (no vtable, 0x28), THorizontalScrollBarRange (0x5617e0, 0x54,
  with a TCallback), THorizontalScrollBar (0x561ed0, 0x38), TScrollBarRange
  (0x561868, 0x4c), TScrollBar (0x561f70, 0x38), TListBox (0x561138, 0x7c,
  +0xa0 StartEdit, +0xa4 StopEdit), TFontListBox (0x5611e0, 0xb0) with
  TListBoxLine (0x18 bytes, formerly "TDialogText") and TProgressBar
  (0x561028, 0x2c). The two scroll ranges are separate classes: their Pressed
  and Dragging flags sit in swapped bytes (+0x38/+0x39), and only the
  horizontal one has a callback.
- **Names**. From the binary: TCheckBox (SkinProperties.CheckBox), TFocusGroup
  (_FocusGroup), getMinimumWidth/getMinimumHeight (see below), the members
  Text, Cursor, MaxLength, FocusGroup, MemberList, Active, FieldWidth, Lines,
  RadioButtonGroup, ToResize, the locals Next, insertlength, appendlength,
  Width, and every parameter named in an assertion or catch block. Kept from
  the sketches (low): TResizer, TBorder, TLabel, TFormattedText, TSkinIcon,
  TObjectIcon, TSkinButton, TToggleButton, TMultiStateButton, TTextButton,
  TTextToggleButton, TBlinkingButton, TEditLine, TScrollBar, TScrollBarRange,
  TSeparator. New and ours (low): TLink, TOutfitIcon, TColorButton, TIconButton,
  TPasswordEditLine, TEditText, THorizontalScrollBar(Range), TListBox,
  TFontListBox, TListBoxLine, TProgressBar; members without evidence are ours
  (Pressed, MouseInside, Repeat, Enabled, SingleLine, Disabled, ReadOnly,
  SelectionStart/Length, FirstVisible, ...). Corrected sketch rows:
  TSkinButton's last parameter is Repeat (a held button resends its callback),
  TTextButton's last parameter of the first constructor is a tooltip, TLabel's
  flag is Shorten, TObjectIcon's flag Framed, TEditLine's flag ReadOnly, the
  console's SetReadOnly (+0xb8) is SetDisabled (+0x65; ReadOnly is the
  constructor's +0x66), TBorder::FUN_00455500 is FitToWindow, TScrollBar's
  parameters are (Min, Max, Value, Visible, ButtonRange).
- **getMinimumWidth/getMinimumHeight** (high): the assertions of
  TBorder::SetSize name "Width>=getMinimumWidth()" and
  "Height>=getMinimumHeight()" (lines 242-243), so GUI.h's virtuals +0x3c/+0x40
  are renamed from GetMinimumWidth/GetMinimumHeight in every file, test and
  names.tsv row.
- **Header code** (medium). The out-of-line copies at the start of the object
  are inline in DialogElements.h; several are called only by later objects
  (TOutfitIcon::SetOutfit, TSkinButton::SetEnabled, TListBox::GetSelection and
  SetEditable from Dialogs.cpp; TMultiStateButton::Reset from ConsoleWindow.cpp;
  TEditLine::HasSelection's copy is ConsoleWindow.obj's 0x417520). The folded
  "return false" slots 0x452620 (x, y) and 0x457700 (x, y, Flags) are inline
  overrides of TLabel and the three icons (TSkinIcon also for MouseWheel). The
  scroll bars' KeyPress slot is 0x4731d0, ICF-folded with a Dialogs.cpp
  function of the same code (Range->KeyPress(Key)). 0x4672c0 is GUI.h's
  implicit ~TElement (jumps to ~TComponent), kept here.
- **Exception tables and switches** (high), checked with ehinfo.py for every
  function (build/scratch/de7/ehcheck.py compares the assertions before TBEGIN
  with the try ranges): constructors' try blocks start after the base class,
  the vector members and the assertions; the frames without try blocks (eleven
  states: Arrange of the separator, resizer, frame, check box, edit line,
  ranges and progress bar, TSkinButton::MouseMove, the IsPressed functions,
  TEditLine::SetActive) are TBEGIN/TEND around code that cannot throw; the
  functions with only assertions and no TBEGIN are noted. tools/ehinfo.py now
  also finds the prologue PUSH -1; MOV EAX, FS:[0]; PUSH stub (0x45dfc0,
  TFocusGroup::AddMember, and a few Container.h instances). The link's and the
  password line's HandleEvent name Event with a constant in their catch
  blocks: their try blocks are inside the test of the event. Jump tables:
  GetToken 0x4567ec (states 0-7, 2-4 unused), TFormattedText::Arrange 0x466650
  (token types 0-3, 10-18), TEditLine::HandleEvent 0x45bc6c (16-20),
  TEditText::KeyPress 0x4690d0 with the byte table 0x4690e4 and 0x4690f0.
- **Odd behaviour kept**: TEditLine::RightButtonUp calls TWindow::LeftButtonUp;
  InsertText inserts at the cursor (Position enters only the assertion and the
  rest's length); select all puts the cursor at max(length, 0); a backspace
  with a selection moves the cursor before deleting it; TEditText::Arrange and
  TListBox::Arrange size the scroll bar with skin widths (11, 10) where skin
  heights would fit; TEditText's PageUp moves one line more than a page and
  reads LineStart(-1) on the first line; TFocusGroup::AddMember does not test
  the last member for a duplicate; TToggleButton and TTextToggleButton pass
  their own unset Tooltip to the base class; TMultiStateButton::Reset sets
  State past the last state (it wraps to 0); a '<' that starts no token
  advances TFormattedText's layout twice; a failed <name=...> token keeps its
  type for the next piece; TCheckBox::SetEnabled rebuilds the label from its
  first line only; TObjectIcon's frame only narrows the clipping area; the
  horizontal range sends its callback before limiting the value in KeyPress
  and twice for a held button beside the slider; TListBox::Draw stops at
  VisibleLines or Lines lines; TFontListBox::Clear frees the texts and keeps
  the pointers; TOutfitIcon keeps a direction it never draws.
- **SkinProperties retyped** (medium; GUI.cpp's stores rewritten, guitest.cpp
  compares the bytes): CheckBox.Data08/Data10 are one TRect Data08 (the area
  TCheckBox fits its label into). Surface.h's slot +0x1c is DrawVerticalLine
  (the edit line's cursor), external.h's TSurfaceGL and playertest.cpp follow.
- **Callers** (high): external.h's sketches are gone; DialogElements.h includes
  external.h (TObject), Status.h and Skins.h, so its users include it
  directly (ConsoleWindow.cpp, ContentWindows.h, ContextMenus.cpp,
  DialogWindow.cpp, MainWindow.cpp, MapWindow.cpp; Control.cpp needs only
  ContextMenus.h's forward declaration). Renamed: Input->SetDisabled,
  SelectionStart/SelectionLength/ReadOnly in ConsoleWindow.cpp and
  ContextMenus.cpp, Frame->FitToWindow in MainWindow.cpp; the placeholder
  literals became false (Repeat, Framed, ReadOnly, State) and NULL (tooltips).
- **callcheck** (all differences explained): constructors of base classes,
  members and new-expressions (TElement, TWindow, TCallback, TDrawing's
  vector constructor iterator, the vectors, the controls' parts) and
  Container.h's operator() are not lexical calls; virtual calls (Arrange,
  IsPressed, IsDisabled, IsChecked, GetPosition, the edit members, StartEdit
  and StopEdit, ReleaseCapture, SetSize, getMinimum*, SetChildArea,
  SetChildPosition, ClipChildren) are indirect in the binary; the binary
  inlines Limit, SetCursor, LimitCursor and SetSelection in places and calls
  them in others; calls of a member of another class with the same name
  (SetActive, SetValue) are resolved to the own class by the tool; OpenURL is
  a function pointer; two SetRange calls of an if/else are one call site in
  the binary; _ftol (0x5124c4) is the compiler's float conversion.
- **Tests**. dialogelementstest.cpp (new; 135,478 checks in the full run over 1,600
  rounds): every round builds one of 27 kinds
  of controls on both sides (the image's constructors at their addresses,
  ours with new) as a child of a root window, three edit lines of a focus
  group or one with a radio group, then 20-60 random operations (sizes,
  Arrange, drawing, mouse moves, clicks with drags and held-button ticks,
  double and right clicks, the wheel, hovering, keys, events, and each
  class's members, the header's inline members against their out-of-line
  copies); the logs of every callee with an effect (text and picture drawing,
  the surface, the mouse-over text, cursor, messages, clipboard, callbacks and
  OpenURL) and a dump of every member of the control and its children are
  compared after each. The metrics, drawing and world are fakes for both
  sides; the image's operator new and ours fill memory with a pattern. A
  mutation check (twenty changes, one at a time) failed each run.
  DIFFTEST_ONLY=DialogElements runs it alone (DE_TRACE, DE_LOGS).
  The earlier window tests keep their fakes as mirror classes of the old
  sketches (tools/difftest/elementfakes.h): PatchElementFakes redirects the
  image's and our constructors (decorated names through /alternatename) and
  non-virtual members to them while each test runs. TObjectIcon's and
  TSeparator's constructors and TMultiStateButton::Reset are inline (inlined
  into our callers), so they run for real on both sides: contextmenustest.cpp
  gives the real separators its test vtable after each construction,
  contentwindowstest.cpp and mainwindowtest.cpp name the real icons and
  separators by vtable (mainwindowtest.cpp fakes DrawObject now).


## SurfaceDX.cpp (issue #9)

- **Object boundary (high)**: DirectDraw is 0x4eae70-0x4efd0f, not just the
  previously settled leading range ending at 0x4ed19f. Vtable 0x56f038
  identifies the functions Ghidra missed, including the pixel writers,
  AttachWindow, both stretch wrappers, the clipped blit wrappers, Init,
  InitDisplay and GetSurfaceRect (assertion line 1411, [bug0000290]). The
  generated source map counts only functions present in the raw decompile;
  the missing entries are still named in names.tsv and implemented from
  instructions, not inferred from that count.
- **The following gap is Direct3D 9 (high)**: 0x4efd10 loads d3d9.dll and
  stores Direct3DCreate9 at 0x71d278; 0x4f1430 and 0x4f03f0 install
  vtable 0x56f480. Assign 0x4efd10-0x4f1ea4 to surfaceDX9.cpp for its own
  reconstruction issue. Do not implement that engine in this issue.
- **Interfaces (high)**: the engine is called DirectX 5, but its GUIDs and
  calls use IDirectDraw4, IDirectDrawSurface4 and IDirect3D3 / Device3 /
  Viewport3 (DirectX 6 interfaces). Use the installed SDK declarations,
  ddraw.lib, dxguid.lib, winmm.lib and user32.lib, not handwritten COM shims.
  d3dtypes.h supplies the empty D3DTLVERTEX constructor at 0x4eae70;
  D3D_OVERLOADS enables it. Its names.tsv kind is stl (SDK/library code).
- **Class and names (medium)**: TSurfaceDX is inferred from the object name.
  Surface, Rect, DirectDraw, Direct3D, hWindow, ViewMode, VMode, GMode,
  FromRect.width/height, ToRect.width/height and the surface-index constants
  come from assertions. The other member/parameter names are ours.
  TBlitSize is a local two-int view solely to retain the asserted lower-case
  width/height names while keeping the existing TRect drawing ABI; it adds
  no binary function or wrapper. The factory is CreateSurfaceDX (0x4eca00):
  its allocation is 0x250 bytes with an inlined empty derived constructor.
- **Layout (high)**: TSurface is 0x70 bytes, not the former 0x44 sketch.
  Doubles at +0x48/+0x50/+0x58/+0x60 give it 8-byte alignment, making +0x04
  padding, not a member. hWindow is still +0x08; mode, size, availability,
  counts and frame timing have real fields. TSurfaceDX adds Recreated at
  +0x70, DirectDraw at +0x74, WindowRect at +0x78, Surface[22] at +0x88,
  Rect[22] at +0xe0 and the three Direct3D pointers at +0x240/+0x244/+0x248.
  +0x0c/+0x20/+0x44/+0x6c/+0x24c remain offset names (low confidence).
  TSurfaceGL retains total size 0x498 by shortening its opaque tail.
- **Folded members (high addresses, medium names)**: eight overrides are
  shared with neighboring engines: ClipRect 0x4f0410, ResetFrameTimer
  0x4f04d0 and SurfaceRecreated 0x4f13e0 (DX9); DrawFrame 0x4fa110,
  DrawFrameClipped 0x4fa2c0, GetCacheSurfaces 0x4fb5e0, IsUnavailable
  0x4fb5f0 and GetFPS 0x4f9d00 (OpenGL). TestEngine is the return-true body
  folded with codecvt at 0x4d7d40; unlike TSurface's implementation it
  does not store the main window. No extra thunk is reconstructed.
- **Shared data ownership (medium/low)**: all engines reference the blit
  counters/statistics and InitStep; place them with the engine-independent
  Surface.cpp data, spanning 0x71d0bc-0x71d0f4, and declare them in Surface.h.
  This revises the earlier boundary argument: 0x71d0f0 is the shared
  NumberOfSurfaces, not the first private DirectDraw datum. The private
  SurfaceMemory[22] starts at 0x71d0f8, followed by WaitForNextFrame's
  Frames at 0x71d150 and LastSecond at 0x71d158. No dynamic static
  initializers are needed for these POD/BSS values. CacheSurfaceWidth and
  CacheSurfaceHeight at 0x5c2568/6c are shared, initially 256 and changed
  to 320 by InitSurfaces. The private DeviceGUID pointer at 0x5c2570
  selects IID_IDirect3DHALDevice. Exact source ownership of shared globals
  is low confidence, but their addresses, types and common use are verified.
- **EH and switches (high)**: AttachWindow has 15 unwind states,
  StretchBlitUnclipped/StretchBlit 30, Blit/BlitRect/GetSurfaceRect 5,
  InitDisplay 20 and CreateSurfaces 45, all with zero try blocks: assertions
  without TBEGIN. ResetDisplay alone has a PROGRAM_ERROR catch and a
  nested catch-all protecting cleanup. The six-way assertion prologues
  interleave an argument load with the FS:[0] setup; ehinfo.py now accepts
  these and validates the FuncInfo magic. Init's switch is a compare chain
  (0, windowed, fullscreen), not a jump table; the whole object contains
  no indexed indirect jump to a switch table.
- **Odd behavior retained**: Locks set dwSize to 0x6c inside a zeroed
  0x7c-byte descriptor. The image writers leave sub-16-bit surfaces locked.
  A zero RGB mask loops forever in GetPixelColor; valid formats avoid it.
  DirectDrawCreate's initial interface leaks. A failed Direct3D setup only
  releases Direct3D and decrements NumberOfSurfaces even if its render
  target was not allocated. GetDisplayMode/GetSurfaceDesc descriptors are
  not zeroed. Shading uses the original float 1/255 factor promoted to
  double before truncation; do not algebraically simplify it. The mode
  callback tests DDSD_REFRESHRATE in ddpfPixelFormat.dwFlags, not dwFlags
  (0x4ec869); preserve this misplaced-flag test rather than correcting it.
- **Callcheck audit**: 14 of 43 blocks differ. The destructor/base-constructor
  calls are implicit (0x4eae90, 0x4eca00); the parser mistakes COM's
  SetColorKey for TSurface::SetColorKey (0x4eafb0). Vtable dispatch is
  absent from Ghidra's named-call counts at 0x4ebd20/0x4ebe60 (ClipRect,
  SetSourceColorKey), 0x4ec220 (WaitForNextFrame), 0x4ec600 (InitDisplay,
  CreateSurfaces, cleanup), 0x4ec690 (cleanup/reset), 0x4ec9e0 (cleanup),
  0x4ecdd0 (FillRectangle), 0x4ecfd0 (timer/framerate) and 0x4eed40
  (AttachWindow). AddFullscreenMode's 8 versus 9 static GetSurface calls
  are the compiler's duplicated loop-condition test; dynamic calls agree.
  ShadeRectangle's SDK vertex-array constructors and twelve x87 _ftol
  conversions are expressed by C++/D3DRGB, not explicit helper calls.
- **Caller audit**: Surface.cpp now calls CreateSurfaceDX; the other engine
  factories stay external. graphics.cpp's five +0x50 calls become
  SetColorKey; the +0x64 window attachment in Control.cpp and +0x9c
  recreation checks in ContentWindows.cpp/MapWindow.cpp retain their
  arguments and slot offsets. Sketches/fakes follow the retyped slots.
  Naming the base CacheSurfaces member exposed a collision with
  graphicstest.cpp's fake counter; rename that counter GraphicsCacheSurfaces
  so its GetCacheSurfaces override still returns the configured test value.
- **Integration with issue #8**: keep its member GetAvailableEngines and
  the shared +0xac name SetFramerateLimit (our initial name was
  SetFramerate). Both names are inferred; the binary clamps values below
  10 or above 200 to 0xffff. There is one implementation, no alias or
  compatibility wrapper. The new SurfaceDX layout and slot signatures
  are used by the existing engine sketches and all test surfaces.
- **Differential coverage**: surfacedxtest.cpp runs every callable object
  function and folded override through real reconstructed/image code on
  independent fake DirectDraw4/Surface4/Clipper and Direct3D3/Device3/
  Viewport3 pools. Imports, preferences and the clock are scripted before
  either side runs. After each operation compare call logs, all 0x250
  object bytes (pointers normalized), thirteen shared globals, per-surface
  memory kinds, COM state and pixels; shading includes every vertex word.
  All code and IAT patches are restored. The standalone run passes
  1,213,828 checks over 16,089 operations of 56 kinds, with 269 coverage
  points. DX_LOGS writes compared logs; DX_TRACE prints phase coverage.
  Coverage includes every mode transition, forced/recovered displays,
  Direct3D setup failures at steps 2-14, cache/video-memory exhaustion,
  busy/still-drawing retries, both shading outcomes, 8/16/24/32-bit
  formats, the misplaced refresh flag and the 32-bit clock wrap.
  Assertion and PROGRAM_ERROR throw paths cannot run in the mapped
  image without its original CRT initialization; EH and instructions
  establish their behavior instead. GetAttachedSurface initializes only
  DDSCAPS2.dwCaps, so the test deliberately does not read its other words.
- **Mutation sensitivity**: eighteen isolated changes were all detected:
  frame-limit boundary, frame schedule, pixel offset, rectangle fill size,
  clipped-fill statistics, horizontal/vertical line size, RGB input stride,
  mask grayscale coefficient, color key, stretch source size, vertex color,
  offscreen memory kind, 3D surface caps, refresh flag location, mode count,
  surface rectangle width and recreation flag. The initial caps mutation
  removed DDSCAPS_3DDEVICE and crashed the fake world after diverging;
  rerunning it with an extra DDSCAPS_LOCALVIDMEM instead produced 4,371
  ordinary comparison failures. All eighteen targets therefore have
  explicit mismatch evidence; no target escaped. Restore the source after
  each mutation and rebuild the unmutated test before final validation.
- **Final validation (November 2, 2026)**: after integrating issue #8
  from origin/main, tools\compile-check.cmd is warning/error-free and
  tools\difftest\build.cmd passes 13,955,244 differential checks,
  67 sysinc checks and 3,201 arithmetic checks, all with zero failures.
  SurfaceDX contributes 1,213,366 checks in that full run; its separate
  deterministic run passes 1,213,828 checks. The full-suite random stream
  changes its input distribution and totals, not the tested code.

## Dialogs.cpp

Reconstructed by Claude Opus 5.5 (issue #8), November 2, 2026.

- **Range** (high). Dialogs.obj is 0x46b790-0x4854ad: the leading run of
  header code settled in issue #7 (now every function named, see below), the
  line-numbered functions (lines 48-4084) and, without lines, the inline
  virtuals and compiler-generated destructors the binary emits with the
  vtables. The last catch block (the rule violation creator's, line 490)
  ends at 0x4854ad; DialogWindow.obj starts at 0x4854b0 (issue #3). No header
  code or Container.h instance follows the last function. The binary moves
  the creators (lines 48-631), the constructors that call functions defined
  further down (TDialog's 0x47c970, the outfit's, the edit list's, the ignore
  list's, the hotkeys', the error report's, the rule violation's) and the
  rule violation's HandleEvent (line 3124, which calls UpdateActions of line
  3158) to the end of the object, Creatures.cpp's pattern; the source
  follows the line numbers. Ghidra missed 0x46bb00, 0x46bef0, 0x4719a0,
  0x47cf50, 0x47cf60 and 0x47db50 (vtable slots and a qsort comparator).
- **Leading run** (high for the code, medium for the names): the inline
  constructors of TQuestionLabel, TLogoutLabel and TWaitLabel (0x46b790,
  0x46b840, 0x46b8f0; the first two take the font by reference and copy it
  into TLabel's), the inline destructors of TWaitLabel, TReconnectWindow,
  TCharacterListDialog, TNewChannelDialog, TGraphicsOptionsDialog,
  TEditBuddyDialog, TRuleViolationDialog, TEditMarkDialog,
  TAdvancedGraphicsDialog, TLoginDialog and TOutfitDialog (each deletes its
  timer, its array or its radio/focus groups), TReconnectWindow::SetTime
  (0x46ba10, "Trying to reconnect in %d %s."), TEditBuddyDialog::KeyPress
  (0x46bef0), the compiler-generated ~TQuestionLabel (0x46bdf0, jmp ~TLabel,
  shared with TLogoutLabel) and the deleting destructors; CompareNames
  (0x46bb00) is the ignore list's qsort comparator (_stricmp of two char *),
  an inline function of the header by its place (low). Container.h's
  vector<TIgnoreEntry> (destructor 0x46bb60, operator() 0x46bb70,
  constructor 0x46c0d0) clears Name[0] and Permanent of every new entry and of
  init: TIgnoreEntry has an inline default constructor (medium; in
  HandleEvent the store of Name[0] before CopyString is missing, which
  suggests whole-program optimisation).
- **Data** (high). Dialogs.obj's only static is NoCallback 0x5cc420
  (initialiser 0x559e10; 0x559e20, the TCallback 0x5cc428, is DialogWindow.obj's,
  as issue #3 found). Its copy of the text table is 0x5c123c-0x5c1484, then its
  regular expression pointer 0x5c1488 and DefaultFontColor 0x5c148c; the
  entries it reads are declared in external.h (names ours) and the test reads
  the image's texts into them. The code also reads SkinProperties, Skins and
  curEngine 0x71d0c4.
- **Classes** (high for layouts, vtables and sizes; names ours, low, unless
  noted). The frame TDialog (vtable 0x563778, 0xc4 bytes; the assertions name
  the parameters _Caption and _Dialog and the members OkButton, CancelButton
  and ExtraButton, so the body is "the dialog" and the frame is named TDialog)
  and 31 bodies, each a TWindow (or a label) with its own KeyPress and
  HandleEvent: TQuestionLabel 0x562110, TLogoutLabel 0x562198, TWaitLabel
  0x562220, TReconnectWindow 0x5622b0, TLoginDialog 0x5627d8,
  TCharacterListDialog 0x562370, TMoveObjectsDialog 0x562b68 (the member
  DraggedObject is named in an assertion), TOutfitDialog 0x562878,
  TTextDialog 0x562c28, TEditListDialog 0x563868, TIgnoreListDialog 0x5639d8,
  TBugReportDialog 0x562cd8, TNewChannelDialog 0x562418, TPrivateChannelDialog
  0x562db8, TReportDialog 0x562e58, TAddBuddyDialog 0x562ef8,
  TEditBuddyDialog 0x562558 (member CharacterID from an assertion),
  TUpdateDialog 0x562ff0, THelpDialog 0x5630c0, THintsDialog 0x563160,
  TRuleViolationDialog 0x5625f8, TEditMarkDialog 0x562698, TOptionsDialog
  0x563258, TGeneralOptionsDialog 0x563360, TGraphicsOptionsDialog 0x5624b8,
  TAdvancedGraphicsDialog 0x562738, TConsoleOptionsDialog 0x563598,
  TNetworkOptionsDialog 0x563650, THotkeyDialog 0x563a90 and
  TErrorReportDialog 0x563b30. Sizes from the creators' new-expressions. A few
  members are never touched and kept as UnusedNN (the move dialog's +0x58, the
  new channel dialog's +0x34/+0x38, the update dialog's +0x2c).
- **Creators** (names ours, medium): CreateMessageDialog ... CreateErrorReportDialog
  (lines 48-631), with the parameter names of the catch blocks (Timeout,
  CharacterID, DataSize) and typed parameters for external.h's placeholders:
  the receivers of the connecting, question and logout dialogs are TComponent
  pointers, the outfit, text and house-list dialogs take int arrays, the new
  channel dialog the channel IDs and an array of 30-character names, the rule
  violation dialog a name and a TTalkLine, the mark dialog a TCoordinate and a
  TMark (deleted by the dialog). The move and report creators return NULL
  when they make no dialog. Control.cpp's 40 call sites call them with casts
  of its INT_PTR parameters.
- **Exception tables and switches** (high): ehinfo.py for every function;
  build/scratch/dg8/ehcheck.py finds the assertions before TBEGIN in every
  constructor (TDialog's three, the outfit's, the text dialog's, the edit
  list's two, the new channel dialog's two, the hotkey dialog's UpdateLine)
  and flags only the ignore list's constructor, whose try block starts after
  the base and its two vector members (state 3), as in the source. The update
  dialog's HandleEvent names Event with the constant 2 in its catch block: its
  try block is inside the test. The only jump table is the outfit dialog's part
  names (0x47d648, cases 0-3); the other switches (the help, options, general
  options, rule violation, hotkey and graphics key handlers) are compare chains.
  Ghidra flattened the control flow of many handlers (the move dialog's
  KeyPress, the reconnect and character list events, the new channel dialog's
  Event = 301, the ignore list's event chain): they were rebuilt from the
  assembly.
- **Odd behaviour kept**: the frame's bottom edge starts at Skins[45].height;
  the OK button's own callback stays NoCallback when the creator passes it
  (only Return gets (this, 1)); the new channel dialog's Return sends 302 with
  a name (nothing but closing; the frame's OK button takes Return first);
  "Exclude player" has an "Add" button; the outfit dialog's part loop runs
  over 216 colours (reading past the 133 buttons); the ignore list's Delete
  loops stop one short of the vector's last index and its selection event
  computes the entry twice; the hotkey dialog's event 11 without a selection
  reads before its texts; the move dialog computes the cumulative pattern and
  does not use it; the help menu's Client Help closes before it asks; an
  incomplete rule violation report still closes the dialog; the advanced
  options' selection is reset to the current engine on every draw; the hints
  and help dialogs are not arranged at the end of their constructors.
- **Side changes** (high unless noted): TObject's member Data1 is Data
  (assertion text "In(DraggedObject.Data,1,100)"; every file and test);
  TBorder frames a TComponent (the outfit dialog frames its TOutfitIcon;
  DialogElements.cpp/.h and elementfakes.cpp's decorated names);
  TListBox::GetNumberOfLines is inline in DialogElements.h (its out-of-line
  copy is folded with Talk.h's TChannelTalkBuffer::GetChannelID 0x507060);
  TSurface's slots +0x8c and +0xac are GetFullscreenModes(TFullscreenMode **)
  and SetFramerateLimit(int) (the arguments pushed), and GetAvailableEngines
  is a non-virtual TSurface member (Dialogs.cpp calls it with the surface in
  ECX; Surface.cpp, external.h's TSurfaceGL and playertest.cpp follow);
  external.h's dialog placeholders are gone.
- **SkinProperties retyped** (medium; build/scratch/dg8/retype.py relayouts the
  blocks, checks that the constructor-zeroed bytes stay the same and rewrote
  GUI.cpp's 260 stores by offset; guitest.cpp compares the bytes): the pairs of
  points Dialogs.cpp reads as one rectangle are TRects (SetChildArea's areas)
  in TSkinProperties (0xbac-0x1518, see GUI.h), StyleB08 (the caption, the
  separator and the body rectangle, which FitToChild takes by value, and the
  three button positions as an array), StyleD3C (the ignore list: relaid
  completely; its colour of permanent entries is Data88), StyleE68, StyleF8C,
  Style1114, Style1274 (the label positions are points), Style1084, Style1394;
  0xc88 is an array of four points (the outfit parts), 0x1064 a TRect that is
  zeroed and never read, 0x14d4 the colour of hotkeys sent automatically.
- **SetChildArea takes a TRect by value** (not changed): every caller in
  Dialogs.obj builds a 16-byte block (sub esp,0x10) for the area, so the
  original signature is SetChildArea(TComponent *, TRect); GUI.h's five-int
  form has the same stack layout, so the source keeps it (issue #27).
- **callcheck** (all differences explained): constructors of base classes,
  members and new-expressions (TWindow, TCallback, TTimer, TLabel, the
  controls, the vectors, TDrawing's vector constructor iterator) are not
  lexical calls; virtual calls (SetChildArea, FitToChildren, SetChecked,
  IsChecked, SetDisabled, IsDisabled, StartEdit, StopEdit, HandleEvent,
  SetTimeout) are indirect in the binary, and calls on this in constructors are
  direct in the binary but resolved to their own class by the tool; members
  whose name several classes share (GetLine, AddLine, SetLine, Clear,
  SetEnabled, SetActive, InsertChild) are not resolved by the tool; the binary
  merges equal tails (SetActionState, AddMessage, Send) and unrolls the
  rule violation's loops over the seven actions (CheckRight, GetActionName,
  AddLine, GetLine); Limit is inlined; OpenURL and SetDisplayMode are function
  pointers that the tool matches with MainFrm.cpp's functions of the same
  names; OutfitColor's GetOutfitColor call is counted for the move dialog's
  HandleEvent (the tool splits at "// 0x" comments); atol is Ghidra's _atol;
  the error report's stringstream calls are library code.
- **Tests**. dialogstest.cpp (new): every round opens one of 34 dialogs on
  both sides through its creator (the image's at its address, ours directly),
  the DialogElements controls real on both sides, then 20-60 random
  operations (drawing, keys, clicks on the dialog's components with drags,
  double and right clicks, the wheel, hovering, caption drags, the body's own
  events and the frame's, Arrange, the timers, the frame's button setters and
  each body's members against their image copies: SetTime, SetTimeout,
  SetTimer, UpdateActions/GetReason/GetAction, UpdateLine, selections and names
  through the controls' members); the logs of every callee with an effect and
  a dump of the frame's tree (geometry, drawing lists, every member word with
  pointers named, strings for the string members, timers and groups) are
  compared after each. The callees are fakes answering from a random world;
  Preferences.cpp's has one copy per side (the dialogs set and read them in
  one operation). The image's error report constructor cannot run in the
  child (its std::stringstream needs the CRT's static initialisers, which call
  imports the child does not resolve): TestErrorReport runs our constructor
  twice and gives the second object the image's vtable, so its KeyPress,
  HandleEvent and destructor are the image's. CompareNames is checked against
  0x46bb00. A mutation check of 22 changes failed each run (the update
  dialog's two-second close needed small clock steps). 98,889 checks in the
  full run.
  DIFFTEST_ONLY=Dialogs runs it alone (DG_TRACE, DG_LOGS).

## surfaceDX9.cpp

Reconstructed by Claude Opus 5.5 (issue #10), November 2, 2026, in parallel with SurfaceDX.cpp (issue #9,
PR #28, merged first into this branch).

- **Range** (high; agreed on #9 and #10). surfaceDX9.obj is 0x4efd10-0x4f46ef: the whole former
  SurfaceDX/surfaceDX9 gap and the code after the asserted 0x4f2040 that Ghidra had not split. Evidence:
  TSurfaceDX9's vtable 0x56f480 (set by the factory 0x4f1430 and the destructor 0x4f03f0) points into
  that range; its .rdata starts with "Direct3DCreate9"/"d3d9.dll" (0x56f458, used only by 0x4efd10 and
  0x4efd40) right after SurfaceDX.cpp's last string 0x56f428, and ends with 0x56f6ac (0x4f4560); its .bss is
  0x71d170-0x71d28f (SurfaceDX.cpp's own ends at 0x71d158, SurfaceGL.cpp's starts at 0x71d290); .data
  0x5c2574 (CurrentTargetNumber = -1). 0x5c2578 (256) is referenced by nothing; it is left out (owner
  unknown). Only Surface.cpp calls into the range (0x4efd10, 0x4f1430). #9 had recorded 0x4efd10-0x4f1ea4 as
  surfaceDX9.cpp; the row now covers the whole object.
- **The surfaceDX9/SurfaceGL gap 0x4f46f0-0x4fb7ef is SurfaceGL.obj** (high), recorded in file-ranges.tsv
  for issue #11 and not reconstructed: .rdata only from 0x56f6e0 (the WGL_/GL_ extension names of
  glh_extensions.h, whose code is 0x4f46f0-0x4f9260), .bss only from 0x71d290 (the extension pointers),
  TSurfaceGL's constructor 0x4fb670 and destructor 0x4f9860 (vtable 0x573250), "OpenGL works" 0x4f9710.
- **Folded copies** (high). Seven of TSurfaceDX9's slots point to identical code the linker kept from
  another object: DrawFrameClipped 0x4fa2c0, DrawFrame 0x4fa110, BlitClipped 0x4fab70, GetCacheSurfaces
  0x4fb5e0, IsUnavailable 0x4fb5f0, GetFPS 0x4f9d00 (SurfaceGL.obj) and SetFramerateLimit 0x4eb000
  (SurfaceDX.obj); they are written at the end of surfaceDX9.cpp, like TSurface::AttachWindow. The other
  way round, TSurfaceDX's ClipRect, ResetFrameTimer and SurfaceRecreated are surfaceDX9.obj's 0x4f0410,
  0x4f04d0, 0x4f13e0 (names.tsv has the TSurfaceDX9 names; #9's TSurfaceDX rows for those addresses were
  replaced in the merge), and the display mode callback of GetFullscreenModes is SurfaceDX.obj's
  AddFullscreenMode 0x4ec810: surfaceDX9.cpp has its own static copy (EnumModesCallback, identical code).
- **Interfaces** (high). Direct3D 9 is loaded at run time (IsDirect3D9Installed: LoadLibraryA,
  GetProcAddress "Direct3DCreate9"; the module handle is never kept, so ExitDisplay's FreeLibrary is dead
  code, kept). DirectDraw is the imported DirectDrawCreate, queried for IDirectDraw4 (the IID at 0x574204,
  as in SurfaceDX.cpp), used only for the display modes, the stretch capabilities and the desktop depth;
  the first IDirectDraw is never released (kept). The interfaces, D3DPRESENT_PARAMETERS, DDCAPS and
  DDSURFACEDESC2 are the Windows SDK's d3d9.h and ddraw.h: the issue assumed the SDK headers were missing,
  but the installed Windows 10 SDK has both, so no shim header was written (the method order is checked by
  the test's fakes, which have only the slots the code uses).
- **Class** (layout high, names medium/low). TSurfaceDX9, 0x2c0 bytes. Names from the assertions:
  Direct3D (+0x1f4), DirectDraw (+0x1fc), Surface9[] (+0x200: the textures; "Surface9[PRIMARY_SURFACE]==NULL"
  reads +0x200), Rect[] (+0x74, Windows RECTs: "Rect[TEMP_SURFACE].right"), hWindow and ViewMode (TSurface),
  VMode, GMode, ToSurface, FromSurface, Surface, NUMBER_OF_SURFACES_DX9 (22) and the surface constants,
  whose values come from the comparisons (ToSurface!=SCREEN_SURFACE is `cmp 1`, FromSurface!=TEMP_SURFACE
  `cmp 2`, STRETCH_SURFACE 3; the CreateSurfaces assertions check +0x200 + 4*N in the order PRIMARY, SCREEN,
  TEMP, CACHE, AUTOMAP, SPRITE). Ours: Device, RenderTarget[] (level 0 of Surface9[]; SCREEN_SURFACE's is the
  back buffer), CurrentTarget, VertexBuffer, ShadeBuffer, ScreenTexture (SCREEN_SURFACE's texture, 106x106),
  WindowRect, SurfaceArea, Ready (+0x70, returned by SurfaceRecreated). The frame timer members (+0x44-+0x6c)
  and the shared statistics are TSurface's/Surface.cpp's as #9 settled them.
- **Merge with #9** (PR #28, merged into this branch first): #9 had named the shared parts from
  SurfaceDX.cpp, so this file uses its names for TSurface's members (Width, Height, Depth, Unavailable,
  CacheSurfaces, the frame timer +0x44-+0x6c), the slots (SetSourceColorKey +0x54, ResetFrameTimer,
  WaitForNextFrame, InitDisplay, ExitDisplay, CreateSurfaces, ResetDisplay, RestoreSurfaces,
  SurfaceRecreated, GetSurfaceMemory, BlitRectClipped with int arguments), the enums in Surface.h and
  Surface.cpp's statistics (BlitCount, BlitPixels, Data71D0D8, LastBlitCount, LastBlitPixels,
  NumberOfSurfaces, InitStep 0x71d0bc). Evidence from this file added to them: +0x98 is SetSurfaceRect.
  IsDirect3D9Installed and CreateSurfaceDX9 are declared in Surface.h, because SurfaceDX.h's DirectX 6
  d3d.h sets DIRECT3D_VERSION 0x700 and empties a later d3d9.h.
- **Function names** (low unless the slot name came from #9 or Surface.h): IsDirect3D9Installed,
  CreateSurfaceDX9, AddQuad, AddShadeQuad, FlushVertices, SetTexture, SetRenderTarget, ApplyColorKey,
  FlushShade, EnumModesCallback; slot +0x98 SetSurfaceRect (TSurface's empty default renamed from Unknown98:
  surfaceDX9.cpp sets Rect[Surface] there).
- **FromRect and ToRect** (medium). The assertions of Blit, BlitRect and StretchBlit name
  "FromRect.width", "FromRect.height" and "ToRect.width": in the original those slots took rectangles whose
  members are width and height (like TSkin; graphics.h's TRect has Width and Height). The binary passes the
  members as separate words, so the slots keep TSurface's int parameters and the assertions are written with
  TASSERT_TEXT/TASSERT2_TEXT, which give the binary's text and values. Issue #29 asks whether TRect's
  members should be renamed.
- **Exception tables** (high). No function has TBEGIN/TEND: the frames hold only the assertions' strings
  (5 states each). ResetDisplay (0x4f1160) is try { InitDisplay; CreateSurfaces } catch (PROGRAM_ERROR) {
  try { ExitDisplay } catch (...) {} throw } (ehinfo: catch PROGRAM_ERROR at states 0-0, catch ... at 2-2).
  No switch has a jump table (Init's switch on ViewMode is a compare chain).
- **Odd behaviour kept**: FillRectangleClipped passes the stack slot of its Rect argument as ClipRect's
  second rectangle (the pixel count uses the unclipped size); SetSurfaceRect sets SurfaceArea.left only for
  surface 0 (an else without braces, by the look of it); WriteImage below SPRITE_SURFACE always writes into
  ScreenTexture and blits all of it to AUTOMAP_SURFACE with point filtering, keyed with ToSurface's colour;
  ApplyColorKey copies Rect[Surface] of the temporary 64x64 surface; RestoreSurfaces resets a device it can
  reset and then releases it and calls InitSurfaces; FinishFrame compares ViewMode before Present without
  using the result; InitDisplay with no listed modes reads before the NULL array; CreateSurfaces returns quietly
  when the alpha blending states fail, checks no other state, sets only dwSize of the display mode
  description, and its cache texture loop has a second exit for textures from the 25th on (16 are made);
  the mode callback tests DDSD_REFRESHRATE in the pixel format's flags; AddShadeQuad has no check for a full
  buffer; WaitForNextFrame's FPS uses an unsigned frame count.
- **callcheck** (13 differences, all explained): the slot calls (ClipRect, BlitRect, StretchBlitRect,
  WaitForNextFrame, InitDisplay, CreateSurfaces, ExitDisplay, InitSurfaces, ResetFrameTimer) are indirect in
  the binary, and the tool matches some of them with functions of the same name elsewhere (the device's
  Reset with DialogElements' 0x452790, SetFramerateLimit with SurfaceDX.obj's copy 0x4eb000); SetTexture is inlined into FlushShade,
  StretchBlitRect and StretchBlitUnclipped and IsDirect3D9Installed into TestEngine; the base constructor
  and destructor calls are not lexical calls.
- **Tests**. surfacedx9test.cpp (new): every round creates the image's surface with 0x4f1430 and ours with
  CreateSurfaceDX9, runs InitSurfaces and Init (windowed or full screen, forced or not, the device lost or
  not), then 20-80 random operations through the vtable (every slot, the image's object called through its
  own vtable) and ExitSurfaces and the destructors; the logs of every fake call, a dump of every member word
  of both objects (fake objects by kind and number), hashes of every fake object's memory and Surface.cpp's
  statistics are compared after each operation. The Windows functions are fakes on both sides (the image's
  import slots get the fakes, difftest.exe's own import slots are patched during the test, DirectDrawCreate
  is defined by the test); the Direct3D 9 and DirectDraw objects are per-side fakes with COM vtables that log
  every call, keep pixel and vertex memory and log a hash of the vertices each DrawPrimitive draws.
  TestEngine and IsDirect3D9Installed (with and without d3d9.dll) run every seventh round, the failing
  alpha state every eleventh. Not run: the error paths (assertions and PROGRAM_ERRORs; the image cannot
  throw in the child). Mutation check (build/scratch/dx9/mutate.py): 27 changes; 25 made the test fail, two
  are equivalent (BlitClipped ignores the colour key StretchBlit passes; at exactly 1 ms late,
  NextFrame = Now - 1.0 assigns the value NextFrame already has, so `>` and `>=` agree). An earlier run's
  heap corruption was the test's own input (a cache surface shrunk below the 106-pixel writes).
  DIFFTEST_ONLY=SurfaceDX9 runs it alone (DX9_TRACE=1, DX9_LOGS=<file>, DX9_ROUNDS=<n>).
