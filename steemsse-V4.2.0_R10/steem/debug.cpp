/*---------------------------------------------------------------------------
PROJECT: Steem SSE
Atari ST emulator
Copyright (C) 2025 by Anthony Hayward and Russel Hayward + SSE

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

FILE: debug.cpp
DESCRIPTION: General debugging facilities. This is meant to debug Steem
itself as well as ST programs. This is not the Debugger, see debugger.cpp etc.
for that.
TRACE function (file or IDE), OSD message.
Some debug facilities are used in the Regular Steem as well as in the
Debugger.
The beta builds can use TRACE_LOG, the sections to trace are defined here.
With the debug builds, sections to trace are commanded in the Debugger.
---------------------------------------------------------------------------*/

#include <pch.h> 
#pragma hdrstop

#include <debug.h>
#include <stdarg.h>
#include <gui.h>
#include <harddiskman.h>
#include <draw.h>
#include <display.h>
#include <computer.h>
#include <osd.h>
#include <infobox.h>
#include <diskman.h>
#include <key_table.h>
#include <stjoy.h>
#ifdef WIN32
#include <time.h>
#endif
#if defined(SSE_DEBUGGER_FRAME_REPORT)
#include <debug_framereport.h>
#endif
#ifdef DEBUG_BUILD
#include <iolist.h>
#include <debugger.h>
#endif

#if defined(SSE_DEBUG)
int debug0,debug1,debug2,debug3,debug4,debug5,debug6,debug7,debug8,debug9;
#if defined(SSE_HD6301_LL)
extern "C" void (*hd6301_trace)(char *fmt,...);
#endif
#endif

TDebug Debug; // singleton, present in all builds

EasyStr sTraceFile;

TDebug::TDebug() {
  ZeroMemory(this,sizeof(TDebug));
#if defined(SSE_ENABLE_TRACE_LOG)
  ZeroMemory(logsection_enabled,100*sizeof(bool));
  logsection_enabled[LOGSECTION_ALWAYS]=1;
  logsection_enabled[LOGSECTION_FDC]=0;
  logsection_enabled[LOGSECTION_IO]=0;
  logsection_enabled[LOGSECTION_MFP_TIMERS]=0;
  logsection_enabled[LOGSECTION_INIT]=0;
  logsection_enabled[LOGSECTION_CRASH]=0;
  logsection_enabled[LOGSECTION_HARDDRIVE]=0;
  logsection_enabled[LOGSECTION_IKBD]=0;
  logsection_enabled[LOGSECTION_AGENDA]=0;
  logsection_enabled[LOGSECTION_INTERRUPTS]=0;
  logsection_enabled[LOGSECTION_TRAP]=0;
  logsection_enabled[LOGSECTION_SOUND]=0;
  logsection_enabled[LOGSECTION_GLUE]=0;
  logsection_enabled[LOGSECTION_BLITTER]=0;
  logsection_enabled[LOGSECTION_PORTS]=0;
  logsection_enabled[LOGSECTION_TRACE]=0;
  logsection_enabled[LOGSECTION_CPU]=0;
  logsection_enabled[LOGSECTION_VIDEO_RENDERING]=0;
  //logsection_enabled[LOGSECTION_OPTIONS]=0;
  logsection_enabled[LOGSECTION_IMAGE_INFO]=0;

#ifdef SSE_BETA
  logsection_enabled[LOGSECTION_IMAGE_INFO]=0;
#endif

#endif//#if defined(SSE_ENABLE_TRACE_LOG)
#ifdef WIN32  
  //SetCurrentDirectory(UsersPath.Text); // bad code, we're in a constructor
#endif
#if defined(SSE_DEBUG) && defined(SSE_HD6301_LL)
  hd6301_trace=&TDebug::TraceLog;
#endif
}


TDebug::~TDebug() {
  if(trace_file_pointer)
  {
    TRACE("End\n");
#if defined(SSE_DEBUG_TRACE_LOCK)
    CloseHandle(trace_file_pointer);
#else
    fclose(trace_file_pointer);
#endif
  }
}


void TDebug::Vbl() { 
#if defined(SSE_DEBUGGER_FRAME_REPORT)
  FrameEvents.Vbl(); 
#endif
#if defined(SSE_DEBUG)
  if(ShifterTricks)
  {
#if defined(SSE_DEBUGGER_FAKE_IO)
    if(OSD_MASK2 & OSD_CONTROL_SHIFTERTRICKS)
      TRACE_OSD("T%X",ShifterTricks);
#endif
#if defined(SSE_OSD_FPS_INFO) // can also give VRAM size
    if(vcount_at_vsync-vbase_at_vbi>32000)
      TRACE_VID("F%d tricks %x %d bytes\n",FRAME,ShifterTricks,vcount_at_vsync-vbase_at_vbi);
    else
#endif
    TRACE_VID("F%d tricks %x\n",FRAME,ShifterTricks);
  }
#endif
  if(!border && (OPTION_VLE>=1) && (!ShifterTricks^noShifterTricks))
  {
    UPDATE_STATUS_BAR_PART(SB_PART_ICONS);
  }
  ShifterTricks=0;
#if defined(SSE_DEBUGGER_FAKE_IO)
/*  This system so that we only report these once per frame, giving
    convenient info about VBI, HBI, and MFP IRQ.
*/
  if((OSD_MASK1 & OSD_CONTROL_INTERRUPT)&&FrameInterrupts)
  {
    char buf1[40]="",buf2[4];
    if(FrameInterrupts&2)
      strcat(buf1,"V");
    if(FrameInterrupts&1)
      strcat(buf1,"H");
    if(FrameMfpIrqs)
    {
      strcat(buf1," MFP ");
      for(int i=15;i>=0;i--)
      {
        if(FrameMfpIrqs&(1<<i))
        {
          sprintf(buf2,"%d ",i);
          strcat(buf1,buf2);
        }
      }
    }
    TRACE_OSD(buf1);
  }
  FrameInterrupts=0;
  FrameMfpIrqs=0;
#endif  
#if defined(SSE_OSD_FPS_INFO)
/*  cases where it doesn't work
    Pacmania STE: VBASE ignored by game
    Pro Tennis Simulator intro: screen cleaned up after DE
*/
  if(OPTION_OSD_FPSINFO) // will the load kill our fps?
  {
    DWORD checksum=0;
    DWORD max=MIN(vcount_at_vsync,mem_len);
    for(DWORD x=vbase_at_vbi;x<max;x+=4)
      checksum+=LPEEK(x);
    if(checksum==frame_checksum)
      frame_no_change++;
    frame_checksum=checksum;
  }
#endif
  ASSERT(!(sys_timer&1));
#ifdef SSE_DEBUG
  nHbis=0;
#endif
  Shifter.nVbl++;
#if defined(SSE_STATS)
  Stats.nFrame++;

#ifdef SSE_BETA
  //TRACE_OSD("%d",A_S_T); // check timer wrap
//for tests only! - or future teaser free version until you pay big $ to register?? ;)
#if 0
  if(Stats.nFrame==30*60 && Cpu.ProcessingState!=TMC68000::HALTED)
  {
    reset_st(RESET_COLD|RESET_NOSTOP|RESET_CHANGESETTINGS|RESET_BACKUP);
  }
#endif
#endif

#endif
#if defined(SSE_DEBUGGER_FRAME_REPORT)
  if(FRAME_REPORT_MASK1 & FRAME_REPORT_MASK_VC_LINES)
    FrameEvents.Add(scan_y,0,"VB",vbase);
  if(FRAME_REPORT_MASK1 & FRAME_REPORT_MASK_SHIFTMODE)
    FrameEvents.Add(scan_y,0,"R=",Shifter.ShiftMode);
  if(FRAME_REPORT_MASK1 & FRAME_REPORT_MASK_SYNCMODE)
    FrameEvents.Add(scan_y,0,"S=",Glue.SyncMode);
#endif
}


