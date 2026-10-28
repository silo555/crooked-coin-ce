#include "ui.h"
#include "render.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void tick(CCUI *u,unsigned ms){while(ms){unsigned step=ms>20?20:ms;cc_ui_tick(u,step);ms-=step;}}
static unsigned hash(void){unsigned i,h=2166136261u;for(i=0;i<sizeof cc_frame;++i)h=(h^cc_frame[i])*16777619u;return h;}
int main(void){
 CCGame g;CCUI u;unsigned i,j,a,b;cc_new(&g,123);cc_ui_init(&u,&g);assert(u.screen==VIEW_HELP);
 for(i=0;i<7;++i){cc_ui_draw(&u);cc_ui_key(&u,KEY_ENTER);}assert(u.screen==VIEW_PLAY&&u.game.tutorial_seen);
 u.game.run[0].bank=100000;cc_ui_key(&u,KEY_ENTER);assert(u.flipping&&u.game.in_flight);
 cc_ui_key(&u,KEY_ENTER);assert(u.game.flips==1);tick(&u,500);
 cc_ui_key(&u,KEY_SHOP);assert(u.screen==VIEW_SHOP);cc_ui_key(&u,KEY_DOWN);cc_ui_key(&u,KEY_ENTER);
 assert(u.game.run[0].level[1]==1&&u.save_requested&&u.flipping);tick(&u,1300);assert(!u.flipping);
 cc_ui_saved(&u,false);assert(u.screen==VIEW_SAVE_ERROR&&u.dirty);
 cc_ui_key(&u,KEY_ENTER);assert(u.save_requested);cc_ui_saved(&u,true);assert(u.screen==VIEW_PLAY&&!u.dirty);
 /* Every reset defaults to NO; cancelling never changes progression. */
 cc_ui_key(&u,KEY_MENU);for(i=0;i<5;++i)cc_ui_key(&u,KEY_DOWN);cc_ui_key(&u,KEY_ENTER);assert(u.screen==VIEW_CONFIRM&&!u.selection);
 cc_ui_key(&u,KEY_ENTER);assert(u.game.run[0].flips==1);for(i=0;i<5;++i)cc_ui_key(&u,KEY_DOWN);cc_ui_key(&u,KEY_ENTER);cc_ui_key(&u,KEY_RIGHT);cc_ui_key(&u,KEY_ENTER);assert(!u.game.run[0].flips);
 /* Loading a saved HEADS renders exactly like the live saved frame. */
 cc_flip(&u.game,0,0);u.screen=VIEW_PLAY;u.toast_ms=0;cc_ui_draw(&u);a=hash();g=u.game;cc_ui_init(&u,&g);cc_ui_draw(&u);b=hash();assert(a==b);
 /* Error/recovery flow cannot silently reset or save default state. */
 u.screen=VIEW_LOAD_ERROR;u.load_problem=1;u.save_requested=0;cc_ui_key(&u,KEY_MENU);assert(u.screen==VIEW_CONFIRM&&!u.save_requested);cc_ui_key(&u,KEY_BACK);assert(u.screen==VIEW_LOAD_ERROR&&!u.save_requested);
 u.load_problem=2;cc_ui_key(&u,KEY_MENU);assert(u.screen==VIEW_LOAD_ERROR&&!u.save_requested);cc_ui_key(&u,KEY_ENTER);assert(u.load_requested);
 /* Render all menu selections, badge pages, handbook pages, and endings. */
 cc_new(&g,6);g.tutorial_seen=1;cc_ui_init(&u,&g);
 for(i=0;i<=VIEW_LOAD_ERROR;++i){u.screen=(uint8_t)i;
  for(j=0;j<14;++j){u.selection=(uint8_t)j;u.page=(uint8_t)(j%7);
   if(i==VIEW_HELP||i==VIEW_STATS||i==VIEW_MENU||i==VIEW_BADGES||i==VIEW_PLAY||i==VIEW_SHOP||i>=VIEW_CONFIRM)cc_ui_draw(&u);
  }
 }
 for(i=1;i<=5;++i){u.screen=VIEW_END;u.game.last_ending=(uint8_t)i;for(j=0;j<3000;j+=37){u.view_ms=j;cc_ui_draw(&u);}}
 u.screen=VIEW_PLAY;u.flipping=1;u.flip_ms=2000;
 for(i=0;i<2000;i+=17){u.animation_ms=i;cc_ui_draw(&u);}
 assert(!cc_draw_errors);puts("UI: actual controls, timing, midair shop, reset confirmations, reload, save failure/retry, all views/ending frames passed");return 0;
}
