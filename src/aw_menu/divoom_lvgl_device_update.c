#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"
#include "divoom_astrootoo_theme.h"
#if DIVOOM_BUILD_RTOS_DEVICE
#include "divoom_ota_rtos.h"
#include "divoom_ota_server.h"
#include "divoom_port_fs.h"
#endif


#define  DIVOOM_LVGL_DEVICE_UPDATE_TRY_MAX_TIME   10

/* AstroToo 480×480 升级页布局（原坐标按约 800×1280 屏写死，会溢出） */
#define UPD_UI_PAD_X        32
#define UPD_UI_TITLE_Y      88
#define UPD_UI_STATUS_Y     168
#define UPD_UI_PERCENT_Y    268
#define UPD_UI_SLIDER_Y     328
#define UPD_UI_SLIDER_H     10
#define UPD_UI_CONTENT_W    (DIVOOM_SCREEN_WIDTH - UPD_UI_PAD_X * 2)

typedef struct _divoom_lvgl_device_update_ctrl
{
    int menu_id;
    int exit_flag;
    int fail_time;
    int send_flag;
    int run_tick;
    int disp_len;
    int read_len;
    int tol_len;
    int  new_version;
    char firmware_url[128];
    char sha128_code[128];
    lv_style_t  normal_style;
    void *time_handle;
    void *main_obj;
    void *back_obj;
    void *slide_obj;
    void *precent_obj;
    void *status_obj;
    uint8 ota_ui_phase;
    /* UI and download each hold one reference; the last owner frees the ctrl. */
    void *download_lock;
    int download_refs;
    int download_cancelled;
    int download_started;
}divoom_lvgl_device_update_ctrl, *Pdivoom_lvgl_device_update_ctrl;

static Pdivoom_lvgl_device_update_ctrl s_jl_ota_ui_handle = NULL;
static uint8 s_jl_ota_pending_percent = 0;
static volatile uint8 s_rtos_ota_ui_active;
static volatile uint8 s_rtos_ota_ui_phase; /* 1=unzip 2=flash */
static volatile uint8 s_rtos_ota_ui_percent;
static volatile uint8 s_tf_ota_percent; /* 插卡：0x77 杰理写 bank / UART 收 tf_update.zip */
static volatile uint8 s_tf_ota_feed;    /* 1=页54用上面百分比；解压/HTTP 接手后清 0 */

/* The download uses a fixed staging filename. A cancelled request retains this
 * local slot until its HTTP call has drained, before another download can use it. */
static Pdivoom_lvgl_device_update_ctrl s_device_update_download;

static void divoom_lvgl_device_update_download_release(Pdivoom_lvgl_device_update_ctrl phandle)
{
    int last = 1;

    if (phandle->download_lock) {
        divoom_system_get_lock_direct(phandle->download_lock);
        last = --phandle->download_refs == 0;
        divoom_system_free_lock_direct(phandle->download_lock);
    }
    if (last) {
        if (phandle->download_lock)
            divoom_system_destroy_lock(phandle->download_lock);
        divoom_system_free(phandle);
    }
}

static int divoom_lvgl_device_update_download_cancel(Pdivoom_lvgl_device_update_ctrl phandle)
{
    int cancelled;

    if (!phandle->download_lock)
        return 1;
    divoom_system_get_lock_direct(phandle->download_lock);
    cancelled = !phandle->download_started;
    if (cancelled)
        phandle->download_cancelled = 1;
    divoom_system_free_lock_direct(phandle->download_lock);
    return cancelled;
}

static void divoom_lvgl_device_update_menu_event_cb(lv_event_t * e)
{
    lv_obj_t *cur_obj;
    lv_group_t * cur_group_obj = lv_obj_get_group(lv_event_get_target_obj(e));
    Pdivoom_lvgl_device_update_ctrl phandle = (Pdivoom_lvgl_device_update_ctrl)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);
    cur_obj = lv_event_get_target_obj(e);
    if(code == LV_EVENT_KEY)
    {
        (void)cur_obj;
        (void)cur_group_obj;
        (void)phandle;
    }
}

