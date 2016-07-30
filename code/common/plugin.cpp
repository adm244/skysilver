/*
  THIS FILE IS A PART OF THE SKYRIM DRAGON SCRIPT PROJECT 
        (C) Alexander Blade 2011
      http://Alexander.SannyBuilder.com
*/

/*
  Changes were made to this file:
    - added two new functions
    - rewritten GetKeyPressed(..) function
*/

#include "plugin.h"
#include "skyscript.h"
#include <windows.h>
#include <stdio.h>

TNativeCall NativeCall;            
TObscriptCall ObscriptCall;        
TGetPlayerObjectHandle GetPlayerObjectHandle; // same as Game::GetPlayer()
TGetConsoleSelectedRef GetConsoleSelectedRef; // gets the object ref selected in the console menu
Tdyn_cast dyn_cast; // object dynamic casting
TRegisterPlugin RegisterPlugin;        
TWait Wait;                  
TBSString_Create BSString_Create;      
TBSString_Free BSString_Free; 
TExecuteConsoleCommand ExecuteConsoleCommand; // executes command just like you typed in in the console
                                              // 2nd param is parent handle which can be 0 or point to a ref
DWORD ___stack[MAX_STACK_LEN];
DWORD ___stackindex;
DWORD ___result;

HMODULE g_hModule;

void Error(char *pattern, ...)
{
  char text[1024];
  va_list lst;
  va_start(lst, pattern);
  vsprintf_s(text, pattern, lst);
  va_end(lst);
  MessageBoxA(0, text, "ScriptDragon plugin critical error", MB_ICONERROR); 
  ExitProcess(0);
}

std::string GetKeyName(BYTE key)
{
  DWORD sc = MapVirtualKeyA(key, 0);
  // check key for ascii
  BYTE buf[256];
  memset(buf, 0, 256);
  WORD temp;
  DWORD asc = (key <= 32);
  if (!asc && (key != VK_DIVIDE)) asc = ToAscii(key, sc, buf, &temp, 1);
  // set bits
  sc <<= 16;
  sc |= 0x1 << 25;  // <- don't care
  if (!asc) sc |= 0x1 << 24; // <- extended bit
  // convert to ansi string
  if (GetKeyNameTextA(sc, (char *)buf, sizeof(buf))) 
    return (char *)buf;
    else return "";
}

std::string GetPathFromFilename(std::string filename)
{
  return filename.substr(0, filename.rfind("\\") + 1);
}

int IniReadInt(char *inifile, char *section, char *param, int def)
{
  char curdir[MAX_PATH];
  GetModuleFileNameA(g_hModule, curdir, sizeof(curdir));
  std::string fname = GetPathFromFilename(curdir) + inifile;
  return GetPrivateProfileIntA(section, param, def, fname.c_str());
}

bool IniReadBool(char *inifile, char *section, char *param, bool def)
{
  int value = IniReadInt(inifile, section, param, def ? 1 : 0);
  return(value != 0 ? true : false);
}

//NOTE(adm244): retrieves a string value from specified section and value of ini file and stores it in buffer
DWORD IniReadString(char *inifile, char *section, char *param, char *buffer, DWORD bufsize, char *def)
{
  char curdir[MAX_PATH];
  GetModuleFileNameA(g_hModule, curdir, sizeof(curdir));
  std::string fname = GetPathFromFilename(curdir) + inifile;
  return GetPrivateProfileStringA(section, param, def, buffer, bufsize, fname.c_str());
}

//NOTE(adm244): retrieves all key-value pairs from specified section of ini file and stores it in buffer
DWORD IniReadSection(char *inifile, char *section, char *buffer, DWORD bufsize)
{
  char curdir[MAX_PATH];
  GetModuleFileNameA(g_hModule, curdir, sizeof(curdir));
  std::string fname = GetPathFromFilename(curdir) + inifile;
  return GetPrivateProfileSectionA(section, buffer, bufsize, fname.c_str());
}

