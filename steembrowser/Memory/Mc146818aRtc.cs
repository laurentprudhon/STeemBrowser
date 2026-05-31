using System;

/// <summary>
/// Motorola MC146818A Real-Time Clock Plus RAM
/// DIP24 package, accessed via MFP I/O port
/// 
/// 114 bytes of non-volatile SRAM
/// 32.768 kHz external crystal oscillator
/// Programmable alarm interrupt
/// 
/// Pinout (DIP-24):
///   AD0-AD7 (1-8): 8-bit multiplexed address/data bus
///   A0-A2 (9-11): Address register select lines
///   IOW (12): Input/Output Write (active high)
///   IOCR (13): I/O Control Read (active high)
///   IE (14): Interrupt Enable (active high)
///   AEN (15): Address Enable (active high)
///   CS (16): Chip Select (active low)
///   DRQ (17): DMA Request (active low)
///   DS (18): Data Strobe (active high)
///   Vcc (19-20): +5V supply / Ground
///   X1 (23): 32.768 kHz crystal input
///   FS (22): Frequency Select (connect to Vcc for 32.768 kHz)
///   X2 (21): 32.768 kHz crystal input
///   VRT (24): Voltage Reset (connect to Vcc)
/// </summary>
public class Mc146818aRtc
{
    /// <summary>
    /// Register A ($00) - Square Wave/Rate Select
    /// 
    /// Bit 0: SQWE - Square wave enable
    /// Bits 1-3: RS[2:0] - Rate select (divider frequency)
    /// Bits 4-7: Unused
    /// </summary>
    public byte RegisterA { get; private set; }
    
    /// <summary>
    /// Register B ($01) - Interrupt Enable/Control
    /// 
    /// Bit 0: UIE - Update interrupt enable
    /// Bit 1: AIE - Alarm interrupt enable
    /// Bit 2: PIE - Periodic interrupt enable
    /// Bit 3: UIUP - Update in progress (read-only)
    /// Bit 4: DM - Data mode (0 = BCD, 1 = binary)
    /// Bit 5: DPE - Periodic/interrupt rate enable
    /// Bit 6: UPS - Update period select
    /// Bit 7: SET - Set mode (0 = run, 1 = stop oscillator during writes)
    /// </summary>
    public byte RegisterB { get; private set; }
    
    /// <summary>
    /// Register C ($02) - Interrupt Flags (read-only, read clears)
    /// 
    /// Bit 0: UF - Update finished flag
    /// Bit 1: AF - Alarm flag
    /// Bit 2: PF - Periodic interrupt flag
    /// Bits 3-7: Unused
    /// </summary>
    public byte RegisterC { get; private set; }
    
    /// <summary>
    /// Register D ($03) - Voltage Reset Flag
    /// 
    /// Bit 7: VRF - Voltage reset flag (read-only)
    /// Bits 0-6: Unused
    /// </summary>
    public byte RegisterD { get; private set; }
    
    /// <summary>
    /// Seconds ($04) - 00-59 BCD (or 0-59 binary)
    /// </summary>
    public byte Seconds { get; private set; }
    
    /// <summary>
    /// Minutes ($05) - 00-59 BCD (or 0-59 binary)
    /// </summary>
    public byte Minutes { get; private set; }
    
    /// <summary>
    /// Hours ($06) - 00-23 BCD (or 0-23 binary)
    /// </summary>
    public byte Hours { get; private set; }
    
    /// <summary>
    /// Day of Week ($07) - 01-07 BCD (Sunday=1)
    /// </summary>
    public byte DayOfWeek { get; private set; }
    
    /// <summary>
    /// Day of Month ($08) - 01-31 BCD
    /// </summary>
    public byte DayOfMonth { get; private set; }
    
    /// <summary>
    /// Month ($09) - 01-12 BCD
    /// </summary>
    public byte Month { get; private set; }
    
    /// <summary>
    /// Year ($0A) - 00-99 BCD
    /// </summary>
    public byte Year { get; private set; }
    
    /// <summary>
    /// Century ($0B) - 19 or 20 (for 1900s or 2000s)
    /// </summary>
    public byte Century { get; private set; }
    
    // Alarm registers
    byte _alarmSeconds;
    byte _alarmMinutes;
    byte _alarmHours;
    byte _alarmDayOfMonth;
    byte _alarmMonth;
    
    // 114 bytes of NVRAM ($0C-$7F)
    byte[] _nvram = new byte[114];
    
    // Internal timing
    DateTime _currentTime;
    TimeSpan _elapsedTime;
    bool _voltageResetFlag;
    bool _updateInProgress;
    
    // Interrupt enable state
    bool _interruptEnabled;
    
    public Mc146818aRtc()
    {
        Reset();
    }
    
    public void Reset()
    {
        RegisterA = 0x00;
        RegisterB = 0x02;
        RegisterC = 0x00;
        RegisterD = 0x00;
        Seconds = 0x00;
        Minutes = 0x00;
        Hours = 0x00;
        DayOfWeek = 0x00;
        DayOfMonth = 0x00;
        Month = 0x00;
        Year = 0x00;
        Century = 0x00;
        _nvram = new byte[114];
        _currentTime = DateTime.UtcNow;
        _elapsedTime = TimeSpan.Zero;
        _voltageResetFlag = true;
        _updateInProgress = false;
        _interruptEnabled = false;
    }
    
