/*
 *  OpenVPN-GUI -- A Windows GUI for OpenVPN.
 *
 *  Copyright (C) 2004 Mathias Sundman <mathias@nilings.se>
 *                2016 Selva Nair <selva.nair@gmail.com>
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
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <windows.h>
#include <tchar.h>
#include <shlobj.h>

#include "main.h"
#include "options.h"
#include "openvpn-gui-res.h"
#include "registry.h"
#include "localization.h"

extern options_t o;

struct regkey_str
{
    const WCHAR *name;
    WCHAR *var;
    int len;
    const WCHAR *value;
} regkey_str[] = {
    { L"config_dir", o.config_dir, _countof(o.config_dir), L"%USERPROFILE%\\OpenVPN\\config" },
    { L"config_ext", o.ext_string, _countof(o.ext_string), L"ovpn" },
    { L"log_dir", o.log_dir, _countof(o.log_dir), L"%USERPROFILE%\\OpenVPN\\log" }
};

struct regkey_int
{
    const WCHAR *name;
    DWORD *var;
    DWORD value;
} regkey_int[] = { { L"log_append", &o.log_append, 0 },
                   { L"iservice_admin", &o.iservice_admin, 1 },
                   { L"show_balloon", &o.show_balloon, 1 },
                   { L"silent_connection", &o.silent_connection, 0 },
                   { L"preconnectscript_timeout", &o.preconnectscript_timeout, 10 },
                   { L"connectscript_timeout", &o.connectscript_timeout, 30 },
                   { L"disconnectscript_timeout", &o.disconnectscript_timeout, 10 },
                   { L"show_script_window", &o.show_script_window, 0 },
                   { L"config_menu_view", &o.config_menu_view, CONFIG_VIEW_AUTO },
                   { L"popup_mute_interval", &o.popup_mute_interval, 24 },
                   { L"disable_popup_messages", &o.disable_popup_messages, 0 },
                   { L"management_port_offset", &o.mgmt_port_offset, 25340 },
                   { L"enable_peristent_connections", &o.enable_persistent, 2 },
                   { L"enable_auto_restart", &o.enable_auto_restart, 1 },
                   { L"auth_pass_concat_otp", &o.auth_pass_concat_otp, 0 },
                   { L"ovpn_engine", &o.ovpn_engine, OPENVPN_ENGINE_OVPN2 } };

static int
RegValueExists(HKEY regkey, const WCHAR *name)
{
    return (RegQueryValueEx(regkey, name, NULL, NULL, NULL, NULL) == ERROR_SUCCESS);
}

static int
GetGlobalRegistryKeys()
{
    TCHAR windows_dir[MAX_PATH];
    HKEY regkey;

    if (!GetWindowsDirectory(windows_dir, _countof(windows_dir)))
    {
        /* can't get windows dir */
        ShowLocalizedMsg(IDS_ERR_GET_WINDOWS_DIR);
        /* Use a default value */
        _sntprintf_0(windows_dir, L"C:\\Windows");
    }

    /* set default editor and log_viewer as a fallback for opening config/log files */
    _sntprintf_0(o.editor, L"%ls\\%ls", windows_dir, L"System32\\notepad.exe");
    _sntprintf_0(o.log_viewer, L"%ls", o.editor);

    /* Get path to OpenVPN installation. */
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\OpenVPN"), 0, KEY_READ, &regkey)
        != ERROR_SUCCESS)
    {
        /* registry key not found */
        regkey = NULL;
        ShowLocalizedMsg(IDS_ERR_OPEN_REGISTRY);
    }
    if (!regkey || !GetRegistryValue(regkey, _T(""), o.install_path, _countof(o.install_path))
        || _tcslen(o.install_path) == 0)
    {
        /* error reading registry value */
        if (regkey)
        {
            ShowLocalizedMsg(IDS_ERR_READING_REGISTRY);
        }
        /* Use a sane default value */
        _sntprintf_0(o.install_path, _T("%ls"), _T("C:\\Program Files\\OpenVPN\\"));
    }
    if (o.install_path[_tcslen(o.install_path) - 1] != _T('\\'))
    {
        _tcscat(o.install_path, _T("\\"));
    }

    /* an admin-defined global config dir defined in HKLM\OpenVPN\config_dir */
    if (!regkey
        || !GetRegistryValue(
            regkey, _T("config_dir"), o.global_config_dir, _countof(o.global_config_dir)))
    {
        /* use default = openvpnpath\config */
        _sntprintf_0(o.global_config_dir, _T("%lsconfig"), o.install_path);
    }

    if (!regkey
        || !GetRegistryValue(
            regkey, _T("autostart_config_dir"), o.config_auto_dir, _countof(o.config_auto_dir)))
    {
        /* use default = openvpnpath\config-auto */
        _sntprintf_0(o.config_auto_dir, L"%lsconfig-auto", o.install_path);
    }

    if (!regkey
        || !GetRegistryValue(regkey, _T("log_dir"), o.global_log_dir, _countof(o.global_log_dir)))
    {
        /* use default = openvpnpath\log */
        _sntprintf_0(o.global_log_dir, L"%lslog", o.install_path);
    }

    if (!regkey
        || !GetRegistryValue(
            regkey, _T("ovpn_admin_group"), o.ovpn_admin_group, _countof(o.ovpn_admin_group)))
    {
        _tcsncpy(o.ovpn_admin_group, OVPN_ADMIN_GROUP, _countof(o.ovpn_admin_group) - 1);
    }

    if (!regkey || !GetRegistryValue(regkey, _T("exe_path"), o.exe_path, _countof(o.exe_path)))
    {
        _sntprintf_0(o.exe_path, _T("%lsbin\\openvpn.exe"), o.install_path);
    }

    if (!regkey
        || !GetRegistryValue(
            regkey, _T("priority"), o.priority_string, _countof(o.priority_string)))
    {
        _tcsncpy(o.priority_string, _T("NORMAL_PRIORITY_CLASS"), _countof(o.priority_string) - 1);
    }
    if (!regkey
        || !GetRegistryValueNumeric(
            regkey, _T("disable_save_passwords"), &o.disable_save_passwords))
    {
        o.disable_save_passwords = 0;
    }
    if (regkey)
    {
        RegCloseKey(regkey);
    }
    return true;
}

