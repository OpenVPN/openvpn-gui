/*
 *  OpenVPN-GUI -- A Windows GUI for OpenVPN.
 *
 *  Copyright (C) 2009 Heiko Hund <heikoh@users.sf.net>
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
#include <windowsx.h>
#include <commctrl.h>
#include <prsht.h>
#include <tchar.h>
#include <stdio.h>
#include <stdarg.h>
#include <malloc.h>

#include "main.h"
#include "localization.h"
#include "openvpn-gui-res.h"
#include "options.h"
#include "registry.h"

extern options_t o;

static const LANGID fallbackLangId = MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT);
static LANGID gui_language;

static HRSRC
FindResourceLang(PTSTR resType, PTSTR resId, LANGID langId)
{
    HRSRC res;

    /* try to find the resource in requested language */
    res = FindResourceEx(o.hInstance, resType, resId, langId);
    if (res)
        return res;

    /* try to find the resource in the default sublanguage */
    LANGID defLangId = MAKELANGID(PRIMARYLANGID(langId), SUBLANG_DEFAULT);
    res = FindResourceEx(o.hInstance, resType, resId, defLangId);
    if (res)
        return res;

    /* try to find the resource in the default language */
    res = FindResourceEx(o.hInstance, resType, resId, fallbackLangId);
    if (res)
        return res;

    /* try to find the resource in any language */
    return FindResource(o.hInstance, resId, resType);
}


LANGID
GetGUILanguage(void)
{
    if (gui_language != 0)
        return gui_language;

    HKEY regkey;
    DWORD value = 0;

    LONG status = RegOpenKeyEx(HKEY_CURRENT_USER, GUI_REGKEY_HKCU, 0, KEY_READ, &regkey);
    if (status == ERROR_SUCCESS)
        GetRegistryValueNumeric(regkey, _T("ui_language"), &value);

    gui_language = ( value != 0 ? value : GetUserDefaultUILanguage() );
    InitMUILanguage(gui_language);
    return gui_language;
}


static void
SetGUILanguage(LANGID langId)
{
    HKEY regkey;
    if (RegCreateKeyEx(HKEY_CURRENT_USER, GUI_REGKEY_HKCU, 0, NULL, 0,
        KEY_WRITE, NULL, &regkey, NULL) != ERROR_SUCCESS )
        ShowLocalizedMsg(IDS_ERR_CREATE_REG_HKCU_KEY, GUI_REGKEY_HKCU);

    SetRegistryValueNumeric(regkey, _T("ui_language"), langId);
    InitMUILanguage(langId);
    gui_language = langId;
}

int
LocalizedTime(const time_t t, LPTSTR buf, size_t size)
{
    /* Convert Unix timestamp to Win32 SYSTEMTIME */
    FILETIME lft;
    SYSTEMTIME st;
    LONGLONG tmp = Int32x32To64(t, 10000000) + 116444736000000000;
    FILETIME ft = { .dwLowDateTime = (DWORD) tmp, .dwHighDateTime = tmp >> 32};
    FileTimeToLocalFileTime(&ft, &lft);
    FileTimeToSystemTime(&lft, &st);

    int date_size = 0, time_size = 0;
    LCID locale = MAKELCID(GetGUILanguage(), SORT_DEFAULT);

    if (size > 0) {
        date_size = GetDateFormat(locale, DATE_SHORTDATE, &st, NULL,
                                  buf, size);
        if (date_size)
            buf[date_size - 1] = ' ';
    }
    if (size - date_size > 0) {
        time_size = GetTimeFormat(locale, TIME_NOSECONDS, &st, NULL,
                                  buf + date_size, size - date_size);
    }
    return date_size + time_size;
}


