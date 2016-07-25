/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2010      Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2004-2011 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2011-2014 Los Alamos National Security, LLC.  All rights
 *                         reserved.
 * Copyright (c) 2014-2016 Intel, Inc. All rights reserved.
 * Copyright (c) 2015      Research Organization for Information Science
 *                         and Technology (RIST). All rights reserved.
 * Copyright (c) 2016      Mellanox Technologies, Inc.
 *                         All rights reserved.
 * Copyright (c) 2016      IBM Corporation.  All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 *
 */

#include <src/include/gds_config.h>

#include <src/include/gds_stdint.h>
#include <src/include/hash_string.h>

#include <string.h>

#include "src/include/gds_globals.h"
#include "src/class/gds_hash_table.h"
#include "src/class/gds_pointer_array.h"
#include "src/mca/bfrops/bfrops.h"
#include "src/util/error.h"
#include "src/util/output.h"

#include "src/util/hash.h"

/**
 * Data for a particular gds process
 * The name association is maintained in the
 * proc_data hash table.
 */
typedef struct {
    /** Structure can be put on lists (including in hash tables) */
    gds_list_item_t super;
    /* List of gds_kval_t structures containing all data
       received from this process */
    gds_list_t data;
} gds_proc_data_t;
static void pdcon(gds_proc_data_t *p)
{
    GDS_CONSTRUCT(&p->data, gds_list_t);
}
static void pddes(gds_proc_data_t *p)
{
    GDS_LIST_DESTRUCT(&p->data);
}
static GDS_CLASS_INSTANCE(gds_proc_data_t,
                           gds_list_item_t,
                           pdcon, pddes);

static gds_kval_t* lookup_keyval(gds_list_t *data,
                                  const char *key);
static gds_proc_data_t* lookup_proc(gds_hash_table_t *jtable,
                                     uint64_t id, bool create);

gds_status_t gds_hash_store(gds_hash_table_t *table,
                    int rank, gds_kval_t *kin)
{
    gds_proc_data_t *proc_data;
    uint64_t id;
    gds_kval_t *hv;

    gds_output_verbose(10, gds_globals.debug_output,
                        "HASH:STORE rank %d key %s",
                        rank, kin->key);

    id = (uint64_t)rank;

    /* lookup the proc data object for this proc - create
     * it if we don't already have it */
    if (NULL == (proc_data = lookup_proc(table, id, true))) {
        return GDS_ERR_OUT_OF_RESOURCE;
    }

    /* see if we already have this key-value */
    hv = lookup_keyval(&proc_data->data, kin->key);
    if (NULL != hv) {
        /* yes we do - so remove the current value
         * and replace it */
        gds_list_remove_item(&proc_data->data, &hv->super);
        GDS_RELEASE(hv);
    }
    GDS_RETAIN(kin);
    gds_list_append(&proc_data->data, &kin->super);

    return GDS_SUCCESS;
}

gds_status_t gds_hash_fetch(gds_hash_table_t *table, int rank,
                              const char *key, gds_value_t **kvs)
{
    gds_status_t rc = GDS_SUCCESS;
    gds_proc_data_t *proc_data;
    gds_kval_t *hv;
    uint64_t id;
    char *node;

    gds_output_verbose(10, gds_globals.debug_output,
                        "HASH:FETCH rank %d key %s",
                        rank, (NULL == key) ? "NULL" : key);

    id = (uint64_t)rank;

    /* - GDS_RANK_UNDEF should return following statuses
     * GDS_ERR_PROC_ENTRY_NOT_FOUND | GDS_SUCCESS
     * - specified rank can return following statuses
     * GDS_ERR_PROC_ENTRY_NOT_FOUND | GDS_ERR_NOT_FOUND | GDS_SUCCESS
     * special logic is basing on these statuses on a client and a server */
    if (GDS_RANK_UNDEF == rank) {
        rc = gds_hash_table_get_first_key_uint64(table, &id,
                (void**)&proc_data, (void**)&node);
        if (GDS_SUCCESS != rc) {
            gds_output_verbose(10, gds_globals.debug_output,
                                "HASH:FETCH proc data for rank %d not found",
                                rank);
            return GDS_ERR_PROC_ENTRY_NOT_FOUND;
        }
    }

    while (GDS_SUCCESS == rc) {
        proc_data = lookup_proc(table, id, false);
        if (NULL == proc_data) {
            gds_output_verbose(10, gds_globals.debug_output,
                                "HASH:FETCH proc data for rank %d not found",
                                rank);
            return GDS_ERR_PROC_ENTRY_NOT_FOUND;
        }

        /* if the key is NULL, then the user wants -all- data
         * put by the specified rank */
        if (NULL == key) {
            /* we will return the data as an array of gds_info_t
             * in the kvs gds_value_t */

        } else {
            /* find the value from within this proc_data object */
            hv = lookup_keyval(&proc_data->data, key);
            if (NULL != hv) {
                /* create the copy */
                if (GDS_SUCCESS != (rc = gds_globals.mypeer->comm.bfrops->copy((void**)kvs, hv->value, GDS_VALUE))) {
                    GDS_ERROR_LOG(rc);
                    return rc;
                }
                break;
            } else if (GDS_RANK_UNDEF != rank) {
                gds_output_verbose(10, gds_globals.debug_output,
                                    "HASH:FETCH data for key %s not found", key);
                return GDS_ERR_NOT_FOUND;
            }
        }

        rc = gds_hash_table_get_next_key_uint64(table, &id,
                (void**)&proc_data, node, (void**)&node);
        if (GDS_SUCCESS != rc) {
            gds_output_verbose(10, gds_globals.debug_output,
                                "HASH:FETCH data for key %s not found", key);
            return GDS_ERR_PROC_ENTRY_NOT_FOUND;
        }
    }

    return rc;
}

