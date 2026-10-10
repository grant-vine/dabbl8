/* SPDX-License-Identifier: GPL-3.0-only */
/* Canonical wire/signature equivalence to PR71's two-encode algorithm. */
#define NPART 8
#define UI_TEST_NO_MAIN 1
#include "ui_test.c"
#include "../firmware/src/d8p1_runtime.c"
static unsigned checks,failures;
static uint8_t raw[D8P1_LIMIT],expected[D8P1_LIMIT];
static size_t expected_n;
#define CHECK(x) do{checks++;if(!(x)){failures++;fprintf(stderr,"FAIL line %u: %s\n",__LINE__,#x);}}while(0)
/* Exact pre-optimization algorithm at 268981e; normal capture still emits wire. */
static int previous_signature(uint32_t *signature)
{
 if(!signature||d8ps_overlap(signature,sizeof *signature,&main_workspace,sizeof main_workspace))return D8RT_BAD;
 d8p1_stage_workspace *stage=&main_workspace.d8p1;size_t n=0;
 int rc=d8p1_capture_runtime(stage->wire,sizeof stage->wire,&n);if(rc)return rc;
 stage->state.project.sel=0;
 stage->state.project.g[G_SLOT]=stage->state.project.g[G_NAME]=stage->state.project.g[G_LOAD]=stage->state.project.g[G_SAVE]=0;
 if(!d8p1_project_encode(stage->wire,sizeof stage->wire,&n,&stage->state))return D8RT_BAD;
 *signature=as_mix(2166136261u,stage->wire,(uint32_t)n);expected_n=n;memcpy(expected,stage->wire,n);return D8RT_OK;
}
static void equivalent(int wanted)
{
 uint32_t a=0xa5a5a5a5,b=a;int before=previous_signature(&a),after=d8p1_signature_runtime(&b);
 CHECK(before==wanted&&after==before);CHECK(a==b);
 if(!before)CHECK(!memcmp(expected,main_workspace.d8p1.wire,expected_n));
 else CHECK(a==0xa5a5a5a5&&b==0xa5a5a5a5);
}
static void load(const char*path)
{
 FILE*f=fopen(path,"rb");if(!f)exit(2);size_t n=fread(raw,1,sizeof raw,f);CHECK(!ferror(f)&&fgetc(f)==EOF);fclose(f);
 CHECK(d8p1_load_runtime(raw,n,15)==D8RT_OK);int32_t out[CTL*2];mix_block(out,CTL);
}
int main(void)
{
 ui_power_on();int32_t out[CTL*2];mix_block(out,CTL);
 load("tests/fixtures/d8p1/minimal.d8p");equivalent(D8RT_OK);
 load("tests/fixtures/d8p1/maximum.d8p");equivalent(D8RT_OK);
 /* Every selected-track value, including invalid values before normalization. */
 uint8_t selection=song.sel;for(unsigned i=0;i<256;i++){song.sel=(uint8_t)i;equivalent(i<8?D8RT_OK:D8RT_BAD);}song.sel=selection;
 step_t saved=trk[7].step[63],*s=&trk[7].step[63];
 for(unsigned i=0;i<5;i++){s->n=(uint8_t)i;equivalent(D8RT_OK);}*s=saved;
 for(unsigned i=0;i<=ST_REST;i++){s->time=(uint8_t)i;equivalent(D8RT_OK);}*s=saved;
 for(unsigned i=0;i<4;i++){s->flags=(uint8_t)i;equivalent(D8RT_OK);step_set_ratchet(s,i+1);equivalent(D8RT_OK);}*s=saved;
 for(unsigned i=0;i<128;i++){s->vel=(uint8_t)i;equivalent(D8RT_OK);}*s=saved;
 for(unsigned i=0;i<256;i++){s->hit=(uint8_t)i;s->acc=(uint8_t)(i&0xa5);equivalent(D8RT_OK);}*s=saved;
 for(unsigned i=0;i<=101;i++){s->probability=(uint8_t)i;equivalent(D8RT_OK);}*s=saved;
 /* Native-invalid captures must still refuse; caller signatures stay untouched. */
 s->n=5;equivalent(D8RT_BAD);*s=saved;
 int16_t p=trk[7].p[P_LEVEL];trk[7].p[P_LEVEL]=128;equivalent(D8RT_BAD);trk[7].p[P_LEVEL]=p;
 uint8_t eng=trk[7].eng_req;trk[7].eng_req=14;equivalent(D8RT_BAD);trk[7].eng_req=eng;
 char name=proj_name[0];proj_name[0]=1;equivalent(D8RT_BAD);proj_name[0]=name;
 d8p1_arrangement *a=&d8p1_runtime_cache.arrangement;
 uint8_t track=a->bank[0].track[7];a->bank[0].track[7]=8;equivalent(D8RT_BAD);a->bank[0].track[7]=track;
 uint8_t count=motion.count;motion.count=65;equivalent(D8RT_BAD);motion.count=count;
 chain_config.count=1;equivalent(D8RT_BAD);chain_config.count=0;
 song.playing=1;equivalent(D8RT_BUSY);song.playing=0;transport_req=1;equivalent(D8RT_BUSY);transport_req=0;
 /* Short and overlapping normal wire outputs remain unchanged on refusal. */
 size_t written=0x1234;memset(raw,0xa5,sizeof raw);CHECK(d8p1_capture_runtime(raw,1,&written)==D8RT_BAD&&written==0x1234&&raw[0]==0xa5);
 CHECK(d8p1_capture_runtime((uint8_t*)&main_workspace.d8p1.state,1,&written)==D8RT_BAD&&written==0x1234);
 CHECK(d8p1_signature_runtime((uint32_t*)((uint8_t*)&main_workspace+sizeof main_workspace-2))==D8RT_BAD);
 equivalent(D8RT_OK);
 printf("Native signature equivalence: %u checks, %u failures; full canonical wire, pre-normalization validity, step fields and preserved refusals\n",checks,failures);
 return failures!=0;
}
