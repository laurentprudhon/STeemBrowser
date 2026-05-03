# Atari ST ROM and BIOS Architecture

## 1. TOS (Tramiel Operating System) Overview

TOS is the proprietary operating system of the Atari ST family. It is a monolithic, single-tasking OS with a cooperative multitasking layer via the AES (Application Environment Services).

### TOS Version History

| Version | Platform | Release Year | ROM Size | Notes |
|---------|----------|-------------|----------|-------|
| TOS 1.00 | 520ST | 1985 | 3 x 27C256 (64 KB each) = 192 KB | Original TOS |
| TOS 1.02 | 1040ST / 520STe | 1986 | 3 x 27C256 = 192 KB | Fuji logo, updated boot |
| TOS 1.04 | Mega ST | 1987 | 3 x 27C256 = 192 KB | First all-in-one model |
| TOS 1.42 | 1040STf | 1987 | 3 x 27C256 = 192 KB | Built-in floppy drive |
| TOS 1.62 | 520STE / 1040STE | 1988 | 4 x 27C512 = 256 KB | STE support, cookie jar |
| TOS 2.06 | Mega STE | 1990 | 6 x 27C512 = 768 KB | MMU, VME, Super Hi-Res |
| TOS 3.06 | TT030 | 1992 | 4 x 27C512 = 256 KB | 32-bit TT architecture |
| TOS 4.04 | Falcon030 | 1992 | 4 x 27C512 = 512 KB | DSP, 32-bit, 16-bit audio |

### ROM Layout

```
Address Range        Size      Content
───────────────────────────────────────────────────────────
$F00000 - $F0FFFF    256 KB    HD6301V1 IKBD microcontroller ROM
                                (keyboard firmware, mouse, joystick, RTC)
$F40000 - $F5FFFF    128 KB    ST-Bios / ST-Dos ROM (TOS 1.x)
$F60000 - $F7FFFF    128 KB    GEM / AES / VDI / GDOS ROM
$F80000 - $F9FFFF    128 KB    TOS core (BIOS, GEMDOS, cookie jar)
$FA0000 - $FBFFFF    128 KB    TOS core continued
$FC0000 - $FDFFFF    128 KB    TOS core continued
$FE0000 - $FFFFFF    128 KB    TOS core continued (final 64KB = $7FFF00-$FFFF)
```

### IKBD Microcontroller ROM ($F00000)

The HD6301V1 (68E01) handles:
- Keyboard scancode scanning (94 keys, matrix de-bounce)
- Mouse polling (absolute position, button states)
- Joystick state reading
- Time-of-day clock (BCD format)
- RS232 serial port (via UART at 7812.5 baud)

**IKBD ROM Checksum:** All bytes from $F00000 to $F0FFFF must sum to $FF.

### TOS ROM Checksum

TOS 1.x-2.x checksum at $7D8E-$7D8F:
```
a := 0
for offset = $0000 to $7D8C step 2:
    a := (a + ROM[offset]) mod 65536
result := (a + WORD[$7D8E]) mod 65536
if result != $B4A9: ROM checksum failure
```

## 2. BIOS (Basic Input/Output System)

The BIOS sits below GEMDOS and provides machine-specific hardware access. It is callable from user mode (up to 3 recursive calls).

### BIOS Entry Point

- **Address:** `TRAP #10` entry at `$FFFFEC84`

### BIOS Function List