int
GetRegistryKeys()
{
    HKEY regkey;
    DWORD status;
    int i;

    if (!GetGlobalRegistryKeys())
    {
        return false;
    }

    status = RegOpenKeyEx(HKEY_CURRENT_USER, GUI_REGKEY_HKCU, 0, KEY_READ, &regkey);

    for (i = 0; i < (int)_countof(regkey_str); ++i)
    {
        if (status != ERROR_SUCCESS
            || !GetRegistryValue(regkey, regkey_str[i].name, regkey_str[i].var, regkey_str[i].len))
        {
            /* no value found in registry, use the default */
            wcsncpy(regkey_str[i].var, regkey_str[i].value, regkey_str[i].len);
            regkey_str[i].var[regkey_str[i].len - 1] = L'\0';
            PrintDebug(L"default: %ls = %ls", regkey_str[i].name, regkey_str[i].var);
        }
        else
        {
            PrintDebug(L"from registry: %ls = %ls", regkey_str[i].name, regkey_str[i].var);
        }
    }

    for (i = 0; i < (int)_countof(regkey_int); ++i)
    {
        if (status != ERROR_SUCCESS
            || !GetRegistryValueNumeric(regkey, regkey_int[i].name, regkey_int[i].var))
        {
            /* no value found in registry, use the default */
            *regkey_int[i].var = regkey_int[i].value;
            PrintDebug(L"default: %ls = %lu", regkey_int[i].name, *regkey_int[i].var);
        }
        else
        {
            PrintDebug(L"from registry: %ls = %lu", regkey_int[i].name, *regkey_int[i].var);
        }
    }

    if (status == ERROR_SUCCESS)
    {
        RegCloseKey(regkey);
    }

    if ((o.disconnectscript_timeout == 0))
    {
        ShowLocalizedMsg(IDS_ERR_DISCONN_SCRIPT_TIMEOUT);
        o.disconnectscript_timeout = 10;
    }
    if ((o.preconnectscript_timeout == 0))
    {
        ShowLocalizedMsg(IDS_ERR_PRECONN_SCRIPT_TIMEOUT);
        o.preconnectscript_timeout = 10;
    }
    if (o.mgmt_port_offset < 1 || o.mgmt_port_offset > 61000)
    {
        o.mgmt_port_offset = 25340;
    }

    /* Read group policy setting for password reveal */
    status = RegOpenKeyExW(
        HKEY_CURRENT_USER, L"Software\\Policies\\Microsoft\\Windows\\CredUI", 0, KEY_READ, &regkey);
    if (status != ERROR_SUCCESS
        || !GetRegistryValueNumeric(regkey, L"DisablePasswordReveal", &o.disable_password_reveal))
    {
        o.disable_password_reveal = 0;
        PrintDebug(L"default: %ls = %lu", L"DisablePasswordReveal", o.disable_password_reveal);
    }
    else
    {
        PrintDebug(L"from policy: %ls = %lu", L"DisablePasswordReveal", o.disable_password_reveal);
    }
    if (status == ERROR_SUCCESS)
    {
        RegCloseKey(regkey);
    }

    ExpandOptions();
    return true;
}

