/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2012-2015 Los Alamos National Security, LLC. All rights
 *                         reserved.
 * Copyright (c) 2016      Intel, Inc. All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#if !defined(GDS_MCA_BASE_FRAMEWORK_H)
#define GDS_MCA_BASE_FRAMEWORK_H
#include <src/include/gds_config.h>

#include "src/mca/mca.h"
#include "src/class/gds_list.h"

/*
 * Register and open flags
 */
enum gds_mca_base_register_flag_t {
    GDS_MCA_BASE_REGISTER_DEFAULT     = 0,
    /** Register all components (ignore selection MCA variables) */
    GDS_MCA_BASE_REGISTER_ALL         = 1,
    /** Do not register DSO components */
    GDS_MCA_BASE_REGISTER_STATIC_ONLY = 2
};

typedef enum gds_mca_base_register_flag_t gds_mca_base_register_flag_t;

enum gds_mca_base_open_flag_t {
    GDS_MCA_BASE_OPEN_DEFAULT         = 0,
    /** Find components in gds_mca_base_components_find. Used by
     gds_mca_base_framework_open() when NOREGISTER is specified
     by the framework */
    GDS_MCA_BASE_OPEN_FIND_COMPONENTS = 1,
    /** Do not open DSO components */
    GDS_MCA_BASE_OPEN_STATIC_ONLY     = 2,
};

typedef enum gds_mca_base_open_flag_t gds_mca_base_open_flag_t;


/**
 * Register the MCA framework parameters
 *
 * @param[in] flags Registration flags (see mca/base/base.h)
 *
 * @retval GDS_SUCCESS on success
 * @retval gds error code on failure
 *
 * This function registers all framework MCA parameters. This
 * function should not call gds_mca_base_framework_components_register().
 *
 * Frameworks are NOT required to provide this function. It
 * may be NULL.
 */
typedef int (*gds_mca_base_framework_register_params_fn_t) (gds_mca_base_register_flag_t flags);

/**
 * Initialize the MCA framework
 *
 * @retval GDS_SUCCESS Upon success
 * @retval GDS_ERROR Upon failure
 *
 * This must be the first function invoked in the MCA framework.
 * It initializes the MCA framework, finds and opens components,
 * populates the components list, etc.
 *
 * This function is invoked during gds_init() and during the
 * initialization of the special case of the ompi_info command.
 *
 * This function fills in the components framework value, which
 * is a list of all components that were successfully opened.
 * This variable should \em only be used by other framework base
 * functions or by ompi_info -- it is not considered a public
 * interface member -- and is only mentioned here for completeness.
 *
 * Any resources allocated by this function must be freed
 * in the framework close function.
 *
 * Frameworks are NOT required to provide this function. It may
 * be NULL. If a framework does not provide an open function the
 * default behavior of gds_mca_base_framework_open() is to call
 * gds_mca_base_framework_components_open(). If a framework provides
 * an open function it will need to call gds_mca_base_framework_components_open()
 * if it needs to open any components.
 */
typedef int (*gds_mca_base_framework_open_fn_t) (gds_mca_base_open_flag_t flags);

/**
 * Shut down the MCA framework.
 *
 * @retval GDS_SUCCESS Always
 *
 * This function should shut downs everything in the MCA
 * framework, and is called during gds_finalize() and the
 * special case of the ompi_info command.
 *
 * It must be the last function invoked on the MCA framework.
 *
 * Frameworks are NOT required to provide this function. It may
 * be NULL. If a framework does not provide a close function the
 * default behavior of gds_mca_base_framework_close() is to call
 * gds_mca_base_framework_components_close(). If a framework provide
 * a close function it will need to call gds_mca_base_framework_components_close()
 * if any components were opened.
 */
typedef int (*gds_mca_base_framework_close_fn_t) (void);

typedef enum {
    GDS_MCA_BASE_FRAMEWORK_FLAG_DEFAULT    = 0,
    /** Don't register any variables for this framework */
    GDS_MCA_BASE_FRAMEWORK_FLAG_NOREGISTER = 1,
    /** Internal. Don't set outside gds_mca_base_framework.h */
    GDS_MCA_BASE_FRAMEWORK_FLAG_REGISTERED = 2,
    /** Framework does not have any DSO components */
    GDS_MCA_BASE_FRAMEWORK_FLAG_NO_DSO     = 4,
    /** Internal. Don't set outside gds_mca_base_framework.h */
    GDS_MCA_BASE_FRAMEWORK_FLAG_OPEN       = 8,
    /**
     * The upper 16 bits are reserved for project specific flags.
     */
} gds_mca_base_framework_flags_t;

