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

#ifndef OPENVPN_GUI_RES_H
#define OPENVPN_GUI_RES_H

/* Icons */
#define ID_ICO_APP                      90
#define ID_ICO_CONNECTED                91
#define ID_ICO_CONNECTING               92
#define ID_ICO_DISCONNECTED             93
#define ID_ICO_EYE                      94
#define ID_ICO_EYESTROKE                95

/* About Dialog */
#define ID_DLG_ABOUT                    100
#define ID_ICON_ABOUT                   110
#define ID_LTEXT_ABOUT2                 122
#define ID_LTEXT_ABOUT3                 123
#define ID_LTEXT_ABOUT4                 124

/* Ask for Passphrase Dialog */
#define ID_DLG_PASSPHRASE               150
#define ID_EDT_PASSPHRASE               151
#define ID_LTEXT_PASSWORD               152
#define ID_PASSWORD_REVEAL              153

/* Status Dialog */
#define ID_DLG_STATUS                   160
#define ID_TXT_STATUS                   161
#define ID_EDT_LOG                      162
#define ID_DISCONNECT                   163
#define ID_RESTART                      164
#define ID_HIDE                         165
#define ID_TXT_VERSION                  166
#define ID_DETACH                       167
#define ID_TXT_BYTECOUNT                168
#define ID_TXT_IP                       169

/* Change Passphrase Dialog */
#define ID_DLG_CHGPASS                  170
#define ID_EDT_PASS_CUR                 174
#define ID_EDT_PASS_NEW                 175
#define ID_EDT_PASS_NEW2                176
#define ID_TXT_KEYFILE                  177
#define ID_TXT_KEYFORMAT                178

/* Auth Username/Password Dialog */
#define ID_DLG_AUTH                     180
#define ID_EDT_AUTH_USER                181
#define ID_EDT_AUTH_PASS                182

/* Auth Username/Password/Challenge Dialog */
#define ID_DLG_AUTH_CHALLENGE           183
#define ID_TXT_AUTH_CHALLENGE           184
#define ID_EDT_AUTH_CHALLENGE           185
#define ID_CHK_SAVE_PASS                186
#define ID_TXT_WARNING                  187
#define ID_LTEXT_USERNAME               188
#define ID_LTEXT_RESPONSE               189

/* Challenege Response Dialog */
#define ID_DLG_CHALLENGE_RESPONSE       190
#define ID_TXT_DESCRIPTION              191
#define ID_EDT_RESPONSE                 192

/* Proxy Settings Dialog */
#define ID_DLG_PROXY                    200
#define ID_GROUPBOX1                    201
#define ID_GROUPBOX2                    202
#define ID_GROUPBOX3                    203
#define ID_GROUPBOX4                    204
#define ID_LTEXT_PORT_OFFSET            205
#define ID_GROUPBOX5                    206
#define ID_RB_PROXY_OPENVPN             210
#define ID_RB_PROXY_MSIE                211
#define ID_RB_PROXY_MANUAL              212
#define ID_RB_PROXY_HTTP                213
#define ID_RB_PROXY_SOCKS               219
#define ID_EDT_PROXY_ADDRESS            214
#define ID_EDT_PROXY_PORT               215
#define ID_TXT_PROXY_ADDRESS            216
#define ID_TXT_PROXY_PORT               217

/* General Settings Dialog */
#define ID_DLG_GENERAL                  230
#define ID_CMB_LANGUAGE                 231
#define ID_TXT_LANGUAGE                 232
#define ID_CHK_STARTUP                  233
/* historic: #define ID_CHK_SERVICE_ONLY              234 */
#define ID_TXT_LOG_APPEND               235
#define ID_CHK_LOG_APPEND               236
#define ID_CHK_SILENT                   237
#define ID_TXT_BALLOON                  238
#define ID_RB_BALLOON0                  239
#define ID_RB_BALLOON1                  240
#define ID_RB_BALLOON2                  241
#define ID_CHK_SHOW_SCRIPT_WIN          242
#define ID_CHK_ALWAYS_USE_ISERVICE      243
#define ID_RB_BALLOON3                  244
#define ID_RB_BALLOON4                  245
#define ID_RB_BALLOON5                  246
#define ID_TXT_PERSISTENT               247
#define ID_CHK_PLAP_REG                 248
#define ID_CHK_AUTO_RESTART             249

