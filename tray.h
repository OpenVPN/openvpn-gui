/*
 *  OpenVPN-GUI -- A Windows GUI for OpenVPN.
 *
 *  Copyright (C) 2004 Mathias Sundman <mathias@nilings.se>
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

#define WM_NOTIFYICONTRAY (WM_APP + 1)

//Popup Menu items

#define IDM_PROXY			221						
#define IDM_ABOUT			222

#define IDM_CLOSE			223						

#define IDM_TEXT_CONN1			"Office"
#define IDM_CONN1			301
#define IDM_TEXT_CONN2			"Home"
#define IDM_CONN2			302

#define IDM_CONNECTMENU			300
#define IDM_DISCONNECTMENU		400
#define IDM_STATUSMENU			500
#define IDM_VIEWLOGMENU			600
#define IDM_EDITMENU			700
#define IDM_PASSPHRASEMENU		800

/* Service submenu */
#define IDM_SERVICEMENU			900
#define IDM_SERVICE_START		901
#define IDM_SERVICE_STOP		902
#define IDM_SERVICE_RESTART		903


void  CreatePopupMenus();			//Create popup menus 
void  DestroyPopupMenus();			//Destroy popup menus 
void  OnNotifyTray(LPARAM lParam);		//Tray message (mouse clicks on tray icon)
void  OnDestroyTray(void);			//WM_DESTROY message 
void  ShowTrayIcon();				//Put app icon in systray
void  SetTrayIcon(int connected);		//Change systray icon
BOOL  LoadAppIcon();                            //Application icon
void  CreateItemList();		                //Crate Popup menu
void  SetMenuStatus (int config, int bCheck);   //Mark connection as connected/disconnected
void  SetServiceMenuStatus();			//Diabled Service menu items.
void  ShowTrayBalloon(char *infotitle_msg, char *info_msg);

