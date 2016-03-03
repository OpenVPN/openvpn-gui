/*
 *  OpenVPN-GUI -- A Windows GUI for OpenVPN.
 *
 *  Copyright (C) 2013 Heiko Hund <heikoh@users.sf.net>
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
#include <wincrypt.h>
#include <tchar.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>

#include "options.h"
#include "manage.h"
#include "misc.h"

/*
 * Helper function to do base64 conversion through CryptoAPI
 */
static void
Base64Encode(const char *input, int input_len, char **output)
{
    DWORD output_len;
    if (!CryptBinaryToStringA((const BYTE *) input, (DWORD) input_len,
        CRYPT_STRING_BASE64, NULL, &output_len) || output_len == 0)
    {
        *output = NULL;
        return;
    }
    *output = (char *)malloc(output_len);
    if (!CryptBinaryToStringA((const BYTE *) input, (DWORD) input_len,
        CRYPT_STRING_BASE64, *output, &output_len))
    {
        free(*output);
        *output = NULL;
        return;
    }
    /* Trim trailing "\r\n" manually.
       Actually they can be stripped by adding CRYPT_STRING_NOCRLF to dwFlags,
       but Windows XP/2003 does not support this flag. */
    if(output_len > 1 && (*output)[output_len - 1] == '\x0A'
        && (*output)[output_len - 2] == '\x0D')
        (*output)[output_len - 2] = 0;
}

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
    int input_len, cmd_len, pos;

    GetDlgItemTextUtf8(hDlg, id, &input, &input_len);

    /* Escape input if needed */
    for (pos = 0; pos < input_len; ++pos)
    {
        if (input[pos] == '\\' || input[pos] == '"')
        {
            LPSTR buf = realloc(input, ++input_len + 1);
            if (buf == NULL)
                goto out;

            input = buf;
            memmove(input + pos + 1, input + pos, input_len - pos + 1);
            input[pos] = '\\';
            pos += 1;
        }
    }

    cmd_len = input_len + strlen(fmt);
    cmd = malloc(cmd_len);
    if (cmd)
    {
        snprintf(cmd, cmd_len, fmt, input);
        retval = ManagementCommand(c, cmd, NULL, regular);
        free(cmd);
    }

out:
    /* Clear buffers with potentially secret content */
    if (input_len)
    {
        memset(input, 'x', input_len);
        SetDlgItemTextA(hDlg, id, input);
        free(input);
    }

    return retval;
}



/*
 * Generate a management command from double user inputs and send it
 */
BOOL
ManagementCommandFromInputBase64(connection_t *c, LPCSTR fmt, HWND hDlg,int id, int id2)
{
    BOOL retval = FALSE;
    LPSTR input, input2, input_b64, input2_b64, cmd;
    int input_len, input2_len, cmd_len, pos;

    GetDlgItemTextUtf8(hDlg, id, &input, &input_len);
    GetDlgItemTextUtf8(hDlg, id2, &input2, &input2_len);

    /* Escape input if needed */
    for (pos = 0; pos < input_len; ++pos)
    {
        if (input[pos] == '\\' || input[pos] == '"')
        {
            LPSTR buf = realloc(input, ++input_len + 1);
            if (buf == NULL)
                goto out;

            input = buf;
            memmove(input + pos + 1, input + pos, input_len - pos + 1);
            input[pos] = '\\';
            pos += 1;
        }
    }
    for (pos = 0; pos < input2_len; ++pos)
    {
        if (input2[pos] == '\\' || input2[pos] == '"')
        {
            LPSTR buf = realloc(input2, ++input2_len + 1);
            if (buf == NULL)
                goto out;

            input2 = buf;
            memmove(input2 + pos + 1, input2 + pos, input2_len - pos + 1);
            input2[pos] = '\\';
            pos += 1;
        }
    }

    Base64Encode(input, input_len, &input_b64);
    Base64Encode(input2, input2_len, &input2_b64);

    cmd_len = input_len * 2 + input2_len * 2 + strlen(fmt);
    cmd = malloc(cmd_len);
    if (cmd)
    {
        snprintf(cmd, cmd_len, fmt, input_b64, input2_b64);
        retval = ManagementCommand(c, cmd, NULL, regular);
        free(cmd);
    }
    free(input_b64);
    free(input2_b64);

out:
    /* Clear buffers with potentially secret content */
    if (input_len)
    {
        memset(input, 'x', input_len);
        SetDlgItemTextA(hDlg, id, input);
        free(input);
    }
    if (input2_len)
    {
        memset(input2, 'x', input2_len);
        SetDlgItemTextA(hDlg, id2, input2);
        free(input2);
    }

    return retval;
}


/*
 * Ensures the given directory exists, by checking for and
 * creating missing parts of the path.
 * If the path does not exist and cannot be created return FALSE.
 */
BOOL
EnsureDirExists(LPTSTR dir)
{
    DWORD attr = GetFileAttributes(dir);
    if (attr == INVALID_FILE_ATTRIBUTES)
    {
        DWORD error = GetLastError();
        if (error == ERROR_PATH_NOT_FOUND)
        {
            LPTSTR pos = _tcsrchr(dir, '\\');
            if (pos == NULL)
                return FALSE;

            *pos = '\0';
            BOOL ret = EnsureDirExists(dir);
            *pos = '\\';
            if (ret == FALSE)
                return FALSE;
        }
        else if (error != ERROR_FILE_NOT_FOUND)
            return FALSE;

        /* No error if directory already exists */
        return (CreateDirectory(dir, NULL) == TRUE
            ||  GetLastError() == ERROR_ALREADY_EXISTS);
    }

    return (attr & FILE_ATTRIBUTE_DIRECTORY ? TRUE : FALSE);
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

/*
 * Force setting window as foreground window by simulating an ALT keypress
 */
BOOL
ForceForegroundWindow(HWND hWnd)
{
    BOOL ret = FALSE;

    keybd_event(VK_MENU, 0, 0, 0);
    ret = SetForegroundWindow(hWnd);
    keybd_event(VK_MENU, 0, KEYEVENTF_KEYUP, 0);

    return ret;
}

/*
 * Check user has admin rights
 * Taken from https://msdn.microsoft.com/en-us/library/windows/desktop/aa376389(v=vs.85).aspx
 * Returns true if the calling process token has the local Administrators group enabled
 * in its SID.  Assumes the caller is not impersonating and has access to open its own 
 * process token.
 */
BOOL IsUserAdmin(VOID)
{
    BOOL b;
    SID_IDENTIFIER_AUTHORITY NtAuthority = {SECURITY_NT_AUTHORITY};
    PSID AdministratorsGroup;

    b = AllocateAndInitializeSid (&NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                  DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0,
                                  &AdministratorsGroup);
    if(b)
    {
        CheckTokenMembership(NULL, AdministratorsGroup, &b);
        FreeSid(AdministratorsGroup);
    }

    return(b);
}
