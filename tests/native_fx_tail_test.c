/* SPDX-License-Identifier: GPL-3.0-only */
/* Resident DSP state and real autosave adapter; simulated physical storage. */
#define D8FLASH_RUNTIME_NO_MAIN 1
#include "d8p1_flash_runtime_test.c"
static unsigned kind;
static int32_t *const histories[]={&fx.dly_lp,&fx.comb_lp[0],&fx.comb_lp[1],&fx.comb_lp[2],&fx.comb_lp[3],&fx.sp_lp,&fx.sp_hp,&dc_l,&dc_r,&lc_l1,&lc_l2,&lc_r1,&lc_r2,&sb_lp1,&sb_lp2,&sb_lp3,&sb_lp4,&sb_env,&sb_h1,&sb_h2,&sb_hl};
enum { HISTORY_COUNT=sizeof histories/sizeof histories[0],TAIL_KINDS=4+HISTORY_COUNT+4+2+1+2+8 };
/* Synthetic cold reset only; production never clears audio to force a save. */
static void clear_tail(void)
{
    memset(dly_buf,0,sizeof dly_buf);memset(cho_buf,0,sizeof cho_buf);
    memset(&fx,0,sizeof fx);rev_clear();fx_tail.delay=fx_tail.chorus=0;
    for(unsigned i=0;i<HISTORY_COUNT;i++)*histories[i]=0;
    memset(lce,0,sizeof lce);dce_l=dce_r=0;memset(&clk,0,sizeof clk);click_req=0;
    for(unsigned t=0;t<8;t++)trk[t].tail=trk[t].dist_hp=trk[t].dist_lp1=trk[t].dist_lp2=0;
}
static void oracle(void)
{
    uint32_t d=0,c=0,r=0,a=0;
    for(unsigned i=0;i<DLY_LEN;i++)d+=dly_buf[i]!=0;
    for(unsigned i=0;i<CHO_LEN;i++)c+=cho_buf[i]!=0;
    for(unsigned i=0;i<sizeof rev_comb/sizeof rev_comb[0];i++)r+=rev_comb[i]!=0;
    if(fx.rtype)for(unsigned i=0;i<4*(SP_N+1);i++)a+=rev_u.sp[i]!=0;
    else for(unsigned i=0;i<sizeof rev_ap/sizeof rev_ap[0];i++)a+=rev_ap[i]!=0;
    CHECK(d==fx_tail.delay&&c==fx_tail.chorus&&r==fx_tail.comb&&a==fx_tail.ap);
}
static void disturb_tail(void)
{
    unsigned k=kind;
    if(k==0)FT16(delay,&dly_buf[DLY_LEN-1],1);
    else if(k==1)FT16(chorus,&cho_buf[CHO_LEN-1],-1);
    else if(k==2)FT16(comb,&rev_comb[sizeof rev_comb/2-1],1);
    else if(k==3)FT16(ap,&rev_ap[sizeof rev_ap/2-1],-1);
    else if((k-=4)<HISTORY_COUNT)*histories[k]=1;
    else if((k-=HISTORY_COUNT)<4)lce[k]=32;
    else if((k-=4)<2){if(k)dce_r=4096;else dce_l=-1;}
    else if((k-=2)==0)fx.sp_he=64;
    else if((k-=1)<2){if(k)clk.env=1;else click_req=1;}
    else {k-=2;trk[k].tail=16;trk[k].dist_lp2=1;}
}
int main(void)
{
    ui_power_on();clear_tail();CHECK(fx_resident_quiet());
    /* Independent full scans are host-only: account every real store/alias,
     * wrap, negative/nonzero overwrite and reverb model fade/reset. */
    uint32_t rng=1;int32_t c[CTL],d[CTL],r[CTL],w[CTL];
    for(unsigned block=0;block<600;block++) {
        song.g[G_RTYPE]=(int16_t)((block/75)&1u);
        unsigned n=block%CTL+1;
        for(unsigned i=0;i<n;i++) {
            rng=rng*1664525u+1013904223u;
            c[i]=block%3?(int32_t)(rng&65535u)-32768:0;
            d[i]=block%5?(int32_t)((rng>>8)&65535u)-32768:0;
            r[i]=block%7?(int32_t)((rng>>16)&65535u)-32768:0;
        }
        fx_buses(c,d,r,w,n);oracle();CHECK(block?!fx_resident_quiet():fx_resident_quiet());
    }
    clear_tail();CHECK(fx_resident_quiet());
    /* Last odd ROOM halfword and 32-bit SPRING cells with a zero halfword. */
    FT16(ap,&rev_ap[sizeof rev_ap/2-1],-1);oracle();CHECK(!fx_resident_quiet());rev_clear();oracle();
    fx.rtype=1;FT32(ap,&rev_u.sp[0],65536);FT32(ap,&rev_u.sp[1],-65536);oracle();
    FT32(ap,&rev_u.sp[0],65536);oracle();FT32(ap,&rev_u.sp[0],0);oracle();rev_clear();oracle();
    CHECK(!fx_tail.comb&&!fx_tail.ap);
    /* Longest delay: actual output is initially zero, resident echo isn't.
     * Master gain zero and fixed USB gain do not excuse the retained sample. */
    clear_tail();song.g[G_RTYPE]=0;song.g[G_BPM]=40;song.g[G_DTIME]=0;song.g[G_DFDBK]=0;song.g[G_DMIX]=127;
    memset(c,0,sizeof c);memset(d,0,sizeof d);memset(r,0,sizeof r);d[0]=32000;
    fx_buses(c,d,r,w,1);CHECK(!w[0]&&fx_tail.delay==1&&!fx_resident_quiet());
    d[0]=0;song.master_q12=0;fx_usb_fixed=1;
    for(unsigned i=1;i<delay_samples();i++) {fx_buses(c,d,r,w,1);CHECK(!w[0]&&!fx_resident_quiet());}
    fx_buses(c,d,r,w,1);CHECK(w[0]>0&&!fx_resident_quiet());oracle();
    song.master_q12=4096;fx_usb_fixed=0;
    /* Stable zero-input error residues versus a mode-changing residue. */
    clear_tail();dce_l=dce_r=4095;fx.sp_he=63;for(unsigned i=0;i<4;i++)lce[i]=31;
    CHECK(fx_resident_quiet());
    memset(w,0,sizeof w);rev_spring(r,w,CTL);
    for(unsigned i=0;i<CTL;i++)CHECK(!w[i]);
    CHECK(fx.sp_he==63&&fx_resident_quiet());
    for(unsigned mode=0;mode<3;mode++){fx_lowcut=(uint8_t)mode;int32_t l=0,rr=0;master_out(&l,&rr);CHECK(!l&&!rr&&fx_resident_quiet());}
    clear_tail();lce[1]=32;fx_lowcut=2;CHECK(!fx_resident_quiet());
    int32_t l=0,rr=0;master_out(&l,&rr);CHECK(l!=0||lc_l2!=0);fx_lowcut=0;
    /* DAC-only metronome has its own pending/envelope state after STOP. */
    clear_tail();click_req=2;CHECK(!fx_resident_quiet());int32_t audio[CTL*2]={0};click_render(audio,CTL);CHECK(clk.env>0&&!fx_resident_quiet());
    for(unsigned i=0;i<64;i++)click_render(audio,CTL);CHECK(!clk.env&&fx_resident_quiet());
    clear_tail();trk[7].dist_lp2=10000;CHECK(fx_resident_quiet());
    trk[7].tail=1;trk[7].p[P_DIST]=0;CHECK(!fx_resident_quiet());
    memset(w,0,sizeof w);track_dist(&trk[7],w,CTL);CHECK(!w[0]&&trk[7].dist_lp2==10000);
    trk[7].p[P_DIST]=127;track_dist(&trk[7],w,CTL);CHECK(w[0]!=0);clear_tail();
    /* Real canonical project and native pool. No implicit authorization. */
    FILE *f=fopen("tests/fixtures/d8p1/minimal.d8p","rb");if(!f)return 2;
    size_t n=fread(wire,1,sizeof wire,f);CHECK(!ferror(f)&&fgetc(f)==EOF);fclose(f);
    CHECK(!d8p1_load_runtime(wire,n,0));mix_block(audio,CTL);
    for(unsigned t=0;t<8;t++)trk[t].p[P_SLCR]=trk[t].p[P_AMODE]=trk[t].p[P_DIST]=0;
    clear_tail();CHECK(df_automatic_quiet());n=0;CHECK(!d8p1_capture_runtime(wire,sizeof wire,&n));
    blank();flash_ok=1;for(unsigned o=0;o<4;o++)CHECK(!d8pool_save(&seed,o,wire,n,7));reset();
    d8pool_index index;CHECK(!d8pool_inventory(&seed,&index));memcpy(baseline,nor,sizeof nor);
    proj_cur=2;trk[7].p[P_LEVEL]=109;char name[sizeof proj_name];memcpy(name,proj_name,sizeof name);
    unsigned operations=2+(unsigned)((n+255)/256)+1;
    for(kind=0;kind<TAIL_KINDS;kind++) {
        clear_tail();reset();disturb_tail();CHECK(!fx_resident_quiet());
        CHECK(d8p1_autosave_flash(1)==D8POOL_BUSY&&!reads&&!writes&&!irq_disabled);
        for(unsigned cutpoint=0;cutpoint<operations;cutpoint++) {
            memcpy(nor,baseline,sizeof nor);clear_tail();reset();inject_at=(int)cutpoint;irq_inject=disturb_tail;
            CHECK(d8p1_autosave_flash(1)==D8POOL_IO&&writes==cutpoint&&!irq_disabled);
            CHECK(proj_cur==2&&trk[7].p[P_LEVEL]==109&&!memcmp(name,proj_name,sizeof name));
            clear_tail();reset();unchanged(&index);
        }
        memcpy(nor,baseline,sizeof nor);clear_tail();reset();after_commit=disturb_tail;
        CHECK(d8p1_autosave_flash(1)==D8POOL_IO&&proj_cur==2&&!irq_disabled);
        clear_tail();reset();CHECK(!d8p1_restore_flash_autosave(1,1)&&proj_cur==PROJ_NO_SLOT&&trk[7].p[P_LEVEL]==109);
        mix_block(audio,CTL);proj_cur=2;
    }
    memcpy(nor,baseline,sizeof nor);clear_tail();reset();CHECK(!d8p1_autosave_flash(1)&&proj_cur==2&&!irq_disabled);
    printf("Native resident tails: %u checks, %u failures; 600 DSP/oracle blocks, longest-delay gap, %u states x %u late mutation cuts and postcommit restores; simulated hooks, no queued-audio proof\n",checks,failures,TAIL_KINDS,operations);
    return failures!=0;
}
