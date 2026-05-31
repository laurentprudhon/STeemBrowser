# Steem Emulator Build System

## Overview

Steem SSE employs a multi-platform build system supporting Windows (Visual Studio, MinGW, Borland C++), Linux/Unix (X11), and potentially other platforms. The build system is designed to handle the complex dependencies and platform-specific requirements of a cycle-accurate Atari ST emulator.

## Build System Architecture

### Platform Support

| Platform | Build System | Compiler | Status |
|----------|-------------|----------|--------|
| Windows | Visual Studio | MSVC | Fully Supported |
| Windows | MinGW | GCC | Fully Supported |
| Windows | Borland C++ | BCC | Legacy Support |
| Windows | Cygwin | GCC | Supported |
| Linux/Unix | Makefile | GCC | Fully Supported |

### Build Configuration Matrix

**Conditional Compilation Flags**:

```cpp
// Platform defines
WIN32          // Windows platform
UNIX           // Unix-like platform
LINUX          // Linux-specific features
CYGWIN         // Cygwin environment
BIG_ENDIAN_PROCESSOR  // Big-endian host (experimental)

// Compiler defines
BCB_BUILD      // Borland C++ Builder
BCC_BUILD      // Borland C++ command line
MINGW_BUILD    // MinGW compiler
VC_BUILD       // Microsoft Visual C++

// Build type defines
DEBUG_BUILD    // Debug build with full instrumentation
NO_DEBUG_BUILD // Release build without debug features
RELEASE_BUILD  // Release build optimizations

// Feature defines
SSE_DD         // DirectDraw support
SSE_D3D        // Direct3D support
SSE_VID_RECORD_AVI  // AVI video recording
SSE_MAIN_LOOP2 // Alternative main loop implementation
SSE_ARCHIVEACCESS_SUPPORT // Archive access support
SSE_WRITEDIR   // Directory writing support
NO_486_ASM     // Disable x86 assembly optimizations
NO_RARLIB      // Disable RAR library support
NO_SHM         // Disable MIT shared memory extension (Unix)
```

## Windows Build Systems

### 1. Visual Studio Build

**Project Files**:
- `windows-build/VS2019/SteemVS2019Quick.vcxproj` - VS2019 project
- `windows-build/VS2015/SteemVS2015Quick.vcxproj` - VS2015 project
- `windows-build/VS2008/SteemVS2008Quick.vcproj` - VS2008 project
- `windows-build/visual-studio/SteemVC6.dsp` - VC6 project

**Project Structure**:
```
SteemVS2019Quick/
├── SteemVS2019Quick.vcxproj      # Main project file
├── SteemVS2019Quick.vcxproj.filters  # File organization
├── SteemVS2019Quick.vcxproj.user    # User-specific settings
└── SteemVS2019Quick.sln            # Solution file
```

**Build Configurations**:
- **Debug**: Full debug symbols, no optimizations, all debug features enabled
- **Release**: Optimized build, debug features disabled, full speed
- **ReleaseDyn**: Release build with dynamic linking

**Required Libraries (Windows)**:
```
winmm.lib      # Windows Multimedia
uuid.lib       # UUID generation
comctl32.lib   # Common Controls
ole32.lib      # OLE32
import32.lib   # Borland-specific (legacy)
cw32mt.lib     # Borland-specific (legacy)
```

**Visual Studio Build Process**:
1. Open solution file in Visual Studio
2. Select build configuration (Debug/Release)
3. Build solution (F7)
4. Output: `Steem.exe` in build directory

### 2. MinGW Build System

**Build Files**:
- `windows-build/mingw/SSE/makefile_MinGW.txt` - Main MinGW makefile
- `windows-build/mingw/SSE/build_mingw_*.bat` - Batch build scripts

**Build Scripts**:
```batch
build_mingw_set.bat      # Environment setup
build_mingw_user.bat     # User build (release)
build_mingw_debug.bat    # Debug build
build_mingw_user_D3D.bat # Direct3D build
build_mingw_debug_D3D.bat# Debug with Direct3D
build_mingw_asm.bat      # Assembly-only build
```

**MinGW Makefile Structure**:

```makefile
# Compiler configuration
CCP := mingw32-g++
CC := mingw32-gcc
WINDRES := windres

# Platform flags
STEEMFLAGS := -DMINGW_BUILD -DWIN32 -DSTEVEN_SEAGAL -DSSE_DD

# Compiler flags
CFLAGS := -Wall -I$(INCDIR) -I$(3RDPARTYROOT) -I$(STEEMROOT) -I$(STEEMROOT)/headers
CPPFLAGS := $(CFLAGS) -fdiagnostics-show-option -Wno-write-strings -fpermissive

# Debug flags
ifeq ($(DEBUG),1)
    STEEMFLAGS += -DDEBUG_BUILD -DSSE_DEBUG
endif

ifeq ($(GDB),1)
    STEEMFLAGS += -ggdb
endif
```

**MinGW Build Process**:
```bash
# Set up environment
call build_mingw_set.bat

# Build user (release) version
mingw32-make.exe -f makefile_MinGW.txt USER=1

# Build debug version
mingw32-make.exe -f makefile_MinGW.txt DEBUG=1

# Build with debug symbols for GDB
mingw32-make.exe -f makefile_MinGW.txt DEBUG=1 GDB=1
```

**Output Files**:
- `Steem.exe` - Main emulator executable
- Object files in `windows-build/mingw/SSE/obj/`

### 3. Borland C++ Build System

**Project Files**:
- `windows-build/bcc/` - Borland C++ build files
- `makefil3.txt`, `makefil3q.txt` - Makefile templates

**Build Scripts**:
```batch
build_bcc_set.bat      # Environment setup
build_user_D3D.bat     # User build with Direct3D
build_debug_D3D.bat    # Debug build with Direct3D
build_user_DD.bat      # User build with DirectDraw
build_debug_DD.bat     # Debug build with DirectDraw
build_bcc_asm.bat      # Assembly build
```

**Borland-Specific Features**:
- Uses `import32.lib` for Windows API access
- Uses `cw32mt.lib` for C runtime
- Assembly object files with Borland-compatible format

## Linux/Unix Build System (XSteem)

### X11 Build System

**Build Files**:
- `X-build/Makefile` - Main makefile
- `X-build/Makefile.txt` - Alternative makefile with detailed configuration
- `X-build/xsteem.mk` - Makefile include
- `X-build/xsteem64.mk` - 64-bit build configuration

**Build Configuration**:

```makefile
# Compiler
CC := g++

# Platform flags
STEEMFLAGS := -DNDEBUG -DUNIX -DLINUX -DSSE_DRAW_C -DSSE_RELEASE -DSSE_LINUX_DYN

# Include paths
CFLAGS := -I$(ROOT)/include \
          -I$(ROOT)/steem/headers \
          -I$(ROOT)/3rdparty \
          -I$(ROOT)/3rdparty/dsp \
          -I$(ROOT)/3rdparty/6301 \
          -I$(ROOT)/3rdparty/zlib/contrib/minizip \
          -I$(ROOT)/3rdparty/zlib \
          -I$(ROOT)/3rdparty/rtaudio \
          -I$(ROOT)/steem/code \
          -I$(ROOT)/steem/code/x \
          -w -Wfatal-errors -fpermissive \
          -O -O2 -std=c++11
```

**Required Libraries (Linux)**:
```
-lX11           # X11 core
-lXext          # X11 extensions
-lpthread       # POSIX threads
-lXxf86vm       # XFree86 video mode extension
-lasound        # ALSA sound support
-lportaudio     # PortAudio support
-lcapsimage     # CAPS image library
-lpulse         # PulseAudio support
-lz             # Zlib compression
-lminizip       # MiniZip support
-lrtaudio       # RtAudio support
```

**Linux Build Process**:
```bash
# Navigate to X-build directory
cd X-build

# Build 32-bit version
make -B -f Makefile.txt 3rdparty
make -B -f Makefile.txt

# Build 64-bit version
make -B -f Makefile64.txt 3rdparty
make -B -f Makefile64.txt

# Clean build
make clean
```

**Output Files**:
- `X-build/output/xsteem` - Main emulator executable
- Object files in `X-build/obj/`

### X11-Specific Components

