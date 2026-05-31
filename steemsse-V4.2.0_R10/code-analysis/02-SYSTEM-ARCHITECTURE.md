# System Architecture - Steem SSE Atari ST Emulator

## Overview

Steem SSE implements the complete Atari ST hardware architecture through a hierarchical component system that closely mirrors the actual hardware organization. This document describes the overall system structure, component interactions, and data flow.

## Hardware Architecture Mapping

### Atari ST Block Diagram vs Steem Implementation

```
ATARI ST HARDWARE ARCHITECTURE:
┌─────────────────────────────────────────────────────────────┐
│                         MC68000 CPU (8MHz)                       │
│                    ┌────────────┬────────────┐                 │
│                    │ Address Bus │ Data Bus    │ Control Signals │
└───────────────────┼────────────┼────────────┼─────────────────┘
                        │            │            │
┌───────────────────▼────────────▼────────────▼───────────────┐
│                    CUSTOM ASICs (4 chips)                       │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ │
│  │   GLUE      │ │    MMU      │ │    DMA      │ │  SHIFTER    │ │
│  │ C029144     │ │ C028300     │ │ C029128     │ │ C028787     │ │
│  │ Address     │ │ Memory      │ │ Floppy/HDC  │ │ Video DAC   │ │
│  │ Decoding    │ │ Controller  │ │ DMA         │ │ Sync Gen    │ │
│  │ DRAM Refresh│ │ Scroll Regs │ │ Arbitration │ │ Pixel Clock │ │
│  └─────────────┘ └─────────────┘ └─────────────┘ └─────────────┘ │
└─────────────────────────────────────────────────────────────┘
                        │            │            │
┌───────────────────▼────────────▼────────────▼───────────────┐
│                    STANDARD CHIPs                                  │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ │
│  │  MC68901    │ │   YM2149    │ │  WD1772     │ │  MC6850     │ │
│  │  MFP        │ │   PSG       │ │  FDC        │ │  ACIA       │ │
│  │ Timers     │ │ Sound Chips │ │ Floppy Ctrl │ │ Serial     │ │
│  │ Interrupts │ │ 3 channels   │ │ DMA        │ │ RS232/MIDI │ │
│  └─────────────┘ └─────────────┘ └─────────────┘ └─────────────┘ │
└─────────────────────────────────────────────────────────────┘
                        │            │            │
┌───────────────────▼────────────▼────────────▼───────────────┐
│                    PERIPHERALS                                     │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ │
│  │  HD6301     │ │  DRAM       │ │  ROM        │ │  I/O Ports  │ │
│  │  IKBD       │ │ 41256/414616│ │ 27256      │ │ Monitor    │ │
│  │ Keyboard    │ │ (512K-4M)   │ │ (192K-384K)│ │ Floppy     │ │
│  │ Mouse       │ │             │ │            │ │ RS232      │ │
│  └─────────────┘ └─────────────┘ └─────────────┘ └─────────────┘ │
└─────────────────────────────────────────────────────────────┘

STEEM SSE IMPLEMENTATION:
┌─────────────────────────────────────────────────────────────┐
│                    TMC68000 Cpu                                   │
│  ┌─────────────────────────────────────────────────────────┐│
│  │ • pc, sr, r[16] registers                                  ││
│  │ • m68kProcess() - main instruction loop                   ││
│  │ • exception() - handles all CPU exceptions               ││
│  └─────────────────────────────────────────────────────────┘│
└───────────────────┬────────────┬────────────┬─────────────────┘
                        │            │            │
┌───────────────────▼────────────▼────────────▼───────────────┐
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ │
│  │  TGlue      │ │   TMmu      │ │   TDma      │ │  TShifter   │ │
│  │ glue.cpp    │ │ mmu.cpp     │ │ dma.cpp     │ │ shifter.cpp │ │
│  │ • Address   │ │ • Memory    │ │ • Floppy    │ │ • Video     │ │
│  │   decoding  │ │   mapping   │ │ • DMA       │ │ • Rendering │ │
│  │ • Timing    │ │ • Scroll    │ │ • Transfer  │ │ • Palette   │ │
│  └─────────────┘ └─────────────┘ └─────────────┘ └─────────────┘ │
└───────────────────┬────────────┬────────────┬─────────────────┘
                        │            │            │
┌───────────────────▼────────────▼────────────▼───────────────┐
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ │
│  │ TMC68901    │ │  TYM2149    │ │  TWD1772    │ │  TMC6850    │ │
│  │ mfp.cpp     │ │ psg.cpp     │ │ fdc.cpp     │ │ acia.cpp    │ │
│  │ • Timers    │ │ • Sound     │ │ • Floppy    │ │ • Serial    │ │
│  │ • Interrupts│ │ • Channels  │ │ • Commands  │ │ • RS232     │ │
│  │ • GPIP      │ │ • Mixing    │ │ • DMA       │ │ • MIDI      │ │
│  └─────────────┘ └─────────────┘ └─────────────┘ └─────────────┘ │
└───────────────────┬────────────┬────────────┬─────────────────┘
                        │            │            │
┌───────────────────▼────────────▼────────────▼───────────────┐
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ │
│  │  THD6301    │ │   Memory    │ │   TRom      │ │  TFloppy    │ │
│  │ ikbd.cpp    │ │ STMem[]     │ │ Tos         │ │ floppy.cpp  │ │
│  │ • Keyboard  │ │ • RAM       │ │ • TOS ROM   │ │ • Drives    │ │
│  │ • Mouse     │ │ • Allocation│ │ • Mapping   │ │ • Images    │ │
│  │ • Joystick  │ │             │ │            │ │ • Formats   │ │
│  └─────────────┘ └─────────────┘ └─────────────┘ └─────────────┘ │
└─────────────────────────────────────────────────────────────┘
```

