/*
 *  OpenVPN-GUI -- A Windows GUI for OpenVPN.
 *
 *  Copyright (C) 2004 Mathias Sundman <mathias@nilings.se>
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
#include <stdio.h>
#include <shellapi.h>
#include <objbase.h>
#include <shlwapi.h>

#include "tray.h"
#include "openvpn.h"
#include "main.h"
#include "options.h"
#include "openvpn-gui-res.h"
#include "localization.h"

extern options_t o;

void ViewLog(int config)
{
  TCHAR filename[2*MAX_PATH];
  TCHAR assocexe[2*MAX_PATH];
  DWORD assocexe_num;

  STARTUPINFO start_info;
  PROCESS_INFORMATION proc_info;
  SECURITY_ATTRIBUTES sa;
  SECURITY_DESCRIPTOR sd;
  HINSTANCE status = 0;

  CLEAR (start_info);
  CLEAR (proc_info);
  CLEAR (sa);
  CLEAR (sd);

  /* Try first using file association */
  CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE); /* Safe to init COM multiple times */
  if (AssocQueryString(0, ASSOCSTR_EXECUTABLE, L".txt", NULL, assocexe, &assocexe_num) == S_OK)
    status = ShellExecuteW (o.hWnd, L"open", assocexe, o.conn[config].log_path, o.log_dir, SW_SHOWNORMAL);
  if (status > (HINSTANCE) 32) /* Success */
    return;
  else
    PrintDebug (L"Opening log file using ShellExecute with verb = open failed"
                 " for config '%s' (status = %lu)", o.conn[config].config_name, status);

  _sntprintf_0(filename, _T("%s \"%s\""), o.log_viewer, o.conn[config].log_path);

  /* fill in STARTUPINFO struct */
  GetStartupInfo(&start_info);
  start_info.cb = sizeof(start_info);
  start_info.dwFlags = 0;
  start_info.wShowWindow = SW_SHOWDEFAULT;
  start_info.hStdInput = NULL;
  start_info.hStdOutput = NULL;

  if (!CreateProcess(NULL,
		     filename, 	//commandline
		     NULL,
		     NULL,
		     TRUE,
		     CREATE_NEW_CONSOLE,
		     NULL,
		     o.log_dir,	//start-up dir
		     &start_info,
		     &proc_info))
    {
      /* could not start log viewer */
      ShowLocalizedMsg(IDS_ERR_START_LOG_VIEWER, o.log_viewer);
    }

  CloseHandle(proc_info.hThread);
  CloseHandle(proc_info.hProcess);
}


void EditConfig(int config)
{
  TCHAR filename[2*MAX_PATH];
  TCHAR assocexe[2*MAX_PATH];
  DWORD assocexe_num;

  STARTUPINFO start_info;
  PROCESS_INFORMATION proc_info;
  SECURITY_ATTRIBUTES sa;
  SECURITY_DESCRIPTOR sd;
  HINSTANCE status = 0;

  CLEAR (start_info);
  CLEAR (proc_info);
  CLEAR (sa);
  CLEAR (sd);

  /* Try first using file association */
  _sntprintf_0(filename, L"%s\\%s", o.conn[config].config_dir, o.conn[config].config_file);

  CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE); /* Safe to init COM multiple times */
  if (AssocQueryString(0, ASSOCSTR_EXECUTABLE, L".txt", NULL, assocexe, &assocexe_num) == S_OK)
    status = ShellExecuteW (o.hWnd, L"open", assocexe, filename, o.conn[config].config_dir, SW_SHOWNORMAL);
  if (status > (HINSTANCE) 32)
    return;
  else
    PrintDebug (L"Opening config file using ShellExecute with verb = open failed"
                 " for config '%s' (status = %lu)", o.conn[config].config_name, status);

  _sntprintf_0(filename, _T("%s \"%s\\%s\""), o.editor, o.conn[config].config_dir, o.conn[config].config_file);

  /* fill in STARTUPINFO struct */
  GetStartupInfo(&start_info);
  start_info.cb = sizeof(start_info);
  start_info.dwFlags = 0;
  start_info.wShowWindow = SW_SHOWDEFAULT;
  start_info.hStdInput = NULL;
  start_info.hStdOutput = NULL;

  if (!CreateProcess(NULL,
		     filename, 	//commandline
		     NULL,
		     NULL,
		     TRUE,
		     CREATE_NEW_CONSOLE,
		     NULL,
		     o.conn[config].config_dir,	//start-up dir
		     &start_info,
		     &proc_info))
    {
        /* could not start editor */ 
	ShowLocalizedMsg(IDS_ERR_START_CONF_EDITOR, o.editor);
    }

  CloseHandle(proc_info.hThread);
  CloseHandle(proc_info.hProcess);
}
