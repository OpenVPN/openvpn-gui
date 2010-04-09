/*
 *  OpenVPN-GUI -- A Windows GUI for OpenVPN.
 *
 *  Copyright (C) 2004 Mathias Sundman <mathias@nilings.se>
 *                2009 Heiko Hund <heikoh@users.sf.net>
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

#include <windows.h>
#include <prsht.h>
#include <tchar.h>
#include <WinInet.h>

#include "config.h"
#include "main.h"
#include "options.h"
#include "registry.h"
#include "proxy.h"
#include "openvpn-gui-res.h"
#include "localization.h"

extern options_t o;

bool CALLBACK ProxySettingsDialogFunc (HWND hwndDlg, UINT msg, WPARAM wParam, UNUSED LPARAM lParam)
{
  HICON hIcon;
  LPPSHNOTIFY psn;

  switch (msg) {

    case WM_INITDIALOG:
      hIcon = LoadLocalizedIcon(ID_ICO_APP);
      if (hIcon) {
        SendMessage(hwndDlg, WM_SETICON, (WPARAM) (ICON_SMALL), (LPARAM) (hIcon));
        SendMessage(hwndDlg, WM_SETICON, (WPARAM) (ICON_BIG), (LPARAM) (hIcon));
      }

      /* Limit Port editbox to 5 chars. */
      SendMessage(GetDlgItem(hwndDlg, ID_EDT_PROXY_PORT), EM_SETLIMITTEXT, 5, 0);

      LoadProxySettings(hwndDlg);
      break;

    case WM_COMMAND:
      switch (LOWORD(wParam)) {

        case ID_RB_PROXY_OPENVPN:
          if (HIWORD(wParam) == BN_CLICKED)
            {
              EnableWindow(GetDlgItem(hwndDlg, ID_RB_PROXY_HTTP), FALSE);
              EnableWindow(GetDlgItem(hwndDlg, ID_RB_PROXY_SOCKS), FALSE);
              EnableWindow(GetDlgItem(hwndDlg, ID_EDT_PROXY_ADDRESS), FALSE);
              EnableWindow(GetDlgItem(hwndDlg, ID_EDT_PROXY_PORT), FALSE);
              EnableWindow(GetDlgItem(hwndDlg, ID_TXT_PROXY_ADDRESS), FALSE);
              EnableWindow(GetDlgItem(hwndDlg, ID_TXT_PROXY_PORT), FALSE);
              EnableWindow(GetDlgItem(hwndDlg, ID_CB_PROXY_AUTH), FALSE);
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
              EnableWindow(GetDlgItem(hwndDlg, ID_CB_PROXY_AUTH), TRUE);
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
              EnableWindow(GetDlgItem(hwndDlg, ID_CB_PROXY_AUTH),
                (IsDlgButtonChecked(hwndDlg, ID_RB_PROXY_HTTP) == BST_CHECKED ? TRUE : FALSE));
            }
          break;

        case ID_RB_PROXY_HTTP:
          if (HIWORD(wParam) == BN_CLICKED)
            {
              EnableWindow(GetDlgItem(hwndDlg, ID_CB_PROXY_AUTH), TRUE);
              SetDlgItemText(hwndDlg, ID_EDT_PROXY_ADDRESS, o.proxy_http_address);
              SetDlgItemText(hwndDlg, ID_EDT_PROXY_PORT, o.proxy_http_port);
            }
          break;

        case ID_RB_PROXY_SOCKS:
          if (HIWORD(wParam) == BN_CLICKED)
            {
              EnableWindow(GetDlgItem(hwndDlg, ID_CB_PROXY_AUTH), FALSE);
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
          SetWindowLong(hwndDlg, DWL_MSGRESULT,
            ( CheckProxySettings(hwndDlg) ? FALSE : TRUE ));
          return TRUE;
        }
      else if (psn->hdr.code == (UINT) PSN_APPLY)
        {
          SaveProxySettings(hwndDlg);
          SetWindowLong(hwndDlg, DWL_MSGRESULT, PSNRET_NOERROR);
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
int CheckProxySettings(HWND hwndDlg)
{
  if (IsDlgButtonChecked(hwndDlg, ID_RB_PROXY_MANUAL) == BST_CHECKED)
    {
      TCHAR text[100];
      BOOL http = (IsDlgButtonChecked(hwndDlg, ID_RB_PROXY_HTTP) == BST_CHECKED);

      GetDlgItemText(hwndDlg, ID_EDT_PROXY_ADDRESS, text, _tsizeof(text));
      if (_tcslen(text) == 0)
        {
          /* proxy address not specified */
          ShowLocalizedMsg((http ? IDS_ERR_HTTP_PROXY_ADDRESS : IDS_ERR_SOCKS_PROXY_ADDRESS));
          return(0);
        }

      GetDlgItemText(hwndDlg, ID_EDT_PROXY_PORT, text, _tsizeof(text));
      if (_tcslen(text) == 0)
        {
          /* proxy port not specified */
          ShowLocalizedMsg((http ? IDS_ERR_HTTP_PROXY_PORT : IDS_ERR_SOCKS_PROXY_PORT));
          return(0);
        }

      long port = _tcstol(text, NULL, 10);
      if ((port < 1) || (port > 65535))
        {
          /* proxy port range error */
          ShowLocalizedMsg((http ? IDS_ERR_HTTP_PROXY_PORT_RANGE : IDS_ERR_SOCKS_PROXY_PORT_RANGE));
          return(0);
        }
    }

  return(1);
}

void LoadProxySettings(HWND hwndDlg)
{
  /* Set Proxy type, address and port */
  if (o.proxy_type == http)  /* HTTP Proxy */
    {
      CheckRadioButton(hwndDlg, ID_RB_PROXY_HTTP, ID_RB_PROXY_SOCKS, ID_RB_PROXY_HTTP);
      SetDlgItemText(hwndDlg, ID_EDT_PROXY_ADDRESS, o.proxy_http_address);
      SetDlgItemText(hwndDlg, ID_EDT_PROXY_PORT, o.proxy_http_port);
    }
  else			  /* SOCKS Proxy */
    {
      CheckRadioButton(hwndDlg, ID_RB_PROXY_HTTP, ID_RB_PROXY_SOCKS, ID_RB_PROXY_SOCKS);
      SetDlgItemText(hwndDlg, ID_EDT_PROXY_ADDRESS, o.proxy_socks_address);
      SetDlgItemText(hwndDlg, ID_EDT_PROXY_PORT, o.proxy_socks_port);
    }

  if (o.proxy_http_auth) CheckDlgButton(hwndDlg, ID_CB_PROXY_AUTH, BST_CHECKED);

  /* Set Proxy Settings Source */
  if (o.proxy_source == config)
    SendMessage(GetDlgItem(hwndDlg, ID_RB_PROXY_OPENVPN), BM_CLICK, 0, 0);
  if (o.proxy_source == browser)
    SendMessage(GetDlgItem(hwndDlg, ID_RB_PROXY_MSIE), BM_CLICK, 0, 0);
  if (o.proxy_source == manual)
    SendMessage(GetDlgItem(hwndDlg, ID_RB_PROXY_MANUAL), BM_CLICK, 0, 0);
}

void SaveProxySettings(HWND hwndDlg)
{
  HKEY regkey;
  DWORD dwDispos;
  TCHAR proxy_source_string[2] = _T("0");
  TCHAR proxy_type_string[2] = _T("0");
  TCHAR proxy_http_auth_string[2] = _T("0");

  /* Save Proxy Settings Source */
  if (IsDlgButtonChecked(hwndDlg, ID_RB_PROXY_OPENVPN) == BST_CHECKED)
    {
      o.proxy_source = config;
      proxy_source_string[0] = _T('0');
    }
  if (IsDlgButtonChecked(hwndDlg, ID_RB_PROXY_MSIE) == BST_CHECKED)
    {
      o.proxy_source = browser;
      proxy_source_string[0] = _T('1');
    }
  if (IsDlgButtonChecked(hwndDlg, ID_RB_PROXY_MANUAL) == BST_CHECKED)
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
                 _tsizeof(o.proxy_http_address));
      GetDlgItemText(hwndDlg, ID_EDT_PROXY_PORT, o.proxy_http_port, 
                 _tsizeof(o.proxy_http_port));

      BOOL auth = (IsDlgButtonChecked(hwndDlg, ID_CB_PROXY_AUTH) == BST_CHECKED);
      o.proxy_http_auth = (auth ? 1 : 0);
      proxy_http_auth_string[0] = (auth ? _T('1') : _T('0'));
    }
  else
    {
      o.proxy_type = socks;
      proxy_type_string[0] = _T('1');

      GetDlgItemText(hwndDlg, ID_EDT_PROXY_ADDRESS, o.proxy_socks_address, 
                 _tsizeof(o.proxy_socks_address));
      GetDlgItemText(hwndDlg, ID_EDT_PROXY_PORT, o.proxy_socks_port, 
                 _tsizeof(o.proxy_socks_port));
    }

  /* Open Registry for writing */
  if (RegCreateKeyEx(HKEY_CURRENT_USER,
                    GUI_REGKEY_HKCU,
                    0,
                    _T(""),
                    REG_OPTION_NON_VOLATILE,
                    KEY_WRITE,
                    NULL,
                    &regkey,
                    &dwDispos) != ERROR_SUCCESS)
    {
      /* error creating Registry-Key */
      ShowLocalizedMsg(IDS_ERR_CREATE_REG_HKCU_KEY, GUI_REGKEY_HKCU);
      return;
    }

  /* Save Settings to registry */
  SetRegistryValue(regkey, _T("proxy_source"), proxy_source_string);
  SetRegistryValue(regkey, _T("proxy_type"), proxy_type_string);
  SetRegistryValue(regkey, _T("proxy_http_auth"), proxy_http_auth_string);
  SetRegistryValue(regkey, _T("proxy_http_address"), o.proxy_http_address);
  SetRegistryValue(regkey, _T("proxy_http_port"), o.proxy_http_port);
  SetRegistryValue(regkey, _T("proxy_socks_address"), o.proxy_socks_address);
  SetRegistryValue(regkey, _T("proxy_socks_port"), o.proxy_socks_port);

  RegCloseKey(regkey);
}

