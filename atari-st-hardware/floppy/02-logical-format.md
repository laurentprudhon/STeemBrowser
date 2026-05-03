# Floppy Disk Logical Format

> Complete description of Atari ST floppy disk logical layout: boot sector, BIOS parameter block (BPB), FAT, directory, and TOS file system structures.

## Disk Label and Media Description

### Media Descriptor Byte

| Parameter | Value |
|-----|-----|
| Media descriptor byte | $F9 (double-sided, 80-track, 9-sector DD) |
| Alternative values | $F0 (40-track DD), $F8 (single-sided) |
| Location | TOS boot sector byte offset 0x1A / sector 1 byte offset 0x1A |
| PC equivalent | $F0 (DD), $F9 (DD HD) |

### Disk Label

| Parameter | Value |
|-----|-----|
| Disk label field | 11 ASCII characters |
| Location | TOS boot sector byte offset 0x1B |
| Purpose | Human-readable disk name (not used by TOS) |

## Boot Sector (Sector 1, Track 0, Side 0)

The first sector on every Atari ST disk (physical sector 1 at track 0, side 0) is the **boot sector**. It contains:

### Boot Sector Layout

```
BOOT SECTOR LAYOUT (512 bytes, offset 0x0000 to 0x01FF):

Offset   Length   Field                    Description
────────  ──────   ───────                    ─────────────────────────────────
  0x000      3    Jump instruction           EB xx 90 — jump to boot code (xx = bytes to skip)
  0x003     11    OEM Name / Disk Label      ASCII disk name (e.g. "TOS  DISK" or "SYSTEM")
  0x00E      2    Bytes Per Sector           0x0200 (512 bytes, fixed for ST)
  0x010      1    Sectors Per Cluster         1–128 (TOS uses 1 = 1 sector/cluster)
  0x011      2    Reserved Sectors           1 (boot sector is the only reserved sector)
  0x013      1    Number of FAT Copies        2 (standard TOS uses dual FAT)
  0x14       2    Max Root Directory Entries  224 (standard ST) / 112 / 160
  0x016      2    Total Sectors (small)       0 if > 0xFFFF (use extended field)
  0x018      1    Media Descriptor Byte        $F9 (80-track DD) / $F0 (40-track)
  0x019      2    Sectors Per FAT             TOS-calculated (depends on format)
  0x01B      2    Sectors Per Track           9 (standard TOS format)
  0x01D      2    Number of Heads (sides)     2
  0x01F      2    Hidden Sectors               0 (no hidden sectors on floppy)
  0x021      2    Total Sectors (large)        0 (floppy uses small field)
  0x023      2    Physical Drive Number        0 (A:) or 0x80 (BIOS drive number)
  0x025      1    Unused / Reserved            0
  0x026      1    Extended Boot Signature      0x29 (TOS extended BPB)
  0x027      12   Volume Serial Number         Random 32-bit + 16-bit (e.g. $1234-5678-9ABC)
  0x02E      11   Volume Label                 Disk label string (uppercase ASCII)
  0x039      8    File System Type             "TOS FAT" or "FAT12" (depends on TOS version)
  0x041    467    Boot Code                    TOS secondary boot loader / BIOS call
  0x1FD      2    Boot Signature               0x55AA (valid boot sector marker)
```

### Jump Instruction

```
EB 0E 90     — Standard TOS jump code
               - EB 0E = JMP $0013 (14 bytes to skip)
               - 90 = NOP (padding)
```

### Boot Signature

Always $55AA at offset 0x1FD. Without this, the TOS does not recognize the sector as bootable.

### Extended Boot Signature

Offset 0x026 = $29 indicates an extended BIOS parameter block is present. This is required for formats larger than 32 MB.

## Boot Process from Floppy

