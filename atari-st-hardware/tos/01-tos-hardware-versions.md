# Atari ST Hardware TOS Version Differences

## Overview of TOS Across Atari Computers

TOS (The Operating System) is the proprietary operating system developed by Atari Corporation for its 16/32-bit ST line of personal computers, introduced with the Atari 520ST in November 1985. TOS combines Digital Research's GEM (Graphics Environment Manager) GUI running on top of GEMDOS (GEM Disk Operating System), along with the BIOS (Basic Input/Output System), XBIOS (Extended BIOS), and Line-A low-level graphics calls.

TOS was always delivered in ROM chips on the standard ST/STe/MegaSTE models, enabling near-instant boot times -- a significant advantage over floppy-booted competitors. Starting with TOS 1.04, all ST models shipped with the latest TOS version in ROM within about six months of the hardware introduction.

TOS evolved in four major version families across the ST line:

- **TOS 1.x** -- Base ST/ST 512/ST XL/ST XM and ST FM lines (68000 CPU)
- **TOS 2.x** -- STe (Super STE) line with enhanced shifter, blitter, and DMA sound
- **TOS 3.x** -- TT030 line with 68030 CPU and VME bus support; TOS 3 also applied to MegaSTE
- **TOS 4.x** -- Falcon030 line with DSP56001 audio processor and MultiTOS multitasking; last official Atari TOS was 4.04

TOS is not monolithic across models: the ROM image layout differs by hardware generation. ST ROMs use 2-chip (64KB each) or 6-chip (32KB each) configurations total 192KB. STE ROMs use 2-chip 256KB configuration. TT ROMs use 4-chip 512KB configuration. The ROM checksum is validated at startup to detect corruption.

---

## TOS 1.x (ST / ST 512 / ST XL / ST XM)

### Hardware Detection

On original ST models, the ROM detects the hardware configuration at boot by examining:

1. **Configuration register at `$FFFF8104`** (byte) -- Bits 0-3 indicate the RAM size (e.g., bit 0 = 512KB, bits 0-1 = 1MB/4MB variants). Bit 7 indicates PAL/NTSC region.
2. **RAM sizing via `$FFFF8202`** -- The memory controller status register reports installed SIMM capacity. For the 520ST this reads 512KB; for 1040ST it reads 1MB.
3. **Shifter identification** -- The GTIA-compatible shifter chip is identified by its fixed register map at `$FF8000-$FFFF7F` (blitter) and `$FF8200-$FFFFBF` (VDP). ST models have a 40-column mono shifter (GCW/MDEN variant).
4. **Boot device type** -- TOS 1.0x checks if the boot disk has a valid Atari checksum (add all 256 words of sector 0; result must be `0x1234`).

### TOS 1.x Sub-Versions and Features

| Version | ROM Date | Firmware Size | New Features |
|---------|----------|---------------|--------------|
| 1.0 | 20 Nov 1985 | 192KB (6-chip) | First ROM TOS. Floppy boot. 520ST/1040ST |
| 1.02 | 22 Apr 1987 | 192KB (6-chip) | Added Blitter co-processor support, RTC (Real-Time Clock) support |
| 1.04 | 06 Apr 1989 | 192KB (6-chip) | "Rainbow TOS". DOS-compatible disk format, improved file selector, autorun GEM programs, 1040STF/M, Mega 1/ST FM, Mega 2/ST FME, Stacy PDA |
| 1.06 | 29 Jul 1989 | 256KB (2-chip) | First STE-compatible TOS. Extended blitter, stereo DMA sound, super hi-res modes. Required STE_FIX.PRG patch |
| 1.62 | 01 Jan 1990 | 256KB (2-chip) | Bug fixes for 1.06 STE TOS |

### BIOS / XBIOS Calls Available in TOS 1.x

Standard ST-era calls (via `trap #14`):

- **XBIOS 0** `Initmouse` -- Initialize mouse
- **XBIOS 2** `Physbase` -- Return physical screen base address
- **XBIOS 3** `Logbase` -- Return logical screen base address
- **XBIOS 4** `Getrez` -- Get/reset vertical retrace flag
- **XBIOS 5** `Setscreen` -- Set screen address and display mode
- **XBIOS 6** `Setpalette` -- Set display palette
- **XBIOS 7** `Setcolor` -- Set foreground/background color
- **XBIOS 8** `Floprd` -- Read sector from floppy
- **XBIOS 9** `Flopwr` -- Write sector to floppy
- **XBIOS 10** `Flopfmt` -- Format floppy disk
- **XBIOS 12** `Midiws` -- MIDI write status
- **XBIOS 13** `Mfpint` -- Modify MFP interrupt mask
- **XBIOS 14** `Iorec` -- Obtain pointer to I/O request block
- **XBIOS 15** `Rsconf` -- Configure serial port
- **XBIOS 16** `Keytbl` -- Load keyboard table
- **XBIOS 18** `Protobt` -- Obtain floppy format prototype
- **XBIOS 20** `Scrdmp` -- Screen dump to printer
- **XBIOS 21** `Cursconf` -- Configure cursor
- **XBIOS 22/23** `Settime`/`Gettime` -- Set/get real-time clock
- **XBIOS 25** `Ikbdws` -- Intercom keyboard write status
- **XBIOS 26/27** `Jdisint`/`Jenabint` -- Jack interrupt disable/enable
- **XBIOS 28/29/30** `Giaccess`/`Offgibit`/`Ongibit` -- Jack I/O
- **XBIOS 31** `Xbtimer` -- Set up XBIOS timer
- **XBIOS 32** `Dosound` -- ST built-in speaker sound
- **XBIOS 38** `Supexec` -- Execute in supervisor mode
- **XBIOS 40** `Blitmode` -- Set blitter mode
- **XBIOS 42/43** `DMAread`/`DMAwrite` -- DMA drive access (ST era)
- **XBIOS 44** `Bconmap` -- Map serial device (TOS 1.02+)
- **XBIOS 48** `Floprate` -- Set floppy motor rate (TOS 1.04+)

TOS 1.x does **not** support: DMA stereo sound control register access, blitter copy mode operations, super hi-res display modes, cookie jar (introduced in TOS 1.06).

