#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"

#define DIVOOM_LVGL_MESSAGE_BOX_MAX_EVENT_BTN 20
typedef struct _divoom_lvgl_message_box_ctrl
{
    int menu_id;
    int select_id;
    int max_id;
    lv_style_t lable_style;
    lv_style_t btn_normal_style[DIVOOM_LVGL_MESSAGE_BOX_MAX_EVENT_BTN];
    lv_style_t btn_press_style[DIVOOM_LVGL_MESSAGE_BOX_MAX_EVENT_BTN];
    uint8 btn_style_inited[DIVOOM_LVGL_MESSAGE_BOX_MAX_EVENT_BTN];
    void *main_obj;
    void *obj_table[DIVOOM_LVGL_MESSAGE_BOX_MAX_EVENT_BTN];
    void *user_data;
    void *pdirect_menu_handle;
    divoom_lvgl_message_box_event_callback *pcallback;
    divoom_lvgl_message_box_focus_callback *pfocus_change_cb;
    void *focus_change_user_data;
    uint8 block_next_pre;
}divoom_lvgl_message_box_ctrl, *Pdivoom_lvgl_message_box_ctrl;

typedef struct
{
    Pdivoom_lvgl_message_box_ctrl phandle;
    int ctrl_id;
} divoom_lvgl_message_box_defer_cb_t;

#define DIVOOM_LVGL_MESSAGE_BOX_CALLBACK_DEFER_MS  50

static void divoom_lvgl_message_box_defer_cb_timer(lv_timer_t *timer)
{
    divoom_lvgl_message_box_defer_cb_t *defer;

    defer = (divoom_lvgl_message_box_defer_cb_t *)lv_timer_get_user_data(timer);
    lv_timer_del(timer);
    if (defer && defer->phandle && defer->phandle->pcallback)
    {
        defer->phandle->pcallback(defer->phandle->user_data, defer->ctrl_id);
    }
    if (defer)
    {
        divoom_system_free(defer);
    }
}

static void divoom_lvgl_message_box_invoke_callback(Pdivoom_lvgl_message_box_ctrl phandle, int ctrl_id)
{
    divoom_lvgl_message_box_defer_cb_t *defer;
    lv_timer_t *timer;

    if (!phandle || !phandle->pcallback)
    {
        return;
    }
    defer = divoom_system_malloc(sizeof(divoom_lvgl_message_box_defer_cb_t));
    if (!defer)
    {
        phandle->pcallback(phandle->user_data, ctrl_id);
        return;
    }
    defer->phandle = phandle;
    defer->ctrl_id = ctrl_id;
    timer = lv_timer_create(divoom_lvgl_message_box_defer_cb_timer,
        DIVOOM_LVGL_MESSAGE_BOX_CALLBACK_DEFER_MS, defer);
    if (timer)
    {
        lv_timer_set_repeat_count(timer, 1);
    }
    else
    {
        phandle->pcallback(phandle->user_data, ctrl_id);
        divoom_system_free(defer);
    }
}

static void divoom_lvgl_message_box_sync_select_id(Pdivoom_lvgl_message_box_ctrl phandle, lv_obj_t *obj)
{
	int i;

	if (!phandle || !obj)
	{
		return;
	}
	for (i = 0; i < DIVOOM_LVGL_MESSAGE_BOX_MAX_EVENT_BTN; i++)
	{
		if (phandle->obj_table[i] == obj)
		{
			phandle->select_id = i;
			return;
		}
	}
}

static void divoom_lvgl_message_box_focus_item(Pdivoom_lvgl_message_box_ctrl phandle, lv_obj_t *obj)
{
	if (!phandle || !obj)
	{
		return;
	}
	divoom_lvgl_message_box_sync_select_id(phandle, obj);
	lv_group_focus_obj(obj);
	lv_obj_scroll_to_view(obj, LV_ANIM_OFF);
	if (phandle->pfocus_change_cb)
	{
		phandle->pfocus_change_cb(phandle->focus_change_user_data, phandle->select_id);
	}
}

static void divoom_lvgl_message_box_btn_clear_outline_shadow(lv_obj_t *btn)
{
	static const lv_state_t states[] = {
		LV_STATE_DEFAULT,
		LV_STATE_FOCUSED,
		LV_STATE_FOCUS_KEY,
		LV_STATE_PRESSED
	};
	int si;

	if (!btn)
	{
		return;
	}

	for (si = 0; si < (int)(sizeof(states) / sizeof(states[0])); si++)
	{
		lv_obj_set_style_outline_width(btn, 0, LV_PART_MAIN | states[si]);
		lv_obj_set_style_outline_pad(btn, 0, LV_PART_MAIN | states[si]);
		lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN | states[si]);
	}
}

