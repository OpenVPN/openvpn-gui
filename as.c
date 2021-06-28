/*
 *  OpenVPN-GUI -- A Windows GUI for OpenVPN.
 *
 *  Copyright (C) 2021 Lev Stipakov <lstipakov@gmail.com>
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
#include <stdlib.h>

#include "config.h"
#include "localization.h"
#include "main.h"
#include "misc.h"
#include "openvpn.h"
#include "openvpn-gui-res.h"
#include "save_pass.h"

#define URL_LEN 1024
#define PROFILE_NAME_LEN 128


INT_PTR CALLBACK
ImportProfileFromURLDialogFunc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_INITDIALOG:
        SetStatusWinIcon(hwndDlg, ID_ICO_APP);

        /* disable OK button by default - not disabled in resources */
        EnableWindow(GetDlgItem(hwndDlg, IDOK), FALSE);

        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case ID_EDT_AUTH_USER:
        case ID_EDT_AUTH_PASS:
        case ID_EDT_URL:
            if (HIWORD(wParam) == EN_UPDATE)
            {
                /* enable OK button only if url and username are filled */
                BOOL enableOK = GetWindowTextLengthW(GetDlgItem(hwndDlg, ID_EDT_URL))
                    && GetWindowTextLengthW(GetDlgItem(hwndDlg, ID_EDT_AUTH_USER));
                EnableWindow(GetDlgItem(hwndDlg, IDOK), enableOK);
            }
            break;

        case IDOK:
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
        break;
    }

    return FALSE;
}

void ImportConfigFromAS()
{
    LocalizedDialogBoxParam(ID_DLG_URL_PROFILE_IMPORT, ImportProfileFromURLDialogFunc, 0);
}