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
#if NTRK == 8 && D8_MIGRATION_SESSION
    if(!d8mc_session_valid()&&!d8ls_read_ready())return D8POOL_UNSUPPORTED;
#endif
    project_native_ops ops={NULL,pnf_catalog,pnf_save,pnf_load,pnf_rename,pnf_prepare_arrangement};
    return project_native_bind(&ops,completed_migration_authorized);
}

#if NTRK == 8 && (FELUCCA_FLASH || defined(D8_NATIVE_READONLY_BOOT_TEST))
/* Read-only boot route. No USB session, native ticket, or physical write hook. */
static int st_boot_legacy_pair(unsigned block);
static int pnr_stopped(void *c)
{ (void)c;return df_stopped(NULL)&&!migration_owner&&mi_r==mi_w&&!midi_in_overflow&&
         !usb.ota_req&&!usb.uboot_req; }
static int pnr_admit(void) { return pnr_stopped(NULL)&&autosave_quiet(); }
static int pnr_erase(void *c,uint32_t off)
{ (void)c;(void)off;return -1; }
static int pnr_program(void *c,uint32_t off,const void *in,uint32_t n)
{ (void)c;(void)off;(void)in;(void)n;return -1; }
static int pnr_open(d8pool_mapped *m)
{
    d8pool_physical io={NULL,df_read,pnr_erase,pnr_program,pnr_stopped};
    return d8pool_mapped_open_readonly(m,&io);
}
static int pnr_catalog(void *c,d8p1_project_catalog *out)
{
    (void)c;d8pool_mapped m;int rc=pnr_open(&m);
    if(!rc)rc=d8p1_catalog_pool(&m.pool,out);
    d8pool_mapped_close(&m);return rc;
}
static int pnr_load(void *c,unsigned slot)
{
    (void)c;d8pool_mapped m;int rc=pnr_open(&m);
    if(!rc)rc=d8p1_load_pool_guarded(&m.pool,slot,pnr_admit);
    d8pool_mapped_close(&m);return rc;
}
static int pnr_arrangement(void *c)
{
    (void)c;d8pool_mapped m;int rc=pnr_open(&m);
    if(!rc)rc=d8arr_prepare_pool(&m.pool);
    d8pool_mapped_close(&m);return rc;
}
int project_native_boot_readonly(int allowed)
{
    if(!flash_ok)return 0;
    /* Classification precedes quiet/workspace checks. Unknown, partial and
     * foreign allocations latch offline ownership, never historical fallback.
     * Fully erased or verified historical records alone retain that route. */
    unsigned historical=1;
    for(unsigned b=0;b<5;b++)if(!st_boot_legacy_pair(b))historical=0;
    if(historical)return 0;
    project_native.mode=2;project_native.ready=0;project_native.readonly=1;
    project_native_ops ops={NULL,pnr_catalog,NULL,pnr_load,NULL,pnr_arrangement};
    int rc=project_native_bind_readonly(&ops);
    if(rc)return -rc;
    if(!allowed||(ui_prefs&PREF_RESTORE_OFF))return -D8POOL_EMPTY;
    rc=pnr_load(NULL,3);
    if(!rc){ui_message("RESTORED");return 1;}
    return -rc;
}

#endif /* native read-only boot route */
