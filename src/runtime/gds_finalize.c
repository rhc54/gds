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
 * Copyright (c) 2008-2015 Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2010-2015 Los Alamos National Security, LLC.
 *                         All rights reserved.
 * Copyright (c) 2013-2016 Intel, Inc. All rights reserved
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

/** @file **/

#include <src/include/gds_config.h>

#include "src/class/gds_object.h"
#include "src/client/gds_client_ops.h"
#include "src/usock/usock.h"
#include "src/util/output.h"
#include "src/util/keyval_parse.h"
#include "src/util/show_help.h"
#include "src/mca/base/base.h"
#include "src/mca/base/gds_mca_base_var.h"
#include "src/mca/pinstalldirs/base/base.h"
#include "src/mca/bfrops/base/base.h"
#include "src/mca/psec/base/base.h"
#include GDS_EVENT_HEADER

#include "src/runtime/gds_rte.h"
#include "src/runtime/gds_progress_threads.h"

extern int gds_initialized;
extern bool gds_init_called;

static void __gds_attribute_destructor__ gds_cleanup (void)
{
    if (!gds_initialized) {
        /* nothing to do */
        return;
    }

    /* finalize the class/object system */
    gds_class_finalize();
}

void gds_rte_finalize(void)
{
    if( --gds_initialized != 0 ) {
        if( gds_initialized < 0 ) {
            fprintf(stderr, "GDS Finalize called too many times\n");
            return;
        }
        return;
    }

    /* shutdown communications */
    gds_usock_finalize();
    if (GDS_PROC_CLIENT == gds_globals.proc_type &&
        0 <= gds_client_globals.myserver.sd) {
        CLOSE_THE_SOCKET(gds_client_globals.myserver.sd);
    }
    #if defined(GDS_ENABLE_DSTORE) && (GDS_ENABLE_DSTORE == 1)
        gds_dstore_finalize();
    #endif /* GDS_ENABLE_DSTORE */

    /* close the security framework */
    (void)gds_mca_base_framework_close(&gds_psec_base_framework);

    /* Clear out all the registered MCA params */
    gds_deregister_params();
    gds_mca_base_var_finalize();

    /* keyval lex-based parser */
    gds_util_keyval_parse_finalize();

    (void)gds_mca_base_framework_close(&gds_pinstalldirs_base_framework);

    /* finalize the show_help system */
    gds_show_help_finalize();

    /* finalize the output system.  This has to come *after* the
       malloc code, as the malloc code needs to call into this, but
       the malloc code turning off doesn't affect gds_output that
       much */
    gds_output_finalize();

    /* close the bfrops */
    (void)gds_mca_base_framework_close(&gds_bfrops_base_framework);

    if (!gds_globals.external_evbase) {
        /* stop the progress thread */
        (void)gds_progress_thread_finalize(NULL);
        #ifdef HAVE_LIBEVENT_GLOBAL_SHUTDOWN
            gds_libevent_global_shutdown();
        #endif
    }

    /* clean out the globals */
    GDS_RELEASE(gds_globals.mypeer);
    GDS_LIST_DESTRUCT(&gds_globals.nspaces);
    if (NULL != gds_globals.cache_local) {
        GDS_RELEASE(gds_globals.cache_local);
    }
    if (NULL != gds_globals.cache_remote) {
        GDS_RELEASE(gds_globals.cache_remote);
    }
    GDS_DESTRUCT(&gds_globals.events);

    #if GDS_NO_LIB_DESTRUCTOR
        gds_cleanup();
    #endif
}
