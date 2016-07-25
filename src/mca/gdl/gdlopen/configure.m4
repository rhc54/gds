# -*- shell-script -*-
#
# Copyright (c) 2009-2015 Cisco Systems, Inc.  All rights reserved.
#
# $COPYRIGHT$
#
# Additional copyrights may follow
#
# $HEADER$
#

AC_DEFUN([MCA_gds_gdl_gdlopen_PRIORITY], [80])

#
# Force this component to compile in static-only mode
#
AC_DEFUN([MCA_gds_gdl_gdlopen_COMPILE_MODE], [
    AC_MSG_CHECKING([for MCA component $2:$3 compile mode])
    $4="static"
    AC_MSG_RESULT([$$4])
])

# MCA_gdl_gdlopen_CONFIG([action-if-can-compile],
#                      [action-if-cant-compile])
# ------------------------------------------------
AC_DEFUN([MCA_gds_gdl_gdlopen_CONFIG],[
    AC_CONFIG_FILES([src/mca/gdl/gdlopen/Makefile])

    dnl This is effectively a back-door for GDS developers to
    dnl force the use of the libltdl gdl component.
    AC_ARG_ENABLE([gdl-dlopen],
        [AS_HELP_STRING([--disable-gdl-dlopen],
                        [Disable the "dlopen" GDL component (and probably force the use of the "libltdl" GDL component).  This option should really only be used by GDS developers.  You are probably actually looking for the "--disable-dlopen" option, which disables all dlopen-like functionality from GDS.])
        ])

    gds_gdl_gdlopen_happy=no
    AS_IF([test "$enable_gdl_dlopen" != "no"],
          [GDS_CHECK_PACKAGE([gds_gdl_gdlopen],
              [dlfcn.h],
              [dl],
              [dlopen],
              [],
              [],
              [],
              [gds_gdl_gdlopen_happy=yes],
              [gds_gdl_gdlopen_happy=no])
          ])

    AS_IF([test "$gds_gdl_gdlopen_happy" = "yes"],
          [gds_gdl_gdlopen_ADD_LIBS=$gds_gdl_gdlopen_LIBS
           $1],
          [$2])

    AC_SUBST(gds_gdl_gdlopen_LIBS)
])
