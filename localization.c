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

/* for GetUserDefaultUILanguage() */
#include <w32api.h>
#define WINVER Windows2000

#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <stdarg.h>
#include "options.h"
#include "registry.h"

extern struct options o;

static const LANGID defaultLangId = MAKELANGID(LANG_ENGLISH, SUBLANG_NEUTRAL);

static HRSRC
FindResourceLang(PTSTR resType, PTSTR resId, LANGID langId)
{
    HRSRC res;

    /* try to find the resource in requested language */
    res = FindResourceEx(o.hInstance, resType, resId, langId);
    if (res)
        return res;

    /* try to find the resource in the default language */
    res = FindResourceEx(o.hInstance, resType, resId, defaultLangId);
    if (res)
        return res;

    /* try to find the resource in any language */
    return FindResource(o.hInstance, resId, resType);
}


static LANGID
GetGUILanguage(void)
{
    return GetUserDefaultUILanguage();
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
        return 0;

    /* get pointer to first entry in resource block */
    entry = (PWCH) LoadResource(o.hInstance, res);
    if (entry == NULL)
        return 0;

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

#ifdef _UNICODE
        wcsncpy(formatStr, entry + 1, *entry);
#else
        WideCharToMultiByte(CP_ACP, 0, entry + 1, *entry, formatStr, *entry, "?", NULL);
#endif

        _vsntprintf(buffer, bufferSize, formatStr, args);
        buffer[bufferSize - 1] = 0;
        free(formatStr);
        return _tcslen(buffer);
    }

    return 0;
}


static PTSTR
__LoadLocalizedString(const UINT stringId, va_list args)
{
    static TCHAR msg[512];
    msg[0] = 0;
    LoadStringLang(stringId, GetGUILanguage(), msg, sizeof(msg)/sizeof(*msg), args);
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


void
ShowLocalizedMsg(const PTSTR caption, const UINT stringId, ...)
{
    va_list args;
    va_start(args, stringId);
    MessageBox(NULL, __LoadLocalizedString(stringId, args), caption, MB_OK | MB_SETFOREGROUND);
    va_end(args);
}


HICON
LoadLocalizedIcon(const UINT iconId)
{
    LANGID langId = GetGUILanguage();

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

    return CreateIconFromResource(resInfo, resSize, TRUE, 0x30000);
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


INT_PTR
LocalizedDialogBox(const UINT dialogId, DLGPROC dialogFunc)
{
    return LocalizedDialogBoxParam(dialogId, dialogFunc, 0);
}


HWND
CreateLocalizedDialog(const UINT dialogId, DLGPROC dialogFunc)
{
    /* find dialog resource */
    HRSRC res = FindResourceLang(RT_DIALOG, MAKEINTRESOURCE(dialogId), GetGUILanguage());
    if (res == NULL)
        return NULL;

    HGLOBAL resInfo = LoadResource(o.hInstance, res);
    if (resInfo == NULL)
        return NULL;

    return CreateDialogIndirect(o.hInstance, resInfo, NULL, dialogFunc);
}
