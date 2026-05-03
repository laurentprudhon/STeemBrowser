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

FILE: main.cpp
DESCRIPTION: This file contains the various main routines, general 
startup/shutdown code for all the versions of Steem, and command-line options.
GUI and non-emulation objects are instantiated here.
---------------------------------------------------------------------------*/

#include <pch.h>
#pragma hdrstop

/*
------------------------------------------------------------------
       S T E E M   E N G I N E

       The ST Enhanced EMulator

       Last updated [today]
                                                  |||
                                                  |||
                                                 / | \
                                               _/  |  \_
------------------------------------------------------------------
*/


#include <debug.h>
#include <computer.h>
#include <steemh.h>
#include <translate.h>
#include <diskman.h>
#include <notifyinit.h>
#include <draw.h>
#include <loadsave.h>
#include <harddiskman.h>
#include <archive.h>
#include <dir_id.h>
#include <shortcutbox.h>
#include <stjoy.h>
#include <infobox.h>
#include <patchesbox.h>
#include <macros.h>
#include <key_table.h>
#include <display.h>
#include <osd.h>
#include <screen_saver.h>
#include <locale.h>
#if defined(SSE_VID_RECORD_AVI)
#include <AVI/AviFile.h> // AVI (DD-only)
#endif
#if defined(SSE_MAIN_LOOP2)
#include <eh.h>
#include <psapi.h>
#endif
#if defined(SSE_ARCHIVEACCESS_SUPPORT)
#include <ArchiveAccess/ArchiveAccess/ArchiveAccessSSE.h>
#endif
#if defined(SSE_VID_STVL1)
#include <interface_stvl.h>
#endif
#ifdef DEBUG_BUILD
#include <debugger.h>
#include <mr_static.h>
#include <debugger_trace.h>
#endif
#if defined(SSE_WRITEDIR)
#include <choosefolder.h>
#endif
#ifdef UNIX
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#endif
#include <di_get_contents.h>

#if USE_PASTI
enum { BOOT_PASTI_DEFAULT,BOOT_PASTI_ON,BOOT_PASTI_OFF };
BYTE BootPasti=BOOT_PASTI_DEFAULT;
#endif

bool CheckForSteemRunning();
void CleanUpSteem();
int SteemIntro(TConfigStoreFile &CSF);

 //singleton objects
TSteemDisplay Disp;
TPatchesBox PatchesBox;
TDiskManager DiskMan;
TGeneralInfo InfoBox;
TOptionBox OptionBox;
TJoystickConfig JoyConfig;
TOption SSEOptions;
TConfig SSEConfig;

#ifdef WIN32
HINSTANCE hInstance=NULL;
HANDLE SteemRunningMutex=NULL;
bool TryDX=true;
#endif
bool TrySound=true;

#ifdef VC_BUILD
#pragma comment(lib, "winmm.lib") // TODO remove linker setting
#endif

#if defined(MINGW_BUILD) || defined(UNIX)

char *ultoa(unsigned int l,char *s,int radix) {
  if (radix==10) sprintf(s,"%u",(unsigned int)l);
  if (radix==16) sprintf(s,"%x",(unsigned int)l);
  return s;
}

char strupr_convert_buf[256]={0},strlwr_convert_buf[256]={0};

char *strupr(char *s) {
  if (strupr_convert_buf[0]==0){
    strupr_convert_buf[0]=1;
    for (int i=1;i<256;i++){
      strupr_convert_buf[i]=(char)i;
      if (islower(i)) strupr_convert_buf[i]=toupper((char)i);
    }
  }
  char *p=s;
  while (*p){
    *p=strupr_convert_buf[(unsigned char)(*p)];
    p++;
  }
  return s;
}

char *strlwr(char *s) {
  if (strlwr_convert_buf[0]==0){
    strlwr_convert_buf[0]=1;
    for (int i=1;i<256;i++){
      strlwr_convert_buf[i]=(char)i;
      if (isupper(i)) strlwr_convert_buf[i]=tolower((char)i);
    }
  }
  char *p=s;
  while (*p){
    *p=strlwr_convert_buf[(unsigned char)(*p)];
    p++;
  }
  return s;
}

#endif//#if defined(MINGW_BUILD) || defined(UNIX)


#ifdef UNIX

Display *XD;
XContext cWinThis,cWinProc;

char **_argv;
int _argc;

KeyCode VK_LBUTTON,VK_RBUTTON,VK_MBUTTON;
KeyCode VK_F1,VK_F11,VK_F12,VK_END;
KeyCode VK_LEFT,VK_RIGHT,VK_UP,VK_DOWN,VK_TAB;
KeyCode VK_SHIFT,VK_LSHIFT,VK_RSHIFT;
KeyCode VK_MENU,VK_LMENU,VK_RMENU;
KeyCode VK_CONTROL,VK_LCONTROL,VK_RCONTROL;
KeyCode VK_NUMLOCK,VK_SCROLL;

void UnixOutput(char *Str) {
  printf("%s\r\n",Str);
}


char *itoa(int i,char *s,int radix) {
  if (radix==10) sprintf(s,"%i",(int)i);
  if (radix==16) sprintf(s,"%x",(int)i);
  return s;
}

#endif//UNIX

bool OpenComLineFilesInCurrent(bool AlwaysSendToCurrent); // command-line of 2nd instance

const char *stem_version_date_text=__DATE__ " - " __TIME__;

char stem_version_text[SSE_VERSION_TXT_LEN];


#ifndef ONEGAME

#if defined(SSE_BUILD)
char stem_window_title[WINDOW_TITLE_MAX_CHARS+1];
char gAppName[]=APP_NAME;
#else
const char *stem_window_title="Steem Engine";
#endif

char gAppBuildInfo[64];

#else

const char *stem_window_title=ONEGAME;
#define _USE_MEMORY_TO_MEMORY_DECOMPRESSION
#include <unrarlib/unrarlib.h>
//#include <urarlib/urarlib.c>
#include "onegame.cpp"

#endif


bool Initialise(int &RetVal);
void PerformCleanShutdown();
EasyStr CrashFile;

#define LOGSECTION LOGSECTION_INIT


#ifdef WIN32

#if defined(SSE_MAIN_LOOP2)
/*  The idea is to report a system exception (SEH) in a normal try/catch block.
*/
void __cdecl trans_func( unsigned int u, EXCEPTION_POINTERS* pExp )
{
    throw SE_Exception(u,pExp); //caught in WinMain()
}
 
#pragma comment (lib, "Psapi.lib")

void SE_Exception::handle_exception() {
  PVOID& pc_crash_address=m_pExp->ExceptionRecord->ExceptionAddress;
  // get module name where exception happened
  char module_name[80];
  module_name[0]='\0';
  HANDLE hProcess=GetCurrentProcess();
  HMODULE hMods[1024];
  DWORD cbNeeded;
  if( EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded))
  {
    for (DWORD i = 0; i < (cbNeeded / sizeof(HMODULE)); i++ )
    {
      MODULEINFO modinfo;
      if(GetModuleInformation(hProcess,hMods[i],&modinfo,sizeof(modinfo)))
      {
        if(pc_crash_address>=modinfo.lpBaseOfDll && pc_crash_address
          < (BYTE*)modinfo.lpBaseOfDll+modinfo.SizeOfImage)
        {
          // Get the full path to the module's file.
          TCHAR szModName[MAX_PATH];
          if(GetModuleFileName(hMods[i], szModName, sizeof(szModName) / sizeof(TCHAR)))
          {
            char *name=GetFileNameFromPath(szModName);
            if(name)
              strncpy(module_name,name,80-1);
          }
        }
      }
    }
  }
  //In some cases this SEH info is enough to track the bug (using a debugger or the map file)
  char exc_string[256];
  sprintf(exc_string,"System exception $%X at $%p in %s",
    m_pExp->ExceptionRecord->ExceptionCode,pc_crash_address,module_name);
  StatusInfo.MessageIndex=TStatusInfo::X86_CRASH;
#ifdef SSE_STATS
  StatsStatic.nSteemCrash++;
#endif
  REFRESH_STATUS_BAR;
  Alert(exc_string,STEEM_CRASH_TXT,MB_ICONEXCLAMATION); // alert box before trace
  TRACE2("%s\n",exc_string);
}

#endif//#if defined(SSE_MAIN_LOOP2)


#if !defined(SSE_LIBRETRONUKE)

INT WINAPI WinMain(HINSTANCE hInstance,HINSTANCE,char *,int) {
  int RetVal=-50;
  ::hInstance=hInstance;
  RunDir=GetEXEDir(); // only call
  NO_SLASH(RunDir);
  OptionBox.TOSBrowseDir=RunDir; // init default
  DocDir=RunDir+SLASH+"doc"+SLASH;
  SetCurrentDirectory(RunDir.Text); // can help using relative paths in ini
#ifdef SSE_MAIN_LOOP1
  try 
#endif
  {
#if defined(SSE_MAIN_LOOP2) 
    _set_se_translator(trans_func);
#endif
    if(!Initialise(RetVal))
    {
      CleanUpSteem();
      return RetVal;
    }
    MSG MainMess;
#if 0 // TEST circling around incorrect code with compilation switch O1, 
      // push EBX = garbage as parameters instead of 0
      // many other issues, O1 is a no go, because of exceptions and longjmp?
    ZeroMemory(&MainMess,sizeof(MSG));
    UINT a=0; HWND h=NULL;
    while(GetMessage(&MainMess,h,a,a)!=0)
#else
    while(GetMessage(&MainMess,NULL,0,0)!=0)
#endif
    {
      if(HandleMessage(&MainMess))
      {
        TranslateMessage(&MainMess);
#ifndef SSE_NO_SCREENSAVER
        TScreenSaver::checkMessage(&MainMess);
#endif
        DispatchMessage(&MainMess);
      }
    }
    if(StemWin)
      ShowWindow(StemWin,SW_HIDE);

#ifdef DEBUG_BUILD
#if defined(SSE_DEBUGGER_TOGGLE)
    BOOL a=DebuggerVisible;
    if(DWin)
      ShowWindow(DWin,SW_HIDE);
    DebuggerVisible=a; // for ini file
#else
    if(DWin)
      ShowWindow(DWin,SW_HIDE);
#endif
    if(trace_window_handle)
      ShowWindow(trace_window_handle,SW_HIDE);
#endif
    if(SnapShotGetLastBackupPath().NotEmpty())
    {
      DeleteFile(SnapShotGetLastBackupPath());
    }
    PerformCleanShutdown();
    return EXIT_SUCCESS;
  }
#if defined(SSE_MAIN_LOOP1) && defined(SSE_MAIN_LOOP2)
  //catch(int e) {
  //  TRACE_LOG("Exception %d\n",e);
  //}
  catch(SE_Exception e) {
    e.handle_exception();
  }
#endif
#ifndef _DEBUG
#ifdef SSE_MAIN_LOOP1
#if defined(SSE_DIRECTMIDI)
  catch(CDMusicException e) {
    Alert((char*)e.GetErrorDescription(),"DirectMidi",MB_ICONEXCLAMATION);
    TRACE2("%s\n","DirectMidi");
  }
#endif
#if defined(SSE_BADALLOC)
  catch(std::bad_alloc) {
    Alert(T("Memory allocation failure"),T(STEEM_CRASH_TXT),MB_ICONEXCLAMATION);
    TRACE2("%s\n",STEEM_CRASH_TXT);
  }
#endif
  catch(...){
    Alert(T("Unknown exception"),T(STEEM_CRASH_TXT),MB_ICONEXCLAMATION); // C++, CRT ?
    TRACE2("%s\n",STEEM_CRASH_TXT);
  }
#endif
  SetErrorMode(0);
  PerformCleanShutdown();
  return EXIT_FAILURE;
#endif
}
#endif//#if !defined(SSE_LIBRETRONUKE)
#endif//WIN32

