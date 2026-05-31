# MC68000 CPU Implementation - Detailed Analysis

## Overview

The Motorola MC68000 (68000) CPU emulation in Steem SSE is a high-level, cycle-accurate implementation that faithfully reproduces the behavior of the 16/32-bit CISC microprocessor used in the Atari ST series. This document provides a comprehensive analysis of the CPU emulation architecture, instruction set implementation, and internal workings.

**Key Characteristics**:
- 32-bit internal architecture with 24-bit address bus
- 16 general-purpose 32-bit registers (D0-D7, A0-A7)
- 32-bit program counter (PC)
- 16-bit status register (SR)
- Seven exception priority levels
- Support for both user and supervisor modes
- Memory-mapped I/O architecture

## CPU Architecture in Steem SSE

### Core Components

```
MC68000 Emulation
├── cpu.cpp          - Core CPU emulation and execution loop
├── cpu_op.cpp       - Instruction implementations
├── cpu_ea.cpp       - Effective address calculation
├── cpuinit.cpp      - Opcode table initialization
├── cpuinit.h        - CPU initialization declarations
├── headers/cpu.h    - CPU data structures and declarations
└── headers/cpu_op.h  - Instruction operation declarations
```

### CPU Data Structures

**TMC68000 Structure** (cpu.h):
```cpp
struct TMC68000 {
    // Processing states
    enum {NORMAL, STOPPED, EXCEPTION, HALTED, NREGS=16};
    
    // Timing
    COUNTER_VAR dbi_timing;           // DBI timing counter
    COUNTER_VAR cycles_for_eclock;   // E-clock cycle counter
    COUNTER_VAR cycles0;              // System time recording
    
    // Program Counter
    MEM_ADDRESS Pc;                   // Official PC register
    DU32 upc;                         // Temporary PC register
    int BusIdleCycles;                // Bus idle cycle count
    
    // Registers
    signed int r[NREGS+1];            // D0-D7, A0-A7 (A7 doubled)
    WORD sr;                         // Status Register
    WORD ird;                        // Current opcode
    WORD irc, ir;                    // Prefetch queue (2 words)
    
    // E-clock synchronization
    BYTE eclock_sync_cycle;          // 0-8
    BYTE ProcessingState;            // Current processing state
    bool tpend;                      // Trace pending flag
    
    #if defined(SSE_ENABLE_TRACE_LOG)
    COUNTER_VAR nExceptions;         // Exception counter
    #endif
};

TMC68000 Cpu;  // Global CPU instance
```

**Register Access Macros**:
```cpp
#define areg (Cpu.r+8)              // Address registers (A0-A7)
#define LITTLE_PC (pc&0xFFFFFE)     // 23-bit PC
#define TRUE_PC Cpu.Pc              // True program counter

// Register access
#define REGB(n) cpureg[n].d8[B0]    // Low byte of register n
#define REGW(n) cpureg[n].d16[LO]   // Low word of register n
#define REGL(n) cpureg[n].d32        // Full 32-bit register n
#define AREG(n) cpureg[8+n].d32      // Address register n (A0-A7)
```

### Global CPU Variables

**Program Counter and Address Bus**:
```cpp
MEM_ADDRESS &pc = Cpu.upc.d32;      // Current PC (reference)
WORD &pch = Cpu.upc.d16[HI];        // High word of PC
WORD &pcl = Cpu.upc.d16[LO];        // Low word of PC
MEM_ADDRESS &SP = (MEM_ADDRESS&)Cpu.r[15];  // A7 as stack pointer
MEM_ADDRESS &other_sp = (MEM_ADDRESS&)Cpu.r[16]; // Alternate SP

DU32 uiabus;                       // Internal address bus
MEM_ADDRESS &iabus = uiabus.d32;    // 32-bit address bus
WORD &iabush = uiabus.d16[HI];      // High word
WORD &iabusl = uiabus.d16[LO];      // Low word
```

**Dual Stack Pointer Mechanism**:
- The MC68000 implements two physical stack pointers using the A7 register
- `Cpu.r[15]` (SP): Currently active stack pointer
- `Cpu.r[16]` (other_sp): Currently inactive stack pointer
- When in supervisor mode (S=1):
  - SP points to Supervisor Stack Pointer (SSP)
  - other_sp points to User Stack Pointer (USP)
- When in user mode (S=0):
  - SP points to User Stack Pointer (USP)
  - other_sp points to Supervisor Stack Pointer (SSP)
- Mode switching automatically swaps the active/inactive roles
- This provides complete isolation between user and supervisor stacks
- User programs cannot access or corrupt the supervisor stack
- Supervisor code can access both stacks using MOVE USP instruction

**Result Registers**:
```cpp
DU32 uresult;                       // 32-bit result register
LONG &resultl = uresult.d32;         // Long result
SHORT &resulth = uresult.d16[HI];   // High word result
SHORT &resultw = uresult.d16[LO];   // Low word result
signed char &resultb = uresult.d8[LO]; // Byte result
```

**Source and Destination Registers**:
```cpp
CHAR m68k_src_b;                    // Byte source
SHORT m68k_src_w;                   // Word source
DUS32 sm68k_src_l;                  // Long source
LONG &m68k_src_l = sm68k_src_l.d32;

CHAR m68k_dst_b;                    // Byte destination
SHORT m68k_dst_w;                   // Word destination
DUS32 sm68k_dst_l;                  // Long destination
LONG &m68k_dst_l = sm68k_dst_l.d32;

MEM_ADDRESS m68k_old_dest;          // Previous destination
```

## CPU Initialization and Opcode Dispatch

### Opcode Table Initialization (cpuinit.cpp)

**Opcode Table Structure**:
```cpp
void (*m68k_call_table[0xffff+1])();  // Big opcode table
// Index: opcode (0x0000 to 0xFFFF)
// Entry: function pointer to instruction handler
```

**Initialization Process**:
```cpp
void cpu_routines_init() {
    // Set default handler for all opcodes
    for(DWORD op = 0; op <= 0xffff; op++) {
        m68k_call_table[op] = m68k_trap1;  // Default: illegal instruction
    }
    
    // Decode opcode bits
    DWORD line = op >> 12;            // High 4 bits (line)
    DWORD b6 = (op & (BITS_ba9|BITS_876)) >> 6;  // Bits 6-8
    DWORD b3 = b6 & 7;               // Bits 6-8
    DWORD b876 = (op & BITS_876) >> 6; // Bits 8-10
    DWORD b543 = (op & BITS_543);     // Bits 5-7
    DWORD b876543 = (op & (BITS_876|BITS_543)) >> 3; // Bits 3-10
    DWORD op7 = op & 0x7;            // Low 3 bits
    
    // Assign handlers based on opcode patterns
    switch(line) {
        case 0:  // Line 0: Various instructions
            switch(b6) {
                case B6_000000:  // ORI
                    if((op & B6_111111) == B6_111100)
                        m68k_call_table[op] = m68k_ori_b_to_ccr;
                    else
                        m68k_call_table[op] = m68k_ori_b;
                    break;
                case B6_000001:  // ORI
                    if((op & B6_111111) == B6_111100)
                        m68k_call_table[op] = m68k_ori_w_to_sr;
                    else
                        m68k_call_table[op] = m68k_ori_w;
                    break;
                // ... hundreds of cases
            }
            break;
        // ... other line cases
    }
}
```

**Bit Definitions for Opcode Decoding**:
```cpp
// Bit masks for opcode decoding
#define BITS_ba9   0x0700  // Bits 8-10
#define BITS_876   0x0700  // Bits 8-10 (same as ba9)
#define BITS_543   0x00E0  // Bits 5-7
#define BITS_012   0x0007  // Bits 0-2

// Common bit patterns
#define B6_000000  0x00
#define B6_000001  0x01
#define B6_111100  0x3C
#define B6_111111  0x3F
#define BITS_543_001 0x20
#define BITS_543_111 0xE0
```

### Main Execution Loop (cpu.cpp)

**m68kProcess() Function**:
```cpp
void m68kProcess() {
    // Record absolute system time
    a_s_t = A_S_T;
    
    // Check for pending trace
    if(Cpu.tpend) {
        Cpu.tpend = false;
        exception(EXCEPTION_TRACE, EA_INST, 0);
    }
    
    // Check for interrupts
    check_ipl();
    
    // Fetch next opcode
    IRD = m68k_fetch(pc);
    pc += 2;
    
    // Execute instruction
    TRY_M68K_EXCEPTION
    {
        // Call instruction handler via opcode table
        (*m68k_call_table[IRD])();
        
        // Update E-clock timing
        Cpu.SyncEClock();
    }
    CATCH_M68K_EXCEPTION
    {
        // Handle CPU exceptions
        TMC68kException &e = ExceptionObject;
        e.handle();
    }
    END_M68K_EXCEPTION
}
```

**Exception Handling**:
```cpp
#define TRY_M68K_EXCEPTION 
    jmp_buf temp_excep_jump; 
    jmp_buf *oldpJmpBuf = pJmpBuf; 
    pJmpBuf = &temp_excep_jump; 
    if (setjmp(temp_excep_jump) == 0) {

#define CATCH_M68K_EXCEPTION } else {

#define END_M68K_EXCEPTION } pJmpBuf = oldpJmpBuf;

// Exception types
enum E68000Exception {
    EXCEPTION_RESET = 0,
    EXCEPTION_BUS_ERROR = 2,
    EXCEPTION_ADDRESS_ERROR = 3,
    EXCEPTION_ILLEGAL = 4,
    EXCEPTION_DIVIDE_BY_ZERO = 5,
    EXCEPTION_CHK = 6,
    EXCEPTION_TRAPV = 7,
    EXCEPTION_PRIVILEGE_VIOLATION = 8,
    EXCEPTION_TRACE = 9,
    EXCEPTION_LINE_1010 = 10,
    EXCEPTION_LINE_1111 = 11,
    // ... up to 255
};

void exception(int exception_number, int action, MEM_ADDRESS address) {
    // Build exception stack frame
    // Set PC to exception vector
    // Update SR
    // Trigger exception processing
    longjmp(*pJmpBuf, 1);
}
```

## Instruction Set Architecture

### Register Set

**Data Registers (D0-D7)**:
- 32-bit general-purpose registers
- Can be accessed as byte, word, or long
- Used for arithmetic, logical, and data operations

**Address Registers (A0-A7)**:
- 32-bit address registers
- A7 is used as the stack pointer (SP)
- Can be used for address calculations and data storage
- Cannot be used as destinations for byte operations

**Special Registers**:
- **PC (Program Counter)**: 32-bit, but only 24 bits used (0x000000-0xFFFFFF)
- **SR (Status Register)**: 16-bit
  - Bits 0-4: Condition Code Register (CCR)
  - Bits 8-10: Interrupt Priority Level (IPL)
  - Bits 13-15: Supervisor/Trace/Interrupt masks

