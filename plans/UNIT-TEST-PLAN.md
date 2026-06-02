# Steem SSE V4.2.0_R10 - Comprehensive Unit Test Suite Plan

## Executive Summary

This plan outlines a strategy for building a comprehensive unit test suite for the Steem SSE Atari ST emulator codebase (~130K lines of C++). The emulator follows a component-based architecture where each hardware chip is a separate class, but lacks any existing test infrastructure.

### Project Profile
| Aspect | Detail |
|--------|--------|
| Lines of code | ~130,718 |
| Source files (steem/) | 95 .cpp files |
| Header files | 95 .h files |
| Language | C++11 (raw, no framework) |
| Existing tests | None |
| Build system | GNU Make (X-build/Makefile.txt) |
| Platform | Linux (X11), Windows (MinGW/MSVC) |
| Test framework availability | G++, GCC 13.3, CMake installed; no GTest/Catch2 pre-installed |

---

## 1. Testing Strategy Overview

### 1.1 Why Google Test (GTest)

**Decision**: Use **Google Test** with **CMake** as the build system for tests.

| Framework | Pros | Cons | Decision |
|-----------|------|------|----------|
| Google Test | CMake-native, excellent mocking, fixture support, widely available, rich assertions | Larger footprint | **WINNER** |
| Catch2 | Header-only, lightweight | No built-in mocking, less CMake integration | Considered |
| doctest | Ultra-light, header-only | Minimal mocking, less mature | Considered |

### 1.2 Three-Tier Testing Approach

```
Tier 1 - PURE UNIT TESTS (no emulator state, fast)
  └── Utility classes, parser logic, math functions, CPU instruction decoding

Tier 2 - MOCKED COMPONENT TESTS (component in isolation with mocked globals)
  └── Hardware chip emulation classes (MMU, DMA, PSG, Glue, Shifter, etc.)

Tier 3 - SUBSYSTEM INTEGRATION TESTS (multiple components, minimal env)
  └── Disk I/O pipeline, CPU+memory interactions, sound pipeline, boot sequence
```

### 1.3 Core Challenge: Global State

The codebase uses extensive global state:
- ~20+ global chip instances declared in `computer.h`
- Global cycle counter `COUNTER_VAR`
- Global configuration state
- Platform-specific code paths (`WIN32` vs `UNIX`)

**Solution**: Create a **Test Framework Layer** that:
1. Provides mock versions of all global state
2. Interposes platform-specific code with no-ops
3. Captures and resets component state for each test
4. Supports deterministic replay of emulator cycles

---

## 2. Build Infrastructure

### 2.1 Directory Structure

```
steemsse-V4.2.0_R10/tests/
├── CMakeLists.txt              # Root CMake config for tests
├── README.md                   # How to build and run
├── cmake/
│   ├── FindGTest.cmake         # GTest download/fetch config
│   └── test_flags.cmake        # Common compiler flags
├── framework/
│   ├── test_helpers.h          # Common test utilities and macros
│   ├── test_helpers.cpp        # Shared test helpers
│   ├── emulator_mock.h         # Mock global state
│   ├── emulator_mock.cpp       # Mock implementations
│   ├── memory_mock.h           # Mock memory for CPU tests
│   └── event_mock.h            # Mock agenda/event system
├── tier1_unit/                 # Pure unit tests (no dependencies)
│   ├── CMakeLists.txt
│   ├── cpu/
│   ├── mmu/
│   ├── shifter/
│   ├── psg/
│   ├── disk/
│   ├── utility/
│   └── acc/
├── tier2_component/            # Mocked component tests
│   ├── CMakeLists.txt
│   ├── glue/
│   ├── mfp/
│   ├── dma/
│   ├── fdc/
│   ├── floppy/
│   ├── blitter/
│   ├── ikbd/
│   ├── rs232/
│   └── acsi/
├── tier3_integration/          # Subsystem integration tests
│   ├── CMakeLists.txt
│   ├── boot/
│   ├── cpu_memory/
│   ├── disk_io/
│   ├── sound/
│   └── video/
└── resources/                  # Test fixtures and test data
    ├── disk_images/            # Minimal disk image test data
    ├── roms/                   # Minimal TOS ROM for boot tests
    ├── save_states/            # Known-good save states
    └── expected_outputs/       # Expected output for comparison
```

