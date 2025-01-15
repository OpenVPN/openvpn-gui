/*
 *  OpenVPN-GUI -- A Windows GUI for OpenVPN.
 *
 *  Copyright (C) 2004 Mathias Sundman <mathias@nilings.se>
 *                2011 Heiko Hund <heikoh@users.sf.net>
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
#include <process.h>
#include <tchar.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#include "main.h"
#include "openvpn-gui-res.h"
#include "options.h"
#include "misc.h"
#include "localization.h"
#include "env_set.h"

extern options_t o;


void
RunPreconnectScript(connection_t *c)
{
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    TCHAR cmdline[256];
    DWORD exit_code;
    struct _stat st;
    int i;

    CLEAR(si);
    CLEAR(pi);

    /* Cut off extention from config filename and add "_pre.bat" */
    int len = _tcslen(c->config_file) - _tcslen(o.ext_string) - 1;
    _sntprintf_0(cmdline, _T("%ls\\%.*ls_pre.bat"), c->config_dir, len, c->config_file);

    /* Return if no script exists */
    if (_tstat(cmdline, &st) == -1)
    {
        return;
    }

    /* Create the filename of the logfile */
    TCHAR script_log_filename[MAX_PATH];
    _sntprintf_0(script_log_filename, _T("%ls\\%ls_pre.log"), o.log_dir, c->config_name);

    /* Create the log file */
    SECURITY_ATTRIBUTES sa;
    CLEAR(sa);
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

    HANDLE logfile_handle = CreateFile(script_log_filename,
                                       GENERIC_WRITE,
                                       FILE_SHARE_READ | FILE_SHARE_WRITE,
                                       &sa,
                                       CREATE_ALWAYS,
                                       FILE_ATTRIBUTE_NORMAL,
                                       NULL);

    /* fill in STARTUPINFO struct */
    GetStartupInfo(&si);
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.wShowWindow = SW_SHOWDEFAULT;
    si.hStdInput = NULL;
    si.hStdOutput = logfile_handle;
    si.hStdError = logfile_handle;

    /* Preconnect script is run too early for config env to be available
     * so we use the default process env here.
     */

    if (!CreateProcess(NULL,
                       cmdline,
                       NULL,
                       NULL,
                       TRUE,
                       (o.show_script_window ? CREATE_NEW_CONSOLE : CREATE_NO_WINDOW),
                       NULL,
                       c->config_dir,
                       &si,
                       &pi))
    {
        goto out;
    }

    /* Wait process without blocking msg pump */
    for (i = 0; i <= (int)o.preconnectscript_timeout; i++)
    {
        if (!GetExitCodeProcess(pi.hProcess, &exit_code) || exit_code != STILL_ACTIVE
            || !OVPNMsgWait(1000, NULL))
        {
            break;
        }
    }

out:
    CloseHandleEx(&pi.hThread);
    CloseHandleEx(&pi.hProcess);
    CloseHandleEx(&logfile_handle);
}


