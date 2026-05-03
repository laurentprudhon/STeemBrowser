# Custom Silicon Chips — Functional Overview

> Functional description of each custom silicon chip: what it does, what controls it (inputs), what it produces (outputs). See detailed pages for register maps, pinouts, and timing.

## Overview

The Atari ST line uses custom ASICs to implement the functions that competitors implemented with dozens of discrete TTL/CMOS chips. The original ST uses four PLCC68 chips; the STe/MegaSTE consolidate them into two PLCC144 packages.

```
Original ST (1985)                          STe / MegaSTE (1989+)
┌── Glue (C029144) ───────────────────┐    ┌── GST MCU C302183 ────────────────┐
│ Bus gateway, chip select, sync,       │    │ (GLUE + MMU + DMA + Blitter)      │
│ clock gen, DRAM refresh               │    │ (single 144-pin PLCC package)      │
└───────────────────────────────────────┘    └──────────────────────────────────────┘
┌── MMU (C028300) ────────────────────┐    ┌── GST Shifter C029145 ────────────┐
│ DRAM controller, scroll engine,       │    │ Super Hi-Res video, 12-bit DAC,  │
│ page mapping, MAD bus output          │    │ stereo sound DAC, 3-bus arch       │
└───────────────────────────────────────┘    └──────────────────────────────────────┘
┌── DMA (C029128) ────────────────────┐    
│ Bus arbitration, FDC/HDC handshake,  │    ┌── SH2 C301842 ────────────────────┐ (MegaSTE only)
│ stereo DMA for sound (STe)            │    │ 16 MHz clock gen, IDE controller  │
└───────────────────────────────────────┘    └──────────────────────────────────────┘
┌── Shifter (C028787) ─────────────────┐
│ Video D/A, pixel clock, sync gen,     │
│ 16-channel audio mixer                 │
└───────────────────────────────────────┘
```

| File | Architecture / Detail Level | Reference |
|------|------|------|
| `01-system-overview.md` | Family history, model comparison, boot sequence | High level |
| `02-motherboard-and-electronic-components.md` | Board layout, component inventory, bus architecture | Board level |
| **`04-custom-silicon-chips.md`** | **Each chip's function, inputs, outputs, data flow** | **Functional level (this file)** |
| `03-custom-silicon.md` | Detailed reference: signal lists, address maps, timing | Implementation level |
| `components/gst/01-gst-mcu-c302183.md` | GST MCU C302183 register map, emulation | GST MCU detailed |
| `components/video/02-gst-shifter-c029145.md` | GST Shifter video modes, timing | GST Shifter detailed |
| `components/blitter/02-blitter-gst-integration.md` | Blitter registers, operations | Blitter detailed |

---

## 1. Glue Logic (C029144)

### Functional Description

The Glue chip is the **system bus gateway**. It arbitrates all memory and I/O activity between the MC68000 CPU and every other chip on the board. It is the first piece of silicon the CPU talks to on every bus cycle.

**Primary functions:**
1. **CPU bus bridge** — Converts MC68000 bus cycles (AS, R/W, address, data) into signals understood by downstream chips
2. **Address decoder** — Takes A0-A23 from the CPU and produces active-low chip-select for ROM, RAM regions, and all I/O ports
3. **DRAM refresh controller** — Generates RAS0/RAS1 for both DRAM banks (odd/even). Refresh runs autonomously every ~3.9 us without CPU intervention
4. **Clock generator** — Divides the 8 MHz crystal input into usable clock signals: 8 MHz, 4 MHz (DRAM timing), 2 MHz (Shifter), 500 kHz (horizontal blank)
5. **Video sync generator** — Counts pixel clocks to generate HSYNC and VSYNC at correct NTSC/PAL frequencies, drives DIN13 monitor
6. **Chip select engine** — Produces MFPCS (MFP select), 6850CS (ACIA select), ROM1-6 (TOS ROM select), DE (data enable), and more
7. **Reset distributor** — Generates the master RESET pulse to every chip on the board
8. **Interrupt bridge** — Routes BLANK (horizontal blank), HSYNC, VSYNC to the MC68000 IPL interrupt pins

### Input Signals

| Signal | Source | Function |
|--------|------|------|
| A0-A23 | MC68000 CPU | Address bus — current address being accessed |
| D0-D15 | MC68000 CPU | Data bus — data for this cycle |
| AS (active low) | MC68000 CPU | Address Strobe — asserts when address phase begins |
| R/W | MC68000 CPU | 1 = read cycle, 0 = write cycle |
| FC0-FC2 | MC68000 CPU | Function code — user/supervisor, program/data type |
| UDS/LDS (active low) | MC68000 CPU | Upper/lower data strobe — byte enable for upper/lower byte |
| DTACK (active low) | Downstream chip | Done Acknowledge — peripheral confirms data available |
| BERR (active low) | Downstream chip | Bus Error — peripheral signals error |
| BR (active low) | DMA chip | Bus Request — DMA wants control of the bus |
| CLK 8 MHz | Crystal Y1 | System clock input |
| !RESET (active low) | Reset circuit | External reset button / power-on reset |

### Output Signals

