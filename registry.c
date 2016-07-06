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

static void
ExpandString (WCHAR *str, int max_len)
{
  WCHAR expanded_string[MAX_PATH];
  int len = ExpandEnvironmentStringsW(str, expanded_string, _countof(expanded_string));

  if (len > max_len || len > (int) _countof(expanded_string))
  {
      PrintDebug (L"Failed to expanded env vars in '%s'. String too long", str);
      return;
  }
  wcsncpy(str, expanded_string, max_len);
}

int
GetRegistryKeys()
{
  TCHAR windows_dir[MAX_PATH];
  TCHAR temp_path[MAX_PATH];
  TCHAR openvpn_path[MAX_PATH];
  TCHAR profile_dir[MAX_PATH];
  HKEY regkey;

  if (!GetWindowsDirectory(windows_dir, _countof(windows_dir))) {
    /* can't get windows dir */
    ShowLocalizedMsg(IDS_ERR_GET_WINDOWS_DIR);
    return(false);
  }

  if (SHGetFolderPath(NULL, CSIDL_PROFILE, NULL, SHGFP_TYPE_CURRENT, profile_dir) != S_OK) {
    ShowLocalizedMsg(IDS_ERR_GET_PROFILE_DIR);
    return(false);
  }

  /* Get path to OpenVPN installation. */
  if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\OpenVPN"), 0, KEY_READ, &regkey)
      != ERROR_SUCCESS) 
    {
      /* registry key not found */
      ShowLocalizedMsg(IDS_ERR_OPEN_REGISTRY);
      return(false);
    }
  if (!GetRegistryValue(regkey, _T(""), openvpn_path, _countof(openvpn_path)))
    {
      /* error reading registry value */
      ShowLocalizedMsg(IDS_ERR_READING_REGISTRY);
      RegCloseKey(regkey);
      return(false);
    }
  if (openvpn_path[_tcslen(openvpn_path) - 1] != _T('\\'))
    _tcscat(openvpn_path, _T("\\"));

  /* an admin-defined global config dir defined in HKLM\OpenVPN\config_dir */
  if (!GetRegistryValue(regkey, _T("config_dir"), o.global_config_dir, _countof(o.global_config_dir)))
    {
      /* use default = openvpnpath\config */
      _sntprintf_0(o.global_config_dir, _T("%sconfig"), openvpn_path);
    }

  if (!GetRegistryValue(regkey, _T("ovpn_admin_group"), o.ovpn_admin_group, _countof(o.ovpn_admin_group)))
    {
      _tcsncpy(o.ovpn_admin_group, OVPN_ADMIN_GROUP, _countof(o.ovpn_admin_group)-1);
    }

  if (o.exe_path[0] != L'\0') /* set by cmd-line */
      ExpandString (o.exe_path, _countof(o.exe_path));
  else if (!GetRegistryValue(regkey, _T("exe_path"), o.exe_path, _countof(o.exe_path)))
    {
      _sntprintf_0(o.exe_path, _T("%sbin\\openvpn.exe"), openvpn_path);
    }

  if (o.priority_string[0] != L'\0') /* set by cmd-line */
    ExpandString (o.priority_string, _countof(o.priority_string));
  if (!GetRegistryValue(regkey, _T("priority"), o.priority_string, _countof(o.priority_string)))
    {
      _tcsncpy(o.priority_string, _T("NORMAL_PRIORITY_CLASS"), _countof(o.priority_string)-1);
    }
  RegCloseKey(regkey);

  /* user-sepcific config_dir in user's profile by default */
  _sntprintf_0(temp_path, _T("%s\\OpenVPN\\config"), profile_dir);
  if (!GetRegKey(_T("config_dir"), o.config_dir, 
      temp_path, _countof(o.config_dir))) return(false);

  if (!GetRegKey(_T("config_ext"), o.ext_string, _T("ovpn"), _countof(o.ext_string))) return(false);

  _sntprintf_0(temp_path, _T("%s\\OpenVPN\\log"), profile_dir);
  if (!GetRegKey(_T("log_dir"), o.log_dir, 
      temp_path, _countof(o.log_dir))) return(false);

  if (!GetRegKey(_T("log_append"), o.append_string, _T("0"), _countof(o.append_string))) return(false);

  if (o.editor[0] != L'\0') /* set by cmd-line */
      ExpandString (o.editor, _countof(o.editor));
  else
      _sntprintf_0(o.editor, _T("%s\\system32\\notepad.exe"), windows_dir);

  if (o.log_viewer[0] != L'\0') /* set by cmd-line */
      ExpandString (o.log_viewer, _countof(o.log_viewer));
  else
      _sntprintf_0(o.log_viewer, _T("%s\\system32\\notepad.exe"), windows_dir);

  if (!GetRegKey(_T("service_only"), o.service_only, _T("0"), _countof(o.service_only))) return(false);

  if (!GetRegKey(_T("show_balloon"), o.show_balloon, _T("1"), _countof(o.show_balloon))) return(false);

  if (!GetRegKey(_T("silent_connection"), o.silent_connection, _T("0"), _countof(o.silent_connection))) return(false);

  if (!GetRegKey(_T("show_script_window"), o.show_script_window, _T("1"), _countof(o.show_script_window))) return(false);

  if (!GetRegKey(_T("connectscript_timeout"), o.connectscript_timeout_string, _T("15"), 
      _countof(o.connectscript_timeout_string))) return(false);
  o.connectscript_timeout = _ttoi(o.connectscript_timeout_string);
  if ((o.connectscript_timeout < 0) || (o.connectscript_timeout > 99))
    {
      /* 0 <= connectscript_timeout <= 99 */
      ShowLocalizedMsg(IDS_ERR_CONN_SCRIPT_TIMEOUT);
      return(false);
    }

  if (!GetRegKey(_T("disconnectscript_timeout"), o.disconnectscript_timeout_string, _T("10"), 
      _countof(o.disconnectscript_timeout_string))) return(false);
  o.disconnectscript_timeout = _ttoi(o.disconnectscript_timeout_string);
  if ((o.disconnectscript_timeout <= 0) || (o.disconnectscript_timeout > 99))
    {
      /* 0 < disconnectscript_timeout <= 99 */
      ShowLocalizedMsg(IDS_ERR_DISCONN_SCRIPT_TIMEOUT);
      return(false);
    }

  if (!GetRegKey(_T("preconnectscript_timeout"), o.preconnectscript_timeout_string, _T("10"), 
      _countof(o.preconnectscript_timeout_string))) return(false);
  o.preconnectscript_timeout = _ttoi(o.preconnectscript_timeout_string);
  if ((o.preconnectscript_timeout <= 0) || (o.preconnectscript_timeout > 99))
    {
      /* 0 < disconnectscript_timeout <= 99 */
      ShowLocalizedMsg(IDS_ERR_PRECONN_SCRIPT_TIMEOUT);
      return(false);
    }

  return(true);
}


