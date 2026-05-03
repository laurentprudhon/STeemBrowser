# Floppy Disk Physical Format

> Complete description of Atari ST 3.5" floppy disk physical format: PM3/MFM encoding, sector layout, track geometry, and flux recording.

## Media and Drives

### Physical Specifications

| Parameter | Value | Notes |
|-----|-----|------|
| Disk size | 3.5" (89 mm square) | Rigid polycarbonate encapsulated |
| Magnetic layer | γ-Fe₂O₃ cobalt-doped | Barium ferrite in later media |
| Platter rotation | 300 RPM ±0.2% | Constant angular velocity |
| Surface zones | CAV (constant angular velocity) | Linear velocity decreases inward |
| Number of platter surfaces | 2 (two sides, two heads) | |
| Number of tracks per surface | 80 (tracks 0–79) | 135 TPI (tracks per inch) |
| Head type | Magnetoresistive read, inductive write | Combined heads (SH2) |
| Track width | ~0.6 mm | |

### Drive Models

| Drive | Max Capacity | Model | Drive Types |
|-----|-----|-----|- --|
| SF354 | 720 KB DS DD | Internal 3.5" DD | TEAC FD235F, Chinon FDD-3148, Citizen 2898 |
| SF354B | 720 KB DS DD | External | Same drive in external enclosure |
| SF314 | 720 KB DS DD | Internal (1040STF) | TEAC FD235F-HW, Chinon CDD-3148 |
| SF354B-HD / SF354B-SD | 1.44 MB HD | External HD | TEAC FD235F-HA, Pioneer 255B |

All DD drives accept standard 3.5" 720 KB or 1.44 MB diskettes. HD drives accept DD and HD diskettes.

### RPM and Timing

```
300 RPM = 5 rotations/second = 200 ms per revolution
Index pulse rate: 5 Hz (once per rotation)
Track circumference (outer, track 0): ~138 mm
Linear velocity (track 0): 3.44 m/s
Linear velocity (track 79): 0.43 m/s
```

## PM3 Encoding (MFM - Modified Frequency Modulation)

PM3 is the standard MFM encoding used on the Atari ST. Each data bit is encoded as a pattern of magnetic flux reversals on the disk surface.

### Encoding Rules

In MFM, each original data bit is represented by two channels (a channel is the time slot between flux transitions):

```
Data Bit   MFM Channel Pattern    Flux Transitions
─────────────────────────────────────────────────
    0        0 1                     1 transition
    1        1 1                     1 transition
   00        0 1 0 1                2 transitions
   01        0 1 1 1                1 transition
   10        0 1 0 1                2 transitions
   11        1 1 0 1                1 transition
  000        0 1 0 1 0 1            3 transitions
  001        0 1 0 1 1 1            1 transition
  010        0 1 1 1 0 1            1 transition
  011        0 1 1 1 1 1            1 transition
  100        1 1 0 1 0 1            2 transitions
  101        1 1 0 1 1 1            1 transition
  110        1 1 1 1 0 1            1 transition
  111        1 1 1 1 1 1            1 transition
```

Rules:
1. Between any two consecutive MFM channels, at least one must be 1 (clock bit). A 00 pattern is illegal.
2. If the data bit is 1, it is written as 11 (clock + data).
3. If the data bit is 0 and the preceding MFM output was 1, output 01. If the preceding was 0, output 00.
4. Clock bit (first of pair) tells the data separator when to expect transitions.
5. Maximum gap between flux transitions: 2 channel periods.
6. Minimum gap between flux transitions: 1 channel period (two adjacent 1s).

### Bit Rate

| Mode | Data Rate | Channel Rate | Bits Per Sector |
|-----|---------|--------------|-----------------|
| DD Single Density (STD) | 250 kbps | 500 kbaud | 128 |
| DD Double Density (ST) | 500 kbps | 1 Mbps | 512 |
| HD Double Density | 500 kbps | 1 Mbps | 1024 |

On the Atari ST, the WD1772 always writes at double-density rate (500 kbps) for both DD and HD modes. The difference is in the write current and head gap.

## Track Structure

A track is laid out sequentially as these areas (starting from the index pulse):

```
<-- Index Gap -->  <---- Sector 1 ---->  <---- Sector 2 ---->  ...  <---- Sector N ---->
| G3   |   IDAM    |  ID   |  GCRC | GAP |  DAM  |  DATA  |  DCRC | G4 |  G3   |  IDAM  ...
| GAP3a|  Track#   | Side# | Sec#  | Sz  | CRC1  |       |        |     | GAP3a |  Track# ...
```

### Complete Track Layout (DD 9-sector format)

