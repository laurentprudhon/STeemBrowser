# Atari ST Boot Process and Disk Loading

## 1. 68000 Reset Sequence

```
MC68000 RESET
    |
    v
SP <- [000000]    ; Load supervisor stack pointer
PC <- [000004]    ; Load program counter
    |
    v
Begin execution at address contained in [000004]
    |
    v
SR <- $2700       ; Supervisor mode, IPL=7, trace=0
```

### Exception Vector Table (Reset)

```
Address | Content
$0000   | Initial Supervisor Stack Pointer (longword)
$0004   | Initial Program Counter (longword)
$0008   | Bus Error vector
$000C   | Address Error vector
$0010   | Illegal Instruction Exception vector
$0014   | Divide by Zero Error vector
$0018   | CHK Instruction Exception vector
$001C   | TRAPV Exception vector
$0020   | Privilege Violation Exception vector
$0024   | Trace Exception vector
$0028   | Line-A Exception vector
$002C   | Line-F Exception vector
$0030   | Reserved by Motorola
$0034   | Coprocessor Protocol Violation vector
$0038   | Format Error vector
$003C   | Uninitialized Interrupt vector
$0040-$005C | Reserved by Motorola
$0060   | Level 1 Auto-Vector Interrupt
$0064   ; Level 2 Auto-Vector Interrupt (HBlank)
$0068 | Level 3 Auto-Vector Interrupt
$006C | Level 4 Auto-Vector Interrupt (VBlank)
$0070 | Level 5 Auto-Vector Interrupt
$0074 | Level 6 Auto-Vector Interrupt
$0078 | Level 7 Auto-Vector Interrupt (NMI)
$007C | Level 8 Auto-Vector Interrupt
$0080 | Level 9 Auto-Vector Interrupt
$0084 | TRAP #0 vector
$0088 | TRAP #1 vector (XBIOS)
$008C | TRAP #2 vector
$0090 | TRAP #3 vector
...
$00BA | TRAP #10 vector (BIOS)
$00BE | TRAP #14 vector (AES/GEM)
$00C2 | TRAP #1E vector (VDI)
...
```

## 2. ROM Bootstrap Execution

```
[TRAP #0] -> Execute from ROM at $FFFF0000
    |
    v
[Step 1] Read config register at FFFFFF104 (byte)
    |   Bit 7 = 1 (PAL) / 0 (NTSC)
    |   Bits 2-3 = machine type (00=ST, 01=STF, 10=STE, 11=MegaSTE)
    |   Bit 1 = 1 if STE shifter present
    |
    v
[Step 2] Validate ROM checksum
    |   Sum all ROM words $0000 to $7D8C
    |   Add checksum word at $7D8E
    |   Result must equal $B4A9
    |   FAIL => "Row of bombs" error (TOS 1.0)
    |
    v
[Step 3] Initialize supervisor stack and memory controller
    |
    v
[Step 4] Check for reset handler ($424) - warm boot detection
    |   Read memvalid ($4F2), memval2 ($43A), memval3 ($51A)
    ; All valid = warm boot (skip RAM test)
    |   Invalid = cold boot (full RAM test and clear)
    |
    v
[Step 5] Cold boot: Test all RAM
    |   Write $55 to each byte, read back verify
    |   Write $AA to each byte, read back verify
    |   Use test pattern $FB55, 43 words, 128KB pages
    |   Bus error = end of physical memory
    |   Store result in phystop
    ;
[Step 5'] Clear memory from $400 to phystop
    |
    v
[Step 6] Initialize VDI drivers
    |
    v
[Step 7] Initialize system variables
    |   Set up VBL counter, memory variables, etc.
    |   Clear screen memory
    ;
[Step 8] Initialize I/O units
    |   FDC (WD1772), serial ports, MIDI
    |
    v
[Step 8'] Check for warm boot
    |   If $424 returns valid value, skip to step 9
    |
    v
[Step 9] Initialize GEMDOS
    |   Set up disk buffers ($200-$BFF)
    |   Initialize file handle table
    |
    v
[Step 10] Initialize GEM (AES, VDI, fonts)
    |
    v
[Step 11] Initialize cookie jar (TOS 1.06+)
    |
    v
[Step 12] Spin up hard disk drives (TOS 2.06+)
    |
    v
[Step 13] Try to boot from hard disk
    |   Load sector 0 from each drive
    |   Check disk signature at $01F8 = 0xAA55
    |   Jump to entry point if valid
    |
    v
[Step 14] If no hard disk boot, try floppy
    |   Check disk status register
    |   Is a disk installed in drive A or B?
    |
    v
[Step 15] Load boot sector from floppy
    |   Read first sector (256 words = 512 bytes) to $200
    |   Calculate checksum: sum all 256 words + last word
    |   if ((sum + WORD[254]) mod 65536) != $1234
    |       => "Row of bombs" (no valid boot disk)
    |   Jump to entry point at $200 (GEMDOS bootstrap)
    ;
    v
[Step 16] GEMDOS boot sector runs
    |   Parses disk directory for first .PRG
    |   Loads AUTO\*.PRG files if present
    |   Executes DESKTOP.PRG (GEM desktop launcher)
    ;
    v
[GEM Desktop Loaded]
    |   AES initialization
    |   VDI driver loading
    |   GDOS initialization
    |   Read DESKTOP.INF or NEWDESK.INF
    :   Desktop ready
```