//NOTE(adm244): rewritten function
bool GetKeyPressed(BYTE key)
{
  //IMPORTANT(adm244): GetKeyState returns 16-bit integer as MSDN says,
  // so why the heck this mask is cheking highest bit in 32-bit integer?!
  // https://msdn.microsoft.com/en-us/library/windows/desktop/ms646301(v=vs.85).aspx
  // I've checked the value GetKeyState returns and it's ACTUALLY a 32-bit integer (windows 7 sp1),
  // but the weird thing is that IT'S STILL puts value that indicates whether the key
  // is pressed or not in highest bit of LOWER 16-bit part (for backwards-compability??)
  // and fills HIGHER 16-bit with ones WHEN corresponding key is pressed and with zeros when it's not
  //return (GetKeyState(key) & 0x80000000) > 0;
  
  SHORT keystate = (SHORT)GetAsyncKeyState(key);
  
  //NOTE(adm244): GetAsyncKeyState does not wait for message queue to be proccessed,
  // so it should retrive the state as soon as this function is called
  return( (keystate & 0x8000) > 0 );
}

void PrintDebug(char *pattern, ...)
{
  char text[1024];
  va_list lst;
  va_start(lst, pattern);
  vsprintf_s(text, pattern, lst);
  va_end(lst);
  OutputDebugStringA(text);
}

void PrintNote(char *pattern, ...)
{
  char text[1024];
  va_list lst;
  va_start(lst, pattern);
  vsprintf_s(text, pattern, lst);
  va_end(lst);
  OutputDebugStringA(text);
  Debug::Notification(text);
}

/*
  DO NOT CHANGE ANYTHING BELOW FOR ANY MEANS
*/

#define SCRIPT_DRAGON "ScriptDragon.dll" 

void DragonPluginInit(HMODULE hModule)
{
  HMODULE hDragon = LoadLibraryA(SCRIPT_DRAGON);
  /* 
  In order to provide NORMAL support i need a plugins to be distributed without the DragonScript.dll engine 
  cuz user always must have the latest version which cud be found ONLY on my web page
  */
  if (!hDragon) Error("Can't load %s, download latest version from http://alexander.sannybuilder.com/Files/tes/", SCRIPT_DRAGON);
  NativeCall = (TNativeCall)GetProcAddress(hDragon, "Nativecall");
  ObscriptCall = (TObscriptCall)GetProcAddress(hDragon, "Obscriptcall");
  GetPlayerObjectHandle = (TGetPlayerObjectHandle)GetProcAddress(hDragon, "GetPlayerObjectHandle");
  ExecuteConsoleCommand = (TExecuteConsoleCommand)GetProcAddress(hDragon, "ExecuteConsoleCommand");
  GetConsoleSelectedRef = (TGetConsoleSelectedRef)GetProcAddress(hDragon, "GetConsoleSelectedRef");
  dyn_cast = (Tdyn_cast)GetProcAddress(hDragon, "dyn_cast");
  RegisterPlugin = (TRegisterPlugin)GetProcAddress(hDragon, "RegisterPlugin");
  Wait = (TWait)GetProcAddress(hDragon, "WaitMs");
  BSString_Create = (TBSString_Create)GetProcAddress(hDragon, "BSString_Create");
  BSString_Free = (TBSString_Free)GetProcAddress(hDragon, "BSString_Free");

  if(!NativeCall || !ObscriptCall || !GetPlayerObjectHandle || !ExecuteConsoleCommand 
    || !GetConsoleSelectedRef || !dyn_cast || !RegisterPlugin || !Wait
    || !BSString_Create || !BSString_Free)
  {
    Error("ScriptDragon engine dll `%s` has not all needed functions inside, exiting", SCRIPT_DRAGON);
  }

  RegisterPlugin(hModule);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD fdwReason, LPVOID lpReserved)
{
  switch (fdwReason)
  {
    case DLL_PROCESS_ATTACH:
    {
      g_hModule = hModule;
      DragonPluginInit(hModule);
      break;
    }
    
    case DLL_PROCESS_DETACH:
    {
      break;
    }
  }
  return TRUE;
}