```
                        TRACK LAYOUT (one rotation = 200 ms)
================================================================================

  Index Gap        Post-ID Gap      Pre-Data Gap      Post-Data Gap
  (IAM area)       (Gap 3a)         (Gap 3b)          (Gap 4)
  ┌──────┐      ┌──────────┐   ┌─────────┐    ┌───────┐   ┌────────┐
  │ G=4E │  →   │ F5 FF FF │ → │ F5 FF   │ →  │ FB FF│ →  │ 54 FF  │ →
  │ SYNC │      │  IDAM    │   │ ID (6B) │    │ GCRC  │   │ DAM    │
  │(80B) │      │  sync    │   │ data    │    │ CRC   │   │ sync   │
  │      │      └──────────┘   └─────────┘    └───────┘   └────────┘
  │      │                                │
  │      │                                ├─── Data (512 bytes) ───┤
  │      │                                │  FB FF FF FF ... FF    │
  │      │                                │  DCRC CRC              │
  │      │                                └────────────────────────┘
  │      │
  └──────┘
   ~1280           ~960            ~768             ~960
   bytes            bytes           bytes            bytes
```

### Sector Structure (Detailed)

Each sector in the Atari ST DD format has this structure:

| Field | Sync Pattern | Data | Length | Description |
|-----|-----|-----|-----|-----|
| GAP 3a | 80× 4E | 00 | ~1280 bytes | Index gap after index pulse (TOS omits IAM byte) |
| IDAM | F5 FF FF | 00 | 3 bytes | ID Address Mark (sync for sector header) |
| Track | - | 1 byte | Track number | Cylinder = head position (0–79) |
| Side | - | 1 byte | Head number | 0 = side A, 1 = side B |
| Sector | - | 1 byte | Sector number | 1–9 (sector numbers are 1-based) |
| Sector Size | - | 1 byte | S = 2 | 0=128, 1=256, 2=512, 3=1024 |
| ID CRC | CRC | - | 2 bytes | CRC-CCITT over 4 header fields above |
| GAP 3b | F5 FF FF | 00 | ~768 bytes | Inter-sector gap (sets up data separator) |
| DAM | FB FF FF | 00 | 3 bytes | Data Address Mark (sync for data field) |
| Data | - | varies | 512 bytes | Actual sector data (TOS writes 512 bytes) |
| Data CRC | CRC | - | 2 bytes | CRC-CCITT over 512 data bytes |
| GAP 4 | F5 FF FF | 00 | ~960 bytes | Post-data gap to next sector IDAM |

```
SECTOR LAYOUT (one sector):

  IDAM      ID FIELD                    GCRC    GAP       DAM        DATA FIELD         DCRC
 ┌────────┬──────────────┬─────┬──────┬──────┬──────┬───────────┬───────────────────┬──────┐
 │ F5 FF  │ Track  Side   Sec   Size  │ CRC1 │ CRC1 │ FB FF    │ 512 data bytes    │ CRC2 │ CRC2 │
 │ sync   │ (1B)   (1B)  (1B)  (1B) │ (2B) │      │ sync     │                   │ (2B) │ (2B) │
 └────────┴──────────────┴──────┴──────┴──────┴──────┴───────────┴───────────────────┴──────┘
    ~3B        ~4B            ~2B    ~768B         ~3B          ~512B              ~2B
```

### CRC-CCITT Specification

Used for both CRC1 and CRC2 in the Atari ST format:

- Polynomial: `x^16 + x^12 + x^5 + 1` (0x1021)
- Initial value: 0x0000
- No final XOR
- Input reflected (LSB first)
- Checksum byte order: high byte first, low byte second

### GAP and Sync Patterns

During `Write Track` (formatting), the FDC translates specific control bytes:

| Control Byte | Written to Disk | Output Description |
|-----|-----|-----|
| 4E FF FF | 4E FF FF | PM3 separator sync |
| 00 | 00 | Sync (all zeros) |
| F6 01 | F6 01 FC | Index Address Mark (IAM) |
| F5 | F5 (with clock insertion) | ID Address Mark (IDAM) |
| FB | FB (with clock insertion) | Data Address Mark (DAM) |
| F7 | (CRC bytes only) | CRC terminator |
| FF | all 1s clocked | Data bytes |

The WD1772 special encoding during `Write Track` (also called formatting):
- `F5` + 6 bytes → written as IDAM header with ID CRC
- `FB` + 512 bytes → written as data field with data CRC
- `F7` → written as CRC bytes only (no sync pattern)
- `F6` + 1 byte → written as index mark

### Track Byte Count for 9-Sector DD Format

| Area | Approx. Bytes | Notes |
|-----|-----|------|
| Index gap (IAM area) | ~1280 | Variable, set at format |
| 9 × (IDAM+ID+GCRC+GAP) | ~7200 | 9 sectors × ID overhead |
| 9 × (DAM+Data+DCRC) | ~23760 | 9 sectors × 512B data per sector |
| 9 × GAP 4 | ~8640 | ~960 bytes per track per sector gap |
| **Total track length** | **~10,500–11,000** | Varies with GAP3a/IAM size |

