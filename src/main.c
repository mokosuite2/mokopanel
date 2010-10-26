/*
 * MokoPanel
 * Entry point
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

#include <stdlib.h>
#include <mokosuite/utils/utils.h>
#include <mokosuite/utils/dbus.h>
#include <mokosuite/ui/gui.h>
#include <mokosuite/utils/remote-config-service.h>
#include <freesmartphone-glib/freesmartphone-glib.h>
#include <dbus/dbus-glib-bindings.h>

#define MOKOPANEL_NAME             "org.mokosuite.panel"
#define MOKOPANEL_CONFIG_PATH    "/org/mokosuite/Panel/Config"

#include "globals.h"
#include "panel.h"
#include "idle.h"
#include "shutdown.h"
#include "notifications-win.h"

// default log domain
int _log_dom = -1;

static MokoPanel* main_panel = NULL;

/* esportabili */
RemoteConfigService* panel_config = NULL;

#if 0
static gboolean test_after_notify(MokoPanel* panel)
{
    mokopanel_notification_queue(main_panel, "Io bene, tu?", "unread-message", "test unread message/2", MOKOPANEL_NOTIFICATION_FLAG_NONE);
    return FALSE;
}
#endif

int main(int argc, char* argv[])
{
    // TODO: initialize Intl
    setlocale(LC_ALL, "");
    bindtextdomain(PACKAGE, LOCALEDIR);
    textdomain(PACKAGE);

    // initialize log
    eina_init();
    _log_dom = eina_log_domain_register(PACKAGE, EINA_COLOR_CYAN);
    eina_log_domain_level_set(PACKAGE, LOG_LEVEL);

    EINA_LOG_INFO("%s version %s", PACKAGE_NAME, VERSION);

    /* initialize mokosuite */
    mokosuite_utils_init();
    mokosuite_ui_init(argc, argv);

    /* GLib mainloop integration */
    mokosuite_utils_glib_init(TRUE);

    /* Freesmartphone */
    freesmartphone_glib_init();

    EINA_LOG_DBG("Loading data from %s", MOKOPANEL_DATADIR);
    elm_theme_extension_add(NULL, MOKOPANEL_DATADIR "/theme.edj");

    elm_theme_overlay_add(NULL, "elm/pager/base/panel");
    elm_theme_overlay_add(NULL, "elm/label/base/panel");
    elm_theme_overlay_add(NULL, "elm/bg/base/panel");

    DBusGConnection* session_bus = dbus_session_bus();
    if (!session_bus) {
        EINA_LOG_ERR("Unable to connect to session bus. Exiting.");
        return EXIT_FAILURE;
    }

    if (!dbus_request_name(session_bus, MOKOPANEL_NAME)) {
        EINA_LOG_ERR("Unable to request name %s. Exiting.", MOKOPANEL_NAME);
        return EXIT_FAILURE;
    }

    char* cfg_file = g_strdup_printf("%s/.config/mokosuite/%s.conf", g_get_home_dir(), PACKAGE);
    panel_config = remote_config_service_new(session_bus,
        MOKOPANEL_CONFIG_PATH,
         cfg_file);
    g_free(cfg_file);

    main_panel = mokopanel_new("mokopanel", "Panel");

    // idle screen
    idlescreen_init(main_panel);

    // FIXME test icone
    #if 0
    mokopanel_notification_queue(main_panel, "Ciao zio come stai?", "unread-message", "test unread message/2",
        MOKOPANEL_NOTIFICATION_FLAG_NONE | MOKOPANEL_NOTIFICATION_FLAG_REPRESENT);
    //mokopanel_notification_queue(main_panel, "+393296061565", MOKOSUITE_DATADIR "call-end.png", NOTIFICATION_MISSED_CALL,
    //    MOKOPANEL_NOTIFICATION_FLAG_DONT_PUSH);
    //mokopanel_notification_queue(main_panel, "+39066520498", MOKOSUITE_DATADIR "call-end.png", NOTIFICATION_MISSED_CALL,
    //    MOKOPANEL_NOTIFICATION_FLAG_DONT_PUSH);
    g_timeout_add_seconds(1, (GSourceFunc) test_after_notify, main_panel);
    #endif
    // FIXME fine test icone

    elm_run();
    elm_shutdown();

    return EXIT_SUCCESS;
}