#ifdef UNIX

int main(int argc,char *argv[]) {
  _argv=argv;
  _argc=argc;
  for(int n=0;n<_argc-1;n++) 
  {
    EasyStr butt;
    int Type=GetComLineArgType(_argv[1+n],butt);
    if(Type==ARG_HELP) 
    {
      PrintHelpToStdout();
      return 0;
    }
  }
  if(_argv[0][0]=='/')
  { //Full path
    RunDir=_argv[0];
    RemoveFileNameFromPath(RunDir,REMOVE_SLASH);
  }
  else
  {
    RunDir.SetLength(MAX_PATH+1);
    getcwd(RunDir,MAX_PATH);
    NO_SLASH(RunDir);
  }
  DocDir=RunDir+SLASH+"doc"+SLASH;
////  TRACE2("\n-- Steem Engine v%s --\n\n",stem_version_text); // not built yet
////  TRACE2("Steem SSE will save all its settings to %s\n",RunDir.Text);
  //printf(EasyStr("\n-- Steem Engine v")+stem_version_text+" --\n\n");
  //printf(EasyStr("Steem will save all its settings to ")+RunDir.Text+"\n");
  XD=XOpenDisplay(NULL);
  if(XD==NULL)
  {
    printf("\nFailed to open X display\n");
    return EXIT_FAILURE;
  }
  XSetErrorHandler(HandleXError);
  hxc::modal_notifyproc=steem_hxc_modal_notify;
  InitColoursAndIcons();
  NO_SLASH(RunDir);
///  SetCurrentDirectory(RunDir.Text); // can help using relative paths in ini
/// doesn't exist in linux
  int RetVal=-50;
  try {
    if(Initialise(RetVal)==0) 
    {
      CleanUpSteem();
      return RetVal;
    }
    XEvent Ev;
    for(;;) {
      if (hxc::wait_for_event(XD,&Ev)){
        if (ProcessEvent(&Ev)==PEEKED_RUN){
          Window FocusWin;
          int RevertFlag;
          XGetInputFocus(XD,&FocusWin,&RevertFlag);
          if (FocusWin==StemWin && fast_forward!=3 && slow_motion!=3){
            if(OPTION_CAPTURE_MOUSE&1)
              SetStemMouseMode(STEM_MOUSEMODE_WINDOW);
          }
          RunBut.set_check(true);
          run();
          RunBut.set_check(0);
        }
      }
      if (Quitting && XPending(XD)==0) break;
    }
    XUnmapWindow(XD,StemWin);
    XFlush(XD);
    if(SnapShotGetLastBackupPath().NotEmpty()) 
    {
      DeleteFile(SnapShotGetLastBackupPath());
    }
    PerformCleanShutdown();
  	return EXIT_SUCCESS;
  }catch(...){}
  PerformCleanShutdown();
  return EXIT_FAILURE;
}

#endif//UNIX

#if defined(SSE_WRITEDIR)

bool IsDirWritable(char* path) {
  int dirlen=(int)strlen(path);
  char *TestOutFileName=(char*)malloc(dirlen+512);
  bool bCanWrite=false;
  strcpy(TestOutFileName,path);
  // we don't use GetTempFileName because of MAX_PATH limit, we always use the same name
  strcat(TestOutFileName,SLASH "CANWRITE.SSE");
  FILE *fp=FOPEN(TestOutFileName,"wb");
  if(fp)
  {
    FCLOSE(fp);
    DeleteFile(TestOutFileName);
    bCanWrite=true;
  }
  free(TestOutFileName);
  return bCanWrite;
}

#else

void FindWriteDir() {
  char TestOutFileName[MAX_PATH+1];
  bool RunDirCanWrite=false;
  if(GetTempFileName(RunDir,"TST",0,TestOutFileName)) // RunDir init in WinMain()
  {
    FILE *fp=fopen(TestOutFileName,"wb");
    if(fp)
    {
      RunDirCanWrite=true;
      fclose(fp);
    }
    DeleteFile(TestOutFileName);
  }
  if(RunDirCanWrite)
    WriteDir=RunDir;
  else 
  {
#ifdef WIN32
    ITEMIDLIST *idl;
#if !defined(SSE_WIN32_A)
    IMalloc *Mal;
    SHGetMalloc(&Mal);
#endif
    if(SHGetSpecialFolderLocation(NULL,CSIDL_APPDATA,&idl)==NOERROR)
    {
      SHGetPathFromIDList(idl,TestOutFileName);
#if defined(SSE_WIN32_A)
      CoTaskMemFree(idl);
#else
      Mal->Free(idl);
#endif
    }
    else
      GetTempPath(MAX_PATH,TestOutFileName);
    NO_SLASH(TestOutFileName);
#ifndef ONEGAME
    WriteDir=Str(TestOutFileName)+SLASH+"Steem";
#else
    WriteDir=Str(TestOutFileName)+SLASH+ONEGAME;
    CreateDirectory(WriteDir,NULL);
    WriteDir+=Str(SLASH)+ONEGAME_NAME;
#endif
    CreateDirectory(WriteDir,NULL);
#else
    // Must find a location that is r/w
    WriteDir=RunDir;
#endif
  }
}

#endif//#if defined(SSE_WRITEDIR)

#if !defined(SSE_LIBRETRONUKE)

