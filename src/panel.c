/*
 * MokoPanel
 * Notification panel
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

#include <Ecore_X.h>
#include <Elementary.h>
#include <Efreet.h>

#include <mokosuite/utils/utils.h>
#include <mokosuite/utils/misc.h>

#include "panel.h"
#include "clock.h"
#include "battery.h"
#include "gsm.h"
#include "notifications-win.h"
#include "notifications-service.h"


static void process_notification_queue(gpointer data);

static char* get_real_icon(const char* name)
{
    if (g_str_has_prefix(name, "file://")) {
        return g_strdup(name + strlen("file://"));
    }

    return efreet_icon_path_find(NULL, name, ICON_SIZE);
}

static void _panel_mouse_down(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
    notify_window_start();
}

static void _panel_mouse_up(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
    notify_window_end();
}

static void free_notification(gpointer data)
{
    MokoNotification *data2 = data;

    // cancella icona solo se tutte le notifiche di quel tipo sono andate
    MokoPanel* panel = data2->panel;
    gboolean update_only = TRUE;

    Eina_List* iter;
    void* _data;
    MokoNotification* no;
    EINA_LIST_FOREACH(panel->list, iter, data) {
        no = _data;
        if (data2 != no && !strcmp(data2->category, no->category)) goto no_icon;
    }

    // tutte le notifiche di quel tipo sono andate, cancella icona
    g_debug("Deleting notification icon %p for type '%s'", data2->icon, data2->type->name);
    evas_object_del(data2->icon);

    // cancella notifica in lista :)
    update_only = FALSE;

no_icon:
    // rimuovi dalla finestra delle notifiche
    notification_window_remove(data2, update_only);

    g_debug("Freeing notification %d", data2->sequence);
    g_free(data2->summary);
    g_free(data2->body);

    #error free other things
    g_free(data);
}

static gboolean do_pop_text_notification(gpointer data)
{
    evas_object_del((Evas_Object *) data);
    return FALSE;
}

// rimuove la hbox contenente una notifica di testo
static gboolean pop_text_notification(gpointer data)
{
    Evas_Object* obj = (Evas_Object *) data;
    MokoPanel* panel = (MokoPanel *) evas_object_data_get(obj, "panel");

    g_queue_pop_head(panel->queue);

    // se c'e' dell'altro, continua a processare
    if (panel->queue->length > 0)
        process_notification_queue(panel);
    else {
        if (panel->topmost == panel->date && !panel->date_pushed) {
            evas_object_show(panel->topmost);
            elm_pager_content_push(panel->pager, panel->date);
        }

        elm_pager_content_promote(panel->pager, panel->topmost);
    }

    g_timeout_add(500, do_pop_text_notification, obj);

    return FALSE;
}

// processa una notifica di testo dalla coda
static void process_notification_queue(gpointer data)
{
    MokoPanel* panel = (MokoPanel *) data;

    // recupera la notifica in testa alla coda e visualizzala
    gpointer* in_data = (gpointer*) g_queue_peek_head(panel->queue);

    // questo non dovrebbe accadare
    g_return_if_fail(in_data != NULL);

    Evas_Object* msgbox = elm_box_add(panel->win);
    elm_box_horizontal_set(msgbox, TRUE);
    evas_object_size_hint_weight_set (msgbox, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_show(msgbox);

    // salva il panel per uso futuro
    evas_object_data_set(msgbox, "panel", panel);

    Evas_Object *ic = elm_icon_add(panel->win);
    elm_icon_file_set(ic, (const char*) in_data[1], NULL);
    elm_icon_no_scale_set(ic, TRUE);
    #ifdef QVGA
    elm_icon_scale_set(ic, FALSE, TRUE);
    #else
    elm_icon_scale_set(ic, TRUE, TRUE);
    #endif

    evas_object_size_hint_min_set(ic, ICON_SIZE, ICON_SIZE);
    evas_object_size_hint_align_set(ic, 0.5, 0.5);
    evas_object_show(ic);

    elm_box_pack_end(msgbox, ic);

    Evas_Object* lmsg = elm_label_add(panel->win);
    elm_label_label_set(lmsg, (const char*) in_data[0]);
    elm_object_style_set(lmsg, "panel");

    evas_object_size_hint_weight_set (lmsg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set(lmsg, -1.0, 0.5);
    evas_object_show(lmsg);

    elm_box_pack_end(msgbox, lmsg);

    elm_pager_content_push(panel->pager, msgbox);

    // aggiungi il timeout per la rimozione
    g_timeout_add_seconds(3, pop_text_notification, msgbox);

    // libera tutto
    g_free(in_data[0]);
    g_free(in_data[1]);
    g_free(in_data);
}

/**
 * Prepara un'array di gpointer per uso dei pusher interni.
 * Se id >= 0 aggiunge un terzo elemento all'array
 */
