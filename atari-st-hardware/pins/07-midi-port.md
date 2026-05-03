# MIDI Port

> The MIDI In and MIDI Out ports are standard DIN5 connectors used for music controller interface.

## Connectors

Two **DIN5S** modules on the rear panel:

| Connector | Type | Function |
|-----------|------|----------|
| MIDI In | 5-pin DIN | Receives MIDI data |
| MIDI Out | 5-pin DIN | Transmits MIDI data |
| MIDI Thru | 5-pin DIN (optional) | Same as In (loopback) |

## MIDI Out Pinout

| Pin | Function | Notes |
|-----|- --|--|
| 1 | Through transmit data | Loopback (opto-coupled) |
| 2 | Shield ground | Pin 2 only |
| 3 | Through loop return | Optical loop |
| 4 | Out transmit data | From ACIA TX |
| 5 | Out loop return | Output loop |

## MIDI In Pinout

| Pin | Function | Notes |
|-----|- --|--|
| 1 | n/c | |
| 2 | n/c | |
| 3 | n/c | |
| 4 | In receive data | To ACIA RX |
| 5 | In loop return | Input loop |

## MIDI Protocol

| Parameter | Value |
|-- |-- |
| Baud rate | 31,250 ± 0.25% |
| Data bits | 8 |
| Stop bits | 1 |
| Parity | None |
| Encoding | RS-232 current loop |
| Signal levels | 0 = 5 mA, 1 = no current |

## MIDI Networking Modes

| Mode | Description |
|------|------|
| OMNI ON | Responds to all MIDI channel messages |
| OMNI OFF | Responds to messages on a specific channel |
| POLY | Polyphonic mode (multiple notes per channel) |
| MONO | Monophonic mode (single note per channel) |

## Through Port

The MIDI Through port provides a loopback of the MIDI In signals via an opto-coupler:
- Signals from MIDI In pin 4 pass through the opto-isolator to MIDI Out pin 1
- This allows MIDI daisy-chaining (In → Out → Next device's In)

## References

- [MIDI 1.0 Specification](https://www.midi.org/specifications-old/item/table-1-summary-of-midi-message)
- [Atari ST Internals - MIDI (PDF)](https://www.atarimania.com/documents/Atari-ST-Internals.pdf)
- [Atari ST Interfaces (DrCoolZic)](https://info-coach.fr/atari/hardware/interfaces.php)
