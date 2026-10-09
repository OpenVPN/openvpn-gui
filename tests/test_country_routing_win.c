/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Exercise the real Windows file and dialog code with isolated cache/registry. */
#include <windows.h>
#include <shlobj.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

static WCHAR test_base[MAX_PATH];
static BYTE registry_data[2048];
static DWORD registry_size;
static UINT last_error_message;

#define CHECK(expr)                                           \
    do                                                        \
    {                                                         \
        if (!(expr))                                          \
        {                                                     \
            fprintf(stderr,                                   \
                    "FAIL line %d: %s (Windows error %lu)\n", \
                    __LINE__,                                 \
                    #expr,                                    \
                    GetLastError());                          \
            exit(1);                                          \
        }                                                     \
    } while (0)

static HRESULT WINAPI
test_folder_path(HWND hwnd, int folder, HANDLE token, DWORD flags, LPWSTR path)
{
    (void)hwnd;
    (void)folder;
    (void)token;
    (void)flags;
    wcscpy_s(path, MAX_PATH, test_base);
    return S_OK;
}

static LSTATUS WINAPI
test_reg_create(HKEY root,
                LPCWSTR name,
                DWORD reserved,
                LPWSTR class_name,
                DWORD options,
                REGSAM access,
                const LPSECURITY_ATTRIBUTES security,
                PHKEY key,
                LPDWORD disposition)
{
    (void)reserved;
    (void)class_name;
    (void)options;
    (void)access;
    (void)security;
    (void)disposition;
    CHECK(root == HKEY_CURRENT_USER && wcscmp(name, L"Software\\OpenVPN-GUI") == 0);
    *key = (HKEY)(UINT_PTR)123;
    return ERROR_SUCCESS;
}

static LSTATUS WINAPI
test_reg_set(HKEY key, LPCWSTR name, DWORD reserved, DWORD type, const BYTE *data, DWORD size)
{
    (void)reserved;
    CHECK(key == (HKEY)(UINT_PTR)123 && wcscmp(name, L"country_routing") == 0);
    CHECK(type == REG_BINARY && size <= sizeof(registry_data));
    memcpy(registry_data, data, size);
    registry_size = size;
    return ERROR_SUCCESS;
}

static LSTATUS WINAPI
test_reg_close(HKEY key)
{
    CHECK(key == (HKEY)(UINT_PTR)123);
    return ERROR_SUCCESS;
}

#define SHGetFolderPathW test_folder_path
#define RegCreateKeyExW  test_reg_create
#define RegSetValueExW   test_reg_set
#define RegCloseKey      test_reg_close
#include "../country_routing.c"
#undef SHGetFolderPathW
#undef RegCreateKeyExW
#undef RegSetValueExW
#undef RegCloseKey

options_t o;

PTSTR
LoadLocalizedString(const UINT id, ...)
{
    static WCHAR result[1024];
    WCHAR format[1024];
    CHECK(LoadStringW(o.hInstance, id, format, _countof(format)));
    va_list args;
    va_start(args, id);
    _vsnwprintf_s(result, _countof(result), _TRUNCATE, format, args);
    va_end(args);
    return result;
}

void
ShowLocalizedMsg(const UINT id, ...)
{
    last_error_message = id;
}

static BOOL CALLBACK
test_settings_init(PINIT_ONCE once, PVOID param, PVOID *context)
{
    (void)once;
    (void)param;
    (void)context;
    settings.version = 1;
    return TRUE;
}

static void
write_zone(const WCHAR *code, const char *data)
{
    WCHAR path[MAX_PATH];
    CHECK(zone_path(code, path, _countof(path)));
    FILE *file = _wfopen(path, L"wb");
    CHECK(file && fwrite(data, 1, strlen(data), file) == strlen(data));
    CHECK(fclose(file) == 0);
}