| Signal | Destination | Function |
|--------|-----------|------|
| A13-A23 | MMU, DMA, Shifter | Upper address lines passed through from CPU |
| AS (active low) | MMU, DMA, Shifter | Address strobe relayed to downstream |
| R/W | MMU, DMA | Read/write signal relayed |
| UDS/LDS | MMU | Byte enabler for data access |
| RAS0 (active low) | MMU / DRAM bank 0 | Row address strobe for odd bank |
| RAS1 (active low) | MMU / DRAM bank 1 | Row address strobe for even bank |
| CAS0_L, CAS0_M, CAS1_L, CAS1_M | MMU / DRAM | Column address strobes for both banks |
| CLK_8MHZ | MMU, DMA, Shifter | 8 MHz clock distribution |
| CLK_4MHZ | MMU, DMA | 4 MHz for DRAM refresh timing |
| CLK_2MHZ | Shifter | 2 MHz for pixel clock divider |
| CLK500KHZ | Shifter / sync | 500 kHz for horizontal blank generation |
| MFPCS (active low) | MC68901 MFP | Chip select for MFP at $FF8000 range |
| 6850CS (active low) | MC6850 ACIA | Chip select for ACIA at $FF8C00 range |
| ROM1-6 (active low each) | TOS ROM chips 1-6 | Individual chip selects for 6-chip TOS ROM |
| DE (Data Enable) | All chips | Enables data bus drivers during bus cycle |
| DTACK_out | MC68000 CPU | DTACK response (if Glue services the cycle) |
| BLANK (IPL1) | MC68000 IPL1 | Horizontal blank for raster sync interrupts |
| HSYNC sync | MC68000 IPL2 | Horizontal sync pulse |
| VSYNC sync | MC68000 | Vertical sync pulse |
| MFPINT | MC68000 IPL0/1/2 | MFP interrupt output to CPU via IPL |
| RESET_out | All chips | Master reset pulse to every IC |
| BR_out | MC68000 BR pin | Bus request from DMA forwarded to CPU |
| BG_out (active low) | DMA chip | Bus Grant acknowledgment to DMA |
| BGACK_out (active low) | DMA | CPU's Bus Grant Acknowledge signal |
| MMU_CS (active low) | MMU chip | Chip select for MMU |
| FDC_CS (active low) | WD1772 FDC | Chip select for FDC at $FF8600 range |
| PSG_CS (active low) | YM2149 PSG | Chip select for PSG at $FF8800 range |
| DMA_CS (active low) | DMA chip | Chip select for DMA at $FF8A00 range |
| Shifter_CS (active low) | Shifter chip | Chip select for Shifter at $FFC000 range |

### Data Flow

```
CPU bus cycle:
  CPU writes address to A0-A23 → Glue decodes address → Glue asserts chip select to target
  CPU puts data on D0-D15 → Glue routes via DE → downstream chip receives data
  Downstream asserts DTACK → Glue forwards DTACK to CPU → cycle completes
```

---

## 2. MMU (C028300)

### Functional Description

Despite its name, the Atari MMU is neither a memory management unit with paging and protection nor a virtual memory controller. It is a **DRAM controller** that handles refresh, address multiplexing (row/column), scroll, and page mapping for the 41256/414616 DRAM chips.

**Primary functions:**
1. **DRAM RAS/CAS generation** — Produces precise timing for row address strobe and column address strobe pulses to all DRAM chips
2. **Address multiplexing** — Takes the 68000's full address and generates row (MAD0-MAD11 via RAS) and column (MAD12-MAD14 via CAS) addresses
3. **Scroll engine** — Contains 8 × 12-bit scroll registers that add to addresses flowing through the chip, enabling smooth horizontal/vertical scrolling without CPU intervention
4. **Page mapping** — Maps physical DRAM pages to logical address space; used to map the 32 KB video RAM at $A0000
5. **DRAM bank interleaving** — Manages odd/even bank access (D0-D7 vs D1-D15, D9-D15)
6. **CAS counter** — Internal CAS register tracks row/column timing

### Input Signals

| Signal | Source | Function |
|--------|---------|------|
| A8-A23 | Glue relay from CPU | Address lines (lower bits A8-A11 handled separately) |
| D4-D7 (I/O) | 68000 data bus | Scroll register write data (MMU accessed via special MMU cycle) |
| AS (active low) | Glue | Address Strobe for MMU cycle |
| R/W | Glue | Read/Write direction for scroll registers |
| MMU_CS (active low) | Glue | Chip select — tells MMU this cycle is for MMU, not RAM |
| CLK_8MHZ | Glue | 8 MHz clock for internal logic |
| CLK_4MHZ | Glue | 4 MHz for DRAM refresh timing |
| UDS/LDS (active low) | Glue | Byte enablers for scroll register writes |
| RAS0 (active low) | Glue input | RAS signal (Glue drives this, MMU amplifies/distributes) |
| CAS (active low) | Glue input | CAS signal (Glue drives this, MMU amplifies/distributes) |

### Output Signals

