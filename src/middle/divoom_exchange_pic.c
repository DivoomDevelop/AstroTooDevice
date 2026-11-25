#include "divoom.h"
#include "divoom_menu_control.h"

typedef struct _divoom_exchange_rate_pic_id_item
{
    uint16 pic_id;
    char   pic_flag[4];
}divoom_exchange_rate_pic_id_item, *Pdivoom_exchange_rate_pic_id_item;

typedef struct _divoom_exchange_rate_currency_name_item
{
    char currency_name[40];
    char pic_flag[4];
}divoom_exchange_rate_currency_name_item, *Pdivoom_exchange_rate_currency_name_item;

#if 1
static const divoom_exchange_rate_currency_name_item divoom_all_currency_name_cn_table[] = 
{
	{"波兰兹罗提", "PLN"},
	{"瑞典克朗", "SEK"},
	{"尼泊尔卢比", "NPR"},
	{"澳门币", "MOP"},
	{"港币", "HKD"},
	{"人民币", "CNY"},
	{"欧元", "EUR"},
	{"智利比索", "CLP"},
	{"印尼盾", "IDR"},
	{"印度卢比", "INR"},
	{"以色列新谢克尔", "ILS"},
	{"越南盾", "VND"},
	{"伊拉克第纳尔", "IQD"},
	{"约旦第纳尔", "JOD"},
	{"英镑", "GBP"},
	{"新加坡元", "SGD"},
	{"土耳其里拉", "TRY"},
	{"新西兰元", "NZD"},
	{"叙利亚镑", "SYP"},
	{"乌干达先令", "UGX"},
	{"文莱元", "BND"},
	{"坦桑尼亚先令", "TZS"},
	{"泰铢", "THB"},
	{"斯里兰卡卢比", "LKR"},
	{"沙特里亚尔", "SAR"},
	{"瑞士法郎", "CHF"},
	{"南非兰特", "ZAR"},
	{"挪威克朗", "NOK"},
	{"墨西哥比索", "MXN"},
	{"摩洛哥迪拉姆", "MAD"},
	{"马来西亚林吉特", "MYR"},
	{"美元", "USD"},
	{"黎巴嫩镑", "LBP"},
	{"老挝基普", "LAK"},
	{"肯尼亚先令", "KES"},
	{"科威特第纳尔", "KWD"},
	{"卡塔尔里亚尔", "QAR"},
	{"柬埔寨瑞尔", "KHR"},
	{"加拿大元", "CAD"},
	{"韩元", "KRW"},
	{"哥斯达黎加科朗", "CRC"},
	{"哥伦比亚比索", "COP"},
	{"菲律宾比索", "PHP"},
	{"俄罗斯卢布", "RUB"},
	{"冰岛克朗", "ISK"},
	{"巴林第纳尔", "BHD"},
	{"巴西雷亚尔", "BRL"},
	{"埃及镑", "EGP"},
	{"阿曼里亚尔", "OMR"},
	{"阿尔及利亚第纳尔", "DZD"},
	{"澳大利亚元", "AUD"},
	{"阿联酋迪拉姆", "AED"},
	{"日元", "JPY"},
};

