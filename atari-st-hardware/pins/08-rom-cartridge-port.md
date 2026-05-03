# ROM Cartridge Port

> The 40-pin cartridge port for ROM expansion on the Atari ST.

## Connector

- **40S socket** (40-pin cartridge expansion slot on the top of the console)

## Pinout

| Pin | Function | Pin | Function |
|-----|-|---|------|
| 1 | Power +5V | 2 | Power +5V |
| 3 | Data 14 | 4 | Data 15 |
| 5 | Data 12 | 6 | Data 13 |
| 7 | Data 10 | 8 | Data 11 |
| 9 | Data 8 | 10 | Data 9 |
| 11 | Data 6 | 12 | Data 7 |
| 13 | Data 4 | 14 | Data 5 |
| 15 | Data 2 | 16 | Data 3 |
| 17 | Data 0 | 18 | Data 1 |
| 19 | Address 13 | 20 | Address 15 |
| 21 | Address 8 | 22 | Address 14 |
| 23 | Address 7 | 24 | Address 9 |
| 25 | Address 10 | 26 | Address 6 |
| 27 | Address 5 | 28 | Address 12 |
| 29 | Address 11 | 30 | Address 4 |
| 31 | ROM3 select | 32 | Address 3 |
| 33 | ROM4 select | 34 | Address 2 |
| 35 | Upper data strobe | 36 | Address 1 |
| 37 | Lower data strobe | 38 | Ground |
| 39 | Ground | 40 | Ground |

## Address Range

ROM cartridge occupies addresses:

| Start | End | Size |
|------|--|----|
| $FA0000 | $FBFFFF | 128 KB max |

The bus decoding uses:
- Address lines A0-A13 (14 address bits → 16 KB per ROM chip)
- ROM3 and ROM4 select lines for ROM bank selection
- Data lines D0-D15

## Memory Mapping

| Address | Content |
|-- | --- |
| $FA0000-$FBFFFF | Cartridge ROM (up to 128 KB) |
| $80000-$F9FFFF | System RAM |
| $FC0000-$FFFFFF | TOS ROM (lower ROM) + hardware registers |

## Bus Interface

The cartridge provides:
- Read-only access (no write line)
- 15 address lines (A0-A14)
- 16 data lines (D0-D15)
- Chip select via ROM3/ROM4 decoded from address lines

## References

- [Atari ST Internals, ch. 1.3 - Cartridge Port (PDF)](https://www.atarimania.com/documents/Atari-ST-Internals.pdf)
- [Atari ST Programmer's Reference Guide (PDF)](https://info-coach.fr/atari/ressources/doc/st_prog_guide_1.htm)
