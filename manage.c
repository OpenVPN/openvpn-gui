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

#include <winsock2.h>

#include "options.h"
#include "main.h"
#include "manage.h"

extern options_t o;

static mgmt_msg_func rtmsg_handler[mgmt_rtmsg_type_max];


/*
 * Initialize the real-time notification handlers
 */
void
InitManagement(const mgmt_rtmsg_handler *handler)
{
    int i;
    for (i = 0; handler[i].handler; ++i)
    {
        rtmsg_handler[handler[i].type] = handler[i].handler;
    }
}


/*
 * Connect to the OpenVPN management interface and register
 * asynchronous socket event notification for it
 */
BOOL
OpenManagement(connection_t *c, u_long addr, u_short port)
{
    WSADATA wsaData;
    SOCKADDR_IN skaddr = {
        .sin_family = AF_INET,
        .sin_addr.s_addr = addr,
        .sin_port = htons(port)
    };

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        return FALSE;

    c->manage.sk = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (c->manage.sk == INVALID_SOCKET)
        return FALSE;

    if (WSAAsyncSelect(c->manage.sk, c->hwndStatus, WM_MANAGEMENT,
        FD_CONNECT|FD_READ|FD_WRITE|FD_CLOSE) != 0)
        return FALSE;

    connect(c->manage.sk, (SOCKADDR *)&skaddr, sizeof(skaddr));

    return TRUE;
}


/*
 * Try to send a queued management command to OpenVPN
 */
static void
SendCommand(connection_t *c)
{
    int res;
    mgmt_cmd_t *cmd = c->manage.cmd_queue;
    if (cmd == NULL || cmd->size == 0)
        return;

    res = send(c->manage.sk, cmd->command, cmd->size, 0);
    if (res < 1)
        return;

    if (res != cmd->size)
        memmove(cmd->command, cmd->command + res, cmd->size - res);

    cmd->size -= res;
}


/*
 * Send a command to the OpenVPN management interface
 */
BOOL
ManagementCommand(connection_t *c, char *command, mgmt_msg_func handler, mgmt_cmd_type type)
{
    mgmt_cmd_t *cmd = calloc(1, sizeof(*cmd));
    if (cmd == NULL)
        return FALSE;

    cmd->size = strlen(command) + 1;
    cmd->command = malloc(cmd->size);
    if (cmd->command == NULL)
    {
        free(cmd);
        return FALSE;
    }
    memcpy(cmd->command, command, cmd->size);
    *(cmd->command + cmd->size - 1) = '\n';

    cmd->handler = handler;
    cmd->type = type;

    if (c->manage.cmd_queue)
    {
        cmd->next = c->manage.cmd_queue;
        cmd->prev = c->manage.cmd_queue->prev;
        cmd->next->prev = cmd->prev->next = cmd;
    }
    else
    {
        cmd->next = cmd->prev = cmd;
        c->manage.cmd_queue = cmd;
    }

    if (c->manage.cmd_queue == cmd)
        SendCommand(c);

    return TRUE;
}


/*
 * Remove a command from a connection's command queue
 */
static BOOL
UnqueueCommand(connection_t *c)
{
    mgmt_cmd_t *cmd = c->manage.cmd_queue;
    if (!cmd)
        return FALSE;

    if (cmd->type == combined)
    {
        cmd->type = regular;
        return TRUE;
    }

    if (cmd->next == cmd)
    {
        c->manage.cmd_queue = NULL;
    }
    else
    {
        cmd->prev->next = cmd->next;
        cmd->next->prev = cmd->prev;
        c->manage.cmd_queue = cmd->next;
        SendCommand(c);
    }

    free(cmd->command);
    free(cmd);

    return TRUE;
}


/*
 * Handle management socket events asynchronously
 */
void
OnManagement(SOCKET sk, LPARAM lParam)
{
    int res;
    char *pos = NULL;
    char data[MAX_LOG_LENGTH];
    connection_t *c = GetConnByManagement(sk);
    if (c == NULL)
        return;

    switch (WSAGETSELECTEVENT(lParam))
    {
    case FD_CONNECT:
        if (WSAGETSELECTERROR(lParam))
            SendMessage(c->hwndStatus, WM_CLOSE, 0, 0);
        break;

    case FD_READ:
        /* Check if there's a complete line to read */
        res = recv(c->manage.sk, data, sizeof(data), MSG_PEEK);
        if (res < 1)
            return;

        pos = memchr(data, (*c->manage.password ? ':' : '\n'), res);
        if (!pos)
            return;

        /* There is data available: read it */
        res = recv(c->manage.sk, data, pos - data + 1, 0);
        if (res != pos - data + 1)
            return;

        /* Reply to a management password request */
        if (*c->manage.password)
        {
            ManagementCommand(c, c->manage.password, NULL, regular);
            *c->manage.password = '\0';
            return;
        }

        /* Handle regular management interface output */
        data[pos - data - 1] = '\0';
        if (data[0] == '>')
        {
            /* Real time notifications */
            pos = data + 1;
            if (strncmp(pos, "LOG:", 4) == 0)
            {
                if (rtmsg_handler[log])
                    rtmsg_handler[log](c, pos + 4);
            }
            else if (strncmp(pos, "STATE:", 6) == 0)
            {
                if (rtmsg_handler[state])
                    rtmsg_handler[state](c, pos + 6);
            }
            else if (strncmp(pos, "HOLD:", 5) == 0)
            {
                if (rtmsg_handler[hold])
                    rtmsg_handler[hold](c, pos + 5);
            }
            else if (strncmp(pos, "PASSWORD:", 9) == 0)
            {
                if (rtmsg_handler[password])
                    rtmsg_handler[password](c, pos + 9);
            }
            else if (strncmp(pos, "INFO:", 5) == 0)
            {
                /* delay until management interface accepts input */
                Sleep(100);
                if (rtmsg_handler[ready])
                    rtmsg_handler[ready](c, pos + 5);
            }
        }
        else if (c->manage.cmd_queue)
        {
            /* Response to commands */
            mgmt_cmd_t *cmd = c->manage.cmd_queue;
            if (strncmp(data, "SUCCESS:", 8) == 0)
            {
                if (cmd->handler)
                    cmd->handler(c, data + 9);
                UnqueueCommand(c);
            }
            else if (strncmp(data, "ERROR:", 6) == 0)
            {
                if (cmd->handler)
                    cmd->handler(c, NULL);
                UnqueueCommand(c);
            }
            else if (strcmp(data, "END") == 0)
            {
                UnqueueCommand(c);
            }
            else if (cmd->handler)
            {
                cmd->handler(c, data);
            }
        }
        break;

    case FD_WRITE:
        SendCommand(c);
        break;

    case FD_CLOSE:
        closesocket(c->manage.sk);
        c->manage.sk = INVALID_SOCKET;
        while (UnqueueCommand(c))
            ;
        WSACleanup();
        if (rtmsg_handler[stop])
            rtmsg_handler[stop](c, "");
        break;
    }
}
