#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"
#include "divoom_menu_direct_ctrl.h"
#include "divoom_astrootoo_ui_base.h"
#include "divoom_astrootoo_games.h"
#include "divoom_game_runtime.h"
#include "divoom_lvgl9_compat.h"

#define DIVOOM_ASTROTOO_GAMES_UI_DIR    "120/"
#define DIVOOM_ASTRO_GAMES_EMPTY_ROW    1

typedef enum
{
    DIVOOM_ASTROTOO_GAMES_PAGE_LIST = 0,
    DIVOOM_ASTROTOO_GAMES_PAGE_RUNNING,
} divoom_astrootoo_games_page_t;

typedef struct _divoom_astrootoo_games_ctrl
{
    divoom_astrootoo_ui_base ui;
    int menu_id;
    divoom_astrootoo_games_page_t page;
    lv_obj_t *root_obj;
    lv_obj_t *header;
    lv_obj_t *content;
    lv_obj_t *run_layer;
    lv_obj_t *key_btn;
    void *direct_handle;
    divoom_game_catalog_t catalog;
    lv_obj_t **row_btns;
    int row_count;
    int focus_row;
    lv_timer_t *launch_timer;
    int pending_launch_index;
    lv_obj_t *status_lbl;
} divoom_astrootoo_games_ctrl, *Pdivoom_astrootoo_games_ctrl;

static void divoom_astrootoo_games_stop_running(Pdivoom_astrootoo_games_ctrl phandle);
static void divoom_astrootoo_games_destroy_rows(Pdivoom_astrootoo_games_ctrl phandle);
static void divoom_astrootoo_games_build_list(Pdivoom_astrootoo_games_ctrl phandle);
static int divoom_astrootoo_games_launch(Pdivoom_astrootoo_games_ctrl phandle, int index);
static void divoom_astrootoo_games_set_list_input(Pdivoom_astrootoo_games_ctrl phandle);
static void divoom_astrootoo_games_set_run_input(Pdivoom_astrootoo_games_ctrl phandle);
static void divoom_astrootoo_games_group_bind_list(Pdivoom_astrootoo_games_ctrl phandle,
    lv_obj_t *focus_target);
static void divoom_astrootoo_games_row_activate(Pdivoom_astrootoo_games_ctrl phandle, int row);
static void divoom_astrootoo_games_request_launch(Pdivoom_astrootoo_games_ctrl phandle, int row);

static void divoom_astrootoo_games_log_mem(const char *stage)
{
    divoom_print("[astro_games] mem stage=%s total_free_kb=%ld largest_free_kb=%ld\n",
        stage ? stage : "",
        divoom_system_get_ram_free_size(),
        divoom_system_get_ram_largest_free_block_size());
}

static void divoom_astrootoo_games_set_label_font(Pdivoom_astrootoo_games_ctrl phandle,
    lv_obj_t *label, int font_size)
{
    if (phandle == NULL || label == NULL)
    {
        return;
    }
    lv_obj_set_style_text_font(label,
        divoom_menu_control_get_font_info(font_size, phandle->menu_id), 0);
}

static void divoom_astrootoo_games_group_bind_list(Pdivoom_astrootoo_games_ctrl phandle,
    lv_obj_t *focus_target)
{
    lv_group_t *g;
    lv_obj_t *focus;
    int i;

    if (phandle == NULL)
    {
        return;
    }
    g = lv_group_get_default();
    if (!g)
    {
        return;
    }
    lv_group_remove_all_objs(g);
    for (i = 0; i < phandle->row_count; i++)
    {
        if (phandle->row_btns && phandle->row_btns[i] && lv_obj_is_valid(phandle->row_btns[i]))
        {
            lv_group_add_obj(g, phandle->row_btns[i]);
        }
    }
    focus = NULL;
    if (focus_target && lv_obj_is_valid(focus_target))
    {
        focus = focus_target;
    }
    else if (phandle->row_btns && phandle->row_count > 0)
    {
        i = phandle->focus_row;
        if (i < 0 || i >= phandle->row_count)
        {
            i = 0;
        }
        focus = phandle->row_btns[i];
    }
    if (focus)
    {
        lv_group_focus_obj(focus);
        divoom_astrootoo_ui_focus_scroll(focus);
    }
}

