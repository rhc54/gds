/*
 * Copyright (c) 2004-2006 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2014      Intel, Inc. All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

/** @file
 *
 * Compiler-specific prefetch functions
 *
 * A small set of prefetch / prediction interfaces for using compiler
 * directives to improve memory prefetching and branch prediction
 */

#ifndef GDS_PREFETCH_H
#define GDS_PREFETCH_H

#if GDS_C_HAVE_BUILTIN_EXPECT
#define GDS_LIKELY(expression) __builtin_expect(!!(expression), 1)
#define GDS_UNLIKELY(expression) __builtin_expect(!!(expression), 0)
#else
#define GDS_LIKELY(expression) (expression)
#define GDS_UNLIKELY(expression) (expression)
#endif

#if GDS_C_HAVE_BUILTIN_PREFETCH
#define GDS_PREFETCH(address,rw,locality) __builtin_prefetch(address,rw,locality)
#else
#define GDS_PREFETCH(address,rw,locality)
#endif

#endif
