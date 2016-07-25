/*
 * Copyright (C) 2014      Artem Polyakov <artpol84@gmail.com>
 * Copyright (c) 2014-2016 Intel, Inc. All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include <src/include/gds_config.h>

#include <gds_common.h>

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <errno.h>
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif


#if GDS_ENABLE_TIMING

#include "src/class/gds_pointer_array.h"
#include "src/class/gds_list.h"
#include "src/util/output.h"
#include "src/util/basename.h"

#include "src/util/timings.h"

#define DELTAS_SANE_LIMIT (10*1024*1024)

struct interval_descr{
    gds_timing_event_t *descr_ev, *begin_ev;
    double interval, overhead;
};

gds_timing_event_t *gds_timing_event_alloc(gds_timing_t *t);
void gds_timing_init(gds_timing_t *t);
gds_timing_prep_t gds_timing_prep_ev(gds_timing_t *t, const char *fmt, ...);

static GDS_CLASS_INSTANCE(gds_timing_event_t, gds_list_item_t, NULL, NULL);


static char *nodename = NULL;
static char *jobid = "";
static double hnp_offs = 0;
static bool gds_timing_overhead = false;

void gds_init_id(char* nspace, int rank)
{
    asprintf(&jobid, "%s:%d", nspace, rank);
}

/* Get current timestamp. Derived from MPI_Wtime */

static double get_ts_gettimeofday(void)
{
    double ret;
    /* Use gettimeofday() if we gds wasn't initialized */
    struct timeval tv;
    gettimeofday(&tv, NULL);
    ret = tv.tv_sec;
    ret += (double)tv.tv_usec / 1000000.0;
    return ret;
}

static get_ts_t _init_timestamping(void)
{
    return get_ts_gettimeofday;
}


gds_timing_event_t *gds_timing_event_alloc(gds_timing_t *t)
{
    if( t->buffer_offset >= t->buffer_size ){
        // notch timings overhead
        double alloc_begin = t->get_ts();

        t->buffer = malloc(sizeof(gds_timing_event_t)*t->buffer_size);
        if( t->buffer == NULL ){
            return NULL;
        }
        memset(t->buffer, 0, sizeof(gds_timing_event_t)*t->buffer_size);

        double alloc_end = t->get_ts();

        t->buffer_offset = 0;
        t->buffer[0].fib = 1;
        t->buffer[0].ts_ovh = alloc_end - alloc_begin;
    }
    int tmp = t->buffer_offset;
    (t->buffer_offset)++;
    return t->buffer + tmp;
}

void gds_timing_init(gds_timing_t *t)
{
    memset(t,0,sizeof(*t));

    t->next_id_cntr = 0;
    t->current_id = -1;
    /* initialize events list */
    t->events = GDS_NEW(gds_list_t);
    /* Set buffer size */
    t->buffer_size = GDS_TIMING_BUFSIZE;
    /* Set buffer_offset = buffer_size so new buffer
     * will be allocated at first event report */
    t->buffer_offset = t->buffer_size;
    /* initialize gettime function */
    t->get_ts = _init_timestamping();

}

gds_timing_prep_t gds_timing_prep_ev(gds_timing_t *t, const char *fmt, ...)
{
    gds_timing_event_t *ev = gds_timing_event_alloc(t);
    if( ev == NULL ){
        gds_timing_prep_t p = { t, NULL, GDS_ERR_OUT_OF_RESOURCE };
        return p;
    }
    GDS_CONSTRUCT(ev, gds_timing_event_t);
    ev->ts = t->get_ts();
    va_list args;
    va_start( args, fmt );
    vsnprintf(ev->descr, GDS_TIMING_DESCR_MAX - 1, fmt, args);
    ev->descr[GDS_TIMING_DESCR_MAX-1] = '\0';
    va_end( args );
    gds_timing_prep_t p = { t, ev, 0 };
    return p;
}

