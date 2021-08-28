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
#include <shellapi.h>

#include "localization.h"
#include "options.h"
#include "manage.h"
#include "main.h"
#include "misc.h"
#include "main.h"
#include "openvpn_config.h"
#include "openvpn-gui-res.h"

/*
 * Helper function to do base64 conversion through CryptoAPI
 * Returns TRUE on success, FALSE on error. Caller must free *output.
 */
BOOL
Base64Encode(const char *input, int input_len, char **output)
{
    DWORD output_len;
    DWORD flags = CRYPT_STRING_BASE64|CRYPT_STRING_NOCRLF;

    if (input_len == 0)
    {
        /* set output to empty string  -- matches the behavior in openvpn */
        *output = calloc (1, sizeof(char));
        return TRUE;
    }
    if (!CryptBinaryToStringA((const BYTE *) input, (DWORD) input_len,
        flags, NULL, &output_len) || output_len == 0)
    {
#ifdef DEBUG
        PrintDebug (L"Error in CryptBinaryToStringA: input = '%.*S'", input_len, input);
#endif
        *output = NULL;
        return FALSE;
    }
    *output = (char *)malloc(output_len);
    if (*output == NULL)
        return FALSE;

    if (!CryptBinaryToStringA((const BYTE *) input, (DWORD) input_len,
        flags, *output, &output_len))
    {
#ifdef DEBUG
        PrintDebug (L"Error in CryptBinaryToStringA: input = '%.*S'", input_len, input);
#endif
        free(*output);
        *output = NULL;
        return FALSE;
    }

    return TRUE;
}
/*
 * Decode a nul-terminated base64 encoded input and save the result in
 * an allocated buffer *output. The caller must free *output after use.
 * The decoded output is nul-terminated so that the caller may treat
 * it as a string when appropriate.
 *
 * Return the length of the decoded result (excluding nul) or -1 on
 * error.
 */
int
Base64Decode(const char *input, char **output)
{
    DWORD len;

    PrintDebug (L"decoding %S", input);
    if (!CryptStringToBinaryA(input, 0, CRYPT_STRING_BASE64_ANY,
                              NULL, &len, NULL, NULL) || len == 0)
    {
        *output = NULL;
        return -1;
    }

    *output = malloc(len + 1);
    if (*output == NULL)
        return -1;

    if (!CryptStringToBinaryA(input, 0,
        CRYPT_STRING_BASE64, (BYTE *) *output, &len, NULL, NULL))
    {
        free(*output);
        *output = NULL;
        return -1;
    }

    /* NUL terminate output */
    (*output)[len] = '\0';
    PrintDebug (L"Decoded output %S", *output);

    return len;
}

