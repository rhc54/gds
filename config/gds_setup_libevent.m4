# -*- shell-script -*-
#
# Copyright (c) 2009-2015 Cisco Systems, Inc.  All rights reserved.
# Copyright (c) 2013      Los Alamos National Security, LLC.  All rights reserved.
# Copyright (c) 2013-2016 Intel, Inc. All rights reserved
# $COPYRIGHT$
#
# Additional copyrights may follow
#
# $HEADER$
#

# MCA_libevent_CONFIG([action-if-found], [action-if-not-found])
# --------------------------------------------------------------------
AC_DEFUN([GDS_LIBEVENT_CONFIG],[
    AC_ARG_WITH([libevent-header],
                [AC_HELP_STRING([--with-libevent-header=HEADER],
                                [The value that should be included in C files to include event.h])])

    AC_ARG_ENABLE([embedded-libevent],
                  [AC_HELP_STRING([--enable-embedded-libevent],
                                  [Enable use of locally embedded libevent])])

    AS_IF([test "$enable_embedded_libevent" = "yes"],
          [_GDS_LIBEVENT_EMBEDDED_MODE],
          [_GDS_LIBEVENT_EXTERNAL])

    AC_MSG_CHECKING([libevent header])
    AC_DEFINE_UNQUOTED([GDS_EVENT_HEADER], [$GDS_EVENT_HEADER],
                       [Location of event.h])
    AC_MSG_RESULT([$GDS_EVENT_HEADER])
    AC_MSG_CHECKING([libevent2/thread header])
    AC_DEFINE_UNQUOTED([GDS_EVENT2_THREAD_HEADER], [$GDS_EVENT2_THREAD_HEADER],
                       [Location of event2/thread.h])
    AC_MSG_RESULT([$GDS_EVENT2_THREAD_HEADER])

    CPPFLAGS="$CPPFLAGS $GDS_EVENT_CPPFLAGS"
    LDFLAGS="$LDFLAGS $GDS_EVENT_LDFLAGS"
    LIBS="$LIBS $GDS_EVENT_LIBS"
])

AC_DEFUN([_GDS_LIBEVENT_EMBEDDED_MODE],[
    AC_MSG_CHECKING([for libevent])
    AC_MSG_RESULT([assumed available (embedded mode)])

    GDS_EVENT_HEADER="$with_libevent_header"
    GDS_EVENT2_THREAD_HEADER="$with_libevent_header"
    GDS_EVENT_CPPFLAGS=
    GDS_EVENT_LIB=
    GDS_EVENT_LDFLAGS=

 ])

AC_DEFUN([_GDS_LIBEVENT_EXTERNAL],[
    GDS_VAR_SCOPE_PUSH([gds_event_dir gds_event_libdir])

    AC_ARG_WITH([libevent],
                [AC_HELP_STRING([--with-libevent=DIR],
                                [Search for libevent headers and libraries in DIR ])])

    # Bozo check
    AS_IF([test "$with_libevent" = "no"],
          [AC_MSG_WARN([It is not possible to configure GDS --without-libevent])
           AC_MSG_ERROR([Cannot continue])])

    AC_ARG_WITH([libevent-libdir],
                [AC_HELP_STRING([--with-libevent-libdir=DIR],
                                [Search for libevent libraries in DIR ])])

    AC_MSG_CHECKING([for libevent in])
    if test ! -z "$with_libevent" && test "$with_libevent" != "yes"; then
        gds_event_dir=$with_libevent
        if test -d $with_libevent/lib; then
            gds_event_libdir=$with_libevent/lib
        elif test -d $with_libevent/lib64; then
            gds_event_libdir=$with_libevent/lib64
        else
            AC_MSG_RESULT([Could not find $with_libevent/lib or $with_libevent/lib64])
            AC_MSG_ERROR([Can not continue])
        fi
        AC_MSG_RESULT([$gds_event_dir and $gds_event_libdir])
    else
        AC_MSG_RESULT([(default search paths)])
    fi
    AS_IF([test ! -z "$with_libevent_libdir" && "$with_libevent_libdir" != "yes"],
          [gds_event_libdir="$with_libevent_libdir"])

    GDS_CHECK_PACKAGE([gds_libevent],
                       [event.h],
                       [event],
                       [event_config_new],
                       [-levent -levent_pthreads],
                       [$gds_event_dir],
                       [$gds_event_libdir],
                       [],
                       [AC_MSG_WARN([LIBEVENT SUPPORT NOT FOUND])
                        AC_MSG_ERROR([CANNOT CONTINE])])

    CPPFLAGS="$gds_libevent_CPPFLAGS $CPPFLAGS"
    LIBS="$gds_libevent_LIBS $LIBS"
    LDFLAGS="$gds_libevent_LDFLAGS $LDFLAGS"


    # Ensure that this libevent has the symbol
    # "evthread_set_lock_callbacks", which will only exist if
    # libevent was configured with thread support.
    AC_CHECK_LIB([event], [evthread_set_lock_callbacks],
                 [],
                 [AC_MSG_WARN([External libevent does not have thread support])
                  AC_MSG_WARN([GDS requires libevent to be compiled with])
                  AC_MSG_WARN([thread support enabled])
                  AC_MSG_ERROR([Cannot continue])])
    AC_CHECK_LIB([event_pthreads], [evthread_use_pthreads],
                 [],
                 [AC_MSG_WARN([External libevent does not have thread support])
                  AC_MSG_WARN([GDS requires libevent to be compiled with])
                  AC_MSG_WARN([thread support enabled])
                  AC_MSG_ERROR([Cannot continue])])
    # Chck if this libevent has the symbol
    # "libevent_global_shutdown", which will only exist in
    # libevent version 2.1.1+
    AC_CHECK_FUNCS([libevent_global_shutdown],[], [])

    # Set output variables
    GDS_EVENT_HEADER="<event.h>"
    GDS_EVENT2_THREAD_HEADER="<event2/thread.h>"
    GDS_EVENT_LIB=-levent
    AS_IF([test "$gds_event_dir" != ""],
        [GDS_EVENT_CPPFLAGS="-I$gds_event_dir/include"])
    AS_IF([test "$gds_event_libdir" != ""],
        [GDS_EVENT_LDFLAGS="-L$gds_event_libdir"])

    GDS_VAR_SCOPE_POP
])dnl