### 2.2 CMake Architecture

**Root CMakeLists.txt** will:
1. Fetch GTest via `FetchContent`
2. Compile a stripped-down version of the Steem source with `TEST_BUILD` defined
3. Link individual test executables against the Steem library + GTest
4. Register all tests with `ctest`

**Key compile definitions for test builds:**
```cmake
add_definitions(-DTEST_BUILD -DUNIX -DLINUX -DSSE_DRAW_C -DSSE_RELEASE)
add_definitions(-DSSE_NO_OSD -DNO_PORTAUDIO -DNO_RTAUDIO -DSSE_NO_SCREENSAVER)
```

### 2.3 Source Compilation Strategy

**Problem**: The Makefile compiles each file individually. We need a **library target**.

**Solution**: Create a thin `steem_test_lib` that:
1. Includes all source files that don't depend on platform code (display, sound backend, etc.)
2. Uses stub implementations for X11, audio, and windowing code
3. Exposes all necessary symbols for testing

```cmake
# Library of testable source (no main.cpp, no platform-specific GUI)
add_library(steem_test_lib OBJECT
    steem/cpu.cpp steem/cpu_ea.cpp steem/cpu_op.cpp steem/cpuinit.cpp
    steem/mmu.cpp steem/ior.cpp steem/iow.cpp
    steem/glue.cpp steem/shifter.cpp steem/dma.cpp
    steem/psg.cpp steem/mfp.cpp steem/fdc.cpp
    steem/floppy_disk.cpp steem/floppy_drive.cpp
    steem/blitter.cpp steem/ikbd.cpp steem/rs232.cpp
    steem/acsi.cpp steem/emulator.cpp steem/reset.cpp
    steem/loadsave.cpp steem/tos.cpp steem/acc.cpp
    steem/archive.cpp steem/dataloadsave.cpp
    steem/disk_stw.cpp steem/disk_scp.cpp steem/disk_hfe.cpp
    steem/disk_ghost.cpp
    steem/macros.cpp steem/historylist.cpp
    steem/options.cpp steem/options_create.cpp
    steem/sound.cpp steem/computer.cpp
    steem/draw.cpp steem/palette.cpp
    steem/hd_gemdos.cpp steem/harddiskman.cpp
    steem/key_table.cpp steem/run.cpp
    steem/d2.cpp steem/debug.cpp
    # ... more files as needed
)
```

---

## 3. Testable Source File Inventory

### 3.1 Tier 1 - Pure Unit Tests (No Global State)

These files contain logic that can be tested in isolation with minimal setup:

#### CPU (High Priority - Core Emulation Logic)
| File | Lines | Testable Functions |
|------|-------|-------------------|
| `cpu_op.cpp` | 8,755 | All 210 MC68000 instruction implementations |
| `cpu_ea.cpp` | 1,916 | Effective address calculation (all 18 addressing modes) |
| `cpu.cpp` | 1,970 | `TMC68000::RunCycles()`, interrupt handling, exception vectors |
| `cpuinit.cpp` | ~200 | Register initialization, lookup tables |

#### Math/Algorithm
| File | Lines | Testable Functions |
|------|-------|-------------------|
| `acc.cpp` | ~400 | Hex conversion, byte search patterns, ASCII utilities |
| `macros.cpp` | ~800 | Macro playback/recording logic |
| `palette.cpp` | ~300 | Color palette generation, STE palette math |

#### Disk Image Formats (Pure Parsing)
| File | Lines | Testable Functions |
|------|-------|-------------------|
| `disk_stw.cpp` | 1,820 | STW v1/v2 format parsing, MFM track data |
| `disk_scp.cpp` | ~600 | SCP format parsing, SuperCard Pro images |
| `disk_hfe.cpp` | ~500 | HFE format parsing, HxC floppy emulator |
| `disk_ghost.cpp` | ~400 | Ghost disk (hi-score) format |
| `archive.cpp` | ~800 | Archive/zip extraction logic |

#### Utility Classes
| File | Lines | Testable Functions |
|------|-------|-------------------|
| `historylist.cpp` | ~300 | LRU list operations |
| `dataloadsave.cpp` | 2,484 | Data persistence serialization |

#### TOS/Cartridge
| File | Lines | Testable Functions |
|------|-------|-------------------|
| `tos.cpp` | 1,121 | Cartridge hooks, TOS interception logic |

