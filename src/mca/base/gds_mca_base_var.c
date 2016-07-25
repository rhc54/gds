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
 * Copyright (c) 2008-2015 Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2012-2015 Los Alamos National Security, LLC. All rights
 *                         reserved.
 * Copyright (c) 2014-2016 Intel, Inc. All rights reserved.
 * Copyright (c) 2015      Research Organization for Information Science
 *                         and Technology (RIST). All rights reserved.
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
#include "src/mca/pinstalldirs/pinstalldirs.h"
#include "src/util/os_path.h"
#include "src/util/path.h"
#include "src/util/show_help.h"
#include "src/util/printf.h"
#include "src/util/argv.h"
#include "src/util/error.h"
#include "src/mca/mca.h"
#include "src/mca/base/gds_mca_base_vari.h"
#include "gds_common.h"
#include "src/util/output.h"
#include "src/util/gds_environ.h"

/*
 * local variables
 */
static gds_pointer_array_t gds_mca_base_vars;
static const char *mca_prefix = "GDS_MCA_";
static char *home = NULL;
static char *cwd  = NULL;
bool gds_mca_base_var_initialized = false;
static char * force_agg_path = NULL;
static char *gds_mca_base_var_files = NULL;
static char *gds_mca_base_envar_files = NULL;
static char **gds_mca_base_var_file_list = NULL;
static char *gds_mca_base_var_override_file = NULL;
static char *gds_mca_base_var_file_prefix = NULL;
static char *gds_mca_base_envar_file_prefix = NULL;
static char *gds_mca_base_param_file_path = NULL;
static char *gds_mca_base_env_list = NULL;
#define GDS_MCA_BASE_ENV_LIST_SEP_DEFAULT ";"
static char *gds_mca_base_env_list_sep = GDS_MCA_BASE_ENV_LIST_SEP_DEFAULT;
static char *gds_mca_base_env_list_internal = NULL;
static bool gds_mca_base_var_suppress_override_warning = false;
static gds_list_t gds_mca_base_var_file_values;
static gds_list_t gds_mca_base_envar_file_values;
static gds_list_t gds_mca_base_var_override_values;

static int gds_mca_base_var_count = 0;

static gds_hash_table_t gds_mca_base_var_index_hash;

const char *var_type_names[] = {
    "int",
    "unsigned_int",
    "unsigned_long",
    "unsigned_long_long",
    "size_t",
    "string",
    "version_string",
    "bool",
    "double"
};

const size_t var_type_sizes[] = {
    sizeof (int),
    sizeof (unsigned),
    sizeof (unsigned long),
    sizeof (unsigned long long),
    sizeof (size_t),
    sizeof (char),
    sizeof (char),
    sizeof (bool),
    sizeof (double)
};

const char *var_source_names[] = {
    "default",
    "command line",
    "environment",
    "file",
    "set",
    "override"
};


static const char *info_lvl_strings[] = {
    "user/basic",
    "user/detail",
    "user/all",
    "tuner/basic",
    "tuner/detail",
    "tuner/all",
    "dev/basic",
    "dev/detail",
    "dev/all"
};

/*
 * local functions
 */
static int fixup_files(char **file_list, char * path, bool rel_path_search, char sep);
static int read_files (char *file_list, gds_list_t *file_values, char sep);
static int var_set_initial (gds_mca_base_var_t *var, gds_mca_base_var_t *original);
static int var_get (int vari, gds_mca_base_var_t **var_out, bool original);
static int var_value_string (gds_mca_base_var_t *var, char **value_string);

/*
 * classes
 */
static void var_constructor (gds_mca_base_var_t *p);
static void var_destructor (gds_mca_base_var_t *p);
GDS_CLASS_INSTANCE(gds_mca_base_var_t, gds_object_t,
                   var_constructor, var_destructor);

static void fv_constructor (gds_mca_base_var_file_value_t *p);
static void fv_destructor (gds_mca_base_var_file_value_t *p);
GDS_CLASS_INSTANCE(gds_mca_base_var_file_value_t, gds_list_item_t,
                   fv_constructor, fv_destructor);

static const char *gds_mca_base_var_source_file (const gds_mca_base_var_t *var)
{
    gds_mca_base_var_file_value_t *fv = (gds_mca_base_var_file_value_t *) var->mbv_file_value;

    if (NULL != var->mbv_source_file) {
        return var->mbv_source_file;
    }

    if (fv) {
        return fv->mbvfv_file;
    }

    return NULL;
}

/*
 * Generate a full name from three names
 */
int gds_mca_base_var_generate_full_name4 (const char *project, const char *framework, const char *component,
                                      const char *variable, char **full_name)
{
    const char * const names[] = {project, framework, component, variable};
    char *name, *tmp;
    size_t i, len;

    *full_name = NULL;

    for (i = 0, len = 0 ; i < 4 ; ++i) {
        if (NULL != names[i]) {
            /* Add space for the string + _ or \0 */
            len += strlen (names[i]) + 1;
        }
    }

    name = calloc (1, len);
    if (NULL == name) {
        return GDS_ERR_OUT_OF_RESOURCE;
    }

    for (i = 0, tmp = name ; i < 4 ; ++i) {
        if (NULL != names[i]) {
            if (name != tmp) {
                *tmp++ = '_';
            }
            strncat (name, names[i], len - (size_t)(uintptr_t)(tmp - name));
            tmp += strlen (names[i]);
        }
    }

    *full_name = name;
    return GDS_SUCCESS;
}

static int compare_strings (const char *str1, const char *str2) {
    if ((NULL != str1 && 0 == strcmp (str1, "*")) ||
        (NULL == str1 && NULL == str2)) {
        return 0;
    }

    if (NULL != str1 && NULL != str2) {
        return strcmp (str1, str2);
    }

    return 1;
}

/*
 * Append a filename to the file list if it does not exist and return a
 * pointer to the filename in the list.
 */
static char *append_filename_to_list(const char *filename)
{
    int i, count;

    (void) gds_argv_append_unique_nosize(&gds_mca_base_var_file_list, filename, false);

    count = gds_argv_count(gds_mca_base_var_file_list);

    for (i = count - 1; i >= 0; --i) {
        if (0 == strcmp (gds_mca_base_var_file_list[i], filename)) {
            return gds_mca_base_var_file_list[i];
        }
    }

    /* *#@*? */
    return NULL;
}

/*
 * Set it up
 */
int gds_mca_base_var_init(void)
{
    int ret;
    char *name = NULL;

    if (!gds_mca_base_var_initialized) {
        /* Init the value array for the param storage */

        GDS_CONSTRUCT(&gds_mca_base_vars, gds_pointer_array_t);
        /* These values are arbitrary */
        ret = gds_pointer_array_init (&gds_mca_base_vars, 128, 16384, 128);
        if (GDS_SUCCESS != ret) {
            return ret;
        }

        gds_mca_base_var_count = 0;

        /* Init the file param value list */

        GDS_CONSTRUCT(&gds_mca_base_var_file_values, gds_list_t);
        GDS_CONSTRUCT(&gds_mca_base_envar_file_values, gds_list_t);
        GDS_CONSTRUCT(&gds_mca_base_var_override_values, gds_list_t);
        GDS_CONSTRUCT(&gds_mca_base_var_index_hash, gds_hash_table_t);

        ret = gds_hash_table_init (&gds_mca_base_var_index_hash, 1024);
        if (GDS_SUCCESS != ret) {
            return ret;
        }

        ret = gds_mca_base_var_group_init ();
        if  (GDS_SUCCESS != ret) {
            return ret;
        }

        /* Set this before we register the parameter, below */

        gds_mca_base_var_initialized = true;

        gds_mca_base_var_cache_files(false);

        /* register the envar-forwarding params */
        (void)gds_mca_base_var_register ("gds", "mca", "base", "env_list",
                                     "Set SHELL env variables",
                                     GDS_MCA_BASE_VAR_TYPE_STRING, NULL, 0, 0, GDS_INFO_LVL_3,
                                     GDS_MCA_BASE_VAR_SCOPE_READONLY, &gds_mca_base_env_list);

        gds_mca_base_env_list_sep = GDS_MCA_BASE_ENV_LIST_SEP_DEFAULT;
        (void)gds_mca_base_var_register ("gds", "mca", "base", "env_list_delimiter",
                                     "Set SHELL env variables delimiter. Default: semicolon ';'",
                                     GDS_MCA_BASE_VAR_TYPE_STRING, NULL, 0, 0, GDS_INFO_LVL_3,
                                     GDS_MCA_BASE_VAR_SCOPE_READONLY, &gds_mca_base_env_list_sep);

        /* Set OMPI_MCA_gds_mca_base_env_list variable, it might not be set before
         * if mca variable was taken from amca conf file. Need to set it
         * here because gds_mca_base_var_process_env_list is called from schizo_ompi.c
         * only when this env variable was set.
         */
        if (NULL != gds_mca_base_env_list) {
            (void) gds_mca_base_var_env_name ("gds_mca_base_env_list", &name);
            if (NULL != name) {
                gds_setenv(name, gds_mca_base_env_list, false, &environ);
                free(name);
            }
        }

        /* Register internal MCA variable gds_mca_base_env_list_internal. It can be set only during
         * parsing of amca conf file and contains SHELL env variables specified via -x there.
         * Its format is the same as for gds_mca_base_env_list.
         */
        (void)gds_mca_base_var_register ("gds", "mca", "base", "env_list_internal",
                "Store SHELL env variables from amca conf file",
                GDS_MCA_BASE_VAR_TYPE_STRING, NULL, 0, GDS_MCA_BASE_VAR_FLAG_INTERNAL, GDS_INFO_LVL_3,
                GDS_MCA_BASE_VAR_SCOPE_READONLY, &gds_mca_base_env_list_internal);
    }

    return GDS_SUCCESS;
}

