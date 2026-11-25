#ifndef _DIVOOM_LIGHT_BOMB_H_
#define _DIVOOM_LIGHT_BOMB_H_

//void divoom_light_bomb_init(void);
void divoom_light_bomb_init(uint8 *data);

void divoom_light_bomb_update(uint8 num);

void divoom_light_bomb_disp_pic(uint8 less_line, uint8 x, uint8 y, uint8 num, uint8 *disp_buf, uint8 pic_num);

void divoom_light_bomb_close(void);

void divoom_light_bomb_play(uint8 flag);
#endif





