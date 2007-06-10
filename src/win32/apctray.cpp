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
#define CMDOPT_NOTIFY   "/notify"

#define USAGE_TEXT   "[" CMDOPT_HOST    " <hostname>] " \
                     "[" CMDOPT_PORT    " <port>] "     \
                     "[" CMDOPT_REFRESH " <sec>] "      \
                     "[" CMDOPT_NOTIFY  "] "            \
                     "[" CMDOPT_INSTALL "] "            \
                     "[" CMDOPT_REMOVE  "] "            \
                     "[" CMDOPT_KILL    "] "            \
                     "[" CMDOPT_ADD     "] "            \
                     "[" CMDOPT_DEL     "] "            \
                     "[" CMDOPT_QUIET   "]"

#define DEFAULT_HOST    "127.0.0.1"
#define DEFAULT_PORT    3551
#define DEFAULT_REFRESH 1
#define DEFAULT_NOTIFY  0


class TrayInstance; // Forward decl

HINSTANCE appinst;                       // Application handle
std::vector<TrayInstance*> instances;    // List of icon/menu instances
bool quiet = false;                      // Suppress user dialogs

class TrayInstance
{
public:
   TrayInstance(const char *host, unsigned short port, int refresh, int notify)
      : m_host(strdup(host)),
        m_port(port),
        m_refresh(refresh),
        m_notify(notify),
        m_menu(NULL)
   {
      // Insert defaults where needed
      m_host = host ? strdup(host) : strdup(DEFAULT_HOST);
      m_port = (port > 0) ? port : DEFAULT_PORT;
      m_refresh = (refresh > 0) ? refresh : DEFAULT_REFRESH;
      m_notify = (notify >= 0) ? notify : DEFAULT_NOTIFY;
   }

   TrayInstance(HKEY instkey)
      : m_menu(NULL)
   {
      // Read values from registry
      m_host = RegQueryString(instkey, "host");
      m_port = RegQueryDWORD(instkey, "port");
      m_refresh = RegQueryDWORD(instkey, "refresh");
      m_notify = RegQueryDWORD(instkey, "notify");

      // Insert defaults where needed
      if (!m_host) m_host = strdup(DEFAULT_HOST);
      if (m_port < 1) m_port = DEFAULT_PORT;
      if (m_refresh < 1) m_refresh = DEFAULT_REFRESH;
      if (m_notify < 0) m_notify = DEFAULT_NOTIFY;
   }

   ~TrayInstance() {
      delete m_menu;
      free(m_host);
   }

   void Create() {
      m_menu = new upsMenu(appinst, m_host, m_port, m_refresh, m_notify);
   }

   void Destroy() {
      if (m_menu)
         m_menu->Destroy();
   }

   // Registry helpers
   HKEY FindInstanceKey(std::string &instname);
   HKEY CreateInstanceKey();
   void DeleteInstanceKey();
   DWORD RegQueryDWORD(HKEY instkey, const char *name);
   char *RegQueryString(HKEY instance, const char *name);
   void RegSetDWORD(HKEY instance, char *name, DWORD value);
   void RegSetString(HKEY instance, char *name, char *value);

   HKEY FindOrCreateInstanceKey() {
      std::string instname;
      HKEY instkey = FindInstanceKey(instname);

      if (!instkey)
         instkey = CreateInstanceKey();

      return instkey;
   }

   // Set current notify state
   void SetNotify(bool notify) {
      m_notify = notify;
      Write();
   }

   // Write settings back to registry
   void Write() {
      HKEY instkey = FindOrCreateInstanceKey();
      if (instkey) {
         RegSetString(instkey, "host", m_host);
         RegSetDWORD(instkey, "port", m_port);
         RegSetDWORD(instkey, "refresh", m_refresh);
         RegSetDWORD(instkey, "notify", m_notify);
         RegCloseKey(instkey);
      }
   }

   // Instance data, cached from registry
   char *m_host;
   unsigned short m_port;
   int m_refresh;
   bool m_notify;

