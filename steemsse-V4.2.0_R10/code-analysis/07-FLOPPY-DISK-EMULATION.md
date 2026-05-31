# Steem Emulator Floppy Disk Emulation

## Overview

Steem SSE implements a comprehensive floppy disk emulation system that accurately reproduces the behavior of the Atari ST's floppy disk subsystem. This includes the Western Digital WD1772 Floppy Disk Controller (FDC), floppy disk drives, and various disk image formats.

**Key Characteristics**:
- Cycle-accurate emulation of WD1772 FDC
- Support for multiple disk image formats (ST, MSA, DIM, STX, IPF, CTR, SCP, HFE, STW, etc.)
- Accurate timing of disk operations
- Drive motor and head positioning simulation
- Integration with DMA system for data transfer
- Support for both standard and protected disks

## Floppy Disk System Architecture

### Component Hierarchy

```
Floppy Disk System
├── FDC (fdc.cpp)                     - WD1772 Floppy Disk Controller
│   ├── Command Processing             - FDC command execution
│   ├── Status Registers              - FDC status and control
│   ├── Data Transfer                 - Read/write operations
│   └── Interrupt Generation          - FDC interrupt handling
│
├── Floppy Drive (floppy_drive.cpp)   - Physical drive emulation
│   ├── TSF314 Class                  - Drive hardware emulation
│   ├── Motor Control                 - Drive motor on/off
│   ├── Head Positioning              - Track and side selection
│   ├── Index Pulse Generation        - Rotation timing
│   └── Sound Emulation               - Drive operation sounds
│
└── Disk Image (floppy_disk.cpp)      - Disk image management
    ├── TFloppyDisk Class             - Disk image representation
    ├── Format-Specific Handlers       - Various image format support
    ├── Track Data Access              - Sector and track reading
    └── Write Protection               - Disk write protection
```

### File Structure

```
steem/
├── fdc.cpp              # WD1772 FDC emulation
├── floppy_drive.cpp     # Floppy drive emulation
├── floppy_disk.cpp      # Disk image management
├── diskman.cpp          # Disk manager (UI and control)
├── diskman_diags.cpp     # Disk manager diagnostics
├── diskman_drag.cpp     # Disk drag-and-drop support
├── headers/fdc.h        # FDC header
├── headers/floppy_drive.h # Floppy drive header
└── headers/floppy_disk.h # Floppy disk header

3rdparty/
├── pasti/              # PASTI disk image support
└── ArchiveAccess/      # Archive access for disk images
```

## WD1772 Floppy Disk Controller Emulation

### FDC Overview

**Hardware**: Western Digital WD1772 (or compatible WD1770)

**Features**:
- 8-bit data bus interface
- 4 floppy disk drive support (Atari ST uses 2)
- Programmed I/O or DMA data transfer
- Single-density and double-density support
- MFM (Modified Frequency Modulation) encoding
- 8" and 5.25" drive support

**Registers**:
```
$FF8600 - Status Register (read-only)
$FF8600 - Command Register (write-only)
$FF8602 - Track Register (write-only)
$FF8604 - Sector Register (write-only)
$FF8606 - Data Register (read/write)
```

### FDC Implementation (fdc.cpp)

**Main Classes**:
```cpp
class TFDC {
public:
    BYTE cr;              // Command Register
    BYTE sr;              // Status Register
    BYTE tr;              // Track Register
    BYTE sc;              // Sector Register
    BYTE dr;              // Data Register
    
    // Internal state
    int StatusType;       // Current status type
    int InterruptCondition; // Current interrupt condition
    int IndexCounter;     // Index pulse counter
    bool bDataRequest;    // Data request flag
    bool bLostData;       // Lost data flag
    
    // Methods
    void Reset();
    BYTE IORead(BYTE reg);
    void IOWrite(BYTE reg, BYTE val);
    void OnIndexPulse(int drive);
    void ExecuteCommand();
    int CommandType(BYTE cmd = 0);
    // ...
};

TFDC Fdc;  // Global FDC instance
```

### FDC Register Implementation

**Status Register ($FF8600)**:
```cpp
#define FDC_STR_BSY     0x01  // Busy
#define FDC_STR_DRQ     0x02  // Data Request
#define FDC_STR_LD      0x04  // Lost Data
#define FDC_STR_CRCERR  0x08  // CRC Error
#define FDC_STR_SE      0x10  // Seek Error
#define FDC_STR_ND      0x20  // Not Data (ID field)
#define FDC_STR_OR      0x40  // Overrun
#define FDC_STR_MO      0x80  // Motor On
```

