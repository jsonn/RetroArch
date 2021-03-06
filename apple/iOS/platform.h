/*  RetroArch - A frontend for libretro.
 *  Copyright (C) 2013-2014 - Jason Fetters
 *  Copyright (C) 2011-2015 - Daniel De Matteis
 * 
 *  RetroArch is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  RetroArch is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with RetroArch.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __RARCH_IOS_PLATFORM_H
#define __RARCH_IOS_PLATFORM_H

#include <CoreLocation/CoreLocation.h>
#import <AVFoundation/AVCaptureOutput.h>
#include "views.h"

typedef struct
{
   char orientations[32];
   unsigned orientation_flags;
   char bluetooth_mode[64];
} apple_frontend_settings_t;
extern apple_frontend_settings_t apple_frontend_settings;

@interface RAGameView : UIViewController<CLLocationManagerDelegate, AVCaptureAudioDataOutputSampleBufferDelegate>
+ (RAGameView*)get;
@end

@interface RetroArch_iOS : UINavigationController<UIApplicationDelegate, UINavigationControllerDelegate, RetroArch_Platform>

@property (nonatomic) UIWindow* window;
@property (nonatomic) NSString* documentsDirectory; // e.g. /var/mobile/Documents

+ (RetroArch_iOS*)get;

- (void)showGameView;
- (void)toggleUI;

- (void)loadingCore:(NSString*)core withFile:(const char*)file;
- (void)unloadingCore;

- (void)refreshSystemConfig;
@end

void get_ios_version(int *major, int *minor);

#endif
