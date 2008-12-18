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
 *
 *  Parts of this sourcefile is taken from openvpnserv.c from the
 *  OpenVPN source, with approval from the author, James Yonan
 *  <jim@yonan.net>.
 */


#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <process.h>
#include "tray.h"
#include "main.h"
#include "openvpn.h"
#include "openvpn_monitor_process.h"
#include "openvpn-gui-res.h"
#include "options.h"
#include "scripts.h"
#include "viewlog.h"
#include "proxy.h"
#include "passphrase.h"
#include <richedit.h>

extern struct options o;

/*
 * Creates a unique exit_event name based on the 
 * config file number.
 */
int CreateExitEvent(int config)
{
  o.cnn[config].exit_event = NULL;
  if (o.oldversion == 1)
    {
      mysnprintf(o.cnn[config].exit_event_name, "openvpn_exit");
      o.cnn[config].exit_event = CreateEvent (NULL,
                                              TRUE, 
                                              FALSE, 
                                              o.cnn[config].exit_event_name);
      if (o.cnn[config].exit_event == NULL)
        {
          if (GetLastError() == ERROR_ACCESS_DENIED)
            {
              /* service mustn't be running, while using old version */ 
              ShowLocalizedMsg(GUI_NAME, ERR_STOP_SERV_ON_OLD_VERSION, "");
            }
          else
            {
              /* error creating exit event */ 
              ShowLocalizedMsg (GUI_NAME, ERR_CREATE_EVENT, o.cnn[config].exit_event_name);
            }
          return(false);
        }
    }
  else
    {
      mysnprintf(o.cnn[config].exit_event_name, "openvpngui_exit_event_%d",config);
      o.cnn[config].exit_event = CreateEvent (NULL,
                                              TRUE, 
                                              FALSE, 
                                              o.cnn[config].exit_event_name);
      if (o.cnn[config].exit_event == NULL)
        {
          /* error creating exit event */
          ShowLocalizedMsg (GUI_NAME, ERR_CREATE_EVENT, o.cnn[config].exit_event_name);
          return(false);
        }
    }

  return(true);
}


/*
 * Set priority based on the registry or cmd-line value
 */
int SetProcessPriority(DWORD *priority)
{

  /* set process priority */
  *priority = NORMAL_PRIORITY_CLASS;
  if (!strcmp (o.priority_string, "IDLE_PRIORITY_CLASS"))
    *priority = IDLE_PRIORITY_CLASS;
  else if (!strcmp (o.priority_string, "BELOW_NORMAL_PRIORITY_CLASS"))
    *priority = BELOW_NORMAL_PRIORITY_CLASS;
  else if (!strcmp (o.priority_string, "NORMAL_PRIORITY_CLASS"))
    *priority = NORMAL_PRIORITY_CLASS;
  else if (!strcmp (o.priority_string, "ABOVE_NORMAL_PRIORITY_CLASS"))
    *priority = ABOVE_NORMAL_PRIORITY_CLASS;
  else if (!strcmp (o.priority_string, "HIGH_PRIORITY_CLASS"))
    *priority = HIGH_PRIORITY_CLASS;
  else
    {
      /* unknown priority */
      ShowLocalizedMsg (GUI_NAME, ERR_UNKNOWN_PRIORITY, o.priority_string);
      return (false);
    }

  return(true);
}


/*
 * Launch an OpenVPN process
 */
