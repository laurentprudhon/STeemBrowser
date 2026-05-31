# Steem Emulator UI Implementation

## Overview

Steem SSE implements a comprehensive user interface that provides full control over the Atari ST emulation while maintaining the look and feel of a native application on each supported platform. The UI is designed to be both functional for end-users and powerful for developers, with extensive debugging capabilities.

## UI Architecture

### Platform-Specific Implementations

Steem SSE maintains separate UI implementations for different platforms:

| Platform | Implementation | Files |
|----------|---------------|-------|
| Windows | Win32 API | `stemwin.cpp`, `gui.cpp` |
| Linux/Unix | X11 | `code/x/x_*.cpp` |
| Common | Cross-platform | `gui.cpp`, `stemdialogs.cpp` |

### UI Component Hierarchy

```
Main Window (TStemWin)
├── Emulation Display Area
│   ├── Screen Rendering Surface
│   ├── Input Handling
│   └── OSD (On-Screen Display)
├── Menu Bar
│   ├── File Menu
│   ├── Emulation Menu
│   ├── Options Menu
│   ├── Debug Menu (debug builds only)
│   └── Help Menu
├── Toolbar (Optional)
├── Status Bar
└── Modal Dialogs
    ├── Configuration Dialogs
    ├── Debug Windows
    └── Information Dialogs
```

## Windows UI Implementation

### Main Window Class: TStemWin

**File**: `steem/stemwin.cpp`

**Responsibilities**:
- Main application window management
- Emulation display surface creation and management
- Input event handling (keyboard, mouse, joystick)
- Menu and command processing
- Window state management (minimize, maximize, fullscreen)

**Key Methods**:
```cpp
TStemWin::TStemWin()           // Constructor
TStemWin::Create()             // Window creation
TStemWin::WndProc()            // Window procedure
TStemWin::Paint()              // Screen painting
TStemWin::HandleKey()          // Keyboard input
TStemWin::HandleMouse()        // Mouse input
TStemWin::ToggleFullscreen()   // Fullscreen mode
```

### Window Creation and Management

**Window Styles**:
```cpp
// Main window styles
WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS

// Fullscreen styles
WS_POPUP | WS_SYSMENU

// Child window styles
WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS
```

**Window Classes**:
- `STEEM_CLASS_NAME` - Main emulator window class
- `STEEM_DEBUG_CLASS` - Debug window class
- Various custom control classes

### Display System

**Rendering Backends**:

1. **DirectDraw (SSE_DD)**:
   - Primary Windows rendering backend
   - Hardware-accelerated blitting
   - Fullscreen exclusive mode support
   - Direct memory access for performance

2. **Direct3D (SSE_D3D)**:
   - Alternative rendering backend
   - 3D acceleration support
   - Enhanced scaling and filtering

3. **GDI**:
   - Fallback rendering backend
   - Software-based rendering
   - Compatible with all Windows versions

**Display Surface Management**:
- Primary surface: Emulation display
- Back buffer: Double buffering for smooth updates
- OSD surface: On-screen display overlay
- Debug surfaces: Various debug display surfaces

### Input Handling

**Keyboard Input**:
- **File**: `steem/key_table.cpp`
- **Mapping**: PC keyboard to Atari ST keyboard
- **Features**:
  - Key remapping
  - Shortcut keys
  - Joystick emulation via keyboard
  - Macro support

**Mouse Input**:
- **DirectInput**: High-precision mouse input
- **Win32 Messages**: Standard mouse messages
- **Features**:
  - Mouse capture in window
  - Relative vs absolute mode
  - Joystick emulation via mouse

**Joystick Input**:
- **File**: `steem/stjoy.cpp`, `steem/stjoy_directinput.cpp`
- **DirectInput**: Modern joystick API
- **WinMM**: Legacy joystick API
- **Features**:
  - Multiple joystick support
  - Force feedback (if supported)
  - Customizable mappings
  - Deadzone and sensitivity adjustment

### Menu System

