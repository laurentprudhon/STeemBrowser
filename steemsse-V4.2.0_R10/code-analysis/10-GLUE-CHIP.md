# Glue Chip Implementation - Steem SSE

## Overview

The Glue chip (C029144 / C300866 / C301578) is one of the four custom ASICs in the Atari ST that handles address decoding, DRAM refresh, clock distribution, sync generation, and various glue logic functions. In the STE, the Glue functionality is integrated into the GST MCU (C302183).

This document describes how the Glue chip is emulated in Steem SSE, including its architecture, timing generation, video synchronization, and address decoding.

## Hardware Background

### Glue Chip Functions

The Glue chip performs several critical functions in the Atari ST:

1. **Address Decoding**: Decodes the 24-bit address bus to generate chip select signals
2. **DRAM Refresh**: Generates RAS/CAS signals for DRAM refresh
3. **Clock Distribution**: Distributes system clocks to other chips
4. **Video Timing**: Generates horizontal and vertical sync signals
5. **Bus Arbitration**: Manages bus access between CPU, DMA, and other masters
6. **Interrupt Control**: Generates video-related interrupts (VBL, HBL)
7. **Memory Bank Selection**: Controls ST RAM bank selection

### Key Specifications
- **Package**: PLCC68 (68-pin Plastic Leaded Chip Carrier)
- **Clock Input**: 8 MHz (Y1)
- **Clock Outputs**: Various derived clocks for other chips
- **Address Bus**: 24-bit input (A0-A23)
- **Control Signals**: Multiple outputs for chip selects, sync, etc.

## Implementation Files

### Primary Files
| File | Purpose | Lines | Key Functions |
|------|---------|-------|----------------|
| `glue.h` | Glue declarations and data structures | ~145 | `TGlue`, `TScanline`, `TGlueStatus` |
| `glue.cpp` | Glue implementation | ~2000+ | `GetNextVideoEvent()`, `Vbl()`, `EndHBL()` |

### Related Files
| File | Purpose |
|------|---------|
| `emulator.cpp` | Core timing and event scheduling |
| `computer.cpp` | Component instantiation |
| `shifter.cpp` | Video rendering (tightly coupled) |
| `mmu.cpp` | Memory management (address decoding) |
| `display.cpp` | Video output |

## Class Structure

### TGlue Class (`glue.h`)

