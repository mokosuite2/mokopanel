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

#include <Elementary.h>
#include <Ecore_X.h>
#include <Ecore_X_Atoms.h>
#include <glib.h>
#include <mokosuite/utils/settingsdb.h>
#include <mokosuite/utils/misc.h>
#include <freesmartphone-glib/freesmartphone-glib.h>
#include <freesmartphone-glib/odeviced/idlenotifier.h>
#include <freesmartphone-glib/ogsmd/call.h>
#include <freesmartphone-glib/ousaged/usage.h>
#include <freesmartphone-glib/opimd/calls.h>
#include <freesmartphone-glib/opimd/callquery.h>
#include <freesmartphone-glib/opimd/call.h>
#include <freesmartphone-glib/opimd/messagequery.h>
#include <freesmartphone-glib/opimd/messages.h>
#include <freesmartphone-glib/opimd/message.h>

#include "globals.h"
#include "panel.h"
#include "idle.h"
#include "notifications-misc.h"
#include "notifications-win.h"

#include <glib/gi18n-lib.h>

static Evas_Object* notification_win = NULL;
static Evas_Object* notification_list = NULL;
static Elm_Genlist_Item_Class itc = {0};
static gboolean notification_show = FALSE;

static MokoPanel* current_panel = NULL;
static Evas_Object* operator_label = NULL;

static void _keyboard_click(void* data, Evas_Object* obj, void* event_info)
{
    system("killall -USR1 mokowm");
    notify_window_hide();
}

static void _close_handle_click(void* data, Evas_Object* obj, void* event_info)
{
    notify_window_hide();
}

static void _focus_out(void* data, Evas_Object* obj, void* event_info)
{
    //g_debug("FOCUS OUT (%d)", notification_show);
    if (notification_show)
        notify_window_hide();
}

static void _list_selected(void *data, Evas_Object *obj, void *event_info)
{
    MokoNotification* n = (MokoNotification*) elm_genlist_item_data_get((const Elm_Genlist_Item *)event_info);

    elm_genlist_item_selected_set((Elm_Genlist_Item*)event_info, FALSE);

    // TODO notify_internal_exec(n);

    notify_window_hide();
}

static char* notification_genlist_label_get(void *data, Evas_Object * obj, const char *part)
{
    Eina_List* group = mokopanel_get_category(
        (MokoPanel*) evas_object_data_get(obj, "panel"),
        (char*) data);

    if (group) {
        MokoNotification* n = group->data;

        if (!strcmp(part, "elm.text")) {

            /*
            char* fmt = (c == 1) ? n->type->description1 : n->type->description2;
            if (n->type->format_count)
                return g_strdup_printf(fmt, c);
            else
                return g_strdup(fmt);
            */
            return g_strdup("TODO");

        }

        else if (!strcmp(part, "elm.text.sub")) {

            //return (c == 1) ? g_strdup(n->subdescription) : NULL;
            return g_strdup("TODO");
        }
    }
    return NULL;
}

static Evas_Object* notification_genlist_icon_get(void *data, Evas_Object * obj, const char *part)
{
    Eina_List* group = mokopanel_get_category(
        (MokoPanel*) evas_object_data_get(obj, "panel"),
        (char*) data);
    if (group) {
        MokoNotification* n = group->data;

        if (!strcmp(part, "elm.swallow.icon")) {
            // icona notifica
            Evas_Object *icon = elm_icon_add(n->win);
            elm_icon_file_set(icon, n->icon_path, NULL);
            //evas_object_size_hint_min_set(icon, 100, 100);
            // TODO icona dimensionata correttamente? :S
            //elm_icon_smooth_set(icon, TRUE);
            elm_icon_no_scale_set(icon, TRUE);
            elm_icon_scale_set(icon, FALSE, FALSE);
            evas_object_show(icon);

            return icon;
        }
    }

    return NULL;
}

/**
 * Aggiunge una notifica alla lista.
 */
void notification_window_add(MokoNotification* n)
{
    g_return_if_fail(n != NULL);

    n->list = notification_list;
    n->win = notification_win;

    Elm_Genlist_Item* item = mokopanel_get_list_item(n->panel, n->category);
    if (item) {
        EINA_LOG_DBG("Item already found for %d, using %p", n->id, item);
        n->item = item;
        elm_genlist_item_update(n->item);
    }

    else {
        EINA_LOG_DBG("Creating new item for id %d", n->id);
        n->item = elm_genlist_item_append(notification_list, &itc, g_strdup(n->category),
            NULL, ELM_GENLIST_ITEM_NONE, NULL, NULL);
    }
}

/**
 * Rimuove una notifica dalla lista.
 * @param n
 * @param update_only
 */
void notification_window_remove(MokoNotification* n, gboolean update_only)
{
    EINA_LOG_DBG("Notification %d, Update_only = %s, Item = %p", n->id,
        update_only ? "TRUE" : "FALSE", n->item);

    if (update_only) {
        elm_genlist_item_update(n->item);
    }
    else {
        elm_genlist_item_del(n->item);
        n->item = NULL;
    }
}

