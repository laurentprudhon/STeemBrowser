# TOS Multitasking and MultiTOS Architecture

## 1. Single-Tasking vs Multitasking in Atari ST

### Stock TOS (Pre-MultiTOS)

Stock TOS is fundamentally a single-tasking operating system with a cooperative multitasking layer.

```
Stock TOS Architecture:
    ┌─────────────────────────────┐
    │      Application Layer      │  ← User programs running
    ├─────────────────────────────┤
    │   AES (Application Env)    │  ← Window management, event handling
    ├─────────────────────────────┤
    │     GEMDOS                 │  ← Disk/file I/O (GEMDOS)
    ├─────────────────────────────┤
    │       BIOS                 │  ← Hardware-specific functions
    ├─────────────────────────────┤
    │       ROM TOS              │  ← Bootstrap, system variables
    └─────────────────────────────┘

Task switching model:
    - Cooperative (programs voluntarily yield control)
    - Triggered by AES events (button clicks, menu opens, etc.)
    - No preemption of running tasks
    - One program at a time (GEM desktop runs concurrently with one app)
```

### MultiTOS Overview

MultiTOS (TOS 1.02+ with MultiTOS patch, or TOS 2.06+) adds preemptive multitasking to the Atari ST:

```
MultiTOS Architecture:
    ┌─────────────────────────────┐
    │         Process 1           │  ← Independent memory space
    ├─────────────────────────────┤
    │         Process 2           │  ← Independent memory space
    ├─────────────────────────────┤
    │       MultiTOS SMP          │  ← Task scheduler
    ├─────────────────────────────┤
    │   AES (shared desktop)     │  ← Shared across tasks
    ├─────────────────────────────┤
    │     Multi GEMDOS            │  ← Per-process file descriptors
    ├─────────────────────────────┤
    │       Multi BIOS            │  ← Per-process I/O
    └─────────────────────────────┘
```

## 2. Process Descriptor Block (PDB)

### Process Structure in Memory

Each process in MultiTOS has a Process Descriptor Block (PDB):

```
PDB Structure:
    Offset    Size    Field             Description
    ────      ────    ──────────────────   ────────
    +0x00   4B      PDB_NEXT       Pointer to next PDB in list
    +0x04   4B      PDB_PREV       Pointer to previous PDB (circular)
    +0x08   4B      PDB_TOS        Pointer to TOS block
    +0x0C   2B      PDB_PID        Process ID (positive number)
    +0x0E   2B      PDB_PRIORITY   Process priority (1-7, higher = higher)
    +0x10   2B      PDB_STATE      Process state
    +0x12   2B      PDB_FLAGS      Process flags
    +0x14   4B      PDB_STACK      Supervisor stack pointer
    +0x18   2B      PDB_STACK_SEG  Segment of supervisor stack
    +0x1A   2B      PDB_USER_A7    User stack pointer (A7)
    +0x1C   2B      PDB_USER_SR    User SR (when in supervisor mode)
    +0x1E   2B      PDB_USER_A0    A0 of process
    +0x20   2B      PDB_USER_A1    A1 of process
    +0x22   2B      PDB_USER_A2    A2 of process
    +0x24   2B      PDB_USER_A3    A3 of process
    +0x26   2B      PDB_USER_A4    A4 of process
    +0x28   2B      PDB_USER_A5    A5 of process
    +0x2A   2B      PDB_USER_A6    A6 of process
    +0x2C   2B      PDB_USER_SR    User SR (when in user mode)
    +0x2E   2B      PDB_PC_PROG    Program count (instructions executed)
    +0x30   2B      PDB_WAIT_FL    Wait flags
    +0x32   4B      PDB_BASE       Base address of PDB
    +0x36   4B      PDB_MEM_PTR    Memory management pointer
    +0x3A   2B      PDB_MEM_COUNT  Memory count
    +0x3C   2B      PDB_MEM_TYPE   Memory type
    +0x3E   2B      PDB_EXITED     Exited flag
    +0x40   4B      PDB_EXIT_CODE  Exit code
    +0x44   4B      PDB_DTA        Disk Transfer Address
    +0x48   4B      PDB_PARENT     Parent process ID
    +0x4C   2B      PDB_CHILD        First child process ID
    +0x4E   2B      PDB_SIBLING      Next sibling process ID
    +0x50   2B      PDB_NAME[18]   Process name (18 bytes, null-padded)
```

