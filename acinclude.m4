AC_DEFUN([AX_ASSERT_LIB], [
	AC_CHECK_HEADER([$2], , [AC_MSG_FAILURE([$1 library headers could not be found.])], [$5])
	AC_MSG_CHECKING([if $1 library is available])
	LIBS="-l$1 $LIBS"
	AC_TRY_LINK(
		[$5
		 #include <$2>], [$3], [AC_MSG_RESULT([yes])],
		[AC_MSG_RESULT([no]); AC_MSG_FAILURE([$4])]
	)
])


AC_DEFUN([AX_SEARCH_LIB], [
	_ldflags="$LDFLAGS"
	_cppflags="$CPPFLAGS"
	_libs="$LIBS"

	AC_ARG_WITH(m4_translit([$1], [_], [-]),
		[AS_HELP_STRING([--with-]m4_translit([$1], [_], [-])[=DIR],
		                [search for $1 files in DIR/lib and DIR/include])],
		[dnl
			if test -d $withval
			then
				LDFLAGS="-L$withval/lib $_ldflags"
				CPPFLAGS="-I$withval/include $_cppflags"
				AC_SUBST(m4_translit([$1], [a-z-], [A-Z_])[_LDFLAGS], [-L$withval/lib])
				AC_SUBST(m4_translit([$1], [a-z-], [A-Z_])[_CPPFLAGS], [-I$withval/include])
			else
				AC_MSG_ERROR([$withval: No such directory])
			fi
		]
	)

	AC_ARG_WITH(m4_translit([$1], [_], [-])[-lib],
		[AS_HELP_STRING([--with-]m4_translit([$1], [_], [-])[-lib=DIR],
		                [search for $1 library in DIR])],
		[dnl
			if test -d $withval
			then
				LDFLAGS="-L$withval $_ldflags"
				AC_SUBST(m4_translit([$1], [a-z-], [A-Z_])[_LDFLAGS], [-L$withval])
			else
				AC_MSG_ERROR([$withval: No such directory])
			fi
		]
	)

	AC_ARG_WITH(m4_translit([$1], [_], [-])[-includes],
		[AS_HELP_STRING([--with-]m4_translit([$1], [_], [-])[-includes=DIR],
		                [search for $1 library header files in DIR])],
		[dnl
			if test -d $withval
			then
				CPPFLAGS="-I$withval $_cppflags"
				AC_SUBST(m4_translit([$1], [a-z-], [A-Z_])[_CPPFLAGS], [-I$withval])
			else
				AC_MSG_ERROR([$withval: No such directory])
			fi
		]
	)

	AC_CHECK_HEADER([$4], , [AC_MSG_FAILURE([$1 library headers could not be found. You may want to specify a search path using `--with-]m4_translit([$1], [_], [-])[-includes'.])])

	_result=no
	for lib in $1 $2; do
		AC_MSG_CHECKING([if $lib library is available])
		LIBS="-l$lib $3 $_libs"
		AC_TRY_LINK([#include <$4>], [$5], [
			AC_SUBST(m4_translit([$1], [a-z-], [A-Z_])[_LIBS], [-l$lib])
			AC_MSG_RESULT([yes])
			_result=yes
			break
			], [AC_MSG_RESULT([no])])
	done
	if test "$_result" = "no"; then
		AC_MSG_FAILURE([$1 library could not be found. You may want to specify a search path using `--with-]m4_translit([$1], [_], [-])[[[-lib]]'.])
	fi

	CPPFLAGS="$_cppflags"
	LDFLAGS="$_ldflags"
	LIBS="$_libs"

	_result=
	_cppflags=
	_ldflags=
	_libs=
])