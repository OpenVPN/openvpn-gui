/*
 *  OpenVPN-GUI -- A Windows GUI for OpenVPN.
 *
 *  Copyright (C) 2004 Mathias Sundman <mathias@nilings.se>
 *                2010 Heiko Hund <heikoh@users.sf.net>
 *                2016 Selva Nair <selva.nair@gmail.com>
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
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <windows.h>
#include <windowsx.h>
#include <prsht.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <shlobj.h>
#include <shlwapi.h>

#include "options.h"
#include "main.h"
#include "openvpn-gui-res.h"
#include "localization.h"
#include "misc.h"
#include "registry.h"
#include "save_pass.h"

#define streq(x, y) (_tcscmp((x), (y)) == 0)

extern options_t o;

static version_t
MakeVersion (short ma, short mi, short b, short r)
{
    version_t v = {ma, mi, b, r};
    return v;
}

static void
ExpandString (WCHAR *str, int max_len)
{
  WCHAR expanded_string[MAX_PATH];
  int len = ExpandEnvironmentStringsW (str, expanded_string, _countof(expanded_string));

  if (len > max_len || len > (int) _countof(expanded_string))
  {
      PrintDebug (L"Failed to expanded env vars in '%s'. String too long", str);
      return;
  }
  wcsncpy (str, expanded_string, max_len);
}

void
ExpandOptions (void)
{
    ExpandString (o.exe_path, _countof(o.exe_path));
    ExpandString (o.config_dir, _countof(o.config_dir));
    ExpandString (o.log_dir, _countof(o.log_dir));
    ExpandString (o.editor, _countof(o.editor));
    ExpandString (o.log_viewer, _countof(o.log_viewer));
}

static int
add_option(options_t *options, int i, TCHAR **p)
{
    if (streq(p[0], _T("help")))
    {
        TCHAR caption[200];
        TCHAR msg[USAGE_BUF_SIZE];
        /* We only print help and exit: release the semapahore to allow another instance */
        CloseSemaphore(o.session_semaphore); /* OK to call even if semaphore is NULL */
        o.session_semaphore = NULL;

        LoadLocalizedStringBuf(caption, _countof(caption), IDS_NFO_USAGECAPTION);
        LoadLocalizedStringBuf(msg, _countof(msg), IDS_NFO_USAGE);
        MessageBoxEx(NULL, msg, caption, MB_OK | MB_SETFOREGROUND, GetGUILanguage());
        exit(0);
    }
    else if (streq(p[0], _T("connect")) && p[1])
    {
        ++i;
        if (!options->auto_connect || options->num_auto_connect == options->max_auto_connect)
        {
            options->max_auto_connect += 10;
            void *tmp = realloc(options->auto_connect, sizeof(wchar_t *)*options->max_auto_connect);
            if (!tmp)
            {
                options->max_auto_connect -= 10;
                ErrorExit(1, L"Out of memory while parsing command line");
            }
            options->auto_connect = tmp;
        }
        options->auto_connect[options->num_auto_connect++] = p[1];
        /* Treat the first connect option to also mean --command connect profile.
         * This gets used if we are not the first instance.
         */
        if (options->num_auto_connect == 1)
        {
            options->action = WM_OVPN_START;
            options->action_arg = p[1];
        }
    }
    else if (streq(p[0], _T("exe_path")) && p[1])
    {
        ++i;
        _tcsncpy(options->exe_path, p[1], _countof(options->exe_path) - 1);
    }
    else if (streq(p[0], _T("config_dir")) && p[1])
    {
        ++i;
        _tcsncpy(options->config_dir, p[1], _countof(options->config_dir) - 1);
    }
    else if (streq(p[0], _T("ext_string")) && p[1])
    {
        ++i;
        _tcsncpy(options->ext_string, p[1], _countof(options->ext_string) - 1);
    }
    else if (streq(p[0], _T("log_dir")) && p[1])
    {
        ++i;
        _tcsncpy(options->log_dir, p[1], _countof(options->log_dir) - 1);
    }
    else if (streq(p[0], _T("priority_string")) && p[1])
    {
        ++i;
        _tcsncpy(options->priority_string, p[1], _countof(options->priority_string) - 1);
    }
    else if ( (streq(p[0], _T("append_string")) ||
             streq(p[0], _T("log_append"))) && p[1] )
    {
        ++i;
        options->log_append = _ttoi(p[1]) ? 1 : 0;
    }
    else if (streq(p[0], _T("log_viewer")) && p[1])
    {
        ++i;
        _tcsncpy(options->log_viewer, p[1], _countof(options->log_viewer) - 1);
    }
    else if (streq(p[0], _T("editor")) && p[1])
    {
        ++i;
        _tcsncpy(options->editor, p[1], _countof(options->editor) - 1);
    }
    else if (streq(p[0], _T("allow_edit")) && p[1])
    {
        ++i;
        PrintDebug (L"Deprecated option: '%s' ignored.", p[0]);
    }
    else if (streq(p[0], _T("allow_service")) && p[1])
    {
        ++i;
        PrintDebug (L"Deprecated option: '%s' ignored.", p[0]);
    }
    else if (streq(p[0], _T("allow_password")) && p[1])
    {
        ++i;
        PrintDebug (L"Deprecated option: '%s' ignored.", p[0]);
    }
    else if (streq(p[0], _T("allow_proxy")) && p[1])
    {
        ++i;
        PrintDebug (L"Deprecated option: '%s' ignored.", p[0]);
    }
    else if (streq(p[0], _T("show_balloon")) && p[1])
    {
        ++i;
        options->show_balloon = _ttoi(p[1]);
    }
    else if (streq(p[0], _T("service_only")) && p[1])
    {
        ++i;
        options->service_only = _ttoi(p[1]) ? 1 : 0;
    }
    else if (streq(p[0], _T("show_script_window")) && p[1])
    {
        ++i;
        options->show_script_window = _ttoi(p[1]) ? 1 : 0;
    }
    else if (streq(p[0], _T("silent_connection")) && p[1])
    {
        ++i;
        options->silent_connection = _ttoi(p[1]) ? 1 : 0;
    }
    else if (streq(p[0], _T("passphrase_attempts")) && p[1])
    {
        ++i;
        PrintDebug (L"Deprecated option: '%s' ignored.", p[0]);
    }
    else if (streq(p[0], _T("connectscript_timeout")) && p[1])
    {
        ++i;
        options->connectscript_timeout = _ttoi(p[1]);
    }
    else if (streq(p[0], _T("disconnectscript_timeout")) && p[1])
    {
        ++i;
        options->disconnectscript_timeout = _ttoi(p[1]);
    }
    else if (streq(p[0], _T("preconnectscript_timeout")) && p[1])
    {
        ++i;
        options->preconnectscript_timeout = _ttoi(p[1]);
    }
    else if (streq(p[0], _T("config_menu_view")) && p[1])
    {
        ++i;
        options->config_menu_view = _ttoi(p[1]);
    }
    else if (streq(p[0], _T("command")) && p[1])
    {
        ++i;
        /* command to be sent to a running instance */
        if (streq(p[1], _T("connect")) && p[2])
        {
            /* Treat this as "--connect profile" in case this is the first instance */
            add_option(options, i, &p[1]);
            ++i;
            options->action = WM_OVPN_START;
            options->action_arg = p[2];
        }
        else if (streq(p[1], _T("disconnect")) && p[2])
        {
            ++i;
            options->action = WM_OVPN_STOP;
            options->action_arg = p[2];
        }
        else if (streq(p[1], _T("reconnect")) && p[2])
        {
            ++i;
            options->action = WM_OVPN_RESTART;
            options->action_arg = p[2];
        }
        else if (streq(p[1], _T("status")) && p[2])
        {
            ++i;
            options->action = WM_OVPN_SHOWSTATUS;
            options->action_arg = p[2];
        }
        else if (streq(p[1], _T("silent_connection")))
        {
            ++i;
            options->action = WM_OVPN_SILENT;
            options->action_arg = p[2] ? p[2] : _T("1");
        }
        else if (streq(p[1], _T("disconnect_all")))
        {
            options->action = WM_OVPN_STOPALL;
        }
        else if (streq(p[1], _T("exit")))
        {
            options->action = WM_OVPN_EXIT;
        }
        else
        {
            ShowLocalizedMsg(IDS_ERR_BAD_OPTION, p[0]);
            exit(1);
        }
    }
    else
    {
        /* Unrecognized option or missing parameter */
        ShowLocalizedMsg(IDS_ERR_BAD_OPTION, p[0]);
        exit(1);
    }
    return i;
}


