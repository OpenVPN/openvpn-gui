OpenVPN GUI
#####################################################
.. image:: https://travis-ci.org/OpenVPN/openvpn-gui.svg?branch=master
  :target: https://travis-ci.org/OpenVPN/openvpn-gui
  :alt: TravisCI status
.. image:: https://ci.appveyor.com/api/projects/status/github/OpenVPN/openvpn-gui?branch=master&svg=true
  :target: https://ci.appveyor.com/project/mattock/openvpn-gui
  :alt: AppVeyor status

Installation Instructions for OpenVPN GUI for Windows
#####################################################

OpenVPN-GUI has been bundled with OpenVPN installers for a long time, so there
is rarely a need to install it separately. Bleeding-edge
versions of OpenVPN-GUI are available in `OpenVPN snapshot
installers <http://build.openvpn.net/downloads/snapshots/>`_ based on Git master
branch. OpenVPN-GUI gets installed by default in all OpenVPN installers.

Installation using the official OpenVPN installers
**************************************************

* Download an `OpenVPN installer <https://openvpn.net/index.php/download/community-downloads.html>`_
* If you have a previous version of OpenVPN GUI running, shut it down.
  Make sure it's closed by ALL logged on users.

* Run the OpenVPN installer

Manual installation of OpenVPN GUI
**********************************

* First install OpenVPN using an official installer as described above.

* Build your own version of OpenVPN GUI from source.  See `BUILD.rst <BUILD.rst>`_
  for build instructions. From the build tree copy *openvpn-gui.exe*,
  *libopenvpn_plap.dll*, *openvpn-plap-install.reg* and *openvpn-plap-uninstall.reg*
  to OpenVPN's bin folder.
  Default is *C:\\Program Files\\OpenVPN\\bin\\*. You must put it in this folder
  because OpenVPN GUI depends on the OpenSSL DLLs installed in this folder by
  OpenVPN.

Configuring OpenVPN GUI to start on Windows logon
*************************************************

OpenVPN GUI can be configured to start automatically on logon to Windows from
its setting menu. This is default behavior for all users if OpenVPN GUI was
installed by an OpenVPN 2.4 installer using default installer options.

Adding an OpenVPN configuration file
************************************

To launch a VPN connections using OpenVPN GUI you need to add an OpenVPN
configuration file with .ovpn suffix. Any text editor (e.g. notepad.exe) can be
used to create a OpenVPN configuration files. Note that *log* and *log-append*
options are ignored as OpenVPN GUI redirects the normal output to a log file
itself. There are sample config files in the *sample-config* folder. Please
refer to the `OpenVPN How To <https://openvpn.net/community-resources/how-to/#creating-configuration-files-for-server-and-clients>`_ for more
information regarding creating the configuration file.

Once the configuration file is ready, you need to let OpenVPN GUI know about it.
There are three ways to do this:

* Place the file into the system-wide location, usually
  *C:\\Program Files\\OpenVPN\\config\\*, or any of its immediate
  subdirectories. This VPN connection will be visible for all users of the
  system.
* Place the file into *C:\\Users\\username\\OpenVPN\\config\\*, or any of its
  immediate subdirectories. The configuration file is only visible for the
  user in question. If the user is not a member of the built-in "Administrators"
  group or "OpenVPN Administrators" group and tries to launch such a connection,
  OpenVPN GUI pops up a UAC, offering to create the latter group (if missing)
  and to add the user to it. This will only work if admin-level credentials are
  available.
* Use the "Import file" function in OpenVPN GUI itself
* See the section on `Persistent or Pre-started connections`_
  for how to use OpenVPN GUI to conftrol configurations in
  *C:\\Program Files\\OpenVPN\\config-auto\\* that are started by the automatic service.

Using OpenVPN GUI
#################

When OpenVPN GUI is started your OpenVPN config folders
(*C:\\Users\\username\\OpenVPN\\config* and
*C:\\Program Files\\OpenVPN\\config*) will be scanned for .ovpn files and the
OpenVPN GUI icon will appear in the system tray. Each OpenVPN configuration 
file shows up as a separate menu item in the OpenVPN GUI tray, allowing you to
selectively connect to and disconnect to your VPNs. The config dir will be
re-scanned for new config files every time you open the OpenVPN GUI menu by
right-clicking the icon.

