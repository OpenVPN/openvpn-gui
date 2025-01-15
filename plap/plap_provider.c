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

#include "plap_common.h"
#include "plap_connection.h"
#include "plap_dll.h"

#define MAX_PROFILES 100 /* a large enough number */

/*
 * OpenVPNProvider for PLAP: a "class" derived from the base "class"
 * ICredentialProvider. In C this is the interface vtable followed
 * by our derived class members.
 */
typedef struct OpenVPNProvider
{
    const ICredentialProviderVtbl *lpVtbl; /* base interface vtable */

    BOOL ui_initialized;
    ULONG conn_count;
    OpenVPNConnection *connections[MAX_PROFILES];

    LONG ref_count;
} OpenVPNProvider;

/* methods we have to implement */
static HRESULT WINAPI QueryInterface(ICredentialProvider *this, REFIID riid, void **ppv);

static ULONG WINAPI AddRef(ICredentialProvider *this);

static ULONG WINAPI Release(ICredentialProvider *this);

static HRESULT WINAPI SetUsageScenario(ICredentialProvider *this,
                                       CREDENTIAL_PROVIDER_USAGE_SCENARIO us,
                                       DWORD flags);

static HRESULT WINAPI SetSerialization(ICredentialProvider *this,
                                       const CREDENTIAL_PROVIDER_CREDENTIAL_SERIALIZATION *cs);

static HRESULT WINAPI Advise(ICredentialProvider *this,
                             ICredentialProviderEvents *e,
                             UINT_PTR context);

static HRESULT WINAPI UnAdvise(ICredentialProvider *this);

static HRESULT WINAPI GetFieldDescriptorCount(ICredentialProvider *this, DWORD *count);

static HRESULT WINAPI GetFieldDescriptorAt(ICredentialProvider *this,
                                           DWORD index,
                                           CREDENTIAL_PROVIDER_FIELD_DESCRIPTOR **fd);

static HRESULT WINAPI GetCredentialCount(ICredentialProvider *this,
                                         DWORD *count,
                                         DWORD *default_cred,
                                         BOOL *autologon_default);

static HRESULT WINAPI GetCredentialAt(ICredentialProvider *this,
                                      DWORD index,
                                      ICredentialProviderCredential **c);

/* a helper function for generating our connection array */
static HRESULT CreateOVPNConnectionArray(OpenVPNProvider *op);

/* make a static object for function table */

#define M_(x) .x = x /* I hate typing */
static const ICredentialProviderVtbl icp_vtbl = { M_(QueryInterface),
                                                  M_(AddRef),
                                                  M_(Release),
                                                  M_(SetUsageScenario),
                                                  M_(SetSerialization),
                                                  M_(Advise),
                                                  M_(UnAdvise),
                                                  M_(GetFieldDescriptorCount),
                                                  M_(GetFieldDescriptorAt),
                                                  M_(GetCredentialCount),
                                                  M_(GetCredentialAt) };

#define ICCPC IConnectableCredentialProviderCredential /* save some more typing */

/* constructor and destructor */

static OpenVPNProvider *
OpenVPNProvider_new(void)
{
    dmsg(L"Entry");

    OpenVPNProvider *this = calloc(sizeof(*this), 1);

    if (this)
    {
        this->lpVtbl = &icp_vtbl;
        this->ref_count = 1; /* we free ourselves when this goes to zero */

        dll_addref();
    }

    return this;
}

static void
OpenVPNProvider_free(OpenVPNProvider *this)
{
    dmsg(L"Entry");

    for (size_t i = 0; i < this->conn_count; ++i)
    {
        if (this->connections[i])
        {
            RELEASE((ICCPC *)this->connections[i]);
        }
    }
    /* Destroy GUI threads and any associated data */
    DeleteUI();

    free(this);

    dll_release();
}

/* Standard methods in every COM object inherited from IUnknown */
static ULONG WINAPI
AddRef(ICredentialProvider *this)
{
    OpenVPNProvider *op = (OpenVPNProvider *)this;

    dmsg(L"ref_count after addref = %d", op->ref_count + 1);

    return InterlockedIncrement(&op->ref_count);
}

