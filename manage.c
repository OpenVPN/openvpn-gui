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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <windows.h>
#include <winsock2.h>
#include <malloc.h>

#include "options.h"
#include "manage.h"
#include "main.h"
#include "misc.h"

extern options_t o;

static mgmt_msg_func rtmsg_handler[mgmt_rtmsg_type_max];

/*
 * Number of seconds to try connecting to management interface
 */
static const time_t max_connect_time = 15;

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
OpenManagement(connection_t *c)
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        return FALSE;
    }

    c->manage.connected = 0;
    c->manage.sk = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (c->manage.sk == INVALID_SOCKET)
    {
        WSACleanup();
        return FALSE;
    }
    if (WSAAsyncSelect(
            c->manage.sk, c->hwndStatus, WM_MANAGEMENT, FD_CONNECT | FD_READ | FD_WRITE | FD_CLOSE)
        != 0)
    {
        return FALSE;
    }

    connect(c->manage.sk, (SOCKADDR *)&c->manage.skaddr, sizeof(c->manage.skaddr));
    c->manage.timeout = time(NULL) + max_connect_time;

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
    {
        return;
    }

    res = send(c->manage.sk, cmd->command, cmd->size, 0);
    if (res < 1)
    {
        return;
    }

    if (res != cmd->size)
    {
        memmove(cmd->command, cmd->command + res, cmd->size - res);
    }

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
    {
        return FALSE;
    }

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
    {
        SendCommand(c);
    }

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
    {
        return FALSE;
    }

    /* Wipe command as it may contain passwords */
    memset(cmd->command, 'x', cmd->size);

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
    char *data;
    ULONG data_size, offset;

    connection_t *c = GetConnByManagement(sk);
    if (c == NULL)
    {
        return;
    }

    switch (WSAGETSELECTEVENT(lParam))
    {
        case FD_CONNECT:
            if (WSAGETSELECTERROR(lParam))
            {
                /* keep trying for connections with persistent daemons */
                if (c->flags & FLAG_DAEMON_PERSISTENT || time(NULL) < c->manage.timeout)
                {
                    /* show a message on status window */
                    if (rtmsg_handler[log_] && (c->flags & FLAG_DAEMON_PERSISTENT))
                    {
                        char buf[256];
                        _snprintf_0(buf,
                                    "%lld,W,Waiting for the management interface to come up",
                                    (long long)time(NULL)) rtmsg_handler[log_](c, buf);
                    }

                    connect(c->manage.sk, (SOCKADDR *)&c->manage.skaddr, sizeof(c->manage.skaddr));
                }
                else
                {
                    /* Connection to MI timed out. */
                    CloseManagement(c);
                    if (c->state != disconnected)
                    {
                        rtmsg_handler[timeout_](c, "");
                    }
                }
            }
            else
            {
                c->manage.connected = 1;
            }
            break;

        case FD_READ:
            if (ioctlsocket(c->manage.sk, FIONREAD, &data_size) != 0 || data_size == 0)
            {
                return;
            }

            data = malloc(c->manage.saved_size + data_size);
            if (data == NULL)
            {
                return;
            }

            res = recv(c->manage.sk, data + c->manage.saved_size, data_size, 0);
            if (res != (int)data_size)
            {
                free(data);
                return;
            }

            /* Copy previously saved management data */
            if (c->manage.saved_size)
            {
                memcpy(data, c->manage.saved_data, c->manage.saved_size);
                data_size += c->manage.saved_size;
                free(c->manage.saved_data);
                c->manage.saved_data = NULL;
                c->manage.saved_size = 0;
            }

            offset = 0;
            while (offset < data_size)
            {
                char *pos;
                char *line = data + offset;
                size_t line_size = data_size - offset;
                BOOL passwd_request = false;
                const char *passwd_prompt = "ENTER PASSWORD:";

                if (line_size >= strlen(passwd_prompt)
                    && memcmp(line, passwd_prompt, strlen(passwd_prompt)) == 0)
                {
                    pos = memchr(line, ':', line_size);
                    passwd_request = true;
                }
                else
                {
                    pos = memchr(line, '\n', line_size);
                }

                if (pos == NULL)
                {
                    c->manage.saved_data = malloc(line_size);
                    if (c->manage.saved_data)
                    {
                        c->manage.saved_size = line_size;
                        memcpy(c->manage.saved_data, line, c->manage.saved_size);
                    }
                    break;
                }

                offset += (pos - line) + 1;

                /* Reply to a management password request */
                if (*c->manage.password && passwd_request)
                {
                    ManagementCommand(c, c->manage.password, NULL, regular);
                    SecureZeroMemory(c->manage.password, sizeof(c->manage.password));

                    continue;
                }

                if (!*c->manage.password && passwd_request)
                {
                    /* either we don't have a password or we used it and didn't match */
                    MsgToEventLog(EVENTLOG_WARNING_TYPE,
                                  L"%ls: management password mismatch",
                                  c->config_name);
                    c->state = disconnecting;
                    CloseManagement(c);
                    rtmsg_handler[stop_](c, "");

                    continue;
                }

                /* Handle regular management interface output */
                line[pos - line - 1] = '\0';
                if (line[0] == '>')
                {
                    /* Real time notifications */
                    pos = line + 1;
                    if (strncmp(pos, "LOG:", 4) == 0)
                    {
                        if (rtmsg_handler[log_])
                        {
                            rtmsg_handler[log_](c, pos + 4);
                        }
                    }
                    else if (strncmp(pos, "STATE:", 6) == 0)
                    {
                        if (rtmsg_handler[state_])
                        {
                            rtmsg_handler[state_](c, pos + 6);
                        }
                    }
                    else if (strncmp(pos, "HOLD:", 5) == 0)
                    {
                        if (rtmsg_handler[hold_])
                        {
                            rtmsg_handler[hold_](c, pos + 5);
                        }
                    }
                    else if (strncmp(pos, "PASSWORD:", 9) == 0)
                    {
                        if (rtmsg_handler[password_])
                        {
                            rtmsg_handler[password_](c, pos + 9);
                        }
                    }
                    else if (strncmp(pos, "PROXY:", 6) == 0)
                    {
                        if (rtmsg_handler[proxy_])
                        {
                            rtmsg_handler[proxy_](c, pos + 6);
                        }
                    }
                    else if (strncmp(pos, "INFO:", 5) == 0)
                    {
                        /* delay until management interface accepts input */
                        /* use real sleep here, since WM_MANAGEMENT might arrive before management
                         * is ready */
                        Sleep(100);
                        c->manage.connected = 2;
                        if (rtmsg_handler[ready_])
                        {
                            rtmsg_handler[ready_](c, pos + 5);
                        }
                    }
                    else if (strncmp(pos, "NEED-OK:", 8) == 0)
                    {
                        if (rtmsg_handler[needok_])
                        {
                            rtmsg_handler[needok_](c, pos + 8);
                        }
                    }
                    else if (strncmp(pos, "NEED-STR:", 9) == 0)
                    {
                        if (rtmsg_handler[needstr_])
                        {
                            rtmsg_handler[needstr_](c, pos + 9);
                        }
                    }
                    else if (strncmp(pos, "ECHO:", 5) == 0)
                    {
                        if (rtmsg_handler[echo_])
                        {
                            rtmsg_handler[echo_](c, pos + 5);
                        }
                    }
                    else if (strncmp(pos, "BYTECOUNT:", 10) == 0)
                    {
                        if (rtmsg_handler[bytecount_])
                        {
                            rtmsg_handler[bytecount_](c, pos + 10);
                        }
                    }
                    else if (strncmp(pos, "INFOMSG:", 8) == 0)
                    {
                        if (rtmsg_handler[infomsg_])
                        {
                            rtmsg_handler[infomsg_](c, pos + 8);
                        }
                    }
                    else if (strncmp(pos, "PKCS11ID", 8) == 0 && c->manage.cmd_queue)
                    {
                        /* This is not a real-time message, but unfortunately implemented
                         * in the core as one. Work around by handling the response here.
                         */
                        mgmt_cmd_t *cmd = c->manage.cmd_queue;
                        if (cmd->handler)
                        {
                            cmd->handler(c, line);
                        }
                        UnqueueCommand(c);
                    }
                }
                else if (c->manage.cmd_queue)
                {
                    /* Response to commands */
                    mgmt_cmd_t *cmd = c->manage.cmd_queue;
                    if (strncmp(line, "SUCCESS:", 8) == 0)
                    {
                        if (cmd->handler)
                        {
                            cmd->handler(c, line + 9);
                        }
                        UnqueueCommand(c);
                    }
                    else if (strncmp(line, "ERROR:", 6) == 0)
                    {
                        /* Response sent to management is not processed. Log an error in status
                         * window  */
                        char buf[256];
                        _snprintf_0(buf,
                                    "%lld,N,Previous command sent to management failed: %s",
                                    (long long)time(NULL),
                                    line) rtmsg_handler[log_](c, buf);

                        if (cmd->handler)
                        {
                            cmd->handler(c, NULL);
                        }
                        UnqueueCommand(c);
                    }
                    else if (strcmp(line, "END") == 0)
                    {
                        UnqueueCommand(c);
                    }
                    else if (cmd->handler)
                    {
                        cmd->handler(c, line);
                    }
                }
            }
            free(data);
            break;

        case FD_WRITE:
            SendCommand(c);
            break;

        case FD_CLOSE:
            CloseManagement(c);
            if (rtmsg_handler[stop_])
            {
                rtmsg_handler[stop_](c, "");
            }
            break;
    }
}

void
CloseManagement(connection_t *c)
{
    if (c->manage.sk != INVALID_SOCKET)
    {
        if (c->manage.saved_size)
        {
            free(c->manage.saved_data);
            c->manage.saved_data = NULL;
            c->manage.saved_size = 0;
        }
        closesocket(c->manage.sk);
        c->manage.sk = INVALID_SOCKET;
        c->manage.connected = 0;
        while (UnqueueCommand(c))
        {
        }
        WSACleanup();
    }
}
