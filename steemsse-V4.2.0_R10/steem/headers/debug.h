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

DOMAIN: Debug
FILE: debug.h
DESCRIPTION: Declarations for debug facilities, like ASSERT, TRACE...
The Debug object and some debug facilities are used in all builds (release
too).
Debugger: fake IO declarations
struct TDebug
This file is included by 6301.c.
---------------------------------------------------------------------------*/

#pragma once
#ifndef SSEDEBUG_H
#define SSEDEBUG_H


#include "SSE.h"
#include "parameters.h"
#include <stdio.h>
#if defined(__cplusplus)
#ifdef WIN32
#include <windows.h>
#endif
#include "conditions.h"
#include "steemh.h" // LINECYCLES etc.
#include <easystr.h>
#endif//c++


#ifdef MINGW_BUILD
#undef NULL
#define NULL 0
#endif

#if defined(SSE_DEBUG) // Debugger build or ide debug build
// general use debug variables;
extern 
#ifdef __cplusplus
"C" 
#endif//c++
int debug0,debug1,debug2,debug3,debug4,debug5,debug6,debug7,debug8,debug9;
#endif

// a structure that may be used by C++ and C objects
#pragma pack(push, 8)

struct TDebug {
#ifdef __cplusplus 
  // ENUM 
  enum EDebug {INIT0,INIT,LOAD,RESET,START,STOP,EXIT};
#endif
  // FUNCTIONS
#ifdef __cplusplus 
  TDebug();
  ~TDebug();
#if defined(SSE_DEBUG_TRACE)
  void FlushTrace();
  void TraceInit();
  void Trace(char *fmt,...); // one function for both IDE & file
  // if logsection enabled, static for function pointer, used in '6301':
  static void TraceLog(char *fmt,...); 
  char* TraceCheckPrivacy(char *string);
#define CHECKPATH(x) Debug.TraceCheckPrivacy(x)
#endif
  void TraceGeneralInfos(int when);
  void Vbl();
  void Reset(bool Cold);
#if defined(SSE_DEBUGGER_TRACE_EVENTS)
  void TraceEvent( void* pointer);
#endif
#endif//C++
  // DATA
  char trace_buffer[MAX_TRACE_CHARS];
#if defined(SSE_DEBUG_TRACE_LOCK)
  HANDLE trace_file_pointer;
#else
  FILE *trace_file_pointer;
#endif
  int ShifterTricks; // used by Stats too

#if defined(SSE_OSD_FPS_INFO)
  DWORD frame_checksum;
  DWORD vbase_at_vbi,vcount_at_vsync;
  BYTE frame_no_change;
#endif
#if defined(SSE_ENABLE_TRACE_LOG)
  int nTrace;
  int LogSection;
  WORD nHbis; // counter for each frame
#endif
#if defined(SSE_DEBUGGER)
  HWND dbg_timer_hwnd[4]; //to record WIN handles
  WORD FrameMfpIrqs;
  BYTE FrameInterrupts; //bit0 VBI 1 HBI 2 MFP
  BYTE DialogOnStopEvent;
  WORD MonitorValue;
  BYTE MonitorValueSpecified; // Debugger SSE option
  BYTE MonitorComparison; // as is, none found = 0 means no value to look for
  BYTE MonitorRange; //check from ad1 to ad2
#endif
#ifdef SSE_BETA
  BYTE IgnoreErrors;
#endif
#if defined(SSE_DEBUGGER_FAKE_IO)
/*  Hack. A free zone in IO is mapped to an array of masks to control 
    a lot of debug options using the Debugger's built-in features. */
  WORD ControlMask[FAKE_IO_LENGTH];
#endif
#if defined(SSE_HD6301_LL)
  BYTE HD6301RamBuffer[256+8];
#endif
  BYTE noShifterTricks;
};

extern 
#ifdef __cplusplus
"C" 
#endif
struct TDebug Debug;

#pragma pack(pop)

#ifdef __cplusplus
extern EasyStr sTraceFile;
extern MEM_ADDRESS aes_intout,vdi_intout,vdi_ptsout;
#if defined(DEBUG_BUILD)
// to place Steem Debugger breakpoints in the code
void debug_set_bk(DWORD ad,bool set); //bool set
#define BREAK(ad) debug_set_bk(ad,true)
#else
#define BREAK(ad)
#endif
#endif//#ifdef __cplusplus

