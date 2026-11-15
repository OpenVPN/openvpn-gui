/*
 *  OpenVPN-GUI -- A Windows GUI for OpenVPN.
 *
 *  Copyright (C) 2021-2026 Selva Nair <selva.nair@gmail.com>
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
#include "remote.h"
#include "main.h"
#include "options.h"
#include "manage.h"
#include "openvpn.h"
#include "misc.h"
#include "openvpn-gui-res.h"
#include "localization.h"
#include <commctrl.h>
#include <errno.h>

extern options_t o;

static const wchar_t *hfontProp = L"header_font";
static const wchar_t *remoteDialogProp = L"remote_dialog";

#define MIN_REQ_MGMT_VERSION 4
#define MIN(a, b)            ((a) < (b) ? (a) : (b))

/* state of list array */
#define STATE_GET_COUNT      1
#define STATE_GET_ENTRY      2
#define STATE_READY          4
#define STATE_SELECTED       8
#define STATE_FAILED         16
#define STATE_COUNT_READY    32
#define STATE_PARTIAL        64
#define REMOTE_LIST_TIMER    2 /* AutoCloseSetup uses timer 1. */

struct remote_dialog
{
    connection_t *c;
    bool interacted; /* User interacted with the dialog */
    bool countdown;  /* Auto submit count down active */
};

struct remote_entry
{
    wchar_t *host;
    wchar_t *port;
    wchar_t *proto;
    char *data;   /* "host,port,protocol", matching the >REMOTE notification */
    bool enabled; /* status of remote from remote-entry-get */
};

/* forward declarations */
static bool remote_entry_parse(const char *data, struct remote_entry *re);
static UINT remote_entry_find(struct remote_list *rl, const char *str, UINT start);
static INT_PTR CALLBACK QueryRemoteDialogProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
static void CALLBACK remote_listview_fill(HWND hwnd,
                                          UINT UNUSED msg,
                                          UINT_PTR id,
                                          DWORD UNUSED now);
static HWND remote_listview_init(HWND parent);
static void remote_listview_resize(HWND hwnd, UINT w, UINT h);

/* Accept current remote entry */
static void
remote_accept(connection_t *c)
{
    ManagementCommand(c, "remote ACCEPT", NULL, regular);
}

/* Skip multiple remotes entries */
static void
remote_skip(connection_t *c, UINT toskip)
{
    char cmd[sizeof("remote SKIP %u") + 10];

    _snprintf_0(cmd, "remote SKIP %u", toskip);
    ManagementCommand(c, cmd, NULL, regular);
}

static bool
remote_list_alloc(connection_t *c)
{
    struct remote_list *rl = &c->remote_list;
    if (rl->count > 0 && !rl->re)
    {
        rl->re = calloc(rl->count, sizeof(remote_entry_t));
    }
    return (rl->re != NULL);
}

static void
remote_entry_free(remote_entry_t *re)
{
    free(re->host);
    free(re->port);
    free(re->proto);
    free(re->data);
    CLEAR(*re);
}

/* Free any allocated memory and clear the remote list */
void
remote_list_clear(struct remote_list *rl)
{
    if (rl->re)
    {
        for (UINT i = 0; i < rl->count; i++)
        {
            remote_entry_free(&rl->re[i]);
        }
        free(rl->re);
    }

    CLEAR(*rl);
}

/* Handle remote-query from management
 * Format: host,port,protocol
 */
