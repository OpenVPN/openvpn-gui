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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <windows.h>
#include <wchar.h>
#include <richedit.h>
#include "main.h"
#include "options.h"
#include "misc.h"
#include "openvpn.h"
#include "echo.h"
#include "tray.h"
#include "openvpn-gui-res.h"
#include "localization.h"
#include "registry.h"

extern options_t o;

/* echo msg types */
#define ECHO_MSG_WINDOW (1)
#define ECHO_MSG_NOTIFY (2)

/* Old text in the window is deleted when content grows beyond this many lines */
#define MAX_MSG_LINES 1000

struct echo_msg_history {
    struct echo_msg_fp fp;
    struct echo_msg_history *next;
};

/* We use a global message window for all messages
 */
static HWND echo_msg_window;

/* Forward declarations */
static void
AddMessageBoxText(HWND hwnd, const wchar_t *text, const wchar_t *title, const wchar_t *from);

static INT_PTR CALLBACK
MessageDialogFunc(HWND hwnd, UINT msg, UNUSED WPARAM wParam, LPARAM lParam);

void
echo_msg_init(void)
{
    echo_msg_window = CreateLocalizedDialogParam(ID_DLG_MESSAGE, MessageDialogFunc, (LPARAM) 0);

    if (!echo_msg_window)
    {
        MsgToEventLog(EVENTLOG_ERROR_TYPE, L"Error creating echo message window.");
    }
}

/* compute a digest of the message and add it to the msg struct */
static void
echo_msg_add_fp(struct echo_msg *msg, time_t timestamp)
{
    md_ctx ctx;

    msg->fp.timestamp = timestamp;
    if (md_init(&ctx, CALG_SHA1) != 0)
        return;
    md_update(&ctx, (BYTE*) msg->text, msg->txtlen*sizeof(msg->text[0]));
    md_update(&ctx, (BYTE*) msg->title, wcslen(msg->title)*sizeof(msg->title[0]));
    md_final(&ctx, msg->fp.digest);
    return;
}

/* find message with given digest in history */
static struct echo_msg_history *
echo_msg_recall(const BYTE *digest, struct echo_msg_history *hist)
{
    for( ; hist; hist = hist->next)
    {
        if (memcmp(hist->fp.digest, digest, HASHLEN) == 0) break;
    }
    return hist;
}

/* Add an item to message history and return the head of the list */
static struct echo_msg_history*
echo_msg_history_add(struct echo_msg_history *head, const struct echo_msg_fp *fp)
{
    struct echo_msg_history *hist = malloc(sizeof(struct echo_msg_history));
    if (hist)
    {
        memcpy(&hist->fp, fp, sizeof(*fp));
        hist->next = head;
        head = hist;
    }
    return head;
}

/* Save message in history -- update if already present */
static void
echo_msg_save(struct echo_msg *msg)
{
    struct echo_msg_history *hist = echo_msg_recall(msg->fp.digest, msg->history);
    if (hist) /* update */
    {
        hist->fp.timestamp = msg->fp.timestamp;
    }
    else     /* add */
    {
        msg->history = echo_msg_history_add(msg->history, &msg->fp);
    }
}

/* persist echo msg history to the registry */
void
echo_msg_persist(connection_t *c)
{
    struct echo_msg_history *hist;
    size_t len = 0;

    for (hist = c->echo_msg.history; hist; hist = hist->next)
    {
        len++;
        if (len > 99) break; /* max 100 history items persisted */
    }
    if (len == 0)
        return;

    size_t size = len*sizeof(struct echo_msg_fp);
    struct echo_msg_fp *data = malloc(size);
    if (data == NULL)
    {
        WriteStatusLog(c, L"GUI> ", L"Failed to persist echo msg history: Out of memory", false);
        return;
    }

    size_t i = 0;
    for (hist = c->echo_msg.history; i < len && hist; hist = hist->next)
    {
        data[i++] = hist->fp;
    }
    if (!SetConfigRegistryValueBinary(c->config_name, L"echo_msg_history", (BYTE *) data, size))
        WriteStatusLog(c, L"GUI> ", L"Failed to persist echo msg history: error writing to registry", false);

    free(data);
    return;
}

