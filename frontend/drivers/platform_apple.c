/* RetroArch - A frontend for libretro.
 * Copyright (C) 2010-2014 - Hans-Kristian Arntzen
 * Copyright (C) 2011-2015 - Daniel De Matteis
 * Copyright (C) 2012-2014 - Jason Fetters
 * Copyright (C) 2014-2015 - Jay McCarthy
 *
 * RetroArch is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Found-
 * ation, either version 3 of the License, or (at your option) any later version.
 *
 * RetroArch is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE. See the GNU General Public License for more details.
 * * You should have received a copy of the GNU General Public License along with RetroArch.
 * If not, see <http://www.gnu.org/licenses/>.
 */

#include "../../apple/common/CFExtensions.h"

#include "../frontend_driver.h"
#include "../../ui/ui_companion_driver.h"

#include <stdint.h>
#include <boolean.h>
#include <stddef.h>
#include <string.h>

#include <sys/utsname.h>

#if defined(OSX)
#include <Carbon/Carbon.h>
#include <IOKit/ps/IOPowerSources.h>
#include <IOKit/ps/IOPSKeys.h>
#endif

#if defined(IOS)
void get_ios_version(int *major, int *minor);

enum frontend_powerstate ios_get_powerstate(int *seconds, int *percent);
#endif

#if defined(OSX)

#define PMGMT_STRMATCH(a,b) (CFStringCompare(a, b, 0) == kCFCompareEqualTo)
#define PMGMT_GETVAL(k,v)   CFDictionaryGetValueIfPresent(dict, CFSTR(k), (const void **) v)

/* Note that AC power sources also include a laptop battery it is charging. */
static void checkps(CFDictionaryRef dict, bool * have_ac, bool * have_battery,
        bool * charging, int *seconds, int *percent)
{
    CFStringRef strval; /* don't CFRelease() this. */
    CFBooleanRef bval;
    CFNumberRef numval;
    bool charge = false;
    bool choose = false;
    bool  is_ac = false;
    int    secs = -1;
    int  maxpct = -1;
    int     pct = -1;
    
    if ((PMGMT_GETVAL(kIOPSIsPresentKey, &bval)) && (bval == kCFBooleanFalse))
        return;                 /* nothing to see here. */
    
    if (!PMGMT_GETVAL(kIOPSPowerSourceStateKey, &strval))
        return;
    
    if (PMGMT_STRMATCH(strval, CFSTR(kIOPSACPowerValue)))
        is_ac = *have_ac = true;
    else if (!PMGMT_STRMATCH(strval, CFSTR(kIOPSBatteryPowerValue)))
        return;                 /* not a battery? */
    
    if ((PMGMT_GETVAL(kIOPSIsChargingKey, &bval)) && (bval == kCFBooleanTrue))
        charge = true;
    
    if (PMGMT_GETVAL(kIOPSMaxCapacityKey, &numval))
    {
        SInt32 val = -1;
        CFNumberGetValue(numval, kCFNumberSInt32Type, &val);
        if (val > 0)
        {
            *have_battery = true;
            maxpct        = (int) val;
        }
    }
    
    if (PMGMT_GETVAL(kIOPSMaxCapacityKey, &numval))
    {
        SInt32 val = -1;
        CFNumberGetValue(numval, kCFNumberSInt32Type, &val);
        if (val > 0)
        {
            *have_battery = true;
            maxpct        = (int) val;
        }
    }
    
    if (PMGMT_GETVAL(kIOPSTimeToEmptyKey, &numval))
    {
        SInt32 val = -1;
        CFNumberGetValue(numval, kCFNumberSInt32Type, &val);
        
        /* Mac OS X reports 0 minutes until empty if you're plugged in. :( */
        if ((val == 0) && (is_ac))
            val = -1;           /* !!! FIXME: calc from timeToFull and capacity? */
        
        secs = (int) val;
        if (secs > 0)
            secs *= 60;         /* value is in minutes, so convert to seconds. */
    }
    
    if (PMGMT_GETVAL(kIOPSCurrentCapacityKey, &numval))
    {
        SInt32 val = -1;
        CFNumberGetValue(numval, kCFNumberSInt32Type, &val);
        pct = (int) val;
    }
    
    if ((pct > 0) && (maxpct > 0))
        pct = (int) ((((double) pct) / ((double) maxpct)) * 100.0);
    
    if (pct > 100)
        pct = 100;
    
    /*
     * We pick the battery that claims to have the most minutes left.
     *  (failing a report of minutes, we'll take the highest percent.)
     */
    if ((secs < 0) && (*seconds < 0))
    {
        if ((pct < 0) && (*percent < 0))
            choose = true;  /* at least we know there's a battery. */
        if (pct > *percent)
            choose = true;
    }
    else if (secs > *seconds)
        choose = true;
    
    if (choose)
    {
        *seconds  = secs;
        *percent  = pct;
        *charging = charge;
    }
}
#endif

