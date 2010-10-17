/*
 * Mokosuite
 * Panel notifications window
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

#ifndef __PANEL_NOTIFICATIONS_WIN_H
#define __PANEL_NOTIFICATIONS_WIN_H

#include "panel.h"

void notification_window_add(MokoNotification* n);
void notification_window_remove(MokoNotification* n, gboolean update_only);

void notify_window_init(MokoPanel* panel);

void notify_window_start(void);
void notify_window_end(void);

void notify_window_show(void);
void notify_window_hide(void);

void notify_window_update_operator(const char* operator);

#endif  /* __PANEL_NOTIFICATIONS_WIN_H */
