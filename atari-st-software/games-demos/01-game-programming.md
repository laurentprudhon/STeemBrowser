# Third-Party Software Development Guide

## 1. Development Toolchains

### Assemblers

| Assembler | Platform | Notable Features |
|-----------|-|------|
| **VASM** | PC/Linux | Modern, TOS format output, fast |
| **RMAC** | Cross-platform | Classic, TOS header generation |
| **AFactory** | Atari ST | Native assembler, fast turnaround |
| **ASM One** | Atari ST | Native, integrated editor |
| **AsmPro** | Atari ST | Native, breakpoint debugger |
**TASM** | PC (DOS) | Turbo Assembler, Atari ST port |
**DevPAC** | Atari ST | Native, integrated IDE |
**HiSoft Assembler** | Atari ST/PC | Professional, well-documented |

### C Compilers

| Compiler | Platform | Notes |
|----------|-|---|
| **Latté C++** | Atari ST | Professional, well-optimized |
**Pure C** | Atari ST/PC | Free, widely used |
**Aztec C** | Atari ST/PC | Early compiler, good code |
**Alcyon C** | Atari ST/PC | Atari-supplied, good standard |
**Turbo C** | Atari ST | Borland port, fast compile |
**High-C** | Atari ST | Atari-supplied, GEM support |

### IDE and Debuggers

| Tool | Platform | Description |
|------|-|--------|
| **Hatari** | PC/Linux | Emulator with debugger, breakpoints |
**Steem/SainT** | PC/Linux | Full-feature emulators |
**BugBug** | Atari ST/PC | Standalone debugger |
**Debug** | Atari ST | Atari-supplied, GEMSYS |
**ASM One Debugger** | Atari ST | Integrated breakpoints |
**AFactory IDE** | Atari ST | Assembler + IDE |
|

## 2. Supervisor Mode Entry

For direct hardware access (video, sound, disk), you must enter supervisor mode:

```asm
; Method 1 (quickest, no stack management)
move.w #$20,-(sp)    ; GEMDOS function 32 (SUPVIS)
trap #14             ; GEMDOS
; D0 now contains old task register value
; A1 now contains old supervisor stack
; A0 now contains supervisor stack base

; Method 2 (register D7)
moveq #$20,d7
move.w d7,-(sp)
trap #14

; Method 3 (via XBIOS)
; XBIOS call 22 (RESMCR) to get supervisor
; via task register manipulation
```

### Supervisor Mode Memory Access

```
Address          Mode            Access
────────────────────────────────────
$00000-$003FF    Supervisor only     Bottom 1KB (system variables)
$FF000-$FFFFF    Supervisor only     Hardware registers
$00400-$FEFFF    Both modes          Application memory
```

## 3. Video Hardware Access

### Shifter Video Registers

| Address    | Register | Description |
|--------|----------|-----------|
| $FFFF8200 | VID_COLOR0| Palette color 0 (background) |
| $FFFF8202 | VID_COLOR1| Palette color 1 |
| $FFFF8204 | VID_COLOR2 | Palette color 2 |
| $FFFF8206 | VID_COLOR3 | Palette color 3 |
| $FFFF8208 | VID_COLOR4 | Palette color 4 |
| $FFFF820A | VID_COLOR5 | Palette color 5 |
| $FFFF820C | VID_COLOR6 | Palette color 6 |
| $FFFF820E | VID_BASEADDH| Screen high byte |
| $FFFF8210 | VID_BASEADM | Screen mid byte |
| $FFFF8212 | VID_BASEADLL| Screen low byte |
| $FFFF8214 | VID_PTRH  | Address pointer high |
| $FFFF8216 | VID_PTRM  | Address pointer mid |
| $FFFF8218 | VID_PTRLL | Address pointer low |
| $FFFF821A | VID_SYNCCTRL | Sync control |
| $FFFF821E | VID_SYNCCTRL | Sync control (STE) |

### Planar Framebuffer Layout (Low-Res 320x200, 16 colors)

```
Pixel (x, y) with 4-bit color index [3:0]:
Color bit 3 -> Plane 4 at address $C000 + y * 20 + (x / 16)
                 bit position = (x % 16)
Color bit 2 -> Plane 3 at address $C000 + 20 * 200 + offset
Color bit 1 -> Plane 2 at address $C000 + 20 * 400 + offset
Color bit 0 -> Plane 1 at address $C000 + 20 * 600 + offset
```

