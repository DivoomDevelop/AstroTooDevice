#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"
#include "divoom_system_msg_notice.h"
#include "divoom_astrootoo_theme.h"
#include "divoom_lvgl_top_control.h"

#define DIVOOM_SYSTEM_MSG_NOTICE_SEP_DOT 28

#define DIVOOM_SYSTEM_MSG_NOTICE_MAX_NUM 40

typedef struct _divoom_system_msg_notice_item
{
    int  x;
    int  y;
    int  w;
    int  h;
    void *lvgl_obj;
    void *para;
    divoom_system_msg_notice_callback *pcallback;
    divoom_system_msg_notice_pre_key_cb *pre_key_cb;
}divoom_system_msg_notice_item, *Pdivoom_system_msg_notice_item;


typedef struct _divoom_system_msg_notice_ctrl
{
    void *main_obj;
    void *pdirect_handle;
    int acticve_num;
    int high;
    lv_style_t btn_normal_type;
    lv_style_t btn_press_type;
    divoom_system_msg_notice_item plist[DIVOOM_SYSTEM_MSG_NOTICE_MAX_NUM];
}divoom_system_msg_notice_ctrl, *Pdivoom_system_msg_notice_ctrl;


static Pdivoom_system_msg_notice_ctrl pdivoom_system_msg_notice_handle = NULL;


int divoom_system_msg_notice_find_obj_index(void *lvgl_obj);

void divoom_system_msg_notice_close(int cur_pos)
{
    if(pdivoom_system_msg_notice_handle->main_obj == NULL || pdivoom_system_msg_notice_handle->acticve_num == 0)
    {
        return;
    }
    
    if(cur_pos >= 0)
    {
        int cur_high = pdivoom_system_msg_notice_handle->plist[cur_pos].h + DIVOOM_SYSTEM_MSG_NOTICE_SEP_DOT;

        if(pdivoom_system_msg_notice_handle->acticve_num > 1)
        {
            //如果有多个，将切换焦点和更新控件坐标
            lv_obj_add_flag(pdivoom_system_msg_notice_handle->plist[cur_pos].lvgl_obj, LV_OBJ_FLAG_HIDDEN);
            for(int i = cur_pos + 1; i < pdivoom_system_msg_notice_handle->acticve_num; i++)
            {      
                pdivoom_system_msg_notice_handle->plist[i].y -= cur_high;
                lv_obj_set_y(pdivoom_system_msg_notice_handle->plist[i].lvgl_obj, pdivoom_system_msg_notice_handle->plist[i].y);            
            }
            if(cur_pos != pdivoom_system_msg_notice_handle->acticve_num -1)
            {
                memmove(&pdivoom_system_msg_notice_handle->plist[cur_pos], &pdivoom_system_msg_notice_handle->plist[cur_pos+1], (pdivoom_system_msg_notice_handle->acticve_num - cur_pos -1) * sizeof(divoom_system_msg_notice_item));
            }
            pdivoom_system_msg_notice_handle->acticve_num --;
            if(cur_pos < pdivoom_system_msg_notice_handle->acticve_num)
            {
                divoom_lvgl_top_control_update_top(pdivoom_system_msg_notice_handle, pdivoom_system_msg_notice_handle->plist[cur_pos].lvgl_obj);
            }
            else
            {
                divoom_lvgl_top_control_update_top(pdivoom_system_msg_notice_handle, pdivoom_system_msg_notice_handle->plist[cur_pos - 1].lvgl_obj);
            }
            pdivoom_system_msg_notice_handle->high -= cur_high; 
            lv_obj_set_size(pdivoom_system_msg_notice_handle->main_obj, DIVOOM_SCREEN_WIDTH, pdivoom_system_msg_notice_handle->high);
        }
        else
        {
            lv_obj_add_flag(pdivoom_system_msg_notice_handle->main_obj, LV_OBJ_FLAG_HIDDEN);
            divoom_lvgl_top_control_remove_top(pdivoom_system_msg_notice_handle);
            lv_obj_del_async(pdivoom_system_msg_notice_handle->main_obj);
            pdivoom_system_msg_notice_handle->main_obj = NULL;
            pdivoom_system_msg_notice_handle->acticve_num --;
        }
    }
    else
    {
        divoom_lvgl_top_control_remove_top(pdivoom_system_msg_notice_handle);
        for(int i = 0; i < pdivoom_system_msg_notice_handle->acticve_num; i++)
        {
            if(pdivoom_system_msg_notice_handle->plist[i].pcallback)
            {
                pdivoom_system_msg_notice_handle->plist[i].pcallback(pdivoom_system_msg_notice_handle->plist[i].para, 0);
            }            
        }
        lv_obj_add_flag(pdivoom_system_msg_notice_handle->main_obj, LV_OBJ_FLAG_HIDDEN);
        lv_obj_del_async(pdivoom_system_msg_notice_handle->main_obj);
        pdivoom_system_msg_notice_handle->main_obj = NULL;
        pdivoom_system_msg_notice_handle->acticve_num = 0;
        
    }
}

