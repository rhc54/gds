/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2012-2015 Los Alamos National Security, LLC. All rights
 *                         reserved.
 * Copyright (c) 2015      Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2016      Intel, Inc. All rights reserved
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include <src/include/gds_config.h>

#include "gds_common.h"
#include "src/util/output.h"

#include "gds_mca_base_framework.h"
#include "gds_mca_base_var.h"
#include "src/mca/base/base.h"

bool gds_mca_base_framework_is_registered (struct gds_mca_base_framework_t *framework)
{
    return !!(framework->framework_flags & GDS_MCA_BASE_FRAMEWORK_FLAG_REGISTERED);
}

bool gds_mca_base_framework_is_open (struct gds_mca_base_framework_t *framework)
{
    return !!(framework->framework_flags & GDS_MCA_BASE_FRAMEWORK_FLAG_OPEN);
}

static void framework_open_output (struct gds_mca_base_framework_t *framework)
{
    if (0 < framework->framework_verbose) {
        if (-1 == framework->framework_output) {
            framework->framework_output = gds_output_open (NULL);
        }
        gds_output_set_verbosity(framework->framework_output,
                                  framework->framework_verbose);
    } else if (-1 != framework->framework_output) {
        gds_output_close (framework->framework_output);
        framework->framework_output = -1;
    }
}

static void framework_close_output (struct gds_mca_base_framework_t *framework)
{
    if (-1 != framework->framework_output) {
        gds_output_close (framework->framework_output);
        framework->framework_output = -1;
    }
}

int gds_mca_base_framework_register (struct gds_mca_base_framework_t *framework,
                                 gds_mca_base_register_flag_t flags)
{
    char *desc;
    int ret;

    assert (NULL != framework);

    framework->framework_refcnt++;

    if (gds_mca_base_framework_is_registered (framework)) {
        return GDS_SUCCESS;
    }

    GDS_CONSTRUCT(&framework->framework_components, gds_list_t);

    if (framework->framework_flags & GDS_MCA_BASE_FRAMEWORK_FLAG_NO_DSO) {
        flags |= GDS_MCA_BASE_REGISTER_STATIC_ONLY;
    }

    if (!(GDS_MCA_BASE_FRAMEWORK_FLAG_NOREGISTER & framework->framework_flags)) {
        /* register this framework with the MCA variable system */
        ret = gds_mca_base_var_group_register (framework->framework_project,
                                           framework->framework_name,
                                           NULL, framework->framework_description);
        if (0 > ret) {
            return ret;
        }

        asprintf (&desc, "Default selection set of components for the %s framework (<none>"
                  " means use all components that can be found)", framework->framework_name);
        ret = gds_mca_base_var_register (framework->framework_project, framework->framework_name,
                                     NULL, NULL, desc, GDS_MCA_BASE_VAR_TYPE_STRING, NULL, 0,
                                     GDS_MCA_BASE_VAR_FLAG_SETTABLE, GDS_INFO_LVL_2,
                                     GDS_MCA_BASE_VAR_SCOPE_ALL_EQ, &framework->framework_selection);
        free (desc);
        if (0 > ret) {
            return ret;
        }

        /* register a verbosity variable for this framework */
        ret = asprintf (&desc, "Verbosity level for the %s framework (default: 0)",
                        framework->framework_name);
        if (0 > ret) {
            return GDS_ERR_OUT_OF_RESOURCE;
        }

        framework->framework_verbose = GDS_MCA_BASE_VERBOSE_ERROR;
        ret = gds_mca_base_framework_var_register (framework, "verbose", desc,
                                               GDS_MCA_BASE_VAR_TYPE_INT,
                                               &gds_mca_base_var_enum_verbose, 0,
                                               GDS_MCA_BASE_VAR_FLAG_SETTABLE,
                                               GDS_INFO_LVL_8,
                                               GDS_MCA_BASE_VAR_SCOPE_LOCAL,
                                               &framework->framework_verbose);
        free(desc);
        if (0 > ret) {
            return ret;
        }

        /* check the initial verbosity and open the output if necessary. we
           will recheck this on open */
        framework_open_output (framework);

        /* register framework variables */
        if (NULL != framework->framework_register) {
            ret = framework->framework_register (flags);
            if (GDS_SUCCESS != ret) {
                return ret;
            }
        }

        /* register components variables */
        ret = gds_mca_base_framework_components_register (framework, flags);
        if (GDS_SUCCESS != ret) {
            return ret;
        }
    }

    framework->framework_flags |= GDS_MCA_BASE_FRAMEWORK_FLAG_REGISTERED;

    /* framework did not provide a register function */
    return GDS_SUCCESS;
}

