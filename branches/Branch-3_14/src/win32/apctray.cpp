#include "winapi.h"
#include <unistd.h>
#include <windows.h>
#include <lmcons.h>
#include <ctype.h>
#include <signal.h>
#include <errno.h>

#include "apc.h"
#include "wintray.h"
#include "winres.h"
#include "winups.h"
#include "statmgr.h"

#include <vector>
#include <string>

#define CMDOPT_PORT     "/port"
#define CMDOPT_HOST     "/host"
#define CMDOPT_REFRESH  "/refresh"
#define CMDOPT_INSTALL  "/install"
#define CMDOPT_REMOVE   "/remove"
#define CMDOPT_KILL     "/kill"
#define CMDOPT_QUIET    "/quiet"
#define CMDOPT_ADD      "/add"
#define CMDOPT_DEL      "/del"

#define USAGE_TEXT   "[" CMDOPT_HOST    " <hostname>] " \
                     "[" CMDOPT_PORT    " <port>] "     \
                     "[" CMDOPT_REFRESH " <sec>] "      \
                     "[" CMDOPT_INSTALL "] "            \
                     "[" CMDOPT_REMOVE  "] "            \
                     "[" CMDOPT_KILL    "] "            \
                     "[" CMDOPT_ADD     "] "            \
                     "[" CMDOPT_DEL     "] "            \
                     "[" CMDOPT_QUIET   "]"

#define DEFAULT_HOST    "127.0.0.1"
#define DEFAULT_PORT    3551
#define DEFAULT_REFRESH 1


class TrayInstance; // Forward decl

HINSTANCE appinst;                       // Application handle
std::vector<TrayInstance*> instances;    // List of icon/menu instances
bool quiet = false;                      // Suppress user dialogs

class TrayInstance
{
public:
   TrayInstance(const char *host, unsigned short port, int refresh)
      : m_host(strdup(host)),
        m_port(port),
        m_refresh(refresh)
   {
      m_menu = new upsMenu(appinst, m_host, m_port, m_refresh);
   }

   ~TrayInstance() {
      delete m_menu;
      free(m_host);
   }

   char *m_host;
   unsigned short m_port;
   int m_refresh;
   upsMenu *m_menu;
};

void NotifyError(const char *format, ...)
{
   va_list args;
   char buf[2048];

   va_start(args, format);
   avsnprintf(buf, sizeof(buf), format, args);
   va_end(args);

   MessageBox(NULL, buf, "Apctray", MB_OK|MB_ICONEXCLAMATION);
}

void NotifyUser(const char *format, ...)
{
   va_list args;
   char buf[2048];

   if (!quiet) {
      va_start(args, format);
      avsnprintf(buf, sizeof(buf), format, args);
      va_end(args);

      MessageBox(NULL, buf, "Apctray", MB_OK|MB_ICONINFORMATION);
   }
}

DWORD QueryInstanceDWORD(HKEY instance, const char *name)
{
   DWORD result;
   DWORD len = sizeof(result);

   // Retrieve DWORD
   if (RegQueryValueEx(instance, name, NULL, NULL, (BYTE*)&result, &len)
         != ERROR_SUCCESS) {
      result = 0;
   }

   return result;
}

char *QueryInstanceString(HKEY instance, const char *name)
{
   // Determine string length
   DWORD len = 0;
   BYTE dummy;
   if (RegQueryValueEx(instance, name, NULL, NULL, &dummy, &len)
         != ERROR_MORE_DATA) {
      return NULL;
   }

   // Allocate storage for string
   char *data = (char*)malloc(len);
   if (!data)
      return NULL;

   // Retrieve string
   if (RegQueryValueEx(instance, name, NULL, NULL, (BYTE*)data, &len)
         != ERROR_SUCCESS) {
      free(data);
      return NULL;
   }

   return data;
}

void SetInstanceDWORD(HKEY instance, char *name, DWORD value)
{
   RegSetValueEx(instance, name, 0, REG_DWORD, (BYTE*)&value, sizeof(value));
}

void SetInstanceString(HKEY instance, char *name, char *value)
{
   RegSetValueEx(instance, name, 0, REG_SZ, (BYTE*)value, strlen(value)+1);
}

