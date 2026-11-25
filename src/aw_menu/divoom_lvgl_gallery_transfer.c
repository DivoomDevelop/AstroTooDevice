#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"
#include "divoom_astrootoo_ui_base.h"
#include "divoom_astrootoo_theme.h"
#include "divoom_lvgl_gallery_transfer.h"
#include "divoom_download_aliyun_file.h"
#include "divoom_update_system_file.h"
#include "divoom_light_app.h"

/* gallery.md §2.5：全屏传图进度 overlay */
#define DIVOOM_LVGL_GALLERY_TRANSFER_STATUS_Y     48
#define DIVOOM_LVGL_GALLERY_TRANSFER_HERO_W     240
#define DIVOOM_LVGL_GALLERY_TRANSFER_HERO_H     274
#define DIVOOM_LVGL_GALLERY_TRANSFER_HERO_X     120
#define DIVOOM_LVGL_GALLERY_TRANSFER_HERO_Y     94
#define DIVOOM_LVGL_GALLERY_TRANSFER_BAR_W      360
#define DIVOOM_LVGL_GALLERY_TRANSFER_BAR_H      12
#define DIVOOM_LVGL_GALLERY_TRANSFER_BAR_Y      384
#define DIVOOM_LVGL_GALLERY_TRANSFER_BAR_R      8
#define DIVOOM_LVGL_GALLERY_TRANSFER_COUNT_Y    408
#define DIVOOM_LVGL_GALLERY_TRANSFER_EXIT_TIME  2
#define DIVOOM_LVGL_GALLERY_TRANSFER_POLL_MS    200
#define DIVOOM_LVGL_GALLERY_TRANSFER_IDLE_POLLS 10
#define DIVOOM_LVGL_GALLERY_TRANSFER_CONFIG_POLLS 75
#define DIVOOM_LVGL_GALLERY_TRANSFER_OFFLINE_POLLS 5
#define DIVOOM_LVGL_GALLERY_TRANSFER_STALL_POLLS 600

typedef struct _divoom_lvgl_gallery_transfer_ctrl
{
    divoom_astrootoo_ui_base ui;
    int menu_id;
    int current;
    int total;
    int album_id;
    int pic_id;
    int downloading_flag;
    uint8 total_known;
    uint8 clock_download;
    uint8 system_update;
    divoom_system_file_stage system_stage;
    uint32 clock_session;
    uint32 config_progress;
    uint16 idle_polls;
    uint16 offline_polls;
    uint16 stall_polls;
    uint32_t pre_disp_time;
    lv_obj_t *root_obj;
    lv_obj_t *status_label;
    lv_obj_t *hero_img;
    lv_obj_t *bar_track;
    lv_obj_t *bar_fill;
    lv_obj_t *count_label;
    lv_obj_t *key_btn;
    lv_timer_t *poll_timer;
} divoom_lvgl_gallery_transfer_ctrl, *Pdivoom_lvgl_gallery_transfer_ctrl;

static Pdivoom_lvgl_gallery_transfer_ctrl g_gallery_transfer_ctrl = NULL;

static void divoom_lvgl_gallery_transfer_format_count(Pdivoom_lvgl_gallery_transfer_ctrl phandle, char *buf, int buf_len)
{
    const char *prefix;

    prefix = divoom_menu_control_get_disp_str("galleryTransferProgress");
    if (!prefix || !prefix[0])
    {
        prefix = "Transferred:";
    }
    if (phandle->clock_download && !phandle->total_known)
    {
        snprintf(buf, (size_t)buf_len, "%s X/X", prefix);
    }
    else
    {
        snprintf(buf, (size_t)buf_len, "%s %d/%d", prefix, phandle->current, phandle->total);
    }
}

