/*
 * Copyright (c) 2010      Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2012      Los Alamos National Security, Inc. All rights reserved.
 * Copyright (c) 2013      Intel, Inc. All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 *
 * These symbols are in a file by themselves to provide nice linker
 * semantics.  Since linkers generally pull in symbols by object
 * files, keeping these symbols as the only symbols in this file
 * prevents utility programs such as "ompi_info" from having to import
 * entire components just to query their version and parameters.
 */

#include "gds_config.h"
#include "gds/constants.h"

#include "gds/mca/base/base.h"

#include "gds/mca/gdstor/gdstor.h"
#include "gds/mca/gdstor/base/base.h"
#include "gdstor_lhash.h"

static int gdstor_lhash_component_open(void);
static int gdstor_lhash_component_query(gds_gdstor_base_module_t **module,
                                   int *store_priority,
                                   int *fetch_priority,
                                   bool restrict_local);
static int gdstor_lhash_component_close(void);
static int gdstor_lhash_component_register(void);

/*
 * Instantiate the public struct with all of our public information
 * and pointers to our public functions in it
 */
gds_gdstor_base_component_t mca_gdstor_lhash_component = {
    {
        GDS_GDSTOR_BASE_VERSION_1_0_0,

        /* Component name and version */
        "lhash",
        GDS_MAJOR_VERSION,
        GDS_MINOR_VERSION,
        GDS_RELEASE_VERSION,

        /* Component open and close functions */
        gdstor_lhash_component_open,
        gdstor_lhash_component_close,
        NULL,
        gdstor_lhash_component_register
    },
    {
        /* The component is checkpoint ready */
        MCA_BASE_METADATA_PARAM_CHECKPOINT
    },
    gdstor_lhash_component_query
};

/* we should be the last place to store data as
 * it usually is stored globally, then can fall
 * down to us if it is internal
 */
static int my_store_priority = 1;
/* we should be the first place to look for data
 * in case we already have it - then try to fetch
 * it globally if we don't
 */
static int my_fetch_priority = 100;

static int gdstor_lhash_component_open(void)
{
    return GDS_SUCCESS;
}

static int gdstor_lhash_component_query(gds_gdstor_base_module_t **module,
                                   int *store_priority,
                                   int *fetch_priority,
                                   bool restrict_local)
{
    /* we are the default - the ESS modules will set the gdstor selection
     * envar if they need someone else
     */
    *store_priority = my_store_priority;
    *fetch_priority = my_fetch_priority;
    *module = &gds_gdstor_lhash_module;
    return GDS_SUCCESS;
}


static int gdstor_lhash_component_close(void)
{
    return GDS_SUCCESS;
}

static int gdstor_lhash_component_register(void)
{
    mca_base_component_t *c = &mca_gdstor_lhash_component.base_version;

    my_store_priority = 1;
    (void) mca_base_component_var_register(c, "store_priority",
                                           "Priority dictating order in which store commands will given to database components",
                                           MCA_BASE_VAR_TYPE_INT, NULL, 0, 0,
                                           GDS_INFO_LVL_9,
                                           MCA_BASE_VAR_SCOPE_READONLY,
                                           &my_store_priority);

    my_fetch_priority = 100;
    (void) mca_base_component_var_register(c, "fetch_priority",
                                           "Priority dictating order in which fetch commands will given to database components",
                                           MCA_BASE_VAR_TYPE_INT, NULL, 0, 0,
                                           GDS_INFO_LVL_9,
                                           MCA_BASE_VAR_SCOPE_READONLY,
                                           &my_fetch_priority);

    return GDS_SUCCESS;
}
