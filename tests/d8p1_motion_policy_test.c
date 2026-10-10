/* SPDX-License-Identifier: GPL-3.0-only */
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"
#include "../firmware/src/d8p1.h"
static uint8_t bytes[D8P1_LIMIT];
static void crc_put(uint8_t *p,uint32_t value){for(unsigned i=0;i<4;i++)p[i]=(uint8_t)(value>>(i*8));}
int main(int argc,char **argv){
 if(argc!=2)return 2;FILE *f=fopen(argv[1],"rb");if(!f)return 2;
 size_t n=fread(bytes,1,sizeof bytes,f);fclose(f);d8p1_view view;int bad=0;
 bad+=check("D8P1 reference parsed before real motion policy comparison",d8p1_read(&view,bytes,n)==1);
 if(bad)return 1;
 const d8p1_chunk *chunk=NULL;for(unsigned i=0;i<view.count;i++)if((view.chunk[i].type&32767)==4)chunk=&view.chunk[i];if(!chunk)return 2;
 motion_store_t store;uint8_t encoded[328];
 bad+=check("D8P1 motion is byte-exact with the actual D8M1 decoder/encoder",motion_v1_decode(&store,chunk->data,chunk->length)&&motion_v1_encode(encoded,sizeof encoded,&store)==chunk->length&&!memcmp(encoded,chunk->data,chunk->length));
 size_t pos=(size_t)(chunk->data-bytes);unsigned cases=0;
 for(unsigned lock=0;lock<2;lock++)for(unsigned id=0;id<P_COUNT;id++){
  bytes[pos+10]=(uint8_t)(id|(lock?128:0));crc_put(bytes+pos-4,d8p1_crc32(bytes+pos,chunk->length));memset(bytes+12,0,4);crc_put(bytes+12,d8p1_crc32(bytes,n));
  bad+=(d8p1_read(&view,bytes,n)==1)!=!!motion_param(id);cases++;
 }
 printf("D8P1 motion policy: %u IDs/kinds match actual motion_param, %d failures; no runtime adoption\n",cases,bad);return bad?1:0;
}