HKEY FindInstanceKey(char *host, unsigned short port, std::string &instname)
{
   // Open registry key apctray
   HKEY apctray;
   if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Software\\Apcupsd\\Apctray",
                    0, KEY_READ|KEY_WRITE, &apctray) != ERROR_SUCCESS) {
      // No instances in registry
      return NULL;
   }

   // Iterate though all apctray instance keys, searching for one
   // with the given host and port. These parameters uniquely identify
   // a given instance.
   HKEY instance;
   int i = 0;
   char name[1024];
   DWORD len = sizeof(name);
   while (RegEnumKeyEx(apctray, i++, name, &len, NULL, NULL,
                       NULL, NULL) == ERROR_SUCCESS) {
      if (len && strncasecmp(name, "instance", 8) == 0 &&
          RegOpenKeyEx(apctray, name, 0, KEY_READ|KEY_WRITE, &instance)
             == ERROR_SUCCESS) {
         char *testhost = QueryInstanceString(instance, "host");
         unsigned short testport = QueryInstanceDWORD(instance, "port");
         if (testhost && strcmp(testhost, host) == 0 && testport == port) {
            RegCloseKey(apctray);
            instname = name;
            return instance;
         }
         free(testhost);
         RegCloseKey(instance);
      }

      len = sizeof(name);
   }

   // If we get here we've been through all the instance keys
   // and did not find a match
   RegCloseKey(apctray);
   return NULL;
}

HKEY CreateInstanceKey()
{
   // Open registry key apctray
   HKEY apctray;
   if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Software\\Apcupsd\\Apctray",
                    0, KEY_READ|KEY_WRITE, &apctray) != ERROR_SUCCESS) {
      // No instances in registry
      return NULL;
   }

   // Iterate until a new instance key is successfully created
   HKEY result = NULL;
   DWORD disposition;
   LONG status;
   char name[20];
   int i = 0;
   do {
      asnprintf(name, sizeof(name), "instance%d", i++);
      status = RegCreateKeyEx(apctray, name, 0, NULL, REG_OPTION_NON_VOLATILE,
                              KEY_READ|KEY_WRITE, NULL, &result, &disposition);
   } while (status == ERROR_SUCCESS && disposition == REG_OPENED_EXISTING_KEY);

   RegCloseKey(apctray);
   return result;
}

int Install()
{
   // Get the full path/filename of this executable
   char path[1024];
   GetModuleFileName(NULL, path, sizeof(path));

   // Add double quotes
   char cmd[1024];
   asnprintf(cmd, sizeof(cmd), "\"%s\"", path);

   // Open registry key for auto-run programs
   HKEY runkey;
   if (RegCreateKey(HKEY_LOCAL_MACHINE, 
                    "Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                    &runkey) != ERROR_SUCCESS) {
      NotifyUser("The System Registry could not be updated.\n"
                 "Apctray was not installed.");
      return 1;
   }

   // Attempt to add Apctray key
   if (RegSetValueEx(runkey, "Apctray", 0, REG_SZ,
         (unsigned char *)cmd, strlen(cmd)+1) != ERROR_SUCCESS) {
      RegCloseKey(runkey);
      NotifyUser("The System Registry could not be updated.\n"
                 "Apctray was not installed.");
      return 1;
   }

   RegCloseKey(runkey);

   NotifyUser("Apctray was installed successfully and will\n"
              "automatically run when users log on.");
}

int AddInstance(char *host, unsigned short port, int refresh)
{
   // Establish defaults for missing parameters
   if (!host) host = DEFAULT_HOST;
   if (!port) port = DEFAULT_PORT;
   if (refresh < 1) refresh = DEFAULT_REFRESH;

   // Find existing instance or create a new one
   std::string instname;
   HKEY instance = FindInstanceKey(host, port, instname);
   if (!instance) {
      instance = CreateInstanceKey();
      if (!instance)
         return 1;
   }

   // Set parameters
   SetInstanceString(instance, "host", host);
   SetInstanceDWORD(instance, "port", port);
   SetInstanceDWORD(instance, "refresh", refresh);

   // Done
   RegCloseKey(instance);

   NotifyUser("The instance (%s:%d) was successfully created.", host, port);

   return 0;
}

int CountInstances(HKEY apctray)
{
   int count = 0;

   HKEY instance;
   int i = 0;
   char name[1024];
   DWORD len = sizeof(name);

   while (RegEnumKeyEx(apctray, i++, name, &len, NULL, NULL,
                       NULL, NULL) == ERROR_SUCCESS) {
      if (len && strncasecmp(name, "instance", 8) == 0)
         count++;

      len = sizeof(name);
   }

   return count;
}

int Remove()
{
   // Open registry key apctray
   HKEY runkey;
   if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
                    "Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                    0, KEY_READ|KEY_WRITE, &runkey) == ERROR_SUCCESS) {
      RegDeleteValue(runkey, "Apctray");
      RegCloseKey(runkey);
   }

   NotifyUser("Apctray will no longer start automatically.");
   return 0;
}

