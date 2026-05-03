# Atari ST Demoscene and Graphic Effects

## 1. The Atari ST Demoscene

The Atari ST demoscene was the largest demoscene platform of the 1980s-early 1990s, producing thousands of intros, demos, and megademos from ~1985 to ~1995.

### Notable Demo Groups

| Group | Active Years | Notable Productions |
|-------|-|---|-|
| **Level16** | 1987-1995 | The Ultimate Demo (1990), The Last Level (1992) |
| **Synkronized** | 1989-1995 | Mega 2K Demo (1991), ST Demos |
| **The Black Lotus** | 1991-1995 | The Black Lotus Demo (1992), ST-2000 (1993) |
| **Abyss** | 1989-1994 | Mega Demo (1990), ST-1600 (1991) |
| **The Last Byte** | 1989-1994 | The Last Byte Demo (1991), ST Demos |
| **DNT Crew** | 1988-1994 | ST megademo "The Ultimate" (1990), guest screens |
| **MJJ Prod** | 1990-1994 | Anomaly megademo (1992), 19 screens + karaoke |
| **Delta Force** | 1992-1995 | The Ultimate Mega (1993), various guest screens |
| **Scape** | 1990-1994 | The Scape Demo (1992), ST intros |

### Demo Competitions

| Competition | Year | Platform |
-------------|------|---------|
| **ST-2K Demo** | 1990+ | Atari ST, 2 KB limit |
| **ST-1600** | 1991 | Atari ST, 1.6 KB limit |
| **4.8K** | 1991+ | Atari STe, 4.8 KB limit |
| **4.24K** | 1991+ | Atari STe, 4.24 KB limit |
| **4.24S** | 1991+ | Atari STe, 4.24 KB limit |

## 2. Raster Bars (Raster Interrupt Bars)

Raster bars are horizontal colored lines that move or change color mid-frame. The ST Shifter chip has no hardware rasterbar interrupt, so rasters are done by:

### Technique: Race The Beam

```asm
; Race the beam: poll the video beam position by changing color 0
movea.l VIDCOL0,a0         ; Palette address $FFFF8200
movea.l $FFFF8200,a4       ; Palette address
main_loop:
    ; Set color 0 to known color $FFFF (white)
move.w #$FFFF,a3     ; white
    move.w (a3),(a4)        ; write to color 0
    ; Now wait for the beam to pass over color 0
    ; The color stays $FFFF as long as beam is not writing
    ; Once beam reaches color 0, it writes $0000 (background)
vbl_wait:
    cmp.w #0,(a4)        ; color 0 still $FFFF?
    bne.s vbl_wait       ; no, beam is at color 0
    ; Now the beam has reached color 0 position
    ; We have the beam position, we can use the delay until next frame
    ; to change palette entries, do work, then return to loop
```

### Technique: Palette Color Tracking

```asm
; More precise race-the-beam with palette tracking:
; Change palette, wait for color 0 to change, do work, change palette
movea.l #0xFFFFF80004,a3   ; Palette
movea.l #0xFFFFF80004,a4   ; Color 0
movea.l #$0000000010,d7   ; Frame counter

    ; Set color 0 to $0201 (non-zero, non-background)
    dc.w 0xFFFF8200        ; color register address
    move w #$4001,-(3)     ; color 1 = $4001 (some color)

    ; Now race: keep polling color 0
    cmp.w #$4001,(a4)      ; still $4001?
    bne.s .color_changed   ; no! beam reached color 0
    bra .poll_color        ; still white

; When beam reaches color 0, the Shifter writes $0000 to color 0
; This happens exactly 693 cycles after the start of the frame
; We can count cycles from here to position our raster bars

; Typical 50 Hz PAL frame timing:
; Frame starts: cycle 0
; Color 0 is $0000 from cycle 56 (693 cycles later)
; Color 0 is $0000 for 320x200 scanlines
; Frame ends: cycle 512 (50 Hz) / 508 (60 Hz)
```

### Raster Bar Example

