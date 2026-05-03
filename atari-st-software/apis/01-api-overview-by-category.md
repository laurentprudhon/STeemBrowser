# Atari ST Software API Reference by Function

This document provides a feature-based categorization of all Atari ST APIs, grouped by functionality. Each section lists the relevant APIs with their entry points and purpose.

---

## 1. Memory Management APIs

### MALLOC — Allocate Memory
- **API:** GEMDOS TRAP #14, function 72
- **Params:** D0 = number of bytes (or -1 to query largest block)
- **Return:** D0 = allocation address, or NULL on failure
- **Use:** Obtain dynamic memory from the free pool

### MFREE — Free Memory
- **API:** GEMDOS TRAP #14, function 73
- **Params:** D0 = address returned by MALLOC
- **Return:** D0 = E_OK (0) or EIMBA (-23)

### MXALLOC — Advanced Allocation
- **API:** GEMDOS function 68
- **Params:** D0 = size, D1.W = mode (RAM type + protection bits)
- **Return:** D0 = address on success, NULL on failure
- **Mode bits:** 0-2 = RAM type, 4-7 = protection mode

### MSHRINK — Resize Block
- **API:** GEMDOS function 74
- **Params:** D0 = address, D1 = new size
- **Return:** D0 = E_OK or error code
- **Special:** D1 = 0 releases block, D1 = -1 queries largest size

### MACCESS / MVALIDATE
- **API:** GEMDOS functions 381 / 321
- **Purpose:** Verify memory accessibility; validate cross-process memory

### LLOC / LMOVE / LUNLOCK
- **API:** GEMDOS functions 45, 42, 46
- **Purpose:** Manage logical-to-contiguous memory mapping

---

## 2. File System / Disk APIs

### FCREATE / FOPEN / FCLOSE
- **API:** GEMDOS functions 1, 2, 3
- **Purpose:** Create, open, and close files
- **Return:** Handle on success, error code on failure

### FREAD / FWRITE
- **API:** GEMDOS functions 5, 6
- **Purpose:** Read/write to file handle
- **Return:** Byte count or error code

### FLSEEK
- **API:** GEMDOS function 7
- **Params:** D1.W = offset, D2.W = origin (0=start, 1=current, 2=EOF)
- **Return:** Current position, or $FFFFFFFF on error

### FREMOVE
- **API:** GEMDOS function 8
- **Purpose:** Delete a file

### FGETDTA / FSETDTA
- **API:** GEMDOS functions 26, 27
- **Purpose:** Get/set Disk Transfer Area address (used by find-first/find-next)

### FGFIRST / FGNEXT / FGSFIRST / FSNEXT
- **API:** GEMDOS functions 9, 10, 31, 32
- **Purpose:** Directory search (full and short name variants)

### DGINFO / DGETFREE
- **API:** GEMDOS functions 33, 34
- **Purpose:** Get disk information and free space

### FSPEED / FATTR — File Attributes
- **API:** GEMDOS functions 4, 17, 18
- **Purpose:** Set baud rate, get/set file attributes, rename files

### DTIME / DGETDATE / DGETTIME
- **API:** GEMDOS functions 28, 29, 30
- **Purpose:** Get/set system date and time

---

## 3. Process Management APIs

### PEXEC — Execute Program
- **API:** GEMDOS function 13
- **Params:** D0 = drive, A0 = filename, A1 = arguments, A2 = old stack limit, A9 = new stack
- **Return:** D0 = PID or $FFFFFFFF on error

### PCREATE — Create Sub-process
- **API:** GEMDOS function 14
- **Purpose:** Spawn a new process (pre-PEXEC variant)

### PTERM / PSTATUS
- **API:** GEMDOS functions 15, 16
- **Purpose:** Terminate process; get process status
- **Return:** Error code or status

### PBASE / PINFO / PGETID / PSETID
- **API:** GEMDOS functions 19, 20, 21, 22
- **Purpose:** Get/set base address, process info, PID management

### PGETPRI / PSETPRI
- **API:** GEMDOS functions 35, 36
- **Purpose:** Get/set process priority level

### QUIT
- **API:** GEMDOS function 0
- **Purpose:** Terminate the current program

---

## 4. AES (Application Environment Services)

AES is called through the work area block (wopbuf) at AESBAS + $800 via TRAP #0.

