#include "divoom.h"
#include "divoom_lvgl_photo_view.h"
#include "divoom_lvgl_photo_add_picture_to_album.h"
#include "lvgl.h"
#include "divoom_menu_control.h"
#include "divoom_astrootoo_ui_base.h"
#include "divoom_astrootoo_theme.h"
#include "divoom_pixel_format.h"
#ifdef DIVOOM_USE_RK_IC
#include "rk_mpi.h"
#include <rga/im2d.h>
#include <rga/rga.h>
#include <rga/RgaApi.h>
#endif

/*
 这个文件主要是对照片进行管理，可以删除照片，也可以将照片归类到某个相册中, 图片显示正方形
*/
#define DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_PIC_IMAGE_SELECT_OPA  200
#define DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_PIC_IMAGE_HIDE_OPA    150
#define DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_PIC_ROW_NUM (3)
#define DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_PIC_GAP    12
#define DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_PIC_PAD    18
#define DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_PIC_WIDTH  (140)
#define DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_CONT_WIDTH  (DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_PIC_WIDTH + DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_PIC_GAP)
#define DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_CONTENT_Y   DIVOOM_ASTRO_CONTENT_Y
#define DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_CONTENT_H   (DIVOOM_SCREEN_HIGH - DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_CONTENT_Y)
#define DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_GRID_BASE_Y (DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_CONTENT_Y - DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_CONT_WIDTH)
#define DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_PIC_NUM_PRE_SCREEN (DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_CONTENT_H / DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_CONT_WIDTH)
#define DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_PIC_LIST_NUM (DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_PIC_NUM_PRE_SCREEN+3)

typedef struct _divoom_lvgl_photo_add_picture_to_album_refresh_item
{
    int pos1;
    int pos2;
    int image_w;
    int image_h;
}divoom_lvgl_photo_add_picture_to_album_refresh_item, *Pdivoom_lvgl_photo_add_picture_to_album_refresh_item;

typedef struct _divoom_lvgl_photo_add_picture_to_album_item
{
    void *cout;
    int pos_y;    
    uint8 img_update_flag[DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_PIC_ROW_NUM];  
    int img_pos[DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_PIC_ROW_NUM];  
    int img_id[DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_PIC_ROW_NUM];
    lv_img_dsc_t *image_data[DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_PIC_ROW_NUM];
    void *img_obj[DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_PIC_ROW_NUM];
    void *select_obj[DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_PIC_ROW_NUM];
    void *hide_flag_obj[DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_PIC_ROW_NUM];
}divoom_lvgl_photo_add_picture_to_album_item, *Pdivoom_lvgl_photo_add_picture_to_album_item;

typedef struct _divoom_lvgl_photo_add_picture_to_album_update_image_info
{
    int  hide_flag;
    int  select_flag;
    void *hide_flag_obj;
    void *image_obj;
    void *select_obj;
    void *image_data;
}divoom_lvgl_photo_add_picture_to_album_update_image_info, *Pdivoom_lvgl_photo_add_picture_to_album_update_image_info;

typedef struct _divoom_lvgl_photo_add_picture_to_album_ctrl
{
    divoom_astrootoo_ui_base ui;
    int menu_id;
    int first_load_flag;
    int operate_btn_flag;   //操作按钮
    int dest_pos;    //最大的偏移量
    int max_pos;    //最大的偏移量
    int dir;
    int start_x;
    int start_y;
    int press_x;
    int press_y;
    int move_steps;
    int press_time;
    lv_timer_t *timer;
    lv_timer_t *update_image_timer;
    int speed_up_time;
    int base_pos;
    int speed_up_speed;
    int pic_num;
    int timer_tick;
    int idle_tick;
    int pre_select_area_id;
    int  album_id;
    unsigned long pre_sum;
    unsigned long cur_sum;
    uint8 *select_table;
    uint8 pic_buf[DIVOOM_THUMB_PIC_HIGH * DIVOOM_THUMB_PIC_WIDTH * DIVOOM_APP_RGB565A8_BYTES_PER_PX];
    uint8 rotate_buf[DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_PIC_WIDTH * DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_PIC_WIDTH * DIVOOM_APP_RGB565A8_BYTES_PER_PX];
    int read_pic_pos;
    void *image_queue;
    void *update_image_queue;
    void *main_obj;
    void *image_main_cout_obj;   //图像的容器
    void *return_btn_obj;        //取消，退回前一级菜单
    void *select_btn_obj;        //开始选择照片
    void *select_label_obj;        //开始选择照片
    void *btn_cout_obj;          //按钮的容器，长按时显示，默认情况下隐藏
    void *func_btn_table[10];     //功能按键数组
    void *pget_pic_handle;
    divoom_lvgl_photo_view_para_info photo_view_info;
    divoom_lvgl_photo_add_picture_to_album_item plist[DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_PIC_LIST_NUM];
}divoom_lvgl_photo_add_picture_to_album_ctrl, *Pdivoom_lvgl_photo_add_picture_to_album_ctrl;

Pdivoom_lvgl_photo_add_picture_to_album_ctrl pdivoom_lvgl_photo_add_picture_to_album_handle = NULL;

int divoom_lvgl_photo_add_picture_to_album_get_next_image_id(Pdivoom_lvgl_photo_add_picture_to_album_ctrl phandle)
{
    int i, max_pos, find, image_pos = 0;

    max_pos = lv_obj_get_y(phandle->plist[0].cout);
    find = 0;
    for(i  = 1; i < DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_PIC_LIST_NUM; i++)
    {
        if(phandle->plist[i].img_id[0] == 0)
        {
            continue;
        }
        if(max_pos < phandle->plist[i].pos_y)
        {
            max_pos = phandle->plist[i].pos_y;
            find = i;
        }
    }
    for(i = 0; i < DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_PIC_ROW_NUM; i++)
    {
        if(phandle->plist[find].img_id[i] == 0)
        {
            break;
        }
        image_pos = phandle->plist[find].img_pos[i];
    }
    return image_pos;
}

