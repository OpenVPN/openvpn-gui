/*
 *  This file is a part of OpenVPN-GUI -- A Windows GUI for OpenVPN.
 *
 *  Copyright (C) 2016 Selva Nair <selva.nair@gmail.com>
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

#ifndef SECURITY_WIN32
#define SECURITY_WIN32
#endif

#include <windows.h>
#include <shellapi.h>
#include <lm.h>
#include <stdlib.h>
#include <security.h>
#include "main.h"
#include "options.h"
#include "service.h"
#include "localization.h"
#include "openvpn-gui-res.h"

extern options_t o;

#define MAX_UNAME_LEN (UNLEN + DNLEN + 2) /* UNLEN, DNLEN from lmcons.h +2 for '\' and NULL */

/*
 * Check whether current user is a member of specified groups
 */
static BOOL
CheckGroupMember(DWORD count, WCHAR *grp[])
{
    LOCALGROUP_USERS_INFO_0 *groups = NULL;
    DWORD nread, nmax, err;
    WCHAR username[MAX_UNAME_LEN];
    DWORD size;
    DWORD i, j;
    BOOL ret = FALSE;

    size = _countof (username);
    /* get username in domain\user format */
    if (!GetUserNameExW (NameSamCompatible, username, &size))
        return FALSE;
#ifdef DEBUG
    PrintDebug(L"Username: \"%s\"", username);
#endif

    /* Get an array of groups the user is member of */
    err = NetUserGetLocalGroups (NULL, username, 0, LG_INCLUDE_INDIRECT,
                    (LPBYTE *) &groups, MAX_PREFERRED_LENGTH, &nread, &nmax);
    if (err && err != ERROR_MORE_DATA)
        goto out;

    /* Check if user's groups include any of the admin groups */
    for (i = 0; i < nread; i++)
    {
#ifdef DEBUG
    PrintDebug(L"user in group %d: %s", i, groups[i].lgrui0_name);
#endif
        for (j = 0; j < count; j++)
        {
            if (wcscmp (groups[i].lgrui0_name, grp[j]) == 0)
            {
                ret = TRUE;
                break;
            }
        }
        if (ret)
            break;
    }
#ifdef DEBUG
    PrintDebug(L"User is%s in an authorized group", ret? L"" : L" not");
#endif

out:
    if (groups)
        NetApiBufferFree (groups);

    return ret;
}

/*
 * Run a command as admin using shell execute and return the exit code.
 * If the command fails to execute, the return value is (DWORD) -1.
 */
static DWORD
RunAsAdmin(const WCHAR *cmd, const WCHAR *params)
{
    SHELLEXECUTEINFO shinfo;
    DWORD status = -1;

    CLEAR (shinfo);
    shinfo.cbSize = sizeof(shinfo);
    shinfo.fMask = SEE_MASK_NOCLOSEPROCESS;
    shinfo.hwnd = NULL;
    shinfo.lpVerb = L"runas";
    shinfo.lpFile = cmd;
    shinfo.lpDirectory = NULL;
    shinfo.nShow = SW_HIDE;
    shinfo.lpParameters = params;

    if (ShellExecuteEx(&shinfo) && shinfo.hProcess)
    {
        WaitForSingleObject(shinfo.hProcess, INFINITE);
        GetExitCodeProcess(shinfo.hProcess, &status);
        CloseHandle(shinfo.hProcess);
    }
    return status;
}

/*
 * The Administrators group may be localized or renamed by admins.
 * Get the local name of the group using the SID.
 */
static BOOL
GetBuiltinAdminGroupName (WCHAR *name, DWORD nlen)
{
    BOOL b = FALSE;
    PSID admin_sid = NULL;
    DWORD sid_size = SECURITY_MAX_SID_SIZE;
    SID_NAME_USE su;

    WCHAR domain[MAX_NAME];
    DWORD dlen = _countof(domain);

    admin_sid = malloc(sid_size);
    if (!admin_sid)
        return FALSE;

    b = CreateWellKnownSid(WinBuiltinAdministratorsSid, NULL, admin_sid,
                            &sid_size);
    if(b)
    {
        b = LookupAccountSidW(NULL, admin_sid, name, &nlen, domain, &dlen, &su);
    }
#ifdef DEBUG
        PrintDebug (L"builtin admin group name = %s", name);
#endif

    free (admin_sid);

    return b;
}
/*
 * Add current user to the specified group. Uses RunAsAdmin to elevate.
 * Reject if the group name contains certain illegal characters.
 */
