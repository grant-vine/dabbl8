/* SPDX-License-Identifier: GPL-3.0-only */
#include "d8engine_policy.h"
#include <limits.h>
#include <stdio.h>
#include <string.h>
static unsigned long checks;
#define CHECK(x) do { ++checks; if(!(x)){fprintf(stderr,"failed line %d\n",__LINE__);return 1;} } while(0)
int main(void)
{
    /* Independent public-contract fixtures, including inherited UI ordering. */
    const unsigned ids[]={0,2,3,4,5,6,7,8,9,10,11,12,13};
    const unsigned display[]={0,12,2,3,4,5,6,7,8,9,11,13,10};
    const unsigned presets[]={0,4,UINT_MAX};
    CHECK(D8ENGINE_ID_LIMIT==14 && D8CARD_ALL==0x3ffdu);
    for(unsigned bits=1;bits<8192;bits++){
        unsigned mask=0,visible[13],count=0;
        for(unsigned i=0;i<13;i++)if(bits&(1u<<i))mask|=1u<<ids[i];
        for(unsigned i=0;i<13;i++)if(mask&(1u<<display[i]))visible[count++]=display[i];
        CHECK(d8engine_count(mask)==count);
        for(unsigned pos=0;pos<count;pos++){
            unsigned out=UINT_MAX;
            CHECK(d8engine_visible(&out,mask,pos)==D8ENGINE_OK && out==visible[pos]);
            CHECK(d8engine_step(&out,mask,visible[pos],1)==D8ENGINE_OK && out==visible[(pos+1)%count]);
            CHECK(d8engine_step(&out,mask,visible[pos],-1)==D8ENGINE_OK && out==visible[(pos+count-1)%count]);
        }
        unsigned out=0xa5a5a5a5u;
        CHECK(d8engine_visible(&out,mask,count)==D8ENGINE_INVALID_REQUEST && out==0xa5a5a5a5u);
        CHECK(d8engine_visible(&out,mask,UINT_MAX)==D8ENGINE_INVALID_REQUEST && out==0xa5a5a5a5u);
        for(unsigned id=0;id<=14;id++)for(unsigned source=0;source<3;source++)for(unsigned p=0;p<3;p++){
            unsigned target=id;
            int expected=D8ENGINE_OK;
            if(id==14 || (source==D8ENGINE_CANONICAL && id==1))expected=D8ENGINE_INVALID_REQUEST;
            else {
                if(id==1)target=12;
                if(source==D8ENGINE_LEGACY_FACTORY && id==4 && presets[p]==4)target=10;
                if(!(mask&(1u<<target)))expected=D8ENGINE_UNAVAILABLE;
            }
            out=0xa5a5a5a5u;
            CHECK(d8engine_target(&out,mask,id,presets[p],source)==expected);
            CHECK(out==(expected?0xa5a5a5a5u:target));
        }
        for(unsigned i=0;i<13;i++){
            unsigned current=ids[i];out=UINT_MAX;
            if(!(mask&(1u<<current)))CHECK(d8engine_step(&out,mask,current,-1)==D8ENGINE_OK && out==visible[0]);
            d8engine_sound sound={UINT_MAX,UINT_MAX};
            CHECK(d8engine_default(&sound,mask,current,0)==D8ENGINE_OK);
            CHECK(sound.engine==((mask&(1u<<current))?current:visible[0]) && sound.preset==0);
        }
    }
    /* Migration checks use the effective target, not the source's mask bit. */
    unsigned out=99;
    CHECK(d8engine_target(&out,1u<<12,1,7,D8ENGINE_LEGACY_FACTORY)==D8ENGINE_OK && out==12);
    out=99;
    CHECK(d8engine_target(&out,D8CARD_ALL&~(1u<<12),1,7,D8ENGINE_LEGACY_FACTORY)==D8ENGINE_UNAVAILABLE && out==99);
    CHECK(d8engine_target(&out,1u<<10,4,4,D8ENGINE_LEGACY_FACTORY)==D8ENGINE_OK && out==10);
    out=99;
    CHECK(d8engine_target(&out,1u<<4,4,4,D8ENGINE_LEGACY_FACTORY)==D8ENGINE_UNAVAILABLE && out==99);
    CHECK(d8engine_target(&out,1u<<4,4,4,D8ENGINE_LEGACY_SELECT)==D8ENGINE_OK && out==4);
    CHECK(d8engine_target(&out,1u<<4,4,4,D8ENGINE_CANONICAL)==D8ENGINE_OK && out==4);
    d8engine_sound sound={99,98};
    CHECK(d8engine_default(&sound,D8CARD_ALL,4,4)==D8ENGINE_OK && sound.engine==10 && sound.preset==0);
    CHECK(d8engine_default(&sound,1u<<4,4,4)==D8ENGINE_OK && sound.engine==4 && sound.preset==0);
    CHECK(d8engine_default(&sound,1u<<12,1,7)==D8ENGINE_OK && sound.engine==12 && sound.preset==7);
    CHECK(d8engine_default(&sound,1u<<3,1,7)==D8ENGINE_OK && sound.engine==3 && sound.preset==0);
    /* Historical default engines/preset numbers stay identical with all engines. */
    const unsigned boot_engine[]={0,12,3,10,0,0,0,0},boot_preset[]={4,4,0,0,0,0,0,0};
    for(unsigned i=0;i<8;i++)CHECK(d8engine_default(&sound,D8CARD_ALL,boot_engine[i],boot_preset[i])==D8ENGINE_OK && sound.engine==boot_engine[i] && sound.preset==boot_preset[i]);
    const unsigned badmask[]={0,2,D8CARD_ALL|2,D8CARD_ALL|0x4000u,UINT_MAX};
    for(unsigned i=0;i<sizeof badmask/sizeof badmask[0];i++){
        out=99;sound=(d8engine_sound){99,98};
        CHECK(d8engine_count(badmask[i])==0);
        CHECK(d8engine_target(&out,badmask[i],0,0,D8ENGINE_LEGACY_SELECT)==D8ENGINE_INVALID_MASK && out==99);
        CHECK(d8engine_visible(&out,badmask[i],0)==D8ENGINE_INVALID_MASK && out==99);
        CHECK(d8engine_step(&out,badmask[i],0,1)==D8ENGINE_INVALID_MASK && out==99);
        CHECK(d8engine_default(&sound,badmask[i],0,0)==D8ENGINE_INVALID_MASK && sound.engine==99 && sound.preset==98);
    }
    const unsigned badid[]={14,UINT_MAX};
    for(unsigned i=0;i<2;i++){
        out=99;sound=(d8engine_sound){99,98};
        CHECK(d8engine_target(&out,D8CARD_ALL,badid[i],0,D8ENGINE_LEGACY_SELECT)==D8ENGINE_INVALID_REQUEST && out==99);
        CHECK(d8engine_step(&out,D8CARD_ALL,badid[i],1)==D8ENGINE_INVALID_REQUEST && out==99);
        CHECK(d8engine_default(&sound,D8CARD_ALL,badid[i],0)==D8ENGINE_INVALID_REQUEST && sound.engine==99 && sound.preset==98);
    }
    out=99;
    CHECK(d8engine_target(&out,D8CARD_ALL,0,0,UINT_MAX)==D8ENGINE_INVALID_REQUEST && out==99);
    CHECK(d8engine_step(&out,D8CARD_ALL,1,1)==D8ENGINE_INVALID_REQUEST && out==99);
    CHECK(d8engine_step(&out,D8CARD_ALL,0,0)==D8ENGINE_INVALID_REQUEST && out==99);
    CHECK(d8engine_step(&out,D8CARD_ALL,0,INT_MIN)==D8ENGINE_INVALID_REQUEST && out==99);
    CHECK(d8engine_target(NULL,D8CARD_ALL,0,0,D8ENGINE_LEGACY_SELECT)==D8ENGINE_INVALID_REQUEST);
    CHECK(d8engine_visible(NULL,D8CARD_ALL,0)==D8ENGINE_INVALID_REQUEST);
    CHECK(d8engine_step(NULL,D8CARD_ALL,0,1)==D8ENGINE_INVALID_REQUEST);
    CHECK(d8engine_default(NULL,D8CARD_ALL,0,0)==D8ENGINE_INVALID_REQUEST);
    printf("engine availability policy: %lu checks passed; helpers only, no runtime adoption\n",checks);return 0;
}
