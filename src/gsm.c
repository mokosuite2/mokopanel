/*
 * Mokosuite
 * GSM signal panel applet
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
#include <libmokosuite/mokosuite.h>
#include <libmokosuite/misc.h>
#include <freesmartphone-glib/odeviced/powersupply.h>
#include <freesmartphone-glib/ousaged/usage.h>
#include <freesmartphone-glib/ogsmd/network.h>
#include <freesmartphone-glib/ogsmd/device.h>
#include <freesmartphone-glib/ogsmd/sim.h>

#include "panel.h"
#include "gsm.h"
#include "idle.h"
#include "notifications-win.h"

#include <glib/gi18n-lib.h>

// segnale GSM
static gint8 signal_strength = 0;

// stato modem GSM
static GsmStatus gsm_status = GSM_STATUS_DISABLED;

// flag offline mode (GetFunctionality)
static gboolean offline_mode = FALSE;

// flag SIM bloccata
static gboolean sim_locked = FALSE;

static void update_operator(const char* operator)
{
    if (!operator)
        operator = _("(No service)");

    idlescreen_update_operator(operator);
    notify_window_update_operator(operator);
}

static void update_icon(Evas_Object* gsm)
{
    if (sim_locked) {
        gsm_applet_set_icon(gsm, GSM_ICON_DISABLED);
        update_operator(_("(SIM locked)"));
        return;
    }

    if (offline_mode) {
        gsm_applet_set_icon(gsm, GSM_ICON_OFFLINE);
        update_operator(NULL);
        return;
    }

    if (gsm_status == GSM_STATUS_DISABLED || gsm_status == GSM_STATUS_REGISTRATION_UNKNOWN) {
        gsm_applet_set_icon(gsm, GSM_ICON_DISABLED);
        update_operator(NULL);
        return;
    }

    if (gsm_status == GSM_STATUS_REGISTRATION_DENIED) {
        update_operator(_("(SOS only)"));
    }

    gsm_applet_set_icon(gsm, GSM_ICON_ONLINE);    
}

/* versione SEGV-safe di ogsmd_network_get_registration_status_from_dbus */
int gsm_network_get_registration_status_from_dbus(GHashTable * properties)
{
    GValue *reg = NULL;
    const char *registration = NULL;

    // ma tu guarda...
    if (properties == NULL || ((reg = g_hash_table_lookup(properties, "registration")) == NULL))
        return NETWORK_PROPERTY_REGISTRATION_UNKNOWN;

    registration = g_value_get_string(reg);
    g_debug("(reg status) registration=%s", registration);

    if (!strcmp
        (registration, "unregistered")) {
        return NETWORK_PROPERTY_REGISTRATION_UNREGISTERED;
    }
    else if (!strcmp(registration, "home")) {
        return NETWORK_PROPERTY_REGISTRATION_HOME;
    }
    else if (!strcmp(registration, "busy")) {
        return NETWORK_PROPERTY_REGISTRATION_BUSY;
    }
    else if (!strcmp
        (registration, "denied")) {
        return NETWORK_PROPERTY_REGISTRATION_DENIED;
    }
    else if (!strcmp
        (registration, "roaming")) {
        return NETWORK_PROPERTY_REGISTRATION_ROAMING;
    }

    return NETWORK_PROPERTY_REGISTRATION_UNKNOWN;
}

const char* gsm_network_get_provider_from_dbus(GHashTable * properties)
{
    GValue *provider;

    if (properties != NULL) {
        provider = g_hash_table_lookup(properties, "provider");
        return provider == NULL ? NULL : g_value_get_string(provider);
    }
    return NULL;
}

static void get_signal_strength_callback(GError *error, int strength, gpointer data)
{
    if (error) {
        g_debug("Unable to get signal strength status: %s", error->message);
        return;
    }

    Evas_Object* gsm = (Evas_Object*) data;
        g_debug("GSM signal strength: %d", strength);

    signal_strength = (error == NULL && strength >= 0) ? strength : 0;

    update_icon(gsm);
}

