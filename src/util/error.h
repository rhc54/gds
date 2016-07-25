/*
 * Copyright (c) 2004-2005 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2006 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2015-2016 Intel, Inc. All rights reserved
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#ifndef GDS_UTIL_ERROR_H
#define GDS_UTIL_ERROR_H

#include <src/include/gds_config.h>


#include <gds_common.h>
#include "src/util/output.h"

 BEGIN_C_DECLS

/* internal error codes - never exposed outside of the library */
#define GDS_ERR_INVALID_CRED                           (GDS_INTERNAL_ERR_BASE -  1)
#define GDS_ERR_HANDSHAKE_FAILED                       (GDS_INTERNAL_ERR_BASE -  2)
#define GDS_ERR_READY_FOR_HANDSHAKE                    (GDS_INTERNAL_ERR_BASE -  3)
#define GDS_ERR_UNKNOWN_DATA_TYPE                      (GDS_INTERNAL_ERR_BASE -  4)
#define GDS_ERR_TYPE_MISMATCH                          (GDS_INTERNAL_ERR_BASE -  5)
#define GDS_ERR_UNPACK_INADEQUATE_SPACE                (GDS_INTERNAL_ERR_BASE -  6)
#define GDS_ERR_UNPACK_FAILURE                         (GDS_INTERNAL_ERR_BASE -  7)
#define GDS_ERR_PACK_FAILURE                           (GDS_INTERNAL_ERR_BASE -  8)
#define GDS_ERR_PACK_MISMATCH                          (GDS_INTERNAL_ERR_BASE -  9)
#define GDS_ERR_PROC_ENTRY_NOT_FOUND                   (GDS_INTERNAL_ERR_BASE - 10)
#define GDS_ERR_UNPACK_READ_PAST_END_OF_BUFFER         (GDS_INTERNAL_ERR_BASE - 11)
#define GDS_ERR_SERVER_NOT_AVAIL                       (GDS_INTERNAL_ERR_BASE - 12)
#define GDS_ERR_INVALID_KEYVALP                        (GDS_INTERNAL_ERR_BASE - 13)
#define GDS_ERR_INVALID_NUM_PARSED                     (GDS_INTERNAL_ERR_BASE - 14)
#define GDS_ERR_INVALID_ARGS                           (GDS_INTERNAL_ERR_BASE - 15)
#define GDS_ERR_INVALID_NUM_ARGS                       (GDS_INTERNAL_ERR_BASE - 16)
#define GDS_ERR_INVALID_LENGTH                         (GDS_INTERNAL_ERR_BASE - 17)
#define GDS_ERR_INVALID_VAL_LENGTH                     (GDS_INTERNAL_ERR_BASE - 18)
#define GDS_ERR_INVALID_VAL                            (GDS_INTERNAL_ERR_BASE - 19)
#define GDS_ERR_INVALID_KEY_LENGTH                     (GDS_INTERNAL_ERR_BASE - 20)
#define GDS_ERR_INVALID_KEY                            (GDS_INTERNAL_ERR_BASE - 21)
#define GDS_ERR_INVALID_ARG                            (GDS_INTERNAL_ERR_BASE - 22)
#define GDS_ERR_NOMEM                                  (GDS_INTERNAL_ERR_BASE - 23)
#define GDS_ERR_IN_ERRNO                               (GDS_INTERNAL_ERR_BASE - 24)
#define GDS_ERR_SILENT                                 (GDS_INTERNAL_ERR_BASE - 25)
#define GDS_ERR_UNKNOWN_DATATYPE                       (GDS_INTERNAL_ERR_BASE - 26)
#define GDS_ERR_RESOURCE_BUSY                          (GDS_INTERNAL_ERR_BASE - 27)
#define GDS_ERR_NOT_AVAILABLE                          (GDS_INTERNAL_ERR_BASE - 28)
#define GDS_ERR_FATAL                                  (GDS_INTERNAL_ERR_BASE - 29)
#define GDS_ERR_VALUE_OUT_OF_BOUNDS                    (GDS_INTERNAL_ERR_BASE - 30)
#define GDS_ERR_PERM                                   (GDS_INTERNAL_ERR_BASE - 31)
#define GDS_ERR_OPERATION_IN_PROGRESS                  (GDS_INTERNAL_ERR_BASE - 32)

#define GDS_ERROR_LOG(r)                                           \
 do {                                                               \
    if (GDS_ERR_SILENT != (r)) {                                   \
        gds_output(0, "GDS ERROR: %s in file %s at line %d",      \
                    PMIx_Error_string((r)), __FILE__, __LINE__);    \
    }                                                               \
} while (0)

 END_C_DECLS

#endif /* GDS_UTIL_ERROR_H */
