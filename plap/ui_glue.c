/*
 *  OpenVPN-PLAP-Provider
 *
 *  Copyright (C) 2017-2022 Selva Nair <selva.nair@gmail.com>
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

#if !defined(UNICODE)
#error UNICODE and _UNICODE must be defined. This version only supports unicode builds.
#endif

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "plap_common.h"
#include "ui_glue.h"
#include "main.h"
#include "options.h"
#include "openvpn.h"
#include "manage.h"
#include "openvpn_config.h"
#include "proxy.h"
#include "registry.h"
#include "openvpn-gui-res.h"
#include "localization.h"
#include "misc.h"
#include "tray.h"
#include "service.h"

/* Global options structure */
options_t o;

int state_connected = connected, state_disconnected = disconnected, state_onhold = onhold;

static connection_t *active_profile;
DWORD status_menu_id = IDM_STATUSMENU;

/* Override management handlers that generate user dialogs
 * and pass them on only for currently active profile.
 * Also ensure no unwanted popus are generated.
 */
static void
OnStop_(connection_t *c, UNUSED char *msg)
{
    dmsg(L"profile: %ls with state = %d", c->config_name, c->state);

    /* do not show any popup error messages */
    c->state = disconnected;
    SetDlgItemText(c->hwndStatus, ID_TXT_STATUS, LoadLocalizedString(IDS_NFO_STATE_DISCONNECTED));
    SetStatusWinIcon(c->hwndStatus, ID_ICO_DISCONNECTED);
    SendMessage(c->hwndStatus, WM_CLOSE, 0, 0);
}

static void
OnInfoMsg_(connection_t *c, char *msg)
{
    if (c == active_profile)
    {
        OnInfoMsg(c, msg);
    }
    else
    {
        DetachOpenVPN(c); /* next attach will handle it */
    }
}

static void
OnNeedOk_(connection_t *c, char *msg)
{
    if (c == active_profile)
    {
        OnNeedOk(c, msg);
    }
    else
    {
        DetachOpenVPN(c); /* next attach will handle it */
    }
}

static void
OnNeedStr_(connection_t *c, char *msg)
{
    if (c == active_profile)
    {
        OnNeedStr(c, msg);
    }
    else
    {
        DetachOpenVPN(c); /* next attach will handle it */
    }
}

static void
OnPassword_(connection_t *c, char *msg)
{
    if (c == active_profile)
    {
        OnPassword(c, msg);
    }
    else
    {
        DetachOpenVPN(c); /* next attach will handle it */
    }
}

static void
OnProxy_(connection_t *c, char *msg)
{
    if (c == active_profile)
    {
        OnProxy(c, msg);
    }
    else
    {
        DetachOpenVPN(c); /* next attach will handle it */
    }
}

/* Intercept state change. Keep track of previous state and handle
 * cases like user wants to disconnect but connection completed in
 * the meantime.
 */
void
OnStateChange_(connection_t *c, char *msg)
{
    int state_prev = c->state;

    OnStateChange(c, msg);

    if (c->state == connected && state_prev == disconnecting)
    {
        /* connection completed while user clicked disconnect,
         * let disconnect process continue. This is required to
         * retain the hold state after SIGHUP restart.
         */
        c->state = disconnecting;
    }
}

