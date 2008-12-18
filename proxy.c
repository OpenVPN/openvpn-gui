/*
 *  OpenVPN-GUI -- A Windows GUI for OpenVPN.
 *
 *  Copyright (C) 2004 Mathias Sundman <mathias@nilings.se>
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
#include "main.h"
#include "options.h"
#include "registry.h"
#include "proxy.h"
#include "ieproxy.h"
#include "openvpn-gui-res.h"

extern struct options o;

void ShowProxySettingsDialog()
{

  DialogBox(o.hInstance, (LPCTSTR)IDD_PROXY, NULL, (DLGPROC)ProxySettingsDialogFunc);
}

BOOL CALLBACK ProxySettingsDialogFunc (HWND hwndDlg, UINT msg, WPARAM wParam, UNUSED LPARAM lParam)
{
  HICON hIcon;

  switch (msg) {

    case WM_INITDIALOG:
      hIcon = (HICON)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(APP_ICON), 
                                                      IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
      if (hIcon) {
        SendMessage(hwndDlg, WM_SETICON, (WPARAM) (ICON_SMALL), (LPARAM) (hIcon));
        SendMessage(hwndDlg, WM_SETICON, (WPARAM) (ICON_BIG), (LPARAM) (hIcon));
      }

      /* Limit Port editboxs to 5 chars. */
      SendMessage(GetDlgItem(hwndDlg, EDIT_PROXY_HTTP_PORT), EM_SETLIMITTEXT, 5, 0);
      SendMessage(GetDlgItem(hwndDlg, EDIT_PROXY_SOCKS_PORT), EM_SETLIMITTEXT, 5, 0);

      LoadProxySettings(hwndDlg);
      break;

    case WM_COMMAND:
      switch (LOWORD(wParam)) {

        case IDOK:
          if (CheckProxySettings(hwndDlg))
            {
              SaveProxySettings(hwndDlg);
              EndDialog(hwndDlg, LOWORD(wParam));
              return TRUE;
            }
          return FALSE;

        case IDCANCEL: 
          EndDialog(hwndDlg, LOWORD(wParam));
          return TRUE;

        case RB_PROXY_USE_OPENVPN:
          if (HIWORD(wParam) == BN_CLICKED)
            {
              EnableWindow(GetDlgItem(hwndDlg, RB_PROXY_HTTP), FALSE);
              EnableWindow(GetDlgItem(hwndDlg, RB_PROXY_SOCKS), FALSE);
              EnableWindow(GetDlgItem(hwndDlg, EDIT_PROXY_HTTP_ADDRESS), FALSE);
              EnableWindow(GetDlgItem(hwndDlg, EDIT_PROXY_HTTP_PORT), FALSE);
              EnableWindow(GetDlgItem(hwndDlg, TEXT_PROXY_HTTP_ADDRESS), FALSE);
              EnableWindow(GetDlgItem(hwndDlg, TEXT_PROXY_HTTP_PORT), FALSE);
              EnableWindow(GetDlgItem(hwndDlg, CHECKB_PROXY_AUTH), FALSE);
              EnableWindow(GetDlgItem(hwndDlg, EDIT_PROXY_SOCKS_ADDRESS), FALSE);
              EnableWindow(GetDlgItem(hwndDlg, EDIT_PROXY_SOCKS_PORT), FALSE);
              EnableWindow(GetDlgItem(hwndDlg, TEXT_PROXY_SOCKS_ADDRESS), FALSE);
              EnableWindow(GetDlgItem(hwndDlg, TEXT_PROXY_SOCKS_PORT), FALSE);
              break;
            }

        case RB_PROXY_USE_IE:
          if (HIWORD(wParam) == BN_CLICKED)
            {
              EnableWindow(GetDlgItem(hwndDlg, RB_PROXY_HTTP), FALSE);
              EnableWindow(GetDlgItem(hwndDlg, RB_PROXY_SOCKS), FALSE);
              EnableWindow(GetDlgItem(hwndDlg, EDIT_PROXY_HTTP_ADDRESS), FALSE);
              EnableWindow(GetDlgItem(hwndDlg, EDIT_PROXY_HTTP_PORT), FALSE);
              EnableWindow(GetDlgItem(hwndDlg, TEXT_PROXY_HTTP_ADDRESS), FALSE);
              EnableWindow(GetDlgItem(hwndDlg, TEXT_PROXY_HTTP_PORT), FALSE);
              EnableWindow(GetDlgItem(hwndDlg, CHECKB_PROXY_AUTH), TRUE);
              EnableWindow(GetDlgItem(hwndDlg, EDIT_PROXY_SOCKS_ADDRESS), FALSE);
              EnableWindow(GetDlgItem(hwndDlg, EDIT_PROXY_SOCKS_PORT), FALSE);
              EnableWindow(GetDlgItem(hwndDlg, TEXT_PROXY_SOCKS_ADDRESS), FALSE);
              EnableWindow(GetDlgItem(hwndDlg, TEXT_PROXY_SOCKS_PORT), FALSE);
              break;
            }

        case RB_PROXY_USE_MANUAL:
          if (HIWORD(wParam) == BN_CLICKED)
            {
              EnableWindow(GetDlgItem(hwndDlg, RB_PROXY_HTTP), TRUE);
              EnableWindow(GetDlgItem(hwndDlg, RB_PROXY_SOCKS), TRUE);

              if (IsDlgButtonChecked(hwndDlg, RB_PROXY_HTTP) == BST_CHECKED)
                {
                  EnableWindow(GetDlgItem(hwndDlg, EDIT_PROXY_HTTP_ADDRESS), TRUE);
                  EnableWindow(GetDlgItem(hwndDlg, EDIT_PROXY_HTTP_PORT), TRUE);
                  EnableWindow(GetDlgItem(hwndDlg, TEXT_PROXY_HTTP_ADDRESS), TRUE);
                  EnableWindow(GetDlgItem(hwndDlg, TEXT_PROXY_HTTP_PORT), TRUE);
                  EnableWindow(GetDlgItem(hwndDlg, CHECKB_PROXY_AUTH), TRUE);
                  EnableWindow(GetDlgItem(hwndDlg, EDIT_PROXY_SOCKS_ADDRESS), FALSE);
                  EnableWindow(GetDlgItem(hwndDlg, EDIT_PROXY_SOCKS_PORT), FALSE);
                  EnableWindow(GetDlgItem(hwndDlg, TEXT_PROXY_SOCKS_ADDRESS), FALSE);
                  EnableWindow(GetDlgItem(hwndDlg, TEXT_PROXY_SOCKS_PORT), FALSE);
                }
              else
                {
                  EnableWindow(GetDlgItem(hwndDlg, EDIT_PROXY_HTTP_ADDRESS), FALSE);
                  EnableWindow(GetDlgItem(hwndDlg, EDIT_PROXY_HTTP_PORT), FALSE);
                  EnableWindow(GetDlgItem(hwndDlg, TEXT_PROXY_HTTP_ADDRESS), FALSE);
                  EnableWindow(GetDlgItem(hwndDlg, TEXT_PROXY_HTTP_PORT), FALSE);
                  EnableWindow(GetDlgItem(hwndDlg, CHECKB_PROXY_AUTH), FALSE);
                  EnableWindow(GetDlgItem(hwndDlg, EDIT_PROXY_SOCKS_ADDRESS), TRUE);
                  EnableWindow(GetDlgItem(hwndDlg, EDIT_PROXY_SOCKS_PORT), TRUE);
                  EnableWindow(GetDlgItem(hwndDlg, TEXT_PROXY_SOCKS_ADDRESS), TRUE);
                  EnableWindow(GetDlgItem(hwndDlg, TEXT_PROXY_SOCKS_PORT), TRUE);
                }
              break;
            }

        case RB_PROXY_HTTP:
          if (HIWORD(wParam) == BN_CLICKED)
            {
              EnableWindow(GetDlgItem(hwndDlg, EDIT_PROXY_HTTP_ADDRESS), TRUE);
              EnableWindow(GetDlgItem(hwndDlg, EDIT_PROXY_HTTP_PORT), TRUE);
              EnableWindow(GetDlgItem(hwndDlg, TEXT_PROXY_HTTP_ADDRESS), TRUE);
              EnableWindow(GetDlgItem(hwndDlg, TEXT_PROXY_HTTP_PORT), TRUE);
              EnableWindow(GetDlgItem(hwndDlg, CHECKB_PROXY_AUTH), TRUE);
              EnableWindow(GetDlgItem(hwndDlg, EDIT_PROXY_SOCKS_ADDRESS), FALSE);
              EnableWindow(GetDlgItem(hwndDlg, EDIT_PROXY_SOCKS_PORT), FALSE);
              EnableWindow(GetDlgItem(hwndDlg, TEXT_PROXY_SOCKS_ADDRESS), FALSE);
              EnableWindow(GetDlgItem(hwndDlg, TEXT_PROXY_SOCKS_PORT), FALSE);
              break;
            }

        case RB_PROXY_SOCKS:
          if (HIWORD(wParam) == BN_CLICKED)
            {
              EnableWindow(GetDlgItem(hwndDlg, EDIT_PROXY_HTTP_ADDRESS), FALSE);
              EnableWindow(GetDlgItem(hwndDlg, EDIT_PROXY_HTTP_PORT), FALSE);
              EnableWindow(GetDlgItem(hwndDlg, TEXT_PROXY_HTTP_ADDRESS), FALSE);
              EnableWindow(GetDlgItem(hwndDlg, TEXT_PROXY_HTTP_PORT), FALSE);
              EnableWindow(GetDlgItem(hwndDlg, CHECKB_PROXY_AUTH), FALSE);
              EnableWindow(GetDlgItem(hwndDlg, EDIT_PROXY_SOCKS_ADDRESS), TRUE);
              EnableWindow(GetDlgItem(hwndDlg, EDIT_PROXY_SOCKS_PORT), TRUE);
              EnableWindow(GetDlgItem(hwndDlg, TEXT_PROXY_SOCKS_ADDRESS), TRUE);
              EnableWindow(GetDlgItem(hwndDlg, TEXT_PROXY_SOCKS_PORT), TRUE);
              break;
            }
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
  char text[100];
  long port;

  if (IsDlgButtonChecked(hwndDlg, RB_PROXY_USE_MANUAL) == BST_CHECKED)
    {
      if (IsDlgButtonChecked(hwndDlg, RB_PROXY_HTTP) == BST_CHECKED)
        {
          GetDlgItemText(hwndDlg, EDIT_PROXY_HTTP_ADDRESS, text, sizeof(text));
          if (strlen(text) == 0)
            {
              /* http_proxy_access not specified */
              ShowLocalizedMsg(GUI_NAME, ERR_HTTP_PROXY_ADDRESS, "");
              return(0);
            }
          GetDlgItemText(hwndDlg, EDIT_PROXY_HTTP_PORT, text, sizeof(text));
          if (strlen(text) == 0)
            {
              /* http_proxy_port not specified */
              ShowLocalizedMsg(GUI_NAME, ERR_HTTP_PROXY_PORT, "");
              return(0);
            }
          else
            {
              port=strtol(text, NULL, 10);
              if ((port < 1) || (port > 65535))
                {
                  /* http_proxy_port range error */
                  ShowLocalizedMsg(GUI_NAME, ERR_HTTP_PROXY_PORT_RANGE, "");
                  return(0);
                }
            }
        }

      if (IsDlgButtonChecked(hwndDlg, RB_PROXY_SOCKS) == BST_CHECKED)
        {
          GetDlgItemText(hwndDlg, EDIT_PROXY_SOCKS_ADDRESS, text, sizeof(text));
          if (strlen(text) == 0)
            {
              /* socks_proxy_address not specified */
              ShowLocalizedMsg(GUI_NAME, ERR_SOCKS_PROXY_ADDRESS, "");
              return(0);
            }
          GetDlgItemText(hwndDlg, EDIT_PROXY_SOCKS_PORT, text, sizeof(text));
          if (strlen(text) == 0)
            {
              /* socks_proxy_port not specified */
              ShowLocalizedMsg(GUI_NAME, ERR_SOCKS_PROXY_PORT, "");
              return(0);
            }
          else
            {
              port=strtol(text, NULL, 10);
              if ((port < 1) || (port > 65535))
                {
                  /* socks_proxy_port range error */
                  ShowLocalizedMsg(GUI_NAME, ERR_SOCKS_PROXY_PORT_RANGE, "");
                  return(0);
                }
            }
        }
    }
  return(1);
}