static void divoom_lvgl_gallery_transfer_refresh_bar(Pdivoom_lvgl_gallery_transfer_ctrl phandle)
{
    char count_buf[64];
    int pct_w;
    int total;

    if (!phandle || !phandle->bar_fill || !phandle->count_label)
    {
        return;
    }

    if (phandle->clock_download && !phandle->total_known)
    {
        lv_obj_set_width(phandle->bar_fill, 0);
        divoom_lvgl_gallery_transfer_format_count(phandle, count_buf, (int)sizeof(count_buf));
        lv_label_set_text(phandle->count_label, count_buf);
        return;
    }

    total = phandle->total > 0 ? phandle->total : 1;
    if (phandle->current < 0)
    {
        phandle->current = 0;
    }
    if (phandle->current > total)
    {
        phandle->current = total;
    }

    pct_w = (DIVOOM_LVGL_GALLERY_TRANSFER_BAR_W * phandle->current) / total;
    if (pct_w < 0)
    {
        pct_w = 0;
    }
    if (pct_w > DIVOOM_LVGL_GALLERY_TRANSFER_BAR_W)
    {
        pct_w = DIVOOM_LVGL_GALLERY_TRANSFER_BAR_W;
    }

    lv_obj_set_width(phandle->bar_fill, (lv_coord_t)pct_w);
    divoom_lvgl_gallery_transfer_format_count(phandle, count_buf, (int)sizeof(count_buf));
    lv_label_set_text(phandle->count_label, count_buf);
}

/* A system update owns this page through local installation and version commit. */
static int divoom_lvgl_gallery_transfer_sync_system_update(Pdivoom_lvgl_gallery_transfer_ctrl phandle)
{
    int current;
    int total;
    int refresh;
    const char *status;
    divoom_system_file_stage stage;

    if (!phandle->system_update)
    {
        if (!divoom_update_system_file_is_resource_updating())
            return 0;
        phandle->system_update = 1;
        phandle->clock_session = 0;
        phandle->config_progress = 0;
    }
    stage = divoom_update_system_file_get_progress(&current, &total);
    refresh = stage != phandle->system_stage || current != phandle->current || total != phandle->total;
    phandle->system_stage = stage;
    phandle->current = current;
    phandle->total = total > 0 ? total : 1;
    phandle->total_known = 1;
    phandle->clock_download = 0;
    phandle->downloading_flag = stage != DIVOOM_SYSTEM_FILE_STAGE_DONE && stage != DIVOOM_SYSTEM_FILE_STAGE_FAILED;
    if (refresh)
    {
        status = stage == DIVOOM_SYSTEM_FILE_STAGE_INSTALL ? "DownloadFirmware" :
            stage == DIVOOM_SYSTEM_FILE_STAGE_COMMIT ? "Save" :
            stage == DIVOOM_SYSTEM_FILE_STAGE_DONE ? "DeviceUpdate" :
            stage == DIVOOM_SYSTEM_FILE_STAGE_RETRY || stage == DIVOOM_SYSTEM_FILE_STAGE_FAILED ? "Failed" :
            "galleryTransferStatus";
        if (phandle->status_label)
            lv_label_set_text(phandle->status_label, divoom_menu_control_get_disp_str(status));
        divoom_lvgl_gallery_transfer_refresh_bar(phandle);
        phandle->stall_polls = 0;
        phandle->pre_disp_time = divoom_system_get_ticks();
        if (phandle->pre_disp_time == 0)
            phandle->pre_disp_time = 1;
    }
    return 1;
}