#if defined(SSE_DEBUG_TRACE_LOCK)
#define BAD_VALUE INVALID_HANDLE_VALUE
#else
#define BAD_VALUE NULL
#endif

void TDebug::TraceInit() {
  trace_file_pointer=BAD_VALUE;
  EasyStr trace_file=SSE_TRACE_FILE_NAME;
  // file could be already open, in that case we create TRACE(1).txt or ... instead
  for(int i=1;trace_file_pointer==BAD_VALUE && i<100;i++)
  {
    EasyStr trace_path=UsersPath+SLASH+trace_file;
#if defined(SSE_DEBUG_TRACE_LOCK)
    // fopen doesn't check or lock the file, must use OS
    trace_file_pointer=CreateFile(trace_path.Text,GENERIC_WRITE,FILE_SHARE_READ,
                                  NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
#else
    trace_file_pointer=fopen(trace_path,"w");
#endif
    if(trace_file_pointer==BAD_VALUE)
    {
      GetNewFileName(trace_file); // new function!
#if defined(SSE_STATS)
      GetNewFileName(sStatsFile); // shrewd!
#endif
    }
    else
      sTraceFile=trace_file;
  }//nxt
  TraceGeneralInfos(INIT0);
}

#undef BAD_VALUE

void TDebug::Reset(bool Cold) {
  TRACE_INIT(PRICV " %s %s\n",A_S_T,(Cold?"Cold":"Warm"),"Reset");
  if(Cold)
  {
#if defined(SSE_BETA)
    IgnoreErrors=0;
#endif
#if defined(SSE_OSD_SHOW_TIME)
    OsdControl.StartingTime=timeGetTime();
    OsdControl.StoppingTime=0;
#endif
  }
  else if(runstate==RUNSTATE_RUNNING)
  {
#if defined(SSE_OSD_DEBUGINFO)
    OsdControl.Trace(TOsdControl::OUTPUT_SB,"Reset");
#endif
  }
#ifdef SSE_DEBUG
  ShifterTricks=0;
#endif
}


#if defined(SSE_DEBUG_TRACE)

void TDebug::Trace(char *fmt,...) {
  // Our TRACE facility has no MFC dependency.
  va_list body;	
  va_start(body, fmt);
#if defined(SSE_UNIX)
  int nchars=vsnprintf(trace_buffer,MAX_TRACE_CHARS,fmt,body); // check for overrun 
#else
#if !defined(SSE_DEBUG)
  _vsnprintf(trace_buffer,MAX_TRACE_CHARS,fmt,body); // check for overrun 
#else
  int nchars=_vsnprintf(trace_buffer,MAX_TRACE_CHARS,fmt,body); // check for overrun 
#endif
#endif
  va_end(body);	
#ifdef SSE_DEBUG
  if(nchars==-1)
    strcpy(trace_buffer,"TRACE buffer overrun\n");
#endif
#ifdef WIN32
#if defined(SSE_DEBUG_TRACE_IDE)
  OutputDebugString(trace_buffer);
#endif
#endif
#if defined(SSE_UNIX_TRACE)
  if(!SSEConfig.TraceFile)  
    fprintf(stderr,trace_buffer);
#endif 
  if(trace_file_pointer && trace_buffer)
  {
#if defined(SSE_DEBUG_TRACE_LOCK) && defined(WIN32)
    // our traces with only \n should be translated to Windows format
    // not very good for performance
    // we can't put \r\n everywhere because of the Linux build
    char neo_buffer[1024];
    DWORD n=0;
#if defined(SSE_420R5) // opt
      char *src=trace_buffer;
      while(*src)
      {
        if(*src=='\n')
          neo_buffer[n++]='\r';
        neo_buffer[n++]=*src++;
      }
#else
    for(int i=0;trace_buffer[i]!='\0';i++)
    {
      if(trace_buffer[i]=='\n')
        neo_buffer[n++]='\r';
      neo_buffer[n++]=trace_buffer[i];
    }
#endif
    WriteFile(trace_file_pointer,neo_buffer,n,&n,NULL);
#else
    //printf("%s",trace_buffer);
    fprintf(trace_file_pointer,"%s",trace_buffer);
#endif
#if defined(SSE_DEBUG)
    nTrace++; 
#endif
  }
#if defined(SSE_DEBUGGER)
  if(TRACE_FILE_REWIND && nTrace>=TRACE_MAX_WRITES && trace_file_pointer)
  {
    nTrace=0;
#if defined(SSE_DEBUG_TRACE_LOCK)
    SetFilePointer(trace_file_pointer, 0, NULL, FILE_BEGIN);
    //SetEndOfFile(trace_file_pointer);
#else
    rewind(trace_file_pointer); // it doesn't erase
#endif
    TRACE("\n============\nREWIND TRACE\n============\n");
  }
#endif
#if defined(SSE_LIBRETRO)//temp
  FlushTrace();
#endif
}


#ifdef WIN32

//https://stackoverflow.com/questions/36543301/detecting-windows-10-version/36545162#36545162

//#ifdef BCC_BUILD
#ifndef VC_BUILD
typedef LONG NTSTATUS;
typedef LONG* PNTSTATUS;
typedef OSVERSIONINFOW RTL_OSVERSIONINFOW;
typedef OSVERSIONINFOW *PRTL_OSVERSIONINFOW;
#endif
#define STATUS_SUCCESS (0x00000000)

typedef NTSTATUS (WINAPI* RtlGetVersionPtr)(PRTL_OSVERSIONINFOW);

RTL_OSVERSIONINFOW GetRealOSVersion() {
    HMODULE hMod = ::GetModuleHandleW(L"ntdll.dll");
    if (hMod) {
        RtlGetVersionPtr fxPtr = (RtlGetVersionPtr)::GetProcAddress(hMod, "RtlGetVersion");
        if (fxPtr != NULL) {
            RTL_OSVERSIONINFOW rovi = { 0 };
            rovi.dwOSVersionInfoSize = sizeof(rovi);
            if ( STATUS_SUCCESS == fxPtr(&rovi) ) {
                return rovi;
            }
        }
    }
    RTL_OSVERSIONINFOW rovi = { 0 };
    return rovi;
}


//https://docs.microsoft.com/en-us/windows/win32/api/wow64apiset/nf-wow64apiset-iswow64process?redirectedfrom=MSDN
typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);

LPFN_ISWOW64PROCESS fnIsWow64Process=NULL;

BOOL IsWow64()
{
    BOOL bIsWow64 = FALSE;

    //IsWow64Process is not available on all supported versions of Windows.
    //Use GetModuleHandle to get a handle to the DLL that contains the function
    //and GetProcAddress to get a pointer to the function if available.
    HMODULE hm=GetModuleHandle(TEXT("kernel32"));
    if(hm)
      fnIsWow64Process = (LPFN_ISWOW64PROCESS) GetProcAddress(hm,"IsWow64Process");

    if(NULL != fnIsWow64Process)
    {
        if (!fnIsWow64Process(GetCurrentProcess(),&bIsWow64))
        {
            //handle error
        }
    }
    return bIsWow64;
}

