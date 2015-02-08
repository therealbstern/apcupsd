/*
 * ActiveNumberFormatter.m
 *
 * Subclass of NSNumberFormatter to provide on-the-fly range checking
 */

/*
 * Copyright (C) 2014 Adam Kropelin
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

#import <Cocoa/Cocoa.h>

//
// NSNumberFormatter allows user to enter invalid characters into the cell and 
// only validates the content when they attempt to tab off or select another
// control. ActiveNumberFormatter provides on-the-fly validation so the content 
// is always known to be correct.
//
@interface ActiveNumberFormatter: NSNumberFormatter

- (BOOL)isPartialStringValid:(NSString *)partialString
            newEditingString:(NSString **)newString
            errorDescription:(NSString **)error;

@end

@implementation ActiveNumberFormatter

- (BOOL)isPartialStringValid:(NSString *)partialString
            newEditingString:(NSString **)newString
            errorDescription:(NSString **)error
{
   NSCharacterSet *nonDigits = 
      [[NSCharacterSet decimalDigitCharacterSet] invertedSet];

   return [partialString length] == 0 ||
      ([partialString rangeOfCharacterFromSet:nonDigits].location == NSNotFound &&
       ([self minimum] == nil || [partialString intValue] >= [[self minimum] intValue]) &&
       ([self maximum] == nil || [partialString intValue] <= [[self maximum] intValue]));
}

@end