void
OnRemote(connection_t *c, char *msg)
{
    struct remote_list *rl = &c->remote_list;

    /* if not supported, just accept the remote and move on */
    if (c->management_version < MIN_REQ_MGMT_VERSION)
    {
        WriteStatusLog(
            c, L"GUI> ", L"Querying remote not supported. Accepting proposed value.", false);
        remote_accept(c);
        return;
    }

    /* Dynamic CR causes remote to be prompted again -- just accept the current value */
    if (c->dynamic_cr)
    {
        remote_accept(c);
        return;
    }
    else if (!(rl->state & STATE_SELECTED)) /* no valid selection */
    {
        if (rl->state & STATE_FAILED)
        {
            remote_list_clear(rl);
        }
        /* cache the proposed remote, and query user */
        strncpy_s(rl->current, sizeof(rl->current), msg, _TRUNCATE);
        rl->selected = (UINT)-1; /* set selection to an invalid index */

        struct remote_dialog dialog = { .c = c };
        INT_PTR result = LocalizedDialogBoxParamEx(
            ID_DLG_REMOTE_QUERY, c->hwndStatus, QueryRemoteDialogProc, (LPARAM)&dialog);
        if (result == -1)
        {
            MsgToEventLog(EVENTLOG_ERROR_TYPE,
                          L"Unable to open remote selection dialog. Stopping connection.");
            StopOpenVPN(c);
            return;
        }
        if (result != IDOK)
        {
            return;
        }
    }

    UINT n = rl->count;
    UINT i = rl->selected;
    if (!(rl->state & STATE_SELECTED) || i >= n || !rl->re || !rl->re[i].data || !rl->re[i].enabled
        || strcmp(rl->re[i].data, msg) == 0)
    {
        /* invalid selection or proposed value matches selection  -- accept current */
        remote_accept(c);
        rl->state &= ~STATE_SELECTED;
    }
    else
    {
        if (rl->state & STATE_PARTIAL)
        {
            /* Missing entries make skip calculation inaccurate -- skip one by one and cycle */
            remote_skip(c, 1);
            return;
        }
        UINT current = remote_entry_find(rl, msg, 0);
        UINT toskip = (UINT)-1;
        /* if there are duplicates, we are not sure of the current
         * position in the daemon. Choose the smallest possible value
         * for skip. This may take a few round trips to get to the
         * selected point.
         * Would be useful to have >REMOTE extended to include the index.
         */
        while (current < n)
        {
            toskip = MIN(toskip, (i + n - current) % n);
            current = remote_entry_find(rl, msg, current + 1);
        }
        remote_skip(c, toskip < n ? toskip : 1);
    }
}

/* Stop list retrieval on error; the dialog will require a reconnect. */
static void
remote_list_failed(connection_t *c)
{
    if (!(c->remote_list.state & STATE_FAILED))
    {
        MsgToEventLog(EVENTLOG_ERROR_TYPE, L"Unable to retrieve the complete remote list.");
    }
    c->remote_list.state |= STATE_FAILED;
}

/* Parse an unsigned decimal count or index without accepting signs or overflow. */
static bool
remote_uint_parse(const char *msg, char **end, UINT *value)
{
    if (!msg || *msg < '0' || *msg > '9')
    {
        return false;
    }
    errno = 0;
    unsigned long n = strtoul(msg, end, 10);
    if (errno == ERANGE)
    {
        return false;
    }
    *value = (UINT)n;
    return true;
}

/* Unlike >PKCS11ID-COUNT, the remote count is an ordinary decimal line. */
static void
remote_count_recv(connection_t *c, char *msg)
{
    struct remote_list *rl = &c->remote_list;
    char *end;
    UINT count;
    if (rl->state & STATE_FAILED)
    {
        return;
    }
    if (msg && strcmp(msg, "END") == 0)
    {
        if (!(rl->state & STATE_COUNT_READY) || rl->count == 0)
        {
            remote_list_failed(c);
        }
        return;
    }
    if (!remote_uint_parse(msg, &end, &count) || *end || (rl->state & STATE_COUNT_READY))
    {
        remote_list_failed(c);
        return;
    }
    rl->count = count;
    rl->state |= STATE_COUNT_READY;
}

/* Ordinary multiline response: i,host,port,proto,enabled|disabled then END.
 * PKCS#11 instead delivers one >PKCS11ID-ENTRY notification per command.
 */
static void
remote_entry_recv(connection_t *c, char *msg)
{
    struct remote_list *rl = &c->remote_list;
    char *p;
    UINT i;

    if (rl->state & STATE_FAILED)
    {
        return;
    }
    if (!msg || strcmp(msg, "END") == 0)
    {
        if (!msg || rl->received != rl->count)
        {
            rl->state |= STATE_PARTIAL;
            WriteStatusLog(c, L"GUI> ", L"Incomplete remote list received.", false);
        }
        rl->state |= STATE_READY;
        return;
    }
    if (!remote_uint_parse(msg, &p, &i) || *p != ',' || i >= rl->count || !rl->re || rl->re[i].data
        || !remote_entry_parse(p + 1, &rl->re[i]))
    {
        rl->state |= STATE_PARTIAL;
        WriteStatusLog(c, L"GUI> ", L"Invalid remote entry ignored.", false);
        return;
    }
    rl->received++;
}

