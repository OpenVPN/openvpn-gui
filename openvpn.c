/*
 *  OpenVPN-GUI -- A Windows GUI for OpenVPN.
 *
 *  Copyright (C) 2004 Mathias Sundman <mathias@nilings.se>
 *                2010 Heiko Hund <heikoh@users.sf.net>
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
#include <windowsx.h>
#include <tchar.h>
#include <stdlib.h>
#include <stdio.h>
#include <process.h>
#include <richedit.h>

#include "tray.h"
#include "main.h"
#include "openvpn.h"
#include "openvpn_config.h"
#include "openvpn-gui-res.h"
#include "options.h"
#include "scripts.h"
#include "viewlog.h"
#include "proxy.h"
#include "passphrase.h"
#include "localization.h"
#include "misc.h"

#define WM_OVPN_STOP    (WM_APP + 10)
#define WM_OVPN_SUSPEND (WM_APP + 11)

extern options_t o;

const TCHAR *cfgProp = _T("conn");

/*
 * Receive banner on connection to management interface
 * Format: <BANNER>
 */
void
OnReady(connection_t *c, UNUSED char *msg)
{
    ManagementCommand(c, "state on", NULL, regular);
    ManagementCommand(c, "log all on", OnLogLine, combined);
}


/*
 * Handle the request to release a hold from the OpenVPN management interface
 */
void
OnHold(connection_t *c, UNUSED char *msg)
{
    ManagementCommand(c, "hold off", NULL, regular);
    ManagementCommand(c, "hold release", NULL, regular);
}

/*
 * Handle a log line from the OpenVPN management interface
 * Format <TIMESTAMP>,<FLAGS>,<MESSAGE>
 */
void
OnLogLine(connection_t *c, char *line)
{
    HWND logWnd = GetDlgItem(c->hwndStatus, ID_EDT_LOG);
    char *flags, *message;
    time_t timestamp;
    TCHAR *datetime;
    const SETTEXTEX ste = {
        .flags = ST_SELECTION,
        .codepage = CP_UTF8
    };

    flags = strchr(line, ',') + 1;
    if (flags - 1 == NULL)
        return;

    message = strchr(flags, ',') + 1;
    if (message - 1 == NULL)
        return;

    /* Remove lines from log window if it is getting full */
    if (SendMessage(logWnd, EM_GETLINECOUNT, 0, 0) > MAX_LOG_LINES)
    {
        int pos = SendMessage(logWnd, EM_LINEINDEX, DEL_LOG_LINES, 0);
        SendMessage(logWnd, EM_SETSEL, 0, pos);
        SendMessage(logWnd, EM_REPLACESEL, FALSE, (LPARAM) _T(""));
    }

    timestamp = strtol(line, NULL, 10);
    datetime = _tctime(&timestamp);
    datetime[24] = _T(' ');

    /* Append line to log window */
    SendMessage(logWnd, EM_SETSEL, (WPARAM) -1, (LPARAM) -1);
    SendMessage(logWnd, EM_REPLACESEL, FALSE, (LPARAM) datetime);
    SendMessage(logWnd, EM_SETTEXTEX, (WPARAM) &ste, (LPARAM) message);
    SendMessage(logWnd, EM_REPLACESEL, FALSE, (LPARAM) _T("\n"));
}


/*
 * Handle a state change notification from the OpenVPN management interface
 * Format <TIMESTAMP>,<STATE>,[<MESSAGE>],[<LOCAL_IP>][,<REMOTE_IP>]
 */
