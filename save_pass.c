/*
 *  This file is a part of OpenVPN-GUI -- A Windows GUI for OpenVPN.
 *
 *  Copyright (C) 2016 Selva Nair <selva.nair@gmail.com>
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
#include "config.h"
#endif

#include <windows.h>
#include <wincrypt.h>

#include "main.h"
#include "registry.h"
#include "save_pass.h"
#include "misc.h"

#define KEY_PASS_DATA  L"key-data"
#define AUTH_PASS_DATA L"auth-data"
#define ENTROPY_DATA   L"entropy"
#define AUTH_USER_DATA L"username"
#define ENTROPY_LEN    16

static DWORD
crypt_protect(BYTE *data, int szdata, char *entropy, BYTE **out)
{
    DATA_BLOB data_out;
    DATA_BLOB data_in;
    DATA_BLOB e;

    data_in.pbData = data;
    data_in.cbData = szdata;
    e.pbData = (BYTE *)entropy;
    e.cbData = entropy ? strlen(entropy) : 0;

    if (CryptProtectData(&data_in, NULL, &e, NULL, NULL, 0, &data_out))
    {
        *out = data_out.pbData;
        return data_out.cbData;
    }
    PrintDebug(L"CryptProtectData failed (error = %lu)", GetLastError());
    return 0;
}

static DWORD
crypt_unprotect(BYTE *data, int szdata, char *entropy, BYTE **out)
{
    DATA_BLOB data_in;
    DATA_BLOB data_out = { 0, 0 };
    DATA_BLOB e;

    data_in.pbData = data;
    data_in.cbData = szdata;
    e.pbData = (BYTE *)entropy;
    e.cbData = entropy ? strlen(entropy) : 0;

    if (CryptUnprotectData(&data_in, NULL, &e, NULL, NULL, 0, &data_out))
    {
        *out = data_out.pbData;
        return data_out.cbData;
    }
    else
    {
        PrintDebug(L"CryptUnprotectData: decryption failed");
        LocalFree(data_out.pbData);
        return 0;
    }
}

/*
 * If not found in registry and generate is true, a new nul terminated
 * random string is generated and saved in registry.
 * Else a zero-length string is returned and registry is not updated.
 */
static void
get_entropy(const WCHAR *config_name, char *e, int sz, BOOL generate)
{
    int len;

    len = GetConfigRegistryValue(config_name, ENTROPY_DATA, (BYTE *)e, sz);
    if (len > 0)
    {
        e[len - 1] = '\0';
        PrintDebug(L"Got entropy from registry: %hs (len = %d)", e, len);
        return;
    }
    else if (generate && GetRandomPassword(e, sz))
    {
        e[sz - 1] = '\0';
        PrintDebug(L"Created new entropy string : %hs", e);
        if (SetConfigRegistryValueBinary(config_name, ENTROPY_DATA, (BYTE *)e, sz))
        {
            return;
        }
    }
    if (generate)
    {
        PrintDebug(L"Failed to generate or save new entropy string -- using null string");
    }
    *e = '\0';
    return;
}
/*
 * Given a nul terminated string password, encrypt it and save in
 * a config specific registry key with specified name.
 * Returns 1 on success.
 */
static int
save_encrypted(const WCHAR *config_name, const WCHAR *password, const WCHAR *name)
{
    BYTE *out;
    DWORD len = (wcslen(password) + 1) * sizeof(WCHAR);
    char entropy[ENTROPY_LEN + 1];

    get_entropy(config_name, entropy, sizeof(entropy), true);
    len = crypt_protect((BYTE *)password, len, entropy, &out);
    if (len > 0)
    {
        SetConfigRegistryValueBinary(config_name, name, out, len);
        LocalFree(out);
        return 1;
    }
    else
    {
        return 0;
    }
}

/*
 * Encrypt the nul terminated string password and store it in the
 * registry with key name KEY_PASS_DATA. Returns 1 on success.
 */
int
SaveKeyPass(const WCHAR *config_name, const WCHAR *password)
{
    return save_encrypted(config_name, password, KEY_PASS_DATA);
}

/*
 * Encrypt the nul terminated string password and store it in the
 * registry with key name AUTH_PASS_DATA. Returns 1 on success.
 */
