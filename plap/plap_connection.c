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
#include <assert.h>
#include "resource.h"
#include "localization.h"
#include "openvpn-gui-res.h"
#include "main.h"
#include "misc.h"

/* A "class" that implements IConnectableCredentialProviderCredential */

struct OpenVPNConnection
{
    const IConnectableCredentialProviderCredentialVtbl *lpVtbl; /* base class vtable */

    /* "private" members */
    ICredentialProviderCredentialEvents *events; /* Passed in by Logon UI for callbacks */
    connection_t *c;                             /* GUI connection data */
    const wchar_t *display_name;
    IQueryContinueWithStatus *qc; /* Passed in by LogonUI for checking status of connect */
    BOOL connect_cancelled;       /* we set this true if user cancels the connect operation */
    LONG ref_count;
};

#define ICCPC             IConnectableCredentialProviderCredential /* shorthand for a long name */

#define ISCONNECTED(c)    (ConnectionState(c) == state_connected)
#define ISDISCONNECTED(c) (ConnectionState(c) == state_disconnected)
#define ISONHOLD(c)       (ConnectionState(c) == state_onhold)

extern DWORD status_menu_id;

/* Methods in IConnectableCredentialProviderCredential that we need to define */

/* IUnknown */
static HRESULT WINAPI QueryInterface(ICCPC *this, REFIID riid, void **ppv);

static ULONG WINAPI AddRef(ICCPC *this);

static ULONG WINAPI Release(ICCPC *this);

/* ICredentialProviderCredential */
static HRESULT WINAPI Advise(ICCPC *this, ICredentialProviderCredentialEvents *e);

static HRESULT WINAPI UnAdvise(ICCPC *this);

static HRESULT WINAPI SetSelected(ICCPC *this, BOOL *auto_logon);

static HRESULT WINAPI SetDeselected(ICCPC *this);

static HRESULT WINAPI GetFieldState(ICCPC *this,
                                    DWORD field,
                                    CREDENTIAL_PROVIDER_FIELD_STATE *fs,
                                    CREDENTIAL_PROVIDER_FIELD_INTERACTIVE_STATE *fis);

static HRESULT WINAPI GetStringValue(ICCPC *this, DWORD index, WCHAR **ws);

static HRESULT WINAPI GetBitmapValue(ICCPC *this, DWORD field, HBITMAP *bmp);

static HRESULT WINAPI GetSubmitButtonValue(ICCPC *this, DWORD field, DWORD *adjacent);

static HRESULT WINAPI SetStringValue(ICCPC *this, DWORD field, const WCHAR *ws);

static HRESULT WINAPI GetCheckboxValue(ICCPC *this, DWORD field, BOOL *checked, wchar_t **label);

static HRESULT WINAPI GetComboBoxValueCount(ICCPC *this,
                                            DWORD field,
                                            DWORD *items,
                                            DWORD *selected_item);

static HRESULT WINAPI GetComboBoxValueAt(ICCPC *this,
                                         DWORD field,
                                         DWORD item,
                                         wchar_t **item_value);

static HRESULT WINAPI SetCheckboxValue(ICCPC *this, DWORD field, BOOL checked);

static HRESULT WINAPI SetComboBoxSelectedValue(ICCPC *this, DWORD field, DWORD selected_item);

static HRESULT WINAPI CommandLinkClicked(ICCPC *this, DWORD field);

static HRESULT WINAPI GetSerialization(ICCPC *this,
                                       CREDENTIAL_PROVIDER_GET_SERIALIZATION_RESPONSE *response,
                                       CREDENTIAL_PROVIDER_CREDENTIAL_SERIALIZATION *cs,
                                       wchar_t **text,
                                       CREDENTIAL_PROVIDER_STATUS_ICON *icon);

static HRESULT WINAPI ReportResult(ICCPC *this,
                                   NTSTATUS status,
                                   NTSTATUS substatus,
                                   wchar_t **status_text,
                                   CREDENTIAL_PROVIDER_STATUS_ICON *icon);

/* IConnectableCredentialProviderCredential */
static HRESULT WINAPI Connect(ICCPC *this, IQueryContinueWithStatus *qc);

static HRESULT WINAPI Disconnect(ICCPC *this);