int GetRegKey(const TCHAR name[], TCHAR *data, const TCHAR default_data[], DWORD len)
{
  LONG status;
  DWORD type;
  HKEY openvpn_key;
  HKEY openvpn_key_write;
  DWORD dwDispos;
  DWORD size = len * sizeof(*data);
  DWORD max_len = len - 1;

  /* If option is already set via cmd-line, return */
  if (data[0] != 0) 
    {
      ExpandString (data, len);
      return(true);
    }

  status = RegOpenKeyEx(HKEY_CURRENT_USER,
                       _T("SOFTWARE\\OpenVPN-GUI"),
                       0,
                       KEY_READ,
                       &openvpn_key);

  if (status != ERROR_SUCCESS)
    {
      if (RegCreateKeyEx(HKEY_CURRENT_USER,
                        _T("Software\\OpenVPN-GUI"),
                        0,
                        _T(""),
                        REG_OPTION_NON_VOLATILE,
                        KEY_READ | KEY_WRITE,
                        NULL,
                        &openvpn_key,
                        &dwDispos) != ERROR_SUCCESS)
        {
          /* error creating registry key */
          ShowLocalizedMsg(IDS_ERR_CREATE_REG_HKCU_KEY, _T("OpenVPN-GUI"));
          return(false);
        }  
    }


  /* get a registry string */
  status = RegQueryValueEx(openvpn_key, name, NULL, &type, (byte *) data, &size);
  if (status != ERROR_SUCCESS || type != REG_SZ)
    {
      /* key did not exist - set default value */
      status = RegOpenKeyEx(HKEY_CURRENT_USER,
			  _T("SOFTWARE\\OpenVPN-GUI"),
			  0,
			  KEY_READ | KEY_WRITE,
			  &openvpn_key_write);

      if (status != ERROR_SUCCESS) {
         /* can't open registry for writing */
         ShowLocalizedMsg(IDS_ERR_WRITE_REGVALUE, _T("OpenVPN-GUI"), name);
         return(false);
      }    
      if(!SetRegistryValue(openvpn_key_write, name, default_data))
        {
          /* cant read / set reg-key */ 
          return(false);
        }
      _tcsncpy(data, default_data, max_len);
      RegCloseKey(openvpn_key_write);

    }
  else
    {
      size /= sizeof(*data);
      data[size - 1] = L'\0'; /* REG_SZ strings are not guaranteed to be null-terminated */
    }

  RegCloseKey(openvpn_key);

  // Expand environment variables inside the string.
  ExpandString (data, len);

  return(true);
}

