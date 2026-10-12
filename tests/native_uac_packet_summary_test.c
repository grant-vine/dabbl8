/* SPDX-License-Identifier: GPL-3.0-only */
/* Packet-summary regression using actual GPL queue/runtime fixtures. */
#define D8OUTPUT_QUEUE_NO_MAIN 1
#include "native_output_queue_test.c"
#include "fixtures/uac/e5-uac-packet.inc"
struct packet_state {
    __typeof__(uac) service;
    uint32_t read, write, ring[UA_N], occupied, history;
    uint8_t packet, uncertain;
};
static struct packet_state initial, expected, actual;
static void take(struct packet_state *s) {
    memset(s,0,sizeof *s);memcpy(&s->service,&uac,sizeof uac);
    s->read=ua_r;s->write=ua_w;memcpy(s->ring,ua_ring,sizeof ua_ring);
    s->occupied=uq.ring;s->history=uq.history;s->packet=uq.packet;s->uncertain=uq.uncertain;
}
static void restore(const struct packet_state *s) {
    memcpy(&uac,&s->service,sizeof uac);ua_r=s->read;ua_w=s->write;
    memcpy(ua_ring,s->ring,sizeof ua_ring);uq.ring=s->occupied;uq.history=s->history;
    uq.packet=s->packet;uq.uncertain=s->uncertain;
}
int main(void) {
    unsigned cases=0;cold();nest_enabled=0;
    for(unsigned rate=0;rate<2;rate++)for(unsigned go=0;go<2;go++)
    for(unsigned match=0;match<2;match++)for(unsigned phase=0;phase<2;phase++)
    for(unsigned band=0;band<4;band++)for(unsigned fill=0;fill<=64;fill++)
    for(unsigned mode=0;mode<6;mode++)for(unsigned pos=0;pos<3;pos++) {
        uint32_t ref[UA_MAXF+2],dst[UA_MAXF+2],standalone[UA_MAXF+2];
        memset(&uac,0,sizeof uac);memset(&uq,0,sizeof uq);memset(ua_ring,0,sizeof ua_ring);
        uac.r48=rate;uac.ring48=match?rate:!rate;uac.go=go;uac.acc=phase?999:0;
        uac.fill_min=band==0?0:band==1?(rate?UA_LO48:UA_LO):band==2?(rate?UA_HI48:UA_HI):UA_N;
        ua_r=pos==0?0:pos==1?511:UINT32_MAX-16u;ua_w=ua_r+fill;
        for(unsigned i=0;i<UA_N;i++) {
            unsigned relative=(i-ua_r)&(UA_N-1u);
            ua_ring[i]=mode==0?0:mode==1?0x12348000u+i:mode==2?(relative==0?1:0):mode==3?(fill&&relative==fill-1?0x80000000u:0):mode==4?(relative>=fill?0x00ff0000u:0):(relative&1?0:0xffff8000u);
            uq.ring+=ua_ring[i]!=0;
        }
        uq.history=7;uq.packet=1;uq.uncertain=1;uac.last=mode&1?0x12340000u:0;
        take(&initial);memset(ref,0xa5,sizeof ref);unsigned nr=uac_packet_reference(ref);take(&expected);
        CHECK(nr>=43&&nr<=UA_MAXF);uint32_t reduction=0;for(unsigned i=0;i<nr;i++)reduction|=ref[i];
        restore(&initial);memset(dst,0xa5,sizeof dst);uint32_t summary=0xdeadbeefu;
        unsigned na=uac_packet_summary(dst,&summary);take(&actual);
        CHECK(nr==na);CHECK(!memcmp(ref,dst,sizeof ref));CHECK(summary==reduction);
        CHECK(!memcmp(&expected,&actual,sizeof actual));CHECK(uq.packet==1&&uq.uncertain==1);
        restore(&initial);memset(standalone,0xa5,sizeof standalone);unsigned ns=uac_packet(standalone);take(&actual);
        CHECK(ns==nr);CHECK(!memcmp(ref,standalone,sizeof ref));CHECK(!memcmp(&expected,&actual,sizeof actual));
        cases++;
    }
    printf("Packet summary: %u cases, %u checks, %u failures; frozen actual parent, full packet/state/sentinel oracle; no hardware timing claim\n",cases,checks,failures);
    return failures!=0;
}