int
SaveRegistryKeys()
{
    HKEY regkey;
    DWORD status;
    int i;
    int ret = false;

    status = RegCreateKeyEx(HKEY_CURRENT_USER,
                            GUI_REGKEY_HKCU,
                            0,
                            NULL,
                            REG_OPTION_NON_VOLATILE,
                            KEY_WRITE | KEY_READ,
                            NULL,
                            &regkey,
                            NULL);
    if (status != ERROR_SUCCESS)
    {
        ShowLocalizedMsg(IDS_ERR_CREATE_REG_HKCU_KEY, GUI_REGKEY_HKCU);
        goto out;
    }
    for (i = 0; i < (int)_countof(regkey_str); ++i)
    {
        /* save only if the value differs from default or already present in registry */
        if (CompareStringExpanded(regkey_str[i].var, regkey_str[i].value) != 0
            || RegValueExists(regkey, regkey_str[i].name))
        {
            if (!SetRegistryValue(regkey, regkey_str[i].name, regkey_str[i].var))
            {
                goto out;
            }
        }
    }

    for (i = 0; i < (int)_countof(regkey_int); ++i)
    {
        if (*regkey_int[i].var != regkey_int[i].value || RegValueExists(regkey, regkey_int[i].name))
        {
            if (!SetRegistryValueNumeric(regkey, regkey_int[i].name, *regkey_int[i].var))
            {
                goto out;
            }
        }
    }
    ret = true;

out:

    if (status == ERROR_SUCCESS)
    {
        RegCloseKey(regkey);
    }
    return ret;
}

static BOOL
GetRegistryVersion(version_t *v)
{
    HKEY regkey;
    CLEAR(*v);
    DWORD len = sizeof(*v);

    if (RegOpenKeyEx(HKEY_CURRENT_USER, GUI_REGKEY_HKCU, 0, KEY_READ, &regkey) == ERROR_SUCCESS)
    {
        if (RegGetValueW(regkey, NULL, L"version", RRF_RT_REG_BINARY, NULL, v, &len)
            != ERROR_SUCCESS)
        {
            CLEAR(*v);
        }
        RegCloseKey(regkey);
    }
    return true;
}

static BOOL
SetRegistryVersion(const version_t *v)
{
    HKEY regkey;
    DWORD status;
    BOOL ret = false;

    status = RegCreateKeyEx(HKEY_CURRENT_USER,
                            GUI_REGKEY_HKCU,
                            0,
                            NULL,
                            REG_OPTION_NON_VOLATILE,
                            KEY_WRITE,
                            NULL,
                            &regkey,
                            NULL);
    if (status == ERROR_SUCCESS)
    {
        if (RegSetValueEx(regkey, L"version", 0, REG_BINARY, (const BYTE *)v, sizeof(*v))
            == ERROR_SUCCESS)
        {
            ret = true;
        }
        RegCloseKey(regkey);
    }
    else
    {
        PrintDebug(L"Eror opening/creating 'HKCU\\%ls' registry key", GUI_REGKEY_HKCU);
    }
    return ret;
}

