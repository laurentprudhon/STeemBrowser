# 68000 Assembly Programming Conventions for Atari ST

## 1. 68000 Instruction Set Fundamentals

### Addressing Modes

```
Immediate:    move.w #val,d0
             lea   addr,a0
Displacement: move.l offset(a0),d0   ; base + displacement
             move.l 4(a0),d0        ; a0 + 4
Base + Displacee move.l offset(a0,d0):  ; a0 + d0
Index:       move.l (a0,d0.w),d1    ; a0 + (d0)
             move.l (a0,d0.l),d1    ; a0 + (d0)
PC-based:    move.l offset(pc),d0     ; literal pool
             move.l label(pc),d0      ; load constant
             lea  label(pc),a0        ; load address of label
```

### Register Map

```
Data registers: D0-D7 (D0 = return value, D1-D2 = arguments)
Address registers: A0-A7 (A7 = stack pointer)
    A0-A5: General purpose
    A6:   Frame pointer (in GEM/BIOS code)
    A7:   Stack pointer (read/write in user mode)

Status register: SR (bit 15 = interrupt mask)
Program counter: PC (read/write in user mode)
```

### Stack Discipline

```
Stack grows toward lower addresses.
PUSH = subtract from A7, then store.
POP  = load from A7, then add to A7.

Standard calling convention:
    Caller pushes arguments (right-to-left)
    Callee reads arguments from stack
    Callee preserves A1-A6 (callee-saved)
    Callee modifies D0-D1, A0 (caller-saved)
    Return value in D0.W (or D0.D for long)
```

## 2. Supervisor Mode Entry and Exit

### Entering Supervisor Mode

```asm
; Method 1: GEMDOS function $20 (SUPER)
    move.w #$20,-(sp)
    trap #14
    ; D0 = old user stack pointer (returned)

; Method 2: Direct mode change (unsafe without A7 save)
    move.l $530,d0          ; Get supervisor stack pointer
    move.w #$2700,SR        ; Set supervisor mode
    ; A7 now points to supervisor stack

; Method 3: XBIOS supervisor call
    moveq #41,d0            ; XBIOS RESMCR (supervisor)
    trap #1
```

### Exiting Supervisor Mode

```asm
; Method 1: Using saved A7 (from SUPER call)
    move.l old_stack,a7     ; Restore user stack
    move.w #$2000,SR        ; Return to user mode

; Method 2: Return from any user trap
    rts                     ; Returns to user mode automatically
```

## 3. Blitter DMA Programming

### Blitter Registers

```
Address    Name        R/W    Bits        Description
───────  ──── │ ─── ─────  ────────── ──────────
$FFFF8400  BLTCOA     R/W   0-23      A register (1st operand)
$FFFF8402  BLTCOB     R/W   0-23      B register (2nd operand)
$FFFF8404  BLTCOC     R/W   0-23      C register (opcode/shape)
$FFFF8406  BLTCOD     R/W   0-23      D operand (dest)
$FFFF8408  BLTAFWM    R/W   0-15      A filter mask
$FFFF840A  BLTBDFM    R/W   0-15      B filter mask
$FFFF840C  BLTCON0    R/W   0-15      Blitter control 0
$FFFF840E  BLTCON1    R/W   0-15      Blitter control 1
```

### BLTCON0 - Blitter Control Register 0

```
Bits 0-3   A count mode (ACM):
    0001 = A count register set to 4-byte increments
    0002 = A count register set to 1-byte increments
Bit  4 = 1: B count
Bit  5 = 1: Start blitter
Bit  6 = 1: A count enable
Bits 7-8 = B count mode (BCM)
Bit  9 = 1: A/B count enable
Bit  10 = 1: Start blitter (alternative)
Bit  11 = 1: D count
Bits 13-15 = D mode (XDMA)
```

### BLTCON1 - Blitter Control Register 1

```
Bit   0   = Line mode (1 = line mode, 0 = AND/OR mode)
Bits 1-3  = Horizontal count
Bits 4-5  = Vertical count
Bit   6   = A count mode (0 = XDMA, 1 = YDMA)
Bits 7-8  = B count mode (0 = XDMA, 1 = YDMA)
Bit   9 = B count enable (1)
Bits 10-11 = D count
Bits 12-13 = D mode (0 = XDMA, 1 = YDMA)
Bits 14-15 = A mode
```

