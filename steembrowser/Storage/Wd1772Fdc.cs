using System;

/// <summary>
/// Western Digital WD1772A Floppy Disk Controller
/// DIP40 package, memory-mapped at $FF8600-$FF86FF
/// 
/// Handles reading/writing MFM-encoded data from 3.5" floppy drives.
/// Supports up to 2 drives, 15 MHz clock (DD), 10 MHz (HD).
/// 
/// Pinout (DIP-40):
///   READ (1): Read data from drive (active low, 1K pull-up)
///   SIDE (2): Disk side select (active high)
///   !DRQ/!DF1 (4): DMA request / Drive 1 select
///   DRQ (5): DMA request (active low)
///   DF0 (6): Drive 0 select (active low)
///   DF1 (7): Drive 1 select (active low)
///   !MOTON (8): Motor on (active low)
///   !DIR (9): Step direction output
///   SP (10): Step pulse output
///   !WD (11): Write data output
///   !WG (12): Write gate (active low)
///   !TRK0 (13): Track 0 detect (active low)
///   !WP (14): Write protect detect (active low)
///   !INTRQ (16): Interrupt request (active low)
///   !BUSY (17): Busy status (active low)
///   BUSY (18): Busy status (active high)
///   RESET (27): Reset input (active high)
///   D0-D7 (29-36): 8-bit data bus
///   INDEX (38): Index pulse from drive
///   Vcc (40): +5V supply
/// </summary>
public class Wd1772Fdc
{
    // FDC Registers (ST-mapped via config register)
    
    /// <summary>
    /// Status Register ($FF8601)
    /// Bit 0: READY - Drive ready
    /// Bit 1: DATA - Data register ready
    /// Bit 2: DMAREQ - DMA request active
    /// Bit 3: INDEX - Index pulse detected
    /// Bit 4: MOTOR - Motor on
    /// Bit 5: !INTRQ - Interrupt request active (inverted)
    /// Bit 6: Unused
    /// Bit 7: !BUSY - FDC busy (inverted)
    /// </summary>
    public byte Status { get; private set; }
    
    /// <summary>
    /// Track Register ($FF8605) - Current track number
    /// </summary>
    public byte Track { get; private set; }
    
    /// <summary>
    /// Sector Register ($FF8603) - Current sector number
    /// </summary>
    public byte Sector { get; private set; }
    
    /// <summary>
    /// Data Register ($FF8607) - Data read/write
    /// </summary>
    public byte Data { get; private set; }
    
    /// <summary>
    /// Access/Command Byte ($FF8604) - Command byte or data depending on register select
    /// 
    /// Commands:
    ///   0x00: Restore (move head to track 0)
    ///   0x10: Seek (seek to specified track)
    ///   0x20: Step (step head one track)
    ///   0x40: Step In (step head inward)
    ///   0x60: Step Out (step head outward)
    ///   0x80: Read Sector (read one sector)
    ///   0xA0: Write Sector (write one sector)
    ///   0xC0: Read Address (read sector address mark)
    ///   0xE0: Read Track (read raw track data MFM)
    ///   0xF0: Write Track (write raw track data MFM)
    /// 
    /// Modifiers (OR into command):
    ///   0x10: Update track register / Multiple sector
    ///   0x04: Add 30ms delay / Interrupt on index pulse
    ///   0x02: Write precomp disabled
    ///   0x01: Write deleted data mark
    ///   0x08: Immediate interrupt
    /// </summary>
    public byte AccessByte { get; private set; }
    
    /// <summary>
    /// Configuration Register ($FF8606)
    /// Bit 7: FDC/HDC - 1 = FDC mode, 0 = HDC mode
    /// Bit 6: DMA - 1 = enable DMA transfer
    /// Bit 5: WRIT - 1 = write mode, 0 = read mode
    /// Bit 4: TRACK - 1 = access track register
    /// Bit 3: Unused
    /// Bit 2: SECTOR - 1 = sector register
    /// Bit 1: Unused
    /// Bit 0: READ - 1 = read register, 0 = write register
    /// 
    /// Register select (bits 4,0):
    ///   00: Register 0 (Status)
    ///   20: Register 1 (Track)
    ///   04: Register 2 (Sector)
    ///   06: Register 3 (Data)
    ///   02: Register 4 (Access Byte)
    ///   07: Register 5 (Config)
    /// </summary>
    public byte Configuration { get; private set; }
    