## 3. Cold vs Warm Boot

### Determination

```asm
; At BIOS initialization point
lea memvalid(pc),a0     ; address $4F2
move.l (a0),d0
lea memval2(pc),a0      ; address $43A
cmp.l (a0),d0
bne warm_boot_check
lea memval3(pc),a0      ; address $51A
cmp.l (a0),d0
beq cold_boot           ; All invalid = cold boot
; If valid -> warm boot path
bra warm_boot
```

### Cold Boot
- Full RAM integrity test (writes $55 pattern, reads back, writes $AA, reads back)
- Clears all RAM from $0400 to `phystop`
- Full GEM initialization via DESKTOP.PRG
- Time taken: 3-5 seconds (depending on RAM size)

### Warm Boot
- Skips RAM test
- Skips full GEM initialization
- Jumps directly to DESKTOP.PRG restart
- Time taken: ~0.5 seconds
- Triggered by: Ctrl+Alt+Del key combination

## 4. Floppy Boot Disk Format

### Boot Sector Checksum Algorithm

```
checksum = 0
for i = 0 to 255:
    checksum = (checksum + WORD[$200 + i * 2]) AND $FFFF

; checksum word at $B54 ($200 + 254) must satisfy:
if ((checksum + WORD[$2B4]) AND $FFFF) != $1234:
    boot fails
```

### Boot Sector Entry Point

```
; $B50 (offset $150) contains disk boot sector type flag:
; $B54 (offset $154) contains the first .PRG file name
; $B56 (offset $156) contains the entry point for DESKTOP.PRG
; $B58 (offset $158) contains the AES init function address
```

## 5. DESKTOP.PRG Loading

The boot sector locates DESKTOP.PRG in the disk directory and loads it:

```
; DESKTOP.PRG is the GEM launcher:
; 1. Opens DESKTOP.PRG (GEMDOS FOPEN)
; 2. Loads it into memory (GEMDOS FREAD)
; 3. Jumps to entry point
; 4. DESKTOP.PRG initializes GEM AES
; 5. AES opens VDI driver
; 6. VDI sets up video hardware
; 7. DESKTOP.PRG loads DESKTOP.INF (user configuration)
; 8. DESKTOP.INF or NEWDESK.INF loads custom icons
; 9. Desktop appears with:
;    - Trash icon
;    - Driver/Config icons
;    - Hard disk icon
;    - Floppy disk icon
;    - Console icon
;    - Fonts icon
;    - GEM help
```

## 6. DESKTOP.PRG Function

DESKTOP.PRG is itself a GEM application:

### DESKTOP.PRG Internal Structure