static void process_env_list(char *env_list, char ***argv, char sep)
{
    char** tokens;
    char *ptr, *value;

    tokens = gds_argv_split(env_list, (int)sep);
    if (NULL == tokens) {
        return;
    }

    for (int i = 0 ; NULL != tokens[i] ; ++i) {
        if (NULL == (ptr = strchr(tokens[i], '='))) {
            value = getenv(tokens[i]);
            if (NULL == value) {
                gds_show_help("help-mca-var.txt", "incorrect-env-list-param",
                               true, tokens[i], env_list);
                break;
            }

            /* duplicate the value to silence tainted string coverity issue */
            value = strdup (value);
            if (NULL == value) {
                /* out of memory */
                break;
            }

            if (NULL != (ptr = strchr(value, '='))) {
                *ptr = '\0';
                gds_setenv(value, ptr + 1, true, argv);
            } else {
                gds_setenv(tokens[i], value, true, argv);
            }

            free (value);
        } else {
            *ptr = '\0';
            gds_setenv(tokens[i], ptr + 1, true, argv);
            /* NTH: don't bother resetting ptr to = since the string will not be used again */
        }
    }

    gds_argv_free(tokens);
}

int gds_mca_base_var_process_env_list(char ***argv)
{
    char sep;
    sep = ';';
    if (NULL != gds_mca_base_env_list_sep) {
        if (1 == strlen(gds_mca_base_env_list_sep)) {
            sep = gds_mca_base_env_list_sep[0];
        } else {
            gds_show_help("help-mca-var.txt", "incorrect-env-list-sep",
                    true, gds_mca_base_env_list_sep);
            return GDS_SUCCESS;
        }
    }
    if (NULL != gds_mca_base_env_list) {
        process_env_list(gds_mca_base_env_list, argv, sep);
    }

    return GDS_SUCCESS;
}

int gds_mca_base_var_process_env_list_from_file(char ***argv)
{
    if (NULL != gds_mca_base_env_list_internal) {
        process_env_list(gds_mca_base_env_list_internal, argv, ';');
    }
    return GDS_SUCCESS;
}

static void resolve_relative_paths(char **file_prefix, char *file_path, bool rel_path_search, char **files, char sep)
{
    char *tmp_str;
    /*
     * Resolve all relative paths.
     * the file list returned will contain only absolute paths
     */
    if( GDS_SUCCESS != fixup_files(file_prefix, file_path, rel_path_search, sep) ) {
#if 0
        /* JJH We need to die! */
        abort();
#else
        ;
#endif
    }
    else {
        /* Prepend the files to the search list */
        asprintf(&tmp_str, "%s%c%s", *file_prefix, sep, *files);
        free (*files);
        *files = tmp_str;
    }
}

int gds_mca_base_var_cache_files(bool rel_path_search)
{
    char *tmp;
    int ret;

    /* We may need this later */
    home = (char*)gds_home_directory();

    if(NULL == cwd) {
        cwd = (char *) malloc(sizeof(char) * MAXPATHLEN);
        if( NULL == (cwd = getcwd(cwd, MAXPATHLEN) )) {
            gds_output(0, "Error: Unable to get the current working directory\n");
            cwd = strdup(".");
        }
    }

#if GDS_WANT_HOME_CONFIG_FILES
    asprintf(&gds_mca_base_var_files, "%s"GDS_PATH_SEP".gds" GDS_PATH_SEP
             "mca-params.conf%c%s" GDS_PATH_SEP "gds-mca-params.conf",
             home, GDS_ENV_SEP, gds_pinstall_dirs.sysconfdir);
#else
    asprintf(&gds_mca_base_var_files, "%s" GDS_PATH_SEP "gds-mca-params.conf",
             gds_pinstall_dirs.sysconfdir);
#endif

    /* Initialize a parameter that says where MCA param files can be found.
       We may change this value so set the scope to GDS_MCA_BASE_VAR_SCOPE_READONLY */
    tmp = gds_mca_base_var_files;
    ret = gds_mca_base_var_register ("gds", "mca", "base", "param_files", "Path for MCA "
                                 "configuration files containing variable values",
                                 GDS_MCA_BASE_VAR_TYPE_STRING, NULL, 0, 0, GDS_INFO_LVL_2,
                                 GDS_MCA_BASE_VAR_SCOPE_READONLY, &gds_mca_base_var_files);
    free (tmp);
    if (GDS_SUCCESS != ret) {
        return ret;
    }

    gds_mca_base_envar_files = strdup(gds_mca_base_var_files);

    (void) gds_mca_base_var_register_synonym (ret, "gds", "mca", NULL, "param_files",
                                          GDS_MCA_BASE_VAR_SYN_FLAG_DEPRECATED);

    ret = asprintf(&gds_mca_base_var_override_file, "%s" GDS_PATH_SEP "gds-mca-params-override.conf",
                   gds_pinstall_dirs.sysconfdir);
    if (0 > ret) {
        return GDS_ERR_OUT_OF_RESOURCE;
    }

    tmp = gds_mca_base_var_override_file;
    ret = gds_mca_base_var_register ("gds", "mca", "base", "override_param_file",
                                 "Variables set in this file will override any value set in"
                                 "the environment or another configuration file",
                                 GDS_MCA_BASE_VAR_TYPE_STRING, NULL, 0, GDS_MCA_BASE_VAR_FLAG_DEFAULT_ONLY,
                                 GDS_INFO_LVL_2, GDS_MCA_BASE_VAR_SCOPE_CONSTANT,
                                 &gds_mca_base_var_override_file);
    free (tmp);
    if (0 > ret) {
        return ret;
    }

    /* Disable reading MCA parameter files. */
    if (0 == strcmp (gds_mca_base_var_files, "none")) {
        return GDS_SUCCESS;
    }

    gds_mca_base_var_suppress_override_warning = false;
    ret = gds_mca_base_var_register ("gds", "mca", "base", "suppress_override_warning",
                                 "Suppress warnings when attempting to set an overridden value (default: false)",
                                 GDS_MCA_BASE_VAR_TYPE_BOOL, NULL, 0, 0, GDS_INFO_LVL_2,
                                 GDS_MCA_BASE_VAR_SCOPE_LOCAL, &gds_mca_base_var_suppress_override_warning);
    if (0 > ret) {
        return ret;
    }

    /* Aggregate MCA parameter files
     * A prefix search path to look up aggregate MCA parameter file
     * requests that do not specify an absolute path
     */
    gds_mca_base_var_file_prefix = NULL;
    ret = gds_mca_base_var_register ("gds", "mca", "base", "param_file_prefix",
                                 "Aggregate MCA parameter file sets",
                                 GDS_MCA_BASE_VAR_TYPE_STRING, NULL, 0, 0, GDS_INFO_LVL_3,
                                 GDS_MCA_BASE_VAR_SCOPE_READONLY, &gds_mca_base_var_file_prefix);
    if (0 > ret) {
        return ret;
    }

    gds_mca_base_envar_file_prefix = NULL;
    ret = gds_mca_base_var_register ("gds", "mca", "base", "envar_file_prefix",
                                 "Aggregate MCA parameter file set for env variables",
                                 GDS_MCA_BASE_VAR_TYPE_STRING, NULL, 0, 0, GDS_INFO_LVL_3,
                                 GDS_MCA_BASE_VAR_SCOPE_READONLY, &gds_mca_base_envar_file_prefix);
    if (0 > ret) {
        return ret;
    }

    ret = asprintf(&gds_mca_base_param_file_path, "%s" GDS_PATH_SEP "amca-param-sets%c%s",
                   gds_pinstall_dirs.gdsdatadir, GDS_ENV_SEP, cwd);
    if (0 > ret) {
        return GDS_ERR_OUT_OF_RESOURCE;
    }

    tmp = gds_mca_base_param_file_path;
    ret = gds_mca_base_var_register ("gds", "mca", "base", "param_file_path",
                                 "Aggregate MCA parameter Search path",
                                 GDS_MCA_BASE_VAR_TYPE_STRING, NULL, 0, 0, GDS_INFO_LVL_3,
                                 GDS_MCA_BASE_VAR_SCOPE_READONLY, &gds_mca_base_param_file_path);
    free (tmp);
    if (0 > ret) {
        return ret;
    }

    force_agg_path = NULL;
    ret = gds_mca_base_var_register ("gds", "mca", "base", "param_file_path_force",
                                 "Forced Aggregate MCA parameter Search path",
                                 GDS_MCA_BASE_VAR_TYPE_STRING, NULL, 0, 0, GDS_INFO_LVL_3,
                                 GDS_MCA_BASE_VAR_SCOPE_READONLY, &force_agg_path);
    if (0 > ret) {
        return ret;
    }

    if (NULL != force_agg_path) {
        if (NULL != gds_mca_base_param_file_path) {
            char *tmp_str = gds_mca_base_param_file_path;

            asprintf(&gds_mca_base_param_file_path, "%s%c%s", force_agg_path, GDS_ENV_SEP, tmp_str);
            free(tmp_str);
        } else {
            gds_mca_base_param_file_path = strdup(force_agg_path);
        }
    }

    if (NULL != gds_mca_base_var_file_prefix) {
       resolve_relative_paths(&gds_mca_base_var_file_prefix, gds_mca_base_param_file_path, rel_path_search, &gds_mca_base_var_files, GDS_ENV_SEP);
    }
    read_files (gds_mca_base_var_files, &gds_mca_base_var_file_values, GDS_ENV_SEP);

    if (NULL != gds_mca_base_envar_file_prefix) {
       resolve_relative_paths(&gds_mca_base_envar_file_prefix, gds_mca_base_param_file_path, rel_path_search, &gds_mca_base_envar_files, ',');
    }
    read_files (gds_mca_base_envar_files, &gds_mca_base_envar_file_values, ',');

    if (0 == access(gds_mca_base_var_override_file, F_OK)) {
        read_files (gds_mca_base_var_override_file, &gds_mca_base_var_override_values, GDS_ENV_SEP);
    }

    return GDS_SUCCESS;
}