### Window Management
| Fn | Name | Purpose |
|----|------|---------|
| 1 | `wind_create` | Create a window |
| 2 | `wind_open` | Open a window |
| 3 | `wind_close` | Close a window |
| 4 | `wind_delete` | Delete a window |
| 5 | `wind_calc` | Calculate window rectangle |
| 10 | `wind_get` | Get window attributes |
| 11 | `wind_set` | Set window attributes |
| 12 | `wind_iconify` | Iconify/deiconify |
| 13 | `wind_find` | Find window by title |

### Menu Management
| Fn | Name | Purpose |
|----|------|---------|
| 20 | `menu_bar` | Create/modify menu bar |
| 21 | `menu_init` | Initialize menu |
| 22-25 | `menu_*` | Deselect/select/exit/run |
| 26 | `menu_bar_rmv` | Remove menu bar |
| 31-33 | `menu_item_*` | Add/insert/delete menu items |

### Dialog Management
| Fn | Name | Purpose |
|----|------|---------|
| 30 | `dialog` | Display dialog box from resource file |
| 40-42 | `c_di*` | Dialog control functions |

### AES Event Codes
| Code | Meaning |
|------|---------|
| 1 | Mouse button click |
| 2 | Menu bar changed |
| 12-14 | AES suspend/resume/alert |

---

## 5. VDI (Virtual Device Interface)

VDI is accessed via TRAP #0. The handle is obtained from `vdi_open_workstation()`, then work functions are selected via the core table (INT16 array).

### Workstation Management
| Function | Description |
|----------|-------------|
| OPEN_WORKSTATION (0x1001) | Open device and return handle |
| CLOSE_WORKSTATION (0x1002) | Close workstation |
| CREATE_WORKSPACE (0x1022) | Create workspace |
| DELETE_WORKSPACE (0x1023) | Delete a workspace |

### Viewport / Window Management
| Function | Description |
|----------|-------------|
| SET_VIEWPORT (0x1006) | Set mapping from logical to physical coords |
| GET_VIEWPORT (0x1010) | Get viewport parameters |
| SET_WINDOW (0x1011) | Set window logical coordinates |
| GET_WINDOW (0x1012) | Get window parameters |
| CLEAR_WINDOW (0x1013) | Clear all work functions in window |
| CREATE_CLIP (0x1014) | Create clipping workspace |
| SELECT_CLIP (0x1016) | Select clipping workspace active |

### Device Information
| Function | Description |
|----------|-------------|
| GET_DEVICE_INFO (0x1004) | Get device capabilities |
| GET_DEVICE_MAP (0x1005) | Get device descriptor |
| CONTROL_DEVICE (0x1003) | Control device-specific parameters |

### Work Function Selection
| Work Fct | Purpose |
|----------|---------|
| 1 | DRAW — draw lines, points, arcs, boxes, polygons |
| 2 | FILL — fill areas, boxes, polygons |
| 3 | PRINT — text rendering via GDOS |
| 4 | MARKER — draw marker symbols |
| 5 | STAMP — draw stamp (bit-block transfer) |

### Drawing Primitives (Work Function 1)
| Work Fn | Description |
|---------|-------------|
| 52 | DRAW_POINT — single point |
| 91 | DRAW_LINE — line between two points |
| 128 | DRAW_ARC |
| 129 | DRAW_POLYLINE |
| 130 | DRAW_POLYGON |
| 153 | DRAW_ELLIPSE |
| 154 | DRAW_ELLIPTICAL_ARC |
| 155 | DRAW_FILLED_ELLIPSE |
| 156-158 | DRAW_PIE slices |

### Fill Primitives (Work Function 2)
| Work Fn | Description |
|---------|-------------|
| 168 | FILLED_BOX |
| 169 | FILLED_POLYGON |

### Color / Palette Management
| API | Description |
|-----|-------------|
| V_set_color (0x2000) | Set color mask for a color |
| V_get_color_mask | Get current color mask |
| V_set_palette (0x2100) | Set palette entry (R,G,B) |
| V_get_palette (0x2101) | Query palette |
| V_set_all_colors (0x2102) | Set all 16 palette entries |

### Status Codes
| Code | Meaning |
|------|---------|
| 0 | Success |
| -1 | Internal error |
| -2 | Device off-line |
| -4 | Bad parameter |
| -5 | No such device/window |
| -27 | Not supported |
| -25 | Format error |
| -31 | Memory unavailable |

---

## 6. GDOS (Generalized Device-Independent OS)

GDOS provides device-independent font rendering and text output.