void
OnStateChange(connection_t *c, char *data)
{
    char *pos, *state, *message;

    pos = strchr(data, ',');
    if (pos == NULL)
        return;
    *pos = '\0';

    state = pos + 1;
    pos = strchr(state, ',');
    if (pos == NULL)
        return;
    *pos = '\0';

    message = pos + 1;
    pos = strchr(message, ',');
    if (pos == NULL)
        return;
    *pos = '\0';

    if (strcmp(state, "CONNECTED") == 0)
    {
        /* Run Connect Script */
        if (c->state == connecting || c->state == resuming)
            RunConnectScript(c, false);

        /* Save the local IP address if available */
        char *local_ip = pos + 1;
        pos = strchr(local_ip, ',');
        if (pos != NULL)
            *pos = '\0';

        /* Convert the IP address to Unicode */
        MultiByteToWideChar(CP_ACP, 0, local_ip, -1, c->ip, _countof(c->ip));

        /* Show connection tray balloon */
        if ((c->state == connecting   && o.show_balloon[0] != '0')
        ||  (c->state == resuming     && o.show_balloon[0] != '0')
        ||  (c->state == reconnecting && o.show_balloon[0] == '2'))
        {
            TCHAR msg[256];
            LoadLocalizedStringBuf(msg, _countof(msg), IDS_NFO_NOW_CONNECTED, c->config_name);
            ShowTrayBalloon(msg, (_tcslen(c->ip) ? LoadLocalizedString(IDS_NFO_ASSIGN_IP, c->ip) : _T("")));
        }

        /* Save time when we got connected. */
        c->connected_since = atoi(data);
        c->failed_psw_attempts = 0;
        c->state = connected;

        SetMenuStatus(c, connected);
        SetTrayIcon(connected);

        SetDlgItemText(c->hwndStatus, ID_TXT_STATUS, LoadLocalizedString(IDS_NFO_STATE_CONNECTED));
        SetStatusWinIcon(c->hwndStatus, ID_ICO_CONNECTED);

        /* Hide Status Window */
        ShowWindow(c->hwndStatus, SW_HIDE);
    }
    else if (strcmp(state, "RECONNECTING") == 0)
    {
        if (strcmp(message, "auth-failure") == 0
        ||  strcmp(message, "private-key-password-failure") == 0)
            c->failed_psw_attempts++;

        if (c->failed_psw_attempts >= o.psw_attempts - 1)
            ManagementCommand(c, "auth-retry none", NULL, regular);

        c->state = reconnecting;
        CheckAndSetTrayIcon();

        SetDlgItemText(c->hwndStatus, ID_TXT_STATUS, LoadLocalizedString(IDS_NFO_STATE_RECONNECTING));
        SetStatusWinIcon(c->hwndStatus, ID_ICO_CONNECTING);
    }
}


/*
 * DialogProc for OpenVPN username/password auth dialog windows
 */
INT_PTR CALLBACK
UserAuthDialogFunc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    connection_t *c;

    switch (msg)
    {
    case WM_INITDIALOG:
        /* Set connection for this dialog and show it */
        c = (connection_t *) lParam;
        SetProp(hwndDlg, cfgProp, (HANDLE) c);
        if (c->state == resuming)
            ForceForegroundWindow(hwndDlg);
        else
            SetForegroundWindow(hwndDlg);
        break;

    case WM_COMMAND:
        c = (connection_t *) GetProp(hwndDlg, cfgProp);
        switch (LOWORD(wParam))
        {
        case ID_EDT_AUTH_USER:
            if (HIWORD(wParam) == EN_UPDATE)
            {
                int len = Edit_GetTextLength((HWND) lParam);
                EnableWindow(GetDlgItem(hwndDlg, IDOK), (len ? TRUE : FALSE));
            }
            break;

        case IDOK:
            ManagementCommandFromInput(c, "username \"Auth\" \"%s\"", hwndDlg, ID_EDT_AUTH_USER);
            ManagementCommandFromInput(c, "password \"Auth\" \"%s\"", hwndDlg, ID_EDT_AUTH_PASS);
            EndDialog(hwndDlg, LOWORD(wParam));
            return TRUE;

        case IDCANCEL:
            EndDialog(hwndDlg, LOWORD(wParam));
            StopOpenVPN(c);
            return TRUE;
        }
        break;

    case WM_CLOSE:
        EndDialog(hwndDlg, LOWORD(wParam));
        return TRUE;

    case WM_NCDESTROY:
        RemoveProp(hwndDlg, cfgProp);
        break;
    }

    return FALSE;
}


/*
 * DialogProc for OpenVPN private key password dialog windows
 */
INT_PTR CALLBACK
PrivKeyPassDialogFunc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    connection_t *c;

    switch (msg)
    {
    case WM_INITDIALOG:
        /* Set connection for this dialog and show it */
        c = (connection_t *) lParam;
        SetProp(hwndDlg, cfgProp, (HANDLE) c);
        if (c->state == resuming)
            ForceForegroundWindow(hwndDlg);
        else
            SetForegroundWindow(hwndDlg);
        break;

    case WM_COMMAND:
        c = (connection_t *) GetProp(hwndDlg, cfgProp);
        switch (LOWORD(wParam))
        {
        case IDOK:
            ManagementCommandFromInput(c, "password \"Private Key\" \"%s\"", hwndDlg, ID_EDT_PASSPHRASE);
            EndDialog(hwndDlg, LOWORD(wParam));
            return TRUE;

        case IDCANCEL:
            EndDialog(hwndDlg, LOWORD(wParam));
            return TRUE;
        }
        break;

    case WM_CLOSE:
        EndDialog(hwndDlg, LOWORD(wParam));
        return TRUE;

    case WM_NCDESTROY:
        RemoveProp(hwndDlg, cfgProp);
        break;
  }
  return FALSE;
}


