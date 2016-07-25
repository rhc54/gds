/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2004-2006 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2006 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2006 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2006 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2013-2015 Los Alamos National Security, LLC. All rights
 *                         reserved.
 * Copyright (c) 2016      Intel, Inc. All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include <src/include/gds_config.h>

#include "src/class/gds_list.h"
#include "src/util/output.h"
#include "src/mca/mca.h"
#include "src/mca/base/base.h"
#include "src/mca/base/gds_mca_base_component_repository.h"
#include "gds_common.h"

void gds_mca_base_component_unload (const gds_mca_base_component_t *component, int output_id)
{
    int ret;

    /* Unload */
    gds_output_verbose (GDS_MCA_BASE_VERBOSE_COMPONENT, output_id,
                         "mca: base: close: unloading component %s",
                         component->gds_mca_component_name);

    ret = gds_mca_base_var_group_find (component->gds_mca_project_name, component->gds_mca_type_name,
                                   component->gds_mca_component_name);
    if (0 <= ret) {
        gds_mca_base_var_group_deregister (ret);
    }

    gds_mca_base_component_repository_release (component);
}

void gds_mca_base_component_close (const gds_mca_base_component_t *component, int output_id)
{
    /* Close */
    if (NULL != component->gds_mca_close_component) {
        component->gds_mca_close_component();
        gds_output_verbose (GDS_MCA_BASE_VERBOSE_COMPONENT, output_id,
                             "mca: base: close: component %s closed",
                             component->gds_mca_component_name);
    }

    gds_mca_base_component_unload (component, output_id);
}

int gds_mca_base_framework_components_close (gds_mca_base_framework_t *framework,
                                              const gds_mca_base_component_t *skip)
{
    return gds_mca_base_components_close (framework->framework_output,
                                           &framework->framework_components,
                                           skip);
}

int gds_mca_base_components_close(int output_id, gds_list_t *components,
                                   const gds_mca_base_component_t *skip)
{
    gds_mca_base_component_list_item_t *cli, *next;

    /* Close and unload all components in the available list, except the
       "skip" item.  This is handy to close out all non-selected
       components.  It's easier to simply remove the entire list and
       then simply re-add the skip entry when done. */

    GDS_LIST_FOREACH_SAFE(cli, next, components, gds_mca_base_component_list_item_t) {
        if (skip == cli->cli_component) {
            continue;
        }

        gds_mca_base_component_close (cli->cli_component, output_id);
        gds_list_remove_item (components, &cli->super);

        GDS_RELEASE(cli);
    }

    /* All done */
    return GDS_SUCCESS;
}