/*
 * Look up an integer MCA parameter.
 */
int gds_mca_base_var_get_value (int vari, const void *value,
                            gds_mca_base_var_source_t *source,
                            const char **source_file)
{
    gds_mca_base_var_t *var;
    void **tmp = (void **) value;
    int ret;

    ret = var_get (vari, &var, true);
    if (GDS_SUCCESS != ret) {
        return ret;
    }

    if (!GDS_VAR_IS_VALID(var[0])) {
        return GDS_ERR_NOT_FOUND;
    }

    if (NULL != value) {
        /* Return a poiner to our backing store (either a char **, int *,
           or bool *) */
        *tmp = var->mbv_storage;
    }

    if (NULL != source) {
        *source = var->mbv_source;
    }

    if (NULL != source_file) {
        *source_file = gds_mca_base_var_source_file (var);
    }

    return GDS_SUCCESS;
}

static int var_set_string (gds_mca_base_var_t *var, char *value)
{
    char *tmp;
    int ret;

    if (NULL != var->mbv_storage->stringval) {
        free (var->mbv_storage->stringval);
    }

    var->mbv_storage->stringval = NULL;

    if (NULL == value || 0 == strlen (value)) {
        return GDS_SUCCESS;
    }

    /* Replace all instances of ~/ in a path-style string with the
       user's home directory. This may be handled by the enumerator
       in the future. */
    if (0 == strncmp (value, "~/", 2)) {
        if (NULL != home) {
            ret = asprintf (&value, "%s/%s", home, value + 2);
            if (0 > ret) {
                return GDS_ERROR;
            }
        } else {
            value = strdup (value + 2);
        }
    } else {
        value = strdup (value);
    }

    if (NULL == value) {
        return GDS_ERR_OUT_OF_RESOURCE;
    }

    while (NULL != (tmp = strstr (value, ":~/"))) {
        tmp[0] = '\0';
        tmp += 3;

        ret = asprintf (&tmp, "%s:%s%s%s", value,
                        home ? home : "", home ? "/" : "", tmp);

        free (value);

        if (0 > ret) {
            return GDS_ERR_OUT_OF_RESOURCE;
        }

        value = tmp;
    }

    var->mbv_storage->stringval = value;

    return GDS_SUCCESS;
}

static int int_from_string(const char *src, gds_mca_base_var_enum_t *enumerator, uint64_t *value_out)
{
    uint64_t value;
    bool is_int;
    char *tmp;

    if (NULL == src || 0 == strlen (src)) {
        if (NULL == enumerator) {
            *value_out = 0;
        }

        return GDS_SUCCESS;
    }

    if (enumerator) {
        int int_val, ret;
        ret = enumerator->value_from_string(enumerator, src, &int_val);
        if (GDS_SUCCESS != ret) {
            return ret;
        }
        *value_out = (uint64_t) int_val;

        return GDS_SUCCESS;
    }

    /* Check for an integer value */
    value = strtoull (src, &tmp, 0);
    is_int = tmp[0] == '\0';

    if (!is_int && tmp != src) {
        switch (tmp[0]) {
        case 'G':
        case 'g':
            value <<= 10;
        case 'M':
        case 'm':
            value <<= 10;
        case 'K':
        case 'k':
            value <<= 10;
            break;
        default:
            break;
        }
    }

    *value_out = value;

    return GDS_SUCCESS;
}

static int var_set_from_string (gds_mca_base_var_t *var, char *src)
{
    gds_mca_base_var_storage_t *dst = var->mbv_storage;
    uint64_t int_value = 0;
    int ret;

    switch (var->mbv_type) {
    case GDS_MCA_BASE_VAR_TYPE_INT:
    case GDS_MCA_BASE_VAR_TYPE_UNSIGNED_INT:
    case GDS_MCA_BASE_VAR_TYPE_UNSIGNED_LONG:
    case GDS_MCA_BASE_VAR_TYPE_UNSIGNED_LONG_LONG:
    case GDS_MCA_BASE_VAR_TYPE_BOOL:
    case GDS_MCA_BASE_VAR_TYPE_SIZE_T:
        ret = int_from_string(src, var->mbv_enumerator, &int_value);
        if (GDS_ERR_VALUE_OUT_OF_BOUNDS == ret ||
            (GDS_MCA_BASE_VAR_TYPE_INT == var->mbv_type && ((int) int_value != (int64_t) int_value)) ||
            (GDS_MCA_BASE_VAR_TYPE_UNSIGNED_INT == var->mbv_type && ((unsigned int) int_value != int_value))) {
            if (var->mbv_enumerator) {
                char *valid_values;
                (void) var->mbv_enumerator->dump(var->mbv_enumerator, &valid_values);
                gds_show_help("help-mca-var.txt", "invalid-value-enum",
                               true, var->mbv_full_name, src, valid_values);
                free(valid_values);
            } else {
                gds_show_help("help-mca-var.txt", "invalid-value",
                               true, var->mbv_full_name, src);
            }

            return GDS_ERR_VALUE_OUT_OF_BOUNDS;
        }

        if (GDS_MCA_BASE_VAR_TYPE_INT == var->mbv_type ||
            GDS_MCA_BASE_VAR_TYPE_UNSIGNED_INT == var->mbv_type) {
            int *castme = (int*) var->mbv_storage;
            *castme = int_value;
        } else if (GDS_MCA_BASE_VAR_TYPE_UNSIGNED_LONG == var->mbv_type) {
            unsigned long *castme = (unsigned long*) var->mbv_storage;
            *castme = (unsigned long) int_value;
        } else if (GDS_MCA_BASE_VAR_TYPE_UNSIGNED_LONG_LONG == var->mbv_type) {
            unsigned long long *castme = (unsigned long long*) var->mbv_storage;
            *castme = (unsigned long long) int_value;
        } else if (GDS_MCA_BASE_VAR_TYPE_SIZE_T == var->mbv_type) {
            size_t *castme = (size_t*) var->mbv_storage;
            *castme = (size_t) int_value;
        } else if (GDS_MCA_BASE_VAR_TYPE_BOOL == var->mbv_type) {
            bool *castme = (bool*) var->mbv_storage;
            *castme = !!int_value;
        }

        return ret;
    case GDS_MCA_BASE_VAR_TYPE_DOUBLE:
        dst->lfval = strtod (src, NULL);
        break;
    case GDS_MCA_BASE_VAR_TYPE_STRING:
    case GDS_MCA_BASE_VAR_TYPE_VERSION_STRING:
        var_set_string (var, src);
        break;
    case GDS_MCA_BASE_VAR_TYPE_MAX:
        return GDS_ERROR;
    }

    return GDS_SUCCESS;
}

/*
 * Set a variable
 */
int gds_mca_base_var_set_value (int vari, const void *value, size_t size, gds_mca_base_var_source_t source,
                            const char *source_file)
{
    gds_mca_base_var_t *var;
    int ret;

    ret = var_get (vari, &var, true);
    if (GDS_SUCCESS != ret) {
        return ret;
    }

    if (!GDS_VAR_IS_VALID(var[0])) {
        return GDS_ERR_BAD_PARAM;
    }

    if (!GDS_VAR_IS_SETTABLE(var[0])) {
        return GDS_ERR_PERM;
    }

    if (NULL != var->mbv_enumerator) {
        /* Validate */
        ret = var->mbv_enumerator->string_from_value(var->mbv_enumerator,
                                                     ((int *) value)[0], NULL);
        if (GDS_SUCCESS != ret) {
            return ret;
        }
    }

    if (GDS_MCA_BASE_VAR_TYPE_STRING != var->mbv_type && GDS_MCA_BASE_VAR_TYPE_VERSION_STRING != var->mbv_type) {
        memmove (var->mbv_storage, value, var_type_sizes[var->mbv_type]);
    } else {
        var_set_string (var, (char *) value);
    }

    var->mbv_source = source;

    if (GDS_MCA_BASE_VAR_SOURCE_FILE == source && NULL != source_file) {
        var->mbv_file_value = NULL;
        var->mbv_source_file = append_filename_to_list(source_file);
    }

    return GDS_SUCCESS;
}

/*
 * Deregister a parameter
 */
int gds_mca_base_var_deregister(int vari)
{
    gds_mca_base_var_t *var;
    int ret;

    ret = var_get (vari, &var, false);
    if (GDS_SUCCESS != ret) {
        return ret;
    }

    if (!GDS_VAR_IS_VALID(var[0])) {
        return GDS_ERR_BAD_PARAM;
    }

    /* Mark this parameter as invalid but keep its info in case this
       parameter is reregistered later */
    var->mbv_flags &= ~GDS_MCA_BASE_VAR_FLAG_VALID;

    /* Done deregistering synonym */
    if (GDS_MCA_BASE_VAR_FLAG_SYNONYM & var->mbv_flags) {
        return GDS_SUCCESS;
    }

    /* Release the current value if it is a string. */
    if ((GDS_MCA_BASE_VAR_TYPE_STRING == var->mbv_type || GDS_MCA_BASE_VAR_TYPE_VERSION_STRING == var->mbv_type) &&
        var->mbv_storage->stringval) {
        free (var->mbv_storage->stringval);
        var->mbv_storage->stringval = NULL;
    } else if (var->mbv_enumerator && !var->mbv_enumerator->enum_is_static) {
        GDS_RELEASE(var->mbv_enumerator);
    }

    var->mbv_enumerator = NULL;

    var->mbv_storage = NULL;

    return GDS_SUCCESS;
}

