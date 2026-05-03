# GST Shifter C029145

## Overview

The **GST Shifter (C029145)** is Atari Corporation's second-generation video controller ASIC, introduced with the **Atari STe** (520STE / 1040STE) in 1988. It succeeded the original **Shifter (C028787)** found in the ATST / Mega ST line, adding a significant list of new capabilities while maintaining backward compatibility.

The GST Shifter is a custom CMOS ASIC packaged in a quad-flat package (QFP) with 64 pins. It is one of two GST ("Group Support Technology") ASICs on the STe motherboard; the other being the **GST MCU** (Memory Controller Unit / MMU + Glue). Together, these two chips replace the four separate TTL and ASIC chips (GLUE, MMU, DMA controller, and Shifter) used in the original ST.

### What GST Shifter Adds Over Original Shifter (C028787)

| Feature | Original Shifter (C028787) | GST Shifter (C029145) |
|---|---|---|
| Palette size | 512 colors (9-bit: 3-3-3 R/G/B) | 4,096 colors (12-bit: 4-4-4 R/G/B) |
| Video modes | Low (320x200 16-col), Medium (640x200 4-col), High (640x400 mono) | All original modes + MCM 16-color + super hi-res 640x512 + double-density modes |
| Fine scrolling | Vertical only (via VBASE registers) | Horizontal + Vertical fine scrolling |
| Sound | None | 8-bit stereo DMA sound with 4 sample rates |
| Color palette | 16 palette registers (stored in RAM, 5 address lines) | 16 palette registers + 4-bit color depth per component |
| Sync generation | Via external GLUE | Integrated in GST MCU, fed back to Shifter |
| DAC | 9-bit DAC (3-bit R, 3-bit G, 3-bit B) | 12-bit DAC (4-bit R, 4-bit G, 4-bit B) |
| External clock | 32 MHz internal oscillator | External SCLK input (8 MHz) for sound DMA timing |
| Data bus latch | 4 external TTL chips (244/373) | Integrated internally |

The GST Shifter retains the same fundamental concept: it acts as a video DMA engine that continuously reads pixel data directly from system RAM and shifts it out as RGB video at the pixel clock rate, while the 68000 CPU retains access to the data bus whenever the Shifter is not accessing RAM. The key architectural difference is that the GST Shifter adds **12-bit color**, **horizontal fine-scrolling**, and **8-bit stereo DMA audio**.

---

## Video Modes Supported

### Mode 0: Low Resolution (320 x 200, 16 colors)

- The standard ST video mode
- 16 pixels per 16-bit word (1 bit per pixel)
- 16 color palette registers define on-screen colors
- 320 / 16 = 20 words per scanline
- 200 scanlines per frame
- Color clock: 16 MHz (one pixel every 62.5 ns)
- Horizontal: 320 active pixels + border / blanking
- Vertical: 50 Hz (PAL) / 60 Hz (NTSC) refresh

### Mode 1: High Resolution (640 x 400, monochrome)

- 1 bit per pixel, 640 pixels per scanline
- 640 / 16 = 40 words per scanline
- 400 scanlines per frame
- Requires monochrome monitor (no DAC used)
- Vertical refresh: ~71-72 Hz
- Color clock: 16 MHz

### Mode 2: Medium Resolution (640 x 200, 4 colors)

- 2 bits per pixel, 640 pixels per scanline
- 640 / 4 = 160 pixels per nibble; 4 pixels per 16-bit word
- 2 bits x 32 = 64 values per 32 consecutive words define palette selections
- 4 color palette registers
- 200 scanlines per frame
- 50 Hz (PAL) / 60 Hz (NTSC)

### Mode 3: MCM 16-Color / Super Hi-Resolution (640 x 512 interlaced, 16 colors)

The **MCM (Multi-Color Mode)** and **Super Hi-Resolution** modes are both enabled via the GST Shifter's register at `$FF8240`.

#### MCM 16-Color Mode (640 x 200 interlaced, 16 colors)

- Same word/pixel layout as high resolution (1 bit/plane x 2 bitplanes x 8 = 16 colors per pixel group)
- 640 pixels / 16 bits = 40 words per half-line
- Uses a 16-pixel block color encoding (similar to the mode 0 palette system but with 640-pixel lines)
- Interlaced: draws 200 odd fields + 200 even fields = 400 total lines per double-frame
- Refresh: ~71 Hz

#### Super Hi-Resolution Mode (640 x 512 interlaced, 16 colors)

