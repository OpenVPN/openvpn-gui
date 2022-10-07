/*
 *  OpenVPN-PLAP-Provider
 *
 *  Copyright (C) 2019-2022 Selva Nair <selva.nair@gmail.com>
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

#undef WIN32_LEAN_AND_MEAN

#include <initguid.h>
#include "plap_common.h"
#include <windows.h>
#include <credentialprovider.h>
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <assert.h>
#include <time.h>

#define error_return(fname, hr) do { fwprintf(stdout, L"Error in %ls: status = 0x%08x", fname, hr);\
                                      return 1;} while (0)
typedef HRESULT (WINAPI * f_func)(REFCLSID rclsid, REFIID riid, LPVOID* ppv);

class MyQueryContinue : public IQueryContinueWithStatus
{
    public:
    STDMETHODIMP_(ULONG) AddRef() {
       return InterlockedIncrement(&ref_count);
    }
    STDMETHODIMP_(ULONG) Release() {
        int count = InterlockedDecrement(&ref_count);
        if (ref_count == 0) delete this;
        return count;
    }
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv) { return E_FAIL; }
    STDMETHODIMP QueryContinue() {return time(NULL) > timeout ? S_FALSE : S_OK;}
    STDMETHODIMP SetStatusMessage(const wchar_t *ws) { wprintf(L"%ls\r", ws); return S_OK; }
    MyQueryContinue() : ref_count(1) {};
    time_t timeout;

    private:
       ~MyQueryContinue()=default;
       LONG ref_count;
};

static int test_provider(IClassFactory *cf)
{
    assert(cf != NULL);
    ICredentialProvider *o = NULL;
    HRESULT hr;

    hr = cf->CreateInstance(NULL, IID_ICredentialProvider, (void**) &o);

    if (!SUCCEEDED(hr)) error_return(L"IID_ICredentialProvider", hr);

    hr = o->SetUsageScenario(CPUS_PLAP, 0);
    if (!SUCCEEDED(hr)) error_return(L"SetUsageScenario", hr);

    DWORD count, def;
    BOOL auto_def;
    hr = o->GetCredentialCount(&count, &def, &auto_def);
    if (!SUCCEEDED(hr)) error_return(L"GetCredentialCount", hr);

    fwprintf(stdout, L"credential count = %lu, default = %d, autologon = %d\n", count, (int) def, auto_def);
    if (count < 1) fwprintf(stdout, L"No persistent configs found!\n");

    ICredentialProviderCredential *c = NULL;
    MyQueryContinue *qc = new MyQueryContinue();
    for (DWORD i = 0; i < count; i++)
    {
        hr = o->GetCredentialAt(i, &c);
        if (!SUCCEEDED(hr)) error_return(L"GetCredentialAt", hr);

        fwprintf(stdout, L"credential # = %lu: ", i);
        wchar_t *ws;

        for (DWORD j = 0; j < 4; j++)
        {
            hr = c->GetStringValue(j, &ws);
            if (!SUCCEEDED(hr)) error_return(L"GetStringValue", hr);
            CoTaskMemFree(ws);
        }

        /* test getbitmap */
        HBITMAP bmp;
        hr = c->GetBitmapValue(0, &bmp);
        if (!SUCCEEDED(hr))
            fwprintf(stdout, L"Warning: could not get bitmap"); /* not fatal */
        else
            DeleteObject(bmp);

        /* set a time out so that we can move to next config in case */
        qc->timeout = time(NULL) + 20;

        /* get a connection instance and call connect on it */
        IConnectableCredentialProviderCredential *c1 = NULL;
        hr = c->QueryInterface(IID_IConnectableCredentialProviderCredential, (void**)&c1);

        fwprintf(stdout, L"\nConnecting connection # <%lu>\n", i);
        c1->Connect(qc); /* this will return when connected/failed or qc timesout */

        fwprintf(stdout, L"\nsleep for 2 sec\n");
        Sleep(2000);
        c1->Release();
    }

    assert(o->Release() == 0); /* check refcount */
    assert(qc->Release() == 0);
    return 0;
}

int wmain()
{
    HRESULT hr;
    _setmode(_fileno(stdout), _O_U16TEXT);
    _setmode(_fileno(stderr), _O_U16TEXT);

    IClassFactory *cf = NULL;
    DWORD ctx = CLSCTX_INPROC_SERVER;
    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (!SUCCEEDED(hr)) error_return(L"CoIntialize", hr);

    /* Test by loading the dll */
    fwprintf(stdout, L"Test plap dll direct loading\n");

    HMODULE lib = LoadLibraryW(L"C:\\Program Files\\OpenVPN\\bin\\libopenvpn_plap.dll");
    f_func func = NULL;
    if (lib == NULL)
    {
        fwprintf(stderr, L"Failed to load the dll: error = 0x%08x\n", GetLastError());
    }
    else {
        func = (f_func) GetProcAddress(lib, "DllGetClassObject");
        if (!func)
            fwprintf(stderr, L"Failed to find DllGetClassObject in dll: error = 0x%08x\n", GetLastError());
    }
    if (func) {
        hr = func(CLSID_OpenVPNProvider, IID_IClassFactory, (void **)&cf);
        if (!SUCCEEDED(hr)) fwprintf(stdout, L"Error in DllGetClassObject: status = 0x%08x\n", hr);
        else {
            fwprintf(stdout, L"Success: found ovpn provider class factory by direct access\n");
            cf->Release();
        }
    }

    /* Test by finding the class through COM's registration mechanism */
    fwprintf(stdout, L"Testing plap using CoGetclassobject -- requires proper dll registration\n");

    hr = CoGetClassObject(CLSID_OpenVPNProvider, ctx, NULL, IID_IClassFactory, (void **)&cf);
    if (SUCCEEDED(hr)) {
        test_provider(cf);
        cf->Release();
    }
    else {
        fwprintf(stdout, L"CoGetClassObject (class not registered?): error = 0x%08x\n", hr);
    }

    CoUninitialize();
    if (lib) {
        FreeLibrary(lib);
    }
    return 0;
}