typedef struct gds_mca_base_framework_t {
    /** Project name for this component (ex "gds") */
    char                                    *framework_project;
    /** Framework name */
    char                                    *framework_name;
    /** Description of this framework or NULL */
    const char                              *framework_description;
    /** Framework register function or NULL if the framework
        and all its components have nothing to register */
    gds_mca_base_framework_register_params_fn_t  framework_register;
    /** Framework open function or NULL */
    gds_mca_base_framework_open_fn_t             framework_open;
    /** Framework close function or NULL */
    gds_mca_base_framework_close_fn_t            framework_close;
    /** Framework flags (future use) set to 0 */
    gds_mca_base_framework_flags_t               framework_flags;
    /** Framework open count */
    int                                      framework_refcnt;
    /** List of static components */
    const gds_mca_base_component_t             **framework_static_components;
    /** Component selection. This will be registered with the MCA
        variable system and should be either NULL (all components) or
        a heap allocated, comma-delimited list of components. */
    char                                    *framework_selection;
    /** Verbosity level (0-100) */
    int                                      framework_verbose;
    /** Pmix output for this framework (or -1) */
    int                                      framework_output;
    /** List of selected components (filled in by gds_mca_base_framework_register()
        or gds_mca_base_framework_open() */
    gds_list_t                              framework_components;
} gds_mca_base_framework_t;


/**
 * Register a framework with MCA.
 *
 * @param[in] framework framework to register
 *
 * @retval GDS_SUCCESS Upon success
 * @retval GDS_ERROR Upon failure
 *
 * Call a framework's register function.
 */
int gds_mca_base_framework_register (gds_mca_base_framework_t *framework,
                                      gds_mca_base_register_flag_t flags);

/**
 * Open a framework
 *
 * @param[in] framework framework to open
 *
 * @retval GDS_SUCCESS Upon success
 * @retval GDS_ERROR Upon failure
 *
 * Call a framework's open function.
 */
int gds_mca_base_framework_open (gds_mca_base_framework_t *framework,
                                  gds_mca_base_open_flag_t flags);

/**
 * Close a framework
 *
 * @param[in] framework framework to close
 *
 * @retval GDS_SUCCESS Upon success
 * @retval GDS_ERROR Upon failure
 *
 * Call a framework's close function.
 */
int gds_mca_base_framework_close (gds_mca_base_framework_t *framework);


/**
 * Check if a framework is already registered
 *
 * @param[in] framework framework to query
 *
 * @retval true if the framework's mca variables are registered
 * @retval false if not
 */
bool gds_mca_base_framework_is_registered (struct gds_mca_base_framework_t *framework);


/**
 * Check if a framework is already open
 *
 * @param[in] framework framework to query
 *
 * @retval true if the framework is open
 * @retval false if not
 */
bool gds_mca_base_framework_is_open (struct gds_mca_base_framework_t *framework);


/**
 * Macro to declare an MCA framework
 *
 * Example:
 *  GDS_MCA_BASE_FRAMEWORK_DECLARE(gds, foo, NULL, gds_foo_open, gds_foo_close, MCA_BASE_FRAMEWORK_FLAG_LAZY)
 */
#define GDS_MCA_BASE_FRAMEWORK_DECLARE(project, name, description, registerfn, openfn, closefn, static_components, flags) \
    gds_mca_base_framework_t project##_##name##_base_framework = {   \
        .framework_project           = #project,                        \
        .framework_name              = #name,                           \
        .framework_description       = description,                     \
        .framework_register          = registerfn,                      \
        .framework_open              = openfn,                          \
        .framework_close             = closefn,                         \
        .framework_flags             = flags,                           \
        .framework_refcnt            = 0,                               \
        .framework_static_components = static_components,               \
        .framework_selection         = NULL,                            \
        .framework_verbose           = 0,                               \
        .framework_output            = -1}

#endif /* GDS_MCA_BASE_FRAMEWORK_H */