```cpp
struct TGlue {
    // ENUM
    enum EGlue {
        // Frequency indices
        FREQ_IDX_50, FREQ_IDX_60, FREQ_IDX_71, NFREQS,
        
        // Horizontal blank states
        HBLANK_OFF, DE_OFF, HBLANK_ON, HSYNC_ON, HSYNC_ON1, 
        HSYNC_ON2, HSYNC_OFF, HSYNC_OFF2,
        
        // Video counter events
        RELOAD_VC, ENABLE_VBI, VERT_OVSCN_LIMIT,
        DE_ON, DE_ON_HSCROLL,
        
        // Scanline timing limits
        LINE_START_LIMIT, LINE_START_LIMIT_PLUS2, 
        LINE_START_LIMIT_PLUS26, LINE_START_LIMIT_MINUS12,
        LINE_STOP_LIMIT, LINE_STOP_LIMIT_MINUS2,
        LINE_PLUS_44_R, LINE_PLUS_20A, LINE_PLUS_20B, 
        LINE_PLUS_20C, LINE_PLUS_20D, LINE_PLUS_26A, 
        LINE_PLUS_26B, LINE_PLUS_26C,
        
        // Medium resolution timing
        MEDRES_OA, MEDRES_OB, MEDRES_OC,
        
        // Stabilization states
        DESTAB_A, DESTAB_B, DESTAB0, STAB_A, STAB_B, STAB_C, STAB_D,
        RENDER_CYCLE, NTIMINGS,
        
        // Bus error types
        BERR, STRAM_OR_ROM, ROM, STRAM, DEV, CART, ALTRAM, 
        ROM1, CART2, ROM_CHECK, STRAM_CHECK, MMU_CONFUSED, 
        STRAM_C2, STRAM_CHECK_C2, CART3,
        
        // Sync flags
        SYNCEXT=BIT_0, SYNCPAL=BIT_1
    };
    
    // FUNCTIONS
    TGlue();
    void AdaptScanlineValues(SHORT CyclesIn);
    void CheckSideOverscan();
    void CheckVerticalOverscan();
    void EndHBL();
    bool FetchingLine();
    void GetNextVideoEvent();
    void IncScanline();
    void Reset(bool Cold);
    void Restore();
    void SetShiftMode(BYTE NewRes);
    void SetSyncMode(BYTE NewSync);
    void Update();
    void Vbl();
    
    // Video trick analysis (for demos)
    void AddFreqChange(BYTE f);
    void AddShiftModeChange(BYTE mode);
    int FreqChangeAtCycle(int cycle);
    int FreqAtCycle(int cycle);
    int ShiftModeAtCycle(int cycle);
    int ShiftModeChangeAtCycle(int cycle);
    short NextShiftModeChange(int cycle, int value=-1);
    short NextChangeToHi(int cycle);
    short NextChangeToLo(int cycle);
    short PreviousChangeToHi(int cycle);
    short PreviousChangeToLo(int cycle);
    short PreviousShiftModeChange(int cycle);
    short CycleOfLastChangeToShiftMode(int value);
    
    // DATA
    COUNTER_VAR hbl_pending_time, vbl_pending_time;
    TEvent video_event;
    int TrickExecuted;
    TGlueStatus m_Status;
    BYTE ShiftMode;              // Current shift mode (2 bits)
    BYTE SyncMode;               // Current sync mode (2 bits)
    BYTE Freq[NFREQS];           // Frequency settings
    BYTE ScanlineCyclesTiming;
    BYTE VideoFreq, PreviousVideoFreq;
    bool de_v_on;                // DE video on flag
    bool vsync;                  // VSYNC state
    bool gamecart;               // Game cartridge flag
    bool hbl_pending, vbl_pending; // Pending interrupt flags
    bool hscroll;                // Horizontal scroll flag
    WORD DE_cycles[NFREQS];      // DE cycle counts for each frequency
    short nLines;                // Number of lines per frame
    short de_start_line, de_end_line, VCount; // Video counter lines
    TScanline PreviousScanline, CurrentScanline, NextScanline;
    short ScanlineTiming[NTIMINGS][NFREQS];
    MEM_ADDRESS cartbase, cartend; // Cartridge memory range
    BYTE Decode[0xFF+1];          // Address decoding table
    bool bFetchingLine;           // Flag for line fetching
    int nFrameCycles;            // Cycles per frame
};
```

### Supporting Structures

```cpp
struct TScanline {
    DWORD Tricks;     // Bitmask of tricks for this scanline
    SHORT StartCycle; // Start cycle (e.g., 64 for 50Hz)
    SHORT EndCycle;   // End cycle (e.g., 384)
    int Cycles;       // Total cycles (e.g., 512)
    BYTE Bytes;       // Bytes per scanline (e.g., 160)
};

struct TGlueStatus {
    bool scanline_done;
    bool vc_reload_done;
    bool vbl_done, vbi_done, hbi_done;
    bool timerb_start, timerb_end;
    BYTE stop_emu;     // Control flag for emulation stop
};
```

## Core Functionality

### 1. Video Timing Generation

The Glue chip is primarily responsible for generating the video timing signals that drive the Shifter and the rest of the video system.

#### Video Event Scheduling

```cpp
void TGlue::GetNextVideoEvent() {
    // This is the core function for video timing
    // It determines when the next video-related event should occur
    
    if (m_Status.vbl_done) {
        // VBL already processed, schedule next frame
        video_event.time = hbl_pending_time + nFrameCycles;
        video_event.type = EVENT_VBL;
        m_Status.vbl_done = false;
    } else if (m_Status.hbl_done) {
        // HBL already processed, schedule next scanline
        video_event.time = hbl_pending_time + CurrentScanline.Cycles;
        video_event.type = EVENT_HBL;
        m_Status.hbl_done = false;
    } else {
        // Schedule based on current position in scanline
        // ... complex timing calculations
    }
}
```

#### Scanline Processing

