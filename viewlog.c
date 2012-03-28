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

#include "tray.h"
#include "openvpn.h"
#include "main.h"
#include "options.h"
#include "openvpn-gui-res.h"
#include "localization.h"

extern options_t o;

void ViewLog(int config)
{
  TCHAR filename[200];

  STARTUPINFO start_info;
  PROCESS_INFORMATION proc_info;
  SECURITY_ATTRIBUTES sa;
  SECURITY_DESCRIPTOR sd;

  CLEAR (start_info);
  CLEAR (proc_info);
  CLEAR (sa);
  CLEAR (sd);

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
  TCHAR filename[200];

  STARTUPINFO start_info;
  PROCESS_INFORMATION proc_info;
  SECURITY_ATTRIBUTES sa;
  SECURITY_DESCRIPTOR sd;

  CLEAR (start_info);
  CLEAR (proc_info);
  CLEAR (sa);
  CLEAR (sd);

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
