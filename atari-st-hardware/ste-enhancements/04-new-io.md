# New I/O Ports on STe / Mega STE

> The STe adds several new I/O features: enhanced joystick/paddle/pen support, stereo audio output, and additional I/O ports.

## Enhanced Joystick/Paddle/Pen Ports

### New Port Configuration

The STe adds 2 new 9-pin ports dedicated to analog input:

| Port | Function | Type |
|--| ---- |--- |
| Port 0 (mouse) | Mouse or 4-axis joystick + 2 paddles | Digital + analog |
| Port 1 (joystick) | Standard joystick or 2-axis + 2 paddles | Digital + analog |
| **Port 2** (new) | 4 analog inputs (2 paddles + 2 pen) | Analog |
| **Port 3** (new) | 4 analog inputs (2 paddles + 2 pen) | Analog |

### Paddle Input

| Paddle | Port 0 | Port 1 | Port 2 | Port 3 |
|------|------|--|----|---|
| X (horizontal) | Pin 1 | Pin 1 | Pin 2 | Pin 2 |
| Y (vertical) | Pin 2 | Pin 2 | Pin 3 | Pin 3 |
| Z (auxiliary) | Pin 3 | Pin 4 | Pin 4 | Pin 4 |
| R (auxiliary) | Pin 4 | Pin 3 | Pin 1 | Pin 1 |

The paddle inputs use RC timing: the GST MCU charges a capacitor through the paddle potentiometer and measures the discharge time.

### Optical Pen Support

The STe GST MCU includes support for an optical pen:

| Signal | Port 2 |
|--------|------|
| Pen X (axis) | Pin 2 (analog) |
| Pen Y (axis) | Pin 3 (analog) |
| Pen trigger | Pin 4 (digital) |

The GST MCU includes an internal SAR ADC (Successive Approximation Register Analog-to-Digital Converter) that converts the analog signals to 8-bit values.

## GST MCU Analog Input Registers

| Register | Address | Description |
|--|--|----|
| Joystick 0 X | $FFFC00 | Analog input axis 0 |
| Joystick 0 Y | $FFFC02 | Analog input axis 1 |
| Joystick 0 Z | $FFFC04 | Analog input axis 2 |
| Joystick 0 R | $FFFC06 | Analog input axis 3 |
| Joystick 1 X | $FFFC08 | Analog input axis 4 |
| Joystick 1 Y | $FFFC0A | Analog input axis 5 |
| Joystick 1 Z | $FFFC0C | Analog input axis 6 |
| Joystick 1 R | $FFFC0E | Analog input axis 7 |
| Pen trigger | $FFFC10 | Optical pen trigger |

### ADC Resolution

The GST MCU ADC provides **8-bit resolution** (0-255):
- 0 = minimum voltage (0V)
- 255 = maximum voltage (+5V)

### ADC Timing

- Conversion time: ~10 us per channel (8-bit SAR ADC)
- The STe GST MCU automatically interleaves ADC conversions
- The CPU can read the ADC results from the I/O registers at any time

## New I/O Port Pinouts

### Port 2 (Analog 1)

| Pin | Analog Input | Digital |
|-----|------|--- |
| 1 | R (aux) | - |
| 2 | X | Pen H sync |
| 3 | Y | Pen V sync |
| 4 | Z | Pen trigger |
| 5+8 | +5V/GND | - |

### Port 3 (Analog 2)

| Pin | Analog Input | Digital |
|-----|------|---|
| 1 | R (aux) | - |
| 2 | X | Pen H sync |
| 3 | Y | Pen V sync |
| 4 | Z | Pen trigger |
| 5+8 | +5V/GND | - |

## Audio Out (Dual RF Sockets)

The STe adds dedicated audio output sockets:

| Socket | Signal | Level | Source |
|--------|---|--|------|
| Audio 1 | Left channel (PCM) | 1V pk-pk | GST Shifter DAC |
| Audio 2 | Right channel (PCM) | 1V pk-pk | GST Shifter DAC |
| Audio 3 | PSG mono (YM2149) | 1V pk-pk | YM2149 |

### Audio Jack Pinout (Dual RF)

| Socket | Pin | Signal |
|------|--|------|
| Audio 1 (Center) | Left PCM output |
| Audio 1 (Shield) | Ground |
| Audio 2 (Center) | Right PCM output |
| Audio 2 (Shield) | Ground |

## Other STe I/O Enhancements

### MIDI Through on STe

The STe adds a third MIDI port:

| Port | Function |
|------|------|
| MIDI In | Standard MIDI input |
| MIDI Out | Standard MIDI output |
| MIDI Through | Opto-coupled loop from In to Out |

### IDE Support (Mega STE)

The Mega STE SH2 chip adds IDE hard disk support:

| Feature | ST | STe | Mega STE |
|------|--|--|-----|
| Floppy drive | 1-2 DD | 1-2 HD | 1-2 HD |
| Hard disk | DMA/SCSI | DMA/SCSI | IDE + SCSI |
| DMA port | DB19 (SCSI) | DB19 (SCSI) | DB19 (IDE/SCSI) |

## Memory Map Changes (STe)

The STe changes the hardware register map:

| Register Range | ST | STe |
|------|--|--|
| $FFC000 | Shifter palette | GST Shifter palette (512 colors) |
| $FFC200 | Shifter control | GST Shifter control |
| $FFE000 | MMU scroll | GST MCU scroll |
| $FFE400 | Reserved | Blitter (Mega STE) |
| $FFE600 | Reserved | STe audio |
| $FFE800 | Reserved | Audio mixer |
| $FFC600 | Reserved | Sample DMA |
| $FA0000 | Cartridge ROM | Cartridge ROM |

## References

- [Info-Coach - STE Hardware](https://info-coach.fr/atari/hardware/STE-HW.php)
- [Atari ST Internals (PDF)](https://www.atarimania.com/documents/Atari-ST-Internals.pdf)
- [Atari ST Bus Doc - GST MCU](http://info-coach.fr/atari/ressources/doc/Atari%20ST%20bus%20doc.pdf)
- [Atari STE FAQ](https://github.com/Number0000009/atari-wiki/blob/master/Atari%20STE%20FAQ%20compiled%20by%20The%20Paranoid%20Paradox.txt)
