#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"

typedef struct _divoom_lvgl_test_image_ctrl
{
    void *main_obj;
    void *image_obj;
    uint8 *image_buf;
    lv_img_dsc_t *image_data;
    lv_timer_t *timer;
}divoom_lvgl_test_image_ctrl, *Pdivoom_lvgl_test_image_ctrl;

Pdivoom_lvgl_test_image_ctrl pdivoom_lvgl_test_image_handle = NULL;

static void divoom_lvgl_test_image_time_task(lv_timer_t *timer)
{
    int image_w = DIVOOM_THUMB_PIC_WIDTH, image_h=  DIVOOM_THUMB_PIC_HIGH;
    static int test_time = 10;
    lv_img_dsc_t *image_data;
    
    divoom_thumb_cache_read_pic(0, test_time, 1, (uint8 *)pdivoom_lvgl_test_image_handle->image_buf, image_w, image_h, NULL, NULL);
    divoom_print("update image: %d, %d, %d\n", test_time, image_w, image_h);
    test_time ++;
    divoom_thumb_cache_pre_read(0, test_time);
    
    //lv_obj_invalidate(lv_disp_get_layer_top(NULL));
    lv_obj_invalidate(pdivoom_lvgl_test_image_handle->image_obj);
    #if 0
    //image_data = divoom_system_malloc(sizeof(lv_img_dsc_t));
    //memcpy(image_data, pdivoom_lvgl_test_image_handle->image_data, sizeof(lv_img_dsc_t));
DIVOOM_LV_IMG_HEADER_INIT(&    image_data->header);
    image_data->header.w = image_w;
    image_data->header.h = image_h;
    image_data->header.cf = LV_COLOR_FORMAT_RGB565A8;
    image_data->data_size = DIVOOM_THUMB_CACHE_PIC_SIZE;
    image_data->data = pdivoom_lvgl_test_image_handle->image_buf;
    lv_img_set_src(pdivoom_lvgl_test_image_handle->image_obj, image_data);
    //divoom_system_free(pdivoom_lvgl_test_image_handle->image_data);
    pdivoom_lvgl_test_image_handle->image_data = image_data;
    #endif
    return;

    if(test_time >= 3)
    {
        test_time = 0;
    }
    if(test_time == 0)
    {
        lv_img_set_src(pdivoom_lvgl_test_image_handle->image_obj, "A:/userdata/1.png");
    }
    else if(test_time == 1)
    {
        lv_img_set_src(pdivoom_lvgl_test_image_handle->image_obj, "A:/userdata/2.png");
    }
    else
    {
        lv_img_set_src(pdivoom_lvgl_test_image_handle->image_obj, "A:/userdata/3.png");
    }
}

void *divoom_lvgl_test_image_init(void *user_data)
{
    int image_w = DIVOOM_THUMB_PIC_WIDTH, image_h = DIVOOM_THUMB_PIC_HIGH;
    pdivoom_lvgl_test_image_handle = divoom_system_malloc(sizeof(divoom_lvgl_test_image_ctrl));

    pdivoom_lvgl_test_image_handle->main_obj = lv_obj_create(NULL);
    pdivoom_lvgl_test_image_handle->image_obj = lv_img_create(pdivoom_lvgl_test_image_handle->main_obj);
    pdivoom_lvgl_test_image_handle->image_data = divoom_system_malloc(sizeof(lv_img_dsc_t));
DIVOOM_LV_IMG_HEADER_INIT(&    pdivoom_lvgl_test_image_handle->image_data->header);
    pdivoom_lvgl_test_image_handle->image_data->header.w = DIVOOM_THUMB_PIC_WIDTH;
    pdivoom_lvgl_test_image_handle->image_data->header.h = DIVOOM_THUMB_PIC_HIGH;
    pdivoom_lvgl_test_image_handle->image_data->header.cf = LV_COLOR_FORMAT_RGB565A8;
    pdivoom_lvgl_test_image_handle->image_data->data_size = DIVOOM_THUMB_CACHE_PIC_SIZE;
    pdivoom_lvgl_test_image_handle->image_buf = divoom_system_malloc(DIVOOM_THUMB_CACHE_PIC_SIZE);
    pdivoom_lvgl_test_image_handle->image_data->data = pdivoom_lvgl_test_image_handle->image_buf;
    divoom_thumb_cache_read_pic(0, 3, 1, (uint8 *)pdivoom_lvgl_test_image_handle->image_buf, image_w, image_h, NULL, NULL);
    pdivoom_lvgl_test_image_handle->image_data->header.w = image_w;
    pdivoom_lvgl_test_image_handle->image_data->header.h = image_h;
    lv_img_set_src(pdivoom_lvgl_test_image_handle->image_obj, pdivoom_lvgl_test_image_handle->image_data);
    divoom_thumb_cache_pre_read(0, 10);
    pdivoom_lvgl_test_image_handle->timer = lv_timer_create(divoom_lvgl_test_image_time_task, 2000, pdivoom_lvgl_test_image_handle);

    return pdivoom_lvgl_test_image_handle;
}

void divoom_lvgl_test_image_disp(void *handle, void *active_obj)
{
    lv_scr_load(pdivoom_lvgl_test_image_handle->main_obj);
}

void divoom_lvgl_test_image_exit(void *handle)
{
    lv_obj_del(pdivoom_lvgl_test_image_handle->main_obj);
    lv_timer_del(pdivoom_lvgl_test_image_handle->timer);
    divoom_system_free(pdivoom_lvgl_test_image_handle->image_buf);
    divoom_system_free(pdivoom_lvgl_test_image_handle->image_data);
    divoom_system_free(pdivoom_lvgl_test_image_handle);
    pdivoom_lvgl_test_image_handle = NULL;
}

