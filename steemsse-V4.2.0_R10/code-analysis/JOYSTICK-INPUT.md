# Joystick Input Analysis - Atari ST

This document provides a detailed technical analysis of the joystick input system in the Atari ST, as implemented in Steem SSE.

## 1. Electrical Interface
The Atari ST joystick ports use a **digital interface**. Each port is a 9-pin D-sub connector. The signals are active-low (pulled to ground when active).

- **Directions**: The stick position is determined by four digital switches (Up, Down, Left, Right). When a direction is pressed, the corresponding pin is connected to ground.
- **Fire Button**: A single digital switch for the fire button. When pressed, the pin is connected to ground.
- **Paddles**: The ST also supports analog paddles. These are read as potentiometers. In the hardware, these are converted to digital values. In Steem SSE, these are mapped to specific memory addresses ($FF9210-$FF9216) and return values based on the PC joystick axis positions.

## 2. Signal Path to the CPU
The joystick signals do not go directly to the CPU. They are processed by the **HD6301 (IKBD)** microcontroller.

1. **Joystick Ports $\to$ HD6301**: The digital signals from the joystick ports are wired to the input pins of the HD6301.
2. **HD6301 Processing**: The HD6301 polls these pins to determine the current state of the joysticks.
3. **HD6301 $\to$ ACIA $\to$ CPU**: 
   - The HD6301 communicates with the MC68000 CPU via a serial link to the **MC6850 ACIA** (Keyboard ACIA).
   - When the CPU requests joystick status or when the HD6301 is configured to send events, the state is transmitted as serialized bytes.
   - The ACIA reconstructs these bytes and places them in its **Receive Data Register (RDR)**.
   - The ACIA then triggers an interrupt via the **MC68901 MFP** to notify the CPU.

## 3. CPU Reading Joystick State
The CPU can read the joystick state through two primary mechanisms:

### A. Memory-Mapped I/O (Direct/Emulated)
In the Atari ST, the joystick state is mapped into the I/O space. Steem SSE emulates these reads in `steem/stjoy.cpp`:

- **Fire Buttons** (`$FF9200`): Reads the state of the fire buttons for all four potential joysticks (Port 0, Port 1, and two others if applicable). 
  - Bit 0: Joy 0 Fire
  - Bit 1: Joy 1 Fire
  - Bit 2: Joy 0's second joystick (if applicable)
  - Bit 3: Joy 1's second joystick (if applicable)
- **Stick Directions** (`$FF9202`): Reads the directions for the joysticks.
  - Joy 0: Bits 0-3 (Up, Down, Left, Right)
  - Joy 1: Bits 4-7 (Up, Down, Left, Right)
  - The mapping is typically: Bit 0=Up, 1=Down, 2=Left, 3=Right (or similar, based on implementation).

### B. Serial Communication via IKBD
The CPU can send commands to the HD6301 via the ACIA to request the current joystick status. The HD6301 then responds with a packet containing the state.

## 4. Difference Between Joystick Ports
While the two main joystick ports (Port 0 and Port 1) are electrically similar, they are mapped to different bits in the status registers:

- **Port 0**: Mapped to the lower nibble of the direction register (`$FF9202`).
- **Port 1**: Mapped to the upper nibble of the direction register (`$FF9202`).
- In `steem/stjoy.cpp`, the implementation explicitly differentiates between `N_JOY_PORT_0` and `N_JOY_PORT_1`.

## 5. Timing and Debouncing
In real hardware, the HD6301 handles the polling and debouncing of the digital switches. In the emulator:
- **Polling**: The emulator updates the `stick` array based on the host PC's joystick state.
- **Timing**: The reads from `$FF9200` and `$FF9202` are near-instantaneous as they access the cached `stick` state.
- **Consistency**: The implementation ensures that if a joystick is disabled (via options), it returns a neutral state.

## 6. Position Determination Logic
The logic used to determine the stick's position is a simple bitmask.

### Direction Logic (in `joy_get_pos`)
The emulator iterates through the four directions and checks if the corresponding PC joystick axis has crossed the defined **DeadZone**:
```cpp
for(int n=0;n<4;n++) {
    if(IsDirIDPressed(Joy[joy].DirID[n],Joy[joy].DeadZone,true,true)) 
        Ret|=1<<n; 
}
```
This converts the analog axis of a modern PC joystick into the digital "pressed/not pressed" state required by the ST.

### Fire Button Logic (in `joy_read_buttons`)
The fire button is mapped to Bit 7 of the stick byte:
```cpp
if(JoyDown)
    stick[joy]|=BIT_7;
else
    stick[joy]&=~BIT_7;
```
The `JoyDown` state is determined by checking the assigned PC joystick button.

### Resultant Register Value
When the CPU reads `$FF9202`, the emulator constructs the word by shifting these bits into their correct positions for each port, as seen in `JoyReadSTEAddress`:
- Joy 0 directions are placed in bits 0-3.
- Joy 1 directions are placed in bits 4-7.
- Port B directions are shifted further into the word.