### Status Register (SR) Layout

```
15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0
T  S  I  -  -  IPL2 IPL1 IPL0 -  X  N  Z  V  C

T: Trace flag (1 = trace mode)
S: Supervisor flag (1 = supervisor mode)
I: Interrupt mask (1 = interrupts disabled)
IPL: Interrupt Priority Level (0-7)
X: Extend flag (carry from previous operation)
N: Negative flag (result < 0)
Z: Zero flag (result == 0)
V: Overflow flag (signed overflow)
C: Carry flag (unsigned overflow)
```

**Supervisor Flag (S-bit) Details**:
- Bit 13 of the Status Register
- Controls processor privilege level
- S=1: Supervisor mode (operating system, full privileges)
- S=0: User mode (application programs, restricted privileges)
- Automatically set to 1 by:
  - Hardware reset
  - Any exception (including interrupts)
  - RTE instruction (restores SR from stack, which typically has S=1)
- Can only be cleared to 0 by:
  - Supervisor mode code executing MOVE to SR
  - RTE instruction (if saved SR on stack has S=0)
- User mode attempts to modify S-bit cause Privilege Violation exception

**Interrupt Priority Level (IPL) Details**:
- Bits 10-8 of the Status Register (I2, I1, I0)
- Determines which interrupts are currently masked
- Current IPL acts as a threshold: only interrupts with priority > current IPL are acknowledged
- IPL 0: All interrupts enabled
- IPL 7: All interrupts disabled (highest priority)
- Modified by:
  - Interrupt acknowledgment (automatically sets IPL to 7)
  - RTE instruction (restores IPL from saved SR)
  - STOP instruction (can set specific IPL in immediate word)

**Flag Access Macros**:
```cpp
// Individual flag access
BYTE &pswT = uflags.d8[7];  // Trace flag
BYTE &pswS = uflags.d8[6];  // Supervisor flag
BYTE &pswI = uflags.d8[5];  // Interrupt mask
BYTE &pswX = uflags.d8[4];  // Extend flag
BYTE &pswN = uflags.d8[3];  // Negative flag
BYTE &pswZ = uflags.d8[2];  // Zero flag
BYTE &pswV = uflags.d8[1];  // Overflow flag
BYTE &pswC = uflags.d8[0];  // Carry flag

// SR bit definitions
#define SR_C    0x0001  // Carry
#define SR_V    0x0002  // Overflow
#define SR_Z    0x0004  // Zero
#define SR_N    0x0008  // Negative
#define SR_X    0x0010  // Extend
#define SR_IPL  0x0700  // Interrupt Priority Level
#define SR_S    0x2000  // Supervisor
#define SR_T    0x8000  // Trace
```

### Addressing Modes

**Effective Address Calculation** (cpu_ea.cpp):

The MC68000 supports 18 addressing modes, implemented in Steem SSE with specialized functions for each mode and data size combination.

**Addressing Mode Types**:
```cpp
// Addressing mode function pointers
void (*m68k_jsr_get_source_b[8])();      // Byte source
void (*m68k_jsr_get_source_w[8])();      // Word source
void (*m68k_jsr_get_source_l[8])();      // Long source
void (*m68k_jsr_get_dest_b[8])();        // Byte destination
void (*m68k_jsr_get_dest_w[8])();        // Word destination
void (*m68k_jsr_get_dest_l[8])();        // Long destination
// ... additional variants for special cases
```

**Addressing Mode Implementation**:

1. **Inherent Mode**:
   - Operand is implicit in instruction
   - Example: `CLR.L D0`

2. **Immediate Mode**:
   - Operand is part of instruction
   - Example: `MOVEQ #5,D0`

3. **Absolute Data Addressing**:
   - Direct memory address
   - Example: `MOVE.B $1234,D0`

4. **Absolute Address Addressing**:
   - Memory address contains effective address
   - Example: `MOVE.B ($1234),D0`

5. **Register Direct**:
   - Operand is in register
   - Example: `MOVE.L D1,D2`

6. **Register Indirect**:
   - Address is in register
   - Example: `MOVE.B (A0),D0`

7. **Register Indirect with Post-increment**:
   - Address is in register, then increment
   - Example: `MOVE.B (A0)+,D0`

8. **Register Indirect with Pre-decrement**:
   - Decrement register, then use as address
   - Example: `MOVE.B -(A0),D0`

9. **Register Indirect with Displacement**:
   - Address = register + displacement
   - Example: `MOVE.B $12(A0),D0`

10. **Register Indirect with Index**:
    - Address = register + index register
    - Example: `MOVE.B (A0,D1),D0`

11. **Program Counter Relative with Displacement**:
    - Address = PC + displacement
    - Example: `MOVE.B $12(PC),D0`

12. **Program Counter Relative with Index**:
    - Address = PC + index register
    - Example: `MOVE.B $12(PC,D1),D0`

13. **Program Counter Memory Indirect with Displacement**:
    - Address = memory at (PC + displacement)
    - Example: `MOVE.B ($12,PC),D0`

14. **Program Counter Memory Indirect with Index**:
    - Address = memory at (PC + displacement) + index
    - Example: `MOVE.B ($12,PC,D1),D0`

15. **Memory Indirect Post-indexed**:
    - Address = (register) + (index register)
    - Example: `MOVE.B (A0,D1.W),D0`

16. **Memory Indirect Pre-indexed**:
    - Address = (register + displacement) + index
    - Example: `MOVE.B ($12,A0,D1.W),D0`

**Effective Address Calculation Functions**:
```cpp
// Get effective address for various modes
void get_ea_abs_w() {  // Absolute word
    effective_address = (LONG)(WORD)m68k_fetch(pc);
    pc += 2;
}

void get_ea_abs_l() {  // Absolute long
    effective_address = m68k_fetch(pc);
    pc += 4;
}

void get_ea_reg_direct() {  // Register direct
    effective_address = m68k_dst_l;
}

void get_ea_reg_indirect() {  // Register indirect
    effective_address = AREG(rx);
}

void get_ea_reg_indirect_postinc() {  // Post-increment
    effective_address = AREG(rx);
    AREG(rx) += (1 << (1 - (op & 1)));  // 1 for byte, 2 for word, 4 for long
}

void get_ea_reg_indirect_predec() {  // Pre-decrement
    AREG(rx) -= (1 << (1 - (op & 1)));
    effective_address = AREG(rx);
}

void get_ea_reg_indirect_disp() {  // Displacement
    effective_address = AREG(rx) + (LONG)(WORD)m68k_fetch(pc);
    pc += 2;
}

void get_ea_reg_indirect_index() {  // Index
    effective_address = AREG(rx) + REGL(ry);
}

void get_ea_pc_disp() {  // PC relative with displacement
    effective_address = pc + (LONG)(WORD)m68k_fetch(pc);
    pc += 2;
}

void get_ea_pc_index() {  // PC relative with index
    effective_address = pc + (LONG)(WORD)m68k_fetch(pc) + REGL(ry);
    pc += 2;
}
```

## Instruction Families and Implementation

### Instruction Classification

The MC68000 instruction set is organized into several families, each with specific implementations in `cpu_op.cpp`.

#### 1. Data Movement Instructions

**MOVE Family**:
```cpp
// MOVE.B (source) -> (destination)
void m68k_move_b() {
    get_source_b();  // Get source operand
    get_ea();        // Calculate effective address
    
    // Perform move based on destination mode
    switch(ea_mode) {
        case MODE_DREG:  // Move to data register
            REGB(dest) = resultb;
            break;
        case MODE_AREG:  // Move to address register (sign-extended)
            AREG(dest) = (LONG)(signed char)resultb;
            break;
        case MODE_MEMORY:  // Move to memory
            m68k_poke_abus(resultb);
            break;
        // ... other modes
    }
    
    // Set flags (MOVE doesn't affect flags)
    CLEAR_VC;
    SR_CHECK_Z_AND_N_B;
}

// MOVE.W, MOVE.L follow similar patterns
void m68k_move_w() { /* ... */ }
void m68k_move_l() { /* ... */ }

// Special MOVE variants
void m68k_movea_w() {  // MOVE to address register (word)
    get_source_w();
    AREG(dest) = (LONG)(WORD)resultw;
}

void m68k_movea_l() {  // MOVE to address register (long)
    get_source_l();
    AREG(dest) = resultl;
}

// MOVEQ (Quick Move)
void m68k_moveq() {
    BYTE data = m68k_fetch(pc);
    pc++;
    REGL(dest) = (LONG)(signed char)data;
    CLEAR_VC;
    SR_CHECK_Z_AND_N_L;
}

// MOVE from SR
void m68k_move_to_sr() {
    get_source_w();
    if(!Cpu.IsPriv(IRD)) {
        exception(EXCEPTION_PRIVILEGE_VIOLATION, EA_INST, 0);
    }
    SR = resultw;
    update_flags_from_sr();
}

// MOVE from CCR
void m68k_move_to_ccr() {
    get_source_w();
    if(!Cpu.IsPriv(IRD)) {
        exception(EXCEPTION_PRIVILEGE_VIOLATION, EA_INST, 0);
    }
    SR = (SR & ~0x1F) | (resultw & 0x1F);
    update_flags_from_sr();
}
```

**LEA (Load Effective Address)**:
```cpp
void m68k_lea() {
    get_ea();
    AREG(dest) = effective_address;
}
```

**PEA (Push Effective Address)**:
```cpp
void m68k_pea() {
    get_ea();
    SP -= 4;
    m68k_poke_abus(effective_address);
}
```

#### 2. Arithmetic Instructions

**ADD Family**:
```cpp
// ADD.B Dn, <ea>
void m68k_add_b() {
    get_source_b();
    get_ea();
    
    m68k_dst_b = m68k_peek(effective_address);
    resultb = m68k_src_b + m68k_dst_b;
    
    // Set flags
    SR_ADD_B;
    
    // Store result
    m68k_poke_abus(resultb);
}

// ADD.W, ADD.L follow similar patterns
void m68k_add_w() { /* ... */ }
void m68k_add_l() { /* ... */ }

// ADDI (Add Immediate)
void m68k_addi_b() {
    get_source_b();  // Immediate value
    get_ea();
    
    m68k_dst_b = m68k_peek(effective_address);
    resultb = m68k_src_b + m68k_dst_b;
    
    SR_ADD_B;
    m68k_poke_abus(resultb);
}

// ADDQ (Add Quick)
void m68k_addq_b() {
    BYTE data = (op & 7) + 1;  // Quick value (1-8)
    get_ea();
    
    m68k_dst_b = m68k_peek(effective_address);
    resultb = data + m68k_dst_b;
    
    SR_ADD_B;
    m68k_poke_abus(resultb);
}
```

