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
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <windows.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>

#include "options.h"
#include "main.h"
#include "openvpn-gui-res.h"
#include "localization.h"

#define streq(x, y) (_tcscmp((x), (y)) == 0)

extern options_t o;

static int
add_option(options_t *options, int i, TCHAR **p)
{
    if (streq(p[0], _T("help")))
    {
        TCHAR caption[200];
        LoadLocalizedStringBuf(caption, _countof(caption), IDS_NFO_USAGECAPTION);
        ShowLocalizedMsgEx(MB_OK, caption, IDS_NFO_USAGE);
        exit(0);
    }
    else if (streq(p[0], _T("connect")) && p[1])
    {
        ++i;
        static int auto_connect_nr = 0;
        if (auto_connect_nr == MAX_CONFIGS)
        {
            /* Too many configs */
            ShowLocalizedMsg(IDS_ERR_MANY_CONFIGS, MAX_CONFIGS);
            exit(1);
        }
        options->auto_connect[auto_connect_nr++] = p[1];
    }
    else if (streq(p[0], _T("exe_path")) && p[1])
    {
        ++i;
        _tcsncpy(options->exe_path, p[1], _countof(options->exe_path) - 1);
    }
    else if (streq(p[0], _T("config_dir")) && p[1])
    {
        ++i;
        _tcsncpy(options->config_dir, p[1], _countof(options->config_dir) - 1);
    }
    else if (streq(p[0], _T("ext_string")) && p[1])
    {
        ++i;
        _tcsncpy(options->ext_string, p[1], _countof(options->ext_string) - 1);
    }
    else if (streq(p[0], _T("log_dir")) && p[1])
    {
        ++i;
        _tcsncpy(options->log_dir, p[1], _countof(options->log_dir) - 1);
    }
    else if (streq(p[0], _T("priority_string")) && p[1])
    {
        ++i;
        _tcsncpy(options->priority_string, p[1], _countof(options->priority_string) - 1);
    }
    else if (streq(p[0], _T("append_string")) && p[1])
    {
        ++i;
        _tcsncpy(options->append_string, p[1], _countof(options->append_string) - 1);
    }
    else if (streq(p[0], _T("log_viewer")) && p[1])
    {
        ++i;
        _tcsncpy(options->log_viewer, p[1], _countof(options->log_viewer) - 1);
    }
    else if (streq(p[0], _T("editor")) && p[1])
    {
        ++i;
        _tcsncpy(options->editor, p[1], _countof(options->editor) - 1);
    }
    else if (streq(p[0], _T("allow_edit")) && p[1])
    {
        ++i;
        _tcsncpy(options->allow_edit, p[1], _countof(options->allow_edit) - 1);
    }
    else if (streq(p[0], _T("allow_service")) && p[1])
    {
        ++i;
        _tcsncpy(options->allow_service, p[1], _countof(options->allow_service) - 1);
    }
    else if (streq(p[0], _T("allow_password")) && p[1])
    {
        ++i;
        _tcsncpy(options->allow_password, p[1], _countof(options->allow_password) - 1);
    }
    else if (streq(p[0], _T("allow_proxy")) && p[1])
    {
        ++i;
        _tcsncpy(options->allow_proxy, p[1], _countof(options->allow_proxy) - 1);
    }
    else if (streq(p[0], _T("show_balloon")) && p[1])
    {
        ++i;
        _tcsncpy(options->show_balloon, p[1], _countof(options->show_balloon) - 1);
    }
    else if (streq(p[0], _T("service_only")) && p[1])
    {
        ++i;
        _tcsncpy(options->service_only, p[1], _countof(options->service_only) - 1);
    }
    else if (streq(p[0], _T("show_script_window")) && p[1])
    {
        ++i;
        _tcsncpy(options->show_script_window, p[1], _countof(options->show_script_window) - 1);
    }
    else if (streq(p[0], _T("silent_connection")) && p[1])
    {
        ++i;
        _tcsncpy(options->silent_connection, p[1], _countof(options->silent_connection) - 1);
    }
    else if (streq(p[0], _T("passphrase_attempts")) && p[1])
    {
        ++i;
        _tcsncpy(options->psw_attempts_string, p[1], _countof(options->psw_attempts_string) - 1);
    }
    else if (streq(p[0], _T("connectscript_timeout")) && p[1])
    {
        ++i;
        _tcsncpy(options->connectscript_timeout_string, p[1], _countof(options->connectscript_timeout_string) - 1);
    }
    else if (streq(p[0], _T("disconnectscript_timeout")) && p[1])
    {
        ++i;
        _tcsncpy(options->disconnectscript_timeout_string, p[1], _countof(options->disconnectscript_timeout_string) - 1);
    }
    else if (streq(p[0], _T("preconnectscript_timeout")) && p[1])
    {
        ++i;
        _tcsncpy(options->preconnectscript_timeout_string, p[1], _countof(options->preconnectscript_timeout_string) - 1);
    }
    else
    {
        /* Unrecognized option or missing parameter */
        ShowLocalizedMsg(IDS_ERR_BAD_OPTION, p[0]);
        exit(1);
    }
    return i;
}


