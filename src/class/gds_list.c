/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 * Copyright (c) 2004-2005 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2006 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2007 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2007      Voltaire All rights reserved.
 * Copyright (c) 2013-2015 Intel, Inc. All rights reserved
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include <src/include/gds_config.h>

#include "src/class/gds_list.h"
#include <gds.h>

/*
 *  List classes
 */

static void gds_list_item_construct(gds_list_item_t*);
static void gds_list_item_destruct(gds_list_item_t*);

GDS_CLASS_INSTANCE(
    gds_list_item_t,
    gds_object_t,
    gds_list_item_construct,
    gds_list_item_destruct
);

static void gds_list_construct(gds_list_t*);
static void gds_list_destruct(gds_list_t*);

GDS_CLASS_INSTANCE(
    gds_list_t,
    gds_object_t,
    gds_list_construct,
    gds_list_destruct
);


/*
 *
 *      gds_list_link_item_t interface
 *
 */

static void gds_list_item_construct(gds_list_item_t *item)
{
    item->gds_list_next = item->gds_list_prev = NULL;
    item->item_free = 1;
#if GDS_ENABLE_DEBUG
    item->gds_list_item_refcount = 0;
    item->gds_list_item_belong_to = NULL;
#endif
}

static void gds_list_item_destruct(gds_list_item_t *item)
{
#if GDS_ENABLE_DEBUG
    assert( 0 == item->gds_list_item_refcount );
    assert( NULL == item->gds_list_item_belong_to );
#endif  /* GDS_ENABLE_DEBUG */
}


/*
 *
 *      gds_list_list_t interface
 *
 */

static void gds_list_construct(gds_list_t *list)
{
#if GDS_ENABLE_DEBUG
    /* These refcounts should never be used in assertions because they
       should never be removed from this list, added to another list,
       etc.  So set them to sentinel values. */

    GDS_CONSTRUCT( &(list->gds_list_sentinel), gds_list_item_t );
    list->gds_list_sentinel.gds_list_item_refcount  = 1;
    list->gds_list_sentinel.gds_list_item_belong_to = list;
#endif

    list->gds_list_sentinel.gds_list_next = &list->gds_list_sentinel;
    list->gds_list_sentinel.gds_list_prev = &list->gds_list_sentinel;
    list->gds_list_length = 0;
}


/*
 * Reset all the pointers to be NULL -- do not actually destroy
 * anything.
 */
static void gds_list_destruct(gds_list_t *list)
{
    gds_list_construct(list);
}


/*
 * Insert an item at a specific place in a list
 */
bool gds_list_insert(gds_list_t *list, gds_list_item_t *item, long long idx)
{
    /* Adds item to list at index and retains item. */
    int     i;
    volatile gds_list_item_t *ptr, *next;

    if ( idx >= (long long)list->gds_list_length ) {
        return false;
    }

    if ( 0 == idx )
    {
        gds_list_prepend(list, item);
    } else {
#if GDS_ENABLE_DEBUG
        /* Spot check: ensure that this item is previously on no
           lists */

        assert(0 == item->gds_list_item_refcount);
#endif
        /* pointer to element 0 */
        ptr = list->gds_list_sentinel.gds_list_next;
        for ( i = 0; i < idx-1; i++ )
            ptr = ptr->gds_list_next;

        next = ptr->gds_list_next;
        item->gds_list_next = next;
        item->gds_list_prev = ptr;
        next->gds_list_prev = item;
        ptr->gds_list_next = item;

#if GDS_ENABLE_DEBUG
        /* Spot check: ensure this item is only on the list that we
           just insertted it into */

        item->gds_list_item_refcount += 1;
        assert(1 == item->gds_list_item_refcount);
        item->gds_list_item_belong_to = list;
#endif
    }

    list->gds_list_length++;
    return true;
}


static
void
gds_list_transfer(gds_list_item_t *pos, gds_list_item_t *begin,
                   gds_list_item_t *end)
{
    volatile gds_list_item_t *tmp;

    if (pos != end) {
        /* remove [begin, end) */
        end->gds_list_prev->gds_list_next = pos;
        begin->gds_list_prev->gds_list_next = end;
        pos->gds_list_prev->gds_list_next = begin;

        /* splice into new position before pos */
        tmp = pos->gds_list_prev;
        pos->gds_list_prev = end->gds_list_prev;
        end->gds_list_prev = begin->gds_list_prev;
        begin->gds_list_prev = tmp;
#if GDS_ENABLE_DEBUG
        {
            volatile gds_list_item_t* item = begin;
            while( pos != item ) {
                item->gds_list_item_belong_to = pos->gds_list_item_belong_to;
                item = item->gds_list_next;
                assert(NULL != item);
            }
        }
#endif  /* GDS_ENABLE_DEBUG */
    }
}


void
gds_list_join(gds_list_t *thislist, gds_list_item_t *pos,
               gds_list_t *xlist)
{
    if (0 != gds_list_get_size(xlist)) {
        gds_list_transfer(pos, gds_list_get_first(xlist),
                           gds_list_get_end(xlist));

        /* fix the sizes */
        thislist->gds_list_length += xlist->gds_list_length;
        xlist->gds_list_length = 0;
    }
}


void
gds_list_splice(gds_list_t *thislist, gds_list_item_t *pos,
                 gds_list_t *xlist, gds_list_item_t *first,
                 gds_list_item_t *last)
{
    size_t change = 0;
    gds_list_item_t *tmp;

    if (first != last) {
        /* figure out how many things we are going to move (have to do
         * first, since last might be end and then we wouldn't be able
         * to run the loop)
         */
        for (tmp = first ; tmp != last ; tmp = gds_list_get_next(tmp)) {
            change++;
        }

        gds_list_transfer(pos, first, last);

        /* fix the sizes */
        thislist->gds_list_length += change;
        xlist->gds_list_length -= change;
    }
}


int gds_list_sort(gds_list_t* list, gds_list_item_compare_fn_t compare)
{
    gds_list_item_t* item;
    gds_list_item_t** items;
    size_t i, index=0;

    if (0 == list->gds_list_length) {
        return GDS_SUCCESS;
    }
    items = (gds_list_item_t**)malloc(sizeof(gds_list_item_t*) *
                                       list->gds_list_length);

    if (NULL == items) {
        return GDS_ERR_OUT_OF_RESOURCE;
    }

    while(NULL != (item = gds_list_remove_first(list))) {
        items[index++] = item;
    }

    qsort(items, index, sizeof(gds_list_item_t*),
          (int(*)(const void*,const void*))compare);
    for (i=0; i<index; i++) {
        gds_list_append(list,items[i]);
    }
    free(items);
    return GDS_SUCCESS;
}