### 3.2 Tier 2 - Mocked Component Tests (Hardware Chips)

These require mocking global state but test individual chip behavior:

| File | Lines | Primary Class | Mock Dependencies |
|------|-------|--------------|-------------------|
| `glue.cpp` | 2,217 | `TGlue` | MFP, DMA, cycle counter |
| `mfp.cpp` | ~1,800 | `TMC68901` | Glue chip, interrupt lines |
| `dma.cpp` | ~900 | `TDma` | Mmu, Shifter, Psg |
| `mmu.cpp` | ~1,500 | `TMmu` | Glue chip, bus signals |
| `shifter.cpp` | ~2,000 | `TShifter` | Dma, DMA buffers |
| `psg.cpp` | 1,122 | `TYM2149` | Sound buffers, DSP |
| `fdc.cpp` | 4,097 | `TWD1772` | FloppyDrive, FloppyDisk |
| `floppy_drive.cpp` | 1,266 | `TSF314` | FloppyDisk, FDC |
| `floppy_disk.cpp` | ~3,000 | `TFloppyDisk` | Disk I/O, image loaders |
| `blitter.cpp` | ~2,000 | `TBlitter` | Mmu, Glue, DMA |
| `ikbd.cpp` | 1,695 | `TMC6850` (IKBD) | Keyboard/mouse input |
| `rs232.cpp` | ~800 | RS232 serial | ACIA, buffers |
| `acsi.cpp` | ~1,200 | `TAcsiHdc` | Harddiskman, I/O |
| `acia.cpp` | ~600 | `TMC6850` | Serial buffers |

### 3.3 Tier 3 - Subsystem Integration Tests

| Subsystem | Files Involved | Test Scenario |
|-----------|---------------|---------------|
| Boot | `reset.cpp`, `computer.cpp`, `tos.cpp`, `emulator.cpp` | Power-on reset sequence |
| CPU+Memory | `cpu.cpp`, `mmu.cpp`, `ior.cpp`, `iow.cpp` | Full instruction cycle with memory access |
| Disk I/O | `fdc.cpp`, `floppy_drive.cpp`, `floppy_disk.cpp`, `disk_stw.cpp` | Read/write floppy sector |
| Video Pipeline | `glue.cpp`, `shifter.cpp`, `dma.cpp` | Scanline generation |
| Sound Pipeline | `psg.cpp`, `sound.cpp`, `dma.cpp` | Sound generation and buffer fill |
| Interrupt System | `mfp.cpp`, `glue.cpp`, `run.cpp` | VBL/HBL interrupt delivery |
| Save State | `loadsave.cpp`, `loadsave_emu.cpp` | Full state save/restore roundtrip |

### 3.4 Non-Testable (Excluded from Unit Tests)

These files are platform-specific or GUI-dependent and are best tested manually:

| Category | Files | Reason |
|----------|-------|--------|
| X11 Display | `display.cpp` (lines using X11) | Platform-specific display |
| Audio Backend | `interface_pulse.cpp`, `interface_pa.cpp`, `interface_rta.cpp` | Audio API dependent |
| GUI | `gui.cpp`, `gui_controls.cpp`, `stemwin.cpp`, `options.cpp`, `options_create.cpp` | X11/Windows dependent |
| HXC Dialogs | `hxc_*.cpp`, `dwin_edit.cpp`, `infobox.cpp`, `patchesbox.cpp`, `shortcutbox.cpp` | X11 widgets |
| Main | `main.cpp` | Entry point, init/shutdown |
| Resource | `rc/resource.cpp` | Resource embedding |
| Platform Misc | `osd.cpp`, `stemdialogs.cpp` | Platform-specific |
| Draw (assembly) | `steem/asm/*.asm` | Assembly draw routines |
| 3rd Party | `3rdparty/*` | External libraries |
| Libretro | `libretro/libretro-core.cpp` | Separate build target |

---

## 4. Test Framework Design

### 4.1 Test Helper Architecture

