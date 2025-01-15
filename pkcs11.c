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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <windows.h>
#include "pkcs11.h"
#include "options.h"
#include "main.h"
#include "manage.h"
#include "openvpn.h"
#include "misc.h"
#include "openvpn-gui-res.h"
#include "localization.h"
#include <commctrl.h>
#include <cryptuiapi.h>
#include <shlwapi.h>

extern options_t o;
static const wchar_t *hfontProp = L"header_font";

/* state of list array */
#define STATE_GET_COUNT 1
#define STATE_GET_ENTRY 2
#define STATE_FILLED    4
#define STATE_SELECTED  8

struct cert_info
{
    wchar_t *commonname;
    wchar_t *issuer;
    wchar_t *notAfter;
    const CERT_CONTEXT *ctx;
};

struct pkcs11_entry
{
    char *id;              /* pkcs11-id string value as received from daemon */
    struct cert_info cert; /* decoded certificate structure */
};

static void
certificate_free(struct cert_info *cert)
{
    if (cert)
    {
        free(cert->commonname);
        free(cert->issuer);
        free(cert->notAfter);
        CertFreeCertificateContext(cert->ctx);
    }
}

static bool
pkcs11_list_alloc(struct pkcs11_list *l)
{
    if (l->count > 0 && !l->pe)
    {
        l->pe = calloc(l->count, sizeof(struct pkcs11_entry));
    }
    return (l->pe != NULL);
}

static void
pkcs11_entry_free(struct pkcs11_entry *pe)
{
    if (!pe)
    {
        return;
    }
    free(pe->id);
    certificate_free(&pe->cert);
}

/* Free any allocated memory and clear the list */
void
pkcs11_list_clear(struct pkcs11_list *l)
{
    if (l->pe)
    {
        for (UINT i = 0; i < l->count; i++)
        {
            pkcs11_entry_free(&l->pe[i]);
        }
        free(l->pe);
    }

    CLEAR(*l);
}

/* Get the "friendly name" (usually the commonname) from a certificate
 * context. If issuer = true, the issuer name is parsed, else the
 * subject. A newly allocated wide char string is returned.
 */
static wchar_t *
extract_name_entry(const CERT_CONTEXT *ctx, bool issuer)
{
    DWORD size = CertGetNameStringW(
        ctx, CERT_NAME_FRIENDLY_DISPLAY_TYPE, issuer ? CERT_NAME_ISSUER_FLAG : 0, NULL, NULL, 0);

    wchar_t *name = malloc(size * sizeof(wchar_t));
    if (name)
    {
        size = CertGetNameStringW(ctx,
                                  CERT_NAME_FRIENDLY_DISPLAY_TYPE,
                                  issuer ? CERT_NAME_ISSUER_FLAG : 0,
                                  NULL,
                                  name,
                                  size);
    }

    return name;
}

/* Decode a  base64 encoded certificate blob and fill in
 * the cert structure with commonname, issuer and validity.
 * Returns false on error.
 */
static bool
decode_certificate(struct cert_info *cert, const char *b64)
{
    unsigned char *der = NULL;
    bool ret = false;

    int len = Base64Decode(b64, (char **)&der);
    if (len < 0)
    {
        goto out;
    }

    const CERT_CONTEXT *ctx = CertCreateCertificateContext(X509_ASN_ENCODING, der, (DWORD)len);

    if (!ctx)
    {
        goto out;
    }
    cert->commonname = extract_name_entry(ctx, 0);
    cert->issuer = extract_name_entry(ctx, CERT_NAME_ISSUER_FLAG);
    cert->notAfter = LocalizedFileTime(&ctx->pCertInfo->NotAfter);
    cert->ctx = ctx;
    ret = true;

out:
    free(der);
    return ret;
}

/* Parse pkcs11-id message "'n', ID:'<id>', BLOB:'<cert>'"
 * and fill in data in pkcs11 enrty.
 * Returns index of the item on success, -1 on error.
 * On success, caller must free the entry after use.
 */