static void frontend_apple_get_name(char *name, size_t sizeof_name)
{
#if defined(IOS)
    struct utsname buffer;
    
    if (uname(&buffer) != 0)
        return;
    
    strlcpy(name, buffer.machine, sizeof_name);
#elif defined(OSX)
    size_t length = 0;
    sysctlbyname("hw.model", name, &length, NULL, 0);
#endif
}

static void frontend_apple_get_os(char *name, size_t sizeof_name, int *major, int *minor)
{
   (void)name;
   (void)sizeof_name;
   (void)major;
   (void)minor;
    
#if defined(IOS)
    get_ios_version(major, minor);
    strlcpy(name, "iOS", sizeof_name);
#elif defined(OSX)
    strlcpy(name, "OSX", sizeof_name);
#endif
}

static void frontend_apple_get_environment_settings(int *argc, char *argv[],
      void *args, void *params_data)
{
   char temp_dir[PATH_MAX_LENGTH];
   char bundle_path_buf[PATH_MAX_LENGTH], home_dir_buf[PATH_MAX_LENGTH];
   CFURLRef bundle_url;
   CFStringRef bundle_path;
   CFBundleRef bundle = CFBundleGetMainBundle();
    
   (void)temp_dir;

   if (!bundle)
      return;

   bundle_url  = CFBundleCopyBundleURL(bundle);
   bundle_path = CFURLCopyPath(bundle_url);
    
   CFStringGetCString(bundle_path, bundle_path_buf, sizeof(bundle_path_buf), kCFStringEncodingUTF8);
   (void)home_dir_buf;

   CFSearchPathForDirectoriesInDomains(CFDocumentDirectory, CFUserDomainMask, 1, home_dir_buf, sizeof(home_dir_buf));
    
#ifdef OSX
    strlcat(home_dir_buf, "/RetroArch", sizeof(home_dir_buf));
#endif
    
   fill_pathname_join(g_defaults.core_dir, home_dir_buf, "modules", sizeof(g_defaults.core_dir));
   fill_pathname_join(g_defaults.core_info_dir, home_dir_buf, "info", sizeof(g_defaults.core_info_dir));
   fill_pathname_join(g_defaults.overlay_dir, home_dir_buf, "overlays", sizeof(g_defaults.overlay_dir));
   fill_pathname_join(g_defaults.autoconfig_dir, home_dir_buf, "autoconfig/hid", sizeof(g_defaults.autoconfig_dir));
   fill_pathname_join(g_defaults.assets_dir, home_dir_buf, "assets", sizeof(g_defaults.assets_dir));
   fill_pathname_join(g_defaults.system_dir, home_dir_buf, ".RetroArch", sizeof(g_defaults.system_dir));
   strlcpy(g_defaults.menu_config_dir, g_defaults.system_dir, sizeof(g_defaults.menu_config_dir));
   fill_pathname_join(g_defaults.config_path, g_defaults.menu_config_dir, "retroarch.cfg", sizeof(g_defaults.config_path));
   fill_pathname_join(g_defaults.database_dir, home_dir_buf, "rdb", sizeof(g_defaults.database_dir));
   fill_pathname_join(g_defaults.cursor_dir, home_dir_buf, "cursors", sizeof(g_defaults.cursor_dir));
   fill_pathname_join(g_defaults.cheats_dir, home_dir_buf, "cht", sizeof(g_defaults.cheats_dir));
   strlcpy(g_defaults.sram_dir, g_defaults.system_dir, sizeof(g_defaults.sram_dir));
   strlcpy(g_defaults.savestate_dir, g_defaults.system_dir, sizeof(g_defaults.savestate_dir));
    
   CFTemporaryDirectory(temp_dir, sizeof(temp_dir));
   strlcpy(g_defaults.extraction_dir, temp_dir, sizeof(g_defaults.extraction_dir));
    
   fill_pathname_join(g_defaults.shader_dir, home_dir_buf, "shaders_glsl", sizeof(g_defaults.shader_dir));
    
#if defined(OSX)
#ifdef HAVE_CG
   fill_pathname_join(g_defaults.shader_dir, home_dir_buf, "shaders_cg", sizeof(g_defaults.shader_dir));
#endif
    fill_pathname_join(g_defaults.audio_filter_dir, home_dir_buf, "audio_filters", sizeof(g_defaults.audio_filter_dir));
    fill_pathname_join(g_defaults.video_filter_dir, home_dir_buf, "video_filters", sizeof(g_defaults.video_filter_dir));
#endif
    
   path_mkdir(bundle_path_buf);
    
   if (access(bundle_path_buf, 0755) != 0)
      RARCH_ERR("Failed to create or access base directory: %s\n", bundle_path_buf);
    else
    {
        path_mkdir(g_defaults.system_dir);
        
        if (access(g_defaults.system_dir, 0755) != 0)
            RARCH_ERR("Failed to create or access system directory: %s.\n", g_defaults.system_dir);
    }

   CFRelease(bundle_path);
   CFRelease(bundle_url);
}

