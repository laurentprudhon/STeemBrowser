# Atari ST Floppy Disk Hardware Technical Analysis

> **Source**: [info-coach.fr/atari/hardware/FD-Hard.php](https://info-coach.fr/atari/hardware/FD-Hard.php)  
> **Last Updated**: April 7, 2017  
> **Purpose**: Comprehensive technical documentation of Atari ST floppy disk hardware for emulation, preservation, and development.

---

## Table of Contents
1. [General Floppy Drives Information](#1-general-floppy-drives-information)
   - [Drive Information](#drive-information)
   - [Floppy Drive Read/Write Heads](#floppy-drive-readwrite-heads)
   - [Disk Storage Basics](#disk-storage-basics)
   - [Reading Data from the Floppy Disk](#reading-data-from-the-floppy-disk)
   - [Data Separator](#data-separator)
   - [Writing Data to the Floppy Disk](#writing-data-to-the-floppy-disk)
   - [Sense, Amplification, and Conversion Circuits](#sense-amplification-and-conversion-circuits)
   - [No Flux Area on Disk](#no-flux-area-on-disk)
2. [Floppy Disk Encoding/Decoding](#2-floppy-disk-encodingdecoding)
   - [Technical Requirements for Encoding and Decoding](#technical-requirements-for-encoding-and-decoding)
   - [Frequency Modulation (FM)](#frequency-modulation-fm)
   - [Modified Frequency Modulation (MFM)](#modified-frequency-modulation-mfm)
3. [Western Digital 1772 FDC Information](#3-western-digital-1772-fdc-information)
   - [WD1772 PLL Data Separator](#wd1772-pll-data-separator)
   - [Detail on the Inspection Window](#detail-on-the-inspection-window)
   - [Detection of Fuzzy Bits by the WD1772](#detection-of-fuzzy-bits-by-the-wd1772)
   - [FDC Address Mark Detector](#fdc-address-mark-detector)
   - [FDC CRC Computation](#fdc-crc-computation)

---

## 1. General Floppy Drives Information

### Drive Information

#### Physical Composition
- **Disk Media**: Thin sheet of **Mylar plastic** coated with **gamma iron oxide** (Fe₂O₃).
- **Drive Components**:
  - **Read/Write Head**: Electromagnetic transducer for data conversion.
  - **Positioning Control System**: Moves head to desired track.
  - **Associated Electronics**: Amplifiers, pulse detectors, and controller interfaces.

#### Track and Sector Structure
- **Tracks**: Thin annular regions on each disk side.
  - **Double-Density 3.5" Disk**: **80 tracks per side**.
- **Sectors**: Subdivisions of tracks where data and identification information are stored.
- **Soft-Sectored Disks**: Use an **index pulse** to mark the start of a track (no physical sector markers).

#### Operational Parameters
- **Seek Operation**: Process of moving the head to the desired track.
- **Transfer Rate**: Speed at which data is written to or read from the disk.
- **Access Time**: Time required to locate a specific sector, composed of:
  - **Seek Time**: Time to position the head over the correct track.
  - **Latency Time**: Time for the desired sector to rotate under the head.

#### Index Pulse Detection
- **3.5" Drives**: Use the **Hall Effect** (magnetic sensor) instead of optical sensors (used in 5.25" drives).
- **Mechanism**:
  - Disk is locked via a pin fitting into a dip near the center.
  - Magnetic coating on the spindle motor generates pulses detected by a sensor as the motor rotates.

---

### Floppy Drive Read/Write Heads

#### Function and Principles
- **Energy Conversion**: Transform electrical signals to magnetic signals (writing) and vice versa (reading).
- **Electromagnetic Principles**:
  - **Writing**: Current through a coil produces a magnetic field (direction depends on current flow).
  - **Reading**: Changing magnetic field induces a current in the coil (Faraday's Law).

#### Head Design
- **Ferrite Heads**: Iron core with wire wrapped around it to form a controllable electromagnet.
- **Contact Recording Technology**:
  - Heads **directly contact** the disk media (unlike hard disks, which use floating heads).
  - **Rationale**: Flexible media (Mylar) cannot maintain consistent floating head gaps.
  - **Speed**: Floppy disks spin at **300–360 RPM** (vs. 3600+ RPM for hard disks), minimizing wear.
  - **Maintenance**: Heads require periodic cleaning due to oxide and dirt buildup.

#### Erase Heads
- **Purpose**: Prevent interference between tracks by erasing stray magnetic information.
- **Types**:
  - **Tunnel-Erase Heads**: Positioned behind and to each side of the read/write head to erase outside the defined track.
  - **Straddle Erasure**: R/W and erasure heads perform recording and erasing simultaneously. The erasure head trims the top/bottom fringes of flux reversals to reduce cross-talk.

#### Double-Sided Disks
- **Configuration**: Two heads (one per side) squeeze the media between them.
- **Compatibility**: Heads vary slightly based on **drive format and density**.

---

### Disk Storage Basics

#### Magnetic Writing
- **Principle**: Current flowing through a coil produces a magnetic field confined in a ring-shaped core.
- **Mechanism**:
  - A narrow slot in the core focuses the field to magnetize the disk surface.
  - Creates alternating **north-south magnetic domains** on the disk coating.

#### Magnetic Reading
- **Principle**: Voltage is induced in a coil by a changing magnetic field (Faraday's Law).
- **Mechanism**:
  - Uniform magnetic field → No voltage (steady state).
  - **Flux Reversal** (change in magnetization) → Rapid field change → Voltage pulse.
- **Signal Conversion**: Analog signal from pulses is converted back to digital data.

#### Head Material
- **Core Composition**: Ceramic with spherical ferrite particles.
- **Contact Impact**: Direct contact with disk surface increases error rates and wear compared to hard disks.

---

### Reading Data from the Floppy Disk

#### Process Flow
1. **Command Issuance**: System sends a read command specifying track and sector.
2. **Seek Operation**: Head moves to the desired track.
3. **Sector Identification**: Desired sector is located via header ID segment; fields are checked per formatting rules.
4. **Flux Detection**: Read/write head records **flux reversals**.
5. **Amplification**: Signal is amplified by the **read/write amplifier** (series of alternating polarity pulses).
6. **Pulse Detection**: **Pulse Detector** replicates the time position of signal peaks (flux reversals).
   - Output: TTL-compatible signal (positive leading edge = signal peak).
7. **Encoding Handling**: Raw data consists of **composite clock and data bits** (depends on encoding scheme: FM or MFM).
8. **Synchronization & Decoding**: **Data Separator** synchronizes and decodes the jittery bit stream.
9. **Parallel Conversion**: FDC's **deserializer** converts serial data to parallel bytes.
10. **Byte Boundary Recognition**: In soft-sectored drives, a **"missing clock" signal** sets byte boundaries.
11. **Data Transfer**: Data is stored in a temporary register in the FDC and transferred to system memory via **DMA (Direct Memory Access)**.

---

### Data Separator

#### Purpose
- Synchronizes and decodes the **jittery bit stream** from the Pulse Detector.

#### Phase Locked Loop (PLL)
- **Function**: Locks onto the bit stream to synchronize it.
- **Soft-Sectored Drives**: Controller does not wait for the index pulse before lock-on (head may not be over a preamble field).
- **Bit Positioning**: Determines nominal positions of clock and data bits, then generates a **clock and data window** centered around these positions.

#### Bit Jitter and Shifting
- **Causes of Bit Shift**:
  - **Motor Speed Variation (MSV)**: Fluctuations in disk rotation speed.
  - **Instantaneous Speed Variation (ISV)**: Short-term speed changes.
  - **Inter-Symbol Interference**: Overlap of closely spaced pulses.
- **PLL Behavior**:
  - Tracks frequency changes but **ignores jitter** to avoid erroneous data.
  - Dynamically adjusts the **inspection window** to compensate for bit shifts.
- **Error Tolerance**: If bit jitter exceeds tolerance, erroneous data may be issued.

#### Data Window Resolution
- **Impact**: Tighter window resolution → Lower **soft error rate**.
- **Phase Adjustment**: PLL uses the phase relationship between a bit and its window to adjust the window's position.

---

### Writing Data to the Floppy Disk

#### Process Flow
1. **Seek Command**: Positions the head over the desired track/sector.
2. **Data Transfer**: System transfers data to the FDC via **DMA**.
3. **Serialization**: FDC's **serializer** converts parallel data to serial data.
4. **Encoding**: FDC provides **NRZ or MFM encoded data** to the drive.
5. **Flux Generation**: Read/write head generates **flux changes** in the media.

#### Bit Shift and Compensation
- **Cause of Bit Shift**:
  - Current change in the read/write head is **not instantaneous** (takes time to peak and return to zero).
  - If flux transitions are close together, the signal buildup from one transition does not reach zero before the next begins.
  - **Result**: Peaks are shifted; narrower bit spacing → Greater shift (especially on inner tracks).

- **Compensation Methods**:
  - **Pre-Compensation**: Bits are deliberately shifted in the **opposite direction** of the expected shift.
    - Controller detects bit patterns and calculates which bits will shift.
  - **Post-Compensation**: Applied during reading to correct predictable bit shifts.
- **Track-Specific Compensation**:
  - **Inner Tracks**: Require compensation to minimize bit shift.
  - **Outer Tracks**: No compensation needed (shift is negligible).

---

### Sense, Amplification, and Conversion Circuits

#### Purpose
- Amplify and interpret weak signals from disk heads to determine if each signal is a **1 or 0**.

#### Signal Characteristics
- **Weak Signals**: Require amplification before processing.
- **Pulse Interaction**: Data pulses near one another **interact**, causing overlap and partial cancellation.

#### Pulse Behavior
- **Ideal Pulses**:
  - Alternate in polarity (positive → negative → positive).
  - Return to center (zero) between pulses.
- **Actual Pulses**:
  - **Rounded** and **broader** than ideal.
  - Exhibit **overshoot** near the end.
  - **Pulse Breadth**: Causes **data pattern sensitivity** (closely spaced pulses of opposite polarity overlap and partially cancel).

#### Worst-Case Pulse Sequence
- **Isolated Pulses (A and B)**:
  - Read with **maximum amplitude** (no nearby pulses to interfere).
  - Trick the drive's **Automatic Gain Control (AGC)** into minimizing gain.
- **Sandwiched Pulse (D)**:
  - Between two pulses of opposite polarity (C and E).
  - **Pulled upward** by neighbors, reducing amplitude.
- **AGC Impact**:
  - First two isolated pulses set AGC to **minimum gain** (maximum amplitude).
  - Next three pulses test the disk surface's magnetic strength.

---

### No Flux Area on Disk

#### Definition
- A **copy protection mechanism** resulting in **no flux transitions** from the data read circuitry.

#### Misconceptions
- **Strong Erasure Theory**:
  - Originally thought to involve erasing disk sectors with a strong magnetic field.
  - **Impracticalities**:
    - Cannot be done with normal read/write head/circuitry.
    - Would require a permanent magnet over desired areas.
    - Would cause **AGC to reach maximum amplification**, likely resulting in random noise fluxes.

#### Actual Implementation
- **Mechanism**: Write flux transitions **close enough to violate encoding rules**, causing the current to **never return to zero**.
- **Result**: No data is detected by the read channel, even though the AGC remains "happy" (still receives flux transitions).
- **Hardware Support**: Used by devices like the **Kryoflux board** for disk imaging and preservation.

#### Bit Shift Exploitation
- **NRZ Media**: Bit shift occurs due to normal read/write head operation.
- **Flux Transition Overlap**: If transitions are written close together (ignoring encoding rules), the current never returns to zero → **No data detected**.

---

## 2. Floppy Disk Encoding/Decoding

### Technical Requirements for Encoding and Decoding

#### Challenges with Direct Encoding
1. **Fields vs. Reversals**:
   - Read/write heads measure **flux reversals** (not absolute polarity) because reversals are easier to detect.
   - **Rationale**: Voltage spikes from reversals are critical for high-density storage.
   - **Implication**: Encoding must be based on **flux reversals**, not absolute fields.

2. **Synchronization**:
   - Need a method to indicate **bit boundaries** (where one bit ends and another begins).
   - **Example**: Encoding 1000 consecutive zeros would make bit boundaries indistinguishable without synchronization.

3. **Field Separation**:
   - Magnetic fields are **additive**; aligning 1000 small fields would create one large field.
   - **Implication**: Impossible to distinguish individual bits without separation.

#### Solutions
- Encode using **flux reversals** (not absolute fields).
- Limit consecutive fields of the same polarity.
- Add **clock synchronization** to the encoding sequence (e.g., markers or milestones).

#### Recording Density
- **Linear Density Limit**: Each inch of track can store only a limited number of flux reversals.
- **Clock Overhead**: Some reversals are used for clock synchronization, reducing those available for data.
- **Goal**: Minimize flux reversals used for clocking to maximize data storage.

#### Density Improvements
- **Hardware Density**: More bits stored by allowing more flux reversals per inch.
- **Encoding Density**: More bits stored by encoding more bits per flux reversal.

---

### Frequency Modulation (FM)

#### Definition
- First common encoding system for digital data on magnetic media.

#### Encoding Scheme
| Bit | Encoding Pattern | Flux Reversals | Description                          |
|-----|-------------------|----------------|--------------------------------------|
| 0   | RN                | 1              | Reversal at start (clock), none mid-bit. |
| 1   | RR                | 2              | Reversal at start (clock) + mid-bit. |

- **Alternative View**:
  - Flux reversal at the **start of each bit** (clock).
  - **Additional reversal in the middle** for a 1; omitted for a 0.

#### Flux Reversals per Bit
| Scenario               | Reversals/Bit | Probability (Random Data) |
|------------------------|---------------|----------------------------|
| All Zeros (Best Case)  | 1             | 50%                        |
| All Ones (Worst Case)  | 2             | 50%                        |
| **Weighted Average**   | **1.5**       | **100%**                   |

#### Frequency Modulation Rationale
- Ones have **double the frequency of reversals** compared to zeros.
- **Example Patterns**:
  - Byte of zeros: `RNRNRNRNRNRNRNRN`.
  - Byte of ones: `RRRRRRRRRRRRRRRR`.

#### Drawbacks
- **Wasteful**: Each bit requires **two flux reversal positions** (one for clock, one for data).
- **Usage**: Early **single-density floppy disks** (late 1970s/early 1980s).
- **Obsolescence**: Replaced by **MFM** before the IBM PC era.

---

### Modified Frequency Modulation (MFM)

#### Definition
- Refinement of FM that **reduces clock flux reversals**.

#### Encoding Scheme
| Bit Pattern       | Encoding Pattern | Flux Reversals | Probability (Random Data) |
|-------------------|-------------------|----------------|----------------------------|
| 0 (preceded by 0) | RN                | 1              | 25%                        |
| 0 (preceded by 1) | NN                | 0              | 25%                        |
| 1                 | NR                | 1              | 50%                        |
| **Weighted Avg**  |                   | **0.75**       | **100%**                   |

- **Rules**:
  - Clock reversal inserted **only between consecutive zeros**.
  - For a **1**: No additional clock reversal needed (mid-bit reversal already exists).
  - For a **0 preceded by 1**: No additional reversal needed (recent reversal exists).
  - Only **long strings of zeros** require clock reversals to break them up.

#### Advantages
- **Efficiency**: **Doubles storage capacity** compared to FM for the same area density.
- **Clock Frequency**: Can be **doubled** (vs. FM), enabling higher data rates.
- **Complexity**: Slightly more complex encoding/decoding circuits, but the trade-off is worthwhile.

#### Usage
- **Standard**: **Double-density floppy disks** (Atari ST, most 1980s systems).
- **RLL Equivalence**: Sometimes called **1,3 RLL** (Run-Length Limited), as pauses between pulses range from 1 to 3.
- **Legacy**: Still used for floppy disks today (unlike hard disks, which moved to more efficient RLL methods).

---

## 3. Western Digital 1772 FDC Information

### WD1772 PLL Data Separator
- **Overview**:
  - The WD1772 includes an **internal Digital Phase-Locked Loop (DPLL) data separator** to separate clock bits from data bits.
  - The DPLL locks onto incoming serial data and must handle **frequency variations**.
  - Once locked, the FDC receives a **synchronized clock** (Inspection Window) to sample serial data.

#### PLL Core Components and Implementation
- **Physical Goal**: Align an internal oscillator with incoming transition timing.
- **3 Core Hardware Blocks**:
  1. **Phase Detector**:
     - Compares incoming signal edges with local clock edges.
     - Outputs **UP** (clock too slow) or **DOWN** (clock too fast) signals.
     - In WD1772: Implemented via **digital logic (flip-flops, XOR gates)**.
  2. **Loop Filter**:
     - Smooths error signals to remove jitter/noise.
     - Turns spikes into stable control values.
     - In WD1772: **Digital accumulator** (no analog components).
  3. **Voltage-Controlled Oscillator (VCO)**:
     - Generates a clock whose frequency depends on control voltage.
     - In WD1772: **Numerically Controlled Oscillator (NCO)** (digital implementation).

- **Closed-Loop System**:
  ```
  Input Signal → Phase Detector → Loop Filter → VCO/NCO → Output Clock
                                                     ↑__________________|
  ```
  - **Feedback Loop**: Continuously adjusts to maintain alignment.

#### Charge-Pump PLL (WD1772 Implementation)
- **Mechanism**:
  - **UP Signal**: Injects current into a capacitor (or increments digital accumulator).
  - **DOWN Signal**: Drains current (or decrements accumulator).
  - **Control Voltage**: Naturally rises or falls based on UP/DOWN balance.
- **Advantages for Floppy Disks**:
  - **Fast Locking**: Achieves lock in **~8 sync bytes (64 bit times)**.
  - **Jitter Immunity**: Narrow loop bandwidth filters out high-frequency jitter.
  - **Stability**: Controlled damping prevents overshoot during frequency steps.

#### Inspection Window
- **Purpose**: Used to **internally sample serial data**.
- **Mechanism**:
  - One state samples the **data bit** of a cell.
  - Alternate state samples the **clock bit**.
- **Dynamic Adjustment**:
  - **Frequency Correction**: Adjusts window period based on input data frequency history.
  - **Phase Correction**: Adjusts window start/stop times proportionally to deviation from center.

- **Frequency Variations**:
  - Arise from:
    - **Motor Rotation Speed Variation (MSV)**: Long-term speed fluctuations.
    - **Instantaneous Speed Variation (ISV)**: Short-term speed changes.
  - The DPLL must track these fluctuations for reliable reads.

- **Key DPLL Parameters**:
  1. **Jitter Tolerance**:
     - **Definition**: System's immunity to phase impulses.
     - **Measurement**: Maximum readable bit shift / (1/4 bit-cell distance), expressed as a percentage of the theoretical inspection window.
     - **WD1772 Value**: **~10% of bit cell width** (empirically measured).

  2. **Lock Time**:
     - **Definition**: Time for DPLL to achieve phase lock with incoming data.
     - **Typical Value**: **64-bit times (8 sync bytes)**.
     - **Assumption**: Sync field jitter ≤5% of the bit cell.
     - **WD1772 Behavior**: Achieves lock within **2–3 disk rotations** (60–90ms at 300 RPM).

  3. **Capture Range**:
     - **Definition**: Maximum frequency range over which the DPLL acquires phase lock.
     - **Components**: Drive motor speed error + ISV.
     - **Frequency Impact**: Higher frequencies reduce allowed ISV magnitude.
     - **WD1772 Performance**:
       - **MFM Encoding**: **±10%** (225–275 KHz for 250 KHz nominal).
       - **FM Encoding**: **±100%** (due to simpler encoding and wider transitions).

- **WD1772 PLL Capabilities**:
  - **Documentation Gap**: WD1772 datasheet does **not** specify jitter tolerance, lock time, or capture range.
  - **Practical Performance**:
    - **IBM Standard**: Allows **<2% rotation speed deviation** → PLL tolerates **4% frequency variation**.
    - **Bit Cell Frequency Range (MFM)**: **225–275 KHz** (3.4µs to 4.4µs cell width).

- **DPLL Algorithm**:
  - **Basis**: Likely implements the algorithm from **US Patent 4,870,844** (used in many 1980s FDCs).
  - **Inspection Window Properties**:
    - Duration **proportional to input data frequency**.
    - Start/stop times adjustable to center subsequent data bits in the **middle of the window**.
  - **Corrections**:
    - **Frequency Correction**: Compensates for **motor speed unsteadiness** (frequency drift).
    - **Phase Correction**: Compensates for **magnetic reversal migration** (phase drift).
  - **Balancing**:
    - **Phase vs. Frequency Correction Ratio**: Carefully tuned for **fast settling** (quick lock) and **stability** (no overshoot).
    - **Excessive Phase Correction**: Speeds up settling but increases noise sensitivity.
    - **Excessive Frequency Correction**: Can destabilize the loop (cause oscillations).

#### Handling Missing Data and Long Zero Sequences
- **Challenge**: Floppy data is messy; PLL must:
  - Lock quickly (within **8 sync bytes**).
  - Ignore jitter (filter high-frequency noise).
  - Not drift during long zero sequences (maintain lock without transitions).
- **Solutions in WD1772**:
  - **Narrow Loop Bandwidth**: Filters out jitter but may reduce capture range.
  - **Controlled Damping**: Avoids overshoot during sudden frequency changes.
  - **MFM Encoding**: Guarantees transitions at least every **2 bit cells** (4µs at 250 KHz), preventing PLL unlock.

#### PLL State Machine (WD1772 Digital Implementation)
- **States**:
  1. **Search**: Looking for sync pattern (e.g., `$A1` or `$C2`).
  2. **Lock**: Adjusting frequency/phase to center transitions in inspection windows.
  3. **Track**: Maintaining lock during data read.
- **Transition Handling**:
  - **Normal Transitions**: Centered in inspection windows → No correction.
  - **Early Transitions**: Detected in first half of window → **Phase advance** (speed up VCO).
  - **Late Transitions**: Detected in second half of window → **Phase delay** (slow down VCO).
  - **Missing Transitions**: No pulse detected → **Hold last state** (rely on frequency correction).

#### Why PLL is Essential for Floppy Disks
- **No Separate Clock Track**: Unlike hard disks, floppies have no dedicated clock track.
- **Disk Speed Variations**: Motor speed is **not perfectly constant** (typical variation: **±1–2%** for good drives, up to **±5%** for worn drives).
- **Data Timing Distortions**:
  - **Bit Shift**: Transitions move due to inter-symbol interference.
  - **Jitter**: Random timing variations from media noise.
- **Result**: PLL enables **reliable recovery of timing from noisy magnetic data**.

#### Intuition: PLL as a Musician
- **Analogy**:
  - **Input Transitions**: Drummer's beats (imperfect timing).
  - **VCO**: Musician's internal tempo.
  - **Phase Detector**: Compares drummer's beats to musician's tempo.
  - **Loop Filter**: Smooths corrections to avoid erratic adjustments.
- **Outcome**: Musician stays synchronized even with imperfect beats.

---

### MFM Encoding: Detailed Technical Explanation

#### Core Principle
- **Goal**: Store data while guaranteeing enough transitions for the PLL to recover the clock.
- **Key Idea**: Data is **not stored as voltage levels**, but as **timing between magnetic transitions** (flux reversals).

#### MFM Encoding Rules (Stateful System)
- **Misconception Clarification**:
  - MFM is **not** a simple per-bit-pair rule (e.g., `00 → no clock`).
  - It is a **stateful timing system** with constraints to avoid long gaps between transitions.

- **Correct Rules**:
  1. **Data Transition**:
     - If the data bit is **1**, there is **always a transition** at the bit boundary.
     - If the data bit is **0**, there is **no transition** at the bit boundary.
  2. **Clock Transition**:
     - Inserted **only if needed** to prevent long gaps between transitions.
     - **Never allowed**: More than **3 bit cells (6µs at 250 KHz)** without a transition (1,3 RLL constraint).
     - **Practical Rule**: A clock transition is inserted **unless the previous and current bits are both 0** (`P=0, D=0`).

- **Bit Cell Composition**:
  - Each bit cell can contain:
    - A **clock transition** (optional, at the start of the cell).
    - A **data transition** (if bit = 1, at the boundary).
  - **Result**: A bit cell may have **0, 1, or 2 transitions** (but never none for long periods).

#### Step-by-Step MFM Encoding Example
- **Data**: `1 0 0 1 0`
- **Assumption**: Previous bit (`P`) = `0` (initial state).

| Step | Current Bit (D) | Previous Bit (P) | Clock Transition? | Data Transition? | Transitions in Cell | Cumulative Transitions |
|------|-----------------|------------------|--------------------|-------------------|----------------------|------------------------|
| 1    | 1               | 0                | ✔ Yes              | ✔ Yes             | 2                    | `C+D`                 |
| 2    | 0               | 1                | ✔ Yes              | ❌ No              | 1                    | `C+D, C`              |
| 3    | 0               | 0                | ❌ No               | ❌ No              | 0                    | `C+D, C`              |
| 4    | 1               | 0                | ✔ Yes              | ✔ Yes             | 2                    | `C+D, C, C+D`        |
| 5    | 0               | 1                | ✔ Yes              | ❌ No              | 1                    | `C+D, C, C+D, C`     |

- **Final Encoded Pattern**: `C+D, C, (none), C+D, C`
  - `C` = Clock transition.
  - `D` = Data transition (only for bit = 1).

#### Why MFM Works for Long Zero Sequences
- **Problem with FM**: `00000000` → No transitions → PLL loses lock.
- **MFM Solution**: Even long runs of zeros still contain **clock transitions** to maintain timing.
  - **Example**: `0 0 0 0 0 0 0` → `C, (none), C, (none), C` (transitions every 2 bit cells).
  - **Result**: PLL stays synchronized because transitions occur **at least every 3 bit cells** (1,3 RLL rule).

#### MFM as a Timing System with Constraints
- **Intuition**:
  - **Data Bits**: Actual music notes.
  - **Clock Transitions**: Metronome beats embedded into the music.
  - **MFM Guarantee**: Even if the music is "silent" (all zeros), the metronome still ticks often enough to keep time.

- **Key Insight**:
  - MFM **minimizes transitions** (for higher density) but **enforces a maximum gap** between them (for PLL lock).
  - **Efficiency**: ~2× better than FM (fewer clock transitions → more data per track).

#### MFM Decoding Process
1. **PLL Lock**:
   - PLL locks onto the **sync pattern** (e.g., `$A1 $A1 $A1 $FE` for IDAM).
   - Sync pattern provides **initial frequency/phase reference**.
2. **Transition Detection**:
   - Each flux reversal (transition) is detected and timed relative to the PLL's clock.
3. **Bit Cell Sampling**:
   - **Inspection Windows**: PLL generates windows to sample data and clock bits.
   - **Data Bit**: Determined by presence/absence of a transition at the **bit boundary**.
   - **Clock Bit**: Determined by presence/absence of a transition at the **clock position**.
4. **Byte Assembly**:
   - Serial bits are assembled into bytes using the **missing clock** signal (for byte synchronization).
5. **Error Handling**:
   - **Jitter**: Small timing variations are filtered by the PLL.
   - **Bit Shift**: Compensated by dynamic adjustment of inspection windows.

#### MFM vs. FM Encoding
| Feature               | FM (Frequency Modulation) | MFM (Modified Frequency Modulation) |
|-----------------------|----------------------------|----------------------------------------|
| **Clock Transition** | Always at start of bit     | Only when needed (not for `00`)        |
| **Data Transition**  | Mid-bit for 1              | At boundary for 1                      |
| **Transitions/Bit**   | 1.5 (avg)                  | 0.75 (avg)                             |
| **Density**           | Single-density            | Double-density                         |
| **PLL Requirement**   | Low (always has clock)    | Higher (must handle variable transitions) |
| **Complexity**        | Simple                    | Stateful                              |

---

### Bit Writing and Pre-Compensation

#### Write Pre-Compensation in WD1772
- **Purpose**: Counteract **bit shift** caused by magnetic interactions and head physics.
- **Bit Shift Causes**:
  1. **Head Field Rise/Fall Time**: Write current does not change instantaneously.
  2. **Inter-Symbol Interference**: Closely spaced transitions overlap and shift.
  3. **Media Demagnetizing Fields**: Neighboring transitions repel each other.

- **Effect of Bit Shift**:
  - **Inner Tracks**: Higher bit density → **greater shift** (up to **0.5 bit cells** without compensation).
  - **Outer Tracks**: Lower bit density → **negligible shift**.

#### Pre-Compensation Mechanism
- **Principle**: Deliberately shift write pulses in the **opposite direction** of the expected bit shift.
- **Implementation in WD1772**:
  - **Bit Pattern Detection**: FDC monitors the **previous 2–3 bits** to predict shift.
  - **Compensation Rules**:
    | Previous Bits | Current Bit | Expected Shift | Pre-Compensation |
    |---------------|--------------|----------------|------------------|
    | `1 1`         | 1            | +0.5 bit       | -0.5 bit         |
    | `1 0`         | 1            | +0.25 bit      | -0.25 bit        |
    | `0 1`         | 1            | +0.25 bit      | -0.25 bit        |
    | `0 0`         | 1            | 0              | 0                |
  - **Timing Adjustment**: Write pulses are **advanced or delayed** by **0–150 ns** (depending on bit density).

- **Compensation Granularity**:
  - **WD1772**: Supports **4 levels of pre-compensation** (0, 1, 2, 3).
  - **Adjustment per Level**: ~**50 ns** (at 250 KHz bit rate).

#### Post-Compensation (Reading)
- **Purpose**: Correct for **residual bit shift** not handled by pre-compensation.
- **Implementation**:
  - **Dynamic Window Adjustment**: PLL adjusts inspection window **position and width** based on detected bit positions.
  - **History-Based**: Uses the **last 3 transitions** to predict and correct for shift.

#### Compensation by Track
- **Track-Dependent Compensation**:
  - **Outer Tracks (0–40)**: Minimal compensation (bit shift <0.1 bit cells).
  - **Middle Tracks (41–60)**: Moderate compensation (~0.2–0.3 bit cells).
  - **Inner Tracks (61–83)**: Maximum compensation (~0.4–0.5 bit cells).
- **WD1772 Implementation**:
  - **Track Register**: FDC uses the **current track number** to adjust compensation level.
  - **Automatic Adjustment**: Compensation increases **linearly with track number**.

#### Compensation and Copy Protection
- **Exploiting Compensation**:
  - Some copy protection schemes **disable pre-compensation** to create **controlled bit shift**.
  - **Result**: Data appears **unreadable** on standard drives but works on drives with specific compensation characteristics.
- **Example**: **Copylock** protection writes data with **no pre-compensation**, causing bit shift on inner tracks.
  - **WD1772 Behavior**: Can still read due to **wide capture range (±10%)** and **dynamic PLL adjustment**.

#### Compensation in Emulation (Steem SSE)
- **Implementation**:
  - **Pre-Compensation Table**: Lookup table for bit patterns and track numbers.
  - **Dynamic Adjustment**: Emulates WD1772's **4-level compensation** based on track and bit history.
  - **PLL Simulation**: Models inspection window adjustments to handle residual shift.

---

### Data/Clock Decoding and Synchronization

#### Byte Synchronization
- **Challenge**: Identify the **start of a byte** in the serial bit stream.
- **Solution**: Use **sync bytes** and **address marks** with **missing clock bits**.

#### Sync Bytes in MFM
- **Purpose**: Provide a **fixed reference point** for byte alignment.
- **MFM Sync Bytes**:
  - **`$A1`**: Used before **IDAM (ID Address Mark)** and **DAM (Data Address Mark)**.
  - **`$C2`**: Used before **IAM (Index Address Mark)**.
- **Encoding**:
  - Sync bytes **violate MFM rules** by omitting clock transitions in specific positions.
  - **Example**: `$A1` (`10100001`) is encoded with **missing clock bits at positions 4 and 5**.

#### Address Mark Detection
- **Process**:
  1. **Preamble**: Long string of zeros (e.g., `00 00 00`) to allow PLL to lock.
  2. **Sync Bytes**: Three consecutive sync bytes (e.g., `$A1 $A1 $A1`).
  3. **Address Mark**: Special byte (e.g., `$FE` for IDAM, `$FB` for DAM).
- **WD1772 Behavior**:
  - **Sync Mark Detector**: Always active during **Read Track** commands.
  - **False Sync Handling**: May detect sync patterns inside data blocks (exploited in copy protection).

#### Missing Clock Bits
- **Purpose**: Create a **unique pattern** that cannot occur in normal data.
- **Mechanism**:
  - Normal MFM: Clock transitions are inserted **only when needed** (not for `00`).
  - Sync Bytes: **Force absence of clock transitions** in specific positions.
- **Result**:
  - **Byte Boundary Detection**: Missing clock bits indicate the start of a sync byte.
  - **PLL Behavior**: Missing transitions cause **phase errors**, but PLL recovers due to subsequent transitions.

#### Data Separator Workflow
1. **Input**: Serial bit stream from read amplifier (pulse detector).
2. **PLL Lock**: DPLL locks onto the bit stream using sync pattern.
3. **Inspection Windows**:
   - **Data Window**: Samples bit at **bit boundary** (determines data bit value).
   - **Clock Window**: Samples bit at **clock position** (determines if clock transition occurred).
4. **Bit Assembly**:
   - **Data Bit**: `1` if transition detected in data window; `0` otherwise.
   - **Clock Bit**: `1` if transition detected in clock window; `0` otherwise.
5. **Byte Formation**:
   - **Deserializer**: Converts serial bits to parallel bytes.
   - **Byte Boundary**: Detected via **missing clock** signal (sync pattern).
6. **Output**: Parallel data bytes sent to FDC data register.

#### Handling Jitter and Bit Shift
- **Jitter Sources**:
  - **Media Noise**: Variations in particle size/orientation.
  - **Electronic Noise**: Amplifier and head circuitry.
  - **Mechanical Runout**: Disk wobble or spindle eccentricity.
- **PLL Response**:
  - **Jitter Tolerance**: WD1772 PLL can handle **±10% bit cell jitter** (empirically measured).
  - **Dynamic Adjustment**: Inspection windows are **continuously adjusted** to center transitions.

#### False Sync Detection
- **Cause**: Sync byte patterns (e.g., `$C2`) can appear **inside data blocks**.
- **WD1772 Behavior**:
  - If a false sync is detected, the FDC **shifts subsequent bits**, causing **data corruption**.
  - **Exploit**: Copy protection schemes **hide data** in false sync patterns.
- **Mitigation**:
  - **`$A1` Sync Byte**: No known false sync patterns (safer for IDAM/DAM).
  - **`$C2` Sync Byte**: Known to false-trigger; used only for IAM.

---

### Practical PLL Implementation in WD1772

#### Digital PLL Architecture
- **Components**:
  1. **Phase Comparator**: Digital logic (XOR gates, flip-flops) to compare input and VCO edges.
  2. **Loop Filter**: Digital accumulator (16-bit) to smooth error signals.
  3. **Numerically Controlled Oscillator (NCO)**:
     - **Frequency Control**: 16-bit divider to adjust clock period.
     - **Resolution**: **~1 part in 65,536** (sufficient for floppy disk timing).

#### Inspection Window Generation
- **Mechanism**:
  - **Period**: Adjusted every **2µs** (half a bit cell at 250 KHz).
  - **Width**: **1µs** (centered on expected transition time).
  - **Adjustment**:
    - **Frequency**: Based on **history of last 3 transitions** (expands/contracts window period).
    - **Phase**: Based on **deviation from window center** (shifts window earlier/later).

#### PLL Lock Process
1. **Sync Detection**: FDC detects **3 consecutive sync bytes** (e.g., `$A1 $A1 $A1`).
2. **Initial Lock**:
   - **Frequency**: Set to **nominal bit rate** (250 KHz for MFM).
   - **Phase**: Aligned to **first sync byte transition**.
3. **Fine Adjustment**:
   - **First 8 Bytes**: PLL adjusts frequency/phase to center transitions in windows.
   - **Lock Time**: **~64 bit times (8 bytes)** for stable lock.
4. **Tracking**:
   - **Continuous Adjustment**: PLL refines frequency/phase during data read.
   - **Jitter Filtering**: Ignores high-frequency deviations (noise).

#### PLL Unlock Conditions
- **Long Gap Without Transitions**:
  - **MFM**: Maximum gap = **3 bit cells** (6µs at 250 KHz).
  - **Result**: PLL **holds last frequency/phase** but may drift.
- **Sudden Frequency Change**:
  - **Threshold**: >**±10%** for MFM (WD1772 limit).
  - **Result**: PLL **loses lock** and must re-acquire sync.

#### PLL Recovery
- **Automatic Re-Lock**:
  - WD1772 **continuously searches** for sync patterns during Read Track.
  - **Recovery Time**: **<1 rotation** (200ms at 300 RPM) for valid sync.
- **Error Handling**:
  - **No Sync Found**: FDC sets **CRC error** or **overrun** status.
  - **Partial Data**: Returns **available data** with error flags.

---

### Summary of Key Concepts

#### PLL as a Feedback System
- **Input**: Magnetic transitions (timing information).
- **Output**: Synchronized clock (inspection windows).
- **Feedback**: Error signals adjust VCO to minimize phase/frequency difference.

#### MFM as a Timing System
- **Data**: Encoded as presence/absence of transitions at bit boundaries.
- **Clock**: Encoded as transitions at clock positions (when needed).
- **Constraint**: Maximum gap of **3 bit cells** between transitions (1,3 RLL).

#### Compensation as Shift Correction
- **Pre-Compensation**: Shift write pulses **opposite** to expected bit shift.
- **Post-Compensation**: Adjust read windows to **follow** actual bit positions.
- **Track-Dependent**: Compensation increases for **inner tracks** (higher density).

#### WD1772 Strengths
- **Wide Capture Range**: ±10% for MFM, ±100% for FM.
- **Fast Lock Time**: 8 sync bytes (64 bit times).
- **Jitter Tolerance**: ±10% of bit cell width.
- **Stateful PLL**: Handles missing transitions and long zero sequences.

---

### Detail on the Inspection Window

#### MFM Data Encoding
- **Nominal Transition Spacing (Double-Density MFM)**:
  - **4µs** (1 bit cell).
  - **6µs** (1.5 bit cells).
  - **8µs** (2 bit cells).

#### Data Input Circuit
- **Function**: Converts data pulses into bits and stores them in the **Data Shift Register (DSR)**.
- **Inspection Window Timing**:
  - Repeats every **2µs** (half a cell size).
  - A **1** is stored if a pulse is detected during a window; otherwise, a **0** is stored.

#### Dynamic Adjustment
- **Frequency Correction**:
  - Period of inspection windows is **gradually adjusted** to compensate for input data frequency shifts.
  - Based on the **history of the last three flux transitions**.
- **Phase Correction**:
  - Start/stop times of inspection windows are adjusted proportionally to the deviation of the last detected pulse from the window's center.
- **Balancing**:
  - **Phase vs. Frequency Correction**: Carefully balanced for **fast settling** and **stability**.
  - **Excessive Phase Correction**: Faster settling but more noise-sensitive.
  - **Excessive Frequency Correction**: Can destabilize the loop.

#### Practical Performance
- **Frequency Variation Tolerance**: Up to **9%** (corroborated by WD1772 measurements).
- **Protection Compatibility**: Exceeds variations used by **Copylock** and **Macrodos** (typically <5%), ensuring correct reads.

---

### Detection of Fuzzy Bits by the WD1772

#### Definition
- **Fuzzy Bits**: Created by placing flux reversals at the **border of the inspection window**.
- **Usage**: Copy protection (e.g., **Dungeon Master** game; see **US Patent 4,849,836**).

#### Mechanism
- **Border Transitions**: If a flux transition occurs **near the border** of an inspection window (uncertain area), it may be detected in **either the current or next window** based on:
  - Small variations in **drive rotation speed** between read-sector commands.
- **Result**: **Pseudo-random values** (fuzzy bits) are returned.

#### Example
- **Transition Timing**: A transition **5µs** from the previous one can be interpreted as:
  - **4µs** (current window).
  - **6µs** (next window).
- **Dependency**: Interpretation depends on **frequency fluctuations** in rotation speed.

#### DPLL Role
- **Automatic Adjustment**:
  - DPLL adjusts the **frequency and phase** of inspection windows to compensate for:
    - **~10% drive speed variation**.
  - Ensures:
    - **Normal transitions** are **centered** in the inspection window.
    - **Marginal transitions** (fuzzy bits) are **perfectly at the border**.

#### Compensating Pairs
- **Usage**: Fuzzy bits are often used in **pairs** to minimize impact on inspection window frequency/phase:
  - **First Transition**: Placed at the **beginning** of the inspection window.
  - **Second Transition**: Placed at the **end** of the inspection window.
- **Result**: Frequency and phase of inspection windows are **almost unaffected**.

---

### FDC Address Mark Detector

#### Purpose
- Recognize **address marks** in the data stream.
- Establish **byte synchronization** (non-ambiguous start of a byte in the bit stream).

---

##### FM Address Marks (Not Used by Atari ST)
- **Mechanism**: Synchronization via **missing clock bits** (normal bytes use an `FF` clock pattern).

| Address Mark               | Data Pattern | Clock Pattern |
|----------------------------|--------------|----------------|
| Index Address Mark (IAM)   | `FC`         | `D7`          |
| ID Address Mark (IDAM)     | `FE`         | `C7`          |
| Data Address Mark (DAM)    | `FB`         | `C7`          |
| Deleted Data Address Mark  | `F8`         | `C7`          |

---

##### MFM Address Marks (Used by Atari ST)

###### Challenge
- In MFM, clock bits are **only added for two consecutive zeros**, making it impossible to differentiate clock and data bits in arbitrary sequences.

###### Synchronization Method
1. **Preamble**:
   - Long string of zeros at the start of each **ID and DATA field**.
   - Provides time for the DPLL to **adjust frequency** and **center the inspection window**.
   - **Critical for DATA fields**: Due to **write splice** (when the head starts re-writing a data field, causing slight rotational speed variations).

2. **Sync Bytes**:
   - Special bytes that **violate MFM encoding rules** by omitting a clock bit in a sequence of zeros.
   - **Sync Byte Values**: `$A1` or `$C2`.
   - **Sequence**: **Three consecutive Sync Bytes** followed by an **Address Mark** (IAM, IDAM, or DAM).

###### Sync Bytes and Address Marks for Atari ST (WD1772)

| Address Mark               | Data Pattern | Clock Pattern | Missing Clock Bits | Resulting Bit Sequence |
|----------------------------|--------------|----------------|--------------------|-------------------------|
| Sync Byte (before IDAM/DAM)| `$A1`        | `$0A`          | 4 & 5              | `0100010010001001` (`$4489`) |
| Sync Byte (before IAM)     | `$C2`        | `$14`          | 3 & 4              | `0101001000100100` (`$5224`) |
| Index Address Mark (IAM)   | `$FC`        | -              | -                  | -                       |
| ID Address Mark (IDAM)     | `$FE`        | -              | -                  | -                       |
| Data Address Mark (DAM)    | `$FB`        | -              | -                  | -                       |
| Deleted Data Address Mark | `$F8`        | -              | -                  | -                       |

- **Bit Order**: **MSB to LSB** (as sent), with the first bit numbered **0**.
- **RLL Compliance**: Encoding **does not violate 1,3 RLL rules** (no sequence of 4 consecutive zeros), so **false sync patterns** can occur in the bit stream.

###### FDC Behavior
- `$A1` sync byte is produced by sending `$F5` to the FDC.
- `$C2` sync byte is produced by sending `$F6` to the FDC.
- **2-byte CRC** is written by sending `$F7`.
- `$C2` is typically used **only before an IAM** and is rare in standard Atari diskettes.
- An IAM on a track (e.g., formatted on a PC) is **acceptable** on Atari.

---

##### MFM Sync Byte Pattern

###### Visual Representation
- **$A1 with Missing Clock**:
  - Flux reversals depicted as ideal pulses with a **missing clock bit between bits 4 and 5**.
- **Normal $A1**:
  - Standard MFM encoding without missing clock bits.
- **$C2 with Missing Clock**:
  - Flux reversals with a **missing clock bit between bits 3 and 4**.
- **Normal $C2**:
  - Standard MFM encoding without missing clock bits.

---

##### False Sync Byte Pattern

###### Issue
- During a **Read Track** command, the WD1772's **Sync Mark Detector** is **always active**, leading to potential **false sync detection**.

###### Cause
- The **`$C2` sync mark** was **not chosen wisely**, as its pattern can appear **inside data blocks**.
- **Example**: The bit sequence for `$029` can match the `$C2` sync byte pattern (shifted by half a cell).

###### Consequence
- If a data block contains a sequence matching `$C2` with a missing clock, the FDC will:
  1. Synchronize on this pattern.
  2. Shift subsequent bits, resulting in **incorrect data interpretation**.

###### Protection Mechanism
- This "feature" can be exploited to **hide information** inside data blocks.
- Used in **copy protection schemes** (e.g., see [Atari Copy Protection Document](http://info-coach.fr/atari/documents/_mydoc/Atari-Copy-Protection.pdf)).

###### Known Sequences
- Many sequences match `$C2`, but **no known sequences match `$A1`** (though this does not guarantee none exist).

---

### FDC CRC Computation

#### Purpose
- Ensure **data integrity** by detecting errors in read/write operations.

#### CRC Polynomial
- **Standard**: **CCITT CRC-16**.
- **Polynomial**: `G(x) = x¹⁶ + x¹² + x⁵ + 1`.

#### Implementation
- Typically implemented using **Linear Feedback Shift Registers (LFSRs)**.
- LFSRs use **XOR operations** to simulate subtraction without carry.

#### CRC Generation Process
1. **Initialization**: All flip-flops in the register are set to **1** (`$FFFF`).
2. **Data Processing**: Bytes are processed **MSB first** (as recorded on floppies).
3. **Final CRC**: After processing all data bits, the register contains the **16-bit CRC**.
4. **CRC Verification**: To check CRC, the same process is applied (including CRC bytes). If all bits are **0** afterward, the data is error-free.

#### CRC Recording
- The CRC is written to the floppy **MSB first** (big-endian).
- **Example**: For a typical ID record (`$A1, $A1, $A1, $FE, $00, $00, $03, $02`), the CRC is **`$AC0D`** (`$AC` first, `$0D` second).

#### CRC Initialization Notes
- **Ambiguity in Documentation**: WD1772 datasheet mentions that in MFM, the CRC is initialized by receipt of a `$F5` byte but does **not** specify the initialization value.
- **Verified Behavior**:
  - **Method 1**: Initialize CRC to **`$FFFF`** on the **first `$A1`** and treat subsequent `$A1` bytes as normal. **Works correctly**.
  - **Method 2 (Ijor)**: Initialize CRC to **`$C4DB`** each time a `$A1` is received. **Also works**.
  - **Emulator Note**: Both methods work for standard sequences of 3 `$A1` bytes before an Address Mark (AM).
- **`$C2` Sync Byte**:
  - If `$C2` is used as a sync byte, the **CRC is not reset**.
  - **Example**: Writing `$C2, $C2, $C2, $FE, $00, $00, $03, $02` is read correctly by the FDC but results in a **wrong CRC** (exploited in protection schemes).

---

#### Example of CRC Code

##### Basic CRC-CCITT Computation (Simon Owen)
```c
for (int i = 0; i < 8; i++)
   crc = (crc << 1) ^ ((((crc >> 8) ^ (b << i)) & 0x0080) ? 0x1021 : 0);
```

##### Lookup Table Method (Faster)
```c
crc = (crc << 8) ^ crc_ccitt[((crc >> 8) & 0xff) ^ b];
```

##### Lookup Table Generation
```c
for (int i = 0; i < 256; i++) {
   WORD w = i << 8;
   for (int j = 0; j < 8; j++)
      w = (w << 1) ^ ((w & 0x8000) ? 0x1021 : 0);
   crc_ccitt[i] = w;
}
```

##### Notes
- The **lookup table method** is **significantly faster** for bulk computations.
- The WD1772 uses **CCITT-CRC-16**, which is widely documented and standardized.

---

## Magnetic Particle Behavior on Disk Surface

### Magnetic Domain Theory
- **Ferromagnetic Materials**: The gamma iron oxide (Fe₂O₃) coating on floppy disks consists of **ferromagnetic particles** embedded in a binder on the Mylar substrate.
- **Domain Formation**:
  - Each particle is a **single-domain** magnet (typically 0.1–0.5 µm in size).
  - Particles are **needle-shaped** (acicular) to maintain stable magnetization along their long axis.
  - **Coercivity**: Resistance to demagnetization (measured in Oersteds). Atari ST disks typically use **300–600 Oe** coercivity oxide.

### Magnetization Process (Writing)
- **Head Field Strength**: The write head generates a magnetic field of **~500–1000 Oe** (sufficient to overcome coercivity).
- **Flux Reversal Mechanism**:
  1. **Saturation**: As the head passes, its magnetic field **saturates** the particles in its vicinity.
  2. **Transition Region**: The boundary between opposite polarities forms a **magnetic transition** (not an abrupt change).
  3. **Transition Width**: Typically **0.5–1.5 µm** for floppy disks (wider than hard disks due to lower coercivity and larger particles).
- **Self-Demagnetizing Fields**:
  - After writing, **demagnetizing fields** from neighboring particles cause the transition to **widen slightly** (self-demag effect).
  - This contributes to **bit shift** (especially on inner tracks with higher bit density).

### Readback Process (Reading)
- **Inductive Reading**:
  - The read head (same as write head in floppy drives) detects **changes in magnetic flux** (dΦ/dt) as it passes over transitions.
  - **Pulse Generation**: A voltage pulse is induced only when the head crosses a **transition** (not in uniform fields).
- **Pulse Shape**:
  - **Ideal Pulse**: Symmetric peak with amplitude proportional to transition density.
  - **Real Pulse**: Asymmetric due to:
    - **Head Geometry**: Gap length and core material affect pulse width.
    - **Media Thickness**: Thicker coatings (e.g., 3–5 µm for floppies) cause **pulse broadening**.
    - **Speed Effects**: At 300 RPM, the head moves at **~1.5 m/s** (linear velocity at middle track), affecting pulse duration.

### Interparticle Interactions
- **Exchange Coupling**:
  - Adjacent particles interact via **magnetic exchange forces**, causing **clustering** of domains.
  - This leads to **non-linear transition shifts** when bits are packed closely.
- **Demagnetizing Fields**:
  - Fields from neighboring transitions **oppose** the main transition, causing:
    - **Peak Shift**: Transitions appear to move toward each other (compression effect).
    - **Amplitude Reduction**: Closely spaced transitions partially cancel (as seen in [Sense, Amplification, and Conversion Circuits](#sense-amplification-and-conversion-circuits)).
- **Superposition**:
  - The readback signal is a **linear superposition** of pulses from individual transitions.
  - For MFM encoding, the minimum distance between transitions is **2 bit cells** (4µs at 250 KHz), reducing interference.

### Thermal Effects
- **Temperature Dependence**:
  - Coercivity **decreases with temperature** (~0.5% per °C).
  - Atari ST drives operate at **20–60°C**, causing slight variations in write/read behavior.
- **Thermal Decay**:
  - Over time, magnetic domains can **randomly flip** due to thermal energy (especially in low-coercivity media).
  - **Retentivity**: Floppy disks retain data for **10–30 years** under ideal conditions (cool, dry, away from magnetic fields).

### Wear and Degradation
- **Head-Disk Contact**:
  - **Friction**: Direct contact causes **abrasive wear** on both head and disk.
  - **Lubrication**: Disk surfaces are coated with a **thin lubricant layer** (e.g., fatty acids) to reduce friction.
- **Oxide Shedding**:
  - Magnetic oxide particles can **detach** from the disk surface, accumulating on the head (**clogging**).
  - This causes **dropouts** (missing pulses) and **increased error rates**.
- **Head Contamination**:
  - Dust, smoke, or fingerprints on the disk surface can **insulate** the head from the magnetic layer, reducing signal amplitude.

### Bit Shift and Non-Linear Effects
- **Transition Crowding**:
  - On inner tracks (higher bit density), transitions are **physically closer**, increasing demagnetizing fields.
  - **Result**: **Bit shift** of up to **0.5 bit cells** (2µs at 250 KHz) without compensation.
- **Non-Linear Bit Shift**:
  - Shift magnitude depends on:
    - **Bit Pattern**: Sequences like `101010` exhibit **less shift** than `11110000`.
    - **Track Position**: Inner tracks (higher density) show **more shift** than outer tracks.
    - **Media Age**: Older disks with degraded oxide exhibit **increased shift** due to reduced coercivity.
- **Compensation in WD1772**:
  - The FDC's **pre-compensation** circuit advances or delays write pulses based on the **previous bit pattern** to counteract expected shift.

### Magnetic Hysteresis
- **Hysteresis Loop**:
  - The magnetic material exhibits a **non-linear response** to the write field.
  - **Remanence (Br)**: Residual magnetization after write field removal (~1000–2000 Gauss for gamma Fe₂O₃).
  - **Coercive Force (Hc)**: Field required to demagnetize (~300–600 Oe).
- **Write Process**:
  - The write head must generate a field **> Hc** to switch particle magnetization.
  - **Partial Switching**: If the field is near Hc, some particles may not fully switch, causing **fuzzy transitions** (exploited in copy protection).

### Domain Wall Motion
- **Wall Movement**:
  - During writing, **domain walls** (boundaries between magnetic regions) move to align with the head field.
  - **Velocity**: Domain walls move at **~100 m/s** in gamma Fe₂O₃ under typical write fields.
- **Barkhausen Noise**:
  - Discrete jumps in domain wall motion cause **microscopic fluctuations** in the readback signal (contributes to **jitter**).

### Signal-to-Noise Ratio (SNR)
- **Sources of Noise**:
  - **Media Noise**: Variations in particle size, shape, and orientation.
  - **Electronic Noise**: Amplifier and head circuitry thermal noise.
  - **Triboelectric Noise**: Static charges from head-disk contact.
- **SNR for Floppies**: Typically **20–30 dB** (lower than hard disks due to contact recording and larger particles).
- **Impact on Readability**:
  - Low SNR limits the **minimum detectable pulse amplitude** (affects fuzzy bit detection).
  - WD1772's **AGC (Automatic Gain Control)** dynamically adjusts amplification to maintain signal integrity.

### Copy Protection Exploits
- **Weak Bits**:
  - Writing with **reduced current** (near Hc) creates transitions that are **marginally detectable**.
  - Small variations in drive speed or head position cause these bits to **flip randomly** (fuzzy bits).
- **No-Flux Areas**:
  - By writing **overlapping transitions** (violating encoding rules), the readback signal can be made to **cancel out completely** (no detectable pulses).
  - Exploits the **linear superposition** of magnetic pulses.
- **Non-Standard Encodings**:
  - Using **FM encoding on DD disks** or **custom bit patterns** can create signals that confuse the FDC's PLL.

## References
- [The Floppy User Guide](http://info-coach.fr/atari/hardware/_fd-hard/floppy-ug.pdf) (Michael Haardt, Alain Knaff, David C. Niemi)
- [The Technology of Magnetic Disk Storage](http://info-coach.fr/atari/hardware/_fd-hard/tech-disks.txt) (Steve Gibson)
- [Western Digital WD1772 Datasheet](http://info-coach.fr/atari/documents/_mydoc/WD1772-JLG.pdf)
- [WD177x Programming Information](http://info-coach.fr/atari/documents/_datasheets/WD177x-Prog.txt)
- [Phase-Locked Loop Tutorial](http://info-coach.fr/atari/hardware/_fd-hard/pll-tut-talk.pdf) (Danny Abramovitch)
- [Atari Copy Protection Document](http://info-coach.fr/atari/documents/_mydoc/Atari-Copy-Protection.pdf)
- [US Patent 4,849,836 - Copy Protection for Computer Disc](http://info-coach.fr/atari/documents/general/patents/US4849836.pdf)
- [US Patent 4,870,844 - Digital Phase Lock Loop](http://info-coach.fr/atari/documents/general/patents/US4870844.pdf)

---

## Appendix: Glossary of Terms
| Term | Definition |
|------|------------|
| **AGC** | Automatic Gain Control: Adjusts signal amplification based on input strength. |
| **AM** | Address Mark: Special byte sequence marking the start of a sector or track. |
| **DAM** | Data Address Mark: Marks the start of a data field. |
| **DPLL** | Digital Phase-Locked Loop: Synchronizes clock and data bits in the FDC. |
| **DSR** | Data Shift Register: Temporary storage for serial data in the FDC. |
| **FM** | Frequency Modulation: Early floppy disk encoding scheme. |
| **FDC** | Floppy Disk Controller: Manages read/write operations (e.g., WD1772). |
| **IDAM** | ID Address Mark: Marks the start of an ID field. |
| **IAM** | Index Address Mark: Marks the start of a track. |
| **ISV** | Instantaneous Speed Variation: Short-term fluctuations in disk rotation speed. |
| **MFM** | Modified Frequency Modulation: Improved encoding scheme for double-density disks. |
| **MSV** | Motor Speed Variation: Long-term fluctuations in disk rotation speed. |
| **NRZ** | Non-Return to Zero: Encoding scheme where bits are represented by signal levels. |
| **PLL** | Phase-Locked Loop: Circuit for synchronizing clock signals. |
| **RLL** | Run-Length Limited: Encoding scheme limiting the number of consecutive zeros. |
| **TTL** | Transistor-Transistor Logic: Digital logic standard for signal levels. |