static void
parse_argv(options_t *options, int argc, TCHAR **argv)
{
    int i, j;

    /* parse command line */
    for (i = 1; i < argc; ++i)
    {
        TCHAR *p[MAX_PARMS];
        CLEAR(p);
        p[0] = argv[i];
        if (_tcsncmp(p[0], _T("--"), 2) != 0)
        {
            /* Missing -- before option. */
            ShowLocalizedMsg(IDS_ERR_BAD_PARAMETER, p[0]);
            exit(0);
        }

        p[0] += 2;

        for (j = 1; j < MAX_PARMS; ++j)
        {
            if (i + j < argc)
            {
                TCHAR *arg = argv[i + j];
                if (_tcsncmp(arg, _T("--"), 2) == 0)
                    break;
                p[j] = arg;
            }
        }
        i = add_option(options, i, p);
    }
}


void
InitOptions(options_t *opt)
{
    CLEAR(*opt);
    opt->netcmd_semaphore = InitSemaphore (NULL);
    opt->version = MakeVersion (PACKAGE_VERSION_RESOURCE);
    opt->clr_warning = RGB(0xff, 0, 0);
    opt->clr_error = RGB(0xff, 0, 0);
}


void
ProcessCommandLine(options_t *options, TCHAR *command_line)
{
    TCHAR **argv;
    TCHAR *pos = command_line;
    int argc = 0;

    /* Count the arguments */
    do
    {
        while (*pos == _T(' '))
            ++pos;

        if (*pos == _T('\0'))
            break;

        ++argc;

        while (*pos != _T('\0') && *pos != _T(' '))
           ++pos;
    }
    while (*pos != _T('\0'));

    if (argc == 0)
        return;

    /* Tokenize the arguments */
    argv = (TCHAR**) malloc(argc * sizeof(TCHAR*));
    pos = command_line;
    argc = 0;

    do
    {
        while (*pos == _T(' '))
            pos++;

        if (*pos == _T('\0'))
            break;

        if (*pos == _T('\"'))
        {
            argv[argc++] = ++pos;
            while (*pos != _T('\0') && *pos != _T('\"'))
                ++pos;
        }
        else
        {
            argv[argc++] = pos;
            while (*pos != _T('\0') && *pos != _T(' '))
                ++pos;
        }

        if (*pos == _T('\0'))
            break;

        *pos++ = _T('\0');
    }
    while (*pos != _T('\0'));

    parse_argv(options, argc, argv);

    free(argv);

    ExpandOptions ();
}


