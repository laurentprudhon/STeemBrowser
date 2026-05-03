# MC68000 Processor

> The Motorola MC68000 is the main CPU of the Atari ST. This document details the processor's architecture, registers, pins, bus cycles, interrupt system, and timing.

## Processor Overview

The Atari ST uses the **Motorola MC68000P8** running at **8 MHz** (16 MHz in Mega STE). This is a 16/32-bit CISC processor with:

- **16 external data pins** (D0-D15, 16-bit data bus)
- **24 external address pins** (A0-A23, 24-bit address bus → 16 MB address space)
- **8 external interrupt levels** (IPL0-IPL2, with autovectoring for levels 1-7)
- 16 × 32-bit general-purpose registers
- 5 control registers (68010-compatible: SR, USP, two stack pointers, VBR)
- **Bus cycles** at 8 MHz with 4 phase times (32 clock cycles)

### ST-Specific 68000 Variant

The exact chip used varies by model and production run:
- **MC68000P8** - Standard 8 MHz CMOS version
- **MC68000P8B** - Binned 8 MHz version (faster timing)
- **MC68EC000** - Embedded version (later production runs)
- **mc68008** - 8-bit data bus variant (very limited, prototype)

All are functionally compatible for emulator purposes.

## Register Map

### General-Purpose Registers (16 × 32-bit)

| Register | Description |
|------|------|
| D0-D7 | Data registers (used for arithmetic, data movement, as addresses) |
| A0-A5 | Address registers (general purpose) |
| A6 | Frame pointer (convention) |
| A7 | Stack pointer (SP) |

Note: The upper 16 bits of each register are sign-extended in the 68000. Writing to D0-D7 as 16-bit values leaves the upper 16 bits unchanged, but writing as 32-bit clears the upper half.

### Status Register (SR) - 16-bit

```
Bits [15]: T (Trace) - set to trap after next instruction
Bits [14]: - (reserved, always 1)
Bits [13]: S (Supervisor) - 1 = supervisor mode
Bits [12-8]: I (Interrupt Priority Level) - 0-7 (7 = NMI)
Bits [7]: X ( Extend) - carry/borrow for shift/add carry
Bits [6]: 0
Bits [5]: V (Overflow) - arithmetic overflow
Bits [4]: C (Carry) - add/digit carry
Bits [3]: 0
Bit [2]: Z (Zero) - result was zero
Bit [1]: N (Negative) - result is negative
Bits [0]: 0
```

### Control Registers

| Register | Purpose | Access |
|------|------|-|--------|
| SR (Status Register) | Control flags, IPL, mode | MOVE.W to/from SR |
| USP (User Stack Pointer) | User mode stack pointer | MOVE.L USP, An / An, USP |
| VBR (Vector Base Register) | Exception vector table base | MOVE.L #addr, VBR (68010+) |

Only available in supervisor mode via privileged instructions:
- `MOVE.W <ea>, SR`
- `ANDI.W #data, SR`
- `ORI.W #data, SR`
- `EORI.W #data, SR`
- `MOVE.L #addr, VBR`

## Pinout (68-pin DIP)

### Pin Designation

```
Pin   Signal      Dir    Description
──────────────────────────────────────────────────────────────
1     Vcc         PWR    +5V supply
2     GND         GND    Ground
3     DTACK       Input  Done Acknowledge (active low)
4     VPA         IO     Vertical Parity Acknowledge
5     AS          Output Address Strobe (active low)
6     E           Output Enable (clock)
7     LDAH        Output Lower Data Strobe (active low)
8     UDAH        Output Upper Data Strobe (active low)
9     DS0         Output Data Strobe 0 (active low)
10    DS1         Output Data Strobe 1 (active low)
11    BERR        Input  Bus Error (active low)
12    Vcc         PWR    +5V
13    BGACK       Input  Bus Grant Acknowledge (active low)
14    BR          Input  Bus Request (active low)
15    BG          Output Bus Grant (active low)
16    R_W         Output Read / Write (high = Read)
17    Vcc         PWR    +5V
18    A14         Out    Address bit 14
19    A15         Out    Address bit 15
20    A16         Out    Address bit 16
21    A17         Out    Address bit 17
22    A18         Out    Address bit 18
23    A19         Out    Address bit 19
24    A20         Out    Address bit 20
25    A21         Out    Address bit 21
26    FC2         Out    Function Code bit 2 (high = supervisor)
27    FC1         Out    Function Code bit 1
28    FC0         Out    Function Code bit 0
29    A22         Out    Address bit 22
30    GND         GND    Ground
31    A23         Out    Address bit 23 (MSB, sign extension bit)
...
2     GND                         (pins 2, 30, 36, 44, 52, 58, 64)
```

