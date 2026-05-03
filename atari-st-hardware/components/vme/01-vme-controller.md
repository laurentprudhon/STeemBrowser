# VME Controller

> The VME controller on the Atari Mega STE provides a VMEbus-compatible expansion interface on the bottom edge connector. It is integrated into the GST MCU (C302183) and the SH2 (C301842) chip.

## Overview of VMEbus in Atari MegaSTE

The VME (VME68) bus was a computer bus standard originally developed by Motorola in 1984 for modular embedded computer systems. It was widely used in professional and industrial applications including telecommunications equipment, test instruments, and military systems.

Atari included a VMEbus-compatible expansion interface **only on the Mega STE** (1990); it was **not available** on the original ST, STe, or Mega ST models. The VME slot on the Mega STE is a 96-pin edge connector on the bottom of the board, accessible from a VME expansion daughter board.

### Purpose in the Atari Ecosystem

- **Professional expansion**: The Mega STE targeted professionals who needed rack-mountable (19-inch VME subrack) systems with standard computer functionality
- **Industrial interoperability**: VMEbus was standard in industrial instrumentation and telecom; the Mega STE could slot directly into VME backplanes
- **High-speed DMA**: VMEbus supports efficient DMA transfers between the Mega STE and expansion cards
- **Multi-master capability**: VMEbus technically supports multiple bus masters, though the Mega STE implementation primarily acts as a single master with slave device support

### Availability

| Model | VME Support | Notes |
|-------|------------|-------|
| 520ST / 1040ST | No | Original ST, no VME bus |
| Mega ST | No | Desktop variant, no VME bus |
| 520STE | No | STe had expanded I/O but no VME |
| 1040STE | No | Same as 520STE |
| Mega STE | **Yes** | 96-pin VME C.1 sub-slot on bottom edge |
| TT030 | Yes | Separate VME controller chip (VME bus, A32/D32) |
| Falcon030 | No | Used different expansion architecture (ST Bus / DSP) |

## VMEbus Protocol Basics

### VME68 Standard

The Mega STE VME interface implements a subset of the **VME68** standard (VMEbus for Motorola 68000 processors). Key aspects:

| Aspect | Mega STE Implementation |
|--------|----------------------|
| Bus standard | VME68 subset (C.1 sub-slot) |
| Address width | A16 and A24 addressing modes |
| Data width | D16 (16-bit data bus) |
| Clock | System clock derived (8 MHz / 16 MHz) |
| Bus arbitration | Internal to GST MCU (no external arbitration) |
| Transfer types | Burst, single-word, swap |

### VME C.1 Sub-Slot

The Mega STE uses the **VME C.1** sub-slot form factor. C.1 defines the physical/mechanical specifications for a sub-card that plugs into a primary card:

- 96-pin edge connector
- Designed to mate with a primary VME board in a backplane
- The Mega STE's VME slot is on the bottom edge; expansion cards plug in from the bottom
- Maximum sub-slot board size: 300 mm x ~100 mm (varies by primary card)
- Power: +5V and +12V on the VME backplane

### VME Bus Signals (Mega STE)

| Signal | Direction | Description |
|--------|-----------|-------------|
| **Address lines** | Output | A1 and A14 used for address decoding (internal GST MCU maps full 32-bit) |
| **DACK1-DACK7** | Output | Device acknowledge lines (active low) |
| **DS0-DS3** | Output | Data strobe lines for data width selection |
| **EOP** | Output | End of Phase (burst transfer delimiter) |
| **I/O Cherry** | Input/Output | I/O space select |
| **Master/Slave** | Input | Bus master/slave mode select |
| **MA1-MA3** | Input | Mode A/B select for cycle type |
| **PAR** | Input/Output | Parity (19-bit parity for D32, unused on D16) |
| **RST** | Input | Reset (active low) |
| **VME IRQ 1-7** | Input | VME interrupt requests (7 vector levels) |

### VME Address Channels

VME defines four address channels with different widths. The Mega STE controller can be configured for:

| Channel | Address Width | Data Width | Address Space | Physical Address Range |
|---------|--------------|------------|---------------|----------------------|
| **A8** | 8 bits | D8/D16 | 256 bytes | Not used on Mega STE |
| **A16** | 16 bits | D16 | 64 KB | `$FFFE0000`-`$FFFEFFFF` |
| **A24** | 24 bits | D16 | 16 MB | `$FE000000`-`$FEFFFFFF` |
| **A32** | 32 bits | D16/D32 | 4 GB | Not supported (Mega STE limitation) |

