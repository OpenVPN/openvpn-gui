/*
 *  OpenVPN-GUI -- A Windows GUI for OpenVPN.
 *
 *  Copyright (C) 2004 Mathias Sundman <mathias@nilings.se>
 *                2010 Heiko Hund <heikoh@users.sf.net>
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

#ifndef OPTIONS_H
#define OPTIONS_H

typedef struct connection connection_t;

#include <windows.h>
#include <stdio.h>
#include <time.h>

#include "manage.h"

/*
 * Maximum number of parameters associated with an option,
 * including the option name itself.
 */
#define MAX_PARMS           5   /* May number of parameters per option */
#define MAX_CONFIGS         50  /* Max number of config-files to read */
#define MAX_CONFIG_SUBDIRS  50  /* Max number of subdirs to scan for configs */


/* connection states */
typedef enum {
    disconnected,
    connecting,
    reconnecting,
    connected,
    disconnecting,
    suspending,
    suspended
} conn_state_t;

/* Connections parameters */
struct connection {
    TCHAR config_file[MAX_PATH];    /* Name of the config file */
    TCHAR config_name[MAX_PATH];    /* Name of the connection */
    TCHAR config_dir[MAX_PATH];     /* Path to this configs dir */
    TCHAR log_path[MAX_PATH];       /* Path to Logfile */
    TCHAR ip[16];                   /* Assigned IP address for this connection */
    BOOL auto_connect;              /* AutoConnect at startup id TRUE */
    conn_state_t state;             /* State the connection currently is in */
    int failed_psw_attempts;        /* # of failed attempts entering password(s) */
    time_t connected_since;         /* Time when the connection was established */

    struct {
        SOCKET sk;
        u_short port;
        char password[16];
        mgmt_cmd_t *cmd_queue;
    } manage;

    HANDLE hProcess;
    HANDLE exit_event;
    DWORD threadId;
    HWND hwndStatus;
};


typedef enum {
    service_noaccess     = -1,
    service_disconnected =  0,
    service_connecting   =  1,
    service_connected    =  2
} service_state_t;

typedef enum {
    config,
    windows,
    manual
} proxy_source_t;

typedef enum {
    http,
    socks
} proxy_t;

/* All options used within OpenVPN GUI */
typedef struct {
    /* Array of configs to autostart */
    const TCHAR *auto_connect[MAX_CONFIGS];

    /* Connection parameters */
    connection_t conn[MAX_CONFIGS];   /* Connection structure */
    int num_configs;                  /* Number of configs */

    service_state_t service_state;    /* State of the OpenVPN Service */
    int psw_attempts;                 /* Number of psw attemps to allow */
    int connectscript_timeout;        /* Connect Script execution timeout (sec) */
    int disconnectscript_timeout;     /* Disconnect Script execution timeout (sec) */
    int preconnectscript_timeout;     /* Preconnect Script execution timeout (sec) */

    /* Proxy Settings */
    proxy_source_t proxy_source;      /* Where to get proxy information from */
    proxy_t proxy_type;               /* The type of proxy to use */
    TCHAR proxy_http_address[100];    /* HTTP Proxy Address */
    TCHAR proxy_http_port[6];         /* HTTP Proxy Port */
    TCHAR proxy_socks_address[100];   /* SOCKS Proxy Address */
    TCHAR proxy_socks_port[6];        /* SOCKS Proxy Address */

    /* Registry values */
    TCHAR exe_path[MAX_PATH];
    TCHAR config_dir[MAX_PATH];
    TCHAR ext_string[16];
    TCHAR log_dir[MAX_PATH];
    TCHAR priority_string[64];
    TCHAR append_string[2];
    TCHAR log_viewer[MAX_PATH];
    TCHAR editor[MAX_PATH];
    TCHAR allow_edit[2];
    TCHAR allow_service[2];
    TCHAR allow_password[2];
    TCHAR allow_proxy[2];
    TCHAR silent_connection[2];
    TCHAR service_only[2];
    TCHAR show_balloon[2];
    TCHAR show_script_window[2];
    TCHAR psw_attempts_string[2];
    TCHAR disconnect_on_suspend[2];
    TCHAR connectscript_timeout_string[4];
    TCHAR disconnectscript_timeout_string[4];
    TCHAR preconnectscript_timeout_string[4];

#ifdef DEBUG
    FILE *debug_fp;
#endif

    HWND hWnd;
    HINSTANCE hInstance;
} options_t;

void InitOptions(options_t *);
void ProcessCommandLine(options_t *, TCHAR *);
int CountConnState(conn_state_t);
connection_t* GetConnByManagement(SOCKET);
#endif
