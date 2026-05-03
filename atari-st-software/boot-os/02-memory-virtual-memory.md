# TOS Memory Management and Virtual Memory

## 1. Physical Memory Architecture

The Atari ST uses a flat 24-bit (16 MB) address space, mapped through the 68000 CPU's A0-A23 address bus. Only 880 KB is physically addressable on the ST range; the Mega STE and TT030 extend this.

### Standard ST Memory Map (520STE / 1040STE)

```
Address Range         Size    Content
───────────────────────────── ─────────────────────────
$000000 - $007FFF     32 KB   System variables (below $400, $800, or $C00)
$008000 - $009FFF     8 KB   Stack area (below $400, $800, or $C00)
$00A000 - $00BFFF     8 KB   System variables
$00C000 - $00C7FF     2 KB   GEM/BIOS variables (below $400, $800, or $C00)
$00C800 - $00BFFF     2 KB   AES work area (below $400, $800, or $C00)
$010000 - $07DFFF     780 KB   Free memory (lowest 800 KB)
$07E000 - $07FFFF     8 KB   Reserved
$400000 - $4FFFFF     1 MB    Frame buffer (lowres, 320x200x4bpp = 32 KB)
$F00000 - $F0FFFF     256 KB IKBD microcontroller ROM
$F40000 - $F9FFFF     384 KB ROM 1 (TOS core)
$FA0000 - $FBFFFF     128 KB ROM 2 (extended TOS, STe+)
$FC0000 - $FDFFFF     128 KB ROM 2 continued
$FE0000 - $FFFFFF     128 KB ROM 2 continued
$FF8000 - $FF87FF     2 KB   BIOS entry area (TRAP #10)
$FF8800 - $FF88FF     256 B  YM2149 PSG audio registers
$FF8C00 - $FF8FFFF    1.5 KB DMA base registers
$FF9800 - $FF98FF     256 B  Acia2 (modem port, Mega STE+)
$FF9A00 - $FF9AFF     256 B  Acia2 data/port
$FFA000 - $FFA1FF     512 B  MFP (68901) interrupts
$FFFC00 - $FFFCFF     256 B  Keyboard ACIA (6530)
$FFFC80 - $FFFCFF     128 B  MIDI ACIA (6530)
$FFFCC0 - $FFFCCF     32 B  IKBD (6530)
$FFFA00 - $FFFAFF     256 B  ACIA 2 (modem port)
$FFFE00 - $FFFEFF     256 B  RAM base (MMU)
```

### Mega STE Memory Map Extension

```
Address Range         Size    Content
───────────────────────────── ─────────────────────────
$080000 - $0FFFFF     512 KB   ST-RAM (standard ST memory above base)
$100000 - $3FFFFF     3 MB     Alternate RAM (VME bus memory)
$400000 - $7FFFFF     4 MB     Frame buffer (STE Hi-Res 640x512x4bpp = 128 KB)
$800000 - $FFFFFFF    8 MB     VME bus memory space
```

### TT030 Memory Map (32-bit)

```
Address Range         Size    Content
───────────────────────────── ─────────────────────────
$00000000 - $0003FFFF   256 KB   ST-RAM (PMMU low)
$00040000 - $0005FFFF   128 KB   Alternate RAM
$00060000 - $007FFFFF   7.875 MB   Free memory
$00800000 - $00FFFFFF   8 MB     Frame buffer (VESA-compatible)
$01000000 - $1FFFFFFF   480 MB   Alternative RAM
$FF000000 - $FFFFFFF    16 MB     Hardware registers (PMMU)
```

### Falcon030 Memory Map (32-bit)

```
Address Range         Size    Content
───────────────────────────── ─────────────────────────
$000000 - $007FFFF      512 KB   ST-RAM
$0080000 - $00FFFFF      512 KB   Alternate RAM
$0100000 - $019FFFF      640 KB   Frame buffer (16-color lowres)
$01A0000 - $019FFFF      640 KB   Frame buffer (Hi-Res, HiVid)
$01A0000 - $01AFFFF      64 KB   DSP shared memory
$FF000000 - $FFFFFFF      16 MB   Hardware registers
```

## 2. GEMDOS Memory Functions

### Malloc (GEMDOS function 72 / TRAP #14)

Allocates a block of memory from the free pool.

