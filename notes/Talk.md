# Talk.cpp reconstruction audit

Reconstructed by ChatGPT gpt-6.1-sol from the local Tibia.exe 7.72 image.
No original client process, loader or entry point was run.

## Scope

- 88 source entry points in Talk.cpp, including fourteen leading gap helpers.
- 21 Talk.h inline COMDAT entry addresses; folded true/false/null/-1 slots
  also remain inline rather than receiving duplicate definitions.
- One Container.h initialized vector<TTalkFilter*> constructor at 0x4fd6e0,
  using the existing template; operator() is folded with 0x452ac0.
- Two extra Ghidra game labels are internal basic blocks, 0x502f70/0x5036c0.
  Every catalogued game address from 0x4fd440 to 0x50826f has a names.tsv row.
- No sysinc implementation and no new STL/compiler-library implementation.
  The already reconstructed TMyException.cpp starts at 0x508270; its first
  entry is the existing names.tsv stl row std_string_find_last_of.

Ownership/evidence is recorded in file-ranges.tsv and notes/decisions.md.
The original 192 Talk catch handlers are represented by TBEGIN/TEND, not
separate functions. The three regenerated exception constructors are the
standard PROGRAM_ERROR (by value), TMyException& and catch-all trio.

## Binary checks

Every source entry's exception information was inspected with
python -I tools/ehinfo.py <address>. In particular:

| Entries | Enclosing try states / assertions |
|---|---|
| 0x4fd440-0x4fd590 | No exception frames; direct flags, counters and clock helper calls. |
| 0x4fd9e0, 0x4fdd80, 0x4fe960, 0x4ff810, 0x503920 | Assertions/errors only, no enclosing TBEGIN. |
| 0x4ff7d0 | No try block, despite compiler cleanup metadata. |
| 0x4ffb90, 0x502340, 0x503a70, 0x5041f0 | 5-5: first assertion is outside TBEGIN. |
| 0x503210 | 5-15: first!=NULL outside; Search assertions inside. The loop's i stays zero. |
| 0x5044b0 | 10-10: mode and text assertions outside TBEGIN. |
| 0x5048a0, 0x5052e0 | 15-15: three assertions outside TBEGIN. |
| 0x505ac0 | 10-10: two assertions outside TBEGIN. |
| 0x505f10 | 5-7: target assertion outside, line construction/append inside. |
| 0x5061f0 | 5-15: Text assertion outside, routing target assertions inside. |
| 0x506bf0 | 6-6: base construction and addressee assertion before TBEGIN. |
| 0x507070 | 1-1: base construction and mode/Closed setup before TBEGIN. |
| 0x506ec0, 0x507420 | Constructor cleanup state, no TBEGIN. |
| 0x506920, 0x507060 | ehinfo scans into the next entry; actual tail-call destructor/getter have no frames. |
| 0x502f70, 0x5036c0 | Not functions; frame/try belongs to 0x502f00 / 0x503670. |

All other TBEGIN functions start at state zero; extra states inside them
correspond to the assertions, allocations and local static initialization
shown in the source. All handler parameter names are retained, including
Write's anomalous bytes name and the loops' i/o/Counter variables.

### Switch tables

These are the only three source switches. Each was decoded from the image,
not trusted solely to Ghidra:

- **Color, 0x4fd9e0**: 26 selectors (0..25), byte map at 0x4fdd5c:
  0,1,1,1,2,1,3,1,1,4,4,4,5,4,4,4,6,6,4,3,3,3,7,3,8,4.
  Target table at 0x4fdd38:
  0x4fda1b, 0x4fdae3, 0x4fdb12, 0x4fdb41, 0x4fdb85,
  0x4fdb6c, 0x4fdbb4, 0x4fdbcd, 0x4fdbf9.
  Mode 0 throws, values >25 take the unknown-mode error. Cases 1..25 are
  all differentially tested.
