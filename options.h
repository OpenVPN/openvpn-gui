/*
 *  OpenVPN-GUI -- A Windows GUI for OpenVPN.
 *
 *  Copyright (C) 2004 Mathias Sundman <mathias@nilings.se>
 *                2010 Heiko Hund <heikoh@users.sf.net>
 *                2016 Selva Nair <selva.nair@gmail.com>
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
#include <lmcons.h>

#include "manage.h"

#define MAX_NAME (UNLEN + 1)

/*
 * Maximum number of parameters associated with an option,
 * including the option name itself.
 */
#define MAX_PARMS           5   /* Max number of parameters per option */
/* Menu ids are constructed as 12 bits for config number, 4 bits for action */
#define MAX_CONFIGS         (1<<12)

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

/* connection states */
typedef enum {
    disconnected,
    connecting,
    reconnecting,
    connected,
    disconnecting,
    suspending,
    suspended,
    resuming,
    timedout
} conn_state_t;

/* Interactive Service IO parameters */
typedef struct {
    OVERLAPPED o; /* This has to be the first element */
    HANDLE pipe;
    HANDLE hEvent;
    WCHAR readbuf[512];
} service_io_t;

#define FLAG_ALLOW_CHANGE_PASSPHRASE (1<<1)
#define FLAG_SAVE_KEY_PASS  (1<<4)
#define FLAG_SAVE_AUTH_PASS (1<<5)
#define FLAG_DISABLE_SAVE_PASS (1<<6)

#define CONFIG_VIEW_AUTO      (0)
#define CONFIG_VIEW_FLAT      (1)
#define CONFIG_VIEW_NESTED    (2)

typedef struct {
    unsigned short major, minor, build, revision;
} version_t;

/* A node of config groups tree that can be navigated from the end
 * node (where config file is attached) to the root. The nodes are stored
 * as array (o.groups[]) with each node linked to its parent.
 * Not a complete tree: only navigation from child to parent is supported
 * which is enough for our purposes.
 */
typedef struct config_group {
    int id;                      /* A unique id for the group >= 0*/
    wchar_t name[40];            /* Name of the group -- possibly truncated */
    int parent;                  /* Id of parent group. -1 implies no parent */
    BOOL active;                 /* Displayed in the menu if true -- used to prune empty groups */
    int children;                /* Number of children groups and configs */
    int pos;                     /* Index within the parent group -- used for rendering */
    HMENU menu;                  /* Handle to menu entry for this group */
} config_group_t;

/* short hand for pointer to the group a config belongs to */
#define CONFIG_GROUP(c) (&o.groups[(c)->group])
#define PARENT_GROUP(cg) ((cg)->parent < 0 ? NULL : &o.groups[(cg)->parent])

/* Connections parameters */
struct connection {
    TCHAR config_file[MAX_PATH];    /* Name of the config file */
    TCHAR config_name[MAX_PATH];    /* Name of the connection */
    TCHAR config_dir[MAX_PATH];     /* Path to this configs dir */
    TCHAR log_path[MAX_PATH];       /* Path to Logfile */
    TCHAR ip[16];                   /* Assigned IP address for this connection */
    TCHAR ipv6[46];                 /* Assigned IPv6 address */
    BOOL auto_connect;              /* AutoConnect at startup id TRUE */
    conn_state_t state;             /* State the connection currently is in */
    int failed_psw_attempts;        /* # of failed attempts entering password(s) */
    int failed_auth_attempts;       /* # of failed user-auth attempts */
    time_t connected_since;         /* Time when the connection was established */
    proxy_t proxy_type;             /* Set during querying proxy credentials */
    int group;                      /* ID of the group this config belongs to */
    int pos;                        /* Index of the config within its group */

    struct {
        SOCKET sk;
        SOCKADDR_IN skaddr;
        time_t timeout;
        char password[16];
        char *saved_data;
        size_t saved_size;
        mgmt_cmd_t *cmd_queue;
        BOOL connected;             /* True, if management interface has connected */
    } manage;

