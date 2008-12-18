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

#define PROXY_SOURCE_OPENVPN 	0
#define PROXY_SOURCE_IE		1
#define PROXY_SOURCE_MANUAL	2
#define PROXY_TYPE_HTTP		0
#define PROXY_TYPE_SOCKS	1
#define PROXY_HTTP_NO_AUTH	0
#define PROXY_HTTP_ASK_AUTH	1

void ShowProxySettingsDialog();
BOOL CALLBACK ProxySettingsDialogFunc (HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
int CheckProxySettings(HWND hwndDlg);
void LoadProxySettings(HWND hwndDlg);
void SaveProxySettings(HWND hwndDlg);
void GetProxyRegistrySettings();
BOOL CALLBACK ProxyAuthDialogFunc (HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
void ConstructProxyCmdLine(char *proxy_string_ptr, unsigned int size);
