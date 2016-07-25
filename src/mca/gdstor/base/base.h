/* -*- C -*-
 *
 * Copyright (c) 2004-2007 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2006 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2012      Los Alamos National Security, Inc.  All rights reserved.
 * Copyright (c) 2014-2016 Intel, Inc. All rights reserved.
 * Copyright (c) 2015      Research Organization for Information Science
 *                         and Technology (RIST). All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 *
 */
#ifndef GDS_GDSTOR_BASE_H_
#define GDS_GDSTOR_BASE_H_

#include <src/include/gds_config.h>


#ifdef HAVE_SYS_TIME_H
#include <sys/time.h> /* for struct timeval */
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#if GDS_HAVE_HWLOC
#include GDS_HWLOC_HEADER
#endif

#include "src/class/gds_pointer_array.h"
#include "src/mca/mca.h"
#include "src/mca/base/gds_mca_base_framework.h"

#include "src/mca/pgdstor/pgdstor.h"


 BEGIN_C_DECLS

/*
 * MCA Framework
 */
extern gds_mca_base_framework_t gds_pgdstor_base_framework;
/**
 * BFROP select function
 *
 * Cycle across available components and construct the list
 * of active modules
 */
gds_status_t gds_pgdstor_base_select(void);

/* Select a pgdstor module for a given peer */
gds_pgdstor_module_t* gds_pgdstor_base_assign_module(const char *options);

/* get a list of available options - caller must free results
 * when done */
char* gds_pgdstor_base_get_available_modules(void);

/**
 * Track an active component / module
 */
struct gds_pgdstor_base_active_module_t {
    gds_list_item_t super;
    int pri;
    gds_pgdstor_module_t *module;
    gds_pgdstor_base_component_t *component;
};
typedef struct gds_pgdstor_base_active_module_t gds_pgdstor_base_active_module_t;
GDS_CLASS_DECLARATION(gds_pgdstor_base_active_module_t);


/* framework globals */
struct gds_pgdstor_globals_t {
    gds_list_t actives;
    bool initialized;
};
typedef struct gds_pgdstor_globals_t gds_pgdstor_globals_t;

extern gds_pgdstor_globals_t gds_pgdstor_globals;

 END_C_DECLS

#endif