### Standard Blitter Patterns

```
Pattern 0: Fill (BLTCON0 = $0000, BLTCON1 = $0105, BLTC = $00FF)
Pattern 1: Copy (BLTCON0 = $0000, BLTCON1 = $0101, BLTC = $0000)
Pattern 2: AND (BLTC = $BBBB)
Pattern 3: OR (BLTC = $AAAA)
Pattern 4: XOR (BLTC = $8888)

; Fill screen with color $FF:
    move.w #$0000,$FFFF8416    ; BLTCON0
    move.w #$0105,$FFFF8416    ; BLTCON1
    move.w #$00FF,$FFFF8418    ; BLTDC
    move.w #$0000,$FFFF841A    ; BLTAOR
    move.w #$0000,$FFFF841C    ; BLTBOR
    move.w #$00FF,$FFFF841E    ; BLTCOR (fill value)
    move.w #$0000,$FFFF8404   ; BLTC (opcode)
    move.w #$0000,$FFFF8408    ; BLTD (dest base)
    movem.l #0,-(sp)           ; Save A0,A1
    ; BLTCON0 |= $0020          ; Start blitter
    ; Wait for completion
    btst #0,$FFFF8414    ; BLTCON0 busy bit
    bne.b wait_blt_done
    ; Done
    move.w sp,(sp)+         ; Restore A0,A1
```

### Blister Sprite Drawing Example

```asm
; Draw sprite using blitter
; A0 = sprite data (RLE compressed)
; A1 = dest buffer
; D0 = width, D1 = height
; D2 = palette offset

blitter_sprite:
    ; Set A to source address
    move.l a0,$FFFF8400    ; BLTCOA (A)
    ; Set D to dest address
    move.l a1,$FFFF8406    ; BLTCD (dest)
    move.w d1,d3             ; width
    lsl.w #5,d3              ; *32 to get stride
    add.l #width, $FFFF840A    ; BLTCOB (B) = stride
    move.w d0,$FFFF842C    ; BLTDRC = width
    move.w d1,$FFFF8430    ; BLTDC = height
    ; Set to copy mode
    move.w #$0700,$FFFF8412       ; BLTCON0 (start)
    move.w #$0000,$FFFF8414       ; BLTCON1 (copy)
    move.w #$0000,$FFFF8416       ; BLTD
    ; Clear BFC and AFC
    move.w #$0700,$FFFF840C        ; BLTAFWM = $0000
    move.w #$0000,$FFFF840D        ; BLTBFM = $0000
    ; Start
    move.w #$0020,$FFFF8412        ; BLTCON0 |= start
wait_blt1:
    btst #0,$FFFF8412               ; Check done
    beq.b wait_blt1
    rts
```

## 4. Standard Atari ST Call Conventions

### GEMDOS Calling Convention

```
GEMDOS entry:
trap #14
function_num  D0

Stack layout when calling:
    SP+0:     return address
    SP+2:     function number (W)
    SP+4:     parameter 1 (W or L)
    SP+6:     parameter 2 (W or L)
    ...

D0 = return value
```

### AES Calling Convention

```
AES function calling:
    Function number in D0 (WORD)
    Parameters in D0...D7 and A0...A6

Standard AES entry:
    move.w #function,d0
    move.w #param_1,d1
    move.w #param_2,d2
    move.l #ptr,a0
    trap #0  ; AES entry
    ; Return value in D0
```

### VDI Calling Convention

```
VDI function calling:
    Device handle in D0 (WORD) - from vdi_openwork
    Function number in D0 (WORD)
    Parameters in D1...D7 as INT16 or INT32

VDI entry:
    move.w #handle,d0
    move.w #function,d1
    lea params_pc,a2
    trap #0  ; VDI entry
    ; Return in params[]
```

### XBIOS Calling Convention

```
XBIOS entry:
trap #1
Function number in D0 (WORD)
Parameters in D0,D1,D2 and A0,A1,A2
```

### BIOS Calling Convention

```
BIOS entry:
trap #10      ; or trap #15
Function number in D0 (WORD)
Parameters in D0,..., D7 and A0,...,A6
```