### Key Limitations

- Single-channel (mono) speaker sound via internal piezo transducer
- Maximum resolution: 640x512 (EHIRES) with external monitor
- No blitter copy operations (only blitter pixel operations)
- No DMA sound controller
- No cookie jar for system variable discovery
- 640KB boot sector checksum required

---

## TOS 2.x (STe)

### New Hardware Features

The Atari STe (Super STE), released in 1989, introduced several hardware enhancements that required TOS 1.06 and later 2.x support:

1. **Super Hi-Res Video Modes** -- The enhanced shifter chip supports additional resolutions:
   - 640x256 (4 colors)
   - 640x512 (2 colors, EHIRES)
   - 320x512 (4 colors)
   - 320x260 (16 colors, HRES)
   - 320x120 (4 colors)
   - 160x120 (16 colors)

2. **Stereo DMA Sound** -- New 2-channel 8-bit PCM stereo sound at 6.25/12.5/25/50 kHz via SMA connectors (RCA audio out). Control registers at `$FF8800-$FF880B`:
   - `$FF8800` -- Sound DMA control (bit 7 = enable, bit 6 = stereo/mono)
   - `$FF8801` -- Sample frequency select
   - `$FF8804` -- Frame start address (long)
   - `$FF8808` -- Frame end address (long)

3. **Blitter Enhancements** -- STe adds blitter copy mode (mode `$C000`) and expanded blitter register set:
   - `$FF8200` -- Blitter control/status
   - `$FF8204` -- Blitter A register
   - `$FF8208` -- Blitter B register
   - `$FF820C` -- Blitter C register
   - `$FF8210` -- Blitter X/Y register

4. **Super CMOS** -- New configuration register at `$FFFF8743` for STE-specific settings, including display mode, stereo enable, and refresh settings.

5. **Cookie Jar** -- Introduced in TOS 1.06/2.06, the cookie jar is a system variable table at the end of available RAM (pointed to by the system variable `COOKIEJAR`), allowing software to discover hardware capabilities without hardcoded version checks.

### TOS 2 Sub-Versions

| Version | ROM Date | Machines | Notes |
|---------|----------|----------|-------|
| 2.02 | -- | Mega STE | Early Mega STE TOS |
| 2.05 | 05 Dec 1990 | Mega STE | Mega STE TOS with 720K floppy |
| 2.06 | 14 Nov 1991 | ST/STE | Last TOS for ST/STE. 1.44MB support, memory test, GTP programs, IDE hard disk boot, GEM enhancements |
| 2.07 | -- | Falcon prototype | Used on "FX-1" Falcon prototype |
| 2.08 | -- | ST-Book | Notebook-specific utilities (STTRANS, power management, AHDI/XHDI drivers) |

### New XBIOS Calls Added in TOS 2

| XBIOS | Name | Purpose |
|-------|------|---------|
| 44 | `Bconmap` | Map serial device to BIOS device |
| 47 | `Waketime` | Set wake-on-CMU timer (STE) |
| 64 | `EsetShift` | Set screen shift register (STE) |
| 65 | `EgetShift` | Get screen shift register (STE) |
| 66 | `EsetBank` | Set screen bank (STE) |
| 67 | `EsetColor` | Set TT/Falcon palette color |
| 68 | `EsetPalette` | Set TT/Falcon palette |
| 69 | `EgetPalette` | Get TT/Falcon palette |
| 70 | `EsetGray` | Set grayscale palette |
| 71 | `EsetSmear` | Set smear correction |

---

## TOS 3.x (MegaSTE / TT030)

### MegaSTE Hardware

The Atari MegaSTE, released in July 1991, combined the STe architecture in a compact case with:

- **VME Bus Support** -- The MegaSTE added a VME bus connector for expansion cards. VME bus control registers reside in the MegaST's PIA/AICA chips, with bus arbitration handled by the AICA (Atari Interrupt Control) chip.
- **68000 CPU at 16MHz** (vs 8MHz in earlier STEs) (68EC000 at 8MHz)
- **16KB on-chip cache** for the 68000
- **Internal SCSI hard disk** (1MB model: external only; 2MB/4MB models: internal SCSI HDD)
- **Two extra RS-232 ports** (9-pin instead of 25-pin)
- **LocalTalk/RS-422 port**
- **1.44MB HD floppy** support
- **Software-switchable CPU speed** (8/16MHz)

### TOS for MegaSTE

MegaSTE uses TOS 2.05 (2-chip 256KB ROM, dated 05 Dec 1990) or TOS 2.06. TOS 2.x on MegaSTE adds VME bus I/O routines and the cookie jar entries for MEGASTE machine identification (`MEGA` cookie).

### TT030 (TOS 3.0x Series)

The Atari TT030, released in 1990, introduced significant architecture changes:

- **68030 CPU at 24MHz** (true 32-bit data/address bus)
- **16MB fast RAM** (80ns) vs standard ST RAM (120ns)
- **Enhanced video** with TT screen resolutions
- **Dropped Line-A API** for extended TT functionality, forcing VDI-compliant calls
- **Enhanced GEM AES** (v3.40) with pop-up menus, 3D window objects, 256-color animated icons

### TOS 3.x Sub-Versions

| Version | Machines | ROM | Notes |
|---------|----------|-----|-------|
| 3.01 | TT030 | 512KB (4-chip) | Initial TT TOS |
| 3.05 | TT030 | 512KB (4-chip) | TT with enhanced memory management |
| 3.06 | TT030 | 512KB (4-chip) | Final TT TOS, improved memory handling |

### New XBIOS Calls for TOS 3

| XBIOS | Name | Purpose |
|-------|------|---------|
| 66 | `NVMaccess` | Non-volatile RAM access (TT/STe NVRAM) |

---

## TOS 4.x (Final Version)

### TOS 4 Overview

TOS 4 was the final official version family from Atari, designed for the Falcon030 line. It introduced MultiTOS (preemptive multitasking kernel) and extensive multimedia capabilities.

### Falcon030 Hardware Features