```
┌─────────────────────────────────────────────┐
│              Individual Test Cases           │
├─────────────────────────────────────────────┤
│         Google Test (TestCase/Fixture)       │
├─────────────────────────────────────────────┤
│     Test Helpers:                            │
│     • ResetEmulator()                        │
│     • MockCounterVar(value)                  │
│     • InjectCycleDelta(delta)                │
│     • CaptureMemoryRange(addr, len)          │
│     • ExpectRegisterWrite(addr, value)       │
│     • ExpectRegisterRead(addr) -> value      │
│     • RunNCycles(n)                          │
│     • AssertRegisterState(expected)          │
│     • SnapshotState() / RestoreSnapshot()    │
├─────────────────────────────────────────────┤
│     Mock Layer:                              │
│     • EmulatorMock (global state mocks)      │
│     • MemoryMock (RAM + ROM + MMIO)          │
│     • AgendaMock (event scheduling)          │
│     • InterruptMock (interrupt lines)        │
├─────────────────────────────────────────────┤
│    Steem Test Library (compiled sources)     │
└─────────────────────────────────────────────┘
```

### 4.2 The Test Framework Will Provide

1. **`TestEmulatorEnvironment`** - A fixture that:
   - Initializes minimal memory (ROM area + RAM + I/O region)
   - Resets all chip state
   - Sets up mock event agenda
   - Provides helpers for reading/writing registers and memory
   - Supports state snapshots for diff-based testing

2. **`MockCpuMemory`** - A mock memory region supporting:
   - Read/write with logging
   - Expected access pattern verification
   - I/O address interception

3. **`MockInterrupt`** - Tracks:
   - Which IRQ lines went high/low
   - Interrupt delivery timing
   - Interrupt priority ordering

4. **`CycleAccumulator`** - Verifies:
   - Per-instruction cycle counts
   - Total cycles consumed
   - Cross-component timing alignment

---

## 5. Execution Plan - Phase by Phase

### Phase 0: Infrastructure (Week 1)

**Goal**: Build system working, first "hello world" test passes.

| Step | File(s) | Description |
|------|---------|-------------|
| 0.1 | `tests/CMakeLists.txt` | Root CMake with GTest FetchContent |
| 0.2 | `tests/cmake/test_flags.cmake` | Compiler flags matching main build |
| 0.3 | `tests/framework/emulator_mock.h/.cpp` | Mock for global state |
| 0.4 | `tests/framework/memory_mock.h` | Mock memory region |
| 0.5 | `tests/framework/event_mock.h` | Mock agenda/event system |
| 0.6 | `tests/framework/test_helpers.h/.cpp` | Common test utilities |
| 0.7 | `tests/tier1_unit/CMakeLists.txt` | Tier 1 CMake config |
| 0.8 | `tests/tier1_unit/utility/test_hello.cpp` | Smoke test to verify build works |
| 0.9 | Build and run `ctest` | Verify infrastructure |

### Phase 1: Tier 1 - CPU Core (Weeks 2-3)

**Highest priority** - CPU is the heart of the emulator.

| Test File | Source File | Tests | Estimated Count |
|-----------|------------|-------|-----------------|
| `cpu/test_cpuinit.cpp` | `cpuinit.cpp` | Register init, lookup tables | ~15 |
| `cpu/test_cpu_ea.cpp` | `cpu_ea.cpp` | All 18 addressing modes, each with edge cases | ~90 |
| `cpu/test_cpu_data_move.cpp` | `cpu_op.cpp` | MOVE family (MOVE.B, W, L, MOVEQ, MOVEM) | ~40 |
| `cpu/test_cpu_alu.cpp` | `cpu_op.cpp` | ADD, SUB, AND, OR, EOR, CMP, ASR, ASL, LSR, LSL, ROT | ~80 |
| `cpu/test_cpu_bitops.cpp` | `cpu_op.cpp` | BTST, BCHG, BIT, BSET, BCLR | ~30 |
| `cpu/test_cpu_control.cpp` | `cpu_op.cpp` | BRA, BSR, Jmp, Jsr, RTS, RTE | ~35 |
| `cpu/test_cpu_multiply.cpp` | `cpu_op.cpp` | MULU, MULS, DIVU, DIVS | ~40 |
| `cpu/test_cpu_ccr.cpp` | `cpu_op.cpp` | TST, AND, OR, NOT, SBCD, NBCD, TFR, TAS | ~30 |
| `cpu/test_cpu_shift.cpp` | `cpu_op.cpp` | CHS, NEG, NEGX, EXT, EXTX, ABCD, SBCD | ~25 |
| `cpu/test_cpu_interrupt.cpp` | `cpu.cpp` | Interrupt entry/exit, reset vector, trap | ~20 |
| `cpu/test_cpu_exception.cpp` | `cpu.cpp` | Bus error, address error, illegal instruction | ~20 |
| **Subtotal** | | | **~425 tests** |

