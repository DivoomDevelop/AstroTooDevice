#include <malloc.h>    
#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"
#include "divoom_lvgl_play_gif_file.h"
#include "divoom_lvgl_photo_slide_effect.h"
#include "divoom_pixel_format.h"
#include "divoom_lvgl_album_video.h"

#define DIVOOM_PHOTO_MENU_FADE_OUT_TIME  2000


typedef struct _divoom_photo_play_ctrl_ctrl
{
    int  play_one_flag;
    int  refresh_num;
    int  play_movie_flag;
    int  pre_mode;
    int  pre_slide_mode;
    int  push_index;
    int  blinds_index;
    int  first_disp_flag;
    int  refresh_flag;
    int  disp_over_flag;
    int  cur_pic_id;
    int  exit_flag;
    int  start_x;
    int  start_y;
    int  area_w;
    int  area_h;
    int  play_status;
    int  photo_num;
    int  disp_index;
    int  play_speed;    //播放速度
    int  start_time;    //播放速度
    int  slide_mode;    //幻灯片效果
    int  album_id;
    int  parent_id;
    int  clock_pos;
    int  video_placeholder_shown;
    int  video_to_photo_pending;
    int  media_load_pending;
    int  default_file_shown;
    int  default_model_id;
    int  default_img_id;
    int  default_confirm_time;
    char default_file_name[256];
    char movie_file_name[256];
    lv_style_t cont_type;
    void *main_obj;
    void *first_img_obj;
    void *second_img_obj;
    lv_img_dsc_t *first_img_buf_obj;
    lv_img_dsc_t *second_img_buf_obj;
    lv_img_dsc_t first_img_buf;
    lv_img_dsc_t second_img_buf;
    lv_anim_t in_anim;
    void  *pget_pic_handle;
    void  *play_gif_handle;
    void  *play_movie_handle;
    void  *exit_sem;
    void *lock_handle;
    void *time_handle;
    void *prandom_handle;
    void *pmix_random_handle;
    Pdivoom_pic_phone_manage_slide_config pslide_config;
}divoom_photo_play_ctrl_ctrl, *Pdivoom_photo_play_ctrl_ctrl;

void divoom_photo_play_ctrl_ctrl_image_buf(Pdivoom_photo_play_ctrl_ctrl phandle, int ctrl_flag);

static Pdivoom_photo_play_ctrl_ctrl pdivoom_photo_play_ctrl_handle = NULL;

static int divoom_photo_play_ctrl_is_video(Pdivoom_photo_play_ctrl_ctrl phandle)
{
    const char *ext = strrchr(phandle->movie_file_name, '.');
    return phandle->play_movie_flag && phandle->photo_num > 0 && ext && strlen(ext) == 4 &&
        (ext[1] == 'm' || ext[1] == 'M') && (ext[2] == 'p' || ext[2] == 'P') &&
        ext[3] == '4' && ext[4] == '\0';
}

