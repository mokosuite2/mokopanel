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

#ifndef __GSM_APPLET_H
#define __GSM_APPLET_H

#include <Evas.h>

#include "panel.h"

typedef enum {
    NETWORK_PROPERTY_REGISTRATION_UNKNOWN = 0,
    NETWORK_PROPERTY_REGISTRATION_UNREGISTERED,
    NETWORK_PROPERTY_REGISTRATION_HOME,
    NETWORK_PROPERTY_REGISTRATION_BUSY,
    NETWORK_PROPERTY_REGISTRATION_DENIED,
    NETWORK_PROPERTY_REGISTRATION_ROAMING
} NetworkStatus;

typedef enum {

    /* risorsa modem assente/disattivata, fsogsmd non in esecuzione
     * ousaged: ResourceChanged (0)
     * ogsmd: DeviceStatus (UNKNOWN, CLOSED, CLOSING, INITIALIZING, NO_SIM)
     */
    GSM_STATUS_DISABLED = 0,

    /* non registrato sulla rete
     * ogsmd: NetworkStatus (REGISTRATION_UNREGISTERED, REGISTRATION_BUSY, REGISTRATION_UNKNOWN)
     */
    GSM_STATUS_REGISTRATION_UNKNOWN,

    /* modalita' offline
     * ogsmd: NetworkStatus (REGISTRATION_UNREGISTERED) -> GetFunctionality (level=airplane)
     */
    GSM_STATUS_OFFLINE,

    /* registrato sulla rete
     * ogsmd: NetworkStatus (REGISTERED_HOME, REGISTRATION_ROAMING)
     */
    GSM_STATUS_REGISTRATION_HOME,

    /* registrazione negata (solo chiamate SOS)
     * ogsmd: NetworkStatus (REGISTRATION_DENIED)
     */
    GSM_STATUS_REGISTRATION_DENIED

} GsmStatus;

typedef enum {
    GSM_ICON_DISABLED = 0,
    GSM_ICON_OFFLINE,
    GSM_ICON_ONLINE
} GsmIcon;

void gsm_applet_set_icon(Evas_Object* gsm, GsmIcon icon);

Evas_Object* gsm_applet_new(MokoPanel* panel);

#endif  /* __GSM_APPLET_H */