static void divoom_astrootoo_games_set_list_input(Pdivoom_astrootoo_games_ctrl phandle)
{
    if (phandle == NULL)
    {
        return;
    }
    if (phandle->key_btn)
    {
        lv_obj_add_flag(phandle->key_btn, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(phandle->key_btn, LV_OBJ_FLAG_CLICKABLE);
        lv_group_remove_obj(phandle->key_btn);
        lv_obj_move_background(phandle->key_btn);
    }
}

static void divoom_astrootoo_games_set_run_input(Pdivoom_astrootoo_games_ctrl phandle)
{
    lv_group_t *g;

    if (phandle == NULL || phandle->key_btn == NULL)
    {
        return;
    }
    lv_obj_clear_flag(phandle->key_btn, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(phandle->key_btn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_move_foreground(phandle->key_btn);
    g = lv_group_get_default();
    if (g)
    {
        lv_group_remove_all_objs(g);
        lv_group_add_obj(g, phandle->key_btn);
        lv_group_focus_obj(phandle->key_btn);
    }
}

static void divoom_astrootoo_games_row_activate(Pdivoom_astrootoo_games_ctrl phandle, int row)
{
    if (phandle == NULL)
    {
        return;
    }

    if (row >= 0 && row < phandle->catalog.count)
    {
        if (divoom_astrootoo_games_launch(phandle, row) != 0)
        {
            if (phandle->status_lbl)
            {
                lv_label_set_text(phandle->status_lbl,
                    divoom_menu_control_get_disp_str("Launch failed"));
            }
        }
        return;
    }
}

static void divoom_astrootoo_games_launch_timer_cb(lv_timer_t *timer)
{
    Pdivoom_astrootoo_games_ctrl phandle;
    int row;

    phandle = (Pdivoom_astrootoo_games_ctrl)lv_timer_get_user_data(timer);
    if (phandle == NULL || phandle->launch_timer != timer)
    {
        return;
    }

    phandle->launch_timer = NULL;
    row = phandle->pending_launch_index;
    phandle->pending_launch_index = -1;
    divoom_astrootoo_games_row_activate(phandle, row);
}

static void divoom_astrootoo_games_request_launch(Pdivoom_astrootoo_games_ctrl phandle, int row)
{
    if (phandle == NULL || phandle->page != DIVOOM_ASTROTOO_GAMES_PAGE_LIST
        || row < 0 || row >= phandle->catalog.count)
    {
        return;
    }

    /*
     * 确认键来自行按钮的 LVGL 事件回调；启动 Native Lua 前会删除这些行按钮。
     * 延后一帧再启动，避免在当前事件栈里删除事件目标导致仿真卡在进入游戏前。
     */
    phandle->pending_launch_index = row;
    if (phandle->launch_timer == NULL)
    {
        phandle->launch_timer = lv_timer_create(divoom_astrootoo_games_launch_timer_cb, 1, phandle);
        if (phandle->launch_timer)
        {
            lv_timer_set_repeat_count(phandle->launch_timer, 1);
        }
        else if (phandle->status_lbl)
        {
            lv_label_set_text(phandle->status_lbl,
                divoom_menu_control_get_disp_str("Launch failed"));
        }
    }
}

static void divoom_astrootoo_games_row_click_cb(lv_event_t *e)
{
    Pdivoom_astrootoo_games_ctrl phandle;
    lv_obj_t *target;
    int i;

    if (lv_event_get_code(e) != LV_EVENT_CLICKED)
    {
        return;
    }
    phandle = (Pdivoom_astrootoo_games_ctrl)lv_event_get_user_data(e);
    if (phandle == NULL || phandle->page != DIVOOM_ASTROTOO_GAMES_PAGE_LIST)
    {
        return;
    }
    target = lv_event_get_target(e);
    for (i = 0; i < phandle->row_count; i++)
    {
        if (phandle->row_btns && phandle->row_btns[i] == target)
        {
            divoom_astrootoo_games_request_launch(phandle, i);
            return;
        }
    }
}

static void divoom_astrootoo_games_stop_running(Pdivoom_astrootoo_games_ctrl phandle)
{
    if (phandle == NULL)
    {
        return;
    }
    divoom_game_native_lua_stop();
    if (phandle->run_layer)
    {
        lv_obj_add_flag(phandle->run_layer, LV_OBJ_FLAG_HIDDEN);
    }
    if (phandle->header)
    {
        lv_obj_clear_flag(phandle->header, LV_OBJ_FLAG_HIDDEN);
    }
    if (phandle->content)
    {
        lv_obj_clear_flag(phandle->content, LV_OBJ_FLAG_HIDDEN);
    }
    phandle->page = DIVOOM_ASTROTOO_GAMES_PAGE_LIST;
    if (phandle->row_btns == NULL || phandle->row_count <= 0)
    {
        /*
         * 正常启动只隐藏列表；若历史流程或异常清掉了行对象，回列表时按 catalog 补建。
         */
        divoom_astrootoo_games_build_list(phandle);
    }
    divoom_astrootoo_games_set_list_input(phandle);
    divoom_astrootoo_games_group_bind_list(phandle, NULL);
}

static void divoom_astrootoo_games_quit_cb(void *user_data)
{
    Pdivoom_astrootoo_games_ctrl phandle;

    phandle = (Pdivoom_astrootoo_games_ctrl)user_data;
    divoom_astrootoo_games_stop_running(phandle);
}

static int divoom_astrootoo_games_launch(Pdivoom_astrootoo_games_ctrl phandle, int index)
{
    const divoom_game_entry_t *entry;

    if (phandle == NULL || index < 0 || index >= phandle->catalog.count)
    {
        return -1;
    }

    entry = &phandle->catalog.entries[index];
    phandle->focus_row = index;
    /*
     * 列表行在仿真/真机进入游戏的关键路径上同步 lv_obj_del 会牵动焦点和滚动事件，
     * 这里保留列表对象并隐藏，退出 Games 页面时再统一释放。
     */
    divoom_astrootoo_games_log_mem("launch keep list rows");
    divoom_lvgl9_image_cache_drop_all();

    if (phandle->run_layer == NULL)
    {
        phandle->run_layer = lv_obj_create(phandle->root_obj);
        lv_obj_add_style(phandle->run_layer, &phandle->ui.content_style, 0);
        lv_obj_set_size(phandle->run_layer, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
        lv_obj_set_pos(phandle->run_layer, 0, 0);
        lv_obj_clear_flag(phandle->run_layer, LV_OBJ_FLAG_SCROLLABLE);
    }

    lv_obj_clear_flag(phandle->run_layer, LV_OBJ_FLAG_HIDDEN);
    if (phandle->header)
    {
        lv_obj_add_flag(phandle->header, LV_OBJ_FLAG_HIDDEN);
    }
    if (phandle->content)
    {
        lv_obj_add_flag(phandle->content, LV_OBJ_FLAG_HIDDEN);
    }

    if (divoom_game_native_lua_start(entry, phandle->run_layer,
            divoom_astrootoo_games_quit_cb, phandle) != 0)
    {
        divoom_astrootoo_games_stop_running(phandle);
        return -1;
    }

    phandle->page = DIVOOM_ASTROTOO_GAMES_PAGE_RUNNING;
    divoom_astrootoo_games_set_run_input(phandle);
    return 0;
}

static void divoom_astrootoo_games_list_key_cb(lv_event_t *e)
{
    Pdivoom_astrootoo_games_ctrl phandle;
    uint8_t key_code;
    lv_obj_t *target;
    int pos;

    if (lv_event_get_code(e) != LV_EVENT_KEY)
    {
        return;
    }
    if (!divoom_astrootoo_ui_key_is_menu_press(e, divoom_astrootoo_ui_key_code_from_event(e)))
    {
        return;
    }

    phandle = (Pdivoom_astrootoo_games_ctrl)lv_event_get_user_data(e);
    if (phandle == NULL || phandle->page != DIVOOM_ASTROTOO_GAMES_PAGE_LIST)
    {
        return;
    }

    target = lv_event_get_target(e);
    key_code = divoom_astrootoo_ui_key_code_from_event(e);
    if (divoom_astrootoo_ui_key_is_esc(key_code))
    {
        divoom_menu_control_return_pre();
        return;
    }

    if (divoom_astrootoo_ui_key_is_up(key_code) || divoom_astrootoo_ui_key_is_down(key_code))
    {
        int direct;
        lv_obj_t *next;

        direct = divoom_astrootoo_ui_key_is_up(key_code) ?
            DIVOOM_MENU_DIRECT_UP : DIVOOM_MENU_DIRECT_DOWN;
        if (phandle->direct_handle && target)
        {
            next = divoom_menu_direct_ctrl_get(phandle->direct_handle, target, direct);
            if (next && next != target)
            {
                lv_group_focus_obj(next);
                divoom_astrootoo_ui_focus_scroll(next);
                phandle->focus_row = divoom_menu_direct_ctrl_get_pos(phandle->direct_handle, next);
            }
        }
        return;
    }

    if (divoom_astrootoo_ui_key_is_enter(key_code))
    {
        if (phandle->direct_handle && target)
        {
            pos = divoom_menu_direct_ctrl_get_pos(phandle->direct_handle, target);
            divoom_astrootoo_games_request_launch(phandle, pos);
        }
        return;
    }
}

static void divoom_astrootoo_games_run_key_cb(lv_event_t *e)
{
    Pdivoom_astrootoo_games_ctrl phandle;
    uint8_t key_code;
    uint8_t press;

    if (lv_event_get_code(e) != LV_EVENT_KEY)
    {
        return;
    }

    phandle = (Pdivoom_astrootoo_games_ctrl)lv_event_get_user_data(e);
    if (phandle == NULL || phandle->page != DIVOOM_ASTROTOO_GAMES_PAGE_RUNNING)
    {
        return;
    }

    key_code = divoom_astrootoo_ui_key_code_from_event(e);
    press = divoom_astrootoo_ui_key_press_from_event(e);

    /* press 类型语义（DIVOOM_PRESS_TYPE）：
     *  - DIVOOM_PRESS_UP            ：松开 → key_down=0 + Lua on_key(false)
     *  - DIVOOM_PRESSING / _LONG    ：按住重复（每 100ms）→ 仅刷新 key_down，不重复触发 Lua
     *  - 其余(SHORT/LONG/LONG_LONG) ：按下边沿 → key_down=1 + Lua on_key(true)
     * 注意：旧写法 `else if (press == 0)` 把 0(=SHORT) 误当释放，导致松开/轻点永不清零、
     * 按住(PRESSING) 完全不处理，上下键过一会儿即卡死。 */
    if (press == DIVOOM_PRESS_UP)
    {
        divoom_game_native_lua_on_key(key_code, 0);
        return;
    }
    if (press == DIVOOM_PRESSING || press == DIVOOM_PRESSING_LONG)
    {
        /* 按住重复：仅刷新按住状态。ESC 不在此触发退出，避免「按住一会儿就直接退出整局」。 */
        divoom_game_native_lua_key_hold(key_code);
        return;
    }

    /* 按下边沿：ESC/返回先交给 Lua on_key("b") 逐级返回；只有 Lua 没有消费（既未
     * game.consume_back() 也未 game.quit()）时，C 层才结束整局游戏（兼容 snake 等旧逻辑）。 */
    if (divoom_astrootoo_ui_key_is_esc(key_code))
    {
        if (!divoom_game_native_lua_dispatch_back(key_code))
        {
            divoom_astrootoo_games_stop_running(phandle);
        }
        return;
    }

    divoom_game_native_lua_on_key(key_code, 1);
}

static void divoom_astrootoo_games_destroy_rows(Pdivoom_astrootoo_games_ctrl phandle)
{
    int i;

    if (phandle == NULL)
    {
        return;
    }
    if (phandle->direct_handle)
    {
        divoom_menu_direct_ctrl_destory(phandle->direct_handle);
        phandle->direct_handle = NULL;
    }
    if (phandle->row_btns)
    {
        for (i = 0; i < phandle->row_count; i++)
        {
            if (phandle->row_btns[i])
            {
                lv_obj_del(phandle->row_btns[i]);
            }
        }
        divoom_system_free(phandle->row_btns);
        phandle->row_btns = NULL;
    }
    phandle->row_count = 0;
}

static void divoom_astrootoo_games_build_list(Pdivoom_astrootoo_games_ctrl phandle)
{
    int i;
    int total;
    lv_coord_t row_h;
    lv_coord_t gap;
    lv_coord_t y;

    divoom_astrootoo_games_destroy_rows(phandle);
    divoom_game_runtime_loader_refresh(&phandle->catalog);

    total = phandle->catalog.count;
    if (phandle->catalog.count == 0)
    {
        total = DIVOOM_ASTRO_GAMES_EMPTY_ROW;
    }
    if (total <= 0)
    {
        return;
    }

    phandle->row_count = total;
    phandle->row_btns = (lv_obj_t **)divoom_system_malloc(
        (uint32)(sizeof(lv_obj_t *) * (size_t)total));
    if (phandle->row_btns == NULL)
    {
        return;
    }

    phandle->direct_handle = divoom_menu_direct_ctrl_create(1, total);
    row_h = DIVOOM_ASTRO_REPEAT_ROW_H;
    gap = DIVOOM_ASTRO_REPEAT_ROW_GAP;

    for (i = 0; i < phandle->catalog.count; i++)
    {
        lv_obj_t *btn;
        lv_obj_t *chevron;
        lv_obj_t *title;

        btn = lv_btn_create(phandle->content);
        lv_obj_add_style(btn, &phandle->ui.list_item_style, 0);
        lv_obj_add_style(btn, &phandle->ui.list_item_focus_style, LV_STATE_FOCUSED);
        lv_obj_add_style(btn, &phandle->ui.list_item_focus_style, LV_STATE_FOCUS_KEY);
        lv_obj_add_style(btn, &phandle->ui.list_item_border_focus_style, LV_STATE_FOCUSED);
        lv_obj_add_style(btn, &phandle->ui.list_item_border_focus_style, LV_STATE_FOCUS_KEY);
        lv_obj_set_style_outline_width(btn, 0, LV_STATE_FOCUSED);
        lv_obj_set_style_outline_width(btn, 0, LV_STATE_FOCUS_KEY);
        lv_obj_set_size(btn, DIVOOM_ASTRO_REPEAT_ROW_W, row_h);
        y = gap + i * (row_h + gap);
        lv_obj_set_pos(btn, DIVOOM_ASTRO_LIST_PAD_X, y);

        title = lv_label_create(btn);
        lv_label_set_text(title, phandle->catalog.entries[i].title);
        lv_obj_set_style_text_color(title, lv_color_hex(DIVOOM_ASTRO_INK), 0);
        divoom_astrootoo_games_set_label_font(phandle, title, 24);
        lv_obj_align(title, LV_ALIGN_LEFT_MID, 20, 0);
        divoom_astrootoo_ui_list_label_prepare(title,
            divoom_astrootoo_ui_list_label_width(DIVOOM_ASTRO_REPEAT_ROW_W, 20, 48, 0));
        divoom_astrootoo_ui_list_row_bind_marquee(btn, title);

        /* 内置游戏均可进入下一级，右侧箭头与设置列表保持一致。 */
        chevron = lv_img_create(btn);
        divoom_menu_control_lv_img_set_src(chevron, "23/at_sublevel_chevron.png");
        divoom_astrootoo_ui_apply_sublevel_chevron_size(chevron);
        lv_obj_align(chevron, LV_ALIGN_RIGHT_MID, -20, 0);

        phandle->row_btns[i] = btn;
        lv_obj_add_event_cb(btn, divoom_astrootoo_games_list_key_cb, LV_EVENT_KEY, phandle);
        lv_obj_add_event_cb(btn, divoom_astrootoo_games_row_click_cb, LV_EVENT_CLICKED, phandle);
        lv_group_add_obj(lv_group_get_default(), btn);
        divoom_menu_direct_ctrl_update(phandle->direct_handle, btn, 0, i);
    }

    if (phandle->catalog.count == 0)
    {
        lv_obj_t *btn;
        lv_obj_t *title;

        i = 0;
        btn = lv_btn_create(phandle->content);
        lv_obj_add_style(btn, &phandle->ui.list_item_style, 0);
        lv_obj_set_size(btn, DIVOOM_ASTRO_REPEAT_ROW_W, row_h);
        y = gap + i * (row_h + gap);
        lv_obj_set_pos(btn, DIVOOM_ASTRO_LIST_PAD_X, y);
        lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_clear_flag(btn, LV_OBJ_FLAG_CLICK_FOCUSABLE);

        title = lv_label_create(btn);
        lv_label_set_text(title, divoom_menu_control_get_disp_str("No games installed"));
        lv_obj_set_style_text_color(title, lv_color_hex(DIVOOM_ASTRO_INK_MUTED), 0);
        divoom_astrootoo_games_set_label_font(phandle, title, 22);
        lv_obj_align(title, LV_ALIGN_LEFT_MID, 20, 0);
        divoom_astrootoo_ui_list_label_prepare(title,
            divoom_astrootoo_ui_list_label_width(DIVOOM_ASTRO_REPEAT_ROW_W, 20, 12, 0));
        lv_label_set_long_mode(title, LV_LABEL_LONG_DOT);

        phandle->row_btns[i] = btn;
        divoom_menu_direct_ctrl_update(phandle->direct_handle, btn, 0, i);
    }

    /* content 是固定高度的滚动视口，滚动范围由越界子项自动计算。
     * 不能把视口本身拉长到所有行的总高度，否则焦点虽变化却没有可滚动范围。 */
}

void *divoom_astrootoo_games_init(void *user_data, int menu_id)
{
    Pdivoom_astrootoo_games_ctrl phandle;

    (void)user_data;

    phandle = divoom_system_malloc_out(sizeof(divoom_astrootoo_games_ctrl));
    if (!phandle)
    {
        return NULL;
    }

    phandle->menu_id = menu_id;
    phandle->page = DIVOOM_ASTROTOO_GAMES_PAGE_LIST;
    phandle->pending_launch_index = -1;
    divoom_game_runtime_low_mem_guard_set(1);
    phandle->root_obj = divoom_astrootoo_ui_create_root(&phandle->ui);

    phandle->header = divoom_astrootoo_ui_create_page_header_tab(phandle->root_obj, &phandle->ui,
        "Games", DIVOOM_ASTRO_SUBPAGE_HEADER_IMG, phandle->menu_id);

    phandle->content = divoom_astrootoo_ui_create_scroll_content(phandle->root_obj, &phandle->ui);
    lv_obj_set_scroll_dir(phandle->content, LV_DIR_VER);
    /* 底部固定提示不参与列表滚动，列表视口需要预留空间，避免最后一屏行内容压住提示。 */
    lv_obj_set_height(phandle->content, DIVOOM_ASTRO_CONTENT_H - 30);
    phandle->status_lbl = lv_label_create(phandle->root_obj);
    lv_label_set_text(phandle->status_lbl, divoom_menu_control_get_disp_str("Press joystick to start game"));
    lv_obj_set_style_text_color(phandle->status_lbl, lv_color_hex(DIVOOM_ASTRO_INK_MUTED), 0);
    divoom_astrootoo_games_set_label_font(phandle, phandle->status_lbl, 18);
    lv_obj_set_pos(phandle->status_lbl, DIVOOM_ASTRO_LIST_PAD_X, DIVOOM_SCREEN_HIGH - 24);

    divoom_astrootoo_games_build_list(phandle);

    phandle->key_btn = divoom_astrootoo_ui_create_key_capture_btn(phandle->root_obj, &phandle->ui,
        divoom_astrootoo_games_list_key_cb, phandle);
    lv_obj_add_event_cb(phandle->key_btn, divoom_astrootoo_games_run_key_cb, LV_EVENT_KEY, phandle);
    divoom_astrootoo_games_set_list_input(phandle);

    if (phandle->row_btns && phandle->row_count > 0)
    {
        lv_obj_t *focus_btn;

        if (phandle->catalog.count > 0)
        {
            focus_btn = phandle->row_btns[0];
        }
        else
        {
            focus_btn = phandle->row_btns[phandle->row_count - 1];
        }
        lv_group_focus_obj(focus_btn);
        divoom_astrootoo_ui_focus_scroll(focus_btn);
    }

    return phandle;
}

void divoom_astrootoo_games_disp(void *handle, void *active_obj)
{
    Pdivoom_astrootoo_games_ctrl phandle;

    phandle = (Pdivoom_astrootoo_games_ctrl)handle;
    if (!phandle)
    {
        return;
    }

    lv_scr_load(phandle->root_obj);
    if (phandle->page == DIVOOM_ASTROTOO_GAMES_PAGE_LIST)
    {
        divoom_astrootoo_games_set_list_input(phandle);
        divoom_astrootoo_games_group_bind_list(phandle, (lv_obj_t *)active_obj);
    }
    else if (phandle->page == DIVOOM_ASTROTOO_GAMES_PAGE_RUNNING)
    {
        divoom_astrootoo_games_set_run_input(phandle);
    }
}

void divoom_astrootoo_games_clean(void *handle)
{
    Pdivoom_astrootoo_games_ctrl phandle;

    phandle = (Pdivoom_astrootoo_games_ctrl)handle;
    if (!phandle)
    {
        return;
    }

    divoom_game_native_lua_stop();
    if (phandle->launch_timer)
    {
        lv_timer_del(phandle->launch_timer);
        phandle->launch_timer = NULL;
        phandle->pending_launch_index = -1;
    }
    divoom_astrootoo_games_destroy_rows(phandle);
    divoom_astrootoo_ui_base_reset_styles(&phandle->ui);
    if (phandle->root_obj)
    {
        lv_obj_del(phandle->root_obj);
    }
    divoom_system_free(phandle);
    divoom_game_runtime_low_mem_guard_set(0);
}

void divoom_astrootoo_games_pause(void *handle)
{
    Pdivoom_astrootoo_games_ctrl phandle;

    phandle = (Pdivoom_astrootoo_games_ctrl)handle;
    if (!phandle)
    {
        return;
    }
    if (phandle->page == DIVOOM_ASTROTOO_GAMES_PAGE_RUNNING)
    {
        /*
         * 从游戏运行态离开页面时必须结束游戏实例，而不是仅暂停 timer。
         * Native Lua 大图缓存、fb 和 Lua VM 都是游戏私有内存，退回主菜单后不应继续常驻。
         */
        divoom_astrootoo_games_stop_running(phandle);
    }
}

static Pdivoom_astrootoo_games_ctrl divoom_astrootoo_games_whitebox_get(void)
{
    int mid;
    void *h;

    h = divoom_menu_control_get_clock_mode_menu_handle();
    if (!h)
    {
        return NULL;
    }
    mid = divoom_menu_control_get_active_menu_id();
    if (mid != DIVOOM_MENU_CONTROL_ID_ASTROTOO_GAMES)
    {
        return NULL;
    }
    return (Pdivoom_astrootoo_games_ctrl)h;
}

int divoom_astrootoo_games_whitebox_get_catalog_count(void)
{
    Pdivoom_astrootoo_games_ctrl phandle;

    phandle = divoom_astrootoo_games_whitebox_get();
    if (!phandle)
    {
        return -1;
    }
    return phandle->catalog.count;
}

int divoom_astrootoo_games_whitebox_get_focus_row(void)
{
    Pdivoom_astrootoo_games_ctrl phandle;
    lv_obj_t *focused;

    phandle = divoom_astrootoo_games_whitebox_get();
    if (!phandle || !phandle->direct_handle)
    {
        return -1;
    }
    focused = lv_group_get_focused(lv_group_get_default());
    if (!focused)
    {
        return -1;
    }
    return divoom_menu_direct_ctrl_get_pos(phandle->direct_handle, focused);
}

int divoom_astrootoo_games_whitebox_get_scroll_y(void)
{
    Pdivoom_astrootoo_games_ctrl phandle;

    phandle = divoom_astrootoo_games_whitebox_get();
    if (!phandle || !phandle->content)
    {
        return -1;
    }
    return (int)lv_obj_get_scroll_y(phandle->content);
}

int divoom_astrootoo_games_whitebox_focus_is_visible(void)
{
    Pdivoom_astrootoo_games_ctrl phandle;
    lv_obj_t *focused;
    lv_area_t viewport;
    lv_area_t row;

    phandle = divoom_astrootoo_games_whitebox_get();
    if (!phandle || !phandle->content)
    {
        return 0;
    }
    focused = lv_group_get_focused(lv_group_get_default());
    if (!focused || lv_obj_get_parent(focused) != phandle->content)
    {
        return 0;
    }

    lv_obj_update_layout(phandle->root_obj);
    lv_obj_get_coords(phandle->content, &viewport);
    lv_obj_get_coords(focused, &row);
    return (row.y1 >= viewport.y1 && row.y2 <= viewport.y2) ? 1 : 0;
}

int divoom_astrootoo_games_whitebox_get_running(void)
{
    Pdivoom_astrootoo_games_ctrl phandle;

    phandle = divoom_astrootoo_games_whitebox_get();
    if (!phandle)
    {
        return -1;
    }
    return (phandle->page == DIVOOM_ASTROTOO_GAMES_PAGE_RUNNING) ? 1 : 0;
}

int divoom_astrootoo_games_whitebox_get_page(void)
{
    Pdivoom_astrootoo_games_ctrl phandle;

    phandle = divoom_astrootoo_games_whitebox_get();
    if (!phandle)
    {
        return -1;
    }
    return (int)phandle->page;
}
