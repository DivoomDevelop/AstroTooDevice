#include "divoom.h"
#include "lvgl.h"
#include "divoom_lvgl_photo_view.h"
#include "divoom_menu_control.h"
#include "divoom_lvgl_select_album_play.h"
#include "divoom_lvgl_play_gif_file.h"
#include "divoom_astrootoo_ui_base.h"
#include "divoom_main.h"
#include "divoom_pixel_format.h"
#ifdef DIVOOM_USE_RK_IC
#include "rk_mpi.h"
#include <turbojpeg.h>
#include "jpeglib.h"
#include <setjmp.h>
#include <rga/im2d.h>
#include <rga/rga.h>
#include <rga/RgaApi.h>
#endif

typedef struct _divoom_lvgl_photo_view_ctrl
{
    int menu_id;
    int cur_disp_pos;
    int play_video_flag;
    int playing_video;
    void *main_obj;
    void *image_obj;
    void *play_gif_handle;
    lv_img_dsc_t image_data;
    lv_style_t backgroud_type;
    divoom_lvgl_photo_view_para_info *pic_info;
    divoom_pic_phone_manage_item_offset pic_para;
    Pdivoom_pic_phone_manage_slide_config pslide_config;
    lv_timer_t *return_pre_timer;
    uint8 *image_buf;
}divoom_lvgl_photo_view_ctrl, *Pdivoom_lvgl_photo_view_ctrl;

#define DIVOOM_LVGL_PHOTO_VIEW_DEFER_MS  50
#define DIVOOM_LVGL_PHOTO_VIEW_IMAGE_BUF_SIZE  (DIVOOM_SCREEN_HIGH * DIVOOM_SCREEN_WIDTH * 3)

static void divoom_lvgl_photo_view_release_media(Pdivoom_lvgl_photo_view_ctrl phandle)
{
    if (!phandle)
    {
        return;
    }
    divoom_lvgl_play_gif_file_destory_handle(phandle->play_gif_handle, 0);
    phandle->play_gif_handle = NULL;
    if (phandle->play_video_flag && phandle->playing_video)
    {
        divoom_menu_control_change_disp_mode(phandle->main_obj, 0);
        divoom_local_movie_play_stop_movie();
        phandle->playing_video = 0;
    }
    phandle->play_video_flag = 0;
    if (phandle->image_obj && lv_obj_is_valid(phandle->image_obj))
    {
        lv_img_set_src(phandle->image_obj, NULL);
    }
    if (phandle->image_buf)
    {
        divoom_system_free(phandle->image_buf);
        phandle->image_buf = NULL;
        memset(&phandle->image_data, 0, sizeof(phandle->image_data));
    }
}

static int divoom_lvgl_photo_view_alloc_image_buf(Pdivoom_lvgl_photo_view_ctrl phandle)
{
    if (!phandle)
    {
        return 0;
    }
    if (phandle->image_buf)
    {
        return 1;
    }
    phandle->image_buf = divoom_system_malloc(DIVOOM_LVGL_PHOTO_VIEW_IMAGE_BUF_SIZE);
    if (!phandle->image_buf)
    {
        divoom_print("[photo_view] alloc image_buf fail\n");
        return 0;
    }
    divoom_lv_img_dsc_init_fmt(&phandle->image_data, DIVOOM_SCREEN_WIDTH,
        DIVOOM_SCREEN_HIGH, LV_COLOR_FORMAT_RGB565A8, phandle->image_buf);
    if (phandle->image_obj && lv_obj_is_valid(phandle->image_obj))
    {
        lv_img_set_src(phandle->image_obj, &phandle->image_data);
    }
    return 1;
}

static void divoom_lvgl_photo_view_deferred_return_cb(lv_timer_t *timer)
{
    Pdivoom_lvgl_photo_view_ctrl phandle;

    phandle = (Pdivoom_lvgl_photo_view_ctrl)lv_timer_get_user_data(timer);
    if (phandle)
    {
        phandle->return_pre_timer = NULL;
    }
    lv_timer_del(timer);
    divoom_menu_control_return_pre();
}

