#ifndef DIVOOM_PRODUCT_TEST_WIFI_INPUT_H
#define DIVOOM_PRODUCT_TEST_WIFI_INPUT_H
#include "lvgl.h"
void divoom_product_test_wifi_input_show(lv_obj_t *parent, int visible);
int divoom_product_test_wifi_input_key(int key);
void divoom_product_test_wifi_input_destroy(void);
#endif
