/* SPDX-License-Identifier: GPL-3.0-only */
/* Bounded actual UI preview; virtual flash/LCD only. */
#define DABBL8_IDENTITY 1
#define FELUCCA_VERSION "v0.1.0"
#define D8FLASH_RUNTIME_NO_MAIN 1
#include "d8p1_flash_runtime_test.c"
static int ppm(const char *dir,unsigned pal,unsigned scene){
 char path[1024];snprintf(path,sizeof path,"%s/p%u-s%u.ppm",dir,pal,scene);
 FILE *f=fopen(path,"wb");if(!f)return 1;fprintf(f,"P6\n240 240\n255\n");
 for(unsigned i=0;i<240*240;i++){uint16_t v=swap16(host_screen[i]);unsigned char rgb[3]={(unsigned char)(((v>>11)&31)*255/31),(unsigned char)(((v>>5)&63)*255/63),(unsigned char)((v&31)*255/31)};fwrite(rgb,1,3,f);}fclose(f);return 0;
}
int main(int argc,char **argv){if(argc!=2)return 2;
 for(unsigned p=0;p<NPALETTES;p++)for(unsigned large=0;large<2;large++)for(unsigned sel=0;sel<8;sel++)for(unsigned sc=0;sc<3;sc++){
  ui_power_on();palette_set(p);song.sel=sel;if(large)ui_prefs|=PREF_LARGE;
  song.rec=1u<<sel;song.g[G_BPM]=124;
  for(unsigned t=0;t<8;t++){trk[t].p[P_LEVEL]=80+t*5;trk[t].p[P_MUTE]=t==6;trk[t].peak=12000+t*1200;my_steps(&trk[t]);}
  for(unsigned i=0;i<SCOPE_N;i++)scope_buf[i]=(int16_t)(((i*37u)%128u)*160-10240);
  if(sc==1)go_page(GR_TRK);if(sc==2)go_page(GR_ROLL);
  ui.force=1;ui_draw();lcd_sync();CHECK(!dma.p&&!dma_errors&&!cv_cpu_active);
  if(p==UI_GREY_INDEX||p==UI_BW_INDEX)for(unsigned t=0;t<8;t++)CHECK(track_ink(t)==T_THEME);
  if(sel==4&&(p==10||p==11))if(ppm(argv[1],p,sc+large*3u))return 3;
 }
 printf("colour UI: 576 frames, checks=%u failures=%u dma_errors=%u\n",checks,failures,dma_errors);return failures?1:0;
}
