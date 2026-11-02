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
  Reviewed in #24: confirmed. Every state's break jumps to 0x4e0203 `cmp [esp+0x10],0x3e7; jne 0x4dfcad`
  (the switch), and the identifier state compares `cmp ebx,0x14` (0x4dfde1) right after its getc, as
  written. Only the first pass differs from a check at the loop head, where Size is 0, so the shape cannot
  change behaviour. scripttest.cpp's TestLongTokens now reads the two limits from those instructions,
  compares identifiers of 18-19 characters and strings of 996-998 stored characters (with escapes) on both
  sides and checks that one character more raises our error (our side only; the image cannot throw in the
  child). Mutations: limit MAX_STRING_LENGTH -> 100 failures; identifier limit - 1 -> our side raises.
  High.

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
  Reviewed in #24: confirmed: the low part is only the source syntax; the behaviour (null-return new,
  free, the destructor) is what talktest compares.
- **ABI and field types** (high for widths/offsets, low for original source
  declarations). Colors use an explicit output pointer and return it, which
  reproduces the hidden-return-buffer x86 ABI without guessing the original
  struct-by-value syntax. TTalkLine::Time is a signed 32-bit time_t, not
  modern time_t. Flags at +0x24..+0x29 have inferred semantic names; their
  byte values, vtable access and six class sizes are checked. Write's original
  catch calls its FILE* stack slot bytes; an INT_PTR alias retains that name.
  Reviewed in #24: confirmed: TTalkLine::Time is only set from _time32 and passed to _localtime32 (never
  compared), so its signedness cannot show; the colour output pointer is the hidden-return ABI (talktest).
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
  Reviewed in #24: confirmed for the behaviour: a .bss datum with no static initialiser in the __xc table
  0x5c002c-0x5c00c4 and no atexit, only read; its owner decides the file, not the code. Owner stays low.
- **Container.h array<T>** (class name low, members high). The four tables
  are 12-byte objects {min, d, entry} with a constructor at line 152
  ("d>0") and operator() at line 178 ("index out of range (original i: ...,
  i: ..., min: ..., d: ...)"); "ObjectProperties->low()" and "->high()"
  name the bounds. Written as operator() returning T&, like vector<T>
  (Container.h already has it so). The destructor `delete[] entry` is
  inferred from ReleaseObjects, which frees entry right before the array.
  Put before vector<T> in Container.h, as in the original line order.
  Reviewed in #24: confirmed: the atexit thunk 0x55a2c0 of Map.obj's array<TMapPoint> (0x5d4c18) deletes
  its entry (0x5d4c20), the same destructor ReleaseObjects inlines.
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
  Reviewed in #24: confirmed: a reference and a pointer are passed alike (the callers push an address);
  objectstest compares every function.
- **Obj.Data2 holds the hook flag** (medium): GetHangPattern compares the
  third TObject field with 18 and 19 (HOOKSOUTH, HOOKEAST).
  Reviewed in #24: confirmed: GetHangPattern reads [Obj+8] (0x4d6f0f) and compares it with 0x12 and 0x13.
  High.
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
  Reviewed in #24: confirmed for the behaviour: ReadErrorFile (0x4da140) writes ECX before any read and
  FatalError (0x4d7d80) only pushes it as a stack slot, so member or free function compile to the same
  behaviour.
- **Names** (low unless noted): Terminating (+0xa0) and RunPatch (+0xa1, Run
  starts patch.exe; set elsewhere through the application pointer),
  GetWindowTitle/GetWindowClassName (the decoded "Tibia   " and
  "TibiaClient"), CountTibiaWindows (the EnumWindows callback at 0x4d9420,
  missed by Ghidra), NumberOfTibiaWindows, LastMousePosition,
  LastMouseCursor. OutputControlErrorInformations (0x445900) and
  CMainFrame::OutputErrorInformations (0x4a8360) are named after Player.cpp's
  catch texts (high).
- **The XOR code tables at 0x5733a4/0x5733c4** are in another object's
  .rdata (the key tables at 0x56dd98/0x56ddb8 are Player's, next to its
  vtable); issue #12 gave them to sysinc.cpp (see "Unknown-owner data").
- **The critical-error text pointer 0x5c249c** is an entry of Player.obj's
  copy of the text table (0x5c22cc-0x5c2517): TextCriticalError in external.h
  (issue #12, see "Unknown-owner data").
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
  Reviewed in #24: superseded: issues #9-#11 typed every slot from the engines' code, and their tests call
  them on both sides.
- **Names** (low): GetCurrentEngine, GetSurface, GetAvailableEngines,
  SurfaceInitInstance (after the binary's SurfaceExitInstance),
  SurfaceWindow, AvailableEngines.
- **The removed try block in SurfaceInitInstance**: the unwind map has two
  states around ChoosePixelFormat/DescribePixelFormat without a try-block
  entry; the compiler drops handlers that only guard C functions. The catch
  body is unknown; it is written like the first one (PROGRAM_ERROR 12), low.
  Reviewed in #24: confirmed: our /EHsc compile of SurfaceInitInstance has the binary's 4 unwind states
  and one try block (states 0-0); the second catch (...) is eliminated by our compiler too (C functions
  only), so its guessed body is never emitted. (difftest builds with /EHa, which keeps it; no test reaches
  it.)
- **0x5c2564** (the "engine N" value) is Surface.cpp's CommandLineEngine
  (issue #12, see "Unknown-owner data"): in .data after Status.cpp's 0x5c2560
  and before CacheSurfaceWidth, written by Player.cpp and read here.

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
  Reviewed in #24: superseded by issue #13: Container.h's vector destructor frees entry, and our
  EventHelps registers it at exit like the binary's atexit thunk 0x55a2b0 (delete of 0x5ce558).

## Inventory.cpp and the Inventory/MainFrm gap

- **Struct and field names** (low unless from the assertions): TContainer
  (IsOpen and Count are in the assertions), Object, Name, Capacity,
  HasParent, Objects; TOffer (OwnOffer.Count and CounterOffer.Count are in
  the assertions) with Name and Objects. CONTAINER_OBJECTS = 36 is our name
  (the binary's limit 0x24); BODY_POSITIONS, OPEN_CONTAINERS and
  OBJECTS_PER_OFFER are from the assertions. Layouts from the offsets
  (container 0x1ec bytes; offer 0x4d4, CounterOffer is 0x4d8 after OwnOffer).
  Reviewed in #24: the padding is confirmed harmless: no instruction or data word refers to
  0x5d116c-0x5d116f (only OwnOffer 0x5d0c98, its Count/Objects 0x5d0cb8-0x5d0cc4, CounterOffer 0x5d1170
  and 0x5d1190-0x5d119c), nothing copies a whole TOffer, and CounterOffer ends at PacketHeader 0x5d1644.
  TOffer is 0x4d4 bytes (Inventory.h said 0x4d8; corrected).
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
  Reviewed in #24: confirmed for the behaviour (.bss, no initialiser, no atexit; the constructor clears
  it). The same review found MainFrm.obj's unused TCallback 0x5d1674 (initialiser 0x559f20) missing: added
  as MainFrm.cpp's static NoCallback.
- **Free functions** ExitProgram, CloseProgram, SetDisplayMode, OpenURL and
  UpdateMouseCursor (names low; OpenURL and SetDisplayMode after
  Control.cpp's callback parameter names) use the global MainFrame and are
  passed as callbacks to InitWindow and the main window.
- **Eliminated try blocks**: ExitProgram, AddFrameSize and KeyUp have the
  unwind states of a TBEGIN/TEND but no try entry; written with TBEGIN/TEND.
  Reviewed in #24: confirmed: our /EHsc compile of ExitProgram, AddFrameSize and KeyUp has the binary's 11
  unwind states and no try block, so the guessed catch bodies are never emitted.
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
  Reviewed in #24: the open question was settled by issue #13 (vector<T> has its destructor; the statics
  free their entries at exit as the binary's atexit thunks do).
- **Static initialisers** (low, not used as evidence): 0x559df0 constructs an
  8-byte (0,0) object at 0x5c4558, just before Creatures data, and
  0x559e00-0x559e20 three at 0x5cc418-0x5cc42f just after it (0x5cc420 is used
  by Dialogs.cpp). The same constructor (0x496130) builds such objects in
  several objects. Issue #12 settled them: 0x5c4558 is Control.obj's
  NoCallback, 0x5cc418 Creatures.obj's (see "Unknown-owner data").
  Reviewed in #24: confirmed: the __xc table 0x5c002c-0x5c00c4 has one TCallback(NULL, 0) (0x496130, no
  atexit) for ConsoleWindow, ContentWindows, ContextMenus, Control, Creatures, Dialogs, DialogWindow,
  MainFrm (0x5d1674), MainWindow and MapWindow; ours are a static NoCallback in each of those files
  (MainFrm.cpp's added now), without a destructor.
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
  Reviewed in #24: no behaviour: the order of definitions does not change the code; IsPlayerInParty's
  inlined read is compared by creaturestest.
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
  Reviewed in #24: confirmed: our /EHsc compile has the binary's 16 and 11 unwind states and no try block,
  so the guessed catch bodies are never emitted.
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
  Reviewed in #24: confirmed: every comparison with 0x40000000 is unsigned (0x44b910 sbb, 0x44d8a1,
  0x451fb4, 0x451ff2 jb), UpdateSkulls compares SkullTime signed (0x44c88d jl, 0x44c894 jle: a 32-bit
  time_t), the bits are 0/1 in either type, GetHealthColor ends with ret 4 (creaturestest). High.
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
  Reviewed in #24: confirmed: 0x559f30 sits between MainFrm.obj's 0x559f20 and Map.obj's 0x559f40 in the
  __xc table 0x5c002c-0x5c00c4. High.
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
  Reviewed in #24: confirmed, and the explanation corrected: Inventory.obj does run zeroing initialisers
  for TObjects in .bss (0x559ea0: TContainer.Object inline and Objects[] through the eh vector constructor
  iterator with 0x4a4450, which clears three words; 0x559ee0/0x559f00: OwnOffer/CounterOffer.Objects), so
  the compiler did not drop them; only the plain TObject globals and arrays (NoObject, Body, Status.cpp's)
  have none. Either way the data are zero before main; ours (default member initialisers) have the same
  three Inventory initialisers. High for the constructor.
- **TRGBColor constructors** (medium for TRGBColor(int Color), low for the
  others). 0x425950 (between ConsoleWindow.cpp and ContentWindows.cpp) takes
  ECX and a palette index, stores R, B, G and returns this: a constructor.
  GetAutomapColor calls it on its return slot; SetAmbientLight has it inlined
  (same store order). Defined inline in external.h's TRGBColor (the header
  it belongs to is not reconstructed); a default constructor and a
  three-value constructor (GetAutomapColor's black, and Control.cpp's
  "= {200, 200, 200}") were added so that the type stays usable as before.
  Reviewed in #24: confirmed: the constructors only store their values (the order of independent stores
  cannot show); the default one zeroes, as the binary does (graphics.cpp's entry); maptest and
  graphicstest compare the colours.
- **Colours by value** (medium). GetAmbientColor and GetAutomapColor return
  TRGBColor by value (hidden return pointer, the same ABI as Talk's explicit
  output pointer); the constructor call on the return slot shows a returned
  temporary.
  Reviewed in #24: confirmed: by value with a hidden return pointer is the ABI the binary shows (maptest).
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
  Reviewed in #24: confirmed: references and pointers are passed alike; DeleteAutomapMark reads only x and
  y either way (maptest).
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
  Reviewed in #24: no behaviour outside the error paths: only the value TEND1(i) prints differs, where the
  original prints an uninitialised slot (cannot be reproduced).
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
  Reviewed in #24: the union is confirmed: its offsets are the accessors', maptest compares every effect
  kind after random operations, and MSVC does no type-based alias analysis, so a union cannot change the
  code.
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
  Reviewed in #24: no behaviour: the header only decides where the identical inline copies are emitted.
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
  Reviewed in #24: confirmed: (int, TRGBColor) has the stack layout of the callers' copies; graphicstest
  records the calls on both sides.
  Settled in #29: Blit, BlitClipped and StretchBlit take their source (and StretchBlit its target) as a
  TRect FromRect/ToRect: every call builds a 16-byte block (see "TRect's members (issue #29)").
- **TVideoCacheItem's overlapping fields** (medium): masked sprites keep the
  mask number and four colours at +0x14/+0x18, pictures and font characters
  TileX, TileY and the text colour at +0x14/+0x18/+0x1c (the Find and Get
  functions read them so). Written as unions. Its constructor 0x48b3f0 clears
  the four colours and then the text colour, in that order; written as a
  user constructor with these stores. A C++03 union cannot hold a TRGBColor
  with a constructor, so the original declaration was probably different
  (low); the stores are what the binary shows.
  Reviewed in #24: confirmed: 0x48b3f0 stores +0x18..+0x47 and then +0x1c..+0x27, as ours; graphicstest
  memcmp's every item (0x5c bytes) after each operation, so layout, union bytes and the constructor are
  compared. The original declaration stays low.
- **TRGBColor's default constructor zeroes** (as GUI.cpp found): the binary
  zeroes default-constructed colours, also here (DrawOutfit's local array, the
  `new TSprite[]` and `new TPicture[]` loops, which leave Offset and the tile
  sizes alone). So TSprite and TPicture have no constructors of their own and
  DrawOutfit just declares its array; the WIP's explicit zeroing (written
  against the old non-zeroing TRGBColor()) was removed in the merge with
  GUI.cpp. TFont's third field is Alignment (was Unknown10 in GUI.h).
  Reviewed in #24: confirmed: the SkinProperties constructor test (guitest, byte for byte) shows the
  zeroing; DrawOutfit sets all four colours before using them, so its array's initial value cannot show.
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
  Reviewed in #24: confirmed: our /EHsc compile has the binary's unwind states and no try block for the
  eight Find/text functions (11, 21, 26 states), and DrawPicturePart's 26 states with the try at 15-15:
  the two always-true assertions emit nothing in either.
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
  Reviewed in #24: no behaviour: linkage only.
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
  Reviewed in #24: confirmed: a 16-byte struct argument has the stack layout of the binary's four stores
  (guitest).
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
  pushes at all their call sites (wrong for SetChildArea, see below). The only known 16-byte struct is TRect;
  its x, y, Width, Height are the left, top, right, bottom margins. The
  SkinProperties members passed this way (+0x0a0, Style194+0x50) became
  TRects.
  Settled in #29: TRect's members are x, y, width, height (the margins are Area.width/Area.height now).
  Settled in #27: SetChildArea gets a 16-byte block at all its 136 call sites too and takes TRect Area
  now; SetChildPosition does get three plain pushes.
  Reviewed in #24: confirmed: the 16-byte block is the binary's; which struct type it is cannot change the
  code.
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
  Reviewed in #24: no behaviour: the member keeps its offset (guitest compares SkinProperties byte for
  byte).
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
  Reviewed in #24: confirmed: the three copies 0x5c1954, 0x5c1230 and 0x5c066c hold 224, 224, 224 (one per
  object, as a header constant gives).
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
  Reviewed in #24: confirmed: 0x559fc0 builds TCallback(NULL, 0) at 0x7152e8 with no atexit, as our static
  does.
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
  Reviewed in #24: confirmed: the conversion only enables the binary's TEND1(Obj) and comparisons; every
  TObject user is compiled and differentially tested.
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
  Reviewed in #24: confirmed: the binary zeroes the new entries (0x4c1ab4) and the window tests compare
  the drawing lists.
