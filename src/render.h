#ifndef CC_RENDER_H
#define CC_RENDER_H
#include <stdint.h>
enum { R_BG, R_PANEL, R_RAISED, R_EDGE, R_TEAL, R_MINT, R_WHITE,
       R_MUTED, R_GOLD, R_LIGHT, R_SHADE, R_DARK, R_RED, R_BLACK, R_COUNT };
extern const uint8_t cc_palette[R_COUNT][3];
void r_rect(int x,int y,int w,int h,uint8_t color);
void r_line(int x,int y,int w,uint8_t color);
void r_border(int x,int y,int w,int h,uint8_t color);
void r_text(int x,int y,const char *s,uint8_t color,int scale);
void r_center(int y,const char *s,uint8_t color,int scale);
void r_ellipse(int cx,int cy,int rx,int ry,uint8_t color);
void r_coin(int cx,int cy,int radius,int width,int face);
#ifdef CC_HOST
extern uint8_t cc_frame[320*240];
extern unsigned cc_draw_errors;
#endif
#endif
