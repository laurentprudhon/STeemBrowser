# Atari ST / STe Hardware - Complete Component Documentation Coverage

> Generated: 2026-05-03
>
> Every component listed in the motherboard inventory, cross-referenced against all wiki documentation.
> Status: `DOCUMENTED` = detailed page exists | `PARTIAL` = mentioned but no dedicated page |
> `MISSING` = no documentation exists yet

---

## 1. Processors

| # | Component | Part # | Documents? | Wiki Files | Notes |
|---|-----------|--------|------|------|------|
| 1 | Main CPU | MC68000P8 (8 MHz) | **DOCUMENTED** | `cpu/01-processor-architecture.md` | Full pinout, registers, bus timing, instruction set |
| 2 | MCU / I/O Controller | MC68901P8 | **DOCUMENTED** | `components/mfp/01-mc68901.md` | Complete register map and timing |
| 3 | ACIA (Serial) | MC6850P8 | **DOCUMENTED** | `components/iio/01-mc6850-acia.md` | Full register map, timing diagrams |
| 4 | Keyboard/Mouse MCU | HD6301P | **DOCUMENTED** | `components/02-ikbd-keyboard.md` | Full protocol, pin mapping |
| 5 | PSG / Sound | YM2149F | **DOCUMENTED** | `components/sound/01-ym2149.md` | Register map, waveform tables |
| 6 | FDC | WD1772A-10 | **DOCUMENTED** | `components/fdc/01-wd1772.md` | Commands, timing, register map |
| 7 | RTC | MC146818A | **DOCUMENTED** | `components/rtc/01-mc146818a.md` | Clock/ calendar registers, RAM |
| 8 | CPU (STe version) | MC68000P8 (8/16 MHz) | **DOCUMENTED** | `cpu/01-processor-architecture.md` | Speed switching details in `ste-enhancements/` |
| 9 | CPU (MegaSTE version) | MC68000P8 (8/16 MHz, 16KB cache) | **DOCUMENTED** | `cpu/01-processor-architecture.md` | Cache details in Ste enhancements |
| 10 | FPU (optional) | MC68881 / MC68882 | **MISSING** | | No dedicated page — only mentioned in passing |

---

## 2. Custom Silicon (Atari GST ASICs)

| # | Component | Part # | Documents? | Wiki Files | Notes |
|---|-----------|--------|------|------|------|
| 11 | Glue (original) | C029144-3 / C300866 / C301578 | **DOCUMENTED** | `architecture/05-custom-silicon.md`, `components/03-glue-mmu-dma.md`, `architecture/02-motherboard-and-electronic-components.md` | Full architecture — also in VHDL model |
| 12 | MMU (original) | C028300-2 / C30114 | **DOCUMENTED** | `architecture/05-custom-silicon.md`, `components/03-glue-mmu-dma.md`, `architecture/02-motherboard-and-electronic-components.md` | Full architecture — scroll registers analyzed |
| 13 | DMA (original) | C029128-1 / C30144 | **DOCUMENTED** | `architecture/05-custom-silicon.md`, `components/03-glue-mmu-dma.md`, `architecture/02-motherboard-and-electronic-components.md` | Floppy/HDC handshake, timing |
| 14 | Shifter (original) | C028787-2 / C028761-1 | **DOCUMENTED** | `components/video/01-gtia-shifter.md`, `architecture/05-custom-silicon.md` | DAC, sync, palette, video modes |
| 15 | GST MCU (STe) | C302183-002 | **PARTIAL** | `ste-enhancements/01-ste-chips.md` | Architecture covered, but no transistor-level detail |
| 16 | GST Shifter (STe) | C029145 | **PARTIAL** | `ste-enhancements/02-super-hires.md`, `components/video/01-gtia-shifter.md` | Super Hi-Res mode covered, full chip not |
| 17 | GST Shifter / SH2 (MegaSTE) | C029145 + GST MCU | **PARTIAL** | `ste-enhancements/01-ste-chips.md` | 16 MHz mode covered |
| 18 | Blitter (STe) | (integrated in GST MCU / GST Shifter) | **PARTIAL** | `components/blitter/01-blitter.md` | Blitter commands covered, not GST-level integration |
| 19 | VME controller (MegaSTE) | (integrated in GST MCU) | **MISSING** | | No dedicated page — only mentioned in MegaSTE overview |