```
; DESKTOP.PRG magic marker
Offset  Size  Content
$0000   2B    MZ header (0x4D5A)
$0002   2B    Last page of file (in 512B pages)
$0004   2B    Word count (reloc. table entries)
$0006   2B    Header size in words
$0008   2B    Minimum memory (in paragraphs)
$000A   2B    Maximum memory (in paragraphs)
$000C   2B    Checksum
; MZ header continues with relocation table, etc.
; After MZ header, TOS header:
; Offset $001E contains disk checksum ($1234)
; Offset $0037 contains entry point (offset within segment)
; Offset $0039 contains segment number
```

## 7. AUTO\*.PRG Execution

After GEM Desktop loads, TOS scans the BOOT drive for AUTO\*.PRG files:

```
GEMDOS FGFIRST("\AUTO\*.PRG") -> handle
loop:
    GEMDOS FGNEXT -> attrs, size, name
    if attrs & 16 (volume label): skip
    if attrs & 8 (subdir): skip
    ; Otherwise load and execute
    GEMDOS PEXEC(LOAD)
    ; Wait for process to terminate
    GEMDOS PTERM
    GEMDOS FGNEXT -> more files?
until no more files
```

### AUTO Folder Convention

- Location: root directory of boot drive
- Wildcard: AUTO\*.PRG
- Files executed in alphabetical order
- Each must complete before the next starts
- Used by: HDDRIVER, RAM disk, mouse drivers, font loaders

## 8. Program Loading by PEXEC

### PEXEC Function Mode 3

```asm
; PEXEC mode 3: Load and execute
; D0 = old stack limit
; A0 = new stack pointer
; A1 = filename string
; A2 = program arguments (string table)
; A3 = parent process handle
; A4 = new program entry point (filled by PEXEC)
; A5 = segment count (filled by PEXEC)
; A6 = segment table (filled by PEXEC)
; A7 = relocated base address (filled by PEXEC)

; Program loading flow:
; 1. Opens program file
; 2. Reads MZ header at 0x00
; 3. Calculates new allocation size
; 4. Allocates memory (GEMDOS LMEMTOP)
; 5. Relocates address map:
;    For each offset 0x1C to 0x1C + mapsize*2 - 1
;        read mapping word
;        if high bit set: add base address
; 6. Reads segments:
;    For segment number 0x1E to 0x1E + mapsize*2 - 1
;        count = word[segment_offset]
;        if count:
;            read count words from file
;            store at base + segment_number * 16
; 7. Copies argument strings to new stack
; 8. Sets up registers for new program:
;    D0 = arg string count
;    A3 = new base address
;    A4 = new stack pointer
; 9. Jumps to entry point
```

## 9. Hard Disk Boot

TOS 2.06+ supports hard disk boot:

```
; HDD boot sector location:
;   Sector 0, track 0 of HDD drive 0
;   Signature at offset $01F8: 0xAA55
;   Entry point at:
;     Offset $0000: Jump instruction
;     Offset $0003: OEM name (8 bytes)
;     Offset $000B: BPB (25 bytes)
;     Offset $001A: Boot code (448 bytes)
;     Offset $01FE: Signature 0xAA55
;     Offset $01FF: Reserved

; HDD boot flow:
; 1. BIOS BLKIN reads sector 0 to $200
; 2. Check WORD[$200+0x1FF] == 0xAA55
; 3. Check WORD[$200+0x1FC] == 0x0000 (reserved)
; 4. Jump to $0000 in boot sector
; 5. Boot sector loads volume ID + root directory
; 6. Searches for first executable .PRG
; 7. Loads and executes via PEXEC mode 3
```

## 10. Boot Timing Summary

```
Event                                 ~Time
──────────────────────────────────────────
Reset vector load              0 ms
Config register read             0 ms
ROM init checks                0 ms
RAM test (4 MB)              150 ms
BIOS init                      50 ms
GEM init                      200 ms
 DESKTOP.PRG load              100 ms
Disk boot (floppy)            150 ms
GEM Desktop display            50 ms
──────────────────────────────────────────
Total Cold Boot               ~700 ms (1040ST, 1MB RAM)
Total Warm Boot               ~100 ms (skip RAM test)
```
