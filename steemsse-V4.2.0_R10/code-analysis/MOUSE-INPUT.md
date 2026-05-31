# Mouse Input Analysis

This document describes the signal chain and processing logic for mouse input on the Atari ST, from the physical hardware encoders to the MC68000 CPU.

## 1. Mouse Hardware
The Atari ST mouse is a relative pointing device utilizing mechanical encoders for movement tracking.

- **Encoders**: Each axis (X and Y) is equipped with an incremental encoder.
- **Quadrature Signals**: The encoders produce two square-wave signals per axis (e.g., X+ and X-), shifted by 90 degrees (quadrature). This phase difference allows the controller to determine both the speed and the direction of movement.
- **Physical Interface**: The mouse connects via a DB9 port.
  - **Pin 1 (XA)** and **Pin 2 (XB)**: X-axis quadrature signals.
  - **Pin 3 (YA)** and **Pin 4 (YB)**: Y-axis quadrature signals.
  - **Pin 6**: Left Mouse Button (LMB).
  - **Pin 9**: Right Mouse Button (RMB).
  - **Pin 7 / Pin 8**: +5V and Ground.

## 2. Movement Tracking and Decoding
Movement tracking is performed entirely within the **IKBD (Intelligent Keyboard) controller**, powered by a Hitachi HD6301 microcontroller.

- **Quadrature Decoding**: The HD6301 monitors the phase relationship between the two signals of each axis. A transition on one signal while the other is at a specific level determines the direction of movement.
- **Pulse Counting**: Each "click" of the encoder wheel generates a pulse. The HD6301 counts these pulses to track relative displacement.
- **Resolution**: The hardware typically provides 200 events per inch (approx. 4 events/mm).

## 3. Signal Flow: Hardware to CPU
The signal path is asynchronous and tiered:

1. **Peripheral $\to$ IKBD**: High-frequency quadrature pulses enter the HD6301.
2. **IKBD Processing**: The HD6301 converts pulse counts into discrete "movement events" or "button events".
3. **IKBD $\to$ ACIA (Serial Link)**: The IKBD transmits these events as serialized bytes over a dedicated RS-232 link to the motherboard.
   - **Baud Rate**: Fixed at 7,812.5 bps.
   - **Format**: 7 data bits, 1 stop bit, even parity.
4. **ACIA $\to$ CPU**: The **MC6850 ACIA** (Asynchronous Communications Interface Adapter) receives the serial bitstream and reconstructs the byte.
   - **RDR Register**: The received byte is placed in the ACIA's Receive Data Register (RDR).
   - **Memory Map**: The CPU accesses this register via the memory-mapped address `$FFFC02`.
5. **CPU Notification**: When a byte arrives in the RDR, the ACIA signals the **MC68901 MFP** (Master Peripheral Interface), which then triggers an interrupt (IRQ) on the MC68000 CPU.

## 4. Detection of Mouse Clicks
Mouse buttons are treated as simple digital switches.

- **Polling**: The HD6301 continuously polls the state of the fire pins (Pins 6 and 9).
- **Event Generation**:
  - A transition from high to low (grounded) triggers a "Button Press" event.
  - A transition from low to high triggers a "Button Release" event.
- **Scan Codes**: These events are transmitted to the ACIA as specific bytes (e.g., `$85` for B1 press, `$86` for B1 release).

## 5. Timing and Polling Frequency
The system uses a push-pull event model rather than a synchronous polling model from the CPU's perspective.

- **IKBD Sampling**: The HD6301 samples the mouse pins at a high internal frequency (relative to the 4MHz clock) to ensure no pulses are missed.
- **Transmission Latency**: The 7.8kbps baud rate introduces a slight delay between the physical movement and the CPU's receipt of the data.
- **CPU Polling**: While the CPU can poll `$FFFC02` manually, it typically relies on the ACIA interrupt to process mouse packets immediately upon arrival.

## 6. Relation to the ACIA and System Timers
Although the system contains CIA-like functionality (the ACIA), the mouse tracking is **not** tied to the motherboard's system timers or the CIA's counters.

- **Autonomous Tracking**: The HD6301 acts as a co-processor. All quadrature decoding and pulse counting happen locally within the keyboard controller.
- **Decoupling**: The CPU does not "count pulses"; it receives "delta reports" (e.g., "Moved X+") or "absolute reports" (if configured), decoupling the CPU's clock from the mouse's physical movement speed.
- **ACIA Role**: The ACIA serves purely as a communication bridge, converting the HD6301's serial output into a byte accessible on the system data bus.
