Installation Instructions for OpenVPN GUI for Windows
-----------------------------------------------------

You can either get my installation package for OpenVPN 2.0.X where I've
bundled the gui in the installation package, or you can use the original
installation package from OpenVPN, and then manually install OpenVPN GUI.


Installation using the bundled OpenVPN package with OpenVPN GUI included
------------------------------------------------------------------------

* Download openvpn-2.0.X-gui-1.0.X-install.exe from 
  http://openvpn.se

* If you have a previous version of OpenVPN GUI installed, shut it down.
  Make sure it's closed by ALL logged on users.

* Run the install program. During the installation you can choose if the GUI
  should be started automatically at system startup. The default is yes.

* Create a xxxx.ovpn config-file with your favorite texteditor and save it in:
  C:\Program files\OpenVPN\config\. You should NOT use the "log" or "log-append"
  options as OpenVPN GUI redirect the normal output to a log file itself. 
  There is a sample config files in the "sample-config" folder. Please
  refer to the OpenVPN project homepage for more information regarding 
  creating the configuration file. http://openvpn.net/


Manual installation of OpenVPN GUI
----------------------------------

* Download and install OpenVPN from http://openvpn.net/

* Download openvpn-gui-1.0.X.exe and save it in OpenVPN's bin folder.
  Default is "C:\Program Files\OpenVPN\bin\". You must put it in this folder
  because OpenVPN GUI depends on the OpenSSL DLLs installed in this folder by
  OpenVPN.

* Create a xxxx.ovpn config-file with your favorite texteditor and save it in:
  C:\Program files\OpenVPN\config\. You should NOT use the "log" or "log-append"
  options as OpenVPN GUI redirect the normal output to a log file itself. 
  There is a sample config files in the "sample-config" folder. Please
  refer to the OpenVPN project homepage for more information regarding 
  creating the configuration file. http://openvpn.net/

* Put a short-cut to openvpn-gui-1.0-X.exe in your 
  "Start->All Program->StartUp" folder if you want the gui started automatically
  when you logon to Windows.

* Start the GUI by double-clicking the openvpn-gui-1.0.X.exe file.

*** You need to be Administrator the first time you run OpenVPN GUI for it to
    create its registry keys. After that you don't have to be administrator
    just to run the GUI, however OpenVPN requires the user to be
    administrator to run! ***


Using OpenVPN GUI
-----------------

When OpenVPN GUI is started your config folder (C:\Program Files\OpenVPN\config)
will be scanned for .ovpn files, and an icon will be displayed in the taskbar's
status area.

If you do not have any openvpn connection running, the config dir will be
re-scanned for new config files every time you open the OpenVPN GUI menu by
right-clicking the icon.

When you choose to connect to a site OpenVPN GUI will launch openvpn with
the specified config file. If you use a passphrase protected key you will be
prompted for the passphrase.

If you want OpenVPN GUI to start a connection automatically when it's started,
you can use the --connect cmd-line option. You have to include the extention
for the config file. Example:

openvpn-gui --connect office.ovpn


Run OpenVPN GUI as a Non-Admin user
-----------------------------------

OpenVPN currently does not work as a normal (non-admin) user. OpenVPN GUI
2.0 will solve this by using an enhanced version of the OpenVPN service
to start and stop openvpn processes.

In the mean time, it is possible to use OpenVPN GUI to control the current
OpenVPN Service to start and stop a connection.

To use OpenVPN GUI to control the OpenVPN service, set the registry value
"service_only" to '1'. See the section about registry values below.

Limitations with this way:
  
  There is no way for OpenVPN GUI ta hand over a password to the service
  wrapper, so you can't use passphrase protected private keys or 
  username/password authentication.

  If you have multiple openvpn configurations, all will be started and
  stopped at the same time.

  OpenVPN GUI is not able to retrieve any status info about the connections
  from OpenVPN, so it will report connected as soon as the service is
  started regarless of if OpenVPN has really succeded to connect or not.

  You cannot see the OpenVPN log in real-time.


Run Connect/Disconnect/Preconnect Scripts
-----------------------------------------

There are three diffrent scripts that OpenVPN GUI can execute to help
with diffrent tasks like mapping network drives.

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
---------------------------------------------------

All OpenVPN GUI reg-values are located below the following key:
HKEY_LOCAL_MACHINE\SOFTWARE\OpenVPN-GUI\

The follow keys are used to control the OpenVPN GUI

config_dir
    configuration file directory, defaults to "C:\Program Files\OpenVPN\config"

config_ext
    file extension on configuration files, defaults to "ovpn"

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
    path to openvpn.exe, defaults to "C:\Program Files\OpenVPN\bin\openvpn.exe"

log_dir
    log file directory, defaults to "C:\Program Files\OpenVPN\log"

log_append
    if set to "0", the log file will be truncated every time you start a
    connection. If set to "1", the log will be appended to the log file.
  
priority
    the windows priority class for each instantiated OpenVPN process, 
    can be one of:

        * "IDLE_PRIORITY_CLASS"
        * "BELOW_NORMAL_PRIORITY_CLASS"
        * "NORMAL_PRIORITY_CLASS" (default)
        * "ABOVE_NORMAL_PRIORITY_CLASS"
        * "HIGH_PRIORITY_CLASS"

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
    If set to "0" - Never show any connected balloon.
              "1" - Show balloon after initial connection is established.
              "2" - Show balloon even after re-connects.
log_viewer
    The program used to view your log files, defaults to
    "C:\windows\notepad.exe"

editor
    The program used to edit your config files, defaults to
    "C:\windows\notepad.exe"

passphrase_attempts
    Number of attempts to enter the passphrase to allow. 

All these registry options is also available as cmd-line options.
Use "openvpn-gui --help" for more info about cmd-line options.


If you have any problem getting OpenVPN GUI to work you can reach me via
email at mathias@nilings.se.


Building OpenVPN GUI from source
--------------------------------

* Download and install MinGW and MSYS from http://www.mingw.org/
  I'm using MinGW-3.2.0-rc-3 and MSYS-1.0.10.

* Download and install the binary distribution of OpenSSL from
  http://www.slproweb.com/products/Win32OpenSSL.html

* Download and extract the OpenVPN GUI source archive.

* Start a bash shell by running msys.bat.

* Run "make" from the OpenVPN GUI source directory. 