static int
LoadStringLang(UINT stringId, LANGID langId, PTSTR buffer, int bufferSize, va_list args)
{
    PWCH entry;
    PTSTR resBlockId = MAKEINTRESOURCE(stringId / 16 + 1);
    int resIndex = stringId & 15;

    /* find resource block for string */
    HRSRC res = FindResourceLang(RT_STRING, resBlockId, langId);
    if (res == NULL)
        goto err;

    /* get pointer to first entry in resource block */
    entry = (PWCH) LoadResource(o.hInstance, res);
    if (entry == NULL)
        goto err;

    /* search for string in block */
    for (int i = 0; i < 16; i++)
    {
        /* skip over this entry */
        if (i != resIndex)
        {
            entry += ((*entry) + 1);
            continue;
        }

        /* string does not exist */
        if (i == resIndex && *entry == 0)
            break;

        /* string found, copy it */
        PTSTR formatStr = (PTSTR) malloc((*entry + 1) * sizeof(TCHAR));
        if (formatStr == NULL)
            break;
        formatStr[*entry] = 0;

        wcsncpy(formatStr, entry + 1, *entry);
        _vsntprintf(buffer, bufferSize, formatStr, args);
        buffer[bufferSize - 1] = 0;
        free(formatStr);
        return _tcslen(buffer);
    }

err:
    /* not found, try again with the default language */
    if (langId != fallbackLangId)
        return LoadStringLang(stringId, fallbackLangId, buffer, bufferSize, args);

    return 0;
}


static PTSTR
__LoadLocalizedString(const UINT stringId, va_list args)
{
    static TCHAR msg[512];
    msg[0] = 0;
    LoadStringLang(stringId, GetGUILanguage(), msg, _countof(msg), args);
    return msg;
}


PTSTR
LoadLocalizedString(const UINT stringId, ...)
{
    va_list args;
    va_start(args, stringId);
    PTSTR str = __LoadLocalizedString(stringId, args);
    va_end(args);
    return str;
}


int
LoadLocalizedStringBuf(PTSTR buffer, int bufferSize, const UINT stringId, ...)
{
    va_list args;
    va_start(args, stringId);
    int len = LoadStringLang(stringId, GetGUILanguage(), buffer, bufferSize, args);
    va_end(args);
    return len;
}


static int
__ShowLocalizedMsgEx(const UINT type, LPCTSTR caption, const UINT stringId, va_list args)
{
    return MessageBoxEx(NULL, __LoadLocalizedString(stringId, args), caption,
        type | MB_SETFOREGROUND, GetGUILanguage());
}


int
ShowLocalizedMsgEx(const UINT type, LPCTSTR caption, const UINT stringId, ...)
{
    va_list args;
    va_start(args, stringId);
    int result = __ShowLocalizedMsgEx(type, caption, stringId, args);
    va_end(args);
    return result;
}


void
ShowLocalizedMsg(const UINT stringId, ...)
{
    va_list args;
    va_start(args, stringId);
    __ShowLocalizedMsgEx(MB_OK, _T(PACKAGE_NAME), stringId, args);
    va_end(args);
}

static HICON
LoadLocalizedIconEx(const UINT iconId, int cxDesired, int cyDesired)
{
    LANGID langId = GetGUILanguage();

    HICON hIcon =
            (HICON) LoadImage (o.hInstance, MAKEINTRESOURCE(iconId),
                    IMAGE_ICON, cxDesired, cyDesired, LR_DEFAULTSIZE|LR_SHARED);
    if (hIcon)
        return hIcon;
    else
        PrintDebug (L"Loading icon using LoadImage failed.");

    /* Fallback to CreateIconFromResource which always scales
     * from the first image in the resource
     */
    /* find group icon resource */
    HRSRC res = FindResourceLang(RT_GROUP_ICON, MAKEINTRESOURCE(iconId), langId);
    if (res == NULL)
        return NULL;

    HGLOBAL resInfo = LoadResource(o.hInstance, res);
    if (resInfo == NULL)
        return NULL;

    int id = LookupIconIdFromDirectory(resInfo, TRUE);
    if (id == 0)
        return NULL;

    /* find the actual icon */
    res = FindResourceLang(RT_ICON, MAKEINTRESOURCE(id), langId);
    if (res == NULL)
        return NULL;

    resInfo = LoadResource(o.hInstance, res);
    if (resInfo == NULL)
        return NULL;

    DWORD resSize = SizeofResource(o.hInstance, res);
    if (resSize == 0)
        return NULL;

    /* Note: this uses the first icon in the resource and scales it */
    hIcon = CreateIconFromResourceEx(resInfo, resSize, TRUE, 0x30000,
            cxDesired, cyDesired, LR_DEFAULTSIZE|LR_SHARED);
    return hIcon;
}

