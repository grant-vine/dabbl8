/* SPDX-License-Identifier: GPL-3.0-only */
/* Focused splash-only bounds/ink/credits checks using the actual native UI. */
#define NPART 8
#define DABBL8_IDENTITY 1
#define FELUCCA_VERSION "v0.1.0"
#define UI_RENDER_NO_MAIN 1
#include "ui_render.c"
int main(void){
 rep=fopen("build/painted-boot/layout-report.txt","w");if(!rep)return 2;
 nfind=nink=mono_bad=al_fail=al_lost=0;
 unsigned frames=0;
 for(unsigned p=0;p<NPALETTES;p++)for(unsigned style=0;style<2;style++)for(unsigned large=0;large<2;large++){
  large_on=large;state();pal(p);ui_style=style;style_apply();cur_name=UI_PALETTES[p].name;
  draw(S_SPLASH);lcd_sync();lint();mono_check();frames++;
 }
 fclose(rep);
 printf("boot splash layout: frames=%u findings=%u ink=%u neutral=%u alignment=%u lost=%u\n",frames,nfind,nink,mono_bad,al_fail,al_lost);
 return !!(nfind||nink||mono_bad||al_fail||al_lost);
}
