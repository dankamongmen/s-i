AC_INIT([udpkg],[0.1])
AC_CONFIG_SRCDIR([udpkg.c])
AM_INIT_AUTOMAKE([foreign 1.8 no-define no-dist no-installinfo no-installman -Wall])
AC_CONFIG_HEADER([config.h])
AM_MAINTAINER_MODE

AC_PROG_CC
AC_ISC_POSIX

AC_GNU_SOURCE

AC_MSG_CHECKING(architecture)
DEB_ARCH=$(dpkg-architecture -qDEB_BUILD_ARCH)
AC_MSG_RESULT($DEB_ARCH)
AC_DEFINE_UNQUOTED([ARCH_TEXT],["$DEB_ARCH"],[Debian architecture])

AC_ARG_ENABLE(debug,AS_HELP_STRING([--enable-debug],[debugging]),,[enable_debug=no])
if test "$enable_debug" = "yes"; then
  AC_DEFINE(DODEBUG,[],[Enable debug])
  CFLAGS="$CFLAGS -g"
fi

dnl Do dependency checking?
AC_ARG_ENABLE(depends,AS_HELP_STRING([--enable-depends],[do not do dependency checking]),,[enable_depends=no])
if test "$enable_depends" = "yes"; then
  AC_DEFINE(DODEPENDS,[],[Should we do full dependency checking])
fi

dnl Load templates?
AC_ARG_ENABLE(loadtemplate,AS_HELP_STRING([--disable-loadtemplate],[do not call debconf-loadtemplate]),,[enable_loadtemplate=yes])
if test "$enable_loadtemplate" = "yes"; then
  AC_DEFINE(DOLOADTEMPLATE,[],[Should we call debconf-loadtemplate])
fi

dnl Support localized status files?
AC_ARG_ENABLE(l18n,AS_HELP_STRING([--enable-l18n],[support localized status files]),,[enable_l18n=no])
if test "$enable_l18n" = "yes"; then
  AC_DEFINE(SUPPORTL18N,[],[Support l18n])
fi

dnl Do we need --remove support?
AC_ARG_ENABLE(remove,AS_HELP_STRING([--enable-remove],[support the --remove flag]),,[enable_remove=no])
if test "$enable_remove" = "yes"; then
  AC_DEFINE(DOREMOVE,[],[Support --remove])
fi

dnl dpkg library directory
ADMINDIR="/var/lib/dpkg"
AC_ARG_WITH(admindir,AS_HELP_STRING([--with-admindir=DIR],[store dpkg data in DIR [/var/lib/dpkg]]),
[case "$withval" in
  "") AC_MSG_ERROR(invalid admindir specified) ;;
  *) ADMINDIR="$withval" ;;
 esac])
AC_DEFINE_UNQUOTED(ADMINDIR,"$ADMINDIR",[Admindir])

AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS(fcntl.h string.h sys/time.h unistd.h)

AC_C_CONST
AC_C_INLINE
AC_HEADER_TIME

AC_TYPE_SIGNAL

AC_CONFIG_FILES(Makefile)

AC_OUTPUT