static void divoom_lvgl_ota_progress_ui_build(Pdivoom_lvgl_device_update_ctrl phandle,
					     const char *status_key)
{
    void *label_obj, *slide_obj;

    phandle->main_obj = lv_obj_create(NULL);
    divoom_menu_control_init_lable_style(&phandle->normal_style);
    lv_obj_set_style_bg_color(phandle->main_obj, lv_color_hex(DIVOOM_ASTRO_BG), LV_PART_MAIN);
    lv_obj_set_pos(phandle->main_obj, 0, 0);
    lv_obj_set_size(phandle->main_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
    lv_obj_add_style(phandle->main_obj, &phandle->normal_style, 0);
    lv_obj_set_scrollbar_mode(phandle->main_obj, LV_SCROLLBAR_MODE_OFF);
    lv_obj_clear_flag(phandle->main_obj, LV_OBJ_FLAG_SCROLLABLE);

    label_obj = lv_label_create(phandle->main_obj);
    lv_obj_set_pos(label_obj, UPD_UI_PAD_X, UPD_UI_TITLE_Y);
    lv_obj_set_size(label_obj, UPD_UI_CONTENT_W, 48);
    lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("DeviceUpdate"));
    lv_obj_set_style_text_color(label_obj, lv_color_hex(DIVOOM_ASTRO_INK), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(32, phandle->menu_id), 0);

    if (status_key && status_key[0]) {
        label_obj = lv_label_create(phandle->main_obj);
        lv_obj_set_pos(label_obj, UPD_UI_PAD_X, UPD_UI_STATUS_Y);
        lv_obj_set_size(label_obj, UPD_UI_CONTENT_W, 40);
        lv_label_set_long_mode(label_obj, LV_LABEL_LONG_WRAP);
        lv_label_set_text(label_obj, divoom_menu_control_get_disp_str(status_key));
        lv_obj_set_style_text_color(label_obj, lv_color_hex(DIVOOM_ASTRO_INK_MUTED), 0);
        lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(24, phandle->menu_id), 0);
        phandle->status_obj = label_obj;
    }

    label_obj = lv_label_create(phandle->main_obj);
    lv_obj_set_pos(label_obj, UPD_UI_PAD_X, UPD_UI_PERCENT_Y);
    lv_obj_set_size(label_obj, UPD_UI_CONTENT_W, 36);
    lv_label_set_text(label_obj, "0%");
    lv_obj_set_style_text_color(label_obj, lv_color_hex(DIVOOM_ASTRO_INK_MUTED), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(28, phandle->menu_id), 0);
    phandle->precent_obj = label_obj;

    slide_obj = lv_slider_create(phandle->main_obj);
    lv_obj_set_style_opa(slide_obj, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_slider_set_range(slide_obj, 0, 100);
    lv_obj_set_pos(slide_obj, UPD_UI_PAD_X + 16, UPD_UI_SLIDER_Y);
    lv_obj_set_size(slide_obj, UPD_UI_CONTENT_W - 32, UPD_UI_SLIDER_H);
    lv_slider_set_value(slide_obj, 0, LV_ANIM_ON);
    lv_obj_set_style_radius(slide_obj, DIVOOM_ASTRO_RADIUS_SM, LV_PART_MAIN);
    lv_obj_set_style_radius(slide_obj, DIVOOM_ASTRO_RADIUS_SM, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(slide_obj, lv_color_hex(DIVOOM_ASTRO_SURFACE_FOCUS), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(slide_obj, lv_color_hex(DIVOOM_ASTRO_STROKE), LV_PART_MAIN);
    lv_obj_add_flag(slide_obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(slide_obj, divoom_lvgl_device_update_menu_event_cb, LV_EVENT_KEY, phandle);
    phandle->slide_obj = slide_obj;
}

static Pdivoom_lvgl_device_update_ctrl divoom_lvgl_ota_progress_ui_create(int menu_id)
{
    Pdivoom_lvgl_device_update_ctrl phandle;

    phandle = divoom_system_malloc(sizeof(divoom_lvgl_device_update_ctrl));
    if (!phandle)
        return NULL;
    memset(phandle, 0, sizeof(divoom_lvgl_device_update_ctrl));
    phandle->menu_id = menu_id;
    divoom_lvgl_ota_progress_ui_build(phandle, NULL);
    return phandle;
}

static void divoom_lvgl_ota_progress_ui_apply_percent(Pdivoom_lvgl_device_update_ctrl phandle, uint8 percent)
{
    if (!phandle || !phandle->slide_obj || !phandle->precent_obj)
        return;
    if (percent > 100)
        percent = 100;
    lv_slider_set_value(phandle->slide_obj, percent, LV_ANIM_ON);
    lv_label_set_text_fmt(phandle->precent_obj, "%d%%", percent);
}

/* ota_jl 线程只写 pending；LVGL 定时器刷新，避免跨线程 invalidate */
void divoom_lvgl_jl_ota_update_set_percent(uint8 percent)
{
    if (percent > 100)
        percent = 100;
    s_jl_ota_pending_percent = percent;
}

void divoom_lvgl_device_update_tf_reset(void)
{
    s_tf_ota_percent = 0;
    s_tf_ota_feed = 1;
}

void divoom_lvgl_device_update_tf_set_percent(uint8 percent)
{
    if (!s_tf_ota_feed || s_rtos_ota_ui_active)
        return;
    if (percent > 100)
        percent = 100;
    s_tf_ota_percent = percent;
}

void divoom_lvgl_device_update_tf_end(void)
{
    s_tf_ota_feed = 0;
}

void divoom_lvgl_device_update_begin_unzip(void)
{
    divoom_lvgl_device_update_tf_end();
    s_rtos_ota_ui_percent = 33;
    s_rtos_ota_ui_phase = 1;
    s_rtos_ota_ui_active = 1;
}

void divoom_lvgl_device_update_begin_flash(void)
{
    divoom_lvgl_device_update_tf_end();
    s_rtos_ota_ui_percent = 66;
    s_rtos_ota_ui_phase = 2;
    s_rtos_ota_ui_active = 1;
}

void divoom_lvgl_device_update_set_ota_progress(uint8 percent)
{
    if (percent > 100)
        percent = 100;
    /* Total progress: download 0-33, unzip 33-66, flash 66-100. */
    if (s_rtos_ota_ui_phase == 2)
        s_rtos_ota_ui_percent = 66 + percent * 34 / 100;
    else if (s_rtos_ota_ui_phase == 1)
        s_rtos_ota_ui_percent = 33 + percent * 33 / 100;
    else
        s_rtos_ota_ui_percent = percent * 33 / 100;
}

void divoom_lvgl_device_update_end_ota_progress(void)
{
    s_rtos_ota_ui_active = 0;
    s_rtos_ota_ui_phase = 0;
    s_rtos_ota_ui_percent = 0;
}

static void divoom_lvgl_jl_ota_update_time_loop(struct _lv_timer_t *ptime_info)
{
    Pdivoom_lvgl_device_update_ctrl phandle =
        (Pdivoom_lvgl_device_update_ctrl)lv_timer_get_user_data(ptime_info);

    if (!phandle)
        return;
    if (phandle->disp_len == (int)s_jl_ota_pending_percent)
        return;
    phandle->disp_len = (int)s_jl_ota_pending_percent;
    divoom_lvgl_ota_progress_ui_apply_percent(phandle, s_jl_ota_pending_percent);
}

void *divoom_lvgl_jl_ota_update_init(void *user_data, int menu_id)
{
    Pdivoom_lvgl_device_update_ctrl phandle;

    (void)user_data;
    phandle = divoom_lvgl_ota_progress_ui_create(menu_id);
    if (!phandle)
        return NULL;
    s_jl_ota_ui_handle = phandle;
    phandle->disp_len = -1;
    phandle->time_handle = lv_timer_create(divoom_lvgl_jl_ota_update_time_loop, 100, phandle);
    divoom_lvgl_ota_progress_ui_apply_percent(phandle, s_jl_ota_pending_percent);
    return phandle;
}

void divoom_lvgl_jl_ota_update_disp(void *handle, void *active_obj)
{
    Pdivoom_lvgl_device_update_ctrl phandle = (Pdivoom_lvgl_device_update_ctrl)handle;

    (void)active_obj;
    if (!phandle)
        return;
    lv_scr_load(phandle->main_obj);
    divoom_lvgl_ota_progress_ui_apply_percent(phandle, s_jl_ota_pending_percent);
    phandle->disp_len = (int)s_jl_ota_pending_percent;
}

void divoom_lvgl_jl_ota_update_exit(void *handle)
{
    Pdivoom_lvgl_device_update_ctrl phandle = (Pdivoom_lvgl_device_update_ctrl)handle;

    if (!phandle)
        return;
    if (s_jl_ota_ui_handle == phandle)
        s_jl_ota_ui_handle = NULL;
    s_jl_ota_pending_percent = 0;
    if (phandle->time_handle)
        lv_timer_del(phandle->time_handle);
    lv_obj_del(phandle->main_obj);
    lv_style_reset(&phandle->normal_style);
    divoom_system_free(phandle);
}

#define DIVOOM_LVGL_DEVICE_UPDATE_TIME_OUT_TICK (5*60*10)

void divoom_lvgl_device_update_time_loop(struct _lv_timer_t *ptime_info)
{
    int fail_time, read_len, tol_len;
    Pdivoom_lvgl_device_update_ctrl phandle = (Pdivoom_lvgl_device_update_ctrl)lv_timer_get_user_data(ptime_info);

    if (phandle->download_lock)
        divoom_system_get_lock_direct(phandle->download_lock);
    fail_time = phandle->fail_time;
    read_len = phandle->read_len;
    tol_len = phandle->tol_len;
    if (phandle->download_started)
        phandle->send_flag = 1;
    if (phandle->download_lock)
        divoom_system_free_lock_direct(phandle->download_lock);
    phandle->run_tick ++;
    if(phandle->run_tick >= DIVOOM_LVGL_DEVICE_UPDATE_TIME_OUT_TICK && !phandle->send_flag)
    {
        //如果超过三分钟没有成功升级，将退出升级，有可能是网络出现问题
        if(phandle->send_flag == 0)
        {
            phandle->send_flag = 1;
            if (divoom_lvgl_device_update_download_cancel(phandle)) {
                phandle->exit_flag = 1;
                divoom_main_push_model_event_id(DIVOOM_MODEL_OTA, DIVOOM_OTA_SERVER_SRV_UPDATE_FAIL);
                divoom_main_push_model_event_id(DIVOOM_MODEL_LIGHT_APP, DIVOOM_LIGHT_APP_EVENT_DISP_CLOCK_DIAL);
            }
        }
        return;
    }
    if(fail_time >= DIVOOM_LVGL_DEVICE_UPDATE_TRY_MAX_TIME)
    {
        if(phandle->exit_flag == 0)
        {
            phandle->exit_flag = 1;
            phandle->send_flag = 1;
            divoom_main_push_model_event_id(DIVOOM_MODEL_OTA, DIVOOM_OTA_SERVER_SRV_UPDATE_FAIL);
        }
        return;
    }

    if (s_rtos_ota_ui_active) {
        uint8 ota_percent = s_rtos_ota_ui_percent;

        if (phandle->ota_ui_phase != s_rtos_ota_ui_phase) {
            phandle->ota_ui_phase = s_rtos_ota_ui_phase;
            phandle->disp_len = -1;
        }
        if (phandle->disp_len != (int)ota_percent) {
            phandle->disp_len = (int)ota_percent;
            lv_slider_set_value(phandle->slide_obj, ota_percent, LV_ANIM_ON);
            lv_label_set_text_fmt(phandle->precent_obj, "%d %%", ota_percent);
        }
        return;
    }

    if (s_tf_ota_feed && !tol_len) {
        uint8 tf_percent = s_tf_ota_percent;

        if (phandle->disp_len != (int)tf_percent) {
            phandle->disp_len = (int)tf_percent;
            lv_slider_set_value(phandle->slide_obj, tf_percent, LV_ANIM_ON);
            lv_label_set_text_fmt(phandle->precent_obj, "%d %%", tf_percent);
        }
        return;
    }
    
    if((phandle->disp_len != read_len) && tol_len)
    {
        int percent;
        phandle->disp_len = read_len;
#if DIVOOM_BUILD_RTOS_DEVICE
        percent = (int)((unsigned long long)phandle->disp_len * 33 / tol_len);
        if (percent > 33)
            percent = 33;
#else
        percent = phandle->disp_len * 100 / tol_len;
#endif
        lv_slider_set_value(phandle->slide_obj, percent, LV_ANIM_ON);
        lv_label_set_text_fmt(phandle->precent_obj, "%d %%", percent);
    }
}


void divoom_lvgl_device_update_down_file_callback(void *user_data,uint32 read_len, uint32 tol_len)
{
    Pdivoom_lvgl_device_update_ctrl phandle = user_data;

    divoom_system_get_lock_direct(phandle->download_lock);
    if (!phandle->download_cancelled) {
        phandle->tol_len = tol_len;
        phandle->read_len = read_len;
    }
    divoom_system_free_lock_direct(phandle->download_lock);
}
size_t exec_command(const char *command, char *buffer, size_t buffer_size);


//下载升级文件，如果下载十次次失败的话，将退出下载线程和菜单
void* divoom_lvgl_device_update_download_task_main(void *param)
{
    Pdivoom_lvgl_device_update_ctrl phandle = param;
    int attempt;

    for (attempt = 0; attempt < DIVOOM_LVGL_DEVICE_UPDATE_TRY_MAX_TIME; attempt++)
    {
        int cancelled;

        divoom_system_get_lock_direct(phandle->download_lock);
        cancelled = phandle->download_cancelled;
        divoom_system_free_lock_direct(phandle->download_lock);
        if (cancelled)
            break;
#if DIVOOM_BUILD_RTOS_DEVICE
        if (divoom_ota_rtos_download_staging(phandle->firmware_url,
                phandle->sha128_code, 1,
                divoom_lvgl_device_update_down_file_callback, phandle))
        {
            char staging_path[256], fex_path[256];

            divoom_ota_rtos_build_staging_download_path(staging_path, sizeof(staging_path));
            divoom_ota_rtos_build_save_path(phandle->firmware_url, fex_path, sizeof(fex_path));
            /* No HTTP/callback runs under this lock. start_from_staging only
             * hands copied paths to the backend task; it never waits for UI. */
            divoom_system_get_lock_direct(phandle->download_lock);
            if (!phandle->download_cancelled &&
                divoom_ota_rtos_start_from_staging(staging_path, fex_path) == 0)
                phandle->download_started = 1;
            cancelled = phandle->download_cancelled || phandle->download_started;
            divoom_system_free_lock_direct(phandle->download_lock);
            if (cancelled)
                break;
        }
#else
        {
            int ret;
            char sha1sum_info[128];

        if(divoom_http_client_down_file_no_delay("/userdata/divoom_app_new", phandle->firmware_url, 1, NULL, divoom_lvgl_device_update_down_file_callback, phandle))
        {
            sha1sum_info[0] = 0;
            divoom_system_delay_ms(1000);
            exec_command_system("sync");
            divoom_app_com_get_file_sha1sum("/userdata/divoom_app_new",sha1sum_info);
            ret = strcmp(sha1sum_info, phandle->sha128_code);
            
            if(ret== 0)
            {
                divoom_system_get_lock_direct(phandle->download_lock);
                if (phandle->download_cancelled) {
                    divoom_system_free_lock_direct(phandle->download_lock);
                    break;
                }
                divoom_print("divoom_finish update!\n");
                exec_command_system("chmod +x /userdata/divoom_app_new");
                exec_command_system("mv -f /userdata/divoom_app_new /usr/bin/divoom_app");
                divoom_app_com_write_file(DIVOOM_APP_SHA256_SUM_FILE_PATH, sha1sum_info, strlen(sha1sum_info));
                divoom_main_set_update_fireware_flag(1);
                exec_command_system("sync");
                exit(0);
                break;
            }
            else
            {
                divoom_print("it fails compare sha1: %s, %s\n", sha1sum_info, phandle->sha128_code);
            }
        }
        }
#endif
        divoom_system_get_lock_direct(phandle->download_lock);
        cancelled = phandle->download_cancelled;
        if (!cancelled)
            phandle->fail_time = attempt + 1;
        divoom_system_free_lock_direct(phandle->download_lock);
        if (cancelled)
            break;
        divoom_system_delay_ms(2000);
    }
    __sync_bool_compare_and_swap(&s_device_update_download, phandle, NULL);
    divoom_lvgl_device_update_download_release(phandle);
    return NULL;
}


void *divoom_lvgl_device_update_init(void *user_data, int menu_id)
{
    Pdivoom_lvgl_device_update_ctrl phandle;
    Pdivoom_general_json_get_update_file_info_respone pupdate_file = (Pdivoom_general_json_get_update_file_info_respone)user_data;

    divoom_print("ota: device_update_init enter menu=%d user=%p\n", menu_id, user_data);
    if (pupdate_file == NULL)
    {
        divoom_print("ota: device_update_init fail, user_data NULL\n");
        return NULL;
    }

    phandle = divoom_system_malloc(sizeof(divoom_lvgl_device_update_ctrl));
    if (phandle == NULL)
    {
        divoom_print("ota: device_update_init malloc fail\n");
        divoom_system_free(pupdate_file);
        return NULL;
    }

    memset(phandle, 0, sizeof(divoom_lvgl_device_update_ctrl));
    phandle->menu_id = menu_id;
    strcpy(phandle->firmware_url, pupdate_file->file_id);
    strcpy(phandle->sha128_code, pupdate_file->sha1_sum);
    phandle->new_version = pupdate_file->version;
    divoom_print("ota: device_update_init ver=%u url=%s sha1=%s\n",
                 (unsigned)phandle->new_version, phandle->firmware_url, phandle->sha128_code);
    divoom_system_free(pupdate_file);
    if (phandle->firmware_url[0] == '/') {
        /* TF/本地包：只开升级页，不走 HTTP 下载 */
        phandle->send_flag = 1;
        divoom_print("ota: device_update_init local package, skip download\n");
    } else {
        divoom_lvgl_device_update_tf_end();
        phandle->download_lock = divoom_system_create_lock();
        phandle->download_refs = 1;
        if (phandle->download_lock &&
            !__sync_bool_compare_and_swap(&s_device_update_download, NULL, phandle)) {
            divoom_system_destroy_lock(phandle->download_lock);
            phandle->download_lock = NULL;
        }
#if DIVOOM_BUILD_RTOS_DEVICE
        if (divoom_ota_rtos_is_running()) {
            __sync_bool_compare_and_swap(&s_device_update_download, phandle, NULL);
            divoom_lvgl_device_update_download_release(phandle);
            return NULL;
        }
#endif
        if (!phandle->download_lock) {
            /* The accepted page owns failure reporting even if no worker can
             * be started. Returning NULL here would leave its OTA schedule live. */
            phandle->fail_time = DIVOOM_LVGL_DEVICE_UPDATE_TRY_MAX_TIME;
        }
    }

    divoom_lvgl_ota_progress_ui_build(phandle, "DownloadFirmware");
    phandle->time_handle = lv_timer_create(divoom_lvgl_device_update_time_loop, 100, phandle);
    if (!phandle->time_handle) {
        lv_obj_del(phandle->main_obj);
        lv_style_reset(&phandle->normal_style);
        __sync_bool_compare_and_swap(&s_device_update_download, phandle, NULL);
        divoom_lvgl_device_update_download_release(phandle);
#if DIVOOM_BUILD_RTOS_DEVICE
        if (!divoom_ota_rtos_is_running())
#endif
            divoom_main_push_model_event_id(DIVOOM_MODEL_OTA, DIVOOM_OTA_SERVER_SRV_UPDATE_FAIL);
        return NULL;
    }
    if (phandle->download_lock) {
        phandle->download_refs++; /* Reserve ownership before the worker can run. */
        if (!divoom_system_create_task(divoom_lvgl_device_update_download_task_main,
                "lv_dev_up", phandle, DIVOOM_TASK_STACK_LVGL_DEV_UPDATE)) {
            __sync_bool_compare_and_swap(&s_device_update_download, phandle, NULL);
            phandle->fail_time = DIVOOM_LVGL_DEVICE_UPDATE_TRY_MAX_TIME;
            divoom_lvgl_device_update_download_release(phandle);
        }
    }

    return phandle;
}

void divoom_lvgl_device_update_disp(void *handle, void *active_obj)
{
    Pdivoom_lvgl_device_update_ctrl phandle = (Pdivoom_lvgl_device_update_ctrl)handle;

    (void)active_obj;
    lv_scr_load(phandle->main_obj);
}

void divoom_lvgl_device_update_exit(void *handle)
{
    Pdivoom_lvgl_device_update_ctrl phandle = (Pdivoom_lvgl_device_update_ctrl)handle;
    
    if (!phandle)
        return;
    divoom_lvgl_device_update_download_cancel(phandle);
    lv_timer_del(phandle->time_handle);
    lv_obj_del(phandle->main_obj);
    lv_style_reset(&phandle->normal_style);
    divoom_lvgl_device_update_download_release(phandle);
}
