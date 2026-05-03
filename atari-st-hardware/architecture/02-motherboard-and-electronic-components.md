# Motherboard and Electronic Components

> Complete board-level inventory and signal routing for Atari ST / STe / Mega STE motherboards, cross-referenced to detailed component documents.

## Board Layout Overview

### Physical Structure

All ST/STe models use a **4-layer PCB** (design: Meyer, Burkhard, revision A309493A and later). The board is approximately the size of A4 paper, with all connectors routed to the rear panel edge.

| Model | PCB Form Factor | Components |
|-------|---------|---------|
| 520ST / 520ST+ / 520STM | Full-size desktop (A4 footprint) | All standard ICs, CA3007H external PSU |
| 1040STF / 1040STFM | Full-size desktop, wider chassis for RF shield | Internally regulated PSU |
| Mega ST / Mega STe | Compact "portable" (under CRT) | Space-optimized layout |
| Mega STE | Compact, revised (IDE + VME) | GST MCU + SH2 + GST Shifter |

### Package Types by Component

| Component Type | Package | Models |
|---------------|---------|--------|
| Standard CMOS ICs | PDIP40, PDIP64, PDIP24 | All ST/STe |
| Main CPU MC68000 | PDIP64 | All ST/STe |
| Original ST Custom ASICs | PLCC68 (Glue, MMU, DMA) | ST, Mega ST |
| Original Shifter | PLCC68 or QFP44 | ST, Mega ST |
| STe I/O/MCU chips | PDIP40, PDIP24 | All STe |
| GST MCU (STe/MegaSTE) | PLCC144 SMT | STe, Mega STE |
| GST Shifter (STe) | PLCC144 SMT | STe, Mega STE |
| SH2 (MegaSTE only) | PLCC68 | Mega STE |
| DRAM chips | DIP28 (ST/STe) / PLCC32 (MegaSTE) | Varies |
| ROM chip | DIP28 (ST) / DIP28 DIP28 / TSOP (STE) | Varies |

## Component Inventory

### Core Processors (All Models)

| Part # | Package | Function | Details |
|--------|---------|---------|---------|
| MC68000P8 | PDIP64 | Main CPU, 8 MHz | Motorola 68000, 24-bit addr, 16-bit data |
| MC68000P8B | PDIP64 | Main CPU (binned) | Same as P8 but faster timing bin |
| MC68EC000 | PDIP64 | Main CPU (late production) | Embedded variant, slightly different pinout |

| Part # | Package | Function | Details |
|--------|---------|---------|---------|
| MC68901P8 | PDIP64 | MFP | Multi-Function Peripheral: 2x 8-bit I/O, 2x timers, AIC/SCI, interrupt control, watchdog |
| MC6850P8 | PDIP40 | ACIA | Asynchronous Communications Interface: MIDI In/Out, RS232 serial |
| YM2149F | PDIP40 | PSG | Yamaha Programmable Sound Generator: 3-channel tone + 1-channel noise + 3-channel PCM |
| WD1772A-10 | PDIP40 | FDC | Western Digital Floppy Disk Controller: 3.5" DD 720KB (ST) / 1.44MB (STe) |
| MC146818A | PDIP24 | RTC | Real-Time Clock: calendar, alarm, 64-byte NVRAM |
| HD6301P | PDIP40 | IKBD | Hitachi Keyboard/Mouse MCU: 48-key keyboard matrix, joystick/mouse port controller |

### Original ST Custom ASICs (Glue, MMU, DMA, Shifter)

| Marking | Package | Function | See Also |
|--------|---------|---------|----------|
| C029144-3 / C300866 / C301578 | PLCC68 | Glue logic (bus interface, refresh, decode) | `architecture/05-custom-silicon.md` |
| C028300-2 / C30114 | PLCC68 | MMU (DRAM controller, scroll engine) | `memory/02-memory-controller.md` |
| C029128-1 / C30144 | PLCC68 | DMA controller (bus arbiter) | `architecture/05-custom-silicon.md` |
| C028787-2 / C028761-1 | PLCC68 / QFP44 | Shifter/VDC (video D/A, sync, pixel clock) | `components/video/01-gtia-shifter.md` |

### STe / MegaSTE Custom ASICs

