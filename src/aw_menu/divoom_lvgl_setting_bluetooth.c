#include "divoom_build_config.h"
#if DIVOOM_BUILD_SIMULATOR
#include <sys/statvfs.h>
#else
#include "divoom_rtos_statvfs_stub.h"
#endif
#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"
#include "divoom_bt_music.h"

#ifdef DIVOOM_TIME_FRAME_SPEAKER

#define DIVOOM_SYSTEM_SETTING_MAX_BT_NUM	    	50

typedef struct _divoom_lvgl_setting_bluetooth_ctrl
{
    int menu_id;
    uint8 bt_status;
    uint8 bt_secret_switch;
    uint8 bt_auto;
	
	int bt_device_num;
	int connect_flag; //0:获取当前网络数据 1:连接其他蓝牙 2:等待连接完成
	int status_flag; //0:连接状态 1:断开连接状态 2:取消配对状态
	int select_id;
	const DeviceInfo *pbluetooth_list;
	const DeviceInfo *pcur_bluetooth;
	char local_adapter_name[128];

	lv_style_t	btn_normal_style;
	lv_style_t	btn_select_style;
	lv_obj_t    *first_active_obj;
	void 		*pdirect_handle;

	lv_obj_t		*switch_img_obj;
	lv_obj_t		*auto_img_obj;
	lv_obj_t		*bt_name_label;
	lv_obj_t		*bt_status_label;		

	lv_obj_t	*btn_obj[DIVOOM_SYSTEM_SETTING_MAX_BT_NUM];	//设备按钮
	lv_obj_t	*image_obj[DIVOOM_SYSTEM_SETTING_MAX_BT_NUM];	//选择图片

	void *main_obj;
    void *time_handle;

	Pdivoom_sys_setting_info psys_config;
}divoom_lvgl_setting_bluetooth_ctrl, *Pdivoom_lvgl_setting_bluetooth_ctrl;

