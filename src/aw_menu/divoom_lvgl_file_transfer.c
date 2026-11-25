#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"
#include "divoom_astrootoo_ui_base.h"
#include "divoom_lvgl_file_transfer.h"
#include "divoom_light_app.h"

#define DIVOOM_FILE_TRANSFER_POLL_MS          200
#define DIVOOM_FILE_TRANSFER_DONE_POLLS         5
#define DIVOOM_FILE_TRANSFER_START_WAIT_POLLS  10
#define DIVOOM_FILE_TRANSFER_OFFLINE_POLLS      5
#define DIVOOM_FILE_TRANSFER_STALL_POLLS      600
#define DIVOOM_FILE_TRANSFER_BAR_INNER_W       366

enum
{
    DIVOOM_FILE_TRANSFER_SOURCE_NONE = 0,
    DIVOOM_FILE_TRANSFER_SOURCE_HTTP,
    DIVOOM_FILE_TRANSFER_SOURCE_DOWNLOAD,
};

typedef struct _divoom_lvgl_file_transfer_ctrl
{
    divoom_astrootoo_ui_base ui;
    int menu_id;
    int percent;
    int transfer_seen;
    int transfer_source;
    int idle_polls;
    int offline_polls;
    int stall_polls;
    int last_progress_percent;
    uint32 last_finished;
    uint32 last_total;
    lv_obj_t *root_obj;
    lv_obj_t *status_label;
    lv_obj_t *bar_fill;
    lv_obj_t *key_btn;
    lv_timer_t *poll_timer;
} divoom_lvgl_file_transfer_ctrl, *Pdivoom_lvgl_file_transfer_ctrl;

