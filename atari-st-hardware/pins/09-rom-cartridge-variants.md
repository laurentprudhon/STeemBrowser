# ROM Cartridge Variants

> Comprehensive documentation of cartridge ROM types, pinout, enable logic, model differences, known cartridges, and emulation considerations for the Atari ST family.

## 1. Cartridge Port Overview

The Atari ST **cartridge port** is a 40-pin expansion connector (40S socket, male) located on the top surface of the console. It provides read-only access to external ROM cartridges that can replace or extend the system TOS ROM.

### Connector Details

| Parameter | Value |
|---|---|
| Connector type | 40-pin single row (40S male slot on board, mated with 40-pin PCB edge connector on cartridge) |
| Pin pitch | 2.54 mm (0.100 in) |
| Location | Top of console (center-rear area) |
| Access type | Read-only (no write signal pin) |
| Max cartridge size | 128 KB (27C512 or equivalent) |

### Models with Cartridge Port

| Model | Cartridge Port | Year |
|---|---|---|
| 520ST | Yes | 1985 |
| 1040ST | Yes | 1986 |
| Mega ST | Yes | 1987 |
| 520STE | Yes | 1989 |
| 1040STE | Yes | 1990 |
| Mega STe | Yes | 1990 |
| Mega STE | Yes | 1991 |

All Atari ST models include the cartridge port. The **TT030** and **Falcon030** do not carry forward the cartridge port, as they moved to different expansion strategies (TT: VME-based, Falcon: IDE/SCSI).

### Purpose

The cartridge port was designed for:
1. **TOS upgrades**: Replace onboard TOS with newer versions (e.g., TOS 2.06 cartridge for updating a 520ST running TOS 1.00)
2. **Standalone utility cartridges**: Diagnostics, demos, games that boot without disk
3. **Specialized ROM expansion**: Add custom firmware for memory management, disk utilities, or other system enhancements

### Cartridge ROM Base Address

The cartridge ROM is mapped to the address range `$FA0000`-$`$FBFFFF` (128 KB), which partially overlaps the TOS ROM region. The system TOS occupies `$FC0000`-$`$FFFFFF` (256-384 KB depending on TOS version).

```
Memory Map (relevant regions):

$000000          Exception vectors
...
$100000          System RAM starts
...
$FA0000          Cartridge ROM (128 KB max)
$FB0000          Cartridge ROM (continued)
$FC0000          TOS ROM (lower portion)
$FE0000          TOS ROM (upper portion)
$FFFFFF
```

## 2. 40-Pin Connector Pinout

The 40-pin cartridge port maps the 68000 bus signals directly to the connector, providing full address/data bus access plus selective ROM enable signals.

### Complete Pinout

| Pin | Signal | Direction | Description |
|---|---|---|---|
| 1 | +5V | Power | +5V power supply |
| 2 | +5V | Power | +5V power supply |
| 3 | D14 | Data Bus | Data bit 14 (68000 bus) |
| 4 | D15 | Data Bus | Data bit 15 (68000 bus) |
| 5 | D12 | Data Bus | Data bit 12 (68000 bus) |
| 6 | D13 | Data Bus | Data bit 13 (68000 bus) |
| 7 | D10 | Data Bus | Data bit 10 (68000 bus) |
| 8 | D11 | Data Bus | Data bit 11 (68000 bus) |
| 9 | D8 | Data Bus | Data bit 8 (68000 bus) |
| 10 | D9 | Data Bus | Data bit 9 (68000 bus) |
| 11 | D6 | Data Bus | Data bit 6 (68000 bus) |
| 12 | D7 | Data Bus | Data bit 7 (68000 bus) |
| 13 | D4 | Data Bus | Data bit 4 (68000 bus) |
| 14 | D5 | Data Bus | Data bit 5 (68000 bus) |
| 15 | D2 | Data Bus | Data bit 2 (68000 bus) |
| 16 | D3 | Data Bus | Data bit 3 (68000 bus) |
| 17 | D0 | Data Bus | Data bit 0 (68000 bus) |
| 18 | D1 | Data Bus | Data bit 1 (68000 bus) |
| 19 | A13 | Addr Bus | Address bit 13 (from 68000) |
| 20 | A15 | Addr Bus | Address bit 15 (from 68000) |
| 21 | A8 | Addr Bus | Address bit 8 (from 68000) |
| 22 | A14 | Addr Bus | Address bit 14 (from 68000) |
| 23 | A7 | Addr Bus | Address bit 7 (from 68000) |
| 24 | A9 | Addr Bus | Address bit 9 (from 68000) |
| 25 | A10 | Addr Bus | Address bit 10 (from 68000) |
| 26 | A6 | Addr Bus | Address bit 6 (from 68000) |
| 27 | A5 | Addr Bus | Address bit 5 (from 68000) |
| 28 | A12 | Addr Bus | Address bit 12 (from 68000) |
| 29 | A11 | Addr Bus | Address bit 11 (from 68000) |
| 30 | A4 | Addr Bus | Address bit 4 (from 68000) |
| 31 | ROM3SEL | Control | ROM3 chip select (active low) |
| 32 | A3 | Addr Bus | Address bit 3 (from 68000) |
| 33 | ROM4SEL | Control | ROM4 chip select (active low) |
| 34 | A2 | Addr Bus | Address bit 2 (from 68000) |
| 35 | US / DSE | Control | Upper data strobe (ST: active low; STe: data strobe for alternate ROM bank) |
| 36 | A1 | Addr Bus | Address bit 1 (from 68000) |
| 37 | LS / DSL | Control | Lower data strobe (ST: active low; STe: data strobe for alternate ROM bank) |
| 38 | GND | Ground | Signal ground |
| 39 | GND | Ground | Signal ground |
| 40 | GND | Ground | Signal ground |

### Visual Pinout Diagram

