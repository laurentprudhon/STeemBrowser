# TTL Circuit Interconnections on Atari ST Motherboards

## Overview of TTL Circuit Connections on Atari ST Motherboards

The Atari ST motherboard uses TTL/CMOS support ICs as the wiring backbone between the Motorola 68000 CPU and its four custom ASICs (Glue, MMU, DMA, Shifter). Unlike generic computers where TTL chips connect discrete components, the Atari ST uses a **minimal TTL complement** because Atari consolidated the traditional SSI/MSI logic onto the four custom chips. The remaining discrete TTL ICs handle only:

1. **Bus direction control** — the 74LS245 (IC1) sets data bus direction
2. **Control signal timing** — flip-flops (IC2, IC8) and latches (IC6, IC9) align 68000 bus phases
3. **Address decoding** — the 74LS138 (IC7) provides hardware chip-select generation
4. **Address buffering** — the 74LS244 (IC5) drives long address traces with high drive current
5. **Inverters and level-shifters** — 74LS04 (IC4) and 74LS05 (IC3) condition clock/reset signals
6. **DRAM timing** — the 74LS163 (IC10) provides row-address counting for refresh cycles

All 10 TTL ICs operate on +5V VCC. The board has two primary power distribution nets:
- **VCC (5.0V)** — connected to all VCC pins (pins 14, 16, 19, 20 depending on package) via 0.1uF decoupling capacitors per IC
- **GND** — connected to all GND pins (pins 7, 8, 10, 12 depending on package)

The TTL logic style is primarily **74LS (Low-power Schottky)** with one **74HC (High-speed CMOS)** replacement on later revisions. TTL inputs float HIGH when unconnected, while CMOS inputs must never be left floating.

### Board Layout Map

```
+--------------------------------------------------------------+
|  TOP SIDE (Component side)                                 |
|                                                              |
|  [CPU MC68000 DIP64]    [IC1 74LS245]    [IC2 74LS174]    |
|       |                      |                   |           |
|       |                      v                   v           |
|  [IC5 74LS244]    [IC3 74LS05]    [IC4 74LS04]            |
|       |                      |                   |           |
|       v                      v                   v           |
|  DRAM BANK A              CLOCK              RESET          |
|                                                              |
|  [IC7 74LS138]    [IC6 74LS374]    [IC10 74LS163]         |
|       |                      |                   |           |
|       v                      v                   v           |
|  ROM/RAM CS          ADDRESS          DRAM ROW ADDR           |
|                                                              |
|  [IC8 74LS175]    [IC9 74HC174]                              |
|       |                      |                               |
|       v                      v                               |
|  DMA LATCH              CONTROL LATCH                        |
|                                                              |
+--------------------------------------------------------------+
|  BOTTOM SIDE (Solder side)                                 |
|                                                              |
|  [Glue C029144]  [MMU C028300]  [DMA C029128]             |
|  [Shifter C028787]  [MFP MC68901]  [FDC WD1772]           |
|  [YM2149]  [ACIA MC6850]  [RTC MC146818A]                  |
|                                                              |
+--------------------------------------------------------------+
```

### Power Distribution Network

```
           +5V REGULATOR (CA3007H)
                  |
            +-----+-----+
            |           |
      +-----+     +-----+
      |           |
   0.1uF       0.1uF      (per-IC decoupling capacitors)
      |           |
      v           v
  IC1 VCC    IC2 VCC    IC3 VCC    IC4 VCC    IC5 VCC
  IC6 VCC    IC7 VCC    IC8 VCC    IC9 VCC    IC10 VCC
      |           |           |           |           |
      +-----+-----+-----+-----+-----+-----+-----+
                              |
                            GND (Ground plane, layer 2)
```

---

## Address Bus Routing Through TTL Chips

The 68000 CPU outputs a 24-bit address bus (A0-A23) that flows through multiple TTL buffers to reach DRAM, the Shifter, and expansion ports. The bus routing splits into three paths:

### Path 1: Lower Address (A0-A15) via 74LS374 (IC6)

Address A0-A15 appears on the 68000's multiplexed AD0-AD15 pins. During the address phase (MLO phase), the 74LS374 captures A0-A7 on the falling edge of the address strobe. The upper nibble A8-A15 goes directly to the Glue and MMU chips.