The `A24/D16` configuration is the primary VME mode for the Mega STE. In A24 mode, the 24 address bits are mapped to VME pins A0-A23. The upper 8 bits (A24-A31) are held at a fixed value controlled by the A24/A32 upper byte register (`$FF8E01`).

In A16 mode, addresses A0-A15 are mapped to VME pins, with upper address lines forced to the value in the A24/A32 upper byte register.

### VME Cycle Types

| Cycle Type | Description | VME Signals |
|-----------|-------------|------------|
| **Single-word read** | One address phase, one data phase | IACK, A/D, DS, R/W |
| **Single-word write** | One address phase, one data phase | IACK, A/D, DS, R/W |
| **Burst read** | One address phase, multiple data phases | IACK, A/D, DS, EOP delimiter |
| **Burst write** | One address phase, multiple data phases | IACK, A/D, DS, EOP delimiter |
| **Swap** | Alternating read/write | IACK, A/D, DS, R/W alternation |
| **FIFO** | Block transfer to FIFO | IACK, A/D, DS, R/W |

### VME Transfer Length Encoding

VME encodes transfer length in the A15-A21 address lines:

| Address Bits | Length |
|-------------|--------|
| `xxxxxxxxx` | 1 word (odd address) |
| `000xxxxxx` | 2 words (even address) |
| `001xxxxxx` | 4 words |
| `01xxxxxx` | 8 words |
| `10xxxxxx` | 16 words |
| `11xxxxxx` | 32 words |
| `111xxxxxx` | 64 words |
| `1111xxxx` | 128 words |
| `11111xxx` | 256 words |
| `111111xx` | 512 words |
| `1111111x` | 1024 words |
| `11111111` | 4096 words |

## VME Controller Integration in GST MCU

### GST MCU Overview

The Mega STE contains three major chips for system control:

1. **GST MCU** (C302183-002) - Glue + MMU + Blitter + I/O + VME controller (144-pin SMT)
2. **SH2** (C301842) - 16 MHz clock generator + address decode (+20 pins, 144-pin SMT)
3. **GST Shifter** (C029145) - Shifter + Blitter + Audio (144-pin SMT)

### VME Controller in GST MCU and SH2

The VME bus controller is split between two chips: the GST MCU handles register interfaces and interrupt mapping, while the SH2 chip handles 16 MHz clock generation and additional address decode for VME address translation.

### VME Controller Block Diagram within GST MCU / SH2

```
+------------------------------------------+
|           GST MCU (C302183)              |
|                                          |
|  +--------+    +--------+    +--------+  |
|  | Glue   |    | MMU    |    | VME    |  |
|  | (bus   |    | (DRAM  |    | Ctrlr  |  |
|  |  logic)|    | mgmt)  |    | unit)  |  |
|  +--------+    +--------+    +--------+  |
|      |           |            |    |      |
|      +-----------+------------+----+      |
|                  |            |           |
|  +--------+      |            |           |
|  | Blitter|      |            |           |
|  +--------+      |            |           |
|                  |            |           |
|  +--------+      |            |           |
|  | I/O &  |      |            |           |
|  | IRQ    |------|            |           |
|  |mux/dec |      |            |           |
|  +--------+      |            |           |
|                  |            |           |
|  +--------+      v            v           |
|  | Clock  |    +--------+    +------+    |
|  | (8/16  |---->| A24/   |    | VME  |    |
|  | MHz)   |    | upper  |    | I/F  |----+---> VMEbus
|  +--------+    | byte   |    +------+         connectors
|               | reg 5   |
|               +--------+
+------------------------------------------+
                    |
+-------------------+-----------------------+
|  SH2 (C301842)                              |
|                                             |
|  +--------+    +--------+    +--------+    |
|  |16 MHz  |    | 16/8   |    | Addnl  |    |
|  | clock  |----| MHz div|    | addr   |    |
|  | gen    |    | gen    |    | decode |    |
|  +--------+    +--------+    +--------+    |
|                                             |
|  +------------------+                       |
|  | VME A24 upper    |                       |
|  | byte register 8  |   (A24-A31 value)    |
|  +------------------+                       |
+----------------------------------------------+
```