| Signal | Destination | Function |
|--------|-----------|------|
| MAD0-MAD11 | DRAM row pins | Row address output (via RAS) — selects one of 1024 rows |
| MAD12-MAD14 | DRAM column pins | Column address output (via CAS) — selects column within row |
| RAS0_out (active low) | Odd DRAM bank chips | Row address strobe for bank 0 (odd bytes) |
| RAS1_out (active low) | Even DRAM bank chips | Row address strobe for bank 1 (even bytes) |
| CAS0_L_out (active low) | Odd DRAM bank | Column strobe low byte for odd bank |
| CAS0_M_out (active low) | Odd DRAM bank | Column strobe mid byte for odd bank |
| CAS0_M_out | Even DRAM bank | Column strobe mid byte for even bank |
| CAS1_L_out (active low) | Even DRAM bank | Column strobe high byte for even bank |
| MAD0..MAD11 | Shifter (MAD bus) | DRAM address bus shared with Shifter for video |
| W/E_out (active low) | Odd DRAM bank | Write Enable for odd bank during writes |
| WDAT_out (to odd bank) | DRAM D0-D7 | Write data to odd bank DRAM chips |
| MAD39..MAD43 | Shifter | MAD signals passed through to Shifter for video read |

### Data Flow

```
MMU scroll register write cycle:
  CPU writes to $FFE0xx (MMU cycle) → Glue asserts MMU_CS
  → MMU latches scroll value D4-D7 on R/W falling edge
  → Scroll value stored in 12-bit register

MMU DRAM access cycle:
  CPU writes address A8-A23 → Glue relay
  → MMU adds scroll value to address
  → MMU splits address into row (MAD0-MAD11) + column (MAD12-MAD14)
  → MMU asserts RAS0 or RAS1 → DRAM takes row
  → MMU asserts CAS strobes → DRAM latches column
  → Data on MAD bus flows to/from DRAM

MMU MAD bus to Shifter:
  MAD0-MAD11 → MAD11_MAD0 bus to Shifter for video RAM read
  → Shifter reads framebuffer pixel data directly from DRAM
```

---

## 3. DMA Controller (C029128)

### Functional Description

The DMA chip is the **bus arbiter** between the CPU and peripherals for shared memory access. When a device needs to transfer blocks of data through the system bus, it uses DMA rather than the CPU.

**Primary functions:**
1. **Bus arbitration** — Seizes control of the 68000 bus when a peripheral requests it, holds bus during transfer, then releases it
2. **Floppy DMA handshake** — Manages FDRQ/FDACK between WD1772 FDC and system RAM (for sector read/write)
3. **Hard disk DMA handshake** — Manages HDRQ/HDAK between the DB19 port (ACSI) and system RAM
4. **STe stereo DMA** — Manages 8-bit stereo sound sample DMA in the STe
5. **DMA cycle generation** — Generates its own DTACK, R/W, and address signals while the bus is in DMA mode

### Input Signals

| Signal | Source | Function |
|--------|--------|------|
| FDRQ (active low) | WD1772 FDC | Floppy DMA Request — FDC needs data |
| FDACK_out (active low) | DMA → FDC | Floppy DMA Acknowledge — DMA says "I'm coming" |
| HDRQ (active low) | DB19 ACSI HDD | Hard Disk DMA Request |
| HDAK_out (active low) | DMA → DB19 HDD | Hard Disk DMA Acknowledge |
| STe DMA request (FDRQ2) | STe sound DAC | Stereo sound sample DMA request |
| !FDCS_out (active low) | DMA → FDC | Floppy DMA chip select |
| !HDCS_out (active low) | DMA → DB19 | Hard disk DMA chip select (DB19) |
| R/W_out (F) | DMA | Floppy DMA read/write direction |
| !R/W (F) | DMA | Hard disk DMA read/write direction |
| A1_out (F) | DMA | Floppy DMA address line A1 |
| DE (active low) | Glue | Data Enable — tells DMA bus is active |
| BGACK (active low) | CPU | Bus Grant Acknowledge — CPU surrenders bus |
| BR (active low) | CPU | Bus Grant — Glue grants DMA bus request |
| AS (active low) | Glue | Address Strobe (during DMA mode) |
| R/W | Glue | Read/Write (during DMA mode) |
| D0-D15 | Glue | Data bus (bidirectional via DMA) |

### Output Signals

| Signal | Destination | Function |
|--------|-----------|------|
| FDRQ_out (active low) | WD1772 FDC | Floppy DMA Request to FDC |
| FDACK_out (active low) | WD1772 FDC | Floppy DMA Acknowledge from DMA |
| !FDCS_out (active low) | WD1772 FDC | Floppy chip select during DMA |
| HDRQ_out (active low) | DB19 ACSI HDD | Hard Disk DMA Request to HDD |
| HDAK_out (active low) | DB19 ACSI HDD | Hard Disk DMA Acknowledge from DMA |
| !HDCS_out (active low) | DB19 ACSI HDD | Hard disk chip select during DMA |
| R/W_out (F) | FDC | Floppy DMA read/write direction |
| R/W_out (F) | DB19 HDD | HDD DMA read/write direction |
| A1_out (F) | FDC | Floppy DMA address line |
| D0-D15 | DRAM banks | DMA data to/from DRAM |
| D2-D15 | DB19 HDD port | DMA data to HDD |
| First-word cycle signal | FDC | Special first-word cycle for command blocks |