| Marking | Package | Function | See Also |
|--------|---------|---------|----------|
| C302183-002 | PLCC144 SMT | GST MCU (GLUE+MMU+DMA+Blitter integrated) | `components/gst/01-gst-mcu-c302183.md` |
| C029145 | PLCC144 SMT | GST Shifter (super hi-res, 12-bit DAC, multi-mode video) | `components/video/02-gst-shifter-c029145.md` |
| C301842 (SH2) | PLCC68 | SH2 (16 MHz clock gen + IDE controller) | Mega STE only |

### Memory Chips

#### DRAM Configuration by Model

| Model | DRAM Part | Qty | Package | Total RAM | Banks |
|-------|---------|----|--------|---------|-----|
| 520ST (512KB) | 41256 (256K x 1) | 8 | DIP28 | 512 KB (2 x 256KB) | Odd + Even |
| 1040ST (1MB) | 414616 (64K x 1) | 16 | DIP28 | 1 MB (2 x 256KB) | Odd + Even |
| Mega ST (1MB) | 414616 (64K x 1) | 16 | DIP28 | 1 MB (2 x 256KB) | Odd + Even |
| 520STE (1MB) | 414616 (64K x 1) | 32 | DIP28 | 1 MB (upgradeable to 2MB) | Odd + Even |
| 1040STE (4MB) | 414616 (64K x 1) | 128 | DIP28 | 4 MB (4 banks) | Quad-bank |
| Mega STE (2MB) | 414616 or 4164 | 16-32 | PLCC32 | 2-16 MB (SIMM) | Odd + Even |

> See `memory/04-dram-chip-41256-414616.md` for full chip specs and timing.

#### ROM Configuration by Model

| Model | ROM Part | Size | Chip Qty | TOS Version | See Also |
|-------|---------|------|----------|------------|----------|
| 520ST | 27256 (32K x 8) | 192 KB | 6 x 32KB | 1.00 | `memory/03-rom-27256-chip.md` |
| 1040ST | 27256 (32K x 8) | 192 KB | 6 x 32KB | 1.04 | |
| 520STE | 27C512 (64K x 8) | 256 KB | 2 x 128KB | 1.06 | |
| 1040STE | 27C512 | 256 KB | 2 x 128KB | 1.06 | |
| Mega STE | 27C512 (64K x 8) | 384 KB | 1 x 256KB + 1 x 128KB | 3.06 | |

> See `memory/03-rom-27256-chip.md` for ROM chip pinout, enable logic, and TOS version differences.

### Crystal Oscillators (All Models)

| Crystal | Frequency | Package | Loads | Drives | See Also |
|---------|-----------|-------|---------|-------|----------|
| Y1 | 8.000 MHz | HC-49/U | 20 pF caps | MC68000 CLK (pin 60), Glue CLK gen | `components/crystals/01-crystal-oscillators.md` |
| Y2 | 32.000 MHz | HC-49/U | 20 pF caps | Shifter pins 1-2 (32 MHz input) | |
| CRY1 | 32.768 kHz | LC-37/CT145-5 | - | MC146818A RTC pins 21/23 | |

> MegaSTE: Y1 = 16.000 MHz; Y2 = 64.000 MHz (doubled for 16 MHz CPU mode)
> All crystals use Pierce oscillator topology with inverting amplifier per crystal.
> Y1 stability: +/-100 ppm; CRY1 accuracy: +/-20 ppm.

### TTL/CMOS Support ICs (ST Baseline)

These 10 chips handle critical bus interfacing, control signal latching, address decoding, and data routing that would otherwise require dozens of discrete gates.

| IC | Part # | Package | Function on Board |
|----|--------|---------|---------|
| IC1 | 74LS245 | PDIP24 | Octal bus transceiver - D0-D15 data bus direction control |
| IC2 | 74LS174 | PDIP20 | Hex D-type flip-flop - CPU control signal latch (2MHz clock) |
| IC | 74LS05 | PDIP14 | Hex inverter (open collector) - clock buffering, sync signals |
| IC4 | 74LS04 | PDIP14 | Hex inverter - general signal inversion |
| IC5 | 74LS244 | PDIP24 | Octal buffer - address bus buffering (A0-A23) |
| IC6 | 74LS374 | PDIP20 | Octal D-type latch - address/data multiplexing |
| IC7 | 74LS138 | PDIP16 | 3-to-8 decoder - chip select generation (CS1/CS2) |
| IC8 | 74LS175 | PDIP14 | Quad D-type latch - various latching functions (DMA/control) |
| IC9 | 74HC174 | PDIP14 | Hex latch (HCMOS) - address data for Shifter control |
| IC10 | 74LS163 | PDIP16 | 4-bit binary counter - DRAM row address counter (refresh) |

