Version 11.51.0
===============

* Higher resolution eye icons
* Support for concatenating OTP with password
* Optionally always prompt for OTP
* Fix tooltip positioning when the taskbar is at top

Version 11.50.0
===============

* Translation improvements (Italian)

Version 11.49.0
===============

Bugfixes
--------
* Fix crash when password contains space, \ or "

Version 11.48.0
===============

Bugfixes
--------
* Fix truncation of text in tooltip

Version 11.47.0
===============

Bugfixes
--------
* Fix tray icon state change for persistent connections

Version 11.46.0
===============

Version 11.45.0
===============
* Cmake build system updates
* Include full version in IV_GUI_VER

Version 11.44.0
===============
* BUILD.rst documentation updates
* Reformat using uncrustify
* Remove obsolete options from --help output

Version 11.43.0
===============

Version 11.42.0
===============

New features
------------
* Add a password reveal button in dialogs

Bug fixes
---------
* Add back the delay before management interface is ready

Version 11.40.0
===============

Removed feature
--------------
- Private key password change feature no longer supported

Version 11.39.0
===============

Bug fixes
---------
* Make --command disconnect_all work again

Version 11.38.0
===============

Bug fixes
---------
* Fix memory leaks in PLAP and localization.c

Updates
-------
* Replace sleep() by wait with message pump
* Remove tray icon during exit processing
* Use the status window as owner of dialogs in same thread

Version 11.37.0
===============