**SUB Family**:
```cpp
// SUB.B <ea>, Dn
void m68k_sub_b() {
    get_source_b();
    get_ea();
    
    m68k_dst_b = REGB(dest);
    resultb = m68k_dst_b - m68k_src_b;
    
    // Set flags with extend
    SR_SUB_B(true);
    
    REGB(dest) = resultb;
}

// SUBI (Subtract Immediate)
void m68k_subi_b() {
    get_source_b();
    get_ea();
    
    m68k_dst_b = m68k_peek(effective_address);
    resultb = m68k_dst_b - m68k_src_b;
    
    SR_SUB_B(false);  // Don't extend for SUBI
    m68k_poke_abus(resultb);
}

// SUBQ (Subtract Quick)
void m68k_subq_b() {
    BYTE data = (op & 7) + 1;
    get_ea();
    
    m68k_dst_b = m68k_peek(effective_address);
    resultb = m68k_dst_b - data;
    
    SR_SUB_B(false);
    m68k_poke_abus(resultb);
}
```

**NEG Family**:
```cpp
// NEG.B <ea>
void m68k_neg_b() {
    get_ea();
    
    m68k_src_b = 0;
    m68k_dst_b = m68k_peek(effective_address);
    resultb = m68k_src_b - m68k_dst_b;
    
    SR_SUB_B(true);  // Extend for NEG
    
    m68k_poke_abus(resultb);
}

// NEGX (Negate with Extend)
void m68k_negx_b() {
    get_ea();
    
    m68k_src_b = 0;
    m68k_dst_b = m68k_peek(effective_address);
    resultb = m68k_src_b - m68k_dst_b - pswX;
    
    SR_SUB_B(true);
    
    m68k_poke_abus(resultb);
}
```

**CMP Family**:
```cpp
// CMP.B <ea>, Dn
void m68k_cmp_b() {
    get_source_b();
    get_ea();
    
    m68k_dst_b = REGB(dest);
    resultb = m68k_dst_b - m68k_src_b;
    
    SR_SUB_B(false);  // Don't extend for CMP
    // Result not stored
}

// CMPM (Compare Memory)
void m68k_cmpm_b() {
    get_source_b();
    get_ea();
    
    m68k_dst_b = m68k_peek(effective_address);
    resultb = m68k_dst_b - m68k_src_b;
    
    SR_SUB_B(false);
}
```

#### 3. Logical Instructions

**AND Family**:
```cpp
// AND.B <ea>, Dn
void m68k_and_b() {
    get_source_b();
    get_ea();
    
    m68k_dst_b = REGB(dest);
    resultb = m68k_dst_b & m68k_src_b;
    
    SR_CHECK_AND_B;
    REGB(dest) = resultb;
}

// ANDI (AND Immediate)
void m68k_andi_b() {
    get_source_b();
    get_ea();
    
    m68k_dst_b = m68k_peek(effective_address);
    resultb = m68k_dst_b & m68k_src_b;
    
    SR_CHECK_AND_B;
    m68k_poke_abus(resultb);
}
```

**OR Family**:
```cpp
// OR.B <ea>, Dn
void m68k_or_b() {
    get_source_b();
    get_ea();
    
    m68k_dst_b = REGB(dest);
    resultb = m68k_dst_b | m68k_src_b;
    
    SR_CHECK_AND_B;  // Same flags as AND
    REGB(dest) = resultb;
}

// ORI (OR Immediate)
void m68k_ori_b() {
    get_source_b();
    get_ea();
    
    m68k_dst_b = m68k_peek(effective_address);
    resultb = m68k_dst_b | m68k_src_b;
    
    SR_CHECK_AND_B;
    m68k_poke_abus(resultb);
}
```

**EOR Family**:
```cpp
// EOR.B <ea>, Dn
void m68k_eor_b() {
    get_source_b();
    get_ea();
    
    m68k_dst_b = REGB(dest);
    resultb = m68k_dst_b ^ m68k_src_b;
    
    SR_CHECK_AND_B;  // Same flags as AND
    REGB(dest) = resultb;
}

// EORI (EOR Immediate)
void m68k_eori_b() {
    get_source_b();
    get_ea();
    
    m68k_dst_b = m68k_peek(effective_address);
    resultb = m68k_dst_b ^ m68k_src_b;
    
    SR_CHECK_AND_B;
    m68k_poke_abus(resultb);
}
```

**NOT**:
```cpp
// NOT.B <ea>
void m68k_not_b() {
    get_ea();
    
    m68k_dst_b = m68k_peek(effective_address);
    resultb = ~m68k_dst_b;
    
    SR_CHECK_AND_B;
    m68k_poke_abus(resultb);
}
```

#### 4. Shift and Rotate Instructions

**ASL/ASR (Arithmetic Shift Left/Right)**:
```cpp
// ASL.B <ea>
void m68k_asl_b() {
    get_ea();
    
    m68k_dst_b = m68k_peek(effective_address);
    
    // Check shift count
    int shift = (op & 7) ? (op & 7) : REGB(0) & 0x3F;
    
    // Perform shift
    resultb = m68k_dst_b;
    pswC = (resultb & 0x80) != 0;
    resultb <<= 1;
    if(shift > 1) {
        resultb <<= (shift - 1);
        pswC = (resultb & 0x80) != 0;
    }
    
    // Set flags
    pswN = (resultb & 0x80) != 0;
    pswZ = (resultb == 0);
    pswV = pswN ^ pswC;
    pswC = (resultb & 0x80) != 0;
    
    m68k_poke_abus(resultb);
}

// ASR.B <ea>
void m68k_asr_b() {
    get_ea();
    
    m68k_dst_b = m68k_peek(effective_address);
    int shift = (op & 7) ? (op & 7) : REGB(0) & 0x3F;
    
    // Arithmetic shift (sign-extended)
    resultb = m68k_dst_b;
    bool sign = (resultb & 0x80) != 0;
    
    for(int i = 0; i < shift; i++) {
        pswC = (resultb & 1) != 0;
        resultb >>= 1;
        if(sign) resultb |= 0x80;
    }
    
    pswN = sign;
    pswZ = (resultb == 0);
    pswV = false;  // ASR never overflows
    
    m68k_poke_abus(resultb);
}
```

**LSL/LSR (Logical Shift Left/Right)**:
```cpp
// LSL.B <ea>
void m68k_lsl_b() {
    get_ea();
    
    m68k_dst_b = m68k_peek(effective_address);
    int shift = (op & 7) ? (op & 7) : REGB(0) & 0x3F;
    
    resultb = m68k_dst_b << shift;
    
    pswN = (resultb & 0x80) != 0;
    pswZ = (resultb == 0);
    pswV = pswN ^ ((m68k_dst_b >> (8 - shift)) & 1);
    pswC = (m68k_dst_b >> (8 - shift)) & 1;
    
    m68k_poke_abus(resultb);
}

// LSR.B <ea>
void m68k_lsr_b() {
    get_ea();
    
    m68k_dst_b = m68k_peek(effective_address);
    int shift = (op & 7) ? (op & 7) : REGB(0) & 0x3F;
    
    resultb = m68k_dst_b >> shift;
    
    pswN = false;  // LSR clears N
    pswZ = (resultb == 0);
    pswV = false;  // LSR never overflows
    pswC = (m68k_dst_b >> (shift - 1)) & 1;
    
    m68k_poke_abus(resultb);
}
```

**ROL/ROR (Rotate Left/Right)**:
```cpp
// ROL.B <ea>
void m68k_rol_b() {
    get_ea();
    
    m68k_dst_b = m68k_peek(effective_address);
    int shift = (op & 7) ? (op & 7) : REGB(0) & 0x3F;
    shift %= 9;  // Max 8 bits + carry
    
    for(int i = 0; i < shift; i++) {
        bool new_c = (m68k_dst_b & 0x80) != 0;
        m68k_dst_b = (m68k_dst_b << 1) | pswC;
        pswC = new_c;
    }
    
    resultb = m68k_dst_b;
    pswN = (resultb & 0x80) != 0;
    pswZ = (resultb == 0);
    pswV = false;
    
    m68k_poke_abus(resultb);
}

// ROR.B <ea>
void m68k_ror_b() {
    get_ea();
    
    m68k_dst_b = m68k_peek(effective_address);
    int shift = (op & 7) ? (op & 7) : REGB(0) & 0x3F;
    shift %= 9;
    
    for(int i = 0; i < shift; i++) {
        bool new_c = (m68k_dst_b & 1) != 0;
        m68k_dst_b = (m68k_dst_b >> 1) | (pswC << 7);
        pswC = new_c;
    }
    
    resultb = m68k_dst_b;
    pswN = (resultb & 0x80) != 0;
    pswZ = (resultb == 0);
    pswV = false;
    
    m68k_poke_abus(resultb);
}
```

**ROXL/ROXR (Rotate with Extend)**:
```cpp
// ROXL.B <ea>
void m68k_roxl_b() {
    get_ea();
    
    m68k_dst_b = m68k_peek(effective_address);
    int shift = (op & 7) ? (op & 7) : REGB(0) & 0x3F;
    shift %= 9;
    
    for(int i = 0; i < shift; i++) {
        bool new_x = (m68k_dst_b & 0x80) != 0;
        m68k_dst_b = (m68k_dst_b << 1) | pswX;
        pswX = pswC;
        pswC = new_x;
    }
    
    resultb = m68k_dst_b;
    pswN = (resultb & 0x80) != 0;
    pswZ = (resultb == 0);
    pswV = false;
    
    m68k_poke_abus(resultb);
}
```

#### 5. Bit Manipulation Instructions

**BTST (Bit Test)**:
```cpp
// BTST.B Dn, <ea>
void m68k_btst() {
    get_source_b();
    get_ea();
    
    BYTE bit = m68k_src_b & 7;  // Bit number (0-7)
    m68k_dst_b = m68k_peek(effective_address);
    
    pswZ = ((m68k_dst_b >> bit) & 1) == 0;
    // Other flags unchanged
}

// BTST.B #imm, <ea>
void m68k_btst_imm() {
    BYTE imm = m68k_fetch(pc);
    pc++;
    get_ea();
    
    BYTE bit = imm & 7;
    m68k_dst_b = m68k_peek(effective_address);
    
    pswZ = ((m68k_dst_b >> bit) & 1) == 0;
}
```

**BSET/BCLR (Bit Set/Clear)**:
```cpp
// BSET.B Dn, <ea>
void m68k_bset() {
    get_source_b();
    get_ea();
    
    BYTE bit = m68k_src_b & 7;
    m68k_dst_b = m68k_peek(effective_address);
    
    pswZ = ((m68k_dst_b >> bit) & 1) == 0;
    m68k_dst_b |= (1 << bit);
    
    m68k_poke_abus(m68k_dst_b);
}

// BCLR.B Dn, <ea>
void m68k_bclr() {
    get_source_b();
    get_ea();
    
    BYTE bit = m68k_src_b & 7;
    m68k_dst_b = m68k_peek(effective_address);
    
    pswZ = ((m68k_dst_b >> bit) & 1) == 0;
    m68k_dst_b &= ~(1 << bit);
    
    m68k_poke_abus(m68k_dst_b);
}
```

