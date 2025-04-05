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
#include "misc.h"

extern options_t o;

#define MAX_UNAME_LEN (UNLEN + DNLEN + 2) /* UNLEN, DNLEN from lmcons.h +2 for '\' and NULL */

static BOOL GetOwnerSID(PSID sid, DWORD sid_size);

static BOOL IsUserInGroup(PSID sid, PTOKEN_GROUPS token_groups, const WCHAR *group_name);

static PTOKEN_GROUPS GetProcessTokenGroups(void);

/*
 * The Administrators group may be localized or renamed by admins.
 * Get the local name of the group using the SID.
 */
static BOOL
GetBuiltinAdminGroupName(WCHAR *name, DWORD nlen)
{
    BOOL b = FALSE;
    PSID admin_sid = NULL;
    DWORD sid_size = SECURITY_MAX_SID_SIZE;
    SID_NAME_USE su;

    WCHAR domain[MAX_NAME];
    DWORD dlen = _countof(domain);

    admin_sid = malloc(sid_size);
    if (!admin_sid)
    {
        return FALSE;
    }

    b = CreateWellKnownSid(WinBuiltinAdministratorsSid, NULL, admin_sid, &sid_size);
    if (b)
    {
        b = LookupAccountSidW(NULL, admin_sid, name, &nlen, domain, &dlen, &su);
    }
#ifdef DEBUG
    PrintDebug(L"builtin admin group name = %ls", name);
#endif

    free(admin_sid);

    return b;
}

/*
 * Add current user to the specified group. Uses RunAsAdmin to elevate.
 * Reject if the group name contains certain illegal characters.
 */
static BOOL
AddUserToGroup(const WCHAR *group)
{
    WCHAR username[MAX_UNAME_LEN];
    WCHAR cmd[MAX_PATH] = L"C:\\windows\\system32\\cmd.exe";
    WCHAR netcmd[MAX_PATH] = L"C:\\windows\\system32\\net.exe";
    WCHAR syspath[MAX_PATH];
    WCHAR *params = NULL;

    /* command: cmd.exe, params: /c net.exe group /add & net.exe group user /add */
    const WCHAR *fmt = L"/c %ls localgroup \"%ls\" /add & %ls localgroup \"%ls\" \"%ls\" /add";
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
        PrintDebug(L"AddUSerToGroup: illegal characters in group name: '%ls'.", group);
#endif
        return retval;
    }

    size = _countof(username);
    if (!GetUserNameExW(NameSamCompatible, username, &size))
    {
        return retval;
    }

    size = _countof(syspath);
    if (GetSystemDirectory(syspath, size))
    {
        syspath[size - 1] = L'\0';
        _snwprintf_s(cmd, _countof(cmd), _TRUNCATE, L"%ls\\%ls", syspath, L"cmd.exe");
        _snwprintf_s(netcmd, _countof(netcmd), _TRUNCATE, L"%ls\\%ls", syspath, L"net.exe");
    }
    size = (wcslen(fmt) + wcslen(username) + 2 * wcslen(group) + 2 * wcslen(netcmd) + 1);
    if ((params = malloc(size * sizeof(WCHAR))) == NULL)
    {
        return retval;
    }

    _snwprintf_s(params, size, _TRUNCATE, fmt, netcmd, group, netcmd, group, username);

    status = RunAsAdmin(cmd, params);
    if (status == 0)
    {
        retval = TRUE;
    }

#ifdef DEBUG
    if (status == (DWORD)-1)
    {
        PrintDebug(L"RunAsAdmin: failed to execute the command [%ls %ls] : error = 0x%x",
                   cmd,
                   params,
                   GetLastError());
    }
    else if (status)
    {
        PrintDebug(L"RunAsAdmin: command [%ls %ls] returned exit_code = %lu", cmd, params, status);
    }
#endif

    free(params);
    return retval;
}

/*
 * Check whether the config location is authorized for startup through
 * interactive service.
 */
static BOOL
CheckConfigPath(const WCHAR *config_dir)
{
    BOOL ret = FALSE;
    int size = wcslen(o.global_config_dir);

    /* if interactive service is not running, no access control: return TRUE */
    if (!CheckIServiceStatus(FALSE))
    {
        ret = TRUE;
    }
    /* if config is from the global location allow it */
    else if (wcsncmp(config_dir, o.global_config_dir, size) == 0
             && wcsstr(config_dir + size, L"..") == NULL)
    {
        ret = TRUE;
    }

    return ret;
}