/*
 * Handle the request to release a hold from the OpenVPN management interface
 */
void
OnPassword(connection_t *c, char *msg)
{
    if (strncmp(msg, "Verification Failed", 19) == 0)
        return;

    if (strstr(msg, "'Auth'"))
    {
        LocalizedDialogBoxParam(ID_DLG_AUTH, UserAuthDialogFunc, (LPARAM) c);
    }
    else if (strstr(msg, "'Private Key'"))
    {
        LocalizedDialogBoxParam(ID_DLG_PASSPHRASE, PrivKeyPassDialogFunc, (LPARAM) c);
    }
    else if (strstr(msg, "'HTTP Proxy'"))
    {
        QueryProxyAuth(c, http);
    }
    else if (strstr(msg, "'SOCKS Proxy'"))
    {
        QueryProxyAuth(c, socks);
    }
}


/*
 * Handle exit of the OpenVPN process
 */
void
OnStop(connection_t *c, UNUSED char *msg)
{
    UINT txt_id, msg_id;
    TCHAR *msg_xtra;
    SetMenuStatus(c, disconnected);

    switch (c->state)
    {
    case connected:
        /* OpenVPN process ended unexpectedly */
        c->failed_psw_attempts = 0;
        c->state = disconnected;
        CheckAndSetTrayIcon();
        SetDlgItemText(c->hwndStatus, ID_TXT_STATUS, LoadLocalizedString(IDS_NFO_STATE_DISCONNECTED));
        SetStatusWinIcon(c->hwndStatus, ID_ICO_DISCONNECTED);
        EnableWindow(GetDlgItem(c->hwndStatus, ID_DISCONNECT), FALSE);
        EnableWindow(GetDlgItem(c->hwndStatus, ID_RESTART), FALSE);
        if (o.silent_connection[0] == '0')
        {
            SetForegroundWindow(c->hwndStatus);
            ShowWindow(c->hwndStatus, SW_SHOW);
        }
        ShowLocalizedMsg(IDS_NFO_CONN_TERMINATED, c->config_name);
        SendMessage(c->hwndStatus, WM_CLOSE, 0, 0);
        break;

    case resuming:
    case connecting:
    case reconnecting:
    case timedout:
        /* We have failed to (re)connect */
        txt_id = c->state == reconnecting ? IDS_NFO_STATE_FAILED_RECONN : IDS_NFO_STATE_FAILED;
        msg_id = c->state == reconnecting ? IDS_NFO_RECONN_FAILED : IDS_NFO_CONN_FAILED;
        msg_xtra = c->state == timedout ? c->log_path : c->config_name;
        if (c->state == timedout)
            msg_id = IDS_NFO_CONN_TIMEOUT;

        c->state = disconnecting;
        CheckAndSetTrayIcon();
        c->state = disconnected;
        EnableWindow(GetDlgItem(c->hwndStatus, ID_DISCONNECT), FALSE);
        EnableWindow(GetDlgItem(c->hwndStatus, ID_RESTART), FALSE);
        SetStatusWinIcon(c->hwndStatus, ID_ICO_DISCONNECTED);
        SetDlgItemText(c->hwndStatus, ID_TXT_STATUS, LoadLocalizedString(txt_id));
        if (o.silent_connection[0] == '0')
        {
            SetForegroundWindow(c->hwndStatus);
            ShowWindow(c->hwndStatus, SW_SHOW);
        }
        ShowLocalizedMsg(msg_id, msg_xtra);
        SendMessage(c->hwndStatus, WM_CLOSE, 0, 0);
        break;

    case disconnecting:
//   /* Check for "certificate has expired" message */
//   if ((strstr(line, "error=certificate has expired") != NULL))
//     {
//       StopOpenVPN(config);
//       /* Cert expired... */
//       ShowLocalizedMsg(IDS_ERR_CERT_EXPIRED);
//     }
//
//   /* Check for "certificate is not yet valid" message */
//   if ((strstr(line, "error=certificate is not yet valid") != NULL))
//     {
//       StopOpenVPN(config);
//       /* Cert not yet valid */
//       ShowLocalizedMsg(IDS_ERR_CERT_NOT_YET_VALID);
//     }
        /* Shutdown was initiated by us */
        c->failed_psw_attempts = 0;
        c->state = disconnected;
        CheckAndSetTrayIcon();
        SendMessage(c->hwndStatus, WM_CLOSE, 0, 0);
        break;

    case suspending:
        c->state = suspended;
        CheckAndSetTrayIcon();
        SetDlgItemText(c->hwndStatus, ID_TXT_STATUS, LoadLocalizedString(IDS_NFO_STATE_SUSPENDED));
        break;

    default:
        break;
    }
}


