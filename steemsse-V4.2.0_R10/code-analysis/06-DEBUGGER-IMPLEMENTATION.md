# Steem Emulator Debugger Implementation

## Overview

Steem SSE features a comprehensive debugger system designed for cycle-accurate analysis and debugging of Atari ST software. The debugger is a core component of the debug build (`DEBUG_BUILD` flag) and provides extensive instrumentation for validating emulator accuracy and diagnosing issues in emulated software.

**Key Characteristics**:
- Full-featured MC68000 debugger
- Cycle-accurate execution tracing
- Memory and I/O access monitoring
- Breakpoint system with multiple trigger types
- Disassembly and register inspection
- Integration with emulator core for seamless debugging

**Platform Support**: Primarily Windows (Win32 API), with limited Linux support via Wine

## Debugger Architecture

### Core Components

```
Debugger System
├── TDebug (debug.cpp)              - Global debug facilities
├── TDebugger (debugger.cpp)        - Main debugger GUI and logic
├── TDebugEmu (debug_emu.cpp)       - Emulation-level debugging
├── TDebuggerTrace (debugger_trace.cpp) - Execution tracing
├── TMrStatic (mr_static.cpp)       - Debug display controls
├── THistoryList (historylist.cpp)  - Execution history tracking
├── TMemBrowser (mem_browser.cpp)   - Memory browser interface
└── TD2 (d2.cpp)                   - MC68000 disassembler
```

### Debug Build Configuration

**Compilation Flags**:
```cpp
#define DEBUG_BUILD      // Enables debugger functionality
#define SSE_DEBUG        // Additional debug features
#define DEBUGGER_INCLUDED // Full debugger support
```

**Conditional Compilation**:
- Debugger is only compiled when `DEBUG_BUILD` is defined
- Separate from Visual Studio debug builds (`_DEBUG` flag)
- Not available for Linux builds (reported to work in Wine)

## Debugger Core System

### TDebug Class (debug.cpp)

**Purpose**: Global debugging facilities used throughout the emulator

**Key Functionality**:
- Trace logging system
- Debug section management
- OSD (On-Screen Display) messaging
- Crash handling
- Performance monitoring

**Trace System**:
```cpp
// Trace levels and sections
#define LOGSECTION_ALWAYS        0   // Always logged
#define LOGSECTION_FDC          1   // Floppy Disk Controller
#define LOGSECTION_IO           2   // I/O operations
#define LOGSECTION_MFP_TIMERS   3   // MFP timer operations
#define LOGSECTION_INIT         4   // Initialization
#define LOGSECTION_CRASH        5   // Crash/Error conditions
#define LOGSECTION_HARDDRIVE    6   // Hard drive operations
#define LOGSECTION_IKBD         7   // Keyboard/Mouse controller
#define LOGSECTION_AGENDA       8   // Event scheduling
#define LOGSECTION_INTERRUPTS   9   // Interrupt handling
#define LOGSECTION_TRAP         10  // Trap instructions
#define LOGSECTION_SOUND        11  // Sound system
#define LOGSECTION_GLUE         12  // Glue chip
#define LOGSECTION_BLITTER      13  // Blitter operations
#define LOGSECTION_PORTS        14  // I/O ports
#define LOGSECTION_TRACE        15  // Execution trace
#define LOGSECTION_CPU          16  // CPU operations
#define LOGSECTION_VIDEO_RENDERING 17 // Video rendering
```

**Trace Macros**:
```cpp
TRACE(...)          // General trace output
TRACE_LOG(...)      // Log to trace file
TRACE_OSD(...)      // Display on OSD
TRACE_VID(...)      // Video-related trace
```

**Trace File Management**:
- Configurable trace file path
- Section-based filtering
- Automatic file rotation
- Locking mechanism for thread safety

### Debug State Management

**Run States**:
```cpp
enum TRunState {
    RUNSTATE_STOPPED,    // Emulation stopped
    RUNSTATE_RUNNING,    // Emulation running
    RUNSTATE_STOPPING,   // Stopping (breakpoint hit)
    RUNSTATE_PAUSED      // Paused by user
};
```

