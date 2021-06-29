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

#include "config.h"
#include "localization.h"
#include "main.h"
#include "misc.h"
#include "openvpn.h"
#include "openvpn-gui-res.h"
#include "save_pass.h"

#define URL_LEN 1024
#define PROFILE_NAME_LEN 128
#define READ_CHUNK_LEN 65536

#define PROFILE_NAME_TOKEN L"# OVPN_ACCESS_SERVER_PROFILE="
#define FRIENDLY_NAME_TOKEN L"# OVPN_ACCESS_SERVER_FRIENDLY_NAME="

/**
 * Extract profile name from profile content.
 *
 * Profile name is either (sorted in priority order):
 * - value of OVPN_ACCESS_SERVER_FRIENDLY_NAME
 * - value of OVPN_ACCESS_SERVER_PROFILE
 * - URL
 *
 * @param profile profile content
 * @param default_name default name for profile if it doesn't contain name
 * @param out_name extracted profile name
 * @param out_name_length max length of out_name char array
 */
void
ExtractProfileName(const WCHAR *profile, const WCHAR *default_name, WCHAR *out_name, size_t out_name_length)
{
    WCHAR friendly_name[PROFILE_NAME_LEN] = { 0 };
    WCHAR profile_name[PROFILE_NAME_LEN] = { 0 };

    /* wcstok() modifies string, need to make a copy */
    WCHAR *buf = _wcsdup(profile);

    WCHAR *pch = NULL;
    pch = wcstok(buf, L"\r\n");

    while (pch != NULL) {
        if (wcsbegins(pch, PROFILE_NAME_TOKEN)) {
            wcsncpy(profile_name, pch + wcslen(PROFILE_NAME_TOKEN), PROFILE_NAME_LEN - 1);
            profile_name[PROFILE_NAME_LEN - 1] = L'\0';
        }
        else if (wcsbegins(pch, FRIENDLY_NAME_TOKEN)) {
            wcsncpy(friendly_name, pch + wcslen(FRIENDLY_NAME_TOKEN), PROFILE_NAME_LEN - 1);
            friendly_name[PROFILE_NAME_LEN - 1] = L'\0';
        }

        pch = wcstok(NULL, L"\r\n");
    }

    /* we use .ovpn here, but extension could be customized */
    /* actual extension will be applied during import */
    if (wcslen(friendly_name) > 0)
        swprintf(out_name, out_name_length, L"%ls.ovpn", friendly_name);
    else if (wcslen(profile_name) > 0)
        swprintf(out_name, out_name_length, L"%ls.ovpn", profile_name);
    else
        swprintf(out_name, out_name_length, L"%ls.ovpn", default_name);

    out_name[out_name_length - 1] = L'\0';

    /* sanitize profile name */
    while (*out_name) {
        wchar_t c = *out_name;
        if (c == L'<' || c == L'>' || c == L':' || c == L'\"' || c == L'/' || c == L'\\' || c == L'|' || c == L'?' || c == L'*')
            *out_name = L'_';
        ++out_name;
    }

    free(buf);
}

void
ShowWinInetError(HANDLE hWnd)
{
    WCHAR err[256] = { 0 };
    FormatMessageW(FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_FROM_SYSTEM, GetModuleHandleW(L"wininet.dll"),
        GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), err, _countof(err), NULL);
    ShowLocalizedMsgEx(MB_OK, hWnd, _T(PACKAGE_NAME), IDS_ERR_URL_IMPORT_PROFILE, GetLastError(), err);
}

struct UrlComponents
{
    int port;
    WCHAR host[URL_LEN];
    bool https;
};

/**
* Extracts protocol, port and hostname from URL
*
* @param url URL to parse, length must be less than URL_MAX
*/
void
ParseUrl(const WCHAR *url, struct UrlComponents* comps)
{
    ZeroMemory(comps, sizeof(struct UrlComponents));

    int domain_off = 0;
    comps->port = 443;
    comps->https = true;
    if (wcsbegins(url, L"http://")) {
        domain_off = 7;
    } else if (wcsbegins(url, L"https://")) {
        domain_off = 8;
    }

    WCHAR* strport = wcsstr(url + domain_off, L":");
    if (strport) {
        WCHAR* end;
        wcsncpy(comps->host, url + domain_off, strport - url - domain_off);
        comps->port = wcstol(strport + 1, &end, 10);
    }
    else {
        wcscpy(comps->host, url + domain_off);
    }

    if (comps->host[wcslen(comps->host) - 1] == L'/')
        comps->host[wcslen(comps->host) - 1] = L'\0';
}

