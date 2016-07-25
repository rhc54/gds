/*
 * Copyright (c) 2016      Intel, Inc. All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#ifndef GDS_GETID_H
#define GDS_GETID_H

#include <src/include/gds_config.h>
#include "include/gds_common.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

BEGIN_C_DECLS

/* lookup the effective uid and gid of a socket */
gds_status_t gds_util_getid(int sd, uid_t *uid, gid_t *gid);

END_C_DECLS

#endif /* GDS_PRINTF_H */

