/*
 * AppController.m
 *
 * Apcupsd monitoring applet for Mac OS X
 */

/*
 * Copyright (C) 2009 Adam Kropelin
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General
 * Public License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA.
 */

#import "AppController.h"
#import "statmgr.h"

//******************************************************************************
// CLASS AppController
//******************************************************************************
@implementation AppController

#define HOSTNAME_PREF_KEY @"host"
#define PORT_PREF_KEY     @"port"
#define REFRESH_PREF_KEY  @"refresh"

- (void) awakeFromNib{

   // Load images
   NSBundle *bundle = [NSBundle mainBundle];
   chargingImage = [[NSImage alloc] initWithContentsOfFile:
      [bundle pathForResource:@"charging" ofType:@"png"]];
   commlostImage = [[NSImage alloc] initWithContentsOfFile:
      [bundle pathForResource:@"commlost" ofType:@"png"]];
   onbattImage = [[NSImage alloc] initWithContentsOfFile:
      [bundle pathForResource:@"onbatt" ofType:@"png"]];
   onlineImage = [[NSImage alloc] initWithContentsOfFile:
      [bundle pathForResource:@"online" ofType:@"png"]];

   // Create our status item
   statusItem = [[[NSStatusBar systemStatusBar] statusItemWithLength:
                  NSVariableStatusItemLength] retain];
   [statusItem setHighlightMode:YES];
   [statusItem setImage:commlostImage];
   [statusItem setMenu:statusMenu];

   // Setup status table control
   statusDataSource = [[StatusTableDataSource alloc] init];
   [[[statusGrid tableColumns] objectAtIndex:0] setIdentifier:[NSNumber numberWithInt:0]];
   [[[statusGrid tableColumns] objectAtIndex:1] setIdentifier:[NSNumber numberWithInt:1]];
   [statusGrid setDataSource:statusDataSource];

   // Setup events table control
   eventsDataSource = [[EventsTableDataSource alloc] init];
   [eventsGrid setDataSource:eventsDataSource];

   // Establish default preferences
   NSUserDefaults *prefs = [NSUserDefaults standardUserDefaults];
   NSDictionary *defaults = [NSDictionary dictionaryWithObjectsAndKeys:
       @"devbox3", HOSTNAME_PREF_KEY, 
       [NSNumber numberWithInt:3551], PORT_PREF_KEY, 
       [NSNumber numberWithInt:1], REFRESH_PREF_KEY, 
       nil];
   [prefs registerDefaults:defaults];

   // Lookup preferences
   configMutex = [[NSLock alloc] init];
   host = [[prefs stringForKey:HOSTNAME_PREF_KEY] retain];
   port = [prefs integerForKey:PORT_PREF_KEY];
   refresh = [prefs integerForKey:REFRESH_PREF_KEY];

   // Save copies of preferences so worker can tell what changed
   prevHost = [host retain];
   prevPort = port;
   prevRefresh = refresh;
   statmgr = NULL;

   // Create timer object used by the thread
   timer = [[NSTimer timerWithTimeInterval:refresh target:self 
            selector:@selector(timerHandler:) userInfo:nil repeats:YES] retain];

   // Start worker thread to handle polling UPS for status
   [NSThread detachNewThreadSelector:@selector(thread:) toTarget:self withObject:nil];
}

- (void) thread:(id)arg
{
   NSAutoreleasePool *p = [[NSAutoreleasePool alloc] init];

   // Add periodic timer to the runloop
   [[NSRunLoop currentRunLoop] addTimer:timer forMode:NSDefaultRunLoopMode];

   // Fire timer once immediately to get the first data sample ASAP
   [timer fire];

   // Enter the run loop...forever
   [[NSRunLoop currentRunLoop] run];

   [p release];
}