/* Initialize GUI data structures. Returns 0 on success */
DWORD
InitializeUI(HINSTANCE hinstance)
{
    WSADATA wsaData;

    /* a session local semaphore to detect second instance */
    HANDLE session_semaphore = InitSemaphore(L"Local\\" PACKAGE_NAME "-PLAP");

    srand(time(NULL));
    /* try to lock the semaphore, else we are not the first instance */
    if (session_semaphore && WaitForSingleObject(session_semaphore, 200) != WAIT_OBJECT_0)
    {
        if (hinstance == o.hInstance)
        {
            /* already initialized */
            return 0;
        }
        else
        {
            MsgToEventLog(
                EVENTLOG_ERROR_TYPE,
                L"InitializeUI called a second time with a different hinstance -- multiple instances of the UI not supported.");
            return 1;
        }
    }

    dmsg(L"Starting OpenVPN UI v%hs", PACKAGE_VERSION);

    if (!GetModuleHandle(_T("RICHED20.DLL")))
    {
        LoadLibrary(_T("RICHED20.DLL"));
    }
    else
    {
        MsgToEventLog(EVENTLOG_ERROR_TYPE, LoadLocalizedString(IDS_ERR_LOAD_RICHED20));
        return 1;
    }

    /* Initialize handlers for management interface notifications
     * Some handlers are replaced by local functions
     */
    mgmt_rtmsg_handler handler[] = { { ready_, OnReady },         { hold_, OnHold },
                                     { log_, OnLogLine },         { state_, OnStateChange_ },
                                     { password_, OnPassword_ },  { proxy_, OnProxy_ },
                                     { stop_, OnStop_ },          { needok_, OnNeedOk_ },
                                     { needstr_, OnNeedStr_ },    { echo_, OnEcho },
                                     { bytecount_, OnByteCount }, { infomsg_, OnInfoMsg_ },
                                     { timeout_, OnTimeout },     { 0, NULL } };

    InitManagement(handler);
    dmsg(L"Init Management done");

    /* initialize options to default state */
    InitOptions(&o);
    o.session_semaphore = session_semaphore;

    dmsg(L"InitOptions done");

    GetRegistryKeys();

    dmsg(L"GetRegistryKeys done");

    /* Do not show status window by default */
    o.silent_connection = 1;
    o.disable_save_passwords = 1;
    o.disable_popup_messages = 1;
    o.enable_persistent = 1;
    /* In case we get queried for proxy, we currently support only system proxy */
    o.proxy_source = windows;

    /* Force scanning persistent connections -- we still need the service
     * running, but in case the service start-up is delayed, this helps
     * as we scan for profiles only once during a login session.
     * We expect users who register the PLAP dll to also enable the service.
     */
    o.service_state = service_connected;

    /* PLAP always uses QR */
    o.use_qr_for_url = TRUE;

    o.hInstance = hinstance;

    DWORD status = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (status)
    {
        MsgToEventLog(EVENTLOG_ERROR_TYPE, L"WSAStartup Failed with error = %lu", status);
        return status;
    }
    dmsg(L"WSAStartup Done");

    BuildFileList();
    dmsg(L"BuildFileList Done");

    dpi_initialize(&o);

    /* If openvpn service is not running but, available, attempt to start it */
    CheckServiceStatus();
    int num_persistent = 0;

    for (connection_t *c = o.chead; c; c = c->next)
    {
        if (c->flags & FLAG_DAEMON_PERSISTENT)
        {
            num_persistent++;
        }
    }

    if (o.service_state == service_disconnected && num_persistent > 0)
    {
        dmsg(L"Attempting to start automatic service");
        StartAutomaticService();
        CheckServiceStatus();
    }

    return 0;
}

/* Returns number of PLAP enabled configs -- for now
 * same as autostarted (persistent) connections.
 * The corresponding connection pointers are set in
 * the conn[] array
 */
DWORD
FindPLAPConnections(connection_t *conn[], size_t max_count)
{
    DWORD count = 0;
    for (connection_t *c = o.chead; c && count < max_count; c = c->next)
    {
        if (!(c->flags & FLAG_DAEMON_PERSISTENT) || !ParseManagementAddress(c))
        {
            continue;
        }
        conn[count++] = c;
    }
    return count;
}

static void
WaitOnThread(connection_t *c, DWORD timeout)
{
    HANDLE h = OpenThread(THREAD_ALL_ACCESS, FALSE, c->threadId);
    if (!h)
    {
        dmsg(L"Failed to get handle to the connection thread");
        goto out;
    }

    DWORD exit_code;
    if (WaitForSingleObject(h, timeout) == WAIT_OBJECT_0 && GetExitCodeThread(h, &exit_code)
        && exit_code != STILL_ACTIVE)
    {
        dmsg(L"Connection thread closed");
        goto out;
    }

    /* Kill the thread */
    dmsg(L"Force terminating a connection thread");
    TerminateThread(h, 1);
    c->hwndStatus = NULL;
    c->threadId = 0;

out:
    if (h && h != INVALID_HANDLE_VALUE)
    {
        CloseHandle(h);
    }
    return;
}

void
GetConnectionStatusText(connection_t *c, wchar_t *status, DWORD len)
{
    wchar_t status_text[256] = L"";

    /* blank status as a default */
    if (len > 0)
    {
        *status = '\0';
    }

    if (c->hwndStatus)
    {
        GetDlgItemText(c->hwndStatus, ID_TXT_STATUS, status_text, _countof(status_text));
        /* showing RECONNECTING while on hold is confusing, use status text */
        if ((strcmp(c->daemon_state, "RECONNECTING") == 0) && c->state == onhold && *status_text)
        {
            wcsncpy_s(status, len, status_text, _TRUNCATE);
        }
        else if (*c->daemon_state) /* this is more fine-grained and thus preferred */
        {
            LoadLocalizedStringBuf(status, len, daemon_state_resid(c->daemon_state));
        }
        else if (*status_text)
        {
            wcsncpy_s(status, len, status_text, _TRUNCATE);
        }
    }
}

void
SetParentWindow(HWND hwnd)
{
    o.hWnd = hwnd;
}

void
ShowStatusWindow(connection_t *c, BOOL show)
{
    if (c->hwndStatus)
    {
        /* Do not enable detach button in the PLAP mode */
        ShowWindowAsync(GetDlgItem(c->hwndStatus, ID_DETACH), SW_HIDE);
        /* Disconnecting from status Window gives no feedback to progress dialog.
         * Do not show the disconnect button.
         */
        ShowWindowAsync(GetDlgItem(c->hwndStatus, ID_DISCONNECT), SW_HIDE);
        ShowWindowAsync(c->hwndStatus, show ? SW_SHOW : SW_HIDE);
    }
}


