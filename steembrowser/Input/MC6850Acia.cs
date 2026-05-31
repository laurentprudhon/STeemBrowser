using System;

/// <summary>
/// Motorola MC6850 Asynchronous Communications Interface Adapter
/// DIP40 package, memory-mapped at $FFFC00 (keyboard) and $FFFC04 (MIDI)
/// 
/// Provides serial communication for the IKBD keyboard link at 7.8125 Kbps
/// and for the MIDI interface at 31.25 Kbps.
/// 
/// Pinout (DIP-40):
///   DA0-DA7 (1-8): 8-bit bidirectional data bus
///   A1, A2 (12-13): Address select lines
///   CS2 (17): Chip select (active low)
///   RESET (18): Reset (active high)
///   RXD (22): Receive data input
///   TXD (23): Transmit data output
///   RTS (26): Request to Send output
///   CTS (28): Clear to Send input
///   DCD (30): Data Carrier Detect input
///   CLOCK (38): Clock input
/// </summary>
public class Mc6850Acia
{
    /// <summary>
    /// Control/Status Register (read returns status, write sets control)
    /// 
    /// Read bits:
    ///   Bit 0: DR - Data Ready (1 = data available to read)
    ///   Bit 1: TDF - Transmitter Data Full (1 = data in DR ready to transmit)
    ///   Bit 2: E - Framing Error
    ///   Bit 3: OE - Overrun Error
    ///   Bit 4: CTS - Clear to Send
    ///   Bit 5: DCD - Data Carrier Detect
    ///   Bit 6-7: Reserved
    /// 
    /// Write bits:
    ///   Bit 0: RIE - Receiver Interrupt Enable
    ///   Bit 1: TIE - Transmitter Interrupt Enable
    ///   Bit 2: PS0 - Parity Select bit 0
    ///   Bit 3: PS1 - Parity Select bit 1
    ///   Bit 4: PBS - Parity Bit Select
    ///   Bit 5: S0 - Stop Bits Select bit 0
    ///   Bit 6: S1 - Stop Bits Select bit 1
    ///   Bit 7: RST - Reset (write 1 to reset ACIA)
    /// </summary>
    public byte ControlStatus { get; private set; }
    
    /// <summary>
    /// Data Register - read returns received data, write sends data
    /// </summary>
    public byte Data { get; private set; }
    
    // Internal state
    byte _rxBuffer;
    bool _rxReady;
    bool _txFull;
    
    // Status flags
    bool _framingError;
    bool _overrunError;
    bool _cts;
    bool _dcd;
    
    // Control flags
    bool _rie;
    bool _tie;
    bool _parityEnabled;
    byte _paritySelect;
    byte _stopBits;
    
    // Baud rate
    int _baudRate;
    Action<byte> _onTransmit;
    Func<byte> _onReceive;
    
    public Mc6850Acia(int baudRate = 7812)
    {
        _baudRate = baudRate;
        Reset();
    }
    
    public void Reset()
    {
        ControlStatus = 0x00;
        Data = 0x00;
        _rxBuffer = 0x00;
        _rxReady = false;
        _txFull = false;
        _framingError = false;
        _overrunError = false;
        _cts = true;
        _dcd = true;
        _rie = false;
        _tie = false;
        _parityEnabled = false;
        _paritySelect = 0;
        _stopBits = 0;
    }
    
    /// <summary>
    /// Read from Control/Status Register ($FFFC00 or $FFFC04)
    /// Returns status flags
    /// </summary>
    public byte ReadControlStatus()
    {
        byte status = 0x00;
        if (_rxReady) status |= 0x01;       // DR
        if (_txFull) status |= 0x02;        // TDF
        if (_framingError) status |= 0x04;  // E
        if (_overrunError) status |= 0x08;  // OE
        if (_cts) status |= 0x10;           // CTS
        if (_dcd) status |= 0x20;           // DCD
        return status;
    }
    
    /// <summary>
    /// Write to Control Register ($FFFC00 or $FFFC04)
    /// Sets control bits
    /// </summary>
    public void WriteControl(byte value)
    {
        if ((value & 0x80) != 0)
        {
            Reset();
            return;
        }
        
        _rie = (value & 0x01) != 0;
        _tie = (value & 0x02) != 0;
        _paritySelect = (byte)((value >> 2) & 0x03);
        _parityEnabled = (value & 0x10) != 0;
        _stopBits = (byte)((value >> 5) & 0x03);
    }
    
    /// <summary>
    /// Read Data Register ($FFFC02 or $FFFC06)
    /// Returns received data
    /// </summary>
    public byte ReadData()
    {
        byte result = _rxBuffer;
        _rxReady = false;
        _framingError = false;
        _overrunError = false;
        return result;
    }
    
    /// <summary>
    /// Write Data Register ($FFFC02 or $FFFC06)
    /// Queues data for transmission
    /// </summary>
    public void WriteData(byte value)
    {
        _txFull = true;
        if (_onTransmit != null)
            _onTransmit(value);
        _txFull = false;
    }
    
    /// <summary>
    /// Simulate receiving a byte from serial line
    /// </summary>
    public void Receive(byte value)
    {
        if (_rxReady)
        {
            _overrunError = true;
            if (_rie)
                OnInterrupt();
            return;
        }
        
        _rxBuffer = value;
        _rxReady = true;
        if (_rie)
            OnInterrupt();
    }
    
    /// <summary>
    /// Set Clear to Send line state
    /// </summary>
    public void SetCts(bool value)
    {
        _cts = value;
    }
    
    /// <summary>
    /// Set Data Carrier Detect line state
    /// </summary>
    public void SetDcd(bool value)
    {
        _dcd = value;
    }
    
    /// <summary>
    /// Set transmit callback
    /// </summary>
    public void SetTransmitCallback(Action<byte> callback)
    {
        _onTransmit = callback;
    }
    
    /// <summary>
    /// Set receive callback
    /// </summary>
    public void SetReceiveCallback(Func<byte> callback)
    {
        _onReceive = callback;
    }
    
    void OnInterrupt()
    {
        // Interrupt will be routed through MFP
    }
    
    /// <summary>
    /// Parity configuration
    /// PS1 PS0
    ///  0   0 = No parity
    ///  0   1 = Odd
    ///  1   0 = Even
    ///  1   1 = Mark
    /// </summary>
    public byte GetParity()
    {
        return _paritySelect;
    }
    
    /// <summary>
    /// Stop bits configuration
    /// S1 S0
    ///  0  0 = 1 stop bit
    ///  0  1 = 1.5 stop bits
    ///  1  0 = 2 stop bits
    ///  1  1 = 1 stop bit
    /// </summary>
    public byte GetStopBits()
    {
        return _stopBits;
    }
}
