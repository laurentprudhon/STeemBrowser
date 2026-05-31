# Sound Output Generation - Atari ST (YM2149 PSG)

## Overview

The Atari ST generates sound using the Yamaha YM2149 Programmable Sound Generator (PSG). This chip provides three independent sound channels, each capable of producing square waves (tones) and white noise, with an integrated envelope generator for amplitude modulation.

## 1. CPU to PSG Interface

### Addressing and Data Bus
The YM2149 PSG is memory-mapped into the system's I/O space. In the Atari ST, the PSG registers are located in the range `0xFF8800` to `0xFF880F`.

- **Memory Mapping**: The CPU accesses the PSG via the data bus. To minimize bus contention and adhere to hardware specifications, the Glue chip (or GSTMCU on the STE) introduces a wait state for PSG accesses.
- **Bus Interaction**: In `steem/iow.cpp:652`, a single cycle wait state is added (`BUS_WAIT_STATES(1)`) whenever the CPU writes to the `0x88` address group.
- **Data Width**: The interface is 8-bit. Although the 68000 CPU typically performs 16-bit (word) or 8-bit (byte) accesses, the PSG only responds to byte-level transactions on the data bus.

## 2. Register Access Mechanism

### Index and Data Registers
The PSG utilizes a register-select mechanism. Unlike standard memory where each address corresponds to a unique value, the PSG requires the CPU to specify which register it intends to modify or read.

- **Writing**: When the CPU writes to a PSG address, the emulator updates the `psg_reg_select` variable to track the active register.
- **Register Mapping**: There are 16 registers (`PSGR_TONE_PERIOD_A_LOW` through `PSGR_PORT_B`).
- **Write Process**: 
    1. The CPU writes a value to a specific address in the `0xFF8800-0xFF880F` range.
    2. In `steem/iow.cpp:660-676`, the emulator captures the value (`hibyte`) and updates the internal `psg_reg` array at the index defined by `psg_reg_select`.
    3. The function `psg_set_reg()` in `steem/psg.cpp` is then invoked to handle any side effects associated with the register change (e.g., updating the active frequency or volume).

### Read Process
Reading from the PSG is handled in `steem/iow.cpp:654-658`. The CPU reads from a register address, and the emulator returns the value of the register currently pointed to by `psg_reg_select`.

## 3. Frequency and Volume Control

### Tone Frequency (Square Waves)
Each of the three channels (A, B, C) has a 12-bit frequency divider (Tone Period).
- **Calculation**: The frequency $f_T$ is determined by the master clock $f_{Master}$ (typically 2MHz) and the tone period $TP$:
  $$f_T = \frac{f_{Master}}{16 \times TP}$$
- **Implementation**: Steem uses software counters. In the legacy implementation, it computes the period in samples. In the low-level implementation (`SSE_YM2149_LL`), it simulates the internal counters toggling the output state.

### Noise Generation
The PSG features a single noise generator that can be mixed into any of the three channels.
- **Mechanism**: Noise is generated using a 17-bit Linear Feedback Shift Register (LFSR).
- **Control**: The `PSGR_NOISE_PERIOD` register controls the rate at which the LFSR shifts, thereby controlling the "pitch" or timbre of the noise.

### Volume and Amplitude Modulation
Volume is controlled in two ways:
1. **Fixed Volume**: For each channel, a 4-bit value in the amplitude registers (`PSGR_AMPLITUDE_A/B/C`) selects one of 16 pre-defined volume levels from `psg_flat_volume_level`.
2. **Envelope Generation**: If enabled via the mixer register, the volume is instead controlled by the Envelope Generator.
   - **Envelope Period**: Controls the speed of the envelope.
   - **Envelope Shape**: Defines how the volume changes (attack, decay, sustain, release).
   - **Implementation**: The envelope state machine advances every `env_period` cycles, updating the current volume level based on the selected shape table (`psg_envelope_level`).

## 4. Signal Flow to Audio Output

The path from the PSG registers to the final audio sample is as follows:

1. **Mixing**: For each channel, the tone generator and noise generator are logically mixed:
   $$\text{Channel Output} = (\text{ToneOn} \mid \text{ToneDisable}) \ \& \ (\text{NoiseOn} \mid \text{NoiseDisable})$$
2. **Amplitude Application**: The mixed binary output is multiplied by the current volume (either the fixed level or the current envelope level).
3. **Summation**: The outputs of the three channels are summed into a sound buffer (`psg_channels_buf`).
4. **Anti-Aliasing**: If enabled, the resulting signal passes through an `AntiAlias` filter to remove high-frequency artifacts resulting from digital synthesis.
5. **DAC/Output**: The final digital samples are sent to the host system's audio hardware for conversion to analog sound.

## 5. Timing and Particularities

### Double Buffering of Frequency
One critical hardware detail is that tone frequency changes are **double-buffered**. When a CPU writes a new tone period, the change does not take effect immediately. Instead, the PSG continues to output the current wave until it completes its current cycle.
- In `steem/psg.cpp:141-170`, Steem emulates this by checking `psg_tone_start_time` and adjusting the update time to the end of the current wave period.

### Emulation Modes
Steem SSE implements two different PSG emulation strategies:
- **High-Level (Legacy)**: Uses mathematical approximations and precomputed tables to generate sound buffers efficiently.
- **Low-Level (`SSE_YM2149_LL`)**: Inspired by MAME, this mode simulates the PSG's internal clock cycles (250kHz driver clock) and state transitions, providing higher accuracy at a higher CPU cost.

### Clocking
The PSG is clocked at $f_{Master}/4$ (approximately 500kHz), but its internal logic for noise and envelopes depends on further divisions of this clock. Steem ensures that these timing relationships are maintained relative to the system's `a_s_t` (absolute system time).
