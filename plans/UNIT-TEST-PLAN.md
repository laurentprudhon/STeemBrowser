ok# Steem SSE V4.2.0_R10 - Comprehensive Unit Test Suite Plan

## Executive Summary

This plan outlines a strategy for building a comprehensive unit test suite for the STeem SSE Atari ST emulator codebase (~130K lines of C++). **The emulator code is NEVER modified.** Tests compile the entire emulator as a single static library and link per-component test executables against it.

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

## 1. Core Strategy: Full Linkage, Modular Tests

### 1.1 Fundamental Decision

**We do NOT compile individual emulator components in isolation.** Instead:

```
┌──────────────────────────────┐
│     steem_test_core.a        │ ← ENTIRE emulator compiled as ONE static library
│                              │   Same source files, same flags as production build
│  cpu.cpp  mmu.cpp  glue.cpp  │   Only excludes: main.cpp, X11 platform code,
│  fdc.cpp  psg.cpp  dma.cpp   │   audio backends
│  ... all ~70 testable .cpps  │
└──────────────┬───────────────┘
               │ single library, ALL symbols resolved internally
         ┌─────▼──────┐ ┌─────▼──────┐ ┌─────▼──────┐
         │ cpu_tests  │ │ fdc_tests   │ │ glue_tests │ ← Per-component test executables
         │            │ │             │ │             │
         │ [GTest]    │ │ [GTest]     │ │ [GTest]     │
         └────────────┘ └─────────────┘ └─────────────┘
```

### 1.2 Why This Approach

| Issue with Isolation | How Full Linkage Solves It |
|---------------------|---------------------------|
| `pch.h` includes X11, audio, joystick headers that break compilation on minimal systems | No problem — we compile the full emulator which already handles these via Makefile.txt |
| Cross-component global references (`Blitter`, `Mfp`, etc.) are impossible to isolate without editing source | All globals link naturally because everything is in one library |
| Function pointer dispatch tables (`m68k_call_table`) span cpu.cpp, cpu_op.cpp, cpu_ea.cpp — must compile together | They compile together by design |
| CPU's `#include <computer.h>` declares externs for every chip — impossible to mock all of them | No mocking needed — the real chip globals resolve from the same library |
| Refactoring source code to expose APIs would violate the "never touch production" rule | Not needed — we test through the existing global/public API |

### 1.3 What Gets Mocked

Since the emulator is linked as a whole, ONLY these need stubbing:

| Category | What | Why |
|----------|------|-----|
| **Platform X11 display** | `Display*`, X resource DB, window creation | Tests run headless |
| **Audio backends** | PulseAudio, PortAudio, RtAudio init/close | Tests don't produce sound |
| **File I/O for runtime** | Live disk images, ROM loading paths | Tests use synthetic data or embedded test fixtures |
| **main() entry point** | The `main.cpp` itself | Tests initialize state themselves via fixture setup |

### 1.4 Google Test with CMake

**Decision**: Use **Google Test** fetched via `FetchContent`. Same rationale as before: CMake-native, excellent fixture support, widely understood assertions.

---

## 2. Build Infrastructure

### 2.1 Directory Structure