**Menu Structure**:
```
File Menu:
├── New
├── Open...
├── Recent Files
├── Save State
├── Load State
├── Screenshot
├── Record AVI
├── Print...
└── Exit

Emulation Menu:
├── Run
├── Pause
├── Reset (Cold/Warm)
├── Disk Management
├── Hard Disk Management
├── Memory Browser
└── Debugger

Options Menu:
├── Display...
├── Sound...
├── Input...
├── Joysticks...
├── Shortcuts...
├── Patches...
└── Preferences...

Debug Menu (Debug Builds):
├── Breakpoints...
├── Memory Browser
├── Trace
├── Frame Report
└── Debugger

Help Menu:
├── Contents
├── About...
└── Check for Updates
```

**Menu Implementation**:
- Created programmatically (not from resource file)
- Dynamic menu item enabling/disabling
- Context-sensitive menus
- Keyboard shortcuts for all menu items

## Dialog System

### Dialog Base Class: TStemDialog

**File**: `steem/stemdialogs.cpp`, `steem/stemdialogs.h`

**Features**:
- Modal and modeless dialog support
- Custom control creation and management
- Dialog positioning and sizing
- Message handling

**Key Methods**:
```cpp
TStemDialog::Create()          // Create dialog
TStemDialog::Show()            // Show dialog
TStemDialog::Hide()            // Hide dialog
TStemDialog::HandleMessage()   // Message handling
TStemDialog::EndDialog()       // Close dialog
```

### Standard Dialogs

**Configuration Dialogs**:

1. **Display Options** (`TOptionsDisplay`):
   - Resolution selection
   - Color depth
   - Scaling options
   - Fullscreen settings
   - Monitor type (RGB, Composite, etc.)

2. **Sound Options** (`TOptionsSound`):
   - Sound enabled/disabled
   - Volume control
   - Sample rate
   - Buffer size
   - Sound backend selection

3. **Input Options** (`TOptionsInput`):
   - Keyboard mapping
   - Mouse settings
   - Joystick configuration
   - Input device selection

4. **Joystick Configuration** (`TStickOptions`):
   - Joystick selection
   - Axis calibration
   - Button mapping
   - Deadzone adjustment

5. **Shortcuts Configuration** (`TShortcutBox`):
   - Shortcut key assignment
   - Macro definition
   - Shortcut categories

6. **Patches Configuration** (`TPatchesBox`):
   - ROM patches
   - TOS patches
   - Memory patches
   - Patch management

**Debug Dialogs**:

1. **Debugger** (`TDebugger`):
   - Disassembly view
   - Register display
   - Memory dump
   - Breakpoint management
   - Execution control

2. **Memory Browser** (`TMemBrowser`):
   - Hex dump view
   - ASCII view
   - Memory comparison
   - Breakpoint on access

3. **Trace Window** (`TDebuggerTrace`):
   - Execution trace
   - Cycle count display
   - Memory access log
   - I/O access log

4. **Frame Report** (`TDebugFrameReport`):
   - Frame-by-frame analysis
   - Cycle count per frame
   - Event timing
   - Performance statistics

5. **History List** (`THistoryList`):
   - Execution history
   - Memory access history
   - Register change history

**Information Dialogs**:

1. **Info Box** (`TInfoBox`):
   - Emulator information
   - System information
   - Version information
   - Statistics

2. **Disk Management** (`TDiskMan`):
   - Disk image selection
   - Disk drive configuration
   - Disk image creation
   - Disk image properties

3. **Hard Disk Management** (`THardDiskMan`):
   - Hard disk image selection
   - Partition configuration
   - Hard disk properties

4. **OSD (On-Screen Display)** (`TOSD`):
   - In-emulation messages
   - Performance display
   - Input display
   - Status information

## GUI Control System

### Control Base Class: TGUIControl

**File**: `steem/gui_controls.cpp`, `steem/headers/gui_controls.h`

