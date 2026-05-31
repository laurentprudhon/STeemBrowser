# Atari ST Emulation Technical Solutions

This document covers the technical implementation of Atari ST emulators and FPGA cores, not the hardware itself. For hardware details see `atari-st-hardware/`.

## Table of Contents

- [Core CPU Emulation](#core-cpu-emulation)
- [Cycle-Accurate Timing](#cycle-accurate-timing)
- [Hardware Chipset Emulation](#hardware-chipset-emulation)
- [Video and Display](#video-and-display)
- [Sound Emulation](#sound-emulation)
- [File System and Disk Emulation](#file-system-and-disk-emulation)
- [MultiTOS and Guest OS Integration](#multitos-and-guest-os-integration)
- [Popular Emulators and Cores](#popular-emulators-and-cores)

## Core CPU Emulation

### 68000 CPU Emulation

The Atari ST uses a Motorola 68000 at 8 MHz. Emulators implement the 68K CPU through different approaches:

- **Interpreted instruction decoding**: Each 68K instruction is decoded and emulated in software. This is the simplest approach but results in lower performance. Instruction timing varies (typically 1-44 cycles depending on opcode and addressing mode).
- **Trace table (lookup table) approach**: Popularized by WinUAE, this approach uses pre-computed trace tables that map each opcode to an array of handler entry points for each combination of register values used as the effective address source. A bitmask derived from the register (e.g., `(Dn >> 12) & 0x0E`) indexes into the lookup table, allowing fast dispatch to the correct handler. This is significantly faster than interpreted decoding because the dispatch is a direct table lookup.
- **Dynamic translation (JIT)**: Translates blocks of 68K machine code into host machine code (x86, ARM) at runtime. This can achieve near-native performance but is complex to implement, especially with the 68K's variable-length instructions (2-16 bytes) and mode-dependent behavior.
- **Binary rewriting**: Replaces 68K instructions with equivalent x86/ARM instructions on the fly, maintaining a translation cache.

Hatari originally used a custom interpreted/traced 68K core. Starting with **Hatari 2.0.0**, it adopted the **WinUAE 68000 CPU core**, which uses the trace table approach with enhanced cycle accuracy: correct IACK sequence, exception/interrupt stacking, 2-cycle access timing, bus access timing depending on memory region, instruction pairing, and proper prefetch pipeline behavior.

### 68030 CPU Emulation (TT / Falcon)

The Atari TT and Falcon030 use a 68030 (some Falcon models use 68030 without FPU). The 68030 adds:

- **Integrated MMU** (manages virtual memory, access permissions, and page tables)
- **Integrated instruction and data caches** (4 KB each for 68030)
- **Faster bus interface** (32-bit internal bus)

Hatari 2.0.0+ implements the 68030 using the WinUAE CPU core with:
- Full MMU emulation with page table walk simulation
- Data and instruction cache with proper invalidation handling
- Bus error retrying for programs relying on MMU behavior
- Cycle-accurate prefetch pipeline and IACK sequence
- 2-cycle memory access timing
- Correct handling of bus accesses depending on memory region

Hatari 2.5.0+ enhanced 68030 emulation with improved prefetch, cache, and MMU behavior, fixing programs that rely on MMU bus-error retrying patterns.

### Falcon DSP (Motorola 56001)

The Falcon030 includes a **DSP56001** 24-bit DSP chip alongside the 68030. DSP emulation requires:

- **DSP instruction set**: 24-bit MAC (multiply-accumulate) operations, specialized addressing modes
- **Dual bus architecture**: Program and data buses operate independently, enabling parallel fetch and execute
- **Hardware multipliers**: 16x16 multiply with 24-bit accumulator
- **Circular addressing buffers** for audio buffer management

Hatari 2.1.0 added initial Falcon DSP emulation. Hatari 2.5.0 further improved DSP cycle accuracy. The DSP handles:
- 44.1 kHz stereo digital audio (CD-quality)
- Sample-rate conversion
- Internal FIFO buffers for DMA sound

The DSP shares memory with the 68030 through a shared memory interface. DSP operations are interleaved with CPU cycles, requiring accurate cycle counting to ensure correct timing of DMA transfers between DSP and system RAM.

## Cycle-Accurate Timing

### The Interleaving Problem

The Atari ST CPU and video display controller share the same 8 MHz system bus and RAM. They alternate access: typically **2 CPU cycles, then 2 video cycles** (a "stall"). This means:

- The effective CPU speed is about 4 MHz (half the nominal 8 MHz)
- Accessing memory during the horizontal blanking period (line front porch) gives the CPU extra access cycles — a technique used by many games and demos to boost performance
- DMA blitter operations also steal bus cycles

Emulators must track the video counter position within the current line and frame to correctly implement this interleaving. The key timing values are:

- **Horizontal count**: 288 lines per frame (224 visible + 64 blank + 2 front porch + 2 back porch in standard 31.25 kHz)
- **Vertical count**: 312 frames per second (NTSC) or 288 frames per second (PAL at 50 Hz)
- **Video clock**: 16 MHz (4x the CPU clock) — the video counter increments every 2 CPU cycles in normal regions
- **Front porch**: Lines 0-1 in horizontal blanking give CPU extra memory access (2 extra cycles per line)

### Bus Cycle Timing

Different memory regions have different timing characteristics:

| Region | Address Range | Timing |
|--------|---------------|--------|
| Chip RAM | `$000000` - `$1FFFFF` (640 KB) | CPU and video alternate access (1 cycle each in normal regions) |
| Atari header | `$00F000` | Read-only, used for cookie jar and system variables |
| GEMDOS/BIOS ROM | `$FFFF8000` - `$FFFFFFFC` | Read-only, CPU accesses without video interference |
| XBIOS entry vector | `$FFFC38` | TRAP #2 vector (0:0x4EF9 FF0002) |
| MFP registers | `$FFFC00` - `$FFFC3F` | 64 bytes, I/O mapped |
| ST-DMA registers | `$FFFA00` - | DMA control registers (STE+) |

Hatari's memory access timing tracks:
- Current video counter position (horizontal and vertical)
- Current bus phase (CPU vs video access)
- Memory region type (chip RAM, ROM, I/O)
- Number of cycles spent in the current phase

### MFP Timer Interrupts

The Motorola 6821/68D30 MFP (Multi-Function Peripheral) generates timer interrupts at specific cycle counts:

- **Timer A** and **Timer B**: Programmable dividers from the 2.4576 MHz clock (divider by 4 prescaler gives 614.4 kHz effective clock). Timers decrement every N input clock pulses and generate a pulse on the terminal count.
- **IKBD interrupt**: Keyboard/controller interrupts from the MC6847-dependent controller, clocked at 3.546875 MHz
- **RTC interrupt**: Real-time clock (1-255 Hz, selectable in TOS 3.x+)

Emulators implement MFP timers as software counters that decrement based on the accumulated CPU cycle count, firing interrupts at the correct times relative to the system clock. The cycle-accurate implementation tracks the exact number of input clock cycles between timer decrements.

### IKBD and IRQ Timing

The IKBD (Integrated Keyboard/ joystick Controller) chip processes:
- Keyboard scancodes at a fixed interrupt rate (~29.155 kHz)
- Joystick and mouse position sampling
- MIDI port data transfer via MFP

Emulators must generate IKBD interrupts at the correct intervals and process pending keyboard/joystick events. Hatari uses `set_ipl` (set interrupt priority level, adds no cycle cost) and `irc2ir` (IRQ conversion) functions to schedule interrupts with proper timing.

## Hardware Chipset Emulation

### Memory Controller (GCR / GAear / GTIA)

The original Atari ST uses three main custom chips (the "ST trilogy"):

1. **GC016000** (Graphics Controller): Memory controller and bus arbitration
2. **GC016001** (GTEAKB2 / GTIA): Video display generator, pixel processing, DAC control
3. **GC016002** (GD8464 variant): DMA controller, floppy interface

Emulators model these chips as a combined "chipset emulation layer" that sits between the CPU address bus and the memory/RAM subsystem:

- **Address decoding**: Routes reads/writes to appropriate regions (RAM, ROM, VDI registers, MFP, etc.)
- **Bus arbitration**: Manages CPU vs. video vs. DMA arbitration on the shared bus
- **DMA channel emulation**: Handles the ST-DMA controller (STE on) which includes FIFO buffers, channel control, and transfer counter registers

The ST-DMA controller in the STE adds:
- **Sound DMA**: 8-bit mono/stereo digital audio with sample-rate FIFO
- **Disk DMA**: DMA for floppy controller data transfer
- **FIFO buffers**: Smoothing out the sampled audio data before output to the DAC

### Floppy Controller (FDC)

The Atari ST uses a Western Digital WD1771/WD1772 floppy disk controller. Cycle-accurate FDC emulation:

- Implements the exact register interface (data, status, command, track, sector registers)
- Handles DMA-driven data transfer between FDC and RAM
- Emulates disk image formats (`.ST`, `.MSA`, `.CTR`, `.IMG`, IPF for copy protection)
- Supports low-level copy-protected disk access (required for games with sector-level protection)

**FX CAST core** (FPGA) includes cycle-accurate FDC that can load and run low-level copy-protected disk images, matching the original hardware behavior exactly.

### Blitter Emulation

The Atari ST Blitter (STE+) is a parallel processing unit for 2D graphics operations:

- **24 independent shift registers** (16 bits each) for source/destination data
- **24 pixel counters** for parallel pixel manipulation
- **4 operations**: AND, OR, XOR, move with shift capability
- **24-bit width** and **65536-bit length** per operation

Hatari implements blitter emulation by:
- Tracking blitter command register interface ($FF81x0)
- Emulating the shift register state machine
- Processing 24-pixel chunks per cycle with proper timing
- Handling blitter bus cycle stealing (blitter uses DMA to access chip RAM)

Blitter operations complete in a number of CPU cycles proportional to `(length + 31) / 24` (rows) times per-shift-cycle count, with bus accesses interleaved with CPU cycles.

### RTC (Real-Time Clock)

Mega STE/TT/Falcon include a real-time clock chip. Emulation:

- Tracks system time with calendar registers (seconds, minutes, hours, day, month, year)
- Generates periodic interrupts at configurable rates
- The Mega RTC chip is different from the Falcon/TT RTC and requires separate emulation logic
- Hatari emulates RTC register reads/writes and generates the appropriate interrupts

### SCSI Controller (TT/Falcon)

The Atari TT and Falcon include a NCR5380 SCSI controller:

- Emulates SCSI command processing (Mode Sense, Report Luns, etc.)
- Supports hard disk image files (`.hdc`, raw disk images)
- Handles SCSI command queuing and DMA transfer between SCSI buffer and system RAM
- Hatari 2.5.0 improved Mode Sense and Report Luns handling

### Serial Ports (ACIA / Serial)

Emulation of Atari serial/iChat ports:

- ACIA (MC6850) register interface at $FFFC30
- Baud rate generation from MFP timer
- XBIOS serial function codes (EXEC, GETMODE, etc.)
- MIDI DIN5 port emulation with real-time data transfer

## Video and Display

### TOS ROM Patching for GEM Compatibility

Atari TOS contains hardcoded framebuffer assumptions. Hatari patches the TOS ROM image at load time:

- Locates GEMDOS `VIA_VER` and `GEM_BAS` address tables in the ROM
- Patches the base address of the GEM workspace area to point to Hatari's virtual framebuffer
- Patches screen resolution, color palette, and video mode parameters to match the output display
- Patches the AES (Application Environment Manager) address to the virtual work area block at `AES_BAS + $8000`

This patching allows GEM applications written for fixed memory layouts to work correctly in the emulated environment without source code changes.

### Framebuffer Rendering Pipeline

Hatari renders the Atari ST display through this pipeline:

1. **Virtual framebuffer**: A linear memory buffer (256x512 pixels at 1-bit per pixel, or 640x480 at 4-bit) representing the Atari ST display memory
2. **Palette translation**: Emulates the Atari ST's 15-bit color DAC ($0-$15) to 24-bit RGB for display output
3. **Color depth modes**: Supports 16-color (default ST), 128-color (STE), 4096-color (STE HiColor), and 16.7M-color (Falcon TrueColor)
4. **SDL2 output**: Uses SDL2's rendering pipeline with optional hardware acceleration (`SDL_RENDERER_ACCELERATED`)
5. **Vertical blank interrupt (VBI)**: Hatari fires the VBI interrupt at exactly the right moment (every 1/50s PAL or 1/60s NTSC) to trigger TOS VBI chains and GEM desktop update

The framebuffer is updated through:
- **Direct memory writes** to video RAM ($A00000-$A7FFFF for ST, expanded for STE/Falcon)
- **VDI/GEMDOS draw calls** translated through the patched GEM workspace
- **Blitter bit-block transfers** (bitblt) to the framebuffer

### Palette and DAC Emulation

The Atari ST uses a 15-bit DAC (5 bits per RGB channel):
- Palette registers at $FF8800-$$FF883F (16 entries)
- Each entry: 5 bits red, 5 bits green, 5 bits blue = $0-$31 per channel
- Hatari stores the palette in memory and converts to SDL2 color indices when rendering

### High-Resolution Mode

Hatari supports expanded resolutions:
- **STE**: 640x480 at 4 bpp (16 colors), 320x480 at 4 bpp (4096 colors with banked memory)
- **Falcon**: 640x480 at 8 bpp (256 colors), 320x480 at 8 bpp (4096 colors), and TrueColor modes
- Bank switching handled through bank register interface

## Sound Emulation

### YM2149 PSG (ST/STe)

All ST models use the Yamaha YM2149 Programmable Sound Generator (or compatible AY-3-8910):

- **3 tone channels** with programmable frequency (16-bit divider) from the 2.4576 MHz source clock
- **1 noise generator** with programmable frequency (white noise only, 16-bit linear feedback shift register)
- **3 amplifier channels** with 16-step volume controls (25 steps for envelope)
- **Envelope generator**: 10 waveforms, 32-step envelope (sustain loop, attack/decay, continuous)
- **Buzzer mode**: Direct control of output pins

YM2149 emulation in Hatari tracks:
- Per-channel tone period registers ($0-$1) and volume registers ($8-$A)
- Noise generator period register ($3$)
- Mixer register ($7$): which channels are enabled for each output
- Envelope generator counters and waveform tables
- Clock-driven sample generation at chip rate (internal step = clk/8)

### ST-DMA Sound (STE+)

The STE/STe/Falcon add digital audio input through ST-DMA:
- 8-bit mono/stereo digital audio at programmable sample rates
- DMA channel with FIFO buffer for audio data
- Sample data written to a RAM buffer via DMA, read by a software mixer
- Hatari implements DMA FIFO for accurate audio timing

### Falcon DSP Audio

The Falcon DSP56001 handles all audio processing:
- 44.1 kHz stereo digital audio at CD quality
- Hardware sample-rate converter
- Internal audio FIFO for smooth playback
- DSP program loading via XBIOS `DSP_LOAD` function calls
- DSP memory mapped to `$FF8000-$FFBFFF`

Hatari implements DSP audio by:
- Emulating DSP56001 instruction execution cycle-by-cycle
- Maintaining DSP internal registers and address counters
- Processing DSP audio samples and mixing with the YM2149 output
- Using SDL2 audio callbacks for host-side audio output

### MIDI Port Emulation

Atari ST has dual MIDI DIN5 ports (IN/OUT). Emulation:

- Routes MIDI data between emulated ST and host MIDI APIs
- Supports USB-MIDI (via host middleware) and virtual serial port MIDI
- IKBD chip transfers MIDI data from port to system memory via MFP
- Emulated MIDI ports can connect to real synthesizers or DAWs

## File System and Disk Emulation

### Disk Image Support

Hatari supports multiple Atari ST disk image formats:

| Format | Extension | Description |
|--------|-----------|-------------|
| Atari native | `.ST`, `.STX` | Raw sector images (9 sectors x 512 bytes, single/double density) |
| MSA | `.MSA` | Microsoft disk image format (also used by Atari) |
| CTR | `.CTR` | Caravel/STonX image format |
| IMG | `.IMG` | Raw sector-by-sector disk images |
| IPF | `.IPF` | ImageDisk Pro (copy-protected disk format with non-standard sector layouts) |

IPF support uses `libcapsimage` (Software Preservation Society license) for reading protected disk images with:
- Variable sector sizes
- Odd-numbered sector layouts
- Non-standard timing values
- Copy protection patterns that require cycle-accurate FDC

### GEMDOS Hard Disk Emulation

Hatari supports virtual hard disks:
- Directories on the host filesystem mapped as GEMDOS partitions
- Hard disk image files (`.HDC`, raw `.img`)
- GEMDOS partition table emulation (MFM/RFM format)
- File allocation table (FAT) emulation

### File Translation Layer

Hatari implements a file I/O translation layer:
- Translates GEMDOS file calls (XBIOS `OPEN`, `CLOSE`, `READ`, `WRITE`, etc.) to host filesystem equivalents
- Maps Atari path conventions (`A:`, `C:`) to host directories
- Handles Atari file attributes (read-only, directory, system, hidden)
- Supports Atari filename conventions (6.3 format, uppercase only)

## MultiTOS and Guest OS Integration

### EmuTOS and Emu86

**EmuTOS** is a free/open-source replacement for proprietary TOS ROM images. Hatari can:
- Load any TOS ROM image (1.02, 2.06, 3.06, 4.05 etc.)
- Load EmuTOS ROM in place of proprietary TOS
- Boot from disk image or autostart a program
- Patch TOS ROM at load time for emulator compatibility

Hatari 2.1.0 added **Emu86** support for MultiTOS host integration:
- Enables Hatari to run as a guest under an existing MultiTOS (FreeMiNT) host OS
- Uses **NatFeats** API for guest-host communication
- Guest applications are executed by the host OS (FreeMiNT) rather than by the emulator

### NatFeats API

The **NatFeats** API enables communication between the emulated Atari and the host emulator process:

| NatFeats Function | Purpose |
|------------------|---------|
| `NF_WRITE` / `NF_WRITE_STR` | Write to Hatari console output |
| `NF_READ` / `NF_READ_STR` | Read from Hatari console input |
| `NF_SCREENINFO` | Get screen resolution and color depth |
| `NF_SCREEN_BLIT` | Transfer framebuffer data to emulator display |
| `NF_GETMOUSE` / `NF_SETMOUSE` | Mouse position and state |
| `NF_GETKEY` / `NF_PUTKEY` | Keyboard state |
| `NF_BEEPER` | Generate a beep sound |
| `NF_GETCLOCK` / `NF_SETCLOCK` | System time |
| `NF_SAVE_SCREEN` | Save screenshot |
| `NF_GETSTATE` / `NF_SETSTATE` | Get/set emulator state |
| `NF_EXEC` | Execute a program in the guest OS |
| `NF_EXIT` | Terminate the guest program |

To enable NatFeats, Hatari must be started with `--natfeats true` and `--cmd-fifo` options. Guest programs call NatFeats functions through a special XBIOS intercept (previously `XBios(255)`, now replaced by dedicated NatFeats entry points).

### MultiTOS Guest Host Integration

With `--natfeats` and `--cmd-fifo` enabled:
1. Guest TOS calls NatFeats XBIOS functions to communicate with Hatari
2. Hatari translates NatFeats calls into host OS operations (console I/O, screen updates, etc.)
3. Guest programs can run on the host OS directly via `NF_EXEC`, bypassing the 68K emulator entirely for host-native code

This enables running some native x86 programs inside the emulated ST environment, significantly improving performance for compatible software.

## State Management

### Save States

Hatari implements full system save states:
- CPU registers (all 8 data, 8 address, program counter, status register, user/kernel mode, interrupt priority)
- Memory contents (complete RAM dump)
- All chipset register states (MFP, IKBD, VDL, SHARC, blitter, DMA)
- ROM state and patch data
- Disk image media state (for IPF and floppy images)
- Video framebuffer contents

Save states are stored as binary files with a `.sam` extension (save and restore memory).

### Soft Reset vs Hard Reset

- **Soft reset** (`COLDBOOT` vector at `$4A`): Executes the ROM COLDBOOT routine, re-initializing RAM and TOS without clearing all memory. Implemented by writing to the reset vector.
- **Hard reset**: Completely reinitializes the emulator state, reloading the TOS ROM and all chipset registers to their power-on defaults.

## Popular Emulators and Cores

### Hatari

The primary open-source Atari ST emulator (GPL v2+).

**Key details:**
- **Author**: David Hafner, Peter Fejes, and contributors
- **License**: GNU GPL v2+ (with exception for IPF library)
- **Platforms**: Linux, Windows, macOS, FreeBSD, NetBSD, Android (via Hataroid)
- **Current version**: 2.6.1
- **CPU**: WinUAE 68000 core + WinUAE 68030/DSP core for TT/Falcon
- **Graphics**: SDL2 rendering with optional hardware acceleration
- **Sound**: YM2149 cycle-accurate + ST-DMA FIFO + Falcon DSP56001
- **Key features**: Blitter (TOS 1.02+), VDI multi-resolution (up to 800x600), IPF disk image support, NatFeats guest-host integration, save states, debugger with instruction tracing and cycle counting

**Versions and milestones:**
- v1.0+ (2011-2013): Blitter emulation, VDI multi-resolution, YM/WAV sound export
- v2.0.0 (2018): WinUAE CPU core, 68030 MMU, caches, prefetch, bus error retry; YM2149 cycle-accurate; improved STE DMA sound
- v2.1.0 (2021): Falcon emulation, DSP56001, screenpointer, CPU frequency/cache control at `$FF8E21`
- v2.5.0 (2024): Enhanced DSP, YM2149 syncsquare, improved 68030 MMU/cache, IKBD improvements, USB-RTC support

**Libretro core:**
- **hatariB**: Libretro/RetroArch core fork by bbbradsmith (Patreon-sponsored). Integrates Hatari emulation with Libretro API for cross-platform frontend support. Maintains compatibility with all Hatari features.

### Steem / Steem Engine

**Steem** was a commercial/freeware Atari ST emulator for Windows, later continued as **Steem SSE** (ST Enhanced Emulator Sensei Software Edition).

**Key details:**
- Supported ST, STe, STFM, STE, Mega STE variants
- Cycle-accurate 68000 CPU with adjustable speed multiplier (can run faster than 8 MHz for acceleration)
- YM2149 sound emulation with MIDI support
- Built-in debugger with trace logging
- Configurable CPU speed (useful for timing-sensitive games)
- Excellent MIDI port integration for music software

Steem SSE continues the project with:
- Updated YM2149 emulation
- Improved compatibility with copy-protected software
- Enhanced MIDI support for modern systems
- 100+ configuration options for fine-tuning emulation

### MiSTery / FX CAST (FPGA Cores)

**FPGA-based cores** (not software emulators) implement the Atari ST hardware at the logic-gate level on FPGA hardware (MiST, MiSTer, DE10-Nano, etc.).

#### MiSTery Core

- **Author**: Gyorgy Szombathelyi (gyurco)
- **Platforms**: MiST (FPGA board), MiSTer (FPGA dev board)
- **Languages**: Verilog, SystemVerilog, VHDL
- **Features:**
  - Cycle-accurate FX68K CPU core (from `ijor/fx68k`)
  - Cycle-accurate chipset emulation
  - USB-RTC support
  - All ST/STe variants
  - Supports TOS ROM images and EmuTOS

#### FX CAST Core

- **Author**: ijor
- **Specializations:**
  - **Cycle-accurate 68000 CPU**: Exact instruction timing
  - **Cycle-accurate FDC**: Can load and run copy-protected disk images
  - **Cycle-accurate chipset**: All ST chipset chips implemented in FPGA logic
  - **STe variant support**: Full STE/Mega STE emulation
  - **Custom MiSTer build**: Available as a standalone flashable image

#### MiSTer MegaSTE Support

MiSTer cores implement the MegaSTE variant with:
- Cache emulation (RAM and ROM cache enable modes)
- Higher color modes (16 and 256 colors)
- ST-DMA sound with FIFO

### ARAnyM

**ARAnyM** (a.k.a. ARAne Mole) is an Atari TT/Falcon/TOS Virtual Machine.

**Key details:**
- **License**: GNU GPL
- **Approach**: System emulation rather than cycle-accurate hardware emulation
- **CPU**: 68030 (TT) and 68030+DSP56001 (Falcon)
- **Features:**
  - Runs real TOS or FreeMiNT as the guest operating system
  - Host OS integration (runs on Linux, Windows, macOS, etc.)
  - Direct file system access (no disk image needed)
  - QEMU-based 68k CPU emulation
  - Network support (TCP/IP, PPP)
  - SCSI hard disk emulation

ARAnyM achieves better performance for productivity software through host OS integration, but is less compatible with timing-sensitive games and demos compared to cycle-accurate approaches.

### QEMU

**QEMU** provides 68k CPU emulation and can run TOS/FreeMiNT on x86 hosts:
- Uses QEMU's built-in 68k core
- Compatible with FreeMiNT as the guest OS (not TOS)
- Better performance than standard ARAnyM when combined with EmuTOS
- Limited to system-level emulation (no cycle-accurate hardware)

## Emulator Comparison

| Feature | Hatari | Steem SSE | MiSTery/FX | ARAnyM |
|---------|--------|-----------|------------|--------|
| **CPU approach** | WinUAE trace tables | Interpreted/traced | FPGA hardware | QEMU 68k core |
| **Cycle accuracy** | Yes (CPU + chipset) | Yes | Exact (FPGA) | No (system-level) |
| **ST model support** | ST/STE/TT/Falcon | ST/STe/MegaSTE | ST/STe/MegaSTE | TT/Falcon |
| **FPU** | 68882 (TT) | External | FPGA | Host |
| **DSP** | Yes (DS$56001$) | No | FPGA | No |
| **Blitter** | TOS 1.02+ | Yes | FPGA | No |
| **Sound** | YM2149 + DMA + DSP | YM2149 + MIDI | FPGA | None |
| **Disk formats** | ST/MSA/CTR/IMG/IPF | ST/MSA | ST/MSA | Any raw disk |
| **Copy protection** | Via IPF + exact FDC | Yes | Exact | No |
| **MultiTOS** | Partial (NF_EXEC) | Yes | Yes (on FPGA ROM) | Yes |
| **Debugger** | Yes (trace + cycles) | Yes (trace) | On-chip logic | None |
| **Save states** | Yes (`.sam`) | Yes | On-chip RAM | No |
| **Libretro core** | hatariB | N/A | N/A | N/A |
| **License** | GPL v2+ | Commercial/Freeware | GPL | GPL |

## Key Implementation Challenges

### Timing-sensitive Software (Games and Demos)

Atari games and demos frequently rely on precise chip timing:
- **Race-the-beam effects**: Reading CRT scanline position during the horizontal blank
- **Raster bars**: Triggering effects at specific video counter values
- **Copy protection**: Exploiting specific disk access timing patterns
- **Music player effects**: YM2149 envelope sync-square, digitized sound with DMA timing

Cycle-accurate emulation is essential for these use cases. Frame-rate-based or instruction-count-based timing produces incorrect behavior.

### Video Timing Complexity

- **Horizontal scanning**: 288 lines at 31.25 kHz (PAL) must align with the video counter
- **Vertical blank**: The VBI interrupt must fire at the exact cycle of the vertical blank, as used by desktop update loops and game synchronization
- **DAC transition timing**: Color changes during certain parts of the scanline can cause visible artifacts (used by demo effects)

### Memory Arbitration

The ST's shared memory bus between the CPU, video controller, and DMA controller creates complex arbitration logic:
- Different memory regions have different bus access timing (chip RAM vs ROM vs. I/O registers)
- DMA steals cycles from both CPU and video
- The blitter can initiate its own DMA bursts

### FPU Emulation

For TT/Falcon software running on ST/STe models, emulating the 68882 FPU through software requires:
- Decoding floating-point instructions (Line F)
- Implementing IEEE 754 double-precision operations in software
- Handling FPU exceptions and trap vectors
- Many ST/STE emulators rely on host OS libraries (libm) for basic FPU support

### MIDI Integration

Atari ST MIDI support requires real-time data transfer at precise timing:
- IKBD chip processes MIDI bytes from the hardware interrupt
- TOS handles MIDI protocol at the driver level (MIDI_INIT, MIDI_OPEN, SEND/RECEIVE commands)
- Emulators must route MIDI events to host MIDI APIs at the correct timing relative to the 68K instruction stream

## Emulator Architecture Diagram

```
+---------------------+         +------------------------+
|  Guest TOS/TurboTOS  |         |  Hatari Emulator Core  |
|  (68K machine code)  |         |                          |
+----------+-----------+         +------------+-------------+
           |                                    |
           |                                    | CPU address/data bus cycles
           |                                    |
           v                                    v
+------------------------------------------------------------------+
|   CPU Core (WinUAE 68000/68030 traces or trace tables)         |
|   Cycle counting, IACK, prefetch, bus arbitration              |
+------------------------------------------------------------------+
           |                                    |
           |  Memory reads/writes              | Interrupt requests
           |                                    |
     +-----+-----+                         +---v---+
     |   Memory Map   |                     |  Chipset Emulation Layer  |
     |  (RAM/ROM/IO)  |                     |                            |
     +-----+-----+                         +---+---+
           |                                    |
     +-----v-----+                         +---v---+     +------------------+
     |  Framebuffer |<-----VDI/Palette-----| VGL/VDL |     |  Sound Engine    |
     |  (SDL2/OSD)  |                       | Gen/     |     | YM2149/DMA/DSP   |
     +-----+-----+                         | Blitter  |     +--------+---------+
           |   +---------------------+     +--------+     |              |
     +-----v---v-----+               |     | |    |    +---v--+   +-----v----+  |
     |  SDL2 Renderer|<----VBI       |     | |    |    |  YM |   |  ST-DMA  |  |
     |  (Hardware)   |<----interrupt |     | |    |    |2149 |   |  FIFO    |  |
     +---------------+               |     | |    |    +------+   +----------+  |
                                     |     |v   v    |              |            |
                                     |     |RTC  SH|     +------v-----+   +------+
                                     |     +------+     |    DSP56001 |   | MIDI |
                                     |                    |   DSP Core  |   | Port |
                                     |                    +-------------+   +------+
                                     |
                              +------+---+--------+
                              |  File Translation  |
                              |  Layer (disk I/O)   |
                              +---------------------+
                             /     |        |         |
                 +----------+      |        |         +----------+
                 | ST/MSA/CTR   IPF       HDD      Host FS/GEMDOS
                 +-----------------------------------------------+
                                       |
                                +------v------+
                                |  Host OS     |
                                |  (Linux/Win) |
                                +---------------+
```