static int var_get (int vari, gds_mca_base_var_t **var_out, bool original)
{
    gds_mca_base_var_t *var;

    if (var_out) {
        *var_out = NULL;
    }

    /* Check for bozo cases */
    if (!gds_mca_base_var_initialized) {
        return GDS_ERROR;
    }

    if (vari < 0) {
        return GDS_ERR_BAD_PARAM;
    }

    var = gds_pointer_array_get_item (&gds_mca_base_vars, vari);
    if (NULL == var) {
        return GDS_ERR_BAD_PARAM;
    }

    if (GDS_VAR_IS_SYNONYM(var[0]) && original) {
        return var_get(var->mbv_synonym_for, var_out, false);
    }

    if (var_out) {
        *var_out = var;
    }

    return GDS_SUCCESS;
}

int gds_mca_base_var_env_name(const char *param_name,
                          char **env_name)
{
    int ret;

    assert (NULL != env_name);

    ret = asprintf(env_name, "%s%s", mca_prefix, param_name);
    if (0 > ret) {
        return GDS_ERR_OUT_OF_RESOURCE;
    }

    return GDS_SUCCESS;
}

/*
 * Find the index for an MCA parameter based on its names.
 */
static int var_find_by_name (const char *full_name, int *vari, bool invalidok)
{
    gds_mca_base_var_t *var = NULL;
    void *tmp;
    int rc;

    rc = gds_hash_table_get_value_ptr (&gds_mca_base_var_index_hash, full_name, strlen (full_name),
                                        &tmp);
    if (GDS_SUCCESS != rc) {
        return rc;
    }

    (void) var_get ((int)(uintptr_t) tmp, &var, false);

    if (invalidok || (var && GDS_VAR_IS_VALID(var[0]))) {
        *vari = (int)(uintptr_t) tmp;
        return GDS_SUCCESS;
    }

    return GDS_ERR_NOT_FOUND;
}

static int var_find (const char *project_name, const char *framework_name,
                     const char *component_name, const char *variable_name,
                     bool invalidok)
{
    char *full_name;
    int ret, vari;

    ret = gds_mca_base_var_generate_full_name4 (NULL, framework_name, component_name,
                                            variable_name, &full_name);
    if (GDS_SUCCESS != ret) {
        return GDS_ERROR;
    }

    ret = var_find_by_name(full_name, &vari, invalidok);

    /* NTH: should we verify the name components match? */

    free (full_name);

    if (GDS_SUCCESS != ret) {
        return ret;
    }

    return vari;
}

/*
 * Find the index for an MCA parameter based on its name components.
 */
int gds_mca_base_var_find (const char *project_name, const char *framework_name,
                       const char *component_name, const char *variable_name)
{
    return var_find (project_name, framework_name, component_name, variable_name, false);
}

/*
 * Find the index for an MCA parameter based on full name.
 */
int gds_mca_base_var_find_by_name (const char *full_name, int *vari)
{
    return var_find_by_name (full_name, vari, false);
}

int gds_mca_base_var_set_flag (int vari, gds_mca_base_var_flag_t flag, bool set)
{
    gds_mca_base_var_t *var;
    int ret;

    ret = var_get (vari, &var, true);
    if (GDS_SUCCESS != ret || GDS_VAR_IS_SYNONYM(var[0])) {
        return GDS_ERR_BAD_PARAM;
    }

    var->mbv_flags = (var->mbv_flags & ~flag) | (set ? flag : 0);

    /* All done */
    return GDS_SUCCESS;
}

/*
 * Return info on a parameter at an index
 */
int gds_mca_base_var_get (int vari, const gds_mca_base_var_t **var)
{
    int ret;
    ret = var_get (vari, (gds_mca_base_var_t **) var, false);

    if (GDS_SUCCESS != ret) {
        return ret;
    }

    if (!GDS_VAR_IS_VALID(*(var[0]))) {
        return GDS_ERR_NOT_FOUND;
    }

    return GDS_SUCCESS;
}

/*
 * Make an argv-style list of strings suitable for an environment
 */
int gds_mca_base_var_build_env(char ***env, int *num_env, bool internal)
{
    gds_mca_base_var_t *var;
    size_t i, len;
    int ret;

    /* Check for bozo cases */

    if (!gds_mca_base_var_initialized) {
        return GDS_ERROR;
    }

    /* Iterate through all the registered parameters */

    len = gds_pointer_array_get_size(&gds_mca_base_vars);
    for (i = 0; i < len; ++i) {
        char *value_string;
        char *str = NULL;

        var = gds_pointer_array_get_item (&gds_mca_base_vars, i);
        if (NULL == var) {
            continue;
        }

        /* Don't output default values or internal variables (unless
           requested) */
        if (GDS_MCA_BASE_VAR_SOURCE_DEFAULT == var->mbv_source ||
            (!internal && GDS_VAR_IS_INTERNAL(var[0]))) {
            continue;
        }

        if ((GDS_MCA_BASE_VAR_TYPE_STRING == var->mbv_type || GDS_MCA_BASE_VAR_TYPE_VERSION_STRING == var->mbv_type) &&
            NULL == var->mbv_storage->stringval) {
            continue;
        }

        ret = var_value_string (var, &value_string);
        if (GDS_SUCCESS != ret) {
            goto cleanup;
        }

        ret = asprintf (&str, "%s%s=%s", mca_prefix, var->mbv_full_name,
                        value_string);
        free (value_string);
        if (0 > ret) {
            goto cleanup;
        }

        gds_argv_append(num_env, env, str);
        free(str);

        switch (var->mbv_source) {
        case GDS_MCA_BASE_VAR_SOURCE_FILE:
        case GDS_MCA_BASE_VAR_SOURCE_OVERRIDE:
            asprintf (&str, "%sSOURCE_%s=FILE:%s", mca_prefix, var->mbv_full_name,
                      gds_mca_base_var_source_file (var));
            break;
        case GDS_MCA_BASE_VAR_SOURCE_COMMAND_LINE:
            asprintf (&str, "%sSOURCE_%s=COMMAND_LINE", mca_prefix, var->mbv_full_name);
            break;
        case GDS_MCA_BASE_VAR_SOURCE_ENV:
        case GDS_MCA_BASE_VAR_SOURCE_SET:
        case GDS_MCA_BASE_VAR_SOURCE_DEFAULT:
            str = NULL;
            break;
        case GDS_MCA_BASE_VAR_SOURCE_MAX:
            goto cleanup;
        }

        if (NULL != str) {
            gds_argv_append(num_env, env, str);
            free(str);
        }
    }

    /* All done */

    return GDS_SUCCESS;

    /* Error condition */

 cleanup:
    if (*num_env > 0) {
        gds_argv_free(*env);
        *num_env = 0;
        *env = NULL;
    }
    return GDS_ERR_NOT_FOUND;
}

/*
 * Shut down the MCA parameter system (normally only invoked by the
 * MCA framework itself).
 */
int gds_mca_base_var_finalize(void)
{
    gds_object_t *gdsect;
    gds_list_item_t *item;
    int size, i;

    if (gds_mca_base_var_initialized) {
        size = gds_pointer_array_get_size(&gds_mca_base_vars);
        for (i = 0 ; i < size ; ++i) {
            gdsect = gds_pointer_array_get_item (&gds_mca_base_vars, i);
            if (NULL != gdsect) {
                GDS_RELEASE(gdsect);
            }
        }
        GDS_DESTRUCT(&gds_mca_base_vars);

        while (NULL !=
               (item = gds_list_remove_first(&gds_mca_base_var_file_values))) {
            GDS_RELEASE(item);
        }
        GDS_DESTRUCT(&gds_mca_base_var_file_values);

        while (NULL !=
               (item = gds_list_remove_first(&gds_mca_base_envar_file_values))) {
            GDS_RELEASE(item);
        }
        GDS_DESTRUCT(&gds_mca_base_envar_file_values);

        while (NULL !=
               (item = gds_list_remove_first(&gds_mca_base_var_override_values))) {
            GDS_RELEASE(item);
        }
        GDS_DESTRUCT(&gds_mca_base_var_override_values);

        if( NULL != cwd ) {
            free(cwd);
            cwd = NULL;
        }

        gds_mca_base_var_initialized = false;
        gds_mca_base_var_count = 0;

        if (NULL != gds_mca_base_var_file_list) {
            gds_argv_free(gds_mca_base_var_file_list);
        }
        gds_mca_base_var_file_list = NULL;

        (void) gds_mca_base_var_group_finalize ();

        GDS_DESTRUCT(&gds_mca_base_var_index_hash);

        free (gds_mca_base_envar_files);
        gds_mca_base_envar_files = NULL;
    }

    /* All done */

    return GDS_SUCCESS;
}


