/*
 * Dumb program to shutdown Windows
 * This code implements a very limited set of the
 * standard Unix shutdown program.
 *
 *   Kern E. Sibbald, July MM
 */

#include   <windows.h>
#include   <stdio.h>
#include   <stdlib.h>

#ifndef HAVE_MINGW
extern void mainCRTStartup();
void WinMainCRTStartup() { mainCRTStartup(); }
#endif

int main(int argc, char **argv)
{
   char *usage;
   DWORD   g_platform_id;
   DWORD   Timeout = 30;
   OSVERSIONINFO osversioninfo;
   osversioninfo.dwOSVersionInfoSize = sizeof(osversioninfo);

   // Get the current OS version
   if (!GetVersionEx(&osversioninfo))
      g_platform_id = 0;
   else
      g_platform_id = osversioninfo.dwPlatformId;
       
   /* For WinNT and Win2000, we must get permission */
   if (g_platform_id == VER_PLATFORM_WIN32_NT) { 

      HANDLE hToken; 
      TOKEN_PRIVILEGES tkp; 
       
      // Get a token for this process. 
#ifdef HAVE_CYGWIN
      if (!OpenThreadToken(GetCurrentThread(), 
         TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, FALSE, &hToken)) {
#else
      if (!OpenProcessToken(GetCurrentProcess(), 
         TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
#endif
// Forge onward anyway in the hopes of succeeding.
//       MessageBox(NULL, "System shutdown failed: OpenProcessToken", "shutdown", MB_OK);
//	 exit(1);
      } 

      // Get the LUID for the shutdown privilege. 
       
      LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, 
	      &tkp.Privileges[0].Luid); 
       
      tkp.PrivilegeCount = 1;  // one privilege to set	  
      tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED; 
       
      // Get the shutdown privilege for this process. 
       
      AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, 
	      (PTOKEN_PRIVILEGES)NULL, 0); 
       
      // Cannot test the return value of AdjustTokenPrivileges. 
       
      if (GetLastError() != ERROR_SUCCESS) {
//       MessageBox(NULL, "System shutdown failed: AdjustTokePrivileges", "shutdown", MB_OK);
//	 exit(1);
      } 
   }

   /* Now, do the real work */
   if (argc >= 2 && (strcmp(argv[1], "-c") == 0)) {   /* Cancel shutdown */
      if (!AbortSystemShutdown(
	      NULL			  /* system name */
	     )) {
         MessageBox(NULL, "Cancel Shutdown request failed.", "shutdown", MB_OK);
	 exit(1);
      } 
      exit(0);
   }

   if (argc == 2 && (strcmp(argv[1], "-r") == 0)) {   /* Reboot */
      /* Try gentle way */
      ExitWindowsEx(EWX_REBOOT, 0);
      Sleep(Timeout*1000);
      /* Force it */
      if (!ExitWindowsEx(EWX_REBOOT|EWX_FORCE, 0)) {
         MessageBox(NULL, "System reboot request failed.", "shutdown", MB_OK);
	 exit(1);
      } 
      exit(0);
   }

   if (argc != 3 || strcmp(argv[1], "-h")) {
      usage = "Incorrect command line arguments given to Shutdown\n\n\
Usage: shutdown [-chr] <now>\n\
       -c:      cancel shutdown.\n\
       -h:      halt after shutdown. (requires now)\n\
       -r:      reboot after shutdown.\n\n";
      printf(usage);
      MessageBox(NULL, usage, "Shutdown", MB_OK);
      exit(1);
   }

   if (strcmp(argv[2], "now") == 0) {
      Timeout = 30;		      /* give 30 seconds anyway */
   } else {
      Timeout = atoi(argv[2]);
      if (Timeout < 0) {
	 Timeout = 0;
      }
   }

   /* For WinNT and Win2000, we must do it differently */
   if (g_platform_id == VER_PLATFORM_WIN32_NT) { 
      /* Try gentle way */
      ExitWindowsEx(EWX_SHUTDOWN, 0);
      /* Now force everything after 30 second warning */
      if (!InitiateSystemShutdown(
	 NULL,			      /* system name */
         "Power failure system going down!", /* message */
	 Timeout,		      /* Time */
	 1,			      /* Force close option */
	 0			      /* reboot option */
	 )) {
         MessageBox(NULL, "System shutdown request failed.", "shutdown", MB_OK);
	 exit(1);
      }

   } else {		    /* Win98/95 */
      /* Try gentle way */
      ExitWindowsEx(EWX_SHUTDOWN, 0);

      Sleep(Timeout*1000);

      /* Force it */
      if (!ExitWindowsEx(EWX_SHUTDOWN|EWX_FORCE, 0)) {
         MessageBox(NULL, "System shutdown request failed.", "shutdown", MB_OK);
	 exit(1);
      }
   }
   exit(0);
}
