# System Overview

> Complete hardware family architecture for the Atari ST / STe / Mega STE line, incorporating the full component inventory, all custom ASICs, and all model variants.

## Atari ST Family Evolution

The Atari ST (originally "Shugart Technology", referencing the 3.5" floppy format) was introduced in **June 1985** and redesigned by a team led by **Jyrri Ala-Mononen** (credited as Jyrri Alanen). The ST line pioneered a cost-effective architecture that packed desktop-class capabilities into a consumer price point through strategic use of custom silicon.

### Design Revolution: The Four-Chip Architecture

The ST's breakthrough was consolidating what would require **dozens of SSI/MSI chips** in comparable competitors into just **four custom ASICs**:

| ASIC | Atari Part # | Function | Chips Replaced |
|------|-------------|----------|---------------|
| **Glue** | C029144 / C300866 / C301578 | Bus glue logic, address decoding, DRAM refresh, clock distribution, sync generation | ~30 TTL/CMOS logic chips |
| **MMU** | C028300-2 / C30114 | DRAM controller, scroll engine, page mapping | 12 TTL chips |
| **DMA** | C029128-1 / C30144 | Floppy/HDC bus arbitration, handshake, DMA cycle generation | 8 TTL chips |
| **Shifter** | C028787-2 / C028761-1 | Video D/A, pixel clock, sync gen, audio mixer | 20 TTL/CMOS chips |

This consolidation enabled the ST to ship at roughly **$600-1000** while offering superior graphics, sound, MIDI, and built-in floppy drive versus competitors.

### The GST Consolidation (STe, 1989)

By 1989, surface-mount technology made it feasible to consolidate the ST's four ASICs into two:

| ST Architecture | STe / MegaSTE Architecture | Savings |
|----------------|---------------------------|---------|
| Glue C029144 + MMU C028300 + DMA C029128 | **GST MCU** C302183-002 (144-pin PLCC) | 3 chips -> 1 |
| Shifter C028787 | **GST Shifter** C029145 (144-pin PLCC) | 1 chip -> 1 (but 6 additional ASICs integrate here) |
| Discrete 192 KB TOS ROM (six 32 KB chips) | Single **27C512** 64 KB EPROM + TOS 2.x BIOS | Fewer sockets |

The GST MCU C302183-002 integrates GLUE, MMU, DMA, and **Blitter** into one die. The GST Shifter C029145 adds a 12-bit DAC (vs. the original 9-bit), expanded color palette, and Super Hi-Res support.

## Models and Specifications

### Original ST (1985-1987)

| Model | RAM | CPU | ROM | PSU | Release |
|-------|-----|-----|-----|-----|---------|
| 520ST | 512 KB | 68000 @ 8 MHz | 192 KB TOS 1.00 | External brick (CA3007H) | Jun 1985 |
| 1040ST | 1 MB | 68000 @ 8 MHz | 192 KB TOS 1.04 | Internal SMPS | Jun 1986 |
| Mega ST | 1 MB | 68000 @ 8 MHz | 192 KB TOS 1.04 | Internal SMPS | 1986 |
| Mega STe | 2 MB | 68000 @ 8 MHz | 256 KB TOS 1.06 | Internal SMPS | 1989 |

### Enhanced Line (1989-1991)

| Model | RAM | CPU | Color Palette | Video Modes | Special |
|-------|-----|-----|--------------|-------------|---------|
| 520STE | 1 MB (exp. to 2 MB) | 68000 @ 8 MHz | 512 (6-bit) | 9 (incl. color) | Blitter, stereo DMA, GST MCU |
| 1040STE | 4 MB (fixed) | 68000 @ 8 MHz | 512 (6-bit) | 9 (incl. color) | Blitter, stereo DMA, GST MCU |
| Mega STE | 2 MB (exp. to 16 MB) | 68000 @ 8/16 MHz | 512 (6-bit) | 23 modes incl. 640x512 SH | VME bus, IDE, GST MCU + SH2 |

### TOS Versions by Model

| TOS Version | Models | Key Hardware Support | ROM Config |
|-------------|--------|---------------------|------------|
| 1.00 | 520ST, 1040ST | ST baseline: 3 video modes, mono PSG, 3.5" FD 720KB | 192 KB (six 32KB EPROMs) |
| 1.04 | 1040ST, Mega ST | DOS-compatible floppy, autorun GEM | 192 KB |
| 1.06 | 520STE, 1040STE | Super Hi-Res, stereo DMA, blitter commands, 6-bit palette | 256 KB (two 128KB EPROMs) |
| 1.62 | STE revisions | Bug fixes for 1.06 STE | 256 KB |
| 3.06 | Mega STE | VME bus, IDE, stereo sound, 16 MHz CPU | 384 KB (27C512) |

## Board Architecture Overview

### Standard ST Architecture (1985)

The base ST motherboard uses a **4-layer PCB** organized into distinct functional zones:

```
 +----------------------------------------------------------+
 |  CPU Section (top-left)                                  |
 |  MC68000 @ 64-pin DIP  |  8 MHz crystal (Y1)  | Reset  |
 |                                                          |
 |  TTL Support (IC1-IC10)                                |
 |  74LS245 bus tx  |  74LS174 flip-flop  | ... many TTLs |
 |                                                          |
 |  Custom Silicon Zone (center)                           |
 |  [Glue] -- [MMU] -- [DMA]  --  [Shifter]              |
 |      |          |          |           |                |
 |      +----------+----------+---DRAM bank--+              |
 |                                                          |
 |  I/O Section (bottom-right)                          |
 |  [MC68901 MFP] [YM2149 PSG] [MC6850 ACIA] [WD1772FDC]|
 |  [MC146818A RTC] [HD6301 IKBD]                      |
 |                                                          |
 |  Connectors (rear panel edge)                         |
 |  DIN13 RGB | DIN14 Floppy | DB25 Par/RS232 |          |
 |  DB19 ACSI | DB9 Mouse | DIN5 MIDI | ROM 40-pin      |
 +----------------------------------------------------------+
```

### STe / MegaSTE Architecture (1989-1991)

The STe board relocates the CPU zone and replaces four ASICs with two large PLCC144 packages:

```
 +----------------------------------------------------------+
 |  CPU Section (top area)                                 |
 |  MC68000 @ 64-pin DIP  |  8/16 MHz crystal (Y1)       |
 |                                                          |
 |  GST MCU C302183 (144-pin PLCC)  --  Internal Blitter  |
 |  [GLUE blk] [MMU blk] [DMA blk] [Blitter blk]          |
 |                                                          |
 |  GST Shifter C029145 (144-pin PLCC)  -- Super Hi-Res   |
 |  [12-bit DAC] [Multi-mode VG] [Sync gen] [3-bus arch] |
 |                                                          |
 |  Standard ICs (mostly unchanged)                        |
 |  MC68901 | YM2149 | MC6850 | WD1772 | MC146818A      |
 |  HD6301 IKBD | 41256/414616 DRAM | 27C512 ROM         |
 |                                                          |
 |  MegaSTE extras:                                        |
 |  SH2 C301842 (PLCC68) -- 16 MHz clock gen + IDE ctrl  |
 |  DB9 pen port + joystick/paddle ADC ports               |
 +----------------------------------------------------------+
```

## Model Comparison Matrix

| Feature | 520ST | 1040ST | 520STE | 1040STE | Mega STE |
|---------|-------|--------|--------|---------|----------|
| **CPU** | 68000 @ 8 MHz | 68000 @ 8 MHz | 68000 @ 8 MHz | 68000 @ 8 MHz | 68000 @ 8/16 MHz |
| **Cache** | None | None | 16 KB (68000E) | 16 KB (68000E) | 16 KB (68000E) |
| **FPU option** | MC68881 (solder) | MC68881 (solder) | socketed | socketed | - |
| **Default RAM** | 512 KB | 1 MB | 1 MB | 4 MB | 2 MB |
| **Max RAM** | 1 MB | 2 MB | 2 MB | 4 MB | 16 MB |
| **DRAM chips** | 8x 41256 (256K) | 16x 414616 (64K) | 32x 414616 | 32x 414616 | 16-32x SIMM |
| **ROM** | 192 KB TOS 1.00 | 192 KB TOS 1.04 | 256 KB TOS 1.06 | 256 KB TOS 1.06 | 384 KB TOS 3.06 |
| **Custom ASICs** | 4x PLCC68 | 4x PLCC68 | 2x PLCC144 | 2x PLCC144 | 3x ASIC (MCU+SH2+Shifter) |
| **Video modes** | 3 | 3 | 9 | 9 | 23 (incl. 640x512) |
| **Palette** | 4-bit (16 colors) | 4-bit | 6-bit (512 colors) | 6-bit | 6-bit |
| **Blitter** | No | No | Yes (integrated) | Yes | Yes |
| **DMA Sound** | No (PSG only) | No (PSG only) | Yes (stereo PCM) | Yes | Yes |
| **MIDI** | In/Out | In/Out | In/Out/Thru | In/Out/Thru | In/Out/Thru |
| **Floppy** | 3.5" DD 720 KB | 3.5" DD 720 KB | 3.5" DD 1.44 MB | 3.5" DD 1.44 MB | 3.5" HD 1.44 MB |
| **Hard Drive** | DB19 ACSI | DB19 ACSI | DB19 ACSI | DB19 IDE/ACSI | DB19 IDE/ACSI |
| **VME bus** | No | No | No | No | Yes |
| **Power** | External brick | Internal SMPS | Internal SMPS | Internal SMPS | Internal SMPS |
| **PSU regulator** | CA3007H | Internal | Internal | Internal | Internal |
| **Crystal Y1** | 8.000 MHz | 8.000 MHz | 8.000 MHz | 8.000 MHz | 16.000 MHz |
| **Crystal Y2** | 32.000 MHz | 32.000 MHz | 32.000 MHz | 32.000 MHz | 64.000 MHz |
| **Crystal CRY1** | 32.768 kHz | 32.768 kHz | 32.768 kHz | 32.768 kHz | 32.768 kHz |

## TTL/CMOS Support ICs (ST Baseline)

The standard ST motherboard uses **10 TTL/CMOS support ICs** for signal routing and control logic:

| IC | Part # | Package | Function on Board |
|----|--------|---------|------------------|
| IC1 | 74LS245 | PDIP24 | Data bus transceiver (D0-D15) |
| IC2 | 74LS174 | PDIP20 | CPU control signal latch (2MHz clock) |
| IC3 | 74LS05 | PDIP14 | Hex inverter (open collector) - clock buffering |
| IC4 | 74LS04 | PDIP14 | Hex inverter - general logic inversion |
| IC5 | 74LS244 | PDIP24 | Octal buffer - address bus buffering |
| IC6 | 74LS374 | PDIP20 | Octal D-type latch - address/data multiplexing |
| IC7 | 74LS138 | PDIP16 | 3-to-8 decoder - chip select generation CS1/CS2 |
| IC8 | 74LS175 | PDIP14 | Quad D-type latch - various latching functions |
| IC9 | 74HC174 | PDIP14 | Hex latch (HCMOS) - address data for Shifter |
| IC10 | 74LS163 | PDIP16 | 4-bit binary counter - DRAM refresh counter |

> See `components/ttl/01-ttl-support-ics.md` for pin-by-pin signal mapping and `components/ttl/02-ttl-circuit-diagrams.md` for bus routing diagrams.

## Power Architecture

### Original 520ST (External Power Supply)

The 520ST uses an external power brick containing a **CA3007H dual adjustable voltage regulator** (TO-3 metal can):

| Regulator Channel | Output | Voltage | Powers |
|------------------|--------|---------|--------|
| Regulator 1 | +8V DC | regulated | Logic rails (secondary 5V regulation), ICs, GTia/Shifter analog |
| Regulator 2 | +22V DC | regulated | CRT video circuitry, analog display circuitry |

> 74LS TTL logic requires +5V regulated; the CA3007H's +8V provides headroom for secondary linear regulators (e.g., 7805) on the motherboard.

### Later Models (Internal SMPS)

All models from 1040ST onward use an internal **switched-mode power supply (SMPS)** that generates +5V and +12V rail directly, eliminating the external brick and CA3007H.

### Crystal Oscillators

Three crystals provide all system clocks:

| Crystal | Frequency | Package | Connects to | Derived Clocks |
|---------|----------|---------|-------------|---------------|
| Y1 | 8.000 MHz | HC-49/U | MC68000 CLK (pin 60) | 8 MHz CPU, 4 MHz DRAM, 2 MHz, 500 kHz (via Glue) |
| Y2 | 32.000 MHz | HC-49/U | Shifter pins 1-2 | 32 MHz pixel clock, 16 MHz shifter, HSYNC/VSYNC |
| CRY1 | 32.768 kHz | LC-37/CT145-5 | MC146818A RTC pins 21/23 | RTC seconds, minutes, hours |

> Load capacitance: 20 pF for Y1 and Y2; Pierce oscillator topology with inverting amplifier for each.
> Y1 stability: +/-100 ppm; CRY1 accuracy: +/-20 ppm.

## System Boot Sequence

```
Power-On
  |
  v
CA3007H / SMPS stabilizes +5V / +22V rails
  |
  v
MC146818A RTC starts CRY1 32.768 kHz count
  |
  v
MC68000 receives RESET pulse -> SR=$2700 (supervisor, IPL7)
  |
  v
Read initial SSP from 0x000000 (4 bytes)
Read initial PC  from 0x000004 (4 bytes)
  |
  v
Glue logic decodes $FFExxxx -> TOS ROM at $FF0000-$FFFFFF
TOS bootstrap executes at $004000 (warm boot) or $002000 (cold boot)
  |
  v
Boot:
  1. TOS reads config register at $FFFF8104 (detects PAL/NTSC, RAM size)
  2. Tests RAM: writes $55/$AA patterns, verifies
  3. Checks memvalid ($4F2), memval2 ($43A), memval3 ($51A)
  4. If all valid -> warm boot (skip RAM test, read memcntrl $424)
  5. If invalid  -> cold boot (full RAM test)
  6. DMA spin-up floppy drives
  7. Load TOS from boot sector (check disk checksum = $1234)
  8. TOS detects hardware: checks $FFFF8202 for RAM sizing
  9. Detects ROM type: 192KB (ST) vs 256KB (STE) vs 384KB (MegaSTE)
 10. XBIOS probes GST MCU (STE) or GLUE/MMU (ST) presence
 11. GEMDOS initializes, GEM desktop loads
```

## System Initialization State (After Boot)

| Component | State |
|-----------|-------|
| MC68000 SR | $2700 (supervisor, IPL7) |
| RAM | Tested, size in $4F2/$43A/$51A |
| Glue MMU | Scroll registers 0-7 at $FFE000-$FFE016 cleared |
| ROM | 192-384 KB TOS mapped at $FC0000-$FFFFFF |
| DRAM | Refreshing every 3.9 us (414616, 1024 rows) |
| Shifter | Power-on: 320x200 16-color (LORAM), palette clear |
| MC68901 MFP | All ports input, interrupts disabled |
| YM2149 PSG | All channels silent |
| MC146818 RTC | Calendar starts from default boot values |

## Block Diagram

```
                              +----------------------------+
                              |       MC68000 CPU          |
                              |     8 MHz / 16 MHz         |
                              |     D0-D15 / A0-A23       |
                              +-------------+--------------+
                                            |
                    +-----------------------+-----------------------+
                    |                       |                       |
          +---------v----------+    +-------v--------+    +---------v--------+
          |  Address Bus A0-A23|    |  Data Bus D0-D15|   |  Control Signals  |
          +---------+----------+    +-------+--------+    +---------+--------+
                    |                       |                       |
          +---------v----------+    +-------v-------v------+    +----v---------+
          | Glue (C029144)     |    | Shifter (C028787)   |    | MC68901 MFP   |
          | or GST MCU         |    | or GST Shifter      |    | IC1 (PDIP64)    |
          | [GLUE blk + MMU    |    | [DAC + Pixel Clk    |    | [Serial: IKBD,  |
          |  + DMA + Blitter]   |    |  + Sync gen + Audio] |    |  MIDI, RTS/DTR] |
          +---------+----------+    +-------+-------+------+    +---------+-----+
                    |                       |                       |
          +---------v----------+    +-------v-------+             +-----v-----+
          | DRAM Refresh/RAS   |    | 32 MHz Pixel  |             | YM2149 PSG|
          | + CAS Control      |    | Clock + HSync |             | IC4 (PDIP40)|
          | + Vsync Gen + Addr |    | + Vsync + DE  |             | [3-ch mono  |
          |   Decoding         |    | + RGB DAC     |             |  + PCM ch]  |
          +---------+----------+    +-------+-------+             +-----+-----+
                    |                       |                           |
          +---------v----------+    +-------v-------+             +-----v-----+
          | DRAM Banks         |    | Video Output   |             | Audio Out |
          | 41256 / 414616     |    | DIN13 Monitor  |             | Speakers  |
          | Odd + Even banks   |    | (RGB + Sync)   |             +-----------+
          +--------------------+    +-----------------+

          +---------+----------+
          | DMA (C029128)      |
          | [Floppy/SCSI DMA]  |
          +---------+----------+
                    |
          +---------v-------+
          | WD1772 FDC      |    +-----------+
          | or IDE (MegaSTE)|    | HD6301 IKBD |
          +---------+-------+    | IC (PDIP40) |
                    |            | [Keyb+Mouse] |
          +---------v-------+    +-------------+
          | DIN14 Floppy    |
          | DIN19 ACSI HD   |

          +-----+-----------+
          | MC6850 ACIA     |    +-------------------+
          | IC5 (PDIP40)    |    | CRY1 32.768 kHz    |
          | [ACIA: RS232,   |    | Y1 8.000 MHz       |
          |  MIDI In/Out]   |    | Y2 32.000 MHz      |
          +---------+--------+    +-------------------+
                    |
          +---------v-------+
          | DB25 RS232/Ext  |    +-------------------+
          | DIN5 MIDI In/Out|    | TTL Support ICs    |
          +-----------------+    | 74LS245/174/05/    |
                                 | 04/244/374/138/      |
                                 | 175/HC174/163        |
                                 +---------------------+
```

## References

- `memory/04-dram-chip-41256-414616.md` -- DRAM chip specifications
- `memory/05-ram-expansion-hacks.md` -- 4MB mod and bank switching
- `components/gst/01-gst-mcu-c302183.md` -- GST MCU architecture
- `components/video/02-gst-shifter-c029145.md` -- GST Shifter details
- `components/ttl/01-ttl-support-ics.md` -- TTL IC pinouts and functions
- `components/crystals/01-crystal-oscillators.md` -- All three crystals
- `power-supply/02-ca3007h-regulator.md` -- CA3007H regulator specs
- `tos/01-tos-hardware-versions.md` -- TOS hardware detection per version
- `memory/03-rom-27256-chip.md` -- ROM/EPROM chip details
