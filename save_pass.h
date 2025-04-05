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

#ifndef SAVEPASS_H
#define SAVEPASS_H

#include <wchar.h>

#define USER_PASS_LEN 128
#define KEY_PASS_LEN  128

int SaveKeyPass(const WCHAR *config_name, const WCHAR *password);

int SaveAuthPass(const WCHAR *config_name, const WCHAR *password);

int SaveUsername(const WCHAR *config_name, const WCHAR *username);

int RecallKeyPass(const WCHAR *config_name, WCHAR *password);

int RecallAuthPass(const WCHAR *config_name, WCHAR *password);

int RecallUsername(const WCHAR *config_name, WCHAR *username);

void DeleteSavedAuthPass(const WCHAR *config_name);

void DeleteSavedKeyPass(const WCHAR *config_name);

void DeleteSavedPasswords(const WCHAR *config_name);

BOOL IsAuthPassSaved(const WCHAR *config_name);

BOOL IsKeyPassSaved(const WCHAR *config_name);

#endif /* ifndef SAVEPASS_H */
