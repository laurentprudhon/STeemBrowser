# Atari ST Floppy Disk Software: Comprehensive Technical Analysis

> **Source**: [Info-Coach - Atari ST FD Software](https://info-coach.fr/atari/software/FD-Soft.php)  
> **Author**: DrCoolZic (Jean Louis-Guérin)  
> **Last Updated**: January 15, 2015  
> **Analysis Date**: May 30, 2026

---

## Table of Contents

1. [Overview](#overview)
2. [Low-Level Formatting](#low-level-formatting)
   - [Atari vs PC vs ISO Double Density Formats](#atari-vs-pc-vs-iso-dd-formats)
   - [Atari Standard Double Density Format](#atari-standard-dd-format)
   - [Multi-Sector Formats (9-10-11 Sectors)](#multi-sector-formats)
   - [Variable Sector Size Formats](#variable-sector-size-formats)
3. [Track and Sector Architecture](#track-and-sector-architecture)
   - [Track Description](#track-description)
   - [Sector ID Segment](#sector-id-segment)
   - [Sector DATA Segment](#sector-data-segment)
   - [Write Splice Mechanisms](#write-splice-mechanisms)
4. [High-Level Formatting](#high-level-formatting)
   - [Disk Preparation Process](#disk-preparation-process)
   - [Boot Sector (BS)](#boot-sector-bs)
   - [Directory Structure](#directory-structure)
   - [File Allocation Table (FAT)](#file-allocation-table-fat)
5. [Technical Deep Dive](#technical-deep-dive)
   - [MFM Encoding and Timing](#mfm-encoding-and-timing)
   - [Gap Analysis and Optimization](#gap-analysis-and-optimization)
   - [Interleaving and Performance](#interleaving-and-performance)
   - [Error Handling and CRC](#error-handling-and-crc)
6. [Protection Mechanisms](#protection-mechanisms)
7. [Compatibility Considerations](#compatibility-considerations)
8. [Practical Applications](#practical-applications)
9. [Conclusion](#conclusion)

---

## Overview

The Atari ST floppy disk system represents a sophisticated implementation of magnetic storage technology for the 16-bit era. Unlike modern solid-state storage, floppy disks required precise physical formatting and logical organization to ensure reliable data storage and retrieval. This document provides an exhaustive technical analysis of how the Atari ST's floppy disk software works at both the low-level (physical) and high-level (logical) layers.

The Atari ST uses a **Western Digital WD1772 Floppy Disk Controller (FDC)** to manage 3.5" (90mm) double-density floppy disks. The system supports various formatting schemes, with the standard Atari format being a modified version of the **ISO Double Density Format**, omitting the Index Address Mark (IAM) byte present in IBM-compatible formats.

---

## Low-Level Formatting

### Concept

Low-level formatting is the process of creating the physical structures on a floppy disk that will hold data. This involves:

1. **Track Creation**: Dividing the disk surface into concentric circles (tracks)
2. **Sector Marking**: Dividing each track into discrete sectors
3. **Gap Allocation**: Creating buffer zones between sectors to account for mechanical tolerances
4. **Preamble/Synchronization Fields**: Adding patterns to help the controller synchronize with the data

Once low-level formatted, the locations of tracks and sectors are permanently fixed on the disk surface.

### Atari vs PC vs ISO DD Formats

The WD1772 FDC was designed to support two primary double-density formats:

| Format | Description | IAM Presence | Atari Compatibility |
|--------|-------------|--------------|---------------------|
| **IBM System 34** | Standard PC 720K format | Yes | Readable by Atari |
| **ISO Double Density** | International standard | Yes | Readable by Atari |
| **Atari Standard** | TOS default format | **No** | **Not readable by PC** |

**Key Difference**: The Atari standard format **omits the Index Address Mark (IAM) byte and its associated GAP** before the first IDAM sector. This makes Atari-formatted disks **incompatible with standard PC drives**, while PC-formatted disks remain readable on Atari systems.

**Implications**:
- Atari can read both IBM System 34 and ISO formats
- PC systems cannot read Atari-formatted disks
- This asymmetry was a deliberate design choice by Atari

### Atari Standard Double Density Format

The standard Atari double-density format for 3.5" disks uses:
- **9 sectors per track**
- **512 bytes per sector**
- **80 tracks per side** (for double-sided disks)
- **Modified Frequency Modulation (MFM) encoding**

#### Track Layout Diagram

```
[Gap 1: Post Index] -> [Sector 1] -> [Gap 5: Pre Index] -> [Index Pulse]
```

Each sector consists of:
```
[Gap 2: Pre ID] -> [ID Segment] -> [Gap 3a: Post ID] -> [Gap 3b: Pre Data] -> [DATA Segment] -> [Gap 4: Post Data]
```

#### Gap Structure (Standard Values for 9 Sectors)

| Gap | Name | Standard Value | Minimum Value (WD1772) | Purpose |
|-----|------|----------------|------------------------|---------|
| 1 | Post Index | 60 × $4E | 32 × $4E | Buffer after index pulse |
| 2 | Pre ID | 12 × $00 + 3 × $A1 | 8 × $00 + 3 × $A1 | Synchronization before ID |
| 3a | Post ID | 22 × $4E | 22 × $4E | Buffer after ID field |
| 3b | Pre Data | 12 × $00 + 3 × $A1 | 12 × $00 + 3 × $A1 | Synchronization before data |
| 4 | Post Data | 40 × $4E | 24 × $4E | Buffer after data field |
| 5 | Pre Index | ~664 × $4E | 16 × $4E | Buffer before index pulse |

**Calculations**:
- **Standard Record Gap** (Gap 2 + 3a + 3b + 4) = 92 bytes
- **Minimum Record Gap** = 72 bytes
- **Standard Record Length** = 92 + 7 (ID) + 515 (DATA+CRC) = **614 bytes**
- **Minimum Record Length** = 72 + 7 + 515 = **594 bytes**

#### Track Capacity Analysis

- **Disk Rotation**: 300 RPM = 5 rotations per second
- **Track Time**: 200ms per rotation
- **MFM Cell Length**: 4μs
- **Total Cells per Track**: 50,000
- **Approximate Bytes per Track**: ~6,250 bytes

For 9 sectors: 9 × 614 = 5,526 bytes + gaps = ~6,250 bytes (matches track capacity)

### Multi-Sector Formats

The Atari ST can support different numbers of sectors per track by adjusting gap sizes:

#### 9 Sectors/Track
- Gap 1: 60 × $4E
- Gap 2: 12 × $00 + 3 × $A1
- Gap 3a: 22 × $4E
- Gap 3b: 12 × $00 + 3 × $A1
- Gap 4: 40 × $4E
- Gap 5: 664 × $4E
- **Record Length**: 614 bytes
- **Total Track**: 6,250 bytes

#### 10 Sectors/Track
- Gap 1: 60 × $4E
- Gap 2: 12 × $00 + 3 × $A1
- Gap 3a: 22 × $4E
- Gap 3b: 12 × $00 + 3 × $A1
- Gap 4: 40 × $4E
- Gap 5: 50 × $4E
- **Record Length**: 614 bytes
- **Total Track**: 6,250 bytes

#### 11 Sectors/Track (Non-Standard)
- Gap 1: **10 × $4E** (reduced)
- Gap 2: **3 × $00 + 3 × $A1** (reduced)
- Gap 3a: 22 × $4E
- Gap 3b: 12 × $00 + 3 × $A1
- Gap 4: **1 × $4E** (severely reduced)
- Gap 5: 20 × $4E
- **Record Length**: 566 bytes
- **Total Track**: 6,250 bytes

**Critical Analysis of 11-Sector Format**:

1. **Minimum WD1772 Requirements**:
   - Minimum track length calculation: 32 (Gap1) + 11 × 594 (min record) + 16 (Gap5) = **6,582 bytes**
   - **Exceeds track capacity by 332 bytes**
   - Requires **32 bytes reduction per sector**

2. **Gap Reduction Strategy**:
   - Gap 2 reduced from 15 to 6 bytes (-9 bytes)
   - Gap 4 reduced from 40 to 1 byte (-39 bytes)
   - Total reduction: 48 bytes per sector (exceeds required 32 bytes)

3. **Technical Implications**:
   - **Gap 3a & 3b must NOT be shortened** - Critical for FDC timing between ID and DATA blocks
   - **Gap 4 + Gap 2 = 7 bytes** between DATA and next ID
   - **Insufficient time** for FDC to read next sector on-the-fly
   - Sector must be read on **next disk rotation**
   - **Performance Impact**: Significant due to rotational latency
   - **Write Calibration Critical**: Risk of data collision with next ID block
   - **Protection Mechanism**: Such tracks are often marked as "read-only"

4. **Physical Constraints**:
   - Outer tracks (track 0) have higher success rate due to lower bit density
   - Inner tracks (track 79) are more challenging due to higher bit density
   - Requires floppy drive with **≤1% RPM deviation** from 300 RPM

### Variable Sector Size Formats

The Atari ST supports various sector sizes with corresponding gap adjustments:

| Sectors/Track | Sector Size | Gap 1 | Gap 2 | Gap 3a | Gap 3b | Gap 4 | Gap 5 | Record Length | Total Track |
|---------------|-------------|-------|-------|-------|-------|-------|-------|----------------|-------------|
| 29 | 128 bytes | 40 | 10+3 | 22 | 12+3 | 25 | 73 | 213 | 6,250 |
| 18 | 256 bytes | 42 | 11+3 | 22 | 12+3 | 26 | 76 | 343 | 6,250 |
| 9 | 512 bytes | 60 | 12+3 | 22 | 12+3 | 40 | 664 | 614 | 6,250 |
| 5 | 1024 bytes | 60 | 40+3 | 22 | 12+3 | 40 | 480 | 1,154 | 6,250 |

**Note on Interleaving**: While not required for double-density formats (unlike single-density), sectors can be written in any order. Interleaving can improve performance by reducing head movement delays.

---

## Track and Sector Architecture

### Track Description

A complete track on an Atari ST floppy disk consists of:

1. **Index Pulse**: Physical marker indicating the start of a track
2. **Gap 1 (Post Index)**: Initial buffer zone after the index pulse
3. **Sector Sequence**: Repeated for each sector on the track:
   - Gap 2 (Pre ID)
   - ID Segment
   - Gap 3a (Post ID)
   - Gap 3b (Pre Data)
   - DATA Segment
   - Gap 4 (Post Data)
4. **Gap 5 (Pre Index)**: Final buffer zone before the next index pulse

### Sector ID Segment

The ID segment is critical for sector identification and synchronization:

```
┌─────────────────────────────────────────────────────────────┐
│                    ID SEGMENT (7 bytes total)                   │
├─────────────────┬─────────────────┬─────────────────┬────────┤
│  ID PREAMBLE     │   ID FIELD       │   ID CRC         │ POST   │
│  (12 bytes)      │   (4 bytes)      │   (2 bytes)      │ AMBLE  │
└─────────────────┴─────────────────┴─────────────────┴────────┘
```

#### ID Preamble
- **PLL Synch Field**: 12 bytes of repetitive clocked data (typically all zeros in NRZ, encoded as 1010... in MFM)
  - Purpose: Allows Phase-Locked Loop (PLL) circuitry to lock onto the incoming data rate
  - **Read Gate Activation**: Signal goes active during this field, indicating data pattern lock
- **ID Synch Field**: Contains a **missing clock code violation** (MFM encoding anomaly)
  - First decoded byte after preamble that is not all zeros
  - Used for **byte alignment**
  - Typically 3 bytes in double-density format
  - **Synch Mark Detection**: Circuitry detects this violation to identify start of ID or DATA field

#### ID Field
- **ID Address Mark (IDAM)**: Required on soft-sectored drives (no hardware sector pulses)
  - Marks the beginning of a sector
  - Standard value: $A1 $A1 $FE (for IDAM)
- **ID Content Field** (4 bytes):
  - Byte 0: Track number (0-79 for standard disks)
  - Byte 1: Sector number (1-11 depending on format)
  - Byte 2: Head number (0 or 1 for double-sided)
  - Byte 3: Reserved/Format identifier
- **ID CRC Field** (2 bytes):
  - Cyclic Redundancy Check using **CRC-CCITT polynomial**
  - Protects the integrity of the ID field
  - Standard polynomial: x¹⁶ + x¹² + x⁵ + 1

#### ID Postamble
- **Function**: Provides time for:
  1. Disk controller to interpret ID field data
  2. Write splicing between ID and DATA segments
  3. PLL circuit to re-lock before DATA segment
  4. Read/write circuitry mode switching (read → write)
- **Typical Size**: 22 bytes of $4E (Gap 3a)

### Sector DATA Segment

The DATA segment contains the actual user data:

```
┌─────────────────────────────────────────────────────────────┐
│                   DATA SEGMENT (515 bytes total)                │
├─────────────────┬─────────────────┬─────────────────┬────────┤
│ DATA PREAMBLE    │   DATA FIELD     │   DATA CRC       │ POST   │
│ (12+3 bytes)     │ (512 bytes)      │ (2 bytes)        │ AMBLE  │
└─────────────────┴─────────────────┴─────────────────┴────────┘
```

#### Data Preamble Field
- **PLL Synch Field**: 12 bytes of $00 (NRZ) → 1010... (MFM)
  - **Critical Function**: Allows PLL to lock onto DATA segment data rate
  - **Why Separate Preamble?** ID and DATA segments are written at different times
  - **Motor Speed Variations**: Cause slight differences in data rates between ID and DATA
  - **PLL Adjustment**: Must adjust frequency and phase before preamble ends
- **Data Synch Field**: Similar to ID synch field, contains code violation
  - Standard: 3 bytes of $A1 followed by $FB (for DATA Address Mark)

#### Data Field
- **Data Address Mark (DAM)**: Similar to IDAM but for data
  - Standard value: $A1 $A1 $FB (for DAM)
  - Marks start of actual data content
- **Data Content Field**: 128-1024 bytes (typically 512 for Atari ST)
  - Contains user file data or system information
- **DATA CRC** (2 bytes):
  - CRC-CCITT protection for the data field
  - Generated when writing, checked when reading
  - Detects data corruption

#### Data Postamble Field
- **Function**: Same as ID Postamble:
  1. Prevents overlap with next sector's ID segment
  2. Accounts for write splicing variations
  3. Provides buffer for motor speed fluctuations
- **Typical Size**: 40 bytes of $4E (Gap 4)
- **Note**: Only written during disk formatting, not during normal data writes

### Write Splice Mechanisms

#### Sector Write Splice

**Definition**: The physical variation in where the first flux change occurs when writing a sector's data segment.

**Causes**:
1. **Rotational Speed Variations**: Slight changes in disk rotation speed
2. **Mechanical Tolerances**: Head positioning and timing variations
3. **Electrical Delays**: Circuit switching times

**Location**: Beginning of the DATA preamble

**Process**:
1. Write gate turns ON at start of DATA preamble
2. First flux change position varies slightly each write
3. Write splice area absorbs these variations

**Requirements**:
- Must provide enough space for worst-case variations
- Must not overlap with ID segment or next sector

#### Track Write Splice

**Definition**: The challenge of perfectly joining the end of a track write with the beginning.

**Process**:
1. Start writing at index mark
2. Write continuously for 200ms (one full rotation)
3. **Problem**: Disk never rotates at exactly 300 RPM

**Outcomes**:
- **Leftover Noise**: If rotation was slower than 300 RPM
- **Overwriting**: If rotation was faster than 300 RPM
- **Solution**: Write splice area at index absorbs these variations

---

## High-Level Formatting

### Disk Preparation Process

High-level formatting creates the logical structures that allow the TOS to manage files:

1. **Low-Level Formatting** (Prerequisite):
   - Creates physical track and sector structures
   - Defines disk geometry (tracks, sectors, sides)

2. **High-Level Formatting**:
   - Creates **Boot Sector** (Track 0, Side 0, Sector 1)
   - Creates **File Allocation Tables (FATs)**
   - Creates **Root Directory**
   - Initializes file system metadata

### Boot Sector (BS)

**Location**: Always at Track 0, Side 0, Sector 1

**Purpose**:
1. Identify disk parameters to TOS
2. Store BIOS Parameter Block (BPB)
3. Contain bootstrap loader for executable disks
4. Provide disk serial number for identification

#### Boot Sector Structure

```
Offset  Size    Name        Description
─────  ─────  ─────────  ─────────────────────────────────────────
$00    2       BRA         680x0 BRA.S instruction to bootstrap code
$02    6       OEM         Reserved; TOS loader places "Loader" here
$08    4       SERIAL      Disk serial number (low 24 bits)
$0B    2       BPS         Bytes per sector (Intel format, usually $0200)
$0D    1       SPC         Sectors per cluster (power of 2, usually 2)
$0E    2       RESSEC      Reserved sectors before first FAT
$10    1       NFATS       Number of FATs (usually 2)
$11    2       NDIRS       Number of root directory entries
$13    2       NSECTS      Total sectors on disk
$15    1       MEDIA       Media descriptor (0xF8 for HD, unused on Atari)
$16    2       SPF         Sectors per FAT
$18    2       SPT         Sectors per track (usually 9)
$1A    2       NHEADS      Number of heads (1 or 2)
$1C    2       NHID        Hidden sectors (unused by Atari)
$1E    2       EXECFLAG    Loaded into cmdload; indicates if command.prg should run
$20    2       LDMODE      Load mode: 0=load file, non-zero=load sectors
$22    2       SSECT       Starting logical sector for boot (if LDMODE≠0)
$24    2       SECTCNT     Number of sectors to load for boot
$26    2       LDAADDR     Memory address for boot program load
$2A    2       FATBUF      Address for FAT and catalog sectors
$2E    11      FNAME       Boot file name (8.3 format)
$39    2       RESERVED    Reserved
$3A    452     BOOTIT      Bootstrap program code
$1FE   2       CHECKSUM    Boot sector checksum
```

#### Boot Sector Details

**Byte Order**: All multi-byte values use **Intel format** (little-endian) except where noted.

**Bootable Disk Identification**:
1. Bytes at $02-$07 must contain "Loader"
2. Sum of entire boot sector + CHECKSUM (Motorola format) = $1234

**Boot Process (4 Stages)**:
1. Boot sector loaded and bootstrap code executed
2. FAT and root directory loaded; loader searches for file specified in FNAME
3. Program image loaded (typically starting at $40000)
4. Loaded program executed

**BPB (Bios Parameter Block)**: Grayed areas in the structure above contain BPB data used by TOS BIOS.

### Directory Structure

**Root Directory**:
- **Location**: Immediately after FATs
  - Single-sided: Side 0, Track 1, Sector 3
  - Double-sided: Side 1, Track 0, Sector 3
- **Size**: 7 sectors (for standard Atari disks)
- **Entry Count**: Defined by NDIRS field in BPB

**Directory Entry Structure** (32 bytes per entry):

```
Offset  Size    Name        Description
─────  ─────  ─────────  ─────────────────────────────────────────
$00    8       FNAME       File/directory name (padded with spaces)
$08    3       FEXT        File extension (padded with spaces)
$0B    1       ATTRIB      File attributes
$0C    10      RES         Reserved (unused)
$16    2       FTIME       Creation/last update time
$18    2       FDATE       Creation/last update date
$1A    2       SCLUSTER    Starting cluster (FAT index)
$1C    4       FSIZE       File size in bytes
```

#### File Name Field (FNAME)

**Special Values**:
- `$00`: Entry never used (end of directory marker)
- `$05`: First character is actually $E5 (deleted file indicator)
- `$2E`: Entry is "." (current directory) or ".." (parent directory)
- `$E5`: File/directory has been deleted

**Directory Aliases**:
- "." (single period): Points to current directory's starting cluster
- ".." (double period): Points to parent directory's starting cluster (0 for root)

#### File Attributes (ATTRIB)

| Bit | Mask | Description |
|-----|------|-------------|
| 0 | $01 | Read-only file |
| 1 | $02 | Hidden file/directory |
| 2 | $04 | System file/directory |
| 3 | $08 | Volume label (only in root directory) |
| 4 | $10 | Directory |
| 5 | $20 | New or modified file |
| 6-7 | $C0 | Reserved (must be 0) |

#### Time and Date Fields

**FTIME (2 bytes)**:
- Bits 0-4: Two-second intervals (0-29)
- Bits 5-10: Minutes (0-59)
- Bits 11-15: Hours (0-23)

**FDATE (2 bytes)**:
- Bits 0-4: Day (1-31)
- Bits 5-8: Month (1-12)
- Bits 9-15: Year (relative to 1980, so 0=1980, 1=1981, etc.)

### File Allocation Table (FAT)

**Purpose**: Track which clusters are allocated to which files/directories.

**Location**: Immediately follows boot sector and any reserved sectors.

**FAT Type**:
- **12-bit FAT**: Used for Atari diskettes (always < 4086 clusters)
- 16-bit FAT: Used for larger storage (not used on standard Atari floppies)

#### FAT Structure (12-bit)

- **Entry Size**: 12 bits per cluster
- **Total FAT Size**: (Number of clusters × 1.5) bytes
- **Number of FATs**: Typically 2 (primary and backup)

**FAT Entry Values**:

| Value | Meaning |
|-------|---------|
| $000 | Available cluster |
| $002-$FEF | Index of next cluster in file/directory |
| $FF0-$FF6 | Reserved |
| $FF7 | Bad sector in cluster (do not use) |
| $FF8-$FFF | Last cluster of file/directory (usually $FFF) |

**Note**: $001 is not used as it corresponds to the second reserved entry.

#### FAT Operation

1. **File Creation**:
   - TOS allocates clusters as needed
   - Links clusters together in FAT chain
   - Updates directory entry with starting cluster

2. **File Reading**:
   - Start at SCLUSTER from directory entry
   - Follow FAT chain to find all clusters
   - Read data from each cluster sequentially

3. **File Deletion**:
   - First byte of filename set to $E5
   - All FAT entries for file's clusters set to $000 (available)
   - **Data not erased** - only metadata updated (allows undelete)

#### Example FAT Chain

Consider a file using clusters 2, 3, 5, 6:

```
FAT Entry 2: $003 (points to cluster 3)
FAT Entry 3: $005 (points to cluster 5)
FAT Entry 4: $FF7 (bad sector - unused)
FAT Entry 5: $006 (points to cluster 6)
FAT Entry 6: $FFF (last cluster)
FAT Entry 7: $000 (available)
```

**Cluster Size**: Defined by SPC (Sectors Per Cluster) in BPB. For Atari:
- Typically 2 sectors per cluster = 1,024 bytes
- Must be a power of 2

---

## Technical Deep Dive

### MFM Encoding and Timing

**Modified Frequency Modulation (MFM)**:
- **Clock Rate**: 250 kHz for double-density
- **Data Rate**: 500 kbps (2 bits per clock cycle)
- **Cell Time**: 4μs per bit cell
- **Encoding Rules**:
  - 0 bit: No flux transition (if previous was 1) or transition (if previous was 0)
  - 1 bit: Always flux transition
  - **Clock bits**: Inserted between data bits for synchronization

**Timing Constraints**:
- **Track Time**: 200ms = 50,000 cells = ~6,250 bytes
- **Bit Density**: Increases toward inner tracks
- **PLL Lock Time**: Must lock within preamble period

### Gap Analysis and Optimization

**Gap Functions**:
1. **Synchronization**: Allow PLL to lock onto data rate
2. **Timing Margin**: Account for mechanical variations
3. **Mode Switching**: Time for read/write circuitry to switch modes
4. **Write Splicing**: Absorb variations in write start position

**Optimization Strategies**:
- **Minimize Gaps**: Increase data capacity (e.g., 11-sector format)
- **Risk**: Reduced reliability, potential data corruption
- **Trade-off**: Capacity vs. compatibility vs. reliability

**Critical Gaps**:
- **Gap 3a + 3b**: Must NOT be shortened - critical for FDC timing
- **Gap 2 + 4**: Can be reduced but impacts performance

### Interleaving and Performance

**Interleaving Concept**:
- Sectors written in non-sequential order (e.g., 1, 3, 5, 7, 2, 4, 6, 8)
- Allows head to move to next sector while disk rotates
- Reduces rotational latency

**Atari ST Implementation**:
- **Not required** for double-density (unlike single-density)
- **Optional**: Can improve performance for sequential access
- **Trade-off**: More complex software, potential compatibility issues

**Performance Impact**:
- **Without Interleaving**: Head must wait for next sector to rotate under it
- **With Interleaving**: Head can move to next sector immediately
- **Optimal Interleave Factor**: Depends on disk rotation speed and head movement time

### Error Handling and CRC

**CRC-CCITT Polynomial**: x¹⁶ + x¹² + x⁵ + 1
- **Initial Value**: $FFFF
- **Algorithm**: Standard CRC-16 implementation
- **Coverage**: Both ID and DATA fields protected

**Error Detection**:
1. **ID CRC**: Verified when reading sector ID
2. **DATA CRC**: Verified when reading sector data
3. **Action on Error**:
   - Retry read operation
   - Mark sector as bad (FAT entry = $FF7)
   - Report error to TOS

**Error Recovery**:
- **Multiple Retries**: TOS attempts multiple reads
- **Alternate Sectors**: Some formats support sector sparing
- **FAT Backup**: Second FAT copy allows recovery from FAT corruption

---

## Protection Mechanisms

### Physical Protection

1. **Write Protect Tab**:
   - Physical tab on disk prevents writing
   - Detected by floppy drive mechanism
   - TOS checks before any write operation

2. **Non-Standard Formatting**:
   - **11-Sector Tracks**: As described earlier, can be marked as "read-only"
   - **Custom Gap Sizes**: Non-standard gaps prevent standard formatting
   - **Bad Sector Marking**: Deliberately mark sectors as bad in FAT

3. **Track Write Protection**:
   - **Odd/Even Track Formatting**: Format only odd or even tracks
   - **Alternate Sector Sizes**: Mix sector sizes on same disk
   - **Non-Contiguous Sectors**: Break sector numbering sequence

### Software Protection

1. **Boot Sector Protection**:
   - **Checksum Verification**: Boot sector checksum must equal $1234
   - **Loader String**: Must contain "Loader" at offset $02
   - **Custom Bootstrap**: Non-standard bootstrap code

2. **Directory Protection**:
   - **Hidden Files**: ATTRIB = $02
   - **System Files**: ATTRIB = $04
   - **Volume Labels**: ATTRIB = $08 (only in root directory)

3. **FAT Protection**:
   - **Custom FAT Entries**: Non-standard values
   - **FAT Chain Manipulation**: Break FAT chains
   - **Cluster Allocation**: Allocate clusters in non-sequential order

4. **Data Protection**:
   - **Custom CRC**: Use non-standard CRC algorithm
   - **Data Encoding**: Encrypt or scramble data
   - **Sector Relocation**: Store data in non-standard sector locations

### Copy Protection Techniques

1. **Weak Sectors**:
   - Sectors with marginal signal strength
   - Require precise drive calibration to read
   - Copy programs often fail to reproduce weak signals

2. **Long Tracks**:
   - Tracks with more data than standard format allows
   - Exploit drive's ability to write beyond standard track length

3. **Short Tracks**:
   - Tracks with less data than expected
   - Cause timing issues for copy programs

4. **Non-Standard Sector IDs**:
   - Use invalid track/sector numbers
   - Confuse copy programs expecting standard values

5. **Checksum Manipulation**:
   - Deliberately corrupt CRC values
   - Require custom verification routines

6. **Boot Loader Tricks**:
   - Custom bootstrap code that checks for original disk
   - Verify disk geometry, timing, or other characteristics

---

## Compatibility Considerations

### Atari vs PC Compatibility

| Format | Atari Read | Atari Write | PC Read | PC Write |
|--------|------------|-------------|---------|----------|
| Atari Standard | ✓ | ✓ | ✗ | ✗ |
| IBM System 34 | ✓ | ✓ | ✓ | ✓ |
| ISO DD | ✓ | ✓ | ✓ | ✓ |

**Key Differences**:
1. **IAM Presence**: Atari omits IAM, PC requires it
2. **Gap Sizes**: Atari uses different gap values
3. **CRC Algorithm**: Both use CRC-CCITT but implementation may vary

### Drive Compatibility

**WD1772 FDC**:
- **Atari ST**: Standard controller
- **Compatibility**: Can read IBM and ISO formats
- **Limitations**: Cannot format with IAM (Atari TOS limitation)

**Drive Requirements**:
- **Rotation Speed**: 300 RPM ±1% for reliable 11-sector formatting
- **Head Alignment**: Critical for reading weak sectors
- **Signal Quality**: Must support MFM encoding at 500 kbps

### Software Compatibility

**TOS Versions**:
- **TOS 1.0-1.4**: Support standard 9-sector format
- **TOS 2.0+**: Additional support for non-standard formats
- **Custom ROMs**: May support additional protection schemes

**Disk Images**:
- **ST Format**: Standard Atari disk image (track-based)
- **MSA Format**: Sector-based format with compression
- **DIM Format**: Disk image with metadata
- **IPF Format**: Interchangeable Preservation Format (supports copy protection)

---

## Practical Applications

### Disk Duplication

1. **Standard Disks**:
   - Use standard formatting tools
   - Copy sector-by-sector
   - Verify with CRC checks

2. **Protected Disks**:
   - Require specialized tools (e.g., Discovery Cartridge, KryoFlux)
   - May need to reproduce non-standard formats
   - Often require custom copy programs

### Disk Preservation

1. **Image Creation**:
   - **Track-Based Imaging**: Captures entire track including gaps
   - **Sector-Based Imaging**: Captures only sector data
   - **Flux-Level Imaging**: Captures raw flux transitions (most accurate)

2. **Image Formats**:
   - **ST**: Simple track-based format
   - **MSA**: Compressed sector-based format
   - **DIM**: Includes metadata and error information
   - **IPF**: Preserves copy protection mechanisms

3. **Tools**:
   - **Steem**: Atari ST emulator with disk imaging
   - **Saint**: Disk image manipulation tool
   - **Pasti**: Advanced disk imaging with protection support
   - **KryoFlux**: Hardware flux-level imager

### Disk Analysis

1. **Track Analysis**:
   - Verify track lengths
   - Check gap sizes
   - Validate sector IDs

2. **Sector Analysis**:
   - Verify CRC values
   - Check for weak sectors
   - Analyze data patterns

3. **FAT Analysis**:
   - Verify FAT chains
   - Check for bad sectors
   - Validate cluster allocation

### Emulation Considerations

1. **Disk Image Support**:
   - Emulator must support various image formats
   - Must handle non-standard formats
   - Should emulate copy protection mechanisms

2. **FDC Emulation**:
   - Accurate WD1772 emulation
   - Proper timing for gap handling
   - Correct MFM encoding/decoding

3. **Performance**:
   - Disk access is slowest part of emulation
   - Optimize common operations (sector reads/writes)
   - Cache frequently accessed sectors

---

## Conclusion

The Atari ST floppy disk software system is a sophisticated implementation that balances capacity, reliability, and performance within the constraints of 1980s hardware. The system's design reflects careful consideration of:

1. **Physical Constraints**: Disk rotation speed, head movement, signal timing
2. **Hardware Capabilities**: WD1772 FDC features and limitations
3. **Software Requirements**: TOS compatibility, file system needs
4. **Protection Mechanisms**: Copy protection, data integrity

Key insights from this analysis:

- The **omission of IAM** in Atari's standard format creates a one-way compatibility with PC disks
- **Gap optimization** allows for non-standard sector counts but at the cost of reliability
- The **12-bit FAT** system provides efficient cluster management for floppy-sized storage
- **Copy protection** mechanisms exploit both hardware limitations and software tricks
- **Emulation and preservation** require accurate reproduction of low-level formatting details

Understanding these technical details is essential for:
- Developing accurate Atari ST emulators
- Preserving original software and games
- Creating compatible disk images
- Implementing disk manipulation tools
- Appreciating the engineering constraints of vintage computing

The Atari ST's floppy disk system remains a testament to the ingenuity of 1980s computer engineering, packing remarkable functionality into limited hardware while providing a foundation for the platform's software ecosystem.