```
tests/
├── CMakeLists.txt                    # Root CMake: GTest fetch, steem_test_core lib build
├── cmake/
│   └── test_flags.cmake              # Mirror production Makefile.txt flags exactly
├── framework/
│   ├── test_helpers.h                # Common helpers: EXPECT_REGISTER, ASSERT_CYCLES, etc.
│   ├── test_helpers.cpp
│   ├── platform_stub.h               # DECLARATIONS of stubbed X11/audio functions
│   ├── platform_stub.cpp             # Stub implementations (link after steem_test_core)
│   ├── memory_helpers.h              # Helpers to read/patch emulator RAM from tests
│   └── test_fixture.h                # TestEmulatorEnvironment base fixture class
├── cpu/                              # CPU component tests
│   ├── CMakeLists.txt
│   ├── test_cpuinit.cpp
│   ├── test_cpu_ea.cpp
│   ├── test_cpu_data_move.cpp
│   ├── test_cpu_alu.cpp
│   ├── test_cpu_bitops.cpp
│   ├── test_cpu_control.cpp
│   ├── test_cpu_multiply.cpp
│   ├── test_cpu_ccr.cpp
│   ├── test_cpu_shift.cpp
│   └── test_cpu_core.cpp
├── disk/                             # Disk format/component tests
│   ├── CMakeLists.txt
│   ├── test_disk_stw.cpp
│   ├── test_disk_scp.cpp
│   ├── test_disk_hfe.cpp
│   ├── test_disk_ghost.cpp
│   └── test_archive.cpp
├── chip/                             # Hardware chip component tests
│   ├── CMakeLists.txt
│   ├── test_mmu_ior.cpp
│   ├── test_mmu_iow.cpp
│   ├── test_glue.cpp
│   ├── test_mfp.cpp
│   ├── test_dma.cpp
│   ├── test_shifter.cpp
│   ├── test_psg.cpp
│   ├── test_fdc.cpp
│   ├── test_floppy_drive.cpp
│   ├── test_floppy_disk.cpp
│   ├── test_blitter.cpp
│   ├── test_ikbd.cpp
│   ├── test_acia.cpp
│   ├── test_rs232.cpp
│   └── test_acsi.cpp
├── utility/                          # Utility component tests
│   ├── CMakeLists.txt
│   ├── test_acc.cpp
│   ├── test_palette.cpp
│   ├── test_macros.cpp
│   ├── test_historylist.cpp
│   ├── test_dataloadsave.cpp
│   └── test_infrastructure_smoke.cpp  # Phase 0 build smoke test
├── integration/                      # Cross-component integration tests
│   ├── CMakeLists.txt
│   ├── test_boot.cpp
│   ├── test_cpu_memory_bus.cpp
│   ├── test_disk_io_flow.cpp
│   ├── test_sound_pipeline.cpp
│   ├── test_video_scanline.cpp
│   ├── test_interrupt_chain.cpp
│   └── test_save_state_roundtrip.cpp
└── resources/                        # Test data
    ├── disk_images/                  # Minimal generated test images per format
    ├── roms/                         # Synthetic TOS ROM for boot tests
    └── binary_fixtures/              # Known-good input/output binary blobs
```

### 2.2 CMake Architecture — Three-Layer Library Build

#### Layer 1: `steem_test_core` (ONE static library, ENTIRE emulator)

```cmake
# Compiled with EXACTLY the same flags as Makefile.txt
add_library(steem_test_core STATIC
    # --- CPU ---
    ${SEEM}/steem/cpu.cpp
    ${SEEM}/steem/cpu_ea.cpp
    ${SEEM}/steem/cpu_op.cpp
    ${SEEM}/steem/cpuinit.cpp
    # --- chips ---
    ${SEEM}/steem/mmu.cpp
    ${SEEM}/steem/ior.cpp
    ${SEEM}/steem/iow.cpp
    ${SEEM}/steem/glue.cpp
    ${SEEM}/steem/shifter.cpp
    ${SEEM}/steem/dma.cpp
    ${SEEM}/steem/blitter.cpp
    ${SEEM}/steem/mfp.cpp
    ${SEEM}/steem/psg.cpp
    # --- disk ---
    ${SEEM}/steem/fdc.cpp
    ${SEEM}/steem/floppy_drive.cpp
    ${SEEM}/steem/floppy_disk.cpp
    ${SEEM}/steem/disk_stw.cpp
    ${SEEM}/steem/disk_scp.cpp
    ${SEEM}/steem/disk_hfe.cpp
    ${SEEM}/steem/disk_ghost.cpp
    ${SEEM}/steem/archive.cpp
    ${SEEM}/steem/hd_gemdos.cpp
    ${SEEM}/steem/harddiskman.cpp
    # --- I/O ---
    ${SEEM}/steem/ikbd.cpp
    ${SEEM}/steem/acia.cpp
    ${SEEM}/steem/rs232.cpp
    ${SEEM}/steem/acsi.cpp
    # --- core ---
    ${SEEM}/steem/emulator.cpp
    ${SEEM}/steem/reset.cpp
    ${SEEM}/steem/run.cpp
    ${SEEM}/steem/computer.cpp
    ${SEEM}/steem/tos.cpp
    ${SEEM}/steem/loadsave.cpp
    ${SEEM}/steem/loadsave_emu.cpp
    # --- utility ---
    ${SEEM}/steem/acc.cpp
    ${SEEM}/steem/macros.cpp
    ${SEEM}/steem/historylist.cpp
    ${SEEM}/steem/dataloadsave.cpp
    ${SEEM}/steem/palette.cpp
    ${SEEM}/steem/draw.cpp         # draw.cpp resolves function pointers, no X11
    ${SEEM}/steem/sound.cpp
    ${SEEM}/steem/stports.cpp
    ${SEEM}/steem/midi.cpp
    # --- extras needed for full linkage ---
    ${SEEM}/steem/options.cpp
    ${SEEM}/steem/debug.cpp
    ${SEEM}/steem/d2.cpp
    ${SEEM}/steem/iolist.cpp
    ${SEEM}/steem/key_table.cpp
    ${SEEM}/steem/wordwrapper.cpp
    # --- code/ subdirs ---
    ${SEEM}/steem/code/draw_c/draw_c.cpp
)

target_include_directories(steem_test_core PRIVATE
    ${SEEM}/include              # binary.h, clarity.h, data_union.h
    ${SEEM}/steem/headers        # pch.h, cpu.h, emulator.h, etc.
    ${SEEM}/3rdparty             # zlib, rtaudio, 6301 headers
    ${SEEM}/3rdparty/zlib
    ${SEEM}/3rdparty/zlib/contrib/minizip
    ${SEEM}/3rdparty/rtaudio
    ${SEEM}/3rdparty/6301
    ${SEEM}/3rdparty/dsp
    ${SEEM}/3rdparty/dsp/FIR-filter-class
    ${SEEM}/steem/code           # debug.h, computer.h, gui.h, osd.h
    ${SEEM}/steem/code/x         # display.h (UNIX-specific declarations)
)

# EXACT mirror of Makefile.txt CFLAGS + STEEMFLAGS
target_compile_options(steem_test_core PRIVATE
    -w -Wfatal-errors -fpermissive
    -O -O2 -std=gnu++11
)

target_compile_definitions(steem_test_core PRIVATE
    UNIX LINUX SSE_DRAW_C SSE_RELEASE NO_DEBUG_BUILD NDEBUG
    SSE_NO_OSD        # disable On-Screen Display
    NO_PORTAUDIO      # no PortAudio init
    NO_RTAUDIO        # no RtAudio init
    TEST_BUILD        # tells some code paths to skip platform init
)

target_link_libraries(steem_test_core PUBLIC
    pthread           # Makefile.txt links -lpthread
)
```