void
DetachAllOpenVPN()
{
    int i;

    /* Detach from the mgmt i/f of all connections */
    for (connection_t *c = o.chead; c; c = c->next)
    {
        if (c->state != disconnected)
        {
            DetachOpenVPN(c);
        }
    }

    /* Wait for all connections to detach (Max 1 sec) */
    for (i = 0; i < 10; i++)
    {
        if (CountConnState(disconnected) == o.num_configs)
        {
            break;
        }
        OVPNMsgWait(100, NULL);
    }

    for (connection_t *c = o.chead; c; c = c->next)
    {
        if (c->hwndStatus)
        {
            /* Status thread still running? kill it */
            WaitOnThread(c, 0);
        }
    }
}

const wchar_t *
ConfigDisplayName(connection_t *c)
{
    return c->config_name;
}

int
ConnectionState(connection_t *c)
{
    return c->state;
}

void
DeleteUI(void)
{
    if (!o.hInstance) /* not initialized? */
    {
        dmsg(L"DeleteUI called before InitializeUI");
    }
    DetachAllOpenVPN();
    /* at this point all status threads have terminated -- we can safely free config list */
    FreeConfigList(&o);
    CloseSemaphore(o.session_semaphore);
    WSACleanup();
    memset(&o, 0, sizeof(o));
}

void
ConnectHelper(connection_t *c)
{
    if (c->state == disconnected || c->state == detached)
    {
        StartOpenVPN(c);
        dmsg(L"Calling StartOpenVPN on <%ls>", c->config_name);
    }
    else if (c->state == onhold)
    {
        ReleaseOpenVPN(c);
        dmsg(L"Calling ReleaseOpenVPN on <%ls>", c->config_name);
    }
}

void
DisconnectHelper(connection_t *c)
{
    if (c->state == disconnected || c->state == onhold
        || c->manage.connected < 2) /* mgmt not yet ready for input */
    {
        return;
    }

    /* disconnect will not work if disconnect button is disabled on the status
     * window -- enable it until out of here.
     */
    ShowWindowAsync(GetDlgItem(c->hwndStatus, ID_DISCONNECT), SW_SHOW);

    dmsg(L"sending stop");
    StopOpenVPN(c);

    /* wait up to a few sec for state to change */
    time_t timeout = time(NULL) + 5;
    while (timeout > time(NULL) && c->state != onhold && c->state != disconnected)
    {
        OVPNMsgWait(100, NULL);
    }

    ShowWindowAsync(GetDlgItem(c->hwndStatus, ID_DISCONNECT), SW_HIDE);

    dmsg(L"profile: %ls state = %d", c->config_name, c->state);
}

/* Set the currently active profile in Connect() operation -- pass NULL to unset.
 * We allow UI dialogs only on the current profile.
 */
void
SetActiveProfile(connection_t *c)
{
    active_profile = c;
}

int
RunProgressDialog(connection_t *c, PFTASKDIALOGCALLBACK cb_fn, LONG_PTR cb_data)
{
    dmsg(L"Entry with profile = <%ls>", c->config_name);

    TASKDIALOG_FLAGS flags =
        TDF_SHOW_MARQUEE_PROGRESS_BAR | TDF_CALLBACK_TIMER | TDF_USE_HICON_MAIN;
    wchar_t main_text[256];
    wchar_t details_btn_text[256];

    if (LangFlowDirection() == 1)
    {
        flags |= TDF_RTL_LAYOUT;
    }
    _sntprintf_0(main_text, L"%ls %ls", LoadLocalizedString(IDS_MENU_CONNECT), c->config_name);
    LoadLocalizedStringBuf(details_btn_text, _countof(details_btn_text), IDS_MENU_STATUS);

    const TASKDIALOG_BUTTON extra_buttons[] = {
        { status_menu_id, details_btn_text },
    };

    const TASKDIALOGCONFIG taskcfg = {
        .cbSize = sizeof(taskcfg),
        .hwndParent = o.hWnd,
        .hInstance = o.hInstance,
        .dwFlags = flags,
        .hMainIcon = LoadLocalizedIcon(ID_ICO_APP),
        .cButtons = _countof(extra_buttons),
        .pButtons = extra_buttons,
        .dwCommonButtons = TDCBF_CANCEL_BUTTON | TDCBF_RETRY_BUTTON,
        .pszWindowTitle = L"" PACKAGE_NAME " PLAP",
        .pszMainInstruction = main_text,
        .pszContent = L"Starting", /* updated in progress callback */
        .pfCallback = cb_fn,
        .lpCallbackData = cb_data,
    };

    int button_clicked = 0;
    dmsg(L"calling taskdialogindirect");
    TaskDialogIndirect(&taskcfg, &button_clicked, NULL, NULL);
    return button_clicked;
}