- Resolution: 640 pixels wide, 512 total scanlines (256 odd + 256 even, interlaced)
- Uses the same 1-bit-per-pixel encoding as modes 1 and 3
- 40 words per half-line (same as hi-res)
- Interlaced 256+256 scanlines
- ~50 Hz frame rate (100 Hz field rate)
- Can display 16 colors simultaneously from the 512-entry palette (in STe palette mode: 4,096 colors)
- The extra 112 lines (512 - 400) provide a larger virtual screen area

#### Double Density 640 x 256 Mode

- Also known as "high density" in STe
- 640 pixels x 256 scanlines
- 1-bit pixels with pixel-skip horizontal scrolling
- Refresh: 50 Hz (PAL) / 60 Hz (NTSC)
- Uses all 16 palette entries

### Mode Timing Summary

| Mode | Resolution | Colors | Scanlines | Refresh | Interlaced |
|---|---|---|---|---|---|
| Low Res | 320 x 200 | 16 (from 512) | 200 | 50/60 Hz | No |
| High Res | 640 x 400 | 1 (mono) | 400 | 71-72 Hz | No |
| Medium Res | 640 x 200 | 4 (from 512) | 200 | 50/60 Hz | No |
| MCM 16 | 640 x 200 | 16 (from 4096) | 200 | 71 Hz | Yes |
| Super Hi-Res | 640 x 512 | 16 (from 4096) | 512 | 50 Hz (100 Hz field) | Yes |
| Double Density | 640 x 256 | 16 (from 4096) | 256 | 50/60 Hz | No |

---

## DAC (Digital-to-Analog Converter)

### Original Shifter DAC (C028787)

The original Shifter uses a **9-bit DAC** with a 16-entry palette:

```
Palette RAM (in Shifter):     16 entries x 9 bits = 144 bits
                            Address lines: A0-A4 (5 bits) = 32 entries (16 used)
Each entry:  3 bits Red + 3 bits Green + 3 bits Blue

DAC resolution:  2^3 = 8 intensity levels per channel
Total colors:    8 x 8 x 8 = 512 possible
On-screen:       16 simultaneous colors from palette
```

The 16 palette entries are stored in main system RAM at the start of each scanline word (the 16 nibbles of the first 32-bit word in each line define which of the 16 palette colors each pixel index maps to).

### GST Shifter DAC (C029145)

The GST Shifter implements a **12-bit DAC** with a 16-entry palette, supporting 4,096 colors:

```
Palette RAM (in Shifter):     16 entries x 12 bits = 192 bits
                            Address lines: A0-A4 (5 bits) + new A5 (via HSCROLL register)
Each entry:  4 bits Red + 4 bits Green + 4 bits Blue

DAC resolution:  2^4 = 16 intensity levels per channel
Total colors:    16 x 16 x 16 = 4,096 possible
On-screen:       16 simultaneous colors from palette
```

The color depth increase from 3-bit to 4-bit per channel provides double the intensity steps per color, enabling the expanded 4,096-color palette. In the STe, the palette registers are accessible via the memory-mapped address range at `$FF8240-$FF823F`:

```
Address      Bits  Name
$FF8242      4 bits  Palette Color 3 Red MSB (4-bit)
$FF8243      4 bits  Palette Color 3 Green/Blue (depending on mode)
```

In the STe palette mode (bit 7 of the `$FF8240` register set), each of the 16 palette entries contains 4 bits per channel:

```
Palette Entry format (each of the 16 entries):
  Byte 1: R3 R2 R1 R0 G3 G2 G1 G0  (High nibble = red, Low nibble = green)
  Byte 2: B3 B2 B1 B0 0 0 0 0       (High nibble = blue, Low nibble reserved)
  
  Total per entry: 12 bits
  Total palette:   16 x 12 = 192 bits stored in internal palette RAM
```

The GST Shifter's DAC outputs three analog voltages (R, G, B) to the RGB connector (DIN 13). Each channel uses an R-2R ladder DAC network, with the 4-bit digital value driving the ladder to produce a proportional analog voltage. The DAC output range is approximately 0 V to 0.7 V per channel (for composite sync / standard RGB levels).

---

## Super Hi-Resolution Mode Details

### Pixel Clock Timing

The GST Shifter's Super Hi-Res mode leverages a **16 MHz color clock** (also called **PCLK** / **COLOR CLK**) derived from the system clock:

```
                        GST Shifter Clocking
                            
     32 MHz system clock
           |
     +-----+-----+
     |           |
  /2          /2
     |           |
     v           v
   16 MHz      16 MHz
 (PCLK)      (CMCLK)
     |           |
     v           v
 Shift regs.   MMU / counters
```

