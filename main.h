/*
 *  OpenVPN-GUI -- A Windows GUI for OpenVPN.
 *
 *  Copyright (C) 2004 Mathias Sundman <mathias@nilings.se>
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

#ifndef MAIN_H
#define MAIN_H

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <tchar.h>

/* Define this to enable DEBUG build */
//#define DEBUG
#define DEBUG_FILE	L"C:\\windows\\temp\\openvpngui_debug.txt"

/* Define this to disable Change Password support */
//#define DISABLE_CHANGE_PASSWORD

/* Registry key for User Settings */
#define GUI_REGKEY_HKCU	_T("Software\\OpenVPN-GUI")

#define MAX_LOG_LENGTH      1024/* Max number of characters per log line */
#define MAX_LOG_LINES		500	/* Max number of lines in LogWindow */
#define DEL_LOG_LINES		10	/* Number of lines to delete from LogWindow */

/* Authorized group who can use any options and config locations */
#define OVPN_ADMIN_GROUP TEXT("OpenVPN Administrators") /* May be reset in registry */

/* bool definitions */
#define bool int
#define true 1
#define false 0

/* GCC function attributes */
#define UNUSED __attribute__ ((unused))
#define NORETURN __attribute__ ((noreturn))

#define PACKVERSION(major,minor) MAKELONG(minor,major)
struct security_attributes
{
  SECURITY_ATTRIBUTES sa;
  SECURITY_DESCRIPTOR sd;
};

/* clear an object */
#define CLEAR(x) memset(&(x), 0, sizeof(x))

/* _sntprintf with guaranteed \0 termination */
#define _sntprintf_0(buf, ...) \
  do { \
    __sntprintf_0(buf, _countof(buf), __VA_ARGS__); \
  } while(0);

static inline int
__sntprintf_0(TCHAR *buf, size_t size, TCHAR *format, ...)
{
    int i;
    va_list args;
    va_start(args, format);
    i = _vsntprintf(buf, size, format, args);
    buf[size - 1] = _T('\0');
    va_end(args);
    return i;
}

/* _snprintf with guaranteed \0 termination */
#define _snprintf_0(buf, ...) \
  do { \
    __snprintf_0(buf, sizeof(buf), __VA_ARGS__); \
  } while(0);
static inline int
__snprintf_0(char *buf, size_t size, char *format, ...)
{
    int i;
    va_list args;
    va_start(args, format);
    i = _vsnprintf(buf, size, format, args);
    buf[size - 1] = '\0';
    va_end(args);
    return i;
}

#ifdef DEBUG
/* Print Debug Message */
#define PrintDebug(...) \
        do { \
           TCHAR x_msg[256]; \
           _sntprintf_0(x_msg, __VA_ARGS__); \
           PrintDebugMsg(x_msg); \
        } while(0)

void PrintDebugMsg(TCHAR *msg);
#else
#define PrintDebug(...) do { } while(0)
#endif

DWORD GetDllVersion(LPCTSTR lpszDllName);

void ImportConfigFile(PTCHAR);

#endif