#if defined(SSE_ENABLE_TRACE_LOG)
enum logsection_enum_tag {
 LOGSECTION_ALWAYS,
 LOGSECTION_FDC,
 LOGSECTION_DMA,
 LOGSECTION_PASTI,
 LOGSECTION_IMAGE_INFO,
 LOGSECTION_IO ,
 LOGSECTION_INTERRUPTS ,
 LOGSECTION_TRAP ,
 LOGSECTION_MMU,
 LOGSECTION_MFP_TIMERS ,
 LOGSECTION_MFP=LOGSECTION_MFP_TIMERS,
 LOGSECTION_CRASH ,
 LOGSECTION_HARDDRIVE ,
 LOGSECTION_ACIA,
 LOGSECTION_IKBD ,
 LOGSECTION_PORTS ,
 LOGSECTION_GLUE ,
 LOGSECTION_BLITTER ,
 LOGSECTION_TRACE ,
 LOGSECTION_CPU ,
 LOGSECTION_CARTRIDGE,
 LOGSECTION_INIT ,
 LOGSECTION_AGENDA ,
 LOGSECTION_OPTIONS, // unused
 LOGSECTION_VIDEO_RENDERING,
 LOGSECTION_SOUND ,
 NUM_LOGSECTIONS,
 };
#endif


#define FRAME (Shifter.nVbl) 


// debug macros
extern BYTE FullScreen; // to avoid asserts in fullscreen

// ASSERT triggers only in all beta builds since v412
#if defined(SSE_BETA) && !defined(UNIX)
#if defined(_DEBUG) && defined(VC_BUILD)
#if defined(SSE_X64_DEBUG)
#define ASSERT(x) {if(!(x) && !FullScreen) DebugBreak();}
#else
#define ASSERT(x) {if(!((x)) && !FullScreen) _asm{int 0x03}}
#endif
#elif defined(SSE_UNIX_TRACE)
#define ASSERT(x) assert(x)
#elif defined(SSE_LIBRETRO) //temp
#define ASSERT(x) {if(!(x) && !FullScreen) DebugBreak();}
#else
#define ASSERT(x) {if (!((x))) { \
  TRACE3("%s: %s\n","ASSERT",#x); \
  if(!Debug.IgnoreErrors && !FullScreen) { \
  int ok8788=MessageBox(0,#x,"ASSERT",MB_ICONWARNING|MB_ABORTRETRYIGNORE); \
  if(ok8788==IDABORT) exit(EXIT_FAILURE); \
  Debug.IgnoreErrors=(ok8788==IDIGNORE);}}}
#endif
#else
#define ASSERT(x)
#endif

