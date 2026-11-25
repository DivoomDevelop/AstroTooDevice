#ifndef __divoom_game_frogger_h
#define __divoom_game_frogger_h

uint16 divoom_disp_game_frogger_init(Pdivoom_disp_game_ctrl handle);
void divoom_disp_game_frogger_main(void);
void divoom_disp_game_frogger_get_task_id(uint32 task_id);
void divoom_disp_game_frogger_close(void);
void divoom_disp_game_frogger_start(void);
void divoom_disp_game_frogger_key(uint8 key_flag);
uint8 divoom_disp_game_frogger_get_game_status(void);


#endif