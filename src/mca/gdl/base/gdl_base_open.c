/*
 * Copyright (c) 2004-2010 The Trustees of Indiana University.
 *                         All rights reserved.
 * Copyright (c) 2011-2013 Los Alamos National Security, LLC.
 *                         All rights reserved.
 * Copyright (c) 2015      Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2016      Intel, Inc. All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include <src/include/gds_config.h>

#include "src/mca/gdl/base/base.h"

#include "src/mca/gdl/base/static-components.h"


/*
 * Globals
 */
gds_gdl_base_module_t *gds_gdl = NULL;
gds_gdl_base_component_t *gds_gdl_base_selected_component = NULL;


/*
 * Function for finding and opening either all MCA components,
 * or the one that was specifically requested via a MCA parameter.
 *
 * Note that we really don't need this function -- we could specify a
 * NULL pointer in the framework declare and the base would do this
 * exact same thing.  However, we need to have at least some
 * executable code in this file, or some linkers (cough cough OS X
 * cough cough) may not actually link in this .o file.
 */
int gds_gdl_base_open(gds_mca_base_open_flag_t flags)
{
    /* Open up all available components */
    return gds_mca_base_framework_components_open(&gds_gdl_base_framework, flags);
}

/* VERY IMPORTANT: This framework is static, and is opened before any
   other dyanmic frameworks are opened (which makes sense, of course).
   But we must mark this framework is NO_DSO so that the MCA framework
   base doesn't try to open any dynamic components in this
   framework. */
GDS_MCA_BASE_FRAMEWORK_DECLARE(gds, gdl, "Dynamic loader framework",
                                NULL /* register */,
                                gds_gdl_base_open /* open */,
                                NULL /* close */,
                                mca_gdl_base_static_components,
                                GDS_MCA_BASE_FRAMEWORK_FLAG_NO_DSO);
