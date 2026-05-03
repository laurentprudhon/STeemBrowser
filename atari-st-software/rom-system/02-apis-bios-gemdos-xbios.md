# BIOS, GEMDOS, XBIOS API Reference

## 1. TOS Software Architecture Layers

```
+---------------------------------------------------+
|             User Applications (.TOS/.PRG)         |
+---------------------------------------------------+
|                    GEM Desktop                     |
+--------+------------+-----------+------------------+
|    AES   |    VDI   |   GDOS   |     FSM          |
|(WIMP)   |(Graphics)|(FFonts)  | (File Switch)    |
+--------+------------+-----------+------------------+
|                      GEMDOS                            |
|            (MS-DOS compatible DOS layer)             |
+-----------------------------------------------------+
|                        BIOS                                |
|          (Machine-specific hardware access)              |
+-----------------------------------------------------+
|                      XBIOS                                 |
|         (Direct hardware access for ST features)        |
+-----------------------------------------------------+
|                    Hardware Register Space              |
|                  ($FF0000 - $FFFFF)                    |
+-----------------------------------------------------+
```

## 2. GEMDOS (Generalized Memory-resident DOS)

TOS 3.0+ officially renames GEMDOS to "DOS", but it was always called GEMDOS. It is the MS-DOS-compatible DOS layer.

### GEMDOS Entry Point
- **Trap:** TRAP #14 (vector at $FFFC028)
- Function number in D0
- **Parameters:** pushed onto stack in reverse order (WORD)
- **Return:** D0 = result, C-bit set on error

### GEMDOS Function List

| Fn # | Name | Description | D0 Return |
|------|-----------|-|-- ---------|
| 0 | `QUIT` | Terminate process (relinquish time slice) | - |
| 1 | `FCREATE` | Create file | Handle or error |
| 2 | `FOPEN` | Open file | Handle or error |
| 3 | `FCLOSE` | Close handle | Error code |
| 4 | `FSPEED` | Set baud rate | Error code |
| 5 | `FREAD` | Read from handle | Bytes read or error |
| 6 | `FWRITE` | Write to handle | Bytes written or error |
| 7 | `FLSEEK` | Set file position | $FFFFFFFF on error |
| 8 | `FREMOVE` | Delete file | Error code |
| 9 | `FGFIRST` | First directory search | Error code |
| 10 | `FGNEXT` | Next directory | Attr+size+time+name |
| 11 | `DGINFO` | Get disk info | $FFFF=error |
| 12 | `FSYNCH` | Flush file buffers | Error code |
| 13 | `PEXEC` | Execute program | $FFFFFFFF=error |
| 14 | `PCREATE` | Create sub-process | PID or error |
| 15 | `PTERM` | Terminate process | Error code |
| 16 | `PSTATUS` | Get process status | Error code |
| 17 | `FGATTR` | Get/set file attributes | Attr or error |
| 18 | `FRENAM` | Rename file | Error code |
| 19 | `PBASE` | Get/set base address | Error code |
| 20 | `PINFO` | Get process info | Error code |
| 21 | `PGETID` | Get process ID | PID |
| 22 | `PSETID` | Set process ID | Error code |
| 23 | `FGFIRST2` | Find first matching | Error code |
| 24 | `FGNEXT2` | Find next matching | Attr+size+name |
| 25 | `FGPATH` | Resolve path | Error code |
| 26 | `FGETDTA` | Get DTA pointer | DTA address |
| 27 | `FSETDTA` | Set DTA address | Error code |
| 28 | `DTIME` | Get/set date/time | Error code |
| 29 | `DGETDATE` | Get disk date | Date |
| 30 | `DGETTIME` | Get disk time | Time |
| 31 | `FSFIRST` | Find first (short) | Error code |
| 32 | `FSNEXT` | Find next (short) | Attr+size+name |
| 33 | `DGETFREE` | Get disk free space | $FFFF=error |
| 34 | `DGINFO` | Get disk info | $FFFF=error |
| 35 | `PGETPRI` | Get process priority | Priority level |
| 36 | `PSETPRI` | Set process priority | Error code |
| 37 | `GEMINFO` | Get GEM info | $FFFF=error |
| 38 | `GEMTERM` | Get GEM termination addr | Error code |
| 39 | `GEMSIZE` | Get GEM max alloc | Size |
| 40 | `LMEMTOP` | Logical memory top | Addr |
| 41 | `LCONV` | Logical/contig. conv. | Error code |
| 42 | `LMOVE` | Logical memory move | Error code |
| 45 | `LLOCK` | Lock logical memory | Error code |
| 46 | `LUNLOCK` | Unlock logical memory | Error code |
| 48 | `MALLOC` | Allocate memory block | Addr or error |
| 49 | `MFREE` | Free memory block | Error code |
| 50 | `MFIRST` | Get first block | Addr |
| 51 | `MFNEXT` | Get next block | Addr |
| 52 | `MREALLOC` | Reallocation | Addr or error |
| 53 | `MSHRST` | Memory heap reset | Addr |
| 61 | `PGETID2` | Get process ID | PID |