/* load echo msg history from registry */
void
echo_msg_load(connection_t *c)
{
    struct echo_msg_fp *data = NULL;
    DWORD item_len = sizeof(struct echo_msg_fp);

    size_t size = GetConfigRegistryValue(c->config_name, L"echo_msg_history", NULL, 0);
    if (size == 0)
        return; /* no history in registry */
    else if (size%item_len != 0)
    {
        WriteStatusLog(c, L"GUI> ", L"echo msg history in registry has invalid size", false);
        return;
    }

    data = malloc(size);
    if (!data || !GetConfigRegistryValue(c->config_name, L"echo_msg_history", (BYTE*) data, size))
        goto out;

    size_t len = size/item_len;
    for(size_t i = 0; i < len; i++)
    {
        c->echo_msg.history = echo_msg_history_add(c->echo_msg.history, &data[i]);
    }

out:
    free(data);
}

/* Return true if the message is same as recently shown */
static BOOL
echo_msg_repeated(const struct echo_msg *msg)
{
    const struct echo_msg_history *hist;

    hist = echo_msg_recall(msg->fp.digest, msg->history);
    return (hist && (hist->fp.timestamp + o.popup_mute_interval*3600 > msg->fp.timestamp));
}

/* Append a line of echo msg */
static void
echo_msg_append(connection_t *c, time_t UNUSED timestamp, const char *msg, BOOL addnl)
{
    wchar_t *eol = L"";
    wchar_t *wmsg = NULL;

    if (!(wmsg = Widen(msg)))
    {
        WriteStatusLog(c, L"GUI> ", L"Error: out of memory while processing echo msg", false);
        goto out;
    }

    size_t len = c->echo_msg.txtlen + wcslen(wmsg) + 1;  /* including null terminator */
    if (addnl)
    {
        eol = L"\r\n";
        len += 2;
    }
    WCHAR *s = realloc(c->echo_msg.text, len*sizeof(WCHAR));
    if (!s)
    {
        WriteStatusLog(c, L"GUI> ", L"Error: out of memory while processing echo msg", false);
        goto out;
    }
    swprintf(s + c->echo_msg.txtlen, len - c->echo_msg.txtlen,  L"%s%s", wmsg, eol);

    s[len-1] = L'\0';
    c->echo_msg.text = s;
    c->echo_msg.txtlen = len - 1; /* exclude null terminator */

out:
    free(wmsg);
    return;
}

/* Called when echo msg-window or echo msg-notify is received */
static void
echo_msg_display(connection_t *c, time_t timestamp, const char *title, int type)
{
    WCHAR *wtitle = Widen(title);

    if (wtitle)
    {
        c->echo_msg.title = wtitle;
    }
    else
    {
        WriteStatusLog(c, L"GUI> ", L"Error: out of memory converting echo message title to widechar", false);
        c->echo_msg.title = L"Message from server";
    }
    echo_msg_add_fp(&c->echo_msg, timestamp); /* add fingerprint: digest+timestamp */

     /* Check whether the message is muted */
    if (c->flags & FLAG_DISABLE_ECHO_MSG || echo_msg_repeated(&c->echo_msg))
    {
        return;
    }
    if (type == ECHO_MSG_WINDOW)
    {
        DWORD_PTR res;
        UINT timeout = 5000; /* msec */
        if (echo_msg_window
            && SendMessageTimeout(echo_msg_window, WM_OVPN_ECHOMSG, 0, (LPARAM) c, SMTO_BLOCK, timeout, &res) == 0)
        {
            WriteStatusLog(c, L"GUI> Failed to display echo message: ", c->echo_msg.title, false);
        }
    }
    else /* notify */
    {
        ShowTrayBalloon(c->echo_msg.title, c->echo_msg.text);
    }
    /* save or update history */
    echo_msg_save(&c->echo_msg);
}