In normal operation, the Shifter has an **internal 32 MHz oscillator** (STi) that is divided by 2 to produce the **16 MHz pixel clock (PCLK)** and the **16 MHz color clock (CMCLK)** for NTSC.

In the STe, the 8 MHz **SCLK** (Sound Clock) input replaces the internal oscillator's role for sound DMA timing. The GST Shifter generates:

```
SCLK (8 MHz external input)
    |
    +---> Internal divider -- 16 MHz PCLK (pixel clock)
    +---> Internal divider -- 500 kHz (audio filter clock)
    +---> Internal divider -- audio sample clocks:
            6,250 Hz   (divide by 2560)
            12,500 Hz  (divide by 1280)
            25,000 Hz  (divide by 640)
            50,000 Hz  (divide by 320)
```

### Super Hi-Res Addressing

In Super Hi-Res 640x512 mode, the GST Shifter uses a **virtual addressing scheme**:

```
  Super Hi-Res 640x512 addressing:
  
  Horizontal:
    640 pixels / 16 bits/word = 40 words per half-line
    
  Vertical (interlaced):
    Field 1 (odd):  256 lines  (lines 0, 2, 4, ... 510)
    Field 2 (even): 256 lines  (lines 1, 3, 5, ... 511)
    Total: 512 lines per double-frame
    
  Video Address Counter (24 bits, read/write in STe):
    $FF8205  bits 21-16  (high byte)
    $FF8207  bits 15-8   (middle byte)
    $FF8209  bits 7-1    (low byte, bit 0 always 0)
    $FF820B  bits 17-14  (FCRDT - word skip during VBL)
  */
  
  Line Offset (FCSEL):
    $FF820F  bits 7-0    (FCSEL - words to skip per line during VBL)

  Frame Start Address (FBASE):
    $FF8201  bits 15-8  (FBASE high)
    $FF8203  bits 7-0   (FBASE low)
    /* maps to video RAM start (bits 6-1 used for word alignment) */

  Frame End Address (FEND):
    $FF820D  bits 15-8  (FEND high)
    $FF820F  bits 15-8  (FEND low, overlapping with FCSEL)
```

The **video address counter** tracks the current read position in video RAM. During each scanline, the counter increments by the number of words per line (40 for 640-pixel modes, 20 for 320-pixel modes). At the end of the active display area, the counter wraps and the next scanline begins.

In the STe, the **FCSEL (Frame Counter Select / HSCROLL)** register at `$FF8265` allows pixel-skip horizontal scrolling:

```
$FF8265:  0 0 0 0  X  X  X  X  (0-15 pixel skip)
  
  Bit 7:6:5:4: Reserved (always 0)
  Bit 3:0: Pixel offset (0-15 pixels)

  When combined with the video address counter, this effectively
  shifts the display window horizontally by up to 15 pixels,
  enabling smooth horizontal panning without moving video data.
```

The **fine scroll** registers provide word-level scrolling:

```
  Fine Scroll (FRDT) - vertical word offset:
    $FF820D:  (FCRDT - Frame Counter Read Delay)
    Controls how many words to skip at the start of each line.

  Line Offset (FCSEL):
    $FF820F:  (FCSEL - Frame Counter SELect)
    Used during VBL to offset the video address counter,
    giving word-level vertical fine scrolling.
```

### Super Hi-Res Pixel Clock Timing Diagram

```
  Horizontal timing (PAL, 640-pixel active):
  
  Line timing (~64 µs total):
  
  Pixel clock = 16 MHz (62.5 ns per pixel)
  
  |<--- active (640 px = 40 words) --->|
  |                                      |
  HSYNC pos:  |====|                       |
  (74 pixels) |    | <--- porch ---|      |
              |    |                |<---->|
              |    |   back porch   |  HBLANK (22 px)
              |    |                |      
  DE (Display Enable):  |<==================>|

  |------- front porch ------|<--|<-------- back porch --------|
  
  Typical PAL line timing:
  Total line:    ~64 µs  = ~1024 color clocks
  Active video:  40.24 µs = 644 pixels (includes 4 px overscan)
  HSYNC:         4.7 µs  = 74 pixels (-2 pulse sync)
  Front porch:   1.5 µs  = 25 pixels
  Back porch:    5.7 µs  = 91 pixels
  Total blank:   235 pixels (345 - 644 active range)
```

---

## Sync Generation

In the **original ST** (with C028787 Shifter), sync generation is split: HSYNC is generated by the GLUE chip, and VSYNC is generated by the MMU timing logic. The signals are fed into the Shifter via the **DE** (Display Enable), **VSYNC**, and **HSYNC** inputs.