| Fn # | Name | Description |
|------|------|-------------|
| 0 | `CRYPT` | RAM integrity verification |
| 1 | `RESINH` | I/O unit initialization |
| 2 | `SWINIT` | Software interrupt initialization |
| 3 | `BIOSEVT` | BIOS event handler |
| 4 | `PICONST` | Printer constant table (not on ST) |
| 5 | `PINIT` | Printer initialization (not on ST) |
| 6 | `GEMGTR` | Set/get palette (6 colors, 16 bytes each) |
| 7 | `SCRNXT` | Read screen data (not on ST) |
| 8 | `SCRCONST` | Screen constants (not on ST) |
| 9 | `GEMCTL` | GEM video control |
| 10 | `GEMDEV` | Get GEM device descriptor |
| 11 | `BIOSWTCH` | Switch video modes |
| 12 | `SLEEP` | Sleep/delay |
| 13 | `BDBASE` | Set disk base address ($FF8E00) |
| 14 | `GETBVB` | Get buffer base ($200)
| 15 | `RESMCR` | Memory controller (get/set, $B456)
| 16 | `DMA` | DMA memory manager
| 17 | `GETVP` | Get VDI pointer
| 18 | `SDEVIC` | Standard device check
| 19 | `SERROR` | Standard error handler
| 20 | `CONIN` | Console input
| 21 | `CONOUT` | Console output
| 22 | `CONCTRL` | Console control
| 23 | `CONIOCTL` | Console I/O control
| 24 | `BLKIN` | Block input (device, buffer, count)
| 25 | `BLKOUT` | Block output
| 26 | `DEVINIT` | Device initialization
| 27 | `DEVIOCTL` | Device I/O control
| 28 | `GETVF` | Get VDI format info
| 29 | `CONWIOCTL` | Console WIO control
| 31 | `VIOCTL` | VDI I/O control (STe only)
| 32 | `SCRMOD` | Set/get video mode (STe only)
| 34 | `DIOCTL` | DMA I/O control (Mega STE only)
| 36 | `SCREG` | Screen record I/O (STe only)
| 38 | `GEMCTL` | Extended video control (TT, Falcon)
| 41 | `MSTGEMGTR` | Extended palette (STE)
| 42 | `MSTSCRNXT` | Screen read (STE)
| 43 | `MSTSCRCT` | Screen constants (STE)
| 44 | `CONIN` | Extended console input
| 46 | `BLKINX` | Extended block input
| 47 | `BLKOUTX` | Extended block output
| 49 | `BIOSEVT` | Extended BIOS event
| 50 | `SWINITS` | Software interrupt (STE)
| 51 | `GEMCONT` | GEM controller (STE)
| 52 | `RESMCR` | Memory controller (STE)
| 53 | `GEMVSCR` | Virtual screen (STE)
| 54 | `BLKIN` | Block input (STE)
| 55 | `BLKOUT` | Block output (STE)

### BIOS Drive Numbers

| Device | Drive Label |
|--------|-------------|
| 0 | Floppy A: |
| 1 | Floppy B: (or logical A: on single-disk) |
| 2 | CON: (console) |
| 3 | PRN: (printer) |
| 4 | LPT: (parallel) |
| 5 | COM1: (serial) |
| 6 | COM2: (serial, Mega STE) |
| 7 | RAM: (disk image, Mega STE/STe HDDriver) |

## 3. XBIOS (Extended BIOS)

The XBIOS provides direct hardware access unique to the Atari ST, bypassing the abstraction layer.

### XBIOS Entry Point
- **TRAP #1** instruction (vector at $FFF FFC82)
- Function number in D0 register

### XBIOS Function List

