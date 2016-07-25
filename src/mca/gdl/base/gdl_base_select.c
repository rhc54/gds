/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2004-2010 The Trustees of Indiana University.
 *                         All rights reserved.
 *
 * Copyright (c) 2015 Cisco Systems, Inc.  All rights reserved.
 * $COPYRIGHT$
 * Copyright (c) 2015      Los Alamos National Security, LLC. All rights
 *                         reserved.
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include <src/include/gds_config.h>

#ifdef HAVE_UNISTD_H
#include "unistd.h"
#endif

#include "gds_common.h"
#include "src/util/output.h"
#include "src/mca/mca.h"
#include "src/mca/base/base.h"
#include "src/mca/gdl/gdl.h"
#include "src/mca/gdl/base/base.h"


int gds_gdl_base_select(void)
{
    int exit_status = GDS_SUCCESS;
    gds_gdl_base_component_t *best_component = NULL;
    gds_gdl_base_module_t *best_module = NULL;

    /*
     * Select the best component
     */
    if (GDS_SUCCESS != gds_mca_base_select("gdl",
                                             gds_gdl_base_framework.framework_output,
                                             &gds_gdl_base_framework.framework_components,
                                             (gds_mca_base_module_t **) &best_module,
                                             (gds_mca_base_component_t **) &best_component, NULL) ) {
        /* This will only happen if no component was selected */
        exit_status = GDS_ERROR;
        goto cleanup;
    }

    /* Save the winner */
    gds_gdl_base_selected_component = best_component;
    gds_gdl = best_module;

 cleanup:
    return exit_status;
}