static void divoom_lvgl_message_box_event_cb(lv_event_t * e)
{
	int i;
	lv_obj_t *cur_obj;
	lv_obj_t *next_obj;
	Pdivoom_lvgl_message_box_ctrl phandle = (Pdivoom_lvgl_message_box_ctrl)lv_event_get_user_data(e);
    
    lv_event_code_t code = lv_event_get_code(e);
    cur_obj = lv_event_get_target_obj(e);
    if(code == LV_EVENT_KEY)
    {
        char *key_code = ((char *)lv_event_get_param(e));

        switch(key_code[0])
        {            
            case DIVOOM_LVGL_KEY_NEXT:
            {
                if (phandle->block_next_pre)
                {
                    divoom_lvgl_message_box_invoke_callback(phandle, -1);
                    break;
                }
                next_obj = divoom_menu_direct_ctrl_get(phandle->pdirect_menu_handle, cur_obj, DIVOOM_MENU_DIRECT_RIGHT);
                if (next_obj)
                {
                    divoom_lvgl_message_box_focus_item(phandle, next_obj);
                }
                break;
            }
            case DIVOOM_LVGL_KEY_PRE:
            {
                if (phandle->block_next_pre)
                {
                    divoom_lvgl_message_box_invoke_callback(phandle, -1);
                    break;
                }
                next_obj = divoom_menu_direct_ctrl_get(phandle->pdirect_menu_handle, cur_obj, DIVOOM_MENU_DIRECT_LEFT);
                if (next_obj)
                {
                    divoom_lvgl_message_box_focus_item(phandle, next_obj);
                }
                break;
            }
               
            case DIVOOM_LVGL_KEY_ESC:
                divoom_lvgl_message_box_invoke_callback(phandle, -1);
                break;
               
            case DIVOOM_LVGL_KEY_ENTER:
                for(i = 0; i < DIVOOM_LVGL_MESSAGE_BOX_MAX_EVENT_BTN; i++)
                {
                    if(cur_obj == phandle->obj_table[i])
                    {
                        divoom_lvgl_message_box_invoke_callback(phandle, i);
                        return;
                    }
                }
                break;
               
            case DIVOOM_LVGL_KEY_HOME:
                break;
                          
        }
    }
}

/*********************************************************************
  Function:        divoom_lvgl_message_box_init
  Description:     创建消息提示框
  Input:           main_obj  主窗体句柄
                   user_data   
                   pcallback   确定或取消的回调
                   pos_x   
                   pos_y
                   size_x
                   size_y
                   bg_path  背景图案如果为空，将使用bg_color
                   bg_color
  Return:          句柄
**********************************************************************/

void *divoom_lvgl_message_box_init(int menu_id, void *main_obj, void *user_data, divoom_lvgl_message_box_event_callback *pcallback, 
                                    int pos_x, int pos_y, int size_x, int size_y, char *bg_path, lv_color_t bg_color, int max_item)
{
    Pdivoom_lvgl_message_box_ctrl phandle;

    phandle = divoom_system_malloc(sizeof(divoom_lvgl_message_box_ctrl));
    if (!phandle)
    {
        return NULL;
    }
    memset(phandle, 0, sizeof(divoom_lvgl_message_box_ctrl));
    phandle->menu_id = menu_id;
    divoom_menu_control_init_lable_style(&phandle->lable_style);
    lv_style_set_border_side(&phandle->lable_style, LV_BORDER_SIDE_NONE);
    lv_style_set_radius(&phandle->lable_style, 32);    
    phandle->pdirect_menu_handle = divoom_menu_direct_ctrl_create(max_item, 1);
    phandle->user_data = user_data;
    phandle->pcallback = pcallback;
    if(bg_path)
    {
        phandle->main_obj = lv_img_create(main_obj);    
        lv_obj_set_pos(phandle->main_obj, pos_x, pos_y);
        lv_obj_set_size(phandle->main_obj, size_x, size_y);
        lv_obj_add_style(phandle->main_obj, &phandle->lable_style, 0);
        lv_obj_set_scrollbar_mode(phandle->main_obj, LV_SCROLLBAR_MODE_OFF);
        divoom_menu_control_lv_img_set_src(phandle->main_obj, bg_path);
    }
    else
    {
        phandle->main_obj = lv_obj_create(main_obj);    
        lv_obj_set_pos(phandle->main_obj, pos_x, pos_y);
        lv_obj_set_size(phandle->main_obj, size_x, size_y);
        lv_obj_add_style(phandle->main_obj, &phandle->lable_style, 0);
        lv_obj_set_scrollbar_mode(phandle->main_obj, LV_SCROLLBAR_MODE_OFF);
        lv_obj_set_style_bg_color(phandle->main_obj, bg_color, LV_PART_MAIN);
    }

    return phandle;
}

