#ifndef CC_UI_H
#define CC_UI_H
#include "game.h"
enum { KEY_NONE,KEY_ENTER,KEY_BACK,KEY_UP,KEY_DOWN,KEY_LEFT,KEY_RIGHT,
 KEY_SHOP,KEY_STATS,KEY_BADGES,KEY_HELP,KEY_MENU,KEY_FISH };
enum { VIEW_PLAY,VIEW_SHOP,VIEW_STATS,VIEW_BADGES,VIEW_HELP,VIEW_MENU,VIEW_END,VIEW_CONFIRM,VIEW_SAVE_ERROR,VIEW_LOAD_ERROR };
typedef struct {
 CCGame game;
 CCResult result;
 uint32_t animation_ms,view_ms,toast_ms;
 uint16_t flip_ms;
 uint8_t screen,selection,page,flipping,dirty,save_requested,quit_requested;
 uint8_t confirm_kind,save_error,save_warning,saved,ending_step,pending_head;
 uint8_t load_problem,load_requested,save_problem;
 char toast[48];
} CCUI;
void cc_ui_init(CCUI *u,const CCGame *g);
void cc_ui_key(CCUI *u,uint8_t key);
void cc_ui_tick(CCUI *u,uint32_t ms);
void cc_ui_draw(const CCUI *u);
void cc_ui_saved(CCUI *u,bool success);
#endif