int divoom_lvgl_photo_add_picture_to_album_get_pre_image_id(Pdivoom_lvgl_photo_add_picture_to_album_ctrl phandle)
{
    int i, min_pos, find, image_pos = 0;

    min_pos = lv_obj_get_y(phandle->plist[0].cout);
    find = 0;
    for(i  = 1; i < DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_PIC_LIST_NUM; i++)
    {
        if(phandle->plist[i].img_id[0] == 0)
        {
            continue;
        }
        if(min_pos > phandle->plist[i].pos_y)
        {
            min_pos = phandle->plist[i].pos_y;
            find = i;
        }
    }
    return phandle->plist[find].img_pos[0];
}
void divoom_lvgl_photo_add_picture_to_album_get_disp_buf(void *handle, int pic_pos, uint8 *rgb_buf, int image_w, int image_h, uint8* out_buf)
{
    (void)handle;
    (void)pic_pos;

    if(rgb_buf == NULL || out_buf == NULL)
    {
        return;
    }
    if(image_w != DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_PIC_WIDTH ||
        image_h != DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_PIC_WIDTH)
    {
        return;
    }
    memcpy(out_buf, rgb_buf,
        divoom_pixel_format_size(DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_PIC_WIDTH,
            DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_PIC_WIDTH,
            LV_COLOR_FORMAT_RGB565A8));
}

int divoom_lvgl_photo_add_picture_to_album_read_pic(int list_id, int pic_pos, int pic_id, int asyn_flag, uint8 *rgb_buf, DIVOOM_PIC_ALBUM_MANAGE_READ_OVER *pcallback, void *para)
{
    (void)pic_pos;

    return divoom_thumb_cache_read_pic(list_id, pic_id, asyn_flag, rgb_buf,
        DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_PIC_WIDTH,
        DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_PIC_WIDTH, pcallback, para);
}
void divoom_lvgl_photo_add_picture_to_album_update_finish_read_over_callback(void *para, uint8 *rgb_buf,int image_w, int image_h)
{
    int i, j;


    for(i = 0; i <DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_PIC_LIST_NUM; i++)
    {
        for(j = 0; j < DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_PIC_ROW_NUM; j++)
        {
            if(pdivoom_lvgl_photo_add_picture_to_album_handle->plist[i].img_obj[j] == para)
            {
                if(pdivoom_lvgl_photo_add_picture_to_album_handle->plist[i].img_update_flag[j])
                {
                    Pdivoom_lvgl_photo_add_picture_to_album_update_image_info update_image_info = divoom_system_malloc(sizeof(divoom_lvgl_photo_add_picture_to_album_update_image_info));
                    lv_img_dsc_t *image_tmp_handle = divoom_queue_pop(pdivoom_lvgl_photo_add_picture_to_album_handle->image_queue);

                    memcpy(image_tmp_handle, pdivoom_lvgl_photo_add_picture_to_album_handle->plist[i].image_data[j], sizeof(lv_img_dsc_t));
                    pdivoom_lvgl_photo_add_picture_to_album_handle->plist[i].img_update_flag[j] = 0;
                    divoom_lvgl_photo_add_picture_to_album_get_disp_buf(pdivoom_lvgl_photo_add_picture_to_album_handle->pget_pic_handle,pdivoom_lvgl_photo_add_picture_to_album_handle->plist[i].img_pos[j],rgb_buf, image_w, image_h, (uint8 *)image_tmp_handle->data);
                    update_image_info->image_data = image_tmp_handle;
                    update_image_info->hide_flag = 0;
                    update_image_info->hide_flag_obj = pdivoom_lvgl_photo_add_picture_to_album_handle->plist[i].hide_flag_obj[j];
                    update_image_info->select_obj = pdivoom_lvgl_photo_add_picture_to_album_handle->plist[i].select_obj[j];
                    update_image_info->select_flag = pdivoom_lvgl_photo_add_picture_to_album_handle->select_table[pdivoom_lvgl_photo_add_picture_to_album_handle->plist[i].img_pos[j]];
                    update_image_info->image_obj = pdivoom_lvgl_photo_add_picture_to_album_handle->plist[i].img_obj[j];
                    divoom_queue_push(pdivoom_lvgl_photo_add_picture_to_album_handle->update_image_queue, update_image_info);
                    divoom_queue_push(pdivoom_lvgl_photo_add_picture_to_album_handle->image_queue, pdivoom_lvgl_photo_add_picture_to_album_handle->plist[i].image_data[j]);
                    pdivoom_lvgl_photo_add_picture_to_album_handle->plist[i].image_data[j] = image_tmp_handle;
                }
                else
                {
                    divoom_print("the pic will be passed: %d, %d\n", i, j);
                }
                return;
            }
        }
    }
}

int divoom_lvgl_photo_add_picture_to_album_compare_update_image_item(void *item1, void *item2)
{
    Pdivoom_lvgl_photo_add_picture_to_album_update_image_info pimage_info = (Pdivoom_lvgl_photo_add_picture_to_album_update_image_info)item2;

    return item1 == pimage_info->image_obj?0:1;
}

void divoom_lvgl_photo_add_picture_to_album_remove_update_image_item(Pdivoom_lvgl_photo_add_picture_to_album_ctrl phandle, void *image_obj)
{
    if(divoom_queue_items(phandle->update_image_queue))
    {        
        Pdivoom_lvgl_photo_add_picture_to_album_update_image_info pimage_info = divoom_queue_pop_cmp(phandle->update_image_queue, image_obj, divoom_lvgl_photo_add_picture_to_album_compare_update_image_item);

        if(pimage_info)
        {
            divoom_system_free(pimage_info);
        }
    }
}

