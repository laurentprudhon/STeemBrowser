/*---------------------------------------------------------------------------
PROJECT: Steem SSE
Atari ST emulator
Copyright (C) 2026 by Anthony Hayward and Russel Hayward + SSE

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see https://www.gnu.org/licenses/.

DOMAIN: OS
FILE: tos.cpp
DESCRIPTION: TOS (The Operating System) utilities, loading TOS images,
also cartridge utilities
---------------------------------------------------------------------------*/

#include <pch.h>
#pragma hdrstop

//#include <debug.h>
#include <computer.h>
#include <translate.h>
#include <notifyinit.h>
#include <iolist.h>

/*
#define SVmemvalid 0x420
#define SVmemctrl 0x424
#define SV_membot 0x432
#define SVmemval2 0x43a
#define SVsshiftmd 0x44c
*/

#define SV_MEMTOP 0x436
#define SV_SCREENPT 0x45e
#define SV_V_BAS_AD 0x44e

// BIOS Functions by Opcode
//#define GETMPB   0x00 //Return the address of the MPB (Memory Parameter Block) structure
//#define BCONSTAT 0x01 //Determine if a character is waiting from a device
//#define BCONIN   0x02 //Input a character from a device
#define BCONOUT  0x03 //Output a character from a device
//#define RWABS    0x04 //Read/write sectors to a device
//#define SETEXC   0x05 //Set or read a system exception vector
//#define TICKAL   0x06 //Return the current system timer calibration
#define GETBPB   0x07 //Return the address of the BPB (BIOS Parameter Block)
//#define BCOSTAT  0x08 //Determine if a device is ready to receive a character
#define MEDIACH  0x09 //Determine if a drive's media has been changed
#define DRVMAP   0x0A //Return a bitmap of mounted drives
//#define KBSHIFT  0x0B //Return the state of the keyboard shift keys

// XBIOS Functions by Opcode
#define RANDOM   0x11 //Return a random number
#define GETTIME  0x17 //Get the time of day and current date
#if defined(SSE_GEM_CONTROL_PANEL)
#define KBRATE   0x23
#endif
#define VSYNC    0x25 //Hold the process until the next vertical blank

BYTE *STRom=NULL;  
#ifndef BIG_ENDIAN_PROCESSOR
BYTE *Rom_End,*Rom_End_minus_1,*Rom_End_minus_2,*Rom_End_minus_4;
#endif
BYTE *cart=NULL,*cart_save=NULL;
#ifndef BIG_ENDIAN_PROCESSOR
BYTE *Cart_End_minus_1,*Cart_End_minus_2,*Cart_End_minus_4;
#endif
DWORD tos_len=0;
MEM_ADDRESS rom_addr=0,rom_addr_end=0;
// init at first trap:
MEM_ADDRESS os_gemdos_vector=0,os_bios_vector=0,os_xbios_vector=0;
WORD tos_version;
bool tos_high;
int aes_calls_since_reset=0;


void intercept_gemdos(),intercept_bios(),intercept_xbios();

// enumerate TOS files for the GUI
EasyStr TTos::GetNextTos(DirSearch &ds) {
  EasyStr Path;
  if((ds.Attrib & (FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_HIDDEN))==0)
  {
    Path=UsersPath+SLASH+ds.Name;
#ifdef WIN32
    if(has_extension(Path,"LNK")) // links are hosted in UsersPath, so TOS files can be anywhere
    {
      WIN32_FIND_DATA wfd;
      EasyStr DestPath=GetLinkDest(Path,&wfd);
      if(has_extension_list(DestPath,"IMG","ROM",NULL))
        if(Exists(DestPath)) 
          Path=DestPath;
    }
#endif
#ifdef UNIX
    char LinkPath[MAX_PATH+1];
    memset(LinkPath,0,MAX_PATH+1);
    if(readlink(Path,LinkPath,MAX_PATH)>0)
    {
      if(has_extension_list(LinkPath,"IMG","ROM",NULL))
      {
        if(Exists(LinkPath))
          Path=LinkPath;
        else
          Path="";
      }
    }
#endif
  }
  return Path;
}


void TTos::GetTosProperties(EasyStr Path,WORD &Ver,BYTE &Country,WORD &Date,BYTE &Recognised) {
  FILE *fp=fopen(Path,"rb");
  if(fp!=NULL)
  {
    DWORD Len=(DWORD)GetFileLength(fp);
    if(Len<0x1f)
      return;
    BYTE *buf=new BYTE[Len];
    FREAD(buf,1,Len,fp);
    Ver=*(WORD*)&buf[2];
    SWAP_BIG_ENDIAN_WORD(Ver);
    Country=buf[0x1d];
    Date=*(WORD*)&buf[0x1e];
    SWAP_BIG_ENDIAN_WORD(Date);
    DWORD checksum=~0u;
    add_to_crc32(checksum,buf,Len);
    checksum=~checksum;
    switch(checksum) {
    case 0x1A586C64: // 1.0 UK
    case 0x3B5CD0C5: // 1.02 UK
    case 0xA50D1D43: // 1.04 UK
    case 0xD72FEA29: // 1.06 UK
    case 0x1C1A4EBA: // 1.62 UK
    case 0x90822603: // 2.05 UK
    case 0x08538E39: // 2.06 UK
    case 0xD13FA742: // EmuTOS 192K 1.0 //update?
    case 0x6FAFB37A: // EmuTOS 256K 1.0
    // ... could add some other countries - but nobody certified any TOS they use
      Recognised=1; // will display in green
      break;
    case 0xD1C6F2FA: // 1.62 UK
    case 0x0296915D: // HD6301V1ST.img
      Recognised=0xff; // bad in red
      break;
    default:
      Recognised=(Len%1024) ? 0xff : 0; // bad if not multiple of 1024
    }
    delete [] buf;
    TRACE_INIT("TOS %03X country %X %02d/%02d/%04d %s %08X path %s\n",Ver,
      Country,(Date&0x1F),(Date>>5)&0xF,(Date>>9)+1980,"CRC32",checksum,GetFileNameFromPath(Path.Text));
    //TRACE_INIT("TOS %03X country %X date %04X CRC32 %08X path %s\n",Ver,Country,
      //Date,checksum,Path.Text);
    fclose(fp);
  }
}