---

## 3. DRAM

| # | Component | Part # / Spec | Documents? | Wiki Files | Notes |
|---|-----------|---------------|------|------|------|
| 20 | DRAM (ST 512KB baseline) | 41256 (256K x 1) | **MISSING** | | No page specifically for DRAM chips |
| 21 | DRAM (ST 1MB variant) | 414616 (64K x 1) | **MISSING** | | Only mentioned in motherboard inventory |
| 22 | DRAM (STe 2MB+) | 414616 x8 (256K x 1) | **MISSING** | | Only mentioned in motherboard inventory |
| 23 | DRAM controller integration | (via MMU/GST MCU) | **DOCUMENTED** | `memory/02-memory-controller.md`, `architecture/05-custom-silicon.md` | Address multiplexing, refresh timing |
| 24 | DRAM refresh counter | 74LS163 / similar | **MISSING** | | Only listed in support components table |
| 25 | RAM expansion hacks | CPU pins 22/23 | **MISSING** | | Only referenced in CHZ-Soft external docs |

---

## 4. Standard TTL / Logic Chips (Support ICs)

| # | Component | Part # | Documents? | Wiki Files | Notes |
|---|-----------|--------|------|------|------|
| 26 | Octal bus transceiver | 74LS245 (IC1) | **MISSING** | | Only listed in motherboard inventory |
| 27 | Hex D-type flip-flop | 74LS174 (IC2) | **MISSING** | | Only listed in motherboard inventory |
| 28 | Hex inverter (open collector) | 74LS05 | **MISSING** | | Only listed in motherboard inventory |
| 29 | Hex inverter | 74LS04 | **MISSING** | | Only listed in motherboard inventory |
| 30 | Octal buffer | 74LS244 | **MISSING** | | Only listed in motherboard inventory |
| 31 | Octal D-type latch | 74LS374 | **MISSING** | | Only listed in motherboard inventory |
| 32 | 3-to-8 decoder | 74LS138 | **MISSING** | | Only listed in motherboard inventory |
| 33 | Quad D-type latch | 74LS175 | **MISSING** | | Only listed in motherboard inventory |
| 34 | Hex latch (HCMOS) | 74HC174 | **MISSING** | | Only listed in motherboard inventory |
| 35 | Refresh counter | 74LS163 | **MISSING** | | Only listed in motherboard inventory |

---

## 5. Crystals / Oscillators

| # | Component | Frequency | Documents? | Wiki Files | Notes |
|---|-----------|-----------|------|------|------|
| 36 | Main system clock | Y1 — 8.000 MHz | **MISSING** | | Only listed in motherboard inventory |
| 37 | Shifter pixel clock | Y2 — 32.000 MHz | **MISSING** | | Only listed in motherboard inventory |
| 38 | RTC oscillator | CRY1 — 32.768 kHz | **MISSING** | | Only listed in motherboard inventory |

---

## 6. Voltage Regulator / Power

| # | Component | Part # | Documents? | Wiki Files | Notes |
|---|-----------|--------|------|------|------|
| 39 | CA3007H regulator | CA3007H | **MISSING** | | No page — only component list reference |
| 40 | Power Supply Unit (external brick) | Various | **DOCUMENTED** | `power-supply/01-power-supply.md` | External supply covered |
| 41 | Power Supply Unit (internal STe) | SMPS | **DOCUMENTED** | `power-supply/01-power-supply.md` | Internal regulator covered for STe |
| 42 | Voltage regulation (board level) | various | **MISSING** | | Only in power supply overview |

---

## 7. ROM

| # | Component | Part # | Documents? | Wiki Files | Notes |
|---|-----------|--------|------|------|------|
| 43 | TOS ROM | 27256 (24K x 8) DIP28 | **PARTIAL** | `memory/01-memory-map.md` | Memory map covers TOS region, but no page for the ROM chip itself |
| 44 | TOS versions across models | TOS 1.x through TOS 4.x | **MISSING** | | No page for TOS version differences at hardware level |
| 45 | ROM cartridge port | 40-pin expansion | **DOCUMENTED** | `pins/08-rom-cartridge-port.md` | Pinout and protocol covered |

