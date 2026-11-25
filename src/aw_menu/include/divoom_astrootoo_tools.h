#ifndef _divoom_astrootoo_tools_h_
#define _divoom_astrootoo_tools_h_

/*
 * AstroToo 实用工具（实用工具列表 + 子工具页）
 * 真源设计: AstroTooUI flowtoo ToolsScreen / StopwatchScreen / CountdownScreen /
 *          ScoreboardScreen / NoiseMeterScreen / WarmMemoScreen / PomodoroScreen
 * 功能逻辑参考工程: divoom_lvgl_stop_watch_menu.c 等。
 * 倒计时/记分牌与 divoom_app_tool 同步，承接 App Tools/SetTimer、Tools/SetScoreBoard。
 *
 * 单一文件按 menu_id 分发：列表 + 6 个子工具页共用一个 ctrl 与 init/clean/disp/pause。
 */

typedef enum
{
    DIVOOM_ASTROTOO_TOOLS_PAGE_LIST = 0,
    DIVOOM_ASTROTOO_TOOLS_PAGE_POMODORO,
    DIVOOM_ASTROTOO_TOOLS_PAGE_STOPWATCH,
    DIVOOM_ASTROTOO_TOOLS_PAGE_COUNTDOWN,
    DIVOOM_ASTROTOO_TOOLS_PAGE_SCOREBOARD,
    DIVOOM_ASTROTOO_TOOLS_PAGE_MEMO,
    DIVOOM_ASTROTOO_TOOLS_PAGE_NOISE,
} divoom_astrootoo_tools_page_t;

void *divoom_astrootoo_tools_init(void *user_data, int menu_id);
void divoom_astrootoo_tools_disp(void *handle, void *active_obj);
void divoom_astrootoo_tools_clean(void *handle);
void divoom_astrootoo_tools_pause(void *handle);

/* 白盒探针只读访问（sim_whitebox_probe） */
int divoom_astrootoo_tools_whitebox_get_pomo_mode(void);
int divoom_astrootoo_tools_whitebox_get_pomo_view(void);
int divoom_astrootoo_tools_whitebox_get_pomo_work_min(void);
int divoom_astrootoo_tools_whitebox_get_stopwatch_running(void);
int divoom_astrootoo_tools_whitebox_get_stopwatch_elapsed_sec(void);
int divoom_astrootoo_tools_whitebox_get_countdown_cd_status(void);
int divoom_astrootoo_tools_whitebox_get_countdown_remain_s(void);
int divoom_astrootoo_tools_whitebox_get_score_left(void);
int divoom_astrootoo_tools_whitebox_get_score_right(void);
int divoom_astrootoo_tools_whitebox_get_noise_db(void);
int divoom_astrootoo_tools_whitebox_get_page(void);
int divoom_astrootoo_tools_whitebox_get_list_focus(void);
int divoom_astrootoo_tools_whitebox_get_record_state(void);
int divoom_astrootoo_tools_whitebox_get_record_clip_sec(void);
int divoom_astrootoo_tools_memo_is_playing(void);
void divoom_astrootoo_tools_memo_sync_ui_idle(void);

/* APP 远程启动且已在番茄页：在 LVGL 线程同步 UI（勿在非 LVGL 线程 lv_scr_load） */
void divoom_astrootoo_tools_pomo_request_sync_active_page(void);

/* 阶段到时叠 256×256 完成动画；非番茄页直接 return。须在 LVGL 线程外用 lv_async */
void divoom_astrootoo_tools_pomo_request_notice_anim(void);

#endif
