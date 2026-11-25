#ifndef __divoom_game_random_h
#define __divoom_game_random_h

uint16 divoom_disp_game_random_init(Pdivoom_disp_game_ctrl handle,uint16 id);
void divoom_disp_game_random_main(void);
void divoom_disp_game_random_get_task_id(uint32 task_id);
void divoom_disp_game_random_close(void);
void divoom_disp_game_random_start(void);
void divoom_disp_game_random_key(uint8 key_flag);
uint8 divoom_disp_game_random_get_game_status(void);


#endif