## 5. Screen Plane Memory Patterns

### Planar Memory Layout (Low Res 320x200x16)

```
Memory is organized in 4 planes of 160x200 bytes each (32,000 bytes total).
Each pixel is encoded across all 4 planes simultaneously.

Plane 0 (color bit 0): Address $C000 - $FFFF
Plane 1 (color bit 1): Address $10000 - $13FFF
Plane 2 (color bit 2): Address $14000 - $17FFF
Plane 3 (color bit 3): Address $18000 - $1BFFF

For pixel (x,y):
    Word offset = x / 16 (0 to 31)
    Bit offset = x % 16 (0 to 15)
    Row offset = y * 160 (byte row width)
    Plane address = base + plane_index * 32000 + row_offset + word_offset
    Bit at plane[address + (15 - bit_offset)] (MSB first)
```

### Plane Access Macros

```asm
; Planar address calculation macro
; Planar address for (x,y,color):
;   plane_address = $C000 + y * 160 + x / 16 + color_bit * 32000
;   bit_mask = 1 << (15 - x % 16)

; To plot a pixel:
;   plane_addr = $C000
;   for bit = 0 to 3:
;       plane_addr += color_bit * 32000
;       word_addr = plane_addr + y * 160 + x / 16
;       mask = $8000 >> (x % 16)
;       plane_addr[word_addr] |= mask
```

## 6. Fast Routines

### Fill Screen with Blitter

```asm
; Fill entire 320x200 screen with pattern byte
; Uses blitter in pattern mode

fill_screen:
    ; Set registers
    move.w #$0000,$FFFF8416       ; BLTCON0
    move.w #$0105,$FFFF8418       ; BLTCON1 (start + D count)
    ; A = $0000 (pattern 0 = copy pattern)
    move.w #$0000,$FFFF841A       ; BLTAOR
    move.w #$00FF,$FFFF841C       ; BLTDC = $FFFF for fill
    move.w #$0000,$FFFF841E       ; BLTD (destination base)
    move.w #$0000,$FFFF8410       ; BLTCOA = $0000
    move.w #$0000,$FFFF84106      ; BLTCOB = $FFFF
    move.w #$00FF,$FFFF8404       ; BLTC = fill value
    move.w #$0000,$FFFF8406       ; BLCOD = $0000
    ; Wait for completion
    btst #$00020
    bne.b wait_done           ; Busy, continue

    ; Wait for completion
    btst #0,$FFFF8412       ; BLTCON0 bit 0 (busy flag)
    bne.b blit_done_wait
    ; Done
    rts
blit_done_wait:
    bra.b blit_done_wait

; Alternative: use $FFFF8412 to check BLTCON0
wait_blt:
    btst #0,$FFFF8412
    beq.b blit_done
    bra wait_blt
```

### Fast Line Drawing with VDI

```asm
; VDI line draw function (VDI work function 91 = 0x3E5B -> actual 0x3E0A draw_line)
; Requires: VDI mode (open_work, set_viewport, set_window)

move.w #91,d1              ; VDI work function 91 = draw_line
; Parameters:
;   D0: x1, D1: y1 (start point)
;   D2: x2, D3: y2 (end point)
le.a param_vec+0          ; params
trap #0                     ; VDI entry
    ; x1 in D0
    ; y1 in D1
    ; x2 in D2
    ; y2 in D3
    ; Returns: x1,y1,x2,y2 in memory
    ; Returns 0 = success, <0 = error

vdia_draw_line:
    dc.w 320,0,320,200       ; x1,y1,x2,y2
```

### Blitter Line Draw

```asm
; Draw diagonal line with blitter
; Sets up A = source (pattern), B = source (pattern), D = dest
draw_blitter_line:
    ; A = source pattern
    move.l #$000000,$FFFF8416     ; BLTCOA (A = $000000)
    ; B = destination stride
    move.l #320,$FFFF8418        ; BLTCOB (B = stride)
    ; D = destination base
    move.l dest_buf,$FFFF841E       ; BLTCOD (D = dest base)
    ; Set count (x160 rows, each 2 bytes)
    move.w #128,$FFFF841C          ; BLTDRC = 128
    ; Set mode: copy 1 word at a time
    move.w #$0000,$FFFF8420       ; BLTC = $4A55 (checkered)
    move.w #$000,$FFFF8422        ; BLTCON1 = horizontal count
    ; Start
    move.w #$0020,$FFFF8416       ; Set BLTCON0 (start)
    ; Check complete
    btst #0,$FFFF8416
    beq done_bline
    bra done_bline

; Done
```