static ULONG WINAPI
Release(ICredentialProvider *this)
{
    OpenVPNProvider *op = (OpenVPNProvider *)this;

    ULONG count = InterlockedDecrement(&op->ref_count);

    dmsg(L"ref_count after release = %d", op->ref_count);

    if (op->ref_count == 0)
    {
        OpenVPNProvider_free(op); /* suicide -- equivalent of "delete this" */
    }
    return count;
}

/* In QueryInterface, return *ppv = pointer to the requested interface (riid),
 * if we implement the interface.
 * In our case we expect riid == IID_ICredentialProvider or IID_IUnknown
 */
static HRESULT WINAPI
QueryInterface(ICredentialProvider *this, REFIID riid, void **ppv)
{
#ifdef DEBUG
    debug_print_guid(riid, L"In Provider Queryinterface with iid = ");
#endif

    if (!ppv)
    {
        dmsg(L"ppv is NULL!");
        return E_POINTER;
    }
    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_ICredentialProvider))
    {
        *ppv = this;
        ADDREF(this);
        return S_OK;
    }
    else
    {
        dmsg(L"unknown iid ignored");
        *ppv = NULL;
        return E_NOINTERFACE;
    }
}

/*
 * SetUsageScenario returns success for supported usages -- we support
 * only PLAP.
 *
 * LogonUI calls this while initializing the provider, so we initialize
 * our internal data structs, GUI related data, enumerate profiles,
 * make a list of connections etc. This is done by calling
 * CreateOVPNConnectionArray().
 * After this, we should be ready to service calls for connection count
 * and individual connection objects.
 */
static HRESULT WINAPI
SetUsageScenario(ICredentialProvider *this,
                 CREDENTIAL_PROVIDER_USAGE_SCENARIO us,
                 UNUSED DWORD flags)
{
    /* I think flags may be ignored for PLAP */

    dmsg(L"cpus = %lu", us);

    OpenVPNProvider *op = (OpenVPNProvider *)this;

    if (us == CPUS_PLAP)
    {
        return CreateOVPNConnectionArray(op);
    }
    else
    {
        return E_NOTIMPL;
    }
}

/*
 * We do not support SetSerialization, nor expect it
 */
static HRESULT WINAPI
SetSerialization(UNUSED ICredentialProvider *this,
                 UNUSED const CREDENTIAL_PROVIDER_CREDENTIAL_SERIALIZATION *cs)
{
    dmsg(L"Entry");
    return E_NOTIMPL;
}

/*
 * called by LogonUI to pass in events ptr -- we ignore this
 */
static HRESULT WINAPI
Advise(UNUSED ICredentialProvider *this, UNUSED ICredentialProviderEvents *e, UNUSED UINT_PTR ctx)
{
    dmsg(L"Entry");
    return S_OK;
}

/*
 * Called by logonUI when the events callback is no longer valid.
 */
static HRESULT WINAPI
UnAdvise(UNUSED ICredentialProvider *this)
{
    dmsg(L"Entry");
    return S_OK;
}

/*
 * Return the count of descriptors for each connection tile.
 * These descriptors are used by LogonUI to display the tile to the user.
 * We have a fixed static set for all tiles -- field_desc[] defined
 * in the header for OpenVPNConnection.
 */
static HRESULT WINAPI
GetFieldDescriptorCount(UNUSED ICredentialProvider *this, DWORD *count)
{
    dmsg(L"Entry");

    *count = _countof(field_desc);
    return S_OK;
}

/*
 * Return the field descriptor for a particular field.
 * We have a fixed set of fields to show, but have to return
 * an allocated copy in *fd. Must use CoTaskMemAlloc and related
 * methods as the caller will use CoTaskMemFree to release memory.
 */