```
Cartridge Port — 40-Pin View (from outside, pins facing up)

Side A (pins 1-20, left half):
  Pins facing toward rear of machine:

  +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
  | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 |10 |11 |12 |13 |14 |15 |16 |17 |18 |19 |20 |
  +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
  | PWR|PWR| D14| D15| D12| D13| D10| D11|  D8|  D9|  D6|  D7|  D4|  D5|  D2|  D3|  D0|  D1| A13| A15|
  +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+

Side B (pins 21-40, right half):

  +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
  |21 |22 |23 |24 |25 |26 |27 |28 |29 |30 |31 |32 |33 |34 |35 |36 |37 |38 |39 |40 |
  +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+
  |  A8| A14|  A7|  A9| A10|  A6|  A5| A12| A11|  A4|ROM3|  A3|ROM4|  A2| US/LS |  A1| LS/DS |GND |GND |GND |
  +---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+---+

Signal groupings:
  Pins  1- 2:  +5V power (tie to cartridge VCC)
  Pins  3-18:  D0-D15 data bus (every other pin, interleaved with address)
  Pins 19-36:  A1-A15 address bus (every other pin, interleaved with data)
  Pin  31:     ROM3SEL (chip select for lower/primary ROM bank, active low)
  Pin  33:     ROM4SEL (chip select for upper/secondary ROM bank, active low)
  Pin  35:     Upper data strobe (UDS) — indicates high byte of data word
  Pin  37:     Lower data strobe (LDS) — indicates low byte of data word
  Pins 38-40:  Ground (tie to cartridge GND)
```

### Data Bus Pin Interleaving Pattern

```
Data pins (D0-D15) on odd/even pins of connector:

  Pin 17 → D0    (odd pin)
  Pin 18 → D1    (even pin)
  Pin 15 → D2    (odd pin)
  Pin 16 → D3    (even pin)
  Pin 13 → D4    (odd pin)
  Pin 14 → D5    (even pin)
  Pin 11 → D6    (odd pin)
  Pin 12 → D7    (even pin)
  Pin  9 → D8    (odd pin)
  Pin 10 → D9    (even pin)
  Pin  7 → D10   (odd pin)
  Pin  8 → D11   (even pin)
  Pin  5 → D12   (odd pin)
  Pin  6 → D13   (even pin)
  Pin  3 → D14   (odd pin)
  Pin  4 → D15   (even pin)
```

### Address Bus Pin Interleaving Pattern

```
Address pins (A1-A15) on odd/even pins of connector:

  Pin 36 → A1     (even pin)
  Pin 30 → A4     (even pin)
  Pin 34 → A2     (even pin)
  Pin 32 → A3     (even pin)
  Pin 27 → A5     (odd pin)
  Pin 26 → A6     (even pin)
  Pin 23 → A7     (odd pin)
  Pin 21 → A8     (odd pin)
  Pin 24 → A9     (even pin)
  Pin 25 → A10    (even pin)
  Pin 29 → A11    (odd pin)
  Pin 28 → A12    (even pin)
  Pin 19 → A13    (odd pin)
  Pin 22 → A14    (even pin)
  Pin 20 → A15    (even pin)
```

## 3. Compatible Cartridge ROM Types

The cartridge port supports several EPROM and EEPROM families. The key constraint is the pin count and package type — cartridges must fit within the physical slot.

### Supported ROM Chips

| ROM Type | Capacity | Organization | Pins | Package | Voltage | Notes |
|---|---|---|---|---|---|---|
| **27256** | 32 KB | 32K x 8 | 28 | DIP28 | 270 nm | Standard EPROM. Only 1 needed (pins 3,7 = 18 for ST mode) |
| **27512** | 64 KB | 64K x 8 | 28 | DIP28 | 270 nm | Standard EPROM. Only 1 needed |
| **27C256** | 32 KB | 32K x 8 | 28 | DIP28 | +5V | UV-erasable CMOS version of 27256 |
| **27C512** | 64 KB | 64K x 8 | 28 | DIP28 | +5V | UV-erasable CMOS. Most common upgrade type |
| **27C010** | 128 KB | 128K x 8 | 28 | DIP28 | +5V | UV-erasable. Max single-chip capacity (fills full 128 KB cartridge slot) |
| **27C020** | 256 KB | 256K x 8 | 32 | DIP32 | +5V | Too many pins (32-pin DIP). Not compatible with slot without adapter |

### ROM Organization for Cartridge Design

#### Single-Chip Configuration (27C512, 64 KB)

```
27C512  (64 KB) covers full $FA0000-$FAFFFF
Address: A0-A12 (13 bits from port pins 36,30,34,32,27,26,23,21,24,25,29,28,19)
CS:      ROM3SEL (pin 31) active low
OE:      Connected to both US and LDS (pins 35 + 37) via OR gate
VCC:     Pin 20 → +5V (pins 1/2 on port)
GND:     Pin 10 → GND (pins 38-40 on port)
```

#### Dual-Chip Configuration (2 x 27C010 or 2 x 27512, 128 KB)

```
27C010 #1 (lower 64 KB: $FA0000-$FAFFFF):
  Address: A0-A12 (13 address lines)
  CS:      ROM3SEL (pin 31) active low
  
27C010 #2 (upper 64 KB: $FB0000-$FBFFFF):
  Address: A0-A12 (13 address lines)
  CS:      ROM4SEL (pin 33) active low
  
Both chips share:
  D0-D7 (lower data bus) via odd pins 3-6,9,11-14,17,19
  US + LDS (OR'ed for output enable) via pins 35 + 37
  +5V and GND
```

### ROM Types Supported Table

