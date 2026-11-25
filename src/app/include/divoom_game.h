#ifndef _divoom_game_h_
#define _divoom_game_h_
enum
{
    DIVOOM_GAME_APP_RUN_EVENT,
};

enum DIVOOM_GAME_COMMAND
{
    DIVOOM_GAME_START_MODE,
    DIVOOM_GAME_CTRL_MODE,
    DIVOOM_GAME_RESTART_MODE,
    DIVOOM_GAME_STOP_MODE,
    DIVOOM_GAME_UPDATE_MODE,
    DIVOOM_GAME_RESULT_MODE,
};

void divoom_game_init(void);

void divoom_game_deinit(void);

/**
 * *  @brief       	divoom_game_main
 				检查游戏是否操作未控制，如果超时，将取消游戏
 * *  @author      	aj
 * *  @date       	2016-02-24
 * *  @param[in]   none
				
 * *  @return        NONE
 * */
void divoom_game_main(void);

uint8 divoom_game_app_schedule_play(uint8 event_id, void *user_data, uint32 resouce_type);

uint8 divoom_game_app_schedule_pause(uint8 event_id, void *user_data, uint8 active_model, uint8 active_event, uint32 resouce_type);

uint8 divoom_game_app_schedule_destroy(uint8 event_id, void *user_data, uint32 status, uint32 resouce_type);

uint8 divoom_game_app_schedule_key_handle(uint8 key_id, uint8 key_type, uint32 resouce_type);
#endif


