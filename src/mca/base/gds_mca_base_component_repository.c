/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2004-2005 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2005 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2008-2015 Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2015      Los Alamos National Security, LLC. All rights
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


#include <src/include/gds_config.h>
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "src/class/gds_list.h"
#include "src/mca/mca.h"
#include "src/mca/base/base.h"
#include "src/mca/base/gds_mca_base_component_repository.h"
#include "src/mca/gdl/base/base.h"
#include "gds_common.h"
#include "src/class/gds_hash_table.h"
#include "src/util/basename.h"

#if GDS_HAVE_GDL_SUPPORT

/*
 * Private types
 */
static void ri_constructor(gds_mca_base_component_repository_item_t *ri);
static void ri_destructor(gds_mca_base_component_repository_item_t *ri);
GDS_CLASS_INSTANCE(gds_mca_base_component_repository_item_t, gds_list_item_t,
                    ri_constructor, ri_destructor);

#endif /* GDS_HAVE_GDL_SUPPORT */


/*
 * Private variables
 */
static bool initialized = false;


#if GDS_HAVE_GDL_SUPPORT

static gds_hash_table_t gds_mca_base_component_repository;

/* two-level macro for stringifying a number */
#define STRINGIFYX(x) #x
#define STRINGIFY(x) STRINGIFYX(x)

static int process_repository_item (const char *filename, void *data)
{
    char name[GDS_MCA_BASE_MAX_COMPONENT_NAME_LEN + 1];
    char type[GDS_MCA_BASE_MAX_TYPE_NAME_LEN + 1];
    gds_mca_base_component_repository_item_t *ri;
    gds_list_t *component_list;
    char *base;
    int ret;

    base = gds_basename (filename);
    if (NULL == base) {
        return GDS_ERROR;
    }

    /* check if the plugin has the appropriate prefix */
    if (0 != strncmp (base, "mca_", 4)) {
        free (base);
        return GDS_SUCCESS;
    }

    /* read framework and component names. framework names may not include an _
     * but component names may */
    ret = sscanf(base, "mca_%" STRINGIFY(GDS_MCA_BASE_MAX_TYPE_NAME_LEN) "[^_]_%"
                 STRINGIFY(GDS_MCA_BASE_MAX_COMPONENT_NAME_LEN) "s", type, name);
    if (0 > ret) {
        /* does not patch the expected template. skip */
        free(base);
        return GDS_SUCCESS;
    }

    /* lookup the associated framework list and create if it doesn't already exist */
    ret = gds_hash_table_get_value_ptr(&gds_mca_base_component_repository, type,
                                        strlen (type), (void **) &component_list);
    if (GDS_SUCCESS != ret) {
        component_list = GDS_NEW(gds_list_t);
        if (NULL == component_list) {
            free (base);
            /* OOM. nothing to do but fail */
            return GDS_ERR_OUT_OF_RESOURCE;
        }

        ret = gds_hash_table_set_value_ptr(&gds_mca_base_component_repository, type,
                                            strlen (type), (void *) component_list);
        if (GDS_SUCCESS != ret) {
            free (base);
            GDS_RELEASE(component_list);
            return ret;
        }
    }

    /* check for duplicate components */
    GDS_LIST_FOREACH(ri, component_list, gds_mca_base_component_repository_item_t) {
        if (0 == strcmp (ri->ri_name, name)) {
            /* already scanned this component */
            free (base);
            return GDS_SUCCESS;
        }
    }

    ri = GDS_NEW(gds_mca_base_component_repository_item_t);
    if (NULL == ri) {
        free (base);
        return GDS_ERR_OUT_OF_RESOURCE;
    }

    ri->ri_base = base;

    ri->ri_path = strdup (filename);
    if (NULL == ri->ri_path) {
        GDS_RELEASE(ri);
        return GDS_ERR_OUT_OF_RESOURCE;
    }

    /* strncpy does not guarantee a \0 */
    ri->ri_type[GDS_MCA_BASE_MAX_TYPE_NAME_LEN] = '\0';
    strncpy (ri->ri_type, type, GDS_MCA_BASE_MAX_TYPE_NAME_LEN);

    ri->ri_name[GDS_MCA_BASE_MAX_TYPE_NAME_LEN] = '\0';
    strncpy (ri->ri_name, name, GDS_MCA_BASE_MAX_COMPONENT_NAME_LEN);

    gds_list_append (component_list, &ri->super);

    return GDS_SUCCESS;
}

