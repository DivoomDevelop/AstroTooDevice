#include "divoom_disp_com.h"

typedef struct _divoom_disp_test_ctrl
{
    uint32 task_id;
    uint8 mode;
    uint8 disp_id;
    uint8 disp_index;
    uint32 sys_tick;
    uint32 step_tick;
    uint32 total_tick;
    uint32 cnt;
    uint8  disp_buf[SCREEN_BUF_SIZE_64];
}divoom_disp_test_ctrl, *Pdivoom_disp_test_ctrl;

static Pdivoom_disp_test_ctrl pdivoom_disp_test_handle = NULL;

static const uint8 divoom_disp_special_left_to_right_pos_info[SCREEN_HEIGHT_32][SCREEN_WIDTH_32] =
{
   {14,13,12,11,10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 0,}, 
   {15,14,13,12,11,10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0,}, 
   {16,15,14,13,12,11,10, 9, 8, 7, 6, 5, 4, 3, 2, 1,}, 
   {17,16,15,14,13,12,11,10, 9, 8, 7, 6, 5, 4, 3, 2,}, 
   {18,17,16,15,14,13,12,11,10, 9, 8, 7, 6, 5, 4, 3,}, 
   {19,18,17,16,15,14,13,12,11,10, 9, 8, 7, 6, 5, 4,}, 
   {20,19,18,17,16,15,14,13,12,11,10, 9, 8, 7, 6, 5,}, 
   {21,20,19,18,17,16,15,14,13,12,11,10, 9, 8, 7, 6,}, 
   {22,21,20,19,18,17,16,15,14,13,12,11,10, 9, 8, 7,}, 
   {23,22,21,20,19,18,17,16,15,14,13,12,11,10, 9, 8,}, 
   {24,23,22,21,20,19,18,17,16,15,14,13,12,11,10, 9,}, 
   {25,24,23,22,21,20,19,18,17,16,15,14,13,12,11,10,}, 
   {26,24,24,23,22,21,20,19,18,17,16,15,14,13,12,11,}, 
   {27,26,24,24,23,22,21,20,19,18,17,16,15,14,13,12,}, 
   {28,27,26,25,24,23,22,21,20,19,18,17,16,15,14,13,}, 
   {28,28,27,26,25,24,23,22,21,20,19,18,17,16,15,14,},
};

void divoom_disp_time_set_pixel(uint16 pos, uint8 *data, Pdivoom_screen_color_info color);


int system_get_temp_info(void)
{
	return 0;//温度修正
}

/*************************************************
功能为先亮白，红绿蓝，逐行显示白色
**************************************************/
void divoom_disp_test_light_disp(uint8 index)
{
    divoom_screen_color_info color;
    if(index == 0)
    {
        color.r_c = 0xfF;
        color.g_c = 0xfF;
        color.b_c = 0xfF;
        divoom_disp_ctrl_fill(pdivoom_disp_test_handle->disp_buf, &color);
    }
    else if(index == 1)
    {
        color.r_c = 0xfF;
        color.g_c = 0x00;
        color.b_c = 0x00;
        divoom_disp_ctrl_fill(pdivoom_disp_test_handle->disp_buf, &color);
    }
    else if(index == 2)
    {
        color.r_c = 0x00;
        color.g_c = 0xfF;
        color.b_c = 0x00;
        divoom_disp_ctrl_fill(pdivoom_disp_test_handle->disp_buf, &color);
    }
    else if(index == 3)
    {
        color.r_c = 0x00;
        color.g_c = 0x00;
        color.b_c = 0xfF;
        divoom_disp_ctrl_fill(pdivoom_disp_test_handle->disp_buf, &color);
    }
    else
    {
        uint8 i, j, off;

        for(i = 0, off = 0; i < SCREEN_HEIGHT_32; i ++)
        {
            for(j = 0; j < SCREEN_WIDTH_32; j++, off ++)
            {
                if(((divoom_disp_special_left_to_right_pos_info[i][j] + index) % 4) < 2)
                {
                    color.r_c = 0xfF;
                    color.g_c = 0xfF;
                    color.b_c = 0xfF;
                }
                else
                {
                    color.r_c = 0x00;
                    color.g_c = 0x00;
                    color.b_c = 0x00;
                }
                divoom_disp_ctrl_set_data(off, &color, pdivoom_disp_test_handle->disp_buf);
            }
        }
    }
    divoom_screen_disp_rgb32_data(pdivoom_disp_test_handle->disp_buf);
}
void divoom_disp_test_run_light_disp(uint8 index)
{
    divoom_screen_color_info color;
    
    {
        uint8 i, j, off;

        for(i = 0, off = 0; i < SCREEN_HEIGHT_32; i ++)
        {
            for(j = 0; j < SCREEN_WIDTH_32; j++, off ++)
            {
                if(((divoom_disp_special_left_to_right_pos_info[i][j] + index) % 4) < 2)
                {
                    color.r_c = 0xfF;
                    color.g_c = 0xfF;
                    color.b_c = 0xfF;
                }
                else
                {
                    color.r_c = 0x00;
                    color.g_c = 0x00;
                    color.b_c = 0x00;
                }
                divoom_disp_ctrl_set_data(off, &color, pdivoom_disp_test_handle->disp_buf);
            }
        }
    }
    divoom_screen_disp_rgb32_data(pdivoom_disp_test_handle->disp_buf);
}