```
                    Lower Address Capture (74LS374 IC6)

CPU 68000                         74LS374 (IC6)                    To: Glue, MMU, DMA
+----------+                    +---------+
| AD0-A7   |---A0-A7 (pins 1,2,3,4,5,6,7,9)                      |
| (AD mux) |   \                | CLK  (pin 21) --- AS! (AS_bar)   |
|          |    > (captured     | OE   (pin 22) --- GND (enabled)   |
|          |     on AS! falling |                                  |
+----------+    edge)           | Q0-Q7 (pins 15,14,13,12,         |
      |                         |         11,10,16,17)             |
      v                         v                                  |
  [Address Phase]          A0-A7  -----------> Glue / MMU        |
                            A8-A15 --------- (direct trace)      |
                                                       |           |
                                                       |           |
                                    68000 A8-A15 ---+-----+-----+
                                                       |     |     |
                                                       v     v     v
                                                   [Glue] [MMU] [DMA]
```

### Path 2: Upper Address (A16-A23) via 74LS244 (IC5)

The upper address bits A16-A23 go through the 74LS244 octal buffer for signal integrity over long traces to DRAM and the Shifter.

```
                    Upper Address Buffering (74LS244 IC5)

CPU 68000                         74LS244 (IC5)
+----------+                    +---------+
| A16-A19  |--Bank1 IN1-4      | IN1-4 (pins 1,2,3,4)         |
| (pins     |                   | OUT1-4 (pins 18,17,16,15)      |
| 21-24)    |   /===\           | OE1    (pin 1)    --- GND      |
|           |--| BUF |----------+                                  |
| A20-A23  |--Bank2 IN5-8      | IN5-8 (pins 10,9,8,7)          |
| (pins     |   \===/           | OUT5-8 (pins 19,21,22,23)      |
| 25-28)    |                   | OE2    (pin 24)   --- GND      |
+----------+                   +------+----------------------------+
                                          |
                                          v
                                    +-----+-----+-----+
                                    |     |     |     |
                                    v     v     v     v
                               [MMU] [DMA] [SHI] [RAM]
                               (A16-A23 to all memory devices)
```

### Path 3: Address to DRAM (via MMU)

The upper 11 address lines (A13-A23) are routed from the 68000 through the Glue chip to the MMU, which generates DRAM row/column addresses:

```
                    DRAM Address Path

68000 CPUs A13-A23 (11 lines)
     |
     +----> Glue (C029144) pin 1-11 -- direct address pass-through
     |
     +----> MMU (C028300) pins 11-16, 37-39 -- address input
              |
              v
   MMU internal address register
              |
    +---------+---------+
    |                   |
row addr (A0-A6)   column addr (A0-A8)
    |                   |
    v                   v
DRAM bank A       DRAM bank B
(414616 x 4)    (414616 x 4)
```

### Address Map Decoded by TTL ICs

```
Address Range         TTL Chip          Decode Mechanism
------------          ----------          ----------------
$000000-$BFFFFF      IC7 (74LS138)     Y0 active (A19=A18=A17=000, CS enabled)
$C00000-$FFFFFF      IC7 (74LS138)     Y1 active (for ROM/RAM region)
$FF8000-$FF83FF       IC7 (74LS138)     Y2 active (MFP I/O select)
$FFC000-$FFDFFF       IC7 (74LS138)     Y5-Y6 active (Shifter select)
$FFE000-$FFFFFF       IC7 (74LS138)     Y7 active (MMU scroll registers)
Expansion slot        IC7 (74LS138)     Y7 active
```

---

## Data Bus Routing Through TTL Chips

The 68000 CPU's 16-bit data bus (D0-D15) is managed by the 74LS245 (IC1) bidirectional transceiver.

### Data Bus Direction Control via 74LS245 (IC1)

The 74LS245 has two 8-bit busses: one connected to the CPU data bus (left side) and one to the peripheral/bus side (right side). Direction is controlled by the DIR pin and data transfer is enabled by the OE pin.

