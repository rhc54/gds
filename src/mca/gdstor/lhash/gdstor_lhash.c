/*
 * Copyright (c) 2010      Cisco Systems, Inc.  All rights reserved. 
 * Copyright (c) 2004-2011 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2011-2013 Los Alamos National Security, LLC.  All rights
 *                         reserved. 
  * $COPYRIGHT$
 * 
 * Additional copyrights may follow
 * 
 * $HEADER$
 *
 */

#include "gds_config.h"
#include "gds/constants.h"

#include <time.h>
#include <string.h>

#include "gds_stdint.h"
#include "gds/class/gds_hash_table.h"
#include "gds/class/gds_pointer_array.h"
#include "gds/dss/dss_types.h"
#include "gds/util/error.h"
#include "gds/util/output.h"
#include "gds/util/show_help.h"

#include "gds/mca/gdstor/base/base.h"
#include "gdstor_hash.h"

static int init(void);
static void finalize(void);
static int store(const gds_identifier_t *proc,
                 gds_scope_t scope,
                 const char *key, const void *object,
                 gds_data_type_t type);
static int store_pointer(const gds_identifier_t *proc,
                         gds_value_t *kv);
static int fetch(const gds_identifier_t *proc,
                 const char *key, void **data,
                 gds_data_type_t type);
static int fetch_pointer(const gds_identifier_t *proc,
                         const char *key,
                         void **data, gds_data_type_t type);
static int fetch_multiple(const gds_identifier_t *proc,
                          gds_scope_t scope,
                          const char *key,
                          gds_list_t *kvs);
static int remove_data(const gds_identifier_t *proc, const char *key);

gds_gdstor_base_module_t gds_gdstor_hash_module = {
    init,
    finalize,
    gds_gdstor_base_set_id,
    store,
    store_pointer,
    NULL,
    fetch,
    fetch_pointer,
    fetch_multiple,
    remove_data,
    NULL
};

/* Local "globals" */
static gds_hash_table_t hash_data;

/**
 * Data for a particular gds process
 * The name association is maintained in the
 * proc_data hash table.
 */
typedef struct {
    /** Structure can be put on lists (including in hash tables) */
    gds_list_item_t super;
    /* List of gds_value_t structures containing all data
       received from this process, sorted by key. */
    gds_list_t data;
} proc_data_t;

static void proc_data_construct(proc_data_t *ptr)
{
    OBJ_CONSTRUCT(&ptr->data, gds_list_t);
}

static void proc_data_destruct(proc_data_t *ptr)
{
    gds_list_item_t *item;

    while (NULL != (item = gds_list_remove_first(&ptr->data))) {
        OBJ_RELEASE(item);
    }
    OBJ_DESTRUCT(&ptr->data);
}
OBJ_CLASS_INSTANCE(proc_data_t, gds_list_item_t,
                   proc_data_construct, proc_data_destruct);

 
static int init(void)
{
    OBJ_CONSTRUCT(&hash_data, gds_hash_table_t);
    gds_hash_table_init(&hash_data, 256);
    return GDS_SUCCESS;
}

static void finalize(void)
{
    proc_data_t *proc_data;
    uint64_t key;
    char *node;

    /* to assist in getting a clean valgrind, cycle thru the hash table
     * and release all data stored in it
     */
    if (GDS_SUCCESS == gds_hash_table_get_first_key_uint64(&hash_data, &key,
                                                             (void**)&proc_data,
                                                             (void**)&node)) {
        if (NULL != proc_data) {
            OBJ_RELEASE(proc_data);
        }
        while (GDS_SUCCESS == gds_hash_table_get_next_key_uint64(&hash_data, &key,
                                                                   (void**)&proc_data,
                                                                   node, (void**)&node)) {
            if (NULL != proc_data) {
                OBJ_RELEASE(proc_data);
            }
        }
    }
    OBJ_DESTRUCT(&hash_data);
}



/**
 * Find data for a given key in a given proc_data_t
 * container.
 */
