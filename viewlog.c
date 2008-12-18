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

#include <windows.h>
#include "tray.h"
#include "openvpn.h"
#include <stdio.h>
#include "main.h"
#include "options.h"
#include "openvpn-gui-res.h"

extern struct options o;

void ViewLog(int config)
{
  char filename[200];

  extern char log_viewer[MAX_PATH];
  extern char log_dir[MAX_PATH];

  STARTUPINFO start_info;
  PROCESS_INFORMATION proc_info;
  SECURITY_ATTRIBUTES sa;
  SECURITY_DESCRIPTOR sd;
  char command_line[256];

  CLEAR (start_info);
  CLEAR (proc_info);
  CLEAR (sa);
  CLEAR (sd);

  mysnprintf(filename, "%s \"%s\"", o.log_viewer, o.cnn[config].log_path);

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
      ShowLocalizedMsg(GUI_NAME, ERR_START_LOG_VIEWER, o.log_viewer);
    }

}


void EditConfig(int config)
{
  char filename[200];

  extern char config_dir[MAX_PATH];
  extern char editor[MAX_PATH];

  STARTUPINFO start_info;
  PROCESS_INFORMATION proc_info;
  SECURITY_ATTRIBUTES sa;
  SECURITY_DESCRIPTOR sd;
  char command_line[256];

  CLEAR (start_info);
  CLEAR (proc_info);
  CLEAR (sa);
  CLEAR (sd);

  mysnprintf(filename, "%s \"%s\\%s\"", o.editor, o.cnn[config].config_dir, o.cnn[config].config_file);

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
		     o.cnn[config].config_dir,	//start-up dir
		     &start_info,
		     &proc_info))
    {
        /* could not start editor */ 
	ShowLocalizedMsg(GUI_NAME, ERR_START_CONF_EDITOR, o.editor);
    }

}