When you choose to connect to a site OpenVPN GUI will launch openvpn with
the specified config file. If you use a passphrase protected key you will be
prompted for the passphrase.

If you want OpenVPN GUI to start a connection automatically when it's started,
you can use the --connect cmd-line option. The extension of the config file
may be optionally included. Example::

    openvpn-gui --connect office.ovpn
    OR
    openvpn-gui --connect office

Starting version 11.34, any connections active when OpenVPN GUI exits or the user
logs out are automatically reconnected when OpenVPN GUI is restarted.

To get help with OpenVPN GUI please use one of the official `OpenVPN support
channels <https://community.openvpn.net/openvpn/wiki/GettingHelp>`_.

Running OpenVPN GUI
*******************

Run OpenVPN-GUI as normal user by double clicking on the icon. No
administrative privileges or `runas-administrator` options are required.
It just works as limited user with the help of Interactive Service which
is enabled by default.

Persistent or Pre-started connections
*************************************

Starting release 2.5.8 (GUI version 11.30), OpenVPN GUI can
control connections started by the "automatic service"
(OpenVPNService) --- also referred to as persistent connections.
OpenVPNService, if running, starts all connection profiles
listed in the `config-auto` directory in the installation path.

By default, such connections are scanned for, and attempt is
made to attach to their management interfaces if available.
User can then view the status of these connections, and disconnect,
reconnect, detach or re-attach them using the menu items.

It requires that such connections be started with
`--management 127.0.0.1 port [pw-file]` option in their config file.
`pw-file` containing a password is optional, but highly recommended.
The password should be a single line of text, preferably shorter than 128
characters of plain ascii.
Further, if `--auth-user-pass` or any such options requiring
interactive user input are present, the config file must also
contain `--management-query-passwords`.

This feature may be controlled by changing the `Persistent Connections`
setting in the `General` tab of the `Settings` menu: choose `auto`
for the default behaviour described above, `manual` to enumerate
and list such connections but not auto-attach, or `disable` to not scan
auto-started connection profiles.

Interactively starting connections before logon
***********************************************

Starting release 2.6 (GUI version 11.30), "Start Before Logon"
aka Pre-Logon Access Provider is supported. This feature is not
enabled by default. It may be enabled during installation or
through the `General Settings` menu of the GUI via the
`Start Before Logon` check mark.

Only `persistent connection` profiles described in the previous
section will be listed on the login screen. Ensure that
connections that may be interactively started from login
screen are setup in the `config-auto` folder, `OpenVPNService`
is running, and these connections are visible and controllable
from the GUI.

In addition to that, PLAP profiles must contain::

    setenv IV_SSO webauth

to indicate that client supports web-based (as QR code for PLAP)
authentication method.

Once those pre-requisites are satisfied, the login screen will
display an icon for `Pre-Logon Access Providers`, clicking which
will bring up a list of OpenVPN connection profiles available, and
allow connecting or disconnecting them interactively.

Run Connect/Disconnect/Preconnect Scripts
*****************************************

There are three different scripts that OpenVPN GUI can execute to help
with different tasks like mapping network drives.

Preconnect  If a file named "xxx_pre.bat" exist in the config folder
            where xxx is the same as your OpenVPN config file name,
            this will be executed BEFORE the OpenVPN tunnel is established.

Connect     If a file named "xxx_up.bat" exist in the config folder
            where xxx is the same as your OpenVPN config file name,
            this will be executed AFTER the OpenVPN tunnel is established.

Disconnect  If a file named "xxx_down.bat" exist in the config folder
            where xxx is the same as your OpenVPN config file name,
            this will be executed BEFORE the OpenVPN tunnel is closed.

The outputs of these scripts are redirected to "xxx_pre.log",
"xxx_up.log" and "xxx_down.log" respectively. These log
files are created in the ``log_dir`` and over-written during
each evocation.

Send Commands to a Running Instance of OpenVPN GUI
**************************************************

When an instance of the GUI is running, certain commands may be sent to
it using the command line interface using the following syntax::

    openvpn-gui.exe --command *cmd* [*args*]

Currently supported *cmds* are

connect ``config-name``
     Connect the configuration named *config-name* (excluding the
     extension .ovpn). If already connected, show the status window.

