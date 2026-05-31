using System;

/// <summary>
/// Hitachi HD6301V1 Intelligent Keyboard Controller
/// DIP40 package, located inside keyboard case
/// 
/// Handles keyboard scanning (8x7 matrix), mouse tracking,
/// and joystick polling. Communicates with ST via MC6850 ACIA
/// serial link at 7.8125 Kbps.
/// 
/// Internal: 4 MHz clock, 48 bytes RAM, 1 KB mask ROM
/// 
/// Mouse events (delta-based):
///   $81 = X+, $82 = X-, $83 = Y+, $84 = Y-
///   $85 = B1 press, $86 = B1 release
///   $87 = B2 press, $88 = B2 release
///   $89-$8F = Joystick events
/// 
/// Keyboard scan codes:
///   Key press (make): $80 + n
///   Key release (break): n
/// 
/// Protocol:
///   Header: $FF
///   Length: 1-4 bytes
///   Data: event/command bytes
///   Checksum: XOR of data bytes
/// </summary>
public class Hd6301Ikbd
{
    // Keyboard matrix state: 8 rows x 7 columns = 56 possible keys
    bool[,] _keyMatrix = new bool[8, 7];
    
    // Event queue (FIFO)
    byte[] _eventQueue = new byte[16];
    int _queueHead;
    int _queueTail;
    int _queueLength;
    
    // Mouse state
    int _mouseX;
    int _mouseY;
    bool _mouseButton1;
    bool _mouseButton2;
    
    // Joystick state
    bool _joy0XPlus;
    bool _joy0XMinus;
    bool _joy0YPlus;
    bool _joy0YMinus;
    bool _joy0Fire;
    bool _joy1XPlus;
    bool _joy1XMinus;
    bool _joy1YPlus;
    bool _joy1YMinus;
    bool _joy1Fire;
    
    // Interrupt state
    bool _interruptEnabled;
    bool _interruptPending;
    
    // Mouse/joystick event enable
    bool _mouseEnabled;
    bool _joyEnabled;
    
    // Serial communication
    byte[] _txBuffer = new byte[8];
    int _txPosition;
    bool _transmitting;
    Action<byte> _onTransmit;
    
    // Scan timer (~300 Hz)
    int _scanCounter;
    int _scanRate;
    
    public Hd6301Ikbd()
    {
        _scanRate = 300;
        _interruptEnabled = true;
        _mouseEnabled = true;
        _joyEnabled = true;
        Reset();
    }
    
    public void Reset()
    {
        Array.Clear(_keyMatrix, 0, _keyMatrix.Length);
        _queueHead = 0;
        _queueTail = 0;
        _queueLength = 0;
        _mouseX = 0;
        _mouseY = 0;
        _mouseButton1 = false;
        _mouseButton2 = false;
        _interruptEnabled = true;
        _interruptPending = false;
        _transmitting = false;
        _scanCounter = 0;
    }
    
    /// <summary>
    /// Process ST commands received from ACIA
    /// </summary>
    public void ProcessCommand(byte command)
    {
        switch (command)
        {
            case 0x01:
                _interruptEnabled = true;
                break;
            case 0x00:
                _interruptEnabled = false;
                break;
            case 0x10:
                _joyEnabled = true;
                break;
            case 0x18:
                _joyEnabled = false;
                break;
            case 0x20:
                ReadJoystickFire();
                break;
            case 0x30:
                _queueHead = _queueTail;
                _queueLength = 0;
                break;
            case 0x31:
                _queueHead = _queueTail;
                _queueLength = 0;
                break;
            case 0x50:
                _mouseEnabled = true;
                break;
            case 0x51:
                _mouseEnabled = false;
                break;
            case 0x52:
                _mouseX = 0;
                _mouseY = 0;
                break;
        }
    }
    
    /// <summary>
    /// Simulate key press - make code ($80 + n)
    /// </summary>
    public void KeyPress(byte scanCode)
    {
        byte makeCode = (byte)(scanCode | 0x80);
        QueueEvent(makeCode);
    }
    
    /// <summary>
    /// Simulate key release - break code (n)
    /// </summary>
    public void KeyRelease(byte scanCode)
    {
        byte breakCode = (byte)(scanCode & 0x7F);
        QueueEvent(breakCode);
    }
    
