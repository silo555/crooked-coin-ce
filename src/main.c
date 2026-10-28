#ifndef CC_HOST
#include "ui.h"
#include "render.h"
#include "storage_ce.h"
#include <graphx.h>
#include <keypadc.h>
#include <sys/timers.h>
#include <time.h>
#include <string.h>

static CCUI ui;
static uint8_t payload[CC_SAVE_SIZE];
static uint32_t generation;
static void graphics_begin(void){
 unsigned i;gfx_Begin();
 for(i=0;i<R_COUNT;++i)gfx_palette[i]=gfx_RGBTo1555(cc_palette[i][0],cc_palette[i][1],cc_palette[i][2]);
 gfx_SetDrawBuffer();
}
static uint8_t input(void){
 if(kb_Data[6]&kb_Enter)return KEY_ENTER;
 if(kb_Data[1]&kb_2nd)return KEY_ENTER;
 if(kb_Data[6]&kb_Clear)return KEY_BACK;
 if(kb_Data[7]&kb_Up)return KEY_UP;if(kb_Data[7]&kb_Down)return KEY_DOWN;
 if(kb_Data[7]&kb_Left)return KEY_LEFT;if(kb_Data[7]&kb_Right)return KEY_RIGHT;
 if(kb_Data[1]&kb_Yequ)return KEY_SHOP;
 if(kb_Data[1]&kb_Window)return KEY_STATS;
 if(kb_Data[1]&kb_Zoom)return KEY_BADGES;
 if(kb_Data[1]&kb_Trace)return KEY_HELP;
 if(kb_Data[1]&(kb_Graph|kb_Mode))return KEY_MENU;
 if(kb_Data[2]&kb_Alpha)return KEY_FISH;
 return KEY_NONE;
}
static void load(void){
 CCGame g;JournalResult status=storage_ce_load(payload,sizeof payload,&generation);
 cc_new(&g,(uint32_t)time(NULL)^(uint32_t)clock());
 if(status==JOURNAL_OK||status==JOURNAL_RECOVERED){
  if(!cc_decode(&g,payload,sizeof payload))status=JOURNAL_CORRUPT;
 }
 cc_ui_init(&ui,&g);
 if(status==JOURNAL_RECOVERED)ui.save_warning=1;
 if(status!=JOURNAL_OK&&status!=JOURNAL_EMPTY&&status!=JOURNAL_RECOVERED){
  ui.screen=VIEW_LOAD_ERROR;ui.load_problem=status==JOURNAL_COLLISION?2:status==JOURNAL_CORRUPT?1:3;
 }
}
int main(void){
 uint8_t previous=KEY_NONE;clock_t last;uint32_t remainder=0;
 graphics_begin();storage_ce_init(gfx_End,graphics_begin);storage_ce_set_validator(cc_validate_save);
 load();last=clock();
 for(;;){
  clock_t now=clock();uint32_t ticks=(uint32_t)(now-last);
  uint64_t scaled=(uint64_t)ticks*1000+remainder;uint32_t ms=(uint32_t)(scaled/CLOCKS_PER_SEC);
  uint8_t key;
  last=now;remainder=(uint32_t)(scaled%CLOCKS_PER_SEC);if(ms)cc_ui_tick(&ui,ms);
  kb_Scan();key=input();if(key&&key!=previous)cc_ui_key(&ui,key);previous=key;
  if(ui.load_requested){load();previous=KEY_NONE;last=clock();}
  if(ui.save_requested&&!ui.flipping){
   JournalResult status;
   cc_encode(&ui.game,payload);status=storage_ce_save(payload,sizeof payload,&generation);
   cc_ui_saved(&ui,status==JOURNAL_OK);
   if(status!=JOURNAL_OK){ui.save_problem=(uint8_t)status;}
  }
  if(ui.quit_requested&&(!ui.dirty||ui.quit_requested==2))break;
  cc_ui_draw(&ui);gfx_BlitBuffer();delay(16);
 }
 gfx_End();return 0;
}
#endif