static gpointer* prepare_text_struct(const char* text, const char* icon, int id)
{
    // FIXME splittare la stringa con i newline!!!

    gpointer* data = g_new0(gpointer, 2 + (id >= 0 ? 1 : 0));
    data[0] = g_strdup(text);
    data[1] = g_strdup(icon);

    if (id >= 0)
        data[2] = GINT_TO_POINTER(id);

    return data;
}

// aggiunge la notifica di testo al pannello
static void push_text_notification(MokoPanel* panel, const char* text, const char* icon)
{
    gpointer* data = prepare_text_struct(text, icon, -1);
    g_queue_push_tail(panel->queue, data);

    // fai partire il processamento se e' la prima notifica
    if (panel->queue->length == 1)
        process_notification_queue(panel);
}

// aggiunge la notifica alla coda dei ripresentati
static void push_represent(MokoPanel* panel, int id, const char* text, const char* icon)
{
    gpointer* data = prepare_text_struct(text, icon, id);
    g_queue_push_tail(panel->represent, data);
}

/**
 * Gestore degli eventi del pannello predefinito.
 */
void mokopanel_event(MokoPanel* panel, int event, gpointer data)
{
    g_debug("Mokopanel event %d", event);

    switch (event) {
        case MOKOPANEL_CALLBACK_NOTIFICATION_START:
            if (panel->date == NULL) {
                panel->date = elm_label_add(panel->win);

                evas_object_data_set(panel->date, "panel", panel);
                evas_object_size_hint_weight_set (panel->date, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
                evas_object_size_hint_align_set (panel->date, EVAS_HINT_FILL, EVAS_HINT_FILL);
            }

            // update date label
            guint64 now = get_current_time();
            struct tm* timestamp_tm = localtime((const time_t*)&now);
            char strf[50+1] = {0, };
            strftime(strf, 50, "%Y-%m-%d", timestamp_tm);
            char* datestr = g_strdup_printf(" <b>%s</b>", strf);
            elm_label_label_set(panel->date, datestr);
            g_free(datestr);

            if (elm_pager_content_top_get(panel->pager) == panel->hbox) {
                panel->date_pushed = TRUE;
                evas_object_show(panel->date);
                elm_pager_content_push(panel->pager, panel->date);
            }
            panel->topmost = panel->date;

            break;

        case MOKOPANEL_CALLBACK_NOTIFICATION_HIDE:
            panel->topmost = panel->hbox;
            if (panel->date && elm_pager_content_top_get(panel->pager) == panel->date) {
                elm_pager_content_promote(panel->pager, panel->topmost);
            }

            break;
    }
}

/**
 * Notifica un evento al pannello.
 */
void mokopanel_fire_event(MokoPanel* panel, int event, gpointer data)
{
    if (panel && panel->callback) {
        MokoPanelCallback cb = panel->callback;
        (cb)(panel, event, data);
    }
}

/**
 * Conta le notifiche di una categoria dato.
 * @param panel
 * @param category
 * @return il conteggio
 */
int mokopanel_count_notifications(MokoPanel* panel, const char* category)
{
    guint count = 0;
    Eina_List* iter;
    void* _data;
    MokoNotification* data;
    EINA_LIST_FOREACH(panel->list, iter, data) {
        data = _data;
        if (!strcmp(data->category, category))
            count++;
    }

    return count;
}

/**
 * Restituisce l'item della lista della categoria data.
 * @param panel
 * @param category
 * @return l'item
 */
Elm_Genlist_Item* mokopanel_get_list_item(MokoPanel* panel, const char* category)
{
    Eina_List* iter;
    void* _data;
    MokoNotification* data;
    EINA_LIST_FOREACH(panel->list, iter, data) {
        data = _data;
        if (!strcmp(data->category, category) && data->item)
            return data->item;
    }

    return NULL;
}

/**
 * Ri-pusha le notifiche con flag represent.
 */
void mokopanel_notification_represent(MokoPanel* panel)
{
    // parti dal primo elemento (il piu' vecchio) e pushalo
    GList* iter = panel->represent->head;
    while (iter) {
        gpointer* data = iter->data;
        push_text_notification(panel, (const char *) data[0], (const char *) data[1]);
        iter = iter->next;
    }
}

/**
 * Rimuove una notifica (rimuove l'icona dalla prima pagina)
 */
void mokopanel_notification_remove(MokoPanel* panel, guint id)
{
    g_return_if_fail(panel != NULL);

    // trova l'id (sigh)
    MokoNotification* data = NULL;
    int i;

    for (i = 0; i < panel->list->len; i++) {

        MokoNotification* data2 = g_ptr_array_index(panel->list, i);
        if (data2) {
            //g_debug("Current index: %d, id = %d (searching: %d)", i, data2->sequence, id);
            if (data2->sequence == id) {
                data = data2;
                break;
            }
        }
    }

    if (data != NULL) {
        // rimozione dai represent
        GList* iter = panel->represent->head;
        while (iter) {
            gpointer* data = iter->data;
            if (GPOINTER_TO_INT(data[2]) == id) {
                g_free(data[0]);
                g_free(data[1]);
                g_free(data);

                g_queue_delete_link(panel->represent, iter);
                break;
            }
            iter = iter->next;
        }

        // rimozione rapida -- cancellazione effettuata dall'array
        #error g_ptr_array_remove_index & free
        g_ptr_array_remove_index(panel->list, i);
    }
}

/**
 * Aggiunge una notifica in coda e ne restituisce l'ID per una futura rimozione.
 * Se il testo è NULL, la notifica è inserita solamente in prima pagina.
 * Se il testo è diverso da NULL, la notifica sarà visualizzata per qualche
 * secondo, riga per riga; dopodiché sarà inserita in prima pagina.
 * 
 * TODO
 * @return l'ID univoco della notifica
 */
guint mokopanel_notification_queue(MokoPanel* panel,
        const char* app_name, guint id, const char* icon, const char* summary,
        const char* body, char** actions, int actions_length, GHashTable* hints,
        int timeout)
{
    // some check...
    g_return_val_if_fail(panel != NULL, 0);

    Evas_Object *ic = NULL;
    MokoNotification *data = NULL;
    guint seq;

    Eina_List *iter;
    void* _data;

    // get category
    const char* catg = map_get_string(hints, "category");
    Eina_List* catg_list = NULL;

    if (!catg || !strlen(catg))
        catg = DEFAULT_CATEGORY;

    // get icon
    if (!icon) {
        // TODO support for icon_data hint
        icon = g_strdup(DEFAULT_ICON);
    }

    else {
        // retrieve real icon path
        icon = get_real_icon(icon);
    }

    if (!icon)
        icon = g_strdup(DEFAULT_ICON);

    catg_list = g_hash_table_lookup(panel->categories, catg);

    // intanto appendi l'icona in prima pagina se non e' gia' presente
    // della stessa categoria
    EINA_LIST_FOREACH(panel->list, iter, data) {
        data = _data;
        if (!strcmp(data->category, catg)) {
            ic = data->icon;
            goto no_icon;
        }
    }

    ic = elm_icon_add(panel->win);
    elm_icon_file_set(ic, icon, NULL);
    elm_icon_no_scale_set(ic, TRUE);
    #ifdef QVGA
    elm_icon_scale_set(ic, FALSE, TRUE);
    #else
    elm_icon_scale_set(ic, TRUE, TRUE);
    #endif
    evas_object_size_hint_align_set(ic, 0.5, 0.5);
    evas_object_size_hint_min_set(ic, ICON_SIZE, ICON_SIZE);
    evas_object_show(ic);
    elm_box_pack_before(panel->hbox, ic, panel->fill);

no_icon:
    seq = panel->sequence;
    if ((seq + 1) >  G_MAXUINT) panel->sequence = 1;
    else panel->sequence++;

    data = g_new0(MokoNotification, 1);
    data->panel = panel;
    data->sequence = seq;
    data->icon = ic;
    data->summary = g_strdup(summary);
    data->body = g_strdup(body);
    data->category = g_strdup(catg);

    // flags
    data->dont_push = map_get_bool(hints, "x-mokosuite-dont-push", TRUE);
    data->show_on_resume = map_get_bool(hints, "x-mokosuite-show-on-resume", TRUE);
    data->insistent = map_get_bool(hints, "x-mokosuite-insistent", TRUE);
    data->ongoing = map_get_bool(hints, "x-mokosuite-ongoing", TRUE);
    data->no_clear = map_get_bool(hints, "x-mokosuite-noclear", TRUE);
    data->autodel = map_get_bool(hints, "x-mokosuite-autodel", TRUE);

    panel->list = eina_list_append(panel->list, data);

    // aggiungi alla finestra delle notifiche
    notification_window_add(data);

    if (body != NULL) {
        // pusha se richiesto
        if (!data->dont_push)
            push_text_notification(panel, body, icon);

        // ripresenta notifica
        if (data->show_on_resume)
            push_represent(panel, seq, body, icon);
    }

    return seq;
}

MokoPanel* mokopanel_new(const char* name, const char* title)
{
    Ecore_X_Window xwin;
    Ecore_X_Window_State states[3];

    MokoPanel* panel = g_new0(MokoPanel, 1);

    panel->queue = g_queue_new();
    panel->represent = g_queue_new();
    panel->callback = mokopanel_event;

    /*
     * The categories table maps notifications sharing the same category.
     * It must not have a free function for value since notification deallocation
     * is manually managed by panel.
     */
    panel->categories = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);

    // Notification Service
    panel->service = (GObject *) notifications_service_new(panel);

    // the panel window
    panel->win = elm_win_add(NULL, name, ELM_WIN_DOCK);
    elm_win_title_set(panel->win, title);
    elm_win_layer_set(panel->win, 200);    // indicator layer :)

    xwin = elm_win_xwindow_get(panel->win);
    ecore_x_icccm_hints_set(xwin, 0, 0, 0, 0, 0, 0, 0);
    states[0] = ECORE_X_WINDOW_STATE_SKIP_TASKBAR;
    states[1] = ECORE_X_WINDOW_STATE_SKIP_PAGER;
    states[2] = ECORE_X_WINDOW_STATE_ABOVE;
    ecore_x_netwm_window_state_set(xwin, states, 3);

    Evas_Object *bg = elm_bg_add(panel->win);
    elm_object_style_set(bg, "panel");
    evas_object_size_hint_weight_set (bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    elm_win_resize_object_add (panel->win, bg);
    evas_object_show(bg);

    // main layout
    panel->layout = elm_layout_add(panel->win);
    elm_layout_file_set(panel->layout, MOKOPANEL_DATADIR "theme.edj", "panel");

    evas_object_size_hint_weight_set (panel->layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    elm_win_resize_object_add (panel->win, panel->layout);

    evas_object_show(panel->layout);

    // the front page! :)
    panel->pager = elm_pager_add(panel->win);
    elm_object_style_set(panel->pager, "panel");

    evas_object_size_hint_weight_set (panel->pager, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_weight_set (panel->pager, EVAS_HINT_FILL, EVAS_HINT_FILL);

    evas_object_show(panel->pager);

    elm_layout_content_set(panel->layout, "content", panel->pager);

    // main hbox
    panel->hbox = panel->topmost = elm_box_add(panel->win);
    elm_box_horizontal_set(panel->hbox, TRUE);
    evas_object_size_hint_weight_set (panel->hbox, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_show(panel->hbox);

    elm_pager_content_push(panel->pager, panel->hbox);

    // filler bg
    panel->fill = elm_bg_add(panel->win);
    evas_object_size_hint_weight_set (panel->fill, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set(panel->fill, EVAS_HINT_FILL, EVAS_HINT_FILL);
    evas_object_show(panel->fill);
    elm_box_pack_end(panel->hbox, panel->fill);

    // gsm
    panel->gsm = gsm_applet_new(panel);
    elm_box_pack_end(panel->hbox, panel->gsm);

    // battery
    panel->battery = battery_applet_new(panel);
    elm_box_pack_end(panel->hbox, panel->battery);

    // clock
    panel->time = clock_applet_new(panel);
    elm_box_pack_end(panel->hbox, panel->time);

    // mouse callback
    Evas_Object* ev = evas_object_rectangle_add(evas_object_evas_get(panel->win));
    evas_object_color_set(ev, 0, 0, 0, 0);

    evas_object_event_callback_add(ev, EVAS_CALLBACK_MOUSE_DOWN, _panel_mouse_down, NULL);
    evas_object_event_callback_add(ev, EVAS_CALLBACK_MOUSE_UP, _panel_mouse_up, NULL);

    elm_win_resize_object_add(panel->win, ev);
    evas_object_show(ev);

    ecore_x_event_mask_set(xwin, ECORE_X_EVENT_MASK_WINDOW_VISIBILITY);

    evas_object_size_hint_min_set(panel->layout, PANEL_WIDTH, PANEL_HEIGHT);
    evas_object_resize(panel->win, PANEL_WIDTH, PANEL_HEIGHT);

    evas_object_show(panel->win);

    // finestra notifiche
    notify_window_init(panel);

    return panel;
}