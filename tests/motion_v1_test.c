/* SPDX-License-Identifier: GPL-3.0-only */
#define main hostsim_main
#include "hostsim.c"
#undef main
static int reject(const uint8_t *b, unsigned n) {
 motion_store_t out; memset(&out,0xa5,sizeof out); motion_store_t before=out;
 return !motion_v1_decode(&out,b,n) && !memcmp(&out,&before,sizeof out);
}
int main(void) {
 int bad=0; uint8_t b[MOTION_V1_MAX],c[MOTION_V1_MAX];
 for(unsigned kind=0;kind<2;kind++) for(unsigned batch=0;batch<8;batch++) {
  motion_store_t m;memset(&m,0,sizeof m);m.count=64;m.on=255;
  for(unsigned i=0;i<64;i++) {
   /* Adjacent addresses spanning all tracks share each pool batch. */
   unsigned address=batch*64+i;
   m.event[i]=(motion_event_t){address,(uint8_t)(P_REV|(kind?MOTION_LOCK:0)),(int16_t)((int)(address%192)-64)};
  }
  motion_store_t out;memset(&out,0,sizeof out);unsigned n=motion_v1_encode(b,sizeof b,&m);
  bad+=n!=328 || !motion_v1_decode(&out,b,n);
  for(unsigned i=0;i<64;i++) bad+=out.event[i].place!=m.event[i].place || out.event[i].param!=m.event[i].param || out.event[i].value!=m.event[i].value;
  for(unsigned len=0;len<n;len++)bad+=!reject(b,len);
  memcpy(c,b,n);c[3]='2';bad+=!reject(c,n);
  memcpy(c,b,n);c[6]=1;bad+=!reject(c,n);
  memcpy(c,b,n);c[4]=65;bad+=!reject(c,n);
  memcpy(c,b,n);c[8]=8;bad+=!reject(c,n);
  memcpy(c,b,n);c[9]=64;bad+=!reject(c,n);
  memcpy(c,b,n);c[10]=127;bad+=!reject(c,n);
  memcpy(c,b,n);c[11]=128;c[12]=0;bad+=!reject(c,n);
  memcpy(c,b,n);c[11]=191;c[12]=255;bad+=!reject(c,n);
  memcpy(c,b,n);memcpy(c+13,c+8,5);c[15]^=MOTION_LOCK;bad+=!reject(c,n);
  memset(c,0xa5,sizeof c);bad+=motion_v1_encode(c,n-1,&m)!=0 || c[0]!=0xa5;
  if(batch>=4) {uint8_t legacy[260];memset(legacy,0xa5,sizeof legacy);bad+=motion_legacy_encode(legacy,&m)!=0 || legacy[0]!=0xa5;}
 }
 motion_store_t empty;memset(&empty,0,sizeof empty);unsigned n=motion_v1_encode(b,sizeof b,&empty);
 bad+=n!=8 || !motion_v1_decode(&empty,b,n);
 empty.count=1;empty.event[0]=(motion_event_t){512,P_REV,0};bad+=motion_v1_encode(b,sizeof b,&empty)!=0;
 printf("D8M1: all 512 addresses, automation and locks, pool batches, malformed records and refusal without mutation: %s\n",bad?"FAIL":"PASS");return !!bad;
}
