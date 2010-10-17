/*
 * Mokosuite
 * Battery status panel applet
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

#ifndef __BATTERY_H
#define __BATTERY_H

#include <Evas.h>

#include "panel.h"

// stato connessione alimentazione
extern gboolean battery_charging;

// flag occupazione display su connessione usb
extern gboolean battery_resource_display;

Evas_Object* battery_applet_new(MokoPanel* panel);

#endif  /* __BATTERY_H */
