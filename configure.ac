
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

AC_CANONICAL_HOST
case $host in
*-cygwin)
    # Build a native Windows binary
    CFLAGS="$CFLAGS -mno-cygwin"
    CPPFLAGS="$CPPFLAGS -mno-cygwin"
    LDFLAGS="$LDFLAGS -mno-cygwin"
    ;;
esac
LDFLAGS="$LDFLAGS -mwindows"

AC_CHECK_TOOL([WINDRES], [windres])

AC_CHECK_HEADER([windows.h], ,
	[AC_MSG_FAILURE([Target system must be Windows])])

AX_ASSERT_LIB([wininet], [wininet.h],
	[DWORD s; InternetQueryOption(NULL, INTERNET_OPTION_PROXY, NULL, &s)],
	[Make sure you're building with a recent WIN API.], [#include <windows.h>])

AX_ASSERT_LIB([comctl32], [prsht.h],
    [PROPSHEETHEADER psh; PropertySheet(&psh)],
    [Make sure you're building with a recent WIN API.], [#include <windows.h>])

AX_ASSERT_LIB([ws2_32], [winsock2.h],
    [WSADATA wsaData; WSAStartup(MAKEWORD(2, 2), &wsaData)],
    [Make sure you're building with a recent WIN API.], [#include <windows.h>])

AX_SEARCH_LIB([crypto], [cryptoeay32 eay32 crypto], [-lgdi32],
	[openssl/pkcs12.h], [EVP_PKEY *k = EVP_PKEY_new()])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