/* Proxy Auth Dialog */
#define ID_DLG_PROXY_AUTH               250
#define ID_EDT_PROXY_USER               251
#define ID_EDT_PROXY_PASS               252

/* Advanced dialog */
#define ID_DLG_ADVANCED                 270
#define ID_TXT_FOLDER                   271
#define ID_TXT_EXTENSION                272
#define ID_EDT_CONFIG_DIR               274
#define ID_EDT_CONFIG_EXT               275
#define ID_EDT_LOG_DIR                  276
#define ID_BTN_CONFIG_DIR               277
#define ID_BTN_LOG_DIR                  278
#define ID_TXT_PRECONNECT_TIMEOUT       279
#define ID_TXT_CONNECT_TIMEOUT          280
#define ID_TXT_DISCONNECT_TIMEOUT       281
#define ID_EDT_PRECONNECT_TIMEOUT       282
#define ID_EDT_CONNECT_TIMEOUT          283
#define ID_EDT_DISCONNECT_TIMEOUT       284
#define ID_EDT_MGMT_PORT                285
#define ID_TXT_FOLDER1                  286
#define ID_EDT_POPUP_MUTE               287
#define ID_RB_ENGINE_SELECTION          288
#define ID_RB_ENGINE_OVPN2              289
#define ID_RB_ENGINE_OVPN3              290

/* Connections dialog */
#define ID_DLG_CONNECTIONS              290

/* Message Dialog */
#define ID_DLG_MESSAGE                  300
#define ID_TXT_MESSAGE                  301

#define ID_DLG_URL_PROFILE_IMPORT       400
#define ID_EDT_URL                      401
#define ID_CHK_AUTOLOGIN                402
#define IDS_ERR_URL_IMPORT_PROFILE      403

/* pkcs11-id query dialog */
#define ID_DLG_PKCS11_QUERY             450
#define ID_LVW_PKCS11                   451
#define ID_TXT_PKCS11                   452

/* QR code dialog */
#define ID_DLG_QR                       500
#define ID_STATIC_QR                    501
#define ID_TXT_QR                       502

/* General settings contd.. */

#define ID_CHK_CONCAT_OTP               470

/*
 * String Table Resources
 */

/* Tray Tooltips */
#define IDS_TIP_CONNECTED               1001
#define IDS_TIP_CONNECTING              1003
#define IDS_TIP_CONNECTED_SINCE         1004
#define IDS_TIP_ASSIGNED_IP             1005

/* Tray Icon Context Menu */
#define IDS_MENU_SERVICE                1006
#define IDS_MENU_SETTINGS               1007
#define IDS_MENU_CLOSE                  1009
#define IDS_MENU_CONNECT                1010
#define IDS_MENU_DISCONNECT             1011
#define IDS_MENU_STATUS                 1012
#define IDS_MENU_VIEWLOG                1013
#define IDS_MENU_EDITCONFIG             1014
#define IDS_MENU_PASSPHRASE             1015
#define IDS_MENU_SERVICE_START          1016
#define IDS_MENU_SERVICE_STOP           1017
#define IDS_MENU_SERVICE_RESTART        1018
#define IDS_MENU_SERVICEONLY_START      1019
#define IDS_MENU_SERVICEONLY_STOP       1020
#define IDS_MENU_SERVICEONLY_RESTART    1021
#define IDS_MENU_ASK_STOP_SERVICE       1022
#define IDS_MENU_IMPORT                 1023
#define IDS_MENU_CLEARPASS              1024
#define IDS_MENU_RECONNECT              1025
#define IDS_MENU_IMPORT_AS              1026
#define IDS_MENU_IMPORT_FILE            1027
#define IDS_MENU_IMPORT_URL             1028