extern void apple_rarch_exited(void);

static void frontend_apple_load_content(void)
{
   driver_t          *driver = driver_get_ptr();
   const ui_companion_driver_t *ui = ui_companion_get_ptr();
    
   if (ui && ui->notify_content_loaded)
      ui->notify_content_loaded(driver->ui_companion_data);
}

static void frontend_apple_shutdown(bool unused)
{
    apple_rarch_exited();
}

static int frontend_apple_get_rating(void)
{
   char model[PATH_MAX_LENGTH];

   frontend_apple_get_name(model, sizeof(model));

   /* iPhone 4 */
#if 0
   if (strstr(model, "iPhone3"))
      return -1;
#endif

   /* iPad 1 */
#if 0
   if (strstr(model, "iPad1,1"))
      return -1;
#endif

   /* iPhone 4S */
   if (strstr(model, "iPhone4,1"))
      return 8;

   /* iPad 2/iPad Mini 1 */
   if (strstr(model, "iPad2"))
      return 9;

   /* iPhone 5/5C */
   if (strstr(model, "iPhone5"))
      return 13;

   /* iPhone 5S */
   if (strstr(model, "iPhone6,1") || strstr(model, "iPhone6,2"))
      return 14;

   /* iPad Mini 2/3 */
   if (     strstr(model, "iPad4,4")
         || strstr(model, "iPad4,5")
         || strstr(model, "iPad4,6")
         || strstr(model, "iPad4,7")
         || strstr(model, "iPad4,8")
         || strstr(model, "iPad4,9")
         )
      return 15;

   /* iPad Air */
   if (     strstr(model, "iPad4,1")
         || strstr(model, "iPad4,2")
         || strstr(model, "iPad4,3")
      )
      return 16;

   /* iPhone 6, iPhone 6 Plus */
   if (strstr(model, "iPhone7"))
      return 17;

   /* iPad Air 2 */
   if (strstr(model, "iPad5,3") || strstr(model, "iPad5,4"))
      return 18;

   /* TODO/FIXME - 
      - more ratings for more systems
      - determine rating more intelligently*/
   return -1;
}

static enum frontend_powerstate frontend_apple_get_powerstate(int *seconds, int *percent)
{
   enum frontend_powerstate ret = FRONTEND_POWERSTATE_NONE;
#if defined(OSX)
   CFIndex i, total;
   CFArrayRef list;
   bool have_ac, have_battery, charging;
   CFTypeRef blob  = IOPSCopyPowerSourcesInfo();

   *seconds        = -1;
   *percent        = -1;

   if (!blob)
      goto end;

   list = IOPSCopyPowerSourcesList(blob);

   if (!list)
      goto end;

   /* don't CFRelease() the list items, or dictionaries! */
   have_ac         = false;
   have_battery    = false;
   charging        = false;
   total           = CFArrayGetCount(list);

   for (i = 0; i < total; i++)
   {
      CFTypeRef ps = (CFTypeRef)CFArrayGetValueAtIndex(list, i);
      CFDictionaryRef dict = IOPSGetPowerSourceDescription(blob, ps);
      if (dict)
         checkps(dict, &have_ac, &have_battery, &charging,
               seconds, percent);
   }

   if (!have_battery)
      ret = FRONTEND_POWERSTATE_NO_SOURCE;
   else if (charging)
      ret = FRONTEND_POWERSTATE_CHARGING;
   else if (have_ac)
      ret = FRONTEND_POWERSTATE_CHARGED;
   else
      ret = FRONTEND_POWERSTATE_ON_POWER_SOURCE;

   CFRelease(list);
end:
   if (blob)
      CFRelease(blob);
#elif defined(IOS)
   ret = ios_get_powerstate(seconds, percent);
#endif
   return ret;
}

enum frontend_architecture frontend_apple_get_architecture(void)
{
   struct utsname buffer;
    
   if (uname(&buffer) != 0)
      return FRONTEND_ARCH_NONE;
    
#ifdef OSX
   if (!strcmp(buffer.machine, "x86_64"))
       return FRONTEND_ARCH_X86_64;
    if (!strcmp(buffer.machine, "x86"))
        return FRONTEND_ARCH_X86;
    if (!strcmp(buffer.machine, "Power Macintosh"))
        return FRONTEND_ARCH_PPC;
#endif
    
   return FRONTEND_ARCH_NONE;
}

const frontend_ctx_driver_t frontend_ctx_apple = {
   frontend_apple_get_environment_settings,
   NULL,                         /* init */
   NULL,                         /* deinit */
   NULL,                         /* exitspawn */
   NULL,                         /* process_args */
   NULL,                         /* exec */
   NULL,                         /* set_fork */
   frontend_apple_shutdown,
   frontend_apple_get_name,
   frontend_apple_get_os,
   frontend_apple_get_rating,
   frontend_apple_load_content,
   frontend_apple_get_architecture,
   frontend_apple_get_powerstate,
   "apple",
};
