using System;

/// <summary>
/// Yamaha YM2149 Programmable Sound Generator
/// DIP40 package, memory-mapped at $FF8800-$FF88FF
/// 
/// 3 tone generator channels + 1 noise generator
/// 2x 8-bit I/O ports (Port A and Port B)
/// Pin-compatible with AY-3-8910
/// 
/// Pinout (DIP-40):
///   Ch A (4): Analog output channel A
///   Ch B (3): Analog output channel B
///   Ch C (38): Analog output channel C
///   DA0-DA7 (5-12): 8-bit data bus
///   BC1 (13): Bus control
///   BC2 (14): Bus control
///   SEL (15): Address select (A9=1: A8/A9 select register)
///   A8-A9 (16-17): Address bits for register selection
///   RESET (18): Reset input (active high)
///   CLK (19): Clock input
///   IOA0-IOA7 (20-27): I/O Port A bidirectional
///   IOB0-IOB7 (28-35): I/O Port B bidirectional
///   Vcc (40): +5V supply
/// </summary>
public class Ym2149Psg
{
    /// <summary>
    /// Channel A Frequency Low ($FF8800)
    /// 16-bit frequency divider - low byte
    /// </summary>
    public byte ChAFreqLow { get; private set; }
    
    /// <summary>
    /// Channel A Frequency High ($FF8801)
    /// 16-bit frequency divider - high byte
    /// </summary>
    public byte ChAFreqHigh { get; private set; }
    
    /// <summary>
    /// Channel B Frequency Low ($FF8802)
    /// </summary>
    public byte ChBFreqLow { get; private set; }
    
    /// <summary>
    /// Channel B Frequency High ($FF8803)
    /// </summary>
    public byte ChBFreqHigh { get; private set; }
    
    /// <summary>
    /// Channel C Frequency Low ($FF8804)
    /// </summary>
    public byte ChCFreqLow { get; private set; }
    
    /// <summary>
    /// Channel C Frequency High ($FF8805)
    /// </summary>
    public byte ChCFreqHigh { get; private set; }
    
    /// <summary>
    /// Noise Generator Period ($FF8806)
    /// 5-bit noise period register
    /// </summary>
    public byte NoisePeriod { get; private set; }
    
    /// <summary>
    /// Mixer Control ($FF8807)
    /// Bit 0: OUT1L - Output 1 left channel enable
    /// Bit 1: OUT1R - Output 1 right channel enable
    /// Bit 2: CH1 - Channel A mixer enable
    /// Bit 3: IN1A - Channel A tone select (1=tone)
    /// Bit 4: CH2 - Channel B mixer enable
    /// Bit 5: IN1B - Channel B tone select
    /// Bit 6: CH3 - Channel C mixer enable
    /// Bit 7: IN1C - Channel C tone select
    /// </summary>
    public byte MixerControl { get; private set; }
    
    /// <summary>
    /// Channel A Amplitude ($FF8808)
    /// 4-bit amplitude (0-15) or envelope control
    /// </summary>
    public byte ChAAmplitude { get; private set; }
    
    /// <summary>
    /// Channel B Amplitude ($FF8809)
    /// </summary>
    public byte ChBAmplitude { get; private set; }
    
    /// <summary>
    /// Channel C Amplitude ($FF880A)
    /// </summary>
    public byte ChCAmplitude { get; private set; }
    
    /// <summary>
    /// Envelope Period Fine ($FF880B)
    /// Lower byte of envelope period
    /// </summary>
    public byte EnvPeriodFine { get; private set; }
    
    /// <summary>
    /// Envelope Period Coarse ($FF880C)
    /// Upper byte of envelope period
    /// Envelope Period = 2 * (Coarse + 1) * 2^Fine / f_CPU
    /// </summary>
    public byte EnvPeriodCoarse { get; private set; }
    
    /// <summary>
    /// Envelope Shape ($FF880D)
    /// Bit 0: C - Continuous (1) or Gate (0)
    /// Bit 1: R - Ramp (1) or Step (0)
    /// Bit 2: A - Alternate (1) or Continuous (0)
    /// Bit 3: H - Hold (1) or Stop (0)
    /// Bits 4-7: Not implemented
    /// </summary>
    public byte EnvShape { get; private set; }
    
