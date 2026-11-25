#ifndef _DIVOOM_SAVE_TO_SD_
#define _DIVOOM_SAVE_TO_SD_

char *divoom_save_to_sd_get_buf(void);
void divoom_save_to_sd_log_write(char *buf, int cur_size);
void divoom_save_to_sd_start(void);
void divoom_save_to_sd_exit(void);
#endif