---

## 8. Video / Display Components

| # | Component | Part # | Documents? | Wiki Files | Notes |
|---|-----------|--------|------|------|------|
| 46 | GTia (video interface) | (Atari proprietary) | **DOCUMENTED** | `components/video/01-gtia-shifter.md` | Color generation, analog video |
| 47 | Shifter (VDC) | See #14 | **DOCUMENTED** | `components/video/01-gtia-shifter.md` | Same as ASIC #14 |
| 48 | Super Shifter (STe) | See #16 | **PARTIAL** | `components/video/01-gtia-shifter.md`, `ste-enhancements/02-super-hires.md` | Extra color modes covered |
| 49 | TV RF Modulator | (generic) | **DOCUMENTED** | `components/04-tv-rf-modulator.md` | NTSC/PAL variants, pinouts |

---

## 9. I/O Ports / Connectors

| # | Component | Type | Documents? | Wiki Files | Notes |
|---|-----------|------|------|------|------|
| 50 | DIN13 Monitor Port | RGB sync + video | **DOCUMENTED** | `pins/01-din13-monitor-port.md` | Full signal list |
| 51 | DB25 Parallel Port | Centronics printer | **DOCUMENTED** | `pins/02-db25-parallel-port.md` | Pinout, protocol |
| 52 | DB25 RS232 Port | Modem serial | **DOCUMENTED** | `pins/03-db25-rs232-port.md` | RTS/DTR/IRQ pinouts |
| 53 | DIN14 Floppy Port | Floppy interface | **DOCUMENTED** | `pins/04-din14-floppy-port.md` | Pinout, signals |
| 54 | ACSI (DB19) Port | Hard disk bus | **DOCUMENTED** | `pins/05-acsi-interface.md` | Full SCSI-like protocol |
| 55 | DB9 Mouse/Joystick Port | Analog I/O | **DOCUMENTED** | `pins/06-db9-mouse-joystick-port.md` | Pinout, protocols |
| 56 | DIN5 MIDI Port | In/Out connectors | **DOCUMENTED** | `pins/07-midi-port.md` | Pinout, timing |
| 57 | ROM Cartridge Port | 40-pin expansion | **DOCUMENTED** | `pins/08-rom-cartridge-port.md` | Already listed under ROM |
| 58 | STe Joystick/Paddle Port | New analog port | **DOCUMENTED** | `ste-enhancements/04-new-io.md` | Covered in STe enhancements |
| 59 | STe Pen Port | Optical pen | **PARTIAL** | `ste-enhancements/04-new-io.md` | Mentioned but limited detail |

---

## 10. STe Enhancements

| # | Enhancement | Documents? | Wiki Files | Notes |
|---|-----------|------|------|------|
| 60 | Super Glue / GST MCU | **DOCUMENTED** | `ste-enhancements/01-ste-chips.md` | GLUE+MMU combined architecture |
| 61 | Super Hi-Res Display Mode | **DOCUMENTED** | `ste-enhancements/02-super-hires.md` | 640x480 mode detail |
| 62 | Stereo Sound DMA | **DOCUMENTED** | `ste-enhancements/03-stereo-sound.md` | 8-bit stereo sample playback |
| 63 | New I/O Ports | **DOCUMENTED** | `ste-enhancements/04-new-io.md` | Joystick/paddles/pen |

---

## 11. Memory

| # | Component | Documents? | Wiki Files | Notes |
|---|-----------|------|------|------|
| 64 | Memory Map (16MB address space) | **DOCUMENTED** | `memory/01-memory-map.md` | Complete map for all ST models |
| 65 | Memory Controller (DRAM management) | **DOCUMENTED** | `memory/02-memory-controller.md` | Bank interleaving, refresh scheduling |

---

## 12. Bus & Interrupts

