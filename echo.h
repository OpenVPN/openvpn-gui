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

#ifndef ECHO_H
#define ECHO_H

#include <wchar.h>

/* data structures and methods for handling echo msg */
#define HASHLEN 20

/* message finger print consists of a SHA1 hash and a timestamp */
struct echo_msg_fp
{
    BYTE digest[HASHLEN];
    time_t timestamp;
};
struct echo_msg_history;
struct echo_msg
{
    struct echo_msg_fp fp; /* keep this as the first element */
    wchar_t *title;
    wchar_t *text;
    int txtlen;
    int type;
    struct echo_msg_history *history;
};

/* init echo message -- call on program start */
void echo_msg_init();

/* Process echo msg and related commands received from mgmt iterface. */
void echo_msg_process(connection_t *c, time_t timestamp, const char *msg);

/* Clear echo msg buffers and optionally history */
void echo_msg_clear(connection_t *c, BOOL clear_history);

/* Load echo msg history from the registry */
void echo_msg_load(connection_t *c);

#endif /* ifndef ECHO_H */
