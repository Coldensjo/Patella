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