```
+------------+-----------+--------+--------+----------+-----------+-----------+
| ROM Type   | Capacity  | Addr L.| CS Pin | VCC Req. | Erase Type| Max Model |
+------------+-----------+--------+--------+----------+-----------+-----------+
| 27256      | 32 KB     | A0-A11 | 31     | 21V/5V   | UV        | All       |
| 27C256     | 32 KB     | A0-A11 | 31     | +5V      | UV        | All       |
| 27512      | 64 KB     | A0-A12 | 31     | 21V/5V   | UV        | All       |
| 27C512     | 64 KB     | A0-A12 | 31     | +5V      | UV        | All       |
| 27C010     | 128 KB    | A0-A13 | 31,33  | +5V      | UV        | ST/STe    |
| 2864 (E2PROM)| 64 KB   | A0-A12 | 31     | +5V      | Electrical | STe only |
+------------+-----------+--------+--------+----------+-----------+-----------+

Voltage notes:
  27xxx (standard EPROM): Requires 21V for programming (EPROM programmer needed)
  27Cxxx (CMOS EPROM):    +5V programming (easier, but UV erasure still needed)
  28xxx (EEPROM):         +5V electrical erase/write (no UV lamp needed)

Addr L = address lines used (determines per-chip capacity)
CS Pin = ROM3SEL = pin 31, ROM4SEL = pin 33
Max Model = which hardware can physically use the chip (all support ROM3SEL/ROM4SEL decode)
```

### Pin Wiring Summary for Cartridge PCB

```
Cartridge PCB Edge Connector (40-pin male → motherboard socket):

  VCC (+5V)  →  Port pins 1, 2
  GND        →  Port pins 38, 39, 40
  
  D0-D15     →  Port pins 17,18,15,16,13,14,11,12,9,10,7,8,5,6,3,4
                 (interleaved odd/even on the motherboard connector)
  
  A0-A12     →  Port pins 36,30,34,32,27,26,23,21,24,25,29,28,19
  A13        →  Port pin 19 (shared with A13 above)
                 (pins used for address decode and ROM address inputs)
  
  ROM3SEL    →  Port pin 31  (chip select, active low)
  ROM4SEL    →  Port pin 33  (chip select, active low)
  
  OE (output enable)
  → Port pins 35 (US/UDS) + 37 (LS/LDS) ORed together
     (OR gate: OE = US NOR LDS, since both are active low)
```

## 4. ROM Size Limits Per Model

The maximum cartridge ROM size depends on the address decoding capability of each model.

### ROM Size Limits by Model

| Model | Max Size | ROM3SEL/A13 Decode | Address Lines Used | Max Capacity |
|---|---|---|---|---|
| **520ST** | 64 KB | ROM3SEL on A13=0, ROM4SEL on A13=1 | A0-A12 | 64 KB (1 x 27C512) |
| **1040ST** | 64 KB | ROM3SEL on A13=0, ROM4SEL on A13=1 | A0-A12 | 64 KB (1 x 27C512) |
| **Mega ST** | 64 KB | ROM3SEL on A13=0, ROM4SEL on A13=1 | A0-A12 | 64 KB (1 x 27C512) |
| **520STE** | 128 KB | ROM3SEL + ROM4SEL independent | A0-A14 | 128 KB (2 x 27C010) |
| **1040STE** | 128 KB | ROM3SEL + ROM4SEL independent | A0-A14 | 128 KB (2 x 27C010) |
| **Mega STe** | 128 KB | ROM3SEL + ROM4SEL independent | A0-A14 | 128 KB (2 x 27C010) |
| **Mega STE** | 128 KB | ROM3SEL + ROM4SEL independent | A0-A14 | 128 KB (2 x 27C010) |

### Address Decoding per Model

#### ST/Mega ST: 64 KB Limit

```
ST Address Decode (64 KB per chip):

  A14 = 0 (fixed, always low in $FAxxxx-$FBxxxx range)
  A13 = 0 → ROM3SEL active → lower 64K ($FA0000-$FAFFFF)
  A13 = 1 → ROM4SEL active → upper 64K ($FB0000-$FBFFFF)
  
  However, on original ST:
  - ROM4SEL is typically NOT wired to cartridge PCB (no dual ROM support)
  - Address A14 is decoded by Glue to allow only $FA0000-$FAFFFF
  - Max usable: A0-A12 = 13 addr lines = 8192 x 8 = 64 KB
  
  Actual ST limit: 64 KB (one 27C512)
```

#### STe/Mega STE: 128 KB (2 ROM banks)

```
STe Address Decode (128 KB total, two ROM banks):

  A14 = 0, A13 = 0 → ROM3SEL active → lower 64K ($FA0000-$FAFFFF)
  A14 = 1, A13 = 1 → ROM4SEL active → upper 64K ($FB0000-$FBFFFF)
  
  STe uses both ROM3SEL and ROM4SEL:
  - ROM3SEL = !A14 (low when A14 is low, i.e., $FA0000-$FAFFFF)
  - ROM4SEL = A14 (high when A14 is high, i.e., $FB0000-$FBFFFF)
  - Combined: full 128 KB accessible
  
  Total: 128 KB (two 27C010 chips in package)
```

### Capacity Calculation

```
+-----------------------------+
| Address bits → ROM capacity |
+-----------------------------+

  8 bits (A0-A7)   → 256 B (2704 type - too small)
  9 bits (A0-A8)   → 512 B (2708 type)
  10 bits (A0-A9)  → 1 KB (2716 type)
  11 bits (A0-A10) → 2 KB (2732 type)
  12 bits (A0-A11) → 4 KB (2764 type)
  13 bits (A0-A12) → 8 KB (27128/27256)
  14 bits (A0-A13) → 16 KB (27256 can map)
  15 bits (A0-A14) → 32 KB (27512/27C512)
                      64 KB (27512)
  16 bits (A0-A15) → 128 KB (27C010)
                      256 KB (27C020 - not compatible)

  ST uses A0-A12 (13 bits) → 8 KB per bank = 64 KB total
  STe uses A0-A14 (15 bits) → 128 KB with dual banks
```

## 5. How TOS Detects and Boots from Cartridges

The Atari TOS boot process checks for a valid cartridge ROM before booting from disk.

### Boot Detection Sequence

