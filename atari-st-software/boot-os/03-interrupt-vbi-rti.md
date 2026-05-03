# TOS Interrupt Handling, VBI, and RTI

## 1. 68000 Interrupt Architecture

The MC68000 supports 7 interrupt levels (1-7, plus NMI at level 8). The Atari ST uses levels 2-7 for hardware interrupts.

### 68000 Interrupt Priority (Lower Number = Higher Priority)

```
Level   Source                     Masking
────── ──── ─────────────────── ────────
NMI     Vertical blank (via NMI line)    Non-maskable (level 8)
7 (IRQ) MFP level 7 interrupt            Maskable
6       IKBD keyboard interrupt          Maskable
5       MFP/Midi ACIA interrupt           Maskable
4       DMA/SCSI interrupt (STE/TT)    Maskable
3       Timer counter interrupt            Maskable
2       Serial bus interrupt               Maskable
1       Unused (reserved)                  Maskable
0       Software (TRAP instructions)      Maskable
```

### Interrupt Vector Table (IVT)

The 68000 IVT starts at address $000000:

```
Offset    Content           Size     Description
─────── ─────── ────────── ├─── ────────────────
$0000     SSP init value     4B     Initial Supervisor Stack Pointer
$0004     PC init value      4B     Initial Program Counter (boot jump)
$0008-$001E   Unused         2B each    Reserved
$0020     TRAP #0 vector     4B     VDI (VDIINT)
$0022     TRAP #1 vector     4B     XBIOS (XBIOS)
$0024     Unused             2B       Reserved
$0026     TRAP #3 vector     2B       Reserved
$0028     Unused             2B       Reserved
$002A     TRAP #5 vector     2B       Reserved
$002C     Unused             2B       Reserved
$002E     TRAP #7 vector     2B       Reserved
$0030     TRAP #8 vector     4B     Reserved (unused)
...
$006C     TRAP #14 vector    4B     GEMDOS
$0070     Unused             2B       Reserved
$0072     TRAP #15 vector    4B     BIOS
$0074     Unused             2B       Reserved
$0FC0     Unused             2B       Reserved
$0FC2     Unused             2B       Reserved
$0FFC     Unused             2B       Reserved
$0FFE     Unused             2B       Reserved
$1400      MFP interrupt     4B     Extended (TT030)
$17C0      TT MFP            4B     Extended (TT030)
$1800      TTClock           4B     Extended (TT030)

Hardware Vectors:
$FFFE00     TRAP #0           4B      Reserved
$FFFE04     TRAP #1           4B      XBIOS (XBIOS)
$FFFE08     TRAP #3           2B       Reserved
$FFFE0A     TRAP #4           2B       Reserved
...
$FFFC3E     TRAP #14          4B      GEMDOS
$FFFC3C     TRAP #13          4B      BIOS
$FFFC3A     TRAP #15          4B      BIOS
$FFFE78     NMI vector        4B      NMI handler
$FFFE7A     Unused            2B       Reserved
$FFFC80     RESET vector      4B      Cold boot reset
$FFFE7C     unused            2B       Reserved
$FFFCFE     IRQ vector        4B      Main IRQ handler
```

### Interrupt Request Level (IRL) Priority

```
The 68000 compares incoming IRL numbers (larger wins).
NMI always overrides all levels.

NMI (level 8)  > IRQ7 > IRQ6 > IRQ5 > IRQ4 > IRQ3 > IRQ2 > IRQ1
```

### SR Status Register Bit Fields

```
Bit    Field    Description
─── ─── ───────────────────────────
15     I       Interrupt Mask (7 = fully masked, 0 = all enabled)
14     N       Negative
13     Z       Zero
12     V       Overflow
11     C       Carry
10-4    Unused   Always 0
3      X       Extended carry
2      U       User mode (0) / Supervisor mode (1)
1-0    -       Mode (00/01 = user, 10/11 = supervisor)
```

## 2. Hardware Interrupt Sources

### MFP (68901) Interrupts

The Multi-function Programmable Peripheral at $FF9A00 generates level 7 interrupts.

#### MFP Registers

```
Address    Register    R/W    Description
─────────────── ──────────────── ──── ─────────────────────────
$FF9A01    GP_IO_A    RW     General purpose I/O port A (keyboard, mouse)
$FF9A03    APE       RW     Active edge register (IRQ edge select)
$FF9A05    DDA       RW     Data direction port A (1=out, 0=in)
$FF9A07    IER_A     RW     Interrupt enable register A
$FF9A09    IER_B     RW     Interrupt enable register B
$FF9A0B    IPR       R       Interrupt pending register (read-only)
$FF9A0D    ICR       W       Interrupt clear register (write to clear)
$FF9A0F    ISR       R       Interrupt service register (write to clear)
$FF9A11    ISR_B     R       Interrupt service register B
$FF9A13    IER_A_mask RW    Interrupt mask register A
$FF9A15    IER_B_mask RW    Interrupt mask register B
```

