/*
 * Copyright (c) 2004-2008 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2005 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2016      Intel, Inc. All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include <src/include/gds_config.h>
#include <gds_common.h>

#include <string.h>

#include "src/mca/mca.h"
#include "src/mca/base/base.h"

#include "src/mca/pgdstor/base/base.h"

static bool selected = false;

/* Function for selecting a prioritized list of components
 * from all those that are available. */
int gds_pgdstor_base_select(void)
{
    gds_mca_base_component_list_item_t *cli = NULL;
    gds_mca_base_component_t *component = NULL;
    gds_mca_base_module_t *module = NULL;
    gds_pgdstor_module_t *nmodule;
    gds_pgdstor_base_active_module_t *newmodule, *mod;
    int rc, priority;
    bool inserted;

    if (selected) {
        /* ensure we don't do this twice */
        return GDS_SUCCESS;
    }
    selected = true;

    /* Query all available components and ask if they have a module */
    GDS_LIST_FOREACH(cli, &gds_pgdstor_base_framework.framework_components, gds_mca_base_component_list_item_t) {
        component = (gds_mca_base_component_t *) cli->cli_component;

        gds_output_verbose(5, gds_pgdstor_base_framework.framework_output,
                            "mca:pgdstor:select: checking available component %s", component->gds_mca_component_name);

        /* If there's no query function, skip it */
        if (NULL == component->gds_mca_query_component) {
            gds_output_verbose(5, gds_pgdstor_base_framework.framework_output,
                                "mca:pgdstor:select: Skipping component [%s]. It does not implement a query function",
                                component->gds_mca_component_name );
            continue;
        }

        /* Query the component */
        gds_output_verbose(5, gds_pgdstor_base_framework.framework_output,
                            "mca:pgdstor:select: Querying component [%s]",
                            component->gds_mca_component_name);
        rc = component->gds_mca_query_component(&module, &priority);

        /* If no module was returned, then skip component */
        if (GDS_SUCCESS != rc || NULL == module) {
            gds_output_verbose(5, gds_pgdstor_base_framework.framework_output,
                                "mca:pgdstor:select: Skipping component [%s]. Query failed to return a module",
                                component->gds_mca_component_name );
            continue;
        }

        /* If we got a module, keep it */
        nmodule = (gds_pgdstor_module_t*) module;
        /* add to the list of selected modules */
        newmodule = GDS_NEW(gds_pgdstor_base_active_module_t);
        newmodule->pri = priority;
        newmodule->module = nmodule;
        newmodule->component = (gds_pgdstor_base_component_t*)cli->cli_component;

        /* maintain priority order */
        inserted = false;
        GDS_LIST_FOREACH(mod, &gds_pgdstor_globals.actives, gds_pgdstor_base_active_module_t) {
            if (priority > mod->pri) {
                gds_list_insert_pos(&gds_pgdstor_globals.actives,
                                     (gds_list_item_t*)mod, &newmodule->super);
                inserted = true;
                break;
            }
        }
        if (!inserted) {
            /* must be lowest priority - add to end */
            gds_list_append(&gds_pgdstor_globals.actives, &newmodule->super);
        }
    }

    if (4 < gds_output_get_verbosity(gds_pgdstor_base_framework.framework_output)) {
        gds_output(0, "Final pgdstor priorities");
        /* show the prioritized list */
        GDS_LIST_FOREACH(mod, &gds_pgdstor_globals.actives, gds_pgdstor_base_active_module_t) {
            gds_output(0, "\tpgdstor: %s Priority: %d", mod->component->base.gds_mca_component_name, mod->pri);
        }
    }

    return GDS_SUCCESS;;
}
