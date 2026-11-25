#include "divoom.h"
#include "divoom_oneline_led.h"
#include "divoom_oneline_led_gamma.h"

typedef struct _divoom_oneline_led_ctrl
{

    uint8 refresh_flag;
    uint8 gamma_type;
	uint8 light_lev;							//灯光开光
	uint8 on_off;								//灯光开关
	uint8 disp_buf[ONELINE_SCREEN_BUF_SIZE];		//存储显示数据的BUF
	uint8 refresh_buf[ONELINE_SCREEN_BUF_SIZE];		//刷新中使用的数据BUF
    uint8 *gamma_handle;						//GAMMA表对应的指针
}divoom_oneline_led_ctrl,*Pdivoom_oneline_led_ctrl;

Pdivoom_oneline_led_ctrl pdivoom_oneline_led_ctrl_handle = NULL;

void divoom_oneline_led_refresh_choice_gamma(uint8 lev)
{
	lev %= (sizeof(divoom_oneline_led_gamma_lev)/sizeof(divoom_oneline_led_gamma_lev[0]));
    if(pdivoom_oneline_led_ctrl_handle->gamma_type)
    {
//	    pdivoom_oneline_led_ctrl_handle->gamma_handle = divoom_oneline_led_gamma_lev_one_one[lev];
    }
    else
    {
	    pdivoom_oneline_led_ctrl_handle->gamma_handle = (uint8 *)divoom_oneline_led_gamma_lev[lev];
    }
}

void divoom_oneline_led_send_rgb_data(uint8 *disp_buf)
{
	uint8 i = 0;
	divoom_led_control_led_info pled_info[DIVOOM_ONELINE_LED_NUM] = {0};

	if(pdivoom_oneline_led_ctrl_handle == NULL)
		return;

	uint8 *refresh_buf = pdivoom_oneline_led_ctrl_handle->refresh_buf;
	if (pdivoom_oneline_led_ctrl_handle->on_off == 0)
	{
		memset(refresh_buf, 0x0, ONELINE_SCREEN_BUF_SIZE);		
	}
	else
	{
	    switch(pdivoom_oneline_led_ctrl_handle->light_lev)
	    {
	        case 0:
	            memset(refresh_buf, 0x0, ONELINE_SCREEN_BUF_SIZE);
	            break;
	        default:
                for(i =0; i <ONELINE_SCREEN_BUF_SIZE; i++)
                {
                    if(disp_buf[i] < sizeof(divoom_oneline_led_gamma_7))    //增加一个异常处理
                    {
					    pdivoom_oneline_led_ctrl_handle->refresh_buf[i] = pdivoom_oneline_led_ctrl_handle->gamma_handle[disp_buf[i]];
                    }
                }
	            break;
	    }
	}

	for (i = 0; i < DIVOOM_ONELINE_LED_NUM; i++)
	{
		pled_info[i].r = refresh_buf[i*3];
		pled_info[i].g = refresh_buf[i*3 + 1];
		pled_info[i].b = refresh_buf[i*3 + 2];
	}
	divoom_led_control_all_rgb(pled_info);
}

/*****************************************************************************
 Prototype    : divoom_oneline_led_set_bright
 Description  : 设置亮度
 Input        : uint8 lev  
 Output       : None
 Return Value : 
 
  History        :
  1.Date         : 2020/8/3
    Author       : coronaa
    Modification : Created function
*****************************************************************************/
void divoom_oneline_led_set_bright(uint8 lev)
{
    int raw_lev = lev,max_level = DIVOOM_ARRAY_LEN(divoom_oneline_led_gamma_lev);
    
	if(pdivoom_oneline_led_ctrl_handle == NULL)
		return;
    pdivoom_oneline_led_ctrl_handle->on_off = 1;
    lev = lev * max_level / 100;
	if(lev > DIVOOM_ARRAY_LEN(divoom_oneline_led_gamma_lev))
		lev = DIVOOM_ARRAY_LEN(divoom_oneline_led_gamma_lev);
    if(lev == 0 && raw_lev)
    {
        lev = 1;
    }
	pdivoom_oneline_led_ctrl_handle->light_lev = lev;
	if(lev != 0)
	{
		divoom_oneline_led_refresh_choice_gamma(lev-1);
	}
    pdivoom_oneline_led_ctrl_handle->refresh_flag = 1;
}

void divoom_oneline_led_on_off(uint8 flag)
{
	if(pdivoom_oneline_led_ctrl_handle == NULL)
		return;

	if (flag)
	{
	    pdivoom_oneline_led_ctrl_handle->on_off = 1;
	}
	else
	{
	    pdivoom_oneline_led_ctrl_handle->on_off = 0;
	}
}

void divoom_oneline_led_init(int switch_off)
{
	if(pdivoom_oneline_led_ctrl_handle)
		return;
	pdivoom_oneline_led_ctrl_handle = (divoom_oneline_led_ctrl *)divoom_system_malloc(sizeof(divoom_oneline_led_ctrl));
	if(NULL == pdivoom_oneline_led_ctrl_handle)
	{
		divoom_print("malloc fail!\n");
		return;
	}
	memset(pdivoom_oneline_led_ctrl_handle,0,sizeof(divoom_oneline_led_ctrl));
    if(switch_off)
    {
    	pdivoom_oneline_led_ctrl_handle->on_off = 1;
    	divoom_oneline_led_set_bright(ONELINE_SCREEN_BRIGHT_DEFAULT_LEV);
    }
}

void divoom_oneline_led_close(void)
{
	if(pdivoom_oneline_led_ctrl_handle)
	{
		Pdivoom_oneline_led_ctrl handle = pdivoom_oneline_led_ctrl_handle;
		pdivoom_oneline_led_ctrl_handle = NULL;
		divoom_free(handle);
	}
}