## 7. Stack Frame Conventions in Atari ST Code

### GEM Application Stack Frame

```asm
; Typical GEM application entry
app_entry:
    move.l sp,m_save             ; save user stack pointer
    lea param_area(pc),a6        ; load parameters (A6 = parameter base)

; AES application parameter block at program start:
; $0(a6)     4B   return value (D0)
; $4(a6)     2B   GEM major version
; $6(a6)     2B   GEM minor version
; $8(a6)     2B   AES major version
; $A(a6)     2B   AES minor version
; $C(a6)     2B   reserved
; $E(a6)     2B   number of workstations
; $10(a6)    4B   screen dimensions
; $14(a6)    4B   max screen dimensions
; $18(a6)    2B   color resolution (bits)
; $1A(a6)    2B   reserved
; $1C(a6)    2B   current resolution (SCRn cookie)
; $1E(a6)    2B   reserved
; $20(a6)    4B   GEMDOS version
; $24(a6)    4B   AES version
; $28(a6)    4B   cookie jar pointer
; $2C(a6)    4B   program name
; $30(a6)    2B   process ID
; $32(a6)    2B   parameter count
; $34(a6)    2B   parameters (N words)
```

### Standard Function Prologue/Epilogue

```asm
; Function prologue
my_function:
    move.l sp,-(sp)                 ; space for local vars
    movem.l d1-d7/a1-a6,-(sp)       ; save used registers
    lea 0(sp),a7                    ; new frame base

; Function epilogue
    movem.l (sp)+,d1-d7/a1-a6       ; restore used registers
    move.l (sp)+,sp                 ; restore stack
    rts
```

### Local Variable Access

```asm
; With frame in A7:
    move.l 4(a7),d0                 ; return value (saved)
    move.l 20(a7),d1                ; parameter 3 (at +20)
    move.b $0(a7),local_var         ; local var at offset 0
    move.w 4(a7),local_var_16       ; local var at offset 4
    move.l 8(a7),local_var_32       ; local var at offset 8
```

## 8. Keyboard/Midi/Joystick I/O Routines

### Keyboard Input via BIOS

```asm
; Read character from console (BIOS CONIN)
kbd_read_char:
    move.w #5,d0                    ; BIOS function 5
    trap #10                        ; BIOS call
    ; D0.W = character
    rts

; Check if key pressed (BIOS CONSTAT)
kbd_check_key:
    move.w #4,d0                    ; BIOS function 4
    trap #10
    ; D0 = 0 if no key, non-zero if key available
    rts

; IKBD command for mouse (BIOS function 48)
kbd_mouse_cmd:
    move.w #48,d0                   ; BIOS function 48
    move.l #kbd_command_data,-(sp)  ; command data
    trap #10
    addq #4,sp                      ; cleanup
    rts

kbd_command_data:
    dc.l $FF8800            ; Keyboard port address
    dc.w $09                ; Absolute mouse mode
```

### MIDI Output Routine

```asm
; Send MIDI byte
midi_send:
    btst #4,$FF9A2D           ; ACIA TX empty
    beq midi_send             ; wait until empty
    move.b port_data,$FF9A2F       ; Write to ACIA data register
    rts

midi_send_byte:
    move.b d0,$FF9A2F           ; Write to MIDI ACIA
    rts
```

### Joystick Reading

```asm
; Read joystick from IKBD
read_joystick:
    lea joy_data(pc),a0
    move.w #3,d0            ; XBIOS IKBD command
    move.l #$FF8004,a1      ; IKBD port
    move.b #14,(a1)         ; Command $14 = joystick status
    ; Or read from IKBD memory-mapped joystick registers
    move.b $FF9800,(a0)     ; Joy 0 data
    move.b $FF9801,(a0)+    ; Joy 1 data
    rts

joy_data                ds.b 2
```

## 9. Common Assembly Programming Patterns

