using System;

/// <summary>
/// Atari 414616 (HM6216/KM4164) 64K×1 DRAM Chip
/// 16-pin DIP, 256 rows × 256 columns = 64 Kbit
/// 
/// Used in: STe Bank 0-1/4-6, MegaSTE
/// 
/// Pinout (16-pin DIP):
///   NC (1): No connect
///   A6 (2): Row address bit 6
///   A5 (3): Row address bit 5
///   A3 (4): Row address bit 3
///   A10 (5): Row address bit 10
///   A0 (6): Row/Col multiplexed address
///   A1 (7): Row/Col multiplexed address
///   A9/AFC (8): Row address A9 / Auto Frequency Change
///   A5 (9): Col multiplexed address
///   A3 (10): Col multiplexed address
///   A0 (11): Multiplexed address
///   A1 (12): Multiplexed address
///   DQ (13): Data input/output (bit 0)
///   DQ (14): Data input/output (bit 0)
///   A4 (15): Row/Col multiplexed
///   VCC (16): +5V supply
/// 
/// Speed grades: -15 (150ns), -12 (120ns), -10 (100ns), -8 (80ns)
/// 
/// DRAM access cycle:
///   1. RAS LOW (Row Address Strobe) -> Row address latched
///   2. CAS LOW (Column Address Strobe) -> Column address latched
///   3. Data valid on DQ after tAC delay (150ns typical)
/// 
/// The STe uses banked 41416 chips in combinations:
///   Bank 0: 8 × 41416 → 64 KB
///   Bank 1: 8 × 41416 → 64 KB
///   Bank 4: 8 × 41416 → 64 KB
///   Bank 5: 8 × 41416 → 64 KB
/// 
/// DRAM refresh required every 15.625 µs, 256 rows
/// </summary>
public class K41416Dram
{
    /// <summary>
    /// Total number of bits in this chip
    /// </summary>
    const int TotalBits = 64 * 1024; // 64 Kbit
    
    private bool[] _memory = new bool[TotalBits];
    private int _rowAddress;
    private int _columnAddress;
    private bool _rowOpen;
    private bool _columnOpen;
    private bool _rasActive;
    private bool _casActive;
    private bool _dataOut;
    private AccessTime _accessTimeGrade;
    
    /// <summary>
    /// Data output pin - latches after tAC delay from CAS LOW
    /// </summary>
    public bool DataOut => _dataOut;
    
    /// <summary>
    /// Row address width
    /// </summary>
    public int RowAddressWidth => 9;
    
    /// <summary>
    /// Column address width
    /// </summary>
    public int ColumnAddressWidth => 9;
    
    /// <summary>
    /// Access time grade (default: 150ns for -15 grade)
    /// </summary>
    public AccessTime AccessTime => _accessTimeGrade;
    
    /// <summary>
    /// Memory capacity in bits
    /// </summary>
    public int BitCount { get; } = TotalBits;
    
    public enum AccessTime
    {
        Grade150ns = 150,
        Grade120ns = 120,
        Grade100ns = 100,
        Grade80ns = 80
    }
    
    public K41416Dram(AccessTime timeGrade = AccessTime.Grade150ns)
    {
        _accessTimeGrade = timeGrade;
        _rowAddress = 0;
        _columnAddress = 0;
        _rowOpen = false;
        _columnOpen = false;
        _rasActive = false;
        _casActive = false;
    }
    
    /// <summary>
    /// Set RAS line (address phase: row)
    /// During RAS LOW, row address pins (6,7,11,12,8,9,10,15,2,3,4,5)
    /// are latched into row address register
    /// </summary>
    public void SetRas(bool activeLow)
    {
        _rasActive = activeLow;
        
        if (activeLow)
        {
            // RAS LOW - latch row address
            // Row address comes from pins: 6(A0),7(A1),11(A2),12(A3),
            // 8(A4-A7,AFC),9(A5),10(A6),15(A7),2(A6),3(A5)
            // For simplicity, use combined row address
            _rowOpen = true;
        }
        else
        {
            // RAS HIGH - precharge
            _rowOpen = false;
            _columnOpen = false;
        }
    }
    
