# DB25 RS232 Port

> The serial modem port on the Atari ST. Supports 50-19200 baud with hardware handshake.

## Connector

- **DB25P** (D-sub 25-pin, male plug)

## Pinout

| Pin | Function | Direction | Signal Type |
|-----|- --|--|-----|------|
| 1 | Protective ground | - | Ground |
| 2 | Tx (transmit) | PSG → external | TTL RS232 |
| 3 | Rx (receive) | external → PSG | TTL RS232 |
| 4 | RTS (ready to send) | PSG → external | TTL |
| 5 | CTS (clear to send) | external → PSG | TTL |
| 6 | n/c | - | |
| 7 | Signal ground | - | Ground |
| 8 | DCD (data carrier detect) | external → PSG | TTL |
| 9-19 | n/c | - | |
| 20 | DTR (data terminal ready) | PSG → external | TTL |
| 21 | n/c | - | |
| 22 | RI (ring indicator) | external → PSG | TTL |
| 23-25 | n/c | - | |

## Signal Levels

RS232 voltage levels:
- Logic 0 (mark): +3V to +12V
- Logic 1 (space): -3V to -12V

## Control

### Output (RTS/DTR) - from PSG I/O Port A

| Pin | Signal | PSG Register | Bit |
|-----|-|----|-- |
| 4 | RTS | $FF880E (Port A) | 3 |
| 20 | DTR | $FF880E (Port A) | 4 |

### Input (CTS/DCD/RI) - to MFP

| Pin | Signal | MFP GPI Register | GPI Bit |
|-----|-|-----|---|
| 5 | CTS | $FFFA01 (GPIOR_A) | bit 2 |
| 8 | DCD | $FFFA01 (GPIOR_A) | bit 1 |
| 22 | RI | $FFFA01 (GPIOR_A) | bit 6 |

## Baud Rate Generation

The MFP Timer D generates the baud rate:

```
Baud Rate = f_clock / (16 × Timer D count)
f_clock = 8 MHz (system clock)
```

| Baud | Timer D value |
|------|------|
| 50 | 10000 |
| 300 | 166 |
| 1200 | 41 |
| 2400 | 20 |
| 4800 | 11 |
| 9600 | 5 |
| 19200 | 2 |
| 38400 | 1 |

## Handshake Control

| Handshake | ST Output | ST Input |
|-- | --- | --- |
| RTS/CTS | PSG Port A bit3 (RTS) | MFP GPI2 (CTS) |
| DTR/DCD | PSG Port A bit4 (DTR) | MFP GPI1 (DCD) |
| - | - | MFP GPI6 (RI) |

## Data Transfer

- **Transmit**: PSG Port A bit 2 → UART TX → ACIA → ACIA data register ($FFFC02) → PSG Port A
- **Receive**: ACIA data register ($FFFC02) ← ACIA RX ← PSG Port A bit 2

## Data Format

- 8 data bits, 1 stop bit, no parity (default)
- Baud rate from 50 to 19,200 baud

## References

- [Atari ST Internals, ch. 1.3 (PDF)](https://www.atarimania.com/documents/Atari-ST-Internals.pdf)
- [RS232 standard specifications](https://www.easysw.com/~mike/serial/serial.html)
- [Atari ST Programmer's Reference Guide (PDF)](https://info-coach.fr/atari/ressources/doc/st_prog_guide_1.htm)
