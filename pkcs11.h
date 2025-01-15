/*
 *  OpenVPN-GUI -- A Windows GUI for OpenVPN.
 *
 *  Copyright (C) 2022 Selva Nair <selva.nair@gmail.com>
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

#ifndef PKCS11_H
#define PKCS11_H

struct pkcs11_list
{
    unsigned int count;      /* number of pkcs11 entries */
    unsigned int selected;   /* entry selected by user: -1 if no selection */
    unsigned int state;      /* a flag indicating list filling status */
    struct pkcs11_entry *pe; /* array of pkcs11-id entries */
};

struct connection;
/**
 * Callback for Need 'pkcs11-id-request' notification from management
 * @param c pointer to connection profile
 * @param msg string received from the management
 */
void OnPkcs11(struct connection *c, char *msg);

/**
 * Clear pkcs11 entry list and release memory.
 * @param l pointer to the list
 *
 */
void pkcs11_list_clear(struct pkcs11_list *l);

#endif