/* Comparison function for sorting entries in remote list.
 * i and j are the original indices of the list and
 * we return (i - j) if rl->show_sorted is false.
 * This may be used to restore the original ordering.
 * Otherwise the ordering is that of the hostname in the
 * entry.
 */
static int CALLBACK
remote_entry_compare(LPARAM first, LPARAM second, LPARAM context)
{
    UINT i = (UINT)first;
    UINT j = (UINT)second;
    const struct remote_list *rl = (const struct remote_list *)context;
    int res = (i > j) - (i < j);

    if (rl->show_sorted && i < rl->count && j < rl->count)
    {
        res = wcscmp(rl->re[i].host, rl->re[j].host);
    }
    return res;
}

/* Queue count and list requests asynchronously, as for PKCS#11.
 * The remote list is ready after END or ERROR, even if some entries are missing.
 */
static void
remote_list_update(connection_t *c)
{
    struct remote_list *rl = &c->remote_list;

    if (rl->state & (STATE_READY | STATE_FAILED))
    {
        return;
    }
    if (!(rl->state & STATE_GET_COUNT))
    {
        if (ManagementCommand(c, "remote-entry-count", remote_count_recv, multiline))
        {
            rl->state |= STATE_GET_COUNT;
        }
        else
        {
            remote_list_failed(c);
        }
    }
    else if (rl->count > 0 && !(rl->state & STATE_GET_ENTRY))
    {
        if (remote_list_alloc(c)
            && ManagementCommand(c, "remote-entry-get all", remote_entry_recv, multiline))
        {
            rl->state |= STATE_GET_ENTRY;
        }
        else
        {
            remote_list_failed(c);
        }
    }
}

/* Report retrieval/UI failure explicitly rather than accepting another server. */
static void
remote_dialog_failed(HWND hwnd, connection_t *c)
{
    KillTimer(hwnd, REMOTE_LIST_TIMER);
    EndDialog(hwnd, IDCANCEL);
    ShowLocalizedMsgEx(MB_OK | MB_ICONERROR, c->hwndStatus, L"OpenVPN GUI", IDS_ERR_REMOTE_LIST);
    StopOpenVPN(c);
}

/* Remember input during list loading as well as during the countdown. */
static void
remote_cancel_auto_submit(HWND hwnd)
{
    struct remote_dialog *dialog = (struct remote_dialog *)GetPropW(hwnd, remoteDialogProp);
    if (dialog)
    {
        dialog->interacted = true;
        dialog->countdown = false;
    }
    AutoCloseCancel(hwnd);
}

