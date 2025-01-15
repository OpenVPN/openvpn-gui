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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <initguid.h> /* needed to define (not extern) the GUIDs */
#include <shlwapi.h>
#include <credentialprovider.h>
#include "plap_common.h"
#include "plap_dll.h"

static LONG dll_ref_count = 0;

HINSTANCE hinst_global = NULL;

/*
 * Dll Entry Point
 */
BOOL WINAPI
DllMain(HINSTANCE hinstDll, DWORD dwReason, UNUSED LPVOID pReserved)
{
    dmsg(L"Entry with dwReason = %lu", dwReason);

    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            /* as per MSDN, this reduces the size of application */
            DisableThreadLibraryCalls(hinstDll);
            init_debug();
            break;

        case DLL_PROCESS_DETACH:
            uninit_debug();
            break;

        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
            break;
    }

    hinst_global = hinstDll; /* global */
    return TRUE;
}

void
dll_addref()
{
    InterlockedIncrement(&dll_ref_count);
    dmsg(L"ref_count after increment = %lu", dll_ref_count);
}

void
dll_release()
{
    InterlockedDecrement(&dll_ref_count);
    dmsg(L"ref_count after decrement = %lu", dll_ref_count);
}

/* A class factory that can create instances of OpenVPNProvider
 * This is returned in the exported function DllGetClassObject
 * We use a static instance.
 */

/* ClassFactory methods we have to implement */
static ULONG WINAPI AddRef(IClassFactory *this);

static ULONG WINAPI Release(IClassFactory *this);

static HRESULT WINAPI QueryInterface(IClassFactory *this, REFIID riid, void **ppv);

static HRESULT WINAPI CreateInstance(IClassFactory *this, IUnknown *iunk, REFIID riid, void **ppv);

static HRESULT WINAPI LockServer(IClassFactory *this, BOOL lock);

/* template object for creation */
static IClassFactoryVtbl cf_vtbl = {
    .QueryInterface = QueryInterface,
    .AddRef = AddRef,
    .Release = Release,
    .CreateInstance = CreateInstance,
    .LockServer = LockServer,
};

static IClassFactory cf = { .lpVtbl = &cf_vtbl };

/* Implementation */
static ULONG WINAPI
AddRef(UNUSED IClassFactory *this)
{
    dll_addref();
    return 1; /* we have just 1 static object */
}

static ULONG WINAPI
Release(UNUSED IClassFactory *this)
{
    dll_release();
    return 1;
}

static HRESULT WINAPI
QueryInterface(IClassFactory *this, REFIID riid, void **ppv)
{
    HRESULT hr;
    dmsg(L"Entry");

#ifdef DEBUG
    debug_print_guid(riid, L"In CF_Queryinterface with iid = ");
#endif

    if (ppv != NULL && riid)
    {
        if (IsEqualIID(&IID_IClassFactory, riid) || IsEqualIID(&IID_IUnknown, riid))
        {
            *ppv = this;
            ADDREF(this);
            hr = S_OK;
        }
        else
        {
            *ppv = NULL;
            hr = E_NOINTERFACE;
        }
    }
    else
    {
        hr = E_POINTER;
    }
    return hr;
}

/* Create an instance of OpenVPNProvider and return it in *ppv */
static HRESULT WINAPI
CreateInstance(UNUSED IClassFactory *this, IUnknown *iunk, REFIID riid, void **ppv)
{
    HRESULT hr;
    dmsg(L"Entry");

#ifdef DEBUG
    debug_print_guid(riid, L"In Provider CreateInstance with iid = ");
#endif

    *ppv = NULL;
    if (!iunk)
    {
        hr = OpenVPNProvider_CreateInstance(riid, ppv);
    }
    else
    {
        hr = CLASS_E_NOAGGREGATION;
    }
    return hr;
}

/* Windows calls this to keep the server in memory:
 * we just increment dll-ref-count so that the dll will not get unloaded.
 */
static HRESULT WINAPI
LockServer(UNUSED IClassFactory *this, BOOL lock)
{
    if (lock)
    {
        dll_addref();
    }
    else
    {
        dll_release();
    }
    return S_OK;
}

/* exported methods */
STDAPI
DllCanUnloadNow()
{
    HRESULT hr;

    dmsg(L"ref_count = %lu", dll_ref_count);
    if (dll_ref_count > 0)
    {
        hr = S_FALSE;
    }
    else
    {
        hr = S_OK;
    }

    return hr;
}

/* Windows calls this on loading the dll and expects a
 * ClassFactory instance in *ppv which can be used to
 * create class of type rclsid. We support only OpenVPNProvider
 * class and check it here.
 */
STDAPI
DllGetClassObject(REFCLSID rclsid, REFIID riid, void **ppv)
{
    dmsg(L"Entry");
    HRESULT hr;

    *ppv = NULL;
    if (IsEqualIID(&CLSID_OpenVPNProvider, rclsid))
    {
        ADDREF(&cf); /* though we are reusing a static instance, follow the usual COM semantics */
        hr = QUERY_INTERFACE((IClassFactory *)&cf, riid, ppv);
        RELEASE((IClassFactory *)&cf);
    }
    else
    {
        hr = CLASS_E_CLASSNOTAVAILABLE;
    }
    return hr;
}