```asm
; Simple raster bar: change color 1 at line Y position
; Using the Shifter's internal timing properties

    ; Change palette during raster
    move.l #0xFFFFF20004,a0  ; Palette (color 0)
    move.w #$0001,-(3)     ; Color 1

; Poll color 0 at position X in line
    ; When beam is at color 0, it clears it
    ; We can detect this by reading the color
    bset #$0001,a1                ; Set bit 0 (LSB)
bset loop:
    cmp.w #(a1,a3,a4,a4)     ; still original color?
    bne.s bset_loop          ; yes, try again
; No! The beam just wrote to the palette
; We changed color 0 at X pixels into the scanline
```

### Raster Bar Timing Table

```
; 320x200 PAL 50 Hz timing (cycles):
; Line 0: starts at 0 cycles, ends at 376 cycles (320 pixel + 40 cycles)
; Line 200: starts at 85200 cycles (200 * 426 cycles/line), line 0
; Frame length: 426 cycles per line * 200 lines = 85,200 cycles

; Color 0 tracking (race-the-beam):
; Color register update: cycle 0 of scanline
; Beam at color 0: cycle 56 after VBI
; Pixel 0: cycle 0 of color 0 change
; Pixel 1: cycle 56 + 1 = cycle 57 after VBI
; Pixel 319: cycle 0 + 320/1 = cycle 82 (320 pixels * 2 cycles) = cycle 82
; Frame end: 426 cycles = cycle 426 = end of VBL
; Frame start: 0 cycles after VBL start
; Total: 426 cycles per line, 200 lines = 85200 cycles per frame
```

## 3. Effect: Syncscroll (Sync to Sound)

Syncscroll synchronizes scrolling effects with the music tracker.

### Tracker Sync Method

```asm
; Tracker: Scream Tracker 3 (.ST3 format)
; Song has 3 channels, 90 BPM, speed 2
; Each pattern row = 4 ticks (4 * 50 Hz frames)

; Syncscroll: scroll text at 1 pixel every 4 VBLs
; (matches the tracker's 1 row per line)

    ; Check if VBL counter matches row counter
    move.w vbl_counter,d0      ; Current VBL frame
    move.w pattern_row,d1      ; Current tracker row
    ; Multiply row by 4 (4 VBLs per row)
    lsl.w #1,d1
    lsl.w #1,d1
    cmp.w d1,d0               ; Same frame?
    bne.s not_synced          ; No, don't scroll
; Yes, scroll now:
    bsr scroll_text_by_1_pixel
    ; Increment row counter
    addq #1,pattern_row
```

## 4. Effect: Overscan Fullscreen

Fullscreen mode removes the border around the 320x200 visible area.

### Hardware Hack: Fooling the Shifter

```asm
; Fullscreen technique: "racing the beam" with palette changes
; At specific points in the scanline, the Shifter chip switches
; between displaying data and displaying the border.

; By changing the refresh rate mid-scan, we can disable the border
; detection and extend the drawing area:

; Setup:
;   A4 = $FFFFF2014 (refresh rate register)
;   A5 = $FFFFF8104 (mode register)
;   D6 = $FFFFF824   (palette register offset)

; Start of scanline:
    move.w (a4),(A4)        ; Set resolution to high res
    ; This fools the Shifter to start drawing early (left border)

    ; Wait until cycle 376 (where right border normally starts)
; The Shifter checks the refresh rate to decide when to stop
; At cycle 376, change rate to 60 Hz:
    move.b d6,(a5)        ; Cycle 376: set 60Hz
    ; The Shifter checks: "Is 50Hz or 60Hz?"
;   - If 50Hz: stop drawing at 376 cycles
;   - If 60Hz: stop drawing at 372 cycles
; We changed to 60Hz, so it tries to stop at 372
; But at 372 cycles, we immediately change back to 50Hz:
    move.w (a5),(A5)        ; Cycle 384: back to 50Hz
; Now the Shifter is confused! It doesn't know whether to draw
; or enter the right border, so it continues drawing.

; At the end of the frame, change back to normal mode.
; At the start of the frame, repeat the cycle.
```

