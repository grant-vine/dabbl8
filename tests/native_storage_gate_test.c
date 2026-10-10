/* SPDX-License-Identifier: GPL-3.0-only */
#define NPART 8
#define main legacy_storage_main
#include "storage_test.c"
#undef main
static uint8_t before[sizeof nor];
int main(void) {
 int bad=0;memset(nor,255,sizeof nor);const char data[]="preserved";
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
