#include "divoom.h"
#include "lvgl.h"
#include "divoom_lvgl_top_radio_wait.h"
#include "divoom_radio.h"
#include "divoom_menu_control.h"
#if defined(_MSC_VER)
#include <intrin.h>
#endif

/* 单个定时器兼顾状态检查和转动。只更新 52px 圆弧，空闲时没有动画对象。 */
#define DIVOOM_RADIO_WAIT_TICK_MS 67
#define DIVOOM_RADIO_WAIT_TURN_MS 1000
#define DIVOOM_RADIO_WAIT_LIMIT_MS 30000

enum
{
    DIVOOM_RADIO_WAIT_HIDDEN,
    DIVOOM_RADIO_WAIT_VISIBLE,
    DIVOOM_RADIO_WAIT_DISMISS
};

typedef struct
{
    uint32 start_tick;
    uint32 request;
    uint8 tracking;
    uint8 dismissed;
} divoom_lvgl_top_radio_wait_request;

typedef struct
{
    lv_timer_t *timer;
    lv_obj_t *panel;
    lv_obj_t *arc;
    divoom_lvgl_top_radio_wait_request playback;
    divoom_lvgl_top_radio_wait_request catalog;
    uint32 catalog_sequence;
    uint32 catalog_request;
    int32 catalog_view;
    uint8 catalog_loading;
    volatile int32 key_state; /* 仅此字段在按键线程与 LVGL 线程间共享。 */
} divoom_lvgl_top_radio_wait_ctrl;

static divoom_lvgl_top_radio_wait_ctrl g_divoom_lvgl_top_radio_wait;

static void divoom_lvgl_top_radio_wait_publish(int32 state)
{
#if defined(_MSC_VER)
    _InterlockedExchange((volatile long *)&g_divoom_lvgl_top_radio_wait.key_state, state);
#else
    __atomic_store_n(&g_divoom_lvgl_top_radio_wait.key_state, state, __ATOMIC_RELEASE);
#endif
}

void divoom_lvgl_top_radio_wait_dismiss_on_key(void)
{
    /* 只收起按键时已经显示的提示，发起起播的按键不会吞掉下一帧的提示。 */
#if defined(_MSC_VER)
    _InterlockedCompareExchange((volatile long *)&g_divoom_lvgl_top_radio_wait.key_state,
        DIVOOM_RADIO_WAIT_DISMISS, DIVOOM_RADIO_WAIT_VISIBLE);
#else
    int32 expected = DIVOOM_RADIO_WAIT_VISIBLE;
    __atomic_compare_exchange_n(&g_divoom_lvgl_top_radio_wait.key_state, &expected,
        DIVOOM_RADIO_WAIT_DISMISS, 0, __ATOMIC_RELEASE, __ATOMIC_RELAXED);
#endif
}

static void divoom_lvgl_top_radio_wait_deleted(lv_event_t *event)
{
    (void)event;
    g_divoom_lvgl_top_radio_wait.panel = NULL;
    g_divoom_lvgl_top_radio_wait.arc = NULL;
    divoom_lvgl_top_radio_wait_publish(DIVOOM_RADIO_WAIT_HIDDEN);
}