    // Internal FDC state
    bool _busy;
    bool _intrq;
    bool _dmareq;
    bool _motorOn;
    bool _track0;
    bool _writeProtect;
    bool _indexPulse;
    
    // FDC command state machine
    FdcCommand _currentCommand;
    FdcState _currentState;
    
    // Track/sector state
    byte _currentDrive;
    byte _currentSide;
    
    // Data buffer for sector operations
    byte[] _sectorBuffer = new byte[512];
    int _sectorPosition;
    int _sectorSize;
    
    // Drive state
    bool[] _driveReady = new bool[2];
    byte[] _driveTrack = new byte[2];
    
    public enum FdcCommand
    {
        None,
        Restore,
        Seek,
        Step,
        StepIn,
        StepOut,
        ReadSector,
        WriteSector,
        ReadAddress,
        ReadTrack,
        WriteTrack
    }
    
    public enum FdcState
    {
        Idle,
        Executing,
        Transferring,
        Complete,
        Error
    }
    
    public Wd1772Fdc()
    {
        Reset();
    }
    
    public void Reset()
    {
        Status = 0x00;
        Track = 0x00;
        Sector = 0x00;
        Data = 0x00;
        AccessByte = 0x00;
        Configuration = 0x00;
        _busy = false;
        _intrq = false;
        _dmareq = false;
        _motorOn = false;
        _track0 = true;
        _writeProtect = false;
        _indexPulse = false;
        _currentCommand = FdcCommand.None;
        _currentState = FdcState.Idle;
        _currentDrive = 0;
        _currentSide = 0;
        _sectorPosition = 0;
        _sectorSize = 512;
        
        _driveReady[0] = true;
        _driveReady[1] = true;
        _driveTrack[0] = 0;
        _driveTrack[1] = 0;
    }
    
    /// <summary>
    /// Read from FDC register space
    /// </summary>
    public byte Read(ushort address)
    {
        byte offset = (byte)(address & 0xFF);
        
        // Config register used to select which register to read
        if (offset == 0x06)
        {
            return Configuration;
        }
        
        // Read based on configuration register
        byte regSelect = (byte)((Configuration >> 4) & 0x0F) | (Configuration & 0x01);
        
        switch (regSelect)
        {
            case 0x00: return Status;
            case 0x20: return Track;
            case 0x04: return Sector;
            case 0x06: return Data;
            case 0x02: return AccessByte;
            default: return 0xFF;
        }
    }
    
    /// <summary>
    /// Write to FDC register space
    /// </summary>
    public void Write(ushort address, byte value)
    {
        byte offset = (byte)(address & 0xFF);
        
        // Write to config register
        if (offset == 0x06)
        {
            Configuration = value;
            
            // Check if this is a write operation to another register
            if ((value & 0x01) == 0)
            {
                // Write to selected register
                byte regSelect = (byte)((value >> 4) & 0x0F);
                switch (regSelect)
                {
                    case 0x00: Status = value; break;
                    case 0x20: Track = value; _driveTrack[_currentDrive] = value; break;
                    case 0x04: Sector = value; break;
                    case 0x06: Data = value; break;
                    case 0x02: WriteCommand(value); break;
                    case 0x07: Configuration = value; break;
                }
            }
            return;
        }
        
        // Direct register access
        switch (offset)
        {
            case 0x01: Status = value; break;
            case 0x05: Track = value; _driveTrack[_currentDrive] = value; break;
            case 0x03: Sector = value; break;
            case 0x07: Data = value; break;
            case 0x04: WriteCommand(value); break;
        }
    }
    