void LoadProxySettings(HWND hwndDlg)
{

  /* Disable IE Settings untill this is implemented */
  // EnableWindow(GetDlgItem(hwndDlg, RB_PROXY_USE_IE), FALSE);

  /* Set Proxy Type */
  if (o.proxy_type == 0)  /* HTTP Proxy */
    {
      CheckRadioButton(hwndDlg,
                       RB_PROXY_HTTP,    /* First button in group */
                       RB_PROXY_SOCKS,   /* Last button in group */
                       RB_PROXY_HTTP);   /* Select this button */
    }
  else			  /* SOCKS Proxy */
    {
      CheckRadioButton(hwndDlg,
                       RB_PROXY_HTTP,    /* First button in group */
                       RB_PROXY_SOCKS,   /* Last button in group */
                       RB_PROXY_SOCKS);  /* Select this button */
    }

  /* Set Proxy Settings Source */
  if (o.proxy_source == 0)
    SendMessage(GetDlgItem(hwndDlg, RB_PROXY_USE_OPENVPN), BM_CLICK, 0, 0);
  if (o.proxy_source == 1)
    SendMessage(GetDlgItem(hwndDlg, RB_PROXY_USE_IE), BM_CLICK, 0, 0);
  if (o.proxy_source == 2)
    SendMessage(GetDlgItem(hwndDlg, RB_PROXY_USE_MANUAL), BM_CLICK, 0, 0);

  /* Set Proxy addresses and ports */
  SetDlgItemText(hwndDlg, EDIT_PROXY_HTTP_ADDRESS, o.proxy_http_address);
  SetDlgItemText(hwndDlg, EDIT_PROXY_HTTP_PORT, o.proxy_http_port);
  SetDlgItemText(hwndDlg, EDIT_PROXY_SOCKS_ADDRESS, o.proxy_socks_address);
  SetDlgItemText(hwndDlg, EDIT_PROXY_SOCKS_PORT, o.proxy_socks_port);
  if (o.proxy_http_auth) CheckDlgButton(hwndDlg, CHECKB_PROXY_AUTH, BST_CHECKED);
}

