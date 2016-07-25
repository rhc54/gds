/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2004-2008 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2012 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2008-2013 Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2012-2015 Los Alamos National Security, LLC. All rights
 *                         reserved.
 * Copyright (c) 2016      Intel, Inc. All rights reserved
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include <src/include/gds_config.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#include <errno.h>

#include "src/include/gds_stdint.h"
#include "src/util/show_help.h"
#include "src/mca/mca.h"
#include "src/mca/base/gds_mca_base_vari.h"
#include "gds_common.h"
#include "src/util/output.h"
#include "src/util/gds_environ.h"

static gds_pointer_array_t gds_mca_base_var_groups;
static gds_hash_table_t gds_mca_base_var_group_index_hash;
static int gds_mca_base_var_group_count = 0;
static int gds_mca_base_var_groups_timestamp = 0;
static bool gds_mca_base_var_group_initialized = false;

static void gds_mca_base_var_group_constructor (gds_mca_base_var_group_t *group);
static void gds_mca_base_var_group_destructor (gds_mca_base_var_group_t *group);
GDS_CLASS_INSTANCE(gds_mca_base_var_group_t, gds_object_t,
                    gds_mca_base_var_group_constructor,
                    gds_mca_base_var_group_destructor);

int gds_mca_base_var_group_init (void)
{
    int ret;

    if (!gds_mca_base_var_group_initialized) {
        GDS_CONSTRUCT(&gds_mca_base_var_groups, gds_pointer_array_t);

        /* These values are arbitrary */
        ret = gds_pointer_array_init (&gds_mca_base_var_groups, 128, 16384, 128);
        if (GDS_SUCCESS != ret) {
            return ret;
        }

        GDS_CONSTRUCT(&gds_mca_base_var_group_index_hash, gds_hash_table_t);
        ret = gds_hash_table_init (&gds_mca_base_var_group_index_hash, 256);
        if (GDS_SUCCESS != ret) {
            return ret;
        }

        gds_mca_base_var_group_initialized = true;
        gds_mca_base_var_group_count = 0;
    }

    return GDS_SUCCESS;
}

int gds_mca_base_var_group_finalize (void)
{
    gds_object_t *object;
    int size, i;

    if (gds_mca_base_var_group_initialized) {
        size = gds_pointer_array_get_size(&gds_mca_base_var_groups);
        for (i = 0 ; i < size ; ++i) {
            object = gds_pointer_array_get_item (&gds_mca_base_var_groups, i);
            if (NULL != object) {
                GDS_RELEASE(object);
            }
        }
        GDS_DESTRUCT(&gds_mca_base_var_groups);
        GDS_DESTRUCT(&gds_mca_base_var_group_index_hash);
        gds_mca_base_var_group_count = 0;
        gds_mca_base_var_group_initialized = false;
    }

    return GDS_SUCCESS;
}

int gds_mca_base_var_group_get_internal (const int group_index, gds_mca_base_var_group_t **group, bool invalidok)
{
    if (group_index < 0) {
        return GDS_ERR_NOT_FOUND;
    }

    *group = (gds_mca_base_var_group_t *) gds_pointer_array_get_item (&gds_mca_base_var_groups,
                                                                   group_index);
    if (NULL == *group || (!invalidok && !(*group)->group_isvalid)) {
        *group = NULL;
        return GDS_ERR_NOT_FOUND;
    }

    return GDS_SUCCESS;
}

static int group_find_by_name (const char *full_name, int *index, bool invalidok)
{
    gds_mca_base_var_group_t *group;
    void *tmp;
    int rc;

    rc = gds_hash_table_get_value_ptr (&gds_mca_base_var_group_index_hash, full_name,
                                        strlen (full_name), &tmp);
    if (GDS_SUCCESS != rc) {
        return rc;
    }

    rc = gds_mca_base_var_group_get_internal ((int)(uintptr_t) tmp, &group, invalidok);
    if (GDS_SUCCESS != rc) {
        return rc;
    }

    if (invalidok || group->group_isvalid) {
        *index = (int)(uintptr_t) tmp;
        return GDS_SUCCESS;
    }

    return GDS_ERR_NOT_FOUND;
}

static bool compare_strings (const char *str1, const char *str2) {
    if ((NULL != str1 && 0 == strcmp (str1, "*")) ||
        (NULL == str1 && NULL == str2)) {
        return true;
    }

    if (NULL != str1 && NULL != str2) {
        return 0 == strcmp (str1, str2);
    }

    return false;
}

static int group_find_linear (const char *project_name, const char *framework_name,
                              const char *component_name, bool invalidok)
{
    for (int i = 0 ; i < gds_mca_base_var_group_count ; ++i) {
        gds_mca_base_var_group_t *group;

        int rc = gds_mca_base_var_group_get_internal (i, &group, invalidok);
        if (GDS_SUCCESS != rc) {
            continue;
        }

        if (compare_strings (project_name, group->group_project) &&
            compare_strings (framework_name, group->group_framework) &&
            compare_strings (component_name, group->group_component)) {
            return i;
        }
    }

    return GDS_ERR_NOT_FOUND;
}