/*************************************************************************/
static int fixup_files(char **file_list, char * path, bool rel_path_search, char sep) {
    int exit_status = GDS_SUCCESS;
    char **files = NULL;
    char **search_path = NULL;
    char * tmp_file = NULL;
    char **argv = NULL;
    char *rel_path;
    int mode = R_OK; /* The file exists, and we can read it */
    int count, i, argc = 0;

    search_path = gds_argv_split(path, GDS_ENV_SEP);
    files = gds_argv_split(*file_list, sep);
    count = gds_argv_count(files);

    rel_path = force_agg_path ? force_agg_path : cwd;

    /* Read in reverse order, so we can preserve the original ordering */
    for (i = 0 ; i < count; ++i) {
        char *msg_path = path;
        if (gds_path_is_absolute(files[i])) {
            /* Absolute paths preserved */
            tmp_file = gds_path_access(files[i], NULL, mode);
        } else if (!rel_path_search && NULL != strchr(files[i], GDS_PATH_SEP[0])) {
            /* Resolve all relative paths:
             *  - If filename contains a "/" (e.g., "./foo" or "foo/bar")
             *    - look for it relative to cwd
             *    - if exists, use it
             *    - ow warn/error
             */
            msg_path = rel_path;
            tmp_file = gds_path_access(files[i], rel_path, mode);
        } else {
            /* Resolve all relative paths:
             * - Use path resolution
             *    - if found and readable, use it
             *    - otherwise, warn/error
             */
            tmp_file = gds_path_find (files[i], search_path, mode, NULL);
        }

        if (NULL == tmp_file) {
            gds_show_help("help-mca-var.txt", "missing-param-file",
                           true, getpid(), files[i], msg_path);
            exit_status = GDS_ERROR;
            break;
        }

        gds_argv_append(&argc, &argv, tmp_file);

        free(tmp_file);
        tmp_file = NULL;
    }

    if (GDS_SUCCESS == exit_status) {
        free(*file_list);
        *file_list = gds_argv_join(argv, sep);
    }

    if( NULL != files ) {
        gds_argv_free(files);
        files = NULL;
    }

    if( NULL != argv ) {
        gds_argv_free(argv);
        argv = NULL;
    }

    if( NULL != search_path ) {
        gds_argv_free(search_path);
        search_path = NULL;
    }

    return exit_status;
}

static int read_files(char *file_list, gds_list_t *file_values, char sep)
{
    char **tmp = gds_argv_split(file_list, sep);
    int i, count;

    if (!tmp) {
        return GDS_ERR_OUT_OF_RESOURCE;
    }

    count = gds_argv_count(tmp);

    /* Iterate through all the files passed in -- read them in reverse
       order so that we preserve unix/shell path-like semantics (i.e.,
       the entries farthest to the left get precedence) */

    for (i = count - 1; i >= 0; --i) {
        char *file_name = append_filename_to_list (tmp[i]);
        gds_mca_base_parse_paramfile(file_name, file_values);
    }

    gds_argv_free (tmp);

    gds_mca_base_internal_env_store();

    return GDS_SUCCESS;
}

/******************************************************************************/
static int register_variable (const char *project_name, const char *framework_name,
                              const char *component_name, const char *variable_name,
                              const char *description, gds_mca_base_var_type_t type,
                              gds_mca_base_var_enum_t *enumerator, int bind,
                              gds_mca_base_var_flag_t flags, gds_mca_base_var_info_lvl_t info_lvl,
                              gds_mca_base_var_scope_t scope, int synonym_for,
                              void *storage)
{
    int ret, var_index, group_index, tmp;
    gds_mca_base_var_group_t *group;
    gds_mca_base_var_t *var, *original = NULL;

    /* Developer error. Storage can not be NULL and type must exist */
    assert (((flags & GDS_MCA_BASE_VAR_FLAG_SYNONYM) || NULL != storage) && type >= 0 && type < GDS_MCA_BASE_VAR_TYPE_MAX);

#if GDS_ENABLE_DEBUG
    /* Developer error: check for alignments */
    uintptr_t align = 0;
    switch (type) {
    case GDS_MCA_BASE_VAR_TYPE_INT:
        align = GDS_ALIGNMENT_INT;
        break;
    case GDS_MCA_BASE_VAR_TYPE_UNSIGNED_INT:
        align = GDS_ALIGNMENT_INT;
        break;
    case GDS_MCA_BASE_VAR_TYPE_UNSIGNED_LONG:
        align = GDS_ALIGNMENT_LONG;
        break;
    case GDS_MCA_BASE_VAR_TYPE_UNSIGNED_LONG_LONG:
        align = GDS_ALIGNMENT_LONG_LONG;
        break;
    case GDS_MCA_BASE_VAR_TYPE_SIZE_T:
        align = GDS_ALIGNMENT_SIZE_T;
        break;
    case GDS_MCA_BASE_VAR_TYPE_BOOL:
        align = GDS_ALIGNMENT_BOOL;
        break;
    case GDS_MCA_BASE_VAR_TYPE_DOUBLE:
        align = GDS_ALIGNMENT_DOUBLE;
        break;
    case GDS_MCA_BASE_VAR_TYPE_VERSION_STRING:
    case GDS_MCA_BASE_VAR_TYPE_STRING:
    default:
        align = 0;
        break;
    }

    if (0 != align) {
        assert(((uintptr_t) storage) % align == 0);
    }

    /* Also check to ensure that synonym_for>=0 when
       MCA_BCASE_VAR_FLAG_SYNONYM is specified */
    if (flags & GDS_MCA_BASE_VAR_FLAG_SYNONYM && synonym_for < 0) {
        assert((flags & GDS_MCA_BASE_VAR_FLAG_SYNONYM) && synonym_for >= 0);
    }
#endif

    if (flags & GDS_MCA_BASE_VAR_FLAG_SYNONYM) {
        original = gds_pointer_array_get_item (&gds_mca_base_vars, synonym_for);
        if (NULL == original) {
            /* Attempting to create a synonym for a non-existent variable. probably a
             * developer error. */
            assert (NULL != original);
            return GDS_ERR_NOT_FOUND;
        }
    }

    /* Initialize the array if it has never been initialized */
    if (!gds_mca_base_var_initialized) {
        gds_mca_base_var_init();
    }

    /* See if this entry is already in the array */
    var_index = var_find (project_name, framework_name, component_name, variable_name,
                          true);

    if (0 > var_index) {
        /* Create a new parameter entry */
        group_index = gds_mca_base_var_group_register (project_name, framework_name, component_name,
                                                   NULL);
        if (-1 > group_index) {
            return group_index;
        }

        /* Read-only and constant variables can't be settable */
        if (scope < GDS_MCA_BASE_VAR_SCOPE_LOCAL || (flags & GDS_MCA_BASE_VAR_FLAG_DEFAULT_ONLY)) {
            if ((flags & GDS_MCA_BASE_VAR_FLAG_DEFAULT_ONLY) && (flags & GDS_MCA_BASE_VAR_FLAG_SETTABLE)) {
                gds_show_help("help-mca-var.txt", "invalid-flag-combination",
                               true, "GDS_MCA_BASE_VAR_FLAG_DEFAULT_ONLY", "GDS_MCA_BASE_VAR_FLAG_SETTABLE");
                return GDS_ERROR;
            }

            /* Should we print a warning for other cases? */
            flags &= ~GDS_MCA_BASE_VAR_FLAG_SETTABLE;
        }

        var = GDS_NEW(gds_mca_base_var_t);

        var->mbv_type        = type;
        var->mbv_flags       = flags;
        var->mbv_group_index = group_index;
        var->mbv_info_lvl  = info_lvl;
        var->mbv_scope       = scope;
        var->mbv_synonym_for = synonym_for;
        var->mbv_bind        = bind;

        if (NULL != description) {
            var->mbv_description = strdup(description);
        }

        if (NULL != variable_name) {
            var->mbv_variable_name = strdup(variable_name);
            if (NULL == var->mbv_variable_name) {
                GDS_RELEASE(var);
                return GDS_ERR_OUT_OF_RESOURCE;
            }
        }

        ret = gds_mca_base_var_generate_full_name4 (NULL, framework_name, component_name,
                                                variable_name, &var->mbv_full_name);
        if (GDS_SUCCESS != ret) {
            GDS_RELEASE(var);
            return GDS_ERROR;
        }

        ret = gds_mca_base_var_generate_full_name4 (project_name, framework_name, component_name,
                                                variable_name, &var->mbv_long_name);
        if (GDS_SUCCESS != ret) {
            GDS_RELEASE(var);
            return GDS_ERROR;
        }

        /* Add it to the array.  Note that we copy the mca_var_t by value,
           so the entire contents of the struct is copied.  The synonym list
           will always be empty at this point, so there's no need for an
           extra RETAIN or RELEASE. */
        var_index = gds_pointer_array_add (&gds_mca_base_vars, var);
        if (0 > var_index) {
            GDS_RELEASE(var);
            return GDS_ERROR;
        }

        var->mbv_index = var_index;

        if (0 <= group_index) {
            gds_mca_base_var_group_add_var (group_index, var_index);
        }

        gds_mca_base_var_count++;
        if (0 <= var_find_by_name (var->mbv_full_name, &tmp, 0)) {
            /* XXX --- FIXME: variable overshadows an existing variable. this is difficult to support */
            assert (0);
        }

        gds_hash_table_set_value_ptr (&gds_mca_base_var_index_hash, var->mbv_full_name, strlen (var->mbv_full_name),
                                       (void *)(uintptr_t) var_index);
    } else {
        ret = var_get (var_index, &var, false);
        if (GDS_SUCCESS != ret) {
            /* Shouldn't ever happen */
            return GDS_ERROR;
        }

        ret = gds_mca_base_var_group_get_internal (var->mbv_group_index, &group, true);
        if (GDS_SUCCESS != ret) {
            /* Shouldn't ever happen */
            return GDS_ERROR;
        }

        if (!group->group_isvalid) {
            group->group_isvalid = true;
        }

        /* Verify the name components match */
        if (0 != compare_strings(framework_name, group->group_framework) ||
            0 != compare_strings(component_name, group->group_component) ||
            0 != compare_strings(variable_name, var->mbv_variable_name)) {
            gds_show_help("help-mca-var.txt", "var-name-conflict",
                           true, var->mbv_full_name, framework_name,
                           component_name, variable_name,
                           group->group_framework, group->group_component,
                           var->mbv_variable_name);
            /* This is developer error. abort! */
            assert (0);
            return GDS_ERROR;
        }

        if (var->mbv_type != type) {
#if GDS_ENABLE_DEBUG
            gds_show_help("help-mca-var.txt",
                           "re-register-with-different-type",
                           true, var->mbv_full_name);
#endif
            return GDS_ERR_VALUE_OUT_OF_BOUNDS;
        }
    }

    if (GDS_MCA_BASE_VAR_TYPE_BOOL == var->mbv_type) {
        enumerator = &gds_mca_base_var_enum_bool;
    } else if (NULL != enumerator) {
        if (var->mbv_enumerator) {
            GDS_RELEASE (var->mbv_enumerator);
        }

        if (!enumerator->enum_is_static) {
            GDS_RETAIN(enumerator);
        }
    }

    var->mbv_enumerator = enumerator;

    if (!original) {
        var->mbv_storage = storage;

        /* make a copy of the default string value */
        if ((GDS_MCA_BASE_VAR_TYPE_STRING == type || GDS_MCA_BASE_VAR_TYPE_VERSION_STRING == type) && NULL != ((char **)storage)[0]) {
            ((char **)storage)[0] = strdup (((char **)storage)[0]);
        }
    } else {
        /* synonym variable */
        gds_value_array_append_item(&original->mbv_synonyms, &var_index);
    }

    /* go ahead and mark this variable as valid */
    var->mbv_flags |= GDS_MCA_BASE_VAR_FLAG_VALID;

    ret = var_set_initial (var, original);
    if (GDS_SUCCESS != ret) {
        return ret;
    }

    /* All done */
    return var_index;
}