void
RunConnectScript(connection_t *c, int run_as_service)
{
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    TCHAR cmdline[256];
    DWORD exit_code;
    struct _stat st;
    int i;

    CLEAR(si);
    CLEAR(pi);

    /* Cut off extention from config filename and add "_up.bat" */
    int len = _tcslen(c->config_file) - _tcslen(o.ext_string) - 1;
    _sntprintf_0(cmdline, _T("%ls\\%.*ls_up.bat"), c->config_dir, len, c->config_file);

    /* Return if no script exists */
    if (_tstat(cmdline, &st) == -1)
    {
        return;
    }

    if (!run_as_service)
    {
        SetDlgItemText(
            c->hwndStatus, ID_TXT_STATUS, LoadLocalizedString(IDS_NFO_STATE_CONN_SCRIPT));
    }

    /* Create the filename of the logfile */
    TCHAR script_log_filename[MAX_PATH];
    _sntprintf_0(script_log_filename, _T("%ls\\%ls_up.log"), o.log_dir, c->config_name);

    /* Create the log file */
    SECURITY_ATTRIBUTES sa;
    CLEAR(sa);
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

    HANDLE logfile_handle = CreateFile(script_log_filename,
                                       GENERIC_WRITE,
                                       FILE_SHARE_READ | FILE_SHARE_WRITE,
                                       &sa,
                                       CREATE_ALWAYS,
                                       FILE_ATTRIBUTE_NORMAL,
                                       NULL);

    /* fill in STARTUPINFO struct */
    GetStartupInfo(&si);
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.wShowWindow = SW_SHOWDEFAULT;
    si.hStdInput = NULL;
    si.hStdOutput = logfile_handle;
    si.hStdError = logfile_handle;

    /* make an env array with confg specific env appended to the process's env */
    WCHAR *env = c->es ? merge_env_block(c->es) : NULL;
    DWORD flags = CREATE_UNICODE_ENVIRONMENT;

    if (!CreateProcess(
            NULL,
            cmdline,
            NULL,
            NULL,
            TRUE,
            (o.show_script_window ? flags | CREATE_NEW_CONSOLE : flags | CREATE_NO_WINDOW),
            env,
            c->config_dir,
            &si,
            &pi))
    {
        PrintDebug(L"CreateProcess: error = %lu", GetLastError());
        ShowLocalizedMsgEx(MB_OK | MB_ICONERROR,
                           c->hwndStatus,
                           TEXT(PACKAGE_NAME),
                           IDS_ERR_RUN_CONN_SCRIPT,
                           cmdline);
        goto out;
    }

    if (o.connectscript_timeout == 0)
    {
        goto out;
    }

    for (i = 0; i <= (int)o.connectscript_timeout; i++)
    {
        if (!GetExitCodeProcess(pi.hProcess, &exit_code))
        {
            ShowLocalizedMsgEx(MB_OK | MB_ICONERROR,
                               c->hwndStatus,
                               TEXT(PACKAGE_NAME),
                               IDS_ERR_GET_EXIT_CODE,
                               cmdline);
            goto out;
        }

        if (exit_code != STILL_ACTIVE)
        {
            if (exit_code != 0)
            {
                ShowLocalizedMsgEx(MB_OK | MB_ICONERROR,
                                   c->hwndStatus,
                                   TEXT(PACKAGE_NAME),
                                   IDS_ERR_CONN_SCRIPT_FAILED,
                                   exit_code);
            }
            goto out;
        }

        if (!OVPNMsgWait(1000, c->hwndStatus)) /* WM_QUIT -- do not popup error */
        {
            goto out;
        }
    }

    ShowLocalizedMsgEx(MB_OK | MB_ICONERROR,
                       c->hwndStatus,
                       TEXT(PACKAGE_NAME),
                       IDS_ERR_RUN_CONN_SCRIPT_TIMEOUT,
                       o.connectscript_timeout);

out:
    free(env);
    CloseHandleEx(&pi.hThread);
    CloseHandleEx(&pi.hProcess);
    CloseHandleEx(&logfile_handle);
}


void
RunDisconnectScript(connection_t *c, int run_as_service)
{
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    TCHAR cmdline[256];
    DWORD exit_code;
    struct _stat st;
    int i;

    CLEAR(si);
    CLEAR(pi);

    /* Cut off extention from config filename and add "_down.bat" */
    int len = _tcslen(c->config_file) - _tcslen(o.ext_string) - 1;
    _sntprintf_0(cmdline, _T("%ls\\%.*ls_down.bat"), c->config_dir, len, c->config_file);

    /* Return if no script exists */
    if (_tstat(cmdline, &st) == -1)
    {
        return;
    }

    if (!run_as_service)
    {
        SetDlgItemText(
            c->hwndStatus, ID_TXT_STATUS, LoadLocalizedString(IDS_NFO_STATE_DISCONN_SCRIPT));
    }

    /* Create the filename of the logfile */
    TCHAR script_log_filename[MAX_PATH];
    _sntprintf_0(script_log_filename, _T("%ls\\%ls_down.log"), o.log_dir, c->config_name);

    /* Create the log file */
    SECURITY_ATTRIBUTES sa;
    CLEAR(sa);
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

    HANDLE logfile_handle = CreateFile(script_log_filename,
                                       GENERIC_WRITE,
                                       FILE_SHARE_READ | FILE_SHARE_WRITE,
                                       &sa,
                                       CREATE_ALWAYS,
                                       FILE_ATTRIBUTE_NORMAL,
                                       NULL);

    /* fill in STARTUPINFO struct */
    GetStartupInfo(&si);
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.wShowWindow = SW_SHOWDEFAULT;
    si.hStdInput = NULL;
    si.hStdOutput = logfile_handle;
    si.hStdError = logfile_handle;

    /* make an env array with confg specific env appended to the process's env */
    WCHAR *env = c->es ? merge_env_block(c->es) : NULL;
    DWORD flags = CREATE_UNICODE_ENVIRONMENT;

    if (!CreateProcess(
            NULL,
            cmdline,
            NULL,
            NULL,
            TRUE,
            (o.show_script_window ? flags | CREATE_NEW_CONSOLE : flags | CREATE_NO_WINDOW),
            NULL,
            c->config_dir,
            &si,
            &pi))
    {
        goto out;
    }

    for (i = 0; i <= (int)o.disconnectscript_timeout; i++)
    {
        if (!GetExitCodeProcess(pi.hProcess, &exit_code) || exit_code != STILL_ACTIVE
            || !OVPNMsgWait(1000, c->hwndStatus)) /* WM_QUIT -- do not popup error */
        {
            goto out;
        }
    }
out:
    free(env);
    CloseHandleEx(&pi.hThread);
    CloseHandleEx(&pi.hProcess);
    CloseHandleEx(&logfile_handle);
}