**Debug Modes**:
```cpp
// Breakpoint modes
enum TBreakpointMode {
    BREAKPOINT_MODE_STOP,  // Stop execution on breakpoint
    BREAKPOINT_MODE_LOG,   // Log but don't stop
    BREAKPOINT_MODE_OFF    // Breakpoints disabled
};

// Monitor modes
enum TMonitorMode {
    MONITOR_MODE_STOP,    // Stop on monitored access
    MONITOR_MODE_LOG,     // Log monitored access
    MONITOR_MODE_OFF      // Monitoring disabled
};
```

## Breakpoint System

### Breakpoint Types

**Address Breakpoints** (`TDebugAddress` structure):
```cpp
struct TDebugAddress {
    MEM_ADDRESS ad;       // Memory address
    int mode;             // 0=off, 1=global, 2=break, 3=log
    int bwr;             // &1=break, &2=write, &4=read
    WORD mask[2];        // Write mask, read mask
    char name[64];       // Optional name/label
};
```

**Breakpoint Trigger Types**:
1. **Execution Breakpoints**: Trigger when PC reaches specific address
2. **Memory Write Breakpoints**: Trigger on write to specific address
3. **Memory Read Breakpoints**: Trigger on read from specific address
4. **I/O Access Breakpoints**: Trigger on I/O register access
5. **Interrupt Breakpoints**: Trigger on specific interrupt

### Breakpoint Management

**Core Functions** (debugger.cpp):
```cpp
TDebugAddress* debug_find_address(MEM_ADDRESS ad)
TDebugAddress* debug_find_or_add_address(MEM_ADDRESS ad)
void debug_remove_address(MEM_ADDRESS ad)
void debug_set_bk(MEM_ADDRESS ad, bool set)
void debug_set_mon(MEM_ADDRESS ad, bool read, WORD mask)
void debug_set_name(MEM_ADDRESS ad, EasyStr name)
void debug_update_bkmon()
void breakpoint_menu_setup()
```

**Breakpoint Configuration**:
- Address specification (hex format)
- Access type (read, write, execute)
- Data mask (for memory access breakpoints)
- Action mode (stop, log, ignore)
- Optional name/label

**Interrupt Breakpoints**:
```cpp
const char *name_of_interrupt[NUM_BREAK_IRQS] = {
    "Centronics","RS232 DCD","RS232 CTS","Blitter",
    "Timer D","Timer C","ACIAs","FDC","Timer B","RS232 TX Error",
    "RS232 RX Buf Empty","RS232 RX Error","RS232 RX Buf Full",
    "Timer A","RS232 Ring Detector","Mono Monitor",
    "Spurious","HSYNC","VSYNC","Line-A","Line-F","Trap"
};

bool break_on_irq[NUM_BREAK_IRQS];  // Break on specific interrupt
```

### Breakpoint Menu System

**Menu Structure**:
- Breakpoint list with addresses and modes
- Interrupt breakpoint submenu
- Monitor point management
- Clear all breakpoints
- Set breakpoint at current PC

**Menu Updates**:
- Dynamic menu population based on active breakpoints
- Visual indication of breakpoint state
- Context menu for breakpoint management

## Execution Tracing

### Trace Window (debugger_trace.cpp)

**Trace Display**:
- Instruction-by-instruction execution trace
- Register state before/after each instruction
- Memory access logging
- Cycle count tracking

**Trace Entry Structure**:
```cpp
struct Ttrace_display_entry {
    MEM_ADDRESS address;       // Instruction address
    char disassembly[80];     // Disassembled instruction
    WORD sr_before;           // Status register before
    WORD sr_after;            // Status register after
    int cycles;               // Cycles taken
    // ... additional fields
};
```

**Trace Features**:
- Single-step execution
- Trace over (skip function calls)
- Trace into (follow function calls)
- Run to cursor
- Conditional tracing

### Trace Execution Flow