### Delay/Wait Loops

```asm
; Simple delay (not precise)
simple_delay:
    move.l #200000,d0           ; loop count
delay_loop:
    dbra d0,delay_loop           ; decrement D0, loop until zero
    rts

; Precise millisecond delay (uses GEMDOS SLEEP)
precise_delay:
    ; SLEEP is in milliseconds
    move.w #100,-(sp)           ; 100 ms
    move.w #12,-(sp)            ; GEMDOS function = 12
    trap #14                    ; GEMDOS
    addq #4,sp                  ; clean up
    rts
```

### String Operations

```asm
; String copy (source in A0, dest in A1, length in D0)
str_copy:
    move.b (a0)+,(a1)+          ; copy byte
    subq #1,d0                  ; decrement length
    bne.s str_copy              ; loop until done
    rts

; String zero-terminate
str_term:
    move.w d0,-(sp)             ; save length
str_term_loop:
    move.b #0,(a0,d0.w)         ; zero at offset
    dbra d0,str_term_loop       ; continue
    move.w (sp)+,d0             ; restore length
    rts
```

### Memory Copy Pattern

```asm
; Block copy (fast version, word-based)
mem_copy:
    move.w d0,-(sp)             ; save length
mem_copy_w:
    move.l (a0)+,(a1)+          ; copy long word
    subq #4,d0                  ; length -= 4
    bne.s mem_copy_w
    move.w (sp)+,d0             ; restore length
    ; Copy remaining bytes
    ror.w #1,d0                 ; check if odd
    beq mem_copy_end
    move.b (a0)+,(a1)+          ; copy remaining byte
mem_copy_end:
    rts
```

## 10. Interrupt Vector Modification

### Installing Custom Vectors

```asm
; Install custom keyboard vector at $0208
install_kbd_vec:
    move.l $0208,old_kbd_vec      ; save old vector
    move.l #my_kbd_handler,new_kbd_vec_write
    rts

new_kbd_vec_write:
    dc.l 0                        ; new vector address
old_kbd_vec                       dc.l 0

; Install mouse vector in IKBD chain
install_mouse_vec:
    moveq #34,d0                ; XBIOS KBDVBASE
    trap #1
    ; D0 = IKBD vector table base
    move.l d0,a0
    move.l 16(a0),old_mouse_vec  ; save old vector
    move.l #my_mouse_handler,16(a0) ; install new
    rts

old_mouse_vec                     dc.l 0
```

### VBI Chain Installation

```asm
; Install deferred VBI handler
install_deferred_vbi:
    move.l $0224,old_vkbd_vec   ; save old VVBLKD vector
    lea my_vbi_handler,a0
    andi.l #$FF000000,a0        ; preserve high byte
    or.l #$0000E7D1,a0          ; use our handler's low word
    move.l a0,$0224             ; write new vector
    rts

my_vbi_handler:
    ; Deferred VBI code (runs after system housekeeping)
    ; Must be short - VBI time is limited
    rte
```

## 11. Common Macro Definitions

```asm
; Macro definitions for Atari ST development

; VDI macros
VDI_SETVIEW    = 0x15A2    ; set_viewport
VDI_SET_WIN    = 0x15A3    ; set_window
VDI_DRAW_POINT = 0x3E01    ; draw_point
VDI_DRAW_LINE  = 0x3E0A    ; draw_line
VDI_DRAW_CIRCLE= 0x3E0D    ; draw_circle (filled_ellipse)
VDI_DRAW_RECT  = 0x4040    ; draw_box (rectangle outline)
VDI_DRAW_FILLED= 0x3F11    ; fill_area

; Blitter macros
BLT_FILL        = 0
BLT_COPY        = 1
BLT_AND         = 2
BLT_OR          = 3
BLT_XOR         = 4
BLT_PATTERN     = 0             ; Pattern mode
BLT_LINE        = 1             ; Line mode

; GEMDOS errors
E_OK            = 0
E_INVMA         = -1            ; Invalid memory address
E_NOMEM         = -2            ; Not enough memory
E_INVFN         = -3            ; Invalid function number
E_NOSUCH        = -4            ; No such file or directory
E_ACCESS        = -5            ; Access denied
E_BADFM         = -8            ; Bad file mode
E_BADFH         = -9            ; Bad file handle
E_FULL          = -14           ; Disk full
E_NEST          = -16           ; Nested call not valid
E_EMD           = -19           ; Error during media change
E_EMD20         = -20           ; Media changed (need reset)
E_EMD21         = -21           ; Media not formatted
E_EMD17         = -68           ; Media not found
```

