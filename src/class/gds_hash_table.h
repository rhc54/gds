/*
 * Copyright (c) 2004-2007 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2005 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2015-2016 Intel, Inc. All rights reserved.
 * Copyright (c) 2015      Research Organization for Information Science
 *                         and Technology (RIST). All rights reserved.
 * Copyright (c) 2016      Mellanox Technologies, Inc.
 *                         All rights reserved.
 *
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 *
 */

/** @file
 *
 *  A hash table that may be indexed with either fixed length
 *  (e.g. uint32_t/uint64_t) or arbitrary size binary key
 *  values. However, only one key type may be used in a given table
 *  concurrently.
 */

#ifndef GDS_HASH_TABLE_H
#define GDS_HASH_TABLE_H

#include <src/include/gds_config.h>
#include <src/include/prefetch.h>

#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

#include "src/class/gds_list.h"

#include <gds_common.h>

BEGIN_C_DECLS

GDS_CLASS_DECLARATION(gds_hash_table_t);

struct gds_hash_table_t
{
    gds_object_t        super;          /**< subclass of gds_object_t */
    struct gds_hash_element_t * ht_table;       /**< table of elements (opaque to users) */
    size_t               ht_capacity;    /**< allocated size (capacity) of table */
    size_t               ht_size;        /**< number of extant entries */
    size_t               ht_growth_trigger; /**< size hits this and table is grown  */
    int                  ht_density_numer, ht_density_denom; /**< max allowed density of table */
    int                  ht_growth_numer, ht_growth_denom;   /**< growth factor when grown  */
    const struct gds_hash_type_methods_t * ht_type_methods;
};
typedef struct gds_hash_table_t gds_hash_table_t;



/**
 *  Initializes the table size, must be called before using
 *  the table.
 *
 *  @param   table   The input hash table (IN).
 *  @param   size    The size of the table, which will be rounded up
 *                   (if required) to the next highest power of two (IN).
 *  @return  GDS error code.
 *
 */

int gds_hash_table_init(gds_hash_table_t* ht, size_t table_size);

/* this could be the new init if people wanted a more general API */
int gds_hash_table_init2(gds_hash_table_t* ht, size_t estimated_max_size,
                                        int density_numer, int density_denom,
                                        int growth_numer, int growth_denom);

/**
 *  Returns the number of elements currently stored in the table.
 *
 *  @param   table   The input hash table (IN).
 *  @return  The number of elements in the table.
 *
 */

static inline size_t gds_hash_table_get_size(gds_hash_table_t *ht)
{
    return ht->ht_size;
}

/**
 *  Remove all elements from the table.
 *
 *  @param   table   The input hash table (IN).
 *  @return  GDS return code.
 *
 */

int gds_hash_table_remove_all(gds_hash_table_t *ht);

/**
 *  Retrieve value via uint32_t key.
 *
 *  @param   table   The input hash table (IN).
 *  @param   key     The input key (IN).
 *  @param   ptr     The value associated with the key
 *  @return  integer return code:
 *           - GDS_SUCCESS       if key was found
 *           - GDS_ERR_NOT_FOUND if key was not found
 *           - GDS_ERROR         other error
 *
 */

int gds_hash_table_get_value_uint32(gds_hash_table_t* table, uint32_t key,
                                                   void** ptr);

/**
 *  Set value based on uint32_t key.
 *
 *  @param   table   The input hash table (IN).
 *  @param   key     The input key (IN).
 *  @param   value   The value to be associated with the key (IN).
 *  @return  GDS return code.
 *
 */

int gds_hash_table_set_value_uint32(gds_hash_table_t* table, uint32_t key, void* value);

/**
 *  Remove value based on uint32_t key.
 *
 *  @param   table   The input hash table (IN).
 *  @param   key     The input key (IN).
 *  @return  GDS return code.
 *
 */

int gds_hash_table_remove_value_uint32(gds_hash_table_t* table, uint32_t key);

/**
 *  Retrieve value via uint64_t key.
 *
 *  @param   table   The input hash table (IN).
 *  @param   key     The input key (IN).
 *  @param   ptr     The value associated with the key
 *  @return  integer return code:
 *           - GDS_SUCCESS       if key was found
 *           - GDS_ERR_NOT_FOUND if key was not found
 *           - GDS_ERROR         other error
 *
 */

int gds_hash_table_get_value_uint64(gds_hash_table_t *table, uint64_t key,
                                                   void **ptr);

/**
 *  Set value based on uint64_t key.
 *
 *  @param   table   The input hash table (IN).
 *  @param   key     The input key (IN).
 *  @param   value   The value to be associated with the key (IN).
 *  @return  GDS return code.
 *
 */

int gds_hash_table_set_value_uint64(gds_hash_table_t *table, uint64_t key, void* value);

/**
 *  Remove value based on uint64_t key.
 *
 *  @param   table   The input hash table (IN).
 *  @param   key     The input key (IN).
 *  @return  GDS return code.
 *
 */

int gds_hash_table_remove_value_uint64(gds_hash_table_t *table, uint64_t key);

/**
 *  Retrieve value via arbitrary length binary key.
 *
 *  @param   table   The input hash table (IN).
 *  @param   key     The input key (IN).
 *  @param   ptr     The value associated with the key
 *  @return  integer return code:
 *           - GDS_SUCCESS       if key was found
 *           - GDS_ERR_NOT_FOUND if key was not found
 *           - GDS_ERROR         other error
 *
 */

int gds_hash_table_get_value_ptr(gds_hash_table_t *table, const void* key,
                                                size_t keylen, void **ptr);

