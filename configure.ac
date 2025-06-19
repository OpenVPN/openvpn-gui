dnl  OpenVPN-GUI -- A Windows GUI for OpenVPN.
dnl
dnl  Copyright (C) 2004 Mathias Sundman <mathias@nilings.se>
dnl                2010 Heiko Hund <heikoh@users.sf.net>
dnl
dnl  This program is free software; you can redistribute it and/or modify
dnl  it under the terms of the GNU General Public License as published by
dnl  the Free Software Foundation; either version 2 of the License, or
dnl  (at your option) any later version.
dnl
dnl  This program is distributed in the hope that it will be useful,
dnl  but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl  GNU General Public License for more details.
dnl
dnl  You should have received a copy of the GNU General Public License
dnl  along with this program (see the file COPYING included with this
dnl  distribution); if not, write to the Free Software Foundation, Inc.,
dnl  59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

AC_PREREQ(2.59)
define([_GUI_VERSION_MAJOR], [11])
define([_GUI_VERSION_MINOR], [54])
AC_INIT([OpenVPN GUI],[_GUI_VERSION_MAJOR],[openvpn-devel@lists.sourceforge.net],[openvpn-gui],[https://github.com/openvpn/openvpn-gui/])
AC_DEFINE([PACKAGE_VERSION_RESOURCE], [_GUI_VERSION_MAJOR,_GUI_VERSION_MINOR,0,0], [Version in windows resource format])
AC_DEFINE([PACKAGE_VERSION_RESOURCE_STR], ["_GUI_VERSION_MAJOR._GUI_VERSION_MINOR.0.0"], [Version as a string])
AC_CONFIG_AUX_DIR([.])
AM_CONFIG_HEADER([config.h])
AC_CONFIG_SRCDIR([main.h])
AM_INIT_AUTOMAKE([subdir-objects])
AC_CANONICAL_HOST
AC_USE_SYSTEM_EXTENSIONS
AC_PROG_CC_C99
AC_PROG_CXX
AC_CHECK_TOOL([WINDRES], [windres])
LT_INIT([win32-dll])
LT_LANG([Windows Resource])

AC_ARG_ENABLE(
	[distonly],
	[AS_HELP_STRING([--enable-distonly], [enable distribute only mode @<:@default=no@:>@])],
	,
	[enable_distonly="no"]
)

AC_ARG_ENABLE(
	[ovpn3],
	[AS_HELP_STRING([--enable-ovpn3], [enable openvpn3 agent and client @<:@default=no@:>@])],
	[
		AC_DEFINE([ENABLE_OVPN3], 1, [Use openvpn3 agent instead of interactive service])
		PKG_CHECK_MODULES([JSON], [json-c])
	]
)

case "$host" in
	*-mingw*)
		CPPFLAGS="${CPPFLAGS} -DWIN32_LEAN_AND_MEAN"
		CPPFLAGS="${CPPFLAGS} -D_WIN32_WINNT=_WIN32_WINNT_WIN7 -DWINVER=_WIN32_WINNT"
		LDFLAGS="${LDFLAGS} -Wl,--nxcompat,--dynamicbase"

		dnl older mingw doesn't support `--high-entropy-va`
		_save_ldflags="$LDFLAGS"
		LDFLAGS="-Wl,--high-entropy-va"
		AC_LINK_IFELSE([AC_LANG_PROGRAM([])],[AS_VAR_SET(high_entropy_aslr,[yes])],)
		LDFLAGS="$_save_ldflags"
		AS_VAR_IF(high_entropy_aslr,[yes],[LDFLAGS="${LDFLAGS} -Wl,--high-entropy-va"],)
		;;
	*)
		test "${enable_distonly}" = "no" && AC_MSG_ERROR([This project is supported for windows only.])
		;;
esac

AC_CHECK_LIB(cryptui, CryptUIDlgViewContext)

AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET

test "${GCC}" = "yes" && CFLAGS="${CFLAGS} -pedantic -Wall -Wextra -Wno-cast-function-type -Werror"

AC_CONFIG_FILES([Makefile plap/Makefile])
AC_OUTPUT