void divoom_test_prog_light_bias_run(uint32 index)
{
	int16 x ,y, row;
    int16 k = 0;
    divoom_screen_color_info color = {0xff,0xff,0xff};
	memset(pdivoom_disp_test_handle->disp_buf,0,sizeof(pdivoom_disp_test_handle->disp_buf));
    k = index % (SCREEN_WIDTH_64+SCREEN_HEIGHT_64)-1;    
	for (row = 0; row < 2; row++)
	{
		for (x = k + row; x >= 0; x--)
		{
			y = k + row - x;
//			divoom_disp_ctrl_set_data_xy(x, y, &color, pdivoom_disp_test_handle->disp_buf);
			if(x >= 0 && y >= 0)
				divoom_disp_ctrl_set_dot_64(x, y, &color, pdivoom_disp_test_handle->disp_buf);
		}
	}
}

void divoom_test_prog_light_multi_bias_run(T_U32 index)
{
	int16 x ,y,i;
    int16 k = 0;
    divoom_screen_color_info color = {0xff,0xff,0xff};
	uint8 run_table[] = {1,1,0,0,0,0,0,0};
	uint8 num = DIVOOM_ARRAY_LEN(run_table);
	memset(pdivoom_disp_test_handle->disp_buf,0,sizeof(pdivoom_disp_test_handle->disp_buf));
	index %= num;
	for(i = 0; i < SCREEN_WIDTH_64+SCREEN_HEIGHT_64-1;i++)
	{	
		k =(i+index)%num;
		if(run_table[k])
		{
			for (x = i; x >= 0; x--)
			{
				y = i - x;
				if(x >= 0 && y >= 0)
					divoom_disp_ctrl_set_dot_64(x, y, &color, pdivoom_disp_test_handle->disp_buf);
			}
		}
	}
}



//extern const uint8 TimeInfoStr[10][3*5];
extern const uint8 TimeInfoStr[10][4*7];
void divoom_disp_test_digit(uint8 x, uint8 y, uint8 width, uint8 height, const uint8 * buf)
{
    uint8 i, j, off, dight_off = 0;
    divoom_screen_color_info color = {0XfF, 0XfF, 0XfF};
    
    off = y * SCREEN_WIDTH_32;
    for(i = 0; i < height; i ++, off += SCREEN_WIDTH_32, dight_off += width)
    {
        for(j = 0; j < width; j ++)
        {
            if(buf[dight_off + j])
            {
                divoom_disp_ctrl_set_data(off + x + j,&color,pdivoom_disp_test_handle->disp_buf);
            }
        }
    }            
}

void divoom_disp_test_temp(void)
{
    divoom_screen_color_info color = {0XfF, 0XfF, 0XfF};
    int value = system_get_temp_info();

    divoom_print("divoom_disp_test_temp: %d\n", value);
    memset(pdivoom_disp_test_handle->disp_buf, 0, SCREEN_BUF_SIZE_32); 
    divoom_disp_ctrl_set_data(SCREEN_WIDTH_32 * 3 -1, &color,pdivoom_disp_test_handle->disp_buf);
    divoom_disp_test_digit(2, 3, 3, 5,TimeInfoStr[value / 10]);
    divoom_disp_test_digit(6, 3, 3, 5,TimeInfoStr[value % 10]);
}