/* use a static table for filling in the methods */
#define M_(x) .x = x
static const IConnectableCredentialProviderCredentialVtbl iccpc_vtbl = {
    M_(QueryInterface),
    M_(AddRef),
    M_(Release),
    M_(Advise),
    M_(UnAdvise),
    M_(SetSelected),
    M_(SetDeselected),
    M_(GetFieldState),
    M_(GetStringValue),
    M_(GetBitmapValue),
    M_(GetCheckboxValue),
    M_(GetSubmitButtonValue),
    M_(GetComboBoxValueCount),
    M_(GetComboBoxValueAt),
    M_(SetStringValue),
    M_(SetCheckboxValue),
    M_(SetComboBoxSelectedValue),
    M_(CommandLinkClicked),
    M_(GetSerialization),
    M_(ReportResult),
    M_(Connect),
    M_(Disconnect),
};

/* constructor */
OpenVPNConnection *
OpenVPNConnection_new()
{
    dmsg(L"Entry");

    OpenVPNConnection *oc = calloc(sizeof(*oc), 1);
    if (oc)
    {
        oc->lpVtbl = &iccpc_vtbl;
        oc->ref_count = 1;

        dll_addref();
    }
    return oc;
}

/* destructor */
static void
OpenVPNConnection_free(OpenVPNConnection *oc)
{
    dmsg(L"Entry: profile: <%ls>", oc->display_name);

    free(oc);
    dll_release();
}

static ULONG WINAPI
AddRef(ICCPC *this)
{
    OpenVPNConnection *oc = (OpenVPNConnection *)this;

    dmsg(L"Connection: ref_count after increment = %lu", oc->ref_count + 1);
    return InterlockedIncrement(&oc->ref_count);
}

static ULONG WINAPI
Release(ICCPC *this)
{
    OpenVPNConnection *oc = (OpenVPNConnection *)this;

    int count = InterlockedDecrement(&oc->ref_count);
    dmsg(L"Connection: ref_count after decrement = %lu", count);
    if (count == 0)
    {
        OpenVPNConnection_free(oc); /* delete self */
    }
    return count;
}

static HRESULT WINAPI
QueryInterface(ICCPC *this, REFIID riid, void **ppv)
{
    HRESULT hr;

#ifdef DEBUG
    debug_print_guid(riid, L"In OVPN Connection QueryInterface with riid = ");
#endif

    if (ppv != NULL)
    {
        if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_ICredentialProviderCredential)
            || IsEqualIID(riid, &IID_IConnectableCredentialProviderCredential))
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

/*
 * LogonUI calls Advise first and the passed in events may be used for
 * making callbacks to notify changes asynchronously
 */
static HRESULT WINAPI
Advise(ICCPC *this, ICredentialProviderCredentialEvents *e)
{
    HWND hwnd;

    dmsg(L"Entry");

    OpenVPNConnection *oc = (OpenVPNConnection *)this;

    if (oc->events)
    {
        RELEASE(oc->events);
    }
    oc->events = e;
    ADDREF(e);

    if (e && e->lpVtbl->OnCreatingWindow(e, &hwnd) == S_OK)
    {
        dmsg(L"Setting hwnd");
        SetParentWindow(hwnd);
    }
    return S_OK;
}

/* We keep a copy of events pointer in Advise, release it here */
static HRESULT WINAPI
UnAdvise(ICCPC *this)
{
    dmsg(L"Entry");

    OpenVPNConnection *oc = (OpenVPNConnection *)this;

    if (oc->events)
    {
        RELEASE(oc->events);
    }
    oc->events = NULL;
    return S_OK;
}

/*
 * LogonUI calls this function when our tile is selected
 * We do nothing here and let the user choose which profile to connect.
 */
static HRESULT WINAPI
SetSelected(ICCPC *this, BOOL *auto_logon)
{
#ifdef DEBUG
    OpenVPNConnection *oc = (OpenVPNConnection *)this;
    dmsg(L"profile: %ls", oc->display_name);
#else
    (void)this;
#endif

    /* setting true here will autoconnect the first entry and Windows will
     * autoselect it for the rest of the login session, blocking all other
     * entries and other providers, if any. Some commercial products do
     * that but its not a good idea, IMO. So we set false here.
     */
    *auto_logon = FALSE;
    return S_OK;
}

static HRESULT WINAPI
SetDeselected(ICCPC *this)
{
#ifdef DEBUG
    OpenVPNConnection *oc = (OpenVPNConnection *)this;
    dmsg(L"profile: %ls", oc->display_name);
#else
    (void)this;
#endif

    return S_OK;
}

/*
 * Return display states for a particular field of a tile
 */