### GST MCU Address Decoding for VME

In A24 mode, the 24-bit address is split:
- 16 bits (A0-A15) come directly from the 68000 address bus
- 8 bits (A16-A23) are provided by an internal address latch register

In A16 mode:
- The lower 16 bits (A0-A15) are passed through
- The upper 8 bits (A16-A23) are driven by an internal shift register that steps on each subsequent word in a burst cycle

The **A24/A32 upper address byte register** (mapped at `$FF8E00`) holds the value for A24-A31:

### VME Expansion Slot

The Mega STE VME slot is a **96-pin edge connector** on the bottom of the PCB. It is designed to receive a VME expansion daughter card. The connector pins include:

| Pin Group | Pins | Signal |
|-----------|------|--------|
| VME Address | 0-23 | A0-A23 |
| VME Data | 24-39 | D0-D15 (D16 bus) |
| VME Control | Various | R/W, DS0-DS3, EOP, IACK, DACK, PAR |
| VME Interrupt | Various | IRQ1-IRQ7 |
| VME Arbitration | Various | AM, BM, CA, CB |
| Power | A1, A2 | +5V, -5V |
| Power | B13, B15 | +12V, -12V |
| Ground | Various | GND |

The physical expansion card plugs in from the **bottom** of the Mega STE, making it accessible when the chassis is on its side.

## VME Register Map

The VME controller is memory-mapped at I/O address **$FF8E00-$FF8E0F** (8-bit access only). All registers are spaced 2 bytes apart (odd addresses used for control access).

### Register Table

| Address (I/O) | Name | Size | R/W | Description |
|---------------|------|------|-----|-------------|
| `$FF8E00` | VME address upper 8 bits (A24-A31) | 1 byte | R/W | A24/A32 upper byte for address channel configuration |
| `$FF8E01` | VME system control mask | 1 byte | R/W | Interrupt mask for system events |
| `$FF8E02` | (unused) | 1 byte | - | Padding (even address, not accessed) |
| `$FF8E03` | VME system control status | 1 byte | R | System control interrupt status |
| `$FF8E04` | (unused) | 1 byte | - | Padding (even address, not accessed) |
| `$FF8E05` | VME system interrupt force | 1 byte | R/W | Forces level 1 system interrupt |
| `$FF8E06` | (unused) | 1 byte | - | Padding (even address, not accessed) |
| `$FF8E07` | VME interrupt force (TT only) | 1 byte | R/W | Forces level 3 VME interrupt (TT only) |
| `$FF8E08` | (unused) | 1 byte | - | Padding (even address, not accessed) |
| `$FF8E09` | VME general purpose reg | 1 byte | R/W | General purpose register (TT only, does nothing on Mega STE) |
| `$FF8E0A` | (unused) | 1 byte | - | Padding (even address, not accessed) |
| `$FF8E0B` | VME general purpose reg | 1 byte | R/W | General purpose register (TT only, does nothing on Mega STE) |
| `$FF8E0C` | VME interrupt mask | 1 byte | R/W | VME interrupt request mask |
| `$FF8E0D` | VME interrupt mask (alt) | 1 byte | R/W | VME interrupt request mask |
| `$FF8E0E` | VME interrupt status | 1 byte | R | VME interrupt request status |
| `$FF8E0F` | VME interrupt status (alt) | 1 byte | R | VME interrupt request status |

### Bit Field Decoding

#### VME Address Upper 8 Bits Register (`$FF8E00`)

Holds the upper 8 address bits (A24-A31) for VME address channel configuration.

```
Bits  | Role
------|------------
7-0   | A24-A31 address upper byte
```

#### VME System Control Mask Register (`$FF8E01`)

Configures which system interrupts are routed through the VME controller:

```
Bit | Name     | Description                                    | R/W
----|----------|------------------------------------------------|----
7   | SYSFAIL  | VME system failure (bus timeout, parity err)     R/W
6   | MFP      | MFP integrated timer/counter interrupt         R/W
5   | SCC      | SCC (Serial Communications Controller) int.    R/W
4   | VSYNC    | Vertical sync interrupt                        R/W
3   | HSYNC    | Horizontal sync interrupt                       R/W
2   | -        | Unassigned                                      R/W
1   | -        | Unassigned                                      R/W
0   | SYSINT   | System software interrupt (bit 0 of $FF8E05)  R/W
```