// BREAKPOINT 
#if defined(SSE_BETA)
#if defined(_DEBUG) && defined(VC_BUILD)
#if defined(SSE_X64_DEBUG)
#define BREAKPOINT(x) {DebugBreak();}
#else
#define BREAKPOINT(x) _asm { int 3 }
#endif
#elif defined(SSE_UNIX_TRACE)
#define BREAKPOINT(x) {assert(0);}
#else
#ifdef __cplusplus
#define BREAKPOINT(x) {if(!Debug.IgnoreErrors) { \
  TRACE3("Breakpoint: %s\n",#x); \
  Debug.IgnoreErrors=!(MessageBox(0,#x,"Breakpoint",MB_ICONWARNING|MB_OKCANCEL)==IDOK);}}
#endif//c++
#endif
#else //!SSE_DEBUG
#define BREAKPOINT(x) {}
#endif

// TRACE
#if defined(SSE_DEBUG_TRACE) && defined(SSE_DEBUG) //395 no TRACE for release mode
#ifdef __cplusplus
#define TRACE Debug.Trace
#endif//c++
#else
#if defined(VC_BUILD) || defined(SSE_UNIX)
#define TRACE(x,...)
#else
#define TRACE
#endif
#endif//#if defined(SSE_DEBUG_TRACE) 

// TRACE_ENABLED
#if defined(SSE_ENABLE_TRACE_LOG)
#define TRACE_ENABLED(section) (section<NUM_LOGSECTIONS && logsection_enabled[(section)])
#else
#define TRACE_ENABLED (0)
#endif

// TRACE_LOG
#if defined(SSE_ENABLE_TRACE_LOG)
#ifdef __cplusplus
#define TRACE_LOG Debug.LogSection=LOGSECTION, Debug.TraceLog //!
//#define TRACE_ONLY(x) x
#endif//C++
#else
#if defined(VC_BUILD) || defined(SSE_UNIX)
#define TRACE_LOG(x,...)
//#define TRACE_ONLY(x)
#else
#define TRACE_LOG
//#define TRACE_ONLY(x)
#endif
#endif

#if defined(DEBUG_BUILD) // detailed traces commanded by Control Mask Browser
#ifdef __cplusplus
#define TRACE_LOG2 if((TRACE_MASK0&TRACE_LEVEL2)) Debug.LogSection=LOGSECTION,Debug.TraceLog
#define TRACE_LOG3 if((TRACE_MASK0&TRACE_LEVEL3)) Debug.LogSection=LOGSECTION,Debug.TraceLog
#endif//C++
#else
#if defined(VC_BUILD) || defined(SSE_UNIX)
#define TRACE_LOG2(x,...)
//#define TRACE_LOG2(x,...) Debug.LogSection=LOGSECTION,Debug.TraceLog
#define TRACE_LOG3(x,...)
#else
#define TRACE_LOG2
#define TRACE_LOG3
#endif
#endif

// v3.6.3 introducing more traces,  verbose here, short in code
// TRACE_FDC
#if defined(SSE_ENABLE_TRACE_LOG)
#ifdef __cplusplus
#define TRACE_FDC Debug.LogSection=LOGSECTION_FDC, Debug.TraceLog //!
#endif//C++
#else
#if defined(VC_BUILD) || defined(SSE_UNIX)
#define TRACE_FDC(x,...)
#else
#define TRACE_FDC
#endif
#endif

// so we get extended trace only if 'wd' checked
#if defined(DEBUG_BUILD)
#ifdef __cplusplus
//#define TRACE_WD if((TRACE_MASK3 & TRACE_CONTROL_FDCWD) && (LOGSECTION==LOGSECTION_FDC)) Debug.LogSection=LOGSECTION,Debug.TraceLog
#define TRACE_WD if((TRACE_MASK3 & TRACE_CONTROL_FDCWD)) Debug.LogSection=LOGSECTION,Debug.TraceLog
#endif//C++
#else
#if defined(VC_BUILD) || defined(SSE_UNIX)
#define TRACE_WD(x,...)
#else
#define TRACE_WD
#endif
#endif

// TRACE_INIT 3.7.0
#if defined(SSE_ENABLE_TRACE_LOG)
#ifdef __cplusplus
#define TRACE_INIT Debug.LogSection=LOGSECTION_INIT, Debug.TraceLog //!
#endif//C++
#else
#if defined(VC_BUILD) || defined(SSE_UNIX)
#define TRACE_INIT(x,...)
#else
#define TRACE_INIT
#endif
#endif

// TRACE_MFM 3.7.1
#if defined(SSE_DEBUGGER_FAKE_IO) 
#ifdef __cplusplus
#define TRACE_MFM if(TRACE_MASK3&TRACE_CONTROL_FDCMFM) Debug.LogSection=LOGSECTION_FDC, Debug.TraceLog //!
#endif//C++
#else
#if defined(VC_BUILD) || defined(SSE_UNIX)
#define TRACE_MFM(x,...)
#else
#define TRACE_MFM
#endif
#endif

// TRACE_VID 3.7.3
#if defined(SSE_ENABLE_TRACE_LOG)
#ifdef __cplusplus
#define TRACE_VID Debug.LogSection=LOGSECTION_GLUE, Debug.TraceLog //!
#endif//C++
#else
#if defined(VC_BUILD) || defined(SSE_UNIX)
#define TRACE_VID(x,...)
#else
#define TRACE_VID
#endif
#endif

// TRACE_VID_R 3.9.3
#if defined(SSE_ENABLE_TRACE_LOG)
#ifdef __cplusplus
#define TRACE_VID_R Debug.LogSection=LOGSECTION_VIDEO_RENDERING, Debug.TraceLog //!
#define TRACE_VID_RECT(rect) Debug.LogSection=LOGSECTION_VIDEO_RENDERING,Debug.TraceLog("%d %d %d %d\n",rect.left,rect.top,rect.right,rect.bottom)
#endif//C++
#else
#if defined(VC_BUILD) || defined(SSE_UNIX)
#define TRACE_VID_R(x,...)
#define TRACE_VID_RECT(rect)
#else
#define TRACE_VID_R
#define TRACE_VID_RECT
#endif
#endif

#if defined(SSE_DEBUGGER_TRACE_EVENTS) //3.8.0
#define TRACE_EVENT(x) Debug.TraceEvent(x)
#else
#define TRACE_EVENT(x) 
#endif

// OSD
#if defined(SSE_DEBUG) && defined(SSE_OSD_DEBUGINFO)
#define TRACE_OSD OsdControl.Trace // (...)
#define TRACE_OSD_DBG if(OPTION_OSD_DEBUGINFO)OsdControl.Trace
#elif defined(VC_BUILD) || defined(SSE_UNIX)
#define TRACE_OSD(x,...)
#define TRACE_OSD_DBG(x,...)
#else
#define TRACE_OSD //?
#define TRACE_OSD_DBG
#endif

#define TRACE_OSD_FPS if(OPTION_OSD_FPSINFO) OsdControl.Trace


#ifdef SSE_DEBUG
// TRACE_RECT 3.9.2
#define TRACE_RECT(rect) TRACE("%d %d %d %d\n",rect.left,rect.top,rect.right,rect.bottom)
#define TRACE_OSD_RECT(rect) TRACE_OSD("%d %d %d %d",rect.left,rect.top,rect.right,rect.bottom)
#endif

// VERIFY
#if defined(SSE_BETA)
#if defined(_DEBUG) && defined(VC_BUILD)
// Our VERIFY facility has no MFC dependency.
#if defined(SSE_X64_DEBUG)
#define VERIFY(x) {if(!(x) && !FullScreen) DebugBreak();}
#else
#define VERIFY(x) {if(!((x)) && !FullScreen) _asm{int 0x03}}
#endif
#elif defined(SSE_UNIX_TRACE)
#define VERIFY(x) if ((x)==0) {TRACE("Verify failed: %s\n",#x); assert(0);} 
#else
#ifdef __cplusplus
#define VERIFY(x) {if((x)==0) {TRACE3("Verify failed: %s\n",#x); \
  if(!Debug.IgnoreErrors&&!FullScreen) { \
  int ok8788=MessageBox(0,#x,"VERIFY",MB_ICONWARNING|MB_ABORTRETRYIGNORE);   \
  if(ok8788==IDABORT) exit(EXIT_FAILURE);\
  Debug.IgnoreErrors=(ok8788==IDIGNORE);}}}
#endif//C++
#endif
#else //!SSE_BETA
#define VERIFY(x) ((void)(x))
#endif

#if defined(SSE_DEBUGGER_FRAME_REPORT)
#define REPORT_LINE FrameEvents.ReportLine()
#else
#define REPORT_LINE
#endif

#if defined(SSE_DEBUG_TRACE) // TRACE2 works in all builds
#define TRACE2 Debug.Trace
#define FLUSH_TRACE Debug.FlushTrace()
#else
#if defined(VC_BUILD) || defined(SSE_UNIX)
#define TRACE2(x,...)
#else
#define TRACE2
#endif
#define FLUSH_TRACE
#endif

#ifdef SSE_BETA
#define TRACE3 Debug.Trace // TRACE3 is a beta-only trace
#else
#if defined(VC_BUILD) || defined(SSE_UNIX)
#define TRACE3(x,...)
#else
#define TRACE3 //bcc
#endif
#endif

#if defined(__cplusplus)

#if defined(SSE_ENABLE_TRACE_LOG)
extern const char* gemdos_calls[0x58];
extern const char* bios_calls[12];
extern const char* xbios_calls[40];
void log_os_call(int trap);
extern bool logsection_enabled[100];
#endif

#if defined(DEBUG_BUILD)

struct struct_logsection {
  char *Name;
  int Index;
};

extern struct_logsection logsections[NUM_LOGSECTIONS+8];
#define IOACCESS_DEBUG_MEM_DMA BIT_13
#define IOACCESS_DEBUG_MEM_WRITE_LOG BIT_14
#define IOACCESS_DEBUG_MEM_READ_LOG BIT_15
extern MEM_ADDRESS debug_mem_write_log_address;
extern int debug_mem_write_log_bytes;

#endif//#if defined(DEBUG_BUILD)

#endif//#if defined(__cplusplus)

#endif//#ifndef SSEDEBUG_H