**Control Types**:
```cpp
enum GUI_CONTROL_TYPE {
    GUI_STATIC,        // Static text
    GUI_BUTTON,        // Button
    GUI_CHECKBOX,      // Checkbox
    GUI_RADIOBUTTON,   // Radio button
    GUI_EDIT,          // Edit box
    GUI_LISTBOX,       // List box
    GUI_COMBOBOX,     // Combo box
    GUI_SLIDER,        // Slider
    GUI_GROUPBOX,      // Group box
    GUI_TAB,           // Tab control
    GUI_PANEL          // Panel/container
};
```

**Control Features**:
- Custom drawing (owner-draw)
- Theming support
- Keyboard navigation
- Mouse interaction
- Focus management

### Control Implementation

**Static Controls**:
- Text display
- Icon display
- Custom drawing

**Button Controls**:
- Standard buttons
- Toggle buttons
- Bitmap buttons
- Custom drawing

**Edit Controls**:
- Single-line text editing
- Multi-line text editing
- Numeric input
- Hexadecimal input

**List Controls**:
- Single selection
- Multiple selection
- Checkbox items
- Icon items
- Custom drawing

**Slider Controls**:
- Horizontal and vertical
- Range control
- Tick marks
- Custom drawing

## X11 UI Implementation (XSteem)

### X11 Window Management

**File**: `steem/code/x/x_stemwin.cpp`

**X11-Specific Features**:
- X11 window creation and management
- X11 event handling
- X11 visual and colormap management
- X11 font handling

**Key Classes**:
- `TXStemWin` - X11 main window
- `TXDisplay` - X11 display surface
- `TXGUI` - X11 GUI management

### X11 GUI Framework (HXC)

**HXC Library**: Custom X11 widget library for Steem

**HXC Files**:
- `include/x/hxc.cpp` - Core HXC implementation
- `include/x/hxc_button.cpp` - Button widget
- `include/x/hxc_edit.cpp` - Edit widget
- `include/x/hxc_listview.cpp` - List view widget
- `include/x/hxc_scrollbar.cpp` - Scrollbar widget
- `include/x/hxc_scrollarea.cpp` - Scroll area widget
- `include/x/hxc_dropdown.cpp` - Dropdown widget
- `include/x/hxc_fileselect.cpp` - File selection dialog
- `include/x/hxc_alert.cpp` - Alert dialog
- `include/x/hxc_popup.cpp` - Popup menu
- `include/x/hxc_popuphints.cpp` - Popup hints/tooltips
- `include/x/hxc_dir_lv.cpp` - Directory list view
- `include/x/hxc_prompt.cpp` - Prompt dialog

**HXC Widget Features**:
- Native X11 implementation
- Steem-specific theming
- Cross-platform look and feel
- Custom drawing support

### X11 Display System

**Rendering Backends**:
1. **X11 Direct**:
   - Direct X11 drawing
   - Software rendering
   - XImage-based surfaces

2. **XShm (Shared Memory)**:
   - MIT Shared Memory extension
   - Faster drawing performance
   - Direct memory access

3. **Xv (XVideo)**:
   - XVideo extension support
   - Hardware-accelerated scaling
   - YUV color space support

**Display Features**:
- Multiple visual support (TrueColor, PseudoColor)
- Colormap management
- Gamma correction
- Scaling and filtering

### X11 Input Handling

**Keyboard Input**:
- X11 key event handling
- Keysym to Atari ST key mapping
- Modifier key handling

**Mouse Input**:
- X11 pointer event handling
- Mouse capture
- Relative vs absolute mode

**Joystick Input**:
- Linux input subsystem
- Joystick device detection
- Axis and button mapping

## UI State Management

### Configuration System

**Configuration Storage**:
- **File**: `steem/configstorefile.cpp`
- **Format**: INI-style configuration files
- **Features**:
  - Persistent settings
  - User preferences
  - Machine-specific configurations
  - Profile support

**Configuration Files**:
- `steem.cfg` - Main configuration
- `steem_debug.cfg` - Debug configuration
- Various component-specific configs

### Window State Management

