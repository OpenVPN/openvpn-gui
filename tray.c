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
#define _WIN32_IE       0x0500
#define NIF_INFO        0x00000010
// Notify Icon Infotip flags
#define NIIF_NONE       0x00000000
// icon flags are mutually exclusive
// and take only the lowest 2 bits
#define NIIF_INFO       0x00000001
#define NIIF_WARNING    0x00000002
#define NIIF_ERROR      0x00000003
#define NIIF_ICON_MASK  0x0000000F

#include <windows.h>
#include <tchar.h>
#include <time.h>

#include "config.h"
#include "tray.h"
#include "service.h"
#include "shellapi.h"
#include "main.h"
#include "options.h"
#include "openvpn.h"
#include "openvpn_config.h"
#include "openvpn-gui-res.h"
#include "localization.h"

//POPUP MENU
HMENU hMenu;
HMENU hMenuConn[MAX_CONFIGS];
HMENU hMenuService;

NOTIFYICONDATA ni;
extern struct options o;

// Mouse clicks on tray
void OnNotifyTray(LPARAM lParam)
{
  POINT pt;				// point structure
  int connected_config;
  int i;

  // Right click, show the menu
  switch(lParam) {
    case WM_RBUTTONDOWN: 
      /* Re-read configs and re-create menus if no connection is running */
      if (CountConnectedState(DISCONNECTED) == o.num_configs)
        {
          DestroyPopupMenus();
          BuildFileList();
          CreatePopupMenus();
        }

      GetCursorPos(&pt);		// get the cursors position
      SetForegroundWindow(o.hWnd);	// set the foreground window
      TrackPopupMenu(hMenu,TPM_RIGHTALIGN,pt.x,pt.y,0,o.hWnd,NULL);// track the popup
      break;

    case WM_LBUTTONDOWN:
      break;

    case WM_LBUTTONDBLCLK:
      if (o.service_only[0]=='1')
        {
          /* Start OpenVPN Service */
          if (o.service_running == SERVICE_DISCONNECTED)
            {
              MyStartService();
            }
          else if (o.service_running == SERVICE_CONNECTED)
            {
              /* Stop OpenVPN service */
              if (MessageBox(NULL, LoadLocalizedString(IDS_MENU_ASK_STOP_SERVICE), PACKAGE_NAME, MB_YESNO | MB_SETFOREGROUND) == IDYES)
                {
                  MyStopService();
                }
            }
        }
      else
        {
          /* Open Status window if only one connection is running */
          connected_config = -1;
          for (i=0; i < o.num_configs; i++)
            {
              if(o.cnn[i].connect_status != DISCONNECTED)
                {
                  if (connected_config == -1)
                    {
                      connected_config = i;
                    }
                  else
                    {
                      connected_config = -1;
                      break;
                    }
                }
            }
          if (connected_config != -1)
            {
              ShowWindow(o.cnn[connected_config].hwndStatus, SW_SHOW); 
              SetForegroundWindow(o.cnn[connected_config].hwndStatus);
            }
 
          /* Re-read configs and re-create menus if no connection is running */
          if (CountConnectedState(DISCONNECTED) == o.num_configs)
            {
              DestroyPopupMenus();
              BuildFileList();
              CreatePopupMenus();

              /* Start connection if only one config exist */
              if ((o.num_configs == 1) && (o.cnn[0].connect_status == DISCONNECTED))
                StartOpenVPN(0);
            }
        }
      break;
  }
  PostMessage(o.hWnd,WM_NULL,0,0);// post a null message
}


void OnDestroyTray()
{
  //Destroy popup menu
  DestroyMenu(hMenu);
	
  //Remove Icon from Tray
  Shell_NotifyIcon(NIM_DELETE, &ni);
}


/* Create Popup Menus */
void CreatePopupMenus()
{
  int i;
  //extern struct options o;

  /* Create Main menu */
  hMenu=CreatePopupMenu();

  /* Create a menu for every config */
  for (i=0; i < o.num_configs; i++)
    hMenuConn[i]=CreatePopupMenu();

  /* Create the Service Menu */
  hMenuService=CreatePopupMenu();

  /* Put something on the menus */
  CreateItemList(hMenu); 
} 

/* Destroy Popup Menus */
void DestroyPopupMenus()
{
  int i;

  /* Destroy Main menu */
  DestroyMenu(hMenu);

  /* Destroy all config submenus */
  for (i=0; i < o.num_configs; i++)
    DestroyMenu(hMenuConn[i]);

  /* Destroy the Service Menu */
  DestroyMenu(hMenuService);

} 