gds_timing_prep_t gds_timing_prep_ev_end(gds_timing_t *t, const char *fmt, ...)
{
    gds_timing_prep_t p = { t, NULL, 0 };

    if( t->current_id >= 0 ){
        gds_timing_event_t *ev = gds_timing_event_alloc(t);
        if( ev == NULL ){
            gds_timing_prep_t p = { t, NULL, GDS_ERR_OUT_OF_RESOURCE };
            return p;
        }
        GDS_CONSTRUCT(ev, gds_timing_event_t);
        ev->ts = t->get_ts();
        p.ev = ev;
    }
    return p;
}

void gds_timing_add_step(gds_timing_prep_t p,
                          const char *func, const char *file, int line)
{
    if( !p.errcode ) {
        p.ev->func = func;
        p.ev->file = file;
        p.ev->line = line;
        p.ev->type = GDS_TIMING_TRACE;
        gds_list_append(p.t->events, (gds_list_item_t*)p.ev);
    }
}

/* Add description of the interval */
int gds_timing_descr(gds_timing_prep_t p,
                           const char *func, const char *file, int line)
{
    if( !p.errcode ){
        p.ev->func = func;
        p.ev->file = file;
        p.ev->line = line;
        p.ev->type = GDS_TIMING_INTDESCR;
        p.ev->id = p.t->next_id_cntr;
        (p.t->next_id_cntr)++;
        gds_list_append(p.t->events, (gds_list_item_t*)p.ev);
        return p.ev->id;
    }
    return -1;
}

void gds_timing_start_id(gds_timing_t *t, int id, const char *func, const char *file, int line)
{
    /* No description is needed. If everything is OK
     * it'll be included in gds_timing_start_init */
    gds_timing_event_t *ev = gds_timing_event_alloc(t);
    if( ev == NULL ){
        return;
    }
    GDS_CONSTRUCT(ev, gds_timing_event_t);

    t->current_id = id;
    ev->ts = t->get_ts();
    ev->func = func;
    ev->file = file;
    ev->line = line;
    ev->type = GDS_TIMING_INTBEGIN;
    ev->id = id;
    gds_list_append(t->events, (gds_list_item_t*)ev);
}

void gds_timing_end(gds_timing_t *t, int id, const char *func, const char *file, int line )
{
    /* No description is needed. If everything is OK
     * it'll be included in gds_timing_start_init */
    gds_timing_event_t *ev = gds_timing_event_alloc(t);
    if( ev == NULL ){
        return;
    }
    GDS_CONSTRUCT(ev, gds_timing_event_t);

    if( id < 0 ){
        ev->id = t->current_id;
        t->current_id = -1;
    } else {
        if( t->current_id == id ){
            t->current_id = -1;
        }
        ev->id = id;
    }
    ev->ts = t->get_ts();
    ev->func = func;
    ev->file = file;
    ev->line = line;
    ev->type = GDS_TIMING_INTEND;
    gds_list_append(t->events, (gds_list_item_t*)ev);
}

void gds_timing_end_prep(gds_timing_prep_t p,
                                        const char *func, const char *file, int line)
{
    gds_timing_event_t *ev = p.ev;

    if( !p.errcode && ( NULL != ev ) ){
        assert(  p.t->current_id >=0 );
        ev->id = p.t->current_id;
        p.t->current_id = -1;
        ev->func = func;
        ev->file = file;
        ev->line = line;
        ev->type = GDS_TIMING_INTEND;
        gds_list_append(p.t->events, (gds_list_item_t*)ev);
    }
}

