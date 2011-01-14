/*
 * Mokosuite
 * Digital clock panel applet
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

#include <Elementary.h>
#include <glib.h>
#include <mokosuite/utils/misc.h>

#include "panel.h"
#include "clock.h"
#include "idle.h"

static void label_clock_update(MokoPanel* p)
{
    guint64 now = get_current_time();
    struct tm* timestamp_tm = localtime((const time_t*)&now);
    char strf[100+1] = {0, };

    strftime(strf, 100, "%H:%M", timestamp_tm);

    char* timestr = g_strdup_printf("<b><font_size=10>%s</></b>", strf);

    elm_label_label_set(p->time, timestr);
    idlescreen_update_time(strf);

    g_free(timestr);
}

static gboolean update_clock(gpointer p)
{
    label_clock_update((MokoPanel*) p);

    return TRUE;
}

Evas_Object* clock_applet_new(MokoPanel* panel)
{
    // orario
    Evas_Object* time = elm_label_add(panel->win);
    label_clock_update(panel);
    evas_object_show(time);

    // timer refresh orario
    g_timeout_add_seconds(1, update_clock, panel);

    // add applet
    mokopanel_append_object(panel, time);

    return time;
}
