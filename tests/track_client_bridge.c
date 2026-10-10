/* SPDX-License-Identifier: GPL-3.0-only */
/* Line transport into the actual host firmware parser, for companion tests.
 * No physical USB/MIDI or flash. stdin: cmd args...; stdout: one JSON reply. */
#define EDITOR_TEST_NO_MAIN 1
#include "editor_test.c"
int main(void) {
    reset(); char line[8192];
    while(fgets(line,sizeof line,stdin)) {
        if(!strncmp(line,"BUSY ",5)) { chain.armed=(uint8_t)(atoi(line+5)!=0); puts("{\"cmd\":0,\"args\":[],\"flash_writes\":0,\"flash_erases\":0}");fflush(stdout);continue; }
        uint8_t bytes[600];unsigned count=0;char *p=line,*end;
        while(*p) {
            long n=strtol(p,&end,10);if(end==p)break;
            if(n<0 || n>127 || count>=sizeof bytes)return 2;bytes[count++]=(uint8_t)n;p=end;
        }
        if(!count)return 2;
        unsigned n=request(bytes[0],bytes+1,count-1);
        printf("{\"cmd\":%u,\"args\":[",n>=6?host_wire[4]:0);
        for(unsigned i=5;i+1<n;i++)printf("%s%u",i==5?"":",",host_wire[i]);
        printf("],\"flash_writes\":%u,\"flash_erases\":%u}\n",host_writes,host_erases);fflush(stdout);
    }
    return ferror(stdin)?1:0;
}
