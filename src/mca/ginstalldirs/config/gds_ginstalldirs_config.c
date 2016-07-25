/*
 * Copyright (c) 2006-2007 Los Alamos National Security, LLC.  All rights
 *                         reserved.
 * Copyright (c) 2016      Intel, Inc. All rights reserved
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include <src/include/gds_config.h>

#include "src/mca/ginstalldirs/ginstalldirs.h"
#include "src/mca/ginstalldirs/config/ginstall_dirs.h"

const gds_ginstalldirs_base_component_t mca_ginstalldirs_config_component = {
    /* First, the mca_component_t struct containing meta information
       about the component itself */
    {
        GDS_GINSTALLDIRS_BASE_VERSION_1_0_0,

        /* Component name and version */
        "config",
        GDS_MAJOR_VERSION,
        GDS_MINOR_VERSION,
        GDS_RELEASE_VERSION,

        /* Component open and close functions */
        NULL,
        NULL
    },
    {
        /* This component is Checkpointable */
        GDS_MCA_BASE_METADATA_PARAM_CHECKPOINT
    },

    {
        GDS_INSTALL_PREFIX,
        GDS_EXEC_PREFIX,
        GDS_BINDIR,
        GDS_SBINDIR,
        GDS_LIBEXECDIR,
        GDS_DATAROOTDIR,
        GDS_DATADIR,
        GDS_SYSCONFDIR,
        GDS_SHAREDSTATEDIR,
        GDS_LOCALSTATEDIR,
        GDS_LIBDIR,
        GDS_INCLUDEDIR,
        GDS_INFODIR,
        GDS_MANDIR,
        GDS_PKGDATADIR,
        GDS_PKGLIBDIR,
        GDS_PKGINCLUDEDIR
    }
};
