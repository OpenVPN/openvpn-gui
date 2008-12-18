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

struct process_info {
  HANDLE hProcess;
  int config;
};

int StartOpenVPN(int config);
void StopOpenVPN(int config);
void SuspendOpenVPN(int config);
void StopAllOpenVPN();
int ReadLineFromStdOut(HANDLE hStdOut, int config, char line[1024]);
BOOL CALLBACK StatusDialogFunc (HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
int AutoStartConnections();
int VerifyAutoConnections();
int CheckVersion();
int CountConnectedState(int CheckVal);
void CheckAndSetTrayIcon();
void SetStatusWinIcon(HWND hwndDlg, int IconID);
void ThreadOpenVPNStatus(int status) __attribute__ ((noreturn));
