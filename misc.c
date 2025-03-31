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
#include <ws2tcpip.h>
#include <shlwapi.h>

#include "localization.h"
#include "options.h"
#include "manage.h"
#include "main.h"
#include "misc.h"
#include "main.h"
#include "openvpn_config.h"
#include "openvpn-gui-res.h"
#include "tray.h"
#include "config_parser.h"

/*
 * Helper function to do base64 conversion through CryptoAPI
 * Returns TRUE on success, FALSE on error. Caller must free *output.
 */
BOOL
Base64Encode(const char *input, int input_len, char **output)
{
    DWORD output_len;
    DWORD flags = CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF;

    if (input_len == 0)
    {
        /* set output to empty string  -- matches the behavior in openvpn */
        *output = calloc(1, sizeof(char));
        return TRUE;
    }
    if (!CryptBinaryToStringA((const BYTE *)input, (DWORD)input_len, flags, NULL, &output_len)
        || output_len == 0)
    {
#ifdef DEBUG
        PrintDebug(L"Error in CryptBinaryToStringA: input = '%.*hs'", input_len, input);
#endif
        *output = NULL;
        return FALSE;
    }
    *output = (char *)malloc(output_len);
    if (*output == NULL)
    {
        return FALSE;
    }

    if (!CryptBinaryToStringA((const BYTE *)input, (DWORD)input_len, flags, *output, &output_len))
    {
#ifdef DEBUG
        PrintDebug(L"Error in CryptBinaryToStringA: input = '%.*hs'", input_len, input);
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

    PrintDebug(L"decoding %hs", input);
    if (!CryptStringToBinaryA(input, 0, CRYPT_STRING_BASE64_ANY, NULL, &len, NULL, NULL)
        || len == 0)
    {
        *output = NULL;
        return -1;
    }

    *output = malloc(len + 1);
    if (*output == NULL)
    {
        return -1;
    }

    if (!CryptStringToBinaryA(input, 0, CRYPT_STRING_BASE64, (BYTE *)*output, &len, NULL, NULL))
    {
        free(*output);
        *output = NULL;
        return -1;
    }

    /* NUL terminate output */
    (*output)[len] = '\0';
    PrintDebug(L"Decoded output %hs", *output);

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
    {
        goto out;
    }

    ucs2_str = malloc(ucs2_len * sizeof(*ucs2_str));
    if (ucs2_str == NULL)
    {
        goto out;
    }

    if (GetDlgItemText(hDlg, id, ucs2_str, ucs2_len) == 0)
    {
        goto out;
    }

    utf8_len = WideCharToMultiByte(CP_UTF8, 0, ucs2_str, -1, NULL, 0, NULL, NULL);
    utf8_str = malloc(utf8_len);
    if (utf8_str == NULL)
    {
        goto out;
    }

    WideCharToMultiByte(CP_UTF8, 0, ucs2_str, -1, utf8_str, utf8_len, NULL, NULL);

    *str = utf8_str;
    *len = utf8_len - 1;
    retval = TRUE;
out:
    free(ucs2_str);
    return retval;
}

/**
 * Escape backslash, space and double-quote in a string
 * @param input  Pointer to the string to escape
 * @returns      A newly allocated string containing the result or NULL
 *               on error. Caller must free it after use.
 */
char *
escape_string(const char *input)
{
    char *out = strdup(input);
    const char *esc = "\"\\ ";

    if (!out)
    {
        MsgToEventLog(EVENTLOG_ERROR_TYPE, L"Error in escape_string: out of memory");
        return NULL;
    }

    int len = strlen(out);

    for (int pos = 0; pos < len; ++pos)
    {
        if (strchr(esc, out[pos]))
        {
            char *buf = realloc(out, ++len + 1);
            if (buf == NULL)
            {
                free(out);
                MsgToEventLog(EVENTLOG_ERROR_TYPE, L"Error in escape_string: out of memory");
                return NULL;
            }
            out = buf;
            memmove(out + pos + 1, out + pos, len - pos);
            out[pos] = '\\';
            pos += 1;
        }
    }

    PrintDebug(L"escape_string: in: '%hs' out: '%hs' len = %d", input, out, len);
    return out;
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

    /* Escape input if needed */
    char *input_e = escape_string(input);
    if (!input_e)
    {
        goto out;
    }
    free(input);
    input = input_e;
    input_len = strlen(input);

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
ManagementCommandFromTwoInputsBase64(connection_t *c, LPCSTR fmt, HWND hDlg, int id, int id2)
{
    BOOL retval = FALSE;
    LPSTR input, input2, input_b64, input2_b64, cmd;
    int input_len, input2_len, cmd_len;

    input_b64 = NULL;
    input2_b64 = NULL;

    GetDlgItemTextUtf8(hDlg, id, &input, &input_len);
    GetDlgItemTextUtf8(hDlg, id2, &input2, &input2_len);

    if (!Base64Encode(input, input_len, &input_b64))
    {
        goto out;
    }
    if (!Base64Encode(input2, input2_len, &input2_b64))
    {
        goto out;
    }

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
    {
        memset(input_b64, 0, strlen(input_b64));
    }
    if (input2_b64)
    {
        memset(input2_b64, 0, strlen(input2_b64));
    }
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
ManagementCommandFromInputBase64(connection_t *c, LPCSTR fmt, HWND hDlg, int id)
{
    BOOL retval = FALSE;
    LPSTR input, input_b64, cmd;
    int input_len, cmd_len;

    input_b64 = NULL;

    GetDlgItemTextUtf8(hDlg, id, &input, &input_len);

    if (!Base64Encode(input, input_len, &input_b64))
    {
        goto out;
    }

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
    {
        memset(input_b64, 0, strlen(input_b64));
    }
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
            {
                return FALSE;
            }

            *pos = '\0';
            BOOL ret = EnsureDirExists(dir);
            *pos = '\\';
            if (ret == FALSE)
            {
                return FALSE;
            }
        }
        else if (error != ERROR_FILE_NOT_FOUND)
        {
            return FALSE;
        }

        /* No error if directory already exists */
        return (CreateDirectory(dir, NULL) == TRUE || GetLastError() == ERROR_ALREADY_EXISTS);
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
DpiSetScale(options_t *options, UINT dpix)
{
    /* scale factor in percentage compared to the reference dpi of 96 */
    if (dpix != 0)
    {
        options->dpi_scale = MulDiv(dpix, 100, 96);
    }
    else
    {
        options->dpi_scale = 100;
    }
    PrintDebug(L"DPI scale set to %u", options->dpi_scale);
}

/*
 * Check user has admin rights
 * Taken from https://msdn.microsoft.com/en-us/library/windows/desktop/aa376389(v=vs.85).aspx
 * Returns true if the calling process token has the local Administrators group enabled
 * in its SID.  Assumes the caller is not impersonating and has access to open its own
 * process token.
 */
BOOL
IsUserAdmin(VOID)
{
    BOOL b;
    SID_IDENTIFIER_AUTHORITY NtAuthority = { SECURITY_NT_AUTHORITY };
    PSID AdministratorsGroup;

    b = AllocateAndInitializeSid(&NtAuthority,
                                 2,
                                 SECURITY_BUILTIN_DOMAIN_RID,
                                 DOMAIN_ALIAS_RID_ADMINS,
                                 0,
                                 0,
                                 0,
                                 0,
                                 0,
                                 0,
                                 &AdministratorsGroup);
    if (b)
    {
        if (!CheckTokenMembership(NULL, AdministratorsGroup, &b))
        {
            b = FALSE;
        }
        FreeSid(AdministratorsGroup);
    }

    return (b);
}

HANDLE
InitSemaphore(WCHAR *name)
{
    HANDLE semaphore = NULL;
    semaphore = CreateSemaphore(NULL, 1, 1, name);
    if (!semaphore)
    {
        MessageBoxW(NULL, L"Error creating semaphore", TEXT(PACKAGE_NAME), MB_OK);
#ifdef DEBUG
        PrintDebug(L"InitSemaphore: CreateSemaphore failed [error = %lu]", GetLastError());
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
CheckFileAccess(const TCHAR *path, int access)
{
    HANDLE h;
    bool ret = FALSE;

    h = CreateFile(path, access, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h != INVALID_HANDLE_VALUE)
    {
        ret = TRUE;
        CloseHandle(h);
    }

    return ret;
}

char *
WCharToUTF8(const WCHAR *wstr)
{
    int utf8_len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
    if (utf8_len == 0)
        return NULL;

    char *utf8_str = (char *)malloc(utf8_len);
    if (!utf8_str)
        return NULL;

    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, utf8_str, utf8_len, NULL, NULL);
    return utf8_str;
}

/**
 * Convert a NUL terminated narrow string to wide string using
 * specified codepage. The caller must free
 * the returned pointer. Return NULL on error.
 */
WCHAR *
WidenEx(UINT codepage, const char *str)
{
    WCHAR *wstr = NULL;
    if (!str)
    {
        return wstr;
    }

    int nch = MultiByteToWideChar(codepage, 0, str, -1, NULL, 0);
    if (nch > 0)
    {
        wstr = malloc(sizeof(WCHAR) * nch);
    }
    if (wstr)
    {
        nch = MultiByteToWideChar(codepage, 0, str, -1, wstr, nch);
    }

    if (nch == 0 && wstr)
    {
        free(wstr);
        wstr = NULL;
    }

    return wstr;
}

/*
 * Same as WidenEx with codepage = UTF8
 */
WCHAR *
Widen(const char *utf8)
{
    return WidenEx(CP_UTF8, utf8);
}

/* Return false if input contains any characters in exclude */
BOOL
validate_input(const WCHAR *input, const WCHAR *exclude)
{
    if (!exclude)
    {
        exclude = L"\n";
    }
    return (wcspbrk(input, exclude) == NULL);
}

/* Concatenate two wide strings with a separator -- if either string is empty separator not added */
void
wcs_concat2(WCHAR *dest, int len, const WCHAR *src1, const WCHAR *src2, const WCHAR *sep)
{
    int n = 0;

    if (!dest || len == 0)
    {
        return;
    }

    if (src1 && src2 && src1[0] && src2[0])
    {
        n = swprintf(dest, len, L"%ls%ls%ls", src1, sep, src2);
    }
    else if (src1 && src1[0])
    {
        n = swprintf(dest, len, L"%ls", src1);
    }
    else if (src2 && src2[0])
    {
        n = swprintf(dest, len, L"%ls", src2);
    }

    if (n < 0 || n >= len) /*swprintf failed */
    {
        n = 0;
    }
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
    {
        return NULL;
    }

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
    {
        goto err;
    }
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

    if (ret <= (HINSTANCE)32)
    {
        MsgToEventLog(
            EVENTLOG_ERROR_TYPE, L"launch_url: ShellExecute <%ls> returned error: %d", url, ret);
        return false;
    }
    return true;
}

extern options_t o;

void
ImportConfigFile(const TCHAR *source, bool prompt_user)
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

    WCHAR destination[MAX_PATH + 1];
    bool no_overwrite = TRUE;

    /* profile name must be unique: check whether a config by same name exists */
    connection_t *c = GetConnByName(fileName);
    if (c && wcsnicmp(c->config_dir, o.config_dir, wcslen(o.config_dir)) == 0)
    {
        /* Ask the user whether to replace the profile or not. */
        if (ShowLocalizedMsgEx(
                MB_YESNO | MB_TOPMOST, o.hWnd, _T(PACKAGE_NAME), IDS_NFO_IMPORT_OVERWRITE, fileName)
            == IDNO)
        {
            return;
        }
        no_overwrite = FALSE;
        swprintf(destination, MAX_PATH, L"%ls\\%ls", c->config_dir, c->config_file);
    }
    else
    {
        if (prompt_user
            && ShowLocalizedMsgEx(MB_YESNO | MB_TOPMOST,
                                  o.hWnd,
                                  TEXT(PACKAGE_NAME),
                                  IDS_NFO_IMPORT_SOURCE,
                                  fileName)
                   == IDNO)
        {
            return;
        }
        WCHAR dest_dir[MAX_PATH + 1];
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
        MsgToEventLog(EVENTLOG_ERROR_TYPE,
                      L"Copy file <%ls> to <%ls> failed (error = %lu)",
                      source,
                      destination,
                      GetLastError());
        ShowLocalizedMsg(IDS_ERR_IMPORT_FAILED, destination);
        return;
    }

    ShowTrayBalloon(LoadLocalizedString(IDS_NFO_IMPORT_SUCCESS), fileName);
    /* destroy popup menus, based on existing num_configs, rescan file list and recreate menus */
    RecreatePopupMenus();
}

/*
 * Find a free port to bind and return it in addr.sin_port
 */
BOOL
find_free_tcp_port(SOCKADDR_IN *addr)
{
    BOOL ret = false;
    SOCKADDR_IN addr_bound;
    WSADATA wsaData;
    int len = sizeof(*addr);

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        return ret;
    }

    u_short old_port = addr->sin_port;

    SOCKET sk = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sk == INVALID_SOCKET)
    {
        MsgToEventLog(EVENTLOG_ERROR_TYPE, L"%hs: socket open failed", __func__);
        goto out;
    }
    while (bind(sk, (SOCKADDR *)addr, len))
    {
        if (addr->sin_port == 0)
        {
            MsgToEventLog(EVENTLOG_ERROR_TYPE, L"%hs: bind to dynamic port failed", __func__);
            goto out;
        }
        addr->sin_port = 0;
    }
    if (getsockname(sk, (SOCKADDR *)&addr_bound, &len))
    {
        MsgToEventLog(EVENTLOG_ERROR_TYPE, L"%hs: getsockname failed", __func__);
        goto out;
    }

    ret = true;

out:
    if (sk != INVALID_SOCKET)
    {
        closesocket(sk);
    }
    addr->sin_port = ret ? addr_bound.sin_port : old_port;
    WSACleanup();
    return ret;
}

/* Parse the management address and password
 * from a config file. Results are returned
 * in c->manage.skaddr and c->magage.password.
 * Returns false on parse error, or if address
 * not found. Password not found is not an error.
 */
BOOL
ParseManagementAddress(connection_t *c)
{
    BOOL ret = true;
    wchar_t *pw_file = NULL;
    wchar_t *workdir = c->config_dir;
    wchar_t config_path[MAX_PATH];
    wchar_t pw_path[MAX_PATH] = L"";

    _sntprintf_0(config_path, L"%ls\\%ls", c->config_dir, c->config_file);

    config_entry_t *head = config_parse(config_path);
    config_entry_t *l = head;

    if (!head)
    {
        return false;
    }

    SOCKADDR_IN *addr = &c->manage.skaddr;
    addr->sin_port = 0;

    while (l)
    {
        if (l->ntokens >= 3 && !wcscmp(l->tokens[0], L"management"))
        {
            /* we require the address to be a numerical ipv4 address -- e.g., 127.0.0.1*/
            if (InetPtonW(AF_INET, l->tokens[1], &addr->sin_addr) != 1)
            {
                config_list_free(head);
                return false;
            }

            addr->sin_port = htons(_wtoi(l->tokens[2]));
            pw_file = l->tokens[3]; /* may be null */
        }
        else if (l->ntokens >= 2 && !wcscmp(l->tokens[0], L"cd"))
        {
            workdir = l->tokens[1];
        }
        l = l->next;
    }

    ret = (addr->sin_port != 0);

    if (ret && pw_file)
    {
        if (PathIsRelativeW(pw_file))
        {
            _sntprintf_0(pw_path, L"%ls\\%ls", workdir, pw_file);
        }
        else
        {
            wcsncpy_s(pw_path, MAX_PATH, pw_file, _TRUNCATE);
        }

        FILE *fp;
        if (_wfopen_s(&fp, pw_path, L"r")
            || !fgets(c->manage.password, sizeof(c->manage.password), fp))
        {
            /* This may be normal as not all users may be given access to this secret */
            ret = false;
        }

        StrTrimA(c->manage.password, "\n\r");

        if (fp)
        {
            fclose(fp);
        }
    }
    config_list_free(head);

    PrintDebug(L"ParseManagementAddress: host = %hs port = %d passwd_file = %s",
               inet_ntoa(addr->sin_addr),
               ntohs(addr->sin_port),
               pw_path);

    return ret;
}

/* Write a message to the event log */
void
MsgToEventLog(WORD type, wchar_t *format, ...)
{
    const wchar_t *msg[2];
    wchar_t buf[256];
    int size = _countof(buf);

    if (!o.event_log)
    {
        o.event_log = RegisterEventSource(NULL, TEXT(PACKAGE_NAME));
        if (!o.event_log)
        {
            return;
        }
    }

    va_list args;
    va_start(args, format);
    int nchar = vswprintf(buf, size - 1, format, args);
    va_end(args);

    if (nchar == -1)
    {
        return;
    }

    buf[size - 1] = '\0';

    msg[0] = TEXT(PACKAGE_NAME);
    msg[1] = buf;
    ReportEventW(o.event_log, type, 0, 0, NULL, 2, 0, msg, NULL);
}

/*
 * Get dpi of the system and set the scale factor.
 * The system dpi may be different from the per monitor dpi on
 * Win 8.1 later. We set dpi awareness to system-dpi level in the
 * manifest, and let Windows automatically re-scale windows
 * if/when dpi changes dynamically.
 */
void
dpi_initialize(options_t *options)
{
    UINT dpix = 0;
    HDC hdc = GetDC(NULL);

    if (hdc)
    {
        dpix = GetDeviceCaps(hdc, LOGPIXELSX);
        ReleaseDC(NULL, hdc);
        PrintDebug(L"System DPI: dpix = %u", dpix);
    }
    else
    {
        PrintDebug(L"GetDC failed, using default dpi = 96 (error = %lu)", GetLastError());
        dpix = 96;
    }

    DpiSetScale(options, dpix);
}

#define PLAP_CLASSID "{4fbb8b67-cf02-4982-a7a8-3dd06a2c2ebd}"
int
GetPLAPRegistrationStatus(void)
{
    int res = 0;
    HKEY regkey;
    wchar_t dllPath[MAX_PATH];

    _sntprintf_0(dllPath, L"%ls%ls", o.install_path, L"bin\\libopenvpn_plap.dll");
    if (!CheckFileAccess(dllPath, GENERIC_READ))
    {
        res = -1;
    }
    else if (RegOpenKeyExW(HKEY_CLASSES_ROOT, L"CLSID\\" PLAP_CLASSID, 0, KEY_READ, &regkey)
             == ERROR_SUCCESS)
    {
        res = 1;
        RegCloseKey(regkey);
    }

    return res;
}

DWORD
SetPLAPRegistration(BOOL value)
{
    DWORD res = -1;
    const wchar_t *cmd = L"C:\\windows\\system32\\reg.exe";
    wchar_t params[MAX_PATH];

    /* Run only if the state has changed */
    int plap_status = GetPLAPRegistrationStatus();
    if (plap_status > 0 && (BOOL)plap_status == value)
    {
        return 0;
    }

    if (value)
    {
        _sntprintf_0(
            params, L"import \"%ls%ls\"", o.install_path, L"bin\\openvpn-plap-install.reg");
    }
    else
    {
        _sntprintf_0(
            params, L"import \"%ls%ls\"", o.install_path, L"bin\\openvpn-plap-uninstall.reg");
    }

    res = RunAsAdmin(cmd, params);
    if (res != 0)
    {
        ShowLocalizedMsg(value ? IDS_ERR_PLAP_REG : IDS_ERR_PLAP_UNREG, res);
    }
    return res;
}

/*
 * Run a command as admin using shell execute and return the exit code.
 * Returns 0 on success or a non-zero exit code status of the command.
 * If the command fails to execute, the return value is (DWORD) -1.
 */
DWORD
RunAsAdmin(const WCHAR *cmd, const WCHAR *params)
{
    SHELLEXECUTEINFO shinfo;
    DWORD status = -1;

    CLEAR(shinfo);
    shinfo.cbSize = sizeof(shinfo);
    shinfo.fMask = SEE_MASK_NOCLOSEPROCESS;
    shinfo.hwnd = NULL;
    shinfo.lpVerb = L"runas";
    shinfo.lpFile = cmd;
    shinfo.lpDirectory = NULL;
    shinfo.nShow = SW_HIDE;
    shinfo.lpParameters = params;

    if (ShellExecuteEx(&shinfo) && shinfo.hProcess)
    {
        WaitForSingleObject(shinfo.hProcess, INFINITE);
        GetExitCodeProcess(shinfo.hProcess, &status);
        CloseHandle(shinfo.hProcess);
    }
    return status;
}

/* Like sleep but service messages. If hdlg is not NULL
 * dialog messages for it are checked. Also services
 * MSG_FILTER hooks if caller wants further special processing.
 * Returns false on if WM_QUIT received else returns true (on timeout).
 */
bool
OVPNMsgWait(DWORD timeout, HWND hdlg)
{
    ULONGLONG now = GetTickCount64();
    ULONGLONG end = now + timeout;

    while (end > now)
    {
        if (MsgWaitForMultipleObjectsEx(0, NULL, end - now, QS_ALLINPUT, MWMO_INPUTAVAILABLE)
            == WAIT_OBJECT_0)
        {
            MSG msg;
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
            {
                if (msg.message == WM_QUIT)
                {
                    PostQuitMessage((int)msg.wParam);
                    return false;
                }
                else if (!CallMsgFilter(&msg, MSGF_OVPN_WAIT)
                         && (!hdlg || !IsDialogMessage(hdlg, &msg)))
                {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
            }
        }
        now = GetTickCount64();
    }
    return true;
}

/*
 * Create a random password from the printable ASCII range
 */
bool
GetRandomPassword(char *buf, size_t len)
{
    HCRYPTPROV cp;
    BOOL retval = FALSE;
    unsigned i;

    if (!CryptAcquireContext(&cp, NULL, NULL, PROV_DSS, CRYPT_VERIFYCONTEXT))
    {
        return FALSE;
    }

    if (!CryptGenRandom(cp, len, (PBYTE)buf))
    {
        goto out;
    }

    /* Make sure all values are between 0x21 '!' and 0x7e '~' */
    for (i = 0; i < len; ++i)
    {
        buf[i] = (buf[i] & 0x5d) + 0x21;
    }

    retval = TRUE;
out:
    CryptReleaseContext(cp, 0);
    return retval;
}

/* Setup Password reveal
 * Inputs: edit   handle to password edit control
 *         btn    handle to the control that toggles the reveal
 *         wParam action being handled, or 0 for init
 */
void
ResetPasswordReveal(HWND edit, HWND btn, WPARAM wParam)
{
    if (!edit || !btn)
    {
        return;
    }

    if (o.disable_password_reveal)
    {
        ShowWindow(btn, SW_HIDE);
        return;
    }

    /* set the password field to be masked as a sane default */
    SendMessage(edit, EM_SETPASSWORDCHAR, (WPARAM)'*', 0);
    SendMessage(btn, STM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)LoadLocalizedSmallIcon(ID_ICO_EYE));

    /* if password is not masked on init, disable reveal "button" */
    if (wParam == 0 && SendMessage(edit, EM_GETPASSWORDCHAR, 0, 0) == 0)
    {
        ShowWindow(btn, SW_HIDE);
    }
    /* on losing focus disable password reveal button */
    else if (HIWORD(wParam) == EN_KILLFOCUS)
    {
        ShowWindow(btn, SW_HIDE);
    }
    /* if/when password is cleared enable/re-enable reveal button */
    else if (GetWindowTextLength(edit) == 0)
    {
        ShowWindow(btn, SW_SHOW);
    }
}

/* Toggle masking of text in password field
 * Inputs: edit   handle to password edit control
 *         btn    handle to the control that toggles the reveal
 *         wParam action being handled
 */
void
ChangePasswordVisibility(HWND edit, HWND btn, WPARAM wParam)
{
    if (!edit || !btn)
    {
        return;
    }
    if (HIWORD(wParam) == STN_CLICKED)
    {
        if (SendMessage(edit, EM_GETPASSWORDCHAR, 0, 0) == 0) /* currently visible */
        {
            SendMessage(edit, EM_SETPASSWORDCHAR, (WPARAM)'*', 0);
            SendMessage(
                btn, STM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)LoadLocalizedSmallIcon(ID_ICO_EYE));
        }
        else
        {
            SendMessage(edit, EM_SETPASSWORDCHAR, 0, 0);
            SendMessage(btn,
                        STM_SETIMAGE,
                        (WPARAM)IMAGE_ICON,
                        (LPARAM)LoadLocalizedSmallIcon(ID_ICO_EYESTROKE));
        }
        InvalidateRect(
            edit, NULL, TRUE); /* without this the control doesn't seem to get redrawn promptly */
    }
}
