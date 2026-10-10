/* SPDX-License-Identifier: GPL-3.0-only
 * Engine identities derive from Felucca, Copyright (C) 2026 Leo Kuroshita
 * (@kurogedelic), Hügelton Instruments. */
#include "d8card.h"
static const d8card_engine engines[]={
#define D8_ENGINE(name,id,components) {id,components,#name},
#include "d8card.def"
#undef D8_ENGINE
};
_Static_assert(sizeof engines/sizeof engines[0]==D8CARD_ENGINES,"registry count");
const d8card_engine *d8card_engine_by_id(unsigned id)
{
    for(unsigned i=0;i<D8CARD_ENGINES;i++)if(engines[i].id==id)return &engines[i];
    return NULL;
}
int d8card_validate(unsigned mask,unsigned components)
{
    if(!mask || (mask&~D8CARD_ALL) || (components&~7u))return D8CARD_INVALID_PROFILE;
    unsigned need=0;
    for(unsigned i=0;i<D8CARD_ENGINES;i++)if(mask&(1u<<engines[i].id))need|=engines[i].components;
    return (need&~components)?D8CARD_MISSING_COMPONENT:D8CARD_OK;
}
int d8card_preflight(d8card_report *out,const void *raw,size_t n,unsigned mask,unsigned components)
{
    d8p1_view view;
    int rc=d8card_validate(mask,components);
    if(rc)return rc;
    if(!out)return D8CARD_INVALID_PROJECT;
    /* Publishing into input storage would damage the original project. */
    uintptr_t a=(uintptr_t)out,b=(uintptr_t)raw;
    if(raw&&n&&(a<=b?b-a<sizeof *out:a-b<n))return D8CARD_INVALID_PROJECT;
    rc=d8p1_read(&view,raw,n);
    if(!rc)return D8CARD_INVALID_PROJECT;
    if(rc!=1)return D8CARD_UNSUPPORTED_PROJECT;
    unsigned required=0;
    for(unsigned i=0;i<view.count;i++)if((view.chunk[i].type&0x7fffu)==2u) {
        const uint8_t *tracks=view.chunk[i].data;
        for(unsigned t=0;t<8;t++) {
            unsigned id=tracks[t*677u+99u];
            if(!d8card_engine_by_id(id))return D8CARD_UNSUPPORTED_PROJECT;
            required|=1u<<id;
        }
    }
    d8card_report report={required,required&~mask};
    *out=report;
    return report.missing?D8CARD_MISSING_ENGINE:D8CARD_OK;
}