static void divoom_lvgl_setting_bluetooth_menu_event_cb(lv_event_t * e)
{
	int cur_pos, i, ret;
    lv_obj_t *cur_obj;
    lv_group_t * cur_group_obj = lv_obj_get_group(lv_event_get_target_obj(e));
    Pdivoom_lvgl_setting_bluetooth_ctrl phandle = (Pdivoom_lvgl_setting_bluetooth_ctrl)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);
    cur_obj = lv_event_get_target_obj(e);
    if(code == LV_EVENT_KEY)
    {
		char c = *((char *)lv_event_get_param(e));
        Pdivoom_sys_setting_info psys_config = divoom_main_get_system_config();
        
		/*if(phandle->set_secret_obj == cur_obj)
        {
            //设置四位数密码
            divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_BT_SET_SECRET, NULL, 0, 0);
            return;
        }*/

		if(c == DIVOOM_LVGL_KEY_NEXT)
		{
			cur_obj = divoom_menu_direct_ctrl_get(phandle->pdirect_handle, cur_obj, DIVOOM_MENU_DIRECT_UP);
			lv_group_focus_obj(cur_obj);
		}
		else if(c == DIVOOM_LVGL_KEY_PRE)
		{
			cur_obj = divoom_menu_direct_ctrl_get(phandle->pdirect_handle, cur_obj, DIVOOM_MENU_DIRECT_DOWN);
			lv_group_focus_obj(cur_obj);
		}
		else if(c == DIVOOM_LVGL_KEY_ENTER)
		{
			cur_pos = divoom_menu_direct_ctrl_get_pos(phandle->pdirect_handle, cur_obj);

			if(cur_pos == 0)
			{
				if(phandle->psys_config->bt_auto_connect)
				{
					if(divoom_bt_music_disable_boot_connect(1) == 0)
					{
						phandle->psys_config->bt_auto_connect = 0;
						divoom_menu_control_lv_img_set_src(phandle->auto_img_obj, "26/26_off.png");
						divoom_main_save_config();
					}
				}
				else
				{
					if(divoom_bt_music_disable_boot_connect(0) == 0)
					{
						phandle->psys_config->bt_auto_connect = 1;
						divoom_menu_control_lv_img_set_src(phandle->auto_img_obj, "26/26_on.png");
						divoom_main_save_config();
					}
				}
			}
			else if(cur_pos == 1)//设备蓝牙名
			{
				
			}
			else if(cur_pos == 2)//连接状态
			{
				
			}
			else if(cur_pos == 3)//蓝牙密码
			{
            	divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_BT_SET_SECRET, NULL, 0, 0);
			}
			else if(cur_pos == 4)//断开连接   开机还会连接
			{
				ret = divoom_bt_music_disconnect_bt();

				if(ret == 0)
				{
					phandle->connect_flag = 0;
					phandle->status_flag = 1;
				}
				else
				{

				}
			}
			else if(cur_pos == 5)//取消配对  开机不会再去连接
			{
				if(phandle->pcur_bluetooth)
				{
					ret = divoom_bt_music_unpair_device(phandle->pcur_bluetooth->bt_addr);

					if(ret == 0)
					{
						phandle->connect_flag = 0;
						phandle->status_flag = 2;
					}
					else
					{
						
					}
				}
			}
			else//已连接设备
			{
				for(i = 6; i < phandle->bt_device_num+6 && i < 6 + DIVOOM_SYSTEM_SETTING_MAX_BT_NUM; i++)
				{
					if(cur_pos == i && phandle->select_id != cur_pos && phandle->connect_flag != 1)
					{
						ret = divoom_bt_music_connect_device(phandle->pbluetooth_list[i - 6].bt_addr);

						lv_obj_add_flag(phandle->image_obj[phandle->select_id], LV_OBJ_FLAG_HIDDEN);
						phandle->select_id = i - 6;

						if(ret == 0)
						{
							phandle->connect_flag = 2;
							phandle->status_flag = 0;
						}
						else
						{
							phandle->connect_flag = 1;
							phandle->status_flag = 0;
						}
						break;
					}
				}
			}

		}
		else if(c == DIVOOM_LVGL_KEY_ESC)
		{
			divoom_main_save_config();
			divoom_menu_control_return_pre();
		}
		else
		{
			char *key_info = (char *)lv_event_get_param(e);
			divoom_main_keypad_send_key_info(key_info[0], key_info[1]);
		}
    }
}
void divoom_lvgl_setting_bluetooth_timer_handle(struct _lv_timer_t * ptime_info)
{
	int ret, find_flag, i;
    Pdivoom_lvgl_setting_bluetooth_ctrl pmenu_handle = (Pdivoom_lvgl_setting_bluetooth_ctrl)lv_timer_get_user_data(ptime_info);    
    Pdivoom_sys_setting_info psys_config = divoom_main_get_system_config();
    uint8 bt_status, bt_device_num;
	const DeviceInfo *pcur_bluetooth;

	pcur_bluetooth = divoom_bt_music_get_current_device_info();
	const char *local_adapter_name = divoom_bt_music_get_local_adapter_name();

	if(pmenu_handle->connect_flag == 0)
	{	
		//if(pcur_bluetooth != pmenu_handle->pcur_bluetooth)

		if(pmenu_handle->status_flag == 1 || pmenu_handle->status_flag == 2)
		{
			lv_label_set_text(pmenu_handle->bt_status_label, " ");
			lv_label_set_text(pmenu_handle->bt_name_label, " ");
			lv_obj_add_flag(pmenu_handle->image_obj[pmenu_handle->select_id], LV_OBJ_FLAG_HIDDEN);
			return;
		}

		if(pcur_bluetooth != NULL)
		{
			find_flag = 0;

			for(i = 0; i < pmenu_handle->bt_device_num; i++)	//比较是否有对应网络
			{
				if(strcmp(pmenu_handle->pbluetooth_list[i].device_name, pcur_bluetooth->device_name) == 0)
				{
					find_flag = 1;
					pmenu_handle->select_id = i;
					lv_obj_clear_flag(pmenu_handle->image_obj[pmenu_handle->select_id], LV_OBJ_FLAG_HIDDEN);
				}
				else
				{
					lv_obj_add_flag(pmenu_handle->image_obj[i], LV_OBJ_FLAG_HIDDEN);
				}
			}

			//divoom_print("select_id = %d, find_flag = %d\n", pmenu_handle->select_id, find_flag);
			if(find_flag == 0)
			{
				return;
			}

			//if(strcmp(pcur_bluetooth->device_name, pmenu_handle->pcur_bluetooth->device_name) != 0)
			{
				lv_label_set_text(pmenu_handle->bt_status_label, pcur_bluetooth->device_name);
			}

			
			pmenu_handle->pcur_bluetooth = pcur_bluetooth;
		}

		
		if(local_adapter_name != NULL && strcmp(local_adapter_name, pmenu_handle->local_adapter_name) != 0)
		{
			lv_label_set_text(pmenu_handle->bt_name_label, local_adapter_name);
			//pmenu_handle->local_adapter_name = local_adapter_name;
			strcpy(pmenu_handle->local_adapter_name, local_adapter_name);
		}

	}
	else if(pmenu_handle->connect_flag == 1)
	{
		ret = divoom_bt_music_connect_device(pmenu_handle->pbluetooth_list[pmenu_handle->select_id].bt_addr);

		if(ret == 0)
		{
			pmenu_handle->connect_flag = 2;
		}
		else
		{
			
		}

		lv_label_set_text(pmenu_handle->bt_status_label, "----");
	}
	else if(pmenu_handle->connect_flag == 2)
	{
		lv_label_set_text(pmenu_handle->bt_status_label, "----");

		if(local_adapter_name != NULL && strcmp(local_adapter_name, pmenu_handle->local_adapter_name) != 0)
		{
			pmenu_handle->connect_flag = 0;
			//lv_label_set_text(pmenu_handle->bt_name_label, local_adapter_name);
			//pmenu_handle->local_adapter_name = local_adapter_name;
		}
	}

    /*
	//bt_status = divoom_main_get_blue_con_status();
    //bt_device_num = divoom_main_get_blue_device_num();	

	if(bt_status != pmenu_handle->bt_status)
    {
        pmenu_handle->bt_status = bt_status;
        if(bt_status)
        {
            divoom_menu_control_lv_img_set_src(pmenu_handle->connect_bt_obj, "26/26_btconn.png");
            lv_label_set_text(pmenu_handle->connect_bt_label_obj, divoom_menu_control_get_disp_str("DisConnectBT"));
        }
        else
        {
            divoom_menu_control_lv_img_set_src(pmenu_handle->connect_bt_obj, "26/26_btdiscon.png");
            lv_label_set_text(pmenu_handle->connect_bt_label_obj, divoom_menu_control_get_disp_str("ConnectBT"));
        }
    }
    if(bt_device_num != pmenu_handle->bt_device_num)
    {
        pmenu_handle->bt_device_num = bt_device_num;
        if(pmenu_handle->bt_device_num)
        {
            divoom_menu_control_lv_img_set_src(pmenu_handle->unpair_obj , "26/26_btpair.png");
        }
        else
        {
            divoom_menu_control_lv_img_set_src(pmenu_handle->unpair_obj , "26/26_btunpair.png");
        }
    }
    if(pmenu_handle->bt_auto != psys_config->bt_auto_connect)
    {
        pmenu_handle->bt_auto = psys_config->bt_auto_connect;
        if(psys_config->bt_auto_connect)
        {
            divoom_menu_control_lv_img_set_src(pmenu_handle->auto_connect_obj, "26/26_on.png");
        }
        else
        {
            divoom_menu_control_lv_img_set_src(pmenu_handle->auto_connect_obj, "26/26_off.png");
        }
    }
    if(pmenu_handle->bt_secret_switch != psys_config->bt_secret)
    {
        pmenu_handle->bt_secret_switch = psys_config->bt_secret;
        if(psys_config->bt_secret)
        {
            divoom_menu_control_lv_img_set_src(pmenu_handle->auto_connect_obj, "26/26_on.png");
        }
        else
        {
            divoom_menu_control_lv_img_set_src(pmenu_handle->auto_connect_obj, "26/26_off.png");
        }
    }*/
}