### Process States

```
PDB_STATE values:
    $0000 = READY      Process ready to run
    $0001 = RUNNING    Process currently executing
    $0002 = WAITING    Process waiting for event
    $0003 = TERMINATED Process has terminated
    $0004 = SUSPENDED  Process suspended by another task
    $0005 = PAUSED     Process paused (VBI or timer)

    State transitions:
        READY → RUNNING:   Scheduler pick-up
        RUNNING → READY:   Task switch (preemption)
        RUNNING → WAITING: Process called wait function
        WAITING → READY:   Event signaled
        READY → SUSPENDED: Suspended by another process
        SUSPENDED → READY: Resumed by another process
        RUNNING → TERM:    Process called exit
        TERM → READY:      Reused for new process
```

## 3. Priority Scheduling

### MultiTOS Priority System

MultiTOS uses a priority-based preemptive scheduler:

```
Priority Levels:
    Priority    Source                    Notes
    ────    ──────────────        ────
    7         System timer (highest)   Cannot be lower than parent
    6         GEM (AES)              Highest user-priorities
    5         File I/O                Disk operations
    4         Network I/O             Serial/RPC/PPP
    3         Process base priority   Inherited from parent
    2         Default priority        Standard applications
    1         Background (lowest)     Lowest priority tasks

    Process inherits parent's priority at creation.
    Priority can be changed via XBIOS RESMCR functions.
```

### Priority-Inheritance Rules

```
When a process is created (via GEMDOS EXEC):
    child_priority = parent_priority  (inheritance)
    If child_priority < parent_priority:
        parent_priority = child_priority
    (This prevents priority inversion)

If parent is at priority 5 (file I/O):
    Child inherits PRIORITY 5
    Parent automatically promoted to PRIORITY 5 if lower

When a process terminates:
    Parent can be demoted from inherited priority
    Parent resumes own base priority
```

### Scheduler Behavior

```
The scheduler runs:
    1. Every VBI (~18.2 Hz / 55.27 ms per tick)
    2. When a process blocks/calls a wait function
    3. When the current process calls a trap/interrupt
    4. When an interrupt occurs with higher-priority pending process

Scheduling algorithm:
    1. Highest priority process ready → pick next
    2. If equal priority → round-robin (FIFO within priority)
    3. If no higher-priority process → continue current
    4. If no ready process at current priority → check higher
    5. If nothing ready → run idle process (priority 0)
```

## 4. MultiTOS System Variables

### System Variables for MultiTOS

```
Offset  Size    Name     Description
────────── ── ────── ───────────────────
$0130   4B      TOSBASE   Pointer to TOS base (PDB list)
$0134   4B      TOSUPPER  Highest available address
$0138   2B      MAXMEM    Maximum memory size (words)
$013A   2B      MAXMEM+2  Upper word of MAXMEM
$013C   2B      NPROC     Number of active processes
$013E   4B      PDB       Process descriptor block list
$0142   2B      PID       Current process ID
$0144   2B      PDB       Parent process descriptor pointer
$0146   4B      PDB       Current process descriptor pointer
$014A   2B      PDB_STATE  Current process state
$014C   4B      SPOOL_PTR  Spooler pointer
$0150   2B      TOS32     TOS 32-bit flag
```

### MultiTOS-Specific System Variables

```
Offset  Size    Name     Description
────────── ── ────── ───────────────────
$0154   2B      CURPROG   Current program name offset
$0156   2B      CURPRIP   Current program priority
$0158   4B      PDB_CUR   Current process descriptor
$015C   4B      PDB_NXT   Next process descriptor
$0160   2B      PDB_CUR_PRI   Current process priority
$0162   2B      WAIT_FL   Wait flags for current process
$0164   2B      EVENT_FL   Event flags
$0166   4B      EVENT_DTA   Event DTA
$016A   2B      EVENT_DTA2   Event DTA high word
$016C   2B      PDB_PARENT_Pri   Parent priority
$016E   2B      PDB_CHILD_PID   Child process ID
$0170   2B      PDB_SIBLING_PID   Sibling process ID
```