#if defined(SSE_TOS_KEYBOARD_CLICK)

void TTos::CheckKeyboardClick() {
  if(LPEEK(0x44E)==vbase) // not perfect, at least it's a check //v402 placed here //v420 Windows: replaced
  {
    if(OPTION_KEYBOARD_CLICK)
      PEEK(0x484)|=0x01;
    else
      PEEK(0x484)&=0xFE;
  }
}

#endif


#ifndef NO_CRAZY_MONITOR

void TTos::HackMemoryForExtendedMonitor() {
  TRACE_INIT("EM mem_len %X vbase %X phystop %X _memtop %X\n",mem_len,vbase,
    LPEEK(SV_PHYSTOP),LPEEK(SV_MEMTOP));
  MEM_ADDRESS bytes_needed=MAX((int)((em_width*em_height*em_planes)/8),0x8000);
  //ASSERT(bytes_needed>0x8000);
  MEM_ADDRESS xbios2a=mem_len-(bytes_needed+256);
  vbase=(xbios2a+255)&-256;
  //ASSERT(vbase+bytes_needed<=mem_len);
  SafeLPoke(SV_MEMTOP,vbase); //_memtop
  SafeLPoke(SV_V_BAS_AD,vbase);
  SafeLPoke(SV_SCREENPT,vbase);
  if(em_planes==1)
    Mfp.reg[MFPR_GPIP]|=MFP_GPIP_MONO_MASK;
  TRACE_INIT("EM bytes_needed %d vbase %X phystop %X _memtop %X\n",bytes_needed,
    vbase,LPEEK(SV_PHYSTOP),LPEEK(SV_MEMTOP));
}

#endif


#define LOGSECTION LOGSECTION_TRAP

#define INVALID_SP (mem_len-128)

// This always returns a valid address
MEM_ADDRESS GetSPBeforeTrap(bool* const pInvalid) {
  if(pInvalid!=NULL)
    *pInvalid=false;
  MEM_ADDRESS my_sp=(SP&0xffffff)+6;
  // my_sp now points to first byte after the exception stack frame
  if(my_sp<mem_len)
  {
    // First byte on stack is high byte of sr
    if((SafePeek(SP&0xffffff) & BIT_5)==0)
      // Supervisor bit not set in stacked sr
      my_sp=other_sp&0xffffff;
  }
  if(my_sp>=mem_len)
  {
    if(pInvalid!=NULL)
      *pInvalid=true;
    return INVALID_SP;
  }
  return my_sp;
}

#undef INVALID_SP

void intercept_os() {
  ioaccess&=~IOACCESS_INTERCEPT_OS;
  if(LITTLE_PC==os_gemdos_vector)
  {
    intercept_gemdos();
    ioaccess|=IOACCESS_INTERCEPT_OS;
  }
  else if(LITTLE_PC==os_bios_vector)
  {
    intercept_bios();
    ioaccess|=IOACCESS_INTERCEPT_OS;
  }
  else if(LITTLE_PC==os_xbios_vector)
  {
    intercept_xbios();
    ioaccess|=IOACCESS_INTERCEPT_OS;
  }
  else if(IRD==0x4E42)
  {
#if defined(SSE_ENABLE_TRACE_LOG)
    if(logsection_enabled[LOGSECTION_TRAP])
#if defined(SSE_DEBUGGER_FAKE_IO)
      if((TRACE_MASK5&TRACE_CONTROL_TRAP2))
#endif
      log_os_call(TRAP_GEM);
#endif
    if(REGW(0)==0x73) // VDI
    {
#if defined(SSE_GUI_STATUS_BAR_MOUSE) && !defined(SSE_GUI_STATUS_BAR_MOUSE2)
      MEM_ADDRESS contrl=SafeLPeek(REGL(1)); //Cpu.r[1] has vdi parameter block.
      WORD vdi_op=SafeDPeek(contrl+0);  //opcode
      if(vdi_op==124) // vq_mouse
      {
        on_rte=ON_RTE_MOUSE;
        on_rte_interrupt_depth=interrupt_depth;
        vdi_ptsout=SafeLPeek(REGL(1)+16);
      }
#endif
    }
#ifndef NO_CRAZY_MONITOR
    if(extended_monitor)
    { //instruction is TRAP #2 (VDI or AES) (Megar)
      if(REGW(0)==0x73) 
      { //vdi
        MEM_ADDRESS adddd=SafeLPeek(Cpu.r[1]); //r[1] has vdi parameter block.  
        if(SafeDPeek(adddd+0)==1) //adddd points to the control array
        { //v_opnwk OPCODE
#if defined(SSE_STATS)
          Stats.nVdii++;
#endif
          on_rte=ON_RTE_EMHACK;
          on_rte_interrupt_depth=interrupt_depth;
          vdi_intout=SafeLPeek(Cpu.r[1]+12);
        }
      }
      ioaccess|=IOACCESS_INTERCEPT_OS;
    }
#endif
  }
}