void *divoom_lvgl_setting_bluetooth_init(void *user_data, int menu_id)
{
	int i, boot_connect_flag;
	const char *local_adapter_name;
    Pdivoom_lvgl_setting_bluetooth_ctrl phandle = divoom_system_malloc(sizeof(divoom_lvgl_setting_bluetooth_ctrl));

	phandle->psys_config = divoom_main_get_system_config();
	phandle->pbluetooth_list = divoom_bt_music_get_device_list(&phandle->bt_device_num);
	phandle->pcur_bluetooth = divoom_bt_music_get_current_device_info();
	phandle->pdirect_handle = divoom_menu_direct_ctrl_create(1, phandle->bt_device_num+6);
	boot_connect_flag = divoom_bt_music_get_boot_connect_flag_from_file();

	if(boot_connect_flag == 0)
	{
		phandle->psys_config->bt_auto_connect = 1;
	}
	else
	{
		phandle->psys_config->bt_auto_connect = 0;
	}

	local_adapter_name = divoom_bt_music_get_local_adapter_name();

	if(local_adapter_name != NULL)
	{
		strcpy(phandle->local_adapter_name, local_adapter_name);
	}
	else
	{
		phandle->local_adapter_name[0] = '\0';
	}

	phandle->connect_flag = 0;

	phandle->main_obj = lv_obj_create(NULL);  

	lv_obj_set_style_border_width(phandle->main_obj, 0, 0);
	lv_obj_set_style_pad_all(phandle->main_obj, 0, 0);
	lv_obj_set_style_shadow_width(phandle->main_obj, 0, 0);
	lv_obj_set_style_outline_width(phandle->main_obj, 0, 0);
    lv_obj_set_style_bg_color(phandle->main_obj, lv_color_hex(0xE4E4E4), LV_PART_MAIN);
    lv_obj_set_pos(phandle->main_obj, 0, 0);
	lv_obj_set_size(phandle->main_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);
    lv_obj_set_scrollbar_mode(phandle->main_obj, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_dir(phandle->main_obj, LV_DIR_VER);

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

	lv_obj_t *title_label_obj = lv_label_create(phandle->main_obj);
    lv_obj_set_pos(title_label_obj, 100, 50);
	lv_obj_set_size(title_label_obj, 600, 78);
    lv_label_set_text(title_label_obj, divoom_menu_control_get_disp_str("Bluetooth"));
    lv_obj_set_style_text_color(title_label_obj, lv_color_hex(0x0), 0);
    lv_obj_set_style_text_align(title_label_obj, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(title_label_obj, divoom_menu_control_get_font_info(32, phandle->menu_id), 0); 

    for(i = 0; i < 6; i++)
	{
		lv_obj_t *btn_obj = lv_btn_create(phandle->main_obj);
		lv_obj_add_style(btn_obj, &phandle->btn_normal_style, 0);
		lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUSED);
		lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUS_KEY);
		lv_obj_set_style_pad_all(btn_obj, 0, 0);
		lv_obj_set_size(btn_obj, 712, 90);
		lv_obj_set_pos(btn_obj, 44, 164+i*98);
		lv_obj_add_event_cb(btn_obj, divoom_lvgl_setting_bluetooth_menu_event_cb, LV_EVENT_KEY, phandle);
		divoom_menu_direct_ctrl_update(phandle->pdirect_handle, btn_obj, 0, i);
		if(phandle->first_active_obj == NULL)
		{
			phandle->first_active_obj = btn_obj;
		}
		
		lv_obj_t *label_obj = lv_label_create(btn_obj);
    	lv_obj_set_pos(label_obj, 32, 28);
		lv_obj_set_size(label_obj, 473, 34);
    	lv_obj_set_style_text_color(label_obj, lv_color_hex(0x0), 0);
    	lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
    	lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(26, phandle->menu_id), 0); 
		
		/*if(i == 0)
		{
			lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("Bluetooth"));

			lv_obj_t *img_obj = lv_img_create(btn_obj);

			divoom_menu_control_lv_img_set_src(img_obj, "26/26_on.png");
			lv_obj_set_pos(img_obj, 608, 23);
		}
		else */
		if(i == 0)
		{
			lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("BTAutoConnect"));

			lv_obj_t *img_obj = lv_img_create(btn_obj);
			
			if(phandle->psys_config->bt_auto_connect)
			{
				divoom_menu_control_lv_img_set_src(img_obj, "26/26_on.png");
			}
			else
			{
				divoom_menu_control_lv_img_set_src(img_obj, "26/26_off.png");
			}

			lv_obj_set_pos(img_obj, 608, 23);

			phandle->auto_img_obj = img_obj;
		}
		else if(i == 1)
		{
			lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("BTDeviceName"));

			lv_obj_t *label_obj2 = lv_label_create(btn_obj);
    		//lv_obj_set_pos(label_obj2, 32, 28);
			//lv_obj_set_size(label_obj2, 473, 34);
			lv_obj_align(label_obj2, LV_ALIGN_TOP_RIGHT, -28, 32);
			lv_obj_set_height(label_obj2, 26);
    		lv_obj_set_style_text_color(label_obj2, lv_color_hex(0x0), 0);
    		lv_obj_set_style_text_align(label_obj2, LV_TEXT_ALIGN_RIGHT, 0);
    		lv_obj_set_style_text_font(label_obj2, divoom_menu_control_get_font_info(20, phandle->menu_id), 0); 
			
			if(phandle->local_adapter_name)
			{
				lv_label_set_text(label_obj2, phandle->local_adapter_name);
			}
			else
			{
				lv_label_set_text(label_obj2, "");
			}			

			phandle->bt_name_label = label_obj2;
		}
		else if(i == 2)
		{
			lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("ConnectionStatus"));

			lv_obj_t *label_obj2 = lv_label_create(btn_obj);
    		//lv_obj_set_pos(label_obj2, 32, 28);
			//lv_obj_set_size(label_obj2, 473, 34);
			lv_obj_align(label_obj2, LV_ALIGN_TOP_RIGHT, -28, 32);
			lv_obj_set_height(label_obj2, 26);
    		lv_obj_set_style_text_color(label_obj2, lv_color_hex(0x0), 0);
    		lv_obj_set_style_text_align(label_obj2, LV_TEXT_ALIGN_RIGHT, 0);
    		lv_obj_set_style_text_font(label_obj2, divoom_menu_control_get_font_info(20, phandle->menu_id), 0); 
			
			if(phandle->pcur_bluetooth)
			{
				lv_label_set_text(label_obj2, phandle->pcur_bluetooth->device_name);
			}
			else
			{
				lv_label_set_text(label_obj2, "");
			}			

			phandle->bt_status_label = label_obj2;
		}
		else if(i == 3)
		{
			lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("BTSecret"));

			lv_obj_t *img_obj = lv_img_create(btn_obj);
			divoom_menu_control_lv_img_set_src(img_obj, "26/26_more.png");
			lv_obj_set_pos(img_obj, 644, 27);
		}
		else if(i == 4)
		{
			lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("DisConnectBT"));

			lv_obj_t *img_obj = lv_img_create(btn_obj);
			divoom_menu_control_lv_img_set_src(img_obj, "26/26_more.png");
			lv_obj_set_pos(img_obj, 644, 27);
		}
		else if(i == 5)
		{
			lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("UnpairBT"));

			lv_obj_t *img_obj = lv_img_create(btn_obj);
			divoom_menu_control_lv_img_set_src(img_obj, "26/26_more.png");
			lv_obj_set_pos(img_obj, 644, 27);
		}
	}
    
	//可用网络
    lv_obj_t *bt_label = lv_label_create(phandle->main_obj);
	lv_obj_set_pos(bt_label, 76, 792);
	lv_obj_set_size(bt_label, 310, 32);
    lv_label_set_text(bt_label, divoom_menu_control_get_disp_str("ConnectedDevices"));
    lv_obj_set_style_text_color(bt_label, lv_color_hex(0x636363), 0);
    lv_obj_set_style_text_align(bt_label, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(bt_label, divoom_menu_control_get_font_info(24, phandle->menu_id), 0);    

	for(i = 0;i < phandle->bt_device_num && i < DIVOOM_SYSTEM_SETTING_MAX_BT_NUM; i++)
	{
		lv_obj_t *btn_obj = lv_btn_create(phandle->main_obj);
        lv_obj_add_style(btn_obj, &phandle->btn_normal_style, 0);
        lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUSED);
        lv_obj_add_style(btn_obj, &phandle->btn_select_style, LV_STATE_FOCUS_KEY);
		lv_obj_set_style_pad_all(btn_obj, 0, 0);
        lv_obj_set_pos(btn_obj, 44, 836 + 98 * i);
    	lv_obj_set_size(btn_obj, 712, 90);
		phandle->btn_obj[i] = btn_obj;

		lv_obj_t *label_obj = lv_label_create(btn_obj);
        lv_obj_set_pos(label_obj, 32, 28);
    	lv_obj_set_size(label_obj, 400, 34);
        lv_label_set_text(label_obj, phandle->pbluetooth_list[i].device_name);
        lv_obj_set_style_text_color(label_obj, lv_color_hex(0x0), 0);
        lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
        lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(26, phandle->menu_id), 0);  
			
		lv_obj_t *image_obj = lv_img_create(btn_obj);
    	lv_obj_set_pos(image_obj, 644, 27);
        lv_obj_set_size(image_obj, 36, 36);
        divoom_menu_control_lv_img_set_src(image_obj, "26/26_lang_select.png");
		lv_obj_add_flag(image_obj, LV_OBJ_FLAG_HIDDEN);
        phandle->image_obj[i] = image_obj;
		
		divoom_menu_direct_ctrl_update(phandle->pdirect_handle, btn_obj, 0, i+6);
		lv_obj_add_event_cb(btn_obj, divoom_lvgl_setting_bluetooth_menu_event_cb, LV_EVENT_KEY, phandle);
		
		if(phandle->pcur_bluetooth && strcmp(phandle->pbluetooth_list[i].device_name, phandle->pcur_bluetooth->device_name) == 0)
		{
			lv_obj_clear_flag(image_obj, LV_OBJ_FLAG_HIDDEN);
		}
		//divoom_print("device_name = %s, bt_addr = %s\n", phandle->pbluetooth_list[i].device_name, phandle->pbluetooth_list[i].bt_addr);
	}

    phandle->time_handle = lv_timer_create(divoom_lvgl_setting_bluetooth_timer_handle, 500, phandle);
    
    return phandle;
}

