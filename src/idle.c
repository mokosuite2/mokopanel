/*
 * Mokosuite
 * Idle screen
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

#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <linux/input.h>

#include <libmokosuite/mokosuite.h>
#include <libmokosuite/settings-service.h>

#include <freesmartphone-glib/odeviced/powersupply.h>
#include <freesmartphone-glib/odeviced/idlenotifier.h>
#include <freesmartphone-glib/odeviced/input.h>
#include <freesmartphone-glib/odeviced/display.h>
#include <freesmartphone-glib/ousaged/usage.h>

#include <Elementary.h>
#include <Ecore_X.h>
#include <Ecore_X_Atoms.h>
#include <glib.h>
#include <X11/Xlib.h>

#include "idle.h"
#include "shutdown.h"
#include "panel.h"

#define TS_DEVICE       "/dev/input/event1"

#ifdef OPENMOKO
#define SCR_DISPLAY     "localhost:0"
#else
#define SCR_DISPLAY     "neo:0"
#endif

#if 0
#define PANEL_HEIGHT        40

#define PORTRAIT_WIDTH      480
#define PORTRAIT_HEIGHT     (640 - PANEL_HEIGHT)
#define LANDSCAPE_WIDTH     640
#define LANDSCAPE_HEIGHT    (480 - PANEL_HEIGHT)
#endif

static Evas_Object* win = NULL;
static Evas_Object* idle_edje = NULL;

static MokoPanel* current_panel = NULL;

static int current_state = -1;
static int previous_state = -1;
static gint delayed_prelock_timeout = 0;

// grab touchscreen
static GPollFD ts_io = {0};
static GSource* ts_src = NULL;

static gboolean dim_on_usb = TRUE;

static int brightness = -1;

static gboolean screensaver_status = FALSE;
static gboolean ignore_busy = FALSE;

// per screensaver X
#define ALL -1
#define TIMEOUT 1
#define INTERVAL 2
#define PREFER_BLANK 3
#define ALLOW_EXP 4
#define SERVER_DEFAULT (-1)
#define DEFAULT_TIMEOUT (-600)


extern MokoSettingsService* panel_settings;

static gboolean _ts_prepare( GSource* source, gint* timeout )
{
    return FALSE;
}

static gboolean _ts_check( GSource* source )
{
    return (ts_io.revents & G_IO_IN);
}

static gboolean _ts_dispatch( GSource* source, GSourceFunc callback, gpointer data )
{
    if ( ts_io.revents & G_IO_IN ) {
        struct input_event event;
        if (!read(ts_io.fd, &event, sizeof( struct input_event )))
            return FALSE;   // eeeh???
    }

    return TRUE;
}

#if 0
static void _set_screensaver(Display *dpy, int mode)
{
    int timeout, interval, prefer_blank, allow_exp;
    g_return_if_fail(dpy != NULL);

    if (mode == ScreenSaverActive) {
        XGetScreenSaver(dpy, &timeout, &interval, &prefer_blank,
            &allow_exp);
        prefer_blank = PreferBlanking;
        XSetScreenSaver(dpy, timeout, interval, prefer_blank,
            allow_exp);
    }
    XForceScreenSaver(dpy, mode);
}
#endif

void screensaver_on(void)
{
#if 0
    Display* dpy = XOpenDisplay(SCR_DISPLAY);
    g_return_if_fail(dpy != NULL);

    _set_screensaver(dpy, ScreenSaverActive);
    XCloseDisplay(dpy);
#else
    odeviced_display_set_backlight_power(FALSE, NULL, NULL);
    screensaver_status = TRUE;
#endif
}

void screensaver_off(void)
{
#if 0
    Display* dpy = XOpenDisplay(SCR_DISPLAY);
    g_return_if_fail(dpy != NULL);

    _set_screensaver(dpy, ScreenSaverReset);
    XCloseDisplay(dpy);
#else
    odeviced_display_set_backlight_power(TRUE, NULL, NULL);
    odeviced_display_set_brightness(brightness, NULL, NULL);
    screensaver_status = FALSE;
#endif
}

#if 0
// thanks to xset
static void
set_saver(Display *dpy, int mask, int value)
{
    int timeout, interval, prefer_blank, allow_exp;

    XGetScreenSaver(dpy, &timeout, &interval, &prefer_blank, &allow_exp);
    if (mask == TIMEOUT)
    timeout = value;
    if (mask == INTERVAL)
    interval = value;
    if (mask == PREFER_BLANK)
    prefer_blank = value;
    if (mask == ALLOW_EXP)
    allow_exp = value;
    if (mask == ALL) {
    timeout = SERVER_DEFAULT;
    interval = SERVER_DEFAULT;
    prefer_blank = DefaultBlanking;
    allow_exp = DefaultExposures;
    }
    XSetScreenSaver(dpy, timeout, interval, prefer_blank, allow_exp);
    if (mask == ALL && value == DEFAULT_TIMEOUT) {
    XGetScreenSaver(dpy, &timeout, &interval, &prefer_blank, &allow_exp);
    if (!timeout)
        XSetScreenSaver(dpy, -DEFAULT_TIMEOUT, interval, prefer_blank,
                allow_exp);
    }
    return;
}

void screensaver_on(void)
{
    //system("xset -display " SCR_DISPLAY " s blank");
    //system("xset -display " SCR_DISPLAY " s activate");

    Display* dpy = XOpenDisplay(SCR_DISPLAY);
    g_return_if_fail(dpy != NULL);

    set_saver(dpy, PREFER_BLANK, PreferBlanking);
    XActivateScreenSaver(dpy);

    XCloseDisplay(dpy);
}

void screensaver_off(void)
{
    //system("xset -display " SCR_DISPLAY " s reset");

    Display* dpy = XOpenDisplay(SCR_DISPLAY);
    g_return_if_fail(dpy != NULL);

    XResetScreenSaver(dpy);
    XCloseDisplay(dpy);
}
#endif

void idle_raise(gboolean ts_grab)
{
    shutdown_window_hide();

    if (ts_src == NULL && ts_grab) {

        // grab touchscreen
        ts_io.events = G_IO_IN | G_IO_HUP | G_IO_ERR;
        ts_io.revents = 0;
        ts_io.fd = open(TS_DEVICE, O_RDONLY);
        if (ts_io.fd < 0) {
            g_critical("Error opening touchscreen device: %d", errno);
            return;
        }

        static GSourceFuncs ts_funcs = {
            _ts_prepare,
            _ts_check,
            _ts_dispatch,
            NULL,
        };
        ts_src = g_source_new(&ts_funcs, sizeof (GSource));
        g_source_add_poll(ts_src, &ts_io);
        g_source_attach(ts_src, NULL);

        // attiva EVIOCGRAB su touchscreen
        ioctl(ts_io.fd, EVIOCGRAB, 1);
    }

    // reimposta controllo prelock
    delayed_prelock_timeout = 0;

    evas_object_show(win);
    elm_win_activate(win);
    elm_win_raise(win);
    elm_object_focus(win);
}

void idle_hide(void)
{
    // disattiva EVIOCGRAB su touchscreen
    if (ts_src != NULL) {
        ioctl(ts_io.fd, EVIOCGRAB, 0);
        close(ts_io.fd);

        g_source_destroy(ts_src);
        ts_src = NULL;
    }

    if (delayed_prelock_timeout > 0)
        g_source_remove(delayed_prelock_timeout);

    // rilascia controllo prelock
    delayed_prelock_timeout = -1;

    odeviced_idlenotifier_set_state(IDLE_STATE_BUSY, NULL, NULL);
    evas_object_hide(win);
}

static void _screensaver_power_check(GError *error, int status, gpointer userdata)
{
    if (error == NULL && (status == POWER_STATUS_FULL || status == POWER_STATUS_CHARGING)) {
        g_debug("not activating screensaver due to charging or battery full");
        return;
    }

    screensaver_on();
}

static void _suspend_power_check(GError *error, int status, gpointer userdata)
{
    if (error == NULL && (status == POWER_STATUS_FULL || status == POWER_STATUS_CHARGING)) {
        g_debug("not suspending due to charging or battery full");
        return;
    }
    ousaged_usage_suspend(NULL, NULL);
}

static gboolean delayed_prelock(gpointer data)
{
    delayed_prelock_timeout = 0;
    odeviced_idlenotifier_set_state(IDLE_STATE_IDLE_PRELOCK, NULL, NULL);
    return FALSE;
}

static void idle_state(gpointer data, int state)
{
    g_debug("IDLE STATE %d", state);
    previous_state = current_state;
    current_state = state;

    // IDLE_DIM - diminuisci luminosita'
    if (state == IDLE_STATE_IDLE_DIM) {
        // TODO parametrizzare :)
        odeviced_display_set_brightness(50, NULL, NULL);
    }

    // PRELOCK - mostra idlescreen
    else if (state == IDLE_STATE_IDLE_PRELOCK) {
        idle_raise(TRUE);
    }

    // LOCK - mostra idlescreen e attiva screensaver
    else if (state == IDLE_STATE_LOCK) {

        // dim on usb disattivo, controlla power status
        if (!dim_on_usb)
            odeviced_powersupply_get_power_status(_screensaver_power_check, NULL);
        else
            screensaver_on();

        idle_raise(TRUE);
    }

    // SUSPEND - sospendi
    else if (state == IDLE_STATE_SUSPEND) {
        odeviced_powersupply_get_power_status(_suspend_power_check, NULL);

    }

    // riattivazione - disattiva screensaver e idlescreen
    else if (state == IDLE_STATE_BUSY) {
        if (previous_state < 0 ||
            previous_state == IDLE_STATE_LOCK ||
            previous_state == IDLE_STATE_SUSPEND) {
            // ripresenta le notifiche
            mokopanel_notification_represent(current_panel);

            if (!delayed_prelock_timeout)
                delayed_prelock_timeout = g_timeout_add_seconds(2, delayed_prelock, NULL);
            else if (delayed_prelock_timeout < 0)
                delayed_prelock_timeout = 0;
        }

        else {
            if (!ignore_busy)
                screensaver_off();
            else
                ignore_busy = FALSE;
        }
    }
}

static void input_event(gpointer data, const char* source, int action, int duration)
{
    g_debug("INPUT EVENT source=%s, action=%d, duration=%d", source, action, duration);

    if (!strcmp(source, "POWER")) {

        if (duration < 1 && action == INPUT_STATE_PRESSED) {
            if (screensaver_status) {
                screensaver_off();
            }

            else {
                screensaver_off();
                idle_hide();
            }
        }

        if (action == INPUT_STATE_RELEASED && duration >= 1 && duration < 3) {
            idle_raise(TRUE);
            odeviced_idlenotifier_set_state(IDLE_STATE_LOCK, NULL, NULL);
        }

        if (action == INPUT_STATE_HELD && duration == 3) {
            g_debug("Shutdown window");
            screensaver_off();
            idle_hide();
            shutdown_window_show();
        }

    }

    else if (!strcmp(source, "AUX")) {
        if (screensaver_status) {
            ignore_busy = TRUE;
        }
    }
}

static Eina_Bool screen_changed (void *data, int type, void *event_info)
{
    static Ecore_X_Randr_Orientation previous_state = -1;
    Ecore_X_Event_Screen_Change* event = event_info;

    // gia' eseguito...
    if (previous_state == event->orientation) return EINA_TRUE;

    g_debug("%s: screen has changed to %dx%d, rotation %d", __func__, event->size.width, event->size.height, event->orientation);

    previous_state = event->orientation;

    #if 0
    if (event->rotation == ECORE_X_RANDR_ROT_0 || event->rotation == ECORE_X_RANDR_ROT_180)
        evas_object_resize(win, PORTRAIT_WIDTH, PORTRAIT_HEIGHT);

    else if (event->rotation == ECORE_X_RANDR_ROT_90 || event->rotation == ECORE_X_RANDR_ROT_270)
        evas_object_resize(win, LANDSCAPE_WIDTH, LANDSCAPE_HEIGHT);
    #endif

    return EINA_TRUE;
}

static void _display_backlight(MokoSettingsService *object, const char* key, const char* value)
{
    g_debug("%s changed to: %s", key, value);

    if (odevicedDisplayBus) {
        brightness = atoi(value);
        odeviced_display_set_brightness(brightness, NULL, NULL);
    }
}

static void _display_dim_usb(MokoSettingsService *object, const char* key, const char* value)
{
    g_debug("%s changed to: %s", key, value);

    dim_on_usb = moko_settings_get_boolean(value);
}

static void init_settings()
{
    char* value = NULL;

    moko_settings_service_get_setting(panel_settings, SETTING_DISPLAY_DIM_USB, NULL, &value, NULL);
    if (value == NULL) {
        // nessuna impostazione definita, impostala
        moko_settings_service_set_setting(panel_settings, SETTING_DISPLAY_DIM_USB, moko_settings_from_boolean(TRUE), NULL);
    } else {
        dim_on_usb = moko_settings_get_boolean(value);
        g_free(value);
    }

    value = NULL;
    moko_settings_service_get_setting(panel_settings, SETTING_DISPLAY_BRIGHTNESS, NULL, &value, NULL);
    if (value == NULL) {
        // nessuna impostazione definita, impostala
        brightness = 90;
        moko_settings_service_set_setting(panel_settings, SETTING_DISPLAY_BRIGHTNESS, "90", NULL);
    } else {
        brightness = atoi(value);
        odeviced_display_set_brightness(brightness, NULL, NULL);
        g_free(value);
    }

    // stato iniziale! :)
    odeviced_idlenotifier_set_state(IDLE_STATE_IDLE_PRELOCK, NULL, NULL);
    screensaver_off();

    #if 0
    // vecchio stato iniziale! :)
    odeviced_idle_notifier_set_state(DEVICE_IDLE_STATE_BUSY, NULL, NULL);
    idle_raise(TRUE);
    screensaver_off();
    #endif
}

static gboolean idle_fso_connect(gpointer data)
{
    odeviced_idlenotifier_dbus_connect();

    if (odevicedIdlenotifierBus == NULL)
        g_critical("Cannot connect to odeviced; will not be able to receive idle state");

    odeviced_display_dbus_connect();

    if (odevicedDisplayBus == NULL)
        g_critical("Cannot connect to odeviced; will not be able to set display brightness");

    odeviced_powersupply_dbus_connect();

    if (odevicedPowersupplyBus == NULL)
        g_critical("Cannot connect to odeviced; will not be able to receive suspend power status");

    // segnali fso
    odeviced_idlenotifier_state_connect(idle_state, NULL);
    odeviced_input_event_connect(input_event, NULL);

    init_settings();

    return FALSE;
}

void idlescreen_update_operator(const char* name)
{
    edje_object_part_text_set(idle_edje, "operator", name);
    // TODO come fare per avere effetto subito?
}

void idlescreen_update_time(const char* time)
{
    edje_object_part_text_set(idle_edje, "time", time);
    // TODO come fare per avere effetto subito?
}

void idlescreen_init(MokoPanel* panel)
{
    win = elm_win_add(NULL, "mokoidle", ELM_WIN_SPLASH);
    if (!win) {
        g_error("Cannot create idle screen window. Exiting.");
        return; // mai raggiunto
    }

    elm_win_title_set(win, "Idlescreen");
    elm_win_borderless_set(win, TRUE);
    elm_win_layer_set(win, 200);    // indicator layer :)
    elm_win_sticky_set(win, TRUE);

    Ecore_X_Window_State states[3];
    states[0] = ECORE_X_WINDOW_STATE_SKIP_TASKBAR;
    states[1] = ECORE_X_WINDOW_STATE_SKIP_PAGER;
    states[2] = ECORE_X_WINDOW_STATE_ABOVE;
    ecore_x_netwm_window_state_set(elm_win_xwindow_get(win), states, 3);

    #if 0
    ecore_x_window_configure(elm_win_xwindow_get(win),
        ECORE_X_WINDOW_CONFIGURE_MASK_STACK_MODE,
        0, 0, 0, 0,
        0, 0,
        ECORE_X_WINDOW_STACK_ABOVE);
    #endif

    Evas_Object* bg = elm_bg_add(win);
    char* bg_img = config_get_string("panel", "wallpaper", MOKOSUITE_DATADIR "wallpaper_mountain.jpg");
    elm_bg_file_set(bg, bg_img, NULL);
    g_free(bg_img);

    evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    elm_win_resize_object_add(win, bg);
    evas_object_show(bg);

    Evas_Object* lo = elm_layout_add(win);
    elm_layout_file_set(lo, MOKOSUITE_DATADIR "theme.edj", "idle");
    idle_edje = elm_layout_edje_get(lo);

    evas_object_size_hint_weight_set(lo, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    elm_win_resize_object_add(win, lo);
    evas_object_show(lo);

    // callback cambiamento per le nostre impostazioni
    moko_settings_service_callback_add(panel_settings, SETTING_DISPLAY_DIM_USB, _display_dim_usb);
    moko_settings_service_callback_add(panel_settings, SETTING_DISPLAY_BRIGHTNESS, _display_backlight);

    g_idle_add(idle_fso_connect, NULL);

    // evento X rotazione schermo
    ecore_x_randr_events_select(elm_win_xwindow_get(win), TRUE);
    ecore_event_handler_add(ECORE_X_EVENT_SCREEN_CHANGE, screen_changed, NULL);

    current_panel = panel;
}
