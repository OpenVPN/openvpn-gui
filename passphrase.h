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

#define MIN_PASSWORD_LEN	8

struct user_auth {
  char username[50];
  char password[50];
};

void CheckPrivateKeyPassphrasePrompt (char *line, int config);
void CheckAuthUsernamePrompt (char *line, int config);
void CheckAuthPasswordPrompt (char *line);
BOOL CALLBACK PassphraseDialogFunc (HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK AuthPasswordDialogFunc (HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);

void ShowChangePassphraseDialog(int config);
BOOL CALLBACK ChangePassphraseDialogFunc (HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
void ChangePassphraseThread(int config);
int ConfirmNewPassword(HWND hwndDlg);
int NewPasswordLengh(HWND hwndDlg);
int ChangePasswordPEM(HWND hwndDlg);
int ChangePasswordPKCS12(HWND hwndDlg);
int GetKeyFilename(int config, char *keyfilename, unsigned int keyfilenamesize, int *keyfile_format);