void CreateItemList()
{
  int i;
 
  if (o.num_configs == 1)
    {
      /* Create Main menu with actions */
      if (o.service_only[0]=='0')
        {
          AppendMenu(hMenu,MF_STRING, IDM_CONNECTMENU, LoadLocalizedString(IDS_MENU_CONNECT));
          AppendMenu(hMenu,MF_STRING, IDM_DISCONNECTMENU, LoadLocalizedString(IDS_MENU_DISCONNECT));
          AppendMenu(hMenu,MF_STRING, IDM_STATUSMENU, LoadLocalizedString(IDS_MENU_STATUS));
          AppendMenu(hMenu,MF_SEPARATOR,0,0);
        }
      else
        {
          AppendMenu(hMenu,MF_STRING, IDM_SERVICE_START, LoadLocalizedString(IDS_MENU_SERVICEONLY_START));
          AppendMenu(hMenu,MF_STRING, IDM_SERVICE_STOP, LoadLocalizedString(IDS_MENU_SERVICEONLY_STOP));
          AppendMenu(hMenu,MF_STRING, IDM_SERVICE_RESTART, LoadLocalizedString(IDS_MENU_SERVICEONLY_RESTART));
          AppendMenu(hMenu,MF_SEPARATOR,0,0);
        }

      AppendMenu(hMenu,MF_STRING, IDM_VIEWLOGMENU, LoadLocalizedString(IDS_MENU_VIEWLOG));
      if (o.allow_edit[0]=='1')
        {
          AppendMenu(hMenu,MF_STRING, IDM_EDITMENU, LoadLocalizedString(IDS_MENU_EDITCONFIG));
        }
#ifndef DISABLE_CHANGE_PASSWORD
      if (o.allow_password[0]=='1')
        {
          AppendMenu(hMenu,MF_STRING, IDM_PASSPHRASEMENU, LoadLocalizedString(IDS_MENU_PASSPHRASE));
        }
#endif

      AppendMenu(hMenu,MF_SEPARATOR,0,0);
      if (o.allow_service[0]=='1' && o.service_only[0]=='0')
        {
          AppendMenu(hMenu,MF_POPUP,(UINT) hMenuService, LoadLocalizedString(IDS_MENU_SERVICE));
          AppendMenu(hMenu,MF_SEPARATOR,0,0);
        }
//TODO: if (o.allow_proxy[0]=='1' && o.service_only[0]=='0')
      AppendMenu(hMenu,MF_STRING ,IDM_SETTINGS, LoadLocalizedString(IDS_MENU_SETTINGS));
      AppendMenu(hMenu,MF_STRING ,IDM_ABOUT, LoadLocalizedString(IDS_MENU_ABOUT));
      AppendMenu(hMenu,MF_STRING ,IDM_CLOSE, LoadLocalizedString(IDS_MENU_CLOSE));

      SetMenuStatus(0, DISCONNECTED); 

    }
  else
    {
      /* Create Main menu with all connections */
      for (i=0; i < o.num_configs; i++)
        AppendMenu(hMenu,MF_POPUP,(UINT) hMenuConn[i],o.cnn[i].config_name); 
      if (o.num_configs > 0)
        AppendMenu(hMenu,MF_SEPARATOR,0,0);
      if (o.allow_service[0]=='1' && o.service_only[0]=='0')
        {
          AppendMenu(hMenu,MF_POPUP,(UINT) hMenuService, LoadLocalizedString(IDS_MENU_SERVICE));
          AppendMenu(hMenu,MF_SEPARATOR,0,0);
        }
      if (o.service_only[0]=='1')
        {
          AppendMenu(hMenu,MF_STRING, IDM_SERVICE_START, LoadLocalizedString(IDS_MENU_SERVICEONLY_START));
          AppendMenu(hMenu,MF_STRING, IDM_SERVICE_STOP, LoadLocalizedString(IDS_MENU_SERVICEONLY_STOP));
          AppendMenu(hMenu,MF_STRING, IDM_SERVICE_RESTART, LoadLocalizedString(IDS_MENU_SERVICEONLY_RESTART));
          AppendMenu(hMenu,MF_SEPARATOR,0,0);
        }
//TODO: if (o.allow_proxy[0]=='1' && o.service_only[0]=='0')
      AppendMenu(hMenu,MF_STRING ,IDM_SETTINGS, LoadLocalizedString(IDS_MENU_SETTINGS));
      AppendMenu(hMenu,MF_STRING ,IDM_ABOUT, LoadLocalizedString(IDS_MENU_ABOUT));
      AppendMenu(hMenu,MF_STRING ,IDM_CLOSE, LoadLocalizedString(IDS_MENU_CLOSE));
 

      /* Create PopUp menus for every connection */
      for (i=0; i < o.num_configs; i++)
        {
          if (o.service_only[0]=='0')
            {
              AppendMenu(hMenuConn[i],MF_STRING, (UINT_PTR)IDM_CONNECTMENU+i, LoadLocalizedString(IDS_MENU_CONNECT));
              AppendMenu(hMenuConn[i],MF_STRING, (UINT_PTR)IDM_DISCONNECTMENU+i, LoadLocalizedString(IDS_MENU_DISCONNECT));
              AppendMenu(hMenuConn[i],MF_STRING, (UINT_PTR)IDM_STATUSMENU+i, LoadLocalizedString(IDS_MENU_STATUS));
              AppendMenu(hMenuConn[i],MF_SEPARATOR,0,0);
            }
          AppendMenu(hMenuConn[i], MF_STRING, (UINT_PTR)IDM_VIEWLOGMENU+i, LoadLocalizedString(IDS_MENU_VIEWLOG));
          if (o.allow_edit[0]=='1') {
            AppendMenu(hMenuConn[i], MF_STRING, (UINT_PTR)IDM_EDITMENU+i, LoadLocalizedString(IDS_MENU_EDITCONFIG));
          }
#ifndef DISABLE_CHANGE_PASSWORD
          if (o.allow_password[0]=='1')
            {
              AppendMenu(hMenuConn[i], MF_STRING, (UINT_PTR)IDM_PASSPHRASEMENU+i, LoadLocalizedString(IDS_MENU_PASSPHRASE));
            }
#endif

          SetMenuStatus(i, DISCONNECTED); 
        }
    }

  /* Create Service menu */
  if (o.allow_service[0]=='1' && o.service_only[0]=='0')
    {
      AppendMenu(hMenuService,MF_STRING, IDM_SERVICE_START, LoadLocalizedString(IDS_MENU_SERVICE_START));
      AppendMenu(hMenuService,MF_STRING, IDM_SERVICE_STOP, LoadLocalizedString(IDS_MENU_SERVICE_STOP));
      AppendMenu(hMenuService,MF_STRING, IDM_SERVICE_RESTART, LoadLocalizedString(IDS_MENU_SERVICE_RESTART));
    }

    SetServiceMenuStatus();
}


