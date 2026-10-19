/* SPDX-License-Identifier: GPL-3.0-only */
/* Actual command81 and panel handlers against private virtual NOR. No device. */
#define D8_PROJECT_TRANSFER_NO_MAIN 1
#include "native_project_transfer_test.c"
int main(void) {
 target_fresh();trk[7].p[P_LEVEL]=91;char line[4096];
 while(fgets(line,sizeof line,stdin)) {
  if(!strncmp(line,"APPROVE",7)) {
   d8ls_draw();mc_ship_tap(B_SAVE);d8ls_poll();
   printf("{\"phase\":%u,\"failures\":%u}\n",pt_phase(),failures);fflush(stdout);continue;
  }
  if(!strncmp(line,"REBOOT",6)) {
   retained_reboot();printf("{\"failures\":%u}\n",failures);fflush(stdout);continue;
  }
  if(!strncmp(line,"CUT ",4)) {cut=atoi(line+4);partial_failure=cut<0?0:17;puts("{\"ok\":true}");fflush(stdout);continue;}
  if(!strcmp(line,"EXPORT_COPY_FAIL\n")) {pt_read_event=2;puts("{\"ok\":true}");fflush(stdout);continue;}
  if(!strcmp(line,"CLEAR_READ_FAULT\n")) {pt_read_event=0;post_commit_io=0;puts("{\"ok\":true}");fflush(stdout);continue;}
  uint8_t bytes[600]={0x7d,0x46,0x4c};unsigned count=3;char *p=line,*end;
  while(*p){long n=strtol(p,&end,10);if(end==p)break;if(n<0||n>127||count>=sizeof bytes)return 2;bytes[count++]=(uint8_t)n;p=end;}
  if(count<4)return 2;ed_n=0;ed_handle(bytes,count);so_r=so_w;
  printf("{\"cmd\":%u,\"args\":[",ed_n>=6?ed_out[4]:0);
  for(unsigned i=5;i+1<ed_n;i++)printf("%s%u",i==5?"":",",ed_out[i]);
  printf("],\"physical_calls\":%u,\"failures\":%u}\n",physical_calls,failures);fflush(stdout);
 }
 return failures||ferror(stdin)?1:0;
}
