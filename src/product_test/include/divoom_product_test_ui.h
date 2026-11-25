#ifndef _divoom_product_test_ui_h_
#define _divoom_product_test_ui_h_

#include "lvgl.h"

int divoom_product_test_ui_init(void);
void divoom_product_test_ui_show(void);
/* Call on the LVGL thread when a background file check fails. */
void divoom_product_test_ui_stop_aging_overlay(void);
/* Resource writer: pause/drain first, then resume on every success/failure exit. */
int divoom_product_test_ui_pause_aging_video(unsigned int timeout_ms);
void divoom_product_test_ui_resume_aging_video(void);
void *divoom_product_test_ui_lvgl_show(void);
void *divoom_product_test_ui_get_screen(void);
lv_group_t *divoom_product_test_ui_get_group(void);

void *divoom_product_test_screen_create(void);
void divoom_product_test_screen_show(void *handle);
void divoom_product_test_screen_destroy(void *handle);

#endif
