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

#include "service.h"
#include "options.h"
#include "main.h"
#include "misc.h"
#include "openvpn-gui-res.h"
#include "localization.h"

#define OPENVPN_SERVICE_NAME_OVPN3 L"ovpnagent"
#define OPENVPN_SERVICE_NAME_OVPN2 L"OpenVPNServiceInteractive"

extern options_t o;

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

    schService = OpenService(schSCManager, o.ovpn_engine == OPENVPN_ENGINE_OVPN3 ?
        OPENVPN_SERVICE_NAME_OVPN3 : OPENVPN_SERVICE_NAME_OVPN2, SERVICE_QUERY_STATUS);

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
        {
            if (IsUserAdmin())
                ShowLocalizedMsg(IDS_ERR_NOTSTARTED_ISERVICE_ADM);
            else
                ShowLocalizedMsg(IDS_ERR_NOTSTARTED_ISERVICE);
        }
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
      goto out;
    }

    schService = OpenService( 
      schSCManager,          // SCM database 
      _T("OpenVPNService"),  // service name
      SERVICE_QUERY_STATUS); 
 
    if (schService == NULL) {
      o.service_state = service_noaccess;
      goto out;
    }

    if (!QueryServiceStatus( 
            schService,   // handle to service 
            &ssStatus) )  // address of status information structure
    {
      /* query failed */
      o.service_state = service_noaccess;
      MsgToEventLog(EVENTLOG_ERROR_TYPE, LoadLocalizedString(IDS_ERR_QUERY_SERVICE));
      goto out;
    }
 
    if (ssStatus.dwCurrentState == SERVICE_RUNNING) 
    {
        o.service_state = service_connected;
        ret = true;
        goto out;
    }
    else 
    { 
        o.service_state = service_disconnected;
        goto out;
    } 

out:
    if (schService)
        CloseServiceHandle(schService);
    if (schSCManager)
        CloseServiceHandle(schSCManager);
    return ret;
} 