int gds_mca_base_var_register (const char *project_name, const char *framework_name,
                           const char *component_name, const char *variable_name,
                           const char *description, gds_mca_base_var_type_t type,
                           gds_mca_base_var_enum_t *enumerator, int bind,
                           gds_mca_base_var_flag_t flags,
                           gds_mca_base_var_info_lvl_t info_lvl,
                           gds_mca_base_var_scope_t scope, void *storage)
{
    /* Only integer variables can have enumerator */
    assert (NULL == enumerator || (GDS_MCA_BASE_VAR_TYPE_INT == type || GDS_MCA_BASE_VAR_TYPE_UNSIGNED_INT == type));

    return register_variable (project_name, framework_name, component_name,
                              variable_name, description, type, enumerator,
                              bind, flags, info_lvl, scope, -1, storage);
}

int gds_mca_base_component_var_register (const gds_mca_base_component_t *component,
                                          const char *variable_name, const char *description,
                                          gds_mca_base_var_type_t type, gds_mca_base_var_enum_t *enumerator,
                                          int bind, gds_mca_base_var_flag_t flags,
                                          gds_mca_base_var_info_lvl_t info_lvl,
                                          gds_mca_base_var_scope_t scope, void *storage)
{
    return gds_mca_base_var_register (component->gds_mca_project_name,
                                       component->gds_mca_type_name,
                                       component->gds_mca_component_name,
                                       variable_name, description, type, enumerator,
                                       bind, flags | GDS_MCA_BASE_VAR_FLAG_DWG,
                                       info_lvl, scope, storage);
}

int gds_mca_base_framework_var_register (const gds_mca_base_framework_t *framework,
                                          const char *variable_name,
                                          const char *help_msg, gds_mca_base_var_type_t type,
                                          gds_mca_base_var_enum_t *enumerator, int bind,
                                          gds_mca_base_var_flag_t flags,
                                          gds_mca_base_var_info_lvl_t info_level,
                                          gds_mca_base_var_scope_t scope, void *storage)
{
    return gds_mca_base_var_register (framework->framework_project, framework->framework_name,
                                       "base", variable_name, help_msg, type, enumerator, bind,
                                       flags | GDS_MCA_BASE_VAR_FLAG_DWG, info_level, scope, storage);
}

int gds_mca_base_var_register_synonym (int synonym_for, const char *project_name,
                                        const char *framework_name,
                                        const char *component_name,
                                        const char *synonym_name,
                                        gds_mca_base_var_syn_flag_t flags)
{
    gds_mca_base_var_flag_t var_flags = (gds_mca_base_var_flag_t) GDS_MCA_BASE_VAR_FLAG_SYNONYM;
    gds_mca_base_var_t *var;
    int ret;

    ret = var_get (synonym_for, &var, false);
    if (GDS_SUCCESS != ret || GDS_VAR_IS_SYNONYM(var[0])) {
        return GDS_ERR_BAD_PARAM;
    }

    if (flags & GDS_MCA_BASE_VAR_SYN_FLAG_DEPRECATED) {
        var_flags |= GDS_MCA_BASE_VAR_FLAG_DEPRECATED;
    }
    if (flags & GDS_MCA_BASE_VAR_SYN_FLAG_INTERNAL) {
        var_flags |= GDS_MCA_BASE_VAR_FLAG_INTERNAL;
    }

    return register_variable (project_name, framework_name, component_name,
                              synonym_name, var->mbv_description, var->mbv_type, var->mbv_enumerator,
                              var->mbv_bind, var_flags, var->mbv_info_lvl, var->mbv_scope,
                              synonym_for, NULL);
}

static int var_get_env (gds_mca_base_var_t *var, const char *name, char **source, char **value)
{
    char *source_env, *value_env;
    int ret;

    ret = asprintf (&source_env, "%sSOURCE_%s", mca_prefix, name);
    if (0 > ret) {
        return GDS_ERROR;
    }

    ret = asprintf (&value_env, "%s%s", mca_prefix, name);
    if (0 > ret) {
        free (source_env);
        return GDS_ERROR;
    }

    *source = getenv (source_env);
    *value = getenv (value_env);

    free (source_env);
    free (value_env);

    if (NULL == *value) {
        *source = NULL;
        return GDS_ERR_NOT_FOUND;
    }

    return GDS_SUCCESS;
}

/*
 * Lookup a param in the environment
 */
static int var_set_from_env (gds_mca_base_var_t *var, gds_mca_base_var_t *original)
{
    const char *var_full_name = var->mbv_full_name;
    const char *var_long_name = var->mbv_long_name;
    bool deprecated = GDS_VAR_IS_DEPRECATED(var[0]);
    bool is_synonym = GDS_VAR_IS_SYNONYM(var[0]);
    char *source_env, *value_env;
    int ret;

    ret = var_get_env (var, var_long_name, &source_env, &value_env);
    if (GDS_SUCCESS != ret) {
        ret = var_get_env (var, var_full_name, &source_env, &value_env);
    }

    if (GDS_SUCCESS != ret) {
        return ret;
    }

    /* we found an environment variable but this variable is default-only. print
       a warning. */
    if (GDS_VAR_IS_DEFAULT_ONLY(original[0])) {
        gds_show_help("help-mca-var.txt", "default-only-param-set",
                       true, var_full_name);

        return GDS_ERR_NOT_FOUND;
    }

    if (GDS_MCA_BASE_VAR_SOURCE_OVERRIDE == original->mbv_source) {
        if (!gds_mca_base_var_suppress_override_warning) {
            gds_show_help("help-mca-var.txt", "overridden-param-set",
                           true, var_full_name);
        }

        return GDS_ERR_NOT_FOUND;
    }

    original->mbv_source = GDS_MCA_BASE_VAR_SOURCE_ENV;

    if (NULL != source_env) {
        if (0 == strncasecmp (source_env, "file:", 5)) {
            original->mbv_source_file = append_filename_to_list(source_env + 5);
            if (0 == strcmp (var->mbv_source_file, gds_mca_base_var_override_file)) {
                original->mbv_source = GDS_MCA_BASE_VAR_SOURCE_OVERRIDE;
            } else {
                original->mbv_source = GDS_MCA_BASE_VAR_SOURCE_FILE;
            }
        } else if (0 == strcasecmp (source_env, "command")) {
            var->mbv_source = GDS_MCA_BASE_VAR_SOURCE_COMMAND_LINE;
        }
    }

    if (deprecated) {
        const char *new_variable = "None (going away)";

        if (is_synonym) {
            new_variable = var->mbv_full_name;
        }

        switch (var->mbv_source) {
        case GDS_MCA_BASE_VAR_SOURCE_ENV:
            gds_show_help("help-mca-var.txt", "deprecated-mca-env",
                           true, var_full_name, new_variable);
            break;
        case GDS_MCA_BASE_VAR_SOURCE_COMMAND_LINE:
            gds_show_help("help-mca-var.txt", "deprecated-mca-cli",
                           true, var_full_name, new_variable);
            break;
        case GDS_MCA_BASE_VAR_SOURCE_FILE:
        case GDS_MCA_BASE_VAR_SOURCE_OVERRIDE:
            gds_show_help("help-mca-var.txt", "deprecated-mca-file",
                           true, var_full_name, gds_mca_base_var_source_file (var),
                           new_variable);
            break;

        case GDS_MCA_BASE_VAR_SOURCE_DEFAULT:
        case GDS_MCA_BASE_VAR_SOURCE_MAX:
        case GDS_MCA_BASE_VAR_SOURCE_SET:
            /* silence compiler warnings about unhandled enumerations */
            break;
        }
    }

    return var_set_from_string (original, value_env);
}

/*
 * Lookup a param in the files
 */