BOOL LoadAppIcon()
{

  // Load icon from resource
  HICON hIcon = LoadLocalizedIcon(ID_ICO_APP);
  if (hIcon) {
    SendMessage(o.hWnd, WM_SETICON, (WPARAM) (ICON_SMALL), (LPARAM) (hIcon));
    SendMessage(o.hWnd, WM_SETICON, (WPARAM) (ICON_BIG), (LPARAM) (hIcon));  //ALT+TAB icon
    return TRUE;
  }
  return FALSE;
}


void ShowTrayIcon()
{
  ni.cbSize = sizeof(ni);
  ni.uID = 0;
  _tcsncpy(ni.szTip, LoadLocalizedString(IDS_TIP_DEFAULT), _tsizeof(ni.szTip)); 
  ni.hWnd = o.hWnd;
  ni.uFlags = NIF_MESSAGE | NIF_TIP | NIF_ICON; // We want to use icon, tip, and callback message
  ni.uCallbackMessage = WM_NOTIFYICONTRAY;      // Our custom callback message (WM_APP + 1)
    
     
  //Load selected icon
  ni.hIcon = LoadLocalizedIcon(ID_ICO_DISCONNECTED);

  Shell_NotifyIcon(NIM_ADD, &ni);       

}

/* SetTrayIcon(int connected)
 * connected=0 -> DisConnected
 * connected=1 -> Connecting
 * connected=2 -> Connected
 */
