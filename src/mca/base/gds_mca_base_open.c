/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2004-2005 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2008 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2011      Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2015      Los Alamos National Security, LLC. All rights
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
#ifdef HAVE_SYSLOG_H
#include <syslog.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "src/mca/pinstalldirs/pinstalldirs.h"
#include "src/util/output.h"
#include "src/util/printf.h"
#include "src/mca/mca.h"
#include "src/mca/base/base.h"
#include "src/mca/base/gds_mca_base_component_repository.h"
#include "gds_common.h"
#include "src/util/gds_environ.h"

/*
 * Public variables
 */
char *gds_mca_base_component_path = NULL;
int gds_mca_base_opened = 0;
char *gds_mca_base_system_default_path = NULL;
char *gds_mca_base_user_default_path = NULL;
bool gds_mca_base_component_show_load_errors = true;
bool gds_mca_base_component_disable_dlopen = false;

static char *gds_mca_base_verbose = NULL;

/*
 * Private functions
 */
static void set_defaults(gds_output_stream_t *lds);
static void parse_verbose(char *e, gds_output_stream_t *lds);


/*
 * Main MCA initialization.
 */
int gds_mca_base_open(void)
{
    char *value;
    gds_output_stream_t lds;
    char hostname[64];
    int var_id;

    if (gds_mca_base_opened++) {
        return GDS_SUCCESS;
    }

    /* define the system and user default paths */
#if GDS_WANT_HOME_CONFIG_FILES
    gds_mca_base_system_default_path = strdup(gds_pinstall_dirs.gdslibdir);
    asprintf(&gds_mca_base_user_default_path, "%s"GDS_PATH_SEP".gds"GDS_PATH_SEP"components", gds_home_directory());
#else
    asprintf(&gds_mca_base_system_default_path, "%s", gds_pinstall_dirs.gdslibdir);
#endif

    /* see if the user wants to override the defaults */
    if (NULL == gds_mca_base_user_default_path) {
        value = strdup(gds_mca_base_system_default_path);
    } else {
        asprintf(&value, "%s%c%s", gds_mca_base_system_default_path,
                 GDS_ENV_SEP, gds_mca_base_user_default_path);
    }

    gds_mca_base_component_path = value;
    var_id = gds_mca_base_var_register("gds", "mca", "base", "component_path",
                                   "Path where to look for additional components",
                                   GDS_MCA_BASE_VAR_TYPE_STRING, NULL, 0, 0,
                                   GDS_INFO_LVL_9,
                                   GDS_MCA_BASE_VAR_SCOPE_READONLY,
                                   &gds_mca_base_component_path);
    (void) gds_mca_base_var_register_synonym(var_id, "gds", "mca", NULL, "component_path",
                                              GDS_MCA_BASE_VAR_SYN_FLAG_DEPRECATED);
    free(value);

    gds_mca_base_component_show_load_errors = true;
    var_id = gds_mca_base_var_register("gds", "mca", "base", "component_show_load_errors",
                                   "Whether to show errors for components that failed to load or not",
                                   GDS_MCA_BASE_VAR_TYPE_BOOL, NULL, 0, 0,
                                   GDS_INFO_LVL_9,
                                   GDS_MCA_BASE_VAR_SCOPE_READONLY,
                                   &gds_mca_base_component_show_load_errors);
    (void) gds_mca_base_var_register_synonym(var_id, "gds", "mca", NULL, "component_show_load_errors",
                                              GDS_MCA_BASE_VAR_SYN_FLAG_DEPRECATED);

    gds_mca_base_component_disable_dlopen = false;
    var_id = gds_mca_base_var_register("gds", "mca", "base", "component_disable_dlopen",
                                   "Whether to attempt to disable opening dynamic components or not",
                                   GDS_MCA_BASE_VAR_TYPE_BOOL, NULL, 0, 0,
                                   GDS_INFO_LVL_9,
                                   GDS_MCA_BASE_VAR_SCOPE_READONLY,
                                   &gds_mca_base_component_disable_dlopen);
    (void) gds_mca_base_var_register_synonym(var_id, "gds", "mca", NULL, "component_disable_dlopen",
                                              GDS_MCA_BASE_VAR_SYN_FLAG_DEPRECATED);

    /* What verbosity level do we want for the default 0 stream? */
    gds_mca_base_verbose = "stderr";
    var_id = gds_mca_base_var_register("gds", "mca", "base", "verbose",
                                   "Specifies where the default error output stream goes (this is separate from distinct help messages).  Accepts a comma-delimited list of: stderr, stdout, syslog, syslogpri:<notice|info|debug>, syslogid:<str> (where str is the prefix string for all syslog notices), file[:filename] (if filename is not specified, a default filename is used), fileappend (if not specified, the file is opened for truncation), level[:N] (if specified, integer verbose level; otherwise, 0 is implied)",
                                   GDS_MCA_BASE_VAR_TYPE_STRING, NULL, 0, 0,
                                   GDS_INFO_LVL_9,
                                   GDS_MCA_BASE_VAR_SCOPE_READONLY,
                                   &gds_mca_base_verbose);
    (void) gds_mca_base_var_register_synonym(var_id, "gds", "mca", NULL, "verbose",
                                              GDS_MCA_BASE_VAR_SYN_FLAG_DEPRECATED);

    memset(&lds, 0, sizeof(lds));
    if (NULL != gds_mca_base_verbose) {
        parse_verbose(gds_mca_base_verbose, &lds);
    } else {
        set_defaults(&lds);
    }
    gethostname(hostname, 64);
    asprintf(&lds.lds_prefix, "[%s:%05d] ", hostname, getpid());
    gds_output_reopen(0, &lds);
    gds_output_verbose (GDS_MCA_BASE_VERBOSE_COMPONENT, 0,
                         "mca: base: opening components at %s", gds_mca_base_component_path);
    free(lds.lds_prefix);

    /* Open up the component repository */

    return gds_mca_base_component_repository_init();
}


