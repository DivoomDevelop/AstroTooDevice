#ifndef _divoom_astrootoo_face_market_h_
#define _divoom_astrootoo_face_market_h_

/*
 * AstroToo 表盘应用市场（顶栏三固定 tab + 三列列表：名称 / 收藏 / 删除）。
 * 全部：我的表盘 clock_history 原顺序；收藏/最近：我的表盘子集，按最后使用顺序优先展示。
 * UI 对照 AstroTooUI WatchFaceManageScreen。
 */

/** 本机最多可保存的表盘数量（与 AstroTooUI WATCH_FACE_MAX_COUNT 一致） */
#define DIVOOM_ASTRO_FACE_MARKET_MAX_DOWNLOAD     15

/** 顶栏固定三 tab（与 AstroTooUI WATCH_FACE_MANAGE_TABS 一致） */
#define DIVOOM_ASTRO_FACE_MARKET_TAB_RECENT       0
#define DIVOOM_ASTRO_FACE_MARKET_TAB_FAVORITE     1
#define DIVOOM_ASTRO_FACE_MARKET_TAB_ALL          2
#define DIVOOM_ASTRO_FACE_MARKET_TAB_COUNT        3

void *divoom_astrootoo_face_market_init(void *user_data, int menu_id);
void divoom_astrootoo_face_market_disp(void *handle, void *active_obj);
void divoom_astrootoo_face_market_clean(void *handle);
void divoom_astrootoo_face_market_pause(void *handle);

#endif
