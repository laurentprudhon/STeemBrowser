# Atari ST Floppy Disk Copy Protection

> Complete analysis of Atari ST floppy copy protection techniques: weak bits, bit-rate variation, sector skew, non-standard sectors, density switching, and their hardware basis.

## Overview

Atari ST copy protection operates at the **physical level** of the floppy disk, exploiting differences between the mastering drive, the protection verification drive, and a standard copying drive. Unlike software-based protections that can be bypassed by patching code, physical protections modify the magnetic flux patterns on the disk surface itself.

### Protection Types Classification

Protection schemes are categorized by their mechanism:

| Type | Name | Detection Basis | Copyable? |
|-|-|-|-|
| 1 | Bad sectors / CRC errors | Hardware error flags | No |
| 2 | Non-standard sector numbers | Sector ID mismatch | No |
| 3 | Weak/Fuzzy bits | Bit-level read uncertainty | No (with standard FDC) |
| 4 | Bit-rate (density) variation | PM3 clock recovery | No (requires two-density writer) |
| 5 | Sector skew / interleave | Sector positioning trick | Partial (with special tools) |
| 6 | Data under index | Data between index pulse and IDAM | No |
| 7 | Multiple density on same track | Mixed DD/SD sectors | No |
| 8 | Custom FDC command tricks | WD1772 behavior quirks | Partial |
| 9 | Sector size variation | Non-standard sector encoding | No |

## Type 1: Bad Sector Protection

### Principle

The master disk has sectors that produce CRC errors when read. The protection code reads these sectors and checks the FDC status register for bit 4 (RNF — Record Not Found) or bit 3 (CRC error). If the error is NOT present, the protection detects a copy.

### Mastering Technique

```
Master disk creation:
1. Write sector data to a specific track location
2. Intentionally corrupt the magnetic media (reduce coercivity)
3. Or write at non-standard gap sizes that cause resync failure
4. The FDC reads the sector header (F5) but fails the data CRC
5. FDC status register sets bit 3 (CRC error) or bit 4 (RNF)

Standard drive reading:
- Sector header F5 sync detected (sector ID readable)
- Data field begins with FB sync pattern
- Data CRC computed ≠ CRC on disk
- FDC returns CRC ERROR status bit

Copy drive reading:
- Identical process
- CRC error still present
- When copied to new disk, the copied data CRC is FIXED
- Verification fails → copy detected
```

### Implementation Example

```assembly
; Atari ST protection: check for bad sector
; Read a sector known to have bad CRC

  move.b  #2, $FF8803    ; Sector number (bad sector)
  move.w  #0, $FF8801    ; Track (cylinder)
  move.b  #0, $FF8802    ; Side (head)

  ; Call read sector via BIOS or direct FDC access
  bsr   ReadSector
  ; Check FDC status at $FF8600
  move.b  $FF8600, d0    ; Read status register
  andi.b  #$18, d0        ; Mask RNF bit 4 + CRC bit 3
  beq    protection_failed  ; No error = copy detected!
```

### Drive Modification

Some protection schemes write bad sectors by:
- Applying a strong magnetic field during write (induced by special head current modulation)
- Writing at reduced write current (magnetic layer barely stores the field)
- Using an external magnet during the write process
- Deliberately misaligning the head gap

## Type 2: Non-Standard Sector Numbers

### Principle

Sectors are written with sector numbers outside the valid range (e.g., sector 10 on a 9-sector disk, or sector 0). The FDC still reads these sectors if the track number matches, but the TOS filesystem driver will not recognize or access them.

### Master Disk Sector Layout

```
Track 0 standard sectors:
┌──────────┬──────────┬──────────┬───────┬──────────┬──────────┬──────────┐
│ Sector 1 │ Sector 2 │ Sector 3 │ ...  │ Sector 7 │ Sector 8 │ Sector 9 │
│ (boot)   │ (BPB)    │ (FAT)    │       │ (DAM)    │ (DAM)    │ (DAM)    │
│ TOS boot │ BPB+dir  │ FAT1     │ ...   │ PROT     │ PROT     │ PROT     │
│ code     │          │          │       │ sector   │ sector   │ sector   │
│          │          │          │       │ (sec#10) │ (sec#11) │ (sec#12) │
└──────────┴──────────┴──────────┴───────┴──────────┴──────────┴──────────┘

Dungeon Master (example) on Track 0, Sector 247:
- 247 is written between sectors 9 and 1 on the next track rotation
- It appears AFTER sector 9 in the track but has sector number 247
- Normal TOS reads sectors 1-9 and stops
- Protection code seeks to track 0, reads until it finds sector 247
- The CRC of this "hidden" sector is checked
```