static void
parse_argv(options_t *options, int argc, TCHAR **argv)
{
    int i, j;

    /* parse command line */
    for (i = 1; i < argc; ++i)
    {
        TCHAR *p[MAX_PARMS];
        CLEAR(p);
        p[0] = argv[i];
        if (_tcsncmp(p[0], _T("--"), 2) != 0)
        {
            /* Missing -- before option. */
            ShowLocalizedMsg(IDS_ERR_BAD_PARAMETER, p[0]);
            exit(0);
        }

        p[0] += 2;

        for (j = 1; j < MAX_PARMS; ++j)
        {
            if (i + j < argc)
            {
                TCHAR *arg = argv[i + j];
                if (_tcsncmp(arg, _T("--"), 2) == 0)
                    break;
                p[j] = arg;
            }
        }
        i = add_option(options, i, p);
    }
}


void
InitOptions(options_t *opt)
{
    CLEAR(*opt);
}


void
ProcessCommandLine(options_t *options, TCHAR *command_line)
{
    TCHAR **argv;
    TCHAR *pos = command_line;
    int argc = 0;

    /* Count the arguments */
    do
    {
        while (*pos == _T(' '))
            ++pos;

        if (*pos == _T('\0'))
            break;

        ++argc;

        while (*pos != _T('\0') && *pos != _T(' '))
           ++pos;
    }
    while (*pos != _T('\0'));

    if (argc == 0)
        return;

    /* Tokenize the arguments */
    argv = (TCHAR**) malloc(argc * sizeof(TCHAR*));
    pos = command_line;
    argc = 0;

    do
    {
        while (*pos == _T(' '))
            pos++;

        if (*pos == _T('\0'))
            break;

        if (*pos == _T('\"'))
        {
            argv[argc++] = ++pos;
            while (*pos != _T('\0') && *pos != _T('\"'))
                ++pos;
        }
        else
        {
            argv[argc++] = pos;
            while (*pos != _T('\0') && *pos != _T(' '))
                ++pos;
        }

        if (*pos == _T('\0'))
            break;

        *pos++ = _T('\0');
    }
    while (*pos != _T('\0'));

    parse_argv(options, argc, argv);

    free(argv);
}


/* Return num of connections with state = check */
int
CountConnState(conn_state_t check)
{
    int i;
    int count = 0;

    for (i = 0; i < o.num_configs; ++i)
    {
        if (o.conn[i].state == check)
            ++count;
    }

    return count;
}

connection_t*
GetConnByManagement(SOCKET sk)
{
    int i;
    for (i = 0; i < o.num_configs; ++i)
    {
        if (o.conn[i].manage.sk == sk)
            return &o.conn[i];
    }
    return NULL;
}