#endif//WIN32

// A series of TRACE giving precious info at various times

void TDebug::TraceGeneralInfos(int when) {

  // get run date and time in strings
  char sdate[9],stime[9];
#ifdef WIN32
  _strdate( sdate );
  _strtime( stime );
#endif
#ifdef UNIX
  time_t rawtime;
  time (&rawtime);
  tm *timeinfo=localtime(&rawtime);
  strftime(sdate,9,"%D",timeinfo);
  strftime(stime,9,"%T",timeinfo);
#endif

  switch(when) {

  case INIT0: // opening of trace
    TRACE2("%s %s %s\n%s v%s (built %s) \n",SSE_TRACE_FILE_NAME,sdate,stime,gAppName,
           gAppBuildInfo,stem_version_date_text);
#ifdef BCC_BUILD
    TRACE2("BCC");
#endif
#ifdef VC_BUILD
    TRACE2("VC%d",_MSC_VER);
#endif
#ifdef MINGW_BUILD
    TRACE2("MinGW");
#endif
#ifdef UNIX
    TRACE2("GCC");
#endif
#ifdef SSE_VID_DD
    TRACE2(" DD%x",DIRECTDRAW_VERSION>>8);
#endif
#ifdef SSE_VID_D3D
    TRACE2(" %s%x","D3D",DIRECT3D_VERSION>>8);
#endif
#ifdef SSE_DRAW_C
    //TRACE2(" DrawC"); // should be all now
#endif
#ifdef SSE_LIBRETRO
    TRACE2(" DLL");
#endif
    TRACE2("\n");
#ifdef WIN32
  {
    RTL_OSVERSIONINFOW rovi=GetRealOSVersion();
    SSEConfig.WindowsVersion=rovi.dwMajorVersion|(rovi.dwMinorVersion<<8)|(rovi.dwBuildNumber<<24);
#ifdef SSE_X64
    int bits=64; // IsWow64() is FALSE!
#else
    int bits=(32<<(int)!!IsWow64());
#endif
    TRACE2("Windows v%d.%d.%d %dbit %s\n",rovi.dwMajorVersion,rovi.
           dwMinorVersion,rovi.dwBuildNumber,bits,rovi.szCSDVersion);
    //TRACE2("%s $%X\n","thread",GetCurrentThreadId());
  }
#endif
    break;

  case INIT: // when init done (but options not retrieved)
  {
    TConfigStoreFile CSF(globalINIFile);
#ifdef WIN32
#if !defined(SSE_420R4) // each DLL loading is already in the trace
    TRACE2("%s %d %s %d %s %d %s %d %s %d %s %d %s %X\n",
      UNRAR_DLL,SSEConfig.UnrarDll,
      UNZIP_DLL,SSEConfig.unzipd32Dll,
      ARCHIVEACCESS_DLL,SSEConfig.ArchiveAccess,
      SSE_DISK_CAPS_PLUGIN_FILE,SSEConfig.CapsImgLib,
      PASTI_DLL,SSEConfig.PastiDll,
      FREE_IMAGE_DLL,SSEConfig.FreeImageDll,
      VIDEO_LOGIC_DLL,SSEConfig.Stvl);
#endif
#if !defined(SSE_TRACE_DUMP_OPTIONS)
    TRACE2("OneInstance %d AutoLoadSnapShot %d RunOnStart %d StartFullscreen %d\n",
      CSF.GetInt("Main","OneInstance",0),
      CSF.GetInt("Options","AutoLoadSnapShot",0),
      CSF.GetInt("Options","RunOnStart",0),
      CSF.GetInt("Options","StartFullscreen",0));
    TRACE2("%s %d %s %d\n",
#ifdef SSE_VID_DD
      "DirectDraw",
#else
      "D3D",
#endif
      !CSF.GetInt("Options","NoDirectDraw",0),
      "DirectSound",
      !CSF.GetInt("Options","NoDirectSound",0));
#endif
#endif//WIN32
    break;
  }

#if !defined(SSE_TRACE_DUMP_OPTIONS)
  case LOAD:
#if defined(SSE_TOS_KEYBOARD_CLICK)
    TRACE2("%s lang $%X click %d %s vm%d blithide %d speed %d %s %d ",
      "KBD",KeyboardLangID,OPTION_KEYBOARD_CLICK,"Mouse",OPTION_VMMOUSE,Disp.BlitHideMouse,
      mouse_speed,"Battery",OPTION_BATTERY6301);
#else
    TRACE2("%s lang $%X %s vm%d blithide %d speed %d %s %d ",
      "KBD",KeyboardLangID,"Mouse",OPTION_VMMOUSE,Disp.BlitHideMouse,
      mouse_speed,"Battery",OPTION_BATTERY6301);
#endif
#if !defined(SSE_LIBRETRONUKE)
    if(IsJoyActive(0))
      TRACE2("J0 ");
    if(IsJoyActive(1))
      TRACE2("J1 ");
#endif
    if(DONGLE_ID)
      TRACE2("D%d",DONGLE_ID);
    TRACE2("\nHighPriority %d AllowTaskSwitch %d StartEmuOnClick %d frameskip %d DrawToVidMem %d\n",
      HighPriority,bAllowTaskSwitch,StartEmuOnClick,frameskip,Disp.DrawToVidMem);
    TRACE2("%s %d %s %d %s %d %s %d %s %d %s %d\n","Borders",border,
      "Scanlines",OPTION_SCANLINES,"Stretch",Draw.Stretch,"ST Aspect Ratio",OPTION_ST_ASPECT_RATIO,
      "WVS",OPTION_WIN_VSYNC,"AVS",OPTION_AUTOVSYNC);
    TRACE2("%s %d %s %d %s %d\n","BFI",OPTION_BFI,"Microseconds",OPTION_MICROSECONDS,
      "Timing hard loop (ms)",OPTION_TIMINGLOOP);
    TRACE2("DefConfig %d Hacks %d RTC%d FB%d BS%d\n",OPTION_ST_PRESELECT,OPTION_HACKS,
      OPTION_RTC_HACK,OPTION_FASTBLITTER,OsdControl.show_jokes);
    TRACE2("FD mfm%d dma%d td%d ff%d A%d%cB%d%cHD acsi %d gemdos %d\n",
      OPTION_AUTOSTW,OPTION_COUNT_DMA_CYCLES,DiskMan.bTurboDrive,DiskMan.floppy_access_ff,
      (DiskMan.nFloppyDrives<1?0:(FloppyDrive[DRIVE_A].bSingleSided?1:2)),
      (FloppyDrive[DRIVE_A].bFreeboot?'F':' '),
      (DiskMan.nFloppyDrives<2?0:(FloppyDrive[DRIVE_B].bSingleSided?1:2)),
      (FloppyDrive[DRIVE_B].bFreeboot?'F':' '),
      SSEOptions.Acsi,!HardDiskMan.DisableHardDrives);
    //no break;
#endif//#if !defined(SSE_TRACE_DUMP_OPTIONS)

  case RESET:
    TRACE2("%s ~%dHz %dK T%X(%d) %s C%d C%d",st_model_name[ST_MODEL],nSysCyclesPerSecond/TICKS8,
           mem_len>>10,tos_version,SSEConfig.TosLanguage,screen_type[SSEConfig.ColourMonitor],
           OPTION_C1,(OPTION_VLE?OPTION_VLE+1:0));
    if(IS_STF)
    {
      if(OPTION_HWOVERSCAN)
        TRACE2(" %s\n",overscan_dev[OPTION_HWOVERSCAN]);
      else
        TRACE2(" WU%d\n",Mmu.WS[OPTION_WS]);
    }
    else
      TRACE2("\n");
    break;

  case START:
#if defined(SSE_OSD_SHOW_TIME)
    if(OsdControl.StoppingTime)
      OsdControl.StartingTime+=timeGetTime()-OsdControl.StoppingTime;
#endif
#if defined(SSE_EMU_THREAD)
    if(OPTION_EMUTHREAD)
      TRACE2("\n%s Run %s $%X\n",stime,"thread",EmuThreadId);
    else
#endif
      TRACE2("\n%s Run\n",stime);
    break;

  case STOP:
#if defined(SSE_OSD_SHOW_TIME)
    OsdControl.StoppingTime=timeGetTime();
#endif
    TRACE2("%s Stop\n",stime);
    break;

  case EXIT:
    TRACE2("%s Leaving Steem\n",stime);
    break;

  }//sw
  FlushTrace();
}