### Data Flow

```
CPU-driven DMA setup:
  CPU writes DMA control register at $FF8A00 → Glue routes to DMA
  → DMA sets direction, count, address via DMA registers

DMA transfer setup (floppy example):
  1. CPU sets address ($FF8A04-$FF8A07) and count ($FF8A08-$FF8A0B) registers
  2. CPU enables DMA (bit 0 of $FF8A00 = 1)
  3. WD1772 asserts FDRQ (active low) when it needs data

DMA bus takeover:
  DMA sees FDRQ asserted → DMA asserts BR (Bus Request) to CPU
  → CPU finishes current cycle → CPU asserts BGACK
  → DMA takes over: asserts DE, generates its own R/W, address
  → Data flows: DRAM ↔ WD1772 (or DB19 HDD)

DMA transfer complete:
  Peripheral de-asserts DRQ → DMA releases bus
  → CPU resumes control of bus
  → DMA status register at $FF8A02 updated
```

---

## 4. Shifter (C028787-2) — Original ST

### Functional Description

The Shifter is Atari's **video display controller**. It reads pixel data directly from system RAM ($A0000-$A7FFF) and generates synchronized RGB video output, sync pulses, and audio — all in real-time with no CPU involvement during vertical blank.

**Primary functions:**
1. **Video DMA engine** — Reads framebuffer from system RAM each scanline at pixel clock rate (32 MHz)
2. **Digital-to-analog converter (9-bit)** — Converts palette indices to analog RGB voltages for DIN13 monitor
3. **Sync signal generator** — Generates HSYNC, VSYNC, and Display Enable (DE) for the monitor
4. **Display mode controller** — Configures LORAM (320x200), MORAM (640x200), or HIRE (640x400) modes
5. **Address counter** — 23-bit video address counter auto-increments each half-line (no CPU needed)
6. **Color palette lookup** — Reads 16 palette entries from start of each scanline, maps pixel indices to RGB
7. **16-channel audio mixer** — Mixes audio from 16 volume registers at $FFC100
8. **Dot Enable (DE)** — Video timing signal controlling when pixels are active (vs. blanking)

### Input Signals

| Signal | Source | Function |
|--------|---------|------|
| D0-D15 | 68000 data bus | Shifter control register writes ($FFC000-$FFC7FF) |
| AS (active low) | Glue | Address Strobe |
| R/W | Glue | Read/Write for control registers |
| A5-A0, A3-A1 | Glue | Address bus for control register selection |
| CS (active low) | Glue | Chip select for Shifter ($FFCxxx) |
| CLK_16MHZ | Glue | 16 MHz half-pixel clock (from 32 MHz crystal) |
| MONO | Glue | Monochrome detect — if HIGH, force all pixels to single color |
| HSYNC (active low) | Glue | Horizontal sync pulse input |
| VSYNC (active low) | Glue | Vertical sync pulse input |
| BLANK (active low) | Glue | Horizontal blank detect — blank pixels during blanking |
| XTL0 | Crystal Y2 | 32 MHz crystal input pin 1 |
| XTL1 | Crystal Y2 | 32 MHz crystal input pin 2 |
| MAD0-MAD11 | MMU (MAD bus) | DRAM address bus for video RAM reads |
| R/W | MMU | Read/Write for video RAM accesses |
| DE_internal | Shifter internal | Display Enable signal (output driving external DE) |
| PixelClock | Shifter internal (PLL) | 32 MHz pixel clock derived from Y2 |

### Output Signals

| Signal | Destination | Function |
|--------|-----------|------|
| R2, R1, R0 (analog) | DIN13 Monitor (pin 4-6) | Red DAC 3-bit outputs (8 levels) |
| G2, G1, G0 (analog) | DIN13 Monitor (pin 9-11) | Green DAC 3-bit outputs (8 levels) |
| B2, B1, B0 (analog) | DIN13 Monitor (pin 12-14) | Blue DAC 3-bit outputs (8 levels) |
| HSYNC_out | DIN13 Monitor (pin 2) | Horizontal sync to monitor |
| VSYNC_out | DIN13 Monitor (pin 17) | Vertical sync to monitor |
| DE_out | DIN13 Monitor (pin 3) | Display Enable (HIGH = active video) |
| AUDIO_out | Speaker / amp | Mixed audio output from Shifter's 16-channel mixer |
| PixelClock_decrement | Internal counter | Drives pixel raster for each scanline |

### Data Flow

```
Video scanline rendering:
  Shifter reads address from internal video address counter
  → Shifter asserts R/W read to MAD bus → MMU/MAD bus → DRAM
  → DRAM returns 16-bit word → Shifter decodes into 8-4-2 pixels (mode dependent)
  → Shifter looks up pixel index in 16-entry palette (read from RAM)
  → Shifter converts palette entry to RGB analog via 3-bit DAC per channel
  → DAC outputs R2-R0, G2-G0, B2-B0 → DIN13 monitor pins
  → Address counter auto-increments by 8 words per line
  → At end of lines, sync pulse generated for next scanline

Audio mixing:
  For each audio sample → Shifter reads volume from $FFC100-$FFC10F
  → Volume applied to channel's amplitude
  → All active channels summed together
  → Single analog audio stream sent to audio amplifier
```

