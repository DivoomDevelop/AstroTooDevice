#ifndef _divoom_led_control_h_
#define _divoom_led_control_h_

#include "divoom_build_config.h"

#if DIVOOM_BUILD_RTOS_DEVICE
#define DIVOOM_LED_CONTROL_MAX_NUM 64
#else
#define DIVOOM_LED_CONTROL_MAX_NUM 26
#endif

typedef struct _divoom_led_control_led_info
{
    uint8 r;
    uint8 g;
    uint8 b;
}divoom_led_control_led_info, *Pdivoom_led_control_led_info;
void divoom_led_control_init(void);
void divoom_led_control_one_rgb(int led_id, Pdivoom_led_control_led_info pled_info);
void divoom_led_control_all_rgb(Pdivoom_led_control_led_info pled_info);
void divoom_led_control_switch_rgb_led(int on_off);
void divoom_led_control_switch_rgb_led(int on_off);
void divoom_led_control_control_white_birghness(int brightness);
void divoom_led_control_switch_rgb_on_off(int on_off);

#endif

