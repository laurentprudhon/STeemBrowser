# Steem Emulator Testing

## Overview

Steem SSE employs a multi-faceted testing approach that combines automated testing, manual verification, and debugging capabilities to ensure accuracy in Atari ST emulation. The testing framework is deeply integrated with the emulator's architecture, leveraging the cycle-accurate emulation core for validation.

## Testing Architecture

### 1. Debug Build System

**Primary Testing Mechanism**: Debug builds with extensive instrumentation
- **File**: `steem/debug.cpp`, `steem/debugger.cpp`, `steem/debug_emu.cpp`
- **Purpose**: Provides comprehensive debugging capabilities for testing emulator accuracy
- **Activation**: Compiled with `DEBUG_BUILD` flag

**Key Debug Components**:
- **TDebugger**: Full-featured debugger with breakpoints, memory inspection, and register monitoring
- **TDebugEmu**: Emulation-level debugging with cycle-accurate tracing
- **TMrStatic**: Memory browser and static analysis tools
- **TDebugFrameReport**: Frame-by-frame execution analysis

### 2. Memory State Validation

**Memory Snapshot Testing**:
- **Implementation**: `steem/loadsave.cpp`, `steem/loadsave_emu.cpp`
- **Functionality**: Save and restore complete emulator state for regression testing
- **Formats**: Native Steem format (.stm), memory snapshots, save states

**Memory Browser**:
- **File**: `steem/mem_browser.cpp`
- **Features**: Real-time memory inspection, breakpoints on memory access, memory comparison tools

### 3. Cycle-Accurate Verification

**Timing Validation**:
- **Core**: Global `COUNTER_VAR` system tracks cycle counts across all components
- **Components**: Each hardware chip (Glue, MMU, DMA, Shifter, CPU, etc.) maintains cycle counters
- **Verification**: Cross-component timing validation ensures synchronization

**Event System Testing**:
- **File**: `steem/emulator.cpp`
- **Agenda System**: Event-driven execution with cycle-accurate scheduling
- **Validation**: Event timing verification against known hardware behavior

### 4. Hardware Component Testing

#### CPU Testing (MC68000)
- **File**: `steem/cpu.cpp`, `steem/cpu_op.cpp`, `steem/cpu_ea.cpp`
- **Test Methods**:
  - Instruction-level tracing (`TDebugger::Trace()`)
  - Effective address calculation validation
  - Exception handling verification
  - Bus cycle accuracy testing

#### Glue Chip Testing
- **File**: `steem/glue.cpp`
- **Test Focus**: Bus arbitration, memory access timing, interrupt generation
- **Validation**: Cycle-count verification for memory access patterns

#### MMU Testing
- **File**: `steem/mmu.cpp`
- **Test Coverage**: Memory mapping, bank switching, ROM/RAM access patterns
- **Verification**: Memory access timing against hardware specifications

#### Shifter Testing
- **File**: `steem/shifter.cpp`
- **Test Areas**: Video memory access, palette handling, display timing
- **Validation**: Screen rendering accuracy, color palette fidelity

#### DMA Testing
- **File**: `steem/dma.cpp`
- **Test Focus**: DMA transfer timing, sound DMA synchronization, memory access patterns

#### PSG Testing (YM2149)
- **File**: `steem/psg.cpp`
- **Test Methods**: Audio output verification, register write timing, sound generation accuracy

### 5. Disk System Testing

**FDC Testing**:
- **File**: `steem/fdc.cpp`, `steem/floppy_drive.cpp`, `steem/floppy_disk.cpp`
- **Test Coverage**:
  - Disk image format compatibility (ST, MSA, DIM, etc.)
  - Track/sector access timing
  - Disk I/O interrupt generation
  - Write protection verification

**Hard Disk Testing**:
- **File**: `steem/harddiskman.cpp`, `steem/hd_gemdos.cpp`
- **Test Areas**: ACSI/IDE emulation, partition handling, file system access

### 6. Input System Testing

**IKBD Testing**:
- **File**: `steem/ikbd.cpp`
- **Test Coverage**: Keyboard input, mouse movement, joystick emulation
- **Validation**: Input event timing, interrupt generation

**Serial Port Testing**:
- **File**: `steem/rs232.cpp`
- **Test Methods**: Serial communication protocol verification

