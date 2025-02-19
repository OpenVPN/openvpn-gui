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
    {
        return match_dir;
    }

    if (ext_len == 0)
    {
        return match_file;
    }

    i = _tcslen(find->cFileName) - ext_len - 1;

    if (i > 0 && find->cFileName[i] == '.' && _tcsicmp(find->cFileName + i + 1, ext) == 0)
    {
        return match_file;
    }

    return match_false;
}

static bool
CheckReadAccess(const TCHAR *dir, const TCHAR *file)
{
    TCHAR path[MAX_PATH];

    _sntprintf_0(path, _T("%ls\\%ls"), dir, file);

    return CheckFileAccess(path, GENERIC_READ);
}

static int
ConfigAlreadyExists(TCHAR *newconfig)
{
    for (connection_t *c = o.chead; c; c = c->next)
    {
        if (_tcsicmp(c->config_file, newconfig) == 0)
        {
            return true;
        }
    }
    return false;
}

static void
AddConfigFileToList(int group, const TCHAR *filename, const TCHAR *config_dir)
{
    connection_t *c = calloc(1, sizeof(connection_t));

    if (!c)
    {
        ErrorExit(1, L"Out of memory in AddConfigFileToList");
    }

    if (o.ctail)
    {
        o.ctail->next = c;
        o.ctail = c;
    }
    else
    {
        o.chead = o.ctail = c;
    }

    c->id = o.num_configs++;
    c->group = group;

    _tcsncpy(c->config_file, filename, _countof(c->config_file) - 1);
    _tcsncpy(c->config_dir, config_dir, _countof(c->config_dir) - 1);
    _tcsncpy(c->config_name, c->config_file, _countof(c->config_name) - 1);
    c->config_name[_tcslen(c->config_name) - _tcslen(o.ext_string) - 1] = _T('\0');
    _sntprintf_0(c->log_path, _T("%ls\\%ls.log"), o.log_dir, c->config_name);

    c->manage.sk = INVALID_SOCKET;
    c->manage.skaddr.sin_family = AF_INET;
    c->manage.skaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    c->manage.skaddr.sin_port = htons(o.mgmt_port_offset + c->id);

    if (wcsstr(config_dir, o.config_auto_dir))
    {
        c->flags |= FLAG_DAEMON_PERSISTENT;
        _sntprintf_0(c->log_path, _T("%ls\\%ls.log"), o.global_log_dir, c->config_name);
        /* set to auto-connect -- this attempts to attach to them on startup */
        if (o.enable_persistent == 2)
        {
            c->auto_connect = true;
        }
    }

    /* Check if connection should be autostarted */
    for (int i = 0; i < o.num_auto_connect; ++i)
    {
        if (_tcsicmp(c->config_file, o.auto_connect[i]) == 0
            || _tcsicmp(c->config_name, o.auto_connect[i]) == 0)
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
        {
            c->flags |= FLAG_SAVE_AUTH_PASS;
        }
        if (IsKeyPassSaved(c->config_name))
        {
            c->flags |= FLAG_SAVE_KEY_PASS;
        }
    }
    if (o.disable_popup_messages)
    {
        DisablePopupMessages(c);
    }
}

#define FLAG_WARN_DUPLICATES   (0x1)
#define FLAG_WARN_MAX_CONFIGS  (0x2)
#define FLAG_ADD_CONFIG_GROUPS (0x4)

/*
 * Create a new group with the given name as a child of the
 * specified parent group id and returns the id of the new group.
 * If FLAG_ADD_CONFIG_GROUPS is not enabled, returns the
 * parent itself.
 */
static int
NewConfigGroup(const wchar_t *name, int parent, int flags)
{
    if (!(flags & FLAG_ADD_CONFIG_GROUPS))
    {
        return parent;
    }

    if (!o.groups || o.num_groups == o.max_groups)
    {
        o.max_groups += 10;
        void *tmp = realloc(o.groups, sizeof(*o.groups) * o.max_groups);
        if (!tmp)
        {
            o.max_groups -= 10;
            ErrorExit(1, L"Out of memory while grouping configs");
        }
        o.groups = tmp;
    }

    config_group_t *cg = &o.groups[o.num_groups];
    memset(cg, 0, sizeof(*cg));

    _sntprintf_0(cg->name, L"%ls", name);
    cg->id = o.num_groups++;
    cg->parent = parent;
    cg->active = false; /* activated later if not empty */

    return cg->id;
}

/*
 * All groups that link at least one config to the root are
 * enabled. Dangling entries with no terminal configs will stay
 * disabled and are not displayed in the menu tree.
 * Also groups with single configs are squashed if the group
 * and config names match --- this improves the display.
 */
