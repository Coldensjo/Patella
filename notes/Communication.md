# Communication.cpp reconstruction handoff

## Coverage and source order

- All 141 `kind=game`, `file=Communication.cpp` rows are represented.
- These comprise 129 protocol routines, 11 compiler/STL entries (not reconstructed), and the false function boundary at `0x416e90`.
- `0x416e90` is an interior loop of `ReadFloor` at `0x416e60`, with no independent prologue, parameters, or EH frame. It is annotated in that function rather than fabricated as a callable routine.
- `SendLogout` at `0x404350` is additionally reconstructed: its body is assigned to the leading gap, but its catch blocks identify Communication.cpp line 178.
- Consequently there are 130 source-level definitions. The 11 compiler/STL entries (VC7.1 STL template instances and two compiler-generated TMyException members) are not part of the source; they are listed in a comment in Communication.cpp and as kind `stl` in names.tsv. (Review change: the first version reimplemented them with the host STL.)
- Assertion/catch line evidence interleaves the second binary run into the first.

## Validation

- All exception tables were inspected using `python -I tools/ehinfo.py`; SendLogout was checked separately.
- The seven functions with pre-try assertions are SendLogin, SendGameLogin, the three SendTalk overloads, SendOutfit, and SendAddBuddy. The other protocol assertions remain inside TBEGIN.
- REA instructions from the read-only main-checkout snapshot were used for ambiguous this pointers, dropped returns, struct-by-value arguments, and the ReadFloor boundary.
- All routines were compared again against the regenerated clean pseudocode, with raw output/instructions used where simplification dropped values or misidentified a throw. External/network call inventories were also compared function by function.
- `python -I tools/simplify.py`: regenerated all 62 clean files after appending names.
- `cmd /c tools\compile-check.cmd`: all src compiles with no warnings or errors (MFC shim).
- `tools\difftest\build.cmd`: 0 failures (see notes/reconstruction.md for the current totals).
- No Communication.cpp function is pure: every protocol routine uses Network, the game state, the GUI or the heap, so none is in the differential test. (The first version tested BuildFloatFormat at 0x412be0, which is STL code and was removed.)

## Preserved odd behavior

- Invalid single-step directions still send the existing command buffer without BeginCommand.
- A missing known creature consumes only one outfit-type byte and does not consume skull/shield bytes.
- The outfit-request flag is set before sending and is not rolled back on exceptions. Control.cpp resets it.
- Whole-file patch metadata is overwritten before checking for an existing transfer.
- Patch progress still divides by PatchDataSize, including the zero-size case.
- ReceiveField repeats the visible-floor lighting refresh already performed by ReadField.
- ReceiveMoveCreature can mutate creature state before producing diagnostics, and still moves the map object when the creature record is missing.
- Buddy online state is true only for byte value 1.

## Uncertainties and integration notes

- ReceiveCreatureMark is medium-confidence terminology for the timed square/highlight update at creature offset +0x7c. Other new names are inferred from behavior/opcodes, not recovered original spellings.
- Outgoing object functions use separate x/y/z arguments, confirmed by catch EBP offsets; Position is the stack position, not a coordinate struct.
- ReceiveOpenContainer and ReceiveCloseContainer take an int reference: catches load the referenced value while naming it Container.
- Temporary TCommunicationObject, TCommunicationCreature, TCommunicationStatement, and TCommunicationChannel layout views belong in their eventual real owner headers.
- The owner of the shared empty-object type at 0x719da0 is not established (Map/Inventory use it). The signature globals are owned by graphics.cpp (pic/spr) and Objects.cpp (dat).

- Existing external.h declarations were not reordered or changed; all additions are under the requested trailing heading.

## Complete external.h additions

Four temporary layout types: `TCommunicationObject`, `TCommunicationCreature`, `TCommunicationStatement`, `TCommunicationChannel`.

Six data declarations: `DAT_005cc758`, `DAT_005ccb64`, `DAT_00719d9c`, `DAT_00719da0`, `PTR_s_Your_request_has_been_closed__005c0348`, `PTR_s_The_channel_has_been_closed__You_005c034c`.

103 callable declarations (99 free functions and four members), grouped by binary owner below. The three creature members are FUN_0044b820/FUN_0044bc70/FUN_0044bf20; the channel member is FUN_00503920.

### Creatures.cpp (leading gap)

- `FUN_0044b820` (0x44b820), `FUN_0044bc70` (0x44bc70).

### Creatures.cpp

- `FUN_0044bf20` (0x44bf20), `FUN_0044c3c0` (0x44c3c0), `FUN_0044c530` (0x44c530).
- `FUN_0044c6e0` (0x44c6e0), `FUN_0044ca60` (0x44ca60), `FUN_0044cc00` (0x44cc00).
- `FUN_0044cf10` (0x44cf10), `FUN_0044eab0` (0x44eab0), `FUN_0044f000` (0x44f000).
- `FUN_004516e0` (0x4516e0), `FUN_004518b0` (0x4518b0), `FUN_00451db0` (0x451db0).
- `FUN_00452320` (0x452320).