- **Speech prefix, 0x5052e0**: selector Mode-1 in 0..10; byte map
  0x505aac: 0,1,2,3,4,4,4,4,3,4,3; target table 0x505a98:
  0x50565b (says), 0x505663 (whispers), 0x50566b (yells),
  0x505673 (name only), 0x50568c (no prefix).
  Animal modes 16/17 take the same no-prefix path. The assertion separately
  limits accepted inputs to 1/2/3/4/9/11/16/17.
- **Console routing, 0x5061f0**: selector TalkMode-18 in 0..5; table
  0x506704: 0x506328 (18, unconditional broadcast), 0x5063cd (19/20,
  event preference), 0x5063d7 (21, status preference), 0x50631b (22,
  info preference), 0x50648f (23, ignored). Other modes follow filters.

Vtables 0x573738, 0x573ad8, 0x573b48, 0x573bc8, 0x573c28,
0x573cc8 establish the base plus private, channel, rule-violation,
rule-violation-channel and private-channel layouts. There is no virtual
buffer destructor. The selected/private-channel predicate at 0x502880 calls
slots +0x3c and +0x38, not the locked getter at +0x48.

## Differential coverage

Run tools\difftest\build.cmd. The new talktest.cpp reuses PatchImage and
redirects only the additional CRT functions it needs. Original vector
constructors are explicitly invoked; no global executable initialization
is assumed. Both sides use one fixed 32-bit clock, restored in the test
executable after the suite.

Coverage includes all 88 source functions on valid inputs and every
executable header/template helper except the unconditional error function:

- all colors, changed latches, ignore flags, throttle boundaries, status
  message expiration/permanence and 4,000 wrapping/line-limit inputs;
- 6,000 filter matches, growing filter vectors, buffer/filter searches,
  insertion, serialization, removal and 2,500 randomized ignore-list updates;
- 6,000 talkbox transitions, duplicates, hidden queues, oldest removal,
  replacement promotion, clock expiration, geometry/color/text accessors;
- 1,500 named/unnamed statement constructions, timestamp selection, name
  truncation, text trimming, offsets and 32-bit time adjustment;
- 2,500 linked-buffer updates, disabled buffers, zero limits, cursor traversal,
  first/middle/last removals, integer-zero removal and real FILE log output;
- all six factory shapes, duplicate detection, inherited getters/setters,
  rule-violation replacement/removal behavior, write enabling, 500 routing
  sequences with preference changes, channel notices, default/direct posts,
  clearing, deletion, the legacy bool factory bridge and ExitTalk.

Base constructor untouched bytes are compared using prefilled storage.
Factory-created uninitialized Closed bytes are set identically before
subsequent comparisons; their unspecified initial value is not guessed.
The original's failure paths are excluded: its uninitialized exception
runtime is not invoked, and allocation failures are not injected. Thus
0x5075c0's unconditional error and the failing nonzero Remove(int) path are
verified from instructions/macros, not executed in the mapped image.

## Remaining callcheck differences

Final validation on November 1, 2026: Talk contributes **1,202,416 checks**;
the complete mapped-image suite reports **2,470,456 checks, 0 failures**.
The exact-arithmetic suite reports **3,201 checks, 0 failures**.
tools\compile-check.cmd compiles all reconstructed sources with **no
warnings or errors**, using the existing MFC shim because MFC is absent.
The generated symbols.tsv retains its required empty trailing TSV columns;
git diff --check is clean for all other changed files.

python -I tools/callcheck.py src/Talk.cpp reports **36 of 88** source entries.
These are lexical-count differences, not unexplained binary behavior.
The complete address list follows; parentheses give ours/decompiled counts.

