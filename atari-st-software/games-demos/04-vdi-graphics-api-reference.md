# VDI Graphics API and Functions Reference

## 1. VDI Fundamentals

### VDI Architecture Overview

The VDI (Virtual Device Interface) provides a device-independent graphics API. It abstracts hardware differences so programs work across all display resolutions and color depths.

```
VDI Architecture Layers:
    ┌──────────────────────────┐
    │    Application Layer     │  ← Program uses VDI functions
    ├──────────────────────────┤
    │  VDI Core Functions      │  ← VDI entry point via TRAP #0
    ├──────────────────────────┤
    │  Device Resolution       │  ← Screen resolution translation
    ├──────────────────────────┤
    │  Virtual Workstation     │  ← Workstation abstraction
    ├──────────────────────────┤
    │  Device Driver Layer     │  ← Screen driver (VID_*)
    ├──────────────────────────┤
    │  Hardware Layer          │  ← Shifter, GST, GPU
    └──────────────────────────┘
```

### VDI Device Drivers

```
VDI device numbers for Atari ST:
    Device #0 = Low-res (320x200, 16 color)
    Device #1 = Medium-res (640x200, 4 color)
    Device #2 = High-res (640x400, 2 color)
    Device #3 = Super-hires (STe Hi-Res, 640x512, 16 color)
    Device #4 = Double high (1280x400, 2 color)
    Device #5 = PAL mode (320x256, 16 color)
```

### VDI Standard Formats

```
VDI standard format for raster images:
    - Device-independent pixel representation
    - Color indices use 0-based indexing
    - Coordinate system: origin at top-left
    - Units: pixels (always, regardless of device)
    - Coordinate range: -32767 to +32767
```

## 2. VDI Workstation Lifecycle

### VDI Initialization Sequence

```c
/* Standard VDI initialization sequence */

/* 1. Establish workstation handle */
workstation_handle = vdi_open_workstation(device_number);

/* 2. Open the workstation */
vdi_open_workstation(workstation_handle);

/* 3. Set input mode */
vdi_set_input_mode(workstation_handle, 0);      /* No pointer */
vdi_set_input_mode(workstation_handle, 1);      /* Pointer + no keyboard */
vdi_set_input_mode(workstation_handle, 2);      /* Pointer + keyboard */
vdi_set_input_mode(workstation_handle, 3);      /* Pointer + keyboard (default) */

/* 4. Set current work function */
vdi_set_workstation_function(workstation_handle, current_wf);

/* 5. Set current line width */
vdi_set_line_width(workstation_handle, width_value);

/* 6. Set current color */
vdi_set_color(workstation_handle, color_index);

/* 7. Open logical workstation (create workspace) */
vdi_open_logical_workstation(workstation_handle);
```

### Complete Workstation Lifecycle

```
workstation_handle = vdi_open_workstation(dev_num);
vdi_open_workstation(workstation_handle);
vdi_open_logical_displays(workstation_handle, num_workspaces);

for each workload:
    set_viewport(workspace_handle, logical_coordinates);
    set_window(viewport_handle, physical_coordinates);

    for each graphic object:
        vdi_draw_object(workstation_handle, object_type, *coordinates, *attrs);

vdi_close_logical_displays(workstation_handle);
vdi_close_workstation(workstation_handle);
```

### Workstation and Workspace Handle Types

```
Handle types in VDI:
    workstation_handle: 16-bit integer identifying the workstation
    workspace_handle: 16-bit integer for each workspace
    viewport_handle: 16-bit integer for each viewport
    return_handle: 16-bit integer (always 1 for ST)
```

## 3. VDI Core Function Tables

### Screen Management Functions (0x1000-0x106F)