/* LogViewer Dialog */
#define IDS_ERR_START_LOG_VIEWER        1101
#define IDS_ERR_START_CONF_EDITOR       1102

/* OpenVpn Related */
#define IDS_ERR_MANY_CONFIGS            1201
#define IDS_ERR_ONE_CONN_OLD_VER        1203
#define IDS_ERR_STOP_SERV_OLD_VER       1204
#define IDS_ERR_CREATE_EVENT            1205
#define IDS_ERR_UNKNOWN_PRIORITY        1206
#define IDS_ERR_LOG_APPEND_BOOL         1207
#define IDS_ERR_GET_MSIE_PROXY          1208
#define IDS_ERR_INIT_SEC_DESC           1209
#define IDS_ERR_SET_SEC_DESC_ACL        1210
#define IDS_ERR_CREATE_PIPE_OUTPUT      1211
#define IDS_ERR_CREATE_PIPE_INPUT       1213
#define IDS_ERR_DUP_HANDLE_OUT_READ     1214
#define IDS_ERR_DUP_HANDLE_IN_WRITE     1215
#define IDS_ERR_CREATE_PROCESS          1217
#define IDS_ERR_CREATE_THREAD_STATUS    1219
#define IDS_NFO_STATE_WAIT_TERM         1220
#define IDS_NFO_STATE_CONNECTED         1222
#define IDS_NFO_NOW_CONNECTED           1223
#define IDS_NFO_ASSIGN_IP               1224
#define IDS_ERR_CERT_EXPIRED            1225
#define IDS_ERR_CERT_NOT_YET_VALID      1226
#define IDS_NFO_STATE_RECONNECTING      1227
#define IDS_NFO_STATE_DISCONNECTED      1228
#define IDS_NFO_CONN_TERMINATED         1229
#define IDS_NFO_STATE_FAILED            1230
#define IDS_NFO_CONN_FAILED             1231
#define IDS_NFO_STATE_FAILED_RECONN     1232
#define IDS_NFO_RECONN_FAILED           1233
#define IDS_NFO_STATE_SUSPENDED         1234
#define IDS_ERR_READ_STDOUT_PIPE        1235
#define IDS_ERR_CREATE_EDIT_LOGWINDOW   1236
#define IDS_ERR_SET_SIZE                1237
#define IDS_ERR_AUTOSTART_CONF          1238
#define IDS_ERR_CREATE_PIPE_IN_READ     1240
#define IDS_NFO_STATE_CONNECTING        1241
#define IDS_NFO_CONNECTION_XXX          1242
#define IDS_NFO_STATE_CONN_SCRIPT       1244
#define IDS_NFO_STATE_DISCONN_SCRIPT    1245
#define IDS_ERR_RUN_CONN_SCRIPT         1246
#define IDS_ERR_GET_EXIT_CODE           1247
#define IDS_ERR_CONN_SCRIPT_FAILED      1248
#define IDS_ERR_RUN_CONN_SCRIPT_TIMEOUT 1249
#define IDS_ERR_CONFIG_EXIST            1251
#define IDS_NFO_CONN_TIMEOUT            1252
#define IDS_NFO_NO_CONFIGS              1253
#define IDS_ERR_CONFIG_NOT_AUTHORIZED   1254
#define IDS_ERR_CONFIG_TRY_AUTHORIZE    1255
#define IDS_NFO_CONFIG_AUTH_PENDING     1256
#define IDS_ERR_ADD_USER_TO_ADMIN_GROUP 1257
#define IDS_NFO_BYTECOUNT               1258
#define IDS_NFO_STATE_ONHOLD            1259
#define IDS_ERR_PARSE_MGMT_OPTION       1260
#define IDS_NFO_STATE_CANCELLING        1261
#define IDS_NFO_STATE_RETRYING          1262
#define IDS_NFO_STATE_DISCONNECTING     1263
#define IDS_NFO_CONN_CANCELLED          1264
#define IDS_NFO_STATE_ROUTE_ERROR       1265
#define IDS_NFO_NOTIFY_ROUTE_ERROR      1266
#define IDS_NFO_OTP_PROMPT              1267