```
TOS Cartridge Boot Detection Sequence:

  1. Power-on reset
     CPU reads exception vectors from $FA0000-$FA0007 (boot vector area)
  
  2. Check cartridge validity
     If cartridge present and contains valid boot vector at $FA0000:
       - Read expected boot vector stored at $FA0000-$FA0007
       - Valid boot vector = $FA0000 (self-referencing)
       - Some implementations check for $FFFF pattern (erased ROM)
         to determine if cartridge is present
      
  3. If valid cartridge detected
       Jump to vector at $FA0004 (cartridge entry point)
       Cartridge firmware executes its own initialization
      
  4. If NO valid cartridge
       Proceed with normal disk/TOS boot from $FC0000:
       - Read TOS ROM base address from reset vector
       - Initialize hardware (Glue, MMU, DMA, Shifter)
       - RAM test ($4F2, $43A, $51A memory validation bytes)
       - Load TOS from boot sector (disk A:)
       - Continue normal TOS initialization
```

### Cartridge Presence Detection Logic

```
TOS $F856 (GetRomVersion / cartridge detection in ROM):

  Method 1 — Write-check at $FAFFFF:
    TOS writes $55 to $FAFFFF, then reads it back
    If readback ≠ $55 → ROM is read-only (cartridge present at $FA0000-$FBFFFF)
    If readback = $55 → RAM at that address (no cartridge present)
  
  Method 2 — Check erase state:
    TOS reads $FAFFFF while cartridge empty ($FFFF = erased ROM state)
    If all $FFFF → no cartridge (or empty cartridge)
    If non-$FFFF → cartridge present
  
  Method 3 — Check boot vector at $FA0000-$FA0007:
    Compare against known valid vector (typically $4EF9 $FA000)
    If valid → jump to cartridge ROM
```

### Cartridge TOS Boot (BIOS Upgrade Cartridge)

```
BIOS Upgrade Cartridge Boot Flow:

  1. Cartridge ROM at $FA0000 contains:
     - Boot vector at $FA0000 (same as TOS except vector at $FA0004 points to cartridge code)
     - TOS code (replacement for onboard TOS at $FC0000-$FFFFFF)
     - TOS data area at lower address

  2. On reset, CPU reads vector:
     SR = $2700 (supervisor mode)
     PC = $FA0004 (cartridge entry point)
  
  3. Cartridge firmware validates itself:
     - Check ROM version bytes
     - Validate checksum
     - Copy ROM data to RAM at $FC0000
   
  4. Jump to $FC0000 (now contains cartridge TOS)
     TOS initializes normally from updated code
  
  5. On next cold boot:
     Cartridge TOS now at $FC0000 in RAM persists
     Cartridge no longer needed (TOS copied to RAM)
```

### Pin Signals Used in Boot Detection

```
Signals involved in cartridge detection:

  A13 ($FA0000-$FBFFFF range select via ROM3SEL/ROM4SEL)
  A14 ($F00000-$F7FFFF decode, prevents cartridge from appearing in wrong range)
  ROM3SEL (pin 31) — asserts when CPU reads from $FA0000-$FAFFFF
  ROM4SEL (pin 33) — asserts when CPU reads from $FB0000-$FBFFFF
  US (pin 35) — upper byte strobe (active during high-byte data transfers)
  LDS (pin 37) — lower byte strobe (active during low-byte data transfers)
  DTACK — Glue provides DTACK based on address decode (ROM3SEL/ROM4SEL)
```

## 6. Cartridge Enable Signal Logic

The cartridge is enabled through a combination of address decode logic and control signals coordinated by the Glue chip (or GST MCU in STe models).

### Enable Logic Diagram

```
Cartridge Enable Logic (Original ST):

                    68000 Address Bus
                    ==================
                            |
         +------------------+------------------+
         |                  |                  |
   Address Decode     ROM3SEL Decode     ROM4SEL Decode
   (in Glue chip)     (from A13=0)       (from A13=1)
         |                  |                  |
   +----+----+          +---+---+          +---+---+
   | A14=0,    |          | ROM3 |          | ROM4 |
   | FC3 (Data) |         | SEL= |         | SEL= |
   | 14 bits   |          |  LOW |         |  LOW |
   | decode    |          +------+          +------+
   +----+----+          Active:          Active:
        |             $FA0000-          $FB0000-
        |             $FAFFF             $FBFFFF
        |
        v
   ROM3SEL ──────────────► Pin 31 of port
   ROM4SEL ──────────────► Pin 33 of port
   US (UDS)  ────────────► Pin 35 (upper data strobe)
   LDS       ────────────► Pin 37 (lower data strobe)
```

### Detailed Enable Logic

```
Cartridge Enable Conditions (when cartridge module is inserted):

  68000 Bus Cycle:
  
  1. CPU initiates read at address $FA0000-$FBFFFF
     → Address A14 goes LOW ($F00000-$F7FFFF decode is separate)
     → Address A13 determines which ROM bank:
     
  2. A14 = LOW (for $FA0000-$FAFFFF range):
     
     Glue Logic:
       A14 = 0 + A22 = 1 (FC3 = Data cycle) 
         → ROM3SEL on pin 31 goes LOW (active)
         → ROM chip #1 enabled
     
  3. A14 = HIGH (for $FB0000-$FBFFFF range):
     
     Glue Logic:
       A14 = 1 + A22 = 1 (FC3 = Data cycle)
         → ROM4SEL on pin 33 goes LOW (active)
         → ROM chip #2 enabled
```

### Enable Logic with Signal Propagation

