/* SPDX-License-Identifier: GPL-3.0-only */
/* Eight-track existing-driver adapter. No scheduler, migration or menu hook.
 * Main loop, IRQs enabled on entry: upstream helpers always restore STI and
 * do not preserve nesting. Each call mutates at most one sector/page. */
#include "d8pool_mapped.h"
static int df_read(void *context,uint32_t off,void *out,uint32_t n)
{ (void)context;return flash_ok?st_read(off,out,n):-1; }
/* Known activity/future replay sources; resident FX histories are a further
 * scheduler eligibility gate, not proved empty by this bounded check. */
static int df_automatic_quiet(void)
{
    if(!autosave_quiet()||mi_r!=mi_w||midi_in_overflow||perf_held||
       perf_latched||perf_act||pf.busy||pf.w||pf.tw||pf.next!=PF_N)return 0;
    for(unsigned t=0;t<NTRK;t++)
        if(trk[t].p[P_SLCR]==SL_STUT||slicer_busy(&trk[t])||
           (trk[t].p[P_AMODE]&&trk[t].nheld))return 0;
    return 1;
}
static int df_stopped(void *context);
static int df_erase(void *context,uint32_t off)
{
    uint32_t flags=irq_save();
    int rc=df_stopped(context)?st_erase(off):-1;
    irq_restore(flags);return rc;
}
static int df_program(void *context,uint32_t off,const void *in,uint32_t n)
{
    uint32_t flags=irq_save();
    int rc=df_stopped(context)?st_prog(off,in,n):-1;
    irq_restore(flags);return rc;
}
static int df_stopped(void *context)
{ return flash_ok&&!cv_cpu_active&&!transport_busy()&&!transport_req&&
         (!context||df_automatic_quiet()); }
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

int d8p1_save_as_flash(unsigned object,const char *name,int native_authorized)
{
    if(object>=3)return D8POOL_INVALID;
    if(!native_authorized)return D8POOL_UNSUPPORTED;
    if(!flash_ok)return D8POOL_IO;
    d8pool_mapped mapped;
    d8pool_physical io={NULL,df_read,df_erase,df_program,df_stopped};
    int rc=d8pool_mapped_open(&mapped,&io,native_authorized);
    if(!rc)rc=d8p1_save_as_pool(&mapped.pool,object,name);
    d8pool_mapped_close(&mapped);return rc;
}

/* Dedicated automatic-write policy: unlike an explicit manual save, never
 * interrupt a held key/button or sounding/releasing voice. Context is local
 * to this synchronous call; every mapped access and mutation rechecks it.
 * df_erase/program recheck under IRQ-off immediately before the driver. */
int d8p1_autosave_flash(int native_authorized)
{
    if(!native_authorized)return D8POOL_UNSUPPORTED;
    if(!flash_ok)return D8POOL_IO;
    uint8_t quiet_policy=1;
    if(!df_stopped(&quiet_policy))return D8POOL_BUSY;
    d8pool_mapped mapped;
    d8pool_physical io={&quiet_policy,df_read,df_erase,df_program,df_stopped};
    int rc=d8pool_mapped_open(&mapped,&io,native_authorized);
    if(!rc)rc=d8p1_save_pool(&mapped.pool,3);
    d8pool_mapped_close(&mapped);return rc;
}
