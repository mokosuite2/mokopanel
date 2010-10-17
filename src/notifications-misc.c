/*
 * Mokosuite
 * Miscellaneous panel notifications
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
#include <libmokosuite/mokosuite.h>
#include <libmokosuite/settings-service.h>
#include <libmokosuite/misc.h>
#include <libmokosuite/notifications.h>
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

#include "panel.h"
#include "idle.h"
#include "notifications-misc.h"

#include <glib/gi18n-lib.h>

static MokoPanel* current_panel = NULL;
static GArray* calls = NULL;

// tipi notifiche per le quali conosciamo i comandi
#define TYPE_ACTIVE_CALL     "active-call"
#define TYPE_MISSED_CALL     "missed-call"
#define TYPE_UNREAD_MESSAGE  "unread-message"
#define TYPE_UNREAD_USSD     "unread-ussd"

// comandi notifiche (FIXME per ora cablati)
#define CMD_ACTIVE_CALL     "/usr/bin/mokophone-activate.sh Frontend string:calls"
#define CMD_MISSED_CALL     "/usr/bin/mokophone-activate.sh Frontend string:log"
#define CMD_UNREAD_MESSAGE  "/usr/bin/phoneui-messages"
#define CMD_UNREAD_USSD     "/usr/bin/mokophone-activate.sh"

// dati per i segnali di modifica/cancellazione chiamate o messaggi
typedef struct {
    int id;
    gpointer update_signal;
    gpointer delete_signal;
} notification_signal_t;

static void call_status(gpointer data, const int id, const int status, GHashTable *props)
{
    g_debug("CallStatus[%d]=%d", id, status);
    int i;
    gboolean call_exists = FALSE;

    switch (status) {
        case CALL_STATUS_INCOMING:
        case CALL_STATUS_OUTGOING:
            // controlla se la chiamata e' gia presente
            for (i = 0; i < calls->len; i++) {
                // chiamata gia' aggiunta, esci
                if (g_array_index(calls, int, i) == id) {
                    call_exists = TRUE;
                    goto end;
                }
            }

            // aggiungi la chiamata
            g_array_append_val(calls, id);

        end:
            if (status == CALL_STATUS_INCOMING) {
                // disattiva idlescreen e screensaver
                screensaver_off();

                // richiedi display
                ousaged_usage_request_resource("Display", NULL, NULL);
            }

            // disabilita idle screen
            idle_hide();
            break;

        case CALL_STATUS_ACTIVE:
            ousaged_usage_release_resource("Display", NULL, NULL);

            break;

        case CALL_STATUS_RELEASE:
            // togli la chiamata dalla lista se presente
            for (i = 0; i < calls->len; i++) {
                if (g_array_index(calls, int, i) == id) {
                    g_array_remove_index_fast(calls, i);
                    break;
                }
            }

            ousaged_usage_release_resource("Display", NULL, NULL);

            if (!calls->len) {
                // ultima chiamata chiusa, togli screensaver
                screensaver_off();
                idle_hide();
            }

            break;

        case CALL_STATUS_HELD:
            g_debug("Held call!");
            // tanto non funziona... :(
            break;
    }

}

#if 0
static int handle_call_status(const char* status)
{
    int st;

    if (!strcmp(status, DBUS_CALL_STATUS_INCOMING)) {
        st = CALL_STATUS_INCOMING;
    }
    else if (!strcmp(status, DBUS_CALL_STATUS_OUTGOING)) {
        // Display outgoing UI
    }
    else if (!strcmp(status, DBUS_CALL_STATUS_ACTIVE)) {
        st = CALL_STATUS_ACTIVE;
    }
    else if (!strcmp(status, DBUS_CALL_STATUS_RELEASE)) {
        st = CALL_STATUS_RELEASE;
    }
    else {
        st = CALL_STATUS_HELD;
    }

    return st;
}

static void list_calls(GError *e, GPtrArray * calls, gpointer data)
{
    g_return_if_fail(e == NULL);

    int i;

    for (i = 0; i < calls->len; i++) {
        GHashTable* props = (GHashTable *) g_ptr_array_index(calls, i);

        call_status(fso_get_attribute_int(props, "id"),
            handle_call_status(fso_get_attribute(props, "status")),
            props);
    }
}
#endif

typedef struct {
    /* liberati subito */
    GHashTable* query;
    /* mantenuti fino alla fine */
    gpointer userdata;
    char* path;
} query_data_t;

static void call_query(GError* error, const char* path, gpointer userdata);
static void call_data(GError* error, GHashTable* props, gpointer data);