static void divoom_lvgl_gallery_transfer_sync_from_download(Pdivoom_lvgl_gallery_transfer_ctrl phandle)
{
    int album_id;
    int pic_id;
    int total;
    int cur_id;
    int downloading;
    uint32 clock_current;
    uint32 clock_total;
    uint8 clock_total_known;
    int clock_download;
    int refresh;

    if (!phandle)
    {
        return;
    }

    if (divoom_lvgl_gallery_transfer_sync_system_update(phandle))
        return;

    clock_download = divoom_light_app_get_clock_download_progress(
        &clock_current, &clock_total, &clock_total_known);
    if(phandle->clock_session &&
        !divoom_light_app_clock_download_session_is_current(phandle->clock_session))
        return;
    if (clock_download)
    {
        if(!phandle->clock_session)
            phandle->clock_session = divoom_light_app_get_clock_download_session();
        refresh = !phandle->clock_download || phandle->total_known != clock_total_known ||
            phandle->current != (int)clock_current || phandle->total != (int)clock_total;
        phandle->clock_download = 1;
        phandle->total_known = clock_total_known;
        phandle->current = (int)clock_current;
        phandle->total = (int)clock_total;
        phandle->downloading_flag = !clock_total_known || clock_current < clock_total;
        if (refresh)
        {
            divoom_lvgl_gallery_transfer_refresh_bar(phandle);
            phandle->pre_disp_time = divoom_system_get_ticks();
            if (phandle->pre_disp_time == 0)
            {
                phandle->pre_disp_time = 1;
            }
        }
        return;
    }

    refresh = phandle->clock_download || !phandle->total_known;
    phandle->clock_download = 0;
    phandle->total_known = 1;
    downloading = divoom_download_aliyun_file_get_app_last_info(&album_id, &pic_id, &total, &cur_id);
    if (total <= 0)
    {
        total = 1;
    }
    if (cur_id < 0)
    {
        cur_id = 0;
    }

    phandle->album_id = album_id;
    phandle->pic_id = pic_id;
    phandle->downloading_flag = downloading;
    if (refresh || phandle->total != total || phandle->current != cur_id)
    {
        phandle->total = total;
        phandle->current = cur_id;
        divoom_lvgl_gallery_transfer_refresh_bar(phandle);
        phandle->pre_disp_time = divoom_system_get_ticks();
        if (phandle->pre_disp_time == 0)
        {
            phandle->pre_disp_time = 1;
        }
    }
}

static void divoom_lvgl_gallery_transfer_request_exit(
    Pdivoom_lvgl_gallery_transfer_ctrl phandle, const char *reason)
{
    divoom_print("gallery transfer exit: %s, progress=%d/%d\n",
        reason, phandle->current, phandle->total);
    lv_timer_pause(phandle->poll_timer);
    /* 表盘等待须清理业务状态，避免重新检查缺失资源又进等待页；相册沿用原退出路径。 */
    if(phandle->clock_session)
        divoom_light_app_post_clock_download_event(DIVOOM_LIGHT_APP_EVENT_EXIT_CLOCK_DOWNLOAD,
            phandle->clock_session);
    else
        divoom_main_push_model_event_id(DIVOOM_MODEL_LIGHT_APP,
            DIVOOM_LIGHT_APP_EVENT_DISP_CLOCK_DIAL);
}

