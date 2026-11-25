#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"
#include "divoom_popup_menu.h"
#include "divoom_pixel_format.h"



typedef struct _divoom_popup_menu_item
{
    int width;
    int high;
    int pre_play_time;
    void *user_data;
    void *main_obj;
    void *image_obj;
    void *bg_image_obj;
    lv_style_t popup_type;
    lv_img_dsc_t notice_image_buf;
    lv_img_dsc_t bg_image_buf;
    int refresh_flag;
    lv_timer_t *time_handle;
    DIVOOM_POPUP_MENU_CANCEL_CALLBACK *pexit_callback;
    DIVOOM_POPUP_MENU_KEY_CALLBACK *pkey_callback;
}divoom_popup_menu_item, *Pdivoom_popup_menu_item;


typedef struct _divoom_popup_menu_ctrl
{
    void *lock;
    Pdivoom_popup_menu_item prun_menu_item;
    struct _lv_timer_t * event_time_handle;
}divoom_popup_menu_ctrl, *Pdivoom_popup_menu_ctrl;


static Pdivoom_popup_menu_ctrl pdivoom_popup_menu_handle = NULL;

/*********************************************************************
  Function:        divoom_popup_menu_init
  Description:     初始化弹窗模块
  Input:           none
  Return:          NONE
**********************************************************************/

void divoom_popup_menu_init(void)
{
    pdivoom_popup_menu_handle = divoom_system_malloc(sizeof(divoom_popup_menu_ctrl));

    pdivoom_popup_menu_handle->prun_menu_item = NULL;
    pdivoom_popup_menu_handle->lock = divoom_system_create_lock();
}


static void divoom_popup_menu_event_cb(lv_event_t * e)
{
    Pdivoom_popup_menu_item phandle = (Pdivoom_popup_menu_item)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_KEY)
    {
        char c = *((char *)lv_event_get_param(e));

        if(phandle->pkey_callback)
        {
            phandle->pkey_callback(phandle->user_data, c);
        }
    }
}

void divoom_popup_menu_start_lvgl_handle(void *user_data)
{
    Pdivoom_popup_menu_item pmenu_item = (Pdivoom_popup_menu_item)user_data;
    
    lv_style_init(&pmenu_item->popup_type);
    lv_style_set_pad_all(&pmenu_item->popup_type, 0);
    lv_style_set_pad_gap(&pmenu_item->popup_type, 0);
    lv_style_set_border_width(&pmenu_item->popup_type, 0);
    lv_style_set_outline_width(&pmenu_item->popup_type, 0);
    lv_style_set_border_side(&pmenu_item->popup_type, LV_BORDER_SIDE_NONE);

    pmenu_item->main_obj = lv_btn_create(lv_scr_act());
    lv_obj_remove_style_all(pmenu_item->main_obj);
    lv_obj_add_style(pmenu_item->main_obj, &pmenu_item->popup_type, 0);
    lv_obj_set_size(pmenu_item->main_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);


    lv_obj_t *back_img_obj = lv_img_create(pmenu_item->main_obj);
    lv_img_set_src(back_img_obj, &pmenu_item->bg_image_buf);
    lv_obj_center(back_img_obj);


    pmenu_item->image_obj = lv_img_create(pmenu_item->main_obj);
    lv_obj_center(pmenu_item->image_obj);    
    lv_img_set_src(pmenu_item->image_obj, &pmenu_item->notice_image_buf);

    lv_obj_add_event_cb(pmenu_item->main_obj, divoom_popup_menu_event_cb, LV_EVENT_KEY, pmenu_item);

}

void divoom_popup_menu_update_flush(struct _lv_timer_t * para)
{
    
     Pdivoom_popup_menu_item phandle = (Pdivoom_popup_menu_item)lv_timer_get_user_data(para);

     if(phandle->refresh_flag)
     {
        lv_obj_invalidate(phandle->image_obj);
        phandle->refresh_flag = 0 ;
     }

}

/*********************************************************************
  Function:        divoom_popup_menu_create
  Description:     创建弹窗提醒
  Input:           width
                   high
				   opa 	透明度 0x0~0xFF

                   pexit_callbck
                   user_data
                   pkey_callback
                   main_obj         父窗口句柄
                   back_flag        是否增加透明度底图
  Return:          NONE
**********************************************************************/

