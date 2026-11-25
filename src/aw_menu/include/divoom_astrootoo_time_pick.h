#ifndef _divoom_astrootoo_time_pick_h_
#define _divoom_astrootoo_time_pick_h_

#include "divoom_type.h"
#include "divoom_astrootoo_settings_internal.h"

/* 对照 AstroTooUI flowtooTimePickInput · FlowtooTimePickField */
typedef enum
{
    DIVOOM_ASTRO_TP_FIELD_AMPM = 0,
    DIVOOM_ASTRO_TP_FIELD_HOUR,
    DIVOOM_ASTRO_TP_FIELD_MINUTE,
    DIVOOM_ASTRO_TP_FIELD_CANCEL,
    DIVOOM_ASTRO_TP_FIELD_CONFIRM,
} divoom_astrootoo_time_pick_field_t;

typedef enum
{
    DIVOOM_ASTRO_TP_KEY_HANDLED = 0,
    DIVOOM_ASTRO_TP_KEY_CANCEL,
    DIVOOM_ASTRO_TP_KEY_CONFIRM,
} divoom_astrootoo_time_pick_key_action_t;

typedef struct
{
    int use_hour24;
    int field;
    int hour24;
    int minute;
} divoom_astrootoo_time_pick_state_t;

int divoom_astrootoo_time_pick_use_hour24(void);
int divoom_astrootoo_time_pick_is_pm(int hour24);
int divoom_astrootoo_time_pick_initial_field(int use_hour24);
int divoom_astrootoo_time_pick_move_field(int field, int dir, int use_hour24);

int divoom_astrootoo_time_pick_move_value_field(int field, int dir, int use_hour24);

void divoom_astrootoo_time_pick_format_hour(char *buf, int len, int hour24, int use_hour24);
/** 状态栏/顶栏时刻：跟随系统 12/24h，12h 不带 AM/PM（真源 formatFlowtooClockLabel） */
void divoom_astrootoo_time_pick_format_clock_label(char *buf, int len, int hour24, int minute);
void divoom_astrootoo_time_pick_bump(divoom_astrootoo_time_pick_state_t *st, int dir);

divoom_astrootoo_time_pick_key_action_t divoom_astrootoo_time_pick_handle_key(
    divoom_astrootoo_time_pick_state_t *st, uint8 key_code, int esc_confirms);

void divoom_astrootoo_time_pick_refresh_widgets(const divoom_astrootoo_time_pick_widgets_t *widgets,
    int field, int hour24, int minute, int use_hour24);

lv_obj_t *divoom_astrootoo_time_pick_focus_obj(const divoom_astrootoo_time_pick_widgets_t *widgets,
    int field);

#endif