In the **STe** (with C029145 GST Shifter), sync timing is handled by the **GST MCU**, which combines the GLUE and MMU functions:

```
  GST MCU Sync Generation:
  
  [GST MCU]                          [GST Shifter]
  +-----------+                      +---------------+
  |           |  HSYNC (active low)  |               |
  |-----------|---------------------->|               |
  |           |  VSYNC (active low)  |               |
  |-----------|---------------------->|               |
  |           |  DE (Display Enable) |               |
  |-----------|---------------------->| HIGH = active |
  |           |  \BLANK (active low) |---------------| \BLANK to blanking circuits
  |-----------|---------------------->|               |
  |           |  \DCYC (Data Cycle)  |               |
  |-----------|---------------------->| RAM data ok   |
  |           |  CMPCS (CycM PCS)    |               |
  |-----------|---------------------->| register sel  |
  |           |                      |               |
  |  50/60   |  REG5060 (config)    |               |
  |  Hz select|---------------------->| sync mode reg |
  +-----------+                      +-------+-------+
                                           |
                                           v
  Sync generation uses a counter fed by the 16 MHz PCLK:
  
  VSYNC:  Generated by counting scanlines
          PAL: 312.5 total lines (288 active, 24.5 total blank)
          NTSC: 262.5 total lines (240 active, 22.5 total blank)
          
  HSYNC:  Generated by counting color clocks per line
          PAL: ~1024 color clocks per line
          NTSC: ~911 color clocks per line (NTSC variant)

  \BLANK is asserted during the retrace (horizontal + vertical)
  to disable video output and prevent display outside the active
  display area.
```

The GST MCU generates sync by using internal counters clocked at the pixel clock rate. The 50/60 Hz configuration register at `$FF8260` (in the GST MCU, not the Shifter) selects between the two timing standards. The GST MCU then feeds HSYNC and VSYNC back to the Shifter, which uses DE (Display Enable) to gate the actual pixel output.

---

## Bus Interface and Control Registers

### Bus Interface

The GST Shifter interfaces with the 68000 system via three distinct buses:

```
  +--------------------+
  |   68000 Bus (16b)  |  <--> Address/Data bus $FF82XX (Shifter regs)
  +--------------------+        <--> MMU-regulated RAM access
        |         |
  \RDAT/WDAT  LATCH   <--> RAM data bus isolation (internal TTL)
        |         |
  +--------------------+
  |   RAM Bus (16b)    |  Direct connection to RAM banks
  +--------------------+        16-bit parallel access
        |
  +--------------------+
  |  Sound DMA Bus (8b)|  SDO0-SDO7 -> DAC pair (left/right audio)
  +--------------------+
```

**Signal Summary:**

| Signal | Direction | Description |
|---|---|---|
| DATA[15:0] | Bidir | 16-bit data (CPU or RAM) |
| A0-A4 | In (internal) | Register address lines within Shifter |
| \RDAT | In (from MMU) | RAM data latch enable |
| \WDAT | In (from MMU) | Write data to RAM latch |
| LATCH | In (from MMU) | Latch 6800 bus data onto RAM bus |
| \DCYC | In (from GST MCU) | Data cycle - RAM data available |
| DE | In (from GST MCU) | Display Enable (HIGH = active video) |
| VSYNC | In (from GST MCU) | Vertical sync pulse |
| HSYNC | In (from GST MCU) | Horizontal sync pulse |
| \BLANK | In (from GST MCU) | Blank video output during retrace |
| \CMPCS | In (from GST MCU) | Cycle Mux PCS - select Shifter registers |
| SREQ | Out (to GST MCU) | Sound DMA REQuest |
| \SLOAD | In (from GST MCU) | Sound DMA LOAD (data available on SDO bus) |
| SDO[7:0] | Out (to DACs) | Sound DMA data output bus |
| \LD | Out (to left DAC) | Left channel DAC latch |
| \RD | Out (to right DAC) | Right channel DAC latch |
| MWE | Out (to LMC1992) | MicroWire Enable |
| MWD | Out (to LMC1992) | MicroWire Data |
| MWK | Out (to LMC1992) | MicroWire Clock |
| COLOR | Out (to composite) | NTSC color clock output |
| R, G, B | Out (to monitor) | RGB analog outputs (via external DAC network) |

### Register Interface

The GST Shifter registers are memory-mapped in the I/O space at address range `$FF8000-$FFFFDF`. The GST MCU decodes this address range and routes accesses:

- `$FF8000-$FF81FF`: DMA sound registers (in Shifter)
- `$FF8200-$FF82FF`: Video registers (split between GST MCU and GST Shifter)
- `$FF8900-$FF89FF`: Additional MMU registers