static void divoom_lvgl_photo_view_schedule_alloc_fail_return(Pdivoom_lvgl_photo_view_ctrl phandle)
{
    lv_timer_t *timer;

    if (!phandle || phandle->return_pre_timer)
    {
        return;
    }
    timer = lv_timer_create(divoom_lvgl_photo_view_deferred_return_cb,
        DIVOOM_LVGL_PHOTO_VIEW_DEFER_MS, phandle);
    if (timer)
    {
        phandle->return_pre_timer = timer;
        lv_timer_set_repeat_count(timer, 1);
    }
}

int divoom_lvgl_photo_view_read_image_info(Pdivoom_lvgl_photo_view_ctrl phandle, uint8 *image_buf, int image_id, int rotate_flag, int  mirror_flag, int video_flag)
{
    char file_name[256];

    (void)image_buf;
    (void)rotate_flag;
    (void)mirror_flag;

    if (!phandle || !phandle->image_buf)
    {
        return 0;
    }

    if(phandle->pic_info->album_id >= DIVOOM_CUSTOM_START_CLOCK_ID)
    {
        divoom_pic_phone_manage_get_picture_name(file_name, image_id);
    }
    else
    {
        divoom_custom_photo_get_pic_name_from_id(image_id, file_name);
    }
    video_flag = strstr(file_name, "mp4") != NULL;
    
    if(strlen(file_name) == 0 || divoom_app_com_check_file_exist(file_name) == 0)
    {
        divoom_print("the file %s isn't exist!\n", file_name);
        return 0;
    }
    divoom_lvgl_play_gif_file_destory_handle(phandle->play_gif_handle, 0);
    phandle->play_gif_handle = NULL;
    if(video_flag)
    {
        divoom_local_movie_play_play(file_name, phandle->pslide_config->video_volume, -1, NULL, phandle);
        lv_obj_add_flag(phandle->image_obj, LV_OBJ_FLAG_HIDDEN);
        divoom_menu_control_change_disp_mode(phandle->main_obj, 1);
        phandle->play_video_flag = 1;
        phandle->playing_video = 1;
    }
    else
    {
        
        lv_obj_clear_flag(phandle->image_obj, LV_OBJ_FLAG_HIDDEN);
        if(phandle->play_video_flag)
        {
            divoom_menu_control_change_disp_mode(phandle->main_obj, 0);
            divoom_local_movie_play_stop_movie();
            phandle->playing_video = 0;
        }
        
        phandle->play_video_flag = 0;
        //divoom_pic_phone_manage_get_picture_disp_buf(file_name, image_buf, &dest_w, &dest_h, 0, 0, 0, NULL, 0);
        

        //divoom_print("pic info: %d, %s, %d, %d, size: %d, %d\n", image_id, file_name, rotate_flag, mirror_flag, dest_w, dest_h);
        phandle->play_gif_handle = divoom_lvgl_play_gif_file_create_handle(file_name, phandle->image_obj, &phandle->image_data, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH, 0, 0);
        divoom_lvgl_play_gif_file_start_play(phandle->play_gif_handle);


        lv_img_set_src(phandle->image_obj, &phandle->image_data);
        lv_obj_invalidate(phandle->image_obj);
    }
    return 1;
}

static int divoom_lvgl_photo_view_step(Pdivoom_lvgl_photo_view_ctrl phandle, int dir)
{
    int i;
    int image_id;
    int pic_num;

    if (!phandle || !phandle->pic_info || !phandle->pic_info->pic_num)
    {
        return 0;
    }
    pic_num = *phandle->pic_info->pic_num;
    if (pic_num <= 0)
    {
        return 0;
    }

    for (i = 0; i < pic_num; i++)
    {
        if (dir > 0)
        {
            if (phandle->cur_disp_pos < pic_num - 1)
            {
                phandle->cur_disp_pos++;
            }
            else
            {
                phandle->cur_disp_pos = 0;
            }
        }
        else
        {
            if (phandle->cur_disp_pos > 0)
            {
                phandle->cur_disp_pos--;
            }
            else
            {
                phandle->cur_disp_pos = pic_num - 1;
            }
        }

        if (phandle->pic_info->album_id >= DIVOOM_CUSTOM_START_CLOCK_ID)
        {
            image_id = divoom_pic_album_manage_get_disp_order_pic_id(phandle->pic_info->pget_pic_handle,
                phandle->cur_disp_pos, NULL);
        }
        else
        {
            image_id = divoom_custom_photo_get_pic_id(phandle->pic_info->pget_pic_handle, phandle->cur_disp_pos);
        }

        if (divoom_lvgl_photo_view_read_image_info(phandle, phandle->image_buf, image_id, 0, 0, 0))
        {
            return 1;
        }
    }
    return 0;
}

