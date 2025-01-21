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
#include <wininet.h>
#include <stdlib.h>
#include <shlwapi.h>

#include "config.h"
#include "localization.h"
#include "main.h"
#include "misc.h"
#include "openvpn.h"
#include "openvpn-gui-res.h"
#include "save_pass.h"

#define URL_LEN             1024
#define PROFILE_NAME_LEN    128
#define READ_CHUNK_LEN      65536

#define PROFILE_NAME_TOKEN  L"# OVPN_ACCESS_SERVER_PROFILE="
#define FRIENDLY_NAME_TOKEN L"# OVPN_ACCESS_SERVER_FRIENDLY_NAME="

/** Replace characters not allowed in Windows filenames with '_' */
void
SanitizeFilename(wchar_t *fname)
{
    const wchar_t *reserved = L"<>:\"/\\|?*;"; /* remap these and ascii 1 to 31 */
    while (*fname)
    {
        wchar_t c = *fname;
        if (c < 32 || wcschr(reserved, c))
        {
            *fname = L'_';
        }
        ++fname;
    }
}

/**
 * Extract profile name from profile content.
 *
 * Profile name is either (sorted in priority order):
 * - value of OVPN_ACCESS_SERVER_FRIENDLY_NAME
 * - value of OVPN_ACCESS_SERVER_PROFILE
 * - specified default_name
 *
 * @param profile profile content
 * @param default_name default name for profile if it doesn't contain name
 * @param out_name extracted profile name
 * @param out_name_length max length of out_name char array
 */
void
ExtractProfileName(const WCHAR *profile,
                   const WCHAR *default_name,
                   WCHAR *out_name,
                   size_t out_name_length)
{
    WCHAR friendly_name[PROFILE_NAME_LEN] = { 0 };
    WCHAR profile_name[PROFILE_NAME_LEN] = { 0 };

    /* wcstok() modifies string, need to make a copy */
    WCHAR *buf = _wcsdup(profile);

    WCHAR *pch = NULL;
    WCHAR *ctx = NULL;
    pch = wcstok_s(buf, L"\r\n", &ctx);

    while (pch != NULL)
    {
        if (wcsbegins(pch, PROFILE_NAME_TOKEN))
        {
            wcsncpy(profile_name, pch + wcslen(PROFILE_NAME_TOKEN), PROFILE_NAME_LEN - 1);
            profile_name[PROFILE_NAME_LEN - 1] = L'\0';
        }
        else if (wcsbegins(pch, FRIENDLY_NAME_TOKEN))
        {
            wcsncpy(friendly_name, pch + wcslen(FRIENDLY_NAME_TOKEN), PROFILE_NAME_LEN - 1);
            friendly_name[PROFILE_NAME_LEN - 1] = L'\0';
        }

        pch = wcstok_s(NULL, L"\r\n", &ctx);
    }

    /* we use .ovpn here, but extension could be customized */
    /* actual extension will be applied during import */
    if (wcslen(friendly_name) > 0)
    {
        swprintf(out_name, out_name_length, L"%ls.ovpn", friendly_name);
    }
    else if (wcslen(profile_name) > 0)
    {
        swprintf(out_name, out_name_length, L"%ls.ovpn", profile_name);
    }
    else
    {
        swprintf(out_name, out_name_length, L"%ls.ovpn", default_name);
    }

    out_name[out_name_length - 1] = L'\0';

    SanitizeFilename(out_name);

    free(buf);
}

void
ShowWinInetError(HANDLE hWnd)
{
    WCHAR err[256] = { 0 };
    FormatMessageW(FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_FROM_SYSTEM,
                   GetModuleHandleW(L"wininet.dll"),
                   GetLastError(),
                   MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                   err,
                   _countof(err),
                   NULL);
    ShowLocalizedMsgEx(
        MB_OK, hWnd, _T(PACKAGE_NAME), IDS_ERR_URL_IMPORT_PROFILE, GetLastError(), err);
}

struct UrlComponents
{
    int port;
    WCHAR host[URL_LEN];
    WCHAR path[URL_LEN];
    char content_type[256];
    bool https;
};

/**
 * Extracts protocol, port and hostname from URL
 *
 * @param url URL to parse, length must be less than URL_MAX
 */