void divoom_disp_test_bat(void)
{
    divoom_disp_ctrl_bat_ctrl bat_info;
    uint8 disp_buf[DIVOOM_IMAGE_SMALL_RGB_SIZE];

    divoom_disp_ctrl_app_get_env_info(DIVOOM_DISP_ENV_BAT, &bat_info);
    divoom_disp_event_display_battery(bat_info.bat_level, 0, disp_buf);    
//    divoom_com_large_pic_to32(disp_buf, pdivoom_disp_test_handle->disp_buf);
	divoom_disp_ctrl_pic_extension(pdivoom_disp_test_handle->disp_buf,disp_buf,DIVOOM_IMAGE_SMALL_WIDTH,2);

}

void divoom_disp_test_key_disp(uint8 key_id)
{
    uint16 key_test_id[] = {DIVOOM_DISP_DIGIT_1_PIC, DIVOOM_DISP_DIGIT_2_PIC, DIVOOM_DISP_DIGIT_3_PIC, DIVOOM_DISP_DIGIT_4_PIC, DIVOOM_DISP_DIGIT_5_PIC, };

    divoom_disp_picture_get_rgb_data(key_test_id[key_id], pdivoom_disp_test_handle->disp_buf);
}


void divoom_disp_test_fill_pic_statu_color(uint16 pic_id,uint8 *disp_buf,uint8 statu)
{
    divoom_screen_color_info color_right = {0, 0xff, 0x00},color_error = {0xff, 0, 0x00},black_color = {0,0,0},color;
    uint16 i;
    divoom_disp_picture_get_rgb_data(pic_id,disp_buf);
    if(0 == statu)
    {
        memcpy(&color_right,&color_error,sizeof(divoom_screen_color_info));
    }    
    for(i = 0; i < SCREEN_DOT_NUM;i++)
    {
        divoom_disp_ctrl_get_data(i,&color,disp_buf);
        if(0 != memcmp(&color,&black_color,sizeof(divoom_screen_color_info)))
        {
            divoom_disp_ctrl_set_data(i, &color_right, disp_buf);
        }
    }    
}

#define DIVOOM_DISP_TEST_TEMP_DIGIT 	DIVOOM_4X7_DIGIT
static const uint16 divoom_disp_test_temp_map[] = {0x67,0x68,0x8,0x8,0x8,0x8,0x7};	//7x7
static const uint16 divoom_disp_test_temp_minus_map[] = {0x7};	//3x1  负号
void divoom_disp_test_draw_temp(int8 temp,uint8 *disp_buf)
{
	divoom_screen_color_info color = {0xff, 0xff, 0xff};
	if(temp<0)
	{
		temp = -temp;
		divoom_disp_game_font_fill_16area(0,16,3,1,(uint16 *)divoom_disp_test_temp_map,disp_buf,&color);
	}
	if(temp > 99)
	{
		divoom_disp_digit_fill_area(5, 13, temp/100,DIVOOM_DISP_TEST_TEMP_DIGIT, disp_buf,&color);
		temp%=100;
		divoom_disp_digit_fill_area(5+7, 13, temp/10,DIVOOM_DISP_TEST_TEMP_DIGIT, disp_buf,&color);
		temp%=10;
		divoom_disp_digit_fill_area(5+14, 13, temp,DIVOOM_DISP_TEST_TEMP_DIGIT, disp_buf,&color);
	}else if(temp > 9)
	{
		divoom_disp_digit_fill_area(5+7, 13, temp/10,DIVOOM_DISP_TEST_TEMP_DIGIT, disp_buf,&color);
		temp%=10;
		divoom_disp_digit_fill_area(5+14, 13, temp,DIVOOM_DISP_TEST_TEMP_DIGIT, disp_buf,&color);
	}
	else
	{
		divoom_disp_digit_fill_area(5+14, 13, temp,DIVOOM_DISP_TEST_TEMP_DIGIT, disp_buf,&color);
	}
	divoom_disp_game_font_fill_16area(24,13,7,7,(uint16 *)divoom_disp_test_temp_map,disp_buf,&color);
}