int StartOpenVPN(int config)
{

  HANDLE hOutputReadTmp = NULL;
  HANDLE hOutputRead = NULL;
  HANDLE hOutputWrite = NULL;
  HANDLE hInputWriteTmp = NULL;
  HANDLE hInputRead = NULL;
  HANDLE hInputWrite = NULL;
  HANDLE hErrorWrite = NULL;

  HANDLE hThread; 
  DWORD IDThread; 
  DWORD priority;
  STARTUPINFO start_info;
  PROCESS_INFORMATION proc_info;
  SECURITY_ATTRIBUTES sa;
  SECURITY_DESCRIPTOR sd;
  char command_line[256];
  char proxy_string[100];
  char msg[200];
  int i, is_connected=0;

  extern HINSTANCE hInstance;

  CLEAR (start_info);
  CLEAR (proc_info);
  CLEAR (sa);
  CLEAR (sd);


  /* If oldversion, allow only ONE connection */
  if (o.oldversion == 1)
    {
      for (i=0; i < o.num_configs; i++) 
        {
          if ((o.cnn[i].connect_status != DISCONNECTED) &&
             (o.cnn[i].connect_status != DISCONNECTING))
            {
              is_connected=1;
              break;
            }
        }
      if (is_connected)
        {
          /* only one simultanious connection on old version */
          ShowLocalizedMsg(GUI_NAME, ERR_ONLY_ONE_CONN_OLD_VERSION, ""); 
          return(false);
        }
    }

  /* Warn if "log" or "log-append" option is found in config file */
  if ((ConfigFileOptionExist(config, "log ")) ||
      (ConfigFileOptionExist(config, "log-append ")))
    {
      TCHAR buf[1000];
      myLoadString(ERR_OPTION_LOG_IN_CONFIG);
      if (MessageBox(NULL, buf, GUI_NAME, MB_YESNO | MB_DEFBUTTON2 | MB_ICONWARNING) != IDYES)
        return(false);
    }

  /* Clear connection unique vars */
  o.cnn[config].failed_psw = 0;
  CLEAR (o.cnn[config].ip);

  /* Create our exit event */
  if (!CreateExitEvent(config)) 
    return(false);

  /* set process priority */
  if (!SetProcessPriority(&priority))
    goto failed;

  /* Check that log append flag has a valid value */
  if ((o.append_string[0] != '0') && (o.append_string[0] != '1'))
    {
      /* append_log must be 0 or 1 */
      ShowLocalizedMsg (GUI_NAME, ERR_LOG_APPEND_BOOL, o.append_string);
      goto failed;
    }
        
  /* construct proxy string to append to command line */
  ConstructProxyCmdLine(proxy_string, sizeof(proxy_string));

  /* construct command line */
  if (o.oldversion == 1)
    {
      mysnprintf (command_line, "openvpn --config \"%s\" %s",
                  o.cnn[config].config_file, proxy_string);
    }
  else
    {
      mysnprintf (command_line, "openvpn --service %s 0 --config \"%s\" %s",
                  o.cnn[config].exit_event_name,
                  o.cnn[config].config_file,
                  proxy_string);
    }


  /* Make security attributes struct for logfile handle so it can
     be inherited. */
  sa.nLength = sizeof (sa);
  sa.lpSecurityDescriptor = &sd;
  sa.bInheritHandle = TRUE;
  if (!InitializeSecurityDescriptor (&sd, SECURITY_DESCRIPTOR_REVISION))
    {
      /* Init Sec. Desc. failed */
      ShowLocalizedMsg (GUI_NAME, ERR_INIT_SEC_DESC, "");
      goto failed;
    }
  if (!SetSecurityDescriptorDacl (&sd, TRUE, NULL, FALSE))
    {
      /* set Dacl failed */
      ShowLocalizedMsg (GUI_NAME, ERR_SET_SEC_DESC_ACL, "");
      goto failed;
    }


  /* Create the child output pipe. */
  if (!CreatePipe(&hOutputReadTmp,&hOutputWrite,&sa,0))
    {
      /* CreatePipe failed. */
      ShowLocalizedMsg(GUI_NAME, ERR_CREATE_PIPE_OUTPUT, "");
      goto failed;
    }

  // Create a duplicate of the output write handle for the std error
  // write handle. This is necessary in case the child application
  // closes one of its std output handles.
  if (!DuplicateHandle(GetCurrentProcess(),hOutputWrite,
                       GetCurrentProcess(),&hErrorWrite,0,
                       TRUE,DUPLICATE_SAME_ACCESS))
    { 
      /* DuplicateHandle failed. */
      ShowLocalizedMsg(GUI_NAME, ERR_DUP_HANDLE_ERR_WRITE, "");
      goto failed;
    }

  // Create the child input pipe.
  if (!CreatePipe(&hInputRead,&hInputWriteTmp,&sa,0))
    {
      /* CreatePipe failed. */
      ShowLocalizedMsg(GUI_NAME, ERR_CREATE_PIPE_INPUT, "");
      goto failed;
    }

  // Create new output read handle and the input write handles. Set
  // the Properties to FALSE. Otherwise, the child inherits the
  // properties and, as a result, non-closeable handles to the pipes
  // are created.
  if (!DuplicateHandle(GetCurrentProcess(),hOutputReadTmp,
                       GetCurrentProcess(),
                       &hOutputRead, // Address of new handle.
                       0,FALSE, // Make it uninheritable.
                       DUPLICATE_SAME_ACCESS))
    {
      /* Duplicate Handle failed. */
      ShowLocalizedMsg(GUI_NAME, ERR_DUP_HANDLE_OUTPUT_READ, "");
      goto failed;
    }

  if (!DuplicateHandle(GetCurrentProcess(),hInputWriteTmp,
                       GetCurrentProcess(),
                       &hInputWrite, // Address of new handle.
                       0,FALSE, // Make it uninheritable.
                       DUPLICATE_SAME_ACCESS))
    {
      /* DuplicateHandle failed */
      ShowLocalizedMsg(GUI_NAME, ERR_DUP_HANDLE_INPUT_WRITE, "");
      goto failed;
    }

  /* Close inheritable copies of the handles */
  if (!CloseHandle(hOutputReadTmp) || !CloseHandle(hInputWriteTmp)) 
    {
      /* Close Handle failed */
      ShowLocalizedMsg(GUI_NAME, ERR_CLOSE_HANDLE_TMP, "");
      CloseHandle (o.cnn[config].exit_event);
      return(0); 
    }
  hOutputReadTmp=NULL;
  hInputWriteTmp=NULL;

  /* fill in STARTUPINFO struct */
  GetStartupInfo(&start_info);
  start_info.cb = sizeof(start_info);
  start_info.dwFlags = STARTF_USESTDHANDLES;
  start_info.hStdInput = hInputRead;
  start_info.hStdOutput = hOutputWrite;
  start_info.hStdError = hErrorWrite;

  /* Run Pre-connect script */
  RunPreconnectScript(config);

  /* create an OpenVPN process for one config file */
  if (!CreateProcess(o.exe_path,
		     command_line,
		     NULL,
		     NULL,
		     TRUE,
		     priority | CREATE_NO_WINDOW,
		     NULL,
		     o.cnn[config].config_dir,
		     &start_info,
		     &proc_info))
    {
      /* CreateProcess failed */
      ShowLocalizedMsg (GUI_NAME, ERR_CREATE_PROCESS,
          o.exe_path,
          command_line,
          o.cnn[config].config_dir);
      goto failed;
    }


  /* close unneeded handles */
  Sleep (250); /* try to prevent race if we close logfile
                  handle before child process DUPs it */

  if(!CloseHandle (proc_info.hThread) ||
     !CloseHandle (hOutputWrite) ||
     !CloseHandle (hInputRead) ||
     !CloseHandle (hErrorWrite))
    {
      /* CloseHandle failed */
      ShowLocalizedMsg (GUI_NAME, ERR_CLOSE_HANDLE, "");
      CloseHandle (o.cnn[config].exit_event);
      return(false);
    }
  hOutputWrite = NULL;
  hInputRead = NULL;
  hErrorWrite = NULL;
 
  /* Save StdIn and StdOut handles in our options struct */
  o.cnn[config].hStdIn = hInputWrite;
  o.cnn[config].hStdOut = hOutputRead;

  /* Save Process Handle */
  o.cnn[config].hProcess=proc_info.hProcess;


  /* Start Thread to show Status Dialog */
  hThread = CreateThread(NULL, 0, 
            (LPTHREAD_START_ROUTINE) ThreadOpenVPNStatus,
            (int *) config,  // pass config nr
            0, &IDThread); 
  if (hThread == NULL) 
    {
    /* CreateThread failed */
    ShowLocalizedMsg (GUI_NAME, ERR_CREATE_THREAD_STATUS, "");
    goto failed;
  }


  return(true); 

failed:
  if (o.cnn[config].exit_event) CloseHandle (o.cnn[config].exit_event);
  if (hOutputWrite) CloseHandle (hOutputWrite);
  if (hOutputRead) CloseHandle (hOutputRead);
  if (hInputWrite) CloseHandle (hInputWrite);
  if (hInputRead) CloseHandle (hInputRead);
  if (hErrorWrite) CloseHandle (hOutputWrite);
  return(false);

}