static void
ActivateConfigGroups(void)
{
    /* the root group is always active */
    o.groups[0].active = true;

    /* children is a counter re-used for activation, menu indexing etc. -- reset before use */
    for (int i = 0; i < o.num_groups; i++)
    {
        o.groups[i].children = 0;
    }

    /* count children of each group -- this includes groups
     * and configs which have it as parent
     */
    for (connection_t *c = o.chead; c; c = c->next)
    {
        CONFIG_GROUP(c)->children++;
    }

    for (int i = 1; i < o.num_groups; i++)
    {
        config_group_t *this = &o.groups[i];
        config_group_t *parent = PARENT_GROUP(this);
        if (parent) /* should be true as i = 0 is omitted */
        {
            parent->children++;
        }

        /* unless activated below the group stays inactive */
        this->active = false;
    }

    /* Squash single config directories with name matching the config
     * one depth up. This is done so that automatically imported configs
     * which are added as a single config per directory are handled
     * as if its in the parent directory. This encourages the
     * practice of keeping each config and its  dependencies (certs,
     * script etc.) in a separate directory, without making the menu structure
     * too deeply nested.
     */
    for (connection_t *c = o.chead; c; c = c->next)
    {
        config_group_t *cg = CONFIG_GROUP(c);

        /* if not root and has only this config as child -- squash it */
        if (PARENT_GROUP(cg) && cg->children == 1 && !wcscmp(cg->name, c->config_name))
        {
            cg->children--;
            c->group = cg->parent;
        }
    }

    /* activate all groups that connect a config to the root */
    for (connection_t *c = o.chead; c; c = c->next)
    {
        config_group_t *cg = CONFIG_GROUP(c);

        while (cg)
        {
            cg->active = true;
            cg = PARENT_GROUP(cg);
        }
        /* also deactivate any configs that are no longer readable */
        if (CheckReadAccess(c->config_dir, c->config_file))
        {
            c->flags &= ~FLAG_CONFIG_DISABLED;
        }
        else
        {
            c->flags |= FLAG_CONFIG_DISABLED;
        }
    }
}

/* Scan for configs in config_dir recursing down up to recurse_depth.
 * Input: config_dir -- root of the directory to scan from
 *        group      -- the group into which add the configs to
 *        flags      -- enable warnings, use directory based
 *                      grouping of configs etc.
 * Currently configs in a directory are grouped together and group is
 * the id of the current group in the global group array |o.groups|
 * This may be recursively called until depth becomes 1 and each time
 * the group is changed to that of the directory being recursed into.
 */
static void
BuildFileList0(const TCHAR *config_dir, int recurse_depth, int group, int flags)
{
    WIN32_FIND_DATA find_obj;
    HANDLE find_handle;
    TCHAR find_string[MAX_PATH];
    TCHAR subdir_name[MAX_PATH];

    _sntprintf_0(find_string, _T("%ls\\*"), config_dir);
    find_handle = FindFirstFile(find_string, &find_obj);
    if (find_handle == INVALID_HANDLE_VALUE)
    {
        return;
    }

    /* Loop over each config file in config dir */
    do
    {
        match_t match_type = match(&find_obj, o.ext_string);
        if (match_type == match_file)
        {
            if (ConfigAlreadyExists(find_obj.cFileName))
            {
                if (flags & FLAG_WARN_DUPLICATES)
                {
                    ShowLocalizedMsg(IDS_ERR_CONFIG_EXIST, find_obj.cFileName);
                }
                continue;
            }

            if (CheckReadAccess(config_dir, find_obj.cFileName))
            {
                AddConfigFileToList(group, find_obj.cFileName, config_dir);
            }
        }
    } while (FindNextFile(find_handle, &find_obj));

    FindClose(find_handle);

    /* optionally loop over each subdir */
    if (recurse_depth < 1)
    {
        return;
    }

    find_handle = FindFirstFile(find_string, &find_obj);
    if (find_handle == INVALID_HANDLE_VALUE)
    {
        return;
    }

    do
    {
        match_t match_type = match(&find_obj, o.ext_string);
        if (match_type == match_dir)
        {
            if (wcscmp(find_obj.cFileName, _T(".")) && wcscmp(find_obj.cFileName, _T("..")))
            {
                /* recurse into subdirectory */
                _sntprintf_0(subdir_name, _T("%ls\\%ls"), config_dir, find_obj.cFileName);
                int sub_group = NewConfigGroup(find_obj.cFileName, group, flags);

                BuildFileList0(subdir_name, recurse_depth - 1, sub_group, flags);
            }
        }
    } while (FindNextFile(find_handle, &find_obj));

    FindClose(find_handle);
}

