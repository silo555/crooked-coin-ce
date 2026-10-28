#include "render.h"
#include <string.h>
#ifndef CC_HOST
#include <graphx.h>
#endif
const uint8_t cc_palette[R_COUNT][3]={
 {9,29,34},{14,43,49},{20,56,62},{31,76,81},{43,115,116},{133,218,191},
 {237,244,225},{136,168,166},{226,170,55},{255,226,134},{160,101,32},
 {84,57,28},{238,119,103},{3,15,19}
};
#ifdef CC_HOST
uint8_t cc_frame[320*240];
unsigned cc_draw_errors;
#endif
void r_rect(int x,int y,int w,int h,uint8_t c){
 int yy;
 if(w<=0||h<=0)return;
 /* Clipping also handles particles and the rising ending coin. */
 if(x<0){w+=x;x=0;} if(y<0){h+=y;y=0;}
 if(x+w>320)w=320-x; if(y+h>240)h=240-y;
 if(w<=0||h<=0||x>=320||y>=240)return;
#ifdef CC_HOST
 if(c>=R_COUNT){++cc_draw_errors;return;}
 for(yy=y;yy<y+h;++yy)memset(cc_frame+yy*320+x,c,(unsigned)w);
#else
 (void)yy; gfx_SetColor(c); gfx_FillRectangle_NoClip(x,y,w,h);
#endif
}
void r_line(int x,int y,int w,uint8_t c){r_rect(x,y,w,1,c);}
void r_border(int x,int y,int w,int h,uint8_t c){
 r_line(x,y,w,c);r_line(x,y+h-1,w,c);r_rect(x,y,1,h,c);r_rect(x+w-1,y,1,h,c);
}
/* Original compact 5x7 alphabet. Each row is five bits, left to right. */
static const char alphabet[]="ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 .,:!?-/+%=()<>$'";
static const uint8_t font[][7]={
 {14,17,17,31,17,17,17},{30,17,17,30,17,17,30},{14,17,16,16,16,17,14},
 {30,17,17,17,17,17,30},{31,16,16,30,16,16,31},{31,16,16,30,16,16,16},
 {14,17,16,23,17,17,15},{17,17,17,31,17,17,17},{14,4,4,4,4,4,14},
 {7,2,2,2,18,18,12},{17,18,20,24,20,18,17},{16,16,16,16,16,16,31},
 {17,27,21,21,17,17,17},{17,25,25,21,19,19,17},{14,17,17,17,17,17,14},
 {30,17,17,30,16,16,16},{14,17,17,17,21,18,13},{30,17,17,30,20,18,17},
 {15,16,16,14,1,1,30},{31,4,4,4,4,4,4},{17,17,17,17,17,17,14},
 {17,17,17,17,17,10,4},{17,17,17,21,21,27,17},{17,17,10,4,10,17,17},
 {17,17,10,4,4,4,4},{31,1,2,4,8,16,31},
 {14,17,19,21,25,17,14},{4,12,4,4,4,4,14},{14,17,1,2,4,8,31},
 {30,1,1,14,1,1,30},{2,6,10,18,31,2,2},{31,16,16,30,1,1,30},
 {14,16,16,30,17,17,14},{31,1,2,4,8,8,8},{14,17,17,14,17,17,14},
 {14,17,17,15,1,1,14},{0,0,0,0,0,0,0},{0,0,0,0,0,6,6},
 {0,0,0,0,0,4,8},{0,6,6,0,6,6,0},{4,4,4,4,0,4,4},
 {14,17,1,2,4,0,4},{0,0,0,31,0,0,0},{1,2,2,4,8,8,16},
 {0,4,4,31,4,4,0},{17,2,4,8,17,0,0},{0,31,0,31,0,0,0},
 {2,4,8,8,8,4,2},{8,4,2,2,2,4,8},{2,4,8,16,8,4,2},
 {8,4,2,1,2,4,8},{4,15,20,14,5,30,4},{4,4,8,0,0,0,0}
};
void r_text(int x,int y,const char *s,uint8_t c,int scale){
 while(*s){int row,col;char ch=*s++;const char *p;
  if(ch>='a'&&ch<='z')ch=(char)(ch-32);
  p=strchr(alphabet,ch);
  if(p)for(row=0;row<7;++row)for(col=0;col<5;++col)
   if(font[p-alphabet][row]&(16>>col))r_rect(x+col*scale,y+row*scale,scale,scale,c);
  x+=6*scale;
 }
}
void r_center(int y,const char *s,uint8_t c,int scale){r_text((320-(int)strlen(s)*6*scale)/2,y,s,c,scale);}
void r_ellipse(int cx,int cy,int rx,int ry,uint8_t c){
 int y,x=rx;
 if(rx<1)rx=1;if(ry<1)ry=1;
 for(y=0;y<=ry;++y){
  while(x>0 && (long)x*x*ry*ry+(long)y*y*rx*rx>(long)rx*rx*ry*ry)--x;
  r_rect(cx-x,cy+y,2*x+1,1,c);if(y)r_rect(cx-x,cy-y,2*x+1,1,c);
 }
}
void r_coin(int cx,int cy,int radius,int width,int face){
 int i,rx=radius*width/100;
 if(rx<2)rx=2;
 r_ellipse(cx+2,cy+4,rx,radius,R_SHADE);
 r_ellipse(cx,cy,rx,radius,R_GOLD);
 r_ellipse(cx-1,cy-2,rx>3?rx-2:1,radius-3,R_LIGHT);
 r_ellipse(cx,cy,rx>5?rx-5:1,radius-7,R_GOLD);
 if(width>55){
  if(face==0){
   /* A crooked lightning cut: the adaptation's own mint mark. */
   for(i=0;i<6;++i)r_rect(cx-9+i*2,cy-16+i*3,15-i,3,R_SHADE);
   for(i=0;i<5;++i)r_rect(cx-1-i*2,cy+1+i*3,13-i,3,R_SHADE);
   r_rect(cx-6,cy-13,3,7,R_LIGHT);
  }else{r_text(cx-10,cy-13,"CE",R_SHADE,2);r_line(cx-10,cy+11,21,R_SHADE);}
  for(i=-2;i<=2;++i)r_rect(cx+i*6-1,cy+radius-5,2,2,R_SHADE);
 }
}
