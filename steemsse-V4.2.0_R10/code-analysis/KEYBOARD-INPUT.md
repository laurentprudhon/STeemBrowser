# Keyboard Input Analysis (Atari ST)

This document describes the signal chain and logic for keyboard input in the Atari ST, from the physical keyboard matrix to the CPU.

## 1. Keyboard Matrix Architecture

The Atari ST keyboard uses a matrix architecture consisting of rows and columns to reduce the number of I/O pins required to detect key presses.

- **Rows**: Mapped to the **DR1** register of the HD6301 (IKBD) microcontroller.
- **Columns**: Mapped to **DR3** (bits 1-7) and **DR4** (bits 0-7).
- **Structure**: The matrix is approximately 8 rows by 15 columns.
- **Components**: The matrix uses diodes to prevent "ghosting" (where multiple key presses might be interpreted as a different key).

## 2. Process of Scanning the Keyboard Matrix

The keyboard scanning is performed autonomously by the **HD6301 (IKBD)** microcontroller, not the main CPU.

### Signal Flow & Timing
1. **Column Activation**: The HD6301 writes to the column registers (**DR3** and **DR4**), setting specific bits to activate a column.
2. **Row Reading**: The HD6301 reads the row register (**DR1**). 
3. **Detection**: 
   - By default, row bits in **DR1** are pulled high. 
   - If a key is pressed in the active column, it completes a circuit to ground, pulling the corresponding bit in **DR1** low.
4. **Iteration**: The HD6301 iterates through all columns and rows rapidly to detect all currently pressed keys.

## 3. Key Press Detection and CPU Communication

Once the HD6301 detects a key press, it translates the matrix coordinates into a **scancode**.

### Detection to CPU Chain
1. **Scancode Generation**: The HD6301 looks up the scancode for the detected row/column pair in its internal ROM.
2. **Serialization**: The scancode is placed in the HD6301's **TDR** (Transmit Data Register).
3. **Serial Link**: The HD6301 sends the scancode serially to the mainboard at a rate of **7812.5 baud** using 10-bit frames.
4. **ACIA Reception**: The **MC6850 ACIA** (Asynchronous Communications Interface Adapter) on the motherboard receives the serial bitstream and reconstructs the byte in its **RDR** (Receive Data Register).
5. **CPU Notification**: The ACIA triggers an interrupt (IRQ) via the **MFP** (Master Peripheral Interface) to notify the MC68000 CPU that data is available.

## 4. Role of the CIA/ACIA Chips

While the prompt mentions "CIA", the Atari ST specifically uses the **MC6850 ACIA**.

- **Serial Interface**: Acts as the bridge between the HD6301's serial output and the CPU's parallel bus.
- **Buffering**: Uses double-buffering (**TDRS/TDR** and **RDRS/RDR**) to handle the asynchronous nature of the serial link.
- **Interrupt Generation**: Sets the `IRQ` bit in the Status Register (SR) when a byte is received (`RDRF`) or when an overrun occurs (`OVRN`), which is then passed to the MFP.
- **Timing**: The ACIA clock is derived from the system clock ( typically 8MHz / 16 = 500kHz).

## 5. Mapping and Scancodes

The HD6301 converts matrix coordinates to 8-bit scancodes.

- **Make Codes**: Sent when a key is first pressed (e.g., `0x1E` for 'A').
- **Break Codes**: Sent when a key is released. The break code is typically the make code ORed with `0x80` (e.g., `0x9E` for 'A' release).
- **Special Keys**: Some keys generate specific sequences or are handled by the HD6301 firmware (e.g., auto-repeat logic).

## 6. CPU Interaction with ACIA Registers

The MC68000 reads keyboard data by interacting with the memory-mapped registers of the Keyboard ACIA.

- **Memory Map**: 
  - `$FFFC00`: ACIA Control/Status Register (SR/CR)
  - `$FFFC02`: ACIA Data Register (DR)
- **Read Process**:
  1. The CPU checks the **Status Register (`$FFFC00`)** for the `RDRF` (Receive Data Register Full) bit (Bit 0).
  2. If `RDRF` is set, the CPU reads the scancode from the **Data Register (`$FFFC02`)**.
  3. Reading the Data Register automatically clears the `RDRF` bit and may clear any pending `OVRN` (Overrun) flags.
- **Overruns**: If the CPU does not read the data register before a second byte arrives, the ACIA sets the `OVRN` bit in the Status Register.