static void divoom_lvgl_photo_add_picture_to_album_update_y(Pdivoom_lvgl_photo_add_picture_to_album_ctrl phandle, int steps)
{
    int i, j, y, h, max_high, find_pos, image_w, image_h;

    if(phandle->base_pos + steps < 0)
    {
        steps = 0 - phandle->base_pos;
    }
    else if(phandle->base_pos + steps > phandle->max_pos)
    {
        steps = phandle->max_pos - phandle->base_pos;
    }
    if(steps == 0)
    {
        //divoom_print("move: %d, %d\n", phandle->max_pos, phandle->base_pos);
        return;
    }
    phandle->base_pos += steps;
    //divoom_print("move: %d, %d\n", steps, phandle->base_pos);
    max_high = DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_PIC_LIST_NUM  * DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_CONT_WIDTH ;
    for (i = 0; i < DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_PIC_LIST_NUM; i++)
    {
        h = phandle->plist[i].pos_y;
        y = h + steps;
        if(y >= max_high)
        {
            //只有向下运动才可能出现这情况
            y  -= max_high;
            //这个位置要重新加载图片
            find_pos = divoom_lvgl_photo_add_picture_to_album_get_pre_image_id(phandle);
            if(find_pos < DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_PIC_ROW_NUM)
            {
                find_pos = DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_PIC_ROW_NUM;
            }
            if(phandle->dir != -1)
            {
                //缓冲方向发生变化 
                divoom_thumb_cache_clear_wait(0);
            }
            find_pos -= DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_PIC_ROW_NUM;
            for(j = 0; j < DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_PIC_ROW_NUM; j ++)
            {
                if(find_pos >= 0 && find_pos < phandle->pic_num)
                {
                    divoom_lvgl_photo_add_picture_to_album_remove_update_image_item(phandle, phandle->plist[i].img_obj[j]);
                    lv_obj_clear_flag(phandle->plist[i].img_obj[j], LV_OBJ_FLAG_HIDDEN);
                    phandle->plist[i].img_id[j] = divoom_pic_album_manage_get_disp_order_pic_id(phandle->pget_pic_handle,find_pos, NULL);
                    phandle->plist[i].img_pos[j] = find_pos;
                    find_pos++;
                    //divoom_print("need update image!%d, %d, %d, %d\n", i, j,phandle->plist[i].img_pos[j], phandle->plist[i].img_id[j]);
                    if(divoom_lvgl_photo_add_picture_to_album_read_pic(0, phandle->plist[i].img_pos[j], phandle->plist[i].img_id[j], 0, 
                        (uint8 *)phandle->plist[i].image_data[j]->data,  divoom_lvgl_photo_add_picture_to_album_update_finish_read_over_callback, phandle->plist[i].img_obj[j]))
                    {
                        lv_img_dsc_t *image_tmp_handle = divoom_queue_pop(phandle->image_queue);
                        
                        phandle->plist[i].img_update_flag[j] = 0;
                        memcpy(image_tmp_handle, phandle->plist[i].image_data[j], sizeof(lv_img_dsc_t));
                        //memcpy(image_tmp_handle->data, phandle->plist[i].image_data[j]->data, DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_PIC_WIDTH * DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_PIC_WIDTH*3);

                        lv_img_set_src(phandle->plist[i].img_obj[j], image_tmp_handle);
                        
                        lv_obj_add_flag(phandle->plist[i].hide_flag_obj[j], LV_OBJ_FLAG_HIDDEN);
                        
                        if(phandle->select_table[phandle->plist[i].img_pos[j]])
                        {
                            lv_obj_clear_flag(phandle->plist[i].select_obj[j], LV_OBJ_FLAG_HIDDEN);
                            lv_obj_set_style_img_opa(phandle->plist[i].img_obj[j], DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_PIC_IMAGE_SELECT_OPA, 0);
                        }
                        else 
                        {
                            lv_obj_add_flag(phandle->plist[i].select_obj[j], LV_OBJ_FLAG_HIDDEN);
                            lv_obj_set_style_img_opa(phandle->plist[i].img_obj[j], 255, 0);
                        }
                        divoom_queue_push(phandle->image_queue, phandle->plist[i].image_data[j]);
                        phandle->plist[i].image_data[j] = image_tmp_handle;
                    }
                    else
                    {
                        phandle->plist[i].img_update_flag[j] = 1;
                        //divoom_print("wait update image!%d, %d\n", i, j);
                    }
                }
                else
                {
                    divoom_print("we will hiden image!%d, %d, %d, %d\n", i, j, find_pos, phandle->pic_num);
                    lv_obj_add_flag(phandle->plist[i].img_obj[j], LV_OBJ_FLAG_HIDDEN);
                }
            }
            find_pos -= (DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_PIC_ROW_NUM + 1);
            for(j = 0; j < DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_PIC_ROW_NUM * 3; j++)
            {
                if(find_pos - j < 0)
                {
                    break;
                }
                divoom_thumb_cache_pre_read(0, divoom_pic_album_manage_get_disp_order_pic_id(phandle->pget_pic_handle,find_pos - j, NULL));            
            }
            phandle->dir = -1;
        }
        else if(y < 0)
        {
            //只有向上运动才可能出现这情况
            y  += max_high;
            if(phandle->dir != 1)
            {
                divoom_thumb_cache_clear_wait(0);
            }
            find_pos = divoom_lvgl_photo_add_picture_to_album_get_next_image_id(phandle) + 1;
            for(j = 0; j < DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_PIC_ROW_NUM; j ++)
            {
                if(find_pos < phandle->pic_num)
                {
                    divoom_lvgl_photo_add_picture_to_album_remove_update_image_item(phandle, phandle->plist[i].img_obj[j]);
                    lv_obj_clear_flag(phandle->plist[i].img_obj[j], LV_OBJ_FLAG_HIDDEN);
                    phandle->plist[i].img_id[j] = divoom_pic_album_manage_get_disp_order_pic_id(phandle->pget_pic_handle,find_pos, NULL);
                    phandle->plist[i].img_pos[j] = find_pos;
                    //divoom_print("update image: %d, %d, %d,%d\n", i, j, find_pos, phandle->plist[i].img_id[j]);
                    find_pos++;
                    //divoom_print("need update image!%d, %d, %d\n", i, j,phandle->plist[i].img_id[j]);
                    if(divoom_lvgl_photo_add_picture_to_album_read_pic(0, phandle->plist[i].img_pos[j], phandle->plist[i].img_id[j], 0, 
                        (uint8 *)phandle->plist[i].image_data[j]->data, divoom_lvgl_photo_add_picture_to_album_update_finish_read_over_callback, phandle->plist[i].img_obj[j]))
                    {
                        lv_img_dsc_t *image_tmp_handle = divoom_queue_pop(phandle->image_queue);
                        
                        phandle->plist[i].img_update_flag[j] = 0;
                        memcpy(image_tmp_handle, phandle->plist[i].image_data[j], sizeof(lv_img_dsc_t));
                        lv_img_set_src(phandle->plist[i].img_obj[j], image_tmp_handle);
                            lv_obj_add_flag(phandle->plist[i].hide_flag_obj[j], LV_OBJ_FLAG_HIDDEN);
                        if(phandle->select_table[phandle->plist[i].img_pos[j]])
                        {
                            lv_obj_clear_flag(phandle->plist[i].select_obj[j], LV_OBJ_FLAG_HIDDEN);
                            lv_obj_set_style_img_opa(phandle->plist[i].img_obj[j], DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_PIC_IMAGE_SELECT_OPA, 0);
                        }
                        else
                        {
                            lv_obj_add_flag(phandle->plist[i].select_obj[j], LV_OBJ_FLAG_HIDDEN);
                            lv_obj_set_style_img_opa(phandle->plist[i].img_obj[j], 255, 0);
                        }
                        divoom_queue_push(phandle->image_queue, phandle->plist[i].image_data[j]);
                        phandle->plist[i].image_data[j] = image_tmp_handle;
                    }
                    else
                    {
                        phandle->plist[i].img_update_flag[j] = 1;
                    }
                }
                else
                {
                    divoom_print("we will hiden image!%d, %d, %d, %d\n", i, j, find_pos, phandle->pic_num);
                    lv_obj_add_flag(phandle->plist[i].img_obj[j], LV_OBJ_FLAG_HIDDEN);
                }
            }
            for(j = 0; j < DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_PIC_ROW_NUM * 3; j++)
            {
                divoom_thumb_cache_pre_read(0, divoom_pic_album_manage_get_disp_order_pic_id(phandle->pget_pic_handle,(find_pos + j)%phandle->pic_num, NULL));            
            }
            phandle->dir = 1;
        }
        phandle->plist[i].pos_y = y;
        lv_obj_set_y(phandle->plist[i].cout, phandle->plist[i].pos_y);
        //divoom_print("ctrl: %d, %d, %d\n", i, h, y);
    }
}