**Command Register ($FF8600)**:
```cpp
// Command types
#define FDC_CR_TYPE1    0x00  // Type 1: Read/Write Sector, Read Address, etc.
#define FDC_CR_TYPE2    0x40  // Type 2: Read Track
#define FDC_CR_TYPE3    0x80  // Type 3: Write Track
#define FDC_CR_TYPE4    0xC0  // Type 4: Force Interrupt

// Command bits
#define FDC_CR_H        0x08  // Head select (side)
#define FDC_CR_V        0x04  // Verify flag
#define FDC_CR_U        0x02  // Update track register flag
#define FDC_CR_S        0x01  // Side compare flag
```

### FDC Command Processing

**Command Execution Flow**:
```cpp
void fdc_command(BYTE cm) {
    // Check if FDC is busy
    if(Fdc.str & FDC_STR_BSY) {
        // Handle command while busy
        if(fdc_spinning_up || !OPTION_HACKS || 
           Fdc.CommandType() == 1 && Fdc.CommandType(cm) == 1) {
            TRACE_LOG("CR %X->%X\n", Fdc.cr, cm);
        }
        else if((cm & 0xF0) != 0xD0) { // Not force interrupt
            TRACE_LOG("Command %X ignored\n", cm);
            return;
        }
    }
    
    // Clear interrupt
    if(Fdc.InterruptCondition != 8) {
        fdc_irq = false;
        update_disk_irq();
    }
    
    Fdc.InterruptCondition = 0;
    floppy_irq_flag = 0;
    Fdc.cr = cm;
    
    // Delete pending events
    agenda_delete(agenda_fdc_finished);
    agenda_delete(agenda_fdc_motor_flag_off);
    
    // Handle motor control
    bool delay_exec = false;
    if(DRIVE < DiskMan.nFloppyDrives && !(Fdc.str & FDC_STR_MO)) {
        if((cm & 0xF0) != 0xd0 && !(cm & Fdc.CR_H)) {
            delay_exec = true;
        }
        Fdc.str = FDC_STR_BSY | FDC_STR_MO;
        fdc_spinning_up = delay_exec ? 2 : 1;
        FloppyDrive[DRIVE].Motor(true);
        
        if(FloppyDrive[DRIVE].Empty()) {
            // No IP
        }
        else if(DiskMan.bTurboDrive) {
            agenda_add(agenda_fdc_spun_up, milliseconds_to_hbl(100), delay_exec);
        }
        else {
            Fdc.IndexCounter = 0;
            WORD delay = FloppyDrive[DRIVE].HblsNextIndex();
            agenda_add(agenda_fdc_spun_up, delay, delay_exec);
        }
    }
    
    if(!delay_exec) {
        fdc_execute();
    }
}
```

**Command Types**:

1. **Type 1 Commands** (Read/Write Sector, Read Address, etc.):
   - Read Sector
   - Write Sector
   - Read Address (ID)
   - Read Track
   - Write Track
   - Verify
   - Seek
   - Restore
   - Step
   - Step-In
   - Step-Out

2. **Type 2 Commands** (Read Track):
   - Read Track with deleted data

3. **Type 3 Commands** (Write Track):
   - Write Track with deleted data

4. **Type 4 Commands** (Force Interrupt):
   - Force Interrupt

### FDC Command Execution

**Command Execution Function**:
```cpp
void fdc_execute() {
    BYTE command = Fdc.cr & 0xF0;  // Command type
    
    switch(command) {
        case 0x00:  // Type 1 commands
            fdc_type1_execute();
            break;
        case 0x40:  // Type 2: Read Track
            fdc_type2_execute();
            break;
        case 0x80:  // Type 3: Write Track
            fdc_type3_execute();
            break;
        case 0xC0:  // Type 4: Force Interrupt
            fdc_type4_execute();
            break;
    }
}
```

**Type 1 Command Execution**:
```cpp
void fdc_type1_execute() {
    BYTE subcommand = Fdc.cr & 0x0F;
    
    switch(subcommand) {
        case 0x00:  // Restore
            fdc_restore();
            break;
        case 0x01:  // Seek
            fdc_seek();
            break;
        case 0x02:  // Step
            fdc_step();
            break;
        case 0x03:  // Step-In
            fdc_step_in();
            break;
        case 0x04:  // Step-Out
            fdc_step_out();
            break;
        case 0x05:  // Step to track 0
            fdc_step_to_0();
            break;
        case 0x06:  // Read Sector
            fdc_read_sector();
            break;
        case 0x07:  // Write Sector
            fdc_write_sector();
            break;
        case 0x08:  // Read Address (ID)
            fdc_read_address();
            break;
        case 0x09:  // Read Track
            fdc_read_track();
            break;
        case 0x0A:  // Write Track
            fdc_write_track();
            break;
        case 0x0B:  // Verify
            fdc_verify();
            break;
    }
}
```

