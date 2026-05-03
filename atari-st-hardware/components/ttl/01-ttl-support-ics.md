# TTL/CMOS Support ICs on Atari ST Motherboards

## Overview of TTL ICs on Atari ST

The Atari ST motherboard (design: Meyer, Burkhard; revision A309493A or later) contains approximately **10 TTL/CMOS support ICs** that handle critical bus interfacing, control signal latching, address decoding, and data routing functions. These chips complement the main microprocessors (Motorola 68000 CPU, 68001 Keyboard Controller, Blitter, and SMC65C22) by providing:

- **Bus direction control** — Octal bus transceivers manage read/write traffic direction between CPU and peripheral buses
- **Control signal latching** — D-type flip-flops capture and hold control signals for timing alignment
- **Address decoding** — Decoders convert CPU address lines into chip-select signals for memory and I/O devices
- **Buffering and isolation** — Buffers isolate high-capacitance loads from CPU pins and provide signal integrity
- **DRAM row address counting** — Binary counters drive DRAM refresh and row-address strobe generation

All ICs operate at **5V TTL logic levels**. Later revisions used 74HC-series parts (HCMOS) for lower power consumption and faster switching, but logic functionality remains pin-compatible.

| Pin | 74LS245 (IC1) | 74LS174 (IC2) | 74LS05 (IC3) | 74LS04 (IC4) | 74LS244 (IC5) | 74LS374 (IC6) | 74LS138 (IC7) | 74LS175 (IC8) | 74HC174 (IC9) |
|-----|---|---|---|---|---|---|---|---|---|
| VCC | 20 | 20 | 14 | 14 | 24 | 20 | 16 | 14 | 14 |
| GND | 8 | 10 | 7 | 7 | 12 | 10 | 8 | 7 | 7 |
| A0-7 | 1,2,3,4,5,6,7,9 | — | — | — | 1,2,3,4,5,6,7,9 | — | — | — | — |
| B0-7 | 15,14,13,12,11,10,9,16 | — | — | — | 23,22,21,19,18,17,16,15 | — | — | — | — |
| Dir | 1 | — | — | — | 20 | 21 | — | — | 11 |
| OE | 19 | 11 | — | — | 1 / 24 | 22 | 6/7 | — | 11 |
| Clk/Latch | — | 11 | — | — | — | 21 | — | 14 | 11 |
| A-input | — | 1,2,3,4,5,6 | 1,2,3,4,5,6 | 1,2,3,4,5,6 | — | — | 1,2,3 | 1,2,3,4 | — |
| A0 output | — | 2,5,10,12,15,20 | 6 | 2 | — | — | 15 | 2 | — |
| A1 output | — | 3 | 5 | 4 | — | — | 14 | 3 | — |
| A2 output | — | 4 | 4 | 6 | — | — | 13 | 4 | — |
| A3 output | — | 5 | 3 | 9 | — | — | — | 5 | — |
| A4 output | — | 6 | 2 | 10 | — | — | — | 6 | — |
| A5 output | — | — | 1 | 11 | — | — | — | — | — |
| A6 output | — | — | — | 13 | — | — | — | — | — |
| Y0-Y7 | — | — | — | — | — | — | 5,4,3,2,1,15,14,13 | — | — |
| CS outputs | — | — | — | — | — | — | 5,4,3,2,1,15,14,13 | — | — |
| Main function | Data bus transceiver | CPU control latch | Clock inverter | Clock buffering | CPU address buffer | Output latch | Address decoder | DMA/control latch | Bus control latch |
| Package | 20-PDIP | 20-PDIP | 14-PDIP | 14-PDIP | 24-PDIP | 20-PDIP | 16-DIP | 14-PDIP | 14-PDIP |

### Bus Transceiver Signal Flow (74LS245)

```
CPU Data Bus (D0-D7)                            Peripheral Data Bus (D0-D7)
+----------------------------+                  +----------------------------+
|                            |                  |                            |
| 74LS245 (IC1)              | Direction = HIGH |                            |
|                            |--------->--------|                            |
| A Port (pins 15,14..) <---|                  |---> B Port (pins 1,2..)    |
|                            | Direction = LOW  |                            |
+----------------------------+                  +----------------------------+

Pin Mapping A Port (Left / CPU Side in read direction):
  Pin 15  -> D0
  Pin 14  -> D1
  Pin 13  -> D2
  Pin 12  -> D3
  Pin 11  -> D4
  Pin 10  -> D5
  Pin 9   -> D6
  Pin 1   -> D7

Pin Mapping B Port (Right / Peripheral Side in read direction):
  Pin 16  -> D7
  Pin 17  -> D6
  Pin 18  -> D5
  Pin 19  -> D4
  Pin 20  -> D3
  Pin 21  -> D2
  Pin 22  -> D1
  Pin 23  -> D0

Control Signals:
  Pin 20  (DIR)  = HIGH  => A->B  (CPU reads from peripherals)
  Pin 20  (DIR)  = LOW   => B->A  (CPU writes to peripherals, or write cycle)
  Pin 19  (OE)   = LOW   => Transceiver enabled
  Pin 19  (OE)   = HIGH  => Bus tri-stated (high-Z)
```