/**
显示5位数字
**/
void divoom_disp_test_show_6num(uint32 num,divoom_screen_color_info *pcolor,uint8 *disp_buf)
{
#if 1

    divoom_disp_digit_fill_area(1, 2, num/100000%10, DIVOOM_3X5_DIGIT, disp_buf,pcolor);
    divoom_disp_digit_fill_area(6, 2, num/10000%10, DIVOOM_3X5_DIGIT, disp_buf,pcolor);
    divoom_disp_digit_fill_area(11, 2 ,num/1000%10, DIVOOM_3X5_DIGIT, disp_buf,pcolor);
    divoom_disp_digit_fill_area(1, 8 ,num/100%10, DIVOOM_3X5_DIGIT, disp_buf,pcolor);
    divoom_disp_digit_fill_area(6, 8 ,num/10%10, DIVOOM_3X5_DIGIT, disp_buf,pcolor);
    divoom_disp_digit_fill_area(11, 8 ,num%10, DIVOOM_3X5_DIGIT, disp_buf,pcolor);
#else
	divoom_disp_digit_fill_area(0, 13, num / 10000, DIVOOM_DISP_TEST_TEMP_DIGIT, disp_buf,&color);
	num %= 10000;
	divoom_disp_digit_fill_area(7, 13, num / 1000, DIVOOM_DISP_TEST_TEMP_DIGIT, disp_buf,&color);
	num %= 1000;
	divoom_disp_digit_fill_area(14, 13, num / 100, DIVOOM_DISP_TEST_TEMP_DIGIT, disp_buf,&color);
	num %= 100;
	divoom_disp_digit_fill_area(21, 13, num / 10, DIVOOM_DISP_TEST_TEMP_DIGIT, disp_buf,&color);
	num %= 10;
	divoom_disp_digit_fill_area(28, 13, num, DIVOOM_DISP_TEST_TEMP_DIGIT, disp_buf,&color);
#endif
}

//32X5  wifi riss
static const uint8 divoom_test_font_wifi_rssi[] = {0x51,0x17,0x27,0xCB,0x55,0x11,0xA5,0x28,0x55,0x13,0xA7,0xEB,0x55,0x11,0x25,0x8A,0x4E,0x11,0xA9,0x69};
//32X5  version
static const uint8 divoom_test_font_version[] = {0xEA,0x8E,0x75,0x09,0x2A,0x4A,0x54,0x4B,0x6A,0xCE,0x55,0x0D,0x2A,0x0A,0x55,0x49,0xE4,0xD2,0x74,0x09};

void divoom_disp_test_show_wifi_rssi(int8 value)
{
	divoom_screen_color_info color = {0xff, 0xff, 0xff};
	divoom_screen_color_info r_color = {0xff, 0, 0};
	divoom_screen_color_info g_color = {0, 0xff, 0};
	divoom_screen_color_info *pcolor = NULL;
	uint8 *disp_buf = pdivoom_disp_test_handle->disp_buf;

	pcolor = &r_color;
	divoom_disp_font_fill_any_area(0,2,32,5,divoom_test_font_wifi_rssi,disp_buf,&color);

	if(value < 0)
	{
		value = -value;
		if(value < 60)
		{
			pcolor = &g_color;
		}
		divoom_disp_game_font_fill_16area(6,18,3,1,(uint16 *)divoom_disp_test_temp_minus_map,disp_buf,pcolor);
	}
    divoom_disp_digit_fill_area(11, 16 ,value/100%10, DIVOOM_3X5_DIGIT, disp_buf,pcolor);
    divoom_disp_digit_fill_area(15, 16 ,value/10%10, DIVOOM_3X5_DIGIT, disp_buf,pcolor);
    divoom_disp_digit_fill_area(20, 16 ,value%10, DIVOOM_3X5_DIGIT, disp_buf,pcolor);
}

void divoom_update_json_check_file_status(int *good_num, int *bad_num, int *total_num);