static int file_exists(const char *filename, const char *ext)
{
    char *final;
    int ret;

    if (NULL == ext) {
        return access (filename, F_OK) == 0;
    }

    ret = asprintf(&final, "%s.%s", filename, ext);
    if (0 > ret || NULL == final) {
        return 0;
    }

    ret = access (final, F_OK);
    free(final);
    return (0 == ret);
}

#endif /* GDS_HAVE_GDL_SUPPORT */

int gds_mca_base_component_repository_add (const char *path)
{
#if GDS_HAVE_GDL_SUPPORT
    char *path_to_use = NULL, *dir, *ctx;
    const char sep[] = {GDS_ENV_SEP, '\0'};

    if (NULL == path) {
        /* nothing to do */
        return GDS_SUCCESS;
    }

    path_to_use = strdup (path);

    dir = strtok_r (path_to_use, sep, &ctx);
    do {
        if ((0 == strcmp(dir, "USER_DEFAULT") || 0 == strcmp(dir, "USR_DEFAULT"))
            && NULL != gds_mca_base_user_default_path) {
            dir = gds_mca_base_user_default_path;
        } else if (0 == strcmp(dir, "SYS_DEFAULT") ||
                   0 == strcmp(dir, "SYSTEM_DEFAULT")) {
            dir = gds_mca_base_system_default_path;
        }

        if (0 != gds_gdl_foreachfile(dir, process_repository_item, NULL)) {
            break;
        }
    } while (NULL != (dir = strtok_r (NULL, sep, &ctx)));

    free (path_to_use);

#endif /* GDS_HAVE_GDL_SUPPORT */

    return GDS_SUCCESS;
}


/*
 * Initialize the repository
 */
int gds_mca_base_component_repository_init(void)
{
  /* Setup internal structures */

  if (!initialized) {
#if GDS_HAVE_GDL_SUPPORT

    /* Initialize the dl framework */
    int ret = gds_mca_base_framework_open(&gds_gdl_base_framework, 0);
    if (GDS_SUCCESS != ret) {
        gds_output(0, "%s %d:%s failed -- process will likely abort (open the dl framework returned %d instead of GDS_SUCCESS)\n",
                    __FILE__, __LINE__, __func__, ret);
        return ret;
    }
    gds_gdl_base_select();

    GDS_CONSTRUCT(&gds_mca_base_component_repository, gds_hash_table_t);
    ret = gds_hash_table_init (&gds_mca_base_component_repository, 128);
    if (GDS_SUCCESS != ret) {
        (void) gds_mca_base_framework_close(&gds_gdl_base_framework);
        return ret;
    }

    ret = gds_mca_base_component_repository_add(gds_mca_base_component_path);
    if (GDS_SUCCESS != ret) {
        GDS_DESTRUCT(&gds_mca_base_component_repository);
        (void) gds_mca_base_framework_close(&gds_gdl_base_framework);
        return ret;
    }
#endif

    initialized = true;
  }

  /* All done */

  return GDS_SUCCESS;
}

int gds_mca_base_component_repository_get_components (gds_mca_base_framework_t *framework,
                                                       gds_list_t **framework_components)
{
    *framework_components = NULL;
#if GDS_HAVE_GDL_SUPPORT
    return gds_hash_table_get_value_ptr (&gds_mca_base_component_repository, framework->framework_name,
                                          strlen (framework->framework_name), (void **) framework_components);
#endif
    return GDS_ERR_NOT_FOUND;
}

