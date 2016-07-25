/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 * Copyright (c) 2004-2007 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2009 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2012-2013 Los Alamos National Security, Inc.  All rights reserved.
 * Copyright (c) 2014-2016 Intel, Inc. All rights reserved.
 * Copyright (c) 2015      Research Organization for Information Science
 *                         and Technology (RIST). All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */
/** @file:
 *
 */
#include <src/include/gds_config.h>

#include <gds_common.h>

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include "src/mca/mca.h"
#include "src/mca/base/base.h"
#include "src/class/gds_list.h"
#include "src/mca/base/gds_mca_base_framework.h"
#include "src/mca/pgdstor/base/base.h"

/*
 * The following file was created by configure.  It contains extern
 * statements and the definition of an array of pointers to each
 * component's public mca_base_component_t struct.
 */

#include "src/mca/pgdstor/base/static-components.h"

/* Instantiate the global vars */
gds_pgdstor_globals_t gds_pgdstor_globals;

static gds_status_t gds_pgdstor_close(void)
{
  gds_pgdstor_base_active_module_t *active;

    if (!gds_pgdstor_globals.initialized) {
        return GDS_SUCCESS;
    }
    gds_pgdstor_globals.initialized = false;

    GDS_LIST_FOREACH(active, &gds_pgdstor_globals.actives, gds_pgdstor_base_active_module_t) {
      if (NULL != active->component->finalize) {
        active->component->finalize();
      }
    }
    GDS_DESTRUCT(&gds_pgdstor_globals.actives);

    return gds_mca_base_framework_components_close(&gds_pgdstor_base_framework, NULL);
}

static gds_status_t gds_pgdstor_open(gds_mca_base_open_flag_t flags)
{
    /* initialize globals */
    gds_pgdstor_globals.initialized = true;
    GDS_CONSTRUCT(&gds_pgdstor_globals.actives, gds_list_t);

    /* Open up all available components */
    return gds_mca_base_framework_components_open(&gds_pgdstor_base_framework, flags);
}

GDS_MCA_BASE_FRAMEWORK_DECLARE(gds, pgdstor, "GDS Security Operations",
                                NULL, gds_pgdstor_open, gds_pgdstor_close,
                                mca_pgdstor_base_static_components, 0);

GDS_CLASS_INSTANCE(gds_pgdstor_base_active_module_t,
                    gds_list_item_t,
                    NULL, NULL);