### Talk.cpp; EAX is the advanced cursor

- `FUN_00503920` (0x503920).

### Control.cpp|Creatures.cpp

- `FUN_0044b870` (0x44b870), `FUN_0044b880` (0x44b880), `FUN_0044b8c0` (0x44b8c0).
- `FUN_0044b8e0` (0x44b8e0), `FUN_0044b900` (0x44b900), `FUN_0044b910` (0x44b910).
- `FUN_0044b920` (0x44b920), `FUN_0044b930` (0x44b930), `FUN_0044b950` (0x44b950).
- `FUN_0044b970` (0x44b970), `FUN_0044b990` (0x44b990), `FUN_0044b9a0` (0x44b9a0).
- `FUN_0044b9b0` (0x44b9b0), `FUN_0044b9c0` (0x44b9c0), `FUN_0044bb00` (0x44bb00).
- `FUN_0044bb20` (0x44bb20), `FUN_0044bb50` (0x44bb50), `FUN_0044bb60` (0x44bb60).

### Help.cpp

- `FUN_004a3690` (0x4a3690).

### Help.cpp|Inventory.cpp

- `FUN_004a4520` (0x4a4520).

### Inventory.cpp

- `FUN_004a4550` (0x4a4550), `FUN_004a46d0` (0x4a46d0), `FUN_004a4b00` (0x4a4b00).
- `FUN_004a4f70` (0x4a4f70), `FUN_004a5220` (0x4a5220), `FUN_004a5380` (0x4a5380).
- `FUN_004a5630` (0x4a5630), `FUN_004a58e0` (0x4a58e0), `FUN_004a6eb0` (0x4a6eb0).
- `FUN_004a7230` (0x4a7230), `FUN_004a7460` (0x4a7460), `FUN_004a75e0` (0x4a75e0).
- `FUN_004a7f00` (0x4a7f00).

### MainWindow.cpp|Map.cpp

- `FUN_004b08a0` (0x4b08a0), `FUN_004b08c0` (0x4b08c0), `FUN_004b08f0` (0x4b08f0).
- `FUN_004b0910` (0x4b0910), `FUN_004b0960` (0x4b0960), `FUN_004b09e0` (0x4b09e0).

### Map.cpp

- `FUN_004b6140` (0x4b6140), `FUN_004b6510` (0x4b6510), `FUN_004b6580` (0x4b6580).
- `FUN_004b88f0` (0x4b88f0), `FUN_004b8ec0` (0x4b8ec0), `FUN_004b9420` (0x4b9420).
- `FUN_004b9940` (0x4b9940), `FUN_004bab40` (0x4bab40), `FUN_004bad00` (0x4bad00).
- `FUN_004bb0c0` (0x4bb0c0), `FUN_004bc720` (0x4bc720), `FUN_004bccb0` (0x4bccb0).
- `FUN_004bd560` (0x4bd560), `FUN_004bdb70` (0x4bdb70), `FUN_004bdd70` (0x4bdd70).
- `FUN_004beb90` (0x4beb90), `FieldRefreshRequested` (0x4be750).

### Objects.cpp

- `FUN_004d1c40` (0x4d1c40), `FUN_004d1f30` (0x4d1f30).

### Player.cpp|Preferences.cpp

- `FUN_004db650` (0x4db650), `FUN_004db670` (0x4db670), `FUN_004db690` (0x4db690).
- `FUN_004db6a0` (0x4db6a0), `FUN_004db6b0` (0x4db6b0).

### Preferences.cpp

- `FUN_004dcac0` (0x4dcac0).

### Script.cpp

- `FUN_004e3100` (0x4e3100), `FUN_004e3500` (0x4e3500).

### Script.cpp|Status.cpp

- `FUN_004e6de0` (0x4e6de0), `FUN_004e6e20` (0x4e6e20), `FUN_004e7200` (0x4e7200).
- `FUN_004e7250` (0x4e7250), `FUN_004e7290` (0x4e7290), `FUN_004e72d0` (0x4e72d0).
- `FUN_004e76e0` (0x4e76e0).

### Status.cpp

- `FUN_004e8580` (0x4e8580), `FUN_004e91d0` (0x4e91d0), `FUN_004e96d0` (0x4e96d0).

### sysinc.cpp|Talk.cpp

- `FUN_004fd4c0` (0x4fd4c0), `FUN_004fd540` (0x4fd540), `FUN_004fd590` (0x4fd590).

### Talk.cpp

- `FUN_004ff500` (0x4ff500), `FUN_00500060` (0x500060), `FUN_005025c0` (0x5025c0).
- `FUN_005052e0` (0x5052e0), `FUN_00505ac0` (0x505ac0), `FUN_00505f10` (0x505f10).
- `FUN_005061f0` (0x5061f0), `FUN_00507bc0` (0x507bc0).
