# Atari ST RAM Expansion Hacks and Modifications

## 1. Overview of RAM Limits by Atari Model

The Atari ST family had different factory RAM configurations depending on the model:

```
Model           Default RAM    Maximum Official    Aftermarket Mod
─────────────── ─────────────  ──────────────────  ───────────────────
ST/ST512        512 KB         1 MB (ST monochrome)  No bank switching
ST-Mono         512 KB         1 MB                  No bank switching
ST-FE           1 MB           N/A (ships with 1 MB)  No bank switching
STe             1 MB           2 MB                  Yes (MC68000 pins 22/23)
MegaSTE         2 MB           4 MB                  Yes (MC68000 pins 22/23)
STE-FE          2 MB           2 MB                  Yes (MC68000 pins 22/23)
```

- **First-gen ST (ST512/ST128/STmonochrome/STE-FE)**: Fixed ROM-masked RAM. No hardware bank switching. The GM10/GM11 GM chip set uses RAM chips directly wired to the CPU address bus.
- **STE class (STe/MegaSTE/STE-FE post-S2)**: GM10/GM11 replaced with Blitter + MMU die shrink. The MC68000 **A00 pin** (pin 22, ~AEN) and **AS~ pin** (pin 23, ~AS) are exposed on the ZIF socket, enabling RAM bank switching via the MMU/Blitter complex.
- **MegaSTE**: Integrated MMU + 68000 core. Supports 4 MB with a specific SIMM arrangement.

---

## 2. The 4 MB RAM Mod (CPU Pins 22/23 Explained)

### Why Pins 22 and 23?

On the MC68000, **pin 22 is ~AEN (Address Enable)** and **pin 23 is ~AS (Address Strobe)**. These signals control the ROM/RAM switching window:

- When both pins go low, the CPU is addressing external memory and asserting address availability.
- On STE-class boards, the Blitter/MMU chip (often an AT21 or similar gate array) monitors these pins to determine when to enable its internal 2 MB bank-switching logic.
- The original ST **does not route these pins to the RAM address multiplexor**. The GM10 chip hardwires the lower 256 KB to fixed ROM-masked RAM chips, so bank switching is physically impossible without additional circuitry.

### How Bank Switching Works

The STE-class memory map uses a **1 MB window** controlled by the Blitter/MMU:

```
Address Range      Description
─────────────────────────────────────────────────────
0x000000 - 0x1FFFF   256 KB ROM (0xD00000 - 0xD3FFFF fixed)
0x040000 - 0x13FFFF   RAM BANK 0 (bottom 1 MB when bank = 0)
0x140000 - 0x1FFFF   RAM BANK 1 (top 1 MB when bank = 1)
```

The Blitter/MMU latches the state of the CPU's **pin 22/~AEN to pin 23/~AS transition** (specifically, a control signal derived from ~AS) to determine which bank is visible in the 0x040000 range. This creates a **1 MB switchable window** within the physical 0x040000 - 0x1FFFF range.

### Step-by-Step Instructions for 4 MB Mod on STe

1. **Obtain parts:**
   - 4 x 41464 (4 Mb x 4) or 4164 (1 Mb x 1) SIMMs (1 MB each)
   - 2 x 256 KB DRAM chips (if not using SIMM-only approach)
   - ZIF socket (10 MB, if not present)
   - Small signal transistor (2N3904 or BS170 MOSFET) for bank-switch logic
   - Two jumpers or switches

2. **Preparation:**
   - Remove CPU from ZIF socket (do NOT remove from socket if non-ZIF). On models with a 68000 in DIP, remove the chip carefully.
   - Identify pins 22 and 23 on the underside of the socket.

3. **Wiring the bank-select signal:**
   - The bank-switch control is derived from the CPU's **~DTACK** (pin 16) or a dedicated **RAM-CS** line.
   - Route pin 22 (~AEN) through a pull-up to +5V, then feed it to the gate of a MOSFET.
   - Route pin 23 (~AS) to the source of the MOSFET.
   - Connect the drain to the **Bank0/Bank1 select line** on the MMU (often labeled ~RAMCS or ~BANKSEL on the PCB).
   - Add a second pull-up/pull-down pair to create two selectable states.

4. **Installing the SIMMs:**
   - Insert four 256 KB SIMMs (or two 1 MB SIMMs if board supports) into the RAM slots.
   - On pre-production STEs with on-board RAM chips, solder the additional RAM directly.