/**
 *  Set value based on arbitrary length binary key.
 *
 *  @param   table   The input hash table (IN).
 *  @param   key     The input key (IN).
 *  @param   value   The value to be associated with the key (IN).
 *  @return  GDS return code.
 *
 */

int gds_hash_table_set_value_ptr(gds_hash_table_t *table, const void* key, size_t keylen, void* value);

/**
 *  Remove value based on arbitrary length binary key.
 *
 *  @param   table   The input hash table (IN).
 *  @param   key     The input key (IN).
 *  @return  GDS return code.
 *
 */

int gds_hash_table_remove_value_ptr(gds_hash_table_t *table, const void* key, size_t keylen);


/** The following functions are only for allowing iterating through
    the hash table. The calls return along with a key, a pointer to
    the hash node with the current key, so that subsequent calls do
    not have to traverse all over again to the key (although it may
    just be a simple thing - to go to the array element and then
    traverse through the individual list). But lets take out this
    inefficiency too. This is similar to having an STL iterator in
    functionality */

/**
 *  Get the first 32 bit key from the hash table, which can be used later to
 *  get the next key
 *  @param  table   The hash table pointer (IN)
 *  @param  key     The first key (OUT)
 *  @param  value   The value corresponding to this key (OUT)
 *  @param  node    The pointer to the hash table internal node which stores
 *                  the key-value pair (this is required for subsequent calls
 *                  to get_next_key) (OUT)
 *  @return GDS error code
 *
 */

int gds_hash_table_get_first_key_uint32(gds_hash_table_t *table, uint32_t *key,
                                        void **value, void **node);


/**
 *  Get the next 32 bit key from the hash table, knowing the current key
 *  @param  table    The hash table pointer (IN)
 *  @param  key      The key (OUT)
 *  @param  value    The value corresponding to this key (OUT)
 *  @param  in_node  The node pointer from previous call to either get_first
                     or get_next (IN)
 *  @param  out_node The pointer to the hash table internal node which stores
 *                   the key-value pair (this is required for subsequent calls
 *                   to get_next_key) (OUT)
 *  @return GDS error code
 *
 */

int gds_hash_table_get_next_key_uint32(gds_hash_table_t *table, uint32_t *key,
                                       void **value, void *in_node,
                                       void **out_node);


/**
 *  Get the first 64 key from the hash table, which can be used later to
 *  get the next key
 *  @param  table   The hash table pointer (IN)
 *  @param  key     The first key (OUT)
 *  @param  value   The value corresponding to this key (OUT)
 *  @param  node    The pointer to the hash table internal node which stores
 *                  the key-value pair (this is required for subsequent calls
 *                  to get_next_key) (OUT)
 *  @return GDS error code
 *
 */

int gds_hash_table_get_first_key_uint64(gds_hash_table_t *table, uint64_t *key,
                                       void **value, void **node);


/**
 *  Get the next 64 bit key from the hash table, knowing the current key
 *  @param  table    The hash table pointer (IN)
 *  @param  key      The key (OUT)
 *  @param  value    The value corresponding to this key (OUT)
 *  @param  in_node  The node pointer from previous call to either get_first
                     or get_next (IN)
 *  @param  out_node The pointer to the hash table internal node which stores
 *                   the key-value pair (this is required for subsequent calls
 *                   to get_next_key) (OUT)
 *  @return GDS error code
 *
 */

int gds_hash_table_get_next_key_uint64(gds_hash_table_t *table, uint64_t *key,
                                       void **value, void *in_node,
                                       void **out_node);


/**
 *  Get the first ptr bit key from the hash table, which can be used later to
 *  get the next key
 *  @param  table    The hash table pointer (IN)
 *  @param  key      The first key (OUT)
 *  @param  key_size The first key size (OUT)
 *  @param  value    The value corresponding to this key (OUT)
 *  @param  node     The pointer to the hash table internal node which stores
 *                   the key-value pair (this is required for subsequent calls
 *                   to get_next_key) (OUT)
 *  @return GDS error code
 *
 */

int gds_hash_table_get_first_key_ptr(gds_hash_table_t *table, void* *key,
                                        size_t *key_size, void **value, void **node);


/**
 *  Get the next ptr bit key from the hash table, knowing the current key
 *  @param  table    The hash table pointer (IN)
 *  @param  key      The key (OUT)
 *  @param  key_size The key size (OUT)
 *  @param  value    The value corresponding to this key (OUT)
 *  @param  in_node  The node pointer from previous call to either get_first
                     or get_next (IN)
 *  @param  out_node The pointer to the hash table internal node which stores
 *                   the key-value pair (this is required for subsequent calls
 *                   to get_next_key) (OUT)
 *  @return GDS error code
 *
 */

int gds_hash_table_get_next_key_ptr(gds_hash_table_t *table, void* *key,
                                       size_t *key_size, void **value,
                                       void *in_node, void **out_node);


/**
 * @brief Returns next power-of-two of the given value.
 *
 * @param value The integer value to return power of 2
 *
 * @returns The next power of two
 *
 * WARNING: *NO* error checking is performed.  This is meant to be a
 * fast inline function.
 * Using __builtin_clz (count-leading-zeros) uses 4 cycles instead of 77
 * compared to the loop-version (on Intel Nehalem -- with icc-12.1.0 -O2).
 */
static inline int gds_next_poweroftwo(int value)
{
    int power2;

#if GDS_C_HAVE_BUILTIN_CLZ
    if (GDS_UNLIKELY (0 == value)) {
        return 1;
    }
    power2 = 1 << (8 * sizeof (int) - __builtin_clz(value));
#else
    for (power2 = 1; value > 0; value >>= 1, power2 <<= 1) /* empty */;
#endif

    return power2;
}


END_C_DECLS

#endif  /* GDS_HASH_TABLE_H */