static lv_obj_t *divoom_lvgl_file_transfer_rect(lv_obj_t *parent,
    int x, int y, int width, int height, uint32 color)
{
    lv_obj_t *obj;

    obj = lv_obj_create(parent);
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_size(obj, width, height);
    lv_obj_set_style_bg_color(obj, lv_color_hex(color), 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_radius(obj, 0, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    return obj;
}

static void divoom_lvgl_file_transfer_add_stripes(lv_obj_t *parent, int y)
{
    static const int x_pos[] = {
        51, 72, 96, 120, 144, 168, 192, 216,
        240, 264, 288, 312, 336, 360, 384, 408
    };
    int i;

    for (i = 0; i < (int)(sizeof(x_pos) / sizeof(x_pos[0])); i++)
    {
        divoom_lvgl_file_transfer_rect(parent, x_pos[i] + 6, y, 15, 3, 0x804B08);
        divoom_lvgl_file_transfer_rect(parent, x_pos[i] + 3, y + 3, 15, 3, 0x804B08);
        divoom_lvgl_file_transfer_rect(parent, x_pos[i], y + 6, 15, 3, 0x804B08);
    }
}

static void divoom_lvgl_file_transfer_add_window_controls(lv_obj_t *parent)
{
    divoom_lvgl_file_transfer_rect(parent, 370, 191, 12, 4, 0xFFAA00);

    divoom_lvgl_file_transfer_rect(parent, 393, 187, 12, 12, 0xFFAA00);
    divoom_lvgl_file_transfer_rect(parent, 397, 191, 4, 4, 0x613604);

    divoom_lvgl_file_transfer_rect(parent, 416, 187, 4, 4, 0xFFAA00);
    divoom_lvgl_file_transfer_rect(parent, 424, 187, 4, 4, 0xFFAA00);
    divoom_lvgl_file_transfer_rect(parent, 420, 191, 4, 4, 0xFFAA00);
    divoom_lvgl_file_transfer_rect(parent, 416, 195, 4, 4, 0xFFAA00);
    divoom_lvgl_file_transfer_rect(parent, 424, 195, 4, 4, 0xFFAA00);
}

static const char *divoom_lvgl_file_transfer_get_status(void)
{
    const char *status;

    status = divoom_menu_control_get_disp_str("galleryTransferStatus");
    if (!status || !status[0])
    {
        status = "Transferring...";
    }
    return status;
}

static void divoom_lvgl_file_transfer_refresh_percent(
    Pdivoom_lvgl_file_transfer_ctrl phandle,
    int percent)
{
    int fill_width;

    if (!phandle || !phandle->status_label || !phandle->bar_fill)
    {
        return;
    }

    if (percent < 0)
    {
        percent = 0;
    }
    if (percent > 100)
    {
        percent = 100;
    }
    phandle->percent = percent;

    lv_label_set_text_fmt(phandle->status_label, "%s %d%%",
        divoom_lvgl_file_transfer_get_status(), percent);

    fill_width = (DIVOOM_FILE_TRANSFER_BAR_INNER_W * percent) / 100;
    lv_obj_set_width(phandle->bar_fill, fill_width);
}

static void divoom_lvgl_file_transfer_refresh_files(
    Pdivoom_lvgl_file_transfer_ctrl phandle, uint32 finished, uint32 total)
{
    int percent = 0;

    if (finished > total)
    {
        finished = total;
    }
    if (total > 0)
    {
        percent = (int)((100U * finished) / total);
        /* 总数仍可能被后续回包扩充；确认批次封口前不提前显示 100%。 */
        if (finished >= total)
        {
            percent = 99;
        }
    }
    /* 动态增加总文件数时，真实比例可能暂时下降，界面保持单调递增。 */
    if (phandle && percent < phandle->percent)
    {
        percent = phandle->percent;
    }
    divoom_lvgl_file_transfer_refresh_percent(phandle, percent);
}

static int divoom_lvgl_file_transfer_get_progress(int *percent,
    uint32 *finished, uint32 *total)
{
    if (divoom_http_server_get_upload_wait_progress(percent))
    {
        return DIVOOM_FILE_TRANSFER_SOURCE_HTTP;
    }
    if (divoom_http_server_get_pixel_upload_progress(percent))
    {
        return DIVOOM_FILE_TRANSFER_SOURCE_HTTP;
    }
    if (divoom_manage_image_get_transfer_progress(finished, total))
    {
        return DIVOOM_FILE_TRANSFER_SOURCE_DOWNLOAD;
    }
    if (divoom_subscribe_manage_transfer_request_is_pending())
    {
        if (finished)
        {
            *finished = 0;
        }
        if (total)
        {
            *total = 0;
        }
        return DIVOOM_FILE_TRANSFER_SOURCE_DOWNLOAD;
    }
    return DIVOOM_FILE_TRANSFER_SOURCE_NONE;
}

static void divoom_lvgl_file_transfer_poll_cb(lv_timer_t *timer)
{
    Pdivoom_lvgl_file_transfer_ctrl phandle;
    int percent = 0;
    int source;
    uint32 finished = 0;
    uint32 total = 0;

    phandle = (Pdivoom_lvgl_file_transfer_ctrl)lv_timer_get_user_data(timer);
    if (!phandle)
    {
        return;
    }

    source = divoom_lvgl_file_transfer_get_progress(
        &percent, &finished, &total);
    if (source != DIVOOM_FILE_TRANSFER_SOURCE_NONE)
    {
        if (divoom_wifi_model_get_wifi_status() == 0)
        {
            phandle->offline_polls++;
        }
        else
        {
            phandle->offline_polls = 0;
        }
        if (source != phandle->transfer_source || percent != phandle->last_progress_percent ||
            finished != phandle->last_finished || total != phandle->last_total)
        {
            phandle->stall_polls = 0;
        }
        else
        {
            phandle->stall_polls++;
        }
        phandle->last_progress_percent = percent;
        phandle->last_finished = finished;
        phandle->last_total = total;
        if (phandle->offline_polls >= DIVOOM_FILE_TRANSFER_OFFLINE_POLLS ||
            phandle->stall_polls >= DIVOOM_FILE_TRANSFER_STALL_POLLS)
        {
            divoom_print("file transfer exit: offline=%d, stalled=%d\n",
                phandle->offline_polls, phandle->stall_polls);
            lv_timer_pause(timer);
            divoom_main_push_model_event_id(DIVOOM_MODEL_LIGHT_APP,
                DIVOOM_LIGHT_APP_EVENT_EXIT_CLOCK_DOWNLOAD);
            return;
        }
    }
    if (source == DIVOOM_FILE_TRANSFER_SOURCE_DOWNLOAD)
    {
        phandle->transfer_seen = 1;
        phandle->transfer_source = source;
        divoom_lvgl_file_transfer_refresh_files(phandle, finished, total);
        /* 队列和在途文件才决定是否继续驻留，旧请求或进度计数不能阻止退出。 */
        if (divoom_manage_image_get_pending_num() > 0)
        {
            phandle->idle_polls = 0;
            return;
        }
        phandle->idle_polls++;
        if (phandle->idle_polls >= DIVOOM_FILE_TRANSFER_DONE_POLLS)
        {
            (void)divoom_manage_image_ack_transfer_progress();
            divoom_lvgl_file_transfer_refresh_percent(phandle, 100);
            lv_timer_pause(timer);
            divoom_main_push_model_event_id(DIVOOM_MODEL_LIGHT_APP,
                DIVOOM_LIGHT_APP_EVENT_EXIT_CLOCK_DOWNLOAD);
        }
        return;
    }
    if (source == DIVOOM_FILE_TRANSFER_SOURCE_HTTP)
    {
        phandle->transfer_seen = 1;
        phandle->transfer_source = source;
        phandle->idle_polls = 0;
        divoom_lvgl_file_transfer_refresh_percent(phandle, percent);
        return;
    }

    phandle->idle_polls++;
    if (phandle->transfer_seen)
    {
        if (phandle->transfer_source == DIVOOM_FILE_TRANSFER_SOURCE_HTTP)
        {
            divoom_lvgl_file_transfer_refresh_percent(phandle, 100);
        }
        if (phandle->idle_polls >= DIVOOM_FILE_TRANSFER_DONE_POLLS)
        {
            lv_timer_pause(timer);
            divoom_main_push_model_event_id(DIVOOM_MODEL_LIGHT_APP,
                DIVOOM_LIGHT_APP_EVENT_DISP_CLOCK_DIAL);
        }
    }
    else if (phandle->idle_polls >= DIVOOM_FILE_TRANSFER_START_WAIT_POLLS)
    {
        lv_timer_pause(timer);
        divoom_main_push_model_event_id(DIVOOM_MODEL_LIGHT_APP,
            DIVOOM_LIGHT_APP_EVENT_DISP_CLOCK_DIAL);
    }
}

static void divoom_lvgl_file_transfer_key_cb(lv_event_t *e)
{
    Pdivoom_lvgl_file_transfer_ctrl phandle;
    uint8 key_code;
    uint8 press_type;

    if (lv_event_get_code(e) != LV_EVENT_KEY)
    {
        return;
    }
    phandle = (Pdivoom_lvgl_file_transfer_ctrl)lv_event_get_user_data(e);
    key_code = divoom_astrootoo_ui_key_code_from_event(e);
    press_type = divoom_astrootoo_ui_key_press_from_event(e);
    if (divoom_astrootoo_ui_key_is_esc(key_code) &&
        divoom_astrootoo_ui_key_is_menu_press(e, key_code))
    {
        /* ESC 只退出进度页，不确认或取消传输；下载任务继续在后台运行。 */
        if (phandle != NULL && phandle->poll_timer != NULL)
        {
            lv_timer_pause(phandle->poll_timer);
        }
        divoom_main_push_model_event_id(DIVOOM_MODEL_LIGHT_APP,
            DIVOOM_LIGHT_APP_EVENT_EXIT_CLOCK_DOWNLOAD);
        return;
    }

    /* 传图页只消费 ESC；其它按键继续交给全局按键链处理。 */
    divoom_main_keypad_send_key_info(key_code, press_type);
}

void *divoom_lvgl_file_transfer_init(void *user_data, int menu_id)
{
    Pdivoom_lvgl_file_transfer_ctrl phandle;
    lv_obj_t *label;
    lv_obj_t *track;
    int percent = 0;
    int source;
    uint32 finished = 0;
    uint32 total = 0;

    (void)user_data;

    phandle = divoom_system_malloc_out(sizeof(divoom_lvgl_file_transfer_ctrl));
    if (!phandle)
    {
        return NULL;
    }
    phandle->menu_id = menu_id;
    phandle->percent = 0;

    phandle->root_obj = divoom_astrootoo_ui_create_root(&phandle->ui);
    lv_obj_set_style_bg_color(phandle->root_obj, lv_color_hex(0x07121B), 0);

    divoom_lvgl_file_transfer_rect(phandle->root_obj, 45, 180, 390, 120, 0x5D3911);
    divoom_lvgl_file_transfer_rect(phandle->root_obj, 48, 183, 361, 20, 0x9D640E);
    divoom_lvgl_file_transfer_rect(phandle->root_obj, 48, 205, 384, 91, 0x2D1401);
    divoom_lvgl_file_transfer_rect(phandle->root_obj, 45, 180, 390, 3, 0xFF8400);
    divoom_lvgl_file_transfer_rect(phandle->root_obj, 45, 203, 390, 3, 0xFF8400);
    divoom_lvgl_file_transfer_rect(phandle->root_obj, 45, 297, 390, 3, 0xFF8400);
    divoom_lvgl_file_transfer_rect(phandle->root_obj, 45, 180, 3, 120, 0xFF8400);
    divoom_lvgl_file_transfer_rect(phandle->root_obj, 432, 180, 3, 120, 0xFF8400);
    divoom_lvgl_file_transfer_rect(phandle->root_obj, 366, 183, 3, 20, 0xFF8400);
    divoom_lvgl_file_transfer_rect(phandle->root_obj, 389, 183, 3, 20, 0xFF8400);
    divoom_lvgl_file_transfer_rect(phandle->root_obj, 412, 183, 3, 20, 0xFF8400);

    label = lv_label_create(phandle->root_obj);
    lv_label_set_text(label, divoom_lvgl_file_transfer_get_status());
    lv_obj_set_pos(label, 54, 184);
    lv_obj_set_size(label, 290, 18);
    lv_obj_set_style_text_color(label, lv_color_hex(0xFFAA00), 0);
    lv_obj_set_style_text_font(label, divoom_menu_control_get_font_info(14, menu_id), 0);

    divoom_lvgl_file_transfer_add_window_controls(phandle->root_obj);
    divoom_lvgl_file_transfer_add_stripes(phandle->root_obj, 209);
    divoom_lvgl_file_transfer_add_stripes(phandle->root_obj, 285);

    label = lv_label_create(phandle->root_obj);
    phandle->status_label = label;
    lv_obj_set_pos(label, 54, 218);
    lv_obj_set_size(label, 372, 40);
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0xFFB200), 0);
    lv_obj_set_style_text_font(label, divoom_menu_control_get_font_info(20, menu_id), 0);

    track = divoom_lvgl_file_transfer_rect(phandle->root_obj, 54, 258, 372, 18, 0x613604);
    phandle->bar_fill = divoom_lvgl_file_transfer_rect(track, 3, 3, 0, 12, 0xFFAA00);

    source = divoom_lvgl_file_transfer_get_progress(
        &percent, &finished, &total);
    phandle->transfer_source = source;
    phandle->transfer_seen = source != DIVOOM_FILE_TRANSFER_SOURCE_NONE;
    if (source == DIVOOM_FILE_TRANSFER_SOURCE_DOWNLOAD)
    {
        divoom_lvgl_file_transfer_refresh_files(phandle, finished, total);
    }
    else
    {
        divoom_lvgl_file_transfer_refresh_percent(phandle, percent);
    }

    phandle->key_btn = divoom_astrootoo_ui_create_key_capture_btn(phandle->root_obj,
        &phandle->ui, divoom_lvgl_file_transfer_key_cb, phandle);
    lv_obj_move_foreground(phandle->key_btn);
    lv_group_add_obj(lv_group_get_default(), phandle->key_btn);

    phandle->poll_timer = lv_timer_create(divoom_lvgl_file_transfer_poll_cb,
        DIVOOM_FILE_TRANSFER_POLL_MS, phandle);
    return phandle;
}