bool Initialise(int &RetVal) { // called once by WinMain()

#ifndef SSE_NOSTEPBYSTEP
  bool StepByStepInit=false;
#endif
#if defined(SSE_VID_LS)
  bool bLoadStartupScreen=false;
#endif
  bool NoINI=false; // NoINI means there's no ini file, so there should be intro!
#ifdef WIN32
  QueryPerformanceFrequency(&MicroTime.Frequency); // 10MHz expected
#endif

  ComputerRestore(); // for drives

#if !defined(SSE_WRITEDIR)
  // update WriteDir, should be RunDir
  FindWriteDir();
#endif

  // build stem_version_text eg "3.7.0" - quite complicated for what it does
  int d1=SSE_VERSION/100; // compiler just pushes the constant values and calls sprintf
  int d2=(SSE_VERSION-d1*100)/10;
  int d3=SSE_VERSION-d1*100-d2*10;
  sprintf(stem_version_text,"%d.%d.%d",d1,d2,d3);

#if 0//def SSE_BETA // version & R make no sense, could be bugfix, could be new features // beta twice...
  //sprintf(gAppBuildInfo,"?.?.? R? %dbit",SSE_BITNESS);
  sprintf(gAppBuildInfo,"%s Beta %dbit",stem_version_text,SSE_BITNESS);
#else
  sprintf(gAppBuildInfo,"%s R%d %dbit",stem_version_text,SSE_VERSION_R,SSE_BITNESS);
#endif
#ifdef SSE_DEBUGGER
  strcat(gAppBuildInfo," Debugger");
#endif
#ifdef SSE_VID_DD
  strcat(gAppBuildInfo," DD");
#endif

  strcpy((char*)stem_window_title,gAppName);

  runstate=RUNSTATE_STOPPED;
  stem_runmode=STEM_MODE_INSPECT;
  struct lconv* loc = localeconv();
  SSEConfig.separator=(loc && SSEConfig.translated) ? loc->thousands_sep[0] : ','; // , or . for numbers, only if Steem is localised
  TranslateFileName=UsersPath+SLASH "Translate.txt";

#ifndef ONEGAME
  bool CustomINI=false;
  // treat command line arguments
#if !defined(SSE_WRITEDIR)
  globalINIFile=UsersPath+SLASH "steem.ini";
#endif
  bool bNoNewInst=false,bAlwaysNewInst=false,bQuitNow=false;
  bool bForceNoTrace=false;
  //SSEConfig.TraceFile=true; // in case of crash on 1st start
  for(int n=0;n<_argc-1;n++)
  {
    EasyStr Path;
    int Type=GetComLineArgType(_argv[1+n],Path);
    if(Type==ARG_SETINIFILE)
    {
      globalINIFile=Path;
      CustomINI=true;
    }
    else if(Type==ARG_SETTRANSFILE)
    {
      if(Exists(Path))
        TranslateFileName=Path;
    }
    else if(Type==ARG_NONEWINSTANCE||Type==ARG_TAKESHOT)
      bNoNewInst=true;
    else if(Type==ARG_ALWAYSNEWINSTANCE)
      bAlwaysNewInst=true;
    else if(Type==ARG_TOSIMAGEFILE)
    {
      ROMFile=Path;
      BootTOSImage=true;
    }
    else if(Type==ARG_QUITQUICKLY)
      bQuitNow=true;
#ifdef DEADC0DE
    else if(Type==ARG_SETFONT)
    {
      /////////////////UNIX_ONLY( hxc::font_sl.Insert(0,0,Path,NULL); )
    }
#endif
    else if(Type==ARG_NONOTIFYINIT)
      SSEConfig.ShowNotify=0;
    else if(Type==ARG_NOTRACE)
    {
      SSEConfig.TraceFile=false;
      bForceNoTrace=true;
    }
  }
#else
  INIFile=RunDir+SLASH ONEGAME_NAME ".ini"; // note should be global...  TODO nuke ONEGAME code?
#endif

#if defined(SSE_WRITEDIR)
  char LimitedPath[MAX_PATH+1]; // SHGetPathFromIDList is limited by MAX_PATH
  EasyStr DocumentsPath,AppDataPath;
  TConfigStoreFile csf;
  DWORD error;

#ifdef WIN32
  ITEMIDLIST* idl;
  bool bWriteReg=false;
  HKEY RegKey=NULL;

#if defined(SSE_420R3)
  // get user's AppData path /Steem
  if(SHGetSpecialFolderLocation(NULL,CSIDL_APPDATA,&idl)==NOERROR)
  {
    SHGetPathFromIDList(idl,LimitedPath);
    CoTaskMemFree(idl);
    NO_SLASH(LimitedPath);
    AppDataPath=LimitedPath;
    AppDataPath+=SLASH "Steem";
  }
#endif

  // get user's Documents path /Steem
  error=SHGetSpecialFolderLocation(NULL,CSIDL_MYDOCUMENTS,&idl);
  if(error==NOERROR)
  {
    SHGetPathFromIDList(idl,LimitedPath);
    CoTaskMemFree(idl);
    NO_SLASH(LimitedPath);
    DocumentsPath=Str(LimitedPath)+SLASH "Steem";
  }
#endif//WIN32

#ifdef UNIX
  if((DocumentsPath = getenv("HOME")) == NULL) {
    DocumentsPath = getpwuid(getuid())->pw_dir;
    DocumentsPath+=SLASH "Steem";
  }
#endif

  // look for INI file, trying RunDir first, then Documents, then the registry
  // and finally AppData
  d1=0; // recycling for some flags
  if(!CustomINI)
  {
    d1|=1; // "no custom INI"

#if defined(SSE_420R4) // first place checked = rundir
    globalINIFile=RunDir+SLASH+"steem.ini";
    error=!csf.Open(globalINIFile);
    csf.Close();
    if(!error)
      error=!IsDirWritable(RunDir.Text);
    if(error)
#endif
    {
      d1|=2; // "not in RunDir"
      globalINIFile=DocumentsPath+SLASH+"steem.ini";
      error=!csf.Open(globalINIFile);
      csf.Close();
#if defined(SSE_420R4)
      if(!error)
        error=!IsDirWritable(DocumentsPath.Text);
#endif
    }
#if !defined(SSE_420R4)
    if(error)
    {
      d1|=2; // "not in Documents"
      globalINIFile=RunDir+SLASH+"steem.ini";
      error=!csf.Open(globalINIFile);
      csf.Close();
#if defined(SSE_420R4)
      if(!error)
        error=!TestWritableDir(RunDir.Text);
#endif
    }
#endif

#ifdef WIN32
    if(error)
    {
      d1|=4; // "not in Documents"
      if(RegOpenKey(HKEY_CURRENT_USER,"Software\\SteemSSE",&RegKey)==ERROR_SUCCESS)
      {
        DWORD Size=SSE_MAX_PATH;
        EasyStr Path;
        Path.SetLength(Size);
        if(RegQueryValueEx(RegKey,"IniPath",NULL,NULL,(BYTE*)Path.Text,&Size)==ERROR_SUCCESS)
        {
          globalINIFile=Path; // full path & name
          error=!csf.Open(globalINIFile);
          csf.Close();
#if defined(SSE_420R4)
          if(!error)
          {
            RemoveFileNameFromPath(Path,REMOVE_SLASH);
            error=!IsDirWritable(Path.Text);
          }
#endif
        }
      }
    }

#if defined(SSE_420R3) // restore possibility of having steem.ini in AppData
    if(error)
    {
      d1|=8; // "not in registry"
      globalINIFile=AppDataPath+SLASH+"steem.ini";
      error=!csf.Open(globalINIFile);
      csf.Close();
#if defined(SSE_420R4)
      if(!error)
        error=!IsDirWritable(AppDataPath.Text);
#endif
    }
#endif
#endif//WIN32

    if(error)
    {
      d1|=0x10; // "not in AppData"
#ifdef WIN32
      // found no ini => prompt user for a location where steem.ini will be created
      // we use ChooseFolder because FileSelect (->GetOpenFileName) chooses its own path
#if defined(SSE_420R4)
      EasyStr path=ChooseFolder(StemWin,T("Found no steem.ini. Please choose the folder that contains\
 it or where it will be created"),LimitedPath); // LimitedPath is Documents
#else
      EasyStr path=ChooseFolder(StemWin,T("Found no steem.ini. Please choose the folder that contains\
 Steem or where it will be created"),LimitedPath); // LimitedPath is Documents
#endif
      if(!Exists(path))
      {
#if defined(SSE_420R4)
        if(path.IsNotEmpty()) // player typed a name
          CreateDirectory(path,NULL);
        else // cancel
#endif
          return false;
      }
#if defined(SSE_420R4)
      if(!IsDirWritable(path.Text))
      {
        Alert(T("Not writable!"),"ERROR",MB_ICONEXCLAMATION);
        return false;
      }
#endif
#if !defined(SSE_420R3)
      // compare with standard folders, we write in registry only if different
      if(strcmp(path,DocumentsPath.Text))
      {
        if(strcmp(path,RunDir.Text))
        {
          d1|=0x20;
          bWriteReg=true;
        }
      }
#endif
#endif
#ifdef UNIX
      EasyStr path=RunDir;
#endif
#if defined(SSE_420R4) // simpler to use player's choice without creating /steem
      globalINIFile=path+SLASH+"steem.ini";
      error=!csf.Open(globalINIFile);
      csf.Close();
      if(error)
      {
        d1|=0x20; // "user points to folder without steem.ini"
        NoINI=true; // intro will run
      }
      // compare with standard folders, we write in registry only if different
      if(strcmp(path.Text,DocumentsPath.Text)) // not Documents/steem
      {
        if(strcmp(path.Text,RunDir.Text)) // not in exe dir
        {
          d1|=0x40; // "write Steem path to registry"
          bWriteReg=true;
        }
      }
#elif defined(SSE_420R3)  // if user points to steem folder, why add /steem, right?
      // we mean "choose the folder where the steem folder that contains the
      // steem folder or where it will be created"
      // but that's confusing, so let's make both ways work
      globalINIFile=path+SLASH+"steem.ini";
      error=!csf.Open(globalINIFile);
      csf.Close();
      if(!error)
        UsersPath=path;
      else
      {
        d1|=0x20; // "user points to folder without steem folder"
        UsersPath=path+SLASH+"steem";
        globalINIFile=UsersPath+SLASH+"steem.ini";
        error=!csf.Open(globalINIFile);
        csf.Close();
      }
      // compare with standard folders, we write in registry only if different
      if(strcmp(UsersPath.Text,DocumentsPath.Text))
      {
        if(strcmp(UsersPath.Text,RunDir.Text))
        {
          d1|=0x40; // "write Steem path to registry"
          bWriteReg=true;
        }
      }
#else
      UsersPath=path+SLASH+"steem";
      globalINIFile=UsersPath+SLASH+"steem.ini";
      error=!csf.Open(globalINIFile);
      csf.Close();
#endif
#if !defined(SSE_420R4)
      if(error)
      {
        d1|=0x80; // "we have no steem.ini"
        NoINI=true; // intro will run
      }
#endif
    }
  }//if(!CustomINI)
  // deduce userspath from steem.ini path
  UsersPath=globalINIFile;
  RemoveFileNameFromPath(UsersPath,REMOVE_SLASH);
  CreateDirectory(UsersPath,NULL); // fails silently if exists

  // get temp folder
#ifdef WIN32

#if defined(SSE_420R3)
#if defined(SSE_420R4)
  //420R4: first check ini
  error=!csf.Open(globalINIFile);
  if(!error)
  {
    TempPath=csf.GetStr("Main","TempPath","");
    csf.Close();
  }
  error=(TempPath.NotEmpty()) ? (!IsDirWritable(TempPath.Text)) : true;
  // then AppData
  if(error && AppDataPath.NotEmpty())
  {
    CreateDirectory(AppDataPath,NULL); // don't forget this...
    error=!IsDirWritable(AppDataPath.Text);
    if(!error)
    {
      TempPath=AppDataPath;
      d1|=0x80; // "temp is AppData/Steem"
    }
  }
  if(error)
#else
  if(AppDataPath.NotEmpty())
    TempPath=AppDataPath;
  else
#endif
  {
    error=GetTempPath(MAX_PATH,LimitedPath);
    if(error==NOERROR)
    {
      d1|=0x100; // "use system temp path"
      NO_SLASH(LimitedPath);
      TempPath=Str(LimitedPath)+SLASH "Steem";
      CreateDirectory(TempPath,NULL); // fails silently if exists
#if defined(SSE_420R4)
      error=!IsDirWritable(TempPath.Text);
#endif
    }
#if defined(SSE_420R4)
    if(error)
#else
    else
#endif
    {
      d1|=0x200; // "no system temp... use app path"
      TempPath=RunDir;
#if defined(SSE_420R4)
      if(!IsDirWritable(TempPath.Text))
      {
        TempPath=UsersPath; // if this one isn't OK we've already quit
        d1|=0x400; // "use steem.ini path"
      }
#endif
    }
  }
#else
  error=SHGetSpecialFolderLocation(NULL,CSIDL_APPDATA,&idl);
  if(error==NOERROR)
  {
    SHGetPathFromIDList(idl,LimitedPath);
    CoTaskMemFree(idl);
  }
  else
    error=GetTempPath(MAX_PATH,LimitedPath);
  if(error==NOERROR)
  {
    NO_SLASH(LimitedPath);
    TempPath=Str(LimitedPath)+SLASH+"Steem";
    CreateDirectory(TempPath,NULL); // fails silently if exists
  }
  else // no system temp... use app path
    TempPath=RunDir; // and we don't check if it's writable
#endif

#endif//WIN32
#ifdef UNIX
  if((TempPath = getenv("TMPDIR")) == NULL) {
    //TempPath = P_tmpdir();
  }
  if(TempPath==NULL)
    TempPath=RunDir;
#endif

#ifdef WIN32
  if(bWriteReg) // write steem.ini path in registry so that we don't ask all the time
  {
#if defined(SSE_420R3) // first try to open existing key
    if(RegKey==NULL)
      RegOpenKey(HKEY_CURRENT_USER,"Software\\SteemSSE",&RegKey);
#endif
    if(RegKey==NULL) // create key if necessary
      RegCreateKey(HKEY_CURRENT_USER,"Software\\SteemSSE",&RegKey);
    if(RegSetValueEx(RegKey,"IniPath",0,REG_SZ,(BYTE*)globalINIFile.Text,
                     (DWORD)globalINIFile.Length()+1)!=ERROR_SUCCESS)
      d1|=0x400; // "write to registry successful"
    RegCloseKey(RegKey);
  }
#endif

#endif//#if defined(SSE_WRITEDIR)

  TConfigStoreFile CSF(globalINIFile); // open main ini

#if defined(SSE_WRITEDIR)
  if(CustomINI) // just in case, we don't write it
  {
    UsersPath=CSF.GetStr("Main","UsersPath",UsersPath);
    TempPath=CSF.GetStr("Main","TempPath",TempPath);
  }
#endif

  if(!bForceNoTrace)
  {
#if defined(SSE_FORCE_TRACE_FILE)
    SSEConfig.TraceFile=true;
#else
    SSEConfig.TraceFile=!!CSF.GetInt("Options","TraceFile",SSEConfig.TraceFile);
#if defined(SSE_420R4)
    SSEConfig.TraceShowPath=!!CSF.GetInt("Options","TraceShowPath",SSEConfig.TraceShowPath);
#endif
#endif
#ifdef UNIX
    SSEConfig.TraceFile=true; // temp
#endif
  }

  if(SSEConfig.TraceFile)
    Debug.TraceInit(); // NOTE: the TRACE facility isn't available before this point!

#if defined(SSE_420R4)
  if(SSEConfig.TraceShowPath)
  {
    TRACE2(T("Privacy notice: full paths are displayed (startup settings)\n%s %s\nTemp %s\n"),
           "steem.ini",globalINIFile.Text,TempPath.Text);
  }
#endif

#ifdef WIN32
  TRACE2("Command-line: %s\n",CHECKPATH(GetCommandLine())); // to see arguments
#endif
  TRACE2("%s $%X\n","steem.ini",d1);

  SSEConfig.ShowNotify=(CSF.GetInt("Main","NoNotify",!SSEConfig.ShowNotify)==0);
#ifndef ONEGAME
#if defined(SSE_ONEINSTANCE)
  if((CSF.GetInt("Main","OneInstance",true)!=0||bNoNewInst)&&!bAlwaysNewInst)
#else
  if((CSF.GetInt("Options","OpenFilesInNew",true)==0||bNoNewInst)&&!bAlwaysNewInst)
#endif
  {
    if(OpenComLineFilesInCurrent(bNoNewInst)) 
    {
#ifdef WIN32      
      HWND CurSteemWin=FindWindow("Steem Window",NULL);
      SetForegroundWindow(CurSteemWin);
#endif
      RetVal=EXIT_SUCCESS;
      return false;
    }
  }
  if(BootTOSImage) 
    CSF.SetStr("Machine","ROM_File",ROMFile);
#if !defined(SSE_WRITEDIR)
  NoINI=(!CustomINI
    && !CSF.GetBool("Update",GetFileNameFromPath(GetEXEFileName().Text),false)
    && !CSF.GetBool("Main","NoIntro",false));
#endif
  CSF.SetInt("Main","DebugBuild",0 DEBUG_ONLY( +1 ) );
  CSF.SetStr("Update","CurrentVersion",Str((char*)stem_version_text));
  if(bQuitNow) 
  {
    RetVal=EXIT_SUCCESS;
    return false;
  }
#endif
  srand(timeGetTime());
  make_crc32_table();
  fdc_make_crc16_table();
#if defined(DEBUG_BUILD)
  load_logsections();
#endif
  InitTranslations();
#if defined(SSE_GUI_RICHEDIT)
  LoadLibrary("RICHED20.DLL");
#endif
#if USE_PASTI
  hPasti=SteemLoadLibrary(PASTI_DLL);
  if(hPasti)
  {
    struct pastiCALLBACKS pcb;
    struct pastiINITINFO pii;
    ZeroMemory(&pcb,sizeof(pcb));
    ZeroMemory(&pii,sizeof(pii));
    bool Failed=true;
    LPPASTIINITPROC pastiInit=(LPPASTIINITPROC)GetProcAddress(hPasti,"pastiInit");
    if(pastiInit) 
    {
      pcb.LogMsg=pasti_log_proc;
      pcb.WarnMsg=pasti_warn_proc;
      pcb.MotorOn=pasti_motor_proc;
      pii.dwSize=sizeof(pii);
      pii.applFlags=0;
      pii.applVersion=2;
      pii.cBacks=&pcb;
      Failed=(pastiInit(&pii)==FALSE);
      pasti=pii.funcs;
    }
    if(Failed)
    {
      SteemFreeLibrary(hPasti);
      Alert(T("Pasti initialisation failed"),T("ERROR"),MB_ICONEXCLAMATION);
    }
    else
    {
      char p_exts[PASTI_FILE_EXTS_BUFFERSIZE];
      ZeroMemory(p_exts,PASTI_FILE_EXTS_BUFFERSIZE);

      pasti->GetFileExtensions(p_exts,PASTI_FILE_EXTS_BUFFERSIZE,TRUE); // returns 0

      // Convert to NULL terminated list
      for(int i=0;i<PASTI_FILE_EXTS_BUFFERSIZE;i++)
      {
        if(p_exts[i]=='\0') 
          break;
        if(p_exts[i]==';') 
          p_exts[i]='\0';
      }
      // Strip *.
      char *p_src=p_exts,*p_dest=pasti_file_exts;
      ZeroMemory(pasti_file_exts,PASTI_FILE_EXTS_BUFFERSIZE);
      while(*p_src)
      {
        if(*p_src=='*') 
          p_src++;
        if(*p_src=='.') 
          p_src++;
        TRACE_INIT("%s ",p_src);
        strcpy(p_dest,p_src);
        p_dest+=strlen(p_dest)+1;
        p_src+=strlen(p_src)+1;
      }
      TRACE_INIT("can be handled by %s\n",PASTI_DLL);
      SSEConfig.PastiDll=true;
    }
  }
#endif //USE_PASTI
  DiskMan.InitGetContents();
#ifndef ONEGAME
  bool TwoSteems=CheckForSteemRunning();
  bool CrashedLastTime=CleanupTempFiles();
  TRACE2(T("Already running %d Crashed last time %d\n"),TwoSteems,CrashedLastTime);
  if(!TwoSteems)
  {
#ifndef SSE_NOSTEPBYSTEP
#ifndef SSE_BETA
    if(CrashedLastTime)
    {
#if defined(SSE_DEBUG) // Debugger too
      // Crashes are common while testing
#elif defined(SSE_BUILD) // don't want emails
      StepByStepInit=Alert(T("It seems that Steem did not close properly. If it\
 crashed we are terribly sorry, it shouldn't happen. If you can get Steem to \
crash 2 or more times when doing the same thing then please post a bug report \
here: ")+"\n\n" STEEM_WEB_BUG_REPORTS "\n\n"+ T("Please write as much detail as \
you can and we'll look into it as soon as possible. ")+"\n\n"+T("If you are \
having trouble starting Steem, you might want to step carefully through the \
initialisation process.  Would you like to do a step-by-step confirmation?"),
        T("Step-By-Step Initialisation"),MB_ICONQUESTION|MB_YESNO)==IDYES;
#else
      StepByStepInit=Alert(T("It seems that Steem did not close properly. If it crashed we are terribly sorry, it shouldn't happen. If you can get Steem to crash 2 or more times when doing the same thing then please tell us, it would be a massive help.")+
        "\n\nE-mail: " STEEM_EMAIL "\n\n"+
        T("Please send as much detail as you can and we'll look into it as soon as possible. ")+
        "\n\n"+T("If you are having trouble starting Steem, you might want to step carefully through the initialisation process.  Would you like to do a step-by-step confirmation?"),
        T("Step-By-Step Initialisation"),MB_ICONQUESTION|MB_YESNO)==IDYES;
#endif
    }
#endif//#ifndef SSE_BETA
#endif//#ifndef SSE_NOSTEPBYSTEP
#if !defined(SSE_NOSTEPBYSTEP) || defined(SSE_420R5) // no step by step but create crashfile
    CrashFile.SetLength(MAX_PATH);
    GetTempFileName(TempPath,"CRA",0,CrashFile);
    FILE *fp=fopen(CrashFile,"wb");
    if(fp)
    {
      fclose(fp);
      SetFileAttributes(CrashFile,FILE_ATTRIBUTE_HIDDEN);
    }
#endif
  }
#if defined(SSE_ONEINSTANCE)
  else
  {
    bool OneInstance=(CSF.GetInt("Main","OneInstance",0)!=0);
    if(OneInstance)
    {
#ifdef WIN32      
      HWND CurSteemWin=FindWindow("Steem Window",NULL);
      SetForegroundWindow(CurSteemWin);
#endif
      RetVal=EXIT_SUCCESS;
      return false;
    }
  }
#endif
  DeleteFile(UsersPath+SLASH "steemcrash.ini");
#ifndef SSE_NOSTEPBYSTEP
  if(StepByStepInit && !NoINI) 
  {
    if(Alert(T("It is possible that one of the settings you changed has made \
Steem crash, do you want to use the default settings? (Note: this won't lose \
your settings, anything you change this time will be saved to steemcrash.ini)"),
      T("Use Default Settings?"),MB_ICONQUESTION|MB_YESNO)==IDYES) 
    {
#if defined(SSE_WRITEDIR)
      ROMFile=CSF.GetStr("Machine","ROM_File",RunDir+SLASH "tos.img");
#else
      ROMFile=CSF.GetStr("Machine","ROM_File",UsersPath+SLASH "tos.img");
#endif
      CSF.Close();
      globalINIFile=UsersPath+SLASH "steemcrash.ini";
      CSF.Open(globalINIFile);
      CSF.SetStr("Machine","ROM_File",ROMFile);
    }
  }
#endif//#ifndef SSE_NOSTEPBYSTEP
#endif//#ifndef ONEGAME

  FONT_SIZE=CSF.GetByte("Main","FontSize",FONT_SIZE);
#if defined(SSE_GUI_BIGICONS)
  // must be set before call to LoadAllIcons() - that's why it's a startup option
  BIG_ICONS=CSF.GetByte("Main","BigIcons",BIG_ICONS); 
#endif
  LoadAllIcons(&CSF,true);
  OPTION_BLOCK_RESIZE=CSF.GetBool("Display","BlockResize",OPTION_BLOCK_RESIZE);
#if !defined(_DEBUG)
  if(SSEConfig.ShowNotify)
    CreateNotifyInitWin(T("Steem is Initialising").Text);
#endif
#ifdef WIN32
  SetNotifyInitText(T("COM and Common Controls"));
  CoInitialize(NULL);
  InitCommonControls();
#if defined(SSE_DIRECTMIDI) && defined(SSE_420R6)
  SSEConfig.DirectMusic=CSF.GetByte("Main","DirectMusic",SSEConfig.DirectMusic);
#endif
#endif//WIN32
  SetNotifyInitText(T("ST Memory"));
#if !defined(SSE_BADALLOC)
  try
#endif
  {
    BYTE ConfigBank1=(BYTE)CSF.GetInt("Machine","Mem_Bank_1",MEMCONF_512);
    BYTE ConfigBank2=(BYTE)CSF.GetInt("Machine","Mem_Bank_2",MEMCONF_512);
    if(ConfigBank1!=MEMCONF_128 && ConfigBank1!=MEMCONF_512 &&
#if defined(SSE_MMU_MONSTER_ALT_RAM)
      ConfigBank1!=MEMCONF_6MB &&
#endif
      ConfigBank1!=MEMCONF_2MB && ConfigBank1!=MEMCONF_7MB)
    {
      // Invalid memory somehow
      ConfigBank1=MEMCONF_512;
      ConfigBank2=MEMCONF_512;
    }
    SSEConfig.make_Mem(ConfigBank1,ConfigBank2);
  }
#if !defined(SSE_BADALLOC)
  catch (...){
    MessageBox(WINDOWTYPE(0),T("Could not allocate enough memory!"),
      T("Out Of Memory"),MB_ICONEXCLAMATION|MB_TASKMODAL|MB_TOPMOST|MB_SETFOREGROUND);
    RetVal=EXIT_FAILURE;
    return false;
  }
#endif
#ifdef DEBUG_BUILD
  for(int m=0;m<MAX_MEMORY_BROWSERS;m++) 
    m_b[m]=NULL;
  for(int m=0;m<MAX_MR_STATICS;m++) 
    m_s[m]=NULL;
#endif
#ifdef WIN32
  OSVERSIONINFO osvi;
  osvi.dwOSVersionInfoSize=sizeof(OSVERSIONINFO);
  GetVersionEx(&osvi);
  WinNT=(osvi.dwPlatformId==VER_PLATFORM_WIN32_NT);
  SetErrorMode(SEM_NOOPENFILEERRORBOX|SEM_FAILCRITICALERRORS|SEM_NOALIGNMENTFAULTEXCEPT);
#endif
#ifdef DISABLE_STEMDOS
  for(int n=0;n<GEMDOS_MAXDRIVES;n++)
  {
    Stemdos.DriveMounted[n]=false;
    Stemdos.MountPath[n]="";
  }
#else
  Stemdos.Init();
#endif
#if defined(SSE_420R4)
  PCpal=Get_PCpal(); // defined in draw_c or asm_draw.asm
#ifndef SSE_NO_OSD
  osd_routines_init();
#endif
#else
  if(!draw_routines_init())
  {
    RetVal=EXIT_FAILURE;
    return false;
  }
#endif
#ifndef ONEGAME
  int IntroResult=2;
#ifdef TEST_STEEM_INTRO
  SteemIntro(CSF);
#endif
#if !defined(SSE_NO_INTRO)
  if(NoINI) 
  {
#if defined(SSE_VID_2SCREENS)
    SetNotifyInitText(T("Introduction")); // if we hide, intro goes on primary!
#else
#ifdef WIN32
    ShowWindow(NotifyWin,SW_HIDE);
#endif
#endif
    IntroResult=SteemIntro(CSF);
    CSF.SetInt("Update",GetFileNameFromPath(GetEXEFileName().Text),1);
    if(IntroResult==1) 
    {
      if(CSF.GetInt("Sound","Channels",3)==3) // only if there was no ini
        CSF.Changed=false; // don't save
      RetVal=EXIT_FAILURE;
      return false;
    }
#ifdef WIN32
    if(NotifyWin) 
    {
      SetWindowPos(NotifyWin,HWND_TOPMOST,0,0,0,0,SWP_NOSIZE|SWP_NOMOVE|SWP_SHOWWINDOW);
      UpdateWindow(NotifyWin);
    }
#endif
  }//noini
#endif//#if !defined(SSE_NO_INTRO)

  SetNotifyInitText(T("ST Operating System"));
  if(IntroResult==2) // = initial value
  {
    ROMFile=CSF.GetStr("Machine","ROM_File",RunDir+SLASH "tos.img");
#if defined(SSE_GUI_CONFIG)
    // add current TOS path if necessary
    if(strchr(ROMFile.Text,SLASHCHAR)==NULL) // no slash = no path
    {
      EasyStr tmp=OptionBox.TOSBrowseDir+SLASH+ROMFile; // not sure the direct way works!
      ROMFile=tmp;
      Tos.UpdateTOSPath(&ROMFile);
    }
#endif
#ifndef SSE_NOSTEPBYSTEP
    if(StepByStepInit) 
    {
      if(Alert(T("A different TOS version may help to stop Steem crashing, would\
 you like to choose one?"),T("Change TOS?"),MB_ICONQUESTION|MB_YESNO)==IDYES)
        ROMFile="";
    }
#endif
    while(!load_TOS(ROMFile)) // TOS loaded first before loading state
    {
#ifdef WIN32
      if(NotifyWin) 
        ShowWindow(NotifyWin,SW_HIDE);
#endif
      if(ROMFile.NotEmpty()) 
      {
        if(!Exists(ROMFile))
        {
          MessageBox((WINDOWTYPE)0,EasyStr(T("Can't find file"))+" "+ROMFile,T("ERROR"),
            MB_ICONEXCLAMATION|MB_TASKMODAL|MB_TOPMOST|MB_SETFOREGROUND);
        }
        else 
        {
          MessageBox((WINDOWTYPE)0,ROMFile+" "+T("is not a valid TOS"),T("ERROR"),
            MB_ICONEXCLAMATION|MB_TASKMODAL|MB_TOPMOST|MB_SETFOREGROUND);
        }
      }
#ifdef WIN32
      char *fstypes=FSTypes(3,NULL);
      ROMFile=FileSelect(NULL,T("Select TOS Image"),OptionBox.TOSBrowseDir,fstypes,1,true,"img");
      free(fstypes);
#endif
#ifdef UNIX
      fileselect.set_corner_icon(&Ico16,ICO16_CHIP);
      ROMFile=fileselect.choose(XD,UsersPath,NULL,
        T("Select TOS Image"),FSM_LOAD|FSM_LOADMUSTEXIST,romfile_parse_routine,".img");
#endif
      if(ROMFile.IsEmpty()) 
      {
        RetVal=EXIT_FAILURE;
        return false;
      }
    }
#ifdef WIN32
    if(NotifyWin) 
    {
      SetWindowPos(NotifyWin,HWND_TOPMOST,0,0,0,0,SWP_NOSIZE|SWP_NOMOVE|SWP_SHOWWINDOW);
      UpdateWindow(NotifyWin);
    }
#endif
  }//if(IntroResult==2) 
  CartFile=CSF.GetStr("Machine","Cart_File",CartFile);
  if(CartFile.NotEmpty())
  {
    if(!load_cart(CartFile)) 
      CartFile="";
  }
#else
  if(!load_TOS("")) 
  {
    RetVal=EXIT_FAILURE;
    return 0;
  }
#endif//#ifndef ONEGAME
#if defined(SSE_ACSI) && !defined(SSE_ACSI_MNGR)
/*  We use the existing Steem "crawler" to load whatever hard disk IMG 
    files are in Steem/ACSI, up to MAX_ACSI_DEVICES.
*/
  ASSERT(!acsi_dev && !SSEConfig.AcsiImg);
  DirSearch ds; 
  EasyStr Fol=UsersPath+SLASH+ACSI_HD_DIR+SLASH;
#if defined(SSE_LONG_PATH)
  EasyStr sText;
  sText.SetLength(SSE_MAX_PATH);
  char *Path=sText.Text;
#endif
  if (ds.Find(Fol+"*.img")){
    do{
#if !defined(SSE_LONG_PATH)
      char Path[MAX_PATH];
#endif
      strcpy(Path,Fol.Text);
      strcat(Path,ds.Name);
      bool ok=AcsiHdc[acsi_dev].Init(acsi_dev,Path); 
      if(ok)
      {
        TRACE2("%s %s\n","ACSI",CHECKPATH(Path));
        SSEConfig.AcsiImg=true;
        acsi_dev++;
      }
    } while (ds.Next() && acsi_dev<MAX_ACSI_DEVICES);
    ds.Close();
  }
#endif//ACSI

#if defined(SSE_NETWORK)
  WSADATA wsaData;
  // Initialize Winsock
  int iResult = WSAStartup(MAKEWORD(2,2),&wsaData);
  if(iResult != 0) {
    TRACE_LOG("WSAStartup failed with error: %d\n",iResult);
  }
#endif

#if defined(SSE_DIRECTMIDI)
  if(SSEConfig.DirectMusic)
  {
#if defined(SSE_420R6) // local try/catch
    SetNotifyInitText("DirectMidi");
    try {
      DirectMusic.Initialize();
      DirectMidiIn.Initialize(DirectMusic);
      DirectMidiOut.Initialize(DirectMusic);
      DirectMidiClock.Initialize(DirectMusic);
    }
    catch(...)
    {
      TRACE2("%s %s\n","DirectMidi","ERROR");
      SSEConfig.DirectMusic=0;
    }
#else
    SetNotifyInitText("DirectMidi");
    DirectMusic.Initialize();
    DirectMidiIn.Initialize(DirectMusic);
    DirectMidiOut.Initialize(DirectMusic);
    DirectMidiClock.Initialize(DirectMusic);
#endif
  }
#endif

#if defined(SSE_420R4)
  SetNotifyInitText(T("MC68000"));
#else
  SetNotifyInitText(T("Jump Tables"));
#endif
  cpu_routines_init();

#if defined(SSE_ARCHIVEACCESS_SUPPORT)
  SetNotifyInitText(ARCHIVEACCESS_DLL);
#ifdef WIN32
  ARCHIVEACCESS_OK=LoadArchiveAccessDll(ARCHIVEACCESS_DLL);
  TRACE_LOG("%s ok:%d\n",ARCHIVEACCESS_DLL,ARCHIVEACCESS_OK);
#endif
  if(ARCHIVEACCESS_OK)
    enable_zip=true;
#endif
#ifdef WIN32
#if !defined(SSE_NO_UNZIPD32)
  SetNotifyInitText(UNZIP_DLL);
  LoadUnzipDLL();
#endif
#endif
#if defined(SSE_DISK_RAR_SUPPORT_WIN)
  SetNotifyInitText(UNRAR_DLL);
  LoadUnrarDLL();
#endif
#if defined(SSE_DISK_CAPS)
  SetNotifyInitText(SSE_DISK_CAPS_PLUGIN_FILE);
  Caps.Init();
#endif
#if defined(SSE_HD6301_LL) 
  SetNotifyInitText(HD6301_ROM_FILENAME);
  Ikbd.Init();
#endif
#ifdef DEBUG_BUILD
  d2_routines_init();
#if defined(SSE_420R5)
  std::fill(pc_history,pc_history+HISTORY_SIZE,MAGIC_HIST_INIT);
#else
  for(int i=0;i<HISTORY_SIZE;i++) 
    pc_history[i]=MAGIC_HIST_INIT;
  pc_history_idx=0;
#endif
#endif
  SetNotifyInitText(T("GUI"));
  if(!MakeGUI())
  {
    RetVal=EXIT_FAILURE;
    return false;
  }
#if !defined(SSE_NO_UPDATE)
  if(Exists(RunDir+SLASH "new_steemupdate.exe")) 
  {
    DeleteFile(RunDir+SLASH "steemupdate.exe");
    if(Exists(RunDir+SLASH "steemupdate.exe")==0)
      MoveFile(RunDir+SLASH "new_steemupdate.exe",RunDir+SLASH "steemupdate.exe");
  }
#endif
#ifndef ONEGAME
  ParseCommandLine(_argc-1,_argv+1);
#endif
  Disp.DrawToVidMem=CSF.GetBool("Options","DrawToVidMem",Disp.DrawToVidMem);
#ifdef WIN32
  Disp.BlitHideMouse=CSF.GetBool("Options","BlitHideMouse",Disp.BlitHideMouse);
  if(CSF.GetInt("Options","NoDirectDraw",0)) 
    TryDX=false;
#ifndef SSE_NOSTEPBYSTEP
  if(TryDX && StepByStepInit) // from prehistoric times
  {
#if defined(SSE_VID_D3D)
    if(Alert(T("DirectX can cause problems on some set-ups, would you like \
  Steem to stop using Direct3D for this session? (Note: Not using Direct3D slows\
 down Steem).")+" "+T("To permanently stop using Direct3D turn on Options->\
Startup->Never Use Direct3D."),T("No Direct3D?"),MB_ICONQUESTION|MB_YESNO)==IDYES)
#endif
#if defined(SSE_VID_DD)
    if(Alert(T("DirectX can cause problems on some set-ups, would you like \
Steem to stop using DirectDraw for this session? (Note: Not using DirectDraw \
slows down Steem).")+" "+T("To permanently stop using DirectDraw turn on \
Options->Startup->Never Use DirectDraw."),T("No DirectDraw?"),MB_ICONQUESTION
      |MB_YESNO)==IDYES)
#endif
      TryDX=false;
  }
#endif//#ifndef SSE_NOSTEPBYSTEP

#if defined(SSE_VID_D3D) || defined(SSE_VID_DD)
  if(TryDX)
#if defined(SSE_VID_D3D) 
    Disp.SetMethods(DISPMETHOD_D3D,DISPMETHOD_GDI,0);
#endif
#if defined(SSE_VID_DD)
    Disp.SetMethods(DISPMETHOD_DD,DISPMETHOD_GDI,0);
#endif
  else
#endif
    Disp.SetMethods(DISPMETHOD_GDI,0);
#endif
#ifdef UNIX
  if(CSF.GetInt("Options","NoSHM",0)) 
    TrySHM=0;
#ifndef SSE_NOSTEPBYSTEP
  if(TrySHM && StepByStepInit) 
  {
    if(Alert(T("It is possible that using the MIT Shared Memory Extension to speed up drawing might cause problems on some systems.  Do you want to disable SHM?"),
      T("No SHM?"),MB_ICONQUESTION|MB_YESNO)==IDYES)
      TrySHM=0;
  }
#endif
  if(TrySHM)
    Disp.SetMethods(DISPMETHOD_XSHM,DISPMETHOD_X,0);
  else
    Disp.SetMethods(DISPMETHOD_X,0);
#endif//UNIX
#if defined(SSE_VID_STVL1)
  SetNotifyInitText(VIDEO_LOGIC_DLL);
  StvlInit();
#endif
  Disp.Init();
  Debug.TraceGeneralInfos(TDebug::INIT);
#if defined(SSE_VID_D3D)
  if(Disp.pD3D) // previous build crashed here when GDI was used
  {
    D3DFORMAT DisplayFormat=D3DFMT_X8R8G8B8; //32bit; D3DFMT_R5G6B5=16bit
    UINT nD3Dmodes=Disp.pD3D->GetAdapterModeCount(Disp.m_Adapter,DisplayFormat);
    //ASSERT(nD3Dmodes);
    D3DDISPLAYMODE Mode;
    Disp.pD3D->EnumAdapterModes(Disp.m_Adapter,DisplayFormat,nD3Dmodes-1,&Mode);
#ifndef NO_CRAZY_MONITOR
    for(int i=0;i<EXTMON_RESOLUTIONS;i++) 
    {
      if(extmon_res[i][0]==0)
        extmon_res[i][0]=Mode.Width;
      if(extmon_res[i][1]==0)
        extmon_res[i][1]=Mode.Height;
    }
#endif
  }
#endif
#if !defined(SSE_SOUND_NO_NOSOUND_OPTION)
#ifdef WIN32
  if(CSF.GetInt("Options","NoDirectSound",0))
  {
    TrySound=false;
    SSEOptions.AudioInterface=0;
  }
#endif
#ifdef UNIX
  x_sound_lib=CSF.GetInt("Sound","Library",x_sound_lib);
  if(CSF.GetInt("Sound","NoPortAudio",0)&&CSF.GetInt("Sound","IgnoreNoPortAudio",0)==0) 
  {
    x_sound_lib=0;
    CSF.SetInt("Sound","IgnoreNoPortAudio",1);
  }
  TrySound=(x_sound_lib!=0);
#endif
#endif
#ifndef SSE_NOSTEPBYSTEP
  if(TrySound && StepByStepInit) 
  {
    if(Alert(T("Would you like to disable sound for this session?")+" "
      ///UNIX_ONLY(+T("To permanently disable sound turn on Options->Startup->Never Use PortAudio."))
      WIN_ONLY(+T("To permanently disable sound turn on Options->Startup->Never Use DirectSound.")),
      T("No Sound?"),MB_ICONQUESTION|MB_YESNO)==IDYES)
      TrySound=false;
  }
#endif
  if(TrySound) 
    InitSound();
  if(CSF.GetBool("Options","RunOnStart",0))
    BootInMode|=BOOT_MODE_RUN;
  init_DirID_to_text();
#ifndef SSE_NOSTEPBYSTEP
  if(StepByStepInit) 
  {
    bool PortOpen=false;
    for(int p=0;p<3;p++) 
    {
      EasyStr PNam=EasyStr("Port_")+p+"_";
      if(CSF.GetInt("MIDI",PNam+"Type",PORTTYPE_NONE)) 
      {
        PortOpen=true;
        break;
      }
    }
    if(PortOpen) 
    {
      TRACE_INIT("PortOpen\n");
      if(Alert(T("Accessing parallel/serial/MIDI ports can cause Steem to freeze up or crash on some systems.")+" "+
        T("Do you want to stop Steem accessing these ports?"),
        T("Disable Ports?"),MB_ICONQUESTION|MB_YESNO)==IDYES) 
      {
        for(int p=0;p<3;p++) 
        {
          EasyStr PNam=EasyStr("Port_")+p+"_";
          CSF.SetInt("MIDI",PNam+"Type",PORTTYPE_NONE);
        }
      }
    }
  }
  CSF.SaveTo(globalINIFile); // Update the INI just in case a dialog does GetCSFInt
#endif//#ifndef SSE_NOSTEPBYSTEP
#ifndef ONEGAME
  SetNotifyInitText(T("Loading state")); // can take quite some time if big disk
#endif
  LoadState(&CSF);
#ifndef ONEGAME
  SetNotifyInitText(T("Power on"));
#endif
#if defined(SSE_420R2)
  power_on(); // like before
#else
  reset_st(RESET_COLD|RESET_NOSTOP); // which will call power_on()
  //power_on();
#endif
#ifdef WIN32
#if !defined(SSE_NO_UPDATE) && !defined(ONEGAME)
  if(CSF.GetInt("Update","AutoUpdateEnabled",true)) 
  {
    if(Exists(RunDir+"\\SteemUpdate.exe")) 
    {
      EasyStr Online=LPSTR(CSF.GetInt("Update","AlwaysOnline",0)?" online":"");
      EasyStr NoPatch=LPSTR(CSF.GetInt("Update","PatchDownload",true)==0?" nopatchcheck":"");
      EasyStr AskPatch=LPSTR(CSF.GetInt("Update","AskPatchInstall",0)?" askpatchinstall":"");
      WinExec(EasyStr("\"")+RunDir+"\\SteemUpdate.exe\" silent"+Online+NoPatch+AskPatch,SW_SHOW);
    }
  }
#endif
#endif
#ifdef DEBUG_BUILD
  update_register_display(true);
#endif
  draw_init_resdependent(); //set up palette conversion & stuff
#if !defined(SSE_420R5) // no reason to do that except crash
  draw(true);
#endif
#ifdef WIN32
  SendMessage(ToolTip,TTM_ACTIVATE,ShowTips,0);
  SetTimer(StemWin,SHORTCUTS_TIMER_ID,50,NULL);
#endif
#ifdef UNIX
  if(ShowTips) 
    hints.start();
  hxc::set_timer(StemWin,SHORTCUTS_TIMER_ID,50,timerproc,NULL);
#endif
#ifndef ONEGAME
#if !defined(SSE_VID_LS)
  bool snapshot_was_loaded=false;
#endif
  //TRACE("Disk A %s, statefile %s\n",BootDisk[0].Text,BootStateFile.Text);
  if(BootDisk[DRIVE_A].NotEmpty())
  {
    if(BootStateFile.NotEmpty()) 
    {
      //  Request: specify both memory snapshot and disks.
      int cnt=0;
      for(int d=0;d<2;d++)
      {
        if(BootDisk[d]!=".")
        {
          cnt++;
          EasyStr Name=GetFileNameFromPath(BootDisk[d]);
          *strrchr(Name,'.')='\0';
          DiskMan.InsertDisk(d,Name,BootDisk[d],false,false);
        }
      }
      if(cnt)
      {
        if(LoadSnapShot(BootStateFile,false,false,false))
          BootInMode|=BOOT_MODE_RUN;
      }
      else if(LoadSnapShot(BootStateFile)) 
        BootInMode|=BOOT_MODE_RUN;
#if !defined(SSE_VID_LS)
      snapshot_was_loaded=true;
#endif
      LastSnapShot=BootStateFile;
      TRACE_INIT("BootStateFile %s\n",CHECKPATH(BootStateFile.Text));
    }
    else
    {
#if USE_PASTI
      if(pasti_active)
      {
        // Check you aren't booting with pasti when passing a non-pasti compatible disk
        for(int d=DRIVE_A;d<=DRIVE_B;d++)
        {
          if(BootDisk[d].NotEmpty()&&NotSameStr_I(BootDisk[d],".")) 
          {
            if(!ExtensionIsPastiDisk(strrchr(BootDisk[d],'.')))
              BootPasti=BOOT_PASTI_OFF;
          }
        }
      }
      if(BootPasti!=BOOT_PASTI_DEFAULT) 
      {
        bool old_pasti=pasti_active;
        pasti_active=(BootPasti==BOOT_PASTI_ON);
        //TRACE_LOG("pasti_active %d\n",pasti_active);
        if(DiskMan.IsVisible()&&old_pasti!=pasti_active) 
          DiskMan.RefreshDiskView();
      }
#endif
      for(int d=DRIVE_A;d<=DRIVE_B;d++) 
      {
        if(BootDisk[d].NotEmpty()&&NotSameStr_I(BootDisk[d],".")) 
        {
          EasyStr Name=GetFileNameFromPath(BootDisk[d]);
          *strrchr(Name,'.')='\0';
          //TRACE("insert %s in %c\n",BootDisk[d].Text,d+'A');
          if(DiskMan.InsertDisk(d,Name,BootDisk[d],false,false))
          {
            if(d==DRIVE_A)
              BootInMode|=BOOT_MODE_RUN;
          }
        }
      }
    }
  }
  else if(AutoLoadSnapShot && !BootTOSImage)
  {
    if(Exists(TempPath+SLASH+AutoSnapShotName+".sts")) 
    {
#ifndef SSE_NOSTEPBYSTEP
      bool Load=true;
      if(StepByStepInit) 
      {
        if(Alert(T("Would you like to restore the state of the ST?"),
          T("Restore State?"),MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2)==IDNO)
          Load=false;
      }
      if(Load) 
#endif
      {
        LoadSnapShot(TempPath+SLASH+AutoSnapShotName+".sts",false,true,false); // Don't add to history, don't change disks
#if !defined(SSE_VID_LS)
        snapshot_was_loaded=true;
#endif
      }
    }
  }
  else
  {
#if defined(SSE_STATS)
    StatsStatic.nReset++;
#endif
#if defined(SSE_VID_LS)
    if(!BootTOSImage && !(BootInMode&BOOT_MODE_RUN))
      bLoadStartupScreen=true; // new steem, window isn't visible yet, must delay
#endif
  }
#ifndef ONEGAME
  SetNotifyInitText(T("Get Ready..."));
#endif
  if(OptionBox.NeedReset())
    reset_st(RESET_COLD|RESET_STOP|RESET_CHANGESETTINGS|RESET_NOBACKUP);
  CheckResetDisplay();
  if(Disp.CanGoToFullScreen()) 
  {
    bool Full=(BootInMode & BOOT_MODE_FLAGS_MASK)==BOOT_MODE_FULLSCREEN;
    if((BootInMode & BOOT_MODE_FLAGS_MASK)==BOOT_MODE_DEFAULT) 
      Full=CSF.GetBool("Options","StartFullscreen",0);
    if(Full)
    {
      TRACE_INIT("StartFullscreen\n");
#ifdef WIN32
      PostMessage(StemWin,WM_SYSCOMMAND,SC_MAXIMIZE,2);
#endif
#ifdef UNIX
#endif
    }
  }
#else
  if(OGInit()==0) 
    QuitSteem();
  PostMessage(StemWin,WM_SYSCOMMAND,SC_MAXIMIZE,2);
#endif
#if !defined(SSE_VID_LS)
  if(!snapshot_was_loaded) // otherwise res_change() will erase starting pic
    res_change();
#endif
#ifndef ONEGAME
  DestroyNotifyInitWin();
#endif
#ifdef WIN32
  int ShowState=SW_SHOW;
  if(CSF.GetInt("Main","Maximized",0)) 
    ShowState=SW_MAXIMIZE;
#endif
  CSF.Close();
#ifdef WIN32
#ifdef DEBUG_BUILD
#if defined(SSE_DEBUGGER_TOGGLE)
  if(DebuggerVisible)
#endif
    ShowWindow(DWin,SW_SHOW);
#endif
  ShowWindow(StemWin,ShowState);
#if !defined(SSE_NO_AOT)
  if(bAlwaysOnTop)
#if defined(SSE_VID_2SCREENS)
    SetWindowPos(StemWin,HWND_TOPMOST,Disp.rcMonitor.left,Disp.rcMonitor.top,0,0,
      SWP_NOMOVE|SWP_NOSIZE);
#else
    SetWindowPos(StemWin,HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);
#endif
#endif
#endif//WIN32
#ifdef UNIX
  XMapWindow(XD,StemWin);
  XFlush(XD);
#endif
  SSEConfig.IsInit=TRUE;
  //TRACE2("is init%d\n",SSEConfig.IsInit);
  if(BootInMode & BOOT_MODE_RUN)
  {
    //if(GetForegroundWindow()==StemWin) 
    CLICK_PLAY_BUTTON();
  }
#ifdef WIN32
  else
    SetFocus(StemWin);
#endif
#if defined(SSE_VID_LS)
  if(bLoadStartupScreen)
  {
    // look for startup screen & display
    EasyStr path=UsersPath+SLASH+SSE_STARTUPSCREEN; // personalised
    if(!Exists(path))
      path=RunDir+SLASH+SSE_STARTUPSCREEN; // official
#if defined(SSE_420R6) // local try/catch just in case
    try {
      Disp.LoadScreenShot(path);
    }
    catch(...) {
      TRACE2("%s %s\n",path,"ERROR");
    }
#else
    Disp.LoadScreenShot(path);
#endif
  }
#endif
  return true;
}