**Window Position and Size**:
- Persistent window geometry
- Maximized/minimized state
- Fullscreen state

**Display Settings**:
- Resolution
- Color depth
- Scaling mode
- Filter settings

### Emulation State Integration

**UI-Emulation Communication**:
- Emulation pause/resume
- Reset requests
- Configuration changes
- Debug commands

**State Synchronization**:
- UI reflects current emulation state
- Emulation state changes update UI
- Thread-safe communication

## Debug UI Components

### Debugger Interface

**File**: `steem/debugger.cpp`, `steem/QuickDebugger.cpp`

**Debugger Features**:
- **Disassembly View**:
  - Current instruction
  - Next instructions
  - Address and opcode display
  - Symbolic information

- **Register Display**:
  - All MC68000 registers
  - Status register
  - Program counter
  - Stack pointer

- **Memory Dump**:
  - Hex dump
  - ASCII dump
  - Memory access breakpoints
  - Memory comparison

- **Breakpoint Management**:
  - Execution breakpoints
  - Memory access breakpoints
  - I/O access breakpoints
  - Conditional breakpoints

- **Execution Control**:
  - Run/Pause
  - Single step
  - Step over
  - Step out
  - Run to cursor

### Memory Browser

**File**: `steem/mem_browser.cpp`, `steem/QuickGui.cpp`

**Features**:
- Memory address navigation
- Hex and ASCII views
- Breakpoint on access
- Memory comparison
- Search functionality
- Goto address

### Trace Window

**File**: `steem/debugger_trace.cpp`

**Features**:
- Execution trace logging
- Cycle count display
- Memory access logging
- I/O access logging
- Filtering options
- Export to file

### Frame Report

**File**: `steem/debug_framereport.cpp`

**Features**:
- Frame-by-frame analysis
- Cycle count per frame
- Event timing
- Performance statistics
- Visualization

## UI Theming and Appearance

### Color Schemes

**Default Color Scheme**:
- Background: Dark gray
- Text: Light gray/white
- Buttons: 3D-style
- Selection: Blue highlight

**Customizable Colors**:
- All UI elements can be themed
- Color schemes can be saved and loaded
- High-contrast mode support

### Fonts

**Font Usage**:
- System fonts for standard controls
- Custom bitmap fonts for OSD
- Monospace fonts for debug displays

**Font Files**:
- `rc/charset.blk` - OSD font
- System fonts for dialogs

### Icons and Bitmaps

**Icon Resources**:
- `steem.ico` - Application icon
- Various toolbar icons
- Status bar icons

**Bitmap Resources**:
- `flags.bmp` - Country flags
- `st_chars_mono.bmp` - ST character set
- `debug_icons.bmp` - Debug icons
- `STControlPanel.bmp` - Control panel

## UI Interaction Patterns

### Modal vs Modeless Dialogs

**Modal Dialogs**:
- Block emulation while open
- Used for critical configuration
- Used for file operations

**Modeless Dialogs**:
- Allow emulation to continue
- Used for debug windows
- Used for information displays

### Keyboard Focus

**Focus Management**:
- Emulation display can have focus (for keyboard input)
- Dialog controls can have focus
- Keyboard shortcuts work regardless of focus

**Focus Indicators**:
- Visual focus rectangle
- Highlighted buttons
- Caret in edit controls

### Mouse Interaction

**Mouse Capture**:
- Mouse can be captured in emulation display
- Relative mouse mode for joystick emulation
- Absolute mouse mode for mouse emulation

**Mouse Wheel**:
- Scrolling in list controls
- Zoom in/out in display
- Navigation in debug windows

### Drag and Drop

**Supported Operations**:
- Disk image files → Disk drives
- ROM files → ROM selection
- Configuration files → Configuration load

**Drag and Drop Implementation**:
- Windows: OLE drag and drop
- Linux: XDND (X Direct Network Drag and Drop)

## UI Performance Considerations

### Rendering Optimization

**Double Buffering**:
- Reduces flicker
- Smooth animation
- Better performance

