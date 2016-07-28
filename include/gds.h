/* -*- C -*-
 *
 * Copyright (c) 2016      Intel, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer listed
 *   in this license in the documentation and/or other materials
 *   provided with the distribution.
 *
 * - Neither the name of the copyright holders nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * The copyright holders provide no reassurances that the source code
 * provided does not infringe any patent, copyright, or any other
 * intellectual property rights of third parties.  The copyright holders
 * disclaim any liability to any recipient for claims brought against
 * recipient by any third party for infringement of that parties
 * intellectual property rights.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */
/**
 * @file
 *
 * Generalized Data Store
 */

#ifndef GDS_H
#define GDS_H

/*********************************************************************
 * The Generalized Data Store library provides an abstraction layer
 * for accessing multiple and diverse data storage implementations.
 * The objective is to provide:
 *
 * - a platform upon which organizations can build applications,
 *   system management tools, and other subsystems without being
 *   totally dependent on a specific GDS implementation; and
 *
 * - a lightweight plugin mechanism by which researchers and implementers
 *   can make their data store implementations available to the
 *   user community without requiring modification of the user's code. This
 *   is also intended to foster greater experimentation in data store
 *   technology by enabling easy testing of new implementations in
 *   production environments.
 *
 * A given "data store" consists of three elements:
 *
 * - a GDS plugin that implements the GDS APIs, translating data and
 *   directives as required
 *
 * - a backend data storage library that actually stores the data. Note
 *   that implementers are welcome to implement their data storage directly
 *   in the GDS library if they so choose, subject to the GDS license and
 *   signing of the appropriate contributor's agreement
 *
 * - a storage "element" within that library to which the caller will
 *   store and/or fetch objects
 *
 * This can be best illustrated via an example. For simplicity, we limit
 * the case to that of a structured SQL-based data store. A user may
 * request a datastore handle by calling the GDS_Attach function. In this
 * case, they can request that they be attached at several levels:
 *
 * - to the SQL data store plugin, without specifying a particular database
 *   or table. Subsequent operations using the handle could then create a
 *   database, write/fetch to an existing database serviced by this plugin
 *   by specifying a query that includes the database and other required
 *   target information, and/or perform various administrative functions.
 *
 * - to a particular SQL database. The active plugins would by queried in
 *   priority order to determine which, if any, could access the specified
 *   database. The first capable of doing so would return the datastore handle,
 *   and subsequent operations using that handle would need to specify the table(s)
 *   they are to be conducted against
 *
 * - to a specific table within a particular SQL database. The plugin query
 *   would return a handle from the highest-priority plugin that can access
 *   the specified database/table combination. Queries in this case would
 *   only need to provide information identifying the column/row elements being
 *   operated upon.
 *
 * The GDS datastore handle contains both metadata about the datastore, and
 * a set of function pointers by which a caller can fetch, store, and delete
 * data objects.
 *
 * General GDS design guidelines include:
 *
 * - Due to the focus on high-performance computing environments, performance
 *   and thread-support are considered basic requirements of the GDS
 *   library. The GDS library infrastructure is therefore designed to be
 *   lightweight and very thin, and all APIs are defined as non-blocking
 *   (except for init/finalize).
 *
 * - APIs are focused on providing flexibility so that future data store
 *   implementations placed beneath them can obtain required information
 *   without modification of the API itself.
 *
 * - Individual plugins are required to parse incoming parameters to determine
 *   if they are capable of performing the required elements of operations,
 *   and respond with a "not supported" error if they cannot do so. Request elements
 *   marked as "optional" may be ignored if they cannot be supported.
 *
 * - Implementations are free to "not support" any API and only implement
 *   a subset appropriate to their technology.
 *
 * - Storage is composed of key-value pairs, where the value is an arbitrary
 *   sized object. Data provided via the GDS interface will be in the form
 *   of a gds_info_t - data store implementations are free to convert to/from
 *   that form as they choose.
 *
 * - Underlying data store implementations are free from any requirement to
 *   support heterogeneity. Users are required to appropriately convert
 *   binary data as they require. Data type support is provided by GDS to assist
 *   with transport across heterogeneous systems as a convenience, not a requirement
 *
 * - Stored objects may be composed of multiple data elements, each expressed
 *   individually as its own key-value pair, with the overall objected referenced
 *   via the provided key. Thus, objects stored/fetched by a single GDS request
 *   can have multiple embedded layers within them, at the user's discretion.
 *
 * - Access control shall be provided by the underlying data store implementations.
 *   GDS will provide user information (uid and gid) to the implementation, but
 *   checking authorizations is the responsibility of the individual implementations
 *
 * - Implementations are free to explore local storage, distributed storage,
 *   service-based, library-based, and/or any combination of the above architectures.
 *   GDS does not currently provide messaging support, but plugins are free to
 *   implement any required support themselves
 *
 * - Data store implementations can be structured or unstructured - the only
 *   requirements are that the combination of the data store and the GDS plugin
 *   provide public-facing support for key-object queries and store requests, and
 *   that the GDS plugin accurately report the overall capabilities to the user
 *   when queried.
 *
 * - Implementations are free to use whatever programming language they prefer.
 *   The GDS infrastructure is provided in C as most languages support an interface
 *   to that language. API bindings for other languages (e.g., Python) will be provided
 *   over time.
 *
 */

