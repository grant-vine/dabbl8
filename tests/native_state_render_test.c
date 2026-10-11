/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual adopted native state and renderer; virtual NOR never grants device authority. */
#define NPART 8
#define UI_RENDER_NO_MAIN 1
#define UI_ASYNC_LCD 1
#include "ui_render.c"
#include "../firmware/src/d8p1_runtime.c"
#include "../firmware/src/d8p1_pool_runtime.c"
#include "../firmware/src/d8arr_prepare.c"
static uint8_t native_nor[D8POOL_BYTES], native_wire[D8P1_LIMIT];
static d8p1_project_state native_source;
static unsigned checks, failures, reads, writes, render_frames, fail_read;
static void proof(int ok,const char *why) { checks++;if(!ok){failures++;fprintf(stderr,"FAIL %s: %s\n",cur_name?cur_name:"seed",why);} }
static int nr(void*c,uint32_t a,void*p,uint32_t n){(void)c;reads++;if(fail_read||a>D8POOL_BYTES||n>D8POOL_BYTES-a)return -1;memcpy(p,native_nor+a,n);return 0;}
static int ne(void*c,uint32_t a){(void)c;if(a%4096||a>D8POOL_BYTES-4096)return -1;writes++;memset(native_nor+a,255,4096);return 0;}
static int np(void*c,uint32_t a,const void*p,uint32_t n){(void)c;if(!n||n>256||a>D8POOL_BYTES-n||(a&255)+n>256)return -1;writes++;for(unsigned i=0;i<n;i++)native_nor[a+i]&=((const uint8_t*)p)[i];return 0;}
static int nq(void*c){(void)c;return 1;}
static d8pool native_pool={NULL,nr,ne,np,nq};
static int nc(void*c,d8p1_project_catalog*out){(void)c;return d8p1_catalog_pool(&native_pool,out);}
static int ns(void*c,unsigned o,const char*n){(void)c;return d8p1_save_as_pool(&native_pool,o,n);}
static int nl(void*c,unsigned o){(void)c;return d8p1_load_pool(&native_pool,o);}
static int nn(void*c,unsigned o,const char*n){(void)c;return d8p1_rename_pool(&native_pool,o,n);}
static int na(void*c){(void)c;return d8arr_prepare_pool(&native_pool);}
static project_native_ops native_ops={NULL,nc,ns,nl,nn,na};
static void seed(void){
 ui_power_on();int32_t audio[CTL*2];mix_block(audio,CTL);transport_req=0;song.g[G_CLOCK]=0;
 memset(native_nor,255,sizeof native_nor);FILE*f=fopen("tests/fixtures/d8p1/minimal.d8p","rb");if(!f)exit(2);
 size_t n=fread(native_wire,1,sizeof native_wire,f);fclose(f);
 proof(d8p1_project_decode(&native_source,native_wire,n,0),"decode original native fixture");
 memset(&native_source.arrangement,0,sizeof native_source.arrangement);
 for(unsigned o=0;o<3;o+=2){
  memcpy(native_source.project.name,o?"UPPER BANK C":"LOWER BANK A",12);
  for(unsigned t=0;t<8;t++)native_source.project.t[t].p[P_LEVEL]=(int16_t)(90+t+o);
  proof(d8p1_project_encode(native_wire,sizeof native_wire,&n,&native_source),"encode distinct native project");
  proof(!d8pool_save(&native_pool,o,native_wire,n,5),"seed sparse actual native catalog");
 }
 d8p1_arrangement*a=&native_source.arrangement;a->banks=2;a->scenes=3;a->rows=3;
 for(unsigned b=0;b<2;b++)for(unsigned t=0;t<8;t++){a->bank[b].project[t]=(uint8_t)(b*2);a->bank[b].track[t]=(uint8_t)(7-t);}
 for(unsigned s=0;s<3;s++){
  memcpy(a->scene[s].name,s==0?"LONG SCENE A":s==1?"LONG SCENE B":"REPLACE FILL",12);
  a->scene[s].bank=(uint8_t)(s%2);a->scene[s].apply=7;a->scene[s].mute=(uint8_t)(1u<<s);
  for(unsigned t=0;t<8;t++){a->scene[s].level[t]=(uint8_t)(70+s+t);a->scene[s].pan[t]=(int8_t)((int)t-4);a->scene[s].transpose[t]=(int16_t)s;}
  a->row[s].scene=(uint8_t)s;a->row[s].repeat=(uint8_t)(s?1:2);
 }
 proof(d8p1_project_encode(native_wire,sizeof native_wire,&n,&native_source),"encode complete native arrangement");
 proof(!d8p1_load_runtime(native_wire,n,5),"adopt actual decoded native arrangement");mix_block(audio,CTL);
 proof(!project_native_bind(&native_ops,1),"explicit fixture-only catalog binding");
 proof(project_native.catalog.present==5&&d8arr_ui_rows()==3,"actual sparse catalog and arrangement available");
}
static int word(const char*s){for(unsigned i=0;i<nscr;i++)if(!strcmp(scr[i].s,s))return 1;return 0;}
static int graph_word(const char*s){for(unsigned i=0;i<nscr;i++)if(scr[i].y0>=(int)graph_y()&&!strcmp(scr[i].s,s))return 1;return 0;}
static void boundary(void){uint32_t p=div_samples(2);clk_step=15;clk_pos=p-4;events_block(4);}
static void start(void){song.g[G_CLOCK]=0;transport_req=0;events_block(1);proof(!chain_prepare()&&chain.armed,"real source-backed preparation");transport_req=0;seq_start();proof(d8arr_running()&&song.playing,"real transport starts");}
static void canvas_refusal(void){
 cur_name="CANVAS_OWNED";
 seq_stop();transport_req=0;lcd_sync();uint32_t generation=0;
 proof(!main_migration_begin(&generation),"actual exclusive workspace acquisition");
 d8mp_workspace *held=main_migration_workspace(generation);proof(held!=NULL,"actual held workspace");
 memset(held,0x69,sizeof *held);uint32_t before=d8p1_crc32(held,sizeof *held);
 unsigned r=reads,w=writes,consumed=dma_consumed,sync=sync_calls;
 ui.force=1;ui_draw();
 proof(reads==r&&writes==w&&dma_consumed==consumed&&sync_calls==sync,"owned canvas defers all drawing and storage access");
 proof(before==d8p1_crc32(held,sizeof *held)&&migration_owner==generation,"render does not corrupt held migration workspace");
 proof(!main_migration_end(generation),"actual exclusive workspace release");
}
static const char *const names[]={"PROJECT_READY","PROJECT_ERROR","PROJECT_UNBOUND","SONG_STOPPED","SONG_RUNNING","SONG_ARM","SONG_FILL","SONG_RETURN","SONG_NEXT"};
static void scene(unsigned s){
 seq_stop();transport_req=0;fail_read=0;
 proof(!project_native_bind(&native_ops,1),"rebind real catalog while stopped");
 ui.confirm=0;ui.msg_t=0;ui.layer=0;ui.home=0;ui.song_row=0;
 if(s<3){go_page(GR_SLOTS);song.g[G_SLOT]=3;if(s==1){fail_read=1;proof(project_native_refresh()!=0,"actual backend error invalidates catalog");}if(s==2)project_native_reset();}
 else{go_page(GR_SONG);if(s>=4){start();if(s==5||s==6||s==7){proof(!d8arr_request_fill(2),"actual fill request");if(s>=6){boundary();proof(chain.native.policy.fill_active,"bar activates fill");}if(s==7){d8arr_cancel_fill();proof(chain.native.policy.fill_active==2,"actual deferred return state");}}
  if(s==8){proof(!d8arr_request_row(1),"actual queued row request");proof(chain.native.policy.pending==1,"actual pending row");}}
 }
}
static void render(unsigned s,unsigned p,unsigned st,unsigned big,unsigned track,const char*out){
 static char name[96];snprintf(name,sizeof name,"%s/%s/style%u/large%u/track%u",names[s],UI_PALETTES[p].name,st,big,track+1);cur_name=name;
 song.sel=(uint8_t)track;ui_style=(uint8_t)st;style_apply();pal(p);ui_prefs=big?ui_prefs|PREF_LARGE:ui_prefs&~PREF_LARGE;large_on=(uint8_t)big;
 nscr=npend=nrules=ntight=0;tight[0]=0;memset(host_screen,0,sizeof host_screen);unsigned r=reads,w=writes,df=dma_errors,nf=nfind,af=al_fail;
 ui.force=1;ui.frame++;ui_draw();lcd_sync();lint();mono_check();render_frames++;
 proof(reads==r&&writes==w,"renderer performs no storage access");proof(dma_errors==df&&!cv_cpu_active&&!migration_owner,"async canvas ownership intact");proof(nfind==nf&&al_fail==af,"no clipping/overlap/alignment findings");
 proof(!memcmp(&d8p1_runtime_cache.arrangement,&native_source.arrangement,sizeof native_source.arrangement),"render preserves complete adopted arrangement and names");
 for(unsigned i=0;i<nscr;i++)if(scr[i].flags&1u)proof((scr[i].flags&8u)!=0,"only explicitly designated user text may ellipsise");
 if(s<3){proof(word("A")&&word("B")&&word("C")&&!word("D"),"exact three native slot labels");proof(word(s==0?"3 PROJECTS":s==1?"CHECK STORAGE":"MIGRATE TO SAVE"),"correct native storage status footer");if(s==0)proof(word("LOWER BANK A")&&word("UPPER BANK C")&&word("--"),"actual sparse catalog names and empty middle slot");if(s==1)proof(!word("LOWER BANK A")&&!word("UPPER BANK C"),"failed refresh hides stale names");}
 else{proof(!word("PAT A x4 > B x1")&&!word("ADD PATTERN")&&!word("PROJECT A-D"),"native graph never advertises legacy pattern or four-slot actions");proof(word("x2")&&word("x1"),"actual native graph repeats");proof(word("LONG SCENE A")&&word("LONG SCENE B")&&word("REPLACE FILL"),"actual native graph scene names");proof(word("ROW")&&word("SCENE"),"native arrangement columns");proof(word(s==3?"REPS":"FILL +/-"),"stopped/running semantics");proof(word(s==8?"NEXT":"PLAY"),"pending/current row label");if(s>=4)proof(graph_word(s==5?"ARM":s==6?"FILL":s==7?"RET":s==8?"NEXT":"RUN"),"actual native graph running/fill/pending status");if(s>=4)proof(word(s==5?"ARM":s==6?"RUN":s==7?"RET":"OFF"),"actual fill status label");}
 if(out&&st==ST_FLAT&&!big&&track==7&&(!strcmp(UI_PALETTES[p].name,"GREY")||!strcmp(UI_PALETTES[p].name,"PAPER")))write_ppm(out,UI_PALETTES[p].name,names[s]);
}
static void cached_transitions(void){
 cur_name="CACHED_TRANSITIONS";
 scene(4);render(4,UI_GREY_INDEX,ST_FLAT,0,7,NULL);
 unsigned r=reads,w=writes;uint32_t old=pixels_hash(host_screen,240*240);
 proof(!d8arr_request_row(1),"actual incremental queued row");nscr=npend=nrules=0;ui.force=0;ui.frame++;ui_draw();lcd_sync();lint();
 proof(graph_word("NEXT")&&pixels_hash(host_screen,240*240)!=old,"pending change redraws native graph without forced invalidation");
 boundary();old=pixels_hash(host_screen,240*240);nscr=npend=nrules=0;ui.force=0;ui.frame++;ui_draw();lcd_sync();lint();
 proof(chain.row==1&&graph_word("RUN")&&!graph_word("NEXT")&&pixels_hash(host_screen,240*240)!=old,"actual row boundary redraws cached native graph");
 proof(reads==r&&writes==w,"incremental rendering never reads storage");
 seq_stop();transport_req=0;ui.song_row=15;nscr=npend=nrules=0;ui.force=1;ui_draw();lcd_sync();lint();
 proof(graph_word("REPLACE FILL")&&word("3"),"native graph clamps a historical row cursor to its adopted final row");
}
static void edge_arrangements(const char*out){
 seq_stop();transport_req=0;cur_name="EDGE_ADOPTION";
 /* The zero-row state is actually decoded/adopted, not a synthetic cache flag. */
 memset(&native_source.arrangement,0,sizeof native_source.arrangement);size_t n=0;
 proof(d8p1_project_encode(native_wire,sizeof native_wire,&n,&native_source)&&!d8p1_load_runtime(native_wire,n,5),"actual empty native arrangement adoption");
 for(unsigned kind=0;kind<2;kind++){
  if(kind){
   d8p1_arrangement*a=&native_source.arrangement;a->banks=2;a->scenes=a->rows=16;
   for(unsigned b=0;b<2;b++)for(unsigned t=0;t<8;t++){a->bank[b].project[t]=(uint8_t)(b*2);a->bank[b].track[t]=(uint8_t)t;}
   for(unsigned r=0;r<16;r++){memcpy(a->scene[r].name,"WWWWWWWWWWWW",12);a->scene[r].bank=(uint8_t)(r%2);a->row[r].scene=(uint8_t)r;a->row[r].repeat=16;}
   proof(d8p1_project_encode(native_wire,sizeof native_wire,&n,&native_source)&&!d8p1_load_runtime(native_wire,n,5),"actual maximum native arrangement adoption");
  }
  go_page(GR_SONG);ui.msg_t=0;
  for(unsigned cursor=0;cursor<3;cursor++)for(unsigned p=0;p<NPALETTES;p++)for(unsigned st=ST_FLAT;st<=ST_LINE;st++)for(unsigned big=0;big<2;big++)for(unsigned bank=0;bank<2;bank++){
   cur_name=kind?"MAXIMUM_NATIVE":"EMPTY_NATIVE";ui.song_row=(uint8_t)(cursor==0?0:cursor==1?15:255);song.sel=(uint8_t)(bank?7:0);
   ui_style=(uint8_t)st;style_apply();pal(p);ui_prefs=big?ui_prefs|PREF_LARGE:ui_prefs&~PREF_LARGE;
   nscr=npend=nrules=0;unsigned r=reads,w=writes,nf=nfind,af=al_fail;ui.force=1;ui_draw();lcd_sync();lint();mono_check();render_frames++;
   proof(nfind==nf&&al_fail==af,"edge layout has no clipping/overlap/alignment failure");proof(reads==r&&writes==w,"edge rendering has no storage access");
   proof(!word("PAT A x4 > B x1")&&!word("ADD PATTERN")&&!word("PROJECT A-D"),"adopted empty/max native graph never falls back to legacy actions");
   proof(word("ROW")&&word("SCENE")&&word("REPS")&&word("PLAY"),"native cards remain native for empty and maximum states");
   if(!kind)proof(graph_word("NO SCENES")&&graph_word("ARRANGEMENT EMPTY"),"explicit empty native graph");
   else{unsigned repeats=0;for(unsigned k=0;k<nscr;k++)if(scr[k].y0>=(int)graph_y()&&!strcmp(scr[k].s,"x16"))repeats++;
    proof(repeats==(cursor?3u:7u),"bounded maximum-row viewport shows first/last/clamped rows");proof(word(cursor?"16":"1"),"native selected row number bounded");}
   for(unsigned k=0;k<nscr;k++)if(scr[k].flags&1u)proof((scr[k].flags&8u)!=0,"widest user names alone may ellipsise");
   proof(!memcmp(&d8p1_runtime_cache.arrangement,&native_source.arrangement,sizeof native_source.arrangement),"edge render preserves maximum/empty native state");
   if(out&&p==UI_GREY_INDEX&&st==ST_FLAT&&!big&&bank==1&&cursor<2)write_ppm(out,"GREY",kind?(cursor?"MAXIMUM_LAST":"MAXIMUM_FIRST"):"EMPTY_NATIVE");
  }
 }
}
int main(int argc,char**argv){const char*out=argc>1?argv[1]:NULL;rep=tmpfile();if(!rep)return 2;seed();
 for(unsigned s=0;s<9;s++){scene(s);for(unsigned p=0;p<NPALETTES;p++)for(unsigned st=ST_FLAT;st<=ST_LINE;st++)for(unsigned b=0;b<2;b++)for(unsigned t=0;t<8;t++)render(s,p,st,b,t,out);}
 cached_transitions();edge_arrangements(out);canvas_refusal();seq_stop();lcd_sync();proof(!nfind&&!nink&&!mono_bad&&!al_fail&&!dma_errors,"all rendering and async ownership checks clean");
 rewind(rep);char buf[1024];while(fgets(buf,sizeof buf,rep))fputs(buf,stdout);fclose(rep);
 printf("native_state_render: %u checks, %u frames, %u failures; actual adopted native catalog/arrangement, no device qualification\n",checks,render_frames,failures);return failures?1:0;}