```
Cartridge Enable Signal Flow:

  CPU Address:
    A14 ──────────────────────┬────────── Address decode
                               │
    A13 ──────────────────────┼────────── Bank select
                               │
    A0-A12 ───────────────────┼────────── ROM address lines
                               │
  ┌────────────────────────────┐
  │      Glue Chip (C029144)   │
  │                            │
  │  Address Decoder:          │
  │    IF A22=1 AND FC3=0     │  (Data bus cycle)
  │      IF A14=0 & A21=1     │
  │        → ROM3CS = 0 (low)  │
  │      IF A14=1 & A21=0     │
  │        → ROM4CS = 1 (high) │
  │                            │
  │  Output Enables:           │
  │    US (from 68000 UDS) ──► Pin 35
  │    LDS (from 68000 LDS) ──► Pin 37
  └────────────────────────────┘
                               │
  ┌────────────────────────────┐
  │  Cartridge ROM PCB         │
  │                            │
  │  Pin 31 (ROM3SEL):         │
  │    = 0 → ROM #1 selected   │
  │    = 1 → ROM #1 deselected │
  │                            │
  │  Pin 33 (ROM4SEL):         │
  │    = 0 → ROM #2 selected   │
  │    = 1 → ROM #2 deselected │
  │                            │
  │  OE = US NOR LDS           │
  │    (OR of two active-low   │
  │     strobes, inverted)     │
  │                            │
  │  D0-D15 bus ──────────────► Data out
  │                            │
  └────────────────────────────┘

  Timing:
    68000 read cycle at $FAxxxx-$FBxxxx:
      A14 low/high ──► ROM3SEL/ROM4SEL
      US, LDS strobe ──► OE (output enable)
      After ~200 ns ──► Data valid on D0-D15
      DTACK pulled low by Glue ──► CPU completes cycle
```

### Cartridge Disable State

```
Cartridge Disable Conditions:

  The cartridge is DISABLED when:
    A14 = HIGH (address above $F00000, i.e., $FF0000+)
    ROM3SEL is high (inactive)
    ROM4SEL is high (inactive)
    DTACK = 0 (not asserted by cartridge, defaults through Glue)
  
  The cartridge is NOT accessed during:
    - Normal RAM reads ($000000-$7FFFF)
    - TOS ROM reads ($FC0000-$FFFFFF) via different decode
    - Hardware register reads ($FF8000-$FFDFFF)
    - ACSI register access ($F80000-$F803FF)
```

## 7. Differences Between ST and STe Cartridge Ports

While the physical 40-pin connector is identical on all models, the address decoding logic and ROM support differ significantly.

### ST vs STe Cartridge Port Comparison

| Feature | ST (520ST, 1040ST, Mega ST) | STe (520STE, 1040STE, Mega STe) | Mega STE |
|---|---|---|---|
| Connector | 40-pin 40S | 40-pin 40S (identical) | 40-pin 40S (identical) |
| Max cartridge size | 64 KB | 128 KB | 128 KB |
| ROM3SEL decode | A13=0 → active | Same | Same |
| ROM4SEL decode | A13=1 → active (theoretical) | A14=1 → active (64 KB upper bank) | Same as STe |
| ROM support | Single chip (27C512, 1 ROM) | Dual chips (2 x 27C010, 2 ROMs) | Same as STe |
| US/LDS signals | UDS (upper data strobe) | UDS + extended decode for alternate bank | Same as STe |
| Address decode | Glue chip (C029144) | GST MCU (C302183) | GST MCU + SH2 |
| Alternate cartridge | No | Yes (via US/LDS) | Yes (via US/LDS) |
| EEPROM support | Limited (UV erase needed) | Full (electrical erase) | Full |

### Key Differences

```
ST Cartridge Port Characteristics:

  Memory mapping:
    $FA0000-$FAFFFF = Cartridge (via ROM3SEL, 64 KB max)
    
  Glue chip (C029144) decode:
    A14 = 0 for cartridge region
    A22 = 1 (FC3 = Data bus cycle)
    A13 = 0 → ROM3CS (active, $FA0000-$FAFFFF)
    A13 = 1 → ROM4CS (active, $FB0000-$FBFFFF - rarely used)
    
  Most ST cartridges:
    - Use only ROM3SEL (single bank)
    - Contain single 27C512 (64 KB)
    - Cannot fill full 128 KB range (only 64 KB of usable space)

STe Cartridge Port Characteristics:

  Memory mapping (full):
    $FA0000-$FAFFFF = Cartridge Bank 1 (via ROM3SEL, A14=0)
    $FB0000-$FBFFFF = Cartridge Bank 2 (via ROM4SEL, A14=1)
    
  GST MCU decode:
    A14 = 0 → ROM3CS (Bank 1, lower 64KB)
    A14 = 1 → ROM4CS (Bank 2, upper 64KB)
    A13 also used as secondary decode line
    
  STe cartridges:
    - Can use both ROM3SEL and ROM4SEL
    - 2 x 27C010 = 128 KB full range
    - US/LDS signals used for alternate ROM enable selection
```

### US/LDS Signal Evolution

```
US (Upper Data Strobe) Signal Evolution:

  ST (original):
    Pin 35 = UDS (Upper Data Strobe)
    Same function as 68000 UDS pin 42
    Used only as part of OR with LDS for ROM output enable
    
  STe / Mega STE:
    Pin 35 = UDS / alternate ROM enable
    STe adds logic to use US as additional select line
    Allows STe to access 2 separate 64 KB cartridge banks
    The GST MCU interprets UDS differently for ROM bank selection

  LDS (Lower Data Strobe) Signal:
    Pin 37 = LDS (Lower Data Strobe)
    Same function as 68000 LDS pin 36
    Standard use across all models for lower byte select

  Together (US | LDS):
    Used in all models for ROM chip output enable:
    OE = !(US AND LDS) for active-low strobes
    When either strobe is low during a bus cycle, ROM outputs data
```

## 8. Known Third-Party Cartridges

Numerous third-party cartridges were produced for the Atari ST, spanning BIOS upgrades system utilities, and games.

### BIOS Upgrade Cartridges