---

## 74LS174 (IC2) — Hex D-type Flip-Flop

**Location:** Near the 68000 CPU, typically on the top side of the PCB.

**Package:** 20-pin DIP

**Function on board:** Captures and holds eight CPU control signals for timing alignment. The 74LS174 is a hex (6 positive-edge-triggered D-type flip-flop) with clear input. On the Atari ST it latches control lines output by the 68000 CPU to provide synchronized control waveforms for memory and I/O devices.

**Signals:**
- **Inputs (D0-D5 = pins 1,2,3,4,5,6):** CPU control line inputs
- **Clock (CLK = pin 11):** Positive edge triggers data capture
- **Clear (CLR = pin 2):** Active-low reset of all flip-flops
- **Outputs (Q0-Q5 = pins 3,5,10,12,15,20):** Latched control signals
- **VCC = pin 20, GND = pin 10**

**Key routed signals (typical):**
- CPU read/write sync signals
- Memory chip-select timing
- I/O control strobes

---

## 74LS05 (IC3) — Hex Inverter (Open Collector)

**Location:** Near the clock generation circuit, typically on top side.

**Package:** 14-pin DIP

**Function on board:** Provides inverted clock signals with open-collector outputs capable of driving higher voltage or multiple loads. Used primarily for clock signal inversion in the system timing circuitry, including generating complementary clock phases for peripherals.

**Signals:**
- **Inputs (pins 1,2,3,4,5,6):** Clock and timing signals
- **Outputs (pins 1,2,3,4,5,6):** Inverted clock signals (open-collector)
- **VCC = pin 14, GND = pin 7**

**Key routed signals:**
- CPU clock inversion (phase generation)
- DRAM refresh clock
- Blitter timing control

---

## 74LS04 (IC4) — Hex Inverter

**Location:** Central area of the motherboard near the clock generator (e.g., ICL8038 or equivalent).

**Package:** 14-pin DIP

**Function on board:** Standard hex inverter used for signal inversion and wave-shaping in clock and reset circuitry. Provides non-inverted and inverted versions of key timing signals.

**Signals:**
- **Inputs (pins 1,2,3,4,5,6):** Raw timing signals
- **Outputs (pins 2,4,6,9,10,11):** Inverted signals
- **VCC = pin 14, GND = pin 7**

**Key routed signals:**
- Non-inverted CPU clock (phi)
- Reset signal conditioning
- Clock distribution to multiple ICs

---

## 74LS244 (IC5) — Octal Buffer

**Location:** Near the CPU, typically on the bottom side of the PCB.

**Package:** 24-pin DIP

**Function on board:** Buffers CPU address bus lines (A16-A23) for driving long traces to DRAM and expansion slots. Also buffers control lines. The 74LS244 provides two banks of four non-inverting buffers each with separate output enables, providing high drive capability for address and control signals.

**Signals:**
- **Inputs G1 (pins 1,2,3,4):** Address/control inputs for bank 1
- **Outputs G1 (pins 18,17,16,15):** Buffered address/control for bank 1
- **Inputs G2 (pins 10,9,8,7):** Address/control inputs for bank 2
- **Outputs G2 (pins 19,21,22,23):** Buffered address/control for bank 2
- **Output Enable G1 (pin 1):** Active-low enable for bank 1
- **Output Enable G2 (pin 24):** Active-low enable for bank 2
- **VCC = pin 24 (actually pin 24 is OE2, VCC is at pin 24 on 24-pin variant - verify), GND = pin 12**

**Key routed signals:**
- CPU address bus A16-A23 (upper address)
- Control signals to DRAM and expansion
- Video address high-order bits

---

## 74LS374 (IC6) — Octal D-type Latch

**Location:** On the bottom side of the motherboard, near bus transceiver IC1.

**Package:** 20-pin DIP

**Function on board:** Latches CPU address lower byte (A0-A7) and data bus for shared address/data multiplexed cycles. During the 68000 address phase, the 74LS374 captures the lower address lines which are subsequently shared as data lines during the data phase.

**Signals:**
- **Data Inputs (D0-D7 = pins 1,2,3,4,5,6,7,9):** CPU bus lines
- **Clock (CLK = pin 21):** Captures data on falling edge (74LS374 is negative-edge triggered)
- **Output Enable (OE = pin 22):** Active-low enable for outputs
- **Outputs (Q0-Q7 = pins 15,14,13,12,11,10,16,17):** Latched bus state
- **VCC = pin 20, GND = pin 10**