**Trace Process** (debugger_trace.cpp:trace()):
```cpp
void trace() {
    BUS_SAVE;  // Save current bus state
    
    // Setup trace window
    SendMessage(trace_window_handle, WM_SETTEXT, 0, (LPARAM)"Trace");
    TRACE("Trace %06X\n", pc);
    
    // Initialize trace
    trace_init();
    d2_trace = true;
    disa_d2(pc & 0x00FFFFFF);  // Disassemble current instruction
    
    // Record state
    int time_text_entry = trace_entries;
    trace_add_text(EasyStr("Instruction time: "));
    d2_trace = false;
    
    // Execute one instruction
    runstate = RUNSTATE_STOPPED;
    debug_in_trace = true;
    
    COUNTER_VAR old_sys_time = ABSOLUTE_SYS_TIME;
    
    TRY_M68K_EXCEPTION
    {
        // Record history
        pc_history_y[pc_history_idx] = scan_y;
        pc_history_c[pc_history_idx] = LINECYCLES;
        pc_history[pc_history_idx++] = pc & 0x00FFFFFF;
        if(pc_history_idx >= HISTORY_SIZE) pc_history_idx = 0;
        
        stem_runmode = STEM_MODE_CPU;
        
        #if !defined(SSE_DEBUGGER_NODRAW)
        draw_begin();
        debug_update_drawing_position();
        #endif
        
        m68kProcess();  // Execute one instruction
        
        sys_cycles_this_instruction = (int)(ABSOLUTE_SYS_TIME - old_sys_time);
        debug_check_for_events();
        
        #if !defined(SSE_DEBUGGER_NODRAW)
        draw_end();
        #endif
        
        stem_runmode = STEM_MODE_INSPECT;
        trace_get_after();  // Get register state after
        
        #if !defined(SSE_DEBUGGER_NODRAW)
        update_display_after_trace();
        #endif
    }
    CATCH_M68K_EXCEPTION
    {
        // Handle CPU exceptions
        TMC68kException &e = ExceptionObject;
        stem_runmode = STEM_MODE_INSPECT;
        
        if(e.bombs > 7) {
            e.crash();
            sys_cycles_this_instruction = (int)(ABSOLUTE_SYS_TIME - old_sys_time);
            trace_exception_display(&e);
        }
        // ... exception handling
    }
    
    debug_in_trace = false;
}
```

### History Tracking

**Execution History** (historylist.cpp):
- PC history with cycle counts
- Register change history
- Memory access history
- Event timing history

**History Buffer**:
```cpp
#define HISTORY_SIZE 1024

MEM_ADDRESS pc_history[HISTORY_SIZE];
SHORT pc_history_y[HISTORY_SIZE];      // Scanline at execution
SHORT pc_history_c[HISTORY_SIZE];      // Cycle count at execution
int pc_history_idx = 0;
```

## Memory Browser

### Memory Browser Implementation (mem_browser.cpp)

**Features**:
- Hex dump view
- ASCII view
- Memory comparison
- Breakpoint on access
- Search functionality
- Goto address

**Memory Browser Classes**:
```cpp
class mem_browser {
public:
    MEM_ADDRESS ad;           // Current address
    int bytes;                // Bytes per line
    bool editflag;            // Allow editing
    EMrStaticType type;       // Display type
    
    // Display and navigation
    void display();
    void scroll(int lines);
    void goto_address(MEM_ADDRESS new_ad);
    void search();
    
    // Data access
    BYTE read_byte(MEM_ADDRESS addr);
    WORD read_word(MEM_ADDRESS addr);
    DWORD read_long(MEM_ADDRESS addr);
    
    // Update
    void update();
};
```

**Specialized Browsers**:
- `m_b_mem_disa`: Disassembly memory browser
- `m_b_stack`: Stack memory browser
- `m_b_trace`: Trace memory browser

### Memory Access Monitoring

**Monitor Types**:
1. **Memory Read Monitoring**: Log/stop on read from specific address
2. **Memory Write Monitoring**: Log/stop on write to specific address
3. **I/O Monitoring**: Log/stop on I/O register access