static const divoom_exchange_rate_currency_name_item divoom_all_currency_name_en_table[] = 
{
	{"Polish złoty", "PLN"},
	{"Swedish Krona", "SEK"},
	{"Nepalese Rupee", "NPR"},
	{"Macanese Pataca", "MOP"},
	{"Hong Kong Dollar", "HKD"},
	{"Renminbi", "CNY"},
	{"Euro", "EUR"},
	{"Chilean Peso", "CLP"},
	{"Indonesian Rupiah", "IDR"},
	{"Indian Rupee", "INR"},
	{"Israeli New Shekel", "ILS"},
	{"Vietnamese Dong", "VND"},
	{"lragi Dinar", "IQD"},
	{"Jordanian Dinar", "JOD"},
	{"British Pound", "GBP"},
	{"Singapore Dollar", "SGD"},
	{"Turkish Lira", "TRY"},
	{"New Zealand Dollar", "NZD"},
	{"Syrian Pound", "SYP"},
	{"Ugandan Shilling", "UGX"},
	{"Brunei Dollar", "BND"},
	{"Tanzanian Shilling", "TZS"},
	{"Thai Baht", "THB"},
	{"Sri Lankan Rupee", "LKR"},
	{"Saudi Riyal", "SAR"},
	{"Swiss Franc", "CHF"},
	{"South Africa", "ZAR"},
	{"Norwegian Krone", "NOK"},
	{"Mexican Peso", "MXN"},
	{"Moroccan Dirham", "MAD"},
	{"Malaysian Ringgit", "MYR"},
	{"United States Dollar", "USD"},
	{"Lebanese Pound", "LBP"},
	{"Lao Kip", "LAK"},
	{"Kenyan Shilling", "KES"},
	{"Kuwaiti Dinar", "KWD"},
	{"Qatari Riyal", "QAR"},
	{"Cambodian Riel", "KHR"},
	{"Canadian Dollar", "CAD"},
	{"South Korean Won", "KRW"},
	{"Costa Rican Colón", "CRC"},
	{"Colombian Peso", "COP"},
	{"Philippine Peso", "PHP"},
	{"Russian Ruble", "RUB"},
	{"Icelandic Króna", "ISK"},
	{"Bahraini Dinar", "BHD"},
	{"Brazilian Real", "BRL"},
	{"Egypt", "EGP"},
	{"Omani Rial", "OMR"},
	{"Algerian Dinar", "DZD"},
	{"Australian Dollar", "AUD"},
	{"United Arab Emirates Dirham", "AED"},
	{"Japanese Yen", "JPY"},
};
#endif


static const divoom_exchange_rate_pic_id_item divoom_all_exchange_rate_table[] = 
{
	{DIVOOM_DISP_EXCHANGE_AED,"AED"},
	{DIVOOM_DISP_EXCHANGE_AUD,"AUD"},
	{DIVOOM_DISP_EXCHANGE_BHD,"BHD"},
	{DIVOOM_DISP_EXCHANGE_BND,"BND"},
	{DIVOOM_DISP_EXCHANGE_BRL,"BRL"},
	{DIVOOM_DISP_EXCHANGE_BUK,"BUK"},
	{DIVOOM_DISP_EXCHANGE_BYR,"BYR"},
	{DIVOOM_DISP_EXCHANGE_CAD,"CAD"},
	{DIVOOM_DISP_EXCHANGE_CHF,"CHF"},
	{DIVOOM_DISP_EXCHANGE_CLP,"CLP"},
	{DIVOOM_DISP_EXCHANGE_CNY,"CNY"},
	{DIVOOM_DISP_EXCHANGE_COP,"COP"},
	{DIVOOM_DISP_EXCHANGE_CRC,"CRC"},
	{DIVOOM_DISP_EXCHANGE_DZD,"DZD"},
	{DIVOOM_DISP_EXCHANGE_EGP,"EGP"},
	{DIVOOM_DISP_EXCHANGE_ESD,"ESD"},
	{DIVOOM_DISP_EXCHANGE_EUR,"EUR"},
	{DIVOOM_DISP_EXCHANGE_GBP,"GBP"},
	{DIVOOM_DISP_EXCHANGE_HKD,"HKD"},
	{DIVOOM_DISP_EXCHANGE_IDR,"IDR"},
	{DIVOOM_DISP_EXCHANGE_ILS,"ILS"},
	{DIVOOM_DISP_EXCHANGE_INR,"INR"},
	{DIVOOM_DISP_EXCHANGE_IQD,"IQD"},
	{DIVOOM_DISP_EXCHANGE_ISK,"ISK"},
	{DIVOOM_DISP_EXCHANGE_JOD,"JOD"},
	{DIVOOM_DISP_EXCHANGE_JPY,"JPY"},
	{DIVOOM_DISP_EXCHANGE_KES,"KES"},
	{DIVOOM_DISP_EXCHANGE_KHR,"KHR"},
	{DIVOOM_DISP_EXCHANGE_KRW,"KRW"},
	{DIVOOM_DISP_EXCHANGE_KWD,"KWD"},
	{DIVOOM_DISP_EXCHANGE_LAK,"LAK"},
	{DIVOOM_DISP_EXCHANGE_LBP,"LBP"},
	{DIVOOM_DISP_EXCHANGE_LKR,"LKR"},
	{DIVOOM_DISP_EXCHANGE_MAD,"MAD"},
	{DIVOOM_DISP_EXCHANGE_MOP,"MOP"},
	{DIVOOM_DISP_EXCHANGE_MXN,"MXN"},
	{DIVOOM_DISP_EXCHANGE_MYR,"MYR"},
	{DIVOOM_DISP_EXCHANGE_NOK,"NOK"},
	{DIVOOM_DISP_EXCHANGE_NZD,"NZD"},
	{DIVOOM_DISP_EXCHANGE_OMR,"OMR"},
	{DIVOOM_DISP_EXCHANGE_PHP,"PHP"},
	{DIVOOM_DISP_EXCHANGE_QAR,"QAR"},
	{DIVOOM_DISP_EXCHANGE_RUB,"RUB"},
	{DIVOOM_DISP_EXCHANGE_SAR,"SAR"},
	{DIVOOM_DISP_EXCHANGE_SGD,"SGD"},
	{DIVOOM_DISP_EXCHANGE_SYP,"SYP"},
	{DIVOOM_DISP_EXCHANGE_THB,"THB"},
	{DIVOOM_DISP_EXCHANGE_TRY,"TRY"},
	{DIVOOM_DISP_EXCHANGE_TWD,"TWD"},
	{DIVOOM_DISP_EXCHANGE_TZS,"TZS"},
	{DIVOOM_DISP_EXCHANGE_UGX,"UGX"},
	{DIVOOM_DISP_EXCHANGE_USD,"USD"},
	{DIVOOM_DISP_EXCHANGE_VND,"VND"},
	{DIVOOM_DISP_EXCHANGE_ZAR,"ZAR"},
	{DIVOOM_DISP_EXCHANGE_ZMK,"ZMK"},
	{DIVOOM_DISP_EXCHANGE_NPR, "NPR"},
	{DIVOOM_DISP_EXCHANGE_SEK, "SEK"},
    {DIVOOM_DISP_EXCHANGE_PLN, "PLN"},
};

