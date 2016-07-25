/*
 * Copyright (c) 2010      Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2012      Los Alamos National Security, Inc. All rights reserved.
 * Copyright (c) 2014-2015 Intel, Inc. All rights reserved.
 * Copyright (c) 2015      Research Organization for Information Science
 *                         and Technology (RIST). All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#ifndef GDS_HASH_H
#define GDS_HASH_H

#include <src/include/gds_config.h>


#include "src/mca/bfrops/bfrops.h"
#include "src/class/gds_hash_table.h"

BEGIN_C_DECLS

/* store a value in the given hash table for the specified
 * rank index.*/
gds_status_t gds_hash_store(gds_hash_table_t *table,
                              int rank, gds_kval_t *kv);

/* Fetch the value for a specified key and rank from within
 * the given hash_table */
gds_status_t gds_hash_fetch(gds_hash_table_t *table, int rank,
                              const char *key, gds_value_t **kvs);

/* Fetch the value for a specified key from within
 * the given hash_table
 * It gets the next portion of data from table, where matching key.
 * To get the first data from table, function is called with key parameter as string.
 * Remaining data from table are obtained by calling function with a null pointer for the key parameter.*/
gds_status_t gds_hash_fetch_by_key(gds_hash_table_t *table, const char *key,
                                     int *rank, gds_value_t **kvs, void **last);

/* remove the specified key-value from the given hash_table.
 * A NULL key will result in removal of all data for the
 * given rank. A rank of GDS_RANK_WILDCARD indicates that
 * the specified key  is to be removed from the data for all
 * ranks in the table. Combining key=NULL with rank=GDS_RANK_WILDCARD
 * will therefore result in removal of all data from the
 * table */
gds_status_t gds_hash_remove_data(gds_hash_table_t *table,
                                    int rank, const char *key);

END_C_DECLS

#endif /* GDS_HASH_H */
