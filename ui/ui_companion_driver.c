/*  RetroArch - A frontend for libretro.
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

#include "ui_companion_driver.h"
#include "../driver.h"
#include <string.h>

#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif

static const ui_companion_driver_t *ui_companion_drivers[] = {
#ifdef IOS
   &ui_companion_cocoatouch,
#endif
#ifdef HAVE_QT
   &ui_companion_qt,
#endif
   &ui_companion_null,
   NULL
};

/**
 * ui_companion_find_driver:
 * @ident               : Identifier name of driver to find.
 *
 * Finds driver with @ident. Does not initialize.
 *
 * Returns: pointer to driver if successful, otherwise NULL.
 **/
const ui_companion_driver_t *ui_companion_find_driver(const char *ident)
{
   unsigned i;

   for (i = 0; ui_companion_drivers[i]; i++)
   {
      if (strcmp(ui_companion_drivers[i]->ident, ident) == 0)
         return ui_companion_drivers[i];
   }

   return NULL;
}

/**
 * ui_companion_init_first:
 *
 * Finds first suitable driver and initialize.
 *
 * Returns: pointer to first suitable driver, otherwise NULL. 
 **/
const ui_companion_driver_t *ui_companion_init_first(void)
{
   unsigned i;

   for (i = 0; ui_companion_drivers[i]; i++)
      return ui_companion_drivers[i];

   return NULL;
}

const ui_companion_driver_t *ui_companion_get_ptr(void)
{
   driver_t *driver        = driver_get_ptr();
   if (!driver)
      return NULL;
   return driver->ui_companion;
}