/* Return num of connections with state = check */
int
CountConnState(conn_state_t check)
{
    int i;
    int count = 0;

    for (i = 0; i < o.num_configs; ++i)
    {
        if (o.conn[i].state == check)
            ++count;
    }

    return count;
}

connection_t*
GetConnByManagement(SOCKET sk)
{
    int i;
    for (i = 0; i < o.num_configs; ++i)
    {
        if (o.conn[i].manage.sk == sk)
            return &o.conn[i];
    }
    return NULL;
}

connection_t*
GetConnByName(const WCHAR *name)
{
    for (int i = 0; i < o.num_configs; ++i)
    {
        if (wcsicmp (o.conn[i].config_file, name) == 0
            || wcsicmp(o.conn[i].config_name, name) == 0)
            return &o.conn[i];
    }
    return NULL;
}

/* callback to set the initial value of folder browse selection */
static int CALLBACK
BrowseCallback (HWND h, UINT msg, UNUSED LPARAM l, LPARAM data)
{
    if (msg == BFFM_INITIALIZED)
        SendMessage (h, BFFM_SETSELECTION, TRUE, data);

    return 0;
}

static BOOL
BrowseFolder (const WCHAR *initial_path, WCHAR *selected_path)
{
    BOOL ret = false;
    BROWSEINFO bi;

    CLEAR(bi);
    bi.lpszTitle = L"Select folder...";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    bi.lpfn    = BrowseCallback;
    bi.lParam  = (LPARAM) initial_path;

    PIDLIST_ABSOLUTE idlist = SHBrowseForFolder (&bi);

    if (idlist)
    {
        ret = SHGetPathFromIDList (idlist, selected_path);
        CoTaskMemFree (idlist);
    }

    return ret;
}