---

## 5. GST MCU C302183 (STe / MegaSTE)

### Functional Description

The GST MCU C302183 is a **system-on-chip** that consolidates the original four separate ASICs (Glue, MMU, DMA) into a single 144-pin PLCC plus adds the STe blitter engine and I/O extensions.

**Primary functions:**
1. **GLUE Block** — Inherits Glue's bus gateway, chip select, sync gen, clock div, and refresh functions
2. **MMU Block** — Inherits MMU's DRAM refresh, scroll engine, page mapping, MAD bus output
3. **DMA Block** — Inherits DMA's bus arbitration, FDC/HDC handshake, stereo sound DMA
4. **Blitter Engine (NEW in STe)** — 2-bit block transfer engine with 16 Boolean ops, halftone RAM, skew, masks
5. **I/O / A/D Block (NEW in STe)** — Joystick/paddle/pen A/D converters, genlock input, STe-specific control registers, Super Hi-Res mode support

### Input Signals

| Signal | Source | Function |
|--------|--------|------|
| D0-D15 | 68000 CPU | Full data bus access to all internal blocks |
| A0-A23 | 68000 CPU | Full address bus — all blocks respond to decoded address ranges |
| AS (active low) | 68000 CPU | Address Strobe for all blocks |
| R/W | 68000 CPU | Read/Write for all internal blocks |
| FC0-FC2 | 68000 CPU | Function code — supervisor/user, program/data |
| RESET (active low) | Reset circuit | System reset to all internal blocks |
| CLK | Y1 crystal / SH2 (MegaSTE) | 8 MHz (or 16 MHz on MegaSTE) clock input |
| HSYNC (active low) | GST MCU internal block | Internal sync fed to GST Shifter |
| VSYNC (active low) | GST MCU internal block | Internal sync fed to GST Shifter |
| DE | GST MCU internal block | Display Enable fed to GST Shifter |
| MAD0-MAD11 | GST MCU MMU block | Internal MAD bus to GST Shifter |
| RAS0/RAS1 (active low) | GST MCU GLUE block | DRAM refresh RAS signals to DRAM banks |
| CAS0/CAS1 (active low) | GST MCU GLUE block | DRAM refresh CAS signals to DRAM banks |

### Output Signals

| Signal | Destination | Function |
|--------|-----------|------|
| D0-D15 | 68000 CPU | Data bus output from all internal blocks |
| A0-A23 | 68000 CPU | Address bus routed to 68000 |
| R/W | 68000 CPU | Bus read/write (shared with CPU during DMA) |
| AS (active low) | 68000 CPU | Address Strobe (shared with CPU) |
| DE_out (DIN13 pin 3) | Monitor | Display Enable output to DIN13 |
| HSYNC_out (DIN13 pin 2) | Monitor | Horizontal sync to DIN13 |
| VSYNC_out (DIN13 pin 17) | Monitor | Vertical sync to DIN13 |
| RESET_out | All chips | System reset to all ICs |
| BR | 68000 CPU BR pin | Bus Request to CPU (via BLR) |
| BG (active low) | DMA peripherals | Bus Grant to peripherals |
| BGACK (active low) | DMA peripherals | Bus Grant Ack from peripherals |
| R/W DMA | Peripherals | DMA read/write output |
| D0-D15 DMA | Peripherals | DMA data bus to peripherals |
| MAD0..MAD11 | GST Shifter C029145 | DRAM address bus for video |
| R/W (MAD) | GST Shifter | DRAM read/write for video |
| DE_MAD | GST Shifter | Data Enable for MAD bus to Shifter |
| HSYNC to Shifter | GST Shifter | Sync pulse to GST Shifter |
| VSYNC to Shifter | GST Shifter | Sync pulse to GST Shifter |
| JOYA0, JOYA1 | DB9 joystick | Joystick A x-axis input (5-bit ADC) |
| JOYB0, JOYB1 | DB9 joystick | Joystick A y-axis input (5-bit ADC) |
| PADDLE0..1 | DB9 joystick | Paddle inputs (5-bit ADC) |
| PEN_DATA | DB9 pen port | Optical pen data input |
| GENLOCK | Genlock input | External sync genlock input |
| REG5060 | GST Shifter | 50/60 Hz mode select for GST Shifter |
| CMPCS | GST Shifter | Cycle period select for GST Shifter |

### Internal Block Diagram

