# Atari ST Reset Button Functionality Analysis

This document provides a detailed technical analysis of the reset button functionality on the Atari ST, tracing the signal path from the physical button to the CPU and its subsequent effect on the system state.

## 1. The Electrical Path of the Reset Signal

The physical reset button on an Atari ST is connected to the system's reset circuitry. While the emulator focuses on the logical result, the hardware signal path is as follows:

- **Signal Polarity**: The reset signal in the Atari ST is **active low**. When the reset button is pressed, the reset line is pulled to ground (0V).
- **Path to CPU**: The signal is routed to the `RESET` pin of the MC68000 CPU and other critical peripherals.
- **Hold Time**: To ensure a reliable reset, the signal must be held active for a sufficient duration. According to hardware notes in `reset.cpp` (line 147), the reset must be active for at least **132 clock cycles**.

## 2. Distribution of the Reset Signal

The reset signal is not exclusive to the CPU; it is distributed across the motherboard to synchronize the initial state of all major components:

- **MC68000 CPU**: Forces the CPU into a reset state, halting execution and preparing it to fetch the initial reset vectors.
- **Glue Chip**: Resets address decoding logic and video timing generators. In Steem SSE, this is implemented in `TGlue::Reset(bool Cold)` (`glue.cpp`).
- **MFP (MC68901)**: Resets the programmable interrupt controller and timers.
- **MMU**: Resets the memory configuration and bank selection.
- **Shifter**: Resets the video generation state and palette.
- **PSG (YM2149)**: Resets the sound generator.
- **FDC (WD1772)**: Resets the floppy disk controller.

## 3. Impact on CPU State

When the reset signal is released, the MC68000 CPU undergoes a specific hardware-defined initialization:

- **Registers**: The general-purpose registers are not explicitly cleared by hardware, but the CPU state is forced into a known configuration.
- **Supervisor State**: The CPU is forced into **Supervisor Mode** (S-bit = 1).
- **Interrupts**: Masked. The Interrupt Priority Level (IPL) is set to **7**, disabling all maskable interrupts.
- **Status Register (SR)**: Initialized to `0x2700`.
- **Program Counter (PC) & Stack Pointer (SP)**: The CPU fetches the initial state from the reset vectors located at the bottom of the address space:
    - **Initial SSP (Supervisor Stack Pointer)**: Fetched from address `0x000000`.
    - **Initial PC (Program Counter)**: Fetched from address `0x000004`.
- In Steem SSE, these are mirrored from the ROM into RAM during power-on to optimize emulation (`reset.cpp`:164-165).

## 4. Interaction with Glue Chip and Clock Generation

The Glue chip plays a central role in the reset process:

- **ROM Selection**: During the reset phase, the Glue chip's address decoding logic ensures that the CPU's requests for addresses `0x000000` and `0x000004` are routed to the **TOS ROM** (via the `ROMEN` signal).
- **Clock Requirement**: It is critical that the system clock is running and stable while the reset signal is asserted. The CPU requires the clock signal to properly transition out of the reset state.
- **Timing Synchronization**: The Glue chip synchronizes the transition of the rest of the system (video timing, etc.) with the CPU's exit from reset.

## 5. Post-Reset Event Sequence

Immediately after the reset signal is released, the following sequence occurs:

1. **Reset Vector Fetch**: The CPU loads the SSP and PC from `0x000000` and `0x000004`.
2. **Initial Execution**: Execution starts at the address specified by the reset vector in the TOS ROM.
3. **Peripheral Sync**: The first few instructions in the ROM typically execute a `RESET` instruction to ensure all peripherals are synchronized.
4. **Warm vs. Cold Start Detection**:
    - The system checks for "Magic Numbers" at `0x000420` and `0x00043A`.
    - If these numbers are present and valid, the system performs a **Warm Start**, skipping memory tests and attempting to restore the memory configuration.
    - Otherwise, a **Cold Start** is performed, which includes a full hardware memory test and initialization of the MMU (`0xFF8001`).
5. **Hardware Init**: The PSG is initialized, the video frequency is set (50Hz/60Hz), and the Shifter palette is initialized.
6. **OS Boot**: The system prepares the interrupt vector table and attempts to boot from the designated boot device (`_bootdev` at `0x000446`).

## 6. Model Variations

While the basic reset mechanism is consistent across the ST family:

- **ST / Mega ST**: Standard Glue chip implementation.
- **STe**: The Glue functionality is integrated into the GST MCU, but the logical reset path to the CPU remains the same.
- **TT / Falcon**: Use different CPU architectures (68030/etc.) and complex MMUs, but they maintain backward compatibility with the reset vector mechanism at the base of the address space for the boot process.
