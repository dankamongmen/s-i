# Process this file with autoconf to produce a configure script.
AC_INIT([autopartkit],[0.76],[debian-boot@lists.debian.org])
AM_INIT_AUTOMAKE([foreign no-define])

AM_CONFIG_HEADER(config.h)

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL

if test "$GCC" = yes; then
  # Can not use -ansi, need 'long long'
  #      -ansi \
  for flag in \
      -pedantic \
      -W \
      -Wall \
      -Wcast-align \
      -Wcast-qual \
      -Wmissing-declarations \
      -Wmissing-prototypes \
      -Wpointer-arith \
      -Wreturn-type \
      -Wstrict-prototypes 
  do
    JAPHAR_GREP_CFLAGS($flag, [ CFLAGS="$CFLAGS $flag" ])
  done
fi

# Checks for libraries.
AC_CHECK_LIB([debconfclient], [debconfclient_new])
AC_CHECK_LIB([dl], [dlopen])
AC_CHECK_LIB([uuid], [uuid_generate])
AC_CHECK_LIB([parted], [ped_constraint_any])
AC_CHECK_LIB([m], [floor])

if test "$ac_cv_lib_parted_ped_constraint_any" != yes; then
    AC_MSG_ERROR(Unable to find required library libparted.)
fi

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h string.h sys/mount.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T
AC_CHECK_MEMBERS([struct stat.st_rdev])

# Checks for library functions.
AC_HEADER_MAJOR
AC_FUNC_MALLOC
AC_FUNC_STAT
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([memset mkdir strchr strdup strerror strstr])

# libparted 1.4 got ped_disk_write, but no ped_disk_commit
# libparted 1.6 got ped_disk_commit, but no ped_disk_write
AC_CHECK_FUNCS([ped_disk_commit ped_disk_write])

AC_CHECK_FUNCS([ped_partition_get_path])

AC_ARG_ENABLE(test,
[  --enable-test            Enable test code [default=no]],
[case "${enableval}" in
      yes) AC_DEFINE(TEST, 1, [Enable test code])
           CFLAGS="$CFLAGS -DTEST -g"
	   ENABLE_TEST=yes
	   ;;
      no)  ENABLE_TEST=no ;;
      *)   AC_MSG_ERROR(bad value ${enableval} for --enable-test) ;;
 esac],[])
AM_CONDITIONAL(TEST, test "$ENABLE_TEST" = yes)

AC_ARG_ENABLE(lvm,
[  --enable-lvm            Enable hack to make LVM partitions [default=no]],
[case "${enableval}" in
      yes) AC_DEFINE(LVM_HACK, 1, [Enable LVM hack]) ;;
      no)  ;;
      *)   AC_MSG_ERROR(bad value ${enableval} for --enable-lvm) ;;
 esac],[])

AC_ARG_ENABLE(small,
[  --enable-small          Make binary as small as possible [default=no]],
[case "${enableval}" in
      yes) if eval "test x$GCC = xyes"; then
	      CFLAGS="$CFLAGS -Os -fomit-frame-pointer"
	   else
	      AC_MSG_ERROR(--enable-small only work with GCC)
	   fi
	   ;;
      no)  ;;
      *)   AC_MSG_ERROR(bad value ${enableval} for --enable-small) ;;
 esac],[])


AC_CONFIG_FILES([Makefile autopartkit.d/Makefile])
AC_OUTPUT