void *divoom_popup_menu_create(int width, int high, int opa, DIVOOM_POPUP_MENU_CANCEL_CALLBACK *pexit_callbck, void *user_data,DIVOOM_POPUP_MENU_KEY_CALLBACK *pkey_callback)
{
    Pdivoom_popup_menu_item pmenu_item;
    uint32 bg_px_count = 160U * 128U;
    uint32 notice_px_count = (uint32)width * (uint32)high;

    divoom_system_get_lock(pdivoom_popup_menu_handle->lock);
    if(pdivoom_popup_menu_handle->prun_menu_item)
    {
        pmenu_item = pdivoom_popup_menu_handle->prun_menu_item;
        divoom_system_free_lock(pdivoom_popup_menu_handle->lock);
        return pmenu_item;
    }

    pmenu_item = divoom_system_malloc(sizeof(divoom_popup_menu_item));
    if(pmenu_item == NULL)
    {
        divoom_system_free_lock(pdivoom_popup_menu_handle->lock);
        return NULL;
    }

    pmenu_item->width = width;
    pmenu_item->high = high;
    pmenu_item->user_data = user_data;
    pmenu_item->pexit_callback = pexit_callbck;
    pmenu_item->pkey_callback = pkey_callback;
    
    pmenu_item->bg_image_buf.data =
        divoom_system_malloc(divoom_pixel_format_size(160, 128, LV_COLOR_FORMAT_RGB565A8));
    divoom_rgb565a8_clear((uint8 *)pmenu_item->bg_image_buf.data, bg_px_count, 0, (uint8)opa);
    divoom_lv_img_dsc_init_fmt(&pmenu_item->bg_image_buf, 160, 128,
        LV_COLOR_FORMAT_RGB565A8, pmenu_item->bg_image_buf.data);
    
    pmenu_item->notice_image_buf.data =
        divoom_system_malloc(divoom_pixel_format_size((uint32)width, (uint32)high,
            LV_COLOR_FORMAT_RGB565A8));
    divoom_rgb565a8_clear((uint8 *)pmenu_item->notice_image_buf.data,
        notice_px_count, 0, 0xFF);
    divoom_lv_img_dsc_init_fmt(&pmenu_item->notice_image_buf, pmenu_item->width,
        pmenu_item->high, LV_COLOR_FORMAT_RGB565A8,
        pmenu_item->notice_image_buf.data);
    divoom_menu_control_lvgl_ctrl(divoom_popup_menu_start_lvgl_handle, pmenu_item);
    //lv_group_focus_obj(pmenu_item->main_obj);
    divoom_print("image info: %d, %d\n", width, high);

    pmenu_item->refresh_flag =1;
    pmenu_item->time_handle =  lv_timer_create(divoom_popup_menu_update_flush, LV_DISP_DEF_REFR_PERIOD, pmenu_item);

    pdivoom_popup_menu_handle->prun_menu_item = pmenu_item;
    divoom_system_free_lock(pdivoom_popup_menu_handle->lock);
    return pmenu_item;
}

/*********************************************************************
  Function:        divoom_popup_menu_set_actice_obj
  Description:     设置激活控件
  Input:           phandle
                   buf
                   play_time
  Return:          NONE
**********************************************************************/

void divoom_popup_menu_set_actice_obj(void *phandle)
{
    Pdivoom_popup_menu_item pmenu_item = (Pdivoom_popup_menu_item)phandle;

    if(pmenu_item == NULL)
    {
        divoom_print("send! err\n");
        return;
    }
    lv_group_focus_obj(pmenu_item->main_obj);

}


/*********************************************************************
  Function:        divoom_popup_menu_update_disp_buf
  Description:     更指定的弹窗提醒的显示缓冲区
  Input:           phandle
                   buf
                   play_time
  Return:          NONE
**********************************************************************/

void divoom_popup_menu_update_disp_buf(void *phandle, void *buf, int play_time)
{
    Pdivoom_popup_menu_item pmenu_item = (Pdivoom_popup_menu_item)phandle;


    if(pmenu_item == NULL)
    {
        divoom_print("send! err\n");
        return;
    }

    memcpy((void *)pmenu_item->notice_image_buf.data, buf,
        (size_t)pmenu_item->width * (size_t)pmenu_item->high *
        DIVOOM_APP_RGB565_BYTES_PER_PX);
    pmenu_item->refresh_flag = 1;
    //lv_obj_invalidate(pmenu_item->image_obj);
}

int divoom_popup_menu_compare_item(void *item1, void *item2)
{
    return item1 == item2?0:1;
}

void divoom_popup_menu_del_lvgl_handle(void *user_data)
{
    Pdivoom_popup_menu_item pmenu_item = (Pdivoom_popup_menu_item)user_data;

    lv_obj_del(pmenu_item->main_obj);

}


/*********************************************************************
  Function:        divoom_popup_menu_destroy
  Description:     将清除指定的弹窗提醒
  Input:           none
  Return:          NONE
**********************************************************************/

void divoom_popup_menu_destroy(void *phandle)
{
    Pdivoom_popup_menu_item pmenu_item = (Pdivoom_popup_menu_item)phandle;

    if(pmenu_item == NULL)
    {
        return;
    }

    divoom_system_get_lock(pdivoom_popup_menu_handle->lock);
    pmenu_item = pdivoom_popup_menu_handle->prun_menu_item;
    if(pmenu_item  == phandle)
    {
        lv_timer_del(pmenu_item->time_handle);
        divoom_menu_control_lvgl_ctrl(divoom_popup_menu_del_lvgl_handle, pmenu_item);
        divoom_system_free((void *)pmenu_item->bg_image_buf.data);
        divoom_system_free((void *)pmenu_item->notice_image_buf.data);
        divoom_system_free(pmenu_item);
        pdivoom_popup_menu_handle->prun_menu_item = NULL;
    }
    divoom_system_free_lock(pdivoom_popup_menu_handle->lock);
}

/*********************************************************************
  Function:        divoom_popup_menu_destroy_all
  Description:     将清除所有的弹窗提醒
  Input:           none
  Return:          NONE
**********************************************************************/

void divoom_popup_menu_destroy_all(void)
{
    Pdivoom_popup_menu_item pmenu_item;

    divoom_system_get_lock(pdivoom_popup_menu_handle->lock);
    if(pdivoom_popup_menu_handle->prun_menu_item)
    {
        pmenu_item = pdivoom_popup_menu_handle->prun_menu_item;
        pdivoom_popup_menu_handle->prun_menu_item = NULL;
        if(pmenu_item)
        {
            lv_timer_del(pmenu_item->time_handle);
            if(pmenu_item->pexit_callback)
            {
                pmenu_item->pexit_callback(pmenu_item->user_data);
            }
            lv_obj_del(pmenu_item->main_obj);
            divoom_system_free((void *)pmenu_item->notice_image_buf.data);
            divoom_system_free(pmenu_item);
        }
    }
    divoom_system_free_lock(pdivoom_popup_menu_handle->lock);
}