### Font Management
- Multiple font families (built-in, user-defined, downloaded)
- Font size: 1pt to 2048pt (in font units, not pixels)
- Weight: 1-400 (normal, bold, etc.)
- Style: normal, italic, underline, strike-through

### Text Output
- Device-independent text rendering
- Uses VDI as the graphics back-end
- Supports printing via GDOS print driver

---

## 7. GEM (Graphics Environment Manager) / FSM (File Switch Manager)

### GEM Functions
| API | Description |
|-----|-------------|
| GEM_INFO (GEMDOS 37) | Get GEM version info |
| GEMTRM (GEMDOS 38) | Get GEM termination address |
| GEMSIZE (GEMDOS 39) | Get GEM max allocation |

### FSM (File Switch Manager)
- Provides a single-file interface to heterogeneous media
- Maps file type suffixes to application handlers
- Mounted as a virtual disk with `.DRIVE` extensions

---

## 8. XBIOS (Extended BIOS) — System Services

XBIOS is accessed via TRAP #2 with function number in D0.

### System Information / Setup
| Fn | Name | Description |
|----|------|-------------|
| 48 | `KBDSTAT` | Get keyboard status |
| 49 | `KBDCHAR` | Get next keyboard character |
| 50 | `KBDVBASE` | Get IKBD vector table base |
| 51 | `MOUSE` | Get mouse position/buttons |
| 53 | `CRTCORE` | Get Core TOS version |
| 54 | `RESMON` | Set/get monitor type |

### Video / Display
| Fn | Name | Description |
|----|------|-------------|
| 31 | `VSETRES` | Set video resolution |
| 32 | `VGETRES` | Get video resolution |
| 33 | `VSYNC` | Wait for vsync |
| 37 | `V_GETBASE` | Get framebuffer base address |
| 43 | `V_SETPAL` | Set palette |
| 63 | `VGETMOD` | Get video mode |
| 64 | `VSETPAL` | Set palette (alternate) |
| 72 | `VGETPAL` | Get palette |

### Blitter (STE+)
| Fn | Name | Description |
|----|------|-------------|
| 28 | `BLTINIT` | Initialize blitter |
| 29 | `BLTMODE` | Set blitter mode |
| 30 | `BLTCMD` | Execute blitter command |
| 49 | `BLTSTAT` | Get blitter status |
| 64 | `BLITMST` | Get blitter mode |

### Sound (STE+)
| Fn | Name | Description |
|----|------|-------------|
| 34 | `VSNDINIT` | Initialize sound (STE) |
| 35 | `VSNDCTL` | Sound control |
| 36 | `VSNDIN` | Sound input |
| 37 | `VSNDOUT` | Sound output |
| 38 | `VSNDSTA` | Sound status |
| 39 | `VSNDLEN` | Sample length |
| 40 | `VSNDADDR` | Sample address |

### Memory Controller (STE+)
| Fn | Name | Description |
|----|------|-------------|
| 17 | `RESMCR` | Get/set memory controller |
| 52 | `RESMCR` | Get/set memory controller (STE) |

### RS232 / Serial
| Fn | Name | Description |
|----|------|-------------|
| 15 | `RS232INIT` | Initialize serial port |
| 16 | `RS232STAT` | Get port status |
| 17 | `RS232INPUT` | Read from port |
| 18 | `RS232OUTPUT` | Write to port |

### Timer / Synchronization (MultiTOS)
| Fn | Name | Description |
|----|------|-------------|
| 95 | `EXEC` | Execute new process |
| 96 | `TERM` | Terminate process |
| 97 | `WAKE` | Wake up a process |
| 98 | `SEMAPHORE` | Semaphore operations |
| 99 | `EVENT_FL` | Set event flags |
| 100 | `GETSCS*` | Various SCSI/status calls |
| 44 | `TIMSET` | Set timer callback |

---

## 9. BIOS (Basic Input/Output System)

BIOS is accessed via TRAP #10 (entry at $FFFFEC84).

### Display Control
| Fn | Name | Description |
|----|------|-------------|
| 6 | `GEMGTR` | Set/get palette |
| 9 | `GEMCTL` | GEM video control |
| 11 | `BIOSWTCH` | Switch video modes |
| 32 | `SCRMOD` | Set/get video mode (STE) |

