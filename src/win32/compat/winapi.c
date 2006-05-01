/*
 * Windows APIs that are different for each system.
 *   We use pointers to the entry points so that a
 *   single binary will run on all Windows systems.
 *
 *     Kern Sibbald MMIII
 */
/*
   Copyright (C) 2003-2006 Kern Sibbald

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as amended with additional clauses defined in the
   file LICENSE in the main source directory.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the 
   the file LICENSE for additional details.

 */

#ifdef __APCUPSD__
#include "apc.h"
#else
#include "bacula.h"
#endif

#if defined(HAVE_CYGWIN) || defined(HAVE_WIN32)

#include "winapi.h"

int win32_client = 1;

#ifdef WIN32_VSS
#include "vss.h"   
#endif

// init with win9x, but maybe set to NT in InitWinAPI
DWORD  g_platform_id = VER_PLATFORM_WIN32_WINDOWS;
#ifdef WIN32_VSS
/* preset VSSClient to NULL */
VSSClient *g_pVSSClient = NULL;
#endif


/* API Pointers */

t_OpenProcessToken      p_OpenProcessToken = NULL;
t_AdjustTokenPrivileges p_AdjustTokenPrivileges = NULL;
t_LookupPrivilegeValue  p_LookupPrivilegeValue = NULL;

t_SetProcessShutdownParameters p_SetProcessShutdownParameters = NULL;

t_CreateFileA   p_CreateFileA = NULL;
t_CreateFileW   p_CreateFileW = NULL;
t_CreateDirectoryA   p_CreateDirectoryA;
t_CreateDirectoryW   p_CreateDirectoryW;

t_wunlink p_wunlink = NULL;
t_wmkdir p_wmkdir = NULL;
t_wopen p_wopen = NULL;

t_GetFileAttributesA    p_GetFileAttributesA = NULL;
t_GetFileAttributesW    p_GetFileAttributesW = NULL;

t_GetFileAttributesExA  p_GetFileAttributesExA = NULL;
t_GetFileAttributesExW  p_GetFileAttributesExW = NULL;

t_SetFileAttributesA    p_SetFileAttributesA = NULL;
t_SetFileAttributesW    p_SetFileAttributesW = NULL;
t_BackupRead            p_BackupRead = NULL;
t_BackupWrite           p_BackupWrite = NULL;
t_WideCharToMultiByte p_WideCharToMultiByte = NULL;
t_MultiByteToWideChar p_MultiByteToWideChar = NULL;

t_FindFirstFileA p_FindFirstFileA = NULL;
t_FindFirstFileW p_FindFirstFileW = NULL;

t_FindNextFileA p_FindNextFileA = NULL;
t_FindNextFileW p_FindNextFileW = NULL;

t_SetCurrentDirectoryA p_SetCurrentDirectoryA = NULL;
t_SetCurrentDirectoryW p_SetCurrentDirectoryW = NULL;

t_GetCurrentDirectoryA p_GetCurrentDirectoryA = NULL;
t_GetCurrentDirectoryW p_GetCurrentDirectoryW = NULL;

t_GetVolumePathNameW p_GetVolumePathNameW = NULL;
t_GetVolumeNameForVolumeMountPointW p_GetVolumeNameForVolumeMountPointW = NULL;

#ifdef WIN32_VSS
void 
VSSCleanup()
{
   if (g_pVSSClient)
      delete (g_pVSSClient);
}
#endif