- (void)timerHandler:(NSTimer*)theTimer
{
   // Reinitialize the StatMgr if host or port setting changes
   [configMutex lock];
   if (!statmgr || ![prevHost isEqualToString:host] || prevPort != port)
   {
      [statusItem setImage:commlostImage];
      delete statmgr;
      statmgr = new StatMgr([host UTF8String], port);
      [prevHost release];
      prevHost = [host retain];
      prevPort = port;
   }
   [configMutex unlock];

   // Grab updated status info from apcupsd
   int battstat;
   astring statstr, upsname;
   statmgr->GetSummary(battstat, statstr, upsname);

   // Update icon based on UPS state
   if (battstat == -1)
      [statusItem setImage:commlostImage];
   else if (battstat == 0)
      [statusItem setImage:onbattImage];
   else if (battstat >= 100)
      [statusItem setImage:onlineImage];
   else
      [statusItem setImage:chargingImage];   

   // Update tooltip with status and UPS name
   astring tooltip;
   if (upsname == "UPS_IDEN" || upsname.empty())
      tooltip = statstr;
   else
      tooltip.format("%s: %s", upsname.str(), statstr.str());
   [statusItem setToolTip:[NSString stringWithCString:tooltip]];

   // Update status window, but only if it's visible (optimization)
   if ([statusWindow isVisible])
   {
      // Update raw status table
      alist<astring> stats;
      statmgr->GetAll(stats);
      [statusDataSource populate:stats];
      [statusGrid reloadData];

      // Update status text
      [statusText setStringValue:[NSString stringWithCString:statstr]];

      // Update runtime
      NSString *tmp = [NSString stringWithCString:statmgr->Get("TIMELEFT")];
      tmp = [[tmp componentsSeparatedByString:@" "] objectAtIndex:0];
      [statusRuntime setStringValue:tmp];

      // Update battery
      tmp = [NSString stringWithCString:statmgr->Get("BCHARGE")];
      tmp = [[tmp componentsSeparatedByString:@" "] objectAtIndex:0];
      [statusBatteryText setStringValue:[tmp stringByAppendingString:@"%"]];
      [statusBatteryBar setIntValue:[tmp intValue]];

      // Update load
      tmp = [NSString stringWithCString:statmgr->Get("LOADPCT")];
      tmp = [[tmp componentsSeparatedByString:@" "] objectAtIndex:0];
      [statusLoadText setStringValue:[tmp stringByAppendingString:@"%"]];
      [statusLoadBar setIntValue:[tmp intValue]];
   }

   // Update events window, but only if it's visible (optimization)
   if ([eventsWindow isVisible])
   {
      // Fetch current events from the UPS
      alist<astring> eventStrings;
      statmgr->GetEvents(eventStrings);
      [eventsDataSource populate:eventStrings];
      [eventsGrid reloadData];
   }

   // If refresh interval changed, invalidate old timer and start a new one
   [configMutex lock];
   if (prevRefresh != refresh)
   {
      prevRefresh = refresh;
      [theTimer invalidate];
      [theTimer release];
      timer = [[NSTimer timerWithTimeInterval:refresh target:self 
               selector:@selector(timerHandler:) userInfo:nil repeats:YES] retain];
      [[NSRunLoop currentRunLoop] addTimer:timer forMode:NSDefaultRunLoopMode];
   }
   [configMutex unlock];
}

- (void) dealloc {
    [chargingImage release];
    [commlostImage release];
    [onlineImage release];
    [onbattImage release];
    [host release];
    [prevHost release];
    [timer invalidate];
    [timer release];
    [statusDataSource release];
    [configMutex release];
    [super dealloc];
}

-(IBAction)config:(id)sender;
{
   // Copy current settings into config window
   [configHost setStringValue:host];
   [configPort setIntValue:port];
   [configRefresh setIntValue:refresh];

   // Force app to foreground and move key focus to config window
   [[NSApplication sharedApplication] activateIgnoringOtherApps:YES];
   [configWindow makeKeyAndOrderFront:self];
}

-(IBAction)configComplete:(id)sender;
{
   bool allFieldsValid = true;

   // Validate host
   NSString *tmpstr = [configHost stringValue];
   tmpstr = [tmpstr stringByTrimmingCharactersInSet:
               [NSCharacterSet whitespaceCharacterSet]];
   if ([tmpstr length] == 0)
   {
      [configHost setBackgroundColor:[NSColor redColor]];
      allFieldsValid = false;
   }
   else
      [configHost setBackgroundColor:[NSColor controlBackgroundColor]];

   // Validate port
   NSCharacterSet *nonDigits = 
      [[NSCharacterSet decimalDigitCharacterSet] invertedSet];

   if ([[configPort stringValue] rangeOfCharacterFromSet:nonDigits].location != NSNotFound ||
       [configPort intValue] < 1 || [configPort intValue] > 65535)
   {
      [configPort setBackgroundColor:[NSColor redColor]];
      allFieldsValid = false;
   }
   else
      [configPort setBackgroundColor:[NSColor controlBackgroundColor]];

   // Validate timeout
   if ([[configRefresh stringValue] rangeOfCharacterFromSet:nonDigits].location != NSNotFound ||
       [configRefresh intValue] < 1)
   {
      [configRefresh setBackgroundColor:[NSColor redColor]];
      allFieldsValid = false;
   }
   else
      [configRefresh setBackgroundColor:[NSColor controlBackgroundColor]];

   // Apply changes
   if (allFieldsValid)
   {
      // Grab new settings from window controls
      [configMutex lock];
      [host release];
      host = [[configHost stringValue] retain];
      port = [configPort intValue];
      refresh = [configRefresh intValue];
      [configMutex unlock];

      // Save new preferences
      NSUserDefaults *prefs = [NSUserDefaults standardUserDefaults];
      [prefs setObject:host forKey:HOSTNAME_PREF_KEY];
      [prefs setInteger:port forKey:PORT_PREF_KEY];
      [prefs setInteger:refresh forKey:REFRESH_PREF_KEY];

      // Hide window
      [configWindow orderOut:self];

      // Kick runloop timer so new config will be acted on immediately
      [timer fire];
   }
}

