/*
 *  OpenVPN-GUI -- A Windows GUI for OpenVPN.
 *
 *  Copyright (C) 2004 Mathias Sundman <mathias@nilings.se>
 *                2010 Heiko Hund <heikoh@users.sf.net>
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

#ifndef OPENVPN_H
#define OPENVPN_H

#include "options.h"

#define TRY_SETPROP(hwnd, name, p)                                                               \
    do                                                                                           \
    {                                                                                            \
        if (SetPropW(hwnd, name, p))                                                             \
            break;                                                                               \
        MsgToEventLog(EVENTLOG_ERROR_TYPE, L"%hs:%d GetProp returned null", __func__, __LINE__); \
        EndDialog(hwnd, IDABORT);                                                                \
        return false;                                                                            \
    } while (0)

BOOL StartOpenVPN(connection_t *);

void StopOpenVPN(connection_t *);

void DetachOpenVPN(connection_t *);

void SuspendOpenVPN(int config);

void RestartOpenVPN(connection_t *);

void ReleaseOpenVPN(connection_t *);

BOOL CheckVersion();

void SetStatusWinIcon(HWND hwndDlg, int IconID);

void OnReady(connection_t *, char *);

void OnHold(connection_t *, char *);

void OnLogLine(connection_t *, char *);

void OnStateChange(connection_t *, char *);

void OnPassword(connection_t *, char *);

void OnStop(connection_t *, char *);

void OnNeedOk(connection_t *, char *);

void OnNeedStr(connection_t *, char *);

void OnEcho(connection_t *, char *);

void OnByteCount(connection_t *, char *);

void OnInfoMsg(connection_t *, char *);

void OnTimeout(connection_t *, char *);

void ResetSavePasswords(connection_t *);

extern const TCHAR *cfgProp;

/* These error codes are from openvpn service sources */
#define ERROR_OPENVPN_STARTUP 0x20000000
#define ERROR_STARTUP_DATA    0x20000001
#define ERROR_MESSAGE_DATA    0x20000002
#define ERROR_MESSAGE_TYPE    0x20000003

/* Write a line to status window and optionally to the log file */
void WriteStatusLog(connection_t *c, const WCHAR *prefix, const WCHAR *line, BOOL fileio);

#define FLAG_CR_TYPE_SCRV1  0x1   /* static challenege */
#define FLAG_CR_TYPE_CRV1   0x2   /* dynamic challenege */
#define FLAG_CR_ECHO        0x4   /* echo the response */
#define FLAG_CR_RESPONSE    0x8   /* response needed */
#define FLAG_PASS_TOKEN     0x10  /* PKCS11 token password needed */
#define FLAG_STRING_PKCS11  0x20  /* PKCS11 id needed */
#define FLAG_PASS_PKEY      0x40  /* Private key password needed */
#define FLAG_CR_TYPE_CRTEXT 0x80  /* crtext */
#define FLAG_CR_TYPE_CONCAT 0x100 /* concatenate otp with password */

typedef struct
{
    connection_t *c;
    unsigned int flags;
    char *str;
    char *id;
    char *user;
    char *cr_response;
} auth_param_t;

/*
 * Parse dynamic challenge string received from the server. Returns
 * true on success. The caller must free param->str and param->id
 * even on error.
 */
BOOL parse_dynamic_cr(const char *str, auth_param_t *param);

void free_auth_param(auth_param_t *param);

/*
 * Given an OpenVPN state as reported by the management interface
 * return the correspnding resource id for translation.
 */
int daemon_state_resid(const char *name);

#endif /* ifndef OPENVPN_H */