### Fullscreen Coordinates

In fullscreen mode:
```
Standard ST (320x200):
  Visible area: 320 pixels wide, 200 lines tall
  Border: 20 lines on top, 10 lines on bottom
  Total lines: 230 (PAL 50 Hz) / 220 (NTSC 60 Hz)

Fullscreen ST:
  Visible area: 416 pixels wide, 274 lines tall
  Border: None (border is drawn on)

; Frame buffer layout (Fullscreen):
; Plane 1: $C000 + offset
; Plane 2: $C8000 + offset
; Plane 3: $C9000 + offset
; Plane 4: $CA000 + offset
; Each plane has 416/16 * 274 words
; Total: 416 * 274 / 4 = 28,484 bytes per plane
```

## 5. Sound Effect: YM2149 Tricks

### YM2149 Amplitude Modulation

```asm
; Amplitude modulation: rapid volume changes
moveq #6,d1              ; Register 6 (volume A)
loop:
    move.w (a2)+,d0      ; Get volume value from LUT
    move.w #$0F00,a2       ; YM2149 register port
    move.b d0,0(a2)        ; Set register 6
    move.b d1,0(a1)        ; Set register 7 (volume)
    move.l #$FFFF,-(3)     ; Sleep for delay
    bra.s loop
```

### Noise Generation (Pseudo-Random)

```asm
; YM2149 has a noise generator with 3 modes:
;   Mode 0: Period 0   / 16   (fixed)
;   Mode 1: Period 2   / 16  (period / 2)
;   Mode 2: Period 3   / 8   (period / 3)
;   Mode 3: Period 7   / 1   (free)
; Noise is mixed with tonal or per channel

; Set noise period to mode 3 (free, any value 0-1023)
; YM2149 register B (mix) controls which channels get noise:
;   Bit 0: Channel A noise
;   Bit 1: Channel B noise
;   Bit 2: Channel C noise

moveq #$3B,A1            ; Register 11 (Noise period)
move.w #$312,B1          ; Mode 3 (free)
; Set mixer to all noise
moveq #$14,A1            ; Register 12 (Mixer)
; YM2149 register 12 (mixer):
;   Bit 0: Channel A noise
;   Bit 1: Channel B
;   Bit 2: Channel C
;   Bit 3: Channel A tone
;   Bit 4: Channel B tone
;   Bit 5: Channel C tone
move.w #$FF,A1           ; All noise (mixer = 0xFF)
```

## 6. Sprite Bob (Blitter Object)

Sprite Bobs use the Blitter chip (STE only) to draw masked bitmaps.

### Blitter Sprite Bob (STE)

```asm
; Bob drawing setup:
movea.l #$FFFF8A00,a0     ; Blitter control register
lea bob_data(pc),a1        ; Bob bitmap
lea bob_mask(pc),a2        ; Bob mask

; Bob bitmap size: 32x32 pixels (4 colors)
; Bob address: (x, y)
; Bob color: 4-bit (0-15)
; Bob mask: 1-bit per pixel (1=transparent, 0=opaque)

; Draw Bob at (x, y):
move.w x_pos,d0
move.w y_pos,d1

; Calculate plane offset
lsl.l #3,d0               ; x * 8
lsl.l #4,d1                ; y * 16
lsl.l #8,d1               ; y * 1024
add.l d0,d1               ; x + y * 1024
lsl.l #4,d1               ; * 16 = byte offset

move.l d1,-(a0)           ; Source X
move.l d1,-(a0)           ; Dest X
move.w #32,-(a0)          ; Width
move.w #32,-(a0)          ; Height
move.w #$0F,-(a0)         ; ROP (OR)
move.w #$0F,-(a0)        ; Enable plane 1,2,3,4
move.w #$0,-(a0)          ; Start blit
; The blitter now draws the 32x32 Bob at (x,y)
```

### Blitter Bob Mask