| Function | Description | Parameters | Return |
|----------|-------------|------------|--------|
| 0x1000 | OPEN WORKSTATION | dev_num → handle | 0 = success |
| 0x1001 | CLOSE WORKSTATION | handle → | 0 = success |
| 0x1002 | CREATE WORKSPACE | handle → workspace_handle | num_workspaces |
| 0x1003 | DELETE WORKSPACE | workspace_handle → | 0 = success |
| 0x1004 | CREATE VIEWPORT | workspace, logical coords → viewport_handle | 0 = success |
| 0x1005 | DELETE VIEWPORT | workspace_handle → viewport_handle → | 0 = success |
| 0x1006 | SET VIEWPORT | viewport_handle, coords → | 0 = success |
| 0x1010 | GET VIEWPORT | viewport_handle → coords | 0 = success |
| 0x1011 | SET WINDOW | window_handle, coords → | 0 = success |
| 0x1012 | GET WINDOW | window_handle → coords | 0 = success |
| 0x1013 | CLEAR WINDOW | window_handle → | 0 = success |
| 0x1014 | CREATE CLIP WINDOW | window_handle → clip_wks | 0 = success |
| 0x1015 | DELETE CLIP WINDOW | clip_wks → | 0 = success |
| 0x1016 | SELECT CLIP WINDOW | clip_wks → | 0 = success |
| 0x1017 | SET CLIP WINDOW | clip_wks_handle, coords → | 0 = success |
| 0x1018 | GET CLIP WINDOW | clip_wks_handle → coords | 0 = success |

### Attribute Management Functions (0x1300-0x13FF)

| Function | Description | Parameters | Return |
|----------|-------------|------------|--------|
| 0x1300 | SET CUR. WORK FCT. | current_wf → | 0 = success |
| 0x1301 | GET CUR. WORK FCT. | → current_wf | 0 = success |
| 0x1302 | REQUEST INPUT MODE | → input_mode | mode |
} 0x1303 | SET INPUT MODE | input_mode → | 0 = success |
| 0x1304 | REQUEST LINE TYPE | → line_type | type |
| 0x1305 | SET LINE TYPE | line_type → | 0 = success |
| 0x1308 | REQUEST LINE WIDTH | → line_width | width |
| 0x1309 | SET LINE WIDTH | line_width → | 0 = success |
| 0x1330 | REQUEST COLOR | → color_index | color |
| 0x1331 | SET COLOR | color_index → | 0 = success |
| 0x1334 | REQUEST COLOR PAIR | → color_pair_id | 0 = success |
| 0x1335 | SET COLOR PAIR | color_pair_id → | 0 = success |
| 0x1340 | REQUEST FILL TYPE | → fill_type | 0 = success |
| 0x1343 | REQUEST FILL AREA | → fill_area | 0 = success |
| 0x1344 | SET FILL AREA | fill_area → | 0 = success |
| 0x1350 | REQUEST FILL COLOR | → fill_color | 0 = success |
| 0x1351 | SET FILL COLOR | fill_color → | 0 = success |
| 0x1360 | REQUEST EDGE STYLE | → edge_style | 0 = success |
| 0x1361 | SET EDGE STYLE | edge_style → | 0 = success |
| 0x1370 | REQUEST EDGE COLOR | → edge_color | 0 = success |
| 0x1371 | SET EDGE COLOR | edge_color → | 0 = success |
| 0x1380 | REQUEST MARKER TYPE | → mark_type | 0 = success |
| 0x1381 | SET MARKER TYPE | mark_type → | 0 = success |
| 0x1382 | REQUEST MARKER SIZE | → marker_size | 0 = success |
| 0x1383 | SET MARKER SIZE | marker_size → | 0 = success |
| 0x1388 | REQUEST MARKER COLOR | → marker_color | 0 = success |
| 0x1389 | SET MARKER COLOR | marker_color → | 0 = success |
| 0x1390 | SET STAMP WIDTH | stamp_width → | 0 = success |
| 0x1391 | GET STAMP WIDTH | → stamp_width | 0 = success |
| 0x1392 | SET STAMP HEIGHT | stamp_height → | 0 = success |
| 0x1393 | GET STAMP HEIGHT | → stamp_height | 0 = success |
| 0x1394 | SET STAMP COLOR | stamp_color → | 0 = success |
| 0x1395 | GET STAMP COLOR | → stamp_color | 0 = success |

### Device Control Functions (0x1400-0x14FF)

| Function | Description | Parameters | Return |
|----------|-------------|------------|--------|
| 0x1400 | CONTROL DEVICE | control_code → | 0 = success |
| 0x1401 | GET DEVICE INFO | → device_info | 0 = success |
| 0x1402 | GET DEVICE MAP | → device_map | 0 = success |
| 0x1403 | GET VIRTUAL DEVICE | → virtual_device | 0 = success |
| 0x1404 | REQUEST INPUT CAP | → input_capabilities | 0 = success |
| 0x1405 | REQUEST OUTPUT CAP | → output_capability | 0 = success |
| 0x1406 | QUERY DEVICE | device_id → supported | 0 = success |
| 0x1420 | GET COLOR CAP | → color_capabilities | 0 = success |
| 0x1440 | CLEAR SCREEN | → | 0 = success |
| 0x1441 | GET SCREEN SIZE | → screen_width, height | 0 = success |