**X Interface Files**:
- `steem/code/x/x_stemwin.cpp` - X11 window management
- `steem/code/x/x_display.cpp` - X11 display handling
- `steem/code/x/x_gui.cpp` - X11 GUI implementation
- `steem/code/x/x_harddiskman.cpp` - X11 hard disk management
- `steem/code/x/x_infobox.cpp` - X11 info box
- `steem/code/x/x_joy.cpp` - X11 joystick handling
- `steem/code/x/x_midi.cpp` - X11 MIDI handling
- `steem/code/x/x_options.cpp` - X11 options dialog
- `steem/code/x/x_patchesbox.cpp` - X11 patches box
- `steem/code/x/x_shortcutbox.cpp` - X11 shortcut box
- `steem/code/x/x_sound.cpp` - X11 sound handling
- `steem/code/x/x_stemdialogs.cpp` - X11 dialogs
- `steem/code/x/x_controls.cpp` - X11 controls

**HXC Library**:
- Custom X11 widget library for Steem
- Files: `include/x/hxc*.cpp`, `include/x/hxc*.h`
- Provides cross-platform GUI widgets for X11

## Source Code Organization

### Directory Structure

```
steem/
├── code/                    # Core emulator code
│   ├── *.cpp, *.h           # Main source files
│   ├── draw_c/             # C-based drawing routines
│   └── x/                  # X11-specific code
├── asm/                    # Assembly language routines
│   ├── asm_draw.asm        # Drawing assembly
│   ├── asm_osd_draw.asm    # OSD drawing assembly
│   ├── asm_int.asm         # Interrupt handling
│   └── asm_portio.asm      # Port I/O assembly
├── headers/                # Header files
│   └── *.h                 # Public headers
├── rc/                     # Resource files
│   ├── resource.rc         # Windows resources
│   └── resource.h          # Resource definitions
└── Steem.cpp               # Main project file (BCB)

include/                   # Shared include files
├── *.h                    # Cross-platform headers
└── x/                     # X11-specific includes

3rdparty/                  # Third-party libraries
├── 6301/                  # HD6301 MCU emulator
├── ArchiveAccess/         # Archive access library
├── avi/                   # AVI file support
├── caps/                  # CAPS image library
├── dsp/                   # DSP sound processing
├── pasti/                 # PASTI disk image support
├── rtaudio/               # RtAudio library
├── unrarlib/              # UnRAR library
└── zlib/                  # Zlib compression
```

### Main Source File (Steem.cpp)

**Purpose**: Central project file for Borland C++ Builder that includes all source files

**Structure**:
```cpp
// Conditional compilation setup
#ifdef BCB_BUILD
#pragma message("Build for Borland C++ Builder...")
#include <condefs.h>
#else
#define USEUNIT(ModName)
#define USEOBJ(FileName)
// ... other macros
#endif

// Assembly object files
USELIB("asm\asm_draw.obj");
USELIB("asm\asm_osd_draw.obj");
// ...

// Core emulator files
USEFILE("code\gui.cpp");
USEFILE("code\d2.cpp");
USEFILE("code\acc.cpp");
// ... (100+ source files)

// X11-specific files
USEFILE("code\x\x_stemwin.cpp");
USEFILE("code\x\x_display.cpp");
// ...

// Include files
USEFILE("..\..\include\mymisc.cpp");
USEFILE("..\..\include\easystr.cpp");
// ...

// Resource file
USERC("rc\resource.rc");
```

## Build Process Details

### 1. Preprocessing

**Header Files**:
- `pch.h` - Precompiled header (used for faster compilation)
- `steemh.h` - Main emulator header with global definitions
- Component-specific headers in `headers/` directory

**Precompiled Headers**:
- Enabled in Visual Studio projects
- Reduces compilation time for large codebase
- Includes commonly used headers and definitions

### 2. Compilation

**Compiler Flags by Platform**:

**Windows (MSVC)**:
```
/O2 (Optimize for speed)
/Od (Disable optimizations for debug)
/Zi (Generate debug information)
/nologo (Suppress startup banner)
/W4 (Warning level 4)
/DWIN32 /D_WINDOWS (Windows-specific defines)
```

**Windows (MinGW)**:
```
-Wall (Enable all warnings)
-fdiagnostics-show-option (Show option for diagnostics)
-Wno-write-strings (Disable write-strings warning)
-fpermissive (Permissive template handling)
-ggdb (Generate GDB debug symbols)
-O2 (Optimization level 2)
```