```
Power-on / Cold Boot:
┌───────────────────────────────────────────────────────────────────┐
│ 1.  Reset vector ($000000) → SP init, CPU init                    │
│ 2.  TRAP #1 (VME boot check → $FF0000–$FFFFFE search for $55AA) │
│ 3.  If found → jump to $FF8004 (VME boot)                        │
│ 4.  If not found → BIOS boot via ROM                               │
│ 5.  $FFFFEC → get_config → $FF8046 (boot init)                    │
│ 6.  Init TOS internal structures                                   │
│ 7.  Call GEMDOS (trap #1) / BIOS functions                         │
│ 8.  BIOS init drives (SINIT at $FF805C)                            │
│ 9.  Read boot sector: SBOOT($FF8E86) → drive A: sector 1         │
│ 10. Load boot sector to $0600:$0000                                │
│ 11. Execute boot code at $0600:$0000                               │
│                                                                   │
│ Boot Sector Code Flow:                                              │
│   - Verify media descriptor byte                                   │
│   - Read FAT1 to register                                           │
│   - Read root directory entry 0 (first directory block)            │
│   - Read file "CONFIG.SYS" from root                               │
│   - CONFIG.SYS → boot TOS.DRV                                      │
│   - TOS.DRV → load TOS.GEM / TOS.SYS (GEMDOS / XBIOS)            │
│   - Jump to GEMDOS entry point                                     │
└───────────────────────────────────────────────────────────────────┘

WARM BOOT (Ctrl+Alt+Del):
┌───────────────────────────────────────────────────────────────────┐
│  1.  MFP interrupt (via KEYBD port)                                │
│  2.  MFP reads keyboard → recognizes Ctrl+Alt+Del                  │
│  3.  Call warm boot vector at $FFFFF4                              │
│  4.  Jump to BIOS warm boot at $FF809C                             │
│  5.  Same as cold boot from step 5 above (re-initializes TOS)      │
└───────────────────────────────────────────────────────────────────┘
```

### Cold Boot Sequence (Detailed)

```
  BIOS Entry Points:
  ─────────────────
  $FFFFEC      GET_CONFIG    Get boot config
  $FF8004      VME_BOOT      Entry for VME bus boot
  $FF8046      BOOT_INIT     Initialize TOS structures
  $FF805C      SINIT         Initialize floppy hardware
  $FF8098      SBOOT         Read boot sector from floppy
  $FF809C      WARM_BOOT     Warm boot entry point

  SBOOT Operation:
  ───────────────
  1.  Set drive = A (drive 0)
  2.  Read physical sector 1, track 0, side 0
  3.  Verify media descriptor byte ($F9)
  4.  Load 512 bytes to $0600:$0000 (RAM address)
  5.  Check boot signature $55AA at offset 0x1FD
  6.  If valid → JMP $0600:$0000
  7.  If invalid → disk error (reject)
```

## BIOS Parameter Block (BPB)

The **BPB** is defined in the boot sector at offsets 0x0B–0x01D. It describes the physical format of the disk to the TOS file system driver.

```
BPB FIELD VALUES (standard 720 KB ST format):

  Bytes/Sector       = $0200 (512)
  Sectors/Cluster    = $01 (1 sector per cluster)
  Reserved Sectors   = $0001 (sector 0 = boot sector)
  FAT copies         = $0002 (dual FAT)
  Root dir entries   = $00E0 (224 × 32 bytes = 7,168 bytes)
  Total sectors      = depends on format
  Media descriptor   = $F9 (80-track DD)
  Sectors/FAT        = calculated: (224 × 32) / 512 = 14 sectors
                       (plus overhead for FAT2 = 14)
  Sectors/track      = $0009 (9)
  Heads              = $0002 (2 sides)
  Hidden sectors     = $0000
  Drive number       = loaded from $0023 or computed by BIOS
```

### FAT Sectors Calculation

```
FAT sectors = ceil((root_dir_entries × 32 bytes) / bytes_per_sector)
            = ceil((224 × 32) / 512)
            = ceil(7168 / 512)
            = 14 sectors

Total sectors = reserved + (FAT1 + FAT2) + root_dir + data
              = 1 + (14 + 14) + 14 + data_area
              = 43 + data_area

For 9-sector DD format:
  data_area = (total_tracks × sectors - 43)
            = (80 × 9 × 2 - 43)
            = 1440 - 43
            = 1397 sectors
```

## FAT Layout (File Allocation Table)

The FAT is stored at sectors starting from the first reserved sector ($0001). With dual FAT, there are two identical copies.

```
FAT LAYOUT FOR 720 KB (9-sector DD):

  Reserved Area:
    Sector 0         (offset 0x0000) = Boot sector / BPB

  FAT1:
    Sectors 1–14     (14 sectors = 7,168 bytes)
    Cluster mapping: clusters 0–2F (48 entries × 12 bits = 7,2 entries)
    Note: 12-bit FAT per cluster

  FAT2:
    Sectors 15–28    (14 sectors = 7,168 bytes)
    Identical copy of FAT1

  Root Directory:
    Sectors 29–42    (14 sectors = 7,168 bytes)
    224 entries × 32 bytes = 7,168 bytes

  Data Area:
    Sectors 43–1439  (1397 sectors × 512 bytes = 715,264 bytes)
    Cluster 0 = reserved (media descriptor)
    Cluster 1 = reserved
    Cluster 2 = first usable data cluster
    Cluster 2–58B = available data clusters (58A usable entries)
```

