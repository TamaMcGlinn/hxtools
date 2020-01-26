AC_INIT([hxtools], [20200126])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_HEADERS([config.h])
AC_PROG_INSTALL
AM_INIT_AUTOMAKE([-Wall foreign subdir-objects tar-pax no-dist-gzip dist-xz])
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CXX

regular_CPPFLAGS="-D_FILE_OFFSET_BITS=64 -D_REENTRANT"
regular_CFLAGS="-Wall -Waggregate-return -Wmissing-declarations \
	-Wmissing-prototypes -Wredundant-decls -Wshadow -Wstrict-prototypes \
	-Winline -pipe -std=gnu11"
regular_CXXFLAGS="-Wall -Wno-pointer-arith -Wshadow -pipe -std=gnu++11"
AC_SUBST([regular_CPPFLAGS])
AC_SUBST([regular_CFLAGS])
AC_SUBST([regular_CXXFLAGS])

AC_CHECK_HEADERS([lastlog.h paths.h])
AH_TEMPLATE([HAVE_LIBMOUNT])
AH_TEMPLATE([HAVE_LIBPCI])
AH_TEMPLATE([HAVE_LIBXCB])
PKG_CHECK_MODULES([libHX], [libHX >= 3.17])
PKG_CHECK_MODULES([libmount], [mount >= 2.19], [AC_DEFINE([HAVE_LIBMOUNT], [1])], [true])
PKG_CHECK_MODULES([libpci], [libpci >= 3], [AC_DEFINE([HAVE_LIBPCI], [1])], [true])
PKG_CHECK_MODULES([libxcb], [xcb >= 1], [AC_DEFINE([HAVE_LIBXCB], [1])], [true])
AC_SEARCH_LIBS([dlopen], [dl], [libdl_LIBS="$LIBS"; LIBS=""])
AC_SUBST([libdl_LIBS])
AC_CHECK_FUNCS([splice])
AC_CHECK_MEMBERS([struct utmpx.ut_session])

AC_LANG_PUSH([C++])
saved_CXXFLAGS="$CXXFLAGS"
CXXFLAGS="$CXXFLAGS $regular_CXXFLAGS"
AC_CHECK_HEADERS([xbrz.h])
AM_CONDITIONAL([HAVE_XBRZ], [test "$ac_cv_header_xbrz_h" = yes])
CXXFLAGS="$saved_CXXFLAGS"
AC_LANG_POP([C++])

kbddatadir='${datadir}/kbd';
x11fontdir='${datadir}/fonts';
AC_ARG_WITH([kbddatadir], AS_HELP_STRING([--with-kbddatadir=PATH],
	[Path to the kbd data directory.
	Most likely /usr/share/kbd.]),
	[kbddatadir="$withval"])
AC_ARG_WITH([x11fontdir], AS_HELP_STRING([--with-x11fontdir=PATH],
	[Path to the X11 font directory.
	Most likely /usr/share/fonts.]),
	[x11fontdir="$withval"])
AC_SUBST([kbddatadir])
AC_SUBST([x11fontdir])

AC_CONFIG_FILES([Makefile data/Makefile doc/Makefile
	kbd/Makefile sadmin/Makefile sdevel/Makefile smath/Makefile
	smm/Makefile suser/Makefile])
AC_OUTPUT