Bob mask is a 1-bit depth bitmap:
```
Bit 0 = transparent pixel
Bit 1 = drawn pixel
Bit 2 = transparent pixel
Bit 3 = ...

; 32x32 Bob: 32 * 32 / 8 words = 128 words
; For 4 colors: 32x16 pixels, each pixel is 1 bit
; 1 plane of 32x16 = 16 words
```

## 7. Copy Protection Bypass

Demos often used copy protection to prevent source code distribution:

### Protection Techniques

```
1. Hidden sectors:
   - Read a sector that appears empty
   - The sector actually contains a checksum or data
   - If checksum fails, demo crashes

2. Timing-based protection:
   - Measure disk read time
   - Genuine disks read faster than copies
   - If read time > threshold, crash

3. Sector ID manipulation:
   - Change sector ID numbers
   - Copies use default sequential sectors
   - Demo checks for non-sequential IDs

4. Track 0:
   - Read entire track 0 (not just sector 1)
   - Look for hidden data in the track
   - Genuine disk has data, copy doesn't

5. Gap manipulation:
   - Change inter-sector gap size
   - Copies use standard gap
   - Demo reads raw track to check

6. Sector size variation:
   - Use non-standard sector sizes (128/256/512)
   - Copies may not support custom sizes
   - Demo checks for correct size

6. Sector 1 checksum:
   - Sector 1 has a checksum at specific offset
   - Demo reads and verifies
```

### Copy Protection Bypass (Emulator)

```
; To bypass protection in Hatari/Steem:
; 1. Load the protected demo disk image
; 2. Start the emulator with the disk
; 3. The demo checks the disk
; 4. If it fails, exit the demo
; 5. Use the emulator's disk swap feature to swap
;   with an unprotected version (if available)
; 6. Or patch the code to skip the check:
;   Find the checksum/crash code at the crash routine
;   Change the CRASH to RTS (return)
```

## 8. Tracker Music

### Tracker Song Structure

```
Song header:
  Pattern table: 100 patterns
  Sample table: 15 samples
  Speed: 6 (ticks per row)
  BPM: 125

Pattern (100 rows x 3 channels):
  Row 0: Note C-4, Vol 15, Effect 0, Sample 1
  Row 1: ...
  ...
  Row 99: Note rest, Vol 0, Effect 2, Sample 0

Sample format:
  Sample 0: 200 bytes
  Sample 1: 300 bytes
  ...
  Sample 14: 100 bytes
  Total samples: < 64 KB

Sample data:
  Each sample is 8-bit PCM
  Signed (two's complement)
  Sample rate: 100 kHz (Y2149 max)
  Actual playback: 69 kHz (after 1/2 decimation)
```

### Playback Loop

```asm
; Tracker playback:
play_loop:
    ; Wait for 50ms
    move.l #$FFFF,-(sp)
    trap #14
    ; Scream Tracker 3 song
; Current song position in pattern table
move.l tracker_pos,a0
move.l (a0)+,d0           ; Current row
move.l (a0)+,d1           ; Current channel
; Get note, volume, effect for this row/channel
move.w (a0),d0           ; Note
move.w (a0)+,d1           ; Volume
move.w (a0)+,d2           ; Effect
move.l (a0)+,d3           ; Sample index
; Send to YM2149
bts play_note
; Advance to next row
addq #1,song_row
; Check if row > 99
cmp.w #99,song_row
ble.s play_note
; Pattern finished, advance pattern
addq #1,song_pattern
move.l (a0)+,d0           ; Check pattern table
move.l (a0)+,d1           ; Pattern number
beq.s song_end            ; End of song?
; Load next pattern
move.l #$FFFF,tracker_pos
move.l d0,tracker_pattern
; Jump to pattern start
bra.s play_loop

song_end:
    rts

; Play a single note:
play_note:
    ; Send note to channel
    move.w d0,$FFFFF800    ; Register 6 (channel A freq)
    move.w d1,$FFFFF881    ; Register 7 (volume)
    ; ... same for B, C
    ; Advance to next note
    add.l #4,tracker_data
    ; ...
    rts
```