### Disk I/O
| Fn | Name | Description |
|----|------|-------------|
| 13 | `BDBASE` | Set disk base address ($FF8E00) |
| 14 | `GETBVB` | Get VDI pointer |
| 24 | `BLKIN` | Block input from device |
| 25 | `BLKOUT` | Block output to device |
| 26 | `DEVINIT` | Device initialization |

### Console I/O
| Fn | Name | Description |
|----|------|-------------|
| 20 | `CONIN` | Console input |
| 21 | `CONOUT` | Console output |
| 22 | `CONCTRL` | Console control |
| 23 | `CONIOCTL` | Console I/O control |

### DMA / Blitter (STE)
| Fn | Name | Description |
|----|------|-------------|
| 16 | `DMA` | DMA memory manager |
| 34 | `DIOCTL` | DMA I/O control (Mega STE) |
| 36 | `SCREG` | Screen record I/O (STE) |
| 41 | `MSTGEMGTR` | Extended palette (STE) |
| 46 | `BLKINX` | Extended block input |
| 47 | `BLKOUTX` | Extended block output |

### Power / Sleep
| Fn | Name | Description |
|----|------|-------------|
| 12 | `SLEEP` | Sleep/delay |
| 0 | `CRYPT` | RAM integrity check |
| 1 | `RESINH` | I/O unit initialization |

---

## 10. Interrupt Vectors