#if GDS_HAVE_GDL_SUPPORT
static void gds_mca_base_component_repository_release_internal(gds_mca_base_component_repository_item_t *ri) {
    int group_id;

    group_id = gds_mca_base_var_group_find (NULL, ri->ri_type, ri->ri_name);
    if (0 <= group_id) {
        /* ensure all variables are deregistered before we dlclose the component */
        gds_mca_base_var_group_deregister (group_id);
    }

    /* Close the component (and potentially unload it from memory */
    if (ri->ri_dlhandle) {
        gds_gdl_close(ri->ri_dlhandle);
        ri->ri_dlhandle = NULL;
    }
}
#endif

#if GDS_HAVE_GDL_SUPPORT
static gds_mca_base_component_repository_item_t *find_component(const char *type, const char *name)
{
    gds_mca_base_component_repository_item_t *ri;
    gds_list_t *component_list;
    int ret;

    ret = gds_hash_table_get_value_ptr (&gds_mca_base_component_repository, type,
                                         strlen (type), (void **) &component_list);
    if (GDS_SUCCESS != ret) {
        /* component does not exist in the repository */
        return NULL;
    }

    GDS_LIST_FOREACH(ri, component_list, gds_mca_base_component_repository_item_t) {
        if (0 == strcmp (ri->ri_name, name)) {
            return ri;
        }
    }

    return NULL;
}
#endif

void gds_mca_base_component_repository_release(const gds_mca_base_component_t *component)
{
#if GDS_HAVE_GDL_SUPPORT
    gds_mca_base_component_repository_item_t *ri;

    ri = find_component (component->gds_mca_type_name, component->gds_mca_component_name);
    if (NULL != ri && !(--ri->ri_refcnt)) {
        gds_mca_base_component_repository_release_internal (ri);
    }
#endif
}

int gds_mca_base_component_repository_retain_component(const char *type, const char *name)
{
#if GDS_HAVE_GDL_SUPPORT
    gds_mca_base_component_repository_item_t *ri = find_component(type, name);

    if (NULL != ri) {
        ++ri->ri_refcnt;
        return GDS_SUCCESS;
    }

    return GDS_ERR_NOT_FOUND;
#else
    return GDS_ERR_NOT_SUPPORTED;
#endif
}