#### Layer 2: Platform stubs (ONE static library, thin shim)

```cmake
add_library(platform_stub STATIC
    framework/platform_stub.cpp
)
target_compile_definitions(platform_stub PRIVATE UNIX LINUX TEST_BUILD)
```

This provides exactly the functions/classes that `steem_test_core` calls which require X11/audio:
- X11 display/window creation → no-op
- Audio backend init/close → no-op
- Any filesystem paths → redirect to test resources/ directory

#### Layer 3: Per-component test executables

Each test directory has its own CMakeLists that creates a test executable:

```cmake
# cpu/CMakeLists.txt example
add_executable(cpu_unit_tests
    test_cpuinit.cpp
    test_cpu_ea.cpp
    test_cpu_data_move.cpp
    # ... more test files
)
target_link_libraries(cpu_unit_tests
    PRIVATE GTest::gtest_main steem_test_core platform_stub
)
gtest_discover_tests(cpu_unit_tests)
```

### 2.3 Build Order and Symbol Resolution

```
Link order for each test executable:
  1. GTest::gtest_main          ← test framework
  2. [component_test_lib]        ← test code for this component
  3. steem_test_core             ← full emulator (ALL symbols defined)
  4. platform_stub               ← overrides platform-only symbols

steem_test_core already resolves ALL internal cross-references:
  cpu.cpp → Mfp.*, Glue.*, Blitter.*  → resolved within same library
  cpu_ea.cpp → io_read(), io_write()   → resolved within same library (via ior.cpp, iow.cpp)
  fdc.cpp → FloppyDrive.*              → resolved via floppy_drive.cpp in same library
  ALL globals defined in their real .cpp files
```

---

## 3. Test Categories and What They Test

### 3.1 CPU Tests

Tests the MC68000 emulation. The CPU is stateless per test: fixture initializes register state, writes known instruction bytes to mock memory region in RAM, calls `m68kProcess()`, then asserts on resulting register/memory state.

Each test follows this pattern:
```cpp
TEST_F(CpuTestFixture, MoveD0ToD1) {
    setup_register(0, 0xDEADBEEF);    // D0 = 0xDEADBEEF
    write_instruction_at(pc, 0x4860); // MOVE.L D0,D1
    cpu_execute();                    // calls m68kProcess()
    EXPECT_REGISTER_EQ(1, 0xDEADBEEF); // D1 now equals old D0
}
```

### 3.2 Disk Format Tests