```
                    Data Bus (74LS245 IC1)

CPU 68000                        74LS245 (IC1)                  Peripherals
+----------+                    +---------+                   +----------+
| D0 (LSB) |---> A_port pin 15 |         |          B_port    | D0 (LSB) |
| D1       |---> A_port pin 14 |         | <---- DIR/OE ---> | D1       |
| D2       |---> A_port pin 13 | 74LS   |      Pin 19 = LOW  | D2       |
| D3       |---> A_port pin 12 | 245    |      Pin 20 = 1/0  | D3       |
| D4       |---> A_port pin 11 |  IC1   |                    | D4       |
| D5       |---> A_port pin 10 |         |                    | D5       |
| D6       |---> A_port pin 9  |         |                    | D6       |
| D7       |---> A_port pin 1  |         |                    | D7       |
+----------+                    |         |                    +----------+
      |                         |         |                         |
      |                  OE (pin 19)     DIR (pin 20)              |
      |                      |             |                       |
      +----------------------|-------------+-----------------------+
                             |             |
                        GND (enabled)    0=CPU->PERIPH
                                       1=PERIPH->CPU
```

**Data bus routing flow:**

#### CPU Write Cycle (D7->D0 direction, DIR=LOW):

```
68000 D0-D15 --> IC1 A-port (pins 15,14,13,12,11,10,9,1)
                     |
                     |  DIR=LOW, OE=LOW
                     v
IC1 B-port (pins 16,17,18,19,20,21,22,23) --> MMU D0-D7
                                                     DMA D0-D7
                                                     Shifter D0-D7
                                                     MFP D0-D7
                                                     FDC D0-D7
```

#### CPU Read Cycle (D0->D7 direction, DIR=HIGH):

```
MMU/MA D0-D7 --> IC1 B-port (pins 16,17,18,19,20,21,22,23)
                     |
                     |  DIR=HIGH, OE=LOW
                     v
IC1 A-port (pins 15,14,13,12,11,10,9,1) --> 68000 D0-D15
```

#### DMA Cycle (peripheral-initiated, OE controlled by Glue):

```
Glue DE! signal --> IC1 OE (pin 19) [low during DMA]
                     |
Periph D0-D7 --> IC1 B-port --> IC1 A-port --> 68000 D0-D15
(per FDRQ/FDACK or HDRQ/HDAK handshake)
```

---

## Control Signal Routing Through TTL Chips

Control signals from the 68000 CPU are routed through the 74LS174 (IC2) flip-flop for timing alignment, then distributed to all components.

### CPU Control Signals via 74LS174 (IC2)

```
                    Control Latch (74LS174 IC2)

68000 CPU                  74LS174 (IC2)               System Devices
+--------+               +-------+                   ----------
| RW!    |--D0 (pin 1)  |       | D0--Q0 (pin 3)--> MMU R/W!
| RDT/W! |--D1 (pin 2)  | 74LS  | D1--Q1 (pin 5)--> MMU W/E!
| AS!    |--D2 (pin 3)  | 174   | D2--Q2 (pin 10)-> Glue AS!
| DTACK! |--D3 (pin 4)  | IC2   | D3--Q3 (pin 12)-> DMA DTACK
| VMA!   |--D4 (pin 5)  |       | D4--Q4 (pin 15)-> SHI CS!
| BCLR   |--D5 (pin 6)  |       | D5--Q5 (pin 20)-> Glue BG
|        |              | CLK   (pin 11)<-- CPU MCI (phase 2)
+--------+              | CLR   (pin 2) <-- RESET!
                        | VCC   (pin 20) -- +5V
                        | GND   (pin 10) -- GND
                        +-------+
```

### Address Strobe and Data Enable Routing

