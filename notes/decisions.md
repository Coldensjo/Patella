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
- **Names and source types** (low for original spelling; medium/high for
  inferred roles/layouts). TTalkBuffer, TTalkLine, TTalkFilter, the five
  derived class names, flag names and helper names not present in assertion
  strings are ours. names.tsv's medium labels describe confidence in the
  role, not recovery of those original spellings. TTextBuffer in external.h
  and TCommunicationChannel are pre-existing incomplete layout views; they
  cannot be rewritten under the append-only rule, so Talk.h defines the full
  compatible layout as TTalkBuffer and the existing calls receive explicit
  ABI bridges. Existing FUN names/calls are retained. The closed-channel
  string globals keep their existing declarations and binary names.
- **Factory's second parameter** (high for int from catches and callers;
  low for the old bool interface). 0x507bc0 takes ReceiveTalkMode and an
  integer SendTalkMode. CreateTextBuffer exposes that real signature. The
  existing FUN_00507bc0(int,bool,...) declaration remains intact and is a
  compatibility bridge: Communication.cpp's (6,true,NULL,ChannelID) still
  passes send mode 1 and creates the original rule-violation channel. Full
  integer modes, including equal receive/send modes, are differentially tested.
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
- **Append-only metadata correction** (high). An initial automatic names
  extraction mistook the explanatory 0x502f70 comment for a function and
  appended BuddyIgnoreList for that address. It is superseded by a later
  TTalkBuffer_Remove_line_block row (last row wins in simplify.py). The
  erroneous historical row is retained to honor the append-only constraint;
  the actual global is BuddyExemptionList, so it cannot pollute callcheck.

See notes/Talk.md for exception-table checks, switch tables, every remaining
callcheck difference, test coverage and the retained external.h declarations.