/*
 * Open a path and get its file information structure.
 * Returns true on success, false on error.
 */
static bool
GetFileInfo(const wchar_t *path, BY_HANDLE_FILE_INFORMATION *info)
{
    bool ret = false;

    /* FILE_FLAG_BACKUP_SEMANTICS required to open directories */
    HANDLE fd = CreateFileW(path,
                            0,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            NULL,
                            OPEN_EXISTING,
                            FILE_FLAG_BACKUP_SEMANTICS,
                            NULL);

    if (fd == INVALID_HANDLE_VALUE)
    {
        MsgToEventLog(EVENTLOG_ERROR_TYPE,
                      L"GetFileInfo: Error opening path <%ls> (status = %lu)",
                      path,
                      GetLastError());
        return ret;
    }

    ret = GetFileInformationByHandle(fd, info);
    if (!ret)
    {
        MsgToEventLog(
            EVENTLOG_ERROR_TYPE,
            L"GetFileInfo: Error accessing file information for path <%ls> (status = %lu)",
            path,
            GetLastError());
    }
    else
    {
        PrintDebug(L"path = <%ls> volumeid = %lu file index = (%lu,%lu)",
                   path,
                   info->dwVolumeSerialNumber,
                   info->nFileIndexLow,
                   info->nFileIndexHigh);
    }
    CloseHandle(fd);

    return ret;
}

/*
 * Compare two paths by checking whether they point to the
 * same object in the file system. Returns true if the paths
 * are same, false otherwise.
 * If the two paths are identical strings return true early.
 * If any of the paths do not exist, are not accessible or
 * fail to provide file information, we return false.
 */
static bool
IsSamePath(const wchar_t *path1, const wchar_t *path2)
{
    BOOL ret = false;
    BY_HANDLE_FILE_INFORMATION info1, info2;

    if (_wcsicmp(path1, path2) == 0)
    {
        return true;
    }

    if (GetFileInfo(path1, &info1) && GetFileInfo(path2, &info2))
    {
        ret = (info1.dwVolumeSerialNumber == info2.dwVolumeSerialNumber
               && info1.nFileIndexLow == info2.nFileIndexLow
               && info1.nFileIndexHigh == info2.nFileIndexHigh);
    }

    return ret;
}

void
BuildFileList()
{
    static bool issue_warnings = true;
    int recurse_depth = 20; /* maximum number of levels below config_dir to recurse into */
    int flags = 0;
    static int root_gp, system_gp, persistent_gp;

    if (o.silent_connection)
    {
        issue_warnings = false;
    }

    /*
     * If first time or no entries in the connection list reset groups and rescan
     * to make a new list. Else we keep all current configs and
     * rescan to add any new one's found.
     */
    if (!o.num_configs)
    {
        o.num_groups = 0;
        flags |= FLAG_ADD_CONFIG_GROUPS;
        root_gp = NewConfigGroup(L"ROOT", -1, flags); /* -1 indicates no parent */
        persistent_gp = NewConfigGroup(L"Persistent Profiles", root_gp, flags);
        system_gp = NewConfigGroup(L"System Profiles", root_gp, flags);
    }
    /* else these parent groups use their saved values */

    if (&o.groups[persistent_gp] != PERSISTENT_ROOT_GROUP)
    {
        MsgToEventLog(EVENTLOG_ERROR_TYPE, L"%hs:%d Logic error", __func__, __LINE__);
        return;
    }

    if (issue_warnings)
    {
        flags |= FLAG_WARN_DUPLICATES | FLAG_WARN_MAX_CONFIGS;
    }

    BuildFileList0(o.config_dir, recurse_depth, root_gp, flags);

    if (!IsSamePath(o.global_config_dir, o.config_dir))
    {
        BuildFileList0(o.global_config_dir, recurse_depth, system_gp, flags);
    }

    if (o.service_state == service_connected && o.enable_persistent)
    {
        if (!IsSamePath(o.config_auto_dir, o.config_dir))
        {
            BuildFileList0(o.config_auto_dir, recurse_depth, persistent_gp, flags);
        }
    }

    if (o.num_configs == 0 && issue_warnings)
    {
        ShowLocalizedMsg(IDS_NFO_NO_CONFIGS, o.config_dir, o.global_config_dir);
    }

    ActivateConfigGroups();

    issue_warnings = false;
}

void
FreeConfigList(options_t *o)
{
    connection_t *next = NULL;
    for (connection_t *c = o->chead; c; c = next)
    {
        next = c->next;
        free(c);
    }
    free(o->groups);
    o->groups = NULL;
    o->num_configs = 0;
    o->num_groups = 0;
}
