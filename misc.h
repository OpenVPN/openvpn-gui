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

#include <wincrypt.h>

#include "options.h"

BOOL ManagementCommandFromInput(connection_t *, LPCSTR, HWND, int);

BOOL ManagementCommandFromTwoInputsBase64(connection_t *, LPCSTR, HWND, int, int);

BOOL ManagementCommandFromInputBase64(connection_t *, LPCSTR, HWND, int);

BOOL EnsureDirExists(LPTSTR);

BOOL streq(LPCSTR, LPCSTR);

BOOL strbegins(const char *str, const char *begin);

BOOL wcsbegins(LPCWSTR, LPCWSTR);

BOOL ForceForegroundWindow(HWND);

void DpiSetScale(options_t *, UINT dpix);

BOOL IsUserAdmin(VOID);

HANDLE InitSemaphore(WCHAR *);

BOOL CheckFileAccess(const TCHAR *path, int access);

BOOL Base64Encode(const char *input, int input_len, char **output);

int Base64Decode(const char *input, char **output);

WCHAR *Widen(const char *utf8);

WCHAR *WidenEx(UINT codepage, const char *utf8);

/**
 * Convert a wide string to a UTF-8 string. The caller must
 * free the returned pointer. Return NULL on error.
 */
char *WCharToUTF8(const WCHAR *wstr);

BOOL validate_input(const WCHAR *input, const WCHAR *exclude);

/* Concatenate two wide strings with a separator */
void wcs_concat2(WCHAR *dest, int len, const WCHAR *src1, const WCHAR *src2, const WCHAR *sep);

void CloseSemaphore(HANDLE sem);

/* Close a handle if not null or invalid */
void CloseHandleEx(LPHANDLE h);

/* Decode url encoded charcters in src and return the result as a newly
 * allocated string. Returns NULL on error.
 */
char *url_decode(const char *src);

/* digest functions */
typedef struct md_ctx
{
    HCRYPTPROV prov;
    HCRYPTHASH hash;
} md_ctx;

DWORD md_init(md_ctx *ctx, ALG_ID hash_type);

DWORD md_update(md_ctx *ctx, const BYTE *data, size_t size);

DWORD md_final(md_ctx *ctx, BYTE *md);

/* Open specified http/https URL using ShellExecute. */
BOOL open_url(const wchar_t *url);

void ImportConfigFile(const TCHAR *path, bool prompt_user);

/*
 * Helper function to convert UCS-2 text from a dialog item to UTF-8.
 * Caller must free *str if *len != 0.
 */
BOOL GetDlgItemTextUtf8(HWND hDlg, int id, LPSTR *str, int *len);

/* Return escaped copy of a string */
char *escape_string(const char *str);

/**
 * Find a free port to bind to
 * @param addr : Address to bind to -- if port >0 it's tried first.
 *               On return the port is set to the one found.
 * @returns true on success, false on error. In case of error
 * addr is unchanged.
 */
BOOL find_free_tcp_port(SOCKADDR_IN *addr);

/**
 * Parse the config file of a connection profile for
 * Managegment address and password.
 * @param c : Pointer to connection profile
 *            On return c->manage.skaddr and c->manage.password
 *            are populated.
 * @returns true on success false on error.
 * Password not specified in the config file is not an error.
 */
BOOL ParseManagementAddress(connection_t *c);

/**
 * Get dpi of the system and set the scale factor.
 * @param o : pointer to the options struct
 * On return initializes o.dpi_scale using the logical pixels
 * per inch value of the system.
 */
#define DPI_SCALE(x) MulDiv(x, o.dpi_scale, 100)
void dpi_initialize(options_t *o);

/**
 * Write a message to the event log
 * @param type : event log type
 * @param format : message format in printf style
 * @param ...    : extra args
 */
void MsgToEventLog(WORD type, wchar_t *format, ...);

/**
 * Check PLAP COM object is is registered
 * @returns 1 if yes, 0 if no, or -1 if PLAP dll not installed.
 */
int GetPLAPRegistrationStatus(void);

/**
 * Register/Unregister PLAP COM object
 * @param action TRUE to register, FALSE to unregister
 * @returns 0 on success or a non-zero error code on error.
 * Requires admin privileges -- user will prompted for admin
 * credentials or UAC consent if required.
 */
DWORD SetPLAPRegistration(BOOL action);

/**
 * Run a command as admin using shellexecute
 * @param cmd The command to run
 * @param params Parameters to the command
 * @returns 0 on success or a non-zero exit code from the
 * command. If the command fails to startup, -1 is returned.
 */
DWORD RunAsAdmin(const WCHAR *cmd, const WCHAR *params);

/**
 * Wait for a timeout while pumping messages. If hdlg is not NULL
 * IsDialogMessage(hdlg, ...) is checked before dispatching messages.
 * caller can install a WH_MSGFILTER hook if any other special processing
 * is necessary. The hook will get called with ncode = MSGF_OVPN_WAIT.
 * @returns false if WM_QUIT was received, else returns true on timeout.
 */
bool OVPNMsgWait(DWORD timeout, HWND hdlg);

bool GetRandomPassword(char *buf, size_t len);

void ResetPasswordReveal(HWND edit, HWND btn, WPARAM wParam);

void ChangePasswordVisibility(HWND edit, HWND btn, WPARAM wParam);

#endif /* ifndef MISC_H */