#### MFP Timer Registers

```
Address    Register    R/W    Description
─────── ──────────── ──── ─────────────
$FF9A17    IRQ_VEC    RW    Interrupt vector base address (lower 5 bits)
$FF9A19    TNA       RW    Timer A control register
$FF9A1B    TNB       RW    Timer B control register
$FF9A1D    SNC       RW    Timer C & D control register
$FF9A1F    TCA       R/W    Timer A count register
$FF9A21    TCB       R/W    Timer B count register
$FF9A23    TCC       R/W    Timer C count register
$FF9A25    TCD       R/W    Timer D count register
$FF9A27    SYNC      RW    Synchronization character
$FF9A29    UCSR      RW    USART control register
$FF9A2B    RSR       R       Receiver status register
$FF9A2D    TSR       R       Transmitter status register
$FF9A2F    USART     R/W    USART data register (Tx/Rx)
```

#### Timer Operating Modes

```
Timer A (write register $FF9A19):
    Bit 0: 0 = stop, 1 = start countdown
    Bit 1: 0 = one-shot, 1 = free-running
    Bit 2: 0 = falling edge, 1 = rising edge (interrupt)
    Bit 3: 0 = 16 clocks per count, 1 = /16 prescaler
    Bit 4: 0 = 16 clocks, 1 = /256 prescaler
    Bit 5: 0 = 16 clocks, 1 = /4096 prescaler
    Bits 6-15: Count value
```

### IKBD (6502-based) Interrupt

The IKBD generates IRQ at level 6 via MFP IRQ B line.

#### IKBD Communication

```
Address    Register    R/W    Description
─────── ──────── ──── ──────────────────────
$FF8800    IOP_B    RW    I/O Port B data
$FF8802    IOP_B_rw RW    PSG/IKBD bidirectional data
$FF860C    ICR      RW    IKBD command register
$FF860E    IIR      R       IKBD interrupt request status
```

#### IKBD Command Register ($FF860C)

```
Bit    Description
─── ────────────────
7      Send complete flag (set by IKBD, cleared by CPU)
6      Data ready flag (set when data available)
5      IKBD ready (1 = IKBD can receive)
4-0    Command number
```

#### IKBD Interrupt Vector Chain

```
Vector table returned by XBIOS 34 (KBDVBASE):
    Offset   Size  Purpose
    ───── ── ──────── ───────
    +0x0      4B  MidiIn (MIDI input handler)
    +0x4      4B  KbErr (keyboard error handler)
    +0x8      4B  MidiErr (MIDI error handler)
    +0xC      4B  KBStat (keyboard status handler)
    +0x10     4B  Mouse (mouse vector)
    +0x14     4B  Cock (clock/time handler)
    +0x18     4B  Joy (joystick handler)
    +0x1C     4B  Snd (sound/IKBD handler)
    +0x20     4B  Snd2 (secondary sound handler)
```

## 3. Vertical Blank Interrupt (VBI)

### VBI Generation Mechanism

The VBI is triggered by the Shifter when the CRT beam reaches the bottom of the screen during the retrace period. This is a **non-maskable interrupt** (NMI) on the Atari ST.

```
VBI Timing (NTSC/PAL variants):

NTSC (mode 0-4):
    Frame rate: ~57.48 Hz
    Frame time: ~17.39 ms
    VBI duration: ~3-4 ms
    Machine cycles per frame: ~138,720 (at 8 MHz)

PAL (mode 5-10):
    Frame rate: ~50.57 Hz
    Frame time: ~19.77 ms
    VBI duration: ~4-5 ms
    Machine cycles per frame: ~158,160 (at 8 MHz)
```

### VBI NMI Flow

When the VBI NMI fires:

```
1. 68000 auto-saves SR and PC on system stack
2. NMI handler at $FFFA starts (ISR at $E7B4)
3. NMI status register at $D40F ($FF9A0C) is read:
    Bit 7: DLI occurred
    Bit 6: RESET key pressed
    If neither: VBI is assumed
4. Jump to VVBLKI vector at $0222 (normally $E7D1)
5. Stage 1 VBI processing:
    a. Increments RTCLOK counter at $12-$14 (tick every ~55.27 us)
    b. Processes attract mode
    c. Decrements system timer 1 at $218-$219
    d. If timer 0, indirect JSR via $226-$227
6. Test critical flag at $42
7. If not critical: jump to VVBLKD vector at $0224 (normally $E93E)
8. Stage 2 VBI:
    a. Enable IRQ interrupts
    b. Auto-repeat keyboard logic
    c. Keyboard debounce processing
    d. Process system timers 2-5
    e. Color palette update (shadow registers)
    f. Game controller/Joy port data read
9. Restore SR and PC
10. Execute RTI instruction
```