### FDC Interrupt System

**Interrupt Conditions**:
```cpp
#define FDC_IRQ_COMPLETION    0  // Command completion
#define FDC_IRQ_INDEX          1  // Index pulse
#define FDC_IRQ_DRQ            2  // Data request
#define FDC_IRQ_LOST_DATA      3  // Data lost
#define FDC_IRQ_CRC_ERROR      4  // CRC error
#define FDC_IRQ_SEEK_ERROR     5  // Seek error
#define FDC_IRQ_NOT_DATA       6  // Not data (ID field)
#define FDC_IRQ_OVERRUN        7  // Overrun
#define FDC_IRQ_FORCE          8  // Force interrupt
```

**Interrupt Handling**:
```cpp
void update_disk_irq() {
    // Check if FDC interrupt should be active
    bool new_irq = fdc_irq && (Fdc.str & FDC_STR_BSY) == 0;
    
    // Update MFP interrupt input
    if(new_irq != floppy_irq_flag) {
        floppy_irq_flag = new_irq;
        Mfp.SetInput(MFP_INPUT_FDC, new_irq);
    }
}

void fdc_set_irq(int condition) {
    Fdc.InterruptCondition = condition;
    fdc_irq = true;
    update_disk_irq();
}
```

## Floppy Drive Emulation

### Drive Implementation (floppy_drive.cpp)

**TSF314 Class**: Represents a single floppy drive

```cpp
class TSF314 {
public:
    int Id;                     // Drive ID (0=A, 1=B)
    bool bDiskInDrive;           // Disk inserted flag
    bool bMotor;                 // Motor state
    BYTE track;                  // Current track (0-83)
    bool reading;                // Read operation in progress
    bool writing;                // Write operation in progress
    
    // Disk image
    TFloppyDiskImageType ImageType;
    void* MfmManager;           // MFM manager for specific formats
    
    // Timing
    COUNTER_VAR time_of_last_ip;  // Last index pulse time
    COUNTER_VAR time_of_next_ip;  // Next index pulse time
    int cycles_per_byte;         // Cycles per byte
    
    // Methods
    TSF314();
    void Init();
    void Restore(BYTE myid);
    void UpdateAdat(bool bRefreshGUI);
    WORD BytePosition();
    WORD BytesToHbls(int bytes);
    DWORD HblsAtIndex();
    WORD HblsNextIndex();
    WORD HblsPerRotation();
    WORD HblsToBytes(WORD hbls);
    int CyclesPerByte();
    void IndexPulse();
    void Motor(bool state);
    bool Empty();
    bool DiskInDrive();
    // ...
};

TSF314 FloppyDrive[2];  // Two drives (A and B)
```

### Drive Motor Control

**Motor State Management**:
```cpp
void TSF314::Motor(bool state) {
    if(Id >= DiskMan.nFloppyDrives)
        state = false;  // No drive
    
    TFloppyDisk &disk = FloppyDisk[Id];
    
    #if defined(SSE_DEBUGGER_FAKE_IO)
    if(state != bMotor && (TRACE_MASK3 & TRACE_CONTROL_FDCPSG)) {
        TRACE_LOG("FDC(%d) Drive %c: motor %s\n", 
                 DiskEmu.LastManager, 'A'+Id, state ? "on" : "off");
    }
    #endif
    
    if(ImageType.Manager != MNGR_WD1772) {
        // Non-WD1772 manager (e.g., PASTI)
    }
    else if(bMotor && !state) { // Stopping
        // Record position
    }
    else if(!bMotor && state) { // Starting
        // Start motor
        bMotor = true;
        
        // Program next index pulse
        if(!disk.Empty()) {
            COUNTER_VAR cycles = (nSysCyclesPerSecond * 60 / DRIVE_RPM);
            if(!ADAT) {
                cycles *= DRIVE_FAST_IP_MULTIPLIER;
            }
            time_of_next_ip = time_of_last_ip + cycles;
            
            // Send pulse to FDC if this drive is selected
            if(Psg.CurrentDrive() == Id) {
                Fdc.OnIndexPulse(Id);
            }
        }
    }
    
    bMotor = state;
    
    #if defined(SSE_DRIVE_SOUND)
    SoundCheckMotor(state);
    #endif
}
```

### Index Pulse Generation

