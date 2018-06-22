/*
 *  This file is a part of OpenVPN-GUI -- A Windows GUI for OpenVPN.
 *
 *  Copyright (C) 2018 Selva Nair <selva.nair@gmail.com>
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

#ifndef SCRIPT_SECURITY_H
#define SCRIPT_SECURITY_H

#include "options.h"

/* script-security flags for OpenVPN */
#define SSEC_UNDEF   -1  /* Do not override the setting in the config file */
#define SSEC_NONE     0  /* No external command execution permitted */
#define SSEC_BUILT_IN 1  /* Permit only built-in commands such as ipconfig, netsh */
#define SSEC_SCRIPTS  2  /* Permit running of user speciifed scripts and executables */
#define SSEC_PW_ENV   3  /* Permit running of scripts/executables that may receive
                          * sensitive data such as passwords through the environment */

static inline int
ssec_default(void)
{
    return SSEC_BUILT_IN;
}

/* Return the permitted script security setting for a connection profile.
 * If no setting is found returns ssec_default()
 */
int get_script_security(const connection_t *c);

/* Update script security setting for a connection profile */
void set_script_security(connection_t *c, int ssec_flag);

/* Lock or unlock the config file for connection profile c
 * to avoid modification when active. A shared lock is created
 * so that other processes can read the file.
 * Lock if lock is true, unlock otherwise.
 */
void lock_config_file(connection_t *c, BOOL lock);

#endif /* SCRIPT_SECURITY_H */
