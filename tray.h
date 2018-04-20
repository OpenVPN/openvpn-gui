/*
 *  OpenVPN-GUI -- A Windows GUI for OpenVPN.
 *
 *  Copyright (C) 2004 Mathias Sundman <mathias@nilings.se>
 *                2010 Heiko Hund <heikoh@users.sf.net>
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

#ifndef TRAY_H
#define TRAY_H

#include "options.h"

#define IDM_SERVICE_START       100
#define IDM_SERVICE_STOP        101
#define IDM_SERVICE_RESTART     102

#define IDM_SETTINGS            221
#define IDM_CLOSE               223
#define IDM_IMPORT              224

#define IDM_CONNECTMENU         300
#define IDM_DISCONNECTMENU      (MAX_CONFIGS + IDM_CONNECTMENU)
#define IDM_STATUSMENU          (MAX_CONFIGS + IDM_DISCONNECTMENU)
#define IDM_VIEWLOGMENU         (MAX_CONFIGS + IDM_STATUSMENU)
#define IDM_EDITMENU            (MAX_CONFIGS + IDM_VIEWLOGMENU)
#define IDM_PASSPHRASEMENU      (MAX_CONFIGS + IDM_EDITMENU)
#define IDM_CLEARPASSMENU       (MAX_CONFIGS + IDM_PASSPHRASEMENU)
#define IDM_RECONNECTMENU       (MAX_CONFIGS + IDM_CLEARPASSMENU)

void CreatePopupMenus();
void OnNotifyTray(LPARAM);
void OnDestroyTray(void);
void ShowTrayIcon();
void SetTrayIcon(conn_state_t);
void SetMenuStatus(connection_t *, conn_state_t);
void SetMenuStatusById(int, conn_state_t);
void SetServiceMenuStatus();
void ShowTrayBalloon(TCHAR *, TCHAR *);
void CheckAndSetTrayIcon();

#endif