**Index Pulse Timing**:
```cpp
void TSF314::IndexPulse() {
    TFloppyDisk &disk = FloppyDisk[Id];
    
    time_of_next_ip = time_of_next_event + nSysCyclesPerSecond;
    
    if(ImageType.Manager != MNGR_WD1772 || !bMotor || Empty())
        return;
    
    time_of_last_ip = time_of_next_event;
    
    // Reset byte position at track start
    if(!reading && !writing || disk.current_byte >= disk.TrackBytes - 1) {
        disk.current_byte = 0;
    }
    
    // Program next index pulse
    COUNTER_VAR cycles = (nSysCyclesPerSecond * 60 / DRIVE_RPM);
    if(!ADAT) {
        cycles *= DRIVE_FAST_IP_MULTIPLIER;
    }
    time_of_next_ip = time_of_last_ip + cycles;
    
    // Send pulse to FDC
    if(Psg.CurrentDrive() == Id) {
        Fdc.OnIndexPulse(Id);
    }
}
```

**Index Pulse Detection**:
```cpp
bool floppy_track_index_pulse_active() {
    TSF314 &drive = FloppyDrive[DRIVE];
    bool bActive = false;
    
    if(Fdc.StatusType == 0) {
        // No active status
    }
    else if(ADAT) {
        // Accurate Disk Access Times
        bActive = (drive.bMotor && drive.BytePosition() <= 125);
    }
    else if(DRIVE < DiskMan.nFloppyDrives) {
        DWORD hbl_rev = DRIVE_HBLS_PER_ROTATION;
        bActive = ((hbl_count % hbl_rev) >= 
                 (DWORD)(hbl_rev - DRIVE_HBLS_OF_INDEX_PULSE));
    }
    
    return bActive;
}
```

### Drive Sound Emulation

**Sound Types**:
```cpp
enum EDriveSound {
    START,      // Motor start sound
    MOTOR,      // Motor running sound
    STEP,       // Step sound
    SEEK,       // Seek sound
    NSOUNDS     // Number of sounds
};
```

**Sound Implementation**:
- Windows: DirectSound buffers
- Linux: PortAudio or PulseAudio streams
- Configurable volume and mixing

## Disk Image Emulation

### Disk Image Management (floppy_disk.cpp)

**TFloppyDisk Class**:
```cpp
class TFloppyDisk {
public:
    int Id;                     // Disk ID (matches drive ID)
    FILE* fp;                   // File pointer
    FILE* Format_fp;            // Format file pointer
    
    // Disk properties
    int TrackBytes;             // Bytes per track
    int SectorsPerTrack;        // Sectors per track
    int TracksPerSide;          // Tracks per side
    int Sides;                  // Number of sides
    int Density;                // Density (0=SD, 1=DD, 2=HD)
    
    // Current position
    WORD current_byte;          // Current byte position
    
    // Format-specific
    bool PastiDisk;             // PASTI disk flag
    BYTE* PastiBuf;             // PASTI buffer
    bool STT_File;              // STT format flag
    bool STX_File;              // STX format flag
    bool STW_File;              // STW format flag
    bool SCP_File;              // SCP format flag
    bool HFE_File;              // HFE format flag
    
    // Track formatting
    bool TrackIsFormatted[2][84]; // Formatted flag per side/track
    
    // Methods
    TFloppyDisk();
    void Init();
    int BytePositionOfFirstId();
    int BytesToID(BYTE &num);
    int HblsPerSector();
    void NextID(BYTE &RecordIdx, int &nHbls);
    int nSectors();
    int PostIndexGap();
    int PreDataGap();
    int PostDataGap();
    int PreIndexGap();
    int RecordLength();
    int SectorGap();
    int TrackGap();
    int GetIDFields(int Side, int Track, TWD1772IDField *IDList);
    // ...
};

TFloppyDisk FloppyDisk[2];  // Disk images for both drives
```

### Disk Image Formats

**Supported Formats**:
```cpp
char *extension_list[NUM_EXT] = {
    "", "ST", "MSA", "DIM", "STT", "STX", "IPF",
    "CTR", "STG", "STW", "PRG", "TOS", "SCP", "HFE"
};

char* disk_manager[NUM_MNGR] = {
    "NONE", "STEEM", "PASTI", "CAPS", "WD1772", "PRG", "ACSI"
};
```

**Format Descriptions**:

1. **ST (Atari ST Disk Image)**:
   - Raw sector dump
   - No track information
   - Simple format, widely supported

2. **MSA (Atari MSA Disk Image)**:
   - Sector-based format
   - Includes metadata
   - Supports various sector sizes

3. **DIM (Disk Image)**:
   - Track-based format
   - Preserves track layout
   - Supports copy protection

4. **STT (Steem Track File)**:
   - Track-based format
   - Preserves exact track data
   - Supports non-standard tracks