/*
 * DialogProc for OpenVPN status dialog windows
 */
INT_PTR CALLBACK
StatusDialogFunc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    connection_t *c;

    switch (msg)
    {
    case WM_MANAGEMENT:
        /* Management interface related event */
        OnManagement(wParam, lParam);
        return TRUE;

    case WM_INITDIALOG:
        c = (connection_t *) lParam;

        /* Set window icon "disconnected" */
        SetStatusWinIcon(hwndDlg, ID_ICO_CONNECTING);

        /* Set connection for this dialog */
        SetProp(hwndDlg, cfgProp, (HANDLE) c);

        /* Create log window */
        HWND hLogWnd = CreateWindowEx(0, RICHEDIT_CLASS, NULL,
            WS_CHILD|WS_VISIBLE|WS_HSCROLL|WS_VSCROLL|ES_SUNKEN|ES_LEFT|
            ES_MULTILINE|ES_READONLY|ES_AUTOHSCROLL|ES_AUTOVSCROLL,
            20, 25, 350, 160, hwndDlg, (HMENU) ID_EDT_LOG, o.hInstance, NULL);
        if (!hLogWnd)
        {
            ShowLocalizedMsg(IDS_ERR_CREATE_EDIT_LOGWINDOW);
            return FALSE;
        }

        /* Set font and fontsize of the log window */
        CHARFORMAT cfm = {
            .cbSize = sizeof(CHARFORMAT),
            .dwMask = CFM_SIZE|CFM_FACE|CFM_BOLD,
            .szFaceName = _T("Microsoft Sans Serif"),
            .dwEffects = 0,
            .yHeight = 160
        };
        if (SendMessage(hLogWnd, EM_SETCHARFORMAT, SCF_DEFAULT, (LPARAM) &cfm) == 0)
            ShowLocalizedMsg(IDS_ERR_SET_SIZE);

        /* Set size and position of controls */
        RECT rect;
        GetClientRect(hwndDlg, &rect);
        MoveWindow(hLogWnd, 20, 25, rect.right - 40, rect.bottom - 70, TRUE);
        MoveWindow(GetDlgItem(hwndDlg, ID_TXT_STATUS), 20, 5, rect.right - 25, 15, TRUE);
        MoveWindow(GetDlgItem(hwndDlg, ID_DISCONNECT), 20, rect.bottom - 30, 110, 23, TRUE);
        MoveWindow(GetDlgItem(hwndDlg, ID_RESTART), 145, rect.bottom - 30, 110, 23, TRUE);
        MoveWindow(GetDlgItem(hwndDlg, ID_HIDE), rect.right - 130, rect.bottom - 30, 110, 23, TRUE);

        /* Set focus on the LogWindow so it scrolls automatically */
        SetFocus(hLogWnd);
        return FALSE;

    case WM_SIZE:
        MoveWindow(GetDlgItem(hwndDlg, ID_EDT_LOG), 20, 25, LOWORD(lParam) - 40, HIWORD(lParam) - 70, TRUE);
        MoveWindow(GetDlgItem(hwndDlg, ID_DISCONNECT), 20, HIWORD(lParam) - 30, 110, 23, TRUE);
        MoveWindow(GetDlgItem(hwndDlg, ID_RESTART), 145, HIWORD(lParam) - 30, 110, 23, TRUE);
        MoveWindow(GetDlgItem(hwndDlg, ID_HIDE), LOWORD(lParam) - 130, HIWORD(lParam) - 30, 110, 23, TRUE);
        MoveWindow(GetDlgItem(hwndDlg, ID_TXT_STATUS), 20, 5, LOWORD(lParam) - 25, 15, TRUE);
        InvalidateRect(hwndDlg, NULL, TRUE);
        return TRUE;

    case WM_COMMAND:
        c = (connection_t *) GetProp(hwndDlg, cfgProp);
        switch (LOWORD(wParam))
        {
        case ID_DISCONNECT:
            SetFocus(GetDlgItem(c->hwndStatus, ID_EDT_LOG));
            StopOpenVPN(c);
            return TRUE;

        case ID_HIDE:
            if (c->state != disconnected)
                ShowWindow(hwndDlg, SW_HIDE);
            else
                DestroyWindow(hwndDlg);
            return TRUE;

        case ID_RESTART:
            c->state = reconnecting;
            SetFocus(GetDlgItem(c->hwndStatus, ID_EDT_LOG));
            ManagementCommand(c, "signal SIGHUP", NULL, regular);
            return TRUE;
        }
        break;

    case WM_SHOWWINDOW:
        if (wParam == TRUE)
        {
            c = (connection_t *) GetProp(hwndDlg, cfgProp);
            if (c->hwndStatus)
                SetFocus(GetDlgItem(c->hwndStatus, ID_EDT_LOG));
        }
        return FALSE;

    case WM_CLOSE:
        c = (connection_t *) GetProp(hwndDlg, cfgProp);
        if (c->state != disconnected)
            ShowWindow(hwndDlg, SW_HIDE);
        else
            DestroyWindow(hwndDlg);
        return TRUE;

    case WM_NCDESTROY:
        RemoveProp(hwndDlg, cfgProp);
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    }
    return FALSE;
}


