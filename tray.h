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

#define WM_NOTIFYICONTRAY (WM_APP + 1)

#define IDM_SETTINGS            221
#define IDM_ABOUT               222
#define IDM_CLOSE               223

#define IDM_CONNECTMENU         300
#define IDM_DISCONNECTMENU      400
#define IDM_STATUSMENU          500
#define IDM_VIEWLOGMENU         600
#define IDM_EDITMENU            700
#define IDM_PASSPHRASEMENU      800

#define IDM_SERVICEMENU         900
#define IDM_SERVICE_START       901
#define IDM_SERVICE_STOP        902
#define IDM_SERVICE_RESTART     903

void CreatePopupMenus();
void OnNotifyTray(LPARAM);
void OnDestroyTray(void);
void ShowTrayIcon();
void SetTrayIcon(conn_state_t);
void SetMenuStatus(int, conn_state_t);
void SetServiceMenuStatus();
void ShowTrayBalloon(TCHAR *, TCHAR *);

#endif