5. **STX (Steem Extended)**:
   - Enhanced track-based format
   - Supports copy protection
   - Used by PASTI

6. **IPF (Interchangeable Preservation Format)**:
   - Advanced preservation format
   - Supports all copy protections
   - Used by CAPS library

7. **CTR (CAPS Track)**:
   - CAPS-specific format
   - Used with IPF

8. **STG (Steem Ghost)**:
   - Ghost disk format
   - Records all write operations

9. **STW (Steem Write)**:
   - Write-protected format
   - Records all changes

10. **SCP (SuperCard Pro)**:
    - Advanced copy protection support
    - Cycle-accurate timing

11. **HFE (HxC Floppy Emulator)**:
    - HxC format support
    - Used by various hardware emulators

12. **PRG (Atari Program)**:
    - Single-file program format
    - Loaded as boot sector

### Disk Image Loading

**Image Loading Process**:
```cpp
bool TFloppyDisk::Open(EasyStr path, int manager) {
    // Close any existing file
    if(fp) {
        fclose(fp);
        fp = NULL;
    }
    
    // Open new file
    fp = fopen(path.Text, "rb");
    if(!fp) {
        return false;
    }
    
    // Determine image type
    ImageType.RealExtension = GetExtension(path.Text);
    ImageType.Extension = ImageType.RealExtension;
    ImageType.Manager = manager;
    
    // Initialize based on format
    switch(ImageType.Extension) {
        case EXT_ST:
            return OpenST();
        case EXT_MSA:
            return OpenMSA();
        case EXT_DIM:
            return OpenDIM();
        case EXT_STT:
            return OpenSTT();
        case EXT_STX:
            return OpenSTX();
        // ... other formats
    }
    
    return true;
}
```

### Track and Sector Access

**Sector Positioning**:
```cpp
int TFloppyDisk::BytesToID(BYTE &num) {
    /* Compute distance in bytes between current byte and desired ID */
    int bytes_to_id = 0;
    const WORD my_current_byte = FloppyDrive[Id].BytePosition();
    
    if(!FloppyDrive[Id].Empty()) {
        int record_length = RecordLength();
        int n_sectors = nSectors();
        int byte_first_id = BytePositionOfFirstId();
        int byte_target_id;
        
        // If looking for next sector
        if(!num) {
            num = (BYTE)((my_current_byte - byte_first_id) / record_length + 1);
            if(((my_current_byte) % record_length) >= byte_first_id)
                num++;
            if(num >= n_sectors + 1)
                num = 1;
        }
        
        byte_target_id = byte_first_id + (num - 1) * record_length;
        bytes_to_id = byte_target_id - my_current_byte;
        
        if(bytes_to_id < 0) // Passed it
            bytes_to_id += TrackBytes; // Next revolution
    }
    
    return bytes_to_id;
}
```

**Sector Reading**:
```cpp
bool TFloppyDisk::ReadSector(int side, int track, int sector, BYTE* buffer) {
    // Check if sector is valid
    if(track >= TracksPerSide || side >= Sides || sector >= SectorsPerTrack)
        return false;
    
    // Seek to sector position
    if(!SeekSector(side, track, sector, false))
        return false;
    
    // Read sector data
    int sector_size = GetSectorSize();
    if(fread(buffer, 1, sector_size, fp) != sector_size)
        return false;
    
    return true;
}
```

## Disk Image Format Handlers

### STEEM Manager (Native Steem Format)

**ST Format Handler**:
- Simple raw sector dump
- Fixed sector size (512 bytes)
- No track information
- Fast loading

**MSA Format Handler**:
- Sector-based with metadata
- Variable sector sizes
- Supports different disk types

**DIM Format Handler**:
- Track-based format
- Preserves track layout
- Supports copy protection

### WD1772 Manager (FDC-Level Emulation)

**STW Format Handler**:
- Steem Write format
- Records all write operations
- Supports write protection

**SCP Format Handler**:
- SuperCard Pro format
- Cycle-accurate timing
- Advanced copy protection support

**HFE Format Handler**:
- HxC Floppy Emulator format
- Used by various hardware emulators
- Supports different encoding schemes

### PASTI Manager (Protected Disk Support)

**PASTI Integration**:
- External PASTI library
- Supports IPF and CTR formats
- Advanced copy protection emulation
- Cycle-accurate timing

**PASTI Initialization**:
```cpp
#if USE_PASTI
HINSTANCE hPasti = NULL;
const struct pastiFUNCS *pasti = NULL;
char pasti_file_exts[PASTI_FILE_EXTS_BUFFERSIZE];
WORD pasti_store_byte_access;
bool pasti_active = false;

void pasti_init() {
    hPasti = LoadLibrary("pasti.dll");
    if(hPasti) {
        // Get PASTI function pointers
        pasti = (const struct pastiFUNCS*)GetProcAddress(hPasti, "pastiFUNCS");
        if(pasti) {
            pasti->getFileExts(pasti_file_exts);
            pasti_active = true;
        }
    }
}
#endif
```

