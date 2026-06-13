# Steem SSE - File-by-File, Feature-by-Feature Test Plan

## Strategy Note

The entire emulator is compiled as a single static library (`steem_test_core.a`). Each phase below creates its own test executable linked against that same library. **We never compile individual components in isolation** — all cross-component globals (CPU↔MFP↔Glue, FDC↔FloppyDrive, etc.) resolve naturally within `steem_test_core`. Platform-only calls (X11 display, PulseAudio) are stubbed in a thin shim library.

This means every test exercises REAL production code, not mocks of internal APIs. The fixture's job is to initialize real state and reset it between tests.

## Index

| Phase | Document Section | Files Covered |
|-------|-----------------|---------------|
| Phase 0 | [Infrastructure Setup](#phase-0-infrastructure-setup) | Framework files, steem_test_core library build |
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

### Build Files to Create

| # | File | Purpose |
|--|------|---------|
| 0.1 | `tests/CMakeLists.txt` | Root CMake: GTest FetchContent, defines `steem_test_core` static lib from ~70 emulator .cpp files with same flags as Makefile.txt |
| 0.2 | `tests/cmake/test_flags.cmake` | Mirror of production Makefile.txt compiler flags |
| 0.3 | `tests/framework/platform_stub.h` | Declarations of stubbed X11/audio symbols not in steem_test_core |
| 0.4 | `tests/framework/platform_stub.cpp` | No-op stub implementations (linked AFTER steem_test_core so real code wins) |
| 0.5 | `tests/framework/test_helpers.h` | Macro definitions: `EXPECT_REGISTER`, `ASSERT_CYCLES`, etc. |
| 0.6 | `tests/framework/test_helpers.cpp` | Implementation of helper functions |
| 0.7 | `tests/framework/memory_helpers.h` | Helpers to read/patch emulator RAM from tests (wraps real memory, not a mock) |
| 0.8 | `tests/framework/test_fixture.h` | `CPUTestFixture` / base fixture class: SetUp calls real init functions, TearDown frees allocations |
| 0.9 | `tests/cpu/CMakeLists.txt` | CPU test executable, links against steem_test_core + platform_stub |
| 0.10 | `tests/chip/CMakeLists.txt` | Hardware component test executable |
| 0.11 | `tests/disk/CMakeLists.txt` | Disk format/component test executable |
| 0.12 | `tests/utility/CMakeLists.txt` | Utility test executable + build smoke test |
| 0.13 | `tests/integration/CMakeLists.txt` | Integration test executable |

### Build Validation Test — Phase 0 Deliverable

**File**: `tests/utility/test_infrastructure_smoke.cpp`

**What it verifies:**
1. CMake finds GTest and steem_test_core builds with zero errors
2. Compiler flags work (same as production build)
3. Link order is correct: test → steem_test_core → platform_stub
4. Memory can be allocated via real `InitMemory()` function
5. `cpu_routines_init()` runs without crash
6. One instruction executes (`m68kProcess()`) and advances PC

```cpp
TEST(Smoke, SingleInstructionExecutes) {
    EXPECT_NO_THROW(cpu_routines_init());
    write16(0x100000u, 0x4E71u); // NOP at known RAM address
    set_pc(0x100000u);
    EXPECT_NO_THROW(execute_one());
    EXPECT_EQ(pc(), 0x100002u); // NOP advances by 2
}
```

**Dev package requirement (install before build):**
```bash
apt-get install -y libx11-dev libxext-dev libxxf86vm-dev g++ cmake make
```

---

## Phase 1: CPU Emulation

> **Rationale**: MC68000 is the heart of the emulator. ~475 tests across 10 test files. Tests exercise REAL opcode dispatch through `m68kProcess()`: fixture writes known instruction bytes to RAM, sets register state, calls `m68kProcess()`, then asserts on resulting register/memory/flag values.

### 1.1 cpuinit.cpp (~200 lines)

**What it does**: Initializes MC68000 lookup tables and CPU state via `cpu_routines_init()`.

| Test | Input | Expected Output |
|--|--|--|
| Init creates correct opcode table | Run `cpu_routines_init()` | All valid opcodes mapped, no nullptr in dispatch table |
| Illegal opcode entries point to trap1 | Check unassigned entries | `m68k_call_table[i] == m68k_trap1` for invalid patterns |
| Register reset to known state | Fixture calls `computer_reset_all(true)` | All D0-D7=0, A0-A7=0, SR matches cold reset vector |
| Stack pointer valid | After reset | A7 within RAM range (0xFFFFFFxx) |
| JSR tables initialized | Check m68k_jsr_get_* arrays | All 12 EA table pointers non-null |
| Bus function pointers set | After `SetTimingFunctions()` | pBusIdle, pBusRead, etc. point to real bus functions |

**Test file**: `tests/cpu/test_cpuinit.cpp`
**Estimated tests**: 15
**Priority**: P0

### 1.2 cpu_ea.cpp (1,916 lines)

**What it does**: Effective address (EA) calculation for all MC68000 addressing modes. The real `m68k_get_source_*` and `m68k_get_dest_*` functions from cpu_ea.cpp are called through JSR tables when instructions execute. Tests verify EA by writing specific opcodes that use each mode and checking memory/register results.

| Test Group | Count | Description |
|--|--|--|
| Data Register Direct (Dn) | 8 | D0-D7 each resolve to current register value via MOVE/Dn instructions |
| Address Register Direct (An) | 8 | A0-A7 each resolve to register as 32-bit pointer |
| Address Register Indirect (An) | 8 | Dereference An, verify pointer arithmetic via MOVE.L (A0),D1 |
| Address Register Indirect + Post-inc (An)+ | 8 | Return An, then increment by 2/4 via MOVE.L (A0)+,D1 |
| Address Register Indirect + Pre-dec -(An) | 8 | Decrement An by 2/4, then dereference via MOVE.L -(A0),D1 |
| Address Register Indirect + disp5/disp8/disp16 | 12 | Displacement offsets added to An |
| Address Register + Index (An,d16,Xn) | 16 | Index register + displacement offset |
| Absolute short/long | 8 | 16-bit and 32-bit absolute addresses |
| PC-relative | 8 | PC-relative addressing with 8/16-bit offset |
| Immediate addressing | 8 | Value embedded in instruction (MOVEQ/MOVE #imm) |
| Register combination validity | 8 | Invalid register combinations produce bus or illegal instr exception |
| **Subtotal** | **~120** | |

**Test file**: `tests/cpu/test_cpu_ea.cpp`
**Priority**: P0

### 1.3 cpu_op.cpp (506+ declared functions, large .cpp)

**What it does**: Implements all MC68000 instruction opcodes. Since the full emulator is linked, each test writes real opcode bytes to RAM and calls `m68kProcess()`. The real dispatch table routes to real implementations. Split into logical sub-suites:

#### Data Movement (30 tests)

| Test | Instruction | Scenario |
|--|--|--|
| MOVE.B src, dest | 8 | All src/dest EA combos for 8-bit |
| MOVE.W src, dest | 8 | All src/dest EA combos for 16-bit |
| MOVE.L src, dest | 8 | All src/dest EA combos for 32-bit |
| MOVEQ immediate,dn | 4 | Sign-extended 8-bit to 32-bit reg |
| MOVEM long word list | 2 | Multiple register save/restore via -(A7)/(A7)+ |

**Test file**: `tests/cpu/test_cpu_data_move.cpp`

#### ALU Operations (80 tests)

| Test | Instructions | Scenarios |
|--|--|--|
| ADD / ADDA / ADDX | 12 | Various sizes, carry propagation, flag updates |
| SUB / SUBA / SUBX | 12 | Same coverage as ADD |
| AND | 8 | Source/dest EA combos, zero flag, negative flag |
| OR | 8 | Same coverage as AND |
| EOR | 8 | Exclusive OR, flag updates |
| CMP | 8 | Compare without store, flag state validation |
| ASR / ASL / LSR / LSL (shift) | 8 | Shift operations, X flag = last bit shifted out |

**Test file**: `tests/cpu/test_cpu_alu.cpp`

#### Bit Operations (30 tests)

| Test | Instructions | Scenarios |
|--|--|--|
| BTST data,Dn / data,M | 8 | Test bit in register or memory, Z flag set to bit value |
| BCHG data,M | 6 | Clear and complement bit, Z reflects old value |
| BSET data,M | 6 | Set bit in operand |
| BCLR data,M | 6 | Clear operand bit with flag update |
| Extended EA for bit ops | 4 | (d16,An,Xn) addressing mode for bit operations |

**Test file**: `tests/cpu/test_cpu_bitops.cpp`

#### Control Flow (35 tests)

| Test | Instructions | Scenarios |
|--|--|--|
| BRA offset short/long | 8 | Branch absolute, near and far targets |
| BSR offset | 6 | Branch to subroutine, return address pushed on stack |
| Bcc (conditional) | 12 | All conditions: EQ, NE, HI, LS, CC, HS, CS, LO, GE, LT, GT, LE |
| JMP absolute/indirect | 4 | Jump to address via various EA modes |
| JSR + RTS pair | 5 | Call subroutine, verify stack push/pop of PC |

**Test file**: `tests/cpu/test_cpu_control.cpp`

#### Multiply/Divide (40 tests)

| Test | Instructions | Scenarios |
|--|--|--|
| MULU 16-bit | 8 | Unsigned multiply, 32-bit result in Dn |
| MULS 16-bit | 8 | Signed multiply, 32-bit signed result |
| DIVU by zero | 4 | DivisionByZero exception (vector 5) thrown |
| DIVU 32-bit | 4 | Unsigned division, quotient+remainder in Dn |
| DIVS 32-bit | 4 | Signed division, overflow check on quotient |
| ABCD / SBCD | 4 | Packed BCD add/subtract with carry |

**Test file**: `tests/cpu/test_cpu_multiply.cpp`

#### State and Flags (30 tests)

| Test | Instructions | Scenarios |
|--|--|--|
| TST | 4 | Set flags from zero operand, N/Z/V unaffected, C/X cleared |
| MOVE to/from SR | 6 | Full status register read/write, supervisor-only |
| MOVE to/from CCR | 4 | Condition codes only, user-mode safe |
| Privilege violation test | 4 | Execute privileged instruction in user mode → exception vector 8 |
| Flag manipulation via CCR | 7 | Set/clear individual N/Z/V/C flags, verify with conditional branch |

**Test file**: `tests/cpu/test_cpu_ccr.cpp`

#### Shift/Rotate (20 tests)

| Test | Instructions | Scenarios |
|--|--|--|
| ASR/ASL/LSR/LSL to Dn and memory | 12 | All shift directions, immediate count, register count |
| X flag captures last bit shifted | 4 | Verify X=N=sign of last bit |

**Test file**: `tests/cpu/test_cpu_shift.cpp`

### 1.4 cpu.cpp (1,970 lines)

**What it does**: CPU execution loop, exception handling, interrupt dispatch. Tests exercise the full `m68kProcess()` function including setjmp/longjmp exception paths.

| Test | Feature | Input / Setup | Expected Output |
|--|--|--|--|
| Single instruction executes | m68kProcess() once | Write NOP at PC | PC advances by 2, no register change |
| NOP consumes cycles | m68kProcess() | Check sys_cycles delta | Cycles > 0 consumed |
| Trace flag causes exception | Set T1=1 in SR | Execute any instruction | Exception vector 9 fetched after instruction completes |
| Interrupt at IPL boundary | Lower IPL via set_sr(), inject pending IRQ | Run one instruction | Exception taken, Mfp.Iack() calls dispatch real interrupt handler |
| Bus error on invalid access | Write MOVE to unmapped address | Execute | Exception vector 2 raised, fault address pushed |
| Address error (unaligned) | Write MOVE.L to odd address | Execute | Exception vector 3 raised, error code includes register number |
| Illegal instruction | Write unknown opcode (e.g. 0x4840) at PC | Execute | Exception vector 4 fetched, crash state recorded in TMC68kException |
| Divide by zero | DIVU/DIVS with Dn=0 as divisor | Execute | Exception vector 5 raised |
| TRAP instruction | Write TRAP #n (n=16..31) at PC | Execute | Exception vector n+16 fetched |
| Privilege violation | Set user mode (S=0), execute privileged op | Execute MOVE to SR | Exception vector 8, SR preserved in user state |
| Stop mode | STOP #$2700 instruction | Execute | Cpu.ProcessingState = TMC68000::STOPPED, e-clock stops |

**Test file**: `tests/cpu/test_cpu_core.cpp`
**Estimated tests**: ~40
**Priority**: P0

### Phase 1 Summary

| Source File | Tests | Test File | Priority |
|--|--|--|--|
| cpuinit.cpp | 15 | test_cpuinit.cpp | P0 |
| cpu_ea.cpp | ~120 | test_cpu_ea.cpp | P0 |
| cpu_op.cpp (data move) | 30 | test_cpu_data_move.cpp | P0 |
| cpu_op.cpp (ALU) | 80 | test_cpu_alu.cpp | P0 |
| cpu_op.cpp (bit ops) | 30 | test_cpu_bitops.cpp | P0 |
| cpu_op.cpp (control) | 35 | test_cpu_control.cpp | P0 |
| cpu_op.cpp (mul/div) | 40 | test_cpu_multiply.cpp | P0 |
| cpu_op.cpp (state/flags) | 30 | test_cpu_ccr.cpp | P0 |
| cpu_op.cpp (shift) | 20 | test_cpu_shift.cpp | P0 |
| cpu.cpp (core loop) | ~40 | test_cpu_core.cpp | P0 |
| **Phase 1 Total** | **~475** | **10 test files** | **P0** |

---

## Phase 2: Memory System

### 2.1 mmu.cpp (~1,500 lines) via ior.cpp and iow.cpp

**What it does**: Memory Management Unit - MMIO routing. Since the full emulator is linked, all real I/O handlers (ior.cpp, iow.cpp) route through real address decoding.

| Test Category | Address Range | What It Tests | Count |
|--|--|--|--|
| DMA register read/write | 0xFF8000-0xFF9FFF | Real `IORDma()`/`IOWDma()` handlers via MMIO access | ~25 |
| MFP register read/write | 0xFFA000-0xFFBFFF | Real `IORMfp()`/`IOWMfp()` via chip_register_* helpers | ~30 |
| DMA glue read/write | 0xFF8800-0xFF9FFE | Real `IORGlue()`/`IOWGlue()`, VDI, SCUMODE access | ~20 |
| Blitter register access | MMIO via IOWBlitter | Blitter parameter registers written/read back | ~15 |
| FDC register access | MMIO via IORDrive/IOWDrive | FDC command/status register read/write | ~10 |

**Test file**: `tests/chip/test_mmu_ior.cpp` (reads) and `tests/chip/test_mmu_iow.cpp` (writes)
**Estimated tests**: ~35 each
**Priority**: P1

---

## Phase 3: Video Pipeline

### 3.1 glue.cpp (2,217 lines)

**What it does**: Glue chip - VBL/HBL generation, bus arbitration, interrupt delivery. Tests exercise real `TGlue` global instance through MMIO register access and by inspecting timing state after CPU cycles run.

| Test | Feature | Scenario |
|--|--|--|
| HBL generation @ PAL 50Hz | Run enough cycles for one HBL | Glue.hbl_count incremented, DE/HSync signals correct |
| HBL timing cycle cost | Check sys_cycles delta per HBL | ~512*TICKS8 cycles consumed |
| VBL generation after 313 HBLs | Run full frame of scanlines | Glue.vbl_count incremented, VBL interrupt pending flag set |
| Bus arbitration CPU vs DMA | Set DMA active, run CPU instruction | Real wait-state function (pBusWS) adds penalty cycles |
| State machine transitions | Full VBL cycle step-by-step | top-margins → display → bottom-margins states observed |

**Test file**: `tests/chip/test_glue.cpp`
**Estimated tests**: ~35
**Priority**: P1

### 3.2 shifter.py (~2,000 lines)

| Test | Feature | Scenario |
|--|--|--|
| ST Palette load (Low/Med/Hires) | Write color registers via MMIO | 16-color or STE palette populated in real Shifter object |
| STE GDR mode | Set resolution register | Correct pixel format extracted from video RAM |
| Color register readback | Write then read back | Values match for all supported modes |

**Test file**: `tests/chip/test_shifter.cpp`
**Estimated tests**: ~20
**Priority**: P1

### 3.3 dma.py (~900 lines)

| Test | Feature | Scenario |
|--|--|--|
| Sound DMA channel activate | Write DMA enable register | DMA state shows sound channel active |
| HSync/VSync DMA transfer | Full scanline VBL cycle | Video buffer updated with data from real Shifter reads |
| Channel priority under contention | All channels active simultaneously | Priority order: HBlank > VBlank > Sound observed in cycle accounting |

**Test file**: `tests/chip/test_dma.cpp`
**Estimated tests**: ~25
**Priority**: P1

### 3.4 blitter.cpp (~2,000 lines)

| Test | Feature | Scenario |
|--|--|--|
| Register programming | Write all 8 blitter registers via MMIO | Source/dest address, word count, mode stored correctly |
| Start trigger | Write BLTCON0 with start bit | Blitter state transitions to BUSY flag |
| Wait register effect | Set BLTWAIT=nonzero | CPU idle cycles added during blitter operation |

**Test file**: `tests/chip/test_blitter.cpp`
**Estimated tests**: ~25
**Priority**: P1

### 3.5 palette.cpp (~300 lines)

| Test | Feature | Scenario |
|--|--|--|
| ST palette generation | Call real palette generation function | 16 colors in expected RGB values |
| STE palette generation | STE enhanced color path | 256 colors from hardware registers |

**Test file**: `tests/utility/test_palette.cpp`
**Estimated tests**: ~10
**Priority**: P2

---

## Phase 4: Sound System

### 4.1 psg.py (1,122 lines)

| Test | Feature | Scenario |
|--|--|--|
| Register programming | Write YM2149 registers via MMIO | Frequency, volume, noise parameters stored in real PSG object |
| Volume envelope ADSR | Set envelope shape + enable per channel | Real envelope state machine advances correctly over cycles |
| I/O port A read (keyboard) | PSG I/O direction set to input on port A | Returns keyboard/mouse data from IKBD chain |
| Noise generator | Set noise mode register | Real noise counter produces different sequences per mode |

**Test file**: `tests/chip/test_psg.cpp`
**Estimated tests**: ~35
**Priority**: P1

### 4.2 sound.cpp (2,063 lines)

Since audio backends are excluded from steem_test_core, tests focus on the internal sound buffer and PSG-to-buffer pipeline:

| Test | Feature | Scenario |
|--|--|--|
| Internal buffer fill after PSG writes | Run n cycles with PSG outputting data | Buffer contains expected sample values |
| Sound engine pause/resume | Toggle pause flag in real state | Buffer stops/starts receiving data |

**Test file**: `tests/chip/test_sound.cpp`
**Estimated tests**: ~10
**Priority**: P2

### 4.3 midi.py (1,199 lines)

| Test | Feature | Scenario |
|--|--|--|
| MIDI note parse | Feed valid MIDI bytes to real parser | Note/channel extracted correctly |
| Clock timing | Send MIDI clock pulse sequence | Timing between clocks verified |

**Test file**: `tests/chip/test_midi.cpp`
**Estimated tests**: ~10
**Priority**: P2

---

## Phase 5: Disk Emulation

### 5.1 fdc.py (4,097 lines)

| Test | Feature | Scenario | Count |
|--|--|--|--|
| Command execution | Write FDC command via MMIO, run cycles | FDC state machine transitions correctly through ready→command→active→terminating phases |
| Sector read | Load test disk image in fixture, issue READ SECTOR | Real data block returned in DSR register area |
| MFM encoding/decoding check | Read track data | CRC16 validates against WD1772 polynomial |
| Error handling | Issue read for nonexistent sector | Check flag set, error status returned |

**Test file**: `tests/chip/test_fdc.cpp`
**Estimated tests**: ~40
**Priority**: P1

### 5.2 floppy_drive.py (1,266 lines)

| Test | Feature | Scenario |
|--|--|--|
| Head load/unload cycle | Seek to track N, verify timing | Head settle delay proportional to distance |
| RPM rotation tracking | Run cycles for one rotation period | Index pulse generated at correct interval |
| Write protection detect | Mark disk as WP, attempt write | FDC returns write error through real I/O chain |

**Test file**: `tests/chip/test_floppy_drive.cpp`
**Estimated tests**: ~20
**Priority**: P1

### 5.3 floppy_disk.py (~3,000 lines)

| Test | Feature | Scenario |
|--|--|--|
| ST image load and track read | Load .ST test fixture in SetUp | Track data for any cylinder/sector accessible |
| Format detection from header bytes | Feed first 64 bytes of various formats | Correct format enum returned by real `DetectFormat()` function |

**Test file**: `tests/chip/test_floppy_disk.cpp`
**Estimated tests**: ~20
**Priority**: P1

### 5.4 disk_stw.py (1,820 lines)

| Test | Feature | Scenario |
|--|--|--|
| STW v1/v2 header parse | Valid binary blob from resources/ | Magic, version, track count parsed correctly |
| Track data extract and CRC | Known-good STW file in resources/ | MFM bytes returned, sector CRC validates |
| Write back and re-read roundtrip | Modify track data, save, reload | CRC recalculated, changes persist |

**Test file**: `tests/disk/test_disk_stw.cpp`
**Estimated tests**: ~30
**Priority**: P1

### 5.5 disk_scp.py (~600 lines)

| Test | Feature | Scenario |
|--|--|--|
| SCP header parse and sector read | Valid SCP test fixture | Sector 0 data matches expected contents |

**Test file**: `tests/disk/test_disk_scp.cpp`
**Estimated tests**: ~15
**Priority**: P1

### 5.6 disk_hfe.py (~500 lines)

| Test | Feature | Scenario |
|--|--|--|
| HFE header parse | Valid HFE test fixture | Magic, version, format fields parsed |
| Track block decompression and read | Compressed HFE track | Raw MFM bytes extracted correctly |

**Test file**: `tests/disk/test_disk_hfe.cpp`
**Estimated tests**: ~15
**Priority**: P1

### 5.7 archive.py (~800 lines)

| Test | Feature | Scenario |
|--|--|--|
| ZIP open and enumerate | Pre-built ZIP test fixture in resources/ | File list matches expected entries |
| Extract file from ZIP | Read known file from archive | Contents match uncompressed version in resources/ |

**Test file**: `tests/disk/test_archive.cpp`
**Estimated tests**: ~15
**Priority**: P2

### 5.8 disk_ghost.py (~400 lines)

| Test | Feature | Scenario |
|--|--|--|
| Ghost save and load | Save known RAM region, restore it | Checksum roundtrip matches |
| Modified ghost reject | Tamper with .GHO file | Checksum mismatch detected in real loader |

**Test file**: `tests/disk/test_disk_ghost.cpp`
**Estimated tests**: ~10
**Priority**: P2

### 5.9 acsi.py (~1,200 lines)

| Test | Feature | Scenario |
|--|--|--|
| ACSI test unit ready via MMIO | Command 0x00 through real SCSI I/O path | Status byte returned in register area |
| Hard disk image mount/unmount | Attach test HDD image file | Real filesystem hook registered |

**Test file**: `tests/chip/test_acsi.cpp`
**Estimated tests**: ~15
**Priority**: P2

### 5.10 hd_gemdos.py (2,436 lines) + harddiskman.py (~1,500 lines)

| Test | Feature | Scenario |
|--|--|--|
| GEMDOS drive mapping | Mount drive C1: to test path | Real path resolution through hd_gemdos hook |

**Test file**: `tests/chip/test_hd_gemdos.cpp`
**Estimated tests**: ~20
**Priority**: P2

### Phase 5 Summary

| Component File | Tests | Test File | Priority |
|--|--|--|--|
| fdc.py | ~40 | test_fdc.cpp | P1 |
| floppy_drive.py | ~20 | test_floppy_drive.cpp | P1 |
| floppy_disk.py | ~20 | test_floppy_disk.cpp | P1 |
| disk_stw.py | ~30 | test_disk_stw.cpp | P1 |
| disk_scp.py | ~15 | test_disk_scp.cpp | P1 |
| disk_hfe.py | ~15 | test_disk_hfe.cpp | P1 |
| archive.py | ~15 | test_archive.cpp | P2 |
| disk_ghost.py | ~10 | test_disk_ghost.cpp | P2 |
| acsi.py | ~15 | test_acsi.cpp | P2 |
| hd_gemdos.py + harddiskman.py | ~20 | test_hd_gemdos.cpp | P2 |
| **Phase 5 Total** | **~200** | **10 test files** | **P1/P2** |

---

## Phase 6: Input Systems

### 6.1 ikbd/py (1,695 lines)

| Test | Feature | Scenario |
|--|--|--|
| Keyboard command via IKBD ACIA register write | Write scan code through MMIO | Key reported in real IKBD buffer, processed on next poll |
| Mouse movement injection | Write mouse delta to IKBD | Real mouse state updated and returned on read |

**Test file**: `tests/chip/test_ikbd.cpp`
**Estimated tests**: ~20
**Priority**: P1

### 6.2 key_table.py (~800 lines)

| Test | Feature | Scenario |
|--|--|--|
| Key mapping lookup (host → ST scan code) | Known host keycode input | Correct ST scan code returned by real table |

**Test file**: `tests/utility/test_key_table.cpp`
**Estimated tests**: ~10
**Priority**: P2

### 6.3 acia.py (~600 lines)

| Test | Feature | Scenario |
|--|--|--|
| Data register read/write through MMIO | Write byte to ACIA data port | Byte available for next real ACIA read |
| Status register flags | After write operation, read status register | TXRDY/RXRDY flags correct |

**Test file**: `tests/chip/test_acia.cpp`
**Estimated tests**: ~15
**Priority**: P1

### 6.4 rs232.py (~800 lines)

| Test | Feature | Scenario |
|--|--|--|
| Baud rate table lookup | Call real baud rate calculator | Divisor matches RS232 spec for given rate |
| Transmit/receive buffer roundtrip | Write byte to TX, read from RX (loopback mode) | Byte delivered through real ring buffer |

**Test file**: `tests/chip/test_rs232.cpp`
**Estimated tests**: ~10
**Priority**: P2

### Phase 6 Summary

| File | Tests | Test File | Priority |
|--|--|--|--|
| ikbd.py | ~20 | test_ikbd.cpp | P1 |
| key_table.py | ~10 | test_key_table.cpp | P2 |
| acia.py | ~15 | test_acia.cpp | P1 |
| rs232.py | ~10 | test_rs232.cpp | P2 |
| **Phase 6 Total** | **~55** | **4 test files** | **P1/P2** |

---

## Phase 7: Emulation Core

### 7.1 emulator.py (1,628 lines) + event system

The real event agenda (`event_vector`, `prepare_next_event()`) is linked from run.cpp and emulator.cpp. Tests exercise real event scheduling:

| Test | Feature | Scenario |
|--|--|--|
| Event scheduling via prepare_next_event | Inject event at known cycle timestamp | Real Agenda queue stores event with correct priority |
| Dummy event handler | Clear agenda, call event_vector | `event_dummy` no-op executes without crash |
| Timer B event scheduling | Write MFP timer register → real chain to event setup | Correct HBL/VBL timer interval programmed in real Agenda |

**Test file**: `tests/chip/test_emulator_events.cpp`
**Estimated tests**: ~20
**Priority**: P1

### 7.2 reset.py (~400 lines)

| Test | Feature | Scenario |
|--|--|--|
| Cold reset via computer_reset_all(true) | Call real reset function | All chip globals reset to known initial states |
| Warm reset | Call with Cold=false | CPU reset but peripheral state preserved |
| Power sequence: InitMemory → computer_reset_all → cpu_routines_init → SetTimingFunctions | Call in order, then execute one CPU instruction | Full boot path works without crash |

**Test file**: `tests/integration/test_reset.cpp`
**Estimated tests**: ~10
**Priority**: P1

### 7.3 loadsave.py + loadsave_emu.py (~2,484 lines total)

| Test | Feature | Scenario |
|--|--|--|
| Full state save to buffer | `SaveState()` writes to memory buffer | All CPU registers, chip states serialized |
| Restore and compare | `LoadState()` from same buffer → run n cycles → state matches fresh run from same start | Roundtrip preserves exact state |

**Test file**: `tests/integration/test_save_state.cpp`
**Estimated tests**: ~10
**Priority**: P1

### Phase 7 Summary

| File | Tests | Test File | Priority |
|--|--|--|--|
| emulator.py + events | ~20 | test_emulator_events.cpp | P1 |
| reset.py | ~10 | test_reset.cpp (in integration/) | P1 |
| loadsave.py + loadsave_emu.py | ~10 | test_save_state.cpp (in integration/) | P1 |
| **Phase 7 Total** | **~40** | **3 test files** | **P1** |

---

## Phase 8: Utilities

### 8.1 acc.py (~400 lines)

These contain pure helper functions with no global state:

| Test | Feature | Scenario |
|--|--|--|
| Hex string ↔ integer conversion | "FF" → 255, 255 → "FF" | Case-insensitive parse, uppercase output |
| Binary pattern match in buffer | Known byte sequence in RAM region | Function returns correct offset or -1 for missing |

**Test file**: `tests/utility/test_acc.cpp`
**Estimated tests**: ~15
**Priority**: P2

### 8.2 macros.py (~800 lines)

| Test | Feature | Scenario |
|--|--|--|
| Macro record start/stop | StartRecordMacro→inject events→StopRecordMacro | Real macro buffer contains injected event data |
| Playback replay | Play recorded macro against real event loop | Events replayed in original order |

**Test file**: `tests/utility/test_macros.cpp`
**Estimated tests**: ~10
**Priority**: P2

### 8.3 historylist.py (~300 lines)

| Test | Feature | Scenario |
|--|--|--|
| Add item, max size enforced, remove, clear | Standard LRU list operations | Order preserved, oldest dropped on overflow |

**Test file**: `tests/utility/test_historylist.cpp`
**Estimated tests**: ~10
**Priority**: P2

### 8.4 dataloadsave.py (2,484 lines)

| Test | Feature | Scenario |
|--|--|--|
| Option serialization roundtrip | Save known config struct to buffer, reload from same buffer | All fields restored to original values |

**Test file**: `tests/utility/test_dataloadsave.cpp`
**Estimated tests**: ~15
**Priority**: P2

### Phase 8 Summary

| File | Tests | Test File | Priority |
|--|--|--|--|
| acc.py | ~15 | test_acc.cpp | P2 |
| macros.py | ~10 | test_macros.cpp | P2 |
| historylist.py | ~10 | test_historylist.cpp | P2 |
| dataloadsave.py | ~15 | test_dataloadsave.cpp | P2 |
| **Phase 8 Total** | **~50** | **4 test files** | **P2** |

---

## Phase 9: Integration Tests

### 9.1 Boot Sequence

**Test file**: `tests/integration/test_boot.cpp`
**Tests**: ~15

| Test | Scenario |
|--|--|
| Power on → InitMemory → cold reset → run first instructions | Run n cycles after full boot chain | CPU starts at vector 0 from ROM, SR set disabled, PC advances through real TOS code in synthetic ROM |
| Boot with floppy disk loaded | Load .ST image in fixture before reset | FDC initialized, drive state reflects mounted media |

### 9.2 CPU + Memory Full Bus Cycle

**Test file**: `tests/integration/test_cpu_memory_bus.cpp`
**Tests**: ~20

| Test | Scenario |
|--|--|--|
| MOVE.L (A0),D1 through full memory chain | Write data at address, set A0, execute MOVE.L | Real cpu_ea.cpp EA calc + real mmu.cpp address decode + real pBusRead timing = correct value in D1 with expected cycle count |
| Interrupt during instruction execution | Inject pending IRQ while CPU runs | Real Mfp interrupt handler called via Iack(), exception vector dispatched from Glue, new PC/SR pushed on stack |

### 9.3 Full Disk Read Pipeline

**Test file**: `tests/integration/test_disk_io_flow.cpp`
**Tests**: ~15

| Test | Scenario |
|--|--|
| FDC command → drive seek → disk read → data returned | End-to-end sector read through real I/O chain | Sector bytes match expected test data from fixture image |

### 9.4 Sound Pipeline

**Test file**: `tests/integration/test_sound_pipeline.cpp`
**Tests**: ~10

| Test | Scenario |
|--|--|
| PSG register write → run n cycles → sample buffer check | Real PSG output flows through internal DMA path | Buffer contains expected waveform samples at correct frequency |

### 9.5 Save State Roundtrip

**Test file**: `tests/integration/test_save_state_roundtrip.cpp`
**Tests**: ~10

| Test | Scenario |
|--|--|
| Save state S → run n cycles from S → verify matches fresh reset + n cycles | Independent run from same seed state produces identical result | Roundtrip deterministic, no hidden state drift |

### Phase 9 Summary

| Scenario | Tests | Test File | Priority |
|--|--|--|--|
| Boot | ~15 | test_boot.cpp | P2 |
| CPU+Memory bus cycle | ~20 | test_cpu_memory_bus.cpp | P2 |
| Disk I/O pipeline | ~15 | test_disk_io_flow.cpp | P2 |
| Sound pipeline | ~10 | test_sound_pipeline.cpp | P2 |
| Save state roundtrip | ~10 | test_save_state_roundtrip.cpp | P2 |
| **Phase 9 Total** | **~70** | **5 test files** | **P2** |

---

## Grand Total

| Phase | Category | Source Files Tested (via steem_test_core) | Test Executables | Est. Tests |
|-------|----------|------------------------------------------|-----------------|------------|
| 0 | Infrastructure | ~70 .cpp files → steem_test_core.a | 1 (smoke) | 1 |
| 1 | CPU Emulation | cpu.cpp, cpu_ea.cpp, cpu_op.cpp, cpuinit.cpp | 1 | ~475 |
| 2 | Memory System | mmu.cpp, ior.cpp, iow.cpp | 1 | ~35 |
| 3 | Video Pipeline | glue.cpp, shifter.cpp, dma.cpp, blitter.cpp | 1 | ~95 |
| 4 | Sound System | psg.cpp, sound.cpp, midi.cpp | 1 | ~55 |
| 5 | Disk Emulation + formats | fdc.py, floppy_*, disk_*, archive.py, acsi.py, hd_gemdos.py | 2 (chip/ + disk/) | ~200 |
| 6 | Input Systems | ikbd.cpp, key_table.cpp, acia.cpp, rs232.cpp | 1 | ~55 |
| 7 | Emulation Core | emulator.cpp, run.cpp, reset.cpp, loadsave*.cpp | 1 | ~40 |
| 8 | Utilities | acc.cpp, macros.cpp, historylist.cpp, dataloadsave.cpp | 1 | ~50 |
| 9 | Integration | Cross-component (all above) | 1 | ~70 |
| **Total** | | **~70 .cpp files compiled once** | **~9 test executables** | **~1,076 tests** |

### Estimated Effort

| Metric | Value |
|--|--|
| Test cases | ~1,080 |
| Test source files | ~35 |
| Production .cpp in steem_test_core | ~70 (compiled once) |
| Framework/stub/fixture LOC | ~1,500 |
| Estimated time | 12-14 weeks at 1 person |

### Key Difference from Original Plan

The original plan attempted to compile only CPU source files in isolation with stubbed dependencies. That approach failed because:
1. `pch.h` pulls in X11/audio headers that prevent clean header-only compilation on minimal systems
2. Global cross-references between every component are impossible to mock without editing production source
3. Function pointer tables (`m68k_call_table`, JSR tables) span files and must compile together

**New approach**: One library build, one set of compiler flags (copied from Makefile.txt), tests linked against the whole thing. This means:
- Zero refactoring of production code
- All test assertions validate against real state
- If a test passes on the full library, it will pass in production — no "mocks diverge from reality" risk