### How It Works at FDC Level

The WD1772 does not validate sector numbers — it only matches the sector number field against the sector register ($FF8602):

```
WD1772 sector matching:
1. FDC scans for IDAM (F5 sync)
2. ID field data: Track #, Side, Sector #, Sector Size
3. FDC compares sector # field with SECTOR register ($FF8602)
4. If match → data field follows, FDC sets DRQ
5. FDC does NOT validate sector number range

Standard TOS driver:
1. TOS sector register = 1 (or whatever TOS needs)
2. TOS only reads expected sector numbers (1-9)
3. TOS skips "out of range" sectors automatically

Protection code:
1. Protection sets SECTOR register to 10, 11, 12, etc.
2. Protection issues Read Sector command
3. FDC matches and returns data for the hidden sector
4. Protection verifies CRC/data
```

### Sector 247 Example (Dungeon Master)

Dungeon Master uses sector 247 as a protection trap. The sector is physically placed on the track after sector 9, but labeled as sector 247:

```
Physical track 0 layout:

  [Index] [Sec1:1B:BOOT] [Sec2:2B:BPB] [Sec3:3B:FAT] ...
  [Sec7:7B:PROT] [Sec8:8B:PROT] [Sec9:9B:data] [Sec247:247B:PROT] [next track]
                                                        ^^^^ hidden
                                                        sector
```

The protection code reads this by setting SECTOR = 247 and seeking to the right cylinder. The FDC accepts it because 247 is a valid byte value in the sector ID field.

## Type 3: Weak / Fuzzy Bits

### The Most Advanced Atari ST Protection

Weak bit protection was used most famously by **Skunksoft** (Skookumsoft) for *Dungeon Master* and *Chaos Strikes Back*. The protection was so effective that crackers could not bypass it for over a year after release.

### Principle

A "weak bit" is a magnetic flux transition written **precisely at the boundary** between a valid 1 and 0 transition. When read, the bit can return either value depending on:
- The state of the drive's preamplifier noise
- Thermal drift in the signal path
- Slight variations in RPM
- The exact head position on the media
- Age and condition of the disk

On the original disk, the bit is deterministic. On a copy, the bit flips randomly, breaking the protection verification.

### Mastering Weak Bits

```
Normal magnetic transition:
───────────────────────────────────
  Strong field (1): ████████████  → always reads as 1
  Weak field (0):   ░░░░░░░░░░    → always reads as 0
                     (clear gap)

Weak bit transition:
───────────────────────────────────────────
  1──────────────│──0   (at boundary)
                 ↑
          exactly at the transition boundary
          
  First read:  1 = 1 (field just above threshold)
  Second read: 1 = 0 (thermal drift changes threshold crossing)
  Third read:  1 = 1 (another random fluctuation)
  
  The bit "fuzzes" — it can be read as either 0 or 1
```

### Write Process for Weak Bits

1. **Write the sector normally** at double-density rate (500 kbps)
2. **Modify the PM3-encoded data** for specific bits near the MFM sync boundary
3. **Write with slightly reduced write current** so the magnetic field is marginal
4. **Use a specific flux transition density pattern** that creates the boundary condition

The exact technique writes a data bit at a position where the PM3 clock transition falls exactly on the flux transition boundary. The resulting magnetic field is neither clearly 1 nor clearly 0.

### Read-Back Behavior

```
  Normal bit (strong field):
  ┌─────────────────────────────────┐
  │  Read #1: 01010101...           │
  │  Read #2: 01010101...  (same)   │
  │  Read #3: 01010101...  (same)   │
  └─────────────────────────────────┘
  
  Weak bit (marginal field):
  ┌─────────────────────────────────┐
  │  Read #1: 01100101...           │ ← bit 2 is '1'
  │  Read #2: 01000101...  (bit 2 is '0')  ← FLIPS!
  │  Read #3: 01100101...  (bit 2 is '1')  ← FLIPS BACK!
  │  Read #4: 01000001...  (bit 2 is '0')
  └─────────────────────────────────┘
  The bit value changes on every read attempt
```

