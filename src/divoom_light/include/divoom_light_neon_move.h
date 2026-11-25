#ifndef _divoom_light_neon_move_h_
#define _divoom_light_neon_move_h_
// #include "anyka_types.h"
// #include "divoom_light_animation.h"



uint32 divoom_light_neon_move_init(void);
void divoom_light_neon_move_play(uint8 play);
void divoom_light_neon_move_close(void);
uint8 divoom_light_neon_move_center_get_index(uint8 x, uint8 y);
void divoom_light_neon_move_main(void);

void divoom_light_neon_move_get_taskid(uint32 id);

// void divoom_light_neon_cp_to_hbuf(uint8 buf[SCREEN_HEIGHT_32][SCREEN_WIDTH_32],uint8 src_buf[SCREEN_HEIGHT_32][SCREEN_WIDTH_32]);
// void divoom_light_neon_fil_block(T_S8 x,T_S8 y, T_S8 w,T_S8 h, uint8 buf[SCREEN_HEIGHT_32][SCREEN_WIDTH_32],uint8 *buf_base);
// void divoom_light_neon_hbuf_rotate(unsigned char* pSrc,int srcW,int srcH,unsigned char* pDst,int dstW,int dstH,double degree,int nchannel);

#endif