/*
 * ThreadProc for OpenVPN status dialog windows
 */
static DWORD WINAPI
ThreadOpenVPNStatus(void *p)
{
    connection_t *c = p;
    TCHAR conn_name[200];
    MSG msg;

    /* Cut of extention from config filename. */
    _tcsncpy(conn_name, c->config_file, _countof(conn_name));
    conn_name[_tcslen(conn_name) - _tcslen(o.ext_string) - 1] = _T('\0');

    c->state = (c->state == suspended ? resuming : connecting);

    /* Create and Show Status Dialog */
    c->hwndStatus = CreateLocalizedDialogParam(ID_DLG_STATUS, StatusDialogFunc, (LPARAM) c);
    if (!c->hwndStatus)
        return 1;

    CheckAndSetTrayIcon();
    SetMenuStatus(c, connecting);
    SetDlgItemText(c->hwndStatus, ID_TXT_STATUS, LoadLocalizedString(IDS_NFO_STATE_CONNECTING));
    SetWindowText(c->hwndStatus, LoadLocalizedString(IDS_NFO_CONNECTION_XXX, conn_name));

    if (!OpenManagement(c))
        PostMessage(c->hwndStatus, WM_CLOSE, 0, 0);

    if (o.silent_connection[0] == '0')
        ShowWindow(c->hwndStatus, SW_SHOW);

    /* Run the message loop for the status window */
    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (msg.hwnd == NULL)
        {
            switch (msg.message)
            {
            case WM_OVPN_STOP:
                c->state = disconnecting;
                RunDisconnectScript(c, false);
                EnableWindow(GetDlgItem(c->hwndStatus, ID_DISCONNECT), FALSE);
                EnableWindow(GetDlgItem(c->hwndStatus, ID_RESTART), FALSE);
                SetMenuStatus(c, disconnecting);
                SetDlgItemText(c->hwndStatus, ID_TXT_STATUS, LoadLocalizedString(IDS_NFO_STATE_WAIT_TERM));
                SetEvent(c->exit_event);
                break;

            case WM_OVPN_SUSPEND:
                c->state = suspending;
                EnableWindow(GetDlgItem(c->hwndStatus, ID_DISCONNECT), FALSE);
                EnableWindow(GetDlgItem(c->hwndStatus, ID_RESTART), FALSE);
                SetMenuStatus(&o.conn[config], disconnecting);
                SetDlgItemText(c->hwndStatus, ID_TXT_STATUS, LoadLocalizedString(IDS_NFO_STATE_WAIT_TERM));
                SetEvent(c->exit_event);
                break;
            }
        }
        else if (IsDialogMessage(c->hwndStatus, &msg) == 0)
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    return 0;
}


