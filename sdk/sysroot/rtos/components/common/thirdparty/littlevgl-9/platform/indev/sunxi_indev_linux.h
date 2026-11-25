
#ifndef SUNXI_INDEV_LINUX_H
#define SUNXI_INDEV_LINUX_H

#if defined(LV_USE_SUNXI_INDEV) && defined(OS_LINUX)

#include <stdint.h>
#include "lvgl.h"
#include "lvgl_private.h"


void touchpad_init(void);
void touchpad_read(lv_indev_t * indev, lv_indev_data_t * data);
bool touchpad_is_pressed(void);
void touchpad_get_xy(int32_t * x, int32_t * y);
void mouse_init(void);
void mouse_read(lv_indev_t * indev, lv_indev_data_t * data);
bool mouse_is_pressed(void);
void mouse_get_xy(int32_t * x, int32_t * y);
void keypad_init(void);
void keypad_read(lv_indev_t * indev, lv_indev_data_t * data);
uint32_t keypad_get_key(void);
void encoder_init(void);
void encoder_read(lv_indev_t * indev, lv_indev_data_t * data);
void encoder_handler(void);
void button_init(void);
void button_read(lv_indev_t * indev, lv_indev_data_t * data);
int8_t button_get_pressed_id(void);
bool button_is_pressed(uint8_t id);

#endif

#endif