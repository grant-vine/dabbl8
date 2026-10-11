/* SPDX-License-Identifier: GPL-3.0-only */
/* Test-only FUN9 writer. Literal historical dimensions and explicit byte fields
 * keep this independent of live NTRK and the production pack/codec helpers.
 * This constructs legacy inputs; it does not authorize native persistence. */
_Static_assert(PROJ_LEGACY_NTRK == 4 && P_COUNT == 99 && G_COUNT == 27 && NSTEP == 64, "frozen FUN9 dimensions");
static void fixture_le16(uint8_t *p, unsigned v) { p[0]=v; p[1]=v>>8; }
static void fixture_le32(uint8_t *p, uint32_t v) { for(unsigned i=0;i<4;i++)p[i]=v>>(8*i); }
static uint32_t fixture_hash(const uint8_t *p,unsigned n) {
 uint32_t h=2166136261u;for(unsigned i=0;i<n;i++)h=(h^p[i])*16777619u;return h;
}
static int fixture_fun9(project_store_t *out,const project_t *q) {
 uint8_t b[3648]={0};unsigned pos=68;
 fixture_le32(b,0x46554e39u);fixture_le32(b+4,3648);
 for(unsigned i=0;i<27;i++)fixture_le16(b+8+i*2,(uint16_t)q->g[i]);
 b[62]=q->sel;b[63]=q->parts;b[64]=q->phys;b[66]=99;
 for(unsigned t=0;t<4;t++) {
  for(unsigned i=0;i<99;i++) {if(q->t[t].p[i]<-64||q->t[t].p[i]>127)return 0;b[pos++]=q->t[t].p[i]+64;}
  b[pos++]=q->t[t].engine;b[pos++]=q->t[t].preset;
  for(unsigned i=0;i<64;i++) {
   const step_t *s=&q->t[t].step[i];unsigned r=(s->flags>>3)&3;
   if(s->n>4||s->time>2||(s->flags&~27u)||s->probability>101)return 0;
   for(unsigned j=0;j<4;j++)b[pos++]=s->note[j]>127?127:s->note[j];
   b[pos++]=s->n|(s->time<<3)|((s->flags&3)<<5);
   b[pos++]=(s->vel>127?127:s->vel)|((r&1)<<7);b[pos++]=s->hit;
   b[pos++]=s->acc&s->hit;b[pos++]=s->probability|((r>>1)<<7);
  }
 }
 if(q->chain.count>16)return 0;
 b[pos++]=q->chain.count;for(unsigned i=0;i<3;i++)b[pos++]=q->chain.rsv[i];
 for(unsigned i=0;i<16;i++) {
  if(i<q->chain.count&&(q->chain.row[i].slot>=4||!q->chain.row[i].repeat||q->chain.row[i].repeat>16))return 0;
  b[pos++]=q->chain.row[i].slot;b[pos++]=q->chain.row[i].repeat;
 }
 if(q->motion.count>64||(q->motion.on&~15u))return 0;
 b[pos++]=q->motion.count;b[pos++]=q->motion.on;
 b[pos++]=q->motion.rsv[0];b[pos++]=q->motion.rsv[1];
 for(unsigned i=0;i<64;i++) {
  const motion_event_t *e=&q->motion.event[i];if(e->place>255)return 0;
  if(i<q->motion.count&&(e->value< -64||e->value>127))return 0;
  b[pos++]=e->place;b[pos++]=e->param;fixture_le16(b+pos,(uint16_t)e->value);pos+=2;
 }
 for(unsigned t=0;t<4;t++)for(unsigned i=0;i<128;i++) {
  if(q->fm6[t][i]>127)return 0;b[3120+t*128+i]=q->fm6[t][i];
 }
 unsigned n=0;while(n<12&&q->name[n]) {
  unsigned c=(unsigned char)q->name[n];if(c<32||c>126){n=0;break;}n++;
 }
 for(unsigned i=0;i<n;i++){unsigned c=(unsigned char)q->name[i];b[3632+i]=c>='a'&&c<='z'?c-32:c;}
 fixture_le32(b+3644,fixture_hash(b,3644));memcpy(out->raw,b,3648);return 1;
}
/* Existing four-track cases still execute the production encoder. Native cases
 * encode explicitly historical first-four inputs and test native refusal apart. */
static int fixture_pack(project_store_t *out,const project_t *q) {
#if NPART == 4
 return proj_pack(out,q);
#else
 return fixture_fun9(out,q);
#endif
}