int gds_mca_base_framework_open (struct gds_mca_base_framework_t *framework,
                             gds_mca_base_open_flag_t flags) {
    int ret;

    assert (NULL != framework);

    /* register this framework before opening it */
    ret = gds_mca_base_framework_register (framework, GDS_MCA_BASE_REGISTER_DEFAULT);
    if (GDS_SUCCESS != ret) {
        return ret;
    }

    /* check if this framework is already open */
    if (gds_mca_base_framework_is_open (framework)) {
        return GDS_SUCCESS;
    }

    if (GDS_MCA_BASE_FRAMEWORK_FLAG_NOREGISTER & framework->framework_flags) {
        flags |= GDS_MCA_BASE_OPEN_FIND_COMPONENTS;

        if (GDS_MCA_BASE_FRAMEWORK_FLAG_NO_DSO & framework->framework_flags) {
            flags |= GDS_MCA_BASE_OPEN_STATIC_ONLY;
        }
    }

    /* lock all of this frameworks's variables */
    ret = gds_mca_base_var_group_find (framework->framework_project,
                                   framework->framework_name,
                                   NULL);
    gds_mca_base_var_group_set_var_flag (ret, GDS_MCA_BASE_VAR_FLAG_SETTABLE, false);

    /* check the verbosity level and open (or close) the output */
    framework_open_output (framework);

    if (NULL != framework->framework_open) {
        ret = framework->framework_open (flags);
    } else {
        ret = gds_mca_base_framework_components_open (framework, flags);
    }

    if (GDS_SUCCESS != ret) {
        framework->framework_refcnt--;
    } else {
        framework->framework_flags |= GDS_MCA_BASE_FRAMEWORK_FLAG_OPEN;
    }

    return ret;
}

int gds_mca_base_framework_close (struct gds_mca_base_framework_t *framework) {
    bool is_open = gds_mca_base_framework_is_open (framework);
    bool is_registered = gds_mca_base_framework_is_registered (framework);
    int ret, group_id;

    assert (NULL != framework);

    if (!(is_open || is_registered)) {
        return GDS_SUCCESS;
    }

    assert (framework->framework_refcnt);
    if (--framework->framework_refcnt) {
        return GDS_SUCCESS;
    }

    /* find and deregister all component groups and variables */
    group_id = gds_mca_base_var_group_find (framework->framework_project,
                                        framework->framework_name, NULL);
    if (0 <= group_id) {
        (void) gds_mca_base_var_group_deregister (group_id);
    }

    /* close the framework and all of its components */
    if (is_open) {
        if (NULL != framework->framework_close) {
            ret = framework->framework_close ();
        } else {
            ret = gds_mca_base_framework_components_close (framework, NULL);
        }

        if (GDS_SUCCESS != ret) {
            return ret;
        }
    } else {
        gds_list_item_t *item;
        while (NULL != (item = gds_list_remove_first (&framework->framework_components))) {
            gds_mca_base_component_list_item_t *cli;
            cli = (gds_mca_base_component_list_item_t*) item;
            gds_mca_base_component_unload(cli->cli_component,
                                           framework->framework_output);
            GDS_RELEASE(item);
        }
        ret = GDS_SUCCESS;
    }

    framework->framework_flags &= ~(GDS_MCA_BASE_FRAMEWORK_FLAG_REGISTERED | GDS_MCA_BASE_FRAMEWORK_FLAG_OPEN);

    GDS_DESTRUCT(&framework->framework_components);

    framework_close_output (framework);

    return ret;
}