### Plotting a Pixel in Assembly

```asm
; Plot pixel at (x, y) with color 'color' in low-res
; Input: D0 = x, D1 = y, D2 = color (0-15)
; Output: pixel drawn

plot_pixel:
    ; Calculate address in each plane
    move.w d0,d3          ; D3 = x
    move.w d1,d4          ; D4 = y
    move.w #320,d5        ; D5 = 320 (row width)
    mulu.w d4,d5          ; D5 = y * 320
    add.w d3,d5           ; D5 = y * 320 + x
    move.w #4,d6          ; D6 = plane bit count

plane_loop:
    ; Calculate plane offset
    lsl.w #2,d6           ; plane number * 20000
    add.w d5,d7           ; base address + offset
    ; Calculate bit position
    divu.w #16,d3         ; divide by 16 to get word position
    lsr.w #1,d3           ; bit position (0-15)
    ; Shift and set bit
    lsl.w d3,d7
    lsr.w #15,d7          ; move MSB to bit 0
    btst #0,d7            ; check if bit set
    bne.s plane_set
    eor.w #4096,d7           ; set the bit
plane_set:
    sub.w #20000,d6       ; subtract plane offset
    move.w d6,-(sp)           ; save offset
    move.w d6,d6                ; plane number = (D6/20000 & 3)
    lsr.w #2,d6           ; divide by 20000
    moveq #3,d4           ; D4 = bit position of plane
    moveq #4,d5           ; D5 = total planes
    move.l (a0,d6*4),d0      ; get plane address
    move.w d0,d7          ; plane address
    add.l d7,d8           ; base + offset
    move.w d5,(d8)        ; write bit to plane
    bgt.s plane_loop
    rts
```

### VDI Drawing vs Direct