- **68030 CPU at 16MHz**
- **DSP56001 Digital Signal Processor** for audio processing
- **16MHz blitter** (doubled rate over STe's 8MHz blitter)
- **Video overlay** support
- **Enhanced AES 3.40** with extended features

### TOS 4 Sub-Versions

| Version | Machines | ROM | Notes |
|---------|----------|-----|-------|
| 4.00 | Falcon030 | 512KB (4-chip) | Initial Falcon TOS |
| 4.01 | Falcon030 | 512KB (4-chip) | Falcon updates |
| 4.02 | Falcon030 | 512KB (4-chip) | Falcon updates |
| 4.04 | Falcon030 | 512KB (4-chip) | Last official Atari TOS |
| 4.08 | Milan | 512KB (4-chip) | Milan Computersysteme, 68040/60 |

### TOS 4 Enhancements

- **MultiTOS multitasking kernel** -- True preemptive multitasking across T, TTP, and PGM program types
- **AES 3.40** -- Pop-up menus, 3D window/dialog objects, 256-color animated icons, soft-loaded fonts, inter-app drag-and-drop, background window manipulation
- **DSP56001 sound processor** -- Multiple audio streams via new XBIOS DSP functions (XBIOS 96-127)
- **New CPX modules** -- International/localization configuration
- **V4.04** -- Last official Atari TOS release (pre-V4.92 prototypes leaked)

---

## Hardware Detection Routines

### How TOS Determines Which Model It Runs On

At startup, TOS executes the following detection sequence:

1. **Read configuration byte at `$FFFF8104`** (byte read)
   - Bit 0 = 1: 68000 family (all ST line)
   - Bit 1 = 1: STE family detected (via Super CMOS chip identification)
   - Bit 2-3 = machine type code (00 = ST, 01 = STF, 10 = STe, 11 = MegaSTE)
   - Bit 7 = 1: PAL region, 0: NTSC region

2. **Read memory controller at `$FFFF8202`**
   - Reports detected SIMM size in 256KB increments
   - For ST: bits encode 512KB (value 2) or 1024KB (value 4)
   - For STE: larger values supported (up to 16 = 4MB)

3. **Shifter chip identification**
   - Read shifter revision register at `$FFFC03`
   - ST shifter returns specific revision code
   - STE shifter returns extended revision code with blitter capability flag

4. **ROM configuration byte at `$FFFF800A`**
   - Bits 0-1: 60Hz/50Hz display standard (reset defaults to 60Hz, checked against this byte)
   - Bits 2-3: RAM configuration
   - Bit 4: Presence of MFP (Multi-function Peripheral) chip
   - Bit 7: ROM type flag (determines 2-chip vs 6-chip ROM layout)

5. **Memory test** (TOS 2.06+)
   - Write/verify test patterns (word `$FB55`, 43 words per page)
   - 128MB page size for testing
   - Bus error indicates end of physical memory
   - Results stored in system variable `phystop`

6. **Cookie jar enumeration** (TOS 1.06+/2.06+)
   - Walk system cookie jar from end of RAM
   - Each cookie: 4-byte ID + length + value
   - NULL cookie (`0x00000000`) marks end; value field = entry count

### Key Detection Memory Locations

| Address | Size | Purpose |
|---------|------|---------|
| `$FFFF8000` | byte | System configuration register |
| `$FFFF8003` | word | TOS version (high word = major, low word = minor) |
| `$FFFF800A` | byte | ROM configuration byte (region, RAM, MFP flags) |
| `$FFFF8104` | byte | Machine type / hardware config |
| `$FFFF8202` | word | RAM size report from memory controller |
| `$FFFF833C` | long | Cookie jar pointer |
| `$FFFF8346` | long | `physbase` -- physical screen base |
| `$FFFF834A` | long | `logbase` -- logical screen base |
| `$FFFF8382` | long | `phystop` -- end of physical memory |
| `$FF8000-$FFFF7F` | -- | Blitter register map (ST) |
| `$FF8200-$FFFFBF` | -- | VDP register map (ST) |
| `$FF8800-$FF880B` | -- | Sound DMA control registers (STE) |
| `$FF8C00-$FF8C0F` | -- | Blitter copy registers (STE) |
| `$FFFF8743` | byte | STE Super CMOS register |

---

## TOS Version by Model Table

```
+-------------------+----------+----------+----------+----------+--------+---------------------------+
| Model             | Base TOS | 1.x      | 2.x      | 3.x      | 4.x    | Notes                     |
+-------------------+----------+----------+----------+----------+--------+---------------------------+
| 520ST / 520STF    | 1.0      | 1.0/1.02 | 1.04/1.06| --       | --     | 512KB RAM, 2-chip ROM    |
| 520STe            | 1.06     | 1.06     | 1.62/2.06| --       | --     | 1MB RAM, STE shifter     |
| 1040ST / 1040STF  | 1.0      | 1.02/1.04| 1.06/2.06| --       | --     | 1MB RAM, 2-chip ROM      |
| 1040STFM          | 1.02     | 1.02/1.04| 1.06/2.06| --       | --     | 1MB RAM, floppy built-in |
| Mega 1 / Mega ST  | 1.02     | 1.02/1.04| 1.06/2.06| --       | --     | 1MB RAM, internal HDD bay|
| Mega 2 / Mega STF | 1.02     | 1.02/1.04| 1.06/2.06| --       | --     | 1MB RAM, SCSI/ACSI       |
| Mega STE          | 2.05     | 2.06     | 2.05/2.06| --       | --     | VME bus, 16MHz option    |
| TT030             | 3.01     | --       | --       | 3.01-3.06| --     | 68030, 24MHz, 512KB ROM  |
| Falcon030         | 4.00     | --       | --       | --       | 4.00-4|04| 68030 + DSP56001          |
| Stacy (PDA)       | 1.04     | 1.04     | --       | --       | --     | Palm-sized, 1.04         |
| ST-Book (notebook)| 2.08     | --       | 2.08     | --       | --     | Notebook utilities        |
| Milan             | 4.08     | --       | --       | --       | 4.08   | 68040/60, Milan Computers|
+-------------------+----------+----------+----------+----------+--------+---------------------------+
```

### Model-Specific RAM Configurations

```
+------------------+------+-------+-------+-------+---------+
| Model            | Base | 256K  | 512K  | 1MB   | 2MB+    |
+------------------+------+-------+-------+-------+---------+
| 260ST            | --   | Yes   | --    | --    | --      | (disk boot)
| 520ST / 520STF   | 512K | --    | Yes   | --    | --      |
| 1040ST / 1040STF | 1MB  | --    | --    | Yes   | --      |
| 1040STFM         | 1MB  | --    | --    | Yes   | --      |
| Mega 1/ST        | 1MB  | --    | --    | Yes   | --      |
| Mega 2/STF       | 1MB  | --    | --    | Yes   | --      |
| Mega STE         | 1MB  | --    | --    | Yes   | Yes (up to 4MB) |
| TT030            | 1MB  | --    | --    | Yes   | Yes (up to 16MB fast) |
| Falcon030        | 4MB  | --    | --    | Yes   | Yes (up to 32MB) |
| STe (prototype)  | 512K | --    | Yes   | --    | --      |
| 520STE           | 1MB  | --    | --    | Yes   | Yes (up to 4MB) |
| 1040STE          | 1MB  | --    | --    | Yes   | Yes (up to 4MB) |
+------------------+------+-------+-------+-------+---------+
```

---

## ROM Checksum Verification Process

### ROM Content Checksum (TOS 1.x-4.x)

At power-on reset, the 68000 begins execution at `$FFFFFC00` (reset vector). The ROM initialization routine performs checksum verification:

```
ROM CHECKSUM (TOS 1.x-2.x):
-----------------------------
1. ROM image starts at $FFFF0000
2. The checksum word is stored at offsets $7D8E-$7D8F (end of valid ROM code)
3. Algorithm:
   
   a := 0
   for word_offset from $0000 to $7D8C step 2:
       a := (a + ROM[word_offset]) mod 65536
   
   expected := ((a + checksum_word) mod 65536)
   if expected != $B4A9:
       error: ROM checksum failure
   
4. On failure: TOS displays "Row of bombs" (TOS 1.0) or "Row of mushroom clouds" (TOS 1.0/very early)
   -- the number of bombs indicates the specific error code
```

### Boot Disk Checksum (All TOS Versions)

```
BOOT DISK CHECKSUM:
--------------------
1. Read first sector (256 words = 512 bytes) to buffer at $200
2. Calculate checksum:
   
   a := 0
   for i from 0 to 255:
       a := (a + WORD[$200 + i*2]) mod 65536
   
   if ((a + WORD[$200 + 254]) mod 65536) != $1234:
       error: invalid boot disk (first "bomb" error)
   
3. On success: entry point at $200 is jumped to (GEMDOS bootstrap)
```

### Memory Test Checksum (TOS 2.06+)

```
MEMORY TEST ALGORITHM (phystop detection):
-------------------------------------------
movea.l #phystop, a0       ; start at top of known memory
moveq #0, d0               ; test pattern value
chkmem8:
    movea.l a0, a1          ; temp address
    move.w #$fb55, d2       ; test pattern
    moveq #$2a, d1           ; iterate 43 times
chkmem9:
    move.w d2, -(a1)        ; write pattern
    add.w #$fb55, d2        ; advance pattern
    dbra d1, chkmem9
    
    movea.l a0, a1
    moveq #$2a, d1
chkmem10:
    cmp.w -(a1), d0         ; compare read with expected
    bne.s chkmemx           ; mismatch => bus error area
    clr.w (a1)              ; erase after test
    add.w #$fb55, d0
    dbra d1, chkmem10
    
    adda.l #$20000, a0     ; advance 128KB page
    bra chkmem8
    
chkmemx:
    suba.l #$20000, a0     ; backing off
    move.l a0, phystop      ; store detected physical memory end
```

---

## BIOS Calls Added Per TOS Version

### BIOS Functions (Trap #3)

All TOS versions share these core BIOS functions:

| BIOS # | Name | TOS Availability | Purpose |
|--------|------|------------------|---------|
| 0 | `Bconin` | All | Read character from device |
| 1 | `Bconout` | All | Write character to device |
| 2 | `Bconstat` | All | Get device input status |
| 3 | `Bcostat` | All | Get output device status |
| 4 | `Getbpb` | All | Get BIOS parameter block address |
| 5 | `Setexc` | All | Set/Read exception vector |
| 6 | `Stkchk` | All | Stack overflow check |
| 7 | `Freemem` | All | Get free memory |
| 8 | `Allocmem` | All | Allocate memory |
| 9 | `Swapvbl` | All | Swap VBL list |
| 10 | `Swtchv` | All | Switch virtual device driver |
| 11 | `Load` | All | Load program |
| 12 | `Open` | All | Open file |
| 13 | `Close` | All | Close file |
| 14 | `Read` | All | Read file |
| 15 | `Write` | All | Write file |
| 16 | `Ffindfirst` | All | Find first matching file |
| 17 | `Ffindnext` | All | Find next matching file |
| 18 | `Fcreate` | All | Create file |
| 19 | `Fremove` | All | Remove file |
| 20 | `Frename` | All | Rename file |
| 21 | `Chdir` | All | Change directory |
| 22 | `Gettatr` | All | Get file attributes |
| 23 | `Settatr` | All | Set file attributes |
| 24 | `Falloc` | All | Allocate file space |
| 25 | `Cdinfo` | All | Get drive change info |
| 26 | `MkDir` | All | Create directory |
| 27 | `RmDir` | All | Remove directory |
| 28 | `Gdrvnfo` | All | Get drive info |
| 29 | `Setver` | All | Set version number |
| 30 | `Ffirst` | All | Find first file |
| 31 | `Fnext` | All | Find next file |
| 32 | `Pfindfirst` | All | Process find first |
| 33 | `Pfindnext` | All | Process find next |
| 34 | `Exit` / Term | All | Terminate process |
| 35 | `Pterm0` | All | Terminate process 0 |
| 36 | `Pterm16` | All | Terminate process 16 |
| 37 | `Coldboot` | All | Cold boot |
| 38 | `Warmboot` | All | Warm boot |
| 39 | `Dbmsg` | TOS 1.04+ | Debug message output |
| 40 | `Gterm` | All | GEM termination |
| 41 | `Gopen` | All | Open GEM virtual screen |
| 42 | `Gclose` | All | Close GEM virtual screen |
| 43 | `Gread` | All | Read GEM virtual screen |
| 44 | `Gwrite` | All | Write GEM virtual screen |
| 45 | `Gsetbase` | TOS 1.04+ | Set GEM screen base |
| 46 | `Gsetport` | TOS 1.04+ | Set GEM screen port |
| 47 | `Ggetbase` | TOS 1.04+ | Get GEM screen base |
| 48 | `Ggetport` | TOS 1.04+ | Get GEM screen port |
| 49 | `Getinfo` | TOS 2.06+ | Get TOS/GEM info block address |

### BIOS Calls by TOS Version Family

```
BIOS CALLS BY TOS FAMILY:
========================

TOS 1.0 / 1.02 (Base ST):
  - Core 39 BIOS functions (0-38)
  - No GEM virtual screen I/O (41-44), added in 1.04
  - No Getinfo (49)

TOS 1.04 (Rainbow TOS):
  - + BIOS 39: Dbmsg
  - + BIOS 45-48: GEM virtual screen base/port functions
  - Modified FAT12 format (DOS-compatible)

TOS 1.06 / TOS 2.06 (STE):
  - + BIOS 49: Getinfo (extended system info block)
  - Enhanced disk I/O for 1.44MB formatting

TOS 3.0x (TT):
  - Core 50 BIOS functions maintained for ST compatibility
  - TT-specific drivers loaded separately

TOS 4.0x (Falcon):
  - All 50 + BIOS functions
  - MultiTOS: additional interrupt and process management
  - DSP56001 driver loaded as separate BIOS module
```

---

## Hardware Feature Detection Memory Map

### Memory Layout for Feature Detection

```
+===============================================================================+
|  ADDRESS RANGE         |  SIZE     |  CONTENT                                |
+========================+===========+=========================================+
| $000000 - $0003FF     |  1KB      |  Exception vectors (128 entries x 4B)     |
|                         |           |  Reset vector at $000000 -> $FFFFFC00     |
+------------------------+-----------+-----------------------------------------+
| $000400 - $0019FF     |  6KB      |  Stack area (user mode)                   |
+------------------------+-----------+-----------------------------------------+
| $001A00 - $001FFF     |  2KB      |  TOS working storage                      |
+------------------------+-----------+-----------------------------------------+
| $002000               |  var.     |  Boot disk buffer (512B)                  |
|                       |           |  (loaded during boot checksum)            |
+------------------------+-----------+-----------------------------------------+
| $800000 - $ffffff      |  var.     |  Standard RAM (phystop determines actual  |
|                       |           |  end of usable memory from memory test)   |
+------------------------+-----------+-----------------------------------------+
| FFFF8000 - FFFF81FF   |  512B     |  ST Configuration & system variables      |
+========================+===========+=========================================+
| $FFFF8000             |  byte     |  ROM ID (always $55)                      |
| $FFFF8001             |  byte     |  ROM configuration byte                    |
| $FFFF8003             |  word     |  TOS version number                         |
|                         |           |  $0100 = TOS 1.0, $0104 = TOS 1.04       |
|                         |           |  $0106 = TOS 1.06, $0206 = TOS 2.06      |
|                         |           |  $0306 = TOS 3.06, $0404 = TOS 4.04      |
| $FFFF800A             |  byte     |  ROM config: region, RAM, MFP flags       |
| $FFFF8029             |  word     |  TOS ROM size in KB                         |
| $FFFF8100             |  word     |  AES base address                          |
| $FFFF8102             |  word     |  AES vector table address                   |
| $FFFF8104             |  byte     |  ** MACHINE TYPE REG **                    |
|                         |           |  Bit 0 = 1: 68000 CPU family              |
|                         |           |  Bit 1 = 1: STE shifter present           |
|                         |           |  Bit 2-3 = machine type:                   |
|                         |           |    00 = ST/520ST    01 = STF              |
|                         |           |    10 = STe       11 = MegaSTE             |
|                         |           |  Bit 7 = 1: PAL region                    |
| $FFFF8105             |  byte     |  Mouse present flag                         |
| $FFFF8107             |  word     |  Memory management flags                    |
| $FFFF8109             |  word     |  MFP device register address               |
| $FFFF810B             |  word     |  MFP interrupt mask                         |
| $FFFF810D             |  long     |  MFP interrupt vector                       |
| $FFFF833C             |  long     |  Cookie jar pointer (TOS 1.06+)            |
| $FFFF8346             |  long     |  PHYSBASE (physical screen base)           |
| $FFFF834A             |  long     |  LOGBASE (logical screen base)             |
| $FFFF8382             |  long     |  PHYSTOP (end of physical memory)          |
| $FFFF8386             |  long     |  LOGTOP (end of logical screen)            |
+------------------------+-----------+-----------------------------------------+
| FFFF8200 - FFFF83FF   |  512B     |  System variables & XBIOS table            |
|                         |           |  XBIOS address table at $FFFF83E2          |
+========================+===========+=========================================+
| FFFF8400 - FFFF8FFF   |  24KB     |  Reserved/BIOS data area                   |
+========================+===========+=========================================+
| FFFF9000 - FFFF9FFF   |  4KB      |  Mouse data area                           |
+------------------------+-----------+-----------------------------------------+
| FFFFA000 - FFFFF7FF   |  28KB     |  GEM workspace, scratch pad                |
+------------------------+-----------+-----------------------------------------+
| FFFF9000 - FFFFF7FF   |  var.     |  TOS ROM code (6-chip: $FFFF0000-F8FF)   |
|                         |           |  TOS ROM code (2-chip STE: $FFFF0000-FFFF) |
+------------------------+-----------+-----------------------------------------+
| FFFFF800 - FFFFFBFF   |  1KB      |  ROM config and exception vectors         |
| FFFFFC00 - FFFFFDFF   |  512B     |  Exception vectors (128 x word)            |
|                         |           |  $FFFFFC00 = reset vector -> $FFFFFC02   |
| FFFFFE00 - FFFFFEFF   |  512B     |  Checksum zone (ROM checksum verified)     |
| FFFFFEF0 - FFFFFEFF   |  word     |  ROM checksum word at $FFFFFEF8            |
| FFFFFEF4 - FFFFFEFF   |  word     |  ROM checksum at $FFFFFE3A                 |
+------------------------+-----------+-----------------------------------------+
| FFFFFF00 - FFFFFFDF   |  224B     |  VECTAB vector table                         |
| $FFFFFC00             |  long     |  Reset vector -> $00000000                 |
| $FFFFFC04             |  long     |  Bus error vector                          |
| $FFFFFC08             |  long     |  Address error vector                      |
| $FFFFFC82             |  long     |  trap #1 vector (XBIOS dispatch)           |
| $FFFFFC86             |  long     |  trap #10 vector (BIOS dispatch)           |
| $FFFFFC8A             |  long     |  trap #14 vector (AES dispatch)            |
| $FFFFFC0E             |  long     |  TRAPA 0 vector (VDI dispatch)             |
+------------------------+-----------+-----------------------------------------+

+===============================================================================+
|  HARDWARE REGISTERS (Memory-Mapped I/O)                                       |
+========================+===========+=========================================+
| $FF8000 - $FF81FF     |  512B     |  Blitter registers                       |
| $FF8200               |  long     |  Blitter control/status + A reg          |
| $FF8204               |  long     |  Blitter B register                        |
| $FF8208               |  long     |  Blitter C register                        |
| $FF820C               |  long     |  Blitter X/Y register                      |
+------------------------+-----------+-----------------------------------------+
| $FF8800 - $FF880B   |  12B      |  STE DMA Sound control registers           |
| $FF8800               |  byte     |  Sound DMA control (bit 7=enable,       |
|                         |           |  bit 6=stereo/mono)                       |
| $FF8801               |  byte     |  Sample frequency (6.25/12.5/25/50 kHz)  |
| $FF8804               |  long     |  Frame start address                       |
| $FF8808               |  long     |  Frame end address                         |
+------------------------+-----------+-----------------------------------------+
| $FF8C00 - $FF8C0F   |  16B      |  STE Blitter copy mode registers           |
| $FF8C00               |  long     |  Copy source address                       |
| $FF8C04               |  long     |  Copy dest address                         |
| $FF8C08               |  byte     |  Copy length (bytes)                       |
| $FF8C09               |  byte     |  Copy pattern                              |
| $FF8C0A               |  word     |  Copy mode selector                        |
| $FF8C0C               |  long     |  Copy line length                          |
+------------------------+-----------+-----------------------------------------+
| $FF8D00 - $FF8DFF   |  256B     |  STe shifter extended registers            |
| $FF8D00               |  byte     |  Shifter configuration                     |
+------------------------+-----------+-----------------------------------------+
| $FF8000 - $FFFF7FFF  |  var.     |  VDP (video display processor)            |
| $FF8200               |  word     |  VDP data register                         |
| $FF8202               |  word     |  VDP address register                        |
| $FF8204               |  word     |  VDP command register                        |
+------------------------+-----------+-----------------------------------------+
| $FFC000 - $FFC1FF   |  512B     |  FPU (if installed, STe/TT)               |
+------------------------+-----------+-----------------------------------------+
| $FFC200 - $FFC2FF   |  256B     |  SCC (Serial Communications Controller)   |
+------------------------+-----------+-----------------------------------------+
| $FFC300 - $FFC3FF   |  256B     |  MFP (Multi-function Peripheral)          |
| $FFC200               |  byte     |  MFP data register A                       |
| $FFC201               |  byte     |  MFP data register B                       |
| $FFC202               |  byte     |  MFP direction register A                  |
| $FFC203               |  byte     |  MFP direction register B                  |
| $FFC204               |  byte     |  MFP interrupt vector                      |
| $FFC205               |  byte     |  MFP interrupt vector modifiable           |
| $FFC206               |  byte     |  MFP interrupt vector mod                  |
| $FFC207               |  byte     |  MFP interrupt acknowledge                 |
| $FFC208               |  word     |  MFP level 2 vector                        |
| $FFC20A               |  word     |  MFP interrupt request (A)                 |
| $FFC20C               |  word     |  MFP interrupt mask (A)                    |
| $FFC20E               |  word     |  MFP interrupt request (B)                 |
| $FFC210               |  word     |  MFP interrupt mask (B)                    |
+------------------------+-----------+-----------------------------------------+
| $FF8000 - $FFFF7F   |  var.     |  Shifter (ST variant) / STE extended VDP  |
| $FFFC00               |  word     |  Shifter revision register                 |
| $FFFC03               |  byte     |  Shifter ID / revision                     |
+------------------------+-----------+-----------------------------------------+
| $FFFF8740           |  byte     |  STE NVRAM / Super CMOS (TOS 2.x+)        |
| $FFFF8743           |  byte     |  Super CMOS config                         |
+------------------------+-----------+-----------------------------------------+
```

### Cookie Jar System Variables

Introduced in TOS 1.06, the cookie jar allows runtime feature detection without version checks:

```
COOKIE JAR FORMAT (walk from end of RAM downward):
+--------------------------------------------------+
| Cookie ID (4 bytes)  | Tag (e.g., 'TOS1')       |
| Cookie Length (4B)   | Value count               |
+--------------------------------------------------+
... (null cookie at end)                          |
| 0x00000000         | Total entries              |
+--------------------------------------------------+

Standard Cookie IDs:
  'TOS1' = TOS version (same as at $FFFF8003)
  'GEM2' = GEM version
  'GEM3' = AES version
  'STe1' = STe present (value = 1 if STe shifter)
  'MEGA' = MegaSTE present (value = 1)
  'TT00' = TT present
  'FCON' = Falcon030 present
  'C24P' = 24-bit color patch
  'BHLP' = BubbleGEM config
  'MC00' = MFP chip present
```

### TOS Version / Atari Model Mapping

```
TOS VERSION / ATARI MODEL MAPPING:
===================================

TOS 1.00  -->  520ST, 1040ST         [2-chip/6-chip ROM, 192KB]
TOS 1.02  -->  520ST, 1040ST, Mega1, Mega2  [6-chip ROM, Blitter+RTC]
TOS 1.04  -->  520ST, 1040ST, Mega1, Mega2,      [6-chip ROM, Rainbow TOS]
                         Stacy, 1040STF, Mega STF
TOS 1.06  -->  520STE, 1040STE            [2-chip ROM, 256KB]
TOS 1.62  -->  520STE, 1040STE            [2-chip ROM, bug fixes]
TOS 2.02  -->  MegaSTE                    [2-chip ROM, early MegaSTE]
TOS 2.05  -->  MegaSTE                    [2-chip ROM, 720K floppy]
TOS 2.06  -->  520ST, 1040ST, 520STE,     [2-chip ROM, last ST/STE TOS]
                         1040STE
TOS 2.07  -->  Falcon "FX-1" prototype     [Falcon prototype TOS]
TOS 2.08  -->  ST-Book notebook            [Notebook utilities]
TOS 3.01  -->  TT030                      [4-chip ROM, 512KB]
TOS 3.05  -->  TT030                      [4-chip ROM, memory fixes]
TOS 3.06  -->  TT030                      [4-chip ROM, final TT TOS]
TOS 4.00  -->  Falcon030                  [4-chip ROM, MultiTOS]
TOS 4.01  -->  Falcon030                  [4-chip ROM]
TOS 4.02  -->  Falcon030                  [4-chip ROM]
TOS 4.04  -->  Falcon030                  [4-chip ROM, last official TOS]
TOS 4.08  -->  Milan                       [4-chip ROM, 68040 Milan]
TOS 4.92  -->  (prototype, leaked)        [MultiTOS, never official]
```

## BIOS Call Summary Diagram

```
BIOS CALLS BY TOS VERSION (Trap #10):
======================================

                    TOS1.0  TOS1.04  TOS2.06  TOS3.06  TOS4.04
                    ------  -------  -------  -------  -------
BIOS 0  Bconin      Y       Y        Y        Y        Y
BIOS 1  Bconout     Y       Y        Y        Y        Y
BIOS 2  Bconstat    Y       Y        Y        Y        Y
BIOS 3  Bcostat     Y       Y        Y        Y        Y
BIOS 4  Getbpb      Y       Y        Y        Y        Y
BIOS 5  Setexc      Y       Y        Y        Y        Y
BIOS 6  Stkchk      Y       Y        Y        Y        Y
BIOS 7  Freemem     Y       Y        Y        Y        Y
BIOS 8  Allocmem    Y       Y        Y        Y        Y
BIOS 9  Swapvbl     Y       Y        Y        Y        Y
BIOS 10 Swtchv      Y       Y        Y        Y        Y
BIOS 11 Load        Y       Y        Y        Y        Y
BIOS 12 Open        Y       Y        Y        Y        Y
BIOS 13 Close       Y       Y        Y        Y        Y
BIOS 14 Read        Y       Y        Y        Y        Y
BIOS 15 Write       Y       Y        Y        Y        Y
BIOS 16 Ffindfirst  Y       Y        Y        Y        Y
BIOS 17 Ffindnext   Y       Y        Y        Y        Y
BIOS 18 Fcreate     Y       Y        Y        Y        Y
BIOS 19 Fremove     Y       Y        Y        Y        Y
BIOS 20 Frename     Y       Y        Y        Y        Y
BIOS 21 Chdir       Y       Y        Y        Y        Y
BIOS 22 Gettatr     Y       Y        Y        Y        Y
BIOS 23 Settatr     Y       Y        Y        Y        Y
BIOS 24 Falloc      Y       Y        Y        Y        Y
BIOS 25 Cdinfo      Y       Y        Y        Y        Y
BIOS 26 MkDir       Y       Y        Y        Y        Y
BIOS 27 RmDir       Y       Y        Y        Y        Y
BIOS 28 Gdrvnfo     Y       Y        Y        Y        Y
BIOS 29 Setver      Y       Y        Y        Y        Y
BIOS 30 Ffirst      Y       Y        Y        Y        Y
BIOS 31 Fnext       Y       Y        Y        Y        Y
BIOS 32 Pfindfirst  Y       Y        Y        Y        Y
BIOS 33 Pfindnext   Y       Y        Y        Y        Y
BIOS 34 Exit        Y       Y        Y        Y        Y
BIOS 35 Pterm0      Y       Y        Y        Y        Y
BIOS 36 Pterm16     Y       Y        Y        Y        Y
BIOS 37 Coldboot    Y       Y        Y        Y        Y
BIOS 38 Warmboot    Y       Y        Y        Y        Y
BIOS 39 Dbmsg       N       Y        Y        Y        Y
BIOS 40 Gterm       Y       Y        Y        Y        Y
BIOS 41 Gopen       N       Y        Y        Y        Y
BIOS 42 Gclose      N       Y        Y        Y        Y
BIOS 43 Gread       N       Y        Y        Y        Y
BIOS 44 Gwrite      N       Y        Y        Y        Y
BIOS 45 Gsetbase    N       Y        Y        Y        Y
BIOS 46 Gsetport    N       Y        Y        Y        Y
BIOS 47 Ggetbase    N       Y        Y        Y        Y
BIOS 48 Ggetport    N       Y        Y        Y        Y
BIOS 49 Getinfo     N       N        Y        Y        Y
BIOS 50+            (TT/Falcon extensions vary)
```

## XBIOS Call Summary by TOS Version (Trap #14)

```
XBIOS CALLS BY TOS VERSION:
==========================

                    TOS1.0  TOS1.04  TOS1.06  TOS2.06  TOS3.06  TOS4.04
                    ------  -------  -------  -------  -------  -------
XBIOS 0   Initmouse Y       Y        Y        Y        Y        Y
XBIOS 2   Physbase  Y       Y        Y        Y        Y        Y
XBIOS 3   Logbase   Y       Y        Y        Y        Y        Y
XBIOS 4   Getrez    Y       Y        Y        Y        Y        Y
XBIOS 5   Setscreen Y       Y        Y        Y        Y        Y
XBIOS 6   Setpalette Y      Y        Y        Y        Y        Y
XBIOS 7   Setcolor  Y       Y        Y        Y        Y        Y
XBIOS 8   Floprd    Y       Y        Y        Y        Y        Y
XBIOS 9   Flopwr    Y       Y        Y        Y        Y        Y
XBIOS 10  Flopfmt   Y       Y        Y        Y        Y        Y
XBIOS 11  Dbmsg     Y       Y        Y        Y        Y        Y
XBIOS 12  Midiws    Y       Y        Y        Y        Y        Y
XBIOS 13  Mfpint    Y       Y        Y        Y        Y        Y
XBIOS 14  Iorec     Y       Y        Y        Y        Y        Y
XBIOS 15  Rsconf    Y       Y        Y        Y        Y        Y
XBIOS 16  Keytbl    Y       Y        Y        Y        Y        Y
XBIOS 17  Random    Y       Y        Y        Y        Y        Y
XBIOS 18  Protobt   Y       Y        Y        Y        Y        Y
XBIOS 19  Flopver   Y       Y        Y        Y        Y        Y
XBIOS 20  Scrdmp    Y       Y        Y        Y        Y        Y
XBIOS 21  Cursconf  Y       Y        Y        Y        Y        Y
XBIOS 22  Settime   Y       Y        Y        Y        Y        Y
XBIOS 23  Gettime   Y       Y        Y        Y        Y        Y
XBIOS 25  Ikbdws    Y       Y        Y        Y        Y        Y
XBIOS 26  Jdisint   Y       Y        Y        Y        Y        Y
XBIOS 27  Jenabint  Y       Y        Y        Y        Y        Y
XBIOS 28  Giaccess  Y       Y        Y        Y        Y        Y
XBIOS 29  Offgibit  Y       Y        Y        Y        Y        Y
XBIOS 30  Ongibit   Y       Y        Y        Y        Y        Y
XBIOS 31  Xbtimer   Y       Y        Y        Y        Y        Y
XBIOS 32  Dosound   Y       Y        Y        Y        Y        Y
XBIOS 38  Supexec   Y       Y        Y        Y        Y        Y
XBIOS 40  Blitmode  Y       Y        Y        Y        Y        Y
XBIOS 42  DMAread   Y       Y        Y        Y        Y        Y
XBIOS 43  DMAwrite  Y       Y        Y        Y        Y        Y
XBIOS 44  Bconmap   N       Y        Y        Y        Y        Y
XBIOS 48  Floprate  N       Y        Y        Y        Y        Y    [1.04+]
XBIOS 64  EsetShift Y       N        Y        Y        Y        Y    [TOS 1.06+/STE]
XBIOS 65  EgetShift Y       N        Y        Y        Y        Y    [TOS 1.06+/STE]
XBIOS 66  EsetBank  Y       N        Y        Y        Y        Y    [TOS 1.06+/STE]
XBIOS 67  EsetColor Y       N        Y        Y        Y        Y    [24-bit color]
XBIOS 68  EsetPalette Y     N        Y        Y        Y        Y    [TT palette]
XBIOS 69  EgetPalette Y     N        Y        Y        Y        Y
XBIOS 70  EsetGray  Y       N        Y        Y        Y        Y
XBIOS 71  EsetSmear Y       N        Y        Y        Y        Y
XBIOS 88  VsetMode  Y       Y        Y        Y        Y        Y    [Falcon only]
XBIOS 96-127  DSP*  N       N        N        N        N        Y    [Falcon DSP]
XBIOS 130 soundcmd Y      Y        Y        Y        Y        Y    [Falcon sound]
XBIOS 150 VsetMask  Y       Y        Y        Y        Y        Y
```

## Hardware Detection Flowchart

```
POWER ON / RESET
    |
    v
[Vector $FFFFFC00] -> Execute from ROM at $FFFF0000
    |
    v
[Step 1] Read config at $FFFF8104 (byte)
    |-- Bit 7 = 1 (PAL) or 0 (NTSC)
    |-- Bits 2-3 = machine type (00=ST, 01=STF, 10=STE, 11=MegaSTE)
    |-- Bit 1 = 1 if STE shifter present
    v
[Step 2] Validate ROM checksum
    |-- Sum all ROM words $0000 to $7D8C
    |-- Add checksum word at $7D8E
    |-- Result must equal $B4A9
    |-- FAIL => Display "Row of bombs" error (TOS 1.0)
    |
    v
[Step 3] Read RAM size from $FFFF8202
    |
    v
[Step 4] Run memory test (TOS 2.06+)
    |-- Write/$FB55 pattern, 43 words, 128KB pages
    |-- Bus error => end of physical memory
    |-- Store result in `phystop`
    |
    v
[Step 5] Identify video hardware
    |-- Read shifter revision at $FFFC03
    |-- ST = GCW/MDEN variant (40-col mono)
    |-- STE = Super Hi-Res variant (multi-mode)
    |-- TT = dedicated TT video chip
    |
    v
[Step 6] Check for cookie jar (TOS 1.06+)
    |-- Read $FFFF833C (cookie jar pointer)
    |-- Walk cookie entries until NULL (0x00000000)
    |-- Use cookies: 'TOS1', 'STe1', 'MEGA', 'TT00', 'FCON'
    |
    v
[Step 7] Initialize hardware
    |-- MFP interrupt vectors
    |-- Blitter registers
    |-- VDP registers
    |-- Sound DMA (if STE present)
    |
    v
[Step 8] Load boot disk
    |-- Read sector 0 to buffer $200
    |-- Verify checksum: sum all 256 words + last word = $1234
    |-- Jump to entry point at $200
    |
    v
[Step 9] GEM Desktop loads
    |-- AES initialization
    |-- VDI driver loading
    |-- GDOS initialization
    |-- Read DESKTOP.INF or NEWDESK.INF
    |
    v
READY
```

---

## References

- Atari ST Internals (Atari Corporation)
- The Atari Compendium (Fritze)
- TOS hyp documentation (https://freemint.github.io/tos.hyp/)
- Town's Little Guide to TOS Revisions (http://www.atari.st/content.php?type=t&file=toslist)
- Atari ST/STe/MSTe/TT/F030 Hardware Register Listing (Atari-Forum)
- Wikipedia: Atari TOS
- Brian Awe: Atari TOS Versions (https://www.briandawe.co.uk/atari_tos_versions.html)
