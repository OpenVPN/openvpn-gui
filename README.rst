Installation Instructions for OpenVPN GUI for Windows
#####################################################

OpenVPN-GUI has been bundled with OpenVPN installers for a long time, so there
is rarely a need to install it separately. Bleeding-edge
versions of OpenVPN-GUI are available in `OpenVPN snapshot
installers <http://build.openvpn.net/downloads/snapshots/>`_ based on Git master
branch. OpenVPN-GUI gets installed by default in all OpenVPN installers.

Installation using the official OpenVPN installers
**************************************************

* Download an `OpenVPN installer <https://openvpn.net/index.php/open-source.html>`_
* If you have a previous version of OpenVPN GUI running, shut it down.
  Make sure it's closed by ALL logged on users.

* Run the OpenVPN installer

Manual installation of OpenVPN GUI
**********************************

* `Download <https://openvpn.net/index.php/download/community-downloads.html>`_
  and install OpenVPN

* Download OpenVPN GUI of your choice and save it in OpenVPN's bin folder.
  Default is *C:\\Program Files\\OpenVPN\\bin\\*. You must put it in this folder
  because OpenVPN GUI depends on the OpenSSL DLLs installed in this folder by
  OpenVPN.

Configuring OpenVPN GUI to start on Windows logon
*************************************************

OpenVPN GUI can be configured to start automatically on logon to Windows from
its setting menu.

Adding an OpenVPN configuration file
************************************

To launch a VPN connections using OpenVPN GUI you need to add an OpenVPN
configuration file with .ovpn suffix. Any text editor (e.g. notepad.exe) can be
used to create a OpenVPN configuration files. Note that *log* and *log-append*
options are ignored as OpenVPN GUI redirects the normal output to a log file
itself. There are sample config files in the *sample-config* folder. Please
refer to the `OpenVPN project homepage <https://openvpn.net>`_ for more
information regarding creating the configuration file.

Once the configuration file is ready, you need to let OpenVPN GUI know about it.
There are three ways to do this:

* Place the file into the system-wide location, usually
  *C:\\Program Files\\OpenVPN\\config\\*, or any of its immediate
  subdirectories. This VPN connection will be visible for all users of the
  system.
* Place the file into *C:\\Users\\username\\OpenVPN\\config\\*, or any of its
  immediated subdirectories. The configuration file is only visible for the
  user in question.
* Use the "Import file" function in OpenVPN GUI itself

Using OpenVPN GUI
#################

When OpenVPN GUI is started your OpenVPN config folder
(*C:\\Program Files\\OpenVPN\\config*) will be scanned for .ovpn files and the
OpenVPN GUI icon will appear in the system tray. Each OpenVPN configuration 
file shows up as a separate menu item in the OpenVPN GUI tray, allowing you to
selectively connect to and disconnect to your VPNs. The config dir will be
re-scanned for new config files every time you open the OpenVPN GUI menu by
right-clicking the icon.

When you choose to connect to a site OpenVPN GUI will launch openvpn with
the specified config file. If you use a passphrase protected key you will be
prompted for the passphrase.

If you want OpenVPN GUI to start a connection automatically when it's started,
you can use the --connect cmd-line option. You have to include the extention
for the config file. Example::

    openvpn-gui --connect office.ovpn

To get help with OpenVPN GUI please use one of the official `OpenVPN support
channels <https://community.openvpn.net/openvpn/wiki/GettingHelp>`_.

Running OpenVPN GUI as a Non-Admin user
***************************************

OpenVPN 2.3.x and earlier bundle an OpenVPN GUI version (< 11) which has to be
run as admin for two reasons

* OpenVPN GUI registry keys are stored in system-wide location
  under HKEY_LOCAL_MACHINE, and they are generated when OpenVPN GUI was
  launched the first time
* OpenVPN itself requires admin-level privileges to modify network settings

OpenVPN GUI 11 and later can make full use of the Interactive Service
functionality in recent versions of OpenVPN. This changes a number of
things:

* OpenVPN GUI can store its settings in user-specific part of the registry under
  HKEY_CURRENT_USER
* OpenVPN is able to delegate certain privileged operations, such as adding
  routes, to the Interactive service, removing the need to run OpenVPN with
  admin privileges. Note that for this to work the *OpenVPNServiceInteractive*
  system service has to be enabled and running.

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


Registry Values affecting the OpenVPN GUI operation
***************************************************

All OpenVPN GUI registry values are located below the
*HKEY_LOCAL_MACHINE\\SOFTWARE\\OpenVPN-GUI\\* key

The follow keys are used to control the OpenVPN GUI

config_dir
    the system-wide configuration file directory, defaults to
    *C:\\Program Files\\OpenVPN\\config*; the user-specific configuration file
    directory is hardcoded to *C:\\Users\\username\\OpenVPN\\config**.

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

exe_path
    path to openvpn.exe, defaults to *C:\\Program Files\\OpenVPN\\bin\\openvpn.exe*

log_dir
    log file directory, defaults to *C:\\Program Files\\OpenVPN\\log*

log_append
    if set to "0", the log file will be truncated every time you start a
    connection. If set to "1", the log will be appended to the log file.
  
priority
    the windows priority class for each instantiated OpenVPN process, 
    can be one of:

    * IDLE_PRIORITY_CLASS
    * BELOW_NORMAL_PRIORITY_CLASS
    * NORMAL_PRIORITY_CLASS (default)
    * ABOVE_NORMAL_PRIORITY_CLASS
    * HIGH_PRIORITY_CLASS

allow_edit
    If set to "1", the Edit config menu will be showed.

allow_password
    If set to "1", the Change Password menu will be showed.

allow_proxy
    If set to "1", the Proxy Settings menu will be showed.

allow_service
    If set to "1", the Service control menu will be showed.

silent_connection
    If set to "1", the status window with the OpenVPN log output will
    not be showed while connecting.

service_only
    If set to "1", OpenVPN GUI's normal "Connect" and "Disconnect"
    actions are changed so they start/stop the OpenVPN service instead
    of launching openvpn.exe directly.

show_balloon
    0: Never show any connected balloon

    1: Show balloon after initial connection is established

    2: Show balloon even after re-connects

log_viewer
    The program used to view your log files, defaults to
    *C:\\Windows\\System32\\notepad.exe*

editor
    The program used to edit your config files, defaults to
    *C:\\Windows\\System32\\notepad.exe*

passphrase_attempts
    Number of attempts to enter the passphrase to allow. 

All these registry options is also available as cmd-line options.
Use "openvpn-gui --help" for more info about cmd-line options.

Building OpenVPN GUI from source
################################

See `BUILD.rst <BUILD.rst>`_ for build instructions.