void divoom_disp_test_show_version(uint32 version)
{
    int total_num, good_num, bad_num;
	divoom_screen_color_info color = {0xff, 0xff, 0xff}, bad_color = {0xff, 0, 0};
	uint8 *disp_buf = pdivoom_disp_test_handle->disp_buf;

	divoom_disp_font_fill_any_area(0,2,32,5,divoom_test_font_version,disp_buf,&color);

    divoom_disp_digit_fill_area(1, 12, version/100000%10, DIVOOM_3X5_DIGIT, disp_buf,&color);
    divoom_disp_digit_fill_area(6, 12, version/10000%10, DIVOOM_3X5_DIGIT, disp_buf,&color);
    divoom_disp_digit_fill_area(11, 12,version/1000%10, DIVOOM_3X5_DIGIT, disp_buf,&color);
    divoom_disp_digit_fill_area(16, 12,version/100%10, DIVOOM_3X5_DIGIT, disp_buf,&color);
    divoom_disp_digit_fill_area(21, 12,version/10%10, DIVOOM_3X5_DIGIT, disp_buf,&color);
    divoom_disp_digit_fill_area(26, 12,version%10, DIVOOM_3X5_DIGIT, disp_buf,&color);

    divoom_update_json_check_file_status(&good_num, &bad_num, &total_num);
    divoom_disp_digit_fill_area(1, 19, total_num/100%10, DIVOOM_3X5_DIGIT, disp_buf,&color);
    divoom_disp_digit_fill_area(6, 19, total_num/10%10, DIVOOM_3X5_DIGIT, disp_buf,&color);
    divoom_disp_digit_fill_area(11, 19,total_num%10, DIVOOM_3X5_DIGIT, disp_buf,&color);
    
    divoom_disp_digit_fill_area(1, 26, good_num/100%10, DIVOOM_3X5_DIGIT, disp_buf,&color);
    divoom_disp_digit_fill_area(6, 26, good_num/10%10, DIVOOM_3X5_DIGIT, disp_buf,&color);
    divoom_disp_digit_fill_area(11, 26,good_num%10, DIVOOM_3X5_DIGIT, disp_buf,&color);
    
    divoom_disp_digit_fill_area(18, 26, bad_num/100%10, DIVOOM_3X5_DIGIT, disp_buf,&bad_color);
    divoom_disp_digit_fill_area(23, 26, bad_num/10%10, DIVOOM_3X5_DIGIT, disp_buf,&bad_color);
    divoom_disp_digit_fill_area(28, 26,bad_num%10, DIVOOM_3X5_DIGIT, disp_buf,&bad_color);
    
}




void divoom_disp_test_main(void *data)
{
    pdivoom_disp_test_handle->sys_tick ++;
    pdivoom_disp_test_handle->step_tick ++;
    if(pdivoom_disp_test_handle->sys_tick >= pdivoom_disp_test_handle->total_tick)
    {
        pdivoom_disp_test_handle->sys_tick = 0;
        pdivoom_disp_test_handle->disp_index = 0;
    }
    switch(pdivoom_disp_test_handle->mode)
    {
        case DIVOOM_DISP_TEST_DISP_VERSION:
        {
			uint32 version = DIVOOM_SOFTWARE_VERSION;
			memset(pdivoom_disp_test_handle->disp_buf,0,sizeof(pdivoom_disp_test_handle->disp_buf));
			divoom_disp_test_show_version(version);
        }
            break;
            
        case DIVOOM_DISP_TEST_RUN_LIGHT_ITEM:
//			divoom_test_prog_light_bias_run(pdivoom_disp_test_handle->cnt);
			divoom_test_prog_light_multi_bias_run(pdivoom_disp_test_handle->cnt);
			/*
            if(pdivoom_disp_test_handle->step_tick >= 7)
            {
                pdivoom_disp_test_handle->step_tick = 0;
                divoom_disp_test_run_light_disp(pdivoom_disp_test_handle->cnt);
				pdivoom_disp_test_handle->disp_index ++;
            }
			*/
            break;
            
        case DIVOOM_DISP_TEST_TEMP_BAT_ITEM:
            if(pdivoom_disp_test_handle->step_tick >= 10)
            {
                divoom_disp_test_bat();
                pdivoom_disp_test_handle->disp_index ++;
                pdivoom_disp_test_handle->step_tick = 0;
            }
            break;
		case DIVOOM_DISP_TEST_TEMP:
			{
				int8 temp = 30;// divoom_thermistor_get_temp();
				memset(pdivoom_disp_test_handle->disp_buf,0,sizeof(pdivoom_disp_test_handle->disp_buf));
				divoom_disp_test_draw_temp(temp,pdivoom_disp_test_handle->disp_buf);
			}
			break;            
		case DIVOOM_DISP_TEST_WIFI_RSSI_ITEM:
			{
				int8 rssi = divoom_main_get_wifi_rssi();
				memset(pdivoom_disp_test_handle->disp_buf,0,sizeof(pdivoom_disp_test_handle->disp_buf));
				divoom_disp_test_show_wifi_rssi(rssi);
			}
			break;

        case DIVOOM_DISP_TEST_KEY_ITEM:
            break;
            
        case DIVOOM_DISP_TEST_MIC_ITEM:
			return;
            break;

		case DIVOOM_DISP_TEST_BUZZER_ITEM:
			break;
            
        case DIVOOM_DISP_TEST_FM_ITEM:
            break;
            
        case DIVOOM_DISP_TEST_LINEIN_ITEM:
            break;
            
        case DIVOOM_DISP_TEST_CHARGE_ITEM:
            break;
            
        case DIVOOM_DISP_TEST_BLUE_ITEM:
            break;
            
    }
	pdivoom_disp_test_handle->cnt++;


	switch(pdivoom_disp_test_handle->mode)
	{
		case DIVOOM_DISP_TEST_RUN_LIGHT_ITEM:
			{
				divoom_screen_disp_rgb_data(pdivoom_disp_test_handle->disp_buf);
			}
			break;

		case DIVOOM_DISP_TEST_WIFI_ITEM:
			break;
		default:
	    	divoom_screen_disp_rgb32_data(pdivoom_disp_test_handle->disp_buf);			
			break;
	}
/*
	if(DIVOOM_DISP_TEST_RUN_LIGHT_ITEM == pdivoom_disp_test_handle->mode)
	{
		divoom_screen_disp_rgb_data(pdivoom_disp_test_handle->disp_buf);
	}
	else
	{
    	divoom_screen_disp_rgb32_data(pdivoom_disp_test_handle->disp_buf);
	}
*/
}