static const divoom_exchange_rate_pic_id_item divoom_all_contry_pic_table[] = 
{
	{DIVOOM_DISP_EXCHANGE_ALB,    "ALB"},
	{DIVOOM_DISP_EXCHANGE_AUT,    "AUT"},
	{DIVOOM_DISP_EXCHANGE_BEL,    "BEL"},
	{DIVOOM_DISP_EXCHANGE_BUL,    "BUL"},
	{DIVOOM_DISP_EXCHANGE_DEN,    "DEN"},
	{DIVOOM_DISP_EXCHANGE_ESP,    "ESP"},
	{DIVOOM_DISP_EXCHANGE_EST,    "EST"},
	{DIVOOM_DISP_EXCHANGE_GRE,    "GRE"},
	{DIVOOM_DISP_EXCHANGE_HUN,    "HUN"},
	{DIVOOM_DISP_EXCHANGE_IRL,    "IRL"},
	{DIVOOM_DISP_EXCHANGE_LAT,    "LAT"},
	{DIVOOM_DISP_EXCHANGE_LTU,    "LTU"},
	{DIVOOM_DISP_EXCHANGE_LUX,    "LUX"},
	{DIVOOM_DISP_EXCHANGE_MKD,    "MKD"},
	{DIVOOM_DISP_EXCHANGE_MON,    "MON"},
	{DIVOOM_DISP_EXCHANGE_ROU,    "ROU"},
	{DIVOOM_DISP_EXCHANGE_SLO,    "SLO"},
	{DIVOOM_DISP_EXCHANGE_SRB,    "SRB"},
	{DIVOOM_DISP_EXCHANGE_UKR,    "UKR"},
	{DIVOOM_DISP_EXCHANGE_GER,"GER"},    //德国
	{DIVOOM_DISP_EXCHANGE_SWE,"SWE"},   //瑞典
	{DIVOOM_DISP_EXCHANGE_NED,"NED"},   //荷兰
	{DIVOOM_DISP_EXCHANGE_ITA,"ITA"},   //意大利
	{DIVOOM_DISP_EXCHANGE_FRA,"FRA"},   //法国
	{DIVOOM_DISP_EXCHANGE_CZE,"CZE"},   //捷克
	{DIVOOM_DISP_EXCHANGE_SVK,"SVK"},    //斯洛伐克
	{DIVOOM_DISP_EXCHANGE_BLR,"BLR"},   //白俄罗斯
	{DIVOOM_DISP_EXCHANGE_HUN,"HUN"},   //匈牙利
	{DIVOOM_DISP_EXCHANGE_FIN,"FIN"},   //芬兰
	{DIVOOM_DISP_EXCHANGE_POL,"POL"},   //波兰   
	{DIVOOM_DISP_EXCHANGE_CNY,"CHN"},
	{DIVOOM_DISP_EXCHANGE_RUB,"ROC"},
	{DIVOOM_DISP_EXCHANGE_JPY,"JPN"},
	{DIVOOM_DISP_EXCHANGE_CHF,"SUI"},
	{DIVOOM_DISP_EXCHANGE_CAD,"CAN"},
	{DIVOOM_DISP_EXCHANGE_USD,"USA"},
	{DIVOOM_DISP_EXCHANGE_KRW,"KOR"},
	{DIVOOM_DISP_EXCHANGE_AUD,"AUS"},
	{DIVOOM_DISP_EXCHANGE_NOK,"NOR"},
	{DIVOOM_DISP_EXCHANGE_NZD,"NZL"},
	{DIVOOM_DISP_EXCHANGE_DZD,"ALG"},
	{DIVOOM_DISP_EXCHANGE_OMR,"OMA"},
	{DIVOOM_DISP_EXCHANGE_AED,"UAE"},
	{DIVOOM_DISP_EXCHANGE_EGP,"EGY"},
	{DIVOOM_DISP_EXCHANGE_BRL,"BRL"},
	{DIVOOM_DISP_EXCHANGE_BHD,"BRN"},
	{DIVOOM_DISP_EXCHANGE_ISK,"ISN"},
	{DIVOOM_DISP_EXCHANGE_PHP,"PHI"},
	{DIVOOM_DISP_EXCHANGE_COP,"COL"},
	{DIVOOM_DISP_EXCHANGE_CRC,"CRC"},
	{DIVOOM_DISP_EXCHANGE_KHR,"CAM"},
	{DIVOOM_DISP_EXCHANGE_QAR,"QAT"},
	{DIVOOM_DISP_EXCHANGE_KWD,"KWT"},
	{DIVOOM_DISP_EXCHANGE_KES,"KEN"},
	{DIVOOM_DISP_EXCHANGE_LAK,"LAO"},
	{DIVOOM_DISP_EXCHANGE_LBP,"LBN"},
	{DIVOOM_DISP_EXCHANGE_MYR,"MYS"},
	{DIVOOM_DISP_EXCHANGE_MAD,"MAR"},
	{DIVOOM_DISP_EXCHANGE_MXN,"MEX"},
	{DIVOOM_DISP_EXCHANGE_ZAR,"ZAF"},
	{DIVOOM_DISP_EXCHANGE_SAR,"SAU"},
	{DIVOOM_DISP_EXCHANGE_LKR,"LKA"},
	{DIVOOM_DISP_EXCHANGE_THB,"THA"},
	{DIVOOM_DISP_EXCHANGE_TZS,"TZA"},
	{DIVOOM_DISP_EXCHANGE_BND,"BRN"},
	{DIVOOM_DISP_EXCHANGE_UGX,"UGA"},
	{DIVOOM_DISP_EXCHANGE_SYP,"SYR"},
	{DIVOOM_DISP_EXCHANGE_TRY,"TUR"},
	{DIVOOM_DISP_EXCHANGE_SGD,"SGP"},
	{DIVOOM_DISP_EXCHANGE_GBP,"GBR"},
	{DIVOOM_DISP_EXCHANGE_JOD,"JOR"},
	{DIVOOM_DISP_EXCHANGE_IQD,"IRQ"},
	{DIVOOM_DISP_EXCHANGE_VND,"VNM"},
	{DIVOOM_DISP_EXCHANGE_ILS,"ILR"},
	{DIVOOM_DISP_EXCHANGE_INR,"IND"},
	{DIVOOM_DISP_EXCHANGE_IDR,"IDN"},
	{DIVOOM_DISP_EXCHANGE_CLP,"CHL"},
	{DIVOOM_DISP_EXCHANGE_HKD,"HKG"},
	{DIVOOM_DISP_EXCHANGE_TWD,"TWN"},
	{DIVOOM_DISP_EXCHANGE_MOP,"MOP"},
	{DIVOOM_DISP_EXCHANGE_NPR, "NPR"},
	{DIVOOM_DISP_EXCHANGE_SEK, "SEK"},
    {DIVOOM_DISP_EXCHANGE_PLN, "PLN"},
};