    /// <summary>
    /// Execute FDC command
    /// </summary>
    void WriteCommand(byte command)
    {
        _busy = true;
        _currentState = FdcState.Executing;
        AccessByte = command;
        
        byte cmdBase = (byte)(command & 0xF0);
        
        switch (cmdBase)
        {
            case 0x00:
                _currentCommand = FdcCommand.Restore;
                Track = 0;
                _driveTrack[_currentDrive] = 0;
                _track0 = true;
                break;
            case 0x10:
                _currentCommand = FdcCommand.Seek;
                break;
            case 0x20:
                _currentCommand = FdcCommand.Step;
                break;
            case 0x40:
                _currentCommand = FdcCommand.StepIn;
                if (Track > 0) Track--;
                _driveTrack[_currentDrive] = Track;
                break;
            case 0x60:
                _currentCommand = FdcCommand.StepOut;
                Track++;
                _driveTrack[_currentDrive] = Track;
                break;
            case 0x80:
                _currentCommand = FdcCommand.ReadSector;
                _currentState = FdcState.Transferring;
                _sectorPosition = 0;
                break;
            case 0xA0:
                _currentCommand = FdcCommand.WriteSector;
                if (_writeProtect)
                {
                    _currentState = FdcState.Error;
                    CompleteCommand();
                    return;
                }
                _currentState = FdcState.Transferring;
                _sectorPosition = 0;
                break;
            case 0xC0:
                _currentCommand = FdcCommand.ReadAddress;
                break;
            case 0xE0:
                _currentCommand = FdcCommand.ReadTrack;
                break;
            case 0xF0:
                _currentCommand = FdcCommand.WriteTrack;
                break;
        }
        
        UpdateStatus();
    }
    
    /// <summary>
    /// Read sector data from FDC
    /// </summary>
    public byte ReadSectorData()
    {
        if (_currentCommand != FdcCommand.ReadSector || _sectorPosition >= _sectorSize)
            return 0xFF;
        
        byte data = _sectorBuffer[_sectorPosition];
        _sectorPosition++;
        
        if (_sectorPosition >= _sectorSize)
        {
            CompleteCommand();
        }
        
        return data;
    }
    
    /// <summary>
    /// Write sector data to FDC
    /// </summary>
    public void WriteSectorData(byte value)
    {
        if (_currentCommand != FdcCommand.WriteSector || _sectorPosition >= _sectorSize)
            return;
        
        _sectorBuffer[_sectorPosition] = value;
        _sectorPosition++;
        
        if (_sectorPosition >= _sectorSize)
        {
            CompleteCommand();
        }
    }
    
    /// <summary>
    /// Set drive state
    /// </summary>
    public void SetDriveState(int drive, bool ready, bool track0, bool writeProtect)
    {
        if (drive < 0 || drive > 1) return;
        _driveReady[drive] = ready;
        _track0 = track0;
        _writeProtect = writeProtect;
        UpdateStatus();
    }
    
    /// <summary>
    /// Simulate DMA request
    /// </summary>
    public void DmaRequest()
    {
        _dmareq = true;
        UpdateStatus();
    }
    
    /// <summary>
    /// Clear DMA request
    /// </summary>
    public void ClearDmaRequest()
    {
        _dmareq = false;
        UpdateStatus();
    }
    
    /// <summary>
    /// Set motor state
    /// </summary>
    public void SetMotor(bool on)
    {
        _motorOn = on;
        UpdateStatus();
    }
    
    /// <summary>
    /// Set index pulse state
    /// </summary>
    public void SetIndexPulse(bool active)
    {
        _indexPulse = active;
        UpdateStatus();
    }
    
    /// <summary>
    /// Complete current command and trigger interrupt
    /// </summary>
    void CompleteCommand()
    {
        _busy = false;
        _currentState = FdcState.Complete;
        _intrq = true;
        UpdateStatus();
    }
    
    /// <summary>
    /// Clear interrupt
    /// </summary>
    public void ClearInterrupt()
    {
        _intrq = false;
        _currentState = FdcState.Idle;
        _currentCommand = FdcCommand.None;
        UpdateStatus();
    }
    
    /// <summary>
    /// Update status register based on internal state
    /// </summary>
    void UpdateStatus()
    {
        Status = 0x00;
        if (_driveReady[_currentDrive]) Status |= 0x01;
        if (_sectorPosition < _sectorSize) Status |= 0x02;
        if (_dmareq) Status |= 0x04;
        if (_indexPulse) Status |= 0x08;
        if (_motorOn) Status |= 0x10;
        if (!_intrq) Status |= 0x20;
        if (!_busy) Status |= 0x80;
    }
    
    /// <summary>
    /// Load sector data for read operations (from image file)
    /// </summary>
    public void LoadSectorData(byte[] data, int size = 512)
    {
        _sectorSize = size;
        Array.Copy(data, _sectorBuffer, Math.Min(data.Length, _sectorSize));
    }
    
    /// <summary>
    /// Get sector data after write operations
    /// </summary>
    public byte[] GetSectorData()
    {
        return _sectorBuffer;
    }
}