void divoom_lvgl_file_transfer_disp(void *handle, void *active_obj)
{
    Pdivoom_lvgl_file_transfer_ctrl phandle;
    int percent = 0;
    int source;
    uint32 finished = 0;
    uint32 total = 0;

    (void)active_obj;
    phandle = (Pdivoom_lvgl_file_transfer_ctrl)handle;
    if (!phandle)
    {
        return;
    }

    phandle->offline_polls = 0;
    phandle->stall_polls = 0;
    source = divoom_lvgl_file_transfer_get_progress(
        &percent, &finished, &total);
    if (source != DIVOOM_FILE_TRANSFER_SOURCE_NONE)
    {
        phandle->transfer_seen = 1;
        phandle->transfer_source = source;
        phandle->idle_polls = 0;
        if (source == DIVOOM_FILE_TRANSFER_SOURCE_DOWNLOAD)
        {
            divoom_lvgl_file_transfer_refresh_files(phandle, finished, total);
        }
        else
        {
            divoom_lvgl_file_transfer_refresh_percent(phandle, percent);
        }
    }
    else
    {
        phandle->transfer_seen = 0;
        phandle->transfer_source = DIVOOM_FILE_TRANSFER_SOURCE_NONE;
        phandle->idle_polls = 0;
        divoom_lvgl_file_transfer_refresh_percent(phandle, 0);
    }
    lv_scr_load(phandle->root_obj);
    if (phandle->key_btn)
    {
        lv_group_focus_obj(phandle->key_btn);
    }
    if (phandle->poll_timer)
    {
        lv_timer_resume(phandle->poll_timer);
    }
}

void divoom_lvgl_file_transfer_pause(void *handle)
{
    Pdivoom_lvgl_file_transfer_ctrl phandle;

    phandle = (Pdivoom_lvgl_file_transfer_ctrl)handle;
    if (phandle && phandle->poll_timer)
    {
        lv_timer_pause(phandle->poll_timer);
    }
}

void divoom_lvgl_file_transfer_exit(void *handle)
{
    Pdivoom_lvgl_file_transfer_ctrl phandle;

    phandle = (Pdivoom_lvgl_file_transfer_ctrl)handle;
    if (!phandle)
    {
        return;
    }
    if (phandle->poll_timer)
    {
        lv_timer_del(phandle->poll_timer);
    }
    if (phandle->root_obj)
    {
        lv_obj_del(phandle->root_obj);
    }
    divoom_astrootoo_ui_base_reset_styles(&phandle->ui);
    divoom_system_free(phandle);
}
