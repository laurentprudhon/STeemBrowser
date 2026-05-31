# Atari ST Bus Management: A Comprehensive Analysis

This document provides an in-depth examination of how the Atari ST manages its system bus, focusing on the collaboration between the Motorola 68000 CPU, the custom MMU chip, the GLUE ASIC, and other system components. The analysis covers memory access arbitration, supervisor/user mode handling, and the unique bus sharing mechanisms that define the Atari ST's performance characteristics.

---

## Table of Contents

1. [Overview of Atari ST Bus Architecture](#1-overview-of-atari-st-bus-architecture)
2. [Key Components in Bus Management](#2-key-components-in-bus-management)
3. [Memory Access Workflow](#3-memory-access-workflow)
4. [Supervisor vs User Mode Handling](#4-supervisor-vs-user-mode-handling)
5. [Bus Arbitration and Priority System](#5-bus-arbitration-and-priority-system)
6. [Cycle-Level Bus Timing Analysis](#6-cycle-level-bus-timing-analysis)
7. [Video Memory Access and Bandwidth Arbitration](#7-video-memory-access-and-bandwidth-arbitration)
8. [DMA Operations and Bus Contention](#8-dma-operations-and-bus-contention)
9. [DRAM Refresh and Memory Timing](#9-dram-refresh-and-memory-timing)
10. [ST vs STE: Evolution of Bus Management](#10-st-vs-ste-evolution-of-bus-management)
11. [Performance Implications](#11-performance-implications)
12. [Practical Examples and Scenarios](#12-practical-examples-and-scenarios)

---

## 1. Overview of Atari ST Bus Architecture

The Atari ST employs a **shared memory architecture** where multiple components compete for access to a single RAM bus. Unlike modern systems with dedicated memory channels or virtual memory management, the ST's design revolves around **cycle-by-cycle arbitration** of a common resource pool.

### Core Principles:
- **Single Bus Design**: All major components (CPU, Video Shifter, DMA controllers) share the same RAM bus
- **Synchronous Operation**: Bus cycles are synchronized to the 68000's E-clock (≈4MHz bus cycle rate)
- **Priority-Based Arbitration**: Components compete for bus access based on a fixed priority hierarchy
- **No Virtual Memory**: The MMU does not perform address translation or paging

### Bus Participants:
| Component | Role | Priority Level |
|-----------|------|----------------|
| Video Shifter | Continuous pixel data fetching | Highest |
| DRAM Refresh | Memory maintenance | High |
| DMA Controllers | Data transfer operations | Medium |
| CPU (68000) | Program execution | Lowest |

---

## 2. Key Components in Bus Management

### 2.1 Motorola 68000 CPU

**Role in Bus Management:**
- Initiates memory access requests
- Provides address and control signals (AS, R/W, UDS, LDS)
- Responds to DTACK signals for cycle completion
- Operates in either **User Mode** or **Supervisor Mode**

**Key Signals:**
- **AS (Address Strobe)**: Indicates valid address on bus
- **R/W**: Read/Write control line
- **UDS/LDS**: Upper/Lower byte select for word operations
- **DTACK**: Data acknowledge signal (wait state control)

**Execution States:**
- **User Mode**: Normal application execution (S-bit = 0 in Status Register)
- **Supervisor Mode**: Privileged operations (S-bit = 1 in Status Register)

### 2.2 GLUE ASIC (Gate Array)

**Primary Responsibilities:**
- **Address Decoding**: Determines which memory region is being accessed
- **Access Control**: Enforces memory protection rules
- **Bus Arbitration**: Manages priority between competing bus masters
- **Bus Error Generation**: Asserts bus errors for illegal accesses

**Protection Logic:**
- Monitors CPU state (User/Supervisor) via S-bit
- Decodes address lines to identify protected regions
- Generates **Bus Error** exceptions for unauthorized accesses
- Works in conjunction with MMU for memory timing

### 2.3 Atari MMU Chip

**Important Clarification:**
The Atari ST's MMU is **not** a traditional Memory Management Unit as found in modern systems. It does **not** perform:
- Virtual-to-physical address translation
- Paging or segmentation
- Per-process address spaces
- Demand paging

**Actual Functions:**
- **DRAM Control**: Manages DRAM timing and refresh cycles
- **Memory Arbitration**: Coordinates access between CPU, Video, and DMA
- **Address Management**: Handles memory addressing for video and system RAM
- **Bandwidth Allocation**: Ensures fair distribution of memory cycles

### 2.4 Video Shifter

**Memory Access Characteristics:**
- **Continuous Operation**: Requires constant memory access for pixel fetching
- **High Bandwidth**: Consumes majority of memory cycles in graphics modes
- **Fixed Priority**: Always gets highest priority in arbitration
- **Non-Interruptible**: Cannot be paused or preempted

**Access Patterns:**
- Low Resolution (320×200, 4 planes): ~64 cycles per scanline
- Medium Resolution (640×200, 2 planes): ~128 cycles per scanline
- High Resolution (640×400, 1 plane): ~256 cycles per scanline

### 2.5 DMA Controllers

**Types in Atari ST:**
- **Floppy DMA**: Handles disk I/O operations
- **Sound DMA (STE only)**: PCM audio sample streaming
- **Blitter (STE only)**: Block memory transfer operations

**Access Characteristics:**
- **Bursty**: Transfers occur in short, intensive bursts
- **Periodic**: Sound DMA operates at regular intervals
- **High Bandwidth**: Blitter can monopolize bus when active

---

## 3. Memory Access Workflow

### 3.1 Standard CPU Memory Access

```
1. CPU places address on bus
   - Asserts AS (Address Strobe)
   - Sets R/W line
   - Activates UDS/LDS for byte selection
   - Provides address (A1-A23)

2. GLUE begins address decoding
   - Identifies target memory region
   - Checks access permissions
   - Determines if access is allowed

3. Arbitration Decision
   - GLUE/MMU checks current bus status
   - Determines if higher-priority device needs bus
   - If video or refresh active: CPU must wait
   - If bus free: proceed to next step

4. Memory Access Execution
   - MMU activates DRAM control signals
   - Data is read from or written to memory
   - DTACK is asserted when data is ready

5. Cycle Completion
   - CPU latches data (for reads)
   - CPU deasserts control signals
   - Bus returns to idle state
```

### 3.2 Access Denied Scenario

```
1. CPU attempts access to protected region in User Mode

2. GLUE detects:
   - Address in protected range (e.g., $FF8000-$FFFFFF)
   - CPU in User Mode (S-bit = 0)

3. GLUE asserts Bus Error signal

4. 68000 CPU responds:
   - Copies current status to stack
   - Sets S-bit (enters Supervisor Mode)
   - Switches to Supervisor Stack Pointer (SSP)
   - Vectors to Bus Error handler (address $000008)

5. TOS (operating system) handles exception in Supervisor Mode
```

---

## 4. Supervisor vs User Mode Handling

### 4.1 Mode Differences

| Feature | User Mode | Supervisor Mode |
|---------|-----------|-----------------|
| Privileged Instructions | ❌ Not allowed | ✅ Allowed |
| Access to I/O Registers | ❌ Not allowed | ✅ Allowed |
| Access to System RAM | ❌ Limited | ✅ Full access |
| Stack Pointer | USP (User Stack Pointer) | SSP (Supervisor Stack Pointer) |
| Exception Handling | ❌ Cannot handle | ✅ Can handle |
| TRAP Instructions | ❌ Cannot execute | ✅ Can execute |

### 4.2 Mode Switching Mechanisms

**User → Supervisor:**
- **TRAP Instructions**: Software interrupts (TRAP #0-#15)
- **Exceptions**: Bus Error, Address Error, Illegal Instruction, etc.
- **Interrupts**: Hardware interrupts (if enabled)

**Supervisor → User:**
- **RTE (Return from Exception)**: Returns from exception handler
- **MOVES Instruction**: Can move data between address spaces

### 4.3 Protection Implementation

**Hardware-Enforced:**
- 68000 CPU blocks privileged instructions in User Mode
- GLUE ASIC blocks access to protected memory regions in User Mode

**Software-Enforced:**
- TOS validates parameters for system calls
- Applications must use TRAP instructions for system services

### 4.4 Protected Memory Regions

| Address Range | Region | Access |
|---------------|--------|--------|
| $000000-$00FFFF | Low RAM | User/Supervisor |
| $FF0000-$FF7FFF | I/O Registers | Supervisor only |
| $FF8000-$FFFFFF | ROM (TOS) | Supervisor only |
| $010000-$7FFFFF | High RAM | User/Supervisor |

---

## 5. Bus Arbitration and Priority System

### 5.1 Priority Hierarchy

The Atari ST employs a **fixed priority system** for bus arbitration:

```
Priority Level 1: Video Shifter (Highest)
├── Continuous pixel fetching
├── Cannot be interrupted
└── Defines worst-case CPU performance

Priority Level 2: DRAM Refresh
├── Periodic memory maintenance
├── Mandatory for DRAM stability
└── Short, fixed-interval cycles

Priority Level 3: DMA Controllers
├── Floppy DMA (ST)
├── Sound DMA (STE)
└── Blitter (STE)

Priority Level 4: CPU (68000) (Lowest)
└── Gets remaining cycles
```

### 5.2 Arbitration Process

```
1. Request Phase:
   - Component asserts bus request
   - GLUE/MMU receives request

2. Evaluation Phase:
   - Check current bus status
   - Identify active high-priority requests
   - Determine if request can be granted

3. Grant Phase:
   - Highest-priority request gets bus
   - Other requests remain pending

4. Execution Phase:
   - Granted component performs memory operation
   - Bus signals (AS, R/W, etc.) are activated

5. Release Phase:
   - Component completes operation
   - Bus returns to idle
   - Next arbitration cycle begins
```

### 5.3 Arbitration Timing

- **Cycle Time**: ~280ns per bus cycle (4MHz bus rate)
- **Decision Time**: ~1-2 clock cycles for arbitration
- **Wait States**: Inserted when higher-priority device has bus
- **Stretching**: Cycles can be extended for slow devices

---

## 6. Cycle-Level Bus Timing Analysis

### 6.1 68000 Bus Cycle States

The 68000 bus cycle is divided into **states** (S0-S4+):

```
State S0:
- Address becomes valid
- AS asserted
- R/W set
- UDS/LDS activated

State S1:
- Address strobe active
- GLUE begins decode
- Arbitration starts

State S2:
- Arbitration decision
- Wait states may begin
- DTACK monitoring starts

State S3:
- Data transfer (if no wait states)
- Or continued waiting

State S4:
- Data latch (for reads)
- Or additional wait states
- Cycle completion
```

### 6.2 Standard Read Cycle (No Contention)

```
Clock:  1   2   3   4   5   6   7   8
State: S0  S1  S2  S3  S4  S4  S4  S4
--------------------------------------
ADDR:  ====VALID===================
AS:       ___-------------------
R/W:      ------------------------
UDS/LDS:  ___-------------------
DTACK:            _____________
DATA:                     ==VALID==
```

**Duration**: 4-8 clock cycles (depending on memory speed)

### 6.3 Read Cycle with Video Contention

```
Clock:  1   2   3   4   5   6   7   8
State: S0  S1  S2  S3  S4  S4  S4  S4
--------------------------------------
ADDR:  ====VALID===================
AS:       ___-------------------
R/W:      ------------------------
VIDEO:         [STEALS CYCLE]
DTACK:                  _______
DATA:                          ==VALID==
CPU:           WAIT  WAIT
```

**Duration**: 6-10+ clock cycles (with wait states)

### 6.4 Cycle Stealing Mechanism

The Atari ST implements **cycle stealing** where:

1. **Video Shifter** takes priority cycles for pixel fetching
2. **CPU** is forced to wait (inserts wait states)
3. **DTACK** is delayed until bus is available
4. **CPU** resumes when cycle is granted

**Key Characteristics:**
- **Irregular Wait States**: Not uniform delays
- **Position-Dependent**: Varies by scanline phase
- **Mode-Dependent**: Different for each video resolution

---

## 7. Video Memory Access and Bandwidth Arbitration

### 7.1 Video Bandwidth Requirements

The Video Shifter requires **continuous memory access** for pixel data:

| Video Mode | Resolution | Planes | Cycles/Scanline | Bandwidth |
|------------|------------|--------|-----------------|-----------|
| Low Res | 320×200 | 4 | ~64 | High |
| Medium Res | 640×200 | 2 | ~128 | Very High |
| High Res | 640×400 | 1 | ~256 | Extreme |

### 7.2 Scanline Timeline (Low Resolution)

```
Cycle:  01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 16
Bus:     V  V  V  V  V  V  V  V  V  V  V  V  V  V  V  V
CPU:     -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -

Cycle:  17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32
Bus:     V  V  V  V  V  V  V  V  V  V  V  V  V  V  V  V
CPU:     -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -

Cycle:  33 34 35 36 37 38 39 40 41 42 43 44 45 46 47 48
Bus:     V  V  V  V  V  V  V  V  R  V  V  V  V  V  V  V
CPU:     -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -

Cycle:  49 50 51 52 53 54 55 56 57 58 59 60 61 62 63 64
Bus:     V  V  V  V  V  V  V  V  V  V  V  V  V  V  V  V
CPU:     -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -

Legend:
V = Video fetch (Shifter)
R = DRAM refresh
- = CPU stalled (waiting for RAM)
```

### 7.3 CPU Burst Execution

In reality, CPU execution occurs in **bursts between video windows**:

```
Time →------------------------------------------------
V V V V V V V V V V V V V V V V  (scanline fetch)
C   C     C   C        C    C    (CPU bursts)
V V V V V V V V V V V V V V V V
C      C      C     C            (CPU bursts)
```

### 7.4 Bandwidth Arbitration Formula

**Effective CPU Performance** can be calculated as:

```
Effective CPU Speed = Base CPU Speed
                     - Video Bandwidth Overhead
                     - Refresh Overhead
                     - DMA Contention
                     - Arbitration Delays
```

**Example Calculation (Low Res):**
- Base: 8MHz CPU
- Video: ~50% bandwidth
- Refresh: ~5% bandwidth
- **Effective**: ~40% of base speed = ~3.2MHz equivalent

---

## 8. DMA Operations and Bus Contention

### 8.1 Floppy DMA (ST)

**Characteristics:**
- **Burst Transfer**: Short, intensive bursts during disk I/O
- **Sporadic**: Only active during floppy operations
- **Medium Priority**: Below video, above CPU

**Access Pattern:**
```
Cycle:  01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 16
Bus:     V  V  V  V  V  V  V  V  V  V  V  V  V  V  V  V
CPU:     -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
DMA:     .  .  .  .  .  .  .  .  .  .  .  .  .  .  .  .

Cycle:  33 34 35 36 37 38 39 40 41 42 43 44 45 46 47 48
Bus:     V  V  V  V  V  V  V  V  R  V  V  V  V  V  V  V
CPU:     -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
DMA:     .  .  .  .  .  D  .  .  .  .  .  D  .  .  .  .

Legend:
D = DMA transfer
. = No DMA activity
```

### 8.2 Sound DMA (STE)

**Characteristics:**
- **Continuous Streaming**: Regular, periodic sample fetching
- **High Priority**: Similar to video in priority
- **Fixed Rate**: Sample rate determines access frequency

**Access Pattern (6kHz Sample Rate):**
```
Cycle:  01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 16
Bus:     V  V  V  V  V  V  V  V
Sound:   S  .  S  .  S  .  S  .
CPU:     -  -  -  -  -  -  -  -

Legend:
S = Sound DMA fetch
. = No sound activity
```

### 8.3 Blitter (STE)

**Characteristics:**
- **Block Transfer Engine**: Copies memory blocks with raster operations
- **Burst Operation**: Long sequences of read/write cycles
- **High Bandwidth**: Can saturate bus when active
- **Bidirectional**: Reads source, writes destination

**Access Pattern:**
```
Cycle:  01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 16
Bus:     V  V  V  V  V  V  V  V
Blit:    B  B  B  B  B  B  B  B
CPU:     -  -  -  -  -  -  -  -

Cycle:  17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32
Bus:     V  V  V  V  V  V  V  V
Blit:    .  .  .  .  .  .  .  .
CPU:     C  C  C  C  C  C  C  C

Legend:
B = Blitter operation
. = Blitter idle
C = CPU operation
```

---

## 9. DRAM Refresh and Memory Timing

### 9.1 DRAM Refresh Requirements

**Purpose:**
- Maintain DRAM cell charges
- Prevent data loss
- Required every ~2ms for each row

**Implementation:**
- **Periodic Cycles**: Refresh cycles inserted at regular intervals
- **Short Duration**: Typically 1-2 bus cycles per refresh
- **High Priority**: Above CPU, below video

**Access Pattern:**
```
Cycle:  01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 16
Bus:     V  V  V  V  V  V  V  V  R  V  V  V  V  V  V  V
CPU:     -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -

Legend:
R = Refresh cycle
```

### 9.2 Refresh Impact on Performance

**Overhead:**
- **Frequency**: ~1-2% of total bus cycles
- **Timing**: Can coincide with CPU requests
- **Effect**: Adds additional wait states

**Mitigation:**
- Refresh cycles are **short** (1-2 cycles)
- Refresh is **periodic** (predictable)
- CPU can **resume quickly** after refresh

### 9.3 DRAM Timing Characteristics

**Access Time:**
- **Row Address Strobe (RAS)**: ~100-150ns
- **Column Address Strobe (CAS)**: ~50-70ns
- **Page Mode**: Faster access to same row

**Atari ST Implementation:**
- **Single-Bank DRAM**: All memory in one bank
- **No Cache**: Direct DRAM access
- **Wait States**: Inserted for slow DRAM

---

## 10. ST vs STE: Evolution of Bus Management

### 10.1 Original ST Model

**Components:**
- CPU (68000)
- Video Shifter
- Floppy DMA
- DRAM Refresh

**Priority:**
```
Video > Refresh > Floppy DMA > CPU
```

**Characteristics:**
- Simple arbitration
- Predictable CPU performance
- Limited DMA contention

### 10.2 STE Enhanced Model

**Additional Components:**
- Sound DMA (PCM audio)
- Blitter (block transfer engine)

**New Priority:**
```
Video > Blitter > Sound DMA > Refresh > Floppy DMA > CPU
```

**Characteristics:**
- Complex arbitration
- More frequent CPU stalls
- Less predictable timing
- Higher DMA contention

### 10.3 Key Differences

| Feature | Atari ST | Atari STE |
|---------|----------|------------|
| DMA Controllers | Floppy only | Floppy + Sound + Blitter |
| CPU Priority | Low | Lowest |
| Bus Contention | Low | High |
| Performance Predictability | High | Low |
| Graphics Acceleration | None | Blitter |
| Audio Capabilities | YM2149 only | YM2149 + DMA Sound |

### 10.4 STE Bus Arbitration Model

```
RAM Bus Contenders:
├── Video Shifter (continuous, highest priority)
├── Blitter (burst, high bandwidth)
├── Sound DMA (periodic, steady)
├── Floppy DMA (occasional bursts)
├── DRAM Refresh (periodic, mandatory)
└── CPU (68000) (lowest priority)

Arbitration Process:
1. Video always gets priority for pixel fetching
2. Blitter gets next priority for active transfers
3. Sound DMA gets periodic slots for sample streaming
4. Floppy DMA gets occasional bursts
5. Refresh gets mandatory cycles
6. CPU gets remaining cycles
```

### 10.5 STE Performance Impact

**CPU Performance Degradation:**
- **Idle Screen**: ~80% of ST performance
- **Sound Active**: ~60-70% of ST performance
- **Blitter Active**: ~40-50% of ST performance
- **Full Load**: ~20-30% of ST performance

**Trade-off:**
- **CPU Speed ↓** when DMA active
- **Graphics Speed ↑** massively (Blitter)
- **Audio Quality ↑** (DMA Sound)

---

## 11. Performance Implications

### 11.1 Factors Affecting CPU Performance

1. **Video Mode:**
   - Low Res: ~50% bandwidth → ~50% CPU performance
   - Medium Res: ~60% bandwidth → ~40% CPU performance
   - High Res: ~70% bandwidth → ~30% CPU performance

2. **DMA Activity:**
   - Floppy: Minimal impact (sporadic)
   - Sound (STE): ~10-15% impact (continuous)
   - Blitter (STE): ~20-40% impact (burst)

3. **Refresh Overhead:**
   - ~1-2% impact (constant)

4. **Arbitration Delays:**
   - ~5-10% impact (variable)

### 11.2 Performance Optimization Strategies

**For Programmers:**
- **Minimize Video Bandwidth**: Use lower resolutions when possible
- **Avoid Busy-Waiting**: Use interrupts instead of polling
- **Optimize Memory Access**: Group memory operations
- **Use Blitter (STE)**: Offload graphics operations
- **Disable Sound (STE)**: When maximum CPU performance needed

**For Hardware Design:**
- **Cycle Stealing**: Allows video to maintain quality
- **Priority System**: Ensures critical operations complete
- **Burst Transfers**: Maximize throughput for DMA operations

### 11.3 Real-World Performance Examples

| Scenario | Effective CPU Speed |
|----------|---------------------|
| ST, Low Res, No DMA | ~4.0 MHz |
| ST, Medium Res, No DMA | ~3.2 MHz |
| ST, High Res, No DMA | ~2.4 MHz |
| STE, Low Res, Sound Active | ~3.5 MHz |
| STE, Low Res, Blitter Active | ~2.5 MHz |
| STE, Medium Res, Full Load | ~1.5 MHz |

---

## 12. Practical Examples and Scenarios

### 12.1 System Call Execution

**Scenario:** Application calls GEMDOS function

```
1. Application runs in User Mode
2. Executes TRAP #1 (GEMDOS)
3. 68000 switches to Supervisor Mode:
   - S-bit set in Status Register
   - Switches to SSP
   - Saves USP to stack
4. TOS handles TRAP in Supervisor Mode
5. TOS validates parameters
6. TOS performs operation (may access protected memory)
7. TOS returns with RTE
8. 68000 switches back to User Mode:
   - S-bit cleared
   - Switches back to USP
   - Restores context
9. Application resumes in User Mode
```

### 12.2 Illegal Memory Access

**Scenario:** User Mode application attempts to access I/O registers

```
1. Application executes: MOVE.B D0,$FF8000
2. CPU places address on bus (User Mode, S-bit=0)
3. GLUE decodes address: $FF8000 is in protected range
4. GLUE checks CPU state: User Mode
5. GLUE asserts Bus Error signal
6. 68000 responds to Bus Error:
   - Copies status to stack
   - Sets S-bit (Supervisor Mode)
   - Switches to SSP
   - Vectors to Bus Error handler ($000008)
7. TOS Bus Error handler executes in Supervisor Mode
8. TOS may terminate application or handle error
```

### 12.3 Video Intensive Operation

**Scenario:** Scrolling screen in Low Resolution

```
Scanline 1:
- Video: 64 cycles for pixel fetch
- CPU: 0 cycles available
- Result: CPU completely stalled

Between Scanlines:
- Video: Brief pause between scanlines
- CPU: ~16 cycles available
- Result: CPU can execute a few instructions

Overall:
- Video: ~90% of bus cycles
- CPU: ~10% of bus cycles
- Effective CPU speed: ~0.8 MHz (from 8 MHz base)
```

### 12.4 STE with Active Blitter

**Scenario:** Blitter copying large bitmap

```
Phase 1: Blitter Setup (CPU)
- CPU configures blitter registers
- CPU starts blitter operation
- CPU: Active, full speed

Phase 2: Blitter Execution
- Blitter: Active, high priority
- Video: Active, highest priority
- CPU: Starved, minimal cycles
- Result: CPU effectively paused

Phase 3: Blitter Completion
- Blitter: Finishes operation
- CPU: Resumes full speed
- Result: Burst execution pattern
```

---

## Conclusion

The Atari ST's bus management system represents a **sophisticated yet simple** approach to shared memory architecture. By employing a **priority-based arbitration system** with the GLUE ASIC and MMU chip working in tandem, the ST achieves:

1. **Efficient Resource Sharing**: Multiple components share a single RAM bus
2. **Memory Protection**: Basic protection between User and Supervisor modes
3. **Performance Optimization**: Video always gets priority for smooth graphics
4. **Hardware Acceleration**: DMA and (in STE) Blitter offload CPU tasks

The **STE enhances this model** by adding more DMA controllers and the Blitter, creating a more complex but more capable system. However, this comes at the cost of **reduced CPU performance** due to increased bus contention.

### Key Takeaways:

1. **The Atari ST does not use a traditional MMU** for virtual memory but rather for **memory arbitration and DRAM control**
2. **GLUE ASIC performs most protection logic**, generating bus errors for illegal accesses
3. **Video Shifter has highest priority**, defining the system's performance characteristics
4. **CPU gets lowest priority**, resulting in variable performance based on video mode and DMA activity
5. **STE adds significant bus contention** with Sound DMA and Blitter, further reducing CPU performance but adding powerful hardware acceleration

### Final Mental Model:

Instead of thinking of the Atari ST as:
> CPU + RAM + Peripherals

Think of it as:
> **A single RAM bus being continuously auctioned every 280ns, with Video always winning the highest bids, and CPU getting whatever is left over.**

This **cycle-by-cycle arbitration** is what defines the Atari ST's unique performance characteristics and makes it both powerful (for its time) and challenging to program efficiently.
