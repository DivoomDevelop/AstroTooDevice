#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"
#include "divoom_wifi_con.h"
#include "cJSON.h"
#ifdef DIVOOM_USE_RK_IC
#include <Rk_wifi.h>
#else
#include  "wifimg.h"
#endif

#define DIVOOM_LVGL_SETTING_WIFI_MAX_NUM    6
typedef struct _divoom_lvgl_setting_wifi_ctrl
{
    int menu_id;
    int wifi_num;
    int select_id;
	int run_flag;
	int connect_flag;	//0:获取当前网络数据 1:连接其他网络 2:等待连接完成
	int get_flag;		//0:没有数据更新,请求获取数据                 1:有数据更新
    char disp_info[DIVOOM_LVGL_SETTING_WIFI_MAX_NUM][128];
    char secret_info[DIVOOM_LVGL_SETTING_WIFI_MAX_NUM][128];
	void *btn_obj[DIVOOM_LVGL_SETTING_WIFI_MAX_NUM];
	void *image_obj[DIVOOM_LVGL_SETTING_WIFI_MAX_NUM];
	void *properties_label_obj[4];
	void *wifi_signal_obj;
	void *cur_active_obj;
	void *main_obj;
	void *exit_sem;
    //void *handle;
	void *pdirect_handle;
	Pdivoom_wifi_menu_wifi_info pwifo_list;

	lv_style_t btn_normal_style;
	lv_style_t btn_select_style;

	void *add_btn;
	int disp_tick;
	int connect_time;
	void *img_obj;
	void *box_img;
	void *bg_obj;
	lv_timer_t *waiting_timer;

	wifi_sta_info_t wifi_info;

	lv_timer_t *refresh_timer;
}divoom_lvgl_setting_wifi_ctrl, *Pdivoom_lvgl_setting_wifi_ctrl;


void divoom_lvgl_setting_wifi_callback(void *handle, int ret_value, uint8 *select_info, int item_num)
{
    int i;
    Pdivoom_lvgl_setting_wifi_ctrl phandle = (Pdivoom_lvgl_setting_wifi_ctrl)handle;

    for(i = 0; i < item_num; i++)
    {
        if(select_info[i])
        {
            //if(phandle->select_id != select_info[i])
			if(phandle->select_id != i)
            {
                wifi_sta_cn_para_t wifi_info = {0};
                wifi_info.ssid = phandle->disp_info[i];
                wifi_info.password = phandle->secret_info[i];
                wifi_sta_connect(&wifi_info);
			}
            return;
        }
    }
}


void divoom_lvgl_setting_wifi_disp(void *handle, void *active_obj)
{
    //Pdivoom_lvgl_setting_wifi_ctrl phandle = (Pdivoom_lvgl_setting_wifi_ctrl)handle;
    //divoom_lvgl_normal_menu_list_disp(phandle->handle, active_obj);
    
	Pdivoom_lvgl_setting_wifi_ctrl phandle = (Pdivoom_lvgl_setting_wifi_ctrl)handle;

	if(phandle->cur_active_obj)
	{
		lv_group_focus_obj(phandle->cur_active_obj);
		phandle->cur_active_obj = NULL;
	}
	else if(active_obj)
	{
		lv_group_focus_obj(active_obj);
    }
	lv_scr_load(phandle->main_obj);
}

void divoom_lvgl_setting_wifi_exit(void *handle)
{
    Pdivoom_lvgl_setting_wifi_ctrl phandle = (Pdivoom_lvgl_setting_wifi_ctrl)handle;

    //divoom_lvgl_normal_menu_list_exit(phandle->handle);
	lv_timer_del(phandle->waiting_timer);
    lv_timer_del(phandle->refresh_timer);
    phandle->run_flag = 0;
    //divoom_system_get_sem(phandle->exit_sem);
    //divoom_system_destroy_sem(phandle->exit_sem);
    lv_style_reset(&phandle->btn_normal_style);
	lv_style_reset(&phandle->btn_select_style);
    divoom_menu_direct_ctrl_destory(phandle->pdirect_handle);
    lv_obj_del(phandle->main_obj);
    //divoom_system_free(phandle);
}