void intercept_gemdos() {
#if defined(SSE_ENABLE_TRACE_LOG)
  if(logsection_enabled[LOGSECTION_TRAP])
#if defined(SSE_DEBUGGER_FAKE_IO)
    if((TRACE_MASK5&TRACE_CONTROL_TRAP1))
#endif
    log_os_call(TRAP_GEMDOS);
#endif
#ifndef DISABLE_STEMDOS
  StemdosCheckTrap1();
#endif
}

#if defined(SSE_ENABLE_TRACE_LOG)
bool vt52=false; // to skip VT-52 emulator escape characters
#endif

void intercept_bios() {
/*The ST BIOS routines can be called from user mode, and
are reentrant to three levels. They use registers A0-A2 and
D0-D2 as scratch registers. You should
note that the BIOS changes the command number and return
address on the stack.
*/
  bool Invalid;
  MEM_ADDRESS my_sp=GetSPBeforeTrap(&Invalid);
  if(Invalid)
    return;
  WORD func=SafeDPeek(my_sp);
#if defined(SSE_ENABLE_TRACE_LOG)
  // if log GEMDOS we echo bconout (includes conout) (debugger or _DEBUG)
  // also if trap bios but not level 2
  if(func==BCONOUT && SafeDPeek(my_sp+2)==2 
    && (!logsection_enabled[LOGSECTION_TRAP] && logsection_enabled[LOGSECTION_HARDDRIVE]
#if defined(SSE_DEBUGGER_FAKE_IO)
      || logsection_enabled[LOGSECTION_TRAP] && (TRACE_MASK5&TRACE_CONTROL_TRAP13) 
      && !(TRACE_MASK0&TRACE_LEVEL2) 
#endif    
    ))
  {
    char c=(char)SafeDPeek(my_sp+4);
    if(c==27)
      vt52=true;
    else if(!vt52)
    {
#if defined(SSE_DEBUGGER_FAKE_IO)
      if((TRACE_MASK5&TRACE_CONTROL_TRAP14))
#endif
      {
        TRACE("%c",c);
      }
    }
    else 
      vt52=false;
  }
  else if(logsection_enabled[LOGSECTION_TRAP]) 
#if defined(SSE_DEBUGGER_FAKE_IO)
    if((TRACE_MASK5&TRACE_CONTROL_TRAP13))
    ///////////////////if((TRACE_MASK0&TRACE_LEVEL2)||)
#endif
    log_os_call(TRAP_BIOS);
#endif
#if defined(SSE_DEBUG_SYMBOLS)
  Tos.LastFunc=func; // BIOS-only for the moment
#endif
  switch(func) {
  case DRVMAP:
#ifndef DISABLE_STEMDOS
    // TOS calls this first before a HD check so this is the right place to do it
    Stemdos.UpdateDrvbits(); 
#endif
    break;
#ifndef DISABLE_STEMDOS
  case GETBPB:
  case MEDIACH:
  {
    WORD d=SafeDPeek(my_sp+2);
    if(d>=2)
    {
      if(Stemdos.IsMounted((BYTE)d))
      {
        if(func==GETBPB)
        { 
#if !defined(SSE_412R16) // fix old bug GEMDOS emu interception of GETBPB (Geneva/NeoDesk)

          TODO: should fake a BPB? but see $472: HD drivers take care of it

#if doc
Getbpb() returns a pointer to the device s BPB. The BPB is defined as follows:
typedef struct
{
WORD recsiz; /* bytes per sector */
WORD clsiz; /* sectors per cluster */
WORD clsizb; /* bytes per cluster */
WORD rdlen; /* sector length of root directory */
WORD fsiz; /* sectors per FAT */
WORD fatrec; /* starting sector of second FAT */
WORD datrec; /* starting sector of data */
WORD numcl; /* clusters per disk */
WORD bflags; /* bit 0=1 - 16 bit FAT, else 12 bit */
} BPB;
#endif
          // Make it get the BPB of A: instead, this might contain nonsense!
          SafeDPoke(my_sp+2,0); 
#endif
        }
        else
        {
          Cpu.r[0]=0; // Hasn't changed - everything ignores this anyway
          m68kPerformRte();  //don't need to check interrupts because sr won't actually have changed
        }
#if defined(SSE_STATS)
        Stats.nBios++;
#endif
      }
    }
    break;
  }
#endif//#ifndef DISABLE_STEMDOS
  default:
    break;
  }//sw
}