Reading `sys_mask` resets pending interrupt bits in `sys_stat`.

#### VME System Control Status Register (`$FF8E03`)

Read-only status of system control interrupts:

```
Bit | Name      | Description                                    | Behavior
----|-----------|------------------------------------------------|---------
7   | SYSFAIL   | VME system failure detected                      | Set by hardware
6   | MFP       | MFP timer/counter interrupt pending              | Auto-vect $66
5   | SCC       | SCC interrupt pending                            | Auto-vect $66
4   | VSYNC     | VSYNC interrupt pending                          | Programmed
3   | HSYNC     | HSYNC interrupt pending                          | Programmed
2   | -         | Unassigned                                       | -
1   | -         | Unassigned                                       | -
0   | SYSINT    | System software interrupt pending                | Programmed
```

#### VME System Interrupt Force Register (`$FF8E05`)

Writing 1 to bit 0 forces a level 1 interrupt:

```
Bit | Name    | Description                                  | R/W
----|---------|----------------------------------------------|----
0   | SYSINTF | Set to 1 to force level 1 interrupt        R/W
1-7 | -       | Unassigned                                   R/W
```

Forced interrupt uses auto-vector address **$000064**. Writing 1 forces an interrupt; writing 0 has no effect. The bit must first be enabled in `sys_mask` to be functional.

#### VME Interrupt Mask Register (`$FF8E0C` / `$FF8E0D`)

Configures which VME bus interrupt requests are routed to the CPU:

```
Bit | IRQ Level | Source                                  | R/W
----|-----------|-----------------------------------------|----
7   | IRQ 7     | VME bus IRQ 7 (highest priority)        R/W
6   | IRQ 6     | VME bus IRQ 6 (shared with MFP)        R/W
5   | IRQ 5     | VME bus IRQ 5 (shared with SCC)        R/W
4   | IRQ 4     | VME bus IRQ 4                           R/W
3   | IRQ 3     | VME bus IRQ 3 (software configurable)  R/W
2   | IRQ 2     | VME bus IRQ 2                           R/W
1   | IRQ 1     | VME bus IRQ 1 (lowest VME priority)    R/W
0   | -         | Unassigned/unimplemented               R/W
```

#### VME Interrupt Status Register (`$FF8E0E` / `$FF8E0F`)

Read-only status of VME interrupt requests:

```
Bit | IRQ Level | Description                              | Behavior
----|-----------|------------------------------------------|---------
7   | IRQ 7     | VME IRQ 7 pending                        | Set by hardware
6   | IRQ 6     | VME IRQ 6 pending                        | Set by hardware
5   | IRQ 5     | VME IRQ 5 pending                        | Set by hardware
4   | IRQ 4     | VME IRQ 4 pending                        | Set by hardware
3   | IRQ 3     | VME IRQ 3 pending                        | Set by hardware
2   | IRQ 2     | VME IRQ 2 pending                        | Set by hardware
1   | IRQ 1     | VME IRQ 1 pending                        | Set by hardware
0   | -         | Status bit 0                             | -
```

**Important**: Reading `vme_mask` clears (resets) the corresponding pending interrupt bits in `vme_stat`. To correctly read the status, read `vme_stat` first, then read `vme_mask`.

### VME Memory-Mapped Address Spaces

| Memory Range | VME Mode | Data Width | Description |
|-------------|----------|-----------|-------------|
| `$FE000000`-`$FEFFFFFF` | A24/D16 | 16-bit | VME board address space (16 MB) |
| `$FFFE0000`-`$FFEFFFFF` | A16/D16 | 16-bit | VME board address space (64 KB) |
| `$FF000000`-`$FFFFFFFF` | A24 shadow | 16/32-bit | 24-bit compatible shadow space |
| `$80000000`-`$BFFFFFFF` | A32 shadow | 32-bit | 32-bit shadow space |
| `$FF8E00`-`$FF8EFF` (I/O) | - | - | VME controller registers |

### VME Address Channel Configuration Diagram