void StopOpenVPN(int config)
{
  int i;
  TCHAR buf[1000];

  o.cnn[config].connect_status = DISCONNECTING;
  
  if (o.cnn[config].exit_event) {
    /* Run Disconnect script */
    RunDisconnectScript(config, false);

    EnableWindow(GetDlgItem(o.cnn[config].hwndStatus, ID_DISCONNECT), FALSE);
    EnableWindow(GetDlgItem(o.cnn[config].hwndStatus, ID_RESTART), FALSE);
    SetMenuStatus(config, DISCONNECTING);
    myLoadString(INFO_STATE_WAIT_TERM);
    /* UserInfo: waiting for OpenVPN termination... */
    SetDlgItemText(o.cnn[config].hwndStatus, TEXT_STATUS, buf); 
    SetEvent(o.cnn[config].exit_event);
  }
}

void SuspendOpenVPN(int config)
{
  int i;
  TCHAR buf[1000];

  o.cnn[config].connect_status = SUSPENDING;
  o.cnn[config].restart = true;

  if (o.cnn[config].exit_event) {
    EnableWindow(GetDlgItem(o.cnn[config].hwndStatus, ID_DISCONNECT), FALSE);
    EnableWindow(GetDlgItem(o.cnn[config].hwndStatus, ID_RESTART), FALSE);
    SetMenuStatus(config, DISCONNECTING);
    myLoadString(INFO_STATE_WAIT_TERM);
    SetDlgItemText(o.cnn[config].hwndStatus, TEXT_STATUS, buf);
    SetEvent(o.cnn[config].exit_event);
  }
}