### Phase 2: Tier 1 - Math/Utilities (Week 4)

| Test File | Source File | Tests | Estimated Count |
|-----------|------------|-------|-----------------|
| `acc/test_acc_conversions.cpp` | `acc.cpp` | Hex/decimal conversion, ASCII utilities | ~25 |
| `acc/test_acc_patterns.cpp` | `acc.cpp` | Byte search, pattern matching | ~15 |
| `utility/test_palette.cpp` | `palette.cpp` | Standard/STE palette generation | ~10 |
| `utility/test_macros.cpp` | `macros.cpp` | Macro record/playback state | ~20 |
| `utility/test_historylist.cpp` | `historylist.cpp` | LRU list operations | ~15 |
| `utility/test_dataloadsave.cpp` | `dataloadsave.cpp` | Serialize/deserialize roundtrip | ~25 |
| `utility/test_tos.cpp` | `tos.cpp` | Cartridge detection, TOS hooks | ~15 |
| **Subtotal** | | | **~125 tests** |

### Phase 3: Tier 1 - Disk Image Formats (Week 5-6)

**These are golden test targets** - pure parsing, deterministic I/O.

| Test File | Source File | Tests | Estimated Count |
|-----------|------------|-------|-----------------|
| `disk/test_disk_stw.cpp` | `disk_stw.cpp` | STW v1/v2 header parse, track read/write, CRC | ~40 |
| `disk/test_disk_scp.cpp` | `disk_scp.cpp` | SCP header parse, sector read | ~20 |
| `disk/test_disk_hfe.cpp` | `disk_hfe.cpp` | HFE block parse, track data extraction | ~20 |
| `disk/test_disk_ghost.cpp` | `disk_ghost.cpp` | Ghost save/load checksums | ~15 |
| `disk/test_archive.cpp` | `archive.cpp` | ZIP extraction, file enumeration | ~20 |
| **Subtotal** | | | **~115 tests** |

### Phase 4: Tier 2 - Hardware Components (Weeks 7-10)

Each component gets its own test file with a fixture that mocks dependencies.

| Test File | Source File | Key Tests | Estimated Count |
|-----------|------------|-----------|-----------------|
| `mmu/test_mmu.cpp` | `mmu.cpp` | Memory map switching, bank access, ROM/RAM routing, wait states | ~35 |
| `mmu/test_ior.cpp` | `ior.cpp` | All MMIO read handlers | ~50 |
| `mmu/test_iow.cpp` | `iow.cpp` | All MMIO write handlers | ~50 |
| `glue/test_glue.cpp` | `glue.cpp` | VBL/HBL generation, bus arbitration, DE timing, interrupt generation | ~45 |
| `dma/test_dma.cpp` | `dma.cpp` | Sound DMA transfer, video DMA, channel priority, burst timing | ~35 |
| `shifter/test_shifter.cpp` | `shifter.cpp` | Palette register programming, resolution mode, STE enhanced colors | ~30 |
| `psg/test_psg.cpp` | `psg.cpp` | Register writes, audio output, ADSR envelope, I/O mode, fixed vol table | ~40 |
| `mfp/test_mfp.cpp` | `mfp.cpp` | Timer A/B/C, RTC, prescaler, interrupt masking, register programming | ~50 |
| `fdc/test_fdc.cpp` | `fdc.cpp` | Command execution, MFM encoding/decoding, CRC, sector read/write | ~45 |
| `floppy/test_floppy_drive.cpp` | `floppy_drive.cpp` | Head movement, RPM timing, media change detection, write gap | ~30 |
| `floppy/test_floppy_disk.cpp` | `floppy_disk.cpp` | Image loading, track data extraction, format detection | ~30 |
| `blitter/test_blitter.cpp` | `blitter.cpp` | Bit-block transfer, pattern modes, line length, wait register | ~35 |
| `ikbd/test_ikbd.cpp` | `ikbd.cpp` | Keyboard scan, mouse movement, joystick, command buffer | ~30 |
| `ikbd/test_acia.cpp` | `acia.cpp` | Read/write data, status register, baud rates, FIFO | ~25 |
| `rs232/test_rs232.cpp` | `rs232.cpp` | Baud rates, transmit/receive, flow control | ~20 |
| `acsi/test_acsi.cpp` | `acsi.cpp` | Command set, hard disk passthrough, partition handling | ~25 |
| **Subtotal** | | | **~525 tests** |