void intercept_xbios() {
  bool Invalid;
  MEM_ADDRESS my_sp=GetSPBeforeTrap(&Invalid);
  if(Invalid)
    return;
  WORD func=SafeDPeek(my_sp);
#if defined(SSE_ENABLE_TRACE_LOG)
  if(logsection_enabled[LOGSECTION_TRAP])
#if defined(SSE_DEBUGGER_FAKE_IO)
    if((TRACE_MASK5&TRACE_CONTROL_TRAP14))
#endif
    log_os_call(TRAP_XBIOS);
#endif
  switch(func) {
  case VSYNC:
    if(OPTION_EMU_DETECT && Cpu.r[6]==Cpu.r[7] && Cpu.r[7]==0x456d753f) // Emu?
    { // Vsync with Emu? in D6, D7 ->emudetect on
      Cpu.r[6]=0x53544565; // STEe
      Cpu.r[7]=0x6d456e67; // mEng
      areg[0]=0xffc100; // base
      emudetect_called=true;
#if !defined(SSE_NO_FALCONMODE)
      emudetect_init();
#endif
#if defined(SSE_STATS)
      Stats.mskSpecial|=Stats.EMU_DETECT;
#endif
      m68kPerformRte();  //don't need to check interrupts because sr won't actually have changed
    }
    break;

#if defined(SSE_GEM_CONTROL_PANEL)
  case KBRATE:
    if(OptionBox.IsVisible() && OptionBox.Page==PAGE_GEM_CP)
    {
      WORD rate=SafeDPeek(my_sp+4);
      WORD delay=SafeDPeek(my_sp+2);
      HWND hwnd=GetDlgItem(OptionBox.Handle,IDC_REPEAT_DELAY);
      if(delay!=0xFFFF)
        PostMessage(hwnd,TBM_SETPOS,1,delay);
      PostMessage(OptionBox.Handle,WM_HSCROLL,0,(LPARAM)hwnd); // force update
      hwnd=GetDlgItem(OptionBox.Handle,IDC_REPEAT_RATE);
      if(rate!=0xFFFF)
      {
        PostMessage(hwnd,TBM_SETPOS,1,rate);
        PostMessage(OptionBox.Handle,WM_HSCROLL,0,(LPARAM)hwnd); // force update
      }
    }
    break;
#endif

#if !defined(DISABLE_STEMDOS)
  case GETTIME:
    if(Stemdos.bInterceptDateTime && OPTION_RTC_HACK)
    { // Get clock time
      time_t t=time(NULL);
      struct tm *lpTime=localtime(&t);
      Cpu.r[0]=TMToDOSDateTime(lpTime);
      m68kPerformRte();  //don't need to check interrupts because sr won't actually have changed
#if defined(SSE_STATS)
      Stats.nXbios++;
#endif
    }
    break;
#endif
//#define SSE_TOS_RANDOM
#if defined(SSE_TOS_RANDOM)
  //Random() returns a 24 bit random number
  case RANDOM:
    if(OPTION_HACKS)
    {
#ifdef SSE_BETA
      TRACE_OSD("RND");
      r[0]=rand()&0xffffff;
      m68kPerformRte();  //don't need to check interrupts because sr won't actually have changed
#if defined(SSE_STATS)
      Stats.nXbios++;
#endif
#endif
    }
    break;
#endif
  default:
    break;
  }//sw
}


#ifndef ONEGAME

MEM_ADDRESS get_TOS_address(char* const File) {
  MEM_ADDRESS ad=0;
  FILE *fp=(File[0]=='\0') ? NULL :fopen(File,"rb");
  if(fp)
  {
    BYTE HiHi=0,LoHi=0,HiLo=0,LoLo=0;
    FREAD(&HiLo,1,1,fp);
    FREAD(&LoLo,1,1,fp);
    if(HiLo==0x60&&LoLo==0x06) // Pre-tos machines, need boot disk, no header
      ad=0xFC0000; // hardcoded in GLUE
    else // read TOS header
    {
      FSEEK(fp,8,SEEK_SET);
      FREAD(&HiHi,1,1,fp);
      FREAD(&LoHi,1,1,fp);
      FREAD(&HiLo,1,1,fp);
      FREAD(&LoLo,1,1,fp);
      MEM_ADDRESS new_rom_addr=MAKELONG(MAKEWORD(LoLo,HiLo),MAKEWORD(LoHi,HiHi))&0xffffff;
      if(LoHi==0xFC||LoHi==0xE0)
        ad=new_rom_addr;
    }
    fclose(fp);
  }
  return ad;
}


#if defined(SSE_GUI_INSTANTCHANGE)

bool load_TOS(char* const File) { // true: succeeded
  if(File[0]=='\0') 
    return false;
  MEM_ADDRESS new_rom_addr=get_TOS_address(File);
  FILE *fp=fopen(File,"rb");
  if(fp==NULL) // for example loading alien snapshot
    return false;
  DWORD Len=((DWORD)GetFileLength(fp)/1024)*1024;
  BYTE *newRom=new BYTE[Len];
  DWORD checksum=~0u;
  FREAD(newRom,1,Len,fp);
  add_to_crc32(checksum,newRom,Len);
  checksum=~checksum;
#ifndef BIG_ENDIAN_PROCESSOR
  FSEEK(fp,0,SEEK_SET);
  BYTE *newRom_End_minus_1=newRom+Len-1;
  for(DWORD m=0;m<Len;m++) 
    *(BYTE*)(newRom_End_minus_1-m)=(BYTE)FGETC(fp); // slow but memory is reversed on little-endian PC
#endif
  fclose(fp);
  BYTE *oldRom=STRom;
  // would need critical section if running...
  STRom=newRom;
  tos_len=Len;
  tos_high=(new_rom_addr==0xfc0000); // $FC0000 is higher than $E00000
  rom_addr=new_rom_addr;
  rom_addr_end=rom_addr+tos_len;
#ifndef BIG_ENDIAN_PROCESSOR
  Rom_End=STRom+tos_len;
  Rom_End_minus_1=newRom_End_minus_1; // already computed!
  Rom_End_minus_2=Rom_End-2;
  Rom_End_minus_4=Rom_End-4;
#endif
  if(oldRom)
    delete[] oldRom;
  tos_version=ROM_DPEEK(2);
  SSEConfig.TosLanguage=ROM_PEEK(0x1D);
  //SSEConfig.SwitchSTModel(ST_MODEL); // to adapt CPU clock
  TRACE2("%s %s v%x/%d %s %08X\n","Load",CHECKPATH(File),tos_version,
    SSEConfig.TosLanguage,"CRC32",checksum);
/*  If we don't need ACSI hard drive emulation, neutralise TOS check for
    a faster boot (from Hatari)*/
  if(OPTION_HACKS && (tos_version==0x106||tos_version==0x162) && !pasti_active  && !ACSI_EMU_ON)
  {
    TRACE_INIT("STE tos boot patch %X %X %X\n",0x576,ROM_LPEEK(0x576),0x4E714E71);
    ROM_LPEEK(0x576)=0x4E714E71; // bsr +$e4 -> nop, "dma boot"
  }
  // nuke hd spinup delay of T205, on T206 there's a time bar so it's not so useful
  MEM_ADDRESS ta=0x0007A0;
  if(OPTION_HACKS && tos_version==0x205 && ROM_DPEEK(ta)==0x631E)
  {
    TRACE_INIT("MSTE tos boot patch %X %X %X\n",ta,ROM_DPEEK(ta),0x631E);
    ROM_DPEEK(ta)=0x601E; 
  }
  Glue.Restore(); // for Decode before Power On so that IR is correct!
#if defined(SSE_GEM_CONTROL_PANEL)
  // find system variables for key repeat
  for(DWORD m=0;m<Len-8;m+=4) 
  {
    if(ROM_LPEEK(m)==0x1B7C000F)
    {
      if(ROM_LPEEK(m+6)==0x1B7C0002)
      {
        OptionBox.TosKeyRepeat=ROM_DPEEK(m+4)&0xFFFF;
        TRACE_INIT("TOS key repeat address: $%X\n",OptionBox.TosKeyRepeat);
      }
    }
  }
#endif
  return true;
}


