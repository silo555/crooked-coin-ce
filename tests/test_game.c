#include "game.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
/* Independent expected factors: actual owned UnityEngine Mathf runtime probe,
 * build25535451; see fixtures/owned-build-25535451.json for source hashes. */
static const uint16_t expected[6][9]={
 {1,1,1,1,1,1,1,1,1},{1,2,3,4,6,8,12,18,26},
 {1,2,4,8,16,32,64,128,256},{1,3,7,16,40,98,245,611,1526},
 {1,3,9,27,81,243,729,2187,6561},{1,4,13,43,151,526,1839,6434,22519}
};
static void economy(void){
 CCGame g;unsigned cat,lv,st,d;const uint8_t denoms[]={1,5,10,25,100};
 const uint32_t costs[4][8]={{1,10,100,1000,10000,100000,1000000,10000000},{1,10,100,1000,10000},{1,10,100,1000,10000},{25,100,625,10000}};
 cc_new(&g,1);assert(cc_chance(&g)==.2f&&cc_combo(&g)==1&&cc_duration_ms(&g)==2000&&cc_denomination(&g)==1);
 for(cat=0;cat<4;++cat){cc_new(&g,1);
  for(lv=0;lv<cc_max_level[cat];++lv){
   assert(cc_price(&g,(uint8_t)cat)==costs[cat][lv]);g.run[0].bank=costs[cat][lv]-1;
   assert(!cc_buy(&g,(uint8_t)cat)&&g.run[0].level[cat]==lv);
   g.run[0].bank=costs[cat][lv];assert(cc_buy(&g,(uint8_t)cat));assert(!g.run[0].bank&&g.run[0].level[cat]==lv+1);
  }assert(!cc_price(&g,(uint8_t)cat)&&!cc_buy(&g,(uint8_t)cat));
 }
 assert(!cc_buy(&g,255));
 for(lv=0;lv<6;++lv)for(st=1;st<10;++st)for(d=0;d<5;++d)
  assert(cc_payout(denoms[d],(uint8_t)lv,(uint8_t)st)==(uint32_t)expected[lv][st-1]*denoms[d]);
 assert(cc_payout(5,1,2)==10&&cc_payout(5,1,3)==15);
 assert(cc_payout(100,5,9)==2251900&&cc_payout(100,5,10)==0);
 puts("economy: all22 purchases/caps,270 payout/denomination cases");
}
static void probability(void){
 static const uint32_t bits[]={1045220557u,1048576000u,1050253722u,1051931444u,1053609166u,1055286888u,1056964609u,1057803470u,1058642331u};
 CCGame g;unsigned lv;float p;uint32_t observed;
 for(lv=0;lv<9;++lv){
  cc_new(&g,2);g.run[0].level[0]=(uint8_t)lv;p=cc_chance(&g);memcpy(&observed,&p,4);assert(observed==bits[lv]);
  assert(cc_flip(&g,nextafterf(p,0.f),0).heads);
  assert(!cc_flip(&g,p,0).heads);assert(!g.run[0].streak);
  assert(!cc_flip(&g,nextafterf(p,1.f),0).heads);
  assert(!cc_flip(&g,1.f,0).heads);assert(cc_flip(&g,0.f,0).heads);
 }
 cc_new(&g,1);assert(!cc_flip(&g,NAN,0).heads&&!g.flips);cc_flip(&g,0,NAN);assert(!g.flips);
 cc_flip(&g,-1,0);cc_flip(&g,2,0);assert(!g.flips);
 {uint32_t heads=0,i;cc_new(&g,0x84ce);for(i=0;i<100000;++i)if(cc_roll(&g)<.2f)++heads;assert(heads>19300&&heads<20700);}
 puts("probability: exact float32 chance bits, strict boundaries, invalid rolls, RNG smoke");
}
static void endings(void){
 CCGame g;CCResult out;unsigned i,e;unsigned count[6]={0};
 static const float thresholds[]={.2f,.4f,.6f,.9f};
 for(i=0;i<1000;++i)++count[cc_pick_ending((float)i/1000.f)];
 assert(count[1]==200&&count[2]==200&&count[3]==200&&count[4]==300&&count[5]==100);
 for(i=0;i<4;++i){assert(cc_pick_ending(nextafterf(thresholds[i],0.f))==i+1);assert(cc_pick_ending(thresholds[i])==i+2);}
 assert(cc_pick_ending(1.f)==CC_TEN);
 cc_new(&g,3);for(i=0;i<10;++i)cc_flip(&g,1,0);assert(!g.ending_pending&&g.run[0].flips==10);
 for(i=0;i<9;++i)cc_flip(&g,0,0);out=cc_flip(&g,1,0);assert(!out.ending&&!g.run[0].streak);
 for(e=1;e<=5;++e){
  for(i=0;i<9;++i){out=cc_flip(&g,0,0);assert(out.streak==i+1&&out.payout==1);}
  out=cc_flip(&g,0,(float)(e==5?95:e==4?70:e*20-10)/100.f);
  assert(out.ending==e&&!out.payout&&g.ending_pending);assert(!g.run[0].flips&&!g.run[0].bank&&!g.run[0].history_count);
  assert(g.achievements&(1u<<(e+8)));assert(g.ending_count[e-1]==1);
  assert(!cc_flip(&g,0,0).heads);g.ending_pending=0;
 }
 assert(g.achievements==0x3FFF);assert(g.best_streak==10);
 puts("endings: trigger sequence, all thresholds/branches, reset, all14 badges persist");
}
static void midair_and_profiles(void){
 CCGame g;int head;CCResult r;cc_new(&g,5);g.run[0].bank=10000;
 cc_flip(&g,0,0);head=cc_begin(&g,0);assert(head==1&&g.run[0].streak==2);
 assert(cc_begin(&g,0)==-1);assert(cc_buy(&g,CC_COMBO)&&cc_buy(&g,CC_VALUE)&&cc_buy(&g,CC_SPEED));
 assert(cc_duration_ms(&g)==1800);r=cc_land(&g,true,0);assert(r.payout==10&&!g.in_flight);
 assert(!cc_land(&g,true,0).payout);
 head=cc_begin(&g,.22f);assert(head==0);assert(cc_buy(&g,CC_CHANCE));r=cc_land(&g,false,0);assert(!r.heads);
 assert(cc_flip(&g,.22f,0).heads);
 g.active=1;assert(g.run[1].bank==0&&g.run[1].level[0]==0);cc_flip(&g,0,0);assert(g.run[1].bank==1);
 g.active=0;assert(g.run[0].level[0]==1);cc_restart(&g);assert(!g.run[0].bank&&g.run[1].bank==1&&g.achievements);
 cc_new(&g,1);assert(!cc_land(&g,true,0).payout);
 puts("midair: sampled odds, live denomination/combo/speed, separate profiles, safe API");
}
static void saves_and_history(void){
 CCGame g,copy,before;uint8_t bytes[CC_SAVE_SIZE],bad[CC_SAVE_SIZE];unsigned i;char s[32];
 cc_new(&g,4);for(i=0;i<100;++i)cc_flip(&g,1,0);assert(g.run[0].history_count==13);
 assert(g.run[0].history[11].kind==1&&g.run[0].history[11].streak==10);
 g.run[0].bank=CC_BANK_MAX;g.earned=CC_BANK_MAX;cc_flip(&g,0,0);assert(g.run[0].bank==CC_BANK_MAX&&g.earned==CC_BANK_MAX);
 cc_advance(&g,UINT32_MAX);cc_advance(&g,1);assert(g.played_ms==UINT32_MAX);
 g.active=1;for(i=0;i<9;++i)cc_flip(&g,0,0);cc_flip(&g,0,.95f);g.fish=1;g.commentary=1;
 cc_encode(&g,bytes);assert(cc_decode(&copy,bytes,sizeof bytes));cc_encode(&copy,bad);assert(!memcmp(bytes,bad,sizeof bytes));
 before=copy;bytes[8]=0;bytes[9]=0;bytes[10]=0;bytes[11]=0;assert(!cc_decode(&copy,bytes,sizeof bytes)&&!memcmp(&copy,&before,sizeof copy));
 cc_encode(&g,bytes);assert(!cc_decode(&copy,bytes,sizeof bytes-1));memcpy(bad,bytes,sizeof bad);bad[639]=1;assert(!cc_decode(&copy,bad,sizeof bad));
 cc_money(s,sizeof s,2251900);assert(!strcmp(s,"$22519.00"));cc_money(s,sizeof s,CC_BANK_MAX);assert(!strcmp(s,"$90000000000000.00"));
 puts("save codec: full profiles/badges/history roundtrip, semantic rejection, limits");
}
int main(void){economy();probability();endings();midair_and_profiles();saves_and_history();puts("game tests passed");return 0;}
