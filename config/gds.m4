dnl -*- shell-script -*-
dnl
dnl Copyright (c) 2004-2010 The Trustees of Indiana University and Indiana
dnl                         University Research and Technology
dnl                         Corporation.  All rights reserved.
dnl Copyright (c) 2004-2005 The University of Tennessee and The University
dnl                         of Tennessee Research Foundation.  All rights
dnl                         reserved.
dnl Copyright (c) 2004-2005 High Performance Computing Center Stuttgart,
dnl                         University of Stuttgart.  All rights reserved.
dnl Copyright (c) 2004-2005 The Regents of the University of California.
dnl                         All rights reserved.
dnl Copyright (c) 2006-2016 Cisco Systems, Inc.  All rights reserved.
dnl Copyright (c) 2007      Sun Microsystems, Inc.  All rights reserved.
dnl Copyright (c) 2009      IBM Corporation.  All rights reserved.
dnl Copyright (c) 2009      Los Alamos National Security, LLC.  All rights
dnl                         reserved.
dnl Copyright (c) 2009-2011 Oak Ridge National Labs.  All rights reserved.
dnl Copyright (c) 2011-2013 NVIDIA Corporation.  All rights reserved.
dnl Copyright (c) 2013-2015 Intel, Inc. All rights reserved
dnl Copyright (c) 2015      Research Organization for Information Science
dnl                         and Technology (RIST). All rights reserved.
dnl Copyright (c) 2016      Mellanox Technologies, Inc.
dnl                         All rights reserved.
dnl
dnl $COPYRIGHT$
dnl
dnl Additional copyrights may follow
dnl
dnl $HEADER$
dnl