#else//#if defined(SSE_GUI_INSTANTCHANGE)

bool load_TOS(char* const File) { // true: succeeded
  if(File[0]=='\0') 
    return false;
  MEM_ADDRESS new_rom_addr=get_TOS_address(File);
  FILE *fp=fopen(File,"rb");
  if(fp==NULL) // for example loading alien snapshot
    return false;
  DWORD Len=((DWORD)GetFileLength(fp)/1024)*1024;
  //ASSERT(STRom || !tos_len);
  if(Len!=tos_len || !STRom)
  {
    if(STRom)
      delete[] STRom;
    STRom=new BYTE[Len];
    tos_len=Len;
  }
  tos_high=(new_rom_addr==0xfc0000); // $FC0000 is higher than $E00000
  rom_addr=new_rom_addr;
  rom_addr_end=rom_addr+tos_len;
#ifndef BIG_ENDIAN_PROCESSOR
  Rom_End=STRom+tos_len;
  Rom_End_minus_1=Rom_End-1;
  Rom_End_minus_2=Rom_End-2;
  Rom_End_minus_4=Rom_End-4;
#endif
  DWORD checksum=~0u;
  FREAD(STRom,1,Len,fp);
  add_to_crc32(checksum,STRom,Len);
  checksum=~checksum;
  FSEEK(fp,0,SEEK_SET);
  memset(STRom,0xff,Len);
  for(DWORD m=0;m<Len;m++) 
  {
    ROM_PEEK(m)=(BYTE)FGETC(fp); // slow but memory is reversed on little-endian PC
  }
  fclose(fp);
  tos_version=ROM_DPEEK(2);
  SSEConfig.TosLanguage=ROM_PEEK(0x1D);
  SSEConfig.SwitchSTModel(ST_MODEL); // to adapt CPU clock
  TRACE2("%s %s v%x/%d %s %08X\n","Load",CHECKPATH(File),tos_version,
    SSEConfig.TosLanguage,"CRC32",checksum);
/*  If we don't need ACSI hard drive emulation, neutralise TOS check for
    a faster boot (from Hatari)*/
  if(OPTION_HACKS && (tos_version==0x106||tos_version==0x162) && !pasti_active  && !ACSI_EMU_ON)
  {
    TRACE_INIT("STE tos boot patch %X %X %X\n",0x576,ROM_LPEEK(0x576),0x4E714E71);
    ROM_LPEEK(0x576)=0x4E714E71; // bsr +$e4 -> nop, "dma boot"
  }
  // nuke hd spinup delay of T205, on T206, there's a time bar so it's not useful
  MEM_ADDRESS ta=0x0007A0;
  if(OPTION_HACKS && tos_version==0x205 && ROM_DPEEK(ta)==0x631E)
  {
    TRACE_INIT("MSTE tos boot patch %X %X %X\n",ta,ROM_DPEEK(ta),0x631E);
    ROM_DPEEK(ta)=0x601E; 
  }
  Glue.Restore(); // for Decode before Power On so that IR is correct!
#if defined(SSE_GEM_CONTROL_PANEL)
  // find system variables for key repeat
  for(DWORD m=0;m<Len-8;m+=4) 
  {
    if(ROM_LPEEK(m)==0x1B7C000F)
    {
      if(ROM_LPEEK(m+6)==0x1B7C0002)
      {
        OptionBox.TosKeyRepeat=ROM_DPEEK(m+4)&0xFFFF;
        TRACE_INIT("TOS key repeat address: $%X\n",OptionBox.TosKeyRepeat);
      }
    }
  }
#endif
  return true;
}

#endif//#if defined(SSE_GUI_INSTANTCHANGE)

#else //ONEGAME

bool load_TOS(char *) {
  tos_len=192*1024;
  tos_high=true;
  rom_addr=0xFC0000;
  Rom_End=STRom+tos_len;
  Rom_End_minus_1=Rom_End-1;
  Rom_End_minus_2=Rom_End-2;
  Rom_End_minus_4=Rom_End-4;
  tos_version=0x0102;
  return true;
}

#endif