/*
 * Set priority based on the registry or cmd-line value
 */
static BOOL
SetProcessPriority(DWORD *priority)
{
    *priority = NORMAL_PRIORITY_CLASS;
    if (!_tcscmp(o.priority_string, _T("IDLE_PRIORITY_CLASS")))
        *priority = IDLE_PRIORITY_CLASS;
    else if (!_tcscmp(o.priority_string, _T("BELOW_NORMAL_PRIORITY_CLASS")))
        *priority = BELOW_NORMAL_PRIORITY_CLASS;
    else if (!_tcscmp(o.priority_string, _T("NORMAL_PRIORITY_CLASS")))
        *priority = NORMAL_PRIORITY_CLASS;
    else if (!_tcscmp(o.priority_string, _T("ABOVE_NORMAL_PRIORITY_CLASS")))
        *priority = ABOVE_NORMAL_PRIORITY_CLASS;
    else if (!_tcscmp(o.priority_string, _T("HIGH_PRIORITY_CLASS")))
        *priority = HIGH_PRIORITY_CLASS;
    else
    {
        ShowLocalizedMsg(IDS_ERR_UNKNOWN_PRIORITY, o.priority_string);
        return FALSE;
    }
    return TRUE;
}


/*
 * Launch an OpenVPN process and the accompanying thread to monitor it
 */