#endif//#if !defined(SSE_LIBRETRONUKE)

#undef LOGSECTION


void QuitSteem() {
  Debug.TraceGeneralInfos(TDebug::EXIT);
  Quitting=true;
#ifdef WIN32
  if(runstate!=RUNSTATE_STOPPED)
  {
#if defined(SSE_EMU_THREAD)
    //ASSERT(bAppActive);
    if(OPTION_EMUTHREAD && !bAppActive)
      bAppActive=true;
#endif
    Glue.m_Status.stop_emu=1;
    PostMessage(StemWin,WM_CLOSE,0,0);
  }
  else if(FullScreen)
  {
    PostMessage(StemWin,WM_COMMAND,MAKEWPARAM(IDC_BACKTOWIN,BN_CLICKED),
      (LPARAM)GetDlgItem(StemWin,IDC_BACKTOWIN));
    PostMessage(StemWin,WM_CLOSE,0,0);
  }
  else 
  {
    draw_end();
    PostQuitMessage(ERR_OK);
  }
#endif
#ifdef UNIX
  if(runstate==RUNSTATE_RUNNING)
    runstate=RUNSTATE_STOPPING;
  else if(FullScreen)
    Disp.ChangeToWindowedMode();
  XAutoRepeatOn(XD);
#endif
}


