/*
 *  This file is a part of OpenVPN-GUI -- A Windows GUI for OpenVPN.
 *
 *  Copyright (C) 2018 Selva Nair <selva.nair@gmail.com>
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
#include <wincrypt.h>
#include "script_security.h"
#include "registry.h"
#include "misc.h"
#include "main.h"

#define MAX_HASHLEN 64
#define HASHALG CALG_SHA1

/* Return the script_security setting that should override the
 * value in the config file.
 * If the config file has been modified since last time the
 * setting was confirmed by the user, its reset to the default.
 */
int
get_script_security(const connection_t *c)
{
    DWORD err;
    wchar_t filename[MAX_PATH];
    BYTE saved_digest[MAX_HASHLEN], current_digest[MAX_HASHLEN];
    DWORD digest_len = _countof(saved_digest);

    int ssec_flag = ssec_default();

    wcs_concat2(filename, _countof(filename), c->config_dir, c->config_file, L"\\");

    err = md_file(HASHALG, filename, current_digest, &digest_len);

    if (err)
    {
        MsgToEventLog(EVENTLOG_ERROR_TYPE, L"Failed to compute file digest of <%s>: error = %lu",
                      filename, err);
        return ssec_flag;
    }

    /* If there is a digest is in registry and matches that of the file,
     * read and use the script security setting in the registry.
     */
    digest_len = GetConfigRegistryValue(c->config_name, L"config_digest", saved_digest,
                                        _countof(saved_digest));
    if (digest_len)
    {
        DWORD flag;
        if (memcmp(current_digest, saved_digest, digest_len))
        {
            /* if digest does not match clear script-security flag from registry
             * to force the default. Its ok to call delete even if the key
             * doesn't exist. */
            DeleteConfigRegistryValue(c->config_name, L"script_security");
        }
        else if (GetConfigRegistryValueNumeric(c->config_name, L"script_security", &flag))
        {
            /* ignore any out of range value in registry */
            if (flag <= SSEC_PW_ENV || (int) flag == -1)
                ssec_flag = (int) flag;
        }
    }

    return ssec_flag;
}

/* Update the script security flag for a connection profile and save
 * it in registry
 */
void
set_script_security(connection_t *c, int ssec_flag)
{
    DWORD err;
    wchar_t filename[MAX_PATH];
    BYTE digest[MAX_HASHLEN];
    DWORD digest_len = _countof(digest);

    wcs_concat2(filename, _countof(filename), c->config_dir, c->config_file, L"\\");

    err = md_file(HASHALG, filename, digest, &digest_len);
    if (err)
    {
        MsgToEventLog(EVENTLOG_ERROR_TYPE, L"Failed to compute file digest of <%s>: error = %lu",
                      filename, err);
        return;
    }
    SetConfigRegistryValueBinary(c->config_name, L"config_digest", digest, digest_len);
    SetConfigRegistryValueNumeric(c->config_name, L"script_security", (DWORD) ssec_flag);
}

/* Lock/unlock the config file to protect against changes when a connection
 * is active. Call with |lock| == true to lock, as false to unlock.
 * The lock will still permit shared read required for OpenVPN to parse
 * the config file.
 */
void
lock_config_file(connection_t *c, BOOL lock)
{
    wchar_t fname[MAX_PATH];
    OVERLAPPED o = {.Offset = 0, .hEvent = NULL};
    DWORD bytes_low = (DWORD) -1;
    DWORD bytes_hi = 0;

    wcs_concat2(fname, _countof(fname), c->config_dir, c->config_file, L"\\");

    if (lock)
    {
        c->hfile = CreateFile(fname, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL, NULL);

        if (c->hfile == NULL || c->hfile == INVALID_HANDLE_VALUE)
        {
            MsgToEventLog(EVENTLOG_ERROR_TYPE, L"Failed to open the config file <%s> for reading",
                          fname);
            return;
        }

        if (!LockFileEx(c->hfile, LOCKFILE_FAIL_IMMEDIATELY, 0, bytes_low, bytes_hi, &o))
            MsgToEventLog(EVENTLOG_ERROR_TYPE, L"Failed to write-lock the config file <%s>: err = %lu",
                          fname, GetLastError());
    }
    else
    {
        if (c->hfile == NULL || c->hfile == INVALID_HANDLE_VALUE) return;

        UnlockFileEx(c->hfile, 0, bytes_low, bytes_hi, &o);
        CloseHandle(c->hfile);
        c->hfile = NULL;
    }
}
