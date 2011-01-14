/*
 * Mokosuite
 * GPS status panel applet
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
#include <mokosuite/utils/utils.h>
#include <freesmartphone-glib/ousaged/usage.h>
#include <freesmartphone-glib/gypsy/device.h>
#include <glib.h>

#include "panel.h"
#include "gps.h"

typedef struct {
    Evas_Object* object;
    MokoPanel* panel;
} applet_container;

// lista applet gps
static Eina_List* applets = NULL;

static bool gps_enabled = FALSE;
static bool gps_fixed = FALSE;

static Ecore_Timer* anim = NULL;

static Evas_Object* create_icon(MokoPanel* panel, bool initial_status)
{
    Evas_Object* gps = elm_icon_add(panel->win);

    elm_icon_file_set(gps, initial_status ? MOKOPANEL_DATADIR "/gps-on.png" : MOKOPANEL_DATADIR "/gps-off.png", NULL);
    elm_icon_no_scale_set(gps, TRUE);
    #ifdef QVGA
    elm_icon_scale_set(gps, FALSE, TRUE);
    #else
    elm_icon_scale_set(gps, TRUE, TRUE);
    #endif

    evas_object_size_hint_min_set(gps, ICON_SIZE, ICON_SIZE);
    evas_object_size_hint_align_set(gps, 0.5, 0.5);
    evas_object_show(gps);

    return gps;
}

static void set_icon(Evas_Object* gps, bool on)
{
    elm_icon_file_set(gps, on ? MOKOPANEL_DATADIR "/gps-on.png" : MOKOPANEL_DATADIR "/gps-off.png", NULL);
    elm_icon_no_scale_set(gps, TRUE);
    #ifdef QVGA
    elm_icon_scale_set(gps, FALSE, TRUE);
    #else
    elm_icon_scale_set(gps, TRUE, TRUE);
    #endif

    evas_object_size_hint_min_set(gps, ICON_SIZE, ICON_SIZE);
    evas_object_size_hint_align_set(gps, 0.5, 0.5);
}

static Eina_Bool animate(void* data)
{
    // gps fixed, do nothing
    if (gps_fixed) return FALSE;

    static bool on_frame = TRUE;
    Eina_List* iter;
    applet_container* cont;
    EINA_LIST_FOREACH(applets, iter, cont)
        set_icon(cont->object, on_frame);

    on_frame = !on_frame;
    return TRUE;
}

static void fix_animation_start(void)
{
    if (!anim)
        anim = ecore_timer_add(0.5, animate, NULL);
}

static void fix_animation_stop(void)
{
    if (anim) {
        ecore_timer_del(anim);
        anim = NULL;
    }
}

static void update_icon(applet_container* gps)
{
    if (gps_enabled) {
        EINA_LOG_DBG("GPS enabled");
        if (!gps->object)
            gps->object = create_icon(gps->panel, FALSE);
        mokopanel_append_object(gps->panel, gps->object);

        if (!gps_fixed) {
            EINA_LOG_DBG("GPS not fixed, starting animation");
            set_icon(gps->object, FALSE);

            // start animation
            fix_animation_start();
        }
        else {
            EINA_LOG_DBG("GPS fixed, stopping animation");
            // stop animation
            fix_animation_stop();

            // gps fixed!!
            set_icon(gps->object, TRUE);
        }
    }

    else {
        EINA_LOG_DBG("GPS disabled");
        fix_animation_stop();
        evas_object_del(gps->object);
        gps->object = NULL;
    }
}

/* FixStatusChange signal */
static void fix_changed(gpointer userdata, gint status)
{
    EINA_LOG_DBG("GPS fix status changed to %d", status);
    gps_fixed = (status >= 3);
    update_icon((applet_container*) userdata);
}

/* ResourceChanged signal */
static void resource_changed(gpointer userdata, const char* name, gboolean status, GHashTable* properties)
{
    if (name && !strcmp(name, "GPS")) {
        gps_enabled = status;
        // fix status will be retrieved from signal -- for now force it FALSE
        gps_fixed = FALSE;
        update_icon((applet_container*) userdata);
    }
}

static void fix_status(GError* error, gint status, gpointer userdata)
{
    if (!error)
        fix_changed(userdata, status);
}

static void resource_state(GError* error, gboolean status, gpointer userdata)
{
    if (!error) {
        resource_changed(userdata, "GPS", status, NULL);

        // initial fix state
        if (gps_enabled)
            gypsy_device_get_fix_status(fix_status, userdata);
    }
}

static gboolean gps_fso_connect(gpointer userdata)
{
    // initial resource state
    ousaged_usage_get_resource_state("GPS", resource_state, userdata);

    return FALSE;
}

Evas_Object* gps_applet_new(MokoPanel* panel)
{
    applet_container* cont = calloc(1, sizeof(applet_container));
    cont->panel = panel;

    // connetti segnali FSO
    ousaged_usage_resource_changed_connect(resource_changed, cont);
    gypsy_device_fix_status_changed_connect(fix_changed, cont);

    // richiesta dati iniziale
    g_idle_add(gps_fso_connect, cont);

    applets = eina_list_append(applets, cont);
    return cont->object;
}
