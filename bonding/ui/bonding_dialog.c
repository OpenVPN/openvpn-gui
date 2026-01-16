/**
 * @file bonding_dialog.c
 * @brief GUI dialog for bonding configuration
 * 
 * TODO: Implement bonding configuration dialog
 */

#include <windows.h>
#include "bonding_config.h"
#include "nic_detector.h"

INT_PTR CALLBACK BondingDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    /* TODO: Implement dialog procedure for bonding configuration */
    return FALSE;
}

int bonding_dialog_show(HWND parent, bonding_profile_t *profile)
{
    /* TODO: Implement dialog display */
    return -1;
}