void TDebug::FlushTrace() {
  if(trace_file_pointer)
  {
    FFLUSH(trace_file_pointer);
    if(!(OPTION_EMUTHREAD&&runstate!=RUNSTATE_STOPPED) && InfoBox.IsVisible())
    {
      if(InfoBox.Page==TGeneralInfo::TRACEFILE)
        InfoBox.CreatePage(InfoBox.Page);
    }
  }
}


// input: a path. output: same path or just the file according to option
// it's because some personal data, such as a name, can be in the path
// and you may want not to have that in a trace file you share
char* TDebug::TraceCheckPrivacy(char *string) {
  return
#if defined(SSE_420R4) // made this optional
  (SSEConfig.TraceShowPath)?string: // full path
#endif
    GetFileNameFromPath(string); // filename only
}


#if defined(SSE_ENABLE_TRACE_LOG)

void TDebug::TraceLog(char *fmt,...) { // static
  if(logsection_enabled[Debug.LogSection])
  {
    va_list body;	
    va_start(body, fmt);
#if defined(SSE_UNIX)
    int nchars=vsnprintf(Debug.trace_buffer,MAX_TRACE_CHARS,fmt,body); // check for overrun 
#else
    int nchars=_vsnprintf(Debug.trace_buffer,MAX_TRACE_CHARS,fmt,body); // check for overrun 
#endif
    va_end(body);	
    if(nchars==-1)
      strcpy(Debug.trace_buffer,"TRACE buffer overrun\n");
#ifdef WIN32
#if defined(SSE_DEBUG_TRACE_IDE)
    if(Debug.LogSection!=LOGSECTION_CPU) //there are limits!
      OutputDebugString(Debug.trace_buffer);
#endif
#endif
#if defined(SSE_UNIX_TRACE)
    if(!SSEConfig.TraceFile)
      fprintf(stderr,Debug.trace_buffer);
#endif 
    if(Debug.trace_file_pointer && Debug.trace_buffer)
    {
#if defined(SSE_DEBUG_TRACE_LOCK)
      // our traces with only \n should be translated to Windows format
      // not very good for performance
      // we can't put \r\n everywhere because of the Linux build
      char neo_buffer[1024];
      DWORD n=0;
#if defined(SSE_420R5) // opt
      char *src=Debug.trace_buffer;
      while(*src)
      {
        if(*src=='\n')
          neo_buffer[n++]='\r';
        neo_buffer[n++]=*src++;
      }
#else
      int l=(int)strlen(Debug.trace_buffer);
      for(int i=0;i<l;i++)
      {
        if(Debug.trace_buffer[i]=='\n')
          neo_buffer[n++]='\r';
        neo_buffer[n++]=Debug.trace_buffer[i];
      }
#endif
      WriteFile(Debug.trace_file_pointer,neo_buffer,n,&n,NULL);
#ifdef TEST02
      //FLUSH_TRACE; // when it's crashy
#endif
#else
      printf(Debug.trace_buffer),Debug.nTrace++; 
#endif
    }
#ifdef DEBUG_BUILD
    if(TRACE_FILE_REWIND && Debug.nTrace>=TRACE_MAX_WRITES && Debug.trace_file_pointer)
    {
      Debug.nTrace=0;
#if defined(SSE_DEBUG_TRACE_LOCK)
      SetFilePointer(Debug.trace_file_pointer, 0, NULL, FILE_BEGIN);
#else
      rewind(Debug.trace_file_pointer); // it doesn't erase
#endif
      TRACE("\n============\nREWIND TRACE\n============\n");
      Debug.TraceGeneralInfos(INIT);
    }
#endif
  }
}

#endif//#if defined(SSE_ENABLE_TRACE_LOG)

#endif//#if defined(SSE_DEBUG_TRACE)


#if defined(SSE_DEBUGGER_TRACE_EVENTS)
  // not very smart...
void TDebug::TraceEvent(void* pointer) {
  TRACE(PRICV " ",A_S_T);
  if(pointer==event_scanline)
    TRACE("event_scanline");
  else if(pointer==event_timer_a_timeout)
    TRACE("event_timer_a_timeout");
  else if(pointer==event_timer_b_timeout)
    TRACE("event_timer_b_timeout");
  else if(pointer==event_timer_c_timeout)
    TRACE("event_timer_c_timeout");
  else if(pointer==event_timer_d_timeout)
    TRACE("event_timer_d_timeout");
  else if(pointer==event_timer_b)
    TRACE("event_timer_b");
  else if(pointer==event_start_vbl)
    TRACE("event_start_vbl");
  else if(pointer==event_vbl_interrupt)
    TRACE("event_vbl_interrupt");
  else if(pointer==event_trigger_vbi)
    TRACE("event_trigger_vbi");
  else if(pointer==event_wd1772)
    TRACE("event_wd1772");
  else if(pointer==event_driveA_ip)
    TRACE("event_driveA_ip");
  else if(pointer==event_driveB_ip)
    TRACE("event_driveB_ip");
  else if(pointer==event_acia)
    TRACE("event_acia");
#if USE_PASTI
  else if(pointer==event_pasti_update)
    TRACE("event_pasti_update");
#endif
    TRACE(" (" PRICV ")\n",A_S_T-time_of_next_event);
}

#endif


#if defined(SSE_ENABLE_TRACE_LOG)
//////////////////////////////// names of OS calls //////////////////////////////////
const char* gemdos_calls[0x58]={"Pterm0","Conin","Conout(c=&)","Cauxin","Cauxout (c=&)",
  "Cprnout(c=&)","Raw con io Crawio(c=&)","Crawcin","Cnecic","Print line(text=%)",
  "ReadLine(buf=%)","Constat","","","SetDrv(drv=&)","","Conout stat","Printer status",
  "inp?(serial)","out?(serial)","","","","","","GetDrv","SetDTA(buf=%)","","","","","",
  "super(%)","","","","","","","","","","GetDate","SetDate(date=&)","Gettime",
  "Settime(time=&)","","GetDTA","Get version number","PtermRes(keepcnt=%,retcode=&)",
  "","","","","Dfree(buf=%,drive=&)","","","Mkdir(path=%=$)","Rmdir(path=%=$)",
  "Chdir(path=%)","Fcreate(fname=%=$,attr=&)","Fopen(fname=%=$,mode=&)","Fclose(handle=&)",
  "Fread(handle=&,count=%,buf=%)","Fwrite(handle=&,count=%,buf=%)","Fdelete(fname=%=$)",
  "Fseek(offset=%,handle=&,seekmode=&)","Fattrib(fname=%=$,flag=&,attrib=&)",
#if SSE_VERSION>=420
  "Mxalloc(amount=%, mode=&)",
#else
  "",
#endif
  "Fdup(handle=&)","Fforce(stdh=&,nonstdh=&)","DgetPath(buf=%,drive=&)",
  "Malloc(%)","Mfree(addr=%)","Mshrink(dummy=&,block=%,newsize=%)","Pexec(mode=&,%=$,%,%)",
  "Pterm(retcode=&)","","Fsfirst(fnam=%=$,attr=&)","Fsnext","","","","","","",
  "Frename(dummy=&,oldname=%=$,newname=%=$)","Fdatime(timeptr=%,handle=&,flag=&)"};