```cpp
void TGlue::IncScanline() {
    // Move to next scanline
    PreviousScanline = CurrentScanline;
    CurrentScanline = NextScanline;
    
    // Calculate next scanline parameters
    AdaptScanlineValues(CurrentScanline.Cycles);
    
    // Update video counter
    VCount++;
    
    // Check for VBL
    if (VCount >= nLines) {
        Vbl();
    }
}

void TGlue::AdaptScanlineValues(SHORT CyclesIn) {
    // Set sync and shift mode for new scanline
    // This updates the timing parameters based on current video mode
    
    // Update DE (Display Enable) timing
    if (de_v_on) {
        // DE is active
        CurrentScanline.StartCycle = DE_cycles[VideoFreq];
        CurrentScanline.EndCycle = CurrentScanline.Cycles - DE_cycles[VideoFreq];
    } else {
        // DE is inactive (border)
        CurrentScanline.StartCycle = 0;
        CurrentScanline.EndCycle = CurrentScanline.Cycles;
    }
    
    // Update sync mode
    SetSyncMode(SyncMode);
}
```

### 2. Vertical Blank (VBL) Handling

```cpp
void TGlue::Vbl() {
    // End of frame processing
    
    // Reset video counter
    VCount = 0;
    
    // Trigger VBL interrupt
    Mfp.GetInterrupt(MFP_INT_MONOCHROME_MONITOR_DETECT, A_S_T);
    
    // Update status
    m_Status.vbl_done = true;
    vbl_pending = false;
    
    // Frame complete - update display
    if (runstate == RUNSTATE_RUNNING) {
        // Signal that a frame is complete
        frame_complete = true;
        
        // Update statistics
        Stats.nFrames++;
    }
    
    // Check for overscan
    CheckVerticalOverscan();
}
```

### 3. Horizontal Blank (HBL) Handling

```cpp
void TGlue::EndHBL() {
    // End of scanline processing
    
    // Trigger HBL interrupt if enabled
    if (hbl_pending) {
        Mfp.GetInterrupt(MFP_INT_TIMER_B, A_S_T);
        hbl_pending = false;
    }
    
    // Update status
    m_Status.hbl_done = true;
    
    // Move to next scanline
    IncScanline();
    
    // Check for side overscan (left/right borders)
    CheckSideOverscan();
}
```

### 4. Address Decoding

The Glue chip decodes the 24-bit address bus to generate chip select signals for the various memory regions.

#### Decode Table

```cpp
// In TGlue constructor
TGlue::TGlue() {
    // Initialize decode table
    memset(Decode, 0, sizeof(Decode));
    
    // Set up decode values for different address ranges
    // High byte of address determines the region
    
    // 0x00-0x3F: ST RAM (0x000000-0x3FFFFF)
    for (int i = 0x00; i <= 0x3F; i++) {
        Decode[i] = STRAM;
    }
    
    // 0xFC-0xFF: ROM and I/O (0xFC0000-0xFFFFFF)
    for (int i = 0xFC; i <= 0xFF; i++) {
        Decode[i] = ROM;
    }
    
    // Special cases for I/O registers
    Decode[0xFF] = DEV; // I/O devices
}
```

#### Memory Region Identification

```cpp
// In mmu.cpp
MEM_ADDRESS mmu_confused_address(MEM_ADDRESS ad) {
    // Determine which memory region an address belongs to
    BYTE high_byte = (ad >> 16) & 0xFF;
    
    switch (Glue.Decode[high_byte]) {
        case TGlue::STRAM:
        case TGlue::STRAM_OR_ROM:
            // ST RAM
            if (ad < himem) {
                return ad;
            }
            break;
            
        case TGlue::ROM:
        case TGlue::ROM1:
            // ROM
            if (ad < tos_len) {
                return ad;
            }
            break;
            
        case TGlue::DEV:
            // I/O devices
            if (ad >= 0xFF8000 && ad <= 0xFFFFFF) {
                return ad;
            }
            break;
            
        case TGlue::CART:
        case TGlue::CART2:
        case TGlue::CART3:
            // Cartridge
            if (ad >= cartbase && ad <= cartend) {
                return ad - cartbase;
            }
            break;
    }
    
    // Address is confused (invalid)
    return MMU_CONFUSED_ADDRESS;
}
```

### 5. Video Mode Control

#### Shift Mode

The Glue chip controls the video shift mode, which determines the video resolution:

```cpp
void TGlue::SetShiftMode(BYTE NewRes) {
    // NewRes contains the new shift mode (2 bits)
    // 00 = Low resolution (320x200, 16 colors)
    // 01 = Medium resolution (640x200, 4 colors)
    // 10 = High resolution (640x400, 2 colors)
    // 11 = (STE) Super high resolution (various modes)
    
    ShiftMode = NewRes & 3;
    
    // Update Shifter
    Shifter.ShiftMode = ShiftMode;
    
    // Update timing parameters
    UpdateScanlineTiming();
}
```

#### Sync Mode

```cpp
void TGlue::SetSyncMode(BYTE NewSync) {
    // NewSync contains the new sync mode (2 bits)
    // Controls PAL/NTSC timing and sync polarity
    
    SyncMode = NewSync & 3;
    
    // Update video frequency
    if (NewSync & SYNCPAL) {
        VideoFreq = FREQ_IDX_50; // PAL (50Hz)
    } else {
        VideoFreq = FREQ_IDX_60; // NTSC (60Hz)
    }
    
    // Update timing
    UpdateScanlineTiming();
}
```

### 6. DRAM Refresh

While the MMU handles the actual DRAM refresh counters, the Glue chip provides the timing signals:

```cpp
// In mmu.cpp
void TMmu::UpdateVideoCounter(short CyclesIn) {
    // Update video counter based on Glue timing
    // This is used for DRAM refresh timing
    
    VideoCounter += CyclesIn;
    
    // Check if we need to trigger DRAM refresh
    if (VideoCounter >= refresh_interval) {
        VideoCounter = 0;
        
        // Trigger refresh cycle
        // ... refresh logic
    }
}
```

## Video Timing Details

### Scanline Structure

Each scanline is divided into several phases:

```
STANDARD SCANLINE (512 cycles @ 8MHz = 64μs):
┌─────────────────────────────────────────────────────────────┐
│ HBL | HSYNC | DE  | VISIBLE | DE  | HSYNC | HBL | BORDER │
│ 64  | 8     | 40  | 448     | 40  | 8     | 64  | (var)  │
│ cyc | cyc  | cyc | cyc    | cyc | cyc  | cyc |        │
├─────┼───────┼─────┼─────────┼─────┼───────┼─────┼────────┤
│     │       │     │         │     │       │     │        │
│ 0   │ 64    │ 72  │ 112     │ 552 │ 560   │ 568 │ 512    │
│     │       │     │         │     │       │     │        │
└─────┴───────┴─────┴─────────┴─────┴───────┴─────┴────────┘

Phase Descriptions:
- HBL (Horizontal Blank): CPU can access memory, no video output
- HSYNC: Horizontal sync pulse (active low)
- DE (Display Enable): Preparing for video output
- VISIBLE: Active video area (pixels are displayed)
- BORDER: Right border (overscan area)
```

### Frame Structure (PAL 50Hz)

```
PAL FRAME (312.5 scanlines, 20ms):
┌─────────────────────────────────────────────────────────────┐
│ VERTICAL BLANK (VBL)  │  VISIBLE AREA  │  VERTICAL BLANK │
│ Lines 0-31           │ Lines 32-311   │ Line 312        │
│ (32 lines)          │ (280 lines)    │ (1 line)        │
├─────────────────────┼────────────────┼─────────────────┤
│                     │                │                 │
│  VBL INTERRUPT      │  ACTIVE VIDEO  │  VBL START      │
│  (MFP IRQ)          │  DISPLAY       │  (next frame)   │
│                     │                │                 │
└─────────────────────┴────────────────┴─────────────────┘

Total: 312.5 lines × 64μs = 20ms (50Hz)
```

### Timing Parameters by Video Mode

```cpp
// In glue.cpp
const short Glue_ScanlineCycles[3] = {512, 512, 512}; // ST, STE, MegaSTE
const short Glue_DE_cycles[3][3] = {
    // 50Hz, 60Hz, 71Hz
    {112, 104, 96},   // Low res
    {112, 104, 96},   // Medium res
    {112, 104, 96}    // High res
};

const short Glue_nLines[3][3] = {
    // 50Hz, 60Hz, 71Hz
    {313, 263, 201},  // PAL variants
    {313, 263, 201},  // NTSC variants
    {313, 263, 201}   // Other variants
};
```

## Integration with Other Components

### 1. Shifter Integration

The Glue chip works closely with the Shifter to generate video output:

```cpp
// In shifter.cpp
void TShifter::Render(SHORT cycles_in, BYTE dispatcher) {
    // Get current scanline info from Glue
    int current_line = Glue.VCount;
    
    // Check if we're in the visible area
    if (current_line >= Glue.de_start_line && 
        current_line < Glue.de_end_line) {
        
        // Render visible pixels
        DrawScanline(cycles_in);
    } else {
        // Render border
        DrawBorder(cycles_in);
    }
}
```

### 2. MMU Integration

The Glue chip provides address decoding information to the MMU:

```cpp
// In mmu.cpp
MEM_ADDRESS TMmu::ReadVideoCounter(short CyclesIn) {
    // Read video counter (used for Shifter address calculation)
    // This is synchronized with Glue timing
    
    UpdateVideoCounter(CyclesIn);
    return VideoCounter;
}
```

### 3. MFP Integration

The Glue chip triggers interrupts through the MFP:

```cpp
// In glue.cpp
void TGlue::Vbl() {
    // ... VBL processing ...
    
    // Trigger VBL interrupt via MFP
    Mfp.GetInterrupt(MFP_INT_MONOCHROME_MONITOR_DETECT, A_S_T);
}
```

## Special Features

### 1. Overscan Handling

The Glue emulation includes support for overscan effects, which are used by many demos:

```cpp
void TGlue::CheckSideOverscan() {
    // Check for left/right border effects
    // This is important for demos that use border tricks
    
    if (CurrentScanline.StartCycle < LINE_START_LIMIT) {
        // Left border effect
        // ... handle overscan
    }
    
    if (CurrentScanline.EndCycle > LINE_STOP_LIMIT) {
        // Right border effect
        // ... handle overscan
    }
}

void TGlue::CheckVerticalOverscan() {
    // Check for top/bottom border effects
    
    if (VCount < de_start_line) {
        // Top border
        // ... handle overscan
    }
    
    if (VCount >= de_end_line) {
        // Bottom border
        // ... handle overscan
    }
}
```

### 2. Video Trick Analysis

Steem includes advanced features for analyzing video tricks used in demos:

```cpp
void TGlue::AddFreqChange(BYTE f) {
    // Record a frequency change at current cycle
    // Used for analyzing demo effects
}

void TGlue::AddShiftModeChange(BYTE mode) {
    // Record a shift mode change at current cycle
    // Used for analyzing demo effects
}

int TGlue::FreqChangeAtCycle(int cycle) {
    // Find frequency change at or before given cycle
    // Used for debugging video effects
}

int TGlue::ShiftModeAtCycle(int cycle) {
    // Get shift mode at given cycle
    // Used for debugging video effects
}
```

### 3. Game Cartridge Support

```cpp
// In glue.cpp
void TGlue::Reset(bool Cold) {
    if (Cold) {
        // Initialize cartridge memory range
        cartbase = 0xFA0000; // Default cartridge base
        cartend = 0xFBFFFF;   // Default cartridge end
    }
}
```

## Accuracy Considerations

### Accurate Behaviors

| Feature | Steem Implementation | Real Hardware | Accuracy |
|---------|---------------------|---------------|----------|
| Video Timing | Cycle-accurate | Cycle-accurate | 100% |
| VBL/HBL Generation | Accurate | Accurate | 100% |
| Address Decoding | Accurate | Accurate | 100% |
| Sync Generation | Accurate | Accurate | 100% |
| Scanline Structure | Accurate | Accurate | 100% |
| Frame Timing | Accurate | Accurate | 100% |
| Overscan Handling | Good | Hardware | ~95% |
| Video Mode Control | Accurate | Accurate | 100% |

### Known Limitations

1. **DRAM Refresh Timing**: Some refresh timing details are simplified
2. **Bus Arbitration**: Priority handling between CPU and DMA is approximate
3. **Wake-up States**: The initial power-on state (WU/WS) has some approximations
4. **Edge Cases**: Some rare timing edge cases may not be perfectly emulated

## Performance Optimizations

### 1. Precomputed Timing Tables

```cpp
// In glue.cpp
short ScanlineTiming[NTIMINGS][NFREQS];

// Initialized once at startup
void TGlue::TGlue() {
    // Precompute all scanline timing values
    for (int timing = 0; timing < NTIMINGS; timing++) {
        for (int freq = 0; freq < NFREQS; freq++) {
            ScanlineTiming[timing][freq] = CalculateTiming(timing, freq);
        }
    }
}
```

