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

#include "globals.h"

/* default category */
#define DEFAULT_CATEGORY        "unknown"
#define DEFAULT_ICON            MOKOPANEL_DATADIR "/unknown.png"

typedef struct {

    /* sequence ID for notifications */
    guint sequence;

    /* panel window */
    Evas_Object* win;

    /* panel layout */
    Evas_Object* layout;

    /* page container */
    Evas_Object* pager;

    /* main box for front page content */
    Evas_Object* hbox;

    /* filler */
    Evas_Object* fill;

    /* clock */
    Evas_Object* time;

    /* date */
    Evas_Object* date;
    gboolean date_pushed;

    /* battery */
    Evas_Object* battery;

    /* gsm */
    Evas_Object* gsm;

    /* gps */
    Evas_Object* gps;

    /* topmost object */
    Evas_Object* topmost;

    /* categories (mapping to Eina_List of MokoNotification) */
    GHashTable* categories;

    /* notification icons list in front page */
    Eina_List* list;

    /* coda delle notifiche testuali da visualizzare (array [ id nella lista, testo, icona ]) */
    GQueue* queue;

    /* coda delle notifiche testuali da ripresentare (come queue) */
    GQueue* represent;

    /* generic callback */
    void* callback;

    /* TRUE if the panel has already called strut */
    gboolean has_strut;

    /* D-Bus notification service */
    GObject* service;

} MokoPanel;

#if 0
typedef struct {

    /* panel stuff */
    MokoPanel* panel;

    /* notification window stuff */
    Evas_Object* list;
    Elm_Genlist_Item* item;
    Evas_Object* win;

    /* list of notifications in this group */
    Eina_List*

    /* icon path */
    char* icon_path;

    /* first line of list item */
    char* summary_format;
    char* summary;
    /* second line of list item */
    char* body_format;
    char* body;

    /* notification category */
    char* category;

} MokoNotificationGroup;
#endif

typedef struct {

    /* panel stuff */
    MokoPanel* panel;
    guint id;

    /* notification window stuff */
    #if 0
    MokoNotificationGroup* group;
    #else
    Evas_Object* list;
    Elm_Genlist_Item* item;
    Evas_Object* win;
    #endif

    /* icon object - might be shared across notification of same category */
    Evas_Object* icon;
    /* icon path */
    char* icon_path;

    /* notification summary - first line of list item */
    char* summary;
    /* notification body - scrolled text in front page and second line of list item */
    char* body;

    char** actions;
    int timeout;

    /* notification category -- extracted from hints */
    char* category;

    /* don't push scrolled text -- extracted from hints */
    gboolean dont_push;
    /* push on resume -- extracted from hints */
    gboolean show_on_resume;
    /* insist with sound -- extracted from hints */
    gboolean insistent;
    /* ongoing event -- extracted from hints */
    gboolean ongoing;
    /* don't clear on clear all -- extracted from hints */
    gboolean no_clear;
    /* delete on click -- extracted from hints */
    gboolean autodel;

} MokoNotification;

#define PANEL_HEIGHT        (SCALE_FACTOR * 28)
#define PANEL_WIDTH         (SCALE_FACTOR * 240)
#define ICON_SIZE           (SCALE_FACTOR * 24)

#define MOKOPANEL_CALLBACK_NOTIFICATION_START       1
#define MOKOPANEL_CALLBACK_NOTIFICATION_END         2
#define MOKOPANEL_CALLBACK_NOTIFICATION_HIDE        3

typedef void (*MokoPanelCallback)(MokoPanel* panel, int event, gpointer data);

void mokopanel_append_object(MokoPanel* panel, Evas_Object* obj);

void mokopanel_fire_event(MokoPanel* panel, int event, gpointer data);
void mokopanel_event(MokoPanel* panel, int event, gpointer data);

void mokopanel_notification_represent(MokoPanel* panel);
int mokopanel_count_notifications(MokoPanel* panel, const char* category);
Elm_Genlist_Item* mokopanel_get_list_item(MokoPanel* panel, const char* category);
Eina_List* mokopanel_get_category(MokoPanel* panel, const char* category);

void mokopanel_notification_remove(MokoPanel* panel, guint id);
guint mokopanel_notification_queue(MokoPanel* panel,
        const char* app_name, guint id, const char* icon, const char* summary,
        const char* body, char** actions, int actions_length, GHashTable* hints,
        int timeout);

MokoPanel* mokopanel_new(const char* name, const char* title);

char** mokopanel_notification_caps(MokoPanel* panel, int* length);

#endif  /* __PANEL_H */