```
                    Control Signal Distribution

68000 Signals          IC2 (74LS174)          Target Chips
------------           ---------------          ------------
AS! (pin 3, latched)  Q2 pin 10             ---> Glue, MMU, DMA, Shifter DTACK
RW! (pin 1, latched)  Q0 pin 3              ---> MMU, DMA, Shifter, FDC
RDT/W! (pin 2)        Q1 pin 5              ---> MMU write enable
DTACK! (pin 4)        Q3 pin 12             ---> Glue (for bus timing)
VMA! (pin 5)          Q4 pin 15             ---> Shifter chip select
BCLR (pin 6)          Q5 pin 20             ---> Glue bus clear

Glue Chip Outputs:
  MFPCS! (pin X)  ---> MFP MC68901 chip select
  6850CS! (pin Y) ---> ACIA MC6850 chip select
  ROM1-6! (pins)  ---> TOS ROM chips chip selects
  RAS0!, RAS1!    ---> DRAM banks row strobe
  CAS0!, CAS1!    ---> DRAM banks column strobe
  HSYNC!, VSYNC!  ---> Shifter sync generation
  DTACK!          ---> 68000 done acknowledge
```

### MFP/ACIA/PSG Address Decoding via 74LS138 (IC7)

The lower I/O addresses ($FF8000-$FF8FFF) are decoded by the 74LS138:

```
                    I/O Address Decode (74LS138 IC7)

Address Lines          IC7 (74LS138)
--------------         ---------------
A15-A13 (always 111) --> Input C, B, A (pins 1, 2, 3) = 1,1,1
A12-A8 ($FF8xx range) --> combined with A22, A23 for chip select

Enable Pins:
  G1 (pin 6)  ---> A22! (always low for < 4MB) OR tied to low for all access
  G2A (pin 7) ---> A23! (always low for < 8MB)
  G2B (pin 4) ---> M68* (68000 cycle type, low for data cycle)

Decoded Outputs:
  Y0! (pin 5)  = $FF80xx MFP chip select
  Y1! (pin 4)  = $FF84xx ACIA chip select
  Y2! (pin 3)  = $FF86xx FDC chip select
  Y3! (pin 2)  = $FF88xx YM2149 PSG chip select
  Y4! (pin 1)  = $FF8Axx DMA chip select
  Y5! (pin 15) = $FF8Exx Reserved expansion
  Y6! (pin 14) = $FFCxxx Shifter select
  Y7! (pin 13) = $FFExx Glue register select
```

---

## Inter-IC Connection Diagrams

### Complete TTL IC Interconnection Map

```
                    Inter-IC Signal Flow (TTL Support ICs)

                    +--------+                                          +--------+
   Address A0-7 ---|>       |---- A0-7 -----+                   +----|>       |---- Address to MMU
   Address A8-15 --|  IC5  |                 |   +--------------->|  IC7     |---- CS signals
   (direct)        |244   |                 |   |                |138      |
   A16-A23 -------|<       |---- A16-A23 --+---+--> ROM/RAM/SHI  |<       |
                    +--------+                 |   +----------------|<       |---- ROM select (Y0)
                                               |                    +--------+         RAM select (Y1)
              +-------+   DIR/OE             |   I/O decode        I/O select (Y2)
   Data D0-7 --|  IC1  |<---+                |                    Exp select (Y5)
   (bidir)    |245   |    |                  |
   +----------|<     |----+------------------+
              |       |---- Periph D0-D7     |
              +-------+                       |
                                               |
                 +-------+   CLK      +-------+-------+   Latched   +--------+
   CPU ctl lines-->|  IC2  |---->----->|   IC6 374     |<----------|  IC10  |
   (RW!, AS!, etc)-|174   |           |244 Latch      |   Row Addr  |163     |
   +--------------|<     |---- CLR   |               |              |        |
   RESET! ---------|       |           |               |              +--------+
                    +-------+           |               |
                                         |               |
              +-------+   Latched   +-------+-------+   |
   DMA ctl ------|  IC8  |---->----|  IC9 174      |   |
   (4 lines)     |175   |          |174 Latch      |   |
   +------------|<     |---- Latched|<     |<------ |---+
                |       |           +--------+      v
                +-------+                 |    DRAM refresh
                                          |
                                      DRAM RAS/CAS

Key:  -- = trace connection
      <> = input buffer
      |  = bus line
```

### Per-IC Signal Flow Details

#### 74LS245 (IC1) Bus Transceiver Flow:

```
     READ cycle:          WRITE cycle:
     Peripheral --> CPU    CPU --> Peripheral

  I/O/MMU/DMA          I/O/MMU/DMA
  data bus             data bus
     |                    |
     v                    ^
  B-port               B-port
  (IC1 pins            (IC1 pins
   16-23)               16-23)
     |                    |
     |  DIR=1             |  DIR=0
     |  OE=0              |  OE=0
     v                    ^
  A-port               A-port
  (IC1 pins            (IC1 pins
   15,14,13,11,10,9,   15,14,13,12,11,10,9,
   1,12)               1,12)
     |                    |
     v                    ^
  68000 D0-D15        68000 D0-D15
```

#### 74LS138 (IC7) Address Decode Fan-Out:

```
  IC7 Output     Active When       Drives To
  --------       ----------        ------------
  Y0 (pin 5)!   addr C00000!      4x TOS ROM (27C010 EPROM)
  Y1 (pin 4)!   addr 000000!      6x DRAM (414616)
  Y2 (pin 3)!   addr A00000!      I/O region decode
  Y3 (pin 2)!   addr FF8800!      YM2149 PSG chip select
  Y4 (pin 1)!   addr FF8A00!      DMA chip select
  Y5 (pin 15)!  addr FF8E00!      Expansion slot decode
  Y6 (pin 14)!  addr FFC000!      Shifter control registers
  Y7 (pin 13)!  addr FFE000!      Glue/MRU scroll registers
```

#### 74LS174 (IC2) and 74LS374 (IC6) Timing Relationship:

```
  CPU 68000 Bus Cycle Timing:

  MCI (internal clock):  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|
                                                                  ^
  AS! (Address Strobe):   |______|                              |
                            |<--> Address Phase        |<------->|
                                                           Data Phase
                              |___________________________|

  IC2 (74LS174) CLK = MCI pin 3 ---- captures on positive edge at AS! transition
  IC6 (74LS374) CLK = AS! ---- captures on negative edge at AS! falling

  Result: IC2 latches control signals at MCI rising edge (AS! phase start)
          IC6 latches address data at AS! falling edge (end of address phase)
          This ensures address is stable before data phase begins
```

#### 74LS04 (IC4) and 74LS05 (IC3) Clock Generation:

```
  Clock Generation Circuit:

  8.000 MHz Crystal (Y1)
         |
         v
  [IC3 74LS05]  (hex inverter, open collector)
  Pin inputs: raw crystal signal (pins 1,2,3)
  Pin outputs: buffered/inv. crystal signal (pins 2,4,6)
         |
         v
  [IC4 74LS04]  (hex inverter, push-pull)
  Pin inputs: from IC3 (pins 1,2,3)
  Pin outputs: clean square wave (pins 2,4,6)
         |
         +----> 68000 CLK pin 11
         +----> Glue CLK_IN pin
         +----> MMU CLK_8MHZ pin 20
         +----> DMA CLK pin
         +----> Shifter reference
```

#### 74LS163 (IC10) DRAM Refresh Counter:

```
  DRAM Refresh Controller (inside Glue):

  Glue internal refresh timer
         |
         | (refresh request, driven by CLK_4MHZ)
         v
  74LS163 (IC10) Counter
  EN1=1, EN2=1 (always enabled during refresh)
  /CLR=1 (not clearing)
  /LD=1 (not loading)
         |
  CLK pin 2 <-- refresh pulse from Glue
         |
         v
  QA-QD (pins 5,6,7,9) = R0-R3 row address bits
         |
         +-----> DRAM A0 (pin 5)
         +-----> DRAM A1 (pin 6)
         +-----> DRAM A2 (pin 7)
         +-----> DRAM A3 (pin 9)

  RC (pin 15) carry output:
         |
         +----> second 74LS163 CLK (cascade to R4-R7)
```

---

## Board-Level Signal Map

### Signal Flow Hierarchy

```
  Level 1: System Bus (backbone)
  Level 2: TTL Support ICs (IC1-IC10)
  Level 3: Custom ASICs (Glue MMU DMA Shifter)
  Level 4: Peripheral ICs (MFP FDC YM2149 ACIA MFP RTC)
```

### Complete Signal Routing Table