static void divoom_lvgl_gallery_transfer_poll_cb(lv_timer_t *timer)
{
    Pdivoom_lvgl_gallery_transfer_ctrl phandle;
    int tick;
    int previous_current;
    int previous_total;
    uint8 previous_total_known;
    uint32 previous_config_progress;
    uint32 idle_limit;

    phandle = (Pdivoom_lvgl_gallery_transfer_ctrl)lv_timer_get_user_data(timer);
    if (!phandle)
    {
        return;
    }

    if(phandle->clock_session &&
        !divoom_light_app_clock_download_session_is_current(phandle->clock_session) &&
        !divoom_update_system_file_is_resource_updating())
    {
        lv_timer_pause(phandle->poll_timer);
        return;
    }
    previous_current = phandle->current;
    previous_total = phandle->total;
    previous_total_known = phandle->total_known;
    previous_config_progress = phandle->config_progress;
    divoom_lvgl_gallery_transfer_sync_from_download(phandle);
    phandle->config_progress = divoom_light_app_get_clock_config_progress(phandle->clock_session);

    /* The OTA-owned schedule lasts through retries and commit; its worker releases it.
     * A photo/clock timer must neither release nor recreate that event. */
    if (phandle->system_update)
        return;
    if (divoom_wifi_model_get_wifi_status() == 0)
    {
        phandle->offline_polls++;
    }
    else
    {
        phandle->offline_polls = 0;
    }
    if (phandle->offline_polls >= DIVOOM_LVGL_GALLERY_TRANSFER_OFFLINE_POLLS)
    {
        divoom_lvgl_gallery_transfer_request_exit(phandle, "offline");
        return;
    }

    if (previous_current != phandle->current || previous_total != phandle->total ||
        previous_total_known != phandle->total_known || previous_config_progress != phandle->config_progress)
    {
        phandle->stall_polls = 0;
    }
    else if (++phandle->stall_polls >= DIVOOM_LVGL_GALLERY_TRANSFER_STALL_POLLS)
    {
        divoom_lvgl_gallery_transfer_request_exit(phandle, "no progress");
        return;
    }

    /* 相册和局域网上传另有任务，只对显式的表盘资源等待检查图片下载队列。 */
    if (phandle->clock_download && !phandle->config_progress &&
        divoom_manage_image_get_pending_num() == 0)
    {
        idle_limit = phandle->clock_download && !phandle->total_known ?
            DIVOOM_LVGL_GALLERY_TRANSFER_CONFIG_POLLS : DIVOOM_LVGL_GALLERY_TRANSFER_IDLE_POLLS;
        if (++phandle->idle_polls >= idle_limit)
        {
            divoom_lvgl_gallery_transfer_request_exit(phandle, "download queue empty");
            return;
        }
    }
    else
    {
        phandle->idle_polls = 0;
    }

    if (phandle->pre_disp_time && phandle->downloading_flag == 0)
    {
        tick = (phandle->current >= phandle->total) ? 1 : 10;
        if (phandle->pre_disp_time + DIVOOM_LVGL_GALLERY_TRANSFER_EXIT_TIME * tick < divoom_system_get_ticks())
        {
            divoom_lvgl_gallery_transfer_request_exit(phandle, "finished");
        }
    }
}

static void divoom_lvgl_gallery_transfer_key_cb(lv_event_t *e)
{
    Pdivoom_lvgl_gallery_transfer_ctrl phandle;
    uint8_t key_code;

    if (lv_event_get_code(e) != LV_EVENT_KEY)
    {
        return;
    }
    if (!divoom_astrootoo_ui_key_is_menu_press(e, divoom_astrootoo_ui_key_code_from_event(e)))
    {
        return;
    }

    phandle = (Pdivoom_lvgl_gallery_transfer_ctrl)lv_event_get_user_data(e);
    if (!phandle)
    {
        return;
    }

    /* The resource worker owns cancellation and page release; keep polling on back. */
    if (phandle->system_update || divoom_update_system_file_is_resource_updating())
        return;
    key_code = divoom_astrootoo_ui_key_code_from_event(e);
    if (divoom_astrootoo_ui_key_is_esc(key_code))
    {
        divoom_lvgl_gallery_transfer_request_exit(phandle, "back key");
    }
}

void divoom_lvgl_gallery_transfer_set_progress(int current, int total)
{
    Pdivoom_lvgl_gallery_transfer_ctrl phandle = g_gallery_transfer_ctrl;

    if (!phandle)
    {
        return;
    }

    phandle->current = current;
    if (total > 0)
    {
        phandle->total = total;
    }
    divoom_lvgl_gallery_transfer_refresh_bar(phandle);
    phandle->pre_disp_time = divoom_system_get_ticks();
    if (phandle->pre_disp_time == 0)
    {
        phandle->pre_disp_time = 1;
    }
}

