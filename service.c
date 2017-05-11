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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <windows.h>
#include <stdio.h>

#include "tray.h"
#include "service.h"
#include "openvpn.h"
#include "options.h"
#include "scripts.h"
#include "main.h"
#include "openvpn-gui-res.h"
#include "localization.h"

extern options_t o;

int MyStartService()
{

  SC_HANDLE schSCManager = NULL;
  SC_HANDLE schService = NULL;
  SERVICE_STATUS ssStatus; 
  DWORD dwOldCheckPoint; 
  DWORD dwStartTickCount;
  DWORD dwWaitTime;
  int i;

    /* Set Service Status = Connecting */
    o.service_state = service_connecting;
    SetServiceMenuStatus();
    CheckAndSetTrayIcon();

    // Open a handle to the SC Manager database. 
    schSCManager = OpenSCManager( 
       NULL,                    // local machine 
       NULL,                    // ServicesActive database 
       SC_MANAGER_CONNECT);     // Connect rights 
   
    if (NULL == schSCManager) {
       /* open SC manager failed */
       ShowLocalizedMsg(IDS_ERR_OPEN_SCMGR);
       goto failed;
    }

    schService = OpenService( 
        schSCManager,          // SCM database 
        _T("OpenVPNService"),  // service name
        SERVICE_START | SERVICE_QUERY_STATUS); 
 
    if (schService == NULL) {
      /* can't open VPN service */
      ShowLocalizedMsg(IDS_ERR_OPEN_VPN_SERVICE);
      goto failed;
    }
 
    /* Run Pre-connect script */
    for (i=0; i<o.num_configs; i++)
        RunPreconnectScript(&o.conn[i]);

    if (!StartService(
            schService,  // handle to service 
            0,           // number of arguments 
            NULL) )      // no arguments 
    {
      /* can't start */
      ShowLocalizedMsg(IDS_ERR_START_SERVICE);
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
      ShowLocalizedMsg(IDS_ERR_QUERY_SERVICE);
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

    if (ssStatus.dwCurrentState != SERVICE_RUNNING) 
    { 
        /* service hasn't started */
        ShowLocalizedMsg(IDS_ERR_SERVICE_START_FAILED);
        goto failed;
    } 

    /* Run Connect script */
    for (i=0; i<o.num_configs; i++)    
      RunConnectScript(&o.conn[i], true);

    /* Set Service Status = Connected */
    o.service_state = service_connected;
    SetServiceMenuStatus();
    CheckAndSetTrayIcon();

    /* Show "OpenVPN Service Started" Tray Balloon msg */
    ShowTrayBalloon(LoadLocalizedString(IDS_NFO_SERVICE_STARTED), _T(""));

    CloseServiceHandle(schService);
    CloseServiceHandle(schSCManager);
    return(true);

failed:
    if (schService)
        CloseServiceHandle(schService);
    if (schSCManager)
        CloseServiceHandle(schSCManager);
    /* Set Service Status = Disconnecting */
    o.service_state = service_disconnected;
    SetServiceMenuStatus();
    CheckAndSetTrayIcon();
    return(false);
}

int MyStopService()
{

  SC_HANDLE schSCManager = NULL;
  SC_HANDLE schService = NULL;
  SERVICE_STATUS ssStatus; 
  int i;
  BOOL ret = false;

    // Open a handle to the SC Manager database. 
    schSCManager = OpenSCManager( 
       NULL,                    // local machine 
       NULL,                    // ServicesActive database 
       SC_MANAGER_CONNECT);     // Connect rights 
   
    if (NULL == schSCManager) {
       /* can't open SCManager */
       ShowLocalizedMsg(IDS_ERR_OPEN_SCMGR, (int) GetLastError());
       return(false);
    }

    schService = OpenService( 
        schSCManager,          // SCM database 
        _T("OpenVPNService"),  // service name
        SERVICE_STOP); 
 
    if (schService == NULL) {
      /* can't open vpn service */
      ShowLocalizedMsg(IDS_ERR_OPEN_VPN_SERVICE);
      goto out;
    }

    /* Run DisConnect script */
    for (i=0; i<o.num_configs; i++)    
      RunDisconnectScript(&o.conn[i], true);

    if (!ControlService( 
            schService,   // handle to service 
            SERVICE_CONTROL_STOP,   // control value to send 
            &ssStatus) )  // address of status info 
    {
      /* stop failed */
      ShowLocalizedMsg(IDS_ERR_STOP_SERVICE);
      goto out;
    }

    o.service_state = service_disconnected;
    SetServiceMenuStatus();
    CheckAndSetTrayIcon();
    ret = true;

out:
    if (schService)
        CloseServiceHandle(schService);
    if (schSCManager)
        CloseServiceHandle(schSCManager);
    return ret;
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

bool
CheckIServiceStatus(BOOL warn)
{
    SC_HANDLE schSCManager = NULL;
    SC_HANDLE schService = NULL;
    SERVICE_STATUS ssStatus;
    BOOL ret = false;

    // Open a handle to the SC Manager database.
    schSCManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);

    if (NULL == schSCManager)
        return(false);

    schService = OpenService(schSCManager, _T("OpenVPNServiceInteractive"),
                             SERVICE_QUERY_STATUS);
    if (schService == NULL &&
        GetLastError() == ERROR_SERVICE_DOES_NOT_EXIST)
    {
        /* warn that iservice is not installed */
        if (warn)
            ShowLocalizedMsg(IDS_ERR_INSTALL_ISERVICE);
        goto out;
    }

    if (!QueryServiceStatus(schService, &ssStatus))
        goto out;

    if (ssStatus.dwCurrentState != SERVICE_RUNNING)
    {
        /* warn that iservice is not started */
        if (warn)
            ShowLocalizedMsg(IDS_ERR_NOTSTARTED_ISERVICE);
        goto out;
    }
    ret = true;

out:
    if (schService)
        CloseServiceHandle(schService);
    if (schSCManager)
        CloseServiceHandle(schSCManager);
    return ret;
}

int CheckServiceStatus()
{

  SC_HANDLE schSCManager = NULL;
  SC_HANDLE schService = NULL;
  SERVICE_STATUS ssStatus; 
  BOOL ret = false;

    // Open a handle to the SC Manager database. 
    schSCManager = OpenSCManager( 
      NULL,                    // local machine 
      NULL,                    // ServicesActive database 
      SC_MANAGER_CONNECT);     // Connect rights 
   
    if (NULL == schSCManager) {
      o.service_state = service_noaccess;
      SetServiceMenuStatus();
      goto out;
    }

    schService = OpenService( 
      schSCManager,          // SCM database 
      _T("OpenVPNService"),  // service name
      SERVICE_QUERY_STATUS); 
 
    if (schService == NULL) {
      /* open vpn service failed */
      ShowLocalizedMsg(IDS_ERR_OPEN_VPN_SERVICE);
      o.service_state = service_noaccess;
      SetServiceMenuStatus();
      goto out;
    }

    if (!QueryServiceStatus( 
            schService,   // handle to service 
            &ssStatus) )  // address of status information structure
    {
      /* query failed */
      ShowLocalizedMsg(IDS_ERR_QUERY_SERVICE);
      goto out;
    }
 
    if (ssStatus.dwCurrentState == SERVICE_RUNNING) 
    {
        o.service_state = service_connected;
        SetServiceMenuStatus(); 
        SetTrayIcon(connected);
        ret = true;
        goto out;
    }
    else 
    { 
        o.service_state = service_disconnected;
        SetServiceMenuStatus();
        SetTrayIcon(disconnected);
        goto out;
    } 

out:
    if (schService)
        CloseServiceHandle(schService);
    if (schSCManager)
        CloseServiceHandle(schSCManager);
    return ret;
} 