const char* bios_calls[12]={"GetMBP(pointer=%)","Bconstat(dev=&)","Bconin(dev=&)",
  //"Bconout(dev=&,c=#)","Rwabs(rwflag=&,buffer=%,number=&,recno=&,dev=&), read/write disk sector",
  "Bconout(dev=&,c=#)","Rwabs(mode=&,buffer=%,count=&,recno=&,dev=&), read/write disk sector",
  "Setexec(number=&,vector=%), set exception vector","Tickcal","Getbpb(dev=&)","Bcostat(dev=&)",
  "Mediach(dev=&)","Drvmap","Kbshift(mode=&)"};

const char* xbios_calls[40]={"InitMouse(type=&,parameter=%,vector=%)","Ssbrk(number=%)",
  "Physbase","Logbase","Getrez","void setscreen(log=%,phys=%,res=&)","Setpalette(ptr=%)",
  "Setcolor(colornum=&,color=&)","Floprd(buffer=%,filler=%,dev=&,sector=&,track=&,side=&,count=&)",
  "Flopwr(buffer=%,filler=%,dev=&,sector=&,track=&,side=&,count=&)",
  "Flopfmt(buffer=%,filler=%,dev=&,spt=&,track=&,side=&,interleave=&,magic=&,virgin=&)","",
  "Midiws(count=&,ptr=%)","Mfpint(number=&,vector=%)","Iorec(dev=&)",
  "Rsconf(baud=&,ctrl=&,ucr=&,rsr=&,tsr=&,scr=&)","Keytbl(unshift=%,shift=%,caps=%)",
  "random","protobt(buffer=%,serialno=%,disktype=&,execflag=&)",
  "Flopver(buffer=%,filler=%,dev=&,sector=&,track=&,side=&,count=&)","Scrdmp",
  "Cursconf(function=&,rate=&)","Settime(time=%)","Gettime","Bioskeys","Ikbdws(number=&,ptr=%)",
  "Jdisint(number=&)","Jenabint(number=&)","Giaccess(data=&,register=&)","Offgibit(bitnumber=&)",
  "Ongibit(bitnumber=&)","Xbtimer(timer=&,control=&,data=&,vector=%)","Dosound(pointer=%)",
  "Setprt(config=&)","Kbdvbase","Kbrate(delay=&,repeat=&)","Prtblk(parameter=%)",
  "Vsync","Supexec(%)","Puntaes"};

#endif


#ifdef DEBUG_BUILD

void stop_cpu_log() {
  logsection_enabled[LOGSECTION_CPU]=false;
  CheckMenuItem(logsection_menu,300+LOGSECTION_CPU, MF_BYCOMMAND|MF_UNCHECKED);
  TRACE("CPU LOG OFF\n");
}

#endif


#if defined(SSE_ENABLE_TRACE_LOG)

