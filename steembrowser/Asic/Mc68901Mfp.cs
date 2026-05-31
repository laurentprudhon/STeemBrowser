using System;

/// <summary>
/// Motorola MC68901 Multi-Function Peripheral
/// DIP64 package, memory-mapped at $FF8000-$FF83FF
/// 
/// 4 timers (A,B,C,D), 8 GPI pins with interrupt, full-duplex USART
/// Timer C = system clock (~200 Hz), Timer D = RS232 baud rate
/// GPI 4 = keyboard/MIDI, GPI 5 = FDC, GPI 6 = RI
/// </summary>
public class Mc68901Mfp
{
    // Address/Bus
    public byte DataBus { get; private set; }
    public ushort AddressBus { get; private set; }
    
    // GPI registers
    public byte Gpio { get; private set; }
    public byte ActiveEdgeIntA { get; private set; }
    public byte DataDir { get; private set; }
    public byte IntEnableA { get; private set; }
    public byte IntEnableB { get; private set; }
    public byte IntPendingA { get; private set; }
    public byte IntPendingB { get; private set; }
    public byte IntInServiceA { get; private set; }
    public byte IntInServiceB { get; private set; }
    public byte MaskA { get; private set; }
    public byte MaskB { get; private set; }
    public byte VectorBase { get; private set; }
    
    // Timer control
    public byte TimerACtrl { get; private set; }
    public byte TimerBCtrl { get; private set; }
    public byte TimerCDctrl { get; private set; }
    public ushort TimerAData { get; private set; }
    public ushort TimerBData { get; private set; }
    public ushort TimerCData { get; private set; }
    public ushort TimerDData { get; private set; }
    
    // USART
    public ushort SyncChar { get; private set; }
    public byte UartCtrl { get; private set; }
    public byte RxStatus { get; private set; }
    public byte TxStatus { get; private set; }
    public byte RxData { get; private set; }
    public byte TxData { get; private set; }
    
    // Internal timer state
    ushort[] _timerCount = new ushort[4];
    bool[] _timerRunning = new bool[4];
    
    // Interrupt output - 68000 IPL level
    public byte Ipl { get; private set; }
    
    public Mc68901Mfp()
    {
        Reset();
    }
    
    public void Reset()
    {
        Gpio = 0x00;
        ActiveEdgeIntA = 0x00;
        DataDir = 0xff; // Default all GPIs as input
        IntEnableA = 0x00;
        IntEnableB = 0x00;
        IntPendingA = 0x00;
        IntPendingB = 0x00;
        IntInServiceA = 0x00;
        IntInServiceB = 0x00;
        MaskA = 0x00;
        MaskB = 0x00;
        VectorBase = 0x00;
        TimerACtrl = 0x00;
        TimerBCtrl = 0x00;
        TimerCDctrl = 0x00;
        TimerAData = 0x0000;
        TimerBData = 0x0000;
        TimerCData = 0x0000;
        TimerDData = 0x0000;
        SyncChar = 0xFF;
        UartCtrl = 0x00;
        RxStatus = 0x00;
        TxStatus = 0xFF;
        RxData = 0x00;
        TxData = 0x00;
        Ipl = 0x00;
        
        for (int i = 0; i < 4; i++)
        {
            _timerCount[i] = 0;
            _timerRunning[i] = false;
        }
    }
    
    public byte Read(ushort address)
    {
        byte offset = (byte)(address & 0xFF);
        
        switch (offset)
        {
            case 0x01: return Gpio;
            case 0x03: return ActiveEdgeIntA;
            case 0x05: return DataDir;
            case 0x07: return IntEnableA;
            case 0x09: return IntEnableB;
            case 0x0B: return IntPendingA;
            case 0x0D: return IntPendingB;
            case 0x0F: return IntInServiceA;
            case 0x11: return IntInServiceB;
            case 0x13: return MaskA;
            case 0x15: return MaskB;
            case 0x17: return VectorBase;
            case 0x19: return TimerACtrl;
            case 0x1B: return TimerBCtrl;
            case 0x1D: return TimerCDctrl;
            case 0x1F: return (byte)(TimerAData & 0xFF);
            case 0x20: return (byte)((TimerAData >> 8) & 0xFF);
            case 0x21: return (byte)(TimerBData & 0xFF);
            case 0x22: return (byte)((TimerBData >> 8) & 0xFF);
            case 0x23: return (byte)(TimerCData & 0xFF);
            case 0x24: return (byte)((TimerCData >> 8) & 0xFF);
            case 0x25: return (byte)(TimerDData & 0xFF);
            case 0x26: return (byte)((TimerDData >> 8) & 0xFF);
            case 0x27: return (byte)(SyncChar & 0xFF);
            case 0x28: return (byte)((SyncChar >> 8) & 0xFF);
            case 0x29: return UartCtrl;
            case 0x2B: return RxStatus;
            case 0x2D: return TxStatus;
            case 0x2F: return RxData;
            default: return 0xFF;
        }
    }
    
