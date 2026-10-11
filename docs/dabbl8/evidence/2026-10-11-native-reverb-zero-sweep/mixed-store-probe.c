/* SPDX-License-Identifier: GPL-3.0-only */
#define D8FLASH_RUNTIME_NO_MAIN 1
#include TREE_RUNTIME
int main(void){
 ui_power_on();memset(&fx,0,sizeof fx);rev_clear();fx.rtype=1;
 int32_t input[1]={16384},output[1]={0};rev_spring(input,output,1);
 CHECK(rev_comb[0]!=0&&rev_u.sp[0]!=0&&!fx_tail.comb&&!fx_tail.ap);
 FT16(comb,&rev_comb[0],0);
 FT16(comb,&rev_comb[sizeof rev_comb/2-1],1);
 CHECK(fx_tail.comb!=0);
 FT32(ap,&rev_u.sp[0],0);
 FT32(ap,&rev_u.sp[sizeof rev_u.sp/4-1],1);
 CHECK(fx_tail.ap!=0);
 printf("Mixed reverb stores: %u checks, %u failures; actual production stores then external updates\\n",checks,failures);
 return failures!=0;
}