**Monitor Configuration**:
```cpp
MEM_ADDRESS debug_mon_read_ad[MAX_BREAKPOINTS];    // Read monitor addresses
MEM_ADDRESS debug_mon_write_ad[MAX_BREAKPOINTS];   // Write monitor addresses
WORD debug_mon_read_mask[MAX_BREAKPOINTS];        // Read masks
WORD debug_mon_write_mask[MAX_BREAKPOINTS];       // Write masks

int debug_num_mon_reads = 0;    // Number of active read monitors
int debug_num_mon_writes = 0;   // Number of active write monitors
```

## Disassembler (TD2)

### MC68000 Disassembler (d2.cpp)

**Purpose**: Full MC68000 instruction disassembly

**Features**:
- Complete MC68000 instruction set support
- All addressing modes
- Symbolic disassembly
- Uppercase/lowercase output
- Monospace formatting

**Disassembly Function**:
```cpp
char* disa_d2(MEM_ADDRESS ad);
// Disassembles instruction at address 'ad'
// Returns pointer to disassembly string
```

**Disassembler Initialization**:
```cpp
void d2_routines_init();
// Initializes disassembler jump tables
// Must be called before using disassembler
```

**Jump Tables**:
- `d2_high_nibble_jump_table[16]`: First-level opcode dispatch
- `d2_jump_line_0[64]` through `d2_jump_line_e[64]`: Opcode-specific handlers
- Specialized tables for different operand types

**Disassembly Output Control**:
```cpp
bool debug_monospace_disa = false;    // Use monospace font
bool debug_uppercase_disa = false;    // Use uppercase for opcodes

void debug_change_upper();            // Toggle uppercase/lowercase
void set_up_reg_browser();            // Setup register browser entries
```

## Register Display

### Register Browser

**Register Entries**:
```cpp
char reg_browser_entry_name[32][20];     // Register names
DWORD* reg_browser_entry_pointer[32];    // Pointers to register values

// Register list:
// 0: PC
// 1-15: D0-D7, A0-A7
// 16: SP (A7)
// 17: (terminator)
```

**Register Display Update**:
```cpp
void update_register_display(bool reset_pc_display);
// Updates all register displays
// If reset_pc_display is true, resets PC display to current PC
```

**Status Register Display**:
- Binary display of SR bits
- Visual indication of set bits
- Tooltip information for each bit

## Debugger UI

### Main Debugger Window

**Window Layout**:
```
Debugger Window
├── Menu Bar
│   ├── File
│   ├── View
│   ├── Breakpoints
│   ├── Monitor
│   ├── Options
│   └── Help
├── Toolbar
├── Disassembly View (left pane)
├── Register Display (right pane)
├── Memory Dump (bottom pane)
├── Status Bar
└── Trace Window (separate)
```

**Window Creation**:
```cpp
HWND DWin = NULL;  // Main debugger window handle

void DWin_init();  // Initialize debugger window
void DebuggerToggle(BOOL visible);  // Show/hide debugger
```

### Debugger Controls

**Display Controls**:
- `sr_display`: Status register display
- `DWin_edit`: Disassembly edit control
- `DWin_trace_button`: Trace button
- `DWin_run_button`: Run/Stop button
- `DWin_trace_over_button`: Trace over button

**Scroll Controls**:
- `DWin_timings_scroller`: Timing information scroller
- `trace_scroller`: Trace window scroller

**Status Bar**:
- `hDbgStatusBar`: Debugger status bar
- `DbgStatusBarMsg()`: Update status bar message

### Debugger Menu System

**Menu Handles**:
```cpp
HMENU debugger_menu;        // Main debugger menu
HMENU breakpoint_menu;      // Breakpoint submenu
HMENU monitor_menu;         // Monitor submenu
HMENU breakpoint_irq_menu;  // Interrupt breakpoint submenu
HMENU insp_menu;            // Inspect menu
HMENU mem_browser_menu;     // Memory browser menu
HMENU history_menu;         // History menu
```

**Menu Setup Functions**:
```cpp
void breakpoint_menu_setup();  // Setup breakpoint menu
void insp_menu_setup();        // Setup inspect menu
```

## Debugger Workflow

### Starting Debug Session

1. **Launch Debug Build**:
   - Compile with `DEBUG_BUILD` flag
   - Debugger automatically initializes