#if 1

void divoom_exchange_pic_save(Pdivoom_exchange_rate_pic_item pitem, int number)
{
    
    char *json_info;
    divoom_cJSON *root, *list, *item;

	root = divoom_cJSON_CreateObject();

    if(root == NULL)
    {
        return;
    }
    
	divoom_cJSON_AddItemToObject(root, "List", list = divoom_cJSON_CreateArray());
    for(int i = 0; i < number; i++)
    {
		divoom_cJSON_AddItemToArray(list, item = divoom_cJSON_CreateObject());  
        
        divoom_cJSON_AddNumberToObject(item , "id", pitem[i].id); 
        divoom_cJSON_AddStringToObject(item , "flag", pitem[i].flag); 
        divoom_cJSON_AddStringToObject(item , "image_url", pitem[i].image_url); 
    }
    json_info = divoom_cJSON_Print(root);
    divoom_cJSON_Delete(root);
    divoom_app_com_write_file(DIVOOM_ALL_EXCHANGE_RATE_PIC_SAVE_PATH, (uint8 *)json_info, strlen(json_info));
    divoom_system_free(json_info);

}

Pdivoom_exchange_rate_pic_item divoom_exchange_pic_load(int *number)
{
    char *file_buf;
    divoom_cJSON *root, *list, *item;
    Pdivoom_exchange_rate_pic_item pitem;


    file_buf = divoom_app_com_read_file(DIVOOM_ALL_EXCHANGE_RATE_PIC_SAVE_PATH, NULL);
    if(file_buf == NULL)
    {
        /* 主配置缺失时读取出厂备份；更新仍只写主配置。 */
        file_buf = divoom_app_com_read_file(DIVOOM_ALL_EXCHANGE_RATE_PIC_SAVE_BAK_PATH, NULL);
        if(file_buf == NULL)
        {
            return NULL;
        }
    }
    root = divoom_cJSON_Parse(file_buf);
    divoom_system_free(file_buf);
    if(root == NULL)
    {
        return NULL;
    }

    list = divoom_cJSON_GetObjectItem(root, "List");
    *number = divoom_cJSON_GetArraySize(list);
    pitem = divoom_system_malloc(*number * sizeof(divoom_exchange_rate_pic_item));
    for(int i = 0; i < *number; i++)
    {
        item = divoom_cJSON_GetArrayItem(list, i);
        pitem[i].id = divoom_cJSON_GetObjectInt(item, "id"); 
        divoom_cJSON_GetObjectString(item, "flag", pitem[i].flag);    
        divoom_cJSON_GetObjectString(item, "image_url", pitem[i].image_url);  
    }
    divoom_cJSON_Delete(root);
    return pitem;
}