static void get_functionality_callback(GError* error, const char* level, gboolean autoregister, const char* pin, gpointer data)
{
    if (error) {
        g_debug("Unable to get GSM functionality: %s", error->message);
        return;
    }

    Evas_Object* gsm = (Evas_Object *) data;

    // errore - gsm disattivo
    if (error != NULL) {
        gsm_status = GSM_STATUS_DISABLED;
        update_icon(gsm);
        return;
    }

    offline_mode = strcasecmp(level, "full");

    update_icon(gsm);
}

static void get_network_status_callback(GError *error, GHashTable *status, gpointer data)
{
    if (error) {
        gsm_status = GSM_STATUS_DISABLED;
        update_icon((Evas_Object *)data);
        g_debug("Unable to get network status: %s", error->message);
        return;
    }

    int status_n = gsm_network_get_registration_status_from_dbus(status);
    const char* operator = gsm_network_get_provider_from_dbus(status);
    g_debug("GSM network status: %d", status_n);

    if (status_n == NETWORK_PROPERTY_REGISTRATION_DENIED) {
        offline_mode = FALSE;
        gsm_status = GSM_STATUS_REGISTRATION_DENIED;
    }

    else if(status_n == NETWORK_PROPERTY_REGISTRATION_HOME ||
        status_n == NETWORK_PROPERTY_REGISTRATION_ROAMING) {
        gsm_status = GSM_STATUS_REGISTRATION_HOME;
        offline_mode = FALSE;
        update_operator(operator);
    }

    else if (status_n == NETWORK_PROPERTY_REGISTRATION_UNREGISTERED ||
        status_n == NETWORK_PROPERTY_REGISTRATION_UNKNOWN) {
        // controlla offline mode
        gsm_status = GSM_STATUS_REGISTRATION_UNKNOWN;
        ogsmd_device_get_functionality(get_functionality_callback, data);
    }

    else {
        gsm_status = GSM_STATUS_REGISTRATION_UNKNOWN;
    }

    update_icon((Evas_Object *)data);
}

static void get_device_status_callback(GError *error, const int status, gpointer data)
{
    if (error) {
        g_debug("Unable to get GSM device status: %s", error->message);
        gsm_status = GSM_STATUS_DISABLED;
        update_icon((Evas_Object *)data);
        return;
    }

    g_debug("GSM Device status %d", status);

    switch (status) {
        case DEVICE_STATUS_ALIVE_SIM_LOCKED:
            sim_locked = TRUE;
            break;

        case DEVICE_STATUS_ALIVE_SIM_UNLOCKED:
        case DEVICE_STATUS_ALIVE_SIM_READY:
            sim_locked = FALSE;
            break;

        case DEVICE_STATUS_ALIVE_REGISTERED:
            // triggera registration status
            ogsmd_network_get_status(get_network_status_callback, data);
            break;

        case DEVICE_STATUS_UNKNOWN:
        case DEVICE_STATUS_CLOSED:
        case DEVICE_STATUS_INITIALIZING:
        case DEVICE_STATUS_ALIVE_NO_SIM:
        case DEVICE_STATUS_CLOSING:
            gsm_status = GSM_STATUS_DISABLED;
            break;
    }

    update_icon((Evas_Object *)data);
}

static void get_auth_status_callback(GError *error, int status, gpointer data)
{
    if (error) {
        g_debug("Unable to get GSM auth status status: %s", error->message);
        return;
    }

    g_debug("SIM auth status %d", status);

    if (status == SIM_AUTH_STATUS_READY) {
        sim_locked = FALSE;
        // probabile rientro da modalita' offline
        ogsmd_device_get_functionality(get_functionality_callback, data);
    }

    else if (status != SIM_AUTH_STATUS_UNKNOWN)
        sim_locked = TRUE;

    update_icon((Evas_Object *)data);
}

static void signal_strength_changed(gpointer data, int strength)
{
    get_signal_strength_callback(NULL, strength, data);
}

