# STE Hardware Components

This directory contains C# emulator models for the Atari ST/STe hardware components.

## Component Overview

| Component | Part Number | Main Functions | Relations | Interface |
|-----------|-------------|-------|-------------|-----|
| **MC68000** | MC68000P8 | 16/32-bit CPU core, instruction decode, ALU, bus master | Drives bus to DRAM/ROM, communicates with all peripherals via address/data buses | 24-bit addr bus, 16-bit data bus, control signals (R/W, VPA, VPL, DS1/DS2) |
| **GST MCU** | C302183 | Glue logic, MMU page tables, DMA controller, Blitter, palette registers | Controls DRAM refresh, DMA transfers, memory protection; interfaces with Shifter for palette | Memory-mapped $FEC000-$FEFFFF, DMA bus on CPU data bus |
| **GST Shifter** | C029145 | Video signal generation, 12-bit color palette, stereo sound DAC, pixel clock | Reads frame buffer from DRAM via DMA, receives palette from GST MCU | Memory-mapped $FFC000-$FFC0FF, analog RGB output, composite/S-Video |
| **MFP** | MC68901 | 3 timers, 2 USART, 16-bit I/O ports, interrupt control | Timer A drives VBLANK sync for Shifter, USART for serial ports, I/O for cartridge slot | Memory-mapped $FF8000-$FF83FF, IRQ to CPU via interrupt acknowledge |
| **ACIA** | MC6850 | Serial UART for keyboard/MIDI communication | Receives keyboard/mouse data from IKBD via serial, connects to MIDI ports | Memory-mapped $FFFC00/$FFFC02 (KBD), $FFFC04/$FFFC06 (MIDI), 16552-compatible UART |
| **IKBD** | HD6301 | Self-contained keyboard MCU with scan logic, mouse polling | Sends serialized key/mouse events to ACIA, handles keyrepeat, ROM firmware | Serial RS-232 to ACIA, PS/2-style mouse, keyboard matrix directly connected |
| **FDC** | WD1772 | Floppy drive controller: read/write/seek/verify disk operations | Receives data via CPU bus, drives floppy drive via 26-pin FDC cable | Memory-mapped $FF8600-$FF86FF, 26-pin floppy interface, DMA channel 0 |
| **PSG** | YM2149 | 3-tone sound generation, noise generator, 8-bit ADC | Receives audio data from CPU, mixes to stereo DAC, outputs to MFP analog | Memory-mapped $FF8800-$FF88FF, analog L/R audio output, ADC input |
| **RTC** | MC146818A | Real-time clock, calendar, alarm, 128-byte NVRAM | Provides datetime to GST MCU via CPU bus, IRQ on alarm | Memory-mapped via CPU, 32.768kHz crystal, square wave output |
| **DRAM** | 414616/41256 | Main system RAM (256KB-1MB), video frame buffer | CPU reads/writes via bus, GST MCU handles refresh cycles | 16/24-bit addr, 8-bit data (x4/x6 chips), CAS/RAS control |
| **ROM** | 27C512/27256 | Boot ROM, TOS BIOS, kernel routines | CPU fetches BIOS vectors, bootstrap loader; mapped via MMU | 19/20-bit addr, 8-bit data, chip select from MMU/Glue |

## Address Map

| Range | Size | Component | Notes |
|-------|------|-----------|-------|
| $000000-$7FFFFF | 2 MB | DRAM | Main system RAM |
| $A0000-$A7FFF | 32 KB | DRAM (video) | Video frame buffer |
| $C00000-$CFFFFF | 1 MB | ROM | TOS ROM bank 0 |
| $D00000-$DFFFFF | 1 MB | ROM | TOS ROM bank 1 |
| $E00000-$EFFFFF | 1 MB | ROM | TOS ROM bank 2 |
| $F00000-$FFFFDF | 1 MB | ROM | TOS ROM bank 3 |
| $FE4000-$FE403F | 64 B | GST MCU | STe color palette registers |
| $FEC000-$FEFFFF | 32 KB | GST MCU | GST MCU control registers |
| $FF8000-$FF83FF | 1 KB | MFP | MC68901 registers |
| $FF8600-$FF86FF | 256 B | FDC | WD1772 registers |
| $FF8800-$FF88FF | 256 B | PSG | YM2149 registers |
| $FF8A00-$FF8AFF | 256 B | DMA/Blitter | DMA control, blitter registers |
| $FF8C00-$FF8CFF | 256 B | ACIA | MC6850 keyboard/MIDI registers |
| $FFC000-$FFCF00 | 3.8 KB | Shifter | GST Shifter video registers |
| $FFE000-$FFE01F | 32 B | MMU | Scroll registers (MMU block) |