void divoom_lvgl_setting_bluetooth_disp(void *handle, void *active_obj)
{
    Pdivoom_lvgl_setting_bluetooth_ctrl phandle = (Pdivoom_lvgl_setting_bluetooth_ctrl)handle;
    
    if(phandle->first_active_obj)
	{
		lv_group_focus_obj(phandle->first_active_obj);
		phandle->first_active_obj = NULL;
	}
	if(active_obj)
	{
		lv_group_focus_obj(active_obj);
	}

    lv_scr_load(phandle->main_obj);
    
}

void divoom_lvgl_setting_bluetooth_exit(void *handle)
{
    Pdivoom_lvgl_setting_bluetooth_ctrl phandle = (Pdivoom_lvgl_setting_bluetooth_ctrl)handle;
    
	lv_style_reset(&phandle->btn_normal_style);
	lv_style_reset(&phandle->btn_select_style);
	
	if(phandle->time_handle)
	{
		lv_timer_del(phandle->time_handle);
    }
	divoom_menu_direct_ctrl_destory(phandle->pdirect_handle);
	lv_obj_del(phandle->main_obj);
    //lv_style_reset(&phandle->normal_style);
    divoom_system_free(phandle);
    return ;
}



#else
typedef struct _divoom_lvgl_setting_bluetooth_ctrl
{
    int menu_id;
    uint8 bt_status;
    uint8 bt_secret_switch;
    uint8 bt_auto;
    uint8 bt_device_num;
    lv_style_t  normal_style;
    void *time_handle;
    void *main_obj;
    void *back_obj;
    void *auto_connect_obj;
    void *secret_switch_obj;
    void *secret_label_obj;
    void *set_secret_obj;
    void *unpair_obj;
    void *unpair_label_obj;
    void *connect_bt_label_obj;
    void *connect_bt_obj;
}divoom_lvgl_setting_bluetooth_ctrl, *Pdivoom_lvgl_setting_bluetooth_ctrl;