/* Dialog proc for querying remote */
static INT_PTR CALLBACK
QueryRemoteDialogProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    connection_t *c;

    switch (msg)
    {
        case WM_INITDIALOG:
            c = ((struct remote_dialog *)lParam)->c;
            TRY_SETPROP(hwndDlg, cfgProp, (HANDLE)c);
            TRY_SETPROP(hwndDlg, remoteDialogProp, (HANDLE)lParam);
            SetStatusWinIcon(hwndDlg, ID_ICO_APP);

            if (remote_listview_init(hwndDlg))
            {
                /* Delay the first call to listview_fill to after
                 * we return from here.
                 */
                if (!SetTimer(hwndDlg, REMOTE_LIST_TIMER, 100, remote_listview_fill))
                {
                    remote_dialog_failed(hwndDlg, c);
                }
            }
            else
            {
                MsgToEventLog(EVENTLOG_ERROR_TYPE, L"Error initializing remote selection dialog.");
                remote_dialog_failed(hwndDlg, c);
            }
            return TRUE;

        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_NCLBUTTONDOWN:
        case WM_NCRBUTTONDOWN:
            remote_cancel_auto_submit(hwndDlg);
            break;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK)
            {
                TRY_GETPROP(hwndDlg, cfgProp, c, FALSE);
                HWND lv = GetDlgItem(hwndDlg, ID_LVW_REMOTE);
                int id = (int)ListView_GetNextItem(lv, -1, LVNI_ALL | LVNI_SELECTED);
                LVITEM lvi = { .iItem = id, .mask = LVIF_PARAM };
                if (id >= 0 && ListView_GetItem(lv, &lvi) && (c->remote_list.state & STATE_READY)
                    && (UINT)lvi.lParam < c->remote_list.count
                    && c->remote_list.re[lvi.lParam].enabled)
                {
                    c->remote_list.selected = (UINT)lvi.lParam;
                    c->remote_list.state |= STATE_SELECTED;
                }
                else
                {
                    return TRUE;
                }
                EndDialog(hwndDlg, wParam);
                return TRUE;
            }
            else if (LOWORD(wParam) == IDCANCEL)
            {
                EndDialog(hwndDlg, wParam);
                TRY_GETPROP(hwndDlg, cfgProp, c, TRUE);
                StopOpenVPN(c);
                return TRUE;
            }
            break;

        case WM_NOTIFY:
            if (((LPNMHDR)lParam)->idFrom != ID_LVW_REMOTE)
            {
                break;
            }
            switch (((LPNMHDR)lParam)->code)
            {
                case NM_CLICK:
                case NM_RCLICK:
                case LVN_KEYDOWN:
                    remote_cancel_auto_submit(hwndDlg);
                    break;

                case LVN_BEGINSCROLL:
                {
                    struct remote_dialog *dialog;
                    TRY_GETPROP(hwndDlg, remoteDialogProp, dialog, FALSE);
                    /* Initial EnsureVisible runs before the countdown is armed. */
                    if (dialog->countdown)
                    {
                        remote_cancel_auto_submit(hwndDlg);
                    }
                    break;
                }

                case LVN_COLUMNCLICK:
                    remote_cancel_auto_submit(hwndDlg);
                    TRY_GETPROP(hwndDlg, cfgProp, c, FALSE);
                    /* Toggle sorting between original and alphabetical order. */
                    HWND lv = GetDlgItem(hwndDlg, ID_LVW_REMOTE);
                    c->remote_list.show_sorted = !c->remote_list.show_sorted;
                    ListView_SortItems(lv, remote_entry_compare, (LPARAM)&c->remote_list);
                    return TRUE;
            }
            break;

        case WM_OVPN_STATE:
            EndDialog(hwndDlg, IDCANCEL);
            return TRUE;

        case WM_SIZE:
            remote_listview_resize(hwndDlg, LOWORD(lParam), HIWORD(lParam));
            InvalidateRect(hwndDlg, NULL, TRUE);
            return FALSE;

        case WM_CTLCOLORSTATIC:
            if (GetDlgCtrlID((HWND)lParam) == ID_TXT_WARNING)
            {
                HBRUSH br = (HBRUSH)DefWindowProc(hwndDlg, msg, wParam, lParam);
                struct remote_dialog *dialog;
                TRY_GETPROP(hwndDlg, remoteDialogProp, dialog, FALSE);
                SetTextColor((HDC)wParam,
                             dialog->countdown ? GetSysColor(COLOR_WINDOWTEXT) : o.clr_warning);
                return (INT_PTR)br;
            }
            break;

        case WM_CLOSE:
            EndDialog(hwndDlg, IDCANCEL);
            TRY_GETPROP(hwndDlg, cfgProp, c, TRUE);
            StopOpenVPN(c);
            return TRUE;

        case WM_NCDESTROY:
            AutoCloseCancel(hwndDlg);
            KillTimer(hwndDlg, REMOTE_LIST_TIMER);
            RemovePropW(hwndDlg, remoteDialogProp);
            RemovePropW(hwndDlg, cfgProp);
            HFONT hf = (HFONT)GetPropW(hwndDlg, hfontProp);
            if (hf)
            {
                DeleteObject(hf);
            }
            RemovePropW(hwndDlg, hfontProp);
            break;
    }
    return FALSE;
}