int divoom_exchange_pic_compare_ex_flag(void *item1, void *item2)
{
    char *exchange_rate = (char *)item1;
    Pdivoom_exchange_rate_pic_item pdest_item = (Pdivoom_exchange_rate_pic_item)item2;

    return strcmp(exchange_rate, pdest_item->flag);
}

int divoom_exchange_pic_compare_id(void *item1, void *item2)
{
    Pdivoom_exchange_rate_pic_item psrc_item = (Pdivoom_exchange_rate_pic_item)item1;
    Pdivoom_exchange_rate_pic_item pdest_item = (Pdivoom_exchange_rate_pic_item)item2;

    return psrc_item->id != pdest_item->id;
}

void divoom_exchange_pic_get_all_list(void)
{
	/* 主配置和出厂备份都不存在时才重新向服务器拉取。 */
	if (divoom_app_com_check_file_exist(DIVOOM_ALL_EXCHANGE_RATE_PIC_SAVE_PATH) == 0 &&
		divoom_app_com_check_file_exist(DIVOOM_ALL_EXCHANGE_RATE_PIC_SAVE_BAK_PATH) == 0)
	{
		divoom_net_app_send_packet(DIVOOM_NET_COMM_GET_ALL_EXCHANGE_RATE_LIST, DIVOOM_SERVER_TOPIC, NULL);
	}
}