/* Program Startup Related */
#define IDS_ERR_OPEN_DEBUG_FILE         1301
#define IDS_ERR_LOAD_RICHED20           1302
#define IDS_ERR_SHELL_DLL_VERSION       1303
/* historic: #define IDS_ERR_GUI_ALREADY_RUNNING     1304 */
#define IDS_NFO_SERVICE_STARTED         1305
#define IDS_NFO_SERVICE_STOPPED         1306
#define IDS_NFO_ACTIVE_CONN_EXIT        1307
#define IDS_NFO_SERVICE_ACTIVE_EXIT     1308
#define IDS_ERR_CREATE_PATH             1309
#define IDS_NFO_CLICK_HERE_TO_START     1310

/* Program Options Related */
#define IDS_NFO_USAGE                   1401
#define IDS_NFO_USAGECAPTION            1402
#define IDS_ERR_BAD_PARAMETER           1403
#define IDS_ERR_BAD_OPTION              1404

/* Change Passphrase Dialog */
#define IDS_ERR_CREATE_PASS_THREAD      1501
#define IDS_NFO_CHANGE_PWD              1502
#define IDS_ERR_PWD_DONT_MATCH          1503
#define IDS_ERR_PWD_TO_SHORT            1504
#define IDS_NFO_EMPTY_PWD               1505
#define IDS_ERR_UNKNOWN_KEYFILE_FORMAT  1506
#define IDS_ERR_OPEN_PRIVATE_KEY_FILE   1507
#define IDS_ERR_OLD_PWD_INCORRECT       1508
#define IDS_ERR_OPEN_WRITE_KEY          1509
#define IDS_ERR_WRITE_NEW_KEY           1510
#define IDS_NFO_PWD_CHANGED             1511
#define IDS_ERR_READ_PKCS12             1512
#define IDS_ERR_CREATE_PKCS12           1513
#define IDS_ERR_OPEN_CONFIG             1514
#define IDS_ERR_ONLY_ONE_KEY_OPTION     1515
#define IDS_ERR_ONLY_KEY_OR_PKCS12      1516
#define IDS_ERR_ONLY_ONE_PKCS12_OPTION  1517
#define IDS_ERR_HAVE_KEY_OR_PKCS12      1518
#define IDS_ERR_KEY_FILENAME_TO_LONG    1519
#define IDS_ERR_PASSPHRASE2STDIN        1520
#define IDS_ERR_CR2STDIN                1521
#define IDS_ERR_AUTH_USERNAME2STDIN     1522
#define IDS_ERR_AUTH_PASSWORD2STDIN     1523
#define IDS_ERR_INVALID_CHARS_IN_PSW    1524

/* Settings Dialog*/
#define IDS_SETTINGS_CAPTION            1550

/* Proxy Settings Dialog */
#define IDS_ERR_HTTP_PROXY_ADDRESS      1601
#define IDS_ERR_HTTP_PROXY_PORT         1602
#define IDS_ERR_HTTP_PROXY_PORT_RANGE   1603
#define IDS_ERR_SOCKS_PROXY_ADDRESS     1604
#define IDS_ERR_SOCKS_PROXY_PORT        1605
#define IDS_ERR_SOCKS_PROXY_PORT_RANGE  1606
#define IDS_ERR_CREATE_REG_HKCU_KEY     1607
#define IDS_ERR_GET_TEMP_PATH           1608

/* General Settings Dialog */
#define IDS_LANGUAGE_NAME               1650

/* Win32 Service Related */
/* historic: #define IDS_ERR_OPEN_SCMGR_ADMIN        1701 */
#define IDS_ERR_OPEN_VPN_SERVICE        1702
#define IDS_ERR_START_SERVICE           1703
#define IDS_ERR_QUERY_SERVICE           1704
#define IDS_ERR_SERVICE_START_FAILED    1705
#define IDS_ERR_OPEN_SCMGR              1706
#define IDS_ERR_STOP_SERVICE            1707
#define IDS_NFO_RESTARTED               1708
#define IDS_ERR_ACCESS_SERVICE_PIPE     1709
#define IDS_ERR_WRITE_SERVICE_PIPE      1710
#define IDS_ERR_NOTSTARTED_ISERVICE     1711
#define IDS_ERR_INSTALL_ISERVICE        1712
#define IDS_ERR_NOTSTARTED_ISERVICE_ADM 1713

