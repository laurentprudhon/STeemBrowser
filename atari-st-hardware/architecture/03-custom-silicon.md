# Custom Silicon

> Complete reference for the four original Atari ST custom ASICs and the two STe/MegaSTE GST ASICs. See the detailed component pages for register maps, pinouts, and cycle-accurate timing.

## Architecture Overview

The Atari ST's cost advantage came from consolidating what competitors implemented with 60+ individual TTL/CMOS chips into just **four custom ASICs**, later reduced to **two PLCC144 packages** in the STe.

### Original ST (1985): Four PLCC68 Chips

```
MC68000 CPU
    |
    |--- Bus Arbitration & DRAM Control ---+
    |                                       |
    |--- Address Decoder ---+               |
    |                       |               |
    +--- Glue (C029144) ---+--- MMU (C028300) --- DRAM banks (41256/414616)
    |                       |
    +--- DMA (C029128) --- WD1772 FDC / ACSI HDD / STe Stereo
    |
    +--- Shifter (C028787) --- RGB DAC / HSync / VSync / DIN13
```

### STe / MegaSTE (1989-1991): Two PLCC144 Packages + SH2

```
MC68000 CPU
    |
    +--- GST MCU C302183-002 (144-pin PLCC)
    |       [GLUE blk] [MMU blk] [DMA blk] [Blitter blk]
    |
    +--- GST Shifter C029145 (144-pin PLCC)
    |       [12-bit DAC] [Multi-mode VDC] [Sync gen] [3-bus arch]
    |
    +--- SH2 C301842 (PLCC68) [MegaSTE only: 16MHz clock + IDE]
```

| Architecture | Custom ASICs | Pins Total | TTL Replacement | Blitter | Color |
|------|-----|----|----------|------|----|
| Original ST | 4x PLCC68 | 272 | 60+ TTL chips | No | 4-bit (16) |
| STe | 2x PLCC144 | 288+ | ~30 TTL chips | Yes (integrated) | 6-bit (512) |
| MegaSTE | 3x ASIC | 432+ | ~20 TTL chips | Yes | 6-bit (512) |

## 1. Glue (C029144) / MMU (C028300) / DMA (C029128) / Shifter (C028787)

These four chips are the complete backbone of the original ST system. Their functions are now consolidated into the GST MCU C302183 and GST Shifter C029145 on STe/MegaSTE boards.

| Chip | Part # | Package | Key Functions | See |
|------|--------|---------|-----|----|
| Glue | C029144-3 / C300866 / C301578 | PLCC68 | Bus interface, address decode, DRAM refresh, clock dist, sync gen, chip select | `architecture/05-custom-silicon.md` (this file) |
| MMU | C028300-2 / C30114 | PLCC68 | DRAM controller, scroll engine, page mapping, 8x 12-bit scroll registers | `architecture/05-custom-silicon.md` (this file) |
| DMA | C029128-1 / C30144 | PLCC68 | Floppy/HDC bus arbiter, handshake controller, stereo DMA (STe) | `architecture/05-custom-silicon.md` (this file), `floppy/01-physical-format.md` |
| Shifter | C028787-2 / C028761-1 | PLCC68/QFP44 | Video D/A, 32MHz pixel clock, sync gen, audio mixer, 256 control registers | See also `components/video/01-gtia-shifter.md` |

### GST MCU C302183 (STe / MegaSTE)

The GST MCU C302183-002 integrates Glue, MMU, DMA, and Blitter into a single 144-pin PLCC:

| Internal Block | Original ST Equivalent | Function |
|---------|-----|-------|
| GLUE block (300866) | C029144 | Bus interface, address decode, refresh, sync gen, clock dist |
| MMU block (C302183 internal) | C028300 | DRAM controller, scroll engine, page mapping |
| DMA block (C302183 internal) | C029128 | Floppy/HDC bus arbitration, stereo DMA, handshake |
| Blitter block (C302183 internal) | None (new) | Block transfer engine, register control, logic ALU |

> Full architecture: `components/gst/01-gst-mcu-c302183.md`

### GST Shifter C029145 (STe)

The GST Shifter C029145 replaces the original C028787 with a 144-pin PLCC:

| Enhancement | Original Shifter | GST Shifter |
|------|------|-----|
| PAC width | 9-bit (16 colors) | 12-bit (512 colors) |
| Video modes | 3 (LORAM/MORAM/HIRE) | 9+ modes (incl. 640x512 SH) |
| Addressing | Byte (16-bit words) | Word-level (40 words/half-line) |
| DAC | 9-bit palette | 12-bit palette + 16-entry RGB |
| Bus architecture | Single bus | 3-bus (68000, RAM, sound DMA) |