static void divoom_lvgl_setting_bluetooth_menu_event_cb(lv_event_t * e)
{
    lv_obj_t *cur_obj;
    lv_group_t * cur_group_obj = lv_obj_get_group(lv_event_get_target_obj(e));
    Pdivoom_lvgl_setting_bluetooth_ctrl phandle = (Pdivoom_lvgl_setting_bluetooth_ctrl)lv_event_get_user_data(e);
    lv_event_code_t code = lv_event_get_code(e);
    cur_obj = lv_event_get_target_obj(e);
    if(code == LV_EVENT_KEY)
    {
        Pdivoom_sys_setting_info psys_config = divoom_main_get_system_config();
        
        if(phandle->back_obj == cur_obj)
        {
            divoom_menu_control_return_pre();
            return;
        }
        if(phandle->auto_connect_obj == cur_obj)
        {
            if(psys_config->bt_auto_connect)
            {
                if(divoom_bt_music_disable_boot_connect(1) == 0)
                {
                    divoom_menu_control_lv_img_set_src(cur_obj, "26/26_off.png");
                    psys_config->bt_auto_connect = 0;
                    divoom_main_save_config();
                    phandle->bt_auto = 0;
                }
            }
            else
            {
                if(divoom_bt_music_disable_boot_connect(0) == 0)
                {
                    divoom_menu_control_lv_img_set_src(cur_obj, "26/26_on.png");
                    psys_config->bt_auto_connect = 1;
                    divoom_main_save_config();
                    phandle->bt_auto = 1;
                }
            }
            return;
        }
        if(phandle->secret_switch_obj == cur_obj)
        {
            int new_secret;

            new_secret = psys_config->bt_secret ? 0 : 1;
            if(divoom_bt_music_set_pair_pin(new_secret, psys_config->bt_password) != 0)
            {
                return;
            }
            if(new_secret == 0)
            {
                divoom_menu_control_lv_img_set_src(cur_obj, "26/26_off.png");
                psys_config->bt_secret = 0;
                lv_obj_set_style_text_color(phandle->secret_label_obj, lv_color_hex(0x777777), 0);
                lv_obj_clear_flag(phandle->secret_label_obj, LV_OBJ_FLAG_CLICKABLE);
                lv_obj_add_flag(phandle->set_secret_obj, LV_OBJ_FLAG_HIDDEN);
            }
            else
            {
                divoom_menu_control_lv_img_set_src(cur_obj, "26/26_on.png");
                lv_obj_set_style_text_color(phandle->secret_label_obj, lv_color_hex(0xFFFFFF), 0);
                lv_obj_add_flag(phandle->secret_label_obj, LV_OBJ_FLAG_CLICKABLE);
                lv_obj_clear_flag(phandle->set_secret_obj, LV_OBJ_FLAG_HIDDEN);
                psys_config->bt_secret = 1;
            }
            psys_config->bt_secret_switch = psys_config->bt_secret;
            phandle->bt_secret_switch = psys_config->bt_secret;
            divoom_main_save_config();
            return;
        }
        else if(phandle->unpair_obj == cur_obj || phandle->unpair_label_obj == cur_obj)
        {
            phandle->bt_device_num = 0;
            divoom_main_unpair_blue_device();
            divoom_menu_control_lv_img_set_src(phandle->unpair_obj, "26/26_btunpair.png");
            return;
        }
        else if(phandle->connect_bt_obj == cur_obj || phandle->connect_bt_label_obj == cur_obj)
        {
            phandle->bt_status = phandle->bt_status?0:1;
            divoom_main_ctrl_blue_connect(phandle->bt_status);
            if(phandle->bt_status)
            {
                divoom_menu_control_lv_img_set_src(phandle->connect_bt_obj, "26/26_btconn.png");
                lv_label_set_text(phandle->connect_bt_label_obj, divoom_menu_control_get_disp_str("DisConnectBT"));
            }
            else
            {
                divoom_menu_control_lv_img_set_src(phandle->connect_bt_obj, "26/26_btdiscon.png");
                lv_label_set_text(phandle->connect_bt_label_obj, divoom_menu_control_get_disp_str("ConnectBT"));
            }
            return;
        }
        else if(phandle->set_secret_obj == cur_obj)
        {
            //设置四位数密码
            divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_BT_SET_SECRET, NULL, 0, 0);
            return;
        }
    }
}
void divoom_lvgl_setting_bluetooth_timer_handle(struct _lv_timer_t * ptime_info)
{
    Pdivoom_lvgl_setting_bluetooth_ctrl pmenu_handle = (Pdivoom_lvgl_setting_bluetooth_ctrl)lv_timer_get_user_data(ptime_info);    
    Pdivoom_sys_setting_info psys_config = divoom_main_get_system_config();
    uint8 bt_status, bt_device_num;

    bt_status = divoom_main_get_blue_con_status();
    bt_device_num = divoom_main_get_blue_device_num();
    if(bt_status != pmenu_handle->bt_status)
    {
        pmenu_handle->bt_status = bt_status;
        if(bt_status)
        {
            divoom_menu_control_lv_img_set_src(pmenu_handle->connect_bt_obj, "26/26_btconn.png");
            lv_label_set_text(pmenu_handle->connect_bt_label_obj, divoom_menu_control_get_disp_str("DisConnectBT"));
        }
        else
        {
            divoom_menu_control_lv_img_set_src(pmenu_handle->connect_bt_obj, "26/26_btdiscon.png");
            lv_label_set_text(pmenu_handle->connect_bt_label_obj, divoom_menu_control_get_disp_str("ConnectBT"));
        }
    }
    if(bt_device_num != pmenu_handle->bt_device_num)
    {
        pmenu_handle->bt_device_num = bt_device_num;
        if(pmenu_handle->bt_device_num)
        {
            divoom_menu_control_lv_img_set_src(pmenu_handle->unpair_obj , "26/26_btpair.png");
        }
        else
        {
            divoom_menu_control_lv_img_set_src(pmenu_handle->unpair_obj , "26/26_btunpair.png");
        }
    }
    if(pmenu_handle->bt_auto != psys_config->bt_auto_connect)
    {
        pmenu_handle->bt_auto = psys_config->bt_auto_connect;
        if(psys_config->bt_auto_connect)
        {
            divoom_menu_control_lv_img_set_src(pmenu_handle->auto_connect_obj, "26/26_on.png");
        }
        else
        {
            divoom_menu_control_lv_img_set_src(pmenu_handle->auto_connect_obj, "26/26_off.png");
        }
    }
    if(pmenu_handle->bt_secret_switch != psys_config->bt_secret)
    {
        pmenu_handle->bt_secret_switch = psys_config->bt_secret;
        if(psys_config->bt_secret)
        {
            divoom_menu_control_lv_img_set_src(pmenu_handle->auto_connect_obj, "26/26_on.png");
        }
        else
        {
            divoom_menu_control_lv_img_set_src(pmenu_handle->auto_connect_obj, "26/26_off.png");
        }
    }
}