### Protection Verification Code

```assembly
; Weak bit verification (Dungeon Master style):
; Read a "fuzzy" sector multiple times and compare

  move.l  #5, d0        ; Read 5 times
  lea    buffer(a6), a0 ; Buffer for read data
  
verify_loop:
  ; Read the protected sector
  move.b  #weak_sector_num, $FF8802  ; Sector number
  move.b  #0, $FF8803               ; Side
  move.b  #$0C, $FF8800             ; Command: Read Sector
  wait_for_DRQ
  
  ; Store read data
  move.l  d1, (a0)+
  dbra   d0, verify_loop
  
  ; Compare all 5 reads
  ; If any two reads differ → confirmed fuzzy bit → ORIGINAL disk
  ; If all 5 reads identical → likely a copy → PROTECTION FAILED
  
  lea    buffer(a6), a1
  move.l  (a1)+, d2   ; First read
  move.l  (a1)+, d3   ; Second read
  cmp.l  d2, d3
  bne    original_disk  ; Bits changed! It's original!
  
  ; ... continue comparing all reads ...
  ; If all match → copy detected
```

### Fuzz Detection Result

- **Original disk**: Fuzzy bits read differently each time → protection passes
- **Standard floppy copy**: Bits are deterministic (FDC copies them as fixed 0 or 1) → protection fails because all reads are identical
- **Flaky bits** (another name): Same concept, slightly different mastering technique

## Type 4: Bit-Rate (Density) Variation

### Principle

The disk contains sectors written at **different bit rates** on the same physical track. The ST DD format uses 500 kbps, but standard PM3 encoding allows the drive to interpret the same track at either 250 kbps (single density) or 500 kbps (double density).

### How Bit-Rate Protection Works

```
Standard DD track (all sectors at 500 kbps):
  ┌──────────────────────────────────────┐
  │ Sector A [500k] │ Sector B [500k]  │
  └──────────────────────────────────────┘
  All sectors readable by any ST drive

Bit-rate protected track:
  ┌────────────────────────────────────────────────┐
  │ Sector A [500k DD] │ Sector B [250k SD] │ ... │
  └────────────────────────────────────────────────┘
  Sector B can only be read at 250 kbps density
  Standard ST at 500 kbps sees Sector B as "Record Not Found"
```

### PM3 Clock Recovery

The data separator in the WD1772 recovers the clock from MFM flux transitions. Different bit rates produce different spacing between transitions:

```
Double density (500 kbps, channel rate = 1 Mbps):
  Data:  1 0 1 1 0 0 1 0 1 1
  PM3:   11 01 11 11 01 01 11 01 11 11
  Time:  └─┬─┬─┴─┬─┬─┬─┴─┬─┬─┬─┬─┘
              1μs channel period

Single density (250 kbps, channel rate = 500 kbps):
  Data:  1 0 1 1 0 0 1 0 1 1
  PM3:   11 01 11 11 01 01 11 01 11 11
  Time:  └───┬───┴───┬───┬───┴───┬───┬───┴───┬───┬───┘
                  2μs channel period

When read back at 500 kbps, the 250 kbps sectors appear as:
  - Extra-wide transitions
  - Data separator resync failure
  - CRC mismatch or Record Not Found
```

### Mastering Bit-Rate Protection

1. Write some sectors at 500 kbps (DD) using normal PM3 encoding
2. Write other sectors at 250 kbps (SD) using the same PM3 data but double the channel period
3. The WD1772 uses write precompensation differently for each sector's bit rate
4. The mastering drive uses a special driver that can switch density mid-track

```
Mastering step-by-step:
1. Write Sector A at 500 kbps (normal DD)
2. Switch write head current for SD (lower write voltage)
3. Write Sector B at 250 kbps (SD) — same flux pattern but wider
4. Switch back to 500 kbps
5. Write remaining sectors at 500 kbps
```

### Detection on Copy

