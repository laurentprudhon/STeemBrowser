# DIN13 Monitor Port

> The main video/audio connector on the Atari ST. Supports RGB monitor, composite video, and mono audio.

## Connector Type

- **DIN13S** (13-pin DIN socket, female)
- Located on rear panel
- Used for video output (RGB + composite) and audio

## Pinout

| Pin | Function | Signal Type | Notes |
|-----|- --|--|------|
| 1 | Audio out | Analog 1V pk-pk, 10 kohm | From YM2149 / Shifter |
| 2 | Composite video | Analog 1V pk-pk, 75 ohm | RF or composite |
| 3 | General purpose | TTL | PSG I/O Port A |
| 4 | Monochrome detect | TTL active low, 1K pull-up | From Shifter/Glue |
| 5 | Audio in | Analog | Unused on ST |
| 6 | Green | Digital 5V TTL | Green DAC signal |
| 7 | Red | Digital 5V TTL | Red DAC signal |
| 8 | Ground | Ground | Shield |
| 9 | Horizontal sync | TTL 5V active low | HSync |
| 10 | Blue | Digital 5V TTL | Blue DAC signal |
| 11 | Monochrome | TTL | Monochrome mode signal |
| 12 | Vertical sync | TTL 5V active low | VSync |
| 13 | Ground | Ground | Reference |

## RGB Signals

### RGB DAC Voltage Levels

On the Atari ST, the RGB output is **digital TTL** (not analog voltage). Each color component is a binary signal:

| Color | Pin | Signal |
|-------|-----|--------|
| Red | 7 | R0-R2 (3-bit) |
| Green | 6 | G0-G2 (3-bit) |
| Blue | 10 | B0-B2 (3-bit) |

In reality the actual ST uses **4-bit colors** (16 colors). The DAC uses resistor networks to generate voltages:

```
Red: bits R0, R1, R2 on pin 7 → resistor ladder → RGB DAC → pin 7
Green: bits G0, G1, G2 on pin 6 → resistor ladder → RGB DAC → pin 6
Blue: bits B0, B1, B2 on pin 10 → resistor ladder → RGB DAC → pin 10
```

For the 16-color palette:
- Each color entry is 6 bits (2 bits per component for original ST)
- R = {R1, R0}, G = {G1, G0}, B = {B1, B0} → 4-bit index
- The DAC generates 4 voltage levels per color (0V, V/3, 2V/3, V)

### RGB Color Voltages (NTSC/ST)

| Color Index | Red | Green | Blue | DAC Voltage (approx) |
|-------------|-----|-------|------|---------------------|
| 0 (black) | 0.45V | 0.45V | 0.45V | all off |
| 1 | 1.0V | 0.45V | 0.45V | red on |
| 2 | 0.45V | 1.0V | 0.45V | green on |
| 15 (white) | 1.0V | 1.0V | 1.0V | all on |

### RGB Pin Mapping on DIN13

The actual RGB DAC outputs go to the monitor via resistors on pin 6 (green), pin 7 (red), pin 10 (blue). Each pin drives a DAC resistor network:

```
Pin 6 (Green):  connects to green DAC via 10 kohm resistors
Pin 7 (Red):    connects to red DAC via 10 kohm resistors
Pin 10 (Blue):  connects to blue DAC via 10 kohm resistors
```

### Sync Signals

| Signal | Pin | Level | Active |
|--------|-----|-------|----------|
| HSync | 9 | TTL 5V | Active LOW |
| VSync | 12 | TTL 5V | Active LOW |
| DE (Dot Enable) | 30 (Shifter pin) | TTL | Active HIGH |
| BLANK | Glue pin | TTL | Active LOW |

### Horizontal and Vertical Sync Frequencies

| Mode | HSync Hz | VSync Hz | Pixels/line | Lines/frame |
|------|----------|----------|-------------|-------------|
| NTSC LORAM | 15,734 | 60.008 | 312 | 525 |
| NTSC HIRE | 25,175 | 60.008 | 407 | 525 |
| PAL LORAM | 15,625 | 50.093 | 312 | 625 |
| PAL HIRE | 24,983 | 50.093 | 407 | 625 |

### Synch Signal Levels

- HSync: 5V TTL, active low (0V during sync)
- VSync: 5V TTL, active low (0V during sync)
- Sync blanking: Sync pulses during the blanking period

## Audio Pin 1

| Parameter | Value |
|-- |-- |
| Amplitude | 1V pk-pk |
| Impedance | 10 kohm |
| Source | Shifter digital audio mixer / YM2149 analog output |

The audio comes from the Shifter's digital audio mixer (16 channels) mixed through the YM2149 PSG.

## Monochrome Detect

| Pin | Function | Value when color monitor | Value when mono monitor |
|-----|--|--|----|----|
| 4 | Mono detect | ~0V (low) | ~5V (high) |

The Atari monitors the state of pin 4 to detect if a monochrome monitor (SM124) is connected. If high, the ST sends higher-frequency sync signals.

## References

- [Atari ST Internals, ch. 1.3 - Shifter (PDF)](https://www.atarimania.com/documents/Atari-ST-Internals.pdf)
- [Atari ST Interfaces (DrCoolZic)](https://info-coach.fr/atari/hardware/interfaces.php)
- [Atari SM1224/SM124 monitors specs](https://info-coach.fr/atari/documents/general/Bitbook2.pdf)