The GST MCU's **\CMPCS** signal is asserted when accessing Shifter registers, enabling the Shifter's internal register file.

---

## Register Map for GST Shifter

### GST Shifter Internal Registers

```
  +------------------------------------------------------------+
  |          GST SHIFTER C029145 REGISTER MAP                   |
  +------------------------------------------------------------+

  Address Range     Register              Type   Description
  =============     ========              ====   ===========

                        VIDEO REGISTERS (GST Shifter internal)
  --------------------                    ------  --------------------------
  $FF8240           VIDMOD                R/W    Video Mode / Resolution
                                              Bit 7: Palette mode (0=ST 512, 1=STe 4096)
                                              Bit 6: V-Counter read mode
                                              Bit 5: H-Counter read mode
                                              Bit 4-3: Reserved
                                              Bit 2:  Super Hi-Res enable
                                              Bit 1:  MCM enable
                                              Bit 0:  Resolution (0=16-col mode, 1=mono hi-res)

  $FF8242           Palette (16 entries)  R/W    16 color palette entries
                                              (each entry: 8 bits in ST mode,
                                               12 bits in STe palette mode)
                                              Palette index comes from pixel data
                                              (bits 8-12 of the video data word)

                        SOUND DMA REGISTERS
  -------------------                     ------  --------------------------
  $FF8900           Frame Start (FSR) Hi  R/W    Sound DMA frame start
  $FF8901           Frame Start (FSR) Lo  R/W    address (18-bit,
                                              bits 16-7 = base word address)
  $FF8905           Frame End (FER) Hi    R/W    Sound DMA frame end
  $FF8906           Frame End (FER) Lo    R/W    address (18-bit)
  $FF890D           SAMPMOD               R/W    Sample mode
                                              Bit 7-6: Mono/Stereo
                                              Bit 5-4: Sample rate
                                                  00 = 6,250 Hz
                                                  01 = 12,500 Hz
                                                  10 = 25,000 Hz
                                                  11 = 50,000 Hz
                                              Bit 3-0: Reserved
  $FF890F           DACSEL                W      DAC select / output
  $FF8910           SNDCTRL               R/W    Sound control
                                              Bit 1-0: Control
                                                  0 = Stop DMA
                                                  1 = Start DMA (one-shot)
                                                  2 = Start DMA (loop)
                                                  3 = Reserved
  $FF8901           LMCDATA               W      LMC1992 data (6 bits)
  $FF890E           LMCMASK               W      LMC1992 command register (3 bits)
```

### Video Control Registers (Shared GST MCU / Shifter)

```
  +------------------------------------------------------------+
  |     VIDEO / SCROLLING REGISTERS (GST MCU + Shifter)        |
  +------------------------------------------------------------+
  
  Address               Register        Type   Description
  ========              ========        ====   ===========

  $FF8201-FBASE-H       Frame Base     R/W    Video RAM start address (high byte)
  $FF8203-FBASE-L       Frame Base     R/W    Video RAM start address (low byte)
                                              (bits 5-0: word offset within 32K page)

  $FF8205               VCounter Hi     R/W    Video Counter bits 21-16
  $FF8207               VCounter Mid    R/W    Video Counter bits 15-8
  $FF8209               VCounter Lo     R/W    Video Counter bits 7-1 (bit 0 = 0)
  
  $FF820B-FCRDT         FCRDT           R/W    Frame Counter Read Delay
                                              (words to ignore at start of line)

  $FF820D-FEND-H        Frame End       R/W    Frame end address (high byte)
                                              overlapped with FCRDT at same address
  $FF820F-FCSEL         FCSEL           R/W    Frame Counter Select
                                              (words to skip during VBL)
                                              overlapped with FEND-H

  $FF8211               V-Scroll Hi      R/W    Vertical scroll offset (high)
  $FF8213               V-Scroll Lo      R/W    Vertical scroll offset (low)

  $FF8265               HSCROLL          R/W    Horizontal pixel shift (0-15 pixels)
                                              Bits 3-0: pixel offset
                                              Bits 7-4: reserved (0)
  
  $FF8260               Configuration    R/W    50/60 Hz select + V-Counter/
                                              H-Counter read enable (in GST MCU)
```

### Register Map Summary Table (ASCII Art)

