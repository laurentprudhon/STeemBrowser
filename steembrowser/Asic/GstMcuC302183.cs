using System;

/// <summary>
/// GST MCU C302183 - System-on-chip that consolidates GLUE + MMU + DMA + Blitter
/// Used in: Atari STe, Mega STe
/// Package: PLCC144
/// 
/// Memory-mapped registers at $FEC000-$FEFFFF
/// Combined DMA/Blitter registers at $FF8A00-$FF8AFF
/// MMU scroll registers at $FFE000-$FFE00F
/// </summary>
public class GstMcuC302183
{
    // === GLUE Block ===
    // Clock generation, DRAM refresh, address decode, bus control
    
    /// <summary>
    /// GST MCU Control Register ($FEC000-$FEC001)
    /// Bit 7: System reset hold
    /// Other bits: DRAM bank select, Super Hi-Res, IRQ control
    /// </summary>
    public ushort GstmCtrl { get; private set; }
    
    /// <summary>
    /// GST MCU Status Register ($FEC002-$FEC003)
    /// Contains interrupt flags and status
    /// </summary>
    public ushort GstmStatus { get; private set; }
    
    /// <summary>
    /// Color Mode / Display Selection ($FEC004-$FEC005)
    /// </summary>
    public ushort ColorMode { get; set; }
    
    /// <summary>
    /// Port A Direction / Data
    /// </summary>
    public byte PortADir { get; set; }
    public byte PortAData { get; set; }
    
    /// <summary>
    /// Joystick/Paddle 0 ($FEC014-$FEC015) - 5-bit ADC x4 paddle inputs
    /// </summary>
    public ushort Joystick0 { get; private set; }
    
    /// <summary>
    /// Joystick/Paddle 1 ($FEC016-$FEC017)
    /// </summary>
    public ushort Joystick1 { get; private set; }
    
    /// <summary>
    /// STe CRT Control ($FEC020-$FEC02B)
    /// Super Hi-Res, pixel doubling, color adjustment
    /// </summary>
    public ushort CrtControl { get; set; }
    
    // === MMU Block ===
    // 8 x 12-bit scroll registers at $FFE000-$FFE00F
    private ushort[] _scrollRegisters = new ushort[8];
    
    // Page mapping table: 32 entries x 16 bits
    private ushort[] _pageMapTable = new ushort[32];
    
    // DRAM refresh counter (10-bit, 0-1023)
    private ushort _refreshCounter;
    
    // === DMA Block ===
    // DMA registers at $FF8A00-$FF8AFF
    
    /// <summary>
    /// DMA Control Register ($FF8A00-$FF8A01)
    /// Bit 0: EN (Enable DMA)
    /// Bit 1: DIR (Device-to-memory / Memory-to-device)
    /// Bit 2: AUT (Auto-initiate)
    /// </summary>
    public ushort DmaCtrl { get; private set; }
    
    /// <summary>
    /// DMA Status ($FF8A02)
    /// Bit 7: RDY, Bit 5: EOP, Bit 4: REQ, Bit 3: DOE
    /// </summary>
    public byte DmaStatus { get; private set; }
    
    /// <summary>
    /// DMA Address Register ($FF8A04-$FF8A07)
    /// </summary>
    public uint DmaAddress { get; private set; }
    
    /// <summary>
    /// DMA Count Register ($FF8A08-$FF8A0B)
    /// </summary>
    public uint DmaCount { get; private set; }
    
    // === Blitter Engine ===
    // Registers at $FF8A00-$FF8A3D (shared base with DMA)
    
    private ushort[] _halftoneRam = new ushort[8];
    private int _srcXInc, _srcYInc, _dstXInc, _dstYInc;
    private uint _srcAddress, _dstAddress;
    private ushort _xCount, _yCount, _endMask1, _endMask2, _endMask3;
    private byte _hop, _op, _lineNumCtrl, _skewCtrl;
    private bool _busy, _hogMode;
    
    public bool BlitterBusy => _busy;
    public bool HogMode => _hogMode;
    
    public GstMcuC302183()
    {
        Reset();
    }
    