void GetTOSKeyTableAddresses(MEM_ADDRESS* const lpUnshiftTable,MEM_ADDRESS* const lpShiftTable) {
  MEM_ADDRESS addr=0;
  while(addr<tos_len)
  {
    if(ROM_PEEK(addr++)=='u')
    {
      if(ROM_PEEK(addr)=='i')
      {
        addr++;
        if(ROM_PEEK(addr)=='o')
        {
          addr++;
          if(ROM_PEEK(addr)=='p')
          {
            *lpUnshiftTable=addr-25;
            break;
          }
        }
      }
    }
  }
  addr=(*lpUnshiftTable)+127;
  while(addr<tos_len)
  {
    if(ROM_PEEK(addr++)==27)
    {
      *lpShiftTable=addr-2;
      break;
    }
  }
}


// check that file exists, if not open file selector
void TTos::UpdateTOSPath(EasyStr* const pPath) {
  ASSERT(pPath);
  if(pPath==NULL)
    return;
  if(!Exists(pPath->Text))
  {
#ifdef WIN32
    char *fstypes=FSTypes(3,NULL);
#if defined(SSE_VID_2SCREENS)
    // Windows deduces screen from handle
    *pPath=FileSelect(NotifyWin,T("Select TOS Image"),UsersPath.Text,fstypes,1,
      true,"img",pPath->Text);
#else
    *pPath=FileSelect(NULL,T("Select TOS Image"),UsersPath.Text,fstypes,1,
      true,"img",pPath->Text);
#endif
    free(fstypes);
#endif//WIN32
#ifdef UNIX
    fileselect.set_corner_icon(&Ico16,ICO16_CHIP);
    *pPath=fileselect.choose(XD,UsersPath,"",T("Select TOS Image"),FSM_LOAD|FSM_LOADMUSTEXIST,
                            romfile_parse_routine,".img");
#endif
  }
}


#undef LOGSECTION
#undef SV_MEMTOP
#undef SV_SCREENPT
#undef SV_V_BAS_AD
#undef BCONOUT
#undef GETBPB
#undef MEDIACH
#undef DRVMAP
#undef RANDOM
#undef GETTIME
#undef VSYNC
#if defined(SSE_GEM_CONTROL_PANEL)
#undef KBRATE
#endif


#if defined(SSE_DEBUG_SYMBOLS) // debugger-only

#define LOGSECTION LOGSECTION_HARDDRIVE // by default

TTosSymbol TosSymbol[MAX_SYMBOLS];

#ifdef SSE_420R8
SHORT NextTosSymbol=0;
#else
BYTE NextTosSymbol=0;
#endif

TTos::TTos() {
  fpPrgCopy=NULL;
  Reset();
}


TTos::~TTos() {
  Reset();
}


void TTos::Reset() {
  if(fpPrgCopy)
    FCLOSE(fpPrgCopy);
  memset(this,0,sizeof(TTos));
  for(int i=0;i<MAX_SYMBOLS;i++)
    TosSymbol[i].ad=0xffffffff;
  NextTosSymbol=0;
}


// TTos::GetSymbols subfunction
void GetSymbolsSub(MEM_ADDRESS textbase,MEM_ADDRESS ad,MEM_ADDRESS fixup_ad) {
  ad=SafeLPeek(fixup_ad+textbase); //relocate this address
  TRACE_LOG2("fixup $%X at $%X was $%X\n",ad,fixup_ad+textbase,ad-textbase);
  for(int i=0;i<MAX_SYMBOLS&&TosSymbol[i].ad!=0xFFFFFFFF;i++) // check all symbols
  {
    if(ad-textbase==TosSymbol[i].value2)
    {
      // TOS fixup is always based on TEXT, even for DATA and BSS addresses
      // there's no segment information in the fixup segment, only in optional symbols
      // So we don't need to check the symbol type
      TTosSymbol& x=TosSymbol[i];
      x.ad=ad; // record absolute address of symbol
      TRACE_LOG2("#%d %s $%X $%X $%X at %X\n",i,x.name,x.type,x.value1,x.value2,x.ad);
      break;
    }
  }//nxt i
}


