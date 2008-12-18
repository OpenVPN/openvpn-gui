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

#include <windows.h>
#include "tray.h"
#include "service.h"
#include "openvpn.h"
#include "options.h"
#include "scripts.h"
#include "main.h"
#include <stdio.h>
#include "openvpn-gui-res.h"

extern struct options o;

int MyStartService()
{

  SC_HANDLE schSCManager;
  SC_HANDLE schService;
  SERVICE_STATUS ssStatus; 
  DWORD dwOldCheckPoint; 
  DWORD dwStartTickCount;
  DWORD dwWaitTime;
  char msg[200];
  TCHAR buf[1000];
  int i;

    /* Set Service Status = Connecting */
    o.service_running = SERVICE_CONNECTING;
    SetServiceMenuStatus();
    CheckAndSetTrayIcon();

    // Open a handle to the SC Manager database. 
    schSCManager = OpenSCManager( 
       NULL,                    // local machine 
       NULL,                    // ServicesActive database 
       SC_MANAGER_CONNECT);     // Connect rights 
   
    if (NULL == schSCManager) {
       /* open SC manager failed */
       ShowLocalizedMsg(GUI_NAME, ERR_OPEN_SCMGR, "");
       goto failed;
    }

    schService = OpenService( 
        schSCManager,          // SCM database 
        "OpenVPNService",     // service name
        SERVICE_START | SERVICE_QUERY_STATUS); 
 
    if (schService == NULL) {
      /* can't open VPN service */
      ShowLocalizedMsg(GUI_NAME, ERR_OPEN_VPN_SERVICE, "");
      goto failed;
    }
 
    /* Run Pre-connect script */
    for (i=0; i<o.num_configs; i++)    
        RunPreconnectScript(i);

    if (!StartService(
            schService,  // handle to service 
            0,           // number of arguments 
            NULL) )      // no arguments 
    {
      /* can't start */
      ShowLocalizedMsg(NULL, ERR_START_SERVICE, "");
      goto failed;
    }
    else 
    {
        //printf("Service start pending.\n"); 
    }
 
    // Check the status until the service is no longer start pending. 
    if (!QueryServiceStatus( 
            schService,   // handle to service 
            &ssStatus) )  // address of status information structure
    {
      /* query failed */
      ShowLocalizedMsg(GUI_NAME, ERR_QUERY_SERVICE, "");
      goto failed;
    }
 
    // Save the tick count and initial checkpoint.
    dwStartTickCount = GetTickCount();
    dwOldCheckPoint = ssStatus.dwCheckPoint;

    while (ssStatus.dwCurrentState == SERVICE_START_PENDING) 
    { 
        // Do not wait longer than the wait hint. A good interval is 
        // one tenth the wait hint, but no less than 1 second and no 
        // more than 5 seconds. 
 
        dwWaitTime = ssStatus.dwWaitHint / 10;

        if( dwWaitTime < 1000 )
            dwWaitTime = 1000;
        else if ( dwWaitTime > 5000 )
            dwWaitTime = 5000;

        Sleep( dwWaitTime );

        // Check the status again. 
        if (!QueryServiceStatus( 
                schService,   // handle to service 
                &ssStatus) )  // address of structure
            break; 
 
        if ( ssStatus.dwCheckPoint > dwOldCheckPoint )
        {
            // The service is making progress.
            dwStartTickCount = GetTickCount();
            dwOldCheckPoint = ssStatus.dwCheckPoint;
        }
        else
        {
            if(GetTickCount()-dwStartTickCount > ssStatus.dwWaitHint)
            {
                // No progress made within the wait hint
                break;
            }
        }
    } 

    CloseServiceHandle(schService); 
    CloseServiceHandle(schSCManager);

    if (ssStatus.dwCurrentState != SERVICE_RUNNING) 
    { 
        /* service hasn't started */
        ShowLocalizedMsg(GUI_NAME, ERR_SERVICE_START_FAILED, ""); 
        goto failed;
    } 

    /* Run Connect script */
    for (i=0; i<o.num_configs; i++)    
      RunConnectScript(i, true);

    /* Set Service Status = Connected */
    o.service_running = SERVICE_CONNECTED;
    SetServiceMenuStatus();
    CheckAndSetTrayIcon();

    /* Show "OpenVPN Service Started" Tray Balloon msg */
    myLoadString(INFO_SERVICE_STARTED);
    mysnprintf(msg," ");
    ShowTrayBalloon(buf, msg);

    return(true);

failed:

    /* Set Service Status = Disconnecting */
    o.service_running = SERVICE_DISCONNECTED;
    SetServiceMenuStatus();
    CheckAndSetTrayIcon();
    return(false);
}