### Core Drawing Functions (0x1500-0x15FF)

| Function | Name | Description | Parameters | Return |
|----------|------|-------------|------------|--------|
| 0x1500 | DRAW POINT | Draw single point | x, y → | 0 = success |
| 0x1501 | GET POINT | Get point color | x, y → color | 0 = success |
| 0x1502 | DRAW POLYLINE | Draw connected lines | coords[] → | 0 = success |
| 0x1503 | DRAW POLYGON | Draw closed polygon | coords[] → | 0 = success |
| 0x1504 | DRAW ARC | Draw circular arc | center_x, y, radius, start_ang, end_ang → | 0 = success |
| 0x1505 | DRAW ARC FROM/TO | Draw arc with from/to points | cx, cy, start_x, y, end_x, y, radius → | 0 = success |
| 0x1506 | DRAW PICTURE | Draw pixmap | coords[], dims[] → | 0 = success |
| 0x1507 | GET PICTURE | Get pixmap data | → coords[], dims[], data[] | 0 = success |
| 0x1508 | DRAW POLYPOLYGON | Draw multiple polygons | coords[], npts[] → | 0 = success |
| 0x1509 | ERASE BOX | Erase rectangular area | x1, y1, x2, y2 → | 0 = success |
| 0x150A | DRAW VLINE | Draw vertical line | x, y1, y2 → | 0 = success |
| 0x150B | DRAW HLINE | Draw horizontal line | x1, y, x2 → | 0 = success |
| 0x150C | MARKER | Draw marker symbol | x, y → | 0 = success |
| 0x150D | DRAW ELLIPSE | Draw ellipse outline | cx, cy, radius_x, radius_y → | 0 = success |
| 0x150E | DRAW ELLIPSE ARC | Draw elliptical arc | cx, cy, rx, ry, start_ang, end_ang → | 0 = success |
| 0x150F | DRAW FILLED ELLIPSE | Draw filled ellipse | cx, cy, rx, ry → | 0 = success |
| 0x1510 | DRAW FILLED SPHERE | Draw filled sphere | cx, cy, radius → | 0 = success |
| 0x1511 | FILL AREA | Fill enclosed area | x, y, closure_type → | 0 = success |
| 0x1512 | DRAW FILLED BOX | Draw filled rectangle | x1, y1, x2, y2 → | 0 = success |
| 0x1513 | DRAW FILLED POLYGON | Draw filled polygon | coords[], n_pts → | 0 = success |
| 0x1514 | DRAW FILLED POLYLINE | Draw filled polyline | coords[], n_pts → | 0 = success |
| 0x1515 | DRAW FILLED POLYPOLYGON | Draw filled polypolygon | coords[], n_pts[] → | 0 = success |
| 0x1516 | DRAW FILLED ELLIPTICAL PIE | Draw filled pie slice | cx, cy, rx, ry, start_ang, end_ang → | 0 = success |
| 0x1517 | DRAW FILLED PIE | Draw filled pie | cx, cy, start_x, start_y, end_x, end_y, radius → | 0 = success |
| 0x1518 | DRAW FILLED ARC | Draw filled arc | cx, cy, rx, ry, start_ang, end_ang → | 0 = success |
| 0x1519 | DRAW FILLED BOX | Draw filled box | x1, y1, x2, y2 → | 0 = success |
| 0x151A | DRAW FILLED BOX | Draw filled box (alternate) | x1, y1, x2, y2 → | 0 = success |

### Text Functions (0x1D00-0x1DFF)