// update boiler symbols using file pointer to GEMDOS file or temp copy
// of file in disk image
bool TTos::GetSymbols(FILE* fpPrg, MEM_ADDRESS const basepage) {
//  TRACE("GetSymbols basepage=%X\n",basepage);
  TraceBasepage(basepage);
  bool ok=false;
  INT64 oldpos=FTELL(fpPrg);
  Basepage=basepage;
  FSEEK(fpPrg,0,SEEK_SET);
  WORD PRG_magic=STfileReadWord(fpPrg);
  if(PRG_magic==0x601a) //executable
    ok=true;
  FSEEK(fpPrg,0x1C,SEEK_SET); //seek to end of header (TEXT Segment)
  ASSERT((basepage+0x100UL+PRG_tsize+PRG_dsize+PRG_bsize)<=SafeLPeek(basepage+0x4)); //basepage+4 contains hi-tpa
  MEM_ADDRESS textbase=basepage+0x100,ad=textbase; //start of text area
  //TRACE("PRG_tsize %d PRG_dsize %d PRG_bsize %d PRG_ssize %d\n",PRG_tsize,PRG_dsize,PRG_bsize,PRG_ssize);
  FSEEK(fpPrg,PRG_tsize,SEEK_CUR); //skip text segment
  ad+=PRG_tsize;
  MEM_ADDRESS ad_of_data=ad;
  FSEEK(fpPrg,PRG_dsize,SEEK_CUR); //skip data segment
  ad+=PRG_dsize;
  //TRACE("TEXT %X DATA %X BSS %X\n",textbase,ad_of_data,ad);
  if(ok && PRG_ssize && (PRG_ssize%14)==0) // TOS symbols present
  {
    int nParts=PRG_ssize/14;
    int ctr=0;
    for(int i=0;i<nParts;i++) // load symbols
    {
      TTosSymbol x;
      memset(&x,0,sizeof(x));
      FREAD(&x.name,8,1,fpPrg);
      FREAD(&x.type,6,1,fpPrg); // risky?
      SWAP_BIG_ENDIAN_WORD(x.type);
#ifdef SSE_420R8
      if(x.type&0x48) // long GST symbol (well, now it's tested and it was buggy!)
      {
        FREAD(&x.name[8],14,1,fpPrg);
        i++;
      }
#else
      if(x.type&0x48) // long GST symbol (untested)
        FREAD(&x.name+8,14,1,fpPrg);
#endif
      SWAP_BIG_ENDIAN_WORD(x.value1);
      SWAP_BIG_ENDIAN_WORD(x.value2);
      x.ad=x.value2+ad_of_data;
      TRACE("#%d %s $%X $%X $%X at %X\n",ctr,x.name,x.type,x.value1,x.value2,x.ad);
      TosSymbol[NextTosSymbol++]=x; // memorize
#ifdef SSE_420R8 // no wrapping anymore
      if(NextTosSymbol>=MAX_SYMBOLS)
        NextTosSymbol=0;
#endif
      ctr++;
    }
    ASSERT(ctr<MAX_SYMBOLS);
    TRACE("%d SYMBOLS in %d bytes\n",ctr,Tos.PRG_ssize);
  }
  // we don't really do fixups but we need to update the symbol addresses
  LONG fixup_ad=STfileReadLong(fpPrg);
  TRACE("FIXUP %X\n",fixup_ad);
  if(ok && fixup_ad && !STfileReadError)
  {
    GetSymbolsSub(textbase,ad,fixup_ad);
    BYTE b;
    for(bool bStop=false;!bStop&&ok;)
    {
      if(!FREAD(&b,1,1,fpPrg))
      {
        TRACE("%s\n","ERROR");
        ok=false;
      }
      else if(b==0)
        bStop=true;
      else if(b==1)
        fixup_ad+=254;
      else
      {
        if(b&1)
        {
          TRACE("%d %s\n",b,"ERROR");
          ok=false;
        }
        fixup_ad+=b;
        GetSymbolsSub(textbase,ad,fixup_ad);
      }
    }//nxt
  }
  FSEEK(fpPrg,oldpos,SEEK_SET); // be polite
  return ok; // not used
}


void HandlePrgCopy() {
  MEM_ADDRESS basepage=pc-0x100; // or (SP+4)
  Tos.GetSymbols(Tos.fpPrgCopy,basepage);
  FCLOSE(Tos.fpPrgCopy);
  Tos.fpPrgCopy=NULL;
  Tos.TrackSymbols=0;
}


void TTos::TraceBasepage(MEM_ADDRESS const bp) {
  // dump basepage info

/*The GEMDOS BASEPAGE structure has the following members:
Name Offset Meaning
p_lowtpa 0x00 This LONG contains a pointer to the Transient
Program Area (TPA).
p_hitpa 0x04 This LONG contains a pointer to the top of the
TPA + 1.
p_tbase 0x08 This LONG contains a pointer to the base of
the text segment
p_tlen 0x0C This LONG contains the length of the text
segment.
p_dbase 0x10 This LONG contains a pointer to the base of
the data segment.
p_dlen 0x14 This LONG contains the length of the data
segment.
p_bbase 0x18 This LONG contains a pointer to the base of
the BSS segment.
p_blen 0x1C This LONG contains the length of the BSS
segment.
p_dta 0x20 This LONG contains a pointer to the
processes’ DTA.
p_parent 0x24 This LONG contains a pointer to the
processes’ parent’s basepage.
p_reserved 0x28 This LONG is currently unused and is
reserved.
p_env 0x2C This LONG contains a pointer to the
processes’ environment string.
p_undef 0x30 This area contains 80 unused, reserved bytes.
p_cmdlin 0x80 This area contains a copy of the 128 byte
command line image.
*/
  TRACE("Basepage %06X p_lowtpa %06X p_hitpa %06X p_tbase %06X p_tlen %06X p_dbase %06X p_dlen %06X\n",
    bp,SafeLPeek(bp),SafeLPeek(bp+4),SafeLPeek(bp+8),SafeLPeek(bp+12),SafeLPeek(bp+16),SafeLPeek(bp+20));
  TRACE("p_bbase %06X p_blen %06X p_dta %06X p_parent %06X p_env %06X\n",
    SafeLPeek(bp+24),SafeLPeek(bp+28),SafeLPeek(bp+32),SafeLPeek(bp+36),SafeLPeek(bp+40));
#if defined(SSE_420R9)
  EasyStr cmdline=ReadStringFromMemory(bp+0x80+1,128); // need to read memory backwards (of course)
  TRACE("p_cmdlin %s\n",cmdline.Text);
#else
  TRACE("p_cmdlin %s\n",SafeLPeek(bp+0x80)); // vsnprintf crash program in case %s is set with an integer
#endif
}

#endif//#if defined(SSE_DEBUG_SYMBOLS)


///////////////
// Cartridge //
///////////////