Pure parsing tests — given a binary blob of known format, verify the parser produces the expected internal representation. No emulator state needed; these can often call static/public functions directly.

### 3.3 Hardware Chip Tests

Each chip has real global instances in `steem_test_core`. The fixture resets the chip's state between tests by writing known values to its registers via MMIO (which is real code, not stubbed). Tests then assert on register reads or observable effects.

### 3.4 Utility Tests

Test standalone helper functions: hex/binary conversions, palette generation, history list operations, macro record/playback logic.

### 3.5 Integration Tests

Run the emulator through multi-component scenarios: boot sequence (power-on → reset vector fetch → TOS execution), full disk read (FDC command → drive seek → sector data returned), or save/restore roundtrip (save state → modify → restore → verify).

---

## 4. Test Framework Design

### 4.1 The `TestEmulatorEnvironment` Fixture

Base class for tests that need emulator state:

```cpp
class TestEmulatorEnvironment : public ::testing::Test {
protected:
    void SetUp() override {
        // Call real init functions from steem_test_core
        InitMemory();             // allocates RAM, maps ROM region
        computer_reset_all(true); // Cold reset: initializes all chip globals
        SetTimingFunctions();     // Sets pBus* function pointers (from cpu.cpp)

        // Set up known-good memory fixture
        write_test_rom();         // Synthetic minimal TOS vectors
        zero_ram();
    }

    void TearDown() override {
        // Free dynamic allocations made by real InitMemory()
    }

    // --- CPU helpers ---
    void set_register(int n, uint32_t val);
    uint32_t reg(int n) const;
    uint16_t sr() const;
    void execute_one();                  // calls m68kProcess()
    void execute_n(uint32_t cycles);     // spins m68kProcess until cycle budget exhausted

    // --- Memory helpers ---
    void write_byte(uint32_t addr, uint8_t val);
    uint8_t read_byte(uint32_t addr) const;
    void write_instruction_at(uint32_t addr, uint16_t opcode);

    // --- Chip I/O helpers ---
    void chip_register_write(uint32_t addr, uint16_t val);  // real iow path
    uint16_t chip_register_read(uint32_t addr) const;       // real ior path
};
```

### 4.2 What's Available to Tests

Since the full emulator is linked, tests can:
- Access global chip instances directly (`Cpu`, `Mmu`, `Glue`, etc.) — they're extern symbols from the library
- Call internal functions via forward declarations in test files (e.g., declare `extern void m68kProcess()`)
- Read/write emulator RAM through real memory accessor functions
- Observe and assert on timing cycles, register state, interrupt lines
- Inject data by writing to known addresses before execution

### 4.3 What's Stubbed in `platform_stub.cpp`

Only the symbols that require platform-specific APIs:

| Symbol | Stub Behavior |
|--------|--------------|
| X11 display/window functions | Return null-safe values, no-op |
| PulseAudio/RtAudio init | No-op, return fake handle |
| Keyboard/mouse event poll | Return empty/stable events |
| File paths (ROM search) | Redirect to `tests/resources/` |

---

## 5. Execution Plan — Phase by Phase

### Phase 0: Infrastructure (Week 1)

**Goal**: Build the full emulator as a static library, verify first test links and runs.

| Step | Deliverable |
|------|-------------|
| 0.1 | `tests/CMakeLists.txt` with GTest FetchContent + `steem_test_core` target |
| 0.2 | `cmake/test_flags.cmake` mirroring Makefile.txt flags |
| 0.3 | `framework/platform_stub.cpp` — minimal stubs for platform calls |
| 0.4 | `framework/test_fixture.h` + `test_helpers.h/.cpp` |
| 0.5 | `utility/CMakeLists.txt` with one smoke test |
| 0.6 | **Build succeeds**, smoke test runs and passes |

**Deliverable**: Full emulator compiles as `steem_test_core.a`, links against a GTest executable, first test passes.

### Phase 1: CPU Core (Weeks 2-6)

**Highest priority** — 475+ tests across 10 files.

See separate PHASE1-WORK-PLAN.md and UNIT-TEST-FILE-BY-FILE.md for detail.

### Phase 2: Disk Formats + Utilities (Weeks 7-8)

| Component | Tests | Priority |
|-----------|-------|----------|
| disk_stw, disk_scp, disk_hfe, disk_ghost, archive | ~135 | P1 |
| acc, palette, macros, historylist, dataloadsave | ~125 | P1 |

### Phase 3: Hardware Chips (Weeks 9-14)

