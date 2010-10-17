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

#include <mokosuite/utils/utils.h>
#include <mokosuite/utils/cfg.h>
#include <freesmartphone-glib/freesmartphone-glib.h>

#include "globals.h"

// default log domain
int _log_dom = -1;

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
    elm_init(argc, argv);

    /* other things */
    mokosuite_utils_init();
    config_init(MOKOPANEL_SYSCONFDIR "/" PACKAGE ".conf");

    freesmartphone_glib_init();

    EINA_LOG_DBG("Loading data from %s", MOKOPANEL_DATADIR);
    elm_theme_extension_add(NULL, MOKOPANEL_DATADIR "/theme.edj");

    // TODO init all the rest

    elm_run();
    elm_shutdown();

    return EXIT_SUCCESS;
}