### Phase 5: Tier 3 - Integration Tests (Weeks 11-12)

| Test File | Subsystem | Tests | Estimated Count |
|-----------|-----------|-------|-----------------|
| `boot/test_boot.cpp` | Power-on + Boot | Reset vector fetch, ROM execution, POST | ~15 |
| `cpu_memory/test_bus_cycle.cpp` | CPU + MMU | Full instruction with memory access timing | ~25 |
| `cpu_memory/test_cpu_interrupt_integration.cpp` | CPU + MFP + Glue | Interrupt delivery chain | ~15 |
| `disk_io/test_fdc_flow.cpp` | FDC + Drive + Disk | Read sector end-to-end | ~20 |
| `sound/test_psg_dma.cpp` | PSG + DMA | Sound buffer generation | ~15 |
| `video/test_scanline.cpp` | Glue + DMA + Shifter | Scanline render cycle | ~10 |
| `video/test_video_modes.cpp` | Shifter + Display | Low/Med/Hires mode switching | ~15 |
| `interrupt/test_vbl_hbl.cpp` | MFP + Glue + Run | VBL/HBL event delivery | ~10 |
| `save_state/test_roundtrip.cpp` | Load/Save | Full save/restore, state diff | ~15 |
| `interrupt/test_run_loop.cpp` | Run events | Agenda scheduling and execution | ~15 |
| **Subtotal** | | | **~160 tests** |

---

## 6. Summary of Scope

| Tier | Category | Source Files | Test Files | Est. Tests | Priority |
|------|----------|-------------|------------|------------|----------|
| 1 | CPU Core | 4 | 11 | ~425 | **P0** |
| 1 | Utilities | 6 | 7 | ~125 | **P1** |
| 1 | Disk Formats | 4 | 5 | ~115 | **P1** |
| 2 | Hardware Chips | 16 | 16 | ~525 | **P1** |
| 3 | Integration | 15 | 10 | ~160 | **P2** |
| **Total** | | **~45 files** | **~49 test files** | **~1,350 tests** | |

---

## 7. Risk Assessment and Mitigations

| Risk | Impact | Mitigation |
|------|--------|------------|
| Heavy global state coupling | Tests interfere with each other | Fixture-level state reset; `TestEmulatorEnvironment` |
| Platform-specific code paths | Tests may not compile on all platforms | `#ifdef UNIX` in test stubs; platform detection in CMake |
| Long compile times | Slow feedback loop | Object library + per-test incremental builds |
| Complex chip timing behavior | Tests may be fragile | Document exact timing expectations; use tolerance ranges |
| Missing test data (ROMs, disk images) | Integration tests blocked | Generate minimal test fixtures programmatically |
| X11 library dependency | Build failures | Stub all X11 calls; compile only non-X11 sources |

---

## 8. Build and Run Commands (Target)

```bash
# Build all tests
cd tests && cmake -S . -B build && cmake --build build

# Run all tests
cd tests/build && ctest --output-on-failure

# Run specific test tier
cd tests/build && ctest -R tier1_cpu --output-on-failure

# Run with verbose output
cd tests/build && ctest --verbose -R test_cpu_alu

# Coverage report (optional)
cd tests/build && gcov -r ../src/*.cpp
```

---

## 9. Code Coverage Goals

| Component | Target Coverage |
|-----------|----------------|
| CPU (cpu_*.cpp) | 90%+ |
| Disk formats (disk_*.cpp) | 85%+ |
| Hardware chips (glue, mmu, dma, etc.) | 75%+ |
| Utilities (acc, macros, pallette) | 80%+ |
| Integration tests | 50%+ (by necessity) |
| **Overall** | **65%+** |

---

## 10. First Steps (Immediate Actions)

1. Create the `tests/` directory structure
2. Write root `CMakeLists.txt` with GTest FetchContent
3. Create `emulator_mock.h` that defines all globals as overridable
4. Write the `TestEmulatorEnvironment` fixture
5. Build a single CPU test to validate the pipeline
6. Iterate from there