> See `components/ttl/01-ttl-support-ics.md` for pin-by-pin signal mapping on each IC.
> See `components/ttl/02-ttl-circuit-diagrams.md` for bus routing diagrams showing how these ICs interconnect.

### Power Components

#### 520ST / STM (External Power Brick)

| Component | Type | Output | Powers | See Also |
|---------|------|--------|--------|---------|
| CA3007H (Regulator 1) | Dual adjustable | +8V DC | Logic rails (secondary 5V via 7805), ICs | `power-supply/02-ca3007h-regulator.md` |
| CA3007H (Regulator 2) | Dual adjustable | +22V DC | CRT video / analog circuits | |

#### Later Models (1040ST onward)

| Component | Type | Outputs | Notes |
|--------- |------|---------|------|
| Switch-PSU (internal) | SMPS + linear regs | +5V (logic), -5V, +12V, -12V | No CA3007H needed |
| 7805 regulators (board) | Linear | +5V from +8V (brick) or SMPS rail | Secondary regulation |

## Board Section Details

### CPU Section

Located near the top-left of the board (viewed from top):

| Item | Detail |
|------|--------|
| MC68000 | DIP64 package, ZIF socket on STe |
| Y1 crystal | 8.000 MHz (4-16 MHz on MegaSTE) direct to CLK pin 60 |
| Reset circuit | Driven by MC68901 timer output, Glue power-on reset, CA3007H voltage good signal |
| Clock buffer | Distributes CLK_8MHZ/CLK_4MHZ/CLK_2MHZ to MMU, DMA, Glue internal logic |
| CPU pins 22/~AEN + 23/~AS | Exposed on ZIF socket (STe) for 4MB RAM bank switching mod |

### Main Memory Section

#### Original ST Memory Configuration

```
ST 520ST (512 KB):                        ST 1040ST (1 MB):
+-----+  +-----+  +-----+  +-----+       +-----+ ... +-----+
| 4   |  | 4   |  | 41  |  | 41  |       |     |     |     |
|256K|  |256K|  |4616 |  |41  |       | 41  |     | 41  |
+--+--+  +--+--+  +--+--+  +--+--+       +--+--+   +--+--+
Odd bank (4 chips) | Even bank (4 chips) Odd/Even banks (8 chips each)
 8 total chips (512 KB)                    16 total chips (1 MB)
```

#### STe Memory Configuration

```
STe 520STE (1MB):                         1040STE (4MB):
+--+--+ ... +--+--+   32x 414616         +--+--+ ... +--+--+   128x 414616
Odd (16 chips) | Even (16 chips)         4 banks x 32 chips each   (1 MB each)
  512 KB per bank x 2 = 1 MB               512 KB per bank x 4 = 4 MB
```

#### MegaSTE Memory

SIMM slots with 414616 or 4164 chips in PLCC32 packages, 2 MB base + 2-14 MB upgradeable to 16 MB total.

### TTL Signal Routing (via Support ICs)

```
Address Bus Routing:
CPU A0-A23 -> 74LS244 (IC5) buffer -> Glue/MMU/DMA/Shifter address inputs
               -> 74LS138 (IC7) decoder for chip-select generation

Data Bus Routing:
CPU D0-D15 -> 74LS245 (IC1) transceiver -> bidirectional to all peripherals
               Direction controlled by 74LS174 (IC2) latch outputs

Control Signal Routing:
CPU R/W, AS, LDS, UDS -> 74LS374 (IC6) latch -> Glue internal decode
74LS163 (IC10) -> DRAM row address counter
74LS175 (IC8) -> DMA/control signal latching
74HC174 (IC9) -> Shifter address/data latching
74LS05 (IC3) + 74LS04 (IC4) -> clock inversion / signal conditioning
```

