/*
 *  OpenVPN-GUI -- A Windows GUI for OpenVPN.
 *
 *  Copyright (C) 2025 Lev Stipakov <lstipakov@gmail.com>
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

#include <windows.h>
#include "qrcodegen/qrcodegen.h"

#include <stdlib.h>

#include "main.h"
#include "qr.h"
#include "misc.h"

#include "openvpn-gui-res.h"

struct url_and_config_name
{
    const wchar_t *url;
    const wchar_t *config_name;
};

typedef struct url_and_config_name url_and_config_name_t;

/* global handle to be able to close the dialog from outside */
static HWND g_hwndQR = NULL;

/* Generate QR bitmap from wide string */
HBITMAP
CreateQRBitmapFromWchar(const wchar_t *wtext, int scale)
{
    char *utf8_text = WCharToUTF8(wtext);
    if (!utf8_text)
        return NULL;

    uint8_t qrcode[qrcodegen_BUFFER_LEN_MAX];
    uint8_t temp[qrcodegen_BUFFER_LEN_MAX];

    bool ok = qrcodegen_encodeText(utf8_text,
                                   temp,
                                   qrcode,
                                   qrcodegen_Ecc_LOW,
                                   qrcodegen_VERSION_MIN,
                                   qrcodegen_VERSION_MAX,
                                   qrcodegen_Mask_AUTO,
                                   true);
    free(utf8_text);
    if (!ok)
        return NULL;

    int size = qrcodegen_getSize(qrcode);
    int imgSize = size * scale;

    HDC hdcScreen = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    HBITMAP hBitmap = CreateCompatibleBitmap(hdcScreen, imgSize, imgSize);
    SelectObject(hdcMem, hBitmap);

    RECT rc = { 0, 0, imgSize, imgSize };
    FillRect(hdcMem, &rc, (HBRUSH)GetStockObject(WHITE_BRUSH));

    HBRUSH blackBrush = (HBRUSH)GetStockObject(BLACK_BRUSH);
    for (int y = 0; y < size; ++y)
    {
        for (int x = 0; x < size; ++x)
        {
            if (qrcodegen_getModule(qrcode, x, y))
            {
                RECT pixelRect = { x * scale, y * scale, (x + 1) * scale, (y + 1) * scale };
                FillRect(hdcMem, &pixelRect, blackBrush);
            }
        }
    }

    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);
    return hBitmap;
}

INT_PTR CALLBACK
QrDialogProc(HWND hwnd, UINT msg, UNUSED WPARAM wp, LPARAM lp)
{
    static HBITMAP hQRBitmap = NULL;
    static UINT_PTR timerId = 1;
    static UINT timerElapse = 20000; /* must be less than 30 sec pre-logon timeout */

    switch (msg)
    {
        case WM_INITDIALOG:
        {
            url_and_config_name_t *uc = (url_and_config_name_t *)lp;

            const int padding = 15;
            const int qrMaxSize = 300; /* max QR bitmap size in pixels */

            /* Generate QR initially at largest acceptable size */
            int scale = 6;
            HBITMAP hQRBitmap = CreateQRBitmapFromWchar(uc->url, scale);
            if (!hQRBitmap)
            {
                MsgToEventLog(EVENTLOG_ERROR_TYPE, L"Failed to generate QR");
                EndDialog(hwnd, 0);
                return TRUE;
            }

            BITMAP bm;
            GetObject(hQRBitmap, sizeof(BITMAP), &bm);

            /* Adjust scale down if bitmap too big */
            while ((bm.bmWidth > qrMaxSize) && scale > 1)
            {
                DeleteObject(hQRBitmap);
                scale--;
                hQRBitmap = CreateQRBitmapFromWchar(uc->url, scale);
                if (!hQRBitmap)
                {
                    MsgToEventLog(EVENTLOG_ERROR_TYPE, L"Failed to generate QR");
                    EndDialog(hwnd, 0);
                    return TRUE;
                }
                GetObject(hQRBitmap, sizeof(BITMAP), &bm);
            }

            int qrWidth = bm.bmWidth;
            int qrHeight = bm.bmHeight;

            HWND hwndStaticQR = GetDlgItem(hwnd, ID_STATIC_QR);
            SetWindowPos(hwndStaticQR, NULL, padding, padding, qrWidth, qrHeight, SWP_NOZORDER);
            SendMessage(hwndStaticQR, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hQRBitmap);

            /* Position text control below QR bitmap */
            HWND hwndStaticText = GetDlgItem(hwnd, ID_TXT_QR);
            int textWidth = qrWidth;
            int textHeight = 50;
            SetWindowPos(hwndStaticText,
                         NULL,
                         padding,
                         qrHeight + padding + 10,
                         textWidth,
                         textHeight,
                         SWP_NOZORDER);

            /* Calculate total dialog size */
            int dialogWidth = qrWidth + padding * 2;
            int dialogHeight = qrHeight + textHeight + padding * 3;

            RECT rcDialog = { 0, 0, dialogWidth, dialogHeight };
            AdjustWindowRectEx(
                &rcDialog, GetWindowLong(hwnd, GWL_STYLE), FALSE, GetWindowLong(hwnd, GWL_EXSTYLE));
            int dlgWidth = rcDialog.right - rcDialog.left;
            int dlgHeight = rcDialog.bottom - rcDialog.top;

            /* Center the dialog on the screen */
            int posX = (GetSystemMetrics(SM_CXSCREEN) - dlgWidth) / 2;
            int posY = (GetSystemMetrics(SM_CYSCREEN) - dlgHeight) / 2;
            SetWindowPos(hwnd, NULL, posX, posY, dlgWidth, dlgHeight, SWP_NOZORDER);

            SetWindowText(hwnd, uc->config_name);

            g_hwndQR = hwnd;

            SetTimer(hwnd, timerId, timerElapse, NULL);
        }
            return TRUE;

        case WM_CLOSE:
            EndDialog(hwnd, 0);
            return TRUE;

        case WM_TIMER:
            if (wp == timerId)
            {
                /* Simulate mouse movement to keep Winlogon from dismissing the PLAP dialog */
                INPUT input = { 0 };
                input.type = INPUT_MOUSE;
                input.mi.dwFlags = MOUSEEVENTF_MOVE;
                input.mi.dx = 0;
                input.mi.dy = 0;
                SendInput(1, &input, sizeof(INPUT));
            }
            return TRUE;

        case WM_DESTROY:
            if (hQRBitmap)
                DeleteObject(hQRBitmap);
            g_hwndQR = NULL;

            KillTimer(hwnd, timerId);
            return TRUE;
    }
    return FALSE;
}

void
CloseQRDialog()
{
    if (g_hwndQR && IsWindow(g_hwndQR))
        PostMessage(g_hwndQR, WM_CLOSE, 0, 0);
}

extern options_t o;

void
OpenQRDialog(const wchar_t *url, const wchar_t *config_name)
{
    url_and_config_name_t uc;
    uc.url = url;
    uc.config_name = config_name;
    DialogBoxParam(o.hInstance, MAKEINTRESOURCE(ID_DLG_QR), o.hWnd, QrDialogProc, (LPARAM)&uc);
}