static void divoom_lvgl_photo_view_event_cb(lv_event_t * e)
{
    Pdivoom_lvgl_photo_view_ctrl phandle = (Pdivoom_lvgl_photo_view_ctrl)lv_event_get_user_data(e);
    uint8_t key_code;
    char *key_info;

    if (lv_event_get_code(e) != LV_EVENT_KEY)
    {
        return;
    }

    key_info = (char *)lv_event_get_param(e);
    key_code = divoom_astrootoo_ui_key_code_from_event(e);

    if (divoom_astrootoo_ui_key_is_esc(key_code))
    {
        if (phandle->menu_id == DIVOOM_MENU_CONTROL_ID_ACCEPT_APP_PHOTO_VIEW)
        {
            divoom_main_keypad_send_key_info(key_info[0], key_info[1]);
        }
        else
        {
            lv_timer_t *timer;

            if (phandle->return_pre_timer)
            {
                return;
            }
            timer = lv_timer_create(divoom_lvgl_photo_view_deferred_return_cb,
                DIVOOM_LVGL_PHOTO_VIEW_DEFER_MS, phandle);
            if (timer)
            {
                phandle->return_pre_timer = timer;
                lv_timer_set_repeat_count(timer, 1);
            }
        }
        return;
    }

    if (!divoom_astrootoo_ui_key_is_menu_press(e, key_code))
    {
        return;
    }

    if (key_code == DIVOOM_LVGL_KEY_PRE || divoom_astrootoo_ui_key_is_right(key_code))
    {
        divoom_lvgl_photo_view_step(phandle, 1);
        return;
    }
    if (key_code == DIVOOM_LVGL_KEY_NEXT || divoom_astrootoo_ui_key_is_left(key_code))
    {
        divoom_lvgl_photo_view_step(phandle, -1);
        return;
    }

    divoom_main_keypad_send_key_info(key_info[0], key_info[1]);
}