### GEMDOS Error Codes

| Code | Name | Description |
|------|------|-----------|
| 1 | `FNOTFUNC` | Function not supported |
| 2 | `FNOTFOUND` | File not found |
| 3 | `FPATHNOTFOUND` | Path not found |
| 4 | `FTOOMANYFILES` | Too many open files |
| 5 | `FWRITEPROTECT` | Write-protected media |
| 6 | `FUNRECOGNIZED` | Unrecognized media |
| 7 | `FSTOLEN` | Memory stolen |
| 8 | `FTOOSMALL` | Insufficient memory |
| 9 | `FWRITE_FAIL` | Disk write failure |
| 10 | `FBADOOR` | Bad file access |
| 11 | `FBADFNID` | Invalid function number |
| 12 | `FBADPARM` | Bad parameter |
| 13 | `FBADVALUE` | Bad value |
| 14 | `FNOFILESYSTEM` | No file system |
| 23 | `FBADCRCT` | Bad CRC (disk error) |
| 24 | `FTOCMOUNT` | Too many mounted |
| 26 | `FDIRNOTEMPTY` | Directory not empty |
| 29 | `FWRITE_ONLY` | Write-only file |
| 35 | `FALREADY` | Already running |

### GEMDOS Calling Convention (Example)

```asm
; FCREATE: Create new file
lea filename(pc),a1      ; pointer to filename
moveq #0,d0              ; attribute: 0=normal
trap #14                 ; GEMDOS FCREATE
bcc.f created            ; no error?
; D0 = error code on failure

; FOPEN: Open existing file
lea filename(pc),a1      ; pointer to filename
moveq #0,d0              ; access mode: 0=read
trap #14                 ; GEMDOS FOPEN

; FREAD: Read from handle
move handle,d1           ; file handle
lea buffer(pc),a0       ; destination address
move #1024,d2           ; count to read
trap #15                 ; GEMDOS FREAD
; D0 = bytes actually read

; PEXEC: Execute program
lea program(pc),a1       ; pointer to filename
move.l #oldstack,d0     ; old stack limit
lea newstack,a0         ; new stack pointer
moveq #3,d1             ; mode: 3=inherit handles
trap #13                 ; GEMDOS PEXEC
; D0 = PID of new program or $FFFFFFFF
```

## 3. AES (Application Environment Services)

AES provides the WIMP (Windows, Icons, Menus, Pointer) desktop environment. It is completely independent of the hardware (uses VDI for graphics, GEMDOS for files).

### AES Entry Point

AES is called through the AES work area block (wopbuf) at `AESBAS + $800`. The work area base pointer (AESBAS) is at `$007C` (system variable) / `$FFFF8A38` (logical address).

All AES functions are called through the AES work area:

```asm
; Call any AES function through work area:
    move.l aesbase(pc),a0      ; AESBAS at $007C/$FFFF8A38
    lea wopbuf(a0),a1          ; wopbuf at AESBAS+$800
    move.w #aes_fn,(a1)         ; set function number in wopbuf[0]
    ; Set up parameters in wopbuf[1...N] (WORDs)
    trap #0                    ; AES call (via AES wopbuf)
    ; Results returned in wopbuf
```

### AES Function Numbers (Selected)

| Fn | Name | Description |
|--|------|-------------|
| 1 | wind_create | Create a window |
| 2 | wind_open | Open a window |
| 3 | wind_close | Close a window |
| 4 | wind_delete | Delete a window |
| 5 | wind_calc | Calculate window rectangle |
| 6 | wind_clear | Clear window content |
| 7 | wind_insert | Insert window into hierarchy |
| 8 | wind_remove | Remove window from hierarchy |
| 9 | wind_update | Update mode (0=beg, 1=end) |
| 10 | wind_get | Get window information |
| 11 | wind_set | Set window information |
| 12 | wind_iconify | Iconify/deiconify window |
| 13 | wind_find | Find window by title |
| 14 | wind_draw | Draw window in grafport |
| 15 | wind_menu | Set menu highlight |
| 16 | wind_wopbuf | Get/set wopbuf pointer |
| 17 | wind_get_sysinfo | Get desktop info |
| 20 | menu_bar | Create menu bar |
| 21 | menu_init | Initialize menu |
| 22 | menu_deselect1 | Deselect a menu item |
| 23 | menu_select1 | Select a menu item |
| 24 | menu_exit | Exit menu loop |
| 25 | menu_run | Run menu handler |
| 26 | menu_bar_rmv | Remove menu bar |
| 30 | dialog | Display dialog box from rsrc |
| 31 | menu_add_item | Add menu item |
| 32 | menu_insert | Insert menu |
| 33 | menu_delete | Delete menu |
| 40 | Cdiatr | Draw dialog items |
| 41 | Cdrun | Run dialog |
| 42 | Cdinit | Init dialog |

