#ifndef _divoom_save_all_config_h_

#define _divoom_save_all_config_h_

void divoom_save_all_config_init(void);

typedef void DIVOOM_SAVE_ALL_CONFIG_CALLBACK(void);

void divoom_save_all_config_add(DIVOOM_SAVE_ALL_CONFIG_CALLBACK *pcallback);
void divoom_save_all_config_add_sync_run(DIVOOM_SAVE_ALL_CONFIG_CALLBACK *pcallback);

#endif