static int var_set_from_file (gds_mca_base_var_t *var, gds_mca_base_var_t *original, gds_list_t *file_values)
{
    const char *var_full_name = var->mbv_full_name;
    const char *var_long_name = var->mbv_long_name;
    bool deprecated = GDS_VAR_IS_DEPRECATED(var[0]);
    bool is_synonym = GDS_VAR_IS_SYNONYM(var[0]);
    gds_mca_base_var_file_value_t *fv;

    /* Scan through the list of values read in from files and try to
       find a match.  If we do, cache it on the param (for future
       lookups) and save it in the storage. */

    GDS_LIST_FOREACH(fv, file_values, gds_mca_base_var_file_value_t) {
        if (0 != strcmp(fv->mbvfv_var, var_full_name) &&
            0 != strcmp(fv->mbvfv_var, var_long_name)) {
            continue;
        }

        /* found it */
        if (GDS_VAR_IS_DEFAULT_ONLY(var[0])) {
            gds_show_help("help-mca-var.txt", "default-only-param-set",
                           true, var_full_name);

            return GDS_ERR_NOT_FOUND;
        }

        if (GDS_MCA_BASE_VAR_FLAG_ENVIRONMENT_ONLY & original->mbv_flags) {
            gds_show_help("help-mca-var.txt", "environment-only-param",
                           true, var_full_name, fv->mbvfv_value,
                           fv->mbvfv_file);

            return GDS_ERR_NOT_FOUND;
        }

        if (GDS_MCA_BASE_VAR_SOURCE_OVERRIDE == original->mbv_source) {
            if (!gds_mca_base_var_suppress_override_warning) {
                gds_show_help("help-mca-var.txt", "overridden-param-set",
                               true, var_full_name);
            }

            return GDS_ERR_NOT_FOUND;
        }

        if (deprecated) {
            const char *new_variable = "None (going away)";

            if (is_synonym) {
                new_variable = original->mbv_full_name;
            }

            gds_show_help("help-mca-var.txt", "deprecated-mca-file",
                           true, var_full_name, fv->mbvfv_file,
                           new_variable);
        }

        original->mbv_file_value = (void *) fv;
        original->mbv_source = GDS_MCA_BASE_VAR_SOURCE_FILE;
        if (is_synonym) {
            var->mbv_file_value = (void *) fv;
            var->mbv_source = GDS_MCA_BASE_VAR_SOURCE_FILE;
        }

        return var_set_from_string (original, fv->mbvfv_value);
    }

    return GDS_ERR_NOT_FOUND;
}

/*
 * Lookup the initial value for a parameter
 */
static int var_set_initial (gds_mca_base_var_t *var, gds_mca_base_var_t *original)
{
    int ret;

    if (original) {
        /* synonym */
        var->mbv_source = original->mbv_source;
        var->mbv_file_value = original->mbv_file_value;
        var->mbv_source_file = original->mbv_source_file;
    } else {
        var->mbv_source = GDS_MCA_BASE_VAR_SOURCE_DEFAULT;
        original = var;
    }

    /* Check all the places that the param may be hiding, in priority
       order. If the default only flag is set the user will get a
       warning if they try to set a value from the environment or a
       file. */
    ret = var_set_from_file (var, original, &gds_mca_base_var_override_values);
    if (GDS_SUCCESS == ret) {
        var->mbv_flags = ~GDS_MCA_BASE_VAR_FLAG_SETTABLE & (var->mbv_flags | GDS_MCA_BASE_VAR_FLAG_OVERRIDE);
        var->mbv_source = GDS_MCA_BASE_VAR_SOURCE_OVERRIDE;
    }

    ret = var_set_from_env (var, original);
    if (GDS_ERR_NOT_FOUND != ret) {
        return ret;
    }

    ret = var_set_from_file (var, original, &gds_mca_base_envar_file_values);
    if (GDS_ERR_NOT_FOUND != ret) {
        return ret;
    }

    ret = var_set_from_file (var, original, &gds_mca_base_var_file_values);
    if (GDS_ERR_NOT_FOUND != ret) {
        return ret;
    }

    return GDS_SUCCESS;
}

/*
 * Create an empty param container
 */
static void var_constructor(gds_mca_base_var_t *var)
{
    memset ((char *) var + sizeof (var->super), 0, sizeof (*var) - sizeof (var->super));

    var->mbv_type = GDS_MCA_BASE_VAR_TYPE_MAX;
    GDS_CONSTRUCT(&var->mbv_synonyms, gds_value_array_t);
    gds_value_array_init (&var->mbv_synonyms, sizeof (int));
}


/*
 * Free all the contents of a param container
 */
static void var_destructor(gds_mca_base_var_t *var)
{
    if ((GDS_MCA_BASE_VAR_TYPE_STRING == var->mbv_type ||
                GDS_MCA_BASE_VAR_TYPE_VERSION_STRING == var->mbv_type) &&
        NULL != var->mbv_storage &&
        NULL != var->mbv_storage->stringval) {
        free (var->mbv_storage->stringval);
        var->mbv_storage->stringval = NULL;
    }

    /* don't release the boolean enumerator */
    if (var->mbv_enumerator && !var->mbv_enumerator->enum_is_static) {
        GDS_RELEASE(var->mbv_enumerator);
    }

    if (NULL != var->mbv_variable_name) {
        free(var->mbv_variable_name);
    }
    if (NULL != var->mbv_full_name) {
        free(var->mbv_full_name);
    }
    if (NULL != var->mbv_long_name) {
        free(var->mbv_long_name);
    }

    if (NULL != var->mbv_description) {
        free(var->mbv_description);
    }

    /* Destroy the synonym array */
    GDS_DESTRUCT(&var->mbv_synonyms);

    /* mark this parameter as invalid */
    var->mbv_type = GDS_MCA_BASE_VAR_TYPE_MAX;

#if GDS_ENABLE_DEBUG
    /* Cheap trick to reset everything to NULL */
    memset ((char *) var + sizeof (var->super), 0, sizeof (*var) - sizeof (var->super));
#endif
}


static void fv_constructor(gds_mca_base_var_file_value_t *f)
{
    memset ((char *) f + sizeof (f->super), 0, sizeof (*f) - sizeof (f->super));
}


static void fv_destructor(gds_mca_base_var_file_value_t *f)
{
    if (NULL != f->mbvfv_var) {
        free(f->mbvfv_var);
    }
    if (NULL != f->mbvfv_value) {
        free(f->mbvfv_value);
    }
    /* the file name is stored in mca_*/
    fv_constructor(f);
}

static char *source_name(gds_mca_base_var_t *var)
{
    char *ret;

    if (GDS_MCA_BASE_VAR_SOURCE_FILE == var->mbv_source || GDS_MCA_BASE_VAR_SOURCE_OVERRIDE == var->mbv_source) {
        struct gds_mca_base_var_file_value_t *fv = var->mbv_file_value;
        int rc;

        if (fv) {
            rc = asprintf(&ret, "file (%s:%d)", fv->mbvfv_file, fv->mbvfv_lineno);
        } else {
            rc = asprintf(&ret, "file (%s)", var->mbv_source_file);
        }

        /* some compilers will warn if the return code of asprintf is not checked (even if it is cast to void) */
        if (0 > rc) {
            return NULL;
        }
        return ret;
    } else if (GDS_MCA_BASE_VAR_SOURCE_MAX <= var->mbv_source) {
        return strdup ("unknown(!!)");
    }

    return strdup (var_source_names[var->mbv_source]);
}

static int var_value_string (gds_mca_base_var_t *var, char **value_string)
{
    const gds_mca_base_var_storage_t *value;
    int ret;

    assert (GDS_MCA_BASE_VAR_TYPE_MAX > var->mbv_type);

    ret = gds_mca_base_var_get_value(var->mbv_index, &value, NULL, NULL);
    if (GDS_SUCCESS !=ret) {
        return ret;
    }

    if (NULL == var->mbv_enumerator) {
        switch (var->mbv_type) {
        case GDS_MCA_BASE_VAR_TYPE_INT:
            ret = asprintf (value_string, "%d", value->intval);
            break;
        case GDS_MCA_BASE_VAR_TYPE_UNSIGNED_INT:
            ret = asprintf (value_string, "%u", value->uintval);
            break;
        case GDS_MCA_BASE_VAR_TYPE_UNSIGNED_LONG:
            ret = asprintf (value_string, "%lu", value->ulval);
            break;
        case GDS_MCA_BASE_VAR_TYPE_UNSIGNED_LONG_LONG:
            ret = asprintf (value_string, "%llu", value->ullval);
            break;
        case GDS_MCA_BASE_VAR_TYPE_SIZE_T:
            ret = asprintf (value_string, "%" PRIsize_t, value->sizetval);
            break;
        case GDS_MCA_BASE_VAR_TYPE_STRING:
        case GDS_MCA_BASE_VAR_TYPE_VERSION_STRING:
            ret = asprintf (value_string, "%s",
                            value->stringval ? value->stringval : "");
            break;
        case GDS_MCA_BASE_VAR_TYPE_BOOL:
            ret = asprintf (value_string, "%d", value->boolval);
            break;
        case GDS_MCA_BASE_VAR_TYPE_DOUBLE:
            ret = asprintf (value_string, "%lf", value->lfval);
            break;
        default:
            ret = -1;
            break;
        }

        ret = (0 > ret) ? GDS_ERR_OUT_OF_RESOURCE : GDS_SUCCESS;
    } else {
        /* we use an enumerator to handle string->bool and bool->string conversion */
        if (GDS_MCA_BASE_VAR_TYPE_BOOL == var->mbv_type) {
            ret = var->mbv_enumerator->string_from_value(var->mbv_enumerator, value->boolval, value_string);
        } else {
            ret = var->mbv_enumerator->string_from_value(var->mbv_enumerator, value->intval, value_string);
        }

        if (GDS_SUCCESS != ret) {
            return ret;
        }
    }

    return ret;
}

