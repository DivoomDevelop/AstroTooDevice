#ifndef __divoom_game_supperzzle_h
#define __divoom_game_supperzzle_h

uint16 divoom_disp_game_supperzzle_init(Pdivoom_disp_game_ctrl handle);
void divoom_disp_game_supperzzle_main(void);
void divoom_disp_game_supperzzle_get_task_id(uint32 task_id);
void divoom_disp_game_supperzzle_close(void);
void divoom_disp_game_supperzzle_start(void);
void divoom_disp_game_supperzzle_key(uint8 key_flag);
uint8 divoom_disp_game_supperzzle_get_game_status(void);


#endif