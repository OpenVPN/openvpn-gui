/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <windows.h>
#include <commctrl.h>
#include <prsht.h>
#include <shlobj.h>
#include <winhttp.h>
#include <wchar.h>
#include <wctype.h>
#include <io.h>
#include <fcntl.h>

#include "main.h"
#include "country_routing.h"
#include "country_routes.h"
#include "country_list.h"
#include "localization.h"
#include "registry.h"
#include "openvpn-gui-res.h"

#define COUNTRY_CODES_SIZE     768
#define COUNTRY_DOWNLOAD_LIMIT (4 * 1024 * 1024)
#define COUNTRY_UPDATE_TIMER   2601
#define COUNTRY_SETTINGS_NAME  L"country_routing"

extern options_t o;

typedef struct
{
    DWORD version;
    DWORD enabled;
    WCHAR countries[COUNTRY_CODES_SIZE]; /* fixed-width records: "IR TR DE " */
} country_settings_t;

static country_settings_t settings;
static SRWLOCK settings_lock = SRWLOCK_INIT;
static INIT_ONCE settings_once = INIT_ONCE_STATIC_INIT;

typedef struct
{
    LONG refs;
    HANDLE cancel;
    WCHAR countries[COUNTRY_CODES_SIZE];
    DWORD error;
    WCHAR failed_country[3];
} country_update_t;

typedef struct
{
    HANDLE thread;
    country_update_t *update;
    BOOL loading;
} country_page_t;

static BOOL
valid_countries(const WCHAR *codes)
{
    size_t length = wcslen(codes);
    if (length % 3 || length > _countof(country_list) * 3)
    {
        return FALSE;
    }
    for (size_t pos = 0; pos < length; pos += 3)
    {
        BOOL found = FALSE;
        if (codes[pos + 2] != L' ')
        {
            return FALSE;
        }
        for (size_t i = 0; i < _countof(country_list); ++i)
        {
            if (codes[pos] == country_list[i].code[0] && codes[pos + 1] == country_list[i].code[1])
            {
                found = TRUE;
                break;
            }
        }
        if (!found)
        {
            return FALSE;
        }
        for (size_t prev = 0; prev < pos; prev += 3)
        {
            if (codes[prev] == codes[pos] && codes[prev + 1] == codes[pos + 1])
            {
                return FALSE;
            }
        }
    }
    return TRUE;
}

static BOOL CALLBACK
load_settings(UNUSED PINIT_ONCE once, UNUSED PVOID param, UNUSED PVOID *context)
{
    country_settings_t saved = { 0 };
    HKEY key;
    DWORD size = sizeof(saved), type;
    settings.version = 1;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, GUI_REGKEY_HKCU, 0, KEY_READ, &key) == ERROR_SUCCESS)
    {
        LONG status =
            RegQueryValueExW(key, COUNTRY_SETTINGS_NAME, NULL, &type, (BYTE *)&saved, &size);
        RegCloseKey(key);
        if (status == ERROR_SUCCESS && type == REG_BINARY && size == sizeof(saved)
            && saved.version == 1 && saved.enabled <= 1
            && saved.countries[_countof(saved.countries) - 1] == L'\0'
            && valid_countries(saved.countries))
        {
            settings = saved;
        }
    }
    return TRUE;
}

static country_settings_t
settings_snapshot(void)
{
    InitOnceExecuteOnce(&settings_once, load_settings, NULL, NULL);
    AcquireSRWLockShared(&settings_lock);
    country_settings_t saved = settings;
    ReleaseSRWLockShared(&settings_lock);
    return saved;
}

static BOOL
cache_directory(WCHAR *path, size_t count)
{
    WCHAR base[MAX_PATH];
    if (SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, SHGFP_TYPE_CURRENT, base) != S_OK)
    {
        SetLastError(ERROR_PATH_NOT_FOUND);
        return FALSE;
    }
    if (swprintf_s(path, count, L"%ls\\OpenVPN-GUI", base) < 0
        || (!CreateDirectoryW(path, NULL) && GetLastError() != ERROR_ALREADY_EXISTS)
        || swprintf_s(path, count, L"%ls\\OpenVPN-GUI\\country-routes", base) < 0
        || (!CreateDirectoryW(path, NULL) && GetLastError() != ERROR_ALREADY_EXISTS))
    {
        return FALSE;
    }
    return TRUE;
}

