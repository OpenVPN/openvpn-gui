/*
 *  OpenVPN-GUI -- A Windows GUI for OpenVPN.
 *
 *  Copyright (C) 2017 Selva Nair <selva.nair@gmail.com>
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
#include <wchar.h>
#include "main.h"
#include "options.h"
#include "misc.h"
#include "openvpn.h"
#include "echo.h"
#include "tray.h"

/* echo msg types */
#define ECHO_MSG_WINDOW (1)
#define ECHO_MSG_NOTIFY (2)

struct echo_msg_history {
    struct echo_msg_fp fp;
    struct echo_msg_history *next;
};

/* We use a global message window for all messages
 */
static HWND echo_msg_window;

/* Forward declarations */
static void
AddMessageBoxText(HWND hwnd, const wchar_t *text, const wchar_t *title, BOOL show);

void
echo_msg_init(void)
{
    /* TODO: create a message box and save handle in echo_msg_window */
    return;
}

/* compute a digest of the message and add it to the msg struct */
static void
echo_msg_add_fp(struct echo_msg *msg, time_t timestamp)
{
    msg->fp.timestamp = timestamp;
    /* digest not implemented */
    return;
}

/* Add message to history -- update if already present */
static void
echo_msg_save(struct echo_msg *msg)
{
    /* Not implemented */
    return;
}

/* persist echo msg history to the registry */
void
echo_msg_persist(connection_t *c)
{
    /* Not implemented */
    return;
}

/* load echo msg history from registry */
void
echo_msg_load(connection_t *c)
{
    /* Not implemented */
    return;
}

/* Return true if the message is same as recently shown */
static BOOL
echo_msg_repeated(const struct echo_msg *msg)
{
    /* Not implemented */
    return false;
}

/* Append a line of echo msg */
static void
echo_msg_append(connection_t *c, time_t UNUSED timestamp, const char *msg, BOOL addnl)
{
    wchar_t *eol = L"";
    wchar_t *wmsg = NULL;

    if (!(wmsg = Widen(msg)))
    {
        WriteStatusLog(c, L"GUI> ", L"Error: out of memory while processing echo msg", false);
        goto out;
    }

    size_t len = c->echo_msg.txtlen + wcslen(wmsg) + 1;  /* including null terminator */
    if (addnl)
    {
        eol = L"\r\n";
        len += 2;
    }
    WCHAR *s = realloc(c->echo_msg.text, len*sizeof(WCHAR));
    if (!s)
    {
        WriteStatusLog(c, L"GUI> ", L"Error: out of memory while processing echo msg", false);
        goto out;
    }
    swprintf(s + c->echo_msg.txtlen, len - c->echo_msg.txtlen,  L"%s%s", wmsg, eol);

    s[len-1] = L'\0';
    c->echo_msg.text = s;
    c->echo_msg.txtlen = len - 1; /* exclude null terminator */

out:
    free(wmsg);
    return;
}

/* Called when echo msg-window or echo msg-notify is received */
static void
echo_msg_display(connection_t *c, time_t timestamp, const char *title, int type)
{
    WCHAR *wtitle = Widen(title);

    if (wtitle)
    {
        c->echo_msg.title = wtitle;
    }
    else
    {
        WriteStatusLog(c, L"GUI> ", L"Error: out of memory converting echo message title to widechar", false);
        c->echo_msg.title = L"Message from server";
    }
    echo_msg_add_fp(&c->echo_msg, timestamp); /* add fingerprint: digest+timestamp */

     /* Check whether the message is muted */
    if (c->flags & FLAG_DISABLE_ECHO_MSG || echo_msg_repeated(&c->echo_msg))
    {
        return;
    }
    if (type == ECHO_MSG_WINDOW)
    {
        HWND h = echo_msg_window;
        if (h)
        {
            AddMessageBoxText(h, c->echo_msg.text, c->echo_msg.title, true);
        }
    }
    else /* notify */
    {
        ShowTrayBalloon(c->echo_msg.title, c->echo_msg.text);
    }
    /* save or update history */
    echo_msg_save(&c->echo_msg);
}

void
echo_msg_process(connection_t *c, time_t timestamp, const char *s)
{
    wchar_t errmsg[256] = L"";

    char *msg = url_decode(s);
    if (!msg)
    {
        WriteStatusLog(c, L"GUI> ", L"Error in url_decode of echo message", false);
        return;
    }

    if (strbegins(msg, "msg "))
    {
        echo_msg_append(c, timestamp, msg + 4, true);
    }
    else if (streq(msg, "msg")) /* empty msg is treated as a new line */
    {
        echo_msg_append(c, timestamp, msg+3, true);
    }
    else if (strbegins(msg, "msg-n "))
    {
        echo_msg_append(c, timestamp, msg + 6, false);
    }
    else if (strbegins(msg, "msg-window "))
    {
        echo_msg_display(c, timestamp, msg + 11, ECHO_MSG_WINDOW);
        echo_msg_clear(c, false);
    }
    else if (strbegins(msg, "msg-notify "))
    {
        echo_msg_display(c, timestamp, msg + 11, ECHO_MSG_NOTIFY);
        echo_msg_clear(c, false);
    }
    else
    {
        _sntprintf_0(errmsg, L"WARNING: Unknown ECHO directive '%hs' ignored.", msg);
        WriteStatusLog(c, L"GUI> ", errmsg, false);
    }
    free(msg);
}

void
echo_msg_clear(connection_t *c, BOOL clear_history)
{
    CLEAR(c->echo_msg.fp);
    free(c->echo_msg.text);
    free(c->echo_msg.title);
    c->echo_msg.text = NULL;
    c->echo_msg.txtlen = 0;
    c->echo_msg.title = NULL;

    if (clear_history)
    {
        echo_msg_persist(c);
        struct echo_msg_history *head = c->echo_msg.history;
        struct echo_msg_history *next;
        while (head)
        {
            next = head->next;
            free(head);
            head = next;
        }
        CLEAR(c->echo_msg);
    }
}

/* Add new message to the message box window and optionally show it */
static void
AddMessageBoxText(HWND hwnd, const wchar_t *text, const wchar_t *title, BOOL show)
{
    /* Not implemented */
    return;
}