| Entry | Explanation |
|---|---|
| 0x4fdd80 | GetNumberOfTalkboxes 2/3: callcheck counts its occurrence inside the assertion's expression string as a third call. Disassembly has exactly two call sites, 0x4fdda8 and 0x4fde85. |
| 0x4ff9c0 | vector<int>::operator() 0/1: C++ List(i), pointer specialization folded with 0x452ac0. |
| 0x4ffb90 | Same operator, 0/1. |
| 0x4ffe40 | Same operator, 0/2. |
| 0x500060 | Same operator, 0/3. |
| 0x5002d0 | Same operator, 0/2; repeated source indexing is optimized/commoned in the image. |
| 0x500520 | Same operator, 0/3. |
| 0x5006f0 | Same operator, 0/2. |
| 0x5008d0 | Same operator, 0/1. |
| 0x500a90 | Same operator, 0/3. |
| 0x500c60 | Same operator, 0/2. |
| 0x500e40 | Same operator, 0/2. |
| 0x501010 | Same operator, 0/2; repeated indexing as at 0x5002d0. |
| 0x501400 | Same operator, 0/3. |
| 0x5015d0 | Same operator, 0/2. |
| 0x501ae0 | Same operator, 0/1. |
| 0x501ca0 | Same operator, 0/2. |
| 0x5021a0 | Clear 1/0: source names the virtual call, pseudocode uses a vtable expression. |
| 0x502340 | Clear 1/0: same virtual-call spelling. |
| 0x5025c0 | vector operator 0/2, including the assertion. |
| 0x502880 | vector operator 0/1; GetSelected and IsPrivateChannel each 1/0 because the pseudocode uses indirect vtable calls. |
| 0x503a70 | GetText 1/0 versus GetTimeStamps 0/1: the image inlines Talk.h::GetText. |
| 0x503d20 | Base Append 0/1: callcheck resolves the explicitly qualified base call as the current class's override and then removes it as a self-call. |
| 0x503eb0 | Base Remove(line) 0/1: qualified base/overload resolution is not understood by callcheck. |
| 0x504050 | Base Remove(number) 0/1: same overload limitation. |
| 0x5041f0 | Base Append 0/1: same qualified-base limitation as 0x503d20. |
| 0x505f10 | Two TTalkLine constructors each 0/1: new TTalkLine(...) is not recognized as a Class_ctor symbol. |
| 0x5061f0 | vector operator 0/2. |
| 0x506720 | vector operator 0/1. |
| 0x506930 | Append 1/0 is a named virtual call; line constructor 0/1 is new syntax. |
| 0x507280 | Base AddText 0/1: same qualified-base/self-call limitation. |
| 0x5076e0 | vector operator 0/1. |
| 0x5079c0 | Base channel constructor 0/1: C++ base initializer versus explicit pseudocode call. |
| 0x507a00 | Clear 0/1: destructor call versus the image's inlined destructor. |
| 0x507bc0 | Six constructors each 0/1: new/base-initializer syntax instead of explicit named calls. The compatibility bridge's self-address is intentionally discarded by callcheck. |
| 0x507fa0 | vector operator 0/2; Clear 0/1 is destructor/inlining as at 0x507a00. |

## Existing external.h declarations now defined

The append-only header is deliberately unchanged. Its retained declarations
are fulfilled by Talk.cpp (and the compatible forwarding member):

- FUN_00501400; FUN_004fd4c0; FUN_004fd540; FUN_004fd590.
- FUN_004ff500; FUN_00500060; FUN_005025c0; FUN_005052e0;
  FUN_00505ac0; FUN_00505f10; FUN_005061f0; FUN_00507bc0.
- FUN_004ff130; FUN_004ff7d0; FUN_00500c60; FUN_00500e40;
  FUN_00504d70; FUN_00504f30; FUN_005076e0; FUN_00507a00.
- TCommunicationChannel::FUN_00503920.
- PTR_s_Your_request_has_been_closed__005c0348 and
  PTR_s_The_channel_has_been_closed__You_005c034c.

No new dependency on an unfinished file required an external declaration:
GetMilliseconds was already declared there. Future cleanup may consolidate
TTextBuffer/TCommunicationChannel with Talk.h, but no caller is renamed in
this reconstruction.

## Open questions

Only original spellings and source-level type syntax remain uncertain:
class/helper/flag names are inferred, colors may originally have returned
TRGBColor by value, and the old external.h factory has a narrower bool view.
The explicit ABI bridge and tested layouts settle runtime behavior on valid
inputs. Original exception/allocation-failure execution remains outside the
mapped-image harness's coverage.
