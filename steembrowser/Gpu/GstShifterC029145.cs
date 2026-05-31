using System;

/// <summary>
/// GST Shifter C029145 - Second-gen video controller for Atari STe
/// Package: PLCC144, 12-bit DAC (4-4-4 R/G/B)
/// Replaces original Shifter C028787
/// </summary>
public class GstShifterC029145
{
    // Video mode: bit 7=palette 16-color, bit 2=superHiRes, bit 1=mode, bit 0=mono
    public byte VidMode { get; private set; }
    public byte HsScroll { get; private set; }
    
    // Palette: 16 entries x 12 bits (4-bit R/G/B)
    public ushort[] Palette = new ushort[16];
    
    // Frame start/end
    public ushort FrameStart { get; private set; }
    public ushort FrameEnd { get; private set; }
    
    // Video counter (24-bit, read/write in STe)
    public uint VideoCounter { get; private set; }
    
    // Frame counter read delay / frame counter select
    public byte FcRdt { get; private set; }
    public byte FcSel { get; private set; }
    
    // Vertical scroll
    public ushort VScroll { get; private set; }
    
    // Configuration: 50/60 Hz, counter read mode
    public byte Config { get; private set; }
    
    // Sound DMA
    public ushort SoundFrameStart { get; private set; }
    public ushort SoundFrameEnd { get; private set; }
    
    /// <summary>
    /// Sample mode: bits 7-6 mono/stereo mode, bits 5-4 sample rate
    /// Sample rates: 00=6250Hz, 01=12500Hz, 10=25000Hz, 11=50000Hz
    /// </summary>
    public byte SampleMode { get; private set; }
    
    /// <summary>
    /// Sound control: bits 1-0
    /// 0=stop, 1=start one-shot, 2=start loop
    /// </summary>
    public byte SoundCtrl { get; private set; }
    
    // Internal video state
    uint _videoAddress;
    int _scanline, _pixel;
    bool _activeVideo;
    
    // Pixel clock: 16 MHz
    double _pixelClockHz = 16_000_000;
    double _pixelPeriodNs = 62.5;
    
    public GstShifterC029145()
    {
        Reset();
    }
    
    public void Reset()
    {
        VidMode = 0x00;
        HsScroll = 0x00;
        Array.Clear(Palette, 0, 16);
        FrameStart = 0xA000;
        FrameEnd = 0xA7FF;
        VideoCounter = 0;
        Config = 0x00;
        SoundFrameStart = 0;
        SoundFrameEnd = 0;
        SampleMode = 0x00;
        SoundCtrl = 0x00;
        _videoAddress = 0xA000;
        _scanline = 0;
        _pixel = 0;
        _activeVideo = false;
    }
    
    public byte ReadVideoRegister(ushort address)
    {
        ushort offset = (ushort)(address & 0xFF);
        switch (offset)
        {
            case 0x40: return VidMode;
            case 0x41: return 0; // Reserved
            case 0x60: return Config;
            case 0x61: return HsScroll;
            case 0x05: return (byte)((VideoCounter >> 16) & 0xFF);
            case 0x07: return (byte)((VideoCounter >> 8) & 0xFF);
            case 0x09: return (byte)(VideoCounter & 0xFF);
            case 0x0B: return FcRdt;
            case 0x0F: return FcSel;
            case 0x11: return (byte)((VScroll >> 8) & 0xFF);
            case 0x13: return (byte)(VScroll & 0xFF);
        }
        // Palette read (indices 0x42-0x4F correspond to palette entries)
        if (offset >= 0x42 && offset < 0x50)
        {
            int idx = offset - 0x42;
            if (idx < 16)
                return (byte)(Palette[idx] & 0xFF);
        }
        return 0xFF;
    }
    
    public void WriteVideoRegister(ushort address, byte value)
    {
        ushort offset = (ushort)(address & 0xFF);
        switch (offset)
        {
            case 0x40:
                VidMode = value;
                // Mode change resets address counters
                break;
            case 0x60: Config = value; break;
            case 0x61: HsScroll = value; break;
            case 0x01: FrameStart = (ushort)((FrameStart & 0x00FF) | (value << 8)); break;
            case 0x03: FrameStart = (ushort)((FrameStart & 0xFF00) | value); break;
            case 0x05: VideoCounter = (VideoCounter & 0x0000FF) | ((uint)value << 16); break;
            case 0x07: VideoCounter = (VideoCounter & 0xFF00FF) | ((uint)value << 8); break;
            case 0x09: VideoCounter = (VideoCounter & 0xFFFF00) | (uint)(value & 0xFE); break; // bit 0 = 0
            case 0x0B: FcRdt = value; break;
            case 0x0D: FrameEnd = (ushort)((FrameEnd & 0x00FF) | (value << 8)); break;
            case 0x0F: FcSel = value; break;
            case 0x11: VScroll = (ushort)((VScroll & 0x00FF) | (value << 8)); break;
            case 0x13: VScroll = (ushort)((VScroll & 0xFF00) | value); break;
        }
        // Palette writes
        if (offset >= 0x42 && offset < 0x50)
        {
            int idx = offset - 0x42;
            if (idx < 16)
            {
                if ((address & 1) == 0)
                    Palette[idx] = (ushort)((Palette[idx] & 0x00FF) | (value << 8));
                else
                    Palette[idx] = (ushort)((Palette[idx] & 0xFF00) | value);
            }
        }
    }
    
