#
# Copyright (c) 2004-2010 The Trustees of Indiana University.
#                         All rights reserved.
# Copyright (c) 2014-2015 Cisco Systems, Inc.  All rights reserved.
# $COPYRIGHT$
#
# Additional copyrights may follow
#
# $HEADER$
#

sources = \
        gdl_gdlopen.h \
        gdl_gdlopen_component.c \
        gdl_gdlopen_module.c

# This component will only ever be built statically -- never as a DSO.

noinst_LTLIBRARIES = libmca_gdl_gdlopen.la

libmca_gdl_gdlopen_la_SOURCES = $(sources)
libmca_gdl_gdlopen_la_LDFLAGS = -module -avoid-version
libmca_gdl_gdlopen_la_LIBADD = $(gds_gdl_gdlopen_LIBS)