int DelInstance(char *host, unsigned short port)
{
   // Open registry key apctray
   HKEY apctray;
   if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Software\\Apcupsd\\Apctray",
                    0, KEY_READ|KEY_WRITE, &apctray) != ERROR_SUCCESS) {
      // No instances in registry
      return 0;
   }

   HKEY instance;
   int i = 0;
   char name[1024];
   DWORD len = sizeof(name);

   // If no host or port was specified, remove all instances
   if (!host && !port) {
      // Iterate through all instances and delete them
      while (RegEnumKeyEx(apctray, i++, name, &len, NULL, NULL,
                          NULL, NULL) == ERROR_SUCCESS) {
         if (len && strncasecmp(name, "instance", 8) == 0 &&
             RegDeleteKey(apctray, name) == ERROR_SUCCESS) {
            i = 0;
         }

         len = sizeof(name);
      }

      NotifyUser("All instances were successfully deleted.");
   } else {
      // Establish defaults for missing parameters
      if (!host) host = DEFAULT_HOST;
      if (!port) port = DEFAULT_PORT;

      // Find and delete the appropriate instance
      std::string instname;
      instance = FindInstanceKey(host, port, instname);
      if (instance != NULL) {
         RegCloseKey(instance);
         RegDeleteKey(apctray, instname.c_str());
      }

      NotifyUser("The specified instance (%s:%d) was successfully deleted.",
         host, port);
   }

   RegCloseKey(apctray);
   return 0;
}

int Kill()
{
   HWND wnd;
   while ((wnd = FindWindow(APCTRAY_WINDOW_CLASS, NULL)) != NULL) {
      PostMessage(wnd, WM_CLOSE, 0, 0);
      Sleep(100);
   }
   return 0;
}

void Usage(const char *text1, const char* text2)
{
   MessageBox(NULL, text1, text2, MB_OK);
   MessageBox(NULL, USAGE_TEXT, "Apctray Usage",
              MB_OK | MB_ICONINFORMATION);
}

void AllocateInstance(char *host, unsigned short port, int refresh)
{
   TrayInstance *inst;

   if (host && port && refresh &&
       (inst = new TrayInstance(host, port, refresh))) {
      instances.push_back(inst);
   }
}

void AllocateInstance(HKEY instance)
{
   char *host;
   DWORD port, refresh;
   upsMenu *menu;

   host = QueryInstanceString(instance, "host");
   port = QueryInstanceDWORD(instance, "port");
   refresh = QueryInstanceDWORD(instance, "refresh");

   AllocateInstance(host, port, refresh);

   free(host);
}

void LaunchInstances()
{
   // Open registry key apctray
   HKEY apctray;
   if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Software\\Apcupsd\\Apctray",
                    0, KEY_READ, &apctray) != ERROR_SUCCESS) {
      // No instances in registry
      return;
   }

   // Iterate though all apctray instance keys, allocating
   // a tray icon for each of them
   int i = 0;
   char name[1024];
   DWORD len = sizeof(name);
   while (RegEnumKeyEx(apctray, i++, name, &len, NULL, NULL,
                       NULL, NULL) == ERROR_SUCCESS) {
      HKEY instance;
      if (len && strncasecmp(name, "instance", 8) == 0 &&
          RegOpenKeyEx(apctray, name, 0, KEY_READ, &instance) == ERROR_SUCCESS) {
         AllocateInstance(instance);
         RegCloseKey(instance);
      }

      len = sizeof(name);
   }

   RegCloseKey(apctray);
}