static void call_next(GError* error, GHashTable* row, gpointer userdata)
{
    query_data_t* data = userdata;

    if (error) {
        g_debug("[%s] Call row error: %s", __func__, error->message);

        // distruggi query
        opimd_callquery_dispose(data->path, NULL, NULL);

        // distruggi dati callback
        g_free(data->path);
        g_free(data);
        return;
    }

    // aggiungi! :)
    call_data(NULL, row, NULL);

    // prossimo risultato
    opimd_callquery_get_result(data->path, call_next, data);
}

static gboolean retry_call_query(gpointer userdata)
{
    query_data_t* data = userdata;
    opimd_calls_query(data->query, call_query, data);
    return FALSE;
}

static void call_query(GError* error, const char* path, gpointer userdata)
{
    query_data_t* data = userdata;
    if (error) {
        g_debug("Missed calls query error: (%d) %s", error->code, error->message);

        // opimd non ancora caricato? Riprova in 5 secondi
        if (FREESMARTPHONE_GLIB_IS_DBUS_ERROR(error, FREESMARTPHONE_GLIB_DBUS_ERROR_SERVICE_NOT_AVAILABLE)) {
            g_timeout_add_seconds(5, retry_call_query, data);
            return;
        }

        g_hash_table_destroy(data->query);
        g_free(data);
        return;
    }

    g_hash_table_destroy(data->query);

    data->path = g_strdup(path);
    opimd_callquery_get_result(data->path, call_next, data);
}

static void missed_calls(GError* error, gint missed, gpointer data);

static gboolean retry_missed_calls(gpointer userdata)
{
    opimd_calls_get_new_missed_calls(missed_calls, userdata);
    return FALSE;
}

static void missed_calls(GError* error, gint missed, gpointer data)
{
    if (error) {
        g_debug("Missed calls error: (%d) %s", error->code, error->message);

        // opimd non ancora caricato? Riprova in 5 secondi
        if (FREESMARTPHONE_GLIB_IS_DBUS_ERROR(error, FREESMARTPHONE_GLIB_DBUS_ERROR_SERVICE_NOT_AVAILABLE))
            g_timeout_add_seconds(5, retry_missed_calls, data);

        return;
    }

    if (missed <= 0) return;

    // crea query chiamate senza risposta :)
    query_data_t* cbdata = g_new0(query_data_t, 1);

    cbdata->query = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_value_free);
    cbdata->userdata = data;

    g_hash_table_insert(cbdata->query, g_strdup("New"),
        g_value_from_int(1));
    g_hash_table_insert(cbdata->query, g_strdup("Answered"),
        g_value_from_int(0));
    g_hash_table_insert(cbdata->query, g_strdup("Direction"),
        g_value_from_string("in"));

    opimd_calls_query(cbdata->query, call_query, cbdata);
}

static void call_update(gpointer data, GHashTable* props)
{
    g_debug("Call has been modified - checking New attribute");
    // chiamata modificata - controlla new
    if (g_hash_table_lookup(props, "New") && !fso_get_attribute_int(props, "New")) {
        notification_signal_t* no = data;

        g_debug("New is 0 - removing missed call");
        mokopanel_notification_remove(current_panel, no->id);

        // disconnetti segnali fso
        opimd_call_call_updated_disconnect(no->update_signal);
        opimd_call_call_deleted_disconnect(no->delete_signal);

        g_free(no);
    }
}

static void call_remove(gpointer data)
{
    notification_signal_t* no = data;

    // chiamata cancellata - rimuovi sicuramente
    g_debug("Call has been deleted - removing missed call");

    mokopanel_notification_remove(current_panel, no->id);

    // disconnetti segnali fso
    opimd_call_call_updated_disconnect(no->update_signal);
    opimd_call_call_deleted_disconnect(no->delete_signal);

    g_free(no);
}

static void call_data(GError* error, GHashTable* props, gpointer data)
{
    if (error != NULL) {
        g_debug("Error getting new call data: %s", error->message);
        return;
    }

    const char* path = fso_get_attribute(props, "Path");

    // new
    gboolean is_new = (fso_get_attribute_int(props, "New") != 0);

    // answered
    gboolean answered = (fso_get_attribute_int(props, "Answered") != 0);

    // direction (incoming)
    const char* _direction = fso_get_attribute(props, "Direction");
    gboolean incoming = !strcasecmp(_direction, "in");

    g_debug("New call: answered=%d, new=%d, incoming=%d",
        answered, is_new, incoming);

    if (!answered && is_new && incoming) {
        const char* peer = fso_get_attribute(props, "Peer");
        char* text = g_strdup_printf(_("Missed call from %s"), peer);

        int id = mokopanel_notification_queue(current_panel,
            text, "missed-call", text,
            MOKOPANEL_NOTIFICATION_FLAG_REPRESENT);
        g_free(text);

        // connetti ai cambiamenti della chiamata per la rimozione
        notification_signal_t* no = g_new(notification_signal_t, 1);
        no->id = id;
        no->delete_signal = opimd_call_call_deleted_connect((char *) path, call_remove, no);
        no->update_signal = opimd_call_call_updated_connect((char *) path, call_update, no);
    }
}

