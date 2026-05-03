# DB25 Parallel Port

> Centronics-compatible parallel printer port on the Atari ST.

## Connector

- **DB25S** (D-sub 25-pin, female)

## Pinout

| Pin | Function | Direction | Notes |
|-----|- --|--|------|
| 1 | !Strobe | PSG IOA5 → peripheral | Active low, 3.3K pull-up to 5V |
| 2 | Data 0 | PSG IOB0 → peripheral | TTL, generated at ~4 KB/s |
| 3 | Data 1 | PSG IOB1 → peripheral | |
| 4 | Data 2 | PSG IOB2 → peripheral | |
| 5 | Data 3 | PSG IOB3 → peripheral | |
| 6 | Data 4 | PSG IOB4 → peripheral | |
| 7 | Data 5 | PSG IOB5 → peripheral | |
| 8 | Data 6 | PSG IOB6 → peripheral | |
| 9 | Data 7 | PSG IOB7 → peripheral | |
| 10 | n/c | - | Acknowledge not supported |
| 11 | Busy | peripheral → PSG IOA0 | Active high, 1K pull-up to 5V |
| 12-17 | n/c | - | |
| 18-25 | Ground | - | Signal ground |

## Operation

The parallel port is controlled by the **YM2149 PSG**:

- **Data**: PSG I/O Port B (register $FF880F) provides 8-bit parallel data
- **Strobe**: PSG I/O Port A bit 5 drives the strobe signal (active low)
- **Busy**: PSG I/O Port A bit 0 reads the Busy signal from the peripheral

### Control Flow

```
ST                          Peripheral
 |                              |
 |  PSG writes data to Port B |
 |  → Data 0-7 (DIN13 Pins 2-9) |
 |                              |
 |  PSG sets Strobe low (Pin 1) |──────────────────────────|
 |                              |                          |
 |                              |  Data latched by printer |
 |  PSG releases Strobe (high)  |                          |
 |                              |                          |
 |                              |  Busy goes high (Pin 11) |
 |<─────────────────────────────|                          |
 |  PSG reads Busy via Port A |                          |
 |                              |                          |
 |  PSG clears Strobe           |                          |
 |                              |                          |
```

### Protocol

| Step | Action | ST |
|------|------|-- |
| 1 | Write data to PSG Port B ($FF880F) | Data on pins 2-9 |
| 2 | Set Strobe = 0 via PSG Port A | Strobe (pin 1) active low |
| 3 | Wait for Busy = 1 (peripheral ready) | Poll PSG Port A bit 0 |
| 4 | Set Strobe = 1 | Strobe goes high |
| 5 | Peripheral acknowledges via pin 11 (Busy) |

### Timing

- Data transfer rate: ~4 KB/s typical
- Strobe pulse width: ≥ 0.5 us (per Centronics spec)
- Busy signal: 1K pull-up to +5V, active high

## Centronics Port Details

The Centronics 8-bit bidirectional parallel port uses the standard STROBE/DATA/HANDSHAKE handshake:

| Signal | Polarity | Timing |
|--------|----------|--------|
| Strobe | Active LOW | < 0.5 us wide |
| Data | TTL 0/5V | Valid ≥ 250 ns before strobe |
| Busy | Active HIGH | Peripheral uses to signal "not ready" |

## References

- [Atari ST Internals, ch. 1.6 - Sound Chip pins (PDF)](https://www.atarimania.com/documents/Atari-ST-Internals.pdf)
- [Centronics Parallel Interface Standard](https://www.printheadresearch.com/centronics/)