LONG GetRegistryValue(HKEY regkey, const TCHAR *name, TCHAR *data, DWORD len)
{
  LONG status;
  DWORD type;
  DWORD data_len;

  data_len = len * sizeof(*data);

  /* get a registry string */
  status = RegQueryValueEx(regkey, name, NULL, &type, (byte *) data, &data_len);
  if (status != ERROR_SUCCESS || type != REG_SZ)
    return(0);

  data_len /= sizeof(*data);
  data[data_len - 1] = L'\0'; /* REG_SZ strings are not guaranteed to be null-terminated */

  return(data_len);

}

LONG
GetRegistryValueNumeric(HKEY regkey, const TCHAR *name, DWORD *data)
{
  DWORD type;
  DWORD size = sizeof(*data);
  LONG status = RegQueryValueEx(regkey, name, NULL, &type, (PBYTE) data, &size);
  return (type == REG_DWORD ? status : ERROR_FILE_NOT_FOUND);
}

int SetRegistryValue(HKEY regkey, const TCHAR *name, const TCHAR *data)
{
  /* set a registry string */
  DWORD size = (_tcslen(data) + 1) * sizeof(*data);
  if(RegSetValueEx(regkey, name, 0, REG_SZ, (PBYTE) data, size) != ERROR_SUCCESS)
    {
      /* Error writing registry value */
      ShowLocalizedMsg(IDS_ERR_WRITE_REGVALUE, GUI_REGKEY_HKCU, name);
      return(0);
    }

  return(1);
}

int
SetRegistryValueNumeric(HKEY regkey, const TCHAR *name, DWORD data)
{
  LONG status = RegSetValueEx(regkey, name, 0, REG_DWORD, (PBYTE) &data, sizeof(data));
  if (status == ERROR_SUCCESS)
    return 1;

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
    const WCHAR fmt[] = L"SOFTWARE\\OpenVPN-GUI\\configs\\%s";
    int count = (wcslen(config_name) + wcslen(fmt) + 1);
    WCHAR *name = malloc(count * sizeof(WCHAR));

    if (!name)
        return 0;

    _snwprintf(name, count, fmt, config_name);
    name[count-1] = L'\0';

    if (!create)
       status = RegOpenKeyEx (HKEY_CURRENT_USER, name, 0, KEY_READ | KEY_WRITE, regkey);
    else
    /* create if key doesn't exist */
       status = RegCreateKeyEx(HKEY_CURRENT_USER, name, 0, NULL,
               REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, NULL, regkey, NULL);
    free (name);

    return (status == ERROR_SUCCESS);
}

int
SetConfigRegistryValueBinary(const WCHAR *config_name, const WCHAR *name, const BYTE *data, DWORD len)
{
    HKEY regkey;
    DWORD status;

    if (!OpenConfigRegistryKey(config_name, &regkey, TRUE))
        return 0;
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
        return 0;
    status = RegQueryValueEx(regkey, name, NULL, &type, data, &len);
    RegCloseKey(regkey);
    if (status == ERROR_SUCCESS)
        return len;
    else
        return 0;
}

int
DeleteConfigRegistryValue(const WCHAR *config_name, const WCHAR *name)
{
    DWORD status;
    HKEY regkey;

    if (!OpenConfigRegistryKey(config_name, &regkey, FALSE))
        return 0;
    status = RegDeleteValue(regkey, name);
    RegCloseKey(regkey);

    return (status == ERROR_SUCCESS);
}