static void divoom_lvgl_photo_add_picture_to_album_update_image_time_task(lv_timer_t *timer)
{
    Pdivoom_lvgl_photo_add_picture_to_album_ctrl phandle = (Pdivoom_lvgl_photo_add_picture_to_album_ctrl)lv_timer_get_user_data(timer);

    while(divoom_queue_items(phandle->update_image_queue))
    {
        Pdivoom_lvgl_photo_add_picture_to_album_update_image_info pimage_info = divoom_queue_pop(phandle->update_image_queue);
        if(pimage_info)
        {
            lv_img_set_src(pimage_info->image_obj, pimage_info->image_data);
            if(pimage_info->hide_flag)
            {
                lv_obj_clear_flag(pimage_info->hide_flag_obj, LV_OBJ_FLAG_HIDDEN);
                lv_obj_set_style_img_opa(pimage_info->image_obj, DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_PIC_IMAGE_HIDE_OPA, 0);
            }
            else
            {
                lv_obj_add_flag(pimage_info->hide_flag_obj, LV_OBJ_FLAG_HIDDEN);
            }
            
            if(pimage_info->select_flag)
            {
                lv_obj_clear_flag(pimage_info->select_obj, LV_OBJ_FLAG_HIDDEN);
                lv_obj_set_style_img_opa(pimage_info->image_obj, DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_PIC_IMAGE_SELECT_OPA, 0);
            }
            else
            {
                lv_obj_add_flag(pimage_info->select_obj, LV_OBJ_FLAG_HIDDEN);
                lv_obj_set_style_img_opa(pimage_info->image_obj, 255, 0);
            }
            divoom_system_free(pimage_info);            
        }
    }

}


static void divoom_lvgl_photo_add_picture_to_album_time_task(lv_timer_t *timer)
{
    int move_steps, run_steps;
    Pdivoom_lvgl_photo_add_picture_to_album_ctrl phandle = (Pdivoom_lvgl_photo_add_picture_to_album_ctrl)lv_timer_get_user_data(timer);

    
    if(phandle->base_pos == phandle->dest_pos)
    {
        lv_timer_del(timer);
        phandle->timer = NULL;
        phandle->timer_tick = 0;
        return;
    }
    //divoom_print("pos inf: %d, %d, %d\n", phandle->base_pos, phandle->dest_pos, divoom_system_get_tickms());
    if(phandle->timer_tick == 0)
    {
        phandle->pre_sum = 0;
        phandle->cur_sum = 0;
    }
    phandle->timer_tick ++;
    //使用抛物线方程来移动18044950 =  sum(200*200-i*i))
    phandle->cur_sum += 200*200 - phandle->timer_tick*phandle->timer_tick;
    if(((phandle->cur_sum - phandle->pre_sum) * phandle->speed_up_speed)<5353300)
    {
        //不需要移动
        return;
    }
    run_steps = ((phandle->cur_sum - phandle->pre_sum) * phandle->speed_up_speed)/5353300;
    phandle->pre_sum += 5353300 * run_steps/(phandle->speed_up_speed);
    move_steps = phandle->base_pos - phandle->dest_pos;
    if(move_steps > 0)
    {
        if(move_steps > run_steps)
        {
            move_steps = 0 - run_steps;
        }
        else 
        {
            move_steps = 0 -move_steps;
        }
        divoom_lvgl_photo_add_picture_to_album_update_y(phandle, move_steps);
    }
    else
    {
        move_steps = 0 - move_steps;
        if(move_steps > run_steps)
        {
            move_steps = run_steps;
        }
        divoom_lvgl_photo_add_picture_to_album_update_y(phandle, move_steps);
    }
}
void divoom_lvgl_photo_add_picture_to_album_change_items(Pdivoom_lvgl_photo_add_picture_to_album_ctrl phandle, lv_point_t *cur_pos)
{
    int select_area_id;
    int cur_y  = cur_pos->y - DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_GRID_BASE_Y + DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_CONT_WIDTH;
    int cur_x;
    
    for(int i = 0; i < DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_PIC_LIST_NUM; i++)
    {
        if(cur_y >= phandle->plist[i].pos_y && cur_y < phandle->plist[i].pos_y + DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_CONT_WIDTH)
        {
            cur_x = (cur_pos->x - DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_PIC_PAD) / DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_CONT_WIDTH;
            if(cur_x < 0 || cur_x >= DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_PIC_ROW_NUM)
            {
                break;
            }
            select_area_id = i * DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_PIC_ROW_NUM + cur_x;
            if(select_area_id != phandle->pre_select_area_id)
            {
                phandle->pre_select_area_id = select_area_id;
                if(phandle->select_table[phandle->plist[i].img_pos[cur_x]])
                {
                    divoom_print("cacel select: %d\n", phandle->plist[i].img_pos[cur_x]);
                    lv_obj_add_flag(phandle->plist[i].select_obj[cur_x], LV_OBJ_FLAG_HIDDEN);
                    lv_obj_set_style_img_opa(phandle->plist[i].img_obj[cur_x], 255, 0);
                    phandle->select_table[phandle->plist[i].img_pos[cur_x]] = 0;
                }
                else
                {
                    divoom_print("select: %d\n", phandle->plist[i].img_pos[cur_x]);
                    lv_obj_clear_flag(phandle->plist[i].select_obj[cur_x], LV_OBJ_FLAG_HIDDEN);
                    lv_obj_set_style_img_opa(phandle->plist[i].img_obj[cur_x], DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_PIC_IMAGE_SELECT_OPA, 0);
                    //lv_obj_move_foreground(phandle->plist[i].select_obj[cur_x]);
                    phandle->select_table[phandle->plist[i].img_pos[cur_x]] = 1;
                }
            
            }
        }
    }
}