void StopAllOpenVPN()
{
  int i;
 
  for(i=0; i < o.num_configs; i++) {
    if(o.cnn[i].connect_status != DISCONNECTED)
      StopOpenVPN(i);
  }

  /* Wait for all connections to terminate (Max 5 sec) */
  for (i=0; i<20; i++, Sleep(250))
    if (CountConnectedState(DISCONNECTED) == o.num_configs) break;

}


BOOL CALLBACK StatusDialogFunc (HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
  BOOL Translated;
  HWND hwndLogWindow;
  RECT rect;
  CHARFORMAT charformat;
  HICON hIcon;
  int config;

  switch (msg) {

    case WM_INITDIALOG:
      /* Set Window Icon "DisConnected" */
      SetStatusWinIcon(hwndDlg, APP_ICON_CONNECTING);

      /* Create LogWindow */
      hwndLogWindow = CreateWindowEx (0, RICHEDIT_CLASS, NULL, 
                                      WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL | \
                                      ES_SUNKEN | ES_LEFT | ES_MULTILINE | \
                                      ES_READONLY | ES_AUTOHSCROLL | ES_AUTOVSCROLL,
                                      20, 25, 350, 160,		// Posision and Size
                                      hwndDlg,			// Parent window handle
                                      (HMENU) EDIT_LOG,		// hMenu
                                      o.hInstance,		// hInstance
                                      NULL);			// WM_CREATE lpParam

                                     
      if (!hwndLogWindow)
        {
          /* Create RichEd LogWindow Failed */
          ShowLocalizedMsg(GUI_NAME, ERR_CREATE_RICHED_LOGWINDOW, "");
          return FALSE;
        }

      /* Set font and fontsize of the LogWindow */
      charformat.cbSize = sizeof(CHARFORMAT); 
      charformat.dwMask = CFM_SIZE | CFM_FACE | CFM_BOLD | CFM_ITALIC | \
                          CFM_UNDERLINE | CFM_STRIKEOUT | CFM_PROTECTED;
      charformat.dwEffects = 0;
      charformat.yHeight = 100;
      strcpy(charformat.szFaceName, "MS Sans Serif");
      if ((SendMessage(hwndLogWindow, EM_SETCHARFORMAT, SCF_DEFAULT, (LPARAM) &charformat) && CFM_SIZE) == 0) {
        /* set size failed */
        ShowLocalizedMsg(GUI_NAME, ERR_SET_SIZE, "");
      }

      /* Set Size and Posision of controls */
      GetClientRect(hwndDlg, &rect);
      MoveWindow (hwndLogWindow, 20, 25, rect.right - 40, rect.bottom - 70, TRUE);
      MoveWindow (GetDlgItem(hwndDlg, TEXT_STATUS), 20, 5, rect.right - 25, 15, TRUE);
      MoveWindow (GetDlgItem(hwndDlg, ID_DISCONNECT), 20, rect.bottom - 30, 90, 23, TRUE);
      MoveWindow (GetDlgItem(hwndDlg, ID_RESTART), 125, rect.bottom - 30, 90, 23, TRUE);
      MoveWindow (GetDlgItem(hwndDlg, ID_HIDE), rect.right - 110, rect.bottom - 30, 90, 23, TRUE);

      /* Set focus on the LogWindow so it scrolls automatically */
      SetFocus(hwndLogWindow);

      return FALSE;

    case WM_SIZE:
      MoveWindow (GetDlgItem(hwndDlg, EDIT_LOG), 20, 25, LOWORD (lParam) - 40,
                  HIWORD (lParam) - 70, TRUE);
      MoveWindow (GetDlgItem(hwndDlg, ID_DISCONNECT), 20,
                  HIWORD (lParam) - 30, 90, 23, TRUE);
      MoveWindow (GetDlgItem(hwndDlg, ID_RESTART), 125,
                  HIWORD (lParam) - 30, 90, 23, TRUE);
      MoveWindow (GetDlgItem(hwndDlg, ID_HIDE), LOWORD (lParam) - 110,
                  HIWORD (lParam) - 30, 90, 23, TRUE);
      MoveWindow (GetDlgItem(hwndDlg, TEXT_STATUS), 20, 5, LOWORD (lParam) - 25, 15, TRUE);
      InvalidateRect(hwndDlg, NULL, TRUE);
      return TRUE;

    case WM_COMMAND:
      config=GetDlgItemInt(hwndDlg, TEXT_CONFIG, &Translated, FALSE);
      switch (LOWORD(wParam)) {

        case ID_DISCONNECT:
          SetFocus(GetDlgItem(o.cnn[config].hwndStatus, EDIT_LOG));
          StopOpenVPN(config);
          return TRUE;

        case ID_HIDE:
          if (o.cnn[GetDlgItemInt(hwndDlg, TEXT_CONFIG, &Translated, FALSE)].connect_status != DISCONNECTED)
            {
              ShowWindow(hwndDlg, SW_HIDE);
            }
          else
            {
              DestroyWindow(hwndDlg);
            }
          return TRUE;
        case ID_RESTART:
          SetFocus(GetDlgItem(o.cnn[config].hwndStatus, EDIT_LOG));
          o.cnn[config].restart = true;
          StopOpenVPN(config);
          return TRUE; 
      }
      break;

    case WM_SHOWWINDOW:
      if (wParam == TRUE)
        {
          config=GetDlgItemInt(hwndDlg, TEXT_CONFIG, &Translated, FALSE);
          if (o.cnn[config].hwndStatus)
            SetFocus(GetDlgItem(o.cnn[config].hwndStatus, EDIT_LOG));
        }
      return FALSE;

    case WM_CLOSE:
      if (o.cnn[GetDlgItemInt(hwndDlg, TEXT_CONFIG, &Translated, FALSE)].connect_status != DISCONNECTED)
        {
          ShowWindow(hwndDlg, SW_HIDE);
        }
      else
        {
          DestroyWindow(hwndDlg);
        }
      return TRUE;

    case WM_DESTROY:
      PostQuitMessage(0);
      break; 
  }
  return FALSE;
}