```
Calling convention:
    D0.D = number of bytes to allocate
    -1    = query largest free block size (also in D0)
    >0    = allocate this many bytes

Return:
    D0.D = starting address of block, or NULL ($0)
```

```asm
; Allocate 4096 bytes
    move.l #4096,d0
    trap #14          ; GEMDOS MALLOC
    cmp.l #0,d0
    beq.s alloc_failed
    move.l d0,mem_block

; Query largest free block
    move.l #-1,d0
    trap #14
    move.l d0,free_bytes  ; D0 = largest contiguous block in bytes
```

### Mfree (GEMDOS function 73 / TRAP #14)

Releases a previously allocated block.

```
Calling convention:
    D0.D = address of block (returned by MALLOC)

Return:
    D0 = E_OK (0) on success, EIMBA (-23) on failure
```

```asm
; Free mem_block
    move.l mem_block,d0
    move.w #73,-(sp)
    trap #14
    addq.l #2,sp
```

### Mxalloc (GEMDOS function 68 / TRAP #14)

Advanced allocation with memory type and protection flags. Available from GEMDOS 0.19+ (MultiTOS, MiNT, MagiC).

```
Mode parameter (WORD):
    Bits 0-2: RAM treatment
        0 = ST-RAM only
        1 = Alternate RAM only
        2 = Either ST preferred
        3 = Either Alternate preferred

    Bits 4-7: Protection mode
        0 = From PRGFLAGS (program flags)
        1 = Private (access only by owner process)
        2 = Global (readable by all supervisor processes)
        3 = Supervisor-mode-only access
        4 = World-readable access

    Bit 14: No-Free mode (OS only - block persists after process terminates)
```

```asm
; Allocate 8192 bytes, ST-RAM preferred, global mode
    move.l #8192,d0
    move.w #$02,d1            ; mode: ST preferred, global
    trap #14
    cmp.l #0,d0
    beq.s alloc_failed
    move.l d0,global_mem
```

### Mshrink (GEMDOS function 74 / TRAP #14)

Resizes an existing allocation.

```
Calling convention:
    D0.D = address of block
    D1.D = new size
    -1 = largest possible size
     0 = release block

Return:
    D0 = E_OK (0), EIMBA (-23), or EGSBF (-11)
```

### Maccess (GEMDOS function 381 / TRAP #14)

Verify accessibility of a memory region.

```
Calling convention:
    D0.D = start address
    D1.D = size
    D2.W = mode (1 = read, 0 = read/write)
```

### Mvalidate (GEMDOS function 321 / TRAP #14)

Verify accessibility of a specific process's memory region.

```
Calling convention:
    D0.W = process ID (0 = current process)
    D1.D = start address
    D2.D = size
    D3.D = flags

Return:
    D0 = E_OK (0) or negative error code
    D3.W = protection flags on success
```

### Maddalt (GEMDOS function 20 / TRAP #14)

Register additional RAM (e.g., VME bus memory) with GEMDOS.

```
Calling convention:
    D0.D = start address
    D1.D = size

Return:
    D0 = E_OK (0) or negative error code
```

## 3. Free Memory Pool Structure

### Memory Block Table

TOS maintains a linked list of free memory blocks starting at system variable $10A (CURFREE):

```
Offset    Size    Content
────────                ─────────
$0000   4B      CURFREE - pointer to first free block
$0004   4B      CURFREE+4 - pointer to last free block
$0008   4B      CURFREE+8 - pointer to highest block
```

Each free block in the table has the following structure:

```
Block Header (per block):
    Offset    Size    Content
    ────        ──── ─────────────────
    +0x00   4B      Next block pointer
    +0x04   4B      Previous block pointer (or 0 if first)
    +0x08   2B      Block size (bytes)
    +0x0A   2B      Owner process ID
    +0x0C   2B      Free block size (bytes) - same as above when free
    +0x0E   2B      Block type (F=free, A=application)
    +0x10   2B      Checksum
```

### Free Memory at $10A / CURFREE

```
$108     2B      MAXFLEN - Max file name length
$10A     4B      CURFREE - Address of lowest free block
$10E     2B      CURFREE+2 - Block size at CURFREE
$110     2B      CURFREE+4 - Owner of CURFREE block
```

### Memory Allocation Algorithm