## 5. Process Management Functions

### XBIOS EXEC Function

```
XBIOS function 95 (EXEC):
    Load and execute a program as a new process.

    D0 = drive number (-1 = current drive)
    D1 = transfer address
    D2 = load address
    D3 = program name (null-terminated, max 64 bytes)
    D4 = basepage pointer
    D5 = basepage length

    Return:
    D0 = PID of new process (positive) or error code (negative)

; Example: Execute DESKTOP.PRG as new process
    move.w #-1,d0           ; Current drive
    move.l #0,d1            ; No transfer address
    move.l #0,d2            ; No load address override
    lea prog_name,a0        ; "DESKTOP.PRG"
    move.l a0,d3            ; Program name
    move.l #0,d4            ; No basepage
    move.w #0,d5            ; No override
    move.w #95,d6          ; XBIOS EXEC
    trap #16                ; BIOS call
    ; Return PID in D0
```

### XBIOS TERM Function

```
XBIOS function 96 (TERM):
    Terminate the calling process.

    D0 = exit code (returned to parent)

; Example: Exit process
    move.w #0,d0            ; Exit code = 0 (success)
    move.w #96,d6          ; XBIOS TERM
    trap #16                ; BIOS call
```

### GEMDOS Process Functions

```
GEMDOS function $01/02: Term (terminate all processes)
GEMDOS function $31: Keep process (set termination code)
GEMDOS function $3E: EXEC (execute program and wait)
GEMDOS function $3F: TERM (terminate current process)

Standard GEMDOS EXEC:
    D0 = drive number
    D1 = transfer address
    D2 = load address
    D3 = program name (offset)
    D4 = basepage address
    trap #14                ; GEMDOS EXEC
    ; Return: D0.W = PID
```

### Process Status Query

```
XBIOS 95 (PROCESS):
    Get status of a specific process.

    D0 = PID to query
    Return:
    D0 = process state
    D1 = process priority
    D2 = process flag word

; Get process status
    move.w #4,-(sp)          ; Query PID
    move.w #95,d0           ; XBIOS PROCESS
    trap #16                ; BIOS call
    addq.l #2,sp
```

## 6. MultiTOS Memory Management

### Per-Process Memory Segments

```
Each process has its own memory allocation table:
    ┌───────────────────────────────┐
    │   Program text segment         │  ← Read-only code
    ├───────────────────────────────┤
    │   Data segment                 │  ← Global variables
    ├───────────────────────────────┤
    │   BSS segment                  │  ← Zero-filled storage
    ├───────────────────────────────┤
    │   Heap (dynamic)               │  ← Malloc allocations
    ├───────────────────────────────┤
    │   Stack (user)                 │  ← Function calls
    ├───────────────────────────────┤
    │   Stack (supervisor)          │  ← Trap frame
    └───────────────────────────────┘

Memory is NOT shared between processes by default.
```

### GEMDOS Memory in MultiTOS

```
Each process's memory space in MultiTOS:
    GEMDOS MALLOC: allocates from per-process free pool
    GEMDOS MFREE: releases back to per-process pool
    Per-process: no memory is visible to other processes
    Memory protection on TT/Falcon: can mark pages as read-only
```

## 7. Synchronization Mechanisms

### XBIOS 96 (WAKEUP) - Wait until another process signals

```
XBIOS function 96 (WAKEUP):
    Wait until another process calls WAKE.

    D0 = PID of process to wait for (0 = any process)
    D1 = event_id (arbitrary tag to identify event)
    D2 = timeout in milliseconds (0 = infinite)

    Return:
    D0 = timeout occurred? 1 = yes, 0 = no
```

### XBIOS 97 (WAKE) - Signal a waiting process

```
XBIOS function 97 (WAKE):
    Wake up a waiting process.

    D0 = PID of process to wake
    D1 = event_id (must match wakeup call)
```

### XBIOS 98 (SEMAPHORE)