```asm
; Helper macros for screen access
; Pixel at (x,y) in plane plane_idx:
;   offset = $C000 + plane_idx * 32000 + y * 160 + x / 16
;   bit = $8000 >> (x & 15)

; Macro for screen plane address calculation:
SCREEN_PLANE    = $C000           ; Base plane address
PLANAR_ROWSIZE  = 160             ; Bytes per row
PLANAR_STRIDE   = 32000           ; Bytes between planes
```

## 12. Debug Techniques

### Breakpoint and Dump Routines

```asm
; Hex dump routine (for debugging)
hex_dump:
    lea dump_buffer(pc),a0
    move.l #0,d0                  ; start address
hex_dump_outer:
    move.l d0,a1                  ; address to dump
    move.w #16,d2                 ; 16 bytes per line
hex_dump_inner:
    move.b (a1)+,d1               ; byte to dump
    andi.b #$0F,d1                ; nibble
    addi.b #'0,d1                  ; ASCII digit
    move.b d1,(a0)+               ; store to buffer
    dec.w d2
    beq hex_dump_newline
    bra hex_dump_inner

hex_dump_newline:
    move.b #13,(a0)+
    move.b #10,(a0)+
    addi.l #$10,d0
    cmpi.l #dump_end,d0
    blt hex_dump_outer
    rts

dump_buffer       ds.b 64         ; buffer space
```

### Watchdog Timer Pattern

```asm
; Simple watchdog pattern
; Monitor for stuck execution
watchdog_timer:
    move.l #wd_count+1,wd_count   ; increment counter
    cmpi.l #wd_max,wd_count       ; check limit
    beq watchdog_timeout            ; if exceeded, timeout
    rts

wd_count              ds.l 1
wd_max                dc.l 1000
```

## 13. Common Bug Patterns

### Stack Corruption

```asm
; BUG: Forgetting to restore all pushed registers
buggy_routine:
    move.l a0,-(sp)               ; push A0
    ; ... A0 gets trashed ...
    move.l (sp)+,a0               ; restore A0 (CORRECT!)
    rts                           ; RETURNS CORRECTLY

; BUG: Asymmetric push/pop
correct_routine:
    push.l a0                     ; BUG: only push A0, pop A0 and A6
    ; Pushed 1 long word but pop 2 long words
    ; Result: A7 is wrong, return address is corrupted
    move.l (sp),a0                ; Wrong!
    move.l 4(sp),a6               ; Still wrong!
    rte                           ; Returns to wrong address!
```

### Memory Access Errors

```asm
; BUG: Accessing hardware registers in user mode
buggy_write:
    move.w #$8000,$FFFF8412       ; Write to blitter (supervisor only!)
    ; This will cause a bus error in user mode.
    ; Use XBIOS functions instead:

safe_blitter_write:
    moveq #30,d0                  ; XBIOS BLTCMD
    move.w #$8000,d1              ; value
    trap #1                       ; XBIOS call
    rts
```

## 14. Atari ST Register Access via XBIOS

### Hardware Access Safety

```
Direct hardware register access requires supervisor mode.
From user mode, use XBIOS functions:
    TRAP #1 with function number to access hardware
    This avoids bus errors on user-mode access to $FFFFxxxx

Example: Read Shifter framebuffer base:
    moveq #37,d0            ; XBIOS V_GETBASE (V_GETBASE)
    trap #1
    ; D0 = framebuffer address (long)

Example: Set palette:
    move.l #pal_array,d0  ; palette array pointer
    moveq #6,d1           ; XBIOS SETPALETTE
    trap #1
    rts

Example: Direct register access in supervisor mode:
    move.w #$20,-(sp)     ; SUPER
    trap #14
    move.w #$10,$FFFF8416 ; Write to blitter (now safe)
    move.w #$20,-(sp)     ; SUPER
    trap #14
    rts
```
