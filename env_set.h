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

#ifndef ENV_SET_H
#define ENV_SET_H

#include <wchar.h>
#include "options.h"

/*
 * data structures and methods for config specific env set and echo setenv
 */
struct env_item;
/* free all env set resources -- to be called when a connection thread exits */
void env_item_del_all(struct env_item *head);
/* parse setenv name val to add name=val to the connection env set */
void process_setenv(connection_t *c, time_t timestamp, const char *msg);

/**
 * Make an env block by merging items in es to the process env block
 * retaining alphabetical order as necessary on Windows.
 * Returns a newly allocated string that may be passed to CreateProcess
 * as the env block or NULL on error. The caller must free the returned
 * pointer.
 */
wchar_t * merge_env_block(const struct env_item *es);

#endif
