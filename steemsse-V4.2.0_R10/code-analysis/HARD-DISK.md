# Hard Disk Drive (HDD) Emulation - ACSI Interface

## Overview

The Atari ST manages hard disk drives primarily through the ACSI (Atari Computer System Interface) bus. ACSI is a proprietary 8-bit bidirectional parallel interface derived from SCSI-1. It allows the Atari ST to communicate with hard disk controllers and other peripherals (like laser printers) using a standardized set of commands and a request/acknowledge handshake.

In Steem SSE, the ACSI interface is emulated in `steem/acsi.cpp` via the `TAcsiHdc` structure and a scheduled agenda system to simulate the asynchronous nature of hardware I/O.

## 1. The ACSI Protocol

### Physical Layer
The ACSI interface uses a custom DB19 connector. The physical bus consists of:
- **8-bit Data Bus (DATA0-DATA7)**: Bidirectional lines for transferring data and commands.
- **Control Signals**:
    - **REQ (Request)**: Asserted by the device to indicate it is ready to transfer a byte.
    - **ACK (Acknowledge)**: Asserted by the host (ST) to confirm receipt or submission of a byte.
    - **BS (Bus Select)**: Used by the host to select a specific device on the bus.
    - **SRST (System Reset)**: Resets all devices on the bus.
    - **I/O, C/D, MSG**: A 3-bit control bus that defines the current phase (Input/Output, Command/Data, or Message).

### Logical Layer
ACSI follows a host-centric model where the Atari ST always controls the bus. The communication follows a specific phase sequence:
1. **Selection**: The host selects a target device (LUN 0-7) by asserting BS.
2. **Command Phase**: The host sends a Command Descriptor Block (CDB), typically 6 bytes for ACSI or 10 bytes for SCSI-compatible devices.
3. **Data Phase**: Bidirectional transfer of data using the REQ/ACK handshake.
4. **Status Phase**: The device returns a status byte (e.g., success or error).
5. **Message Phase**: Optional exchange of messages.

## 2. Signal Chain: CPU to ACSI Controller

The signal flow from the CPU to the ACSI controller is as follows:

1. **Memory-Mapped I/O**: The CPU interacts with the ACSI controller via memory-mapped registers:
    - `$F80000`: Command Register (Write)
    - `$F80001`: Status Register (Read)
    - `$F80002`: Address/LUN Register (Write)
    - `$F80004`: Data Register (Read/Write)
2. **Command Initiation**:
    - The CPU writes the target device ID (LUN) to the address register.
    - The CPU writes the CDB to the command register.
3. **Hardware Trigger**: In Steem SSE, these writes are intercepted by `TAcsiHdc::IOWrite()`.
    - The emulator verifies the LUN and the `Ready` state of the device.
    - The CDB is buffered in `cmd_block`.
4. **Execution**: Once the full CDB is received, the controller parses the opcode (e.g., `$08` for Read, `$0A` for Write) and initiates the physical operation.

## 3. Reading Data from the HDD

Reading data involves a transition from the command phase to the data phase.

### Handshaking and Signal Flow
1. **Request**: The ACSI device prepares the requested sector and asserts **REQ**.
2. **Acknowledgement**: The host (via the DMA controller) detects REQ and asserts **ACK**.
3. **Transfer**: A byte is placed on the DATA0-DATA7 bus and transferred to the host.
4. **Repeat**: This process repeats for every byte in the sector (512 bytes).

### Steem SSE Implementation
In the emulator, this is implemented as a timed series of events:
1. **`ReadWrite()`**: Initiates the operation. It calls `Seek()` to position the virtual file pointer (`fseek`) to the correct absolute sector index.
2. **`agenda_acsi(PHASE_HDRW)`**: To simulate the timing of a real drive (~1.5 MB/s), Steem schedules a task in the agenda.
3. **Byte-by-Byte Transfer**:
    - In `PHASE_HDRW`, the emulator reads one byte from the disk image using `FREAD`.
    - It immediately calls `Dma.Drq(MNGR_ACSI)` to signal a DMA request to the system.
    - The DMA controller then moves this byte into system RAM.
4. **Completion**: Once `block_count` sectors are processed, the controller updates the status register (`STR`) and triggers an interrupt.

## 4. Writing Data to the HDD

Writing data is the inverse of the reading process.

### Handshaking and Signal Flow
1. **Request**: The host asserts **REQ**, indicating it has data ready to be written.
2. **Acknowledgement**: The device asserts **ACK**, indicating it is ready to receive the byte.
3. **Transfer**: The host places the byte on the bus, and the device captures it.
4. **Repeat**: The process continues for the duration of the sector/block.

### Steem SSE Implementation
1. **Command**: The CPU sends a Write command (`$0A`) and the target L CNBC.
2. **Execution**: `TAcsiHdc::ReadWrite()` is called, which positions the virtual disk image pointer.
3. **DMA-driven Transfer**:
    - The emulator enters `PHASE_HDRW` in the agenda.
    - It calls `Dma.Drq(MNGR_ACSI)`, signaling the DMA controller to provide data from RAM.
    - It then uses `FWRITE` to commit the byte from the data register (`DR`) to the disk image file.
4. **Verification**: If `DiskMan.bDiskProtectImage` is active, the write is bypassed.
5. **Completion**: The device alerts the CPU via the interrupt line.

## 5. Interrupt Handling

Interrupts are critical for asynchronous I/O, allowing the CPU to perform other tasks while the HDD is seeking or transferring data.

### The Interrupt Chain
1. **Device Trigger**: When a read/write operation is complete or an error occurs, the ACSI controller calls `TAcsiHdc::Irq(true)`.
2. **Global State**: This sets the global boolean `hdc_irq = true` in `emulator.cpp`.
3. **MFP Integration**: The function `update_disk_irq()` is called. It performs a logical OR of the floppy (`fdc_irq`) and hard disk (`hdc_irq`) interrupt lines:
   ```cpp
   bool disk_irq = (fdc_irq || hdc_irq);
   mfp_gpip_set_bit(MFP_GPIP_FDC_BIT, !disk_irq); // Active Low
   ```
4. **CPU Notification**: The MFP (Motorola 68901) receives this signal on its GPIP pin and asserts the interrupt line to the MC68000 CPU.
5. **ISR**: The CPU executes the Hard Disk Interrupt Service Routine (ISR), reads the ACSI status register to verify completion, and clears the interrupt.

## 6. Timing and Electrical Particularities

### Timing
ACSI timing is simulated in Steem using the `HBL_PER_SECOND` constant. A typical transfer rate is approximated at 3072 sectors per second:
- **Sector Transfer**: The delay between bytes is calculated as `HBL_PER_SECOND / 3072`.
- **Latency**: Seeking and spin-up are simplified but accounted for by the agenda system to prevent the CPU from receiving data instantaneously.

### Electrical and Logical Constraints
- **Sector Size**: Fixed at 512 bytes.
- **Bus Width**: 8-bit parallel data.
- **Active Low**: Like many Atari ST signals, the interrupt line (`MFP_GPIP_FDC_BIT`) and the ACSI control signals (BS, SRST) are active low.
- **LUNs**: Support for multiple devices is handled by the `Id` (0-7) in `TAcsiHdc`, ensuring only the device with the matching LUN responds to the BS signal.