```
  +------------------+---------+-------+------------------------------------------+
  | Address          | Bits    | Name  | Function                               |
  +------------------+---------+-------+------------------------------------------+
  | $FF8201          | B15-B8  | FSRH  | Frame Start Register High (Sound DMA)  |
  | $FF8203          | B7-B0   | FSRL  | Frame Start Register Low (Sound DMA)   |
  | $FF8205          | B7-B4   | V21-16| V-Counter High bits (Video address)    |
  | $FF8207          | B7-B0   | VMID  | V-Counter Mid bits                       |
  | $FF8209          | B7-B1   | VLOW  | V-Counter Low bits (B0=0)               |
  | $FF8209          | B0      | -     | Fixed to 0                               |
  | $FF820B          | B7-B0   | FCRDT | Frame Count Read Delay                   |
  | $FF820D          | B15-B8  | FEHI  | Frame End High (overlaps FCSEL)          |
  | $FF820F          | B15-B8  | FELO  | Frame End Low                            |
  | $FF820F          | B7-B0   | FCSEL | Frame Counter Select                     |
  | $FF8211          | B15-B8  | VSHI  | Vertical Scroll High offset               |
  | $FF8213          | B7-B0   | VSHL  | Vertical Scroll Low offset                |
  | $FF8240          | B7-B0   | VIDMOD| Video Mode register                        |
  | $FF8242 x16     | B7-B0   | PALxx | 16 Palette entries (Color 0-15)           |
  | $FF8260          | B7-B0   | CONF  | Configuration (50/60 Hz + counters)      |
  | $FF8265          | B3-B0   | HSCRL | Horizontal pixel shift (0-15)             |
  | $FF8265          | B7-B4   | -     | Reserved                                  |
  | $FF8900          | B15-B8  | FSRH  | Sound Frame Start High                      |
  | $FF8901          | B7-B0   | FSRL  | Sound Frame Start Low (also LMCDATA)      |
  | $FF8905          | B15-B8  | FEHI  | Sound Frame End High                        |
  | $FF8906          | B7-B0   | FELO  | Sound Frame End Low                         |
  | $FF890D          | B7-B0   | SAMPM | Sample Mode (rate + stereo/mono)          |
  | $FF890F          | B7-B0   | DACSEL| DAC Select                                |
  | $FF8910          | B7-B0   | SNDCT | Sound Control (start/stop/loop)           |
  | $FF890E          | B7-B0   | LMCMK | LMC1992 command mask                       |
  +------------------+---------+-------+------------------------------------------+
```

---

## GST Shifter Internal Block Diagram

```
  +------------------------------------------------------------------------+
  |                                                                        |
  |                    GST SHIFTER C029145 (Atari STe)                     |
  |                                                                        |
  |  +===============+  +============+  +===============================+   |
  |  |   68000 Bus   |  | Register  |  |         Video Engine           |   |
  |  |   Interface   |->|   File     |->|                                 |   |
  |  |   (16-bit)    |  | $FF82xx   |  |  +-----+  +-----+  +---------+ |   |
  |  +===============+  +------------+  |  | PAL |->| DAC |->| RGB Out | |   |
  |                                      |  | Reg |  | (12b  |  | (DIN13)| |   |
  |  +================================+  |  +-----+  | 4/3/4|  +---------+ |   |
  |  |                                   |  |Palette| |bit)   |             |   |
  |  |  +-----------------------------+  |  |Reg    |           |   Sync     |   |
  |  |  |                             |  |  |File   |  +-------+   | Gen |   |
  |  |  |     RAM Data Path            |->|  +---|<--| CMCLK  |<--|From |   |
  |  |  |                             |  |  | |   |  | (16MHz)|   |GSTMCU|   |
  |  |  +-----------------------------+  |  | |   |  +-------+   |+------)|   |
  |  |                                   |  | |   |              | |DE,    |   |
  |  |  +-----------------------------+  |  +--|--+  +----------+V| HS,  |   |
  |  |  |                             |  |    |    |  Sound     |B| VSync|   |
  |  |  |     Shift Register Array     |    |    |  DMA        | | |BLANK}|   |
  |  |  |                             |    |    |  Engine     | | |      |   |
  |  +--|----------------------------|----|    |    +----------+  | +----+ |   |
  |     |  +-----+  +-----+  +-----+     |    |                 |        |   |
  |     +->|SR 0|->|SR 1|->|SR 2|-- ...  |    |          +------v------+  |   |
  |        +-----+  +-----+  +-----+     |    |          | 8-Bit DAC  |  |   |
  |         1 bit  1 bit   1 bit          |    |          | pair (L/R) |  |   |
  |        (4x parallel shift registers)  |    |          +------v------+  |   |
  |                                        |    |                 SDO0-7   |   |
  |                                        |    |                 |         |   |
  |  +===================================+    +==================|=========+   |
  |  |                                    |                         |           |
  |  |  +-----------------------------+   |     +----------+       |           |
  |  |  |  Address Decoder             |<-+<----| Internal |<------+           |
  |  |  |  (pixel clock / color clock) |       | | Palette |               |   |
  |  |  +-----------------------------+   |     | | Register|               |   |
  |  |                                    |     +----------+               |   |
  |  |  +-----------------------------+   |                                 |
  |  +->|  Pixel / Color Clock         |   |     +----------+               |
  |     |  Generator (16 MHz PCLK)     |---->|  LMC1992 |<-- MWE, MWD, MWK |
  |     +-----------------------------+     |  (Audio)   |  (Microwire)     |
  |                                          +----------+                    |
  +----------------------------------------------------------------------+-+
  |                                                                        |
  |  Clocks: SCLK (8 MHz ext input) -> internal dividers ->             |
  |  16 MHz PCLK, 500 kHz filter, audio sample clocks                   |
  |                                                                        |
  +------------------------------------------------------------------------+
```