int MyStopService()
{

  SC_HANDLE schSCManager;
  SC_HANDLE schService;
  SERVICE_STATUS ssStatus; 
  DWORD dwOldCheckPoint; 
  DWORD dwStartTickCount;
  DWORD dwWaitTime;
  int i;

    // Open a handle to the SC Manager database. 
    schSCManager = OpenSCManager( 
       NULL,                    // local machine 
       NULL,                    // ServicesActive database 
       SC_MANAGER_CONNECT);     // Connect rights 
   
    if (NULL == schSCManager) {
       /* can't open SCManager */
       ShowLocalizedMsg(GUI_NAME, ERR_OPEN_SCMGR, (int) GetLastError());
       return(false);
    }

    schService = OpenService( 
        schSCManager,          // SCM database 
        "OpenVPNService",     // service name
        SERVICE_STOP); 
 
    if (schService == NULL) {
      /* can't open vpn service */
      ShowLocalizedMsg(GUI_NAME, ERR_OPEN_VPN_SERVICE, "");
      return(false);
    }

    /* Run DisConnect script */
    for (i=0; i<o.num_configs; i++)    
      RunDisconnectScript(i, true);

    if (!ControlService( 
            schService,   // handle to service 
            SERVICE_CONTROL_STOP,   // control value to send 
            &ssStatus) )  // address of status info 
    {
      /* stop failed */
      ShowLocalizedMsg(GUI_NAME, ERR_STOP_SERVICE, "");
      return(false);
    }

    o.service_running = SERVICE_DISCONNECTED;
    SetServiceMenuStatus();
    CheckAndSetTrayIcon();
    return(true);
}

int MyReStartService()
{

    MyStopService();
    Sleep(1000);
    if (MyStartService()) {
       return(true);
    }

    return(false);
}


int CheckServiceStatus()
{

  SC_HANDLE schSCManager;
  SC_HANDLE schService;
  SERVICE_STATUS ssStatus; 
  DWORD dwOldCheckPoint; 
  DWORD dwStartTickCount;
  DWORD dwWaitTime;

    // Open a handle to the SC Manager database. 
    schSCManager = OpenSCManager( 
      NULL,                    // local machine 
      NULL,                    // ServicesActive database 
      SC_MANAGER_CONNECT);     // Connect rights 
   
    if (NULL == schSCManager) {
      o.service_running = SERVICE_NOACCESS;
      SetServiceMenuStatus();
      return(false);
    }

    schService = OpenService( 
      schSCManager,          // SCM database 
      "OpenVPNService",     // service name
      SERVICE_QUERY_STATUS); 
 
    if (schService == NULL) {
      /* open vpn service failed */
      ShowLocalizedMsg(GUI_NAME, ERR_OPEN_VPN_SERVICE, "");
      o.service_running = SERVICE_NOACCESS;
      SetServiceMenuStatus();
      return(false);
    }

    if (!QueryServiceStatus( 
            schService,   // handle to service 
            &ssStatus) )  // address of status information structure
    {
      /* query failed */
      ShowLocalizedMsg(GUI_NAME, ERR_QUERY_SERVICE, "");
      return(false);
    }
 
    if (ssStatus.dwCurrentState == SERVICE_RUNNING) 
    {
        o.service_running = SERVICE_CONNECTED;
        SetServiceMenuStatus(); 
        SetTrayIcon(CONNECTED);
        return(true);
    }
    else 
    { 
        o.service_running = SERVICE_DISCONNECTED;
        SetServiceMenuStatus();
        SetTrayIcon(DISCONNECTED);
        return(false);
    } 
} 
