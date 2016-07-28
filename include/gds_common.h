/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2016      Intel, Inc. All rights reserved
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

#ifndef GDS_COMMON_H
#define GDS_COMMON_H

#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h> /* for struct timeval */
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h> /* for uid_t and gid_t */
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h> /* for uid_t and gid_t */
#endif
#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif

#if defined(c_plusplus) || defined(__cplusplus)
extern "C" {
#endif

/****  GDS CONSTANTS    ****/

/* define maximum value and key sizes */
#define GDS_MAX_DSLEN     255
#define GDS_MAX_KEYLEN    511
#if defined(MAXHOSTNAMELEN)
#define GDS_MAXHOSTNAMELEN (MAXHOSTNAMELEN + 1)
#elif defined(HOST_NAME_MAX)
#define GDS_MAXHOSTNAMELEN (HOST_NAME_MAX + 1)
#else
/* SUSv2 guarantees that "Host names are limited to 255 bytes". */
#define GDS_MAXHOSTNAMELEN (255 + 1)
#endif

/* define a set of "standard" GDS attributes that can
 * be queried. Implementations (and users) are free to extend as
 * desired, so the get functions need to be capable
 * of handling the "not found" condition. Note that these
 * are attributes of the system and the job as opposed to
 * values the application (or underlying MPI library)
 * might choose to expose - i.e., they are values provided
 * by the resource manager as opposed to the application. Thus,
 * these keys are RESERVED */
#define GDS_ATTR_UNDEF      NULL

/* initialization directives */

/* identification attributes */
#define GDS_USERID                          "gds.euid"              // (uint32_t) effective user id
#define GDS_GRPID                           "gds.egid"              // (uint32_t) effective group id

/* query directives */
#define GDS_QUERY_DSTORE                    "gds.qdstore"           // (char*) name of a particular data store whose capabilities are being queried
#define GDS_DSTORE_TYPE                     "gds.dtype"             // (char*) case-insensitive, comma-delimited list of data store types (e.g., dht)

/* operational directives */
#define GDS_WAIT_UNTIL_COMPLETE             "gds.wait"              // (bool) wait until operation is complete before callback
#define GDS_MAX_WAIT_TIME                   "gds.maxwait"           // (struct timeval_t) max time to wait before returning error callback
#define GDS_READ_LOCK_UPON_ACCESS           "gds.rlockacc"          // (bool) set read lock on object upon access
#define GDS_WRITE_LOCK_UPON_ACCESS          "gds.wlockacc"          // (bool) set write lock on object upon access
#define GDS_DELETE_LOCK_UPON_ACCESS         "gds.dlockacc"          // (bool) set delete lock on object upon access
#define GDS_READ_LOCK                       "gds.rlock"             // (bool) obtain a read lock on the object
#define GDS_WRITE_LOCK                      "gds.wlock"             // (bool) obtain a write lock on the object
#define GDS_DELETE_LOCK                     "gds.dlock"             // (bool) obtain a delete lock on the object

/* notification directives */
#define GDS_NOTIFY_ON_MODIFICATION          "gds.nmod"              // (bool) notify when object is modified
#define GDS_NOTIFY_ON_ACCESS                "gds.amod"              // (bool) notify when object is accessed
#define GDS_NOTIFY_ON_DELETE                "gds.dmod"              // (bool) notify when object is deleted
#define GDS_NOTIFY_ON_LOCK                  "gds.nlock"             // (bool) notify when object is locked
#define GDS_NOTIFY_ON_UNLOCK                "gds.nunlock"           // (bool) notify when object is unlocked
#define GDS_NOTIFY_CANCEL_ON_NOTIFICATION   "gds.cnot"              // (bool) cancel the registration upon first notification

/****    GDS ERROR CONSTANTS    ****/
/* GDS errors are always negative, with 0 reserved for success */
#define GDS_ERR_BASE                    0

typedef int gds_status_t;

#define GDS_SUCCESS                             (GDS_ERR_BASE)
#define GDS_ERROR                               (GDS_ERR_BASE -  1)    // general error

/* operational */
#define GDS_OP_ERR_BASE                 -100
#define GDS_ERR_NO_PERMISSIONS                  (GDS_OP_ERR_BASE -  1)
#define GDS_ERR_TIMEOUT                         (GDS_OP_ERR_BASE -  2)
#define GDS_ERR_WOULD_BLOCK                     (GDS_OP_ERR_BASE -  3)
#define GDS_EXISTS                              (GDS_OP_ERR_BASE -  4)
#define GDS_ERR_NOT_SUPPORTED                   (GDS_OP_ERR_BASE -  5)
#define GDS_ERR_NOT_FOUND                       (GDS_OP_ERR_BASE -  6)
#define GDS_ERR_BAD_PARAM                       (GDS_OP_ERR_BASE -  7)
#define GDS_ERR_DATA_VALUE_NOT_FOUND            (GDS_OP_ERR_BASE -  8)
#define GDS_ERR_OUT_OF_RESOURCE                 (GDS_OP_ERR_BASE -  9)
#define GDS_ERR_INIT                            (GDS_OP_ERR_BASE - 10)
#define GDS_ERR_EVENT_REGISTRATION              (GDS_OP_ERR_BASE - 11)

/* notification */
#define GDS_NOTIFY_ERR_BASE             -200
#define GDS_ERR_OBJ_MODIFIED                    (GDS_NOTIFY_ERR_BASE -  1)
#define GDS_ERR_OBJ_ACCESSED                    (GDS_NOTIFY_ERR_BASE -  2)
#define GDS_ERR_OBJ_DELETED                     (GDS_NOTIFY_ERR_BASE -  3)
#define GDS_ERR_OBJ_LOCKED                      (GDS_NOTIFY_ERR_BASE -  4)
#define GDS_ERR_OBJ_UNLOCKED                    (GDS_NOTIFY_ERR_BASE -  5)


/* define a starting point for user-level defined error
 * constants - negative values larger than this are guaranteed
 * not to conflict with GDS values. Definitions should always
 * be based on the GDS_EXTERNAL_ERR_BASE constant and -not- a
 * specific value as the value of the constant may change */
#define GDS_EXTERNAL_ERR_BASE           -5000

/****    GDS DATA TYPES    ****/
typedef int16_t gds_data_type_t;
#define GDS_UNDEF               0
#define GDS_BOOL                1  // converted to/from native true/false to uint8 for pack/unpack
#define GDS_BYTE                2  // a byte of data
#define GDS_STRING              3  // NULL-terminated string
#define GDS_SIZE                4  // size_t
#define GDS_PID                 5  // OS-pid
#define GDS_INT                 6
#define GDS_INT8                7
#define GDS_INT16               8
#define GDS_INT32               9
#define GDS_INT64              10
#define GDS_UINT               11
#define GDS_UINT8              12
#define GDS_UINT16             13
#define GDS_UINT32             14
#define GDS_UINT64             15
#define GDS_FLOAT              16
#define GDS_DOUBLE             17
#define GDS_TIMEVAL            18
#define GDS_TIME               19
#define GDS_STATUS             20  // needs to be tracked separately from integer so it can be converted to
                                   // host error definitions
#define GDS_HWLOC_TOPO         21
#define GDS_VALUE              22
#define GDS_INFO_ARRAY         23
#define GDS_PROC               24
#define GDS_APP                25
#define GDS_INFO               26
#define GDS_DINFO              27
#define GDS_BUFFER             28
#define GDS_BYTE_OBJECT        29
#define GDS_KVAL               30
#define GDS_MODEX              31
#define GDS_PERSIST            32
#define GDS_POINTER            33
#define GDS_SCOPE              34
#define GDS_DATA_RANGE         35
#define GDS_COMMAND            36
#define GDS_INFO_DIRECTIVES    37
#define GDS_DATA_TYPE          38


/* define a set of bit-mask flags for specifying behavior of
 * command directives via gds_info_t arrays */
typedef uint32_t gds_info_directives_t;
#define GDS_INFO_REQD          0x0001


/****    GDS BYTE OBJECT    ****/
typedef struct gds_byte_object {
    char *bytes;
    size_t size;
} gds_byte_object_t;


/****    GDS VALUE STRUCT    ****/
typedef struct gds_info_t gds_info_t;

typedef struct gds_info_array {
    size_t size;
    gds_info_t *array;
} gds_info_array_t;
/* NOTE: operations can supply a collection of values under
 * a single key by passing a gds_value_t containing an
 * array of type GDS_INFO_ARRAY, with each array element
 * containing its own gds_info_t object */

typedef struct gds_value {
    gds_data_type_t type;
    union {
        bool flag;
        uint8_t byte;
        char *string;
        size_t size;
        pid_t pid;
        int integer;
        int8_t int8;
        int16_t int16;
        int32_t int32;
        int64_t int64;
        unsigned int uint;
        uint8_t uint8;
        uint16_t uint16;
        uint32_t uint32;
        uint64_t uint64;
        float fval;
        double dval;
        time_t time;
        struct timeval tv;
        gds_status_t status;
        gds_proc_t proc;
        gds_info_array_t array;
        gds_byte_object_t bo;
        void *ptr;
    } data;
} gds_value_t;
/* allocate and initialize a specified number of value structs */
#define GDS_VALUE_CREATE(m, n)                                         \
    do {                                                                \
        int _ii;                                                        \
        (m) = (gds_value_t*)malloc((n) * sizeof(gds_value_t));        \
        memset((m), 0, (n) * sizeof(gds_value_t));                     \
        for (_ii=0; _ii < (int)(n); _ii++) {                            \
            (m)[_ii].type = GDS_UNDEF;                                 \
        }                                                               \
    } while (0)

/* release a single gds_value_t struct, including its data */
#define GDS_VALUE_RELEASE(m)                                           \
    do {                                                                \
        GDS_VALUE_FREE((m), 1);                                        \
    } while (0)

/* initialize a single value struct */
#define GDS_VALUE_CONSTRUCT(m)                 \
    do {                                        \
        memset((m), 0, sizeof(gds_value_t));   \
        (m)->type = GDS_UNDEF;                 \
    } while (0)

/* release the memory in the value struct data field */
#define GDS_VALUE_DESTRUCT(m)                                          \
    do {                                                                \
        if (GDS_STRING == (m)->type) {                                 \
            if (NULL != (m)->data.string) {                             \
                free((m)->data.string);                                 \
                (m)->data.string = NULL;                                \
            }                                                           \
        } else if (GDS_BYTE_OBJECT == (m)->type) {                     \
            if (NULL != (m)->data.bo.bytes) {                           \
                free((m)->data.bo.bytes);                               \
                (m)->data.bo.bytes = NULL;                              \
            }                                                           \
        } else if (GDS_INFO_ARRAY == (m)->type) {                      \
            size_t _n;                                                  \
            gds_info_t *_p = (gds_info_t*)((m)->data.array.array);    \
            if (NULL != _p) {                                           \
                for (_n=0; _n < (m)->data.array.size; _n++) {           \
                    if (GDS_STRING == _p[_n].value.type) {             \
                        if (NULL != _p[_n].value.data.string) {         \
                            free(_p[_n].value.data.string);             \
                            _p[_n].value.data.string = NULL;            \
                        }                                               \
                    } else if (GDS_BYTE_OBJECT == _p[_n].value.type) { \
                        if (NULL != _p[_n].value.data.bo.bytes) {       \
                            free(_p[_n].value.data.bo.bytes);           \
                            _p[_n].value.data.bo.bytes = NULL;          \
                        }                                               \
                    }                                                   \
                }                                                       \
                free(_p);                                               \
                (m)->data.array.array = NULL;                           \
            }                                                           \
        }                                                               \
    } while (0)

#define GDS_VALUE_FREE(m, n)                           \
    do {                                                \
        size_t _s;                                      \
        if (NULL != (m)) {                              \
            for (_s=0; _s < (n); _s++) {                \
                GDS_VALUE_DESTRUCT(&((m)[_s]));        \
            }                                           \
            free((m));                                  \
            (m) = NULL;                                 \
        }                                               \
    } while (0)

/* expose two functions that are resolved in the
 * GDS library, but part of a header that
 * includes internal functions - we don't
 * want to expose the entire header here
 */
void gds_value_load(gds_value_t *v, void *data, gds_data_type_t type);
gds_status_t gds_value_xfer(gds_value_t *kv, gds_value_t *src);




/****    GDS INFO STRUCT    ****/
struct gds_info_t {
    char key[GDS_MAX_KEYLEN+1];    // ensure room for the NULL terminator
    gds_info_directives_t flags;   // bit-mask of flags
    gds_value_t value;
};

/* utility macros for working with gds_info_t structs */
#define GDS_INFO_CREATE(m, n)                                  \
    do {                                                        \
        (m) = (gds_info_t*)malloc((n) * sizeof(gds_info_t));  \
        memset((m), 0, (n) * sizeof(gds_info_t));              \
    } while (0)

#define GDS_INFO_CONSTRUCT(m)                  \
    do {                                        \
        memset((m), 0, sizeof(gds_info_t));    \
        (m)->value.type = GDS_UNDEF;           \
    } while (0)

#define GDS_INFO_DESTRUCT(m) \
    do {                                        \
        GDS_VALUE_DESTRUCT(&(m)->value);       \
    } while (0)

#define GDS_INFO_FREE(m, n)                    \
    do {                                        \
        size_t _s;                              \
        if (NULL != (m)) {                      \
            for (_s=0; _s < (n); _s++) {        \
                GDS_INFO_DESTRUCT(&((m)[_s])); \
            }                                   \
            free((m));                          \
            (m) = NULL;                         \
        }                                       \
    } while (0)

#define GDS_INFO_LOAD(m, k, v, t)                      \
    do {                                                \
        (void)strncpy((m)->key, (k), GDS_MAX_KEYLEN);  \
        gds_value_load(&((m)->value), (v), (t));       \
    } while (0)
#define GDS_INFO_XFER(d, s)                                \
    do {                                                    \
        (void)strncpy((d)->key, (s)->key, GDS_MAX_KEYLEN); \
        (d)->flags = (s)->flags;                            \
        gds_value_xfer(&(d)->value, &(s)->value);          \
    } while(0)

#define GDS_INFO_REQUIRED(m)       \
    (m)->flags |= GDS_INFO_REQD;
#define GDS_INFO_OPTIONAL(m)       \
    (m)->flags &= ~GDS_INFO_REQD;


/****    GDS QUERY RETURN STRUCT    ****/
typedef struct gds_dstor_info {
    char name[GDS_MAX_DSLEN+1];  // ensure room for the NULL terminator
    gds_info_t info[];
    size_t ninfo;
} gds_dstor_info_t;

/* utility macros for working with gds_dstor_info_t structs */
#define GDS_DINFO_CREATE(m, n)                                              \
    do {                                                                    \
        (m) = (gds_dstor_info_t*)malloc((n) * sizeof(gds_dstor_info_t));    \
        memset((m), 0, (n) * sizeof(gds_dstor_info_t));                     \
    } while (0)

#define GDS_DINFO_RELEASE(m)        \
    do {                            \
        GDS_DINFO_FREE((m), 1);     \
    } while (0)

#define GDS_DINFO_CONSTRUCT(m)                      \
    do {                                            \
        memset((m), 0, sizeof(gds_dstor_info_t));   \
    } while (0)

#define GDS_DINFO_DESTRUCT(m)                       \
    do {                                            \
        if (NULL != (m)->info) {                    \
            GDS_INFO_FREE((m)->info, (m)->ninfo);   \
        }                                           \
    } while (0)

#define GDS_DINFO_FREE(m, n)                           \
    do {                                                \
        size_t _s;                                      \
        if (NULL != (m)) {                              \
            for (_s=0; _s < (n); _s++) {                \
                GDS_DINFO_DESTRUCT(&((m)[_s]));        \
            }                                           \
            free((m));                                  \
            (m) = NULL;                                 \
        }                                               \
    } while (0)


/****    DATA STORE LOCK OBJECT    ****/
typedef struct gds_lock {
    bool locked;        // whether or not the lock is active
    uid_t uid;          // user ID holding the lock
    gid_t gid;          // group ID of user holding the lock
    char *time_taken;   // time the lock was taken
    char *ert;          // estimated time of release, if given
} gds_lock_t;


/****    DATA OBJECT VERSION    ****/
typedef uint64_t gds_version_t;

/****    OPS RECORDING OBJECT    ****/
typedef struct gds_stamp {
    uid_t uid;
    gid_t gid;
    char *time;
} gds_stamp_t;
/* convenience macros for timestamping ops */
#define GDS_STAMP(r, u, g)                                      \
    do {                                                        \
        time_t _t;                                              \
        if (NULL == (r)) {                                      \
            (r) = (gds_stamp_t*)malloc(sizeof(gds_stamp_t));    \
        }                                                       \
        (r)->uid = (u);                                         \
        (r)->gid = (g);                                         \
        if (NULL != (r)->time) {                                \
            free((r)->time);                                    \
        }                                                       \
        _t = time();                                            \
        (r)->time = strdup(ctime(_t));                          \
    } while(0)


/****    GDS METATDATA OBJECT    ****/
typedef struct gds_metadata {
    /* track the creator of the object, and when
     * it was created */
    gds_stamp_t *creator;
    /* track the last user to modify it, and when */
    gds_stamp_t *modifier;
    /* track the last user to access it, and when */
    gds_stamp_t *accessor;
    /* maintain a simple count of the number of times
     * this object has been modified */
    gds_version_t version;
    /* provide ability to lock the object */
    gds_lock_t readlock;        // preclude anyone else from accessing the object
    gds_lock_t writelock;       // preclude anyone else from overwriting the object
    gds_lock_t deletelock;      // preclude anyone else from deleting the object
    /* access control authorizations */
    gds_info_t *authorizations;
    size_t nauths;;
} gds_metadata_t;


/****    GDS BASIC DATA OBJECT    ****/
typedef gds_data_object {
    gds_metadata_t metadata;
    char key[GDS_MAX_KEYLEN+1]; // leave room for terminating NULL
    gds_value_t value;
} gds_data_object_t;


/****    CALLBACK FUNCTIONS FOR NON-BLOCKING OPERATIONS    ****/

/* general release callback function */
typedef void (*gds_release_cbfunc_t)(gds_status_t status, void *cbdata);

/* define a callback function for returning the results
 * of a query for available datastores and their support.
 * The release_cbfunc is provided so that GDS can release
 * the info array when the query_cbfunc has completed */
typedef void (*gds_query_cbfunc_t)(gds_status_t status,
                                   gds_dstor_info_t info[], size_t ndinfo,
                                   gds_release_cbfunc_t cbfunc, void *relcbdata,
                                   void query_cbdata);

/* define a callback function to return a GDS data store
 * handle
 */
typedef void (*gds_attach_cbfunc_t)(gds_status_t status,
                                    gds_dstor_handle_t *hdl,
                                    void cbdata);

/* define a callback function for returning found data objects. The
 * array of objects will be malloc'd and must be free'd by the
 * receiver when done - i.e., the GDS library will not release
 * this memory. */
typedef void (*gds_fetch_cbfunc_t)(gds_status_t status,
                                   gds_data_object_t objects[], size_t nobjs,
                                   void fetch_cbdata);

/* define a callback function for returning info from a lock
 * query request.
 */
typedef void (*gds_query_lock_cbfunc_t)(gds_status_t status,
                                        gds_lock_t locks[], size_t nlocks,
                                        void *query_cbdata);

/* define a callback by which a Datastore can notify a requestor
 * that a specified action has occurred on the designated object.
 * The returned status will indicate what action occurred. If non-NULL,
 * the info array will contain further information on the event (e.g.,
 * the uid/gid of the tool that locked an object). The original
 * requestor's cbdata is also returned.
 *
 * Event handlers are required to call the release cbfunc when
 * done processing the event so that any malloc'd data (e.g.,
 * the info array) can be released.
 */
typedef void (*gds_event_notification_cbfunc_fn_t)(gds_status_t status,
                                                   gds_data_object_t *object,
                                                   gds_info_t info[], size_t ninfo,
                                                   void *notify_cbdata,
                                                   gds_release_cbfunc_t cbfunc, void *cbdata);

/* define a callback function for calls to GDS_Register_evhdlr. The
 * status indicates if the request was successful or not, evhdlr_ref is
 * an integer reference assigned to the event handler by GDS, this reference
 * must be used to deregister the err handler. A ptr to the original
 * cbdata is returned. */
typedef void (*gds_evhdlr_reg_cbfunc_t)(gds_status_t status,
                                         size_t evhdlr_ref,
                                         void *cbdata);

/* Provide a string representation of a gds_status_t value. Note
 * that the provided string is statically defined and must NOT be
 * free'd */
const char* GDS_Error_string(gds_status_t status);

/* Get the GDS version string. Note that the provided string is
 * statically defined and must NOT be free'd  */
const char* GDS_Get_version(void);


#if defined(c_plusplus) || defined(__cplusplus)
}
#endif

#endif