/*
 * Set sane default values for the lds
 */
static void set_defaults(gds_output_stream_t *lds)
{

    /* Load up defaults */

    GDS_CONSTRUCT(lds, gds_output_stream_t);
    lds->lds_syslog_priority = LOG_INFO;
    lds->lds_syslog_ident = "ompi";
    lds->lds_want_stderr = true;
}


/*
 * Parse the value of an environment variable describing verbosity
 */
static void parse_verbose(char *e, gds_output_stream_t *lds)
{
    char *edup;
    char *ptr, *next;
    bool have_output = false;

    if (NULL == e) {
        return;
    }

    edup = strdup(e);
    ptr = edup;

    /* Now parse the environment variable */

    while (NULL != ptr && strlen(ptr) > 0) {
        next = strchr(ptr, ',');
        if (NULL != next) {
            *next = '\0';
        }

        if (0 == strcasecmp(ptr, "syslog")) {
            lds->lds_want_syslog = true;
            have_output = true;
        }
        else if (strncasecmp(ptr, "syslogpri:", 10) == 0) {
            lds->lds_want_syslog = true;
            have_output = true;
            if (strcasecmp(ptr + 10, "notice") == 0)
                lds->lds_syslog_priority = LOG_NOTICE;
            else if (strcasecmp(ptr + 10, "INFO") == 0)
                lds->lds_syslog_priority = LOG_INFO;
            else if (strcasecmp(ptr + 10, "DEBUG") == 0)
                lds->lds_syslog_priority = LOG_DEBUG;
        } else if (strncasecmp(ptr, "syslogid:", 9) == 0) {
            lds->lds_want_syslog = true;
            lds->lds_syslog_ident = ptr + 9;
        }

        else if (strcasecmp(ptr, "stdout") == 0) {
            lds->lds_want_stdout = true;
            have_output = true;
        } else if (strcasecmp(ptr, "stderr") == 0) {
            lds->lds_want_stderr = true;
            have_output = true;
        }

        else if (strcasecmp(ptr, "file") == 0) {
            lds->lds_want_file = true;
            have_output = true;
        } else if (strncasecmp(ptr, "file:", 5) == 0) {
            lds->lds_want_file = true;
            lds->lds_file_suffix = ptr + 5;
            have_output = true;
        } else if (strcasecmp(ptr, "fileappend") == 0) {
            lds->lds_want_file = true;
            lds->lds_want_file_append = 1;
            have_output = true;
        }

        else if (strncasecmp(ptr, "level", 5) == 0) {
            lds->lds_verbose_level = 0;
            if (ptr[5] == GDS_ENV_SEP)
                lds->lds_verbose_level = atoi(ptr + 6);
        }

        if (NULL == next) {
            break;
        }
        ptr = next + 1;
    }

    /* If we didn't get an output, default to stderr */

    if (!have_output) {
        lds->lds_want_stderr = true;
    }

    /* All done */

    free(edup);
}