## Clock Domains

| Clock | Source | Frequency | Used By |
|-------|--------|-----------|---------|
| CLK | Crystal Y1 | 7.16 MHz (NTSC) / 8 MHz (PAL) | CPU, GST MCU, MFP |
| PCLK | GST Shifter | 16 MHz | Video pixel clock |
| SCLK | External | 50/60 Hz | Stereo sound sample rate |
| RTC | Crystal | 32.768 kHz | Real-time clock |

## Motherboard Diagram

```
+------------------------------------------------------------------+
|                   ATARI ST/STe MOTHERBOARD                        |
+------------------------------------------------------------------+
|                                                                  |
|  +----------+      +------------------+      +----------------+  |
|  |  MC68000 |<---->|                |<---->|      DRAM       |  |
|  |   CPU    |      |  GST MCU       |      |  (Main RAM)    |  |
|  | (64-pin) |<>Bus>|  C302183       |      | 414616/41256   |  |
|  +----------+      | (144-pin)      |      +---------------+  |
|       |            | : GLUE        |             ^            |
|       |            | : MMU         |      Refresh|            |
|       |            | : DMA         |      signals|            |
|       |            | : Blitter     |             |            |
|       |            +----------------+                         |
|       |                    |           +----------------+     |
|       |            +-------+----+     |    ROM         |     |
|       |            |              |     | 27C512/27256 |     |
|       |            |   MMU/Glue  |<---->| TOS BIOS     |     |
|       |            |  ChipSel    +-----+ (Bank 0-3)   |     |
|       |            +------+-----+                     +-----+
|       |                   |                               |
|       |     +-------------+---------------+               |
|       |     |                           |               |
|       v     v                           v               |
|  +----------+ +----------+      +----------------+       |
|  |    MFP   | |   ACIA   |      |    GST       |        |
|  | MC68901  | |  MC6850  |      |   Shifter    |        |
|  | (64-pin) | | (40-pin) |      |  C029145     |        |
|  +----------+ +----------+      | (144-pin)    |        |
|       |          |              +-------+------+        |
|       |          |                      |               |
|       |          |              +-------+------+        |
|       |          |              |                      |        |
|       |          |              |   Video Out          |        |
|       |          |              |   RGB/DAC 12-bit     |        |
|       |          |              +---------------------+        |
|       |          |                                             |
|  +----+----+ +----+----+                                     |
|  | Timers  | | USART   |                                     |
|  | VBLANK  | | Serial  |                                     |
|  +---------+ +---------+                                     |
|       |          |                                             |
|       v          v                                             |
|  +----------+ +----------+                                    |
|  | IKBD     | | MIDI     |                                    |
|  | HD6301   | | Ports    |                                    |
|  +----------+ +----------+                                    |
|       |                                                       |
|       | Serial                                                |
|       v                                                       |
|  +----------+                                                 |
|  | Keyb/Mse |                                                 |
|  +----------+                                                 |
|                                                              |
|  +----------+    +----------+    +----------+                |
|  |   FDC    |    |   PSG    |    |   RTC    |                |
|  |  WD1772  |    |  YM2149  |    | MC146818A|                |
|  | (40-pin) |    | (40-pin) |    | (24-pin) |                |
|  +----------+    +----------+    +----------+                |
|       |               |               |                      |
|       v               v               v                      |
|  +----------+    +----------+    +----------+                |
|  | Floppy   |    | Audio L/ |    |  NVRAM   |                |
|  | Drives   |    |   R out  |    | 32.768kHz|                |
|  +----------+    +----------+    +----------+                |
|                                                              |
|  Bus Legend:                                                |
|  <>Bus  = 24-bit Addr + 16-bit Data + Control lines          |
|  |      = Data flow or control signals                        |
+------------------------------------------------------------------+
```
