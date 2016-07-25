#
# Copyright (c) 2010      Cisco Systems, Inc.  All rights reserved.
# Copyright (c) 2012-2013 Los Alamos National Security, Inc. All rights reserved.
# $COPYRIGHT$
#
# Additional copyrights may follow
#
# $HEADER$
#

sources = \
        gdstor_lhash.h \
        gdstor_lhash_component.c \
        gdstor_lhash.c

# Make the output library in this directory, and name it either
# mca_<type>_<name>.la (for DSO builds) or libmca_<type>_<name>.la
# (for static builds).

if MCA_BUILD_gds_gdstor_lhash_DSO
component_noinst =
component_install = mca_gdstor_lhash.la
else
component_noinst = libmca_gdstor_lhash.la
component_install =
endif

mcacomponentdir = $(gdslibdir)
mcacomponent_LTLIBRARIES = $(component_install)
mca_gdstor_lhash_la_SOURCES = $(sources)
mca_gdstor_lhash_la_LDFLAGS = -module -avoid-version
mca_gdstor_lhash_la_LIBADD = $(gdstor_hash_LIBS)

noinst_LTLIBRARIES = $(component_noinst)
libmca_gdstor_lhash_la_SOURCES =$(sources)
libmca_gdstor_lhash_la_LDFLAGS = -module -avoid-version
