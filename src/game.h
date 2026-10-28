#ifndef CC_GAME_H
#define CC_GAME_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define CC_VERSION "0.1.1"
#define CC_HISTORY 13
#define CC_SAVE_SIZE 640
#define CC_BANK_MAX UINT64_C(9000000000000000)
enum { CC_CHANCE, CC_SPEED, CC_COMBO, CC_VALUE, CC_SHOPS };
enum { CC_NONE, CC_EDGE, CC_VISITOR, CC_SKY, CC_SHATTER, CC_TEN };
typedef struct { uint32_t payout; uint8_t streak,kind; } CCMessage;
typedef struct {
 uint64_t bank;
 uint32_t flips;
 uint8_t level[4],streak,history_count;
 CCMessage history[CC_HISTORY]; /* oldest first; streak zero means tails */
} CCRun;
typedef struct {
 CCRun run[2];
 uint64_t earned;
 uint32_t rng,flips,heads,played_ms,ending_count[5];
 uint32_t last_run_flips,last_run_ms,run_ms[2];
 uint16_t achievements;
 uint8_t active,reduced_motion,commentary,tutorial_seen,best_streak;
 uint8_t last_ending,ending_pending,fish;
 uint8_t in_flight,pending_head; /* runtime only; save only settled states */
} CCGame;
typedef struct { uint32_t payout; uint16_t new_badges; uint8_t heads,streak,ending; } CCResult;
extern const uint8_t cc_max_level[4];
extern const char *const cc_shop_names[4];
extern const char *const cc_end_names[6];
extern const char *const cc_badge_names[14];
void cc_new(CCGame *g,uint32_t seed);
void cc_restart(CCGame *g); /* current run only, achievements/statistics retained */
uint32_t cc_price(const CCGame *g,uint8_t shop);
bool cc_buy(CCGame *g,uint8_t shop);
float cc_chance(const CCGame *g);
float cc_combo(const CCGame *g);
uint16_t cc_duration_ms(const CCGame *g);
uint8_t cc_denomination(const CCGame *g);
uint32_t cc_payout(uint8_t denomination,uint8_t combo_level,uint8_t streak);
uint8_t cc_pick_ending(float roll);
CCResult cc_flip(CCGame *g,float head_roll,float ending_roll);
int cc_begin(CCGame *g,float head_roll);
CCResult cc_land(CCGame *g,bool heads,float ending_roll);
const char *cc_message_text(const CCMessage *m);
uint32_t cc_random(CCGame *g);
float cc_roll(CCGame *g);
void cc_advance(CCGame *g,uint32_t elapsed_ms);
void cc_encode(const CCGame *g,uint8_t out[CC_SAVE_SIZE]);
bool cc_decode(CCGame *g,const uint8_t *data,size_t len);
bool cc_validate_save(const uint8_t *data,size_t len,void *ctx);
void cc_money(char *buf,size_t size,uint64_t cents);
#endif