```
VME Address Channel Configuration
=================================

A24 Mode (primary VME mode):
  A24-A31: Set by $FF8E00 register
  A23-A16: Set by internal A-shift register (steps on each word access)
  A15-A0:  Direct from 68000 address bus
  
  $FF8E00 reg:  [ A31 | A30 | A29 | A28 | A27 | A26 | A25 | A24 ]

A16 Mode:
  A23-A16: Set by internal A-shift register
  A15-A0:  Set by internal A-shift register
  Upper addr lines: fixed from $FF8E00
  
  Both address bytes are stepped on successive word accesses.

Memory Mapping:
  A24/D16 -> $FE000000 - $FEFFFFFF  (16 MB VME address space)
  A16/D16 -> $FFFE0000 - $FFEFFFFF  (64 KB VME address space)
  A24 Shadow -> $FF000000 - $FFFFFFFF (24-bit compat mirror)
```

## VME Data Widths

### Supported Data Widths

| Data Width | VME Designation | Mega STE Support | Notes |
|-----------|----------------|-----------------|-------|
| 8-bit | D8 | No | Not supported on Mega STE VME |
| 16-bit | D16 | **Yes** | Primary data width for all VME accesses |
| 32-bit | D32 | No | Not supported (A32/D32 is TT030 only) |

### Data Strobe Configuration

The VME bus uses data strobe lines (DS0-DS3) for byte enables:

| DS Line | Byte Lane | Data Width Usage |
|---------|-----------|-----------------|
| DS0 | Byte 0 (D0-D7) | Used in D16 mode |
| DS1 | Byte 1 (D8-D15) | Used in D16 mode |
| DS2 | Byte 2 (D16-D23) | Unused (D16 only) |
| DS3 | Byte 3 (D24-D31) | Unused (D16 only) |

In D16 mode on the Mega STE, only DS0 and DS1 are used. DS2 and DS3 are held inactive.

### Word Swap Mode

The Mega STE VME controller supports **Swap** transfer mode, which alternates read and write operations on successive data phases. This is useful for bidirectional FIFO transfers:

```
Swap Cycle Timing:
  Phase 1: A/D, DS0-DS1 active, R/W = Read  (Phase 1 data phase)
  Phase 2: A/D, DS0-DS1 active, R/W = Write  (Phase 2 data phase)
  ... alternates for N phases ...
  EOP: End of Phase delimiter
```

### Burst Mode

Burst mode transfers multiple words after a single address phase, with each successive word addressing consecutive VME addresses:

```
Burst Write Example:

Phase 1 (Address):
  A/D strobe low
  Address = start_address (A0-A23)
  IACK complete
  DS0-DS1 active (D16 mode)

Phase 2-N (Data):
  Data on D0-D15
  R/W = 0 (write)
  A/D strobe low on each data phase
  ...

Phase N+1 (End of Burst):
  EOP strobe
  IACK high (end of arbitration)
  A/D high
```

The burst length is encoded in the address bits A21-A15:

| A21 | A20 | A19 | Length (words) |
|-----|-----|-----|----------------|
| 0 | 0 | 0 | 2 |
| 0 | 0 | 1 | 4 |
| 0 | 1 | x | 8 |
| 1 | 0 | x | 16 |
| 1 | 1 | x | 32 |
| 1 | 1 | 1 | 64 |
| 1 | 1 | 1 (with more 1s) | Up to 4096 |

### Address Shift Register

During A16/A24 burst operations, the internal address shift register steps on each successive word:

```
A-Shift Register (A16 mode):
  Cycle 1: A[17:2] = [A_upper_16]
  Cycle 2: A[17:2] = [A_upper_16 + 1]  (shifted, wraps)
  Cycle 3: A[17:2] = [A_upper_16 + 2]
  ...
```

## VME Timing

### VME Bus Cycle Timing Diagrams

#### Single-Word Read (Phase Time = 125 ns at 8 MHz)

