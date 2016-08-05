dnl Helper macro called from top-level configure.ac to add an enable debug
dnl option that sets the CFLAGS and defines ENABLE_DEBUG to either 1 or 0.
AC_DEFUN([CHECK_DEBUG], [
          AC_ARG_ENABLE([debug],
                        [AS_HELP_STRING([--enable-debug],
                                        [Enable debugging - default NO])],
                        [CFLAGS="$CFLAGS -O0 -g -Werror"
                         dbg=1],
                        [CFLAGS="$CFLAGS -O2 -DNDEBUG"
                         dbg=0])

          AC_DEFINE_UNQUOTED([ENABLE_DEBUG], [$dbg],
                             [defined to 1 if configured with --enable-debug])
])
