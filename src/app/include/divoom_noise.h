#ifndef _divoom_noise_h_
#define _divoom_noise_h_
enum{
    DIVOOM_NOISE_EVENT_GET_STATUS,
    DIVOOM_NOISE_EVENT_SET_STATUS,
};

enum
{
    DIVOOM_NOISE_APP_PLAY_EVENT,
};


void divoom_noise_init(void);

uint8 divoom_noise_app_schedule_play(uint8 event_id, void *user_data, uint32 resouce_type);

uint8 divoom_noise_app_schedule_pause(uint8 event_id, void *user_data, uint8 active_model, uint8 active_event, uint32 resouce_type);

uint8 divoom_noise_app_schedule_destroy(uint8 event_id, void *user_data, uint32 status, uint32 resouce_type);

uint8 divoom_noise_app_schedule_key_handle(uint8 key_id, uint8 key_type, uint32 resouce_type);

#endif