| Function | Name | Description | Parameters | Return |
|----------|------|-------------|------------|--------|
| 0x1D00 | REQUEST TEXT | → text_info | 0 = success |
| 0x1D01 | SET TEXT FMT | text_alignment, horiz, vert → | 0 = success |
| 0x1D02 | GET TEXT FMT | → text_alignment, horiz, vert | 0 = success |
| 0x1D03 | SET TEXT HT | text_height → | 0 = success |
| 0x1D04 | GET TEXT HT | → text_height | 0 = success |
| 0x1D05 | SET CHAR DIR | char_dir → | 0 = success |
| 0x1D06 | GET CHAR DIR | → char_dir | 0 = success |
| 0x1D07 | SET CHAR HT | char_height → | 0 = success |
| 1D08 | GET CHAR HT | → char_height | 0 = success |
| 0x1D09 | SET FONT NAME | font_name → | 0 = success |
| 0x1D0A | GET FONT NAME | → font_name | 0 = success |
| 0x1D0B | SET FONT | font_num → | 0 = success |
| 0x1D0C | GET FONT | → font_num | 0 = success |
| 0x1D0D | DRAW TEXT | x, y, "text" → | 0 = success |
| 0x1D0E | DRAW STRING | x, y, str_ptr → | 0 = success |
| 0x1D0F | GET TEXT SIZE | text, dims → | 0 = success |
| 0x1D10 | DRAW FILLED BOX | x1, y1, x2, y2 → | 0 = success |

### Color/Mask Functions (0x2000-0x20FF)

| Function | Name | Description | Parameters | Return |
|----------|------|-------------|------------|--------|
| 0x2000 | SET COLOR MASK | color_index, mask → | 0 = success |
| 0x2001 | GET COLOR MASK | color_index → mask | 0 = success |
| 0x2002 | SET COLOR PAIR | color_pair, pair_id → | 0 = success |
| 0x2003 | GET COLOR PAIR | color_pair, pair_id → | 0 = success |
| 0x2004 | COLOR MODE | → color_depth (2/4/8/16) | 0 = success |

### Palette Functions (0x2100-0x21FF)

| Function | Name | Description | Parameters | Return |
|----------|------|-------------|------------|--------|
| 0x2100 | SET COLOR PALETTE | color_index, r, g, b → | 0 = success |
| 0x2101 | GET COLOR PALETTE | color_index → r, g, b | 0 = success |
| 0x2102 | SET ALL COLORS | all_16_colors → | 0 = success |
| 0x2103 | SET PALETTE | all_colors → | 0 = success |
| 0x2104 | SET COLOR | color_index → | 0 = success |
| 0x2105 | GET COLOR | color_index → | 0 = success |
| 0x2106 | SET COLOR PALETTE | palette_entries → | 0 = success |
| 0x2107 | GET COLOR PALETTE | palette_entries → | 0 = success |
| 0x2108 | SET COLOR | color → | 0 = success |
| 0x2109 | GET COLOR | → color | 0 = success |
| 0x210A | SET COLOR | color → | 0 = success |
| 0x210B | GET COLOR | → color | 0 = success |

## 4. Workstation Configuration Functions

### Setting Input Modes

```
Input Mode Values:
    0:   No mouse cursor, keyboard input enabled
    1:   Mouse cursor, no keyboard input
    2:   Mouse cursor, keyboard disabled
    3:   Mouse cursor, keyboard enabled (default)

    move.w #3,-(sp)              ; input mode 3
    move.w #handle, -(sp)
    move.w #3,d0                 ; VDI REQUEST INPUT MODE
    move.w #3,d1                ; VDI FUNCTION
    trap #0                     ; VDI entry
    ; Returns: input_mode in D0
    addq.l #6,sp
```

### Setting Line Attributes

```
Line Width values:
    1:   Default line width
    2:   Double line width  
    3:   Triple line width
    :   Variable by device

Line Type values:
    0:   Solid line
    1:   Dashed line
    2:   Dotted line
    3:   Dash-dot line
    4:   Double-dash line

    move.w #1,-(sp)             ; Line width
    move.w #handle,-(sp)
    move.w #10,d0               ; Set line width
    trap #0                     ; VDI entry
```

### Setting Current Work Function

```
Work Function Values:
    0:   Draw function (default)
        Points, lines, circles, rectangles, polygons
    1:   Marker function
        Draw marker symbols
    2:   Fill function
        Fill enclosed areas
    3:   Print function
        Text output

    move.w #2,-(sp)             ; Work function 2 (fill)
    move.w #handle,-(sp)
    move.w #12,d0               ; VDI SET WORK FCT
    trap #0                     ; VDI entry
```

## 5. Coordinate System Mapping

### Logical vs Physical Coordinates