static UINT
pkcs11_entry_parse(const char *data, struct pkcs11_list *l)
{
    char *token = NULL;
    UINT index = (UINT)-1;
    const char *quotes = " '";
    struct pkcs11_entry *pe = NULL;

    char *p = strdup(data);

    if (!p)
    {
        goto out;
    }

    token = strtok(p, ",");
    /* parse index */
    if (token)
    {
        StrTrimA(token, quotes);
        UINT i = strtoul(token, NULL, 10);
        if (i >= l->count) /* invalid entry number */
        {
            goto out;
        }
        index = i;
    }
    pe = &l->pe[index];

    while ((token = strtok(NULL, ",")) != NULL)
    {
        char *tmp;
        if ((tmp = strstr(token, "ID:")) != NULL)
        {
            tmp += 3;
            StrTrimA(tmp, quotes);
            pe->id = strdup(tmp);
        }
        else if ((tmp = strstr(token, "BLOB:")) != NULL)
        {
            tmp += 5;
            StrTrimA(tmp, quotes);
            if (!decode_certificate(&pe->cert, tmp))
            {
                pkcs11_entry_free(pe);
                index = (UINT)-1;
                goto out;
            }
        }
    }

out:
    free(p);
    return index;
}

/* send pkcs11-id to management-interface */
static void
pkcs11_id_send(connection_t *c, const char *id)
{
    const char *format = "needstr 'pkcs11-id-request' '%s'";

    size_t len = strlen(format) + (id ? strlen(id) : 0) + 1;
    char *cmd = malloc(len);
    if (cmd)
    {
        snprintf(cmd, len, format, id ? id : "");
        cmd[len - 1] = '\0';
        ManagementCommand(c, cmd, NULL, regular);
    }
    else
    {
        WriteStatusLog(c, L"GUI> ", L"Out of memory in pkcs11_id_send", false);
        ManagementCommand(c, "needstr 'pkcs11-id-request' ''", NULL, regular);
    }
    free(cmd);
}

static INT_PTR CALLBACK QueryPkcs11DialogProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);

/* Handle Need 'pkcs11-id-request' from management */
void
OnPkcs11(connection_t *c, UNUSED char *msg)
{
    struct pkcs11_list *l = &c->pkcs11_list;

    pkcs11_list_clear(l);
    l->selected = (UINT)-1; /* set selection to an invalid index */

    /* prompt user to select a certificate */
    if (IDOK
            == LocalizedDialogBoxParamEx(
                ID_DLG_PKCS11_QUERY, c->hwndStatus, QueryPkcs11DialogProc, (LPARAM)c)
        && l->state & STATE_SELECTED && l->selected < l->count)
    {
        pkcs11_id_send(c, l->pe[l->selected].id);
    }
}

/* Callback when pkcs11-id-count is received */
static void
pkcs11_count_recv(connection_t *c, char *msg)
{
    struct pkcs11_list *l = &c->pkcs11_list;
    if (msg && strbegins(msg, ">PKCS11ID-COUNT:"))
    {
        l->count = strtoul(msg + 16, NULL, 10);
    }
    else
    {
        WriteStatusLog(c, L"GUI> ", L"Invalid pkcs11-id-count ignored", false);
        l->state &= ~STATE_GET_COUNT;
    }
    if (l->count == 0)
    {
        l->state |= STATE_FILLED;
    }
}

/*
 * Callback for receiving pkcs11 entry from daemon.
 * Expect msg = >PKCS11ID-ENTRY:'index', ID:'<id>', BLOB:'<cert>'
 */
static void
pkcs11_entry_recv(connection_t *c, char *msg)
{
    struct pkcs11_list *l = &c->pkcs11_list;
    UINT index = (UINT)-1;

    if (msg && strbegins(msg, ">PKCS11ID-ENTRY:"))
    {
        index = pkcs11_entry_parse(msg + 16, l);
    }

    if (index == (UINT)-1)
    {
        WriteStatusLog(c, L"GUI> ", L"Invalid pkcs11 entry ignored.", false);
        return;
    }
    else if (index + 1 == l->count) /* done */
    {
        l->state |= STATE_FILLED;
    }
}

/*
 *  Helper to populate the pkcs11 list by querying the daemon
 *
 *  The requests are queued and completed asynchronously.
 *  We return immediately if already in progress or
 *  not yet ready to populate. This is called by
 *  pkcs11_listview_fill until state & STATE_FILLED evaluates to 1.
 *
 *  To recreate the list afresh, call pkcs11_list_clear() first.
 */
