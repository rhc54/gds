/*
 * Copyright (c) 2006-2013 Los Alamos National Security, LLC.  All rights
 *                         reserved.
 * Copyright (c) 2007-2010 Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2010      Sandia National Laboratories. All rights reserved.
 * Copyright (c) 2016      Intel, Inc. All rights reserved
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 *
 */

#ifndef GDS_GINSTALLDIRS_BASE_H
#define GDS_GINSTALLDIRS_BASE_H

#include <src/include/gds_config.h>
#include "src/mca/base/gds_mca_base_framework.h"
#include "src/mca/ginstalldirs/ginstalldirs.h"

/*
 * Global functions for MCA overall ginstalldirs open and close
 */
BEGIN_C_DECLS

/**
 * Framework structure declaration
 */
extern gds_mca_base_framework_t gds_ginstalldirs_base_framework;

/* Just like gds_ginstall_dirs_expand() (see ginstalldirs.h), but will
   also insert the value of the environment variable $GDS_DESTDIR, if
   it exists/is set.  This function should *only* be used during the
   setup routines of ginstalldirs. */
char * gds_ginstall_dirs_expand_setup(const char* input);

END_C_DECLS

#endif /* GDS_BASE_GINSTALLDIRS_H */
