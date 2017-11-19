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
#include <windowsx.h>
#include <prsht.h>
#include <tchar.h>
#include <winhttp.h>
#include <stdlib.h>

#include "main.h"
#include "options.h"
#include "registry.h"
#include "proxy.h"
#include "openvpn-gui-res.h"
#include "localization.h"
#include "manage.h"
#include "openvpn.h"
#include "misc.h"

extern options_t o;

INT_PTR CALLBACK
ProxySettingsDialogFunc(HWND hwndDlg, UINT msg, WPARAM wParam, UNUSED LPARAM lParam)
{
    HICON hIcon;
    LPPSHNOTIFY psn;

    switch (msg)
    {
    case WM_INITDIALOG:
        hIcon = LoadLocalizedIcon(ID_ICO_APP);
        if (hIcon)
        {
            SendMessage(hwndDlg, WM_SETICON, (WPARAM) (ICON_SMALL), (LPARAM) (hIcon));
            SendMessage(hwndDlg, WM_SETICON, (WPARAM) (ICON_BIG), (LPARAM) (hIcon));
        }

        /* Limit Port editbox to 5 chars. */
        SendMessage(GetDlgItem(hwndDlg, ID_EDT_PROXY_PORT), EM_SETLIMITTEXT, 5, 0);

        LoadProxySettings(hwndDlg);
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case ID_RB_PROXY_OPENVPN:
            if (HIWORD(wParam) == BN_CLICKED)
            {
                EnableWindow(GetDlgItem(hwndDlg, ID_RB_PROXY_HTTP), FALSE);
                EnableWindow(GetDlgItem(hwndDlg, ID_RB_PROXY_SOCKS), FALSE);
                EnableWindow(GetDlgItem(hwndDlg, ID_EDT_PROXY_ADDRESS), FALSE);
                EnableWindow(GetDlgItem(hwndDlg, ID_EDT_PROXY_PORT), FALSE);
                EnableWindow(GetDlgItem(hwndDlg, ID_TXT_PROXY_ADDRESS), FALSE);
                EnableWindow(GetDlgItem(hwndDlg, ID_TXT_PROXY_PORT), FALSE);
            }
            break;

        case ID_RB_PROXY_MSIE:
            if (HIWORD(wParam) == BN_CLICKED)
            {
                EnableWindow(GetDlgItem(hwndDlg, ID_RB_PROXY_HTTP), FALSE);
                EnableWindow(GetDlgItem(hwndDlg, ID_RB_PROXY_SOCKS), FALSE);
                EnableWindow(GetDlgItem(hwndDlg, ID_EDT_PROXY_ADDRESS), FALSE);
                EnableWindow(GetDlgItem(hwndDlg, ID_EDT_PROXY_PORT), FALSE);
                EnableWindow(GetDlgItem(hwndDlg, ID_TXT_PROXY_ADDRESS), FALSE);
                EnableWindow(GetDlgItem(hwndDlg, ID_TXT_PROXY_PORT), FALSE);
            }
            break;

        case ID_RB_PROXY_MANUAL:
            if (HIWORD(wParam) == BN_CLICKED)
            {
                EnableWindow(GetDlgItem(hwndDlg, ID_RB_PROXY_HTTP), TRUE);
                EnableWindow(GetDlgItem(hwndDlg, ID_RB_PROXY_SOCKS), TRUE);
                EnableWindow(GetDlgItem(hwndDlg, ID_EDT_PROXY_ADDRESS), TRUE);
                EnableWindow(GetDlgItem(hwndDlg, ID_EDT_PROXY_PORT), TRUE);
                EnableWindow(GetDlgItem(hwndDlg, ID_TXT_PROXY_ADDRESS), TRUE);
                EnableWindow(GetDlgItem(hwndDlg, ID_TXT_PROXY_PORT), TRUE);
            }
            break;

        case ID_RB_PROXY_HTTP:
            if (HIWORD(wParam) == BN_CLICKED)
            {
                SetDlgItemText(hwndDlg, ID_EDT_PROXY_ADDRESS, o.proxy_http_address);
                SetDlgItemText(hwndDlg, ID_EDT_PROXY_PORT, o.proxy_http_port);
            }
            break;

        case ID_RB_PROXY_SOCKS:
            if (HIWORD(wParam) == BN_CLICKED)
            {
                SetDlgItemText(hwndDlg, ID_EDT_PROXY_ADDRESS, o.proxy_socks_address);
                SetDlgItemText(hwndDlg, ID_EDT_PROXY_PORT, o.proxy_socks_port);
            }
            break;
        }
        break;

    case WM_NOTIFY:
        psn = (LPPSHNOTIFY) lParam;
        if (psn->hdr.code == (UINT) PSN_KILLACTIVE)
        {
            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (CheckProxySettings(hwndDlg) ? FALSE : TRUE));
            return TRUE;
        }
        else if (psn->hdr.code == (UINT) PSN_APPLY)
        {
            SaveProxySettings(hwndDlg);
            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
            return TRUE;
        }
        break;

    case WM_CLOSE:
        EndDialog(hwndDlg, LOWORD(wParam));
        return TRUE;
    }
    return FALSE;
}


