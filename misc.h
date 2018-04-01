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

#ifndef MISC_H
#define MISC_H

BOOL ManagementCommandFromInput(connection_t *, LPCSTR, HWND, int);
BOOL ManagementCommandFromInputBase64(connection_t *, LPCSTR, HWND, int, int);

BOOL EnsureDirExists(LPTSTR);

BOOL streq(LPCSTR, LPCSTR);
BOOL strbegins(const char *str, const char *begin);
BOOL wcsbegins(LPCWSTR, LPCWSTR);

BOOL ForceForegroundWindow(HWND);

BOOL IsUserAdmin(VOID);
HANDLE InitSemaphore (WCHAR *);
BOOL CheckFileAccess (const TCHAR *path, int access);

BOOL Base64Encode(const char *input, int input_len, char **output);
int Base64Decode(const char *input, char **output);
WCHAR *Widen(const char *utf8);
BOOL validate_input(const WCHAR *input, const WCHAR *exclude);
/* Concatenate two wide strings with a separator */
void wcs_concat2(WCHAR *dest, int len, const WCHAR *src1, const WCHAR *src2, const WCHAR *sep);
void CloseSemaphore(HANDLE sem);

#endif
