/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2004-2008 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2007 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2009      Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2013-2015 Los Alamos National Security, LLC. All rights
 *                         reserved.
 * Copyright (c) 2015      Research Organization for Information Science
 *                         and Technology (RIST). All rights reserved.
 * Copyright (c) 2016      Intel, Inc. All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#ifndef MCA_BASE_H
#define MCA_BASE_H

#include <src/include/gds_config.h>

#include "src/class/gds_object.h"
#include "src/class/gds_list.h"

/*
 * These units are large enough to warrant their own .h files
 */
#include "src/mca/mca.h"
#include "src/mca/base/gds_mca_base_var.h"
#include "src/mca/base/gds_mca_base_framework.h"
#include "src/util/output.h"

BEGIN_C_DECLS

/*
 * Structure for making plain lists of components
 */
struct gds_mca_base_component_list_item_t {
    gds_list_item_t super;
    const gds_mca_base_component_t *cli_component;
};
typedef struct gds_mca_base_component_list_item_t gds_mca_base_component_list_item_t;
GDS_CLASS_DECLARATION(gds_mca_base_component_list_item_t);

/*
 * Structure for making priority lists of components
 */
struct gds_mca_base_component_priority_list_item_t {
    gds_mca_base_component_list_item_t super;
    int cpli_priority;
};
typedef struct gds_mca_base_component_priority_list_item_t
    gds_mca_base_component_priority_list_item_t;

GDS_CLASS_DECLARATION(gds_mca_base_component_priority_list_item_t);

/*
 * Public variables
 */
extern char *gds_mca_base_component_path;
extern bool gds_mca_base_component_show_load_errors;
extern bool gds_mca_base_component_disable_dlopen;
extern char *gds_mca_base_system_default_path;
extern char *gds_mca_base_user_default_path;

/*
 * Standard verbosity levels
 */
enum {
    /** total silence */
    GDS_MCA_BASE_VERBOSE_NONE  = -1,
    /** only errors are printed */
    GDS_MCA_BASE_VERBOSE_ERROR = 0,
    /** emit messages about component selection, open, and unloading */
    GDS_MCA_BASE_VERBOSE_COMPONENT = 10,
    /** also emit warnings */
    GDS_MCA_BASE_VERBOSE_WARN  = 20,
    /** also emit general, user-relevant information, such as rationale as to why certain choices
     * or code paths were taken, information gleaned from probing the local system, etc. */
    GDS_MCA_BASE_VERBOSE_INFO  = 40,
    /** also emit relevant tracing information (e.g., which functions were invoked /
     * call stack entry/exit info) */
    GDS_MCA_BASE_VERBOSE_TRACE = 60,
    /** also emit GDS-developer-level (i.e,. highly detailed) information */
    GDS_MCA_BASE_VERBOSE_DEBUG = 80,
    /** also output anything else that might be useful */
    GDS_MCA_BASE_VERBOSE_MAX   = 100,
};

/*
 * Public functions
 */

/**
 * First function called in the MCA.
 *
 * @return GDS_SUCCESS Upon success
 * @return GDS_ERROR Upon failure
 *
 * This function starts up the entire MCA.  It initializes a bunch
 * of built-in MCA parameters, and initialized the MCA component
 * repository.
 *
 * It must be the first MCA function invoked.  It is normally
 * invoked during the initialization stage and specifically
 * invoked in the special case of the *_info command.
 */
int gds_mca_base_open(void);

/**
 * Last function called in the MCA
 *
 * @return GDS_SUCCESS Upon success
 * @return GDS_ERROR Upon failure
 *
 * This function closes down the entire MCA.  It clears all MCA
 * parameters and closes down the MCA component respository.
 *
 * It must be the last MCA function invoked.  It is normally invoked
 * during the finalize stage.
 */
int gds_mca_base_close(void);

/**
 * A generic select function
 *
 */