## Component Hierarchy

### Level 1: Core System
```
┌─────────────────────────────────────────────────────────────┐
│                        CORE SYSTEM                              │
├─────────────────────────────────────────────────────────────┤
│  • computer.cpp - Component instantiation and coordination     │
│  • emulator.cpp - Core emulation functions and timing         │
│  • run.cpp - Main execution loop                               │
│  • reset.cpp - System reset handling                           │
└─────────────────────────────────────────────────────────────┘
```

### Level 2: Hardware Components
```
┌─────────────────────────────────────────────────────────────┐
│                      HARDWARE COMPONENTS                        │
├─────────────────────────────────────────────────────────────┤
│  CUSTOM ASICs:                                                   │
│    • Glue (glue.cpp/h) - Address decoding, timing              │
│    • MMU (mmu.cpp/h) - Memory management, scroll registers      │
│    • DMA (dma.cpp/h) - Direct memory access                     │
│    • Shifter (shifter.cpp/h) - Video generation                 │
│    • Blitter (blitter.cpp/h) - Bit-block transfer (STE)        │
│                                                                 │
│  STANDARD CHIPs:                                                 │
│    • CPU (cpu.cpp/h, cpu_op.cpp) - MC68000 emulation            │
│    • MFP (mfp.cpp/h) - MC68901 Multi-Function Peripheral         │
│    • PSG (psg.cpp/h) - YM2149 Programmable Sound Generator        │
│    • FDC (fdc.cpp/h) - WD1772 Floppy Disk Controller             │
│    • ACIA (acia.cpp/h) - MC6850 Asynchronous Communications     │
│    • RTC (rtc.cpp/h) - MC146818A Real-Time Clock                 │
│    • IKBD (ikbd.cpp/h) - HD6301 Keyboard/Mouse Controller        │
│                                                                 │
│  STE ENHANCEMENTS:                                               │
│    • GST MCU - Integrated Glue/MMU/DMA/Blitter (STE)          │
│    • GST Shifter - Enhanced video with Super Hi-Res (STE)      │
│    • DMA Sound - 8-bit stereo PCM (STE)                         │
└─────────────────────────────────────────────────────────────┘
```

### Level 3: Support Systems
```
┌─────────────────────────────────────────────────────────────┐
│                       SUPPORT SYSTEMS                           │
├─────────────────────────────────────────────────────────────┤
│  • Memory System (mmu.cpp, mem_*.cpp)                          │
│  • Video System (display.cpp, draw.cpp, palette.cpp)           │
│  • Sound System (sound.cpp)                                   │
│  • Input System (stjoy.cpp, stports.cpp)                       │
│  • Disk System (floppy_*.cpp, disk_*.cpp)                      │
│  • Hard Disk System (acsi.cpp, hd_*.cpp)                       │
│  • Debug System (debug*.cpp)                                  │
│  • GUI System (gui.cpp, stemdialogs.cpp, etc.)                 │
└─────────────────────────────────────────────────────────────┘
```