void SetStatusWinIcon(HWND hwndDlg, int IconID)
{
  HICON hIcon;

  /* Set Window Icon */
  hIcon = (HICON)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IconID), 
                                                IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
  if (hIcon) {
    SendMessage(hwndDlg, WM_SETICON, (WPARAM) (ICON_SMALL), (LPARAM) (hIcon));
    SendMessage(hwndDlg, WM_SETICON, (WPARAM) (ICON_BIG), (LPARAM) (hIcon));  
  }
}

int AutoStartConnections()
{
  int i;

  for (i=0; i < o.num_configs; i++)
    {
      if (o.cnn[i].auto_connect)
        StartOpenVPN(i);
    }

  return(true);
}

int VerifyAutoConnections()
{
  int i,j;
  BOOL match;

  for (i=0; (o.auto_connect[i] != 0) && (i < MAX_CONFIGS); i++)
    {
      match = false;
      for (j=0; j < MAX_CONFIGS; j++)
        {
          if (strcasecmp(o.cnn[j].config_file, o.auto_connect[i]) == 0)
            {
              match=true;
              break;
            }
        }
      if (match == false)
        {
          /* autostart config not found */
          ShowLocalizedMsg(GUI_NAME, ERR_AUTOSTART_CONF_NOT_FOUND, o.auto_connect[i]);
          return false;
        }
    }
  
  return true;
}


