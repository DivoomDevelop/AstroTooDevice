#ifndef __divoom_game_ball_h
#define __divoom_game_ball_h

uint16 divoom_disp_game_ball_init(Pdivoom_disp_game_ctrl handle);
void divoom_disp_game_ball_main(void);
void divoom_disp_game_ball_get_task_id(uint32 task_id);
void divoom_disp_game_ball_close(void);
void divoom_disp_game_ball_start(void);
void divoom_disp_game_ball_key(uint8 key_flag);
uint8 divoom_disp_game_ball_get_game_status(void);


#endif