HICON
LoadLocalizedIcon(const UINT iconId)
{
   /* get the required normal icon size (e.g., taskbar icon) */
   int cx = GetSystemMetrics(SM_CXICON);
   int cy = GetSystemMetrics(SM_CYICON);
   return LoadLocalizedIconEx(iconId, cx, cy);
}

HICON
LoadLocalizedSmallIcon(const UINT iconId)
{
   /* get the required small icon size (e.g., tray icon) */
   int cx = GetSystemMetrics(SM_CXSMICON);
   int cy = GetSystemMetrics(SM_CYSMICON);
   return LoadLocalizedIconEx(iconId, cx, cy);
}

LPCDLGTEMPLATE
LocalizedDialogResource(const UINT dialogId)
{
    /* find dialog resource */
    HRSRC res = FindResourceLang(RT_DIALOG, MAKEINTRESOURCE(dialogId), GetGUILanguage());
    if (res == NULL)
        return NULL;

    return LoadResource(o.hInstance, res);
}


INT_PTR
LocalizedDialogBoxParam(const UINT dialogId, DLGPROC dialogFunc, const LPARAM param)
{
    LPCDLGTEMPLATE resInfo = LocalizedDialogResource(dialogId);
    if (resInfo == NULL)
        return -1;

    return DialogBoxIndirectParam(o.hInstance, resInfo, NULL, dialogFunc, param);
}


HWND
CreateLocalizedDialogParam(const UINT dialogId, DLGPROC dialogFunc, const LPARAM param)
{
    /* find dialog resource */
    HRSRC res = FindResourceLang(RT_DIALOG, MAKEINTRESOURCE(dialogId), GetGUILanguage());
    if (res == NULL)
        return NULL;

    HGLOBAL resInfo = LoadResource(o.hInstance, res);
    if (resInfo == NULL)
        return NULL;

    return CreateDialogIndirectParam(o.hInstance, resInfo, NULL, dialogFunc, param);
}


HWND
CreateLocalizedDialog(const UINT dialogId, DLGPROC dialogFunc)
{
    return CreateLocalizedDialogParam(dialogId, dialogFunc, 0);
}


static PTSTR
LangListEntry(const UINT stringId, const LANGID langId, ...)
{
    static TCHAR str[128];
    va_list args;

    va_start(args, langId);
    LoadStringLang(stringId, langId, str, _countof(str), args);
    va_end(args);
    return str;
}


typedef struct {
    HWND languages;
    LANGID language;
} langProcData;


static BOOL
FillLangListProc(UNUSED HANDLE module, UNUSED PTSTR type, UNUSED PTSTR stringId, WORD langId, LONG_PTR lParam)
{
    langProcData *data = (langProcData*) lParam;

    int index = ComboBox_AddString(data->languages, LangListEntry(IDS_LANGUAGE_NAME, langId));
    ComboBox_SetItemData(data->languages, index, langId);

    /* Select this item if it is the currently displayed language */
    if (langId == data->language
    ||  (PRIMARYLANGID(langId) == PRIMARYLANGID(data->language)
     && ComboBox_GetCurSel(data->languages) == CB_ERR) )
        ComboBox_SetCurSel(data->languages, index);

    return TRUE;
}

static BOOL 
GetLaunchOnStartup()
{
	
    WCHAR regPath[MAX_PATH], exePath[MAX_PATH];	
    BOOL result = FALSE;
    HKEY regkey;

    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_READ, &regkey) == ERROR_SUCCESS) {

        if (GetRegistryValue(regkey, L"OpenVPN-GUI", regPath, MAX_PATH) &&
            GetModuleFileNameW(NULL, exePath, MAX_PATH)) {
            if (_wcsicmp(regPath, exePath) == 0)
                result = TRUE;
        }

        RegCloseKey(regkey);

    }
	
    return result;

}