BOOL
StartOpenVPN(connection_t *c)
{
    TCHAR cmdline[1024];
    TCHAR *options = cmdline + 8;
    TCHAR exit_event_name[17];
    HANDLE hStdInRead = NULL, hStdInWrite = NULL;
    HANDLE hNul = NULL, hThread = NULL, service = NULL;
    DWORD written;
    BOOL retval = FALSE;

    CLEAR(c->ip);

    RunPreconnectScript(c);

    /* Check that log append flag has a valid value */
    if ((o.append_string[0] != '0') && (o.append_string[0] != '1'))
    {
        ShowLocalizedMsg(IDS_ERR_LOG_APPEND_BOOL, o.append_string);
        return FALSE;
    }

    /* Create thread to show the connection's status dialog */
    hThread = CreateThread(NULL, 0, ThreadOpenVPNStatus, c, CREATE_SUSPENDED, &c->threadId);
    if (hThread == NULL)
    {
        ShowLocalizedMsg(IDS_ERR_CREATE_THREAD_STATUS);
        goto out;
    }

    /* Create an event object to signal OpenVPN to exit */
    _sntprintf_0(exit_event_name, _T("%x%08x"), GetCurrentProcessId(), c->threadId);
    c->exit_event = CreateEvent(NULL, TRUE, FALSE, exit_event_name);
    if (c->exit_event == NULL)
    {
        ShowLocalizedMsg(IDS_ERR_CREATE_EVENT, exit_event_name);
        goto out;
    }

    /* Create a management interface password */
    GetRandomPassword(c->manage.password, sizeof(c->manage.password) - 1);

    /* Construct command line -- put log first */
    _sntprintf_0(cmdline, _T("openvpn --log%s \"%s\" --config \"%s\" "
        "--setenv IV_GUI_VER \"%S\" --service %s 0 --auth-retry interact "
        "--management %S %hd stdin --management-query-passwords %s"
        "--management-hold"),
        (o.append_string[0] == '1' ? _T("-append") : _T("")), c->log_path,
        c->config_file, PACKAGE_STRING, exit_event_name,
        inet_ntoa(c->manage.skaddr.sin_addr), ntohs(c->manage.skaddr.sin_port),
        (o.proxy_source != config ? _T("--management-query-proxy ") : _T("")));

    /* Try to open the service pipe */
    if (!IsUserAdmin())
      service = CreateFile(_T("\\\\.\\pipe\\openvpn\\service"),
                GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

    if (service && service != INVALID_HANDLE_VALUE)
    {
        DWORD size = _tcslen(c->config_dir) + _tcslen(options) + sizeof(c->manage.password) + 3;
        TCHAR startup_info[1024];
        DWORD dwMode = PIPE_READMODE_MESSAGE;
        if (!SetNamedPipeHandleState(service, &dwMode, NULL, NULL))
        {
            ShowLocalizedMsg (IDS_ERR_ACCESS_SERVICE_PIPE);
            CloseHandle(c->exit_event);
            goto out;
        }

        c->manage.password[sizeof(c->manage.password) - 1] = '\n';
        _sntprintf_0(startup_info, _T("%s%c%s%c%.*S"), c->config_dir, _T('\0'),
            options, _T('\0'), sizeof(c->manage.password), c->manage.password);
        c->manage.password[sizeof(c->manage.password) - 1] = '\0';

        if (!WriteFile(service, startup_info, size * sizeof (TCHAR), &written, NULL))
        {
            ShowLocalizedMsg (IDS_ERR_WRITE_SERVICE_PIPE);
            CloseHandle(c->exit_event);
            goto out;
        }
    }
    else
    {
        /* Start OpenVPN directly */
        DWORD priority;
        STARTUPINFO si;
        PROCESS_INFORMATION pi;
        SECURITY_DESCRIPTOR sd;

        /* Make I/O handles inheritable and accessible by all */
        SECURITY_ATTRIBUTES sa = {
            .nLength = sizeof(sa),
            .lpSecurityDescriptor = &sd,
            .bInheritHandle = TRUE
        };
        if (!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION))
        {
            ShowLocalizedMsg(IDS_ERR_INIT_SEC_DESC);
            CloseHandle(c->exit_event);
            return FALSE;
        }
        if (!SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE))
        {
            ShowLocalizedMsg(IDS_ERR_SET_SEC_DESC_ACL);
            CloseHandle(c->exit_event);
            return FALSE;
        }

        /* Set process priority */
        if (!SetProcessPriority(&priority))
        {
            CloseHandle(c->exit_event);
            return FALSE;
        }

        /* Get a handle of the NUL device */
        hNul = CreateFile(_T("NUL"), GENERIC_WRITE, FILE_SHARE_WRITE, &sa, OPEN_EXISTING, 0, NULL);
        if (hNul == INVALID_HANDLE_VALUE)
        {
            CloseHandle(c->exit_event);
            return FALSE;
        }

        /* Create the pipe for STDIN with only the read end inheritable */
        if (!CreatePipe(&hStdInRead, &hStdInWrite, &sa, 0))
        {
            ShowLocalizedMsg(IDS_ERR_CREATE_PIPE_IN_READ);
            CloseHandle(c->exit_event);
            goto out;
        }
        if (!SetHandleInformation(hStdInWrite, HANDLE_FLAG_INHERIT, 0))
        {
            ShowLocalizedMsg(IDS_ERR_DUP_HANDLE_IN_WRITE);
            CloseHandle(c->exit_event);
            goto out;
        }

        /* Fill in STARTUPINFO struct */
        GetStartupInfo(&si);
        si.cb = sizeof(si);
        si.dwFlags = STARTF_USESTDHANDLES;
        si.hStdInput = hStdInRead;
        si.hStdOutput = hNul;
        si.hStdError = hNul;

        /* Create an OpenVPN process for the connection */
        if (!CreateProcess(o.exe_path, cmdline, NULL, NULL, TRUE,
                        priority | CREATE_NO_WINDOW, NULL, c->config_dir, &si, &pi))
        {
            ShowLocalizedMsg(IDS_ERR_CREATE_PROCESS, o.exe_path, cmdline, c->config_dir);
            CloseHandle(c->exit_event);
            goto out;
        }

        /* Pass management password to OpenVPN process */
        c->manage.password[sizeof(c->manage.password) - 1] = '\n';
        WriteFile(hStdInWrite, c->manage.password, sizeof(c->manage.password), &written, NULL);
        c->manage.password[sizeof(c->manage.password) - 1] = '\0';

        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }

    /* Start the status dialog thread */
    ResumeThread(hThread);
    retval = TRUE;

out:
    if (service && service != INVALID_HANDLE_VALUE)
        CloseHandle(service);
    if (hThread && hThread != INVALID_HANDLE_VALUE)
        CloseHandle(hThread);
    if (hStdInWrite && hStdInWrite != INVALID_HANDLE_VALUE)
        CloseHandle(hStdInWrite);
    if (hStdInRead && hStdInRead != INVALID_HANDLE_VALUE)
        CloseHandle(hStdInRead);
    if (hNul && hNul != INVALID_HANDLE_VALUE)
        CloseHandle(hNul);
    return retval;
}


void
StopOpenVPN(connection_t *c)
{
    PostThreadMessage(c->threadId, WM_OVPN_STOP, 0, 0);
}