static int
MigrateNilingsKeys()
{
    DWORD status;
    BOOL ret = false;
    HKEY regkey, regkey_proxy, regkey_nilings;

    status = RegOpenKeyEx(
        HKEY_CURRENT_USER, L"Software\\Nilings\\OpenVPN-GUI", 0, KEY_READ, &regkey_nilings);
    if (status != ERROR_SUCCESS)
    {
        return true; /* No old keys to migrate */
    }
    status = RegCreateKeyEx(HKEY_CURRENT_USER,
                            GUI_REGKEY_HKCU,
                            0,
                            NULL,
                            REG_OPTION_NON_VOLATILE,
                            KEY_ALL_ACCESS,
                            NULL,
                            &regkey,
                            NULL);
    if (status != ERROR_SUCCESS)
    {
        ShowLocalizedMsg(IDS_ERR_CREATE_REG_HKCU_KEY, GUI_REGKEY_HKCU);
        RegCloseKey(regkey_nilings);
        return false;
    }

    /* For some reason this needs ALL_ACCESS for the CopyTree below to work */
    status = RegCreateKeyEx(regkey,
                            L"proxy",
                            0,
                            NULL,
                            REG_OPTION_NON_VOLATILE,
                            KEY_ALL_ACCESS,
                            NULL,
                            &regkey_proxy,
                            NULL);
    if (status == ERROR_SUCCESS)
    {
        DWORD ui_lang;
        /* Move language setting from Nilings to GUI_REGKEY_HKCU */
        if (GetRegistryValueNumeric(regkey_nilings, L"ui_language", &ui_lang))
        {
            SetRegistryValueNumeric(regkey, L"ui_language", ui_lang);
        }

        status = RegCopyTree(regkey_nilings, NULL, regkey_proxy);
        if (status == ERROR_SUCCESS)
        {
            RegDeleteValue(regkey_proxy, L"ui_language"); /* in case copied here */
            ret = true;
        }
        RegCloseKey(regkey_proxy);
    }
    else
    {
        PrintDebug(L"Error creating key 'proxy' in HKCU\\%ls", GUI_REGKEY_HKCU);
    }

    RegCloseKey(regkey);
    RegCloseKey(regkey_nilings);

    return ret;
}

int
UpdateRegistry(void)
{
    version_t v;

    GetRegistryVersion(&v);

    if (memcmp(&v, &o.version, sizeof(v)) == 0)
    {
        return true;
    }

    switch (v.major)
    {
        case 0: /* Cleanup GUI_REGKEY_HKCU and migrate any values under Nilings */

            RegDeleteTree(HKEY_CURRENT_USER, GUI_REGKEY_HKCU); /* delete all values and subkeys */

            if (!MigrateNilingsKeys())
            {
                return false;
            }

        /* fall through to handle further updates */
        case 11:
            /* current version -- nothing to do */
            break;

        default:
            break;
    }

    SetRegistryVersion(&o.version);
    PrintDebug(L"Registry updated to version %hu.%hu", o.version.major, o.version.minor);

    return true;
}

LONG
GetRegistryValue(HKEY regkey, const TCHAR *name, TCHAR *data, DWORD len)
{
    LONG status;
    DWORD type;
    DWORD data_len;

    data_len = len * sizeof(*data);

    /* get a registry string */
    status = RegQueryValueEx(regkey, name, NULL, &type, (byte *)data, &data_len);
    if (status != ERROR_SUCCESS || type != REG_SZ)
    {
        return (0);
    }

    data_len /= sizeof(*data);
    if (data_len > 0)
    {
        data[data_len - 1] = L'\0'; /* REG_SZ strings are not guaranteed to be null-terminated */
    }
    else
    {
        data[0] = L'\0';
    }

    return (data_len);
}