int
SaveAuthPass(const WCHAR *config_name, const WCHAR *password)
{
    return save_encrypted(config_name, password, AUTH_PASS_DATA);
}

/*
 * Returns 1 on success, 0 on failure. password should have space
 * for up to capacity wide chars incluing nul termination
 */
static int
recall_encrypted(const WCHAR *config_name, WCHAR *password, DWORD capacity, const WCHAR *name)
{
    BYTE in[2048];
    BYTE *out;
    DWORD len;
    int retval = 0;
    char entropy[ENTROPY_LEN + 1];

    get_entropy(config_name, entropy, sizeof(entropy), false);

    memset(password, 0, capacity);

    len = GetConfigRegistryValue(config_name, name, in, sizeof(in));
    if (len == 0)
    {
        return 0;
    }

    len = crypt_unprotect(in, len, entropy, &out);
    if (len == 0)
    {
        return 0;
    }

    if (len <= capacity * sizeof(*password))
    {
        memcpy(password, out, len);
        password[capacity - 1] = L'\0'; /* in case the data was corrupted */
        retval = 1;
    }
    else
    {
        PrintDebug(L"recall_encrypted: saved '%ls' too long (len = %d bytes)", name, len);
    }

    SecureZeroMemory(out, len);
    LocalFree(out);

    return retval;
}

/*
 * Reccall saved private key password. The buffer password should be
 * have space for up to KEY_PASS_LEN WCHARs including nul.
 * Returns 1 on success, 0 on failure.
 */
int
RecallKeyPass(const WCHAR *config_name, WCHAR *password)
{
    return recall_encrypted(config_name, password, KEY_PASS_LEN, KEY_PASS_DATA);
}

/*
 * Reccall saved auth password. The buffer password should be
 * have space for up to USER_PASS_LEN WCHARs including nul.
 * Returns 1 on success, 0 on failure.
 */
int
RecallAuthPass(const WCHAR *config_name, WCHAR *password)
{
    return recall_encrypted(config_name, password, USER_PASS_LEN, AUTH_PASS_DATA);
}

int
SaveUsername(const WCHAR *config_name, const WCHAR *username)
{
    DWORD len = (wcslen(username) + 1) * sizeof(*username);
    SetConfigRegistryValueBinary(config_name, AUTH_USER_DATA, (BYTE *)username, len);
    return 1;
}
/*
 * The buffer username should be have space for up to USER_PASS_LEN
 * WCHARs including nul.
 */
int
RecallUsername(const WCHAR *config_name, WCHAR *username)
{
    DWORD capacity = USER_PASS_LEN * sizeof(WCHAR);
    DWORD len;

    len = GetConfigRegistryValue(config_name, AUTH_USER_DATA, (BYTE *)username, capacity);
    if (len == 0)
    {
        return 0;
    }
    username[USER_PASS_LEN - 1] = L'\0';
    return 1;
}

void
DeleteSavedKeyPass(const WCHAR *config_name)
{
    DeleteConfigRegistryValue(config_name, KEY_PASS_DATA);
}

void
DeleteSavedAuthPass(const WCHAR *config_name)
{
    DeleteConfigRegistryValue(config_name, AUTH_PASS_DATA);
}

/* delete saved config-specific auth password and private key passphrase */
void
DeleteSavedPasswords(const WCHAR *config_name)
{
    DeleteConfigRegistryValue(config_name, KEY_PASS_DATA);
    DeleteConfigRegistryValue(config_name, AUTH_PASS_DATA);
    DeleteConfigRegistryValue(config_name, ENTROPY_DATA);
}

/* check if auth password is saved */
BOOL
IsAuthPassSaved(const WCHAR *config_name)
{
    DWORD len = 0;
    len = GetConfigRegistryValue(config_name, AUTH_PASS_DATA, NULL, 0);
    PrintDebug(L"checking auth-pass-data in registry returned len = %d", len);
    return (len > 0);
}

/* check if key password is saved */
BOOL
IsKeyPassSaved(const WCHAR *config_name)
{
    DWORD len = 0;
    len = GetConfigRegistryValue(config_name, KEY_PASS_DATA, NULL, 0);
    PrintDebug(L"checking key-pass-data in registry returned len = %d", len);
    return (len > 0);
}
