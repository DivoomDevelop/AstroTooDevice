#ifndef DIVOOM_RADIO_MENU_H
#define DIVOOM_RADIO_MENU_H
#include "lvgl.h"
#include "divoom_radio_browse.h"

/* 页面记忆只有小型值状态；退出释放全部LVGL对象/可见行，不持有JSON或音源。 */
typedef struct
{
    int32 tab;
    int32 parent;
    char parent_name[96];
    int32 focus[28];
    int32 scroll[28];
    uint8 column[3];
    uint8 pane;
    uint8 initialized;
    divoom_radio_browse_query filter;
} divoom_radio_menu_memory;

void *divoom_radio_menu_create(lv_obj_t *content, lv_obj_t *header, int32 menu_id, divoom_radio_menu_memory *memory);
void divoom_radio_menu_destroy(void *handle);
void divoom_radio_menu_refresh(void *handle);
/* 返回1表示请求退出音源子页；键回调仅投递业务意图与修改轻量导航状态。 */
uint8 divoom_radio_menu_key(void *handle, uint8 key);
/* 接收完整按压类型，页签合并连按预览与松手短按，避免一次按键跳过一项。 */
uint8 divoom_radio_menu_key_event(void *handle, uint8 key, uint8 press);
int divoom_radio_menu_probe(void *handle, const char *name, long *out);
#endif