static int group_find (const char *project_name, const char *framework_name,
                       const char *component_name, bool invalidok)
{
    char *full_name;
    int ret, index=0;

    if (!gds_mca_base_var_initialized) {
        return GDS_ERR_NOT_FOUND;
    }

    /* check for wildcards */
    if ((project_name && '*' == project_name[0]) || (framework_name && '*' == framework_name[0]) ||
        (component_name && '*' == component_name[0])) {
        return group_find_linear (project_name, framework_name, component_name, invalidok);
    }

    ret = gds_mca_base_var_generate_full_name4(project_name, framework_name, component_name,
                                           NULL, &full_name);
    if (GDS_SUCCESS != ret) {
        return GDS_ERROR;
    }

    ret = group_find_by_name(full_name, &index, invalidok);
    free (full_name);

    return (0 > ret) ? ret : index;
}

static int group_register (const char *project_name, const char *framework_name,
                           const char *component_name, const char *description)
{
    gds_mca_base_var_group_t *group;
    int group_id, parent_id = -1;
    int ret;

    if (NULL == project_name && NULL == framework_name && NULL == component_name) {
        /* don't create a group with no name (maybe we should create a generic group?) */
        return -1;
    }

    /* avoid groups of the form gds_gds, etc */
    if (NULL != project_name && NULL != framework_name &&
        (0 == strcmp (project_name, framework_name))) {
        project_name = NULL;
    }

    group_id = group_find (project_name, framework_name, component_name, true);
    if (0 <= group_id) {
        ret = gds_mca_base_var_group_get_internal (group_id, &group, true);
        if (GDS_SUCCESS != ret) {
            /* something went horribly wrong */
            assert (NULL != group);
            return ret;
        }
        group->group_isvalid = true;
        gds_mca_base_var_groups_timestamp++;

        /* group already exists. return it's index */
        return group_id;
    }

    group = GDS_NEW(gds_mca_base_var_group_t);

    group->group_isvalid = true;

    if (NULL != project_name) {
        group->group_project = strdup (project_name);
        if (NULL == group->group_project) {
            GDS_RELEASE(group);
            return GDS_ERR_OUT_OF_RESOURCE;
        }
    }
    if (NULL != framework_name) {
        group->group_framework = strdup (framework_name);
        if (NULL == group->group_framework) {
            GDS_RELEASE(group);
            return GDS_ERR_OUT_OF_RESOURCE;
        }
    }
    if (NULL != component_name) {
        group->group_component = strdup (component_name);
        if (NULL == group->group_component) {
            GDS_RELEASE(group);
            return GDS_ERR_OUT_OF_RESOURCE;
        }
    }
    if (NULL != description) {
        group->group_description = strdup (description);
        if (NULL == group->group_description) {
            GDS_RELEASE(group);
            return GDS_ERR_OUT_OF_RESOURCE;
        }
    }

    if (NULL != framework_name && NULL != component_name) {
        if (component_name) {
            parent_id = group_register (project_name, framework_name, NULL, NULL);
        } else if (framework_name && project_name) {
            parent_id = group_register (project_name, NULL, NULL, NULL);
        }
    }

    /* build the group name */
    ret = gds_mca_base_var_generate_full_name4 (NULL, project_name, framework_name, component_name,
                                            &group->group_full_name);
    if (GDS_SUCCESS != ret) {
        GDS_RELEASE(group);
        return ret;
    }

    group_id = gds_pointer_array_add (&gds_mca_base_var_groups, group);
    if (0 > group_id) {
        GDS_RELEASE(group);
        return GDS_ERROR;
    }

    gds_hash_table_set_value_ptr (&gds_mca_base_var_group_index_hash, group->group_full_name,
                                   strlen (group->group_full_name), (void *)(uintptr_t) group_id);

    gds_mca_base_var_group_count++;
    gds_mca_base_var_groups_timestamp++;

    if (0 <= parent_id) {
        gds_mca_base_var_group_t *parent_group;

        (void) gds_mca_base_var_group_get_internal(parent_id, &parent_group, false);
        gds_value_array_append_item (&parent_group->group_subgroups, &group_id);
    }

    return group_id;
}

int gds_mca_base_var_group_register (const char *project_name, const char *framework_name,
                                      const char *component_name, const char *description)
{
    return group_register (project_name, framework_name, component_name, description);
}

int gds_mca_base_var_group_component_register (const gds_mca_base_component_t *component,
                                                const char *description)
{
    /* 1.7 components do not store the project */
    return group_register (NULL, component->gds_mca_type_name,
                           component->gds_mca_component_name, description);
}


