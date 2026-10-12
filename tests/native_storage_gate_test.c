/* SPDX-License-Identifier: GPL-3.0-only */
#define NPART 8
#define main legacy_storage_main
#include "storage_test.c"
#undef main
static uint8_t before[sizeof nor];
int main(void) {
 int bad=0;memset(nor,255,sizeof nor);const char data[]="preserved";
 /* Read-only boot helpers never authorize the legacy writers below. These
  * checks use synthetic NOR contents, including a valid historical envelope. */
 char got[32], untouched[sizeof got];memset(untouched,0x5a,sizeof untouched);
 memcpy(before,nor,sizeof nor);
 for(unsigned block=0;block<5;block++)bad+=check("erased project/autosave pairs allow historical fallback",st_boot_legacy_pair(block)==1);
 bad+=check("boot classification preserves every physical byte",!memcmp(before,nor,sizeof nor));
 io_calls=0;
 bad+=check("out-of-range boot pair refuses before reads",!st_boot_legacy_pair(5)&&!st_boot_legacy_pair(UINT32_MAX)&&!io_calls);
 memcpy(got,untouched,sizeof got);
 bad+=check("empty autosave read refuses with output and NOR retained",st_load_autosave(got,sizeof got)<0&&!memcmp(got,untouched,sizeof got)&&!memcmp(before,nor,sizeof nor));
 uint32_t base=st_sector(OBJ_AUTOSAVE,0);nor[base+128]=0;
 memcpy(before,nor,sizeof nor);memcpy(got,untouched,sizeof got);
 bad+=check("partial autosave ownership cannot look erased/legacy",st_boot_legacy_pair(4)==0);
 bad+=check("malformed autosave read retains output and NOR",st_load_autosave(got,sizeof got)<0&&!memcmp(got,untouched,sizeof got)&&!memcmp(before,nor,sizeof nor));
 memset(nor,255,sizeof nor);
 st_hdr_t h={.magic=ST_MAGIC,.type=OBJ_AUTOSAVE,.slot=0,.seq=1,.len=sizeof data,.crc=st_crc32(data,sizeof data)};
 h.hcrc=st_crc32(&h,sizeof h-4u);memcpy(nor+base,&h,sizeof h);memcpy(nor+base+ST_PAYLOAD_OFF,data,sizeof data);
 memcpy(before,nor,sizeof nor);memcpy(got,untouched,sizeof got);
 bad+=check("CRC-valid autosave envelope permits historical fallback",st_boot_legacy_pair(4)==1);
 bad+=check("autosave read returns exact payload, retaining tail and NOR",st_load_autosave(got,sizeof got)==(int)sizeof data&&!memcmp(got,data,sizeof data)&&!memcmp(got+sizeof data,untouched+sizeof data,sizeof got-sizeof data)&&!memcmp(before,nor,sizeof nor));
 memcpy(got,untouched,sizeof got);
 bad+=check("short autosave destination refuses without mutation",st_load_autosave(got,sizeof data-1)<0&&!memcmp(got,untouched,sizeof got)&&!memcmp(before,nor,sizeof nor));
 nor[base+ST_PAYLOAD_OFF]^=1;memcpy(before,nor,sizeof nor);
 bad+=check("corrupt autosave payload refuses historical fallback",st_boot_legacy_pair(4)==0);
 bad+=check("corrupt autosave read retains output and NOR",st_load_autosave(got,sizeof got)<0&&!memcmp(got,untouched,sizeof got)&&!memcmp(before,nor,sizeof nor));
 memset(nor,255,sizeof nor);io_calls=0;
 bad+=check("eight-track legacy write refused before native binding",st_save(OBJ_PROJECT0,data,sizeof data)!=0);
 memcpy(before,nor,sizeof nor);io_calls=0;
 for(unsigned o=OBJ_PROJECT0;o<OBJ_PROJECT0+4;o++)for(int to=-1;to<=1;to++)bad+=check("native ownership gates all legacy project copies",st_save_to(o,data,sizeof data,to)!=0);
 for(int to=-1;to<=1;to++)bad+=check("native ownership gates legacy autosave copies",st_save_to(OBJ_AUTOSAVE,data,sizeof data,to)!=0);
 bad+=check("refusal is before reads/erase/program, all physical bytes retained",!io_calls&&!memcmp(before,nor,sizeof nor));
 for(unsigned o=0;o<OBJ_COUNT;o++)bad+=check("every legacy application object refuses before IO",st_save(o,data,sizeof data)!=0);
 bad+=check("quarantine preserves all objects before reads/erase/program",!io_calls&&!memcmp(before,nor,sizeof nor));
 bad+=check("cold/unbound context cannot write the fourth legacy pair",st_save(OBJ_PROJECT0+3,data,sizeof data)!=0);
 printf("Native storage gate: %d failures; simulated legacy driver only\n",bad);return bad!=0;
}