| Cartridge | Version | Size | Year | Purpose |
|---|---|---|---|---|
| **Atari TOS 2.03 Cartridge** | TOS 2.03 | 64 KB | 1986 | Upgrade 520ST (TOS 1.0) to TOS 2.0. Contains TOS 2.03 ROM in $FA0000-$FAFFFF, copies to RAM on boot. |
| **Atari TOS 2.06 Cartridge** | TOS 2.06 | 64 KB | 1989 | Most common BIOS upgrade. Upgrades all ST models to TOS 2.06 (adds mouse support, STe compatibility). |
| **Atari TOS 3.06 Cartridge** | TOS 3.06 | 128 KB | 1991 | Mega STE BIOS upgrade. Fills 128 KB, upgrades to TOS 3.06. Requires STe hardware support. |
| **Atari TOS 4.04 Cartridge** | TOS 4.04 | 128 KB | 1991 | Mega STE upgrade cartridge. Full TOS 4.04 in cartridge ROM. |
| **Medion TOS 3.01 Cartridge** | TOS 3.01 | 64 KB | 1990 | Medion (Atari licensee in Germany) upgrade cartridge. |
| **Amstrad TOS 2.06 Cartridge** | TOS 2.06 | 64 KB | 1990 | Amstrad (European licensee) branded upgrade. |

### System Utility Cartridges

| Cartridge | Purpose | Size | Notes |
|---|---|---|---|
| **Diagnosis Cartridge** | System diagnostics by Atari | 64 KB | Tests memory, I/O ports, disk drives, MIDI. Used by Atari service centers. |
| **Datalight ROM-DOS** | ROM-based disk operating system | 128 KB | ROMDOS replaces Atari's disk DOS. Boots from cartridge, provides DOS-compatible filesystem. |
| **BlitterDOS** | ROMDOS-compatible utility | 64 KB | Alternative to Datalight ROM-DOS. Includes disk utilities. |
| **Degas Professional cartridge** | Standalone graphics cartridge | 128 KB | Degas Pro ROM boots standalone without disk. |
| **Sparta XT Upgrade Cartridge** | Enhanced GEM with utilities | 64 KB | Includes sparta-dma driver, enhanced file manager. |
| **Mega STE Diagnostic** | Mega STE hardware test | 128 KB | Tests IDE, Super Hi-Res, stereo audio for Mega STE. |
| **RAM Turbo** | Memory manager (loads into RAM) | 64 KB | Provides expanded memory management, boots from cartridge. |

### Entertainment/Demo Cartridges

| Cartridge | Type | Size | Notes |
|---|---|---|---|
| **STe Demo Cartridge** | Demo disc ROM | 128 KB | Shown at CES 1990. Demonstration of cartridge boot capability. |
| **Various Game Cartridges** | Unlicensed | varies | Some third-party games used cartridge ROM format. Common in German market (Amstrad/Medion era). |
| **STe Test Firmware** | Internal Atari test | 128 KB | Used by Atari engineers. Pre-production STe firmware test ROM. |

### Cartridge Pin Compatibility Across Models

```
+----------------------------------------+----------+----------+---------+
| Cartridge Type                         | ST       | STe     | Mega STE |
+----------------------------------------+----------+----------+---------+
| ROM3SEL only cartridge (64 KB)         | ✓ Works  | ✓ Works  | ✓ Works |
| 27C512 single-chip (64 KB)             | ✓ Works  | ✓ Works  | ✓ Works |
| Dual ROM (128 KB via ROM3+ROM4)         | ✗ limited| ✓ Works  | ✓ Works |
| 27C010 dual-chip (128 KB)              | ✗ 64 KB  | ✓ Works  | ✓ Works |
| EEPROM-based (2864/2816)               | ✗ limited| ✓ Works  | ✓ Works |
| TOS 2.06 BIOS upgrade                  | ✓ Works  | ✓ Works  | ✓ Works |
| TOS 3.06 BIOS upgrade                  | ✗ partial| ✓ Works  | ✓ Works |
| TOS 4.04 BIOS upgrade                  | ✗ partial| ✓ Works  | ✓ Works |
+----------------------------------------+----------+----------+---------+

Notes:
  "Works" = Cartridge loads and functions correctly on that model.
  "64 KB" = Only lower 64 KB usable on ST (upper bank not decoded).
  "partial" = Some functionality may work, but full features require STe/Mega STE.
```

## 9. Hardware Compatibility Issues

Several real-world compatibility issues exist with Atari ST cartridge ROMs, both hardware and software level.

### Known Compatibility Issues

| Issue | Description | Affected Models | Workaround |
|---|---|---|---|
| **Cartridge not detected** | TOS does not find valid boot vector at $FA0000 | All ST models | Ensure cartridge is fully seated; check $FA0000-$FA0007 for valid vector |
| **Wrong ROM type** | EEPROM (2864) not recognized as ROM by some TOS versions | ST, early STe | Use EPROM (27Cxxx) for best compatibility |
| **ROM speed mismatch** | 100 ns EPROM may be too slow at 8 MHz CPU speed | All ST | Use 70 ns or 60 ns EPROM for reliable operation |
| **Address line conflicts** | A13/A14 decode may conflict with Glue chip | Early Mega ST | Use later revision motherboard with corrected decode |
| **Boot loop** | Cartridge ROM at $FC0000 conflicts after copying TOS to RAM | ST (pre-TOS 2.0) | Upgrade to TOS 2.06 or later before using upgrade cartridge |
| **STe-only cartridge on ST** | 128 KB cartridge uses ROM4SEL which ST Glue chip ignores | ST, Mega ST | Use 64 KB ROM (27C512) on ST |
| **Power supply surge** | Inserting cartridge can cause +5V transient | All ST | Power off before inserting/removing cartridges |
| **Pin corrosion** | 40-pin connector corrosion causes detection failures | All (older units) | Clean pins with contact cleaner |
| **Wrong cartridge model** | TT030/Falcon cartridges don't fit ST cartridge slot | TT/Falcon | Use ST-specific cartridges |

### ROM Speed Requirements