### VBI System Variables

```
Offset    Size    Name        Description
─────── ── ──── ──────────────── ────────────
$0222     2B      VVBLKI      Vertical blank immediate vector ($E7D1)
$0224     2B      VVBLKD      Vertical blank deferred vector ($E93E)
$12-14    3B      RTCLOK      System tick counter (~18.2 Hz)
$21-22    3B      RTCLOK+2    RTCLOK high byte
$42       1B      CRITIC      Critical flag (time-critical code)
$218-219  2B      SYS1TMR     System timer 1
$26A-26B  2B      SYS2TMR     System timer 2
$26C-26D  2B      SYS3TMR     System timer 3
$26E-26F  2B      SYS4TMR     System timer 4
$270-271  2B      SYS5TMR     System timer 5
$345-346  2B      VBL         VBL counter for double-buffering
$461-462  2B      TICKSPER    Ticks per second multiplier
$4B2-4B3  2B      TICKPERM    Milliseconds per tick
$626-627  2B      CURTMR0     System timer 0
```

### Installing Custom VBI Handler

```asm
; Install custom deferred VBI handler
; VVBLKD is at $0224

install_vbi_handler:
    ; Save old deferred VBI vector
    move.l $0224,old_vkbd_vec

    ; Install our handler
    lea my_vbi_handler,a0
    move.w $FF,a0         ; high byte
    move.l a0,$0224       ; write vector

    ; Set up the immediate callback via XBIOS
    moveq #6,d0           ; update immediate VBI vector
    move.l #$FFE7D1,x,d0  ; address of handler
    move.w #$88,a0        ; XBIOS setvector
    trap #16              ; BIOS call

    rts

; Our deferred VBI handler
my_vbi_handler:
    ; Save regs used
    movem.l d1-d7/a1-a6,-(sp)

    ; Your VBI code here
    ; (must be fast - run during VBI time only)
    ; Approx 7980 machine cycles available in graphics mode 0

    ; Restore regs
    movem.l (sp)+,d1-d7/a1-a6
    rte

; Save old vectors for restoration
old_vkbd_vec    dc.l 0
```

## 4. Real-Time Interrupt (RTI)

### RTI Overview

RTI is an additional interrupt source available on 1040STE and later models, generated by the STE-specific hardware (GT64A022 ST-MFP).

```
RTI Features:
    - Hardware timer-based interrupt (not display-driven)
    - Triggered by ST-MFP Timer D ($FF9A25/TCD)
    - Available only on STE and newer hardware
    - Typically set to fire every 1/60 second (~16.67 ms)
    - Useful for game timing and background processing

RTI Timer Calculation:
    ST-MFP clock = 16 MHz
    Timer D prescaler = /256
    Effective clock = 62,500 Hz
    For 60 Hz: timer value = 62,500 / 60 = 1041
```

### RTI Configuration

```asm
; Configure RTI on STE
    moveq #4,d0           ; MFP Timer D control
    move.b d0,$FF9A1D     ; SNC register
    move.w #1041,$FF9A25  ; Timer D count register
    move.b d0,$FF9A19     ; Clear timer A control
    move.b #$40,$FF9A07   ; Enable RTI on IRQ B
    rts
```

## 5. Interrupt Service Routine Design

### ISR Architecture on Atari ST

```
Standard ISR structure:
    1. Auto-saved by 68000: SR, PC onto system stack
    2. ISR identifies source by reading STATUS REGISTER
    3. ISR saves any used registers
    4. ISR performs hardware-specific handling
    5. ISR clears the interrupt flag
    6. ISR restores saved registers
    7. ISR executes RTE (Return from Exception)
```

### Critical Sections

```
A critical section in TOS occurs when:
    - The critical flag ($42) is set
    - The I bit in SR is set (mask = 0)
    - Serial I/O is in progress

During a critical section, the VBI must return immediately
to avoid data corruption.

Checking for critical section:
    btst #6,SR       ; test I bit
    bne critical     ; if masked, return immediately
    btst #0,$42      ; test critical flag
    bne critical     ; if set, return immediately
```

### Timer Callback Vector Chain

