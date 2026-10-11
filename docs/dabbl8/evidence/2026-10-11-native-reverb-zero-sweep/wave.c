#define NPART 8
#define UI_TEST_NO_MAIN 1
#include TREE_UI
#define FM1_AUDIO_HALF 0x80u
static uint8_t fm1_audio_pending(void){return 0;}
static void fm1_audio_ack_aux(uint8_t p){(void)p;}
static uint32_t fm1_audio_free_half(void){return 0;}
static void fm1_audio_ack_half(void){}
static void fm1_audio_init(int32_t*p,uint32_t n,void(*i)(void),uint32_t q){(void)p;(void)n;(void)i;(void)q;}
void isr_alnk0(void){}
#define FELUCCA_UAC 1
static int32_t tapped[2*CTL];
static void uac_tap(const int32_t*p,uint32_t n){memcpy(tapped,p,8*n);}
static void uac_render_start(void){}
#define felucca_dbg wave_dbg
#include TREE_AUDIO
#undef felucca_dbg
int main(int argc,char**argv){if(argc!=2)return 2;FILE*f=fopen(argv[1],"wb");if(!f)return 2;ui_power_on();song.master_q12=4096;
for(unsigned t=0;t<8;t++){host_preset(&trk[t],0,5);trk[t].p[P_SLCR]=0;trk[t].p[P_DIST]=30;trk[t].p[P_CHOR]=32;trk[t].p[P_DLY]=48;trk[t].p[P_REV]=64;}
int32_t out[2*CTL];for(unsigned b=0;b<1200;b++){if(b%80==0)trk_note_on(&trk[(b/80)%8],48+(b/80)%24,90);if(b%80==50)trk_note_off(&trk[(b/80)%8],48+(b/80)%24);
song.g[G_RTYPE]=(b/150)&1;fx_usb_fixed=(b/200)&1;fx_lowcut=(b/300)%3;if(b%100==0)click_req=2;perf_solo=b%160<80?(uint8_t)(1u<<((b/160)%8)):0;
audio_block(out,CTL);fwrite(out,sizeof out,1,f);fwrite(tapped,sizeof tapped,1,f);}return fclose(f)!=0;}