```
+-------------------[GST MCU C302183]---(144-pin PLCC)------------------+
|                                                                      |
|  +--[GLUE Block]----+  +--[MMU Block]---+  +--[DMA Block]---+      |
|  | Clock gen, sync  |  | Scroll 8x12bit |  | FDRQ/Ack       |      |
|  | addr decode      |  | Page mapping   |  | HDRQ/HDAK      |      |
|  | refresh (RAS/   |  | MAD output     |  | stereo DMA     |      |
|  |  CAS)            |  | MAD bus (to    |  | DMA cycle gen  |      |
|  | CS generators    |  |  Shifter)      |  |                |      |
|  +-----+----+-------+  +-----+----+-----+  +-----+----+-----+      |
|        |    |                 |    |               |    |          |
|        |    |                 |    +---------------|    |          |
|        +----+-----------------+--------------------+    |          |
|                       |                                 |          |
|             +------[COMMON GST BUS]-----+              |          |
|             |   (internal register bus  |              |          |
|             |    / multiplexer / mux)   |              |          |
|             +---------------------------+              |          |
|                                                    +----+--+         |
|             +--------------------------------------+  [Blitter]  |
|             |   +--[Blitter Block]----+            |  [Engine]    |
|             |   | Src/Dest Addr       |            |  [FIFO]      |
|             |   | X/Y Count           |            |  [ALU]       |
|             |   | Halftone RAM (8 wrd)|            |  [OP code]   |
|             |   | Skew register       |            |  [Logic]     |
|             |   | EndMask 1,2,3       |            |              |
|             |   | HOP, OP, Line Ctrl  |            |              |
|             |   +-----+-----------+---+            |              |
|             +---------|           |-----+          |              |
+-----+----|  Joystick   |  Pen     |-----+----+     |              |
          |  [I/O Block]  |  [ADC]   |     |    |     |              |
          +--+-------+---++-+-------++-----+--+-+      v              |
             |       |                 |       |                  |      |
             v       v                 v       v                  v       |
       [A0-A23]  [D0-D15]        [RAS/CAS] [MAD]           [SYNC/DE]   |
       [AS]   [RESET]            [HSYNC]   [VSYNC]          [JOY/ADC]   |
       [R/W]  [BR/BG/BGACK]     [PEN]     [GENLOCK]       [REG5060]   |
       [DTACK] [SIZ]             [IACK]    [DACK]          [EOP]       |
```

---

## 6. GST Shifter C029145 (STe)

### Functional Description

The GST Shifter replaces the original C028787 Shifter with **12-bit DAC**, **Super Hi-Res video**, **stereo sound**, and a **3-bus architecture** for independent access by 68000, RAM, and sound DMA.

**Primary functions:**
1. **12-bit DAC** — 4-bit R + 4-bit G + 4-bit B per channel (4,096 colors vs. original 512)
2. **Multi-mode video** — Adds STe modes to original LORAM/MORAM/HIRE: MCM 16-color, 640x512 interlaced Super Hi-Res, double-density 640x256
3. **Stereo sound DAC** — 8-bit stereo samples via DMA at 4 sample rates (6,250 / 12,500 / 25,000 / 50,000 Hz)
4. **3-bus architecture** — Independent 68000 bus, RAM bus, and sound DMA bus
5. **Word-level addressing** — 40 words per half-line for 640-line modes (vs. byte-level in original)
6. **Super Hi-Res counter** — 24-bit video address counter with frame start/fend registers
7. **Horizontal fine-scan** — Pixel-skip scroll via HSCROLL register
8. **16-color palette** — 16 palette entries, each 12-bit (4,096 possible), 16 simultaneous on-screen

### Input Signals

| Signal | Source | Function |
|--------|---------|------|
| D0-D15 | 68000 via GST MCU | GST Shifter control register writes ($FF8200-$FF823F) |
| AS (active low) | GST MCU relays | Address Strobe for registers |
| R/W | GST MCU relays | Read/Write for registers |
| CS (active low) | GST MCU relays | Chip select for GST Shifter ($FF8xxx) |
| MAD0..MAD11 | GST MCU MMU/Internal | DRAM address for video RAM read (via MAD bus) |
| R/W (MAD) | GST MCU relays | Read/Write for video RAM |
| DE (from GST MCU) | GST MCU internal | Display Enable — HIGH = active video period |
| HSYNC (from GST MCU) | GST MCU internal | Horizontal sync pulse input |
| VSYNC (from GST MCU) | GST MCU internal | Vertical sync pulse input |
| SCLK (8 MHz) | GST MCU / clock gen | 8 MHz Sound Clock for stereo DMA sample timing |
| XTL0/XTL1 | Internal or ext | 32 MHz crystal input (replaced by 8 MHz SCLK on STe) |
| PixelClock (16 MHz) | GST Shifter internal | Pixel clock (derived from SCLK /2) |
| ColorClock (16 MHz) | GST Shifter internal | Color clock for pixel data (derived from 32 MHz /2) |
| FCSEL (from register) | GST Shifter internal | Frame Counter SElect — horizontal pixel-skip offset |
| FBASE (from register) | GST Shifter internal | Frame Start Address — base pointer to video RAM |
| FEND (from register) | GST Shifter internal | Frame End Address — end pointer for VRAM bounds |
| REG5060 (from GST MC) | GST MCU internal | 50/60 Hz mode select |
| CMPCS | GST MCU | Cycle period select (sync/generation mode) |

### Output Signals

