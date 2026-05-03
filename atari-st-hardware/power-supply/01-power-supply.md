# Power Supply

> The Atari ST power supply varies by model. The 520ST uses an external power brick, while the STF/1040ST onwards have an internal switched-mode PSU.

## PSU Variants by Model

| Model | PSU Type | Location |
|-------|----------|----------|
| 520ST / 520ST+ | External brick | External, AC cable |
| 520STM | External brick + RF modulator | External |
| 1040STF | Internal switched-mode | Inside console |
| 1040STFM | Internal switched-mode + RF modulator | Inside console |
| Mega ST | Internal switched-mode (220V/110V switch) | Inside console + internal PSU |
| Mega STE | Internal switching | Inside console |

## 520ST External Power Supply

### Specifications

| Parameter | Value |
|-----------|-------|
| Input | 220-240V AC, 50Hz (EU) or 110-120V AC, 60Hz (US) |
| Output | +8V DC, +22V DC, +V (unregulated) |
| Connector | 10-pin DIN plug to motherboard |
| Maximum current (+8V) | ~3A |
| Maximum current (+22V) | ~0.5A |

### DC Output Voltages

| Voltage | Purpose | Ripple (max) |
|---------|---------|-------------|
| +8V | Logic power (regulated) | 100 mV |
| +22V | CRT/analog power (regulated) | 200 mV |
| V (unregulated) | Drive motors (unregulated) | High |

### 520ST External Brick (PSU 1)

| Component | Part | Value |
|-----------|------|-------|
| Transformer | Mitsumi SR98 | 220V 50/60Hz |
| Voltage regulator | CA3007H | +8V / +22V |
| Bridge rectifier | CA3007 | AC to DC |
| Filter caps | Electrolytic | Various microfarads |

### Connector to Motherboard (520ST)

| Pin | Signal |
|-----|--------|
| 1 | +22V DC |
| 2 | +8V DC |
| 3 | +V (unregulated) |
| 4 | -V (negative) |
| 5 | +V (unregulated) |
| 6 | +8V DC (sense) |
| 7 | +8V DC (sense) |
| 8 | +8V DC (sense) |
| 9 | +22V DC (sense) |
| 10 | +22V DC (sense) |

## 1040STF Internal Power Supply

### Specifications

| Parameter | Value |
|-----------|-------|
| Input | 220-240V AC, 50Hz or 110-120V AC, 60Hz |
| Type | Switched-mode power supply (SMPS) |
| +8V output | ~5A |
| +16V output | ~1.5A |
| +40V output | ~0.5A |

### PSU 1 (Mitsumi 68-4231A / ASTEC ASP34)

| Model | PSU Type |
|-------|----------|
| ASTEC ASP34 | 220V EU variant |
| ASTEC ASPF34-1 | 220V EU variant (rev B) |
| Mitsumi SR98 / SR118 | US variant |
| 68-4231A | Universal variant |
| Phihong PSM-5341 | 100-240V variant |
| DVE DSP-508A / DSP-508AA | 100-240V variant |
| Skynet ATR-1886 | 220V variant |

### PSU Internal Connector (to motherboard)

| Pin | Signal |
|-----|--------|
| 1 | +16V (from rectified mains) |
| 2 | +40V (from rectified mains) |
| 3 | +8V DC |
| 4 | Ground |
| 5-8 | Ground |

### PSU 2 (Internal Drive Power)

For models with internal floppy drive (STF, 1040STF, etc.):

| Voltage | Usage |
|---------|-------|
| +12V | Floppy drive motor |
| +5V | Drive logic |

## Power Distribution on Motherboard

### Regulation Chain

1. **Raw AC**: Mains passes through fuse and EMI filter
2. **Rectification**: Bridge rectifier converts AC to DC
3. **SMPS switching**: High-frequency switching (typically 50-100 kHz) steps down voltage
4. **Output rectification**: Diodes rectify the switched output
5. **Regulation**: Linear regulators (CA3007H) provide +8V and +22V rails
6. **Distribution**: Voltage rails distributed to all ICs via power traces

### Power Rail List

| Rail | Voltage | Usage |
|------|---------|-------|
| Vcc logic | +5V | All TTL/CMOS logic ICs |
| Vcc DRAM | +5V / +12V | DRAM refresh circuits |
| Vcc CRT | +22V / +40V | CRT anode, video output |
| Vcc drive | +12V | Floppy drive motors |
| Vcc RGB | +5V (regulated) | RGB DAC resistors |
| Vcc audio | +5V | Sound amplifier |
| Vcc motor | Unregulated | Drive motors |

## Fuse Protection

| Location | Rating | Purpose |
|----------|--------|---------|
| Mains input | T2A / F2AH | Overcurrent protection (main power) |
| +22V output | T2A | +22V rail protection |
| +8V output | T4A | +8V rail protection |
| Drive motor | T1A | Floppy motor protection |

## RF Modulator (STM/FM variants)

| Parameter | Value |
|-----------|-------|
| Type | Analog RF modulator |
| Output | Channel 3 or 4 (NTSC) / Channel 28 (PAL) |
| Video standard | NTSC-M / PAL-B/G or PAL-D/K |
| Modulation type | FM (frequency) |
| Audio | Mono, integrated with video |
| Output connector | 75 ohm F-type or screw-on |

### STM RF Modulator (built into 520STM)

The STM variant adds RF modulator circuitry to allow TV connection:
- Composite video modulated onto RF channel
- Audio embedded in RF carrier (mono)
- Compatible with standard NTSC/PAL televisions
- Lower quality than direct RGB connection

## STacy PSU

The STacy portable has its own internal PSU:

| Parameter | Value |
|-----------|-------|
| Type | Switched-mode, internal |
| Input | Battery (rechargeable) or external adapter |
| Battery | ~8 hours typical |
| External adapter | 9V DC, center positive |

## References

- [Atari ST Internals, ch. 1.9 - Power Supply](https://www.atarimania.com/documents/Atari-ST-Internals.pdf)
- [Console5 - Atari ST Cap Lists](https://wiki.console5.com/wiki/Atari_ST)
- [The LaST Upgrade - Atari PSU repair](https://www.exxosforum.co.uk/atari/last/psu/index.htm)
- [SidecarTridge PSU](https://docs.sidecartridge.com/sidecartridge-psu/)