void CloseAllDialogs() {
  ShortcutBox.Hide();
#if !defined(SSE_420R5) // DiskMan does it
  HardDiskMan.Hide();
#if defined(SSE_ACSI_MNGR)
  AcsiHardDiskMan.Hide();
#endif
#endif
  DiskMan.Hide();
  JoyConfig.Hide();
  InfoBox.Hide();
  OptionBox.Hide();
  PatchesBox.Hide();
}


void PerformCleanShutdown() {
#ifndef ONEGAME
  //TRACE2("is init%d\n",SSEConfig.IsInit);
#if !defined(SSE_LIBRETRONUKE)
  // don't try to save if we just crashed, it could corrupt files
  if(SSEConfig.IsInit && StatusInfo.MessageIndex!=TStatusInfo::X86_CRASH)
  {
    TConfigStoreFile CSF(globalINIFile);
    HardDiskMan.Hide();
    for(int n=0;n<nStemDialogs;n++)
    {
      if(DialogList[n])
        DialogList[n]->SaveVisible(&CSF);
    }
    CloseAllDialogs();
    SaveState(&CSF);
    CSF.Close();
  }
#endif//#if !defined(SSE_LIBRETRONUKE)
#endif
  CleanUpSteem();
#if defined(SSE_VID_RECORD_AVI)
  if(pAviFile)
    delete pAviFile;
  pAviFile=NULL;
#endif
#if !defined(SSE_LIBRETRONUKE)
  if(GetContents_ListFile)
    delete[] GetContents_ListFile;
#ifndef SSE_LEAN_AND_MEAN
  GetContents_ListFile=NULL;
#endif
#endif
}