```
Timer callback addresses and priorities:
    $226-$227: Timer 1 callback (highest priority)
    $26A-$26B: Timer 2 callback
    $26C-$26D: Timer 3 callback
    $26E-$26F: Timer 4 callback
    $270-$271: Timer 5 callback (lowest priority)

Setting timer callback via SETVBV (XBIOS):
```

```asm
; Set timer N callback via SETVBV
; A = timer number (1-5) or vector selector
; X = high byte of handler address
; Y = low byte of handler address
    moveq #1,d0           ; Timer 1
    move.w #6,-(sp)       ; SETVBV function
    move.w #$FFE7C0,d1    ; Handler high byte
    move.w #0,d2           ; Handler low byte
    trap #16              ; BIOS
```

## 6. Trap Handling

### Trap Entry Points

```
Trap instruction triggers the 68000 exception mechanism:
    trap #n     ; n = 0 to 15
    PC = IVT[n*4]  where IVT = interrupt vector table base
    SR = saved on stack
    PC = saved on stack
```

### Key Trap Vectors

```
Trap    Vector Address    Entry Point      Purpose
─── ──── ─────────── ───── ────────────────────────────────
#0      $006C          $8D9C             VDI interface
    #1      $0070          $8B62             XBIOS interface
    #2      $0074          $8B60             Unused
    #3      $0076          $8B5E             Unused
    #4      $0078          $8B5C             Unused
    #5      $007A          $8B5A             Unused
    #6      $007C          $8B58             Unused
    #7      $007E          $8B56             Unused
    #8      $0080          $8B54             Unused
    ...
    #13     $00DE          $FF8000           BIOS
    #14     $00E4          $FF83EE           GEMDOS
    #15     $00E6          $FF8500           BIOS (redundant)

Extended (TT030):
    #16     $00E8          $FFC3EE           XBXOS/TT030
    ...
```

### Trap Calling Convention

```
Standard TRAP function calling convention:
    trap #14            ; GEMDOS call
    trap #10 or #15   ; BIOS call
    trap #1             ; XBIOS call (entry at $8B62)
    trap #0             ; VDI call (entry at $8D9C)

Function number in D0.W for all traps.
Parameters pushed onto stack before the trap.
Return values in D0.W or D0.D.
```

## 7. MFP Interrupt Sources in Detail

### MFP IRQ A (Port A - $FF9A07)

```
Bit    Source                         Example
─── ──── ────────────────────────── ──────
0      ACIA 0 (keyboard) data ready  Keyboard key press
1      ACIA 0 (keyboard) TX empty    Keyboard buffer drain
2      ACIA 1 (midi) data ready      MIDI byte received
3      ACIA 1 (midi) TX empty        MIDI transmitter empty
4      Timer C (system tick)         VBI system tick
5      Sync character                USART sync detect
6      Rising edge GP I/O pin        External event
7      Falling edge GP I/O pin       External event
```

### MFP IRQ B (Port B - $FF9A09)

```
Bit    Source                         Example
─── ──── ────────────────────────── ──────
0      Timer D (real-time)           STE RTI
1      Falling edge GP I/O           Hardware interrupt
2      Rising edge GP I/O            Hardware interrupt
3-7    Unused/Reserved
```

### MFP IPR (Interrupt Pending Register - $FF9A0B)

```
Reading IPR shows pending (not yet cleared) interrupt sources:
    Bit 0: ACIA 0 data ready
    Bit 1: ACIA 0 TX empty
    Bit 2: ACIA 1 data ready
    Bit 3: ACIA 1 TX empty
    Bit 4: Timer C
    Bit 5: Sync character
    Bit 6: RISING edge GP I/O
    Bit 7: FALLING edge GP I/O

To clear: write to ISR ($FF9A0D) with appropriate bit set.
```

## 8. Interrupt Priority and Masking

### Interrupt Mask Setting

```
Setting the interrupt mask to prevent interrupts during critical ops:
    move.w #$700,-(sp)    ; Set I = 7 (mask all)
    trap #14              ; GEMDOS entry (auto pushes SR)
    ; OR simply:
    move.w #$2700,SR      ; User mode, I = 7 (stack still valid)

Clearing:
    move.w #$2000,SR      ; User mode, I = 0 (unmask)

Note: NMI ($FFA000) cannot be masked by I bit
```

### IRQ Enable/Disable via XBIOS

```
; Disable all interrupts:
    moveq #26,d0          ; XBIOS JENABINT / DISABLE INT
    trap #1

; Enable interrupts:
    moveq #27,d0          ; XBIOS JENABINT / ENABLE INT
    trap #1
```

## 9. Interrupt Timing Analysis

### Machine Cycle Timing

