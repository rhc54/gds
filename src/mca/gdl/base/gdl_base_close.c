/*
 * Copyright (c) 2004-2010 The Trustees of Indiana University.
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

#include "src/mca/mca.h"
#include "src/mca/base/base.h"

#include "src/mca/gdl/gdl.h"
#include "src/mca/gdl/base/base.h"


int gds_gdl_base_close(void)
{
    /* Close all available modules that are open */
    return gds_mca_base_framework_components_close(&gds_gdl_base_framework, NULL);
}