void GetProxyRegistrySettings()
{
  LONG status;
  HKEY regkey;
  TCHAR proxy_source_string[2] = _T("0");
  TCHAR proxy_type_string[2] = _T("0");
  TCHAR proxy_http_auth_string[2] = _T("0");
  TCHAR temp_path[100];

  /* Construct Authfile path */
  if (!GetTempPath(_tsizeof(temp_path) - 1, temp_path))
    {
      /* Error getting TempPath - using C:\ */
      ShowLocalizedMsg(IDS_ERR_GET_TEMP_PATH);
      _tcscpy(temp_path, _T("C:\\"));
    }
  _tcsncat(temp_path, _T("openvpn_authfile.txt"), 
          _tsizeof(temp_path) - _tcslen(temp_path) - 1); 
  _tcsncpy(o.proxy_authfile, temp_path, _tsizeof(o.proxy_authfile));

  /* Open Registry for reading */
  status = RegOpenKeyEx(HKEY_CURRENT_USER,
                       GUI_REGKEY_HKCU,
                       0,
                       KEY_READ,
                       &regkey);

  /* Return if can't open the registry key */
  if (status != ERROR_SUCCESS) return;
      

  /* get registry settings */
  GetRegistryValue(regkey, _T("proxy_http_address"), o.proxy_http_address, 
                   _tsizeof(o.proxy_http_address));
  GetRegistryValue(regkey, _T("proxy_http_port"), o.proxy_http_port, 
                   _tsizeof(o.proxy_http_port));
  GetRegistryValue(regkey, _T("proxy_socks_address"), o.proxy_socks_address, 
                   _tsizeof(o.proxy_socks_address));
  GetRegistryValue(regkey, _T("proxy_socks_port"), o.proxy_socks_port, 
                   _tsizeof(o.proxy_socks_port));
  GetRegistryValue(regkey, _T("proxy_source"), proxy_source_string, 
                   _tsizeof(proxy_source_string));
  GetRegistryValue(regkey, _T("proxy_type"), proxy_type_string, 
                   _tsizeof(proxy_type_string));
  GetRegistryValue(regkey, _T("proxy_http_auth"), proxy_http_auth_string, 
                   _tsizeof(proxy_http_auth_string));

  if (proxy_source_string[0] == _T('1'))
    o.proxy_source = config;
  if (proxy_source_string[0] == _T('2'))
    o.proxy_source = browser;
  if (proxy_source_string[0] == _T('3'))
    o.proxy_source = manual;

  if (proxy_type_string[0] == _T('1'))
    o.proxy_type = socks;

  if (proxy_http_auth_string[0] == _T('1'))
    o.proxy_http_auth = TRUE;

  RegCloseKey(regkey);
}

