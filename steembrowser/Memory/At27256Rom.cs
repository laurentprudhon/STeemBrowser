using System;

/// <summary>
/// Atari 27256 EPROM Chip (27xxx series)
/// 28-pin DIP, 32K×8 = 256 Kbit = 32 KiB
/// 
/// Used in: 520ST (U32), 1040ST (U32), STE (U9, U10)
/// 
/// TOS ROM versions: TOS 1.00-1.62, TOS 2.06, TOS 3.00-3.06
/// 
/// ROM access time: 250 ns typical (5V)
/// ROM contains: ST-Kernel, BIOS, GEM/AES, GEMDOS, Bootcode, MMIO, ARANYM
/// </summary>
public class At27256Rom
{
    byte[] _romData;
    
    public int Capacity { get; } = 32 * 1024;
    public bool OutputEnable { get; set; }
    public bool ChipEnable { get; set; }
    public int AccessTimeNs { get; } = 250;
    public string TOSVersion { get; private set; }
    
    public At27256Rom()
    {
        _romData = new byte[32 * 1024];
        Array.Fill(_romData, 0xFF);
        OutputEnable = false;
        ChipEnable = false;
    }
    
    public bool LoadRomData(byte[] data)
    {
        if (data == null || data.Length == 0)
            return false;
        
        Array.Copy(data, _romData, Math.Min(data.Length, _romData.Length));
        
        for (int i = data.Length; i < _romData.Length; i++)
            _romData[i] = 0xFF;
        
        return true;
    }
    
    public byte Read(ushort address)
    {
        if (!ChipEnable || !OutputEnable)
            return 0xFF;
        
        if (address >= _romData.Length)
            return 0xFF;
        
        return _romData[address];
    }
    
    public void Select(bool activeLow)
    {
        ChipEnable = activeLow;
    }
    
    public void SetOutputEnable(bool activeLow)
    {
        OutputEnable = activeLow;
    }
    
    public ushort CalculateChecksum()
    {
        ushort checksum = (ushort)(_romData[_romData.Length - 2] << 8);
        checksum |= _romData[_romData.Length - 1];
        return checksum;
    }
    
    public string GetVersionString()
    {
        string version = null;
        
        try
        {
            byte[] versionBytes = new byte[10];
            Array.Copy(_romData, 0x10, versionBytes, 0, 10);
            version = System.Text.Encoding.ASCII.GetString(versionBytes);
            
            int nullIndex = version.IndexOf('\0');
            if (nullIndex >= 0)
                version = version.Substring(0, nullIndex);
            
            version = version.Trim();
        }
        catch
        {
            return "Unknown";
        }
        
        if (string.IsNullOrEmpty(version) || !version.StartsWith("TOS"))
            return "Unknown";
        
        return version;
    }
    
    public byte[] GetRomData()
    {
        return _romData;
    }
    
    public bool VerifyChecksum(ushort expectedChecksum)
    {
        return CalculateChecksum() == expectedChecksum;
    }
}