void *divoom_lvgl_setting_bluetooth_init(void *user_data, int menu_id)
{
    Pdivoom_lvgl_setting_bluetooth_ctrl pmenu_handle = divoom_system_malloc(sizeof(divoom_lvgl_setting_bluetooth_ctrl));

    int i;
    void *image_obj, *label_obj, *slide_obj; 
    Pdivoom_sys_setting_info psys_config = divoom_main_get_system_config();
    
    pmenu_handle->menu_id = menu_id;
    pmenu_handle->main_obj = lv_obj_create(NULL);  
    divoom_menu_control_init_lable_style(&pmenu_handle->normal_style);
    lv_obj_set_style_bg_color(pmenu_handle->main_obj, lv_color_hex(0), LV_PART_MAIN);
    lv_obj_set_pos(pmenu_handle->main_obj, 0, 0);
	lv_obj_set_size(pmenu_handle->main_obj, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH);  

    lv_obj_add_style(pmenu_handle->main_obj, &pmenu_handle->normal_style, 0);
    lv_obj_set_scrollbar_mode(pmenu_handle->main_obj, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_scroll_dir(pmenu_handle->main_obj, LV_DIR_VER);

    image_obj = lv_img_create(pmenu_handle->main_obj);
	lv_obj_set_pos(image_obj, 19, 50);
    lv_obj_set_size(image_obj, 78, 78);
    divoom_menu_control_lv_img_set_src(image_obj, "26/26_back.png");
    pmenu_handle->back_obj = image_obj;
    lv_obj_add_flag(image_obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(image_obj, divoom_lvgl_setting_bluetooth_menu_event_cb, LV_EVENT_KEY, pmenu_handle);
    


    image_obj = lv_img_create(pmenu_handle->main_obj);
    lv_obj_set_pos(image_obj, 37, 209);
	lv_obj_set_size(image_obj, 726, 360);
    divoom_menu_control_lv_img_set_src(image_obj, "26/26_4line_bg.png");
    
    label_obj = lv_label_create(pmenu_handle->main_obj);
    lv_obj_set_pos(label_obj, 100, 59);
	lv_obj_set_size(label_obj, 600, 78);
    lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("Bluetooth"));
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(48, pmenu_handle->menu_id), 0);  

    
    
    image_obj = lv_img_create(pmenu_handle->main_obj);
    lv_obj_set_pos(image_obj, 37, 209);
	lv_obj_set_size(image_obj, 726, 270);
    divoom_menu_control_lv_img_set_src(image_obj, "26/26_3line_bg.png");

    label_obj = lv_label_create(pmenu_handle->main_obj);
    lv_obj_set_pos(label_obj, 73, 233);
    lv_obj_set_size(label_obj, 300, 42);
    lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("BTAutoConnect"));
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(32, pmenu_handle->menu_id), 0);  
    
    image_obj = lv_img_create(pmenu_handle->main_obj);
    lv_obj_set_pos(image_obj, 598, 209);
    lv_obj_set_size(image_obj, 165, 90);
    if(psys_config->bt_auto_connect)
    {
        divoom_menu_control_lv_img_set_src(image_obj, "26/26_on.png");
    }
    else
    {
        divoom_menu_control_lv_img_set_src(image_obj, "26/26_off.png");
    }
    pmenu_handle->bt_auto = psys_config->bt_auto_connect;
    lv_obj_add_flag(image_obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(image_obj, divoom_lvgl_setting_bluetooth_menu_event_cb, LV_EVENT_KEY, pmenu_handle);
    pmenu_handle->auto_connect_obj = image_obj;
    
    
    

    label_obj = lv_label_create(pmenu_handle->main_obj);
    lv_obj_set_pos(label_obj, 73, 323);
    lv_obj_set_size(label_obj, 300, 42);
    lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("BTSecret"));
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(32, pmenu_handle->menu_id), 0);  

    image_obj = lv_img_create(pmenu_handle->main_obj);
    lv_obj_set_pos(image_obj, 598, 323);
    lv_obj_set_size(image_obj, 165, 90);
    if(psys_config->bt_secret)
    {
        divoom_menu_control_lv_img_set_src(image_obj, "26/26_on.png");
    }
    else
    {
        divoom_menu_control_lv_img_set_src(image_obj, "26/26_off.png");
    }
    pmenu_handle->bt_secret_switch = psys_config->bt_secret;
    lv_obj_add_flag(image_obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(image_obj, divoom_lvgl_setting_bluetooth_menu_event_cb, LV_EVENT_KEY, pmenu_handle);
    pmenu_handle->secret_switch_obj = image_obj;
    

    label_obj = lv_label_create(pmenu_handle->main_obj);
    lv_obj_set_pos(label_obj, 73, 413);
    lv_obj_set_size(label_obj, 300, 42);
    lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("SetSecret"));
    if(psys_config->bt_secret)
    {
        lv_obj_add_flag(label_obj, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_style_text_color(label_obj, lv_color_hex(0xFFFFFF), 0);
    }
    else
    {
        lv_obj_set_style_text_color(label_obj, lv_color_hex(0x777777), 0);
    }
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(32, pmenu_handle->menu_id), 0);  
    lv_obj_add_event_cb(label_obj, divoom_lvgl_setting_bluetooth_menu_event_cb, LV_EVENT_KEY, pmenu_handle);
    pmenu_handle->secret_label_obj = label_obj;

    image_obj = lv_img_create(pmenu_handle->main_obj);
    lv_obj_set_pos(image_obj, 598, 389);
    lv_obj_set_size(image_obj, 90, 90);
    divoom_menu_control_lv_img_set_src(image_obj, "26/26_more.png");
    if(psys_config->bt_secret == 0)
    {
        lv_obj_add_flag(image_obj, LV_OBJ_FLAG_HIDDEN);
    }
    lv_obj_add_flag(image_obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(image_obj, divoom_lvgl_setting_bluetooth_menu_event_cb, LV_EVENT_KEY, pmenu_handle);
    pmenu_handle->set_secret_obj = image_obj;
    

    


    image_obj = lv_img_create(pmenu_handle->main_obj);
    lv_obj_set_pos(image_obj, 37, 505);
	lv_obj_set_size(image_obj, 726, 180);
    divoom_menu_control_lv_img_set_src(image_obj, "26/26_2line_bg.png");
    
    

    label_obj = lv_label_create(pmenu_handle->main_obj);
    lv_obj_set_pos(label_obj, 73, 529);
    lv_obj_set_size(label_obj, 300, 42);
    lv_label_set_text(label_obj, divoom_menu_control_get_disp_str("UnpairBT"));
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(32, pmenu_handle->menu_id), 0);  
    lv_obj_add_flag(label_obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(label_obj, divoom_lvgl_setting_bluetooth_menu_event_cb, LV_EVENT_KEY, pmenu_handle);
    pmenu_handle->unpair_label_obj = label_obj;
    

    image_obj = lv_img_create(pmenu_handle->main_obj);
    lv_obj_set_pos(image_obj, 598, 505);
    lv_obj_set_size(image_obj, 90, 90);
    pmenu_handle->bt_device_num = divoom_main_get_blue_device_num();
    if(pmenu_handle->bt_device_num)
    {
        divoom_menu_control_lv_img_set_src(image_obj, "26/26_btpair.png");
    }
    else
    {
        divoom_menu_control_lv_img_set_src(image_obj, "26/26_btunpair.png");
    }
    lv_obj_add_flag(image_obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(image_obj, divoom_lvgl_setting_bluetooth_menu_event_cb, LV_EVENT_KEY, pmenu_handle);
    pmenu_handle->unpair_obj = image_obj;
    
    
    label_obj = lv_label_create(pmenu_handle->main_obj);
    lv_obj_set_pos(label_obj, 73, 619);
    lv_obj_set_size(label_obj, 300, 42);
    lv_obj_set_style_text_color(label_obj, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_align(label_obj, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(label_obj, divoom_menu_control_get_font_info(32, pmenu_handle->menu_id), 0); 
    lv_obj_add_flag(label_obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(label_obj, divoom_lvgl_setting_bluetooth_menu_event_cb, LV_EVENT_KEY, pmenu_handle);
    pmenu_handle->connect_bt_label_obj = label_obj;
    

    image_obj = lv_img_create(pmenu_handle->main_obj);
    lv_obj_set_pos(image_obj, 598, 595);
    lv_obj_set_size(image_obj, 90, 90);
    pmenu_handle->connect_bt_obj = image_obj;
    pmenu_handle->bt_status = divoom_main_get_blue_con_status();
    if(pmenu_handle->bt_status)
    {
        divoom_menu_control_lv_img_set_src(pmenu_handle->connect_bt_obj, "26/26_btconn.png");
        lv_label_set_text(pmenu_handle->connect_bt_label_obj, divoom_menu_control_get_disp_str("DisConnectBT"));
    }
    else
    {
        divoom_menu_control_lv_img_set_src(pmenu_handle->connect_bt_obj, "26/26_btdiscon.png");
        lv_label_set_text(pmenu_handle->connect_bt_label_obj, divoom_menu_control_get_disp_str("ConnectBT"));
    }
    lv_obj_add_flag(image_obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(image_obj, divoom_lvgl_setting_bluetooth_menu_event_cb, LV_EVENT_KEY, pmenu_handle);
    
    pmenu_handle->time_handle = lv_timer_create(divoom_lvgl_setting_bluetooth_timer_handle, 1000, pmenu_handle);
    
    return pmenu_handle;
}

void divoom_lvgl_setting_bluetooth_disp(void *handle, void *active_obj)
{
    Pdivoom_lvgl_setting_bluetooth_ctrl phandle = (Pdivoom_lvgl_setting_bluetooth_ctrl)handle;
    
	if(active_obj)
	{
		lv_group_focus_obj(active_obj);
	}
	
    lv_scr_load(phandle->main_obj);
    
}

void divoom_lvgl_setting_bluetooth_exit(void *handle)
{
    Pdivoom_lvgl_setting_bluetooth_ctrl phandle = (Pdivoom_lvgl_setting_bluetooth_ctrl)handle;
    lv_timer_del(phandle->time_handle);
    lv_obj_del(phandle->main_obj);
    lv_style_reset(&phandle->normal_style);
    divoom_system_free(phandle);
    return ;
}
#endif
