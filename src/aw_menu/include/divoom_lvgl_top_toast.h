#ifndef _divoom_lvgl_top_toast_h_
#define _divoom_lvgl_top_toast_h_

/* 顶层纯文字 toast（lv_layer_top）。duration_ms=0 一直显示直到 hide。 */
void divoom_lvgl_top_toast_show(const char *text, unsigned int duration_ms);
void divoom_lvgl_top_toast_hide(void);
void divoom_lvgl_top_toast_hide_timed(void);
void divoom_lvgl_top_toast_poll(void);

#endif