void
echo_msg_process(connection_t *c, time_t timestamp, const char *s)
{
    wchar_t errmsg[256] = L"";

    char *msg = url_decode(s);
    if (!msg)
    {
        WriteStatusLog(c, L"GUI> ", L"Error in url_decode of echo message", false);
        return;
    }

    if (strbegins(msg, "msg "))
    {
        echo_msg_append(c, timestamp, msg + 4, true);
    }
    else if (streq(msg, "msg")) /* empty msg is treated as a new line */
    {
        echo_msg_append(c, timestamp, msg+3, true);
    }
    else if (strbegins(msg, "msg-n "))
    {
        echo_msg_append(c, timestamp, msg + 6, false);
    }
    else if (strbegins(msg, "msg-window "))
    {
        echo_msg_display(c, timestamp, msg + 11, ECHO_MSG_WINDOW);
        echo_msg_clear(c, false);
    }
    else if (strbegins(msg, "msg-notify "))
    {
        echo_msg_display(c, timestamp, msg + 11, ECHO_MSG_NOTIFY);
        echo_msg_clear(c, false);
    }
    else
    {
        _sntprintf_0(errmsg, L"WARNING: Unknown ECHO directive '%hs' ignored.", msg);
        WriteStatusLog(c, L"GUI> ", errmsg, false);
    }
    free(msg);
}

void
echo_msg_clear(connection_t *c, BOOL clear_history)
{
    CLEAR(c->echo_msg.fp);
    free(c->echo_msg.text);
    free(c->echo_msg.title);
    c->echo_msg.text = NULL;
    c->echo_msg.txtlen = 0;
    c->echo_msg.title = NULL;

    if (clear_history)
    {
        echo_msg_persist(c);
        struct echo_msg_history *head = c->echo_msg.history;
        struct echo_msg_history *next;
        while (head)
        {
            next = head->next;
            free(head);
            head = next;
        }
        CLEAR(c->echo_msg);
    }
}

/*
 * Read the text from edit control h within the range specified in the
 * CHARRANGE structure chrg. Return the result in a newly allocated
 * string or NULL on error.
 *
 * The caller must free the returned pointer.
 */
static wchar_t *
get_text_in_range(HWND h, CHARRANGE chrg)
{
    if (chrg.cpMax <= chrg.cpMin)
	return NULL;

    size_t len = chrg.cpMax - chrg.cpMin;
    wchar_t *txt = malloc((len + 1)*sizeof(wchar_t));

    if (txt)
    {
        TEXTRANGEW txtrg = {chrg, txt};
        if (SendMessage(h, EM_GETTEXTRANGE, 0, (LPARAM)&txtrg) <= 0)
            txt[0] = '\0';
        else
            txt[len] = '\0'; /* safety */
    }
    return txt;
}

/* Enable url detection and subscribe to link click notification in an edit control */
static void
enable_url_detection(HWND hmsg)
{
    /* Recognize URLs embedded in message text */
    SendMessage(hmsg, EM_AUTOURLDETECT, AURL_ENABLEURL, 0);
    /* Have notified by EN_LINK messages when URLs are clicked etc. */
    LRESULT evmask = SendMessage(hmsg, EM_GETEVENTMASK, 0, 0);
    SendMessage(hmsg, EM_SETEVENTMASK, 0, evmask | ENM_LINK);
}

/* Open URL when ENLINK notification is received */
static int
OnEnLinkNotify(HWND UNUSED hwnd, ENLINK *el)
{
    if (el->msg == WM_LBUTTONUP)
    {
        /* get the link text */
        wchar_t *url = get_text_in_range(el->nmhdr.hwndFrom, el->chrg);
        if (url)
            open_url(url);
        free(url);
        return 1;
    }
    return 0;
}

