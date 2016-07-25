#
# Copyright (c) 2004-2005 The Trustees of Indiana University and Indiana
#                         University Research and Technology
#                         Corporation.  All rights reserved.
# Copyright (c) 2004-2005 The University of Tennessee and The University
#                         of Tennessee Research Foundation.  All rights
#                         reserved.
# Copyright (c) 2004-2009 High Performance Computing Center Stuttgart,
#                         University of Stuttgart.  All rights reserved.
# Copyright (c) 2004-2005 The Regents of the University of California.
#                         All rights reserved.
# Copyright (c) 2006-2016 Cisco Systems, Inc.  All rights reserved.
# Copyright (c) 2012-2013 Los Alamos National Security, Inc.  All rights reserved.
# Copyright (c) 2013-2016 Intel, Inc. All rights reserved
# $COPYRIGHT$
#
# Additional copyrights may follow
#
# $HEADER$
#

# Note that the -I directory must *exactly* match what was specified
# via AC_CONFIG_MACRO_DIR in configure.ac.
ACLOCAL_AMFLAGS = -I ./config

SUBDIRS = \
	util/keyval \
    mca/base \
    $(MCA_gds_FRAMEWORKS_SUBDIRS) \
    $(MCA_gds_FRAMEWORK_COMPONENT_STATIC_SUBDIRS) \
    . \
    $(MCA_gds_FRAMEWORK_COMPONENT_DSO_SUBDIRS)

DIST_SUBDIRS = \
	util/keyval \
    mca/base \
    $(MCA_gds_FRAMEWORKS_SUBDIRS) \
    $(MCA_gds_FRAMEWORK_COMPONENT_ALL_SUBDIRS)

headers =
sources =
nodist_headers =
EXTRA_DIST =
dist_gdsdata_DATA =

libgds_la_LIBADD = \
	mca/base/libgds_mca_base.la \
	$(MCA_gds_FRAMEWORK_LIBS)
libgds_la_DEPENDENCIES = $(libgds_la_LIBADD)

if GDS_EMBEDDED_MODE
noinst_LTLIBRARIES = libgds.la
libgds_la_SOURCES = $(headers) $(sources)
libgds_la_LDFLAGS =

else

lib_LTLIBRARIES = libgds.la
libgds_la_SOURCES = $(headers) $(sources)
libgds_la_LDFLAGS = -version-info $(libgds_so_version)

endif !GDS_EMBEDDED_MODE

include class/Makefile.include
include include/Makefile.include
include mca/Makefile.include
include util/Makefile.include
include runtime/Makefile.include

MAINTAINERCLEANFILES = Makefile.in config.h config.h.in
DISTCLEANFILES = Makefile
CLEANFILES = core.* *~
AM_CFLAGS = -Wall