static BOOL
CheckAdvancedDlgParams (HWND hdlg)
{
    WCHAR tmp_path[MAX_PATH];

    /* replace empty entries by current values */
    if (GetWindowTextLength (GetDlgItem(hdlg, ID_EDT_CONFIG_DIR)) == 0)
        SetDlgItemText (hdlg, ID_EDT_CONFIG_DIR, o.config_dir);
    if (GetWindowTextLength (GetDlgItem(hdlg, ID_EDT_LOG_DIR)) == 0)
        SetDlgItemText (hdlg, ID_EDT_LOG_DIR, o.log_dir);
    if (GetWindowTextLength (GetDlgItem(hdlg, ID_EDT_CONFIG_EXT)) == 0)
        SetDlgItemText (hdlg, ID_EDT_CONFIG_EXT, o.ext_string);

    /* ensure paths are absolute */
    GetDlgItemText (hdlg, ID_EDT_CONFIG_DIR, tmp_path, _countof(tmp_path));
    ExpandString (tmp_path, _countof(tmp_path));
    if (PathIsRelativeW (tmp_path))
    {
        MessageBox (NULL, L"Specified config directory is not an absolute path",
                    L"Option error", MB_OK);
        return false;
    }

    GetDlgItemText (hdlg, ID_EDT_LOG_DIR, tmp_path, _countof(tmp_path));
    ExpandString (tmp_path, _countof(tmp_path));
    if (PathIsRelativeW (tmp_path))
    {
        MessageBox (NULL, L"Specified log directory is not an absolute path",
                    L"Option error", MB_OK);
        return false;
    }
    return true;
}

static BOOL
SaveAdvancedDlgParams (HWND hdlg)
{
    WCHAR tmp_path[MAX_PATH], tmp_path1[MAX_PATH];
    UINT tmp;
    BOOL status;

    GetDlgItemText (hdlg, ID_EDT_CONFIG_DIR, o.config_dir, _countof(o.config_dir));

    GetDlgItemText (hdlg, ID_EDT_LOG_DIR, tmp_path, _countof(tmp_path));
    wcsncpy (tmp_path1, tmp_path, _countof(tmp_path1));
    ExpandString (tmp_path1, _countof(tmp_path1));

    if (EnsureDirExists (tmp_path1)) /* this will try to create the path if needed */
        wcsncpy (o.log_dir, tmp_path, _countof(o.log_dir)); /* save unexpanded path */
    else
    {
        ShowLocalizedMsg(IDS_ERR_CREATE_PATH, L"Log", tmp_path1);
        return false;
    }

    GetDlgItemText (hdlg, ID_EDT_CONFIG_EXT, o.ext_string, _countof(o.ext_string));

    tmp = GetDlgItemInt (hdlg, ID_EDT_PRECONNECT_TIMEOUT, &status, FALSE);
    if (status && tmp > 0) o.preconnectscript_timeout = tmp;

    tmp = GetDlgItemInt (hdlg, ID_EDT_CONNECT_TIMEOUT, &status, FALSE);
    if (status) o.connectscript_timeout = tmp;

    tmp = GetDlgItemInt (hdlg, ID_EDT_DISCONNECT_TIMEOUT, &status, FALSE);
    if (status && tmp > 0) o.disconnectscript_timeout = tmp;

    SaveRegistryKeys ();
    ExpandOptions ();

    return true;
}