/*********************************************************************
  Function:        divoom_exchange_pic_update_list
  Description:     更新汇率列表
  Input:           plist
                   num
                    
  Return:          NONE
**********************************************************************/

void divoom_exchange_pic_update_list(Pdivoom_exchange_rate_pic_item plist, int num)
{
    #if 1
    int i,  j, update_flag = 0, cur_number = 0, last_number;
    uint32 image_id;
    Pdivoom_exchange_rate_pic_item pcur_list, pnew_list;
    divoom_manage_image_item_info image_item;

    pcur_list = divoom_exchange_pic_load(&cur_number);
    pnew_list = divoom_system_malloc(sizeof(divoom_exchange_rate_pic_item) * (cur_number + num));
    if(pcur_list)
    {
        memcpy(pnew_list, pcur_list, sizeof(divoom_exchange_rate_pic_item) *cur_number);
        divoom_system_free(pcur_list);
    }
    last_number = cur_number;
    
    for(i = 0; i < num; i ++)
    {
        for(j = 0; j < cur_number; j++)
        {
            if(plist[i].id == pnew_list[j].id)
            {
                if(strcmp(plist[i].image_url, pnew_list[j].image_url))
                {
                    //更新配置，先删除原来的文件，再添加下载新文件
                    image_id = divoom_exchange_pic_get_id(plist[i].flag);
                    divoom_manage_image_del_direct(DIVOOM_MANAGE_IMAGE_CLOCK_DIAL_APP_PIC, &image_id, 1);
                    strcpy(pnew_list[j].image_url, plist[i].image_url);
                    image_item.flag = 1;
                    image_item.image_id = image_id;
                    strcpy(image_item.file_addr, plist[i].image_url);
                    image_item.pcallback = divoom_app_com_finishcallback;
                    divoom_manage_image_add(DIVOOM_MANAGE_IMAGE_CLOCK_DIAL_APP_PIC, &image_item, 1);
    //                divoom_manage_image_clear_image_cache(image_id);
                    update_flag = 1;
                }
                break;
            }
        }
        if(j >= cur_number)
        {
            //增加下载项
            memcpy(&pnew_list[last_number], &plist[i], sizeof(divoom_exchange_rate_pic_item));
            last_number ++;
            image_item.flag = 1;
            image_item.image_id = divoom_exchange_pic_get_id(plist[i].flag);
			divoom_manage_image_del_direct(DIVOOM_MANAGE_IMAGE_CLOCK_DIAL_APP_PIC, &image_item.image_id, 1);
            strcpy(image_item.file_addr, plist[i].image_url);
            image_item.pcallback = divoom_app_com_finishcallback;
            divoom_manage_image_add(DIVOOM_MANAGE_IMAGE_CLOCK_DIAL_APP_PIC, &image_item, 1);
//                divoom_manage_image_add_image_cache(image_item.image_id);
            update_flag = 1;
        }
    }

    if(update_flag)
    {
        divoom_exchange_pic_save(pnew_list, last_number);
    }
    divoom_system_free(pnew_list);
    #endif
}
//#else

