# CA3007H Dual Adjustable Voltage Regulator

> The CA3007H is the core linear voltage regulation chip in the Atari ST external power supply (520ST / 520STM). It provides dual independent regulated outputs from a single package — +8V for logic rails and +22V for CRT/analog circuits.

## Table of Contents

- [Overview](#overview)
- [Pinout](#pinout)
- [Electrical Specifications](#electrical-specifications)
- [Protection Features](#protection-features)
- [Atari ST Power Rail Distribution](#atari-st-power-rail-distribution)
- [Input/Output Capacitor Requirements](#inputoutput-capacitor-requirements)
- [Power Dissipation Calculations](#power-dissipation-calculations)
- [Replacement / Regulator Options](#replacement--regulator-options)
- [Emulation Notes](#emulation-notes)

---

## Overview

### What is the CA3007H?

The CA3007H is a **dual adjustable positive voltage regulator** manufactured by Signetics (later Philips). It contains **two complete, independent adjustable regulator circuits** within a single TO-3 metal can package. Each regulator channel functions similarly to an LM317 — it uses a 1.25V internal reference and a feedback resistor network to set the output voltage.

In the Atari ST, the CA3007H is used in the **520ST / 520STM external power supply brick** (PSU 2) to provide:

| Regulator Channel | Output | Voltage | Purpose |
|-------------------|--------|---------|---------|
| Regulator 1 | +8V DC | 8.0V | Logic power (5V logic via secondary regulation, ICs, GTia/Shifter) |
| Regulator 2 | +22V DC | 22.0V | CRT video and analog circuitry power |

### Which Atari ST models use the CA3007H?

| Model | Uses CA3007H? | PSU Type |
|-------|---------------|----------|
| **520ST** | Yes | External brick with CA3007H |
| **520ST+** | Yes | External brick with CA3007H |
| **520STM** | Yes | External brick with CA3007H |
| **1040STF** | No | Internal SMPS (PSU 1: Mitsumi/Astec) |
| **1040STFM** | No | Internal SMPS + RF modulator inside |
| **Mega ST** | No | Internal SMPS (220V/110V switch) |
| **Mega STe** | No | Internal switching PSU |
| **1040STE** | No | Internal SMPS |
| **Mega STE** | No | Internal switching PSU |
| **STacy** | No | Internal/external adapter PSU |

The CA3007H appears **exclusively** in the early external-brick ST models (520ST, 520ST+, 520STM). Later models moved regulation into the internal switched-mode power supply, eliminating a separate linear regulator chip on the power board.

### Physical Description

| Property | Value |
|----------|-------|
| Package | TO-3 metal can (10 pins) |
| Manufacturer | Signetics (later Philips Semiconductors) |
| Die | Two complete adjustable regulators on single die |
| Mounting | Insulated from heatsink (plastic washer or mica insulator) |
| Lead Material | Metal case mounted — case is electrically active |

---

## Pinout

### CA3007H Pin Configuration (Top View — Looking at Pin Face)

```
    CA3007H (TO-3 Metal Can Package)
    ┌─────────┐
  0 │ ═══│ ═══ │ 0      Heatslug = Pin 10
  2 └─│┌─────┐│─┘ 1      Slug connects internally to Output 2
  4   ││     ││ 3
  6   ││  DIE││ 5
    └─┴┴─────┴┴─┘
     8   7   9   1 1
```

**Detailed Pin Descriptions (looking at the flat pin face, pins numbered clockwise starting from top-left):**

| Pin | Name | Description |
|-----|------|-------------|
| 1 | OUT1 | Output of Regulator 1 (regulated voltage) |
| 2 | ADJ1 | Adjustment pin for Regulator 1 — feedback input from resistor divider |
| 3 | GND1 | Ground connection for Regulator 1 internal circuitry |
| 4 | IN1 | Input of Regulator 1 (unregulated DC from bridge rectifier) |
| 5 | OUT2 | Output of Regulator 2 (regulated voltage) |
| 6 | ADJ2 | Adjustment pin for Regulator 2 — feedback input from resistor divider |
| 7 | GND2 | Ground connection for Regulator 2 internal circuitry |
| 8 | IN2 | Input of Regulator 2 (unregulated DC from bridge rectifier) |
| 9 | NC/HEAT | No connect / thermal pad contact point |
| 10 | SLUG/OUT2 | Metal heatslug — electrically connected to Output 2 (OUT2) |

### Functional Summary Diagram

```
  Pin 4        Pin 1
 IN1 ──────────┤    ┌─── OUT1 (Regulator 1 regulated output)
                │    │
                │ REG│
                │  1 │
  Pin 2  Pin 3 ────────┤    ┌─── ADJ1 (feedback to resistor divider)
 ADJ1  ──┤ GND1 (IC ground)  │
                  └───────────┘

  Pin 8        Pin 5
 IN2 ──────────┤    ┌─── OUT2/SLUG (Regulator 2 regulated output)
                │    │
                │ REG│
                │  2 │
  Pin 6  Pin 7 ────────┤    ┌─── ADJ2 (feedback to resistor divider)
 ADJ2  ──┤ GND2 (IC ground)  │
                  └───────────┘

  Pin 10 ─── SLUG ── electrically tied to OUT2
  Pin 9  ─── NC  ── no internal connection (for mounting thermal contact)
```

### CA3007H Pinout Diagram (Expanded View)

```
                    CA3007H Package Cross-Section

              ┌───────────────────────────────┐
              │       Metal TO-3 Can          │
        ┌─────┤                               ├─────┐
        │     │  ┌───────┐  ┌───────┐        │     │
        │     │  │REG 1  │  │ REG 2 │        │     │
        │     │  │       │  │       │        │     │
  Pin 1 ┤OUT1 │  │       │  │       │  │OUT2 ├─┐  Pin 5
        │     │  │       │  │       │  │     │  │     │
  Pin 2 ┤ADJ1 │  │ REF  │  │ REF  │  │ADJ2 ├─┤  Pin 6
        │     │  │       │  │       │  │     │  │     │
  Pin 3 ┤ GND1│  │       │  │       │  │ GND2├─┤  Pin 7
        │     │  └───────┘  └───────┘        │     │
  Pin 4 ┤ IN1 ├───────────────────────────────┤ IN2 ├─ Pin 8
        │     │                               │     │
  Pin 9 ┤  NC  ┤         ┌──────────┐        ├─ HEAT├─ Pin 10(sl)
        │     └───────────┤ DIE SUB──┤────────┘     │
        └──────────────────┤ (Ceramic)├──────────────┘
                           └──────────┘

   Legend:
     SL = Heatslug (Pin 10, electrically tied to OUT2)
```

### Power Supply Integration — CA3007H in 520ST External Brick

```
  ┌──────────────────────────────────────────────────────────────┐
  │           520ST / 520STM EXTERNAL POWER BRICK               │
  │                                                              │
  │   AC Input ──┐                                              │
  │   (120/240V) │                                              │
  │              ▼                                              │
  │   ┌─────────────────┐       ┌──────────────┐               │
  │   │  Transformer    │       │  Bridge       │               │
  │   │  (Mitsumi SR98) │──────►│  Rectifier    │               │
  │   └─────────────────┘       └──────┬───────┘               │
  │                                     │                       │
  │                           Unregulated DC                    │
  │                        (typ. ~10V-14V per winding)        │
  │                                     │                       │
  │                    ┌────────────────┼───────────────┐       │
  │                    │                │               │       │
  │                    ▼                ▼               ▼       │
  │              ┌──────────┐    ┌──────────────┐    ┌───────┐  │
  │              │ C INPUT1 │    │ CA3007H      │    │C OUT1 │  │
  │              │ (input   │    │              │    │(output│  │
  │              │  filter) │    │ ┌─┐          │    │ filter│  │
  │              └──────────┘    │ │ TO-3       │    └───────┘  │
  │                               │ │ Package    │                │
  │  +8V Rail <──────────────────┤1 ┤           │                │
  │  +22V Rail <─────────────────┤5 │           │                │
  │                               └─┴┘           │                │
  │                    ┌──────────────────────────┤                │
  │                    │                |          │                │
  │                    ▼                ▼          ▼                │
  │              ┌──────────┐    ┌───────────┐    ┌───────┐       │
  │              │ C INPUT2 │    │ C GND1    │    │ C OUT2│       │
  │              │ (input   │    │ (GND filt)│    │(output│       │
  │              │  filter) │    └───────────┘    │ filter│       │
  │              └──────────┘             ▲       └───────┘       │
  │                                      │    +22V Regulated    │
  │                 GND Reference ◄───────┘    +8V Regulated    │
  │                                      │                       │
  └──────────────────────────────────────┼───────────────────────┘
                                         │
                                   10-pin DIN
                                   to motherboard
  ┌─────────────────────────────────────▼──────────────────────────┐
  │   10-PIN DIN CONNECTOR (to motherboard)                       │
  │                                                                 │
  │  Pin 1 = +22V DC     Pin 6 = +8V sense                        │
  │  Pin 2 = +8V DC      Pin 7 = +8V sense                        │
  │  Pin 3 = +V unreg    Pin 8 = +8V sense                        │
  │  Pin 4 = GND/neg     Pin 9 = +22V sense                       │
  │  Pin 5 = +V unreg    Pin 10 = +22V sense                      │
  └─────────────────────────────────────────────────────────────────┘
```

---

## Electrical Specifications

### Absolute Maximum Ratings

| Parameter | Rating |
|-----------|--------|
| Input-to-Output Voltage Difference | 30V per regulator |
| Output Current (per regulator) | 1.5A |
| Peak Output Current (per regulator) | 2.25A (short burst) |
| Junction Temperature Range | -55°C to +150°C |
| Maximum Power Dissipation (with heatsink) | 30W (total, both regulators) |
| Maximum Power Dissipation (free air) | 20W (total, both regulators) |
| Maximum Case Temperature | +150°C |

### Electrical Characteristics (per regulator channel)

| Parameter | Test Conditions | Min | Typ | Max | Unit |
|-----------|----------------|-----|-----|-----|------|
| **Output Voltage Range** | Adjustable via external resistors | 1.25 | — | 30 | V |
| **Reference Voltage (VREF)** | — | — | 1.25 | — | V |
| **Reference Voltage Tolerance** | TA = 25°C | — | — | ±2% | % |
| **Line Regulation** | VIN – VOUT = 3V → 25V | — | 0.01 | 0.05 | %/V |
| **Load Regulation** | IOUT = 10mA → 1.5A | — | — | 0.3% | V/V |
| **Output Voltage Temperature Coefficient** | −55°C → +150°C | — | — | 0.1% | /°C |
| **Adjustment Pin Current (IADJ)** | — | — | 50 | 100 | μA |
| **Adjustment Pin Current Stability** | IOUT = 10mA → 1.5A | — | — | 5 | μA |
| **Ripple Rejection Ratio** | f = 120 Hz, CADJ = 10 μF | 45 | 65 | — | dB |
| **Ripple Rejection Ratio (no CADJ)** | f = 120 Hz | 30 | 50 | — | dB |

### Thermal Characteristics

| Parameter | Rating |
|-----------|--------|
| Junction-to-Ambient Thermal Resistance (RθJA) | 62.5°C/W (free air) |
| Junction-to-Case Thermal Resistance (RθJC) | 2.5°C/W (metal can) |
| Maximum Junction Temperature | 150°C |
| Recommended Max Case Temperature | 125°C |

### Typical Internal Circuit Parameters

Each regulator channel contains:

| Internal Component | Description |
|-------------------|-------------|
| Reference | 1.25V bandgap reference voltage |
| Error amplifier | Differential amplifier |
| Pass transistor | NPN power transistor (series pass element) |
| Current limit | Internal current-sensing resistor + foldback circuit |
| Thermal protection | Diode junction temperature sensor |

---

## Protection Features

### 1. Overcurrent Protection (Current Limiting)

| Feature | Detail |
|---------|--------|
| Type | Constant current foldback limiting |
| Maximum limit current | 1.5A (specified) |
| Peak surge capability | 2.25A (brief, <1s) |
| Foldback behavior | As output voltage drops (short circuit), current limit decreases proportionally to reduce power dissipation |
| Recovery | Auto-recovery when fault is removed |

The foldback characteristic is critical: when the output is shorted to ground, the current limit **decreases** rather than staying constant. This prevents the regulator from dissipating excessive power during a short circuit, since P = I × (VIN − VOUT) would otherwise be at its maximum.

### 2. Thermal Shutdown

| Feature | Detail |
|---------|--------|
| Type | Junction temperature sensor (bipolar diode) |
| Trip threshold | ~150°C junction temperature |
| Hysteresis | ~20°C (resets at ~130°C) |
| Auto-recovery | Yes — output returns when junction cools |
| Interaction with current limit | Thermal shutdown activates before current limit would cause damage |

The thermal shutdown activates independently of current limiting. Even with no load, if the junction temperature reaches the trip point (e.g., from high input-to-output differential voltage), regulation is suspended.

### 3. Safe Operating Area (SOA) Compensation

| Feature | Detail |
|---------|--------|
| Purpose | Prevents transistor avalanche at high voltage/current combinations |
| Mechanism | Internal SOA limiter reduces current as (VIN − VOUT) increases |
| Region A | Low VCE, high IC — limited by current limit |
| Region B | High VCE, medium IC — limited by SOA curve |
| Region C | High VCE, low IC — limited by power dissipation |

The SOA protection is essential because the pass transistor's safe operating area is bounded not just by maximum current and maximum voltage, but by the power dissipation curve. At high collector-emitter voltages, the allowable current must be reduced to prevent hot-spot formation.

### 4. Internal Short-Circuit Protection

| Feature | Detail |
|---------|--------|
| Output-to-GND short | Protected by foldback current limit |
| Output-to-Input short | Limited by pass transistor breakdown characteristics |
| Reverse output voltage | Not explicitly protected; external diode recommended for capacitive loads |

---

## Atari ST Power Rail Distribution

### Regulator Output Connections (CA3007H Pin 1 and Pin 5)

```
  CA3007H Regulator Channels in Atari ST Application

  ┌─────────────────────────────────────────┐
  │    CA3007H                             │
  │                                         │
  │  Reg 1     ┌─ OUTPUT 1 (Pin 1)        │
  │  Reg 2     └─ OUTPUT 2 (Pin 10/Slug)  │
  │                                         │
  │  ADJ1 ──┐                               │
  │  ADJ2 ──┘      Voltage-setting resistors│
  └─────────────────────────────────────────┘
                     │                │
              +8V regulated      +22V regulated
              Pin 1              Pin 5/Slug
                     │                │
```

### +8V Rail Distribution

The +8V output from Regulator 1 feeds multiple subsystems on the Atari ST motherboard:

```
  +8V REGULATOR OUTPUT (Pin 1)
  │
  ├──► [Local RC Filter] ──► +5V LOGIC REGULATOR (on-board, if present)
  │                           │
  │                           ├──> MC68000 CPU (core logic)
  │                           ├──> Glue / MMU / DMA chips
  │                           ├──> GTia/Shifter video processor
  │                           ├──> HD6301 IKBD controller
  │                           ├──> MC68901 MFP
  │                           ├──> WD1772 FDC
  │                           ├──> YM2149 PSG (sound)
  │                           └──> All TTL/CMOS logic ICs
  │
  ├──► [Heavily filtered] ──► +5V RGB DAC reference
  │                           (DAC resistor network)
  │
  ├──► [Filtered] ──► +5V Audio circuits
  │                       (sound amplifier)
  │
  └──► [Through 10-pin DIN] ──► External sense lines (pins 6-8)
```

+5V Logic ICs powered (typical):

| Chip | Function | Current Draw |
|------|----------|-------------|
| MC68000 | CPU core | ~500mA |
| Glue + MMU + DMA | Custom silicon | ~200mA |
| GTia/Shifter | Video generation | ~150mA |
| HD6301V1 | IKBD keyboard/mouse | ~50mA |
| MC68901 | MFP (I/O) | ~50mA |
| WD1772 | Floppy controller | ~50mA |
| YM2149 | Sound generator | ~30mA |
| Various TTL (74-series) | Interface logic | ~200mA |
| **Total (approx)** | | **~1000mA** |

### +22V Rail Distribution

The +22V output from Regulator 2 feeds CRT and analog circuitry:

```
  +22V REGULATOR OUTPUT (Pin 5 / Slug)
  │
  ├──► [Filtered] ──► CRT video output amplifiers
  │                       (Shifter output buffers)
  │
  ├──► [Filtered] ──► GTia analog video processing
  │                       (color/overlay circuits)
  │
  ├──► [Filtered] ──► RF modulator (STM variants)
  │                       (composite video modulation)
  │
  └──► [Through 10-pin DIN] ──► External sense line (pin 9, 10)
```

### Unregulated +V Rail

Note: The external brick also provides an unregulated +V rail (not from the CA3007H):

```
  UNREGULATED V (transformer winding, post-rectification, pre-regulation)
  │
  ├──► Floppy drive motor (12V when transformer taps for higher voltage)
  ├──► Drive mechanism circuits
  └──► No direct connection to CA3007H
```

### Complete Power Distribution Block Diagram

```
  ATARI ST POWER SUPPLY BLOCK DIAGRAM (520ST External Brick)

  ┌─────────────┐
  │  Mains AC   │  110-120V or 220-240V, 50/60Hz
  │  Input      │
  └──────┬──────┘
         │
  ┌──────▼──────┐
  │   Mains     │  EMI filter + fuse T2A
  │   Fuse/Filt │
  └──────┬──────┘
         │
  ┌──────▼──────┐
  │  Transformer│  Mitsumi SR98 (step-down)
  │  (Dual wtg) │  Primary: mains, Secondary: dual
  └──────┬──────┘
         │  Dual secondary windings
         │  (typically 10V-14V AC each)
  ┌──────┴──────┐
  │             │
  ▼             ▼
  ┌────────┐   ┌────────┐
  │ Bridge │   │ Bridge │
  │ Rect 1 │   │ Rect 2 │
  └───┬────┘   └───┬────┘
      │             │
      ▼             ▼
  ┌──────────┐  ┌──────────┐
  │+12V unreg│  │+10V unreg│  (example values)
  │(Reg 1 In)│  │(Reg 2 In)│
  └────┬─────┘  └────┬─────┘
       │              │
  ┌────▼─────┐  ┌────▼─────┐
  │CA3007H   │  │CA3007H   │
  │ REG 1    │  │ REG 2    │
  │ (Pin 4)  │  │ (Pin 8)  │
  └────┬─────┘  └────┬─────┘
       │              │
       ▼              ▼
  ┌──────────┐  ┌──────────┐
  │ +8V      │  │ +22V     │
  │ regulated│  │ regulated│
  └────┬─────┘  └────┬─────┘
       │              │
       │              │
  ┌────▼──────────────▼────┐
  │     10-PIN DIN        │
  │    CONNECTOR          │
  └────┬──────────────────┬┘
       │                  │
       ▼                  ▼
  ┌──────────┐      ┌──────────┐
  │ +8V DIN  │      │ +22V DIN │
  │ (Pin 2)  │      │ (Pin 1)  │
  └────┬─────┘      └────┬─────┘
       │                  │
  ┌────▼───────────────────▼─────┐
  │         MOTHERBOARD          │
  │                              │
  │  +8V ──► Local regulator ──► +5V Logic                   │
  │        │                     ├─ MC68000                   │
  │        │                     ├─ Glue/MMU/DMA             │
  │        │                     ├─ Shifter/GTia             │
  │        │                     ├─ IKBD                     │
  │        │                     ├─ Other Logic ICs          │
  │        │                     └─ Blitter                  │
  │                              │                            │
  │  +22V ──► CRT circuits ──► + GTia analog                 │
  │        │                   + RF modulator (STM)          │
  │        │                   + Video buffering             │
  │                              │                            │
  │  Unreg ──► Drive motors ──► Floppy drive                │
  │                                mechanism                 │
  └──────────────────────────────────────────────────────────┘
```

---

## Input/Output Capacitor Requirements

### Regulator 1 (+8V) — Capacitor Network

| Position | Value | Voltage Rating | Type | Purpose |
|----------|-------|----------------|------|---------|
| C1 (Input, Pin 4) | 1000 μF | 25V | Electrolytic | Input filter / energy storage |
| C2 (Output, Pin 1) | 100 μF | 25V | Electrolytic | Output ripple reduction |
| C3 (Output, Pin 1) | 0.1 μF | 50V | Ceramic | High-frequency decoupling |
| C4 (ADJ1, Pin 2) | 10 μF | 25V | Electrolytic | Ripple rejection (noise filter on ADJ) |
| C5 (ADJ1, Pin 2) | 0.1 μF | 50V | Ceramic | High-frequency filter on ADJ |

### Regulator 2 (+22V) — Capacitor Network

| Position | Value | Voltage Rating | Type | Purpose |
|----------|-------|----------------|------|---------|
| C6 (Input, Pin 8) | 1000 μF | 35V | Electrolytic | Input filter / energy storage |
| C7 (Output, Pin 5) | 100 μF | 35V | Electrolytic | Output ripple reduction |
| C8 (Output, Pin 5) | 0.1 μF | 50V | Ceramic | High-frequency decoupling |
| C9 (ADJ2, Pin 6) | 10 μF | 35V | Electrolytic | Ripple rejection (noise filter on ADJ) |
| C10 (ADJ2, Pin 6) | 0.1 μF | 50V | Ceramic | High-frequency filter on ADJ |

### Capacitor Selection Notes for Emulation

| Parameter | Recommended for Accurate Modeling |
|-----------|----------------------------------|
| ESR of electrolytic caps | 0.1–2 Ω (depends on value — lower for larger caps) |
| ESR of ceramic caps | <0.1 Ω (negligible) |
| Capacitor leakage | Negligible for emulation, but large caps in original supply had measurable leakage over time |
| Capacitor aging | Electrolytic ESR increases with age; original 520ST bricks may have dried-out capacitors |

### Capacitor Ripple Current Ratings

| Capacitor | RMS Ripple Current (approx) | Notes |
|-----------|------------------------------|-------|
| C1, C6 (Input) | ~500 mA | Highest ripple; must be rated for sustained current |
| C2, C7 (Output) | ~50 mA | Lower ripple; standard electrolytic sufficient |
| C3–C10 (High-freq) | ~5 mA | Ceramic; negligible ripple |
| C4, C9 (ADJ) | ~1 mA | Filter current; very low ripple |

### Minimum Capacitor Requirements

The CA3007H datasheet specifies **minimum** capacitance values for stability:

| Position | Minimum Value | Minimum Voltage Rating |
|----------|---------------|----------------------|
| Input (IN) | 1.0 μF | ≥ input peak voltage |
| Output (OUT) | 1.0 μF | ≥ output voltage |
| ADJ | 0.1 μF | ≥ output voltage |

The Atari ST external brick used much larger values (1000 μF input, 100 μF output) for low ripple and stability under full load, which is why the +8V and +22V outputs have such low ripple specifications.

---

## Power Dissipation Calculations

### Regulator 1: +8V Channel

**Worst-case power dissipation:**

| Parameter | Value |
|-----------|-------|
| Regulated output voltage | 8V |
| Unregulated input voltage (from transformer secondary, rectified) | ~12V-14V peak |
| Minimum differential (dropout) | 2V (typical) |
| Maximum differential | ~6V |
| Maximum output current | ~1.5A (limited by transformer rating ~3A) |
| **Worst-case power dissipation** | **P = I × ΔV = 1.5A × 6V = 9W** |

### Regulator 2: +22V Channel

| Parameter | Value |
|-----------|-------|
| Regulated output voltage | 22V |
| Unregulated input voltage (from transformer secondary, rectified) | ~27V-31V peak |
| Minimum differential (dropout) | 2V (typical) |
| Maximum differential | ~9V |
| Maximum output current | ~0.5A (limited by transformer +22V winding rating) |
| **Worst-case power dissipation** | **P = I × ΔV = 0.5A × 9V = 4.5W** |

### Combined Power Dissipation

| Parameter | Value |
|-----------|-------|
| Total worst-case dissipation (both regulators, same chip) | ~13.5W |
| With heatsink on TO-3 case | Within limits (RθJC = 2.5°C/W, max 30W with adequate heatsink) |
| Case-to-heatsink thermal resistance | ~0.5°C/W (with thermal compound) |
| Heatsink-to-ambient thermal resistance needed | ~2.3°C/W (to stay under 150°C junction) |
| Typical heatsink used in 520ST brick | Metal case contacts internal heatsink plate |

### Temperature Rise Calculation

```
  REGULATOR 1 (+8V, worst case):
  ─────────────────────────────────────────

  Junction Temp = Ta + (P × RθJA)

  With heatsink:
  TJ = Tambient + P × (RθJC + RθCS + RθSA)
  TJ = 40°C + 9W × (2.5 + 0.5 + 2.3) °C/W
  TJ = 40°C + 9W × 5.3 °C/W
  TJ = 40°C + 47.7°C
  TJ = 87.7°C  (well within 150°C limit)

  Without adequate heatsink:
  TJ = 40°C + 9W × 62.5°C/W  (RθJA free air)
  TJ = 40°C + 562.5°C
  TJ = 602.5°C  → THERMAL SHUTDOWN at ~150°C
```

```
  REGULATOR 2 (+22V, worst case):
  ─────────────────────────────────────────

  TJ = Tambient + P × (RθJC + RθCS + RθSA)
  TJ = 40°C + 4.5W × 5.3 °C/W
  TJ = 40°C + 23.85°C
  TJ = 63.85°C  (comfortably within limits)
```

### Power Dissipation vs Input Voltage

| VIN (unregulated) | ΔV (Reg 1) | ΔV (Reg 2) | P_REG1 (at Imax) | P_REG2 (at Imax) | Total |
|-------------------|------------|------------|-------------------|------------------|-------|
| 10V (nominal) | 2V | - | 3W | — | — |
| 12V (nominal) | 4V | 10V | 6W | 5W | **11W** |
| 14V (high) | 6V | 12V | 9W | 6W | **15W** |
| 8V (low/sag) | 0V | - | — | — | — |

### Drop-out Voltage

The minimum input-to-output voltage differential for proper regulation:

| Condition | Dropout (typical) | Dropout (max) |
|-----------|-------------------|---------------|
| IOUT = 10 mA | 1.5V | 2.0V |
| IOUT = 100 mA | 1.7V | 2.2V |
| IOUT = 500 mA | 2.0V | 2.5V |
| IOUT = 1000 mA | 2.2V | 3.0V |
| IOUT = 1500 mA | 2.5V | 3.5V |

The dropout voltage increases with output current due to the saturation voltage of the internal pass transistor. This is an important parameter for emulation: if the transformer sags under heavy load, the input voltage drops and the regulator may fail to maintain regulation.

---

## Replacement / Regulator Options

### Direct Replacement (OEM-equivalent)

| Part | Manufacturer | Notes |
|------|-------------|-------|
| **CA3007H** | Signetics / Philips | OEM part — rare, collectible |
| **SJA3007H** | SGS-Thomson | Direct replacement |
| **MB3007H** | Mitsubishi | Pin-compatible |
| **D3007** | Sanken / various | Close equivalent |

### Modern Equivalent Using Individual Regulators

Since the CA3007H is essentially two LM317 circuits on one die, the most common modern approach is to use two discrete LM317 regulators:

| Regulator | Configuration |
|-----------|--------------|
| **LM317 (×2)** | Two LM317T in TO-220 packages — most common replacement |
| **LM317K (×2)** | LM317 in TO-3 package — closest form-factor match |
| **MC146905 / SLE4710H** | Similar dual adjustable regulators (older stock) |

### Replacement Circuit Comparison

```
  OEM (CA3007H) vs MODERN REPLACEMENT

  CA3007H:                    Two LM317s:
  ┌─────────────┐            ┌───────┐  ┌───────┐
  │  ┌───┐  ┌───┐ │          │  317  │  │  317  │
  │  │REG│  │REG│ │          │ REG1  │  │ REG2  │
  │  │ 1 │  │ 2 │ │          │       │  │       │
  │  └───┘  └───┘ │          └───┬───┘  └───┬───┘
  │ Single TO-3    │            │            │
  │ One chip       │            │            │
  └─────────────┘            └────────────┘

  Advantages of CA3007H:          Advantages of LM317×2:
  - Single component              - Available everywhere
  - Matched tracking              - Individual adjustment possible
  - Compact                       - Cheaper (discrete)
  - OEM authentic                 - Easier to diagnose
```

### Voltage-setting Resistor Values (for reproduction)

To reproduce the +8V and +22V outputs with CA3007H:

```
  VOUT = VREF × (1 + R2/R1) + IADJ × R2
  Where VREF = 1.25V, IADJ ≈ 50μA (neglected for large R)
  Simplified: VOUT ≈ 1.25V × (1 + R2/R1)

  +8V REGULATOR (Reg 1, ADJ1 pin):
  ─────────────────────────────────
  R1 = 240 Ω (standard)
  R2 = R1 × (VOUT/VREF - 1)
    = 240 × (8.0/1.25 - 1)
    = 240 × 5.4
    = 1296 Ω → use 1.3kΩ standard

  +22V REGULATOR (Reg 2, ADJ2 pin):
  ─────────────────────────────────
  R1 = 240 Ω (standard)
  R2 = R1 × (VOUT/VREF - 1)
    = 240 × (22.0/1.25 - 1)
    = 240 × 16.6
    = 3984 Ω → use 4.0kΩ or 3.9kΩ standard
```

### Upgrade Options

| Upgrade | Description | Impact on Emulation |
|---------|-------------|-------------------|
| Low-ESR capacitors | Replace originals with low-ESR electrolytics | Reduced ripple — more ideal regulation |
| Schottky rectifier upgrade | Replace rectifier diodes with Schottky | Lower forward voltage, slightly higher output |
| Precision resistors | Use 1% resistors for voltage setting | Tighter voltage regulation, less drift |
| Heatsink upgrade | Add external heatsink to TO-3 case | Reduced thermal drift, lower effective RθJC |
| Full SMPS replacement | Replace entire linear supply with switched-mode | Eliminates regulator entirely — significant emulation impact |

### Emulation-relevant replacement notes

| Factor | OEM CA3007H | LM317×2 Upgrade |
|--------|-------------|-----------------|
| Output voltage accuracy | ±2% | ±1% |
| Ripple rejection | 65 dB | 60 dB |
| Thermal drift | Built-in (same die) | Independent die (slightly different drift) |
| Dropout voltage | ~2V typ | ~2V typ |
| Current limit foldback | Integrated | Handled separately per regulator |

---

## Emulation Notes

### Power Modeling Essentials for CA3007H

When emulating the Atari ST 520ST/520STM power supply, the CA3007H contributes these key behaviors:

| Feature | Modeling Approach |
|---------|-----------------|
| **Voltage regulation** | Fixed output (8V and 22V) when VIN exceeds dropout; pass-through when VIN < VOUT + Vdropout |
| **Dropout behavior** | Regulator passes input voltage directly when VIN < VOUT + Vdropout; model this as a 2V minimum differential |
| **Current limiting** | Clamp output current to 1.5A per regulator; model foldback (current decreases near short circuit) |
| **Ripple rejection** | ~65dB ripple rejection means output ripple is ~1/1778 of input ripple on the regulated output |
| **Thermal response** | Model junction temperature: TJ = Tambient + P × Rθ; if TJ > 150°C, regulation fails until cooling |
| **Input capacitance** | Large electrolytic capacitor (1000μF @ 25V/35V) on each regulator input — stores energy, provides ripple buffer |
| **Output capacitance** | 100μF + 0.1μF per output — smoothing and transient response |
| **ADJ pin filtering** | 10μF + 0.1μF on each ADJ pin — critical for ripple rejection; removing this cap dramatically increases output ripple |

### Thermal Modeling

```
  THERMAL MODEL (per regulator channel in single chip):

  P dissipated = (VIN - VOUT) × IOUT

  Junction temp = Tambient + P × Rθtotal
    Rθtotal = RθJC + RθCS + RθSA

  Thermal shutdown threshold: TJ = 150°C
  Thermal recovery threshold: TJ = 130°C

  If TJ > 150°C:
    - Regulation is suspended
    - Output voltage drops toward zero
    - No current limiting (open circuit)
  If TJ drops below 130°C:
    - Regulation resumes automatically
    - Output voltage returned to set value
```

### Transient Response Modeling

| Condition | Response |
|-----------|----------|
| Load step (0 → 1A) | Output dips by ~10-50mV, recovers in ~10-100ms (dominated by output capacitor discharge/recharge) |
| Load step (1A → 0) | Output rises by ~10-50mV, recovers similarly |
| Input voltage sag | Output follows until VIN < VOUT + 2V, then output begins to drop |
| Input voltage recovery | Output returns to set value after ~10ms (input capacitor recharge time) |

### Key Modeling Equations

```
  OUTPUT VOLTAGE:
  VOUT = 1.25V × (1 + R2/R1) + IADJ × R2
  (with IADJ variation ±50μA)
  Set tolerance: ±2% = ±0.16V at 8V output

  RIPPLE REJECTION:
  Vripple_out = Vripple_in × 10^(-65/20)
              = Vripple_in / 1778

  CURRENT LIMIT (foldback):
  I_LIMIT(ILOAD) = IMAX when ILOAD < IMAX
                 = IMAX × (VOUT / VSET) when near short
                 ≈ IMAX × (VIN - VDROP) / VIN when dropping

  THERMAL:
  TJ = TA + P × (RθJC + RθCS + RθSA)
  where RθJC = 2.5°C/W, RθCS ≈ 0.5°C/W, RθSA ≈ 2.3°C/W
```

### Power Supply Ripple Propagation

```
  520ST EXTERNAL BRICK RIPPLE PROPAGATION

  Transformer Secondary    Bridge Rectifier     C Input (1000μF)
  ripple ~ 100mV          → ripple ~ 200mVpp   → ripple ~ 10mVpp
  (50/60Hz)               (full wave)          (after big cap filter)
                                                                        
        │                                                       
        ▼                                                       
  CA3007H Regulator (ripple rejection ~65dB)                     
                                                                        
        │                                                       
        ▼                                                       
  +8V Output ripple ~ 5uVpp      +22V Output ripple ~ 5uVpp   
  (negligible)                    (negligible)                  
                                                                
  NOTE: Actual measured ripple on original hardware is typically
  50-200uV on +8V and +22V — dominated by external PCB trace
  resistance and load current fluctuations.
```

### Important Emulation Details

1. **The CA3007H is NOT a switched-mode regulator** — it is a linear series pass regulator. It works by dissipating excess voltage as heat, so efficiency = VOUT / VIN. For the +8V rail, efficiency ≈ 8V/12V = ~67%. For the +22V rail, efficiency ≈ 22V/27V = ~81%.

2. **The unregulated +V rail (floppy motor) is NOT part of the CA3007H** — it comes directly from the transformer winding through a separate rectifier path, with no regulation.

3. **The 520ST+ (enhanced model) uses the same PSU** as the 520ST — the CA3007H remains unchanged.

4. **The 520STM RF modulator variant** uses the same CA3007H in the same external brick — the RF circuitry draws power from the +22V rail.

5. **If emulating the internal SMPS models (1040STF, etc.)** — the CA3007H is NOT used. Regulation is done via the SMPS controller IC (typically an NE555 or dedicated PWM controller) feeding into the linear regulators on the motherboard, which may use LM317 or the MC34063 family.

6. **Capacitor aging is significant** in original hardware — electrolytic capacitors in the 520ST external brick degrade over decades, increasing ESR and reducing capacitance. This causes:
   - Increased output ripple
   - Poorer ripple rejection
   - Potential regulator instability

7. **The TO-3 metal case serves as a heatsink** — the CA3007H relies on thermal contact with a metal plate inside the external brick for heat dissipation. In emulation, this manifests as a fixed thermal environment rather than free-air conditions.

### Reference Schematic (CA3007H Typical Application — Atari ST Configuration)

```
  CA3007H TYPICAL APPLICATION (Atari ST 520ST PSU Layout)

                      R1 (240Ω)
              +------+-----/\/\/\/-----+
              |                          |
   +12V unreg |                          |
   (from rect)│                          ├────────────────▶ +8V OUT (to motherboard)
   C1 (1000μF)│                          |
   25V         │                R2 (1.3kΩ)|
   C2 (100μF)  │                          |
   25V          ├─── Pin 1 ─── OUT1 ───┘
   C3 (0.1μF)  │
   50V           ├─── Pin 2 ─── ADJ1 ──┤
   C4 (10μF)    │                      ├───▶ 0V (reference ground)
   25V          ├─── Pin 3 ─── GND1 ──┤
   50V           │
                ┌┴────────────────────┤
                │  CA3007H             │
                │  (TO-3 Package)      │
                │                      │
                │                     ┌┴┘
  +10V unreg    │   C6 (1000μF)       |
  (from rect)   │   35V               |
  C5 (1000μF)   ├─── Pin 8 ─── IN2 ──┼--- To transformer winding
  35V           |                      |
  C7 (100μF)    │                      |
  35V          ├─── Pin 5 ─── OUT2 ────┤
  C8 (0.1μF)   ├─── Pin 10 ─ SLUG ────┤
  50V           │                      ├───▶ 0V (reference ground)
                │   ┌─ Pin 6 ── ADJ2   |
  Pin 9 = NC    │   │                  |
  (not used)    │   R3 (240Ω)          R4 (4.0kΩ)
                │   ───/\/\/\/────┘     /\/\/\/
                │                     ───▶ 0V

  NOTE: This is a representative schematic based on the CA3007H
  two-regulator architecture and the Atari ST power supply
  specifications. Exact resistor values may vary by manufacturing
  batch and regional variant.
```

---

## References

- [Atari ST Internals, ch. 1.9 - Power Supply](https://www.atarimania.com/documents/Atari-ST-Internals.pdf) — Atari ST PSU specifications and pinouts
- [Signetics CA3007 Dual Adjustable Regulator Datasheet](https://www.datasheetarchive.com/signetics+voltage+regulator-datasheet.html) — Original CA3007H datasheet archive
- [Console5 - Atari ST Cap Lists](https://wiki.console5.com/wiki/Atari_ST) — Component lists for Atari ST power supply
- [The LaST Upgrade - Atari PSU repair](https://www.exxosforum.co.uk/atari/last/psu/index.htm) — PSU repair and modification guide
- [SidecarTridge PSU](https://docs.sidecartridge.com/sidecartridge-psu/) — Modern ATARI ST PSU replacement information
- [Atari Age Forums - PSU Discussion](https://forums.atariage.com/topic/359755-modern-power-supply-to-replace-the-bricks-of-old/) — Community PSU replacement discussion
- [jepSTone.net - Atari ST Power Supplies](https://jepstone.net/atarist/power-supply/2022/06/02/atari-st-power-supplies.html) — PSU variants analysis