/*
 * If config_dir for a connection is not in an authorized location,
 * and user is not in built-in Administrators or ovpn_admin groups
 * show a dialog to add the user to the ovpn_admin_group.
 */
BOOL
AuthorizeConfig(const connection_t *c)
{
    DWORD res;
    BOOL retval = FALSE;
    WCHAR *admin_group;
    WCHAR sysadmin_group[MAX_NAME];
    BYTE sid_buf[SECURITY_MAX_SID_SIZE];
    DWORD sid_size = SECURITY_MAX_SID_SIZE;
    PSID sid = (PSID)sid_buf;
    PTOKEN_GROUPS groups = NULL;

    if (GetBuiltinAdminGroupName(sysadmin_group, _countof(sysadmin_group)))
    {
        admin_group = sysadmin_group;
    }
    else
    {
        admin_group = L"Administrators";
    }

    PrintDebug(L"Authorized groups: '%ls', '%ls'", admin_group, o.ovpn_admin_group);

    if (CheckConfigPath(c->config_dir))
    {
        return TRUE;
    }

    if (!GetOwnerSID(sid, sid_size))
    {
        if (!o.silent_connection)
        {
            MessageBoxW(NULL, L"Failed to determine process owner SID", L"" PACKAGE_NAME, MB_OK);
        }
        return FALSE;
    }
    groups = GetProcessTokenGroups();
    if (IsUserInGroup(sid, groups, admin_group) || IsUserInGroup(sid, groups, o.ovpn_admin_group))
    {
        free(groups);
        return TRUE;
    }
    free(groups);

    /* do not attempt to add user to sysadmin_group or a no-name group */
    if (wcscmp(admin_group, o.ovpn_admin_group) == 0 || wcslen(o.ovpn_admin_group) == 0
        || !o.netcmd_semaphore)
    {
        ShowLocalizedMsg(IDS_ERR_CONFIG_NOT_AUTHORIZED, c->config_name, o.ovpn_admin_group);
        return FALSE;
    }

    if (WaitForSingleObject(o.netcmd_semaphore, 0) != WAIT_OBJECT_0)
    {
        /* Could not lock semaphore -- auth dialog already running? */
        ShowLocalizedMsg(IDS_NFO_CONFIG_AUTH_PENDING, c->config_name, o.ovpn_admin_group);
        return FALSE;
    }
    /* semaphore locked -- relase before return */
    res = ShowLocalizedMsgEx(MB_YESNO | MB_ICONWARNING,
                             NULL,
                             TEXT(PACKAGE_NAME),
                             IDS_ERR_CONFIG_TRY_AUTHORIZE,
                             c->config_name,
                             o.ovpn_admin_group);
    if (res == IDYES)
    {
        AddUserToGroup(o.ovpn_admin_group);
        /*
         * Check the success of above by testing the group membership again
         */
        if (IsUserInGroup(sid, NULL, o.ovpn_admin_group))
        {
            retval = TRUE;
        }
        else
        {
            ShowLocalizedMsg(IDS_ERR_ADD_USER_TO_ADMIN_GROUP, o.ovpn_admin_group);
        }
        SetForegroundWindow(o.hWnd);
    }
    ReleaseSemaphore(o.netcmd_semaphore, 1, NULL);

    return retval;
}

/*
 * Find SID from name
 *
 * On input sid should have space for at least sid_size bytes.
 * Returns TRUE on success, FALSE on error.
 * Hint: allocate sid to hold SECURITY_MAX_SID_SIZE bytes
 */
static BOOL
LookupSID(const WCHAR *name, PSID sid, DWORD sid_size)
{
    SID_NAME_USE su;
    WCHAR domain[MAX_NAME];
    DWORD dlen = _countof(domain);

    if (!LookupAccountName(NULL, name, sid, &sid_size, domain, &dlen, &su))
    {
        PrintDebug(L"LookupSID failed for '%ls'", name);
        return FALSE;
    }
    return TRUE;
}

/**
 * Get a list of groups in the token for the current proceess.
 * Returns a pointer to TOKEN_GROUPS structure or NULL on error.
 * The caller should free the returned pointer.
 */