void divoom_lvgl_setting_wifi_load_wifi_info(Pdivoom_lvgl_setting_wifi_ctrl phandle)
{
    char *file_buf;
    int file_len;
    int i;
    divoom_cJSON *root;
    divoom_cJSON *list;

    phandle->wifi_num = 0;
    file_buf = divoom_app_com_read_file(DIVOOM_WPA_SUPPLICANT_CONF, &file_len);
    if (file_buf == NULL)
    {
        return;
    }

    root = divoom_cJSON_Parse(file_buf);
    divoom_system_free(file_buf);
    if (root == NULL)
    {
        return;
    }

    list = divoom_cJSON_GetObjectItem(root, "wifi");
    if (list && (list->type & divoom_cJSON_Array))
    {
        for (i = 0; i < divoom_cJSON_GetArraySize(list) &&
                phandle->wifi_num < DIVOOM_LVGL_SETTING_WIFI_MAX_NUM; i++)
        {
            divoom_cJSON *item = divoom_cJSON_GetArrayItem(list, i);
            divoom_cJSON *ssid_item;
            divoom_cJSON *password_item;

            if (item == NULL || (item->type & divoom_cJSON_Object) == 0)
            {
                continue;
            }
            ssid_item = divoom_cJSON_GetObjectItem(item, "ssid");
            password_item = divoom_cJSON_GetObjectItem(item, "password");
            if (ssid_item == NULL || password_item == NULL ||
                (ssid_item->type & divoom_cJSON_String) == 0 ||
                (password_item->type & divoom_cJSON_String) == 0 ||
                ssid_item->valuestring == NULL || ssid_item->valuestring[0] == '\0')
            {
                continue;
            }
            strncpy(phandle->disp_info[phandle->wifi_num], ssid_item->valuestring,
                    sizeof(phandle->disp_info[phandle->wifi_num]) - 1);
            strncpy(phandle->secret_info[phandle->wifi_num], password_item->valuestring,
                    sizeof(phandle->secret_info[phandle->wifi_num]) - 1);
            phandle->wifi_num++;
        }
    }

    divoom_cJSON_Delete(root);
}

void divoom_lvgl_setting_wifi_event_cb(lv_event_t *e)
{
	lv_obj_t *cur_btn_obj = lv_event_get_target(e);
	lv_event_code_t code = lv_event_get_code(e);
	Pdivoom_lvgl_setting_wifi_ctrl phandle = (Pdivoom_lvgl_setting_wifi_ctrl)lv_event_get_user_data(e);
	int cur_pos;
	
	if(code == LV_EVENT_KEY)
	{
		char c = *((char *)lv_event_get_param(e));

		if(c == DIVOOM_LVGL_KEY_NEXT)
		{
			cur_btn_obj = divoom_menu_direct_ctrl_get(phandle->pdirect_handle, cur_btn_obj, DIVOOM_MENU_DIRECT_UP);
			lv_group_focus_obj(cur_btn_obj);
		}
		else if(c == DIVOOM_LVGL_KEY_PRE)
		{
			cur_btn_obj = divoom_menu_direct_ctrl_get(phandle->pdirect_handle, cur_btn_obj, DIVOOM_MENU_DIRECT_DOWN);
			lv_group_focus_obj(cur_btn_obj);
		}
		else if(c == DIVOOM_LVGL_KEY_ENTER)
		{
			cur_pos = divoom_menu_direct_ctrl_get_pos(phandle->pdirect_handle, cur_btn_obj);

			if(cur_pos == 0)	//添加新的网络
			{
				int *add_flag = (int *)divoom_system_malloc(sizeof(int));
				(*add_flag) = 2;
				divoom_menu_control_clearall_load(DIVOOM_MENU_CONTROL_ID_CONFIG_WIFI, (void *)add_flag, 0, 0);
			}
			else if((phandle->connect_flag != 1) && (phandle->select_id != (cur_pos - 1)))	
			{
				lv_obj_add_flag(phandle->image_obj[phandle->select_id], LV_OBJ_FLAG_HIDDEN);
				phandle->select_id = cur_pos - 1;
				phandle->connect_flag = 1;
			}
		}
		else if(c == DIVOOM_LVGL_KEY_ESC)
		{
			divoom_menu_control_return_pre();
		}
		
	}

}


