/*
 * Copyright (c) 2004-2010 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2015      Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2016      Intel, Inc. All rights reserved
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#ifndef GDS_GDL_BASE_H
#define GDS_GDL_BASE_H

#include <src/include/gds_config.h>
#include "src/mca/gdl/gdl.h"
#include "src/util/gds_environ.h"

#include "src/mca/base/base.h"


BEGIN_C_DECLS

/**
 * Globals
 */
extern gds_mca_base_framework_t gds_gdl_base_framework;
extern gds_gdl_base_component_t
*gds_gdl_base_selected_component;
extern gds_gdl_base_module_t *gds_gdl;


/**
 * Initialize the GDL MCA framework
 *
 * @retval GDS_SUCCESS Upon success
 * @retval GDS_ERROR   Upon failures
 *
 * This function is invoked during gds_init();
 */
int gds_gdl_base_open(gds_mca_base_open_flag_t flags);

/**
 * Select an available component.
 *
 * @retval GDS_SUCCESS Upon Success
 * @retval GDS_NOT_FOUND If no component can be selected
 * @retval GDS_ERROR Upon other failure
 *
 */
int gds_gdl_base_select(void);

/**
 * Finalize the GDL MCA framework
 *
 * @retval GDS_SUCCESS Upon success
 * @retval GDS_ERROR   Upon failures
 *
 * This function is invoked during gds_finalize();
 */
int gds_gdl_base_close(void);

/**
 * Open a DSO
 *
 * (see gds_gdl_base_module_open_ft_t in gds/mca/gdl/gdl.h for
 * documentation of this function)
 */
int gds_gdl_open(const char *fname,
                 bool use_ext, bool private_namespace,
                 gds_gdl_handle_t **handle, char **err_msg);

/**
 * Lookup a symbol in a DSO
 *
 * (see gds_gdl_base_module_lookup_ft_t in gds/mca/gdl/gdl.h for
 * documentation of this function)
 */
int gds_gdl_lookup(gds_gdl_handle_t *handle,
                   const char *symbol,
                   void **ptr, char **err_msg);

/**
 * Close a DSO
 *
 * (see gds_gdl_base_module_close_ft_t in gds/mca/gdl/gdl.h for
 * documentation of this function)
 */
int gds_gdl_close(gds_gdl_handle_t *handle);

/**
 * Iterate over files in a path
 *
 * (see gds_gdl_base_module_foreachfile_ft_t in gds/mca/gdl/gdl.h for
 * documentation of this function)
 */
int gds_gdl_foreachfile(const char *search_path,
                        int (*cb_func)(const char *filename, void *context),
                        void *context);

END_C_DECLS

#endif /* GDS_GDL_BASE_H */