void CleanUpSteem() {
#ifdef WIN32
  KillTimer(StemWin,SHORTCUTS_TIMER_ID);
#endif
#ifdef UNIX
  hxc::kill_timer(StemWin,HXC_TIMER_ALL_IDS);
#endif
  macro_end(MACRO_ENDRECORD|MACRO_ENDPLAY);
  CloseAllDialogs();
  MIDIPort.Close();
  ParallelPort.Close();
  SerialPort.Close();
#ifndef DISABLE_STEMDOS
  Stemdos.CloseAllFiles();
#endif
  Disp.Release();
#ifndef SSE_NO_OSD
  if(osd_plasma_pal) 
  {
    delete[] osd_plasma_pal;
    delete[] osd_plasma;     
#ifndef SSE_LEAN_AND_MEAN
    osd_plasma_pal=NULL;
    osd_plasma=NULL;
#endif
  }
#endif
  SoundRelease();
  FreeJoysticks();
#if !defined(SSE_LIBRETRONUKE)
  CleanupGUI();
#endif
  DestroyKeyTable();

#ifdef WIN32
#if !defined(SSE_NO_UNZIPD32)
  if(hUnzip) 
    FreeLibrary(hUnzip); 
#ifndef SSE_LEAN_AND_MEAN
  enable_zip=false; //hUnzip=NULL;
#endif
#endif
#if defined(SSE_ARCHIVEACCESS_SUPPORT)
  if(ARCHIVEACCESS_OK)
    UnloadArchiveAccessDll();
#ifndef SSE_LEAN_AND_MEAN
  ARCHIVEACCESS_OK=FALSE;
#endif
#endif
#if defined(SSE_DISK_RAR_SUPPORT_WIN)
  if(hUnrar)
    SteemFreeLibrary(hUnrar); 
#endif
#if defined(SSE_NETWORK)
  WSACleanup();
#endif
#endif//WIN32

#ifdef UNIX
  if(XD) 
  {
    XCloseDisplay(XD);
    XD=NULL;
  }
#endif

  if(cart_save)
    cart=cart_save;
#ifndef SSE_LEAN_AND_MEAN
  cart_save=NULL;
#endif
  if(cart) 
  { 
#if defined(SSE_CARTRIDGE_ACTIVE2)
    VirtualFree(cart,0,MEM_RELEASE);
#else
    delete[] cart;
#endif
#ifndef SSE_LEAN_AND_MEAN
    cart=NULL; 
#endif
  }
  if(STMem) 
  { 
    delete[] STMem;
#if defined(SSE_420R2B)
    himem=0; // for SafePeek
    mem_len=0;
#endif
#ifndef SSE_LEAN_AND_MEAN    
    STMem=NULL; 
#endif
  }
  if(STRom) 
  { 
    delete[] STRom;
#ifndef SSE_LEAN_AND_MEAN    
    STRom=NULL;
#endif
#if defined(SSE_420R2B)
    rom_addr=rom_addr_end=0;
#endif
  }
  if(TranslateBuf) 
    delete[] TranslateBuf;
  if(TranslateUpperBuf) 
    delete[] TranslateUpperBuf;
#ifndef SSE_LEAN_AND_MEAN
  TranslateBuf=TranslateUpperBuf=NULL;
#endif
  if(psg_channels_buf!=NULL)
    delete[] psg_channels_buf;
#ifndef SSE_LEAN_AND_MEAN
  psg_channels_buf=NULL;
#endif
  if(SteSoundChannelBuf!=NULL)
    delete[] SteSoundChannelBuf;
#ifndef SSE_LEAN_AND_MEAN
  SteSoundChannelBuf=NULL;
#endif
  ONEGAME_ONLY(OGCleanUp(); )
#ifdef WIN32
  if(SteemRunningMutex)
    CloseHandle(SteemRunningMutex);
#endif
#if USE_PASTI
  if(hPasti) 
    SteemFreeLibrary(hPasti);
#endif
#if !defined(SSE_NO_FREEIMAGE)
  if(Disp.hFreeImage)
    SteemFreeLibrary(Disp.hFreeImage);
#endif
#if defined(SSE_VID_STVL1)
  if(hStvl) 
    SteemFreeLibrary(hStvl);
#endif
  if(CrashFile.NotEmpty())
    DeleteFile(CrashFile);
#ifndef SSE_LEAN_AND_MEAN
  CrashFile="";
#endif

}