void SetTrayIcon(int connected)
{
  TCHAR msg[500];
  TCHAR msg_connected[100];
  TCHAR msg_connecting[100];
  TCHAR connected_since[50];
  int i, first_conn;
  int config=0;

  ni.cbSize = sizeof(ni);
  ni.uID = 0;
  ni.hWnd = o.hWnd;
  ni.uFlags = NIF_MESSAGE | NIF_TIP | NIF_ICON; // We want to use icon, tip, and callback message
  ni.uCallbackMessage = WM_NOTIFYICONTRAY;      // Our custom callback message (WM_APP + 1)
   
  _tcsncpy(msg, LoadLocalizedString(IDS_TIP_DEFAULT), _tsizeof(ni.szTip));
  _tcsncpy(msg_connected, LoadLocalizedString(IDS_TIP_CONNECTED), _tsizeof(msg_connected));
  _tcsncpy(msg_connecting, LoadLocalizedString(IDS_TIP_CONNECTING), _tsizeof(msg_connecting));

  first_conn=1;
  for (i=0; i < o.num_configs; i++)
    {
      if(o.cnn[i].connect_status == CONNECTED)
        {
          /* Append connection name to Icon Tip Msg */
          if (first_conn)
            _tcsncat(msg, msg_connected, _tsizeof(msg) - _tcslen(msg) - 1);
          else
            _tcsncat(msg, ", ", _tsizeof(msg) - _tcslen(msg) - 1);
          _tcsncat(msg, o.cnn[i].config_name, _tsizeof(msg) - _tcslen(msg) - 1);
          first_conn=0;
          config=i;
        }
    }

  first_conn=1;
  for (i=0; i < o.num_configs; i++)
    {
      if((o.cnn[i].connect_status == CONNECTING) ||
         (o.cnn[i].connect_status == RECONNECTING))
        {
          /* Append connection name to Icon Tip Msg */
          if (first_conn)
            _tcsncat(msg, msg_connecting, _tsizeof(msg) - _tcslen(msg) - 1);
          else
            _tcsncat(msg, ", ", _tsizeof(msg) - _tcslen(msg) - 1);
          _tcsncat(msg, o.cnn[i].config_name, _tsizeof(msg) - _tcslen(msg) - 1);
          first_conn=0;
        }
    }

  if (CountConnectedState(CONNECTED) == 1)
    {
      /* Append "Connected Since and Assigned IP msg" */
      time_t con_time;
      con_time=time(NULL);
      _tcsftime(connected_since, _tsizeof(connected_since), _T("%b %d, %H:%M"), 
                localtime(&o.cnn[config].connected_since));
      _tcsncat(msg, LoadLocalizedString(IDS_TIP_CONNECTED_SINCE), _tsizeof(msg) - _tcslen(msg) - 1);
      _tcsncat(msg, connected_since, _tsizeof(msg) - _tcslen(msg) - 1);
      if (_tcslen(o.cnn[config].ip) > 0)
        {
          TCHAR assigned_ip[100];
          _sntprintf_0(assigned_ip, LoadLocalizedString(IDS_TIP_ASSIGNED_IP), o.cnn[config].ip);
          _tcsncat(msg, assigned_ip, _tsizeof(msg) - _tcslen(msg) - 1);
        }
    }

  _tcsncpy(ni.szTip, msg, _tsizeof(ni.szTip));
     
  //Load selected icon
  if (connected==2) 
    ni.hIcon = LoadLocalizedIcon(ID_ICO_CONNECTED);
  else if (connected==1)
    ni.hIcon = LoadLocalizedIcon(ID_ICO_CONNECTING); 
  else if (connected==0)
    ni.hIcon = LoadLocalizedIcon(ID_ICO_DISCONNECTED);

  Shell_NotifyIcon(NIM_MODIFY, &ni);       
}

void ShowTrayBalloon(TCHAR *infotitle_msg, TCHAR *info_msg)
{
  ni.cbSize = sizeof(ni);
  ni.uID = 0;
  ni.hWnd = o.hWnd;
  ni.uFlags = NIF_INFO; 			/* We want to show a balloon */
  ni.uTimeout = 5000;
  ni.dwInfoFlags = NIIF_INFO;			/* Show an Info Icon */
  _tcsncpy(ni.szInfo, info_msg, _tsizeof(ni.szInfo)); 
  _tcsncpy(ni.szInfoTitle, infotitle_msg, _tsizeof(ni.szInfoTitle)); 

  Shell_NotifyIcon(NIM_MODIFY, &ni);       
}