```
Source IC      Signal         Dest IC(s)           Path Description
---------      ------         ----------           ---------------
IC2 (174)      Q0 Q1 Q2       Glue, MMU, DMA      Control signals from 68000
IC2 (174)      Q3 Q4 Q5       Glue, Shifter       Timed control signals
IC5 (244)      OUT1-4         MMU, DMA, Shifter   Upper address buffer (A16-A23)
IC5 (244)      OUT5-8         DRAM banks          Upper address + control
IC1 (245)      A-port         68000 D0-D15        CPU data bus
IC1 (245)      B-port         MMU, DMA, Shifter   Peripheral data bus
IC6 (374)      Q0-Q7          Glue, MMU           Latched lower address (A0-A7)
IC7 (138)      Y0-Y7          ROM, RAM, I/O chips Address decode chip selects
IC8 (175)      Q0-Q3          DMA, FDC            Latched DMA control signals
IC9 (174)      Q0-Q5          Bus control         Latched bus arbitration signals
IC3 (05)       outputs        Clock dist          Inverted clock signals
IC4 (04)       outputs        Multiple            Cleaned clock signals
IC10 (163)     QA-QD          DRAM banks          Row address counter
```

### Critical Path Analysis

```
  Critical Path 1: CPU -> MMU -> DRAM (memory read/write)
  68000 D0-D15 --> IC1 B-port (TTL buffering) --> MMU D0-D7 (4ns)
  68000 A0-A23 --> IC5 (244) + IC6 (374) (15ns) --> MMU address (4ns)
  MMU RAS/CAS --> DRAM (100ns cycle time for 414616)
  Total memory access: ~200ns (5 MHz effective)

  Critical Path 2: CPU -> Glue -> Shifter (video generation)
  CPU A5-A15 --> Glue (5ns) --> Shifter A5-A15
  Shifter pixel clock (32MHz) --> RGB DAC (analog) --> Monitor
  Video cycle: 31.25ns per pixel (32 MHz)

  Critical Path 3: CPU -> IC2 -> All (control timing)
  CPU control lines --> IC2 (55ns TTL propagation) --> all peripherals
  Control alignment: IC2 latches on MCI rising edge
  IC6 aligns address data on AS! falling edge
  Skew between control/data: ~15ns max
```

### Pin-to-Signal Reference

```
  TTL Pin Reference (all DIP packages, top view, pin 1 = dot side):

  IC1 (20-pin):   A15=A0, A14=A1, A13=A2, A12=A3, A11=A4, A10=A5, A9=A6, A1=A7
                   A16=D7, A17=D6, A18=D5, A19=D4, A20=D3, A21=D2, A22=D1, A23=D0
                   Pin 19=OE!, Pin 20=DIR, Pin 8=GND, Pin 20=VCC

  IC2 (20-pin):   D1=pin1, D2=pin2, D3=pin3, D4=pin4, D5=pin5, D6=pin6
                   Q1=pin3, Q2=pin5, Q3=pin10, Q4=pin12, Q5=pin15, Q6=pin20
                   Pin 11=CLK, Pin 2=CLR, Pin 10=GND, Pin 20=VCC

  IC3 (14-pin):   IN1/OUT1=pin1/2, IN2/OUT2=pin3/4, IN3/OUT3=pin5/6
                   IN4/OUT4=pin9/10, IN5/OUT5=pin11/12, IN6/OUT6=pin13/14
                   Pin 7=GND, Pin 14=VCC (open collector outputs)

  IC4 (14-pin):   IN1/OUT1=pin1/2, IN2/OUT2=pin3/4, IN3/OUT3=pin5/6
                   IN4/OUT4=pin9/10, IN5/OUT5=pin11/12, IN6/OUT6=pin13/14
                   Pin 7=GND, Pin 14=VCC (push-pull outputs)

  IC5 (24-pin):   Bank1: IN 1-4=pins1,2,3,4 | OUT 1-4=pins18,17,16,15 | OE1=pin1
                   Bank2: IN 5-8=pins10,9,8,7 | OUT 5-8=pins19,21,22,23 | OE2=pin24
                   Pin 12=GND, VCC=pins3,24 (note: both pins need VCC)

  IC6 (20-pin):   D1-D8=pins1,2,3,4,5,6,7,9 | Q1-Q8=pins15,14,13,12,11,10,16,17
                   Pin 21=CLK, Pin 22=OE! | Pin 10=GND, Pin 20=VCC

  IC7 (16-pin):   A=pin1, B=pin2, C=pin3, G2A=pin4, Y4=pin1, Y3=pin2, Y2=pin3, Y1=pin4
                   G1=pin6, G2B=pin7 | Y0=pin5, Y5=pin15, Y6=pin14, Y7=pin13
                   Pin 8=GND, Pin 16=VCC

  IC8 (14-pin):   D1=pin1, D2=pin2, D3=pin4, D4=pin5
                   Q1=pin10, Q2=pin12, Q3=pin13, Q4=pin15
                   Pin 14=CLK, Pin 9=CLR! | Pin 8=GND, Pin 14=VCC

  IC9 (14-pin):   D1-D6=pins1,2,3,4,5,6
                   Q1=pin2, Q2=pin3, Q3=pin5, Q4=pin10, Q5=pin12, Q6=pin15
                   Pin 11=CLK, Pin 10=CLR! | Pin 7=GND, Pin 14=VCC

  IC10 (16-pin): A=pin1, B=pin2, C=pin3, D=pin4
                   QA=pin5, QB=pin6, QC=pin7, QD=pin9
                   Pin 2=CLK, Pin 1=CLR!, Pin 13=PLD!, Pins7,10=EN1,EN2
                   Pin 15=RC (carry), Pin 8=GND, Pin 16=VCC
```

