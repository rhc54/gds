/*
 * Copyright (c) 2014-2016 Intel, Inc.  All rights reserved.
 * Copyright (c) 2015      Cisco Systems, Inc.  All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include <src/include/gds_config.h>
#include "src/include/types.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <string.h>
#include <pthread.h>
#include GDS_EVENT_HEADER

#include "src/class/gds_list.h"
#include "src/util/error.h"
#include "src/util/fd.h"

#include "src/runtime/gds_progress_threads.h"

/* define a thread object */
#define GDS_THREAD_CANCELLED   ((void*)1);
typedef void *(*gds_thread_fn_t) (gds_object_t *);

typedef struct gds_thread_t {
    gds_object_t super;
    gds_thread_fn_t t_run;
    void* t_arg;
    pthread_t t_handle;
} gds_thread_t;
static void ptcon(gds_thread_t *p)
{
    p->t_arg = NULL;
    p->t_handle = (pthread_t) -1;
}
GDS_CLASS_INSTANCE(gds_thread_t,
                  gds_object_t,
                  ptcon, NULL);

static int gds_thread_start(gds_thread_t *t)
{
    int rc;

    if (GDS_ENABLE_DEBUG) {
        if (NULL == t->t_run || t->t_handle != (pthread_t) -1) {
            return GDS_ERR_BAD_PARAM;
        }
    }

    rc = pthread_create(&t->t_handle, NULL, (void*(*)(void*)) t->t_run, t);

    return (rc == 0) ? GDS_SUCCESS : GDS_ERROR;
}


static int gds_thread_join(gds_thread_t *t, void **thr_return)
{
    int rc = pthread_join(t->t_handle, thr_return);
    t->t_handle = (pthread_t) -1;
    return (rc == 0) ? GDS_SUCCESS : GDS_ERROR;
}


/* create a tracking object for progress threads */
typedef struct {
    gds_list_item_t super;

    int refcount;
    char *name;

    gds_event_base_t *ev_base;

    /* This will be set to false when it is time for the progress
       thread to exit */
    volatile bool ev_active;

    /* This event will always be set on the ev_base (so that the
       ev_base is not empty!) */
    gds_event_t block;

    bool engine_constructed;
    gds_thread_t engine;
} gds_progress_tracker_t;

static void tracker_constructor(gds_progress_tracker_t *p)
{
    p->refcount = 1;  // start at one since someone created it
    p->name = NULL;
    p->ev_base = NULL;
    p->ev_active = false;
    p->engine_constructed = false;
}

static void tracker_destructor(gds_progress_tracker_t *p)
{
    gds_event_del(&p->block);

    if (NULL != p->name) {
        free(p->name);
    }
    if (NULL != p->ev_base) {
        gds_event_base_free(p->ev_base);
    }
    if (p->engine_constructed) {
        GDS_DESTRUCT(&p->engine);
    }
}

static GDS_CLASS_INSTANCE(gds_progress_tracker_t,
                          gds_list_item_t,
                          tracker_constructor,
                          tracker_destructor);

static bool inited = false;
static gds_list_t tracking;
static struct timeval long_timeout = {
    .tv_sec = 3600,
    .tv_usec = 0
};
static const char *shared_thread_name = "GDS-wide async progress thread";

/*
 * If this event is fired, just restart it so that this event base
 * continues to have something to block on.
 */
static void dummy_timeout_cb(int fd, short args, void *cbdata)
{
    gds_progress_tracker_t *trk = (gds_progress_tracker_t*)cbdata;

    gds_event_add(&trk->block, &long_timeout);
}

/*
 * Main for the progress thread
 */
static void* progress_engine(gds_object_t *obj)
{
    gds_thread_t *t = (gds_thread_t*)obj;
    gds_progress_tracker_t *trk = (gds_progress_tracker_t*)t->t_arg;

    while (trk->ev_active) {
        gds_event_loop(trk->ev_base, GDS_EVLOOP_ONCE);
    }

    return GDS_THREAD_CANCELLED;
}

static void stop_progress_engine(gds_progress_tracker_t *trk)
{
    assert(trk->ev_active);
    trk->ev_active = false;

    /* break the event loop - this will cause the loop to exit upon
       completion of any current event */
    gds_event_base_loopbreak(trk->ev_base);

    gds_thread_join(&trk->engine, NULL);
}

