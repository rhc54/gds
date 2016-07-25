#
# Copyright (c) 2004-2008 The Trustees of Indiana University and Indiana
#                         University Research and Technology
#                         Corporation.  All rights reserved.
# Copyright (c) 2004-2005 The University of Tennessee and The University
#                         of Tennessee Research Foundation.  All rights
#                         reserved.
# Copyright (c) 2004-2005 High Performance Computing Center Stuttgart,
#                         University of Stuttgart.  All rights reserved.
# Copyright (c) 2004-2005 The Regents of the University of California.
#                         All rights reserved.
# Copyright (c) 2010-2016 Cisco Systems, Inc.  All rights reserved.
# $COPYRIGHT$
#
# Additional copyrights may follow
#
# $HEADER$
#

# Need this so that the base knows where to load dynamic components from
# (by default)

AM_CPPFLAGS = \
    $(LTDLINCL)

noinst_LTLIBRARIES = libgds_mca_base.la

dist_gdsdata_DATA = help-mca-base.txt help-mca-var.txt

# Source code files

headers = \
        base.h \
        gds_mca_base_component_repository.h \
        gds_mca_base_var.h \
        gds_mca_base_var_enum.h \
        gds_mca_base_var_group.h \
        gds_mca_base_vari.h \
        gds_mca_base_framework.h

# Library

libgds_mca_base_la_SOURCES = \
        $(headers) \
        gds_mca_base_close.c \
        gds_mca_base_component_compare.c \
        gds_mca_base_component_find.c \
        gds_mca_base_component_repository.c \
        gds_mca_base_components_open.c \
        gds_mca_base_components_close.c \
        gds_mca_base_components_select.c \
        gds_mca_base_list.c \
        gds_mca_base_open.c \
        gds_mca_base_var.c \
        gds_mca_base_var_enum.c \
        gds_mca_base_var_group.c \
        gds_mca_base_parse_paramfile.c \
        gds_mca_base_components_register.c \
        gds_mca_base_framework.c

# Conditionally install the header files

if WANT_INSTALL_HEADERS
gdsdir = $(gdsincludedir)/$(subdir)
gds_HEADERS = $(headers)
endif
