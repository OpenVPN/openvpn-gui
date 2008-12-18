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
 */

#include <windows.h>
#include <time.h>

/*
 * Maximum number of parameters associated with an option,
 * including the option name itself.
 */
#define MAX_PARMS 		5
#define MAX_CONFIGS		50	/* Max number of config-files to read. */
#define MAX_CONFIG_SUBDIRS	50	/* Max number of subdirs to scan */

/* connect_status STATES */
#define DISCONNECTED		0
#define CONNECTING		1
#define CONNECTED		2
#define RECONNECTING		3
#define DISCONNECTING		4
#define SUSPENDING		5
#define SUSPENDED		6

/* OpenVPN Service STATES */
#define SERVICE_NOACCESS       -1
#define SERVICE_DISCONNECTED	0
#define SERVICE_CONNECTING	1
#define	SERVICE_CONNECTED	2

/* Connections parameters */
struct connections
{
  char config_file[MAX_PATH]; 	/* Name of the config file */
  char config_name[MAX_PATH];	/* Name of the connection */
  char config_dir[MAX_PATH];	/* Path to this configs dir */
  char log_path[MAX_PATH];	/* Path to Logfile */
  char ip[16];			/* Assigned IP address for this connection */
  char exit_event_name[50];	/* Exit Event name for this connection */
  int connect_status;		/* 0=Not Connected 1=Connecting 
                                   2=Connected 3=Reconnecting 4=Disconnecting */
  int auto_connect;		/* true=AutoConnect at startup */
  int failed_psw;		/* 1=OpenVPN failed because of wrong psw */
  int failed_psw_attempts;	/* # of failed attempts maid to enter psw */
  int restart;			/* true=Restart connection after termination */
  time_t connected_since;	/* Time when the connection was established */

  HANDLE exit_event;
  HANDLE hProcess;
  HANDLE hStdOut;
  HANDLE hStdIn;
  HWND hwndStatus;		/* Handle to Status Dialog Window */
};

/* All options used within OpenVPN GUI */
struct options
{
  /* Array of configs to autostart */
  const char *auto_connect[MAX_CONFIGS];

  /* Connection parameters */
  struct connections cnn[MAX_CONFIGS];  /* Connection structure */
  int num_configs;			/* Number of configs */

  int oldversion;			/* 1=OpenVPN version below 2.0-beta6 */
  char connect_string[100];		/* String to look for to report connected */
  int psw_attempts;			/* Number of psw attemps to allow */
  int connectscript_timeout;		/* Connect Script execution timeout (sec) */
  int disconnectscript_timeout;		/* Disconnect Script execution timeout (sec) */
  int preconnectscript_timeout;		/* Preconnect Script execution timeout (sec) */
  int service_running;			/* true if OpenVPN Service is started */
  HWND hWnd;				/* Main Window Handle */
  HINSTANCE hInstance;

  /* Registry values */
  char exe_path[MAX_PATH];
  char config_dir[MAX_PATH];
  char ext_string[16];
  char log_dir[MAX_PATH];
  char priority_string[64];
  char append_string[2];
  char log_viewer[MAX_PATH];
  char editor[MAX_PATH];
  char allow_edit[2];
  char allow_service[2];
  char allow_password[2];
  char allow_proxy[2];
  char silent_connection[2];
  char service_only[2];
  char show_balloon[2];
  char show_script_window[2];
  char psw_attempts_string[2];
  char disconnect_on_suspend[2];
  char connectscript_timeout_string[4];
  char disconnectscript_timeout_string[4];
  char preconnectscript_timeout_string[4];

  /* Proxy Settings */
  int proxy_source;			/* 0=OpenVPN config, 1=IE, 2=Manual */
  int proxy_type;			/* 0=HTTP, 1=SOCKS */
  int proxy_http_auth;			/* 0=Auth Disabled, 1=Auth Enabled */
  char proxy_http_address[100];		/* HTTP Proxy Address */
  char proxy_http_port[6];		/* HTTP Proxy Port */
  char proxy_socks_address[100];	/* SOCKS Proxy Address */
  char proxy_socks_port[6];		/* SOCKS Proxy Address */
  char proxy_authfile[100];		/* Path to proxy auth file */ 

  /* Debug file pointer */
#ifdef DEBUG
  FILE *debug_fp;
#endif
};

#define streq(x, y) (!strcmp((x), (y)))
void init_options (struct options *o);
int Createargcargv(struct options* options, char* command_line);
void parse_argv (struct options* options, int argc, char *argv[]);
static int add_option (struct options *options, int i, char *p[]);
int ConfigFileOptionExist(int config, const char *option);
