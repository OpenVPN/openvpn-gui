/*
 *  OpenVPN-PLAP-Provider
 *
 *  Copyright (C) 2017-2022 Selva Nair <selva.nair@gmail.com>
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

#include "plap_common.h"
#include <windows.h>
#include <shlwapi.h>
#include <stdio.h>
#include <time.h>

#include "localization.h"
static FILE *fp;
static CRITICAL_SECTION log_write;

void
init_debug()
{
    if (!fp)
    {
        _wfopen_s(&fp, L"C:\\Windows\\Temp\\openvpn-plap-debug.txt", L"a+,ccs=UTF-8");
    }
    InitializeCriticalSection(&log_write);
}

void
uninit_debug()
{
    if (fp)
    {
        fclose(fp);
    }
    DeleteCriticalSection(&log_write);
}

void
x_dmsg(const char *file, const char *func, const wchar_t *fmt, ...)
{
    wchar_t buf[1024];
    if (!fp)
    {
        return;
    }

    va_list args;
    va_start(args, fmt);
    vswprintf(buf, _countof(buf), fmt, args);
    va_end(args);

    wchar_t date[30];
    time_t log_time = time(NULL);
    struct tm *time_struct = localtime(&log_time);
    _snwprintf_s(date,
                 _countof(date),
                 _TRUNCATE,
                 L"%d-%.2d-%.2d %.2d:%.2d:%.2d",
                 time_struct->tm_year + 1900,
                 time_struct->tm_mon + 1,
                 time_struct->tm_mday,
                 time_struct->tm_hour,
                 time_struct->tm_min,
                 time_struct->tm_sec);

    EnterCriticalSection(&log_write);

    if (file && func)
    {
        fwprintf(fp, L"%ls %hs:%hs: %ls\n", date, file, func, buf);
    }
    else
    {
        fwprintf(fp, L"%ls %ls\n", date, buf);
    }
    fflush(fp);

    LeaveCriticalSection(&log_write);
}

void
debug_print_guid(const GUID *riid, const wchar_t *context)
{
    RPC_CSTR str = NULL;
    if (UuidToStringA((GUID *)riid, &str) == RPC_S_OK)
    {
        x_dmsg(NULL, NULL, L"%ls %hs", context, str);
        RpcStringFreeA(&str);
    }
}