**BCHG (Bit Change)**:
```cpp
// BCHG.B Dn, <ea>
void m68k_bchg() {
    get_source_b();
    get_ea();
    
    BYTE bit = m68k_src_b & 7;
    m68k_dst_b = m68k_peek(effective_address);
    
    pswZ = ((m68k_dst_b >> bit) & 1) == 0;
    m68k_dst_b ^= (1 << bit);
    
    m68k_poke_abus(m68k_dst_b);
}
```

#### 6. Branch Instructions

**BRA (Branch Always)**:
```cpp
// BRA.W <displacement>
void m68k_bra_w() {
    WORD disp = m68k_fetch(pc);
    pc += 2;
    
    NEW_PC(pc + (LONG)(WORD)disp);
}

// BRA.L <displacement>
void m68k_bra_l() {
    LONG disp = m68k_fetch(pc);
    pc += 4;
    
    NEW_PC(pc + disp);
}
```

**Conditional Branches**:
```cpp
// BEQ.W <displacement> (Branch if Equal)
void m68k_beq_w() {
    WORD disp = m68k_fetch(pc);
    pc += 2;
    
    if(pswZ) {
        NEW_PC(pc + (LONG)(WORD)disp);
    }
}

// BNE.W <displacement> (Branch if Not Equal)
void m68k_bne_w() {
    WORD disp = m68k_fetch(pc);
    pc += 2;
    
    if(!pswZ) {
        NEW_PC(pc + (LONG)(WORD)disp);
    }
}

// BCS.W <displacement> (Branch if Carry Set)
void m68k_bcs_w() {
    WORD disp = m68k_fetch(pc);
    pc += 2;
    
    if(pswC) {
        NEW_PC(pc + (LONG)(WORD)disp);
    }
}

// BCC.W <displacement> (Branch if Carry Clear)
void m68k_bcc_w() {
    WORD disp = m68k_fetch(pc);
    pc += 2;
    
    if(!pswC) {
        NEW_PC(pc + (LONG)(WORD)disp);
    }
}

// Condition code macros
#define CC_T (true)   // Always true
#define CC_F (false)  // Always false
#define CC_HI (!pswC && !pswZ)  // Higher
#define CC_LS (pswC || pswZ)    // Lower or same
#define CC_CC (!pswC)          // Carry clear
#define CC_CS (pswC)           // Carry set
#define CC_NE (!pswZ)          // Not equal
#define CC_EQ (pswZ)           // Equal
#define CC_VC (!pswV)          // Overflow clear
#define CC_VS (pswV)           // Overflow set
#define CC_PL (!pswN)          // Plus
#define CC_MI (pswN)           // Minus
#define CC_GE (pswV ^ !pswN)   // Greater than or equal
#define CC_LT (pswV ^ pswN)    // Less than
#define CC_GT (!pswZ && (pswV ^ !pswN))  // Greater than
#define CC_LE (pswZ || (pswV ^ pswN))   // Less than or equal
```

**DBcc (Decrement and Branch if Condition Code)**:
```cpp
// DBRA.W Dn, <displacement> (Decrement and Branch Always)
void m68k_dbra_w() {
    WORD disp = m68k_fetch(pc);
    pc += 2;
    
    REGW(rx)--;
    if(REGW(rx) != -1) {
        NEW_PC(pc + (LONG)(WORD)disp);
    }
}

// DBEQ.W Dn, <displacement> (Decrement and Branch if Equal)
void m68k_dbeq_w() {
    WORD disp = m68k_fetch(pc);
    pc += 2;
    
    REGW(rx)--;
    if(REGW(rx) != -1 && pswZ) {
        NEW_PC(pc + (LONG)(WORD)disp);
    }
}
```

**SBcd/ABcd (BCD Subtract/Add)**:
```cpp
// SBCD.B Dn, Dn (BCD Subtract with Extend)
void m68k_sbcd_b() {
    BYTE src = REGB(rx);
    BYTE dst = REGB(ry);
    
    WORD res = dst - src - pswX;
    
    if(res > 0x99 || (res & 0xF) > 9) {
        res -= 0x60;
        pswC = true;
    } else {
        pswC = false;
    }
    
    res &= 0xFF;
    if(res > 0x99) {
        res -= 0x60;
        pswC = true;
    }
    
    pswX = pswC;
    pswV = ((src ^ dst ^ res) & 0x80) != 0;
    pswZ = (res == 0);
    pswN = (res & 0x80) != 0;
    
    REGB(ry) = res;
}
```

#### 7. Jump and Subroutine Instructions

**JMP (Jump)**:
```cpp
// JMP <ea>
void m68k_jmp() {
    get_ea();
    NEW_PC(effective_address);
}
```

**JSR (Jump to Subroutine)**:
```cpp
// JSR <ea>
void m68k_jsr() {
    get_ea();
    
    // Push return address
    SP -= 4;
    m68k_poke_abus(pc);
    
    // Jump to subroutine
    NEW_PC(effective_address);
}
```

**RTS (Return from Subroutine)**:
```cpp
// RTS
void m68k_rts() {
    // Pop return address
    pc = m68k_peek(SP);
    SP += 4;
}
```

**BSR (Branch to Subroutine)**:
```cpp
// BSR.W <displacement>
void m68k_bsr_w() {
    WORD disp = m68k_fetch(pc);
    pc += 2;
    
    // Push return address
    SP -= 4;
    m68k_poke_abus(pc);
    
    // Branch
    NEW_PC(pc + (LONG)(WORD)disp);
}
```

#### 8. Stack and System Instructions

**LINK/UNLK (Link/Unlink)**:
```cpp
// LINK A6, #<displacement>
void m68k_link() {
    WORD disp = m68k_fetch(pc);
    pc += 2;
    
    // Push A6
    SP -= 4;
    m68k_poke_abus(AREG(6));
    
    // Set A6 to SP + displacement
    AREG(6) = SP;
    SP += disp;
}

// UNLK A6
void m68k_unlk() {
    // Restore SP from A6
    SP = AREG(6);
    
    // Pop A6
    AREG(6) = m68k_peek(SP);
    SP += 4;
}
```

**MOVE USP (Move User Stack Pointer)**:
```cpp
// MOVE USP, An
void m68k_move_usp_to_an() {
    if(!Cpu.IsPriv(IRD)) {
        exception(EXCEPTION_PRIVILEGE_VIOLATION, EA_INST, 0);
    }
    AREG(dest) = other_sp;
}

// MOVE An, USP
void m68k_move_an_to_usp() {
    if(!Cpu.IsPriv(IRD)) {
        exception(EXCEPTION_PRIVILEGE_VIOLATION, EA_INST, 0);
    }
    other_sp = AREG(src);
}
```

**STOP**:
```cpp
// STOP #<immediate>
void m68k_stop() {
    if(!Cpu.IsPriv(IRD)) {
        exception(EXCEPTION_PRIVILEGE_VIOLATION, EA_INST, 0);
    }
    
    WORD imm = m68k_fetch(pc);
    pc += 2;
    
    // Set SR
    SR = imm;
    update_flags_from_sr();
    
    // Enter stopped state
    Cpu.ProcessingState = TMC68000::STOPPED;
}
```

**RESET**:
```cpp
// RESET
void m68k_reset() {
    if(!Cpu.IsPriv(IRD)) {
        exception(EXCEPTION_PRIVILEGE_VIOLATION, EA_INST, 0);
    }
    
    // Perform system reset
    exception(EXCEPTION_RESET, EA_INST, 0);
}
```

#### 9. Exception Instructions

**TRAP**:
```cpp
// TRAP #<vector>
void m68k_trap() {
    BYTE vector = op & 15;
    
    // Trigger exception
    exception(EXCEPTION_TRAP_0 + vector, EA_INST, 0);
}
```

**TRAPV (Trap on Overflow)**:
```cpp
// TRAPV
void m68k_trapv() {
    if(pswV) {
        exception(EXCEPTION_TRAPV, EA_INST, 0);
    }
}
```

**CHK (Check Register Against Bounds)**:
```cpp
// CHK.W <ea>, Dn
void m68k_chk_w() {
    get_source_w();
    get_ea();
    
    WORD lower = m68k_peek(effective_address);
    WORD upper = m68k_peek(effective_address + 2);
    
    if(REGW(dest) < lower || REGW(dest) > upper) {
        exception(EXCEPTION_CHK, EA_INST, 0);
    }
}
```

#### 10. Multiply and Divide Instructions

**MULU/MULS (Multiply Unsigned/Signed)**:
```cpp
// MULU.W <ea>, Dn
void m68k_mulu_w() {
    get_source_w();
    get_ea();
    
    LONG src = (LONG)(WORD)m68k_peek(effective_address);
    LONG dst = (LONG)(WORD)REGW(dest);
    LONG result = dst * src;
    
    REGL(dest) = result;
    
    pswZ = (result == 0);
    pswN = (result & 0x80000000) != 0;
    // V and C undefined
}

// MULS.W <ea>, Dn
void m68k_muls_w() {
    get_source_w();
    get_ea();
    
    LONG src = (LONG)(WORD)m68k_peek(effective_address);
    LONG dst = (LONG)(WORD)REGW(dest);
    LONG result = dst * src;
    
    REGL(dest) = result;
    
    pswZ = (result == 0);
    pswN = (result & 0x80000000) != 0;
}
```

**DIVU/DIVS (Divide Unsigned/Signed)**:
```cpp
// DIVU.W <ea>, Dn
void m68k_divu_w() {
    get_source_w();
    get_ea();
    
    WORD divisor = m68k_peek(effective_address);
    LONG dividend = REGL(dest);
    
    if(divisor == 0) {
        exception(EXCEPTION_DIVIDE_BY_ZERO, EA_INST, 0);
    }
    
    LONG quotient = dividend / divisor;
    WORD remainder = dividend % divisor;
    
    if(quotient > 0xFFFF || quotient < -0x8000) {
        // Overflow
        pswV = true;
        pswN = (dividend & 0x80000000) != 0;
        pswZ = false;
    } else {
        REGL(dest) = (quotient & 0xFFFF) | ((remainder & 0xFFFF) << 16);
        
        pswZ = (quotient == 0);
        pswN = (quotient & 0x8000) != 0;
        pswV = false;
    }
    
    pswC = false;
}
```

#### 11. MOVEM (Move Multiple Registers)