Full register map and timing: `components/video/02-gst-shifter-c029145.md`

### Blitter (Integrated in GST MCU)

The blitter is part of the GST MCU C302183-002 die — it is not a separate chip:

| Feature | Original ST | STe | MegaSTE |
|------|-----|-----|----|
| Blitter presence | No | Yes (integrated) | Yes (integrated) |
| Registers | N/A | 16+ registers at $FF8A00-$FF8A3D | Same |
| Operations | N/A | Move, Solid, Halftone, SMUDGE | Same |
| Bus arbitration | N/A | 64-cycle arbitration, HOG mode | 16 MHz mode |
| Speed vs 68000 | N/A | 8 MHz: up to 24x faster (8-bit) | 16 MHz: up to 48x faster |

Full details: `components/blitter/02-blitter-gst-integration.md`

## 2. Glue Chip (C029144) — Detailed Reference

The Glue chip is the primary bus interface between the MC68000 CPU and all other system components.

### Address Map Decoded by Glue

| Address Range | Device | Decoding Logic |
|---------|-----|-------|
| $000000-$7FFFF | RAM (via MMU) | FC=3, A22=0, A23=0 |
| $80000-$FFFFFF | ROM or RAM | FC=3, A22=1 or A23=1 |
| $FF8000-$FF83FF | MFP (68901) | FC=3, FF80xx decode |
| $FF8600-$FF86FF | FDC (WD1772) | FC=3, FF86xx decode |
| $FF8800-$FF88FF | YM2149 PSG | FC=3, FF88xx decode |
| $FF8A00-$FF8AFF | DMA (C029128) | FC=3, FF8Axx decode |
| $FF8C00-$FF8CFF | ACIA (MC6850) | FC=3, FF8Cxx decode |
| $FFC000-$FFDCFF | Shifter control registers | FC=3, FFCxxx decode |
| $FFD000-$FFDFFF | GTia color generation | FC=3, FFDxxx decode |
| $FFE000-$FFE2FF | MMU mapped scroll registers | FC=3, FFExxx decode |
| $FFE800-$FFFFF9 | Glue config registers | FC=3, FFExxx decode |

### Key Signals

| Category | Signal | Description |
|------|-----|-----|
| **DRAM Refresh** | RAS0, RAS1 | Row address strobe for odd/even banks |
| | CAS0_L, CAS0_M, CAS1_L, CAS1_M | Column address strobes |
| **Clock** | CLK_8MHZ | 8 MHz to MMU, DMA, etc. |
| | CLK_4MHZ | 4 MHz for DRAM refresh timing |
| | CLK_2MHZ | 2 MHz for Shifter pixel sub-multiple |
| | CLK500KHZ | 500 kHz horizontal blank generator |
| **Chip Select** | MFPCS | MFP chip select |
| | 6850CS | ACIA chip select |
| | ROM1-6 | TOS ROM chip selects |
| | DE | Data Enable for bus cycles |
| **Interrupt** | BLANK (IPL1) | Horizontal blank |
| | HSYNC, VSYNC | Sync pulse interrupt triggers |
| | MFPINT | MFP interrupt to 68000 IPL |
| **Bus Control** | BR (Bus Request) | From DMA to CPU |
| | BG (Bus Grant) | CPU acknowledgment to DMA |
| | BGACK (Bus Grant Ack) | DMA confirms bus takeover |
| | RESET | Reset to all chips |
| | UDS/LDS | Upper/lower data strobe |
| | DTACK | Done Acknowledge |

### DRAM Refresh

Refresh occurs autonomously without CPU intervention:

1. Glue generates a RAS pulse every ~3.9 us (4 ms period for 1024 rows)
2. MMU takes over the bus for one DRAM cycle
3. Row address increments; data is read back and rewritten
4. CPU sees no interference — refresh cycles interleave with CPU cycles
5. ~830 us total refresh time per 4 ms period

### Sync Signal Generation

| Sync Type | NTSC | PAL |
|------|-----|-----|
| HSYNC | 15,734 Hz (~63.5 us) | 15,625 Hz (~64 us) |
| VSYNC | 60 Hz | 50 Hz |
| Horizontal pixels/line | 312 lines | 313 lines |
| Lines/frame | 262 | 313 |

## 3. MMU Chip (C028300) — Detailed Reference

Despite its name, the Atari MMU is a **DRAM controller and scroll engine**, not a virtual memory unit.

### Key Features