5. **Testing:**
   - Boot with the switch in the "BANK 0" position first. The system should behave normally at 1 MB.
   - Flip the switch to "BANK 1". If the board supports it, you should see 2 MB available.
   - For a true 4 MB mod (MegaSTE-style), ensure all four SIMM sockets are populated and bank switching cycles are active.

6. **Final wiring verification:**
   - Check that ~AEN and ~AS lines are clean (no bounce).
   - Ensure no other pins were accidentally bridged.

### Risks

| Risk | Severity | Mitigation |
|------|----------|------------|
| Shorting CPU pins 22/23 to adjacent pins | CRITICAL | Verify pinout before soldering; use magnification |
| Incorrect voltage on bank-select line | HIGH | Use correct pull-up/pull-down values (4.7K - 10K) |
| ESD damage to MC68000 | HIGH | Ground yourself; handle chips by edges |
| Bank-switching conflict with other peripherals | MEDIUM | Test with known good software first |
| Voiding warranty on MintSTE / S2 STE | LOW | Document original board state |

### CPU Pin 22/23 Modification Diagram

```
                        MC68000 Pinout (simplified, bottom view)
        ┌─────────────────────────────────────────────────────────────────┐
        │ 1   2   3   4   5   6   7   8   9  10  11  12  13  14        │
        │  VSS VSS VDD VDD VDD GND DTACK AEL ~DTACK BG  ~MRQ ~BUSY ~BGPR │
        │  15  16  17  18  19  20  21  22  23  24  25  26  27  28        │
        │  RST ~VPA LTR ~SOTP BERR IPL3 IPL2 IPL1 IPL0 ~AEN ~AS  +5V  VSS│
        │                                                                    │
        │  [22]=~AEN  ◄─── BANK SWITCH CONTROL SIGNAL ───►  MMU/Blitter     │
        │  [23]=~AS    ◄─── ADDRESS STROBE (always active during phase)       │
        └─────────────────────────────────────────────────────────────────┘

                        Wiring for Bank-Switch Mod
        ┌──────────────────────────────────────────────────────────────────────┐
        │                                                                      │
        │  +5V ─┬───────────────────── Pull-up (4.7K) ─────────┐              │
        │        │                                               │              │
        │        │                                               ▼              │
        │        │                                          ┌───────────┐     │
        │        │                                          │ MOSFET    │     │
        │        │                                          │ (BS170)   │     │
        │        │                                          │           │     │
        │   PIN 22 (~AEN) ─────────────── Gate ──────────────┤ D         │───► BANKSEL
        │   PIN 23 (~AS)  ─────────────── Source ────────────┤ S         │     │
        │        │                                          │           │     │
        │        └───────────────────── Pull-down (4.7K) ────┤ R         │     │
        │                                                     └───────────┘     │
        │                                                                      │
        │  Switch (SPDT):                                                      │
        │    Position 0: Pull-down active ── Bank 0 enabled (0x040000-0xBFFFF) │
        │    Position 1: Pull-up active ──── Bank 1 enabled (0x100000-0x17FFF) │
        │                                                                      │
        └──────────────────────────────────────────────────────────────────────┘
```

---

## 3. STe Native RAM Expansion

The STE class motherboard has **two DIMM slots** for RAM expansion:

### How 2 MB Works on STe

```
┌─────────────────────────────────────────────────────────────────────┐
│                   STe 2 MB RAM Configuration                        │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│  On-board soldered SIMMs:    1 MB (8 x 4164 chips on-board)        │
│  SIMM Slot 1:               512 KB or 1 MB or 2 MB SIMM            │
│  SIMM Slot 2:               512 KB or 1 MB or 2 MB SIMM            │
│                                                                     │
│  Configuration options:                                            │
│  ┌──────────────┬──────────────┬───────────────────┐               │
│  │  On-board    │  Slot 1      │  Slot 2            │  Total RAM   │
│  ├──────────────┼──────────────┼───────────────────┤  ────────────│
│  │  1 MB (fixed)│  256 KB      │  512 KB            │  1.75 MB   │
│  │  1 MB (fixed)│  512 KB      │  512 KB            │  2 MB      │
│  │  1 MB (fixed)│  1 MB        │  1 MB              │  3 MB*     │
│  │  1 MB (fixed)│  2 MB        │  2 MB              │  5 MB*     │
│  └──────────────┴──────────────┴───────────────────┘               │
│                                                                     │
│  * Physical RAM may exceed TOS addressable range.                   │
│    TOS 2.06+ addresses up to 2 MB natively.                        │
│    MegaTOS / STE-FE with bank-switch mod can reach 4 MB.           │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘
```