static HRESULT WINAPI
GetFieldState(UNUSED ICCPC *this,
              DWORD field,
              CREDENTIAL_PROVIDER_FIELD_STATE *fs,
              CREDENTIAL_PROVIDER_FIELD_INTERACTIVE_STATE *fis)
{
    HRESULT hr;

    dmsg(L"field = %lu", field);

    if (field < _countof(field_states))
    {
        if (fs)
        {
            *fs = field_states[field];
        }
        if (fis)
        {
            *fis = (field_desc[field].cpft == CPFT_SUBMIT_BUTTON) ? CPFIS_FOCUSED : CPFIS_NONE;
        }
        hr = S_OK;
    }
    else
    {
        hr = E_INVALIDARG;
    }
    return hr;
}

/*
 * Return the string value of the field at the index
 */
static HRESULT WINAPI
GetStringValue(ICCPC *this, DWORD index, WCHAR **ws)
{
    HRESULT hr = S_OK;

    dmsg(L"index = %lu", index);

    OpenVPNConnection *oc = (OpenVPNConnection *)this;

    const wchar_t *val = L"";

    if (index >= _countof(field_desc))
    {
        return E_INVALIDARG;
    }

    /* we have custom field strings only for connection name */
    if (field_desc[index].cpft == CPFT_LARGE_TEXT)
    {
        val = oc->display_name; /* connection name */
    }

    /* do not to use malloc here as the client will free using CoTaskMemFree */
    hr = SHStrDupW(val, ws);
    if (!SUCCEEDED(hr))
    {
        hr = E_OUTOFMEMORY;
        dmsg(L"OOM while copying field_strings[%lu]", index);
    }
    else
    {
        dmsg(L"Returning %ls", *ws);
    }

    return hr;
}

/*
 * return the image for the tile at index field
 */