| Feature | Detail |
|------|-----|
| DRAM refresh | RAS/CAS generation for 41256/414616 chips |
| Address multiplexing | 68000 address lines → row/column split |
| Scroll engine | 8x 12-bit scroll registers (96-bit total) |
| Page mapping | Controls which physical page maps to logical address space |
| Video RAM | Maps 32 KB at $A0000-$A7FFF for display |

### Scroll Registers

| Register | I/O Address | Bits | Range |
|------|------|---- |-----|
| Scroll 0 | $FF8A00 | 12 | 0-4095 (LSB — sub-byte scroll) |
| Scroll 1 | $FF8A02 | 12 | 0-4095 |
| Scroll 2 | $FF8A04 | 12 | 0-4095 |
| Scroll 3 | $FF8A06 | 12 | 0-4095 |
| Scroll 4 | $FF8A08 | 12 | 0-4095 |
| Scroll 5 | $FF8A0A | 12 | 0-4095 |
| Scroll 6 | $FF8A0C | 12 | 0-4095 |
| Scroll 7 | $FF8A0E | 12 | 0-4095 (MSB — page-level scroll) |

**Scroll formula**: `Effective Address = Raw Address + Scroll Value`

Scroll registers divide into "digits" (12-bit each, 0-4095). Combined they form a 96-bit value used by:

| Mode | Scroll Purpose | Registers |
|---|---------|-----|
| LORAM | Horizontal scroll | Scroll 0-2 (36 bits) |
| MORAM | Horizontal scroll | Scroll 0-1 (24 bits) |
| HIRE | Vertical scroll | Scroll 0-2 (36 bits) |

For LORAM (320x200): Scroll 0 = sub-byte horizontal offset, Scroll 1 = page-level scroll.

### Odd/Even Bank Interleaving

| Bank | Data Pins | Address |
|------|-------|------|
| Odd | D1, D3, D5, D7, D9, D11, D13, D15 | Odd bytes |
| Even | D0, D2, D4, D6, D8, D10, D12, D14 | Even bytes |

Word access activates both banks simultaneously (native 68000 word cycle).

## 4. DMA Chip (C029128) — Detailed Reference

The DMA chip arbitrates between the CPU and peripherals for shared memory access.

### Bus Takeover Sequence

```
Peripheral DRQ -> DMA asserts BR to CPU -> CPU completes cycle -> BGACK
DMA asserts DE -> DMA generates own DTACK/R/W/address -> data transfer
Peripheral de-asserts DRQ -> DMA releases bus -> CPU resumes
```

### DMA Cycle Types

| Type | Description |
|------|------|
| Read cycle | Peripheral -> system RAM |
| Write cycle | System RAM -> peripheral |
| First-word cycle | Special for command block transfer (writes first word, generates DTACK without CPU) |

### Floppy DMA Handshake

| Signal | Description |
|------|------|
| FDRQ (F) | Floppy DMA Request (active low) — from WD1772 FDC when sector data needed |
| FDACK (F) | Floppy DMA Acknowledge (active low) — from DMA to FDC |
| /FDCS (F) | Floppy DMA chip select (active low) |
| R/W (F) | Floppy DMA read/write direction |
| A1 | Floppy DMA address line |
| D0-D15 | Floppy DMA data bus (bidirectional via FIFO) |

> Floppy DMA transfer: 512-byte sectors read/written word-by-word via DMA.
> FDC at $FF8600-$FF86FF, DMA registers at $FF8A00-$FF8A0B.
> See `floppy/01-physical-format.md` for sector layout and PM3 encoding details.

### Hard Disk / ACSI DMA

| Signal | Description |
|------|------|
| HDRQ (F) | Hard Disk DMA Request (active low) |
| HDAK (F) | Hard Disk DMA Acknowledge (active low) |
| /HDCS (F) | Hard disk chip select (active low) |
| R/W (F) | DMA Read/Write |
| A1 | DMA address line |

### DMA Priority

| Priority | Device |
|------|-----|
| 1 | Hard disk (highest) |
| 2 | Floppy disk |
| 3 | STe stereo sound (STe only) |

Timing: 68000 bus cycles at system clock (8 MHz or 16 MHz MegaSTE). Each DMA cycle takes one bus cycle.

## 5. Shifter Chip (C028787-2) — Detailed Reference

The Shifter is a **video display controller** and **audio mixer**. It reads the framebuffer directly from system RAM at `$A0000-$A7FFF` — no dedicated video memory.

### Pinout (40-pin QFP / 68-pin PLCC variants)