    /// <summary>
    /// Simulate mouse movement
    /// </summary>
    public void MouseMove(int deltaX, int deltaY)
    {
        if (!_mouseEnabled) return;
        
        _mouseX += deltaX;
        _mouseY += deltaY;
        
        // Generate mouse events
        while (deltaX > 0) { QueueEvent(0x81); deltaX--; }
        while (deltaX < 0) { QueueEvent(0x82); deltaX++; }
        while (deltaY > 0) { QueueEvent(0x83); deltaY--; }
        while (deltaY < 0) { QueueEvent(0x84); deltaY++; }
    }
    
    /// <summary>
    /// Simulate mouse button press
    /// </summary>
    public void MouseButton(bool button1, bool pressed)
    {
        if (!_mouseEnabled) return;
        
        if (button1)
        {
            if (pressed && !_mouseButton1)
            {
                _mouseButton1 = true;
                QueueEvent(0x85);
            }
            else if (!pressed && _mouseButton1)
            {
                _mouseButton1 = false;
                QueueEvent(0x86);
            }
        }
        else
        {
            if (pressed && !_mouseButton2)
            {
                _mouseButton2 = true;
                QueueEvent(0x87);
            }
            else if (!pressed && _mouseButton2)
            {
                _mouseButton2 = false;
                QueueEvent(0x88);
            }
        }
    }
    
    /// <summary>
    /// Simulate joystick input
    /// </summary>
    public void SetJoystick(byte port, bool xPlus, bool xMinus, bool yPlus, bool yMinus, bool fire)
    {
        if (!_joyEnabled) return;
        
        if (port == 0)
        {
            _joy0XPlus = xPlus;
            _joy0XMinus = xMinus;
            _joy0YPlus = yPlus;
            _joy0YMinus = yMinus;
            _joy0Fire = fire;
        }
        else
        {
            _joy1XPlus = xPlus;
            _joy1XMinus = xMinus;
            _joy1YPlus = yPlus;
            _joy1YMinus = yMinus;
            _joy1Fire = fire;
        }
    }
    
    void ReadJoystickFire()
    {
        byte fireStates = 0x00;
        if (_joy0Fire) fireStates |= 0x01;
        if (_joy1Fire) fireStates |= 0x02;
        QueueEvent(fireStates);
    }
    
    /// <summary>
    /// Queue an event for transmission to ACIA
    /// </summary>
    void QueueEvent(byte eventData)
    {
        if (_queueLength >= _eventQueue.Length)
            return;
        
        _eventQueue[(_queueTail + _queueLength) % _eventQueue.Length] = eventData;
        _queueLength++;
        
        if (_interruptEnabled)
        {
            _interruptPending = true;
            TransmitPacket();
        }
    }
    
    /// <summary>
    /// Build and transmit a packet to ACIA
    /// Format: Header ($FF), Length, Data..., Checksum (XOR)
    /// </summary>
    void TransmitPacket()
    {
        if (_transmitting) return;
        
        _transmitting = true;
        _txPosition = 0;
        
        // Start with header
        SendByte(0xFF);
    }
    
    void SendByte(byte value)
    {
        if (_onTransmit != null)
            _onTransmit(value);
        _txPosition++;
        
        if (_txPosition >= _txBuffer.Length)
        {
            _transmitting = false;
            _interruptPending = false;
            _queueLength = 0;
        }
    }
    
    /// <summary>
    /// Keyboard scan cycle - called at ~300 Hz
    /// </summary>
    public void ScanCycle()
    {
        _scanCounter++;
        if (_scanCounter >= _scanRate)
        {
            _scanCounter = 0;
            // Scan key matrix here
            ScanKeyMatrix();
        }
    }
    
    void ScanKeyMatrix()
    {
        // Scan each row/column for new key state
        for (int row = 0; row < 8; row++)
        {
            for (int col = 0; col < 7; col++)
            {
                // Key state changes would be detected here
                // and events queued through QueueEvent()
            }
        }
    }
    
    /// <summary>
    /// Set serial transmit callback
    /// </summary>
    public void SetTransmitCallback(Action<byte> callback)
    {
        _onTransmit = callback;
    }
    
    /// <summary>
    /// Check if there are pending events to send
    /// </summary>
    public bool HasPendingEvents => _queueLength > 0;
}
