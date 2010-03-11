AC_PREREQ([2.53])
AC_INIT([openvpn-gui], [1.0.3], [openvpn-devel@lists.sourceforge.net])
AC_CONFIG_SRCDIR([chartable.h])
AC_CONFIG_HEADERS([config.h])

AC_PROG_CC_C99

# Build Win32 native binaries
CFLAGS="$CFLAGS -mno-cygwin"
CPPFLAGS="$CPPFLAGS -mno-cygwin"
LDFLAGS="$LDFLAGS -mno-cygwin -mwindows"

AC_PATH_PROG([WINDRES], [windres])
AC_PATH_PROG([ZIP], [zip])

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
