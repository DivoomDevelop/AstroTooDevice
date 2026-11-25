#include "divoom.h"
#include "lvgl.h"
#include "divoom_light_app.h"
#include "divoom_menu_control.h"

typedef struct _divoom_lvgl_setting_set_language_ctrl
{
    int menu_id;
    int idle_time;
    void *pdialog_handle;
    void *handle;
    void *time_handle;
}divoom_lvgl_setting_set_language_ctrl, *Pdivoom_lvgl_setting_set_language_ctrl;


static const divoom_select_language_menu_item pdivoom_lang_list[] =
{
    {DIVOOM_LVGL_MENU_LANG_ENGLISH,"English"},  //英语
    {DIVOOM_LVGL_MENU_LANG_SC, "SC"},      //简体中文
    {DIVOOM_LVGL_MENU_LANG_TC,  "TC"},     //繁体
    {DIVOOM_LVGL_MENU_LANG_JP, "JP"},      //日语
    {DIVOOM_LVGL_MENU_LANG_Germany,"Germany"},  //德国
    {DIVOOM_LVGL_MENU_LANG_Thai,"Thai"},     //泰文
    {DIVOOM_LVGL_MENU_LANG_Russian,"Russian"},  //俄语
    {DIVOOM_LVGL_MENU_LANG_Korean,"Korean"},   //韩国人
    {DIVOOM_LVGL_MENU_LANG_Vietnam, "Vietnam"}, //越南
    {DIVOOM_LVGL_MENU_LANG_French,"French"},   //法语
    {DIVOOM_LVGL_MENU_LANG_Turkey,  "TR"},     //土耳其
    {DIVOOM_LVGL_MENU_LANG_Spanish,"Spanish"}, //西班牙
    {DIVOOM_LVGL_MENU_LANG_Portuguese,"Portuguese"},   //葡萄牙的
    {DIVOOM_LVGL_MENU_LANG_Indonesian,  "IN"},     //印尼语
    {DIVOOM_LVGL_MENU_LANG_Italy, "Italy"},   //意大利
    {DIVOOM_LVGL_MENU_LANG_Ukraine, "Ukraine"}, //乌克兰
    {DIVOOM_LVGL_MENU_LANG_Swedish, "SV"},   //瑞典
    {DIVOOM_LVGL_MENU_LANG_Danish, "DA"},   //丹麦语
    {DIVOOM_LVGL_MENU_LANG_Poland, "Polska"},  
};

const divoom_select_language_menu_item *divoom_lvgl_set_language_get_list(void)
{
    return pdivoom_lang_list;
}

int divoom_lvgl_set_language_get_list_count(void)
{
    return DIVOOM_LVGL_MENU_LANG_END;
}

typedef char divoom_lvgl_lang_list_count_must_match_end[
    (sizeof(pdivoom_lang_list) / sizeof(pdivoom_lang_list[0]) == DIVOOM_LVGL_MENU_LANG_END) ? 1 : -1];

char *divoom_lvgl_set_language_btn_img_str[] = {
	"43/43_English.png",
	"43/43_SC.png",
	"43/43_TC.png",
	"43/43_JP.png",
	"43/43_Germany.png",
	"43/43_Thai.png",
	"43/43_Russian.png",
	"43/43_Korean.png",
	"43/43_Vietnam.png",
	"43/43_French.png",
	"43/43_TR.png",
	"43/43_Spanish.png",
	"43/43_Portuguese.png",
	"43/43_IN.png",
	"43/43_Italy.png",
	"43/43_Ukraine.png",
	"43/43_SV.png",
	"43/43_DA.png",
};

char *divoom_lvgl_set_language_label_str[] = {
	"Press the second large knob to confirm the system language",
	"按下第二个大旋钮，确认系统语言",
	"按下第二個大旋鈕，確認系統語言",
	"2番目の大きなノブを押して、システム言語を確定してください",
	"Drücken Sie den zweiten großen Drehknopf, um die Systemsprache zu bestätigen",
	"กดปุ่มหมุนอันที่สองเพื่อยืนยันภาษาระบบ",
	"Нажмите вторую большую ручку, чтобы подтвердить язык системы",
	"두 번째 큰 노브를 눌러 시스템 언어를 확인하세요",
	"Nhấn núm lớn thứ hai để xác nhận ngôn ngữ hệ thống",
	"Appuyez sur le deuxième grand bouton pour confirmer la langue du système",	
	"Sistem dilini onaylamak için ikinci büyük düğmeye basın",
	"Pulsa el segundo mando grande para confirmar el idioma del sistema",
	"Pressione o segundo botão grande para confirmar o idioma do sistema",
	"Tekan kenop besar kedua untuk konfirmasi bahasa sistem",
	"Premi il secondo grande pulsante per confermare la lingua di sistema",
	"Натисніть другу велику ручку, щоб підтвердити мову системи",
	"Tryck på den andra stora vredet för att bekräfta systemspråket",
	"Tryk på den anden store drejeknap for at bekræfte systemsproget",	
	"Naciśnij drugie duże przycisk, aby potwierdzić język systemu",
};