```
malloc(size):
    entry = $10A                    ; CURFREE
    best = NULL
    bytes_free = 0

    while entry < end_of_ram:
        block_size = WORD[entry + 8]
        if block_size >= size:
            if best = NULL or block_size < best_size:
                best = entry
                best_size = block_size
            bytes_free += block_size
        entry += block_size

    if best != NULL:
        if best_size - size >= min_split_threshold:
            ; Split block
            new_block = best + size
            WORD[new_block + 8] = best_size - size
            WORD[best + 8] = size
    return best
```

## 4. Virtual Memory Architecture

### Mega STE Virtual Memory (PMMU)

The Mega STE (GT64A016 MMU) introduces hardware virtual memory with 4 K-page table registers.

```
PMMU Registers:
    Register    Address    Description
    ────────    ───────    ───────────
    PVR         $FF922C    Page Vector Register
    PRR         $FF922E    Page Resolution Register
    PUCR        $FF920C    Page Update Count Register
    PCR         $FF920D    Page Control Register
    PACR        $FF920E    Page Access Control Register
    PAIR0       $FF921C-   Page Access Information Register (4 pages)
    PACR0       $FF9210    Page Access Control Register (4 pages)
    PBRR        $FF922E    Page Base Register Resolution Register
```

### Page Table Entry Format (Mega STE)

```
Page Table Entry (32 bits):
    Bits 0-19: Frame address (20-bit = 1 MB pages)
    Bit 20: Valid bit
    Bit 21: Dirty bit
    Bit 22: Accessed bit
    Bit 23: Cache enable (STE only)
    Bit 24: Copy-on-write
    Bit 25-26: Priority (00 = lowest, 11 = highest)
    Bit 27-28: Page type
    Bit 29: User/supervisor (0 = supervisor only)
    Bit 30: Read-only (1 = read-only)
    Bit 31: Page present (1 = in memory)
```

### MMU Page Table Layout (Mega STE)

```
Address       Content
─────────────────────────
$FF9200       MMU base register
$FF921C       Page table entries (256 entries)
$FF925C       Page vector table
$FF926C       Page control register
$FF926E       Page access control register
$FF921C0      Page access info registers (4 pages)
$FF90000      Page base register
$FF94000      Page resolution register
```

### TT030 PMMU (32-bit Protected Memory Management Unit)

The TT030 replaces the STE MMU with a full 32-bit PMMU:

```
TT PMMU Architecture:
    - 4 KB pages (4096 bytes)
    - 4 TLB (Translation Lookaside Buffer) entries
    - Full protection: user/supervisor, read/write/exec
    - MMU page directory at $FF000000

Page Directory Entry (32-bit):
    Bits 0-21: Page frame address (22 bits = 4 GB)
    Bit 22: D (Dirty bit)
    Bit 23: C (Copy-on-write)
    Bit 24: U/S (User/Supervisor)
    Bit 25: R/W (Read/Write)
    Bit 26: P (Present)

Page Size: 4096 bytes ($1000)
Address alignment: pages must be 4K-aligned
```

### Falcon030 PMMU

Falcon uses a simplified PMMU with:

```
    - 1 KB pages
    - PMMU page table at $01A80000
    - MMU enabled/disabled via bit 5 of system variable at $536
    - 4 TLB entries + software extension
```

## 5. Memory Types and Allocation Modes

### RAM Type Codes

| Code | Name | Platform | Range | Characteristics |
|------|------|----------|-------|----------------|
| 0 | ST-RAM | All | $00040000+ | Standard ST RAM only |
| 1 | Alternate RAM | STE/TT/Falcon | $100000+ | VME/PCI/Expansion RAM |
| 2 | Either ST pref.| STE/TT/Falcon | $00040000+ | Tries ST first |
| 3 | Either Alt pref.| STE/TT/Falcon | $100000+ | Tries alternate first |
| 4 | TT-RAM | TT030 | $02000000+ | TT-specific RAM |
| 5 | Falcon RAM | Falcon | $01000000+ | Falcon-specific RAM |

### Mode Combination Examples