int gds_mca_base_component_repository_open(gds_mca_base_framework_t *framework,
                                            gds_mca_base_component_repository_item_t *ri)
{
#if GDS_HAVE_GDL_SUPPORT
    gds_mca_base_component_t *component_struct;
    gds_mca_base_component_list_item_t *mitem = NULL;
    char *struct_name = NULL;
    int vl, ret;

    gds_output_verbose(GDS_MCA_BASE_VERBOSE_INFO, 0, "gds_mca_base_component_repository_open: examining dynamic "
                        "%s MCA component \"%s\" at path %s", ri->ri_type, ri->ri_name, ri->ri_path);

    vl = gds_mca_base_component_show_load_errors ? GDS_MCA_BASE_VERBOSE_ERROR : GDS_MCA_BASE_VERBOSE_INFO;

    /* Ensure that this component is not already loaded (should only happen
       if it was statically loaded).  It's an error if it's already
       loaded because we're evaluating this file -- not this component.
       Hence, returning GDS_ERR_PARAM indicates that the *file* failed
       to load, not the component. */

    GDS_LIST_FOREACH(mitem, &framework->framework_components, gds_mca_base_component_list_item_t) {
        if (0 == strcmp(mitem->cli_component->gds_mca_component_name, ri->ri_name)) {
            gds_output_verbose (GDS_MCA_BASE_VERBOSE_INFO, 0, "gds_mca_base_component_repository_open: already loaded (ignored)");
            return GDS_ERR_BAD_PARAM;
        }
    }

    /* silence coverity issue (invalid free) */
    mitem = NULL;

    if (NULL != ri->ri_dlhandle) {
        gds_output_verbose (GDS_MCA_BASE_VERBOSE_INFO, 0, "gds_mca_base_component_repository_open: already loaded. returning cached component");
        mitem = GDS_NEW(gds_mca_base_component_list_item_t);
        if (NULL == mitem) {
            return GDS_ERR_OUT_OF_RESOURCE;
        }

        mitem->cli_component = ri->ri_component_struct;
        gds_list_append (&framework->framework_components, &mitem->super);

        return GDS_SUCCESS;
    }

    if (0 != strcmp (ri->ri_type, framework->framework_name)) {
        /* shouldn't happen. attempting to open a component belonging to
         * another framework. if this happens it is likely a MCA base
         * bug so assert */
        assert (0);
        return GDS_ERR_NOT_SUPPORTED;
    }

    /* Now try to load the component */

    char *err_msg = NULL;
    if (GDS_SUCCESS != gds_gdl_open(ri->ri_path, true, false, &ri->ri_dlhandle, &err_msg)) {
        if (NULL == err_msg) {
            err_msg = "gds_dl_open() error message was NULL!";
        }
        /* Because libltdl erroneously says "file not found" for any
           type of error -- which is especially misleading when the file
           is actually there but cannot be opened for some other reason
           (e.g., missing symbol) -- do some simple huersitics and if
           the file [probably] does exist, print a slightly better error
           message. */
        if (0 == strcasecmp("file not found", err_msg) &&
            (file_exists(ri->ri_path, "lo") ||
             file_exists(ri->ri_path, "so") ||
             file_exists(ri->ri_path, "dylib") ||
             file_exists(ri->ri_path, "dll"))) {
            err_msg = "perhaps a missing symbol, or compiled for a different version of Open MPI?";
        }
        gds_output_verbose(vl, 0, "gds_mca_base_component_repository_open: unable to open %s: %s (ignored)",
                            ri->ri_base, err_msg);
        return GDS_ERR_BAD_PARAM;
    }

    /* Successfully opened the component; now find the public struct.
       Malloc out enough space for it. */

    do {
        ret = asprintf (&struct_name, "mca_%s_%s_component", ri->ri_type, ri->ri_name);
        if (0 > ret) {
            ret = GDS_ERR_OUT_OF_RESOURCE;
            break;
        }

        mitem = GDS_NEW(gds_mca_base_component_list_item_t);
        if (NULL == mitem) {
            ret = GDS_ERR_OUT_OF_RESOURCE;
            break;
        }

        err_msg = NULL;
        ret = gds_gdl_lookup(ri->ri_dlhandle, struct_name, (void**) &component_struct, &err_msg);
        if (GDS_SUCCESS != ret || NULL == component_struct) {
            if (NULL == err_msg) {
                err_msg = "gds_dl_loookup() error message was NULL!";
            }
            gds_output_verbose(vl, 0, "gds_mca_base_component_repository_open: \"%s\" does not appear to be a valid "
                                "%s MCA dynamic component (ignored): %s. ret %d", ri->ri_base, ri->ri_type, err_msg, ret);

            ret = GDS_ERR_BAD_PARAM;
            break;
        }

        /* done with the structure name */
        free (struct_name);
        struct_name = NULL;

        /* We found the public struct.  Make sure its MCA major.minor
           version is the same as ours. TODO -- add checks for project version (from framework) */
        if (!(GDS_MCA_BASE_VERSION_MAJOR == component_struct->gds_mca_major_version &&
              GDS_MCA_BASE_VERSION_MINOR == component_struct->gds_mca_minor_version)) {
            gds_output_verbose(vl, 0, "gds_mca_base_component_repository_open: %s \"%s\" uses an MCA interface that is "
                                "not recognized (component MCA v%d.%d.%d != supported MCA v%d.%d.%d) -- ignored",
                                ri->ri_type, ri->ri_path, component_struct->gds_mca_major_version,
                                component_struct->gds_mca_minor_version, component_struct->gds_mca_release_version,
                                GDS_MCA_BASE_VERSION_MAJOR, GDS_MCA_BASE_VERSION_MINOR, GDS_MCA_BASE_VERSION_RELEASE);
            ret = GDS_ERR_BAD_PARAM;
            break;
        }

        /* Also check that the component struct framework and component
           names match the expected names from the filename */
        if (0 != strcmp(component_struct->gds_mca_type_name, ri->ri_type) ||
            0 != strcmp(component_struct->gds_mca_component_name, ri->ri_name)) {
            gds_output_verbose(vl, 0, "Component file data does not match filename: %s (%s / %s) != %s %s -- ignored",
                                ri->ri_path, ri->ri_type, ri->ri_name,
                                component_struct->gds_mca_type_name,
                                component_struct->gds_mca_component_name);
            ret = GDS_ERR_BAD_PARAM;
            break;
        }

        /* Alles gut.  Save the component struct, and register this
           component to be closed later. */

        ri->ri_component_struct = mitem->cli_component = component_struct;
        ri->ri_refcnt = 1;
        gds_list_append(&framework->framework_components, &mitem->super);

        gds_output_verbose (GDS_MCA_BASE_VERBOSE_INFO, 0, "gds_mca_base_component_repository_open: opened dynamic %s MCA "
                             "component \"%s\"", ri->ri_type, ri->ri_name);

        return GDS_SUCCESS;
    } while (0);

    if (mitem) {
        GDS_RELEASE(mitem);
    }

    if (struct_name) {
        free (struct_name);
    }

    gds_gdl_close (ri->ri_dlhandle);
    ri->ri_dlhandle = NULL;

    return ret;
#else

    /* no dlopen support */
    return GDS_ERR_NOT_SUPPORTED;
#endif
}

