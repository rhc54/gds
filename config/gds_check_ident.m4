dnl -*- shell-script -*-
dnl
dnl Copyright (c) 2007      Sun Microsystems, Inc.  All rights reserved.
dnl Copyright (c) 2015      Intel, Inc. All rights reserved
dnl $COPYRIGHT$
dnl
dnl Additional copyrights may follow
dnl
dnl $HEADER$
dnl
dnl defines:
dnl   GDS_$1_USE_PRAGMA_IDENT
dnl   GDS_$1_USE_IDENT
dnl   GDS_$1_USE_CONST_CHAR_IDENT
dnl

# GDS_CHECK_IDENT(compiler-env, compiler-flags,
# file-suffix, lang) Try to compile a source file containing
# a #pragma ident, and determine whether the ident was
# inserted into the resulting object file
# -----------------------------------------------------------
AC_DEFUN([GDS_CHECK_IDENT], [
    AC_MSG_CHECKING([for $4 ident string support])

    gds_pragma_ident_happy=0
    gds_ident_happy=0
    gds_static_const_char_happy=0
    _GDS_CHECK_IDENT(
        [$1], [$2], [$3],
        [[#]pragma ident], [],
        [gds_pragma_ident_happy=1
         gds_message="[#]pragma ident"],
        _GDS_CHECK_IDENT(
            [$1], [$2], [$3],
            [[#]ident], [],
            [gds_ident_happy=1
             gds_message="[#]ident"],
            _GDS_CHECK_IDENT(
                [$1], [$2], [$3],
                [[#]pragma comment(exestr, ], [)],
                [gds_pragma_comment_happy=1
                 gds_message="[#]pragma comment"],
                [gds_static_const_char_happy=1
                 gds_message="static const char[[]]"])))

    AC_DEFINE_UNQUOTED([GDS_$1_USE_PRAGMA_IDENT],
        [$gds_pragma_ident_happy], [Use #pragma ident strings for $4 files])
    AC_DEFINE_UNQUOTED([GDS_$1_USE_IDENT],
        [$gds_ident_happy], [Use #ident strings for $4 files])
    AC_DEFINE_UNQUOTED([GDS_$1_USE_PRAGMA_COMMENT],
        [$gds_pragma_comment_happy], [Use #pragma comment for $4 files])
    AC_DEFINE_UNQUOTED([GDS_$1_USE_CONST_CHAR_IDENT],
        [$gds_static_const_char_happy], [Use static const char[] strings for $4 files])

    AC_MSG_RESULT([$gds_message])

    unset gds_pragma_ident_happy gds_ident_happy gds_static_const_char_happy gds_message
])

# _GDS_CHECK_IDENT(compiler-env, compiler-flags,
# file-suffix, header_prefix, header_suffix, action-if-success, action-if-fail)
# Try to compile a source file containing a #-style ident,
# and determine whether the ident was inserted into the
# resulting object file
# -----------------------------------------------------------
AC_DEFUN([_GDS_CHECK_IDENT], [
    eval gds_compiler="\$$1"
    eval gds_flags="\$$2"

    gds_ident="string_not_coincidentally_inserted_by_the_compiler"
    cat > conftest.$3 <<EOF
$4 "$gds_ident" $5
int main(int argc, char** argv);
int main(int argc, char** argv) { return 0; }
EOF

    # "strings" won't always return the ident string.  objdump isn't
    # universal (e.g., OS X doesn't have it), and ...other
    # complications.  So just try to "grep" for the string in the
    # resulting object file.  If the ident is found in "strings" or
    # the grep succeeds, rule that we have this flavor of ident.

    echo "configure:__oline__: $1" >&5
    gds_output=`$gds_compiler $gds_flags -c conftest.$3 -o conftest.${OBJEXT} 2>&1 1>/dev/null`
    gds_status=$?
    AS_IF([test $gds_status = 0],
          [test -z "$gds_output"
           gds_status=$?])
    GDS_LOG_MSG([\$? = $gds_status], 1)
    AS_IF([test $gds_status = 0 && test -f conftest.${OBJEXT}],
          [gds_output="`strings -a conftest.${OBJEXT} | grep $gds_ident`"
           grep $gds_ident conftest.${OBJEXT} 2>&1 1>/dev/null
           gds_status=$?
           AS_IF([test "$gds_output" != "" || test "$gds_status" = "0"],
                 [$6],
                 [$7])],
          [GDS_LOG_MSG([the failed program was:])
           GDS_LOG_FILE([conftest.$3])
           $7])

    unset gds_compiler gds_flags gds_output gds_status
    rm -rf conftest.* conftest${EXEEXT}
])dnl