gds_status_t gds_hash_fetch_by_key(gds_hash_table_t *table, const char *key,
                                     int *rank, gds_value_t **kvs, void **last)
{
    gds_status_t rc = GDS_SUCCESS;
    gds_proc_data_t *proc_data;
    gds_kval_t *hv;
    uint64_t id;
    char *node;
    static const char *key_r = NULL;

    if (key == NULL && (node = *last) == NULL) {
        return GDS_ERR_PROC_ENTRY_NOT_FOUND;
    }

    if (key == NULL && key_r == NULL) {
        return GDS_ERR_PROC_ENTRY_NOT_FOUND;
    }

    if (key) {
        rc = gds_hash_table_get_first_key_uint64(table, &id,
                (void**)&proc_data, (void**)&node);
        key_r = key;
    } else {
        rc = gds_hash_table_get_next_key_uint64(table, &id,
                (void**)&proc_data, node, (void**)&node);
    }

    gds_output_verbose(10, gds_globals.debug_output,
                        "HASH:FETCH BY KEY rank %d key %s",
                        (int)id, key_r);

    if (GDS_SUCCESS != rc) {
        gds_output_verbose(10, gds_globals.debug_output,
                            "HASH:FETCH proc data for key %s not found",
                            key_r);
        return GDS_ERR_PROC_ENTRY_NOT_FOUND;
    }

    /* find the value from within this proc_data object */
    hv = lookup_keyval(&proc_data->data, key_r);
    if (hv) {
        /* create the copy */
        if (GDS_SUCCESS != (rc = gds_globals.mypeer->comm.bfrops->copy((void**)kvs, hv->value, GDS_VALUE))) {
            GDS_ERROR_LOG(rc);
            return rc;
        }
    } else {
        return GDS_ERR_NOT_FOUND;
    }

    *rank = (int)id;
    *last = node;

    return GDS_SUCCESS;
}

gds_status_t gds_hash_remove_data(gds_hash_table_t *table,
                          int rank, const char *key)
{
    gds_status_t rc = GDS_SUCCESS;
    gds_proc_data_t *proc_data;
    gds_kval_t *kv;
    uint64_t id;
    char *node;

    id = (uint64_t)rank;

    /* if the rank is wildcard, we want to apply this to
     * all rank entries */
    if (GDS_RANK_UNDEF == rank) {
        rc = gds_hash_table_get_first_key_uint64(table, &id,
                (void**)&proc_data, (void**)&node);
        while (GDS_SUCCESS == rc) {
            if (NULL != proc_data) {
                if (NULL == key) {
                    GDS_RELEASE(proc_data);
                } else {
                    GDS_LIST_FOREACH(kv, &proc_data->data, gds_kval_t) {
                        if (0 == strcmp(key, kv->key)) {
                            gds_list_remove_item(&proc_data->data, &kv->super);
                            GDS_RELEASE(kv);
                            break;
                        }
                    }
                }
            }
            rc = gds_hash_table_get_next_key_uint64(table, &id,
                    (void**)&proc_data, node, (void**)&node);
        }
    }

    /* lookup the specified proc */
    if (NULL == (proc_data = lookup_proc(table, id, false))) {
        /* no data for this proc */
        return GDS_SUCCESS;
    }

    /* if key is NULL, remove all data for this proc */
    if (NULL == key) {
        while (NULL != (kv = (gds_kval_t*)gds_list_remove_first(&proc_data->data))) {
            GDS_RELEASE(kv);
        }
        /* remove the proc_data object itself from the jtable */
        gds_hash_table_remove_value_uint64(table, id);
        /* cleanup */
        GDS_RELEASE(proc_data);
        return GDS_SUCCESS;
    }

    /* remove this item */
    GDS_LIST_FOREACH(kv, &proc_data->data, gds_kval_t) {
        if (0 == strcmp(key, kv->key)) {
            gds_list_remove_item(&proc_data->data, &kv->super);
            GDS_RELEASE(kv);
            break;
        }
    }

    return GDS_SUCCESS;
}

/**
 * Find data for a given key in a given gds_list_t.
 */
static gds_kval_t* lookup_keyval(gds_list_t *data,
                                  const char *key)
{
    gds_kval_t *kv;

    GDS_LIST_FOREACH(kv, data, gds_kval_t) {
        if (0 == strcmp(key, kv->key)) {
            return kv;
        }
    }
    return NULL;
}


/**
 * Find proc_data_t container associated with given
 * gds_identifier_t.
 */
static gds_proc_data_t* lookup_proc(gds_hash_table_t *jtable,
                                     uint64_t id, bool create)
{
    gds_proc_data_t *proc_data = NULL;

    gds_hash_table_get_value_uint64(jtable, id, (void**)&proc_data);
    if (NULL == proc_data && create) {
        /* The proc clearly exists, so create a data structure for it */
        proc_data = GDS_NEW(gds_proc_data_t);
        if (NULL == proc_data) {
            gds_output(0, "gds:client:hash:lookup_gds_proc: unable to allocate proc_data_t\n");
            return NULL;
        }
        gds_hash_table_set_value_uint64(jtable, id, proc_data);
    }

    return proc_data;
}
