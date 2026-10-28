#include "game.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
const uint8_t cc_max_level[4]={8,5,5,4};
static const uint32_t prices[4][8]={
 {1,10,100,1000,10000,100000,1000000,10000000},
 {1,10,100,1000,10000},{1,10,100,1000,10000},{25,100,625,10000}
};
static const uint8_t denominations[5]={1,5,10,25,100};
/* Numerically verified against all 54 results of the owned Unity math runtime.
 * A lookup keeps host and CE math-library approximations out of the economy. */
static const uint16_t factors[6][9]={
 {1,1,1,1,1,1,1,1,1},{1,2,3,4,6,8,12,18,26},
 {1,2,4,8,16,32,64,128,256},{1,3,7,16,40,98,245,611,1526},
 {1,3,9,27,81,243,729,2187,6561},{1,4,13,43,151,526,1839,6434,22519}
};
const char *const cc_shop_names[4]={"HEADS CHANCE","FLIP SPEED","COMBO POWER","COIN VALUE"};
const char *const cc_end_names[6]={"","ON THE EDGE","UNEXPECTED COMPANY","OUT OF ORBIT","SMALL CHANGE","TEN AT LAST"};
const char *const cc_badge_names[14]={"FIRST HEAD","DOUBLE TAKE","THREE OF A KIND","FOURWARD","HIGH FIVE","SIX APPEAL","LUCKY SEVEN","FIGURE EIGHT","CLOUD NINE","ON THE EDGE","UNEXPECTED COMPANY","OUT OF ORBIT","SMALL CHANGE","TEN AT LAST"};
static uint32_t add32(uint32_t x,uint32_t y){return UINT32_MAX-x<y?UINT32_MAX:x+y;}
static void history(CCRun *r,uint8_t code,uint32_t payout,uint8_t kind);
static const uint16_t tutorial_at[]={4,5,10,11,12,20,21,24,25,40,100,150,151,152,153,156,500,520,521,522,523,524,526,527,1000,1020,1021,1022,1023,1024,1025,1026,1029,5000,10000};
static const char *const tutorial[]={
 "WELCOME TO THE MINT.","A SMALL COIN. A LARGE AMOUNT OF CHANCE.",
 "INSPIRED BY HEATHER FLOWERS' GAME.","PIXELS REMINTED FOR A SMALL SCREEN.","THE CE EXPERIENCE IS SILENT.",
 "THE TASK FITS IN ONE SENTENCE.","TEN HEADS. CONSECUTIVELY.","PERHAPS THE NEXT RUN WILL BE SHORT.","PROBABILITY MAKES NO APPOINTMENTS.",
 "THIS COIN IS THE WHOLE GAME.","ONE HUNDRED LITTLE EXPERIMENTS.","THERE IS MORE THAN ONE FINAL FATE.","FIRST, FIND NINE HEADS IN A ROW.",
 "THE NEXT FLIP CAN STILL BE TAILS.","A TENTH HEAD DRAWS AN ENDING.","ONE ENDING HAS A TEN PERCENT CHANCE.","FIVE HUNDRED FLIPS. STILL HERE.",
 "OUR BRAINS LIKE PATTERNS.","THE COIN USES ITS DISPLAYED CHANCE.","YOUR ODDS ARE SHOWN ABOVE THE COIN.","THERE IS NO SECRET LUCK SWITCH.",
 "WANTING CERTAINTY IS VERY HUMAN.","NO BUTTON CAN GUARANTEE A HEAD.","INDEPENDENT ATTEMPTS TAKE REAL TIME.","ONE THOUSAND FLIPS IN THE LEDGER.",
 "A RUN COULD CONTINUE INDEFINITELY.","THERE IS NO PITY COUNTER.","EACH ROLL IS ANOTHER EXPERIMENT.","TWENTY PERCENT REMAINS TWENTY.",
 "THIRTY PERCENT REMAINS THIRTY.","CERTAINTY WOULD REQUIRE A HUNDRED.","EVEN EVERY UPGRADE LEAVES A CHANCE.","THE WAIT IS PART OF THE COIN.",
 "FIVE THOUSAND FLIPS. QUITE A MINT.","TEN THOUSAND ATTEMPTS. TAKE A BREATH."
};
void cc_new(CCGame *g,uint32_t seed){memset(g,0,sizeof(*g));g->rng=seed?seed:0xC01DCEu;}
void cc_restart(CCGame *g){memset(&g->run[g->active],0,sizeof(CCRun));g->run_ms[g->active]=0;g->ending_pending=0;g->in_flight=0;g->pending_head=0;}
uint32_t cc_price(const CCGame *g,uint8_t shop){
 if(shop>=4||g->run[g->active].level[shop]>=cc_max_level[shop])return 0;
 return prices[shop][g->run[g->active].level[shop]];
}
bool cc_buy(CCGame *g,uint8_t shop){
 uint32_t cost=cc_price(g,shop);CCRun *r=&g->run[g->active];
 if(!cost||g->ending_pending||r->bank<cost)return false;
 r->bank-=cost;++r->level[shop];history(r,shop,0,2);return true;
}
float cc_chance(const CCGame *g){
 float v=.2f;uint8_t i;for(i=0;i<g->run[g->active].level[CC_CHANCE];++i)v+=.05f;return v;
}
float cc_combo(const CCGame *g){return 1.f+.5f*g->run[g->active].level[CC_COMBO];}
uint16_t cc_duration_ms(const CCGame *g){return (uint16_t)(2000-200*g->run[g->active].level[CC_SPEED]);}
uint8_t cc_denomination(const CCGame *g){return denominations[g->run[g->active].level[CC_VALUE]];}
uint32_t cc_payout(uint8_t denom,uint8_t combo_level,uint8_t streak){
 if(!streak||streak>=10||combo_level>5)return 0;
 return (uint32_t)factors[combo_level][streak-1]*denom;
}
uint8_t cc_pick_ending(float roll){
 if(roll<.2f)return CC_EDGE;if(roll<.4f)return CC_VISITOR;
 if(roll<.6f)return CC_SKY;if(roll<.9f)return CC_SHATTER;return CC_TEN;
}
static void history(CCRun *r,uint8_t streak,uint32_t payout,uint8_t kind){
 if(r->history_count==CC_HISTORY){memmove(r->history,r->history+1,(CC_HISTORY-1)*sizeof(CCMessage));--r->history_count;}
 r->history[r->history_count].streak=streak;r->history[r->history_count].payout=payout;r->history[r->history_count].kind=kind;++r->history_count;
}
const char *cc_message_text(const CCMessage *m){
 if(m->kind==1&&m->streak<sizeof tutorial/sizeof tutorial[0])return tutorial[m->streak];
 if(m->kind==2&&m->streak<4)return cc_shop_names[m->streak];
 if(m->kind==3)return "MINT NOTE: EACH FLIP IS INDEPENDENT.";
 return m->streak?"HEADS":"TAILS";
}
int cc_begin(CCGame *g,float roll){
 CCRun *r=&g->run[g->active];unsigned i;bool head;
 if(g->ending_pending||g->in_flight||roll<0.f||roll>1.f||!isfinite(roll))return -1;
 r->flips=add32(r->flips,1);g->flips=add32(g->flips,1);
 if(!g->commentary){for(i=0;i<sizeof tutorial_at/sizeof tutorial_at[0];++i)
   if(r->flips==tutorial_at[i])history(r,(uint8_t)i,0,1);
 }else if(r->flips%40==1)history(r,0,0,3);
 head=roll<cc_chance(g);
 if(head){++r->streak;g->heads=add32(g->heads,1);}else r->streak=0;
 g->in_flight=1;g->pending_head=head?1:0;
 return head?1:0;
}
CCResult cc_land(CCGame *g,bool heads,float ending_roll){
 CCResult out={0};CCRun *r=&g->run[g->active];uint16_t old=g->achievements;
 if(g->ending_pending||!g->in_flight||heads!=(g->pending_head!=0)||ending_roll<0.f||ending_roll>1.f||!isfinite(ending_roll)||(heads&&(r->streak<1||r->streak>10)))return out;
 g->in_flight=0;
 if(heads){
  out.heads=1;out.streak=r->streak;
  if(r->streak>g->best_streak)g->best_streak=r->streak;
  if(r->streak<10){
   g->achievements|=(uint16_t)(1u<<(r->streak-1));
   out.payout=cc_payout(cc_denomination(g),r->level[CC_COMBO],r->streak);
   r->bank=r->bank>CC_BANK_MAX-out.payout?CC_BANK_MAX:r->bank+out.payout;
   g->earned=g->earned>CC_BANK_MAX-out.payout?CC_BANK_MAX:g->earned+out.payout;
   history(r,r->streak,out.payout,0);
  }else{
   out.ending=cc_pick_ending(ending_roll);g->last_ending=out.ending;
   g->achievements|=(uint16_t)(1u<<(out.ending+8));
   g->ending_count[out.ending-1]=add32(g->ending_count[out.ending-1],1);
   g->last_run_flips=r->flips;g->last_run_ms=g->run_ms[g->active];
   cc_restart(g);g->ending_pending=1;g->commentary=0;
  }
 }else{r->streak=0;history(r,0,0,0);}
 out.new_badges=(uint16_t)(g->achievements&~old);return out;
}
CCResult cc_flip(CCGame *g,float head_roll,float ending_roll){
 CCResult empty={0};int head;
 if(ending_roll<0.f||ending_roll>1.f||!isfinite(ending_roll))return empty;
 head=cc_begin(g,head_roll);if(head<0)return empty;return cc_land(g,head!=0,ending_roll);
}
uint32_t cc_random(CCGame *g){uint32_t x=g->rng;x^=x<<13;x^=x>>17;x^=x<<5;g->rng=x;return x;}
float cc_roll(CCGame *g){return (float)((double)cc_random(g)/4294967295.0);}
void cc_advance(CCGame *g,uint32_t ms){g->played_ms=add32(g->played_ms,ms);if(!g->ending_pending)g->run_ms[g->active]=add32(g->run_ms[g->active],ms);}
/* Portable wire format: explicit LE integers; never serialize compiler padding. */
static void put(uint8_t **p,uint64_t v,unsigned n){while(n--){*(*p)++=(uint8_t)v;v>>=8;}}
static uint64_t get(const uint8_t **p,unsigned n){uint64_t v=0;unsigned i;for(i=0;i<n;++i)v|=(uint64_t)*(*p)++<<(8*i);return v;}
void cc_encode(const CCGame *g,uint8_t out[CC_SAVE_SIZE]){
 uint8_t *p=out;unsigned i,j;memset(out,0,CC_SAVE_SIZE);memcpy(p,"CCSTATE1",8);p+=8;
 put(&p,g->rng,4);put(&p,g->earned,8);put(&p,g->flips,4);put(&p,g->heads,4);put(&p,g->played_ms,4);
 for(i=0;i<5;++i)put(&p,g->ending_count[i],4);
 put(&p,g->last_run_flips,4);put(&p,g->last_run_ms,4);put(&p,g->achievements,2);
 put(&p,g->active,1);put(&p,g->reduced_motion,1);put(&p,g->commentary,1);put(&p,g->tutorial_seen,1);
 put(&p,g->best_streak,1);put(&p,g->last_ending,1);put(&p,g->ending_pending,1);put(&p,g->fish,1);
 for(i=0;i<2;++i){const CCRun *r=&g->run[i];put(&p,g->run_ms[i],4);put(&p,r->bank,8);put(&p,r->flips,4);
  for(j=0;j<4;++j)put(&p,r->level[j],1);
  put(&p,r->streak,1);put(&p,r->history_count,1);
  for(j=0;j<CC_HISTORY;++j){put(&p,r->history[j].payout,4);put(&p,r->history[j].streak,1);put(&p,r->history[j].kind,1);}
 }
}
bool cc_decode(CCGame *dest,const uint8_t *data,size_t len){
 CCGame t;const uint8_t *p=data;unsigned i,j;
 if(len!=CC_SAVE_SIZE||memcmp(p,"CCSTATE1",8))return false;p+=8;memset(&t,0,sizeof(t));
 t.rng=(uint32_t)get(&p,4);t.earned=get(&p,8);t.flips=(uint32_t)get(&p,4);t.heads=(uint32_t)get(&p,4);t.played_ms=(uint32_t)get(&p,4);
 for(i=0;i<5;++i)t.ending_count[i]=(uint32_t)get(&p,4);
 t.last_run_flips=(uint32_t)get(&p,4);t.last_run_ms=(uint32_t)get(&p,4);t.achievements=(uint16_t)get(&p,2);
 t.active=(uint8_t)get(&p,1);t.reduced_motion=(uint8_t)get(&p,1);t.commentary=(uint8_t)get(&p,1);t.tutorial_seen=(uint8_t)get(&p,1);
 t.best_streak=(uint8_t)get(&p,1);t.last_ending=(uint8_t)get(&p,1);t.ending_pending=(uint8_t)get(&p,1);t.fish=(uint8_t)get(&p,1);
 if(!t.rng||t.active>1||t.reduced_motion>1||t.commentary>1||t.tutorial_seen>1||t.best_streak>10||t.last_ending>5||t.ending_pending>1||t.fish>1||t.achievements>0x3FFF||t.earned>CC_BANK_MAX||t.heads>t.flips|| (t.ending_pending&&!t.last_ending))return false;
 for(i=0;i<2;++i){CCRun *r=&t.run[i];t.run_ms[i]=(uint32_t)get(&p,4);r->bank=get(&p,8);r->flips=(uint32_t)get(&p,4);
  for(j=0;j<4;++j){r->level[j]=(uint8_t)get(&p,1);if(r->level[j]>cc_max_level[j])return false;}
  r->streak=(uint8_t)get(&p,1);r->history_count=(uint8_t)get(&p,1);
  if(r->bank>CC_BANK_MAX||r->streak>9||r->streak>r->flips||r->history_count>CC_HISTORY)return false;
  for(j=0;j<CC_HISTORY;++j){CCMessage *m=&r->history[j];m->payout=(uint32_t)get(&p,4);m->streak=(uint8_t)get(&p,1);m->kind=(uint8_t)get(&p,1);
   if(m->kind>3||m->payout>2251900u||(m->kind&&m->payout))return false;
   if((m->kind==0&&(m->streak>9||(!m->streak&&m->payout)))||(m->kind==1&&m->streak>=35)||(m->kind==2&&m->streak>=4)||(m->kind==3&&m->streak))return false;
  }
 }
 /* Reserved bytes must remain zero, making incompatible data fail closed. */
 while(p<data+len)if(*p++)return false;*dest=t;return true;
}
bool cc_validate_save(const uint8_t *d,size_t n,void *ctx){CCGame temp;(void)ctx;return cc_decode(&temp,d,n);}
void cc_money(char *buf,size_t size,uint64_t cents){
 /* Avoid relying on optional printf long-long support on CE. */
 char digits[24];unsigned n=0,i;uint64_t dollars=cents/100;char *p=buf;
 if(size<6){if(size)buf[0]=0;return;}
 do{digits[n++]=(char)('0'+dollars%10);dollars/=10;}while(dollars&&n<sizeof(digits));
 if(n+5>size){snprintf(buf,size,"LARGE");return;}
 *p++='$';for(i=n;i>0;--i)*p++=digits[i-1];*p++='.';*p++=(char)('0'+cents/10%10);*p++=(char)('0'+cents%10);*p=0;
}