int CheckVersion()
{
  HANDLE hOutputReadTmp = NULL;
  HANDLE hOutputRead = NULL;
  HANDLE hOutputWrite = NULL;
  HANDLE hInputWriteTmp = NULL;
  HANDLE hInputRead = NULL;
  HANDLE hInputWrite = NULL;
  HANDLE exit_event;
  HANDLE debug_event;

  STARTUPINFO start_info;
  PROCESS_INFORMATION proc_info;
  SECURITY_ATTRIBUTES sa;
  SECURITY_DESCRIPTOR sd;
  char command_line[256];
  char line[1024];
  char bin_path[MAX_PATH];
  char *p;
  int oldversion, i;

  CLEAR (start_info);
  CLEAR (proc_info);
  CLEAR (sa);
  CLEAR (sd);
   
  exit_event = CreateEvent (NULL, TRUE, FALSE, "openvpn_exit");
  if (exit_event == NULL)
    {
#ifdef DEBUG
      PrintErrorDebug("CreateEvent(openvpn_exit) failed.");
#endif
      if (GetLastError() == ERROR_ACCESS_DENIED)
        {
          /* Assume we're running OpenVPN 1.5/1.6 and the service is started. */
          o.oldversion=1;
          strncpy(o.connect_string, "Successful ARP Flush", sizeof(o.connect_string));
          return(true);
        }
      else
        {
          /* CreateEvent failed */
          ShowLocalizedMsg(GUI_NAME, ERR_VERSION_CREATE_EVENT, "");
          return(false);
       }
    }

#ifdef DEBUG
  PrintErrorDebug("CreateEvent(openvpn_exit) succeded.");
#endif

  /* construct command line */
  mysnprintf (command_line, "openvpn --version");

  /* construct bin path */
  strncpy(bin_path, o.exe_path, sizeof(bin_path));
  for (i=strlen(bin_path) - 1; i > 0; i--)
    if (bin_path[i] == '\\') break;
  bin_path[i] = '\0';

  /* Make security attributes struct for logfile handle so it can
     be inherited. */
  sa.nLength = sizeof (sa);
  sa.lpSecurityDescriptor = &sd;
  sa.bInheritHandle = TRUE;
  if (!InitializeSecurityDescriptor (&sd, SECURITY_DESCRIPTOR_REVISION))
  {
      /* Init Sec. Desc. failed */
      ShowLocalizedMsg (GUI_NAME, ERR_INIT_SEC_DESC, "");
      return(0);
    }
  if (!SetSecurityDescriptorDacl (&sd, TRUE, NULL, FALSE))
    {
      /* Set Dacl failed */
      ShowLocalizedMsg (GUI_NAME, ERR_SET_SEC_DESC_ACL, "");
      return(0);
    }

  /* Create the child input pipe. */
  if (!CreatePipe(&hInputRead,&hInputWriteTmp,&sa,0))
    {
      /* create pipe failed */
      ShowLocalizedMsg(GUI_NAME, ERR_CREATE_PIPE_INPUT_READ, "");
      return(0);
    }

  /* Create the child output pipe. */
  if (!CreatePipe(&hOutputReadTmp,&hOutputWrite,&sa,0))
    {
      /* CreatePipe failed */
      ShowLocalizedMsg(GUI_NAME, ERR_CREATE_PIPE_OUTPUT, "");
      return(0);
    }

  if (!DuplicateHandle(GetCurrentProcess(),hOutputReadTmp,
                       GetCurrentProcess(),
                       &hOutputRead, // Address of new handle.
                       0,FALSE, // Make it uninheritable.
                       DUPLICATE_SAME_ACCESS))
    {
      /* DuplicateHandle failed */
      ShowLocalizedMsg(GUI_NAME, ERR_DUP_HANDLE_OUTPUT_READ, "");
      return(0);
    }

  if (!DuplicateHandle(GetCurrentProcess(),hInputWriteTmp,
                       GetCurrentProcess(),
                       &hInputWrite, // Address of new handle.
                       0,FALSE, // Make it uninheritable.
                       DUPLICATE_SAME_ACCESS))
    {
      /* DuplicateHandle failed */
      ShowLocalizedMsg(GUI_NAME, ERR_DUP_HANDLE_INPUT_WRITE, "");
      return(0);
    }


  /* Close inheritable copies of the handles */
  if (!CloseHandle(hOutputReadTmp) || !CloseHandle(hInputWriteTmp)) 
    {
      /* CloseHandle failed */
      ShowLocalizedMsg(GUI_NAME, ERR_CLOSE_HANDLE_TMP, "");
      return(0); 
    }

  /* fill in STARTUPINFO struct */
  GetStartupInfo(&start_info);
  start_info.cb = sizeof(start_info);
  start_info.dwFlags = STARTF_USESTDHANDLES|STARTF_USESHOWWINDOW;
  start_info.wShowWindow = SW_HIDE;
  start_info.hStdInput = hInputRead;
  start_info.hStdOutput = hOutputWrite;
  start_info.hStdError = hOutputWrite;

  /* Start OpenVPN to check version */
  if (!CreateProcess(o.exe_path,
		     command_line,
		     NULL,
		     NULL,
		     TRUE,
		     CREATE_NEW_CONSOLE,
		     NULL,
		     bin_path,
		     &start_info,
		     &proc_info))
    {
      /* CreateProcess failed */
      ShowLocalizedMsg (GUI_NAME, ERR_CREATE_PROCESS,
          o.exe_path,
          command_line,
          bin_path);
      return(0);
    }

  /* Default value for oldversion */
  oldversion=0;

  /* Default string to look for to report "Connected". */
  strncpy(o.connect_string, "Successful ARP Flush", sizeof(o.connect_string));

  if (ReadLineFromStdOut(hOutputRead, 0, line) == 1)
    {
#ifdef DEBUG
      PrintDebug("VersionString: %s", line);
#endif
      if (line[8] == '2') /* Majorversion = 2 */
        {
          if (line[10] == '0') /* Minorversion = 0 */
            {
              p=strstr(line, "beta");
              if (p != NULL)
                {
                  if (p[5] == ' ') /* 2.0-beta1 - 2.0-beta9 */
                    {
                      if (p[4] >= '6') /* 2.0-beta6 - 2.0-beta9 */
                        {
                          oldversion=0;
                        }
                      else /* < 2.0-beta6 */
                        {
                          oldversion=1;
                        }
                    }
                  else /* >= 2.0-beta10 */
                    {
                      if (strncmp(&p[6], "ms", 2) == 0) /* 2.0-betaXXms */
                        strncpy(o.connect_string, "Initialization Sequence Completed",
                                sizeof(o.connect_string));
                      if ( !((p[4] == 1) && (p[5] == 0)) ) /* >= 2.0-beta11 */
                        strncpy(o.connect_string, "Initialization Sequence Completed",
                                sizeof(o.connect_string));
                        
                      oldversion=0;
                    }
                }
              else /* 2.0 non-beta */
                {
                  strncpy(o.connect_string, "Initialization Sequence Completed",
                          sizeof(o.connect_string));
                  oldversion=0;
                }
            }
          else /* > 2.0 */
            {
              strncpy(o.connect_string, "Initialization Sequence Completed",
                      sizeof(o.connect_string));
              oldversion=0;
            }
        }
      else
        {
          if (line[8] == '1') /* Majorversion = 1 */
            {
              oldversion=1;
            }
          else /* Majorversion != (1 || 2) */
            {
              oldversion=0;
            }
        }
    }
  else return(0);

  o.oldversion = oldversion;
  

  if(!CloseHandle (proc_info.hThread) || !CloseHandle (hOutputWrite)
     || !CloseHandle (hInputRead) || !CloseHandle(exit_event))
    {
      /* CloseHandle failed */
      ShowLocalizedMsg (GUI_NAME, ERR_CLOSE_HANDLE, "");
      return(0);
    }

  return(1); 
}