```
VDI coordinate mapping (viewport/window):

Logical coordinates (VDI space):
    Origin at top-left of logical workspace
    Units: device-independent units
    Range: -32768 to +32767

Physical coordinates (screen space):
    Origin at top-left of physical viewport
    Units: pixels
    Range: 0 to viewport_width-1, 0 to viewport_height-1

    vdi_set_viewport(workspace_handle, logical_x1, y1, logical_x2, y2, ...);
    vdi_set_window(  workspace_handle, logical_x1, y1, physical_x2, physical_y2, return_handle);

    Logical (0,0) → Physical (0,0) when viewport == window
    Logical (width,height) → Physical (viewport_width, viewport_height)
```

### Example Coordinate Mapping

```c
/* Set up 640x400 logical coordinate system in 320x200 physical viewport */
vdi_set_viewport(workspace_handle, 0, 0, 319, 199, return_handle);
vdi_set_window(  workspace_handle, 0, 0, 639, 399, return_handle);

/* Now when drawing at logical coordinate (320, 200),
   it maps to physical pixel (159, 99) */
```

## 6. Raster Operations (Raster Transfer)

### VDI Raster Functions

| Function | Description | Parameters | Return |
|----------|-------------|------------|--------|
| 0x2500 | GET RASTER | x, y, width, height, data_ptr → | 0 = success |
| 0x2501 | SET RASTER | x, y, width, height, data_ptr → | 0 = success |
| 0x2502 | RASTER MAP | src_x, y, dst_x, y, width, height, mask → | 0 = success |
| 0x2503 | RASTER BIT | src_x, y, dst_x, y, width, height → | 0 = success |
| 0x2504 | RASTER COPY | src_x, y, dst_x, y, width, height → | 0 = success |
| 0x2505 | RASTER XOR | src_x, y, dst_x, y, width, height → | 0 = success |
| 0x2506 | RASTER AND | src_x, y, dst_x, y, width, height → | 0 = success |
| 0x2507 | RASTER OR | src_x, y, dst_x, y, width, height → | 0 = success |
| 0x2508 | RASTER NAND | src_x, y, dst_x, y, width, height → | 0 = success |
| 0x2509 | RASTER NOR | src_x, y, dst_x, y, width, height → | 0 = success |
| 0x250A | RASTER XNOR | src_x, y, dst_x, y, width, height → | 0 = success |
| 0x250B | BIT BLT | x, y, width, height, src_ptr → | 0 = success |
| 0x250C | WORD BLT | x, y, width, height, src_ptr → | 0 = success |
| 0x250D | LONG WORD BLT | x, y, width, height, src_ptr → | 0 = success |
| 0x250E | RASTER FILL | x, y, width, height, pattern → | 0 = success |
| 0x250F | RASTER MASK | x, y, width, height, mask → | 0 = success |

### Blitter Raster Function Example

```asm
; Copy rectangle using VDI raster MAP (VDI function 0x2502)
raster_map_copy:
    move.w #handle,d0          ; VDI device handle
    move.w #1010,d1            ; VDI RASTER MAP (0x2502 = ROP3)
    lea params_pc,a2           ; parameter array
    move.w #16,-(sp)           ; param count
    trap #0                     ; VDI entry
    addq.l #2,sp
    rts

params:
    dc.w 0,0                     ; source x1,y1
    dc.w 320,200                 ; source x2,y2
    dc.w 160,100                 ; dest x1,y1
    dc.w 480,300                 ; dest x2,y2
    dc.w 320,200                 ; width,height
    dc.w 0                       ; mask (0 = none)
```

## 7. Seed Fill (Region Fill) Function

```c
/* SeedFill function for flood-fill */
/* VDI function: 0x2514 (Seed Fill) */

/* SeedFill parameters: */
x = start_x;
y = start_y;
max_dist = max_distance;  /* max fill distance */
closure = fill_closure;   /* closure type */
seed_color = current_color;     /* seed fill boundary color */
```

### Seed Fill Parameters

```
Closure types:
    0:   Fill all boundary colors
    1:   Fill until boundary color found
    2:   Fill until non-boundary color

SeedFill algorithm:
    1. Start at seed point (x,y)
    2. Check pixel color against boundary color
    3. If boundary, stop filling
    4. If match, fill with current color
    5. Continue in all 4 directions (flood fill)
    6. Stop at boundary or distance limit
```

## 8. VDI Device Information

### GET DEVICE INFO Function