### Net Names (Per-Net Summary)

```
  NET_NAME              Wires   Connected To
  --------              -----   --------------
  DATA_BUS_A            8       IC1 A-port (CPU side)
  DATA_BUS_B            8       IC1 B-port (peripheral side)
  ADDR_LOWER            8       IC6 latch inputs/outputs
  ADDR_UPPER            8       IC5 buffer inputs/outputs
  CTRL_LATCHED          6       IC2 outputs (R/W, AS, DTACK, etc.)
  CS_ROM                1       IC7 Y0 + TOS ROM chip selects
  CS_RAM                1       IC7 Y1 + DRAM bank select
  CS_IO_REGION          1       IC7 Y2 + I/O decode
  CS_EXPANSION          1       IC7 Y5-Y7 + expansion slot
  CLOCK_PRIMARY         2       IC3 IC4 + 68000/Glue/MMU
  CLOCK_INVERTED        2       IC3 outputs + phase generation
  ROW_ADDR              4       IC10 outputs + DRAM row pins
  DMA_CONTROL           4       IC8 outputs + DMA peripheral
  BUS_CONTROL           6       IC9 outputs + arbitration
  RESET_DISTRIBUTED     1       IC2 CLR + all IC reset pins
```

---

## TTL IC Electrical Characteristics Reference

```
  Family    VCC   Input High  Input Low  Output High  Output Low  Propagation  Power/IC
  ------    ---   -----------  ---------  ------------  ----------  -----------  --------
  74LS      5.0V  2.0V min   0.8V max   2.7V min    0.5V max    15ns typ   2mA
  74HC      5.0V  3.5V min   1.5V max   VCC-0.1V   GND+0.1V   8ns typ    5uA
  74ALS     5.0V  2.0V min   0.8V max   2.7V min    0.5V max    5ns typ    4mA
  VCC tolerance: +5% / -10% (4.75V to 5.25V)
  Input clamp current: -18mA max per pin (74LS)
  Output current: +8mA source, -8mA sink (74LS)
  Bus capacitance: 50pF max per line
```

---

## References

- Atari ST Internals, Ch. 1.1 - Integrated Circits (PDF) - [atarimania.com](https://www.atarimania.com/documents/Atari-ST-Internals.pdf)
- Motorola 74LS/74HC Data Sheets - [onsemi.com](https://www.onsemi.com), [ti.com](https://www.ti.com)
- Atari 520ST Schematic (1985) - [florentflament.com](http://www.florentflament.com/blog/static/atari-st-csync/Atari_520ST_Schematic_1985.pdf)
- Atari ST Bus Doc - [info-coach.fr](http://info-coach.fr/atari/ressources/doc/Atari%20ST%20bus%20doc.pdf)