### AES Event Codes

| Code | Meaning |
|--|-----|
| 1 | Mouse button click |
| 2 | Menu bar changed |
| 3 | Button selected |
| 4 | edit field changed |
| 5 | Icon clicked |
| 6 | Window refresh |
| 7 | Window grow/h shrink |
| 8 | Workstation opened |
| 9 | Workstation closed |
| 10 | Disk change |
| 11 | AES event (system) |
| 12 | AES suspend (time slice) |
| 13 | AES resume |
| 14 | AES alert |

## 4. VDI (Virtual Device Interface)

VDI provides hardware-independent graphics drawing.

### VDI Entry Point
- **TRAP #0** (vector at $FFFFF0E)
- Function number in INTIN (at $FFFFFFFE)
- Parameters in INDATA array (at $FFFFFFA8)
- Results in OUTDATA array (at $FFFFFFC0)

### VDI Workstation Types

| Type | Name | Resolution | Description |
|------|--|--|------|-|
| 0 | `GEMVTASCII` | - | ASCII art output |
| 1 | `GEMVTGRAPH` | 320x200 16-color | Standard ST low-res |
| 2 | `GEMVTGRAPHICS` | 640x200 4-color | ST mid-res |
| 3 | `GEMVTGRAPHICS` | 640x400 2-color | ST high-res |

### VDI Functions (Selected)

| VDI Const | Fn # | Name | Description |
|-----------|------|--|---|---|
| 0x1000 | 1 | `wind_create` | Create a window |
| 0x1001 | 2 | `wind_open` | Open workstation |
| 0x1002 | 3 | `wind_inquire` | Inquire workstation |
| 0x1010 | 4 | `wind_set_view` | Set view port |
| 0x1011 | 5 | `wind_set_win` | Set window |
| 0x1012 | 6 | `wind_set_color` | Set logical color |
| 0x1013 | 7 | `wind_create_font` | Create font |
| 0x1016 | 8 | `wind_draw_box` | Draw filled box |
| 0x1018 | 9 | `wind_fill_sp` | Fill specified position |
| 0x1019 | 10 | `wind_draw_text` | Draw text |
| 0x101A | 11 | `wind_draw_line` | Draw line |
| 0x101B | 12 | `wind_arc` | Draw arc |
| 0x101C | 13 | `wind_bar` | Draw bar |
| 0x101D | 14 | `wind_set_view` | Set view port |
| 0x101E | 15 | `wind_set_win` | Set window |
| 0x101F | 16 | `wind_get_palette` | Get color palette |
| 0x1020 | 17 | `wind_set_palette` | Set color palette |
| 0x1023 | 18 | `wind_get_font` | Get font info |
| 0x1025 | 19 | `wind_set_colormap` | Set color map |
| 0x1024 | 20 | `wind_inquire_font` | Inquire font info |
| 0x1026 | 21 | `wind_set_view` | Set view port |
| 0x1027 | 22 | `wind_set_phys_cursor` | Set physical cursor |
| 0x1028 | 23 | `wind_get_win` | Get window info |
| 0x1029 | 24 | `wind_set_win` | Set window rectangle |
| 0x102A | 25 | `wind_inquire_dev` | Inquire device |
| 0x102B | 26 | `wind_set_view` | Set view port |
| 0x102C | 27 | `wind_set_win` | Set window |

## 5. GDOS (Generalized Device-independent O/S)

GDOS provides device-independent font rendering and printing. GDOS drivers are loaded at DESKTOP.PRG startup.

### GDOS Features
- Multiple font families (built-in, user-defined, downloaded)
- Font size/weight/style selection
- Device-independent text output
- Print driver support

## 6. System Variables

### Critical TOS System Variables

