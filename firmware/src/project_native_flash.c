/* SPDX-License-Identifier: GPL-3.0-only */
/* Explicit post-migration main-loop connection; no boot caller grants it. */
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
    project_native_ops ops={NULL,pnf_catalog,pnf_save,pnf_load,pnf_rename};
    return project_native_bind(&ops,completed_migration_authorized);
}
