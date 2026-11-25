#ifndef _divoom_lvgl_top_phone_call_menu_h_
#define _divoom_lvgl_top_phone_call_menu_h_

#include <stdint.h>

void divoom_lvgl_top_phone_call_menu_notify(uint8_t call_type, const char *number,
					    uint8_t number_len, const char *name,
					    uint8_t name_len);
void divoom_lvgl_top_phone_call_menu_dismiss(void);
void divoom_lvgl_top_phone_call_menu_poll(void);
void divoom_lvgl_top_phone_call_menu_exit(void);
int divoom_lvgl_top_phone_call_menu_active(void);

#endif
