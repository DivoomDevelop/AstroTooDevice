
//渐隐效果
#include <malloc.h>    
#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"
#include "divoom_lvgl_photo_view.h"
#include "divoom_lvgl_photo_slide_effect.h"
#include "divoom_lvgl_play_gif_file.h"
#include "divoom_pixel_format.h"
#ifdef DIVOOM_SIMULATOR
#include "sim_pal.h"
#endif

#define DIVOOM_LVGL_PHOTO_PUSH_DISP_CHANGE_TIME   DIVOOM_PHOTO_SLIDE_EFFECT_TIME

#define DIVOOM_LVGL_PUSH_MAC_LARGE_PARA 110
#define DIVOOM_LVGL_PHOTO_PUSH_MOSAIC_TILE  10
/* 对齐模拟器 WatchFace.albumEmpty：左右各 24px padding */
#define DIVOOM_LVGL_PHOTO_PUSH_EMPTY_HINT_W  (DIVOOM_SCREEN_WIDTH - 48)

typedef struct _divoom_lvgl_photo_push_item
{
    divoom_pic_phone_manage_item_offset  raw_info;
    Pdivoom_pic_phone_manage_item_offset  image_info;
    void *img_obj;
    lv_img_dsc_t lv_image_info; 
    lv_img_dsc_t *image_data;    
    void *video_handle;
    void *play_gif_handle;
}divoom_lvgl_photo_push_item, *Pdivoom_lvgl_photo_push_item;


typedef struct _divoom_lvgl_photo_push_ctrl
{    
    uint8 mix_index;
    uint8 mix_slide_mode;
    uint8 push_index;
    uint8 mosaic_index;
    int album_id;
    int menu_id;
    volatile int run_flag;
    volatile int read_task_alive;
    int run_tick;
    int dir_flag;               //幻灯片播放方向
    int play_one_flag;               //幻灯片播放方向
    int end_flag;
    int pre_index;
    int update_display_flag;    //0：无状态，1：正在更新数据；2：数据更新完，将更新显示
    int video_handoff_flag;
    lv_timer_t *refresh_timer;  //照片读完刷新显示的定时器
    lv_timer_t *read_timer;     //切换读照片的定时器
#ifdef DIVOOM_SIMULATOR
    lv_timer_t *sim_keepalive_timer; /* 静态 webp 无刷新线程，sim direct_mode 下须周期性重绘 */
#endif
    void *main_obj; 
    lv_style_t main_style;            
    void *pread_sem;            //读照片线程的信号量
    void *pexit_sem;            //读照片线程的信号量
    void *panim_handle_obj[2];
    lv_anim_t image_anim[2];    //照片切换动画
    int use_pos;                //照片列表的使用位置
    int pic_num;
    divoom_lvgl_photo_push_item *pcur_item;
    divoom_lvgl_photo_push_item *pload_item;
    void *prandom_handle;
    void *pmix_random_handle;
    void *photo_title_obj;
    char  cur_photo_title[1024];
    void *pget_pic_handle;
    Pdivoom_pic_phone_manage_slide_config pslide_config;
}divoom_lvgl_photo_push_ctrl, *Pdivoom_lvgl_photo_push_ctrl;

void divoom_lvgl_photo_push_exit(void *handle);
static void divoom_lvgl_photo_push_show_empty_album_label(Pdivoom_lvgl_photo_push_ctrl phandle);
static int divoom_lvgl_photo_push_show_video_last_frame(Pdivoom_lvgl_photo_push_ctrl phandle,
    Pdivoom_lvgl_photo_push_item old);

static void divoom_lvgl_photo_push_free_image_item(Pdivoom_lvgl_photo_push_item pitem)
{
    if(pitem == NULL)
    {
        return;
    }
    if(pitem->image_data && pitem->image_data->data)
    {
        divoom_system_free((uint8 *)pitem->image_data->data);
        pitem->image_data->data = NULL;
    }
    divoom_system_free(pitem);
}

static void divoom_lvgl_photo_push_enter_empty_state(Pdivoom_lvgl_photo_push_ctrl phandle)
{
    if(phandle == NULL)
    {
        return;
    }

    divoom_lvgl_photo_push_free_image_item(phandle->pcur_item);
    divoom_lvgl_photo_push_free_image_item(phandle->pload_item);
    phandle->pcur_item = NULL;
    phandle->pload_item = NULL;
    phandle->run_flag = 0;
    phandle->read_task_alive = 0;
    phandle->pic_num = 0;
    divoom_lvgl_photo_push_show_empty_album_label(phandle);
}


//这个是切换照片时调用
void divoom_lvgl_photo_push_read_image_file(Pdivoom_lvgl_photo_push_ctrl phandle)
{
    int total = divoom_pic_album_manage_get_disp_order_pic_num(phandle->pget_pic_handle);
    if(total <= 0 || phandle->update_display_flag != 0)
    {
        return;
    }
    phandle->update_display_flag = 1;
    divoom_pic_album_manage_get_disp_order_pic_id(phandle->pget_pic_handle, phandle->use_pos, phandle->pload_item->image_info);
    phandle->use_pos = (phandle->use_pos + 1) % total;
    phandle->pic_num = total;   
#ifndef DIVOOM_SIMULATOR
    if(phandle->pcur_item->video_handle && !phandle->pload_item->image_info->pic_video_flag)
    {
        /* Decode photos only after VDEC has returned its frame buffers. */
        if(!divoom_lvgl_photo_push_show_video_last_frame(phandle, phandle->pcur_item))
        {
            divoom_menu_control_change_disp_mode(phandle->main_obj, 0);
        }
        divoom_local_movie_play_stop_movie_no_cb();
        if(divoom_local_movie_play_wait_stopped(2500) != 0)
        {
            divoom_print("photo wait for video release failed\n");
            phandle->update_display_flag = 0;
            phandle->end_flag = 1;
            return;
        }
        phandle->pcur_item->video_handle = NULL;
        phandle->video_handoff_flag = 0;
        divoom_print("photo video released before image decode\n");
    }
#endif
    divoom_system_put_sem(phandle->pread_sem);
}


//这个是切换照片时调用
void divoom_lvgl_photo_push_read_time_handle(lv_timer_t *timer)
{
    int pic_num;
    Pdivoom_lvgl_photo_push_ctrl phandle = (Pdivoom_lvgl_photo_push_ctrl)lv_timer_get_user_data(timer);

    if(phandle->pic_num == 0 || phandle->pcur_item == NULL || phandle->pcur_item->image_info == NULL)
    {
        return;
    }
    if(phandle->play_one_flag)
    {
        return;
    }
    pic_num = divoom_pic_album_manage_get_disp_order_pic_num(phandle->pget_pic_handle);

    /* 单视频由播放器原地循环；结束/失败后仍允许重新加载。 */
    if(phandle->pic_num == pic_num && pic_num == 1 &&
        divoom_pic_album_manage_get_disp_last_pic_id(phandle->pget_pic_handle) <= 0 &&
        (phandle->pcur_item->image_info->pic_video_flag == 0 ||
            (phandle->pcur_item->video_handle && phandle->end_flag == 0)))
    {
        return;
    }
    if(phandle->pcur_item && phandle->pcur_item->image_info && phandle->pslide_config)
    {
        phandle->run_tick ++;
        //视频只要播放结束就可以启动，照片播放固定的时间
        if((phandle->pcur_item->image_info->pic_video_flag && phandle->end_flag) || 
            (phandle->pcur_item->image_info->pic_video_flag == 0 && phandle->run_tick >= phandle->pslide_config->play_time / 100) ||
            divoom_pic_album_manage_get_disp_last_pic_id(phandle->pget_pic_handle) > 0)
        {
            phandle->run_tick = 0;
            phandle->end_flag = 0;         
            divoom_lvgl_photo_push_read_image_file(phandle);
        }
    }

}

//向上边移出
void divoom_lvgl_photo_push_up_out_anim_cb(void * a, int32_t v)
{
    lv_obj_set_y(a, 0 - v*DIVOOM_SCREEN_HIGH/100);
}

//向上边移入
void divoom_lvgl_photo_push_up_in_anim_cb(void * a, int32_t v)
{
    lv_obj_set_y(a, DIVOOM_SCREEN_HIGH  - v*DIVOOM_SCREEN_HIGH/100);
}

//向下边移出
void divoom_lvgl_photo_push_down_out_anim_cb(void * a, int32_t v)
{
    lv_obj_set_y(a, v*DIVOOM_SCREEN_HIGH/100);
}

//向下边移入
void divoom_lvgl_photo_push_down_in_anim_cb(void * a, int32_t v)
{
    lv_obj_set_y(a, v*DIVOOM_SCREEN_HIGH/100 - DIVOOM_SCREEN_HIGH);
}

//右边移出
void divoom_lvgl_photo_push_right_out_anim_cb(void * a, int32_t v)
{
    lv_obj_set_x(a, 0 - v*DIVOOM_SCREEN_WIDTH/100);
}

//从右边移入
void divoom_lvgl_photo_push_right_in_anim_cb(void * a, int32_t v)
{
    lv_obj_set_x(a, DIVOOM_SCREEN_WIDTH - v*DIVOOM_SCREEN_WIDTH/100);
}

//向右边移出
void divoom_lvgl_photo_push_left_out_pre_anim_cb(void * a, int32_t v)
{
    lv_obj_set_x(a, v*DIVOOM_SCREEN_WIDTH/100);
    //lv_obj_set_style_img_opa(a, (100 - v)*255/100, 0);
}

