#ifndef _divoom_clock_play_plan_h_
#define _divoom_clock_play_plan_h_

#include "divoom_type.h"

typedef struct _divoom_clock_play_plan_cycle_item
{
    uint8  is_enable;
    uint32 start_time;
    uint32 end_time;
    uint8  repeat_flag[7];
    int    clock_id;
    char   clock_name[64];
    char   image_pixel_id[64];
    uint32 start_utc; /* 本地 JSON StartUtc；单次 cycle 用，MQTT 不上报 */
    uint32 modify_seq; /* 本地修改序号，MQTT 不上报；重叠时越大越优先 */
}divoom_clock_play_plan_cycle_item, *Pdivoom_clock_play_plan_cycle_item;

typedef struct _divoom_clock_play_plan_date_item
{
    uint8  is_enable;
    uint8  month;
    uint8  day;
    uint32 start_time;
    uint32 end_time;
    int    clock_id;
    char   clock_name[64];
    char   image_pixel_id[64];
    uint32 start_utc; /* 本地 JSON StartUtc；MQTT 不上报 */
    uint32 modify_seq; /* 本地修改序号，MQTT 不上报；重叠时越大越优先 */
}divoom_clock_play_plan_date_item, *Pdivoom_clock_play_plan_date_item;

enum
{
    DIVOOM_CLOCK_PLAY_PLAN_RULE_NONE = 0,
    DIVOOM_CLOCK_PLAY_PLAN_RULE_CYCLE,
    DIVOOM_CLOCK_PLAY_PLAN_RULE_DATE,
    DIVOOM_CLOCK_PLAY_PLAN_RULE_TRADITIONAL,
};

enum
{
    DIVOOM_CLOCK_PLAY_PLAN_APP_EVENT_RUN,
};

enum
{
    DIVOOM_CLOCK_PLAY_PLAN_INIT_SWITCH, /* 已废弃：Get/SetClockPlayPlanSwitch */
    DIVOOM_CLOCK_PLAY_PLAN_INIT_CYCLE,
    DIVOOM_CLOCK_PLAY_PLAN_INIT_DATE,
};

void divoom_clock_play_plan_init(void);
void divoom_clock_play_plan_main(void);
void divoom_clock_play_plan_refresh(void);
void divoom_clock_play_plan_request_sync_all(void);
void divoom_clock_play_plan_user_dismiss(void);

uint8 divoom_clock_play_plan_take_internal_restore_flag(void);
uint8 divoom_clock_play_plan_is_holding_display(void);

uint8 divoom_clock_play_plan_app_schedule_play(uint8 event_id, void *user_data, uint32 resouce_type);
uint8 divoom_clock_play_plan_app_schedule_pause(uint8 event_id, void *user_data, uint8 active_model, uint8 active_event, uint32 resouce_type);
uint8 divoom_clock_play_plan_app_schedule_destroy(uint8 event_id, void *user_data, uint32 status, uint32 resouce_type);
uint8 divoom_clock_play_plan_app_schedule_key_handle(uint8 key_id, uint8 key_type, uint32 resouce_type);

#endif