```
XBIOS function 98 (SEMAPHORE):
    Simple semaphore for cross-process synchronization.

    D0 = operation:
        0 = init semaphore (value in D1)
        1 = wait (P operation)
        2 = signal (V operation)
    D1 = sem_id (arbitrary semaphore ID)

    Return:
    D0 = result (0 = success, -1 = error)
```

### XBIOS 99 (EVENT_FL) - Send event to another process

```
XBIOS function 99 (EVENT_FL):
    Set event flags in another process.

    D0 = PID of target process
    D1 = event_flags (bit mask)
    D2 = event_data (arbitrary data word)
```

## 8. Process Priority Manipulation

### XBIOS PRIORITY Functions

```
XBIOS function 14 (SETPRI):
    Set process priority.
    D0 = new priority level (1-7)
    D1 = PID (-1 = current process)
    Return: D0 = old priority

XBIOS 51 (GETPRI):
    Get current process priority.
    D0 = PID (-1 = current process)
    Return: D0 = priority level
```

### Priority Scheduling Example

```asm
; Set all processes to specific priority
set_process_priority:
    lea pdb_ptr,a0             ; PDB pointer
    move.w d0,d1                ; target PID
    move.w d2,d0               ; new priority
    move.w #14,d3            ; XBIOS SETPRI
    trap #16
    ; D0 = old priority

; Priority inheritance example
; Child is created at parent priority automatically.
; If child wants higher priority, must call SETPRI.
```

## 9. MultiTOS Initialization Sequence

### Boot-Time MultiTOS Setup

```
MultiTOS initialization sequence:
    1. ROM bootstrap (cold/warm start)
    2. Boot sequence reads boot sector
    3. TOS kernel initializes:
        a. Memory management (GEMDOS)
        b. I/O subsystem (BIOS)
        c. MultiTOS scheduler
        d. PDB chain initialization
        e. Desktop process creation
    4. DESKTOP.PRG loaded as first user process
    5. Scheduler starts (priority 4, first process)
    6. AES initialized (window manager)
    7. MultiTOS event loop enters idle state
```

### MultiTOS PDB List Initialization

```
After initialization:
    PDB chain initialized at $0140 (SYS_PDB):
        PDB entry at $0140:
            0x00: pointer to next PDB
            0x04: pointer to previous PDB
            0x08: pointer to TOS block
            0x0C: PID = 1 (INIT process)
            0x0E: priority = 6 (AES/GEM)
            0x10: state = RUNNING
    All active PDBs linked in circular list
```

## 10. FreeMiRT MultiTOS

### FreeMiNT Overview

FreeMiRT (MiNT) is a third-party multitasking operating system for Atari ST:

```
FreeMiNT features vs stock MultiTOS:
    Feature             FreeMiNT          Stock MultiTOS
    ───────             ──────────── ──────────
    Preemptive multitasking  Yes           Yes
    Process limit         64 (default)        16 (max)
    Memory protection    Yes (PMMU)          No
    Virtual memory       Yes                No
    Networking          Full TCP/IP           Limited
    POSIX compliance    Partial              No
    File system support FAT16, HFS, XFS   Only FAT
    68040 support       Yes                  No
    Process creation    fork()/exec()        EXEC only
    Thread support      Yes                  No
    Signal handling     POSIX signals        None
```

### FreeMiNT Process Table

```
FreeMiNT process descriptor (kernel-level):
    Structure fields:
        pid             Process identifier
        ppid         Parent PID
        priority        Priority level (0 = highest)
        state          Process state (RUN, SLEEP, ZOMBIE)
        signal_mask   Signal mask flags
        flags           Status flags
        user             User credentials
        resources   Resource limits
        fd_table      Open file descriptors
        memory_map   Memory mapping table
        threads    Thread table (multiple threads)
```

## 11. Practical MultiTOS Programming

### Cross-Process Communication (IPC) Example