Actual track storage (user data): 9 sectors × 512 bytes = 4,608 bytes per track per side = **737,280 bytes/disk**

## Sector Layout Details

### Inter-Sector Timing

At 500 kbps data rate, each bit period is 2 µs. The MFM encoding doubles this to 4 µs per channel.

The data separator chip in the WD1772 (or in the ST glue logic) handles the clock recovery from MFM transitions. The ST format uses a standard data separator that:

1. Detects flux transitions on the Read Data pin
2. Recovers a clock from the transitions (up to 2 channel periods between transitions)
3. Synchronizes to F5/FB/C7 address mark sync patterns
4. Samples data bits at the recovered clock phase
5. Passes raw data to the WD1772 state machine

### Interleave

The Atari ST does **not** use physical interleave. Sectors are sequential around the track:

```
Track layout (9 sectors):

   Index ┴── Sector 1 ──┴── Sector 2 ──┴── Sector 3 ──┴── ... ──┴── Sector 9 ──┴──→ Index

   Sector numbering:     1         2         3         4         5         6         7  8  9
```

The TOS floppy driver also uses **logical interleave** in software. TOS sector 1 (the boot sector) is physically sector 1 on every track. File data sectors are laid out sequentially in the directory's FAT chain. The TOS floppy driver handles the DMA transfer in software, so there are no timing constraints on sequential reads.

### Default TOS Format Parameters

The standard TOS `format` command produces:

| Parameter | Value |
|-----|-----|
| Sectors per track | 9 |
| Sides | 2 |
| Cylinders (tracks) | 80 |
| Bytes per sector (data field) | 512 |
| ID CRC | CRC-CCITT |
| Data CRC | CRC-CCITT |
| Sector numbering | 1-based (1 through 9) |
| Sector size field | 2 (value encoding 512 bytes) |
| Gap 3a / IAM | Omitted (no IAM byte, unlike IBM PC format) |
| IDAM sync | F5 |
| DAM sync | FB |
| Initial sector data | Written by TOS after format (boot code, BPB, directory, FAT) |

## Comparison with IBM PC Format

Both use PM3 (MFM) encoding, double-sided, 80 tracks, 512-byte sectors. Differences:

| Parameter | Atari ST Default | IBM PC DD 1.44 MB | IBM PC HD 1.44 MB |
|-----|-----|-----|-----|
| Sectors/track | 9 | 18 | 18 |
| Gap 3a + IAM | No IAM byte | IAM (F6) present | IAM present |
| Data area gap | Different gap size | 32 bytes | 32 bytes |
| RPM | 300 | 300 | 300 |
| Bit rate (DD) | 500 kbps | 250 kbps | 500 kbps |
| Head settle time | 30 ms | 30 ms | 30 ms |
| Stepping rate | Fixed | Various | Various |

The ST format's **9 sectors × 512 bytes × 2 sides × 80 tracks = 720 KB** is not the same as PC DD format (18 sectors × 256 bytes × 2 × 40 = 720 KB) because:
1. Different sectors per track (9 vs 18)
2. Different sector data size (512 vs 256)
3. Different track count (80 vs 40 for PC DD)
4. Different gap padding (different sizes, different IAM usage)

### Why ST Floppies Don't Read on PCs

Atari ST 720 KB format (9/512/80/PM3) cannot be read by standard PC drives due to:
1. PC DD drives only write 40 tracks (ST needs 80)
2. Gap sizes differ (PC uses 32-byte Gap 3, ST uses different gap)
3. Different sectors per track
4. Different sector sizes (PC DD = 256B, ST = 512B)

A PC HD drive (1.44 MB) reading 80-track PM3 at 500 kbps can read the track geometry but gap padding still won't match.

## Reading and Writing Mechanics

### Read Path

```
┌─────────┐     ┌──────┐   ┌──────────┐  ┌─────────┐   ┌───────────┐   ┌──────────┐
│  Drive  │  ←  │ Amp  │ ← │ Data Sep │← │ WD1772  │ ← │ DMA FIFO │ ← │ System  │
│ Head(s) │     │      │   │  (1772)   │  │ FDC     │   │ (32B)    │   │ RAM     │
└─────────┘     └──────┘   └──────────┘  └─────────┘   └───────────┘   └──────────┘
  (flux        (preamp   (clock        (data          DMA             (512B
   recovery →  recovery  recovery)   recovery)      handshaking        sectors)
   pin 1)      → pin    → F5/FB       → FDC           → FDRQ/FDACK     ←→ DRAM
               DIN14    sync lock     state         (active low)
                                        machine
```