static void new_call(gpointer data, const char* path)
{
    g_debug("New call created %s", path);
    // ottieni dettagli chiamata per controllare se e' senza risposta
    opimd_call_get_multiple_fields(path, "Path,Peer,Answered,New,Direction", call_data, NULL);
}

static void message_query(GError* error, const char* path, gpointer userdata);
static void message_data(GError* error, GHashTable* props, gpointer data);

// solo per messaggi non letti :)
static void message_next(GError* error, GHashTable* row, gpointer userdata)
{
    query_data_t* data = userdata;

    if (error) {
        g_debug("[%s] Message row error: %s", __func__, error->message);

        // distruggi query
        opimd_messagequery_dispose(data->path, NULL, NULL);

        // distruggi dati callback
        g_free(data->path);
        g_free(data);
        return;
    }

    // aggiungi! :)
    message_data(NULL, row, NULL);

    // prossimo risultato
    opimd_messagequery_get_result(data->path, message_next, data);
}

static gboolean retry_message_query(gpointer userdata)
{
    query_data_t* data = userdata;
    opimd_messages_query(data->query, message_query, data);
    return FALSE;
}

static void message_query(GError* error, const char* path, gpointer userdata)
{
    query_data_t* data = userdata;
    if (error) {
        g_debug("Message query error: (%d) %s", error->code, error->message);

        // opimd non ancora caricato? Riprova in 5 secondi
        if (FREESMARTPHONE_GLIB_IS_DBUS_ERROR(error, FREESMARTPHONE_GLIB_DBUS_ERROR_SERVICE_NOT_AVAILABLE)) {
            g_timeout_add_seconds(5, retry_message_query, data);
            return;
        }

        g_hash_table_destroy(data->query);
        g_free(data);
        return;
    }

    g_hash_table_destroy(data->query);

    data->path = g_strdup(path);
    opimd_messagequery_get_result(data->path, message_next, data);
}

static void unread_messages(GError* error, gint unread, gpointer data);

static gboolean retry_unread_messages(gpointer userdata)
{
    opimd_messages_get_unread_messages(unread_messages, userdata);
    return FALSE;
}

static void unread_messages(GError* error, gint unread, gpointer data)
{
    if (error) {
        g_debug("Unread messages error: (%d) %s", error->code, error->message);

        // opimd non ancora caricato? Riprova in 5 secondi
        if (FREESMARTPHONE_GLIB_IS_DBUS_ERROR(error, FREESMARTPHONE_GLIB_DBUS_ERROR_SERVICE_NOT_AVAILABLE))
            g_timeout_add_seconds(5, retry_unread_messages, data);

        return;
    }

    if (unread <= 0) return;

    // crea query messaggi entranti non letti
    query_data_t* cbdata = g_new0(query_data_t, 1);

    cbdata->query = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_value_free);
    cbdata->userdata = data;

    g_hash_table_insert(cbdata->query, g_strdup("MessageRead"),
        g_value_from_int(0));
    g_hash_table_insert(cbdata->query, g_strdup("Direction"),
        g_value_from_string("in"));

    opimd_messages_query(cbdata->query, message_query, cbdata);
}

static void message_update(gpointer data, GHashTable* props)
{
    g_debug("Message has been modified - checking MessageRead attribute");
    // chiamata modificata - controlla read
    if (g_hash_table_lookup(props, "MessageRead") && fso_get_attribute_bool(props, "MessageRead", TRUE)) {
        notification_signal_t* no = data;

        g_debug("MessageRead is 1 - removing message");
        mokopanel_notification_remove(current_panel, no->id);

        opimd_message_message_updated_disconnect(no->update_signal);
        opimd_message_message_deleted_disconnect(no->delete_signal);

        g_free(no);
    }
}

static void message_remove(gpointer data)
{
    notification_signal_t* no = data;

    // messaggio cancellato - rimuovi sicuramente
    g_debug("Message has been deleted - removing unread message");

    mokopanel_notification_remove(current_panel, no->id);

    opimd_message_message_updated_disconnect(no->update_signal);
    opimd_message_message_deleted_disconnect(no->delete_signal);

    g_free(no);
}