---

## Comparison: Original Shifter vs GST Shifter

```
  +----------------------------------+------------------+------------------+
  | Feature / Parameter              | Shifter C028787  | GST Shifter      |
  |                                  | (Original ST)    | C029145 (STe)    |
  +----------------------------------+------------------+------------------+
  | Function                         | Video controller | Video + Sound    |
  |                                  | (video DMA only) | DMA controller   |
  +----------------------------------+------------------+------------------+
  | Package                          | 64-pin QFP       | 64-pin QFP       |
  +----------------------------------+------------------+------------------+
  | CMOS / NMOS                      | NMOS             | CMOS             |
  +----------------------------------+------------------+------------------+
  | Internal oscillator              | 32 MHz STi       | None (external   |
  |                                  |                  | SCLK 8 MHz)      |
  +----------------------------------+------------------+------------------+
  | Pixel clock                      | 16 MHz (from /2  | 16 MHz (from     |
  |                                  |  of STi)         | internal /2 of   |
  |                                  |                  | SCLK via DACSEL) |
  +----------------------------------+------------------+------------------+
  | Color clock (NTSC)               | 16 MHz           | 16 MHz / output  |
  |                                  |                  | on COLOR pin     |
  +----------------------------------+------------------+------------------+
  | DAC type                         | R-2R ladder      | R-2R ladder (    |
  |                                  |  (per channel)   | per channel)     |
  +----------------------------------+------------------+------------------+
  | DAC resolution                   | 9-bit (3+3+3)    | 12-bit (4+4+4)   |
  +----------------------------------+------------------+------------------+
  | Palette size                     | 16 entries       | 16 entries       |
  |                                  | 512 total colors | 4,096 total      |
  |                                  | 3-bit/channel    | 4-bit/channel    |
  +----------------------------------+------------------+------------------+
  | Video modes                      | Mode 0: 320x200 | All original +   |
  |                                  | Mode 1: 640x400 | MCM 16-color     |
  |                                  | Mode 2: 640x200 | + Super Hi-Res   |
  |                                  | (mono)           | + Double-density |
  +----------------------------------+------------------+------------------+
  | Fine scrolling (vertical)        | Yes (VBASE)      | Yes (FCSEL,      |
  |                                  |                  | VScroll regs)    |
  +----------------------------------+------------------+------------------+
  | Fine scrolling (horizontal)      | No               | Yes (HSCROLL)    |
  |                                  |                  | (0-15 pixel      |
  |                                  |                  | shift)           |
  +----------------------------------+------------------+------------------+
  | Super Hi-Res (640x512 interl.)   | No               | Yes              |
  +----------------------------------+------------------+------------------+
  | MCM 16-color (640x200 int.)      | No               | Yes              |
  +----------------------------------+------------------+------------------+
  | Sound                            | None             | 8-bit stereo     |
  |                                  |                  | DMA (6.25/12.5/  |
  |                                  |                  | 25/50 kHz)       |
  +----------------------------------+------------------+------------------+
  | Sound output interface           | N/A              | SDO0-7 bus +     |
  |                                  |                  | 2x DAC pair +    |
  |                                  |                  | LMC1992 mixer    |
  +----------------------------------+------------------+------------------+
  | LMC1992 Microwire control        | No               | Yes (MWE,MWD,    |
  |                                  |                  | MWK pins)        |
  +----------------------------------+------------------+------------------+
  | Bus interface                    | 16-bit (video)   | 16-bit (video +  |
  |                                  |                  | 8-bit (sound)    |
  +----------------------------------+------------------+------------------+
  | Data bus latch                   | 4x external TTL  | Integrated       |
  |  (RAM bus isolation)             |  (244/373)       | (4x TTL equiv.)  |
  +----------------------------------+------------------+------------------+
  | Sync signals                     | IN: DE, VSYNC,  | IN: DE, VSYNC,  |
  |                                  | HSYNC, BLANK    | HSYNC, BLANK    |
  +----------------------------------+------------------+------------------+
  | Sync generation                  | In GLUE (ST)    | In GST MCU (STe) |
  +----------------------------------+------------------+------------------+
  | Register address range           | $FF8200-$FF8242 | $FF8200-$FF8265  |
  |                                  |                  | + $FF89xx (sound) |
  +----------------------------------+------------------+------------------+
  | Address bus lines (reg select)   | A0-A4 (5 bits)   | A0-A5 (6 bits)   |
  +----------------------------------+------------------+------------------+
  | Video RAM addressing             | 18-bit           | 24-bit (read/    |
  |                                  | (FBASE, VCNT)    | write VCNT)      |
  +----------------------------------+------------------+------------------+
  | Frame Start (Sound)              | N/A              | $FF8900-$FF8901  |
  +----------------------------------+------------------+------------------+
  | Frame End (Sound)                | N/A              | $FF8905-$FF8906  |
  +----------------------------------+------------------+------------------+
  | V-Counter                        | Read-only        | Read/write       |
  +----------------------------------+------------------+------------------+
  | Horizontal counter               | Read-only        | Read/write       |
  +----------------------------------+------------------+------------------+
  | External clock input             | None (int osc)   | SCLK (8 MHz ext) |
  +----------------------------------+------------------+------------------+
  | Genlock support                  | Via external     | Via SCLK ext     |
  |                                  | oscillator       | oscillator       |
  +----------------------------------+------------------+------------------+
  | Maximum resolution               | 640x400 mono     | 640x512 color    |
  |                                  |                  | (interlaced)     |
  +----------------------------------+------------------+------------------+
  | Power consumption                | Lower (NMOS)     | Higher (CMOS     |
  |                                  |                  | still low-power) |
  +----------------------------------+------------------+------------------+
  | Number of GST ASICs (STe)        | 1 (Shifter)     | 2 (Shifter +     |
  |                                  | + GLUE + MMU    | MCU)             |
  +----------------------------------+------------------+------------------+
```

