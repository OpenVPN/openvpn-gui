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

#ifndef REGISTRY_H
#define REGISTRY_H

int GetRegistryKeys(void);

int SaveRegistryKeys(void);

int UpdateRegistry(void);

int GetRegKey(const TCHAR name[], TCHAR data[], const TCHAR default_data[], DWORD len);

LONG GetRegistryValue(HKEY regkey, const TCHAR *name, TCHAR *data, DWORD len);

LONG GetRegistryValueNumeric(HKEY regkey, const TCHAR *name, DWORD *data);

int SetRegistryValue(HKEY regkey, const TCHAR *name, const TCHAR *data);

int SetRegistryValueNumeric(HKEY regkey, const TCHAR *name, DWORD data);

int SetConfigRegistryValueBinary(const WCHAR *config_name,
                                 const WCHAR *name,
                                 const BYTE *data,
                                 DWORD len);

DWORD GetConfigRegistryValue(const WCHAR *config_name, const WCHAR *name, BYTE *data, DWORD len);

int DeleteConfigRegistryValue(const WCHAR *config_name, const WCHAR *name);

#endif /* ifndef REGISTRY_H */
