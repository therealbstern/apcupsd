/*
 * InstanceManager.m
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
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1335, USA.
 */

#import "InstanceConfig.h"
#import "AppController.h"
#include <CoreServices/CoreServices.h>

@implementation InstanceManager

//******************************************************************************
// PRIVATE helper methods
//******************************************************************************

-(NSURL*)appURL
{
   // Get application URL (10.4 lacks NSBundle::bundleURL so get the path
   // and then convert to a file URL)
   return [NSURL fileURLWithPath:[[NSBundle mainBundle] bundlePath]];
}

// Fetches a reference to the apcagent login item or returns NULL if no login
// item for apcagent is found
-(LSSharedFileListItemRef)getLoginItem
{
   LSSharedFileListItemRef ret = NULL;

   // Fetch current user login items
   LSSharedFileListRef fileList = 
      LSSharedFileListCreate(NULL, kLSSharedFileListSessionLoginItems, NULL);
   if (!fileList)
      return NULL;
   UInt32 seed;
   NSArray *loginItems = 
      (NSArray *)LSSharedFileListCopySnapshot(fileList, &seed);

   // Search list for our URL
   if (loginItems)
   {
      NSURL *appUrl = [self appURL];
      for (unsigned i = 0; !ret && i < [loginItems count]; i++)
      {
         LSSharedFileListItemRef item = 
            (LSSharedFileListItemRef)[loginItems objectAtIndex:i];
         NSURL *url = nil;

         // LSSharedFileListItemResolve is deprecated in Mac OS X 10.10
         // Switch to LSSharedFileListItemCopyResolvedURL if possible
#if MAC_OS_X_VERSION_MIN_REQUIRED < 10100
         LSSharedFileListItemResolve(item, 0, (CFURLRef*)&url, NULL);
#else
         url = (NSURL*)LSSharedFileListItemCopyResolvedURL(item, 0, NULL);
#endif 
         if (url)
         {
            if ([url isEqual:appUrl])
               ret = (LSSharedFileListItemRef)CFRetain(item);
            [url release];
         }
      }

      [loginItems release];
   }

   CFRelease(fileList);
   return ret;
}

// Checks to see if apcagent login item is installed
-(BOOL)isStartAtLogin
{
   LSSharedFileListItemRef item = [self getLoginItem];
   BOOL ret = item != NULL;
   if (item)
      CFRelease(item);
   return ret;
}

// Add login item for apcagent (if not already present)
- (void)addLoginItem
{
   if (![self isStartAtLogin])
   {
      LSSharedFileListRef fileList = 
         LSSharedFileListCreate(NULL, kLSSharedFileListSessionLoginItems, NULL);
      if (fileList)
      {
         LSSharedFileListItemRef item = LSSharedFileListInsertItemURL(
            fileList, kLSSharedFileListItemLast, NULL, NULL, (CFURLRef)[self appURL], NULL, NULL);
         if (item)
            CFRelease(item);
         CFRelease(fileList);
      }
   }
}

// Remove login item for apcagent (if present)
- (void)removeLoginItem
{
   LSSharedFileListRef fileList = 
      LSSharedFileListCreate(NULL, kLSSharedFileListSessionLoginItems, NULL);
   if (fileList)
   {
      LSSharedFileListItemRef item = [self getLoginItem];
      if (item)
      {
         LSSharedFileListItemRemove(fileList, item);
         CFRelease(item);
      }
      CFRelease(fileList);
   }
}

// Toggle start at login in response to menu click
-(IBAction)startAtLogin:(id)sender
{
   if ([self isStartAtLogin])
   {
      [self removeLoginItem];
      [sender setState:NSOffState];
   }
   else
   {
      [self addLoginItem];
      [sender setState:NSOnState];
   }
}

- (void) instantiateMonitor:(InstanceConfig*)config
{
   // Instantiate the NIB for this monitor
   NSArray *objs;
   // instantiateNibWithOwner is deprecated in 10.8 where instantiateWithOwner
   // is preferred due to better memory management characteristics
#if MAC_OS_X_VERSION_MIN_REQUIRED < 1080
   [nib instantiateNibWithOwner:self topLevelObjects:&objs];
#else
   [nib instantiateWithOwner:self topLevelObjects:&objs];
#endif
   [instmap setObject:objs forKey:[config id]];

   // Locate the AppController object and activate it
   for (unsigned int j = 0; j < [objs count]; j++)
   {
      if ([[objs objectAtIndex:j] isMemberOfClass:[AppController class]])
      {
         AppController *ctrl = [objs objectAtIndex:j];
         [ctrl activateWithConfig:config manager:self];
         break;
      }
   }
}

//******************************************************************************
// PUBLIC methods
//******************************************************************************

- (InstanceManager *)init
{
   self = [super init];
   if (!self) return nil;

   instmap = [[NSMutableDictionary alloc] init];
   nib = [[NSNib alloc] initWithNibNamed:@"MainMenu" bundle:nil];

   return self;
}

- (void)dealloc
{
   [nib release];
   [instmap release];
   [super dealloc];
}

- (void) createMonitors
{
   NSUserDefaults *prefs = [NSUserDefaults standardUserDefaults];
   InstanceConfig *config;

   // Fetch instances array from preferences
   NSArray *instances = [prefs arrayForKey:INSTANCES_PREF_KEY];

   // If instance array does not exist or is empty, add a default entry
   if (!instances || [instances count] == 0)
   {
      config = [InstanceConfig configWithDefaults];
      [config save];
      instances = [prefs arrayForKey:INSTANCES_PREF_KEY];

      // Add login item if not already there
      [self addLoginItem];
   }

   // Instantiate monitors
   for (unsigned int i = 0; i < [instances count]; i++)
   {
      config = [InstanceConfig configWithDictionary:[instances objectAtIndex:i]];
      [self instantiateMonitor:config];
   }
}

-(IBAction)add:(id)sender
{
   // Create a new default monitor and save it to prefs
   InstanceConfig *config = [InstanceConfig configWithDefaults];
   [config save];

   // Instantiate the new monitor
   [self instantiateMonitor:config];
}

-(IBAction)remove:(id)sender
{
   NSLog(@"%s:%d %@", __FUNCTION__, __LINE__, [[sender menu] delegate]);

   // Find AppController instance which this menu refers to
   AppController *ac = (AppController *)[[sender menu] delegate];

   // Remove the config from prefs for this monitor
   [InstanceConfig removeConfigWithId:[ac id]];

   // Instruct the instance to close
   [ac close];

   // Remove our reference to the instance and all of its NIB objects
   [instmap removeObjectForKey:[ac id]];

#if MAC_OS_X_VERSION_MIN_REQUIRED < 1080
   // When using instantiateNibWithOwner above in instantiateMonitor() we need
   // to manually remove a ref on ac because the NIB top-level object array is
   // retained "automatically".
   [ac release];
#endif

   // If all instances have been removed, terminate the app
   if ([instmap count] == 0)
      [self removeAll:sender];
}

-(IBAction)removeAll:(id)sender
{
   // Remove all instances from preferences
   NSUserDefaults *prefs = [NSUserDefaults standardUserDefaults];
   [prefs removeObjectForKey:INSTANCES_PREF_KEY];

   // Remove user login item
   [self removeLoginItem];

   // Terminate the app
   [[NSApplication sharedApplication] terminate:self];
}

@end