static HRESULT WINAPI
GetBitmapValue(UNUSED ICCPC *this, DWORD field, HBITMAP *bmp)
{
    HRESULT hr = S_OK;

    dmsg(L"field = %lu ", field);

    if (field_desc[field].cpft == CPFT_TILE_IMAGE && bmp)
    {
        HBITMAP tmp = LoadBitmapW(hinst_global, MAKEINTRESOURCE(IDB_TILE_IMAGE));
        if (tmp)
        {
            *bmp = tmp; /* The caller takes ownership of this memory */
            dmsg(L"Returning a bitmap for PLAP tile %lu", field);
        }
        else
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            dmsg(L"LoadBitmap failed with error = 0x%08x", hr);
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}

/*
 * Return the index of the field the button should be placed adjacent to.
 */
static HRESULT WINAPI
GetSubmitButtonValue(UNUSED ICCPC *this, DWORD field, DWORD *adjacent)
{
    HRESULT hr;

    dmsg(L"field = %lu", field);

    /* we have defined field ids in order of appearance */
    if (field_desc[field].cpft == CPFT_SUBMIT_BUTTON && field > 0 && adjacent)
    {
        *adjacent = field - 1;
        hr = S_OK;
    }
    else
    {
        hr = E_INVALIDARG;
    }
    return hr;
}

/*
 * Set the value of a field which can accept user text input
 * This gets called as user types into a field.
 */
static HRESULT WINAPI
SetStringValue(UNUSED ICCPC *this, UNUSED DWORD field, UNUSED const WCHAR *ws)
{
    HRESULT hr;

    dmsg(L"field = %lu", field);

    /* We do not allow setting any fields -- at least not yet */
    hr = E_INVALIDARG;

    return hr;
}

static HRESULT WINAPI
GetCheckboxValue(UNUSED ICCPC *this,
                 UNUSED DWORD field,
                 UNUSED BOOL *checked,
                 UNUSED wchar_t **label)
{
    dmsg(L"Entry");
    return E_NOTIMPL;
}

static HRESULT WINAPI
GetComboBoxValueCount(UNUSED ICCPC *this,
                      UNUSED DWORD field,
                      UNUSED DWORD *items,
                      UNUSED DWORD *selected_item)
{
    dmsg(L"Entry");
    return E_NOTIMPL;
}

static HRESULT WINAPI
GetComboBoxValueAt(UNUSED ICCPC *this,
                   UNUSED DWORD field,
                   UNUSED DWORD item,
                   UNUSED wchar_t **item_value)
{
    dmsg(L"Entry");
    return E_NOTIMPL;
}

static HRESULT WINAPI
SetCheckboxValue(UNUSED ICCPC *this, UNUSED DWORD field, UNUSED BOOL checked)
{
    dmsg(L"Entry");
    return E_NOTIMPL;
}

static HRESULT WINAPI
SetComboBoxSelectedValue(UNUSED ICCPC *this, UNUSED DWORD field, UNUSED DWORD selected_item)
{
    dmsg(L"Entry");
    return E_NOTIMPL;
}

static HRESULT WINAPI
CommandLinkClicked(UNUSED ICCPC *this, UNUSED DWORD field)
{
    dmsg(L"Entry");
    return E_NOTIMPL;
}

/*
 * Collect the username and password into a serialized credential for the usage scenario
 * This gets called soon after connect -- result returned here influences the status of
 * Connect.
 * We do not support SSO, so no credentials are serialized. We just indicate whether
 * we got correct credentials for the Connect process or not.
 */
static HRESULT WINAPI
GetSerialization(ICCPC *this,
                 CREDENTIAL_PROVIDER_GET_SERIALIZATION_RESPONSE *response,
                 UNUSED CREDENTIAL_PROVIDER_CREDENTIAL_SERIALIZATION *cs,
                 wchar_t **text,
                 CREDENTIAL_PROVIDER_STATUS_ICON *icon)
{
    HRESULT hr = S_OK;

    OpenVPNConnection *oc = (OpenVPNConnection *)this;
    dmsg(L"Entry: profile <%ls>", oc->display_name);

    if (oc->connect_cancelled || !ISCONNECTED(oc->c))
    {
        dmsg(L"connect cancelled or failed");
        /* return _NOT_FINISHED response here for the prompt go
         * back to the PLAP tiles screen. Unclear the return value
         * should be success or not : using S_OK.
         */
        *response = CPGSR_NO_CREDENTIAL_NOT_FINISHED;

        if (text && icon)
        {
            if (oc->connect_cancelled)
            {
                hr = SHStrDupW(LoadLocalizedString(IDS_NFO_CONN_CANCELLED), text);
            }
            else
            {
                hr = SHStrDupW(LoadLocalizedString(IDS_NFO_CONN_FAILED, oc->display_name), text);
            }

            *icon = CPSI_ERROR;
        }
    }
    else
    {
        /* Finished but no credentials to pass on to LogonUI */
        *response = CPGSR_NO_CREDENTIAL_FINISHED;
        /* return CPGSR_RETURN_NO_CREDENTIAL_FINISHED leads to a loop! */
        if (text && icon)
        {
            hr = SHStrDupW(LoadLocalizedString(IDS_NFO_OVPN_STATE_CONNECTED), text);
            *icon = CPSI_SUCCESS;
        }
    }
    return hr;
}

static HRESULT WINAPI
ReportResult(UNUSED ICCPC *this,
             UNUSED NTSTATUS status,
             UNUSED NTSTATUS substatus,
             UNUSED wchar_t **status_text,
             UNUSED CREDENTIAL_PROVIDER_STATUS_ICON *icon)
{
    dmsg(L"Entry");
    return E_NOTIMPL;
}

/* Helper to send status string feedback to events object and qc if available */
static void
NotifyEvents(OpenVPNConnection *oc, const wchar_t *status)
{
    if (oc->events)
    {
        oc->events->lpVtbl->SetFieldString(
            oc->events, (ICredentialProviderCredential *)oc, STATUS_FIELD_INDEX, status);
    }
    if (oc->qc)
    {
        oc->qc->lpVtbl->SetStatusMessage(oc->qc, status);
    }
}

static HRESULT WINAPI
ProgressCallback(HWND hwnd, UINT msg, WPARAM wParam, UNUSED LPARAM lParam, LONG_PTR ref_data)
{
    assert(ref_data);

    OpenVPNConnection *oc = (OpenVPNConnection *)ref_data;
    connection_t *c = oc->c;
    IQueryContinueWithStatus *qc = oc->qc;

    assert(qc);

    HRESULT hr = S_FALSE;
    WCHAR status[256] = L"";

    if (msg == TDN_BUTTON_CLICKED && wParam == IDCANCEL)
    {
        dmsg(L"wParam=IDCANCEL -- returning S_OK");
        hr = S_OK; /* this will cause the progress dialog to close */
    }
    else if (ISCONNECTED(c) || ISDISCONNECTED(c) || (qc->lpVtbl->QueryContinue(qc) != S_OK))
    {
        /* this will trigger IDCANCEL */
        PostMessageW(hwnd, WM_CLOSE, 0, 0);
        dmsg(L"profile = %ls, closing progress dialog", oc->display_name);
        hr = S_OK;
    }
    else if (ISONHOLD(c)) /* Try to connect again */
    {
        ConnectHelper(c);
    }
    else if (!ISCONNECTED(c) && msg == TDN_BUTTON_CLICKED && wParam == status_menu_id)
    {
        dmsg(L"wParam = status_menu_id  -- showing status window");
        ShowStatusWindow(c, TRUE);
    }
    else if (!ISCONNECTED(c) && msg == TDN_BUTTON_CLICKED && wParam == IDRETRY)
    {
        dmsg(L"wParam = IDRETRY -- closing progress dialog for restart");
        hr = S_OK;
    }

    if (msg == TDN_CREATED)
    {
        dmsg(L"starting progress bar marquee");
        PostMessageW(hwnd, TDM_SET_PROGRESS_BAR_MARQUEE, 1, 0);
    }

    if (msg == TDN_TIMER)
    {
        GetConnectionStatusText(c, status, _countof(status));
        NotifyEvents(oc, status);
        SendMessageW(hwnd, TDM_UPDATE_ELEMENT_TEXT, TDE_CONTENT, (LPARAM)status);
        dmsg(L"Connection status <%ls>", status);
    }

    return hr;
}

static HRESULT WINAPI
Connect(ICCPC *this, IQueryContinueWithStatus *qc)
{
    OpenVPNConnection *oc = (OpenVPNConnection *)this;
    wchar_t status[256] = L"";

    dmsg(L"profile: <%ls>", oc->display_name);

    oc->qc = qc;
    NotifyEvents(oc, L"");

again:
    oc->connect_cancelled = FALSE;
    SetActiveProfile(oc->c); /* This enables UI dialogs to be shown for c */

    ConnectHelper(oc->c);

    OVPNMsgWait(100, NULL);

    /* if not immediately connected, show a progress dialog with
     * service state changes and retry/cancel options. Progress dialog
     * returns on error, cancel or when connected.
     */
    if (!ISCONNECTED(oc->c) && !ISDISCONNECTED(oc->c))
    {
        dmsg(L"Runninng progress dialog");
        int res = RunProgressDialog(oc->c, ProgressCallback, (LONG_PTR)oc);
        dmsg(L"Out of progress dialog with res = %d", res);

        if (res == IDRETRY && !ISCONNECTED(oc->c))
        {
            LoadLocalizedStringBuf(status, _countof(status), IDS_NFO_STATE_RETRYING);
            NotifyEvents(oc, status);

            DisconnectHelper(oc->c);
            goto again;
        }
        else if (res == IDCANCEL && !ISCONNECTED(oc->c) && !ISDISCONNECTED(oc->c))
        {
            LoadLocalizedStringBuf(status, _countof(status), IDS_NFO_STATE_CANCELLING);
            NotifyEvents(oc, status);

            DisconnectHelper(oc->c);
            oc->connect_cancelled = TRUE;
        }
    }

    GetConnectionStatusText(oc->c, status, _countof(status));
    NotifyEvents(oc, status);

    ShowStatusWindow(oc->c, FALSE);
    oc->qc = NULL;
    SetActiveProfile(NULL);

    dmsg(L"Exit with: <%ls>", ISCONNECTED(oc->c) ? L"success" : L"error/cancel");

    return ISCONNECTED(oc->c) ? S_OK : E_FAIL;
}

static HRESULT WINAPI
Disconnect(ICCPC *this)
{
    OpenVPNConnection *oc = (OpenVPNConnection *)this;
    dmsg(L"profile <%ls>", oc->display_name);

    NotifyEvents(oc, LoadLocalizedString(IDS_NFO_STATE_DISCONNECTING));

    DisconnectHelper(oc->c);

    NotifyEvents(oc, L""); /* clear status text */

    return S_OK;
}

HRESULT WINAPI
OVPNConnection_Initialize(OpenVPNConnection *this, connection_t *conn, const wchar_t *display_name)
{
    HRESULT hr = S_OK;

    this->c = conn;
    this->display_name = display_name;
    dmsg(L"profile <%ls>", display_name);

    return hr;
}

/* Copy field descriptor -- caller will free using CoTaskMemFree, alloc using compatible allocator
 */
HRESULT
CopyFieldDescriptor(CREDENTIAL_PROVIDER_FIELD_DESCRIPTOR *fd_out,
                    const CREDENTIAL_PROVIDER_FIELD_DESCRIPTOR *fd_in)
{
    HRESULT hr = S_OK;

    memcpy(fd_out, fd_in, sizeof(CREDENTIAL_PROVIDER_FIELD_DESCRIPTOR));
    /* now copy the label string if present */
    if (fd_in->pszLabel)
    {
        hr = SHStrDupW(fd_in->pszLabel, &fd_out->pszLabel);
    }

    return hr;
}