```
VDC GET DEVICE_INFO (VDI function $0x1401):
    Returns device capabilities in array[7]:
        index 0: max resolution (pixels)
        index 1: number of colors (0=monochrome)
        index 2: minimum line width
        index 3: maximum line width (0=variable)
        index 4: line width granularity
        index 5: number of workstations
        index 6: workstations available
        index 7: virtual device type (0-255)
```

### GET DEVICE MAP Function

```
VDI GET DEVICE MAP ($0x1402):
    Returns device capabilities:
        index 0: device number (0-255)
        index 1: device name (19 chars)
        index 2: vendor (10 chars)
        index 3: max resolution
        index 4: number of colors
        index 5: min line width
```

## 9. VDI Window and Viewport Management

### Window Parameters

```
vdi_set_window parameters:
    virtual_wks_handle    - workspace handle
    logical_left          - left coordinate
    logical_top           - top coordinate
    logical_right         - right coordinate
    logical_bottom        - bottom coordinate
    return_handle         - always 1

vdi_set_viewport parameters:
    virtual_wks_handle    - workspace handle
    physical_left         - left pixel coordinate
    physical_top          - top pixel coordinate
    physical_right        - right pixel coordinate
    physical_bottom       - bottom pixel coordinate
    return_handle         - always 1
```

### Viewport Transformation Example

```c
/* Setup viewport from window: logical coords (0,0) -> (639,399) 
   map to physical coords (0,0) -> (319,199) */
vdi_set_window(workspace_handle, 0, 0, 639, 399, &return_handle);
vdi_set_viewport(workspace_handle, 0, 0, 319, 199, &return_handle);

/* Drawing a 320x200 pixel rectangle at logical coordinates (0,0): */
x1 = 0; y1 = 0;
x2 = 319; y2 = 199;

vdia_draw_filled_box(workspace_handle, x1, y1, x2, y2);

/* This draws at physical (0,0) -> (159,99) due to 2:1 scaling */
```

## 10. VDI Text Rendering

### Text Drawing Functions

```
VDI text drawing (VDI function 0x1D0D):
    Text is drawn at (x, y) position.
    Text rendering uses GDOS font driver.
    Text size controlled by SET TEXT HT (VDI 0x1D03).
    Text orientation controlled by SET CHAR DIR (VDI 0x1D05).

    Text positioning:
        Origin at baseline, left edge
        Characters drawn above baseline (positive Y)
```

## 11. Complete VDI Example Program

```asm
; Standard VDI graphics example
; Draws a filled rectangle at specified coordinates

vdia_example:
; Initialize VDI
    lea vdi_work,a2              ; VDI workstation block
    move.w #1,d0                 ; device number (default display)
    trap #0                     ; VDI OPEN WORKSTATION
    move.w d0,work_handle    ; save handle
    move.w d0,v_handle         ; save virtual screen handle
    move.w v_handle,-(sp)
    move.w #16,-(sp)           ; 16 workspaces
    move.w #16,-(sp)           ; return workspace count
    trap #0                     ; VDI CREATE WORK
    ; Workspaces created

; Set color to red (index 2)
    lea palette_color,a0       ; color palette entry
    move.w #2,-(sp)           ; color index
    trap #0                     ; VDI set_color
    addq.l #2,sp

; Draw lines
    lea line_coord,a1
move.w #handle,d0
    move.w #300,d1           ; handle
    lea pts,a2                ; param block
    trap #0                     ; VDI draw polyline

; Fill rectangle using work function 2 (fill)
move.w #2,-(sp)
    move.w #handle,-(sp)
    move.w #12,d0               ; SET WORK FCT 2 (fill)
    trap #0                     ; VDI entry
    addq.l #4,sp

    move.w #3,d0               ; work function 2 (fill)
    move.w #handle,-(sp)
    trap #0                     ; VDI SET WORK FCT
    addq.l #2,sp

draw_box:
    move.w #handle,d0
    move.w #12,d1
    lea params,a2
    trap #0
    rts

; Cleanup
    move.w v_handle,-(sp)
    move.w #16,-(sp)
    trap #0                     ; DELETE WORKSPACES
    addq.l #4,sp

    move.w #handle,-(sp)
    trap #0                     ; VDI CLOSE WORKSTATION
    addq.l #2,sp
    rts

; Data structures
params:
    dc.w 50,50            ; x1,y1
    dc.w 270,150          ; x2,y2

palette_color:
    dc.w 2,193,167,153    ; color red (R=193, G=167, B=153)
work_handle:
    dc.w 0
v_handle:
    dc.w 0
pts:
    dc.w 0,0,100,0,100,100,0,100,0,0    ; polygon points
line_coord:
    dc.w 10,10,310,190         ; line from (10,10) to (310,190)
```