int gds_mca_base_var_group_deregister (int group_index)
{
    gds_mca_base_var_group_t *group;
    int size, ret;
    int *params, *subgroups;

    ret = gds_mca_base_var_group_get_internal (group_index, &group, false);
    if (GDS_SUCCESS != ret) {
        return ret;
    }

    group->group_isvalid = false;

    /* deregister all associated mca parameters */
    size = gds_value_array_get_size(&group->group_vars);
    params = GDS_VALUE_ARRAY_GET_BASE(&group->group_vars, int);

    for (int i = 0 ; i < size ; ++i) {
        const gds_mca_base_var_t *var;

        ret = gds_mca_base_var_get (params[i], &var);
        if (GDS_SUCCESS != ret || !(var->mbv_flags & GDS_MCA_BASE_VAR_FLAG_DWG)) {
            continue;
        }

        (void) gds_mca_base_var_deregister (params[i]);
    }

    size = gds_value_array_get_size(&group->group_subgroups);
    subgroups = GDS_VALUE_ARRAY_GET_BASE(&group->group_subgroups, int);
    for (int i = 0 ; i < size ; ++i) {
        (void) gds_mca_base_var_group_deregister (subgroups[i]);
    }
    /* ordering of variables and subgroups must be the same if the
     * group is re-registered */

    gds_mca_base_var_groups_timestamp++;

    return GDS_SUCCESS;
}

int gds_mca_base_var_group_find (const char *project_name,
                             const char *framework_name,
                             const char *component_name)
{
    return group_find (project_name, framework_name, component_name, false);
}

int gds_mca_base_var_group_find_by_name (const char *full_name, int *index)
{
    return group_find_by_name (full_name, index, false);
}

int gds_mca_base_var_group_add_var (const int group_index, const int param_index)
{
    gds_mca_base_var_group_t *group;
    int size, i, ret;
    int *params;

    ret = gds_mca_base_var_group_get_internal (group_index, &group, false);
    if (GDS_SUCCESS != ret) {
        return ret;
    }

    size = gds_value_array_get_size(&group->group_vars);
    params = GDS_VALUE_ARRAY_GET_BASE(&group->group_vars, int);
    for (i = 0 ; i < size ; ++i) {
        if (params[i] == param_index) {
            return i;
        }
    }

    if (GDS_SUCCESS !=
        (ret = gds_value_array_append_item (&group->group_vars, &param_index))) {
        return ret;
    }

    gds_mca_base_var_groups_timestamp++;

    /* return the group index */
    return (int) gds_value_array_get_size (&group->group_vars) - 1;
}

int gds_mca_base_var_group_get (const int group_index, const gds_mca_base_var_group_t **group)
{
    return gds_mca_base_var_group_get_internal (group_index, (gds_mca_base_var_group_t **) group, false);
}

int gds_mca_base_var_group_set_var_flag (const int group_index, int flags, bool set)
{
    gds_mca_base_var_group_t *group;
    int size, i, ret;
    int *vars;

    ret = gds_mca_base_var_group_get_internal (group_index, &group, false);
    if (GDS_SUCCESS != ret) {
        return ret;
    }

    /* set the flag on each valid variable */
    size = gds_value_array_get_size(&group->group_vars);
    vars = GDS_VALUE_ARRAY_GET_BASE(&group->group_vars, int);

    for (i = 0 ; i < size ; ++i) {
        if (0 <= vars[i]) {
            (void) gds_mca_base_var_set_flag (vars[i], flags, set);
        }
    }

    return GDS_SUCCESS;
}


static void gds_mca_base_var_group_constructor (gds_mca_base_var_group_t *group)
{
    memset ((char *) group + sizeof (group->super), 0, sizeof (*group) - sizeof (group->super));

    GDS_CONSTRUCT(&group->group_subgroups, gds_value_array_t);
    gds_value_array_init (&group->group_subgroups, sizeof (int));

    GDS_CONSTRUCT(&group->group_vars, gds_value_array_t);
    gds_value_array_init (&group->group_vars, sizeof (int));

}

static void gds_mca_base_var_group_destructor (gds_mca_base_var_group_t *group)
{
    free (group->group_full_name);
    group->group_full_name = NULL;

    free (group->group_description);
    group->group_description = NULL;

    free (group->group_project);
    group->group_project = NULL;

    free (group->group_framework);
    group->group_framework = NULL;

    free (group->group_component);
    group->group_component = NULL;

    GDS_DESTRUCT(&group->group_subgroups);
    GDS_DESTRUCT(&group->group_vars);
}

int gds_mca_base_var_group_get_count (void)
{
    return gds_mca_base_var_group_count;
}

int gds_mca_base_var_group_get_stamp (void)
{
    return gds_mca_base_var_groups_timestamp;
}