When a standard ST drive reads the protected track:
1. Sectors written at 500 kbps → read correctly
2. Sectors written at 250 kbps → read fails (data separator can't resync at wrong rate) → RNF (Record Not Found)
3. Protection checks for RNF → if no RNF → drive read wrong density → copy detected!

## Type 5: Sector Skew Protection

### Principle

Sectors are written in a **non-standard interleave order**. The TOS expects sectors in sequence (1, 2, 3... 9) on each physical track, but protection writes them differently.

### Standard vs Protected Skew

```
Standard TOS format (physical):
  Track 0: [1][2][3][4][5][6][7][8][9]
  
Protection using sector skew:
  Track 0: [1][5][9][4][8][3][7][2][6]
  
  TOS driver reads sector: 1, then waits for next sector
  Next physical sector after 1 is sector 5
  TOS sets sector register to 2, but physical sector 3 is at position [3]
  TOS never reaches sector [3] at position [3] — it's at position [3] in the skew
  But wait — TOS reads sector 1, then asks for sector 2
  Sector 2 is at physical position [9] — TOS misses it!
  
  Protection reads all sectors by specifying sector numbers explicitly.
  The protection reads sectors in any order, so skew doesn't matter.
  Standard TOS reads sectors sequentially — skew breaks the chain.
```

### Skew Pattern Techniques

Some protections use **variable skew** that changes per track:

```
Track 0:  [1][5][9][4][8][3][7][2][6]  (skew factor 4)
Track 1:  [3][7][2][6][1][5][9][4][8]  (skew factor 3)
Track 2:  [8][3][7][2][6][1][5][9][4]  (skew factor 5)
...

The protection code knows the exact skew pattern per track.
TOS reads them in physical order 1, 2, 3... which never matches.
```

### Counter-Protection Skew Detection

Protection code reads all sectors and verifies the skew pattern:

```assembly
; Read track with non-standard skew
; Expected order: 1, 5, 9, 4, 8, 3, 7, 2, 6

  lea    skew_table(a6), a0  ; Pointer to expected sector order
  move.b  #9, d0              ; 9 sectors per track
  
read_skew_loop:
  move.b  (a0)+, d1           ; Next expected sector number
  move.b  d1, $FF8802         ; Set FDC sector register
  move.b  #$0C, $FF8800       ; Command: Read Sector
  wait_for_DRQ
  
  ; Verify sector data matches
  bne    skew_mismatch       ; Read failed → copy detected
  dbra   d0, read_skew_loop  ; Continue for all 9 sectors
```

## Type 6: Data Under Index

### Principle

Data is written **between the index pulse and the first sector's IDAM** on each track. Standard drives skip this area because they wait for the first IDAM marker. Protection code reads this "hidden" data before the TOS driver starts.

### Track Layout with Data Under Index

```
Standard DD track (no data under index):
  [Index gap ~1280B][IDAM][Sec1][IDAM][Sec2]...[IDAM][Sec9]
  └────────── Gap (no data) ──────────┘

Track with data under index:
  [Index][Hidden Data ~512B][IDAM with IAM][Sec1][Sec2]...[Sec9]
   ^^^^   ^^^^^^^^^^^^^^^    ┌── IDAM ──┐
   data    (before first      │ F6 01
   written  sector)          │ (IAM byte)
                             │
    TOS ignores this area!
```

### Writing Data Under Index

```
Write process during mastering:
1. Write the "hidden" data immediately after the index pulse
2. This is a complete sector header + data field written BEFORE the normal first sector
3. The IAM (F6 01) is written to mark the start of the hidden data
4. Standard format (TOS FORMAT) doesn't write here

Reading data under index:
1. Protection code issues "Read Address" or "Read Track" command
2. FDC reads the index pulse and immediately captures the next sector
3. The FDC returns data that TOS never reads
```

### FDC Command for Data Under Index

The "Read Address" command (1100 0h E 0 0) reads the next address mark and returns the 6-byte ID field:

```assembly
; Read Address command type III
; Returns track, side, sector, size in data registers

  move.b  #$60, $FF8800    ; Command: Read Address
  wait_for_DRQ
  move.b  $FF8603, d0      ; Track number returned
  move.b  $FF8603, d1      ; Side number returned
  move.b  $FF8603, d2      ; Sector number returned
  move.b  $FF8603, d3      ; Sector size returned
  move.b  $FF8603, d4      ; CRC high
  move.b  $FF8603, d5      ; CRC low
```

## Type 7: Multiple Density on Same Track

### Principle

Different sectors on the same track are written at different densities — a combination of Types 1, 3, and 4.

### Example: Skunksoft Protection (Dungeon Master)

Skunksoft used a combination:

```
Track 0 on Dungeon Master master:

  [Sec1: 9B @500k DD]  [Sec2: BOOT CODE]
  [Sec3-6: 256B @500k DD] [Sec7: 512B @500k DD + weak bits!]
  [Sec247: 512B @500k DD (non-standard sector #)]
  [Sec8-9: 512B @500k DD + bit-rate variation!]

  Bit-rate variation: some bits in sectors 8-9 written at slightly
  different MFM timing (effective "sub-density" shift)
  
  Weak bits: specific bits in sector 7 written at boundary
  between valid 1 and 0 transition
```

## Type 8: WD1772 FDC Command Tricks

### Principle

The WD1772 has undocumented or unexpected behavior during certain command sequences that can be exploited for protection.

### Multi-Sector Command

The WD1772 Read Sector command has a "Multiple Sectors" bit (bit 4 of the command byte). When set, the FDC reads multiple consecutive sectors until interrupted.

```
Command byte bits: [M=1][h=1][E=1][0][0]  → 0xE6

  Standard use: Read sectors 1 through N in one rotation
  Protection trick: Read more sectors than exist on the track
  The FDC will try to read "sector 10" on a 9-sector disk
  This may return garbage or the same data as sector 1
  Protection checks for this behavior
```

### Motor-On Flag Exploitation

Using the motor-on flag (bit 3) during sector reads:

```
h=1 on a Read Sector command:
1. FDC asserts motor-on
2. FDC waits for 6 index pulses before reading
3. During this wait, RPM stabilizes
4. Protection checks the motor status bit in the status register
5. A copy drive might not support the motor-on behavior
```

### Force Interrupt Timing

The Force Interrupt command ($D8, $D0) can be used to wake up a stalling FDC. Timing-sensitive protection:

```assembly
; Force interrupt timing check
  move.b  #$D8, $FF8600   ; Force Interrupt
  ; MUST wait 32 microseconds before next operation
  ; If next操作 happens too fast, FDC hangs permanently
  ; Protection verifies FDC responds correctly
  wait_32us
  
  move.b  #$D0, $FF8600   ; Clear Force Interrupt
  ; Check if FDC status is valid
  move.b  $FF8600, d0
  andi.b  #$81, d0         ; Motor on + Busy bits
  bne    fdc_working       ; FDC is functioning normally
  
; If FDC is dead here, protection detects it as a "copy driver"
```

## Type 9: Custom Sector Size

### Principle

Sectors are written with a non-standard sector size field in the ID field. Standard TOS only recognizes sizes $00 (128B), $01 (256B), $02 (512B), and $03 (1024B). Some protections use other values.

### Non-Standard Sector Size

```
ID field sector size byte:
  $00 = 128 bytes
  $01 = 256 bytes (STOS format)
  $02 = 512 bytes (standard TOS)
  $03 = 1024 bytes (HD format)
  $04-$FF = ??? (unrecognized by TOS)

Some protections write sectors with size field = $FF
TOS will read 256 bytes (defaulting to $02 = 512) but the
actual data is only $05 bytes (5 bytes), causing a premature
EOF in the data field. Protection checks for this behavior.
```

## Copy Protection Bypass Techniques

### Kryoflux / SuperCard Pro

These hardware tools read the raw flux transitions on the disk, capturing weak bits and bit-rate variations that standard FDC commands cannot:

```
Kryoflux workflow:
1. Connect Kryoflux between ST floppy port
2. Record raw flux transitions for each track (Kryoflux .raw format)
3. Kryoflux captures:
   - Exact timing of each flux transition (sub-microsecond precision)
   - Weak bit boundary positions
   - Bit-rate variations (sub-density shifts)
   - Data under index areas
   - All MFM encoding details
4. Play back flux pattern to a writable drive (Greaseweazle or similar)
5. Result: exact magnetic replica of the protected disk
```

### Stealing the Key

Many protections only verify the disk **once** at startup. After verification:
1. Read the encrypted or decrypted data sector from the protected track
2. Patch the protection verification code to bypass re-checks
3. Dump the decrypted game data to a clean disk
4. Result: playable copy that no longer needs the original disk

### FDC Emulation Bypass

Emulators like Hatari or MiSTer implement FDC emulation. Protection verification code that reads sectors in unusual ways may fail to detect protection in emulation:

```
Atari ST protection verification code vs. emulator:

  Real hardware:
  - FDC status bit 3 (CRC error) set after reading bad sector ✓
  - FDC status bit 4 (RNF) set after missing sector ✓
  - Weak bits return different values each read ✓
  
  Emulator (naive):
  - FDC returns clean data (no CRC errors) ✗
  - Weak bits are deterministic ✗
  - Sector 247 doesn't exist in .ST image ✗
  
  → Protection fails → emulation works → disk can be dumped normally
```

### Blitz Copier / Synchro Express

These were hardware duplicators that connected two Atari ST floppy drives directly. They worked by:

```
Blitz Copier duplication:
1. Master disk in Drive A (protected)
2. Blank disk in Drive B
3. Atari ST controls both drives simultaneously
4. Protection code tries to verify sector → FDC returns error
5. Blitz Copier intercepts the FDC data
6. Reads the "error" data directly from the master MFM output
7. Writes raw MFM data to the blank disk
8. Result: exact copy that passes protection verification
```

## Documented Protection Schemes

### Dungeon Master / Chaos Strikes Back (Skunksoft)

| Feature | Detail |
|-----|- |
| Weak bits | Sector 7 on Track 0 (multiple fuzzy bits) |
| Non-standard sector | Sector 247 on Track 0 |
| Bit-rate variation | Some sectors written at non-standard MFM timing |
| Data under index | Hidden data in the IAM gap area |
| Verification reads | Multiple reads of fuzzy sectors, compares results |
| Crack bypass | Kryoflux → Greaseweazle (exact flux copy) |

### Starshield (Starshield Corp.)

| Feature | Detail |
|-----|- |
| Technique | Sector skew + CRC error injection |
| Sector order | Non-standard interleaving per track |
| Bad sectors | Deliberate CRC errors in data fields |
| Bypass | Sector-by-sector dump with ST-Dump or similar |

### Rob Northen Protection (RNP)

Rob Northen wrote protection utilities that worked on multiple platforms including Atari ST:

| Protection Name | Technique |
|-----|- |
| RNP-1 | Sector skipping (read alternate sector numbers) |
| RNP-2 | CRC verification of specific sectors |
| RNP-3 | Custom sector formats on standard 9-sector layout |
| RNP-4 | Bad sector verification |
| RNP-5 | Bit density (PM3 timing) manipulation |
| RNP-6 | Multiple density per track |
| RNP-7 | Sector size manipulation |

## Detection and Analysis Tools

### Atari ST Diagnostic Tools

| Tool | Purpose |
|-----|- |
| TRAKREAD (START disk) | Read track and dump raw FDC data |
| Disk Duplicator | Detect different sectors per drive |
| Copy Girl / Copy II-ST | Identify protection type |
| ST-Dump | Sector-by-sector binary dump |
| AntiBitos | Detect and bypass protection |
| STFormat | Low-level format (create protection) |

### Modern Analysis Tools

| Tool | Platform | Purpose |
|-----|-|-|
| Greaseweazle | PC/HID | Read/write raw MFM flux, bypass all protections |
| Kryoflux | PC/USB | Raw flux capture (.raw format) |
| SuperCard Pro | PC/SCSI | Raw MFM capture and recreation |
| HxC Floppy Emulator | PC/USB | Emulate floppies with .STX and .IPF images |
| STX Converter | PC | Convert .STX to .ST for emulation |

## Protection Comparison Summary

| Protection | Original → Copy | Required Tool | Effectiveness |
|-|-|-|-|
| Bad sectors | ✓ (copy has clean CRC) | ST-Dump (bypassable) | Low |
| Non-standard sector | ✓ (copy has no sector 247) | Sector-by-sector dump | Low |
| Weak bits | ✗ (copy has fixed bits) | Kryoflux / Greaseweazle | High |
| Bit-rate variation | ✗ (copy at one density) | Multi-density writer | Very high |
| Sector skew | Partial (TOS can't read) | STFormat / sector dump | Medium |
| Data under index | ✓ (copy has data in IAM area) | Read Track command | Medium |
| Mixed approach (DM) | ✗ | Kryoflux + Greaseweazle | Very high |
