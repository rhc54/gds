/*
 * Copyright (c) 2004-2007 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2007 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2008      Sun Microsystems, Inc.  All rights reserved.
 * Copyright (c) 2010-2012 Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2014-2016 Intel, Inc. All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

/** @file **/

#ifndef GDS_RTE_H
#define GDS_RTE_H

#include "gds_config.h"
#include "gds_common.h"
#include "src/class/gds_object.h"

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include GDS_EVENT_HEADER

#include "src/include/gds_globals.h"

BEGIN_C_DECLS

#if GDS_ENABLE_TIMING
extern char *gds_timing_sync_file;
extern char *gds_timing_output;
extern bool gds_timing_overhead;
#endif

extern int gds_initialized;

/** version string of gds */
extern const char gds_version_string[];

/**
 * Initialize the GDS layer, including the MCA system.
 *
 * @retval GDS_SUCCESS Upon success.
 * @retval GDS_ERROR Upon failure.
 *
 */
gds_status_t gds_rte_init(gds_proc_type_t type,
                            gds_info_t info[], size_t ninfo);

/**
 * Finalize the GDS layer, including the MCA system.
 *
 */
void gds_rte_finalize(void);

/**
 * Internal function.  Do not call.
 */
gds_status_t gds_register_params(void);
gds_status_t gds_deregister_params(void);

void gds_client_notify_recv(struct gds_peer_t *peer, gds_usock_hdr_t *hdr,
                             gds_buffer_t *buf, void *cbdata);

END_C_DECLS

#endif /* GDS_RTE_H */