static BOOL
zone_path(const WCHAR *code, WCHAR *path, size_t count)
{
    WCHAR dir[MAX_PATH];
    return cache_directory(dir, _countof(dir))
           && swprintf_s(
                  path, count, L"%ls\\%lc%lc.zone", dir, towlower(code[0]), towlower(code[1]))
                  >= 0;
}

/* Downloads are bounded and HTTPS-only. Redirects cannot change the provider. */
static DWORD
download_zone(HINTERNET session, const WCHAR *code, const WCHAR *target)
{
    WCHAR url[128], temp[MAX_PATH], dir[MAX_PATH];
    HINTERNET connection = NULL, request = NULL;
    FILE *output = NULL;
    DWORD error = ERROR_INVALID_DATA, status = 0, status_size = sizeof(status);
    country_routes_t routes = { 0 };
    temp[0] = L'\0';

    connection = WinHttpConnect(session, L"www.ipdeny.com", INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!connection)
    {
        return GetLastError();
    }
    swprintf_s(url,
               _countof(url),
               L"/ipblocks/data/aggregated/%lc%lc-aggregated.zone",
               towlower(code[0]),
               towlower(code[1]));
    request = WinHttpOpenRequest(connection,
                                 L"GET",
                                 url,
                                 NULL,
                                 WINHTTP_NO_REFERER,
                                 WINHTTP_DEFAULT_ACCEPT_TYPES,
                                 WINHTTP_FLAG_SECURE);
    DWORD redirects = WINHTTP_OPTION_REDIRECT_POLICY_NEVER;
    if (!request
        || !WinHttpSetOption(request, WINHTTP_OPTION_REDIRECT_POLICY, &redirects, sizeof(redirects))
        || !WinHttpSendRequest(
            request, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0)
        || !WinHttpReceiveResponse(request, NULL)
        || !WinHttpQueryHeaders(request,
                                WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                                WINHTTP_HEADER_NAME_BY_INDEX,
                                &status,
                                &status_size,
                                WINHTTP_NO_HEADER_INDEX))
    {
        error = GetLastError();
        goto out;
    }
    if (status != 200)
    {
        error = status == 404 ? ERROR_FILE_NOT_FOUND : ERROR_BAD_NET_RESP;
        goto out;
    }
    if (!cache_directory(dir, _countof(dir)) || !GetTempFileNameW(dir, L"upd", 0, temp))
    {
        error = GetLastError();
        goto out;
    }
    output = _wfopen(temp, L"w+b");
    if (!output)
    {
        error = ERROR_OPEN_FAILED;
        goto out;
    }
    DWORD total = 0, read;
    char buffer[8192];
    for (;;)
    {
        if (!WinHttpReadData(request, buffer, sizeof(buffer), &read))
        {
            error = GetLastError();
            goto out;
        }
        if (!read)
        {
            break;
        }
        total += read;
        /* Embedded NULs would hide a line suffix from the CIDR parser. */
        if (total > COUNTRY_DOWNLOAD_LIMIT || memchr(buffer, '\0', read)
            || fwrite(buffer, 1, read, output) != read)
        {
            error = ERROR_INVALID_DATA;
            goto out;
        }
    }
    if (fflush(output) || fseek(output, 0, SEEK_SET) || !CountryRoutesRead(output, &routes))
    {
        goto out;
    }
    if (fclose(output))
    {
        output = NULL;
        error = ERROR_WRITE_FAULT;
        goto out;
    }
    output = NULL;
    if (!MoveFileExW(temp, target, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
    {
        error = GetLastError();
        goto out;
    }
    temp[0] = L'\0';
    error = ERROR_SUCCESS;
out:
    if (output)
    {
        fclose(output);
    }
    if (temp[0])
    {
        DeleteFileW(temp);
    }
    CountryRoutesFree(&routes);
    if (request)
    {
        WinHttpCloseHandle(request);
    }
    WinHttpCloseHandle(connection);
    return error;
}

static void
release_update(country_update_t *update)
{
    if (InterlockedDecrement(&update->refs) == 0)
    {
        CloseHandle(update->cancel);
        free(update);
    }
}

static DWORD WINAPI
update_countries(PVOID data)
{
    country_update_t *update = data;
    HINTERNET session = WinHttpOpen(L"OpenVPN-GUI country routes",
                                    WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                    WINHTTP_NO_PROXY_NAME,
                                    WINHTTP_NO_PROXY_BYPASS,
                                    0);
    if (!session)
    {
        update->error = GetLastError();
        goto out;
    }
    WinHttpSetTimeouts(session, 10000, 10000, 10000, 10000);
    for (size_t pos = 0; update->countries[pos]; pos += 3)
    {
        if (WaitForSingleObject(update->cancel, 0) == WAIT_OBJECT_0)
        {
            update->error = ERROR_CANCELLED;
            break;
        }
        WCHAR path[MAX_PATH];
        if (!zone_path(update->countries + pos, path, _countof(path)))
        {
            update->error = GetLastError();
            break;
        }
        update->error = download_zone(session, update->countries + pos, path);
        if (update->error)
        {
            update->failed_country[0] = update->countries[pos];
            update->failed_country[1] = update->countries[pos + 1];
            break;
        }
        /* IPdeny's fair-use policy recommends spacing sequential requests. */
        if (WaitForSingleObject(update->cancel, 500) == WAIT_OBJECT_0)
        {
            update->error = ERROR_CANCELLED;
            break;
        }
    }
    WinHttpCloseHandle(session);
out:
    release_update(update);
    return 0;
}

static void
selected_countries(HWND hwnd, WCHAR *codes)
{
    HWND list = GetDlgItem(hwnd, ID_LST_COUNTRIES);
    size_t pos = 0;
    for (size_t i = 0; i < _countof(country_list); ++i)
    {
        if (ListView_GetCheckState(list, (int)i))
        {
            codes[pos++] = country_list[i].code[0];
            codes[pos++] = country_list[i].code[1];
            codes[pos++] = L' ';
        }
    }
    codes[pos] = L'\0';
}

static void
cache_status(HWND hwnd)
{
    WCHAR codes[COUNTRY_CODES_SIZE], path[MAX_PATH];
    selected_countries(hwnd, codes);
    unsigned int count = 0, cached = 0;
    FILETIME oldest = { 0 };
    for (size_t pos = 0; codes[pos]; pos += 3)
    {
        WIN32_FILE_ATTRIBUTE_DATA attr;
        ++count;
        if (zone_path(codes + pos, path, _countof(path))
            && GetFileAttributesExW(path, GetFileExInfoStandard, &attr)
            && !(attr.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            if (!cached || CompareFileTime(&attr.ftLastWriteTime, &oldest) < 0)
            {
                oldest = attr.ftLastWriteTime;
            }
            ++cached;
        }
    }
    if (cached)
    {
        FILETIME local;
        SYSTEMTIME time;
        WCHAR date[80] = L"";
        FileTimeToLocalFileTime(&oldest, &local);
        FileTimeToSystemTime(&local, &time);
        GetDateFormatW(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &time, NULL, date, _countof(date));
        SetDlgItemTextW(hwnd,
                        ID_TXT_COUNTRY_CACHE,
                        LoadLocalizedString(IDS_COUNTRY_CACHE_DATE, cached, count, date));
    }
    else
    {
        SetDlgItemTextW(
            hwnd, ID_TXT_COUNTRY_CACHE, LoadLocalizedString(IDS_COUNTRY_CACHE_EMPTY, count));
    }
}

static void
page_enabled(HWND hwnd, BOOL busy)
{
    BOOL enabled = IsDlgButtonChecked(hwnd, ID_CHK_COUNTRY_BYPASS) == BST_CHECKED;
    EnableWindow(GetDlgItem(hwnd, ID_CHK_COUNTRY_BYPASS), !busy);
    EnableWindow(GetDlgItem(hwnd, ID_LST_COUNTRIES), !busy && enabled);
    EnableWindow(GetDlgItem(hwnd, ID_EDT_COUNTRY_SEARCH), !busy && enabled);
    EnableWindow(GetDlgItem(hwnd, ID_BTN_COUNTRY_SEARCH), !busy && enabled);
    EnableWindow(GetDlgItem(hwnd, ID_BTN_COUNTRY_UPDATE), !busy && enabled);
}

static int
find_country(HWND list, const WCHAR *query)
{
    size_t length = wcslen(query);
    if (!length)
    {
        return -1;
    }
    /* Exact ISO codes take precedence over substrings in other country names. */
    if (length == 2)
    {
        for (size_t i = 0; i < _countof(country_list); ++i)
        {
            if (towupper(query[0]) == country_list[i].code[0]
                && towupper(query[1]) == country_list[i].code[1])
            {
                return (int)i;
            }
        }
    }
    int start = ListView_GetNextItem(list, -1, LVNI_SELECTED);
    for (size_t step = 1; step <= _countof(country_list); ++step)
    {
        size_t i = (start + (int)step) % (int)_countof(country_list);
        const WCHAR *name = country_list[i].name;
        size_t name_length = wcslen(name);
        for (size_t pos = 0; pos + length <= name_length; ++pos)
        {
            if (!_wcsnicmp(name + pos, query, length))
            {
                return (int)i;
            }
        }
    }
    return -1;
}

static void
search_country(HWND hwnd)
{
    WCHAR text[128];
    GetDlgItemTextW(hwnd, ID_EDT_COUNTRY_SEARCH, text, _countof(text));
    WCHAR *query = text;
    while (iswspace(*query))
    {
        ++query;
    }
    size_t length = wcslen(query);
    while (length && iswspace(query[length - 1]))
    {
        query[--length] = L'\0';
    }
    if (!length)
    {
        SetFocus(GetDlgItem(hwnd, ID_EDT_COUNTRY_SEARCH));
        return;
    }
    HWND list = GetDlgItem(hwnd, ID_LST_COUNTRIES);
    int found = find_country(list, query);
    if (found < 0)
    {
        MessageBoxW(hwnd,
                    LoadLocalizedString(IDS_COUNTRY_SEARCH_NOT_FOUND),
                    TEXT(PACKAGE_NAME),
                    MB_OK | MB_ICONINFORMATION);
        return;
    }
    ListView_SetItemState(list, -1, 0, LVIS_SELECTED | LVIS_FOCUSED);
    ListView_SetItemState(list, found, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    ListView_EnsureVisible(list, found, FALSE);
    SetFocus(list);
}

static BOOL
validate_selection(HWND hwnd)
{
    WCHAR codes[COUNTRY_CODES_SIZE];
    selected_countries(hwnd, codes);
    if (IsDlgButtonChecked(hwnd, ID_CHK_COUNTRY_BYPASS) == BST_CHECKED && !codes[0])
    {
        MessageBoxW(hwnd,
                    LoadLocalizedString(IDS_COUNTRY_SELECT),
                    TEXT(PACKAGE_NAME),
                    MB_OK | MB_ICONINFORMATION);
        return FALSE;
    }
    return TRUE;
}

INT_PTR CALLBACK
CountryRoutingDlgProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    country_page_t *page = (country_page_t *)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    switch (msg)
    {
        case WM_INITDIALOG:
        {
            page = calloc(1, sizeof(*page));
            if (!page)
            {
                return FALSE;
            }
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)page);
            page->loading = TRUE;
            country_settings_t saved = settings_snapshot();
            CheckDlgButton(
                hwnd, ID_CHK_COUNTRY_BYPASS, saved.enabled ? BST_CHECKED : BST_UNCHECKED);
            HWND list = GetDlgItem(hwnd, ID_LST_COUNTRIES);
            SendDlgItemMessageW(hwnd, ID_EDT_COUNTRY_SEARCH, EM_SETLIMITTEXT, 127, 0);
            ListView_SetExtendedListViewStyle(list, LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT);
            RECT rect;
            GetClientRect(list, &rect);
            LVCOLUMNW column = { .mask = LVCF_WIDTH, .cx = rect.right - 24 };
            ListView_InsertColumn(list, 0, &column);
            for (size_t i = 0; i < _countof(country_list); ++i)
            {
                LVITEMW item = { .mask = LVIF_TEXT,
                                 .iItem = (int)i,
                                 .pszText = (WCHAR *)country_list[i].name };
                ListView_InsertItem(list, &item);
                for (size_t pos = 0; saved.countries[pos]; pos += 3)
                {
                    if (saved.countries[pos] == country_list[i].code[0]
                        && saved.countries[pos + 1] == country_list[i].code[1])
                    {
                        ListView_SetCheckState(list, (int)i, TRUE);
                    }
                }
            }
            page->loading = FALSE;
            page_enabled(hwnd, FALSE);
            cache_status(hwnd);
            return TRUE;
        }
        case WM_COMMAND:
            if (!page)
            {
                break;
            }
            if (LOWORD(wparam) == ID_CHK_COUNTRY_BYPASS)
            {
                page_enabled(hwnd, FALSE);
            }
            else if (LOWORD(wparam) == ID_BTN_COUNTRY_SEARCH && !page->thread)
            {
                search_country(hwnd);
            }
            else if (LOWORD(wparam) == ID_BTN_COUNTRY_UPDATE && !page->thread)
            {
                WCHAR codes[COUNTRY_CODES_SIZE];
                selected_countries(hwnd, codes);
                if (!codes[0])
                {
                    validate_selection(hwnd);
                    break;
                }
                country_update_t *update = calloc(1, sizeof(*update));
                if (!update)
                {
                    break;
                }
                update->cancel = CreateEventW(NULL, TRUE, FALSE, NULL);
                wcscpy_s(update->countries, _countof(update->countries), codes);
                update->refs = 2; /* page and worker */
                page->update = update;
                if (update->cancel)
                {
                    page->thread = CreateThread(NULL, 0, update_countries, update, 0, NULL);
                }
                if (!page->thread)
                {
                    release_update(update);
                    release_update(update);
                    page->update = NULL;
                    SetDlgItemTextW(hwnd,
                                    ID_TXT_COUNTRY_CACHE,
                                    LoadLocalizedString(IDS_COUNTRY_UPDATE_START_ERROR));
                    break;
                }
                page_enabled(hwnd, TRUE);
                SetDlgItemTextW(
                    hwnd, ID_TXT_COUNTRY_CACHE, LoadLocalizedString(IDS_COUNTRY_UPDATING));
                SetTimer(hwnd, COUNTRY_UPDATE_TIMER, 200, NULL);
            }
            break;
        case WM_TIMER:
            if (page && wparam == COUNTRY_UPDATE_TIMER && page->thread
                && WaitForSingleObject(page->thread, 0) == WAIT_OBJECT_0)
            {
                KillTimer(hwnd, COUNTRY_UPDATE_TIMER);
                CloseHandle(page->thread);
                page->thread = NULL;
                cache_status(hwnd);
                page_enabled(hwnd, FALSE);
                if (page->update->error)
                {
                    MessageBoxW(hwnd,
                                LoadLocalizedString(IDS_COUNTRY_UPDATE_ERROR,
                                                    page->update->failed_country,
                                                    page->update->error),
                                TEXT(PACKAGE_NAME),
                                MB_OK | MB_ICONERROR);
                }
                release_update(page->update);
                page->update = NULL;
            }
            break;
        case WM_NOTIFY:
        {
            NMHDR *header = (NMHDR *)lparam;
            if (header->idFrom == ID_LST_COUNTRIES && header->code == LVN_ITEMCHANGED)
            {
                if (page && !page->loading && !page->thread)
                {
                    cache_status(hwnd);
                }
            }
            else if (header->code == (UINT)PSN_KILLACTIVE)
            {
                SetWindowLongPtrW(hwnd, DWLP_MSGRESULT, !validate_selection(hwnd));
                return TRUE;
            }
            else if (header->code == (UINT)PSN_APPLY)
            {
                if (!page || page->thread || !validate_selection(hwnd))
                {
                    SetWindowLongPtrW(hwnd, DWLP_MSGRESULT, PSNRET_INVALID);
                    return TRUE;
                }
                country_settings_t saved = { .version = 1 };
                saved.enabled = IsDlgButtonChecked(hwnd, ID_CHK_COUNTRY_BYPASS) == BST_CHECKED;
                selected_countries(hwnd, saved.countries);
                HKEY key;
                LONG result = RegCreateKeyExW(HKEY_CURRENT_USER,
                                              GUI_REGKEY_HKCU,
                                              0,
                                              NULL,
                                              0,
                                              KEY_SET_VALUE,
                                              NULL,
                                              &key,
                                              NULL);
                if (result == ERROR_SUCCESS)
                {
                    result = RegSetValueExW(
                        key, COUNTRY_SETTINGS_NAME, 0, REG_BINARY, (BYTE *)&saved, sizeof(saved));
                    RegCloseKey(key);
                }
                if (result == ERROR_SUCCESS)
                {
                    AcquireSRWLockExclusive(&settings_lock);
                    settings = saved;
                    ReleaseSRWLockExclusive(&settings_lock);
                }
                else
                {
                    MessageBoxW(hwnd,
                                LoadLocalizedString(IDS_COUNTRY_SAVE_ERROR),
                                TEXT(PACKAGE_NAME),
                                MB_OK | MB_ICONERROR);
                }
                SetWindowLongPtrW(hwnd,
                                  DWLP_MSGRESULT,
                                  result == ERROR_SUCCESS ? PSNRET_NOERROR : PSNRET_INVALID);
                return TRUE;
            }
            break;
        }
        case WM_DESTROY:
            if (page)
            {
                KillTimer(hwnd, COUNTRY_UPDATE_TIMER);
                if (page->update)
                {
                    SetEvent(page->update->cancel);
                    release_update(page->update);
                }
                if (page->thread)
                {
                    CloseHandle(page->thread);
                }
                free(page);
                SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
            }
            break;
    }
    return FALSE;
}

void
CleanupCountryRouting(connection_t *c)
{
    if (c->country_route_lock && c->country_route_lock != INVALID_HANDLE_VALUE)
    {
        CloseHandle(c->country_route_lock);
    }
    c->country_route_lock = NULL;
    if (c->country_route_file[0])
    {
        DeleteFileW(c->country_route_file);
        c->country_route_file[0] = L'\0';
    }
}

BOOL
PrepareCountryRouting(connection_t *c)
{
    country_settings_t saved = settings_snapshot();
    country_routes_t routes = { 0 };
    WCHAR path[MAX_PATH], dir[MAX_PATH];
    FILE *file = NULL;
    BOOL result = FALSE;
    if (!saved.enabled)
    {
        return TRUE;
    }
    if (!saved.countries[0])
    {
        goto out;
    }
    if (o.ovpn_engine != OPENVPN_ENGINE_OVPN2)
    {
        ShowLocalizedMsg(IDS_COUNTRY_ENGINE_ERROR);
        return FALSE;
    }
    for (connection_t *other = o.chead; other; other = other->next)
    {
        if (other != c && other->state != disconnected && other->state != detached)
        {
            ShowLocalizedMsg(IDS_COUNTRY_MULTIPLE_ERROR);
            return FALSE;
        }
    }
    for (size_t pos = 0; saved.countries[pos]; pos += 3)
    {
        if (!zone_path(saved.countries + pos, path, _countof(path)))
        {
            goto out;
        }
        file = _wfopen(path, L"rb");
        if (!file || !CountryRoutesRead(file, &routes))
        {
            goto out;
        }
        fclose(file);
        file = NULL;
    }
    if (!cache_directory(dir, _countof(dir))
        || !GetTempFileNameW(dir, L"vpn", 0, c->country_route_file))
    {
        goto out;
    }
    /* Deny replacement/writes while OpenVPN can read this supplemental config. */
    c->country_route_lock = CreateFileW(c->country_route_file,
                                        GENERIC_READ | GENERIC_WRITE,
                                        FILE_SHARE_READ,
                                        NULL,
                                        OPEN_EXISTING,
                                        FILE_ATTRIBUTE_TEMPORARY,
                                        NULL);
    if (c->country_route_lock == INVALID_HANDLE_VALUE)
    {
        c->country_route_lock = NULL;
        goto out;
    }
    HANDLE write_handle;
    if (!DuplicateHandle(GetCurrentProcess(),
                         c->country_route_lock,
                         GetCurrentProcess(),
                         &write_handle,
                         0,
                         FALSE,
                         DUPLICATE_SAME_ACCESS))
    {
        goto out;
    }
    int fd = _open_osfhandle((intptr_t)write_handle, _O_WRONLY | _O_TEXT);
    if (fd == -1)
    {
        CloseHandle(write_handle);
        goto out;
    }
    file = _fdopen(fd, "w");
    if (!file)
    {
        _close(fd);
        goto out;
    }
    fprintf(file, "# Country IPv4 bypass; data from https://www.ipdeny.com/\n");
    if (!CountryRoutesWrite(file, &routes))
    {
        goto out;
    }
    result = fclose(file) == 0;
    file = NULL;
out:
    if (file)
    {
        fclose(file);
    }
    CountryRoutesFree(&routes);
    if (!result)
    {
        CleanupCountryRouting(c);
        ShowLocalizedMsg(IDS_COUNTRY_PREPARE_ERROR);
    }
    return result;
}