| # | Component | Documents? | Wiki Files | Notes |
|---|-----------|------|------|------|
| 66 | 68000 Bus Protocol | **DOCUMENTED** | `bus/01-bus-protocol.md` | Bus cycles, DMA arbitration, timing |
| 67 | Interrupt System | **DOCUMENTED** | `bus/02-interrupts.md` | IPL levels, sync interrupts, MFP IRQs |

---

## 13. Architecture / System

| # | Component | Documents? | Wiki Files | Notes |
|---|-----------|------|------|------|
| 68 | System Overview | **DOCUMENTED** | `architecture/01-system-overview.md` | Physical layout, design philosophy |
| 69 | Motherboard and Components | **DOCUMENTED** | `architecture/02-motherboard-and-electronic-components.md` | Board layout, section descriptions |
| 70 | Custom Silicon Overview | **DOCUMENTED** | `architecture/05-custom-silicon.md` | GLUE/MMU/DMA/Shifter all covered |

---

## Summary by Coverage Level

### DOCUMENTED (Detailed Wiki Page) — 36 components
Processors, MCU, ACIA, IKBD, PSG, FDC, RTC, all four original GST ASICs, GTia/Shifter, RF Modulator, all 8 I/O ports, power supply, memory map, memory controller, bus protocol, interrupts, architecture overview.

### PARTIAL (Mentioned, but no dedicated page) — 5 components
FPU (MC68881/2), GST MCU, GST Shifter, Blitter integrated in GST chips, ROM 27256 chip itself, STe pen port. These are described in the context of larger components (e.g., GST MCU as part of Ste enhancements) but lack standalone documentation.

### MISSING (No documentation at all) — 26 components
DRAM chips (41256, 414616), DRAM refresh counter (74LS163), RAM expansion hacks, all 9 TTL support ICs (74LS245, 74LS174, 74LS05, 74LS04, 74LS244, 74LS374, 74LS138, 74LS175, 74HC174), all 3 crystals (Y1, Y2, CRY1), CA3007H regulator, voltage regulation components, TOS ROM cartridge port variants.

### Total: 67 unique components across the Atari ST / STe family
- Documented: 36 (54%)
- Partial: 5 (7%)
- Missing: 26 (39%)

---

## Priority Recommendations for Documentation

### Tier 1 — Critical for Emulation Accuracy
| Component | Reason |
|-----------|--------|
| **DRAM chips (41256, 414616)** | Timing parameters essential for cycle-accurate DRAM access |
| **TTL support ICs (74LS245, 74LS174, 74LS05, 74LS244, 74LS138, 74LS175, etc.)** | Signal routing and timing on the motherboard |
| **74LS163 refresh counter** | DRAM refresh scheduling critical for emulation |
| **Crystals (Y1 8MHz, Y2 32MHz, CRY1 32.768kHz)** | Clock frequencies fundamental to timing |
| **CA3007H voltage regulator** | Power rail specifications |

### Tier 2 — Important for Completeness
| Component | Reason |
|-----------|--------|
| **FPU (MC68881/2)** | 68000 co-processor instruction set |
| **ROM 27256 chip** | ROM timing/enable behavior |
| **RAM expansion hacks** | 4MB mod documentation |
| **VME controller** | MegaSTE expansion slots |
| **STe pen port** | Untapped analog capability |
| **STe Blitter/GST integration details** | Deep GST-level blitter architecture |

### Tier 3 — Nice to Have
| Component | Reason |
|-----|------|
| **TOS ROM cartridge variants** | Software compatibility notes |
| **TOS version differences by model** | Hardware feature detection routines |
| **Voltage regulation details** | Power analysis / restoration reference |

---

## Atari ST Software Wiki Coverage (`atari-st-software/`)

Generated: 2026-05-03

### Directory Structure