```
68000 Bus Timing at 8 MHz:

  Bus cycle: 4 phase times = 500 ns (@ 8 MHz = 125 ns per phase)
  
  EPROM speed requirements:
  
  27C512 at 120 ns → Borderline at 8 MHz (~acceptable)
  27C512 at 100 ns → Acceptable at 8 MHz
  27C512 at 70 ns  → Recommended for 8 MHz operation
  27C512 at 60 ns  → Best performance at 8 MHz
  
  27C010 at 120 ns → Borderline at 8 MHz (~acceptable)
  27C010 at 100 ns → Acceptable at 8 MHz
  27C010 at 70 ns  → Recommended for 8 MHz operation
  27C010 at 50 ns  → Best performance at Mega STE (16 MHz)
  
  Mega STE (16 MHz CPU) is more demanding:
    Minimum: 50 ns EPROM
    Recommended: 35 ns EPROM or 25 ns EEPROM
```

### Mechanical Compatibility

```
Cartridge physical constraints:

  Slot dimensions (on motherboard):
    Width: ~115 mm (fits along top edge, adjacent to keyboard)
    Depth: ~45 mm (measured from slot position)
    Height: ~18 mm (from top of PCB to lowest component)
    Weight limit: ~50 g (slot connectors rated for small force)

  Cartridge PCB requirements:
    Edge connector: 40-pin gold-finger edge card
    Thickness: 1.6 mm (standard PCB thickness)
    No components on edge connector side
    No protrusion above 18 mm (interferes with keyboard cover)
```

### Power Considerations

```
Power delivery to cartridge:

  +5V supply from port pins 1 and 2:
    Current available: ~100 mA (shared with other internal circuits)
    Total cartridge ROM power: ~30 mA (single 27C512 active)
    Two 27C010 in STe: ~60 mA maximum
  
  Ground paths via pins 38, 39, 40:
    Provides redundant grounding
    Each pin rated for ~100 mA
```

### Cross-Compatible Cartridges

```
Which cartridges work on which models:

  TOS upgrade cartridges:
    TOS 2.06 → Works on all ST/STe/Mega STE (copies to RAM on boot)
    TOS 3.06 → Works on STe/Mega STE (may partially work on ST)
    TOS 4.04 → Mega STE only
    
  Utility cartridges:
    ROM-DOS → Works on all models (64 KB variant)
    Mega STE diagnostics → Mega STE only (uses 128 KB address decode)
    
  Games/demo cartridges:
    Generally work on all models if ≤ 64 KB
    > 64 KB cartridges work on STe/Mega STE only
```

## 10. Emulation Notes

Emulating the Atari ST cartridge port requires careful handling of ROM mapping, detection logic, and model-specific behavior.

### Steem Emulator Cartridge Support

```
Steem Cartridge Emulation Configuration (steem.ini / steem.cfg):

  [Cartridge]
  CartridgePath = C:/STROM/cartridge.rom    ; Path to cartridge ROM image
  CartridgeSize = 65536                       ; ROM size in bytes (64K or 128K)
  CartridgeModel = ST/STE/MEGASTE             ; Target model
  AutoBoot = 1                                ; Auto-boot from cartridge
  
  ROM image format:
    Raw binary (no header, no padding)
    Size must match CartridgeSize exactly
    Address: $FA0000-$FAFFFF for ST (64 KB)
    Address: $FA0000-$FBFFFF for STe (128 KB)
```

### Emulation ROM Mapping

```
Emulator ROM mapping (address space):

  ST emulation:
    $FA0000-$FAFFFF : Cartridge ROM (64 KB)
    $FB0000-$FBFFFF : Unused (or RAM if extended)
    $FC0000-$FFFFFF : TOS ROM (192-256 KB)
    
  STe emulation:
    $FA0000-$FAFFFF : Cartridge Bank 1 (64 KB, ROM3SEL)
    $FB0000-$FBFFFF : Cartridge Bank 2 (64 KB, ROM4SEL)
    $FC0000-$FDFFFF : RAM / TOS lower
    $FE0000-$FFFFFF : TOS upper
    
  Mega STE emulation:
    $FA0000-$FBFFFF : Cartridge ROM (up to 128 KB)
    $FC0000-$F7FFFF : RAM
    $F80000-$F803FF : IDE/HDC registers
    $F00000-$F7FFFF : STe BIOS (512 KB)
    $F80000-$FFFFFF : Mega STE TOS (256 KB)
```

### Cartridge Detection Emulation

```
Emulating TOS cartridge detection:

  Method 1 — Memory region check (what TOS $F856 does):
    When TOS writes to $FAFFFF and reads back:
      If cartridge ROM present → returns original ROM value (read-only)
      If no cartridge → returns written value (RAM behaves normally)
    
    Emulation: Must preserve original ROM content at cartridge address
    Do NOT reflect writes back. Only serve reads from cartridge ROM image.
    
  Method 2 — Boot vector check:
    TOS reads exception vector at $FA0000-$FA0007:
      SR = $02700 (supervisor mode value)
      PC = $FA0004 (entry point into cartridge ROM)
    
    Emulation: Populate $FA0000-$FA0007 with valid boot vector before
    first boot cycle. Set SR to $02700, PC to cartridge entry point.
    
  Method 3 — Cartridge presence flag:
    TOS checks memvalid ($4F2), memval2 ($43A), memval3 ($51A) in RAM
    to determine cold vs warm boot after cartridge copy.
    
    Emulation: When cartridge boots:
      1. Copy cartridge ROM ($FAxxxx) to RAM at $FC0000
      2. Set memvalid, memval2, memval3 to valid markers
      3. Continue TOS initialization from $FC0000
```

### Model-Specific Emulation Details