static BOOL
AddUserToGroup (const WCHAR *group)
{
    WCHAR username[MAX_UNAME_LEN];
    WCHAR cmd[MAX_PATH] = L"C:\\windows\\system32\\cmd.exe";
    WCHAR netcmd[MAX_PATH] = L"C:\\windows\\system32\\net.exe";
    WCHAR syspath[MAX_PATH];
    WCHAR *params = NULL;

    /* command: cmd.exe, params: /c net.exe group /add & net.exe group user /add */
    const WCHAR *fmt = L"/c %s localgroup \"%s\" /add & %s localgroup \"%s\" \"%s\" /add";
    DWORD size;
    DWORD status;
    BOOL retval = FALSE;
    WCHAR reject[] = L"\"\?\\/[]:;|=,+*<>\'&";

    /*
     * The only unknown content in the command line is the variable group. Ensure it
     * does not contain any '"' character. Here we reject all characters not allowed
     * in group names and special characters such as '&' as well.
     */
    if (wcspbrk(group, reject) != NULL)
    {
#ifdef DEBUG
        PrintDebug (L"AddUSerToGroup: illegal characters in group name: '%s'.", group);
#endif
        return retval;
    }

    size = _countof(username);
    if (!GetUserNameExW (NameSamCompatible, username, &size))
        return retval;

    size = _countof(syspath);
    if (GetSystemDirectory (syspath, size))
    {
        syspath[size-1] = L'\0';
        size = _countof(cmd);
        _snwprintf(cmd, size, L"%s\\%s", syspath, L"cmd.exe");
        cmd[size-1] = L'\0';
        size = _countof(netcmd);
        _snwprintf(netcmd, size, L"%s\\%s", syspath, L"net.exe");
        netcmd[size-1] = L'\0';
    }
    size = (wcslen(fmt) + wcslen(username) + 2*wcslen(group) + 2*wcslen(netcmd)+ 1);
    if ((params = malloc (size*sizeof(WCHAR))) == NULL)
        return retval;

    _snwprintf(params, size, fmt, netcmd, group, netcmd, group, username);
    params[size-1] = L'\0';

    status = RunAsAdmin (cmd, params);
    if (status == 0)
        retval = TRUE;

#ifdef DEBUG
    if (status == (DWORD) -1)
        PrintDebug(L"RunAsAdmin: failed to execute the command [%s %s] : error = 0x%x",
                    cmd, params, GetLastError());
    else if (status)
        PrintDebug(L"RunAsAdmin: command [%s %s] returned exit_code = %lu",
                    cmd, params, status);
#endif

    free (params);
    return retval;
}

/*
 * Check whether the config location is authorized for startup through
 * interactive service. Either the user be a member of the specified groups,
 * or the config_dir be inside the global_config_dir
 */
static BOOL
CheckConfigPath (const WCHAR *config_dir, DWORD ngrp, WCHAR *admin_group[])
{
    BOOL ret = FALSE;
    int size = wcslen(o.global_config_dir);

    /* if interactive service is not running, no access control: return TRUE */
    if (!CheckIServiceStatus(FALSE))
        ret = TRUE;

    /* if config is from the global location allow it */
    else if ( wcsncmp(config_dir, o.global_config_dir, size) == 0  &&
              wcsstr(config_dir + size, L"..") == NULL
            )
        ret = TRUE;

    /* check user is in an authorized group */
    else
    {
        if (CheckGroupMember(ngrp, admin_group))
            ret = TRUE;
    }

    return ret;
}

/*
 * If config_dir for a connection is not in an authorized location,
 * show a dialog to add the user to the ovpn_admin_group.
 */
BOOL
AuthorizeConfig(const connection_t *c)
{
    DWORD res;
    BOOL retval = FALSE;
    WCHAR *admin_group[2];
    WCHAR sysadmin_group[MAX_NAME];

    if (GetBuiltinAdminGroupName(sysadmin_group, _countof(sysadmin_group)))
        admin_group[0] = sysadmin_group;
    else
        admin_group[0] = L"Administrators";

    /* assuming TCHAR == WCHAR as we do not support non-unicode build */
    admin_group[1] = o.ovpn_admin_group;

#ifdef DEBUG
        PrintDebug (L"admin groups: %s, %s", admin_group[0], admin_group[1]);
#endif

    if (CheckConfigPath(c->config_dir, 2, admin_group))
        return TRUE;
    /* do not attempt to add user to sysadmin_group or a no-name group */
    else if ( wcscmp(sysadmin_group, o.ovpn_admin_group) == 0 ||
              wcslen(o.ovpn_admin_group) == 0                 ||
              !o.netcmd_semaphore )
    {
        ShowLocalizedMsg(IDS_ERR_CONFIG_NOT_AUTHORIZED, c->config_name, sysadmin_group);
        return FALSE;
    }

    if (WaitForSingleObject (o.netcmd_semaphore, 0) != WAIT_OBJECT_0)
    {
        /* Could not lock semaphore -- auth dialog already running? */
        ShowLocalizedMsg(IDS_NFO_CONFIG_AUTH_PENDING, c->config_name, admin_group[1]);
        return FALSE;
    }

    /* semaphore locked -- relase before return */

    res = ShowLocalizedMsgEx(MB_YESNO|MB_ICONWARNING, TEXT(PACKAGE_NAME),
                             IDS_ERR_CONFIG_TRY_AUTHORIZE, c->config_name,
                             o.ovpn_admin_group);
    if (res == IDYES)
    {
        AddUserToGroup (o.ovpn_admin_group);
        /*
         * Check the success of above by testing the config path again.
         */
        if (CheckConfigPath(c->config_dir, 2, admin_group))
            retval = TRUE;
        else
            ShowLocalizedMsg(IDS_ERR_ADD_USER_TO_ADMIN_GROUP, o.ovpn_admin_group);
        SetForegroundWindow (o.hWnd);
    }
    ReleaseSemaphore (o.netcmd_semaphore, 1, NULL);

    return retval;
}