2. **Load Program**:
   - Load ROM, disk image, or snapshot
   - Emulator starts in stopped state

3. **Set Breakpoints**:
   - Set execution breakpoints at key addresses
   - Configure interrupt breakpoints
   - Set memory access monitors

4. **Start Execution**:
   - Click Run button or press F5
   - Emulation runs until breakpoint hit

### Debugging Commands

**Execution Control**:
- **Run (F5)**: Continue execution until breakpoint
- **Stop (Shift+F5)**: Stop execution immediately
- **Step Into (F11)**: Execute one instruction
- **Step Over (F10)**: Execute one instruction, skip function calls
- **Trace (F7)**: Trace execution with detailed logging

**Breakpoint Management**:
- **Set Breakpoint (F9)**: Set breakpoint at current PC
- **Clear Breakpoint**: Remove specific breakpoint
- **Clear All**: Remove all breakpoints
- **Toggle Breakpoint**: Enable/disable breakpoint

**Inspection**:
- **Memory Browser**: View memory at any address
- **Register Display**: View all CPU registers
- **Disassembly**: View disassembled code
- **History**: View execution history

### Exception Handling

**Exception Processing** (debugger.cpp:debug_trace_crash):
```cpp
void debug_trace_crash(TMC68kException &e) {
    SendMessage(trace_window_handle, WM_SETTEXT, 0, (LPARAM)"Exception");
    trace_init();
    trace_pc = e.ucrash_address.d32;
    trace_sr_before = e.crash_sr;
    update_register_display(true);
    trace_exception_display(&e);
    trace_display();
}
```

**Exception Types**:
```cpp
const char* bombs_name[12] = {
    "SSP after reset","PC after reset","bus error",
    "address error","illegal instruction","division by zero","CHK instruction",
    "TRAPV instruction","Privilege violation","Trace","Line-A","Line-F"
};
```

## Advanced Debugging Features

### Conditional Breakpoints

**Conditional Logic**:
- Break on specific data values
- Break on register values
- Break on memory patterns
- Break on cycle counts

**Condition Evaluation**:
- Simple boolean conditions
- Register comparisons
- Memory value checks
- Combined conditions with AND/OR

### Data Visualization

**Memory Visualization**:
- Hex dump with ASCII side-by-side
- Word/longword display
- Signed/unsigned interpretation
- Floating-point display

**Register Visualization**:
- Binary display for status register
- Hexadecimal for data registers
- Address display for address registers
- Color-coded changes

### Timing Analysis

**Cycle Counting**:
- Instruction cycle counts
- Frame cycle counts
- Event timing
- Performance statistics

**Timing Display**:
- Cycles per instruction
- Cycles per frame
- Timing histograms
- Performance graphs

### Plugin System

**Debug Plugin Support**:
```cpp
struct TDebugPluginfo {
    char name[64];
    void (*read_mem)(MEM_ADDRESS, BYTE*, int);
    void (*write_mem)(MEM_ADDRESS, BYTE*, int);
    // ... other plugin functions
};

DynamicArray<TDebugPluginfo> debug_plugins;
void debug_plugin_load();
void debug_plugin_free();
```

## Debugger Integration with Emulator

### Emulator State Access

**CPU State**:
- Direct access to MC68000 registers
- Program counter (PC)
- Status register (SR)
- Data and address registers (D0-D7, A0-A7)

**Memory Access**:
- Read/write memory through emulator functions
- Memory-mapped I/O access
- ROM/RAM bank switching awareness

**Hardware State**:
- Glue chip registers
- MMU configuration
- DMA status
- Shifter state
- PSG registers
- FDC status

### Event System Integration

**Debug Event Checking**:
```cpp
void debug_check_for_events();
// Checks for debug-related events
// Called during emulation execution
```

**Event Types**:
- Breakpoint hits
- Watchpoint triggers
- Interrupt occurrences
- Exception conditions
- Cycle count thresholds

### Debug Build Differences

**Debug-Only Features**:
- Full debugger UI
- Breakpoint system
- Trace execution
- Memory monitoring
- Register inspection

