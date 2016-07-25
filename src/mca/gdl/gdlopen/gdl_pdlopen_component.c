/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2015 Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2015      Los Alamos National Security, LLC. All rights
 *                         reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include <src/include/gds_config.h>

#include "gds_common.h"
#include "src/mca/gdl/gdl.h"
#include "src/util/argv.h"

#include "gdl_gdlopen.h"


/*
 * Public string showing the sysinfo ompi_linux component version number
 */
const char *gds_gdl_gdlopen_component_version_string =
    "GDS gdl gdlopen MCA component version " GDS_VERSION;


/*
 * Local functions
 */
static int gdlopen_component_register(void);
static int gdlopen_component_open(void);
static int gdlopen_component_close(void);
static int gdlopen_component_query(gds_mca_base_module_t **module, int *priority);

/*
 * Instantiate the public struct with all of our public information
 * and pointers to our public functions in it
 */

gds_gdl_gdlopen_component_t mca_gdl_gdlopen_component = {

    /* Fill in the mca_gdl_base_component_t */
    .base = {

        /* First, the mca_component_t struct containing meta information
           about the component itself */
        .base_version = {
            GDS_GDL_BASE_VERSION_1_0_0,

            /* Component name and version */
            .gds_mca_component_name = "gdlopen",
            GDS_MCA_BASE_MAKE_VERSION(component, GDS_MAJOR_VERSION, GDS_MINOR_VERSION,
                                       GDS_RELEASE_VERSION),

            /* Component functions */
            .gds_mca_register_component_params = gdlopen_component_register,
            .gds_mca_open_component = gdlopen_component_open,
            .gds_mca_close_component = gdlopen_component_close,
            .gds_mca_query_component = gdlopen_component_query,
        },

        .base_data = {
            /* The component is checkpoint ready */
            GDS_MCA_BASE_METADATA_PARAM_CHECKPOINT
        },

        /* The gdl framework members */
        .priority = 80
    },
};


static int gdlopen_component_register(void)
{
    int ret;

    mca_gdl_gdlopen_component.filename_suffixes_mca_storage = ".so,.dylib,.dll,.sl";
    ret =
        gds_mca_base_component_var_register(&mca_gdl_gdlopen_component.base.base_version,
                                             "filename_suffixes",
                                             "Comma-delimited list of filename suffixes that the gdlopen component will try",
                                             GDS_MCA_BASE_VAR_TYPE_STRING,
                                             NULL,
                                             0,
                                             GDS_MCA_BASE_VAR_FLAG_SETTABLE,
                                             GDS_INFO_LVL_5,
                                             GDS_MCA_BASE_VAR_SCOPE_LOCAL,
                                             &mca_gdl_gdlopen_component.filename_suffixes_mca_storage);
    if (ret < 0) {
        return ret;
    }
    mca_gdl_gdlopen_component.filename_suffixes =
        gds_argv_split(mca_gdl_gdlopen_component.filename_suffixes_mca_storage,
                        ',');

    return GDS_SUCCESS;
}

static int gdlopen_component_open(void)
{
    return GDS_SUCCESS;
}


static int gdlopen_component_close(void)
{
    if (NULL != mca_gdl_gdlopen_component.filename_suffixes) {
        gds_argv_free(mca_gdl_gdlopen_component.filename_suffixes);
        mca_gdl_gdlopen_component.filename_suffixes = NULL;
    }

    return GDS_SUCCESS;
}


static int gdlopen_component_query(gds_mca_base_module_t **module, int *priority)
{
    /* The priority value is somewhat meaningless here; by
       gds/mca/gdl/configure.m4, there's at most one component
       available. */
    *priority = mca_gdl_gdlopen_component.base.priority;
    *module = &gds_gdl_gdlopen_module.super;

    return GDS_SUCCESS;
}