static HRESULT WINAPI
GetFieldDescriptorAt(UNUSED ICredentialProvider *this,
                     DWORD index,
                     CREDENTIAL_PROVIDER_FIELD_DESCRIPTOR **fd)
{
    HRESULT hr = E_OUTOFMEMORY;

    dmsg(L"index = %lu", index);

    if (index < _countof(field_desc) && fd)
    {
        /* LogonUI frees this using CoTaskMemFree, so we should not use malloc */
        CREDENTIAL_PROVIDER_FIELD_DESCRIPTOR *tmp =
            (CREDENTIAL_PROVIDER_FIELD_DESCRIPTOR *)CoTaskMemAlloc(
                sizeof(CREDENTIAL_PROVIDER_FIELD_DESCRIPTOR));
        if (tmp)
        {
            /* call our copy helper for deep copy */
            hr = CopyFieldDescriptor(tmp, &field_desc[index]);
            if (SUCCEEDED(hr))
            {
                *fd = tmp;
            }
            else
            {
                CoTaskMemFree(tmp);
            }
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}

/*
 * Return the number of available connections.
 * default_cred is the one that will be zoomed-in by default.
 * As per MSDN, autologon_default causes immediate call to GetSerialization
 * for the default item. We don't want this, so set no default.
 */
static HRESULT WINAPI
GetCredentialCount(ICredentialProvider *this,
                   DWORD *count,
                   DWORD *default_cred,
                   BOOL *autologon_default)
{
    OpenVPNProvider *op = (OpenVPNProvider *)this;

    *count = op->conn_count;

    *default_cred = CREDENTIAL_PROVIDER_NO_DEFAULT;
    *autologon_default = FALSE;

    dmsg(L"Returning count = %lu, no default", *count);

    return S_OK;
}

/*
 * Returns the credential at index.
 */
static HRESULT WINAPI
GetCredentialAt(ICredentialProvider *this, DWORD index, ICredentialProviderCredential **ic)
{
    HRESULT hr;

    dmsg(L"index = %lu", index);

    OpenVPNProvider *op = (OpenVPNProvider *)this;

    if (index < op->conn_count && ic)
    {
        hr = QUERY_INTERFACE((ICredentialProviderCredential *)op->connections[index],
                             &IID_ICredentialProviderCredential,
                             (void **)ic);
        /* In our case the same as *ic = op->connections[index], but the above is standard COM way
         * which checks the IID and increments ref-count as well */
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}

/*
 * Create connection objects from available config files
 */
static HRESULT
CreateOVPNConnectionArray(OpenVPNProvider *op)
{
    HRESULT hr = S_OK;

    dmsg(L"Entry");

    if (op->ui_initialized) /* Already initialized */
    {
        return hr;
    }

    /* delete previous connections if any */
    for (size_t i = 0; i < op->conn_count; i++)
    {
        RELEASE((ICCPC *)op->connections[i]);
    }
    op->conn_count = 0;

    if (InitializeUI(hinst_global) != 0) /* init GUI data structs */
    {
        return E_FAIL;
    }
    op->ui_initialized = 1;

    dmsg(L"UI initialized");

    /* Create a connection object for every config that could be prestarted */
    connection_t *c[MAX_PROFILES];
    DWORD count = FindPLAPConnections(c, _countof(c));

    for (DWORD i = 0; i < count; i++)
    {
        OpenVPNConnection *oc = OpenVPNConnection_new();

        if (oc)
        {
            /* we have to initialize OpenVPNConnection objects before use */
            hr = OVPNConnection_Initialize(oc, c[i], ConfigDisplayName(c[i]));
            if (SUCCEEDED(hr))
            {
                op->connections[op->conn_count++] = oc;
            }
            else
            {
                RELEASE((ICCPC *)oc);
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
            break;
        }
        dmsg(L"added connection object for <%ls>", ConfigDisplayName(c[i]));
    }
    return hr;
}

/* A helper function for use by DllGetClassObject in plap_dll.c
 * This is the only non-static function in this file.
 */
HRESULT
OpenVPNProvider_CreateInstance(REFIID riid, void **ppv)
{
    HRESULT hr;

#ifdef DEBUG
    debug_print_guid(riid, L"In Provider CreateInstance with iid = ");
#endif

    OpenVPNProvider *p = OpenVPNProvider_new();
    if (p)
    {
        hr = QUERY_INTERFACE((ICredentialProvider *)p, riid, ppv);
        RELEASE((ICredentialProvider *)p);
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }
    return hr;
}
