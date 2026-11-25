#include "divoom.h"

typedef struct  _divoom_charge_ctrl
{
    int  status;
    int  capacity;
    int  voltage;
}divoom_charge_ctrl, *Pdivoom_charge_ctrl;

static Pdivoom_charge_ctrl pdivoom_charge_handle = NULL;

#define BATTERY_CAPACITY_PATH "/sys/class/power_supply/battery/capacity"
#define BATTERY_STATUS_PATH "/sys/class/power_supply/battery/status"
#define BATTERY_VOLTAGE_PATH "/sys/class/power_supply/battery/status"

void* divoom_charge_read_driver(void *param)
{
    char read_buf[128];
    FILE *file_status, *file_capacity, *file_voltage;

    file_status = divoom_port_file_open(BATTERY_STATUS_PATH, "r");
    file_capacity = divoom_port_file_open(BATTERY_CAPACITY_PATH, "r");
    file_voltage = divoom_port_file_open(BATTERY_VOLTAGE_PATH, "r");
    while(1)
    {
        if(file_status)
        {   
            if(fgets(read_buf, 128, file_status))
            {
                if(strstr(read_buf, "Charging"))
                {
                    pdivoom_charge_handle->status = 1;      
                }
                else
                {
                    pdivoom_charge_handle->status = 0;      
                }
            }
        }
        
        if(file_capacity)
        {   
            if(fgets(read_buf, 128, file_capacity))
            {
                pdivoom_charge_handle->capacity = atoi(read_buf);
            }
        }
        
        if(file_voltage)
        {   
            if(fgets(read_buf, 128, file_voltage))
            {
                pdivoom_charge_handle->voltage = atoi(read_buf)/1000;
            }
        }
        
        divoom_system_delay_ms(2000);
    }
    return NULL;
}

/*********************************************************************
  Function:        divoom_charge_init
  Description:     初始化
  Input:           none
  Return:          none
**********************************************************************/


void divoom_charge_init(void)
{
    pdivoom_charge_handle = divoom_system_malloc(sizeof(divoom_charge_ctrl));
    divoom_system_create_task(divoom_charge_read_driver, "charge", pdivoom_charge_handle, DIVOOM_TASK_STACK_CHARGE);
}

/*********************************************************************
  Function:        divoom_charge_get_status
  Description:     获取充电状态
  Input:           none
  Return:          1:充电；0：非充电
**********************************************************************/

int divoom_charge_get_status(void)
{
    return pdivoom_charge_handle->status;
}


/*********************************************************************
  Function:        divoom_charge_get_voltage
  Description:     获取电池的电压
  Input:           none
  Return:          返回mv
**********************************************************************/

int divoom_charge_get_voltage(void)
{
    return pdivoom_charge_handle->voltage;
}



/*********************************************************************
  Function:        divoom_charge_get_capacity
  Description:     获取电池的电量
  Input:           none
  Return:          返回百分比
**********************************************************************/

int divoom_charge_get_capacity(void)
{
    return pdivoom_charge_handle->capacity;
}