static void
LoadAdvancedDlgParams (HWND hdlg)
{
    SetDlgItemText (hdlg, ID_EDT_CONFIG_DIR, o.config_dir);
    SetDlgItemText (hdlg, ID_EDT_CONFIG_EXT, o.ext_string);
    SetDlgItemText (hdlg, ID_EDT_LOG_DIR, o.log_dir);
    SetDlgItemInt (hdlg, ID_EDT_PRECONNECT_TIMEOUT, o.preconnectscript_timeout, FALSE);
    SetDlgItemInt (hdlg, ID_EDT_CONNECT_TIMEOUT, o.connectscript_timeout, FALSE);
    SetDlgItemInt (hdlg, ID_EDT_DISCONNECT_TIMEOUT, o.disconnectscript_timeout, FALSE);
}

INT_PTR CALLBACK
AdvancedSettingsDlgProc (HWND hwndDlg, UINT msg, UNUSED WPARAM wParam, LPARAM lParam)
{
    LPPSHNOTIFY psn;

    switch(msg) {

    case WM_INITDIALOG:
        /* Limit extension editbox to 4 chars. */
        SendMessage (GetDlgItem(hwndDlg, ID_EDT_CONFIG_EXT), EM_SETLIMITTEXT, 4, 0);
        /* Populate UI */
        LoadAdvancedDlgParams (hwndDlg);
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        WCHAR path[MAX_PATH];

        case ID_BTN_CONFIG_DIR:
            GetDlgItemText (hwndDlg, ID_EDT_CONFIG_DIR, path, _countof(path));
            if (BrowseFolder (path, path))
                SetDlgItemText (hwndDlg, ID_EDT_CONFIG_DIR, path);
            break;

        case ID_BTN_LOG_DIR:
            GetDlgItemText (hwndDlg, ID_EDT_LOG_DIR, path, _countof(path));
            if (BrowseFolder (path, path))
                SetDlgItemText (hwndDlg, ID_EDT_LOG_DIR, path);
            break;
        }
        break;

    case WM_NOTIFY:
        psn = (LPPSHNOTIFY) lParam;
        if (psn->hdr.code == (UINT) PSN_KILLACTIVE)
        {
            SetWindowLongPtr (hwndDlg, DWLP_MSGRESULT, (CheckAdvancedDlgParams(hwndDlg) ? FALSE : TRUE));
            return TRUE;
        }
        if (psn->hdr.code == (UINT) PSN_APPLY)
        {
            BOOL status = SaveAdvancedDlgParams (hwndDlg);
            SetWindowLongPtr (hwndDlg, DWLP_MSGRESULT, status? PSNRET_NOERROR:PSNRET_INVALID);
            return TRUE;
        }
        break;
    }

    return FALSE;
}

int
CompareStringExpanded (const WCHAR *str1, const WCHAR *str2)
{
    WCHAR str1_cpy[MAX_PATH], str2_cpy[MAX_PATH];

    wcsncpy (str1_cpy, str1, _countof(str1_cpy));
    wcsncpy (str2_cpy, str2, _countof(str2_cpy));
    str1_cpy[MAX_PATH-1] = L'\0';
    str2_cpy[MAX_PATH-1] = L'\0';

    ExpandString (str1_cpy, _countof(str1_cpy));
    ExpandString (str2_cpy, _countof(str2_cpy));

    return wcsicmp (str1_cpy, str2_cpy);
}

/* Hide the password save options from user */
void
DisableSavePasswords(connection_t *c)
{
    DeleteSavedPasswords(c->config_name);
    c->flags &= ~(FLAG_SAVE_AUTH_PASS | FLAG_SAVE_KEY_PASS);
    c->flags |= FLAG_DISABLE_SAVE_PASS;
}