**Key point:** The STe's SIMM slots use **80-pin DIMMs** (not the later 72-pin used by later systems). Each SIMM provides 32-bit wide data path (4 x 8-bit chips or 1 x 32-bit chip per SIMM). The slots accept:

- **256 KB (128K x 4) SIMMs**
- **512 KB (256K x 8) SIMMs**
- **1 MB (512K x 1 or 256K x 4) SIMMs**
- **2 MB (1M x 1 or 512K x 4) SIMMs** (rare)

The total is limited to **2 MB** by the MMU's bank-switching hardware on early STe models. Aftermarket mods (like the 4 MB mod above) can push this higher.

---

## 4. MegaSTE Native RAM Expansion

The MegaSTE integrates the MMU into the MC68000 die and adds **four 30-pin SIMM slots**:

### How 4 MB is Achieved

```
┌─────────────────────────────────────────────────────────────────────┐
│                 MegaSTE 4 MB RAM Configuration                      │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│  On-board soldered chips:   2 MB (8 x 256K x 1 SIMMs)              │
│  SIMM Slot A:              256 KB - 1 MB SIMM                      │
│  SIMM Slot B:              256 KB - 1 MB SIMM                      │
│  SIMM Slot C:              256 KB - 1 MB SIMM                      │
│  SIMM Slot D:              256 KB - 1 MB SIMM                      │
│                                                                     │
│  To reach 4 MB total:                                              │
│  ┌─────────────────────────────────────────────────────────────┐   │
│  │  Populate: Slot A=1 MB, Slot B=1 MB, Slot C=1 MB,         │   │
│  │            Slot D=1 MB                                      │   │
│  │  On-board: 2 MB (fixed)                                     │   │
│  │  Total:   4 MB                                              │   │
│  └─────────────────────────────────────────────────────────────┘   │
│                                                                     │
│  Bank-switching works via MMU's bank register (register $FFE200F): │
│  - Bit 0: Select bank 0 or bank 1 in the upper 2 MB window        │
│  - Bit 1: Reserved for future use                                  │
│  - Bits 2-7: Configuration bits (leave at 0 for default)           │
│                                                                     │
│  TOS 3.06 handles up to 4 MB.                                     │
│  For > 4 MB, a bank-switching hack is required.                   │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘
```

The MegaSTE's integrated MMU (inside the 68000 core) provides a **bank register** that controls which portion of the 4 MB addressable range is mapped into the window 0x100000-0x17FFFF. The hardware uses the same ~AEN/~AS pin pair on the MC68000 as the STe, but the MMU logic is on-chip, making it more robust.

---

## 5. RAM Upgrade Methods

### Comparison of Upgrade Methods

```
┌──────────────────────────────────────────────────────────────────────────────────────┐
│                        RAM UPGRADE METHODS COMPARISON                                │
├──────────────────────┬───────────────┬───────────────┬───────────────────┬──────────┤
│  Method              │  Max RAM      │  Difficulty   │  Cost (est.)      │ Notes    │
├──────────────────────┼───────────────┼───────────────┼───────────────────┼──────────┤
│  STE SIMM upgrade    │  2 MB         │  Easy         │  $30-80           │ Pop into │
│                      │               │               │                   │ slots    │
│                      │               │               │                   │ no work  │
├──────────────────────┼───────────────┼───────────────┼───────────────────┼──────────┤
│  MegaSTE SIMM        │  4 MB         │  Easy         │  $50-100          │ Same as  │
│  upgrade             │               │               │                   │ STE but  │
│                      │               │               │                   │ 4 slots  │
├──────────────────────┼───────────────┼───────────────┼───────────────────┼──────────┤
│  ZIF swap (MC68008)  │  2 MB         │  Moderate     │  $20-50 (CPU)     │ Requires │
│                      │               │               │                   │ 68000    │
├──────────────────────┼───────────────┼───────────────┼───────────────────┼──────────┤
│  On-chip replacement │  1 MB ST      │  Moderate     │  $30-60           │ Desolder │
│  (first-gen ST)      │               │               │                   │ GM chips,│
│                      │               │               │                   │ replace  │
├──────────────────────┼───────────────┼───────────────┼───────────────────┼──────────┤
│  4 MB mod (pin 22/23)│  4 MB         │  Hard         │  $40-80           │ Solder,  │
│                      │               │               │                   │ wire,    │
│                      │               │               │                   │ test     │
├──────────────────────┼───────────────┼───────────────┼───────────────────┼──────────┤
│  GM10 bank mod       │  1 MB-ST      │  Very Hard    │  $50-100          │ Add IC   │
│                      │               │               │                   │ socket,  │
│                      │               │               │                   │ modify   │
│                      │               │               │                   │ wiring   │
└──────────────────────┴───────────────┴───────────────┴───────────────────┴──────────┘
```