void
ParseUrl(const WCHAR *url, struct UrlComponents *comps)
{
    ZeroMemory(comps, sizeof(struct UrlComponents));

    comps->port = 443;
    comps->https = true;
    if (wcsbegins(url, L"http://"))
    {
        url += 7;
    }
    else if (wcsbegins(url, L"https://"))
    {
        url += 8;
    }

    WCHAR *strport = wcsstr(url, L":");
    WCHAR *pathptr = wcsstr(url, L"/");
    if (strport)
    {
        wcsncpy_s(comps->host, URL_LEN, url, strport - url);
        comps->port = wcstol(strport + 1, NULL, 10);
    }
    else if (pathptr)
    {
        wcsncpy_s(comps->host, URL_LEN, url, pathptr - url);
    }
    else
    {
        wcsncpy_s(comps->host, URL_LEN, url, _TRUNCATE);
    }

    if (pathptr)
    {
        wcsncpy_s(comps->path, URL_LEN, pathptr + 1, _TRUNCATE);
    }
}

/**
 * Download profile content. In case of error displays error message.
 *
 * @param hWnd handle of window which initiated download
 * @param hRequest WinInet request handle
 * @param pbuf pointer to a buffer, will be allocated by this function. Caller must free it after
 * use.
 * @param psize pointer to a profile size, assigned by this function
 */
BOOL
DownloadProfileContent(HANDLE hWnd, HINTERNET hRequest, char **pbuf, size_t *psize)
{
    size_t pos = 0;
    size_t size = READ_CHUNK_LEN;

    *pbuf = calloc(1, size + 1);
    char *buf = *pbuf;
    if (buf == NULL)
    {
        MessageBoxW(hWnd, L"Out of memory", _T(PACKAGE_NAME), MB_OK);
        return FALSE;
    }
    while (true)
    {
        DWORD bytesRead = 0;
        if (!InternetReadFile(hRequest, buf + pos, READ_CHUNK_LEN, &bytesRead))
        {
            ShowWinInetError(hWnd);
            return FALSE;
        }

        buf[pos + bytesRead] = '\0';

        if (bytesRead == 0)
        {
            size = pos;
            break;
        }

        if (pos + bytesRead >= size)
        {
            size += READ_CHUNK_LEN;
            *pbuf = realloc(*pbuf, size + 1);
            if (!*pbuf)
            {
                free(buf);
                MessageBoxW(hWnd, L"Out of memory", _T(PACKAGE_NAME), MB_OK);
                return FALSE;
            }
            buf = *pbuf;
        }

        pos += bytesRead;
    }

    *psize = size;

    return TRUE;
}

/*
 * DialogProc for challenge-response
 */
INT_PTR CALLBACK
CRDialogFunc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    auth_param_t *param = NULL;

    switch (msg)
    {
        case WM_INITDIALOG:
            param = (auth_param_t *)lParam;
            TRY_SETPROP(hwndDlg, cfgProp, (HANDLE)param);

            WCHAR *wstr = Widen(param->str);
            if (!wstr)
            {
                EndDialog(hwndDlg, LOWORD(wParam));
                break;
            }
            SetDlgItemTextW(hwndDlg, ID_TXT_DESCRIPTION, wstr);
            free(wstr);

            /* Set password echo on if needed */
            if (param->flags & FLAG_CR_ECHO)
            {
                SendMessage(GetDlgItem(hwndDlg, ID_EDT_RESPONSE), EM_SETPASSWORDCHAR, 0, 0);
            }

            SetForegroundWindow(hwndDlg);

            /* disable OK button by default - not disabled in resources */
            EnableWindow(GetDlgItem(hwndDlg, IDOK), FALSE);
            ResetPasswordReveal(
                GetDlgItem(hwndDlg, ID_EDT_RESPONSE), GetDlgItem(hwndDlg, ID_PASSWORD_REVEAL), 0);
            break;

        case WM_COMMAND:
            param = (auth_param_t *)GetProp(hwndDlg, cfgProp);

            switch (LOWORD(wParam))
            {
                case ID_EDT_RESPONSE:
                    if (!(param->flags & FLAG_CR_ECHO))
                    {
                        ResetPasswordReveal(GetDlgItem(hwndDlg, ID_EDT_RESPONSE),
                                            GetDlgItem(hwndDlg, ID_PASSWORD_REVEAL),
                                            wParam);
                    }
                    if (HIWORD(wParam) == EN_UPDATE)
                    {
                        /* enable OK if response is non-empty */
                        BOOL enableOK = GetWindowTextLength((HWND)lParam);
                        EnableWindow(GetDlgItem(hwndDlg, IDOK), enableOK);
                    }
                    break;

                case IDOK:
                {
                    int len = 0;
                    GetDlgItemTextUtf8(hwndDlg, ID_EDT_RESPONSE, &param->cr_response, &len);
                    EndDialog(hwndDlg, LOWORD(wParam));
                }
                    return TRUE;

                case IDCANCEL:
                    EndDialog(hwndDlg, LOWORD(wParam));
                    return TRUE;

                case ID_PASSWORD_REVEAL: /* password reveal symbol clicked */
                    ChangePasswordVisibility(GetDlgItem(hwndDlg, ID_EDT_RESPONSE),
                                             GetDlgItem(hwndDlg, ID_PASSWORD_REVEAL),
                                             wParam);
                    return TRUE;
            }
            break;

        case WM_CLOSE:
            EndDialog(hwndDlg, LOWORD(wParam));
            return TRUE;

        case WM_NCDESTROY:
            RemoveProp(hwndDlg, cfgProp);
            break;
    }

    return FALSE;
}

