/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual UI renderer with asynchronous LCD simulation; no device access. */
#define DABBL8_IDENTITY 1
#define FELUCCA_VERSION "v0.1.0"
#define D8FLASH_RUNTIME_NO_MAIN 1
#include "d8p1_flash_runtime_test.c"
int main(int argc,char **argv){
 if(argc!=2)return 2;
 for(unsigned pal=0;pal<NPALETTES;pal++){
  ui_power_on();palette_set(pal);draw_splash();
  CHECK(!dma.p&&!dma_errors);CHECK(!cv_cpu_active);
  unsigned ink=0;
  for(unsigned y=95;y<145;y++)for(unsigned x=40;x<200;x++)ink+=host_screen[y*240+x]!=swap16(T_RAISE);
  CHECK(ink>100);
  if(pal==UI_DABBL_DARK_INDEX){
   FILE *f=fopen(argv[1],"wb");if(!f)return 3;fprintf(f,"P6\n240 240\n255\n");
   for(unsigned i=0;i<240*240;i++){uint16_t v=swap16(host_screen[i]);unsigned char p[3]={(unsigned char)(((v>>11)&31)*255/31),(unsigned char)(((v>>5)&63)*255/63),(unsigned char)((v&31)*255/31)};fwrite(p,1,3,f);}fclose(f);
  }
 }
 printf("boot wordmark: palettes=%u checks=%u failures=%u dma_errors=%u\n",NPALETTES,checks,failures,dma_errors);
 return failures?1:0;
}