### Exception Vectors (at $0000)
| Offset | Purpose | Default Address |
|--------|---------|----------------|
| $0020-003C | Reserved (TRAP #0-3) | $006C |
| $0084-0088 | TRAP #0 | $0D9C (VDI) |
| $0088-008C | TRAP #1 | $8B62 (XBIOS) |
| $00BA-00BE | TRAP #14 | GEMDOS |
| $00C2 | TRAP #10 | BIOS |
| $00E4 | TRAP #1E | VDI |

### Hardware Interrupt Sources
| Level | Source | Vector |
|-------|--------|--------|
| 7 | MFP (IRQ B) | $007C |
| 6 | IKBD keyboad/mouse | $007C |
| 5 | ACIA serial/MIDI | $007C |
| 4 | Shifter (VBLI) | $007C |
| 3 | Timer C | $0078 |
| 2 | IRQ2 | $0074 |
| 1 | Unused | $0070 |
| NMI | VBLI (Shifter) | $006C |
| 8 (RESET) | Cold boot | $0004 |

### Deferred / Immediate VBI Vectors
| Address | Purpose |
|---------|---------|
| $0222 | VVBLKI — immediate VBI handler (usually $E7D1) |
| $0224 | VVBLKD — deferred VBI handler (usually $E93E) |
| $12-$14 | RTCLOK — system tick counter |
| $21-$22 | RTCLOK high byte |
| $345-$346 | VBL counter |

---

## 11. Cookie Jar

Located at pointer given by GEMDOS GEMINFO. Contains system info as 4-byte tag/value pairs.

| Cookie | Type | Content |
|--------|------|---------|
| `TOS1` | WORD | TOS major |
| `TOS2` | WORD | TOS minor |
| `TOS3` | DWORD | Full TOS version |
| `STe1` | WORD | 1 if STE detected |
| `MEGA` | WORD | 1 if Mega STE |
| `TT00` | WORD | 1 if TT |
| `FCON` | WORD | 1 if Falcon |
| `HRES` | WORD | Hi-Res support |
| `SRES` | WORD | Super Hi-Res |
| `MCHR` | WORD | Memory controller version |
| `SCR0` | DWORD | Screen buffer pointer |
| `RSC0` | DWORD | Resource file pointer |
| `CPU ` | DWORD | CPU type (68000/030/040) |
| `MMU ` | DWORD | MMU type |
| `RAM ` | DWORD | Total RAM in KB |

---

## 12. Keyboard and Input APIs

### IKBD (6502-based Keyboard Controller)
- **Address:** $FF8800-$FF88FF
- **Interrupt:** Level 6 via MFP IRQ B
- **Commands:** $01 = keyboard ready, $02 = mouse ready, $08 = joystick status

### IKBD Command Register
| Addr | Register | Purpose |
|------|----------|---------|
| $FF860C | ICR | IKBD command register |
| $FF860E | IIR | IKBD interrupt status |

### MFP (68901)
| Addr | Register | Purpose |
|------|----------|---------|
| $FF9A01 | GP_IO_A | General-purpose I/O |
| $FF9A05 | DDA | Data direction A |
| $FF9A07 | IER_A | Interrupt enable A |
| $FF9A09 | IER_B | Interrupt enable B |
| $FF9A0B | IPR | Interrupt pending |
| $FF9A0D | ISR | Interrupt service |
| $FF9A17 | IRQ_VEC | Interrupt vector base |
| $FF9A19/TNA | Timer A control |
| $FF9A1B/TNB | Timer B control |
| $FF9A1D/SNC | Timer C/D control |
| $FF9A1F/TCA | Timer A count |
| $FF9A21/TCB | Timer B count |
| $FF9A23/TCC | Timer C count |
| $FF9A25/TCD | Timer D count |
| $FF9A27/SYNC | Sync character |
| $FF9A2B/RSR | Receiver status |
| $FF9A2D/TSR | Transmitter status |
| $FF9A2F/USART | UART data register |

---

## 13. Hardware Access APIs

### Plaitter ($FFFF8400-$FFFF843F)
| Addr | Register | Purpose |
|------|----------|---------|
| $FFFF8400 | BLTCOA | Operand A address |
| $FFFF8402 | BLTCOB | Operand B address |
| $FFFF8404 | BLTCOC | Operand C address |
| $FFFF8406 | BLTCOD | Operand D address |
| $FFFF8410 | BLTCON0 | Control register 0 |
| $FFFF8412 | BLTCON1 | Control register 1 |

### Shifter Video Registers ($FFFF8200)
| Addr | Register | Purpose |
|------|----------|---------|
| $FFFF8200-$FFFF820E | VID_COLOR0-5 | Palette entries (RGB) |
| $FFFF8210-$FFFF8212 | VID_BASEADD | Framebuffer base (3B) |
| $FFFF8214-$FFFF8218 | VID_PTR* | Address pointer |
| $FFFF821A | VID_SYNCCTRL | Sync control |
| $FFFF8224 | VID_DPR | Data path register |
| $FFFF8208 | VID_PTR | Address pointer |

### YM2149 PSG Audio ($FFFF8800)
| Addr | Register | Purpose |
|------|----------|---------|
| $FFFF8800 | Reg 6 | Tone A (low byte) |
| $FFFF8801 | Reg 7 | Tone B (low byte) |
| $FFFF8802 | Reg 8 | Tone C (low byte) |
| $FFFF8803 | Reg 9 | Control (mixer/tone) |
| $FFFF8804-$FFFF8805 | Reg A-B | Tone A/B/C high byte |
| $FFFF8806-$FFFF8816 | Reg C-D | Noise period |
| $FFFF8817-$FFFF881B | Reg E-F | Timer control |
| $FFFF881C-$FFFF881F | Reg 10-13 | Tone/Noise period |
| $FFFF8820-$FFFF8823 | Reg 14-17 | Volume A/B/C/Noise |

---

## Quick Reference Summary

### API Entry Points
| API Family | TRAP | Entry Point |
|------------|------|-------------|
| GEMDOS | 14 | $FFFC3C |
| BIOS | 10 | $FFFFEC84 |
| AES/GEM | 0 | $FF9C |
| VDI | 0 | $FF9C |
| XBIOS | 2 | $8B62 |

### Common System Variables ($0000-$003F)
| Offset | Name | Purpose |
|--------|------|---------|
| $0000 | INIT_SP | Initial supervisor stack |
| $0004 | INIT_PC | Initial PC on reset |
| $007C | AESBAS | AES work area base |
| $009A | VDIHAND | VDI handle array |
| $00A6 | RESMON | Monitor type |
| $00AA | CURFREE | Pointer to free memory |
| $00AE | AVLMAX | Largest free block |
| $00B6 | APPLMAX | Largest app block |
| $011E | KBD_VBASE | IKBD vector table base |
| $014E | PID | Current process ID |
| $0152 | PDB | Process descriptor base |
| $016A | RESMCR | Memory controller ver |
| $0176 | CURVER | Current video resolution |
| $0178 | RESMOD | Active video mode |
| $017C | RESMOD | Video mode (alternate) |
| $018D | RSCFILE | Resource file handle |

### Most Common API Calls
| Call | Use |
|------|-----|
| `trap #14` | GEMDOS (files, memory, processes) |
| `trap #10` | BIOS (I/O, disk, console, DMA) |
| `trap #0` | AES/GEM (windows, menus, dialogs) |
| `trap #0` | VDI (graphics, fonts, colors) |
| `trap #2` | XBIOS (hardware access) |