void SetMenuStatus (int config, int bCheck)
{
  /* bCheck values:
   * 0 - Not Connected
   * 1 - Connecting
   * 2 - Connected
   * 4 - Disconnecting
   */


  unsigned int iState;

  if (o.num_configs == 1)
    {
      if (bCheck == DISCONNECTED)
        {
          EnableMenuItem(hMenu, IDM_CONNECTMENU, MF_ENABLED);
          EnableMenuItem(hMenu, IDM_DISCONNECTMENU, MF_GRAYED);
          EnableMenuItem(hMenu, IDM_STATUSMENU, MF_GRAYED);
        }
      if (bCheck == CONNECTING)
        {
          EnableMenuItem(hMenu, IDM_CONNECTMENU, MF_GRAYED);
          EnableMenuItem(hMenu, IDM_DISCONNECTMENU, MF_ENABLED);
          EnableMenuItem(hMenu, IDM_STATUSMENU, MF_ENABLED);
        }
      if (bCheck == CONNECTED)
        {
          EnableMenuItem(hMenu, IDM_CONNECTMENU, MF_GRAYED);
          EnableMenuItem(hMenu, IDM_DISCONNECTMENU, MF_ENABLED);
          EnableMenuItem(hMenu, IDM_STATUSMENU, MF_ENABLED);
        }
      if (bCheck == DISCONNECTING)
        {
          EnableMenuItem(hMenu, IDM_CONNECTMENU, MF_GRAYED);
          EnableMenuItem(hMenu, IDM_DISCONNECTMENU, MF_GRAYED);
          EnableMenuItem(hMenu, IDM_STATUSMENU, MF_ENABLED);
        }
    }
  else 
    {
      iState = ((bCheck == CONNECTED) || (bCheck == DISCONNECTING)) ? 
               MF_CHECKED : MF_UNCHECKED ;
      CheckMenuItem (hMenu, (UINT) hMenuConn[config], iState) ;

      if (bCheck == DISCONNECTED)
        {
          EnableMenuItem(hMenuConn[config], (UINT_PTR)IDM_CONNECTMENU + config, MF_ENABLED);
          EnableMenuItem(hMenuConn[config], (UINT_PTR)IDM_DISCONNECTMENU + config, MF_GRAYED);
          EnableMenuItem(hMenuConn[config], (UINT_PTR)IDM_STATUSMENU + config, MF_GRAYED);
        }
      if (bCheck == CONNECTING)
        {
          EnableMenuItem(hMenuConn[config], (UINT_PTR)IDM_CONNECTMENU + config, MF_GRAYED);
          EnableMenuItem(hMenuConn[config], (UINT_PTR)IDM_DISCONNECTMENU + config, MF_ENABLED);
          EnableMenuItem(hMenuConn[config], (UINT_PTR)IDM_STATUSMENU + config, MF_ENABLED);
        }
      if (bCheck == CONNECTED)
        {
          EnableMenuItem(hMenuConn[config], (UINT_PTR)IDM_CONNECTMENU + config, MF_GRAYED);
          EnableMenuItem(hMenuConn[config], (UINT_PTR)IDM_DISCONNECTMENU + config, MF_ENABLED);
          EnableMenuItem(hMenuConn[config], (UINT_PTR)IDM_STATUSMENU + config, MF_ENABLED);
        }
      if (bCheck == DISCONNECTING)
        {
          EnableMenuItem(hMenuConn[config], (UINT_PTR)IDM_CONNECTMENU + config, MF_GRAYED);
          EnableMenuItem(hMenuConn[config], (UINT_PTR)IDM_DISCONNECTMENU + config, MF_GRAYED);
          EnableMenuItem(hMenuConn[config], (UINT_PTR)IDM_STATUSMENU + config, MF_ENABLED);
        }
    }

}

void SetServiceMenuStatus()
{
  HMENU hMenuHandle;

  if (o.allow_service[0]=='0' && o.service_only[0]=='0')
    return;

  if (o.service_only[0]=='1')
    hMenuHandle = hMenu;
  else
    hMenuHandle = hMenuService;


  if ((o.service_running == SERVICE_NOACCESS) ||
      (o.service_running == SERVICE_CONNECTING))
    {
      /* Service is disabled */
      EnableMenuItem(hMenuHandle, IDM_SERVICE_START, MF_GRAYED);
      EnableMenuItem(hMenuHandle, IDM_SERVICE_STOP, MF_GRAYED);
      EnableMenuItem(hMenuHandle, IDM_SERVICE_RESTART, MF_GRAYED);
    }
  else if (o.service_running == SERVICE_CONNECTED)
    {
      /* Service is running */
      EnableMenuItem(hMenuHandle, IDM_SERVICE_START, MF_GRAYED);
      EnableMenuItem(hMenuHandle, IDM_SERVICE_STOP, MF_ENABLED);
      EnableMenuItem(hMenuHandle, IDM_SERVICE_RESTART, MF_ENABLED);
    }
  else
    {
      /* Service is not running */
      EnableMenuItem(hMenuHandle, IDM_SERVICE_START, MF_ENABLED);
      EnableMenuItem(hMenuHandle, IDM_SERVICE_STOP, MF_GRAYED);
      EnableMenuItem(hMenuHandle, IDM_SERVICE_RESTART, MF_GRAYED);
    }

}


