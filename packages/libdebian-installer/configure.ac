AC_INIT([libdebian-installer],m4_esyscmd(dpkg-parsechangelog | perl -ne 'print $1 if m/^Version: (.*)$/;'))
AM_INIT_AUTOMAKE([])
AM_MAINTAINER_MODE

AC_PROG_CC
AC_PROG_LIBTOOL

AC_CHECK_FUNCS(memrchr)

AC_CHECK_PROGS(DOXYGEN, doxygen, false)

LIBRARY_VERSION_MAJOR=4
LIBRARY_VERSION_MINOR=0
LIBRARY_VERSION_REVISION=4
LIBRARY_VERSION="$LIBRARY_VERSION_MAJOR.$LIBRARY_VERSION_MINOR.$LIBRARY_VERSION_REVISION"
LIBRARY_VERSION_LIBTOOL="$LIBRARY_VERSION_MAJOR:$LIBRARY_VERSION_MINOR:$LIBRARY_VERSION_REVISION"

AC_SUBST(LIBRARY_VERSION_LIBTOOL)
AC_SUBST(LIBRARY_VERSION_MAJOR)

AC_DEFINE_UNQUOTED(LIBDI_VERSION_MAJOR,$LIBRARY_VERSION_MAJOR,[Library version - major])
AC_DEFINE_UNQUOTED(LIBDI_VERSION_MINOR,$LIBRARY_VERSION_MINOR,[Library version - minor])
AC_DEFINE_UNQUOTED(LIBDI_VERSION_REVISION,$LIBRARY_VERSION_REVISION,[Library version - revision])
AC_DEFINE_UNQUOTED(LIBDI_VERSION,$LIBRARY_VERSION,[Library version])

AC_CACHE_CHECK([whether $CC supports __attribute__((alias))],di_cv_support_cc_attribute_alias,
	AC_TRY_COMPILE(,[
		void test_alias() __attribute__ ((alias("test")));
		],[di_cv_support_cc_attribute_alias=yes],[di_cv_support_cc_attribute_alias=no]))
if test $di_cv_support_cc_attribute_alias = yes; then
	HAVE_ATTRIBUTE_ALIAS=1
	AC_DEFINE_UNQUOTED(HAVE_ATTRIBUTE_ALIAS,1,[Define to 1 if compiler supports __attribute__((alias))])
fi

AH_VERBATIM([PATH_MAX],
[/* Define PATH_MAX if missing. */
#ifndef PATH_MAX
# undef PATH_MAX
#endif])

AC_CHECK_DECLS(PATH_MAX,[],[AC_DEFINE(PATH_MAX,4096)],[
#include <limits.h>
])

AC_ARG_ENABLE(extensive-debug,AS_HELP_STRING([--enable-extensive-debug],[Enable extensive debugging output]))
if test "$enable_extensive_debug" = yes; then
	AC_DEFINE_UNQUOTED(ENABLE_EXTENSIVE_DEBUG,1,[Define to 1 to enable extensive debugging output])
fi

AC_CONFIG_HEADERS([config.h include/debian-installer/config.h])

AC_CONFIG_FILES([
Makefile
doc/Doxyfile
doc/Makefile
include/Makefile
include/debian-installer/Makefile
include/debian-installer/system/Makefile
src/Makefile
src/system/Makefile
libdebian-installer.pc
libdebian-installer-extra.pc
])

AC_OUTPUT