    /// <summary>
    /// Set CAS line (data phase: column)
    /// During CAS LOW, column address pins are latched
    /// Data becomes valid after tAC delay
    /// </summary>
    public void SetCas(bool activeLow)
    {
        _casActive = activeLow;
        
        if (activeLow && _rowOpen)
        {
            // CAS LOW - latch column address and output data
            // Column address: 6(A0),7(A1),11(A2),12(A3),
            // 8(AFC,A4-A7),9(A5),10(A6),15(A7)
            int address = (_rowAddress << 9) | _columnAddress;
            
            if (address < _memory.Length)
            {
                _dataOut = _memory[address];
            }
            _columnOpen = true;
        }
        else
        {
            _columnOpen = false;
        }
    }
    
    /// <summary>
    /// Write data to chip
    /// Write cycle: RAS LOW (row) -> CAS LOW -> data on DQ -> CAS HIGH
    /// </summary>
    public void WriteData(bool data)
    {
        if (!_rowOpen || !_casActive)
            return;
        
        // Calculate address from row + column
        int address = (_rowAddress << 9) | _columnAddress;
        
        if (address < _memory.Length)
            _memory[address] = data;
    }
    
    /// <summary>
    /// Set row address (9 bits for 41416: A0-A8)
    /// Latched when RAS goes LOW
    /// </summary>
    public void SetRowAddress(int address)
    {
        _rowAddress = address & 0x01FF; // 9 bits
    }
    
    /// <summary>
    /// Set column address (9 bits for 41416: A0-A8)
    /// Latched when CAS goes LOW
    /// </summary>
    public void SetColumnAddress(int address)
    {
        _columnAddress = address & 0x01FF; // 9 bits
    }
    
    /// <summary>
    /// Refresh a row (RAS-only refresh)
    /// During AINT signal, all rows are refreshed sequentially
    /// The ST BIU / GS02 generates refresh pulses for each bank
    /// </summary>
    public void RefreshRow(int row)
    {
        if (row < 0 || row >= 256)
            return;
        
        // RAS LOW, row address on pins, CAS HIGH
        // The row is refreshed without data output
        _rowAddress = row;
        _rowOpen = true;
        _rasActive = true;
        
        // After tRP (45ns max), RAS goes HIGH
        _rasActive = false;
        _rowOpen = false;
    }
    
    /// <summary>
    /// Perform DRAM refresh cycle
    /// The ST refreshes all 256 rows in a 15.625 µs period (NTSC)
    /// or 20.000 µs period (PAL)
    /// </summary>
    public void AutoRefresh(int rowCounter)
    {
        _rowAddress = rowCounter & 0x01FF;
        RefreshRow(_rowAddress);
        
        // Wrap around after all rows
        if ((rowCounter + 1) >= 256)
            _rowAddress = 0;
    }
    
    /// <summary>
    /// Write to memory at physical address
    /// </summary>
    public void WriteMemory(int address, bool bit)
    {
        if (address < 0 || address >= _memory.Length)
            return;
        _memory[address] = bit;
    }
    
    /// <summary>
    /// Read from memory at physical address
    /// </summary>
    public bool ReadMemory(int address)
    {
        if (address < 0 || address >= _memory.Length)
            return false;
        return _memory[address];
    }
    
    /// <summary>
    /// Get byte at address
    /// 8 chips provide 8 bits per byte
    /// </summary>
    public byte ReadByte(K41416Dram[] byteBus, int address)
    {
        byte result = 0;
        if (byteBus != null && byteBus.Length == 8)
        {
            foreach (var chip in byteBus)
            {
                if (chip.ReadMemory(address))
                    result <<= 1;
                else
                    result |= 1;
            }
        }
        return result;
    }
    
    /// <summary>
    /// Write byte to bus of 8 chips
    /// </summary>
    public void WriteByte(K41416Dram[] byteBus, int address, byte value)
    {
        if (byteBus != null && byteBus.Length == 8)
        {
            for (int i = 7; i >= 0; i--)
            {
                byteBus[i].WriteMemory(address, (value & (1 << i)) != 0);
            }
        }
    }
}
