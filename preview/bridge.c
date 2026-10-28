#include "ui.h"
#include "render.h"
#ifdef _WIN32
#define EXPORT __declspec(dllexport)
#else
#define EXPORT __attribute__((visibility("default")))
#endif
static CCUI ui;
EXPORT void preview_init(uint32_t seed){CCGame game;cc_new(&game,seed);cc_ui_init(&ui,&game);}
EXPORT void preview_key(uint8_t key){cc_ui_key(&ui,key);}
EXPORT void preview_tick(uint32_t ms){cc_ui_tick(&ui,ms);if(ui.save_requested&&!ui.flipping)cc_ui_saved(&ui,true);}
EXPORT uint8_t *preview_frame(void){cc_ui_draw(&ui);return cc_frame;}
EXPORT const uint8_t *preview_palette(void){return &cc_palette[0][0];}
EXPORT uint32_t preview_flips(void){return ui.game.run[ui.game.active].flips;}
EXPORT uint8_t preview_screen(void){return ui.screen;}
/* Only the local screenshot generator calls fixture. This is not a game key
 * or calculator cheat, and this source is excluded from the native build. */
EXPORT void preview_fixture(unsigned which){
 CCGame game;unsigned i;cc_new(&game,0x84ce);game.tutorial_seen=1;
 for(i=0;i<6;++i)cc_flip(&game,0,0);
 game.run[0].bank=12345;game.run[0].level[0]=3;game.run[0].level[1]=2;
 game.run[0].level[2]=2;game.run[0].level[3]=2;
 cc_ui_init(&ui,&game);
 if(which==1)ui.screen=VIEW_SHOP;
 if(which==2)ui.screen=VIEW_STATS;
 if(which==3){ui.screen=VIEW_BADGES;ui.selection=5;}
 if(which>=4&&which<=8){ui.game.last_ending=(uint8_t)(which-3);ui.game.ending_pending=1;ui.screen=VIEW_END;ui.view_ms=1500;}
 if(which==9){ui.screen=VIEW_PLAY;ui.game.active=1;ui.game.run[1]=game.run[0];}
}
