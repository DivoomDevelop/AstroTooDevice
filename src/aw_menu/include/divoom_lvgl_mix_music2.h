#ifndef _divoom_lvgl_mix_music2_h_
#define _divoom_lvgl_mix_music2_h_

typedef enum
{
    DIVOOM_LVGL_MIX_MUSIC2_PAGE_KEYBOARD = 1,
    DIVOOM_LVGL_MIX_MUSIC2_PAGE_RACK,
    DIVOOM_LVGL_MIX_MUSIC2_PAGE_PIANO,
    DIVOOM_LVGL_MIX_MUSIC2_PAGE_BELL,
    DIVOOM_LVGL_MIX_MUSIC2_PAGE_VIOLIN,
    DIVOOM_LVGL_MIX_MUSIC2_PAGE_MARACAS,
    DIVOOM_LVGL_MIX_MUSIC2_PAGE_SAXOPHONE,
    DIVOOM_LVGL_MIX_MUSIC2_PAGE_MAX = DIVOOM_LVGL_MIX_MUSIC2_PAGE_SAXOPHONE
}DIVOOM_LVGL_MIX_MUSIC2_PAGE_TYPE;

#define DIVOOM_LVGL_MIX_MUSIC2_ICON_IDLE 0xff

void *divoom_lvgl_mix_music2_init(void *user_data, int menu_id);
void divoom_lvgl_mix_music2_exit(void *handle);
void divoom_lvgl_mix_music2_disp(void *handle, void *active_obj);
void divoom_lvgl_mix_music2_pause(void *handle);

#endif