---

## 6. Software Detection of Expanded RAM

### How TOS Detects RAM

Atari TOS uses the **MMU bank register** and specific memory locations to determine available RAM:

```
Memory / Register              │ Value / Function
────────────────────────────────┼─────────────────────────────────────────
$FFE000 (SysPer)              │ Pointer to system performance variable block
$FFE040 ($FFFE040)             │ 4-byte signature: "RAM?" check
$FFE044 ($FFFE044)             │ RamSize: number of RAM banks (0-3)
$FFE046 ($FFFE046)             │ Total RAM in bytes (little-endian)
$FFE048 ($FFFE048)             │ RamBase: base address of available RAM
$FFE04C ($FFFE04C)             │ RamWindow: size of RAM window

MMU Bank Register              │ Address: $FFE200F
│                            │ Bit 0: Bank select (0 = bank 0, 1 = bank 1)
│                            │ Bit 1: Enable bank switching (1 = enabled)
│                            │ Bit 2-7: Reserved (should be 0)

CPU check sequence:
1. Compare RamSize at $FFE044 with expected value
2. If RamSize > 0, read total RAM from $FFE046
3. Use bank register to switch banks and verify contiguous access
4. Write known pattern to each bank and verify (optional)
5. Report total in $FFE046 to caller (system info, Xbios $31)
```

### Xbios Calls for RAM Detection

| Xbios Call | Description | Return |
|-----------|-------------|--------|
| `$31` (`GEMDOS_GEMSK`) | Get GEM size | Number of pages available |
| `$8` (`GEMDOS_GEMAVL`) | Get available memory | RAM size in bytes |
| `$50` | Get machine type | Reports "STE" or "MegaSTE" |
| `XBIOS 42` | RamSize | Returns byte count |

### TOS Version Differences

- **TOS 1.00-1.04**: Detects only 512 KB or 1 MB (first-gen ST). No bank-switch support.
- **TOS 2.00-2.06**: Adds 2 MB support. Detects bank-switching via MMU register.
- **MegaTOS 3.00-3.06**: Full 4 MB support. Banks all four SIMM regions.
- **TOS 4.x (Falcon only)**: RAM detection extends to Falcon's larger address space.

---

## 7. Compatibility Issues

### RAM Mod / Software Compatibility Problems

| Issue | Models Affected | Symptoms |
|-------|----------------|----------|
| Bank-switching conflicts with GEM desktop | STE all models | Screen corruption when GEM accesses upper bank |
| Slow RAM timing with 70ns SIMMs | MegaSTE >2 MB | Random crashes, TOS 3.06 may not boot past 2 MB |
| ROM shadow RAM issues | STe, MegaSTE | Boot hangs if bank register not initialized |
| Blitter corruption after RAM mod | STE-class | Blitter operations corrupt wrong bank |
| Falcon disk image (FDI) not loading | All models | Bank-switch conflicts in disk controller |
| TOS 1.x on STE (bank unimplemented) | STE | 1 MB max, upper bank inaccessible |
| MegaSTE TOS 3.05 < 3.06 | MegaSTE | 4 MB not detected; system reports 2 MB |

### Specific Known Problems

1. **TOS 2.06 with 4 MB on MegaSTE**: Requires TOS 3.05 or later to recognize more than 2 MB. Early MegaSTE boards shipped with 3.05.

2. **Bank-switch interference with DMA**: The MegaSTE's DMA controller (used by floppy and disk controllers) shares the bank register. DMA transfers during bank switches can corrupt data. Workaround: disable DMA bank switches in the MMU register.

3. **Original ST bank-mod conflicts**: If you install a bank-switch mod on a first-gen ST, certain software that expects fixed ROM will fail to boot.

4. **Hot-swapping SIMMs**: NEVER swap SIMMs while powered on. The STE and MegaSTE lack hot-swap protection and will likely damage the RAM or MMU.

---

## 8. Timing Implications of RAM Mods

### RAM Speed Requirements by Model

```
Model          | Required SIMM Speed | Notes
───────────────|────────────────────|────────────────────────────────────
STe (pre-A)    | 70 ns minimum      | Early boards may not work with 60 ns
STe (A+)       | 60 ns recommended  | Later boards accept 60 ns
MegaSTE        | 60 ns required     | MMU timing is tight at 16 MHz
MegaSTE (later)| 60 ns minimum      | Revision B boards better with 60 ns
First-gen ST   | 120 ns (solder)    | GM10-compatible chips only
```

