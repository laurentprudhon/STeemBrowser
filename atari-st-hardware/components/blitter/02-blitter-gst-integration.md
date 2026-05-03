# STe/MegaSTE Blitter - GST Integration

## 1. Overview

### 1.1 Physical Location

The blitter in the Atari STe and MegaSTE is **not a standalone chip**. It is implemented as a functional block within the **GST MCU** (Glue + Memory Management Unit) ASIC. This ASIC is a custom Atari chip, physically the large gate-array package (typically labeled "GSTMCU" or similar) on the STe/MegaSTE motherboard. The GST MCU combines several functions:

- Glue logic between the Motorola 68000 CPU and peripheral chips
- Memory management unit (MMU) with DRAM refresh
- Address decoding for ROM, RAM, and I/O regions
- Bus arbitration between 68000, blitter, and other DMA devices
- Joystick/paddle/pen management
- Clock generation

The **GST Shifter** (a separate ASIC) handles video signal generation (PAL/NTSC encoder) and sound DAC DMA, but does not contain the blitter itself.

### 1.2 Historical Context

The original Atari ST (520ST, 1040ST, STe predecessor) had **no blitter** -- all bit-block operations were performed by the 68000 CPU in software, which was extremely slow for graphics operations. The Atari STe (1989) added the blitter as one of its major hardware upgrades, alongside:

- 16-color graphics modes (enhanced Shifter)
- SIMM-based RAM (vs. discrete DRAM chips)
- DMA sound
- MultiTOS support (MegaSTE)
- 16 MHz CPU mode (MegaSTE)