### 7. Video System Testing

**Display Testing**:
- **File**: `steem/display.cpp`, `steem/draw.cpp`
- **Test Areas**: Screen rendering, color palette, resolution switching
- **Validation**: Visual output comparison with expected results

**Drawing Engine Testing**:
- **Files**: `steem/draw.cpp`, `steem/code/draw_c/` (C-based drawing routines)
- **Test Coverage**: Low-resolution, medium-resolution, high-resolution modes
- **Assembly Validation**: `steem/asm/asm_draw.asm`, `steem/asm/asm_osd_draw.asm`

## Testing Methodologies

### 1. Regression Testing

**Save State Comparison**:
- Load known-good save states
- Execute defined number of cycles
- Compare resulting state with expected state
- **Tools**: Memory snapshots, register dumps, memory comparison

**Demoscene Testing**:
- Run known demos with expected behavior
- Verify visual output, audio output, and timing
- **Test Cases**: Specific demo files known to stress particular hardware features

### 2. Automated Testing Framework

**Test Harness**:
- **Implementation**: Custom test framework integrated with emulator core
- **Execution**: Command-line driven test execution
- **Reporting**: Detailed logging of test results and failures

**Test Scripts**:
- **Location**: Various test scripts in build directories
- **Purpose**: Automated build and test execution
- **Example**: `windows-build/mingw/SSE/build_mingw_user.bat`

### 3. Manual Testing Procedures

**Visual Verification**:
- Compare emulator output with real hardware screenshots
- Verify color accuracy, resolution, and timing
- Test various display modes (ST Low, ST Medium, ST High, STE)

**Audio Verification**:
- Compare PSG output with real hardware recordings
- Test various sample rates and audio quality settings
- Verify sound synchronization with video

**Performance Testing**:
- Measure emulation speed accuracy
- Verify cycle timing against real hardware
- Test on various host hardware configurations

### 4. Debugging Tools

**Built-in Debugger**:
- **Features**:
  - Breakpoints (execution, memory access, I/O access)
  - Single-step execution
  - Register and memory inspection
  - Disassembly viewing
  - Call stack tracking

**Trace Logging**:
- **File**: `steem/debugger_trace.cpp`
- **Capabilities**:
  - Execution trace with cycle counts
  - Memory access logging
  - I/O register access tracking
  - Interrupt handling logging

**Memory Analysis**:
- **Tools**: Memory browser, memory comparison, breakpoint on access
- **Features**: Real-time memory monitoring, change detection, access pattern analysis

### 5. Platform-Specific Testing

**Windows Testing**:
- **Build Systems**: Visual Studio (2008, 2015, 2019), MinGW, Borland C++
- **Test Focus**: DirectX compatibility, Windows API integration
- **Tools**: Visual Studio debugger, Windows performance counters

**Linux/Unix Testing**:
- **Build System**: Makefile-based compilation
- **Test Focus**: X11 compatibility, POSIX compliance
- **Tools**: gdb, valgrind, strace

**Cross-Platform Validation**:
- Ensure consistent behavior across all supported platforms
- Verify endianness handling (little-endian host systems)
- Test 32-bit and 64-bit compatibility

## Test Cases and Validation

### Known Test Cases

1. **CPU Instruction Testing**:
   - All MC68000 instruction opcodes
   - Addressing mode validation
   - Exception handling (reset, interrupts, bus errors)

2. **Memory System Testing**:
   - ROM access patterns
   - RAM bank switching
   - Memory-mapped I/O verification

3. **Video System Testing**:
   - Screen memory access patterns
   - Palette register changes
   - Resolution switching timing

4. **Sound System Testing**:
   - PSG register writes
   - DMA sound transfer timing
   - Audio output quality

5. **Disk System Testing**:
   - Various disk image formats
   - Track/sector access timing
   - Disk I/O interrupt handling

### Validation Tools

**Hardware Documentation**:
- Atari ST technical documentation in `./atari-st-hardware/`
- Component datasheets and timing diagrams
- Known hardware behavior documentation

**Comparison Tools**:
- Memory state comparison utilities
- Execution trace comparison
- Visual output comparison (screenshot-based)

**Performance Measurement**:
- Cycle count verification
- Timing accuracy measurement
- Speed comparison with real hardware