void log_os_call(int trap) {
#ifdef SSE_ENABLE_TRACE_LOG
  if(!SSEConfig.TraceFile||(IRD&0x4E40)!=0x4E40)
    return;
#endif
  int detail=1;
  char stmp[512];
  EasyStr l="",a="";
  LONG lpar=0;
  bool Invalid;
  MEM_ADDRESS my_sp=GetSPBeforeTrap(&Invalid);
  if(Invalid)
    return;
  MEM_ADDRESS my_spp=my_sp+2;
  unsigned int call=SafeDPeek(my_sp);
#if SSE_VERSION>=420
  l=HEXSl(old_pc,6)+"-"+HEXSl(IRD,4) +": ";

#else
  l=HEXSl(old_pc,6)+": ";
#endif
  switch(trap) {
  case TRAP_GEMDOS:
    if(call<0x58ul)
      a=(char*)gemdos_calls[call];
    l+="GEMDOS $";
    l+=HEXSl(call,3); //3 for later OS bigger numbers, but only know to $57
    break;
  case TRAP_BIOS:
    if(call<12ul)
      a=(char*)bios_calls[call];
    l+="BIOS $";
    l+=HEXSl(call,2); //1 digit would be enough
    //l+=(int)call;
    if(call==9||call==1||call==3) //Mediach + Bconstat + Bconout
      detail=2;

    //if(call==7) runstate=RUNSTATE_STOPPING;

    break;
  case TRAP_XBIOS:
    if(call<40ul)
      a=(char*)xbios_calls[call];
    l+="XBIOS $";
    l+=HEXSl(call,2); //2 digits are enough
    //l+=(int)call;
    break;
  case TRAP_GEM:
    //switch(REGL(0)) {
    switch(REGW(0)) {
    case 0x73: // VDI
    {
      /*
.data
_contrl: ds.w 12
_intin: ds.w 128
_ptsin: ds.w 256
_intout: ds.w 128
_ptsout: ds.w 256
_VDIpb: dc.l _contrl, _intin, _ptsin
dc.l _intout, _ptsout
.end
      */

      a="VDI ";
      MEM_ADDRESS contrl=SafeLPeek(Cpu.r[1]); //Cpu.r[1] has vdi parameter block.
      WORD vdi_op=SafeDPeek(contrl+0);  //opcode
      a+=vdi_op;
     // a+=", subopcode ";
      a+=",";
      WORD subop=SafeDPeek(contrl+10); //subopcode
      a+=subop;
      WORD contrl1=SafeDPeek(contrl+2);
      a+=",";
      a+=contrl1;

      MEM_ADDRESS intin=SafeLPeek(Cpu.r[1]+4);
      vdi_intout=SafeLPeek(Cpu.r[1]+12);

      switch(vdi_op) {
      case 1: a+=" v_opnwk"; break; // Open physical workstation. 7.66
      case 2: a+=" v_clswk"; break; // Close a physical workstation. 7.35
      case 3: a+=" v_clrwk"; break; // Close a physical workstation. 7.34
      case 4: a+=" v_updwk"; break; // Update workstation. 7.78
      case 5:
        switch(subop) {
        case 1: a+=" vq_chcells"; break; // Return alpha screen size. 7.87
        case 2: a+=" v_exit_cur"; break; // Exit text mode. 7.46
        case 3: a+=" v_enter_cur"; break; // Enter text mode. 7.45
        case 4: a+=" v_curup"; break; // Move text cursor up one row. 7.40
        case 5: a+=" v_curdown"; break; // Move text cursor down one row. 7.37
        case 6: a+=" v_curright"; break; // Move text cursor right one row. 7.38
        case 7: a+=" v_curleft"; break; // Move text cursor up one row. 7.38
        case 8: a+=" v_curhome"; break; // Home text cursor. 7.37
        case 9: a+=" v_eeos"; break; // Erase to end of screen. 7.42
        case 10: a+=" v_eeol"; break; // Erase to end of line. 7.41
        case 11: a+=" vs_curaddress"; break; // Position text cursor. 7.126
        case 12: a+=" v_curtext"; break; // Output text (alpha mode). 7.39
        case 13: a+=" v_rvon"; break; // Reverse text on (alpha mode). 7.75
        case 14: a+=" v_rvoff"; break; // Reverse text off (alpha mode). 7.75
        case 15: a+=" vq_curaddress"; break; // Inquire text cursor location. 7.89
        case 16: a+=" vq_tabstatus"; break; // Get availability of tablet. 7.95
        case 17: a+=" v_hardcopy"; break; // Output screen to printer. 7.57
        case 18: a+=" v_dspcur"; break; // Display text cursor. 7.40
        case 19: a+=" v_rmcur"; break; // Remove text cursor. 7.74
        case 20: a+=" v_form_adv"; break; // Advance printer page. 7.48
        case 21: a+=" v_output_window"; break; // Output window of page to printer. 7.68
        case 22: a+=" v_clear_disp_list"; break; // Clear display list. 7.34
        case 23: a+=" v_bit_image"; break; // Render bit-image file. 7.31
        case 24: a+=" vq_scan"; break; // Return printer scan heights. 7.94
        case 25: a+=" v_alpha_text"; break; // Output printer text (alpha mode). 7.23
        case 60: a+=" vs_palette"; break; // Set color palette. 7.127
        case 81: a+=" vt_resolution"; break; // Set tablet resolution. 7.165
        case 82: a+=" vt_axis"; break; // Set tablet axis resolution. 7.164
        case 83: a+=" vt_origin"; break; // Set tablet origin. 7.164
        case 84: a+=" vq_tdimensions"; break; // Return tablet X and Y dimensions. 7.96
        case 85: a+=" vt_alignment"; break; // Set tablet alignment. 7.163
        case 91: a+=" vqp_films"; break; // Return camera film types. 7.101
        case 92: a+=" vqp_state"; break; // Return camera driver state. 7.101
        case 93: a+=" vsp_state"; break; // Set camera driver state. 7.145
        case 94: a+=" vsp_save"; break; // Save camera driver state. 7.145
        case 95: a+=" vsp_message"; break; // Supress camera screen messages. 7.144
        case 96: a+=" vqp_error"; break; // Return camera error status. 7.100
        case 98: a+=" v_meta_extents"; break; // Specify metafile bounding box. 7.60
        case 99:
          switch(contrl1) {
          case 0: a+=" vm_pagesize"; break; // Set metafile page size. 7.85
          case 1: a+=" vm_coords"; break; // Set metafile coordinate system. 7.83
          case 32: a+=" v_bez_qual"; break; // Set bezier quality. 7.30
          default: a+=" v_write_meta"; break; // Write metafile item. 7.79
          }
          break;
        case 100: a+=" vm_filename"; break; // Set metafile filename. 7.84
        case 102: a+=" v_fontinit"; break; // Select a new system font. 7.48
        case 2000: a+=" v_pgcount"; break; // Specify laser printer copies. 7.69
        }
        break;
      case 6: a+=(subop==13) ? " v_bez" : " v_pline"; break; // Draw a polyline. 7.71
      //case 6, 13: a+=" v_bez"; break; // Draw a bezier curve. 7.26
      case 7: a+=" v_pmarker"; break; // Draw polymarkers. 7.72
      case 8: // Output graphic text. 7.56
      {
        WORD n=SafeDPeek(contrl+6);
        sprintf(stmp," v_gtext (%d) ",n);
        char *stmp2=stmp+strlen(stmp);
        for(int i=0;i<n;i++,stmp2++)
        {
          char b=(char)SafeDPeek(intin+i*2);
          sprintf(stmp2,"%c",(b>=' ') ? b : '?');
        }
        *stmp2='\0';
        a+=stmp;//" v_gtext";
        break;
      }
      case 9: a+=(subop==13) ? " v_bez_fill" : " v_fillarea"; break; // Draw a filled polygon. 7.46
      //9, 13: a+=" v_bez_fill"; break; // Draw a filled bezier curve. 7.27
      case 10: a+=" v_cellarray"; break; // Draw a cell array. 7.32
      case 11:
        switch(subop) {
        case 1: a+=" v_bar"; break; // Draw a rectangle. 7.25
        case 2: a+=" v_arc"; break; // Draw an arc. 7.24
        case 3: a+=" v_pieslice"; break; // Draw a pieslice. 7.70
        case 4: a+=" v_circle"; break; // Draw a circle. 7.33
        case 5: a+=" v_ellipse"; break; // Draw an ellipse 7.43
        case 6: a+=" v_ellarc"; break; // Draw an elliptical arc. 7.42
        case 7: a+=" v_ellpie"; break; // Draw an elliptical pie segment. 7.44
        case 8: a+=" v_rbox"; break; // Draw a rounded-rectangle. 7.72
        case 9: a+=" v_rfbox"; break; // Draw a filled rounded-rectangle. 7.73
        case 10: a+=" v_justified"; break; // Output justified text. 7.58
        case 13: a+=(contrl1==0) ? " v_bez_off" : " v_bez_on"; break; // Disable bezier drawing. 7.28
        }
        break;
      case 12: a+=" vst_height"; break; // Set graphic text height (in pixels). 7.153
      case 13: a+=" vst_rotation"; break; // Set graphic text rotation. 7.156
      case 14: a+=" vs_color"; break; // Set color palette index. 7.126
      case 15: a+=" vsl_type"; break; // Set line type. 7.135
      case 16: a+=" vsl_width"; break; // Set line width. 7.137
      case 17: a+=" vsl_color"; break; // Set line color. 7.134
      case 18: a+=" vsm_type"; break; // Set marker type. 7.142
      case 19: a+=" vsm_height"; break; // Set marker height. 7.139
      case 20: a+=" vsm_color"; break; // Set marker color. 7.138
      case 21: a+=" vst_font"; break; // Set graphic text font. 7.152
      case 22: a+=" vst_color"; break; // Set graphic text color. 7.150
      case 23: a+=" vsf_interior"; break; // Set fill interior type. 7.129
      case 24: a+=" vsf_style"; break; // Set fill style type. 7.131
      case 25: a+=" vsf_color"; break; // Set fill color. 7.129
      case 26: a+=" vq_color"; break; // Inquire palette index. 7.88
      case 27: a+=" vq_cellarray"; break; // Inquire cell array. 7.86
      case 28: a+=" vrq_locator"; break; // Poll for mouse/keyboard input. 7.121
      //28: a+=" vsm_locator"; break; // Sample mouse/keyboard input. 7.140
      case 29: a+=" vrq_valuator"; break; // Poll for valuator input. 7.123
      //29: a+=" vsm_valuator"; break; // Sample valuator input. 7.143
      case 30: a+=" vrq_choice"; break; // Poll for choice input. 7.121
      //30: a+=" vsm_choice"; break; // Sample input from choice device. 7.138
      case 31: a+=" vsm_string"; break; // Sample keyboard string input. 7.141
      case 32: a+=" vswr_mode"; break; // Set writing mode. 7.162
      case 33: // Set input mode. 7.133
      { //Using this function will cause the AES to function improperly.
        WORD handle=SafeDPeek(contrl+12);
        WORD device=SafeDPeek(intin);
        WORD mode=SafeDPeek(intin+2);
        sprintf(stmp," vsin_mode (%d,%d,%d)",handle,device,mode);
        a+=stmp;//" vsin_mode";
        break;
      }
      case 35: a+=" vql_attributes"; break; // Return line attributes. 7.98
      case 36: a+=" vqm_attributes"; break; // Return marker attributes. 7.99
      case 37: a+=" vqf_attributes"; break; // Return fill area attributes. 7.96
      case 38: a+=" vqt_attributes"; break; // Return text attributes. 7.104
      case 39: a+=" vst_alignment"; break; // Set graphic text alignment. 7.146
      case 100: a+=" v_opnvwk"; break; // Open: a+=" virtual workstation. 7.61
      case 101: a+=" v_clsvwk"; break; // Close a: a+=" virtual workstation. 7.35
      case 102: a+=" vq_extnd"; break; // Inquire workstation attributes. 7.89
      case 103: a+=" v_contourfill"; break; // Fill an irregularly shaped region. 7.36
      case 104: a+=" vsf_perimeter"; break; // Set fill perimeter: a+=" visibility. 7.130
      case 105: a+=" v_get_pixel"; break; // Read screen pixel: a+=" value. 7.55
      case 106: a+=" vst_effects"; break; // Set graphic text effects. 7.150
      case 107:// Set graphic text height (by point). 7.155
      {
        WORD handle=SafeDPeek(contrl+12);
        WORD point=SafeDPeek(intin);
        sprintf(stmp," vst_point (%d,%d)",handle,point);
        a+=stmp;//" vst_point";
        break;
      }
      case 108: a+=" vsl_ends"; break; // Set line end style. 7.134
      case 109: a+=" vro_cpyfm"; break; // Copy raster (opaque mode). 7.119
      case 110: a+=" vr_trnfm"; break; // Transform raster form. 7.117
      case 111: a+=" vsc_form"; break; // Set mouse form. 7.128
      case 112: a+=" vsf_udpat"; break; // Set user defined fill pattern 7.132
      case 113: a+=" vsl_udsty"; break; // Set user-defined line style. 7.136
      case 114: a+=" vr_recfl"; break; // Output filled rectangle. 7.117
      case 115: a+=" vqin_mode"; break; // Return input mode for device. 7.97
      case 116: a+=" vqt_extent"; break; // Return graphic text extent. 7.107
      case 117: a+=" vqt_width"; break; // Return graphic character width. 7.115
      case 118: a+=" vex_timv"; break; // Install timer tick routine. 7.83
      case 119: a+=" vst_load_fonts"; break; // Load fonts from disk. 7.154
      case 120: a+=" vst_unload_fonts"; break; // Unload fonts. 7.160
      case 121: a+=" vrt_cpyfm"; break; // Copy raster (transparent mode). 7.124
      case 122: a+=" void v_show_c"; break; // Show mouse cursor. 7.77
      case 123: a+=" void v_hide_c"; break; // Hide mouse cursor. 7.57
      case 124: a+=" vq_mouse"; break; // Get mouse position and state. 7.93
      case 125: a+=" vex_butv"; break; // Install mouse button routine. 7.80
      case 126: a+=" vex_motv"; break; // Install mouse movement routine. 7.82
      case 127: a+=" vex_curv"; break; // Install mouse rendering routine. 7.81
      case 128: a+=" vq_key_s"; break; // Get shift key status. 7.93
      case 129: a+=" vs_clip"; break; // Set clipping rectangle. 7.125
      case 130: a+=" vqt_name"; break; // Return font name and index. 7.113
      case 131: a+=" vqt_fontinfo"; break; // Return font size information. 7.111
      case 232: a+=" vqt_fontheader"; break; // Copy the Speedo font header into a user defined buffer. 7.110
      case 234: a+=" vqt_trackkern"; break; // Inquire about current track kerning. 7.114
      case 235: a+=" vqt_pairkern"; break; // Inquire about current pair kerning. 7.115
      case 236: a+=" vst_charmap"; break; // Set ASCII/Speedo index interpretation mode. 7.149
      case 237: a+=" vst_kern"; break; // Set kerning modes. 7.154
      case 239: a+=" v_getbitmap_info"; break; // Return Speedo font bitmap extents. 7.53
      case 240: a+=" vqt_f_extent"; break; // Return outline text extent. 7.108
      case 241: a+=" v_ftext"; break; // Output outlined text. 7.49
      //241: a+=" v_ftext16"; break; // Output 16-bit outlined text. 7.50
      //241: a+=" v_ftext_offset"; break; // Output outlined text with individual character offsets. 7.51
      //241: a+=" v_ftext_offset16"; break; // Output 16-bit outlined text with individual character offsets. 7.52
      case 242: a+=" v_killoutline"; break; // Free character outline (no longer used with SpeedoGDOS). 7.59
      case 243: a+=" v_getoutline"; break; // Return character outline. 7.54
      case 244: a+=" vst_scratch"; break; // Set outline scratch buffer. 7.157
      case 245: a+=" vst_error"; break; // Set GDOS error reporting mode. 7.151
      case 246: a+=" vst_arbpt"; break; // Set outline text point size. 7.147
      //246: a+=" vst_arbpt32"; break; // Set outline text point size to a fix31: a+=" value. 7.148
      case 247: a+=" vqt_advance"; break; // Return character advance: a+=" vector. 7.102
      //247: a+=" vqt_advance32"; break; // Return character advance: a+=" vector as a fix31: a+=" value. 7.103
      case 248: a+=" vqt_devinfo"; break; // Return device information. 7.106
      case 249: a+=" v_savecache"; break; // Save bitmap cache to disk. 7.76
      case 250: a+=" v_loadcache"; break; // Load bitmap cache from disk. 7.59
      case 251: a+=" v_flushcache"; break; // Flush outline font cache. 7.47
      case 252: a+=" vst_setsize"; break; // Set outline text proportion. 7.158
      //252: a+=" vst_setsize32"; break; // Set outline text proportion to a fix31: a+=" value. 7.159
      case 253: a+=" vst_skew"; break; // Set outline text skew factor. 7.160
      case 254: a+=" vqt_get_table"; break; // Return character mappings. 7.112
      case 255: a+=" vqt_cachesize"; break; // Return bitmap cache size 7.105
      }//switch(vdi_op)
      break;
    }
    case 0xC8: //AES
    {
/*
struct aespb
{
WORD *contrl;
WORD *global;
WORD *intin;
WORD *intout;
LONG *addrin;
LONG *addrout;
};
*/
      aes_calls_since_reset++;
      a="AES ";
      MEM_ADDRESS contrl=SafeLPeek(Cpu.r[1]); //Cpu.r[1] has aes parameter block.
      MEM_ADDRESS intin=SafeLPeek(Cpu.r[1]+8);
      aes_intout=SafeLPeek(Cpu.r[1]+12);
      WORD aes_op=SafeDPeek(contrl+0);  //opcode
      a+=aes_op;
      switch(aes_op) {
      case 10: a+=" appl_init"; break;
      case 11: a+=" appl_read"; break;
      case 12: a+=" appl_write"; break;
      case 13: a+=" appl_find"; break;
      case 14: a+=" appl_tplay"; break;
      case 15: a+=" appl_trecord"; break;
      case 18: a+=" appl_search"; break;
      case 19: a+=" appl_exit"; break;
      case 20: a+=" evnt_keybd"; break;
      case 21: a+=" evnt_button"; break;
      case 22: a+=" evnt_mouse"; break;
      case 23: a+=" evnt_mesag"; break;
      case 24: a+=" evnt_timer"; break;
      case 25: a+=" evnt_multi"; break;
      case 26: a+=" evnt_dclick"; break;
      case 30: a+=" menu_bar"; break;
      case 31: a+=" menu_icheck"; break;
      case 32: a+=" menu_ienable"; break;
      case 33: a+=" menu_tnormal"; break;
      case 34: a+=" menu_text"; break;
      case 35: a+=" menu_register"; break;
      case 36: a+=" menu_popup"; break;
      case 37: a+=" menu_attach"; break;
      case 38: a+=" menu_istart"; break;
      case 39: a+=" menu_settings"; break;
      case 40: a+=" objc_add"; break;
      case 41: a+=" objc_delete"; break;
      case 42: a+=" objc_draw"; break;
      case 43: a+=" objc_find"; break;
      case 44: a+=" objc_offset"; break;
      case 45: a+=" objc_order"; break;
      case 46: a+=" objc_edit"; break;
      case 47: a+=" objc_change"; break;
      case 48: a+=" objc_sysvar"; break;
      case 50: a+=" form_do"; break;
      case 51: a+=" form_dial"; break;
      case 52: a+=" form_alert"; break;
      case 53: a+=" form_error"; break;
      case 54: a+=" form_center"; break;
      case 55: a+=" form_keybd"; break;
      case 56: a+=" form_button"; break;
      case 70: a+=" graf_rubberbox"; break;
      case 71: a+=" graf_dragbox"; break;
      case 72: a+=" graf_movebox"; break;
      case 73: a+=" graf_growbox"; break;
      case 74: a+=" graf_shrinkbox"; break;
      case 75: a+=" graf_watchbox"; break;
      case 76: a+=" graf_slidebox"; break;
      case 77: a+=" graf_handle"; break;
      case 78: a+=" graf_mouse"; break;
      case 79: a+=" graf_mkstate"; break;
      case 80: a+=" scrp_read"; break;
      case 81: a+=" scrp_write"; break;
      case 90: a+=" fsel_input"; break;
      case 91: a+=" fsel_exinput"; break;
      case 100: a+=" wind_create"; break;
      case 101: a+=" wind_open"; break;
      case 102: a+=" wind_close"; break;
      case 103: a+=" wind_delete"; break;
      case 104:
      {
        WORD handle=SafeDPeek(intin+0);
        WORD mode=SafeDPeek(intin+2);
        sprintf(stmp," wind_get (%d, %d) (0=ERR)",handle,mode);
        a+=stmp;//" wind_get";
        break;
      }
      case 105: a+=" wind_set"; break;
      case 106: a+=" wind_find"; break;
      case 107: a+=" wind_update"; break;
      case 108: a+=" wind_calc"; break;
      case 109: a+=" wind_new"; break;
      case 110: a+=" rsrc_load"; break;
      case 111: a+=" rsrc_free"; break;
      case 112: a+=" rsrc_gaddr"; break;
      case 113: a+=" rsrc_saddr"; break;
      case 114: a+=" rsrc_obfix"; break;
      case 115: a+=" rsrc_rcfix"; break;
      case 120: a+=" shel_read"; break;
      case 121: a+=" shel_write"; break;
      case 122: a+=" shel_get"; break;
      case 123: a+=" shel_put"; break;
      case 124: a+=" shel_find"; break;
      case 125: a+=" shel_envrn"; break;
      case 130: a+=" appl_getinfo"; break;
      }//switch(aes_op)
      break;
    }//scope
    }//switch(REGL(0))
    break;
  }//switch(trap)
  if(a.IsEmpty())
  {
    //l+=" (unrecognised)";
    char stmp2[128];
    sprintf(stmp2," ? D0 %X",REGL(0));
    l+=stmp2;
    //l+=" (unknown)";
  }
  else
  {
    l+="  ";
    for(INT_PTR i=0;i<a.Length();i++)
    {
      if(a[i]=='%')
      {
        lpar=SafeLPeek(my_spp);
        my_spp+=4;
        l+=HEXSl(lpar,8);
      }
      else if(a[i]=='#') // new, for Bconout
      {
        WORD w=SafeDPeek(my_spp);
        char c=(char)w;
        if(c>=' ')
          l+=c;
        else
          l+=w;
        my_spp+=2;
      }
      else if(a[i]=='&')
      {
        l+=HEXSl(SafeDPeek(my_spp),4);
        my_spp+=2;
      }
      else if(a[i]=='$')
      {
        char c;
        int ii;
        for(ii=0;ii<30;ii++)
        {
          c=(char)SafePeek(lpar+ii);
          if(!c)
            break;
          l+=c;
        }
        if(ii>=30)
          l+="...";
      }
      else
        l+=a[i];
    }
  }
#ifdef DEBUG_BUILD
  if(  (detail<2||(TRACE_MASK0&TRACE_LEVEL2))
    && (detail<3||(TRACE_MASK0&TRACE_LEVEL3)) )
#else
  if(detail==1)
#endif
  {
#if SSE_VERSION>=420
    // instruction isn't always TRAP
    TRACE2("TV#%d %s\n",trap,l.Text); // section trap is enabled
#else
    TRACE2("Trap #%d %s\n",trap,l.Text); // section trap is enabled
#endif
  }
}