void divoom_lvgl_setting_wifi_refresh_wifi_info(lv_timer_t *refresh_timer)
{
	int i, find_flag;
	Pdivoom_lvgl_setting_wifi_ctrl phandle = (Pdivoom_lvgl_setting_wifi_ctrl)lv_timer_get_user_data(refresh_timer);
	
	if(phandle->get_flag == 1 && phandle->connect_flag == 0)
	{
		find_flag = 0;
		
		for(i = 0;i < phandle->wifi_num; i++)	//比较是否有对应网络
		{
			if(strcmp(phandle->disp_info[i], phandle->wifi_info.ssid) == 0)
			{
				find_flag = 1;
				phandle->select_id = i;
				lv_obj_clear_flag(phandle->image_obj[phandle->select_id], LV_OBJ_FLAG_HIDDEN);
			}
			else
			{
				lv_obj_add_flag(phandle->image_obj[i], LV_OBJ_FLAG_HIDDEN);
			}
		}
		if(find_flag == 0)
		{
			phandle->get_flag = 0;
			return;
		}
		
		lv_obj_clear_flag(phandle->wifi_signal_obj, LV_OBJ_FLAG_HIDDEN);
	
		//不断获取刷新网络信息
		lv_label_set_text_fmt(phandle->properties_label_obj[0], "%s", phandle->wifi_info.ssid);
		lv_label_set_text(phandle->properties_label_obj[1], divoom_menu_control_get_disp_str("Connected"));
		lv_label_set_text_fmt(phandle->properties_label_obj[2], "%d.%d.%d.%d", phandle->wifi_info.ip_addr[0], phandle->wifi_info.ip_addr[1], phandle->wifi_info.ip_addr[2], phandle->wifi_info.ip_addr[3]);
		lv_label_set_text_fmt(phandle->properties_label_obj[3], "%02x:%02x:%02x:%02x:%02x:%02x", phandle->wifi_info.mac_addr[0], phandle->wifi_info.mac_addr[1], phandle->wifi_info.mac_addr[2], 
			phandle->wifi_info.mac_addr[3], phandle->wifi_info.mac_addr[4], phandle->wifi_info.mac_addr[5]);

		lv_obj_align_to(phandle->wifi_signal_obj, phandle->properties_label_obj[0], LV_ALIGN_TOP_LEFT, -45, -5);
		if(phandle->wifi_info.rssi > -60)
        {
           divoom_menu_control_lv_img_set_src(phandle->wifi_signal_obj, "73/73_wifi_level_3.png");
        }
        else if(phandle->wifi_info.rssi > -70)
        {
           divoom_menu_control_lv_img_set_src(phandle->wifi_signal_obj, "73/73_wifi_level_2.png");
        }
        else
        {
           divoom_menu_control_lv_img_set_src(phandle->wifi_signal_obj, "73/73_wifi_level_1.png");
        }
		phandle->get_flag = 0;
	}

	if(phandle->connect_flag == 1 || phandle->connect_flag == 2)
	{
		//正在连接
		lv_obj_add_flag(phandle->wifi_signal_obj, LV_OBJ_FLAG_HIDDEN);
		
		lv_label_set_text_fmt(phandle->properties_label_obj[0], "%s", phandle->disp_info[phandle->select_id]);
		lv_label_set_text(phandle->properties_label_obj[1], divoom_menu_control_get_disp_str("Connecting"));
		lv_label_set_text(phandle->properties_label_obj[2], "-.-.-.-");
		lv_label_set_text(phandle->properties_label_obj[3], "-:-:-:-:-:-");
	}

}