AC_DEFUN([GDS_SETUP_CORE],[

    AC_REQUIRE([AC_USE_SYSTEM_EXTENSIONS])
    AC_REQUIRE([AC_CANONICAL_TARGET])
    AC_REQUIRE([AC_PROG_CC])

    # If no prefix was defined, set a good value
    m4_ifval([$1],
             [m4_define([gds_config_prefix],[$1/])],
             [m4_define([gds_config_prefix], [])])

    # Get gds's absolute top builddir (which may not be the same as
    # the real $top_builddir)
    GDS_startdir=`pwd`
    if test x"gds_config_prefix" != "x" && test ! -d "gds_config_prefix"; then
        mkdir -p "gds_config_prefix"
    fi
    if test x"gds_config_prefix" != "x"; then
        cd "gds_config_prefix"
    fi
    GDS_top_builddir=`pwd`
    AC_SUBST(GDS_top_builddir)

    # Get gds's absolute top srcdir (which may not be the same as the
    # real $top_srcdir.  First, go back to the startdir incase the
    # $srcdir is relative.

    cd "$GDS_startdir"
    cd "$srcdir"/gds_config_prefix
    GDS_top_srcdir="`pwd`"
    AC_SUBST(GDS_top_srcdir)

    # Go back to where we started
    cd "$GDS_startdir"

    AC_MSG_NOTICE([gds builddir: $GDS_top_builddir])
    AC_MSG_NOTICE([gds srcdir: $GDS_top_srcdir])
    if test "$GDS_top_builddir" != "$GDS_top_srcdir"; then
        AC_MSG_NOTICE([Detected VPATH build])
    fi

    # Get the version of gds that we are installing
    AC_MSG_CHECKING([for gds version])
    GDS_VERSION="`$GDS_top_srcdir/config/gds_get_version.sh $GDS_top_srcdir/VERSION`"
    if test "$?" != "0"; then
        AC_MSG_ERROR([Cannot continue])
    fi
    AC_MSG_RESULT([$GDS_VERSION])
    AC_SUBST(GDS_VERSION)
    AC_DEFINE_UNQUOTED([GDS_VERSION], ["$GDS_VERSION"],
                       [The library version is always available, contrary to VERSION])

    GDS_RELEASE_DATE="`$GDS_top_srcdir/config/gds_get_version.sh $GDS_top_srcdir/VERSION --release-date`"
    AC_SUBST(GDS_RELEASE_DATE)

    # Save the breakdown the version information
    AC_MSG_CHECKING([for gds major version])
    GDS_MAJOR_VERSION="`$GDS_top_srcdir/config/gds_get_version.sh $GDS_top_srcdir/VERSION --major`"
    if test "$?" != "0"; then
        AC_MSG_ERROR([Cannot continue])
    fi
    AC_SUBST(GDS_MAJOR_VERSION)
    AC_DEFINE_UNQUOTED([GDS_MAJOR_VERSION], [$GDS_MAJOR_VERSION],
                       [The library major version is always available, contrary to VERSION])

    AC_MSG_CHECKING([for gds minor version])
    GDS_MINOR_VERSION="`$GDS_top_srcdir/config/gds_get_version.sh $GDS_top_srcdir/VERSION --minor`"
    if test "$?" != "0"; then
        AC_MSG_ERROR([Cannot continue])
    fi
    AC_SUBST(GDS_MINOR_VERSION)
    AC_DEFINE_UNQUOTED([GDS_MINOR_VERSION], [$GDS_MINOR_VERSION],
                       [The library minor version is always available, contrary to VERSION])

    AC_MSG_CHECKING([for gds release version])
    GDS_RELEASE_VERSION="`$GDS_top_srcdir/config/gds_get_version.sh $GDS_top_srcdir/VERSION --release`"
    if test "$?" != "0"; then
        AC_MSG_ERROR([Cannot continue])
    fi
    AC_SUBST(GDS_RELEASE_VERSION)
    AC_DEFINE_UNQUOTED([GDS_RELEASE_VERSION], [$GDS_RELEASE_VERSION],
                       [The library release version is always available, contrary to VERSION])

    # Debug mode?
    AC_MSG_CHECKING([if want gds maintainer support])
    gds_debug=
    AS_IF([test "$gds_debug" = "" && test "$enable_debug" = "yes"],
          [gds_debug=1
           gds_debug_msg="enabled"])
    AS_IF([test "$gds_debug" = ""],
          [gds_debug=0
           gds_debug_msg="disabled"])
    # Grr; we use #ifndef for GDS_DEBUG!  :-(
    AH_TEMPLATE(GDS_ENABLE_DEBUG, [Whether we are in debugging mode or not])
    AS_IF([test "$gds_debug" = "1"], [AC_DEFINE([GDS_ENABLE_DEBUG])])
    AC_MSG_RESULT([$gds_debug_msg])

    AC_MSG_CHECKING([for gds directory prefix])
    AC_MSG_RESULT(m4_ifval([$1], gds_config_prefix, [(none)]))

    # Note that private/config.h *MUST* be listed first so that it
    # becomes the "main" config header file.  Any AC-CONFIG-HEADERS
    # after that (gds/config.h) will only have selective #defines
    # replaced, not the entire file.
    AC_CONFIG_HEADERS(gds_config_prefix[src/include/gds_config.h])

    # GCC specifics.
    if test "x$GCC" = "xyes"; then
        GDS_GCC_CFLAGS="-Wall -Wmissing-prototypes -Wundef"
        GDS_GCC_CFLAGS="$GDS_GCC_CFLAGS -Wpointer-arith -Wcast-align"
    fi

    ############################################################################
    # Check for compilers and preprocessors
    ############################################################################
    gds_show_title "Compiler and preprocessor tests"

    #
    # Check for some types
    #

    AC_CHECK_TYPES(int8_t)
    AC_CHECK_TYPES(uint8_t)
    AC_CHECK_TYPES(int16_t)
    AC_CHECK_TYPES(uint16_t)
    AC_CHECK_TYPES(int32_t)
    AC_CHECK_TYPES(uint32_t)
    AC_CHECK_TYPES(int64_t)
    AC_CHECK_TYPES(uint64_t)
    AC_CHECK_TYPES(long long)

    AC_CHECK_TYPES(intptr_t)
    AC_CHECK_TYPES(uintptr_t)
    AC_CHECK_TYPES(ptrdiff_t)

    #
    # Check for type sizes
    #

    AC_CHECK_SIZEOF(_Bool)
    AC_CHECK_SIZEOF(char)
    AC_CHECK_SIZEOF(short)
    AC_CHECK_SIZEOF(int)
    AC_CHECK_SIZEOF(long)
    if test "$ac_cv_type_long_long" = yes; then
        AC_CHECK_SIZEOF(long long)
    fi
    AC_CHECK_SIZEOF(float)
    AC_CHECK_SIZEOF(double)

    AC_CHECK_SIZEOF(void *)
    AC_CHECK_SIZEOF(size_t)
    if test "$ac_cv_type_ssize_t" = yes ; then
        AC_CHECK_SIZEOF(ssize_t)
    fi
    if test "$ac_cv_type_ptrdiff_t" = yes; then
        AC_CHECK_SIZEOF(ptrdiff_t)
    fi
    AC_CHECK_SIZEOF(wchar_t)

    AC_CHECK_SIZEOF(pid_t)

    #
    # Check for type alignments
    #

    GDS_C_GET_ALIGNMENT(bool, GDS_ALIGNMENT_BOOL)
    GDS_C_GET_ALIGNMENT(int8_t, GDS_ALIGNMENT_INT8)
    GDS_C_GET_ALIGNMENT(int16_t, GDS_ALIGNMENT_INT16)
    GDS_C_GET_ALIGNMENT(int32_t, GDS_ALIGNMENT_INT32)
    GDS_C_GET_ALIGNMENT(int64_t, GDS_ALIGNMENT_INT64)
    GDS_C_GET_ALIGNMENT(char, GDS_ALIGNMENT_CHAR)
    GDS_C_GET_ALIGNMENT(short, GDS_ALIGNMENT_SHORT)
    GDS_C_GET_ALIGNMENT(wchar_t, GDS_ALIGNMENT_WCHAR)
    GDS_C_GET_ALIGNMENT(int, GDS_ALIGNMENT_INT)
    GDS_C_GET_ALIGNMENT(long, GDS_ALIGNMENT_LONG)
    if test "$ac_cv_type_long_long" = yes; then
        GDS_C_GET_ALIGNMENT(long long, GDS_ALIGNMENT_LONG_LONG)
    fi
    GDS_C_GET_ALIGNMENT(float, GDS_ALIGNMENT_FLOAT)
    GDS_C_GET_ALIGNMENT(double, GDS_ALIGNMENT_DOUBLE)
    if test "$ac_cv_type_long_double" = yes; then
        GDS_C_GET_ALIGNMENT(long double, GDS_ALIGNMENT_LONG_DOUBLE)
    fi
    GDS_C_GET_ALIGNMENT(void *, GDS_ALIGNMENT_VOID_P)
    GDS_C_GET_ALIGNMENT(size_t, GDS_ALIGNMENT_SIZE_T)


    #
    # Does the C compiler native support "bool"? (i.e., without
    # <stdbool.h> or any other help)
    #

    GDS_VAR_SCOPE_PUSH([MSG])
    AC_MSG_CHECKING(for C bool type)
    AC_COMPILE_IFELSE([AC_LANG_PROGRAM([
                                          AC_INCLUDES_DEFAULT],
                                       [[bool bar, foo = true; bar = foo;]])],
                      [GDS_NEED_C_BOOL=0 MSG=yes],[GDS_NEED_C_BOOL=1 MSG=no])
    AC_DEFINE_UNQUOTED(GDS_NEED_C_BOOL, $GDS_NEED_C_BOOL,
                       [Whether the C compiler supports "bool" without any other help (such as <stdbool.h>)])
    AC_MSG_RESULT([$MSG])
    AC_CHECK_SIZEOF(_Bool)
    GDS_VAR_SCOPE_POP

    #
    # Check for other compiler characteristics
    #

    GDS_VAR_SCOPE_PUSH([GDS_CFLAGS_save])
    if test "$GCC" = "yes"; then

        # gcc 2.96 will emit oodles of warnings if you use "inline" with
        # -pedantic (which we do in developer builds).  However,
        # "__inline__" is ok.  So we have to force gcc to select the
        # right one.  If you use -pedantic, the AC_C_INLINE test will fail
        # (because it names a function foo() -- without the (void)).  So
        # we turn off all the picky flags, turn on -ansi mode (which is
        # implied by -pedantic), and set warnings to be errors.  Hence,
        # this does the following (for 2.96):
        #
        # - causes the check for "inline" to emit a warning, which then
        # fails
        # - checks for __inline__, which then emits no error, and works
        #
        # This also works nicely for gcc 3.x because "inline" will work on
        # the first check, and all is fine.  :-)

        GDS_CFLAGS_save=$CFLAGS
        CFLAGS="$GDS_CFLAGS_BEFORE_PICKY -Werror -ansi"
    fi
    AC_C_INLINE
    if test "$GCC" = "yes"; then
        CFLAGS=$GDS_CFLAGS_save
    fi
    GDS_VAR_SCOPE_POP

    if test "x$CC" = "xicc"; then
        GDS_CHECK_ICC_VARARGS
    fi


    ##################################
    # Only after setting up
    # C do we check compiler attributes.
    ##################################

    gds_show_subtitle "Compiler characteristics"

    GDS_CHECK_ATTRIBUTES
    GDS_CHECK_COMPILER_VERSION_ID

    ##################################
    # Header files
    ##################################

    gds_show_title "Header file tests"

    AC_CHECK_HEADERS([arpa/inet.h \
                      fcntl.h inttypes.h libgen.h \
                      netinet/in.h \
                      stdint.h stddef.h \
                      stdlib.h string.h strings.h \
                      sys/param.h \
                      sys/select.h sys/socket.h \
                      stdarg.h sys/stat.h sys/time.h \
                      sys/types.h sys/un.h sys/uio.h net/uio.h \
                      sys/wait.h syslog.h \
                      time.h unistd.h dirent.h \
                      crt_externs.h signal.h \
                      ioLib.h sockLib.h hostLib.h limits.h \
                      sys/statfs.h sys/statvfs.h])

    # Note that sometimes we have <stdbool.h>, but it doesn't work (e.g.,
    # have both Portland and GNU installed; using pgcc will find GNU's
    # <stdbool.h>, which all it does -- by standard -- is define "bool" to
    # "_Bool" [see
    # http://gdsw.opengroup.org/onlinepubs/009695399/basedefs/stdbool.h.html],
    # and Portland has no idea what to do with _Bool).

    # So first figure out if we have <stdbool.h> (i.e., check the value of
    # the macro HAVE_STDBOOL_H from the result of AC_CHECK_HEADERS,
    # above).  If we do have it, then check to see if it actually works.
    # Define GDS_USE_STDBOOL_H as approrpaite.
    AC_CHECK_HEADERS([stdbool.h], [have_stdbool_h=1], [have_stdbool_h=0])
    AC_MSG_CHECKING([if <stdbool.h> works])
    if test "$have_stdbool_h" = "1"; then
        AC_COMPILE_IFELSE([AC_LANG_PROGRAM([AC_INCLUDES_DEFAULT[
                                                   #if HAVE_STDBOOL_H
                                                   #include <stdbool.h>
                                                   #endif
                                               ]],
                                           [[bool bar, foo = true; bar = foo;]])],
                          [GDS_USE_STDBOOL_H=1 MSG=yes],[GDS_USE_STDBOOL_H=0 MSG=no])
    else
        GDS_USE_STDBOOL_H=0
        MSG="no (don't have <stdbool.h>)"
    fi
    AC_DEFINE_UNQUOTED(GDS_USE_STDBOOL_H, $GDS_USE_STDBOOL_H,
                       [Whether to use <stdbool.h> or not])
    AC_MSG_RESULT([$MSG])

    # checkpoint results
    AC_CACHE_SAVE

    ##################################
    # Types
    ##################################

    gds_show_title "Type tests"

    AC_CHECK_TYPES([socklen_t, struct sockaddr_in, struct sockaddr_un,
                    struct sockaddr_in6, struct sockaddr_storage],
                   [], [], [AC_INCLUDES_DEFAULT
                            #if HAVE_SYS_SOCKET_H
                            #include <sys/socket.h>
                            #endif
                            #if HAVE_SYS_UN_H
                            #include <sys/un.h>
                            #endif
                            #ifdef HAVE_NETINET_IN_H
                            #include <netinet/in.h>
                            #endif
                           ])

    AC_CHECK_DECLS([AF_UNSPEC, PF_UNSPEC, AF_INET6, PF_INET6],
                   [], [], [AC_INCLUDES_DEFAULT
                            #if HAVE_SYS_SOCKET_H
                            #include <sys/socket.h>
                            #endif
                            #ifdef HAVE_NETINET_IN_H
                            #include <netinet/in.h>
                            #endif
                           ])

    # SA_RESTART in signal.h
    GDS_VAR_SCOPE_PUSH([MSG2])
    AC_MSG_CHECKING([if SA_RESTART defined in signal.h])
                        AC_EGREP_CPP(yes, [
                                            #include <signal.h>
                                            #ifdef SA_RESTART
                                            yes
                                            #endif
                                        ], [MSG2=yes VALUE=1], [MSG2=no VALUE=0])
    AC_DEFINE_UNQUOTED(GDS_HAVE_SA_RESTART, $VALUE,
                       [Whether we have SA_RESTART in <signal.h> or not])
    AC_MSG_RESULT([$MSG2])
    GDS_VAR_SCOPE_POP

    AC_CHECK_MEMBERS([struct sockaddr.sa_len], [], [], [
                         #include <sys/types.h>
                         #if HAVE_SYS_SOCKET_H
                         #include <sys/socket.h>
                         #endif
                     ])

    AC_CHECK_MEMBERS([struct dirent.d_type], [], [], [
                         #include <sys/types.h>
                         #include <dirent.h>])

    AC_CHECK_MEMBERS([siginfo_t.si_fd],,,[#include <signal.h>])
    AC_CHECK_MEMBERS([siginfo_t.si_band],,,[#include <signal.h>])

    #
    # Checks for struct member names in struct statfs
    #
    AC_CHECK_MEMBERS([struct statfs.f_type], [], [], [
                         AC_INCLUDES_DEFAULT
                         #ifdef HAVE_SYS_VFS_H
                         #include <sys/vfs.h>
                         #endif
                         #ifdef HAVE_SYS_STATFS_H
                         #include <sys/statfs.h>
                         #endif
                     ])

    AC_CHECK_MEMBERS([struct statfs.f_fstypename], [], [], [
                         AC_INCLUDES_DEFAULT
                         #ifdef HAVE_SYS_PARAM_H
                         #include <sys/param.h>
                         #endif
                         #ifdef HAVE_SYS_MOUNT_H
                         #include <sys/mount.h>
                         #endif
                         #ifdef HAVE_SYS_VFS_H
                         #include <sys/vfs.h>
                         #endif
                         #ifdef HAVE_SYS_STATFS_H
                         #include <sys/statfs.h>
                         #endif
                     ])

    #
    # Checks for struct member names in struct statvfs
    #
    AC_CHECK_MEMBERS([struct statvfs.f_basetype], [], [], [
                         AC_INCLUDES_DEFAULT
                         #ifdef HAVE_SYS_STATVFS_H
                         #include <sys/statvfs.h>
                         #endif
                     ])

    AC_CHECK_MEMBERS([struct statvfs.f_fstypename], [], [], [
                         AC_INCLUDES_DEFAULT
                         #ifdef HAVE_SYS_STATVFS_H
                         #include <sys/statvfs.h>
                         #endif
                     ])

    AC_CHECK_MEMBERS([struct ucred.uid, struct ucred.cr_uid, struct sockpeercred.uid],
                     [], [],
                     [#include <sys/types.h>
                      #include <sys/socket.h> ])

    #
    # Check for ptrdiff type.  Yes, there are platforms where
    # sizeof(void*) != sizeof(long) (64 bit Windows, apparently).
    #
    AC_MSG_CHECKING([for pointer diff type])
    if test $ac_cv_type_ptrdiff_t = yes ; then
        gds_ptrdiff_t="ptrdiff_t"
        gds_ptrdiff_size=$ac_cv_sizeof_ptrdiff_t
    elif test $ac_cv_sizeof_void_p -eq $ac_cv_sizeof_long ; then
        gds_ptrdiff_t="long"
        gds_ptrdiff_size=$ac_cv_sizeof_long
    elif test $ac_cv_type_long_long = yes && test $ac_cv_sizeof_void_p -eq $ac_cv_sizeof_long_long ; then
        gds_ptrdiff_t="long long"
        gds_ptrdiff_size=$ac_cv_sizeof_long_long
        #else
        #    AC_MSG_ERROR([Could not find datatype to emulate ptrdiff_t.  Cannot continue])
    fi
    AC_DEFINE_UNQUOTED([GDS_PTRDIFF_TYPE], [$gds_ptrdiff_t],
                       [type to use for ptrdiff_t])
    AC_MSG_RESULT([$gds_ptrdiff_t (size: $gds_ptrdiff_size)])

    ##################################
    # Linker characteristics
    ##################################

    AC_MSG_CHECKING([the linker for support for the -fini option])
    GDS_VAR_SCOPE_PUSH([LDFLAGS_save])
    LDFLAGS_save=$LDFLAGS
    LDFLAGS="$LDFLAGS_save -Wl,-fini -Wl,finalize"
    AC_TRY_LINK([void finalize (void) {}], [], [AC_MSG_RESULT([yes])
            gds_ld_have_fini=1], [AC_MSG_RESULT([no])
            gds_ld_have_fini=0])
    LDFLAGS=$LDFLAGS_save
    GDS_VAR_SCOPE_POP

    gds_destructor_use_fini=0
    gds_no_destructor=0
    if test x$gds_cv___attribute__destructor = x0 ; then
        if test x$gds_ld_have_fini = x1 ; then
            gds_destructor_use_fini=1
        else
            gds_no_destructor=1;
        fi
    fi

    AC_DEFINE_UNQUOTED(GDS_NO_LIB_DESTRUCTOR, [$gds_no_destructor],
        [Whether libraries can be configured with destructor functions])
    AM_CONDITIONAL(GDS_DESTRUCTOR_USE_FINI, [test x$gds_destructor_use_fini = x1])

    ##################################
    # Libraries
    ##################################

    gds_show_title "Library and Function tests"

    GDS_SEARCH_LIBS_CORE([socket], [socket])

    # IRIX and CentOS have dirname in -lgen, usually in libc
    GDS_SEARCH_LIBS_CORE([dirname], [gen])

    # Darwin doesn't need -lm, as it's a symlink to libSystem.dylib
    GDS_SEARCH_LIBS_CORE([ceil], [m])

    AC_CHECK_FUNCS([asprintf snprintf vasprintf vsnprintf strsignal socketpair strncpy_s usleep statfs statvfs getpeereid strnlen])

    # On some hosts, htonl is a define, so the AC_CHECK_FUNC will get
    # confused.  On others, it's in the standard library, but stubbed with
    # the magic glibc foo as not implemented.  and on other systems, it's
    # just not there.  This covers all cases.
    AC_CACHE_CHECK([for htonl define],
                   [gds_cv_htonl_define],
                   [AC_PREPROC_IFELSE([AC_LANG_PROGRAM([
                                                          #ifdef HAVE_SYS_TYPES_H
                                                          #include <sys/types.h>
                                                          #endif
                                                          #ifdef HAVE_NETINET_IN_H
                                                          #include <netinet/in.h>
                                                          #endif
                                                          #ifdef HAVE_ARPA_INET_H
                                                          #include <arpa/inet.h>
                                                          #endif],[
                                                          #ifndef ntohl
                                                          #error "ntohl not defined"
                                                          #endif
                                                      ])], [gds_cv_htonl_define=yes], [gds_cv_htonl_define=no])])
    AC_CHECK_FUNC([htonl], [gds_have_htonl=yes], [gds_have_htonl=no])
    AS_IF([test "$gds_cv_htonl_define" = "yes" || test "$gds_have_htonl" = "yes"],
          [AC_DEFINE_UNQUOTED([HAVE_UNIX_BYTESWAP], [1],
                              [whether unix byteswap routines -- htonl, htons, nothl, ntohs -- are available])])

    # check pandoc separately so we can setup an AM_CONDITIONAL off it
    AC_CHECK_PROG([gds_have_pandoc], [pandoc], [yes], [no])
    AM_CONDITIONAL([GDS_HAVE_PANDOC], [test "x$gds_have_pandoc" = "xyes"])

    #
    # Make sure we can copy va_lists (need check declared, not linkable)
    #

    AC_CHECK_DECL(va_copy, GDS_HAVE_VA_COPY=1, GDS_HAVE_VA_COPY=0,
                  [#include <stdarg.h>])
    AC_DEFINE_UNQUOTED(GDS_HAVE_VA_COPY, $GDS_HAVE_VA_COPY,
                       [Whether we have va_copy or not])

    AC_CHECK_DECL(__va_copy, GDS_HAVE_UNDERSCORE_VA_COPY=1,
                  GDS_HAVE_UNDERSCORE_VA_COPY=0, [#include <stdarg.h>])
    AC_DEFINE_UNQUOTED(GDS_HAVE_UNDERSCORE_VA_COPY, $GDS_HAVE_UNDERSCORE_VA_COPY,
                       [Whether we have __va_copy or not])

    AC_CHECK_DECLS(__func__)

    # checkpoint results
    AC_CACHE_SAVE

    ##################################
    # System-specific tests
    ##################################

    gds_show_title "System-specific tests"

    AC_C_BIGENDIAN
    GDS_CHECK_BROKEN_QSORT

    ##################################
    # Visibility
    ##################################

    # Check the visibility declspec at the end to avoid problem with
    # the previous tests that are not necessarily prepared for
    # the visibility feature.
    gds_show_title "Symbol visibility feature"

    GDS_CHECK_VISIBILITY

    ##################################
    # Libevent
    ##################################
    gds_show_title "Libevent"

    GDS_LIBEVENT_CONFIG

    ##################################
    # HWLOC
    ##################################
    gds_show_title "HWLOC"

    GDS_HWLOC_CONFIG

    ##################################
    # MCA
    ##################################

    gds_show_title "Modular Component Architecture (MCA) setup"

    AC_MSG_CHECKING([for subdir args])
    GDS_CONFIG_SUBDIR_ARGS([gds_subdir_args])
    AC_MSG_RESULT([$gds_subdir_args])

    GDS_MCA

    ############################################################################
    # final compiler config
    ############################################################################

    gds_show_subtitle "Set path-related compiler flags"

    #
    # This is needed for VPATH builds, so that it will -I the appropriate
    # include directory.  We delayed doing it until now just so that
    # '-I$(top_srcdir)' doesn't show up in any of the configure output --
    # purely aesthetic.
    #
    # Because gds_config.h is created by AC_CONFIG_HEADERS, we
    # don't need to -I the builddir for gds/include. However, if we
    # are VPATH building, we do need to include the source directories.
    #
    if test "$GDS_top_builddir" != "$GDS_top_srcdir"; then
        # Note the embedded m4 directives here -- we must embed them
        # rather than have successive assignments to these shell
        # variables, lest the $(foo) names try to get evaluated here.
        # Yuck!
        CPPFLAGS='-I$(GDS_top_builddir) -I$(GDS_top_srcdir) -I$(GDS_top_srcdir)/src -I$(GDS_top_builddir)/include -I$(GDS_top_srcdir)/include'" $CPPFLAGS"
    else
        CPPFLAGS='-I$(GDS_top_srcdir) -I$(GDS_top_srcdir)/src -I$(GDS_top_srcdir)/include'" $CPPFLAGS"
    fi

    # gdsdatadir, gdslibdir, and gdsinclude are essentially the same as
    # pkg*dir, but will always be */gds.
    gdsdatadir='${datadir}/gds'
    gdslibdir='${libdir}/gds'
    gdsincludedir='${includedir}/gds'
    AC_SUBST(gdsdatadir)
    AC_SUBST(gdslibdir)
    AC_SUBST(gdsincludedir)

    ############################################################################
    # final output
    ############################################################################

    gds_show_subtitle "Final output"

    AC_CONFIG_FILES(
        gds_config_prefix[Makefile]
        gds_config_prefix[config/Makefile]
        gds_config_prefix[include/Makefile]
        gds_config_prefix[src/Makefile]
        gds_config_prefix[src/util/keyval/Makefile]
        gds_config_prefix[src/mca/base/Makefile]
        )

    # Success
    $2
])dnl

AC_DEFUN([GDS_DEFINE_ARGS],[
    # Embedded mode, or standalone?
    AC_MSG_CHECKING([if embedded mode is enabled])
    AC_ARG_ENABLE([embedded-mode],
        [AC_HELP_STRING([--enable-embedded-mode],
                [Using --enable-embedded-mode causes GDS to skip a few configure checks and install nothing.  It should only be used when building GDS within the scope of a larger package.])])
    AS_IF([test ! -z "$enable_embedded_mode" && test "$enable_embedded_mode" = "yes"],
          [gds_mode=embedded
           AC_MSG_RESULT([yes])],
          [gds_mode=standalone
           AC_MSG_RESULT([no])])

    # Rename symbols?
    AC_ARG_WITH([gds-symbol-rename],
                AC_HELP_STRING([--with-gds-symbol-rename=FILE],
                               [Provide an include file that contains directives to rename GDS symbols]))
    AS_IF([test ! -z "$with_gds_symbol_rename" && test "$with_gds_symbol_rename" != "yes"],
          [gds_symbol_rename="$with_gds_symbol_rename"],
          [gds_symbol_rename=\"src/include/rename.h\"])
    AC_DEFINE_UNQUOTED(GDS_SYMBOL_RENAME, [$gds_symbol_rename],
                       [The gds symbol rename include directive])

    # Install tests and examples?
    AC_MSG_CHECKING([if tests and examples are to be installed])
    AC_ARG_WITH([tests-examples],
        [AC_HELP_STRING([--with-tests-examples],
                [Whether or not to install the tests and example programs.])])
    AS_IF([test ! -z "$with_tests_examples" && test "$with_tests_examples" = "no"],
          [gds_tests=no
           AC_MSG_RESULT([no])],
          [gds_tests=yes
           AC_MSG_RESULT([yes])])

#
# Is this a developer copy?
#

if test -d .git; then
    GDS_DEVEL=1
else
    GDS_DEVEL=0
fi


#
# Developer picky compiler options
#

AC_MSG_CHECKING([if want developer-level compiler pickyness])
AC_ARG_ENABLE(picky,
    AC_HELP_STRING([--enable-picky],
                   [enable developer-level compiler pickyness when building GDS (default: disabled)]))
if test "$enable_picky" = "yes"; then
    AC_MSG_RESULT([yes])
    WANT_PICKY_COMPILER=1
else
    AC_MSG_RESULT([no])
    WANT_PICKY_COMPILER=0
fi
#################### Early development override ####################
if test "$WANT_PICKY_COMPILER" = "0" && test -z "$enable_picky" && test "$GDS_DEVEL" = "1"; then
    WANT_PICKY_COMPILER=1
    echo "--> developer override: enable picky compiler by default"
fi
#################### Early development override ####################

#
# Developer debugging
#

AC_MSG_CHECKING([if want developer-level debugging code])
AC_ARG_ENABLE(debug,
    AC_HELP_STRING([--enable-debug],
                   [enable developer-level debugging code (not for general GDS users!) (default: disabled)]))
if test "$enable_debug" = "yes"; then
    AC_MSG_RESULT([yes])
    WANT_DEBUG=1
else
    AC_MSG_RESULT([no])
    WANT_DEBUG=0
fi
#################### Early development override ####################
if test "$WANT_DEBUG" = "0" && test -z "$enable_debug" && test "$GDS_DEVEL" = "1"; then
    WANT_DEBUG=1
    echo "--> developer override: enable debugging code by default"
fi
#################### Early development override ####################
if test "$WANT_DEBUG" = "0"; then
    CFLAGS="-DNDEBUG $CFLAGS"
    CXXFLAGS="-DNDEBUG $CXXFLAGS"
fi
AC_DEFINE_UNQUOTED(GDS_ENABLE_DEBUG, $WANT_DEBUG,
                   [Whether we want developer-level debugging code or not])

AC_ARG_ENABLE(debug-symbols,
              AC_HELP_STRING([--disable-debug-symbols],
                             [Disable adding compiler flags to enable debugging symbols if --enable-debug is specified.  For non-debugging builds, this flag has no effect.]))

#
# Do we want to install the internal devel headers?
#
AC_MSG_CHECKING([if want to install project-internal header files])
AC_ARG_WITH(devel-headers,
    AC_HELP_STRING([--with-devel-headers],
                   [normal GDS users/applications do not need this (gds.h and friends are ALWAYS installed).  Developer headers are only necessary for authors doing deeper integration (default: disabled).]))
if test "$with_devel_headers" = "yes"; then
    AC_MSG_RESULT([yes])
    WANT_INSTALL_HEADERS=1
else
    AC_MSG_RESULT([no])
    WANT_INSTALL_HEADERS=0
fi
AM_CONDITIONAL(WANT_INSTALL_HEADERS, test "$WANT_INSTALL_HEADERS" = 1)

#
# Support per-user config files?
#
AC_ARG_ENABLE([per-user-config-files],
   [AC_HELP_STRING([--enable-per-user-config-files],
      [Disable per-user configuration files, to save disk accesses during job start-up.  This is likely desirable for large jobs.  Note that this can also be acheived by environment variables at run-time.  (default: enabled)])])
if test "$enable_per_user_config_files" = "no" ; then
  result=0
else
  result=1
fi
AC_DEFINE_UNQUOTED([GDS_WANT_HOME_CONFIG_FILES], [$result],
     [Enable per-user config files])

#
# Do we want the pretty-print stack trace feature?
#

AC_MSG_CHECKING([if want pretty-print stacktrace])
AC_ARG_ENABLE([pretty-print-stacktrace],
              [AC_HELP_STRING([--enable-pretty-print-stacktrace],
                              [Pretty print stacktrace on process signal (default: enabled)])])
if test "$enable_pretty_print_stacktrace" = "no" ; then
    AC_MSG_RESULT([no])
    WANT_PRETTY_PRINT_STACKTRACE=0
else
    AC_MSG_RESULT([yes])
    WANT_PRETTY_PRINT_STACKTRACE=1
fi
AC_DEFINE_UNQUOTED([GDS_WANT_PRETTY_PRINT_STACKTRACE],
                   [$WANT_PRETTY_PRINT_STACKTRACE],
                   [if want pretty-print stack trace feature])

#
# Do we want the shared memory datastore usage?
#

AC_MSG_CHECKING([if want special dstore usage])
AC_ARG_ENABLE([dstore],
              [AC_HELP_STRING([--enable-dstore],
                              [Using special datastore (default: disabled)])])
if test "$enable_dstore" = "yes" ; then
    AC_MSG_RESULT([yes])
    WANT_DSTORE=1
else
    AC_MSG_RESULT([no])
    WANT_DSTORE=0
fi
AC_DEFINE_UNQUOTED([GDS_ENABLE_DSTORE],
                   [$WANT_DSTORE],
                   [if want special dstore feature])
AM_CONDITIONAL([WANT_DSTORE],[test "x$enable_dstore" = "xyes"])

#
# Ident string
#
AC_MSG_CHECKING([if want ident string])
AC_ARG_WITH([ident-string],
            [AC_HELP_STRING([--with-ident-string=STRING],
                            [Embed an ident string into GDS object files])])
if test "$with_ident_string" = "" || test "$with_ident_string" = "no"; then
    with_ident_string="%VERSION%"
fi
# This is complicated, because $GDS_VERSION may have spaces in it.
# So put the whole sed expr in single quotes -- i.e., directly
# substitute %VERSION% for (not expanded) $GDS_VERSION.
with_ident_string="`echo $with_ident_string | sed -e 's/%VERSION%/$GDS_VERSION/'`"

# Now eval an echo of that so that the "$GDS_VERSION" token is
# replaced with its value.  Enclose the whole thing in "" so that it
# ends up as 1 token.
with_ident_string="`eval echo $with_ident_string`"

AC_DEFINE_UNQUOTED([GDS_IDENT_STRING], ["$with_ident_string"],
                   [ident string for GDS])
AC_MSG_RESULT([$with_ident_string])

#
# Timing support
#
AC_MSG_CHECKING([if want developer-level timing support])
AC_ARG_ENABLE(timing,
              AC_HELP_STRING([--enable-timing],
                             [enable developer-level timing code (default: disabled)]))
if test "$enable_timing" = "yes"; then
    AC_MSG_RESULT([yes])
    WANT_TIMING=1
else
    AC_MSG_RESULT([no])
    WANT_TIMING=0
fi

AC_DEFINE_UNQUOTED([GDS_ENABLE_TIMING], [$WANT_TIMING],
                   [Whether we want developer-level timing support or not])

#
# Install header files
#
AC_MSG_CHECKING([if want to head developer-level header files])
AC_ARG_WITH(devel-headers,
              AC_HELP_STRING([--with-devel-headers],
                             [also install developer-level header files (only for internal GDS developers, default: disabled)]))
if test "$with_devel_headers" = "yes"; then
    AC_MSG_RESULT([yes])
    WANT_INSTALL_HEADERS=1
else
    AC_MSG_RESULT([no])
    WANT_INSTALL_HEADERS=0
fi

AM_CONDITIONAL([WANT_INSTALL_HEADERS], [test $WANT_INSTALL_HEADERS -eq 1])
])dnl

# This must be a standalone routine so that it can be called both by
# GDS_INIT and an external caller (if GDS_INIT is not invoked).
AC_DEFUN([GDS_DO_AM_CONDITIONALS],[
    AS_IF([test "$gds_did_am_conditionals" != "yes"],[
        AM_CONDITIONAL([GDS_EMBEDDED_MODE], [test "x$gds_mode" = "xembedded"])
        AM_CONDITIONAL([GDS_TESTS_EXAMPLES], [test "x$gds_tests" = "xyes"])
        AM_CONDITIONAL([GDS_COMPILE_TIMING], [test "$WANT_TIMING" = "1"])
        AM_CONDITIONAL([GDS_WANT_MUNGE], [test "$gds_munge_support" = "1"])
        AM_CONDITIONAL([GDS_WANT_SASL], [test "$gds_sasl_support" = "1"])
    ])
    gds_did_am_conditionals=yes
])dnl