    /// <summary>
    /// Read from RTC register
    /// First AEN, then address on A0-A2, then IOCR
    /// </summary>
    public byte Read(byte registerAddress)
    {
        switch (registerAddress)
        {
            case 0x00: return RegisterA;
            case 0x01: return RegisterB;
            case 0x02:
                // Reading register C clears the flags
                byte flags = RegisterC;
                RegisterC = 0x00;
                return flags;
            case 0x03: return RegisterD;
            case 0x04: return Seconds;
            case 0x05: return Minutes;
            case 0x06: return Hours;
            case 0x07: return DayOfWeek;
            case 0x08: return DayOfMonth;
            case 0x09: return Month;
            case 0x0A: return Year;
            case 0x0B: return Century;
            default:
                // Alarm registers and NVRAM
                if (registerAddress >= 0x0C && registerAddress < 0x7F)
                {
                    return _nvram[registerAddress - 0x0C];
                }
                return 0x00;
        }
    }
    
    /// <summary>
    /// Write to RTC register
    /// First AEN, then address on A0-A2, then data on AD0-AD7, then IOW
    /// </summary>
    public void Write(byte registerAddress, byte value)
    {
        switch (registerAddress)
        {
            case 0x00:
                RegisterA = value;
                break;
            case 0x01:
                RegisterB = value;
                break;
            case 0x04:
                Seconds = (byte)(value & 0x7F);
                break;
            case 0x05:
                Minutes = (byte)(value & 0x7F);
                break;
            case 0x06:
                Hours = (byte)(value & 0x3F);
                break;
            case 0x07:
                DayOfWeek = (byte)((value & 0x07) | ((DayOfWeek & 0xF0) == 0 ? 0x01 : (DayOfWeek & 0xF0)));
                break;
            case 0x08:
                DayOfMonth = (byte)(value & 0x3F);
                break;
            case 0x09:
                Month = (byte)(value & 0x1F);
                break;
            case 0x0A:
                Year = value;
                break;
            case 0x0B:
                Century = value;
                break;
            default:
                if (registerAddress >= 0x0C && registerAddress < 0x7F)
                {
                    _nvram[registerAddress - 0x0C] = value;
                }
                break;
        }
    }
    
    /// <summary>
    /// Read NVRAM byte
    /// </summary>
    public byte ReadNvram(int address)
    {
        if (address < 0 || address >= _nvram.Length)
            return 0x00;
        return _nvram[address];
    }
    
    /// <summary>
    /// Write NVRAM byte
    /// </summary>
    public void WriteNvram(int address, byte value)
    {
        if (address < 0 || address >= _nvram.Length)
            return;
        _nvram[address] = value;
    }
    
    /// <summary>
    /// BCD to binary conversion
    /// </summary>
    public static int BcdToBinary(byte bcd)
    {
        return (bcd & 0x0F) + ((bcd >> 4) * 10);
    }
    
    /// <summary>
    /// Binary to BCD conversion
    /// </summary>
    public static byte BinaryToBcd(int value)
    {
        return (byte)((value / 10) << 4 | (value % 10));
    }
    
    /// <summary>
    /// Get square wave output frequency from Register A
    /// </summary>
    public int GetSquareWaveFrequency()
    {
        if ((RegisterA & 0x01) == 0)
            return 0; // SQWE disabled
        
        switch ((RegisterA >> 1) & 0x07)
        {
            case 0x00: return 32768;
            case 0x01: return 4096;
            case 0x02: return 512;
            case 0x03: return 256;
            case 0x04: return 128;
            case 0x05: return 64;
            case 0x06: return 32;
            case 0x07: return 16;
            default: return 32768;
        }
    }
    
    /// <summary>
    /// Update RTC with current time
    /// </summary>
    public void UpdateFromSystemTime()
    {
        DateTime now = DateTime.UtcNow;
        
        // Use BCD format by default (RegisterB bit 4 = DM, 0 = BCD)
        bool useBinary = (RegisterB & 0x10) != 0;
        
        if (useBinary)
        {
            Seconds = (byte)now.Second;
            Minutes = (byte)now.Minute;
            Hours = (byte)now.Hour;
            DayOfWeek = (byte)((int)now.DayOfWeek == 0 ? 7 : (int)now.DayOfWeek);
            DayOfMonth = (byte)now.Day;
            Month = (byte)now.Month;
            Year = (byte)(now.Year % 100);
        }
        else
        {
            // BCD encoding
            Seconds = BinaryToBcd(now.Second);
            Minutes = BinaryToBcd(now.Minute);
            Hours = BinaryToBcd(now.Hour);
            DayOfWeek = (byte)((int)now.DayOfWeek == 0 ? 7 : (int)now.DayOfWeek);
            DayOfMonth = BinaryToBcd(now.Day);
            Month = BinaryToBcd(now.Month);
            Year = BinaryToBcd(now.Year % 100);
        }
        
        Century = (byte)(now.Year < 2000 ? 0x19 : 0x20);
    }
    
    /// <summary>
    /// Check if alarm condition is met
    /// </summary>
    public bool CheckAlarm()
    {
        if ((RegisterB & 0x02) == 0)
            return false; // Alarm not enabled
        
        // Compare current time against alarm values
        // Note: Alarm registers can have special values for match-any-day-of-month
        if (Seconds == _alarmSeconds && (Minutes == _alarmMinutes || _alarmMinutes == 0xFF)
            && (Hours == _alarmHours || _alarmHours == 0xFF)
            && (DayOfMonth == _alarmDayOfMonth || _alarmDayOfMonth == 0xFF))
        {
            RegisterC |= 0x02; // Set alarm flag
            return true;
        }
        return false;
    }
    
    /// <summary>
    /// Check for periodic interrupt
    /// </summary>
    public bool CheckPeriodicInterrupt()
    {
        if ((RegisterB & 0x04) == 0)
            return false;
        
        // Periodic interrupt occurs at rate determined by Register A RS bits
        // For simplicity, this would be called at the right interval based on RS
        _elapsedTime += TimeSpan.FromSeconds(1.0 / GetSquareWaveFrequency());
        return false;
    }
}