| Chip | Tests | Notes |
|------|-------|-------|
| MMU + IOR + IOW | ~135 | Memory map routing |
| Glue | ~45 | VBL/HBL, bus arbitration |
| MFP | ~50 | Timers, RTC, interrupts |
| DMA | ~35 | Sound/video transfer |
| Shifter + Blitter | ~65 | Video pipeline |
| PSG | ~40 | Audio register programming |
| FDC + Floppy | ~105 | Disk I/O chain |
| IKBD + ACIA + RS232 | ~75 | Peripherals |
| ACSI | ~25 | Hard disk passthrough |

### Phase 4: Integration Tests (Weeks 15-16)

Multi-component scenarios that exercise the real execution loop.

---

## 6. Total Scope

| Phase | Category | Source Files Compiled | Test Files | Est. Tests |
|-------|----------|---------------------|------------|------------|
| 0 | Infrastructure | ~70 emulator .cpp → steem_test_core.a | 1 | 1 |
| 1 | CPU Core | (same lib) | 10 | ~475 |
| 2 | Disk + Utility | (same lib) | 8 | ~260 |
| 3 | Hardware Chips | (same lib) | 14 | ~580 |
| 4 | Integration | (same lib) | 7 | ~160 |
| **Total** | | **~70 .cpp files once** | **~40 test executables** | **~1,476 tests** |

---

## 7. Risk Assessment

| Risk | Impact | Mitigation |
|------|--------|------------|
| Large compile times for steem_test_core | Slow feedback on each test | Object library caching; only recompile changed files |
| Platform stubs incomplete | Link failures | Build once, resolve all missing symbols up front in Phase 0 |
| Global state interference between tests | Flaky tests | Fixture-level reset using real `computer_reset_all()` |
| X11 dev headers not available on CI | Build failure | Install packages: libx11-dev, libxext-dev, libxxf86vm-dev (Phase 0 task) |
| Memory allocation in emulator init fails under test fixture limits | Crash on SetUp | Allocate realistic memory sizes; don't over-allocate |
| Audio backend stubs silently fail | Sound tests miss coverage | Stub returns detectable failure codes; assertions verify stub path taken |

---

## 8. Build and Run Commands (Target)

```bash
# Build full emulator test library + all test executables
cd tests && cmake -S . -B build && cmake --build build

# Run ALL tests
cd tests/build && ctest --output-on-failure

# Run CPU tests only
cd tests/build && ctest -R cpu_ --output-on-failure

# Run a single test with verbose output
cd tests/build && ./cpu/cpu_unit_tests --gtest_filter="MoveD0ToD1" --gtest_verbose

# Coverage report (optional)
cd tests/build && gcov $(find ../.. -name "*.gcda") # after building with -fprofile-arcs
```

---

## 9. Code Coverage Goals

| Component | Target |
|-----------|--------|
| CPU (cpu_*.cpp) | 90%+ |
| Disk formats (disk_*.cpp, archive.cpp) | 85%+ |
| Hardware chips (glue, mmu, dma, fdc, etc.) | 75%+ |
| Utilities (acc, macros, palette, historylist) | 80%+ |
| Integration tests | 50%+ |
| **Overall** | **65%+** |

---

## 10. What Changed From Previous Plan

The previous plan attempted to compile individual components in isolation with stubbed dependencies. This failed because:

1. `pch.h` pulls in X11, audio, and joystick headers — impossible to strip without modifying production source
2. Global variables span every component — cpu.cpp references `Blitter`, `Mfp`, etc. directly
3. Function pointer dispatch tables require all CPU source files compiled together
4. The stub layer was larger than the code it tried to isolate

**New approach**: Compile everything once, mock only platform APIs, test by grouping related assertions into separate executables. The emulator library is built with the SAME flags and SAME source files as the production Makefile.txt build. No production code is ever modified.

This means the test framework's job shifts from "provide fake implementations of internal symbols" to "initialize real state, reset between tests, provide assertion helpers."

---

## 11. First Steps (Immediate Actions)

1. Create `tests/` directory structure per Section 2.1
2. Write root `CMakeLists.txt` — define `steem_test_core` from the same .cpp list as Makefile.txt OBJS
3. Write `platform_stub.cpp` — stub X11 display functions, audio init/cleanup
4. Install missing dev packages: `libx11-dev libxext-dev libxxf86vm-dev`
5. Build `steem_test_core.a` successfully
6. Write one smoke test that initializes memory and reads a CPU register
7. Iterate from there