void divoom_lvgl_gallery_transfer_bind_session(void *handle, unsigned int session)
{
    Pdivoom_lvgl_gallery_transfer_ctrl phandle = handle;
    if(!phandle)
        return;
    if (divoom_update_system_file_is_resource_updating())
        session = 0;
    else if (!divoom_light_app_clock_download_session_is_current(session))
        return;
    phandle->clock_session = session;
    phandle->system_update = 0;
    phandle->system_stage = DIVOOM_SYSTEM_FILE_STAGE_IDLE;
    phandle->config_progress = 0;
    phandle->clock_download = 1;
    phandle->idle_polls = 0;
    phandle->offline_polls = 0;
    phandle->stall_polls = 0;
    phandle->pre_disp_time = divoom_system_get_ticks();
    divoom_lvgl_gallery_transfer_sync_from_download(phandle);
    if (!phandle->system_update && phandle->clock_session && phandle->status_label)
        lv_label_set_text(phandle->status_label,
            divoom_menu_control_get_disp_str("galleryTransferStatus"));
    if (phandle->root_obj)
    {
        /* Same-menu reuse may follow a clock transition that replaced the active root. */
        lv_scr_load(phandle->root_obj);
        lv_obj_invalidate(phandle->root_obj);
    }
    if(phandle->poll_timer)
        lv_timer_resume(phandle->poll_timer);
}

void *divoom_lvgl_gallery_transfer_init(void *user_data, int menu_id)
{
    Pdivoom_lvgl_gallery_transfer_ctrl phandle;
    lv_obj_t *label;

    phandle = divoom_system_malloc_out(sizeof(divoom_lvgl_gallery_transfer_ctrl));
    if (!phandle)
    {
        return NULL;
    }

    phandle->menu_id = menu_id;
    phandle->current = 0;
    phandle->total = 1;
    phandle->total_known = 1;
    phandle->clock_download = 0;
    phandle->system_update = 0;
    phandle->system_stage = DIVOOM_SYSTEM_FILE_STAGE_IDLE;
    phandle->clock_session = (uint32)(uintptr_t)user_data;
    phandle->downloading_flag = divoom_download_aliyun_file_get_app_last_info(
        &phandle->album_id, &phandle->pic_id, &phandle->total, &phandle->current);
    if (phandle->total <= 0)
    {
        phandle->total = 1;
    }
    g_gallery_transfer_ctrl = phandle;

    phandle->root_obj = divoom_astrootoo_ui_create_root(&phandle->ui);
    lv_obj_set_style_bg_color(phandle->root_obj, lv_color_hex(DIVOOM_ASTRO_BG), 0);

    label = lv_label_create(phandle->root_obj);
    phandle->status_label = label;
    lv_label_set_text(label, divoom_menu_control_get_disp_str("galleryTransferStatus"));
    lv_obj_set_style_text_color(label, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    lv_obj_set_style_text_font(label, divoom_menu_control_get_font_info(28, menu_id), 0);
    lv_obj_set_width(label, DIVOOM_SCREEN_WIDTH - 32);
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, DIVOOM_LVGL_GALLERY_TRANSFER_STATUS_Y);

    phandle->hero_img = lv_img_create(phandle->root_obj);
    lv_obj_set_size(phandle->hero_img, DIVOOM_LVGL_GALLERY_TRANSFER_HERO_W, DIVOOM_LVGL_GALLERY_TRANSFER_HERO_H);
    lv_obj_set_pos(phandle->hero_img, DIVOOM_LVGL_GALLERY_TRANSFER_HERO_X, DIVOOM_LVGL_GALLERY_TRANSFER_HERO_Y);
    divoom_menu_control_lv_img_set_src(phandle->hero_img, "15/gallery_transfer_hero.png");

    phandle->bar_track = lv_obj_create(phandle->root_obj);
    lv_obj_add_style(phandle->bar_track, &phandle->ui.content_style, 0);
    lv_obj_set_size(phandle->bar_track, DIVOOM_LVGL_GALLERY_TRANSFER_BAR_W, DIVOOM_LVGL_GALLERY_TRANSFER_BAR_H);
    lv_obj_set_pos(phandle->bar_track, (DIVOOM_SCREEN_WIDTH - DIVOOM_LVGL_GALLERY_TRANSFER_BAR_W) / 2,
        DIVOOM_LVGL_GALLERY_TRANSFER_BAR_Y);
    lv_obj_set_style_bg_color(phandle->bar_track, lv_color_hex(DIVOOM_ASTRO_STROKE), 0);
    lv_obj_set_style_radius(phandle->bar_track, DIVOOM_LVGL_GALLERY_TRANSFER_BAR_R, 0);
    lv_obj_clear_flag(phandle->bar_track, LV_OBJ_FLAG_SCROLLABLE);

    phandle->bar_fill = lv_obj_create(phandle->bar_track);
    lv_obj_add_style(phandle->bar_fill, &phandle->ui.content_style, 0);
    lv_obj_set_size(phandle->bar_fill, 0, DIVOOM_LVGL_GALLERY_TRANSFER_BAR_H);
    lv_obj_set_pos(phandle->bar_fill, 0, 0);
    lv_obj_set_style_bg_color(phandle->bar_fill, lv_color_hex(DIVOOM_ASTRO_SURFACE_FOCUS), 0);
    lv_obj_set_style_radius(phandle->bar_fill, DIVOOM_LVGL_GALLERY_TRANSFER_BAR_R, 0);
    lv_obj_clear_flag(phandle->bar_fill, LV_OBJ_FLAG_SCROLLABLE);

    label = lv_label_create(phandle->root_obj);
    phandle->count_label = label;
    lv_obj_set_style_text_color(label, lv_color_hex(DIVOOM_ASTRO_INK_MUTED), 0);
    lv_obj_set_style_text_font(label, divoom_menu_control_get_font_info(16, menu_id), 0);
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, DIVOOM_LVGL_GALLERY_TRANSFER_COUNT_Y);

    divoom_lvgl_gallery_transfer_refresh_bar(phandle);

    phandle->key_btn = divoom_astrootoo_ui_create_key_capture_btn(phandle->root_obj, &phandle->ui,
        divoom_lvgl_gallery_transfer_key_cb, phandle);
    lv_obj_move_foreground(phandle->key_btn);
    lv_group_add_obj(lv_group_get_default(), phandle->key_btn);

    phandle->poll_timer = lv_timer_create(divoom_lvgl_gallery_transfer_poll_cb,
        DIVOOM_LVGL_GALLERY_TRANSFER_POLL_MS, phandle);
    phandle->pre_disp_time = divoom_system_get_ticks();
    if (phandle->pre_disp_time == 0)
    {
        phandle->pre_disp_time = 1;
    }

    return phandle;
}