| Pin | Signal | Direction | Description |
|----|------||---|------|
| 1 | XTL0 | Input | 32 MHz external crystal |
| 2 | XTL1 | Input | 32 MHz crystal phase |
| 3-10 | D0-D7 | I/O | 8-bit data bus (control registers) |
| 11 | LOAD | Input | Register load signal |
| 12-19 | D8-D15 | I/O | Upper 8 bits of data bus |
| 20 | GND | GND | |
| 21-29 | B2-B0, G2-G0, R2-R0 | Output | RGB DAC outputs (3 bits each = 8 colors) |
| 30 | MONO | Input | Monochrome detect from Glue |
| 31-38 | A4-A0, R/W, AS | I/O | Address bus for control registers |
| 39 | CLK_16MHZ | Input | 16 MHz (half of 32 MHz) |
| 40 | CS | Input | Chip select (active low) |

### Video Modes (Original ST)

| Mode | Resolution | Pixels/byte | Memory width | Lines | Total RAM |
|------|-------|-------|------|----|------ |
| LORAM | 320x200 | 2 pixels (nibble) | 160 bytes/line | 200 | 32 KB |
| MORAM | 640x200 | 4 pixels (2 bits) | 80 bytes/line | 200 | 32 KB |
| HIRE | 640x400 | 8 pixels (1 bit) | 80 bytes/line | 400 | 32 KB |

### Pixel Clock Timing

| Parameter | NTSC | PAL |
|------|-----|-----|
| Pixel clock | 32 MHz (31.25 ns) | 32 MHz (31.25 ns) |
| Pixels/line | 312 | 313 |
| Lines/frame | 262 | 312 |
| HSYNC | 15,734 Hz | 15,625 Hz |
| VSYNC | 60 Hz | 50 Hz |

### Control Registers ($FFC000-$FFC7FF)

| Register Range | Function |
|--------|-----|
| $FFC000-$FFC07F | Palette registers (colors 0-31, 9-bit each in hires) |
| $FFC080-$FFC0FF | Reserved / audio test data |
| $FFC100-$FFC1FF | Audio mixer volume registers (16 channels, 8-bit volume) |
| $FFC200-$FFC3FF | Reserved |
| $FFC400-$FFC407 | Display mode control (LORAM/MORAM/HIRE/STe modes) |
| $FFC408-$FFC40F | R/G/B DAC configuration |
| $FFC410-$FFC41F | Monochrome mode control |
| $FFC420-$FFC47F | Sync configuration |

### Audio Mixing

The Shifter has 16 audio channels (volumes at $FFC100-$FFC1FF). The YM2149 PSG handles the 3-tone mono channels (0-2) and 3 PCM channels (voices). In ST mode, audio comes from both PSG and Shifter's 16-channel mixer.

## 6. GST Shifter (C029145) STe — Detailed Reference

The GST Shifter enhances the original with:

| Enhancement | Detail |
|------|-----|
| 12-bit DAC | 4096 RGB levels (4-4-4 R/G/B) vs. original 9-bit (8 levels each) |
| 16-color palette | 16 entry palette, each entry 12-bit (512-color total) |
| Super Hi-Res | 640x512 interlaced mode |
| 3-bus architecture | Independent 68000 bus, RAM bus, and stereo DMA bus |
| Word-level addressing | 40 words/half-line (vs. byte-level) for faster access |
| Video address counter | 24-bit read/write counter |

Full details: `components/video/02-gst-shifter-c029145.md`

## 7. VME Controller (MegaSTE)

Integrated into the GST MCU:

| Parameter | Value |
|------|-----|
| Register base | I/O $FF8E00-$FF8E0F |
| Address channels | A8, A16, A24, A32 |
| Data widths | D8, D16, D32 |
| Cycle types | Burst, Swap (block), Sequential, Single-word |
| Expansion card | Kili MultiBus, pro_VME VMEST |

Full details: `components/vme/01-vme-controller.md`

## 8. Interconnection — Original ST Block Diagram

```
MC68000 (CPU)
    |
    +--- [Data Bus D0-D15] ---> Glue (C029144) ---+
    |                                                |
    +--- [Address Bus A0-A23, FC0-2] ---> Glue ----+---> RAS/CAS to MMU
    |                                                |
    +--- [Bus Control (AS, R/W, DTACK, BR/BG)] -->+
                                                |
    +--- [Data Bus D0-D15] ---> MMU (C028300) --+---> DRAM banks (41256/414616)
    +--- [Address Bus A0-A23] ------------------|
                                                 |
    +--- [Data Bus D0-D15] ---> DMA (C029128) --+---> FDC (WD1772)
    +--- [Address Bus] -------------------------|    ACSI HDD
                                                 |    STe Stereo DAC
    +--- [Data Bus D0-D15] ---> Shifter (C028787)
    +--- [Address Bus] -------------------------|
                                                 |
    +--- [Data Bus] ---> MFP (68901)           |
    +--- [Data Bus] ---> YM2149 PSG             |
    +--- [Data Bus] ---> ACIA (MC6850)          |
    +--- [Data Bus] ---> FDC (WD1772) --|      |
                                                 |
    Glue ---RAS/CAS---> MMU ---DRAM data---> Shifter
    Glue ---clk dist---> MMU + DMA + Shifter
    Glue ---sync----> DIN13 Monitor (HSYNC/VSYNC/RGB)
    Glue ---reset---> All chips
    DMA ---FDRQ/FDACK---> WD1772 FDC
    DMA ---HDRQ/HDAK---> DB19 ACSI HDD
```