/* Check that proxy settings are valid */
int
CheckProxySettings(HWND hwndDlg)
{
    if (IsDlgButtonChecked(hwndDlg, ID_RB_PROXY_MANUAL) == BST_CHECKED)
    {
        TCHAR text[100];
        BOOL http = (IsDlgButtonChecked(hwndDlg, ID_RB_PROXY_HTTP) == BST_CHECKED);

        GetDlgItemText(hwndDlg, ID_EDT_PROXY_ADDRESS, text, _countof(text));
        if (_tcslen(text) == 0)
        {
            /* proxy address not specified */
            ShowLocalizedMsg((http ? IDS_ERR_HTTP_PROXY_ADDRESS : IDS_ERR_SOCKS_PROXY_ADDRESS));
            return 0;
        }

        GetDlgItemText(hwndDlg, ID_EDT_PROXY_PORT, text, _countof(text));
        if (_tcslen(text) == 0)
        {
            /* proxy port not specified */
            ShowLocalizedMsg((http ? IDS_ERR_HTTP_PROXY_PORT : IDS_ERR_SOCKS_PROXY_PORT));
            return 0;
        }

        long port = _tcstol(text, NULL, 10);
        if ((port < 1) || (port > 65535))
        {
            /* proxy port range error */
            ShowLocalizedMsg((http ? IDS_ERR_HTTP_PROXY_PORT_RANGE : IDS_ERR_SOCKS_PROXY_PORT_RANGE));
            return 0;
        }
    }

    return 1;
}


void
LoadProxySettings(HWND hwndDlg)
{
    /* Set Proxy type, address and port */
    if (o.proxy_type == http)
    {
        CheckRadioButton(hwndDlg, ID_RB_PROXY_HTTP, ID_RB_PROXY_SOCKS, ID_RB_PROXY_HTTP);
        SetDlgItemText(hwndDlg, ID_EDT_PROXY_ADDRESS, o.proxy_http_address);
        SetDlgItemText(hwndDlg, ID_EDT_PROXY_PORT, o.proxy_http_port);
    }
    else if (o.proxy_type == socks)
    {
        CheckRadioButton(hwndDlg, ID_RB_PROXY_HTTP, ID_RB_PROXY_SOCKS, ID_RB_PROXY_SOCKS);
        SetDlgItemText(hwndDlg, ID_EDT_PROXY_ADDRESS, o.proxy_socks_address);
        SetDlgItemText(hwndDlg, ID_EDT_PROXY_PORT, o.proxy_socks_port);
    }

    /* Set Proxy Settings Source */
    if (o.proxy_source == config)
    {
        SendMessage(GetDlgItem(hwndDlg, ID_RB_PROXY_OPENVPN), BM_CLICK, 0, 0);
    }
    else if (o.proxy_source == windows)
    {
        SendMessage(GetDlgItem(hwndDlg, ID_RB_PROXY_MSIE), BM_CLICK, 0, 0);
    }
    else if (o.proxy_source == manual)
    {
        SendMessage(GetDlgItem(hwndDlg, ID_RB_PROXY_MANUAL), BM_CLICK, 0, 0);
    }
}