int gds_mca_base_select(const char *type_name, int output_id,
                         gds_list_t *components_available,
                         gds_mca_base_module_t **best_module,
                         gds_mca_base_component_t **best_component,
                         int *priority_out);

/**
 * A function for component query functions to discover if they have
 * been explicitly required to or requested to be selected.
 *
 * exclusive: If the specified component is the only component that is
 *            available for selection.
 *
 */
int gds_mca_base_is_component_required(gds_list_t *components_available,
                                        gds_mca_base_component_t *component,
                                        bool exclusive,
                                        bool *is_required);

/* gds_mca_base_component_compare.c */

int gds_mca_base_component_compare_priority(gds_mca_base_component_priority_list_item_t *a,
                                             gds_mca_base_component_priority_list_item_t *b);
int gds_mca_base_component_compare(const gds_mca_base_component_t *a,
                                    const gds_mca_base_component_t *b);
int gds_mca_base_component_compatible(const gds_mca_base_component_t *a,
                                       const gds_mca_base_component_t *b);
char * gds_mca_base_component_to_string(const gds_mca_base_component_t *a);

/* gds_mca_base_component_find.c */

int gds_mca_base_component_find (const char *directory, gds_mca_base_framework_t *framework,
                                  bool ignore_requested, bool open_dso_components);

/**
 * Parse the requested component string and return an gds_argv of the requested
 * (or not requested) components.
 */
int gds_mca_base_component_parse_requested (const char *requested, bool *include_mode,
                                             char ***requested_component_names);

/**
 * Filter a list of components based on a comma-delimted list of names and/or
 * a set of meta-data flags.
 *
 * @param[in,out] components List of components to filter
 * @param[in] output_id Output id to write to for error/warning/debug messages
 * @param[in] filter_names Comma delimited list of components to use. Negate with ^.
 * May be NULL.
 * @param[in] filter_flags Metadata flags components are required to have set (CR ready)
 *
 * @returns GDS_SUCCESS On success
 * @returns GDS_ERR_NOT_FOUND If some component in {filter_names} is not found in
 * {components}. Does not apply to negated filters.
 * @returns gds error code On other error.
 *
 * This function closes and releases any components that do not match the filter_name and
 * filter flags.
 */
int gds_mca_base_components_filter (gds_mca_base_framework_t *framework, uint32_t filter_flags);



/* Safely release some memory allocated by gds_mca_base_component_find()
   (i.e., is safe to call even if you never called
   gds_mca_base_component_find()). */
int gds_mca_base_component_find_finalize(void);

/* gds_mca_base_components_register.c */
int gds_mca_base_framework_components_register (struct gds_mca_base_framework_t *framework,
                                                 gds_mca_base_register_flag_t flags);

/* gds_mca_base_components_open.c */
int gds_mca_base_framework_components_open (struct gds_mca_base_framework_t *framework,
                                             gds_mca_base_open_flag_t flags);

int gds_mca_base_components_open(const char *type_name, int output_id,
                                  const gds_mca_base_component_t **static_components,
                                  gds_list_t *components_available,
                                  bool open_dso_components);

/* gds_mca_base_components_close.c */
/**
 * Close and release a component.
 *
 * @param[in] component Component to close
 * @param[in] output_id Output id for debugging output
 *
 * After calling this function the component may no longer be used.
 */
void gds_mca_base_component_close (const gds_mca_base_component_t *component, int output_id);

/**
 * Release a component without closing it.
 * @param[in] component Component to close
 * @param[in] output_id Output id for debugging output
 *
 * After calling this function the component may no longer be used.
 */
void gds_mca_base_component_unload (const gds_mca_base_component_t *component, int output_id);

int gds_mca_base_components_close(int output_id, gds_list_t *components_available,
                                   const gds_mca_base_component_t *skip);

int gds_mca_base_framework_components_close (struct gds_mca_base_framework_t *framework,
                                              const gds_mca_base_component_t *skip);

END_C_DECLS

#endif /* MCA_BASE_H */