### 2. Event-Driven Execution

The Glue chip uses the agenda system to schedule video events efficiently:

```cpp
// In emulator.cpp
void init_timings() {
    // Schedule first video event
    Agenda.AddEvent(Glue.hbl_pending_time, EVENT_HBL, Glue.EndHBL);
}
```

### 3. Inline Functions

Critical functions are marked as inline for performance:

```cpp
inline bool TGlue::FetchingLine() {
    return bFetchingLine;
}
```

## Debugging Support

### 1. Video Timing Debugging

```cpp
// In glue.cpp
#ifdef DEBUG_BUILD
int TGlue::NextFreqChange(int cycle, int value) {
    // Find next frequency change after given cycle
    // Used for debugging video effects
}

int TGlue::PreviousFreqChange(int cycle) {
    // Find previous frequency change before given cycle
    // Used for debugging video effects
}
#endif
```

### 2. State Inspection

The Glue state can be inspected through the debugger:
- Current shift mode
- Current sync mode
- Video frequency
- Scanline timing
- VBL/HBL status
- Overscan settings

### 3. Snapshot Support

The Glue state is saved and restored in snapshots:
- All registers
- Timing state
- Video mode settings
- Event scheduling state

## Comparison with Real Hardware

### Glue Chip Pinout (Simplified)

```
GLUE CHIP (C029144) - PLCC68
┌─────────────────────────────────────────────────────────────┐
│  Pin   │ Signal Name          │ Direction │ Description        │
├────────┼─────────────────────┼───────────┼────────────────────┤
│   1    │ A0                  │ Input     │ Address bus bit 0  │
│   2    │ A1                  │ Input     │ Address bus bit 1  │
│   ...  │ ...                 │ ...       │ ...                │
│  25    │ A23                 │ Input     │ Address bus bit 23 │
│  26    │ CLK (8MHz)          │ Input     │ System clock       │
│  27    │ RESET               │ Input     │ System reset       │
│  28    │ DTACK               │ Output    │ Data transfer ack  │
│  29    │ BERR                │ Output    │ Bus error          │
│  30    │ VPA                 │ Output    │ Video address      │
│  31    │ RAS                 │ Output    │ Row address select │
│  32    │ CAS0-CAS3           │ Output    │ Column address     │
│  33    │ SHIFTER_CLK         │ Output    │ Shifter clock      │
│  34    │ HSYNC               │ Output    │ Horizontal sync    │
│  35    │ VSYNC               │ Output    │ Vertical sync      │
│  36    │ DE                  │ Output    │ Display enable     │
│  37    │ ROMEN               │ Output    │ ROM enable         │
│  38    │ RAMEN               │ Output    │ RAM enable         │
│  39    │ I/O EN              │ Output    │ I/O enable         │
│  40    │ VBL                 │ Output    │ VBL interrupt      │
│  41    │ HBL                 │ Output    │ HBL interrupt      │
│  ...  │ ...                 │ ...       │ ...                │
└─────────────────────────────────────────────────────────────┘
```

### Steem vs Real Hardware

| Aspect | Real Hardware | Steem Implementation |
|--------|---------------|---------------------|
| Address Decoding | Hardware logic gates | Software decode table |
| Timing Generation | Hardware counters | Software counters |
| Sync Generation | Hardware circuits | Software state machine |
| DRAM Refresh | Hardware counters | Software counters |
| Bus Arbitration | Hardware priority | Software priority |
| Video Timing | Hardware clocks | Software cycle counting |

## Conclusion

The Glue chip emulation in Steem SSE is a **highly accurate, cycle-accurate implementation** that faithfully reproduces the behavior of the real Atari ST Glue chip. The implementation:

- **Generates accurate video timing** for all video modes
- **Handles address decoding** for the entire 24-bit address space
- **Manages interrupts** (VBL, HBL) correctly
- **Supports overscan effects** used by demos
- **Integrates tightly** with the Shifter, MMU, and MFP
- **Provides debugging support** for video trick analysis

The Glue emulation serves as the **timing backbone** for the entire video system, providing the precise timing signals needed for accurate video output and synchronization with the rest of the hardware.