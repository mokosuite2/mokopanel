/*
 * Mokosuite
 * Notification panel window
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

#ifndef __PANEL_H
#define __PANEL_H

#include <Elementary.h>
#include <glib-object.h>

struct _MokoPanel {

    /* sequence per gli ID delle notifiche */
    guint sequence;

    /* finestra del pannello */
    Evas_Object* win;

    /* layout del pannello */
    Evas_Object* layout;

    /* contenitore pagine */
    Evas_Object* pager;

    /* box principale contenuto prima pagina */
    Evas_Object* hbox;

    /* filler */
    Evas_Object* fill;

    /* orologio */
    Evas_Object* time;

    /* data */
    Evas_Object* date;
    gboolean date_pushed;

    /* batteria */
    Evas_Object* battery;

    /* gsm */
    Evas_Object* gsm;

    /* oggetto in primo piano */
    Evas_Object* topmost;

    /* definizione tipi */
    GHashTable* types;

    /* lista icone di notifica in prima pagina */
    GPtrArray* list;

    /* coda delle notifiche testuali da visualizzare (array [ id nella lista, testo, icona ]) */
    GQueue* queue;

    /* coda delle notifiche testuali da ripresentare (come queue) */
    GQueue* represent;

    /* callback per uso generico */
    gpointer callback;

    /* flag se abbiamo gia' struttato */
    gboolean has_strut;

    /* servizio dbus notifiche */
    GObject* service;
};

typedef struct _MokoPanel MokoPanel;

struct _MokoNotificationType {
    char* name;
    char* icon;
    char* description1;
    char* description2;
    gboolean format_count;
};

typedef struct _MokoNotificationType MokoNotificationType;

struct _MokoNotification {
    /* roba della finestra delle notifiche */
    Evas_Object* list;
    Elm_Genlist_Item* item;
    Evas_Object* win;

    /* roba del pannello */
    MokoPanel* panel;
    guint sequence;

    Evas_Object* icon;
    char* text;
    char* subdescription;
    MokoNotificationType* type;
};

typedef struct _MokoNotification MokoNotification;

#define PANEL_HEIGHT        (MOKOSUITE_SCALE_FACTOR * 28)
#define PANEL_WIDTH         (MOKOSUITE_SCALE_FACTOR * 240)
#define ICON_SIZE           (MOKOSUITE_SCALE_FACTOR * 24)

#define MOKOPANEL_CALLBACK_NOTIFICATION_START       1
#define MOKOPANEL_CALLBACK_NOTIFICATION_END         2
#define MOKOPANEL_CALLBACK_NOTIFICATION_HIDE        3

typedef void (*MokoPanelCallback)(MokoPanel* panel, int event, gpointer data);

void mokopanel_fire_event(MokoPanel* panel, int event, gpointer data);
void mokopanel_event(MokoPanel* panel, int event, gpointer data);

void mokopanel_notification_represent(MokoPanel* panel);
int mokopanel_count_notifications(MokoPanel* panel, const char* type);
Elm_Genlist_Item* mokopanel_get_list_item(MokoPanel* panel, const char* type);

void mokopanel_notification_remove(MokoPanel* panel, int id);
int mokopanel_notification_queue(MokoPanel* panel,
        const char* text,
        const char* type,
        const char* subdescription,
        int flags);

void mokopanel_register_notification_type(MokoPanel* panel,
        const char* type,
        const char* icon,
        const char* description1,
        const char* description2,
        gboolean format_count);

MokoPanel* mokopanel_new(const char* name, const char* title);

#endif  /* __PANEL_H */