static void network_status_changed(gpointer data, GHashTable* status)
{
    get_network_status_callback(NULL, status, data);
}

static void device_status_changed(gpointer data, int status)
{
    get_device_status_callback(NULL, status, data);
}

static void auth_status_changed(gpointer data, int status)
{
    get_auth_status_callback(NULL, status, data);
}

static gboolean gsm_fso_connect(gpointer data)
{
    ogsmd_network_dbus_connect();
    if (ogsmdNetworkBus == NULL) {
        g_critical("Cannot connect to ogsmd (network)");
    }
    else {
        ogsmd_network_get_status(get_network_status_callback, data);
        ogsmd_network_get_signal_strength(get_signal_strength_callback, data);
        ogsmd_network_status_connect(network_status_changed, data);
        ogsmd_network_signal_strength_connect(signal_strength_changed, data);
    }

    ogsmd_device_dbus_connect();
    if (ogsmdDeviceBus == NULL) {
        g_critical("Cannot connect to ogsmd (device)");
    }
    else {
        ogsmd_device_get_device_status(get_device_status_callback, data);
        ogsmd_device_device_status_connect(device_status_changed, data);
    }

    ogsmd_sim_dbus_connect();
    if (ogsmdSimBus == NULL) {
        g_critical("Cannot connect to ogsmd (sim)");
    }
    else {
        ogsmd_sim_get_auth_status(get_auth_status_callback, data);
        ogsmd_sim_auth_status_connect(auth_status_changed, data);
    }

    return FALSE;
}

void gsm_applet_set_icon(Evas_Object* gsm, GsmIcon icon)
{
    char *ic = NULL;
    int perc = 0;

    switch (icon) {
        case GSM_ICON_DISABLED:
            ic = g_strdup(MOKOSUITE_DATADIR "gsm-null.png");
            break;

        case GSM_ICON_OFFLINE:
            ic = g_strdup(MOKOSUITE_DATADIR "gsm-offline.png");
            break;

        case GSM_ICON_ONLINE: //[*ti amuzzoooooooo troppoooooooooooooooooooooooooooo!!!:)!!!Bacino!!!:)!*]
            g_debug("GSM online icon, strength = %d", signal_strength);

            if (signal_strength <= 0)
                perc = 0;
            else if (signal_strength > 0 && signal_strength <= 30)
                perc = 1;
            else if (signal_strength > 30 && signal_strength <= 55)
                perc = 2;
            else if (signal_strength > 55 && signal_strength <= 80)
                perc = 3;
            else if (signal_strength > 80)
                perc = 4;

            ic = g_strdup_printf(MOKOSUITE_DATADIR "gsm-%d.png", perc);

            break;

    }

    if (ic != NULL) {
        elm_icon_file_set(gsm, ic, NULL);
        g_free(ic);

        // maledetto!!!
        elm_icon_no_scale_set(gsm, TRUE);
        #ifdef QVGA
        elm_icon_scale_set(gsm, FALSE, TRUE);
        #else
        elm_icon_scale_set(gsm, TRUE, TRUE);
        #endif
        evas_object_size_hint_align_set(gsm, 0.5, 0.5);
        evas_object_size_hint_min_set(gsm, ICON_SIZE, ICON_SIZE);
    }
}

Evas_Object* gsm_applet_new(MokoPanel* panel)
{
    Evas_Object* gsm = elm_icon_add(panel->win);

    elm_icon_file_set(gsm, MOKOSUITE_DATADIR "gsm-null.png", NULL);
    elm_icon_no_scale_set(gsm, TRUE);
    #ifdef QVGA
    elm_icon_scale_set(gsm, FALSE, TRUE);
    #else
    elm_icon_scale_set(gsm, TRUE, TRUE);
    #endif

    evas_object_size_hint_align_set(gsm, 0.5, 0.5);
    evas_object_size_hint_min_set(gsm, ICON_SIZE, ICON_SIZE);
    evas_object_show(gsm);

    // richiesta dati iniziale
    g_idle_add(gsm_fso_connect, gsm);

    return gsm;
}