```
ST Model Cartridge Emulation:

  Address decode in Glue chip C029144:
    A14 = 0: Selects cartridge region
    ROM3SEL (pin 31) = NOT A13
    ROM4SEL (pin 33) = A13 (but not typically used with ST cartridges)
  
  Emulation key:
    Only map cartridge to $FA0000-$FAFFFF (lower 64 KB)
    Upper bank $FB0000-$FBFFFF should return nothing or be ignored
    ROM3SEL = !A13 → Active when A13 = 0 ($FA0000-$FA7FFF)
    Actually: A13 maps through ROM decode in Glue to ROM3SEL

STe Model Cartridge Emulation:

  Address decode in GST MCU:
    ROM3SEL when A14 = 0 (Bank 1: $FA0000-$FAFFFF)
    ROM4SEL when A14 = 1 (Bank 2: $FB0000-$FBFFFF)
    
  Emulation key:
    Map Bank 1 to $FA0000-$FAFFFF
    Map Bank 2 to $FB0000-$FBFFFF
    Use separate ROM data for each bank (or one 128 KB image)
    US/LDS signals are used as bank select on STe

Mega STE Cartridge Emulation:

  SH2 chip adds additional decode:
    16 MHz clock affects timing requirements
    SH2 handles additional address decode for Mega STE-specific regions
    Cartridge ROM can be up to 128 KB
    
  Emulation key:
    Same as STe (128 KB dual-bank support)
    But ROM speed timing must match 16 MHz bus (faster ROM)
    Consider adding SH2-specific address decode if emulating full STE
```

### ROM Image Format for Emulation Tools

```
Creating a valid bootable cartridge ROM image:

  For TOS upgrade cartridges, the ROM image must contain:

  Offset  | Contents                          | Size
  $00000 | Boot vector: $4EFA $FA000        | 8 bytes
           | (Supervisor return to same ROM)   |
  $00008 | TOS header (version, checksum)     | variable
           | (TOS 2.06: version $0400 at $00)  |
  $00010 | TOS code and data                 | ~192-256 KB
           | (remaining cartridge space)        |

  For a complete TOS 2.06 upgrade cartridge:
    Total size: 65536 bytes (64 KB)
    $00000-$00007: boot vector
    $00008-$01FFF: TOS header + data
    $02000-$FFFF: TOS code

  Note: Most real TOS upgrade cartridges contain only TOS code in the
  ROM space above the boot vector — the RAM copy logic in the boot
  vector code handles expanding to fill the full TOS ROM area.

  For utility/demo cartridges (standalone ROMDOS, games, etc.):
    $00000-$00007: boot vector (must be valid $4EF9 $FA00y pattern)
    Rest: Utility code and data (no TOS header required)
```

### Common Emulation Bugs

```
Known emulation issues with cartridge ROMs:

  1. ROM bank detection
     Some emulators only map Bank 1 ($FA0000-$FAFFFF), ignore Bank 2
     Fix: Map both banks for STe/Mega STE models. Use ROM3SEL/ROM4SEL decode.

  2. Boot vector format
     TOS expects $4EF9 (JSR) instruction at $FA0000+$FA0004
     If vector is malformed, TOS skips cartridge and boots from disk
     Fix: Ensure valid boot vector at $FA0000-$FA0007

  3. Timing issues
     16 MHz Mega STE requires faster ROM access timing than 8 MHz
     Some old EPROM images (120 ns) may not work at 16 MHz
     Fix: Use 50 ns or faster for Mega STE emulation

  4. Address decode boundary issues
     ROM3SEL/ROM4SEL signals must be decoded correctly for $FAxxxx vs $FBxxxx
     Some emulators incorrectly use same decode for both banks
     Fix: ROM3SEL = !A13, ROM4SEL = A13 (both ANDed with A22=1)

  5. UDS/LDS strobe handling
     US (pin 35) and LDS (pin 37) must be properly strobed during bus cycles
     Incorrect strobe timing causes intermittent data errors
     Fix: Assert US/LDS for full data phase of bus cycle
```

### Alternative Emulators

| Emulator | Cartridge Support | Cartridge Format | Notes |
|---|---|---|---|
| **Steem** | Full (ST/STe/Mega STE) | .rom (raw binary) | Most accurate cartridge emulation |
| **SteemHD** | Full | .rom | Hard drive enhanced, same cartridge core |
| ** hatari** | Partial (BIOS override only) | .rom | Emulates BIOS but not cartridge boot |
| **Stella** (not Atari related) | No | — | Atari 2600 emulator |
| **AR Anywhere** | Partial | .st / .car | Partial cartridge support |

### Hatari TOS Cartridge Handling

```
Hatari BIOS/cartridge configuration:

  Hatari does not emulate the cartridge slot as a bootable ROM.
  Instead, Hatari loads a TOS BIOS image directly:
  
  Hatari settings:
    TosImage = /path/to/tos306.rom    ; BIOS image loaded at $FC0000
    TosBaseDir = /path/to/tos/        ; TOS base directory
    GemtaioDir = /path/to/tos/        ; GEMTAIO directory
    
  For cartridge ROM testing:
    - Use Steem for accurate cartridge boot emulation
    - Hatari BIOS image is functionally similar to a TOS upgrade cartridge
    - But Hatari does NOT support ROM3SEL/ROM4SEL dual-bank cartridge
```

## References

- [Atari ST Internals, Ch. 1.3 - Cartridge Port (PDF)](https://www.atarimania.com/documents/Atari-ST-Internals.pdf)
- [Atari ST Programmer's Reference Guide (PDF)](https://info-coach.fr/atari/ressources/doc/st_prog_guide_1.htm)
- [The Atari ST Bus Doc (PDF)](http://info-coach.fr/atari/ressources/doc/Atari%20ST%20bus%20doc.pdf)
- [Master Memory Map (PDF)](https://www.atarimania.com/documents/Master-Memory-Map.pdf)
- [AtariWiki: Memory Map](https://atariwiki.org/wiki/Wiki.jsp?page=Memory%20Map)
- [Atari ST Wiki - Cartridge ROM](https://atariwiki.org/wiki/Wiki.jsp?page=Cartridge_ROM)
- [Info-Coach - STE Hardware](https://info-coach.fr/atari/hardware/STE-HW.php)
- [Atari ST Port Pinout Reference](https://www.atarimania.com/documents/Atari_ST_ports.pdf)
