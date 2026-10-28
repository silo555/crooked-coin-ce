#include "ui.h"
#include "render.h"
#include <stdio.h>
#include <string.h>
static void toast(CCUI *u,const char *s){snprintf(u->toast,sizeof(u->toast),"%s",s);u->toast_ms=2200;}
static void view(CCUI *u,uint8_t screen){u->screen=screen;u->selection=0;u->page=0;u->view_ms=0;}
static void changed(CCUI *u){u->dirty=1;u->saved=0;u->save_requested=1;}
void cc_ui_init(CCUI *u,const CCGame *g){
 memset(u,0,sizeof(*u));u->game=*g;u->screen=g->ending_pending?VIEW_END:g->tutorial_seen?VIEW_PLAY:VIEW_HELP;
}
void cc_ui_saved(CCUI *u,bool success){
 u->save_requested=0;
 if(success){u->dirty=0;u->saved=1;u->save_error=0;if(u->screen==VIEW_SAVE_ERROR)view(u,u->game.ending_pending?VIEW_END:VIEW_PLAY);}
 else{u->save_error=1;u->quit_requested=0;view(u,VIEW_SAVE_ERROR);}
}
void cc_ui_key(CCUI *u,uint8_t key){
 CCGame *g=&u->game;
 if(u->screen==VIEW_LOAD_ERROR){
  if(key==KEY_BACK)u->quit_requested=2;
  if(key==KEY_ENTER)u->load_requested=1;
  if(key==KEY_MENU&&u->load_problem==1){u->confirm_kind=5;view(u,VIEW_CONFIRM);}
  return;
 }
 if(u->flipping){
  if(key==KEY_SHOP){view(u,VIEW_SHOP);return;}
  if(key==KEY_BACK){view(u,VIEW_PLAY);return;}
  if(u->screen!=VIEW_SHOP)return;
 }
 if(u->screen==VIEW_SAVE_ERROR){
  if(key==KEY_ENTER)u->save_requested=1;
  else if(key==KEY_BACK){u->confirm_kind=3;view(u,VIEW_CONFIRM);}
  return;
 }
 if(u->screen==VIEW_CONFIRM){
  if(key==KEY_BACK){view(u,u->confirm_kind==5?VIEW_LOAD_ERROR:u->save_error?VIEW_SAVE_ERROR:VIEW_MENU);return;}
  if(key==KEY_LEFT||key==KEY_RIGHT)u->selection^=1;
  if(key==KEY_ENTER){
   if(!u->selection){view(u,u->confirm_kind==5?VIEW_LOAD_ERROR:u->save_error?VIEW_SAVE_ERROR:VIEW_MENU);return;}
   if(u->confirm_kind==3){u->quit_requested=2;return;}
   if(u->confirm_kind==4){cc_restart(g);g->commentary^=1;changed(u);view(u,VIEW_HELP);return;}
   if(u->confirm_kind==1)cc_restart(g);
   else{uint32_t seed=g->rng;cc_new(g,seed);g->tutorial_seen=1;}
   changed(u);view(u,VIEW_PLAY);toast(u,"A FRESH COIN. THE SAME ODDS.");
  }return;
 }
 if(u->screen==VIEW_END){
  if(key==KEY_ENTER&&u->view_ms>=1000){g->ending_pending=0;changed(u);view(u,VIEW_PLAY);}
  if(key==KEY_BACK){changed(u);u->quit_requested=1;}
  return;
 }
 if(key==KEY_BACK){
  if(u->screen==VIEW_PLAY)view(u,VIEW_MENU);else view(u,VIEW_PLAY);return;
 }
 if(u->screen==VIEW_PLAY){
  switch(key){
   case KEY_ENTER:
    if(!u->flipping){u->pending_head=(uint8_t)cc_begin(g,cc_roll(g));u->flipping=1;u->animation_ms=0;u->flip_ms=cc_duration_ms(g);}
    break;
   case KEY_SHOP:view(u,VIEW_SHOP);break;case KEY_STATS:view(u,VIEW_STATS);break;
   case KEY_BADGES:view(u,VIEW_BADGES);break;case KEY_HELP:view(u,VIEW_HELP);break;
   case KEY_MENU:view(u,VIEW_MENU);break;
   case KEY_FISH:g->fish^=1;changed(u);toast(u,g->fish?"THE MINT HAS ACQUIRED A FISH.":"THE FISH HAS LEFT THE MINT.");break;
  }return;
 }
 if(u->screen==VIEW_SHOP){
  if(key==KEY_UP)u->selection=(u->selection+3)%4;
  if(key==KEY_DOWN)u->selection=(u->selection+1)%4;
  if(key==KEY_ENTER){
   if(cc_buy(g,u->selection)){changed(u);toast(u,"UPGRADE PURCHASED");}
   else toast(u,cc_price(g,u->selection)?"NOT ENOUGH CHANGE":"ALREADY AT MAXIMUM");
  }
 }else if(u->screen==VIEW_STATS){
  if(key==KEY_LEFT||key==KEY_RIGHT)u->page^=1;
 }else if(u->screen==VIEW_BADGES){
  if(key==KEY_UP&&u->selection)u->selection--;
  if(key==KEY_DOWN&&u->selection<13)u->selection++;
 }else if(u->screen==VIEW_HELP){
  if(key==KEY_LEFT&&u->page)u->page--;
  if(key==KEY_RIGHT||key==KEY_ENTER){
   if(u->page<6)u->page++;else{g->tutorial_seen=1;changed(u);view(u,VIEW_PLAY);}
  }
 }else if(u->screen==VIEW_MENU){
  if(key==KEY_UP)u->selection=(u->selection+7)%8;
  if(key==KEY_DOWN)u->selection=(u->selection+1)%8;
  if(key==KEY_ENTER){switch(u->selection){
   case 0:changed(u);u->quit_requested=1;break;
   case 1:g->reduced_motion^=1;changed(u);break;
   case 2:u->confirm_kind=4;view(u,VIEW_CONFIRM);break;
   case 3:g->active^=1;changed(u);view(u,VIEW_PLAY);toast(u,g->active?"APRIL PROFILE - SEPARATE PROGRESS":"STANDARD PROFILE");break;
   case 4:view(u,VIEW_HELP);break;
   case 5:u->confirm_kind=1;view(u,VIEW_CONFIRM);break;
   case 6:u->confirm_kind=2;view(u,VIEW_CONFIRM);break;
   case 7:view(u,VIEW_PLAY);break;
  }}
 }
}
void cc_ui_tick(CCUI *u,uint32_t ms){
 if(ms>1000)ms=1000;
 u->view_ms+=ms;
 if(u->toast_ms)u->toast_ms=u->toast_ms>ms?u->toast_ms-ms:0;
 if(u->screen!=VIEW_SAVE_ERROR)cc_advance(&u->game,ms);
 if(u->flipping){
  u->animation_ms+=ms;
  u->flip_ms=cc_duration_ms(&u->game);
  if(u->animation_ms>=u->flip_ms){
   float ending=0.f;
   /* The PC draws the ending roll only after the successful tenth head. */
   if(u->game.run[u->game.active].streak==10&&u->pending_head)ending=cc_roll(&u->game);
   u->result=cc_land(&u->game,u->pending_head!=0,ending);u->flipping=0;changed(u);
   if(u->result.ending)view(u,VIEW_END);
   else if(u->result.new_badges)toast(u,"NEW STREAK BADGE UNLOCKED");
  }
 }
}
static void header(const char *title){
 r_rect(0,0,320,240,R_BG);r_rect(0,0,320,29,R_PANEL);
 r_rect(10,9,8,10,R_GOLD);r_rect(12,7,4,14,R_LIGHT);
 r_text(26,11,title,R_WHITE,1);r_line(0,28,320,R_EDGE);
}
static void footer(const char *left,const char *right){
 r_rect(0,220,320,20,R_PANEL);r_line(0,220,320,R_EDGE);
 r_text(9,227,left,R_MINT,1);r_text(320-(int)strlen(right)*6-9,227,right,R_MUTED,1);
}
static void amount(int x,int y,uint64_t money,uint8_t col,int scale){char b[28];cc_money(b,sizeof b,money);r_text(x,y,b,col,scale);}
static void number(int x,int y,uint32_t n,uint8_t c){char b[16];snprintf(b,sizeof b,"%lu",(unsigned long)n);r_text(x,y,b,c,1);}
static void clocktext(int x,int y,uint32_t ms){char b[24];uint32_t s=ms/1000;snprintf(b,sizeof b,"%lu:%02lu:%02lu",(unsigned long)(s/3600),(unsigned long)(s/60%60),(unsigned long)(s%60));r_text(x,y,b,R_WHITE,1);}
static void shop_value(const CCGame *g,int shop,char *b,size_t n){
 switch(shop){case CC_CHANCE:snprintf(b,n,"%u%% HEADS",20+5*g->run[g->active].level[0]);break;
 case CC_SPEED:snprintf(b,n,"%u.%u SECONDS",cc_duration_ms(g)/1000,cc_duration_ms(g)/100%10);break;
 case CC_COMBO:snprintf(b,n,"%u.%uX PER HEAD",(unsigned)(cc_combo(g)*10)/10,(unsigned)(cc_combo(g)*10)%10);break;
 default:cc_money(b,n,cc_denomination(g));break;}
}
static void play(const CCUI *u){
 const CCGame *g=&u->game;const CCRun *run=&g->run[g->active];char b[40];int i,cy=117,width=100,face=0;
 const CCMessage *last=NULL;
 for(i=run->history_count;i>0;--i)if(!run->history[i-1].kind){last=&run->history[i-1];break;}
 header(g->active?"TOTALLY FAIR COIN / APRIL":"CROOKED COIN CE");
 r_text(12,40,g->active?"YOUR TALENT":"YOUR CHANGE",R_MUTED,1);amount(12,54,run->bank,R_WHITE,run->bank<1000000000u?2:1);
 snprintf(b,sizeof b,"%u%% HEADS",20+5*run->level[0]);r_text(246,42,b,R_MINT,1);
 snprintf(b,sizeof b,"%u.%uX COMBO",(unsigned)(cc_combo(g)*10)/10,(unsigned)(cc_combo(g)*10)%10);r_text(246,57,b,R_MUTED,1);
 r_ellipse(161,161,41,5,R_BLACK);
 if(u->flipping&&!g->reduced_motion){
  static const uint8_t widths[8]={100,72,32,8,32,72,100,72};
  unsigned stage=u->animation_ms/70;
  uint32_t p=u->animation_ms*1000/u->flip_ms;
  cy-= (int)(4*p*(1000-p)/40000);width=widths[stage%8];face=(stage/4)%2;
 }else if(!u->flipping&&last)face=!last->streak;
 if(g->fish){r_ellipse(264,133,15,7,R_TEAL);r_rect(279,127,6,12,R_TEAL);r_rect(255,131,2,2,R_WHITE);}
 r_coin(159,cy,29,width,face);
 if(u->flipping){
  r_text(12,94,"IN THE AIR",R_MUTED,1);r_rect(12,108,70,3,R_EDGE);
  r_rect(12,108,(int)(70u*u->animation_ms/u->flip_ms),3,R_GOLD);
 }else if(last){
  r_text(12,95,g->active?(last->streak?"SKILL!":"PRACTICE!"):(last->streak?"HEADS":"TAILS"),last->streak?R_GOLD:R_MUTED,1);
  if(last->payout){r_text(12,111,"+",R_MINT,1);amount(20,111,last->payout,R_MINT,1);}
  else r_text(12,111,"TRY AGAIN",R_MUTED,1);
 }
 r_text(245,95,"COIN",R_MUTED,1);amount(245,109,cc_denomination(g),R_GOLD,1);
 r_text(245,135,"FLIPS",R_MUTED,1);number(245,149,run->flips,R_WHITE);
 snprintf(b,sizeof b,"HEADS IN A ROW: %u / 10",run->streak);r_center(170,b,R_MUTED,1);
 for(i=0;i<10;++i){r_rect(85+i*15,184,10,4,i<run->streak?R_GOLD:R_EDGE);}
 r_rect(83,198,154,18,u->flipping?R_PANEL:R_GOLD);
 r_center(204,u->flipping?"WAIT FOR THE COIN":g->active?"ENTER / 2ND  APPLY SKILL":"ENTER / 2ND  FLIP",u->flipping?R_MUTED:R_DARK,1);
 footer("Y= SHOP  WIN STATS","ZOOM BADGES  GRAPH MENU");
}
static void shop(const CCUI *u){
 char b[40];int i;header("THE MINT / UPGRADES");
 r_text(12,38,"AVAILABLE",R_MUTED,1);amount(83,38,u->game.run[u->game.active].bank,R_GOLD,1);
 for(i=0;i<4;++i){int y=55+i*39;uint32_t cost=cc_price(&u->game,(uint8_t)i);unsigned lv=u->game.run[u->game.active].level[i];
  r_rect(9,y,302,34,u->selection==i?R_RAISED:R_PANEL);
  if(u->selection==i){r_border(9,y,302,34,R_GOLD);r_rect(9,y,3,34,R_GOLD);}
  r_text(18,y+6,cc_shop_names[i],R_WHITE,1);shop_value(&u->game,i,b,sizeof b);r_text(18,y+20,b,R_MINT,1);
  snprintf(b,sizeof b,"LV %u/%u",lv,cc_max_level[i]);r_text(180,y+6,b,R_MUTED,1);
  if(cost)amount(242,y+20,cost,u->game.run[u->game.active].bank>=cost?R_GOLD:R_MUTED,1);
  else r_text(242,y+20,"MAX",R_MINT,1);
 }
 footer("UP/DOWN SELECT  ENTER BUY","CLEAR BACK");
}
static void stats(const CCUI *u){
 const CCGame *g=&u->game;const CCRun *run=&g->run[g->active];int i;char b[48];header(u->page?"THE LEDGER / RECENT FLIPS":"THE LEDGER / STATISTICS");
 if(!u->page){
  r_text(12,40,"CURRENT RUN",R_GOLD,1);r_text(174,40,"ALL PROFILES",R_GOLD,1);
  r_text(12,61,"FLIPS",R_MUTED,1);number(112,61,run->flips,R_WHITE);
  r_text(174,61,"FLIPS",R_MUTED,1);number(242,61,g->flips,R_WHITE);
  r_text(12,80,"STREAK",R_MUTED,1);number(112,80,run->streak,R_WHITE);
  r_text(174,80,"HEADS",R_MUTED,1);number(242,80,g->heads,R_WHITE);
  r_text(12,99,"TIME",R_MUTED,1);clocktext(76,99,g->run_ms[g->active]);
  r_text(174,99,"BEST",R_MUTED,1);number(257,99,g->best_streak,R_WHITE);
  r_line(12,119,296,R_EDGE);r_text(12,132,"LIFETIME EARNED",R_MUTED,1);amount(174,132,g->earned,R_MINT,1);
  r_text(12,151,"TIME IN THE MINT",R_MUTED,1);clocktext(174,151,g->played_ms);
  r_text(12,177,"ENDINGS",R_GOLD,1);
  for(i=0;i<5;++i){snprintf(b,sizeof b,"%u:%lu",i+1,(unsigned long)g->ending_count[i]);r_text(12+i*61,194,b,R_WHITE,1);}
 }else{
  r_text(12,39,"OLDEST TO NEWEST / LAST 13 RESULTS",R_MUTED,1);
  if(!run->history_count)r_center(118,"THE LEDGER IS WAITING FOR A FLIP.",R_MUTED,1);
  for(i=0;i<run->history_count;++i){int y=54+i*12;const CCMessage *m=&run->history[i];
   if(m->kind){r_text(15,y,cc_message_text(m),m->kind==2?R_MINT:R_MUTED,1);}
   else{snprintf(b,sizeof b,m->streak?"HEADS %uX":"TAILS",m->streak);r_text(15,y,b,m->streak?R_GOLD:R_MUTED,1);amount(230,y,m->payout,R_MINT,1);}
  }
 }
 footer("LEFT/RIGHT CHANGE PAGE","CLEAR BACK");
}
static void badges(const CCUI *u){
 unsigned i,first=u->selection/7*7,count=0;char b[48];header("THE COLLECTION / ACHIEVEMENTS");
 for(i=0;i<14;++i)if(u->game.achievements&(1u<<i))++count;
 snprintf(b,sizeof b,"%u / 14 COLLECTED",count);r_text(12,39,b,R_GOLD,1);
 for(i=first;i<first+7&&i<14;++i){int y=57+(i-first)*21;bool yes=(u->game.achievements&(1u<<i))!=0;
  if(u->selection==i)r_rect(9,y-3,302,18,R_RAISED);
  r_rect(16,y,8,8,yes?R_GOLD:R_EDGE);r_text(34,y,cc_badge_names[i],yes?R_WHITE:R_MUTED,1);
  if(i<9){snprintf(b,sizeof b,"%u HEAD%s",i+1,i?"S":"");r_text(249,y,b,R_MUTED,1);}
  else r_text(249,y,"ENDING",R_MUTED,1);
 }
 r_text(12,207,u->selection<9?"EARN A CONSECUTIVE HEADS STREAK.":"DISCOVER THIS FINAL FLIP OUTCOME.",R_MINT,1);
 footer("UP/DOWN BROWSE","CLEAR BACK");
}
static const char *const lessons[7][7]={
 {"A COIN WITH AN AGENDA","FLIP UNTIL YOU GET TEN HEADS IN A ROW.","THE COIN STARTS AT ONLY 20% HEADS.","TAILS BREAKS YOUR STREAK.","YOUR MONEY AND UPGRADES STAY.","", "ENTER / RIGHT: NEXT"},
 {"EVERY HEAD COUNTS","A HEAD PAYS YOUR COIN'S CENT VALUE.","CONSECUTIVE HEADS RAISE THE PAYOUT.","POWER = COMBO TO THE (STREAK - 1).", "ROUND THAT POWER UP, THEN MULTIPLY", "BY THE COIN'S VALUE. TAILS PAYS ZERO.","EXAMPLE: 5 CENTS X CEIL(1.5) = 10."},
 {"A BETTER SORT OF UNFAIR","Y= OPENS THE MINT'S FOUR UPGRADES.","HEADS CHANCE: +5 POINTS, UP TO 60%.","SPEED: 2.0 SECONDS DOWN TO 1.0.","COMBO: 1.0X UP TO 3.5X.","COINS: 1, 5, 10, 25, THEN 100 CENTS.","ARROWS SELECT. ENTER BUYS."},
 {"ONE LAST FLIP","A SUCCESSFUL TENTH HEAD ENDS A RUN.","IT PAYS NO MONEY. FIVE FATES AWAIT.","PROGRESS RESETS FOR ANOTHER ATTEMPT.","YOUR LOCAL BADGES STAY WITH YOU.","WINDOW SHOWS STATS AND FLIP HISTORY.","ZOOM OPENS YOUR BADGE COLLECTION."},
 {"KEEP YOUR CHANGE","COMPLETED FLIPS AND PURCHASES SAVE.","TWO CHECKED COPIES GUARD PROGRESS.","IF A SAVE FAILS, RETRY OR CONFIRM", "QUITTING WITHOUT SAVING.","GRAPH OPENS SETTINGS AND SAFE QUIT.","RESETS ALWAYS REQUIRE CONFIRMATION."},
 {"TWO SIDES OF THE SAME JOKE","APRIL MODE CHANGES THE PRESENTATION.","THE ODDS AND PRICES ARE IDENTICAL.","EACH PROFILE KEEPS ITS OWN PROGRESS.","REDUCED MOTION KEEPS THE FLIP TIMING.","THIS CALCULATOR HAS NO GAME AUDIO.","STEAM FEATURES STAY ON YOUR PC."},
 {"MINTED FOR THE CE","ORIGINAL ART, WORDS, AND NATIVE C.","INSPIRED BY UNFAIR FLIPS, CREATED", "BY HEATHER FLOWERS. PLEASE SUPPORT", "THE ORIGINAL GAME ON STEAM.","CROOKED COIN CE IS AN INDEPENDENT", "FAN ADAPTATION. ENTER TO PLAY."}
};
static const char *const commentary[7][7]={
 {"MINT NOTES / THE ADAPTATION","THIS IS ORIGINAL COMMENTARY ABOUT", "BRINGING THE RULES TO A CALCULATOR.","THE COMMERCIAL GAME'S PERSONAL", "DEVELOPER ESSAYS REMAIN IN THE PC", "GAME. THIS IS NOT THEIR TRANSCRIPT.","ENTER / RIGHT: NEXT"},
 {"MINT NOTES / THE ECONOMY","THE ROUNDING ORDER MATTERS.","A 5-CENT COIN AT A 1.5X POWER", "EARNS 10 CENTS, NOT 8 CENTS.","THE POWER IS ROUNDED UP FIRST.","PRICES FOLLOW THE INSTALLED GAME'S", "SCENE DATA, NOT WEB ESTIMATES."},
 {"MINT NOTES / THE SCREEN","320 BY 240 IS PLENTY FOR A COIN.","THE SHOP GETS ITS OWN PAGE SO", "NUMBERS STAY LARGE ENOUGH TO READ.","ALL PIXEL ART IS DRAWN FOR THIS", "ADAPTATION. THE COIN'S CROOKED", "MINT MARK IS OUR OWN DESIGN."},
 {"MINT NOTES / THE FINAL HEAD","THE ORDINARY HEADS ROLL COMES FIRST.","ONLY ITS TENTH SUCCESS IN A ROW", "DRAWS THE SEPARATE ENDING ROLL.","THE FINAL HEAD EARNS NO PAYOUT.","FIVE OUTCOMES FOLLOW THE ORIGINAL", "20 / 20 / 20 / 30 / 10 SPLIT."},
 {"MINT NOTES / MEMORY","A SAVE IS MORE THAN A PILE OF BYTES.","EACH RECORD CARRIES A VERSION,", "GENERATION NUMBER, AND CHECKSUM.","THE PREVIOUS COPY STAYS AVAILABLE", "WHILE THE NEXT ONE IS WRITTEN.","A BAD COPY CAN BE RECOVERED."},
 {"MINT NOTES / WHAT CHANGES","LOCAL BADGES REPLACE STEAM BADGES.","THE LEDGER ADDS LIFETIME TOTALS.","APRIL GETS SEPARATE PROGRESSION.","SOUND, STEAM CLOUD, AND STORE LINKS", "HAVE NO CALCULATOR EQUIVALENT.","SEE THE REPOSITORY'S PARITY NOTES."},
 {"MINT NOTES / THANK YOU","A SMALL MACHINE CAN HOLD A VERY", "PERSISTENT COIN. THANKS FOR PLAYING.","ORIGINAL GAME: HEATHER FLOWERS.","TOOLS: CEDEV AND THE CE LIBRARIES.","SOURCE AND LIMITATIONS ARE PUBLIC", "WHEN THE REPOSITORY IS RELEASED."}
};
static void help(const CCUI *u){
 int i;char b[16];const char *const *page=u->game.commentary?commentary[u->page]:lessons[u->page];header("THE HANDBOOK");
 snprintf(b,sizeof b,"%u / 7",u->page+1);r_text(270,11,b,R_MUTED,1);
 r_text(12,45,page[0],R_GOLD,1);r_line(12,61,296,R_EDGE);
 for(i=1;i<7;++i)r_text(12,68+i*20,page[i],i==6?R_MINT:R_WHITE,1);
 footer("LEFT/RIGHT  ENTER NEXT","CLEAR BACK");
}
static void menu(const CCUI *u){
 const CCGame *g=&u->game;const char *items[8]={"SAVE AND QUIT",g->reduced_motion?"MOTION: REDUCED":"MOTION: FULL",g->commentary?"MINT NOTES: ON":"MINT NOTES: OFF",g->active?"PROFILE: APRIL":"PROFILE: STANDARD","HOW TO PLAY / CREDITS","RESTART CURRENT RUN","ERASE ALL PROGRESS","BACK TO THE COIN"};int i;
 header("MINT OFFICE / OPTIONS");
 for(i=0;i<8;++i){int y=40+i*21;if(u->selection==i){r_rect(9,y-4,302,19,R_RAISED);r_rect(9,y-4,3,19,R_GOLD);}
  r_text(20,y,items[i],i==5||i==6?R_RED:R_WHITE,1);
 }
 footer("UP/DOWN SELECT  ENTER CHANGE","CLEAR BACK");
}
static void ending(const CCUI *u){
 const CCGame *g=&u->game;int cx=160,cy=109,i;uint32_t t=u->view_ms;header("THE FINAL FLIP");
 r_center(43,cc_end_names[g->last_ending],R_GOLD,2);
 switch(g->last_ending){
  case CC_EDGE:r_ellipse(161,149,33,4,R_BLACK);r_coin(cx,cy,28,8,0);break;
  case CC_VISITOR:r_ellipse(cx,cy,28,24,R_LIGHT);r_ellipse(cx,cy+4,20,19,R_GOLD);
   r_rect(cx-11,cy-6,4,4,R_DARK);r_rect(cx+7,cy-6,4,4,R_DARK);r_line(cx-7,cy+6,14,R_SHADE);
   for(i=-1;i<=1;i+=2){r_rect(cx+i*30-2,cy+10,7,3,R_GOLD);r_rect(cx+i*21-1,cy-26,2,7,R_GOLD);}break;
  case CC_SKY:
   for(i=0;i<15;++i)r_rect((i*43+17)%300+10,76+(i*29)%70,2,2,R_MUTED);
   cy=g->reduced_motion?94:135-(int)((t<2600?t:2600)/22);r_coin(cx,cy,12,80,0);break;
  case CC_SHATTER:
   for(i=0;i<16;++i){int d=(g->reduced_motion?100:(int)(t%2000))/30;
    int x=cx+((i*37)%63-31)*(d+7)/12,y=cy+((i*23)%51-25)*(d+6)/12;
    r_rect(x,y,3+i%4,3+i%3,i%2?R_GOLD:R_LIGHT);}break;
  case CC_TEN:r_coin(cx,cy,27,100,0);r_rect(144,72,33,5,R_GOLD);
   r_rect(144,62,6,12,R_GOLD);r_rect(157,59,6,15,R_GOLD);r_rect(171,62,6,12,R_GOLD);break;
 }
 {static const char *const lines[6][2]={{"",""},{"IT FOUND A THIRD SIDE.","THE MINT DECLINES TO COMMENT."},{"SOMETHING SMALL WAVES HELLO.","THAT WAS NOT IN THE ODDS TABLE."},{"THE COIN HAS OTHER PLANS.","YOUR CEILING IS SOMEONE'S SKY."},{"THE PRESSURE GOT TO IT.","PLEASE COLLECT YOUR CHANGE."},{"TEN HEADS. ONE IMPOSSIBLE MOMENT.","YOU AND THE COIN FINALLY AGREE."}};
  r_center(158,lines[g->last_ending][0],R_WHITE,1);r_center(175,lines[g->last_ending][1],R_MUTED,1);
 }
 r_center(201,"RUN RESET / BADGES AND TOTALS KEPT",R_MINT,1);
 footer(t<1000?"THE COIN SETTLES...":"ENTER NEW RUN","CLEAR SAVE + QUIT");
}
void cc_ui_draw(const CCUI *u){
 switch(u->screen){case VIEW_PLAY:play(u);break;case VIEW_SHOP:shop(u);break;case VIEW_STATS:stats(u);break;
 case VIEW_BADGES:badges(u);break;case VIEW_HELP:help(u);break;case VIEW_MENU:menu(u);break;case VIEW_END:ending(u);break;
 case VIEW_CONFIRM:header("PLEASE CONFIRM");r_center(65,u->confirm_kind==3?"QUIT WITHOUT SAVING?":u->confirm_kind==4?"RESTART TO CHANGE MINT NOTES?":u->confirm_kind==1?"RESTART THIS RUN?":"ERASE BOTH PROFILES?",R_RED,1);
  r_center(95,u->confirm_kind==1||u->confirm_kind==4?"BADGES AND LIFETIME TOTALS STAY.":u->confirm_kind==3?"UNSAVED CHANGES WILL BE LOST.":"ALL LOCAL PROGRESS WILL BE RESET.",R_WHITE,1);
  r_center(118,"THIS CANNOT BE UNDONE IN GAME.",R_MUTED,1);
  r_rect(62,156,85,25,u->selection?R_PANEL:R_GOLD);r_text(89,165,"NO",u->selection?R_WHITE:R_DARK,1);
  r_rect(173,156,85,25,u->selection?R_RED:R_PANEL);r_text(199,165,"YES",u->selection?R_DARK:R_WHITE,1);
  footer("LEFT/RIGHT  ENTER CONFIRM","CLEAR CANCEL");break;
 case VIEW_SAVE_ERROR:header("PROGRESS NEEDS ATTENTION");r_center(61,u->save_problem==4?"CCSAVE APPVAR NAME CONFLICT":"SAVE COULD NOT BE VERIFIED",R_RED,1);
  r_center(90,"YOUR GAME IS STILL IN MEMORY.",R_WHITE,1);r_center(111,u->save_problem==4?"BACK UP AND CHECK THE CC APPVARS.":"FREE SOME STORAGE IF NEEDED.",R_MUTED,1);
  r_center(132,"DO NOT DELETE UNKNOWN APPVARS.",R_MUTED,1);r_center(165,"ENTER RETRIES THE SAVE.",R_MINT,1);
  footer("ENTER RETRY","CLEAR QUIT OPTIONS");break;
 case VIEW_LOAD_ERROR:header("SAVE RECOVERY");
  r_center(58,u->load_problem==2?"APPVAR NAME CONFLICT":"NO VALID SAVE COULD BE LOADED",R_RED,1);
  r_center(86,"EXISTING APPVARS HAVE BEEN PRESERVED.",R_WHITE,1);
  r_center(107,"ENTER RETRIES. CLEAR QUITS SAFELY.",R_MINT,1);
  if(u->load_problem==1){r_center(143,"GRAPH: CONFIRM A FRESH START",R_GOLD,1);r_center(163,"THIS REPLACES DAMAGED CC SAVES.",R_MUTED,1);}
  else{r_center(143,"BACK UP CCSAVE0 AND CCSAVE1.",R_GOLD,1);r_center(163,"CHECK THEM WITH TI CONNECT CE.",R_MUTED,1);}
  footer("ENTER RETRY","CLEAR QUIT");break;
 }
 if(u->toast_ms&&u->screen!=VIEW_END){r_rect(5,3,310,23,R_RAISED);r_border(5,3,310,23,R_EDGE);r_center(11,u->toast,R_MINT,1);}
 if(u->save_warning&&u->screen==VIEW_PLAY){r_rect(69,29,182,10,R_DARK);r_text(74,31,"RECOVERED PREVIOUS SAVE COPY",R_GOLD,1);}
}