---

## Clock Generation Details

The GST Shifter's clock architecture differs significantly from the original Shifter:

```
  Original ST (C028787):
  +-------------+       +--------------+
  | 32 MHz Int. | /2    |  16 MHz PCLK |----> Pixel clock
  | Oscillator  |       |  (Color Clock)|----> NTSC CMCLK
  | (STi)       |       +--------------+
  +-------------+
  
  STe (C029145):
  +-------------+  SCLK  +--------------+
  | 8 MHz Ext.  |------->|  Internal    | /2 -> 16 MHz PCLK
  | Oscillator  |        |  Dividers    | /32 -> 500 kHz
  | (SCLK)      |        +-----+--------+
  +-------------+              |
                               +-> Audio sample clocks:
                                  /2560 = 6,250 Hz
                                  /1280 = 12,500 Hz
                                  /640  = 25,000 Hz
                                  /320  = 50,000 Hz
```

The external SCLK (8 MHz) input on the GST Shifter is critical because the sound DMA's sample rate accuracy depends on a stable clock source. If the STe is genlocked to an external video source (e.g., 36 MHz), the system clock changes but the 8 MHz SCLK remains stable, ensuring the audio sample rates do not shift.

---

## References and Further Reading

- Atari STE Field Service Manual (C302481-001 Rev A, August 1991)
- "The Atari STE Hardware" by HardmaSTer, ST Magazine No. 44, September 1990
- "Atari ST Internals" by K. Gerits, L. Englisch, R. Bruckmann (Data Becker / Abacus Software)
- "Atari STe Developer Information Addendum" (Atari Corporation)
- GSTMCU Simulation Model - György Szombathy, GitHub: gyurco/gstmcu
- ST Shifter Verilog Model - Steve Monson, GitHub: smonson78/st-shifter-verilog
- AtariST_MiSTer Project - MiSTer-devel GitHub organization
- "Shifter Distortions" by Dbug, Defence Force blog
- "Atari ST FAQ" compiled by The Paranoid / Paradox, AtariForumWiki

---

*This documentation is for educational and reference purposes. Chip part numbers C029145 (GST Shifter) and C028787 (original Shifter) are Atari Corporation proprietary ASIC part numbers.*
