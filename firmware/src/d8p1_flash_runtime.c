/* SPDX-License-Identifier: GPL-3.0-only */
/* Eight-track existing-driver adapter. No scheduler, migration or menu hook.
 * Main loop, IRQs enabled on entry: upstream helpers always restore STI and
 * do not preserve nesting. Each call mutates at most one sector/page. */
#include "d8pool_mapped.h"
static int df_read(void *context,uint32_t off,void *out,uint32_t n)
{ (void)context;return flash_ok?st_read(off,out,n):-1; }
static int df_stopped(void *context);
static int df_erase(void *context,uint32_t off)
{
    (void)context;uint32_t flags=irq_save();
    int rc=df_stopped(NULL)?st_erase(off):-1;
    irq_restore(flags);return rc;
}
static int df_program(void *context,uint32_t off,const void *in,uint32_t n)
{
    (void)context;uint32_t flags=irq_save();
    int rc=df_stopped(NULL)?st_prog(off,in,n):-1;
    irq_restore(flags);return rc;
}
static int df_stopped(void *context)
{ (void)context;return flash_ok&&!cv_cpu_active&&!transport_busy()&&!transport_req; }
/* native_authorized must come from a separately approved completed migration,
 * not from recognizing one record. No present caller grants that policy. */
int d8p1_save_flash(unsigned object,int native_authorized)
{
    if(object>=D8POOL_OBJECTS)return D8POOL_INVALID;
    if(!native_authorized)return D8POOL_UNSUPPORTED;
    if(!flash_ok)return D8POOL_IO;
    d8pool_mapped mapped;
    d8pool_physical io={NULL,df_read,df_erase,df_program,df_stopped};
    int rc=d8pool_mapped_open(&mapped,&io,native_authorized);
    if(!rc)rc=d8p1_save_pool(&mapped.pool,object);
    d8pool_mapped_close(&mapped);return rc;
}
int d8p1_load_flash(unsigned object,int native_authorized)
{
    if(object>=D8POOL_OBJECTS)return D8POOL_INVALID;
    if(!native_authorized)return D8POOL_UNSUPPORTED;
    if(!flash_ok)return D8POOL_IO;
    d8pool_mapped mapped;
    d8pool_physical io={NULL,df_read,df_erase,df_program,df_stopped};
    int rc=d8pool_mapped_open(&mapped,&io,native_authorized);
    if(!rc)rc=d8p1_load_pool(&mapped.pool,object);
    d8pool_mapped_close(&mapped);return rc;
}
int d8p1_restore_flash_autosave(int native_authorized,int allowed)
{ return allowed?d8p1_load_flash(3,native_authorized):D8POOL_EMPTY; }

int d8p1_catalog_flash(d8p1_project_catalog *catalog,int native_authorized)
{
    if(!catalog)return D8POOL_INVALID;
    if(!native_authorized)return D8POOL_UNSUPPORTED;
    if(!flash_ok)return D8POOL_IO;
    d8pool_mapped mapped;
    d8pool_physical io={NULL,df_read,df_erase,df_program,df_stopped};
    int rc=d8pool_mapped_open(&mapped,&io,native_authorized);
    if(!rc)rc=d8p1_catalog_pool(&mapped.pool,catalog);
    d8pool_mapped_close(&mapped);return rc;
}
int d8p1_rename_flash(unsigned object,const char *name,int native_authorized)
{
    if(object>=3||!name)return D8POOL_INVALID;
    if(!native_authorized)return D8POOL_UNSUPPORTED;
    if(!flash_ok)return D8POOL_IO;
    d8pool_mapped mapped;
    d8pool_physical io={NULL,df_read,df_erase,df_program,df_stopped};
    int rc=d8pool_mapped_open(&mapped,&io,native_authorized);
    if(!rc)rc=d8p1_rename_pool(&mapped.pool,object,name);
    d8pool_mapped_close(&mapped);return rc;
}
