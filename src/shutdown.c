/*
 * Mokosuite
 * Shutdown window
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
#include <mokosuite/utils/settings.h>
#include <mokosuite/utils/utils.h>
#include <freesmartphone-glib/ousaged/usage.h>

#include "globals.h"
#include "shutdown.h"

#include <glib/gi18n-lib.h>

#define WIN_WIDTH       400

#define MOKO_PHONE_NAME                 "org.mokosuite.phone"
#define MOKO_PHONE_INTERFACE            "org.mokosuite.Phone"
#define MOKO_PHONE_PATH                 MOKOSUITE_DBUS_PATH "/Phone"
#define MOKO_PHONE_SETTINGS_PATH        MOKO_PHONE_PATH "/Settings"

static Evas_Object* win = NULL;
static gboolean offline_mode = FALSE;
static DBusGProxy* phone_settings = NULL;

static void _offline_mode(void* data, Evas_Object* obj, void* event_info)
{
    g_debug("Offline mode");
    shutdown_window_hide();

    // imposta variabile offline_mode!!!
    settings_set(phone_settings, "offline_mode", settings_from_boolean(!offline_mode), NULL);
    // lascia il resto in balia dell'oscurita'... lol
}

static void _poweroff_cancel(void* data, Evas_Object* obj, void* event_info)
{
    evas_object_del((Evas_Object*)data);
}

static void _poweroff(void* data, Evas_Object* obj, void* event_info)
{
    _poweroff_cancel(data, obj, event_info);

    // poweroff :)
    ousaged_usage_shutdown(NULL, NULL);
}

static void poweroff_confirm_window(void)
{
    Evas_Object* pwin = elm_win_add(NULL, "mokopanel_shutdown", ELM_WIN_DIALOG_BASIC);

    elm_win_title_set(pwin, _("Power off"));
    //elm_win_borderless_set(pwin, TRUE);
    elm_win_autodel_set(pwin, TRUE);

    Evas_Object *bg = elm_bg_add(pwin);
    evas_object_size_hint_weight_set (bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    elm_win_resize_object_add (pwin, bg);
    evas_object_show(bg);

    // box principale
    Evas_Object* vbox = elm_box_add(pwin);
    elm_box_homogenous_set(vbox, TRUE);

    evas_object_size_hint_weight_set (vbox, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    elm_win_resize_object_add (pwin, vbox);
    evas_object_show(vbox);

    // messaggio
    Evas_Object* lb = elm_label_add(pwin);
    elm_label_label_set(lb, _("The phone will power off."));
    evas_object_size_hint_weight_set (lb, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set (lb, 0.5, 0.5);

    evas_object_show(lb);

    // padding frame
    Evas_Object *pad_frame = elm_frame_add(pwin);
    evas_object_size_hint_weight_set(pad_frame, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set(pad_frame, EVAS_HINT_FILL, EVAS_HINT_FILL);

    elm_object_style_set(pad_frame, "pad_large");
    elm_frame_content_set(pad_frame, lb);

    evas_object_show(pad_frame);
    elm_box_pack_start(vbox, pad_frame);

    // box pulsanti
    Evas_Object* hbox = elm_box_add(pwin);
    elm_box_homogenous_set(hbox, TRUE);
    elm_box_horizontal_set(hbox, TRUE);
    evas_object_size_hint_weight_set (hbox, EVAS_HINT_EXPAND, 0.0);
    evas_object_size_hint_align_set (hbox, EVAS_HINT_FILL, 0.0);
    evas_object_show(hbox);

    // pulsante OK
    Evas_Object* btn_ok = elm_button_add(pwin);
    elm_button_label_set(btn_ok, _("OK"));

    evas_object_size_hint_weight_set (btn_ok, EVAS_HINT_EXPAND, 0.0);
    evas_object_size_hint_align_set (btn_ok, EVAS_HINT_FILL, 0.0);
    evas_object_show(btn_ok);

    evas_object_smart_callback_add(btn_ok, "clicked", _poweroff, pwin);

    elm_box_pack_start(hbox, btn_ok);

    // pulsante Annulla
    Evas_Object* btn_cancel = elm_button_add(pwin);
    elm_button_label_set(btn_cancel, _("Cancel"));

    evas_object_size_hint_weight_set (btn_cancel, EVAS_HINT_EXPAND, 0.0);
    evas_object_size_hint_align_set (btn_cancel, EVAS_HINT_FILL, 0.0);
    evas_object_show(btn_cancel);

    evas_object_smart_callback_add(btn_cancel, "clicked", _poweroff_cancel, pwin);

    elm_box_pack_end(hbox, btn_cancel);


    elm_box_pack_end(vbox, hbox);

    evas_object_show(pwin);
    elm_win_activate(pwin);
}

static void _poweroff_confirm(void* data, Evas_Object* obj, void* event_info)
{
    g_debug("Poweroff confirmation");
    shutdown_window_hide();

    poweroff_confirm_window();
}

static void _shutdown_close(void* data, Evas_Object* obj, void* event_info)
{
    shutdown_window_hide();
}

static void update_buttons(void)
{
    // TODO aggiorna pulsante
    char* value = settings_get(phone_settings, "offline_mode", NULL, NULL);
    if (value == NULL) {
        offline_mode = FALSE;
    } else {
        offline_mode = settings_to_boolean(value);
        g_free(value);
    }

    evas_object_show(win);
    elm_win_activate(win);
}

void shutdown_window_hide(void)
{
    if (win != NULL) {
        evas_object_del(win);
        win = NULL;
    }
}

void shutdown_window_show(void)
{
    if (win == NULL)
        shutdown_window_init();

    update_buttons();
}

void shutdown_window_init(void)
{
    // phone settings
    phone_settings = settings_connect(MOKO_PHONE_NAME, MOKO_PHONE_SETTINGS_PATH);

    win = elm_win_add(NULL, "mokopanel_shutdown", ELM_WIN_DIALOG_BASIC);

    elm_win_title_set(win, _("Shutdown"));
    //elm_win_borderless_set(win, TRUE);
    evas_object_resize(win, WIN_WIDTH, 0);

    evas_object_smart_callback_add(win, "delete,request", _shutdown_close, NULL);

    Evas_Object *bg = elm_bg_add(win);
    evas_object_size_hint_weight_set (bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    elm_win_resize_object_add (win, bg);
    evas_object_show(bg);

    Evas_Object* vbox = elm_box_add(win);
    evas_object_size_hint_weight_set (vbox, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    elm_win_resize_object_add (win, vbox);
    evas_object_show(vbox);

    // pulsante offline mode
    Evas_Object* btn_silent = elm_button_add(win);
    elm_button_label_set(btn_silent, _("Silent mode"));

    evas_object_size_hint_weight_set (btn_silent, EVAS_HINT_EXPAND, 0.0);
    evas_object_size_hint_align_set (btn_silent, EVAS_HINT_FILL, 0.0);
    evas_object_show(btn_silent);

    //evas_object_smart_callback_add(btn_silent, "clicked", _poweroff_confirm, NULL);

    elm_box_pack_end(vbox, btn_silent);

    // pulsante offline mode
    Evas_Object* btn_offline = elm_button_add(win);
    elm_button_label_set(btn_offline, _("Offline mode"));

    evas_object_size_hint_weight_set (btn_offline, EVAS_HINT_EXPAND, 0.0);
    evas_object_size_hint_align_set (btn_offline, EVAS_HINT_FILL, 0.0);
    evas_object_show(btn_offline);

    evas_object_smart_callback_add(btn_offline, "clicked", _offline_mode, NULL);

    elm_box_pack_end(vbox, btn_offline);

    // pulsante power off
    Evas_Object* btn_pwroff = elm_button_add(win);
    elm_button_label_set(btn_pwroff, _("Power off"));

    evas_object_size_hint_weight_set (btn_pwroff, EVAS_HINT_EXPAND, 0.0);
    evas_object_size_hint_align_set (btn_pwroff, EVAS_HINT_FILL, 0.0);
    evas_object_show(btn_pwroff);

    evas_object_smart_callback_add(btn_pwroff, "clicked", _poweroff_confirm, NULL);

    elm_box_pack_end(vbox, btn_pwroff);
}