**Key routed signals:**
- Lower address/lower data multiplexed bus
- Address/data demultiplexing
- Peripheral address recognition

---

## 74LS138 (IC7) — 3-to-8 Line Decoder

**Location:** Central area of motherboard, close to memory banks.

**Package:** 16-pin DIP

**Function on board:** Primary address decoder for the Atari ST. Converts 3 address lines (A17-A19, tied to STreq/STak signals) into 8 mutually exclusive chip-select outputs. Combined with enable inputs (G1, G2A, G2B) that gate the decode with system control signals, it generates select signals for ROM, RAM, and I/O regions.

**Decode Table:**

| C (pin 1) | B (pin 2) | A (pin 3) | Y0 (pin 5) | Y1 (pin 4) | Y2 (pin 3) | Y3 (pin 2) | Y4 (pin 1) | Y5 (pin 15) | Y6 (pin 14) | Y7 (pin 13) |
|---|---|---|---|---|---|---|---|---|---|---|
| H | H | H | H | H | H | H | H | H | H | L |
| H | H | L | H | H | H | H | H | H | L | H |
| H | L | H | H | H | H | H | H | L | H | H |
| H | L | L | H | H | H | H | L | H | H | H |
| L | H | H | H | H | H | H | L | H | H | H |
| L | H | L | H | H | H | L | H | H | H | H |
| L | L | H | H | H | L | H | H | H | H | H |
| L | L | L | H | L | H | H | H | H | H | H |

**Enable Inputs:**
- **G1 (pin 6):** Active-high — must be HIGH
- **G2A, G2B (pins 7, 4):** Active-low — must both be LOW

**Key routed signals:**
- Y0 = ROM chip select (4x27C010/27C21 EPROMs at C00000h-C0FFFFh)
- Y1 = RAM chip select (6x4164 DRAMs at 000000h-BFFFFFh)
- Y2 = I/O region select (A00000h-A001FFh)
- Y5-Y7 = Expansion slot select

---

## 74LS175 (IC8) — Quad D-type Latch

**Location:** Near control logic area.

**Package:** 16-pin DIP

**Function on board:** Latches 4 control or status signals for DMA and special function control. Lower pin count than the 74LS174, used when fewer control lines need latching.

**Signals:**
- **Data Inputs (D0-D3 = pins 1,2,4,5):** Control inputs
- **Clock (CLK = pin 14):** Positive edge triggers capture
- **Clear (CLR = pin 9):** Active-low reset
- **Outputs (Q0-Q3 = pins 10,12,13,15):** Latched outputs
- **VCC = pin 16, GND = pin 8**

**Key routed signals:**
- DMA request/control latching
- Floppy disk control
- Special system functions

---

## 74HC174 (IC9) — Hex D-type Latch (HCMOS)

**Location:** On motherboard revisions using HCMOS family for lower power.

**Package:** 16-pin DIP

**Function on board:** Like the 74LS174 but in the HC (High-speed CMOS) family. Provides the same hex D-type flip-flop functionality with CMOS input characteristics (higher impedance, lower power). Often used on later Atari ST revisions as a drop-in replacement for the 74LS174 with improved power efficiency.

**Signals:**
- **Data Inputs (D0-D5 = pins 1,2,3,4,5,6):** Latch inputs for bus control
- **Clock (CLK = pin 14):** Rising edge triggers capture
- **Clear (CLR = pin 10):** Active-low clear
- **Outputs (Q0-Q5 = pins 2,3,5,10,12,15):** Latched outputs
- **VCC = pin 16, GND = pin 8**

**Differences from 74LS174:**
- HC speed: 40ns max propagation vs 55ns for LS
- Input impedance: ~10^12 ohms vs ~20K ohms
- Quiescent current: ~1uA vs ~2mA each gate
- Noise margin: ~1.5V vs ~0.4V (better noise immunity)

---

## 74LS163 (IC10) — 4-bit Binary Counter (DRAM Refresh)

**Location:** Near DRAM bank, typically on the bottom side.

**Package:** 16-pin DIP

**Function on board:** Provides a 4-bit binary counter for **DRAM row address counting** during refresh cycles. The Atari ST uses 64Kx1 DRAMs (4164) which require row addresses to be incremented during refresh cycles. This counter increments on each refresh request from the DRAM refresh controller, providing the row address bits R0-R3.