### CAPS Manager (IPF/CTR Support)

**CAPS Integration**:
- External CAPS library
- Supports IPF (Interchangeable Preservation Format)
- Supports CTR (CAPS Track)
- Advanced copy protection support

## Disk Management System

### Disk Manager (diskman.cpp)

**Disk Manager Class**:
```cpp
class TDiskMan {
public:
    int nFloppyDrives;           // Number of floppy drives
    bool bDiskProtectImage;      // Write protection flag
    bool bDiskProtectImageStx;   // STX write protection
    bool bTurboDrive;            // Turbo drive mode
    
    // UI
    HWND Handle;                 // Disk manager window handle
    
    // Methods
    void Init();
    bool InsertDisk(int drive, EasyStr path);
    bool EjectDisk(int drive);
    void ToggleDiskProtection(int drive);
    void UpdateUI();
    bool IsVisible();
    // ...
};

TDiskMan DiskMan;  // Global disk manager instance
```

### Disk Operations

**Disk Insertion**:
```cpp
bool TDiskMan::InsertDisk(int drive, EasyStr path) {
    if(drive >= nFloppyDrives)
        return false;
    
    // Eject any existing disk
    EjectDisk(drive);
    
    // Determine image type from extension
    int ext = GetExtensionIndex(path.Text);
    int manager = MNGR_STEEM;
    
    switch(ext) {
        case EXT_STX:
            manager = MNGR_PASTI;
            break;
        case EXT_IPF:
        case EXT_CTR:
            manager = MNGR_CAPS;
            break;
        case EXT_STW:
        case EXT_SCP:
        case EXT_HFE:
            manager = MNGR_WD1772;
            break;
    }
    
    // Open disk image
    if(!FloppyDisk[drive].Open(path, manager))
        return false;
    
    // Update drive state
    FloppyDrive[drive].Restore(drive);
    
    // Update UI
    UpdateUI();
    
    return true;
}
```

**Disk Ejection**:
```cpp
bool TDiskMan::EjectDisk(int drive) {
    if(drive >= nFloppyDrives)
        return false;
    
    // Close disk image
    FloppyDisk[drive].Close();
    
    // Update drive state
    FloppyDrive[drive].Restore(drive);
    
    // Update UI
    UpdateUI();
    
    return true;
}
```

## Data Transfer System

### DMA Integration

**DMA Data Transfer**:
```cpp
// Writing bytes to DMA FIFO
void write_to_dma(BYTE Val, int Num = 1) {
    int n = (Num <= 0) ? 1 : Num;
    for(int i = 0; i < n; i++)
        Dma.AddToFifo(MNGR_STEEM, Val);
}

// Reading from DMA FIFO
BYTE read_from_dma() {
    return Dma.GetFifoByte(MNGR_STEEM);
}
```

**FDC-DMA Interaction**:
- FDC reads data from disk and sends to DMA
- DMA transfers data to memory
- FDC writes data from DMA to disk
- Interrupt generation on completion

### Data Flow

**Read Operation**:
1. FDC receives Read Sector command
2. FDC positions head at correct track/sector
3. FDC reads sector data from disk image
4. FDC sends data bytes to DMA FIFO
5. DMA transfers data to memory
6. FDC generates interrupt on completion

**Write Operation**:
1. FDC receives Write Sector command
2. FDC positions head at correct track/sector
3. DMA transfers data from memory to FIFO
4. FDC reads data bytes from DMA FIFO
5. FDC writes data to disk image
6. FDC generates interrupt on completion

## Timing and Synchronization

### Accurate Disk Access Times (ADAT)

**ADAT Configuration**:
```cpp
void TSF314::UpdateAdat(bool const bRefreshGUI) {
    /* ADAT = Accurate Disk Access Times */
    bAdat = (!DiskMan.bTurboDrive && ImageType.Manager == MNGR_STEEM
           || ImageType.Extension == EXT_STX || ImageType.Manager == MNGR_CAPS
           || ImageType.Manager == MNGR_WD1772 && 
              (!DiskMan.bTurboDrive || ImageType.Extension == EXT_SCP));
    
    if(bRefreshGUI) {
        #ifdef WIN32
        if(DiskMan.IsVisible()) {
            InvalidateRect(GetDlgItem(DiskMan.Handle, IDC_DRIVEA), NULL, FALSE);
            InvalidateRect(GetDlgItem(DiskMan.Handle, IDC_DRIVEB), NULL, FALSE);
        }
        #endif
    }
}
```