| Func # | Opcode | Name | Description |
|--------|--------|------|-------------|
| 1 | $01 | `MSPROP` | Mouse properties |
| 2 | $02 | `KBDNAME` | Keyboard language name |
| 3 | $03 | `KBDNUM` | Keyboard layout number |
| 4 | $04 | `KEYBD` | Keybd driver call |
| 5 | $05 | `RESINIT` | I/O unit init |
| 6 | $06 | `BICON` | Baud rate (COM1) |
| 7 | $07 | `GEMGTR` | Color properties |
| 8 | $08 | `SCRNXT` | Get screen data |
| 9 | $09 | `SCRREG` | Set screen record |
| 10 | $0A | `VIOCNT` | Video controller |
| 11 | $0B | `VIOCTL` | Video I/O control |
| 12 | $0C | `GEMCTL` | Video mode |
| 13 | $0D | `DMA` | DMA control |
| 14 | $0E | `BDBASE` | Set disk base ($FF8E00) |
| 15 | $0F | `BDEVST` | Device status |
| 16 | $10 | `SWINITS` | Software ints |
| 17 | $11 | `SWRESM` | Memory controller |
| 18 | $12 | `BINTADR` | Block in (low-level) |
| 19 | $13 | `BOUTADR` | Block out (low-level) |
| 20 | $14 | `GEMDEV` | Get device descriptor |
| 21 | $15 | `SCRMOD` | Set video mode |
| 22 | $16 | `RESMCR` | Get/set memory controller |
| 23 | $17 | `BICONMAP` | Baud rate map |
| 24 | $18 | `RSHINIT` | RS232 init |
| 25 | $19 | `RSHIN` | RS232 input |
| 26 | $1A | `RSHOUT` | RS232 output |
| 27 | $1B | `RSHCTR` | RS232 control |
| 28 | $1C | `BLTINIT` | Blitter init (STE) |
| 29 | $1D | `BLTMODE` | Set blitter mode |
| 30 | $1E | `BLTCMD` | Blitter command |
| 31 | $1F | `VSETRES` | Set video resolution |
| 32 | $20 | `VGETRES` | Get video resolution |
| 33 | $21 | `VSYNC` | Wait for vsync |
| 34 | $22 | `VSNDINIT` | Sound init (STE) |
| 35 | $23 | `VSNDCTL` | Sound control |
| 36 | $24 | `VSNDIN` | Sound input |
| 37 | $25 | `VSNDOUT` | Sound output |
| 38 | $26 | `VSNDSTA` | Sound status |
| 39 | $27 | `VSNDLEN` | Sound sample length |
| 40 | $28 | `VSNDADDR` | Sound sample address |
| 41 | $29 | `VSNDPRO` | Sound properties |
| 42 | $2A | `VMEMAL` | Allocate virtual memory (STE) |
| 43 | $2B | `VMEMFR` | Free virtual memory (STE) |
| 44 | $2C | `BCONMAP` | Serial device to BIOS #1 |
| 45 | $2D | `BICONP` | Baud rate (all ports) |
| 46 | $2E | `RSHINP` | RS232 input (all ports) |
| 47 | $2F | `RSHOUTP` | RS232 output (all ports) |
| 48 | $30 | `GEMCTLP` | GEM controller (all ports) |
| 49 | $31 | `BLTSTAT` | Blitter status |
| 50 | $32 | `GETSCS0` | Get SCSI status (Mega STE) |
| 51 | $33 | `GETSCS1` | Get SCSI info (Mega STE) |
| 52 | $34 | `GETSCS2` | Get SCSI config (Mega STE) |
| 53 | $35 | `GETSCS3` | Get SCSI ID (Mega STE) |
| 54 | $36 | `GETSCS4` | Get SCSI phase (Mega STE) |
| 55 | $37 | `SETSCS0` | Set SCSI status (Mega STE) |
| 56 | $38 | `SETSCS1` | Set SCSI info (Mega STE) |
| 57 | $39 | `GETSCS5` | Get SCSI data (Mega STE) |
| 58 | $3A | `GETSCS6` | Get SCSI phase (Mega STE) |
| 59 | $3B | `GETSCS7` | Get SCSI status (Mega STE) |
| 60 | $3C | `GETSCS8` | Get SCSI config (Mega STE) |
| 61 | $3D | `GETSCS9` | Get SCSI data (Mega STE) |
| 62 | $3E | `VGETMOD` | Get video mode |
| 63 | $3F | `VSETPAL` | Set palette |
| 64 | $40 | `BLITMST` | Get blitter mode |
| 65 | $41 | `DMAST` | Get DMA status |
| 66 | $42 | `DMAADDR` | Get DMA address |
| 67 | $43 | `DMACNT` | Get DMA count |
| 68 | $44 | `GEMCTLC` | GEM close |
| 69 | $45 | `DMACTL` | DMA controller |
| 70 | $46 | `VSCNTRL` | VDI control (STE) |
| 71 | $47 | `GEMCTLK` | GEM key |
| 72 | $48 | `VSETMOD` | Set video mode |
| 73 | $49 | `VGETPAL` | Get palette |
| 74 | $4A | `SCRNXTB` | Get screen (STE, block) |
| 75 | $4B | `SCRREGB` | Set screen record (STE, block) |
| 76 | $4C | `DMAADDRX` | Get/set DMA address (STE) |
| 77 | $4D | `DMACNTX` | Get/set DMA count (STE) |
| 78 | $4E | `VSCRCTL` | VDI screen control (STE) |
| 79 | $4F | `VDEVST` | VDI device status |
| 80 | $50 | `VMOUSE` | Virtual mouse (STE) |
| 81 | $51 | `VMEMALX` | Virtual memory alloc (STE) |
| 82 | $52 | `VMEMFRX` | Virtual memory free (STE) |
| 83 | $53 | `VGETMODX` | Get video mode (STE) |
| 84 | $54 | `VSETPALX` | Set palette (STE) |
| 85 | $55 | `VGETPALX` | Get palette (STE) |
| 86 | $56 | `VSCRNCT` | VDI screen color (STE) |
| 87 | $57 | `VDEVSTX` | VDI device status (STE) |
| 88 | $58 | `VSETMODE` | Set mode (Falcon) |
| 96 | $60 | `DSP_DOBLOCK` | DSP block (Falcon) |
| 97 | $61 | `DSP_BLKHAND` | DSP block handshake (Falcon) |
| 98 | $62 | `DSP_BLKUNPK` | DSP unpacket block (Falcon) |
| 100 | $64 | `DSP_OUTSTR` | DSP output stream (Falcon) |
| 300 | $12C | `PCI_LB` | PCI lower base |
| 301 | $12D | `PCI_LM` | PCI lower mask |
| 302 | $12E | `read_config_byte` | PCI read byte |
| 303 | $12F | `read_config_word` | PCI read word |
| 304 | $130 | `read_config_long` | PCI read long |
| 305 | $131 | `fast_read_config_byte` | PCI fast read byte |
| 306 | $132 | `fast_read_config_word` | PCI fast read word |
| 307 | $133 | `fast_read_config_long` | PCI fast read long |
| 308 | $134 | `write_config_byte` | PCI write byte |
| 309 | $135 | `write_config_word` | PCI write word |
| 310 | $136 | `write_config_long` | PCI write long |
| 311 | $137 | `fast_write_config_byte` | PCI fast write byte |
| 312 | $138 | `fast_write_config_word` | PCI fast write word |
| 313 | $139 | `fast_write_config_long` | PCI fast write long |
| 321 | $141 | `fast_read_mem_byte` | PCI fast read mem byte |
| 322 | $142 | `fast_read_mem_word` | PCI fast read mem word |
| 323 | $143 | `fast_read_mem_long` | PCI fast read mem long |
| 330 | $14A | `fast_read_io_byte` | PCI fast read IO byte |
| 331 | $14B | `fast_read_io_word` | PCI fast read IO word |
| 332 | $14C | `fast_read_io_long` | PCI fast read IO long |
| 346 | $152 | `fast_write_mem_byte` | PCI fast write mem byte |
| 347 | $153 | `fast_write_mem_word` | PCI fast write mem word |
| 348 | $154 | `fast_write_mem_long` | PCI fast write mem long |
| 360 | $154 | `fast_write_io_byte` | PCI fast write IO byte |
| 361 | $155 | `fast_write_io_word` | PCI fast write IO word |
| 362 | $156 | `fast_write_io_long` | PCI fast write IO long |
