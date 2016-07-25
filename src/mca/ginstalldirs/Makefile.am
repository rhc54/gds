#
# Copyright (c) 2006      Los Alamos National Security, LLC.  All rights
#                         reserved.
# Copyright (c) 2010      Cisco Systems, Inc.  All rights reserved.
# Copyright (c) 2016      Intel, Inc. All rights reserved
# $COPYRIGHT$
#
# Additional copyrights may follow
#
# $HEADER$
#

# main library setup
noinst_LTLIBRARIES = libmca_ginstalldirs.la
libmca_ginstalldirs_la_SOURCES =

# local files
headers = ginstalldirs.h
libmca_ginstalldirs_la_SOURCES += $(headers)

# Conditionally install the header files
if WANT_INSTALL_HEADERS
gdsdir = $(gdsincludedir)/$(subdir)
nobase_gds_HEADERS = $(headers)
endif

include base/Makefile.am

distclean-local:
	rm -f base/static-components.h