    public byte ReadSoundRegister(ushort address)
    {
        ushort offset = (ushort)(address & 0xFF);
        switch (offset)
        {
            case 0x00: return (byte)((SoundFrameStart >> 8) & 0xFF);
            case 0x01: return (byte)(SoundFrameStart & 0xFF);
            case 0x05: return (byte)((SoundFrameEnd >> 8) & 0xFF);
            case 0x06: return (byte)(SoundFrameEnd & 0xFF);
            case 0x0D: return SampleMode;
            case 0x10: return SoundCtrl;
        }
        return 0xFF;
    }
    
    public void WriteSoundRegister(ushort address, byte value)
    {
        ushort offset = (ushort)(address & 0xFF);
        switch (offset)
        {
            case 0x00: SoundFrameStart = (ushort)((SoundFrameStart & 0x00FF) | (value << 8)); break;
            case 0x01: SoundFrameStart = (ushort)((SoundFrameStart & 0xFF00) | value); break;
            case 0x05: SoundFrameEnd = (ushort)((SoundFrameEnd & 0x00FF) | (value << 8)); break;
            case 0x06: SoundFrameEnd = (ushort)((SoundFrameEnd & 0xFF00) | value); break;
            case 0x0D: SampleMode = value; break;
            case 0x10: SoundCtrl = value; break;
        }
    }
    
    /// <summary>
    /// Get sample rate from SampleMode register (bits 5-4)
    /// 00=6250Hz, 01=12500Hz, 10=25000Hz, 11=50000Hz
    /// </summary>
    public int GetSampleRate()
    {
        switch ((SampleMode >> 4) & 3)
        {
            case 0: return 6250;
            case 1: return 12500;
            case 2: return 25000;
            case 3: return 50000;
            default: return 6250;
        }
    }
    
    /// <summary>
    /// Stereo/mono mode from SampleMode bits 7-6
    /// </summary>
    public bool IsStereo => (SampleMode & 0xC0) != 0;
    
    /// <summary>
    /// Fetch one pixel from video RAM and return palette color index
    /// </summary>
    public byte FetchPixel(ushort[] videoRam)
    {
        if (VideoCounter < (uint)FrameStart || VideoCounter >= (uint)FrameEnd)
            return 0;
        
        int wordIndex = (int)(VideoCounter >> 1); // 16-bit words
        if (wordIndex < 0 || wordIndex >= videoRam.Length)
            return 0;
            
        ushort word = videoRam[wordIndex];
        
        // In low-res mode (320x200): 1 bit per pixel, 16 pixels/word
        // 0xA000 = 20 words per scanline
        // Determine pixel position within word
        int bitOffset = (int)(VideoCounter & 1) * 8;
        byte pixelVal = (byte)((word >> bitOffset) & 0xFF);
        
        // Extract color index from top bits (depends on mode)
        return (byte)(pixelVal & 0x0F);
    }
    
    /// <summary>
    /// RGB output for current pixel (0-15 intensity per channel)
    /// </summary>
    public (byte R, byte G, byte B) GetRgbOutput(byte paletteIndex)
    {
        if (paletteIndex >= 16) paletteIndex = 0;
        ushort entry = Palette[paletteIndex];
        byte r = (byte)((entry >> 12) & 0x0F);
        byte g = (byte)((entry >> 8) & 0x0F);
        byte b = (byte)((entry >> 4) & 0x0F);
        return (r, g, b);
    }
    
    public void AdvancePixel()
    {
        _pixel++;
        if (_pixel >= 640)
        {
            _pixel = 0;
            _scanline++;
            if (_scanline >= 200)
            {
                _scanline = 0;
                // Vertical blank handling
                VideoCounter = (uint)FrameStart;
            }
            else
            {
                // Advance video address (40 words per 640-pixel line)
                VideoCounter += 40;
            }
        }
        
        _activeVideo = (_scanline < 200 && _pixel < 640);
    }
}