/**
 * Download profile content. In case of error displays error message.
 *
 * @param hWnd handle of window which initiated download
 * @param hRequest WinInet request handle
 * @param pbuf pointer to a buffer, will be allocated by this function. Caller must free it after use.
 * @param psize pointer to a profile size, assigned by this function
 */
BOOL
DownloadProfileContent(HANDLE hWnd, HINTERNET hRequest, char** pbuf, size_t* psize)
{
    size_t pos = 0;
    size_t size = READ_CHUNK_LEN;

    *pbuf = calloc(1, size + 1);
    char* buf = *pbuf;
    if (buf == NULL) {
        MessageBoxW(hWnd, L"Out of memory", _T(PACKAGE_NAME), MB_OK);
        return FALSE;
    }
    while (true) {
        DWORD bytesRead = 0;
        if (!InternetReadFile(hRequest, buf + pos, READ_CHUNK_LEN, &bytesRead)) {
            ShowWinInetError(hWnd);
            return FALSE;
        }

        buf[pos + bytesRead] = '\0';

        if (bytesRead == 0) {
            size = pos;
            break;
        }

        if (pos + bytesRead >= size) {
            size += READ_CHUNK_LEN;
            *pbuf = realloc(*pbuf, size + 1);
            if (!*pbuf) {
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

/**
 * Download profile from AS and save it to a special-named temp file
 *
 * @param hWnd handle of window which initiated download
 * @param host AS hostname, entered by user, might contain protocol and port
 * @param username UTF-8 encoded username used for HTTP basic auth
 * @param password UTF-8 encoded password used for HTTP basic auth
 * @param autologin should autologin profile be used
 * @param out_path full path to where profile is downloaded. Value assigned by this function.
 * @param out_path_size number of elements in out_path arrray
 */
BOOL
DownloadProfile(HANDLE hWnd, const WCHAR *host, const char *username, const char *password_orig,
    BOOL autologin, WCHAR *out_path, size_t out_path_size)
{
    HANDLE hInternet = NULL;
    HANDLE hConnect = NULL;
    HANDLE hRequest = NULL;
    BOOL result = FALSE;
    char* buf = NULL;

    char password[USER_PASS_LEN] = { 0 };
    strncpy(password, password_orig, USER_PASS_LEN - 1);
    password[USER_PASS_LEN - 1] = '\0';

    hInternet = InternetOpenW(L"openvpn-gui/1.0", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!hInternet) {
        ShowWinInetError(hWnd);
        goto done;
    }

    struct UrlComponents comps = { 0 };
    ParseUrl(host, &comps);

    /* wait cursor will be automatically reverted later */
    SetCursor(LoadCursorW(0, IDC_WAIT));

    hConnect = InternetConnectW(hInternet, comps.host, comps.port, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
    if (!hConnect) {
        ShowWinInetError(hWnd);
        goto done;
    }

    WCHAR obj_name[URL_LEN] = { 0 };
    swprintf(obj_name, URL_LEN, L"/rest/%ls?tls-cryptv2=1&action=import", autologin ? L"GetAutologin" : L"GetUserlogin");
    obj_name[URL_LEN - 1] = L'\0';
    hRequest = HttpOpenRequestW(hConnect, NULL, obj_name, NULL, NULL, NULL, comps.https ? INTERNET_FLAG_SECURE : 0, 0);
    if (!hRequest) {
        ShowWinInetError(hWnd);
        goto done;
    }

again:
    if (buf) {
        free(buf);
        buf = NULL;
    }

    /* turns out that *A WinAPI function must be used with UTF-8 encoded parameters to get
     * correct Base64 encoding (used by Basic HTTP auth) for non-ASCII characters
     */
    InternetSetOptionA(hRequest, INTERNET_OPTION_USERNAME, (LPVOID)username, (DWORD)strlen(username));
    InternetSetOptionA(hRequest, INTERNET_OPTION_PASSWORD, (LPVOID)password, (DWORD)strlen(password));

    /* handle cert errors */
    /* https://www.betaarchive.com/wiki/index.php/Microsoft_KB_Archive/182888 */
    if (!HttpSendRequestW(hRequest, NULL, 0, NULL, 0)) {
        DWORD err = GetLastError();
        if ((err == ERROR_INTERNET_INVALID_CA) ||
            (err == ERROR_INTERNET_SEC_CERT_CN_INVALID) ||
            (err == ERROR_INTERNET_SEC_CERT_DATE_INVALID) ||
            (err == ERROR_INTERNET_SEC_CERT_REV_FAILED)) {

            /* ask user what to do and modify options if needed */
            DWORD dlg_result = InternetErrorDlg(hWnd, hRequest,
                err,
                FLAGS_ERROR_UI_FILTER_FOR_ERRORS |
                FLAGS_ERROR_UI_FLAGS_GENERATE_DATA |
                FLAGS_ERROR_UI_FLAGS_CHANGE_OPTIONS,
                NULL);

            if (dlg_result == ERROR_SUCCESS) {
                /* for unknown reasons InternetErrorDlg() doesn't change options for ERROR_INTERNET_SEC_CERT_REV_FAILED,
                 * despite user is willing to continue, so we have to do it manually */
                if (err == ERROR_INTERNET_SEC_CERT_REV_FAILED) {
                    DWORD flags;
                    DWORD len = sizeof(flags);
                    InternetQueryOption(hRequest, INTERNET_OPTION_SECURITY_FLAGS, (LPVOID)&flags, &len);

                    flags |= SECURITY_FLAG_IGNORE_REVOCATION;
                    InternetSetOption(hRequest, INTERNET_OPTION_SECURITY_FLAGS, &flags, sizeof(flags));
                    goto again;
                }

                SetCursor(LoadCursorW(0, IDC_WAIT));
                goto again;
            }
            else
                goto done;
        }

        ShowWinInetError(hWnd);
        goto done;
    }

    /* get http status code */
    DWORD statusCode = 0;
    DWORD length = sizeof(DWORD);
    HttpQueryInfoW(hRequest, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, &statusCode, &length, NULL);
    if (statusCode != 200) {
        ShowLocalizedMsgEx(MB_OK, hWnd, _T(PACKAGE_NAME), IDS_ERR_URL_IMPORT_PROFILE, statusCode, "HTTP error");
        goto done;
    }

    /* download profile content */
    size_t size = 0;
    if (!DownloadProfileContent(hWnd, hRequest, &buf, &size))
        goto done;

    WCHAR name[MAX_PATH] = {0};
    WCHAR* wbuf = Widen(buf);
    if (!wbuf) {
        MessageBoxW(hWnd, L"Failed to convert profile content to wchar", _T(PACKAGE_NAME), MB_OK);
        goto done;
    }
    ExtractProfileName(wbuf, comps.host, name, MAX_PATH);
    free(wbuf);

    /* save profile content into tmp file */
    DWORD res = GetTempPathW((DWORD)out_path_size, out_path);
    if (res == 0 || res > out_path_size) {
        MessageBoxW(hWnd, L"Failed to get TMP path", _T(PACKAGE_NAME), MB_OK);
        goto done;
    }
    swprintf(out_path, out_path_size, L"%s%s", out_path, name);
    out_path[out_path_size - 1] = '\0';
    FILE* f = _wfopen(out_path, L"w");
    if (f == NULL) {
        MessageBoxW(hWnd, L"Unable to save downloaded profile", _T(PACKAGE_NAME), MB_OK);
        goto done;
    }
    fwrite(buf, sizeof(char), size, f);
    fclose(f);

    result = TRUE;

done:
    if (buf)
        free(buf);

    if (hRequest)
        InternetCloseHandle(hRequest);

    if (hConnect)
        InternetCloseHandle(hConnect);

    if (hInternet)
        InternetCloseHandle(hInternet);

    return result;
}

INT_PTR CALLBACK
ImportProfileFromURLDialogFunc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    WCHAR url[URL_LEN] = {0};
    BOOL autologin = FALSE;

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
            if (HIWORD(wParam) == EN_UPDATE) {
                /* enable OK button only if url and username are filled */
                BOOL enableOK = GetWindowTextLengthW(GetDlgItem(hwndDlg, ID_EDT_URL))
                    && GetWindowTextLengthW(GetDlgItem(hwndDlg, ID_EDT_AUTH_USER));
                EnableWindow(GetDlgItem(hwndDlg, IDOK), enableOK);
            }
            break;

        case IDOK:
            autologin = IsDlgButtonChecked(hwndDlg, ID_CHK_AUTOLOGIN) == BST_CHECKED;

            GetDlgItemTextW(hwndDlg, ID_EDT_URL, url, _countof(url));

            int username_len = 0;
            char *username = NULL;
            GetDlgItemTextUtf8(hwndDlg, ID_EDT_AUTH_USER, &username, &username_len);

            int password_len = 0;
            char *password = NULL;
            GetDlgItemTextUtf8(hwndDlg, ID_EDT_AUTH_PASS, &password, &password_len);

            WCHAR path[MAX_PATH + 1] = { 0 };
            BOOL downloaded = DownloadProfile(hwndDlg, url, username, password, autologin, path, _countof(path));

            if (username_len != 0)
                free(username);

            if (password_len != 0)
                free(password);

            if (downloaded) {
                EndDialog(hwndDlg, LOWORD(wParam));

                ImportConfigFile(path);
                _wunlink(path);
            }
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