int gds_mca_base_var_check_exclusive (const char *project,
                                  const char *type_a,
                                  const char *component_a,
                                  const char *param_a,
                                  const char *type_b,
                                  const char *component_b,
                                  const char *param_b)
{
    gds_mca_base_var_t *var_a = NULL, *var_b = NULL;
    int var_ai, var_bi;

    /* XXX -- Remove me once the project name is in the componennt */
    project = NULL;

    var_ai = gds_mca_base_var_find (project, type_a, component_a, param_a);
    var_bi = gds_mca_base_var_find (project, type_b, component_b, param_b);
    if (var_bi < 0 || var_ai < 0) {
        return GDS_ERR_NOT_FOUND;
    }

    (void) var_get (var_ai, &var_a, true);
    (void) var_get (var_bi, &var_b, true);
    if (NULL == var_a || NULL == var_b) {
        return GDS_ERR_NOT_FOUND;
    }

    if (GDS_MCA_BASE_VAR_SOURCE_DEFAULT != var_a->mbv_source &&
        GDS_MCA_BASE_VAR_SOURCE_DEFAULT != var_b->mbv_source) {
        char *str_a, *str_b;

        /* Form cosmetic string names for A */
        str_a = source_name(var_a);

        /* Form cosmetic string names for B */
        str_b = source_name(var_b);

        /* Print it all out */
        gds_show_help("help-mca-var.txt",
                       "mutually-exclusive-vars",
                       true, var_a->mbv_full_name,
                       str_a, var_b->mbv_full_name,
                       str_b);

        /* Free the temp strings */
        free(str_a);
        free(str_b);

        return GDS_ERR_BAD_PARAM;
    }

    return GDS_SUCCESS;
}

int gds_mca_base_var_get_count (void)
{
    return gds_mca_base_var_count;
}

int gds_mca_base_var_dump(int vari, char ***out, gds_mca_base_var_dump_type_t output_type)
{
    const char *framework, *component, *full_name;
    int i, line_count, line = 0, enum_count = 0;
    char *value_string, *source_string, *tmp;
    int synonym_count, ret, *synonyms = NULL;
    gds_mca_base_var_t *var, *original=NULL;
    gds_mca_base_var_group_t *group;

    ret = var_get(vari, &var, false);
    if (GDS_SUCCESS != ret) {
        return ret;
    }

    ret = gds_mca_base_var_group_get_internal(var->mbv_group_index, &group, false);
    if (GDS_SUCCESS != ret) {
        return ret;
    }

    if (GDS_VAR_IS_SYNONYM(var[0])) {
        ret = var_get(var->mbv_synonym_for, &original, false);
        if (GDS_SUCCESS != ret) {
            return ret;
        }
        /* just for protection... */
        if (NULL == original) {
            return GDS_ERR_NOT_FOUND;
        }
    }

    framework = group->group_framework;
    component = group->group_component ? group->group_component : "base";
    full_name = var->mbv_full_name;

    synonym_count = gds_value_array_get_size(&var->mbv_synonyms);
    if (synonym_count) {
        synonyms = GDS_VALUE_ARRAY_GET_BASE(&var->mbv_synonyms, int);
    }

    ret = var_value_string (var, &value_string);
    if (GDS_SUCCESS != ret) {
        return ret;
    }

    source_string = source_name(var);
    if (NULL == source_string) {
        free (value_string);
        return GDS_ERR_OUT_OF_RESOURCE;
    }

    if (GDS_MCA_BASE_VAR_DUMP_PARSABLE == output_type) {
        if (NULL != var->mbv_enumerator) {
            (void) var->mbv_enumerator->get_count(var->mbv_enumerator, &enum_count);
        }

        line_count = 8 + (var->mbv_description ? 1 : 0) + (GDS_VAR_IS_SYNONYM(var[0]) ? 1 : synonym_count) +
            enum_count;

        *out = (char **) calloc (line_count + 1, sizeof (char *));
        if (NULL == *out) {
            free (value_string);
            free (source_string);
            return GDS_ERR_OUT_OF_RESOURCE;
        }

        /* build the message*/
        asprintf(&tmp, "mca:%s:%s:param:%s:", framework, component,
                 full_name);

        /* Output the value */
        char *colon = strchr(value_string, ':');
        if (NULL != colon) {
            asprintf(out[0] + line++, "%svalue:\"%s\"", tmp, value_string);
        } else {
            asprintf(out[0] + line++, "%svalue:%s", tmp, value_string);
        }

        /* Output the source */
        asprintf(out[0] + line++, "%ssource:%s", tmp, source_string);

        /* Output whether it's read only or writable */
        asprintf(out[0] + line++, "%sstatus:%s", tmp, GDS_VAR_IS_DEFAULT_ONLY(var[0]) ? "read-only" : "writeable");

        /* Output the info level of this parametere */
        asprintf(out[0] + line++, "%slevel:%d", tmp, var->mbv_info_lvl + 1);

        /* If it has a help message, output the help message */
        if (var->mbv_description) {
            asprintf(out[0] + line++, "%shelp:%s", tmp, var->mbv_description);
        }

        if (NULL != var->mbv_enumerator) {
            for (i = 0 ; i < enum_count ; ++i) {
                const char *enum_string = NULL;
                int enum_value;

                ret = var->mbv_enumerator->get_value(var->mbv_enumerator, i, &enum_value,
                                                     &enum_string);
                if (GDS_SUCCESS != ret) {
                    continue;
                }

                asprintf(out[0] + line++, "%senumerator:value:%d:%s", tmp, enum_value, enum_string);
            }
        }

        /* Is this variable deprecated? */
        asprintf(out[0] + line++, "%sdeprecated:%s", tmp, GDS_VAR_IS_DEPRECATED(var[0]) ? "yes" : "no");

        asprintf(out[0] + line++, "%stype:%s", tmp, var_type_names[var->mbv_type]);

        /* Does this parameter have any synonyms or is it a synonym? */
        if (GDS_VAR_IS_SYNONYM(var[0])) {
            asprintf(out[0] + line++, "%ssynonym_of:name:%s", tmp, original->mbv_full_name);
        } else if (gds_value_array_get_size(&var->mbv_synonyms)) {
            for (i = 0 ; i < synonym_count ; ++i) {
                gds_mca_base_var_t *synonym;

                ret = var_get(synonyms[i], &synonym, false);
                if (GDS_SUCCESS != ret) {
                    continue;
                }

                asprintf(out[0] + line++, "%ssynonym:name:%s", tmp, synonym->mbv_full_name);
            }
        }

        free (tmp);
    } else if (GDS_MCA_BASE_VAR_DUMP_READABLE == output_type) {
        /* There will be at most three lines in the pretty print case */
        *out = (char **) calloc (4, sizeof (char *));
        if (NULL == *out) {
            free (value_string);
            free (source_string);
            return GDS_ERR_OUT_OF_RESOURCE;
        }

        asprintf (out[0], "%s \"%s\" (current value: \"%s\", data source: %s, level: %d %s, type: %s",
                  GDS_VAR_IS_DEFAULT_ONLY(var[0]) ? "informational" : "parameter",
                  full_name, value_string, source_string, var->mbv_info_lvl + 1,
                  info_lvl_strings[var->mbv_info_lvl], var_type_names[var->mbv_type]);

        tmp = out[0][0];
        if (GDS_VAR_IS_DEPRECATED(var[0])) {
            asprintf (out[0], "%s, deprecated", tmp);
            free (tmp);
            tmp = out[0][0];
        }

        /* Does this parameter have any synonyms or is it a synonym? */
        if (GDS_VAR_IS_SYNONYM(var[0])) {
            asprintf(out[0], "%s, synonym of: %s)", tmp, original->mbv_full_name);
            free (tmp);
        } else if (synonym_count) {
            asprintf(out[0], "%s, synonyms: ", tmp);
            free (tmp);

            for (i = 0 ; i < synonym_count ; ++i) {
                gds_mca_base_var_t *synonym;

                ret = var_get(synonyms[i], &synonym, false);
                if (GDS_SUCCESS != ret) {
                    continue;
                }

                tmp = out[0][0];
                if (synonym_count == i+1) {
                    asprintf(out[0], "%s%s)", tmp, synonym->mbv_full_name);
                } else {
                    asprintf(out[0], "%s%s, ", tmp, synonym->mbv_full_name);
                }
                free(tmp);
            }
        } else {
            asprintf(out[0], "%s)", tmp);
            free(tmp);
        }

        line++;

        if (var->mbv_description) {
            asprintf(out[0] + line++, "%s", var->mbv_description);
        }

        if (NULL != var->mbv_enumerator) {
            char *values;

            ret = var->mbv_enumerator->dump(var->mbv_enumerator, &values);
            if (GDS_SUCCESS == ret) {
                asprintf (out[0] + line++, "Valid values: %s", values);
                free (values);
            }
        }
    } else if (GDS_MCA_BASE_VAR_DUMP_SIMPLE == output_type) {
        *out = (char **) calloc (2, sizeof (char *));
        if (NULL == *out) {
            free (value_string);
            free (source_string);
            return GDS_ERR_OUT_OF_RESOURCE;
        }

        asprintf(out[0], "%s=%s (%s)", var->mbv_full_name, value_string, source_string);
    }

    free (value_string);
    free (source_string);

    return GDS_SUCCESS;
}

