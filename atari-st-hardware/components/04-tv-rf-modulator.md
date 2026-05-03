# TV RF Modulator (STM/FM variants)

> The TV RF modulator allows Atari ST computers to connect directly to a television for video output. Available on STM (520STM) and FM (1040STFM) variants only.

## Variants

| Model | RF Modulator | Type |
|-------|------ |-- |
| 520ST | None (external monitor only) |
| 520STM | Built-in in console |
| 520ST+ | None |
| 1040STF | None (built-in floppy) |
| 1040STFM | Built-in in console |
| Mega ST | No separate RF (built-in PSU handles it) |
| Mega STE | No separate RF |
| STacy | No RF modulator |

## Specifications

| Parameter | NTSC Value | PAL Value |
|------|-|---|-- |
| Video standard | NTSC-M | PAL-B/G, PAL-D/K |
| Video modulation | FM (VSB) | FM (VSB) |
| Video carrier frequency | 45.75 MHz (NTSC) | 47.75 MHz (PAL) |
| Audio modulation | FM | FM |
| Audio carrier offset | +4.5 MHz | +5.5 MHz |
| Output channel (NTSC) | Channel 3 or 4 |
| Output channel (PAL) | Channel 28 |
| Video amplitude | 1V pk-pk (75 ohm) |
| Audio amplitude | 0.5V pk-pk |
| Impedance | 75 ohm (video), 75K ohm (audio) |
| Output connector | F-type connector (screw-on) |

## NTSC vs PAL

The Atari ST RF modulator supports both NTSC and PAL standards. The mode is determined by the display controller (Shifter) chip:

| Standard | Line count | Frame rate | Pixel clock | Audio offset |
|------|--|----|--|----|
| NTSC | 525 (480 visible) | 60 Hz | 32.0 MHz | +4.5 MHz |
| PAL | 625 (576 visible) | 50 Hz | 28.3226 MHz | +5.5 MHz |

### Video Output Signals

The RF modulator takes the Atari's composite video signal and modulates it onto the RF carrier:

1. **Video signal**: Monochrome composite video from the Shifter chip
2. **Audio signal**: Mixed audio from YM2149 PSG and digital audio mixer
3. **Combination**: Video and audio combined into RF signal
4. **Output**: RF signal sent to TV via coaxial cable

## Composite Video Output

The Atari ST can output composite video directly from the Shifter chip:

| Pin (DIN13) | Function | Specification |
|-----|------|----|
| 2 | Composite video | 1V pk-pk, 75 ohm |

The composite video signal can be tapped for:
- Genlock with video sources
- VCR recording
- Projector input
- Direct TV connection (via converter)

## RF Output

The TV RF output connector on the rear panel:

| Parameter | Value |
|------ |-|
| Connector type | F-type (screw-on) |
| Signal type | RF modulated TV signal |
| Impedance | 75 ohm |
| Output level | 60-80 dBuV |
| Frequency range | Channel 3 (61.25 MHz) or Channel 4 (69.25 MHz) |

### Channel Selection

Some STM/FM models have a switch to select between channel 3 and channel 4:

| Channel | Frequency | Use case |
|------|--|-|
| Channel 3 | 61.25 MHz | Most common setting |
| Channel 4 | 69.25 MHz | Avoid interference with local TV |

## Audio Output

The RF modulator embeds mono audio along with the video signal:

| Signal | Path | Level |
|------|---|----|
| Audio 1 | YM2149 PSG output | Mixed into RF carrier (+ 4.5 MHz) |
| Audio 2 | Digital audio mixer | Mixed into RF carrier (STe only) |

The audio from the DIN13 pin 1 (mono audio) is embedded into the RF carrier for TV speakers.

## Connection to Television

### Required Cable
- **75 ohm coaxial cable** (RG-59 or similar)
- **F-to-F connector** or F-to-RCA adapter

### TV Input
- **RF/ANT input** on the TV (not HDMI or RF input)
- Set TV to the matching channel (3, 4, or 28)
- Some TVs require a coaxial connector adapter

### Image Quality
- Quality: Lower than RGB monitor (lower resolution, more distortion)
- Suitable for: Gaming, basic applications
- Not suitable for: Desktop text, CAD work

## References

- [Atari ST Internals, ch. 1.3 - Display (PDF)](https://www.atarimania.com/documents/Atari-ST-Internals.pdf)
- [NTSC Standard](https://en.wikipedia.org/wiki/NTSC)
- [PAL Standard](https://en.wikipedia.org/wiki/PAL)
- [Atari STM Service Manual](https://info-coach.fr/atari/hardware/STE-HW.php)
