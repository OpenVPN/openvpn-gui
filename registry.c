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


#include <windows.h>
#include <shlobj.h>
#include "main.h"
#include "options.h"
#include "openvpn-gui-res.h"
#include "registry.h"

extern struct options o;

int
GetRegistryKeys()
{
  char windows_dir[MAX_PATH];
  char temp_path[MAX_PATH];
  char openvpn_path[MAX_PATH];
  HKEY regkey;

  if (!GetWindowsDirectory(windows_dir, sizeof(windows_dir))) {
    /* can't get windows dir */
    ShowLocalizedMsg(GUI_NAME, ERR_GET_WINDOWS_DIR, "");
    return(false);
  }

  /* Get path to OpenVPN installation. */
  if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\OpenVPN", 0, KEY_READ, &regkey)
      != ERROR_SUCCESS) 
    {
      /* registry key not found */
      ShowLocalizedMsg(GUI_NAME, ERR_OPEN_REGISTRY, "");
      return(false);
    }
  if (!GetRegistryValue(regkey, "", openvpn_path, sizeof(openvpn_path)))
    {
      /* error reading registry value */
      ShowLocalizedMsg(GUI_NAME, ERR_READING_REGISTRY, "");
      return(false);
    }
  if(openvpn_path[strlen(openvpn_path) - 1] != '\\')
    strcat(openvpn_path, "\\");


  mysnprintf(temp_path, "%sconfig", openvpn_path);
  if (!GetRegKey("config_dir", o.config_dir, 
      temp_path, sizeof(o.config_dir))) return(false);

  if (!GetRegKey("config_ext", o.ext_string, "ovpn", sizeof(o.ext_string))) return(false);

  mysnprintf(temp_path, "%sbin\\openvpn.exe", openvpn_path);
  if (!GetRegKey("exe_path", o.exe_path, 
      temp_path, sizeof(o.exe_path))) return(false);

  mysnprintf(temp_path, "%slog", openvpn_path);
  if (!GetRegKey("log_dir", o.log_dir, 
      temp_path, sizeof(o.log_dir))) return(false);

  if (!GetRegKey("log_append", o.append_string, "0", sizeof(o.append_string))) return(false);

  if (!GetRegKey("priority", o.priority_string, 
      "NORMAL_PRIORITY_CLASS", sizeof(o.priority_string))) return(false);

  mysnprintf(temp_path, "%s\\notepad.exe", windows_dir);
  if (!GetRegKey("log_viewer", o.log_viewer, 
      temp_path, sizeof(o.log_viewer))) return(false);

  mysnprintf(temp_path, "%s\\notepad.exe", windows_dir);
  if (!GetRegKey("editor", o.editor, 
      temp_path, sizeof(o.editor))) return(false);

  if (!GetRegKey("allow_edit", o.allow_edit, "1", sizeof(o.allow_edit))) return(false);
  
  if (!GetRegKey("allow_service", o.allow_service, "0", sizeof(o.allow_service))) return(false);

  if (!GetRegKey("allow_password", o.allow_password, "1", sizeof(o.allow_password))) return(false);

  if (!GetRegKey("allow_proxy", o.allow_proxy, "1", sizeof(o.allow_proxy))) return(false);

  if (!GetRegKey("service_only", o.service_only, "0", sizeof(o.service_only))) return(false);

  if (!GetRegKey("show_balloon", o.show_balloon, "1", sizeof(o.show_balloon))) return(false);

  if (!GetRegKey("silent_connection", o.silent_connection, "0", sizeof(o.silent_connection))) return(false);

  if (!GetRegKey("show_script_window", o.show_script_window, "1", sizeof(o.show_script_window))) return(false);

  if (!GetRegKey("disconnect_on_suspend", o.disconnect_on_suspend, "1", 
      sizeof(o.disconnect_on_suspend))) return(false);

  if (!GetRegKey("passphrase_attempts", o.psw_attempts_string, "3", 
      sizeof(o.psw_attempts_string))) return(false);
  o.psw_attempts = atoi(o.psw_attempts_string);
  if ((o.psw_attempts < 1) || (o.psw_attempts > 9))
    {
      /* 0 <= passphrase_attempts <= 9 */
      ShowLocalizedMsg(GUI_NAME, ERR_PASSPHRASE_ATTEMPTS, "");
      return(false);
    }

  if (!GetRegKey("connectscript_timeout", o.connectscript_timeout_string, "15", 
      sizeof(o.connectscript_timeout_string))) return(false);
  o.connectscript_timeout = atoi(o.connectscript_timeout_string);
  if ((o.connectscript_timeout < 0) || (o.connectscript_timeout > 99))
    {
      /* 0 <= connectscript_timeout <= 99 */
      ShowLocalizedMsg(GUI_NAME, ERR_CONN_SCRIPT_TIMEOUT, "");
      return(false);
    }

  if (!GetRegKey("disconnectscript_timeout", o.disconnectscript_timeout_string, "10", 
      sizeof(o.disconnectscript_timeout_string))) return(false);
  o.disconnectscript_timeout = atoi(o.disconnectscript_timeout_string);
  if ((o.disconnectscript_timeout <= 0) || (o.disconnectscript_timeout > 99))
    {
      /* 0 < disconnectscript_timeout <= 99 */
      ShowLocalizedMsg(GUI_NAME, ERR_DISCONN_SCRIPT_TIMEOUT, "");
      return(false);
    }

  if (!GetRegKey("preconnectscript_timeout", o.preconnectscript_timeout_string, "10", 
      sizeof(o.preconnectscript_timeout_string))) return(false);
  o.preconnectscript_timeout = atoi(o.preconnectscript_timeout_string);
  if ((o.preconnectscript_timeout <= 0) || (o.preconnectscript_timeout > 99))
    {
      /* 0 < disconnectscript_timeout <= 99 */
      ShowLocalizedMsg(GUI_NAME, ERR_PRECONN_SCRIPT_TIMEOUT, "");
      return(false);
    }

  return(true);
}


