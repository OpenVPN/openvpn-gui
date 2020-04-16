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
define([_GUI_VERSION_MINOR], [15])
AC_INIT([OpenVPN GUI],[_GUI_VERSION_MAJOR],[openvpn-devel@lists.sourceforge.net],[openvpn-gui],[https://github.com/openvpn/openvpn-gui/])
AC_DEFINE([PACKAGE_VERSION_RESOURCE], [_GUI_VERSION_MAJOR,_GUI_VERSION_MINOR,0,0], [Version in windows resource format])
AC_DEFINE([PACKAGE_VERSION_RESOURCE_STR], ["_GUI_VERSION_MAJOR._GUI_VERSION_MINOR.0.0"], [Version as a string])
AC_DEFINE([GUI_COPYRIGHT_YEAR_END], ["2018"], [Last copyright year for GUI in About tab])
AC_DEFINE([CORE_COPYRIGHT_YEAR_END], ["2018"], [Last copyright year for daemon in About tab])
AC_CONFIG_AUX_DIR([.])
AM_CONFIG_HEADER([config.h])
AC_CONFIG_SRCDIR([main.h])
AM_INIT_AUTOMAKE
AC_CANONICAL_HOST
AC_USE_SYSTEM_EXTENSIONS
AC_PROG_CC_C99
AC_CHECK_TOOL([WINDRES], [windres])

AC_ARG_ENABLE(
	[distonly],
	[AS_HELP_STRING([--enable-distonly], [enable distribute only mode @<:@default=no@:>@])],
	,
	[enable_distonly="no"]
)

AC_ARG_ENABLE(
	[password-change],
	[AS_HELP_STRING([--disable-password-change], [disable password change support @<:@default=yes@:>@])],
	,
	[enable_password_change="yes"]
)

case "$host" in
	*-mingw*)
		CPPFLAGS="${CPPFLAGS} -DWIN32_LEAN_AND_MEAN"
		CPPFLAGS="${CPPFLAGS} -D_WIN32_WINNT=_WIN32_WINNT_VISTA -DWINVER=_WIN32_WINNT"
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

AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET

if test "${enable_password_change}" = "yes"; then
PKG_PROG_PKG_CONFIG
PKG_CHECK_MODULES(
	[OPENSSL_CRYPTO],
	[libcrypto >= 0.9.6],
	,
	[AC_CHECK_LIB(
		[crypto],
		[RSA_new],
		[OPENSSL_CRYPTO_LIBS="-lcrypto"],
		[test "${enable_distonly}" = "no" && AC_MSG_ERROR([Cannot find OpenSSL library])]
	)]
)
else
	AC_DEFINE([DISABLE_CHANGE_PASSWORD],[1],[disable password change])
fi

test "${GCC}" = "yes" && CFLAGS="${CFLAGS} -pedantic -Wall -Wextra"

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