### 12-Bit FAT Entry Format

| Entry Value | Meaning |
|-----|-----|
| $000 | Free cluster |
| $001 | Reserved (bad cluster indicator) |
| $002–$FFE | Next cluster in chain |
| $FFF | End of file (EOF) |
| $FF8–$FFB | Reserved (media descriptor area on some implementations) |
| $FFC–$FFF | Reserved / used as media descriptor area |

### Cluster to Sector Mapping

Since sectors per cluster = 1, cluster N maps to sector:

```
absolute_sector = 1 + FAT_sectors × 2 + root_dir_sectors + cluster
                = 1 + 28 + 14 + cluster
                = 43 + cluster

physical_track = absolute_sector / (9 × 2)       = sector / 18
physical_sector = (absolute_sector mod 18) + 1     = (sector % 18) + 1
physical_head = (absolute_sector / 9) mod 2        = (sector / 9) % 2
```

## Root Directory Structure

Each directory entry is 32 bytes:

```
DIRECTORY ENTRY (32 bytes):

Offset   Length   Field                    Description
────────  ──────   ───────                    ───────
  0x00      8    Filename                  8-byte file name
  0x06      3    Extension                 3-byte file extension
  0x09      1    Attributes                Bit flags:
                                           - Bit 0: Read-only (01h)
                                           - Bit 1: Hidden (02h)
                                           - Bit 2: System (04h)
                                           - Bit 3: Volume ID (08h)
                                           - Bit 4: Directory (10h)
                                           - Bit 5: Archive (20h)
  0x0A      1    Unused / reserved          Always $00
  0x0B      7    Create time/date            milliseconds+time+date
  0x0E      2    Last access date            date only
  0x10      2    First cluster (high 16-bit)| FAT12: high 16 bits of cluster #
  0x12      2    Modify time/date            time+date
  0x14      2    First cluster (low 16-bit) | FAT12: low 16 bits of cluster #
  0x16      4    File size                   bytes
  0x1A     16    Pad / unused                Always $00
  0x2A      2    Unused                      Always $00
```

### Filename Padding

- Filenames < 8 characters: padded with spaces ($20)
- Extensions < 3 characters: padded with spaces ($20)
- First byte of name: if $E5, entry is deleted
- First byte $00: end of directory (no more entries)

### File Names on Atari ST Disks

TOS standard files (boot disk):

| Name | Type | Purpose |
|-----|-|-|
| CONFIG.SYS | Application | TOS configuration program (loads drives) |
| TOS.DRV | Application | TOS driver loader |
| TOS.SYS | Program | Kernel / GEMDOS |
| TOS.GEM | Program | GEM / GUI |
| XBIOS.CTL | Application | Extended BIOS calls |
| MCGAME | Directory | Default game directory |
| MCEXE | Directory | Executable directory |
| MCDOC | Directory | Document directory |

## Data Area Layout

```
FAT12 CLUSTER CHAIN FOR A FILE:

  ┌─ Cluster 2 ──┐   ┌─ Cluster 5 ──┐   ┌─ Cluster 8 ──┐   ┌─ EOF ──┐
  │ FAT[2] = $005│ → │ FAT[5] = $008│ → │ FAT[8] = $00F│ → │$00FF F │
  │ Dir entry     │   │ Dir entry     │   │ Dir entry     │   │         │
  │ $0002–$01FF  │   │ $0002–$01FF  │   │ $0002–$01FF  │   │         │
  └──────────────┘   └──────────────┘   └──────────────┘   └─────────┘

  First cluster stored in directory entry at offset 0x14 (high) + 0x16 (low)
  For FAT12: first 12 bits of the 16-bit word at offset 0x14 give the high 12 bits
  Actually for FAT12:
    offset 0x14 → word = (high 8 bits of cluster number)
    offset 0x16 → word = (low 16 bits of cluster number)
  Cluster = (word[0x14] high byte) × 256 + word[0x16]

  Note: 520ST uses FAT12 with 128 entries for root dir:
  Entry[2] → word at 0x14 has high bits, word at 0x16 has low bits
```

## Custom Formats

TOS supports custom formats via the `FORMAT` BIOS call:

| Parameter | Value |
|-----|-----|
| Sectors/track | 1–33 (any value) |
| Sides | 1 or 2 |
| Cylinders | 1–255 |
| Sectors/cluster | 1, 2, 4, 8, 16, 32, 64, 128 |
| Root dir entries | 32–224 (must be multiple of 8) |
| Reserved sectors | 1–255 |

### Common Custom Formats

| Name | Format | Capacity | Used For |
|-----|-|- | --- |
| ST default | 9 × 512 × 2 × 80 | 720 KB | Standard |
| 18-sector | 18 × 256 × 2 × 80 | 720 KB | PC compatibility |
| 22-sector | 22 × 256 × 2 × 80 | 880 KB | ST-Max, specialized |
| HD 18-sector | 18 × 512 × 2 × 80 | 1.44 MB | HD format (requires modification) |
| 8-sector | 8 × 512 × 2 × 80 | 640 KB | Bootable (smaller FAT) |
| 6-sector | 6 × 512 × 2 × 80 | 480 KB | Smaller FAT overhead |

### Format Command

The TOS FORMAT command at BIOS $FF8076 performs:

1. Low-level format of each track (Write Track command on FDC)
2. Format sector headers with ID, sector number, CRC
3. Write gap bytes (IAM area, GAP 3a, GAP 3b, DAM, GAP 4)
4. Verify each track (Read Address command)
5. Mark bad sectors if detected
6. Initialize boot sector with BPB
7. Initialize FAT1 and FAT2
8. Initialize root directory

### STOS / ST-Max Formats

STOS formats a disk at 18 sectors × 256 bytes × 2 sides × 80 tracks = **720 KB** but with:
- Different gap sizes than standard TOS format
- Sector size field = $01 (256 bytes)
- Uses PC-compatible sector count (82 per disk side)
- The 18 sectors per track uses same PM3 encoding but with different gap padding

## TOS File System Driver (DRIVE.SYS)

The TOS file system driver (`DRIVE.SYS`) abstracts the physical floppy format:

### BIOS Disk Functions

| Function | Entry Point | Description |
|-----|-|- |
| $FF805C | SINIT | Initialize floppy hardware |
| $FF8062 | SDISK | Disk present check |
| $FF8076 | FORMAT | Format disk |
| $FF807C | SDISK2 | Extended disk information |
| $FF8E86 | SBOOT | Read boot sector |
| $FF8092 | SDISK5 | Read sector with retry |
| $FF80A0 | SDISK6 | Write sector with retry |

### GEMDOS Disk Functions

GEMDOS trap $65 ($24) calls use DRIVE.SYS, which abstracts:

| GEMDOS Function | Description |
|-----|-|
| $65 (get disk address) | Get disk parameters |
| $66 (set disk address) | Set active drive |
| $3B (get current directory) | Get current directory path |
| $3C (create subdirectory) | Create dir entry |
| $3E (delete file) | Remove file entry |
| $3F (find first) | Search directory |

### File System Abstraction

```
GEMDOS / XBIOS Application calls
         ↓
    GEMDOS / XBIOS layer
         ↓
    DRIVE.SYS (file system driver)
         ↓
    BIOS (low-level disk I/O)
         ↓
    SINIT → SBOOT / SDISK5 / SDISK6
         ↓
    FDC (WD1772) commands: Read Sector / Write Sector
         ↓
    DMA (FDRQ/FDACK handshake) → RAM
         ↓
    WD1772 → DIN14 → Head/Disk (PM3 encoding/decoding)
```

## FAT12 File Allocation in Detail

### FAT12 Entry Encoding

FAT12 uses 12 bits (1.5 bytes) per entry, stored as overlapping entries in a byte-aligned array:

```
  FAT byte layout for entries [0, 1, 2, 3, 4, 5]:
  
  Byte #:  0   1   2   3   4   5   6   7   8   9
           ┌───┬───┬───┬───┬───┬───┬───┬───┬───┬───┐
           │0_l│0_h│1_l│1_h│2_l│2_h│3_l│3_h│4_l│4_h│ ← even entry low byte
           ├───┼───┤   ├───┼───┤   ├───┼───┤   ├───┤
           │1_l│   │2_l│   │3_l│   │4_l│   │5_l│   │ ← odd entry high byte
           └───┘   └───┘   └───┘   └───┘   └───┘

  Entry N (even): read word at byte 2N, take low 12 bits
  Entry N (odd): read word at byte 2N-1, take high 12 bits
```