**Release Build Features**:
- Basic trace logging (if enabled)
- OSD messages
- Limited debugging output

## Debugger Limitations

### Platform Limitations

1. **Windows Only**: Debugger is primarily designed for Windows
2. **Linux Support**: Limited support via Wine
3. **No Native Linux Debugger**: Porting to Linux would be a significant effort

### Performance Considerations

1. **Execution Overhead**: Debug builds are significantly slower
2. **Memory Usage**: Debugger maintains extensive state information
3. **Breakpoint Overhead**: Memory access breakpoints add significant overhead
4. **Trace Overhead**: Instruction tracing is very slow

### Feature Limitations

1. **No Reverse Execution**: Cannot step backward through execution
2. **Limited Multi-Threading**: Debugger assumes single-threaded execution
3. **No Hardware Breakpoints**: All breakpoints are software-based
4. **Limited Symbol Support**: Symbolic debugging requires manual symbol loading

## Debugger Configuration

### Build Configuration

**Debug Build Flags**:
```makefile
# MinGW debug build
STEEMFLAGS += -DDEBUG_BUILD -DSSE_DEBUG

# Visual Studio debug configuration
# Use Debug configuration with DEBUG_BUILD defined
```

**Optional Debug Features**:
```cpp
#define SSE_DEBUGGER              // Full debugger support
#define SSE_DEBUGGER_TOGGLE      // Debugger show/hide toggle
#define SSE_DEBUGGER_STATUS_BAR   // Debugger status bar
#define SSE_DEBUGGER_FRAME_REPORT // Frame report in debugger
#define SSE_DBG_NOSIMULTRACE      // Disable simultaneous trace
#define SSE_DEBUGGER_NODRAW      // Disable drawing in debugger
#define SSE_DEBUGGER_FAKE_IO     // Fake I/O for debugging
```

### Runtime Configuration

**Debug Options**:
- Enable/disable debugger on startup
- Configure trace logging
- Set default breakpoint actions
- Configure debug display options

## Debugger Usage Examples

### Example 1: Setting a Breakpoint

1. Run emulator in debug mode
2. Load program to debug
3. Navigate to desired address in disassembly view
4. Press F9 to set breakpoint
5. Press F5 to run
6. Execution stops at breakpoint

### Example 2: Tracing Execution

1. Set breakpoint at function entry point
2. Run until breakpoint hits
3. Press F7 to start tracing
4. Each instruction executes with full state display
5. Use F11 to step through instructions

### Example 3: Memory Monitoring

1. Set monitor on specific memory address
2. Configure to stop on write
3. Run emulation
4. Execution stops when monitored address is written
5. Inspect memory and registers to see what changed

### Example 4: Exception Debugging

1. Run program that crashes
2. Debugger automatically catches exception
3. Exception information displayed
4. Register state at crash point shown
5. Stack trace available (if configured)

## Future Debugger Enhancements

### Planned Improvements

1. **Dockable Debug Windows**: Modern IDE-style docking
2. **Better Visualization**: Graphical display of memory and registers
3. **Symbolic Debugging**: Improved symbol table support
4. **Scripting Support**: Automated debugging scripts
5. **Reverse Execution**: Step backward through execution

### Proposed Features

1. **Breakpoint Conditions**: Complex conditional breakpoints
2. **Watch Expressions**: Monitor arbitrary expressions
3. **Call Stack**: Full call stack display
4. **Memory Map**: Visual memory map display
5. **Performance Profiling**: Execution profiling tools

## Conclusion

Steem SSE's debugger implementation provides a comprehensive and powerful debugging environment for Atari ST software development and emulator validation. The debugger is deeply integrated with the emulator core, providing cycle-accurate execution control and extensive instrumentation capabilities.

While primarily designed for Windows, the debugger's architecture allows for potential porting to other platforms. The extensive breakpoint system, execution tracing, and memory monitoring capabilities make it an invaluable tool for both emulator developers and Atari ST software developers.

The debugger's integration with the emulator's event system and hardware component emulation ensures that it can provide accurate debugging information at any level of the emulation, from high-level software behavior to low-level hardware interactions.
