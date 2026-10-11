/* SPDX-License-Identifier: GPL-3.0-only */
/* Explicit post-migration main-loop connection; no boot caller grants it. */
#include "d8arr_prepare.c"
static int pnf_prepare_arrangement(void *c)
{
    (void)c;
    if(!flash_ok)return D8POOL_IO;
    d8pool_mapped mapped;d8pool_physical io={NULL,df_read,df_erase,df_program,df_stopped};
    int rc=d8pool_mapped_open(&mapped,&io,1);
    if(!rc)rc=d8arr_prepare_pool(&mapped.pool);
    d8pool_mapped_close(&mapped);return rc;
}
static int pnf_catalog(void *c,d8p1_project_catalog *out)
{ (void)c;return d8p1_catalog_flash(out,1); }
static int pnf_save(void *c,unsigned slot,const char *name)
{ (void)c;return d8p1_save_as_flash(slot,name,1); }
static int pnf_load(void *c,unsigned slot)
{ (void)c;return d8p1_load_flash(slot,1); }
static int pnf_rename(void *c,unsigned slot,const char *name)
{ (void)c;return d8p1_rename_flash(slot,name,1); }
int project_native_bind_flash(int completed_migration_authorized)
{
    project_native_ops ops={NULL,pnf_catalog,pnf_save,pnf_load,pnf_rename,pnf_prepare_arrangement};
    return project_native_bind(&ops,completed_migration_authorized);
}