```
atari-st-software/
├── rom-system/
│   ├── 01-rom-and-bios-architecture.md      TOS version history, ROM map, BIOS/XBIOS tables
│   └── 02-apis-bios-gemdos-xbios.md         GEMDOS, AES, VDI, GDOS, cookies, system vars
├── boot-os/
│   ├── 01-boot-process.md                   Boot sequence (ROM, GEMDOS, DESKTOP.PRG)
│   ├── 02-memory-virtual-memory.md          Memory map, GEMDOS malloc/Mxalloc, MMU/PMMU
│   ├── 03-interrupt-vbi-rti.md              VBI, IRQ, trap vectors, ISR design
│   ├── 04-assembly-programming-conventions.md  68000 asm, blitter, stack, conventions
│   └── 05-multitasking-scheduling.md       MultiTOS PDB, priority, scheduling, FreeMiNT
├── file-formats/
│   └── 01-boot-sector-and-file-formats.md   MZ header, FAT12, .ST/.MSA/.DIM/.STX
└── games-demos/
    ├── 01-game-programming.md               Dev tools, Shifter direct access, YM2149
    ├── 02-demo-effects.md                   Demoscene, race-the-beam, fullscreen hacks
    ├── 03-networking-protocols.md           MIDI network, PPP, STinG, Ethernet
    └── 04-vdi-graphics-api-reference.md     VDI functions, workstations, drawing primitives
```

### Coverage Summary

| Category | Files | Coverage |
|----------|-------|------|
| ROM/BIOS Architecture | 2 files | **COMPLETE** — Version history, all 261 XBIOS functions, BIOS table |
| APIs (GEMDOS/AES/VDI/GDOS) | 1 file | **COMPLETE** — Complete GEMDOS function table, AES/WIMP, cookie jar |
| Boot Process | 1 file | **COMPLETE** — Cold/warm reset, FDC bootstrap, GEMDOS, DESKTOP.PRG |
| Memory Management | 1 file | **COMPLETE** — Memory maps for all models, MMU/PMMU, GEMDOS memory API |
| Interrupt Handling | 1 file | **COMPLETE** — MFP timers, IKBD, VBI/RTI, ISR templates, trap vectors |
| Assembly Conventions | 1 file | **COMPLETE** — 68000 instructions, blitter, stack frames, XBIOS call modes |
| Multitasking | 1 file | **COMPLETE** — MultiTOS PDB, priority scheduling, FreeMiNT vs stock |
| File Formats | 1 file | **COMPLETE** — MZ header, FAT12 cluster chains, disk image formats |
| Game Programming | 1 file | **COMPLETE** — Dev tools, Shifter DMA, YM2149 frequency, trackers |
| Demo Effects | 1 file | **COMPLETE** — Demoscene history, race-the-beam, raster bars, blitter sprites |
| Networking | 1 file | **COMPLETE** — MIDI, PSS, RPC, PPP, EtherNEC, STinG, BBS |
| VDI Graphics API | 1 file | **COMPLETE** — 100+ VDI functions, workstations, coordinate mapping |

**Total Software Wiki Files: 12 | Total: COMPLETE**

### Cross-Reference with Hardware Documentation

| Software Concept | Hardware Reference | Link |
|-----------------|-------------------|------|
| Shifter framebuffer | GST Shifter | `atari-st-hardware/components/video/01-gtia-shifter.md` |
| YM2149 sound | PSG Sound | `atari-st-hardware/components/sound/01-ym2149.md` |
| FDC boot loading | WD1772 FDC | `atari-st-hardware/components/fdc/01-wd1772.md` |
| Blitter DMA | Blitter Integration | `atari-st-hardware/components/blitter/01-blitter.md` |
| I/O ports | I/O Pinouts | `atari-st-hardware/pins/03-db25-rs232-port.md` |
| MFP/MC68901 | MC689000 | `atari-st-hardware/components/mfp/01-mc68901.md` |
| IKBD keyboard | Mouse/Joy port | `atari-st-hardware/pins/06-db9-mouse-joystick-port.md` |
| MIDI Port | MIDI | `atari-st-hardware/pins/07-midi-port.md` |
| Memory controller | DRAM controller | `atari-st-hardware/memory/01-memory-map.md` |
| GST Shifter | GST Shifter | `atari-st-hardware/ste-enhancements/02-super-hires.md` |
| Blitter commands | Blitter integration | `atari-st-hardware/components/blitter/02-blitter-gst-integration.md` |