void SaveProxySettings(HWND hwndDlg)
{

  LONG status;
  HKEY regkey;
  DWORD dwDispos;
  char proxy_source_string[2]="0";
  char proxy_type_string[2]="0";
  char proxy_http_auth_string[2]="0";

  /* Save Proxy Settings Source */
  if (IsDlgButtonChecked(hwndDlg, RB_PROXY_USE_OPENVPN) == BST_CHECKED)
    {
      o.proxy_source = 0;
      proxy_source_string[0] = '0';
    }
  if (IsDlgButtonChecked(hwndDlg, RB_PROXY_USE_IE) == BST_CHECKED)
    {
      o.proxy_source = 1;
      proxy_source_string[0] = '1';
    }
  if (IsDlgButtonChecked(hwndDlg, RB_PROXY_USE_MANUAL) == BST_CHECKED)
    {
      o.proxy_source = 2;
      proxy_source_string[0] = '2';
    }

  /* Save Proxy addresses and ports */
  GetDlgItemText(hwndDlg, EDIT_PROXY_HTTP_ADDRESS, o.proxy_http_address, 
                 sizeof(o.proxy_http_address));
  GetDlgItemText(hwndDlg, EDIT_PROXY_HTTP_PORT, o.proxy_http_port, 
                 sizeof(o.proxy_http_port));
  GetDlgItemText(hwndDlg, EDIT_PROXY_SOCKS_ADDRESS, o.proxy_socks_address, 
                 sizeof(o.proxy_http_address));
  GetDlgItemText(hwndDlg, EDIT_PROXY_SOCKS_PORT, o.proxy_socks_port, 
                 sizeof(o.proxy_socks_port));
  
  if (IsDlgButtonChecked(hwndDlg, RB_PROXY_HTTP) == BST_CHECKED)
    {
      o.proxy_type = 0;
      proxy_type_string[0] = '0';
    }
  else
    {
      o.proxy_type = 1;
      proxy_type_string[0] = '1';
    }

  if (IsDlgButtonChecked(hwndDlg, CHECKB_PROXY_AUTH) == BST_CHECKED)
    {
      o.proxy_http_auth = 1;
      proxy_http_auth_string[0] = '1';
    }
  else
    {
      o.proxy_http_auth = 0;
      proxy_http_auth_string[0] = '0';
    }

  /* Open Registry for writing */
  if (RegCreateKeyEx(HKEY_CURRENT_USER,
                    GUI_REGKEY_HKCU,
                    0,
                    (LPTSTR) "",
                    REG_OPTION_NON_VOLATILE,
                    KEY_WRITE,
                    NULL,
                    &regkey,
                    &dwDispos) != ERROR_SUCCESS)
    {
      /* error creating Registry-Key */
      ShowLocalizedMsg(GUI_NAME, ERR_CREATE_REG_HKCU_KEY, GUI_REGKEY_HKCU);
      return;
    }

  /* Save Settings to registry */
  SetRegistryValue(regkey, "proxy_source", proxy_source_string);
  SetRegistryValue(regkey, "proxy_type", proxy_type_string);
  SetRegistryValue(regkey, "proxy_http_auth", proxy_http_auth_string);
  SetRegistryValue(regkey, "proxy_http_address", o.proxy_http_address);
  SetRegistryValue(regkey, "proxy_http_port", o.proxy_http_port);
  SetRegistryValue(regkey, "proxy_socks_address", o.proxy_socks_address);
  SetRegistryValue(regkey, "proxy_socks_port", o.proxy_socks_port);

  RegCloseKey(regkey);
 
}

