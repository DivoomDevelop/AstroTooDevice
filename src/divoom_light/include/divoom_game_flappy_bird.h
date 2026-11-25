#ifndef __divoom_game_flappy_bird_h
#define __divoom_game_flappy_bird_h

uint16 divoom_disp_game_flappy_bird_init(Pdivoom_disp_game_ctrl handle);
void divoom_disp_game_flappy_bird_main(void);
void divoom_disp_game_flappy_bird_get_task_id(uint32 task_id);
void divoom_disp_game_flappy_bird_close(void);
void divoom_disp_game_flappy_bird_start(void);
void divoom_disp_game_flappy_bird_key(uint8 key_flag);
uint8 divoom_disp_game_flappy_bird_get_game_status(void);


#endif