```
Single-Word VME Read Cycle (8 MHz, 4 phase = 500 ns)

A0-A23    |====Addr====|========idle========|
I/O CHERR |L____________|____________________H|
BERR      |L___________|_______H______________|
R/W       |L_______________________________H__|  (L=Read)
A/D       |_L______________________________H___|  (Address/Data strobe)
IACK      |_____L__________________________H___|  (Inter-Ack)
DS0       |__________L_____________H___________|  (D16 byte 0)
DS1       |__________L_____________H___________|  (D16 byte 1)
D0-D15    |_________HHHHHHHHHHHHHHHHHHHHHHHHHH)|  (data valid)
T         |t0|t8|t16|t24|t32|t40|t48|t56|t64|t72|

t0: Address placed, I/O CHERR goes low
t8: A/D goes low, R/W set, IACK goes low
t24: Data strobes go low, data valid on bus (D0-D15)
t32: Data strobes go high, data phase complete
t48: IACK goes high, A/D goes high
```

#### Single-Word Write

```
Single-Word VME Write Cycle

A0-A23    |====Addr====|========idle========|
I/O CHERR |L____________|____________________H|
BERR      |L___________|_______H______________|
R/W       |_______________________________L_H_|  (L=Write)
A/D       |_L______________________________H___|
IACK      |_____L__________________________H___|
DS0       |__________L_____________H___________|
DS1       |__________L_____________H___________|
D0-D15    |________HMMMMMMMMMMMMMMMMMMMMMMMMM|  (data placed at t16)
T         |t0|t8|t16|t24|t32|t40|t48|t56|t64|t72|

t16: Data placed on D0-D15
t24: Data valid on bus
t32: Data strobes go high (latch data on VME card)
```

### Burst Transfer

```
Burst Write Cycle (N-phase burst)

A0-A23    |====Addr====|=====A+1=====|====A+2====|.....|
IACK      |_____L______________H___________L________|...|
A/D       |_L_________________________L______________|...|
RS          |___L______________________L______________|...| (R/W stays)
DS0         |_______L_____________H__L______________H|..|
DS1         |_______L_____________H__L______________H|..|
R/W         |__L_____________________________________|...|
D0-D15      |_______HMMMMMMMMMMMMMMMHHMMMMMMMMMMMMMMM|...|
EOP         |______________________________________L|
T           |t0|t8|t16|t24|t32|...|t24(N)|...|t24(N)+8|

t0: Address + IACK + A/D
t16-t32: Each data phase (D0-D15 + DS0 + DS1)
t24N+8: EOP (last data transfer)
```

### VME Swap Transfer

```
VME Swap Cycle (Bidirectional FIFO transfer)

A0-A23    |====Addr====|=========idle==========|
IACK      |_____L______________________________H|
A/D         |_L_________________________________|
DS0         |____L___H__L___H__L___H__...__L___H|
DS1         |____L___H__L___H__L___H__...__L___H|
R/W         |__L___H__L___H__L___H__...__L___H__|
D(in)       |__HMMMM|HHHHH|HHHHH| ...|HHHHH|HHHH|  (when R/W=H=read)
D(out)      |__HHHHH|MMMMM|HHHHH| ...|HHHHH|HHHH|  (when R/W=L=write)
EOP         |_____________________________________|L|
T           |t0|t8|t16|t24|...|t32|t40|t48|t56|...|

R/W alternates between each data phase
D(in): data read from VME device (appears on bus during read phase)
D(out): data written to VME device (placed on bus during write phase)
```

### VME Interrupt Timing

```
VME Bus Interrupt Response

VME IRQ n   |______L_______________________________________________|
(external)
IACK_n      |______________________________L_____________H__________|
(IACK vector)|                              L=IACK for vector
            |                              H=IACK released
            |
68000 IPL   |_______________________________________________L_H_____
(input)     |                              IPL1-IPL7 (level n)
            |
68000 CPU   |...[complete current]| [L]  [H]  [Fetch vec $64-n] |...
cycle       |                    | BR  BGACK | Load vector reg    |
```

VME interrupt levels:

| Vector | IPL Level | Mega STE Register Bit |
|--------|-----------|----------------------|
| $64 | Level 1 (lowest) | sys_mask bit 0 / vme_stat bit 1 / vme_int bit 0 (Mega STE) |
| $66 | Level 2 | Not used on Mega STE (MFP/SCC routed here) |
| $68 | Level 3 | vme_mask bit 4 / vme_stat bit 4 |
| $6A | Level 4 | vme_mask bit 5 / vme_stat bit 5 |
| $6C | Level 5 | vme_mask bit 6 / vme_stat bit 6 |
| $6E | Level 6 | vme_mask bit 7 / vme_stat bit 7 |
| $70 | Level 7 (highest)| vme_mask bit 7 / vme_stat bit 7 |