BOOL
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
ManagementCommandFromTwoInputsBase64(connection_t *c, LPCSTR fmt, HWND hDlg,int id, int id2)
{
    BOOL retval = FALSE;
    LPSTR input, input2, input_b64, input2_b64, cmd;
    int input_len, input2_len, cmd_len;

    input_b64 = NULL;
    input2_b64 = NULL;

    GetDlgItemTextUtf8(hDlg, id, &input, &input_len);
    GetDlgItemTextUtf8(hDlg, id2, &input2, &input2_len);

    if (!Base64Encode(input, input_len, &input_b64))
        goto out;
    if (!Base64Encode(input2, input2_len, &input2_b64))
        goto out;

    cmd_len = strlen(input_b64) + strlen(input2_b64) + strlen(fmt);
    cmd = malloc(cmd_len);
    if (cmd)
    {
        snprintf(cmd, cmd_len, fmt, input_b64, input2_b64);
        retval = ManagementCommand(c, cmd, NULL, regular);
        free(cmd);
    }

out:
    /* Clear buffers with potentially secret content */
    if (input_b64)
        memset(input_b64, 0, strlen(input_b64));
    if (input2_b64)
        memset(input2_b64, 0, strlen(input2_b64));
    free(input_b64);
    free(input2_b64);

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
 * Generate a management command from base64-encoded user inputs and send it
 */
BOOL
ManagementCommandFromInputBase64(connection_t* c, LPCSTR fmt, HWND hDlg, int id)
{
    BOOL retval = FALSE;
    LPSTR input, input_b64, cmd;
    int input_len, cmd_len;

    input_b64 = NULL;

    GetDlgItemTextUtf8(hDlg, id, &input, &input_len);

    if (!Base64Encode(input, input_len, &input_b64))
        goto out;

    cmd_len = strlen(input_b64) + strlen(fmt);
    cmd = malloc(cmd_len);
    if (cmd)
    {
        snprintf(cmd, cmd_len, fmt, input_b64);
        retval = ManagementCommand(c, cmd, NULL, regular);
        free(cmd);
    }

out:
    /* Clear buffers with potentially secret content */
    if (input_b64)
        memset(input_b64, 0, strlen(input_b64));
    free(input_b64);

    if (input_len)
    {
        memset(input, 'x', input_len);
        SetDlgItemTextA(hDlg, id, input);
        free(input);
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
strbegins(const char *str, const char *begin)
{
    return (strncmp(str, begin, strlen(begin)) == 0);
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
 * Set scale factor of windows in pixels. Scale = 100% for dpi = 96
 */
void
DpiSetScale(options_t* options, UINT dpix)
{
    /* scale factor in percentage compared to the reference dpi of 96 */
    if (dpix != 0)
        options->dpi_scale = MulDiv(dpix, 100, 96);
    else
        options->dpi_scale = 100;
    PrintDebug(L"DPI scale set to %u", options->dpi_scale);
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
        if (!CheckTokenMembership(NULL, AdministratorsGroup, &b))
            b = FALSE;
        FreeSid(AdministratorsGroup);
    }

    return(b);
}

HANDLE
InitSemaphore (WCHAR *name)
{
    HANDLE semaphore = NULL;
    semaphore = CreateSemaphore (NULL, 1, 1, name);
    if (!semaphore)
    {
        MessageBoxW (NULL, L"Error creating semaphore", TEXT(PACKAGE_NAME), MB_OK);
#ifdef DEBUG
        PrintDebug (L"InitSemaphore: CreateSemaphore failed [error = %lu]", GetLastError());
#endif
    }
    return semaphore;
}

void
CloseSemaphore(HANDLE sem)
{
    if (sem)
    {
        ReleaseSemaphore(sem, 1, NULL);
    }
    CloseHandle(sem);
}

/* Check access rights on an existing file */
BOOL
CheckFileAccess (const TCHAR *path, int access)
{
    HANDLE h;
    bool ret = FALSE;

    h = CreateFile (path, access, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                   FILE_ATTRIBUTE_NORMAL, NULL);
    if ( h != INVALID_HANDLE_VALUE )
    {
        ret = TRUE;
        CloseHandle (h);
    }

    return ret;
}

/*
 * Convert a NUL terminated utf8 string to widechar. The caller must free
 * the returned pointer. Return NULL on error.
 */
WCHAR *
Widen(const char *utf8)
{
    WCHAR *wstr = NULL;
    if (!utf8)
        return wstr;

    int nch = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0);
    if (nch > 0)
        wstr = malloc(sizeof(WCHAR) * nch);
    if (wstr)
        nch =  MultiByteToWideChar(CP_UTF8, 0, utf8, -1, wstr, nch);

    if (nch == 0 && wstr)
    {
        free (wstr);
        wstr = NULL;
    }

    return wstr;
}

/* Return false if input contains any characters in exclude */
BOOL
validate_input(const WCHAR *input, const WCHAR *exclude)
{
    if (!exclude)
        exclude = L"\n";
    return (wcspbrk(input, exclude) == NULL);
}

/* Concatenate two wide strings with a separator -- if either string is empty separator not added */
void
wcs_concat2(WCHAR *dest, int len, const WCHAR *src1, const WCHAR *src2, const WCHAR *sep)
{
    int n = 0;

    if (!dest || len == 0)
        return;

    if (src1 && src2 && src1[0] && src2[0])
        n = swprintf(dest, len, L"%s%s%s", src1, sep, src2);
    else if (src1 && src1[0])
        n = swprintf(dest, len, L"%s", src1);
    else if (src2 && src2[0])
        n = swprintf(dest, len, L"%s", src2);

    if (n < 0 || n >= len) /*swprintf failed */
        n = 0;
    dest[n] = L'\0';
}

void
CloseHandleEx(LPHANDLE handle)
{
    if (handle && *handle && *handle != INVALID_HANDLE_VALUE)
    {
        CloseHandle(*handle);
        *handle = INVALID_HANDLE_VALUE;
    }
}

/*
 * Decode url encoded characters in buffer src and
 * return the result in a newly allocated buffer. The
 * caller should free the returned pointer. Returns
 * NULL on memory allocation error.
 */
char *
url_decode(const char *src)
{
    const char *s = src;
    char *out = malloc(strlen(src) + 1); /* output is guaranteed to be not longer than src */
    char *o;

    if (!out)
        return NULL;

    for (o = out; *s; o++)
    {
        unsigned int c = *s++;
        if (c == '%' && isxdigit(s[0]) && isxdigit(s[1]))
        {
            sscanf(s, "%2x", &c);
            s += 2;
        }
        /* We passthough all other chars including % not followed by 2 hex digits */
        *o = (char)c;
    }
    *o = '\0';

    return out;
}

DWORD
md_init(md_ctx *ctx, ALG_ID hash_type)
{
    DWORD status = 0;

    if (!CryptAcquireContext(&ctx->prov, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
        goto err;
    if (!CryptCreateHash(ctx->prov, hash_type, 0, 0, &ctx->hash))
    {
        CryptReleaseContext(ctx->prov, 0);
        goto err;
    }

    return status;

err:
    status = GetLastError();
    MsgToEventLog(EVENTLOG_ERROR_TYPE, L"Error in md_ctx_init: status = %lu", status);
    return status;
}

DWORD
md_update(md_ctx *ctx, const BYTE *data, size_t size)
{
    DWORD status = 0;

    if (!CryptHashData(ctx->hash, data, (DWORD)size, 0))
    {
        status = GetLastError();
        MsgToEventLog(EVENTLOG_ERROR_TYPE, L"Error in md_update: status = %lu", status);
    }
    return status;
}

DWORD
md_final(md_ctx *ctx, BYTE *md)
{
    DWORD status = 0;

    DWORD digest_len = HASHLEN;
    if (!CryptGetHashParam(ctx->hash, HP_HASHVAL, md, &digest_len, 0))
    {
        status = GetLastError();
        MsgToEventLog(EVENTLOG_ERROR_TYPE, L"Error in md_final: status = %lu", status);
    }
    CryptDestroyHash(ctx->hash);
    CryptReleaseContext(ctx->prov, 0);
    return status;
}

/* Open specified http/https URL using ShellExecute. */
BOOL
open_url(const wchar_t *url)
{
    if (!url || !wcsbegins(url, L"http"))
    {
        return false;
    }

    HINSTANCE ret = ShellExecuteW(NULL, L"open", url, NULL, NULL, SW_SHOWNORMAL);

    if (ret <= (HINSTANCE) 32)
    {
        MsgToEventLog(EVENTLOG_ERROR_TYPE, L"launch_url: ShellExecute <%s> returned error: %d", url, ret);
        return false;
    }
    return true;
}

extern options_t o;

void
ImportConfigFile(const TCHAR* source)
{
    TCHAR fileName[MAX_PATH] = _T("");
    TCHAR ext[MAX_PATH] = _T("");
    _wsplitpath(source, NULL, NULL, fileName, ext);

    /* check if the source points to the config_dir */
    if (wcsnicmp(source, o.global_config_dir, wcslen(o.global_config_dir)) == 0
        || wcsnicmp(source, o.config_dir, wcslen(o.config_dir)) == 0)
    {
        ShowLocalizedMsg(IDS_ERR_IMPORT_SOURCE, source);
        return;
    }
    /* Ensure the source exists and is readable */
    if (!CheckFileAccess(source, GENERIC_READ))
    {
        ShowLocalizedMsg(IDS_ERR_IMPORT_ACCESS, source);
        return;
    }

    WCHAR destination[MAX_PATH+1];
    bool no_overwrite = TRUE;

    /* profile name must be unique: check whether a config by same name exists */
    connection_t *c = GetConnByName(fileName);
    if (c && wcsnicmp(c->config_dir, o.config_dir, wcslen(o.config_dir)) == 0)
    {
        /* Ask the user whether to replace the profile or not. */
        if (ShowLocalizedMsgEx(MB_YESNO, NULL, _T(PACKAGE_NAME), IDS_NFO_IMPORT_OVERWRITE, fileName) == IDNO)
        {
            return;
        }
        no_overwrite = FALSE;
        swprintf(destination, MAX_PATH, L"%ls\\%ls", c->config_dir, c->config_file);
    }
    else
    {
        WCHAR dest_dir[MAX_PATH+1];
        swprintf(dest_dir, MAX_PATH, L"%ls\\%ls", o.config_dir, fileName);
        dest_dir[MAX_PATH] = L'\0';
        if (!EnsureDirExists(dest_dir))
        {
            ShowLocalizedMsg(IDS_ERR_IMPORT_FAILED, dest_dir);
            return;
        }
        swprintf(destination, MAX_PATH, L"%ls\\%ls.%ls", dest_dir, fileName, o.ext_string);
    }
    destination[MAX_PATH] = L'\0';

    if (!CopyFile(source, destination, no_overwrite))
    {
       MsgToEventLog(EVENTLOG_ERROR_TYPE, L"Copy file <%ls> to <%ls> failed (error = %lu)",
                     source, destination, GetLastError());
       ShowLocalizedMsg(IDS_ERR_IMPORT_FAILED, destination);
       return;
    }

    ShowLocalizedMsg(IDS_NFO_IMPORT_SUCCESS);
    /* rescan file list after import */
    BuildFileList();
}
