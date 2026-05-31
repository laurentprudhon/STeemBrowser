# General Implementation Logic - Steem SSE Atari ST Emulator

## Overview

Steem SSE (Super Steem Engine) is a highly accurate Atari ST/STE/Mega STE emulator that implements the complete hardware architecture through a modular, component-based design. The emulator achieves cycle-accurate timing for most operations and supports advanced features like the STE enhancements.

## Core Architecture Principles

### 1. Component-Based Design

The emulator is structured around **independent hardware component classes** that each emulate a specific chip or subsystem:

```
+------------------+     +------------------+     +------------------+
|   TMC68000      |     |   TGlue         |     |   TShifter      |
|   (CPU)         |     |   (Glue Chip)   |     |   (Video)       |
+------------------+     +------------------+     +------------------+
         |                        |                        |
+------------------+     +------------------+     +------------------+
|   TMmu           |     |   TDma          |     |   TBlitter      |
|   (Memory Mgmt)  |     |   (DMA Ctrl)    |     |   (STE only)    |
+------------------+     +------------------+     +------------------+
         |                        |                        |
+------------------+     +------------------+     +------------------+
|   TMC68901      |     |   TWD1772        |     |   TYM2149       |
|   (MFP)         |     |   (FDC)         |     |   (PSG Sound)  |
+------------------+     +------------------+     +------------------+
```

### 2. Global State Management

#### Computer Class (`computer.cpp`)
- **Purpose**: Central instantiation and coordination of all hardware components
- **Key Function**: `ComputerRestore()` - Resets all components to initial state
- **Components Instantiated**:
  ```cpp
  TGlue Glue;           // Glue chip emulation
  TMC68000 Cpu;        // MC68000 CPU
  TMmu Mmu;            // Memory Management Unit
  TShifter Shifter;     // Video Shifter
  TMC68901 Mfp;        // Multi-Function Peripheral
  TDma Dma;            // DMA Controller
  TWD1772 Fdc;         // Floppy Disk Controller
  TYM2149 Psg;         // PSG Sound Chip
  TBlitter Blitter;    // Blitter (STE)
  TMC6850 acia[2];     // ACIA chips (RS232/MIDI)
  THD6301 Ikbd;        // Keyboard/Mouse controller
  ```

#### Global Variables
- `STMem`: Pointer to ST RAM (dynamically allocated based on configuration)
- `himem`: Limit of ST RAM address space
- `A_S_T`: Absolute system time counter (cycle-accurate)

### 3. Timing System

#### Counter Variables
- **Type**: `COUNTER_VAR` (typically `DWORD` or `DWORD64`)
- **Resolution**: 8MHz system clock cycles (1 cycle = 125ns)
- **Usage**: All timing-critical operations use this global time counter

#### Key Timing Functions
```cpp
// In emulator.cpp
void init_timings();    // Sets up all counters and clocks
COUNTER_VAR A_S_T;      // Absolute system time

// Timing macros
#define TICKS8 1          // 8MHz clock tick
#define CPU_BUS_IDLE(t) pBusIdle((t)*TICKS8)
```

#### Event Scheduling (Agenda System)
- **Purpose**: Schedule hardware events at specific cycle counts
- **Implementation**: `TAgenda` class with event queue
- **Usage**: Video events, interrupts, DMA transfers, etc.

```cpp
// Example: Scheduling a VBL interrupt
void agenda_vbl(int when) {
    Agenda.AddEvent(when, EVENT_VBL, vbl_handler);
}
```

### 4. Memory Access Architecture

#### Memory Mapping
```
ST Memory Layout (simplified):
+------------------+
| 0x000000-0x3FFFFF | ST RAM (up to 4MB on STE)
+------------------+
| 0xFC0000-0xFFFFFF | TOS ROM (192KB-384KB)
+------------------+
| 0xFF0000-0xFFFFFF | I/O registers (memory-mapped)
+------------------+
| 0xFF8000-0xFF825F | Glue/MMU registers
+------------------+
| 0xFF8600-0xFF860F | DMA registers
+------------------+
| 0xFF8800-0xFF880F | MFP registers
+------------------+
| 0xFF8900-0xFF891F | PSG registers
+------------------+
| 0xFFFA00-0xFFFA1F | MFP registers (extended)
+------------------+
```

#### Memory Access Functions
```cpp
// Peek/Poke functions (emulator.cpp)
BYTE& PEEK(DWORD ad);        // Read byte from ST memory
WORD& DPEEK(DWORD ad);      // Read word from ST memory
DWORD& LPEEK(DWORD ad);     // Read long from ST memory
void SafePoke(DWORD ad, BYTE val);    // Write byte safely
void SafeDPoke(DWORD ad, WORD val);  // Write word safely

// CPU-specific access
BYTE m68k_peek(MEM_ADDRESS ad);
WORD m68k_dpeek(MEM_ADDRESS ad);
void m68k_poke_abus(BYTE x);
```