| Signal | Destination | Function |
|--------|-----------|------|
| R3-R0 (analog) | DIN13 Monitor (pins) | Red DAC 4-bit outputs (16 levels) |
| G3-G0 (analog) | DIN13 Monitor (pins) | Green DAC 4-bit outputs (16 levels) |
| B3-B0 (analog) | DIN13 Monitor (pins) | Blue DAC 4-bit outputs (16 levels) |
| HSYNC_out | DIN13 (pin 2) | Horizontal sync to monitor |
| VSYNC_out | DIN13 (pin 17) | Vertical sync to monitor |
| DE_out | DIN13 (pin 3) | Display Enable to monitor |
| AUDIO_L_out | Speaker/amp | Left channel stereo audio output (8-bit PCM) |
| AUDIO_R_out | Speaker/amp | Right channel stereo audio output (8-bit PCM) |
| SCLK_sync | GST MCU | Sound clock sync feedback |
| STEREO_DAC_OUT | Internal | 8-bit stereo sample DAC output |
| SUPER_HIRE_out | Mode select | Super Hi-Res mode enable signal |
| Interlace_field | Timing logic | Odd/even field select for interlaced modes |
| MCM_ena | Mode select | Multi-Color Mode enable signal |

---

## 7. GST MCU I/O Block (Joystick, Pen, ADC)

### Functional Description

The GST MCU adds a new **I/O block** with analog-to-digital converters for the STe's enhanced joystick/paddle ports and pen port.

**Primary functions:**
1. **Joystick/paddle ADC (2 × 5-bit)** — Converts analog voltages from DB9 joysticks/paddles to digital values
2. **Pen port ADC** — Converts optical pen position data to digital values
3. **Genlock input** — Synchronizes STe video to external source (e.g., VTR)
4. **Port A/B direction control** — Configurable I/O port directions for STe extensions

### Input Signals

| Signal | Source | Function |
|--------|---------|------|
| JOYA0, JOYA1 | DB9 Joystick | X-axis input from joystick A (analog voltage) |
| JOYB0, JOYB1 | DB9 Joystick | Y-axis input from joystick A |
| PADDLE0..1 | DB9 Joystick | Paddle inputs (analog) |
| PEN_DATA | DB9 Pen port | Optical pen position data |
| GENLOCK | External (VTR/etc.) | External sync reference for genlock |
| Port A/B direction | Software | Configurable as I/O or analog input |

### Output Signals

| Signal | Destination | Function |
|--------|-----------|------|
| Joystick 0 X | GST MCU internal registers | Digital X value from ADC (0-31) |
| Joystick 0 Y | GST MCU internal registers | Digital Y value from ADC (0-31) |
| Joystick 1 X | GST MCU internal registers | Digital X value (0-31) |
| Joystick 1 Y | GST MCU internal registers | Digital Y value (0-31) |
| Paddle 0..3 | GST MCU internal registers | Digital paddle values (0-31 each) |
| Genlock state | GST MCU control reg | Genlock status flag |
| STe mode enable | GST MCU GST Ctrl | Enable STe-specific features |

---

## 8. Blitter Engine (STe/MegaSTE)

### Functional Description

The blitter is a hardware **2D bit-blit engine** implemented inside the GST MCU. It accelerates block copy, fill, halftone, and pattern operations that the 68000 CPU would otherwise perform in software.

**Primary functions:**
1. **Block transfer** — Copy bitmaps from source to destination at up to 24x the speed of the 68000 (8-bit mode)
2. **16 Boolean operations** — XOR, AND, OR, NOR, etc. on source and destination words
3. **Halftoning** — Apply 16-line halftone pattern overlay
4. **Skew** — Shift source data 0-15 bits horizontally during transfer
5. **End masks** — Mask first word, middle words, and last word of each destination line independently
6. **2D address increment** — Independent X and Y increments for source and destination
7. **Bus arbitration modes** — HOG (CPU halted, blitter has full bus) or shared (64 cycles to CPU, 64 to blitter)

### Input Signals (to Blitter from CPU)

| Signal | Source | Function |
|--------|---------|------|
| D0-D15 | 68000 CPU | Blitter register writes via $FF8Axx |
| AS (active low) | 68000 CPU | Address Strobe for blitter registers |
| R/W | 68000 CPU | Read/Write for blitter registers |
| A0-A5 | 68000 CPU | Address for blitter register select ($FF8A00-$FF8A3D) |
| HOG (bit 6) | BLK Ctrl | HOG mode: CPU halted while blitter runs |
| SrcAddr (from $FF8A24) | CPU write | Source field address |
| DestAddr (from $FF8A32) | CPU write | Destination field address |
| SrcXInc (from $FF8A20) | CPU write | Source X increment per word |
| SrcYInc (from $FF8A22) | CPU write | Source Y increment per line |
| DestXInc (from $FF8A2E) | CPU write | Dest X increment per word |
| DestYInc (from $FF8A30) | CPU write | Dest Y increment per line |
| XCount (from $FF8A36) | CPU write | Words per line |
| YCount (from $FF8A38) | CPU write | Number of lines |
| OP (from $FF8A3B) | CPU write | Boolean logic operation (0-15) |
| HOP (from $FF8A3A) | CPU write | Halftone operation |
| Skew (from $FF8A3D) | CPU write | Skew amount (0-15 bits) |
| EMask1/2/3 (from $FF8A28-$2C) | CPU write | End masks for each word position |

### Output Signals (from Blitter Engine)

