#ifndef DIVOOM_LVGL_INIT_H
#define DIVOOM_LVGL_INIT_H

#ifndef DIVOOM_LVGL_ENABLE_APP
#define DIVOOM_LVGL_ENABLE_APP      1
#endif

#ifndef DIVOOM_LVGL_ENABLE_TEST
#define DIVOOM_LVGL_ENABLE_TEST     0
#endif

#ifndef DIVOOM_LVGL_AUTO_BOOT
#define DIVOOM_LVGL_AUTO_BOOT       0
#endif

#ifndef DIVOOM_LVGL_BOOT_MODE
#define DIVOOM_LVGL_BOOT_MODE       DIVOOM_LVGL_BOOT_APP
#endif

/* Set to 1 in src/lvgl/Makefile when divoom_main link port is complete. */
#ifndef CONFIG_DIVOOM_MAIN_PORT_READY
#define CONFIG_DIVOOM_MAIN_PORT_READY  1
#endif

#define DIVOOM_LVGL_BOOT_APP        0
#define DIVOOM_LVGL_BOOT_TEST_MIN   1
#define DIVOOM_LVGL_BOOT_TEST_MAX   7

#ifdef __cplusplus
extern "C" {
#endif

void divoom_lvgl_boot(void);

int divoom_lvgl_app_init(void);
int divoom_lvgl_app_is_ready(void);

void divoom_get_lvgl_lock(void);
void divoom_free_lvgl_lock(void);
int divoom_get_lvgl_try_lock(void);

int divoom_aw_pic_decoder_global_init(void);
int divoom_aw_pic_decoder_lock(void);
void divoom_aw_pic_decoder_unlock(void);

#ifdef __cplusplus
}
#endif

#endif