#include "gds_common.h"

#if defined(c_plusplus) || defined(__cplusplus)
extern "C" {
#endif


/* initialize the library - setup the GDS plugin
 * infrastructure and assemble a prioritized list
 * of available plugins. The user may pass an optional
 * array of directives to "guide" the initialization
 * process by requesting optional behaviors. These will
 * be defined and extended over time. */
gds_status_t GDS_Init(gds_info_t directives[], size_t ndirs);

/* Query available datastores and support. Only data stores accessible
 * to the user/group will be included in the query. Directives can include:
 *
 * - GDS_QUERY_DSTORE: designation of a particular data store whose
 *                     capabilities are to be returned
 *
 * - GDQ_DSTORE_TYPE: restrict the query to data stores of the
 *                    indicated type(s)
 */
gds_status_t GDS_Query(gds_info_t directives[], size_t ndirs,
                       gds_query_cbfunc_t cbfunc, void *cbdata);

/* Attach to an existing Datastore by requesting a gds_dstor_handle_t
 * be returned for it. The name of the datastore is purely for the
 * caller's convenience - any provided value will be copied into the
 * returned GDS handle so the caller can refer to it.
 *
 * Directives may be used to further qualify the attach request,
 * perhaps identifying the desired Datastore or describing the
 * supporting capabilities required. In addition, directives can
 * indicate that the accepting plugin should create the described
 * datastore if not already existing.
 */
gds_status_t GDS_Attach(const char name[],
                        gds_info_t directives[], size_t ndirs,
                        gds_attach_cbfunc_t cbfunc, void *cbdata);

/* The following functions are all associated with a Datastore
 * handle returned in the GDS_Attach cbfunc.
 */

/* Store an object into a Datastore */
gds_status_t (*gds_store_fn_t)(gds_data_object_t *object,
                               gds_info_t directives[], size_t ndirs,
                               gds_release_cbfunc_t cbfunc, void *cbdata);

/* Fetch one or more objects from a Datastore
 *
 * - GDS_WAIT_UNTIL_COMPLETE: call the cbfunc when all matching objects
 *                            are available - e.g., if an object is locked,
 *                            then wait for it to unlock before returning
 *
 * - GDS_READ_LOCK_UPON_ACCESS: set the read lock on the object once access
 *                              is obtained - i.e., object is returned in a
 *                              read-locked state
 *
 * - GDS_WRITE_LOCK_UPON_ACCESS: set the write lock on the object once access
 *                               is obtained - i.e., object is returned in a
 *                               write-locked state
 *
 * - GDS_DELETE_LOCK_UPON_ACCESS: set the delete lock on the object once access
 *                                is obtained - i.e., object is returned in a
 *                                delete-locked state
 */
gds_status_t (*gds_fetch_fn_t)(char **keys,
                               gds_info_t directives[], size_t ndirs,
                               gds_fetch_cbfunc_t cbfunc, void *cbdata);

/* Delete an object from its Datastore */
gds_status_t (*gds_delete_fn_t)(gds_data_object_t *object,
                                gds_info_t directives[], size_t ndirs,
                                gds_release_cbfunc_t cbfunc, void *cbdata);

/* Query lock status on a Datastore object. This returns information
 * regarding the status of all locks on the object, including estimated
 * time of release (if available) and the identity of the lock holder.
 * Directives can be used to request that a lock be set if the object
 * is not already locked.
 *
 * Note that locks can be an expensive operation, and not all
 * implementations will choose to support it. If lock support
 * is required by a tool or application, then that should be
 * specified in the directives when attaching to a datastore.
 */
gds_status_t (*gds_query_lock_fn_t)(gds_data_object_t *object,
                                    gds_info_t directives[], size_t ndirs,
                                    gds_query_lock_cbfunc_t cbfunc, void *cbdata);

/* Lock a Datastore object. The directives can be
 * used to fine-tune the behavior of the function. This includes:
 *
 * - GDS_MAX_WAIT_TIME: return an error in the cbfunc if the specified
 *                      lock(s) cannot be obtained within the given time
 *
 * - GDS_READ_LOCK: obtain a read lock on the object
 *
 * - GDS_WRITE_LOCK: obtain a write lock on the object
 *
 * - GDS_DELETE_LOCK: obtain a delete lock on the object
 */
gds_status_t (*gds_lock_fn_t)(gds_data_object_t *object,
                              gds_info_t directives[], size_t ndirs,
                              gds_release_cbfunc_t cbfunc, void *cbdata);

/* Release a lock on an object */
gds_status_t (*gds_unlock_fn_t)(gds_data_object_t *object,
                                gds_info_t directives[], size_t ndirs,
                                gds_release_cbfunc_t cbfunc, void *cbdata);

/* Request notification of actions taken on a Datastore object
 *
 *
 * - GDS_NOTIFY_ON_MODIFICATION
 *
 * - GDS_NOTIFY_ON_ACCESS
 *
 * - GDS_NOTIFY_ON_DELETE
 *
 * - GDS_NOTIFY_ON_LOCK
 *
 * - GDS_NOTIFY_ON_UNLOCK
*/
gds_status_t (*gds_notify_fn_t)(gds_data_object_t *object,
                                gds_info_t directives[], size_t ndirs,
                                gds_event_notification_cbfunc_fn_t cbfunc, void *notify_cbdata,
                                gds_evhdlr_reg_cbfunc_t rel_cbfunc, void *cbdata);

/* Cancel an event registration */
gds status_t (*gds_denotify_fn_t)(gds_data_object_t *object,
                                  gds_info_t directives[], size_t ndirs,
                                  gds_release_cbfunc_t cbfunc, void *cbdata);

/****    GDS DATA STORE HANDLE    ****/
typedef struct gds_dstor_handle {
    char                    name[GDS_MAX_DSLEN+1];         // user-provided name
    gds_metadata_t          metadata;
    gds_store_fn_t          store;
    gds_fetch_fn_t          fetch;
    gds_delete_fn_t         delete;
    gds_query_lock_fn_t     query_lock;
    gds_lock_fn_t           lock;
    gds_unlock_fn_t         unlock;
    gds_notify_fn_t         register_event_hdlr;
    gds_denotify_fn_t       deregister_event_hdlr;
} gds_dstor_handle_t;


/* Detach from a Datastore */
gds_status_t GDS_Detach(gds_dstor_handle_t *hdl,
                        gds_info_t directives[], size_t ndirs,
                        gds_release_cbfunc_t cbfunc, void *cbdata);

/* Destroy a Datastore */
gds_status_t GDS_Destroy(gds_dstor_handle_t *hdl,
                         gds_info_t directives[], size_t ndirs,
                         gds_release_cbfunc_t cbfunc, void *cbdata);


#if defined(c_plusplus) || defined(__cplusplus)
}
#endif

#endif
