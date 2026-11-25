#include "divoom.h"
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

typedef struct _divoom_led_control_ctrl
{
    int rgb_switch;
    int white_switch;
    int white_bright;
    int led_file_handle;
	uint32 led_rgb_data[DIVOOM_LED_CONTROL_MAX_NUM];
}divoom_led_control_ctrl, *Pdivoom_led_control_ctrl;

Pdivoom_led_control_ctrl pdivoom_led_control_handle = NULL;

void *divoom_led_control_test_task_main(void *para)
{
    return NULL;
}
void divoom_led_control_init(void)
{
    int i;
    char file_name[64];
    
    pdivoom_led_control_handle = divoom_system_malloc(sizeof(divoom_led_control_ctrl));
//    divoom_system_create_task(divoom_led_control_test_task_main, "led_test", NULL, DIVOOM_TASK_STACK_LED_TEST);
}

#if 0
void divoom_led_control_one_rgb(int led_id, Pdivoom_led_control_led_info pled_info)
{
    int ret;
    char led_info[32];
    
    if(pdivoom_led_control_handle == NULL)
    {
        return;
    }
    if(led_id >= DIVOOM_LED_CONTROL_MAX_NUM)
    {
        return;
    }
    memcpy(&pdivoom_led_control_handle->all_rgb_info[led_id], pled_info, sizeof(divoom_led_control_led_info));
    if(pdivoom_led_control_handle->led_handle[led_id][0])
    {
        sprintf(led_info, "%d",pled_info->r);
        ret = write(pdivoom_led_control_handle->led_handle[led_id][0], led_info, strlen(led_info));
    }
    if(pdivoom_led_control_handle->led_handle[led_id][1])
    {
        sprintf(led_info, "%d",pled_info->g);
        ret = write(pdivoom_led_control_handle->led_handle[led_id][1], led_info, strlen(led_info));
    }
    if(pdivoom_led_control_handle->led_handle[led_id][2])
    {
        sprintf(led_info, "%d",pled_info->b);
        ret = write(pdivoom_led_control_handle->led_handle[led_id][2], led_info, strlen(led_info));
    }
}
#endif

void divoom_led_control_all_rgb(Pdivoom_led_control_led_info pled_info)
{
    int i, ret;
//    uint32 led_rgb_data[DIVOOM_LED_CONTROL_MAX_NUM];
    
    if(pdivoom_led_control_handle == NULL)
    {
        return;
    }
    //memcpy(pdivoom_led_control_handle->all_rgb_info, pled_info, sizeof(divoom_led_control_led_info)*DIVOOM_LED_CONTROL_MAX_NUM);
    for(i = 0; i < DIVOOM_LED_CONTROL_MAX_NUM; i++)
    {
        pdivoom_led_control_handle->led_rgb_data[i] = (pled_info[i].g << 16) | (pled_info[i].r << 8) | pled_info[i].b;
        #if 0
        if(pdivoom_led_control_handle->led_handle[i][0])
        {
            sprintf(led_info, "%d",pled_info[i].r);
            ret = write(pdivoom_led_control_handle->led_handle[i][0], led_info, strlen(led_info));
        }
        if(pdivoom_led_control_handle->led_handle[i][1])
        {
            sprintf(led_info, "%d",pled_info[i].g);
            ret = write(pdivoom_led_control_handle->led_handle[i][1], led_info, strlen(led_info));
        }
        if(pdivoom_led_control_handle->led_handle[i][2])
        {
            sprintf(led_info, "%d",pled_info[i].b);
            ret = write(pdivoom_led_control_handle->led_handle[i][2], led_info, strlen(led_info));
        }
        #endif
    }
}

#if 0
void divoom_led_control_switch_rgb_led(int on_off)
{
    int i, ret;
    char led_info[32];
    if(pdivoom_led_control_handle == NULL)
    {
        return;
    }
    if(pdivoom_led_control_handle->rgb_switch == on_off)
    {
        return;
    }
    if(on_off)
    {
        Pdivoom_led_control_led_info pled_info = pdivoom_led_control_handle->all_rgb_info;
        
        for(i = 0; i < DIVOOM_LED_CONTROL_MAX_NUM; i++)
        {
            if(pdivoom_led_control_handle->led_handle[i][0])
            {
                sprintf(led_info, "%d",pled_info[i].r);
                ret = write(pdivoom_led_control_handle->led_handle[i][0], led_info, strlen(led_info));
            }
            if(pdivoom_led_control_handle->led_handle[i][1])
            {
                sprintf(led_info, "%d",pled_info[i].g);
                ret = write(pdivoom_led_control_handle->led_handle[i][1], led_info, strlen(led_info));
            }
            if(pdivoom_led_control_handle->led_handle[i][2])
            {
                sprintf(led_info, "%d",pled_info[i].b);
                ret = write(pdivoom_led_control_handle->led_handle[i][2], led_info, strlen(led_info));
            }
        }
    }
    else
    {
        uint8 brightness = 0;
        led_info[0] = '0';
        led_info[1] = 0;
        
        for(i = 0; i < DIVOOM_LED_CONTROL_MAX_NUM; i++)
        {
            if(pdivoom_led_control_handle->led_handle[i][0])
            {
                ret = write(pdivoom_led_control_handle->led_handle[i][0], led_info, 1);
            }
            if(pdivoom_led_control_handle->led_handle[i][1])
            {
                ret = write(pdivoom_led_control_handle->led_handle[i][1], led_info, 1);
            }
            if(pdivoom_led_control_handle->led_handle[i][2])
            {
                ret = write(pdivoom_led_control_handle->led_handle[i][2], led_info, 1);
            }
        }
    }
    
    pdivoom_led_control_handle->rgb_switch = on_off;
}
#endif

void divoom_led_control_switch_rgb_on_off(int on_off)
{
    int ret;
	if(pdivoom_led_control_handle == NULL)
	{
		return;
	}

}

void divoom_led_control_switch_white_led(int on_off)
{
}


void divoom_led_control_control_white_birghness(int brightness)
{
    
}



