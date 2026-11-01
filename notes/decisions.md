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
  Status and SurfaceDX in the link order. Status.cpp's data ends before
  0x71d0c0 and SurfaceDX's starts at 0x71d0f0.
- **0x4eae70-0x4ed19f is SurfaceDX.cpp** (high): TSurfaceDX's vtable
  (0x56f038) follows TSurface's, its destructor 0x4eae90 sets it and calls
  ~TSurface, and the DirectX 5 factory 0x4eca00 is what GetSurface calls for
  engine 0. Recorded in file-ranges.tsv, not reconstructed yet.
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