    public void Write(ushort address, byte value)
    {
        byte offset = (byte)(address & 0xFF);
        
        switch (offset)
        {
            case 0x01: Gpio = value; break;
            case 0x03: ActiveEdgeIntA = value; break;
            case 0x05: DataDir = value; break;
            case 0x07: IntEnableA = value; break;
            case 0x09: IntEnableB = value; break;
            case 0x13: MaskA = value; break;
            case 0x15: MaskB = value; break;
            case 0x17: VectorBase = value; break;
            case 0x19: 
                TimerACtrl = value;
                _timerRunning[0] = (value & 0x02) != 0;
                break;
            case 0x1B:
                TimerBCtrl = value;
                _timerRunning[1] = (value & 0x02) != 0;
                break;
            case 0x1D:
                TimerCDctrl = value;
                _timerRunning[2] = ((value & 0x02) != 0);
                _timerRunning[3] = ((value & 0x20) != 0);
                break;
            case 0x1F: TimerAData = (ushort)((TimerAData & 0xFF00) | value); break;
            case 0x20: TimerAData = (ushort)((TimerAData & 0x00FF) | (value << 8)); break;
            case 0x21: TimerBData = (ushort)((TimerBData & 0xFF00) | value); break;
            case 0x22: TimerBData = (ushort)((TimerBData & 0x00FF) | (value << 8)); break;
            case 0x23: TimerCData = (ushort)((TimerCData & 0xFF00) | value); break;
            case 0x24: TimerCData = (ushort)((TimerCData & 0x00FF) | (value << 8)); break;
            case 0x25: TimerDData = (ushort)((TimerDData & 0xFF00) | value); break;
            case 0x26: TimerDData = (ushort)((TimerDData & 0x00FF) | (value << 8)); break;
            case 0x27: SyncChar = (ushort)((SyncChar & 0xFF00) | value); break;
            case 0x28: SyncChar = (ushort)((SyncChar & 0x00FF) | (value << 8)); break;
            case 0x29: UartCtrl = value; break;
            case 0x2F: TxData = value; break;
        }
    }
    
    /// <summary>
    /// Timer tick - called at system clock (8 MHz PAL / 7.16 MHz NTSC)
    /// </summary>
    public void Tick()
    {
        for (int i = 0; i < 4; i++)
        {
            if (_timerRunning[i])
            {
                if (_timerCount[i] > 0)
                    _timerCount[i]--;
                else
                {
                    _timerRunning[i] = false;
                    OnTimerInterrupt(i);
                }
            }
        }
    }
    
    void OnTimerInterrupt(int index)
    {
        TimerAData = _timerRunning[0] ? _timerCount[0] : 0;
        // Timer interrupt routing depends on control bits
        // Timer C (sys timer) -> IPL 6, Timer D -> baud, Timer B -> HBLANK
        
        switch (index)
        {
            case 0: IntPendingA |= 0x80; break;
            case 1: IntPendingA |= 0x40; break;
            case 2: IntPendingA |= 0x20; break;
            case 3: IntPendingB |= 0x80; break;
        }
        
        RecalcIpl();
    }
    
    void RecalcIpl()
    {
        byte highest = 0;
        
        // GPI 7 = FDC -> IPL 8
        if ((IntEnableA & 0x20) != 0 && (IntPendingA & 0x20) != 0)
            highest = Math.Max(highest, 8);
        
        // GPI 4 = keyboard -> IPL 6
        if ((IntEnableA & 0x10) != 0 && (IntPendingA & 0x10) != 0)
            highest = Math.Max(highest, 8);
        
        // GPI 6 = RI -> IPL 6
        if ((IntEnableA & 0x40) != 0 && (IntPendingA & 0x40) != 0)
            highest = Math.Max(highest, 6);
        
        Ipl = highest;
    }
    
    /// <summary>
    /// Baud rate generator formula from Timer D
    /// Baud = 8MHz / (16 * TimerDValue)
    /// </summary>
    public int GetBaudRate()
    {
        if (TimerDData == 0) return 38400;
        return 8_000_000 / (16 * TimerDData);
    }
    
    /// <summary>
    /// Set GPI pin state (e.g., keyboard pressed, FDC IRQ)
    /// </summary>
    public void SetGpiPin(int pin, bool value)
    {
        if (pin < 0 || pin >= 8) return;
        
        if (value)
            Gpio |= (byte)(1 << pin);
        else
            Gpio &= (byte)~(1 << pin);
        
        // Check if this triggers an interrupt
        if ((ActiveEdgeIntA & (byte)(1 << pin)) != 0)
            IntPendingA |= (byte)(1 << pin);
            
        RecalcIpl();
    }
    
    /// <summary>
    /// Push data into USART RX buffer
    /// </summary>
    public void UsartReceive(byte data)
    {
        RxData = data;
        RxStatus |= 0x10; // Data ready
    }
    
    /// <summary>
    /// Clear interrupt service flag
    /// </summary>
    public void ClearInterrupt(int vector)
    {
        switch (vector)
        {
            case 0: IntPendingA = 0; break;
            case 1: IntPendingB = 0; break;
            case 2: IntInServiceA = 0; break;
            case 3: IntInServiceB = 0; break;
        }
        RecalcIpl();
    }
}