void *divoom_lvgl_setting_wifi_task_main(void *handle)
{
	wifi_sta_info_t wifi_info;
	Pdivoom_lvgl_setting_wifi_ctrl phandle = (Pdivoom_lvgl_setting_wifi_ctrl)handle;

	while(phandle->run_flag)
    {
    	if(phandle->get_flag == 0)	//不断获取数据
    	{
    		wifi_sta_get_info(&wifi_info);
			//divoom_print("ssid = %s, cur_ssid = %s\n", wifi_info.ssid, phandle->wifi_info.ssid);

			if(phandle->connect_flag == 2)	//正在连接
			{
				if(memcmp(&wifi_info.ssid, &phandle->wifi_info.ssid, sizeof(phandle->wifi_info.ssid)))
				{
					//if(strcmp(&wifi_info.ssid, phandle->disp_info[phandle->select_id]) == 0)
					{
						memcpy(&phandle->wifi_info, &wifi_info, sizeof(wifi_sta_info_t));
						phandle->connect_flag = 0;
						phandle->get_flag = 1;
					}
				}
				continue;
			}
			
			if(memcmp(&wifi_info, &phandle->wifi_info, sizeof(wifi_sta_info_t))) //获取当前网络信息
			{
				memcpy(&phandle->wifi_info, &wifi_info, sizeof(wifi_sta_info_t));
				phandle->get_flag = 1;
			}
    	}
		
		if(phandle->connect_flag == 1)	//开始连接
		{
			 divoom_wifi_connect_ssid(phandle->disp_info[phandle->select_id], phandle->secret_info[phandle->select_id]);
			 phandle->connect_flag = 2;
			 phandle->get_flag = 0;
		}
		divoom_system_delay_ms(1000);
	}
	divoom_system_free(phandle);
	//divoom_system_put_sem(phandle->exit_sem);
	return NULL;
}

void divoom_lvgl_setting_wifi_menu_box_event_cb(lv_event_t * e)
{
	Pdivoom_lvgl_setting_wifi_ctrl phandle = (Pdivoom_lvgl_setting_wifi_ctrl)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *cur_obj = lv_event_get_target(e);

	if(code == LV_EVENT_KEY)
	{		
		char c = *((char *)lv_event_get_param(e));

		if(c == DIVOOM_LVGL_KEY_ENTER)
		{
			lv_obj_del_async(phandle->box_img);
			lv_group_focus_obj(phandle->add_btn);
		}
		else if(c == DIVOOM_LVGL_KEY_ESC)
		{
			lv_obj_del_async(phandle->box_img);
			lv_group_focus_obj(phandle->add_btn);
		}
	}
}