    /// <summary>
    /// I/O Port A Data ($FF880E)
    /// Output data for port A
    /// Atari ST usage:
    ///   Bit 0: MIDI out data
    ///   Bit 1: Joystick trigger
    ///   Bit 2: RS232 handshake / LED control
    ///   Bit 3: RS232 RTS
    ///   Bit 4: RS232 DTR
    /// </summary>
    public byte IoPortA { get; private set; }
    
    /// <summary>
    /// I/O Port B Data ($FF880F)
    /// 8-bit parallel port data output (printer)
    /// </summary>
    public byte IoPortB { get; private set; }
    
    // Internal state
    byte _regSelect;
    uint _counterA, _counterB, _counterC;
    uint _noiseCounter;
    bool _noiseEnabled;
    
    // Frequency clock divisor
    uint _clockFrequency;
    
    public Ym2149Psg(uint clockFrequency = 8_000_000)
    {
        _clockFrequency = clockFrequency;
        Reset();
    }
    
    public void Reset()
    {
        ChAFreqLow = 0x00;
        ChAFreqHigh = 0x00;
        ChBFreqLow = 0x00;
        ChBFreqHigh = 0x00;
        ChCFreqLow = 0x00;
        ChCFreqHigh = 0x00;
        NoisePeriod = 0x00;
        MixerControl = 0x00;
        ChAAmplitude = 0x00;
        ChBAmplitude = 0x00;
        ChCAmplitude = 0x00;
        EnvPeriodFine = 0x00;
        EnvPeriodCoarse = 0x00;
        EnvShape = 0x00;
        IoPortA = 0x00;
        IoPortB = 0x00;
        _regSelect = 0x00;
        _counterA = 0;
        _counterB = 0;
        _counterC = 0;
        _noiseCounter = 0;
        _noiseEnabled = false;
    }
    
    /// <summary>
    /// Select register (write to $FF88xx where xx = register number)
    /// </summary>
    public void SelectRegister(byte register)
    {
        _regSelect = register;
    }
    
    /// <summary>
    /// Write data to currently selected register
    /// </summary>
    public void WriteRegister(byte data)
    {
        switch (_regSelect)
        {
            case 0x00: ChAFreqLow = data; ChannelAFrequency = (ushort)((ChAFreqHigh << 8) | data); break;
            case 0x01: ChAFreqHigh = data; ChannelAFrequency = (ushort)((data << 8) | ChAFreqLow); break;
            case 0x02: ChBFreqLow = data; break;
            case 0x03: ChBFreqHigh = data; break;
            case 0x04: ChCFreqLow = data; break;
            case 0x05: ChCFreqHigh = data; break;
            case 0x06: NoisePeriod = data; break;
            case 0x07: MixerControl = data; UpdateNoiseEnabled(); break;
            case 0x08: ChAAmplitude = data; break;
            case 0x09: ChBAmplitude = data; break;
            case 0x0A: ChCAmplitude = data; break;
            case 0x0B: EnvPeriodFine = data; break;
            case 0x0C: EnvPeriodCoarse = data; break;
            case 0x0D: EnvShape = data; break;
            case 0x0E: IoPortA = data; break;
            case 0x0F: IoPortB = data; break;
        }
    }
    
    /// <summary>
    /// Read from currently selected register
    /// </summary>
    public byte ReadRegister()
    {
        switch (_regSelect)
        {
            case 0x00: return ChAFreqLow;
            case 0x01: return ChAFreqHigh;
            case 0x02: return ChBFreqLow;
            case 0x03: return ChBFreqHigh;
            case 0x04: return ChCFreqLow;
            case 0x05: return ChCFreqHigh;
            case 0x06: return NoisePeriod;
            case 0x07: return MixerControl;
            case 0x08: return ChAAmplitude;
            case 0x09: return ChBAmplitude;
            case 0x0A: return ChCAmplitude;
            case 0x0B: return EnvPeriodFine;
            case 0x0C: return EnvPeriodCoarse;
            case 0x0D: return EnvShape;
            case 0x0E: return IoPortA;
            case 0x0F: return IoPortB;
        }
        return 0x00;
    }
    
