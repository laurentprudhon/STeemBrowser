# Super Glue / GST MCU / SH2

> The STe introduced a new generation of custom chips. The original Glue, MMU, DMA, and Shifter chips were replaced by two main SMT chips: the GST MCU and the GST Shifter. The Mega STE added the SH2 chip for 16 MHz operation.

## Component Evolution

### Original ST (520ST, 1040ST, Mega ST)
Four PLCC68 QFP chips:
- Glue (C029144)
- MMU (C028300)
- DMA (C029128)
- Shifter (C028787 / C028761)

### STe (520STE, 1040STE, Mega STe)
Two PLCC144 SMT chips:
- GST MCU (C302183-002) - Glue + MMU + Blitter + Joystick/paddle/pen
- GST Shifter (C029145) - Shifter + Blitter + Audio

### Mega STE
Three chips:
- GST MCU (C302183-002)
- SH2 (C301842) - 16 MHz clock generator + additional address decode
- GST Shifter (C029145)

## GST MCU (Glue + MMU + Blitter + I/O)

Part number: **C302183-002** (also C301157-002, C303075)

This is a 144-pin SMT chip in the STe that integrates:

| Function | Original Chip |
|----------|----|---|
| Glue logic | Glue (C029144) |
| MMU | MMU (C028300) |
| Blitter | (STe doesn't have it, only in Mega STE) |
| Joystick/paddle/pen | new GST I/O feature |
| Clock generation | separate oscillator |
| Address decoding | Glue function |

### GST MCU Functions

1. **DRAM management**: Controls DRAM refresh and page mapping (was MMU's job)
2. **Address decode**: Decodes 68000 address space for RAM/ROM/I/O (was Glue's job)
3. **Address generation**: Provides addresses to DRAM (row/column)
4. **Refresh cycle**: DRAM row refresh (was MMU/Glue combined)
5. **Clock generation**: Generates CLK_16MHZ, CLK_8MHZ, CLK_4MHZ, CLK_2MHZ
6. **Joystick/paddle/pen**: New ADC-based analog input support
7. **Scrolling address**: Same as MMU scrolling registers

### GST MCU Pin Mapping (144-pin SMT)

Key signals and functions of the GST MCU:

| Signal Group | Pins | Description |
|--------------|------|------|
| Address bus | ~50 pins | A0-A23 to 68000, row/col to DRAM |
| Data bus | ~16 pins | D0-D15 to 68000 |
| Clock | ~10 pins | Clock generation |
| DRAM control | ~20 pins | RAS, CAS, MAD to DRAM |
| Bus control | ~10 pins | AS, DTACK, R/W, DE |
| I/O | ~20 pins | Joystick, paddle, pen, IRQ |
| Power | ~10 pins | Vcc, GND |
| Scroll | ~8 pins | Scroll address generation |
| Other | ~10 pins | Various (reset, IRQ, etc.) |

### GST MCU Scroll Registers

Same as the original MMU scroll:
| Address | Register | Size |
|--|---|--|
| $FFE000 | Scroll 0 | 12 bits |
| $FFE002 | Scroll 1 | 12 bits |
| $FFE004-$FFE016 | Scroll 2-7 | 12 bits each |

## GST Shifter (C029145)

The GST Shifter (part C029145) also integrates:

| Function | Description |
|----------|------------|
| Shifter (VDC) | Video display controller |
| Blitter | BITBLT engine (Mega STE only) |
| Audio mixer | 16 channels |
| 8-bit stereo DAC | Sample playback (STe/Mega STE) |
| Super Hi-Res | 640×480 support |
| Color palette | 512 colors (6-bit per component) |

## SH2 Chip (Mega STE only)

Part number: **C301842** (also SH2)

The SH2 chip in the Mega STE adds:

1. **16 MHz clock generation**: Divides the 16 MHz system clock down to 8 MHz, 4 MHz, 2 MHz
2. **Additional address decoding**: Maps Mega STE-specific memory regions
3. **IDE support**: Interface to IDE hard disk drives (replaces some SCSI functions)
4. **Super Hi-Res support**: Additional video timing for 640×480 mode
5. **Memory mapping**: Maps additional DRAM banks (up to 16 MB)

### SH2 Registers

| Address Range | Function |
|--|--|
| $F80000-$F803FF | IDE/HDC control registers |
| $FEC000-$FEC7FF | Super Hi-Res control registers |
| $FEC800-$FEFFFF | Reserved / SH2 control |

### Super Glue Registers (STe/Mega STE)

| Address | Registers | Function |
|-|--|--|
| $FEC000-$FEFFFF | GST MCU registers | GST MCU control |
| $FFC000-$FFCFFF | GST Shifter registers | Super Shifter control |
| $FFE000-$FFE016 | Scroll registers | Memory scroll |
| $FFE400-$FFE4FF | Blitter registers (Mega STE) | Blitter control |
| $FFE600-$FFE6FF | STe audio control | 8-bit stereo DAC |
| $FFE800-$FFE8FF | Audio mixer control | Digital audio |
| $FFEC00-$FFECFF | Palette registers (STe) | 512 color palette |

## STe vs ST Component Summary

| Component | ST | STe | Mega STE |
|------|----|--- | ----|
| Glue | C029144 (PLCC68) | GST MCU (PLCC144) | GST MCU (PLCC144) |
| MMU | C028300 (PLCC68) | GST MCU (built-in) | SH2 (PLCC68) |
| DMA | C029128 (PLCC68) | GST MCU (built-in) | GST MCU (built-in) |
| Shifter | C028787 (PLCC68) | GST Shifter (PLCC144) | GST Shifter (PLCC144) |
| Blitter | No | GST Shifter (built-in, no) | GST Shifter (built-in, yes) |
| SH2 | No | No | C301842 (PLCC68) |

## References

- [Atari STE Hardware - Info-Coach](https://info-coach.fr/atari/hardware/STE-HW.php)
- [Atari ST Internals (PDF)](https://www.atarimania.com/documents/Atari-ST-Internals.pdf)
- [Atari Forum - GST MCU](https://forums.atariage.com/topic/247569-huge-list-of-new-original-atari-custom-ic-chips)
- [Atari ST Bus Doc - GST MCU](http://info-coach.fr/atari/ressources/doc/Atari%20ST%20bus%20doc.pdf)