#if 1
void divoom_exchange_get_currency_name(char *exchage_flag, char *currency_name)
{
    int i = 0;
    int lang = divoom_main_set_lang_flag(-1);

	if (lang == DIVOOM_LVGL_MENU_LANG_SC)
	{
		for(i = 0; i < sizeof(divoom_all_currency_name_cn_table)/sizeof(divoom_all_currency_name_cn_table[0]); i++)
	    {
	        if(memcmp(exchage_flag, divoom_all_currency_name_cn_table[i].pic_flag, 3) == 0)
	        {
				strcpy(currency_name, divoom_all_currency_name_cn_table[i].currency_name);
	        }
	    }
	}
	else
	{
		for(i = 0; i < sizeof(divoom_all_currency_name_en_table)/sizeof(divoom_all_currency_name_en_table[0]); i++)
		{
			if(memcmp(exchage_flag, divoom_all_currency_name_en_table[i].pic_flag, 3) == 0)
			{
				strcpy(currency_name, divoom_all_currency_name_en_table[i].currency_name);
			}
		}
	}
}
#endif

/*********************************************************************
  Function:        divoom_exchange_pic_get_id
  Description:     得到汇率图片id
  Input:           exchage_flag  汇率标记
                    
  Return:          图片ＩＤ
**********************************************************************/

int divoom_exchange_pic_get_id(char *exchage_flag)
{
    int i = 0, image_id = 0;
	static int exchange_request_time = 0;
	char temp_flag[4] = {0};

	memcpy(temp_flag, exchage_flag, strlen(exchage_flag));
	if (divoom_main_get_server_type() == 0)
	{
		if (strstr(exchage_flag, "MOP") || strstr(exchage_flag, "HKD"))
		{
			strcpy(temp_flag, "CNY");
		}
	}

    for(i = 0; i < sizeof(divoom_all_exchange_rate_table)/sizeof(divoom_all_exchange_rate_table[0]); i++)
    {
        if(memcmp(temp_flag, divoom_all_exchange_rate_table[i].pic_flag, 3) == 0)
        {
            image_id = DIVOOM_MANAGE_IMAGE_ID_BASE_LOCAL_START_GIF + divoom_all_exchange_rate_table[i].pic_id;
        }
    }

	//检测一下该文件是否存在，不存在则进行下载
	if (image_id)
	{
		char file_name[128] = {0};
		divoom_manage_image_download_get_file_name(DIVOOM_MANAGE_IMAGE_CLOCK_DIAL_APP_PIC, image_id, file_name);
		if (divoom_port_file_exists((char *)file_name))
		{
	        return image_id; 
		}

		if (exchange_request_time < divoom_system_get_ticks())
		{
			exchange_request_time = divoom_system_get_ticks() + 3;
			divoom_net_app_send_packet(DIVOOM_NET_COMM_GET_ALL_EXCHANGE_RATE_LIST, DIVOOM_SERVER_TOPIC, NULL);
		}
	}

	return image_id;
 
}


/*********************************************************************
  Function:        divoom_contry_pic_get_id
  Description:     得到国家图片id
  Input:           contry_flag  国家三个缩写标记
                    
  Return:          图片ＩＤ
**********************************************************************/

int divoom_contry_pic_get_id(char *contry_flag)
{
    int i = 0, image_id = 0;
	static int country_request_time = 0;

    for(i = 0; i < sizeof(divoom_all_contry_pic_table)/sizeof(divoom_all_contry_pic_table[0]); i++)
    {
        if(memcmp(contry_flag, divoom_all_contry_pic_table[i].pic_flag, 3) == 0)
        {
            image_id = DIVOOM_MANAGE_IMAGE_ID_BASE_LOCAL_START_GIF + divoom_all_contry_pic_table[i].pic_id;
        }
    }

	//检测一下该文件是否存在，不存在则进行下载
	if (image_id)
	{
		char file_name[128] = {0};
		divoom_manage_image_download_get_file_name(DIVOOM_MANAGE_IMAGE_CLOCK_DIAL_APP_PIC, image_id, file_name);
		if (divoom_port_file_exists((char *)file_name))
		{
			return image_id; 
		}
	
		if (country_request_time < divoom_system_get_ticks())
		{
			country_request_time = divoom_system_get_ticks() + 3;
			divoom_net_app_send_packet(DIVOOM_NET_COMM_GET_ALL_EXCHANGE_RATE_LIST, DIVOOM_SERVER_TOPIC, NULL);
		}
	}

    return image_id;
    
}

#endif