The blitter implements a hardware version of the **BitBlt** (Bit-Block Transfer) algorithm, first formally defined by Pike, Guibas, and Ingalls (SIGGRAPH '84). It performs the same class of operations as the Amiga's blitter and is capable of moving, filling, and manipulating bitmaps at speeds far exceeding the 68000.

### 1.3 Capabilities

The STe blitter can:

- Transfer bit-aligned blocks of source data to a destination
- Apply one of **16 Boolean logic operations** (OP register) combining source and destination
- Apply halftone patterns (16x16 pattern RAM)
- Skew (shift) source data by 0-15 bits horizontally
- Mask individual destination words using up to 3 end masks (left boundary, center, right boundary)
- Execute 2D block transfers with independent source and destination line increments
- Operate in **shared bus mode** (64 cycles shared between CPU and blitter) or **HOG mode** (CPU halted entirely)

## 2. Blitter Register Map

All blitter registers are memory-mapped at base address **$FFFF8A00** (base $FF8A00 in ST-compatible addressing). The base block spans bytes $FF8A00 through $FF8A3F.

### 2.1 Full Register Map Table

| Offset (hex) | Access | Size | Name | Bits | Description |
|:---:|:---:|:---:|:---|:---|:---|
| $FF8A00-$FF8A1E | RW | 16-bit (x8) | Halftone RAM | 0-15 | 8-word (16-line) halftone pattern memory. Word `n` is used for lines `n` and `n+16` (repeats every 16 lines). Only accessible via word/long instruction, not byte. |
| $FF8A20 | RW | 16-bit | Source X Increment | X14-X0 | Signed 15-bit value (LSB ignored). Offset in **bytes** to next source word in the current line. Sign-extended and added to Source Address after each source word fetch. Byte access invalid. |
| $FF8A22 | RW | 16-bit | Source Y Increment | X14-X0 | Signed 15-bit value (LSB ignored). Offset in **bytes** to the first source word of the **next line**. Sign-extended and added to Source Address after last source word of each line. Byte access invalid. Note: Y-inc must be even (blitter accesses only word-aligned). |
| $FF8A24-$FF8A26 | RW | 24-bit (3 bytes) | Source Address | A22-A0 | Current source field address (23-bit physical address). Only word addresses are valid. Readback always returns address of **next** word to be used. Updated by Source X/Y increments during transfer. Access via word/long. Upper 9 bits of long write are ignored. |
| $FF8A28 | RW | 16-bit | EndMask 1 | M15-M0 | Mask for the **first** word of each destination line. Bits set to 1 are written; bits set to 0 are preserved. Byte access invalid. |
| $FF8A2A | RW | 16-bit | EndMask 2 | M15-M0 | Mask for all **middle** words of each destination line (words 2 through N-1). Byte access invalid. |
| $FF8A2C | RW | 16-bit | EndMask 3 | M15-M0 | Mask for the **last** word of each destination line. Byte access invalid. For a 1-word-wide destination, EndMask 1 is used for the single word. |
| $FF8A2E | RW | 16-bit | Destination X Increment | D14-D0 | Signed 15-bit value (LSB ignored). Offset in **bytes** to next destination word in the current line. Sign-extended and added to Destination Address after each destination word write. Byte access invalid. |
| $FF8A30 | RW | 16-bit | Destination Y Increment | D14-D0 | Signed 15-bit value (LSB ignored). Offset in **bytes** to first destination word of **next line**. Sign-extended and added to Destination Address after last word of each line. Byte access invalid. |
| $FF8A32-$FF8A34 | RW | 24-bit (3 bytes) | Destination Address | A22-A0 | Current destination field address (23-bit physical address). Readback returns address of **next** word to be modified. Updated by Destination X/Y increments. Access via word/long. |
| $FF8A36 | RW | 16-bit | X Count | X15-X0 | Number of **words** in one destination line. Range: 1-65536 (0 means 65536). Readback returns **remaining** words in current line (auto-decrementing counter), not the initial value. Byte access invalid. |
| $FF8A38 | RW | 16-bit | Y Count | Y15-Y0 | Number of **lines** in the destination. Range: 1-65536 (0 means 65536). Readback returns **remaining** lines (auto-decrementing). Byte access invalid. Reaches 0 when transfer completes. |
| $FF8A3A | R/W | 8-bit | HOP (Halftone Operation) | - | Bits 1-0: source/halftone combination rule. Bit 7: reserved. `00` = all ones (halftone ignored), `01` = halftone only, `10` = source only, `11` = source AND halftone. |
| $FF8A3B | RW | 8-bit | OP (Logic Operation) | - | Bits 3-0: Boolean logic combining source and destination. `0`=zero, `1`=SRC&DST, `2`=SRC&~DST, `3`=SRC, `4`=~SRC&DST, `5`=DST, `6`=SRC XOR DST, `7`=SRC\|DST, `8`=~SRC&~DST, `9`=~SRC XOR DST, `A`=~DST, `B`=SRC\|~DST, `C`=~SRC, `D`=~SRC\|DST, `E`=~SRC\|~DST, `F`=all ones. Upper 4 bits read as 0. |
| $FF8A3C | RW | 8-bit | Line Number / Status | - | Bits 3-0: Current halftone pattern index (0-15). Address = FF8A00 + (value x 2). Auto-incremented/decremented each line. Bit 5: SMUDGE -- when set, uses least-significant 4 bits of skewed source data as halftone address. Bits 7-6: BUSY (bit 7) and HOG (bit 6). |
| $FF8A3D | RW | 8-bit | Skew / Status | - | Bits 3-0: Source skew (0-15 bits right shift). Bit 6: NFSR (No Final Source Read) -- when set, suppresses last source read of each line. Bit 7: FXSR (Force eXtra Source Read) -- when set, forces extra source read at start of each line to prime source buffer. |

**Register layout diagram:**

```
Base: $FF8A00

Offset     Name                    Access    Size
------+----------------------------+---------+------
 $00  | Halftone RAM word 0      | R/W     | 16-b
 $02  | Halftone RAM word 1      | R/W     | 16-b
 $04  | Halftone RAM word 2      | R/W     | 16-b
 $06  | Halftone RAM word 3      | R/W     | 16-b
 $08  | Halftone RAM word 4      | R/W     | 16-b
 $0A  | Halftone RAM word 5      | R/W     | 16-b
 $0C  | Halftone RAM word 6      | R/W     | 16-b
 $0E  | Halftone RAM word 7      | R/W     | 16-b
 $10  | (unused/reserved)        |      -  |   --
 $12  | (unused/reserved)        |      -  |   --
 $20  | Source X Increment       | R/W     | 16-b
 $22  | Source Y Increment       | R/W     | 16-b
 $24  | Source Address (lo)      | R/W     | 16-b
 $26  | Source Address (hi byte) | R/W     |  8-b
 $28  | EndMask 1              | R/W     | 16-b
 $2A  | EndMask 2              | R/W     | 16-b
 $2C  | EndMask 3              | R/W     | 16-b
 $2E  | Destination X Increment  | R/W     | 16-b
 $30  | Destination Y Increment  | R/W     | 16-b
 $32  | Destination Address (lo) | R/W     | 16-b
 $34  | Destination Address (hi) | R/W     |  8-b
 $36  | X Count                | R/W     | 16-b
 $38  | Y Count                | R/W     | 16-b
 $3A  | HOP                    | R/W     |  8-b
 $3B  | OP                     | R/W     |  8-b
 $3C  | Line Num / SMUDGE / HOG/BUSY | R/W | 8-b
 $3D  | Skew / NFSR / FXSR   | R/W     |  8-b
------+----------------------------+---------+------
```

### 2.2 Control Bit Field Details

**$FF8A3C (Line Number / Control):**
```
 Bit  7  6  5  4  3  2  1  0
+---+--+--+--+-------------+
|BY |HO |SM | LINENUM       |
+---+--+--+--+-------------+
   |  |  |
   |  |  +-- SMUDGE: (bit 5) Use skewed source LSBs as halftone addr
   |  +----- HOG:      (bit 6) 0=shared bus, 1=hog mode (CPU halted)
   +-------- BUSY:     (bit 7) 0=idle, 1=transfer in progress
```

**$FF8A3D (Skew / Control):**
```
 Bit  7  6  5  4  3  2  1  0
+-----+-----+---+-----------+
|FXSR |NFSR |RS | SKEW      |
+-----+-----+---+-----------+
         |
         +-- RS: reserved, always reads 0
```

## 3. Blitter Commands and Operations

### 3.1 Operation Types

The STe blitter supports the following fundamental operation categories:

#### 3.1.1 Move (Bit-Block Transfer)

The basic operation: copy a rectangle of bits from source to destination with optional logic combination.

```
Setup:
  Source X/Y Increment  -> define source stride
  Source Address        -> define source origin
  Dest X/Y Increment   -> define dest stride
  Dest Address          -> define dest origin
  X Count, Y Count     -> define rectangle size
  EndMask 1/2/3       -> define left/center/right masks
  Op (OP register)    -> select logic function
  Skew, FXSR, NFSR    -> define bit alignment

Start:
  Set BUSY bit (bit 7 of $FF8A3C)

Result:
  For each line (Y Count times):
    For each word in line (X Count times):
      Read source word
      Skew source word right by SKEW bits
      Apply halftone operation (HOP)
      Apply logic operation with destination
      Apply current EndMask
      Write to destination
      Increment destinations
      Re-prime source buffer if needed (FXSR/end-of-line)
```

#### 3.1.2 Solid Fill

Fill destination with all zeros or all ones (no source data involved).

```
OP = $0  -> fill with all zeros
OP = $F  -> fill with all ones
EndMask = $FFFF (all bits written)
HOP = $0 (ignores halftone and source)
Source X/Y Increment irrelevant
```

#### 3.1.3 Halftone/Pattern Operation

Use the 8-word halftone RAM as the data source for a repeating 16-line pattern.

```
Setup:
  Load halftone RAM with pattern data (8 words)
  HOP = $01 (halftone only) or $03 (source AND halftone)
  Line Number -> which halftone word for current line
  OP -> logic operation to combine halftone with destination
  Destination X/Y Increment for single-word line stride (or more)
```

#### 3.1.4 SMUDGE Mode

Uses skewed source data's least-significant 4 bits as an index into halftone RAM:

```
SMUDGE (bit 5 of $FF8A3C) = 1
  Skewed source LSB[3:0] -> halftone RAM address (0-15)
  Halftone pattern at that address -> used in HOP operation
  Enables indexed/fuzzy halftone effects
  Useful for dithering, texture mapping, etc.
```

### 3.2 Logic Operations (OP Register)

```
OP = $0 : Result = 0x0000         ; Clear
OP = $1 : Result = SRC & DST      ; AND
OP = $2 : Result = SRC & ~DST     ; AND NOT
OP = $3 : Result = SRC            ; COPY SOURCE
OP = $4 : Result = ~SRC & DST     ; OR NOT (inverted)
OP = $5 : Result = DST            ; KEEP DESTINATION
OP = $6 : Result = SRC ^ DST      ; XOR
OP = $7 : Result = SRC | DST      ; OR
OP = $8 : Result = ~SRC & ~DST    ; NOR
OP = $9 : Result = ~SRC ^ DST     ; XNOR (if DST inverted)
OP = $A : Result = ~DST           ; NOT DESTINATION
OP = $B : Result = SRC | ~DST     ; OR NOT (alternate)
OP = $C : Result = ~SRC           ; NOT SOURCE
OP = $D : Result = ~SRC | DST     ; NAND
OP = $E : Result = ~SRC | ~DST    ; NAND (alternate)
OP = $F : Result = 0xFFFF         ; Set (fill ones)
```

### 3.3 Halftone Operation (HOP Register)

```
HOP[1:0] = $0 : Halftone = 0xFFFF (all ones, source passes through)
HOP[1:0] = $1 : Halftone only (halftone RAM data is used)
HOP[1:0] = $2 : Source only (halftone RAM ignored, source passes through)
HOP[1:0] = $3 : Halftone AND source (bitwise AND)
```

### 3.4 Source/Destination Addressing

Both source and destination addresses are 23-bit physical addresses. The blitter operates on **word (16-bit) granularity**.

- **Source Address** and **Destination Address** are 24-bit registers (the upper byte occupies the next odd address)
- **Source X/Y Increment** and **Destination X/Y Increment** are signed 15-bit byte offsets
- The LSB of each increment register is ignored (blitter always operates on even/word boundaries)
- **X Count** specifies the number of words per row
- **Y Count** specifies the number of rows
- At end of each line, the Source/Destination address is adjusted by Y-Increment
- During each line, each word write adjusts the address by X-Increment

### 3.5 Bit Alignment and Skewing

The blitter handles sub-word alignment through its skew mechanism:

```
Skew register (bits 3:0): 0-15 bit right shift of source data

FXSR (bit 7 of $FF8A3D): "Force eXtra Source Read"
  When set: an extra source word is read at the start of each line
  This primes the 32-bit source buffer for proper skew alignment

NFSR (bit 6 of $FF8A3D): "No Final Source Read"
  When set: the last source read of each line is suppressed
  The lower-to-upper half buffer transfer still occurs at line end
  Reduces unnecessary memory reads for certain mask/skew combinations
```

**Source buffer (internal 32-bit):**

```
Normal scan direction (left to right):
  [ Lower 16 bits | Upper 16 bits ]
       <- skew-shifted     <- holds remaining bits

Right-to-left scan (overlapping transfers):
  Buffer is drained from the upper half first
  Then the lower half is promoted to upper
  Then the next source word fills the lower half
```

### 3.6 Bus Access Control

```
HOG bit (bit 6 of $FF8A3C):
  $0 (cleared) = SHARED mode
    CPU and blitter each get 64 bus cycles, then the other gets 64
    CPU can execute and service interrupts in its 64-cycle windows
    Slower for blit execution but non-blocking to the CPU

  $1 (set) = HOG mode
    CPU is halted entirely until the blit transfer completes
    Blitter gets all bus access until done
    Fastest blit execution but blocks all CPU work

BUSY bit (bit 7 of $FF8A3C) = START COMMAND
  After all registers are set, write 1 to this bit to start the transfer
  Bit stays 1 while blitter is active
  Write 0 to stop (halt) the transfer
  The interrupt line is connected to BUSY
```

## 4. GST-Level Integration

### 4.1 Bus Architecture

The blitter is a **Bus Master** on the 68000 system bus. From the perspective of any bus device, the blitter appears as another bus master identical to the 68000 CPU:

```
+--------------+     +----------+
|  68000 CPU   |<--->| GST MCU  |<--> Address Bus / Data Bus / Control
+--------------+     | (GSTMCU) |       (16 data, 24 address lines)
                     |          |
                     |  Blitter |
                     | (DMA)    |
                     +----------+
                           |
                     +----------+
                     |   RAM    |
                     +----------+
```

The GST MCU contains:

- **Bus arbiter** -- determines which bus master (68000 or BLiTTER) controls the bus at any time
- **Address decoder** -- routes transactions to correct memory regions
- **MMU** -- handles DRAM refresh and address translation for scrolling
- **Blitter control logic** -- holds the blitter registers and blit-state machine

### 4.2 Bus Arbitration

When the blitter is active, the GST MCU arbitrates bus access between the 68000 and the blitter:

#### Shared Mode (HOG = 0, default):

```
Time:    [ CPU:64 cycles ][ BLiTTER:64 cycles ][ CPU:64 cycles ][ BLiTTER:64 cycles ] ...
          |<-- CPU runs, services interrupts -->|<- blit operations (reads + writes)   ->| ...

Behavior:
  - Each master gets 64 consecutive bus cycles
  - 68000 can execute instructions and service interrupts during its time slice
  - Blitter executes its memory operations during its time slice
  - Transfer takes about 2x wall-clock time vs. HOG mode
  - Non-blocking: CPU continues running (with 64-cycle latency)
  - Interrupt latency: worst case ~64 bus cycles for blitter slice
```

The blitter can also release the bus prematurely:

```
Optimized restart technique:
  CPU restarts blitter (clears then sets BUSY) after just 7 bus cycles
  instead of waiting the full 64-cycle share
  Interrupts may be serviced before restart code regains control
  Achieves ~90% of HOG-mode performance while retaining interrupt handling
  Key technique for game demo programmers
```

#### HOG Mode (HOG = 1):

```
Time:    [ CPU: halted ]======[ BLiTTER: full transfer ]====[ CPU: resumes ]
          | CPU cannot execute |<- exclusive bus access for blitter ->|
          
Behavior:
  - 68000 is fully halted (bus cycles = 0, no execution)
  - Blitter has exclusive access to all memory
  - Fastest possible transfer time
  - No interrupts possible during transfer
  - Risk: any instruction following HOG set may not execute "immediately"
```

### 4.3 Stall Behavior

**68000 Stalls When:**

- Blitter is active and has bus ownership (its 64-cycle slice in shared mode, or entire transfer in HOG mode)
- The 68000 attempts a bus cycle during blitter's time slice
- GST MCU issues a stall signal (HSNAIL/HLDA equivalent) to the 68000
- The 68000 enters wait states until bus ownership transfers back

**Blitter Yielding:**

- The blitter always yields to other DMA devices (e.g., FDC disk controller)
- GST MCU arbitration gives highest priority to DMA requests outside CPU/blitter
- The blitter can be preempted by the Falcon IDE or other DMA if asserted

**Register Write During Transfer:**

```
Attempting to write blitter registers during an active transfer:
  - Registers at $FF8A00-$FF8A35 (all data/address/count/operation registers)
    can be read/written mid-transfer to set up THE NEXT blit
    (this is how the "premature restart" technique works)
  - However, you must NOT call VDI or LINE A routines from within
    an interrupt context, as unpredictable results occur if blits nest
  - The BUSY bit protects against re-entry
```

### 4.4 Priority Summary

Bus master priority (highest to lowest):

```
1.  Other DMA devices (FDC, GST Shifter video/sound DMA)
    - These can preempt both CPU and blitter at any time
    
2.  Bus Master Arbitration (CPU vs. Blitter)
    Shared mode: round-robin 64-cycle time slices
    HOG mode:   blitter has absolute priority until transfer complete
    
3.  68000 CPU
    Default bus master (blitted mode if active, normal if not)
```

## 5. Data Path

### 5.1 Block Transfer Diagram

```
                    HALFTONE RAM (8 words)
                           |
                  +--------v--------+
                  |  HOP MUX        |
                  | (select: all-1,  |
                  |  halftone, source,|
                  |  src&hlto)       |
                  +--------+--------+
                           |
                  +--------v--------+
                  |  XOR / SKEW      |
                  |  logic unit      |
                  +--------+--------+
                           |
                    SKEW MUX
                   (shift 0-15 bits)
                           |
                  +--------v--------+
                  |  LOGIC OP ALU    |
                  | (16 possible     |
                  |  SRC/DST ops)    |
                  +--------+--------+
                           |
                    ENDMASK MUX
                 (EndMask 1/2/3 select)
                           |
                  +--------v--------+
                  |  DEST WRITE      |
                  |  (with mask)     |
                  +--------+--------+
                           |
                    DESTINATION MEMORY
                  (via GST MCU bus)

Source flow:
  SOURCE MEMORY -> SOURCE LATCH -> 32-BIT BUFFER -> SKEW -> HOP ALU

Control inputs:
  OP register   -> selects logic operation among 16
  HOP register  -> selects halftone operation
  Line Number   -> selects current halftone word
  Skew register -> selects shift amount (0-15)
  SMUDGE bit    -> uses skewed source LSB[3:0] as halftone addr
  EndMask reg   -> controls which destination bits to update
  BUSY / HOG    -> start / bus mode control
  X/Y Count     -> 2D loop counters (auto-decrement)
  X/Y Inc (src/dst) -> address increments
```

### 5.2 Detailed Data Path

```
              +-------------------------------+
              |    BLOCK TRANSFER DATA PATH    |
              +-------------------------------+

 SOURCE MEM                SOURCE LATCH
 (via bus)           +---->|  16-bit latch     |
                     |     +---------+---------+
                     |               |
                      -----> 32-BIT BUFFER
                                  |  (shift-right as data streams in)
                     +------------|------------+
                     | lower 16 | upper 16 |
                     +------------|------------+
                                  |
                           SKEW SHIFT
                         (0-15 bits Right)
                                  |
                           +------v------+
                           |  HOP MUX    |
                           | HOP[1:0]:   |
                           |   00=all-1  |
                           |   01=hlto   |
                           |   10=source |
                           |   11=src&hlto|
                           +------+-------+
                                  |
                              HALFTONE RAM
                                (8 words)
                       +----------+--------+
                       |  (when SMUDGE=0)  |
                       | Line Num -> addr  |
                       +-------------------+
                       |  (when SMUDGE=1)  |
                       | skd SRC LSB[3:0]  |
                       +----------+--------+
                                  |
                           +------v-------+
                           | LOGIC ALU    |
                           | OP[3:0]:     |
                           |   00=0000     |
                           |   01=SRC&DST  |
                           |   ...=16 ops  |
                           |   FF=FFFF     |
                           +------+--------+
                                  |
                           +------v-------+
                           | ENDMASK MUX  |
                           | 1st word: #1 |
                           | mid words: #2 |
                           | last word:#3  |
                           +------+--------+
                                  |
                           +------v-------+
                           | WRITE to DEST|
                           | MEM (via bus) |
                           +--------------+

 ADDRESS GENERATION:
   SRC_ADDR += SRC_X_INC  (per word)
   SRC_ADDR += SRC_Y_INC  (per line)
   DST_ADDR += DST_X_INC  (per word)
   DST_ADDR += DST_Y_INC  (per line)
   X_COUNT decrements per word
   Y_COUNT decrements per line
   LINE_NUM increments/decrements per line
```

### 5.3 Cycle Counts and Performance

```
Approximate blitter memory access cycles per word:
  Read source word:     1 cycle
  Write dest word:      1 cycle
  (possibly +1 for mask read-modify-write if endmask has zeros)

In shared mode, effective throughput:
  ~1 word per 2 bus cycles (one for blitter slice, one for CPU slice)
  At 8 MHz: ~4 Mwords/sec effective
  At 16 MHz (MegaSTE): ~8 Mwords/sec effective

In HOG mode:
  ~1 word per 1 bus cycle
  At 8 MHz: ~8 Mwords/sec
  At 16 MHz (MegaSTE): ~16 Mwords/sec

Compared to 68000 software BitBlt:
  68000 (8 MHz): ~0.1-0.2 Mwords/sec for typical blit
  68000 (16 MHz MegaSTE): ~0.3-0.5 Mwords/sec
  
Blitter speedup factor: approximately 20-80x over 68000 software
```

### 5.4 Comparison: 68000 vs Blitter

```
+-----------------------------------+------------+------------+
| Metric                            | 68000 CPU  | Blitter    |
+-----------------------------------+------------+------------+
| Operation type                    | Software   | Hardware   |
| Max throughput (8 MHz STe)        | ~0.2 Mw/s  | ~8 Mw/s    |
| Max throughput (16 MHz MegaSTE)   | ~0.5 Mw/s  | ~16 Mw/s   |
| Simultaneous CPU execution        | N/A        | Shared mode|
| Boolean logic                    | Manual op  | 16 ops HW  |
| Skewing/bit-shift                 | Manual     | Hardware   |
| Pattern/halftone                  | Manual     | 8-word HW  |
| Endmask for clipping              | Manual     | 3 masks HW |
| Typical fill 320x200 1-plane      | ~300 ms    | ~8 ms      |
| Typical fill 640x512 4-plane      | ~2000 ms   | ~64 ms     |
| Interrupt handling                | Native     | Shared mode|
| Memory bandwidth share            | 100%       | 50% (shared)|
| HOG mode CPU blocking             | N/A        | 100% blocked|
+-----------------------------------+------------+------------+
```

## 6. Emulation Notes

### 6.1 MiSTer/atariST_MiSTer Implementation

The most accurate blitter emulation is by **Jorge Cwik (CrunC)** in the [AtariST_MiSTer](https://github.com/MiSTer-devel/AtariST_MiSTer) project. Key points:

- **Cycle-accurate blitter** -- each bus access is simulated at the correct clock cycle
- The blitter registers are implemented as a memory-mapped block at $FF8A00
- Bus arbitration between 68000 and blitter uses 64-cycle time slices in shared mode
- HOG mode correctly halts the 68000 for the duration of the transfer
- The blitter register write-back behavior matches hardware: reading X Count / Y Count returns the *current counter value*, not the initial value

### 6.2 Hatari Emulator

Hatari provides **functional** blitter emulation (not cycle-accurate):

- Registers and data path are correctly implemented
- Bus sharing is simulated at a higher level (not cycle-accurate)
- HOG mode is approximated rather than perfectly modeled
- Sufficient for running software that uses the blitter
- Some timing-sensitive demo/demo scene code may not work correctly

### 6.3 gstmcu Verilog Model

The [gyurco/gstmcu](https://github.com/gyurco/gstmcu) repository contains a Verilog model of the GST MCU (including blitter logic):

- Includes gate-level circuits with asynchronous clocking
- Also provides a synchronous model suitable for FPGA synthesis
- Based on recovered schematics from Christian Zietz (chzsoft.de/asic-web)
- Combined with Jorge Cwik's Shifter implementation for complete video emulation
- The blitter is integrated into the GST MCU block, not as a separate submodule

### 6.4 Emulation Challenges

When emulating the STe blitter:

1. **Bus timing** -- The 64-cycle arbitration is the hardest part to get right. Premature restart (clear-then-set BUSY after ~7 cycles) is critical for correct behavior of some software.

2. **Register read-back** -- X Count and Y Count return their *current* values during a transfer, not the initial loaded values. This is used by software that polls these registers.

3. **BUSY/interrupt coupling** -- The blitter interrupt line is directly wired to the BUSY bit. Clearing BUSY in HOG mode stops the transfer and fires the interrupt.

4. **Source buffer priming** -- The FXSR/NFSR logic controls a 32-bit source buffer that shifts data across line boundaries. This buffer state must be correctly managed for skewed/sub-word blits.

5. **Scan direction for overlapping transfers** -- When source and destination blocks overlap, the scan direction may reverse (bottom-right to top-left when source >= dest address). This affects source buffer behavior.

6. **EndMask handling** -- A read-modify-write cycle is only issued when the current EndMask has zero bits. If all bits are 1, a write-only cycle is used. For the last word of each line, if NFSR is set, a read-modify-write is forced regardless of EndMask.

7. **Address alignment** -- The blitter only handles word-aligned addresses. Byte-granularity source/destination addresses in software will cause incorrect results.

8. **HALFTONE RAM is not self-updating** -- Software must load all 8 words of halftone RAM before starting a blit. The hardware does not automatically repeat patterns; it simply wraps the Line Number index at 0-15.

9. **SMUDGE mode interaction** -- When SMUDGE is set, the halftone address comes from the skewed source data's LSB[3:0], not from the Line Number register. This means the effective halftone pattern can vary per word within a line.

10. **MegaSTE cache interaction** -- The MegaSTE's instruction/data cache does not directly cache blitter memory accesses. Cache behavior during blit operations in HOG mode depends on the specific cache configuration and whether any code runs during the blit.

### 6.5 Common Bugs to Watch For in Emulation

```
Bug: Emulator starts blit before BUSY is actually set
Fix: Delay blit activation by the exact number of cycles it takes to latch BUSY

Bug: BUSY bit stays 0 after transfer completes
Fix: BUSY must remain 1 from first write until transfer completes (Y Count reaches 0)

Bug: Halftone pattern repeats every 8 lines instead of 16
Fix: Line Number wraps at 0xF back to 0x0 (16-line cycle, not 8)

Bug: EndMask 1 not used for single-word-wide rectangles
Fix: For X Count == 1, always use EndMask 1 for the single word

Bug: Source Y-Increment not applied when X Count == 1
Fix: Source Y-Increment is ALWAYS applied (even when X Count == 1),
     but Source X-Increment is only applied when X Count > 1

Bug: NFSR suppresses write to last destination word
Fix: NFSR only suppresses the SOURCE READ. The destination write still occurs,
     but with a forced read-modify-write.

Bug: Blitter HOG mode doesn't halt 68000 correctly
Fix: 68000 must be stalled (entered wait states) for the entire HOG-mode transfer,
     not just during blitter memory accesses
```
