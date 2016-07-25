/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2004-2010 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2005 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2007-2012 Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2007      Sun Microsystems, Inc.  All rights reserved.
 * Copyright (c) 2009      Oak Ridge National Labs.  All rights reserved.
 * Copyright (c) 2010-2015 Los Alamos National Security, LLC.
 *                         All rights reserved.
 * Copyright (c) 2013-2016 Intel, Inc. All rights reserved
 * Copyright (c) 2015      Research Organization for Information Science
 *                         and Technology (RIST). All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

/** @file **/

#include <src/include/gds_config.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "src/util/output.h"
#include "src/util/show_help.h"
#include "src/mca/base/base.h"
#include "src/mca/base/gds_mca_base_var.h"
#include "src/mca/pinstalldirs/base/base.h"
#include "src/mca/bfrops/base/base.h"
#include "src/mca/psec/base/base.h"

#include "src/event/gds_event.h"
#include "src/include/types.h"
#include "src/usock/usock.h"
#include "src/util/error.h"
#include "src/util/keyval_parse.h"

#include "src/runtime/gds_rte.h"
#include "src/runtime/gds_progress_threads.h"

#if GDS_CC_USE_PRAGMA_IDENT
#pragma ident GDS_IDENT_STRING
#elif GDS_CC_USE_IDENT
#ident GDS_IDENT_STRING
#endif
const char gds_version_string[] = GDS_IDENT_STRING;

int gds_initialized = 0;
bool gds_init_called = false;
gds_globals_t gds_globals = {
    .init_cntr = 0,
    .mypeer = NULL,
    .pindex = 0,
    .evbase = NULL,
    .external_evbase = false,
    .debug_output = -1,
    .proc_type = GDS_PROC_UNDEF,
    .connected = false,
    .cache_local = NULL,
    .cache_remote = NULL
};


int gds_rte_init(gds_proc_type_t type,
                  gds_info_t info[], size_t ninfo)
{
    int ret, debug_level;
    char *error = NULL, *evar;
    char *param;
    size_t n;

    if( ++gds_initialized != 1 ) {
        if( gds_initialized < 1 ) {
            return GDS_ERROR;
        }
        return GDS_SUCCESS;
    }

    #if GDS_NO_LIB_DESTRUCTOR
        if (gds_init_called) {
            /* can't use show_help here */
            fprintf (stderr, "gds_init: attempted to initialize after finalize without compiler "
                     "support for either __attribute__(destructor) or linker support for -fini -- process "
                     "will likely abort\n");
            return GDS_ERR_NOT_SUPPORTED;
        }
    #endif

    gds_init_called = true;

    /* initialize the output system */
    if (!gds_output_init()) {
        return GDS_ERROR;
    }

    /* initialize install dirs code */
    if (GDS_SUCCESS != (ret = gds_mca_base_framework_open(&gds_pinstalldirs_base_framework, 0))) {
        fprintf(stderr, "gds_pinstalldirs_base_open() failed -- process will likely abort (%s:%d, returned %d instead of GDS_SUCCESS)\n",
                __FILE__, __LINE__, ret);
        return ret;
    }

    /* initialize the help system */
    gds_show_help_init();

    /* keyval lex-based parser */
    if (GDS_SUCCESS != (ret = gds_util_keyval_parse_init())) {
        error = "gds_util_keyval_parse_init";
        goto return_error;
    }

    /* Setup the parameter system */
    if (GDS_SUCCESS != (ret = gds_mca_base_var_init())) {
        error = "mca_base_var_init";
        goto return_error;
    }

    /* read any param files that were provided */
    if (GDS_SUCCESS != (ret = gds_mca_base_var_cache_files(false))) {
        error = "failed to cache files";
        goto return_error;
    }

    /* register params for gds */
    if (GDS_SUCCESS != (ret = gds_register_params())) {
        error = "gds_register_params";
        goto return_error;
    }

    /* initialize the mca */
    if (GDS_SUCCESS != (ret = gds_mca_base_open())) {
        error = "mca_base_open";
        goto return_error;
    }

    /* setup the globals structure */
    gds_globals.proc_type = type;
    memset(&gds_globals.myid, 0, sizeof(gds_proc_t));
    GDS_CONSTRUCT(&gds_globals.nspaces, gds_list_t);
    GDS_CONSTRUCT(&gds_globals.events, gds_events_t);
    /* get our effective id's */
    gds_globals.uid = geteuid();
    gds_globals.gid = getegid();
    /* see if debug is requested */
    if (NULL != (evar = getenv("GDS_DEBUG"))) {
        debug_level = strtol(evar, NULL, 10);
        gds_globals.debug_output = gds_output_open(NULL);
        gds_output_set_verbosity(gds_globals.debug_output, debug_level);
    }
    /* create our peer object */
    gds_globals.mypeer = GDS_NEW(gds_peer_t);

    /* scan incoming info for directives */
    if (NULL != info) {
        for (n=0; n < ninfo; n++) {
            if (0 == strcmp(GDS_EVENT_BASE, info[n].key)) {
                gds_globals.evbase = (gds_event_base_t*)info[n].value.data.ptr;
                gds_globals.external_evbase = true;
            }
        }
    }

    /* open the bfrops - we will select the active plugin later */
    if( GDS_SUCCESS != (ret = gds_mca_base_framework_open(&gds_bfrops_base_framework, 0)) ) {
        error = "gds_bfrops_base_open";
        goto return_error;
    }
    if( GDS_SUCCESS != (ret = gds_bfrop_base_select()) ) {
        error = "gds_bfrops_base_select";
        goto return_error;
    }

    /* open the psec and select the default module for this environment */
    if (GDS_SUCCESS != (ret = gds_mca_base_framework_open(&gds_psec_base_framework, 0))) {
        error = "gds_psec_base_open";
        goto return_error;
    }
    if (GDS_SUCCESS != (ret = gds_psec_base_select())) {
        error = "gds_psec_base_select";
        goto return_error;
    }
    param = getenv("GDS_SEC_MODULE");  // if directive was given, use it
    gds_globals.mypeer->comm.sec = gds_psec_base_assign_module(param);

    /* tell libevent that we need thread support */
    gds_event_use_threads();
    if (!gds_globals.external_evbase) {
        /* create an event base and progress thread for us */
        if (NULL == (gds_globals.evbase = gds_progress_thread_init(NULL))) {
            error = "progress thread";
            ret = GDS_ERROR;
            goto return_error;
        }
    }

    /* setup the dstore support, if enabled */
    #if defined(GDS_ENABLE_DSTORE) && (GDS_ENABLE_DSTORE == 1)
        if (GDS_SUCCESS != (rc = gds_dstore_init())) {
            return rc;
        }
    #endif /* GDS_ENABLE_DSTORE */

    /* setup the usock service */
    if (GDS_PROC_SERVER == type) {
        gds_usock_init(NULL);
    } else {
        gds_usock_init(gds_client_notify_recv);
    }

    return GDS_SUCCESS;

 return_error:
    gds_show_help( "help-gds-runtime.txt",
                    "gds_init:startup:internal-failure", true,
                    error, ret );
    return ret;
}