void divoom_lvgl_photo_add_picture_to_album_cancel_all_select(Pdivoom_lvgl_photo_add_picture_to_album_ctrl phandle)
{

    for(int i = 0; i < DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_PIC_LIST_NUM; i++)
    {
        for(int j = 0; j < DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_PIC_ROW_NUM; j++)
        {
            lv_obj_add_flag(phandle->plist[i].select_obj[j], LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(phandle->plist[i].hide_flag_obj[j], LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_style_img_opa(phandle->plist[i].img_obj[j], 255, 0);
                
        }
    }
}

static void divoom_lvgl_photo_add_picture_to_album_event_cb(lv_event_t * e)
{
	lv_point_t p1;
	lv_event_code_t code = lv_event_get_code(e);
    Pdivoom_lvgl_photo_add_picture_to_album_ctrl phandle = (Pdivoom_lvgl_photo_add_picture_to_album_ctrl)lv_event_get_user_data(e);


	switch(code)
	{
        case LV_EVENT_KEY:
            //divoom_print("event: %d\n", code, phandle->move_steps);
            if(phandle->move_steps == 0)
            {
                if(phandle->operate_btn_flag)
                {
                }
                else
                {
                }
            }
            break;
            
		case LV_EVENT_PRESSED:
            phandle->pre_select_area_id = -1;
    	    lv_indev_get_point(lv_indev_get_act(), &p1);
            if(phandle->timer)
            {
                lv_timer_pause(phandle->timer);                
            }
            //divoom_print("event: %d, %d, %d\n", code, p1.x, p1.y);
            phandle->press_time = divoom_system_get_tickms();
            phandle->press_y = p1.y;
            phandle->press_x = p1.x;
            phandle->start_y = p1.y;
            phandle->start_x = p1.x;
            phandle->move_steps = 0;
            if(phandle->operate_btn_flag)
            {
                //divoom_lvgl_photo_add_picture_to_album_change_items(phandle, &p1);
                if(phandle->timer)
                {
        			lv_timer_del(phandle->timer);
                    phandle->timer = NULL;
                }
            }
			break;
		case LV_EVENT_PRESSING:
    		lv_indev_get_point(lv_indev_get_act(), &p1);
            {
                int move_steps;
                
                move_steps = p1.y - phandle->press_y;
                
                if(move_steps + phandle->base_pos < 0)
                {
                    move_steps = 0 - phandle->base_pos;
                }
                else if(move_steps + phandle->base_pos > phandle->max_pos)
                {
                    move_steps = phandle->max_pos - phandle->base_pos;
                }
                //divoom_print("event: %d, %d, %d\n", code, p1.x, p1.y);
    	        divoom_lvgl_photo_add_picture_to_album_update_y(phandle, move_steps);
                phandle->move_steps += move_steps;
                phandle->press_y = p1.y;
                if(phandle->operate_btn_flag)
                {
                    int move_x = p1.x - phandle->press_x;
                    if(abs(move_x) > abs(move_steps) + 5)//这五个点算是误差
                    {
                        divoom_lvgl_photo_add_picture_to_album_change_items(phandle, &p1);
                    }
                }
                phandle->press_x = p1.x;
            }
			break;

        case LV_EVENT_LONG_PRESSED:
            break;
            
		case LV_EVENT_RELEASED:
            if(phandle->operate_btn_flag)
            {
                if(phandle->timer)
                {
        			lv_timer_del(phandle->timer);
                    phandle->timer = NULL;
                }
                lv_indev_get_point(lv_indev_get_act(), &p1);
                if(phandle->start_x == p1.x && phandle->start_y == p1.y)
                {
                    divoom_lvgl_photo_add_picture_to_album_change_items(phandle, &p1);
                }
            }
            else
            {
                int abs_move_steps,press_time = divoom_system_get_tickms() - phandle->press_time;
                #if 1
                //divoom_print("event: %d, %d\n", code, phandle->move_steps);
                phandle->timer_tick = 0;

                abs_move_steps = phandle->move_steps>0?phandle->move_steps:(0-phandle->move_steps);
                //这里要再实际判断一下滑动效果，如果是滑动，将创建定时器滑动
                //divoom_print("move speed: %d, %d, %d\n",abs_move_steps, abs_move_steps * 1000 / press_time, DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_CONT_WIDTH);
                if(abs_move_steps == 0)
                {
                    if(phandle->timer)
                    {
            			lv_timer_del(phandle->timer);
                        phandle->timer = NULL;
                    }
                    else
                    {
            	        lv_indev_get_point(lv_indev_get_act(), &p1);
                    }
                }
                else if(abs_move_steps * 1000 / press_time > DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_CONT_WIDTH)
                {  
                    if(phandle->timer == NULL)
                    {
                        //如果定时器没有开，将开启
                        phandle->speed_up_speed = abs_move_steps *(DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_CONT_WIDTH *5)/200;
                    }
                    else
                    {
                        phandle->speed_up_speed = phandle->speed_up_speed * 6/5;
                    }      
                    if(phandle->move_steps > 0)
                    {
                        phandle->dest_pos = phandle->base_pos + phandle->speed_up_speed;
                        if(phandle->dest_pos >= phandle->max_pos)
                        {
                            phandle->dest_pos = phandle->max_pos;
                        }
                    }
                    else
                    {
                        phandle->dest_pos = phandle->base_pos - phandle->speed_up_speed;
                        if(phandle->dest_pos < 0)
                        {
                            phandle->dest_pos = 0;
                        }
                    }
                    if(phandle->timer == NULL)
                    {
                        //如果定时器没有开，将开启
                        phandle->timer = lv_timer_create(divoom_lvgl_photo_add_picture_to_album_time_task, LV_DISP_DEF_REFR_PERIOD, phandle);
                    }
        			lv_timer_resume(phandle->timer);
                }
                else if(phandle->timer == NULL)
                {
                }
                else
                {
        			lv_timer_del(phandle->timer);
                    phandle->timer = NULL;
                }
            }
            #endif
			break;
        default:
            break;
	}
}

void divoom_lvgl_photo_add_picture_to_album_update_all_pic_info(Pdivoom_lvgl_photo_add_picture_to_album_ctrl phandle, int del_num)
{
    int i, j;
    
    phandle->max_pos = ((phandle->pic_num + DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_PIC_ROW_NUM - 1)/DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_PIC_ROW_NUM) * DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_CONT_WIDTH;
    if(phandle->max_pos <= DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_CONTENT_H)
    {
        phandle->max_pos = 0;
    }
    else
    {
        phandle->max_pos -= DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_CONTENT_H;
    }
    divoom_print("del num: %d\n", del_num);
    if(del_num)
    {
        for(i = 0; i <DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_PIC_LIST_NUM; i++)
        {
            for(j = 0; j < DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_PIC_ROW_NUM; j++)
            {
                if(phandle->plist[i].img_pos[j] > 0 && phandle->plist[i].img_pos[j] < del_num)
                {
                    del_num = phandle->plist[i].img_pos[j];
                }
            }
        }

        for(i = 0; i <DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_PIC_LIST_NUM; i++)
        {
            for(j = 0; j < DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_PIC_ROW_NUM; j++)
            {
                if(phandle->plist[i].img_pos[j] > 0)
                {
                    phandle->plist[i].img_pos[j] -= del_num;
                    if(phandle->plist[i].img_pos[j] >= phandle->pic_num)
                    {
                        phandle->plist[i].img_pos[j] = 0;
                    }
                }
            }
        }
    }

    for(i = 0; i <DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_PIC_LIST_NUM; i++)
    {
        for(j = 0; j < DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_PIC_ROW_NUM; j++)
        {
            //如果在这删除位置后面的图像，要全部重新加载
            if(phandle->plist[i].img_pos[j]>=0)
            {           
                lv_img_dsc_t *image_tmp_handle = divoom_queue_pop(phandle->image_queue);
                memcpy(image_tmp_handle, phandle->plist[i].image_data[j], sizeof(lv_img_dsc_t));
                phandle->plist[i].img_id[j] = divoom_pic_album_manage_get_disp_order_pic_id(phandle->pget_pic_handle,phandle->plist[i].img_pos[j], NULL);
                if(divoom_lvgl_photo_add_picture_to_album_read_pic(0, phandle->plist[i].img_pos[j],phandle->plist[i].img_id[j], 0, 
                    (uint8 *)image_tmp_handle->data,  divoom_lvgl_photo_add_picture_to_album_update_finish_read_over_callback, phandle->plist[i].img_obj[j]))
                {
                    
                    //divoom_print("update image!%d, %d, %d\n", i, j,phandle->plist[i].img_id[j]);
                    lv_img_set_src(phandle->plist[i].img_obj[j], image_tmp_handle);
                    divoom_queue_push(phandle->image_queue, phandle->plist[i].image_data[j]);
                    phandle->plist[i].image_data[j] = image_tmp_handle;
                    phandle->plist[i].img_update_flag[j] = 0;
                }
                else
                {
                    divoom_queue_push(phandle->image_queue, image_tmp_handle);
                    phandle->plist[i].img_update_flag[j] = 1;
                    //divoom_print("wait update image!%d, %d\n", i, j,phandle->plist[i].img_id[j]);
                }
            }
            else
            {
                phandle->plist[i].img_id[j] = 0;
                lv_obj_add_flag(phandle->plist[i].img_obj[j], LV_OBJ_FLAG_HIDDEN);
            }
        }
    }
}

static void divoom_lvgl_photo_add_picture_to_album_btn_event_cb(lv_event_t * e)
{
    int i, active_id;
    lv_obj_t *cur_obj;
    lv_group_t * cur_group_obj = lv_obj_get_group(lv_event_get_target_obj(e));
    Pdivoom_lvgl_photo_add_picture_to_album_ctrl phandle = (Pdivoom_lvgl_photo_add_picture_to_album_ctrl)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);
    cur_obj = lv_event_get_target_obj(e);
    if(code == LV_EVENT_KEY)
    {
        int cur_pos;

        if(phandle->return_btn_obj == (void *)cur_obj)
        {
            divoom_menu_control_return_pre();
            return;
        }
        else if(phandle->select_btn_obj == (void *)cur_obj)
        {
            int select_num = 0, i;
            uint32 *pitem_list = NULL;
    
    
            for(i = 0; i < phandle->pic_num; i++)
            {
                if(phandle->select_table[i])
                {
                    divoom_print("select: id %d, %d\n", i,select_num);
                    select_num ++;
                }
            }
            divoom_print("select: number %d\n", select_num);
            
            pitem_list = divoom_system_malloc(select_num * sizeof(uint32));
            for(i = 0, select_num = 0; i < phandle->pic_num; i++)
            {
                if(phandle->select_table[i])
                {
                    pitem_list[select_num] = divoom_pic_album_manage_get_disp_order_pic_id(phandle->pget_pic_handle,i, NULL);
                    select_num ++;
                }
            }
            divoom_pic_phone_manage_add_piclist_to_album(pitem_list, select_num, phandle->album_id);            
            divoom_system_free(pitem_list);            
            divoom_menu_control_return_pre();
        }
    }
}

void *divoom_lvgl_photo_add_picture_to_album_init(void *user_data, int menu_id)
{
    int i, j, image_w, image_h;
    Pdivoom_lvgl_photo_add_picture_to_album_ctrl phandle;
    lv_obj_t *btn_obj, *label_obj;
    Pdivoom_pic_album_manage_list_item pdisp_info;
    

    phandle = divoom_system_malloc(sizeof(divoom_lvgl_photo_add_picture_to_album_ctrl));
    phandle->menu_id = menu_id;
    if (user_data)
    {
        phandle->album_id = *(int *)user_data;
        divoom_system_free(user_data);
    }
    else
    {
        phandle->album_id = divoom_pic_album_manage_get_album_id_from_type(DIVOOM_PIC_ALBUM_TYPE_ALL);
    }
    phandle->first_load_flag = 1;
    phandle->image_queue = divoom_queue_init(100, __func__, __LINE__);
    phandle->update_image_queue = divoom_queue_init(100, __func__, __LINE__);
    lv_img_cache_set_size(20);
    for(i = 0; i < 100; i++)
    {
        lv_img_dsc_t *img_info = divoom_system_malloc(sizeof(lv_img_dsc_t));
        divoom_queue_push(phandle->image_queue, img_info);
    }
    pdivoom_lvgl_photo_add_picture_to_album_handle = phandle;
    phandle->main_obj = divoom_astrootoo_ui_create_root(&phandle->ui);
    divoom_astrootoo_ui_create_subpage_header(phandle->main_obj, &phandle->ui, "AddPicToAlbum", phandle->menu_id);
    phandle->image_main_cout_obj = lv_obj_create(phandle->main_obj);
    lv_obj_add_style(phandle->image_main_cout_obj, &phandle->ui.content_style, 0);
	lv_obj_set_size(phandle->image_main_cout_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_CONT_WIDTH*DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_PIC_LIST_NUM);
    lv_obj_set_pos(phandle->image_main_cout_obj, 0, DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_GRID_BASE_Y);
	lv_obj_clear_flag(phandle->image_main_cout_obj, LV_OBJ_FLAG_SCROLLABLE);
    
    //默认按照片时间的从新到旧排序
    phandle->pget_pic_handle = divoom_pic_album_manage_create_disp_order_handle(divoom_pic_album_manage_get_album_id_from_type(DIVOOM_PIC_ALBUM_TYPE_ALL), DIVOOM_PIC_PHOTO_SLIDE_DISP_SAVE_TIME, 1, 1);
    phandle->pic_num = divoom_pic_album_manage_get_disp_order_pic_num(phandle->pget_pic_handle);
    phandle->select_table = divoom_system_malloc(phandle->pic_num);
    phandle->read_pic_pos = 0;
    phandle->max_pos = ((phandle->pic_num + DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_PIC_ROW_NUM - 1)/DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_PIC_ROW_NUM) * DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_CONT_WIDTH;
    if(phandle->max_pos <= DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_CONTENT_H)
    {
        phandle->max_pos = 0;
    }
    else
    {
        phandle->max_pos -= DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_CONTENT_H;
    }
    phandle->base_pos = phandle->max_pos;
    lv_img_cache_invalidate_src(NULL);
    for(i = 0; i < DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_PIC_LIST_NUM; i++)
    {
        phandle->plist[i].cout = lv_obj_create(phandle->image_main_cout_obj);
        lv_obj_add_style(phandle->plist[i].cout, &phandle->ui.content_style, 0);
    	lv_obj_clear_flag(phandle->plist[i].cout, LV_OBJ_FLAG_SCROLLABLE);
    	lv_obj_set_size(phandle->plist[i].cout, DIVOOM_SCREEN_WIDTH, DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_CONT_WIDTH);
        lv_obj_set_pos(phandle->plist[i].cout, 0, i*DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_CONT_WIDTH);
        phandle->plist[i].pos_y = i*DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_CONT_WIDTH;
        if(phandle->read_pic_pos >= phandle->pic_num)
        {
            //如果所有图片全部读完了，就隐藏控件
            lv_obj_add_flag(phandle->plist[i].cout, LV_OBJ_FLAG_HIDDEN);
        }
        for(j = 0; j < DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_PIC_ROW_NUM; j++)
        {
            phandle->plist[i].img_update_flag[j] = 0;
            phandle->plist[i].img_obj[j] = lv_img_create(phandle->plist[i].cout);
            lv_obj_set_pos(phandle->plist[i].img_obj[j],
                DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_PIC_PAD + j * DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_CONT_WIDTH, 0);
            lv_obj_set_size(phandle->plist[i].img_obj[j], DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_PIC_WIDTH, DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_PIC_WIDTH);
            phandle->plist[i].image_data[j] = divoom_system_malloc(sizeof(lv_img_dsc_t));
            divoom_lv_img_dsc_init_fmt(phandle->plist[i].image_data[j],
                DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_PIC_WIDTH,
                DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_PIC_WIDTH,
                LV_COLOR_FORMAT_RGB565A8, NULL);
            phandle->plist[i].image_data[j]->data =
                divoom_system_malloc(phandle->plist[i].image_data[j]->data_size);
            if(phandle->read_pic_pos < phandle->pic_num)
            {
                if(i > 0)
                {
                    //第一行，跳过，因为第一行不在可视范围
                    phandle->plist[i].img_pos[j] = phandle->read_pic_pos;
                    phandle->plist[i].img_id[j] = divoom_pic_album_manage_get_disp_order_pic_id(phandle->pget_pic_handle,phandle->read_pic_pos, NULL);
                    //divoom_print("read info: %d, %d, %d, %d\n", i, j, phandle->read_pic_pos, pitem_info->id);
                    phandle->read_pic_pos ++;
                }
                else
                {
                    phandle->plist[i].img_pos[j] = j;
                    phandle->plist[i].img_id[j] = divoom_pic_album_manage_get_disp_order_pic_id(phandle->pget_pic_handle,j, NULL);
                    //divoom_print("read info: %d, %d, %d, %d\n", i, j, j, pitem_info->id);
                }
            }
            else
            {
                divoom_print("over image and empty: %d, %d\n", i ,j);
                phandle->plist[i].img_id[j] = 0;
                lv_obj_add_flag(phandle->plist[i].img_obj[j], LV_OBJ_FLAG_HIDDEN);
            }
            if(phandle->plist[i].img_id[j])
            {
                divoom_lvgl_photo_add_picture_to_album_read_pic(0, phandle->plist[i].img_pos[j],phandle->plist[i].img_id[j], 1, (uint8 *)phandle->plist[i].image_data[j]->data, NULL, NULL);
                divoom_lv_img_dsc_init_fmt(phandle->plist[i].image_data[j],
                    DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_PIC_WIDTH,
                    DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_PIC_WIDTH,
                    LV_COLOR_FORMAT_RGB565A8,
                    phandle->plist[i].image_data[j]->data);
            }
            lv_img_set_src(phandle->plist[i].img_obj[j], phandle->plist[i].image_data[j]);
            phandle->plist[i].select_obj[j] = lv_img_create(phandle->plist[i].cout);
            lv_obj_set_size(phandle->plist[i].select_obj[j], 20, 20);
            lv_obj_set_pos(phandle->plist[i].select_obj[j],
                DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_PIC_PAD + (j + 1) * DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_CONT_WIDTH - 24,
                DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_PIC_WIDTH - 24);
            lv_obj_add_flag(phandle->plist[i].select_obj[j], LV_OBJ_FLAG_HIDDEN);
            divoom_menu_control_lv_img_set_src(phandle->plist[i].select_obj[j], "confirm.png");
            
        }
        lv_obj_add_event_cb(phandle->plist[i].cout, divoom_lvgl_photo_add_picture_to_album_event_cb,
            LV_EVENT_ALL, phandle);
    }
    
    btn_obj = lv_btn_create(phandle->main_obj);
    phandle->return_btn_obj = btn_obj;
	lv_obj_set_size(btn_obj, 100, 100);
    lv_obj_set_pos(btn_obj, 50, 50);
    label_obj = lv_label_create(btn_obj);
    lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("Return"));
    lv_obj_center(label_obj);
    lv_obj_add_event(btn_obj, divoom_lvgl_photo_add_picture_to_album_btn_event_cb, LV_EVENT_KEY, phandle);
    lv_obj_move_foreground(phandle->return_btn_obj);
    
    btn_obj = lv_btn_create(phandle->main_obj);
    phandle->select_btn_obj = btn_obj;
	lv_obj_set_size(btn_obj, 100, 100);
    lv_obj_set_pos(btn_obj,DIVOOM_SCREEN_WIDTH - 150,  50);
    label_obj = lv_label_create(btn_obj);
    lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("Add"));
    lv_obj_center(label_obj);
    phandle->select_label_obj = label_obj;
    lv_obj_add_event(btn_obj, divoom_lvgl_photo_add_picture_to_album_btn_event_cb, LV_EVENT_KEY, phandle);
    
    lv_obj_move_foreground(phandle->select_btn_obj);
    
    
    phandle->dir = 1;
    for(j = 0; j < DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_PIC_ROW_NUM*2; j++)
    {
        if(phandle->read_pic_pos + j >= phandle->pic_num)
        {
            break;
        }
        divoom_thumb_cache_pre_read(0, divoom_pic_album_manage_get_disp_order_pic_id(phandle->pget_pic_handle,phandle->read_pic_pos + j, NULL));
    }
    phandle->update_image_timer = lv_timer_create(divoom_lvgl_photo_add_picture_to_album_update_image_time_task, 10, phandle);

    return phandle;
}