static void
pkcs11_list_update(connection_t *c)
{
    struct pkcs11_list *l = &c->pkcs11_list;

    if ((l->state & STATE_GET_COUNT) == 0)
    {
        ManagementCommand(c, "pkcs11-id-count", pkcs11_count_recv, regular);
        l->state |= STATE_GET_COUNT;
    }
    else if (l->count > 0 && (l->state & STATE_GET_ENTRY) == 0)
    {
        if (!l->pe && !pkcs11_list_alloc(l))
        {
            WriteStatusLog(c, L"GUI> ", L"Out of memory for pkcs11 entry list", false);
            l->count = 0;
            l->state |= STATE_FILLED;
            return;
        }
        /* required space = strlen("pkcs11-id-get ") + 10 + 1 = 25 */
        char cmd[25];
        for (UINT i = 0; i < l->count; i++)
        {
            _snprintf_0(cmd, "pkcs11-id-get %u", i);
            ManagementCommand(c, cmd, pkcs11_entry_recv, regular);
        }
        l->state |= STATE_GET_ENTRY;
    }
}

static void
listview_set_column_width(HWND lv)
{
    for (int i = 0; i < 3; i++)
    {
        /* MSDN docs on this is not clear, but using AUTOSIZE_USEHEADER
         * instead of AUTOSIZE ensures the column is wide enough for
         * both header and content strings -- must be set again if/when
         * content or header strings are modified. */
        ListView_SetColumnWidth(lv, i, LVSCW_AUTOSIZE_USEHEADER);
    }
}

/*
 * Position widgets in pkcs11 list window using current dpi.
 * Takes client area width and height in screen pixels as input.
 */
static void
pkcs11_listview_resize(HWND hwnd, UINT w, UINT h)
{
    HWND lv = GetDlgItem(hwnd, ID_LVW_PKCS11);

    MoveWindow(lv, DPI_SCALE(20), DPI_SCALE(25), w - DPI_SCALE(40), h - DPI_SCALE(120), TRUE);
    MoveWindow(GetDlgItem(hwnd, ID_TXT_PKCS11),
               DPI_SCALE(20),
               DPI_SCALE(5),
               w - DPI_SCALE(30),
               DPI_SCALE(15),
               TRUE);
    MoveWindow(GetDlgItem(hwnd, ID_TXT_WARNING),
               DPI_SCALE(20),
               h - DPI_SCALE(80),
               w - DPI_SCALE(20),
               DPI_SCALE(30),
               TRUE);
    MoveWindow(GetDlgItem(hwnd, IDOK),
               DPI_SCALE(20),
               h - DPI_SCALE(30),
               DPI_SCALE(60),
               DPI_SCALE(23),
               TRUE);
    MoveWindow(GetDlgItem(hwnd, IDCANCEL),
               DPI_SCALE(90),
               h - DPI_SCALE(30),
               DPI_SCALE(60),
               DPI_SCALE(23),
               TRUE);
    MoveWindow(GetDlgItem(hwnd, IDRETRY),
               DPI_SCALE(200),
               h - DPI_SCALE(30),
               DPI_SCALE(60),
               DPI_SCALE(23),
               TRUE);

    listview_set_column_width(lv);
}

/* initialize the listview widget for displaying pkcs11 entries */
static HWND
pkcs11_listview_init(HWND parent)
{
    HWND lv;
    RECT rc;

    lv = GetDlgItem(parent, ID_LVW_PKCS11);
    if (!lv)
    {
        return NULL;
    }

    SendMessage(lv, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);

    /* Use bold font for header */
    HFONT hf = (HFONT)SendMessage(lv, WM_GETFONT, 0, 0);
    if (hf)
    {
        LOGFONT lf;
        GetObject(hf, sizeof(LOGFONT), &lf);
        lf.lfWeight = FW_BOLD;

        HFONT hfb = CreateFontIndirect(&lf);
        if (hfb && SetPropW(parent, hfontProp, (HANDLE)hfb))
        {
            SendMessage(ListView_GetHeader(lv), WM_SETFONT, (WPARAM)hfb, 1);
        }
        else if (hfb)
        {
            DeleteObject(hfb);
        }
    }

    /* Add column headings */
    int hdrs[] = { IDS_CERT_DISPLAYNAME, IDS_CERT_ISSUER, IDS_CERT_NOTAFTER };

    LVCOLUMNW lvc;
    lvc.mask = LVCF_TEXT | LVCF_SUBITEM;

    for (int i = 0; i < 3; i++)
    {
        lvc.iSubItem = i;
        lvc.pszText = LoadLocalizedString(hdrs[i]);
        ListView_InsertColumn(lv, i, &lvc);
    }

    GetClientRect(parent, &rc);
    pkcs11_listview_resize(parent, rc.right - rc.left, rc.bottom - rc.top);

    EnableWindow(lv, FALSE); /* disable until filled in */
    EnableWindow(GetDlgItem(parent, IDOK), FALSE);
    EnableWindow(GetDlgItem(parent, IDRETRY), FALSE);

    return lv;
}

