/*
 * Copyright (c) 2014-2016 Intel, Inc.  All rights reserved.
 * Copyright (c) 2015      Cisco Systems, Inc.  All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#ifndef GDS_PROGRESS_THREADS_H
#define GDS_PROGRESS_THREADS_H

#include "gds_config.h"

/**
 * Initialize a progress thread name; if a progress thread is not
 * already associated with that name, start a progress thread.
 *
 * If you have general events that need to run in *a* progress thread
 * (but not necessarily a your own, dedicated progress thread), pass
 * NULL the "name" argument to the gds_progress_thead_init() function
 * to glom on to the general GDS-wide progress thread.
 *
 * If a name is passed that was already used in a prior call to
 * gds_progress_thread_init(), the event base associated with that
 * already-running progress thread will be returned (i.e., no new
 * progress thread will be started).
 */
gds_event_base_t *gds_progress_thread_init(const char *name);

/**
 * Finalize a progress thread name (reference counted).
 *
 * Once this function is invoked as many times as
 * gds_progress_thread_init() was invoked on this name (or NULL), the
 * progress function is shut down and the event base associated with
 * it is destroyed.
 *
 * Will return GDS_ERR_NOT_FOUND if the progress thread name does not
 * exist; GDS_SUCCESS otherwise.
 */
int gds_progress_thread_finalize(const char *name);

/**
 * Temporarily pause the progress thread associated with this name.
 *
 * This function does not destroy the event base associated with this
 * progress thread name, but it does stop processing all events on
 * that event base until gds_progress_thread_resume() is invoked on
 * that name.
 *
 * Will return GDS_ERR_NOT_FOUND if the progress thread name does not
 * exist; GDS_SUCCESS otherwise.
 */
int gds_progress_thread_pause(const char *name);

/**
 * Restart a previously-paused progress thread associated with this
 * name.
 *
 * Will return GDS_ERR_NOT_FOUND if the progress thread name does not
 * exist; GDS_SUCCESS otherwise.
 */
int gds_progress_thread_resume(const char *name);

#endif
