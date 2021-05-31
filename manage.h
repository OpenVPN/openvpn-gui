/*
 *  OpenVPN-GUI -- A Windows GUI for OpenVPN.
 *
 *  Copyright (C) 2010 Heiko Hund <heikoh@users.sf.net>
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

#ifndef MANAGE_H
#define MANAGE_H

#include <winsock2.h>

typedef enum {
    ready_,
    stop_,
    bytecount_,
    echo_,
    hold_,
    log_,
    password_,
    proxy_,
    state_,
    needok_,
    needstr_,
    pkcs11_id_count_,
    infomsg_,
    mgmt_rtmsg_type_max
} mgmt_rtmsg_type;

typedef enum {
    regular,
    combined
} mgmt_cmd_type;

typedef void (*mgmt_msg_func)(connection_t *, char *);

typedef struct {
    mgmt_rtmsg_type type;
    mgmt_msg_func handler;
} mgmt_rtmsg_handler;

typedef struct mgmt_cmd {
    struct mgmt_cmd *prev, *next;
    char *command;
    int size;
    mgmt_msg_func handler;
    mgmt_cmd_type type;
} mgmt_cmd_t;


void InitManagement(const mgmt_rtmsg_handler *handler);
BOOL OpenManagement(connection_t *);
BOOL ManagementCommand(connection_t *, char *, mgmt_msg_func, mgmt_cmd_type);

void OnManagement(SOCKET, LPARAM);
void CloseManagement(connection_t *);

#endif