static void divoom_lvgl_top_radio_wait_create(void)
{
    divoom_lvgl_top_radio_wait_ctrl *ctrl = &g_divoom_lvgl_top_radio_wait;
    ctrl->panel = lv_obj_create(lv_layer_top());
    if (!ctrl->panel)
    {
        return;
    }
    lv_obj_remove_style_all(ctrl->panel);
    lv_obj_set_style_border_width(ctrl->panel, 0, 0);
    lv_obj_set_style_pad_all(ctrl->panel, 0, 0);
    lv_obj_clear_flag(ctrl->panel, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(ctrl->panel, 80, 80);
    lv_obj_set_style_bg_color(ctrl->panel, lv_color_hex(0xf2f0eb), 0);
    lv_obj_set_style_bg_opa(ctrl->panel, LV_OPA_70, 0);
    lv_obj_set_style_radius(ctrl->panel, 12, 0);
    lv_obj_center(ctrl->panel);
    lv_obj_add_event_cb(ctrl->panel, divoom_lvgl_top_radio_wait_deleted, LV_EVENT_DELETE, NULL);

    ctrl->arc = lv_arc_create(ctrl->panel);
    if (!ctrl->arc)
    {
        lv_obj_del(ctrl->panel);
        return;
    }
    lv_obj_remove_style_all(ctrl->arc);
    lv_obj_clear_flag(ctrl->arc, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(ctrl->arc, 52, 52);
    lv_obj_center(ctrl->arc);
    lv_obj_set_style_arc_color(ctrl->arc, lv_color_hex(0xddd8d0), LV_PART_MAIN);
    lv_obj_set_style_arc_color(ctrl->arc, lv_color_hex(0x243746), LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(ctrl->arc, 5, LV_PART_MAIN);
    lv_obj_set_style_arc_width(ctrl->arc, 5, LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(ctrl->arc, 1, LV_PART_INDICATOR);
    lv_arc_set_bg_angles(ctrl->arc, 0, 360);
    lv_arc_set_angles(ctrl->arc, 0, 100);
    divoom_lvgl_top_radio_wait_publish(DIVOOM_RADIO_WAIT_VISIBLE);
}

void divoom_lvgl_top_radio_wait_set_catalog(uint8 loading, int32 view, uint32 request)
{
    divoom_lvgl_top_radio_wait_ctrl *ctrl = &g_divoom_lvgl_top_radio_wait;
    /* 切视图、发起新目录请求或重新进入加载状态都可显示下一次提示。 */
    if (loading && (!ctrl->catalog_loading || view != ctrl->catalog_view ||
        request != ctrl->catalog_request))
    {
        ctrl->catalog_view = view;
        ctrl->catalog_request = request;
        ctrl->catalog_sequence++;
    }
    ctrl->catalog_loading = loading;
}

static uint8 divoom_lvgl_top_radio_wait_update(divoom_lvgl_top_radio_wait_request *wait,
    uint8 active, uint32 request, uint32 now)
{
    if (!active)
    {
        wait->tracking = 0;
        return 0;
    }
    if (!wait->tracking || request != wait->request)
    {
        wait->tracking = 1;
        wait->dismissed = 0;
        wait->request = request;
        wait->start_tick = now;
    }
    return !wait->dismissed && now - wait->start_tick < DIVOOM_RADIO_WAIT_LIMIT_MS;
}

static void divoom_lvgl_top_radio_wait_poll(lv_timer_t *timer)
{
    (void)timer;
    divoom_lvgl_top_radio_wait_ctrl *ctrl = &g_divoom_lvgl_top_radio_wait;
#if defined(_MSC_VER)
    int32 key_state = _InterlockedCompareExchange((volatile long *)&ctrl->key_state, 0, 0);
#else
    int32 key_state = __atomic_load_n(&ctrl->key_state, __ATOMIC_ACQUIRE);
#endif
    uint8 key_dismissed = key_state == DIVOOM_RADIO_WAIT_DISMISS;
    if (key_dismissed)
    {
        /* 收起本次已跟踪的等待；下面识别到的新请求继续使用自己的等待状态。 */
        ctrl->playback.dismissed = 1;
        ctrl->catalog.dismissed = 1;
    }
    uint32 request = 0;
    uint8 starting = divoom_radio_is_starting(&request);
    uint32 now = lv_tick_get();
    uint8 playback_visible = divoom_lvgl_top_radio_wait_update(&ctrl->playback, starting, request, now);
    uint8 catalog_visible = divoom_lvgl_top_radio_wait_update(&ctrl->catalog,
        ctrl->catalog_loading, ctrl->catalog_sequence, now);
    /* 起播和目录共用同一面板。收起提示不改变目录请求或播放状态。 */
    if ((!playback_visible && !catalog_visible) || key_dismissed)
    {
        if (ctrl->panel)
        {
            lv_obj_del(ctrl->panel);
        }
        return;
    }
    if (!ctrl->panel)
    {
        divoom_lvgl_top_radio_wait_create();
    }
    if (!ctrl->arc)
    {
        return;
    }
    uint32 elapsed = now - (playback_visible ? ctrl->playback.start_tick : ctrl->catalog.start_tick);
    uint16 angle = (uint16)((elapsed % DIVOOM_RADIO_WAIT_TURN_MS) * 360 / DIVOOM_RADIO_WAIT_TURN_MS);
    /* 轨道静止，只改变指示弧，LVGL 可按弧段范围刷新。 */
    lv_arc_set_angles(ctrl->arc, angle, angle + 100);
}

void divoom_lvgl_top_radio_wait_init(void)
{
    if (!g_divoom_lvgl_top_radio_wait.timer)
    {
        g_divoom_lvgl_top_radio_wait.timer = lv_timer_create(divoom_lvgl_top_radio_wait_poll,
            DIVOOM_RADIO_WAIT_TICK_MS, NULL);
    }
}
