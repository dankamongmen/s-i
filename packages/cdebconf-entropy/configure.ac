AC_INIT(newt-plugin-entropy-text.c)
PACKAGE=entropy-text
AC_DEFINE_UNQUOTED(PACKAGE,"$PACKAGE")

AC_PROG_MAKE_SET
AC_PROG_CC

dnl Enable debugging?
AC_ARG_WITH(debug,[  --without-debug         turn off debugging?])
if test "$with_debug" == "yes"; then
  AC_DEFINE(DODEBUG)
  CFLAGS="$CFLAGS -g -D_DEBUG_"
fi

AC_CHECK_LIB(newt, newtInit, FRONTENDS="$FRONTENDS newt", echo
 "*** Cannot build Newt plugin ***")

AC_SUBST(FRONTENDS)
AC_SUBST(PACKAGE)

AC_OUTPUT(Makefile)