static int start_progress_engine(gds_progress_tracker_t *trk)
{
    assert(!trk->ev_active);
    trk->ev_active = true;

    /* fork off a thread to progress it */
    trk->engine.t_run = progress_engine;
    trk->engine.t_arg = trk;

    int rc = gds_thread_start(&trk->engine);
    if (GDS_SUCCESS != rc) {
        GDS_ERROR_LOG(rc);
    }

    return rc;
}

gds_event_base_t *gds_progress_thread_init(const char *name)
{
    gds_progress_tracker_t *trk;
    int rc;

    if (!inited) {
        GDS_CONSTRUCT(&tracking, gds_list_t);
        inited = true;
    }

    if (NULL == name) {
        name = shared_thread_name;
    }

    /* check if we already have this thread */
    GDS_LIST_FOREACH(trk, &tracking, gds_progress_tracker_t) {
        if (0 == strcmp(name, trk->name)) {
            /* we do, so up the refcount on it */
            ++trk->refcount;
            /* return the existing base */
            return trk->ev_base;
        }
    }

    trk = GDS_NEW(gds_progress_tracker_t);
    if (NULL == trk) {
        GDS_ERROR_LOG(GDS_ERR_OUT_OF_RESOURCE);
        return NULL;
    }

    trk->name = strdup(name);
    if (NULL == trk->name) {
        GDS_ERROR_LOG(GDS_ERR_OUT_OF_RESOURCE);
        GDS_RELEASE(trk);
        return NULL;
    }

    if (NULL == (trk->ev_base = gds_event_base_create())) {
        GDS_ERROR_LOG(GDS_ERR_OUT_OF_RESOURCE);
        GDS_RELEASE(trk);
        return NULL;
    }

    /* add an event to the new event base (if there are no events,
       gds_event_loop() will return immediately) */
    gds_event_set(trk->ev_base, &trk->block, -1, GDS_EV_PERSIST,
                   dummy_timeout_cb, trk);
    gds_event_add(&trk->block, &long_timeout);

    /* construct the thread object */
    GDS_CONSTRUCT(&trk->engine, gds_thread_t);
    trk->engine_constructed = true;
    if (GDS_SUCCESS != (rc = start_progress_engine(trk))) {
        GDS_ERROR_LOG(rc);
        GDS_RELEASE(trk);
        return NULL;
    }
    gds_list_append(&tracking, &trk->super);

    return trk->ev_base;
}

int gds_progress_thread_finalize(const char *name)
{
    gds_progress_tracker_t *trk;

    if (!inited) {
        /* nothing we can do */
        return GDS_ERR_NOT_FOUND;
    }

    if (NULL == name) {
        name = shared_thread_name;
    }

    /* find the specified engine */
    GDS_LIST_FOREACH(trk, &tracking, gds_progress_tracker_t) {
        if (0 == strcmp(name, trk->name)) {
            /* decrement the refcount */
            --trk->refcount;

            /* If the refcount is still above 0, we're done here */
            if (trk->refcount > 0) {
                return GDS_SUCCESS;
            }

            /* If the progress thread is active, stop it */
            if (trk->ev_active) {
                stop_progress_engine(trk);
            }

            gds_list_remove_item(&tracking, &trk->super);
            GDS_RELEASE(trk);
            return GDS_SUCCESS;
        }
    }

    return GDS_ERR_NOT_FOUND;
}

/*
 * Stop the progress thread, but don't delete the tracker (or event base)
 */
int gds_progress_thread_pause(const char *name)
{
    gds_progress_tracker_t *trk;

    if (!inited) {
        /* nothing we can do */
        return GDS_ERR_NOT_FOUND;
    }

    if (NULL == name) {
        name = shared_thread_name;
    }

    /* find the specified engine */
    GDS_LIST_FOREACH(trk, &tracking, gds_progress_tracker_t) {
        if (0 == strcmp(name, trk->name)) {
            if (trk->ev_active) {
                stop_progress_engine(trk);
            }

            return GDS_SUCCESS;
        }
    }

    return GDS_ERR_NOT_FOUND;
}

int gds_progress_thread_resume(const char *name)
{
    gds_progress_tracker_t *trk;

    if (!inited) {
        /* nothing we can do */
        return GDS_ERR_NOT_FOUND;
    }

    if (NULL == name) {
        name = shared_thread_name;
    }

    /* find the specified engine */
    GDS_LIST_FOREACH(trk, &tracking, gds_progress_tracker_t) {
        if (0 == strcmp(name, trk->name)) {
            if (trk->ev_active) {
                return GDS_ERR_RESOURCE_BUSY;
            }

            return start_progress_engine(trk);
        }
    }

    return GDS_ERR_NOT_FOUND;
}