### Custom Silicon Section

The four original ST ASICs (or STe's two GST PLCC144 chips) form the system backbone:

| Component | ST Location | STe Location | Role |
|---------|-----------|----------|------|
| Glue (IC12) / GST MCU (IC2) | Center | Placed centrally at board center | System glue, DRAM refresh, video timing |
| MMU (IC8) / GST MCU (IC2) | Adjacent to Glue | Integrated into GST MCU | Memory page mapping, scroll |
| DMA (IC9) / GST MCU (IC2) | Adjacent to MMU | Integrated into GST MCU | Bus arbitration (FDC/HDD/Stereo) |
| Shifter (IC11) / GST Shifter (IC3) | Right side | Adjacent to GST MCU | Video gen, DAC, sync, audio mix |

> On STe: Glue+MMU+DMA+Blitter = GST MCU C302183 (IC2), Shifter = C029145 (IC3).
> On MegaSTE: GST MCU + GST Shifter + SH2 (IC4, PLCC68, for 16 MHz timing + IDE).

### Floppy Disk System

| Component | Part | Function | See |
|-- ----|------|-- ---|-- --|
| WD1772 FDC | WD1772A-10 / 1772-02 | Floppy Disk Controller: 3.5" DD/HD, MFM encoding, DMA handshake | `floppy/01-physical-format.md` |
| DMA (C029128) | C029128 / C30144 | FDRQ/FDACK floppy DMA transfer, $FF8A00-$FF8A0B registers | `floppy/01-physical-format.md` |
| DIN14 connector | 14-pin DIN | SF354/SF314 drive interface, 34-pin to drive | `pins/04-din14-floppy-port.md` |
| DRAM (41256/414616) | DIP28 / PLCC32 | 256K x 1 / 64K x 1 DRAM chips, 8/16/32/128 chips model-dependent | See `architecture/05-custom-silicon.md` |
| TTL ICs (10 chips) | 74LS245/174/05/04/244/374/138/175/HC174/163 | Bus interfacing, control latch, address decode, DRAM refresh | `components/ttl/01-ttl-support-ics.md` |
| STe GST MCU | C302183-002 | GLUE+MMU+DMA+Blitter integrated, 144-pin PLCC SMT | `components/gst/01-gst-mcu-c302183.md` |
| STe GST Shifter | C029145 | 12-bit DAC, multi-mode video, 3-bus architecture, 144-pin PLCC | `components/video/02-gst-shifter-c029145.md` |
| MegaSTE SH2 | C301842 | SH2: 16 MHz clock gen + IDE controller (only MegaSTE) | MegaSTE only |

> ST default format: 80 tracks × 9 sectors × 512 bytes × 2 sides = 720 KB
> PC-compatible format: 80 tracks × 18 sectors × 256 bytes × 2 sides = 720 KB
> HD format: 80 tracks × 18 sectors × 512 bytes × 2 sides = 1.44 MB
> See `floppy/01-physical-format.md` for PM3 encoding, sector layout, and track geometry.
> See `floppy/02-logical-format.md` for boot sector, BPB, FAT, and TOS filesystem.
> See `floppy/03-copy-protection.md` for copy protection techniques and analysis.

### Connector Section (Rear Panel Edge)

| Connector | Type | Function | See |
|-----|- --|-------|-- |- |
| DIN13 | 13-pin | Monitor RGB + sync | `pins/01-din13-monitor-port.md` |
| DIN14 | 14-pin | Floppy drive interface + floppy disk system (WD1772, DMA, physical media) | `pins/04-din14-floppy-port.md` |
| DB25 | 25-pin | Centronics parallel printer | `pins/02-db25-parallel-port.md` |
| DB25 | 25-pin | RS232 modem serial (ACIA) | `pins/03-db25-rs232-port.md` |
| DB19 | 19-pin | ACSI hard disk bus | `pins/05-acsi-interface.md` |
| DB9 | 9-pin | Mouse/joystick analog input | `pins/06-db9-mouse-joystick-port.md` |
| DIN5 | 5-pin x2 | MIDI In / MIDI Out | `pins/07-midi-port.md` |
| DIN7 | 7-pin | Power (520ST only external) | External |
| 40-pin | 40-pin | ROM cartridge slot | `pins/08-rom-cartridge-port.md` |

STe-only adds:
| DIN9 | 9-pin | STe joystick/paddle port | `ste-enhancements/04-new-io.md` |
| DB9 | 9-pin | STe pen (optical) port | `ste-enhancements/05-pen-port.md` |

## Bus Architecture (Standard ST)

```
MC68000 CPU
    |
    |---[Data Bus D0-D15]---\
    |                         |
    |---[Address Bus A0-A23]--+--- Glue (C029144) --- DRAM refresh/RAS/CAS/decode
    |                         |
    +--------------------------+--- MMU (C028300) --- DRAM banks / scroll registers
    |                         |
    +--------------------------+--- DMA (C029128) --- floppy/HDC handshake
    |                         |
    +--------------------------+--- Shifter (C028787) --- video DAC / sync
    |                         |
    +--------------------------+--- MC68901 MFP (68901) --- I/O + interrupts
    |                         |
    +--------------------------+--- YM2149 PSG (YM2149) --- sound
    |                         |
    +--------------------------+--- MC6850 ACIA (6850) --- serial/MIDI
    |                         |
    +--------------------------+--- WD1772 FDC (WD1772) --- floppy
```

### Address-Decoded Device Map (Original ST)

| Address Range | Device | Decoding Logic |
|------|--------|---------|
| $000000-$7FFFF | RAM (via MMU) | FC=3 (Data), A22=0, A23=0 |
| $80000-$FFFFFF | ROM or RAM | FC=3, A22=1 or A23=1 |
| $FF8000-$FF83FF | MFP (68901) | FC=3 + FF80xx decode |
| $FF8600-$FF86FF | FDC (WD1772) | FC=3 + FF86xx decode |
| $FF8800-$FF88FF | YM2149 PSG | FC=3 + FF88xx decode |
| $FF8A00-$FF8AFF | DMA (C029128) | FC=3 + FF8Axx decode |
| $FF8C00-$FF8CFF | ACIA (MC6850) | FC=3 + FF8Cxx decode |
| $FFC000-$FFDFDF | Shifter ($FFCxxx) | FC=3 + FFCxxx decode |
| $FFE000-$FFFFFF | MMU ($FFE0xx) | FC=3 + FFExxx decode |

### STe GST MCU Address Map

| Address Range | Device in GST MCU | Function |
|------|---------|---------|
| $FFC000-$FFC9FF | Shifter control | DAC, video mode, palette |
| $FFC100-$FCF1FF | Shifter audio | 16-channel mixer volumes |
| $FF82xx | GST Shifter reg | Video mode control, color registers |
| $FF89xx | STe sound DMA | Stereo sample playback |
| $FF8A00-$FF8A3D | DMA + Blitter | Blitter registers 0-61 |
| $FFEB00-$FFEBFF | Glue config | STe glue registers |
| $FFEC00-$FFEFFF | MMU scroll | Scroll registers, page map |
| $FFE000-$FFE016 | MMU scroll 0-7 | Each 12-bit scroll value |

> See `components/gst/01-gst-mcu-c302183.md` for full GST MCU register map.
> See `components/video/02-gst-shifter-c029145.md` for GST Shifter registers.
> See `components/blitter/02-blitter-gst-integration.md` for blitter registers.

## RAM Expansion Hacks (ST Baseline Only)

The original ST (520ST/1020ST) has **fixed** RAM with no hardware bank switching. The 4 MB bank-switching hack requires:

- **STe/MegaSTE only** (CPU pins 22/~AEN and 23/~AS exposed on ZIF socket)
- Blitter/MMU chip (AT21 or similar) monitors ~AS transitions to enable bank switching
- 1 MB switchable window within physical 0x040000-0x140000 range
- Requires 2N3904 transistor or BS170 MOSFET + jumpers to tie pin 22/23

> See `memory/05-ram-expansion-hacks.md` for detailed 4MB mod instructions and wiring diagrams.

## TOS ROM Cartridge Port (ST All Models)

The 40-pin ROM cartridge slot on the rear panel enables:

- **ROM types**: 27256 (32KB), 27512 (64KB), 27C512 (64KB CMOS), 27C010 (128KB)
- **Signal routing**: 40 pins include address lines, data lines, chip enable, output enable, write protect, and power/ground
- **ST vs STe differences**: ST limits to 64 KB; STe/Mega STE supports up to 128 KB (27C512)
- **Boot detection**: TOS monitors cartridge enable pin to determine boot source priority

> See `pins/09-rom-cartridge-variants.md` for complete 40-pin pinout and third-party cartridge list.

## VME Bus (MegaSTE Only)

The MegaSTE VME controller is integrated into the GST MCU and exposed via the VME expansion slot:

| Parameter | Value |
|-----|-----|
| Channel | A8/A16/A24/A32 |
| Data widths | D8/D16/D32 |
| Register base | I/O $FF8E00-$FF8E0F |
| Bus cycles | Burst, swap, sequential, single-word |
| Expansion cards | Kili MultiBus, pro_VME VMEST |

> See `components/vme/01-vme-controller.md` for full VME register map and timing diagrams.

## Summary: Component Counts by Architecture

### Original ST (520ST/1040ST/Mega ST)

| Category | Count | Examples |
|---------|-------|---------|
| Main CPUs | 1 | MC68000 |
| Standard ICs | 7 | MFP, ACIA, PSG, FDC, RTC, IKBD, ROM |
| DRAM chips | 8-16 | 41256 or 414616 |
| ROM chips | 6 | 27256 (520ST) / 27C512 (later) |
| Custom ASICs | 4 | Glue, MMU, DMA, Shifter (all PLCC68) |
| TTL support ICs | 10 | 74LS245/174/05/04/244/374/138/175/HC174/163 |
| Crystals | 3 | Y1, Y2, CRY1 |
| Power regulators | 1 (external) | CA3007H |

### STe / MegaSTE

| Category | Count | Examples |
|---------|-------|---------|
| Main CPUs | 1 | MC68000 |
| Standard ICs | 7 (same) | MFP, ACIA, PSG, FDC, RTC, IKBD, ROM |
| DRAM chips | 32 (STe) / 16-32 (MegaSTE) | 414616 |
| ROM chips | 2 | 27C512 |
| Custom ASICs | 2-3 | GST MCU C302183 + GST Shifter C029145 (+ SH2 on MegaSTE) |
| TTL support ICs | ~6 (reduced) | Fewer discrete TTLs (integrated into GST chips) |
| Crystals | 3 | Y1/Y2/CRY1 (freqs may change) |
| Power regulators | 0 (SMPS) | Internal switching PSU |

## References

- `fpu/01-fp-mc68881.md` -- FPU MC68881/2 optional co-processor
- `components/gst/01-gst-mcu-c302183.md` -- GST MCU C302183 comprehensive architecture
- `components/video/02-gst-shifter-c029145.md` -- GST Shifter C029145 video modes
- `components/blitter/02-blitter-gst-integration.md` -- Blitter inside GST MCU
- `components/ttl/01-ttl-support-ics.md` -- All 10 TTL ICs with pinouts
- `components/ttl/02-ttl-circuit-diagrams.md` -- Bus routing and interconnection
- `components/crystals/01-crystal-oscillators.md` -- Y1/Y2/CRY1 specifications
- `power-supply/02-ca3007h-regulator.md` -- CA3007H regulator specs
- `tos/01-tos-hardware-versions.md` -- TOS hardware detection by version
- `memory/05-ram-expansion-hacks.md` -- 4MB mod instructions
- `memory/04-dram-chip-41256-414616.md` -- DRAM chip specs and timing
- `pins/09-rom-cartridge-variants.md` -- ROM cartridge port variants
- `components/vme/01-vme-controller.md` -- VME bus on MegaSTE
- [Atari ST Internals (PDF)](https://www.atarimania.com/documents/Atari-ST-Internals.pdf)
- [Atari ST Programmer's Reference Guide (PDF)](https://info-coach.fr/atari/hardware/interfaces.php)
- [VHDL ST System-on-Chip Project](https://devlynx.ti-fr.com/ST/dev-docs.atariforge.org/alltogether.pdf)