**ADAT vs Turbo Drive**:
- **ADAT (Accurate)**: Cycle-accurate disk access timing
- **Turbo Drive**: Faster disk operations (less accurate)
- Configurable per drive

### Timing Constants

**Drive Constants**:
```cpp
#define DRIVE_RPM               300     // Rotations per minute
#define DRIVE_FAST_CYCLES_PER_BYTE  16   // Fast mode cycles per byte
#define DRIVE_FAST_IP_MULTIPLIER     6   // Fast mode IP multiplier
#define FLOPPY_MAX_TRACK_NUM         83   // Maximum track number
```

**Timing Calculations**:
```cpp
int TSF314::CyclesPerByte() {
    int cycles;
    
    #if defined(SSE_MEGASTE)
    if(MegaSte.FdHd & BIT_0)
        cycles = (STE_CLOCK8 * TICKS8 * 2); // Double speed
    else
    #endif
    
    if(IS_STE) // STE has separate 8MHz quartz
        cycles = STE_CLOCK8 * TICKS8;
    else
        cycles = CpuNormalHz;
    
    cycles /= DRIVE_RPM / 60; // Per rotation
    cycles /= FloppyDisk[Id].TrackBytes; // Per byte
    
    if(!ADAT)
        cycles = DRIVE_FAST_CYCLES_PER_BYTE; // Fast mode
    
    cycles_per_byte = cycles;
    return cycles;
}
```

## Error Handling and Recovery

### Disk Error Handling

**Error Conditions**:
- Disk not inserted
- Invalid track/sector
- Unformatted track
- CRC errors
- Seek errors
- Overrun errors

**Error Recovery**:
```cpp
bool fdc_handle_file_error(bool floppyno, bool bWrite, int sector,
                            int PosInSector, bool bFromFormat) {
    static DWORD last_reinsert_time[2] = {0, 0};
    TSF314 &drive = FloppyDrive[floppyno];
    TFloppyDisk &disk = FloppyDisk[floppyno];
    
    TRACE_LOG("%s %s\n", ((bWrite) ? "Write" : "Read"), "ERROR");
    
    bool bWorkingNow = false;
    
    if(timer >= last_reinsert_time[floppyno] + 2000 && drive.DiskInDrive()) {
        // Over 2 seconds since last failure
        FILE *fpDest = NULL;
        
        if(bFromFormat) {
            if(disk.ReopenFormatFile())
                fpDest = disk.Format_fp;
            else if(drive.ReinsertDisk())
                fpDest = disk.fp;
        }
        
        if(fpDest) {
            if(disk.SeekSector(floppy_current_side(), drive.track, sector, bFromFormat)) {
                FSEEK(fpDest, PosInSector, SEEK_CUR);
                BYTE temp;
                
                if(bWrite) {
                    temp = Dma.GetFifoByte(MNGR_STEEM);
                    if(!DiskMan.bDiskProtectImage)
                        bWorkingNow = (FWRITE(&temp, 1, 1, fpDest) > 0);
                }
                else {
                    bWorkingNow = (FREAD(&temp, 1, 1, fpDest) > 0);
                    if(DMA_ADDRESS_IS_VALID_W && Dma.Counter)
                        write_to_dma(temp, 0);
                }
            }
        }
        
        #if !defined(SSE_LIBRETRONUKE)
        else {
            DiskMan.EjectDisk(floppyno);
        }
        #endif
    }
    
    last_reinsert_time[floppyno] = timer;
    return (!bWorkingNow);
}
```

### Disk Reinsertion

**Automatic Reinsertion**:
- Attempts to reopen disk after errors
- Handles temporary file access issues
- Configurable timeout between attempts

## Special Features

### Ghost Disk Support

**Ghost Disk Concept**:
- Records all write operations to a separate file
- Allows testing of write-protected software
- Can be replayed for debugging

**Ghost Disk Implementation**:
```cpp
#if defined(SSE_DISK_GHOST)
bool TSF314::CheckGhostDisk(bool const bWrite) {
    if(!bGhost) {
        EasyStr STGPath = FloppyDisk[Id].GetImageFile();
        STGPath += dot_ext(EXT_STG);
        if(bWrite || Exists(STGPath)) {
            if(GhostDisk[Id].Open(STGPath.Text))
                bGhost = true;
        }
    }
    return bGhost;
}
#endif
```

### M3U Playlist Support

**M3U Disk Swapper**:
- Supports M3U playlist files
- Automatic disk swapping
- Multiple disk image support