void GetProxyRegistrySettings()
{
  LONG status;
  HKEY regkey;
  char proxy_source_string[2]="0";
  char proxy_type_string[2]="0";
  char proxy_http_auth_string[2]="0";
  char temp_path[100];

  /* Construct Authfile path */
  if (!GetTempPath(sizeof(temp_path) - 1, temp_path))
    {
      /* Error getting TempPath - using C:\ */
      ShowLocalizedMsg(GUI_NAME, ERR_GET_TEMP_PATH, "");
      strcpy(temp_path, "C:\\");
    }
  strncat(temp_path, "openvpn_authfile.txt", 
          sizeof(temp_path) - strlen(temp_path) - 1); 
  strncpy(o.proxy_authfile, temp_path, sizeof(o.proxy_authfile));

  /* Open Registry for reading */
  status = RegOpenKeyEx(HKEY_CURRENT_USER,
                       GUI_REGKEY_HKCU,
                       0,
                       KEY_READ,
                       &regkey);

  /* Return if can't open the registry key */
  if (status != ERROR_SUCCESS) return;
      

  /* get registry settings */
  GetRegistryValue(regkey, "proxy_http_address", o.proxy_http_address, 
                   sizeof(o.proxy_http_address));
  GetRegistryValue(regkey, "proxy_http_port", o.proxy_http_port, 
                   sizeof(o.proxy_http_port));
  GetRegistryValue(regkey, "proxy_socks_address", o.proxy_socks_address, 
                   sizeof(o.proxy_socks_address));
  GetRegistryValue(regkey, "proxy_socks_port", o.proxy_socks_port, 
                   sizeof(o.proxy_socks_port));
  GetRegistryValue(regkey, "proxy_source", proxy_source_string, 
                   sizeof(proxy_source_string));
  GetRegistryValue(regkey, "proxy_type", proxy_type_string, 
                   sizeof(proxy_type_string));
  GetRegistryValue(regkey, "proxy_http_auth", proxy_http_auth_string, 
                   sizeof(proxy_http_auth_string));

  if (proxy_source_string[0] == '1')
    o.proxy_source=1;
  if (proxy_source_string[0] == '2')
    o.proxy_source=2;
  if (proxy_source_string[0] == '3')
    o.proxy_source=3;

  if (proxy_type_string[0] == '1')
    o.proxy_type=1;

  if (proxy_http_auth_string[0] == '1')
    o.proxy_http_auth=1;

  RegCloseKey(regkey);
  return;  
}

