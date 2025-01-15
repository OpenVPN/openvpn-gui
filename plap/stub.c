/*
 *  OpenVPN-PLAP-Provider
 *
 *  Copyright (C) 2017-2022 Selva Nair <selva.nair@gmail.com>
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
#include "plap_common.h"
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include "options.h"
#include "main.h"
#include "localization.h"
#include "ui_glue.h"

void
PrintDebugMsg(wchar_t *msg)
{
    x_dmsg("GUI-source", "", msg);
}

void
ErrorExit(UNUSED int exit_code, const wchar_t *msg)
{
    if (msg)
    {
        MessageBoxExW(NULL,
                      msg,
                      TEXT(PACKAGE_NAME),
                      MB_OK | MB_SETFOREGROUND | MB_ICONERROR,
                      GetGUILanguage());
    }
    DetachAllOpenVPN();
    /* do not exit */
}

void
RecreatePopupMenus(void)
{
    return;
}

void
CreatePopupMenus(void)
{
    return;
}

void
OnNotifyTray(UNUSED LPARAM lp)
{
    return;
}
void
OnDestroyTray(void)
{
    return;
}
void
ShowTrayIcon(void)
{
    return;
}
void
SetTrayIcon(UNUSED conn_state_t state)
{
    return;
}
void
SetMenuStatus(UNUSED connection_t *c, UNUSED conn_state_t state)
{
    return;
}
void
SetServiceMenuStatus(void)
{
    return;
}
void
ShowTrayBalloon(UNUSED wchar_t *s1, UNUSED wchar_t *s2)
{
    return;
}
void
CheckAndSetTrayIcon(void)
{
    return;
}
void
RunPreconnectScript(UNUSED connection_t *c)
{
    return;
}
void
RunConnectScript(UNUSED connection_t *ic, UNUSED int run_as_service)
{
    return;
}
void
RunDisconnectScript(UNUSED connection_t *c, UNUSED int run_as_service)
{
    return;
}
int
SaveKeyPass(UNUSED const WCHAR *config_name, UNUSED const WCHAR *password)
{
    return 1;
}
int
SaveAuthPass(UNUSED const WCHAR *config_name, UNUSED const WCHAR *password)
{
    return 1;
}
int
SaveUsername(UNUSED const WCHAR *config_name, UNUSED const WCHAR *username)
{
    return 1;
}

int
RecallKeyPass(UNUSED const WCHAR *config_name, UNUSED WCHAR *password)
{
    return 0;
}
int
RecallAuthPass(UNUSED const WCHAR *config_name, UNUSED WCHAR *password)
{
    return 0;
}
int
RecallUsername(UNUSED const WCHAR *config_name, UNUSED WCHAR *username)
{
    return 0;
}

void
DeleteSavedAuthPass(UNUSED const WCHAR *config_name)
{
    return;
}

void
DeleteSavedKeyPass(UNUSED const WCHAR *config_name)
{
    return;
}
void
DeleteSavedPasswords(UNUSED const WCHAR *config_name)
{
    return;
}

BOOL
IsAuthPassSaved(UNUSED const WCHAR *config_name)
{
    return 0;
}
BOOL
IsKeyPassSaved(UNUSED const WCHAR *config_name)
{
    return 0;
}
void
env_item_del_all(UNUSED struct env_item *head)
{
    return;
}
void
process_setenv(UNUSED connection_t *c, UNUSED time_t timestamp, UNUSED const char *msg)
{
    return;
}
BOOL
AuthorizeConfig(UNUSED const connection_t *c)
{
    return 1;
}

void
echo_msg_process(UNUSED connection_t *c, UNUSED time_t timestamp, UNUSED const char *msg)
{
    return;
}

void
echo_msg_clear(UNUSED connection_t *c, UNUSED BOOL clear_history)
{
    return;
}

void
echo_msg_load(UNUSED connection_t *c)
{
    return;
}

BOOL
CheckKeyFileWriteAccess(UNUSED connection_t *c)
{
    return 0;
}