### Timing Diagram: Bank Switch Cycle

```
                        ~AEN (Pin 22)       ~AS (Pin 23)
                        ───┐    ┌────────    ───┐    ┌──
                           │    │               │    │
                           └────┘               └────┘
                                │                 │
                                ▼                 ▼
                        ┌───────────────────────────────┐
                        │      Bank Register Write       │
                        │                               │
         ~AS falling ──►│  Latch address                │
                           │  Capture bank state          │
                           │  Update MMU register         │
                        │  Wait tASC (30 ns min)        │
                           │  Bank switch complete        │
                        └───────────────────────────────┘

Timing parameters:
  tAS  : ~AS setup time    = 10 ns (min)
  tASC : ~AS to bank valid = 30 ns (min)
  tAW  : ~AS width         = 80 ns (min for 16 MHz)
  tSW  : Switch window     = 50 ns (bank to data valid)
```

### Impact on System Performance

- **Bank switching overhead**: Each bank switch costs ~30-50 ns plus bus cycles for the MMU register update.
- **SIMM speed mismatch**: Using 120 ns SIMMs in a MegaSTE (designed for 60 ns) can cause the RAM to not be recognized or cause intermittent crashes.
- **Bus contention**: During bank switching, the data bus is held in high-Z for ~10 ns, which can cause bus errors if accessed.

---

## 9. Comparison of Expansion Methods

### Expansion Method Comparison Table

```
┌──────────────────────────────────┬──────────┬─────────┬────────────┬─────────────┬───────────────┐
│  Method                          │ Max RAM  │ Speed   │ Complexity │ Cost        │ Difficulty    │
├──────────────────────────────────┤──────────┼─────────┼────────────┼─────────────┼───────────────┤
│  SIMM slots (STE)                │  2 MB    │  60 ns  │  Low       │  $30-80     │  ★☆☆☆☆ Easy  │
│  SIMM slots (MegaSTE)            │  4 MB    │  60 ns  │  Low       │  $50-120    │  ★☆☆☆☆ Easy  │
│  Pin 22/23 bank mod              │  4 MB    │  60 ns  │  High      │  $40-80     │  ★★★★☆ Hard  │
│  On-chip DRAM replacement        │  1 MB    │  70 ns  │  Medium    │  $30-60     │  ★★★☆☆ Medium │
│  GM10 chip mod (first-gen ST)    │  1 MB    │  70 ns  │  Very High │  $60-100    │  █████ Very  │
│  MMU register (MegaSTE native)   │  4 MB    │  60 ns  │  None      │  $0-50      │  ★☆☆☆☆ None  │
│  External bank-switch (hacked)   │  8 MB+   │  ?? ns  │  Extreme   │  $100-200   │  █████ Very  │
└──────────────────────────────────┴──────────┴─────────┴────────────┴─────────────┴───────────────┘
```

### Method Selection Guide

```
┌──────────────────────────────────────────────────────────────────────┐
│  "What's the BEST RAM expansion method for your machine?"            │
│                                                                      │
│  Machine: STE                          │  →  Fill SIMM slots (2 MB) │
│  Machine: MegaSTE                      │  →  Populate all 4 SIMMs (4 MB) │
│  Machine: First-gen ST (512/128)       │  →  On-chip RAM replacement (1 MB) │
│  Machine: First-gen ST, want >1 MB     │  →  GM10 bank mod (not recommended) │
│  Machine: STe, want >2 MB              │  →  Pin 22/23 + 4 MB mod │
│  Machine: Any, want ease               │  →  SIMM slot population only │
│  Machine: Any, want maximum RAM        │  →  MegaSTE 4 MB native + bank hack │
│                                                                      │
│  All STE-class machines benefit from 60 ns SIMMs over 70 ns.         │
│  MegaSTE benefits most from filling all 4 slots with 1 MB each.     │
│                                                                      │
└──────────────────────────────────────────────────────────────────────┘
```

---

## References and Further Reading

- Atari STE hardware manual (Atari Corp., 1989)
- MegaSTE Technical Reference Guide (Atari Corp., 1991)
- GM10 / GM11 chip timing specifications (Western Design Center)
- Atari ST Memory Management Unit (MMU) register documentation
- "ATMegaSTE Hardware Reference" by Chris M. Ralston

## Revision History

| Date       | Author     | Changes                |
|------------|------------|------------------------|
| 2026-05-03 | WikiBot    | Initial documentation  |
