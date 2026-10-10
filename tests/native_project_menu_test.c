/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual project menu/capture/shared storage; virtual NOR only. */
#define D8POOL_RUNTIME_NO_MAIN 1
#include "d8p1_pool_runtime_test.c"
static unsigned calls;
static int catalog_error;
static int menu_catalog(void *c,d8p1_project_catalog *out)
{ calls++;return catalog_error?D8POOL_IO:d8p1_catalog_pool(c,out); }
static int menu_save(void *c,unsigned slot,const char *name)
{ calls++;return d8p1_save_as_pool(c,slot,name); }
static int menu_load(void *c,unsigned slot)
{ calls++;return d8p1_load_pool(c,slot); }
static int menu_rename(void *c,unsigned slot,const char *name)
{ calls++;return d8p1_rename_pool(c,slot,name); }
static project_native_ops ops={&pool,menu_catalog,menu_save,menu_load,menu_rename};
static void image(const char *path) {
 FILE *f=fopen(path,"wb");if(!f)exit(2);fprintf(f,"P6\n240 240\n255\n");
 for(unsigned i=0;i<240*240;i++){uint16_t c=swap16(host_screen[i]);uint8_t rgb[3]={(uint8_t)(((c>>11)&31)*255/31),(uint8_t)(((c>>5)&63)*255/63),(uint8_t)((c&31)*255/31)};fwrite(rgb,1,3,f);}fclose(f);
}
int main(int argc,char **argv){
 ui_power_on();int32_t audio[CTL*2];mix_block(audio,CTL);blank();char name[13];
 proof(PROJECT_UI_SLOTS==3&&GP[G_SLOT].max==4&&global_desc(G_SLOT)->max==3&&sizeof proj_slot/sizeof proj_slot[0]==4,"three-slot active range preserves historical four-slot descriptor/cache");
 calls=0;proof(project_native_bind(&ops,0)==D8POOL_UNSUPPORTED&&!calls&&!project_native_owns_storage(),"ownership never inferred or granted without completed migration policy");
 proof(project_save_as(0,"DENIED")&&msg_is("MIGRATION REQUIRED")&&!writes&&!calls,"actual menu save refuses before migration");
 size_t n=fixture("tests/fixtures/d8p1/minimal.d8p");proof(!d8p1_load_runtime(wire,n,0),"native live menu fixture");mix_block(audio,CTL);
 for(unsigned o=0;o<4;o++)proof(!d8pool_save(&pool,o,wire,n,7),"explicit test-only native migration seed");reset();
 proof(!project_native_bind(&ops,1)&&project_native_status()==1&&project_native_owns_storage(),"explicit completed migration handoff binds actual native pool");
 trk[7].p[P_LEVEL]=111;go_page(GR_SLOTS);song.g[G_SLOT]=3;edit_param(0,99);
 proof(song.g[G_SLOT]==3,"actual project knob cannot select historical fourth identity");
 proof(!project_save_as(2,"TRACK EIGHT")&&proj_cur==2&&!strcmp(proj_name,"TRACK EIGHT")&&project_name(2,name)&&!strcmp(name,"TRACK EIGHT"),"actual menu saves live track eight/name and refreshes cache");
 trk[7].p[P_LEVEL]=3;project_load(2);proof(trk[7].p[P_LEVEL]==111&&proj_cur==2&&msg_is("LOADED"),"actual menu recalls all eight tracks through native runtime");mix_block(audio,CTL);
 trk[7].p[P_LEVEL]=17;proof(!project_rename(2,"SAVED NAME")&&trk[7].p[P_LEVEL]==17&&!strcmp(proj_name,"SAVED NAME"),"actual menu rename preserves unsaved live edits");
 calls=0;proof(project_save(3)&&!calls&&!project_used(3)&&!project_name(3,name)&&!graph_project_used(3)&&!graph_project_name(3)[0],"invalid menu/graph identities never alias an available slot");
 proof(chain_prepare()==1&&!chain_busy(),"native arrangement does not execute stale historical chain cache");
 go_page(GR_SLOTS);ui.force=1;ui.frame++;(void)graph_project_used(0);calls=0;
 cv_begin(240,124,T_BG);uint32_t pixels=pixels_hash(cv_px,CV_MAX);
 for(unsigned i=0;i<3;i++)proof(project_name(i,name)&&project_used(i)&&graph_project_used(i),"names/used render queries are cache-only during drawing");
 proof(!calls&&pixels_hash(cv_px,CV_MAX)==pixels,"native catalog reads never overwrite an active canvas or touch storage");cv_blit(0,0);lcd_sync();
 ui.force=1;ui_draw();lcd_sync();if(argc>1)image(argv[1]);proof(!dma_errors,"actual project page render preserves LCD ownership");
 /* Failed rescan never presents stale cache as current, but keeps ownership. */
 catalog_error=1;proof(project_native_refresh()==D8POOL_IO&&project_native_status()==2&&project_native_owns_storage()&&!project_used(0)&&!project_name(0,name),"failed refresh hides stale names while retaining native ownership");
 /* Storage recovery alone cannot turn hidden occupancy into an empty save. */
 catalog_error=0;calls=0;go_page(GR_SLOTS);song.g[G_SLOT]=1;ui.act=4;act_do();
 proof(!name_on()&&ui.confirm==CF_NONE&&msg_is("STORAGE ERROR")&&!calls,"actual SAVE action refuses stale occupancy before name/overwrite decision");
 proof(project_save_as(0,"NO OVERWRITE")&&!calls&&msg_is("STORAGE ERROR"),"direct/name-screen save cannot bypass stale-inventory refusal after storage recovers");
 proof(!project_native_refresh()&&project_native_status()==1,"explicit rescan recovers current inventory");
 ui.act=4;act_do();proof(ui.confirm==CF_OVR_PROJ&&ui.confirm_trk==0&&!name_on(),"recovered actual SAVE action requires occupied-slot overwrite confirmation");ui.confirm=CF_NONE;
 reset();snapshot();cut=0;int failed=project_save_as(1,"FAIL");before.ui_state=ui;proof(failed&&msg_is("STORAGE ERROR")&&unchanged()&&strcmp(proj_name,"FAIL"),"menu interrupted write preserves music/name/identity and reports failure");reset();
 song.playing=1;calls=0;proof(project_save(0)&&!calls&&msg_is("STOP TO SAVE"),"playing menu save refuses before callback");song.playing=0;
 project_native_ops invalid=ops;invalid.save=NULL;calls=0;
 proof(project_native_bind(&invalid,1)==D8POOL_INVALID&&project_native_status()==2&&project_native_owns_storage()&&!calls,"failed rebind retains native ownership and disables operations");
 project_load(0);proof(msg_is("MIGRATION REQUIRED")&&!calls,"offline native mode cannot fall back to stale historical RAM slot");
 proof(!project_native_bind(&ops,1),"explicit valid rebind recovers native menu");
 project_native_reset();proof(!project_native_owns_storage()&&project_native_status()==0,"cold reset revokes all transient callbacks/ownership rather than auto-authorizing storage");
 printf("Native project menu: %u checks, %u failures; actual UI/runtime, virtual storage only\n",checks,failures);return failures!=0;
}