/* Add new message to the message box window */
static void
AddMessageBoxText(HWND hwnd, const wchar_t *text, const wchar_t *title, const wchar_t *from)
{
    HWND hmsg = GetDlgItem(hwnd, ID_TXT_MESSAGE);

    /* Start adding new message at the top */
    SendMessage(hmsg, EM_SETSEL, 0, 0);

    CHARFORMATW cfm = {.cbSize = sizeof(CHARFORMATW) };

    /* save current alignment */
    PARAFORMAT pf = {.cbSize = sizeof(PARAFORMAT) };
    SendMessage(hmsg, EM_GETPARAFORMAT, 0, (LPARAM) &pf);
    WORD pf_align_saved = pf.dwMask & PFM_ALIGNMENT ? pf.wAlignment : PFA_LEFT;
    pf.dwMask |= PFM_ALIGNMENT;

    if (from && wcslen(from))
    {
        /* Change font to italics */
        SendMessage(hmsg, EM_GETCHARFORMAT, SCF_DEFAULT, (LPARAM) &cfm);
        cfm.dwMask |= CFM_ITALIC;
        cfm.dwEffects |= CFE_ITALIC;

        SendMessage(hmsg, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM) &cfm);
       /* Align to right */
        pf.wAlignment = PFA_RIGHT;
        SendMessage(hmsg, EM_SETPARAFORMAT, 0, (LPARAM) &pf);
        SendMessage(hmsg, EM_REPLACESEL, FALSE, (LPARAM) from);
        SendMessage(hmsg, EM_REPLACESEL, FALSE, (LPARAM) L"\n");
    }

    pf.wAlignment = PFA_LEFT;
    SendMessage(hmsg, EM_SETPARAFORMAT, 0, (LPARAM) &pf);

    if (title && wcslen(title))
    {
        /* Increase font size and set font color for title of the message */
        SendMessage(hmsg, EM_GETCHARFORMAT, SCF_DEFAULT, (LPARAM) &cfm);
        cfm.dwMask |= CFM_SIZE|CFM_COLOR;
        cfm.yHeight = MulDiv(cfm.yHeight, 4, 3); /* scale up by 1.33: 12 pt if default is 9 pt */
        cfm.crTextColor = RGB(0, 0x33, 0x99);
        cfm.dwEffects = 0;

        SendMessage(hmsg, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM) &cfm);
        SendMessage(hmsg, EM_REPLACESEL, FALSE, (LPARAM) title);
        SendMessage(hmsg, EM_REPLACESEL, FALSE, (LPARAM) L"\n");
    }

    /* Revert to default font and set the text */
    SendMessage(hmsg, EM_GETCHARFORMAT, SCF_DEFAULT, (LPARAM) &cfm);
    SendMessage(hmsg, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM) &cfm);
    if (text)
    {
        SendMessage(hmsg, EM_REPLACESEL, FALSE, (LPARAM) text);
        SendMessage(hmsg, EM_REPLACESEL, FALSE, (LPARAM) L"\n");
    }
    /* revert alignment */
    pf.wAlignment = pf_align_saved;
    SendMessage(hmsg, EM_SETPARAFORMAT, 0, (LPARAM) &pf);

    /* Remove lines from the window if it is getting full
     * We allow the window to grow by upto 50 lines beyond a
     * max value before truncating
     */
    int pos2 = SendMessage(hmsg, EM_GETLINECOUNT, 0, 0);
    if (pos2 > MAX_MSG_LINES + 50)
    {
        int pos1 = SendMessage(hmsg, EM_LINEINDEX, MAX_MSG_LINES, 0);
        SendMessage(hmsg, EM_SETSEL, pos1, -1);
        SendMessage(hmsg, EM_REPLACESEL, FALSE, (LPARAM) _T(""));
        PrintDebug(L"Text from character position %d to end removed", pos1);
    }

    /* Select top of the message and scroll to there */
    SendMessage(hmsg, EM_SETSEL, 0, 0);
    SendMessage(hmsg, EM_SCROLLCARET, 0, 0);
}