## 12. VDI Graphics Mode Configuration

### Setting Screen Resolution via VDI

```c
/* Set screen resolution via VDI */
vd_set_resolution(width, height, colors) {
    /* Device index for resolution:
       ST modes:
        0 = 320x200x16 (standard low-res)
        1 = 640x200x4 (medium res)
        2 = 640x400x2 (high res)
       STE modes:
        3 = 640x512x16 (HiVid)
        4 = 640x256x16 (super hi-res)
        5 = 1280x400x2 (double hi-res)
    */
    device = lookup_device(width, height, colors);
    workstation_handle = vdi_open_workstation(device);
}
```

## 13. VDI Function Status Codes

```
VDI return codes:
    E_OK        = 0       Success
    E_ERROR     = -1      Internal error
    E_DEVOFFL   = -2      Device off-line
    E_DYNERR    = -3      Dynamic error
    E_ARG       = -4      Bad parameter
    E_NOSUCH    = -5      No such device/window/viewport
    E_NOTSUPP   = -27     Not supported
    E_INVFN     = -3      Invalid function
    E_UNSUP    = -27      Unsupported operation
    E_FORMAT    = -25         Format error
    E_NOMEM     = -31     Memory unavailable
    E_WKSFULL   = -38     Workspace full
    E_NODRV     = -39     No driver available
    E_NOHNDL    = -40     No handle available
    E_WKSSRCMD  = -48     Workspace source mismatch
    E_INVWKS      = -49     Invalid workspace
    E_INVPORT   = -50     invalid viewport
    E_INVWIN    = -51     Invalid window
    E_INVCLP    = -52     invalid clip region
    E_INVPAI    = -53     Invalid color pair
    E_INVCOL    = -59     Invalid color
    E_NOTINP    = -66     Not in process
    E_INVFMT    = -2      Invalid format
    E_NODATA    = -14     (NOMEM for disk)
```

## 14. VDI Drawing Primitives Reference

### Point Drawing

```c
/* Draw point at (x, y) with current color */
vdia_draw_point(handle, x, y);

/* Get point color at (x, y) */
color = vdi_get_point(handle, x, v);
```

### Line Drawing

```c
/* Draw line from (x1,y1) to (x2, y2) */
vdia_draw_line(handle, x1, y1, x2, y2);

/* Drawing vertical line */
vdia_draw_vline(handle, x, y1, y2);

/* Draw horizontal line */
vpia_draw_hline(handle, x1, y, x2);
```

### Circle and Ellipse Drawing

```c
/* Draw circle: center (cx,cy), radius r */
vdia_draw_circle(handle, cx, cy, r);

/* Draw ellipse: center, radius X, radius Y */
vdia_draw_ellipse(handle, cx, cy, rx, ry);

/* Draw circular arc: center, radius, start_angle, end_angle */
vdia_draw_arc(handle, cx, cy, r, start_ang, end_ang);

/* Draw elliptical arc */
vdia_draw_arc_ellipse(handle, cx, cy, rx, ry, start_ang, end_ang);

/* Draw arc from point to point */
vdia_draw_arc_from_to(handle, cx, cy, start_x, start_y, end_x, end_y, r);
```

### Rectangle Drawing

```c
/* Draw rectangle outline */
vdia_draw_rect(handle, x1, y1, x2, y2);

/* Draw filled rectangle */
vdia_draw_filled_rect(handle, x1, y1, x2, y2);
```

### Polygon Drawing

```c
/* Draw polygon: n points at coords[n][2] */
vdia_draw_polygon(handle, n_points, coords_ptr);

/* Draw filled polygon: n points */
vdia_draw_filled_polygon(handle, n_points, coords_ptr);

/* Draw polyline (open chain of lines) */
vdia_draw_polyline(handle, n_points, coords_ptr);

/* Draw filled polyline */
vdia_draw_filled_polyline(handle, n_points, coords_ptr);

/* Draw polypolygon (multiple polygons) */
vdia_draw_polypolygon(handle, n_points, coords_ptr, num_points_ptr);

/* Draw filled polypolygon */
vdia_draw_filled_polypolygon(handle, n_points, coords_ptr, num_points_ptr);
```
