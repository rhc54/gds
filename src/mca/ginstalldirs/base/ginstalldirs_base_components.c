/*
 * Copyright (c) 2006-2012 Los Alamos National Security, LLC.  All rights
 *                         reserved.
 * Copyright (c) 2007      Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2010      Sandia National Laboratories. All rights reserved.
 * Copyright (c) 2015      Research Organization for Information Science
 *                         and Technology (RIST). All rights reserved.
 * Copyright (c) 2016      Intel, Inc. All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 *
 */

#include <src/include/gds_config.h>

#include "gds_common.h"
#include "src/mca/mca.h"
#include "src/mca/ginstalldirs/ginstalldirs.h"
#include "src/mca/ginstalldirs/base/base.h"
#include "src/mca/ginstalldirs/base/static-components.h"

gds_ginstall_dirs_t gds_ginstall_dirs = {0};

#define CONDITIONAL_COPY(target, origin, field)                 \
    do {                                                        \
        if (origin.field != NULL && target.field == NULL) {     \
            target.field = origin.field;                        \
        }                                                       \
    } while (0)

static int
gds_ginstalldirs_base_open(gds_mca_base_open_flag_t flags)
{
    gds_mca_base_component_list_item_t *component_item;
    int ret;

    ret = gds_mca_base_framework_components_open(&gds_ginstalldirs_base_framework, flags);
    if (GDS_SUCCESS != ret) {
        return ret;
    }

    GDS_LIST_FOREACH(component_item, &gds_ginstalldirs_base_framework.framework_components, gds_mca_base_component_list_item_t) {
        const gds_ginstalldirs_base_component_t *component =
            (const gds_ginstalldirs_base_component_t *) component_item->cli_component;

        /* copy over the data, if something isn't already there */
        CONDITIONAL_COPY(gds_ginstall_dirs, component->install_dirs_data,
                         prefix);
        CONDITIONAL_COPY(gds_ginstall_dirs, component->install_dirs_data,
                         exec_prefix);
        CONDITIONAL_COPY(gds_ginstall_dirs, component->install_dirs_data,
                         bindir);
        CONDITIONAL_COPY(gds_ginstall_dirs, component->install_dirs_data,
                         sbindir);
        CONDITIONAL_COPY(gds_ginstall_dirs, component->install_dirs_data,
                         libexecdir);
        CONDITIONAL_COPY(gds_ginstall_dirs, component->install_dirs_data,
                         datarootdir);
        CONDITIONAL_COPY(gds_ginstall_dirs, component->install_dirs_data,
                         datadir);
        CONDITIONAL_COPY(gds_ginstall_dirs, component->install_dirs_data,
                         sysconfdir);
        CONDITIONAL_COPY(gds_ginstall_dirs, component->install_dirs_data,
                         sharedstatedir);
        CONDITIONAL_COPY(gds_ginstall_dirs, component->install_dirs_data,
                         localstatedir);
        CONDITIONAL_COPY(gds_ginstall_dirs, component->install_dirs_data,
                         libdir);
        CONDITIONAL_COPY(gds_ginstall_dirs, component->install_dirs_data,
                         includedir);
        CONDITIONAL_COPY(gds_ginstall_dirs, component->install_dirs_data,
                         infodir);
        CONDITIONAL_COPY(gds_ginstall_dirs, component->install_dirs_data,
                         mandir);
        CONDITIONAL_COPY(gds_ginstall_dirs, component->install_dirs_data,
                         gdsdatadir);
        CONDITIONAL_COPY(gds_ginstall_dirs, component->install_dirs_data,
                         gdslibdir);
        CONDITIONAL_COPY(gds_ginstall_dirs, component->install_dirs_data,
                         gdsincludedir);
    }

    /* expand out all the fields */
    gds_ginstall_dirs.prefix =
        gds_ginstall_dirs_expand_setup(gds_ginstall_dirs.prefix);
    gds_ginstall_dirs.exec_prefix =
        gds_ginstall_dirs_expand_setup(gds_ginstall_dirs.exec_prefix);
    gds_ginstall_dirs.bindir =
        gds_ginstall_dirs_expand_setup(gds_ginstall_dirs.bindir);
    gds_ginstall_dirs.sbindir =
        gds_ginstall_dirs_expand_setup(gds_ginstall_dirs.sbindir);
    gds_ginstall_dirs.libexecdir =
        gds_ginstall_dirs_expand_setup(gds_ginstall_dirs.libexecdir);
    gds_ginstall_dirs.datarootdir =
        gds_ginstall_dirs_expand_setup(gds_ginstall_dirs.datarootdir);
    gds_ginstall_dirs.datadir =
        gds_ginstall_dirs_expand_setup(gds_ginstall_dirs.datadir);
    gds_ginstall_dirs.sysconfdir =
        gds_ginstall_dirs_expand_setup(gds_ginstall_dirs.sysconfdir);
    gds_ginstall_dirs.sharedstatedir =
        gds_ginstall_dirs_expand_setup(gds_ginstall_dirs.sharedstatedir);
    gds_ginstall_dirs.localstatedir =
        gds_ginstall_dirs_expand_setup(gds_ginstall_dirs.localstatedir);
    gds_ginstall_dirs.libdir =
        gds_ginstall_dirs_expand_setup(gds_ginstall_dirs.libdir);
    gds_ginstall_dirs.includedir =
        gds_ginstall_dirs_expand_setup(gds_ginstall_dirs.includedir);
    gds_ginstall_dirs.infodir =
        gds_ginstall_dirs_expand_setup(gds_ginstall_dirs.infodir);
    gds_ginstall_dirs.mandir =
        gds_ginstall_dirs_expand_setup(gds_ginstall_dirs.mandir);
    gds_ginstall_dirs.gdsdatadir =
        gds_ginstall_dirs_expand_setup(gds_ginstall_dirs.gdsdatadir);
    gds_ginstall_dirs.gdslibdir =
        gds_ginstall_dirs_expand_setup(gds_ginstall_dirs.gdslibdir);
    gds_ginstall_dirs.gdsincludedir =
        gds_ginstall_dirs_expand_setup(gds_ginstall_dirs.gdsincludedir);

#if 0
    fprintf(stderr, "prefix:         %s\n", gds_ginstall_dirs.prefix);
    fprintf(stderr, "exec_prefix:    %s\n", gds_ginstall_dirs.exec_prefix);
    fprintf(stderr, "bindir:         %s\n", gds_ginstall_dirs.bindir);
    fprintf(stderr, "sbindir:        %s\n", gds_ginstall_dirs.sbindir);
    fprintf(stderr, "libexecdir:     %s\n", gds_ginstall_dirs.libexecdir);
    fprintf(stderr, "datarootdir:    %s\n", gds_ginstall_dirs.datarootdir);
    fprintf(stderr, "datadir:        %s\n", gds_ginstall_dirs.datadir);
    fprintf(stderr, "sysconfdir:     %s\n", gds_ginstall_dirs.sysconfdir);
    fprintf(stderr, "sharedstatedir: %s\n", gds_ginstall_dirs.sharedstatedir);
    fprintf(stderr, "localstatedir:  %s\n", gds_ginstall_dirs.localstatedir);
    fprintf(stderr, "libdir:         %s\n", gds_ginstall_dirs.libdir);
    fprintf(stderr, "includedir:     %s\n", gds_ginstall_dirs.includedir);
    fprintf(stderr, "infodir:        %s\n", gds_ginstall_dirs.infodir);
    fprintf(stderr, "mandir:         %s\n", gds_ginstall_dirs.mandir);
    fprintf(stderr, "pkgdatadir:     %s\n", gds_ginstall_dirs.pkgdatadir);
    fprintf(stderr, "pkglibdir:      %s\n", gds_ginstall_dirs.pkglibdir);
    fprintf(stderr, "pkgincludedir:  %s\n", gds_ginstall_dirs.pkgincludedir);
#endif

    /* NTH: Is it ok not to close the components? If not we can add a flag
       to mca_base_framework_components_close to indicate not to deregister
       variable groups */
    return GDS_SUCCESS;
}


