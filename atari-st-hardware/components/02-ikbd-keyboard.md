# IKBD Keyboard Controller (HD6301)

> The Hitachi HD6301V1 is a microcontroller that handles keyboard scanning, mouse scanning, and generating events for the Atari ST. Part of the IKBD (Intelligent Keyboard) subsystem.

## Overview

| Parameter | Value |
|-----------|-------|
| Manufacturer | Hitachi (also manufactured as HD6301) |
| Part | HD6301V1 |
| Package | DIP40 |
| Function | Intelligent keyboard/controller microcontroller |
| Clock | 4 MHz (internal) |
| I/O | 8-bit parallel port, serial I/O |
| Timer | 16-bit timer |
| RAM | 48 bytes on-chip |
| ROM | 1 KB on-chip (mask-programmed) |

## Position on Atari ST

The HD6301 is located **inside the keyboard case** (or inside the console in built-in keyboard models), connected to the main ST motherboard via a flat cable. It communicates with the ST's MC6850 ACIA chip through a dedicated serial link.

## Keyboard Scanning

### Keyboard Matrix

The Atari ST keyboard uses a matrix of rows and columns:

| Row | Column | Function |
|-----|--------|----------|
| R0-R7 | C0-C6 | Standard key matrix scanning |

- **Rows**: 8 row lines
- **Columns**: 7 column lines
- **Total keys**: 94 (on the standard ST keyboard)
- **Scan method**: Row-by-row scanning at high frequency
- **Scan cycle**: ~300 Hz keyboard scan rate

### Key Encoding

Each key press generates a scan code:

| Event | Code Format | Description |
|-------|-------------|-------------|
| Key press (make) | $80 + n | High bit set indicates key press |
| Key release (break) | n | Low bit = raw key scancode |
| Extended key | High bit + special code | Function keys, cursor keys, etc. |
| Special events | $81-$8F | Mouse events, joystick events, etc. |

## Mouse Port Scanning

The IKBD scans the mouse port for motion events:

| Axis | Signal Type | Scan Method |
|------|-------------|-------------|
| X axis | Digital pulses | Counts quadrature encoder pulses |
| Y axis | Digital pulses | Counts quadrature encoder pulses |
| Button 1 | Digital | Polls fire pin |
| Button 2 | Digital | Polls fire pin |

Mouse events are generated as:

| Event | Code | Description |
|-------|------|-------------|
| X+ | $81 | Mouse moved right |
| X- | $82 | Mouse moved left |
| Y+ | $83 | Mouse moved down |
| Y- | $84 | Mouse moved up |
| B1 press | $85 | Button 1 pressed |
| B1 release | $86 | Button 1 released |
| B2 press | $87 | Button 2 pressed |
| B2 release | $88 | Button 2 released |
| Joystick 0 | $89-$8F | Joystick events |

## Serial Link to ACIA

The IKBD communicates with the ST's MC6850 ACIA via a dedicated serial port:

| Parameter | Value |
|-----------|-------|
| Baud rate | 7,812.5 bps (fixed) |
| Data format | 7 data bits, 1 stop bit, even parity |
| Direction | Bidirectional (IKBD and ACIA can both transmit) |
| Protocol | Simple serial packet protocol |

### Packet Format (IKBD -> ACIA)

| Byte | Name | Description |
|------|------|-------------|
| 1 | Header | $FF (0x80 repeated) |
| 2 | Length | Number of data bytes (1-4) |
| 3-n | Data | Event data or command |
| n+1 | Checksum | XOR of data bytes |

### Commands (ST -> IKBD)

The ST can send commands to the IKBD:

| Command | Data | Description |
|---------|------|-------------|
| Enable interrupt | 0x01 | Re-enable IKBD interrupts |
| Disable interrupt | 0x00 | Disable IKBD interrupts |
| Joystick enable | 0x10+port | Enable joystick polling |
| Joystick disable | 0x18+port | Disable joystick polling |
| Joystick fire | 0x20 | Read joystick fire button |
| Clear mouse events | 0x30 | Flush mouse data |
| Clear joystick data | 0x31 | Flush joystick data |
| Mouse enable | 0x50 | Enable mouse events |
| Mouse disable | 0x51 | Disable mouse events |
| Mouse reset | 0x52 | Reset mouse state |

## Serial Link from ACIA -> HD6301

The HD6301 receives the ACIA data via its serial input and processes commands:

1. HD6301 serial input connected to MC6850 ACIA TXD
2. When ACIA receives command from ST, HD6301 acts on it
3. Commands control keyboard/mouse event reporting

## IKBD Event Queue

The ST can read the serial event queue from the ACIA receiver data register:

- Events are queued in FIFO order
- Queue depth: typically unlimited (limited only by ACIA buffer)
- Each event is a single byte sent via serial

## Joystick Port Handling

### ST Joystick Ports

The IKBD scans two joystick ports:

| Port | Usage |
|------|-------|
| Port 0 | Mouse or joystick 0 |
| Port 1 | Joystick 1 |

### Digital Input

For each joystick, the IKBD polls:

| Signal | Type | Active |
|--------|------|--------|
| X+ | TTL input | Ground |
| X- | TTL input | Ground |
| Y+ | TTL input | Ground |
| Y- | TTL input | Ground |
| Fire | TTL input | Ground |

### Analog Input (STe+)

On the STe, the GST MCU adds ADC capability:

| Signal | Type | Resolution |
|--------|------|------------|
| X paddle | Analog | 8-bit SAR ADC (0-255) |
| Y paddle | Analog | 8-bit SAR ADC (0-255) |
| Z paddle | Analog | 8-bit SAR ADC (0-255) |
| R paddle | Analog | 8-bit SAR ADC (0-255) |

## Timer

The HD6301 has an internal timer used for:

- **Video timing**: Counting frames for keyboard scan sync
- **Polling interval**: Controlling how often joystick/mouse ports are scanned
- **Baud rate generation**: Clock for serial communication

## Emulator Notes

- On emulators, the IKBD can be simulated as a separate process or integrated into the CPU cycle
- The keyboard matrix is fully scanned ~300 times per second
- Events are sent to the ACIA serial buffer which the ST reads
- Mouse events use a delta-based protocol (not absolute position)
- Joystick scan uses RC timing for analog position in STe
- The IKBD firmware is masked ROM (1 KB) - cannot be upgraded without new ROM chip

## References

- [Atari ST Internals, ch. 1.1 - IKBD Keyboard](https://www.atarimania.com/documents/Atari-ST-Internals.pdf)
- [MC68901 MFP (PDF)](https://nfggames.com/X68000/Development/datasheets/MC68901%20-%20MFP.pdf)
- [HD6301 datasheet](https://www.tapr.org/tprpubs/TAPR_MicroNewsletter/1987/TAPR%20Micro%20Newsletter%201987-05.pdf)
- [Atari ST Programmer's Reference Guide](https://info-coach.fr/atari/hardware/interfaces.php)