void 
InitWinAPIWrapper() 
{
   HMODULE hLib = LoadLibraryA("KERNEL32.DLL");
   if (hLib) {
      /* create file calls */
      p_CreateFileA = (t_CreateFileA)
          GetProcAddress(hLib, "CreateFileA");
      p_CreateFileW = (t_CreateFileW)
          GetProcAddress(hLib, "CreateFileW");      

      p_CreateDirectoryA = (t_CreateDirectoryA)
          GetProcAddress(hLib, "CreateDirectoryA");
      p_CreateDirectoryW = (t_CreateDirectoryW)
          GetProcAddress(hLib, "CreateDirectoryW");      

      /* attribute calls */
      p_GetFileAttributesA = (t_GetFileAttributesA)
          GetProcAddress(hLib, "GetFileAttributesA");
      p_GetFileAttributesW = (t_GetFileAttributesW)
          GetProcAddress(hLib, "GetFileAttributesW");
      p_GetFileAttributesExA = (t_GetFileAttributesExA)
          GetProcAddress(hLib, "GetFileAttributesExA");
      p_GetFileAttributesExW = (t_GetFileAttributesExW)
          GetProcAddress(hLib, "GetFileAttributesExW");
      p_SetFileAttributesA = (t_SetFileAttributesA)
          GetProcAddress(hLib, "SetFileAttributesA");
      p_SetFileAttributesW = (t_SetFileAttributesW)
          GetProcAddress(hLib, "SetFileAttributesW");
      /* process calls */
      p_SetProcessShutdownParameters = (t_SetProcessShutdownParameters)
          GetProcAddress(hLib, "SetProcessShutdownParameters");
      /* backup calls */
      p_BackupRead = (t_BackupRead)
          GetProcAddress(hLib, "BackupRead");
      p_BackupWrite = (t_BackupWrite)
          GetProcAddress(hLib, "BackupWrite");
      /* char conversion calls */
      p_WideCharToMultiByte = (t_WideCharToMultiByte)
          GetProcAddress(hLib, "WideCharToMultiByte");
      p_MultiByteToWideChar = (t_MultiByteToWideChar)
          GetProcAddress(hLib, "MultiByteToWideChar");

      /* find files */
      p_FindFirstFileA = (t_FindFirstFileA)
          GetProcAddress(hLib, "FindFirstFileA"); 
      p_FindFirstFileW = (t_FindFirstFileW)
          GetProcAddress(hLib, "FindFirstFileW");       
      p_FindNextFileA = (t_FindNextFileA)
          GetProcAddress(hLib, "FindNextFileA");
      p_FindNextFileW = (t_FindNextFileW)
          GetProcAddress(hLib, "FindNextFileW");
      /* set and get directory */
      p_SetCurrentDirectoryA = (t_SetCurrentDirectoryA)
          GetProcAddress(hLib, "SetCurrentDirectoryA");
      p_SetCurrentDirectoryW = (t_SetCurrentDirectoryW)
          GetProcAddress(hLib, "SetCurrentDirectoryW");       
      p_GetCurrentDirectoryA = (t_GetCurrentDirectoryA)
          GetProcAddress(hLib, "GetCurrentDirectoryA");
      p_GetCurrentDirectoryW = (t_GetCurrentDirectoryW)
          GetProcAddress(hLib, "GetCurrentDirectoryW");      

      /* some special stuff we need for VSS
         but statically linkage doesn't work on Win 9x */
      p_GetVolumePathNameW = (t_GetVolumePathNameW)
          GetProcAddress(hLib, "GetVolumePathNameW");
      p_GetVolumeNameForVolumeMountPointW = (t_GetVolumeNameForVolumeMountPointW)
          GetProcAddress(hLib, "GetVolumeNameForVolumeMountPointW");
    
      FreeLibrary(hLib);
   }
   
   hLib = LoadLibraryA("MSVCRT.DLL");
   if (hLib) {
      /* unlink */
      p_wunlink = (t_wunlink)
      GetProcAddress(hLib, "_wunlink");
      /* wmkdir */
      p_wmkdir = (t_wmkdir)
      GetProcAddress(hLib, "_wmkdir");
      /* wopen */
      p_wopen = (t_wopen)
      GetProcAddress(hLib, "_wopen");
        
      FreeLibrary(hLib);
   }
   
   hLib = LoadLibraryA("ADVAPI32.DLL");
   if (hLib) {
      p_OpenProcessToken = (t_OpenProcessToken)
         GetProcAddress(hLib, "OpenProcessToken");
      p_AdjustTokenPrivileges = (t_AdjustTokenPrivileges)
         GetProcAddress(hLib, "AdjustTokenPrivileges");
      p_LookupPrivilegeValue = (t_LookupPrivilegeValue)
         GetProcAddress(hLib, "LookupPrivilegeValueA");
      FreeLibrary(hLib);
   }

   // do we run on win 9x ???
   OSVERSIONINFO osversioninfo;   
   osversioninfo.dwOSVersionInfoSize = sizeof(osversioninfo);

   DWORD dwMinorVersion;

   // Get the current OS version
   if (!GetVersionEx(&osversioninfo)) {
      g_platform_id = 0;
      dwMinorVersion = 0;
   } else {
      g_platform_id = osversioninfo.dwPlatformId;
      dwMinorVersion = osversioninfo.dwMinorVersion;
   }

   /* deinitialize some routines on win95 - they're not implemented well */
   if (g_platform_id == VER_PLATFORM_WIN32_WINDOWS) {
      p_BackupRead = NULL;
      p_BackupWrite = NULL;

      p_CreateFileW = NULL;          
      p_GetFileAttributesW = NULL;          
      p_GetFileAttributesExW = NULL;
          
      p_SetFileAttributesW = NULL;
                
      p_FindFirstFileW = NULL;
      p_FindNextFileW = NULL;
      p_SetCurrentDirectoryW = NULL;
      p_GetCurrentDirectoryW = NULL;

      p_wunlink = NULL;
      p_wmkdir = NULL;
      p_wopen = NULL;

      p_GetVolumePathNameW = NULL;
      p_GetVolumeNameForVolumeMountPointW = NULL;
   }   

   /* decide which vss class to initialize */
#ifdef WIN32_VSS
   switch (dwMinorVersion) {
      case 1: 
         g_pVSSClient = new VSSClientXP();
         atexit(VSSCleanup);
         break;
      case 2: 
         g_pVSSClient = new VSSClient2003();
         atexit(VSSCleanup);
         break;
   }
#endif /* WIN32_VSS */
}

#else

/* Not Windows */
int win32_client = 0;

#endif