- **Surface slots** (high for the arguments, low for the names): +0x20
  DrawFrameClipped(Surface, TRect, LineWidth, TRGBColor, TRect Clip), +0x30
  BlitRect(ToSurface, x, y, FromSurface, TRect Source), +0x40
  StretchBlitRect(ToSurface, TRect, FromSurface, TRect), +0x4c
  ShadeRectangles(Surface, Count, TRect *, TRGBColor *) with four corner
  colours per rectangle, +0xa4 CanShadeRectangles(). +0x9c(2) returning true
  makes Draw forget the tile cache (the surface lost its contents, low).
  Reviewed in #24: no behaviour: only the explanation of +0x9c(2) is low; the code does what the binary
  does (mapwindowtest).
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
  Reviewed in #24: confirmed: the inline clamp is compared wherever it is used (window tests).
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
  Settled in #27: SetChildArea takes a TRect; Arrange passes TRect(Data140.x, Data140.y, Data148.x,
  Data148.y) (the two points may be one TRect, #31).
  Settled in #31: SkinProperties.Data140 is one TRect (+0x140-+0x14f), and Arrange passes it.
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
  Reviewed in #24: confirmed: 0x559f30 builds TCallback(NULL, 0) with no atexit, as our static does.
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
  Reviewed in #24: no behaviour: the order of definitions does not change the code.
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
  Reviewed in #24: confirmed: the minimum heights read between are getters that cannot change
  Splitter->Height, GetConsoleHeight has no side effect, the question array comes from operator new[]
  (0x523817); mainwindowtest compares SetSize and ChangeMode.
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
  Reviewed in #24: confirmed: guitest compares the constructor's and InitSkinProperties' bytes; within the
  zeroed bytes, five points and a TRect are the same code.
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
  Reviewed in #24: superseded: the sketched classes are the reconstructed ones now (issues #3-#8), tested.
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
  Reviewed in #24: confirmed: 0x559e20 builds TCallback(NULL, 0) with no atexit, as our static does.
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
  Reviewed in #24: no behaviour: inline or out of line, the code is the same (dialogwindowtest).
- **Source order** (medium). Line order. The binary puts Arrange (169),
  Shrink (377-394), Grow (505) and Rearrange (271-297) at the end of the
  object: each calls a function defined further down (GetFreeHeight 524,
  the strategy helpers), the pattern noted for Creatures.cpp.
  Reviewed in #24: no behaviour: the order of definitions does not change the code.
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
  Reviewed in #24: superseded: TResizer, TSkinButton and TSubWindowBody are the reconstructed classes now
  (issues #6, #7).
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
  Reviewed in #24: confirmed: 0x559de0 builds TCallback(NULL, 0) with no atexit, as our static does.
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
  Reviewed in #24: no behaviour: inline or out of line, the code is the same (contextmenustest).
- **Source order** (medium). Line order. The binary moves the help bubble's
  constructor (106) and Place (257), AddEntry (506) and Show (486) behind the
  event handlers, followed by all menu constructors: each calls a function
  defined further down (Creatures.cpp's pattern).
  Reviewed in #24: no behaviour: the order of definitions does not change the code.
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
  Reviewed in #24: no behaviour: the buffers receive sprintf of a name (at most 29 characters) and at most
  11 fixed ones, or _snprintf with the binary's limit 200, so a size at or above that cannot show.
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
  Reviewed in #24: confirmed for the TRect (the same 16-byte block); the sketches are the reconstructed
  classes now.
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
  Reviewed in #24: no behaviour: which object keeps the identical copies does not change the code.
- **NoCallback** (high for the datum, medium for the form; issue #26). The
  object's only .bss datum is the unused TCallback 0x5c4390, constructed as
  TCallback(NULL, 0) (0x496130) by the static initialiser 0x559dc0 (between
  Communication.obj's 0x559da0 and ContentWindows.obj's 0x559dd0; no atexit;
  its only reference is the initialiser). It follows Communication.obj's .bss
  (0x5c4340-0x5c438f) and comes before ContentWindows.obj's (0x5c4398).
  Written as a static NoCallback after the includes, like DialogWindow.cpp's.
  Reviewed in #24: confirmed: 0x559dc0 builds TCallback(NULL, 0) with no atexit, as our static does.
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
  Reviewed in #24: no behaviour: the order of definitions does not change the code.
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
  Reviewed in #24: confirmed: the inlined call stores CurrentLineNumber as the binary does
  (consolewindowtest).
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
  Reviewed in #24: confirmed: guitest compares the constructor's and InitSkinProperties' bytes.
- **DialogElements.cpp sketches** (external.h, low): TEditLine is a TWindow
  (vtable 0x560e98, 0x68 bytes, constructor 0x467b50 with an area, 255 and
  0; SetActive 0x45bc80; the inline HasSelection; slots +0xa0 SetText, +0xa4
  Clear, +0xb8 SetReadOnly, +0xc0 SetFont; Text at +0x2c); TSkinButton got
  its constructor 0x466860 and members; new: TToggleButton (0x4671e0),
  TMultiStateButton (0x4672d0, a varargs constructor: __cdecl with this
  pushed first; Reset 0x452790), TScrollBar (0x469a80) with its
  TScrollBarRange (SetRange 0x460450, Value at +0x30). All have
  low-confidence names.tsv rows.
  Reviewed in #24: superseded: DialogElements.h (issue #7); TScrollBar's last parameter is ButtonRange, 1
  at ConsoleWindow.cpp's call.
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
  Reviewed in #24: confirmed: the folded 0x425a00 returns true, and contentwindowstest found and compares
  it.
- **Limit** (low for the name, medium for the header). 0x425920 clamps
  *Value to Min..Max in either order. ContentWindows.cpp inlines it in
  TTradeWindowBody::SetSize (with std::min/std::max of the maximum and
  minimum heights, each read twice), DialogElements.cpp calls the
  out-of-line copy. A header both include: common.h, next to In.
  Reviewed in #24: confirmed: the clamp is compared wherever it is used.
- **Source order** (medium). Line order (the creators first, lines 106-280).
  The binary moves the creators, the constructors of the bodies,
  TAutomapWindow, TStatusWindow, TSmallInventoryWindow's neighbours and
  TInventoryView's mouse handlers, TPlayerStates' Draw and MouseHover and
  TAutomapWindow::HandleEvent to the end of the object (each calls functions
  defined further down, Creatures.cpp's pattern). Functions without lines
  where the binary has them: TInventoryView::GetSlotAt after HelpMouseMove
  (1344), ~TSkillsWindowBody after its constructor (1980).
  Reviewed in #24: no behaviour: the order of definitions does not change the code.
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
  Reviewed in #24: confirmed: guitest compares the constructor's and InitSkinProperties' bytes; the
  by-value block is the same 16 bytes.
- **Surface slots named** (medium): +0x0c SetPixel(Surface, x, y, Color), +0x10
  FillRectangleClipped(Surface, Rect, Color, Clip), +0x18
  DrawHorizontalLine(Surface, x, y, Width, Color), from the arguments pushed
  here; external.h's TSurfaceGL and playertest.cpp's stubs follow.
  Reviewed in #24: confirmed: the parameters are the pushed arguments; the tests' recording surface
  compares the calls.
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
  Reviewed in #24: superseded: DialogElements.h (issue #7).
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
  Reviewed in #24: confirmed: the parameter roles are what the code does with them (dialogelementstest).
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
  Reviewed in #24: no behaviour: inline or out of line, the code is the same.
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
  Reviewed in #24: confirmed: guitest compares the constructor's and InitSkinProperties' bytes.
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
  no binary function or wrapper. (Settled in #29: TBlitSize is gone; the
  slots take TRect FromRect/ToRect, whose members are width and height.) The factory is CreateSurfaceDX (0x4eca00):
  its allocation is 0x250 bytes with an inlined empty derived constructor.
  Reviewed in #24: confirmed: a local view and an empty inlined constructor add no code (surfacedxtest).
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
  Reviewed in #24: confirmed for the behaviour: zero .bss and the .data words 0x100 (0x5c2568/0x5c256c)
  and a pointer constant (0x5c2570), no initialisers; the owner stays medium/low.
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
  Reviewed in #24: confirmed: vector<TIgnoreEntry>'s constructor 0x46c0d0 clears Name[0] and Permanent of
  each entry, as our default constructor; containertest compares that instance on pattern-filled memory.
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
  Reviewed in #24: confirmed: pointers and arrays are passed alike; dialogstest compares every creator.
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
  Reviewed in #24: confirmed: guitest compares the constructor's and InitSkinProperties' bytes.
- **SetChildArea takes a TRect by value** (not changed): every caller in
  Dialogs.obj builds a 16-byte block (sub esp,0x10) for the area, so the
  original signature is SetChildArea(TComponent *, TRect); GUI.h's five-int
  form has the same stack layout, so the source keeps it (issue #27).
  Settled in #27: SetChildArea(TComponent *Child, TRect Area); the callers pass the TRects themselves.
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
  Settled in #29: TRect's members are width and height, the slots take TRect FromRect/ToRect (the callers
  build 16-byte blocks; "the binary passes the members as separate words" was wrong), and the assertions
  are plain TASSERT/TASSERT2 expressions with the binary's text.
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

## Resources (issue #17)

- **The pictures are not committed.** The two cursors and the icon are CipSoft's artwork, the other
  cursors and the two bitmaps Microsoft's (MFC). tools/extract_resources.py rebuilds them byte-exact
  from a local Tibia.exe into build/res (gitignored); src/res/Patella.rc refers to them by file name
  and is compiled with /I build\res. The text resources (version information, the MFC strings and
  the MFC dialog) are written out in the script.
- **MFC's resources are written out, not included.** Tibia.exe carries the afxres.rc of MFC 7.1 (with
  the splitter, tracker, OLE and property sheet parts switched off and no afxprint.rc). Including the
  installed MFC 14's afxres.rc would add resources and change texts, so Patella.rc has MFC 7.1's
  entries as the image has them. They are kept even where no code of the image refers to them, so that
  the resource set equals the original's.
- **Names.** The original resource.h is unknown. src/res/resource.h names only the client's resources:
  IDR_MAINFRAME (icon 128, AppWizard's usual name for it), IDC_CROSSHAIR (cursor 0x84) and
  IDC_MAGNIFIER (cursor 0x89), named after their pictures (MainFrm.cpp calls the 0x89 cursor
  HandCursor; left as it is, the picture is a magnifying glass). The MFC entries keep their numbers,
  with afxres.h names in comments only, so the header cannot clash with afxres.h once MFC is installed;
  the afxres.h names are from memory (MFC is not installed), the wheel cursors' names agree with their
  pictures. src/*.cpp keep their literal IDs (MAKEINTRESOURCE(128), 0x84, 0x89), as the binary does.
- **Order of the script.** rc.exe numbers the RT_CURSOR and RT_ICON images with one counter in script
  order; the original numbers (cursors 1-2 German, icons 3-4, MFC cursors 5-20) come out with the
  client's cursors first, then the icon, the version information and MFC's part in afxres.rc's order
  (the wheel cursors are not in ID order). This is also the order of the data in Tibia.exe.
- **Languages.** The two client cursors are German (1031), everything else English (US, 1033); the
  version information's string block is 000004b0 (language neutral, Unicode) with Translation 0, 1200.
- **No AFX_IDS_APP_TITLE (0xE000), no AFX_IDI_STD_FRAME/MDIFRAME (0x7A02/0x7A01).** MFC asks for them,
  the original has none and falls back to the module name and IDI_APPLICATION; Patella keeps them out.
- **Compare.** tools/compare_resources.py compares (type, name, language) and the data; not the
  directory time stamp, the code page field (0 in the image), the order of the data or the .res memory
  flags. The first compile with rc.exe 10.0.26100.0 matched all 55 entries; nothing in the script needed
  a workaround for rc.exe's formats (the version information's padding is the same).
- **Final check is #16's.** Patella.exe cannot be linked yet; notes/resources.md says how #16 adds the
  .res (no afxres.rc, /MANIFEST:NO) and runs compare_resources.py on Patella.exe.

## Unknown-owner data (issue #12)

Settled by Claude Opus 5.5, November 2, 2026. Method: the neighbours of each datum in .data, .rdata
and .bss (each object's contributions are contiguous, in link order), the code that references
those neighbours (objdump of the whole image), and the static initialisers listed per object in
the __xc table 0x5c0004-0x5c00ff. Names ours unless noted. No names.tsv or file-ranges.tsv rows:
both list code only (names.tsv has no data rows).

- **0x5c249c is Player.obj's copy of the text table** (high): TextCriticalError. The "table of UI
  texts 0x5c2480-0x5c2517" is not a table of its own but the tail of a full copy of the text
  header's table, 0x5c22cc-0x5c2517 (147 entries from "You are poisoned" to "Report Rule
  Violation", the same entries as MapWindow.obj's copy 0x5c2064-0x5c22af and Control.obj's
  0x5c0b34-0x5c0d7f). Every object that includes the text header has its own copy (the entries
  have internal linkage), so the object whose code reads an entry owns that copy: only 0x4d9ed5
  (Player.cpp's HandleException) reads it, and Player.cpp's LastMouseCursor (0x5c2518) follows
  directly, then the type descriptors of out_of_range/bad_alloc that Player.cpp's catch blocks
  need, then Preferences.cpp's WindowedWidth (0x5c2558). Like the other copies it stays declared
  in external.h, under "Player.obj's copy of the text table" (the text header is not
  reconstructed); playertest.cpp defines it for difftest.exe. No other entry of Player's copy is
  read (the crash report texts 0x5c24a0/0x5c24a4 are read through other objects' copies, if at all).
- **0x5c2564 is Surface.cpp's** (medium): CommandLineEngine, initialised to -1. It lies between
  Status.cpp's LinkPattern (0x5c2560, its only .data) and Surface.cpp's CacheSurfaceWidth/Height
  (0x5c2568/0x5c256c), so it is the end of Status.obj's .data or the start of Surface.obj's.
  Status.cpp never uses it; Surface.cpp's SurfaceInitInstance (0x4eac49) reads it to choose the
  engine (curEngine is Surface.cpp's too), Player.cpp's command-line parser (0x4da5c6) writes it.
  Defined in Surface.cpp, extern in Surface.h; Surface.cpp is linked into difftest.exe, so
  playertest.cpp's definition went.
  Reviewed in #24: confirmed for the behaviour: the .data word is -1 and ours is constant-initialised;
  owner medium.
- **The XOR code tables 0x5733a4/0x5733c4 are sysinc.cpp's** (low): TitleCode and ClassNameCode
  (names from the Player.cpp reconstruction), extern in sysinc.h, defined in sysinc.cpp;
  playertest.cpp keeps a test definition (sysinc.cpp is not in difftest.exe). They lie exactly
  between SurfaceGL.obj's last .rdata string ("Type>=0 && Type<3" 0x573390, read by 0x4fca8c) and
  sysinc.obj's first ("Unknown error." 0x5733f0, GetErrorText's), so the owner is one of the two
  (an object with data only and no code between them would be a file this project has never seen;
  rejected). Both are referenced only by Player.cpp's decoders (0x4d9270, 0x4d93e0). The deciding
  argument is the order within an object's .rdata: Player.obj's own file-scope data (after
  the "error.txt" literal 0x56dd8c: the key tables 0x56dd98/0x56ddb8, then the message map 0x56dde4)
  comes before the vtable and the strings of its functions, i.e. file-scope definitions are
  emitted where they are in the source. As SurfaceGL.cpp's, the tables would have to be defined
  after the last function that uses a string (TSurfaceGL's vtable 0x573250 and the strings of its
  later functions precede them); as sysinc.cpp's they are its first .rdata, defined at the top of
  the file as usual. If #11's reconstruction of SurfaceGL.cpp finds file-scope data at the end of
  that file, revisit this.
  Reviewed in #24: confirmed for the behaviour: constant tables with the binary's values; the owner stays
  low.
- **0x5c4558 is Control.cpp's NoCallback** (high): the static initialiser 0x559df0 constructs it as
  TCallback(NULL, 0) (0x496130, no atexit), nothing else references it. The __xc table lists one
  initialiser per object in link order: ContextMenus.obj's 0x559de0 (0x5c4508), then 0x559df0, then
  0x559e00 for 0x5cc418, then Dialogs.obj's 0x559e10 (0x5cc420). 0x5cc418 is the end of
  Creatures.obj's .bss (directly after the creature table 0x5c68b0-0x5cc417; DialogElements.obj
  has no statics), so Creatures.obj's NoCallback is 0x5cc418, and 0x5c4558, after Control.cpp's
  ShowFPS (0x5c4554) and before Creatures.cpp's first datum (0x5c4560), is Control.obj's. Both
  objects keep it at the end of their .bss, as ContentWindows.obj does (0x5c4500). Both are now
  written as static NoCallback (Control.cpp, Creatures.cpp; the second was not in the issue but
  is settled by the same evidence).
- **0x5d16b4 was already settled** (medium, issue #2): MainWindow.cpp's static NoCallback
  (initialiser 0x559f30, between MainFrm.obj's 0x559f20 and Map.obj's 0x559f40); the issue text
  was stale. Nothing changed.
  Reviewed in #24: confirmed (see Map.cpp's entry).
- **The text table "owner unknown, .data 0x5c0c80-0x5c0d7c"** in external.h is Control.obj's copy
  (0x5c0b34-0x5c0d7f, before Control.cpp's LastPacketType 0x5c0d8c; every entry declared there is
  read only by Control.cpp, 0x4499c8-0x449f1a): the comment says so now.
## SurfaceGL.cpp (issue #11)

Reconstructed by Claude Opus 5.5, November 2, 2026.

- **Range** (high). SurfaceGL.obj is 0x4f46f0-0x4fcb8f: the surfaceDX9/SurfaceGL gap settled in #10 plus
  the asserted functions after it, whose last one (SurfaceRecreated, line 1555) ends at 0x4fcb8f, where
  sysinc.cpp starts. Ghidra missed CreateSurfaces 0x4fb070, GetFullscreenModes 0x4fb4e0, InitSurfaces
  0x4fb700, Blit 0x4fc480, BlitRect 0x4fc6a0, GetSurfaceRect 0x4fc8a0, SurfaceRecreated 0x4fca30 and the
  slots 0x4f9b80/0x4f9bd0/0x4f9c10/0x4f9c40/0x4fa510/0x4fa8b0/0x4fab70; all come from the vtable 0x573250
  and were written from the instructions. 0x4f98a2 is not a function (Ghidra split SelectSurface). The
  object's data: .data 0x5c257c (the filter, GL_LINEAR) and 0x5c2580 (the selected surface, 1); .bss
  0x71d290-0x71d317 (dirty rectangle, pbuffer, main context and DC, SurfaceCreated[22]), the glh pointers
  0x71d318-0x71dba8, glh's strings 0x71dbac/0x71dbb0, the frame counter 0x71dbb4 and 0x71dbb8 (FPS time).
  0x71d2bc is referenced by nothing (padding or an unused static; left out). 0x5c2578 (256) is still
  unowned (#10).
- **glh_extensions.h** (high for the code, medium for the file layout). The leading run 0x4f46f0-0x4f954f
  is NVIDIA's glh_extensions.h with glh_genext.h (the memory error message keeps the path
  "Extern\glh/glh_extensions.h", lines 173 and 191), included with GLH_EXT_SINGLE_FILE: glh_init_extension
  (121 extension names, 549 entry points; a newer version of the header that knows GL_VERSION_1_2-1_5),
  the static ExtensionExists (extName passed in EBX: a register convention the compiler chose for the
  static function), glh_init_extensions and 0x4f9510, which frees the two strings. The NVIDIA SDK is not
  installed, so both headers are reconstructed in src/Extern/glh (the original's include path):
  glh_genext.h is generated from the binary (build/scratch/gl/glhparse.py parses the blocks of 0x4f46f0,
  genext.py writes the header; every name/slot pair is consistent, no slot unused). The pointers are PROC
  except the WGL_ARB_pbuffer/pixel_format ones SurfaceGL.cpp calls (their wglext.h types written out); the
  types do not change the code. A `#line 173 "Extern\\glh/glh_extensions.h"` gives the two CHECK_MEMORY
  messages the original's __FILE__/__LINE__. names.tsv kind stl (library code), the names are glh's
  except glh_shutdown_extensions (0x4f9510, ours, low; it sits right after glh_init_extensions, so it is
  the header's) and the prefix of glh_ExtensionExists. strdup is written _strdup (the same CRT function;
  the modern SDK deprecates the POSIX name). Odd behaviour kept: ExtensionExists' GL_VERSION_1_x answers
  leak padExtName.
  Reviewed in #24: confirmed: our compile pushes 173 and 191 for the two messages, as 0x4f92e2 and
  0x4f93ca do.
- **Class** (layout high, names low unless from the binary). TSurfaceGL, 0x498 bytes: Context +0x70, the
  textures TTextureGL Textures[22] +0x74 (0x20 bytes each: name, size, power-of-two size, the colour key;
  the inline constructor sets the key to (1,1,1) after TRGBColor's default constructor zeroed it), RECT
  Rect[22] +0x334, bool Recreated[3] +0x494. From TSurface it uses +0x0c as the window's HDC
  (Surface.h's Data0C renamed DeviceContext) and +0x28 as a RECT, the client area in screen coordinates
  (Data27[0x11] split into Data27 and ScreenRect). NUMBER_OF_SURFACES_GL, Type (SurfaceRecreated) and
  FromRect are from the assertions. Ours: Context, Textures, TTextureGL and its members, Recreated,
  ScreenRect, DeviceContext, IsOpenGLWorking (Surface.cpp's former FUN_004f9710), CreatePbuffer,
  ReleasePbuffer, CreateTexture, the statics.
- **TSurface slots** (medium). The empty TSurface slots +0x04, +0x08 and +0x80 are overridden only here:
  +0x04 SelectSurface(int Surface) (the drawing target), +0x08 SetTextureFilter(int Filter) (GL_LINEAR or
  GL_NEAREST), +0x80 AddDirtyRect(TRect Rect): the callers build a 16-byte TRect on the stack, so the
  parameter is a TRect (the former four ints had the same stack layout). Surface.h, Surface.cpp,
  names.tsv (TSurface_SelectSurface, TSurface_AddDirtyRect, TSurface_SetSurfaceRect for the old
  Unknown04/80/98 rows) and surfacedxtest.cpp's calls are renamed. The base slot names do not collide with
  TSurfaceDX9's non-virtual SetRenderTarget.
  Reviewed in #24: confirmed: a TRect by value is the callers' 16-byte block; surfacegltest compares the
  calls.
- **Folded copies** (high). Six slots of the vtable point to identical code kept from another object:
  ClipRect 0x4f0410 and ResetFrameTimer 0x4f04d0 (surfaceDX9.obj), SetFramerateLimit 0x4eb000
  (SurfaceDX.obj), CanShadeRectangles and TestEngine 0x4d7d40 (the return-true body folded with the STL's
  codecvt::do_always_noconv); they are written at the end of SurfaceGL.cpp, as surfaceDX9.cpp does. +0x50
  (SetColorKey), +0x64 (AttachWindow) and +0x98 (SetSurfaceRect) are TSurface's. The other way round,
  GetFPS 0x4f9d00, DrawFrame 0x4fa110, DrawFrameClipped 0x4fa2c0, BlitClipped 0x4fab70, GetCacheSurfaces
  0x4fb5e0 and IsUnavailable 0x4fb5f0 are this object's code in the DX5/DX9 vtables: SurfaceGL.cpp has its
  own versions, the same text as surfaceDX9.cpp's/SurfaceDX.cpp's (they agree: the copies of those files
  were tested against the same addresses). names.tsv's TSurfaceDX_* rows for them became TSurfaceGL_*.
  WaitForNextFrame 0x4f9c40 is the same code as surfaceDX9.cpp's with this file's own counters (not
  folded: other data). There is no display mode callback here: GetFullscreenModes uses
  EnumDisplaySettingsA.
- **Order**. Only WriteImage (767-768), WriteMaskedImage (802-803), BlitRectClipped (1023), Blit (1073),
  BlitRect (1086), GetSurfaceRect (1540) and SurfaceRecreated (1555) have lines; they are in binary order,
  so the file is in binary order.
- **EH tables and switches** (high). IsOpenGLWorking: try { ... } catch (...) { return false; } from
  CreatePbuffer to the return (the state never goes back to -1 before it). ResetDisplay: try { InitDisplay }
  catch (...) { try { ExitDisplay } catch (...) { throw 12 } throw 12 } (states 0 and 2). RestoreSurfaces:
  try { CreateSurfaces } catch (...) { throw 12 }. The asserted functions have only their assertions'
  states (10, 10, 5, 5, 5, 5, 5; the last four share one FuncInfo); no TBEGIN. No jump tables.
- **MainFrm.cpp's DisplayChanged** (high). 0x5d1670, set by OnDisplayChange, is read and cleared by Init
  (SurfaceRecreated(2) reports it): it is no longer static; MainFrm.h declares it.
- **Odd behaviour kept**: ReleasePbuffer and CreatePbuffer test the imported functions for NULL; after
  wglCreateContext CreatePbuffer tests the pbuffer again instead of the context; IsOpenGLWorking's early
  returns leave the context current; FillRectangleClipped passes an uninitialised RECT as ClipRect's second
  rectangle and counts the unclipped size; StretchBlitUnclipped's scaled path ignores both colours;
  StretchBlit selects CACHE_SURFACE before reading STRETCH_SURFACE back; WriteImage does not select
  AUTOMAP_SURFACE; CreateSurfaces does not check TEMP_SURFACE's result and has the second way out of the
  cache loop for i >= 24; CreateTexture leaves the texture contents undefined (an uncleared buffer) and
  counts in SurfaceCreated without a bound; InitDisplay sets the view mode after a failed switch back to
  the desktop, changes nothing after a failed full-screen switch and reads before an empty mode list;
  GetFullscreenModes never sets the description's size and does not store NumberOfModes; Init reads
  Resolution->Depth for the pixel format in windowed mode too and only reports missing extensions to the
  debugger; ExitSurfaces frees glh's strings with glh_shutdown_extensions' inlined code.
- **callcheck** (20 of 45 blocks differ, all explained): the slot calls are indirect in the binary
  (SelectSurface, AddDirtyRect, SetTextureFilter, ClipRect, FillRectangle(Clipped) in the frames,
  WaitForNextFrame, GetFullscreenModes, InitDisplay/ExitDisplay/CreateSurfaces/ResetDisplay/
  RestoreSurfaces/UpdateWindowRect in Init and ResetDisplay, ExitDisplay in ExitSurfaces); the base
  constructor/destructor calls are implicit (0x4fb670, 0x4f9860); TRGBColor's operator== is a call in the
  binary (0x48b380, WriteMaskedImage) but an operator here; glh_shutdown_extensions is inlined into
  ExitSurfaces in the binary. The functions Ghidra missed are not counted by the tool; they were compared
  with their instructions.
- **Tests**. surfacegltest.cpp (new): the opengl32/glu32 functions are never linked: the import symbols
  of ours are the test's GLSlot_ variables (/alternatename), gluOrtho2D is defined by the test; the
  user32/gdi32/kernel32/winmm functions are patched in difftest.exe's import table and the image's. The
  fakes log every call (pixels hashed, floating point as bits) and answer from one world that both sides
  start from. Our statics are found once by sentinel values (handles 0x7a5e300x, a filter, a surface
  rectangle) and glh's strings through glh_shutdown_extensions' code; every compare covers the logs, all
  object words, the statics of both files, the shared statistics, SurfaceCreated, DisplayChanged, glh's
  strings and the 549 extension pointers. Rounds: Init and 20-80 random operations of 46 kinds (every
  slot, CreateTexture with Empty, glh rounds with random extension/version strings and requests,
  IsOpenGLWorking failing at each step). Not run: the assertions and PROGRAM_ERRORs (CreateSurfaces throws
  only when the pbuffer fails; the image cannot throw in the child) and glh's out-of-memory exits. The
  test has its own random generator, so the later tests' inputs are unchanged.
  DIFFTEST_ONLY=SurfaceGL (GL_TRACE, GL_LOGS, GL_ROUNDS). Coverage was checked in the compared logs (every
  import and branch, among them the copy-back, the projection growth, the GetClientRect of SelectSurface,
  both "not supported" messages, the readback of StretchBlit, the version answers of ExtensionExists).
  Mutation check (build/scratch/gl/mutate.py, 120 rounds each): 28 changes in SurfaceGL.cpp and
  glh_extensions.h, all detected. Two survived the first run and showed weak inputs, since fixed: the fake
  wglChoosePixelFormatARB stopped logging the attribute list at the first 0 value instead of the first 0 key,
  and GL_VERSION_1_4 was rarely requested under a "1.3" version string.
- **Test surfaces**. The window and graphics tests used external.h's TSurfaceGL sketch as their recording
  surface; it is tools/difftest/testsurface.h's TTestSurface now (the same members and size; the slots
  graphicstest.cpp and mapwindowtest.cpp define stay there, the never-called ones abort inline instead of
  playertest.cpp's stubs, and FUN_004f9710's stub is gone). graphicstest.cpp had taken the recording
  surface from our GetSurface (which created the sketch with new for curEngine 1); it now creates the
  TTestSurface itself and patches our GetSurface for good to return it, which mapwindowtest.cpp then takes
  as before. Every test's check count in the full run is unchanged (DialogWindow 24,567, ContextMenus 33,088,
  ConsoleWindow 30,948, ContentWindows 52,506, DialogElements 135,478, Dialogs 98,889, Graphics 570,954,
  MapWindow 115,169, MainWindow 14,164); the full run passes 14,068,269 checks with SurfaceGL's 53,082.
## Header code (issue #13)

Claude Opus 5.5, November 2, 2026. glh_extensions.h's 0x4f9260 is left to #11 (SurfaceGL.cpp).

- **Every header function was in src/ already** (high). The earlier file issues wrote each one where
  its object needed it: Container.h's templates (Status/Preferences/Talk/Objects/Map), GUI.h's
  GetFirstChild/GetLastChild (0x417de0/0x417f00, TERROR "not possible", lines 681-682),
  ConsoleWindow.h's TLineFormat constructors (0x418020, lines 47-54, and the inline default one),
  Utils.h's TBuffer::SetSize (0x4ca300, line 50), Talk.h's TTalkBuffer constructor, GetText,
  GetChannelID and TRuleViolationChannel::AddText (line 321). Issue #13 checked each against its
  disassembly and exception table again; nothing in the code changed. No .cpp or external.h kept a
  hand-written copy of a header member (every vector/array/matrix user includes Container.h).
- **One template, every instance** (high). All 13 vector<T> instances compile from the one
  Container.h template: the four initialized constructors 0x425e70 (TSkinButton *), 0x453300 (int),
  0x4973f0 (TTimer *) and 0x4fd6e0 (TTalkFilter *) are the same instructions apart from addresses
  (one copy per object: functions with exception tables are not folded), lines 355-356, ten EH
  states. The plain constructors have ten states for elements without a constructor (int,
  TContentWindowSize, TBuddy, TMessage) and eleven for those with one (TLineFormat, TMark,
  TListBoxLine, TIgnoreEntry, THotkey, TErrorReport): the extra state frees the new[] block if an
  element constructor throws; init is default-constructed first (THotkey: TCallback's constructor
  clears +0x1c/+0x20 before the first store). vector<THotkey>::operator() 0x496570 grows with new[]
  (TCallback's constructor on every new entry), copies 12-byte entries by swapping and frees with
  operator delete[] (0x523812): the template's code. The out-of-line destructors are delete[] of the
  entry pointer, whose offset gives the element size (0x1c: 4 bytes, 0x30: 0x18, 0x34: 0x1f, 0x88:
  0x70, matrix's 0x10).
- **names.tsv** (high). The three low rows of GUI.obj's instances (0x496570, 0x4973f0, 0x4976f0) and
  the four medium destructor rows are high now; vector_talkfilter_ctor is
  vector_TTalkFilterPtr_ctor_init and vector_TTimerPtr_ctor vector_TTimerPtr_ctor_init, like
  vector_int_ctor_init and vector_TSkinButtonPtr_ctor_init. Rows keep C identifiers (simplify.py
  substitutes them into the pseudocode and callcheck.py reads Class_method); the note of each row
  names the instance as Container.h writes it, e.g. vector<TTalkFilter *>::vector(min, max, block, init).
- **tools/triage.py's header table** (medium for the rule). A header function counts as written when
  src/ names its address (the "// 0x..." comments next to the definitions); names.tsv rows of kind stl
  count as not source. notes/source-map.md now lists only what is left (glh_extensions.h 0x4f9260).
  The rule is textual: a comment that names an address without defining the function would hide it.
- **Coverage** (high). A temporary build put an INT3 on the first byte of each image instance and of
  each of our compiled instances (addresses from the link map), counted the first hit per test and
  re-armed at the next test (build/scratch/cover, not committed); the full suite passed with it
  (14,015,187 checks, 0 failures). Every image instance was reached by its users' tests (ConsoleWindow,
  ContentWindows, Map, DialogElements, Dialogs, GUI, Help, Preferences, PreferencesFile, Talk,
  Objects, Creatures, Graphics, MapWindow, Status) except the out-of-line destructors 0x425b10,
  0x452c50, 0x452c60, 0x46bb60, 0x4b0b20 (only unwind funclets and atexit thunks call them),
  TBuffer::SetSize 0x4ca300 and the three functions that only throw (0x417de0, 0x417f00, 0x5075c0).
  Our static vectors' constructors run before main (the image's are called explicitly by the tests).
- **containertest.cpp** (DIFFTEST_ONLY=Container, before TestMapWindow): every image instance of
  vector, array, matrix and matrix3d next to ours on random sizes, blocks and init values; after each
  call the objects (all bytes but the entry pointer) and the whole allocated arrays are compared,
  operator()'s returned entry as an index. Both sides' operator new[] fill new memory with the same
  random pattern (as contentwindowstest.cpp does), so entries neither side initialises agree; the
  vector objects get a different pattern than their entries (otherwise an entry wrongly set from an
  uninitialised init looks like a fresh one: the first mutation run missed exactly that). The
  out-of-line destructors run on the image's objects. Also TLineFormat's constructor 0x418020,
  TBuffer::SetSize and GetChannelID 0x507060. Not run: the assertion and index-error paths (the
  image cannot throw in the child, its stringstream cannot run) and the three throwing functions.
  It draws from its own generator, so the later tests (MapWindow, MainWindow) get the same inputs
  as before: the full run is the old total plus its 192,832 checks.
- **Mutation check** (build/scratch/mutate13.py: each mutation rebuilds difftest.exe completely, since
  every object instantiates the templates and the linker keeps one COMDAT, then runs
  DIFFTEST_ONLY=Container): eight changes (the initialized constructor's last entry, initialized in
  the plain constructor, block 0 growing down, filling below min without init, growing up dropping the
  last entry, matrix3d's layout, matrix's ymin, array's entry), all detected after the pattern split.
  An equivalent change exists: dropping `entry[j - start] = h` in the growth loops (the old array is
  freed right after).

## Library region (issue #15)

Classified by Claude Opus 5.5, November 2, 2026.

- **Scope** (high). The library region is symbols.tsv's 1,102 functions of kind library from 0x50eae8
  (after vlong.obj) to the end of .text; the 14,716 Unwind@ funclets after 0x52fc20 are the whole
  program's EH cleanup code (kind eh-funclet) and are not counted. Every one of the 1,102 has a
  names.tsv row now (kind stl, the library first in the note; one row of kind game). Ghidra missed
  further code in MFC and the CRT (342 addresses that code or data refers to are no function start in
  symbols.tsv: vtable slots and SEH filters and handlers inside Ghidra's function bodies); they belong to the same
  library objects as their neighbours (file-ranges.tsv) and have no rows of their own.
- **Counts** (names.tsv notes): MFC 7.1 523, CRT 441, C++ library 58, ATL 7.1 45, Henry Spencer's
  regexp 15, import thunks 8, delay-load 6, Windows SDK multimon.h stubs 5, game code 1. By object block
  (file-ranges.tsv, symbols.tsv's file column): nafxcw.lib 569 (MFC with the ATL string templates and
  the multimon stubs it instantiates), libcmt.lib 441, libcpmt.lib 58, ATL 7.1 4 (CAtlBaseModule),
  delay-load 6, import thunks 8, regexp.c 15, Inventory.cpp 1.
- **How** (high for the named, medium/low for some helper names). Tibia.exe imports no CRT or MFC DLL,
  so the CRT is the static multithreaded libcmt.lib (with _mtinit/_getptd), MFC the static ANSI
  nafxcw.lib, the C++ library libcpmt.lib ("Copyright (c) 1992-2001 by P.J. Plauger" at 0x5c42e2). 850
  functions carry Ghidra's FID names (VS 2003 libraries); they keep them, made identifiers
  (`CWnd::~CWnd` -> CWnd_dtor, templates dropped, one leading underscore of C names dropped; duplicates
  get the address). The 252 others were classified by script (build/scratch/rx/classify.py) and by
  hand: their neighbours' library, RTTI of the vtables they set or sit in (scalar deleting destructors,
  destructors, vtable slots), message-map entries, __finally blocks (`push n; call _unlock`) named after
  their function, throw infos (AfxThrowMemory/InvalidArg/Resource/UserException), and the code itself
  for the ones game code calls (strstr, strchr, srand, localeconv, _ftol2, floor, rename, remove,
  _unlink, _fileno, bad_cast, std::_Lockit, std::_Mutex, _Mtxinit..., _String_base::_Xlen/_Xran,
  CWnd::Get/SetWindowPlacement, AfxGetAfxWndProc, AfxGetThread, CFrameWnd::OnSize, CAsyncSocket's
  constructor, AfxThrowOleException). 36 helper names stay low (MFC_<address>, <Class>_vslot_<n>):
  the library is certain, the function within it is not identified; none is called by game code.
- **atoi** (high). 0x51262a, called by the game (Dialogs, Player, Preferences, vlong), is `jmp atol`:
  atox.c's atoi. names.tsv called it atol; it is atoi now, 0x5125a2 is atol. The sources still write
  atol (same result on Win32): issue #30.
- **Game code in the region** (high). 0x559ea0 (Ghidra found it after the EH funclets) is Inventory.obj's
  dynamic initialiser of Containers (0x5cedd8), like the other objects' initialisers 0x559df0-0x559f81;
  file-ranges.tsv gives it to Inventory.cpp, tools/triage.py now applies file-ranges.tsv to library
  rows as well and makes such a row kind game. No other CipSoft code sits in the region: no zlib, MD5
  or other third-party strings; the sprintf/_snprintf family is the CRT's.
- **_fileno** (high). 0x523017 is the CRT's _fileno although the linker placed it among the MFC objects
  (after AfxInitialize); a one-function libcmt.lib row in file-ranges.tsv.
- **regexp: origin and version** (high). 0x50eb00-0x50fbb8 is Henry Spencer's regexp(3) of 1986 (the
  University of Toronto version posted to Usenet): its 19 messages are that version's ("NULL argument",
  "regexp too big", "out of space", "too many ()", "unmatched ()", "junk on end", "*+ operand could be
  empty", "nested *?+", "invalid [] range", "unmatched []", "internal urp", "?+* follows nothing",
  "trailing \\", "internal disaster", "NULL parameter", "corrupted program", "memory corruption",
  "corrupted pointers", "internal foulup"), MAGIC is 0234 (0x9c), NSUBEXP 10 (OPEN 20, CLOSE 30, the
  jump tables end at CLOSE+9), sizeof(regexp) 0x60 (the 1986 layout: startp, endp, regstart, reganch,
  regmust, regmlen, program), the size limit 32767 and `regsize` counting as in the original. Later
  versions differ in visible ways the binary does not have (the 4.4BSD/Tcl/Vim descendants rename the
  error function, add const, regsub's "damaged regexp fed to regsub" and regerror's "regexp(3): %s"
  belong to separate files). Not in the client: regsub.c, regerror.c, DEBUG's regdump/regprop/regnarrate
  and the STRCSPN fallback (the CRT's strcspn 0x511ee0 is called). The text was written from memory of
  the original (nothing was downloaded) and checked function by function against the assembly
  (build/scratch/rx/rx.asm): every node, flag, test, jump table (regatom 0x50f990/0x50f9b4, regmatch
  0x50efdc/0x50f010, regrepeat 0x50ec88) and message agrees. No function has EH tables.
- **regexp: the client's changes** (the behaviour high, the form low). (1) regerror: every message
  goes through `if (p != NULL) p(message)` with p the .bss word 0x71f034, which nothing writes, so a
  failing regcomp returns NULL and regexec 0 silently. Written as a static regerror in regexp.c calling
  the global `regerrorhandler` (our name; it may have been static in the original); the call sites keep
  the original text. (2) Includes: the client's own "regexp.h" and <string.h>/<stdlib.h> in place of the
  K&R `extern char *malloc();`/`extern char *strchr();` declarations (modern headers reject them with
  malloc's prototype; the binary calls the CRT's functions directly, strlen is inlined). (3) regexp.h has
  prototypes with extern "C" for the C++ callers (the original's K&R declarations mean no parameters in
  C++); regsub and regerror are no longer declared. The file is compiled as C (src/Extern/regexp/
  regexp.c, K&R definitions kept); `#pragma warning(disable: 4018)` keeps /W3 quiet about the original's
  unsigned `strlen(...) >= len`. The order of the source is the original's; the binary's order is the
  compiler's (callees first), and its static functions take register arguments (regc in CL, regnext
  and regrepeat in EAX, regtail in EAX/ECX, regtry in ESI/EBX ...), which our compiler chooses itself.
- **regexp: data** (high). .bss 0x71f010 regdummy, 0x71f014 regsize, 0x71f018 regnpar, 0x71f01c regbol,
  0x71f020 regstartp, 0x71f024 regparse, 0x71f028 regcode, 0x71f02c regendp, 0x71f030 reginput,
  0x71f034 the error hook; .rdata 0x574094-0x5741e1 the messages and META. The object is 16-byte
  aligned like the game's (the libraries are not) and follows vlong.obj, so it is a project file (in a
  subfolder: src/Extern/regexp like glh's Extern\glh). Licence: Spencer's 1986 terms (redistribution
  allowed, altered versions marked) are in the file; the alteration is marked at its top.
- **Callers**. external.h's regcomp/regexec declarations are gone; Status.cpp includes
  Extern/regexp/regexp.h (the names were already the original's). statustest.cpp no longer forwards our
  regcomp/regexec to the image: difftest.exe links regexp.c, so Status.cpp's tests run ours.
- **tools/callcheck.py** (medium). names.tsv's new library rows would have made callcheck count calls
  such as memcpy, strlen, operator new and strcpy in our sources while the pseudocode keeps Ghidra's
  names (_memcpy, ...) or inlines them; a library-region name now counts only when Ghidra left the
  function unnamed and this file's pseudocode calls it by that name. Every file's count is the same or
  lower than before (Dialogs, Preferences, Script and sysinc lose the old atol/_fileno/rename/remove/
  srand differences).
- **Tests**. tools/difftest/regexptest.cpp (new, DIFFTEST_ONLY=Regexp, RX_SEED, RX_ROUNDS, RX_TRACE):
  both regcomps on hand-made patterns for every message and boundary (9 and 10 groups, ranges with ]
  and -, invalid ranges, quantifier errors, backslashes, chars above 0x7f, the size limit at exactly
  32766/32767), random strings of pattern characters, patterns from a small grammar and the client's
  LinkPattern (the image's 0x5c2560) with URL-like texts; compared: NULL or not, the messages, the
  allocation size, regstart/reganch/regmust/regmlen and every program byte; regexec on both programs
  (own and cross) with the result, the messages and all ten startp/endp (from a common sentinel), plus
  hand-made corrupted programs (wrong magic, unknown opcodes, a missing next node, STAR/PLUS of a
  BRANCH or BACK) and NULL arguments; a quarter of the rounds with both hooks NULL as in the client.
  229,958 checks, 0 failures. Coverage (RX_TRACE): every opcode including OPEN1-9/CLOSE1-9, regmust
  with rejections, anchored and regstart programs; every message except four: "junk on end",
  "internal urp" and "internal disaster" cannot happen (the original says so for the first; regpiece is
  never called on '\0', '|' or ')', and strcspn is at least 1 for a non-META character), "out of space"
  needs a failing malloc (not simulated). Mutation check (build/scratch/rx/rxmutate.py, 1,500 rounds): 44
  changes in every function, 41 detected; the three survivors are equivalent: SIMPLE for a two-character
  EXACTLY (regatom backs off before a quantifier, so a multi-character node is never a STAR/PLUS operand),
  reganch's shortcut in regexec (an anchored program fails at every later position anyway and leaves the
  same NULL startp/endp) and ANYBUT's `*reginput == '\0'` test (strchr finds the terminator, so the
  remaining test fails too).
## Review of the low-confidence decisions (issue #24)

Every low or medium entry above that could change behaviour, checked against the binary or a test.
Entries whose low part is only a name, a spelling or a file name are left out (names only: the Script.cpp
patch names, Talk.cpp's table, Objects.cpp's TProperties and flag names, the Preferences/Script gap,
ContextMenus/Control and start/Communication gaps, sysinc.cpp, Help.cpp's names, Inventory.cpp's function
names, surfaceDX9.cpp, Resources, Header code and every "Names"/"Classes" entry). Skipped as other issues':
TRect width/height and FromRect/ToRect (#29), SetChildArea's TRect (#27). Line numbers are those before this
review. (Settled in #29: see "TRect's members (issue #29)" below. Settled in #27: see "SetChildArea
(issue #27)" below.)

Tools (new): tools/xctable.py lists the __xc initialiser table with what each initialiser builds and the
atexit thunk it registers; tools\listing.cmd compiles src files like compile-check.cmd with assembly listings
(build\listing); tools/ehcompare.py prints ehinfo.py next to the unwind states and try blocks of our listing.

- **Static initialisers and destructors at exit**: our listings' dynamic initialisers (??__E) and atexit
  destructors (??__F) per object against the binary's __xc table 0x5c002c-0x5c00c4: the same initialisers in
  the same order, with an atexit destructor exactly where the binary registers one (GUI's Timers/Hotkeys,
  Help, Map's three, Network's TXTEA, Player, Preferences' five, Status' two, Talk's four, TRSA), none for
  the TCallbacks, SkinProperties, Inventory's three (their TObjects are zeroed, as by 0x559ea0/0x559ee0/
  0x559f00), the network buffers and StartTime, after two corrections: MainFrm.obj's TCallback 0x5d1674
  (0x559f20) was missing (now MainFrm.cpp's static NoCallback), and the unused `vlong(65537)` with an atexit
  destructor that Communication.obj (0x5c435c, 0x559da0) and TRSA.obj (0x71eff8, 0x55a1c0, right before
  TRSA.cpp's PublicExponent 0x71f004) construct was missing: one copy per includer of TRSA.h (vlong.obj has
  none, so not vlong.h), now `static vlong HeaderExponent(65537)` in TRSA.h (name ours). Both are unused data:
  no differential test can see them.

Result: 80 entries reviewed, none left open: 79 confirmed (8 of them settled by later issues that replaced
the guess, 15 without any possible effect on behaviour, one with a new test: nextToken's length limits in
scripttest.cpp), one confirmed with its explanation corrected (Map.cpp's TObject initialisers). Source
changes: MainFrm.cpp's NoCallback, TRSA.h's HeaderExponent, the TOffer size comment in Inventory.h.

| Line | Section | Entry | Verdict | Evidence | Change |
|---|---|---|---|---|---|
| 44 | Script.cpp | nextToken's string length check | confirmed + test | 0x4e0203 cmp Size,0x3e7 after every state; 0x4dfde1 cmp ebx,0x14 | scripttest.cpp TestLongTokens |
| 163 | Talk.cpp | Allocation and destruction | confirmed | talktest (behaviour); syntax only is low | - |
| 173 | Talk.cpp | ABI and field types | confirmed | Time only stored and passed to _localtime32; hidden-return ABI | - |
| 220 | Objects.cpp | NoObject 0x719da0 is Objects.cpp's | confirmed (behaviour) | .bss, no initialiser, never written | - |
| 225 | Objects.cpp | Container.h array<T> destructor | confirmed | atexit 0x55a2c0 frees array<TMapPoint>'s entry (+8); ReleaseObjects | - |
| 250 | Objects.cpp | Parameter passing | confirmed | references and pointers are the same ABI; objectstest | - |
| 255 | Objects.cpp | Obj.Data2 holds the hook flag | confirmed | 0x4d6f0f mov eax,[esi+8]; sub eax,0x12; je; dec eax; je | - |
| 305 | Player.cpp | Member functions of CPlayerApp | confirmed (behaviour) | ReadErrorFile writes ECX before reading it; FatalError only push ecx | - |
| 382 | Surface.cpp | TSurface slot parameter lists | confirmed (superseded) | slots typed and tested by issues #9-#11 | - |
| 393 | Surface.cpp | Removed try block in SurfaceInitInstance | confirmed | tools/ehcompare.py (our /EHsc compile): 4 states, 1 try block in both | - |
| 448 | Help.cpp | vector<int> destructor at exit | confirmed (superseded) | atexit 0x55a2b0 frees 0x5ce558; Container.h ~vector since #13 | - |
| 453 | Inventory.cpp | CounterOffer 4 bytes after OwnOffer | confirmed | no reference to 0x5d116c-0x5d116f | Inventory.h comment |
| 486 | MainFrm.cpp | PacketHeader 0x5d1644 is MainFrm.cpp's | confirmed (behaviour) | .bss, no initialiser or atexit | MainFrm.cpp NoCallback 0x5d1674 |
| 494 | MainFrm.cpp | Eliminated try blocks | confirmed | tools/ehcompare.py (our /EHsc compile): 11 states, no try block in all three | - |
| 526 | Creatures.cpp | Trailing gap: template destructor open question | confirmed (superseded) | Container.h ~vector since #13 | - |
| 542 | Creatures.cpp | Static initialisers (NoCallback objects) | confirmed | the __xc table 0x5c002c-0x5c00c4: TCallback(NULL,0), no atexit, one per object | MainFrm.cpp NoCallback |
| 548 | Creatures.cpp | Source order | confirmed (no behaviour) | order of definitions; creaturestest | - |
| 579 | Creatures.cpp | Dropped try blocks | confirmed | tools/ehcompare.py (our /EHsc compile): AddCreature 16, UpdateSkulls 11 states, no try block | - |
| 597 | Creatures.cpp | Types | confirmed | 0x40000000 compares jb/sbb; SkullTime jl/jle | - |
| 621 | Map.cpp | 0x5d16b4 left to MainWindow.cpp | confirmed | initialiser 0x559f30 between MainFrm's and Map's | - |
| 660 | Map.cpp | TObject's default constructor clears it | confirmed; explanation corrected | 0x4a4450 clears 3 words; 0x559ee0/0x559f00 zero the offers' TObjects | note |
| 668 | Map.cpp | TRGBColor constructors | confirmed | store order irrelevant; maptest/graphicstest | - |
| 676 | Map.cpp | Colours by value | confirmed | hidden return pointer = by value | - |
| 705 | Map.cpp | Parameter forms | confirmed | references and pointers are the same ABI; maptest | - |
| 724 | Map.cpp | Locals that the catch blocks print | confirmed (error path only) | only TEND1(i)'s value on an error path | - |
| 730 | Map.cpp | TEffect union | confirmed | offsets from the accessors; maptest; no type-based aliasing in MSVC | - |
| 767 | graphics.cpp | DialogWindow/graphics gap (header placement) | confirmed (no behaviour) | placement of identical inline copies | - |
| 806 | graphics.cpp | Surface slots +0x50/+0x54 parameters | confirmed | sub esp,0xc copies = TRGBColor by value; graphicstest | - |
| 815 | graphics.cpp | TVideoCacheItem's overlapping fields | confirmed | 0x48b3f0 store order; graphicstest memcmp of every item | - |
| 823 | graphics.cpp | TRGBColor's default constructor zeroes | confirmed | DrawOutfit overwrites the array before use; SkinProperties constructor byte for byte | - |
| 830 | graphics.cpp | Dropped code with exception states left | confirmed | tools/ehcompare.py (our /EHsc compile): same states, no try block; DrawPicturePart 26 states, try 15-15 | - |
| 865 | graphics.cpp | Data not static | confirmed (no behaviour) | linkage only | - |
| 871 | graphics.cpp | DrawPicturePartClipped's clip TRect by value | confirmed | sub esp,0x10 and four stores | - |
| 946 | GUI.cpp | FitToChildren/FitToChild take a TRect by value | confirmed | 16-byte block; any 16-byte struct is the same ABI | - |
| 956 | GUI.cpp | NumberOfIcons as Data490 | confirmed (no behaviour) | same offset; SkinProperties byte for byte | - |
| 973 | GUI.cpp | TFont() and DefaultFontColor | confirmed | 0x5c1954, 0x5c1230, 0x5c066c all (224,224,224) | - |
| 1041 | MapWindow.cpp | NoCallback 0x7152e8 form | confirmed | the __xc table 0x5c002c-0x5c00c4 | - |
| 1078 | MapWindow.cpp | TObject converts to int | confirmed | compile-check; mapwindowtest and the other TObject users | - |
| 1098 | MapWindow.cpp | TDrawing constructors | confirmed | new TDrawing[8] zeroing loop at 0x4c1ab4; window dumps | - |
| 1101 | MapWindow.cpp | Surface slots (+0x9c interpretation) | confirmed (no behaviour) | code mirrors the binary; mapwindowtest | - |
| 1117 | MapWindow.cpp | Limit | confirmed | clamp in either order; window tests | - |
| 1194 | MainWindow.cpp | NoCallback 0x5d16b4 form | confirmed | the __xc table 0x5c002c-0x5c00c4 | - |
| 1229 | MainWindow.cpp | Source order | confirmed (no behaviour) | order of definitions | - |
| 1261 | MainWindow.cpp | Evaluation order | confirmed | the reads between are getters; new[] 0x523817; mainwindowtest | - |
| 1267 | MainWindow.cpp | SkinProperties.Style0D8 retyped | confirmed | guitest: constructor and InitSkinProperties byte for byte | - |
| 1278 | MainWindow.cpp | External sketches | superseded | replaced by the real classes (#3-#8) | - |
| 1321 | DialogWindow.cpp | NoCallback 0x5cc428 form | confirmed | the __xc table 0x5c002c-0x5c00c4 | - |
| 1351 | DialogWindow.cpp | No inline header code | confirmed (no behaviour) | placement of identical code | - |
| 1360 | DialogWindow.cpp | Source order | confirmed (no behaviour) | order of definitions | - |
| 1385 | DialogWindow.cpp | External sketches | superseded | real classes (#6, #7) | - |
| 1436 | ContextMenus.cpp | NoCallback form | confirmed | the __xc table 0x5c002c-0x5c00c4 | - |
| 1460 | ContextMenus.cpp | Inline header functions | confirmed (no behaviour) | placement of identical code | - |
| 1468 | ContextMenus.cpp | Source order | confirmed (no behaviour) | order of definitions | - |
| 1502 | ContextMenus.cpp | Text buffers | confirmed (no behaviour) | sprintf of names <= 29 chars + <= 11 fixed; _snprintf limit 200 | - |
| 1505 | ContextMenus.cpp | Data types changed elsewhere | confirmed / superseded | TRect by value = same block; sketches replaced (#5, #7) | - |
| 1543 | ConsoleWindow.cpp | Range (In/TRGBColor() copies) | confirmed (no behaviour) | identical COMDAT copies | - |
| 1573 | ConsoleWindow.cpp | NoCallback form | confirmed | the __xc table 0x5c002c-0x5c00c4 | - |
| 1611 | ConsoleWindow.cpp | Source order | confirmed (no behaviour) | order of definitions | - |
| 1617 | ConsoleWindow.cpp | Arrange's inlined SetCurrentLineNumber(-1) | confirmed | the same store; consolewindowtest | - |
| 1660 | ConsoleWindow.cpp | SkinProperties.Style9E8 retyped | confirmed | guitest byte for byte | - |
| 1667 | ConsoleWindow.cpp | DialogElements.cpp sketches | superseded | real classes (#7) | - |
| 1786 | ContentWindows.cpp | TSubWindowBody | confirmed | folded 0x425a00 returns true; contentwindowstest | - |
| 1800 | ContentWindows.cpp | Limit (header) | confirmed | 0x425920; tests | - |
| 1805 | ContentWindows.cpp | Source order | confirmed (no behaviour) | order of definitions | - |
| 1841 | ContentWindows.cpp | SkinProperties retyped | confirmed | guitest byte for byte | - |
| 1855 | ContentWindows.cpp | Surface slots named | confirmed | pushed arguments; recorded surface | - |
| 1859 | ContentWindows.cpp | DialogElements.cpp sketches | superseded | real classes (#7) | - |
| 1981 | DialogElements.cpp | Names (parameter roles) | confirmed | dialogelementstest | - |
| 2005 | DialogElements.cpp | Header code | confirmed (no behaviour) | placement of identical code | - |
| 2047 | DialogElements.cpp | SkinProperties retyped | confirmed | guitest byte for byte | - |
| 2117 | SurfaceDX.cpp | TBlitSize and the factory | confirmed | surfacedxtest | - |
| 2140 | SurfaceDX.cpp | Shared data ownership | confirmed (behaviour) | .bss zero; 0x5c2568/6c = 0x100; no initialisers | - |
| 2251 | Dialogs.cpp | Leading run (CompareNames, TIgnoreEntry) | confirmed | 0x46c0d0 clears Name[0] and Permanent; containertest | - |
| 2297 | Dialogs.cpp | Creators' parameter types | confirmed | pointers/arrays same ABI; dialogstest | - |
| 2345 | Dialogs.cpp | SkinProperties retyped | confirmed | guitest byte for byte | - |
| 2554 | Unknown-owner data | 0x5c2564 is Surface.cpp's | confirmed (behaviour) | .data word 0xffffffff; no initialiser | - |
| 2561 | Unknown-owner data | XOR tables are sysinc.cpp's | confirmed (behaviour) | .rdata constants; owner only | - |
| 2586 | Unknown-owner data | 0x5d16b4 already settled | confirmed | the __xc table 0x5c002c-0x5c00c4 | - |
| 2607 | SurfaceGL.cpp | glh_extensions.h #line | confirmed | our listing pushes 173 and 191 like 0x4f92e2/0x4f93ca | - |
| 2632 | SurfaceGL.cpp | TSurface slots +0x04/+0x08/+0x80 | confirmed | AddDirtyRect's TRect = the 16-byte block; surfacegltest | - |
| - | Communication/TRSA | unused vlong(65537) statics (not in the log before) | corrected | the __xc table 0x5c002c-0x5c00c4: 0x559da0, 0x55a1c0 | TRSA.h HeaderExponent |

## TRect's members (issue #29)

Claude Opus 5.5, November 2, 2026.

- **Evidence** (every assertion string in Tibia.exe that names a member of a rectangle, from a scan of the
  printable strings for width/height/.x/.y/Rect/skin, build/scratch/slotcalls.py and directcalls.py for the
  call sites):
  - "FromRect.width>0 && FromRect.height>0" (0x56f2a4) with the value names "FromRect.width" and
    "FromRect.height" (0x56f5cc/0x56f5dc): the Blit and BlitRect slots of all three engines (SurfaceDX.cpp
    lines 811/824, surfaceDX9.cpp 1302/1315, SurfaceGL.cpp 1073/1086). BlitRect already took a TRect, so
    FromRect is a TRect, and its size members are width and height.
  - "FromRect.width+1<=Rect[TEMP_SURFACE].right && FromRect.height+1<=..." and "ToRect.width<=
    Rect[STRETCH_SURFACE].right && ToRect.height<=..." (0x56f178/0x56f1d8): StretchBlitUnclipped and
    StretchBlit of SurfaceDX.cpp and StretchBlit of surfaceDX9.cpp.
  - TRect members of SkinProperties named with lower-case .x/.y: "x+SkinProperties.StatusWindow.
    HitpointsBar.x>=0" (and ManaBar), "y+ItemY+SkinProperties.BuddylistWindow.Logo.y>=0" (all three are
    TRects in GUI.h; Buttons.Texture and CheckBox.Item are TGUIPoints).
  - The other rectangles of the client use the same lower-case names: TSkin ("Skins[skin].width",
    "Skins[skin].height") and TDrawing ("DrawingList.x", ".y", ".width", ".height", "DrawingList[i].skin").
  - No string names a rectangle member Width or Height. The capitalised Width/Height strings are
    parameters and locals ("Width>0", "x>=0 && y>=0 && Width>0 && Height>0" of WriteImage, "_Width"),
    TComponent's getWidth()/getHeight() and Objects.cpp's "Entry.Width"/"Entry.Height" (not rectangles).
- **TRect is {x, y, width, height}** (high for width/height, which the assertions name on a TRect parameter;
  x and y were lower case already). graphics.h's members and its constructor's parameters were renamed.
  The rename was compiler-checked: graphics.h's members renamed, every src file and every difftest file
  compiled with column numbers (build/scratch/cc.cmd), and each "'Width': is not a member of 'TRect'"
  error renamed at its exact line and column (build/scratch/fixmembers.py) until the build was clean, so no
  Width/Height of another type (TComponent, TSurface, TFullscreenMode, TFont, the window sizes, the
  parameters) was touched: 906 uses in 23 files (11 in src, 12 in tools/difftest), plus two comments
  (GUI.cpp's FitToChild, DialogWindow.h's Area). No assertion text changed: no TASSERT stringified a TRect member.
- **TSkin stays its own struct** (low). It has TRect's layout and now its member names, but nothing in the
  binary uses a TSkin as a TRect: the callers of DrawPicturePart (0x490c60) push a skin's four members as
  separate ints (eight pushes, no 16-byte block), DrawSkin takes the skin number, and no code copies a skin
  into a TRect parameter. A typedef or a TRect array would give the same code; with no evidence for it,
  Skins.h keeps TSkin.
- **The blit slots take TRects** (high for the 16-byte blocks and the names FromRect/ToRect). Every call
  through the slots builds the rectangle as a 16-byte block on the stack (`sub esp,0x10` and four stores),
  separate from the pushes of the ints: graphics.cpp's ten calls (0x48fa3c Blit: `sub esp,0xc` for the
  colour key and `sub esp,0x10` for Item->x, Item->y, 32, 32, then four pushes; 0x48fc13 BlitClipped:
  clip, colour key, source; 0x48ff5d StretchBlit: clip, highlight, colour key, source, then the surface
  pushed, then the target as a second block, then ToSurface) and every engine-internal call (StretchBlit's
  and StretchBlitUnclipped's Blit/BlitClipped calls, BlitClipped's and BlitRect's BlitRectClipped calls:
  two blocks and four pushes). The new signatures (Surface.h and the three engines; the parameter order and
  the stack layout are the old ones):
  - +0x30 BlitRect(int ToSurface, int x, int y, int FromSurface, TRect FromRect) (parameter renamed from
    Source: its assertion names FromRect)
  - +0x34 Blit(int ToSurface, int x, int y, int FromSurface, TRect FromRect, TRGBColor ColorKey)
  - +0x38 BlitRectClipped(int ToSurface, int x, int y, int FromSurface, TRect FromRect, TRect Clip)
  - +0x3c BlitClipped(int ToSurface, int x, int y, int FromSurface, TRect FromRect, TRGBColor ColorKey,
    TRect Clip)
  - +0x44 StretchBlitUnclipped(int ToSurface, TRect ToRect, int FromSurface, TRect FromRect,
    TRGBColor ColorKey, TRGBColor Highlight) (no caller outside the engines; its assertions name ToRect and
    FromRect like StretchBlit's, and its own calls build the same blocks)
  - +0x48 StretchBlit(int ToSurface, TRect ToRect, int FromSurface, TRect FromRect, TRGBColor ColorKey,
    TRGBColor Highlight, TRect Clip)
  - +0x40 StretchBlitRect keeps (ToSurface, TRect Destination, FromSurface, TRect Source): it took TRects
    already, and no assertion names its parameters (the names stay ours, low).
  The x, y of Blit/BlitClipped/BlitRect/BlitRectClipped stay ints: the calls push them separately (four
  pushes: ToSurface, x, y, FromSurface). WriteImage, WriteMaskedImage and SetSurfaceRect keep their ints
  (six and eight pushes, no block; WriteImage's assertion "x>=0 && y>=0 && Width>0 && Height>0" names
  separate parameters).
- **The assertions are real expressions now**: SurfaceDX.cpp's TBlitSize view and the TASSERT_TEXT and
  TASSERT2_TEXT macros of surfaceDX9.cpp and SurfaceGL.cpp are gone; the assertions are TASSERT and
  TASSERT2(FromRect.width>0 && FromRect.height>0, FromRect.width, FromRect.height), whose texts are the
  binary's. Inside StretchBlit the second and last blits pass TRect(0, 0, ToRect.width, ToRect.height)
  for the STRETCH_SURFACE part (the binary builds a new block there; whether the original wrote a TRect
  temporary or a local is unknown, the values are the same).
- **Tests**: the test surfaces were changed to the new signatures (testsurface.h's TTestSurface and
  graphicstest.cpp's recorders, which log the same values in the same order as before) and the calls of
  surfacedxtest.cpp, surfacedx9test.cpp and surfacegltest.cpp build TRects where they passed four ints;
  the image's functions are called through the same declarations (same stack layout). The difftest totals
  are unchanged (14,492,680 checks, 67 in sysinctest.exe, 3,201 vlong; 0 failures).
- **callcheck**: no file's list changed (SurfaceDX.cpp 14, surfaceDX9.cpp 13, SurfaceGL.cpp 20,
  graphics.cpp 10, and 18 other files that use TRects: identical output before and after).
- **For #27** (SetChildArea): TRect's members are x, y, width, height now, so SetChildArea(TComponent
  *Child, TRect Area) can use Area.width/Area.height; GUI.cpp's FitToChild/FitToChildren already take a
  TRect by value. The callers in Dialogs.cpp pass R.x, R.y, R.width, R.height of SkinProperties' TRects
  today and can pass the TRect itself.
  Settled in #27: see "SetChildArea (issue #27)" below.

## SetChildArea (issue #27)

Claude Opus 5.5, November 2, 2026.

- **Evidence** (build/scratch/areacalls.py: every indirect call through slot +0x8c and every direct call of
  0x49f5d0, with the argument pattern since the previous call and the function from names.tsv). The slot
  calls of the surface engines (TSurfaceDX9's D3D device calls, 0x479d3c's GetSurface()->+0x8c) are other
  vtables and are left out. That leaves 136 calls (102 direct, 34 through the slot), and every one of them
  builds the area as a 16-byte block: `sub esp,0x10`, four stores and one push of the child. The Arrange
  functions with a larger frame have `sub esp,0x184`/`0x18c` for the frame and then the same block. No call
  pushes four separate values. The source had 136 calls as well, and the counts per file are the same
  (ConsoleWindow 4, ContentWindows 16, ContextMenus 1, DialogElements 6, DialogWindow 5, Dialogs 103,
  MapWindow 1). The calls by file, with the address of each call instruction (all build a block):
  - ConsoleWindow.cpp (4): TConsoleWindow_SetSize (0x41deec, 0x41df22, 0x41df58); TConsole_Arrange (0x4214f9).
  - ContentWindows.cpp (16): TInventoryView_ctor (0x42a873, 0x42a8d7, 0x42a937); TTradeOffers_SetSize
    (0x43183e); TTradeNames_SetSize (0x432b6c, 0x432ba2, 0x432bd8); TTradeButtons_SetSize (0x43353c,
    0x433596); TTradeWindowBody_SetSize (0x4340d5, 0x43410b, 0x434141); CreateSmallInventoryWindow (0x436690,
    0x4366f7, 0x43675a); TAutomapWindow_ctor (0x436bf8).
  - ContextMenus.cpp (1): THelpBubble_SetSize (0x43d838).
  - DialogElements.cpp (6): TLink_Arrange (0x456c70); TListBox_StartEdit (0x46310f); TFontListBox_StartEdit
    (0x464926); TCheckBox_ctor (0x467779); TEditText_Arrange (0x468768); TListBox_Arrange (0x46a1aa).
  - DialogWindow.cpp (5): TSubWindow_SetSize (0x4882b0, 0x488322, 0x48835c); TSubWindow_SetIcon (0x48a143);
    TSubWindow_SetTitle (0x48a333).
  - Dialogs.cpp (103): TDialog_Arrange (0x46cd7b, 0x46cdb1, 0x46cde7); TReconnectWindow_ctor (0x46e6e4,
    0x46e70a, 0x46e730); TLoginDialog_ctor (0x46ee66, 0x46eea2, 0x46eed8, 0x46ef6d, 0x46effa);
    TCharacterListDialog_ctor (0x46f6a5, 0x46f729, 0x46f8bf); TMoveObjectsDialog_ctor (0x47098c, 0x470a05);
    TTextDialog_ctor (0x471704, 0x4717a2, 0x471836); TBugReportDialog_ctor (0x472fdb, 0x473068);
    TNewChannelDialog_ctor (0x4736a7, 0x4736d7, 0x47379d, 0x4737cd); TPrivateChannelDialog_ctor (0x473f7e,
    0x473fae); TReportDialog_ctor (0x4745d0, 0x4745fe); TAddBuddyDialog_ctor (0x474c3d, 0x474c6b);
    TEditBuddyDialog_ctor (0x475348, 0x4753a6, 0x47543c, 0x4755ac, 0x475612, 0x47569f); TUpdateDialog_ctor
    (0x475b4c, 0x475bab); THintsDialog_ctor (0x476db1, 0x476e33); TEditMarkDialog_ctor (0x477e86, 0x477f07,
    0x477f9d); TOptionsDialog_ctor (0x478cfc); TGeneralOptionsDialog_ctor (0x4794a5, 0x4794d8, 0x479509,
    0x47953a, 0x47956d); TGraphicsOptionsDialog_ctor (0x479c6b, 0x479c9e, 0x479ccc, 0x479cfa);
    TAdvancedGraphicsDialog_ctor (0x47a754, 0x47a785, 0x47a7b8, 0x47a7e9); TConsoleOptionsDialog_ctor
    (0x47b145, 0x47b178, 0x47b1a9, 0x47b1da, 0x47b20d); TNetworkOptionsDialog_ctor (0x47b69c);
    TOutfitDialog_ctor (0x47d1d2); TEditListDialog_ctor (0x47d910, 0x47d9f5); TIgnoreListDialog_ctor
    (0x47dc36, 0x47dcc4, 0x47dd55, 0x47de77, 0x47df05, 0x47df6a, 0x47dfcf, 0x47e310, 0x47e3a6, 0x47e437,
    0x47e559, 0x47e5e7, 0x47e64c); THotkeyDialog_ctor (0x47eee0, 0x47ef7d, 0x47f00c, 0x47f0c2, 0x47f12c);
    TErrorReportDialog_ctor (0x47f448, 0x47f47b, 0x47f4a9, 0x47f4da); CreateInfoDialog (0x480a5a, 0x480a8d,
    0x480ac0); TRuleViolationDialog_ctor (0x484744, 0x4847fa, 0x48488e, 0x484912, 0x48499f, 0x484a0e,
    0x484a9b, 0x484b0c, 0x484b99, 0x484c46, 0x484cc2).
  - MapWindow.cpp (1): TMapWindow_Arrange (0x4bfe40).
- **SetChildArea(TComponent *Child, TRect Area)** (high for the 16-byte block; the type is TRect because it
  is the only 16-byte struct, and FitToChild/FitToChildren take theirs the same way). The parameter name
  Area is ours (low). The function's three assertions ("Child->Parent==this", "Width>0" and "Height>0"
  with the values "Width" and "Height") read the window's Width and Height at this+0x1c/+0x20. The catch
  block (line 1690) names nothing, so the binary does not name the area. Area matches FitToChild's
  parameter and the function's name. The body changes Area.x, Area.y, Area.width and Area.height in place,
  as it changed the four int parameters before. The binary keeps them in registers and locals, and the
  values are the same. The stack layout is the old one: the four ints were at the same offsets as the
  TRect's members.
- **The callers**: 118 pass a SkinProperties TRect whose x, y, width and height they used to pass one by
  one; they pass the TRect itself now (Dialogs.cpp 100, ContentWindows.cpp 10, ConsoleWindow.cpp 4,
  DialogWindow.cpp 2, ContextMenus.cpp 1, DialogElements.cpp 1). 18 build TRect(...):
  - computed areas (8): DialogElements.cpp's TLink::Arrange, TEditText::Arrange, TListBox::Arrange and the
    two StartEdits, Dialogs.cpp's TReconnectWindow constructor (three; 0x46e6cc builds the block from the
    computed heights). guitest.cpp's tree operation 9 does the same.
  - pairs of TGUIPoints (10): the position and the size are two points in SkinProperties (MapWindow.cpp's
    Data140/Data148, DialogWindow.cpp's Style194 Data28/30, Data60/68, Data70/78, ContentWindows.cpp's six
    Style6FC pairs). In the binary each pair is copied as 16 contiguous bytes (0x4bfe1d copies
    0x5cd108..0x5cd117), the same code a TRect copy gives. Whether these pairs are TRects is a SkinProperties
    retype: issue #31. The source builds TRect(P.x, P.y, Q.x, Q.y) (medium; the values and the layout are
    the binary's either way).
    Settled in #31: each pair is one TRect, and the ten callers pass it (see "SkinProperties point pairs
    (issue #31)" below).
- **Tests**: no test fake overrides SetChildArea; guitest.cpp's call builds a TRect. Totals unchanged
  (14,492,680 checks in difftest.exe, 67 in sysinctest.exe, 3,201 vlong; 0 failures). tools\compile-check.cmd
  is clean. The first full run had 125 "dialogs differ" failures in the outfit dialog, caused by the
  harness and not by this change, so it was run again. The original side's TColorButton Next pointers sat at
  0x00abab98-like addresses, and dialogstest's IsPattern prints those as fill pattern. Main's build (from
  git archive 82ae6f9) fails the same way with the default %TEMP% (2 failures in the rule violation dialog,
  a word printed P on one side and hex on the other), and both builds pass with build.cmd's TEMP. The
  second full run passed. The dump's dependence on addresses is issue #32.
- **callcheck**: identical output before and after for GUI.cpp (20 of 63 functions differ), ConsoleWindow.cpp
  (55 of 76), ContentWindows.cpp (82 of 140), ContextMenus.cpp (24 of 36), DialogElements.cpp (112 of 160),
  DialogWindow.cpp (23 of 47), Dialogs.cpp (111 of 139) and MapWindow.cpp (14 of 26). SetChildArea is a
  virtual call (indirect) or a qualified TWindow:: call in the decompile, so the tool does not count it, and
  TRect's constructor is not a names.tsv function.

## SkinProperties point pairs (issue #31)

Claude Opus 5.5, November 2, 2026.

- **Evidence** (build/scratch/i31/refs.py in the i31 worktree: every instruction of .text whose operand is an
  absolute address inside one of the ten 16-byte pairs, with the function from names.tsv, and every dword
  outside .text that points into them):
  - Each pair has exactly eight references: InitSkinProperties' (0x498160) four stores, one per word, and the
    four loads of its one SetChildArea call, which copy the words in order (+0, +4, +8, +c) into the
    16-byte block (`sub esp,0x10`, four stores, the push of the child, the call through +0x8c). No other
    function reads or writes any word of them, and no dword in .rdata, .data or .rsrc points into them.
  - No code takes the address of SkinProperties (0x5ccfc8) or of the blocks Style194 (0x5cd15c) and
    Style6FC (0x5cd6c4) except the static initialiser 0x559e50, which passes SkinProperties to its
    constructor 0x496800. The block constructors 0x4954f0 and 0x495700 (and 0x496800 for +0x140) zero every
    word of the pairs in order, which TRect's constructor and two TGUIPoint constructors give alike.
  - No assertion string names them: the only "SkinProperties." strings name PlayerStates.NumberOfIcons,
    StatusWindow.HitpointsBar/ManaBar, BuddylistWindow.Logo, Buttons.Texture and CheckBox.Item.
  - So nothing in the binary uses either half of a pair as a point; the only use is the 16-byte copy that a
    TRect passed by value gives, the same criterion as Dialogs.cpp's retype (issue #8) and the 118 TRects
    passed to SetChildArea (#27).
- **Each pair is one TRect** (medium-high for the type: the code is the same for either type, and the only
  use is the TRect copy; low for the names, kept from the first member because no string names them). The
  values are InitSkinProperties' (x, y, width, height); the calls are the call instructions:

  | Member (address) | Was | Call | Area of | Values |
  |---|---|---|---|---|
  | Data140 (0x5cd108) | Data140/Data148 | 0x4bfe40 TMapWindow::Arrange | the hint button | -35, 5, 32, 32 |
  | Style194.Data28 (0x5cd184) | Data28/Data30 | 0x48a333 TSubWindow::SetTitle | the title | 20, 4, -41, 10 |
  | Style194.Data60 (0x5cd1bc) | Data60/Data68 | 0x488322 TSubWindow::SetSize | Resizer1 | 3, -16, 13, 13 |
  | Style194.Data70 (0x5cd1cc) | Data70/Data78 | 0x48835c TSubWindow::SetSize | Resizer2 | 0, -4, 0, 0 |
  | Style6FC.Data04 (0x5cd6c8) | Data04/Data0C | 0x434141 TTradeWindowBody::SetSize | Names | 0, 0, 0, 12 |
  | Style6FC.Data34 (0x5cd6f8) | Data34/Data3C | 0x432bd8 TTradeNames::SetSize | its Separator | 0, -2, 0, 2 |
  | Style6FC.Data58 (0x5cd71c) | Data58/Data60 | 0x43410b TTradeWindowBody::SetSize | Offers | 0, 12, 0, -26 |
  | Style6FC.DataA8 (0x5cd76c) | DataA8/DataB0 | 0x43183e TTradeOffers::SetSize | its Separator | 77, 0, 2, 0 |
  | Style6FC.DataB8 (0x5cd77c) | DataB8/DataC0 | 0x4340d5 TTradeWindowBody::SetSize | Buttons | 0, -26, 0, 0 |
  | Style6FC.DataE8 (0x5cd7ac) | DataE8/DataF0 | 0x43353c TTradeButtons::SetSize | its Separator | 0, 0, 0, 2 |

  No pair was kept as two points.
- **The source**: GUI.h declares the ten TRects at the old offsets (the layout is the same: a TRect is two
  TGUIPoints' 16 bytes, both int-aligned). InitSkinProperties stores the second point's x and y as the
  TRect's width and height, in the old order (the binary interleaves the stores of neighbouring members as
  its register allocation goes; only the bytes after the function matter, and guitest compares them). The
  ten callers pass the TRect itself instead of TRect(P.x, P.y, Q.x, Q.y); no caller of SetChildArea builds
  an area from two points any more (the 8 computed TRect(...) areas of #27 remain). DialogWindow.h's
  comments of Resizer1/Resizer2 name the new members. dialogwindowtest.cpp's random Style194 values set
  Data28/Data60/Data70.width and .height where they set the second points' x and y (the same statements in
  the same order, so the same Random() sequence); no other test referred to the members.
- **Tests**: totals unchanged (14,492,680 checks in difftest.exe, 67 in sysinctest.exe, 3,201 vlong;
  0 failures) on the first complete run (the run before stopped at dialogwindowtest.cpp's compile errors,
  which led to its change above). tools\compile-check.cmd is clean.
- **callcheck**: identical output before and after for GUI.cpp (20 of 63 functions differ), MapWindow.cpp
  (14 of 26), DialogWindow.cpp (23 of 47) and ContentWindows.cpp (82 of 140).

## external.h removed (issue #14)

Settled by Claude Opus 5.5, November 2, 2026. src/external.h is gone; every declaration is in the header of
the file that defines it, and every file includes the headers it uses (compile-guided: external.h was removed
from all 36 includers, then each file got the headers the compiler asked for). Evidence scripts:
build/scratch/texttable.py (the text table's copies and texts), copyrefs.py (every code reference into and
around each copy) and gaps.py (the data between the copies), all in the i14 worktree.

- **What external.h held and where it went** (high):

  | Content | Now |
  |---|---|
  | TObject (Objects.cpp's type) | Objects.h, unchanged (comment kept); Status.h, Inventory.h, Map.h, Creatures.h, DialogElements.h, ContextMenus.h and ContentWindows.h include Objects.h |
  | PacketHeader (0x5d1644, "owner unknown") | already defined in MainFrm.cpp and declared in MainFrm.h; Network.cpp includes MainFrm.h |
  | TFullscreenMode forward declaration | dropped (Preferences.h has the type) |
  | #include of Talk.h, sysinc.h, GUI.h, ConsoleWindow.h, DialogWindow.h, MainWindow.h, Surface.h, MainFrm.h, <ostream> | in the files that use them (below) |
  | the text tables of seven objects (externs) | src/Text.h (below) |
  | the action-state comment ("Status.cpp. Action state: 0 idle, ...") | Status.h, at ACTIONSTATE_NONE |
  | empty headings ("Map.cpp", "Script.cpp", ...) and the stale FUN_ remark | dropped |

  Includes added: Communication.cpp Text.h; ConsoleWindow.cpp Text.h; ContentWindows.cpp Text.h;
  ContextMenus.cpp ConsoleWindow.h, Text.h, sysinc.h; Control.cpp ConsoleWindow.h, MainWindow.h, Surface.h,
  Text.h, sysinc.h; Creatures.cpp GUI.h (its NoCallback), Text.h, sysinc.h; DialogElements.cpp, Dialogs.cpp,
  DialogWindow.cpp, GUI.cpp, Help.cpp, MapWindow.cpp Text.h; MainFrm.cpp GUI.h, MainWindow.h, Surface.h,
  Text.h; MainWindow.cpp ConsoleWindow.h, Text.h; Map.cpp, Script.cpp, Status.cpp sysinc.h; Network.cpp
  MainFrm.h; Player.cpp MainFrm.h, Text.h; Talk.cpp Text.h, graphics.h (TRGBColor), sysinc.h; Map.h and
  Creatures.h graphics.h (TRGBColor); Dialogs.h Text.h. Preferences.cpp, Surface.cpp, SurfaceDX.cpp,
  surfaceDX9.cpp and SurfaceGL.cpp needed nothing from it. No include cycle appeared: none of the headers
  that used to include external.h is included by Objects.h, graphics.h or Text.h.
- **The text header is src/Text.h, 147 `static const char *` entries** (high for the form, low for the
  file name and the names). The image has sixteen copies of the table in .data (every pointer to "You are
  poisoned" 0x55d1b0 starts one), each 147 pointers (0x24c bytes), all sixteen identical pointer for
  pointer. The code reads the entries from memory (`mov reg, [0x5c....]`), so they are not const pointers
  (a `char *const` would be folded into the code and land in .rdata); they are defined in a header with
  internal linkage, which gives exactly one copy per including object, and every object's code reads its
  own copy. Text.h defines all 147 in the image's order with the image's texts (generated from the binary,
  so the copies of the old test definitions are byte-identical; the test totals confirm it), so a .cpp that
  includes it reads the entry of its own copy: entry n of a copy is at the copy's address + 4n. A table
  (array) or 147 named variables give the same .data; named variables were kept because the users already
  read them by name. The existing names all landed on the entry of their address without a conflict
  (TextSelectType 70 and TextSelectType2 112 are different entries with the same text, as are
  TextEnterCharacterName 67/71 and TextEnterShortDescription 72/113). New names (ours): TextEnterFilename
  (73), TextServerAddress (90), TextServerPort (91), TextRequestClosed (126), TextChannelClosed (127),
  TextPermanentlyIgnoring (134), TextSignupURL (144).
- **The owners of the sixteen copies** (in .data order; each object's .data is contiguous, in link order):

  | Copy | Object | Evidence | Confidence |
  |---|---|---|---|
  | 0x5c0150 | Communication.obj | reads 126, 127 (0x411b60, 0x411eea) | high |
  | 0x5c041c | ConsoleWindow.obj | reads 12-19, 119, 120, 122, 146 | high |
  | 0x5c0678 | ContentWindows.obj | reads 0-11, 20-50, 52, 56, 57 | high |
  | 0x5c08d4 | ContextMenus.obj | no reads; between ContentWindows.obj's and Control.obj's | high |
  | 0x5c0b34 | Control.obj | reads 109-111, 114, 115, 145 | high |
  | 0x5c0d90 | Creatures.obj | no reads; after Control.cpp's LastPacketType 0x5c0d8c, before DialogElements' | high |
  | 0x5c0fe0 | DialogElements.obj | no reads; its DefaultFontColor 0x5c1230 (read by its header code) follows | high |
  | 0x5c123c | Dialogs.obj | reads 53-55, 58-72, 74-89, 92-108, 112, 113, 117, 118, 121, 123, 128-133, 135-142 | high |
  | 0x5c1498 | DialogWindow.obj | no reads; DialogWindow, graphics and GUI come in this order and only one table lies between Dialogs' and GUI's | medium |
  | 0x5c1708 | GUI.obj | no reads; GUI.cpp's DefaultFontColor 0x5c1954 follows directly | high |
  | 0x5c1960 | Help.obj | no reads; two copies lie between GUI.obj's and MainWindow.obj's, where Help, Inventory and MainFrm link | low (Inventory.obj possible) |
  | 0x5c1bb0 | MainFrm.obj | as above; the copy is followed by a DefaultFontColor, and of the three only MainFrm.cpp includes GUI.h (graphics.h) | low |
  | 0x5c1e0c | MainWindow.obj | reads 124, 143 | high |
  | 0x5c2064 | MapWindow.obj | reads 51, 125 | high |
  | 0x5c22cc | Player.obj | reads 116 | high |
  | 0x5c258c | Talk.obj | reads 134 (0x507843) | high |

  Each of those sixteen .cpp files includes Text.h (DialogElements.cpp, ContextMenus.cpp, Creatures.cpp,
  DialogWindow.cpp, GUI.cpp, Help.cpp and MainFrm.cpp read nothing from it). Text.h's header comment lists
  the table. The DialogElements.cpp comment that put its copy at 0x5c0d8c-0x5c1228 was off (0x5c0d8c is
  Control.cpp's LastPacketType, 0x5c0d90 Creatures.obj's copy); it now says 0x5c0fe0-0x5c122b.
- **Dialogs.h includes Text.h** (high): TReconnectBody::SetTime is inline in Dialogs.h and reads
  TextTryingToReconnect (entry 62); its out-of-line copy 0x46ba10 (Dialogs.obj's header code) reads
  Dialogs.obj's 0x5c1334. Dialogs.h's includers (Control.cpp, Dialogs.cpp) both have a copy. Text.h's other
  users are .cpp files.
- **Two Ghidra names and a misplaced text in Talk.cpp** (high): Talk.cpp defined
  `PTR_s_Your_request_has_been_closed__005c0348` and `PTR_s_The_channel_has_been_closed__You_005c034c`
  (Talk.h declared them) for Communication.cpp's two reads; 0x5c0348/0x5c034c are entries 126 and 127 of
  Communication.obj's own copy. Communication.cpp now reads TextRequestClosed/TextChannelClosed from its copy;
  the two globals are gone. Talk.cpp's `static const char *PermanentIgnoreMessage` (0x5c27a4) is entry 134
  of Talk.obj's copy: it reads TextPermanentlyIgnoring now. Same texts, never written: no behaviour change.
  No FUN_00/DAT_00/PTR_ name is left in src/.
- **Tests** (high): the window tests defined the entries their files declared extern (consolewindowtest,
  contentwindowstest, mainwindowtest, mapwindowtest, playertest); those definitions are gone (the src files
  have their copies). dialogstest.cpp's text globals and InitTexts (which pointed our side's entries at the
  image's texts) clashed with Text.h's statics through Dialogs.h and are gone too: this is more than an
  #include change in dialogstest.cpp (issue #32 works on its dump at the same time; the removed block, "Dialogs.obj's
  copy of the text table" with InitTexts and its call, is apart from the dump). Our Dialogs.cpp reads its own copy now, whose
  texts equal the image's; the dumps print the texts behind pointers (ReadString), not the pointers, so they
  compare the same values. Include fixes: contextmenustest/mainwindowtest ConsoleWindow.h, playertest
  Surface.h and sysinc.h (its `const int TitleCode[8]` definition needs the extern declaration, or it gets
  internal linkage and difftest.exe does not link), talktest graphics.h, creaturestest sysinc.h.
- **Tools**: tools/rename_externals.py retired (it renamed FUN_ names and deleted their lines from
  external.h; no FUN_ name is left). tools/triage.py and tools/simplify.py never read external.h.
  notes/agent-prompt.md and notes/reconstruction.md describe the rule that replaces external.h.
- **Not done here**: Status.cpp's LinkPattern pointer (0x55b758) has 17 copies in .data, one per object,
  so it is a header static too (issue #33).
## build.cmd reliability (issue #23)

Claude Opus 5.5 (issue #23), November 2, 2026.

- **Causes found** (high, reproduced). (1) The silent "exit 1" from Git Bash: main's harness with
  talktest.cpp's old double delete (734a5f7 reverted for the experiment) ends with `exit 1` and no output
  from Git Bash; the child died with 0xc0000374 and build.cmd's `|| exit /b 1` said nothing. The new
  harness prints `crash: exception 0xc0000374 (heap corruption) ... in TestTalk` and the parent's line.
  (2) Git Bash runs on main failed with output: 336 failures in dialogstest.cpp ("Dialogs ... (hotkeys):
  dialogs differ") while PowerShell passed. The dump took the original's string pointer 0x02abab30 for
  the 0xab fill pattern (two 0xab bytes) and printed it as a number, ours as the string. Git Bash's larger
  environment block moves the heap, so only some shells hit the 256-byte window 0x..abab... IsPattern now
  needs a 0xab top byte (no pointer of a 32-bit process is that high) or low byte (no object is at an odd
  address). A second flake of the same dump (1 of 10 cmd runs): a number both sides hold, 0x00df64af in a
  TRuleViolationDialog, was unreadable during our dump and readable during the original's (the heap grew
  in between), so one side printed "P". Compare now keeps each page's first readability answer for both
  dumps; every actual read checks the page again (PageReadable), because the cache alone let DumpWord read
  a page the heap had decommitted meanwhile (an access violation in 2 of about 60 runs of
  DIFFTEST_ONLY=Dialogs, found by running it under environment blocks of different sizes and pinned by the
  new crash report plus a /MAP build). (3) `cmd /c ...` from Git Bash: MSYS turns /c into C:/, so cmd starts interactively, prints
  its banner, reads the empty stdin and exits 0 without running anything; run `cmd //c`. (4) A locked
  difftest.exe: a child left running (its parent killed, or Windows Error Reporting holding a crashed
  child) keeps the image locked and the link fails. No other environment difference mattered: vcvarsall,
  cl.exe, link.exe (vcvars puts MSVC's before Git's /usr/bin/link.exe), python and TEMP/TMP (Windows paths
  from Git Bash too) resolve the same in all three shells.
- **Test names through shared memory, not stdout** (medium). The parent passes an inheritable file
  mapping handle as a third argument (`--child <Tibia.exe> <handle>`); the child writes the state and the
  test's name there. It survives the child, so even `__fastfail` (no handler in the process runs) gets
  the test named. A pipe or status file would work as well; this needs no reader thread and no file.
- **Function-like macros announce the tests** (medium). `#define TestX(...) RUN_TEST(TestX, __VA_ARGS__)`
  before main wraps each call in BeginTest/EndTest without changing the registration lines (a macro is
  not expanded again inside its own expansion, so the inner TestX calls the function). The cost: a new
  test needs a macro line; without one a crash in it is reported "after <previous test> (between the
  tests, or in a test that does not announce itself)". The alternative, a table or a RUN(...) on each
  line, would have rewritten every registration line that other branches (#11) add to.
- **Normal end through atexit** (medium). The child marks itself finished in an atexit handler, so the
  DIFFTEST_ONLY early returns need no change; a child that ends without atexit (crash, ExitProcess in
  image code, TerminateProcess) is reported even with exit code 0 (difftest.exe then returns 2).
- **The child ends crashes itself** (medium). Its unhandled-exception filter prints and terminates with
  the exception code (no Windows Error Reporting, no CRT terminate path); terminate() goes to abort(),
  whose SIGABRT handler exits with 3 (abort's own code); invalid parameter exits with 0xc0000417, a pure
  call with 0xc0000409; `__report_gsfailure` is patched into a jump to a reporter that prints the return
  address of the function whose /GS cookie was overwritten. SetThreadStackGuarantee(64 KB) leaves the
  vectored handler stack after a stack overflow. Child exit codes: 0 passed, 1 failures, 2 could not
  start, 3 abort, 4 heap check failed.
- **_heapchk() after every test, always** (high). It costs nothing measurable (full build.cmd runs took
  174-201 s before, 177-273 s after with other worktrees building at the same time; 30 final runs, 10
  each from cmd, PowerShell and Git Bash, all with 14,015,187 + 67 + 3,201 checks and 0 failures). It finds corruption of the heap's own structures (a large-block overrun is reported
  "in the heap check after TestX"); an overrun inside the low-fragmentation heap's small blocks can stay
  unnoticed until the heap trips over it.
- **Job object and timeout** (high). The child is in a job with KILL_ON_JOB_CLOSE (it dies with the
  parent) and DIE_ON_UNHANDLED_EXCEPTION (no error dialog). DIFFTEST_TIMEOUT defaults to 1800 s; a full
  difftest.exe run takes about 2.5 minutes, traced runs can set 0 (none).
- **build.cmd** (high). Each step's failure prints a message and exits 1; the run line of difftest.exe
  and its cl line keep their text (the parent prints the report; issue #11 adds files to those lines),
  only a `call :remove difftest.exe` before them is new. `:remove` deletes the old program before linking;
  if it is locked, PowerShell stops processes whose full path is this checkout's program (never other
  worktrees') and retries the delete for 10 s. ROOT is normalised (`%%~fI`), so TEMP is now
  ...\build\difftest\tmp without `tools\difftest\..\..`. vcvarsall's output goes to build\difftest\vcvars.txt
  and is shown when it fails. A passing run ends with "build.cmd: all tests passed"; the totals lines are
  unchanged. No incremental build: a full run takes about 3 minutes.
## dialogstest dump (issue #32)

dialogstest.cpp failed now and then with "dialogs differ" when the code had not changed. The failures
depended on the heap layout of the run (the environment, TEMP), not on the code.

- **Cause, confirmed**: two rules of the dump decided by addresses.
  - `Readable` asked VirtualQuery at dump time whether a word points to committed memory (`P`) or not (a
    number). The two dumps are built one after the other, and our dump's own strings (about 50 KB for the
    hotkey dialog) grow the heap between them. A non-pointer word in an address range the heap reaches during
    our dump was hex in ours and `P` in the original's: THotkeyDialog's AutoSend bools (words 0x01000100,
    0x01010001 ...) at member words 2323-2331. The reverse (`P` vs hex, the rule violation dialog of #27)
    happens when the heap gives memory back between the dumps.
  - `IsPattern` took every word with two 0xab bytes for the fill pattern. A real pointer such as 0x00abab28
    was hex on that side and the tree name on the other: TColorButton's Next and TOutfitDialog's buttons
    (`abab28` vs `F.4.49`), the help dialog's parent pointers (`abab38` vs `F.3`).
- **Reproduction**: DG_HEAPSHIFT=<bytes> allocates a block of that size first (and keeps it). On main, 5 of
  25 layouts failed (shift 40000, 100000, 250000 with build.cmd's TEMP: 1, 6, 2 failures, all hotkey AutoSend;
  shift 1500000 with the default TEMP: 1, hotkeys; shift 40000 with the default TEMP: 1,195, the outfit
  dialog's 0x00abab.. pointers in the 20 failures printed). The other 20 combinations of the shifts (none,
  1000, 30000, 40000, 100000, 250000, 600000, 1500000, 3000000, 8000000) and TEMPs (build.cmd's, the default,
  a long path) passed. The help dialog's case showed up in the mutation runs.
- **The new rules**: a word is a pointer only by where it points, decided from tables that do not change
  while the two dumps are built:
  - the side's tree components, receiver, parent window and statement (by name, as before);
  - a block allocated while a round runs (both sides' operator new and new[], the image's malloc, the fakes'
    memory): `H<size>+<offset>`, `D<size>+<offset>` when freed. The table is shared by both sides. Our
    operator delete and delete[] and the image's free (its operator delete calls it) mark a block as freed;
    freed blocks stay allocated until the round ends, so a dangling pointer cannot point into a newer block of
    one side only. Allocations of the test's own work (the names, the dumps) are not recorded, and frees of
    recorded blocks during the dumps wait until both dumps are built. The table's nodes come from a heap of
    their own (HeapCreate), not from the operator new it records;
  - the Tibia image, difftest.exe and the DLLs (MEM_IMAGE regions, taken once before the rounds): `S`;
  - the main thread's stack reservation: `K`;
  - anything else is a number (hex). 0xabababab is not an address, so the fill pattern needs no rule of its
    own; a word with some pattern bytes is a number or a pointer by the same rules on both sides.
  Strings (the string members, and texts in the logs) are read only from a pointer and only inside the block,
  image or stack it points into; timers are recognised by the vtable at the start of a live block.
- **Why not "allocation N"**: each side numbering the blocks its pointers reach (first reference in the
  dump) would compare pointer identity, but the same number in both dumps (random members such as
  TTextDialog's object number) can fall into a block that only one side's dump references, and would then
  get different numbers. Size and offset describe the block the same way for both sides. Pointer identity is
  still compared where it was before: pointers to tree components by name. Pointers to other blocks were `P`
  and now compare size and offset as well.
- **Not weakened (mutation check)**: 22 mutations of src\Dialogs.cpp from issue #8, 4 that change only where
  a pointer member points or the timer's lifetime (the outfit and VIP dialogs' radio selections, the login
  dialog's focus, SetTimer without the delete), and the outfit wrap. Each ran against main's dump and the new
  one (same seed, same 98,474 checks). The new dump catches every mutation the old one caught, with the
  same number of failures as the old dump's lowest count over three TEMP layouts. The old dump's count varies
  with the layout (the login focus mutation: 1155, 1159, 1174 and 1501 failures; the outfit wrap: 165, 235 and
  247), the new one does not (1155 and 165 each time). Only the old dump flakes in those runs. The pointer
  mutations are caught by the dump alone: 1690 failures for the outfit part selection, 592 for the VIP icon
  selection, 1155 for the login focus. SetTimer without the delete makes our side throw (GUI.cpp's timer list),
  under both dumps. Issue #8's `Outfit > LastOutfit` to `>=` gives 0 failures under the new dump. Under the old
  dump it gave 5 and 209 failures in two runs, all of them the IsPattern flake in the help dialog: with
  today's random sequence, no outfit reaches LastOutfit by event 6. `Outfit = FirstOutfit + 1` after the wrap
  is caught (165). git diff --stat src is empty afterwards.
- **Layouts**: the new dump passes all 18 layouts tried (no shift, 1000, 30000, 40000, 100000, 250000,
  600000, 1500000, 8000000; build.cmd's TEMP and the default TEMP), including the 5 that failed on main, with
  98,474 checks and 0 failures each. The check count of Dialogs is unchanged (the dump changed what it prints,
  not what it compares).
- **The other tests**: no other test decides pointers by readability or partial 0xab patterns.
  contentwindowstest.cpp, consolewindowtest.cpp, contextmenustest.cpp, dialogwindowtest.cpp,
  dialogelementstest.cpp, mainwindowtest.cpp, surfacedx9test.cpp and surfacegltest.cpp name objects by role or
  kind from their own maps (a pointer they do not know prints as "?", the same on both sides);
  mapwindowtest.cpp compares the windows' bytes at fixed offsets. The raw %p they print are values shared by
  both sides (the receivers of the element fakes, the talk buffer, the mark menu's window, a made-up number),
  and dialogelementstest.cpp's Tip tests only the whole word 0xabababab. They are unchanged.
- **Totals**: unchanged. tools\difftestuild.cmd: 14,492,680 checks in difftest.exe (Dialogs 98,889 in the
  full run, 98,474 alone), 67 in sysinctest.exe, 3,201 vlong, 0 failures. tools\compile-check.cmd is clean.
