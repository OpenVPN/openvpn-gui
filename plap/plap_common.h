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

#ifndef PLAP_HELPER_H
#define PLAP_HELPER_H

#include <winsock2.h> /* avoids warning about winsock2 not included first */
#include <windows.h>

#ifdef __GNUC__
#define UNUSED __attribute__((unused))
#else
#define UNUSED
#endif

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef DEBUG
#define dmsg(fmt, ...) x_dmsg(__FILE__, __func__, fmt, ##__VA_ARGS__)
#else
#define dmsg(...) \
    do            \
    {             \
        ;         \
    } while (0)
#endif

    void x_dmsg(const char *file, const char *func, const wchar_t *fmt, ...);

    void init_debug();

    void uninit_debug();

    void debug_print_guid(const GUID *riid, const wchar_t *context);

/* Shortcuts for cumbersome calls to COM methods of an object through its v-table */
#define ADDREF(p)                     (p)->lpVtbl->AddRef(p)
#define RELEASE(p)                    (p)->lpVtbl->Release(p)
#define QUERY_INTERFACE(p, riid, ppv) (p)->lpVtbl->QueryInterface(p, riid, ppv)

#ifdef __cplusplus
}
#endif

#endif /* PLAP_HELPER_H */

/* GUID for a OpenVPNProvider -- created using uuidgen -r
 * The following must be outside any include guards. Include <initguid.h>
 * before including this file in one location where this GUID has to be created.
 * In other places this will get defined as an extern.
 */
#ifdef DEFINE_GUID
DEFINE_GUID(CLSID_OpenVPNProvider,
            0x4fbb8b67,
            0xcf02,
            0x4982,
            0xa7,
            0xa8,
            0x3d,
            0xd0,
            0x6a,
            0x2c,
            0x2e,
            0xbd);
#endif
