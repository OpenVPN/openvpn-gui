/*
 *  OpenVPN-GUI -- A Windows GUI for OpenVPN.
 *
 *  Copyright (C) 2012 Heiko Hund <heikoh@users.sf.net>
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
#include <stdlib.h>
#include <malloc.h>

#include "options.h"
#include "manage.h"
#include "misc.h"

/*
 * Helper function to convert UCS-2 text from a dialog item to UTF-8.
 * Caller must free *str if *len != 0.
 */
static BOOL
GetDlgItemTextUtf8(HWND hDlg, int id, LPSTR *str, int *len)
{
    int ucs2_len, utf8_len;
    BOOL retval = FALSE;
    LPTSTR ucs2_str = NULL;
    LPSTR utf8_str = NULL;

    *str = "";
    *len = 0;

    ucs2_len = GetWindowTextLength(GetDlgItem(hDlg, id)) + 1;
    if (ucs2_len == 1)
        goto out;

    ucs2_str = malloc(ucs2_len * sizeof(*ucs2_str));
    if (ucs2_str == NULL)
        goto out;

    if (GetDlgItemText(hDlg, id, ucs2_str, ucs2_len) == 0)
        goto out;

    utf8_len = WideCharToMultiByte(CP_UTF8, 0, ucs2_str, -1, NULL, 0, NULL, NULL);
    utf8_str = malloc(utf8_len);
    if (utf8_str == NULL)
        goto out;

    WideCharToMultiByte(CP_UTF8, 0, ucs2_str, -1, utf8_str, utf8_len, NULL, NULL);

    *str = utf8_str;
    *len = utf8_len - 1;
    retval = TRUE;
out:
    free(ucs2_str);
    return retval;
}


/*
 * Generate a management command from user input and send it
 */
BOOL
ManagementCommandFromInput(connection_t *c, LPCSTR fmt, HWND hDlg, int id)
{
    BOOL retval = FALSE;
    LPSTR input, cmd;
    int input_len, cmd_len;

    GetDlgItemTextUtf8(hDlg, id, &input, &input_len);

    cmd_len = input_len + strlen(fmt);
    cmd = malloc(cmd_len);
    if (cmd)
    {
        snprintf(cmd, cmd_len, fmt, input);
        retval = ManagementCommand(c, cmd, NULL, regular);
        free(cmd);
    }

    /* Clear buffers with potentially secret content */
    if (input_len)
    {
        memset(input, 'x', input_len - 1);
        SetDlgItemTextA(hDlg, id, input);
        free(input);
    }

    return retval;
}


/*
 * Various string helper functions
 */
BOOL
streq(LPCSTR str1, LPCSTR str2)
{
    return (strcmp(str1, str2) == 0);
}

BOOL
wcsbegins(LPCWSTR str, LPCWSTR begin)
{
    return (wcsncmp(str, begin, wcslen(begin)) == 0);
}

BOOL
wcseq(LPCWSTR str1, LPCWSTR str2)
{
    return (wcscmp(str1, str2) == 0);
}
