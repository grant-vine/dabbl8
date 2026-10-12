/* SPDX-License-Identifier: GPL-3.0-only */
#include "felucca_web.c"
static unsigned checks,failures;
static uint8_t owned_nor[sizeof nor];
#define CHECK(x) do{checks++;if(!(x)){failures++;printf("FAIL line%d: %s\n",__LINE__,#x);}}while(0)
int main(void){
 web_nor_erase();web_boot();CHECK(d8fi.phase==FI_REQUIRED);CHECK(!web_flash_writes);
 for(unsigned i=0;i<1000;i++)web_ms();CHECK(d8fi.shown);CHECK(!web_flash_writes);
 web_buttons(1u<<B_SAVE);for(unsigned i=0;i<20;i++)web_ms();CHECK(d8fi.phase==FI_RELEASE);CHECK(!web_flash_writes);
 web_buttons(0);for(unsigned i=0;i<30000&&d8fi.phase!=FI_IDLE&&d8fi.phase!=FI_FAILED;i++)web_ms();
 CHECK(d8fi.phase==FI_IDLE);CHECK(project_native_status()==1);CHECK(proj_cur==0);CHECK(native_as.ready);CHECK(web_flash_writes);CHECK(!d8_instrument_write_allowed());CHECK(!mc_write_enabled());
 unsigned writes=web_flash_writes;uint32_t crc=d8p1_crc32(nor,sizeof nor);web_power_on();for(unsigned i=0;i<1000;i++)web_ms();CHECK(d8fi.phase==FI_IDLE);CHECK(project_native_status()==1);CHECK(web_flash_writes==writes);CHECK(d8p1_crc32(nor,sizeof nor)==crc);CHECK(proj_cur==0);
 memcpy(owned_nor,nor,sizeof nor);
 /* Valid own header with unsupported version must preserve, never format. */
 uint8_t*h=nor+d8pool_mapped_address(0);h[4]=99;
 uint32_t hc=d8p1_crc32(h,28);for(unsigned i=0;i<4;i++)h[28+i]=(uint8_t)(hc>>(8*i));
 crc=d8p1_crc32(nor,sizeof nor);web_power_on();for(unsigned i=0;i<1000;i++)web_ms();
 CHECK(d8fi.phase==FI_IDLE);CHECK(project_native_status()==2);CHECK(web_flash_writes==writes);CHECK(d8p1_crc32(nor,sizeof nor)==crc);
 /* Own committed header, damaged payload: preservation/refusal is read-only. */
 memcpy(nor,owned_nor,sizeof nor);nor[d8pool_mapped_address(0)+D8POOL_HEADER]^=1;
 crc=d8p1_crc32(nor,sizeof nor);web_power_on();for(unsigned i=0;i<1000;i++)web_ms();
 CHECK(d8fi.phase==FI_IDLE);CHECK(project_native_status()==2);CHECK(web_flash_writes==writes);CHECK(d8p1_crc32(nor,sizeof nor)==crc);
 printf("Emulator actual blank first-use: %u checks %u failures\n",checks,failures);return failures!=0;
}