#### Bank Switching (MMU)
- **Purpose**: Handle memory bank selection for different address ranges
- **Implementation**: `TMmu::BankLength()` and address decoding
- **Special Cases**: 
  - ST RAM banks (even/odd)
  - ROM mapping
  - Cartridge space
  - I/O register space

### 5. Execution Flow

#### Main Emulation Loop
```cpp
// In run.cpp
void stem_run() {
    while (runstate == RUNSTATE_RUNNING) {
        // Process CPU instructions
        m68kProcess();
        
        // Check for pending hardware events
        CheckAgenda();
        
        // Update hardware state
        UpdateHardware();
        
        // Handle interrupts
        CheckInterrupts();
    }
}
```

#### CPU Instruction Processing
```cpp
// In cpu.cpp
void m68kProcess() {
    TRY_M68K_EXCEPTION
    {
        // Fetch opcode
        IRC = m68k_fetch(pc);
        pc += 2;
        
        // Execute instruction
        (*m68k_call_table[IRD])();
        
        // Check for interrupts
        if (Cpu.IrqPending) {
            m68k_interrupt();
        }
    }
    CATCH_M68K_EXCEPTION
    {
        // Handle exceptions
        m68k_finish_exception(ExceptionObject.uaddress);
    }
    END_M68K_EXCEPTION
}
```

### 6. Interrupt System

#### Interrupt Sources
| Priority | Source | Handler |
|----------|--------|---------|
| 7 (highest) | Reset | `exception(EXCEPTION_RESET, ...)` |
| 6 | Bus Error | `exception(EXCEPTION_BUS_ERROR, ...)` |
| 5 | Address Error | `exception(EXCEPTION_ADDRESS_ERROR, ...)` |
| 4 | Illegal Instruction | `exception(EXCEPTION_ILLEGAL, ...)` |
| 3 | Division by Zero | `exception(EXCEPTION_DIVISION_BY_ZERO, ...)` |
| 2 | MFP Interrupts | `Mfp.UpdateNextIrq()` |
| 1 | VBL/Video | `Glue.Vbl()` |
| 0 | HBL/Horizontal Blank | `Glue.EndHBL()` |

#### MFP Interrupt Handling
```cpp
// In mfp.cpp
void TMC68901::UpdateNextIrq(COUNTER_VAR at_time) {
    // Check all interrupt sources
    for (int i = 0; i < N_MFP_IRQS; i++) {
        if (mfp_interrupt_enabled[i] && 
            (reg[MFPR_IPRA] & interrupt_i_bit[i])) {
            SetPending(i, at_time);
        }
    }
}
```

### 7. Video Rendering Pipeline

#### Scanline-Based Rendering
```cpp
// In shifter.cpp
void TShifter::Render(SHORT cycles_in, BYTE dispatcher) {
    // Determine current scanline
    int current_line = Glue.VCount;
    
    // Calculate visible area
    if (current_line >= de_start_line && current_line < de_end_line) {
        // Render visible pixels
        DrawScanline(cycles_in);
    } else {
        // Render border
        DrawBorder(cycles_in);
    }
}
```

#### Video Timing
- **HBL (Horizontal Blank)**: End of scanline, sync pulse generation
- **VBL (Vertical Blank)**: End of frame, interrupt generation
- **DE (Display Enable)**: Active video area
- **Video Counter**: Tracks position within frame

### 8. Sound Generation

#### PSG Emulation
```cpp
// In psg.cpp
void psg_write_buffer(BYTE reg, DWORD to_t) {
    // Update PSG registers
    psg_reg[reg] = value;
    
    // Generate sound samples
    if (reg < 14) {  // Tone/Noise/Envelope registers
        UpdateChannel(reg);
    }
    
    // Mix with other sound sources
    MixAudio();
}
```

#### DMA Sound (STE)
- **Implementation**: `Shifter::SoundPlay()`
- **Features**: 8-bit stereo PCM playback
- **Timing**: Synchronized with video scanlines

### 9. Floppy Disk Emulation

#### FDC State Machine
```cpp
// In fdc.cpp
void TWD1772::NewCommand(BYTE command) {
    // Decode command
    cr = command;
    
    // Start appropriate state machine
    switch (CommandType(command)) {
        case TYPE_I: TypeI_Command(); break;
        case TYPE_II: TypeII_Command(); break;
        case TYPE_III: TypeIII_Command(); break;
        case TYPE_IV: TypeIV_Command(); break;
    }
}
```

#### Disk Image Formats Supported
- `.ST` - Standard Atari ST disk images
- `.MSA` - MSA format
- `.STX` - STX format
- `.HFE` - HFE format (via PASTI)
- `.SCP` - SCP format

