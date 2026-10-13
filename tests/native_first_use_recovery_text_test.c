/* SPDX-License-Identifier: GPL-3.0-only */
static void firstuse_text_hook(const char *);
#define GFX_HOOK_TEXT(x0,y0,x1,y1,s,flags) firstuse_text_hook(s)
#define GFX_HOOK_BEGIN() ((void)0)
#define GFX_HOOK_BLIT(x,y,r0) ((void)0)
#define GFX_HOOK_PIXELS(n) ((void)0)
#define D8_FIRST_USE_SESSION_NO_MAIN 1
#include "native_first_use_session_test.c"
static unsigned saw_save,saw_recovery;
static void firstuse_text_hook(const char *s)
{
 if(strstr(s,"PRESS AND RELEASE SAVE"))saw_save++;
 if(strstr(s,"REBOOT FOR RECOVERY"))saw_recovery++;
}
int main(void)
{
 cold();project_native_reset();flash_ok=1;usb.config=0;usb.ota_req=usb.uboot_req=0;
 memset(&d8mc,0,sizeof d8mc);memset(&d8mc_edit,0,sizeof d8mc_edit);d8ls_reset();
 int32_t audio[CTL*2];mix_block(audio,CTL);reset();physical_calls=0;
 const unsigned phases[]={FI_REQUIRED,FI_UNCERTAIN,FI_FAILED,FI_REFUSED};
 for(unsigned i=0;i<4;i++) {
  memset(&d8fi,0,sizeof d8fi);d8fi.phase=(uint8_t)phases[i];saw_save=saw_recovery=0;
  d8fi_draw();CHECK(i<2?saw_save==1&&!saw_recovery:saw_recovery==1&&!saw_save);
  CHECK(!physical_calls&&!writes);
  if(i>=2)CHECK(!d8fi.shown&&d8fi.phase==phases[i]);
 }
 printf("First-use recovery text: %u checks, %u failures; four actual LCD draw phases only\n",checks,failures);
 return failures!=0;
}