BOOL CALLBACK ProxyAuthDialogFunc (HWND hwndDlg, UINT msg, WPARAM wParam, UNUSED LPARAM lParam)
{
  TCHAR username[50];
  TCHAR password[50];
  FILE *fp;

  switch (msg) {

    case WM_INITDIALOG:
      //SetForegroundWindow(hwndDlg);
      break;

    case WM_COMMAND:
      switch (LOWORD(wParam)) {

        case IDOK:
          GetDlgItemText(hwndDlg, ID_EDT_PROXY_USER, username, _tsizeof(username) - 1);
          GetDlgItemText(hwndDlg, ID_EDT_PROXY_PASS, password, _tsizeof(password) - 1);
          if (!(fp = _tfopen(o.proxy_authfile, _T("w"))))
            {
              /* error creating AUTH file */
              ShowLocalizedMsg(IDS_ERR_CREATE_AUTH_FILE, o.proxy_authfile);
              EndDialog(hwndDlg, LOWORD(wParam));
            }
          _fputts(username, fp);
          _fputts(_T("\n"), fp);
          _fputts(password, fp);
          _fputts(_T("\n"), fp);
          fclose(fp);
          EndDialog(hwndDlg, LOWORD(wParam));
          return TRUE;
      }
      break;
    case WM_CLOSE:
      EndDialog(hwndDlg, LOWORD(wParam));
      return TRUE;
     
  }
  return FALSE;
}