### FAT Chain Following Algorithm

```
  Given cluster C in directory entry:
  
  1. Compute FAT offset for entry C:
     FAT_offset = C × 3 / 2  (in bytes)
  
  2. If C is even:
     temp = FAT[FAT_offset] + (FAT[FAT_offset+1] × 256)
     next_cluster = temp AND $0FFF   (take low 12 bits)
  
  3. If C is odd:
     temp = (FAT[FAT_offset] / 16) + (FAT[FAT_offset+1] × 16)
     next_cluster = temp AND $0FFF   (take high 12 bits)
  
  4. If next_cluster = $000 → free (end of search)
  5. If next_cluster = $FFF → EOF
  6. If next_cluster = $001 → bad cluster
  7. Continue with next_cluster
```

### Root Directory Location

```
  Root directory start sector (absolute) = 1 + FAT_sectors × 2
                                          = 1 + 14 × 2
                                          = 29
  
  Root directory end sector (absolute) = 29 + root_dir_sectors - 1
                                        = 29 + 14 - 1
                                        = 42
  
  Root directory starts at physical:
    track = 29 / 18 = 1
    sector = (29 mod 18) + 1 = 11 + 1 = 12
    head = 29 / 9 mod 2 = 1
```

## ST-Specific File System Features

### ST File Type Extension

Atari ST uses a 3-character file type extension (beyond the regular 3-char FAT extension):

| File Type | Description |
|-----|- |
| TOS / EXE / PRG | Executable (GEM/DOS program) |
| TOS / DRV / SYS | Device driver |
| TOS / BIF | BIF font file |
| TOS / FNT | Font file |
| TOS / CDT | Atari CDT CD-ROM file |
| TOS / XBP | XBD file |
| STX / TOS | ST-XBIOS executable |

### Special Boot Files

When booting, TOS expects these files in order:

1. `CONFIG.SYS` — System configuration (disk loading)
2. `TOS.DRV` — TOS driver loader
3. `TOS.SYS` — GEMDOS kernel
4. `TOS.GEM` — GEM desktop environment
5. `XBIOS.CTL` — Extended BIOS handler

CONFIG.SYS can also have:
- `DISK A:` — Load driver from drive A
- `DISK B:` — Load driver from drive B
- `DISK C:` — Load driver from drive C
- Line numbers and comment markers

## Disk Image Formats and Logical Layout

### .ST Image Format

Standard Atari disk image (raw physical sectors back-to-back):

```
  .ST FILE LAYOUT:
  
  Offset    Size         Content
  ────    ────         ───────
  0x0000   1×512B      Sector 1 (boot/bpb) from track 0 side 0
  0x0200   1×512B      Sector 2 from track 0 side 0
  ...       ...
  0x0E00   1×512B      Sector 9 from track 0 side 0
  0x0E01   1×512B      Sector 1 from track 0 side 1
  ...       ...
  0x0FFF   1×512B      Sector 9 from track 0 side 1
  ...       ...
  0x5D400  1×512B      Last sector (track 79 side 1 sector 9)
  
  Total: 720 × 512 = 368,640 bytes per side
          720 KB physical (raw 512-byte sector data, no gaps/IAM)
```

Logical layout: sectors are in physical storage order (track by track, side by side), not in filesystem order.

### .MSA Image Format

Metadata-enriched image:

```
  .MSA FILE LAYOUT:
  
  Offset    Size         Content
  ────    ────         ───────
  0x0000   24 bytes     MSA header (magic + metadata)
  0x0018   1 byte       Media descriptor byte ($F9, $F0, etc.)
  0x0019   2 bytes      Tracks (little-endian, 80)
  0x001B   2 bytes      Sectors per track (little-endian, 9)
  0x001D   2 bytes      Sides (1 or 2)
  0x001F   2 bytes      Sector size (128/256/512)
  0x0021   1 byte       Sectors per cluster (1)
  0x0022   1 byte       Reserved sectors (1)
  0x0023   1 byte       FAT copies (2)
  0x0024   2 bytes      Root dir entries (224)
  ...      ...
  header_end raw_sector data (each 512-byte sector with metadata)
```

Logical layout: same as .ST but with metadata header.

### .DIM / .STX Preservation Formats

.DIM and .STX (PASTI) preserve physical layout details including:
- Gap sizes per sector
- MFM encoder state
- Weak bit positions
- Density modulation (bit-rate variation protection)
- Track orientation
- Raw flux timing (via .SCP/.HFE integration)