-(IBAction)status:(id)sender
{
   // Force app to foreground and move key focus to status window
   [[NSApplication sharedApplication] activateIgnoringOtherApps:YES];
   [statusWindow makeKeyAndOrderFront:self];

   // Kick timer so window is updated immediately
   [timer fire];
}

-(IBAction)events:(id)sender
{
   // Force app to foreground and move key focus to events window
   [[NSApplication sharedApplication] activateIgnoringOtherApps:YES];
   [eventsWindow makeKeyAndOrderFront:self];

   // Kick timer so window is updated immediately
   [timer fire];
}

-(IBAction)about:(id)sender;
{
   // Normally we'd just wire the about button directly to NSApp 
   // orderFrontStandardAboutPanel. However, since we're a status item without
   // a main window we need to bring ourself to the foreground or else the
   // about box will be buried behind whatever window the user has active at
   // the time. So we'll force ourself active, then call out to NSApp.
   [[NSApplication sharedApplication] activateIgnoringOtherApps:YES];
   [[NSApplication sharedApplication] orderFrontStandardAboutPanel:self];
}

@end

//******************************************************************************
// CLASS StatusTableDataSource
//******************************************************************************
@implementation StatusTableDataSource

- (id) init
{
   if ((self = [super init]))
   {
      mutex = [[NSLock alloc] init];
      keys = [[NSMutableArray alloc] init];
      values = [[NSMutableArray alloc] init];
   }
   return self;
}

- (void) dealloc
{
   [keys release];
   [values release];
   [mutex release];
   [super dealloc];
}

- (void)populate:(alist<astring> &)stats
{
   [mutex lock];
   [keys removeAllObjects];
   [values removeAllObjects];

   alist<astring>::const_iterator iter;
   for (iter = stats.begin(); iter != stats.end(); ++iter)
   {
      NSString *data = [NSString stringWithCString:*iter];
      NSString *key = [data substringToIndex:9];
      NSString *value = [data substringFromIndex:10];
      [keys addObject:key];
      [values addObject:value];
   }
   [mutex unlock];
}

- (int)numberOfRowsInTableView:(NSTableView *)aTableView
{
   [mutex lock];
   int count = [keys count];
   [mutex unlock];
   return count;
}

- (id)tableView:(NSTableView *)aTableView 
   objectValueForTableColumn:(NSTableColumn *)aTableColumn 
   row:(int)rowIndex
{
   NSNumber *col = [aTableColumn identifier];
   NSString *ret;

   // Copy value under lock so we don't worry about life cycle
   [mutex lock];
   if ([col intValue] == 0)
      ret = [NSString stringWithString:[keys objectAtIndex:rowIndex]];
   else
      ret = [NSString stringWithString:[values objectAtIndex:rowIndex]];
   [mutex unlock];

   return ret;
}

@end

//******************************************************************************
// CLASS EventsTableDataSource
//******************************************************************************
@implementation EventsTableDataSource

- (id) init
{
   if ((self = [super init]))
   {
      mutex = [[NSLock alloc] init];
      strings = [[NSMutableArray alloc] init];
   }
   return self;
}

- (void) dealloc
{
   [strings release];
   [mutex release];
   [super dealloc];
}

- (void)populate:(alist<astring> &)stats
{
   [mutex lock];
   [strings removeAllObjects];

   alist<astring>::const_iterator iter;
   for (iter = stats.begin(); iter != stats.end(); ++iter)
   {
      NSString *data = [NSString stringWithCString:*iter];
      [strings addObject:data];
   }
   [mutex unlock];
}

- (int)numberOfRowsInTableView:(NSTableView *)aTableView
{
   [mutex lock];
   int count = [strings count];
   [mutex unlock];
   return count;
}

- (id)tableView:(NSTableView *)aTableView 
   objectValueForTableColumn:(NSTableColumn *)aTableColumn 
   row:(int)rowIndex
{
   NSString *ret;

   // Copy value under lock so we don't worry about life cycle
   [mutex lock];
   ret = [NSString stringWithString:[strings objectAtIndex:rowIndex]];
   [mutex unlock];

   return ret;
}

@end
