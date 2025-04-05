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

#ifndef UI_LINK_H
#define UI_LINK_H

#ifdef __cplusplus
extern "C"
{
#endif
#include <winsock2.h> /* suppress warning about order of includes */
#include <commctrl.h>

    typedef struct connection connection_t;
    /* access to GUI states without exposing the options header */
    extern int state_connected, state_disconnected, state_onhold;

    /**
     * Initialize GUI data structures
     * @param hinstance of the calling process/dll context
     * @return 0 on success > 0 on error
     */
    DWORD InitializeUI(HINSTANCE hinstance);

    /**
     * Close threads, cleanup resources created by Initialize
     */
    void DeleteUI(void);

    /**
     * Enumerate PLAP enabled connection profiles.
     *
     * @param conn[] On output this contains an array of connection_t
     *               structs for PLAP enabled profiles
     * @param max_count size of conn[] array on input
     * @returns the number of connection profiles found.
     */
    DWORD FindPLAPConnections(connection_t *conn[], size_t max_count);

    /**
     * Display name of a connection profile.
     *
     * @param c connection_t struct for the profile (input)
     * @returns a constant wide char string
     */
    const wchar_t *ConfigDisplayName(connection_t *c);

    /**
     * State of a connection
     */
    int ConnectionState(connection_t *c);

    /**
     * Textual description of current connection state
     *
     * @param status On input must have space of at least len wide characters
     *               On output contains the NUL-terminated status text
     * @param len    Capacity of memory pointed to by status
     */
    void GetConnectionStatusText(connection_t *c, WCHAR *status, DWORD len);

    /**
     * Set hwnd as the parent window handle for dialogs
     */
    void SetParentWindow(HWND hwnd);

    void DetachAllOpenVPN();

    /**
     * Enable/Disable display of status Window for connection c
     */
    void ShowStatusWindow(connection_t *c, BOOL show);

    /**
     * Start or Release OpenVPN connection for c
     * The connection is completed asynchronously.
     */
    void ConnectHelper(connection_t *c);

    /**
     * Stop OpenVPN connection for c
     * After initiating a disconnection, this polls for the connection status
     * and waits for the disconnection to complete for a max of 5 seconds.
     */
    void DisconnectHelper(connection_t *c);

    /**
     * Set c as the currently active profile selected for user interaction
     * UI dialogs are suppressed for non-active profiles
     */
    void SetActiveProfile(connection_t *c);

    /**
     * A helper function to run a dialog showing progress of a connection process.
     *
     * @param c Connection to monitor
     * @param cb_fn A callback function that is called back every 200 msec
     * @param cb_data Data passed to the callback function
     */
    int RunProgressDialog(connection_t *c, PFTASKDIALOGCALLBACK cb_fn, LONG_PTR cb_data);

#ifdef __cplusplus
}
#endif

#endif /* UI_LINK_H */