## VME Expansion Slots

### Physical Connector

The Mega STE VME expansion interface uses a **96-pin low-profile edge connector** mounted on the bottom edge of the motherboard. The connector pins mate with a VME daughter card.

### VME Daughter Card Pinout

The 96-pin connector is organized into rows (pin pairs for power, singles for signals). Key pin assignments:

| Signal Group | Pin Range | Description |
|-------------|-----------|-------------|
| **Power** | Row A1/A2 | +5V, GND |
| **Power** | Row B13/B15 | +12V, -12V |
| **GND** | Multiple | Ground return paths |
| **Address** | Rows A14-A23 | A0-A23 to VME expansion card |
| **Data** | Rows B24-B39 | D0-D15 to VME expansion card |
| **Control** | Various | A/D, IACK, R/W, EOP, BERR, I/O CHERR |
| **Interrupt** | Various | IRQ1-IRQ7 from VME expansion card |
| **Acknowledge** | Various | DACK1-DACK7 to VME expansion cards |
| **Data Strobe** | Various | DS0-DS3 |
| **Arbitration** | Various | AM, BM, CA, CB (bus arbitration) |

### Connector Details

```
                    Mega STE Bottom Edge
                    ==================

                   /================================\
                  /    96-Pin VME Expansion        \
                 /      Daughter Card Slot          \
                /                                    \
    Row A:  |  A1  A2  A3  A4  A5  A6  ...  A24  |
    Row B:  |  B1  B2  B3  B4  B5  B6  ...  B24  |
                \                                    /
                 \__________________________________/
                      (expansion card plugged below)

Power on Row A:
  A1: GND
  A2: GND

Power on Row B:
  B13: +12V
  B15: -12V

Address/Data:
  Rows A7-A14: VME A0-A23 (24 address lines)
  Rows B24-B39: VME D0-D15 (16 data lines)
```

Known VME daughter card manufacturer: Kili

### Known VME Expansion Cards for Atari

#### Atari Official Products

| Card | Description | Availability |
|------|-------------|-------------|
| **Kili MultiBus VME interface board** | Multi-fuction card with Serial, Parallel, SCSI, Hard Disk on it | Official Atari product, limited availability |

The Kili card was the most common and documented VME expansion card for the Atari Mega STE. It was designed by Kili Engineering and marketed as an Atari-branded product.

#### Third-Party / Unofficial

| Card | Description | Notes |
|------|-------------|-------|
| **pro_VME VMEST** | Industrial VME clone system by VME Systems GmbH | Custom industrial system, VME rack mount |
| Various generic VME cards | Standard VME cards (not Atari-specific) | Would require custom device drivers |

### VME Address Configuration on Expansion Cards

| Memory Region | VME Address | Data Width | Description |
|--------------|-------------|-----------|-------------|
| VME board space | $FE000000 | D16 | A24 mode, 16 MB window |
| VME board space | $FFFE0000 | D16 | A16 mode, 64 KB window |
| VME board space | $FF000000 | D16/D32 | 24-bit shadow (compatibility) |

#### Known VME Expansion Cards for Atari

| Card | Manufacturer | Description |
|------|-------------|-------------|
| Kili MultiBus VME board | Kili Engineering (Atari) | Serial, parallel, SCSI, HDD controller on VME |
| pro_VME VMEST | VME Systems GmbH | Industrial clone, full VME interface |

### VME Card Memory Mapping

The Mega STE maps VME board memory using the A24 upper byte register. By setting this register appropriately, any of the 16 MB VME address space can be mapped to the physical address range `$FE000000-$FEFFFFFF`.

## EMULATION NOTES

### Hatari Emulator

The Hatari emulator implements Mega STE VME bus support. Key Hatari VME implementation details:

```c
// From Hatari (steemsse-V4.2.0_R10):
// Memory-mapped VME registers at I/O ports

// Registers:
#define SVmemvalid  0x420   // VME valid (address upper byte)
#define SVmemctrl   0x424   // VME control
#define SVmemval2   0x43a   // VME valid 2 (alternate)

// State:
  MegaSte.VmeSysMask  // VME system control mask (sys_mask reg)
  MegaSte.VmeSysStat  // VME system control status (sys_stat reg)
  MegaSte.VmeSysInt   // VME system interrupt force (sys_int reg)
  MegaSte.VmeMask     // VME interrupt mask (vme_mask reg)
  MegaSte.VmeStat     // VME interrupt status (vme_stat reg)

// I/O mapping:
  $FF8E01  -> VmeSysMask
  $FF8E03  -> VmeSysStat (read-only)
  $FF8E05  -> VmeSysInt
  $FF8E0D  -> VmeMask
  $FF8E0F  -> VmeStat (read-only)
```

### Emulation Challenges

1. **VME Bus Not Fully Emulated**: Hatari's VME bus implementation is marked "**VME bus not emulated**" in some code paths. The register shadow state is maintained but actual VME bus protocol timing and data transfers are not implemented.

2. **Register State Shadowing**: The emulator maintains proper register state for `sys_mask`, `sys_stat`, `sys_int`, `vme_mask`, and `vme_stat` so that Atari software that polls these registers will behave correctly.

3. **Address Translation**: The `VmeSysMask` register (upper address byte) maps Mega STE addresses to VME expansion board address space. In emulation, this is used to distinguish VME board accesses from regular system memory accesses.

4. **Interrupt Routing**: The emulator must correctly route VME interrupts through the 68000 IPL level system. VME IRQs 1-7 map to IPL levels 1-7 (with IPL7 being highest priority).

5. **Cycle Types**: Full VME cycle types (burst, swap, single-word) should be emulated for correct VME expansion card timing. Burst transfers require tracking address shift registers and EOP signals.

6. **A16 vs A24 Mode Switching**: Software dynamically switches between A16 and A24 modes by writing to the address upper byte register. Emulators must track the active mode.

### Key Register Operations

```
// Correct reading of VME interrupt status:
// (IMPORTANT: reading vme_mask clears bits in vme_stat!)

BYTE vme_stat_val = read_io($FF8E0F);    // Read status FIRST
BYTE vme_mask_val = read_io($FF8E0D);    // Then read mask (clears status)

// Correct clearing of system interrupt status:
BYTE sys_stat_val = read_io($FF8E03);    // Read status FIRST
BYTE sys_mask_val = read_io($FF8E01);    // Then read mask (clears status)

// Forcing system interrupt (level 1):
write_io($FF8E05, 0x01);   // Set bit 0 to generate level 1 interrupt
```

### Timing Notes for Emulators

| Cycle Type | Minimum Timing | Notes |
|-----------|---------------|-------|
| Single-word read | 500 ns (4 phase at 8 MHz) | One address + one data phase |
| Single-word write | 500 ns (4 phase at 8 MHz) | One address + one data phase |
| Burst write | 500 ns + (N-1) x 125 ns | Address + N data phases |
| Burst read | 500 ns + (N-1) x 125 ns | Address + N data phases |
| Swap | 500 ns + (N-1) x 125 ns | Alternating R/W per phase |
| Interrupt acknowledge | ~500 ns | IACK vector fetch cycle |

## References

- **Atari VME Expansion for TT030 and Mega STE Products Spec** (official Atari spec, ~127 KB)
- **[pro_VME VMEST - rare ST clone](https://randoc.wordpress.com/2024/04/11/pro_vme-vmest-the-rarest-of-the-contemporary-atari-st-clones/)** - Industrial clone details
- **[Inside the Atari Mega STe](https://www.goto10retro.com/p/inside-the-atari-mega-ste)** - Board overview with VME controller section
- **[Hatari User's Manual - Hardware List](https://hatari.li)** - Hatari VME bus implementation details
- **[VHDL ST System-on-Chip Project - Ch. 5](https://devlynx.ti-fr.com/ST/dev-docs.atariforge.org/alltogether.pdf)** - VME integration
- **[Steem SSE Documentation - hardware.txt](steemsse-V4.2.0_R10/steem/doc/hardware.txt)** - Register definitions
- **[Atari ST VME Interface Technical Reference Manual](http://ftp.pigwa.net/stuff/collections/Atari%20documents/Manuals/Atari%20TT030/vme_spec_7-19-1991.pdf)** - VME C.1 sub-spec

(End of file)