void
SaveProxySettings(HWND hwndDlg)
{
    HKEY regkey;
    DWORD dwDispos;
    TCHAR proxy_source_string[2] = _T("0");
    TCHAR proxy_type_string[2] = _T("0");
    TCHAR proxy_subkey[MAX_PATH];

    /* Save Proxy Settings Source */
    if (IsDlgButtonChecked(hwndDlg, ID_RB_PROXY_OPENVPN) == BST_CHECKED)
    {
        o.proxy_source = config;
        proxy_source_string[0] = _T('0');
    }
    else if (IsDlgButtonChecked(hwndDlg, ID_RB_PROXY_MSIE) == BST_CHECKED)
    {
        o.proxy_source = windows;
        proxy_source_string[0] = _T('1');
    }
    else if (IsDlgButtonChecked(hwndDlg, ID_RB_PROXY_MANUAL) == BST_CHECKED)
    {
        o.proxy_source = manual;
        proxy_source_string[0] = _T('2');
    }

    /* Save Proxy type, address and port */
    if (IsDlgButtonChecked(hwndDlg, ID_RB_PROXY_HTTP) == BST_CHECKED)
    {
        o.proxy_type = http;
        proxy_type_string[0] = _T('0');

        GetDlgItemText(hwndDlg, ID_EDT_PROXY_ADDRESS, o.proxy_http_address,
                    _countof(o.proxy_http_address));
        GetDlgItemText(hwndDlg, ID_EDT_PROXY_PORT, o.proxy_http_port,
                    _countof(o.proxy_http_port));
    }
    else
    {
        o.proxy_type = socks;
        proxy_type_string[0] = _T('1');

        GetDlgItemText(hwndDlg, ID_EDT_PROXY_ADDRESS, o.proxy_socks_address,
                    _countof(o.proxy_socks_address));
        GetDlgItemText(hwndDlg, ID_EDT_PROXY_PORT, o.proxy_socks_port,
                    _countof(o.proxy_socks_port));
    }

    /* Open Registry for writing */
    _sntprintf_0(proxy_subkey, _T("%s\\proxy"), GUI_REGKEY_HKCU);
    if (RegCreateKeyEx(HKEY_CURRENT_USER, proxy_subkey, 0, _T(""), REG_OPTION_NON_VOLATILE,
                       KEY_WRITE, NULL, &regkey, &dwDispos) != ERROR_SUCCESS)
    {
        /* error creating Registry-Key */
        ShowLocalizedMsg(IDS_ERR_CREATE_REG_HKCU_KEY, proxy_subkey);
        return;
    }

    /* Save Settings to registry */
    SetRegistryValue(regkey, _T("proxy_source"), proxy_source_string);
    SetRegistryValue(regkey, _T("proxy_type"), proxy_type_string);
    SetRegistryValue(regkey, _T("proxy_http_address"), o.proxy_http_address);
    SetRegistryValue(regkey, _T("proxy_http_port"), o.proxy_http_port);
    SetRegistryValue(regkey, _T("proxy_socks_address"), o.proxy_socks_address);
    SetRegistryValue(regkey, _T("proxy_socks_port"), o.proxy_socks_port);

    RegCloseKey(regkey);
}


void
GetProxyRegistrySettings()
{
    LONG status;
    HKEY regkey;
    TCHAR proxy_source_string[2] = _T("0");
    TCHAR proxy_type_string[2] = _T("0");
    TCHAR proxy_subkey[MAX_PATH];

    /* Open Registry for reading */
    _sntprintf_0(proxy_subkey, _T("%s\\proxy"), GUI_REGKEY_HKCU);
    status = RegOpenKeyEx(HKEY_CURRENT_USER, proxy_subkey, 0, KEY_READ, &regkey);
    if (status != ERROR_SUCCESS)
        return;

    /* get registry settings */
    GetRegistryValue(regkey, _T("proxy_http_address"), o.proxy_http_address, _countof(o.proxy_http_address));
    GetRegistryValue(regkey, _T("proxy_http_port"), o.proxy_http_port, _countof(o.proxy_http_port));
    GetRegistryValue(regkey, _T("proxy_socks_address"), o.proxy_socks_address, _countof(o.proxy_socks_address));
    GetRegistryValue(regkey, _T("proxy_socks_port"), o.proxy_socks_port, _countof(o.proxy_socks_port));
    GetRegistryValue(regkey, _T("proxy_source"), proxy_source_string, _countof(proxy_source_string));
    GetRegistryValue(regkey, _T("proxy_type"), proxy_type_string, _countof(proxy_type_string));

    if (proxy_source_string[0] == _T('0'))
    {
        o.proxy_source = config;
    }
    else if (proxy_source_string[0] == _T('1'))
    {
        o.proxy_source = windows;
    }
    else if (proxy_source_string[0] == _T('2'))
    {
        o.proxy_source = manual;
    }

    o.proxy_type = (proxy_type_string[0] == _T('0') ? http : socks);

    RegCloseKey(regkey);
}