/* A modeless message box.
 * Use AddMessageBoxText to add content and display
 * the window. On WM_CLOSE the window is hidden, not destroyed.
 */
static INT_PTR CALLBACK
MessageDialogFunc(HWND hwnd, UINT msg, UNUSED WPARAM wParam, LPARAM lParam)
{
    HICON hIcon;
    HWND hmsg;
    const UINT top_margin = DPI_SCALE(16);
    const UINT side_margin = DPI_SCALE(20);
    NMHDR *nmh;

    switch (msg)
    {
    case WM_INITDIALOG:
        hIcon = LoadLocalizedIcon(ID_ICO_APP);
        if (hIcon) {
            SendMessage(hwnd, WM_SETICON, (WPARAM) (ICON_SMALL), (LPARAM) (hIcon));
            SendMessage(hwnd, WM_SETICON, (WPARAM) (ICON_BIG), (LPARAM) (hIcon));
        }
        hmsg = GetDlgItem(hwnd, ID_TXT_MESSAGE);
        SetWindowText(hwnd, L"OpenVPN Messages");
        SendMessage(hmsg, EM_SETMARGINS, EC_LEFTMARGIN|EC_RIGHTMARGIN,
                         MAKELPARAM(side_margin, side_margin));

        enable_url_detection(hmsg);

        /* Position the window close to top right corner of the screen */
        RECT rc;
        GetWindowRect(hwnd, &rc);
        OffsetRect(&rc, -rc.left, -rc.top);
        int ox = GetSystemMetrics(SM_CXSCREEN); /* screen size along x */
        ox -= rc.right + DPI_SCALE(rand()%50 + 25);
        int oy = DPI_SCALE(rand()%50 + 25);
        SetWindowPos(hwnd, HWND_TOP, ox > 0 ? ox:0, oy, 0, 0, SWP_NOSIZE);

        return TRUE;

    case WM_SIZE:
        hmsg = GetDlgItem(hwnd, ID_TXT_MESSAGE);
        /* leave some space as top margin */
        SetWindowPos(hmsg, NULL, 0, top_margin, LOWORD(lParam), HIWORD(lParam)-top_margin, 0);
        InvalidateRect(hwnd, NULL, TRUE);
        break;

    /* set the whole client area background to white */
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSTATIC:
        return (INT_PTR) GetStockObject(WHITE_BRUSH);
        break;

    case WM_COMMAND:
        if (LOWORD(wParam) == ID_TXT_MESSAGE)
        {
            /* The caret is distracting in a readonly msg box: hide it when we get focus */
            if (HIWORD(wParam) == EN_SETFOCUS)
            {
                HideCaret((HWND)lParam);
            }
            else if (HIWORD(wParam) == EN_KILLFOCUS)
            {
                ShowCaret((HWND)lParam);
            }
        }
        break;

    /* Must be sent with lParam = connection pointer
     * Adds the current echo message and shows the window.
     */
    case WM_OVPN_ECHOMSG:
        {
            connection_t *c = (connection_t *) lParam;
            wchar_t from[256];
            _sntprintf_0(from, L"From: %s %s", c->config_name, _wctime(&c->echo_msg.fp.timestamp));

            /* strip \n added by _wctime */
            if (wcslen(from) > 0)
                from[wcslen(from)-1] = L'\0';

            AddMessageBoxText(hwnd, c->echo_msg.text, c->echo_msg.title, from);
            SetForegroundWindow(hwnd);
            ShowWindow(hwnd, SW_SHOW);
        }
        break;

    case WM_NOTIFY:
        nmh = (NMHDR*) lParam;
        /* We handle only EN_LINK messages */
        if (nmh->idFrom == ID_TXT_MESSAGE && nmh->code == EN_LINK)
            return OnEnLinkNotify(hwnd, (ENLINK*)lParam);
        break;

    case WM_CLOSE:
        ShowWindow(hwnd, SW_HIDE);
        return TRUE;
    }

    return 0;
}