bool load_cart(char* const filename) { // true: succeeded (402R10)
/*  Loading a ROM cartridge.
    Steem original format STC has 4 null bytes at the start, then the 128 KB
    of the cartridge.
    Now we accept files where there are no extra null bytes
    We also accept 64KB cartridges (like test kits).
    We recognise the MV16 and RP16 sound cartridges (our custom, fake dumps, condition SSE_SOUND_CARTRIDGE).
    We can recognise the fake Cubase 2 cartridge (our custom, fake dump, condition SSE_DONGLE_CUBASE2).
    We recognise the fake Cubase 3 cartridge (our custom, fake dump, condition SSE_DONGLE_CUBASE3).
*/
  bool failed=false;
#if defined(SSE_CARTRIDGE_ACTIVE)
  CartridgeCheck=NULL;
#endif
  SSEConfig.mv16=SSEConfig.mr16=false;
#if defined(SSE_DONGLE_CUBASE2)
  SSEConfig.Cubase2Cart=false;
#endif
#if defined(SSE_DONGLE_CUBASE3)
  SSEConfig.Cubase3Cart=false;
#endif
  FILE *fp=fopen(filename,"rb");
  if(fp==NULL)
    failed=true;
  else
  {
    LONG const FileLen=(LONG)GetFileLength(fp); //can be 64KB, 128KB, 128KB+4bytes
    LONG Len=FileLen;
    DWORD FirstBytes;
    int offset=0;
    switch(FileLen) {
    case 64*1024:
      offset=FileLen;
      break;
    case 128*1024:
      break;
    case 128*1024+4: // legacy Steem format
      Len-=4;
      FREAD(&FirstBytes,4,1,fp);
      if(FirstBytes) // must be 0, don't ask why
        failed=true;
      break;
    default:
      TRACE3("??? Cartridge file size %d bytes\n",FileLen);
      failed=true;
    }
    if(!failed)
    {
      FREAD(&FirstBytes,4,1,fp);
      SWAP_LITTLE_ENDIAN_DWORD(FirstBytes);
      if(0) {}
#if defined(SSE_SOUND_CARTRIDGE)
      else if(FirstBytes==0x3631564D) // "MV16"
        SSEConfig.mv16=true;
      else if(FirstBytes==0x3631524D) // "MR16"
        SSEConfig.mv16=SSEConfig.mr16=true;
#endif
#if defined(SSE_DONGLE_CUBASE2)
      else if(FirstBytes==0x32425543) // "CUB2" => check on every UDS
        SSEConfig.Cubase2Cart=true;
#endif
#if defined(SSE_DONGLE_CUBASE3)
      else if(FirstBytes==0x33425543) // "CUB3" => check on every cartridge read
        SSEConfig.Cubase3Cart=true;
#endif
      if(cart_save)
        cart=cart_save;
      cart_save=NULL;
#if defined(SSE_CARTRIDGE_ACTIVE2)
      if(cart)
        VirtualFree(cart,0,MEM_RELEASE);
      cart=(BYTE*)VirtualAlloc(NULL,128*1024,MEM_COMMIT | MEM_RESERVE,PAGE_EXECUTE_READWRITE);
#else
      if(cart)
        delete[] cart;
      cart=new BYTE[128*1024]; // even if cartridge is smaller
#endif
      memset(cart,0xFF,128*1024);
      FSEEK(fp,-4,SEEK_CUR); //hehe
      for(int bn=Len-1;bn>=0;bn--)
        FREAD(cart+bn+offset,1,1,fp); // backwards
#if defined(SSE_CARTRIDGE_ACTIVE2) // executable code inside stc file!
      if(0
#if defined(SSE_DONGLE_CUBASE2)
        ||SSEConfig.Cubase2Cart
#endif
#if defined(SSE_DONGLE_CUBASE3)
        ||SSEConfig.Cubase3Cart
#endif
        )
      {
        int ProcOffset,ProcLen;
#ifdef UNIX
        FSEEK(fp,0x20+SSE_BITNESS,SEEK_SET); // one cartridge can be used by all builds
#else
        FSEEK(fp,SSE_BITNESS,SEEK_SET);
#endif
        FREAD(&ProcOffset,1,4,fp);
        SWAP_BIG_ENDIAN_DWORD(ProcOffset);
        FREAD(&ProcLen,1,4,fp);
        SWAP_BIG_ENDIAN_DWORD(ProcLen);
        FSEEK(fp,ProcOffset,SEEK_SET);
        FREAD(cart+ProcOffset,1,ProcLen,fp); // forwards
        CartridgeCheck=(WORD(*)(DWORD ad,BYTE *rpin))(cart+ProcOffset);
      }
#endif
#if defined(SSE_DONGLE_CUBASE2_BUILD)
      if(SSEConfig.Cubase2Cart)
        CartridgeCheck=&CartridgeCheckCubase2;
#endif
#if defined(SSE_DONGLE_CUBASE3_BUILD)
      if(SSEConfig.Cubase3Cart)
        CartridgeCheck=&CartridgeCheckCubase3;
#endif
#ifndef BIG_ENDIAN_PROCESSOR
      Cart_End_minus_1=cart+(128*1024-1);
      Cart_End_minus_2=Cart_End_minus_1-1;
      Cart_End_minus_4=Cart_End_minus_1-3;
#endif
      fclose(fp);
      DWORD crc32=GetCRCFromFile(filename);
      TRACE2("%s %s %s %X\n","Load",CHECKPATH(filename),"CRC32",crc32);
    }
  }
  return !failed;
}


#if defined(SSE_CARTRIDGE_ACTIVE)
BYTE CartridgeData[CARTRIDGE_DATA_SIZE];
WORD (*CartridgeCheck)(DWORD ad,BYTE *rpin);
#endif