bool logsection_enabled[100];

#endif//#if defined(SSE_ENABLE_TRACE_LOG)


#if defined(DEBUG_BUILD)

char d2_t_buf[200]; //yep, 200 bytes for int to (hexa)decimal string conversion

struct_logsection logsections[NUM_LOGSECTIONS+8]={
  {"Always",LOGSECTION_ALWAYS},
//  {"-",-1},
  {"Device IO",LOGSECTION_IO},
  {"-",-1},
  {"CPU",LOGSECTION_CPU},
  {"Trace",LOGSECTION_TRACE},
  {"Crash",LOGSECTION_CRASH},
  {"Interrupts",LOGSECTION_INTERRUPTS},
  {"Trap",LOGSECTION_TRAP},
  {"GLU",LOGSECTION_GLUE}, // formerly VIDEO
  {"MMU",LOGSECTION_MMU},
  {"Blitter",LOGSECTION_BLITTER},
  {"MFP",LOGSECTION_MFP_TIMERS},
  {"ACIA",LOGSECTION_ACIA},
  {"IKBD",LOGSECTION_IKBD},
  {"Cartridge",LOGSECTION_CARTRIDGE},
  {"FDC",LOGSECTION_FDC},
  {"DMA",LOGSECTION_DMA},
  {"Ports",LOGSECTION_PORTS},
  {"-",-1},
  {"Image info",LOGSECTION_IMAGE_INFO},
  {"Pasti",LOGSECTION_PASTI},
  {"Hard drive",LOGSECTION_HARDDRIVE}, // ACSI + GEMDOS
  {"Init",LOGSECTION_INIT},//first of log2
  //{"Options",LOGSECTION_OPTIONS},
  {"Tasks",LOGSECTION_AGENDA},
  {"Video rendering",LOGSECTION_VIDEO_RENDERING},
  {"Sound",LOGSECTION_SOUND},
  {"*",-1}};


MEM_ADDRESS debug_mem_write_log_address;
int debug_mem_write_log_bytes;

#endif//#if defined(DEBUG_BUILD)

MEM_ADDRESS aes_intout,vdi_intout,vdi_ptsout=0;

#undef LOGSECTION