/**
 * Construct the REST URL for AS profile
 *
 * @param host AS hostname, entered by user, might contain protocol and port
 * @param autologin should autologin profile be used
 * @param comps  Pointer to UrlComponents. Value assigned by this function.
 */
static void
GetASUrl(const WCHAR *host, bool autologin, struct UrlComponents *comps)
{
    ParseUrl(host, comps);

    swprintf(comps->path,
             URL_LEN,
             L"/rest/%ls?tls-cryptv2=1&action=import",
             autologin ? L"GetAutologin" : L"GetUserlogin");
    comps->path[URL_LEN - 1] = L'\0';
}

/**
 * Read content-disposition header and extract file name if any.
 * Returns true on success, false otherwise.
 */
bool
ExtractFilenameFromHeader(HINTERNET hRequest, wchar_t *name, size_t len)
{
    DWORD index = 0;
    char *buf = NULL;
    DWORD buflen = 256;
    bool res = false;
    UINT codepage = 28591; /* ISO 8859_1 -- the default char set for http header */

    buf = malloc(buflen);
    if (!buf
        || (!HttpQueryInfoA(hRequest, HTTP_QUERY_CONTENT_DISPOSITION, buf, &buflen, &index)
            && GetLastError() != ERROR_INSUFFICIENT_BUFFER))
    {
        goto done;
    }

    if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    {
        /* try again with more space */
        free(buf);
        buf = malloc(buflen);
        if (!buf || !HttpQueryInfoA(hRequest, HTTP_QUERY_CONTENT_DISPOSITION, buf, &buflen, &index))
        {
            goto done;
        }
    }

    /* look for filename=<name> */
    char *p = strtok(buf, ";");
    char *fn = NULL;
    for (; p; p = strtok(NULL, ";"))
    {
        if ((fn = strstr(p, "filename=")) != NULL)
        {
            fn += 9;
            continue;
        }
        else if ((fn = strstr(p, "filename*=utf-8''")) != NULL)
        {
            fn += 17;
            UrlUnescapeA(fn, NULL, NULL, URL_UNESCAPE_INPLACE);
            codepage = CP_UTF8;
            break; /* we prefer filename*= value */
        }
    }

    if (fn && strlen(fn))
    {
        StrTrimA(fn, " \""); /* strip leading and trailing spaces and quotes */
        wchar_t *wfn = WidenEx(codepage, fn);
        if (wfn)
        {
            wcsncpy_s(name, len, wfn, _TRUNCATE);
            res = true;
            free(wfn);
        }
    }

    SanitizeFilename(name);

done:
    free(buf);
    return res;
}

/**
 * Download profile from a generic URL and save it to a temp file
 *
 * @param hWnd handle of window which initiated download
 * @param comps pointer to struct UrlComponents describing the URL
 * @param username UTF-8 encoded username used for HTTP basic auth
 * @param password UTF-8 encoded password used for HTTP basic auth
 * @param out_path full path to where profile is downloaded. Value assigned by this function.
 * @param out_path_size number of elements in out_path arrray
 *
 * Filename in out_path is parsed from tags in received data
 * with the url hostname as a fallback.
 */