    public void Reset()
    {
        GstmCtrl = 0x0000;
        GstmStatus = 0x0000;
        ColorMode = 0x0000;
        CrtControl = 0x0000;
        DmaCtrl = 0x0000;
        DmaStatus = 0x0000;
        DmaAddress = 0;
        DmaCount = 0;
        _refreshCounter = 0;
        _busy = false;
        _hogMode = false;
        
        // Identity page mapping
        for (int i = 0; i < 32; i++)
            _pageMapTable[i] = (ushort)i;
        
        // Clear scroll registers
        Array.Clear(_scrollRegisters, 0, 8);
        Array.Clear(_halftoneRam, 0, 8);
    }
    
    /// <summary>
    /// Read from GST MCU register space
    /// </summary>
    public byte ReadByte(ushort address)
    {
        // GST MCU control space $FEC000-$FEFFFF
        if (address >= 0xEC00 && address < 0xF000)
        {
            ushort offset = (ushort)(address & 0x1FFF);
            switch (offset)
            {
                case 0x0000: return (byte)(GstmCtrl & 0xFF);
                case 0x0001: return (byte)((GstmCtrl >> 8) & 0xFF);
                case 0x0002: return (byte)(GstmStatus & 0xFF);
                case 0x0003: return (byte)((GstmStatus >> 8) & 0xFF);
                case 0x0004: return (byte)(ColorMode & 0xFF);
                case 0x0005: return (byte)((ColorMode >> 8) & 0xFF);
                case 0x0006: return PortADir;
                case 0x0007: return PortAData;
                case 0x0014: return (byte)(Joystick0 & 0xFF);
                case 0x0015: return (byte)((Joystick0 >> 8) & 0xFF);
                case 0x0016: return (byte)(Joystick1 & 0xFF);
                case 0x0017: return (byte)((Joystick1 >> 8) & 0xFF);
                case 0x0020: return (byte)(CrtControl & 0xFF);
                case 0x0021: return (byte)((CrtControl >> 8) & 0xFF);
            }
        }
        
        // DMA/Blitter space $FF8A00-$FF8A3F
        if (address >= 0x8A00 && address < 0x8A40)
        {
            ushort offset = (ushort)(address & 0x3F);
            switch (offset)
            {
                case 0x00: return (byte)(DmaCtrl & 0xFF);
                case 0x01: return (byte)((DmaCtrl >> 8) & 0xFF);
                case 0x02: return DmaStatus;
                case 0x04: return (byte)(DmaAddress & 0xFF);
                case 0x05: return (byte)((DmaAddress >> 8) & 0xFF);
                case 0x06: return (byte)((DmaAddress >> 16) & 0xFF);
                case 0x07: return (byte)((DmaAddress >> 24) & 0xFF);
                case 0x08: return (byte)(DmaCount & 0xFF);
                case 0x09: return (byte)((DmaCount >> 8) & 0xFF);
                case 0x3C: return (byte)(_lineNumCtrl | (_busy ? 0x80 : 0x00));
            }
        }
        
        // MMU scroll registers $FFE000-$FFE00F
        if (address >= 0xE000 && address < 0xE010)
        {
            int idx = (address & 0x0F) / 2;
            if (idx < 8)
            {
                if ((address & 1) == 0)
                    return (byte)(_scrollRegisters[idx] & 0xFF);
                else
                    return (byte)((_scrollRegisters[idx] >> 8) & 0xFF);
            }
        }
        
        return 0xFF; // Unmapped reads return 0xFF
    }
    
