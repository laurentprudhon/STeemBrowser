# Steem SSE - File-by-File, Feature-by-Feature Test Plan

## Index

| Phase | Document Section | Files Covered |
|-------|-----------------|---------------|
| Phase 0 | [Infrastructure Setup](#phase-0-infrastructure-setup) | Framework files |
| Phase 1 | [CPU Emulation](#phase-1-cpu-emulation) | cpu.cpp, cpu_ea.cpp, cpu_op.cpp, cpuinit.cpp |
| Phase 2 | [Memory System](#phase-2-memory-system) | mmu.cpp, ior.cpp, iow.cpp |
| Phase 3 | [Video Pipeline](#phase-3-video-pipeline) | glue.cpp, shifter.cpp, dma.cpp, blitter.cpp, palette.cpp |
| Phase 4 | [Sound System](#phase-4-sound-system) | psg.cpp, sound.cpp, midi.cpp |
| Phase 5 | [Disk Emulation](#phase-5-disk-emulation) | fdc.cpp, floppy_drive.cpp, floppy_disk.cpp, disk_stw.cpp, disk_scp.cpp, disk_hfe.cpp, disk_ghost.cpp, archive.cpp, acsi.cpp, hd_gemdos.cpp, harddiskman.cpp |
| Phase 6 | [Input Systems](#phase-6-input-systems) | ikbd.cpp, key_table.cpp, acia.cpp, rs232.cpp, stjoy.cpp |
| Phase 7 | [Emulation Core](#phase-7-emulation-core) | emulator.cpp, run.cpp, reset.cpp, computer.cpp, tos.cpp, loadsave.cpp, loadsave_emu.cpp |
| Phase 8 | [Utilities](#phase-8-utilities) | acc.cpp, macros.cpp, historylist.cpp, dataloadsave.cpp |
| Phase 9 | [Integration Tests](#phase-9-integration-tests) | Cross-component scenarios |

---

## Phase 0: Infrastructure Setup

### Files to Create

| # | File | Purpose |
|--|------|---------|
| 0.1 | `tests/CMakeLists.txt` | Root CMake: GTest fetch, project config, subdirectory includes |
| 0.2 | `tests/cmake/test_flags.cmake` | Mirror of production flags: `-DUNIX -DLINUX -DSSE_DRAW_C` |
| 0.3 | `tests/framework/test_helpers.h` | Macro definitions: `EXPECT_REGISTER`, `ASSERT_CYCLES`, etc. |
| 0.4 | `tests/framework/test_helpers.cpp` | Implementation of helper functions |
| 0.5 | `tests/framework/emulator_mock.h` | Mock declarations for global state: `COUNTER_VAR`, `SSEConfig`, chip externals |
| 0.6 | `tests/framework/emulator_mock.cpp` | No-op implementations of platform calls |
| 0.7 | `tests/framework/memory_mock.h` | `MockMemory` class: read/write with logging, expected access verification |
| 0.8 | `tests/framework/agenda_mock.h` | `MockAgenda` class: event scheduling with cycle timestamps |
| 0.9 | `tests/tier1_unit/CMakeLists.txt` | Tier 1 test executables |
| 0.10 | `tests/tier2_component/CMakeLists.txt` | Tier 2 test executables |
| 0.11 | `tests/tier3_integration/CMakeLists.txt` | Tier 3 test executables |
| 0.12 | `tests/resources/disk_images/` | Empty; populated with generated test data |
| 0.13 | `tests/resources/roms/` | Empty; will hold minimal mock TOS |

### Build Validation Test

**File**: `tests/tier1_unit/utility/test_infrastructure_smoke.cpp`
```cpp
// Single test to verify:
// 1. CMake finds GTest
// 2. Compiler flags work
// 3. Steem source compiles with TEST_BUILD
// 4. Mock layer loads
```

---

## Phase 1: CPU Emulation

> **Rationale**: MC68000 is the heart of the emulator. 425+ tests across 12 test files. Tests are pure unit: given register state + instruction bytes, verify result register state.

### 1.1 cpuinit.cpp (~200 lines)

**What it does**: Initializes MC68000 lookup tables and CPU state.

| Test | Input | Expected Output |
|--|--|--|
| Init creates correct opcode table | Run `CpuInit()` | All 256 opcodes mapped, no nullptr in fetch table |
| Register reset to known state | Run `TMC68000::Reset()` | All D0-D7=0, A0-A7=0, SR matches TOS vector |
| Stack pointer valid | After reset | A7 = 0xFFFFFFxx (within RAM) |
| Precomputed tables built | Run `CpuInit2()` | All table pointers non-null |

**Test file**: `tests/tier1_unit/cpu/test_cpuinit.cpp`
**Estimated tests**: 15
**Priority**: P0

### 1.2 cpu_ea.cpp (1,916 lines)

**What it does**: Effective address (EA) calculation for all 18 MC68000 addressing modes.

| Test Group | Count | Description |
|--|--|--|
| Data Register Direct (Dn) | 8 | D0-D7 each resolve to current register value |
| Address Register Direct (An) | 8 | A0-A7 each resolve to register as 32-bit pointer |
| Address Register Indirect (An) | 8 | Dereference An, verify pointer arithmetic |
| Address Register Indirect + Post-inc (An)+ | 8 | Return An, then increment by 2/4 |
| Address Register Indirect + Pre-dec -(An) | 8 | Decrement An by 2/4, then dereference |
| Address Register Indirect + disp5/disp8/disp16 | 12 | Displacement offsets added to An |
| Address Register + Index (An,d16,Xn) | 16 | Index register + displacement offset |
| Address Register + Index + PC | 8 | PC-relative indexed addressing |
| Absolute short/long | 8 | 16-bit and 32-bit absolute addresses |
| PC-relative | 8 | PC-relative addressing with 8/16-bit offset |
| PC-relative indexed | 8 | PC + index register + displacement |
| Immediate addressing | 8 | Value embedded in instruction |
| Move indirect addressing | 4 | (An,PC) 32-bit indirect |
| Auto-index exception handling | 4 | Stack auto-index on interrupt/trap |
| Register combination validity | 8 | Invalid register combinations produce bus error |
| **Subtotal** | **~120** | |

**Test file**: `tests/tier1_unit/cpu/test_cpu_ea.cpp`
**Priority**: P0

### 1.3 cpu_op.cpp (8,755 lines)

**What it does**: Implements all MC68000 instruction opcodes. ~500+ methods.

This is the largest single file. Breaking into logical groups:

#### Data Movement (30 tests)

| Test | Instruction | Scenario |
|--|--|--|
| MOVE.B src, dest | 8 | All src/dest EA combos for 8-bit |
| MOVE.W src, dest | 8 | All src/dest EA combos for 16-bit |
| MOVE.L src, dest | 8 | All src/dest EA combos for 32-bit |
| MOVEQ immediate,dn | 4 | Sign-extended 8-bit to 32-bit reg |
| MOVEM long word list | 2 | Multiple register save/restore |
| LEA effective_addr,An | 2 | Effective address calculation (no memory access) |

#### ALU Operations (80 tests)

| Test | Instructions | Scenarios |
|--|--|--|
| ADD / ADDA / ADDX | 12 | Various sizes, carry propagation, flag updates |
| SUB / SUBA / SUBX | 12 | Same coverage as ADD |
| AND | 8 | Source/dest EA combos, zero flag, negative flag |
| OR | 8 | Same coverage as AND |
| EOR | 8 | Exclusive OR, flag updates |
| CMP | 8 | Compare without store, flag state validation |
| ASR / ASL / LSR / LSL | 8 | Shift operations, X flag = last bit shifted |
| ROXL / ROXR | 4 | Rotate through X flag |
| ROTL / ROTR | 4 | Rotate without X flag |
| NOP | 2 | Cycles consumed, flags unchanged |
| SWAP | 2 | Exchange high/low word of D register |

#### Bit Operations (30 tests)

| Test | Instructions | Scenarios |
|--|--|--|
| BTST data,Dn | 4 | Test bit in data register |
| BTST data,mem | 4 | Test bit in memory location |
| BCHG data,Dn / mem | 4 | Clear and complement bit |
| BIT data,Dn / mem | 4 | AND bit with flags |
| BSET data,Dn / mem | 4 | Set bit in operand |
| BCLR data,Dn / mem | 4 | Clear bit with flag update |
| BXXX extended data EA | 4 | Extended addressing for bit ops |
| BXXX byte addressing | 2 | Bit offset in byte-sized operand |

#### Control Flow (35 tests)

| Test | Instructions | Scenarios |
|--|--|--|
| BRA offset | 4 | Short/long branch, near and far |
| BSR offset | 4 | Branch to subroutine, return address pushed |
| Bcc (conditional) | 12 | All conditions: EQ, NE, HI, LS, CC, HS, CS, LO, GE, LT, GT, LE |
| JMP absolute/indirect | 4 | Jump to address |
| JSR absolute/indirect | 4 | Jump to subroutine |
| RTS | 4 | Return from subroutine, PC popped |
| RTE | 4 | Return from exception, SR and PC popped |
| STOP | 2 | Set SR, enter stop state |
| TRAP | 2 | Trigger exception vector 16-31 |
| TRAPV | 2 | Trap on overflow flag |
| TAS | 2 | Test and set memory location |
| RTR | 2 | Return from trap, SR popped |

#### Multiply/Divide/Shift (40 tests)

| Test | Instructions | Scenarios |
|--|--|--|
| MULU 16-bit | 4 | Unsigned multiply, 32-bit result |
| MULS 16-bit | 4 | Signed multiply, 32-bit result |
| DIVU by zero | 4 | Division by zero exception, vector 5 |
| DIVU 32-bit | 4 | Unsigned division |
| DIVS by zero | 4 | Signed division by zero |
| DIVS 32-bit | 4 | Signed division, overflow check |
| CHK | 4 | Range check, trap if out of range |
| CHK2 | 2 | Second range check path |
| NEG / NEGX / NOT | 4 | Negate with/without extend, bitwise NOT |
| EXT / EXTX | 2 | Sign-extend word/byte to longword |
| ABCD / SBCD | 4 | Packed BCD add/subtract with carry |

#### State and Flags (30 tests)

| Test | Instructions | Scenarios |
|--|--|--|
| TST | 4 | Set flags from zero operand |
| MOVE to/from SR | 4 | Status register access, privileged mode |
| MOVE to/from CCR | 4 | Condition codes only, no privilege check |
| CHS | 2 | Change sign of data register |
| MOVE from user SR | 2 | User-mode access restriction |
| Illegal instruction | 2 | Vector 4 exception |
| Privilege violation | 2 | Vector 8 exception |

**Test files**:
- `tests/tier1_unit/cpu/test_cpu_data_move.cpp`
- `tests/tier1_unit/cpu/test_cpu_alu.cpp`
- `tests/tier1_unit/cpu/test_cpu_bitops.cpp`
- `tests/tier1_unit/cpu/test_cpu_control.cpp`
- `tests/tier1_unit/cpu/test_cpu_multiply.cpp`
- `tests/tier1_unit/cpu/test_cpu_ccr.cpp`
- `tests/tier1_unit/cpu/test_cpu_shift.cpp`
**Priority**: P0

### 1.4 cpu.cpp (1,970 lines)

**What it does**: CPU execution loop, cycle management, exception handling.

| Test | Method | Scenario |
|--|--|--|
| RunCycles basic | `RunCycles(n)` | Consumes exactly n cycles |
| RunCycles instruction boundary | `RunCycles(n)` | Stops at instruction boundary, not mid-instruction |
| Interrupt handling | `InterruptRequest()` | IRQ delivered at correct cycle count |
| Reset exception | `Reset()` | Fetches vector 0, sets SR to disable interrupts |
| Bus error handling | Force bus error | Vector 2 fetched, fault address pushed |
| Address error | Force unaligned access | Vector 3 fetched, error code pushed |
| Illegal instruction | Execute opcode 0x4840 | Vector 4 fetched |
| Divide by zero | DIVU/DIVS by 0 | Vector 5 fetched, quotient undefined |
| CHK exception | OUT_OF_RANGE_CHK() | Vector 6 fetched, instruction pointed |
| Trap instruction | TRAP #n | Vector 16+n fetched |
| Stack overflow | Push beyond stack | Bus error on push |
| Stop mode | STOP #$2700 | CPU enters stopped state |

**Test file**: `tests/tier1_unit/cpu/test_cpu_core.cpp`
**Estimated tests**: ~40
**Priority**: P0

### Phase 1 Summary

| File | Tests | Test Files |
|--|--|--|
| cpuinit.cpp | 15 | 1 |
| cpu_ea.cpp | 120 | 1 |
| cpu_op.cpp | ~300+ | 7 |
| cpu.cpp | ~40 | 1 |
| **Phase 1 Total** | **~475** | **10** |

---

## Phase 2: Memory System

### 2.1 mmu.cpp (~1,500 lines)

**What it does**: Memory Management Unit - bank switching, memory routing, wait states.

| Test | Scenario | Expected |
|--|--|--|
| ROM region read (0x000000-0x00FFFF) | Read address 0 | ROM data returned, wait states correct |
| RAM region read (0-1MB) | Read address 0x100000 | RAM data returned |
| RAM bank switching | Set bank config, read 0x80000+ | Different banks visible |
| TOS bank select | Bank 0 (ROM 1) vs Bank 1 (ROM 2) | Correct ROM mapped at 0 |
| Memory config 128KB | Set config, read beyond 128KB | Mirrored RAM or bus error |
| Memory config 512KB | Set config, read bank boundaries | Correct mirroring |
| Memory config 1MB | Set config, read full range | Full 1MB contiguous |
| Memory config 4MB | Set config, read extended RAM | Banks 16-31 mapped |
| Memory config 14MB | Set config, Monster alt RAM | Extended bank mapping |
| I/O memory-mapped reads (0xFF8000+) | Read glue register address | Delegates to glue read handler |
| Cart ROM detection | Check 0xFA0000-0xFFFFFE | Cart data mapped if present |

**Test file**: `tests/tier1_unit/mmu/test_mmu_basic.cpp`
**Estimated tests**: 30
**Priority**: P1

### 2.2 ior.cpp (1,114 lines)

**What it does**: Memory-mapped I/O read handlers for all chip registers.

| Test | I/O Address Range | Chip | Expected |
|--|--|--|--|
| Read 0xFF8000-0xFF9FFF | DMA chip registers | `IORDma()` | DMA register value |
| Read 0xFFA000-0xFFBFFF | MFP registers | `IORMFP()` | MFP register value |
| Read 0xFFC000-0xFFDFFF | Acia 0 (RS232) | `IORAcia0()` | ACIA data/status register |
| Read 0xFFE000-0xFFFF7E | Acia 1 (IKBD) | `IORAcia1()` | IKBD data/command register |
| Read cartridge space (0xFA0000-0xFF7E) | Cartridge | `IORCart()` | Cartridge ROM data |
| Read PSG ports (0xFFC800-0xFFCFFE) | PSG | `IORPsg()` | PSG port A/B/control |
| Read glue registers (0xFF8800-0xFF8FFF) | Glue | `IORGlue()` | VDI, SCUMODE, etc. |
| Read blitter registers | Blitter | `IORBlitter()` | Blitter register values |
| Read FDC registers | FDC | `IORDrive()` | FDC status/data register |
| Read shifter control | Shifter | `IORShifter()` | Shifter register state |

**Test file**: `tests/tier2_component/mmu/test_ior.cpp`
**Estimated tests**: 40
**Priority**: P1

### 2.3 iow.cpp (1,846 lines)

**What it does**: Memory-mapped I/O write handlers for all chip registers.

| Test | I/O Address Range | Chip | Expected |
|--|--|--|--|
| Write DMA register | `IOWDma()` | DMA state updated, correct register targeted |
| Write MFP register | `IOWMFP()` | MFP register state updated, timer reloaded if applicable |
| Write Acia 0 | `IOWAcia0()` | RS232 data/command register updated |
| Write Acia 1 | `IOWAcia1()` | IKBD command sent, keyboard buffer modified |
| Write PSG port | `IOWPsg()` | PSG register written, audio parameters changed |
| Write glue register | `IOWGlue()` | VDI mode changed, SCUMODE updated, etc. |
| Write blitter register | `IOWBlitter()` | Blitter parameter updated, triggered if start register |
| Write FDC register | `IOWDrive()` | FDC command/data register updated |
| Write shifter register | `IOWShifter()` | Color register changed, VST-registers updated |

**Test file**: `tests/tier2_component/mmu/test_iow.cpp`
**Estimated tests**: 45
**Priority**: P1

### Phase 2 Summary

| File | Tests | Test Files |
|--|--|--|
| mmu.cpp | 30 | 1 |
| ior.cpp | 40 | 1 |
| iow.cpp | 45 | 1 |
| **Phase 2 Total** | **115** | **3** |

---

## Phase 3: Video Pipeline

### 3.1 glue.cpp (2,217 lines)

**What it does**: Glue chip - bus arbitration, timing, interrupt generation, VBL/HBL.

| Test | Method | Scenario |
|--|--|--|
| VBL generation @ 50Hz | `DoHBL()` | VBL after 313 scanlines (GLU_PAL_SCANLINES) |
| VBL generation @ 60Hz | `DoHBL()` | VBL after 263 scanlines (GLU_NTSC_SCANLINES) |
| VBL generation @ 72Hz | `DoHBL()` | VBL after 501 scanlines (GLU_MONO_SCANLINES) |
| HBL timing @ 50Hz | `DoHBL()` | Cycle cost = 512*TICKS8 |
| HBL timing @ 60Hz | `DoHBL()` | Cycle cost = 508*TICKS8 |
| HBL timing @ 72Hz | `DoHBL()` | Cycle cost = 224*TICKS8 |
| DE (Display Enable) timing PAL | Glue::DE_on() | DE goes high after GLU_DE_ON_PAL=64 scanlines |
| DE timing NTSC | Glue::DE_on() | DE goes high after GLU_DE_ON_NTSC=60 scanlines |
| DE timing Mono | Glue::DE_on() | DE goes high after GLU_DE_ON_MONO=14 scanlines |
| HSYNC low duration | Glue::HSyncOff() | Low for GLU_HSYNC_DURATION_LO=40 CPU cycles |
| HSYNC high duration | Glue::HSyncOn() | High for GLU_HSYNC_DURATION_HI=24 CPU cycles |
| Bus arbitration CPU vs DMA | `Wait()` with DMA active | Correct wait states per bus master |
| Bus arbitration CPU vs Blitter | Wait with Blitter active | Blitter delay computation correct |
| STVL callback invocation | Video logic plugin active | Callback fired at correct cycle |
| Scanline state machine transitions | Full VBL cycle | State transitions: top-margins -> display -> bottom-margins |
| Interrupt request to MFP | After VBL/HBL threshold | MFP IRQ triggered via glue->MFP link |

**Test file**: `tests/tier2_component/glue/test_glue.cpp`
**Estimated tests**: 35
**Priority**: P1

### 3.2 shifter.cpp (~2,000 lines)

**What it does**: Color conversion, video memory read, palette management, STE enhanced features.

| Test | Feature | Scenario |
|--|--|--|
| ST Palette load | 16-color ST mode | 16 colors in palette table |
| STE Palette load | STE enhanced colors | 256 colors, STE palette registers |
| Resolution mode Low | 320x200 | 4bpp, low-res pixel extraction |
| Resolution mode Medium | 640x200 | 2bpp, med-res pixel extraction |
| Resolution mode High | 640x400 | 1bpp, high-res pixel extraction + color |
| STE GDR mode | 512x256 | STE-specific resolution mode |
| Color register writes | All 16 ST colors | Each register stores correct RGB value |
| STE color registers | All 32 STE pairs | Each pair correctly parsed and stored |
| Video RAM read patterns | Scanline buffer fill | Correct pixel data extracted from VRAM |
| Palette color interpolation | STE VDI palette | Intermediate color computation |

**Test file**: `tests/tier2_component/shifter/test_shifter.cpp`
**Estimated tests**: 30
**Priority**: P1

### 3.3 dma.cpp (~900 lines)

**What it does**: Direct Memory Access controller - sound DMA, video DMA, channel priority.

| Test | Feature | Scenario |
|--|--|--|
| Sound DMA channel | Enable sound DMA | DMA reads from sound buffer address |
| Sound DMA burst | Full burst request | Correct bytes per burst |
| Channel priority | All 4 channels active | Priority order: Sound > VSync > HSync |
| Wait state computation | `Wait()` with active DMA | Correct cycle penalty |
| Bus access coordination | `BusAccess()` for each channel | Handshake protocol with glue |
| Sound DMA word transfers | 16-bit words | Each word delivered to PSG sound buffer |
| HSync channel | HBL DMA | VDI memory update for scanline |
| VSync channel | VBL DMA | Full screen memory update for scanline |
| DMA register writes | All registers | Register values correctly updated |
| DMA transfer completion | End of transfer | Callback invoked |

**Test file**: `tests/tier2_component/dma/test_dma.cpp`
**Estimated tests**: 30
**Priority**: P1

### 3.4 blitter.cpp (~2,000 lines)

**What it does**: STE blitter chip - bit-block transfers, bitmap operations.

| Test | Feature | Scenario |
|--|--|--|
| Destination address init | BLTDADDR write | Address loaded, auto-increment configured |
| Source address init | BLTSDADDR write | Source address loaded |
| Word count configuration | BLTWDLEN0/1 write | Width+length parsed from 16-bit reg |
| Vertical line length | BLTVLLEN write | Line count loaded |
| Horizontal line length | BLTHTLEN write | Horizontal length loaded |
| Pattern register | BLTCPATTERN write | Pattern data stored |
| Mode register | BLTMODE write | Mode flags set: source, mask, complement, etc. |
| Wait register | BLTWAIT write | Wait cycles value loaded |
| Start trigger | BLTCON0 write with start set | Blitter begins operation |
| Bit-block copy | Full copy operation | Source pixels copied to dest |
| Bit-block AND/OR/XOR | With pattern | Bitwise operation applied per pixel |
| Bus access with CPU | CPU+Blitter wait | Wait states during shared bus |
| Auto-increment dest | Vertical scan | Next scanline offset computed |
| Latch latency | Start trigger to first access | BLITTER_LATCH_LATENCY cycles observed |

**Test file**: `tests/tier2_component/blitter/test_blitter.cpp`
**Estimated tests**: 30
**Priority**: P1

### 3.5 palette.cpp (~300 lines)

| Test | Feature | Scenario |
|--|--|--|
| Generate ST palette | 16-color | All 16 entries filled with expected RGB |
| Generate STE palette | 256-color | All entries from STE hardware values |
| NTSC vs PAL palette | Color temperature diff | NTSC palette warmer, PAL cooler |
| Palette gamma correction | Non-linear gamma | Gamma-corrected values within tolerance |

**Test file**: `tests/tier1_unit/utility/test_palette.cpp`
**Estimated tests**: 10
**Priority**: P2

### Phase 3 Summary

| File | Tests | Test Files |
|--|--|--|
| glue.cpp | 35 | 1 |
| shifter.cpp | 30 | 1 |
| dma.cpp | 30 | 1 |
| blitter.cpp | 30 | 1 |
| palette.cpp | 10 | 1 |
| **Phase 3 Total** | **135** | **5** |

---

## Phase 4: Sound System

### 4.1 psg.cpp (1,122 lines)

**What it does**: Yamaha YM2149 PSG - 3-channel sound generator, noise, envelope, I/O.

| Test | Feature | Scenario |
|--|--|--|
| Channel A frequency | Write port 0,1 | Period set, output frequency correct |
| Channel B frequency | Write port 2,3 | Period set |
| Channel C frequency | Write port 4,5 | Period set |
| Noise frequency | Write port 6 | Noise period set |
| Noise shape selection | Write port 7 (bits 4-5) | All 6 noise shapes produce distinct patterns |
| Volume control A-C | Write port 7 (bits 0-3) | Volume 0-15, mute at 0 |
| Envelope enable | Write port 7 (bit 6) | Envelope replaces volume for channel |
| Envelope frequency | Write port 8,9 | Envelope period set |
| Envelope shape | Write port 7 (bits 8-11) | All 16 envelope shapes |
| I/O port A read | Port 12 | Keyboard/mouse data returned |
| I/O port B read | Port 13 | Joystick data returned |
| I/O direction A,B | Port 14 | Direction bit controls read/write mode |
| Mixed output | All 3 channels + noise | Combined into single sample stream |
| Sample buffer write | Fill buffer | Samples appended to buffer |
| Fixed vol table load | `LoadFixedVolTable()` | Volume lookup table populated |
| Microwire mode | STE microwire access | Correct data on microwire |
| DSP filtering | Filter active | Low-pass / high-shelf filters applied |
| ADSR envelope timing | Envelope attack | Attack time matches register value |

**Test file**: `tests/tier2_component/psg/test_psg.cpp`
**Estimated tests**: 40
**Priority**: P1

### 4.2 sound.cpp (2,063 lines)

| Test | Feature | Scenario |
|--|--|--|
| Sound buffer init | Buffer setup | Correct buffer size, sample rate |
| Sound quality setting | LQ/HQ toggle | Different filter parameters applied |
| Sound pause/resume | Toggle pause | Buffer stops/starts filling |
| Sound mute | Toggle mute | Output silenced, buffer not filled |
| Drive sound toggle | Drive sound on/off | Drive click sounds mixed in |
| Sound buffer underrun | Buffer empty | Graceful handling, no crash |
| Batch sound generation | Multiple VBLs | Batched generation correct amount |

**Test file**: `tests/tier1_unit/sound/test_sound.cpp`
**Estimated tests**: 20
**Priority**: P2

### 4.3 midi.cpp (1,199 lines)

| Test | Feature | Scenario |
|--|--|--|
| MIDI note on parse | Valid MIDI message | Correct note/channel extracted |
| MIDI note off parse | Note off | Note release triggered |
| MIDI program change | Change patch | Instrument index updated |
| MIDI volume control | CC 7 | Volume parameter set |
| MIDI channel mapping | Channel 1 | Maps to PSG channel correctly |
| MIDI clock timing | Clock pulse | Timing between pulses correct |
| Sysex buffer handling | System exclusive | Sysex data buffered correctly |
| MIDI output buffer | Output MIDI event | Data written to serial output |

**Test file**: `tests/tier1_unit/sound/test_midi.cpp`
**Estimated tests**: 15
**Priority**: P2

### Phase 4 Summary

| File | Tests | Test Files |
|--|--|--|
| psg.cpp | 40 | 1 |
| sound.cpp | 20 | 1 |
| midi.cpp | 15 | 1 |
| **Phase 4 Total** | **75** | **3** |

---

## Phase 5: Disk Emulation

### 5.1 fdc.cpp (4,097 lines)

**Largest single source file.** The WD1772 Floppy Disk Controller.

| Test | Feature | Count |
|--|--|--|
| Command parsing | All 22 commands | Read data, read address, write data, write delete, format track, etc. |
| Status register | Each command | Correct status after completion: CRC, ECC, end-of-data, index, etc. |
| Phase register | Transition between phases | Phase changes: ready -> command -> active -> terminating |
| Sector read | ST format (9 sectors) | 128-byte data returned, CRC check |
| Sector read 10 sectors | ST format variant | 128-byte data returned |
| Sector read 11 sectors | Extended sector count | 128-byte data returned, interleave correct |
| Sector write | Full write | Data written to disk image, CRC computed |
| Track format | With ID + data | ID field and data fields written |
| MFM encoding verification | `MFMSectorCheck()` | MFM encoding/decoding roundtrip |
| CRC computation | `Crc16()` | CRC16 matches WD1772 polynomial |
| DMA mode | DMA transfer during sector read | DMA channel used, bytes transferred |
| Interrupt generation | Command completion | IRQ to MFP via FDC interrupt line |
| Register programming | All 10 registers | Each register takes correct values |
| Error handling | Bad sector, read error | Error codes returned correctly |
| Disk change detection | Disk swapped during read | Check flag set, read returns error |

**Test file**: `tests/tier2_component/fdc/test_fdc.cpp`
**Estimated tests**: 45
**Priority**: P1

### 5.2 floppy_drive.cpp (1,266 lines)

**What it does**: SF314 floppy drive - head loading, rotation, RPM.

| Test | Feature | Scenario |
|--|--|--|
| Head load/unload | Load head | Head settles, delay consumed |
| Seek | Move to track N | Seek time proportional to distance |
| RPM tracking | Drive at 300 RPM | Sector timing consistent |
| Media change detect | Load different disk | Check flag raised |
| Motor on/off | Motor control | Power enabled/disabled |
| Index pulse | Disk rotation cycle | Index pulse generated per rotation |
| Write protection | Write on WP disk | Write blocked, error returned |
| Single-sided vs double-sided | Side selection | Correct side active |
| Drive 0/1/2 | Three drive slots | Each drive operates independently |
| Track position read | `GetTrackPos()` | Current fractional track position |

**Test file**: `tests/tier2_component/floppy/test_floppy_drive.cpp`
**Estimated tests**: 25
**Priority**: P1

### 5.3 floppy_disk.cpp (~3,000 lines)

**What it does**: Disk image handling - loading, track data, format detection.

| Test | Feature | Scenario |
|--|--|--|
| Load .ST disk image | Standard 880KB | All 40/80 tracks loaded |
| Load .MSA disk image | MSA format | Header parsed, track data mapped |
| Load .DIM disk image | DIM format | Header parsed |
| Load .STT disk image | STT format | Per-track images loaded |
| Load .STX disk image | Pasti format | STX header parsed via Pasti DLL |
| Track read | Any format | Track bytes returned, correct length |
| Track write | Any format | Track bytes written back |
| Format detection | Unknown extension | Auto-detection from header bytes |
| Sector layout | 9/10/11 sectors | Correct sector count per track |
| Write protection read | Image marked WP | WP flag correct |

**Test file**: `tests/tier2_component/floppy/test_floppy_disk.cpp`
**Estimated tests**: 30
**Priority**: P1

### 5.4 disk_stw.cpp (1,820 lines)

**What it does**: STW MFM disk image format - track-level MFM data.

| Test | Feature | Scenario |
|--|--|--|
| STW v1 header parse | 64-byte header | Magic "STW", version 1, track count |
| STW v2 header parse | Extended header | Version 2, extra fields |
| Track data extraction | Any track | MFM-encoded bytes returned |
| Sector CRC check | Each sector | CRC computed and validated |
| Track write | Modified track data | Data written with correct CRC |
| ID field parse | Cylinder, head, sector, RPS | Fields parsed from MFM stream |
| Data field parse | Sector payload | 128/256 byte data extracted |
| Gap computation | Gap 1, 2, 3 | Gap length computed from spec |

**Test file**: `tests/tier1_unit/disk/test_disk_stw.cpp`
**Estimated tests**: 35
**Priority**: P1

### 5.5 disk_scp.cpp (~600 lines)

| Test | Feature | Scenario |
|--|--|--|
| SCP header parse | Standard SCP file | Magic detected, track count read |
| Sector data read | Any track/sector | Raw sector data returned |
| Track conversion to STW | SCP->STW | MFM encoding applied correctly |

**Test file**: `tests/tier1_unit/disk/test_disk_scp.cpp`
**Estimated tests**: 20
**Priority**: P1

### 5.6 disk_hfe.cpp (~500 lines)

| Test | Feature | Scenario |
|--|--|--|
| HFE header parse | HFE file | Magic "HFE", version, disk format |
| Track block read | Any track | Block data extracted, decompressed |
| Parameter set blocks | HFE param set | Motor, side, RPM parameters parsed |
| Raw block handling | Compressed vs raw | Correct decompression |

**Test file**: `tests/tier1_unit/disk/test_disk_hfe.cpp`
**Estimated tests**: 20
**Priority**: P1

### 5.7 archive.cpp (~800 lines)

| Test | Feature | Scenario |
|--|--|--|
| ZIP file open | Valid ZIP | Archive opened, file list populated |
| ZIP file read | Extract file | Bytes match original |
| ZIP central directory | Multi-file archive | All entries enumerated |
| 7Z support (UNIX) | 7z file | Archive opened via ArchiveAccess |
| Nested archive | Archive in archive | Recursive extraction works |
| File size tracking | Large files | Size returned correctly |

**Test file**: `tests/tier1_unit/disk/test_archive.cpp`
**Estimated tests**: 20
**Priority**: P2

### 5.8 disk_ghost.cpp (~400 lines)

| Test | Feature | Scenario |
|--|--|--|
| Ghost save create | Save hi-score data | .GHO file created with checksum |
| Ghost save load | Load matching .GHO | Data restored, checksum valid |
| Ghost checksum verify | Modified .GHO | Checksum mismatch detected |
| Memory region selection | Selectable RAM regions | Correct memory area saved |

**Test file**: `tests/tier1_unit/disk/test_disk_ghost.cpp`
**Estimated tests**: 15
**Priority**: P2

### 5.9 acsi.cpp (~1,200 lines)

| Test | Feature | Scenario |
|--|--|--|
| ACSI test unit ready | Command 0x00 | Returns ready/not-ready status |
| ACSI read block | Command 0x08 | Data block read from emulated disk |
| ACSI write block | Command 0x0A | Data block written |
| ACSI format unit | Command 0x04 | Format command processed |
| ACSI request sense | Command 0x03 | Error codes returned |
| Hard disk image mounting | Attach image | File mapped as ACSI device |
| Partition table parse | Read partition table | Partition count and geometry |
| ACSI interrupt | Command completion | Interrupt delivered to glue |

**Test file**: `tests/tier2_component/acsi/test_acsi.cpp`
**Estimated tests**: 20
**Priority**: P2

### 5.10 hd_gemdos.cpp (2,436 lines) + harddiskman.cpp (~1,500 lines)

| Test | Feature | Scenario |
|--|--|--|
| GEMDOS drive mapping | Drive C1: to path | Path resolved from drive letter |
| GEMDOS file open | Open file on C1: | TOS hook intercepts call |
| GEMDOS read | Read file data | Data read from host filesystem |
| GEMDOS write | Write file | Data written, flushed |
| GEMDOS directory listing | FSnext struct | Directory entries returned |
| Hard disk manager | Mount/unmount | Disk image attached/detached |
| Drive letter assignment | Auto-assign C1:, C2:, etc. | Sequential assignment |
| FSBuilt | File system build | Filesystem structures initialized |

**Test file**: `tests/tier2_component/acsi/test_hd_gemdos.cpp`
**Estimated tests**: 30
**Priority**: P2

### Phase 5 Summary

| File | Tests | Test Files |
|--|--|--|
| fdc.cpp | 45 | 1 |
| floppy_drive.cpp | 25 | 1 |
| floppy_disk.cpp | 30 | 1 |
| disk_stw.cpp | 35 | 1 |
| disk_scp.cpp | 20 | 1 |
| disk_hfe.cpp | 20 | 1 |
| disk_ghost.cpp | 15 | 1 |
| archive.cpp | 20 | 1 |
| acsi.cpp | 20 | 1 |
| hd_gemdos.cpp | 30 | 1 |
| harddiskman.cpp | (included with hd_gemdos) | |
| **Phase 5 Total** | **260** | **10** |

---

## Phase 6: Input Systems

### 6.1 ikbd.cpp (1,695 lines)

**What it does**: IKBD ACIA + HD6301 keyboard controller - keyboard, mouse, joystick.

| Test | Feature | Scenario |
|--|--|--|
| Keyboard command | Send key scan code | Key reported via IKBD response |
| Keyboard buffer | 128-character buffer | Keys queued, FIFO processed |
| Mouse movement | Delta X/Y | Mouse coordinates sent in next poll |
| Mouse click | Button state change | Click event reported |
| Mouse poll timing | After IKBD mouse poll | Wait time = IKBD_SCANLINES_FROM_ABS_MOUSE_POLL_TO_SEND |
| Joystick poll | Joystick state query | All 4 joystick directions + button |
| Joystick state machine | Direction transitions | State machine transitions: center -> edge -> center |
| HD6301 clock | 6301 cycle simulation | Timing consistent with 1MHz clock |
| ACIA command register | Write to IKBD | Key/mouse/joystick command dispatched |
| IKBD reset | Keyboard reset | All buffers flushed, state cleared |
| Command interpreter | IKBDI feature | Custom command processed |

**Test file**: `tests/tier2_component/ikbd/test_ikbd.cpp`
**Estimated tests**: 25
**Priority**: P1

### 6.2 key_table.cpp (~800 lines)

| Test | Feature | Scenario |
|--|--|--|
| Key mapping lookup | Host key -> ST key | Correct scan code returned |
| Keyboard layout switch | ANSI vs ISO | Different key mappings |
| Extended key processing | Function keys, arrows | Extended scan codes |
| Key repeat | Auto-repeat enabled | Repeat events generated |
| Modifier state | Shift, Ctrl, Alt | Modifier flags tracked |

**Test file**: `tests/tier1_unit/ikbd/test_key_table.cpp`
**Estimated tests**: 15
**Priority**: P2

### 6.3 acia.cpp (~600 lines)

| Test | Feature | Scenario |
|--|--|--|
| Data register read | Character available | Next byte from FIFO returned |
| Data register write | Send character | Byte queued to output |
| Command register | Set baud rate, parity | Baud rate computed correctly |
| CTS/RTS handling | Hardware flow control | CTS checked before transmit |
| Break detection | Break received | Break flag set in status register |
| TX/RX interrupt | Data sent/received | Interrupt to MFP requested |

**Test file**: `tests/tier2_component/ikbd/test_acia.cpp`
**Estimated tests**: 20
**Priority**: P1

### 6.4 rs232.cpp (~800 lines)

| Test | Feature | Scenario |
|--|--|--|
| Baud rate table | All supported rates | RS232_CalculateBaud() returns correct divisor |
| Transmit buffer | Queue byte | Byte written to output buffer |
| Receive buffer | Byte received | Byte available for ACIA |
| Loopback mode | Loopback enabled | Transmitted bytes echoed to receive |
| File dump | MIDI dump to file | Bytes written to FILE_MIDIDUMP |
| TCP/IP bridge | Network mode enabled | Socket connection established |

**Test file**: `tests/tier2_component/rs232/test_rs232.cpp`
**Estimated tests**: 20
**Priority**: P2

### 6.5 stjoy.cpp (2,657 lines)

**Primarily GUI-dependent**, but joystick state logic is testable.

| Test | Feature | Scenario |
|--|--|--|
| Joystick state machine | State transitions | Center, up, down, left, right, button |
| Multi-joy mapping | Up to 8 PC joysticks | Each mapped to ST joystick |
| Joystick setup | 6 setups saved | Setup state serialization |
| Direction dead zone | Near center reading | Ignored as "centered" |

**Test file**: `tests/tier2_component/stjoy/test_stjoy_state.cpp`
**Estimated tests**: 15
**Priority**: P2

### Phase 6 Summary

| File | Tests | Test Files |
|--|--|--|
| ikbd.cpp | 25 | 1 |
| key_table.cpp | 15 | 1 |
| acia.cpp | 20 | 1 |
| rs232.cpp | 20 | 1 |
| stjoy.cpp | 15 | 1 |
| **Phase 6 Total** | **95** | **5** |

### 7.2 emulator.cpp (1,628 lines)

| Test | Feature | Scenario |
|--|--|--|
| Agenda event scheduling | Enqueue event | Event stored with cycle timestamp |
| Agenda event processing | Cycle reaches event | Callback invoked in priority order |
| VBL agenda event | VBL callback scheduled | VBL processing at correct interval |
| HBL agenda event | HBL callback scheduled | HBL processing at correct interval |
| Sound timer event | PSG timer callback | Timer fires after correct cycle delta |
| Agenda cancellation | Cancel pending event | Event removed from queue |
| Stats tracking | Stats enabled | Disk access stats accumulated |
| Disk emulator state | Drive emulation | Drive emulator initialized |

**Test file**: `tests/tier1_unit/core/test_emulator.cpp`
**Estimated tests**: 25
**Priority**: P1

### 7.3 run.cpp (1,959 lines)

| Test | Feature | Scenario |
|--|--|--|
| Emulator run loop | Single cycle iteration | One cycle consumed, correct chips updated |
| Run state transitions | Power on -> run -> pause | State machine transitions |
| VBL run loop | Full VBL cycle | 313 HBLs processed |
| Run speed control | Speed option change | Emulation speed adjusted |
| Pause/resume | Pause then resume | State preserved across pause |
| Event processing order | Multiple events same cycle | Order matches hardware priority |
| Timer management | Timers tick | MFP timer decremented |
| Run state initialization | `RunStateInit()` | All state cleared |

**Test file**: `tests/tier2_component/run/test_run.cpp`
**Estimated tests**: 20
**Priority**: P1

### 7.4 reset.cpp (~400 lines)

| Test | Feature | Scenario |
|--|--|--|
| Power-on reset | Full reset | All components reset to initial state |
| Cold reset via keyboard | Ctrl+Alt+Del | Same as power on |
| Warm reset | Reset CPU only | CPU reset, peripherals retain state |
| TOS loading | After reset | TOS ROM at correct address |
| Memory clear | RAM zeroed | All RAM bytes = 0 |
| CPU state after reset | Register values | All registers = 0, SR = 0x2700 |
| Vector table init | Vectors at 0x000000 | All 256 vectors initialized |

**Test file**: `tests/tier2_component/reset/test_reset.cpp`
**Estimated tests**: 15
**Priority**: P1

### 7.5 computer.cpp (~800 lines)

| Test | Feature | Scenario |
|--|--|--|
| Global chip instances | All externals | Each pointer valid and non-null |
| Computer initialization | `InitComputer()` | All components initialized |
| Computer shutdown | `CloseComputer()` | Cleanup of all components |
| Memory allocation | 1MB+ RAM allocated | RAM pointer valid |

**Test file**: `tests/tier2_component/computer/test_computer.cpp`
**Estimated tests**: 10
**Priority**: P1

### 7.6 tos.cpp (1,121 lines)

| Test | Feature | Scenario |
|--|--|--|
| TOS ROM present | ROM at 0x000000 | TOS data mapped |
| TOS ROM size | 64KB or 128KB | Correct size detected |
| Cartridge detection | Cart at 0xFA0000 | Cartridge data detected |
| TOS version parse | TOS1.0/1.2/1.4/2.0 | Version byte parsed from ROM |
| TOS intercept hooks | GemDOS call in ROM | Hook vector registered |
| PRG autorun | PRG file loaded | File auto-executed |

**Test file**: `tests/tier1_unit/core/test_tos.cpp`
**Estimated tests**: 15
**Priority**: P1

### 7.7 loadsave.cpp (~800 lines) + loadsave_emu.cpp (1,656 lines)

| Test | Feature | Scenario |
|--|--|--|
| Full state save | `SaveState()` | All chip states serialized |
| Full state restore | `LoadState()` | States restored, emulator continues |
| Version compatibility | V1-VN format | Each version handled |
| Memory snapshot | RAM dump | Full RAM saved and loaded |
| Register snapshot | CPU registers | PC, SR, D0-D7, A0-A7 saved |
| Disk state save | Current disk tracks | Disk state serialized |
| Save file integrity | Checksum/size | File valid, not truncated |
| State diff | Two states differ | Correct diffs identified |

**Test file**: `tests/tier2_component/loadsave/test_loadsave.cpp`
**Estimated tests**: 20
**Priority**: P1

### Phase 7 Summary

| File | Tests | Test Files |
|--|--|--|
| emulator.cpp | 25 | 1 |
| run.cpp | 20 | 1 |
| reset.cpp | 15 | 1 |
| computer.cpp | 10 | 1 |
| tos.cpp | 15 | 1 |
| loadsave.cpp + loadsave_emu.cpp | 20 | 1 |
| **Phase 7 Total** | **105** | **6** |

---

## Phase 8: Utilities

### 8.1 acc.cpp (~400 lines)

| Test | Feature | Scenario |
|--|--|--|
| Hex string to integer | "FF" -> 255 | Case-insensitive conversion |
| Integer to hex string | 255 -> "FF" | Uppercase hex output |
| Binary to hex | 8-bit binary value | Correct hex representation |
| Byte search in buffer | Find byte in memory region | Correct offset returned |
| Pattern match | Multi-byte pattern | Pattern location returned |
| ASCII string utility | String manipulation | Buffer copies, trims |

**Test file**: `tests/tier1_unit/acc/test_acc.cpp`
**Estimated tests**: 20
**Priority**: P2

### 8.2 macros.cpp (~800 lines)

| Test | Feature | Scenario |
|--|--|--|
| Macro recording start | `StartRecordMacro()` | Recording enabled, buffer cleared |
| Macro recording stop | `StopRecordMacro()` | Recording disabled, buffer saved |
| Macro playback | Play recorded macro | Events replayed in order |
| Macro save | Save to file | Macro data persisted |
| Macro load | Load from file | Macro data restored |
| Mouse event in macro | Mouse movement recorded | Delta and VBL count stored |
| Keyboard event in macro | Key press released | Scan code and timing stored |

**Test file**: `tests/tier1_unit/utility/test_macros.cpp`
**Estimated tests**: 20
**Priority**: P2

### 8.3 historylist.cpp (~300 lines)

| Test | Feature | Scenario |
|--|--|--|
| Add item | Append to list | New item at head |
| Max size enforced | Exceed max entries | Oldest entry dropped |
| Remove item | Find and remove | Correct item removed, order preserved |
| Clear list | All items removed | List empty |

**Test file**: `tests/tier1_unit/utility/test_historylist.cpp`
**Estimated tests**: 15
**Priority**: P2

### 8.4 dataloadsave.cpp (2,484 lines)

| Test | Feature | Scenario |
|--|--|--|
| Option serialization | Save options | All options written to file |
| Option deserialization | Load options | Options restored to pre-save values |
| Disk image list save | Image path list | List serialized with paths |
| Hard disk config save | Hard disk mappings | Drive mappings persisted |
| Config migration | Old format loaded | Conversion to new format |

**Test file**: `tests/tier1_unit/utility/test_dataloadsave.cpp`
**Estimated tests**: 25
**Priority**: P2

### Phase 8 Summary

| File | Tests | Test Files |
|--|--|--|
| acc.cpp | 20 | 1 |
| macros.cpp | 20 | 1 |
| historylist.cpp | 15 | 1 |
| dataloadsave.cpp | 25 | 1 |
| **Phase 8 Total** | **80** | **4** |

---

## Phase 9: Integration Tests

### 9.1 Boot Sequence

**Test file**: `tests/tier3_integration/boot/test_boot.cpp`
**Tests**: 15

| Test | Scenario |
|--|--|
| Power on reset | Run 1000 cycles, CPU starts at vector 0 |
| Boot ROM execution | TOS code runs, screen initialized |
| Keyboard input during boot | Ctrl key held, bypasses boot |
| Disk boot | Floppy disk detected, loaded |
| Hard disk boot | HDD detected, GEMDOS initialized |

### 9.2 CPU + Memory Bus Cycles

**Test file**: `tests/tier3_integration/cpu_memory/test_bus_cycle.cpp`
**Tests**: 25

| Test | Scenario |
|--|--|
| MOVE.L (A0),D0 | Full bus cycle: address -> data -> cycles |
| MOVE.L D0,(A0) | Write bus cycle with wait states |
| Interrupt during instruction | IRQ pending, delivered after instruction |
| Bus error on write | Protected address generates bus error |
| CPU+DMA bus arbitration | CPU yields to DMA, resumes after |

### 9.3 Disk I/O Pipeline

**Test file**: `tests/tier3_integration/disk_io/test_fdc_flow.cpp`
**Tests**: 20

| Test | Scenario |
|--|--|
| Read sector end-to-end | FDC command -> drive seek -> disk read -> data returned |
| Write sector end-to-end | FDC command -> drive seek -> disk write -> verify |
| Format track | FDC format -> disk formatted -> verify CRC |
| Disk change during read | Disk swapped midway -> check flag raised -> error |

### 9.4 Video Pipeline

**Test file**: `tests/tier3_integration/video/test_video_pipeline.cpp`
**Tests**: 25

| Test | Scenario |
|--|--|
| Full scanline render | Glue VBL -> DMA fetch -> Shifter read -> frame buffer |
| Mode switch mid-frame | Resolution changed mid-VBL -> correct blend |
| STE enhanced colors | STE palette set -> colors rendered |

### 9.5 Sound Pipeline

**Test file**: `tests/tier3_integration/sound/test_sound_pipeline.cpp`
**Tests**: 15

| Test | Scenario |
|--|--|
| PSG + DMA sound generation | PSG register written -> DMA transfers -> buffer filled |
| Multi-channel mix | All 3 channels + noise active -> mixed output |

### 9.6 Interrupt System

**Test file**: `tests/tier3_integration/interrupt/test_interrupt_flow.cpp`
**Tests**: 25

| Test | Scenario |
|--|--|
| VBL -> MFP -> CPU IRQ | Full interrupt chain: VBL -> MFP timer -> CPU interrupt handler |
| Multiple IRQ sources | Disk + timer + keyboard IRQs -> priority correct |

### 9.7 Save State Roundtrip

**Test file**: `tests/tier3_integration/save_state/test_roundtrip.cpp`
**Tests**: 15

| Test | Scenario |
|--|--|
| Save + restore + continue | Save -> run N cycles from save -> verify state matches expected |
| Multi-save hierarchy | Save A -> Save B from A -> Restore A -> state at A |

### Phase 9 Summary

| Subsystem | Tests | Test Files |
|--|--|--|
| Boot | 15 | 1 |
| CPU+Memory | 25 | 1 |
| Disk I/O | 20 | 1 |
| Video | 25 | 1 |
| Sound | 15 | 1 |
| Interrupts | 25 | 1 |
| Save state | 15 | 1 |
| **Phase 9 Total** | **140** | **7** |

---

## Grand Total

| Phase | Category | Tests | Test Files | Est. Lines of Test Code |
|--|--|--|--|--|
| 0 | Infrastructure | 1 | 1 | ~200 |
| 1 | CPU | ~475 | 10 | ~12,000 |
| 2 | Memory System | 115 | 3 | ~3,000 |
| 3 | Video Pipeline | 135 | 5 | ~3,500 |
| 4 | Sound System | 75 | 3 | ~2,000 |
| 5 | Disk Emulation | 260 | 10 | ~7,000 |
| 6 | Input Systems | 95 | 5 | ~2,500 |
| 7 | Emulation Core | 105 | 6 | ~3,000 |
| 8 | Utilities | 80 | 4 | ~2,000 |
| 9 | Integration | 140 | 7 | ~4,000 |
| **TOTAL** | | **~1,481** | **54** | **~39,200** |

### Estimated Effort

| Metric | Value |
|--|--|
| Test cases | ~1,480 |
| Test source files | ~54 |
| Test source LOC | ~39,200 |
| Framework LOC | ~2,000 |
| CMake config LOC | ~500 |
| Total test LOC | **~41,700** |
| Estimated time | 12 weeks at 1 person |