static int
gds_ginstalldirs_base_close(void)
{
    free(gds_ginstall_dirs.prefix);
    free(gds_ginstall_dirs.exec_prefix);
    free(gds_ginstall_dirs.bindir);
    free(gds_ginstall_dirs.sbindir);
    free(gds_ginstall_dirs.libexecdir);
    free(gds_ginstall_dirs.datarootdir);
    free(gds_ginstall_dirs.datadir);
    free(gds_ginstall_dirs.sysconfdir);
    free(gds_ginstall_dirs.sharedstatedir);
    free(gds_ginstall_dirs.localstatedir);
    free(gds_ginstall_dirs.libdir);
    free(gds_ginstall_dirs.includedir);
    free(gds_ginstall_dirs.infodir);
    free(gds_ginstall_dirs.mandir);
    free(gds_ginstall_dirs.gdsdatadir);
    free(gds_ginstall_dirs.gdslibdir);
    free(gds_ginstall_dirs.gdsincludedir);
    memset (&gds_ginstall_dirs, 0, sizeof (gds_ginstall_dirs));

    return gds_mca_base_framework_components_close (&gds_ginstalldirs_base_framework, NULL);
}

/* Declare the ginstalldirs framework */
GDS_MCA_BASE_FRAMEWORK_DECLARE(gds, ginstalldirs, NULL, NULL, gds_ginstalldirs_base_open,
                                gds_ginstalldirs_base_close, mca_ginstalldirs_base_static_components,
                                GDS_MCA_BASE_FRAMEWORK_FLAG_NOREGISTER | GDS_MCA_BASE_FRAMEWORK_FLAG_NO_DSO);