/* Populate the pkcs11 list and listview widget. Unless the state
 * of the list evaluates to STATE_FILLED, a callback to this
 * is requeued. Meant to be used as a Timer callback.
 */
static void CALLBACK
pkcs11_listview_fill(HWND hwnd, UINT UNUSED msg, UINT_PTR id, DWORD UNUSED now)
{
    connection_t *c = (connection_t *)GetProp(hwnd, cfgProp);
    struct pkcs11_list *l = &c->pkcs11_list;

    HWND lv = GetDlgItem(hwnd, ID_LVW_PKCS11);

    LVITEMW lvi = { 0 };
    lvi.mask = LVIF_TEXT | LVIF_PARAM;

    if ((l->state & STATE_FILLED) == 0)
    {
        /* request list update and set a timer to call this routine again */
        pkcs11_list_update(c);
        SetTimer(hwnd, id, 100, pkcs11_listview_fill);
    }
    else
    {
        int pos;
        for (UINT i = 0; i < l->count; i++)
        {
            lvi.iItem = i;
            lvi.iSubItem = 0;
            lvi.pszText = l->pe[i].cert.commonname;
            lvi.lParam = (LPARAM)i;
            pos = ListView_InsertItem(lv, &lvi);

            ListView_SetItemText(lv, pos, 1, l->pe[i].cert.issuer);
            ListView_SetItemText(lv, pos, 2, l->pe[i].cert.notAfter);
        }

        if (l->count == 0)
        {
            /* no certificates -- show a message and let user retry */
            SetDlgItemTextW(hwnd, ID_TXT_WARNING, LoadLocalizedString(IDS_ERR_NO_PKCS11));
            EnableWindow(GetDlgItem(hwnd, IDRETRY), TRUE);
        }
        else
        {
            EnableWindow(lv, TRUE);
            SetFocus(lv);
            EnableWindow(GetDlgItem(hwnd, IDOK), TRUE);
            EnableWindow(GetDlgItem(hwnd, IDRETRY), TRUE);
            SendMessage(GetDlgItem(hwnd, IDOK), BM_SETSTYLE, BS_DEFPUSHBUTTON, TRUE);
            SendMessage(GetDlgItem(hwnd, IDCANCEL), BM_SETSTYLE, BS_PUSHBUTTON, TRUE);
            SendMessage(GetDlgItem(hwnd, IDRETRY), BM_SETSTYLE, BS_PUSHBUTTON, TRUE);
        }

        /* if there is only one item, select it by default */
        if (l->count == 1)
        {
            ListView_SetItemState(lv, pos, LVIS_SELECTED, LVIS_SELECTED);
        }
        listview_set_column_width(lv);

        KillTimer(hwnd, id);
    }
}

/* Reset the listview, clear the list and initiate a fresh scan
 * without closing the dialog.
 */
static void
pkcs11_listview_reset(HWND parent)
{
    connection_t *c = (connection_t *)GetProp(parent, cfgProp);
    struct pkcs11_list *l = &c->pkcs11_list;
    HWND lv = GetDlgItem(parent, ID_LVW_PKCS11);

    /* ensure the list is not being built and the widget exists */
    if ((l->state & STATE_FILLED) == 0 || !lv)
    {
        return;
    }

    /* clear the list and listview */
    pkcs11_list_clear(l);

    EnableWindow(lv, FALSE);
    EnableWindow(GetDlgItem(parent, IDOK), FALSE);
    EnableWindow(GetDlgItem(parent, IDRETRY), FALSE);

    ListView_DeleteAllItems(lv);
    SetDlgItemTextW(parent, ID_TXT_WARNING, L"");

    /* initiate a rebuild of the list */
    SetTimer(parent, 0, 100, pkcs11_listview_fill);
}

void
display_certificate(HWND parent, connection_t *c, UINT i)
{
    struct pkcs11_list *l = &c->pkcs11_list;
    if (i < l->count)
    {
/* Currently cryptui.lib is missing in mingw for i686
 * Remove this and corresponding check in configure.ac
 * when that changes.
 */
#if defined(HAVE_LIBCRYPTUI) || defined(_MSC_VER)
        CryptUIDlgViewContext(
            CERT_STORE_CERTIFICATE_CONTEXT, l->pe[i].cert.ctx, parent, L"Certificate", 0, NULL);
#else
        (void)i;
        (void)parent;
        WriteStatusLog(c, L"GUI> ", L"Certificate display not supported in this build", false);
#endif
    }
}

