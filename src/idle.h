/*
 * Mokosuite
 * Idle screen
 * Copyright (C) 2009-2010 Daniele Ricci <daniele.athome@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef __IDLE_H
#define __IDLE_H

#include "panel.h"

#define CONFIG_SECTION_IDLE        "idle"

#define CONFIG_DISPLAY_DIM_USB     "display_dim_usb"
#define CONFIG_DISPLAY_BRIGHTNESS  "display_brightness"
#define CONFIG_SCREENSAVER_IMAGE   "screensaver_image"

#define CONFIG_DEFAULT_DISPLAY_DIM_USB      TRUE
#define CONFIG_DEFAULT_DISPLAY_BRIGHTNESS   90

void idlescreen_update_operator(const char* name);
void idlescreen_update_time(const char* time);

void screensaver_on(void);
void screensaver_off(void);

void idle_hide(void);
void idle_raise(gboolean ts_grab);

void idlescreen_init(MokoPanel* panel);

#endif  /* __IDLE_H */
