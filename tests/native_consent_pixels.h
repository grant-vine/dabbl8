/* SPDX-License-Identifier: GPL-3.0-only */
/* Real LCD pixels from valid shipping command proposals, virtual NOR only. */
static unsigned consent_visible(unsigned y, unsigned height)
{
 uint16_t bg=swap16(T_BG);unsigned visible=0;
 for(unsigned j=y;j<y+height;j++)for(unsigned x=12;x<228;x++)visible+=host_screen[j*240+x]!=bg;
 return visible;
}
static void consent_pixels(const char *screen,unsigned y1,unsigned h1,unsigned y2,unsigned h2)
{
 static const unsigned palettes[]={UI_DABBL_DARK_INDEX,UI_DABBL_LIGHT_INDEX};
 static const char *themes[]={"dark","light"};
 unsigned previous=settings.palette;
 for(unsigned i=0;i<2;i++) {
  settings.palette=palettes[i];palette_set(settings.palette);d8ls_draw();
  CHECK(d8ls.shown==d8ls.generation&&d8ls.shown);
  unsigned first=consent_visible(y1,h1),second=consent_visible(y2,h2);
  printf("PIXELS %s %s first=%u second=%u bg=%04x ink=%04x text=%04x\n",screen,themes[i],first,second,T_BG,T_INK,T_TEXT);
  CHECK(first>30);CHECK(second>30);
  const char *directory=getenv("D8_CONSENT_CAPTURE_DIR");
  if(directory) {
   char path[1024];int n=snprintf(path,sizeof path,"%s/%s-%s.rgb565",directory,screen,themes[i]);
   CHECK(n>0&&(unsigned)n<sizeof path);
   if(n>0&&(unsigned)n<sizeof path) {
    FILE *f=fopen(path,"wb");CHECK(f!=NULL);
    if(f){CHECK(fwrite(host_screen,sizeof host_screen,1,f)==1);CHECK(!fclose(f));}
   }
  }
 }
 settings.palette=previous;palette_set(previous);d8ls_draw();
}