//从左边出来
void divoom_lvgl_photo_push_left_in_pre_anim_cb(void * a, int32_t v)
{
    lv_obj_set_x(a, v*DIVOOM_SCREEN_WIDTH/100 - DIVOOM_SCREEN_WIDTH);
    //lv_obj_set_style_img_opa(a, (30 + 70*v/100)*255/100, 0);
}

static void divoom_lvgl_photo_push_anim_ready_cb(lv_anim_t *a)
{
    Pdivoom_lvgl_photo_push_ctrl phandle = (Pdivoom_lvgl_photo_push_ctrl)lv_anim_get_user_data(a);

    if(phandle == NULL)
    {
        return;
    }
    if(phandle->pcur_item && phandle->pcur_item->img_obj)
    {
        lv_obj_clear_flag(phandle->pcur_item->img_obj, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_style_img_opa(phandle->pcur_item->img_obj, LV_OPA_COVER, 0);
        lv_obj_invalidate(phandle->pcur_item->img_obj);
    }
    /* 过渡结束后隐藏旧图层，避免 sim 下离屏/半透明图层覆盖当前照片 */
    if(phandle->pload_item && phandle->pload_item->img_obj)
    {
        lv_obj_add_flag(phandle->pload_item->img_obj, LV_OBJ_FLAG_HIDDEN);
        lv_obj_invalidate(phandle->pload_item->img_obj);
    }
    if(phandle->main_obj)
    {
        lv_obj_invalidate(phandle->main_obj);
    }
#ifdef DIVOOM_SIMULATOR
    sim_movie_force_lvgl_redraw_after_video();
#endif
}

static void divoom_lvgl_photo_push_bind_anim_ready(Pdivoom_lvgl_photo_push_ctrl phandle, lv_anim_t *anim)
{
    lv_anim_set_ready_cb(anim, divoom_lvgl_photo_push_anim_ready_cb);
    lv_anim_set_user_data(anim, phandle);
}

static void divoom_lvgl_photo_push_refresh_photo_layer(Pdivoom_lvgl_photo_push_ctrl phandle)
{
    if(phandle == NULL)
    {
        return;
    }
    if(phandle->pcur_item && phandle->pcur_item->img_obj)
    {
        lv_obj_clear_flag(phandle->pcur_item->img_obj, LV_OBJ_FLAG_HIDDEN);
        lv_obj_invalidate(phandle->pcur_item->img_obj);
    }
    if(phandle->pload_item && phandle->pload_item->img_obj)
    {
        lv_obj_invalidate(phandle->pload_item->img_obj);
    }
    if(phandle->main_obj)
    {
        lv_obj_invalidate(phandle->main_obj);
    }
#ifdef DIVOOM_SIMULATOR
    sim_movie_force_lvgl_redraw_after_video();
#endif
}

#ifdef DIVOOM_SIMULATOR
static void divoom_lvgl_photo_push_sim_keepalive_cb(lv_timer_t *timer)
{
    Pdivoom_lvgl_photo_push_ctrl phandle = (Pdivoom_lvgl_photo_push_ctrl)lv_timer_get_user_data(timer);

    if(phandle == NULL || phandle->pic_num == 0)
    {
        return;
    }
    if(phandle->update_display_flag == 2)
    {
        return;
    }
    if(phandle->pcur_item == NULL || phandle->pcur_item->image_info == NULL)
    {
        return;
    }
    if(phandle->pcur_item->image_info->pic_video_flag)
    {
        return;
    }
    divoom_lvgl_photo_push_refresh_photo_layer(phandle);
}
#endif

int divoom_lvgl_photo_push_play_over_callback(void *user_data, int status)
{
    Pdivoom_lvgl_photo_push_ctrl phandle = (Pdivoom_lvgl_photo_push_ctrl)user_data;

    if(status == 0 && (phandle->play_one_flag ||
        (divoom_pic_album_manage_get_disp_order_pic_num(phandle->pget_pic_handle) == 1 &&
            divoom_pic_album_manage_get_disp_last_pic_id(phandle->pget_pic_handle) <= 0)))
    {
        divoom_print("replay!\n");
        return 0;
    }
#ifndef DIVOOM_SIMULATOR
    if(status == 0)
    {
        phandle->end_flag = 1;
        return DIVOOM_LOCAL_MOVIE_HOLD_LAST_FRAME;
    }
#endif
    if(status == 1)
    {
        divoom_print("play over!\n");
        phandle->end_flag = 1;
    }

    return  1;
}

//从左边出来
void divoom_lvgl_photo_push_large_anim_anim_cb(void * a, int32_t v)
{
    lv_img_set_zoom(a, v);
    lv_obj_set_style_img_opa(a, LV_OPA_100 - (v - LV_IMG_ZOOM_NONE), 0);
    //lv_obj_set_style_img_opa(a, (30 + 70*v/100)*255/100, 0);
}

void divoom_lvgl_photo_push_large_anim(Pdivoom_lvgl_photo_push_ctrl phandle)
{
    Pdivoom_lvgl_photo_push_item pitem = phandle->pload_item;
    
    //lv_obj_set_style_img_opa(pitem->img_obj, 30, 0);
    lv_obj_set_style_img_opa(pitem->img_obj, LV_OPA_100, 0);
    lv_obj_set_pos(pitem->img_obj, 0, 0);
    if(phandle->pload_item->image_info->pic_video_flag || phandle->pcur_item->image_info->pic_video_flag)
    {
        lv_img_set_zoom(pitem->img_obj, LV_IMG_ZOOM_NONE);
    }
    else
    {
        lv_img_set_zoom(pitem->img_obj, LV_IMG_ZOOM_NONE * DIVOOM_LVGL_PUSH_MAC_LARGE_PARA/100);
        lv_anim_set_var(&phandle->image_anim[0], pitem->img_obj);
        phandle->panim_handle_obj[0] = pitem->img_obj;
        lv_anim_path_linear(&phandle->image_anim[0]);
        lv_anim_set_path_cb(&phandle->image_anim[0], lv_anim_path_ease_out);
        lv_anim_set_exec_cb(&phandle->image_anim[0], (lv_anim_exec_xcb_t) lv_img_set_zoom);
        lv_anim_set_time(&phandle->image_anim[0], 800);
        lv_anim_set_values(&phandle->image_anim[0],  LV_IMG_ZOOM_NONE *DIVOOM_LVGL_PUSH_MAC_LARGE_PARA/100, LV_IMG_ZOOM_NONE);
        divoom_lvgl_photo_push_bind_anim_ready(phandle, &phandle->image_anim[0]);
        lv_anim_start(&phandle->image_anim[0]);    
    }
    pitem = phandle->pcur_item;
    lv_obj_set_style_img_opa(pitem->img_obj, LV_OPA_100, 0);
    lv_obj_set_pos(pitem->img_obj, DIVOOM_SCREEN_WIDTH, 0);
    if(pitem)
    {
        if(pitem->play_gif_handle)
        {
            divoom_lvgl_play_gif_file_destory_handle(pitem->play_gif_handle, 0);
            pitem->play_gif_handle = NULL;
        }
    }
}

void divoom_lvgl_photo_push_push_anim(Pdivoom_lvgl_photo_push_ctrl phandle, int push_index)
{
    Pdivoom_lvgl_photo_push_item pitem = phandle->pload_item;

    if(push_index >= 0)
    {
        phandle->push_index = push_index;
    }
    else
    {
        phandle->push_index = divoom_app_com_get_rand_index(phandle->push_index, 4);
    }
    lv_obj_set_style_img_opa(pitem->img_obj, LV_OPA_100, 0);
    
    if(phandle->pload_item->image_info->pic_video_flag || phandle->pcur_item->image_info->pic_video_flag)
    {
        lv_obj_set_pos(pitem->img_obj, 0, 0);
    }
    else
    {
        //0:向左，１：向右；２：向上；３：向下
        lv_anim_set_var(&phandle->image_anim[0], pitem->img_obj);
        phandle->panim_handle_obj[0] = pitem->img_obj;
        lv_anim_path_linear(&phandle->image_anim[0]);
        lv_anim_set_path_cb(&phandle->image_anim[0], lv_anim_path_linear);
        if(phandle->push_index == 0)
        {
            lv_obj_set_pos(pitem->img_obj, 0 - DIVOOM_SCREEN_WIDTH, 0);
            lv_anim_set_exec_cb(&phandle->image_anim[0], (lv_anim_exec_xcb_t) divoom_lvgl_photo_push_left_in_pre_anim_cb);
        }
        else if(phandle->push_index == 1)
        {
            lv_obj_set_pos(pitem->img_obj, DIVOOM_SCREEN_WIDTH, 0);
            lv_anim_set_exec_cb(&phandle->image_anim[0], (lv_anim_exec_xcb_t) divoom_lvgl_photo_push_right_in_anim_cb);
        }
        else if(phandle->push_index == 2)
        {
            lv_obj_set_pos(pitem->img_obj, 0, 0 - DIVOOM_SCREEN_HIGH);
            lv_anim_set_exec_cb(&phandle->image_anim[0], (lv_anim_exec_xcb_t) divoom_lvgl_photo_push_up_in_anim_cb);
        }
        else
        {
            lv_obj_set_pos(pitem->img_obj, 0, DIVOOM_SCREEN_HIGH);
            lv_anim_set_exec_cb(&phandle->image_anim[0], (lv_anim_exec_xcb_t) divoom_lvgl_photo_push_down_in_anim_cb);
        }
        lv_anim_set_time(&phandle->image_anim[0], 1000);
        lv_anim_set_values(&phandle->image_anim[0], 0, 100);
        divoom_lvgl_photo_push_bind_anim_ready(phandle, &phandle->image_anim[0]);
        lv_anim_start(&phandle->image_anim[0]);    
    }
    pitem = phandle->pcur_item;
    lv_obj_set_style_img_opa(pitem->img_obj, LV_OPA_100, 0);
    lv_obj_set_pos(pitem->img_obj, 0, 0);
    if(pitem)
    {
        if(pitem->play_gif_handle)
        {
            divoom_lvgl_play_gif_file_destory_handle(pitem->play_gif_handle, 0);
            pitem->play_gif_handle = NULL;
        }
        if(phandle->pload_item->image_info->pic_video_flag || phandle->pcur_item->image_info->pic_video_flag)
        {
            lv_obj_set_pos(pitem->img_obj, DIVOOM_SCREEN_WIDTH, 0);
        }
        else
        {
            phandle->panim_handle_obj[1] = pitem->img_obj;
            lv_anim_set_var(&phandle->image_anim[1], pitem->img_obj);
            lv_anim_path_linear(&phandle->image_anim[1]);
            lv_anim_set_path_cb(&phandle->image_anim[1], lv_anim_path_linear);
            if(phandle->push_index == 0)
            {
                lv_anim_set_exec_cb(&phandle->image_anim[1], (lv_anim_exec_xcb_t) divoom_lvgl_photo_push_left_out_pre_anim_cb);
            }
            else if(phandle->push_index == 1)
            {
                lv_anim_set_exec_cb(&phandle->image_anim[1], (lv_anim_exec_xcb_t) divoom_lvgl_photo_push_right_out_anim_cb);
            }
            else if(phandle->push_index == 2)
            {
                lv_anim_set_exec_cb(&phandle->image_anim[1], (lv_anim_exec_xcb_t) divoom_lvgl_photo_push_up_out_anim_cb);
            }
            else
            {
                lv_anim_set_exec_cb(&phandle->image_anim[1], (lv_anim_exec_xcb_t) divoom_lvgl_photo_push_down_out_anim_cb);
            }
            lv_anim_set_time(&phandle->image_anim[1], 1000);
            lv_anim_set_values(&phandle->image_anim[1], 0, 100);
            divoom_lvgl_photo_push_bind_anim_ready(phandle, &phandle->image_anim[1]);
            lv_anim_start(&phandle->image_anim[1]);   
        }
    }
}


void divoom_lvgl_photo_push_fade_out_anim_cb(void * a, int32_t v)
{
    lv_obj_set_style_img_opa(a, (100-v)*LV_OPA_100/100, 0);
}

void divoom_lvgl_photo_push_fade_in_anim_cb(void * a, int32_t v)
{
    lv_obj_set_style_img_opa(a, v*LV_OPA_100/100, 0);
}

void divoom_lvgl_photo_push_fade_out_anim(Pdivoom_lvgl_photo_push_ctrl phandle)
{
    Pdivoom_lvgl_photo_push_item pitem = phandle->pload_item;
    
    lv_obj_center(pitem->img_obj);
    

    lv_obj_set_style_img_opa(pitem->img_obj, LV_OPA_100, 0);
    lv_obj_set_pos(pitem->img_obj, 0, 0);
    if(phandle->pload_item->image_info->pic_video_flag || phandle->pcur_item->image_info->pic_video_flag)
    {
        lv_obj_set_style_img_opa(pitem->img_obj, LV_OPA_100, 0);
    }
    else
    {
        lv_obj_set_style_img_opa(pitem->img_obj, 25, 0);
        phandle->panim_handle_obj[0] = pitem->img_obj;
        lv_anim_set_var(&phandle->image_anim[0], pitem->img_obj);
        lv_anim_path_linear(&phandle->image_anim[0]);
        lv_anim_set_path_cb(&phandle->image_anim[0], lv_anim_path_linear);
        lv_anim_set_exec_cb(&phandle->image_anim[0], (lv_anim_exec_xcb_t) divoom_lvgl_photo_push_fade_in_anim_cb);
        lv_anim_set_time(&phandle->image_anim[0], 1000);
        lv_anim_set_values(&phandle->image_anim[0], 0, 100);
        lv_anim_start(&phandle->image_anim[0]);   
    }

    pitem = phandle->pcur_item;

    lv_obj_set_style_img_opa(pitem->img_obj, LV_OPA_100, 0);
    lv_obj_set_pos(pitem->img_obj, 0, 0);
    if(pitem)
    {
        if(pitem->play_gif_handle)
        {
            divoom_lvgl_play_gif_file_destory_handle(pitem->play_gif_handle, 0);
            pitem->play_gif_handle = NULL;
        }
        if(phandle->pload_item->image_info->pic_video_flag || phandle->pcur_item->image_info->pic_video_flag)
        {
            lv_obj_set_style_img_opa(pitem->img_obj, 0, 0);
        }
        else
        {
            phandle->panim_handle_obj[1] = pitem->img_obj;
            lv_anim_set_var(&phandle->image_anim[1], pitem->img_obj);
            lv_anim_path_linear(&phandle->image_anim[1]);
            lv_anim_set_path_cb(&phandle->image_anim[1], lv_anim_path_linear);
            lv_anim_set_exec_cb(&phandle->image_anim[1], (lv_anim_exec_xcb_t) divoom_lvgl_photo_push_fade_out_anim_cb);
            lv_anim_set_time(&phandle->image_anim[1], 1000);
            lv_anim_set_values(&phandle->image_anim[1], 0, 100);
            divoom_lvgl_photo_push_bind_anim_ready(phandle, &phandle->image_anim[1]);
            lv_anim_start(&phandle->image_anim[1]);   
        }
    }
}

void divoom_lvgl_photo_push_rotate_anim_cb(void * a, int32_t v)
{
    //lv_obj_set_style_img_opa(a, (100-v)*LV_OPA_100/100, 0);
}

void divoom_lvgl_photo_push_rotate_fade_in_anim_cb(void * a, int32_t v)
{
    if(v>LV_OPA_100)
    {
        lv_obj_set_style_img_opa(a, LV_OPA_100, 0);
    }
    else
    {
        lv_obj_set_style_img_opa(a, v, 0);
    }
    lv_img_set_zoom(a, v);
    //lv_img_set_angle(a, v*3600/100);
}

void divoom_lvgl_photo_push_rotate_anim(Pdivoom_lvgl_photo_push_ctrl phandle)
{
    Pdivoom_lvgl_photo_push_item pitem = phandle->pload_item;
    
    pitem->image_data = &pitem->lv_image_info;
    lv_obj_set_size(pitem->img_obj, pitem->image_data->header.w, pitem->image_data->header.h);
    lv_img_set_src(pitem->img_obj, pitem->image_data);
    lv_obj_center(pitem->img_obj);

    
    lv_obj_set_style_img_opa(pitem->img_obj, LV_OPA_100, 0);
    lv_obj_set_pos(pitem->img_obj, 0, 0);
    if(phandle->pload_item->image_info->pic_video_flag || phandle->pcur_item->image_info->pic_video_flag)
    {
        lv_obj_set_style_img_opa(pitem->img_obj, LV_OPA_100, 0);
        lv_img_set_angle(pitem->img_obj, 0);
        lv_img_set_zoom(pitem->img_obj, LV_IMG_ZOOM_NONE);
    }
    else
    {
        lv_obj_set_style_img_opa(pitem->img_obj, 25, 0);
        lv_img_set_angle(pitem->img_obj, 0);
        lv_img_set_zoom(pitem->img_obj, 1);
        phandle->panim_handle_obj[0] = pitem->img_obj;
        lv_anim_set_var(&phandle->image_anim[0], pitem->img_obj);
        lv_anim_path_linear(&phandle->image_anim[0]);
        lv_anim_set_path_cb(&phandle->image_anim[0], lv_anim_path_linear);
        lv_anim_set_exec_cb(&phandle->image_anim[0], (lv_anim_exec_xcb_t) divoom_lvgl_photo_push_rotate_fade_in_anim_cb);
        lv_anim_set_time(&phandle->image_anim[0], 3000);
        lv_anim_set_values(&phandle->image_anim[0], 0, 256);
        lv_anim_start(&phandle->image_anim[0]);   
    }

    pitem = phandle->pcur_item;
    lv_obj_set_style_img_opa(pitem->img_obj, LV_OPA_100, 0);
    lv_obj_set_pos(pitem->img_obj, 0, 0);
    if(pitem)
    {
        if(pitem->play_gif_handle)
        {
            divoom_lvgl_play_gif_file_destory_handle(pitem->play_gif_handle, 0);
            pitem->play_gif_handle = NULL;
        }
        
        if(phandle->pload_item->image_info->pic_video_flag || phandle->pcur_item->image_info->pic_video_flag)
        {
            lv_obj_set_style_img_opa(pitem->img_obj, 0, 0);
        }
        else
        {
            lv_img_set_angle(pitem->img_obj, 0);
            phandle->panim_handle_obj[1] = pitem->img_obj;
            lv_anim_set_var(&phandle->image_anim[1], pitem->img_obj);
            lv_anim_path_linear(&phandle->image_anim[1]);
            lv_anim_set_path_cb(&phandle->image_anim[1], lv_anim_path_linear);
            lv_anim_set_exec_cb(&phandle->image_anim[1], (lv_anim_exec_xcb_t) divoom_lvgl_photo_push_rotate_anim_cb);
            lv_anim_set_time(&phandle->image_anim[1], 3000);
            lv_anim_set_values(&phandle->image_anim[1], 0, 100);
            divoom_lvgl_photo_push_bind_anim_ready(phandle, &phandle->image_anim[1]);
            lv_anim_start(&phandle->image_anim[1]); 
        }
    }
}



void divoom_lvgl_photo_push_blinds_anim_cb(void * a, int32_t v)
{
    int strip_size;
    Pdivoom_lvgl_photo_push_ctrl phandle = (Pdivoom_lvgl_photo_push_ctrl)a;

    strip_size = v - phandle->pre_index;
    if (strip_size <= 0)
    {
        return;
    }

    if (phandle->mosaic_index == 0)
    {
        divoom_app_com_set_image_opa((uint8 *)phandle->pcur_item->image_data->data, phandle->pre_index, 0, strip_size, DIVOOM_SCREEN_HIGH, 0xFF, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
        divoom_app_com_set_image_opa((uint8 *)phandle->pload_item->image_data->data, phandle->pre_index, 0, strip_size, DIVOOM_SCREEN_HIGH, 0, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
    }
    else if (phandle->mosaic_index == 1)
    {
        divoom_app_com_set_image_opa((uint8 *)phandle->pcur_item->image_data->data, DIVOOM_SCREEN_WIDTH - v, 0, strip_size, DIVOOM_SCREEN_HIGH, 0xFF, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
        divoom_app_com_set_image_opa((uint8 *)phandle->pload_item->image_data->data, DIVOOM_SCREEN_WIDTH - v, 0, strip_size, DIVOOM_SCREEN_HIGH, 0, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
    }
    else if (phandle->mosaic_index == 2)
    {
        divoom_app_com_set_image_opa((uint8 *)phandle->pcur_item->image_data->data, 0, phandle->pre_index, DIVOOM_SCREEN_WIDTH, strip_size, 0xFF, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
        divoom_app_com_set_image_opa((uint8 *)phandle->pload_item->image_data->data, 0, phandle->pre_index, DIVOOM_SCREEN_WIDTH, strip_size, 0, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
    }
    else
    {
        divoom_app_com_set_image_opa((uint8 *)phandle->pcur_item->image_data->data, 0, DIVOOM_SCREEN_HIGH - v, DIVOOM_SCREEN_WIDTH, strip_size, 0xFF, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
        divoom_app_com_set_image_opa((uint8 *)phandle->pload_item->image_data->data, 0, DIVOOM_SCREEN_HIGH - v, DIVOOM_SCREEN_WIDTH, strip_size, 0, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
    }
    if ((v == DIVOOM_SCREEN_WIDTH && phandle->mosaic_index < 2) || (v == DIVOOM_SCREEN_HIGH && phandle->mosaic_index >= 2))
    {
        lv_obj_add_flag(phandle->pload_item->img_obj, LV_OBJ_FLAG_HIDDEN);
    }
    lv_obj_invalidate(phandle->pcur_item->img_obj);
    lv_obj_invalidate(phandle->pload_item->img_obj);
    phandle->pre_index = v;
}

void divoom_lvgl_photo_push_blinds_anim(Pdivoom_lvgl_photo_push_ctrl phandle)
{
    Pdivoom_lvgl_photo_push_item pitem = phandle->pload_item;
    
    pitem->image_data = &pitem->lv_image_info;
    lv_obj_set_size(pitem->img_obj, pitem->image_data->header.w, pitem->image_data->header.h);
    lv_img_set_src(pitem->img_obj, pitem->image_data);
    lv_obj_center(pitem->img_obj);


    
    lv_obj_set_style_img_opa(pitem->img_obj, LV_OPA_100, 0);
    lv_obj_set_pos(pitem->img_obj, 0, 0);
    lv_img_set_angle(pitem->img_obj, 0);
    lv_img_set_zoom(pitem->img_obj, LV_IMG_ZOOM_NONE);
    
    pitem = phandle->pcur_item;
    lv_obj_set_style_img_opa(pitem->img_obj, LV_OPA_100, 0);
    lv_obj_set_pos(pitem->img_obj, 0, 0);
    if(pitem)
    {
        if(pitem->play_gif_handle)
        {
            divoom_lvgl_play_gif_file_destory_handle(pitem->play_gif_handle, 0);
            pitem->play_gif_handle = NULL;
        }
        
        if(phandle->pload_item->image_info->pic_video_flag || phandle->pcur_item->image_info->pic_video_flag)
        {
            lv_obj_set_style_img_opa(pitem->img_obj, 0, 0);
        }
        else
        {
            divoom_app_com_set_image_opa((uint8 *)phandle->pload_item->image_data->data, 0, 0, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH, 0, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
            lv_img_set_angle(pitem->img_obj, 0);
            phandle->mosaic_index = divoom_app_com_get_rand_index(phandle->mosaic_index, 4);
            phandle->pre_index = 0;
            phandle->panim_handle_obj[1] = phandle;
            lv_anim_set_var(&phandle->image_anim[1], phandle);
            lv_anim_path_linear(&phandle->image_anim[1]);
            lv_anim_set_path_cb(&phandle->image_anim[1], lv_anim_path_linear);
            lv_anim_set_exec_cb(&phandle->image_anim[1], (lv_anim_exec_xcb_t) divoom_lvgl_photo_push_blinds_anim_cb);
            lv_anim_set_time(&phandle->image_anim[1], 2000);
            if (phandle->mosaic_index < 2)
            {
                lv_anim_set_values(&phandle->image_anim[1], 0, DIVOOM_SCREEN_WIDTH);
            }
            else
            {
                lv_anim_set_values(&phandle->image_anim[1], 0, DIVOOM_SCREEN_HIGH);
            }
            divoom_lvgl_photo_push_bind_anim_ready(phandle, &phandle->image_anim[1]);
            lv_anim_start(&phandle->image_anim[1]); 
        }
    }
}

static int divoom_lvgl_photo_push_mosaic_col_num(void)
{
    return (DIVOOM_SCREEN_WIDTH + DIVOOM_LVGL_PHOTO_PUSH_MOSAIC_TILE - 1) / DIVOOM_LVGL_PHOTO_PUSH_MOSAIC_TILE;
}

static int divoom_lvgl_photo_push_mosaic_row_num(void)
{
    return (DIVOOM_SCREEN_HIGH + DIVOOM_LVGL_PHOTO_PUSH_MOSAIC_TILE - 1) / DIVOOM_LVGL_PHOTO_PUSH_MOSAIC_TILE;
}

static int divoom_lvgl_photo_push_mosaic_tile_count(void)
{
    return divoom_lvgl_photo_push_mosaic_col_num() * divoom_lvgl_photo_push_mosaic_row_num();
}

static void divoom_lvgl_photo_push_mosaic_apply_tile(Pdivoom_lvgl_photo_push_ctrl phandle, int index, int col_num)
{
    int tile_x;
    int tile_y;
    int tile_w;
    int tile_h;

    tile_x = (index % col_num) * DIVOOM_LVGL_PHOTO_PUSH_MOSAIC_TILE;
    tile_y = (index / col_num) * DIVOOM_LVGL_PHOTO_PUSH_MOSAIC_TILE;
    tile_w = DIVOOM_LVGL_PHOTO_PUSH_MOSAIC_TILE;
    tile_h = DIVOOM_LVGL_PHOTO_PUSH_MOSAIC_TILE;
    if (tile_x + tile_w > DIVOOM_SCREEN_WIDTH)
    {
        tile_w = DIVOOM_SCREEN_WIDTH - tile_x;
    }
    if (tile_y + tile_h > DIVOOM_SCREEN_HIGH)
    {
        tile_h = DIVOOM_SCREEN_HIGH - tile_y;
    }
    if (tile_w <= 0 || tile_h <= 0)
    {
        return;
    }
    divoom_app_com_set_image_opa((uint8 *)phandle->pcur_item->image_data->data, tile_x, tile_y, tile_w, tile_h,
        0xFF, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
    divoom_app_com_set_image_opa((uint8 *)phandle->pload_item->image_data->data, tile_x, tile_y, tile_w, tile_h,
        0, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
}

void divoom_lvgl_photo_push_mosaic_anim_cb(void * a, int32_t v)
{
    int index;
    int i;
    int j;
    int col_num;
    int row_num;
    int refresh_num;
    Pdivoom_lvgl_photo_push_ctrl phandle = (Pdivoom_lvgl_photo_push_ctrl)a;

    col_num = divoom_lvgl_photo_push_mosaic_col_num();
    row_num = divoom_lvgl_photo_push_mosaic_row_num();

    if (v == LV_IMG_ZOOM_NONE)
    {
        int last_num = divoom_random_get_no_use(phandle->prandom_handle);

        for (j = 0; j < last_num; j++)
        {
            index = divoom_random_get(phandle->prandom_handle);
            divoom_lvgl_photo_push_mosaic_apply_tile(phandle, index, col_num);
        }
        divoom_random_destroy(phandle->prandom_handle);
        lv_obj_add_flag(phandle->pload_item->img_obj, LV_OBJ_FLAG_HIDDEN);
        phandle->prandom_handle = NULL;
    }
    else
    {
        refresh_num = v * col_num * row_num / LV_IMG_ZOOM_NONE;
        for (i = 0; i < refresh_num - phandle->pre_index; i++)
        {
            index = divoom_random_get(phandle->prandom_handle);
            divoom_lvgl_photo_push_mosaic_apply_tile(phandle, index, col_num);
        }
        phandle->pre_index = refresh_num;
    }
    lv_obj_invalidate(phandle->pcur_item->img_obj);
    lv_obj_invalidate(phandle->pload_item->img_obj);
}

void divoom_lvgl_photo_push_mosaic_anim(Pdivoom_lvgl_photo_push_ctrl phandle)
{
    Pdivoom_lvgl_photo_push_item pitem = phandle->pload_item;
    
    pitem->image_data = &pitem->lv_image_info;
    lv_obj_set_size(pitem->img_obj, pitem->image_data->header.w, pitem->image_data->header.h);
    lv_img_set_src(pitem->img_obj, pitem->image_data);
    lv_obj_center(pitem->img_obj);


    
    lv_obj_set_style_img_opa(pitem->img_obj, LV_OPA_100, 0);
    lv_obj_set_pos(pitem->img_obj, 0, 0);
    lv_img_set_angle(pitem->img_obj, 0);
    lv_img_set_zoom(pitem->img_obj, LV_IMG_ZOOM_NONE);
    
    pitem = phandle->pcur_item;
    lv_obj_set_style_img_opa(pitem->img_obj, LV_OPA_100, 0);
    lv_obj_set_pos(pitem->img_obj, 0, 0);
    if(pitem)
    {
        if(pitem->play_gif_handle)
        {
            divoom_lvgl_play_gif_file_destory_handle(pitem->play_gif_handle, 0);
            pitem->play_gif_handle = NULL;
        }
        
        if(phandle->pload_item->image_info->pic_video_flag || phandle->pcur_item->image_info->pic_video_flag)
        {
            lv_obj_set_style_img_opa(pitem->img_obj, 0, 0);
        }
        else
        {
            if (phandle->prandom_handle)
            {
                divoom_random_destroy(phandle->prandom_handle);
            }
            phandle->prandom_handle = divoom_random_create(divoom_lvgl_photo_push_mosaic_tile_count());
            divoom_app_com_set_image_opa((uint8 *)phandle->pload_item->image_data->data, 0, 0, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH, 0, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
            lv_img_set_angle(pitem->img_obj, 0);
            phandle->pre_index = 0;
            phandle->panim_handle_obj[1] = phandle;
            lv_anim_set_var(&phandle->image_anim[1], phandle);
            lv_anim_path_linear(&phandle->image_anim[1]);
            lv_anim_set_path_cb(&phandle->image_anim[1], lv_anim_path_linear);
            lv_anim_set_exec_cb(&phandle->image_anim[1], (lv_anim_exec_xcb_t) divoom_lvgl_photo_push_mosaic_anim_cb);
            lv_anim_set_time(&phandle->image_anim[1], 2000);
            lv_anim_set_values(&phandle->image_anim[1], 0, LV_IMG_ZOOM_NONE);
            divoom_lvgl_photo_push_bind_anim_ready(phandle, &phandle->image_anim[1]);
            lv_anim_start(&phandle->image_anim[1]); 
        }
    }
}

void divoom_lvgl_photo_push_mix_anim(Pdivoom_lvgl_photo_push_ctrl phandle)
{
    int slide_mode, push_index = 0, mix_index;
    
    if(phandle->pmix_random_handle == NULL)
    {
        phandle->pmix_random_handle = divoom_random_create(4+2*2+4);
    }
    phandle->mix_index = divoom_random_get(phandle->pmix_random_handle);
    if(phandle->mix_index < 4)
    {
        slide_mode = DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_FADE_OUT;
    }
    else if(phandle->mix_index < 6)
    {
        slide_mode = DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_PUSH;
        push_index = 0;
        if(phandle->push_index == 1 && phandle->mix_slide_mode == DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_PUSH)
        {
            slide_mode = DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_FADE_OUT;
        }
    }
    else if(phandle->mix_index < 8)
    {
        slide_mode = DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_PUSH;
        push_index = 1;
        if(phandle->push_index == 0 && phandle->mix_slide_mode == DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_PUSH)
        {
            slide_mode = DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_MOSAIC;
        }
    }
    else if(phandle->mix_index == 8)
    {
        slide_mode = DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_PUSH;
        push_index = 2;
        if(phandle->push_index == 3 && phandle->mix_slide_mode == DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_PUSH)
        {
            slide_mode = DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_MOSAIC;
        }
    }
    else if(phandle->mix_index  == 9)
    {
        slide_mode = DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_PUSH;
        push_index = 3;
        if(phandle->push_index == 2 && phandle->mix_slide_mode == DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_PUSH)
        {
            slide_mode = DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_FADE_OUT;
        }
    }
    else if(phandle->mix_index  == 10)
    {
        slide_mode = DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_BLINDS;
    }
    else
    {
        slide_mode = DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_MOSAIC;
    }
    
    phandle->mix_slide_mode = slide_mode;

    switch(slide_mode)
    {
        case DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_PUSH:
            phandle->push_index = push_index;
            divoom_lvgl_photo_push_push_anim(phandle, push_index);
            break;
            
        case DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_FADE_OUT:
            divoom_lvgl_photo_push_fade_out_anim(phandle);
            break;
            
        case DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_BLINDS:
            divoom_lvgl_photo_push_blinds_anim(phandle);
            break;
            
        case DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_MOSAIC:
            divoom_lvgl_photo_push_mosaic_anim(phandle);
            break;
    }
    
    
}

static void divoom_lvgl_photo_push_prepare_transition_anim(Pdivoom_lvgl_photo_push_ctrl phandle)
{
    if (phandle == NULL)
    {
        return;
    }
    lv_anim_del(phandle, NULL);
    if (phandle->pload_item && phandle->pload_item->img_obj)
    {
        lv_anim_del(phandle->pload_item->img_obj, NULL);
    }
    if (phandle->pcur_item && phandle->pcur_item->img_obj)
    {
        lv_anim_del(phandle->pcur_item->img_obj, NULL);
    }
    lv_anim_init(&phandle->image_anim[0]);
    lv_anim_init(&phandle->image_anim[1]);
}

static int divoom_lvgl_photo_push_show_video_last_frame(Pdivoom_lvgl_photo_push_ctrl phandle,
    Pdivoom_lvgl_photo_push_item old)
{
#ifndef DIVOOM_SIMULATOR
    if(old->image_info->pic_video_flag && old->image_data &&
        divoom_local_movie_play_copy_last_frame((uint8 *)old->image_data->data,
            DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH))
    {
        lv_img_set_zoom(old->img_obj, LV_IMG_ZOOM_NONE);
        lv_img_set_angle(old->img_obj, 0);
        lv_obj_set_pos(old->img_obj, 0, 0);
        lv_obj_set_style_img_opa(old->img_obj, LV_OPA_COVER, 0);
        lv_obj_clear_flag(old->img_obj, LV_OBJ_FLAG_HIDDEN);
        lv_obj_invalidate(old->img_obj);
        divoom_menu_control_change_disp_mode(phandle->main_obj, 0);
        /* Let the UI framebuffer reach a scanout before releasing VDEC. */
        divoom_system_delay_ms(20);
        divoom_print("photo video handoff saved last frame\n");
        return 1;
    }
#endif
    return 0;
}

void divoom_lvgl_photo_push_disp_image_cout(Pdivoom_lvgl_photo_push_ctrl phandle)
{
    int i;
    lv_img_dsc_t *tmp_image_data;
    Pdivoom_lvgl_photo_push_item pitem;

    if(phandle->pcur_item->video_handle && phandle->pload_item->image_info->pic_video_flag == 0)
    {
        //如果之前播放视频，先结束，将最后一帧数据
        divoom_lvgl_movie_play_stop(phandle->pcur_item->video_handle, (uint8 *)phandle->pcur_item->image_data->data);   
        divoom_menu_control_change_disp_mode(phandle->main_obj, 0);
        phandle->pcur_item->video_handle = NULL;
        lv_obj_invalidate(phandle->pcur_item->img_obj);
    }

    divoom_lvgl_photo_push_prepare_transition_anim(phandle);

    switch(phandle->pslide_config->effect_type)
    {
        case DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_MIX:
            divoom_lvgl_photo_push_mix_anim(phandle);
            break;//混合模式
        case DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_PUSH:
            divoom_lvgl_photo_push_push_anim(phandle, -1);
            break;
            
        case DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_FADE_OUT:
            divoom_lvgl_photo_push_fade_out_anim(phandle);
            break;
        case DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_ROTATE:
            divoom_lvgl_photo_push_rotate_anim(phandle);
            break;
        case DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_BLINDS:
            divoom_lvgl_photo_push_blinds_anim(phandle);
            break;
        case DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_MOSAIC:
            divoom_lvgl_photo_push_mosaic_anim(phandle);
            break;
        case DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_NORMAL:
            break;
    }

    
    pitem = phandle->pcur_item;
    phandle->pcur_item = phandle->pload_item;
    phandle->pload_item = pitem;
    phandle->end_flag = 0;
    if(phandle->pcur_item->image_info->pic_video_flag)
    {
        char file_name[256];
        int first_flag = 0;
        
        if(phandle->pload_item->image_info->pic_video_flag == 0)
        {
            first_flag = 1;
        }
        phandle->video_handoff_flag = divoom_lvgl_photo_push_show_video_last_frame(phandle, phandle->pload_item);
        divoom_pic_phone_manage_get_picture_name(file_name, phandle->pcur_item->image_info->id);
        phandle->pcur_item->video_handle = divoom_lvgl_movie_play_play(phandle->pcur_item->img_obj, phandle,0, 0, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH, 
            file_name, divoom_pic_phone_manage_convert_video_volume(phandle->pslide_config->video_volume), 0, phandle->pslide_config->play_time, divoom_lvgl_photo_push_play_over_callback);
        if(first_flag && !phandle->video_handoff_flag)
        {
            divoom_menu_control_change_disp_mode(phandle->main_obj, 1);
        }
        lv_obj_add_flag(phandle->pcur_item->img_obj, LV_OBJ_FLAG_HIDDEN);
        if(!phandle->video_handoff_flag)
        {
            lv_obj_add_flag(phandle->pload_item->img_obj, LV_OBJ_FLAG_HIDDEN);
        }
    }
    else
    {
        lv_obj_clear_flag(phandle->pcur_item->img_obj, LV_OBJ_FLAG_HIDDEN);
        if(phandle->pslide_config->effect_type == DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_NORMAL)
        {
            lv_obj_add_flag(phandle->pload_item->img_obj, LV_OBJ_FLAG_HIDDEN);
        }
        else
        {
            lv_obj_clear_flag(phandle->pload_item->img_obj, LV_OBJ_FLAG_HIDDEN);
        }
        divoom_lvgl_play_gif_file_start_play(phandle->pcur_item->play_gif_handle);
        lv_obj_invalidate(phandle->pload_item->img_obj);
        lv_obj_invalidate(phandle->pcur_item->img_obj);
        if(phandle->pslide_config->effect_type == DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_NORMAL)
        {
            divoom_lvgl_photo_push_refresh_photo_layer(phandle);
        }
    }
}

//这个是读完数据后进行调用来更新图像
void divoom_lvgl_photo_push_refresh_image_time_handle(lv_timer_t *timer)
{
    int i;
    Pdivoom_lvgl_photo_push_ctrl phandle = (Pdivoom_lvgl_photo_push_ctrl)lv_timer_get_user_data(timer);

#ifndef DIVOOM_SIMULATOR
    if(phandle->video_handoff_flag)
    {
        if(divoom_local_movie_play_is_rendering())
        {
            divoom_menu_control_change_disp_mode(phandle->main_obj, 1);
            lv_obj_add_flag(phandle->pload_item->img_obj, LV_OBJ_FLAG_HIDDEN);
            phandle->video_handoff_flag = 0;
            divoom_print("photo video handoff first frame ready\n");
        }
        else if(phandle->end_flag)
        {
            /* Keep the saved frame visible if the new decoder failed. */
            phandle->video_handoff_flag = 0;
        }
    }
#endif
    
    if(phandle->update_display_flag != 2)
    {
        return;
    }
    phandle->update_display_flag = 0;
    divoom_lvgl_photo_push_disp_image_cout(phandle);
    phandle->dir_flag = 1;
}

static void divoom_lvgl_photo_push_event_cb(lv_event_t * e)
{
	lv_point_t p1;
	lv_event_code_t code = lv_event_get_code(e);
    Pdivoom_lvgl_photo_push_ctrl  phandle = (Pdivoom_lvgl_photo_push_ctrl)lv_event_get_user_data(e);

    
	switch(code)
	{
        case LV_EVENT_KEY:
            divoom_print("event: %d\n", code);
            divoom_menu_control_return_pre();
            break;
            
		case LV_EVENT_PRESSED:
	        lv_indev_get_point(lv_indev_get_act(), &p1);
			break;
		case LV_EVENT_PRESSING:
			lv_indev_get_point(lv_indev_get_act(), &p1);
			break;
		case LV_EVENT_RELEASED:
			break;
        default:
            break;
	}
}

int divoom_lvgl_photo_push_get_fill_flag(Pdivoom_lvgl_photo_push_ctrl phandle, Pdivoom_pic_phone_manage_item_offset pic_para)
{
    if(phandle->pslide_config->skip_fill_flag)
    {
        return phandle->pslide_config->fill_flag;
    }
    else
    {
        return pic_para->fill_flag;
    }
}

int divoom_lvgl_photo_push_load_image_buffer(Pdivoom_lvgl_photo_push_ctrl phandle, divoom_lvgl_photo_push_item *pread_item, int lock_flag)
{

    int image_w, image_h, dest_w, dest_h,fill_flag;
    char file_name[256];
    divoom_pic_phone_manage_item_offset pic_para;
    divoom_lvgl_photo_push_item *pstop_item;
    void *gif_handle;

    divoom_database_manage_get_pic_title(pread_item->image_info->id, phandle->cur_photo_title);
    divoom_pic_phone_manage_get_picture_name(file_name, pread_item->image_info->id);
    if(divoom_app_com_check_file_exist(file_name) == 0)
    {
        //如果这个照片不存在，将删除, 并修改加载计数，重新加载
        divoom_print("it fails to read: %d, %s\n", pread_item->image_info->id, file_name);
        divoom_pic_album_manage_remove_list_from_id(phandle->pslide_config->album_id, &pread_item->image_info->id, 1);
        phandle->end_flag = 1;
        phandle->run_tick = phandle->pslide_config->play_time / 100 + 100;
        return 0;
    }
    if(pread_item->image_info->pic_video_flag)
    {
       return 1;
    }
    if(lock_flag)
    {
        divoom_get_lvgl_lock();
    }
    if(phandle->pload_item == pread_item)
    {
        pstop_item = phandle->pcur_item;
    }
    else
    {
        pstop_item = phandle->pload_item;
    }
    /* Remove the shared reference before destruction waits for the GIF worker. */
    gif_handle = pstop_item->play_gif_handle;
    pstop_item->play_gif_handle = NULL;
    if(lock_flag)
    {
        divoom_free_lvgl_lock();
    }
    if(gif_handle)
    {
        divoom_lvgl_play_gif_file_destory_handle(gif_handle, lock_flag);
    }
    divoom_pic_phone_manage_get_pic_offset_info(pread_item->image_info->id, &pic_para);
    fill_flag = divoom_lvgl_photo_push_get_fill_flag(phandle, &pic_para);
    dest_w = DIVOOM_SCREEN_WIDTH;
    dest_h = DIVOOM_SCREEN_HIGH;
    divoom_print("image info: %d: %s; %d, %d, time: %d\n",pread_item->image_info->id, file_name, pic_para.mirror_flag , pic_para.rotate_flag, phandle->pslide_config->play_time);
    gif_handle = divoom_lvgl_play_gif_file_create_handle(file_name, pread_item->img_obj, pread_item->image_data, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH, lock_flag, 0);
    if(lock_flag)
    {
        divoom_get_lvgl_lock();
    }
    pread_item->play_gif_handle = gif_handle;
    if(lock_flag == 0)
    {
        divoom_lvgl_play_gif_file_start_play(gif_handle);
    }
    lv_obj_invalidate(pread_item->img_obj);
    if(lock_flag)
    {
        divoom_free_lvgl_lock();
    }
    return 1;
    #if 0
    if(divoom_pic_phone_manage_get_picture_disp_buf(file_name, (uint8 *)pread_item->image_data->data, 
        &dest_w, &dest_h, pic_para->rotate_flag, fill_flag, pic_para->mirror_flag, NULL, phandle->pslide_config->bg_type))
    {
        return 1;
    }
    else
    {
        divoom_print("it fails to read: %s\n", file_name);
        return 0;
    }
    #else
    #endif
}
static void divoom_lvgl_photo_push_show_empty_album_label(Pdivoom_lvgl_photo_push_ctrl phandle)
{
    void *use_font = divoom_menu_control_get_font_info(30, phandle->menu_id);
    void *label_obj = lv_label_create(phandle->main_obj);

    if(use_font != NULL)
    {
        lv_obj_set_style_text_font(label_obj, use_font, 0);
    }
    lv_label_set_long_mode(label_obj, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(label_obj, DIVOOM_LVGL_PHOTO_PUSH_EMPTY_HINT_W);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_align(label_obj, LV_ALIGN_CENTER);
    lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("PlsAddPhoto"));
}

//读照片的线程
void *divoom_lvgl_photo_push_update_image_buffer(void *data)
{
    Pdivoom_lvgl_photo_push_ctrl phandle = (Pdivoom_lvgl_photo_push_ctrl)data;

    while(phandle->run_flag)
    {
        divoom_system_get_sem(phandle->pread_sem);
        if(phandle->run_flag == 0)
        {
            break;
        }
        if(phandle->update_display_flag != 1)
        {
            continue;
        }
        if(divoom_lvgl_photo_push_load_image_buffer(phandle, phandle->pload_item, 1))
        {
            phandle->update_display_flag = 2;
        }
    }

    divoom_system_put_sem(phandle->pexit_sem);
    phandle->read_task_alive = 0;
    divoom_print("exit all!\n");
    return  NULL;
    
}

void *divoom_lvgl_photo_push_init(void *user_data, int menu_id)
{
    int i;
    Pdivoom_lvgl_photo_push_ctrl phandle = NULL;
    Pdivoom_lvgl_photo_slide_effect_info para_info = (Pdivoom_lvgl_photo_slide_effect_info)user_data;

    if(para_info == NULL) return NULL;
    lv_img_cache_invalidate_src(NULL);
    phandle = divoom_system_malloc(sizeof(divoom_lvgl_photo_push_ctrl));
    if(phandle == NULL) return NULL;
    memset(phandle, 0, sizeof(*phandle));
    malloc_trim(0);
    phandle->album_id = para_info->album_id;
    phandle->menu_id = menu_id;
    phandle->run_flag = 0;
    phandle->update_display_flag = 0;
    phandle->pslide_config = divoom_pic_phone_manage_get_slide_config();
    phandle->video_handoff_flag = 0;
    phandle->pget_pic_handle = divoom_pic_album_manage_create_disp_order_handle(para_info->album_id, phandle->pslide_config->disp_order, phandle->pslide_config->disp_reverse, 1);
    if(phandle->pget_pic_handle == NULL) goto init_failed;
    phandle->pic_num = divoom_pic_album_manage_get_disp_order_pic_num(phandle->pget_pic_handle);
    phandle->main_obj = lv_btn_create(NULL);
    if(phandle->main_obj == NULL) goto init_failed;
    divoom_menu_control_init_btn_stype(NULL, NULL, &phandle->main_style, NULL, NULL, NULL, 0);
    lv_obj_set_pos(phandle->main_obj, 0, 0);
    lv_obj_add_style(phandle->main_obj, &phandle->main_style, 0);
    lv_obj_set_size(phandle->main_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
    lv_obj_clear_flag(phandle->main_obj, LV_OBJ_FLAG_SCROLLABLE);
    if(phandle->pic_num == 0)
    {
        divoom_lvgl_photo_push_show_empty_album_label(phandle);
        divoom_print("empty album init: %d\n", phandle->album_id);
        return phandle;
    }

    phandle->run_flag = 1;
    phandle->pread_sem = divoom_system_create_sem(1, 0);
    phandle->pexit_sem = divoom_system_create_sem(1, 0);
    phandle->pcur_item = divoom_system_malloc(sizeof(divoom_lvgl_photo_push_item));
    phandle->pload_item = divoom_system_malloc(sizeof(divoom_lvgl_photo_push_item));
    if(phandle->pcur_item) memset(phandle->pcur_item, 0, sizeof(*phandle->pcur_item));
    if(phandle->pload_item) memset(phandle->pload_item, 0, sizeof(*phandle->pload_item));
    if(phandle->pcur_item == NULL || phandle->pload_item == NULL ||
        phandle->pread_sem == NULL || phandle->pexit_sem == NULL)
        goto init_failed;
    phandle->pcur_item->image_info = &phandle->pcur_item->raw_info;
    phandle->pcur_item->image_data = &phandle->pcur_item->lv_image_info;
    divoom_lv_img_dsc_init_fmt(phandle->pcur_item->image_data,
        DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH, LV_COLOR_FORMAT_RGB565A8, NULL);
    phandle->pcur_item->image_data->data = divoom_system_malloc(phandle->pcur_item->image_data->data_size);

    phandle->pload_item->image_info = &phandle->pload_item->raw_info;
    phandle->pload_item->image_data = &phandle->pload_item->lv_image_info;
    divoom_lv_img_dsc_init_fmt(phandle->pload_item->image_data,
        DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH, LV_COLOR_FORMAT_RGB565A8, NULL);
    phandle->pload_item->image_data->data = divoom_system_malloc(phandle->pload_item->image_data->data_size);
    if(phandle->pcur_item->image_data->data == NULL || phandle->pload_item->image_data->data == NULL)
        goto init_failed;

    divoom_pic_album_manage_get_last_play_album_pic_id(phandle->album_id, &phandle->play_one_flag);
    phandle->read_task_alive = 1;
    if(divoom_system_create_task(divoom_lvgl_photo_push_update_image_buffer, "lv_photo", phandle, DIVOOM_TASK_STACK_LVGL_PHOTO_PUSH) == NULL)
    {
        phandle->read_task_alive = 0;
        goto init_failed;
    }

    phandle->pload_item->img_obj = lv_img_create(phandle->main_obj);
    if(phandle->pload_item->img_obj == NULL) goto init_failed;
    lv_obj_set_size(phandle->pload_item->img_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
    lv_img_set_src(phandle->pload_item->img_obj, phandle->pload_item->image_data);
    phandle->pcur_item->img_obj = lv_img_create(phandle->main_obj);
    if(phandle->pcur_item->img_obj == NULL) goto init_failed;
    lv_obj_set_size(phandle->pcur_item->img_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
    lv_img_set_src(phandle->pcur_item->img_obj, phandle->pcur_item->image_data);
    for(i = 0; i < 2; i++)
    {
        lv_anim_init(&phandle->image_anim[i]);
    }
    
    phandle->run_tick = 0;
    phandle->use_pos = divoom_pic_album_manage_get_first_play_info(phandle->pget_pic_handle, NULL);
    divoom_pic_album_manage_get_disp_order_pic_id(phandle->pget_pic_handle, phandle->use_pos,phandle->pcur_item->image_info);
    phandle->use_pos ++;
    divoom_lvgl_photo_push_load_image_buffer(phandle, phandle->pcur_item, 0);
    if(phandle->pcur_item->image_info->pic_video_flag)
    {
        char file_name[256];
        
        divoom_pic_phone_manage_get_picture_name(file_name, phandle->pcur_item->image_info->id);   
        phandle->pcur_item->video_handle = divoom_lvgl_movie_play_play(phandle->pcur_item->img_obj, phandle,0, 0, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH, 
            file_name, divoom_pic_phone_manage_convert_video_volume(phandle->pslide_config->video_volume), 0, phandle->pslide_config->play_time, divoom_lvgl_photo_push_play_over_callback);     
        divoom_menu_control_change_disp_mode(phandle->main_obj, 1);
        lv_obj_add_flag(phandle->pcur_item->img_obj, LV_OBJ_FLAG_HIDDEN);
    }
    phandle->dir_flag = 1;
    phandle->read_timer = lv_timer_create(divoom_lvgl_photo_push_read_time_handle, 100, phandle);
    if(phandle->read_timer == NULL) goto init_failed;
    lv_timer_pause(phandle->read_timer);        
    phandle->refresh_timer = lv_timer_create(divoom_lvgl_photo_push_refresh_image_time_handle, 16, phandle);
    phandle->photo_title_obj = NULL;
    if(phandle->refresh_timer == NULL) goto init_failed;
    lv_timer_pause(phandle->refresh_timer);
#ifdef DIVOOM_SIMULATOR
    phandle->sim_keepalive_timer = lv_timer_create(divoom_lvgl_photo_push_sim_keepalive_cb, 500, phandle);
    if(phandle->sim_keepalive_timer == NULL) goto init_failed;
#endif
    
    //lv_obj_add_event_cb(phandle->main_obj, divoom_lvgl_photo_push_event_cb,
    //    LV_EVENT_ALL, phandle);
    return phandle;
init_failed:
    divoom_lvgl_photo_push_exit(phandle);
    return NULL;
}

void *divoom_lvgl_photo_push_get_lvgl_obj(void *handle)
{
    Pdivoom_lvgl_photo_push_ctrl phandle = (Pdivoom_lvgl_photo_push_ctrl)handle;
    
    return phandle->main_obj;
}

void divoom_lvgl_photo_push_update_time(void *handle, int play_time)
{
    Pdivoom_lvgl_photo_push_ctrl phandle = (Pdivoom_lvgl_photo_push_ctrl)handle;
    

    //lv_timer_set_period(phandle->read_timer, play_time * 1000);
}

void divoom_lvgl_photo_push_play_next(void *handle, int  dir)
{
    Pdivoom_lvgl_photo_push_ctrl phandle = (Pdivoom_lvgl_photo_push_ctrl)handle;
    int total;

    if(phandle == NULL || phandle->pic_num == 0 || phandle->read_timer == NULL || phandle->pcur_item == NULL)
    {
        return;
    }
    total = divoom_pic_album_manage_get_disp_order_pic_num(phandle->pget_pic_handle);
    /* 没有下一项时保持当前播放，避免重建同一视频导致黑屏。 */
    if(total == 1 && phandle->pic_num == total &&
        divoom_pic_album_manage_get_disp_last_pic_id(phandle->pget_pic_handle) <= 0)
    {
        return;
    }
    if(total > 2 && dir == 0)
    {
        phandle->use_pos = (phandle->use_pos + total - 2)%total;
    }
    phandle->dir_flag = dir;
    phandle->run_tick = phandle->pslide_config->play_time / 100;
    if(phandle->pcur_item->image_info && phandle->pcur_item->image_info->pic_video_flag)
    {
        phandle->end_flag = 1;
    }
    lv_timer_ready(phandle->read_timer);

}


void divoom_lvgl_photo_push_get_pic_info(void *handle, Pdivoom_lvgl_photo_view_para_info pic_info)
{
    Pdivoom_lvgl_photo_push_ctrl phandle = (Pdivoom_lvgl_photo_push_ctrl)handle;
    
    
    phandle->pic_num = divoom_pic_album_manage_get_disp_order_pic_num(phandle->pget_pic_handle);
    pic_info->cur_pos = phandle->use_pos;
    if(phandle->pcur_item && phandle->pcur_item->image_info)
    {
        pic_info->pic_id = phandle->pcur_item->image_info->id;
    }
    pic_info->pic_num = &phandle->pic_num;
    pic_info->pget_pic_handle = phandle->pget_pic_handle;
    pic_info->pdel_callback = NULL;
}

void divoom_lvgl_photo_push_remove_notice(void *handle, uint32 *pic_list, int pic_num)
{
    Pdivoom_lvgl_photo_push_ctrl phandle = (Pdivoom_lvgl_photo_push_ctrl)handle;

    if(phandle == NULL || phandle->pic_num == 0 || phandle->pcur_item == NULL || phandle->pcur_item->image_info == NULL)
    {
        return;
    }
    for(int i = 0; i < pic_num; i++)
    {
        if(phandle->pcur_item->image_info->id == pic_list[i])
        {
            if(phandle->pcur_item->video_handle)
            {
                divoom_menu_control_change_disp_mode(phandle->main_obj, 0);
                divoom_lvgl_movie_play_stop(phandle->pcur_item->video_handle, NULL);
                phandle->pcur_item->video_handle = NULL;
            }
            if(phandle->pcur_item->play_gif_handle)
            {
                void *gif_handle = phandle->pcur_item->play_gif_handle;
                phandle->pcur_item->play_gif_handle = NULL;
                divoom_lvgl_play_gif_file_destory_handle(gif_handle, 0);
            }
        }
        
        if(phandle->pload_item && phandle->pload_item->image_info &&
            phandle->pload_item->image_info->id == pic_list[i])
        {
            if(phandle->pload_item->video_handle)
            {
                divoom_menu_control_change_disp_mode(phandle->main_obj, 0);
                divoom_lvgl_movie_play_stop(phandle->pload_item->video_handle, NULL);
                phandle->pload_item->video_handle = NULL;
            }
            if(phandle->pload_item->play_gif_handle)
            {
                void *gif_handle = phandle->pload_item->play_gif_handle;
                phandle->pload_item->play_gif_handle = NULL;
                divoom_lvgl_play_gif_file_destory_handle(gif_handle, 0);
            }
        }
        divoom_pic_album_manage_remove_pic_id_from_disp_handle(phandle->pget_pic_handle, pic_list[i]);
        phandle->pic_num = divoom_pic_album_manage_get_disp_order_pic_num(phandle->pget_pic_handle);        
    }
    if(phandle->use_pos >= phandle->pic_num)
    {
        phandle->use_pos = phandle->pic_num > 0 ? phandle->pic_num - 1 : 0;
    }
}

void divoom_lvgl_photo_push_update_disp(void *handle)
{
    Pdivoom_lvgl_photo_push_ctrl phandle = (Pdivoom_lvgl_photo_push_ctrl)handle;
    Pdivoom_lvgl_photo_push_item pitem = phandle->pcur_item;

    if(divoom_lvgl_photo_push_load_image_buffer(phandle, pitem, 0))
    {
        lv_obj_invalidate(pitem->img_obj);
        if(phandle->pcur_item->video_handle)
        {
            divoom_menu_control_change_disp_mode(phandle->main_obj, 1);
            if(divoom_local_movie_play_have_handle())
            {
                divoom_lvgl_movie_play_ctrl(phandle->pcur_item->video_handle, 1);
            }
            else
            {
                char file_name[256];
                
                divoom_pic_phone_manage_get_picture_name(file_name, phandle->pcur_item->image_info->id);   
                phandle->pcur_item->video_handle = divoom_lvgl_movie_play_play(phandle->pcur_item->img_obj, phandle,0, 0, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH, 
                    file_name, divoom_pic_phone_manage_convert_video_volume(phandle->pslide_config->video_volume), 0, phandle->pslide_config->play_time, divoom_lvgl_photo_push_play_over_callback);
                lv_obj_add_flag(phandle->pcur_item->img_obj, LV_OBJ_FLAG_HIDDEN);
            }
        }
    }
}

void divoom_lvgl_photo_push_disp(void *handle, void *active_obj)
{
    Pdivoom_lvgl_photo_push_ctrl phandle = (Pdivoom_lvgl_photo_push_ctrl)handle;

    if(phandle->refresh_timer)
    {
        lv_timer_resume(phandle->refresh_timer);
    }
    if(phandle->read_timer)
    {
        lv_timer_resume(phandle->read_timer);
    }
    lv_scr_load(phandle->main_obj);
    if(phandle->pic_num == 0)
    {
        divoom_menu_control_change_disp_mode(phandle->main_obj, 0);
        return;
    }
    if(phandle->pcur_item->image_info && phandle->pcur_item->image_info->pic_video_flag)
    {
        divoom_menu_control_change_disp_mode(phandle->main_obj, 1);
        lv_obj_add_flag(phandle->pcur_item->img_obj, LV_OBJ_FLAG_HIDDEN);
        if(divoom_local_movie_play_have_handle())
        {
            divoom_lvgl_movie_play_ctrl(phandle->pcur_item->video_handle, 1);
        }
        else
        {
            char file_name[256];
            
            divoom_pic_phone_manage_get_picture_name(file_name, phandle->pcur_item->image_info->id);   
            phandle->pcur_item->video_handle = divoom_lvgl_movie_play_play(phandle->pcur_item->img_obj, phandle,0, 0, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH, 
                file_name, divoom_pic_phone_manage_convert_video_volume(phandle->pslide_config->video_volume), 0, phandle->pslide_config->play_time, divoom_lvgl_photo_push_play_over_callback);
        }
    }
    else
    {
        divoom_menu_control_change_disp_mode(phandle->main_obj, 0);
        divoom_lvgl_photo_push_refresh_photo_layer(phandle);
    }
}

void divoom_lvgl_photo_push_pause(void *handle, int exit_flag)
{
    Pdivoom_lvgl_photo_push_ctrl phandle = (Pdivoom_lvgl_photo_push_ctrl)handle;
    
    if(phandle)
    {
        phandle->video_handoff_flag = 0;
        if(phandle->refresh_timer)
        {
            lv_timer_pause(phandle->refresh_timer);
        }
        if(phandle->read_timer)
        {
            lv_timer_pause(phandle->read_timer);
        }
        if(phandle->pcur_item && phandle->pcur_item->video_handle)
        {
            int image_w, image_h;
            if(exit_flag == 0)
            {
                exec_command_system("echo /tmp/filename.bmp>/sys/class/disp/disp/attr/capture_dump");
            }
            divoom_menu_control_change_disp_mode(phandle->main_obj, 0);
            divoom_lvgl_movie_play_stop(phandle->pcur_item->video_handle, NULL);
            phandle->pcur_item->video_handle = NULL;
            if(exit_flag == 0)
            {
                image_w = DIVOOM_SCREEN_WIDTH;
                image_h = DIVOOM_SCREEN_HIGH;
                divoom_decode_bmp_file_lvfmt(DIVOOM_TMP_DIR "/filename.bmp",
                    &image_w, &image_h,
                    (uint8 *)phandle->pcur_item->image_data->data,
                    LV_COLOR_FORMAT_RGB565A8);
                divoom_app_com_del_file(DIVOOM_TMP_DIR "/filename.bmp");
                lv_obj_clear_flag(phandle->pcur_item->img_obj, LV_OBJ_FLAG_HIDDEN);
            }
        }
    }
}

void divoom_lvgl_photo_push_exit(void *handle)
{
    Pdivoom_lvgl_photo_push_ctrl phandle = (Pdivoom_lvgl_photo_push_ctrl)handle;

    if(phandle == NULL)
    {
        return;
    }
    lv_anim_del(phandle, NULL);
    if (phandle->panim_handle_obj[0])
    {
        lv_anim_del(phandle->panim_handle_obj[0], NULL);
    }
    if (phandle->panim_handle_obj[1])
    {
        lv_anim_del(phandle->panim_handle_obj[1], NULL);
    }

    if(phandle->refresh_timer)
    {
        lv_timer_del(phandle->refresh_timer);
    }
    if(phandle->read_timer)
    {
        lv_timer_del(phandle->read_timer);
    }
#ifdef DIVOOM_SIMULATOR
    if(phandle->sim_keepalive_timer)
    {
        lv_timer_del(phandle->sim_keepalive_timer);
    }
#endif
    phandle->run_flag = 0;
    if(phandle->read_task_alive)
    {
        divoom_system_put_sem(phandle->pread_sem);
        divoom_free_lvgl_lock();
        while(phandle->read_task_alive)
            divoom_system_get_sem_with_time(phandle->pexit_sem, 1);
        divoom_get_lvgl_lock();
    }
    {
        /* STOP is asynchronous; callbacks still own phandle until idle. */
        divoom_free_lvgl_lock();
        while(divoom_local_movie_play_stop_owner(phandle, 1000) != 0)
            divoom_print("photo push: waiting for owned video callback to finish\n");
        divoom_get_lvgl_lock();
        if(phandle->pcur_item) phandle->pcur_item->video_handle = NULL;
        if(phandle->pload_item) phandle->pload_item->video_handle = NULL;
        if(phandle->main_obj && divoom_local_movie_play_is_idle())
            divoom_menu_control_change_disp_mode(phandle->main_obj, 0);
    }
    //在这里清除所有信息
    if(phandle->pget_pic_handle)
    {
        divoom_pic_album_manage_destory_disp_order_handle(phandle->pget_pic_handle);
        phandle->pget_pic_handle = NULL;
    }
    if(phandle->pcur_item)
    {
        if(phandle->pcur_item->play_gif_handle)
        {
            divoom_lvgl_play_gif_file_destory_handle(phandle->pcur_item->play_gif_handle, 0);
        }
        if(phandle->pcur_item->image_info)
        {
            divoom_pic_album_manage_update_last_play_album_pic_id(phandle->album_id, phandle->pcur_item->image_info->id);
        }
    }
    if(phandle->pload_item)
    {
        if(phandle->pload_item->play_gif_handle)
        {
            divoom_lvgl_play_gif_file_destory_handle(phandle->pload_item->play_gif_handle, 0);
        }

    }

    if(phandle->pexit_sem) divoom_system_destroy_sem(phandle->pexit_sem);
    if(phandle->pread_sem) divoom_system_destroy_sem(phandle->pread_sem);
    if(phandle->main_obj) lv_obj_del(phandle->main_obj);
    lv_style_reset(&phandle->main_style);
    divoom_lvgl_photo_push_free_image_item(phandle->pcur_item);
    divoom_lvgl_photo_push_free_image_item(phandle->pload_item);
    if(phandle->prandom_handle)
    {
        divoom_random_destroy(phandle->prandom_handle);
    }
    if(phandle->pmix_random_handle)
    {
        divoom_random_destroy(phandle->pmix_random_handle);
    }
    divoom_system_free(phandle);
    divoom_print("exit!\n");
}
