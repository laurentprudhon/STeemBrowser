# ACSI (Atari Computer System Interface)

> The ACSI is Atari's proprietary hard disk bus, similar to SCSI-1. It connects hard drives and laser printers to the Atari ST via a custom 19-pin D-sub connector (the DB19 port).

## Overview

ACSI stands for **Atari Computer System Interface**. It is an 8-bit bidirectional parallel bus that allows multiple devices (typically hard disks and laser printers) to be connected to the Atari ST. ACSI is derived from SCSI-1 but with important differences:

| Feature | ACSI | SCSI-1 |
|---------|------|--------|
| Bus control | Host-only (ST always controls) | Can be initiated by any device |
| Device ID | 0-6 (7 addresses) | 0-7 (8 devices) |
| Data width | 8 bits | 8 bits |
| Transfer rate | Up to 1 MB/s (practical), up to 2 MB/s with efficient programming | Up to 5 MB/s |
| Max devices | 5 (including host as one) | 8 (including host) |
| Cable length | ~6 meters typical | ~6 meters (passive), ~25m (active) |
| Connectors | DB19 (custom 19-pin D-sub) | DB25 (standard SCSI connector) |

## ACSI Protocol

### Handshake Mechanism

ACSI uses a simple handshake protocol similar to SCSI:

| Signal | Description |
|--------|-------------|
| DATA0-DATA7 | 8-bit data bus |
| REQ | Data Request (device to host) |
| ACK | Acknowledge (host to device) |
| BS | Bus Select (host signals device) |
| SRST | System Reset (host to all devices) |
| IO | Input/Output (0 = SELECT, 1 = READ) |
| CD | Command/Data select |
| MSG | Message signal |
| I/O, C/D, MSG | Combined as 3-bit control bus |

### Phase Sequence

1. **Attention**: Host asserts BS, sets I/O, C/D, MSG lines to select target device
2. **Selection**: Device responds with ACK
3. **Command phase**: Host sends 4-byte CDB (Command Descriptor Block)
4. **Data phase**: Bidirectional data transfer via REQ/ACK handshake
5. **Status phase**: Device returns 1-byte status
6. **Message phase**: Optional message exchange (2 bytes)
7. **Disconnection/Reconnection**: Optional, device disconnects from bus

### CDB (Command Descriptor Block) Format