/*
 * Finalize the repository -- close everything that's still open.
 */
void gds_mca_base_component_repository_finalize(void)
{
    if (!initialized) {
        return;
    }

    initialized = false;

#if GDS_HAVE_GDL_SUPPORT
    gds_list_t *component_list;
    void *node, *key;
    size_t key_size;
    int ret;

    ret = gds_hash_table_get_first_key_ptr (&gds_mca_base_component_repository, &key, &key_size,
                                             (void **) &component_list, &node);
    while (GDS_SUCCESS == ret) {
        GDS_LIST_RELEASE(component_list);
        ret = gds_hash_table_get_next_key_ptr (&gds_mca_base_component_repository, &key,
                                                &key_size, (void **) &component_list,
                                                node, &node);
    }

    (void) gds_mca_base_framework_close(&gds_gdl_base_framework);
    GDS_DESTRUCT(&gds_mca_base_component_repository);
#endif
}

#if GDS_HAVE_GDL_SUPPORT

/*
 * Basic sentinel values, and construct the inner list
 */
static void ri_constructor (gds_mca_base_component_repository_item_t *ri)
{
    memset(ri->ri_type, 0, sizeof(ri->ri_type));
    ri->ri_dlhandle = NULL;
    ri->ri_component_struct = NULL;
    ri->ri_path = NULL;
}


/*
 * Close a component
 */
static void ri_destructor (gds_mca_base_component_repository_item_t *ri)
{
    /* dlclose the component if it is still open */
    gds_mca_base_component_repository_release_internal (ri);

    /* It should be obvious, but I'll state it anyway because it bit me
       during debugging: after the dlclose(), the gds_mca_base_component_t
       pointer is no longer valid because it has [potentially] been
       unloaded from memory.  So don't try to use it.  :-) */

    if (ri->ri_path) {
        free (ri->ri_path);
    }

    if (ri->ri_base) {
        free (ri->ri_base);
    }
}

#endif /* GDS_HAVE_GDL_SUPPORT */