/*
 * GetIeHttpProxy() fetches the current IE proxy settings for HTTP.
 * Coded by Ewan Bhamrah Harley <code@ewan.info>, but refactored.
 *
 * If result string does not contain an '=' sign then there is a
 * single proxy for all protocols. If multiple proxies are defined
 * they are space seperated and have the form protocol=host[:port]
 */
static BOOL
GetIeHttpProxy(TCHAR *host, size_t *hostlen, TCHAR *port, size_t *portlen)
{
  INTERNET_PROXY_INFO pinfo;
  DWORD psize = sizeof(pinfo);
  const TCHAR *start, *colon, *space, *end;
  const size_t _hostlen = *hostlen;
  const size_t _portlen = *portlen;

  *hostlen = 0;
  *portlen = 0;

  if (!InternetQueryOption(NULL, INTERNET_OPTION_PROXY, (LPVOID) &pinfo, &psize))
  {
    ShowLocalizedMsg(IDS_ERR_GET_MSIE_PROXY);
    return FALSE;
  }

  /* Check if a proxy is used */
  if (pinfo.dwAccessType != INTERNET_OPEN_TYPE_PROXY)
    return TRUE;

  /* Check if a HTTP proxy is defined */
  start = _tcsstr(pinfo.lpszProxy, _T("http="));
  if (start == NULL && _tcschr(pinfo.lpszProxy, _T('=')))
    return TRUE;

  /* Get the pointers needed for string extraction */
  start = (start ? start + 5 : pinfo.lpszProxy);
  colon = _tcschr(start, _T(':'));
  space = _tcschr(start, _T(' '));

  /* Extract hostname */
  end = (colon ? colon : space);
  if (end)
    *hostlen = end - start;
  else
    *hostlen = _tcslen(start);

  if (_hostlen < *hostlen)
    return FALSE;

  _tcsncpy(host, start, *hostlen);

  if (!colon)
    return TRUE;

  start = colon + 1;
  end = space;
  if (end)
    *portlen = end - start;
  else
    *portlen = _tcslen(start);

  if (_portlen < *portlen)
    return FALSE;

  _tcsncpy(port, start, *portlen);

  return TRUE;
}

/*
 * Construct the proxy options to append to the cmd-line.
 */
void ConstructProxyCmdLine(TCHAR *proxy_string_ptr, unsigned int size)
{
  TCHAR proxy_string[100];

  CLEAR(proxy_string);

  if (o.proxy_source == manual)
    {
      if (o.proxy_type == http)
        {
          if (o.proxy_http_auth)
            {
              /* Ask for Proxy username/password */
              LocalizedDialogBox(ID_DLG_PROXY_AUTH, ProxyAuthDialogFunc);
              _sntprintf_0(proxy_string, _T("--http-proxy %s %s %s"),
                           o.proxy_http_address,
                           o.proxy_http_port,
                           o.proxy_authfile);
            }
          else
            {
              _sntprintf_0(proxy_string, _T("--http-proxy %s %s"),
                           o.proxy_http_address,
                           o.proxy_http_port);
            }
        }
      if (o.proxy_type == socks)
        {
          _sntprintf_0(proxy_string, _T("--socks-proxy %s %s"),
                       o.proxy_socks_address,
                       o.proxy_socks_port);
        }
    }

  else if (o.proxy_source == browser)
    {
      TCHAR host[64];
      TCHAR port[6];
      size_t hostlen = _tsizeof(host);
      size_t portlen = _tsizeof(port);

      if (GetIeHttpProxy(host, &hostlen, port, &portlen) && hostlen != 0)
        {
          if (o.proxy_http_auth)
            {
              /* Ask for Proxy username/password */
              LocalizedDialogBox(ID_DLG_PROXY_AUTH, ProxyAuthDialogFunc);
              _sntprintf_0(proxy_string, _T("--http-proxy %s %s %s"),
                           host, (portlen ? port : _T("80")), o.proxy_authfile);
            }
          else
            {
              _sntprintf_0(proxy_string, _T("--http-proxy %s %s"),
                           host, (portlen ? port : _T("80")));
            }
        }
    }

  _tcsncpy(proxy_string_ptr, proxy_string, size);
}
