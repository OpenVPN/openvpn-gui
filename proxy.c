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
#include <prsht.h>
#include <tchar.h>
#include <wininet.h>
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
    if (RegCreateKeyEx(HKEY_CURRENT_USER, GUI_REGKEY_HKCU, 0, _T(""), REG_OPTION_NON_VOLATILE,
                       KEY_WRITE, NULL, &regkey, &dwDispos) != ERROR_SUCCESS)
    {
        /* error creating Registry-Key */
        ShowLocalizedMsg(IDS_ERR_CREATE_REG_HKCU_KEY, GUI_REGKEY_HKCU);
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

    /* Open Registry for reading */
    status = RegOpenKeyEx(HKEY_CURRENT_USER, GUI_REGKEY_HKCU, 0, KEY_READ, &regkey);
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
    connection_t *c;

    switch (msg)
    {
    case WM_INITDIALOG:
        /* Set connection for this dialog */
        SetProp(hwndDlg, cfgProp, (HANDLE) lParam);
        SetForegroundWindow(hwndDlg);
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            c = (connection_t *) GetProp(hwndDlg, cfgProp);
            ManagementCommandFromInput(c, "username \"HTTP Proxy\" \"%s\"", hwndDlg, ID_EDT_PROXY_USER);
            ManagementCommandFromInput(c, "password \"HTTP Proxy\" \"%s\"", hwndDlg, ID_EDT_PROXY_PASS);
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
 * Construct the proxy options to append to the cmd-line.
 */
void ConstructProxyCmdLine(TCHAR *proxy_string, unsigned int size)
{
    proxy_string[0] = _T('\0');

    if (o.proxy_source == manual)
    {
        if (o.proxy_type == http)
        {
            __sntprintf_0(proxy_string, size, _T(" --http-proxy %s %s auto"),
                          o.proxy_http_address, o.proxy_http_port);
        }
        else if (o.proxy_type == socks)
        {
            __sntprintf_0(proxy_string, size, _T(" --socks-proxy %s %s"),
                          o.proxy_socks_address, o.proxy_socks_port);
        }
    }
    else if (o.proxy_source == windows)
    {
        __sntprintf_0(proxy_string, size, _T(" --auto-proxy"));
    }
}
