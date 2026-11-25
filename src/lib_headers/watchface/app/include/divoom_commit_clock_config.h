#ifndef _divoom_commit_clock_config_h_

#define _divoom_commit_clock_config_h_

void divoom_commit_clock_config_init(void);

void divoom_commit_clock_config_update(int clock_id, char *item_id, char *item_value, char *item_name);
void divoom_commit_clock_get_config(Pdivoom_channel_app_clock_config_info pclock_info);


#endif