static BOOL
DownloadProfile(HANDLE hWnd,
                const struct UrlComponents *comps,
                const char *username,
                const char *password_orig,
                WCHAR *out_path,
                size_t out_path_size)
{
    HANDLE hInternet = NULL;
    HANDLE hConnect = NULL;
    HANDLE hRequest = NULL;
    BOOL result = FALSE;
    char *buf = NULL;

    /* need to make copy of password to use it for dynamic response */
    char password[USER_PASS_LEN] = { 0 };
    strncpy_s(password, _countof(password), password_orig, _TRUNCATE);

    /* empty password causes reuse of previously cached value -- set it to some character */
    if (strlen(password) == 0)
    {
        password[0] = 'x';
    }

    hInternet = InternetOpenW(L"openvpn-gui/1.0", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!hInternet)
    {
        ShowWinInetError(hWnd);
        goto done;
    }

    /* Calls to connect and receive block: set timeouts that are not too long */
    unsigned long timeout = 30000; /* 30 seconds */
    InternetSetOption(hInternet, INTERNET_OPTION_CONNECT_TIMEOUT, &timeout, sizeof(timeout));
    InternetSetOption(hInternet, INTERNET_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(timeout));

    /* wait cursor will be automatically reverted later */
    SetCursor(LoadCursorW(0, IDC_WAIT));

    hConnect = InternetConnectW(
        hInternet, comps->host, comps->port, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    if (!hConnect)
    {
        ShowWinInetError(hWnd);
        goto done;
    }

    DWORD req_flags = INTERNET_FLAG_RELOAD; /* load from server, do not use cached data */
    req_flags |= comps->https ? INTERNET_FLAG_SECURE : 0;
    hRequest = HttpOpenRequestW(hConnect, NULL, comps->path, NULL, NULL, NULL, req_flags, 0);
    if (!hRequest)
    {
        ShowWinInetError(hWnd);
        goto done;
    }

again:
    if (buf)
    {
        free(buf);
        buf = NULL;
    }

    /* turns out that *A WinAPI function must be used with UTF-8 encoded parameters to get
     * correct Base64 encoding (used by Basic HTTP auth) for non-ASCII characters
     */
    InternetSetOptionA(
        hRequest, INTERNET_OPTION_USERNAME, (LPVOID)username, (DWORD)strlen(username));
    InternetSetOptionA(
        hRequest, INTERNET_OPTION_PASSWORD, (LPVOID)password, (DWORD)strlen(password));

    /* handle cert errors */
    /* https://www.betaarchive.com/wiki/index.php/Microsoft_KB_Archive/182888 */
    if (!HttpSendRequestW(hRequest, NULL, 0, NULL, 0))
    {
#ifdef DEBUG
        DWORD err = GetLastError();
        if ((err == ERROR_INTERNET_INVALID_CA) || (err == ERROR_INTERNET_SEC_CERT_CN_INVALID)
            || (err == ERROR_INTERNET_SEC_CERT_DATE_INVALID)
            || (err == ERROR_INTERNET_SEC_CERT_REV_FAILED))
        {
            /* ask user what to do and modify options if needed */
            DWORD dlg_result = InternetErrorDlg(hWnd,
                                                hRequest,
                                                err,
                                                FLAGS_ERROR_UI_FILTER_FOR_ERRORS
                                                    | FLAGS_ERROR_UI_FLAGS_GENERATE_DATA
                                                    | FLAGS_ERROR_UI_FLAGS_CHANGE_OPTIONS,
                                                NULL);

            if (dlg_result == ERROR_SUCCESS)
            {
                /* for unknown reasons InternetErrorDlg() doesn't change options for
                 * ERROR_INTERNET_SEC_CERT_REV_FAILED, despite user is willing to continue, so we
                 * have to do it manually */
                if (err == ERROR_INTERNET_SEC_CERT_REV_FAILED)
                {
                    DWORD flags;
                    DWORD len = sizeof(flags);
                    InternetQueryOptionW(
                        hRequest, INTERNET_OPTION_SECURITY_FLAGS, (LPVOID)&flags, &len);

                    flags |= SECURITY_FLAG_IGNORE_REVOCATION;
                    InternetSetOptionW(
                        hRequest, INTERNET_OPTION_SECURITY_FLAGS, &flags, sizeof(flags));
                    goto again;
                }

                SetCursor(LoadCursorW(0, IDC_WAIT));
                goto again;
            }
            else
            {
                goto done;
            }
        }
#endif /* ifdef DEBUG */
        ShowWinInetError(hWnd);
        goto done;
    }

    /* get http status code */
    DWORD status_code = 0;
    DWORD length = sizeof(DWORD);
    HttpQueryInfoW(
        hRequest, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &status_code, &length, NULL);

    size_t size = 0;

    /* download profile content */
    if ((status_code == 200) || (status_code == 401))
    {
        if (!DownloadProfileContent(hWnd, hRequest, &buf, &size))
        {
            goto done;
        }

        char *msg_begin = strstr(buf, "<Message>CRV1:");
        char *msg_end = strstr(buf, "</Message>");
        if ((status_code == 401) && msg_begin && msg_end)
        {
            *msg_end = '\0';
            auth_param_t *param = (auth_param_t *)calloc(1, sizeof(auth_param_t));
            if (!param)
            {
                goto done;
            }

            if (parse_dynamic_cr(msg_begin + 14, param))
            {
                /* prompt user for dynamic challenge */
                INT_PTR res =
                    LocalizedDialogBoxParam(ID_DLG_CHALLENGE_RESPONSE, CRDialogFunc, (LPARAM)param);
                if (res == IDOK)
                {
                    _snprintf_0(password, "CRV1::%s::%s", param->id, param->cr_response);
                }

                free_auth_param(param);

                if (res == IDOK)
                {
                    goto again;
                }
            }
            else
            {
                free_auth_param(param);
            }
        }
    }

    if (status_code != 200)
    {
        ShowLocalizedMsgEx(
            MB_OK, hWnd, _T(PACKAGE_NAME), IDS_ERR_URL_IMPORT_PROFILE, status_code, L"HTTP error");
        goto done;
    }

    /* check content-type if specified */
    if (strlen(comps->content_type) > 0)
    {
        char tmp[256];
        DWORD len = sizeof(tmp);
        BOOL res = HttpQueryInfoA(hRequest, HTTP_QUERY_CONTENT_TYPE, tmp, &len, NULL);
        if (!res || stricmp(comps->content_type, tmp))
        {
            ShowLocalizedMsgEx(MB_OK,
                               hWnd,
                               _T(PACKAGE_NAME),
                               IDS_ERR_URL_IMPORT_PROFILE,
                               0,
                               L"HTTP content-type mismatch");
            goto done;
        }
    }

    WCHAR name[MAX_PATH] = { 0 };
    /* read filename from header or from the profile metadata */
    if (strlen(comps->content_type) == 0 /* AS profile */
        || !ExtractFilenameFromHeader(hRequest, name, MAX_PATH))
    {
        WCHAR *wbuf = Widen(buf);
        if (!wbuf)
        {
            MessageBoxW(
                hWnd, L"Failed to convert profile content to wchar", _T(PACKAGE_NAME), MB_OK);
            goto done;
        }
        ExtractProfileName(wbuf, comps->host, name, MAX_PATH);
        free(wbuf);
    }

    /* save profile content into tmp file */
    DWORD res = GetTempPathW((DWORD)out_path_size, out_path);
    if (res == 0 || res > out_path_size)
    {
        MessageBoxW(hWnd, L"Failed to get TMP path", _T(PACKAGE_NAME), MB_OK);
        goto done;
    }
    swprintf(out_path, out_path_size, L"%ls%ls", out_path, name);
    out_path[out_path_size - 1] = '\0';
    FILE *f;
    if (_wfopen_s(&f, out_path, L"w"))
    {
        MessageBoxW(hWnd, L"Unable to save downloaded profile", _T(PACKAGE_NAME), MB_OK);
        goto done;
    }
    fwrite(buf, sizeof(char), size, f);
    fclose(f);

    result = TRUE;

done:
    if (buf)
    {
        free(buf);
    }

    /* wipe the password */
    SecureZeroMemory(password, sizeof(password));

    if (hRequest)
    {
        InternetCloseHandle(hRequest);
    }

    if (hConnect)
    {
        InternetCloseHandle(hConnect);
    }

    if (hInternet)
    {
        InternetCloseHandle(hInternet);
    }

    return result;
}

typedef enum
{
    server_as = 1,
    server_generic = 2
} server_type_t;

INT_PTR CALLBACK
ImportProfileFromURLDialogFunc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    WCHAR url[URL_LEN] = { 0 };
    BOOL autologin = FALSE;
    server_type_t type;

    switch (msg)
    {
        case WM_INITDIALOG:
            type = (server_type_t)lParam;
            TRY_SETPROP(hwndDlg, cfgProp, (HANDLE)lParam);
            SetStatusWinIcon(hwndDlg, ID_ICO_APP);

            if (type == server_generic)
            {
                /* Change window title and hide autologin checkbox */
                SetWindowTextW(hwndDlg, LoadLocalizedString(IDS_MENU_IMPORT_URL));
                ShowWindow(GetDlgItem(hwndDlg, ID_CHK_AUTOLOGIN), SW_HIDE);
            }
            /* disable OK button until required data is filled in */
            EnableWindow(GetDlgItem(hwndDlg, IDOK), FALSE);
            ResetPasswordReveal(
                GetDlgItem(hwndDlg, ID_EDT_AUTH_PASS), GetDlgItem(hwndDlg, ID_PASSWORD_REVEAL), 0);
            break;

        case WM_COMMAND:
            type = (server_type_t)GetProp(hwndDlg, cfgProp);
            switch (LOWORD(wParam))
            {
                case ID_EDT_AUTH_PASS:
                    ResetPasswordReveal(GetDlgItem(hwndDlg, ID_EDT_AUTH_PASS),
                                        GetDlgItem(hwndDlg, ID_PASSWORD_REVEAL),
                                        wParam);

                /* fall through */
                case ID_EDT_AUTH_USER:
                case ID_EDT_URL:
                    if (HIWORD(wParam) == EN_UPDATE)
                    {
                        /* enable OK button only if url and username are filled */
                        BOOL enableOK =
                            GetWindowTextLengthW(GetDlgItem(hwndDlg, ID_EDT_URL))
                            && GetWindowTextLengthW(GetDlgItem(hwndDlg, ID_EDT_AUTH_USER));
                        EnableWindow(GetDlgItem(hwndDlg, IDOK), enableOK);
                    }
                    break;

                case IDOK:

                    GetDlgItemTextW(hwndDlg, ID_EDT_URL, url, _countof(url));

                    int username_len = 0;
                    char *username = NULL;
                    GetDlgItemTextUtf8(hwndDlg, ID_EDT_AUTH_USER, &username, &username_len);

                    int password_len = 0;
                    char *password = NULL;
                    GetDlgItemTextUtf8(hwndDlg, ID_EDT_AUTH_PASS, &password, &password_len);

                    WCHAR path[MAX_PATH + 1] = { 0 };
                    struct UrlComponents comps = { 0 };
                    if (type == server_as)
                    {
                        autologin = IsDlgButtonChecked(hwndDlg, ID_CHK_AUTOLOGIN) == BST_CHECKED;
                        GetASUrl(url, autologin, &comps);
                    }
                    else
                    {
                        ParseUrl(url, &comps);
                        strncpy_s(comps.content_type,
                                  _countof(comps.content_type),
                                  "application/x-openvpn-profile",
                                  _TRUNCATE);
                    }
                    BOOL downloaded =
                        DownloadProfile(hwndDlg, &comps, username, password, path, _countof(path));

                    if (username_len > 0)
                    {
                        free(username);
                    }

                    if (password_len > 0)
                    {
                        SecureZeroMemory(password, strlen(password));
                        free(password);
                    }

                    if (downloaded)
                    {
                        EndDialog(hwndDlg, LOWORD(wParam));

                        ImportConfigFile(path, false); /* do not prompt user */
                        _wunlink(path);
                    }
                    return TRUE;

                case IDCANCEL:
                    EndDialog(hwndDlg, LOWORD(wParam));
                    return TRUE;

                case ID_PASSWORD_REVEAL: /* password reveal symbol clicked */
                    ChangePasswordVisibility(GetDlgItem(hwndDlg, ID_EDT_AUTH_PASS),
                                             GetDlgItem(hwndDlg, ID_PASSWORD_REVEAL),
                                             wParam);
                    return TRUE;
            }
            break;


        case WM_CLOSE:
            EndDialog(hwndDlg, LOWORD(wParam));
            return TRUE;

        case WM_NCDESTROY:
            RemoveProp(hwndDlg, cfgProp);
            break;
    }

    return FALSE;
}

void
ImportConfigFromAS()
{
    LocalizedDialogBoxParam(
        ID_DLG_URL_PROFILE_IMPORT, ImportProfileFromURLDialogFunc, (LPARAM)server_as);
}

void
ImportConfigFromURL()
{
    LocalizedDialogBoxParam(
        ID_DLG_URL_PROFILE_IMPORT, ImportProfileFromURLDialogFunc, (LPARAM)server_generic);
}