**Linux (GCC)**:
```
-w (Suppress warnings)
-Wfatal-errors (Treat warnings as errors)
-fpermissive (Permissive template handling)
-O -O2 (Optimization)
-std=c++11 (C++11 standard)
```

### 3. Assembly Compilation

**Assembly Files**:
- `asm_draw.asm` - Drawing routines (optimized)
- `asm_osd_draw.asm` - OSD drawing
- `asm_int.asm` - Interrupt handling
- `asm_portio.asm` - Port I/O

**Assembly Build Process**:
- Compiled separately for each platform
- Windows: MASM (Microsoft Macro Assembler)
- Linux: NASM (Netwide Assembler)
- Borland: TASM (Turbo Assembler)

**Assembly Object Files**:
- `asm_draw.obj` / `asm_draw.o`
- `asm_osd_draw.obj` / `asm_osd_draw.o`
- `asm_int.obj` / `asm_int.o`
- `asm_portio.obj` / `asm_portio.o`

### 4. Linking

**Linker Inputs**:
- All compiled object files (.obj or .o)
- Resource files (.res)
- Third-party libraries (.lib or .a)
- System libraries

**Linker Output**:
- Windows: `Steem.exe`
- Linux: `xsteem` (executable)

**Linker Scripts**:
- Visual Studio: Automatic linker configuration
- MinGW: Custom linker scripts in makefiles
- Linux: Standard GCC linker behavior

### 5. Resource Compilation

**Resource Files**:
- `rc/resource.rc` - Main resource script
- `rc/resource.h` - Resource definitions

**Resource Types**:
- Icons: `steem.ico`
- Bitmaps: Various UI bitmaps
- Cursors: Custom cursors
- Version Info: Version information
- Binary Data: Font files, etc.

**Resource Compilation**:
- Windows: `windres` (MinGW) or RC.EXE (MSVC)
- Output: `resource.res` or `resource.o`

## Dependency Management

### Internal Dependencies

**Core Dependencies**:
```
steem.cpp → All source files
source files → headers/ directory
source files → include/ directory
assembly files → include/ directory
```

**Component Dependencies**:
- CPU emulator depends on memory system
- Hardware chips depend on timing system
- Display system depends on shifter and palette
- Sound system depends on PSG and DMA

### Third-Party Dependencies

**Required Libraries**:

| Library | Purpose | Platform |
|---------|---------|----------|
| Zlib | Compression | All |
| MiniZip | ZIP archive support | All |
| RtAudio | Audio I/O | Linux |
| PortAudio | Audio I/O | Linux |
| CAPS Image | Disk image support | All |
| AVI File | Video recording | Windows |
| DirectX | Graphics/Sound | Windows |
| X11 | Graphics | Linux |
| ALSA | Audio | Linux |
| PulseAudio | Audio | Linux |

**Third-Party Source Integration**:
- `3rdparty/6301/` - HD6301 MCU emulator (for IKBD)
- `3rdparty/dsp/` - DSP sound processing
- `3rdparty/pasti/` - PASTI disk image support
- `3rdparty/ArchiveAccess/` - Archive access library

## Build Customization

### Feature Toggles

**Graphics Backend**:
```makefile
# DirectDraw (Windows)
STEEMFLAGS += -DSSE_DD

# Direct3D (Windows)
STEEMFLAGS += -DSSE_D3D

# C-based drawing (cross-platform)
STEEMFLAGS += -DSSE_DRAW_C
```

**Sound Backend**:
```makefile
# PortAudio
STEEMFLAGS += -DSSE_SOUND_PA

# RtAudio
STEEMFLAGS += -DSSE_SOUND_RTA

# PulseAudio
STEEMFLAGS += -DSSE_SOUND_PULSE
```

**Debug Features**:
```makefile
# Full debug build
STEEMFLAGS += -DDEBUG_BUILD -DSSE_DEBUG

# Debug with GDB symbols
STEEMFLAGS += -ggdb

# No debug features
STEEMFLAGS += -DNO_DEBUG_BUILD
```

### Platform-Specific Customization

**Windows-Specific**:
```makefile
STEEMFLAGS += -DWIN32 -D_WINDOWS
LIBS += -lwinmm -luuid -lcomctl32 -lole32
```

