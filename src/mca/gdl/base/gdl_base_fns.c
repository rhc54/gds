/*
 * Copyright (c) 2004-2010 The Trustees of Indiana University.
 *                         All rights reserved.
 * Copyright (c) 2015      Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2016      Intel, Inc. All rights reserved
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

/**
 * This file is a simple set of wrappers around the selected GDS GDL
 * component (it's a compile-time framework with, at most, a single
 * component; see gdl.h for details).
 */

#include <src/include/gds_config.h>

#include "gds_common.h"

#include "src/util/output.h"
#include "src/mca/gdl/base/base.h"


int gds_gdl_open(const char *fname,
                 bool use_ext, bool private_namespace,
                 gds_gdl_handle_t **handle, char **err_msg)
{
    *handle = NULL;

    if (NULL != gds_gdl && NULL != gds_gdl->open) {
        return gds_gdl->open(fname, use_ext, private_namespace,
                             handle, err_msg);
    }

    return GDS_ERR_NOT_SUPPORTED;
}

int gds_gdl_lookup(gds_gdl_handle_t *handle,
                   const char *symbol,
                   void **ptr, char **err_msg)
{
    if (NULL != gds_gdl && NULL != gds_gdl->lookup) {
        return gds_gdl->lookup(handle, symbol, ptr, err_msg);
    }

    return GDS_ERR_NOT_SUPPORTED;
}

int gds_gdl_close(gds_gdl_handle_t *handle)
{
    if (NULL != gds_gdl && NULL != gds_gdl->close) {
        return gds_gdl->close(handle);
    }

    return GDS_ERR_NOT_SUPPORTED;
}

int gds_gdl_foreachfile(const char *search_path,
                        int (*cb_func)(const char *filename, void *context),
                        void *context)
{
    if (NULL != gds_gdl && NULL != gds_gdl->foreachfile) {
       return gds_gdl->foreachfile(search_path, cb_func, context);
    }

    return GDS_ERR_NOT_SUPPORTED;
}