Bug fixes
---------
* Fix broken tray icon menu with single profile (regression in 11.36.0) (#592)

Version 11.36.0
===============

Updates
-------
* Do not open multiple instances of Settings window
* Do not use assert in debug builds
* Translations (Italian)

Bug fixes
---------
* Use a list instead of array for connection list
* Handle possible wraparound of time in auto-close of dialogs
* Fix missing files in dist tarball
* Check return value of SetProp() (fixes crash reported in github issue #577)

Version 11.35.0
===============

Updates
-------
* Translations (Italian, Chinese-Simplified)
* Notify user if connection completes with route addition errors

Version 11.34.0
===============

New features
------------
* Connections active on exit/logout are now automatically restarted
  in the next session of the GUI

Updates
-------
* Support for WEB_AUTH request from the server
* Persistent connections are now shown in a submenu even if
  nested-menu-view is not active

Bug fixes
---------
* Fix truncated text in German localization of settings dialog
* Fix the issue of management-password truncated at 15 bytes

Version 11.33.0 (2022-12-15)
============================

Updates
-------
* RTL support in message boxes and echo message window
* Target for mingw build changed to _WIN32_WINNT_WIN7
* Translations (Farsi, Chinese simplified)
* Localize daemon state names and PLAP dialog
* Always show persistent connections as a separate group

Bug fixes
---------
* Fix disconnection failure when management interface repeatedly
  tries to connect and fails in some corner cases

Version 11.32.0 (2022-12-02)
============================

New features
------------
* Support for RTL layout (for Farsi) and bidirectional text

Updates
-------
* Translations (Chinese simplified)

Version 11.31.0 (2022-11-07)
============================

Updates
-------
* Update README
* Forget passwords while stopping persistent connections

Version 11.30.0 (2022-11-04)
============================

New features
-----------
* Pre-logon access provider for starting connections from login screen
* Persistent connections: Connections in config-auto folder started by
  automatic service now visible and controllable from the GUI
* Handle pkcs11-id query from daemon
* Support for OpenVPN 3

Updates
-------
* Translations (Chinese simplified)
* Added a config file parser
* Qspectre protection and control flow guard
* Automatically find and use a free port for management interface

Version 11.29.0 (2022-05-31)
============================

Updates
-------
* MSVC and CI build improvements
* Load legacy provider if required
* OpenSSL initializations: set OPENSSL_CONF and OPENSSL_MODULES
* Support for OpenSSL 3 in MSVC builds

Bug fixes
---------
* Username string escape in CRV response

Version 11.27.0 (2021-12-15)
============================

Updates
-------
* Github action improvements
* Allow for longer challenge response text (up to 120 characters)
* Support import as a context menu for .ovpn files
  Facilitates automatic import of downloaded ovpn files
* Check content-deposition when importing from URL

Bug fixes
---------
* Include applink for change password
* Character remapping in filename of imported configs

Version 11.26.0 (2021-10-05)
============================

New features
------------
* Implement importing of profile from a URL
* New "--command import" command line option
* Option to disable echo messages

Updates
-------
* Translations (Japanese)

Bug fixes
---------
* Correctly parse challenge response containing ': character


Version 11.25.0 (2021-06-17)
============================

New features
------------
* Support for CR_TEXT challenge from server
* Support for web-based authentication (OPEN_URL)

Updates
-------
* Make resource files MSVC compliant
* Github actions use cmake instead of msvc project

Version 11.24.0 (2021-04-21)
============================

Updates
-------
* Translations (Polish, Portuguese)
* Remove limit on max number of configs
* Command line option for management_port_offset

Version 11.23.0 (2021-02-24)
============================

New features
------------
* User configurable management_port_offset & menu_view
* Display of echo messages from server
* Indicate profiles in connecting state by a check-mark
* New language: Farsi
* Open all active connection status windows by double-click

Version 11.21.0 (2020-12-09)
============================

Updates
-------
* Update README
* Add logging support for pre/up/down scripts

Version 11.19.0 (2020-09-21)
============================

New features
------------
* Per-monitor DPI scaling support

Updates
-------
* Always use interactive service (even for admin users)
* Allow config directories to nest deep up to 20 levels
* Translations (Dutch, Ukrainian)

Version 11.17.0 (2020-09-01)
============================

Updates
-------
* Startup option now named "Launch on User Logon"

Version 11.16.0 (2020-08-12)
============================

Updates
-------
* Translations (Danish, German)

Bug fixes
---------
* Do not do escape processing of static-challenge response

Version 11.15.0 (2020-04-16)
============================

New features
------------
* Add "--command rescan" to rescan config folders

Updates
-------
* Allow overwriting of profiles during import
* MSVC build support

Bug fixes
---------
* Remove CRLF in base64 output

Version 11.15.0 (2019-10-30)
============================

Updates
-------
* Translations (Finnish)

Version 11.13.0 (2019-04-19)
============================

Updates
-------
* Appveyor/CI improvements
* Translations (Russian)

Version 11.12.0 (2019-02-20)
============================

New features
------------
* Nested config menu display
  User selectable from settings: flat/nested/auto
* Setting of env variables from server: 'echo setenv name var'
* New language: Simplified Chinese

Updates
-------
* Translations (Italian, Korean, Dutch)
* Use a dynamic array for configuration profile list
* Ignore pushed --route-method when using interactive service
* Service-only menu item removed
* Set 'notepad.exe' as the fallback editor
* Do not clear saved passwords on verification failure

Bug fixes
---------
* Display IP address correctly when only IPv6 is assigned
* 'openvpn-gui --help' not to be treated as a running instance
* 'echo save-passwords' should not override 'disable_save_passwords'
  enforced by an Administrator

Version 11.10.0 (2018-03-02)
============================

New features
------------
* Display assigned IPs and connection stats on status window
* Support sending commands to running instance
* Add restart button to connection menus
* Auto submit saved auth-user-pass credentials after a brief delay

Updates
-------
* Translations (German, Russian, French)
* In '--connect profile-name' make the extension (.ovpn) optional
* Treat --connect as --command connect in case GUI is already running
* Allow the GUI to run without any registry keys present using defaults
* Check for invalid characters in user inputs

Bug fixes
---------
* Correct parsing of the process ID returned by interactive service

Version 11.9.0 (2017-09-26)
===========================

New features
------------
* Highlight (color) warning and error messages in status window

Updates
-------
* Translations (French)
* Add instruction how to build using MSYS2

Version 11.8.0 (2017-07-25)
===========================

Updates
-------
* Translations (Ukrainian, Russian)
* Add warning to credential dialogs on retry after auth failure

Version 11.7.0 (2017-06-20)
===========================

Updates
-------
* Translations (German, Finnish)
* Set focus to password field when username is filled
* Close registry keys and service handles after use

Version 11.6.0 (2017-05-12)
===========================

Updates
-------
* Close token handle in GetProcessTokenGroups()
* Translations (Korean)
* Several AppVeyor build improvements

Version 11.5.0 (2017-03-22)
===========================

New features
------------
* Add a system-wide option to disable the password save feature
* Parse ECHO directives from openvpn
  - "echo forget-passwords"
  - "echo save-passwords"
* New language: Czech

Updates
-------
* Translations (French)
* AppVeyor build support
* Readme: add AppVeyor and travis badge
* Check group membership without needing connection to DC
* Update travis-ci
* Target changed to _WIN32_WINNT_VISTA
* Fix truncation of usage message shown with --help
* Enable ASLR and DEP
* Close service pipe in case of startup error
* Update README
* Added Windows Vista/Win7/Win8/Win8.1/Win10 to compatibility manifest
* Suppress warning popups if silent_connection is set
* Translations (Dutch, Chinese-traditional)

Bug fixes
---------
* Do not set status as connected when connection completes with errors

Version 11.4.0 (2016-12-16)
===========================

Updates
-------
* Load icons at sizes given by DPI-dependent system metric
* Add 24x24 and 20x20 versions of each icon.

Version 11.3.0 (2016-12-02)
===========================

Updates
-------
* Translations (Norwegian)

Version 11.2.0 (2016-11-25)
===========================

New features
------------
* Make the program DPI aware

Version 11.1.0 (2016-11-17)
===========================

New features
------------
* Support pkcs11 token insertion request and pin input
* Handle dynamic challenge/response
* Make options saved in registry editable by user
* Use file associations to open config and log
* Save username and optionally passwords
* Add "Launch on startup" setting
* New Windows 8 styled system tray icons.
* Support user and global config directories

Updates
-------
* Translations (Ukrainian, Russian, Italian, Dutch, Portuguese)
* Check for interactive service only if OpenVPN version is >= 2.4
* Update About page
* Do not start a connection when a previous thread has not fully exited
* Force-kill any openvpn processes that fail to stop
* NUL terminate messages received from interactive service
* Improve the message shown when no config files are found
* Remove unused nsis installer
* CI-build: add build with --disable-password-change and other improvements
* Add instructions on how to build openvpn-gui using openvpn-build
* Rename README as README.rst and modernize it
* Rescan configs even when connections are active
* Read errors from the service pipe and handle fatal ones
* Update build instructions
* Handle interactive service policy restrictions
* Remove "Run with highest privilege available"

Bug fixes
---------
* Fix exit handling while in modal loops
* Fix some duplicate resource ids
* Handle empty strings in Base64Encode
* Ensure strings read from registry are null terminated
* Fix wrongly used o.conn[config] in place of current config c
* Fix potential out-of-bounds access

Version 11 (2016-02-22)
=======================

New features
------------
* "Import file" feature

Updates
-------
* Warn if integrative service is not installed or not running
* Updating README build instructions
* Better error reporting when connection fails to come up
* Put --log first in the command line
* Fix the path of notepad.exe
* Change default log file location to a OpenVPN/log in user's profile directory
* Do not use interactive service if running as admin
* cleanup .travis.yml

Version 10(2016-01-04)
======================

Updates
-------
* Support for travis-ci builds


Version 9 (2016-01-04)
======================

Updates
-------
* Run with highest privilege available
* Do not disconnect on suspend
* Convert changes.txt to CHANGES.rst
* Translations (Russian, Ukrainian)

Bug fixes
---------
* Fix errors reported by cppcheck

Version 7 (2015-02-27)
======================

Bug fixes
---------

* Fixed some typo's and spelling errors in Dutch translation.
* Fixed typo in tray tooltip (polish language)

New features
------------

* Update program graphics, thanks to Evgheni Dereveanchin
* Add NSIS installer files Samuli Seppänen

Version 5 (2013-08-05)
======================

Bug fixes
---------

* Fix disconnect happening when closing RDP client

Version 4 (2013-06-03)
======================

Bug fixes
---------

* Fix NULL pointer dereference, closes issue #28
* Don't let menu IDs overlap when MAX_CONFIGS > 100, closes issue #30
* Use UI language set by user for l10n, closes #27
* Make auth popups show when returning from suspend

Version 3 (2013-03-07)
======================

Bug fixes
---------

* Fix spelling, closes community ticket #254
* Fix crash on 64 bit Windows, closes trac bug #247

Version 2 (2012-12-13)
======================

New features
------------

* Added XP theme support to GUI
* Localization support
* Moved proxy settings into a general settings dialog tab
* Support starting OpenVPN via interactive service
* Add Finnish localization by Samuli Seppaenen
* Add Danish localization by Morten Christensen
* Update Swedish localization
* Add Turkish localization by Hakan Darama
* Add Japanese localization by Taro Yamazaki
* Add Chinese (trad.) localization by Yi-Wen Cheng
* Add Russian localization by Roman Azarenko

Bug fixes
---------

* Fix starting a connection with double click on icon
* Fix connection status if only one config exists
* Fix IP address display in tooltip, closes #3176526
* Fix connect script name, closes bug #3213131
* Fix loading of the proxy source from registry
* Make management interface work with Windows 8

Version 1.0.3 (2005-08-18)
==========================

Bug Fixes
---------

There was a bug in the code that expands variables in
registry values. If the expanded string was longer than
the original string it got incorrectly truncated.


Version 1.0.2 (2005-07-27)
==========================

Pass paths read in OpenVPN GUI's registry values through
ExpandEnvironmentStrings(). This allows the use of Windows
variables like %HOMEPATH% or %PROGRAMFILES%. This allows
multiple users on the same system to have their own set
of config files and keys in their home dir.


Version 1.0.1 (2005-06-10)
==========================

Bug Fixes
---------

The Change Password feature did not work correctly when TABs
were used in the config file between the key/pkcs12 keyword and
the accual path to the key file.


Version 1.0 (2005-04-21)
========================

No changes

Version 1.0-rc5 (2005-03-29)
============================

Bug Fixes
---------

[Pre/Dis]Connect scripts were not executed when starting or stopping
the OpenVPN Service, or using "Service Only" mode.


Version 1.0-rc4 (2005-02-17)
============================

Increased the width of buttons and space between text labels and edit
controls on dialogs to ease localization of OpenVPN GUI.

Bug Fixes
---------

Some fixed text strings was introduced in the code in 1.0-rc3. These
are moved to the resource file now to allow localization.

If starting the OpenVPN service failed, OpenVPN GUI would get
stuck with a yellow icon.


Version 1.0-rc3 (2005-02-14)
============================

New Features
------------

New registry value (show_balloon) to control whether to show the
"Connected Balloon" or not. show_ballon can have the following values
  
0=Never show any balloon. 
1=Show balloon when the connection establishes (default).
2=Show balloon every time OpenVPN has reconnected (old behavior).

Show "Connected since: XXX" and "Assigned IP: X.X.X.X" in the tray
icon tip msg.

If a batch file named xxx_pre.bat exists in the config folder, where
xxx is the same name as an OpenVPN config file, this will be executed
before OpenVPN is launced.

If a batch file named xxx_down.bat exists in the config folder, where
xxx is the same name as an OpenVPN config file, this will be executed
on disconnect, but before the OpenVPN tunnel is closed.

Registry value "show_script_window" controls whether _up, _down and
_pre scripts should execute in the background or in a visible cmd-line
window.

Registry value "[pre/dis]connectscript_timeout" controls how long to
wait for each script to finish.

Updated information on the about dialog.

Bug Fixes
---------

Removed unused code that tried to determine the path to "Program 
Files". This code caused an error in some rare occasions.


Version 1.0-rc2 (2005-01-12)
============================

New Features
------------

Support for one level of subdirectories below the config directory.
This means that if you have multiple connections, you can now put
them in a seperate subdirectory together with their keys and certs.

"Service Only" mode. This is a mode that makes OpenVPN GUI more
friendly to use for non-admin users to control the OpenVPN Service.
Enable this mode by setting the registry value "service_only" to "1".

In this mode the following happends:

- The normal "Connect", "Disconnect" and "Show Status" is removed.
- The Service menu items "Start", "Stop" and "Restart" is replaced 
  by "Connect", "Disconnect" and "Reconnect" directly on the main
  menu. These now control the OpenVPN Service instead.
- Dubbleclicking the icon starts the OpenVPN Service.
- Dubbleclicking the icon when the service is running brings up a
  dialog asking if you want to disconnect.
- The Proxy Settings menu item is removed as it can't control the service
  anyway.
- The "OpenVPN Service started" dialog msg is replaced with a balloon msg.
- Ask the user if he really wants to exit OpenVPN GUI if the OpenVPN Service is 
  running.
    
Bug Fixes
---------

Full rights were required to control the OpenVPN Service. Now only
Start/Stop permissions are required, which allows a normal user to
control the OpenVPN Service if these rights are granted to the user.
(Can be done with subinacl.exe from the resource kit)

When passwords were retrieved from a user, OpenVPN GUI received them
in the default windows codepage (ISO 8859-1 on english XP), and this 
was passed on untouched to OpenVPN. When OpenVPN is run from command-
line on the other hand, the old DOS CP850 codepage is used. This
caused passwords containing non-ASCII (7-bit) chars that worked from
cmd-line not to work from OpenVPN GUI. This is now solved by
retrieving passwords in unicode and translate them to CP850 before
supplying them to OpenVPN.

Re-scan the config dir for new files when dubble-clicking the tray
icon.


Version 1.0-rc1 (2005-01-06)
============================

New Features
------------

Show a warning message if "log" or "log-append" is found in the config
file.

Bug Fixes
---------

Added a bunch of compiler warnings which revealed a lot of minor
programming errors. Mostly cast conversion errors between signed and
unsigned intergers. All fixed now.

Set focus on the log window when the status window is re-opened to make
sure the log is scrolled automatically.

Set focus on the log window when clicking disconnect to allow the log
to continue scrolling automatically until OpenVPN is terminated.


Version 1.0-beta26 (2004-12-04)
===============================

New Features
------------

Show "Connecting to: xxx" msg in tray icon tip message in addition to
the previously displayed "Connected to:" msg.

Bug Fixes
---------

Don't ask if you are sure you want to change your password to an EMPTY
password if you're not allowed to use passwords shorter than 8 chars.

Clear password buffers after use to avoid having passwords in memory.

  
Version 1.0-beta25 (2004-12-01)
===============================

Changed button labels on the status dialog from DisConnect and ReConnect
to Disconnect and Reconnect.

Don't show "Assigned IP:" at all in the connected balloon if no IP
address is known, as when a real remote DHCP server is used.

Stripped out the last hardcoded strings to the resource file.

Raised maximum number of configs from 20 to 50.

Bug Fixes
---------

If OpenVPN printed a line longer that 1024 chars, OpenVPN GUI would crash.
This could happend when using "verb 5" or higher as OpenVPN then prints
an "r" or "w" for every packet without any line breaks. A new line will
now be inserted when 1024 chars is reached.

Ask if you want to close active connections when WM_CLOSE is received.

Handle WM_QUERYENDSESSION and WM_ENDSESSION correctly by closing any
active connections and then terminate.

Version 1.0-beta24 (2004-11-15)
===============================

Bug Fixes
---------

Some openssl #includes were not #ifdef:ed when building a nochangepsw
version causing the build to fail if the openssl headers were not
available.

When using OpenVPN 1.5/1.6 and entering a false private key passphrase,
OpenVPN GUI would falsely think that the user attempted to start another
connection.

Version 1.0-beta23 (2004-11-08)
===============================

Bug Fixes
---------

Passphrase protected keys stored in MS Certificate Store did not work
due to the way the openvpn console window was hidden.

Version 1.0-beta22 (2004-11-05)
===============================

Bug Fixes
---------

OpenVPN GUI did not pass a CR/LF correctly after supplying OpenVPN
with the private key passphrase! OpenVPN 2.0-beta12 and higher which
uses a new prompt worked, but not earlier versions of OpenVPN.

If the Shell (explorer.exe) is restarted, OpenVPN GUI did not
re-register the tray icon.


Version 1.0-beta21 (2004-10-29)
===============================

New Features
------------

Added support for username/password based authentication.

Support for Localization. Language have to chosen at build time.
Available are english, german, czech and swedish.

Bug Fixes
---------

Fixed crash after displaying that too many connections exist.

Removed duplicate length-check on setting new password.

Fixed error dialog which had the error message shown in window caption.

Status windows did not change to yellow icon while ReConnecting.

DisConnect and ReConnect button was not disabled after a termination.
This bug was introduced with beta20.

The Change Password feature did not parse the key/pkcs12 line in the
config file correctly if there was TABs after the filename.

The Change Password feature did not work if a relative path with
subdirectories was used.

Version 1.0-beta20 (2004-10-18)
===============================

New Features
------------

Accept the new passphrase prompt introduced with OpenVPN 2.0-beta12.

When the machine is about to enter suspend mode the connection is
closed. When the machine is powered up again, the connection is
re-established.
  
Registry option "disconnect_on_suspend". Set to zero to disable the
above feature. 

ReConnect button on the status dialog.

Registry option "allow_proxy" to hide the Proxy Settings menu item.

Registry option "silent_connection" that suppresses the status
dialog from being showed while connecting.

Command-line option to set the time to wait for the connect script
to finish.

Icon color now reflects the status of the OpenVPN Service.

Bug Fixes
---------

Included shellapi.h with the sourcecode, as the one distributed with
the current stable version of MinGW miss some definitions.

When closing OpenVPN GUI it waits for all connections to close before
exiting (Max 5 sec).

Made the password dialog always be on top of other windows.

Fixed a bug that occured if opening the log file for writing failed.
(which happends if you try to run OpenVPN GUI without admin rights)

The menuitems on the OpenVPN Service menu was incorrectly enabled/
disabled. This bug was introduced with beta19 as a result of the
dynamic rescanning for configs on every menu opening.

Starting OpenVPN GUI with OpenVPN 1.5/1.6 installed and OpenVPN
Service running failed with previous versions. (CreateEvent() error)

The installation package did not remove the OpenVPN-GUI registry key
on uninstall.

Removed dependency on libeay32.dll for the no change password build.

Version 1.0-beta19 (2004-09-22)
===============================

New Features
------------

The menu is restructured. Previous versions had all "actions" on the
main menu, and a submenu with all configs for every action. This version
lists all configs on the main menu, and have a submenu with actions.

If only one config exist, the actions are placed on the main menu.

If no connection is running, the config dir is re-scanned for configs
every time the menu is opened.

If a file exists in the config folder named xxxx_up.bat, where xxxx
is the same name as an existing config file, this batch file will be
executed after a connection has been establish. If the batch file
fails (return an exitcode other than 0), an error message is displayed.

Auto-hide status window after a connection is established and show
a systray info balloon instead.

Show assigned IP address in connected balloon.

Don't allow starting multiple instances of OpenVPN GUI.

Added a cancel button to the Ask Password dialog.

Bug Fixes
---------

Removed [nopass] parameter on --connect option as the password prompt
is only showed if the private key really is passphrase protected.

Show an error msg if --connect refers to a non existing config file.

Ignore case of config file extension.

Version 1.0-beta18 (2004-09-13)
===============================

New Features
------------

New Icons! Supplied by Radek Hladik.

If only one config file exists, dubble-clicking the systray icon will
start that connection.

Bug Fixes
---------

A bug in the GetRegKey() function caused OpenVPN GUI sometimes to
fail starting with the following error msg:
Error creating exit_event when checking openvpn version.


Version 1.0-beta17 (2004-09-02)
===============================

New Features
------------

A dialog to configure Proxy Settings. You can now set http-proxy or
socks-proxy address and port from the GUI. You can also make the GUI
ask for proxy username and password, which will then be supplied to
OpenVPN via an auth file.

Use Internet Explorer Proxy Settings (Ewan Bhamrah Harley)
  
A "Hide" button on the status dialog.

Show an error message if the client certificate has expired or is not
yet valid.

Bug Fixes
---------

If OpenVPN was installed in a non default folder, OpenVPN GUI would try
to locate openvpn.exe, log-dir and conf-dir in the default openvpn
folder anyway. Fixed in this version.

OpenVPN GUI tried to check the status of the OpenVPN Service even
if the service menu was disabled in the registry, which caused an
error message to be showed if the service was not installed properly.

Wait for two seconds when exiting OpenVPN GUI, so running openvpn
processes can exit cleanly.

Disable Disconnect menu item while waiting for an openvpn process
to terminate.

Version 1.0-beta16 (2004-08-25)
===============================

Bug Fixes
---------

When only a filename (no full path) was specified in the config file
for --key or --pkcs12, OpenVPN GUI did not look for the file in the
config dir when changing password. Fixed in this version.

Version 1.0-beta15 (2004-08-25)
===============================

When changing password, require new password to be at least 8 chars.

Version 1.0-beta14 (2004-08-24)
===============================

New Features
------------

Change password of the private key. Both PEM and PKCS #12 files
are supported.

Version 1.0-beta13 (2004-08-19)
===============================

New Features
------------

Shows which connections are connected in the TrayIcon tip msg

Bug Fixes
---------

The "Enter Passphrase" dialog was a bit miss-designed. The textlabel
and the editbox was overlapping a few pixels which made it look a
little strange in some occasions.

Version 1.0-beta12 (2004-08-16)
===============================

New Features
------------

Show a Status Window while connecting that shows the output from
OpenVPN in real-time.

A new menuitem to show the real-time status window.

If only one connection is running, dubbleclicking the trayicon will
show the status window for the running connection.

Show a yellow TrayIcon while connecting.

Detect "restarting process" message, and shows "Connecting" status
until a new connected msg is received.

Version 1.0-beta11a (2004-08-15)
================================

Bug Fixes
---------

The exit_event handle was not closed after checking the openvpn version
which made it impossible to restart connections with OpenVPN versions 
lower than 2.0-beta6. You received the following msg when trying to
connect a second time:

"I seem to be running as a service, but my exit event object is telling me to exit immediately"

This bug was introduced with OpenVPN GUI v1.0-beta10.

Version 1.0-beta11 (2004-08-09)
===============================

New Features
------------

This version is bundled with a patched version of openvpn that will
output a log message AFTER routes have been added to the system. This
allows the GUI to report "Connected" after this msg. This patch will
be included in next official release of OpenVPN 2.0-beta, so the GUI
will continue to work with future official releases of openvpn. Older
versions of openvpn will still work with this version of OpenVPN GUI,
but "Connected" will then be reported before routes are added as it
did with OpenVPN GUI 1.0-beta10.

If wrong passphrase is entered, openvpn will automatically be restarted
a specified nr of times (default 3), which allows the user to re-enter
his passphrase.

Number of passphase attempts to allow can be specified with reg-key 
"passphrase_attempts" or cmd-line option with the same name.

Bug Fixes
---------

An empty line was printed in the log when prompting for passphrase.
 
Version 1.0-beta10 (2004-08-08)
===============================

Default registry setting for showing the "Edit Config" menuitem is
changed to "1" (Show it). If a previous version of OpenVPN GUI has
been used, the registry key will of cource not change without manually
changing it.

New Features
------------

Check version of openvpn.exe, so it can support all versions of OpenVPN
without a special build of OpenVPN GUI. Tested with 1.5.0, 1.6.0,
2.0-beta4, 2.0-beta7 and 2.0-beta10. Older versions than 2.0-beta6 still
only support one simultaneous connection though. 

Redirect StdIn/StdOut/StdErr through OpenVPN GUI, so we can pass the
private key passphrase to openvpn without requiring a patched version
of OpenVPN. This also allows OpenVPN GUI to prompt for a passphrase only
when it's needed.

If connecting fails, ask the user if he wants to view the log.

Show a dialog while connecting to allow the user to abort the connection.

Bug Fixes
---------

Disable both "Connect" and "DisConnect" while connecting.

Version 1.0-beta9 (2004-07-23)
==============================

The passphrase support added in v1.5-beta1 has been merched into the v1.0
source so v1.5 does not exist any longer!

New Features
------------

Cmd-line options: 
::

   --connect cnn [nopass]: Autoconnect to "cnn" at startup. If "nopass"
                           is used, no passphrase will be asked for.

   --help                : Show list of cmd-line options.

And all registry settings is now available as cmd-line options:
::

   --exe_path            : Path to openvpn.exe.\n"
   --config_dir          : Path to dir to search for config files in.\n"
   --ext_string          : Extension on config files.\n"
   --log_dir             : Path to dir where log files will be saved.\n"
   --priority_string     : Priority string (See install.txt for more info).\n"
   --append_string       : 1=Append to log file. 0=Truncate logfile.\n"
   --log_viewer          : Path to log viewer.\n"
   --editor              : Path to config editor.\n"
   --allow_edit          : 1=Show Edit Config menu\n"
   --allow_service       : 1=Show Service control menu\n"

Bug Fixes
---------

If the GUI was started from a cmd prompt and no passphrase was given
openvpn.exe would query the user for the passphrase from the console
(which is not showed), so the openvpn process got stuck there.


Version 1.5-beta1 (2004-07-16)
==============================

This version is based on v1.0-beta8.

v1.5 is just a temporary version in wait for the management interface
to OpenVPN. When this is available features added in v1.5 will be
rewritten to use this interface instead in v2.0 of OpenVPN-GUI.

New Features
------------

Support for passphrase protected private keys. OpenVPN-GUI will now
always query the user for a passphrase before connecting. The
passphrase is then supplied to OpenVPN via the --passphrase option.
This requires a patched version of OpenVPN that supports the
--passphrase option. A patched version that supports this is included
in the OpenVPN-GUI v1.5-betaX installation package.

The user will always be asked for a passphrase even if the private
key is not encrypted. This is because the GUI does not know in advance
if the key is encrypted or not. This will be fixed in v2.0 when we
have the management interface ready.


Version v1.0-beta8 (2004-07-16)
===============================

New Features
------------

Tray Icon now shows red/green if any connection is established.

Bug Fixes
---------

If something failed before starting openvpn.exe, exit_event and
log_handle was not closed correctly which could make it impossible
to make any more connections without restarting OpenVPN-GUI.

Version 1.0-beta7 (2004-07-08)
==============================

New Features
------------

A seperate build version supporting OpenVPN v1.5, v1.6 and the
2.0 series before beta6. This version only supports having one
connection running at the same time.

Added an About box.

If there are active connections when "Exit OpenVPN-GUI" is selected,
a "Are you sure you want to exit?" box is displayed.

Bug Fixes
---------

It was not possible to have cmd-line options on the reg-keys
"log_viewer" or "editor". This is now possible.

Version 1.0-beta6 (2004-07-05)
==============================

Bug Fixes
---------

The default values for paths created by beta3, beta4 and beta5 used
hardcoded values for "C:\windows..." and "C:\program files...", which
did not work on some localized Windows versions that is not using
these folders. This is fixed now by getting those pathnames from the
system.

If you have installed beta3-beta5 you need to manualy delete the
whole HKEY_LM\SOFTWARE\OpenVPN-GUI key in the registry. The correct
reg-keys will then be recreated when OpenVPN-GUI is started.

Version 1.0-beta5 (2004-07-04)
==============================

New Features
------------

Menu-commands to Start/Stop/Restart the OpenVPN Service. Enable this
feature by setting the following reg-key to 1:
HKEY_LM\SOFTWARE\OpenVPN-GUI\allow_service

Bug Fixes
---------

v1.0-beta4 always opened the registry with write-access, which made
it imposible to start it without administator rights.

Version 1.0-beta4 (2004-07-04)
==============================

New Features
------------

Menu-command to open a config-file for editing. Enable this feature
by setting the following reg-key to 1: 
HKEY_LM\SOFTWARE\OpenVPN-GUI\allow_edit
 
Version 1.0-beta3 (2004-07-04)
==============================

New Features
------------

Log Viewer. As default OpenVPN-GUI launches Notepad to view the log.
The program used to view the log can be changed with this reg-key:
HKEY_LM\SOFTWARE\OpenVPN-GUI\log-viewer

OpenVPN-GUI now uses its own registry-keys, instead of the same as
the service wrapper uses. It now stores its values under this key:
HKEY_LM\SOFTWARE\OpenVPN-GUI\
If this key does not exist, OpenVPN-GUI will create it with the same
default values as the service-wrapper uses, so if you want to use the
service-wrapper on config-files indepentent of the GUI you should
change the "config-dir" key to another folder.

Version 1.0-beta2 (2004-07-03)
==============================

New Features
------------

Connect/Disconnect now shows a sub-menu so each connection can be 
brought up/down individually.

Upon connect OpenVPN-GUI will wait for 3 seconds and then check if
the openvpn process is still alive and report "Connection successful"
only if this is the case.

OpenVPN-GUI monitors the openvpn processes it has started, and if a
process is terminated before the user has chosen to take it down, this
will be reported to the user.

If no config files is found when OpenVPN-GUI is started, it will
notify the user of this and terminate.

Version 1.0-beta1 (2004-07-02)
==============================

Initial release

Features
--------

Adds itself as a system tray icon.

Menuitem "Connect" - Starts openvpn for all config-files it has found.