**MOVEM.L (Move Multiple Registers Long)**:
```cpp
// MOVEM.L <register list>, <ea>
void m68k_movem_l_to_mem() {
    WORD reg_list = m68k_fetch(pc);
    pc += 2;
    get_ea();
    
    MEM_ADDRESS ad = effective_address;
    
    for(int i = 0; i < 16; i++) {
        if(reg_list & (1 << i)) {
            m68k_poke_abus(REGL(i));
            ad += 4;
        }
    }
}

// MOVEM.L <ea>, <register list>
void m68k_movem_l_from_mem() {
    WORD reg_list = m68k_fetch(pc);
    pc += 2;
    get_ea();
    
    MEM_ADDRESS ad = effective_address;
    
    for(int i = 15; i >= 0; i--) {
        if(reg_list & (1 << i)) {
            REGL(i) = m68k_peek(ad);
            ad += 4;
        }
    }
}
```

#### 12. EXT (Sign Extend)

**EXT.W (Extend Word to Long)**:
```cpp
// EXT.W Dn
void m68k_ext_w() {
    REGL(dest) = (LONG)(WORD)REGW(dest);
    
    pswN = (REGW(dest) & 0x8000) != 0;
    pswZ = (REGW(dest) == 0);
    pswV = false;
    pswC = false;
}
```

#### 13. TAS (Test and Set)

**TAS.B (Test and Set Byte)**:
```cpp
// TAS.B <ea>
void m68k_tas_b() {
    get_ea();
    
    m68k_dst_b = m68k_peek(effective_address);
    
    pswN = (m68k_dst_b & 0x80) != 0;
    pswZ = (m68k_dst_b == 0);
    pswV = false;
    pswC = false;
    
    // Set high bit
    m68k_dst_b |= 0x80;
    m68k_poke_abus(m68k_dst_b);
}
```

#### 14. SWAP

**SWAP.W (Swap Words)**:
```cpp
// SWAP.W Dn
void m68k_swap() {
    DWORD reg = REGL(dest);
    reg = ((reg & 0xFFFF) << 16) | ((reg >> 16) & 0xFFFF);
    REGL(dest) = reg;
    
    pswZ = (reg == 0);
    pswN = (reg & 0x80000000) != 0;
    pswV = false;
    pswC = false;
}
```

#### 15. NOP

**NOP (No Operation)**:
```cpp
// NOP
void m68k_nop() {
    // Do nothing
}
```

## Flag Handling

### Flag Setting Macros

**Clear Flags**:
```cpp
#define CLEAR_VC pswV = false; pswC = false
#define CLEAR_V pswV = false
#define CLEAR_C pswC = false
#define CLEAR_Z pswZ = false
#define CLEAR_N pswN = false
```

**Flag Setting for Logical Operations**:
```cpp
#define SR_CHECK_AND_B \
    CLEAR_VC; \
    SR_CHECK_Z_AND_N_B;

#define SR_CHECK_Z_AND_N_B \
    pswN = (resultb < 0); \
    pswZ = (resultb == 0);
```

**Flag Setting for Addition**:
```cpp
void sr_add_b() {
    pswV = ((((m68k_src_b & m68k_dst_b & ~resultb) | 
             (~m68k_src_b & ~m68k_dst_b & resultb)) & MSB_B) != 0);
    pswX = pswC = ((((m68k_src_b & m68k_dst_b) | 
                   (~resultb & m68k_dst_b) | 
                   (m68k_src_b & ~resultb)) & MSB_B) != 0);
    pswZ = (resultb == 0);
    pswN = ((resultb & MSB_B) != 0);
}
```

**Flag Setting for Subtraction**:
```cpp
void sr_sub_b(bool extend_flag) {
    pswV = (((((~m68k_src_b) & m68k_dst_b & ~resultb) | 
             (m68k_src_b & ~m68k_dst_b & resultb)) & MSB_B) != 0);
    pswC = (((((m68k_src_b & ~m68k_dst_b) | 
             (resultb & ~m68k_dst_b) | 
             (m68k_src_b & resultb)) & MSB_B) != 0);
    if(extend_flag)
        pswX = pswC;
    pswZ = (resultb == 0);
    pswN = (resultb < 0);
}
```

## Memory Access Functions

### Peek and Poke Functions (cpu_ea.cpp)

**Memory Read Functions**:
```cpp
BYTE m68k_peek(MEM_ADDRESS ad) {
    // Handle different memory regions
    abus = ad & 0xFFFFFFFE;  // Align to word boundary
    BYTE bit0 = ad & 1;
    
    // Determine byte index based on endianness
    #if defined(BIG_ENDIAN_PROCESSOR)
    BYTE byte_index = bit0;
    #else
    BYTE byte_index = bit0 ^ 1;
    #endif
    
    // Check memory region using Glue chip decoding
    BYTE b = (BYTE)(abus >> 16);
    switch(Glue.Decode[b]) {
        case TGlue::MMU_CONFUSED:
            return mmu_confused_peek(ad);
        case TGlue::STRAM_OR_ROM:
            if(abus >= MEM_START_OF_USER_AREA || SUPERFLAG)
                return PEEK(ad);
            else
                exception(EXCEPTION_BUS_ERROR, EA_READ, abus);
        case TGlue::DEV:
            return io_read(abus) >> (8 * byte_index);
        case TGlue::ROM:
            return ROM_PEEK(ad - rom_addr);
        case TGlue::CART:
            return CART_PEEK(ad - Glue.cartbase);
    }
}

WORD m68k_dpeek(MEM_ADDRESS ad) {
    // Double-word peek
    return (WORD)m68k_peek(ad) | ((WORD)m68k_peek(ad + 1) << 8);
}

WORD m68k_fetch(MEM_ADDRESS ad) {
    // Fetch opcode word
    // Similar to peek but with additional checks
}
```

**Memory Write Functions**:
```cpp
void m68k_poke_abus(BYTE value) {
    // Write byte to current address bus
    m68k_poke(iabus, value);
}

void m68k_poke(MEM_ADDRESS ad, BYTE value) {
    abus = ad & 0xFFFFFFFE;
    BYTE bit0 = ad & 1;
    
    #if defined(BIG_ENDIAN_PROCESSOR)
    BYTE byte_index = bit0;
    #else
    BYTE byte_index = bit0 ^ 1;
    #endif
    
    BYTE b = (BYTE)(abus >> 16);
    switch(Glue.Decode[b]) {
        case TGlue::MMU_CONFUSED:
            mmu_confused_poke(ad, value);
            break;
        case TGlue::STRAM_OR_ROM:
            if(abus >= MEM_START_OF_USER_AREA || SUPERFLAG)
                POKE(ad, value);
            else
                exception(EXCEPTION_BUS_ERROR, EA_WRITE, abus);
            break;
        case TGlue::DEV:
            io_write(abus, value << (8 * byte_index));
            break;
        case TGlue::ROM:
            // ROM is read-only
            break;
        case TGlue::CART:
            CART_POKE(ad - Glue.cartbase, value);
            break;
    }
}
```

## Timing and Cycle Counting

### Bus Cycle Emulation

**Bus State Functions**:
```cpp
// Function pointers for bus operations
void (*pBusIdle)(int t);           // Bus idle
void (*pBusWS)(int t);             // Wait states
void (*pBusPrefetchOnly)();        // Prefetch only
void (*pBusPrefetchFinal)();       // Prefetch final
void (*pBusPrefetchTotal)();       // Prefetch total
void (*pBusReadB)();               // Read byte
void (*pBusRead)();                // Read word
void (*pBusWrite)();               // Write word
void (*pBusWriteB)();              // Write byte
void (*pBusBltRead)();             // Blitter read
void (*pBusBltWrite)();            // Blitter write
```

**Timing Configuration** (cpu.cpp:SetTimingFunctions):
```cpp
void SetTimingFunctions() {
    // Different timing functions for STF, STE, Mega STE
    if(IS_STE) {
        if(SSEConfig.CpuBoosted) {
            // 2 low-level video, acceleration
            pBusIdle = BusSte2Idle;
            pBusWS = Bus2WS;
            pBusPrefetchOnly = BusSte2PrefetchOnly;
            // ...
        } else {
            // 1 low-level video, no acceleration
            pBusIdle = BusSte1Idle;
            pBusPrefetchOnly = BusSte1PrefetchOnly;
            // ...
        }
    } else if(SSEConfig.Mega) {
        // Mega STE
        pBusIdle = BusMegaIdle;
        pBusPrefetchOnly = BusMegaPrefetchOnly;
        // ...
    } else {
        // STF
        pBusIdle = BusStfIdle;
        pBusPrefetchOnly = BusStfPrefetchOnly;
        // ...
    }
}
```

### E-Clock Synchronization

**E-Clock Implementation**:
```cpp
int TMC68000::SyncEClock() {
    // Synchronize E-clock with system clock
    // Returns number of cycles consumed
    
    COUNTER_VAR cycles = ABSOLUTE_SYS_TIME - cycles0;
    
    if(eclock_sync_cycle < 8) {
        // E-clock synchronization logic
        if(cycles >= cycles_for_eclock) {
            cycles_for_eclock = 0;
            eclock_sync_cycle++;
            if(eclock_sync_cycle == 8) {
                // E-clock cycle complete
                return 1;
            }
        } else {
            cycles_for_eclock -= cycles;
        }
    }
    
    return 0;
}

void TMC68000::UpdateCyclesForEClock() {
    // Update cycle counters for E-clock
    cycles0 = ABSOLUTE_SYS_TIME;
    cycles_for_eclock = 0;
    eclock_sync_cycle = 0;
}
```

## CPU Reset and Initialization

### CPU Reset (cpu.cpp)

```cpp
void TMC68000::Reset(bool Cold) {
    // Reset registers
    for(int i = 0; i < NREGS; i++) {
        r[i] = 0;
    }
    
    // Reset PC
    if(Cold) {
        Pc = 0;  // Cold reset starts at address 0
    } else {
        // Warm reset: get reset vector from hardware
        Pc = GetResetVector();
    }
    
    // Reset SR
    SR = 0x2700;  // Supervisor mode, interrupts enabled
    update_flags_from_sr();
    
    // Reset prefetch queue
    irc = 0;
    ir = 0;
    ird = 0;
    
    // Reset processing state
    ProcessingState = NORMAL;
    tpend = false;
    
    // Reset timing
    dbi_timing = 0;
    cycles_for_eclock = 0;
    cycles0 = 0;
    BusIdleCycles = 0;
    eclock_sync_cycle = 0;
    
    // Initialize opcode table
    cpu_routines_init();
    
    // Set timing functions
    SetTimingFunctions();
}
```

## Exception Handling

### Exception Overview

The MC68000 exception system is a critical feature that enables robust operating system design. Exceptions are the processor's mechanism for handling errors, external events, and special conditions. The system provides structured control transfer to handler routines while preserving processor state.