static void
SetLaunchOnStartup(BOOL value) 
{

    WCHAR exePath[MAX_PATH];
    HKEY regkey;

    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_WRITE, &regkey) == ERROR_SUCCESS) {

        if (value) {
            if (GetModuleFileNameW(NULL, exePath, MAX_PATH)) 
                SetRegistryValue(regkey, L"OpenVPN-GUI", exePath);
        }
        else 
            RegDeleteValue(regkey, L"OpenVPN-GUI");            

        RegCloseKey(regkey);

    }

}

INT_PTR CALLBACK
GeneralSettingsDlgProc(HWND hwndDlg, UINT msg, UNUSED WPARAM wParam, LPARAM lParam)
{
    LPPSHNOTIFY psn;
    langProcData langData = {
        .languages = GetDlgItem(hwndDlg, ID_CMB_LANGUAGE),
        .language = GetGUILanguage()
    };

    switch(msg) {

    case WM_INITDIALOG:
        /* Populate UI language selection combo box */
        EnumResourceLanguages( NULL, RT_STRING, MAKEINTRESOURCE(IDS_LANGUAGE_NAME / 16 + 1),
            (ENUMRESLANGPROC) FillLangListProc, (LONG_PTR) &langData );

        /* If none of the available languages matched, select the fallback */
        if (ComboBox_GetCurSel(langData.languages) == CB_ERR)
            ComboBox_SelectString(langData.languages, -1,
                LangListEntry(IDS_LANGUAGE_NAME, fallbackLangId));

        /* Clear language id data for the selected item */
        ComboBox_SetItemData(langData.languages, ComboBox_GetCurSel(langData.languages), 0);

        if (GetLaunchOnStartup())
            Button_SetCheck(GetDlgItem(hwndDlg, ID_CHK_STARTUP), BST_CHECKED);

        if (o.log_append)
            Button_SetCheck(GetDlgItem(hwndDlg, ID_CHK_LOG_APPEND), BST_CHECKED);
        if (o.silent_connection)
            Button_SetCheck(GetDlgItem(hwndDlg, ID_CHK_SILENT), BST_CHECKED);
        if (o.show_balloon == 0)
            CheckRadioButton (hwndDlg, ID_RB_BALLOON0, ID_RB_BALLOON2, ID_RB_BALLOON0);
        else if (o.show_balloon == 1)
            CheckRadioButton (hwndDlg, ID_RB_BALLOON0, ID_RB_BALLOON2, ID_RB_BALLOON1);
        else if (o.show_balloon == 2)
            CheckRadioButton (hwndDlg, ID_RB_BALLOON0, ID_RB_BALLOON2, ID_RB_BALLOON2);
        if (o.show_script_window)
            Button_SetCheck(GetDlgItem(hwndDlg, ID_CHK_SHOW_SCRIPT_WIN), BST_CHECKED);

        break;

    case WM_NOTIFY:
        psn = (LPPSHNOTIFY) lParam;
        if (psn->hdr.code == (UINT) PSN_APPLY)
        {
            LANGID langId = (LANGID) ComboBox_GetItemData(langData.languages,
                ComboBox_GetCurSel(langData.languages));

            if (langId != 0)
                SetGUILanguage(langId);

            SetLaunchOnStartup(Button_GetCheck(GetDlgItem(hwndDlg, ID_CHK_STARTUP)) == BST_CHECKED);

            o.log_append =
                (Button_GetCheck(GetDlgItem(hwndDlg, ID_CHK_LOG_APPEND)) == BST_CHECKED);
            o.silent_connection =
                (Button_GetCheck(GetDlgItem(hwndDlg, ID_CHK_SILENT)) == BST_CHECKED);
            if (IsDlgButtonChecked(hwndDlg, ID_RB_BALLOON0))
                o.show_balloon = 0;
            else if (IsDlgButtonChecked(hwndDlg, ID_RB_BALLOON2))
                o.show_balloon = 2;
            else
                o.show_balloon = 1;
            o.show_script_window =
                (Button_GetCheck(GetDlgItem(hwndDlg, ID_CHK_SHOW_SCRIPT_WIN)) == BST_CHECKED);

            SaveRegistryKeys();

            SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
            return TRUE;
        }
        break;
    }

    return FALSE;
}