/*********************************************************************
  Function:        divoom_system_msg_notice_init
  Description:     初始化
  Input:           

  Return:          none
**********************************************************************/

void divoom_system_msg_notice_init(void)
{
    pdivoom_system_msg_notice_handle = divoom_system_malloc(sizeof(divoom_system_msg_notice_ctrl));
    /* 获焦/默认圆角与 Astro 卡片一致（20），避免叠两条时上面变胶囊、下面是卡片 */
    divoom_menu_control_init_btn_stype(&pdivoom_system_msg_notice_handle->btn_press_type, NULL, &pdivoom_system_msg_notice_handle->btn_normal_type, NULL, NULL, NULL, DIVOOM_ASTRO_RADIUS_MD);
	//lv_style_set_bg_color(&pdivoom_system_msg_notice_handle->btn_press_type, lv_color_hex(0xFFFFFF));
    lv_style_set_bg_opa(&pdivoom_system_msg_notice_handle->btn_press_type, LV_OPA_COVER);
    lv_style_set_radius(&pdivoom_system_msg_notice_handle->btn_press_type, DIVOOM_ASTRO_RADIUS_MD);
	//lv_style_set_bg_color(&pdivoom_system_msg_notice_handle->btn_normal_type, lv_color_hex(0xFFFFFF));
    lv_style_set_bg_opa(&pdivoom_system_msg_notice_handle->btn_normal_type, LV_OPA_COVER);
    lv_style_set_radius(&pdivoom_system_msg_notice_handle->btn_normal_type, DIVOOM_ASTRO_RADIUS_MD);
}

void divoom_system_msg_notice_clear_one(void *lvgl_obj, int lvgl_lock)
{
	//退出指定的
	if(lvgl_lock)
    {
        divoom_get_lvgl_lock();
    }

	int cur_pos = divoom_system_msg_notice_find_obj_index(lvgl_obj);

	if(pdivoom_system_msg_notice_handle->plist[cur_pos].pcallback)
	{
		int result = pdivoom_system_msg_notice_handle->plist[cur_pos].pcallback(pdivoom_system_msg_notice_handle->plist[cur_pos].para, 0);
		pdivoom_system_msg_notice_handle->plist[cur_pos].pcallback = NULL;
		if(result)
		{
			divoom_system_msg_notice_close(-1);
		}
		else
		{
			divoom_system_msg_notice_close(cur_pos);
		}
	}
	else
	{
		divoom_system_msg_notice_close(cur_pos);
	}

	if(lvgl_lock)
    {
        divoom_free_lvgl_lock();
    }
}

/*********************************************************************
  Function:        divoom_system_msg_notice_clear_all
  Description:     取消所有的弹窗信息
  Input:           

  Return:          none
**********************************************************************/

void divoom_system_msg_notice_clear_all(int lvgl_lock)
{
    if(lvgl_lock)
    {
        divoom_get_lvgl_lock();
    }
    if(pdivoom_system_msg_notice_handle)
    {
        divoom_system_msg_notice_close(-1);
    }
    if(lvgl_lock)
    {
        divoom_free_lvgl_lock();
    }
}