/* Find index of remote entry matching a string
 * The search starts at index=start. Use start = 0 to search
 * the whole list. Returns -1 if not found.
 */
static UINT
remote_entry_find(struct remote_list *rl, const char *str, UINT start)
{
    UINT res = (UINT)-1;

    if (!rl->re)
    {
        return res;
    }

    for (UINT i = start; i < rl->count; i++)
    {
        if (rl->re[i].data && strcmp(rl->re[i].data, str) == 0)
        {
            res = i;
            break;
        }
    }
    return res;
}

/* Parse host,port,protocol,status. Store status separately so data matches
 * the three-field >REMOTE notification. Replace an entry only on success.
 */
static bool
remote_entry_parse(const char *data, struct remote_entry *re)
{
    struct remote_entry entry = { 0 };
    char *p = strdup(data);
    bool res = false;
    if (!p)
    {
        return false;
    }

    char *status = strrchr(p, ',');
    if (!status || (strcmp(status + 1, "enabled") && strcmp(status + 1, "disabled")))
    {
        goto out;
    }
    entry.enabled = strcmp(status + 1, "enabled") == 0;
    *status = '\0';
    entry.data = strdup(p);

    char *port = strchr(p, ',');
    if (!port)
    {
        goto out;
    }
    *port++ = '\0';
    char *proto = strchr(port, ',');
    if (!proto)
    {
        goto out;
    }
    *proto++ = '\0';
    if (!*p || !*port || !*proto || strchr(proto, ','))
    {
        goto out;
    }
    entry.host = Widen(p);
    entry.port = Widen(port);
    entry.proto = Widen(proto);
    if (!entry.host || !entry.port || !entry.proto || !entry.data)
    {
        goto out;
    }
    remote_entry_free(re);
    *re = entry;
    CLEAR(entry);
    res = true;
out:
    remote_entry_free(&entry);
    free(p);
    return res;
}

/* As in the PKCS#11 list, size columns after filling and resizing the view.
 * AUTOSIZE alone can clip headers; USEHEADER also fills the last column.
 */
static void
listview_set_column_width(HWND lv)
{
    for (int i = 0; i < 3; i++)
    {
        ListView_SetColumnWidth(lv, i, LVSCW_AUTOSIZE_USEHEADER);
    }
}

/*
 * Position widgets in remote list window using current dpi.
 * Takes client area width and height in screen pixels as input.
 */
static void
remote_listview_resize(HWND hwnd, UINT w, UINT h)
{
    MoveWindow(GetDlgItem(hwnd, ID_LVW_REMOTE),
               DPI_SCALE(20),
               DPI_SCALE(25),
               w - DPI_SCALE(40),
               h - DPI_SCALE(100),
               TRUE);
    MoveWindow(GetDlgItem(hwnd, ID_TXT_WARNING),
               DPI_SCALE(20),
               h - DPI_SCALE(65),
               w - DPI_SCALE(40),
               DPI_SCALE(30),
               TRUE);
    MoveWindow(GetDlgItem(hwnd, ID_TXT_REMOTE),
               DPI_SCALE(20),
               DPI_SCALE(5),
               w - DPI_SCALE(30),
               DPI_SCALE(15),
               TRUE);
    MoveWindow(GetDlgItem(hwnd, IDOK),
               DPI_SCALE(20),
               h - DPI_SCALE(30),
               DPI_SCALE(110),
               DPI_SCALE(23),
               TRUE);
    MoveWindow(GetDlgItem(hwnd, IDCANCEL),
               DPI_SCALE(145),
               h - DPI_SCALE(30),
               DPI_SCALE(110),
               DPI_SCALE(23),
               TRUE);
    listview_set_column_width(GetDlgItem(hwnd, ID_LVW_REMOTE));
}