   // Icon/Menu for this instance
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

DWORD TrayInstance::RegQueryDWORD(HKEY instance, const char *name)
{
   DWORD result;
   DWORD len = sizeof(result);

   // Retrieve DWORD
   if (RegQueryValueEx(instance, name, NULL, NULL, (BYTE*)&result, &len)
         != ERROR_SUCCESS) {
      result = (DWORD)-1;
   }

   return result;
}

char *TrayInstance::RegQueryString(HKEY instance, const char *name)
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

void TrayInstance::RegSetDWORD(HKEY instance, char *name, DWORD value)
{
   RegSetValueEx(instance, name, 0, REG_DWORD, (BYTE*)&value, sizeof(value));
}

void TrayInstance::RegSetString(HKEY instance, char *name, char *value)
{
   RegSetValueEx(instance, name, 0, REG_SZ, (BYTE*)value, strlen(value)+1);
}

HKEY TrayInstance::FindInstanceKey(std::string &instname)
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
         char *testhost = RegQueryString(instance, "host");
         unsigned short testport = RegQueryDWORD(instance, "port");
         if (testhost && strcmp(testhost, m_host) == 0 && testport == m_port) {
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

HKEY TrayInstance::CreateInstanceKey()
{
   // Open registry key apctray
   HKEY apctray;
   if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, "Software\\Apcupsd\\Apctray",
                      0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ|KEY_WRITE,
                      NULL, &apctray, NULL) != ERROR_SUCCESS) {
      // Could not open Apctray key
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

void TrayInstance::DeleteInstanceKey()
{
   // Find the name of our instance key
   std::string instname;
   HKEY instkey = FindInstanceKey(instname);

   if (instkey) {
      RegCloseKey(instkey);

      // Open registry key apctray
      HKEY apctray;
      if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Software\\Apcupsd\\Apctray",
                       0, KEY_READ|KEY_WRITE, &apctray) == ERROR_SUCCESS) {
         // Delete instance key
         RegDeleteKey(apctray, instname.c_str());
         RegCloseKey(apctray);
      }
   }
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

int AddInstance(char *host, unsigned short port, int refresh, int notify)
{
   TrayInstance inst(host, port, refresh, notify);
   inst.Write();

   NotifyUser("The instance (%s:%d) was successfully created.",
      inst.m_host, inst.m_port);

   return 0;
}

int RegCountInstances(HKEY apctray)
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
   // If no host or port was specified, remove all instances
   if (!host && !port) {
      // Open registry key apctray
      HKEY apctray;
      if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Software\\Apcupsd\\Apctray",
                       0, KEY_READ|KEY_WRITE, &apctray) != ERROR_SUCCESS) {
         // No instances in registry
         return 0;
      }

      int i = 0;
      char name[1024];
      DWORD len = sizeof(name);

      // Iterate through all instances and delete them
      while (RegEnumKeyEx(apctray, i++, name, &len, NULL, NULL,
                          NULL, NULL) == ERROR_SUCCESS) {
         if (len && strncasecmp(name, "instance", 8) == 0 &&
             RegDeleteKey(apctray, name) == ERROR_SUCCESS) {
            i = 0;
         }

         len = sizeof(name);
      }

      RegCloseKey(apctray);
      NotifyUser("All instances were successfully deleted.");
   } else {
      // Deleting a single instance: We can use TrayInstance
      TrayInstance inst(host, port, 0, -1);
      inst.DeleteInstanceKey();

      NotifyUser("The specified instance (%s:%d) was successfully deleted.",
         host, port);
   }

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

void AllocateInstance(char *host, unsigned short port, int refresh, int notify)
{
   TrayInstance *inst = new TrayInstance(host, port, refresh, notify);
   if (inst) {
      instances.push_back(inst);
      inst->Create();
   }
}

void AllocateInstance(HKEY instkey)
{
   TrayInstance *inst = new TrayInstance(instkey);
   if (inst) {
      instances.push_back(inst);
      inst->Create();
   }
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

TrayInstance *FindInstance(upsMenu *menu)
{
   std::vector<TrayInstance*>::iterator iter;

   for (iter = instances.begin(); iter != instances.end(); iter++) {
      if ((*iter)->m_menu == menu) {
         return *iter;
      }
   }

   return NULL;
}

void CloseInstance(upsMenu *menu)
{
   std::vector<TrayInstance*>::iterator iter;

   for (iter = instances.begin(); iter != instances.end(); iter++) {
      if ((*iter)->m_menu == menu) {
         (*iter)->Destroy();
         delete *iter;
         instances.erase(iter);
         break;
      }
   }
}

void RemoveInstance(upsMenu *menu)
{
   TrayInstance *inst = FindInstance(menu);
   if (inst) {
      DelInstance(inst->m_host, inst->m_port);
      CloseInstance(menu);
   }
}

void SetNotify(upsMenu *menu, bool notify)
{
   TrayInstance *inst = FindInstance(menu);
   if (inst)
      inst->SetNotify(notify);
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
   int notify = -1;

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
      } else if (strcasecmp(arg, CMDOPT_NOTIFY) == 0) {
         notify = true;
      } else if (strcasecmp(arg, CMDOPT_INSTALL) == 0) {
         return Install();
      } else if (strcasecmp(arg, CMDOPT_REMOVE) == 0) {
         return Remove();
      } else if (strcasecmp(arg, CMDOPT_ADD) == 0) {
         return AddInstance(host, port, refresh, notify);
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

   if (!host && !port) {
      // No command line instance options were given: Launch
      // all instances specified in the registry
      LaunchInstances();

      // If no instances were created from the registry,
      // allocate a default one and write it to the registry.
      if (instances.empty()) {
         AllocateInstance(DEFAULT_HOST, DEFAULT_PORT, refresh, true);
         instances.back()->Write();
      }
   } else {
      // One or more command line options were given, so launch a single
      // instance using the specified parameters, filling in any missing
      // ones with defaults
      AllocateInstance(host, port, refresh, notify);
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

      case WM_BNOTIFY:
         // Record notify state in registry
         SetNotify((upsMenu*)msg.lParam, msg.wParam);
         break;

      default:
         DispatchMessage(&msg);
      }
   }

   // Instruct all instances to destroy
   std::vector<TrayInstance*>::iterator iter;
   for (iter = instances.begin();
        iter != instances.end();
        iter++)
   {
      (*iter)->Destroy();
   }

   // Free all instances. This waits for destruction to complete.
   while (!instances.empty()) {
      delete instances.back();
      instances.pop_back();
   }

   WSACleanup();
   return 0;
}