**Linux-Specific**:
```makefile
STEEMFLAGS += -DUNIX -DLINUX -DSSE_LINUX_DYN
LIBS += -lX11 -lXext -lpthread -lXxf86vm
```

**Assembly Optimization**:
```makefile
# Enable x86 assembly (default)
# Disable with:
STEEMFLAGS += -DNO_486_ASM
```

## Build Output

### Windows Build Output

**Debug Build**:
- `Steem.exe` (~10-15 MB)
- Debug symbols in separate PDB file (MSVC)
- All debug features enabled

**Release Build**:
- `Steem.exe` (~5-8 MB)
- Optimized for speed
- Debug features disabled

**Build Directories**:
- Visual Studio: `windows-build/VS2019/Debug/` or `Release/`
- MinGW: `windows-build/mingw/SSE/`
- Borland: `windows-build/bcc/`

### Linux Build Output

**Debug Build**:
- `xsteem` executable (~15-20 MB)
- Debug symbols included
- All debug features enabled

**Release Build**:
- `xsteem` executable (~8-12 MB)
- Optimized for speed
- Debug features disabled

**Build Directories**:
- `X-build/output/` - Final executable
- `X-build/obj/` - Object files

## Build Verification

### Post-Build Testing

1. **Basic Functionality Test**:
   - Launch emulator
   - Verify main window appears
   - Check basic menu functionality

2. **Hardware Emulation Test**:
   - Load known-good ROM image
   - Verify boot sequence
   - Check basic input/output

3. **Performance Test**:
   - Measure emulation speed
   - Verify timing accuracy
   - Check for regressions

### Build Artifacts

**Windows**:
- Executable: `Steem.exe`
- Configuration files: Various .ini files
- Documentation: Readme files
- Resource files: Icons, bitmaps

**Linux**:
- Executable: `xsteem`
- Configuration files: Various .cfg files
- Documentation: Readme files
- Resource files: XPM icons, etc.

## Troubleshooting

### Common Build Issues

**Missing Dependencies**:
- Windows: DirectX SDK, Windows SDK
- Linux: X11 development packages, ALSA development packages

**Compiler Compatibility**:
- MSVC: Requires Visual Studio 2008 or later
- MinGW: Requires MinGW-w64 for 64-bit builds
- GCC: Requires GCC 4.8 or later for C++11 support

**Assembly Compatibility**:
- Windows: Requires MASM for MSVC, NASM for MinGW
- Linux: Requires NASM for assembly compilation

**Linker Issues**:
- Missing libraries: Ensure all required .lib/.a files are in library path
- Symbol conflicts: Check for duplicate symbol definitions
- Architecture mismatch: Ensure 32-bit vs 64-bit consistency

### Debugging Build Problems

**Windows (MSVC)**:
- Check Output window for detailed error messages
- Use /Bv flag for verbose output
- Verify include paths in project settings

**Windows (MinGW)**:
```bash
mingw32-make.exe -f makefile_MinGW.txt VERBOSE=1
```

**Linux**:
```bash
make VERBOSE=1
# or
make -n  # Dry run to see commands
```

## Build System Evolution

### Historical Build Systems

1. **Borland C++ Builder (Original)**:
   - First build system for Steem
   - Used .bpr project files
   - Legacy support maintained

2. **Visual Studio 6 (VC6)**:
   - Early Microsoft compiler support
   - .dsp and .dsw project files
   - Limited to 32-bit builds

3. **Visual Studio 2008**:
   - Modern MSVC support
   - 32-bit and 64-bit support
   - Improved debugging

4. **MinGW Support**:
   - Open-source compiler support
   - Cross-platform compatibility
   - GCC compatibility

5. **Linux/X11 Port**:
   - Full cross-platform support
   - Makefile-based build system
   - Native X11 integration

### Current Build Systems

1. **Visual Studio 2019**: Primary Windows development platform
2. **MinGW**: Alternative Windows compiler
3. **GCC/Linux**: Primary Linux development platform

## Conclusion

Steem SSE's build system is designed to support multiple platforms and compilers while maintaining the complex dependencies required for accurate Atari ST emulation. The build system has evolved over time to support modern development environments while maintaining backward compatibility with legacy systems. Each platform has its own optimized build configuration, with platform-specific code paths and feature toggles to ensure the best possible performance and compatibility.