**M3U Implementation**:
```cpp
class TDumbDiskSwapper {
public:
    EasyStr m3upath;
    int num_images;
    int image_index;
    EasyStr image_path;
    bool ejected;
    
    TDumbDiskSwapper();
    WORD Open(EasyStr path);
    EasyStr GetPath(WORD index);
};

TDumbDiskSwapper DumbDiskSwapper;
```

## Performance Considerations

### Optimization Techniques

1. **Caching**:
   - Track data caching
   - Sector caching
   - ID field caching

2. **Lazy Loading**:
   - Load tracks on demand
   - Unload unused tracks
   - Memory-efficient loading

3. **Fast Paths**:
   - Turbo drive mode for faster operations
   - Simplified timing for non-accurate mode
   - Direct memory access where possible

### Memory Usage

**Memory Allocation**:
- Track buffers: ~6-12KB per track
- Sector buffers: 512-1024 bytes per sector
- ID field cache: Small per-track overhead
- Ghost disk buffers: Variable based on writes

**Memory Management**:
- Dynamic allocation based on disk size
- Automatic cleanup on disk ejection
- Memory usage monitoring

## Platform-Specific Considerations

### Windows Implementation

**File I/O**:
- Standard Windows file API
- Async I/O support
- Memory-mapped files where beneficial

**Sound**:
- DirectSound for drive sounds
- Wave file playback
- Volume control

### Linux Implementation

**File I/O**:
- Standard POSIX file API
- Large file support
- Memory-mapped files

**Sound**:
- PortAudio support
- PulseAudio support
- ALSA support

## Debugging and Testing

### Debug Features

**Trace Logging**:
```cpp
#define LOGSECTION_FDC  1  // FDC-specific logging

TRACE_LOG("FDC(%d) Drive %c: motor %s\n", 
          DiskEmu.LastManager, 'A'+Id, state ? "on" : "off");
```

**Debug Information**:
- FDC register state
- Drive state
- Disk position
- Timing information
- Error conditions

### Test Cases

**Standard Test Cases**:
- Boot from various disk images
- Read/write operations
- Seek operations
- Error handling
- Copy protection schemes

**Performance Tests**:
- Load time measurement
- Seek time measurement
- Data transfer rate
- CPU usage during operations

## Configuration Options

### User-Configurable Options

**Disk Options**:
- Number of floppy drives (1-2)
- Turbo drive mode
- Write protection
- Automatic disk insertion
- Disk image format preferences

**FDC Options**:
- FDC emulation accuracy
- Interrupt behavior
- Command timing
- Error handling

### Compile-Time Options

**Feature Toggles**:
```cpp
#define USE_PASTI           // Enable PASTI support
#define SSE_DISK_GHOST     // Enable ghost disk support
#define SSE_DISK_STW       // Enable STW format support
#define SSE_DISK_SCP       // Enable SCP format support
#define SSE_DISK_HFE       // Enable HFE format support
#define SSE_DRIVE_SOUND    // Enable drive sound emulation
#define SSE_DISK_M3U       // Enable M3U playlist support
```

## Future Enhancements

### Planned Improvements

1. **Additional Format Support**:
   - More disk image formats
   - Better copy protection support
   - Improved error handling

2. **Performance Optimizations**:
   - Faster track loading
   - Better caching strategies
   - Reduced memory usage

3. **Enhanced Features**:
   - Disk image creation tools
   - Disk image conversion
   - Disk image analysis

4. **Debugging Tools**:
   - Disk image inspector
   - Track viewer
   - Sector editor

### Proposed Features

1. **Disk Image Mounting**:
   - Virtual filesystem support
   - Read-only mounting
   - Write-through caching

2. **Advanced Copy Protection**:
   - More protection schemes
   - Better timing accuracy
   - Anti-piracy emulation

3. **Network Disk Support**:
   - Remote disk images
   - Network-based disk swapping
   - Collaborative debugging

## Conclusion

Steem SSE's floppy disk emulation system provides a comprehensive and accurate implementation of the Atari ST's floppy disk subsystem. The system combines cycle-accurate emulation of the WD1772 FDC with flexible support for various disk image formats, ensuring compatibility with a wide range of Atari ST software.

The architecture separates concerns between the FDC emulation, drive mechanics, and disk image management, allowing for independent development and testing of each component. The extensive format support, including advanced copy protection schemes through PASTI and CAPS, makes Steem SSE one of the most compatible Atari ST emulators available.

The floppy disk system's integration with the emulator's event system and DMA controller ensures accurate timing and data transfer, providing a faithful reproduction of the real hardware behavior. This level of accuracy is essential for running protected software and validating emulator correctness.