/*********************************************************************
  Function:        divoom_lvgl_message_box_add_label_item
  Description:     为提示框增加一个LABEL控件
  Input:           handle  主窗体句柄
                   disp_str   
                   ctrl_id   控件ID必须是唯一的,确定后返回的值，如果不关心可以设置为-1
                   font_id   字号
                   font_color  字体颜色
                   align_mode  对齐模式
                   pos_x   
                   pos_y
                   size_x
                   size_y
  Return:          NONE
**********************************************************************/


void divoom_lvgl_message_box_add_label_item(void *handle, char *disp_str, int ctrl_id, int font_id, lv_color_t font_color, int align_mode,int pos_x, int pos_y, int size_x, int size_y)
{
    void *label_obj, *cout_obj;
    Pdivoom_lvgl_message_box_ctrl phandle = (Pdivoom_lvgl_message_box_ctrl)handle;

    if(ctrl_id >= DIVOOM_LVGL_MESSAGE_BOX_MAX_EVENT_BTN)
    {
        return;
    }

    if(ctrl_id != -1 && phandle->obj_table[ctrl_id])
    {
        divoom_print("it fails to create repeat ctrl id: %d\n", ctrl_id);
        return;
    }
    
    cout_obj = lv_obj_create(phandle->main_obj);
    lv_obj_add_style(cout_obj, &phandle->lable_style, 0);
    lv_obj_set_pos(cout_obj, pos_x, pos_y);
    lv_obj_set_size(cout_obj, size_x, size_y);
    lv_obj_set_style_bg_opa(cout_obj, LV_OPA_TRANSP, 0);
    
    
    label_obj = lv_label_create(cout_obj);
    lv_label_set_text(label_obj, disp_str);
    lv_obj_set_style_text_align(label_obj, align_mode, 0);
    lv_obj_set_style_text_color(label_obj, font_color, 0);  
    lv_label_set_long_mode(label_obj, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(label_obj, size_x); 
    
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(font_id, phandle->menu_id), 0); 
    if(ctrl_id != -1)
    {
        phandle->obj_table[ctrl_id] = cout_obj;
        lv_obj_add_flag(cout_obj, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(cout_obj, divoom_lvgl_message_box_event_cb, LV_EVENT_KEY, phandle);
    }
    lv_obj_center(label_obj);
        
}

/*********************************************************************
  Function:        divoom_lvgl_message_box_add_label_item
  Description:     为提示框增加一个LABEL图像
  Input:           handle  主窗体句柄
                   image_path   
                   ctrl_id   控件ID必须是唯一的,确定后返回的值，如果不关心可以设置为-1
                   pos_x   
                   pos_y
                   size_x
                   size_y
  Return:          NONE
**********************************************************************/


void divoom_lvgl_message_box_add_image_item(void *handle, char *image_path, int ctrl_id, int pos_x, int pos_y, int size_x, int size_y)
{
    void *image_obj;
    Pdivoom_lvgl_message_box_ctrl phandle = (Pdivoom_lvgl_message_box_ctrl)handle;

    if(ctrl_id >= DIVOOM_LVGL_MESSAGE_BOX_MAX_EVENT_BTN)
    {
        return;
    }

    if(ctrl_id != -1 && phandle->obj_table[ctrl_id])
    {
        divoom_print("it fails to create repeat ctrl id: %d\n", ctrl_id);
        return;
    }
    

    image_obj = lv_img_create(phandle->main_obj);    
    lv_obj_set_pos(image_obj, pos_x, pos_y);
    lv_obj_set_size(image_obj, size_x, size_y);
    divoom_menu_control_lv_img_set_src(image_obj, image_path);
    if(ctrl_id != -1)
    {
        phandle->obj_table[ctrl_id] = image_obj;
        lv_obj_add_flag(image_obj, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(image_obj, divoom_lvgl_message_box_event_cb, LV_EVENT_KEY, phandle);
    }

}

/*********************************************************************
  Function:        divoom_lvgl_message_box_set_active_item
  Description:     设置焦点控件
  Input:           handle  主窗体句柄
                   ctrl_id  控件ID
  Return:          NONE
**********************************************************************/

void divoom_lvgl_message_box_set_active_item(void *handle, int ctrl_id)
{
    Pdivoom_lvgl_message_box_ctrl phandle = (Pdivoom_lvgl_message_box_ctrl)handle;

    if (phandle && ctrl_id >= 0 && ctrl_id <= phandle->max_id && phandle->obj_table[ctrl_id])
    {
        divoom_lvgl_message_box_focus_item(phandle, phandle->obj_table[ctrl_id]);
    }
}

int divoom_lvgl_message_box_get_active_item(void *handle)
{
    Pdivoom_lvgl_message_box_ctrl phandle = (Pdivoom_lvgl_message_box_ctrl)handle;

    if (!phandle)
    {
        return -1;
    }
    return phandle->select_id;
}

void divoom_lvgl_message_box_set_focus_change_cb(void *handle,
	divoom_lvgl_message_box_focus_callback *pcallback, void *user_data)
{
	Pdivoom_lvgl_message_box_ctrl phandle = (Pdivoom_lvgl_message_box_ctrl)handle;

	if (!phandle)
	{
		return;
	}
	phandle->pfocus_change_cb = pcallback;
	phandle->focus_change_user_data = user_data;
}

void divoom_lvgl_message_box_dispatch_key(void *handle, uint8 key)
{
    Pdivoom_lvgl_message_box_ctrl phandle = (Pdivoom_lvgl_message_box_ctrl)handle;
    lv_obj_t *btn;
    char key_param[2];

    if (!phandle)
    {
        return;
    }

    if (phandle->select_id < 0 || phandle->select_id > phandle->max_id)
    {
        return;
    }

    btn = phandle->obj_table[phandle->select_id];
    if (!btn)
    {
        return;
    }

    divoom_lvgl_message_box_focus_item(phandle, btn);
    key_param[0] = (char)key;
    key_param[1] = DIVOOM_PRESS_SHORT;
    lv_obj_send_event(btn, LV_EVENT_KEY, key_param);
}

void divoom_lvgl_message_box_set_block_next_pre(void *handle, uint8 block_next_pre)
{
    Pdivoom_lvgl_message_box_ctrl phandle = (Pdivoom_lvgl_message_box_ctrl)handle;

    if (phandle)
    {
        phandle->block_next_pre = block_next_pre;
    }
}

/*********************************************************************
  Function:        divoom_lvgl_message_box_add_label_item
  Description:     为提示框增加一个button控件
  Input:           handle  主窗体句柄
                   btn_name    
                   font_id   字号
                   normal_font_color  未激活字体颜色
                   active_font_color  激活字体颜色 
                   ctrl_id   控件ID必须是唯一的,确定后返回的值，如果不关心可以设置为-1
                   pos_x   
                   pos_y
                   size_x
                   size_y
  Return:          NONE
**********************************************************************/


void divoom_lvgl_message_box_add_btn_item(void *handle, char *btn_name, int font_id, lv_color_t normal_color, lv_color_t active_color,lv_color_t font_color,int ctrl_id, int pos_x, int pos_y, int size_x, int size_y)
{
    void *btn_obj, *label_obj;
    Pdivoom_lvgl_message_box_ctrl phandle = (Pdivoom_lvgl_message_box_ctrl)handle;

    if(ctrl_id >= DIVOOM_LVGL_MESSAGE_BOX_MAX_EVENT_BTN)
    {
        return;
    }

    if(ctrl_id != -1 && phandle->obj_table[ctrl_id])
    {
        divoom_print("it fails to create repeat ctrl id: %d\n", ctrl_id);
        return;
    }


    btn_obj = lv_btn_create(phandle->main_obj);    
    if(ctrl_id >= 0)
    {
        phandle->max_id = ctrl_id;
        divoom_menu_direct_ctrl_update(phandle->pdirect_menu_handle, btn_obj, ctrl_id, 0);
        divoom_menu_control_init_btn_stype( NULL, NULL, &phandle->btn_normal_style[ctrl_id], NULL, &phandle->btn_press_style[ctrl_id], NULL, 99);
        phandle->btn_style_inited[ctrl_id] = 1;
        lv_style_set_border_color(&phandle->btn_normal_style[ctrl_id], normal_color);
        //lv_style_set_text_color(&phandle->btn_normal_style[ctrl_id], normal_font_color);
        lv_style_set_border_color(&phandle->btn_press_style[ctrl_id], active_color);
        //lv_style_set_text_color(&phandle->btn_press_style[ctrl_id], active_color);
        lv_obj_add_style(btn_obj, &phandle->btn_normal_style[ctrl_id], 0);
        lv_obj_add_style(btn_obj, &phandle->btn_press_style[ctrl_id] , LV_STATE_FOCUSED);
        lv_obj_add_style(btn_obj, &phandle->btn_press_style[ctrl_id], LV_STATE_FOCUS_KEY);
    }
    else
    {

        lv_obj_set_style_bg_opa(btn_obj, 0, 0);        
        lv_obj_set_style_border_color(btn_obj, lv_color_hex(0), 0);
        lv_obj_set_style_outline_color(btn_obj, lv_color_hex(DIVOOM_MENNU_NORMAL_BG_COLOR), 0);
        lv_obj_set_style_border_width(btn_obj, 0, 0);
        lv_obj_set_style_outline_width(btn_obj, 0, 0);
        lv_obj_set_style_shadow_width(btn_obj, 0, 0);
        lv_obj_set_style_radius(btn_obj, 0, 99);
        lv_obj_set_style_border_side(btn_obj, LV_BORDER_SIDE_NONE, 0);
    }
    lv_obj_set_pos(btn_obj, pos_x, pos_y);
    lv_obj_set_size(btn_obj, size_x, size_y);
    if(ctrl_id >= 0)
    {
        lv_group_t *grp;

        phandle->obj_table[ctrl_id] = btn_obj;
        lv_obj_add_event_cb(btn_obj, divoom_lvgl_message_box_event_cb, LV_EVENT_KEY, phandle);
        grp = lv_group_get_default();
        if (grp)
        {
            lv_group_add_obj(grp, btn_obj);
        }
        divoom_lvgl_message_box_btn_clear_outline_shadow(btn_obj);
    }
    label_obj = lv_label_create(btn_obj);
    lv_obj_set_style_text_color(label_obj, font_color, 0); 
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(font_id, phandle->menu_id), 0); 
    lv_label_set_text(label_obj, btn_name);
    lv_obj_center(label_obj);

}

/*********************************************************************
  Function:        divoom_lvgl_message_box_destroy
  Description:     释放所有资源
  Input:           handle  主窗体句柄
  Return:          NONE
**********************************************************************/

void divoom_lvgl_message_box_hide(void *handle)
{
    Pdivoom_lvgl_message_box_ctrl phandle = (Pdivoom_lvgl_message_box_ctrl)handle;
    int i;

    if (!phandle || !phandle->main_obj)
    {
        return;
    }
    for (i = 0; i < DIVOOM_LVGL_MESSAGE_BOX_MAX_EVENT_BTN; i++)
    {
        if (phandle->obj_table[i])
        {
            lv_group_remove_obj(phandle->obj_table[i]);
        }
    }
    lv_obj_add_flag(phandle->main_obj, LV_OBJ_FLAG_HIDDEN);
}

void divoom_lvgl_message_box_destroy(void *handle, int sync_flag)
{
    Pdivoom_lvgl_message_box_ctrl phandle = (Pdivoom_lvgl_message_box_ctrl)handle;
    int i;

    if(handle == NULL)
    {
        return;
    }
    for (i = 0; i < DIVOOM_LVGL_MESSAGE_BOX_MAX_EVENT_BTN; i++)
    {
        if (phandle->obj_table[i])
        {
            lv_group_remove_obj(phandle->obj_table[i]);
        }
    }
    lv_obj_add_flag(phandle->main_obj, LV_OBJ_FLAG_HIDDEN);
    if(sync_flag)
    {
        //lv_obj_del(phandle->main_obj);
    }
    else
    {
        lv_obj_del(phandle->main_obj);
    }
    lv_style_reset(&phandle->lable_style);
    /* 仅 reset 实际初始化过的按钮 style：未初始化(memset 0)的 style 调 lv_style_reset 会
     * 触发 LV_ASSERT_STYLE(while(1)) 死循环。label/image 框无 event_btn 时全部未初始化。 */
    for(int i = 0; i < DIVOOM_LVGL_MESSAGE_BOX_MAX_EVENT_BTN; i++)
    {
        if (phandle->btn_style_inited[i])
        {
            lv_style_reset(&phandle->btn_normal_style[i]);
            lv_style_reset(&phandle->btn_press_style[i]);
        }
    }
    divoom_menu_direct_ctrl_destory(phandle->pdirect_menu_handle);
    divoom_system_free(phandle);
}