/* LVGL lock held, before the video worker reuses the former front buffer. */
static void divoom_photo_play_ctrl_video_first_frame(void *owner)
{
    Pdivoom_photo_play_ctrl_ctrl phandle = owner;
    lv_obj_add_flag(phandle->first_img_obj, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(phandle->second_img_obj, LV_OBJ_FLAG_HIDDEN);
    lv_img_cache_invalidate_src(&phandle->first_img_buf);
    lv_img_cache_invalidate_src(&phandle->second_img_buf);
    phandle->video_placeholder_shown = 0;
}

/* LVGL lock held. The loader uses try-lock to avoid blocking the UI behind its lock. */
static void divoom_photo_play_ctrl_stop_video(Pdivoom_photo_play_ctrl_ctrl phandle)
{
    phandle->video_placeholder_shown = 0;
    if(phandle->play_movie_handle)
    {
        divoom_photo_play_ctrl_video_first_frame(phandle);
        divoom_lvgl_album_video_stop(phandle->play_movie_handle);
        phandle->play_movie_handle = NULL;
        phandle->video_to_photo_pending = 1;
        /* The last writer has joined; restore the photo views before decoding into them. */
        lv_img_cache_invalidate_src(&phandle->first_img_buf);
        lv_img_cache_invalidate_src(&phandle->second_img_buf);
        divoom_rgb565a8_clear((uint8 *)phandle->first_img_buf.data,
            phandle->area_w * phandle->area_h, 0, 0);
        divoom_rgb565a8_clear((uint8 *)phandle->second_img_buf.data,
            phandle->area_w * phandle->area_h, 0, 0);
    }
}

static void divoom_photo_play_ctrl_start_video(Pdivoom_photo_play_ctrl_ctrl phandle, int show_entry_default)
{
    divoom_lvgl_album_video_buffers buffers = {
        { (void *)phandle->first_img_buf.data, (void *)phandle->second_img_buf.data },
        { phandle->first_img_buf.data_size, phandle->second_img_buf.data_size },
        divoom_photo_play_ctrl_video_first_frame, phandle
    };
    divoom_photo_play_ctrl_stop_video(phandle);
    phandle->video_to_photo_pending = 0;
    lv_anim_del(phandle, NULL);
    lv_obj_set_pos(phandle->first_img_obj, 0, 0);
    lv_obj_set_pos(phandle->second_img_obj, 0, 0);
    lv_img_set_zoom(phandle->first_img_obj, LV_IMG_ZOOM_NONE);
    lv_img_set_zoom(phandle->second_img_obj, LV_IMG_ZOOM_NONE);
    lv_obj_set_style_img_opa(phandle->first_img_obj, LV_OPA_100, 0);
    lv_obj_set_style_img_opa(phandle->second_img_obj, LV_OPA_100, 0);
    lv_obj_clear_flag(phandle->main_obj, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(phandle->first_img_obj, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(phandle->second_img_obj, LV_OBJ_FLAG_HIDDEN);
    /* Only the initial video on dial entry gets a default. Later album
     * transitions must never insert the default between user media. */
    if(show_entry_default && phandle->first_img_buf.data && phandle->default_file_name[0] &&
        divoom_app_com_check_file_exist(phandle->default_file_name))
    {
        void *placeholder;
        divoom_rgb565a8_clear((uint8 *)phandle->first_img_buf.data,
            phandle->area_w * phandle->area_h, 0, 0);
        placeholder = divoom_lvgl_play_gif_file_create_handle(
            phandle->default_file_name, phandle->first_img_obj,
            &phandle->first_img_buf, phandle->area_w, phandle->area_h, 0, 1);
        if(placeholder)
            divoom_lvgl_play_gif_file_destory_handle(placeholder, 0);
        lv_img_cache_invalidate_src(&phandle->first_img_buf);
        lv_img_set_src(phandle->first_img_obj, &phandle->first_img_buf);
        lv_obj_clear_flag(phandle->first_img_obj, LV_OBJ_FLAG_HIDDEN);
        lv_obj_invalidate(phandle->first_img_obj);
        phandle->video_placeholder_shown = 1;
    }
    phandle->play_movie_handle = divoom_lvgl_album_video_start_with_buffers(phandle->main_obj,
        phandle->movie_file_name, phandle->area_w, phandle->area_h,
        phandle->pslide_config->play_time,
        phandle->play_one_flag || phandle->photo_num == 1, &buffers);
    phandle->disp_over_flag = phandle->play_movie_handle == NULL;
    phandle->first_disp_flag = 0;
    if(!phandle->play_movie_handle)
        divoom_print("album video busy or unavailable: %s\n", phandle->movie_file_name);
}

int divoom_photo_play_ctrl_play_over_callback(void *user_data, int status)
{
    Pdivoom_photo_play_ctrl_ctrl phandle = (Pdivoom_photo_play_ctrl_ctrl)user_data;

    if(phandle->play_one_flag || (phandle->photo_num == 1 && status == 0))
    {
        divoom_print("replay!\n");
        return 0;
    }
    if(status == 1)
    {
        divoom_print("play over!\n");
        phandle->disp_over_flag = 1;
    }

    return  1;
}

int divoom_photo_play_ctrl_load_pic_data(Pdivoom_photo_play_ctrl_ctrl phandle, lv_img_dsc_t *image_data, void *image_obj, int lock_flag)
{    
    int i,  image_w, image_h, dest_w, dest_h,fill_flag, pic_id;
    char *file_name = phandle->movie_file_name;
    uint8 *jpeg_buf = NULL;

    if(divoom_light_app_check_cloud_photo_clock(phandle->album_id))
    {
        phandle->photo_num = divoom_cloud_photo_ctrl_get_total();
    }
    else if(phandle->album_id < DIVOOM_CUSTOM_START_CLOCK_ID)
    {
        phandle->photo_num = divoom_custom_photo_get_total_items(phandle->pget_pic_handle);
    }
    else
    {
        phandle->photo_num = divoom_pic_album_manage_get_disp_order_pic_num(phandle->pget_pic_handle);
    }
    if(phandle->photo_num == 0)
    {
        void *old_gif;

        phandle->media_load_pending = 0;
        if(lock_flag)
        {
            if(divoom_get_lvgl_try_lock())
            {
                return 0;
            }
        }
        old_gif = phandle->play_gif_handle;
        phandle->play_gif_handle = NULL;
        divoom_lvgl_play_gif_file_retire_handle(old_gif);
        divoom_photo_play_ctrl_stop_video(phandle);
        divoom_photo_play_ctrl_ctrl_image_buf(phandle, 1);
        if(lock_flag)
        {
            divoom_free_lvgl_lock();
        }
        /* The loader owns the slot; join the old decoder outside its LVGL lock. */
        divoom_lvgl_play_gif_file_destory_handle(old_gif, 0);
        if(strlen(phandle->default_file_name) && divoom_app_com_check_file_exist(phandle->default_file_name))
        {
            phandle->play_gif_handle = divoom_lvgl_play_gif_file_create_handle(phandle->default_file_name, image_obj, image_data, phandle->area_w, phandle->area_h, 0, 0);
            phandle->default_file_shown = 1;
        }
        else if(strlen(phandle->default_file_name) == 0)
        {
            if(divoom_light_app_check_cloud_photo_clock(phandle->album_id))
            {
                divoom_rgb565a8_clear((uint8 *)image_data->data, phandle->area_w * phandle->area_h, 0, 0);
            }
            else if(phandle->album_id < DIVOOM_CUSTOM_START_CLOCK_ID)
            {
                divoom_rgb565a8_clear((uint8 *)image_data->data, phandle->area_w * phandle->area_h, 0, 0);
            }
            else
            {
                phandle->play_gif_handle = divoom_lvgl_play_gif_file_create_handle(DIVOOM_EMPTY_ALBUM_PHOTO_NAME, image_obj, image_data, phandle->area_w, phandle->area_h, 0, 0);
            }
        }
        
        return 1;
    }
    
    if(!phandle->media_load_pending)
    {
        if(divoom_light_app_check_cloud_photo_clock(phandle->album_id))
        {
            pic_id = divoom_cloud_photo_ctrl_get_item(file_name);
            phandle->cur_pic_id = pic_id;
        }
        else if(phandle->album_id < DIVOOM_CUSTOM_START_CLOCK_ID)
        {
            pic_id = divoom_custom_photo_get_file_name(phandle->pget_pic_handle, file_name);
            phandle->cur_pic_id = pic_id;
        
        }
        else
        {
            phandle->cur_pic_id = divoom_pic_album_manage_get_disp_order_pic_id(phandle->pget_pic_handle, phandle->disp_index, NULL);
            divoom_pic_phone_manage_get_picture_name(file_name, phandle->cur_pic_id);
        }
        phandle->media_load_pending = 1;
    }
    pic_id = phandle->cur_pic_id;
    if(divoom_app_com_check_file_exist(file_name) == 0)
    {
        phandle->media_load_pending = 0;
        if(divoom_light_app_check_cloud_photo_clock(phandle->album_id))
        {
            divoom_cloud_photo_redownload_pic_id(pic_id);
        }
        else if(phandle->album_id < DIVOOM_CUSTOM_START_CLOCK_ID)
        {
            divoom_clock_id_info clock_info = {0};
            clock_info.clock_id = phandle->album_id;
            clock_info.parent_id = phandle->parent_id;
            clock_info.clock_pos = phandle->clock_pos;
            divoom_custom_photo_delete_item(&clock_info, pic_id);
        }
        else
        {
            divoom_pic_album_manage_remove_list_from_id(phandle->album_id, &phandle->cur_pic_id, 1);
        }
        //如果这个照片不存在，将删除, 并修改加载计数，重新加载
        return 0;
        
    }
    if(phandle->play_gif_handle)
    {
        void *old_gif;

        if(lock_flag)
        {
            if(divoom_get_lvgl_try_lock())
            {
                return 0;
            }
        }
        old_gif = phandle->play_gif_handle;
        phandle->play_gif_handle = NULL;
        divoom_lvgl_play_gif_file_retire_handle(old_gif);
        if(lock_flag)
        {
            divoom_free_lvgl_lock();
        }
        divoom_lvgl_play_gif_file_destory_handle(old_gif, 0);
    }
    dest_w = phandle->area_w;
    dest_h = phandle->area_h;
    if(divoom_photo_play_ctrl_is_video(phandle))
    {
        //这个位置不需要处理
        
    }
    else
    {
        
        /* Join the video under the LVGL lock before this loader writes a borrowed buffer. */
        if(lock_flag)
        {
            if(divoom_get_lvgl_try_lock())
            {
                return 0;
            }
        }
        divoom_photo_play_ctrl_stop_video(phandle);
        divoom_photo_play_ctrl_ctrl_image_buf(phandle, 1);
        if(lock_flag)
        {
            divoom_free_lvgl_lock();
        }
        phandle->play_gif_handle = divoom_lvgl_play_gif_file_create_handle(file_name, image_obj, image_data, dest_w, dest_h, 0, 0);
        divoom_lvgl_play_gif_file_start_play(phandle->play_gif_handle);
    }
    /* Commit selection only after the GUI preparation succeeded. A busy GUI
     * retries this same file, including cloud/custom iterators that consumed next. */
    phandle->media_load_pending = 0;
    phandle->disp_index++;
    if(phandle->disp_index >= phandle->photo_num)
        phandle->disp_index = 0;
    return 1;
}

void divoom_photo_play_ctrl_in_anim_fade_out(void * a, int32_t v)
{
    Pdivoom_photo_play_ctrl_ctrl phandle = (Pdivoom_photo_play_ctrl_ctrl)a;



    lv_obj_set_style_img_opa(phandle->first_img_obj, v , 0);
    lv_obj_set_style_img_opa(phandle->second_img_obj, LV_OPA_100 -v , 0);
    if(v == 255)
    {
        lv_obj_add_flag(phandle->second_img_obj, LV_OBJ_FLAG_HIDDEN);
        phandle->disp_over_flag = 1;
    }
}

void divoom_photo_play_ctrl_in_anim_push_down(void * a, int32_t v)
{
    Pdivoom_photo_play_ctrl_ctrl phandle = (Pdivoom_photo_play_ctrl_ctrl)a;



    lv_obj_set_style_img_opa(phandle->first_img_obj, v , 0);
    lv_obj_set_y(phandle->first_img_obj, phandle->area_h * (v - 255)/255);
    lv_obj_set_style_img_opa(phandle->second_img_obj, LV_OPA_100  - v, 0);
    lv_obj_set_y(phandle->second_img_obj, phandle->area_h *v/255);
    if(v == 255)
    {
        lv_obj_add_flag(phandle->second_img_obj, LV_OBJ_FLAG_HIDDEN);
        phandle->disp_over_flag = 1;
    }
}

void divoom_photo_play_ctrl_in_anim_push_up(void * a, int32_t v)
{
    Pdivoom_photo_play_ctrl_ctrl phandle = (Pdivoom_photo_play_ctrl_ctrl)a;



    lv_obj_set_style_img_opa(phandle->first_img_obj, v , 0);
    lv_obj_set_y(phandle->first_img_obj, phandle->area_h * (255 - v)/255);
    lv_obj_set_style_img_opa(phandle->second_img_obj, LV_OPA_100  - v, 0);
    lv_obj_set_y(phandle->second_img_obj, phandle->area_h *(0 - v)/255);
    if(v == 255)
    {
        lv_obj_add_flag(phandle->second_img_obj, LV_OBJ_FLAG_HIDDEN);
        phandle->disp_over_flag = 1;
    }
}

void divoom_photo_play_ctrl_in_anim_push_right(void * a, int32_t v)
{
    Pdivoom_photo_play_ctrl_ctrl phandle = (Pdivoom_photo_play_ctrl_ctrl)a;



    lv_obj_set_style_img_opa(phandle->first_img_obj, v, 0);
    lv_obj_set_x(phandle->first_img_obj, phandle->area_w * (v - 255)/255);

    lv_obj_set_style_img_opa(phandle->second_img_obj, LV_OPA_100  - v, 0);
    lv_obj_set_x(phandle->second_img_obj, phandle->area_w * v / 255);
    if(v == 255)
    {
        lv_obj_add_flag(phandle->second_img_obj, LV_OBJ_FLAG_HIDDEN);
        phandle->disp_over_flag = 1;
    }
}

void divoom_photo_play_ctrl_in_anim_push_left(void * a, int32_t v)
{
    Pdivoom_photo_play_ctrl_ctrl phandle = (Pdivoom_photo_play_ctrl_ctrl)a;



    lv_obj_set_style_img_opa(phandle->first_img_obj, v, 0);
    lv_obj_set_x(phandle->first_img_obj, phandle->area_w * (255 - v)/255);

    lv_obj_set_style_img_opa(phandle->second_img_obj, LV_OPA_100  - v, 0);
    lv_obj_set_x(phandle->second_img_obj, phandle->area_w * (0 - v) / 255);
    if(v == 255)
    {
        lv_obj_add_flag(phandle->second_img_obj, LV_OBJ_FLAG_HIDDEN);
        phandle->disp_over_flag = 1;
    }
}

void divoom_photo_play_ctrl_in_anim_rotate(void * a, int32_t v)
{
    Pdivoom_photo_play_ctrl_ctrl phandle = (Pdivoom_photo_play_ctrl_ctrl)a;


    if(v > LV_OPA_100)
    {
        lv_obj_set_style_img_opa(phandle->first_img_obj, LV_OPA_100, 0);
        lv_obj_set_style_img_opa(phandle->second_img_obj, 0, 0);
    }
    else
    {
        lv_obj_set_style_img_opa(phandle->first_img_obj, v, 0);

        lv_obj_set_style_img_opa(phandle->second_img_obj, LV_OPA_100 - v, 0);
    }
    lv_img_set_zoom(phandle->first_img_obj, v);
    if(v == LV_IMG_ZOOM_NONE)
    {
        lv_obj_add_flag(phandle->second_img_obj, LV_OBJ_FLAG_HIDDEN);
        phandle->disp_over_flag = 1;
    }
}

void divoom_photo_play_ctrl_in_anim_blinds(void * a, int32_t v)
{
    Pdivoom_photo_play_ctrl_ctrl phandle = (Pdivoom_photo_play_ctrl_ctrl)a;

    if(phandle->blinds_index == 0)
    {
        divoom_app_com_set_image_opa((uint8 *)phandle->first_img_buf_obj->data, phandle->refresh_num, 0, (v - phandle->refresh_num), phandle->area_h, 0xFF, phandle->area_w, phandle->area_h);
        divoom_app_com_set_image_opa((uint8 *)phandle->second_img_buf_obj->data, phandle->refresh_num, 0, (v - phandle->refresh_num), phandle->area_h, 0, phandle->area_w, phandle->area_h);
    }
    else if(phandle->blinds_index == 1)
    {
        divoom_app_com_set_image_opa((uint8 *)phandle->first_img_buf_obj->data, phandle->area_w  - v, 0, (v - phandle->refresh_num), phandle->area_h, 0xFF, phandle->area_w, phandle->area_h);
        divoom_app_com_set_image_opa((uint8 *)phandle->second_img_buf_obj->data, phandle->area_w  - v, 0, (v - phandle->refresh_num), phandle->area_h, 0, phandle->area_w, phandle->area_h);
    }
    else if(phandle->blinds_index == 2)
    {
        divoom_app_com_set_image_opa((uint8 *)phandle->first_img_buf_obj->data, 0, phandle->refresh_num, phandle->area_w, (v - phandle->refresh_num), 0xFF, phandle->area_w, phandle->area_h);
        divoom_app_com_set_image_opa((uint8 *)phandle->second_img_buf_obj->data, 0, phandle->refresh_num, phandle->area_w, (v - phandle->refresh_num), 0, phandle->area_w, phandle->area_h);
    }
    else
    {
        divoom_app_com_set_image_opa((uint8 *)phandle->first_img_buf_obj->data, 0, phandle->area_h - v, phandle->area_w, (v - phandle->refresh_num), 0xFF, phandle->area_w, phandle->area_h);
        divoom_app_com_set_image_opa((uint8 *)phandle->second_img_buf_obj->data, 0, phandle->area_h - v, phandle->area_w, (v - phandle->refresh_num), 0, phandle->area_w, phandle->area_h);
    }
    phandle->refresh_num = v;

    if((v == phandle->area_w && phandle->blinds_index < 2) || (v == phandle->area_h && phandle->blinds_index >= 2))
    {
        lv_obj_add_flag(phandle->second_img_obj, LV_OBJ_FLAG_HIDDEN);
        phandle->disp_over_flag = 1;
    }
    lv_obj_invalidate(phandle->second_img_obj);
    lv_obj_invalidate(phandle->first_img_obj);
}

void divoom_photo_play_ctrl_in_anim_mosaic(void * a, int32_t v)
{
    int index, i, j, row_num, col_num;
    Pdivoom_photo_play_ctrl_ctrl phandle = (Pdivoom_photo_play_ctrl_ctrl)a;


    row_num = (phandle->area_w + 9)/10;
    if(v == LV_IMG_ZOOM_NONE)
    {
        int last_num = divoom_random_get_no_use(phandle->prandom_handle);
        for(j = 0; j < last_num; j++)
        {
            index = divoom_random_get(phandle->prandom_handle);
            divoom_app_com_set_image_opa((uint8 *)phandle->first_img_buf_obj->data, (index%row_num)*10, (index/row_num)*10, 10, 10, 0xFF, phandle->area_w, phandle->area_h);
            divoom_app_com_set_image_opa((uint8 *)phandle->second_img_buf_obj->data, (index%row_num)*10, (index/row_num)*10, 10, 10, 0, phandle->area_w, phandle->area_h);
        }
        divoom_random_destroy(phandle->prandom_handle);
        lv_obj_add_flag(phandle->second_img_obj, LV_OBJ_FLAG_HIDDEN);
        phandle->disp_over_flag = 1;
        phandle->prandom_handle = NULL;
    }
    else
    {
        int refresh_num;
        
        col_num = (phandle->area_h + 9)/10;
        refresh_num = v*row_num*col_num/LV_IMG_ZOOM_NONE;
        for(i =0; i < refresh_num - phandle->refresh_num; i++)
        {
            index = divoom_random_get(phandle->prandom_handle);
            divoom_app_com_set_image_opa((uint8 *)phandle->first_img_buf_obj->data, (index%row_num)*10, (index/row_num)*10, 10, 10, 0xFF, phandle->area_w, phandle->area_h);
            divoom_app_com_set_image_opa((uint8 *)phandle->second_img_buf_obj->data, (index%row_num)*10, (index/row_num)*10, 10, 10, 0, phandle->area_w, phandle->area_h);
        }
        phandle->refresh_num = refresh_num;
    }
    lv_obj_invalidate(phandle->second_img_obj);
    lv_obj_invalidate(phandle->first_img_obj);

}


void divoom_photo_play_ctrl_disp_refresh_ui(Pdivoom_photo_play_ctrl_ctrl phandle)
{
    int slide_mode, push_index = -1;
    int from_video = phandle->play_movie_handle != NULL || phandle->video_to_photo_pending;
    void *tmp_obj;

    if(divoom_photo_play_ctrl_is_video(phandle))
    {
        divoom_photo_play_ctrl_start_video(phandle, 0);
        return;
    }
    divoom_photo_play_ctrl_stop_video(phandle);
    phandle->video_to_photo_pending = 0;

    if(phandle->photo_num <= 1)
    {
        if(phandle->photo_num == 0)
        {
            lv_obj_clear_flag(phandle->main_obj, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(phandle->first_img_obj, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(phandle->second_img_obj, LV_OBJ_FLAG_HIDDEN);
            /* The loader has already replaced any previous GIF with the default. */
        }
        else
        {
            lv_obj_clear_flag(phandle->first_img_obj, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(phandle->second_img_obj, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(phandle->main_obj, LV_OBJ_FLAG_HIDDEN);
        }
        phandle->first_disp_flag = 0;
        
        return;
    }
    if(phandle->first_disp_flag)
    {
        lv_obj_invalidate(phandle->first_img_obj);
        lv_obj_clear_flag(phandle->main_obj, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(phandle->first_img_obj, LV_OBJ_FLAG_HIDDEN);
        phandle->first_disp_flag = 0;
        
        return;
    }
    tmp_obj = phandle->first_img_obj;
    phandle->first_img_obj = phandle->second_img_obj;
    phandle->second_img_obj = tmp_obj;
    tmp_obj = phandle->first_img_buf_obj;
    phandle->first_img_buf_obj = phandle->second_img_buf_obj;
    phandle->second_img_buf_obj = tmp_obj;
    lv_anim_del(phandle, NULL);
    lv_obj_clear_flag(phandle->main_obj, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(phandle->first_img_obj, LV_OBJ_FLAG_HIDDEN);
    /* 视频借用过照片缓冲，直接显示重新解码的新图，不拿旧视频数据参与照片过渡。 */
    if(from_video)
    {
        lv_obj_set_pos(phandle->first_img_obj, 0, 0);
        lv_img_set_zoom(phandle->first_img_obj, LV_IMG_ZOOM_NONE);
        lv_obj_set_style_img_opa(phandle->first_img_obj, LV_OPA_100, 0);
        lv_obj_add_flag(phandle->second_img_obj, LV_OBJ_FLAG_HIDDEN);
        lv_obj_invalidate(phandle->first_img_obj);
        phandle->play_status = 0;
        phandle->disp_over_flag = 1;
        return;
    }
    lv_obj_clear_flag(phandle->second_img_obj, LV_OBJ_FLAG_HIDDEN);
    lv_obj_invalidate(phandle->first_img_obj);
    lv_obj_invalidate(phandle->second_img_obj);

    lv_anim_init(&phandle->in_anim);
    phandle->play_status = 1;

    lv_anim_set_var(&phandle->in_anim, phandle);

    lv_anim_path_linear(&phandle->in_anim);

    lv_anim_set_path_cb(&phandle->in_anim, lv_anim_path_ease_in_out);

    lv_anim_set_time(&phandle->in_anim, DIVOOM_PHOTO_MENU_FADE_OUT_TIME);

    lv_anim_set_values(&phandle->in_anim, 0, 255);

    lv_obj_set_style_img_opa(phandle->first_img_obj, LV_OPA_0, 0);
    lv_anim_set_repeat_count(&phandle->in_anim, 1);

    lv_obj_set_style_img_opa(phandle->second_img_obj, LV_OPA_100, 0);
    lv_img_set_angle(phandle->first_img_obj, 0);
    lv_img_set_zoom(phandle->first_img_obj, LV_IMG_ZOOM_NONE);
    slide_mode = phandle->pslide_config->effect_type;
    if(phandle->play_gif_handle)
    {
        slide_mode = DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_NORMAL;
    }
    else if(slide_mode == DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_MIX)
    {
        if(phandle->pmix_random_handle == NULL)
        {
            phandle->pmix_random_handle = divoom_random_create(4+2*2+4);
        }
        phandle->pre_mode = divoom_random_get(phandle->pmix_random_handle);
        if(phandle->pre_mode < 4)
        {
            slide_mode = DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_FADE_OUT;
        }
        else if(phandle->pre_mode < 6)
        {
            slide_mode = DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_PUSH;
            push_index = 0;
        }
        else if(phandle->pre_mode < 8)
        {
            slide_mode = DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_PUSH;
            push_index = 1;
        }
        else if(phandle->pre_mode == 8)
        {
            slide_mode = DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_PUSH;
            push_index = 2;
        }
        else if(phandle->pre_mode  == 9)
        {
            slide_mode = DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_PUSH;
            push_index = 3;
        }
        else if(phandle->pre_mode  == 10)
        {
            slide_mode = DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_BLINDS;
        }
        else
        {
            slide_mode = DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_MOSAIC;
        }
        if(slide_mode == DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_PUSH && phandle->pre_slide_mode == slide_mode)
        {
            if(push_index == 0 && phandle->push_index == 1)
            {
                slide_mode = DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_MOSAIC;
            }
            else if(push_index == 1 && phandle->push_index == 0)
            {
                slide_mode = DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_FADE_OUT;
            }
            else if(push_index == 2 && phandle->push_index == 3)
            {
                slide_mode = DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_MOSAIC;
            }
            else if(push_index == 3 && phandle->push_index == 2)
            {
                slide_mode = DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_FADE_OUT;
            }
        }
        phandle->pre_slide_mode = slide_mode;
    }
    switch(slide_mode)
    {
        case DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_NORMAL:
            lv_obj_set_pos(phandle->first_img_obj, 0, 0);
            lv_obj_set_pos(phandle->second_img_obj, 0, 0);
            lv_obj_set_style_img_opa(phandle->first_img_obj, LV_OPA_100, 0);
            lv_obj_set_style_img_opa(phandle->second_img_obj, LV_OPA_0, 0);
            lv_obj_add_flag(phandle->second_img_obj, LV_OBJ_FLAG_HIDDEN);
            phandle->disp_over_flag = 1;
            break;

        case DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_FADE_OUT:
            lv_obj_set_pos(phandle->first_img_obj, 0, 0);
            lv_obj_set_pos(phandle->second_img_obj, 0, 0);
            lv_anim_set_exec_cb(&phandle->in_anim, (lv_anim_exec_xcb_t) divoom_photo_play_ctrl_in_anim_fade_out);
            break;

        case DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_PUSH:
            if(push_index == -1)
            {
                push_index = divoom_app_com_get_rand_index(phandle->push_index, 4);
            }
            if(push_index == 0)
            {
                lv_obj_set_pos(phandle->first_img_obj, 0 - phandle->area_w, 0);
                lv_obj_set_pos(phandle->second_img_obj, 0, 0);
                lv_anim_set_exec_cb(&phandle->in_anim, (lv_anim_exec_xcb_t) divoom_photo_play_ctrl_in_anim_push_left);
            }
            else if(push_index == 1)
            {
                lv_obj_set_pos(phandle->first_img_obj, 0 - phandle->area_w, 0);
                lv_obj_set_pos(phandle->second_img_obj, 0, 0);
                lv_anim_set_exec_cb(&phandle->in_anim, (lv_anim_exec_xcb_t) divoom_photo_play_ctrl_in_anim_push_right);
            }
            else if(push_index == 2)
            {
                lv_obj_set_pos(phandle->first_img_obj, 0, 0 - phandle->area_h);
                lv_obj_set_pos(phandle->second_img_obj, 0, 0);
                lv_anim_set_exec_cb(&phandle->in_anim, (lv_anim_exec_xcb_t) divoom_photo_play_ctrl_in_anim_push_up);
            }
            else 
            {
                lv_obj_set_pos(phandle->first_img_obj, 0, 0 - phandle->area_h);
                lv_obj_set_pos(phandle->second_img_obj, 0, 0);
                lv_anim_set_exec_cb(&phandle->in_anim, (lv_anim_exec_xcb_t) divoom_photo_play_ctrl_in_anim_push_down);
            }
            phandle->push_index = push_index;
            break;

        #if 0
        case DIVOOM_PHOTO_MENU_ROTATE:
            lv_obj_set_pos(phandle->first_img_obj, 0 , 0);
            lv_obj_set_pos(phandle->second_img_obj, 0, 0);
            lv_img_set_zoom(phandle->first_img_obj, 0);
            lv_anim_set_values(&phandle->in_anim, 0, LV_IMG_ZOOM_NONE);
            lv_anim_set_exec_cb(&phandle->in_anim, (lv_anim_exec_xcb_t) divoom_photo_play_ctrl_in_anim_rotate);
            break;
        #endif
        case DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_BLINDS:
            phandle->blinds_index = divoom_app_com_get_rand_index(phandle->blinds_index, 4);
            divoom_app_com_set_image_opa((uint8 *)phandle->first_img_buf_obj->data, 0, 0, phandle->area_w, phandle->area_h, 0, phandle->area_w, phandle->area_h);
            lv_obj_set_pos(phandle->first_img_obj, 0 , 0);
            lv_obj_set_pos(phandle->second_img_obj, 0, 0);
            lv_obj_set_style_img_opa(phandle->first_img_obj, LV_OPA_100, 0);
            lv_obj_set_style_img_opa(phandle->second_img_obj, LV_OPA_100, 0);
            lv_img_set_zoom(phandle->first_img_obj, LV_IMG_ZOOM_NONE);
            lv_img_set_zoom(phandle->second_img_obj, LV_IMG_ZOOM_NONE);
            if(phandle->blinds_index < 2)
            {
                lv_anim_set_values(&phandle->in_anim, 0, phandle->area_w);
            }
            else
            {
                lv_anim_set_values(&phandle->in_anim, 0, phandle->area_h);
            }
            lv_anim_set_exec_cb(&phandle->in_anim, (lv_anim_exec_xcb_t) divoom_photo_play_ctrl_in_anim_blinds);
            break;
            
        case DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_MOSAIC:
            if(phandle->prandom_handle)
            {
                divoom_random_destroy(phandle->prandom_handle);
                phandle->prandom_handle = NULL;
            }
            phandle->prandom_handle = divoom_random_create(((phandle->area_w +9)/10) * ((phandle->area_h + 9)/10));
            phandle->refresh_num = 0;
            divoom_app_com_set_image_opa((uint8 *)phandle->first_img_buf_obj->data, 0, 0, phandle->area_w, phandle->area_h, 0, phandle->area_w, phandle->area_h);
            lv_obj_set_pos(phandle->first_img_obj, 0 , 0);
            lv_obj_set_pos(phandle->second_img_obj, 0, 0);
            lv_obj_set_style_img_opa(phandle->first_img_obj, LV_OPA_100, 0);
            lv_obj_set_style_img_opa(phandle->second_img_obj, LV_OPA_100, 0);
            lv_img_set_zoom(phandle->first_img_obj, LV_IMG_ZOOM_NONE);
            lv_img_set_zoom(phandle->second_img_obj, LV_IMG_ZOOM_NONE);
            lv_anim_set_values(&phandle->in_anim, 0, LV_IMG_ZOOM_NONE);
            lv_anim_set_exec_cb(&phandle->in_anim, (lv_anim_exec_xcb_t) divoom_photo_play_ctrl_in_anim_mosaic);
            break;
            


    }
    if(slide_mode != DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_NORMAL)
    {
        lv_anim_start(&phandle->in_anim);
    }
}


void *divoom_photo_play_ctrl_task_main(void *user_data)
{
    int photo_num, last_pic_id;
    Pdivoom_photo_play_ctrl_ctrl phandle = (Pdivoom_photo_play_ctrl_ctrl)user_data;

    while(phandle->exit_flag == 0)
    {
        if(phandle->play_one_flag)
        {
            divoom_system_delay_ms(100);
            continue;
        }
        photo_num = phandle->photo_num;
        if(phandle->album_id >= DIVOOM_CUSTOM_START_CLOCK_ID)
        {
            phandle->photo_num = divoom_pic_album_manage_get_disp_order_pic_num(phandle->pget_pic_handle);
            last_pic_id = divoom_pic_album_manage_get_disp_last_pic_id(phandle->pget_pic_handle);
        }
        else if(divoom_light_app_check_cloud_photo_clock(phandle->album_id))
        {
            phandle->photo_num = divoom_cloud_photo_ctrl_get_total();
            last_pic_id = divoom_cloud_photo_get_last_pic_id();
        }
        else
        {
            phandle->photo_num = divoom_custom_photo_get_total_items(phandle->pget_pic_handle);
            last_pic_id = divoom_custom_photo_get_last_pic_id(phandle->pget_pic_handle);
        }
        if(photo_num != phandle->photo_num)
        {
             phandle->disp_over_flag = 1;
        }
        if(phandle->photo_num == 0 && strlen(phandle->default_file_name))
        {
            if(divoom_app_com_check_file_exist(phandle->default_file_name) == 0)
            {
                phandle->default_file_shown = 0;
                if(phandle->default_img_id > 0 && (phandle->default_confirm_time == 0 || phandle->default_confirm_time + 1500 < divoom_system_get_tickms()))
                {
                    divoom_manage_image_confirm_download(phandle->default_model_id, phandle->default_img_id, phandle->default_file_name);
                    phandle->default_confirm_time = divoom_system_get_tickms();
                }
            }
        }
        if(photo_num != phandle->photo_num || last_pic_id > 0 || phandle->start_time == 0 || (photo_num + phandle->photo_num == 1 || photo_num && phandle->photo_num == 0) || (phandle->start_time + phandle->pslide_config->play_time < divoom_system_get_tickms() && phandle->photo_num > 1 && phandle->disp_over_flag == 1) || (phandle->photo_num == 0 && phandle->default_file_shown == 0 && strlen(phandle->default_file_name) && divoom_app_com_check_file_exist(phandle->default_file_name)))
        {
            divoom_system_get_lock(phandle->lock_handle); 
            if(photo_num  == 0 || phandle->photo_num == 0)
            {
                
                if(divoom_photo_play_ctrl_load_pic_data(phandle, phandle->first_img_buf_obj, phandle->first_img_obj, 1) == 0)
                {
                    divoom_system_free_lock(phandle->lock_handle);
                    continue;
                }
                phandle->first_disp_flag = 1;
            }
            else
            {
                if(phandle->photo_num == 1)
                {
                    if(divoom_photo_play_ctrl_load_pic_data(phandle, phandle->first_img_buf_obj, phandle->first_img_obj, 1) == 0)
                    {
                        divoom_system_free_lock(phandle->lock_handle);
                        continue;
                    }
                }
                else
                {
                    if(divoom_photo_play_ctrl_load_pic_data(phandle, phandle->second_img_buf_obj, phandle->second_img_obj, 1) == 0)
                    {
                        divoom_system_free_lock(phandle->lock_handle);
                        continue;
                    }
                }
            }
            /* 必须在发布刷新前、持锁时复位计时和完成状态，避免 UI 已完成切换后又被加载线程清零。 */
            phandle->start_time = divoom_system_get_tickms();
            phandle->disp_over_flag = 0;
            phandle->refresh_flag = 1;
            divoom_system_free_lock(phandle->lock_handle);
        }
        divoom_system_delay_ms(100);
    }
    divoom_system_put_sem(phandle->exit_sem);
    
    return NULL;
}

void divoom_photo_play_ctrl_time_handle(struct _lv_timer_t *user_data)
{
    Pdivoom_photo_play_ctrl_ctrl  phandle = (Pdivoom_photo_play_ctrl_ctrl)lv_timer_get_user_data(user_data);

    
    /* Never block LVGL behind the loader, which may be trying the LVGL lock. */
    if(divoom_system_try_lock(phandle->lock_handle) != 0)
        return;
    if(phandle->refresh_flag)
    {
        divoom_photo_play_ctrl_disp_refresh_ui(phandle);
        phandle->refresh_flag = 0;
    }
    if(phandle->video_placeholder_shown && phandle->play_movie_handle &&
        divoom_lvgl_album_video_has_frame(phandle->play_movie_handle))
    {
        divoom_photo_play_ctrl_video_first_frame(phandle);
    }
    if(phandle->play_movie_handle &&
        divoom_lvgl_album_video_finished(phandle->play_movie_handle))
        phandle->disp_over_flag = 1;
    divoom_system_free_lock(phandle->lock_handle);
}
void divoom_photo_play_ctrl_ctrl_image_buf(Pdivoom_photo_play_ctrl_ctrl phandle, int ctrl_flag)
{
    if(ctrl_flag)
    {
#if 0
        /* 视频播放时不能提前显示旧图片缓冲区；等新照片解码完成后由 UI 刷新统一切换。 */
        lv_obj_clear_flag(phandle->first_img_obj, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(phandle->second_img_obj, LV_OBJ_FLAG_HIDDEN);
#endif
        if(!phandle->play_movie_handle && !phandle->video_to_photo_pending)
        {
            lv_obj_clear_flag(phandle->first_img_obj, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(phandle->second_img_obj, LV_OBJ_FLAG_HIDDEN);
        }
        if(phandle->first_img_buf.data == NULL)
        {
            phandle->first_img_buf.data = divoom_system_malloc(phandle->first_img_buf.data_size);
            phandle->second_img_buf.data = divoom_system_malloc(phandle->second_img_buf.data_size);
        }
    }
    else 
    {
        if(phandle->first_img_obj == NULL)
        {
            divoom_system_free((uint8 *)phandle->first_img_buf.data);
            phandle->first_img_buf.data = NULL;
            divoom_system_free((uint8 *)phandle->second_img_buf.data);
            phandle->second_img_buf.data = NULL;
        }
        lv_obj_add_flag(phandle->first_img_obj, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(phandle->second_img_obj, LV_OBJ_FLAG_HIDDEN);
    }
}


//获取正在播放的表盘相簿信息
int divoom_photo_play_ctrl_get_play_album_info(Pdivoom_clock_id_info pclock_info, int *pic_id)
{
    if(pdivoom_photo_play_ctrl_handle == NULL)
    {
        return 0;
    }
    if(divoom_light_app_check_cloud_photo_clock(pdivoom_photo_play_ctrl_handle->album_id))
    {
        return 0;
    }
    pclock_info->clock_id = pdivoom_photo_play_ctrl_handle->album_id;
    pclock_info->clock_pos = pdivoom_photo_play_ctrl_handle->clock_pos;
    pclock_info->parent_id = pdivoom_photo_play_ctrl_handle->parent_id;
    *pic_id = pdivoom_photo_play_ctrl_handle->cur_pic_id;

    return 1;
}

void *divoom_photo_play_ctrl_init(void *main_obj,int start_x, int start_y,int width, int high, int album_id,int effect_id, int play_speed, int parent_id, int clock_pos, int play_movie, char *default_name, int default_model_id, int default_img_id)
{
    Pdivoom_photo_play_ctrl_ctrl phandle = divoom_system_malloc(sizeof(divoom_photo_play_ctrl_ctrl));
    phandle->pslide_config = divoom_pic_phone_manage_get_slide_config();


    if(divoom_light_app_check_cloud_photo_clock(album_id))
    {
        //请求
        divoom_cloud_photo_ctrl_request_more_info(album_id);
    }

    pdivoom_photo_play_ctrl_handle = phandle;
    //速度用系统默认速度
    play_speed = phandle->pslide_config->play_time;

    if(play_speed < 3)
    {
        play_speed = 3;
    }
    phandle->play_movie_flag = play_movie;
    phandle->play_status = 0;
    phandle->main_obj = main_obj;
    phandle->album_id = album_id;
    phandle->parent_id = parent_id;
    phandle->clock_pos = clock_pos;
    phandle->area_w = width;
    phandle->area_h = high;
    phandle->start_x = start_x;
    phandle->start_y = start_y;
    phandle->exit_flag = 0;
    phandle->exit_sem = divoom_system_create_sem(1, 0);
    phandle->lock_handle = divoom_system_create_lock();
    phandle->video_placeholder_shown = 0;
    phandle->default_file_shown = 0;
    phandle->default_model_id = default_model_id;
    phandle->default_img_id = default_img_id;
    phandle->default_confirm_time = 0;
    if(default_name && strlen(default_name))
    {
        strcpy(phandle->default_file_name, default_name);
    }
    else
    {
        phandle->default_file_name[0] = 0;
    }
    divoom_print("area : %d, %d, %d,%d, play_movie:%d\n", start_x,  start_y,  width,  high, play_movie);
    phandle->first_img_buf_obj = &phandle->first_img_buf;;
    divoom_lv_img_dsc_init_fmt(&phandle->first_img_buf,
        phandle->area_w, phandle->area_h, LV_COLOR_FORMAT_RGB565A8, NULL);
    phandle->first_img_buf.data = divoom_system_malloc(phandle->first_img_buf.data_size);
    divoom_rgb565a8_clear((uint8 *)phandle->first_img_buf.data, phandle->area_w * phandle->area_h, 0, 0);

    phandle->first_img_obj = lv_img_create(phandle->main_obj);   
    lv_obj_set_pos(phandle->first_img_obj, 0, 0);
    lv_obj_set_size(phandle->first_img_obj, phandle->area_w, phandle->area_h);
    lv_img_set_src(phandle->first_img_obj, &phandle->first_img_buf);


    phandle->second_img_buf_obj = &phandle->second_img_buf;
    divoom_lv_img_dsc_init_fmt(&phandle->second_img_buf,
        phandle->area_w, phandle->area_h, LV_COLOR_FORMAT_RGB565A8, NULL);
    phandle->second_img_buf.data = divoom_system_malloc(phandle->second_img_buf.data_size);
    divoom_rgb565a8_clear((uint8 *)phandle->second_img_buf.data, phandle->area_w * phandle->area_h, 0, 0);

    phandle->second_img_obj = lv_img_create(phandle->main_obj);   
    lv_obj_set_pos(phandle->second_img_obj, 0, 0);
    lv_obj_set_size(phandle->second_img_obj, phandle->area_w, phandle->area_h);
    lv_img_set_src(phandle->second_img_obj, &phandle->second_img_buf);
    lv_obj_add_flag(phandle->second_img_obj, LV_OBJ_FLAG_HIDDEN);

	lv_obj_clear_flag(phandle->main_obj, LV_OBJ_FLAG_SCROLLABLE);
    if(album_id >= DIVOOM_CUSTOM_START_CLOCK_ID)
    {
        int last_pic_id;
        
        divoom_pic_album_manage_get_last_play_album_pic_id(phandle->album_id, &phandle->play_one_flag);
        phandle->pget_pic_handle = divoom_pic_album_manage_create_disp_order_handle(album_id, phandle->pslide_config->disp_order, phandle->pslide_config->disp_reverse, play_movie);
        last_pic_id = divoom_pic_album_manage_get_last_play_album_pic_id(album_id, NULL);
        if(last_pic_id > 0)
        {
            if(divoom_pic_album_manage_get_pic_info_from_pic_id(phandle->pget_pic_handle, last_pic_id, &phandle->disp_index, NULL))
            {
                phandle->disp_index ++;
            }
        }
        
        phandle->photo_num = divoom_pic_album_manage_get_disp_order_pic_num(phandle->pget_pic_handle);
    }
    else if(divoom_light_app_check_cloud_photo_clock(album_id))
    {
        phandle->pget_pic_handle = divoom_cloud_photo_ctrl_open_handle();
        phandle->photo_num = divoom_cloud_photo_ctrl_get_total();
    }
    else
    {
        divoom_clock_id_info clock_info;

        clock_info.clock_id = album_id;
        clock_info.clock_pos = clock_pos;
        clock_info.parent_id = parent_id;
        phandle->pget_pic_handle = divoom_custom_photo_open(&clock_info, phandle->pslide_config->disp_order, phandle->pslide_config->disp_reverse, NULL);
        phandle->photo_num = divoom_custom_photo_get_total_items(phandle->pget_pic_handle);
        phandle->play_one_flag = divoom_custom_photo_get_play_one_flag(phandle->pget_pic_handle);
    }
    phandle->slide_mode = phandle->pslide_config->effect_type;
    phandle->play_speed = play_speed;
    phandle->start_time = divoom_system_get_tickms();
    divoom_photo_play_ctrl_load_pic_data(phandle, &phandle->first_img_buf, phandle->first_img_obj, 0);
    phandle->disp_over_flag = 1;
    if(album_id < DIVOOM_CUSTOM_START_CLOCK_ID && phandle->photo_num == 0)
    {
        //lv_obj_add_flag(phandle->main_obj, LV_OBJ_FLAG_HIDDEN);
    }
    if(divoom_photo_play_ctrl_is_video(phandle))
    {
        divoom_photo_play_ctrl_start_video(phandle, 1);
    }

    divoom_system_create_task(divoom_photo_play_ctrl_task_main, "photo_play", phandle, DIVOOM_TASK_STACK_PHOTO_PLAY);
    phandle->time_handle = lv_timer_create(divoom_photo_play_ctrl_time_handle, 1, phandle);
    
    return phandle;
}



int divoom_photo_play_ctrl_get_disp_index(void *handle)
{
    Pdivoom_photo_play_ctrl_ctrl phandle = (Pdivoom_photo_play_ctrl_ctrl)handle;

    if(handle == NULL)
    {
        return 0;
    }
    return phandle->disp_index;
}

void divoom_photo_play_ctrl_update_all_info(void *handle, int album_id, int slide_mode, int play_speed, int parent_id, int clock_pos)
{
    Pdivoom_photo_play_ctrl_ctrl phandle = (Pdivoom_photo_play_ctrl_ctrl)handle;

    play_speed = phandle->pslide_config->play_time;
    
    phandle->slide_mode = phandle->pslide_config->effect_type;
    if(play_speed < 3)
    {
        play_speed = 3;
    }
    phandle->play_speed = play_speed;
    if(phandle->album_id != album_id)
    {
        void *palbum_handle;
        Pdivoom_pic_album_manage_list_item pdisp_info;
        
        
        divoom_system_get_lock(phandle->lock_handle);
        phandle->media_load_pending = 0;
        if(phandle->play_movie_handle)
        {
            
            divoom_photo_play_ctrl_stop_video(phandle);
            divoom_photo_play_ctrl_ctrl_image_buf(phandle, 1);
        }    
        
        if(phandle->play_gif_handle)
        {
            divoom_lvgl_play_gif_file_destory_handle(phandle->play_gif_handle, 0);
            phandle->play_gif_handle = NULL;
        }
        if(phandle->album_id >= DIVOOM_CUSTOM_START_CLOCK_ID)
        {
            divoom_pic_album_manage_destory_disp_order_handle(phandle->pget_pic_handle);
        }
        else if(divoom_light_app_check_cloud_photo_clock(phandle->album_id))
        {
            divoom_cloud_photo_ctrl_close_handle();
            phandle->pget_pic_handle = NULL;
        }
        else
        {
            divoom_custom_photo_close(phandle->pget_pic_handle);
        }
        phandle->cur_pic_id = 0;
        if(album_id >= DIVOOM_CUSTOM_START_CLOCK_ID)
        {
            phandle->pget_pic_handle = divoom_pic_album_manage_create_disp_order_handle(album_id, phandle->pslide_config->disp_order, phandle->pslide_config->disp_reverse, phandle->play_movie_flag);
            
            phandle->photo_num = divoom_pic_album_manage_get_disp_order_pic_num(phandle->pget_pic_handle);
            
        }
        else if(divoom_light_app_check_cloud_photo_clock(album_id))
        {
            phandle->pget_pic_handle = divoom_cloud_photo_ctrl_open_handle();
            phandle->photo_num = divoom_cloud_photo_ctrl_get_total();
        }
        else
        {
            divoom_clock_id_info clock_info;

            clock_info.clock_id = album_id;
            clock_info.clock_pos = clock_pos;
            clock_info.parent_id = parent_id;
            phandle->pget_pic_handle = divoom_custom_photo_open(&clock_info, phandle->pslide_config->disp_order, phandle->pslide_config->disp_reverse, &phandle->disp_index);
            phandle->photo_num = divoom_custom_photo_get_total_items(phandle->pget_pic_handle);
        }
        
        phandle->disp_index = 0;
        phandle->start_time = 0;
        phandle->default_file_shown = 0;
        phandle->default_confirm_time = 0;
        
        divoom_system_free_lock(phandle->lock_handle);
        phandle->album_id = album_id;
    }
        
}




void divoom_photo_play_ctrl_pause(void *handle)
{
    Pdivoom_photo_play_ctrl_ctrl phandle = (Pdivoom_photo_play_ctrl_ctrl)handle;
    
    if(handle == NULL)
    {
        return;
    }
    if(phandle->exit_flag == 1)
    {
        return ;
    }
    
    lv_anim_del(phandle, NULL);
    if(phandle->time_handle)
    {
        lv_timer_del(phandle->time_handle);
        phandle->time_handle = NULL;
    }
    
    phandle->exit_flag = 1;
    
    divoom_system_get_sem(phandle->exit_sem);
    divoom_system_destroy_sem(phandle->exit_sem);
    
    
    divoom_photo_play_ctrl_stop_video(phandle);
    
    if(phandle->play_gif_handle)
    {
        divoom_lvgl_play_gif_file_destory_handle(phandle->play_gif_handle, 0);
        phandle->play_gif_handle = NULL;
    }
    pdivoom_photo_play_ctrl_handle = NULL;
}

void divoom_photo_play_ctrl_exit(void *handle)
{
    Pdivoom_photo_play_ctrl_ctrl phandle = (Pdivoom_photo_play_ctrl_ctrl)handle;

    if(handle == NULL)
    {
        return;
    }

    /* Also safe for callers that did not perform the separate pause phase. */
    if(phandle->exit_flag == 0)
        divoom_photo_play_ctrl_pause(handle);
    pdivoom_photo_play_ctrl_handle = NULL;

    if(phandle->time_handle)
    {
        lv_timer_del(phandle->time_handle);
    }
    divoom_photo_play_ctrl_stop_video(phandle);
    
    if(phandle->play_gif_handle)
    {
        
        divoom_lvgl_play_gif_file_destory_handle(phandle->play_gif_handle, 0);
        
        phandle->play_gif_handle = NULL;
    }
    if(phandle->album_id >= DIVOOM_CUSTOM_START_CLOCK_ID)
    {
        divoom_pic_album_manage_destory_disp_order_handle(phandle->pget_pic_handle);
        if(phandle->cur_pic_id > 0)
        {
            divoom_pic_album_manage_update_last_play_album_pic_id(phandle->album_id, phandle->cur_pic_id);
        }
    }
    else if(divoom_light_app_check_cloud_photo_clock(phandle->album_id))
    {
        divoom_cloud_photo_ctrl_close_handle();
        phandle->pget_pic_handle = NULL;
    }
    else
    {
        divoom_custom_photo_close(phandle->pget_pic_handle);
        if(phandle->cur_pic_id > 0)
        {
            divoom_clock_id_info clock_info;

            clock_info.clock_id = phandle->album_id;
            clock_info.clock_pos = phandle->clock_pos;
            clock_info.parent_id = phandle->parent_id;
            divoom_custom_photo_update_pos_info(&clock_info, phandle->cur_pic_id);
        }
    }
    if(phandle->first_img_buf.data)
    {
        divoom_system_free((uint8 *)phandle->first_img_buf.data);
    }

    if(phandle->second_img_buf.data)
    {
        divoom_system_free((uint8 *)phandle->second_img_buf.data);
    }
    
    divoom_system_destroy_lock(phandle->lock_handle);
    if(phandle->prandom_handle)
    {
        divoom_random_destroy(phandle->prandom_handle);
    }
    if(phandle->pmix_random_handle)
    {
        divoom_random_destroy(phandle->pmix_random_handle);
    }
    divoom_system_free(phandle);
    
}