| Signal | Destination | Function |
|--------|-----------|------|
| SrcAddr_out | GST Blitter internal | Current source address (auto-incremented) |
| DestAddr_out | GST Blitter internal | Current dest address (auto-incremented) |
| SrcWord_out | GST Blitter internal | Source word for ALU operation |
| DestWord_out | GST Blitter internal | Dest word for ALU operation |
| AluResult | GST Blitter internal | Result of OP logic (SRC op DST) |
| MaskedResult | GST Blitter endmask | Result ANDed with end mask |
| BltWord_out | MAD bus → DRAM | Destination word written to DRAM |
| Busy | Line Num Ctrl (bit 7) | 1 = blitter running; 0 = idle |
| LineCount | Line Ctrl (bits 3-0) | Current halftone line (0-15) |
| RemainingX | XCount readback | Words remaining in current line |
| RemainingY | YCount readback | Lines remaining in transfer |
| HalftoneWord_out | ALU | Halftone pattern word for current line |
| SkewedWord_out | ALU | Source word after skew shift |

---

## 9. VME Controller (MegaSTE only)

### Functional Description

The **VME controller** is a hardware block inside the GST MCU and SH2 that provides VMEbus-compatible expansion on the MegaSTE's bottom-edge slot.

| File | Detail Level |
|------|------|
| `components/vme/01-vme-controller.md` | Full VME register map, cycle types, timing |

### Input Signals

| Signal | Source | Function |
|--------|---------|------|
| D8/D16/D32 | VME bus data lines | Data from expansion cards |
| A8/A16/A24/A32 | VME address channels | Address inputs from expansion bus |
| DS0-DS3 | VME bus | Data strobe selects for data widths |
| EOP | VME bus | End of Phase (burst transfer delimiter) |
| IACK | VME bus | I/O Acknowledge line |
| R/W | VME bus | Read/write direction |
| PAR | VME bus | 19-bit parity |
| VME IRQ1-7 | VME bus | VME interrupt request vector |

### Output Signals

| Signal | Destination | Function |
|--------|-----------|------|
| DACK1-DACK7 | VME slaves | Device acknowledge lines |
| A/D | VME bus | Address/Data phase lines |
| I/O Cherry | VME bus | I/O space select |
| MA1-MA3 | VME mode | Mode A/B select for cycle type |
| Master/Slave | VME | Master/slave mode select |
| RST (active low) | VME | Reset to VME bus |

---

## Summary — Signal Flow Across Chips

### Memory Access Flow

```
CPU wants to read:
  CPU → address A on A0-A23 + data D on D0-D15
  → Glue/GST MCU decodes address → selects target
  → If RAM region: MMU adds scroll offset → MAD bus → DRAM
  → DRAM returns data → MAD bus → MMU → D0-D15 → CPU

CPU wants to write:
  CPU → address A on A0-A23 + data D on D0-D15
  → Glue/GST MCU decodes → selects target
  → If register write (e.g., $FFC000): Shifter control
  → If MMU write: MMU scroll register
  → If DMA config: DMA registers
  → If blitter write: Blitter engine
```

### Video Display Flow

```
GST MCU GLUE block:
  Internal pixel clock counter → generates HSYNC/VSYNC/DE

GST MCU MMU block:
  MAD0-MAD11 + R/W → feeds GST Shifter (video RAM)

GST Shifter C029145:
  Reads video RAM → pixel clock → 12-bit palette → 12-bit DAC
  → R3-R0/G3-G0/B3-B0 analog → DIN13 monitor pins
  → AUDIO_L/R output from stereo DAC
```

### DMA Transfer Flow

```
CPU → configures DMA registers ($FF8A00-$FF8A0B)
WD1772 FDC → asserts FDRQ (active low)
DMA chip → sees FDRQ → asserts BR to CPU
CPU → completes cycle → asserts BGACK
DMA → takes bus → generates own R/W, address, DTACK
DRAM ↔ FDC data transfer (words via D0-D15)
FDC de-asserts FDRQ → DMA releases bus → CPU resumes
```

## References

- `components/gst/01-gst-mcu-c302183.md` — GST MCU complete register map and architecture
- `components/video/02-gst-shifter-c029145.md` — GST Shifter video modes, DAC, sync
- `components/blitter/02-blitter-gst-integration.md` — Blitter registers, operations, bus modes
- `components/vme/01-vme-controller.md` — MegaSTE VME controller details
- `components/ttl/01-ttl-support-ics.md` — TTL support ICs (ST only)
- `memory/04-dram-chip-41256-414616.md` — DRAM chip specs and timing
- `components/crystals/01-crystal-oscillators.md` — Y1/Y2/CRY1 crystal details
- `power-supply/02-ca3007h-regulator.md` — CA3007H regulator (520ST only)
- `tos/01-tos-hardware-versions.md` — TOS hardware detection by version
- [Atari ST Internals Ch. 1.2 (PDF)](https://www.atarimania.com/documents/Atari-ST-Internals.pdf)
- [VHDL ST System-on-Chip Ch. 5 (PDF)](https://devlynx.ti-fr.com/ST/dev-docs.atariforge.org/alltogether.pdf)
