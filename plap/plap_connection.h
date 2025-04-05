/*
 *  OpenVPN-PLAP-Provider
 *
 *  Copyright (C) 2017-2022 Selva Nair <selva.nair@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program (see the file COPYING included with this
 *  distribution); if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef PLAP_CONNECTION_H
#define PLAP_CONNECTION_H

#include "plap_common.h"
#include "ui_glue.h"
#include <shlwapi.h>
#include <credentialprovider.h>

/**
 * Field descriptors for the connection tiles shown to user.
 * array of {index, type, name, guid } -- guid unused here.
 */
static const CREDENTIAL_PROVIDER_FIELD_DESCRIPTOR field_desc[] = {
    { 0, CPFT_TILE_IMAGE, (wchar_t *)L"Image", { 0 } },
    { 1, CPFT_LARGE_TEXT, (wchar_t *)L"VPN Profile", { 0 } },
    { 2, CPFT_SUBMIT_BUTTON, (wchar_t *)L"Connect", { 0 } },
    { 3, CPFT_SMALL_TEXT, (wchar_t *)L"Status", { 0 } },
};

#define STATUS_FIELD_INDEX 3 /* index to the status text in the above array */

/** Field states -- show image and name always, rest when selected */
static const CREDENTIAL_PROVIDER_FIELD_STATE field_states[] = {
    CPFS_DISPLAY_IN_BOTH,          /* image */
    CPFS_DISPLAY_IN_BOTH,          /* profile name */
    CPFS_DISPLAY_IN_SELECTED_TILE, /* button */
    CPFS_DISPLAY_IN_SELECTED_TILE, /* status text */
};

/** Helper to deep copy field descriptor */
HRESULT CopyFieldDescriptor(CREDENTIAL_PROVIDER_FIELD_DESCRIPTOR *fd_out,
                            const CREDENTIAL_PROVIDER_FIELD_DESCRIPTOR *fd_in);

typedef struct OpenVPNConnection OpenVPNConnection;

HRESULT WINAPI OVPNConnection_Initialize(OpenVPNConnection *,
                                         connection_t *conn,
                                         const wchar_t *display_name);

OpenVPNConnection *OpenVPNConnection_new(void);

#endif /* PLAP_CONNECTION_H */