| Address | Variable | Size | Description |
|---------|----------|------|---|---|
| $0000 | - | 4B | Supervior STSCK |
| $0004 | - | 4B | Initial PC |
| $0060 | VECTAB | 4B | Vector table address |
| $0064 | VEC05 | 4B | Level 5 auto-vector |
| $0068 | VEC06 | 4B | Level 6 auto-vector |
| $006C | VEC07 | 4B | Level 7 auto-vector |
| $0070 | VEC08 | 4B | Level 8 auto-vector |
| $0074 | VEC09 | 4B | Level 9 auto-vector |
| $0078 | VECTBL | 4B | Trap $000A (GEMDOS trap) |
| $007C | AES_BASE | 4B | AES work area base |
| $0080 | BINTADR | 6B | BIOS interrupt handler |
| $0086 | MEMCNTRL | 4B | Memory controller value |
| $008A | MEMVALID | 4B | Memory valid (first check) |
| $008E | MEMVAL2 | 4B | Memory valid (second check) |
| $0092 | MEMVAL3 | 4B | Memory valid (third check) |
| $0096 | SLEEP | 2B | Sleep counter |
| $0098 | T1JIFFY | 4B | T1 timer counter |
| $009C | T2JIFFY | 4B | T2 timer counter |
| $00A0 | VBL | 2B | VBL counter ($466) |
| $00A2 | VBLTIME | 2B | VBL time |
| $00A4 | MOUSE_X | 2B | Mouse X position |
| $00A6 | MOUSE_Y | 2B | Mouse Y position |
| $00A8 | MOUSE_BUTS | 1B | Mouse buttons |
| $00AA | KB_SHIFT | 1B | Keyboard shift state |
| $00AB | KB_FLAGS | 1B | Keyboard flags |
| $00AC | KB_CODE | 2B | Keyboard scan code |
| $00AE | KBDAT | 1B | Keyboard data |
| $00AF | KBFULL | 1B | Keyboard buffer full |
| $00B0 | RESST | 2B | Device status |
| $00B2 | VEC10 | 4B | Trap $0A (GEMDOS) |
| $00B6 | VEC14 | 4B | Trap $14 (AES) |
| $00BA | VEC1E | 4B | Trap $1E (VDI) |
| $00BE | VEC28 | 4B | Trap $28 (coprocessor) |
| $00C2 | VEC2C | 4B | Trap $2C (Line-F) |
| $00C6 | FDC_BASE | 4B | FDC base address ($FF8600) |
| $00CA | HDD_BASE | 4B | HDD base address ($FF8E00) |
| $00CE | Rtc_BASE | 4B | RTC base address |
| $00D2 | DSKST | 4B | Disk status ($300) |
| $00D6 | DSKSTS | 4B | Disk status ($304) |
| $00DA | DSKCHG | 4B | Disk change counter ($3E9) |
| $00DE | PHYSTOP | 4B | Physical memory top (RAM size) |
| $00E2 | LOGTOP | 4B | Logical memory top (free) |
| $00E6 | LOGEND | 4B | Logical memory bottom (HMA) |
| $00EA | LOGLOW | 4B | Logical memory bottom |
| $00EE | LMEMTOP | 4B | Logical memory top (LIM) |
| $00F2 | LMEMBOT | 4B | Logical memory bottom (LIM) |
| $00F6 | LOGEND2 | 4B | Logical memory end 2 |
| $00FA | HIGHTOP | 4B | High memory top |
| $00FE | MEMSIZE | 4B | Memory block size |
| $0102 | CURTOP | 4B | Current memory top |
| $0106 | CURBOT | 4B | Current memory bottom |
| $010A | CURFREE | 4B | Current free memory |

## 8. Cookie Jar

TOS 1.06+ provides a cookie jar at `$FFFF833C` for system information.

### Cookie Structure
```
Cookie entries: <cookie_id: 4 bytes>, <cookie_value: 4 bytes> x N, $00000000 (end marker)
```

### Standard Cookie IDs

| Cookie | Value Type | Content |
|--------|------|------|
| `TOS1` | WORD | TOS major version (e.g., 1 for TOS 1.x) |
| `TOS2` | WORD | TOS minor version (e.g., 06 for TOS 1.06) |
| `TOS3` | DWORD | TOS version (e.g., 0x0106 for TOS 1.06) |
| `STe1` | WORD | 1 if STe hardware detected |
| `MEGA` | WORD | 1 if Mega STE hardware detected |
| `TT00` | WORD | 1 if TT hardware detected |
| `FCON` | WORD | 1 if Falcon hardware detected |
| `SRES` | WORD | Super Hi-Res mode support |
| `HRES` | WORD | High resolution support |
| `MCHR` | WORD | MCHR (memory controller version) |
| `RSC0` | DWORD | Resource file pointer |
| `SCR0` | DWORD | Screen buffer pointer |

### Cookie Jar Walk (C-style pseudocode)
```c
ULONG *cookieptr = (ULONG *)0xFFFF833C;
while (*cookieptr != 0) {
    cookie_id = cookieptr[i];       // e.g., 'TOS1' = 0x544F5331
    cookie_val = cookieptr[i+1];
    if (cookie_id == 'TOS1') {
        t1 = cookie_val;   }
    if (cookie_id == 'STe1') {
        ste = cookie_val;
    }
    i += 2;
}
```
