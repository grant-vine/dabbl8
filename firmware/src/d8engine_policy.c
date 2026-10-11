/* SPDX-License-Identifier: GPL-3.0-only */
/* Display order and historical conversions derive from Felucca, Copyright
 * (C) 2026 Leo Kuroshita (@kurogedelic), Huegelton Instruments. */
#include "d8engine_policy.h"
/* This ordering must retain the full-profile UI order. IDs remain stable. */
static const unsigned order[]={0,12,2,3,4,5,6,7,8,9,11,13,10};
_Static_assert(sizeof order/sizeof order[0]==D8CARD_ENGINES,"display count");
static int mask_ok(unsigned mask){return mask && !(mask&~D8CARD_ALL);}
static int id_ok(unsigned id){return id<D8ENGINE_ID_LIMIT && id!=D8ENGINE_DIGITAL;}
int d8engine_target(unsigned *out,unsigned mask,unsigned requested,unsigned preset,unsigned source)
{
    if(!mask_ok(mask))return D8ENGINE_INVALID_MASK;
    if(!out || requested>=D8ENGINE_ID_LIMIT || source>D8ENGINE_LEGACY_FACTORY ||
       (source==D8ENGINE_CANONICAL && requested==D8ENGINE_DIGITAL))return D8ENGINE_INVALID_REQUEST;
    unsigned target=requested;
    if(source!=D8ENGINE_CANONICAL && requested==D8ENGINE_DIGITAL)target=D8ENGINE_FM6;
    if(source==D8ENGINE_LEGACY_FACTORY && requested==D8ENGINE_SAMPLE && preset==D8ENGINE_PERC_PRESET)target=D8ENGINE_DRUM;
    if(!(mask&(1u<<target)))return D8ENGINE_UNAVAILABLE;
    *out=target;return D8ENGINE_OK;
}
unsigned d8engine_count(unsigned mask)
{
    if(!mask_ok(mask))return 0;
    unsigned count=0;
    for(unsigned i=0;i<D8CARD_ENGINES;i++)if(mask&(1u<<order[i]))count++;
    return count;
}
int d8engine_visible(unsigned *out,unsigned mask,unsigned position)
{
    if(!mask_ok(mask))return D8ENGINE_INVALID_MASK;
    if(!out)return D8ENGINE_INVALID_REQUEST;
    for(unsigned i=0;i<D8CARD_ENGINES;i++)if(mask&(1u<<order[i])){
        if(!position){*out=order[i];return D8ENGINE_OK;}position--;
    }
    return D8ENGINE_INVALID_REQUEST;
}
int d8engine_step(unsigned *out,unsigned mask,unsigned current,int dir)
{
    if(!mask_ok(mask))return D8ENGINE_INVALID_MASK;
    if(!out || !id_ok(current) || (dir!=-1 && dir!=1))return D8ENGINE_INVALID_REQUEST;
    unsigned rank=0,count=d8engine_count(mask);
    if(!(mask&(1u<<current)))return d8engine_visible(out,mask,0);
    for(unsigned i=0;i<D8CARD_ENGINES;i++)if(mask&(1u<<order[i])){
        if(order[i]==current)break;rank++;
    }
    return d8engine_visible(out,mask,(rank+(dir>0?1:count-1))%count);
}
int d8engine_default(d8engine_sound *out,unsigned mask,unsigned requested,unsigned preset)
{
    if(!out)return D8ENGINE_INVALID_REQUEST;
    unsigned target;
    int rc=d8engine_target(&target,mask,requested,preset,D8ENGINE_LEGACY_FACTORY);
    if(rc==D8ENGINE_UNAVAILABLE){rc=d8engine_visible(&target,mask,0);preset=0;}
    if(rc)return rc;
    /* PERC's target is DRUM kit 0, not DRUM preset 4. DIGITAL's number must
     * remain intact so the caller migrates the original four-op preset. */
    if(requested==D8ENGINE_SAMPLE && target==D8ENGINE_DRUM && preset==D8ENGINE_PERC_PRESET)preset=0;
    d8engine_sound result={target,preset};*out=result;return D8ENGINE_OK;
}