void divoom_lvgl_photo_add_picture_to_album_disp(void *handle, void *active_obj)
{
    Pdivoom_lvgl_photo_add_picture_to_album_ctrl phandle = (Pdivoom_lvgl_photo_add_picture_to_album_ctrl)handle;
    
    (void)active_obj;
    lv_scr_load(phandle->main_obj);
    lv_group_focus_obj(phandle->main_obj);
    if(phandle->first_load_flag == 0)
    {
        divoom_lvgl_photo_add_picture_to_album_update_all_pic_info(phandle, 0);
    }
    phandle->first_load_flag = 0;
}

void divoom_lvgl_photo_add_picture_to_album_pause(void *handle)
{
    (void)handle;
}

void divoom_lvgl_photo_add_picture_to_album_exit(void *handle)
{
    int i, j;
    Pdivoom_lvgl_photo_add_picture_to_album_ctrl phandle = (Pdivoom_lvgl_photo_add_picture_to_album_ctrl)handle;

    if(phandle->timer)
    {
        lv_timer_del(phandle->timer);
    }

    divoom_thumb_cache_clear_all();
    if(phandle->update_image_timer)
    {
        lv_timer_del(phandle->update_image_timer);
    }
    lv_obj_del(phandle->main_obj);
    pdivoom_lvgl_photo_add_picture_to_album_handle = NULL;
    divoom_pic_album_manage_destory_disp_order_handle(phandle->pget_pic_handle);
    for(i = 0; i < DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_PIC_LIST_NUM; i++)
    {
        for(j = 0; j < DIVOOM_LVGL_PHOTO_ADD_PICTURE_ALBUM_PIC_ROW_NUM; j++)
        {
            divoom_system_free((uint8 *)phandle->plist[i].image_data[j]->data);
            divoom_system_free(phandle->plist[i].image_data[j]);
        }
    }
    divoom_queue_destroy(phandle->image_queue, NULL);
    divoom_queue_destroy(phandle->update_image_queue, NULL);
    if(phandle->select_table)
    {
        divoom_system_free(phandle->select_table);
    }
    divoom_astrootoo_ui_base_reset_styles(&phandle->ui);
    divoom_system_free(phandle);
    
    lv_img_cache_set_size(500); //默认的
}