## Data Flow Architecture

### 1. CPU Execution Flow

```
┌─────────────┐     ┌─────────────┐     ┌─────────────┐
│  m68kProcess │────▶│ m68k_fetch  │────▶│ Instruction  │
│   (cpu.cpp)  │     │  (cpu.cpp)   │     │  Execution   │
└─────────────┘     └─────────────┘     └─────────────┘
         │                    │                    │
         ▼                    ▼                    ▼
┌─────────────────────────────────────────────────────────────┐
│                    MEMORY ACCESS                               │
│  ┌─────────────┐     ┌─────────────┐     ┌─────────────┐    │
│  │ m68k_peek   │◀────│ m68k_poke   │◀────│ Address      │    │
│  │ m68k_dpeek  │     │ m68k_dpoke  │     │ Decoding    │    │
│  └─────────────┘     └─────────────┘     └─────────────┘    │
└─────────────────────────────────────────────────────────────┘
         │                    │                    │
         ▼                    ▼                    ▼
┌─────────────┐     ┌─────────────┐     ┌─────────────┐
│  Registers   │     │   ST RAM    │     │ I/O Space    │
│  (D0-D7,     │     │  (STMem[])  │     │ (Glue, MFP,  │
│   A0-A7)     │     │             │     │  PSG, etc.)   │
└─────────────┘     └─────────────┘     └─────────────┘
```

### 2. Video Rendering Flow

```
┌─────────────────────────────────────────────────────────────┐
│                    VIDEO RENDERING PIPELINE                     │
├─────────────────────────────────────────────────────────────┤
│                                                                  │
│  Glue.Vbl()                    Shifter.Render()               │
│      │                              │                        │
│      ▼                              ▼                        │
│  ┌─────────────┐          ┌─────────────────┐                │
│  │ VBL         │          │ Scanline Render  │                │
│  │ Interrupt   │          │ (per line)       │                │
│  └──────┬──────┘          └────────┬─────────┘                │
│         │                         │                           │
│         ▼                         ▼                           │
│  ┌─────────────┐          ┌─────────────────┐                │
│  │ Frame       │◀─────────│ Draw Scanline    │                │
│  │ Complete    │          │ (visible area)   │                │
│  └─────────────┘          └─────────────────┘                │
│                                    │                           │
│                                    ▼                           │
│                          ┌─────────────────┐                │
│                          │ Display Output   │                │
│                          │ (to screen)      │                │
│                          └─────────────────┘                │
│                                                                  │
└─────────────────────────────────────────────────────────────┘
```

### 3. Interrupt Handling Flow

```
┌─────────────────────────────────────────────────────────────┐
│                    INTERRUPT HANDLING                          │
├─────────────────────────────────────────────────────────────┤
│                                                                  │
│  Hardware Event                MFP Processing                │
│      │                         │                              │
│      ▼                         ▼                              │
│  ┌─────────────┐          ┌─────────────────┐                │
│  │ Event       │          │ TMC68901::       │                │
│  │ Triggered   │          │ UpdateNextIrq()  │                │
│  └──────┬──────┘          └────────┬─────────┘                │
│         │                         │                           │
│         ▼                         ▼                           │
│  ┌─────────────┐          ┌─────────────────┐                │
│  │ SetPending  │◀─────────│ Check Interrupt  │                │
│  │ (IRQ flag)  │          │ Sources         │                │
│  └──────┬──────┘          └─────────────────┘                │
│         │                                                    │
│         ▼                                                    │
│  ┌─────────────────────────────────────────────────────────┐│
│  │                    CPU INTERFACE                         ││
│  │  ┌─────────────┐     ┌─────────────┐     ┌─────────────┐ ││
│  │  │ Check IPL   │◀────│ Update      │◀────│ m68k_        │ ││
│  │  │ (priority)  │     │ IPL         │     │ interrupt()  │ ││
│  │  └─────────────┘     └─────────────┘     └─────────────┘ ││
│  └─────────────────────────────────────────────────────────┘
│         │                                                    │
│         ▼                                                    │
│  ┌─────────────┐                                           │
│  │ Exception   │                                           │
│  │ Handler     │                                           │
│  └─────────────┘                                           │
│                                                                  │
└─────────────────────────────────────────────────────────────┘
```

