# Tibia.exe 7.72 findings

Static analysis with REA + Ghidra 12.1.4. Addresses are virtual (image base `0x400000`);
the file offset is the address minus `0x400000` in `.text`, `.rdata` and `.data`.

"Observed" means read from the decompiled code. "Inferred" means a name taken from known
7.72 protocol documentation and not yet checked against the handler body.

## Identity

- "Tibia Client Version 7.72, Copyright (C) 2002-2006 CipSoft GmbH" (string at `0x55c650`).
- Source files named in assertions: `.\Communication.cpp`, `.\Control.cpp`.
- Graphics: OpenGL (`OPENGL32`, `GLU32`) and DirectDraw (`DDRAW`). Network: `WS2_32`, imported by ordinal.

## Login packet: `0x413780` (observed)

1. Generates a 16-byte random XTEA key with `rand()` and stores it with `0x4ce210` -> `0x508ca0`,
   which copies it into the cipher object at offsets +4 to +0x10.
2. Builds a 128-byte RSA block and encrypts it with the modulus at `0x55b620`
   (file offset `0x15b620`, 309 decimal digits, the standard CipSoft key).
3. Writes: protocol byte `1`, OS `2` (u16), version `0x304` = 772 (u16), then three u32 values
   from `0x719d9c`, `0x5ccb64` and `0x5cc758` (inferred: dat, spr, pic signatures), then the RSA block.

The RSA block is written through a local buffer as: byte `0`, the 16-byte XTEA key,
AccountID (u32), Password (string) (observed from the `this` pointers in the assembly).

## Packet framing (observed, see `src/Network.cpp`)

- Sending: `BeginCommand` reserves 4 bytes; `SendCommand(Encrypt)` fills them.
  Unencrypted: `[u16 length][payload]`. Encrypted: `[u16 total][XTEA([u16 length][payload][padding])]`,
  padded with `rand()` bytes to a multiple of 8.
- Receiving: `TSocket::OnReceive` buffers up to 16 KB and calls the receive handler when a complete
  packet is in; `PrepareNextPacket` decrypts it in place; `GetNextCommand` returns -1 when no packet is left.
- Login servers are tried in turn; a failure marks a server unavailable and moves to the next one.
  Connection timeout is 150 calls of `CheckConnectionTimeout`.

## Base layer (reconstructed in `src/`)

- `TMyException` (the only game class with RTTI) keeps a message,
  `"<file> <line>: <kind> (<name> = <value>), reason:
<detail>"`, and an origin `"<file> <line>"`.
  Wrapping keeps the innermost origin. The second exception type is `enum PROGRAM_ERROR`.
- RSA is George Barwood's public-domain `vlong` library (Montgomery exponentiation) with CipSoft's
  byte and decimal conversions. Public exponent 65537 (static `vlong` at `0x71f004`), 128-byte blocks,
  errors thrown as `const char*`.
- `Utils.cpp`: Latin-1 helpers (German umlauts count as letters), the 133-colour outfit palette
  (7 rows of 19 HSV colours, column 0 grey), and the packet buffers.
- The linker order is case-insensitive alphabetical by file name; this placed `TMyException`, `TRSA`
  and `TXTEA` (names inferred) between `Talk.cpp` and `Utils.cpp`, and `vlong.cpp` after `Utils.cpp`.

## XTEA (observed)

- `0x508cd0`: encrypt one 8-byte block, delta `0x9e3779b9`, 32 cycles (unrolled 4 per loop, 8 loops).
- `0x508e00`: the matching decrypt (same constant, found by byte search).

## Login servers: `0x4ca0e0` (observed)

Fills a table at `0x715368`: four entries of `0x70` bytes, hostname at +0, port (u16) at +0x64.

| Entry | Host                 | Port |
|-------|----------------------|------|
| 0     | `server.tibia.com`   | 7171 |
| 1     | `server2.tibia.com`  | 7171 |
| 2     | `tibia1.cipsoft.com` | 7171 |
| 3     | `tibia2.cipsoft.com` | 7171 |

The first server tried is `rand() & 3`. The two `cipsoft.com` names are written byte by byte as
immediates, so they do not appear in a string search.

## Server packet dispatcher: `0x44a900` (observed)

Reads one opcode at a time (`0x4cc550`, -1 = no more data) and keeps the last 10 opcodes in a ring
buffer at `0x5c4514`. Unknown opcodes throw "unknown packet type during login/game" (`Control.cpp`).
State from `0x4e71f0`: 4 = login server, otherwise game server.

Login server opcodes (handler, inferred meaning):