LONG
GetRegistryValueNumeric(HKEY regkey, const TCHAR *name, DWORD *data)
{
    DWORD type;
    DWORD size = sizeof(*data);
    LONG status = RegQueryValueEx(regkey, name, NULL, &type, (PBYTE)data, &size);
    return (type == REG_DWORD && status == ERROR_SUCCESS) ? 1 : 0;
}

int
SetRegistryValue(HKEY regkey, const TCHAR *name, const TCHAR *data)
{
    /* set a registry string */
    DWORD size = (_tcslen(data) + 1) * sizeof(*data);
    if (RegSetValueEx(regkey, name, 0, REG_SZ, (PBYTE)data, size) != ERROR_SUCCESS)
    {
        /* Error writing registry value */
        ShowLocalizedMsg(IDS_ERR_WRITE_REGVALUE, GUI_REGKEY_HKCU, name);
        return (0);
    }

    return (1);
}

int
SetRegistryValueNumeric(HKEY regkey, const TCHAR *name, DWORD data)
{
    LONG status = RegSetValueEx(regkey, name, 0, REG_DWORD, (PBYTE)&data, sizeof(data));
    if (status == ERROR_SUCCESS)
    {
        return 1;
    }

    ShowLocalizedMsg(IDS_ERR_WRITE_REGVALUE, GUI_REGKEY_HKCU, name);
    return 0;
}

/*
 * Open HKCU\Software\OpenVPN-GUI\configs\config-name.
 * The caller must close the key. Returns 1 on success.
 */
static int
OpenConfigRegistryKey(const WCHAR *config_name, HKEY *regkey, BOOL create)
{
    DWORD status;
    const WCHAR fmt[] = L"SOFTWARE\\OpenVPN-GUI\\configs\\%ls";
    int count = (wcslen(config_name) + wcslen(fmt) + 1);
    WCHAR *name = malloc(count * sizeof(WCHAR));

    if (!name)
    {
        return 0;
    }

    _snwprintf_s(name, count, _TRUNCATE, fmt, config_name);

    if (!create)
    {
        status = RegOpenKeyEx(HKEY_CURRENT_USER, name, 0, KEY_READ | KEY_WRITE, regkey);
    }
    else
    {
        /* create if key doesn't exist */
        status = RegCreateKeyEx(HKEY_CURRENT_USER,
                                name,
                                0,
                                NULL,
                                REG_OPTION_NON_VOLATILE,
                                KEY_READ | KEY_WRITE,
                                NULL,
                                regkey,
                                NULL);
    }
    free(name);

    return (status == ERROR_SUCCESS);
}

int
SetConfigRegistryValueBinary(const WCHAR *config_name,
                             const WCHAR *name,
                             const BYTE *data,
                             DWORD len)
{
    HKEY regkey;
    DWORD status;

    if (!OpenConfigRegistryKey(config_name, &regkey, TRUE))
    {
        return 0;
    }
    status = RegSetValueEx(regkey, name, 0, REG_BINARY, data, len);
    RegCloseKey(regkey);

    return (status == ERROR_SUCCESS);
}

/*
 * Read registry value into the user supplied buffer data that can hold
 * up to len bytes. Returns the actual number of bytes read or zero on error.
 * If data is NULL returns the required buffer size, and no data is read.
 */
DWORD
GetConfigRegistryValue(const WCHAR *config_name, const WCHAR *name, BYTE *data, DWORD len)
{
    DWORD status;
    DWORD type;
    HKEY regkey;

    if (!OpenConfigRegistryKey(config_name, &regkey, FALSE))
    {
        return 0;
    }
    status = RegQueryValueEx(regkey, name, NULL, &type, data, &len);
    RegCloseKey(regkey);
    if (status == ERROR_SUCCESS)
    {
        return len;
    }
    else
    {
        return 0;
    }
}

int
DeleteConfigRegistryValue(const WCHAR *config_name, const WCHAR *name)
{
    DWORD status;
    HKEY regkey;

    if (!OpenConfigRegistryKey(config_name, &regkey, FALSE))
    {
        return 0;
    }
    status = RegDeleteValue(regkey, name);
    RegCloseKey(regkey);

    return (status == ERROR_SUCCESS);
}