int divoom_system_msg_notice_find_obj_index(void *lvgl_obj)
{
    for(int i = 0;i < pdivoom_system_msg_notice_handle->acticve_num; i++)
    {
        if(pdivoom_system_msg_notice_handle->plist[i].lvgl_obj == lvgl_obj)
        {
            return i;
        }
    }
    divoom_print("it fails to find!\n");
    return 0;
}

static int divoom_system_msg_notice_find_obj_slot(void *lvgl_obj)
{
    int i;

    if (!lvgl_obj || !pdivoom_system_msg_notice_handle)
        return -1;

    for (i = 0; i < pdivoom_system_msg_notice_handle->acticve_num; i++) {
        if (pdivoom_system_msg_notice_handle->plist[i].lvgl_obj == lvgl_obj)
            return i;
    }
    return -1;
}

void divoom_system_msg_notice_update_para(void *lvgl_obj, void *para)
{
    int cur_pos;

    if (!pdivoom_system_msg_notice_handle || !lvgl_obj)
        return;
    cur_pos = divoom_system_msg_notice_find_obj_slot(lvgl_obj);
    if (cur_pos < 0)
        return;
    pdivoom_system_msg_notice_handle->plist[cur_pos].para = para;
}

void divoom_system_msg_notice_deliver_item_result(void *lvgl_obj, int callback_result)
{
    int cur_pos;
    int cbret;

    if (!pdivoom_system_msg_notice_handle)
        return;
    cur_pos = divoom_system_msg_notice_find_obj_slot(lvgl_obj);
    if (cur_pos < 0)
        return;
    if (pdivoom_system_msg_notice_handle->plist[cur_pos].pcallback) {
        cbret = pdivoom_system_msg_notice_handle->plist[cur_pos].pcallback(
                pdivoom_system_msg_notice_handle->plist[cur_pos].para,
                callback_result);
        pdivoom_system_msg_notice_handle->plist[cur_pos].pcallback = NULL;
        if (cbret)
            divoom_system_msg_notice_close(-1);
        else
            divoom_system_msg_notice_close(cur_pos);
    } else
        divoom_system_msg_notice_close(cur_pos);
}

void divoom_system_msg_notice_set_pre_key_cb(void *lvgl_obj, divoom_system_msg_notice_pre_key_cb *cb)
{
    int cur_pos;

    if (!lvgl_obj || !pdivoom_system_msg_notice_handle)
        return;
    cur_pos = divoom_system_msg_notice_find_obj_slot(lvgl_obj);
    if (cur_pos >= 0)
        pdivoom_system_msg_notice_handle->plist[cur_pos].pre_key_cb = cb;
}

static void divoom_system_msg_notice_event_cb(lv_event_t * e)
{
    int  cur_pos;
    lv_obj_t *cur_obj;
    lv_group_t * cur_group_obj = lv_obj_get_group(lv_event_get_target_obj(e));
    lv_event_code_t code = lv_event_get_code(e);
    cur_obj = lv_event_get_target_obj(e);
    if(code == LV_EVENT_KEY)
    {
        char *key_code = ((char *)lv_event_get_param(e));
        cur_pos = divoom_system_msg_notice_find_obj_slot(cur_obj);

        if (cur_pos >= 0 && pdivoom_system_msg_notice_handle->plist[cur_pos].pre_key_cb) {
            if (pdivoom_system_msg_notice_handle->plist[cur_pos].pre_key_cb(
                        pdivoom_system_msg_notice_handle->plist[cur_pos].para,
                        key_code[0]))
                return;
        }

        switch(key_code[0])
        {            
            case DIVOOM_LVGL_KEY_NEXT:
            case DIVOOM_LVGL_KEY_PRE:
            if(pdivoom_system_msg_notice_handle->acticve_num > 1)
            {
                //移动图片，加载新图片
                cur_pos = divoom_system_msg_notice_find_obj_index(cur_obj);
                /* 新卡在 plist[0]、y 更小（屏幕上方）。TimesVibe 旋钮顺时针=PRE 往更大 index；
                 * AstroToo 方向键应对齐画面：上键往上（index--），下键往下（index++）。 */
                if(key_code[0] == DIVOOM_LVGL_KEY_PRE)
                {
                    if(cur_pos > 0)
                    {
                        cur_pos --;
                    }
                    else
                    {
                        cur_pos = pdivoom_system_msg_notice_handle->acticve_num -1;
                    }
                }
                else
                {
                    if(cur_pos < pdivoom_system_msg_notice_handle->acticve_num -1)
                    {
                        cur_pos ++;
                    }
                    else
                    {
                        cur_pos = 0;
                    }
                }
                divoom_lvgl_top_control_update_top(pdivoom_system_msg_notice_handle, pdivoom_system_msg_notice_handle->plist[cur_pos].lvgl_obj);
                break;
            }
            
            case DIVOOM_LVGL_KEY_ESC:
                divoom_system_msg_notice_close(-1);
                break;
               
            case DIVOOM_LVGL_KEY_ENTER:                
                divoom_system_msg_notice_deliver_item_result(cur_obj, 1);
                break;
            case DIVOOM_LVGL_KEY_HOME:
                //进入操作界面
                break;
                          
        }

    }
}