disconnect ``config-name``
     Disconnect the configuration named *config-name* if connected.

reconnect ``config-name``
     Disconnect and then reconnect the configuration named *config-name*
     if connected.

disconnect\_all
     Disconnect all active connections.

silent\_connection 0 \| 1
     Set the silent connection flag on (1) or off (0)

exit
     Disconnect all active connections and terminate the GUI process

rescan
     Rescan the config folders for changes

import ``path``
     Import the config file pointed to by ``path``.

If no running instance of the GUI is found, these commands do nothing
except for *--command connect config-name* which gets interpreted
as *--connect config-name*

Registry Values affecting the OpenVPN GUI operation
***************************************************

Parameters taken from the global registry values in
*HKEY_LOCAL_MACHINE\\SOFTWARE\\OpenVPN\\* key

(Default)
    The installation directory of openvpn (e.g., *C:\\Program Files\\OpenVPN*).
    This value must be present.

config_dir
    The global configuration file directory. Defaults to
    *C:\\Program Files\\OpenVPN\\config*

exe_path
    path to openvpn.exe, defaults to *C:\\Program Files\\OpenVPN\\bin\\openvpn.exe*

priority
    the windows priority class for each instantiated OpenVPN process,
    can be one of:

    * IDLE_PRIORITY_CLASS
    * BELOW_NORMAL_PRIORITY_CLASS
    * NORMAL_PRIORITY_CLASS (default)
    * ABOVE_NORMAL_PRIORITY_CLASS
    * HIGH_PRIORITY_CLASS

ovpn_admin_group
    The windows group whose membership allows the user to start any configuration file
    in their profile (not just those installed by the administrator in the global
    config directory). Default: "OpenVPN Administrators".

disable_save_passwords
    Set to a nonzero value to disable the password save feature.
    Default: 0

auto_config_dir
    If persistent connection support is enabled and the so-called automatic
    service (OpenVPNService) is running, any config files in this folder are
    scanned and listed in the list of connection profiles.

User Preferences
****************

All other OpenVPN GUI registry values are located below the
*HKEY_CURRENT_USER\\SOFTWARE\\OpenVPN-GUI\\* key. In a fresh
installation none of these values are present and are not
required for the operation of the program. These keys are only
used for persisting user's preferences, and the key names
and their values are subject to change.

The user is not expected to edit any of these values directly.
Instead, edit all preferences using the settings menu.

config_dir
    The user-specific configuration file directory: defaults to
    *C:\\Users\\username\\OpenVPN\\config*.
    The GUI parses this directory for configuration files before
    parsing the global config_dir.

config_ext
    file extension on configuration files, defaults to *ovpn*

connectscript_timeout
    Time in seconds to wait for the connect script to finish. If set to 0
    the exitcode of the script is not checked.

disconnectscript_timeout
    Time in seconds to wait for the disconnect script to finish. Must be a
    value between 1-99.

preconnectscript_timeout
    Time in seconds to wait for the preconnect script to finish. Must be a
    value between 1-99.

log_dir
    log file directory, defaults to *C:\\Users\\username\\OpenVPN\\log*

log_append
    if set to "0", the log file will be truncated every time you start a
    connection. If set to "1", the log will be appended to the log file.

silent_connection
    If set to "1", the status window with the OpenVPN log output will
    not be shown while connecting. Warnings such as interactive service
    not started or multiple config files with same name are also suppressed.

show_balloon
    0: Never show any connected balloon

    1: Show balloon after initial connection is established

    2: Show balloon even after re-connects

config_menu_view
    0: Use a hierarchical (nested) display of config menu reflecting the directory sturcture of config files if the number of configs exceed 25, else use a flat display

    1: Force flat menu

    2: Force nested menu

disable_popup_messages
    If set to 1 echo messages are ignored

popup_mute_interval
    Amount of time in hours for which repeated echo messages are not displayed.
    Defaults to 24 hours.

management_port_offset
    The management interface port is chosen as this offset plus a connection specific index.
    Allowed values: 1 to 61000, defaults to 25340.

All of these registry options are also available as cmd-line options.
Use "openvpn-gui --help" for more info about cmd-line options.

Building OpenVPN GUI from source
################################

See `BUILD.rst <BUILD.rst>`_ for build instructions.