void notify_window_init(MokoPanel* panel)
{
    Evas_Object* win = elm_win_add(NULL, "mokonotifications", ELM_WIN_BASIC);
    if (win == NULL) {
        g_critical("Cannot create notifications window; will not be able to read notifications");
        return;
    }

    elm_win_title_set(win, "Notifications");
    elm_win_borderless_set(win, TRUE);
    elm_win_sticky_set(win, TRUE);

    evas_object_smart_callback_add(win, "focus,out", _focus_out, NULL);
    evas_object_smart_callback_add(win, "delete,request", _focus_out, NULL);

    #if 0
    // FIXME FIXME FIXME!!!
    evas_object_resize(win, 480, 600);
    evas_object_move(win, 0, 40);
    #endif

    Ecore_X_Window_State state;

    state = ECORE_X_WINDOW_STATE_SKIP_TASKBAR;
    ecore_x_netwm_window_state_set(elm_win_xwindow_get(win), &state, 1);

    state = ECORE_X_WINDOW_STATE_SKIP_PAGER;
    ecore_x_netwm_window_state_set(elm_win_xwindow_get(win), &state, 1);

    state = ECORE_X_WINDOW_STATE_ABOVE;
    ecore_x_netwm_window_state_set(elm_win_xwindow_get(win), &state, 1);

    ecore_x_window_configure(elm_win_xwindow_get(win),
        ECORE_X_WINDOW_CONFIGURE_MASK_STACK_MODE,
        0, 0, 0, 0,
        0, 0,
        ECORE_X_WINDOW_STACK_ABOVE);

    Evas_Object* bg = elm_bg_add(win);
    evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    elm_win_resize_object_add(win, bg);
    evas_object_show(bg);

    Evas_Object* vbox = elm_box_add(win);
    evas_object_size_hint_weight_set(vbox, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    elm_win_resize_object_add(win, vbox);
    evas_object_show(vbox);

    // intestazione
    Evas_Object* hdrbox = elm_box_add(win);
    elm_box_horizontal_set(hdrbox, TRUE);
    evas_object_size_hint_weight_set(hdrbox, EVAS_HINT_EXPAND, 0.0);
    evas_object_size_hint_align_set(hdrbox, EVAS_HINT_FILL, 0.0);
    evas_object_show(hdrbox);

    // operatore gsm
    operator_label = elm_label_add(win);
    evas_object_size_hint_weight_set(operator_label, EVAS_HINT_EXPAND, 0.0);
    evas_object_size_hint_align_set(operator_label, 0.1, 0.5);
    evas_object_show(operator_label);

    elm_box_pack_start(hdrbox, operator_label);

    // TEST pulsante tastiera :)
    Evas_Object* vkb = elm_button_add(win);
    elm_button_label_set(vkb, _("Keyboard"));
    evas_object_size_hint_weight_set(vkb, 0.0, 0.0);
    evas_object_size_hint_align_set(vkb, 1.0, 0.5);
    evas_object_smart_callback_add(vkb, "clicked", _keyboard_click, NULL);
    evas_object_show(vkb);

    elm_box_pack_end(hdrbox, vkb);

    // aggiungi l'intestazione alla finestra
    elm_box_pack_start(vbox, hdrbox);

    Evas_Object* list = elm_genlist_add(win);
    evas_object_data_set(list, "panel", panel);
    elm_genlist_bounce_set(list, FALSE, FALSE);
    evas_object_smart_callback_add(list, "selected", _list_selected, NULL);

    evas_object_size_hint_weight_set(list, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set(list, EVAS_HINT_FILL, EVAS_HINT_FILL);

    itc.item_style = "generic_sub";
    itc.func.label_get = notification_genlist_label_get;
    itc.func.icon_get = notification_genlist_icon_get;

    elm_box_pack_end(vbox, list);
    evas_object_show(list);

    Evas_Object* close_handle = elm_button_add(win);
    elm_button_label_set(close_handle, "[close]");
    evas_object_smart_callback_add(close_handle, "clicked", _close_handle_click, NULL);

    evas_object_size_hint_weight_set(close_handle, EVAS_HINT_EXPAND, 0.0);
    evas_object_size_hint_align_set(close_handle, EVAS_HINT_FILL, 0.0);

    elm_box_pack_end(vbox, close_handle);
    evas_object_show(close_handle);

    notification_win = win;
    notification_list = list;
    current_panel = panel;
}

void notify_window_start(void)
{
    g_return_if_fail(notification_win != NULL);

    if (!evas_object_visible_get(notification_win)) {
        // notifica al pannello l'evento
        mokopanel_fire_event(current_panel, MOKOPANEL_CALLBACK_NOTIFICATION_START, NULL);
    }
}

void notify_window_end(void)
{
    g_return_if_fail(notification_win != NULL);

    notify_window_show();
    notification_show = TRUE;

    // notifica al pannello l'evento
    mokopanel_fire_event(current_panel, MOKOPANEL_CALLBACK_NOTIFICATION_END, NULL);
}

void notify_window_show(void)
{
    g_return_if_fail(notification_win != NULL);

    evas_object_show(notification_win);
    elm_win_activate(notification_win);
    evas_object_focus_set(notification_win, TRUE);
}

void notify_window_hide(void)
{
    g_return_if_fail(notification_win != NULL);

    evas_object_hide(notification_win);

    notification_show = FALSE;

    // notifica al pannello l'evento
    mokopanel_fire_event(current_panel, MOKOPANEL_CALLBACK_NOTIFICATION_HIDE, NULL);
}

void notify_window_update_operator(const char* operator)
{
    if (operator_label) {
        char* op = g_strdup_printf("<b><font_size=10>%s</></b>", operator);
        elm_label_label_set(operator_label, op);
        g_free(op);
    }
}