    HANDLE hProcess;                /* Handle of openvpn process if directly started */
    service_io_t iserv;

    HANDLE exit_event;
    DWORD threadId;
    HWND hwndStatus;
    int flags;
    char *dynamic_cr;              /* Pointer to buffer for dynamic challenge string received */
    unsigned long long int bytes_in;
    unsigned long long int bytes_out;
    struct env_item *es;           /* Pointer to the head of config-specific env variables list */
};

/* All options used within OpenVPN GUI */
typedef struct {
    /* Array of configs to autostart */
    const TCHAR **auto_connect;

    /* Connection parameters */
    connection_t *conn;               /* Array of connection structure */
    config_group_t *groups;           /* Array of nodes defining the config groups tree */
    int num_configs;                  /* Number of configs */
    int num_auto_connect;             /* Number of auto-connect configs */
    int num_groups;                   /* Number of config groups */
    int max_configs;                  /* Current capacity of conn array */
    int max_auto_connect;             /* Current capacity of auto_connect array */
    int max_groups;                   /* Current capacity of groups array */

    service_state_t service_state;    /* State of the OpenVPN Service */

    /* Proxy Settings */
    proxy_source_t proxy_source;      /* Where to get proxy information from */
    proxy_t proxy_type;               /* The type of proxy to use */
    TCHAR proxy_http_address[100];    /* HTTP Proxy Address */
    TCHAR proxy_http_port[6];         /* HTTP Proxy Port */
    TCHAR proxy_socks_address[100];   /* SOCKS Proxy Address */
    TCHAR proxy_socks_port[6];        /* SOCKS Proxy Address */

    /* HKLM Registry values */
    TCHAR exe_path[MAX_PATH];
    TCHAR global_config_dir[MAX_PATH];
    TCHAR priority_string[64];
    TCHAR ovpn_admin_group[MAX_NAME];
    DWORD disable_save_passwords;
    /* HKCU registry values */
    TCHAR config_dir[MAX_PATH];
    TCHAR ext_string[16];
    TCHAR log_dir[MAX_PATH];
    DWORD log_append;
    TCHAR log_viewer[MAX_PATH];
    TCHAR editor[MAX_PATH];
    DWORD silent_connection;
    DWORD service_only;
    DWORD show_balloon;
    DWORD show_script_window;
    DWORD connectscript_timeout;        /* Connect Script execution timeout (sec) */
    DWORD disconnectscript_timeout;     /* Disconnect Script execution timeout (sec) */
    DWORD preconnectscript_timeout;     /* Preconnect Script execution timeout (sec) */
    DWORD config_menu_view;             /* 0 for auto, 1 for original flat menu, 2 for hierarchical */

#ifdef DEBUG
    FILE *debug_fp;
#endif

    HWND hWnd;
    HINSTANCE hInstance;
    BOOL session_locked;
    HANDLE netcmd_semaphore;
    version_t version;
    char ovpn_version[16]; /* OpenVPN version string: 2.3.12, 2.4_alpha2 etc.. */
    unsigned int dpi_scale;
    COLORREF clr_warning;
    COLORREF clr_error;
    int action;            /* action to send to a running instance */
    TCHAR *action_arg;
    HANDLE session_semaphore;
    HANDLE event_log;
} options_t;

void InitOptions(options_t *);
void ProcessCommandLine(options_t *, TCHAR *);
int CountConnState(conn_state_t);
connection_t* GetConnByManagement(SOCKET);
connection_t* GetConnByName(const WCHAR *config_name);
INT_PTR CALLBACK ScriptSettingsDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK ConnectionSettingsDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK AdvancedSettingsDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
void DisableSavePasswords(connection_t *);

void ExpandOptions(void);
int CompareStringExpanded(const WCHAR *str1, const WCHAR *str2);
#endif