static void message_data(GError* error, GHashTable* props, gpointer data)
{
    if (error != NULL) {
        g_debug("Error getting new message data: %s", error->message);
        return;
    }

    const char* path = fso_get_attribute(props, "Path");

    // read
    gboolean is_read = (fso_get_attribute_bool(props, "MessageRead", TRUE) != 0);

    // direction (incoming)
    const char* _direction = fso_get_attribute(props, "Direction");
    gboolean incoming = !strcasecmp(_direction, "in");

    g_debug("New message: read=%d, incoming=%d",
        is_read, incoming);

    if (!is_read && incoming) {
        const char* peer = fso_get_attribute(props, "Peer");
        char* text = g_strdup_printf(_("New message from %s"), peer);
        char* subtext = g_strdup_printf("%s: %s", peer, fso_get_attribute(props, "Content"));

        int id = mokopanel_notification_queue(current_panel,
            text, "unread-message", subtext,
            MOKOPANEL_NOTIFICATION_FLAG_REPRESENT);
        g_free(text);
        g_free(subtext);

        // connetti ai cambiamenti della chiamata per la rimozione
        notification_signal_t* no = g_new(notification_signal_t, 1);
        no->id = id;
        no->delete_signal = opimd_message_message_deleted_connect((char *) path, message_remove, no);
        no->update_signal = opimd_message_message_updated_connect((char *) path, message_update, no);

    }
}

static void new_incoming_message(gpointer data, const char* path)
{
    g_debug("New incoming message %s", path);
    opimd_message_get_multiple_fields(path, "Path,Peer,Direction,MessageRead,Content", message_data, NULL);
}

static gboolean fso_connect(gpointer data)
{
    // call status per i cambiamenti delle chiamate
    ogsmd_call_call_status_connect(call_status, data);

    // TODO un giorno faremo anche questo -- ogsmd_call_list_calls(list_calls, data);

    // nuove chiamate senza risposte :)
    opimd_calls_new_call_connect(new_call, data);

    // ottieni le chiamate senza risposta attuali
    opimd_calls_get_new_missed_calls(missed_calls, data);

    // nuovi messaggi entranti
    opimd_messages_incoming_message_connect(new_incoming_message, data);

    // ottieni i messaggi entranti non letti attuali
    opimd_messages_get_unread_messages(unread_messages, data);

    return FALSE;
}

/**
 * Esegue la notifica passata.
 * @param n la notifica
 */
void notify_internal_exec(MokoNotification* n)
{
    char* cmd = NULL;

    if (!strcmp(n->type->name, TYPE_ACTIVE_CALL))
        cmd = CMD_ACTIVE_CALL;
    else if (!strcmp(n->type->name, TYPE_MISSED_CALL))
        cmd = CMD_MISSED_CALL;
    else if (!strcmp(n->type->name, TYPE_UNREAD_MESSAGE))
        cmd = CMD_UNREAD_MESSAGE;
    else if (!strcmp(n->type->name, TYPE_UNREAD_USSD))
        cmd = CMD_UNREAD_USSD;

    if (cmd) {
        // FIXME mamma mia che porcata! :S
        GError *err = NULL;

        cmd = g_strdup_printf("sh -c \"%s\"", cmd);
        g_spawn_command_line_async(cmd, &err);

        g_free(cmd);
        g_debug("Process spawned, error: %s", (err != NULL) ? err->message : "OK");

        if (err != NULL)
            g_error_free(err);
    }
}

/**
 * Inizializza le notifiche interne per un pannello.
 * TODO implementazione multi-panel
 * @param panel il pannello
 */
void notify_internal_init(MokoPanel* panel)
{
    // registra i tipi gestiti internamente
    mokopanel_register_notification_type(panel, "unread-message", MOKOSUITE_DATADIR "message-dock.png",
        _("%d unread message"), _("%d unread messages"), TRUE);
    mokopanel_register_notification_type(panel, "missed-call", MOKOSUITE_DATADIR "call-end.png",
        _("%d missed call"), _("%d missed calls"), TRUE);

    // metti i valori da recuperare al volo dall'hash table

    ogsmd_call_dbus_connect();

    if (ogsmdCallBus == NULL) {
        g_error("Cannot connect to ogsmd (call). Exiting");
        return;
    }

    odeviced_idlenotifier_dbus_connect();

    if (odevicedIdlenotifierBus == NULL) {
        g_error("Cannot connect to odeviced (idle notifier). Exiting");
        return;
    }

    current_panel = panel;

    // array chiamate
    calls = g_array_new(TRUE, TRUE, sizeof(int));

    g_idle_add(fso_connect, panel);
}