### 4. Memory Access Flow

```
┌─────────────────────────────────────────────────────────────┐
│                    MEMORY ACCESS ARCHITECTURE                   │
├─────────────────────────────────────────────────────────────┤
│                                                                  │
│  CPU Request                   MMU Decoding                    │
│      │                         │                              │
│      ▼                         ▼                              │
│  ┌─────────────┐          ┌─────────────────┐                │
│  │ Address     │─────────▶│ TMmu::          │                │
│  │ (24-bit)    │          │ BankLength()    │                │
│  └─────────────┘          │ Decode[]        │                │
│                            └────────┬─────────┘                │
│                                     │                           │
│                                     ▼                           │
│                          ┌─────────────────┐                │
│                          │ Memory Region    │                │
│                          │ Identification   │                │
│                          └────────┬─────────┘                │
│                                   │                           │
│          ┌────────────────────────┼────────────────────────┐  │
│          ▼                            ▼                            ▼  │
│  ┌─────────────┐          ┌─────────────┐          ┌─────────────┐ │
│  │ ST RAM      │          │ TOS ROM     │          │ I/O Space   │ │
│  │ (0x000000-  │          │ (0xFC0000-  │          │ (0xFF8000-  │ │
│  │  0x3FFFFF)  │          │  0xFFFFFF)   │          │  0xFFFFFF)  │ │
│  └─────────────┘          └─────────────┘          └─────────────┘ │
│          │                        │                        │        │
│          ▼                        ▼                        ▼        │
│  ┌─────────────────────────────────────────────────────────┐│
│  │                    DATA RETURN                             ││
│  └─────────────────────────────────────────────────────────┘
│                                                                  │
└─────────────────────────────────────────────────────────────┘
```

## Component Communication

### 1. Direct Method Calls
- **Usage**: Components calling methods on other components
- **Example**: `Glue.Vbl()` calls `Shifter.IncScanline()`
- **Pros**: Simple, direct, type-safe
- **Cons**: Creates dependencies between components

### 2. Global Variables
- **Usage**: Shared state through global variables
- **Example**: `A_S_T` (absolute system time)
- **Pros**: Fast access, no function call overhead
- **Cons**: Can lead to spaghetti code, harder to debug

### 3. Event/Callback System
- **Usage**: Agenda system for scheduling events
- **Example**: `Agenda.AddEvent(time, event_type, handler)`
- **Pros**: Decouples components, flexible timing
- **Cons**: Slightly more overhead, harder to trace

### 4. Memory-Mapped I/O
- **Usage**: Components expose registers in memory space
- **Example**: MFP registers at 0xFFFA00-0xFFFA1F
- **Pros**: Accurate hardware emulation, easy CPU access
- **Cons**: Requires address decoding logic

### 5. Interrupt System
- **Usage**: Components signal interrupts to CPU
- **Example**: `Mfp.GetInterrupt(irq, when)`
- **Pros**: Accurate priority handling, hardware-like behavior
- **Cons**: Complex to implement correctly

## Class Relationships

### Inheritance Hierarchy
```
┌─────────────────────────────────────────────────────────────┐
│                    CLASS INHERITANCE                            │
├─────────────────────────────────────────────────────────────┤
│                                                                  │
│  No significant inheritance - components are mostly standalone  │
│                                                                  │
│  ┌─────────────────────────────────────────────────────────┐│
│  │  Base Classes:                                             ││
│  │    • None - all components inherit from no base class    ││
│  │    • Some use structs for data grouping                   ││
│  └─────────────────────────────────────────────────────────┘│
│                                                                  │
│  ┌─────────────────────────────────────────────────────────┐│
│  │  Composition:                                             ││
│  │    • TGlue contains TScanline                             ││
│  │    • TWD1772 contains TWD1772IDField, TWD1772MFM, etc.     ││
│  │    • TMC68901 contains TMC68901IrqInfo                      ││
│  └─────────────────────────────────────────────────────────┘│
│                                                                  │
└─────────────────────────────────────────────────────────────┘
```