## Debug Build Configuration

### Compilation Flags

```cpp
// Debug build activation
#define DEBUG_BUILD
#define SSE_DEBUG

// Debug features
#define DEBUGGER_INCLUDED  // Full debugger support
#define MR_STATIC_INCLUDED  // Memory browser
#define DEBUG_EMU_INCLUDED  // Emulation-level debugging
```

### Debug Build Components

**Additional Files in Debug Builds**:
- `steem/debugger.cpp` - Main debugger implementation
- `steem/debug_emu.cpp` - Emulation debugging
- `steem/mr_static.cpp` - Memory browser
- `steem/debugger_trace.cpp` - Trace logging
- `steem/d2.cpp` - Debug display
- `steem/mem_browser.cpp` - Memory browser interface
- `steem/dwin_edit.cpp` - Debug window editing
- `steem/historylist.cpp` - Execution history

### Debugging Interface

**Main Debugger Window**:
- Disassembly view
- Register display
- Memory dump
- Breakpoint list
- Call stack

**Memory Browser**:
- Hex dump view
- ASCII view
- Breakpoint setting
- Memory comparison

**Trace Window**:
- Execution trace
- Cycle count display
- Memory access log
- I/O access log

## Testing Workflow

### 1. Build Configuration

**Debug Build**:
```bash
# MinGW debug build
mingw32-make.exe -f makefile_MinGW.txt DEBUG=1

# Visual Studio debug configuration
# Use Debug configuration in VS2019 project
```

**Release Build with Debug Info**:
```bash
mingw32-make.exe -f makefile_MinGW.txt DEBUG=1 RELEASE=1 GDB=1
```

### 2. Test Execution

**Manual Testing**:
1. Launch Steem in debug mode
2. Load test ROM or disk image
3. Set breakpoints as needed
4. Execute and monitor behavior
5. Compare with expected results

**Automated Testing**:
1. Run test script with defined inputs
2. Execute emulator with test parameters
3. Capture output (memory state, screenshots, audio)
4. Compare with expected output
5. Generate test report

### 3. Result Analysis

**Pass/Fail Criteria**:
- Memory state matches expected state
- Visual output matches reference images
- Audio output matches reference recordings
- Timing matches hardware specifications
- No unexpected interrupts or exceptions

**Failure Analysis**:
- Identify first point of divergence
- Analyze execution trace leading to failure
- Check memory access patterns
- Verify register states at failure point
- Review hardware documentation for expected behavior

## Known Limitations

### Testing Coverage Gaps

1. **Limited Automated Testing**: Most testing is manual or semi-automated
2. **Platform-Specific Issues**: Some tests may not work on all platforms
3. **Performance Constraints**: Debug builds may be too slow for some tests
4. **Hardware Variations**: Testing focuses on STE model; ST/TT/Falcon variations may have less coverage

### Debugging Limitations

1. **No Reverse Execution**: Cannot step backward through execution
2. **Limited Multi-Core Debugging**: Debugger designed for single-threaded execution
3. **Memory Breakpoint Overhead**: Memory access breakpoints may significantly slow execution
4. **Platform-Specific Debug Features**: Some debug features only available on Windows

## Future Testing Enhancements

### Planned Improvements

1. **Automated Test Suite**: Comprehensive automated testing framework
2. **Test Case Database**: Library of known-good test cases
3. **Cross-Platform Testing**: Unified testing across all supported platforms
4. **Performance Testing**: Automated performance measurement and comparison
5. **Regression Test Suite**: Automated regression testing for each build

### Proposed Testing Features

1. **Test Script Language**: Domain-specific language for defining test cases
2. **Automated Screenshot Comparison**: Visual regression testing
3. **Audio Output Comparison**: Automated audio validation
4. **Timing Accuracy Measurement**: Precise cycle-count verification
5. **Hardware Behavior Database**: Reference database of known hardware behavior

## Conclusion

Steem SSE's testing approach combines comprehensive debugging capabilities with manual verification against hardware documentation. The debug build system provides extensive instrumentation for validating emulator accuracy, while the save state system enables regression testing. The testing methodology focuses on cycle-accurate verification of hardware behavior, ensuring that Steem SSE provides faithful emulation of the Atari ST hardware.
