
#include "divoom_disp_com.h"

typedef struct _divoom_disp_user_define_ctrl
{
    uint8 index;    //第几个自定义
    uint8 flag;     //是否有作品
}divoom_disp_user_define_ctrl, *Pdivoom_disp_user_define_ctrl;

Pdivoom_disp_user_define_ctrl pidvoom_disp_user_define_handle = NULL;

void divoom_disp_user_define_init(void *data)
{
}


void divoom_disp_user_define_update(void *data)
{
}


void divoom_disp_user_define_close(void)
{
}