T_BOOL divoom_product_check_voice(void)
{
	return 1;
}
T_BOOL rtc_is_external_osc(void)
{
	return 1;
}

T_BOOL divoom_disp_test_set_mode(uint8 mode, uint8 status)
{
    uint8 ret = 1;
    
    pdivoom_disp_test_handle->mode = mode;
    switch(pdivoom_disp_test_handle->mode)
    {
        case DIVOOM_DISP_TEST_DISP_VERSION:
        {
			uint32 version = DIVOOM_SOFTWARE_VERSION;
			memset(pdivoom_disp_test_handle->disp_buf,0,sizeof(pdivoom_disp_test_handle->disp_buf));
			divoom_disp_test_show_version(version);
        }
            break;
            
        case DIVOOM_DISP_TEST_ALL_WHITE_ITEM:
        {
            divoom_screen_color_info color = {0xff, 0xff, 0xff};
            divoom_disp_ctrl_fill(pdivoom_disp_test_handle->disp_buf,&color);
            break;
        }   
            
        case DIVOOM_DISP_TEST_RED_LIGHT_ITEM:
            pdivoom_disp_test_handle->total_tick = 5 * 7;
            divoom_disp_test_light_disp(1);
            return ret;
            
        case DIVOOM_DISP_TEST_GREEN_LIGHT_ITEM:
            pdivoom_disp_test_handle->total_tick = 5 * 7;
            divoom_disp_test_light_disp(2);
            return ret;
            
        case DIVOOM_DISP_TEST_BLUE_LIGHT_ITEM:
            pdivoom_disp_test_handle->total_tick = 5 * 7;
            divoom_disp_test_light_disp(3);
            return ret;
            
        case DIVOOM_DISP_TEST_RUN_LIGHT_ITEM:
            pdivoom_disp_test_handle->total_tick = 5 * 7;
			pdivoom_disp_test_handle->cnt = 0;
            divoom_test_prog_light_bias_run(pdivoom_disp_test_handle->cnt);
//            divoom_disp_test_run_light_disp(pdivoom_disp_test_handle->disp_index);
            return ret;
            
            
        case DIVOOM_DISP_TEST_TEMP_BAT_ITEM:
            pdivoom_disp_test_handle->total_tick = 200;
            divoom_disp_test_bat();
            return ret;
            
        case DIVOOM_DISP_TEST_KEY_ITEM:
            divoom_disp_test_key_disp(status);
            break;
            
        case DIVOOM_DISP_TEST_MIC_ITEM:
			memset(pdivoom_disp_test_handle->disp_buf,0,sizeof(pdivoom_disp_test_handle->disp_buf));
           // divoom_disp_picture_get_rgb_data(DIVOOM_DISP_TALK_WAIT_PIC,pdivoom_disp_test_handle->disp_buf);
            break;
            
        case DIVOOM_DISP_TEST_FM_ITEM:
          //  divoom_disp_picture_get_rgb_data(DIVOOM_DISP_FM_MODE_PIC, pdivoom_disp_test_handle->disp_buf);
            break;
            

        case DIVOOM_DISP_TEST_SD_ITEM:
//					0:没插卡 	1：插卡挂载成功 2 插卡挂载失败

            if(0 == status)
            {
				divoom_disp_picture_get_rgb_data(DIVOOM_DISP_TEST_SD_PULL_OUT_PIC,pdivoom_disp_test_handle->disp_buf);
            }
            else if(1 == status)
            {
				divoom_disp_picture_get_rgb_data(DIVOOM_DISP_TEST_SD_PULL_IN_PIC,pdivoom_disp_test_handle->disp_buf);
            }
            else if(2 == status)
            {
				divoom_disp_picture_get_rgb_data(DIVOOM_DISP_TEST_SD_ERR_PIC,pdivoom_disp_test_handle->disp_buf);
            }

            break;   

        case DIVOOM_DISP_TEST_LINEIN_ITEM:
            if(status)
            {
               // divoom_disp_picture_get_rgb_data(DIVOOM_DISP_TEST_LINE_IN_PIC,pdivoom_disp_test_handle->disp_buf);
            }
            else
            {
               // divoom_disp_picture_get_rgb_data(DIVOOM_DISP_TEST_LINE_OUT_PIC,pdivoom_disp_test_handle->disp_buf);
            }
            break;
         
        case DIVOOM_DISP_TEST_CHARGE_ITEM:
            if(status)
            {
                divoom_disp_picture_get_rgb_data(DIVOOM_DISP_TEST_USB_IN_PIC,pdivoom_disp_test_handle->disp_buf);
            }
            else
            {
                divoom_disp_picture_get_rgb_data(DIVOOM_DISP_TEST_USB_OUT_PIC,pdivoom_disp_test_handle->disp_buf);
            }
            break;
			
		case DIVOOM_DISP_TEST_BUZZER_ITEM:
            divoom_disp_picture_get_rgb_data(DIVOOM_DISP_ALARM_CLOCK_ADD_PIC,pdivoom_disp_test_handle->disp_buf);
			break;
			
        case DIVOOM_DISP_TEST_RTC_ITEM:
			/*
            if(rtc_is_external_osc())
            {
                divoom_screen_color_info color = {0x00, 0xff, 0x00};
                divoom_disp_ctrl_fill(pdivoom_disp_test_handle->disp_buf, &color);
                //divoom_disp_picture_get_rgb_data(DIVOOM_DISP_BT_CONNECT_PIC,pdivoom_disp_test_handle->disp_buf);
            }
            else
            {
                divoom_screen_color_info color = {0xff, 0x00, 0x00};
                divoom_disp_ctrl_fill(pdivoom_disp_test_handle->disp_buf, &color);
                ret = 1;
                //divoom_disp_picture_get_rgb_data(DIVOOM_DISP_TEST_BLUE_ERR, pdivoom_disp_test_handle->disp_buf);
            }
			*/
            if(status)
            {
                divoom_disp_test_fill_pic_statu_color(DIVOOM_DISP_TEST_MODE_RTC,pdivoom_disp_test_handle->disp_buf,1);
            }
            else
            {
                divoom_disp_test_fill_pic_statu_color(DIVOOM_DISP_TEST_MODE_RTC,pdivoom_disp_test_handle->disp_buf,0);            
            }
            break;

        case DIVOOM_DISP_TEST_WIFI_ITEM:
            if(status)
            {
                divoom_disp_picture_display(DIVOOM_DISP_WIFI_MODE_PIC);
            }
            else
            {
                divoom_disp_picture_display(DIVOOM_DISP_WIFI_OFF_PIC);
            }
            return ret;
            break;

		case DIVOOM_DISP_TEST_WIFI_RSSI_ITEM:
			{
				memset(pdivoom_disp_test_handle->disp_buf,0,sizeof(pdivoom_disp_test_handle->disp_buf));
			}
			break;


        case DIVOOM_DISP_TEST_UPDATE_ITEM:
            if(status == DIVOOM_OTA_SERVER_UPGRADE_FILE_DOWNLOADING)
            {
                //divoom_disp_picture_display(DIVOOM_DISP_UPDATE_PIC);
				divoom_disp_picture_get_rgb_data(DIVOOM_DISP_UPDATE_PIC,pdivoom_disp_test_handle->disp_buf);
            }
            else if(status == DIVOOM_OTA_SERVER_UPGRADE_FILE_OK || status == DIVOOM_OTA_SERVER_UPGRADE_FILE_READY)
            {
                //divoom_disp_picture_display(DIVOOM_DISP_DOWN_PIC2TV_00);
				divoom_disp_picture_get_rgb_data(DIVOOM_DISP_UPDATE_PIC,pdivoom_disp_test_handle->disp_buf);
            }
            else if(status == DIVOOM_OTA_SERVER_UPGRADE_FILE_ERR)
            {
                //divoom_disp_picture_display(DIVOOM_DISP_SD_ERR_PIC);
				divoom_disp_picture_get_rgb_data(DIVOOM_DISP_TEST_ERROR_PIC,pdivoom_disp_test_handle->disp_buf);
            }
            
            break;

            
        case DIVOOM_DISP_TEST_BLUE_ITEM:
            if(status)
            {
                divoom_disp_test_fill_pic_statu_color(DIVOOM_DISP_TEST_MODE_BLUE,pdivoom_disp_test_handle->disp_buf,1);                
            }
            else
            {
                divoom_disp_test_fill_pic_statu_color(DIVOOM_DISP_TEST_MODE_BLUE,pdivoom_disp_test_handle->disp_buf,0);
                //divoom_disp_picture_get_rgb_data(DIVOOM_DISP_TEST_BLUE_ERR, pdivoom_disp_test_handle->disp_buf);
            }
            break;
		case DIVOOM_DISP_TEST_SHOWNUM:
		{
			uint32 num = status;
			divoom_screen_color_info color = {0xff, 0xff, 0xff};
			memset(pdivoom_disp_test_handle->disp_buf,0,sizeof(pdivoom_disp_test_handle->disp_buf));
			divoom_disp_test_show_6num(num,&color,pdivoom_disp_test_handle->disp_buf);
		}
			break;

            
    }
	switch(pdivoom_disp_test_handle->mode)
	{
		case DIVOOM_DISP_TEST_RUN_LIGHT_ITEM:
			{
				divoom_screen_disp_rgb_data(pdivoom_disp_test_handle->disp_buf);
			}
			break;

		case DIVOOM_DISP_TEST_WIFI_ITEM:
			break;
		default:
	    	divoom_screen_disp_rgb32_data(pdivoom_disp_test_handle->disp_buf);			
			break;
	}
/*
	if(DIVOOM_DISP_TEST_RUN_LIGHT_ITEM == pdivoom_disp_test_handle->mode)
	{
		divoom_screen_disp_rgb_data(pdivoom_disp_test_handle->disp_buf);
	}
	else
	{
    	divoom_screen_disp_rgb32_data(pdivoom_disp_test_handle->disp_buf);
	}
*/
    return ret;
}