/* Dialog proc for querying pkcs11 */
static INT_PTR CALLBACK
QueryPkcs11DialogProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    connection_t *c;

    switch (msg)
    {
        case WM_INITDIALOG:
            c = (connection_t *)lParam;
            TRY_SETPROP(hwndDlg, cfgProp, (HANDLE)lParam);
            SetStatusWinIcon(hwndDlg, ID_ICO_APP);

            /* init the listview and schedule a call to listview_fill */
            if (pkcs11_listview_init(hwndDlg))
            {
                SetTimer(hwndDlg, 0, 50, pkcs11_listview_fill);
            }
            else
            {
                WriteStatusLog(c, L"GUI> ", L"Error initializing pkcs11 selection dialog.", false);
                EndDialog(hwndDlg, wParam);
            }
            return TRUE;

        case WM_COMMAND:
            c = (connection_t *)GetProp(hwndDlg, cfgProp);
            if (LOWORD(wParam) == IDOK)
            {
                HWND lv = GetDlgItem(hwndDlg, ID_LVW_PKCS11);
                int id = (int)ListView_GetNextItem(lv, -1, LVNI_ALL | LVNI_SELECTED);
                LVITEM lvi = { .iItem = id, .mask = LVIF_PARAM };
                if (id >= 0 && ListView_GetItem(lv, &lvi))
                {
                    c->pkcs11_list.selected = (UINT)lvi.lParam;
                    c->pkcs11_list.state |= STATE_SELECTED;
                }
                else if (c->pkcs11_list.count > 0)
                {
                    /* No selection -- show an error message */
                    SetDlgItemTextW(
                        hwndDlg, ID_TXT_WARNING, LoadLocalizedString(IDS_ERR_SELECT_PKCS11));
                    return TRUE;
                }
                EndDialog(hwndDlg, wParam);
                return TRUE;
            }
            else if (LOWORD(wParam) == IDCANCEL)
            {
                StopOpenVPN(c);
                EndDialog(hwndDlg, wParam);
                return TRUE;
            }
            else if (LOWORD(wParam) == IDRETRY)
            {
                pkcs11_listview_reset(hwndDlg);
                return TRUE;
            }
            break;

        case WM_OVPN_STATE: /* state changed -- destroy the dialog */
            EndDialog(hwndDlg, LOWORD(wParam));
            return TRUE;

        case WM_CTLCOLORSTATIC:
            if (GetDlgCtrlID((HWND)lParam) == ID_TXT_WARNING)
            {
                HBRUSH br = (HBRUSH)DefWindowProc(hwndDlg, msg, wParam, lParam);
                COLORREF clr = o.clr_warning;
                SetTextColor((HDC)wParam, clr);
                return (INT_PTR)br;
            }
            break;

        case WM_SIZE:
            pkcs11_listview_resize(hwndDlg, LOWORD(lParam), HIWORD(lParam));
            InvalidateRect(hwndDlg, NULL, TRUE);
            return FALSE;

        case WM_NOTIFY:
            c = (connection_t *)GetProp(hwndDlg, cfgProp);
            if (((NMHDR *)lParam)->idFrom == ID_LVW_PKCS11)
            {
                NMITEMACTIVATE *ln = (NMITEMACTIVATE *)lParam;
                if (ln->iItem >= 0 && ln->uNewState & LVNI_SELECTED)
                {
                    /* remove the no-selection warning */
                    SetDlgItemTextW(hwndDlg, ID_TXT_WARNING, L"");
                }
                if (ln->hdr.code == NM_DBLCLK && ln->iItem >= 0)
                {
                    display_certificate(hwndDlg, c, (UINT)ln->iItem);
                }
            }
            break;

        case WM_CLOSE:
            c = (connection_t *)GetProp(hwndDlg, cfgProp);
            StopOpenVPN(c);
            EndDialog(hwndDlg, wParam);
            return TRUE;

        case WM_NCDESTROY:
            RemoveProp(hwndDlg, cfgProp);
            HFONT hf = (HFONT)GetProp(hwndDlg, hfontProp);
            if (hf)
            {
                DeleteObject(hf);
            }
            RemoveProp(hwndDlg, hfontProp);
            break;
    }
    return FALSE;
}
