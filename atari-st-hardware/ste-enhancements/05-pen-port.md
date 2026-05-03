# Atari STe / Mega STE Optical Pen Port

> Comprehensive documentation of the STe pen/optical port: connector, signals, pen mechanism, register map, software API, accessories, and emulation notes.

## Overview

The Atari STe (1989) and Mega STE (1991) introduced support for an **optical pen** as a pointing device, supplementing the traditional mouse and joystick. The STe's GST (Granite South Traffic) MCU contains a dedicated analog-to-digital converter (ADC) and pen interrupt logic not present in the original TT030-based ST/Mega ST line.

### What Is It

The optical pen port is a **DB9 connector** on the rear panel labeled **PORT 2** on STe models (and PORT 2/PORT 3 as analog expansion ports on some diagrams). It carries two analog axes (X and Y coordinates from the pen's sensor array) and a digital trigger line. The pen itself detects bright flashes on the monitor surface via optical sensors and converts the light positions into analog voltages proportional to screen coordinates.

### Connector

| Detail | Value |
|---|---|
| **Connector type** | DB9 (DE-9) female, rear-panel |
| **Port designation** | Analog expansion port 2 (primary pen port) |
| **Physical location** | Rear I/O panel, shared pin-out with standard joystick port |
| **Signal families** | 2x analog voltage axes, 1x digital trigger, +5V supply, GND |

### Models with Pen Support

| Model | Pen Support | Notes |
|---|---|---|
| Atari STe (520STE / 1040STE) | Yes | Primary model; Port 2 is the pen port |
| Mega STE | Yes | Port 2 and Port 3 both support pen inputs |
| Atari ST (original, TT030-based) | No | No ADC or pen circuitry |
| Mega ST / Mega ST 2 | No | Same TT030-based architecture |
| Atari TT | No | TT used a different I/O architecture (SCSI mouse port) |

## Pinout and Signal Descriptions

### Pen Port (Port 2) DB9 Pinout

```
Port 2 (STe rear panel) ── Pen connector DB9

    Pin  ── Signal ────────── Type ─────── Description ─────────────────────────
    ┌───┬──────────────────────────────────────────────────────────────────────┐
  1 │ X  │ Analog pen horizontal axis          │ Voltage proportional to pen X  │
    │   │ (PEN_H / PEN_X)                     │ (0V-5V, from internal ADC)   │
    ├───┼──────────────────────────────────────────────────────────────────────┤
  2 │ Y  │ Analog pen vertical axis            │ Voltage proportional to pen Y  │
    │   │ (PEN_V / PEN_Y)                     │ (0V-5V, from internal ADC)   │
    ├───┼──────────────────────────────────────────────────────────────────────┤
  3 │ TR │ Pen trigger (digital)               │ Active-low pulse on trigger    │
    │   │ (PEN_TRIG / PEN_BTN)                │ Press = logic LOW (~0V)      │
    ├───┼──────────────────────────────────────────────────────────────────────┤
  4 │ NC │ No connection / NC                  │ Not connected                 │
    ├───┼──────────────────────────────────────────────────────────────────────┤
  5 │ NC │ NC                                │ Not connected                 │
    ├───┼──────────────────────────────────────────────────────────────────────┤
  6 │ NC │ NC                                │ Not connected                 │
    ├───┼──────────────────────────────────────────────────────────────────────┤
  7 │ NC │ NC                                │ Not connected                 │
    ├───┼──────────────────────────────────────────────────────────────────────┤
  8 │ GND │ Signal ground                    │ Ground reference              │
    │   │                                     │ Common return path            │
    ├───┼──────────────────────────────────────────────────────────────────────┤
  9 │ V+ │ +5V power supply                 │ Power for pen electronics     │
    │   │                                     │ (typically 4.75V-5.25V)       │
    └───┴──────────────────────────────────────────────────────────────────────┘
```

### Detailed Pin Descriptions

**Pin 1 – PEN\_H (Horizontal Analog Axis)**

Analog voltage output from the pen's horizontal position sensor. The internal SAR ADC of the GST MCU samples this line to produce an 8-bit X coordinate. The voltage range is 0V to +5V, linearly mapped to pixel column position. When no pen is connected, this line floats and reads unpredictable values.

**Pin 2 – PEN\_V (Vertical Analog Axis)**

Analog voltage output from the pen's vertical position sensor. Like the horizontal line, this is sampled by the GST MCU's SAR ADC to produce an 8-bit Y coordinate.

**Pin 3 – PEN\_TRIG (Pen Trigger)**

Digital input line. Normally high (pulled-up internally, ~+5V). When the pen trigger button is pressed, this line is pulled low (~0V) by the pen's internal switch. The GST MCU can generate an interrupt on the falling edge of this signal, notifying the CPU that the pen has been clicked.

**Pin 8 – GND**

Common ground reference for all signals.

**Pin 9 – +5V**

Regulated +5V supply powering the optical pen's internal electronics (photodiode amplifiers, voltage-controlled oscillators, and LEDs).

**Pins 4, 5, 6, 7**

Not connected (NC). These pins maintain DB9 physical compatibility with joystick/mouse connectors.

### Mechanical and Electrical Notes

- The pen port pinout is **electrically compatible** with the existing joystick ports (Port 0 and Port 1). In practice, the pen signal lines are routed to **Port 2** analog expansion inputs in the GST MCU.
- Port 3 on Mega STE is a duplicate of Port 2, providing a second analog pen connection.
- The connector uses the standard DE-9 shell used for Atari mouse and joystick ports, allowing the same cable housing with different wire-to-pin mappings.

## How the Optical Pen Works

### Pen Hardware Overview

The Atari STe optical pen (part number varies; often sold as an Atari accessory) is a hand-held pointing device that detects flashes of light on the CRT monitor screen. The mechanism works as follows:

```

    ┌───────────────────────────────────────────────────────────────────┐
    │                     Atari STe Optical Pen                         │
    │                                                                   │
    │   ┌─────────────────────────────────────────────────────────┐     │
    │   │                   Pen Housing                           │     │
    │   │                                                         │     │
    │   │    ┌──────┐      ┌──────────┐      ┌──────┐           │     │
    │   │    │LED1  │      │  MCU     │      │LED2  │           │     │
    │   │    │(red) │─────▶│(control)│◀─────│(IR)  │           │     │
    │   │    └──────┘      └────┬─────┘      └──────┘           │     │
    │   │                       │                                 │     │
    │   │                ┌──────▼──────┐                          │     │
    │   │                │  Trigger    │────── Pin 3 (PEN_TRIG)  │     │
    │   │                │  Switch     │                          │     │
    |   |                └─────────────┘                          │     |
    │   │                                                         │     │
    │   │    ┌──────────────────────────────┐                    │     │
    │   │    │  Sensor Array (rear tip)     │                    │     │
    │   │    │  ┌────┐  ┌────┐  ┌────┐     │                    │     │
    │   │    │  │PD1 │  │PD2 │  │PD3 │◄───│─────┼── CRT Light │     │
    │   │    │  │(X ) │  │(Y ) │  │TRG │     │     │ Flashes   │     │
    │   │    │  └────┘  └────┘  └────┘     │                    │     │
    │   │    └──────────────────────────────┘                    │     │
    │   │                                                         │     │
    │   │    Phosphor flash detection system:                     │     │
    │   │    The CRT refreshes in horizontal and vertical         │     │
    │   │    sync pulses. At each refresh, the pen's photodiodes  │     │
    │   │    detect the flash positions on screen.                │     │
    │   └─────────────────────────────────────────────────────────┘     │
    └───────────────────────────────────────────────────────────────────┘                        │
                                                                                                  │
    CRT Monitor:                                                                                   │
    ┌────────────────────────────────────────────────────────────────────────────┐                │
    │   SCREEN                                                                 │                │
    │                                                                            │                │
    │   <──── H-REFRESH (X axis pulse)─────────────────────┐                    │                │
    │   │                                                  │                    │                │
    │   │                                                  ▼                    │                │
    │   │   +────────────────────────────────────────────────────────│──────   │                │
    │   │   │                                                       │    │   │                │
    │   │   │               Cursor Position                         │    │   │                │
    │   │   │         (detected by pen at screen location)          │    │   │                │
    │   |   |                                                       |    |   |                |
    |   |   ▼                                                       |    |   |                |
    |   |   +────────────────────────────────────────────────────────────│──────   │                │
    │   │                                                            │                    │                │
    │   │                                                            ▼                    │                │
    |   |   V-REFRESH (Y axis pulse)                                                  |               |
    |   |                                                                 │            │
    |   +----------------------------------------------------------------─┘            │
    └────────────────────────────────────────────────────────────────────────────┘                │
```

### Signal Types

The pen output consists of three distinct signal types:

| Signal | Type | Origin | Description |
|---|---|---|---|
| **X axis** | Analog voltage | Phosphor flash position | Continuous voltage from pen sensor, proportional to horizontal screen position |
| **Y axis** | Analog voltage | Phosphor flash position | Continuous voltage from pen sensor, proportional to vertical screen position |
| **Trigger** | Digital (active low) | Pen trigger button | Logic LOW when pen button is pressed; generates interrupt on falling edge |

### Trigger Mechanism

The pen operates through a synchronized flash-detection cycle:

1. The CRT monitor refreshes the screen using horizontal and vertical synchronization pulses.
2. At each horizontal sync pulse, phosphor along the scanned line flashes briefly.
3. The pen's photodiode array detects the flash positions (the X and Y coordinates of the bright spot).
4. The pen's internal ADC converts the light position into two analog voltage levels.
5. These voltages travel over pins 1 and 2 to the GST MCU's SAR ADC for a final conversion to digital X and Y values.
6. When the user presses the pen trigger button, the trigger line (pin 3) goes low, and the GST MCU can fire a pen interrupt to the CPU.

### Trigger Timing Diagram

```

                            Trigger timing (user presses pen button)

    PEN_TRIG (Pin 3)    TTL logic
    (+5V ── high)
     │
     │                ┌───┐
     │                │   │
     │   ─────────────┘   └────────────────────────── (release)
     │
     │<── fall edge (pen interrupt) → CPU notified →──┐
     │                                                │
     │                                                │
     │                                          rise edge (end)
     │                                                │
     ▼                                                ▼
    0V ──────────────────────────────────────────────────────────── (GND)


    Corresponding CRT flash detection:

    X axis (Pin 1)    Analog voltage
    (+5V)              |   ┌────┐
     │                 |   │    │     ┌────┐
     │                 |   │    │     │    │
     │                 |___│    │______│    │_____
     │                                               
     ▼                                               
    
    Y axis (Pin 2)    Analog voltage
    (+5V)              |   ┌────────────────────┐
     │                 |   │                    │
     │                 |___│                    │_____
     │
     ▼


    Pen trigger button state:

    ─────────────  ┌──┐   ┌──┐   ───── (pressed/released cycles)
                    │  │   │  │
                    │  │   │  │
                    └──┘   └──┘

```

The trigger generates a **falling-edge interrupt** to the GST MCU. The GST reports the corresponding digital X, Y values (from the preceding scan line) along with the trigger state. The trigger pulse width should be at least one horizontal scan time (~64 us at 31.5 kHz) to ensure reliable detection.

## Reading Pen Data

### GST MCU Analog Input Registers

Pen data is accessed through the GST MCU's I/O register space at `$FFFC00` through `$FFFC20`. The GST MCU automatically performs ADC conversions on all connected input channels, and the CPU reads the results from fixed register addresses.

### Register Map

```

    ┌──────┬──────────────┬─────────┬──────────────────────────────────────────┐
    │ Addr │ Register    │ Width   │ Description                              │
    ├──────┼──────────────┼─────────┼──────────────────────────────────────────┤
    │ $00  │ JS0_X        │ 8-bit   │ Joystick 0 X-axis (Port 0)              │
    │ $02  │ JS0_Y        │ 8-bit   │ Joystick 0 Y-axis (Port 0)              │
    │ $04  │ JS0_Z        │ 8-bit   │ Joystick 0 auxiliary (Port 0)           │
    │ $06  │ JS0_R        │ 8-bit   │ Joystick 0 paddle (Port 0)              │
    │ $08  │ JS1_X        │ 8-bit   │ Joystick 1 X-axis (Port 1)              │
    │ $0A  │ JS1_Y        │ 8-bit   │ Joystick 1 Y-axis (Port 1)              │
    │ $0C  │ JS1_Z        │ 8-bit   │ Joystick 1 auxiliary (Port 1)           │
    │ $0E  │ JS1_R        │ 8-bit   │ Joystick 1 paddle (Port 1)              │
    │ $10  │ PEN_TRIG     │ 8-bit   │ Optical pen trigger (bit 7 = state)     │
    │ $12  │ PEN_X        │ 8-bit   │ Optical pen X-axis (from Port 2)        │
    │ $14  │ PEN_Y        │ 8-bit   │ Optical pen Y-axis (from Port 2)        │
    │ $16  │ PEN2_TRIG    │ 8-bit   │ Port 3 pen trigger (Mega STE only)      │
    │ $18  │ PEN2_X       │ 8-bit   │ Port 3 pen X-axis (Mega STE only)       │
    │ $1A  │ PEN2_Y       │ 8-bit   │ Port 3 pen Y-axis (Mega STE only)       │
    │ $1C  │ PEN2_BTN     │ 8-bit   │ Port 3 pen button state                  │
    │ $1E  │ PEN2_R       │ 8-bit   │ Port 3 paddle R-axis                     │
    │ $20  │ PEN2_Z       │ 8-bit   │ Port 3 paddle Z-axis                     │
    └──────┴──────────────┴─────────┴──────────────────────────────────────────┘

    Memory-mapped base address: $FFFC00

    Legend:
      8-bit  = 1 byte read/write
      Bit 7  = Trigger state: 1 = released (no press), 0 = pressed
      Bit 0-6 = Paddle pot values (RC timing based, 0-127)
```

### Register Details

**`$FFFC10` – PEN\_TRIG (Pen Trigger)**

This register reports the pen trigger button state. Bit 7 is set to `1` when the trigger is released (normal) and `0` when pressed. Bits 0-6 may contain paddle potentiometer values if paddles are connected to Port 2 pins 1-3.

```
    PEN_TRIG byte layout:

    Bit  7   6   5   4   3   2   1   0
    ┌───┬───┬───┬───┬───┬───┬───┬───┐
    │TRG│  │  │   │   │   │   │   │
    │   │  │  │       (paddle R values)    │
    └───┴───┴───┴───┴───┴───┴───┴───┘

    Bit 7 = 1 : Trigger released (open, pulled high)
    Bit 7 = 0 : Trigger pressed (closed, pulled low)
```

**`$FFFC12` – PEN\_X (Pen Horizontal)**

8-bit ADC value for the horizontal axis. Range: 0-255.

- `0` = pen at leftmost screen position (0V input)
- `255` = pen at rightmost screen position (+5V input)
- Values scale linearly with the voltage at Pin 1

**`$FFFC14` – PEN\_Y (Pen Vertical)**

8-bit ADC value for the vertical axis. Range: 0-255.

- `0` = pen at top screen position (0V input)
- `255` = pen at bottom screen position (+5V input)
- Values scale linearly with the voltage at Pin 2

### Reading Sequence

The standard way to read pen data is to poll the three registers repeatedly:

```
    Polling loop (pseudocode):

    loop:
        x := READ.L $FFFC12    ; Read X axis (8-bit ADC value)
        y := READ.L $FFFC14    ; Read Y axis (8-bit ADC value)
        trg := READ.B $FFFC10  ; Read trigger state

        if (trg & $80) == 0:
            ; Trigger pressed — report click at (x, y)
            handle_pen_click(x, y)

        ; X and Y are continuously updated as the pen moves
        handle_pen_move(x, y)

        wait for horizontal retrace (or short delay)
    goto loop
```

### ADC Resolution and Characteristics

| Parameter | Value |
|---|---|
| **Resolution** | 8 bits (256 levels) |
| **Input voltage range** | 0V to +5V |
| **ADC type** | SAR (Successive Approximation Register) |
| **Conversion time** | ~10 us per channel |
| **Sampling** | GST MCU auto-interleaves all channels |
| **Linearity** | Approximate (not specified to ±N LSB) |
| **Update rate** | Limited by GST MCU ADC mux cycle (~100+Hz all channels combined) |

The GST MCU automatically cycles through all analog inputs (joystick ports and pen ports) in sequence. The CPU reads the most recently converted value from each register. No explicit ADC control is needed — the GST handles the scanning internally.

### Polling vs. Interrupt-Based Reading

The GST MCU can generate an interrupt when the pen trigger fires (falling edge on Pin 3). Two usage modes are available:

1. **Polling** (most common): Read all three registers in a simple loop. Simplest approach, works with any OS.
2. **Interrupt-driven**: Enable the pen interrupt via the GST interrupt mask registers. The GST fires an NMI/interrupt when the trigger goes low. The ISR reads PEN\_TRIG, PEN\_X, and PEN\_Y.

Polling is simpler and sufficient for most applications at the ~100 Hz scan rate the GST provides.

## Comparison to Joystick Port Analog Inputs

The pen port shares the same **physical connector type** (DB9 DE-9) and **analog input architecture** as the Atari ST joystick/mouse ports. Here's a comparison:

### Physical Comparison

| Feature | Joystick/Mouse Port | Pen Port (Port 2) |
|---|---|---|
| **Connector** | DB9 DE-9 female | DB9 DE-9 female (same) |
| **Physical label** | "JOYSTICK" or "MOUSE" | "PORT 2" (STe) / "PORT 2/3" (Mega STE) |
| **Port designation** | Port 0 (mouse), Port 1 (joystick) | Port 2 (analog expansion) |
| **Signal pins used** | Digital (buttons, X/Y paddle) | Analog (pen axes) + digital (trigger) |

### Signal Architecture Comparison

```
    ┌─────────────────────────────────────────────────────────────────────────────────┐
    │                    Analog Input Architecture Comparison                          │
    │                                                                                  │
    │   ┌──────────────────────┐                    ┌─────────────────────────┐       │
    │   │   Joystick Port      │                    │   Pen Port (Port 2)     │       │
    │   │   (Port 0/1)         │                    │                         │       │
    │   ├──────────────────────┤                    ├─────────────────────────┤       │
    │   │   Pin 1: Paddle R    │    RC timing       │   Pin 1: Pen X (analog)│       │
    │   │   → pot discharge    │    (timing based)  │   → voltage (ADC)       │       │
    │   │                      │                    │                         │       │
    │   │   Pin 2: Paddle X    │    RC timing       │   Pin 2: Pen Y (analog)│       │
    │   │   → pot discharge    │    (timing based)  │   → voltage (ADC)       │       │
    │   │                      │                    │                         │       │
    │   │   Pin 3: Paddle Y    │    RC timing       │   Pin 3: Pen trigger    │       │
    │   │   → pot discharge    │    (timing based)  │   → digital (int'rg)    │       │
    │   │                      │                    │                         │       │
    │   │   Pin 4: Paddle/Z    │    RC timing       │   Pin 4-7: NC           │       │
    │   │   → pot discharge    │    (timing based)  │                         │       │
    │   │                      │                    │                         │       │
    │   │   Buttons: Digital   │    GPIO input      │   V+: +5V supply        │       │
    │   │   input (GPIO)       │                    │   GND: Ground           │       │
    │   └──────────────────────┘                    └─────────────────────────┘       │
    │                                                                                  │
    │   Key difference: Joystick ports use RC TIMING for paddle inputs,               │
    │   while pen ports use VOLTAGE (ADC). The pen's analog signals are                │
    │   continuously-proportional voltages, NOT timing-based discharge.                │
    └─────────────────────────────────────────────────────────────────────────────────┘
```

### Analog Measurement Method Difference

| Aspect | Joystick Paddle (RC timing) | Pen Port (voltage/ADC) |
|---|---|---|
| **Measurement principle** | Time to discharge a capacitor through a pot | Voltage level read by SAR ADC |
| **Output** | Digital count (time-based, 0-127) | 8-bit ADC value (voltage-based, 0-255) |
| **GST MCU method** | Charges capacitor, measures discharge via timer/counter | Direct voltage-to-digital conversion via SAR ADC |
| **Resolution** | 7 bits (pot position: 0-127) | 8 bits (voltage: 0-255) |
| **Update speed** | Slower (timing measurement, ~1-2 ms) | Faster (~10 us per conversion) |
| **Noise sensitivity** | Less sensitive to noise | More sensitive to voltage noise |

### Why the Pen Needs a Different System

The RC-timing method used for joysticks measures the **time** it takes for a capacitor to discharge through a potentiometer. This works well for discrete pot inputs but cannot capture the continuous voltage levels needed by the optical pen, which outputs **analog voltages** proportional to detected screen flash positions. The STe's GST MCU included a dedicated SAR ADC specifically to handle these continuous voltage inputs from the pen.

## Software API for Pen Input

### BIOS Calls

Atari BIOS provides several vector calls for pen support. These are the standard entry points available through the BIOS jump table:

| BIOS Vector | Name | Function |
|---|---|---|
| **`GEMSV` ($6E2C**) | GemSV | GEM Screen Vector — includes pen support |
| **`VIA` ($E600**) | Via | GEM VDI Initialization — includes pen driver access |
| **`MESAG` ($EA28**) | MesaG | GEM Message Handler — includes pen message types |
| **`VSV` ($6E4A**) | VSV | VDI Screen Vector |

#### GEM Vector for Pen Input

In GEM (Graphics Environment Manager), the pen is reported through the same **`CUPEN`** message type (`$A8A0`) used in AES/DOS. When the pen trigger is pressed, the AES sends a `CUPEN` message containing the X and Y coordinates.

```
    AES/C-DOS Message format for pen events:

    ┌──────────────┬──────────────┬──────────────┬──────────────┬──────────────┐
    │ msgid  ($A8A0) │ x          │ y            │ button state │ pen button   │
    ├──────────────┼──────────────┼──────────────┼──────────────┼──────────────┤
    │ 16-bit       │ 16-bit       │ 16-bit       │ 16-bit       │ 16-bit       │
    │ 0xA8A0       │ X pixel pos  │ Y pixel pos  │ Button info  │ Pen button   │
    └──────────────┴──────────────┴──────────────┴──────────────┴──────────────┘

    Button info:  bit 0 = left/mouse button, bit 1 = right/middle button
    Pen button state: 0 = pressed, non-zero = released

    Note: Pen X and Y coordinates come as 16-bit screen positions.
    The OST/GST MCU provides 8-bit ADC values that must be scaled
    to the current screen resolution by the VDI driver
```

### GEM / TOS Vector Table (Pen-Specific)

The STe TOS (or MultiTOS) includes a **pen driver** that hooks into the standard BIOS vectors. The driver is located at vector `$01B1` (pen interrupt handler) in newer TOS versions.

```
    ┌───────────┬──────────────────────────────┬─────────────────────────────────┐
    │ Vector #  │ Name                       │ Description                     │
    ├───────────┼──────────────────────────────┼─────────────────────────────────┤
    │ $01B1     │ PEN_ISR                    │ Pen interrupt ISR in BIOS       │
    │           │                              │ Reads GST pen registers,        │
    │           │                              │ posts coordinates to AES        │
    ├───────────┼──────────────────────────────┼─────────────────────────────────┤
    │ $01B2     │ PEN_INIT                   │ Pen driver initialization       │
    │           │                              │ Configures GST pen ADC & IMR    │
    ├───────────┼──────────────────────────────┼─────────────────────────────────┤
    │ $01B3     │ PEN_VDI_DRVR               │ VDI pen driver entry point      │
    │           │                              │ Used by VDI for pen-aware       │
    │           │                              │ drawing operations              │
    ├───────────┼──────────────────────────────┼─────────────────────────────────┤
    │ $01B4     │ PEN_GET_POS                │ Get current pen position        │
    │           │                              │ Reads PEN_X, PEN_Y directly    │
    ├───────────┼──────────────────────────────┼─────────────────────────────────┤
    │ $01B5     │ PEN_GET_STATE              │ Get pen trigger state           │
    │           │                              │ Reads PEN_TRIG directly         │
    └───────────┴──────────────────────────────┴─────────────────────────────────┘
```

### GST MCU Pen Configuration Registers

The GST MCU also has control registers to configure the pen interrupt behavior:

| Register | Address | Function |
|---|---|---|
| **Pen IMR** (Interrupt Mask Register) | `$FFFA21` | Mask/unmask pen interrupt |
| **Pen ISR** (Interrupt Service Register) | `$FFFA31` | Clear pending pen interrupt |
| **GST IMR** (General GST IMR) | `$FFFA24` | GST MCU interrupt mask (includes pen) |

To enable the pen interrupt driver:
1. Write to GST IMR (`$FFFA24`) to unmask the pen IRQ bit
2. Set vector `$01B1` to the pen ISR
3. The GST MCU fires an NMI on trigger fall edge

### DOS / Extended BIOS Calls

Atari DOS 2.06 and MultiTOS 1.04+ expose additional pen-related BIOS functions:

```
    Extended BIOS Pen Functions (DOS 2.06 / MultiTOS 1.04+):

    ┌──────────┬───────────┬───────────────────────────────────┐
    │ Function │ Entry     │ Description                       │
    ├──────────┼───────────┼───────────────────────────────────┤
    │ $10      │ 0xAAAA    │ PEN_OPEN    : Open pen subsystem  │
    │ $10      │ 0xAAAA+1  │ PEN_GETPOS  : Get raw pen X,Y     │
    │ $10      │ 0xAAAA+2  │ PEN_GETSTA  : Get pen button state│
    │ $10      │ 0xAAAA+3  │ PEN_RAWPOS  : Get unscaled pen pos│
    └──────────┴───────────┴───────────────────────────────────┘
```

### Example: Polling Pen Data in C

```c
/* Poll pen data — works on STe/Mega STE with any TOS version */

#define GEM base  $FFFC00

unsigned char pen_x;
unsigned char pen_y;
unsigned char pen_trig;

void poll pen(void) {
    pen_x = *(volatile unsigned char *)($FFFC12);
    pen_y = *(volatile unsigned char *)($FFFC14);
    pen_trig = *(volatile unsigned char *)($FFFC10);
}

/* Check for pen click */
if ((pen_trig & $80) == 0) {
    /* Pen trigger is pressed */
    handle_click(pen_x, pen_y);
}
```

## Known Pen Accessories

### Original Atari Pen

| Detail | Value |
|---|---|
| **Name** | Atari Optical Pen (ATARI Pen) |
| **Manufacturer** | Atari, Inc. / Various OEMs |
| **Part number** | Varies by region (often listed as "Atari Pen ST") |
| **Compatible models** | STe, Mega STE |
| **Connectors** | DB9 cable (matches Port 2 pinout) |
| **Features** | 2-axis optical pointing, trigger button, +5V powered |
| **Status** | Discontinued; sold with early STe units in some regions |

The original Atari optical pen was a proprietary design with a CRT phosphor flash detection system. It was only compatible with CRT monitors, due to the pen's need for visible refresh flashes.

### Third-Party and Compatible Pens

| Pen Name | Manufacturer | Notes |
|---|---|---|
| **WICO STe Pen** | WICO | Third-party alternative to Atari pen |
| **LogiComp Optical Pen** | LogiComp | Compatible optical pen for STe |
| **Generic "STe pen"** | Various | No-name clones, functionally equivalent |
| **DIY optical pen** | Hobbyist community | Designs exist using 555 timers and photodiodes |

### Pen Limitations

| Limitation | Detail |
|---|---|
| **CRT only** | Does not work with LCDFLAT panels (no phosphor flashes) |
| **Screen resolution** | Limited to ~128-255 discrete positions per axis (8-bit ADC) |
| **No pen pressure** | Binary trigger only, no pressure sensitivity |
| **Latency** | ~10-50 ms latency due to CRT refresh timing |
| **Lighting sensitivity** | Ambient light can interfere with detection |
| **One pen at a time** | Only one pen can be used at a time on Port 2 |

### Pen Cable Pin-Out (DIY)

If building a DIY pen, wire the pen cable as:

```
    Pen cable → STe Port 2 DB9:

    Pen wire  →  DB9 pin
    ────────────────────
    sensor X  →  Pin 1  (PEN_H)
    sensor Y  →  Pin 2  (PEN_V)
    trigger   →  Pin 3  (PEN_TRIG)
    shield/GND → Pin 8  (GND)
    V+ supply → Pin 9  (+5V)
    Pins 4-7  →  Not wired
```

## Emulation Notes

Emulating the Atari STe optical pen port in STeem Browser and other emulators requires faithfully reproducing both the GST MCU analog input behavior and the CRT flash detection model.

### GST MCU Emulation

The GST MCU must be emulated at the register level to support pen input:

```
    ┌────────────────────────────────────────────────────────────────────────────────┐
    │                    GST MCU Emulation for Pen Port                              │
    │                                                                                │
    │   ┌────────────────────────────┐     ┌────────────────────────────────────┐    │
    │   │      Emulator (STe Browser) │     │      Guest TOS/BIOs               │    │
    │   ├────────────────────────────┤     ├────────────────────────────────────┤    │
    │   │                                                            │            │
    │   │   GST MCU Registers:                Emulated GST MCU       │            │
    │   │   $FFFC10 PEN_TRIG   ← user input    SAR ADC              │            │
    │   │   $FFFC12 PEN_X      ← user input    ┌─────┐              │            │
    │   │   $FFFC14 PEN_Y      ← user input    │ ADC │              │            │
    │   │                                     └──┬──┘              │            │
    │   │                                        │ Converted        │            │
    │   │                                     ┌──▼──┐              │            │
    │   │                                     │ GST  │              │            │
    │   │                                     │ MCU  │              │            │
    │   │                                     └──────┘              │            │
    │   │                                                           │            │
    │   │                           CPU reads registers           │            │
    │   │                           at $FFFCxx via direct bus     │            │
    │   └────────────────────────────────────┘     └────────────────────────────────────┘    │
    └────────────────────────────────────────────────────────────────────────────────┘
```

### Key Emulation Requirements

1. **Register mirroring:** The emulator must expose the pen registers at physical addresses `$FFFC10`, `$FFFC12`, and `$FFFC14`. Reads from these addresses must return the current emulated state.

2. **Mouse-to-pen mapping:** Real CRT flash detection cannot be emulated on modern displays. A common approach is to map the host mouse to the pen port:
   - Host mouse X/Y → Pen X/Y registers
   - Host button events → Pen trigger (Pin 3)
   - This provides the same user experience without requiring a real pen

3. **ADC sampling simulation:** The GST ADC in the real hardware samples pen voltages at ~100 Hz (limited by GST MCU mux cycle). The emulator should either:
   - Update pen register values on every CPU read (lazy update)
   - Simulate periodic GST ADC scan at ~100 Hz in the emulator's timing loop

4. **Trigger interrupt emulation:** When the host mouse button triggers, the emulator must:
   - Set PEN\_TRIG bit 7 to `0` (trigger pressed)
   - Fire a GST pen interrupt to the VM's CPU (equivalent to NMI on GST IMR bit)
   - Invoke vector `$01B1` in the guest BIOS (pen ISR)

5. **8-bit resolution:** Pen X and Y must be stored as 8-bit values (0-255), then scaled by the VDI driver to screen resolution. The emulator should keep raw 8-bit values in the GST registers.

### emu: Browser-Specific Notes

In STeem Browser (browser-based STe emulator):

- Pen input can be mapped to the browser's mouse device via JavaScript `mousemove` and `mousedown` events
- The host mouse position should be scaled to the STe screen resolution (320x200, 640x480, or 1280x1024) and mapped to 8-bit ADC range
- The browser's pointer lock API (`requestPointerLock()`) can provide unrestricted cursor movement
- **Caveat:** True CRT flash detection emulation (as the real pen requires) cannot work in a browser because modern displays do not produce phosphor flashes. Mouse-based mapping is the practical alternative.
- Mega STE Port 3 pen support should map to a second mouse / touch input if available

### Common Emulation Pitfalls

| Pitfall | Solution |
|---|---|
| Pen X/Y values don't update | Ensure guest reads `$FFFC10/$FFFC12/​$FFFC14` from the GST MCU address space, not the standard joystick registers |
| Trigger interrupt never fires | Check that GST IMR (`$FFFA24`) pen bit is unmasked in the emulated register |
| Pen coordinates scaled wrong | Guest VDI driver maps 0-255 ADC values to screen resolution. Ensure emulator passes raw ADC values |
| Pen works in Port 2 but not Port 3 | Port 3 registers are at `$FFFC16-$FFFC1E` (Mega STE only); verify emulation scope |
| No pen interrupt on guest boot | TOS must initialize pen driver at startup. Some older TOS versions only enable pen after GEM launches |

### Verification Checklist

When testing pen support in emulation:

1. Read `$FFFC10` → Should respond to mouse button (bit 7 toggles)
2. Read `$FFFC12` → Should reflect horizontal mouse position (scaled to 0-255)
3. Read `$FFFC14` → Should reflect vertical mouse position (scaled to 0-255)
4. Verify GST pen interrupt fires on trigger event
5. Confirm pen driver initializes in guest TOS (check vector `$01B1`)
6. Test with a pen-aware GEM application (e.g., AtariDraw pen strokes)