void CloseInstance(upsMenu *menu)
{
   std::vector<TrayInstance*>::iterator iter;

   for (iter = instances.begin(); iter != instances.end(); iter++) {
      if ((*iter)->m_menu == menu) {
         delete *iter;
         instances.erase(iter);
         break;
      }
   }
}

void RemoveInstance(upsMenu *menu)
{
   std::vector<TrayInstance*>::iterator iter;

   for (iter = instances.begin(); iter != instances.end(); iter++) {
      if ((*iter)->m_menu == menu) {
         DelInstance((*iter)->m_host, (*iter)->m_port);
         CloseInstance(menu);
         break;
      }
   }
}

// WinMain parses the command line and either calls the main App
// routine or, under NT, the main service routine.
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   PSTR CmdLine, int iCmdShow)
{
   InitWinAPIWrapper();
   WSA_Init();

   // Publicize application handle
   appinst = hInstance;

   // Instance parameters
   char *host = NULL;
   unsigned short port = 0;
   int refresh = 0;

   // Check command line options
   char *arg;
   char *opt = CmdLine;
   while ((arg = GetArg(&opt))) {
      if (strcasecmp(arg, CMDOPT_HOST) == 0) {
         if (!(arg = GetArg(&opt))) {
            Usage(CMDOPT_HOST, "Option requires string argument");
            return 1;
         }
         host = arg;
      } else if (strcasecmp(arg, CMDOPT_PORT) == 0) {
         if (!(arg = GetArg(&opt))) {
            Usage(CMDOPT_PORT, "Option requires integer argument");
            return 1;
         }
         port = strtoul(arg, NULL, 0);
      } else if (strcasecmp(arg, CMDOPT_REFRESH) == 0) {
         if (!(arg = GetArg(&opt))) {
            Usage(CMDOPT_REFRESH, "Option requires integer argument");
            return 1;
         }
         refresh = strtoul(arg, NULL, 0);
      } else if (strcasecmp(arg, CMDOPT_INSTALL) == 0) {
         return Install();
      } else if (strcasecmp(arg, CMDOPT_REMOVE) == 0) {
         return Remove();
      } else if (strcasecmp(arg, CMDOPT_ADD) == 0) {
         return AddInstance(host, port, refresh);
      } else if (strcasecmp(arg, CMDOPT_DEL) == 0) {
         return DelInstance(host, port);
      } else if (strcasecmp(arg, CMDOPT_KILL) == 0) {
         return Kill();
      } else if (strcasecmp(arg, CMDOPT_QUIET) == 0) {
         quiet = true;
      } else {
         Usage(arg, "Unknown option");
         return 1;
      }
   }

   if (refresh < 1) refresh = DEFAULT_REFRESH;

   if (!host && !port) {
      // No command line instance options were given: Launch
      // all instances specified in the registry
      LaunchInstances();

      // If no instances were created from the registry,
      // allocate a default one
      if (instances.empty())
         AllocateInstance(DEFAULT_HOST, DEFAULT_PORT, refresh);
   } else {
      // One or more command line options were given, so launch a single
      // instance using the specified parameters, filling in any missing
      // ones with defaults
      if (!host) host = DEFAULT_HOST;
      if (!port) port = DEFAULT_PORT;
      AllocateInstance(host, port, refresh);
   }

   // Enter the Windows message handling loop until told to quit
   MSG msg;
   while (GetMessage(&msg, NULL, 0, 0)) {
      TranslateMessage(&msg);

      switch (LOWORD(msg.message)) {
      case WM_CLOSEINST:
         // Close specified instance
         CloseInstance((upsMenu*)msg.lParam);
         if (instances.empty())
            PostQuitMessage(0);
         break;

      case WM_REMOVEALL:
         // Remove all instances (and close)
         DelInstance(NULL, 0);
         PostQuitMessage(0);
         break;

      case WM_REMOVE:
         // Remove the given instance
         RemoveInstance((upsMenu*)msg.lParam);
         if (instances.empty())
            PostQuitMessage(0);
         break;

      default:
         DispatchMessage(&msg);
      }
   }

   // Free all instances
   while (!instances.empty()) {
      delete instances.back();
      instances.pop_back();
   }

   WSACleanup();
   return 0;
}