```asm
; Process 1: Send data to Process 2
process_1_send_data:
    lea data_buffer,a0            ; Data to send
    move.l d0,data_len            ; Data length
    move.w #200,d6              ; PID of process 2
    move.l #shared_mem,d0       ; Shared memory address
    move.w #10,d1              ; Shared data ID
    move.w #70,d0               ; XBIOS 70 = IPC send
    trap #16
    rts

; Process 2: Receive data from Process 1
process_2_recv_data:
    move.w #200,d6             ; PID of process 1
    move.l #shared_mem,d0    ; Shared memory address
    move.w #10,d1             ; Shared data ID
    move.w #71,d0             ; XBIOS 71 = IPC receive
    trap #16
    ; Data in shared_mem
```

### Process Monitoring and Management

```asm
; Monitor all processes:
monitor_processes:
    move.w #1,d6              ; Start with PID #1
monitor_loop:
    move.w d6,d0              ; PID to query
    move.w #95,d1             ; XBIOS 95 = PROCESS
    trap #16
    ; D0 = state, D1 = priority
    cmpe.w #0,d0              ; Invalid PID?
    beq monitor_done          ; No more processes
    ; Record state/priority
    addq.w #1,d6              ; Next PID
    cmp.w #64,d6             ; Max is 64
    blt monitor_loop
monitor_done:
    rts

; Terminate a specific process
terminate_process:
    move.w target_pid,d6      ; PID to terminate
    move.w #0,d0               ; Exit code
    move.w #151,d1             ; XBIOS TERM
    trap #16
    rts
```

## 12. MultiTOS Disk I/O in Multitasking

### Per-Process File Descriptors

```
Each process maintains its own file descriptor table:
    File descriptor table entry:
        Descriptor    I/O unit     Reference
        0             STDIN        BIOS input device
        1             STDOUT       BIOS output device
        2             STDERR       BIOS error device
        3-N           Variable     Open files
```

### Concurrent Disk Access

```
MultiTOS handles concurrent disk access via:
    1. Per-process file handle table
    2. Disk sector lock (one process at a time)
    3. BIOS I/O serialization
    4. GEMDOS file locking (per-process)
    5. Drive media change detection across processes

    Warning: Concurrent writes to same partition can corrupt FAT.
    Always ensure exclusive access with XBIOS semaphore.
```

## 13. Multitos vs Native MultiTOS

### Multitos (Third-Party)

```
Multitos (by Atari):
    Alternative to stock MultiTOS
    Adds true preemptive multitasking to stock TOS
    Process management features:
        - Process creation/deletion
        - Priority boosting/boosting
        - Process suspension/resumption
        - Process memory management

Multitos memory layout:
    $000000-$007FFF: Stock TOS system area
    $008000-$01FFFF: Multos extensions
    $020000-$0FFFFF: Free memory pool
    $100000+$0FFFFFF: Process memory allocation
    Each process gets: text segment + data segment + heap + stack
```

### Geneva (Third-Party)

```
Geneva (by Gribnif Software):
    Full multitasking environment for Atari ST
    Features:
        - Preemptive multitasking
        - Memory protection (with MMU hardware)
        - Virtual memory (with MMU)
        - File system support (FAT, HFS)
        - Network support (TCP/IP)
        - POSIX-compatible process API
        - Multiple desktop environments

    Geneva memory management:
        4KB page-based allocation
        Page table per process
        MMU page table entries (PTE)
        Virtual memory with swap to disk
        Demand paging with page fault handler
```

## 14. MultiTOS Process Creation Flow

### EXEC Process Creation

```
EXEC (XBIOS 150) process creation flow:
    1. Allocate PDB from PDB pool
    2. Load PRG file from disk
    3. Allocate memory for process
    4. Copy code/data to allocation
    5. Initialize PDB:
        PID = new PID
        priority = parent priority
        state = READY
        PDB_NEXT = NULL
        PDB_PREV = NULL
    6. Add PDB to active PDB list
    7. Set PC to program entry point
    8. Switch to process context
    9. Return PID to caller
```

### Process Cleanup on Termination

```
Process termination flow:
    1. Set PDB_STATE = TERMINATED
    2. Send SIGCHLD signal to parent
    3. Deallocate process memory
    4. Remove PDB from active list
    5. Release process file descriptors
    6. If has child processes: adopt by init (PID 1)
    7. Free PDB entry for reuse
```