```
Mode WORD = RAM-type (bits 0-2) | Protection (bits 4-7)

Example 1: ST-RAM, supervisor-only
    RAM-type:     000b (0 = ST-RAM)
    Protection:   011b (3 = Supervisor-only)
    Mode = $0003

Example 2: Alternate RAM, global mode
    RAM-type:     001b (1 = Alternate)
    Protection:   010b (2 = Global)
    Mode = $0010

Example 3: Either, ST preferred, user mode (default)
    RAM-type:     010b (2 = ST preferred)
    Protection:   000b (0 = From PRGFLAGS)
    Mode = $0002
```

## 6. System Variables for Memory

### Core Memory System Variables (below $400)

```
Offset    Size    Name        Description
──────── ──── ─────────────────────────────
$003E   2B      MAXMEM      Maximum available memory size
$0040   4B      AVLMAX      Largest available block (long)
$005A   4B      APLMAX      Largest application block (long)
$00AE   2B      APLMAX+2    Upper byte of APLMAX
$00B2   2B      LOFREM      Lower word of free memory
$00B4   2B      LOFREM+2    Upper word of free memory
```

### Core Memory System Variables ($400-$FFF)

```
Offset    Size    Name        Description
──────── ──── ─────────────────────────────
$0108   2B      MAXMEM      Maximum memory size (words)
$010A   4B      CURFREE     Lowest free block address
$010E   2B      CURFREE+2   Block size at CURFREE
$0110   2B      CURFREE+4   Owner at CURFREE
```

### MultiTOS Memory Variables

```
Offset    Size    Name        Description
──────── ──── ─────────────────────────────
$0140   4B      TOSBASE     TOS ROM base address
$0144   4B      TOSUPPER    Highest available address
$0148   2B      NPROC       Number of active processes
$014A   4B      PDB         Process descriptor base
$014E   2B      PID         Current process ID
```

## 7. Cookie Jar

The cookie jar stores hardware features and OS capabilities. It is a linked list of 4-byte key-value pairs stored in the GEMDOS memory space, pointed to by system variable `$0140` (GEMINFO output). TOS 1.06+ provides built-in cookies.

### Cookie Jar Format

```
Header:
    Magic bytes:  "CookieJ " (8 bytes)
    Followed by pairs:
        Offset   Size   Description
        ───────  ────   ─────────
        +0       4B     Cookie ID (tag, usually ASCII 4-char)
        +4       4B     Cookie value
        ... terminated by $FFFFFFFF tag
```

### Common Cookies

```
Cookie ID    Value Range  Description
──────────────── ───────────────────────────────────
"VBL "       25-300     VBL interval (microseconds)
"SCRn"       0-5        Current screen resolution
"RESl"       320        Resolved base address
"CPU "       68000      CPU type
"MMU "       MMU type   MMU type present ($FFFF if none)
"MOT "       Motion       Mouse resolution (MOT2 = STE+)
"CPU "       68000      CPU type
"CPU "       68030      CPU type (030)
"CPU "       68040      CPU type (040)
"CPU "       68LC040    CPU type (LC040)
"MMU "       MMU type   MMU type present
"PMM "       PMMU type  PMMU present
"RAM "       Total RAM   Total RAM in KB
"RESv"       Resolution  Display resolution
"VDEv"       VDI device  VDI virtual device number
"FPU "       1          FPU present
"VMM "       1          Virtual memory manager present
"HDR"n       1          Hardware driver present
"GEMV"       GEM version  GEM version number
"GEMR"       GEM revision  GEM revision number
```

### Reading the Cookie Jar

```asm
; Query "CPU " cookie
    lea cookie_jar,a0          ; address from $140
query_cpu:
    move.l (a0)+,d0            ; read tag
    cmp.l #'CPU ,d0
    bne.s next_cookie
    move.l (a0),d0             ; CPU type in D0
    rts
next_cookie:
    cmp.l #-1,d0
    bne.s query_cpu
    move.l #'NOOP ,d0             ; not found
    rts
```

## 8. Stack Architecture

### Stack Usage in User Mode

```
68000 Stack (User mode, $A7 points to top):
    Direction: grows toward lower addresses (push = subtract)

    Top of stack (grows downward)
    ─────────────────────────────────
    Return address (PC)     2B  (after JSR/BSR)
    SR (Status Reg)         2B  (after RTS/RTE)
    Parameters              N*2B (pushed before call)
    Space for locals        N*1B (allocated by subtracting from A7)
    Saved registers (ABCD)  N*2B (PUSHL instruction)
    ─────────────────────────────────
    Stack base (user)
```