The CDB is 4 bytes (unlike SCSI-1's 6 or 10 bytes):

| Byte | Bits | Field |
|------|------|-------|
| 0 | 0-7 | Operation code (READ = $08, WRITE = $0A, etc.) |
| 1 | 0-4 | Logical block address (low byte) |
| 1 | 5-7 | Logical block address (high bits) |
| 2 | 0-7 | Logical block address (middle) |
| 3 | 0-7 | Logical block address (high), sector count, or mode byte |

### Command Codes

| Code | Command | Description |
|------|---------|-------------|
| $00 | TEST UNIT READY | Check if device is ready |
| $04 | REQUEST SENSE | Get device error information |
| $05 | FORMAT UNIT | Format a disk unit |
| $08 | READ (10) | Read sectors from device |
| $0A | WRITE (10) | Write sectors to device |
| $0B | VERIFY | Verify sectors on device |
| $0E | MODE SELECT | Set device parameters |
| $0F | MODE SENSE | Get device parameters |
| $12 | INQUIRY | Get device identification |
| $1A | STOP UNIT | Stop spindle rotation |
| $1B | START START UNIT | Start spindle rotation |
| $1E | PREFETCH | Pre-fetch data |
| $25 | READ CAPACITY | Get disk capacity information |
| $3F | REPORT LUN | Report logical unit numbers |

## DB19 Connector

### Physical Connector

| Parameter | Value |
|-----------|-------|
| Type | DB19S (D-sub 19-pin, female) |
| Location | Rear panel |
| Shield | Metal casing |

### Pinout (Standard DB19)

| Pin | Signal Name | Direction | Description |
|-----|-------------|-----------|-------------|
| 1 | DATA0 | Bidir | Data bit 0 (LSB) |
| 2 | DATA1 | Bidir | Data bit 1 |
| 3 | DATA2 | Bidir | Data bit 2 |
| 4 | DATA3 | Bidir | Data bit 3 |
| 5 | DATA4 | Bidir | Data bit 4 |
| 6 | DATA5 | Bidir | Data bit 5 |
| 7 | DATA6 | Bidir | Data bit 6 |
| 8 | DATA7 | Bidir | Data bit 7 (MSB) |
| 9 | REQ | Output (device) | Request (active low) |
| 10 | ACK | Input (device) | Acknowledge (active low) |
| 11 | GND | Ground | Signal ground |
| 12 | BS | Output (host) | Bus select (active low) |
| 13 | GND | Ground | Signal ground |
| 14 | I/O | Output (host) | Input/Output control |
| 15 | GND | Ground | Signal ground |
| 16 | C/D | Output (host) | Command/Data select |
| 17 | GND | Ground | Signal ground |
| 18 | SRST | Output (host) | System reset (active low) |
| 19 | MSG | Output (host) | Message signal |

Note: The exact pin mapping varies between Atari models. The 520ST uses a different pinout than the Mega STE and Mega ST.

## Memory Mapping

ACSI registers are memory-mapped in the ST address space:

| Address Range | Purpose | Size |
|---------------|---------|------|
| $F80000-$F803FF | ACSI command/status registers | 1 KB |
| $F80400-$F804FF | ACSI data buffer | 256 bytes (not all models) |

### ACSI Control Registers

| Offset | Address | Access | Description |
|--------|---------|--------|-------------|
| $0000 | $F80000 | W | ACSI command register |
| $0001 | $F80001 | R | ACSI status register |
| $0002 | $F80002 | W | ACSI address register (logical unit) |
| $0004 | $F80004 | R/W | ACSI data register (byte access) |
| $0006 | $F80006 | R/W | ACSI data register (word access) |

### Status Register Bits

| Bit | Name | Description |
|-----|------|-------------|
| 0 | IRQ | ACSI interrupt pending |
| 1 | BUSY | ACSI command in progress |
| 2 | DATA_READY | Data available in buffer |
| 3 | DRQ | DMA request pending |
| 4-7 | - | Reserved |

## ACSI Device IDs and Selection

ACSI devices are selected by logical unit number:

| LUN | Purpose |
|-----|---------|
| 0 | First ACSI device (primary hard disk) |
| 1 | Second ACSI device (secondary hard disk) |
| 2 | Third ACSI device (laser printer) |
| 3-4 | Other devices |
| 5-6 | Reserved |

The host selects a device by writing the LUN to the ACSI address register, then asserting BS (Bus Select). Only the selected device responds.

## DMA Interaction

The ACSI interface uses the DMA chip for data transfers:

1. Host writes ACSI command to $F80000
2. Host sets up DMA channel with device address
3. DMA chip asserts HDRQ (Hard Disk DMA Request)
4. When device is ready, DMA chip takes bus via BG
5. Data transfers between ACSI device and system RAM
6. Upon completion, ACSI device asserts interrupt via MFP

## Hard Disk Format on Atari ST

### HD/File System

Atari ST hard disks use the **HD** file system (also called **DOS-F**):

- **Boot block**: Track 0, Sector 1 (contains HD driver and TOS bootstrap)
- **Volume table**: Immediately after boot block (contains volume info, free space map)
- **File allocation table**: Tracks which clusters are in use
- **Directory**: File names and attributes
- **Data area**: Actual file contents

### Track/Sector Layout

| Parameter | Value |
|-----------|-------|
| Sectors per track | 18 (DD) or 36 (HD) |
| Bytes per sector | 512 |
| Tracks per cylinder | 1 (single-sided) or 2 (dual-sided) |
| Clusters per FAT | 64K entries |

## Laser Printer Interface

The ACSI port can also connect Atari laser printers:

| Printer | Model | Speed | Resolution |
|---------|-------|-------|------------|
| SLM804 | Atari SLM804 | 8 ppm | 300 dpi |
| SLM605 | Atari SLM605 | 6 ppm | 300 dpi |

The laser printer connection:
- Uses ACSI protocol for print job transfer
- Printer sends status via ACK/REQ handshake
- Prints page data as rasterized bitmap sent from host

## References

- [ACSI, SCSI and IDE - AtariForumWiki](https://temlib.org/AtariForumWiki/index.php/ACSI,_SCSI_and_IDE)
- [Atari ST Internals, ch. 1.3 - DMA Controller](https://www.atarimania.com/documents/Atari-ST-Internals.pdf)
- [SCSI-1 Specification](https://en.wikipedia.org/wiki/SCSI)
- [The Atari ST Bus Doc (PDF)](http://info-coach.fr/atari/ressources/doc/Atari%20ST%20bus%20doc.pdf)
- [Atari ST Quick FAQ](https://atari.org/hosted/quickfaq/stfaq_3.htm)