INT_PTR CALLBACK
ProxyAuthDialogFunc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LPCSTR proxy_type;
    connection_t *c;
    char fmt[32];

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
        switch (LOWORD(wParam))
        {
        case ID_EDT_PROXY_USER:
            if (HIWORD(wParam) == EN_UPDATE)
            {
                int len = Edit_GetTextLength((HWND) lParam);
                EnableWindow(GetDlgItem(hwndDlg, IDOK), (len ? TRUE : FALSE));
            }
            break;

        case IDOK:
            c = (connection_t *) GetProp(hwndDlg, cfgProp);
            proxy_type = (c->proxy_type == http ? "HTTP" : "SOCKS");

            snprintf(fmt, sizeof(fmt), "username \"%s Proxy\" \"%%s\"", proxy_type);
            ManagementCommandFromInput(c, fmt, hwndDlg, ID_EDT_PROXY_USER);

            snprintf(fmt, sizeof(fmt), "password \"%s Proxy\" \"%%s\"", proxy_type);
            ManagementCommandFromInput(c, fmt, hwndDlg, ID_EDT_PROXY_PASS);

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


void
QueryProxyAuth(connection_t *c, proxy_t type)
{
    c->proxy_type = type;
    LocalizedDialogBoxParam(ID_DLG_PROXY_AUTH, ProxyAuthDialogFunc, (LPARAM) c);
}


typedef enum { HTTPS_URL, SOCKS_URL } url_scheme;
static LPCWSTR
UrlSchemeStr(const url_scheme scheme)
{
    static LPCWSTR scheme_strings[] = { L"https", L"socks" };
    return scheme_strings[scheme];
}


static LPWSTR
QueryWindowsProxySettings(const url_scheme scheme, LPCSTR host)
{
    LPWSTR proxy = NULL;
    BOOL auto_detect = TRUE;
    LPWSTR auto_config_url = NULL;
    WINHTTP_CURRENT_USER_IE_PROXY_CONFIG proxy_config;

    if (WinHttpGetIEProxyConfigForCurrentUser(&proxy_config))
    {
        proxy = proxy_config.lpszProxy;
        auto_detect = proxy_config.fAutoDetect;
        auto_config_url = proxy_config.lpszAutoConfigUrl;
        GlobalFree(proxy_config.lpszProxyBypass);
    }

    if (auto_detect)
    {
        LPWSTR old_url = auto_config_url;
        DWORD flags = WINHTTP_AUTO_DETECT_TYPE_DHCP | WINHTTP_AUTO_DETECT_TYPE_DNS_A;
        if (WinHttpDetectAutoProxyConfigUrl(flags, &auto_config_url))
            GlobalFree(old_url);
    }

    if (auto_config_url)
    {
        HINTERNET session = WinHttpOpen(NULL, WINHTTP_ACCESS_TYPE_NO_PROXY,
            WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
        if (session)
        {
            int size = _snwprintf(NULL, 0, L"%s://%S", UrlSchemeStr(scheme), host) + 1;
            LPWSTR url = malloc(size * sizeof(WCHAR));
            if (url)
            {
                _snwprintf(url, size, L"%s://%S", UrlSchemeStr(scheme), host);

                LPWSTR old_proxy = proxy;
                WINHTTP_PROXY_INFO proxy_info;
                WINHTTP_AUTOPROXY_OPTIONS options = {
                    .fAutoLogonIfChallenged = TRUE,
                    .dwFlags = WINHTTP_AUTOPROXY_CONFIG_URL,
                    .lpszAutoConfigUrl = auto_config_url,
                    .dwAutoDetectFlags = 0,
                    .lpvReserved = NULL,
                    .dwReserved = 0
                };

                if (WinHttpGetProxyForUrl(session, url, &options, &proxy_info))
                {
                    GlobalFree(old_proxy);
                    GlobalFree(proxy_info.lpszProxyBypass);
                    proxy = proxy_info.lpszProxy;
                }
                free(url);
            }
            WinHttpCloseHandle(session);
        }
        GlobalFree(auto_config_url);
    }

    return proxy;
}


static VOID
ParseProxyString(LPWSTR proxy_str, url_scheme scheme,
                 LPCSTR *type, LPCWSTR *host, LPCWSTR *port)
{
    if (proxy_str == NULL)
        return;

    LPCWSTR delim = L"; ";
    LPWSTR token = wcstok(proxy_str, delim);

    LPCWSTR scheme_str = UrlSchemeStr(scheme);
    LPCWSTR socks_str = UrlSchemeStr(SOCKS_URL);

    /* Token format: [<scheme>=][<scheme>"://"]<server>[":"<port>] */
    while (token)
    {
        BOOL match = FALSE;
        LPWSTR eq = wcschr(token, '=');
        LPWSTR css = wcsstr(token, L"://");

        /*
         * If the token has a <scheme>, test for the one we're looking for.
         * If we're looking for a https proxy, socks will also do.
         * If it's a proxy without a <scheme> it's only good for https.
         */
        if (eq || css)
        {
            if (wcsbegins(token, scheme_str))
            {
                match = TRUE;
            }
            else if (scheme == HTTPS_URL && wcsbegins(token, socks_str))
            {
                match = TRUE;
                scheme = SOCKS_URL;
            }
        }
        else if (scheme == HTTPS_URL)
        {
            match = TRUE;
        }

        if (match)
        {
            LPWSTR server = token;
            if (css)
                server = css + 3;
            else if (eq)
                server = eq + 1;

            /* IPv6 addresses are surrounded by brackets */
            LPWSTR port_delim;
            if (server[0] == '[')
            {
                server += 1;
                LPWSTR end = wcschr(server, ']');
                if (end == NULL)
                    continue;
                *end++ = '\0';

                port_delim = (*end == ':' ? end : NULL);
            }
            else
            {
                port_delim = wcsrchr(server, ':');
                if (port_delim)
                    *port_delim = '\0';
            }

            *type = (scheme == HTTPS_URL ? "HTTP" : "SOCKS");
            *host = server;
            if (port_delim)
                *port = port_delim + 1;
            else
                *port = (scheme == HTTPS_URL ? L"80": L"1080");

            break;
        }
        token = wcstok(NULL, delim);
    }
}


/*
 * Respond to management interface PROXY notifications
 * Input format: REMOTE_NO,PROTOCOL,HOST
 */
void
OnProxy(connection_t *c, char *line)
{
    LPSTR proto, host;
    char *pos = strchr(line, ',');
    if (pos == NULL)
        return;

    proto = ++pos;
    pos = strchr(pos, ',');
    if (pos == NULL)
        return;

    *pos = '\0';
    host = ++pos;
    if (host[0] == '\0')
        return;

    LPCSTR type = "NONE";
    LPCWSTR addr = L"", port = L"";
    LPWSTR proxy_str = NULL;

    if (o.proxy_source == manual)
    {
        if (o.proxy_type == http && streq(proto, "TCP"))
        {
            type = "HTTP";
            addr = o.proxy_http_address;
            port = o.proxy_http_port;
        }
        else if (o.proxy_type == socks)
        {
            type = "SOCKS";
            addr = o.proxy_socks_address;
            port = o.proxy_socks_port;
        }
    }
    else if (o.proxy_source == windows)
    {
        url_scheme scheme = (streq(proto, "TCP") ? HTTPS_URL : SOCKS_URL);
        proxy_str = QueryWindowsProxySettings(scheme, host);
        ParseProxyString(proxy_str, scheme, &type, &addr, &port);
    }

    char cmd[128];
    snprintf(cmd, sizeof(cmd), "proxy %s %ls %ls", type, addr, port);
    cmd[sizeof(cmd) - 1] = '\0';
    ManagementCommand(c, cmd, NULL, regular);

    GlobalFree(proxy_str);
}