void divoom_disp_test_update(Pdivoom_disp_ctrl_product_test_info light_data)
{
	if(light_data)
	{
		divoom_disp_test_set_mode(light_data->mode,light_data->status);
	}
	else
	{
		divoom_disp_test_set_mode(DIVOOM_DISP_TEST_DISP_VERSION,0);
	}
}



void divoom_disp_test_init(Pdivoom_disp_ctrl_product_test_info light_data)
{
	divoom_malloc_check_all_ram_in();
	if(pdivoom_disp_test_handle)
	{
		divoom_disp_test_update(light_data);
		return;
	}
    pdivoom_disp_test_handle = divoom_com_malloc(sizeof(divoom_disp_test_ctrl),__FILE__, __LINE__);
    if(pdivoom_disp_test_handle == NULL)
    {
        divoom_print("%s:%d: it fails to malloc!\n",__FILE__, __LINE__);
        return ;
    }
    memset(pdivoom_disp_test_handle, 0, sizeof(divoom_disp_test_ctrl));
    pdivoom_disp_test_handle->task_id = divoom_com_create_task( divoom_disp_test_main, 100);
    divoom_com_resume_task(pdivoom_disp_test_handle->task_id);
	divoom_disp_test_update(light_data);
}




void divoom_disp_test_close(void)
{
    divoom_com_pause_task(pdivoom_disp_test_handle->task_id);
    divoom_com_destroy_task(pdivoom_disp_test_handle->task_id);
    divoom_com_free(pdivoom_disp_test_handle,__func__,__LINE__);
    pdivoom_disp_test_handle = NULL;
	divoom_malloc_check_all_ram_out(__func__);
}