void divoom_lvgl_setting_wifi_remind_box(Pdivoom_lvgl_setting_wifi_ctrl phandle)
{
	lv_obj_t *box_img = lv_img_create(phandle->main_obj);
	divoom_menu_control_lv_img_set_src(box_img, "28/28_box.png");
	lv_obj_set_pos(box_img, 80, 436-50);
	phandle->box_img = box_img;

	lv_obj_t *title_label = lv_label_create(box_img);
	lv_obj_set_pos(title_label, 0, 23+5);
	lv_obj_set_size(title_label, 640, 44);
	lv_obj_set_style_text_align(title_label, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_set_style_text_color(title_label, lv_color_hex(0x000000), 0);
	lv_obj_set_style_text_font(title_label, divoom_menu_control_get_font_info(32, phandle->menu_id), 0);
	lv_label_set_text(title_label, divoom_menu_control_get_disp_str("NetworkConnect"));
	
	lv_obj_t *text_label = lv_label_create(box_img);
	lv_obj_set_pos(text_label, 20, 90+50);
	lv_obj_set_size(text_label, 600, 180-50);
	lv_obj_set_style_text_align(text_label, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_set_style_text_color(text_label, lv_color_hex(0x0), 0);
	lv_obj_set_style_text_font(text_label, divoom_menu_control_get_font_info(26, phandle->menu_id), 0);
	lv_label_set_text(text_label, divoom_menu_control_get_disp_str("NetworkConnectFail"));

	lv_obj_t *btn = lv_btn_create(box_img);
	lv_obj_remove_style_all(btn);
	lv_obj_set_style_border_width(btn, 4, 0);
	lv_obj_set_style_border_color(btn, lv_color_hex(0x6D6D6D), 0);
	lv_obj_set_style_bg_color(btn, lv_color_hex(0xF2F2F2), 0);
	lv_obj_set_style_radius(btn, 99, 0);
	lv_obj_set_style_pad_all(btn, 0, 0);
	lv_obj_set_pos(btn, 179, 294);
	lv_obj_set_size(btn, 282, 90);
	lv_group_focus_obj(btn);
	lv_obj_add_event_cb(btn, divoom_lvgl_setting_wifi_menu_box_event_cb, LV_EVENT_KEY, phandle);
	
	lv_obj_t *btn_label = lv_label_create(btn);
	lv_obj_set_pos(btn_label, 0, 23);
	lv_obj_set_size(btn_label, 282, 44);
	lv_obj_set_style_text_align(btn_label, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_set_style_text_color(btn_label, lv_color_hex(0x000000), 0);
	lv_obj_set_style_text_font(btn_label, divoom_menu_control_get_font_info(32, phandle->menu_id), 0);
	lv_label_set_text(btn_label, divoom_menu_control_get_disp_str("Ok"));
}


void divoom_lvgl_setting_wifi_time_callback(struct _lv_timer_t *time_handle)
{
	char file_name[128];
    Pdivoom_lvgl_setting_wifi_ctrl phandle = (Pdivoom_lvgl_setting_wifi_ctrl)lv_timer_get_user_data(time_handle);

	if(phandle->connect_flag != 0)
    {
		phandle->disp_tick ++;
		phandle->connect_time ++;
    	if(phandle->disp_tick >= 8)
    	{
        	phandle->disp_tick = 0;
    	}
    	sprintf(file_name, "79/Loading%d.png", phandle->disp_tick + 1);
		
		divoom_menu_control_lv_img_set_src(phandle->img_obj, file_name);

		if(phandle->connect_time == 1)
		{
			lv_obj_set_style_img_recolor(phandle->img_obj, lv_color_hex(0x0), 0);
			lv_obj_set_style_img_recolor_opa(phandle->img_obj, LV_OPA_100, 0);
			lv_obj_clear_flag(phandle->bg_obj, LV_OBJ_FLAG_HIDDEN);
    	}
		else if(phandle->connect_time == 100)
		{	
			lv_obj_add_flag(phandle->bg_obj, LV_OBJ_FLAG_HIDDEN);
			divoom_lvgl_setting_wifi_remind_box(phandle);
		}
	}
	else
	{
		phandle->connect_time = 0;
		phandle->disp_tick = 0;
		lv_obj_add_flag(phandle->bg_obj, LV_OBJ_FLAG_HIDDEN);
	}
}

void *divoom_lvgl_setting_wifi_init(void *user_data, int menu_id)
{
	int i;
    char file_name[64];
    Pdivoom_lvgl_setting_wifi_ctrl phandle;
    divoom_lvgl_normal_menu_list_info para_info = {0};
    
#ifdef DIVOOM_USE_RK_IC
    RK_WIFI_INFO_Connection_s conn_info;
    RK_wifi_running_getConnectionInfo(&conn_info);
#else
    //wifi_sta_info_t wifi_info;
    //wifi_sta_get_info(&wifi_info);
#endif

    phandle = divoom_system_malloc(sizeof(divoom_lvgl_setting_wifi_ctrl));
    if(phandle == NULL)
    {
       return NULL;
    }
	memset(phandle, 0, sizeof(divoom_lvgl_setting_wifi_ctrl));
	phandle->cur_active_obj = NULL;
    phandle->menu_id = menu_id;
	phandle->run_flag = 1;
	phandle->connect_flag = 0;
	phandle->get_flag = 0;
	phandle->connect_time = 0;
	//phandle->select_id = -1;
    //divoom_lvgl_setting_wifi_load_wifi_info(phandle);
	phandle->pwifo_list = divoom_wifi_menu_get_all_wifi_info(&phandle->wifi_num);
	
	for(i = 0; i < phandle->wifi_num; i++)
    {
    	if(i >= DIVOOM_LVGL_SETTING_WIFI_MAX_NUM)
    	{
			phandle->wifi_num = DIVOOM_LVGL_SETTING_WIFI_MAX_NUM;
			break;
		}
        memcpy(phandle->disp_info[i], phandle->pwifo_list[i].ssid, sizeof(phandle->pwifo_list[i].ssid));
		memcpy(phandle->secret_info[i], phandle->pwifo_list[i].pswd, sizeof(phandle->pwifo_list[i].pswd));
	}
	divoom_system_free(phandle->pwifo_list);

	phandle->pdirect_handle = divoom_menu_direct_ctrl_create(1, phandle->wifi_num+1);
	//phandle->exit_sem = divoom_system_create_sem(1, 0);
	
	lv_style_init(&phandle->btn_normal_style);
	lv_style_set_bg_color(&phandle->btn_normal_style, lv_color_hex(0xF2F2F2));
	lv_style_set_pad_all(&phandle->btn_normal_style, 0);
	lv_style_set_border_width(&phandle->btn_normal_style, 4);
	lv_style_set_border_color(&phandle->btn_normal_style, lv_color_hex(0xE4E4E4));
	lv_style_set_radius(&phandle->btn_normal_style, 17);
	lv_style_set_shadow_width(&phandle->btn_normal_style, 0);
	lv_style_set_outline_width(&phandle->btn_normal_style, 0);

	lv_style_init(&phandle->btn_select_style);
	lv_style_set_bg_color(&phandle->btn_select_style, lv_color_hex(0xF2F2F2));
	lv_style_set_pad_all(&phandle->btn_select_style, 0);
	lv_style_set_border_width(&phandle->btn_select_style, 4);
	lv_style_set_border_color(&phandle->btn_select_style, lv_color_hex(0x6D6D6D));
	lv_style_set_radius(&phandle->btn_select_style, 17);
	lv_style_set_shadow_width(&phandle->btn_select_style, 0);
	lv_style_set_outline_width(&phandle->btn_select_style, 0);
	
	phandle->main_obj = lv_obj_create(NULL);  
    lv_obj_set_style_bg_color(phandle->main_obj, lv_color_hex(0xE4E4E4), LV_PART_MAIN);
    lv_obj_set_pos(phandle->main_obj, 0, 0);
	lv_obj_set_size(phandle->main_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);  
    lv_obj_set_scrollbar_mode(phandle->main_obj, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_dir(phandle->main_obj, LV_DIR_VER);
	lv_obj_clear_flag(phandle->main_obj, LV_OBJ_FLAG_SCROLLABLE);

	lv_obj_t *title_label_obj = lv_label_create(phandle->main_obj);
    lv_obj_set_pos(title_label_obj, 100, 50);
	lv_obj_set_size(title_label_obj, 600, 78);
    lv_label_set_text(title_label_obj, divoom_menu_control_get_disp_str("NetworkConnect"));
    lv_obj_set_style_text_color(title_label_obj, lv_color_hex(0x0), 0);
    lv_obj_set_style_text_align(title_label_obj, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(title_label_obj, divoom_menu_control_get_font_info(32, phandle->menu_id), 0); 

	//网络属性
	lv_obj_t *properties_obj = lv_obj_create(phandle->main_obj);
	lv_obj_set_style_bg_color(properties_obj, lv_color_hex(0xF2F2F2), 0);
	lv_obj_set_style_radius(properties_obj, 17, 0);
	lv_obj_set_style_border_width(properties_obj, 0, 0);
	lv_obj_set_style_pad_all(properties_obj, 0, 0);
	lv_obj_set_pos(properties_obj, 44, 164);
	lv_obj_set_size(properties_obj, 712, 280);

	for(i = 0;i < 4; i++)
	{
        lv_obj_t *label_obj = lv_label_create(properties_obj);
        lv_obj_set_pos(label_obj, 32, 28+65*i);
    	lv_obj_set_size(label_obj, 400, 32);
        lv_obj_set_style_text_color(label_obj, lv_color_hex(0x767676), 0);
        lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
        lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(24, phandle->menu_id), 0); 

		lv_obj_t *properties_label_obj = lv_label_create(properties_obj);
        if(i == 0)
		{
			lv_obj_align(properties_label_obj, LV_ALIGN_TOP_RIGHT, -32, 28);
        }
		else
		{
			lv_obj_set_pos(properties_label_obj, 356, 28+65*i);
    		lv_obj_set_size(properties_label_obj, 324, 32);
        }
        lv_obj_set_style_text_color(properties_label_obj, lv_color_hex(0x767676), 0);
        lv_obj_set_style_text_align(properties_label_obj, LV_TEXT_ALIGN_RIGHT, 0);
        lv_obj_set_style_text_font(properties_label_obj, divoom_menu_control_get_font_info(24, phandle->menu_id), 0);
		phandle->properties_label_obj[i] = properties_label_obj;
		
        if(i == 0)	//当前网络
        {
        	lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("CurrentWifi"));
#ifdef DIVOOM_USE_RK_IC
			lv_label_set_text_fmt(properties_label_obj, "%s", conn_info.ssid);
#else
			//lv_label_set_text_fmt(properties_label_obj, "%s", wifi_info.ssid);
			lv_label_set_text(properties_label_obj, "--");
#endif
        	lv_obj_t *image_obj = lv_img_create(properties_obj);
    		lv_obj_align_to(image_obj, properties_label_obj, LV_ALIGN_TOP_LEFT, -45, -5);
        	lv_obj_set_size(image_obj, 36, 36);
			phandle->wifi_signal_obj = image_obj;
            /*if(wifi_info.rssi > -60)
            {
        	    divoom_menu_control_lv_img_set_src(image_obj, "73/73_wifi_level_3.png");
            }
            else if(wifi_info.rssi > -70)
            {
        	    divoom_menu_control_lv_img_set_src(image_obj, "73/73_wifi_level_2.png");
            }
            else
            {
        	    divoom_menu_control_lv_img_set_src(image_obj, "73/73_wifi_level_1.png");
            }*/
        }
		else if(i == 1)	//连接状态
		{
			lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("ConnectionStatus"));
			lv_label_set_text(properties_label_obj, divoom_menu_control_get_disp_str("Connecting"));
		}
		else if(i == 2) //IP地址
		{
			lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("IP Address"));
#ifdef DIVOOM_USE_RK_IC
			lv_label_set_text_fmt(properties_label_obj, "%s", conn_info.ip_address);
#else
			//lv_label_set_text_fmt(properties_label_obj, "%d.%d.%d.%d", wifi_info.ip_addr[0], wifi_info.ip_addr[1], wifi_info.ip_addr[2], wifi_info.ip_addr[3]);
			lv_label_set_text(properties_label_obj, "-.-.-.-");
#endif
		}
		else	//Mac地址
		{
			lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("Mac Address"));
#ifdef DIVOOM_USE_RK_IC
			lv_label_set_text_fmt(properties_label_obj, "%s", conn_info.mac_address);
#else
			//lv_label_set_text_fmt(properties_label_obj, "%02x:%02x:%02x:%02x:%02x:%02x", wifi_info.mac_addr[0], wifi_info.mac_addr[1], wifi_info.mac_addr[2], wifi_info.mac_addr[3], wifi_info.mac_addr[4], wifi_info.mac_addr[5]);
			lv_label_set_text(properties_label_obj, "-:-:-:-:-:-");
#endif
		}
        
	}

	//添加新的wifi
	lv_obj_t *add_btn = lv_btn_create(phandle->main_obj);
	lv_obj_add_style(add_btn, &phandle->btn_normal_style, 0);
    lv_obj_add_style(add_btn, &phandle->btn_select_style, LV_STATE_FOCUSED);
    lv_obj_add_style(add_btn, &phandle->btn_select_style, LV_STATE_FOCUS_KEY);
	lv_obj_set_style_pad_all(add_btn, 0, 0);
    lv_obj_set_pos(add_btn, 44, 496);
    lv_obj_set_size(add_btn, 712, 90);
	lv_obj_add_event_cb(add_btn, divoom_lvgl_setting_wifi_event_cb, LV_EVENT_KEY, phandle);
	phandle->cur_active_obj = add_btn;
	phandle->add_btn = add_btn;
	
	lv_obj_t *add_label = lv_label_create(add_btn);
	lv_obj_set_pos(add_label, 32, 28);
	lv_obj_set_size(add_label, 400, 34);
    lv_label_set_text(add_label, divoom_menu_control_get_disp_str("AddWifi"));
    lv_obj_set_style_text_color(add_label, lv_color_hex(0x000000), 0);
    lv_obj_set_style_text_align(add_label, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(add_label, divoom_menu_control_get_font_info(26, phandle->menu_id), 0); 
	divoom_menu_direct_ctrl_update(phandle->pdirect_handle, add_btn, 0, 0);

	lv_obj_t *add_img = lv_img_create(add_btn);
	divoom_menu_control_lv_img_set_src(add_img, "44/44_add.png");
	lv_obj_set_pos(add_img, 644, 30);

	//可用网络
    lv_obj_t *wifi_label = lv_label_create(phandle->main_obj);
	lv_obj_set_pos(wifi_label, 76, 634);
	lv_obj_set_size(wifi_label, 310, 32);
    lv_label_set_text(wifi_label, divoom_menu_control_get_disp_str("AvailableNetwork"));
    lv_obj_set_style_text_color(wifi_label, lv_color_hex(0x363636), 0);
    lv_obj_set_style_text_align(wifi_label, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(wifi_label, divoom_menu_control_get_font_info(24, phandle->menu_id), 0);    

	for(i = 0;i < phandle->wifi_num; i++)
	{
		lv_obj_t *btn_obj  = lv_btn_create(phandle->main_obj);
        lv_obj_add_style(btn_obj, &phandle->btn_normal_style, 0);
        lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUSED);
        lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUS_KEY);
		lv_obj_set_style_pad_all(btn_obj, 0, 0);
        lv_obj_set_pos(btn_obj, 44, 678 + 98 * i);
    	lv_obj_set_size(btn_obj, 712, 90);
		phandle->btn_obj[i] = btn_obj;

		lv_obj_t *label_obj = lv_label_create(btn_obj);
        lv_obj_set_pos(label_obj, 32, 28);
    	lv_obj_set_size(label_obj, 400, 34);
        lv_label_set_text(label_obj, phandle->disp_info[i]);
    	
        lv_obj_set_style_text_color(label_obj, lv_color_hex(0x0), 0);
        lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
        lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(26, phandle->menu_id), 0);  
			
		lv_obj_t *image_obj = lv_img_create(btn_obj);
    	lv_obj_set_pos(image_obj, 644, 27);// + 90 * i);
        lv_obj_set_size(image_obj, 36, 36);
        divoom_menu_control_lv_img_set_src(image_obj, "26/26_lang_select.png");
		lv_obj_add_flag(image_obj, LV_OBJ_FLAG_HIDDEN);
        phandle->image_obj[i] = image_obj;
		
		divoom_menu_direct_ctrl_update(phandle->pdirect_handle, btn_obj, 0, i+1);
		lv_obj_add_event_cb(btn_obj, divoom_lvgl_setting_wifi_event_cb, LV_EVENT_KEY, phandle);
		
	}

	lv_obj_t *bg_obj = lv_obj_create(phandle->main_obj);
	lv_obj_set_style_bg_color(bg_obj, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
	lv_obj_set_style_bg_opa(bg_obj, 70*LV_OPA_100/100, LV_PART_MAIN);
    lv_obj_set_pos(bg_obj, 0, 0);
	lv_obj_set_size(bg_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);  
	lv_obj_clear_flag(bg_obj, LV_OBJ_FLAG_SCROLLABLE);
	phandle->bg_obj = bg_obj;

	phandle->img_obj = lv_img_create(bg_obj);
	lv_obj_set_pos(phandle->img_obj, 360-15, 600);
    lv_obj_set_size(phandle->img_obj, 80, 80);
	lv_obj_set_style_img_recolor(phandle->img_obj, lv_color_hex(0x0), 0);
	lv_obj_set_style_img_recolor_opa(phandle->img_obj, LV_OPA_100, 0);
    divoom_menu_control_lv_img_set_src(phandle->img_obj, "79/Loading1.png");
	lv_obj_add_flag(phandle->bg_obj, LV_OBJ_FLAG_HIDDEN);

	phandle->waiting_timer = lv_timer_create(divoom_lvgl_setting_wifi_time_callback, 200, phandle);

	phandle->refresh_timer = lv_timer_create(divoom_lvgl_setting_wifi_refresh_wifi_info, 1000, phandle);
	divoom_system_create_task(divoom_lvgl_setting_wifi_task_main, "lv_wifi", phandle, DIVOOM_TASK_STACK_LVGL_WIFI);
	
    /*para_info.phandle = phandle;
    para_info.disp_title = divoom_menu_control_get_disp_str("NetworkConnect");   //标题
    para_info.select_flag = 1;	 //是否有选择按键
    para_info.confirm_flag = 0;  //是否有确认按键
    para_info.muti_flag = 0;     //是否支持多选
    para_info.menu_items = phandle->wifi_num;    //菜单项总数
    
    para_info.pcallback = divoom_lvgl_setting_wifi_callback;

    phandle->handle = divoom_lvgl_normal_menu_list_init(&para_info, menu_id);
	*/
    return phandle;
}