static int _prepare_descriptions(gds_timing_t *t, struct interval_descr **__descr)
{
    struct interval_descr *descr;
    gds_timing_event_t *ev, *next;

    if( t->next_id_cntr == 0 ){
        return 0;
    }

    *__descr = malloc(sizeof(struct interval_descr) * t->next_id_cntr);
    descr = *__descr;
    memset(descr, 0, sizeof(struct interval_descr) * t->next_id_cntr);

    GDS_LIST_FOREACH_SAFE(ev, next, t->events, gds_timing_event_t){

        /* gds_output(0,"EVENT: type = %d, id=%d, ts = %.12le, ovh = %.12le %s",
                    ev->type, ev->id, ev->ts, ev->ts_ovh,
                    ev->descr );
        */
        switch(ev->type){
        case GDS_TIMING_INTDESCR:{
            if( ev->id >= t->next_id_cntr){
                char *file = gds_basename(ev->file);
                gds_output(0,"gds_timing: bad event id at %s:%d:%s, ignore and remove",
                            file, ev->line, ev->func);
                free(file);
                gds_list_remove_item(t->events, (gds_list_item_t *)ev);
                continue;
            }
            if( NULL != descr[ev->id].descr_ev ){
                gds_timing_event_t *prev = descr[ev->id].descr_ev;
                char *file = gds_basename(ev->file);
                char *file_prev = gds_basename(prev->file);
                gds_output(0,"gds_timing: duplicated description at %s:%d:%s, "
                            "previous: %s:%d:%s, ignore and remove", file, ev->line, ev->func,
                            file_prev, prev->line, prev->func);
                free(file);
                free(file_prev);
                gds_list_remove_item(t->events, (gds_list_item_t *)ev);
                continue;
            }

            descr[ev->id].descr_ev = ev;
            descr[ev->id].begin_ev = NULL;
            descr[ev->id].interval = 0;
            descr[ev->id].overhead = 0;
            break;
        }
        case GDS_TIMING_INTBEGIN:
        case GDS_TIMING_INTEND:{
            if( ev->id >= t->next_id_cntr || (NULL == descr[ev->id].descr_ev ) ){
                char *file = gds_basename(ev->file);
                gds_output(0,"gds_timing: bad event id at %s:%d:%s, ignore and remove",
                            file, ev->line, ev->func);
                free(file);
                gds_list_remove_item(t->events, (gds_list_item_t *)ev);
                continue;
            }
            break;
        }
        case GDS_TIMING_TRACE:
            break;
        }
    }
    return t->next_id_cntr;
}

/* Output lines in portions that doesn't
 * exceed GDS_TIMING_OUTBUF_SIZE for later automatic processing */
int gds_timing_report(gds_timing_t *t, char *fname)
{
    gds_timing_event_t *ev;
    FILE *fp = NULL;
    char *buf = NULL;
    int buf_size = 0;
    struct interval_descr *descr = NULL;
    int rc = GDS_SUCCESS;

    if( fname != NULL ){
        fp = fopen(fname,"a");
        if( fp == NULL ){
            gds_output(0, "gds_timing_report: Cannot open %s file"
                        " for writing timing information!",fname);
            rc = GDS_ERROR;
            goto err_exit;
        }
    }

    _prepare_descriptions(t, &descr);

    buf = malloc(GDS_TIMING_OUTBUF_SIZE+1);
    if( buf == NULL ){
        rc = GDS_ERR_OUT_OF_RESOURCE;
        goto err_exit;
    }
    buf[0] = '\0';

    double overhead = 0;
    GDS_LIST_FOREACH(ev, t->events, gds_timing_event_t){
        char *line, *file;
        if( ev->fib && gds_timing_overhead ){
            overhead += ev->ts_ovh;
        }
        file = gds_basename(ev->file);
        switch( ev->type ){
        case GDS_TIMING_INTDESCR:
            // Service event, skip it.
            continue;
        case GDS_TIMING_TRACE:
            rc = asprintf(&line,"[%s:%d] %s \"%s\" [GDS_TRACE] %s:%d %.10lf\n",
                          nodename, getpid(), jobid, ev->descr, file, ev->line,
                          ev->ts + hnp_offs + overhead);
            break;
        case GDS_TIMING_INTBEGIN:
            rc = asprintf(&line,"[%s:%d] %s \"%s [start]\" [GDS_TRACE] %s:%d %.10lf\n",
                          nodename, getpid(), jobid, descr[ev->id].descr_ev->descr,
                          file, ev->line, ev->ts + hnp_offs + overhead);
            break;
        case GDS_TIMING_INTEND:
            rc = asprintf(&line,"[%s:%d] %s \"%s [stop]\" [GDS_TRACE] %s:%d %.10lf\n",
                          nodename, getpid(), jobid, descr[ev->id].descr_ev->descr,
                          file, ev->line, ev->ts + hnp_offs + overhead);
            break;
        }
        free(file);

        if( rc < 0 ){
            rc = GDS_ERR_OUT_OF_RESOURCE;
            goto err_exit;
        }
        rc = 0;

        /* Sanity check: this shouldn't happen since description
             * is event only 1KB long and other fields should never
             * exceed 9KB */
        assert( strlen(line) <= GDS_TIMING_OUTBUF_SIZE );


        if( buf_size + strlen(line) > GDS_TIMING_OUTBUF_SIZE ){
            // flush buffer to the file
            if( fp != NULL ){
                fprintf(fp,"%s", buf);
                fprintf(fp,"\n");
            } else {
                gds_output(0,"\n%s", buf);
            }
            buf[0] = '\0';
            buf_size = 0;
        }
        sprintf(buf,"%s%s", buf, line);
        buf_size += strlen(line);
        free(line);
    }

    if( buf_size > 0 ){
        // flush buffer to the file
        if( fp != NULL ){
            fprintf(fp,"%s", buf);
            fprintf(fp,"\n");
        } else {
            gds_output(0,"\n%s", buf);
        }
        buf[0] = '\0';
        buf_size = 0;
    }

err_exit:
    if( NULL != descr ){
        free(descr);
    }
    if( buf != NULL ){
        free(buf);
    }
    if( fp != NULL ){
        fflush(fp);
        fclose(fp);
    }
    return rc;
}