static PTOKEN_GROUPS
GetProcessTokenGroups(void)
{
    HANDLE token;
    PTOKEN_GROUPS groups = NULL;
    DWORD buf_size = 0;

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token))
    {
        return NULL;
    }
    if (!GetTokenInformation(token, TokenGroups, NULL, 0, &buf_size)
        && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    {
        groups = malloc(buf_size);
    }
    if (!groups)
    {
        PrintDebug(L"GetProcessTokenGroups: error = %lu", GetLastError());
        goto out;
    }
    if (!GetTokenInformation(token, TokenGroups, groups, buf_size, &buf_size))
    {
        PrintDebug(L"Failed to get Token Group Information: error = %lu", GetLastError);
        free(groups);
        groups = NULL;
    }

out:
    CloseHandle(token);
    return groups;
}

/**
 * Check the list of token_groups include the SID of the group_name
 * OR the specified user SID is in a local group named group_name.
 * The latter check is done to recognize situations where the user is
 * added to the group dynamically through the GUI.
 *
 * Using sid and token groups instead of username avoids reference to
 * domains so that this could be completed without access to a Domain
 * Controller.
 *
 * Returns true if the user is in the group, false otherwise.
 */
static BOOL
IsUserInGroup(PSID sid, const PTOKEN_GROUPS token_groups, const WCHAR *group_name)
{
    BOOL ret = FALSE;
    DWORD_PTR resume = 0;
    DWORD err;
    BYTE grp_sid[SECURITY_MAX_SID_SIZE];
    int nloop = 0; /* a counter used to not get stuck in the do .. while() */

    /* first check in the token groups */
    if (token_groups && LookupSID(group_name, (PSID)grp_sid, _countof(grp_sid)))
    {
        for (DWORD i = 0; i < token_groups->GroupCount; ++i)
        {
            if (EqualSid((PSID)grp_sid, token_groups->Groups[i].Sid))
            {
                PrintDebug(L"Found group in token at position %lu", i);
                return TRUE;
            }
        }
    }

    if (!sid)
    {
        return FALSE;
    }

    do
    {
        DWORD nread, nmax;
        LOCALGROUP_MEMBERS_INFO_0 *members = NULL;
        err = NetLocalGroupGetMembers(
            NULL, group_name, 0, (LPBYTE *)&members, MAX_PREFERRED_LENGTH, &nread, &nmax, &resume);
        if (err != NERR_Success && err != ERROR_MORE_DATA)
        {
            break;
        }

        /* If a match is already found, ret = TRUE, the loop is skipped */
        for (DWORD i = 0; i < nread && !ret; ++i)
        {
            ret = EqualSid(members[i].lgrmi0_sid, sid);
        }
        NetApiBufferFree(members);
        /* MSDN says the lookup should always iterate until err != ERROR_MORE_DATA */
    } while (err == ERROR_MORE_DATA && nloop++ < 100);

    if (err != NERR_Success && err != NERR_GroupNotFound)
    {
        PrintDebug(L"NetLocalGroupGetMembers for group '%ls' failed: error = %lu", group_name, err);
    }
    if (ret)
    {
        PrintDebug(L"User is in group '%ls'", group_name);
    }
    return ret;
}

/**
 * Get SID of the current process owner
 * On input sid must have space for at least sid_size bytes
 *
 * On success return true, else return false.
 */
static BOOL
GetOwnerSID(PSID sid, DWORD sid_size)
{
    BOOL ret = FALSE;
    HANDLE token;
    DWORD buf_size = 0;
    TOKEN_USER *tu = NULL;

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token))
    {
        PrintDebug(L"Failed to get current process token: error = %lu", GetLastError());
        return ret;
    }
    GetTokenInformation(token, TokenUser, NULL, 0, &buf_size);
    PrintDebug(L"Needed buffer size for Token User = %lu", buf_size);

    tu = malloc(buf_size);
    if (!tu || !GetTokenInformation(token, TokenUser, tu, buf_size, &buf_size))
    {
        PrintDebug(L"Failed to get Token User Information: error = %lu", GetLastError);
        goto out;
    }
    if (!CopySid(sid_size, sid, tu->User.Sid))
    {
        PrintDebug(L"CopySid Failed: error = %lu", GetLastError());
        goto out;
    }
    ret = TRUE;

out:
    CloseHandle(token);
    free(tu);
    return ret;
}