### Supervisor Stack Switch

When entering supervisor mode via TRAP #14:

```
Supervisor mode triggers:
    1. 68000 switches to supervisor A7 (from $530, $534, or $536)
    2. If in MultiTOS, task_switch occurs via process descriptor
    3. Stack frame for supervisor trap:
        $sp+0:  SR (saved)
        $sp+2:  PC (return address)
```

### Stack Protection

```
Atari ST does not have native stack canaries (pre-MMU era).
Protect against stack overflow by:
    1. Reserve extra stack space at program startup
    2. Use fixed-size data structures (avoid dynamic recursion)
    3. Monitor stack pointer position relative to end of application segment
    4. Use GEMDOS function $20 (SUPER) to check if supervisor mode
```

## 9. Memory Pool Management (MultiTOS)

### Process Memory Regions in MultiTOS

Each process in MultiTOS has independent memory regions:

```
Process Memory Layout:
    Region              Size    Access
    ───────────         ────    ──────
    Program text        varies  Read-only (in PRG file)
    Initial data        varies  Read-write (copied to alloc)
    Uninitialized data  varies  Zero-filled
    Heaps               varies  Dynamic allocation
    Stacks              2 KB    Per process (user + supervisor)
```

### GEM DOS Memory Block Header

```
GEMDOS Block Header (at allocated address):
    Offset    Size    Content
    ────        ────  ───────
    +0x00   4B      Next block pointer
    +0x04   4B      Previous block pointer
    +0x08   2B      Block size (bytes, including header)
    +0x0A   2B      Owner PID
    +0x0C   2B      Free block size
    +0x0E   2B      Flags (0x0002 = allocated)
    +0x10   2B      Checksum
```

## 10. Physical Memory Configuration

### SIMM Memory Configuration

```
Model           RAM     Expansion
───────── ─── ─────────────────────────────────────
520ST       512 KB    12-pin SIMM sockets (30-pin)
                520STE      512 KB + 1x30-pin SIMM (up to 1.5 MB)
1040ST      1024 KB   2x30-pin SIMM sockets (up to 2 MB)
                1040STE     1 MB + 1x30-pin SIMM (up to 4 MB)
                Mega STE  2 MB + 2x72-pin SIMM (up to 64 MB)
                TT030     4 MB + 8x72-pin SIMM (up to 512 MB)
                Falcon    2 MB + 1x72-pin SIMM (up to 16 MB)
```

### Memory Detection Sequence

```
1. BIOS boot checks memory at $4F2 (warm/cold boot flag)
2. TOS detects installed RAM via write-read test at $7D0
3. CURFREE (at $10A) updated to point to highest free address
4. Memory controller (RESMCR, XBIOS $15/$52) queried
5. Cookie jar populated with RAM information
6. GEMDOS memory management initialized
```

## 11. Memory Allocation Best Practices

```asm
; Safe memory allocation pattern:
safe_malloc:
    moveq #0,d7                    ; Disable task switching
    move.w #$20,-(sp)
    trap #14
    move.l #size_needed,d0
    trap #14                     ; GEMDOS MALLOC
    move.w #$20,-(sp)
    trap #14                     ; Re-enable task switching
    cmp.l #0,d0
    beq.s alloc_err

    move.l d0,mem_block
    ; Zero the allocated memory
    move.l d0,a0
    move.l #size_needed,d1
    moveq #0,d2
    beq_clear_mem:
        move.w d2,d2             ; D2 = offset
        move.l #[0,0,0,0],(a0,d2.w) ; clear 16 bytes
        addq.l #16,d2
        cmp.l #size_needed,d2
        blt.s beq_clear_mem
    rts
```

## 12. FreeMiNT Memory Extensions

FreeMiNT extends GEMDOS memory management beyond stock TOS:

```
FreeMiNT-specific extensions:
    Maccess (GEMDOS 381) - page accessibility check
    Mvalidate (GEMDOS 321) - process memory validation
    Mxalloc (GEMDOS 68) - protection-enabled allocation
    Maddalt (GEMDOS 20)   - register alternate RAM
    Mshrink (GEMDOS 74)   - resize block (MiNT extension)
```