On read:
1. Head reads flux transitions at 100 kbit/s raw on the drive (DIN14 pin 1)
2. Preamp amplifies and filters the signal (S-chip or STe)
3. Data separator in WD1772 reclocks from transitions
4. FDC state machine identifies F5/FB address marks
5. When sector ID matches, FDC sets DRQ active
6. WD1772 sends FDRQ (active low) to DMA chip
7. DMA controller grabs bus from CPU, sets FDACK (active low)
8. For each byte: FDC loads byte into DSR, DRQ set → CPU reads $FF8603

### Write Path

On write:
1. Software (TOS or user) queues data to transfer buffer
2. FDC issues Step command to seek track
3. FDC issues Write Sector command with ID, sector number, size
4. FDC writes PM3-synced sector header (F5) when index pulse detected
5. After IDAM sync, writes sector header data bytes + CRC1
6. Writes gap bytes (F5-synced)
7. Writes DAM (FB) when data separator resyncs
8. Reads data bytes from FIFO via DRQ/DMA handshake
9. CRC2 computed by FDC hardware, written after data
10. Writes GAP 4, continues with next sector if multiple sector mode

### Motor Control

Motor-on sequence:
1. FDC writes command with motor-on flag (bit 3 set)
2. FDC waits for 6 index pulses (~1.2 seconds) before proceeding
3. Drive motor reaches 300 RPM within ~50 ms
4. After command completes, FDC counts 10 idle index pulses (~2 seconds)
5. FDC de-asserts motor-on signal
6. VBL routine (TOS at $43E / flock flag) handles side-band motor-off

## Track 0 Seek and Restore

The WD1772 seeks to a track via:

| Command | Byte | Action |
|-----|-----|-----|
| Restore | 00h | Step inward until TR00 = 0 (track 0) |
| Seek | 01h+ | Seek to cylinder in DATA register |
| Step In | 20h+ | Step one track toward 0 |
| Step Out | 30h+ | Step one track away from 0 |

- Stepping rate r1r0: 00 = 6ms, 01 = 12ms, 10 = 2ms, 11 = 3ms (3.5" = 11)
- Head settles ~30 ms after stepping
- Verify flag (bit 2): read sector header, compare track number

## Bad Sector Handling

The ST handles bad sectors via:

1. **Format-time bad sector remapping**: During format, any sector that fails CRC is marked bad
2. **Bad sector table (BST)**: TOS maintains a list of bad cylinders (in TOS memory at $170)
3. **TOS retries**: On CRC error, BIOS retries up to 4 times
4. **Remap during format**: TOS formats around bad sectors, marking them in the BST
5. **Bad sector marking in hardware**: User can write to a specific track sector and induce errors

The BST is stored in RAM and loaded from the last formatted disk on each boot. Bad sectors on disk become inaccessible to the filesystem.

## Density and Write Precompensation

### Write Precompensation

For double-density (MFM), write precompensation adjusts flux density on inner tracks:

- Inner tracks have shorter circumference → tighter bit spacing
- Without precomp, adjacent flux transitions would interfere
- Write precomp point: adjusted by WD1772 register (P bit in command byte)

P bit in Write Sector command:
- P = 1: Enable write precompensation (required for DD write)
- P = 0: Disable (for SD write only)

On the Atari ST, P is always set for DD write operations.

### Density Mode

| FDC Register Bit | Value | Density |
|-----|-----|-----|
| 1772 Register 3, bit 4 | 0 | Single density (250 kbps) |
| 1772 Register 3, bit 4 | 1 | Double density (500 kbps) |

On the Atari ST, the ST always uses double density. The density bit is set by the TOS driver when accessing 720 KB disks.

## Track Capacity Summary

| Parameter | DD (9/512) | DD (18/256) HD (18/512) |
|-----|-----|-----|
| Sectors/track | 9 | 18 |
| Bytes/sector | 512 | 256 |
| Sides | 2 | 2 |
| Tracks/side | 80 | 80 |
| Total user capacity | 720 KB | 720 KB | 1.44 MB |
| Track overhead | ~54% | ~70% | ~54% |

The ST default format produces 720 KB of user data per disk (9 sectors × 512 bytes × 2 sides × 80 tracks).

## Low-Level Format Commands

The Atari BIOS provides these low-level format commands:

| Command | Byte | Description |
|-----|-----|-----|
| Format Track (Write Track) | 71h | Write sector headers, gaps, and data |
| Format with zeroes | 71h + data of 00 | Writes blank data field pattern |
| Format with test pattern | 71h + data of 6DB6 | Writes maximum flux transitions |

Atari's recommended format pattern is 0x6DB6 repeated (fills with max flux transitions: 11011011 pattern), which stresses PM3 encoding on all transitions.
