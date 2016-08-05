AC_PREREQ([2.57])
AC_INIT([fi-resolve], [0])
AC_CONFIG_AUX_DIR([config])
AC_CONFIG_MACRO_DIR([config])
AC_CONFIG_HEADER(config.h)

AM_INIT_AUTOMAKE([foreign no-define 1.11 subdir-objects])

AM_SILENT_RULES([yes])

AS_IF([test "x$CFLAGS" = "x"],
      [CFLAGS="-fno-strict-aliasing -Wall -Wextra"])
CFLAGS="$CFLAGS -D_GNU_SOURCE"

AC_PROG_CC_C99
AS_IF([test "$ac_cv_proc_cc_c99" = "no"],
      [AC_MSG_WARN([C99 compiler unable to be found.])
       AC_MSG_ERROR([Cannot continue])])

LT_INIT

CHECK_LIBFABRIC
CHECK_DEBUG

# Output files
AC_CONFIG_FILES([
    Makefile
])

# Party on
AC_OUTPUT
