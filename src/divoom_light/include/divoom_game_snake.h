#ifndef __divoom_game_snake_h
#define __divoom_game_snake_h

uint16 divoom_disp_game_snake_init(Pdivoom_disp_game_ctrl handle);
void divoom_disp_game_snake_main(void);
void divoom_disp_game_snake_get_task_id(uint32 task_id);
void divoom_disp_game_snake_close(void);
void divoom_disp_game_snake_start(void);
void divoom_disp_game_snake_key(uint8 key_flag);
uint8 divoom_disp_game_snake_get_game_status(void);


#endif