**Exception Categories**:

1. **Synchronous Exceptions**: Caused directly by instruction execution
   - Reset, Bus Error, Address Error
   - Illegal Instruction, Divide by Zero
   - CHK, TRAPV, Privilege Violation
   - Trace, Line A/F Emulator

2. **Asynchronous Exceptions**: External events independent of instruction execution
   - Hardware Interrupts (IPL 1-7)
   - Spurious Interrupt

3. **Software-Initiated Exceptions**: Explicitly triggered by software
   - TRAP instructions (TRAP #0-TRAP #15)

### Exception Types

```cpp
enum E68000Exception {
    EXCEPTION_RESET = 0,           // Reset - system initialization
    EXCEPTION_BUS_ERROR = 2,       // Bus Error - invalid memory access
    EXCEPTION_ADDRESS_ERROR = 3,  // Address Error - alignment violation
    EXCEPTION_ILLEGAL = 4,        // Illegal Instruction - undefined opcode
    EXCEPTION_DIVIDE_BY_ZERO = 5, // Divide by Zero - division with zero divisor
    EXCEPTION_CHK = 6,            // CHK Instruction - value out of bounds
    EXCEPTION_TRAPV = 7,          // TRAPV Instruction - overflow condition
    EXCEPTION_PRIVILEGE_VIOLATION = 8, // Privilege Violation - user mode executing privileged instruction
    EXCEPTION_TRACE = 9,          // Trace - single-step execution
    EXCEPTION_LINE_1010 = 10,     // Line 1010 Emulator - 68010 emulation
    EXCEPTION_LINE_1111 = 11,     // Line 1111 Emulator - 68020 emulation
    // 12-23: Reserved
    EXCEPTION_SPURIOUS = 24,      // Spurious Interrupt - no device claimed interrupt
    EXCEPTION_LEVEL_1 = 25,       // Level 1 Interrupt
    EXCEPTION_LEVEL_2 = 26,       // Level 2 Interrupt
    EXCEPTION_LEVEL_3 = 27,       // Level 3 Interrupt
    EXCEPTION_LEVEL_4 = 28,       // Level 4 Interrupt
    EXCEPTION_LEVEL_5 = 29,       // Level 5 Interrupt
    EXCEPTION_LEVEL_6 = 30,       // Level 6 Interrupt
    EXCEPTION_LEVEL_7 = 31,       // Level 7 Interrupt (NMI)
    // 32-47: TRAP instructions (TRAP #0 to TRAP #15)
    EXCEPTION_TRAP_0 = 32,
    // ...
    EXCEPTION_TRAP_15 = 47,
    // 48-255: User-defined/reserved
};
```

**Exception Vector Table**:
- Located at addresses 0x000000 to 0x0000FF
- Each vector is 4 bytes (long word) containing handler address
- Vector address = base + (exception number × 4)
- Reset vector at 0x000004 (special case)
- Other vectors start at 0x000008

### Exception Processing Sequence

When an exception occurs, the MC68000 performs the following steps:

1. **Internal State Preservation**: Saves current PC and SR internally
2. **Vector Address Calculation**: Determines handler address from vector table
3. **Mode Transition**: Automatically switches to supervisor mode (S-bit = 1)
4. **Stack Frame Creation**:
   - Pushes current PC (4 bytes) onto supervisor stack
   - Pushes current SR (2 bytes) onto supervisor stack
   - For bus/address errors: pushes additional information (access address, instruction register, special status word)
5. **Handler Invocation**: Loads PC from exception vector address
6. **Execution Transfer**: Begins executing exception handler code

**Exception Stack Frames**:

Standard Exception Frame (most exceptions):
```
Supervisor Stack:
+------------------+
|     PC (4)      |  <- SP (before exception)
+------------------+
|     SR (2)      |  <- SP+4
+------------------+
```

Bus/Address Error Frame:
```
Supervisor Stack:
+------------------+
|     PC (4)      |  <- SP
+------------------+
|     SR (2)      |  <- SP+4
+------------------+
|  Access Address |  <- SP+6 (2 bytes for address error, 4 bytes for bus error)
+------------------+
| Instruction Reg |  <- SP+8/10 (2 bytes)
+------------------+
| Special Status |  <- SP+10/12 (2 bytes)
+------------------+
```

### Exception Handling Implementation

```cpp
void exception(int exception_number, int action, MEM_ADDRESS address) {
    // Build exception stack frame
    // Format depends on exception type
    
    // For most exceptions:
    // Push PC
    SP -= 4;
    m68k_poke(SP, pc);
    
    // Push SR
    SP -= 2;
    m68k_poke(SP, SR);
    
    // For bus/address errors, also push:
    // - Access address
    // - Instruction register
    // - Special status word
    
    // Set PC to exception vector
    MEM_ADDRESS vector_address = exception_number * 4;
    pc = m68k_peek(vector_address) | 
         ((MEM_ADDRESS)m68k_peek(vector_address + 2) << 16);
    
    // Set supervisor mode
    bool was_supervisor = (SR & SR_S) != 0;
    SR |= SR_S;  // Set supervisor bit
    update_flags_from_sr();
    
    // Set trace flag if appropriate
    if(exception_number == EXCEPTION_TRACE) {
        SR |= SR_T;
        update_flags_from_sr();
    }
    
    // Trigger longjmp to return to main loop
    ExceptionObject.number = exception_number;
    ExceptionObject.action = action;
    ExceptionObject.address = address;
    longjmp(*pJmpBuf, 1);
}
```

**Exception Action Types**:
```cpp
enum EExceptionAction {
    EA_INST,    // Instruction fetch - error occurred during opcode fetch
    EA_READ,    // Memory read - error occurred during data read
    EA_WRITE,   // Memory write - error occurred during data write
    EA_RMW      // Read-Modify-Write - error occurred during RMW operation
};
```

### Privilege Violation Exception

One of the most important exceptions for system security:

**Causes**:
- Attempt to execute privileged instruction in user mode
- Privileged instructions include: MOVE to SR, MOVE to CCR, MOVE to USP, RESET, STOP, RTE, ANDI/ORI/EORI to SR
- Attempt to modify S-bit in user mode

**Processing**:
1. Processor detects privileged instruction attempt in user mode
2. Generates Privilege Violation exception (Vector 8)
3. Automatically switches to supervisor mode
4. Saves PC and SR on supervisor stack
5. Transfers control to exception handler at address 0x000020 (8 × 4)

**Handler Responsibilities**:
- Examine saved PC to identify offending instruction
- Take appropriate action (terminate program, log error, etc.)
- Return via RTE or terminate faulty process

### Interrupt Handling

The MC68000 supports seven interrupt priority levels (IPL 1-7):

**Interrupt Priority System**:
- IPL 0: No interrupt (normal execution)
- IPL 1: Lowest priority interrupt
- IPL 7: Highest priority interrupt (Non-Maskable Interrupt)
- Current IPL in SR acts as mask: only interrupts with priority > current IPL are acknowledged

**Interrupt Processing**:
1. External device asserts interrupt at specific level
2. Processor compares with current IPL in SR
3. If interrupt priority > current IPL:
   - Processor completes current instruction
   - Acknowledges interrupt
   - Begins exception processing (same as other exceptions)
4. Interrupt handler executes
5. Handler returns via RTE

**Interrupt Vector Assignment**:
- Vectors 25-31 correspond to IPL 1-7
- Vector address = 0x000008 + (24 + ipl) × 4
- Example: IPL 3 interrupt uses vector at 0x000008 + 27 × 4 = 0x000074

**Interrupt Latency**:
- Interrupts are only recognized between instructions
- Minimum latency: time to complete current instruction + exception processing overhead
- Maximum latency: time to complete longest instruction (e.g., DIVU with 64-bit result)

### Trace Exception

The trace exception enables single-step debugging:

**Activation**:
- Set T-bit (bit 15) in SR to 1
- Can be set by supervisor code or by exception handler

**Operation**:
1. Processor executes one instruction normally
2. After instruction completion, checks T-bit
3. If T-bit is set, generates Trace exception (Vector 9)
4. Exception processing begins (switches to supervisor mode)
5. Trace handler executes
6. Handler can examine state, then clear T-bit and return via RTE

**Usage**:
- Single-step debugging
- Instruction tracing
- Software breakpoints

**Note**: Trace exception occurs AFTER the traced instruction completes, allowing the instruction to execute fully before the exception is generated.

### Exception Return (RTE)

The RTE (Return from Exception) instruction is the primary mechanism for returning from exception handlers:

**Operation**:
1. Pops SR from stack (2 bytes)
2. Pops PC from stack (4 bytes)
3. If returning to user mode (S-bit in restored SR = 0):
   - Switches from supervisor stack to user stack
   - Swaps SSP and USP
4. Resumes execution at restored PC

**Implementation**:
```cpp
// RTE instruction implementation
void m68k_rte() {
    if(!Cpu.IsPriv(IRD)) {
        exception(EXCEPTION_PRIVILEGE_VIOLATION, EA_INST, 0);
    }
    
    // Pop SR
    WORD new_sr = m68k_peek(SP);
    SP += 2;
    
    // Check if returning to user mode
    bool returning_to_user = ((new_sr & SR_S) == 0);
    
    // Pop PC
    MEM_ADDRESS new_pc = m68k_peek(SP);
    SP += 4;
    
    // If returning to user mode, switch stacks
    if(returning_to_user && SUPERFLAG) {
        // Switch from SSP to USP
        MEM_ADDRESS temp = SP;
        SP = other_sp;
        other_sp = temp;
    }
    
    // Restore SR
    SR = new_sr;
    update_flags_from_sr();
    
    // Restore PC
    pc = new_pc;
}
```

### Exception Priority

The MC68000 defines exception processing priority:

1. **Highest Priority**:
   - Reset (Vector 0)
   - Bus Error (Vector 2)
   - Address Error (Vector 3)

2. **Medium Priority**:
   - Illegal Instruction (Vector 4)
   - Divide by Zero (Vector 5)
   - CHK (Vector 6)
   - TRAPV (Vector 7)
   - Privilege Violation (Vector 8)

3. **Lower Priority**:
   - Trace (Vector 9)
   - Line A/F Emulator (Vectors 10-11)

4. **Interrupts**:
   - Higher number = higher priority (IPL 7 > IPL 6 > ... > IPL 1)

5. **TRAP Instructions**:
   - TRAP #0 (Vector 32) through TRAP #15 (Vector 47)

**Priority Rules**:
- Higher priority exceptions can preempt lower priority exception handlers
- Exceptions of same or lower priority are queued until higher priority handler completes
- Reset has absolute highest priority and cannot be masked

**Exception Action Types**:
```cpp
enum EExceptionAction {
    EA_INST,    // Instruction fetch
    EA_READ,    // Memory read
    EA_WRITE,   // Memory write
    EA_RMW      // Read-Modify-Write
};

const char* exception_action_name[4] = {
    "read from",
    "write to",
    "fetch from",
    "instruction execution"
};
```

**Exception Structure**:
```cpp
struct TMC68kException {
    int number;           // Exception number
    int action;           // Action type
    MEM_ADDRESS address;  // Address involved
    WORD sr;              // SR at exception
    MEM_ADDRESS pc;      // PC at exception
    WORD ird;            // Current opcode
    BYTE bombs;          // Bomb number (for exceptions)
    WORD crash_sr;       // SR at crash
    MEM_ADDRESS ucrash_address; // Crash address
    
    void crash();        // Crash handler
    void handle();       // Exception handler
};

TMC68kException ExceptionObject;
```

**Exception Handling Function**:
```cpp
void exception(int exception_number, int action, MEM_ADDRESS address) {
    // Build exception stack frame
    // Format depends on exception type
    
    // For most exceptions:
    // Push PC
    SP -= 4;
    m68k_poke(SP, pc);
    
    // Push SR
    SP -= 2;
    m68k_poke(SP, SR);
    
    // For bus/address errors, also push:
    // - Access address
    // - Instruction register
    // - Special status word
    
    // Set PC to exception vector
    MEM_ADDRESS vector_address = exception_number * 4;
    pc = m68k_peek(vector_address) | 
         ((MEM_ADDRESS)m68k_peek(vector_address + 2) << 16);
    
    // Set supervisor mode
    bool was_supervisor = (SR & SR_S) != 0;
    SR |= SR_S;  // Set supervisor bit
    update_flags_from_sr();
    
    // Set trace flag if appropriate
    if(exception_number == EXCEPTION_TRACE) {
        SR |= SR_T;
        update_flags_from_sr();
    }
    
    // Trigger longjmp to return to main loop
    ExceptionObject.number = exception_number;
    ExceptionObject.action = action;
    ExceptionObject.address = address;
    longjmp(*pJmpBuf, 1);
}
```

## CPU State Management

### Processing States

```cpp
enum ProcessingState {
    NORMAL,      // Normal instruction execution
    STOPPED,     // Stopped (STOP instruction)
    EXCEPTION,   // Exception processing
    HALTED       // Halted
};
```

### Privilege Checking

The MC68000 enforces strict privilege checking to maintain system security. The `IsPriv` function determines whether the current processor mode can execute a specific instruction.

**Privileged Instructions** (require supervisor mode):
- **MOVE to SR** (0x4E40-0x4E5F): Modifies status register, can change mode bits
- **MOVE to CCR** (0x4E60-0x4E7F): Modifies condition code register
- **MOVE to USP** (0x4E70-0x4E7F): Accesses user stack pointer
- **RESET** (0x4E80-0x4E9F): Resets external hardware
- **STOP** (0x4E90-0x4EAF): Stops processor execution
- **RTE** (0x4EA0-0x4EBF): Returns from exception, restores PC and SR
- **ANDI to SR** (0x023C): Logical AND with status register
- **ORI to SR** (0x003C): Logical OR with status register
- **EORI to SR** (0x0A3C): Logical XOR with status register

**Privilege Violation Handling**:
When a user mode program attempts to execute a privileged instruction:
1. Processor detects the attempt
2. Generates Privilege Violation exception (Vector 8)
3. Automatically switches to supervisor mode
4. Saves current PC and SR on supervisor stack
5. Transfers control to exception handler at 0x000020
6. Exception handler can examine the saved state and take appropriate action

```cpp
bool TMC68000::IsPriv(WORD op) {
    // Check if current mode can execute privileged instruction
    // Some instructions require supervisor mode
    
    if((op & 0xFFC0) == 0x4E40) { // MOVE to SR
        return (SR & SR_S) != 0;
    }
    if((op & 0xFFC0) == 0x4E60) { // MOVE to CCR
        return (SR & SR_S) != 0;
    }
    if((op & 0xFFC0) == 0x4E70) { // MOVE to USP
        return (SR & SR_S) != 0;
    }
    if((op & 0xFFC0) == 0x4E80) { // RESET
        return (SR & SR_S) != 0;
    }
    if((op & 0xFFC0) == 0x4E90) { // STOP
        return (SR & SR_S) != 0;
    }
    if((op & 0xFFC0) == 0x4EA0) { // RTE
        return (SR & SR_S) != 0;
    }
    
    return true;  // Most instructions don't require privilege
}
```

### User/Supervisor Mode Transitions

The MC68000 enforces strict rules for mode transitions to maintain system security:

**User → Supervisor Transition**:
- **Only Method**: Via exceptions (including interrupts)
- **Automatic**: Any exception automatically sets S-bit to 1
- **Cannot be Bypassed**: User code cannot directly switch to supervisor mode
- **Security**: Attempting to set S-bit in user mode causes Privilege Violation

**Supervisor → User Transition**:
- **Method 1**: Clear S-bit in SR using MOVE instruction
  ```cpp
  // Supervisor code switching to user mode
  MOVE.W #0x0000,SR  // Clear all status bits including S
  ```
- **Method 2**: Use RTE instruction (restores SR from stack)
  ```cpp
  // Exception handler returning to user mode
  RTE  // Restores SR from stack, which may have S=0
  ```
- **Method 3**: Use MOVE USP instruction to switch stack pointers

**Mode Transition Implementation**:
```cpp
// Mode switching functions
inline void change_to_supervisor_mode() {
    if(!SUPERFLAG) {
        MEM_ADDRESS temp = SP; SP = other_sp; other_sp = temp;
        SET_S;
    }
}

inline void detect_change_to_user_mode() {
    if(!SUPERFLAG) {
        MEM_ADDRESS temp = SP; SP = other_sp; other_sp = temp;
        CLEAR_S;
    }
}
```

### Dual Stack Pointer Security

The dual stack pointer mechanism provides critical security benefits:

**Security Features**:
1. **Stack Isolation**: User and supervisor stacks are completely separate
2. **Access Control**: User code cannot access supervisor stack pointer
3. **Error Containment**: User stack corruption doesn't affect supervisor stack
4. **Recovery Capability**: Supervisor can always recover from user errors

**Example Scenario**:
1. User program has a bug that corrupts its stack pointer
2. User program executes RTS with corrupted stack
3. Processor jumps to random address, eventually causes exception
4. Exception processing automatically switches to supervisor mode
5. Supervisor stack pointer (SSP) is intact and unused by user code
6. Exception handler can examine error, clean up, and continue system operation
7. Faulty user program can be terminated without system crash

**Stack Pointer Access**:
- **User Mode**: Can only access USP (A7)
- **Supervisor Mode**: Can access both SSP and USP
  - SSP is the active A7
  - USP can be accessed via MOVE USP instruction

**MOVE USP Instruction**:
```cpp
// MOVE USP, An - Read USP into address register
void m68k_move_usp_to_an() {
    if(!Cpu.IsPriv(IRD)) {
        exception(EXCEPTION_PRIVILEGE_VIOLATION, EA_INST, 0);
    }
    AREG(dest) = other_sp;  // other_sp holds USP when in supervisor mode
}

// MOVE An, USP - Write address register to USP
void m68k_move_an_to_usp() {
    if(!Cpu.IsPriv(IRD)) {
        exception(EXCEPTION_PRIVILEGE_VIOLATION, EA_INST, 0);
    }
    other_sp = AREG(src);  // Set USP to address register value
}
```

### Memory Protection with MMU

While the MC68000 itself doesn't include an MMU, it provides signals that enable external MMU implementation:

**MMU Integration**:
- Processor outputs mode signals (user/supervisor) during memory accesses
- MMU uses these signals to validate each memory access
- Access violations generate bus error or address error exceptions

**Memory Space Partitioning**:
- **Supervisor Space**: Accessible only in supervisor mode
- **User Space**: Accessible in both modes
- **Read-Only Regions**: Cannot be written to in any mode
- **Device Space**: Memory-mapped I/O regions

**MMU Operation**:
1. Processor generates address and control signals (R/W, mode, access type)
2. MMU checks address against permission table
3. If access is legal: memory operation proceeds
4. If access is illegal: MMU signals bus error to processor
5. Processor generates Bus Error exception (Vector 2)

**Benefits**:
- Operating system memory protected from user programs
- User programs protected from each other
- Hardware devices protected from unauthorized access
- System stability improved through memory isolation

## User/Supervisor Mode Practical Applications

### Operating System Design

The user/supervisor mode mechanism is fundamental to operating system architecture on the MC68000:

**OS Kernel**:
- Runs in supervisor mode with full privileges
- Manages system resources (memory, devices, processes)
- Handles exceptions and interrupts
- Provides system services to user programs

**User Applications**:
- Run in user mode with restricted privileges
- Access system services via TRAP instructions or system calls
- Cannot directly access hardware or OS memory
- Errors are contained and handled by OS

**System Call Mechanism**:
1. User program needs OS service (e.g., file I/O)
2. User program executes TRAP instruction (e.g., TRAP #1 for file operations)
3. Processor generates TRAP exception (Vector 32 + trap number)
4. Control transfers to OS TRAP handler
5. OS handler performs requested service
6. OS handler returns via RTE to user program

### Error Handling and System Robustness

The exception and mode mechanism provides robust error handling:

**Error Containment**:
- User program errors generate exceptions
- Control transfers to OS exception handler
- OS can examine error context and take appropriate action
- Faulty program can be terminated without affecting other programs

**Example Error Scenarios**:

1. **Division by Zero**:
   - User program attempts DIVU by zero
   - Processor generates Divide by Zero exception (Vector 5)
   - OS handler can log error, notify user, terminate program

2. **Illegal Instruction**:
   - User program executes undefined opcode
   - Processor generates Illegal Instruction exception (Vector 4)
   - OS handler can identify faulty program and terminate it

3. **Address Error**:
   - User program attempts word access to odd address
   - Processor generates Address Error exception (Vector 3)
   - OS handler can examine access address and take action

4. **Privilege Violation**:
   - User program attempts to execute STOP instruction
   - Processor generates Privilege Violation exception (Vector 8)
   - OS handler can terminate untrusted program

**Recovery Mechanisms**:
- **Program Termination**: OS can clean up resources and load new program
- **Error Logging**: OS can record error details for debugging
- **Safe State**: OS can put system in safe state for critical errors
- **Retry**: For transient errors, OS can attempt to retry operation

### Memory Protection Benefits

The combination of user/supervisor modes and MMU provides comprehensive memory protection:

**Protection Levels**:
1. **Instruction Protection**: User cannot execute privileged instructions
2. **Stack Protection**: Dual stack pointers prevent user from corrupting supervisor stack
3. **Memory Space Protection**: MMU prevents unauthorized memory access
4. **Device Protection**: Memory-mapped I/O can be protected from user access

**Memory Access Control**:
- **Supervisor-Only Memory**: OS data structures, device registers
- **User Memory**: Application code and data
- **Shared Memory**: Read-only data accessible to both modes
- **Device Memory**: Memory-mapped I/O regions with controlled access

**Example Memory Layout**:
```
Address Range      | Access Mode      | Purpose
-------------------|------------------|-------------------------
0x000000-0x00FFFF   | Supervisor only  | Exception vectors, OS kernel
0x010000-0x01FFFF   | Supervisor only  | OS data structures
0x020000-0x02FFFF   | Both modes       | Shared libraries
0x030000-0xFFFFFF   | User only        | Application memory
```

### Multitasking Support

The user/supervisor mechanism enables robust multitasking:

**Task Isolation**:
- Each task runs in user mode with its own memory space
- Tasks cannot interfere with each other's memory
- OS can preempt tasks via interrupts

**Context Switching**:
1. Interrupt occurs (e.g., timer interrupt)
2. Current task's state saved on its stack
3. OS interrupt handler executes
4. OS decides to switch to another task
5. New task's state restored from its stack
6. New task resumes execution

**Task Management**:
- OS maintains task control blocks in supervisor memory
- Each task has its own USP and memory space
- OS can create, terminate, and manage tasks
- Tasks communicate via OS-provided IPC mechanisms

### Security Implications

The MC68000's protection mechanisms provide several security benefits:

**Prevention of**:
- Unauthorized privilege escalation
- Memory corruption between tasks
- Direct hardware access by user programs
- System crashes due to user program errors

**Enforcement of**:
- Controlled access to system resources
- Proper error handling procedures
- Memory space isolation
- Privilege separation

**Limitations**:
- Protection relies on proper OS implementation
- MMU required for full memory protection
- User programs can still crash themselves
- No protection against supervisor mode bugs

### Debugging and Development

The exception mechanism provides powerful debugging capabilities:

**Trace Exception**:
- Enables single-step execution
- Useful for debugging complex issues
- Can be enabled/disabled programmatically

**Breakpoints**:
- Can be implemented via illegal instruction exception
- Replace instruction with illegal opcode
- Exception handler checks for breakpoint, then continues

**Error Reporting**:
- Exception handlers can provide detailed error information
- Stack frames contain context at time of error
- Can implement crash dumps and post-mortem analysis

**Development Tools**:
- Debuggers can use exception mechanism for breakpoints
- Profilers can use trace exception for instruction counting
- Memory checkers can use bus error exception for access validation

## Performance Optimizations

### High-Level vs Low-Level Emulation

Steem SSE uses a **high-level emulation** approach for the MC68000:

**Advantages**:
- Better performance than low-level emulation (LLE)
- Easier to debug and understand
- More traceable execution path
- Each instruction has its own function

**Disadvantages**:
- Less cycle-accurate than LLE
- More code to maintain
- Harder to achieve perfect timing

### Opcode Table Optimization

The use of a **big opcode table** (`m68k_call_table[0xFFFF+1]`) provides:
- Fast instruction dispatch (O(1) lookup)
- Easy to add new instructions
- Clear separation of instruction implementations
- Automatic illegal instruction handling

### Inline Functions

Frequently used functions are marked as `inline`:
```cpp
inline void m68k_new_pc(MEM_ADDRESS ad) {
    pc_high_byte = ad & 0xFF000000;
    pc = ad;
}
```

### Macro Usage

Extensive use of macros for common operations:
```cpp
#define NEW_PC(ad) m68k_new_pc(ad)
#define CLEAR_VC pswV = false; pswC = false
#define SR_CHECK_AND_B CLEAR_VC; SR_CHECK_Z_AND_N_B
```

## Debugging Support

### Debugger Integration

The CPU emulation integrates with Steem's debugger:

**Debug Registers**:
- Access to all CPU registers
- Program counter tracking
- Status register display
- Stack pointer monitoring

**Single-Step Execution**:
- Execute one instruction at a time
- Breakpoint support
- Trace execution

**Disassembly**:
- Full MC68000 disassembler (d2.cpp)
- Symbolic disassembly
- Uppercase/lowercase output

### Trace Logging

**Trace Macros**:
```cpp
#define LOGSECTION_CPU 16

TRACE("PC: %06X, IR: %04X\n", pc, IRD);
TRACE_LOG("CPU: Exception %d at PC %06X\n", exception_number, pc);
```

## CPU Configuration Options

### Compile-Time Options

```cpp
// In conditions.h
#define SSE_OPTION_FASTBLITTER  // Fast blitter operations
#define SSE_MEGASTE             // Mega STE support
#define SSE_VID_STVL1           // STVL video support
#define BIG_ENDIAN_PROCESSOR    // Big-endian host (experimental)
```

### Runtime Configuration

**CPU Speed Options**:
- Normal speed (8MHz for STF, 16MHz for STE)
- Boosted speed (faster execution)
- Custom clock speed

**CPU Type Options**:
- MC68000 (standard)
- MC68010 (extended features)
- Mega STE variations

## Instruction Timing

### Cycle Counting

Each instruction consumes a specific number of cycles:

**Instruction Timing Examples**:
- NOP: 4 cycles
- MOVE.B: 4-8 cycles (depending on addressing mode)
- ADD.B: 4-12 cycles
- MULU.W: 70 cycles
- DIVU.W: 140+ cycles

**Timing Factors**:
- Addressing mode complexity
- Memory access patterns
- Bus wait states
- E-clock synchronization

### Bus Cycle Types

1. **Prefetch Cycles**: Fetching next instruction
2. **Read Cycles**: Reading operands from memory
3. **Write Cycles**: Writing results to memory
4. **Idle Cycles**: Bus not in use
5. **Wait States**: Memory access delays

## CPU Emulation Accuracy

### Strengths

1. **Complete Instruction Set**: All MC68000 instructions implemented
2. **Accurate Flag Handling**: Correct condition code setting
3. **Proper Addressing Modes**: All 18 addressing modes supported
4. **Exception Handling**: Full exception processing
5. **Memory Access**: Accurate memory-mapped I/O
6. **Timing**: Reasonable cycle counting

### Limitations

1. **Not Cycle-Perfect**: High-level emulation means some timing approximations
2. **No Pipeline Emulation**: Real MC68000 has instruction pipeline
3. **Simplified E-Clock**: E-clock synchronization is approximated
4. **No Caching**: Real MC68000 has no cache, but emulation doesn't model memory hierarchy

### Accuracy Considerations

**Cycle-Accurate Features**:
- Instruction execution timing
- Memory access timing
- Bus cycle counting
- Interrupt handling timing

**Approximated Features**:
- Pipeline effects
- E-clock synchronization
- Wait state timing
- DMA interaction timing

## Exception and Supervisor Mode Summary

### Core Concepts

The MC68000's exception handling and user/supervisor mode mechanism provide a sophisticated foundation for operating system design and system robustness. The key components are:

1. **Two Processor States**: User mode (restricted) and Supervisor mode (privileged)
2. **Exception Mechanism**: Structured control transfer for error handling and system services
3. **Dual Stack Pointers**: Complete isolation between user and supervisor stacks
4. **Privileged Instructions**: Instructions that can only be executed in supervisor mode
5. **Memory Protection**: Foundation for secure memory access control

### How It All Works Together

**Normal Operation**:
1. System boots in supervisor mode
2. Operating system initializes hardware and data structures
3. OS switches to user mode and starts application programs
4. Applications run in user mode with restricted privileges
5. Applications request OS services via TRAP instructions or system calls

**Error Handling**:
1. Application encounters error (division by zero, illegal instruction, etc.)
2. Processor generates appropriate exception
3. Automatic switch to supervisor mode
4. Automatic switch to supervisor stack
5. OS exception handler examines error context
6. OS takes appropriate action (recover, terminate, log)
7. OS returns control to application or loads new program

**Interrupt Handling**:
1. External device asserts interrupt at specific priority level
2. Processor compares with current IPL in SR
3. If interrupt priority > current IPL:
   - Complete current instruction
   - Acknowledge interrupt
   - Generate interrupt exception
   - Switch to supervisor mode and stack
   - Execute interrupt handler
4. Handler services device and returns via RTE

**Security Enforcement**:
1. User program attempts privileged operation
2. Processor detects attempt
3. Generates Privilege Violation exception
4. Switches to supervisor mode
5. OS exception handler examines attempt
6. OS takes appropriate security action (terminate, log, etc.)

### Benefits of This Architecture

**For System Stability**:
- User program errors don't crash the entire system
- OS can recover from most user errors
- Critical system data protected from user access
- Hardware protected from unauthorized access

**For Security**:
- Privilege separation between OS and applications
- Memory isolation between different tasks
- Controlled access to system resources
- Protection against malicious or buggy code

**For Development**:
- Structured error handling mechanisms
- Debugging support via trace and breakpoint exceptions
- Memory protection for catching memory errors
- System call mechanism for controlled OS access

**For Performance**:
- Efficient exception processing
- Minimal overhead for normal operation
- Fast mode switching
- Optimized for common case (user mode execution)

### Real-World Applications

The MC68000's exception and protection mechanisms were used in various systems:

**Atari ST Series**:
- TOS (The Operating System) runs in supervisor mode
- Applications (GEM programs, games) run in user mode
- Memory protection provided by hardware (limited MMU in some models)
- Exception handling for robust system operation

**Other 68000-Based Systems**:
- Early Macintosh computers
- Unix workstations
- Embedded control systems
- Arcade game hardware

**Modern Systems**:
- Similar concepts used in x86 protected mode
- ARM processor exception handling
- Modern OS kernel/user space separation
- Virtual memory and memory protection systems

## Conclusion

Steem SSE's MC68000 CPU emulation provides a comprehensive and accurate implementation of the Motorola 68000 processor. The high-level emulation approach balances performance with accuracy, making it suitable for running Atari ST software while maintaining good execution speed.

The implementation covers the complete instruction set, all addressing modes, and proper exception handling with accurate user/supervisor mode transitions. The use of a large opcode table ensures fast instruction dispatch, while the modular design makes the code maintainable and extensible.

The exception and supervisor mode mechanisms are particularly well-implemented, providing the robust foundation needed for accurate emulation of the Atari ST's operating system and application software. These mechanisms enable the emulator to faithfully reproduce the behavior of the original hardware, including error conditions, interrupt handling, and system-level operations.

While not cycle-perfect at the microarchitectural level, the emulation is accurate enough to run virtually all Atari ST software correctly, including demos, games, and applications that rely on precise timing and hardware behavior.

The integration with Steem's debugger provides powerful tools for analyzing and debugging emulated software, making Steem SSE not just an emulator but also a development platform for Atari ST software.
