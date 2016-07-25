/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2006-2015 Los Alamos National Security, LLC.  All rights
 *                         reserved.
 * Copyright (c) 2016      Intel, Inc. All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#ifndef GDS_MCA_GINSTALLDIRS_GINSTALLDIRS_H
#define GDS_MCA_GINSTALLDIRS_GINSTALLDIRS_H

#include <src/include/gds_config.h>

#include "src/mca/mca.h"
#include "src/mca/base/base.h"

BEGIN_C_DECLS

/*
 * Most of this file is just for ompi_info.  The only public interface
 * once gds_init has been called is the gds_ginstall_dirs structure
 * and the gds_ginstall_dirs_expand() call */
struct gds_ginstall_dirs_t {
    char* prefix;
    char* exec_prefix;
    char* bindir;
    char* sbindir;
    char* libexecdir;
    char* datarootdir;
    char* datadir;
    char* sysconfdir;
    char* sharedstatedir;
    char* localstatedir;
    char* libdir;
    char* includedir;
    char* infodir;
    char* mandir;

    /* Note that the following fields intentionally have an "ompi"
       prefix, even though they're down in the GDS layer.  This is
       not abstraction break because the "ompi" they're referring to
       is for the build system of the overall software tree -- not an
       individual project within that overall tree.

       Rather than using pkg{data,lib,includedir}, use our own
       ompi{data,lib,includedir}, which is always set to
       {datadir,libdir,includedir}/gds. This will keep us from
       having help files in prefix/share/open-rte when building
       without GDS, but in prefix/share/gds when building
       with GDS.

       Note that these field names match macros set by configure that
       are used in Makefile.am files.  E.g., project help files are
       installed into $(gdsdatadir). */
    char* gdsdatadir;
    char* gdslibdir;
    char* gdsincludedir;
};
typedef struct gds_ginstall_dirs_t gds_ginstall_dirs_t;

/* Install directories.  Only available after gds_init() */
extern gds_ginstall_dirs_t gds_ginstall_dirs;

/**
 * Expand out path variables (such as ${prefix}) in the input string
 * using the current gds_ginstall_dirs structure */
char * gds_ginstall_dirs_expand(const char* input);


/**
 * Structure for ginstalldirs components.
 */
struct gds_ginstalldirs_base_component_2_0_0_t {
    /** MCA base component */
    gds_mca_base_component_t component;
    /** MCA base data */
    gds_mca_base_component_data_t component_data;
    /** install directories provided by the given component */
    gds_ginstall_dirs_t install_dirs_data;
};
/**
 * Convenience typedef
 */
typedef struct gds_ginstalldirs_base_component_2_0_0_t gds_ginstalldirs_base_component_t;

/*
 * Macro for use in components that are of type ginstalldirs
 */
#define GDS_GINSTALLDIRS_BASE_VERSION_1_0_0 \
    GDS_MCA_BASE_VERSION_1_0_0("ginstalldirs", 1, 0, 0)

END_C_DECLS

#endif /* GDS_MCA_GINSTALLDIRS_GINSTALLDIRS_H */