static HWND
remote_listview_init(HWND parent)
{
    HWND lv;
    RECT rc;

    lv = GetDlgItem(parent, ID_LVW_REMOTE);
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
    int hdrs[] = { IDS_HOSTNAME, IDS_PORT, IDS_PROTOCOL };

    LVCOLUMNW lvc = { 0 };
    lvc.mask = LVCF_FMT | LVCF_TEXT | LVCF_SUBITEM;
    lvc.fmt = LVCFMT_LEFT;

    for (size_t i = 0; i < _countof(hdrs); i++)
    {
        lvc.iSubItem = i;
        lvc.pszText = LoadLocalizedString(hdrs[i]);
        ListView_InsertColumn(lv, i, &lvc);
    }

    GetClientRect(parent, &rc);
    remote_listview_resize(parent, rc.right - rc.left, rc.bottom - rc.top);

    EnableWindow(lv, FALSE); /* disable until filled in */
    EnableWindow(GetDlgItem(parent, IDOK), FALSE);

    return lv;
}

static void CALLBACK
remote_listview_fill(HWND hwnd, UINT UNUSED msg, UINT_PTR id, DWORD UNUSED now)
{
    connection_t *c;
    TRY_GETPROP_VOID(hwnd, cfgProp, c);
    struct remote_list *rl = &c->remote_list;

    HWND lv = GetDlgItem(hwnd, ID_LVW_REMOTE);

    LVITEMW lvi = { 0 };
    lvi.mask = LVIF_TEXT | LVIF_PARAM;

    if (rl->state & STATE_FAILED)
    {
        remote_dialog_failed(hwnd, c);
        return;
    }
    if (rl->state & STATE_READY)
    {
        int pos;
        int rows = 0;
        UINT selected = (UINT)-1;
        for (UINT i = 0; i < rl->count; i++)
        {
            if (!rl->re[i].enabled)
            {
                continue;
            }
            lvi.iItem = rows;
            lvi.iSubItem = 0;
            lvi.pszText = rl->re[i].host;
            lvi.lParam = (LPARAM)i;
            pos = ListView_InsertItem(lv, &lvi);
            if (pos < 0)
            {
                remote_list_failed(c);
                remote_dialog_failed(hwnd, c);
                return;
            }
            rows++;

            if (strcmp(rl->current, rl->re[i].data) == 0)
            {
                selected = (UINT)pos;
            }

            ListView_SetItemText(lv, pos, 1, rl->re[i].port);
            ListView_SetItemText(lv, pos, 2, rl->re[i].proto);
        }
        if (selected >= (UINT)rows && !(rl->state & STATE_PARTIAL))
        {
            /* A complete list must contain the daemon's proposed remote. */
            remote_list_failed(c);
            remote_dialog_failed(hwnd, c);
            return;
        }
        if (rows == 0)
        {
            SetDlgItemTextW(hwnd, ID_TXT_WARNING, LoadLocalizedString(IDS_REMOTE_LIST_EMPTY));
            KillTimer(hwnd, id);
            return;
        }
        if (rl->state & STATE_PARTIAL)
        {
            SetDlgItemTextW(hwnd, ID_TXT_WARNING, LoadLocalizedString(IDS_REMOTE_LIST_PARTIAL));
        }
        EnableWindow(lv, TRUE);
        EnableWindow(GetDlgItem(hwnd, IDOK), TRUE);
        if (selected >= (UINT)rows)
        {
            selected = 0;
        }
        if (selected < (UINT)rows)
        {
            ListView_EnsureVisible(lv, selected, false);
            ListView_SetItemState(lv, selected, LVIS_SELECTED, LVIS_SELECTED);
        }
        ListView_SortItems(lv, remote_entry_compare, (LPARAM)&c->remote_list);
        listview_set_column_width(lv);
        SetFocus(lv);
        KillTimer(hwnd, id);
        struct remote_dialog *dialog = (struct remote_dialog *)GetPropW(hwnd, remoteDialogProp);
        if (dialog && !dialog->interacted && !(rl->state & STATE_PARTIAL))
        {
            dialog->countdown = true;
            AutoCloseSetup(hwnd, IDOK, 6, ID_TXT_WARNING, IDS_NFO_AUTO_REMOTE);
        }
    }
    else                       /* request list update and set a timer to call this routine again */
    {
        remote_list_update(c); /* this returns immediately if list building is in progress */
        /* The existing window timer repeats until the list is ready. */
    }
}