### Component Dependencies

```
┌─────────────────────────────────────────────────────────────┐
│                    COMPONENT DEPENDENCIES                        │
├─────────────────────────────────────────────────────────────┤
│                                                                  │
│  CORE COMPONENTS (minimal dependencies):                      │
│    • TMC68000 - Only depends on memory system                   │
│    • TMmu - Depends on TGlue for address decoding              │
│    • TGlue - Depends on TShifter for video timing               │
│    • TShifter - Depends on TMmu for memory access              │
│                                                                  │
│  PERIPHERAL COMPONENTS (depend on core):                      │
│    • TMC68901 (MFP) - Depends on CPU for interrupts             │
│    • TYM2149 (PSG) - Depends on MFP for timing                  │
│    • TWD1772 (FDC) - Depends on DMA and MFP                     │
│    • THD6301 (IKBD) - Depends on MFP for interrupts             │
│    • TBlitter - Depends on MMU and Glue for bus access         │
│                                                                  │
│  SUPPORT SYSTEMS (depend on components):                       │
│    • Video System - Depends on Shifter, Glue, MMU              │
│    • Sound System - Depends on PSG, Shifter (DMA sound)        │
│    • Disk System - Depends on FDC, DMA, MFP                    │
│    • Input System - Depends on IKBD, MFP                        │
│                                                                  │
└─────────────────────────────────────────────────────────────┘
```

## Initialization Sequence

### System Startup
```
1. main() in Steem.cpp
   ├─▶ Parse command line arguments
   ├─▶ Initialize configuration
   └─▶ Call stem_init()

2. stem_init() in emulator.cpp
   ├─▶ Allocate memory (STMem, Rom_End, etc.)
   ├─▶ Initialize global variables
   ├─▶ SetTimingFunctions()
   └─▶ ComputerRestore()

3. ComputerRestore() in computer.cpp
   ├─▶ Mfp.Restore()
   ├─▶ Glue.Restore()
   ├─▶ Mmu.Restore()
   ├─▶ Shifter.Restore()
   ├─▶ Psg.Restore()
   ├─▶ for each acia: acia[i].Restore()
   ├─▶ for each floppy drive: FloppyDrive[i].Restore()
   └─▶ ... (all other components)

4. Component Restore() methods
   ├─▶ Reset internal state
   ├─▶ Initialize registers to power-on values
   └─▶ Set up initial configuration
```

### Machine Reset
```
1. ComputerReset() in computer.cpp
   ├─▶ Cpu.Reset(Cold)
   ├─▶ Mfp.Reset(Cold)
   ├─▶ Glue.Reset(Cold)
   ├─▶ Mmu.Reset(Cold)
   ├─▶ Shifter.Reset(Cold)
   ├─▶ Psg.Reset()
   └─▶ ... (all other components)

2. Component Reset() methods
   ├─▶ Cold reset: Initialize to power-on state
   └─▶ Warm reset: Preserve some state (e.g., memory)
```

## Execution Model

### 1. Single-Step Execution
- **Purpose**: Debugging, precise timing control
- **Implementation**: Process one CPU instruction at a time
- **Usage**: Debugger, trace logging

### 2. Continuous Execution
- **Purpose**: Normal emulation operation
- **Implementation**: Loop processing CPU instructions with event checks
- **Usage**: Main emulation mode

### 3. Event-Driven Execution
- **Purpose**: Accurate timing of hardware events
- **Implementation**: Agenda system schedules events at specific cycle counts
- **Usage**: Video events, interrupts, DMA transfers

### Execution Loop (simplified)
```cpp
// In run.cpp
void stem_run() {
    while (runstate == RUNSTATE_RUNNING) {
        // Check if we need to stop
        if (stop_emu_requested) {
            runstate = RUNSTATE_STOPPING;
            break;
        }
        
        // Process CPU instructions until next event
        COUNTER_VAR next_event = Agenda.NextEventTime();
        while (A_S_T < next_event && runstate == RUNSTATE_RUNNING) {
            m68kProcess();
        }
        
        // Process pending events
        Agenda.ProcessEvents();
        
        // Update hardware state
        UpdateHardware();
        
        // Handle display updates
        if (video_update_pending) {
            UpdateDisplay();
        }
        
        // Handle audio mixing
        if (audio_update_pending) {
            UpdateAudio();
        }
    }
}
```

