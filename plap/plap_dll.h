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

#ifndef PLAP_DLL_H
#define PLAP_DLL_H

#include <windows.h>

#ifdef __cplusplus
extern "C"
{
#endif

    extern HINSTANCE hinst_global;
    HRESULT OpenVPNProvider_CreateInstance(REFIID riid, void **ppv);

    void dll_addref();

    void dll_release();

    STDAPI DllCanUnloadNow();

    STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void **ppv);

#ifdef __cplusplus
}
#endif

#endif /* PLAP_DLL_H */
