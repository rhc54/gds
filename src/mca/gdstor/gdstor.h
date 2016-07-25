/*
 * Copyright (c) 2004-2010 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2011 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2006-2013 Los Alamos National Security, LLC.
 *                         All rights reserved.
 * Copyright (c) 2009-2012 Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2011      Oak Ridge National Labs.  All rights reserved.
 * Copyright (c) 2013-2016 Intel, Inc.  All rights reserved.
 * Copyright (c) 2014-2015 Artem Y. Polyakov <artpol84@gmail.com>.
 *                         All rights reserved.
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

#ifndef USOCK_H
#define USOCK_H

#include <src/include/gds_config.h>

#include <src/include/types.h>
#include <gds/gds_common.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_SYS_UN_H
#include <sys/un.h>
#endif
#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif
#ifdef HAVE_NET_UIO_H
#include <net/uio.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#include GDS_EVENT_HEADER

#include "src/include/gds_globals.h"
#include "src/mca/bfrops/bfrops.h"
#include "src/class/gds_hash_table.h"
#include "src/class/gds_list.h"



/* usock structure for tracking posted recvs */
typedef struct {
    gds_list_item_t super;
    gds_event_t ev;
    uint32_t tag;
    gds_usock_cbfunc_t cbfunc;
    void *cbdata;
} gds_usock_posted_recv_t;
GDS_CLASS_DECLARATION(gds_usock_posted_recv_t);

/* usock struct for posting send/recv request */
typedef struct {
    gds_object_t super;
    gds_event_t ev;
    gds_peer_t *peer;
    gds_buffer_t *bfr;
    gds_usock_cbfunc_t cbfunc;
    void *cbdata;
} gds_usock_sr_t;
GDS_CLASS_DECLARATION(gds_usock_sr_t);

/* usock struct for tracking ops */
typedef struct {
    gds_list_item_t super;
    gds_event_t ev;
    volatile bool active;
    bool checked;
    int status;
    gds_status_t pstatus;
    gds_scope_t scope;
    gds_buffer_t data;
    gds_usock_cbfunc_t cbfunc;
    gds_op_cbfunc_t op_cbfunc;
    gds_value_cbfunc_t value_cbfunc;
    gds_lookup_cbfunc_t lookup_cbfunc;
    gds_spawn_cbfunc_t spawn_cbfunc;
    gds_evhdlr_reg_cbfunc_t errreg_cbfunc;
    size_t errhandler_ref;
    void *cbdata;
    char nspace[GDS_MAX_NSLEN+1];
    int rank;
    char *key;
    gds_value_t *value;
    gds_proc_t *procs;
    gds_info_t *info;
    size_t ninfo;
    size_t nvals;
} gds_cb_t;
GDS_CLASS_DECLARATION(gds_cb_t);

typedef struct {
    gds_object_t super;
    gds_event_t ev;
    void *cbdata;
} gds_timer_t;
GDS_CLASS_DECLARATION(gds_timer_t);

/* internal convenience macros */
#define GDS_ACTIVATE_SEND_RECV(p, b, cb, d)                            \
    do {                                                                \
        int rc = -1;                                                    \
        gds_usock_sr_t *ms;                                            \
        gds_output_verbose(5, gds_globals.debug_output,               \
                            "[%s:%d] post send to server",              \
                            __FILE__, __LINE__);                        \
        ms = GDS_NEW(gds_usock_sr_t);                                  \
        ms->peer = (p);                                                 \
        ms->bfr = (b);                                                  \
        ms->cbfunc = (cb);                                              \
        ms->cbdata = (d);                                               \
        rc = event_assign(&((ms)->ev), gds_globals.evbase, -1,         \
                          EV_WRITE, gds_usock_send_recv, (ms));        \
        gds_output_verbose(10, gds_globals.debug_output,              \
                            "event_assign returned %d", rc);            \
        event_active(&((ms)->ev), EV_WRITE, 1);                         \
    } while (0)

#define GDS_ACTIVATE_POST_MSG(ms)                                      \
    do {                                                                \
        gds_output_verbose(5, gds_globals.debug_output,               \
                            "[%s:%d] post msg",                         \
                            __FILE__, __LINE__);                        \
        event_assign(&((ms)->ev), gds_globals.evbase, -1,              \
                     EV_WRITE, gds_usock_process_msg, (ms));           \
        event_active(&((ms)->ev), EV_WRITE, 1);                         \
    } while (0)

#define CLOSE_THE_SOCKET(socket)                \
    do {                                        \
        if (0 <= socket) {                      \
            shutdown(socket, 2);                \
            close(socket);                      \
            socket = -1;                        \
        }                                       \
    } while (0)


#define GDS_TIMER_EVENT(s, f, d)                                       \
    do {                                                                \
        gds_timer_t *tm;                                               \
        struct timeval tv;                                              \
        tm = GDS_NEW(gds_timer_t);                                     \
        tm->cbdata = (d);                                               \
        event_assign(&tm->ev, gds_globals.evbase, -1, 0, (f), tm);     \
        tv.tv_gdstor = (s);                                                \
        tv.tv_ugdstor = 0;                                                 \
        GDS_OUTPUT_VERBOSE((1, gds_globals.debug_output,              \
                             "defining timer event: %ld gdstor %ld ugdstor at %s:%d", \
                             (long)tv.tv_gdstor, (long)tv.tv_ugdstor,         \
                             __FILE__, __LINE__));                      \
        event_add(&tm->ev, &tv);                                        \
    } while (0)


/* usock common variables */
typedef struct {
    gds_list_t posted_recvs;     // list of gds_usock_posted_recv_t
} gds_usock_globals_t;
extern gds_usock_globals_t gds_usock_globals;

/* usock common functions */
void gds_usock_init(gds_usock_cbfunc_t cbfunc);
void gds_usock_finalize(void);
gds_status_t gds_usock_set_nonblocking(int sd);
gds_status_t  gds_usock_set_blocking(int sd);
gds_status_t gds_usock_send_blocking(int sd, char *ptr, size_t size);
gds_status_t gds_usock_recv_blocking(int sd, char *data, size_t size);
void gds_usock_send_recv(int sd, short args, void *cbdata);
void gds_usock_send_handler(int sd, short flags, void *cbdata);
void gds_usock_recv_handler(int sd, short flags, void *cbdata);
void gds_usock_process_msg(int fd, short flags, void *cbdata);

#endif // USOCK_H