void divoom_lvgl_gallery_transfer_disp(void *handle, void *active_obj)
{
    Pdivoom_lvgl_gallery_transfer_ctrl phandle;

    (void)active_obj;

    phandle = (Pdivoom_lvgl_gallery_transfer_ctrl)handle;
    if (!phandle)
    {
        return;
    }

    g_gallery_transfer_ctrl = phandle;
    divoom_lvgl_gallery_transfer_sync_from_download(phandle);
    phandle->idle_polls = 0;
    phandle->offline_polls = 0;
    phandle->stall_polls = 0;
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

void divoom_lvgl_gallery_transfer_pause(void *handle)
{
    Pdivoom_lvgl_gallery_transfer_ctrl phandle;

    phandle = (Pdivoom_lvgl_gallery_transfer_ctrl)handle;
    if (phandle && phandle->poll_timer)
    {
        lv_timer_pause(phandle->poll_timer);
    }
}

void divoom_lvgl_gallery_transfer_exit(void *handle)
{
    Pdivoom_lvgl_gallery_transfer_ctrl phandle;

    phandle = (Pdivoom_lvgl_gallery_transfer_ctrl)handle;
    if (!phandle)
    {
        return;
    }

    if (g_gallery_transfer_ctrl == phandle)
    {
        g_gallery_transfer_ctrl = NULL;
    }

    if (phandle->poll_timer)
    {
        lv_timer_del(phandle->poll_timer);
        phandle->poll_timer = NULL;
    }

    if (phandle->root_obj)
    {
        lv_obj_del(phandle->root_obj);
    }

    divoom_astrootoo_ui_base_reset_styles(&phandle->ui);
    divoom_system_free(phandle);
}