void
SuspendOpenVPN(int config)
{
    PostThreadMessage(o.conn[config].threadId, WM_OVPN_SUSPEND, 0, 0);
}


void
SetStatusWinIcon(HWND hwndDlg, int iconId)
{
    HICON hIcon = LoadLocalizedIcon(iconId);
    if (!hIcon)
        return;

    SendMessage(hwndDlg, WM_SETICON, (WPARAM) ICON_SMALL, (LPARAM) hIcon);
    SendMessage(hwndDlg, WM_SETICON, (WPARAM) ICON_BIG, (LPARAM) hIcon);
}


/*
 * Read one line from OpenVPN's stdout.
 */
static BOOL
ReadLineFromStdOut(HANDLE hStdOut, char *line, DWORD size)
{
    DWORD len, read;

    while (TRUE)
    {
        if (!PeekNamedPipe(hStdOut, line, size, &read, NULL, NULL))
        {
            if (GetLastError() != ERROR_BROKEN_PIPE)
                ShowLocalizedMsg(IDS_ERR_READ_STDOUT_PIPE);
            return FALSE;
        }

        char *pos = memchr(line, '\r', read);
        if (pos)
        {
            len = pos - line + 2;
            if (len > size)
                return FALSE;
            break;
        }

        /* Line doesn't fit into the buffer */
        if (read == size)
            return FALSE;

        Sleep(100);
    }

    if (!ReadFile(hStdOut, line, len, &read, NULL) || read != len)
    {
        if (GetLastError() != ERROR_BROKEN_PIPE)
            ShowLocalizedMsg(IDS_ERR_READ_STDOUT_PIPE);
        return FALSE;
    }

    line[read - 2] = '\0';
    return TRUE;
}


BOOL
CheckVersion()
{
    HANDLE hStdOutRead;
    HANDLE hStdOutWrite;
    BOOL retval = FALSE;
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    TCHAR cmdline[] = _T("openvpn --version");
    char match_version[] = "OpenVPN 2.";
    TCHAR pwd[MAX_PATH];
    char line[1024];
    TCHAR *p;

    CLEAR(si);
    CLEAR(pi);

    /* Make handles inheritable and accessible by all */
    SECURITY_DESCRIPTOR sd;
    SECURITY_ATTRIBUTES sa = {
        .nLength = sizeof(sa),
        .lpSecurityDescriptor = &sd,
        .bInheritHandle = TRUE
    };
    if (!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION))
    {
        ShowLocalizedMsg(IDS_ERR_INIT_SEC_DESC);
        return FALSE;
    }
    if (!SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE))
    {
        ShowLocalizedMsg(IDS_ERR_SET_SEC_DESC_ACL);
        return FALSE;
    }

    /* Create the pipe for STDOUT with inheritable write end */
    if (!CreatePipe(&hStdOutRead, &hStdOutWrite, &sa, 0))
    {
        ShowLocalizedMsg(IDS_ERR_CREATE_PIPE_IN_READ);
        return FALSE;
    }
    if (!SetHandleInformation(hStdOutRead, HANDLE_FLAG_INHERIT, 0))
    {
        ShowLocalizedMsg(IDS_ERR_DUP_HANDLE_IN_WRITE);
        goto out;
    }

    /* Construct the process' working directory */
    _tcsncpy(pwd, o.exe_path, _countof(pwd));
    p = _tcsrchr(pwd, _T('\\'));
    if (p != NULL)
        *p = _T('\0');

    /* Fill in STARTUPINFO struct */
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    si.hStdOutput = hStdOutWrite;
    si.hStdError = hStdOutWrite;

    /* Start OpenVPN to check version */
    if (!CreateProcess(o.exe_path, cmdline, NULL, NULL, TRUE,
                       CREATE_NO_WINDOW, NULL, pwd, &si, &pi))
    {
        ShowLocalizedMsg(IDS_ERR_CREATE_PROCESS, o.exe_path, cmdline, pwd);
    }
    else if (ReadLineFromStdOut(hStdOutRead, line, sizeof(line)))
    {
#ifdef DEBUG
        PrintDebug(_T("VersionString: %S"), line);
#endif
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);

        /* OpenVPN version 2.x */
        if (strstr(line, match_version))
            retval = TRUE;
    }

out:
    CloseHandle(hStdOutRead);
    CloseHandle(hStdOutWrite);
    return retval;
}