## Memory Management

### Memory Allocation
```cpp
// In emulator.cpp
BYTE *STMem = nullptr;      // ST RAM
BYTE *Rom_End = nullptr;    // TOS ROM
BYTE *Cart_End = nullptr;   // Cartridge ROM

void AllocateMemory() {
    // Allocate ST RAM based on configuration
    mem_len = GetMemorySize();
    STMem = new BYTE[mem_len + MEM_EXTRA_BYTES];
    
    // Set up memory pointers
    Mem_End = STMem + mem_len;
    Mem_End_minus_1 = Mem_End - 1;
    Mem_End_minus_2 = Mem_End - 2;
    Mem_End_minus_4 = Mem_End - 4;
    
    // Allocate ROM space
    tos_len = GetRomSize();
    Rom_End = new BYTE[tos_len];
}
```

### Memory Access Functions
```cpp
// Direct access (fastest)
BYTE& PEEK(DWORD ad) { return *LPBYTE(Mem_End_minus_1 - ad); }
WORD& DPEEK(DWORD ad) { return *LPWORD(Mem_End_minus_2 - ad); }

// Safe access (with bounds checking)
void SafePoke(MEM_ADDRESS ad, BYTE val) {
    if (ad < himem) PEEK(ad) = val;
}

// CPU-specific access (with timing)
BYTE m68k_peek(MEM_ADDRESS ad) {
    // Check for I/O space
    if (ad >= 0xFF8000) {
        return io_peek(ad);
    }
    // Check for ROM
    if (ad >= 0xFC0000) {
        return ROM_PEEK(ad);
    }
    // ST RAM
    return PEEK(ad);
}
```

## Configuration Management

### Machine Types
```cpp
// In parameters.h
enum EMachineType {
    MACHINE_ST,        // Original ST (520ST, 1040ST)
    MACHINE_STE,       // STE (520STE, 1040STE)
    MACHINE_MEGA_ST,   // Mega ST
    MACHINE_MEGA_STE,  // Mega STE
    MACHINE_TT,        // TT030
    MACHINE_FALCON     // Falcon030
};
```

### Memory Configurations
```cpp
// In mmu.h
enum EMmu {
    MEMCONF_128,    // 128KB (520ST)
    MEMCONF_512,    // 512KB (520ST)
    MEMCONF_2MB,    // 2MB (1040ST, STE)
    MEMCONF_4MB,    // 4MB (1040STE, Mega STE)
    MEMCONF_14MB    // 14MB (MonSTer hack)
};
```

### Feature Configuration
```cpp
// In conditions.h - Compile-time features
#define SSE_STE           // Enable STE enhancements
#define SSE_MEGASTE       // Enable Mega STE features
#define SSE_ACSI          // Enable ACSI hard disk
#define SSE_DMA_SOUND     // Enable STE DMA sound
#define SSE_BLITTER       // Enable Blitter
#define SSE_VME           // Enable VME bus

// In options.h - Runtime configuration
struct TOptions {
    EMachineType MachineType;
    EMemoryConfig MemoryConfig;
    bool EnableBlitter;
    bool EnableDmaSound;
    bool EnableCycleAccurateTiming;
    // ... many more options
};
```

## Conclusion

Steem SSE's system architecture is designed to **faithfully reproduce the Atari ST hardware** while maintaining **modularity, performance, and accuracy**. The component-based design allows each hardware chip to be emulated independently, while the global timing system ensures accurate synchronization between all components.

Key architectural features:
1. **Component Isolation** - Each hardware chip has its own class
2. **Accurate Timing** - Global cycle counter with event scheduling
3. **Memory-Mapped I/O** - Authentic register access patterns
4. **Interrupt-Driven** - Hardware events trigger CPU interrupts
5. **Configurable** - Supports multiple machine types and configurations
6. **Extensible** - Easy to add new features or improve accuracy

This architecture allows Steem SSE to achieve **high accuracy** in emulating the complex interactions between the Atari ST's various hardware components, from the CPU and custom ASICs to the standard peripheral chips.