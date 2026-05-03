# DB9 Mouse/Joystick Port

> The mouse and joystick interface ports on the Atari ST.

## Connector

- **DB9P** (D-sub 9-pin, male plug)

## Port Assignment

| Port | Primary use |
|------|-----|
| Port 0 | Mouse (or joystick) |
| Port 1 | Joystick |

## Pinout

| Pin | Joystick Function | Mouse/Joystick 0 Function |
|-----|--|--|---|
| 1 | Up (YB axis) | XB/Up |
| 2 | Down (XA axis) | XA/Down |
| 3 | Left (YA axis) | YA/Left |
| 4 | Right (YB axis) | YB/Right |
| 5 | Reserved | n/c |
| 6 | Fire | Left button/Fire |
| 7 | +5V | +5V |
| 8 | GND | Ground |
| 9 | Joystick 1 Fire | Right button/Joystick 1 fire |

## Mouse Protocol

### Mouse Motion Reporting

The mouse reports motion as **events**:

| Event Type | Description |
|-- | --- |
| Delta X+ | Mouse moved right |
| Delta X- | Mouse moved left |
| Delta Y+ | Mouse moved down |
| Delta Y- | Mouse moved up |
| Button press | Button 1 or 2 pressed |
| Button release | Button 1 or 2 released |

### Resolution

- 200 events per inch (4 events/mm)
- Max tracking velocity: 10 inches/sec (250 mm/sec)
- Max pulse phase error: 50%

### Motion Reporting Modes

| Mode | Description |
|------|------|
| Relative | Report delta from last position |
| Absolute | Report absolute screen coordinates |
| Cursor key | Report as cursor key presses (movable per keystroke) |

### Mouse Data Format

Mouse data is transmitted serially from the IKBD via the HD6301 keyboard controller to the ACIA. Each mouse event generates a scan code.

## Joystick Protocol

### Joystick Input

Each joystick has:

| Axis | Signal | Active State |
|------|--------|------|
| X axis (horizontal) | Pin 1/2 (XA/YB) | Ground when pressed |
| Y axis (vertical) | Pin 3/4 (YA/YB) | Ground when pressed |
| Fire | Pin 6 | Ground when pressed |

### Analog Joysticks (STe+)

The STe added analog input capability:

| Signal | Pin | Type | Range |
|--------|-----|-|------|
| X paddle | Pin 1 | Analog 0-5V | 0V = min, 5V = max |
| Y paddle | Pin 2 | Analog 0-5V | 0V = min, 5V = max |
| Z paddle (axis 1) | Pin 3-4 | Analog 0-5V | 0V = min, 5V = max |
| R paddle (axis 2) | Pin 3-4 | Analog 0-5V | 0V = min, 5V = max |

The STe GST MCU converts the analog signals via an internal ADC (typically a SAR ADC).

## IKBD Mouse/Joystick Scan

The IKBD (HD6301) scans the mouse/joystick ports:

1. Applies voltage to X+ (pin 1) via internal resistor
2. Reads Y+ (pin 2) for X-axis position
3. Applies voltage to Y+ (pin 3) via internal resistor
4. Reads X+ (pin 1) for Y-axis position
5. Reads fire button state (pin 6)
6. Uses RC timing to measure joystick displacement

The IKBD generates events when the threshold is exceeded:

```
IKBD scan cycle:
1. Apply reference voltage to axis pins
2. Measure RC discharge time via internal counter
3. If change > threshold, generate event
4. Transmit events via serial link to ST ACIA
```

## References

- [Atari ST Internals, ch. 1.3 - Mouse/Joystick (PDF)](https://www.atarimania.com/documents/Atari-ST-Internals.pdf)
- [Atari ST Programmer's Reference Guide (PDF)](https://info-coach.fr/atari/ressources/doc/st_prog_guide_1.htm)
- [The Little Black Bit Book - Mouse/Joystick (PDF)](https://info-coach.fr/atari/documents/general/Bitbook2.pdf)