## 9. GST MCU C302183 Internal Architecture (STe)

```
+-----[GST MCU C302183-002]-----(144-pin PLCC)-----------------------------------+
|                                                                                |
|  +---[GLUE Block]---+  +---[MMU Block]---+  +---[DMA Block]---+ +--[Blitter] |
|  | Address decoder  |  | Page map reg]   |  | Bus arbiter]     |  | [Engine]   |
|  | Clock gen]       |  | DRAM refresh]   |  | FDRQ/Ack]        |  | [FIFO]     |
|  | Refresh timing   |  | Scroll engine]  |  | HDRQ/HDAK]       |  | [ALU]      |
|  | Sync gen]        |  | Address multi.] |  | Stereo DMA]      |  | [Control]  |
|  | DE/DTACK/R/W]    |  | MAD bus]        |  | First-word cyc]  |  | [Op code]  |
|  | Chip select gen] |  | Page map reg]   |  | Bus arbiter]     |  | [Logic]    |
|  +------------------+  +-----------------+  +------------------+  +----------+ |
|                                                                                |
|  Internal bus: MAD bus connects all blocks internally (no external traces)     |
|  Shared 68000 interface: D0-D15, A0-A23 all go to/come from GST MCU          |
+--------------------------------------------------------------------------------+
```

| Internal Block | ST Equivalent | Pins Saved |
|------|------|------|
| Glue | C029144 (PLCC68, 68 pins) | 68 pins eliminated |
| MMU | C028300 (PLCC68, 68 pins) | 68 pins eliminated |
| DMA | C029128 (PLCC68, 68 pins) | 68 pins eliminated |
| Blitter | New integration | 144-pin PLCC only |

Net: 3 external PLCC68 chips -> 1 PLCC144 package (and GST Shifter C029145 replaces C028787).

## 10. TTL Support ICs (ST Baseline Only)

The standard ST motherboard uses 10 TTL/CMOS support ICs for signal routing:

| IC | Part # | Function |
|----|------|------|
| IC1 | 74LS245 | Octal bus transceiver (D0-D15) |
| IC2 | 74LS174 | Hex D-type flip-flop (CPU control latch) |
| IC3 | 74LS05 | Hex inverter (open collector) |
| IC4 | 74LS04 | Hex inverter |
| IC5 | 74LS244 | Octal buffer (address bus) |
| IC6 | 74LS374 | Octal D-type latch |
| IC7 | 74LS138 | 3-to-8 decoder (chip select) |
| IC8 | 74LS175 | Quad D-type latch |
| IC9 | 74HC174 | Hex latch (HCMOS) |
| IC10 | 74LS163 | 4-bit counter (DRAM refresh) |

> STe reduces this to ~6 TTLs (GLUE+MMU+DMA integrated into GST MCU).
> Full pinout and circuit diagrams: `components/ttl/01-ttl-support-ics.md` and `components/ttl/02-ttl-circuit-diagrams.md`

## 11. References

- `components/gst/01-gst-mcu-c302183.md` -- GST MCU architecture, register map, emulation
- `components/video/02-gst-shifter-c029145.md` -- GST Shifter video modes, timing
- `components/blitter/02-blitter-gst-integration.md` -- Blitter operations, registers
- `components/vme/01-vme-controller.md` -- VME controller on MegaSTE
- `components/ttl/01-ttl-support-ics.md` -- TTL IC pinouts and functions
- `memory/04-dram-chip-41256-414616.md` -- DRAM chips and bank config
- `memory/02-memory-controller.md` -- DRAM refresh and page mapping
- `memory/01-memory-map.md` -- Full 16 MB address space
- [Atari ST Internals, Ch. 1.2 - Custom Semiconductor (PDF)](https://www.atarimania.com/documents/Atari-ST-Internals.pdf)
- [VHDL ST System-on-Chip Project - Ch. 5 (PDF)](https://devlynx.ti-fr.com/ST/dev-docs.atariforge.org/alltogether.pdf)