### 10. Configuration and Options

#### Machine Configuration
```cpp
// In parameters.h
enum EMachineType {
    MACHINE_ST,
    MACHINE_STE,
    MACHINE_MEGA_ST,
    MACHINE_MEGA_STE,
    MACHINE_TT,
    MACHINE_FALCON
};

enum EMemoryConfig {
    MEMCONF_128,    // 128KB
    MEMCONF_512,    // 512KB
    MEMCONF_2MB,    // 2MB
    MEMCONF_4MB,    // 4MB
    MEMCONF_14MB    // 14MB (MonSTer hack)
};
```

#### Feature Flags
```cpp
// In conditions.h
#define SSE_STE           // STE enhancements
#define SSE_MEGASTE       // Mega STE support
#define SSE_ACSI          // ACSI hard disk
#define SSE_DMA_SOUND     // STE DMA sound
#define SSE_BLITTER       // Blitter support
#define SSE_VME           // VME bus (Mega STE)
```

## Design Patterns Used

### 1. State Pattern
- **Usage**: Hardware components with multiple states (FDC, CPU, etc.)
- **Implementation**: State variables and state-specific methods

### 2. Observer Pattern
- **Usage**: Interrupt system, where components register for notifications
- **Implementation**: Callback functions and event flags

### 3. Singleton Pattern
- **Usage**: Global hardware components (one instance per system)
- **Implementation**: Global variables in `computer.cpp`

### 4. Factory Pattern
- **Usage**: Creating different machine configurations
- **Implementation**: Configuration-based object initialization

### 5. Template Method Pattern
- **Usage**: CPU instruction execution
- **Implementation**: Base instruction processing with specialized handlers

## Performance Optimizations

### 1. Direct Memory Access
- **Technique**: Direct pointer access to ST memory
- **Benefit**: Faster than function calls for frequent access

### 2. Function Pointer Tables
- **Technique**: `m68k_call_table[0xffff+1]` for CPU instructions
- **Benefit**: Fast dispatch without switch statements

### 3. Inline Functions
- **Technique**: Heavy use of `inline` for performance-critical code
- **Benefit**: Reduces function call overhead

### 4. Cycle Counting Macros
- **Technique**: `CPU_BUS_IDLE(t)`, `PREFETCH`, etc.
- **Benefit**: Consistent and efficient timing updates

### 5. Conditional Compilation
- **Technique**: `#ifdef` for optional features
- **Benefit**: Smaller, faster code when features are disabled

## Debugging Support

### 1. Debugger Integration
- **Features**: Breakpoints, memory inspection, register display
- **Implementation**: `debugger.cpp`, `debug_emu.cpp`

### 2. Trace Logging
- **Features**: Instruction tracing, memory access logging
- **Implementation**: `SSE_ENABLE_TRACE_LOG` flag

### 3. Statistics Collection
- **Features**: Performance metrics, cycle counts
- **Implementation**: `TStats` class in `emulator.h`

### 4. Snapshot Support
- **Features**: Save/load complete emulator state
- **Implementation**: `Restore()` methods in all components

## Accuracy Considerations

### Cycle-Accurate Timing
- **CPU**: Cycle-accurate instruction timing
- **Memory**: Accurate wait state modeling
- **Video**: Scanline-accurate rendering
- **Sound**: Sample-accurate PSG emulation

### Known Limitations
1. **DRAM Refresh**: Simplified refresh timing
2. **Bus Arbitration**: Approximate priority handling
3. **Analog Components**: Digital approximation of analog circuits
4. **Timing Jitter**: Some hardware jitter not fully emulated

### Accuracy Levels by Component
| Component | Accuracy | Notes |
|-----------|----------|-------|
| CPU | Very High | Cycle-accurate for most instructions |
| Glue | High | Accurate address decoding and timing |
| MMU | High | Accurate memory mapping |
| Shifter | Very High | Scanline-accurate video |
| MFP | High | Accurate timer and interrupt behavior |
| PSG | High | Sample-accurate sound generation |
| FDC | Medium | Functional, timing approximations |
| DMA | Medium | Functional, some timing simplifications |
| Blitter | Medium | Functional, cycle timing approximations |

## Conclusion

Steem SSE uses a **modular, component-based architecture** with **cycle-accurate timing** to achieve high accuracy in Atari ST emulation. Each hardware component is implemented as a separate class with its own state, timing, and behavior. The emulator uses **event-driven execution** with a global timing system to coordinate all components accurately.

The design prioritizes:
1. **Accuracy** - Cycle-accurate timing where possible
2. **Modularity** - Separate components for each hardware chip
3. **Performance** - Optimized code paths for critical operations
4. **Extensibility** - Easy to add new features or improve accuracy
5. **Debuggability** - Comprehensive debugging support