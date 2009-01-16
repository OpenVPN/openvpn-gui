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

#include <windows.h>
#include <tchar.h>

static const LANGID defaultLangId = MAKELANGID(LANG_ENGLISH, SUBLANG_NEUTRAL);

static HRSRC
FindResourceLang(HINSTANCE instance, PTSTR resType, PTSTR resId, LANGID langId)
{
    HRSRC res;

    /* try to find the resource in requested language */
    res = FindResourceEx(instance, resType, resId, langId);
    if (res)
        return res;

    /* try to find the resource in the default language */
    res = FindResourceEx(instance, resType, resId, defaultLangId);
    if (res)
        return res;

    /* try to find the resource in any language */
    return FindResource(instance, resId, resType);
}


int
LoadStringLang(HINSTANCE instance, UINT stringId, LANGID langId, PTSTR buffer, int bufferSize)
{
    PWCH entry;
    PTSTR resBlockId = MAKEINTRESOURCE(stringId / 16 + 1);
    int resIndex = stringId & 15;

    /* find resource block for string */
    HRSRC res = FindResourceLang(instance, RT_STRING, resBlockId, langId);
    if (res == NULL)
        return 0;

    /* get pointer to first entry in resource block */
    entry = (PWCH) LoadResource(instance, res);
    if (entry == NULL)
        return 0;

    /* search for string in block */
    for (int i = 0; i < 16; i++)
    {
        /* string found, copy it */
        if (i == resIndex && *entry)
        {
            int maxLen = (*entry < bufferSize ? *entry : bufferSize);
#ifdef _UNICODE
            wcsncpy(buffer, entry + 1, maxLen);
#else
            WideCharToMultiByte(CP_ACP, 0, entry + 1, *entry, buffer, maxLen, "?", NULL);
#endif
            buffer[bufferSize - 1] = 0;
            return _tcslen(buffer);
        }

        /* string does not exist */
        if (i == resIndex)
            break;

        /* skip over this entry */
        entry += ((*entry) + 1);
    }

    return 0;
}


HICON
LoadIconLang(HINSTANCE instance, PTSTR iconId, LANGID langId)
{
    /* find group icon resource */
    HRSRC res = FindResourceLang(instance, RT_GROUP_ICON, iconId, langId);
    if (res == NULL)
        return NULL;

    HGLOBAL resInfo = LoadResource(instance, res);
    if (resInfo == NULL)
        return NULL;

    int id = LookupIconIdFromDirectory(resInfo, TRUE);
    if (id == 0)
        return NULL;

    /* find the actual icon */
    res = FindResourceLang(instance, RT_ICON, MAKEINTRESOURCE(id), langId);
    if (res == NULL)
        return NULL;

    resInfo = LoadResource(instance, res);
    if (resInfo == NULL)
        return NULL;

    DWORD resSize = SizeofResource(instance, res);
    if (resSize == 0)
        return NULL;

    return CreateIconFromResource(resInfo, resSize, TRUE, 0x30000);
}

INT_PTR
DialogBoxLang(HINSTANCE instance, PTSTR dialogId, LANGID langId, HWND parentWnd, DLGPROC dialogFunc)
{
    /* find dialog resource */
    HRSRC res = FindResourceLang(instance, RT_DIALOG, dialogId, langId);
    if (res == NULL)
        return -1;

    HGLOBAL resInfo = LoadResource(instance, res);
    if (resInfo == NULL)
        return -1;

    return DialogBoxIndirect(instance, resInfo, parentWnd, dialogFunc);
}
