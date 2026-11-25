#ifndef _divoom_feishu_calendar_h_
#define _divoom_feishu_calendar_h_

void divoom_feishu_calendar_update_switch(int pop_flag, int event_flag);
void divoom_feishu_calendar_update_token(int pop_flag, int event_flag, char *token);
void divoom_feishu_calendar_update_extra(char *calendar_id, char *union_id);
int divoom_feishu_calendar_check_login(void);
void divoom_feishu_calendar_init(void);
int divoom_feishu_calendar_get_update_flag(void);
int divoom_feishu_calendar_load_calenday_current_month(void **queue_item);
int divoom_feishu_calendar_get_active_lvgl_event_info(void *data);
void divoom_feishu_calendar_user_cancle_event(void);
void divoom_feishu_calendar_update_menu_flag(uint8 menu_flag);

#endif