/* Return num of connections with Status = CheckVal */
int CountConnectedState(int CheckVal)
{
  int i;
  int count=0;
  
  for (i=0; i < o.num_configs; i++)
    {
      if (o.cnn[i].connect_status == CheckVal)
        count++;
    }

  return (count);
}

void CheckAndSetTrayIcon()
{

  /* Show green icon if service is running */
  if (o.service_running == SERVICE_CONNECTED)
    {
      SetTrayIcon(CONNECTED);
      return;
    }

  /* Change tray icon if no more connections is running */
  if (CountConnectedState(CONNECTED) != 0)
    SetTrayIcon(CONNECTED);
  else
    {
      if ((CountConnectedState(CONNECTING) != 0) || 
          (CountConnectedState(RECONNECTING) != 0) ||
          (o.service_running == SERVICE_CONNECTING))
        SetTrayIcon(CONNECTING);
      else
        SetTrayIcon(DISCONNECTED);
    }
}

void ThreadOpenVPNStatus(int config) 
{
  char conn_name[200];
  HANDLE hThread; 
  DWORD IDThread; 
  char msg[200];
  MSG messages;

  /* Cut of extention from config filename. */
  strncpy(conn_name, o.cnn[config].config_file, sizeof(conn_name));
  conn_name[strlen(conn_name) - (strlen(o.ext_string)+1)]=0;

  if (o.cnn[config].restart)
    {
      /* UserInfo: Connecting */
      TCHAR buf[1000];
      myLoadString(INFO_STATE_CONNECTING);
      SetDlgItemText(o.cnn[config].hwndStatus, TEXT_STATUS, buf); 
      SetStatusWinIcon(o.cnn[config].hwndStatus, APP_ICON_CONNECTING);
      EnableWindow(GetDlgItem(o.cnn[config].hwndStatus, ID_DISCONNECT), TRUE);
      EnableWindow(GetDlgItem(o.cnn[config].hwndStatus, ID_RESTART), TRUE);
      SetFocus(GetDlgItem(o.cnn[config].hwndStatus, EDIT_LOG));
      o.cnn[config].restart = false;
    }
  else
    {
      /* Create and Show Status Dialog */  
      TCHAR buf[1000];
      if (!(o.cnn[config].hwndStatus = CreateDialog (o.hInstance, 
            MAKEINTRESOURCE (IDD_STATUS),
            NULL, (DLGPROC) StatusDialogFunc)))
        ExitThread(1);
      /* UserInfo: Connecting */
      myLoadString(INFO_STATE_CONNECTING);
      SetDlgItemText(o.cnn[config].hwndStatus, TEXT_STATUS, (LPCTSTR)buf); 
      SetDlgItemInt(o.cnn[config].hwndStatus, TEXT_CONFIG, (UINT)config, FALSE);
      myLoadString(INFO_CONNECTION_XXX);
      mysnprintf(msg, buf, conn_name);
      SetWindowText(o.cnn[config].hwndStatus, msg);

      if (o.silent_connection[0]=='0')
        ShowWindow(o.cnn[config].hwndStatus, SW_SHOW);
    }


  /* Start Thread to monitor our OpenVPN process */
  hThread = CreateThread(NULL, 0, 
            (LPTHREAD_START_ROUTINE) WatchOpenVPNProcess,
            (int *) config,  // pass config nr
            0, &IDThread); 
  if (hThread == NULL) 
    {
      /* CreateThread failed */
      ShowLocalizedMsg (GUI_NAME, ERR_CREATE_THREAD_READ_STDOUT, "");
      ExitThread(0);
    }

  /* Run the message loop. It will run until GetMessage() returns 0 */
  while (GetMessage (&messages, NULL, 0, 0))
    {
      if(!IsDialogMessage(o.cnn[config].hwndStatus, &messages))
      {
        TranslateMessage(&messages);
        DispatchMessage(&messages);
      }
    }

  ExitThread(0);
}