static gds_value_t* lookup_keyval(proc_data_t *proc_data,
                                   const char *key)
{
    gds_value_t *kv = NULL;
    for (kv = (gds_value_t *) gds_list_get_first(&proc_data->data);
         kv != (gds_value_t *) gds_list_get_end(&proc_data->data);
         kv = (gds_value_t *) gds_list_get_next(kv)) {
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
static proc_data_t* lookup_gds_proc(gds_hash_table_t *jtable, gds_identifier_t id)
{
    proc_data_t *proc_data = NULL;
    
    gds_hash_table_get_value_uint64(jtable, id, (void**)&proc_data);
    if (NULL == proc_data) {
        /* The proc clearly exists, so create a data structure for it */
        proc_data = OBJ_NEW(proc_data_t);
        if (NULL == proc_data) {
            gds_output(0, "gdstor:hash:lookup_gds_proc: unable to allocate proc_data_t\n");
            return NULL;
        }
        gds_hash_table_set_value_uint64(jtable, id, proc_data);
    }
    
    return proc_data;
}

static int store(const gds_identifier_t *uid,
                 gds_scope_t scope,
                 const char *key, const void *data,
                 gds_data_type_t type)
{
    proc_data_t *proc_data;
    gds_value_t *kv;
    gds_byte_object_t *boptr;
    gds_identifier_t id;

    /* data must have an assigned scope */
    if (GDS_SCOPE_UNDEF == scope) {
        return GDS_ERR_BAD_PARAM;
    }

    /* to protect alignment, copy the data across */
    memcpy(&id, uid, sizeof(gds_identifier_t));

    /* we are at the bottom of the store priorities, so
     * if this fell to us, we store it
     */
    gds_output_verbose(1, gds_gdstor_base_framework.framework_output,
                        "gdstor:hash:store storing data for proc %" PRIu64 " for scope %d",
                        id, (int)scope);

    /* lookup the proc data object for this proc */
    if (NULL == (proc_data = lookup_gds_proc(&hash_data, id))) {
        /* unrecoverable error */
        GDS_OUTPUT_VERBOSE((5, gds_gdstor_base_framework.framework_output,
                             "gdstor:hash:store: storing key %s[%s] for proc %" PRIu64 " unrecoverably failed",
                             key, gds_dss.lookup_data_type(type), id));
        return GDS_ERR_OUT_OF_RESOURCE;
    }

    /* see if we already have this key in the data - means we are updating
     * a pre-existing value
     */
    kv = lookup_keyval(proc_data, key);
#if GDS_ENABLE_DEBUG
    char *_data_type = gds_dss.lookup_data_type(type);
    GDS_OUTPUT_VERBOSE((5, gds_gdstor_base_framework.framework_output,
                         "gdstor:hash:store: %s key %s[%s] for proc %" PRIu64 "",
                         (NULL == kv ? "storing" : "updating"),
                         key, _data_type, id));
    free (_data_type);
#endif
    if (NULL != kv) {
        gds_list_remove_item(&proc_data->data, &kv->super);
        OBJ_RELEASE(kv);
    }
    kv = OBJ_NEW(gds_value_t);
    kv->key = strdup(key);
    kv->scope = scope;
    gds_list_append(&proc_data->data, &kv->super);

    /* the type could come in as an GDS one (e.g., GDS_VPID). Since
     * the value is an GDS definition, it cannot cover GDS data
     * types, so convert to the underlying GDS type
     */
    switch (type) {
    case GDS_STRING:
        kv->type = GDS_STRING;
        if (NULL != data) {
            kv->data.string = strdup( (const char *) data);
        } else {
            kv->data.string = NULL;
        }
        break;
    case GDS_UINT64:
        if (NULL == data) {
            GDS_ERROR_LOG(GDS_ERR_BAD_PARAM);
            return GDS_ERR_BAD_PARAM;
        }
        kv->type = GDS_UINT64;
        /* to avoid alignment issues */
        memcpy(&kv->data.uint64, data, 8);
        break;
    case GDS_UINT32:
        if (NULL == data) {
            GDS_ERROR_LOG(GDS_ERR_BAD_PARAM);
            return GDS_ERR_BAD_PARAM;
        }
        kv->type = GDS_UINT32;
        /* to avoid alignment issues */
        memcpy(&kv->data.uint32, data, 4);
        break;
    case GDS_UINT16:
        if (NULL == data) {
            GDS_ERROR_LOG(GDS_ERR_BAD_PARAM);
            return GDS_ERR_BAD_PARAM;
        }
        kv->type = GDS_UINT16;
        /* to avoid alignment issues */
        memcpy(&kv->data.uint16, data, 2);
        break;
    case GDS_INT:
        if (NULL == data) {
            GDS_ERROR_LOG(GDS_ERR_BAD_PARAM);
            return GDS_ERR_BAD_PARAM;
        }
        kv->type = GDS_INT;
        /* to avoid alignment issues */
        memcpy(&kv->data.integer, data, sizeof(int));
        break;
    case GDS_UINT:
        if (NULL == data) {
            GDS_ERROR_LOG(GDS_ERR_BAD_PARAM);
            return GDS_ERR_BAD_PARAM;
        }
        kv->type = GDS_UINT;
        /* to avoid alignment issues */
        memcpy(&kv->data.uint, data, sizeof(unsigned int));
        break;
    case GDS_FLOAT:
        if (NULL == data) {
            GDS_ERROR_LOG(GDS_ERR_BAD_PARAM);
            return GDS_ERR_BAD_PARAM;
        }
        kv->type = GDS_FLOAT;
        memcpy(&kv->data.fval, data, sizeof(float));
        break;
    case GDS_BYTE_OBJECT:
        kv->type = GDS_BYTE_OBJECT;
        boptr = (gds_byte_object_t*)data;
        if (NULL != boptr && NULL != boptr->bytes && 0 < boptr->size) {
            kv->data.bo.bytes = (uint8_t *) malloc(boptr->size);
            memcpy(kv->data.bo.bytes, boptr->bytes, boptr->size);
            kv->data.bo.size = boptr->size;
        } else {
            kv->data.bo.bytes = NULL;
            kv->data.bo.size = 0;
        }
        break;
    default:
        GDS_ERROR_LOG(GDS_ERR_NOT_SUPPORTED);
        return GDS_ERR_NOT_SUPPORTED;
    }

    return GDS_SUCCESS;
}

static int store_pointer(const gds_identifier_t *uid,
                         gds_value_t *kv)
{
    proc_data_t *proc_data;
    gds_value_t *k2;
    gds_identifier_t id;

    /* data must have an assigned scope */
    if (GDS_SCOPE_UNDEF == kv->scope) {
        return GDS_ERR_BAD_PARAM;
    }

    /* to protect alignment, copy the data across */
    memcpy(&id, uid, sizeof(gds_identifier_t));

    /* we are at the bottom of the store priorities, so
     * if this fell to us, we store it
     */
    gds_output_verbose(1, gds_gdstor_base_framework.framework_output,
                        "gdstor:hash:store storing data for proc %" PRIu64 " for scope %d",
                        id, (int)kv->scope);

    /* lookup the proc data object for this proc */
    if (NULL == (proc_data = lookup_gds_proc(&hash_data, id))) {
        /* unrecoverable error */
        GDS_OUTPUT_VERBOSE((5, gds_gdstor_base_framework.framework_output,
                             "gdstor:hash:store: storing key %s[%s] for proc %" PRIu64 " unrecoverably failed",
                             kv->key, gds_dss.lookup_data_type(kv->type), id));
        return GDS_ERR_OUT_OF_RESOURCE;
    }

    /* see if we already have this key in the data - means we are updating
     * a pre-existing value
     */
    k2 = lookup_keyval(proc_data, kv->key);
    GDS_OUTPUT_VERBOSE((5, gds_gdstor_base_framework.framework_output,
                         "gdstor:hash:store: %s pointer of key %s[%s] for proc %" PRIu64 "",
                         (NULL == k2 ? "storing" : "updating"),
                         kv->key, gds_dss.lookup_data_type(kv->type), id));
    if (NULL != k2) {
        gds_list_remove_item(&proc_data->data, &k2->super);
        OBJ_RELEASE(k2);
    }
    kv->scope |= GDS_SCOPE_REFER;  // mark that this value was stored by reference and doesn't belong to us
    gds_list_append(&proc_data->data, &kv->super);
    return GDS_SUCCESS;
}

static int fetch(const gds_identifier_t *uid,
                 const char *key, void **data,
                 gds_data_type_t type)
{
    proc_data_t *proc_data;
    gds_value_t *kv;
    gds_byte_object_t *boptr;
    gds_identifier_t id;

    /* to protect alignment, copy the data across */
    memcpy(&id, uid, sizeof(gds_identifier_t));

    GDS_OUTPUT_VERBOSE((5, gds_gdstor_base_framework.framework_output,
                         "gdstor:hash:fetch: searching for key %s[%s] on proc %" PRIu64 "",
                         (NULL == key) ? "NULL" : key,
                         gds_dss.lookup_data_type(type), id));

    /* if the key is NULL, that is an error */
    if (NULL == key) {
        GDS_ERROR_LOG(GDS_ERR_BAD_PARAM);
        return GDS_ERR_BAD_PARAM;
    }

    /* lookup the proc data object for this proc */
    if (NULL == (proc_data = lookup_gds_proc(&hash_data, id))) {
        /* maybe they can find it elsewhere */
        GDS_OUTPUT_VERBOSE((5, gds_gdstor_base_framework.framework_output,
                             "gdstor_hash:fetch data for proc %" PRIu64 " not found", id));
        return GDS_ERR_TAKE_NEXT_OPTION;
    }

    /* find the value */
    if (NULL == (kv = lookup_keyval(proc_data, key))) {
        /* let them look globally for it */
        GDS_OUTPUT_VERBOSE((5, gds_gdstor_base_framework.framework_output,
                             "gdstor_hash:fetch key %s for proc %" PRIu64 " not found",
                             (NULL == key) ? "NULL" : key, id));
        return GDS_ERR_TAKE_NEXT_OPTION;
    }

    /* do the copy and check the type */
    switch (type) {
    case GDS_STRING:
        if (GDS_STRING != kv->type) {
            return GDS_ERR_TYPE_MISMATCH;
        }
        if (NULL != kv->data.string) {
            *data = strdup(kv->data.string);
        } else {
            *data = NULL;
        }
        break;
    case GDS_UINT64:
        if (GDS_UINT64 != kv->type) {
            return GDS_ERR_TYPE_MISMATCH;
        }
        memcpy(*data, &kv->data.uint64, 8);
        break;
    case GDS_UINT32:
        if (GDS_UINT32 != kv->type) {
            return GDS_ERR_TYPE_MISMATCH;
        }
        memcpy(*data, &kv->data.uint32, 4);
        break;
    case GDS_UINT16:
        if (GDS_UINT16 != kv->type) {
            return GDS_ERR_TYPE_MISMATCH;
        }
        memcpy(*data, &kv->data.uint16, 2);
        break;
    case GDS_INT:
        if (GDS_INT != kv->type) {
            return GDS_ERR_TYPE_MISMATCH;
        }
        memcpy(*data, &kv->data.integer, sizeof(int));
        break;
    case GDS_UINT:
        if (GDS_UINT != kv->type) {
            return GDS_ERR_TYPE_MISMATCH;
        }
        memcpy(*data, &kv->data.uint, sizeof(unsigned int));
        break;
    case GDS_FLOAT:
        if (GDS_FLOAT != kv->type) {
            return GDS_ERR_TYPE_MISMATCH;
        }
        memcpy(*data, &kv->data.fval, sizeof(float));
        break;
    case GDS_BYTE_OBJECT:
        if (GDS_BYTE_OBJECT != kv->type) {
            return GDS_ERR_TYPE_MISMATCH;
        }
        boptr = (gds_byte_object_t*)malloc(sizeof(gds_byte_object_t));
        if (NULL != kv->data.bo.bytes && 0 < kv->data.bo.size) {
            boptr->bytes = (uint8_t *) malloc(kv->data.bo.size);
            memcpy(boptr->bytes, kv->data.bo.bytes, kv->data.bo.size);
            boptr->size = kv->data.bo.size;
        } else {
            boptr->bytes = NULL;
            boptr->size = 0;
        }
        *data = boptr;
        break;
    default:
        GDS_ERROR_LOG(GDS_ERR_NOT_SUPPORTED);
        return GDS_ERR_NOT_SUPPORTED;
    }

    return GDS_SUCCESS;
}

static int fetch_pointer(const gds_identifier_t *uid,
                         const char *key,
                         void **data, gds_data_type_t type)
{
    proc_data_t *proc_data;
    gds_value_t *kv;
    gds_identifier_t id;

    /* to protect alignment, copy the data across */
    memcpy(&id, uid, sizeof(gds_identifier_t));

    GDS_OUTPUT_VERBOSE((5, gds_gdstor_base_framework.framework_output,
                         "gdstor:hash:fetch_pointer: searching for key %s on proc %" PRIu64 "",
                         (NULL == key) ? "NULL" : key, id));

    /* if the key is NULL, that is an error */
    if (NULL == key) {
        GDS_ERROR_LOG(GDS_ERR_BAD_PARAM);
        return GDS_ERR_BAD_PARAM;
    }

    /* lookup the proc data object for this proc */
    if (NULL == (proc_data = lookup_gds_proc(&hash_data, id))) {
        /* look elsewhere */
        return GDS_ERR_TAKE_NEXT_OPTION;
    }

    /* find the value */
    if (NULL == (kv = lookup_keyval(proc_data, key))) {
        /* let them look globally for it */
        return GDS_ERR_TAKE_NEXT_OPTION;
    }

   switch (type) {
    case GDS_STRING:
        if (GDS_STRING != kv->type) {
            return GDS_ERR_TYPE_MISMATCH;
        }
        *data = kv->data.string;
        break;
    case GDS_UINT64:
        if (GDS_UINT64 != kv->type) {
            return GDS_ERR_TYPE_MISMATCH;
        }
        *data = &kv->data.uint64;
        break;
    case GDS_UINT32:
        if (GDS_UINT32 != kv->type) {
            return GDS_ERR_TYPE_MISMATCH;
        }
        *data = &kv->data.uint32;
        break;
    case GDS_UINT16:
        if (GDS_UINT16 != kv->type) {
            return GDS_ERR_TYPE_MISMATCH;
        }
        *data = &kv->data.uint16;
        break;
    case GDS_INT:
        if (GDS_INT != kv->type) {
            return GDS_ERR_TYPE_MISMATCH;
        }
        *data = &kv->data.integer;
        break;
    case GDS_UINT:
        if (GDS_UINT != kv->type) {
            return GDS_ERR_TYPE_MISMATCH;
        }
        *data = &kv->data.uint;
        break;
    case GDS_BYTE_OBJECT:
        if (GDS_BYTE_OBJECT != kv->type) {
            return GDS_ERR_TYPE_MISMATCH;
        }
        *data = &kv->data.bo;
        break;
    case GDS_FLOAT:
        if (GDS_FLOAT != kv->type) {
            return GDS_ERR_TYPE_MISMATCH;
        }
        *data = &kv->data.fval;
        break;
    default:
        GDS_ERROR_LOG(GDS_ERR_NOT_SUPPORTED);
        return GDS_ERR_NOT_SUPPORTED;
    }

    return GDS_SUCCESS;
}

static int fetch_multiple(const gds_identifier_t *uid,
                          gds_scope_t scope,
                          const char *key,
                          gds_list_t *kvs)
{
    proc_data_t *proc_data;
    gds_value_t *kv, *kvnew;
    int rc;
    char *srchkey, *ptr;
    size_t len = 0;
    gds_identifier_t id;

    /* to protect alignment, copy the data across */
    memcpy(&id, uid, sizeof(gds_identifier_t));

    GDS_OUTPUT_VERBOSE((5, gds_gdstor_base_framework.framework_output,
                         "gdstor:hash:fetch_multiple: searching for key %s on proc %" PRIu64 "",
                         (NULL == key) ? "NULL" : key, id));

    /* lookup the proc data object for this proc */
    if (NULL == (proc_data = lookup_gds_proc(&hash_data, id))) {
        /* look elsewhere */
        return GDS_ERR_TAKE_NEXT_OPTION;
    }

    /* if the key is NULL, then return all the values */
    if (NULL == key) {
        for (kv = (gds_value_t*) gds_list_get_first(&proc_data->data);
             kv != (gds_value_t*) gds_list_get_end(&proc_data->data);
             kv = (gds_value_t*) gds_list_get_next(kv)) {
            /* check for a matching scope */
            if (!(scope & kv->scope)) {
                continue;
            }
            if (GDS_SUCCESS != (rc = gds_dss.copy((void**)&kvnew, kv, GDS_VALUE))) {
                GDS_ERROR_LOG(rc);
                return rc;
            }
            gds_list_append(kvs, &kvnew->super);
        }
        return GDS_SUCCESS;
    }

    /* see if the key includes a wildcard */
    srchkey = strdup(key);
    if (NULL != (ptr = strchr(srchkey, '*'))) {
        *ptr = '\0';
        len = strlen(srchkey);
    }

    /* otherwise, find all matching keys and return them */
    for (kv = (gds_value_t*) gds_list_get_first(&proc_data->data);
         kv != (gds_value_t*) gds_list_get_end(&proc_data->data);
         kv = (gds_value_t*) gds_list_get_next(kv)) {
        /* check for a matching scope */
        if (!(scope & kv->scope)) {
            continue;
        }
        if ((0 < len && 0 == strncmp(srchkey, kv->key, len)) ||
            (0 == len && 0 == strcmp(key, kv->key))) {
            if (GDS_SUCCESS != (rc = gds_dss.copy((void**)&kvnew, kv, GDS_VALUE))) {
                GDS_ERROR_LOG(rc);
                free(srchkey);
                return rc;
            }
            gds_list_append(kvs, &kvnew->super);
        }
    }
    free(srchkey);
    return GDS_SUCCESS;
}

static int remove_data(const gds_identifier_t *uid, const char *key)
{
    proc_data_t *proc_data;
    gds_value_t *kv;
    gds_identifier_t id;

    /* to protect alignment, copy the data across */
    memcpy(&id, uid, sizeof(gds_identifier_t));

    /* lookup the specified proc */
    if (NULL == (proc_data = lookup_gds_proc(&hash_data, id))) {
        /* no data for this proc */
        return GDS_SUCCESS;
    }

    /* if key is NULL, remove all data for this proc */
    if (NULL == key) {
        while (NULL != (kv = (gds_value_t *) gds_list_remove_first(&proc_data->data))) {
            OBJ_RELEASE(kv);
        }
        /* remove the proc_data object itself from the jtable */
        gds_hash_table_remove_value_uint64(&hash_data, id);
        /* cleanup */
        OBJ_RELEASE(proc_data);
        return GDS_SUCCESS;
    }

    /* remove this item */
    for (kv = (gds_value_t*) gds_list_get_first(&proc_data->data);
         kv != (gds_value_t*) gds_list_get_end(&proc_data->data);
         kv = (gds_value_t*) gds_list_get_next(kv)) {
        if (0 == strcmp(key, kv->key)) {
            gds_list_remove_item(&proc_data->data, &kv->super);
            if (!(kv->scope & GDS_SCOPE_REFER)) {
                OBJ_RELEASE(kv);
            }
            break;
        }
    }

    return GDS_SUCCESS;
}

