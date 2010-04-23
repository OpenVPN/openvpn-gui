
define(MAJOR_VERSION,  1)
define(MINOR_VERSION,  0)
define(RELEASE,        3)

AC_PREREQ([2.53])
AC_INIT([OpenVPN GUI], [MAJOR_VERSION.MINOR_VERSION.RELEASE],
	[openvpn-devel@lists.sourceforge.net], [openvpn-gui],
	[http://openvpn-gui.sf.net])
AC_CONFIG_SRCDIR([chartable.h])
AC_CONFIG_HEADERS([config.h])

AC_DEFINE(PACKAGE_MAJOR_VERSION, [MAJOR_VERSION],
	[Define to the package major version.])
AC_DEFINE(PACKAGE_MINOR_VERSION, [MINOR_VERSION],
	[Define to the package minor version.])
AC_DEFINE(PACKAGE_RELEASE, [RELEASE],
	[Define to the package release number.])

AC_PROG_CC_C99

AC_ARG_ENABLE([unicode],
	AS_HELP_STRING([--disable-unicode],
		[produce a binary without Unicode support]), [],
	[CPPFLAGS="$CPPFLAGS -DUNICODE -D_UNICODE"])

# Build Win32 native binaries
CFLAGS="$CFLAGS -mno-cygwin"
CPPFLAGS="$CPPFLAGS -mno-cygwin"
LDFLAGS="$LDFLAGS -mno-cygwin -mwindows"

AC_PATH_PROG([WINDRES], [windres])

AC_CHECK_HEADER([windows.h], ,
	[AC_MSG_FAILURE([Target system must be Win32])])

AX_ASSERT_LIB([wininet], [wininet.h],
	[DWORD s; InternetQueryOption(NULL, INTERNET_OPTION_PROXY, NULL, &s)],
	[Make sure you have a recent w32api installed.])

AX_ASSERT_LIB([comctl32], [prsht.h],
	[PROPSHEETHEADER psh; PropertySheet(&psh)],
	[Make sure you have a recent w32api installed.], [#include <windows.h>])

AX_SEARCH_LIB([crypto], [cryptoeay32 eay32 crypto], [-lgdi32],
	[openssl/pkcs12.h], [EVP_PKEY *k = EVP_PKEY_new()])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
