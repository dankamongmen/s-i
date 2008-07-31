AC_INIT

PACKAGE=terminal
AC_DEFINE_UNQUOTED(PACKAGE,"$PACKAGE")

AC_PROG_MAKE_SET
AC_PROG_CC

dnl Enable debugging?
AC_ARG_WITH(debug,[  --without-debug         turn off debugging?])
if test "$with_debug" == "yes"; then
  AC_DEFINE(DODEBUG)
  CFLAGS="$CFLAGS -g -D_DEBUG_"
fi

FRONTENDS=""

PKG_CHECK_MODULES(GTK, [gtk+-directfb-2.0],
 FRONTENDS="$FRONTENDS gtk",
 echo "*** Cannot build GTK+ plugin ***")

PKG_CHECK_MODULES(VTE, [vte])

AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)
AC_SUBST(VTE_CFLAGS)
AC_SUBST(FRONTENDS)
AC_SUBST(PACKAGE)

AC_OUTPUT(Makefile)

