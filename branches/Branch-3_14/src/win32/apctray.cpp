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

char *ups_status(int stat);
int battstat = -1;
static char buf[MAXSTRING];

// WinMain parses the command line and either calls the main App
// routine or, under NT, the main service routine.
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   PSTR CmdLine, int iCmdShow)
{
   InitWinAPIWrapper();
   WSA_Init();

   // Create tray icon & menu if we're running as an app
   upsMenu *menu = new upsMenu(hInstance);
   if (menu == NULL) {
      PostQuitMessage(0);
   }

   // Now enter the Windows message handling loop until told to quit!
   MSG msg;
   while (GetMessage(&msg, NULL, 0, 0)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
   }

   if (menu != NULL) {
      delete menu;
   }

   WSACleanup();
   return 0;
}

/*
 * What follows are routines that are called by the Windows
 * C++ routines to get status information.
 * They are used for the info bubble on the system tray.
 */

#define MAX_PAIRS 256
#define MAX_DATA  100

struct pair {
   const char* key;
   const char* value;
   char data[MAX_DATA];
};

static const char *lookup_key(const char *key, struct pair table[])
{
   int idx;
   const char *ret = NULL;

   for (idx=0; idx < MAX_PAIRS && table[idx].key; idx++) {
      if (strcmp(key, table[idx].key) == 0) {
         ret = table[idx].value;
         break;
      }
   }

   return ret;
}

char *ltrim(char *str)
{
   while(isspace(*str))
      *str++ = '\0';

   return str;
}

void rtrim(char *str)
{
   char *tmp = str + strlen(str) - 1;
   
   while (tmp >= str && isspace(*tmp))
      *tmp-- = '\0';
}

char *trim(char *str)
{
   str = ltrim(str);
   rtrim(str);
   return str;
}

#define HOSTNAME "pia"

static struct pair *fetch_status()
{
   static struct pair pairs[MAX_PAIRS];
   int s, i, len;

   memset(pairs, 0, sizeof(pairs));

   s = net_open(HOSTNAME, NULL, 3551);
   if (s == -1)
      return NULL;

   if (net_send(s, "status", 6) != 6)
   {
      net_close(s);
      return NULL;
   }

   i = 0;
   while (i < MAX_PAIRS &&
          (len = net_recv(s, pairs[i].data, sizeof(pairs[i].data)-1)) > 0)
   {
      char *key, *value, *tmp;

      // NUL-terminate the string
      pairs[i].data[len] = '\0';

      // Find separator
      value = strchr(pairs[i].data, ':');
      
      // Trim whitespace from value
      if (value)
      {
         *value++ = '\0';
         value = trim(value);
      }
 
      // Trim whitespace from key;
      key = trim(pairs[i].data);

      pairs[i].key = key;
      pairs[i].value = value;
      i++;
   }

   net_close(s);
   return pairs;
}

/*
 * Return a UPS status string. For this cut, we return simply
 * the Online/OnBattery status. The stat flag is intended to
 * be used to retrieve additional status strings.
 */
char *ups_status(int stat)
{
   buf[0] = 0;

   // Fetch data from the UPS
   struct pair *pairs = fetch_status();
   if (pairs == NULL) {
      battstat = -1;
      return "COMMLOST";
   }

   // Lookup the STATFLAG key
   const char *statflag = lookup_key("STATFLAG", pairs);
   if (!statflag || *statflag == '\0') {
      battstat = -1;
      return "COMMLOST";
   }
   unsigned long status = strtoul(statflag, NULL, 0);

   // Lookup BCHARGE key
   const char *bcharge = lookup_key("BCHARGE", pairs);

   // Determine battery charge percent
   if (status & UPS_onbatt)
      battstat = 0;
   else if (bcharge && *bcharge != '\0')
      battstat = (int)atof(bcharge);
   else
      battstat = 100;

   // Now output status in human readable form
   if (status & UPS_calibration)
      astrncat(buf, "CAL ", sizeof(buf));
   if (status & UPS_trim)
      astrncat(buf, "TRIM ", sizeof(buf));
   if (status & UPS_boost)
      astrncat(buf, "BOOST ", sizeof(buf));
   if (status & UPS_online)
      astrncat(buf, "ONLINE ", sizeof(buf));
   if (status & UPS_onbatt)
      astrncat(buf, "ON BATTERY ", sizeof(buf));
   if (status & UPS_overload)
      astrncat(buf, "OVERLOAD ", sizeof(buf));
   if (status & UPS_battlow)
      astrncat(buf, "LOWBATT ", sizeof(buf));
   if (status & UPS_replacebatt)
      astrncat(buf, "REPLACEBATT ", sizeof(buf));
   if (!status & UPS_battpresent)
      astrncat(buf, "NOBATT ", sizeof(buf));

   // This overrides the above
   if (status & UPS_commlost) {
      astrncpy(buf, "COMMLOST", sizeof(buf));
      battstat = -1;
   }

   // This overrides the above
   if (status & UPS_shutdown)
      astrncpy(buf, "SHUTTING DOWN", sizeof(buf));

   return buf;
}

void FillStatusBox(HWND hwnd, int id_list)
{
   int s, len;

   snprintf(buf, sizeof(buf), "Status not available.");
      
   s = net_open(HOSTNAME, NULL, 3551);
   if (s == -1) {
      SendDlgItemMessage(hwnd, id_list, LB_ADDSTRING, 0, (LONG) buf);
      return;
   }

   if (net_send(s, "status", 6) != 6)
   {
      net_close(s);
      SendDlgItemMessage(hwnd, id_list, LB_ADDSTRING, 0, (LONG) buf);
      return;
   }

   while ((len = net_recv(s, buf, sizeof(buf)-1)) > 0)
   {
      buf[len] = '\0';
      rtrim(buf);
      SendDlgItemMessage(hwnd, id_list, LB_ADDSTRING, 0, (LONG) buf);
   }

   net_close(s);
}

/* Fill the Events list box with the last events */
void FillEventsBox(HWND hwnd, int id_list)
{
   int s, len;

   snprintf(buf, sizeof(buf), "Events not available.");

   s = net_open(HOSTNAME, NULL, 3551);
   if (s == -1) {
      SendDlgItemMessage(hwnd, id_list, LB_ADDSTRING, 0, (LONG) buf);
      return;
   }

   if (net_send(s, "events", 6) != 6)
   {
      net_close(s);
      SendDlgItemMessage(hwnd, id_list, LB_ADDSTRING, 0, (LONG) buf);
      return;
   }

   while ((len = net_recv(s, buf, sizeof(buf)-1)) > 0)
   {
      buf[len] = '\0';
      rtrim(buf);
      SendDlgItemMessage(hwnd, id_list, LB_ADDSTRING, 0, (LONG) buf);
   }

   net_close(s);
}