void *divoom_lvgl_photo_view_init(void *user_data, int menu_id)
{
    Pdivoom_lvgl_photo_view_ctrl  phandle;

    phandle = divoom_system_malloc(sizeof(divoom_lvgl_photo_view_ctrl));
    phandle->menu_id = menu_id;
    phandle->pslide_config = divoom_pic_phone_manage_get_slide_config();
    phandle->main_obj = lv_btn_create(NULL);
    phandle->pic_info = (Pdivoom_lvgl_photo_view_para_info)user_data;
    phandle->cur_disp_pos = phandle->pic_info->cur_pos;
    lv_obj_set_size(phandle->main_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
    lv_style_init(&phandle->backgroud_type);
    lv_style_set_bg_color(&phandle->backgroud_type, lv_color_hex(0x000000));
    lv_style_set_text_color(&phandle->backgroud_type, lv_color_hex(DIVOOM_MENNU_TEXT_COLOR));
    lv_obj_add_style(phandle->main_obj, &phandle->backgroud_type, 0);
    phandle->playing_video = 0;

    phandle->image_obj = lv_img_create(phandle->main_obj);
    lv_obj_set_size(phandle->image_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
    lv_obj_center(phandle->image_obj);
    if(phandle->pic_info->album_id >= DIVOOM_CUSTOM_START_CLOCK_ID)
    {
        divoom_pic_album_manage_get_disp_order_pic_id(phandle->pic_info->pget_pic_handle,phandle->cur_disp_pos, &phandle->pic_para);
        divoom_print("view pic info: %d, %d\n", phandle->cur_disp_pos, phandle->pic_para.id);
    }
    else
    {
    }

    lv_obj_add_event_cb(phandle->main_obj, divoom_lvgl_photo_view_event_cb,
        LV_EVENT_KEY, phandle);
    lv_group_add_obj(lv_group_get_default(), phandle->main_obj);
    
    return phandle;
}

void divoom_lvgl_photo_view_update_info(void *handle, void *user_data)
{
    Pdivoom_lvgl_photo_view_ctrl  phandle = (Pdivoom_lvgl_photo_view_ctrl)handle;
    
    phandle->pic_info = (Pdivoom_lvgl_photo_view_para_info)user_data;
    phandle->cur_disp_pos = phandle->pic_info->cur_pos;

    if (!divoom_lvgl_photo_view_alloc_image_buf(phandle))
    {
        divoom_print("[photo_view] update_info alloc image_buf fail, return pre\n");
        divoom_lvgl_photo_view_schedule_alloc_fail_return(phandle);
        return;
    }

    if(phandle->pic_info->album_id >= DIVOOM_CUSTOM_START_CLOCK_ID)
    {
        divoom_pic_album_manage_get_disp_order_pic_id(phandle->pic_info->pget_pic_handle,phandle->cur_disp_pos, &phandle->pic_para);
        divoom_print("view pic info: %d, %d\n", phandle->cur_disp_pos, phandle->pic_para.id);
        divoom_lvgl_photo_view_read_image_info(phandle, phandle->image_buf, phandle->pic_para.id, phandle->pic_para.rotate_flag, phandle->pic_para.mirror_flag, phandle->pic_para.pic_video_flag);
    }
    else
    {
        int image_id = divoom_custom_photo_get_pic_id(phandle->pic_info->pget_pic_handle,phandle->cur_disp_pos);
        divoom_lvgl_photo_view_read_image_info(phandle, phandle->image_buf, image_id, 0, 0, 0);
    }
    
    
    
}

void divoom_lvgl_photo_view_pause(void *handle)
{
    Pdivoom_lvgl_photo_view_ctrl  phandle = (Pdivoom_lvgl_photo_view_ctrl)handle;

    if (!phandle)
    {
        return;
    }
    if (phandle->return_pre_timer)
    {
        lv_timer_del(phandle->return_pre_timer);
        phandle->return_pre_timer = NULL;
    }
    if (phandle->main_obj && lv_obj_is_valid(phandle->main_obj))
    {
        lv_group_remove_obj(phandle->main_obj);
    }
    divoom_lvgl_photo_view_release_media(phandle);
}

void divoom_lvgl_photo_view_disp(void *handle, void *active_obj)
{
    Pdivoom_lvgl_photo_view_ctrl  phandle = (Pdivoom_lvgl_photo_view_ctrl)handle;
    lv_group_t *grp;

    (void)active_obj;
    if (!divoom_lvgl_photo_view_alloc_image_buf(phandle))
    {
        divoom_print("[photo_view] disp alloc image_buf fail, return pre\n");
        divoom_lvgl_photo_view_schedule_alloc_fail_return(phandle);
        return;
    }
    grp = lv_group_get_default();
    if (grp && phandle->main_obj)
    {
        if (!lv_obj_get_group(phandle->main_obj))
        {
            lv_group_add_obj(grp, phandle->main_obj);
        }
        lv_group_focus_obj(phandle->main_obj);
    }
    
    if(phandle->pic_info->album_id >= DIVOOM_CUSTOM_START_CLOCK_ID)
    {
        int image_id = divoom_pic_album_manage_get_disp_order_pic_id(phandle->pic_info->pget_pic_handle,phandle->cur_disp_pos, NULL);
        divoom_lvgl_photo_view_read_image_info(phandle, phandle->image_buf, image_id, 0, 0, 0);
    }
    else
    {
        int image_id = divoom_custom_photo_get_pic_id(phandle->pic_info->pget_pic_handle,phandle->cur_disp_pos);
        divoom_lvgl_photo_view_read_image_info(phandle, phandle->image_buf, image_id, 0, 0, 0);
    }
    lv_scr_load(phandle->main_obj);
        
      
}

void divoom_lvgl_photo_view_exit(void *handle)
{
    Pdivoom_lvgl_photo_view_ctrl  phandle = (Pdivoom_lvgl_photo_view_ctrl)handle;

    if (phandle->return_pre_timer)
    {
        lv_timer_del(phandle->return_pre_timer);
        phandle->return_pre_timer = NULL;
    }
    divoom_lvgl_photo_view_release_media(phandle);

    if (phandle->main_obj && lv_obj_is_valid(phandle->main_obj))
    {
        lv_group_remove_obj(phandle->main_obj);
    }
    
    lv_obj_del(phandle->main_obj);
    lv_style_reset(&phandle->backgroud_type);
    divoom_system_free(phandle);
}