void *divoom_system_msg_notice_add(int width, int high, divoom_system_msg_notice_callback *pcallback, void *para)
{
    void *lvgl_obj;
    int start_y = 0, cur_high = high + DIVOOM_SYSTEM_MSG_NOTICE_SEP_DOT;

    divoom_get_lvgl_lock();
    if(pdivoom_system_msg_notice_handle->acticve_num == 0)
    {
        pdivoom_system_msg_notice_handle->main_obj = lv_obj_create(lv_layer_top()); //在顶层创建主页面
    	lv_obj_set_style_bg_opa(pdivoom_system_msg_notice_handle->main_obj, 0, 0);
    	lv_obj_set_style_pad_all(pdivoom_system_msg_notice_handle->main_obj, 0, 0);
    	lv_obj_set_style_border_width(pdivoom_system_msg_notice_handle->main_obj, 0, 0);
    	lv_obj_set_style_shadow_width(pdivoom_system_msg_notice_handle->main_obj, 0, 0);
    	lv_obj_set_style_outline_width(pdivoom_system_msg_notice_handle->main_obj, 0, 0);
    	lv_obj_clear_flag(pdivoom_system_msg_notice_handle->main_obj, LV_OBJ_FLAG_SCROLLABLE);
    	lv_obj_set_pos(pdivoom_system_msg_notice_handle->main_obj, 0, 0);
        pdivoom_system_msg_notice_handle->high = DIVOOM_SYSTEM_MSG_NOTICE_SEP_DOT;
    }
    else if(pdivoom_system_msg_notice_handle->acticve_num == DIVOOM_SYSTEM_MSG_NOTICE_MAX_NUM)
    {
        //已经满了，将最后一个删除
        lv_obj_add_flag(pdivoom_system_msg_notice_handle->plist[DIVOOM_SYSTEM_MSG_NOTICE_MAX_NUM - 1].lvgl_obj, LV_OBJ_FLAG_HIDDEN);

        if(pdivoom_system_msg_notice_handle->plist[DIVOOM_SYSTEM_MSG_NOTICE_MAX_NUM - 1].pcallback)
        {
            divoom_free_lvgl_lock();
            pdivoom_system_msg_notice_handle->plist[DIVOOM_SYSTEM_MSG_NOTICE_MAX_NUM - 1].pcallback(pdivoom_system_msg_notice_handle->plist[DIVOOM_SYSTEM_MSG_NOTICE_MAX_NUM - 1].para, 0);            
            divoom_get_lvgl_lock();
        }    
        pdivoom_system_msg_notice_handle->acticve_num --;
        pdivoom_system_msg_notice_handle->high -= (pdivoom_system_msg_notice_handle->plist[DIVOOM_SYSTEM_MSG_NOTICE_MAX_NUM - 1].h + DIVOOM_SYSTEM_MSG_NOTICE_SEP_DOT);
    }
    //将目前所有的坐标后面移，第一个位置放最新的
    for(int i = 0; i < pdivoom_system_msg_notice_handle->acticve_num; i++)
    {
        pdivoom_system_msg_notice_handle->plist[i].y += cur_high;
        lv_obj_set_y(pdivoom_system_msg_notice_handle->plist[i].lvgl_obj, pdivoom_system_msg_notice_handle->plist[i].y);  
    }
    if(pdivoom_system_msg_notice_handle->acticve_num)
    {
        memmove(&pdivoom_system_msg_notice_handle->plist[1], &pdivoom_system_msg_notice_handle->plist[0], pdivoom_system_msg_notice_handle->acticve_num * sizeof(divoom_system_msg_notice_item));
    }

    lvgl_obj = lv_btn_create(pdivoom_system_msg_notice_handle->main_obj);
    lv_obj_set_style_bg_opa(lvgl_obj, LV_OPA_COVER, 0);
    lv_obj_add_style(lvgl_obj, &pdivoom_system_msg_notice_handle->btn_normal_type, 0);
    lv_obj_add_style(lvgl_obj, &pdivoom_system_msg_notice_handle->btn_press_type, LV_STATE_FOCUSED);
    lv_obj_add_style(lvgl_obj, &pdivoom_system_msg_notice_handle->btn_press_type, LV_STATE_FOCUS_KEY);
    lv_obj_set_pos(lvgl_obj, (DIVOOM_SCREEN_WIDTH - width)/2, DIVOOM_SYSTEM_MSG_NOTICE_SEP_DOT);
    lv_obj_set_size(lvgl_obj, width, high);
    pdivoom_system_msg_notice_handle->plist[0].lvgl_obj = lvgl_obj;
    pdivoom_system_msg_notice_handle->plist[0].y = DIVOOM_SYSTEM_MSG_NOTICE_SEP_DOT;
    pdivoom_system_msg_notice_handle->plist[0].w = width;
    pdivoom_system_msg_notice_handle->plist[0].h = high;
    pdivoom_system_msg_notice_handle->plist[0].pcallback = pcallback;
    pdivoom_system_msg_notice_handle->plist[0].para = para;
    pdivoom_system_msg_notice_handle->plist[0].pre_key_cb = NULL;
    pdivoom_system_msg_notice_handle->high  += cur_high;
    lv_obj_set_size(pdivoom_system_msg_notice_handle->main_obj, DIVOOM_SCREEN_WIDTH, pdivoom_system_msg_notice_handle->high);
    lv_obj_add_event_cb(lvgl_obj, divoom_system_msg_notice_event_cb, LV_EVENT_KEY, pdivoom_system_msg_notice_handle);
    if(pdivoom_system_msg_notice_handle->acticve_num == 0)
    {
        divoom_lvgl_top_control_add_top(pdivoom_system_msg_notice_handle, pdivoom_system_msg_notice_handle->plist[0].lvgl_obj);
    }
    else
    {
        divoom_lvgl_top_control_update_top(pdivoom_system_msg_notice_handle, pdivoom_system_msg_notice_handle->plist[0].lvgl_obj);
    }
    pdivoom_system_msg_notice_handle->acticve_num ++;

    divoom_lvgl_layer_top_raise(pdivoom_system_msg_notice_handle->main_obj);
    divoom_free_lvgl_lock();

    return lvgl_obj;
}
//int divoom_system_msg_notice_test_test_time = 0;

//下面是测试代码
int divoom_system_msg_notice_test_callback(void *para, int result)
{
    int ret, *test_time = (int *)para;

    divoom_print("aj test : %d\n", *test_time);
    if(*test_time == 8)
    {
        ret = 1;
    }
    else
    {
        ret = 0;
    }
    divoom_system_free(test_time);
    return ret;
}

void divoom_system_msg_notice_test(void)
{
    int *test_para = divoom_system_malloc(sizeof(int));

    *test_para = rand()%10;
    void *lvgl_obj = divoom_system_msg_notice_add(600, 48, divoom_system_msg_notice_test_callback, test_para);
    divoom_get_lvgl_lock();

    void *label_obj = lv_label_create(lvgl_obj);
    lv_obj_set_pos(label_obj, 0, 0);
	lv_obj_set_size(label_obj, DIVOOM_SCREEN_WIDTH, 36);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(32, 1), 0);
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text_fmt(label_obj, "this is test time: %d\n", *test_para);
    divoom_free_lvgl_lock();
}

