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

    /* Cut off extention from config filename and add "_pre.bat" */
    int len = _tcslen(c->config_file) - _tcslen(o.ext_string) - 1;
    _sntprintf_0(cmdline, _T("%s\\%.*s_pre.bat"), c->config_dir, len, c->config_file);

    /* Return if no script exists */
    if (_tstat(cmdline, &st) == -1)
        return;

    CLEAR(si);
    CLEAR(pi);

    /* fill in STARTUPINFO struct */
    GetStartupInfo(&si);
    si.cb = sizeof(si);
    si.dwFlags = 0;
    si.wShowWindow = SW_SHOWDEFAULT;
    si.hStdInput = NULL;
    si.hStdOutput = NULL;

    /* Preconnect script is run too early for config env to be available
     * so we use the default process env here.
     */

    if (!CreateProcess(NULL, cmdline, NULL, NULL, TRUE,
                       (o.show_script_window ? CREATE_NEW_CONSOLE : CREATE_NO_WINDOW),
                       NULL, c->config_dir, &si, &pi))
        return;

    for (i = 0; i <= (int) o.preconnectscript_timeout; i++)
    {
        if (!GetExitCodeProcess(pi.hProcess, &exit_code))
            goto out;

        if (exit_code != STILL_ACTIVE)
            goto out;

        Sleep(1000);
    }
out:
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
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

    /* Cut off extention from config filename and add "_up.bat" */
    int len = _tcslen(c->config_file) - _tcslen(o.ext_string) - 1;
    _sntprintf_0(cmdline, _T("%s\\%.*s_up.bat"), c->config_dir, len, c->config_file);

    /* Return if no script exists */
    if (_tstat(cmdline, &st) == -1)
        return;

    if (!run_as_service)
        SetDlgItemText(c->hwndStatus, ID_TXT_STATUS, LoadLocalizedString(IDS_NFO_STATE_CONN_SCRIPT));

    CLEAR(si);
    CLEAR(pi);

    /* fill in STARTUPINFO struct */
    GetStartupInfo(&si);
    si.cb = sizeof(si);
    si.dwFlags = 0;
    si.wShowWindow = SW_SHOWDEFAULT;
    si.hStdInput = NULL;
    si.hStdOutput = NULL;

    /* make an env array with confg specific env appended to the process's env */
    WCHAR *env = c->es ? merge_env_block(c->es) : NULL;
    DWORD flags = CREATE_UNICODE_ENVIRONMENT;

    if (!CreateProcess(NULL, cmdline, NULL, NULL, TRUE,
                       (o.show_script_window ? flags|CREATE_NEW_CONSOLE : flags|CREATE_NO_WINDOW),
                       env, c->config_dir, &si, &pi))
    {
        PrintDebug(L"CreateProcess: error = %lu", GetLastError());
        ShowLocalizedMsg(IDS_ERR_RUN_CONN_SCRIPT, cmdline);
        free(env);
        return;
    }

    if (o.connectscript_timeout == 0)
        goto out;

    for (i = 0; i <= (int) o.connectscript_timeout; i++)
    {
        if (!GetExitCodeProcess(pi.hProcess, &exit_code))
        {
            ShowLocalizedMsg(IDS_ERR_GET_EXIT_CODE, cmdline);
            goto out;
        }

        if (exit_code != STILL_ACTIVE)
        {
            if (exit_code != 0)
                ShowLocalizedMsg(IDS_ERR_CONN_SCRIPT_FAILED, exit_code);
            goto out;
        }

        Sleep(1000);
    }

    ShowLocalizedMsg(IDS_ERR_RUN_CONN_SCRIPT_TIMEOUT, o.connectscript_timeout);

out:
    free(env);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
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

    /* Cut off extention from config filename and add "_down.bat" */
    int len = _tcslen(c->config_file) - _tcslen(o.ext_string) - 1;
    _sntprintf_0(cmdline, _T("%s\\%.*s_down.bat"), c->config_dir, len, c->config_file);

    /* Return if no script exists */
    if (_tstat(cmdline, &st) == -1)
        return;

    if (!run_as_service)
        SetDlgItemText(c->hwndStatus, ID_TXT_STATUS, LoadLocalizedString(IDS_NFO_STATE_DISCONN_SCRIPT));

    CLEAR(si);
    CLEAR(pi);

    /* fill in STARTUPINFO struct */
    GetStartupInfo(&si);
    si.cb = sizeof(si);
    si.dwFlags = 0;
    si.wShowWindow = SW_SHOWDEFAULT;
    si.hStdInput = NULL;
    si.hStdOutput = NULL;

    /* make an env array with confg specific env appended to the process's env */
    WCHAR *env = c->es ? merge_env_block(c->es) : NULL;
    DWORD flags = CREATE_UNICODE_ENVIRONMENT;

    if (!CreateProcess(NULL, cmdline, NULL, NULL, TRUE,
                       (o.show_script_window ? flags|CREATE_NEW_CONSOLE : flags|CREATE_NO_WINDOW),
                       NULL, c->config_dir, &si, &pi))
    {
        free(env);
        return;
    }

    for (i = 0; i <= (int) o.disconnectscript_timeout; i++)
    {
        if (!GetExitCodeProcess(pi.hProcess, &exit_code))
            goto out;

        if (exit_code != STILL_ACTIVE)
            goto out;

        Sleep(1000);
    }
out:
    free(env);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
}