int
main(int argc, char **argv)
{
    WCHAR cwd[MAX_PATH], path[MAX_PATH], dir[MAX_PATH];
    CHECK(GetCurrentDirectoryW(_countof(cwd), cwd));
    CHECK(swprintf_s(test_base,
                     _countof(test_base),
                     L"%ls\\country-routing-test-%lu",
                     cwd,
                     GetCurrentProcessId())
          > 0);
    CHECK(CreateDirectoryW(test_base, NULL));
    o.hInstance = GetModuleHandleW(NULL);
    CHECK(InitOnceExecuteOnce(&settings_once, test_settings_init, NULL, NULL));
    CHECK(valid_countries(L"IR TR DE "));
    CHECK(!valid_countries(L"IR IR ") && !valid_countries(L"ZZ "));
    CHECK(!valid_countries(L"IR") && !valid_countries(L"../") && !valid_countries(L"ir "));

    if (argc == 2 && strcmp(argv[1], "--download") == 0)
    {
        HINTERNET session = WinHttpOpen(L"OpenVPN-GUI country routes test",
                                        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                        WINHTTP_NO_PROXY_NAME,
                                        WINHTTP_NO_PROXY_BYPASS,
                                        0);
        CHECK(session && zone_path(L"IR", path, _countof(path)));
        CHECK(WinHttpSetTimeouts(session, 10000, 10000, 10000, 10000));
        DWORD error = download_zone(session, L"IR", path);
        printf("Live IPdeny HTTPS download: error %lu\n", error);
        CHECK(error == ERROR_SUCCESS);
        WinHttpCloseHandle(session);
        country_routes_t actual = { 0 };
        FILE *zone = _wfopen(path, L"rb");
        CHECK(zone && CountryRoutesRead(zone, &actual) && actual.count > 100);
        printf("Validated %zu live Iranian CIDR ranges.\n", actual.count);
        fclose(zone);
        CountryRoutesFree(&actual);
    }

    connection_t c = { 0 };
    wcscpy_s(c.config_file, _countof(c.config_file), L"untouched.ovpn");
    CHECK(PrepareCountryRouting(&c));
    CHECK(!c.country_route_file[0]);
    settings.enabled = 1;
    wcscpy_s(settings.countries, _countof(settings.countries), L"IR TR ");
    write_zone(L"IR", "8.8.8.0/24\n");
    write_zone(L"TR", "8.8.9.0/24\n8.8.8.0/25\n");
    CHECK(PrepareCountryRouting(&c));
    CHECK(wcscmp(c.config_file, L"untouched.ovpn") == 0);
    CHECK(c.country_route_file[0] && c.country_route_lock);
    FILE *generated = _wfopen(c.country_route_file, L"rb");
    CHECK(generated); /* OpenVPN must be able to open the file despite the lock. */
    char text[512] = { 0 };
    CHECK(fread(text, 1, sizeof(text) - 1, generated));
    fclose(generated);
    CHECK(strstr(text, "route 8.8.8.0 255.255.254.0 net_gateway"));
    CHECK(!strstr(text, "remote ") && !strstr(text, "client\n"));
    HANDLE malicious = CreateFileW(
        c.country_route_file, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    CHECK(malicious == INVALID_HANDLE_VALUE && GetLastError() == ERROR_SHARING_VIOLATION);
    CHECK(!DeleteFileW(c.country_route_file) && GetLastError() == ERROR_SHARING_VIOLATION);
    wcscpy_s(path, _countof(path), c.country_route_file);
    CleanupCountryRouting(&c);
    CHECK(GetFileAttributesW(path) == INVALID_FILE_ATTRIBUTES);
    CHECK(!c.country_route_file[0] && !c.country_route_lock);
    CleanupCountryRouting(&c);

    write_zone(L"TR", "8.8.9.0/24\nup malicious\n");
    CHECK(!PrepareCountryRouting(&c));
    CHECK(last_error_message == IDS_COUNTRY_PREPARE_ERROR);
    CHECK(!c.country_route_file[0] && !c.country_route_lock);
    write_zone(L"TR", "8.8.9.0/24\n");
    o.ovpn_engine = OPENVPN_ENGINE_OVPN3;
    CHECK(!PrepareCountryRouting(&c) && last_error_message == IDS_COUNTRY_ENGINE_ERROR);
    o.ovpn_engine = OPENVPN_ENGINE_OVPN2;
    connection_t other = { .state = connected };
    o.chead = &other;
    CHECK(!PrepareCountryRouting(&c) && last_error_message == IDS_COUNTRY_MULTIPLE_ERROR);
    o.chead = NULL;

    INITCOMMONCONTROLSEX controls = { sizeof(controls), ICC_LISTVIEW_CLASSES };
    CHECK(InitCommonControlsEx(&controls));
    HRSRC resource =
        FindResourceW(o.hInstance, MAKEINTRESOURCEW(ID_DLG_COUNTRY_ROUTING), RT_DIALOG);
    CHECK(resource);
    DWORD size = SizeofResource(o.hInstance, resource);
    BYTE *dialog = malloc(size);
    CHECK(dialog);
    memcpy(dialog, LockResource(LoadResource(o.hInstance, resource)), size);
    /* DIALOGEX style is at byte 12: keep this test dialog hidden. */
    *(DWORD *)(dialog + 12) &= ~WS_VISIBLE;
    HWND hwnd = CreateDialogIndirectParamW(
        o.hInstance, (DLGTEMPLATE *)dialog, NULL, CountryRoutingDlgProc, 0);
    CHECK(hwnd);
    HWND list = GetDlgItem(hwnd, ID_LST_COUNTRIES);
    CHECK(ListView_GetItemCount(list) == (int)_countof(country_list));
    CHECK(IsDlgButtonChecked(hwnd, ID_CHK_COUNTRY_BYPASS) == BST_CHECKED);
    CHECK(IsWindowEnabled(GetDlgItem(hwnd, ID_EDT_COUNTRY_SEARCH)));
    CHECK(IsWindowEnabled(GetDlgItem(hwnd, ID_BTN_COUNTRY_SEARCH)));
    SetDlgItemTextW(hwnd, ID_EDT_COUNTRY_SEARCH, L"  ir  ");
    SendMessageW(hwnd, WM_COMMAND, ID_BTN_COUNTRY_SEARCH, 0);
    int found = ListView_GetNextItem(list, -1, LVNI_SELECTED);
    CHECK(found >= 0 && strcmp(country_list[found].code, "IR") == 0);
    SetDlgItemTextW(hwnd, ID_EDT_COUNTRY_SEARCH, L"tUrKeY");
    SendMessageW(hwnd, WM_COMMAND, ID_BTN_COUNTRY_SEARCH, 0);
    found = ListView_GetNextItem(list, -1, LVNI_SELECTED);
    CHECK(found >= 0 && strcmp(country_list[found].code, "TR") == 0);
    SetDlgItemTextW(hwnd, ID_EDT_COUNTRY_SEARCH, L"united");
    int first = -1, previous = -1, matches = 0;
    for (size_t i = 0; i < _countof(country_list); ++i)
    {
        if (wcsstr(country_list[i].name, L"United"))
        {
            ++matches;
        }
    }
    CHECK(matches > 1);
    for (int i = 0; i <= matches; ++i)
    {
        SendMessageW(hwnd, WM_COMMAND, ID_BTN_COUNTRY_SEARCH, 0);
        found = ListView_GetNextItem(list, -1, LVNI_SELECTED);
        CHECK(found >= 0 && wcsstr(country_list[found].name, L"United"));
        CHECK(found != previous);
        if (!i)
        {
            first = found;
        }
        previous = found;
    }
    CHECK(found == first);
    CHECK(find_country(list, L"not-a-country") == -1);
    SetDlgItemTextW(hwnd, ID_EDT_COUNTRY_SEARCH, L"   ");
    SendMessageW(hwnd, WM_COMMAND, ID_BTN_COUNTRY_SEARCH, 0);
    CHECK(ListView_GetNextItem(list, -1, LVNI_SELECTED) == first);
    page_enabled(hwnd, TRUE);
    CHECK(!IsWindowEnabled(GetDlgItem(hwnd, ID_EDT_COUNTRY_SEARCH)));
    CHECK(!IsWindowEnabled(GetDlgItem(hwnd, ID_BTN_COUNTRY_SEARCH)));
    page_enabled(hwnd, FALSE);
    for (size_t i = 0; i < _countof(country_list); ++i)
    {
        BOOL selected =
            strcmp(country_list[i].code, "IR") == 0 || strcmp(country_list[i].code, "TR") == 0;
        CHECK(!!ListView_GetCheckState(list, (int)i) == !!selected);
    }
    CheckDlgButton(hwnd, ID_CHK_COUNTRY_BYPASS, BST_UNCHECKED);
    SendMessageW(hwnd, WM_COMMAND, ID_CHK_COUNTRY_BYPASS, 0);
    CHECK(!IsWindowEnabled(list));
    CHECK(!IsWindowEnabled(GetDlgItem(hwnd, ID_EDT_COUNTRY_SEARCH)));
    CHECK(!IsWindowEnabled(GetDlgItem(hwnd, ID_BTN_COUNTRY_SEARCH)));
    PSHNOTIFY notify = { .hdr = { .code = PSN_APPLY } };
    SendMessageW(hwnd, WM_NOTIFY, 0, (LPARAM)&notify);
    CHECK(GetWindowLongPtrW(hwnd, DWLP_MSGRESULT) == PSNRET_NOERROR);
    CHECK(registry_size == sizeof(settings));
    CHECK(!settings.enabled && wcscmp(settings.countries, L"IR TR ") == 0);
    CHECK(memcmp(registry_data, &settings, sizeof(settings)) == 0);
    DestroyWindow(hwnd);
    free(dialog);

    CHECK(zone_path(L"IR", path, _countof(path)) && DeleteFileW(path));
    CHECK(zone_path(L"TR", path, _countof(path)) && DeleteFileW(path));
    CHECK(cache_directory(dir, _countof(dir)) && RemoveDirectoryW(dir));
    CHECK(swprintf_s(dir, _countof(dir), L"%ls\\OpenVPN-GUI", test_base) > 0);
    CHECK(RemoveDirectoryW(dir) && RemoveDirectoryW(test_base));
    puts("Windows routing file locking/cleanup, validation, UI, and global preferences passed.");
    return 0;
}