int GetRegKey(const char name[], char *data, const char default_data[], DWORD len)
{
  LONG status;
  DWORD type;
  HKEY openvpn_key;
  HKEY openvpn_key_write;
  DWORD dwDispos;
  char expanded_string[MAX_PATH];
  DWORD max_len;

  /* Save maximum string length */
  max_len=len;

  /* If option is already set via cmd-line, return */
  if (data[0] != 0) 
    {
      // Expand environment variables inside the string.
      ExpandEnvironmentStrings(data, expanded_string, sizeof(expanded_string));
      strncpy(data, expanded_string, max_len);
      return(true);
    }

  status = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                       "SOFTWARE\\OpenVPN-GUI",
                       0,
                       KEY_READ,
                       &openvpn_key);

  if (status != ERROR_SUCCESS)
    {
      if (RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                        "Software\\OpenVPN-GUI",
                        0,
                        (LPTSTR) "",
                        REG_OPTION_NON_VOLATILE,
                        KEY_READ | KEY_WRITE,
                        NULL,
                        &openvpn_key,
                        &dwDispos) != ERROR_SUCCESS)
        {
          /* error creating registry key */
          ShowLocalizedMsg(GUI_NAME, ERR_CREATE_REG_KEY, "");
          return(false);
        }  
    }


  /* get a registry string */
  status = RegQueryValueEx(openvpn_key, name, NULL, &type, (byte *) data, &len);
  if (status != ERROR_SUCCESS || type != REG_SZ)
    {
      /* key did not exist - set default value */
      status = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
			  "SOFTWARE\\OpenVPN-GUI",
			  0,
			  KEY_READ | KEY_WRITE,
			  &openvpn_key_write);

      if (status != ERROR_SUCCESS) {
         /* can't open registry for writing */
         ShowLocalizedMsg(GUI_NAME, ERR_OPEN_WRITE_REG, "");
         return(false);
      }    
      if(RegSetValueEx(openvpn_key_write,
                       name,
                       0,
                       REG_SZ,
                       default_data,
                       strlen(default_data)+1))
        {
          /* cant read / set reg-key */ 
          ShowLocalizedMsg(GUI_NAME, ERR_READ_SET_KEY, name);
          return(false);
        }
      strncpy(data, default_data, max_len);
      RegCloseKey(openvpn_key_write);

    }

  RegCloseKey(openvpn_key);

  // Expand environment variables inside the string.
  ExpandEnvironmentStrings(data, expanded_string, sizeof(expanded_string));
  strncpy(data, expanded_string, max_len);

  return(true);
}

LONG GetRegistryValue(HKEY regkey, const char *name, char *data, DWORD len)
{
  LONG status;
  DWORD type;
  DWORD data_len;

  data_len = len;

  /* get a registry string */
  status = RegQueryValueEx(regkey, name, NULL, &type, (byte *) data, &data_len);
  if (status != ERROR_SUCCESS || type != REG_SZ)
    return(0);

  return(data_len);

}

int SetRegistryValue(HKEY regkey, const char *name, char *data)
{
  /* set a registry string */
  if(RegSetValueEx(regkey, name, 0, REG_SZ, data, strlen(data)+1) != ERROR_SUCCESS)
    {
      /* Error writing registry value */
      ShowLocalizedMsg(GUI_NAME, ERR_WRITE_REGVALUE, GUI_REGKEY_HKCU, name);
      return(0);
    }

  return(1);

}
