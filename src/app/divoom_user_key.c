#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include "divoom.h"
#include "divoom_pt_ui.h"
#include "divoom_idle_poweroff.h"
#include "lvgl.h"
/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/

typedef struct _divoom_user_key_info
{
    int key_status;     //DIVOOM_PRESS_TYPE
    int key_id;         //DIVOOM_KEY_VALUE
}divoom_user_key_info, *Pdivoom_user_key_info;

typedef struct _divoom_user_key_item
{
    int key_id;
    int status;     //3:长长按状态；2:长按状态；1：短按状态；0：松开
    int file_id;
    int send_key_info_time; //前一次发送按键消息的时间
    int press_time;   //按键开始按下的时间
}divoom_user_key_item, *Pdivoom_user_key_item;

#define DIVOOM_USER_KEY_DEV_MAX 3

typedef struct _divoom_user_key_ctrl
{
    void *key_queue;
    divoom_user_key_item key_list[DIVOOM_USER_KEY_DEV_MAX];
    int key_button;    
    int key_val;
}divoom_user_key_ctrl, *Pdivoom_user_key_ctrl;


static Pdivoom_user_key_ctrl pdivoom_user_key_handle = NULL;
/**********************
 *      MACROS
 **********************/
static void divoom_user_key_queue_key(int key_id, int key_status, int reset_show_idle)
{
    Pdivoom_user_key_info pkey_info;

    if (divoom_product_test_is_active()) {
        divoom_pt_ui_post_key((uint8_t)key_id, (uint8_t)key_status);
        return;
    }

    if (divoom_onoff_is_poweroff_active())
        return;

    if (!pdivoom_user_key_handle || !pdivoom_user_key_handle->key_queue)
        return;

    divoom_idle_poweroff_reset();
#if DIVOOM_SHOW_MODE
    if (reset_show_idle)
        divoom_light_app_show_clock_rotate_pause();
#endif

    pkey_info = divoom_system_malloc(sizeof(divoom_user_key_info));
    if (!pkey_info)
        return;

    pkey_info->key_id = key_id;
    pkey_info->key_status = key_status;
    divoom_queue_push(pdivoom_user_key_handle->key_queue, pkey_info);
}

void divoom_user_key_send_key_info(int key_id, int key_status)
{
    divoom_user_key_queue_key(key_id, key_status, 1);
}

void divoom_user_key_inject_key_info(int key_id, int key_status)
{
    divoom_user_key_queue_key(key_id, key_status, 0);
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
void* divoom_user_key_read_loop(void *param)
{

    return NULL;
}

/**
 * Initialize the evdev interface
 */
void divoom_user_key_init(void)
{
    pdivoom_user_key_handle = divoom_system_malloc(sizeof(divoom_user_key_ctrl));

    pdivoom_user_key_handle->key_queue = divoom_queue_init(100, __func__, __LINE__);
    pdivoom_user_key_handle->key_val = 0;
    pdivoom_user_key_handle->key_button = LV_INDEV_STATE_REL;
}

/**
 * Get the current position and state of the evdev
 * @param data store the evdev data here
 * @return false: because the points are not buffered, so no more data to be read
 */
void divoom_user_key_read(lv_indev_t *indev, lv_indev_data_t *data)
{
    if(divoom_queue_items(pdivoom_user_key_handle->key_queue))
    {
        Pdivoom_user_key_info pkey_info;
        
        pkey_info = divoom_queue_pop(pdivoom_user_key_handle->key_queue);
        if(pkey_info)
        {
            data->key = (pkey_info->key_status << 8) | pkey_info->key_id;
            data->state = LV_INDEV_STATE_PR;
            pdivoom_user_key_handle->key_val = data->key;
            if(pkey_info->key_status == DIVOOM_PRESS_UP || pkey_info->key_status == DIVOOM_PRESS_SHORT)
            {
                pdivoom_user_key_handle->key_button = LV_INDEV_STATE_REL;
            }
            else
            {
                pdivoom_user_key_handle->key_button = LV_INDEV_STATE_PR;
            }
            //divoom_print("send lvgl: %x, %d, %d, %d\n", data->key, data->state, pkey_info->key_id, pkey_info->key_status);
            divoom_system_free(pkey_info);
            return;
        }
        
    }
    if (lv_indev_get_type(indev) == LV_INDEV_TYPE_KEYPAD)
    {
        /* No data retrieved */
        data->key = pdivoom_user_key_handle->key_val;
        data->state = pdivoom_user_key_handle->key_button;
        return ;
    }
}