| Op   | Handler    | Meaning                         |
|------|------------|---------------------------------|
| 0x0A | `0x40a4c0` | error message                   |
| 0x0B | `0x40a670` | error message (new login)       |
| 0x14 | `0x40a810` | message of the day              |
| 0x1E | `0x40a9c0` | patch required (+ `0x44a770`)   |
| 0x1F | `0x40ae70` | patch required (+ `0x44a770`)   |
| 0x20 | `0x40b290` | patch required (+ `0x44a770`)   |
| 0x28 | `0x40bc80` | switch login server             |
| 0x64 | `0x40b8b0` | character list                  |

Game server opcodes (handler, inferred meaning):

| Op   | Handler    | Meaning               | Op   | Handler    | Meaning                |
|------|------------|-----------------------|------|------------|------------------------|
| 0x0A | inline     | self appear / enter   | 0x90 | `0x40fce0` | creature skull         |
| 0x0B | `0x40c000` | GM actions            | 0x91 | `0x40fec0` | creature shield        |
| 0x14 | `0x40c1b0` | disconnect message    | 0x96 | `0x410080` | text window            |
| 0x15 | `0x40c360` | error / wait          | 0x97 | `0x410260` | house text window      |
| 0x16 | `0x40c500` | waiting list          | 0xA0 | `0x410410` | player stats           |
| 0x1E | `0x40c6a0` | ping                  | 0xA1 | `0x410660` | player skills          |
| 0x64 | `0x415200` | full map description  | 0xA2 | `0x410810` | status icons           |
| 0x65 | `0x4153f0` | map slice north       | 0xA3 | `0x4109b0` | cancel target          |
| 0x66 | `0x4155c0` | map slice east        | 0xAA | `0x410b40` | creature speech        |
| 0x67 | `0x415790` | map slice south       | 0xAB | `0x411030` | channel list           |
| 0x68 | `0x415960` | map slice west        | 0xAC | `0x411230` | open channel           |
| 0x69 | `0x415b30` | update tile           | 0xAD | `0x4113f0` | open private channel   |
| 0x6A | `0x40cd10` | add tile thing        | 0xAE | `0x4115a0` | rule violation channel |
| 0x6B | `0x40cfa0` | transform tile thing  | 0xAF | `0x411760` | rule violation remove  |
| 0x6C | `0x40d440` | remove tile thing     | 0xB0 | `0x411940` | rule violation cancel  |
| 0x6D | `0x415d40` | move creature         | 0xB1 | `0x411b20` | rule violation lock    |
| 0x6E | `0x40d7d0` | open container        | 0xB2 | `0x411ce0` | create own channel     |
| 0x6F | `0x40da70` | close container       | 0xB3 | `0x411ea0` | close channel          |
| 0x70 | `0x40dc20` | container add item    | 0xB4 | `0x412060` | text message           |
| 0x71 | `0x40de40` | container update item | 0xB5 | `0x416c40` | cancel walk            |
| 0x72 | `0x40e060` | container remove item | 0xBE | `0x417020` | floor change up        |
| 0x78 | `0x40e230` | inventory set slot    | 0xBF | `0x417270` | floor change down      |
| 0x79 | `0x40e440` | inventory clear slot  | 0xC8 | `0x412350` | outfit window          |
| 0x7D | `0x40e5e0` | trade request         | 0xD2 | `0x412540` | VIP add                |
| 0x7E | `0x40e800` | trade counter offer   | 0xD3 | `0x412720` | VIP login              |
| 0x7F | `0x40ea30` | trade close           | 0xD4 | `0x4128c0` | VIP logout             |
| 0x82 | `0x40ebc0` | world light           |      |            |                        |
| 0x83 | `0x40ed60` | magic effect          |      |            |                        |
| 0x84 | `0x40ef50` | animated text         |      |            |                        |
| 0x85 | `0x40f150` | distance missile      |      |            |                        |
| 0x86 | `0x40f380` | creature square       |      |            |                        |
| 0x8C | `0x40f550` | creature health       |      |            |                        |
| 0x8D | `0x40f710` | creature light        |      |            |                        |
| 0x8E | `0x40f8e0` | creature outfit       |      |            |                        |
| 0x8F | `0x40fb20` | creature speed        |      |            |                        |

## Open questions

- Client-to-server packet builders (the `0x4cb2a0` / `0x4cb6e0` / `0x4cb880` writer family).
- Where the XTEA decrypt is applied to incoming data and how the length header is handled.
- Tibia.dat / Tibia.spr parsers (error strings at `0x5734ac` to `0x57352c`).
