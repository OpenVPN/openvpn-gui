/*
 *  OpenVPN-GUI -- A Windows GUI for OpenVPN.
 *
 *  Copyright (C) 2004 Mathias Sundman <mathias@nilings.se>
 *                2010 Heiko Hund <heikoh@users.sf.net>
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
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <windows.h>

#include "main.h"
#include "openvpn-gui-res.h"
#include "options.h"
#include "localization.h"
#include "save_pass.h"
#include "misc.h"
#include "passphrase.h"

typedef enum
{
    match_false,
    match_file,
    match_dir
} match_t;

extern options_t o;

static match_t
match(const WIN32_FIND_DATA *find, const TCHAR *ext)
{
    size_t ext_len = _tcslen(ext);
    int i;

    if (find->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        return match_dir;

    if (ext_len == 0)
        return match_file;

    i = _tcslen(find->cFileName) - ext_len - 1;

    if (i > 0 && find->cFileName[i] == '.'
    && _tcsicmp(find->cFileName + i + 1, ext) == 0)
        return match_file;

    return match_false;
}

static bool
CheckReadAccess (const TCHAR *dir, const TCHAR *file)
{
    TCHAR path[MAX_PATH];

    _sntprintf_0 (path, _T("%s\\%s"), dir, file);

    return CheckFileAccess (path, GENERIC_READ);
}

static int
ConfigAlreadyExists(TCHAR *newconfig)
{
    int i;
    for (i = 0; i < o.num_configs; ++i)
    {
        if (_tcsicmp(o.conn[i].config_file, newconfig) == 0)
            return true;
    }
    return false;
}

static void
AddConfigFileToList(int config, const TCHAR *filename, const TCHAR *config_dir)
{
    connection_t *c = &o.conn[config];
    int i;

    memset(c, 0, sizeof(*c));

    _tcsncpy(c->config_file, filename, _countof(c->config_file) - 1);
    _tcsncpy(c->config_dir, config_dir, _countof(c->config_dir) - 1);
    _tcsncpy(c->config_name, c->config_file, _countof(c->config_name) - 1);
    c->config_name[_tcslen(c->config_name) - _tcslen(o.ext_string) - 1] = _T('\0');
    _sntprintf_0(c->log_path, _T("%s\\%s.log"), o.log_dir, c->config_name);

    c->manage.sk = INVALID_SOCKET;
    c->manage.skaddr.sin_family = AF_INET;
    c->manage.skaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    c->manage.skaddr.sin_port = htons(25340 + config);

#ifndef DISABLE_CHANGE_PASSWORD
    if (CheckKeyFileWriteAccess (c))
        c->flags |= FLAG_ALLOW_CHANGE_PASSPHRASE;
#endif

    /* Check if connection should be autostarted */
    for (i = 0; i < MAX_CONFIGS && o.auto_connect[i]; ++i)
    {
        if (_tcsicmp(c->config_file, o.auto_connect[i]) == 0)
        {
            c->auto_connect = true;
            break;
        }
    }
    /* check whether passwords are saved */
    if (o.disable_save_passwords)
    {
        DisableSavePasswords(c);
    }
    else
    {
        if (IsAuthPassSaved(c->config_name))
            c->flags |= FLAG_SAVE_AUTH_PASS;
        if (IsKeyPassSaved(c->config_name))
            c->flags |= FLAG_SAVE_KEY_PASS;
    }
}


static void
BuildFileList0(const TCHAR *config_dir, bool warn_duplicates)
{
    WIN32_FIND_DATA find_obj;
    HANDLE find_handle;
    TCHAR find_string[MAX_PATH];
    TCHAR subdir_table[MAX_CONFIG_SUBDIRS][MAX_PATH];
    int subdirs = 0;
    int i;

    _sntprintf_0(find_string, _T("%s\\*"), config_dir);
    find_handle = FindFirstFile(find_string, &find_obj);
    if (find_handle == INVALID_HANDLE_VALUE)
        return;

    /* Loop over each config file in main config dir */
    do
    {
        if (o.num_configs >= MAX_CONFIGS)
        {
            ShowLocalizedMsg(IDS_ERR_MANY_CONFIGS, MAX_CONFIGS);
            break;
        }

        match_t match_type = match(&find_obj, o.ext_string);
        if (match_type == match_file)
        {
            if (ConfigAlreadyExists(find_obj.cFileName))
            {
                if (warn_duplicates)
                    ShowLocalizedMsg(IDS_ERR_CONFIG_EXIST, find_obj.cFileName);
                continue;
            }

            if (CheckReadAccess (config_dir, find_obj.cFileName))
                AddConfigFileToList(o.num_configs++, find_obj.cFileName, config_dir);
        }
        else if (match_type == match_dir)
        {
            if (_tcsncmp(find_obj.cFileName, _T("."), _tcslen(find_obj.cFileName)) != 0
            &&  _tcsncmp(find_obj.cFileName, _T(".."), _tcslen(find_obj.cFileName)) != 0
            &&  subdirs < MAX_CONFIG_SUBDIRS)
            {
                /* Add dir to dir_table */
                _sntprintf_0(subdir_table[subdirs], _T("%s\\%s"), config_dir, find_obj.cFileName);
                subdirs++;
            }
        }
    } while (FindNextFile(find_handle, &find_obj));

    FindClose(find_handle);

    /* Loop over each config file in every subdir */
    for (i = 0; i < subdirs; ++i)
    {
        _sntprintf_0(find_string, _T("%s\\*"), subdir_table[i]);

        find_handle = FindFirstFile (find_string, &find_obj);
        if (find_handle == INVALID_HANDLE_VALUE)
            continue;

        do
        {
            if (o.num_configs >= MAX_CONFIGS)
            {
                ShowLocalizedMsg(IDS_ERR_MANY_CONFIGS, MAX_CONFIGS);
                FindClose(find_handle);
                return;
            }

            /* does file have the correct type and extension? */
            if (match(&find_obj, o.ext_string) != match_file)
                continue;

            if (ConfigAlreadyExists(find_obj.cFileName))
            {
                if (warn_duplicates)
                    ShowLocalizedMsg(IDS_ERR_CONFIG_EXIST, find_obj.cFileName);
                continue;
            }

            if (CheckReadAccess (subdir_table[i], find_obj.cFileName))
                AddConfigFileToList(o.num_configs++, find_obj.cFileName, subdir_table[i]);
        } while (FindNextFile(find_handle, &find_obj));

        FindClose(find_handle);
    }
}

void
BuildFileList()
{
    static bool issue_warnings = true;

    if (o.silent_connection)
        issue_warnings = false;

    /*
     * If no connections are active reset num_configs and rescan
     * to make a new list. Else we keep all current configs and
     * rescan to add any new one's found
     */
    if (CountConnState(disconnected) == o.num_configs)
        o.num_configs = 0;

    BuildFileList0 (o.config_dir, issue_warnings);

    if (_tcscmp (o.global_config_dir, o.config_dir))
        BuildFileList0 (o.global_config_dir, issue_warnings);

    if (o.num_configs == 0 && issue_warnings)
        ShowLocalizedMsg(IDS_NFO_NO_CONFIGS, o.config_dir, o.global_config_dir);

    issue_warnings = false;
}