The full pinout in two groups:

#### Left side (top to bottom, pins 1-31):
```
Vcc, GND, DTACK, VPA, AS, E, LDAH, UDAH, DS0, DS1, BERR, Vcc
BGACK, BR, BG, R_W, Vcc, A14, A15, A16, A17, A18, A19, A20
A21, FC2, FC1, FC0, A22, GND, A23
```

#### Right side (top to bottom, pins 32-64):
```
D0, D1, D2, D3, D4, D5, D6, D7, D8, D9, D10, D11, D12, D13, D14, D15
A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, GND
CLK, !RESET, !HALT, Vcc, IPL0, IPL1, IPL2
```

### Signal Groups

#### Address Bus (A0-A23) - 24 lines
| Pin | Signal | Width | Description |
|------|--------|-
-----|---------|
| 2+15 | A0-A7 | 8 bits | Lower address (byte select for D0-D7) |
| 18-32 | A8-A23 | 16 bits | Upper address |

The ST uses all 24 address lines for a **16 MB address space**: $000000-$FFFFFF.

#### Data Bus (D0-D15) - 16 lines, bidirectional
| Pin | Signal | Description |
|-----|--------|------|
| 2+11 | D0-D7 | Lower byte (byte address $X0, $X1, ..., $X7) |
| 32+11 | D8-D15 | Upper byte (byte address $X8, $X9, ..., $XF) |

Note: A0 is used as the **byte select signal** when D0 is paired with D8. When A0 is low, D0-D7 are selected; when A0 is high, D8-D15 are selected.

#### Bus Control Signals
| Signal | Pin | Dir | Description |
|--------|-----|-----|------|
| !AS | 5 | Out | Address Strobe - active low during address phase |
| !LDAH / !UDAH | 7, 8 | Out | Upper/Lower Data Strobe - byte enables for data bus |
| DS0/DS1 | 9, 10 | Out | Data Strobe - active during data phase |
| E | 6 | Out | Clock enable (driven by oscillator) |
| DTACK | 3 | In | Done Acknowledge - peripheral confirms data ready |

#### Function Code (FC0-FC2)
| Pin | Signal | Description |
|-----|--------|------|
| 26 | FC2 | 1 = supervisor mode, 0 = user mode |
| 27 | FC1 | 1 = data access, 0 = program access |
| 28 | FC0 | 1 = data access (I/O or memory), 0 = program (instructions) |

Combined function codes:
| FC2 | FC1 | FC0 | Meaning |
|-----|-----|-----|---------|
| 0 | 0 | 0 | User program fetch |
| 0 | 0 | 1 | User data access (I/O or memory) |
| 0 | 1 | 0 | User data access (I/O or memory) |
| 0 | 1 | 1 | User data access (I/O or memory) |
| 1 | 0 | 0 | Supervisor program fetch |
| 1 | 00 | 1 | Supervisor data (I/O or mem) |
| 1 | 1 | 0 | Supervisor data (I/O or mem) |
| 1 | 1 | 1 | Supervisor data (I/O or mem) |

#### Interrupt Control
| Signal | Pin | Dir | Description |
|--------|-----|-----|------|
| IPL0 | 61 | In | Interrupt level 0 (NMI) |
| IPL1 | 62 | In | Interrupt level 1 (BLANK) |
| IPL2 | 33 | In | Interrupt level 2 (HBLANK from Glue) |
| BR | 14 | In | Bus Request (from DMA) |
| BG | 15 | Out | Bus Grant (acknowledge to DMA) |
| BGACK | 13 | In | Bus Grant Acknowledge |

#### Control
| Signal | Pin | Dir | Description |
|--------|-----|-----|------|
| CLK | 60 | In | 8 MHz system clock |
| !RESET | 38 | In | Active low reset |
| !HALT | 37 | In | Halt processor (active low) |
| BERR | 11 | In | Bus Error (active low) |

## Reset Sequence

Upon reset, the MC68000:

1. Initializes SR to `$2700` (supervisor mode, IPL=7, trace=0)
2. Sets initial SSP (supervisor stack pointer) from address `$000000` (4 bytes)
3. Sets PC (program counter) from address `$000004` (4 bytes)
4. Begins executing from the address in `$000004-$000007`

TOS ROM places the initial bootstrap code at `$004000-$004400` (the "warm boot" entry point) or `$002000` (ROM bootstrap). The exact vectors are:

| Address | Content |
|-- --|----|
| $000000 | Initial SSP (4 bytes) |
| $000004 | Initial PC (4 bytes) |
| $000008 | Bus error vector |
| $00000C | Address error vector |
| ... | ... (continues through exception table) |

## Bus Cycles

The 68000 uses **phase-based bus cycles**. Each bus cycle takes 4 phase times (32 clock cycles at 8 MHz = 4 us).

### Read Cycle Sequence

```
Phase 1 (t0-t7):  A0-A23 placed on address bus
Phase 2 (t8-15):  AS goes low, FC0-FC2 placed on bus
Phase 3 (t16-23): DS0/DS1, LDAH/UDAH strobe data phase
Phase 4 (t24-31): Data on D0-D15, DTACK must arrive by t32
```

### Write Cycle Sequence

```
Phase 1 (t0-t7):  A0-A23 placed on address bus
Phase 2 (t8-15):  AS goes low, FC0-FC2 placed on bus
Phase 3 (t16-23): D0-D15 placed, R_W stays high
Phase 4 (t24-31): AS goes low again, data written
```

### DMA Cycle

- DMA chip takes over bus after BGACK from CPU
- DMA can generate its own address (only A1 used, rest from DMA)
- DMA data transfer uses D0-D7 for lower byte, D8-D15 for upper byte
- DMA DTACK is generated by the DMA chip (not the peripheral)

## Instruction Timing (Approximate)

| Instruction | Cycles (at 8 MHz) | Notes |
|------|--------|------|
| ABCD, NBCD | 15 | Byte arithmetic |
| ADD/L/DIV/MUL/SUB | 23-121 | Varies by addressing mode |
| ADDQ | 2-6 | Quick add |
| AND/EOR/OR | 2-30 | Varies by addressing mode |
| Bcc | 12-25 | Branch (taken/not taken) |
| JSR | 11-12 | Jump to subroutine |
| JMP | 9 | Jump |
| LEA | 3-6 | Load effective address |
| MOVE | 2-10 | Register-to-register |
| MOVE.L to SR | 4 | Load status register |
| MOVE.L to USP | 4 | Load user stack pointer |
| PEA | 4-10 | Push effective address |
| RTE | 30 | Return from exception |
| RTS | 10 | Return from subroutine |
| TRAP | 38 | Software trap |
| NOP | 1 | No operation |
| MOVEQ | 1 | Quick move immediate |

At 8 MHz, a 2-cycle instruction takes 1 us. A 38-cycle TRAP takes 4.75 us.

## Clock and Oscillator

The MC68000 is driven by an **8.000 MHz crystal** (Y1) connected to the CLK pin. Some timing values:

| Clock | Frequency | Period |
|------|--------|----------|
| Main clock (CLK) | 8.000 MHz | 125 ns |
| Phase time (t) | 8.000 MHz | 125 ns |
| Bus cycle | 2.000 MHz | 500 ns (4 phases × 125 ns) |
| RAM refresh | ~15.6 kHz | Depends on DRAM |

## Exceptions and Interrupts

See [bus/02-interrupts.md](../bus/02-interrupts.md) for the ST-specific interrupt system.

## Addressing Modes

The 68000 supports 13 addressing modes. Key ones used in ST programming:

| Mode | Syntax | Example | Bytes/cycle |
|------|--------|-|------|------|
| Register | Dn / An | `MOVE.D0, D1` | 1/2 |
| Register Direct | Dn / An | `ADD D0, D1` | 1/2 |
| Address Register Indirect | (An) | `MOVE (A0), D0` | 1/2 |
| Address Register Indirect with Displacement | (d16, An) | `MOVE 8(A0), D0` | 3/4 |
| Address Register Indirect with Index | (d8, An, Xn.w) | `MOVE (8, D0.W), D1` | 3/4/6 |
| Absolute Short | (d16) | `MOVE.L #(long), D0` | 3/4 |
| Absolute Long | (d32) | `MOVE.L #long, D0` | 5/6 |
| Program Counter | (d16, PC) | `MOVE.L (d16, PC), D0` | 3/4/6 |
| Immediate | #data | `MOVE.L #value, D0` | 3 |

## References

- [Atari ST Internals, Ch. 1.1 - The 68000 Processor (PDF)](https://www.atarimania.com/documents/Atari-ST-Internals.pdf)
- [Motorola MC68000 User's Manual](https://www.nxp.com/docs/en/user-manual/MC68000UM.pdf) (NXP)
- [68000 Assembly for the Atari ST - ChibiAkumas](https://www.chibiakumas.com/68000/atarist.php)
- [Atari ST Bus Guide](https://atariwiki.org/wiki/Wiki.jsp?page=Bus)