BOOL CALLBACK ProxyAuthDialogFunc (HWND hwndDlg, UINT msg, WPARAM wParam, UNUSED LPARAM lParam)
{
  char username[50];
  char password[50];
  FILE *fp;

  switch (msg) {

    case WM_INITDIALOG:
      //SetForegroundWindow(hwndDlg);
      break;

    case WM_COMMAND:
      switch (LOWORD(wParam)) {

        case IDOK:
          GetDlgItemText(hwndDlg, EDIT_PROXY_USERNAME, username, sizeof(username) - 1);
          GetDlgItemText(hwndDlg, EDIT_PROXY_PASSWORD, password, sizeof(password) - 1);
          if (!(fp = fopen(o.proxy_authfile, "w")))
            {
              /* error creating AUTH file */
              ShowLocalizedMsg(GUI_NAME, ERR_CREATE_AUTH_FILE, o.proxy_authfile);
              EndDialog(hwndDlg, LOWORD(wParam));
            }
          fputs(username, fp);
          fputs("\n", fp);
          fputs(password, fp);
          fputs("\n", fp);
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
 * Construct the proxy options to append to the cmd-line.
 */
void ConstructProxyCmdLine(char *proxy_string_ptr, unsigned int size)
{
  char proxy_string[100];

  CLEAR(proxy_string);

  if (o.proxy_source == PROXY_SOURCE_MANUAL)
    {
      if (o.proxy_type == PROXY_TYPE_HTTP)
        {
          if (o.proxy_http_auth == PROXY_HTTP_ASK_AUTH)
            {
              /* Ask for Proxy username/password */
              DialogBox(o.hInstance, (LPCTSTR)IDD_PROXY_AUTH, NULL, (DLGPROC)ProxyAuthDialogFunc);
              mysnprintf(proxy_string, "--http-proxy %s %s %s",
                         o.proxy_http_address,
                         o.proxy_http_port,
                         o.proxy_authfile);
            }
          else
            {
              mysnprintf(proxy_string, "--http-proxy %s %s",
                         o.proxy_http_address,
                         o.proxy_http_port);
            }
        }
      if (o.proxy_type == PROXY_TYPE_SOCKS)
        {
          mysnprintf(proxy_string, "--socks-proxy %s %s",
                     o.proxy_socks_address,
                     o.proxy_socks_port);
        }
    }

  if (o.proxy_source == PROXY_SOURCE_IE)
    {
      LPCTSTR ieproxy;
      char *pos;

      ieproxy = getIeHttpProxy();
      if (!ieproxy)
        {
          /* can't get ie proxy settings */
          ShowLocalizedMsg(GUI_NAME, ERR_GET_IE_PROXY_SETTINGS, getIeHttpProxyError);
        }
      else
        {
          /* Don't use a proxy if IE proxy string is empty */
          if (strlen(ieproxy) > 1)
            {
              /* Parse the port number from the IE proxy string */
              if ((pos = strchr(ieproxy, ':')))
                {
                  *pos = '\0';
                  if (o.proxy_http_auth == PROXY_HTTP_ASK_AUTH)
                    {
                      /* Ask for Proxy username/password */
                      DialogBox(o.hInstance, (LPCTSTR)IDD_PROXY_AUTH, NULL, (DLGPROC)ProxyAuthDialogFunc);
                      mysnprintf(proxy_string, "--http-proxy %s %s %s", 
                                 ieproxy, pos+1, o.proxy_authfile)
                    }
                  else
                    {
                      mysnprintf(proxy_string, "--http-proxy %s %s", ieproxy, pos+1)
                    }
                }
              /* No port is specified, use 80 as default port */
              else
                {
                  if (o.proxy_http_auth == PROXY_HTTP_ASK_AUTH)
                    {
                      /* Ask for Proxy username/password */
                      DialogBox(o.hInstance, (LPCTSTR)IDD_PROXY_AUTH, NULL, (DLGPROC)ProxyAuthDialogFunc);
                      mysnprintf(proxy_string, "--http-proxy %s 80 %s", 
                                 ieproxy, o.proxy_authfile)
                    }
                  else
                    {
                      mysnprintf(proxy_string, "--http-proxy %s 80", ieproxy)
                    }
                }
            }
          //free (ieproxy);
        }
    }

  strncpy(proxy_string_ptr, proxy_string, size);
}