```
68000 at 8 MHz:
    One machine cycle = 125 ns
    One MFP clock tick = 12 / 8 MHz = 1.5 ns (prescaler dependent)
    Timer C at /16: 1 tick = 2 ns * 16 = 32 ns
    Timer D for 60 Hz: 1 / (8 MHz / 256) * 1041 = 33.3 us
```

### VBI Window Timing

```
Graphics mode 0 text (320x200):
    Horizontal sync: ~64 us
    Visible line: ~529 us
    Frame: ~17.39 ms (NTSC) or ~19.77 ms (PAL)
    VBI window: ~4 ms
    Safe processing time: ~3 ms (avoid full-screen blits)

Safe VBI routine estimate:
    3ms / 125ns = 24,000 machine cycles available
    ~50% overhead for trap entry/exit = ~12,000 cycles usable
```

## 10. Interrupt Service Routine Template

```asm
; Complete ISR template for keyboard data ready
; (IRQ A, bit 0 via MFP IPR register)

kbd_isr:
    ; Save all registers
    movem.l d0-d7/a0-a6,-(sp)

    ; Check if this is really our interrupt source
    move.b $FF9A0B,d0     ; IPR
    btst #0,d0            ; ACIA 0 data ready?
    beq.kbd_exit          ; no, exit

    ; Acknowledge by clearing IPR
    move.b d0,$FF9A0D     ; write to ISR (auto-clear)

    ; Read keyboard character
     move.b $FF9A00,d0     ; ACIA 0 data register
    ; Process character...

kbd_exit:
    ; Restore registers and return
    movem.l (sp)+,d0-d7/a0-a6
    rte

; Enable keyboard interrupt:
kbd_enable:
    move.b #$01,$FF9A07   ; IER bit 0 = enable ACIA 0
    move.w #$2E00,SR      ; Set SR.I to level 5 (I = 5)
    rts
```

### Complete Timer Callback Example

```asm
; Timer-based game tick handler (every ~55ms)
; Set via SETVBV with function 1

timer_tick_handler:
    movem.l d0-d7/a0-a6,-(sp)

    ; Game state update
    add.q #1,tick_count
    btst #4,tick_count       ; every 16 ticks = ~881ms
    beq.s timer_no_anim
    move.w #1,frame_update
    clr.w frame_update + 1

timer_no_anim:
    ; Check joystick every 32 ticks
    btst #5,tick_count
    beq.s timer_no_joy
    bsr read_joystick
    move.w #1,joy_update
    clr.w joy_update + 1

timer_no_joy:
    ; Check key states
    bsr check_keyboard

timer_exit:
    movem.l (sp)+,d0-d7/a0-a6
    rte

tick_count          dc.w 0
frame_update        dc.w 0
joy_update          dc.w 0
```

## 11. Interrupt and DMA Interaction

### DMA-Enabled Units

```
DMA units on Atari ST:
    1. FDC (WD1772) - $FF8604-$FF860F
        Sector-based read/write
        256 sectors max per transfer
        Auto-incrementing address register
        DSR (DMA Status) register: DSR bit 2 = FIFO full
        DSR bit 3 = FIFO empty

    2. Blitter ($FFFF8400-$FFFF843F)
        Block-level memory copy
        16-bit word transfers
        Auto-increment source/dest
        No interrupt (polling or completion flag)

    3. Shifter DMA ($FFFF8200)
        Video readout (always active)
        Frame buffer DMA at fixed rate
        VBL counter register: $FFFF8205-$FFFF8209
```

### DMA Completion Detection

```asm
; Check FDC DMA completion
fdc_complete:
    btst #4,$FF8607           ; DSR bit 4 (busy)
    bne.fdc_waiting             ; still busy
    ; DMA complete
    move.b $FF860A,d0         ; Read data
    rts
```

## 12. ISR and Memory Management Interaction

### Interrupt Context Restrictions

```
Rules for interrupt service routines:
    1. NEVER call GEMDOS functions (may cause deadlock)
    2. NEVER call AES functions (non-reentrant)
    3. Always push/pop all used registers
    4. Use RTE, not RTS
    5. Keep ISR short (VBI timing is critical)
    6. Set SR.I = 7 during critical memory operations
    7. NEVER use floating-point in ISR (no FP on ST)
    8. Always clear interrupt source before returning
```

### Safe Interrupt Communication with Main Program

```
Interrupt-to-main communication:
    Use flag variables in shared memory
    Set flags in ISR, poll in main loop
    Use atomic test-and-set for flag setting

Example:
    isr_flag_flag        ds.w 0
    my_isr_handler:
        .set flag_flag
        move.w #1,my_isr_flag_flag
        b my_isr_exit_flag
```