/* Registry Related */
#define IDS_ERR_GET_WINDOWS_DIR         1801
#define IDS_ERR_GET_PROGRAM_DIR         1802
#define IDS_ERR_OPEN_REGISTRY           1803
#define IDS_ERR_READING_REGISTRY        1804
#define IDS_ERR_PASSPHRASE_ATTEMPTS     1805
#define IDS_ERR_CONN_SCRIPT_TIMEOUT     1806
#define IDS_ERR_DISCONN_SCRIPT_TIMEOUT  1807
#define IDS_ERR_PRECONN_SCRIPT_TIMEOUT  1808
#define IDS_ERR_CREATE_REG_KEY          1809
#define IDS_ERR_OPEN_WRITE_REG          1810
#define IDS_ERR_READ_SET_KEY            1811
#define IDS_ERR_WRITE_REGVALUE          1812
#define IDS_ERR_GET_PROFILE_DIR         1813
#define IDS_ERR_PLAP_REG                1814
#define IDS_ERR_PLAP_UNREG              1815

/* Importation Related */

#define IDS_ERR_IMPORT_EXISTS           1901
#define IDS_ERR_IMPORT_FAILED           1902
#define IDS_NFO_IMPORT_SUCCESS          1903
#define IDS_NFO_IMPORT_OVERWRITE        1904
#define IDS_ERR_IMPORT_SOURCE           1905
#define IDS_ERR_IMPORT_ACCESS           1906
#define IDS_NFO_IMPORT_SOURCE           1907

/* Save password related messages */
#define IDS_NFO_DELETE_PASS             2001

/* Token password dialog related */
#define IDS_NFO_TOKEN_PASSWORD_CAPTION  2100
#define IDS_NFO_TOKEN_PASSWORD_REQUEST  2101
#define IDS_NFO_AUTO_CONNECT            2102

/* Password retry messages */
#define IDS_NFO_AUTH_PASS_RETRY         2150
#define IDS_NFO_KEY_PASS_RETRY          2151

/* Invalid input errors */
#define IDS_ERR_INVALID_PASSWORD_INPUT  2152
#define IDS_ERR_INVALID_USERNAME_INPUT  2153

/* pkcs11 related */
#define IDS_ERR_NO_PKCS11               2160
#define IDS_ERR_SELECT_PKCS11           2161
#define IDS_CERT_DISPLAYNAME            2162
#define IDS_CERT_ISSUER                 2163
#define IDS_CERT_NOTAFTER               2164

/* openvpn daemon state descriptions */
#define IDS_NFO_OVPN_STATE_INITIAL      2200
#define IDS_NFO_OVPN_STATE_CONNECTING   2201
#define IDS_NFO_OVPN_STATE_ASSIGN_IP    2202
#define IDS_NFO_OVPN_STATE_ADD_ROUTES   2203
#define IDS_NFO_OVPN_STATE_CONNECTED    2204
#define IDS_NFO_OVPN_STATE_RECONNECTING 2205
#define IDS_NFO_OVPN_STATE_EXITING      2206
#define IDS_NFO_OVPN_STATE_WAIT         2207
#define IDS_NFO_OVPN_STATE_AUTH         2208
#define IDS_NFO_OVPN_STATE_GET_CONFIG   2209
#define IDS_NFO_OVPN_STATE_RESOLVE      2210
#define IDS_NFO_OVPN_STATE_TCP_CONNECT  2211
#define IDS_NFO_OVPN_STATE_AUTH_PENDING 2212
#define IDS_NFO_OVPN_STATE_UNKNOWN      2220

/* Timer IDs */
#define IDT_STOP_TIMER                  2500 /* Timer used to trigger force termination */

#endif                                       /* ifndef OPENVPN_GUI_RES_H */