**Signals:**
- **Data Inputs (A, B, C, D = pins 1, 2, 3, 4):** Parallel load inputs
- **Clock (CLK = pin 2):** Positive edge triggers count increment
- **Clear (/CLR = pin 1):** Active-low asynchronous clear
- **Load (/LD = pin 13):** Active-low parallel load control
- **Enable (EN1, EN2 = pins 7, 10):** Both must be HIGH for counting
- **Output (RC / Ripple Clock = pin 15):** Carry output when count rolls over from 15 to 0
- **Outputs (QA, QB, QC, QD = pins 5, 6, 7, 9):** Counter outputs (R0-R3)
- **VCC = pin 16, GND = pin 8**

**Key routed signals:**
- DRAM row address bits R0-R3
- Refresh cycle timing
- Cascade to expand to 8-bit addressing for DRAM rows (typically cascaded with another 74LS163 or 74LS161 for full 8-bit row address)

**Refresh cycle timing:**
```
DRAM Refresh Controller
        |
        | (refresh request, ~15.6kHz)
        v
     74LS163 (IC10)
        |
  +----+----+----+
  |    |    |    |
 R0   R1   R2   R3  -> DRAM row address pins (A0-A3)
 (pin 5)(pin 6)(pin 7)(pin 9)
```

### DRAM Row Address Counter Cascade (Typical Implementation)

```
IC10 (74LS163)                    IC11 (74LS163 - if present)
+----------------+                +----------------+
| QA  (pin 5)   |--- R0          | QA  (pin 5)   |--- R0
| QB  (pin 6)   |--- R1          | QB  (pin 6)   |--- R1
| QC  (pin 7)   |--- R2          | QC  (pin 7)   |--- R2
| QD  (pin 9)   |--- R3          | QD  (pin 9)   |--- R3
| RC  (pin 15)  |---> CLK        | QA  (pin 5)   |--- R4
+----------------+     IC11 CLK   | QB  (pin 6)   |--- R5
                                   | QC  (pin 7)   |--- R6
                                   | QD  (pin 9)   |--- R7
                                   +----------------+
                                        |
                                    DRAM pins A0-A7
```

---

## Summary Table of All TTL Support ICs

| IC# | Part Number | Family | Package | Pins | Function | Key Output Signals | Location |
|-----|-------------|--------|---------|------|----------|-------------------|----------|
| IC1 | 74LS245 | TTL | 20-PDIP | 20 | Octal bus transceiver (directional buffer) | Data bus D0-D7 (A->B or B->A controlled by DIR/OE) | Near CPU, top side |
| IC2 | 74LS174 | TTL | 20-PDIP | 20 | Hex D-type flip-flop | Latched CPU control signals (6 FF) | Near CPU, top side |
| IC3 | 74LS05 | TTL | 14-PDIP | 14 | Hex inverter (open-collector) | Inverted clock/timing signals (open-collector) | Clock circuit area |
| IC4 | 74LS04 | TTL | 14-PDIP | 14 | Hex inverter | Inverted timing signals, reset conditioning | Clock circuit area |
| IC5 | 74LS244 | TTL | 24-PDIP | 24 | Octal buffer (dual 4-channel) | Buffered address bus A16-A23, control lines | Near CPU, bottom side |
| IC6 | 74LS374 | TTL | 20-PDIP | 20 | Octal D-type latch | Latched lower address/data (A0-A7) | Bottom side, near bus transceiver |
| IC7 | 74LS138 | TTL | 16-DIP | 16 | 3-to-8 decoder | ROM select (Y0), RAM select (Y1), I/O select (Y2), Expansion select (Y5-Y7) | Near memory banks |
| IC8 | 74LS175 | TTL | 14-PDIP | 14 | Quad D-type latch | Latched DMA/control signals (4 FF) | Control logic area |
| IC9 | 74HC174 | HCMOS | 16-PDIP | 16 | Hex D-type latch (CMOS) | Latched bus control signals (6 FF) | Later revisions, replaces LSB174 |
| IC10 | 74LS163 | TTL | 16-PDIP | 16 | 4-bit binary counter | DRAM row address (R0-R3), refresh counter | Near DRAM bank |

### IC Family Distribution

| Family | Count | Purpose |
|--------|-------|---------|
| 74LS (TTL) | 9 | Primary logic — bus interfacing, timing, decoding |
| 74HC (HCMOS) | 1 | Lower-power replacement on later revisions |
| **Total** | **10** | |

### Power Consumption Estimate

| Type | Per-IC Quiescent Current | Total (10 ICs) |
|------|------------------------:|--------------:|
| 74LS (all LS) | ~8 mA per IC | ~80 mA |
| Mixed (9xLS + 1xHC) | ~73 mA | ~73 mA |
| All HC | ~2 mA per IC | ~20 mA |

The Atari ST's use of mixed TTL/CMOS design was characteristic of mid-1980s computer architecture, balancing speed (TTL) with power efficiency (HC) while maintaining CMOS-compatible input thresholds.