    /// <summary>
    /// Write to GST MCU register space
    /// </summary>
    public void WriteByte(ushort address, byte value)
    {
        if (address >= 0xEC00 && address < 0xF000)
        {
            ushort offset = (ushort)(address & 0x1FFF);
            switch (offset)
            {
                case 0x0000: GstmCtrl = (ushort)((GstmCtrl & 0xFF00) | value); break;
                case 0x0001: GstmCtrl = (ushort)((GstmCtrl & 0x00FF) | (value << 8)); break;
                case 0x0004: ColorMode = (ushort)((ColorMode & 0xFF00) | value); break;
                case 0x0005: ColorMode = (ushort)((ColorMode & 0x00FF) | (value << 8)); break;
                case 0x0006: PortADir = value; break;
                case 0x0007: PortAData = value; break;
                case 0x0020: CrtControl = (ushort)((CrtControl & 0xFF00) | value); break;
                case 0x0021: CrtControl = (ushort)((CrtControl & 0x00FF) | (value << 8)); break;
            }
        }
        
        if (address >= 0x8A00 && address < 0x8A40)
        {
            ushort offset = (ushort)(address & 0x3F);
            switch (offset)
            {
                case 0x00: DmaCtrl = (ushort)((DmaCtrl & 0xFF00) | value); break;
                case 0x01: DmaCtrl = (ushort)((DmaCtrl & 0x00FF) | (value << 8));
                           if ((DmaCtrl & 1) != 0) StartDma(); break;
                case 0x04: DmaAddress = (uint)((DmaAddress & 0xFFFFFF00) | value); break;
                case 0x05: DmaAddress = (uint)((DmaAddress & 0xFFFF00FF) | (value << 8)); break;
                case 0x06: DmaAddress = (uint)((DmaAddress & 0xFF00FFFF) | (value << 16)); break;
                case 0x07: DmaAddress = (uint)((DmaAddress & 0x00FFFFFF) | (uint)(value << 24)); break;
                case 0x08: DmaCount = (uint)((DmaCount & 0xFFFFFF00) | value); break;
                case 0x09: DmaCount = (uint)((DmaCount & 0xFFFF00FF) | (value << 8)); break;
                case 0x3C: 
                    _lineNumCtrl = value;
                    _hogMode = (value & 0x40) != 0;
                    if ((value & 0x80) != 0) _busy = false;
                    break;
            }
        }
        
        if (address >= 0xE000 && address < 0xE010)
        {
            int idx = (address & 0x0F) / 2;
            if (idx < 8)
            {
                if ((address & 1) == 0)
                    _scrollRegisters[idx] = (ushort)((_scrollRegisters[idx] & 0xFF00) | value);
                else
                    _scrollRegisters[idx] = (ushort)((_scrollRegisters[idx] & 0x00FF) | (value << 8));
            }
        }
    }
    
    /// <summary>
    /// Map logical address to physical DRAM address
    /// </summary>
    public uint MapAddress(uint logicalAddress)
    {
        // Get page number (32KB pages = bits 15-17)
        int pageNum = (int)((logicalAddress >> 15) & 0x1F);
        ushort physicalPage = _pageMapTable[pageNum];
        
        // Combine physical page with offset
        uint physicalAddress = (uint)(physicalPage << 15) | (logicalAddress & 0x7FFF);
        return physicalAddress;
    }
    
    /// <summary>
    /// DRAM refresh cycle - autonomous from CPU
    /// </summary>
    public void RefreshCycle()
    {
        _refreshCounter++;
        if (_refreshCounter >= 1024) _refreshCounter = 0;
    }
    
    /// <summary>
    /// Start DMA transfer
    /// </summary>
    void StartDma()
    {
        DmaStatus |= 0x10; // Set REQ
    }
    
    /// <summary>
    /// Execute one DMA cycle
    /// </summary>
    public void DmaCycle(byte[] memory, bool deviceToMemory)
    {
        if ((DmaCtrl & 1) == 0) return; // DMA not enabled
        if (DmaCount == 0) return;
        
        if (deviceToMemory)
        {
            // Device to memory transfer
            byte data = ReadDeviceByte(); 
            memory[DmaAddress] = data;
        }
        else
        {
            // Memory to device transfer
            byte data = memory[DmaAddress];
            WriteDeviceByte(data);
        }
        
        DmaAddress++;
        uint tempCount = DmaCount;
        if ((tempCount & 1) != 0)
        {
            DmaCount--;
            if (DmaCtrl & 4) // Auto-initiate
                DmaStatus &= ~0x10;
        }
        else
        {
            DmaCount--;
            DmaStatus |= 0x20; // End of page
        }
    }
    
    byte ReadDeviceByte() { return 0; } // Placeholder
    void WriteDeviceByte(byte data) { /* Placeholder */ }
}