**Dirty Rectangle Updates**:
- Only update changed screen areas
- Reduces rendering overhead
- Improves performance

**Hardware Acceleration**:
- DirectDraw/Direct3D on Windows
- XShm/Xv on Linux
- Assembly-optimized drawing routines

### Input Optimization

**Input Polling**:
- Efficient input device polling
- Minimal CPU usage
- Low latency

**Input Buffering**:
- Keyboard input buffering
- Mouse event buffering
- Joystick event buffering

### Memory Usage

**Surface Management**:
- Optimal surface sizes
- Memory-efficient formats
- Surface caching

**Resource Management**:
- Efficient bitmap handling
- Font caching
- Icon caching

## Platform-Specific UI Differences

### Windows-Specific Features

**DirectX Integration**:
- DirectDraw for 2D acceleration
- Direct3D for 3D acceleration
- DirectInput for input devices
- DirectSound for audio

**Windows API Usage**:
- Win32 API for window management
- GDI for fallback rendering
- Common Controls for standard dialogs
- Shell API for file operations

**Windows-Specific Dialogs**:
- Native file dialogs
- Native color dialogs
- Native font dialogs
- System dialogs (About, etc.)

### Linux-Specific Features

**X11 Integration**:
- X11 for window management
- Xext for extensions
- XShm for shared memory
- Xv for video

**GTK/Qt Independence**:
- Custom X11 widgets (HXC)
- No GTK/Qt dependencies
- Lightweight implementation

**Linux-Specific Dialogs**:
- Custom file dialogs
- Custom color dialogs
- Native system dialogs where available

### Cross-Platform Consistency

**Unified API**:
- Abstracted platform-specific code
- Common interface for all platforms
- Consistent behavior across platforms

**Platform Abstraction**:
- `notwindows.h` - Non-Windows platform definitions
- Platform-specific includes
- Conditional compilation

## UI Development Tools

### Debugging UI Issues

**UI Debug Builds**:
- Additional UI debugging information
- Visual debugging aids
- UI performance metrics

**UI Testing**:
- Manual testing of all dialogs
- Keyboard navigation testing
- Mouse interaction testing
- Resizing and layout testing

### UI Customization

**User Customization**:
- Configurable keyboard shortcuts
- Customizable toolbars
- Adjustable colors and fonts
- Window layout preferences

**Developer Customization**:
- Easy addition of new dialogs
- Flexible control system
- Extensible architecture

## UI Future Enhancements

### Planned UI Improvements

1. **Modern UI Framework**:
   - Qt-based UI (optional)
   - Better theming support
   - High-DPI support

2. **Improved Debug UI**:
   - Dockable debug windows
   - Better visualization
   - Enhanced navigation

3. **Touch Support**:
   - Touch-friendly controls
   - Gesture support
   - Tablet optimization

4. **Accessibility**:
   - Screen reader support
   - High-contrast themes
   - Keyboard-only navigation

### Proposed UI Features

1. **Tabbed Interface**:
   - Multiple emulation sessions
   - Tabbed debug windows
   - Workspace management

2. **Customizable Layout**:
   - Dockable panels
   - Floating windows
   - Saved layouts

3. **Enhanced Visualization**:
   - Memory map visualization
   - Execution flow visualization
   - Performance graphs

4. **Improved Configuration**:
   - Profile management
   - Quick configuration presets
   - Configuration inheritance

## Conclusion

Steem SSE's UI implementation provides a comprehensive and flexible interface for controlling the Atari ST emulation. The UI is carefully designed to support both end-user functionality and extensive debugging capabilities. With platform-specific implementations for Windows and Linux, Steem maintains a consistent user experience across all supported platforms while leveraging each platform's native capabilities for optimal performance.

The UI architecture separates concerns between emulation core and user interface, allowing for independent development and testing. The extensive debug UI components provide powerful tools for validating emulator accuracy and diagnosing issues, making Steem SSE not just an emulator but also a development platform for Atari ST software.