#undef LOGSECTION
#define LOGSECTION LOGSECTION_INIT

#if !defined(SSE_LIBRETRONUKE)

bool ComLineArgCompare(char*Arg,char*s,bool truncate=false) {
  if(*Arg=='/'||*Arg=='-') 
    Arg++;
  if(*Arg=='-') 
    Arg++;
  if(truncate) 
    return(IsSameStr_I(EasyStr(Arg).Lefts((int)strlen(s)),s));
  return IsSameStr_I(Arg,s);
}


int GetComLineArgType(char *Arg,EasyStr &Path) {
  if(Arg[0]=='-' || Arg[0]=='/')
    Arg++; //v4.1.2
  if(0)
  {}
  else
#ifdef WIN32
  if(ComLineArgCompare(Arg,"NODD")||ComLineArgCompare(Arg,"GDI"))
    return ARG_GDI;
  else if(ComLineArgCompare(Arg,"NODS")||ComLineArgCompare(Arg,"NOSOUND"))
    return ARG_NODS;
#if defined(SSE_DIRECTMIDI)
  else if(ComLineArgCompare(Arg,"NODM"))
    return ARG_NODM;
#endif
  else if(ComLineArgCompare(Arg,"DOUBLECHECKSHORTCUTS"))
    return ARG_DOUBLECHECKSHORTCUTS;
  else if(ComLineArgCompare(Arg,"OLDPORTIO"))
    return ARG_OLDPORTIO;
  else if(ComLineArgCompare(Arg,"GDIFSBORDER"))
    return ARG_GDIFSBORDER;
#endif
#if USE_PASTI
  else if(ComLineArgCompare(Arg,"PASTI"))
    return ARG_PASTI;
  else if(ComLineArgCompare(Arg,"NOPASTI"))
    return ARG_NOPASTI;
#endif
#ifdef UNIX
  if(ComLineArgCompare(Arg,"NOSHM"))
    return ARG_NOSHM;
#endif
  else if(ComLineArgCompare(Arg,"WINDOW"))
    return ARG_WINDOW;
  else if(ComLineArgCompare(Arg,"FULLSCREEN"))
    return ARG_FULLSCREEN;
  else if(ComLineArgCompare(Arg,"NONEW"))
    return ARG_NONEWINSTANCE;
  else if(ComLineArgCompare(Arg,"OPENNEW"))
    return ARG_ALWAYSNEWINSTANCE;
  else if(ComLineArgCompare(Arg,"NOLPT"))
    return ARG_NOLPT;
  else if(ComLineArgCompare(Arg,"NOCOM"))
    return ARG_NOCOM;
  else if(ComLineArgCompare(Arg,"SCLICK"))
    return ARG_SOUNDCLICK;
  else if(ComLineArgCompare(Arg,"HELP")||ComLineArgCompare(Arg,"H"))
    return ARG_HELP;
  else if(ComLineArgCompare(Arg,"QUITQUICKLY"))
    return ARG_QUITQUICKLY;
  else if(ComLineArgCompare(Arg,"DONTLIMITSPEED"))
    return ARG_DONTLIMITSPEED;
  else if(ComLineArgCompare(Arg,"ACCURATEFDC"))
    return ARG_ACCURATEFDC;
  else if(ComLineArgCompare(Arg,"NOPCJOYSTICKS"))
    return ARG_NOPCJOYSTICKS;
  else if(ComLineArgCompare(Arg,"ALLOWREADOPEN"))
    return ARG_ALLOWREADOPEN;
  else if(ComLineArgCompare(Arg,"NOINTS"))
    return ARG_NOINTS;
  else if(ComLineArgCompare(Arg,"STFMBORDER"))
    return ARG_STFMBORDER;
  else if(ComLineArgCompare(Arg,"SCREENSHOTUSEFULLNAME"))
    return ARG_SCREENSHOTUSEFULLNAME;
  else if(ComLineArgCompare(Arg,"SCREENSHOTALWAYSADDNUM"))
    return ARG_SCREENSHOTALWAYSADDNUM;
  else if(ComLineArgCompare(Arg,"ALLOWLPTINPUT"))
    return ARG_ALLOWLPTINPUT;
  else if(ComLineArgCompare(Arg,"NONOTIFYINIT"))
    return ARG_NONOTIFYINIT;
  else if(ComLineArgCompare(Arg,"PSGCAPTURE"))
    return ARG_PSGCAPTURE;
  else if(ComLineArgCompare(Arg,"CROSSMOUSE"))
    return ARG_CROSSMOUSE;
  else if(ComLineArgCompare(Arg,"RUN"))
    return ARG_RUN;
  else if(ComLineArgCompare(Arg,"NOAUTOSNAPSHOT"))
    return ARG_NOAUTOSNAPSHOT;
  else if(ComLineArgCompare(Arg,"SOF=",true)) 
  {
    Path=strchr(Arg,'=')+1;
    return ARG_SETSOF;
  }
#ifdef DEADC0DE
  else if(ComLineArgCompare(Arg,"FONT=",true)) 
  {
    Path=strchr(Arg,'=')+1;
    return ARG_SETFONT;
  }
#endif
  else if(ComLineArgCompare(Arg,"SCREENSHOT=",true)) 
  {
    Path=strchr(Arg,'=')+1;
    return ARG_TAKESHOT;
  }
  else if(ComLineArgCompare(Arg,"SCREENSHOT",true)) 
  {
    Path="";
    return ARG_TAKESHOT;
  }
#ifdef UNIX
#if !defined(NO_PORTAUDIO)
  else if(ComLineArgCompare(Arg,"PABUFSIZE=",true)) 
  {
    Path=strchr(Arg,'=')+1;
    return ARG_SETPABUFSIZE;
  }
#endif
#if !defined(NO_RTAUDIO)
  else if(ComLineArgCompare(Arg,"RTBUFSIZE",true)) 
  {
    Path=strchr(Arg,'=')+1;
    return ARG_RTBUFSIZE;
  }
  else if(ComLineArgCompare(Arg,"RTBUFNUM",true)) 
  {
    Path=strchr(Arg,'=')+1;
    return ARG_RTBUFNUM;
  }
#endif
#endif//UNIX
  else if(ComLineArgCompare(Arg,"NOTRACE",true))
    return ARG_NOTRACE;
#if defined(SSE_UNIX_TRACE)
  else if(ComLineArgCompare(Arg,"TRACEFILE=",true)) 
  { //Y,N
    Path=strchr(Arg,'=')+1;
    return ARG_TRACEFILE;
  }
  else if(ComLineArgCompare(Arg,"LOGSECTION=",true)) 
  {
    Path=strchr(Arg,'=')+1;
    return ARG_LOGSECTION;
  }
#endif
  else 
  {
    int Type=ARG_UNKNOWN;
    char *pArg=Arg;
    if(ComLineArgCompare(Arg,"INI=",true)) 
    {
      pArg=strchr(Arg,'=')+1;
      Type=ARG_SETINIFILE;
    }
    else if(ComLineArgCompare(Arg,"TRANS=",true)) 
    {
      pArg=strchr(Arg,'=')+1;
      Type=ARG_SETTRANSFILE;
    }
#ifdef DEAD_C0DE
    else if(ComLineArgCompare(Arg,"CUTS=",true)) 
    {
      pArg=strchr(Arg,'=')+1;
      Type=ARG_SETCUTSFILE; // unused
    }
#endif
    Path.SetLength(MAX_PATH);
    GetLongPathName(pArg,Path,MAX_PATH);
    if(Type!=ARG_UNKNOWN) 
      return Type;
    char *dot=strrchr(GetFileNameFromPath(Path),'.');
    if(dot)
    {
      if(ExtensionIsDisk(dot))
        return ARG_DISKIMAGEFILE;
#if USE_PASTI
      else if(ExtensionIsPastiDisk(dot))
        return ARG_PASTIDISKIMAGEFILE;
#endif
      else if(IsSameStr_I(dot,".STS"))
        return ARG_SNAPSHOTFILE;
      else if(IsSameStr_I(dot,".STC"))
        return ARG_CARTFILE;
      else if(IsSameStr_I(dot,".PRG")||IsSameStr_I(dot,".APP")||IsSameStr_I(dot,".TOS"))
        return ARG_STPROGRAMFILE;
      else if(IsSameStr_I(dot,".GTP")||IsSameStr_I(dot,".TTP"))
        return ARG_STPROGRAMTPFILE;
#ifdef WIN32
      else if(IsSameStr_I(dot,".LNK"))
        return ARG_LINKFILE;
#endif
      else if(IsSameStr_I(dot,".IMG")||IsSameStr_I(dot,".ROM"))
        return ARG_TOSIMAGEFILE;
    }
    return ARG_UNKNOWN;
  }
}