int divoom_lvgl_setting_set_language_callback(void *handle, int ret_value, uint8 *select_info, int item_num)
{
    int i;
    Pdivoom_lvgl_setting_set_language_ctrl phandle = (Pdivoom_lvgl_setting_set_language_ctrl)handle;

    for(i = 0; i < item_num; i++)
    {
        if(select_info[i])
        {
            Pdivoom_sys_setting_info psys_config = divoom_main_get_system_config();

            if(psys_config->lang != pdivoom_lang_list[i].lang_id)
            {
                psys_config->lang = pdivoom_lang_list[i].lang_id;
				divoom_main_send_sys_settings_config();
                if(divoom_main_get_wifi_config_flag())
                {
                    divoom_main_save_config();
                }
                divoom_menu_control_set_language(psys_config->lang);
            }
            return divoom_menu_control_menu_items() > 1;
        }
    }
    return divoom_menu_control_menu_items() > 1;
}

void divoom_lvgl_setting_set_language_select_callback(void *handle, int select_item)
{
    Pdivoom_sys_setting_info psys_config = divoom_main_get_system_config();
    
    if(psys_config->lang != pdivoom_lang_list[select_item].lang_id)
    {
        psys_config->lang = pdivoom_lang_list[select_item].lang_id;

		divoom_main_send_sys_settings_config();
        divoom_menu_control_set_language(psys_config->lang);
        if(divoom_menu_control_menu_get_first_menu_id() == DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_LANGUGE)
        {
            divoom_light_app_change_default_clock(psys_config->lang);
        }
        if(divoom_main_set_wifi_config_status(-1))
        {
            divoom_main_save_config();
            divoom_menu_control_return_pre();
        }
        else
        {
            divoom_menu_control_load_next_pop_cur(DIVOOM_MENU_CONTROL_ID_CONFIG_WIFI, NULL, 0, 0);
        }
    }
}
void divoom_lvgl_setting_set_language_time_handle(struct _lv_timer_t *user_data)
{
    Pdivoom_lvgl_setting_set_language_ctrl phandle = (Pdivoom_lvgl_setting_set_language_ctrl)lv_timer_get_user_data(user_data);
    
    divoom_print("idle time: %d, %d\n", phandle->idle_time, divoom_system_get_tickms());
    phandle->idle_time ++;

    if(phandle->idle_time >= 10)
    {
		#if 0
        divoom_main_enter_stanby_mode();
        divoom_port_delay_ms((1) * 1000U);
        exit(0);
		#endif
        divoom_menu_control_load_next(DIVOOM_MENU_CONTROL_ID_PRODUCT_STRESS_TESTING_MENU, NULL, 0, 0);
    }
    
}

void *divoom_lvgl_setting_set_language_init(void *user_data, int menu_id)
{
    Pdivoom_lvgl_setting_set_language_ctrl phandle;
    divoom_lvgl_normal_menu_list_info para_info = {0};
    Pdivoom_sys_setting_info psys_config = divoom_main_get_system_config();

    phandle = divoom_system_malloc(sizeof(divoom_lvgl_setting_set_language_ctrl));
    phandle->menu_id = menu_id;
    para_info.phandle = phandle;
    //para_info.bg_width = 726;
    //para_info.bg_high = 1620;
    //para_info.bg_image_path = "26/26_language_bg.png";   //背景图片
    para_info.disp_title = divoom_menu_control_get_disp_str("Language");   //标题
    para_info.select_flag = 1;	 //是否有选择按键
    para_info.confirm_flag = 0;  //是否有确认按键
    para_info.muti_flag = 0;     //是否支持多选
    para_info.btn_image_flag = 1;
	para_info.title_image_flag = 1;
	para_info.title_image_path = "43/43_language.png";
		
	para_info.title_tip_flag = 1;

    if(psys_config->lang < 0)
    {
        psys_config->lang  = 0;
    }
    para_info.menu_items = sizeof(pdivoom_lang_list)/sizeof(pdivoom_lang_list[0]);    //菜单项总数
    for(int i = 0; i < para_info.menu_items; i++)
    {
        if(psys_config->lang == pdivoom_lang_list[i].lang_id)
        {
            para_info.select_info[i] = 1;
        }
        else
        {
            para_info.select_info[i] = 0;
        }
        para_info.select_str[i] = divoom_menu_control_get_disp_str(pdivoom_lang_list[i].lang_info);   //下面菜单选择项
        //para_info.btn_image_path[i] = divoom_lvgl_set_language_btn_img_str[i];

		if(para_info.title_tip_flag)
		{
			para_info.title_tip[i] = divoom_lvgl_set_language_label_str[i];
		}
    }
    para_info.pcallback = divoom_lvgl_setting_set_language_callback;
    para_info.pchange_callback = divoom_lvgl_setting_set_language_select_callback;

    phandle->handle = divoom_lvgl_normal_menu_list_init(&para_info, menu_id);
    if(divoom_main_get_wifi_config_flag() == 0)
    {
        phandle->time_handle = lv_timer_create(divoom_lvgl_setting_set_language_time_handle, 60*1000, phandle);
    }

    return phandle;
}

void divoom_lvgl_setting_set_language_disp(void *handle, void *active_obj)
{
    Pdivoom_lvgl_setting_set_language_ctrl phandle = (Pdivoom_lvgl_setting_set_language_ctrl)handle;

    divoom_lvgl_normal_menu_list_disp(phandle->handle, active_obj);
}

void divoom_lvgl_setting_set_language_exit(void *handle)
{
    Pdivoom_lvgl_setting_set_language_ctrl phandle = (Pdivoom_lvgl_setting_set_language_ctrl)handle;

    divoom_lvgl_normal_menu_list_exit(phandle->handle);
    if(phandle->time_handle)
    {
        lv_timer_del(phandle->time_handle);
    }
    divoom_system_free(phandle);
}