| Feature | VDI (via TRAP #0) | Direct Memory |
|---|---|------|
Speed | ~100-500 µs per pixel | ~5-20 µs per pixel |
Easiness | High (VDI function calls) | Medium (bitplane math) |
Abstraction | Device-independent | Direct framebuffer |
Palette | VDI palette functions | Direct register write |
Font rendering | Built-in | Manual bitmap |
Line drawing | Bresenham in VDI | Must implement |

## 4. Sound Hardware (YM2149 PSG)

### Register Map

| Register | Address | Purpose |
|-|-|---|
| 6 | $4 | Tonal period (A) |
| 7 | $5 | Tonal period (A) |
| 8 | $6 | Tonal period (B) |
| 9 | $7 | Tonal period (C) |
| 10 | $8 | Tonal period (C) |
| 11 | $9 | Noise control |
| 12 | $A | Mixer control |
| 13 | $B | Volume channel A |
| 14 | $C | Volume channel B |
| 15 | $D | Volume channel C |
| 16 | $E | Amplitude modulation |
| 17 | $F | Tonal period (A) |
| 18 | $10 | Tonal period (B) |
| 19 | $11 | Noise control |
| 20 | $12 | Mixer |
| 21 | $13 | Volume A |
| 22 | $14 | Volume B |
| 23 | $15 | Volume C |

### YM2149 Frequency Formula

```
frequency = 16_267_123 / (27 * period)
period = 0 - 1023 (10-bit range)

Example:
  frequency of C4 (261.6 Hz):
    period = 16_267_123 / (27 * 261.6)
    period = 16_267_123 / 7_067
    period = 2299

; Channel A frequency to register:
move.w #2299,d0
move.w #6,d1
move.b d1,$FFFF8800        ; register A
move.b d0,$FFFF8801        ; LSB
move.b d0,$FFFF8802        ; MSB
```

### Sound Effects Programming

```asm
; Sound effect: ascending pitch sweep
; Channel A, volume 0xF (max)

sound_fx_sweep:
    lea $FFFF8800,a0           ; YM2149 register port
    moveq #6,d0              ; register 6 = channel A
    move.b d0,(a0)
    moveq #7,d0              ; register 7 = channel B
    move.b d0,(a0)
    move.w #220,d1           ; starting period
sweep_loop:
    move.w d1,d0
    and.w #$3FF,d0            ; MSB
    lsr.w #8,d0
    and.w #$3F,d0             ; MSB
    move.b d0,$FFFF8801          ; register 7 (LSB)
    move.b d0,$FFFF8802          ; register 6 (MSB)
    clr.w -(sp)            ; sleep for delay
    trap #14               ; GEMDOS SLEEP
    add.w #200,d1              ; decrease pitch
    bgt.s sweep_loop            ; stop at period 0
    rts
```

### Music Trackers

| Tracker | Format | Notes |
|---------|---|---|-|
| **Scream Tracker 3** | `.S3M` | .ST module format, 3-channel |
| **Scream Tracker 2** | `.STM` | Classic module format |
| **OctaMED** | `.MED` | 8-channel player, advanced |
| **SFX** | `.SFX` | YM2149 sound samples |
| **ST3 Tracker** | `.SFX` | Native tracker for STE |
| **Arkos Tracker** | `.AKS` | Atari 8-bit, 16, 32 |

### Tracker Module Format

```
; Scream Tracker 3 header
Offset    Size      Content
────────    ────      ───────
+0x0000   4B      Magic "SCRM"
+0x0004   4B      Format 0x0020
+0x0008   20B     Song name (null-padded)
+0x0018   2B      Song length (patterns)
+0x001A   2B      Keyframe count
+0x001C   2B      Pattern loop start
+0x001E   2B      Pattern loop end
+0x0020   2B      Channel flags (3 channels)
; Channel flags:
;   Bit 0: Channel active
;   Bit 1: Channel is stereo
;   Bit 2: Channel is mono
;   Bit 3: Channel is sample
;   Bit 4: Channel is pattern
;   Bit 5: Channel is effect
;   Bit 6: Channel is note
;   Bit 7: Channel is speed

+0x0022   2B      Default speed
+0x0024   2B      Song position
+0x0026   patterns  Pattern index table
+0x??00   patterns  Pattern data
+0x??00   samples   Sample data
```

## 5. Keyboard and Input

### Keyboard Scan Codes

```
; Keyboard is managed by the IKBD (6801 microcontroller)
; XBIOS function 94 (KBDSTAT) returns:
;   D0 = key status & shift state
;   D1 = ASCII character
;   D2 = scan code
;   D3 = character count

; IKBD status byte D0 bit flags:
;   Bit 15: Any key?
;   Bit 14: Shift key down
;   Bit 13: Right Shift key down
;   Bit 11: Ctrl key down
;   Bit 10: Right Ctrl key down
;   Bit 9: Alt key down
;   Bit 8: Right Alt key down
;   Bit 7: Caps Lock on
;   Bit 6: Num Lock on
;   Bit 5: Scroll Lock on
;   Bit 4: Command key (STE+)
;   Bit 3: Gadget key (STE+)
;   Bit 2: Right Shift (STE+)
;   Bit 1: Right Ctrl (STE+)
;   Bit 0: Right Alt (STE+)
;   Bit 12: Right shift (STE+)
;   Bit 12: Right shift down
```

### Mouse Input

```
; Mouse position is in X/Y coordinates
; Mouse button state at $FFFFF28 (MOUSE_BUTTS)
; XBIOS function 68 (MOUSECTL) to read:

moveq #68,d0            ; MOUSECTL
trap #1                ; XBIOS
; D0 = MOUSE_X, D1 = MOUSE_Y
; D2 = BUTTONS (bit 0=left, bit 1=right)
```

### Mouse Button Reading

```asm
; Get mouse position and button states
moveq #68,d0             ; XBIOS MOUSE position
trap #1
; D0 = X coordinate (0..319)
; D1 = Y coordinate (0..199)
; D2 = Button states (bit 0=left, bit 1=right)
; If D3 == $FFFF, mouse position is not initialized
```

### Joystick Input

```
; Joystick port read:
; XBIOS function 78 (JOYPOS) returns:
;   D0 = direction bit flags
;   D1 = fire button state

; Direction bits:
;   Bit 12: Fire  (bit 12 = fire button)
;   Bit 8:  Left
;   Bit 4:  Right
;   Bit 2:  Up   (bit 2 = up)
;   Bit 1:  Down
;   Bit 0:  Center

; Fire bit layout:
;   Bit 7:  Left fire  (bit 7 = left fire button)
;   Bit 6:  Right fire
;   Bit 5:  Center fire
;   Bit 4:  Button 4
;   Bit 3:  Fire
;   Bit 2:  Button 2
;   Bit 1:  Button 1
;   Bit 0:  Button 0
; (Note: layout depends on joystick type)
```

## 6. MIDI Interface

### MIDI Port Configuration

```
; Atari ST MIDI port at $FF9A (MIDI)
; Serial configuration:
;   Baud rate: 31250 bps
;   Parity    : No
;   Stop bits : 1
;   Data bits : 8

; Sending MIDI message:
lea midi_data(pc),a0       ; MIDI message buffer
moveq #12,d1              ; message length
moveq #6,d0              ; MIDI port number
trap #1                ; XBIOS RS232 write
; MIDI data at $FF9A port
```

### MIDI Message Format

| Byte | Name | Description |
|--|--|-|
| 1 | Status | 0x80 = note off, 0x90 = note on |
| 2 | Data 1 | Note number (0-127) |
| 3 | Data 2 | Velocity (0-127) |

```asm
; Send MIDI note on, channel 1, note 60 (middle C), velocity 127
midi_note_on:
    dc.b $90              ; Note on, channel 1
    dc.b 60            ; Note 60 (middle C)
    dc.b 127           ; Max velocity
    moveq #12,d1
    moveq #12,d0
    trap #1
    rts
```

## 7. Memory Management

### Free Memory Allocation

```asm
; Check free memory at $10A (CURFREE):
lea $10A(pc),a0
move.l (a0),d0           ; D0 = free bytes available
; Minimum needed for game: ~1 KB (stack) + program size

; Request memory (GEMDOS 34):
; Fallocate request:
move.l #size_needed,-(sp)
move.w #34,-(sp)         ; GEMDOS Fallocate
trap #14
; D0 = block address or $FFFFFFFF on error
; If size > free memory, allocation failed

; Free memory (GEMDOS 35):
move.l address,-(sp)
move.w #35,-(sp)         ; GEMDOS free
trap #14
```

### Memory Map for Application

```
Address          Content
───────          ───────
$00000 - $003FF   System variables
$00400 - $00BFF   GEM/BIOS variables
$00C00 - $00CFF   AES work area
$00D00 - ?       Free application space
$40000 - $C0000  Frame buffer (lowres)
$C0000 - $FFFFF  Hardware registers (supervisor)
```

## 8. Game Loop Architecture

```asm
; Typical game main loop (in direct video mode):
    ; Enter supervisor mode
    move.w #$20,-(sp)
    trap #14

    ; Set video mode to low-res (320x200, 16 colors)
    move.w #0,-(sp)          ; video mode
    move.w #12,-(sp)         ; BIOS WRMODE
    trap #10
    clr.w -(sp)            ; GEMDOS FALLOCATE = 1
    trap #14

main_loop:
    ; Wait for vsync (vertical sync)
    move.w VBL,d0          ; current VBL count
vbl_wait:
    cmp.w VBL,d0           ; still same?
    beq.s vbl_wait        ; yes, wait

    ; Process input
    moveq #78,d0           ; XBIOS JOYPOS
    trap #1
    moveq #68,d0           ; XBIOS MOUSE position
    trap #1

    ; Process game logic (update entities, physics)
    bsr update_game_state

    ; Render frame (blit sprites, draw background)
    bsr render_frame

    ; Sound update (step tracker or play notes)
    bsr update_sound

    ; Check for game over / exit condition
    bsr check_exit
    bne.s main_loop

exit:
    ; Exit supervisor mode
    move.w #$20,-(sp)
    trap #14
    move.w #0,-(sp)        ; GEMDOS QUITT
    trap #14
```

### VBL Handling

```asm
; VBL counter at $466 updated every vertical sync
; Read $466 for current frame count
move.w $466,d0          ; D0 = total VBL counter
move.l $466,d1            ; D1 = VBL counter + time
```

### Sound Timing Loop

```asm
; Sound loop (runs during every game frame):
update_sound:
    ; Update sound tracker (play next note)
    move.l tracker_pos,a6
    move.w (a6)+,d0
    move.w (a6)+,d1
    move.w (a6)+,d2
    ; Send to YM2149
    move.w #6,d3          ; Channel A
    move.b d3,$FFFF8800    ; YM2149 register
    move.b d0,$FFFF8801  ; LSByte
    move.b d0,$FFFF8802  ; MSByte
    ; ... same for channels B and C
    rts
```
