/*
 * Copyright (c) 2006-2007 Los Alamos National Security, LLC.  All rights
 *                         reserved.
 * Copyright (c) 2007      Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2016      Intel, Inc. All rights reserved
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include <src/include/gds_config.h>

#include <stdlib.h>
#include <string.h>

#include "gds_common.h"
#include "src/mca/ginstalldirs/ginstalldirs.h"

static int ginstalldirs_env_open(void);


gds_ginstalldirs_base_component_t mca_ginstalldirs_env_component = {
    /* First, the mca_component_t struct containing meta information
       about the component itself */
    {
        GDS_GINSTALLDIRS_BASE_VERSION_1_0_0,

        /* Component name and version */
        "env",
        GDS_MAJOR_VERSION,
        GDS_MINOR_VERSION,
        GDS_RELEASE_VERSION,

        /* Component open and close functions */
        ginstalldirs_env_open,
        NULL
    },
    {
        /* This component is checkpointable */
        GDS_MCA_BASE_METADATA_PARAM_CHECKPOINT
    },

    /* Next the gds_ginstall_dirs_t install_dirs_data information */
    {
        NULL,
    },
};


#define SET_FIELD(field, envname)                                         \
    do {                                                                  \
        char *tmp = getenv(envname);                                      \
         if (NULL != tmp && 0 == strlen(tmp)) {                           \
             tmp = NULL;                                                  \
         }                                                                \
         mca_ginstalldirs_env_component.install_dirs_data.field = tmp;     \
    } while (0)


static int
ginstalldirs_env_open(void)
{
    SET_FIELD(prefix, "GDS_INSTALL_PREFIX");
    SET_FIELD(exec_prefix, "GDS_EXEC_PREFIX");
    SET_FIELD(bindir, "GDS_BINDIR");
    SET_FIELD(sbindir, "GDS_SBINDIR");
    SET_FIELD(libexecdir, "GDS_LIBEXECDIR");
    SET_FIELD(datarootdir, "GDS_DATAROOTDIR");
    SET_FIELD(datadir, "GDS_DATADIR");
    SET_FIELD(sysconfdir, "GDS_SYSCONFDIR");
    SET_FIELD(sharedstatedir, "GDS_SHAREDSTATEDIR");
    SET_FIELD(localstatedir, "GDS_LOCALSTATEDIR");
    SET_FIELD(libdir, "GDS_LIBDIR");
    SET_FIELD(includedir, "GDS_INCLUDEDIR");
    SET_FIELD(infodir, "GDS_INFODIR");
    SET_FIELD(mandir, "GDS_MANDIR");
    SET_FIELD(gdsdatadir, "GDS_PKGDATADIR");
    SET_FIELD(gdslibdir, "GDS_PKGLIBDIR");
    SET_FIELD(gdsincludedir, "GDS_PKGINCLUDEDIR");

    return GDS_SUCCESS;
}