#if !defined(VC_BUILD)
bool no_ints=false;
#endif

void ParseCommandLine(int NumArgs,char *Arg[],int Level) {
  for(int n=0;n<NumArgs;n++) 
  {
    EasyStr Path;
    int Type=GetComLineArgType(Arg[n],Path);
    //TRACE("ARG %d %s %d\n",n,Path.Text,Type);
    switch(Type) {
#ifdef WIN32
    case ARG_GDI: TryDX=false; break;
    case ARG_NODS: TrySound=false; break;
#if defined(SSE_DIRECTMIDI) && defined(SSE_420R5)
    case ARG_NODM:
      SSEConfig.DirectMusic=0;
      break;
#endif
    case ARG_DOUBLECHECKSHORTCUTS: DiskMan.DoExtraShortcutCheck=true; break;
    case ARG_OLDPORTIO: TPortIO::AlwaysUseNTMethod=false; break;
    case ARG_GDIFSBORDER: Disp.DrawLetterboxWithGDI=true; break;
    case ARG_LINKFILE:
      if(Level<10)
      {
        WIN32_FIND_DATA wfd;
        Path=GetLinkDest(Path,&wfd);
        if(Path.NotEmpty())
          ParseCommandLine(1,&(Path.Text),Level+1);
      }
      break;
#endif
#ifdef UNIX
    case ARG_NOSHM:  TrySHM=false; break;
#endif
    case ARG_NOLPT:  AllowLPT=false; break;
    case ARG_NOCOM:  AllowCOM=false; break;
    case ARG_WINDOW:
      BootInMode&=~BOOT_MODE_FLAGS_MASK;
      BootInMode=BOOT_MODE_WINDOW; break;
    case ARG_FULLSCREEN:
      BootInMode&=~BOOT_MODE_FLAGS_MASK;
      BootInMode=BOOT_MODE_FULLSCREEN; break;
    case ARG_SETSOF:
      sound_comline_freq=atoi(Path);
      sound_chosen_freq=sound_comline_freq;break;
    case ARG_SOUNDCLICK: sound_click_at_start=true; break;
    case ARG_DONTLIMITSPEED: disable_speed_limiting=true; break;
    case ARG_ACCURATEFDC: DiskMan.bTurboDrive=false; break;
    case ARG_NOPCJOYSTICKS: DisablePCJoysticks=true; break;
    case ARG_TAKESHOT:
      Disp.ScreenShotNextFile=Path;
      if(runstate==RUNSTATE_RUNNING)
        DoSaveScreenShot|=1;
      else
        Disp.SaveScreenShot();
      break;
#ifndef NO_PORTAUDIO      
    case ARG_SETPABUFSIZE:  UNIX_ONLY(pa_output_buffer_size=atoi(Path); ) break;
#endif
    case ARG_ALLOWREADOPEN: StemdosComlineReadRb=true; break;
#if !defined(VC_BUILD)
    case ARG_NOINTS: no_ints=true; break;
#endif
    case ARG_STFMBORDER:
      SSEConfig.SwitchSTModel(STF);
      break;
#if !defined(SSE_420R4) // ini option
    case ARG_SCREENSHOTUSEFULLNAME: Disp.ScreenShotUseFullName=true; break;
    case ARG_SCREENSHOTALWAYSADDNUM: Disp.ScreenShotAlwaysAddNum=true; break;
#endif
    case ARG_ALLOWLPTINPUT: comline_allow_LPT_input=true; break;
    case ARG_CROSSMOUSE: no_set_cursor_pos=true; break;
#if defined(UNIX) && !defined(NO_RTAUDIO)
    case ARG_RTBUFSIZE: rt_buffer_size=atoi(Path); break;
    case ARG_RTBUFNUM: rt_buffer_num=atoi(Path); break;
#endif
    case ARG_RUN: BootInMode|=BOOT_MODE_RUN; break;
#if USE_PASTI
    case ARG_PASTI: BootPasti=BOOT_PASTI_ON; break;
    case ARG_NOPASTI: BootPasti=BOOT_PASTI_OFF; break;
    case ARG_PASTIDISKIMAGEFILE:
      BootPasti=BOOT_PASTI_ON;
      if(BootDisk[DRIVE_B].Empty())
        BootDisk[(int)(BootDisk[DRIVE_A].Empty() ? DRIVE_A : DRIVE_B)]=Path;
      break;    
#endif
    case ARG_SNAPSHOTFILE:
      BootStateFile=Path;
      // no break
    case ARG_NOAUTOSNAPSHOT:
      BootDisk[DRIVE_A]=".";
      BootDisk[DRIVE_B]=".";
      break;
    case ARG_DISKIMAGEFILE:
        //TRACE("ARG_DISKIMAGEFILE %s A free %d B free %d\n",Path.Text,BootDisk[0].Empty(),BootDisk[1].Empty());
      if(BootDisk[DRIVE_B].Empty()||BootDisk[DRIVE_B]==".")
        BootDisk[(int)((BootDisk[DRIVE_A].Empty()||BootDisk[DRIVE_A]==".")?DRIVE_A:DRIVE_B)]=Path;
        //TRACE("Boot disks %s %s\n",BootDisk[0].Text,BootDisk[1].Text);
      break;
/*    case ARG_SNAPSHOTFILE:
      BootDisk[DRIVE_A]=".";
      BootDisk[DRIVE_B]=".";
      BootStateFile=Path;
      //TRACE_INIT("BootStateFile %s given as argument\n",GetFileNameFromPath(BootStateFile.Text));
      break;*/
    case ARG_CARTFILE:
      if(load_cart(Path))
      {
        CartFile=Path;
        OptionBox.MachineUpdateIfVisible();
      }
      break;
    case ARG_STPROGRAMFILE:
      // Mount folder as Z: (disable all normal hard drives)
      // Copy autorun program into auto folder
      break;
    case ARG_TOSIMAGEFILE:
      if(!BootTOSImage)
      {
        if(load_TOS(Path))
        {
          ROMFile=Path;
          BootTOSImage=true;
        }
      }
      break;
    }
  }
}


bool OpenComLineFilesInCurrent(bool AlwaysSendToCurrent) {
  EasyStringList esl;
  esl.Sort=eslNoSort;
  for(int n=0;n<_argc-1;n++) 
  {
    EasyStr Path;
    int Type=GetComLineArgType(_argv[1+n],Path);
#ifdef WIN32
    if(Type==ARG_LINKFILE)
    {
      WIN32_FIND_DATA wfd;
      int Level=0;
      EasyStr DestPath=Path;
      do
      {
        DestPath=GetLinkDest(DestPath,&wfd);
        if(DestPath.NotEmpty()) 
          break;
        Type=GetComLineArgType(DestPath,Path);
      } while(Type==ARG_LINKFILE && (++Level)<10);
    }
#endif
    switch(Type) {
    case ARG_DISKIMAGEFILE:case ARG_PASTIDISKIMAGEFILE:case ARG_SNAPSHOTFILE:
    case ARG_CARTFILE:case ARG_TOSIMAGEFILE:
      esl.Add(Path,0);
      // no break
    case ARG_TAKESHOT:
      esl.Add(_argv[1+n],0);
      break;
    case ARG_RUN:
      esl.Add(_argv[1+n],1);
      break;
    }
  }
  if(esl.NumStrings) 
  {
    bool RunOnly=true;
    for(int i=0;i<esl.NumStrings;i++) 
    {
      if(esl[i].Data[0]==0) 
      {
        RunOnly=false;
        break;
      }
    }
    // If you only pass the RUN command and haven't specified to open in current
    // then we shouldn't do anything, RUN is handled later.
    if(RunOnly && !AlwaysSendToCurrent) 
      return false;
    // Send strings to running Steem
#ifdef WIN32
    HWND CurSteemWin=FindWindow("Steem Window",NULL);
    if(CurSteemWin) 
    {
      bool Success=false;
      COPYDATASTRUCT cds;
      cds.dwData=MAKECHARCONST('S','C','O','M');
      for(int n=0;n<esl.NumStrings;n++) 
      {
        cds.cbData=(DWORD)strlen(esl[n].String)+1;
        cds.lpData=esl[n].String;
        if(SendMessage(CurSteemWin,WM_COPYDATA,0,LPARAM(&cds))==MAKECHARCONST('Y','A','Y','S')) 
          Success=true;
      }
      if(Success) 
        return true;
    }
#endif
  }
  return false;
}


bool CheckForSteemRunning() {
#ifdef WIN32
  SteemRunningMutex=CreateMutex(NULL,0,"Steem_Running");
  return (GetLastError()==ERROR_ALREADY_EXISTS);
#endif
#ifdef UNIX
  return true; // hack
#endif
}

#endif//#if !defined(SSE_LIBRETRONUKE)

#undef LOGSECTION
