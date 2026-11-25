#ifndef __divoom_game_matchpic_h
#define __divoom_game_matchpic_h

uint16 divoom_disp_game_matchpic_init(Pdivoom_disp_game_ctrl handle);
void divoom_disp_game_matchpic_main(void);
void divoom_disp_game_matchpic_get_task_id(uint32 task_id);
void divoom_disp_game_matchpic_close(void);
void divoom_disp_game_matchpic_start(void);
void divoom_disp_game_matchpic_key(uint8 key_flag);
uint8 divoom_disp_game_matchpic_get_game_status(void);


#endif