    /// <summary>
    /// Get channel A frequency divisor
    /// </summary>
    public ushort ChannelAFrequency => (ushort)((ChAFreqHigh << 8) | ChAFreqLow);
    
    /// <summary>
    /// Get channel B frequency divisor
    /// </summary>
    public ushort ChannelBFrequency => (ushort)((ChBFreqHigh << 8) | ChBFreqLow);
    
    /// <summary>
    /// Get channel C frequency divisor
    /// </summary>
    public ushort ChannelCFrequency => (ushort)((ChCFreqHigh << 8) | ChCFreqLow);
    
    /// <summary>
    /// Calculate output frequency for a channel
    /// Frequency = f_clock / (20 * N * R)
    /// </summary>
    public double CalculateFrequency(ushort frequencyDivisor)
    {
        if (frequencyDivisor == 0)
            frequencyDivisor = 1;
        return _clockFrequency / (20.0 * frequencyDivisor);
    }
    
    void UpdateNoiseEnabled()
    {
        _noiseEnabled = ((MixerControl >> 3) & 0x70) != 0;
    }
    
    /// <summary>
    /// Generate one sample of audio output
    /// </summary>
    public (short left, short right) GenerateSample()
    {
        short leftChannel = 0;
        short rightChannel = 0;
        
        short channelA = GenerateToneChannel(ref _counterA, ChannelAFrequency);
        short channelB = GenerateToneChannel(ref _counterB, ChannelBFrequency);
        short channelC = GenerateToneChannel(ref _counterC, ChannelCFrequency);
        
        // Channel A routing
        if ((MixerControl & 0x04) != 0) // CH1 enabled
        {
            if ((MixerControl & 0x01) != 0) // OUT1L
            {
                if ((MixerControl & 0x08) != 0) // IN1A tone enabled
                    leftChannel += MultiplyByVolume(channelA, ChAAmplitude);
                else
                    leftChannel += GenerateNoise();
            }
            else // OUT1R
            {
                if ((MixerControl & 0x08) != 0)
                    rightChannel += MultiplyByVolume(channelA, ChAAmplitude);
                else
                    rightChannel += GenerateNoise();
            }
        }
        
        // Channel B routing
        if ((MixerControl & 0x10) != 0) // CH2 enabled
        {
            if ((MixerControl & 0x02) != 0) // OUT1R
                rightChannel += MultiplyByVolume(channelB, ChBAmplitude);
            else // OUT1L
                leftChannel += MultiplyByVolume(channelB, ChBAmplitude);
        }
        
        // Channel C routing
        if ((MixerControl & 0x40) != 0) // CH3 enabled
        {
            if ((MixerControl & 0x08) != 0)
                leftChannel += MultiplyByVolume(channelC, ChCAmplitude);
            else
                rightChannel += GenerateNoise();
        }
        
        return (leftChannel, rightChannel);
    }
    
    short GenerateToneChannel(ref uint counter, ushort frequencyDivisor)
    {
        if (frequencyDivisor == 0) frequencyDivisor = 1;
        
        counter++;
        if (counter >= frequencyDivisor)
            counter = 0;
        
        return (short)(Math.Abs((int)(Math.Sin(2 * Math.PI * counter / frequencyDivisor)) * 32767));
    }
    
    short GenerateNoise()
    {
        if (NoisePeriod == 0)
            return 0;
        
        _noiseCounter++;
        if (_noiseCounter >= ((uint)NoisePeriod * 2))
        {
            // LFSR for noise
            bool bit = (_noiseCounter & 1) ^ ((_noiseCounter >> 1) & 1);
            return (short)(bit ? 32767 : -32767);
        }
        return 0;
    }
    
    short MultiplyByVolume(short sample, byte amplitude)
    {
        // If envelope is used
        if ((amplitude & 0x0F) == 0x0F)
        {
            return sample * 16; // Full amplitude when envelope active
        }
        return (short)(sample * amplitude);
    }
}
