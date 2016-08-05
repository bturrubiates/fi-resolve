dnl Helper macro to check for libfabric and fail if not available.
AC_DEFUN([CHECK_LIBFABRIC], [
          AC_ARG_WITH([libfabric],
                      AS_HELP_STRING([--with-libfabric], [use non-default location for libfabric]),
                      [AS_IF([test -d $withval/lib64], [fabric_libdir="lib64"], [fabric_libdir="lib"])
                              CPPFLAGS="-I$withval/include $CPPFLAGS"
                              LDFLAGS="-L$withval/$fabric_libdir $LDFLAGS"],
         [])

         AC_CHECK_LIB([fabric], fi_getinfo, [],
                       AC_MSG_ERROR([fi_getinfo() not found.]))
         AC_CHECK_HEADER([rdma/fabric.h], [],
                          AC_MSG_ERROR([<rdma/fabric.h> not found.]))
])