/* Output events as one buffer so the data won't be mixed
 * with other output. This function is supposed to be human readable.
 * The output goes only to stdout. */
int gds_timing_deltas(gds_timing_t *t, char *fname)
{
    gds_timing_event_t *ev;
    FILE *fp = NULL;
    char *buf = NULL;
    struct interval_descr *descr = NULL;
    int i, rc = GDS_SUCCESS;
    size_t buf_size = 0, buf_used = 0;

    if( fname != NULL ){
        fp = fopen(fname,"a");
        if( fp == NULL ){
            gds_output(0, "gds_timing_report: Cannot open %s file"
                        " for writing timing information!",fname);
            rc = GDS_ERROR;
            goto err_exit;
        }
    }

    _prepare_descriptions(t, &descr);

    GDS_LIST_FOREACH(ev, t->events, gds_timing_event_t){
        int id;
        if( ev->fib ){
            /* this event caused buffered memory allocation
             * for events. Account the overhead for all active
             * intervals. */
            int i;
            for( i = 0; i < t->next_id_cntr; i++){
                if( (NULL != descr[i].descr_ev) && (NULL != descr[i].begin_ev) ){
                    if( gds_timing_overhead ){
                        descr[i].overhead += ev->ts_ovh;
                    }
                }
            }
        }

        /* we already process all GDS_TIMING_DESCR events
         * and we ignore GDS_TIMING_EVENT */
        if( ev->type == GDS_TIMING_INTDESCR ||
                ev->type == GDS_TIMING_TRACE){
            /* skip */
            continue;
        }

        id = ev->id;
        if( id < 0 || id >= t->next_id_cntr ){
            char *file = gds_basename(ev->file);
            gds_output(0,"gds_timing_deltas: bad interval event id: %d at %s:%d:%s (maxid=%d)",
                        id, file, ev->line, ev->func, t->next_id_cntr - 1 );
            free(file);
            /* skip */
            continue;
        }

        /* id's assigned auomatically. Ther shouldn't be any gaps in descr[] */
        assert( NULL != descr[id].descr_ev);

        if( ev->type == GDS_TIMING_INTBEGIN ){
            if( NULL != descr[id].begin_ev ){
                /* the measurement on this interval was already
                 * started! */
                gds_timing_event_t *prev = descr[ev->id].begin_ev;
                char *file = gds_basename(ev->file);
                char *file_prev = gds_basename(prev->file);
                gds_output(0,"gds_timing_deltas: duplicated start statement at %s:%d:%s, "
                            "previous: %s:%d:%s", file, ev->line, ev->func,
                            file_prev, prev->line, prev->func);
                free(file);
                free(file_prev);
            } else {
                /* save pointer to the start of measurement event */
                descr[id].begin_ev = ev;
            }
            /* done, go to the next event */
            continue;
        }

        if( ev->type == GDS_TIMING_INTEND ){
            if( NULL == descr[id].begin_ev ){
                /* the measurement on this interval wasn't started! */
                char *file = gds_basename(ev->file);
                gds_output(0,"gds_timing_deltas: inteval end without start at %s:%d:%s",
                            file, ev->line, ev->func );
                free(file);
            } else {
                descr[id].interval += ev->ts - descr[id].begin_ev->ts;
                descr[id].begin_ev = NULL;
                if( ev->fib ){
                    descr[id].overhead += ev->ts_ovh;
                }
            }
            continue;
        }

        /* shouldn't ever get here: bad ev->type */
        gds_output(0, "gds_timing_deltas: bad event type %d", ev->type);
        assert(0);
    }

    buf = malloc(GDS_TIMING_OUTBUF_SIZE + 1);
    if( buf == NULL ){
        rc = GDS_ERR_OUT_OF_RESOURCE;
        goto err_exit;
    }
    buf[0] = '\0';
    buf_size = GDS_TIMING_OUTBUF_SIZE + 1;
    buf_used = 0;
    for(i = 0; i < t->next_id_cntr; i++){
        char *line = NULL;
        size_t line_size;
        rc = asprintf(&line,"[%s:%d] %s \"%s\" [GDS_OVHD] %le\n",
                      nodename, getpid(), jobid, descr[i].descr_ev->descr,
                      descr[i].interval - descr[i].overhead);
        if( rc < 0 ){
            rc = GDS_ERR_OUT_OF_RESOURCE;
            goto err_exit;
        }
        rc = 0;
        line_size = strlen(line);

        /* Sanity check: this shouldn't happen since description
         * is event only 1KB long and other fields should never
         * exceed 9KB */
        assert( line_size <= GDS_TIMING_OUTBUF_SIZE );

        if( buf_used + strlen(line) > buf_size ){
            // Increase output buffer
            while( buf_used + line_size > buf_size && buf_size < DELTAS_SANE_LIMIT){
                buf_size += GDS_TIMING_OUTBUF_SIZE + 1;
            }
            if( buf_size > DELTAS_SANE_LIMIT ){
                gds_output(0, "gds_timing_report: delta sane limit overflow (%u > %u)!\n",
                            (unsigned int)buf_size, DELTAS_SANE_LIMIT);
                free(line);
                rc = GDS_ERR_OUT_OF_RESOURCE;
                goto err_exit;
            }
            buf = realloc(buf, buf_size);
            if( buf == NULL ){
                gds_output(0, "gds_timing_deltas: Out of memory!\n");
                rc = GDS_ERR_OUT_OF_RESOURCE;
                goto err_exit;
            }
        }
        sprintf(buf,"%s%s", buf, line);
        buf_used += line_size;
        free(line);
    }


    if( buf_used > 0 ){
        // flush buffer to the file
        if( fp != NULL ){
            fprintf(fp,"%s", buf);
            fprintf(fp,"\n");
        } else {
            gds_output(0,"\n%s", buf);
        }
        buf[0] = '\0';
        buf_size = 0;
    }

err_exit:
    if( NULL != descr ){
        free(descr);
    }
    if( NULL != buf ){
        free(buf);
    }
    if( fp != NULL ){
        fflush(fp);
        fclose(fp);
    }
    return rc;
}

void gds_timing_release(gds_timing_t *t)
{
    int cnt = gds_list_get_size(t->events);

    if( cnt > 0 ){
        gds_list_t *tmp = GDS_NEW(gds_list_t);
        int i;
        for(i=0; i<cnt; i++){
            gds_timing_event_t *ev = (gds_timing_event_t *)gds_list_remove_first(t->events);
            if( ev->fib ){
                gds_list_append(tmp,(gds_list_item_t*)ev);
            }
        }

        cnt = gds_list_get_size(tmp);
        for(i=0; i<cnt; i++){
            gds_timing_event_t *ev = (gds_timing_event_t *)gds_list_remove_first(tmp);
            free(ev);
        }
        GDS_RELEASE(tmp);
    } else {
        // Error case. At list one event was inserted at initialization.

    }

    GDS_RELEASE(t->events);
    t->events = NULL;
}
#endif
