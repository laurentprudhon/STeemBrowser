# Glue / MMU / DMA Custom Chips

> The original Atari ST (520ST-1040STM) uses three main custom chips: the Glue logic, MMU, and DMA chips.

## Glue Chip (C029144)

The glue chip provides control logic between the 68000 CPU and the rest of the system.

### Part Information

| Parameter | Value |
|------|-----|
| Manufacturer | Atari/Synergy |
| Part | C029144-C3 |
| Package | PLCC68 |
| Function | Generalized Logic Unit (GLU) |
| Speed | 8 MHz (standard), 16 MHz (Mega STE) |

### Glue Chip Functions

1. **Address decode**: Determines which memory/I/O addresses are active
2. **Bus arbitration**: Manages CPU vs. DMA bus control requests
3. **DRAM control**: RAS/CAS generation for DRAM refresh
4. **Video timing**: Generates horizontal and vertical sync signals
5. **Clock generation**: Provides clock signals to the system
6. **Reset logic**: Handles system reset and warm/cold boot sequences

### Glue Output Signals

| Signal | Description |
|------|---|
| AS | Address Strobe (CPU) |
| DTACK | Data Transfer Acknowledge |
| BG | Bus Grant (to DMA) |
| BR | Bus Request (from DMA) |
| RAS0/RAS1 | Row Address Strobe (DRAM banks) |
| CAS | Column Address Strobe (DRAM) |
| HBLANK | Horizontal blanking pulse |
| VBLANK | Vertical blanking pulse |
| HSYNC | Horizontal sync pulse |
| VSYNC | Vertical sync pulse |
| BLANK | Address valid signal |
| LWORD | Long word access signal |
| SIZ | Data size select (16/32 bit mode) |
| RESET | System reset output |
| CLK | Clock output (8 MHz) |

## MMU Chip (C028300)

The MMU (Memory Management Unit) handles DRAM refresh and scrolling.

### Part Information

| Parameter | Value |
|------|-----|
| Manufacturer | Atari/Synergy |
| Part | C028300-C2 |
| Package | PLCC68 |
| Function | Memory Management Unit |
| Speed | 8 MHz |

### MMU Functions

1. **DRAM refresh**: RAS-only refresh cycles (every 4ms for 414616 DRAM)
2. **Page mapping**: Maps physical RAM pages into logical address space
3. **Scroll control**: 8 scroll registers for memory address offset
4. **Address generation**: DRAM row/column address generation
5. **Refresh arbitration**: Decides when CPU gets access vs refresh

### MMU Scroll Registers

| Address | Register | Bits |
|--|--|--|
| $FFE000 | Scroll 0 | 12 bits (0-4095) |
| $FFE002 | Scroll 1 | 12 bits (0-4095) |
| $FFE004-$FFE016 | Scrolls 2-7 | 12 bits each |

Total scroll value is 8 × 12 = 96 bits, providing sub-page scrolling.

### MMU Scroll Use

The scroll registers allow the video memory page to be offset:

```
Logical Address = Physical Address + Scroll_Value
```

For horizontal scrolling in LORAM:
- Each scroll register provides a 12-bit offset
- Combined scroll gives fine-grained horizontal scrolling
- Maximum sub-page scroll = 4095 pixels

## DMA Chip (C029128)

The DMA chip controls direct memory access for floppy and hard disk.

### Part Information

| Parameter | Value |
|------|-----|
| Manufacturer | Atari/Synergy |
| Part | C029128-C1 |
| Package | PLCC68 |
| Function | Direct Memory Access |
| Speed | 8 MHz |

### DMA Functions

1. **Floppy DMA**: Controls data transfer between FDC and RAM via $FF86xx
2. **Hard disk DMA**: Controls data transfer between ACSI/HDC and RAM via $FF8xxx
3. **Bus arbitration**: Requests, grants, and releases bus control
4. **Counters**: Maintains transfer counters and address counters
5. **Status**: Reports completion and error status

### DMA Register (Mapped at $FF8A00-$FF8AFF)

| Offset | Address | Access | Description |
|--|--|--|--|
| 0x00 | $FF8A00 | W | DMA control register |
| 0x01 | $FF8A01 | R | DMA status register |

### DMA Control Register (bit layout)

| Bit | Name | Description |
|----|-- |-- |
| 0 | EN | Enable DMA (1 = enable, 0 = disable) |
| 1 | DIR | Direction (1 = CPU-to-device, 0 = device-to-CPU) |
| 2 | AUT | Auto-initiate (1 = automatic DMA) |
| 3-7 | - | Not used |

### DMA Status Register (bit layout)

| Bit | Name | Description |
|----|----|-- |
| 0 | DONE | DMA transfer complete |
| 1 | REQ | DMA request pending |
| 2 | EOP | End of page |
| 7 | RDY | DMA ready (not currently transferring) |

### DMA Timing

| Mode | Transfer per cycle | Notes |
|-----|- |-- |
| Floppy DMA | 1 byte per DMA cycle | Used for FDC data transfer |
| Hard disk DMA | 1 byte per DMA cycle | Used for ACSI/HDC data transfer |
| DRAM refresh | 1 row per cycle | 1024 rows, 4ms total |

## ST Component Summary (ST/STF/STM/STFM)

| Component | Part | Function |
|------|--|--|
| CPU | MC68000 | 8 MHz processor |
| Glue | C029144 | System control logic |
| MMU | C028300 | Memory management + DRAM refresh |
| DMA | C029128 | DMA controller |
| Shifter | C028787/C028761 | Video display controller |
| MFP | MC68901 | Interrupt/timer/I/O controller |
| FDC | WD1772 | Floppy disk controller |
| YM2149 | YM2149F | Sound generator |
| ACIA (×2) | MC6850 | Keyboard/MIDI serial links |
| IKBD | HD6301 | Keyboard/mouse controller |

## Component Evolution

| Model | Glue | MMU | DMA | Shifter | Notes |
|------|----|-|--|--|-----|
| 520ST/1040ST | C029144 (PLCC68) | C028300 (PLCC68) | C029128 (PLCC68) | C028787 (PLCC68) | 4 custom chips |
| Mega STE | GST MCU (PLCC144) | Integrated | Integrated | C029145 (PLCC144) | 2 chips |
| STe | GST MCU (PLCC144) | Integrated | Integrated | C029145 (PLCC144) | 2 chips |

## References

- [Atari ST Internals, ch. 1.2 - Glue and MMU (PDF)](https://www.atarimania.com/documents/Atari-ST-Internals.pdf)
- [Atari ST Custom Chips (PDF)](https://www.atariparadise.com/files/Atari-st.pdf)
- [Atari ST Bus Doc - Custom Chips (PDF)](http://info-coach.fr/atari/ressources/doc/Atari%20ST%20bus%20doc.pdf)
