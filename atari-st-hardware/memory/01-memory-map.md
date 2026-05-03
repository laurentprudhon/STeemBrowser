# Memory Map

> Complete 16 MB address space map for Atari ST / STe / Mega ST / Mega STE systems.

## Overview

The Atari ST uses the MC68000's 24-bit address bus, providing a **16 MB** address space ($000000-$FFFFFF). The memory map is divided into:

- **$000000-$007FFF**: OS page (exception vectors + OS data)
- **$008000-$3FFF**: RAM (user-accessible)
- **$400000-$9FFF**: Reserved/RAM
- **$A00000-$A7FFF**: Video RAM (32 KB shared with system RAM)
- **$A80000-$F7FFF**: RAM (extended)
- **$F80000-$F9FFF**: Reserved (HDC/ACSI)
- **$FA0000-$FBFFF**: Cartridge ROM (128 KB)
- **$FC0000-$FDFFF**: TOS ROM (lower, 192-256 KB)
- **$FE0000-$FFFFFF**: TOS ROM (upper, 384 KB + hardware registers)

## Full Memory Map

| Address Range | Size | Purpose | R/W |
|-- | ---- |---- | --|
| $000000-$000003 | 4 B | Supervisor Stack Pointer (boot) | R |
| $000004-$000007 | 4 B | Initial PC (boot) | R |
| $000008-$00000B | 4 B | Bus Error vector | R/W |
| $00000C-$00000F | 4 B | Address Error vector | R/W |
| $000010-$0003FB | 1 KB | Exception vector table | R/W |
| $000400-$0007FF | 1 KB | OS vector table | R/W |
| $000800-$000FFF | 2 KB | OS page variables | R/W |
| $001000-$00FFFF | 384 KB | TOS ROM bootstrap + OS code | R |
| $100000-$3FFF | 3.875 MB | System RAM (varies by model) | R/W |
| $400000-$4FFF | 1 MB | RAM (STe: system RAM) | R/W |
| $500000-$5FFF | 1 MB | RAM (STe: system RAM) | R/W |
| $600000-$AFFF | 4 MB | RAM (expandable) | R/W |
| $A00000-$AAFFFF | 32 KB | Video RAM (shared with system RAM) | R/W |
| $B000000-$F7FFFF | 7.5 MB | RAM (expandable) | R/W |
| $F80000-$F803FF | 1 KB | ACSI registers | R/W |
| $F90000-$F9FFFF | 64 KB | Reserved (HDC/SCSI) | - |
| $FA0000-$FBFFFF | 128 KB | Cartridge ROM | R |
| $FC00000-$FDFFFF | 64 KB | RAM (last page) | R/W |
| $FE0000-$FEFFFF | 64 KB | ROM (TOS lower) | R |
| $FF0000-$FFFFFF | 512 KB | TOS ROM (upper) + hardware registers | R/W |

## RAM Sizes by Model

| Model | Total RAM | Video RAM | User RAM (TPA) |
| ------ | --- |-- | ---- |
| 520ST | 512 KB | 32 KB | 480 KB |
| 1040ST | 1 MB | 32 KB | 960 KB |
| Mega ST | 1 MB | 32 KB | 960 KB |
| 520STE | 1 MB | 32 KB | 960 KB |
| 1040STE | 4 MB | 32 KB | 3.97 MB |
| Mega STe | 2 MB | 32 KB | ~1.97 MB |

## TOS ROM Mapping

| Address | Content | Size |
|-- | --- | --- |
| $F80000-$FEFFFF | TOS 1.x (Mega TOS, 256 KB) | 256 KB |
| $FE0000-$FFFFFF | TOS 2.x (192 KB usable) | 192 KB |
| $FE0000-$FFFFFF | TOS 3.x (384 KB) | 384 KB |

On the Mega STE:

| Address | Content | Size |
| -- | --- | --- |
| $F00000-$F7FFFF | STe TOS (512 KB) | 512 KB |
| $F80000-$FFFFFF | Mega STE TOS (256 KB) | 256 KB |

## User Mode Restrictions

In **user mode** (S bit = 0 in SR):
- Addresses $000000-$00007FFF are inaccessible (first 2 KB of RAM)
- Hardware registers at $FF8000 and above are inaccessible
- Only supervisor mode (S bit = 1) can access these areas

In **supervisor mode**:
- Full 16 MB address space is accessible
- All hardware registers are accessible

## DMA Address Space

The ACSI/DB19 port maps to $F80000-$F803FF:

| Address | Function |
| -- | --|
| $F80000 | Command register (write) |
| $F80002 | Status register (read) |
| $F80004-$F803E | SCSI/HDC data buffers and commands |

## References

- [Atari Master Memory Map (PDF)](https://www.atarimania.com/documents/Master-Memory-Map.pdf)
- [Atari ST/STe/Mega ST Memory Map - AtariCompendium PDF](https://atari-forum.com/viewtopic.php?t=42594)
- [Retrovirology - Atari ST Memory Map](https://www.retrovirology.ca/pages/AtariMemoryMap-en.html)
- [AtariWiki: Memory Map](https://atariwiki.org/wiki/Wiki.jsp?page=Memory%20Map)
