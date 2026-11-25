#include "divoom_disp_com.h"
#if 0
#define DIVOOM_LIGHT_SPECIAL_GET_RAND(x)       divoom_com_get_random(x)
#define DIVOOM_LIGHT_SPECIAL_MALLOC(size)      divoom_com_malloc(size, __func__, __LINE__)
#define DIVOOM_LIGHT_SPECIAL_FREE(size)        divoom_com_free(size, __func__, __LINE__)
#define DIVOOM_LIGHT_SPECIAL_PRINT             divoom_print

#define DIVOOM_DISP_SPECIAL_MODE1_MAX_COLOR   29
#define DIVOOM_DISP_SPECIAL_MODE2_MAX_COLOR   26
#define DIVOOM_DISP_SPECIAL_LEFT_RINGHT_STEP  5
#define DIVOOM_DISP_SPECIAL_MODE2_STEP        5


static const divoom_light_color_info DIVOOM_DISP_RANDOM_GROUD = {0,0,255};
const divoom_light_color_info DIVOOM_DISP_RANDOM_DISP_COLOR[] = {{0,0,255},{255,0,0},{0,255,0},{0,255,255},{255,0,255},{255,255,0}};
#define DIVOOM_DISP_RANDOM_TASK_TIME   10
#define DIVOOM_DISP_RANDOM_GET_RANDOM  10
#define DIVOOM_DISP_RANDOM_DIS_TIME    8


#define DIVOOM_DISP_SPECIAL_REFRESH_TIME  7

typedef struct _divoom_light_special_rotate_para
{
    uint8 step;
    uint16 index;
}divoom_light_special_rotate_para, *Pdivoom_light_special_rotate_para;


typedef struct _divoom_light_special_ramdon_info
{
    divoom_light_color_info bg_color;
    divoom_light_color_info front_color;
    uint8 step;
    uint8 total;
}divoom_light_special_ramdon_info, *Pdivoom_light_special_ramdon_info;

typedef struct _divoom_disp_count_down_ctrl
{
    uint8 play_flag;
    uint8 task_id;
    uint8 disp_id;
    divoom_light_color_info color;
    uint8 cur_tick;
    uint8 preview_time;
    uint8 static_time;
    uint8 end_tick;
    uint8 disp_mode;
    uint8 pic_num;
    divoom_light_special_rotate_para para;
    divoom_light_special_ramdon_info ramdon_info;
    uint8 *backbuf;
    uint8 *picbuf;
}divoom_disp_count_down_ctrl, *Pdivoom_disp_count_down_ctrl;
/*
typedef struct _divoom_light_special_ctrl
{
    uint8 mode;
    uint8 rand_disp_mode;
    uint8 rand_disp_off;
    uint8 rand_disp_time;
    uint8 rand_disp_index;
    uint32 task_id;
    uint32 disp_index;
    uint32 color_index;
    uint32 disp_tick;
    int16 rainbow_color_r_step;
    int16 rainbow_color_g_step;
    int16 rainbow_color_b_step;
    divoom_light_color_info color;
    divoom_light_color_info base_color;
    divoom_light_color_info light_color_table[DIVOOM_DISP_SPECIAL_MODE1_MAX_COLOR];
    uint8 back_disp_buf[SCREEN_BUF_SIZE_32];
    uint8 disp_buf[SCREEN_BUF_SIZE_32];
    divoom_disp_count_down_ctrl coutdown;
}divoom_light_special_ctrl, *Pdivoom_light_special_ctrl;
static Pdivoom_light_special_ctrl pdivoom_light_special_handle  = NULL;
*/


typedef struct _divoom_light_special_ctrl
{
    uint8 mode;
    int32 time_id;
    uint32 tick;
    uint32 task_id;
}divoom_light_special_ctrl, *Pdivoom_light_special_ctrl;
typedef struct _divoom_light_special_ctrl_original
{
    uint8 rand_disp_mode;
    uint8 rand_disp_off;
    uint8 rand_disp_time;
    uint8 rand_disp_index;
    uint32 disp_index;
    uint32 color_index;
    uint32 disp_tick;
    int16 rainbow_color_r_step;
    int16 rainbow_color_g_step;
    int16 rainbow_color_b_step;
    divoom_light_color_info color;
    divoom_light_color_info base_color;
    divoom_light_color_info light_color_table[DIVOOM_DISP_SPECIAL_MODE1_MAX_COLOR];
    uint8 back_disp_buf[SCREEN_BUF_SIZE_32];
    uint8 disp_buf[SCREEN_BUF_SIZE_32];
    uint8 random_buf[SCREEN_BUF_SIZE_32];    
    divoom_disp_count_down_ctrl coutdown;
}divoom_light_special_ctrl_original, *Pdivoom_light_special_ctrl_original;
static Pdivoom_light_special_ctrl pdivoom_light_special_handle  = NULL;
static Pdivoom_light_special_ctrl_original pdivoom_light_special_original_handle  = NULL;







typedef struct _divoom_light_special_pic_info
{
    uint8 pic_num;
    uint8 pic_id[12];
}divoom_light_special_pic_info, *Pdivoom_light_special_pic_info;


static const uint8 divoom_light_special_mode1_change_info[] =
{0,1,0,0,1,2,3,4,5,5,6,6,6,7,8,8,9,9,9,10,10,11,11,12,12,13,13,14,14,14
        
};

static const divoom_light_color_info divoom_light_roll_color_table[] =
{
     {255,0,0},
    {255,121,0},
    {255,207,0},
    {255,255,0},
    {148,255,0},
    {0,255,0},
    {0,255,156},
    {0,255,255},
    {0,85,255},
    {0,0,255},
    {66,0,255},
    {198,0,255},
    {255,0,255},
    {255,0,156},
    {255,0,74}
};

static const divoom_light_color_info divoom_light_mode1_color_table[] =
{
    {255,0,74},
    {255,0,156},
    {247,0,255},
    {189,0,255},
    {123,0,255},
    {57,0,255},
    {0,125,255},
    {0,227,255},
    {0,255,206},
    {0,255,126},
    {123,255,0},
    {181,255,0},
    {255,255,0},
    {255,178,0},
    {255,72,0},
    {255,0,0},
};

static const divoom_light_color_info divoom_light_mode2_color_table[] =
{
    {255,48,0},
    {255,219,0},
    {8,255,0},
    {0,255,247},
    {0, 0,255},
    {140,0,255},
    {255,0,74},
};


static const divoom_light_color_info divoom_light_rainbow_color_table[] =
{
    {255,0,74},
    {255,0,156},
    {247,0,255},
    {189,0,255},
    {123,0,255},
    {57,0,255},
    {0,125,255},
    {0,227,255},
    {0,255,206},
    {0,255,126},
    {123,255,0},
    {181,255,0},
    {255,255,0},
    {255,178,0},
    {255,72,0},
    {255,0,0},
};
/*********************************************************************
  Function:         divoom_light_special_mode2_convert_pos
  Description:     将X,Y坐标值转换为第一个区域的值，方便做中心对称图形
  Input:             x
                        y
                        new_x       新坐标
                        new_y       新坐标
  Author:           divoom
**********************************************************************/

void divoom_light_special_mode2_convert_pos(uint8 x, uint8 y, uint8 *new_x, uint8 *new_y)
{
    if(x >= (SCREEN_WIDTH_32 >> 1))
    {
        *new_x = SCREEN_WIDTH_32 - 1 - x;
    }
    else
    {
        *new_x = x;
    }

    if(y >= (SCREEN_HEIGHT_32 >> 1))
    {
        *new_y = SCREEN_HEIGHT_32 - 1 - y;
    }
    else
    {
        *new_y = y;
    }
}


/*********************************************************************
  Function:         divoom_light_special_get_pos_index
  Description:     得到相应坐标的色彩索引值
  Input:             x
                        y
  Author:           divoom
**********************************************************************/

uint8 divoom_light_special_mode2_get_pos_index(uint8 x, uint8 y)
{
    uint8 index,new_x, new_y;

    divoom_light_special_mode2_convert_pos(x, y, &new_x, &new_y);

    index = new_x + new_y;
    if(index >= DIVOOM_DISP_SPECIAL_MODE2_MAX_COLOR)
    {
        index = DIVOOM_DISP_SPECIAL_MODE2_MAX_COLOR - 1;
    }
    return index;
}

void divoom_light_special_get_mode1_color_index(Pdivoom_light_color_info color, uint32 index)
{
    memcpy(color, &divoom_light_mode1_color_table[index % (sizeof(divoom_light_mode1_color_table)/sizeof(divoom_light_color_info))], sizeof(divoom_light_color_info));

}

#define DIVOOM_DISP_SPECIAL_STEP_COLOR    3
void divoom_light_special_get_light_left_color_index(Pdivoom_light_color_info color, uint32 index, uint8 step)
{
    uint32 color_index;

    color_index = index / step % (sizeof(divoom_light_roll_color_table)/sizeof(divoom_light_color_info));

    memcpy(color, &divoom_light_roll_color_table[color_index], sizeof(divoom_light_color_info));

}

void divoom_light_special_get_mode2_color_index(Pdivoom_light_color_info color, uint32 index, uint8 step)
{
    uint32 color_index;

    color_index = index / step % (sizeof(divoom_light_mode2_color_table)/sizeof(divoom_light_color_info));

    memcpy(color, &divoom_light_mode2_color_table[color_index], sizeof(divoom_light_color_info));
}

void divoom_light_special_get_rainbow_color_step(void)
{
    uint8 cur, next;

    cur = pdivoom_light_special_original_handle->color_index %(sizeof(divoom_light_rainbow_color_table) / sizeof(divoom_light_color_info));
    next = (pdivoom_light_special_original_handle->color_index + 1) %(sizeof(divoom_light_rainbow_color_table) / sizeof(divoom_light_color_info));
    pdivoom_light_special_original_handle->rainbow_color_r_step = (int16)divoom_light_rainbow_color_table[next].r_c - (int16)divoom_light_rainbow_color_table[cur].r_c;
    pdivoom_light_special_original_handle->rainbow_color_g_step = (int16)divoom_light_rainbow_color_table[next].g_c - (int16)divoom_light_rainbow_color_table[cur].g_c;
    pdivoom_light_special_original_handle->rainbow_color_b_step = (int16)divoom_light_rainbow_color_table[next].b_c - (int16)divoom_light_rainbow_color_table[cur].b_c;
}

void divoom_light_special_init_rainbow_color(void)
{
    
    pdivoom_light_special_original_handle->disp_index = 0;
    pdivoom_light_special_original_handle->color_index = 0;
    pdivoom_light_special_original_handle->disp_tick = 0;
    memcpy(&pdivoom_light_special_original_handle->color, &divoom_light_rainbow_color_table[pdivoom_light_special_original_handle->color_index], sizeof(divoom_light_color_info));
    memcpy(&pdivoom_light_special_original_handle->base_color, &pdivoom_light_special_original_handle->color, sizeof(divoom_light_color_info));
    divoom_light_special_get_rainbow_color_step();
}

void divoom_light_special_update_rainbow_color(void)
{
    pdivoom_light_special_original_handle->color.r_c = pdivoom_light_special_original_handle->base_color.r_c + pdivoom_light_special_original_handle->rainbow_color_r_step * (int16)pdivoom_light_special_original_handle->disp_index / 25;
    pdivoom_light_special_original_handle->color.g_c = pdivoom_light_special_original_handle->base_color.g_c + pdivoom_light_special_original_handle->rainbow_color_g_step * (int16)pdivoom_light_special_original_handle->disp_index / 25;
    pdivoom_light_special_original_handle->color.b_c = pdivoom_light_special_original_handle->base_color.b_c + pdivoom_light_special_original_handle->rainbow_color_b_step * (int16)pdivoom_light_special_original_handle->disp_index / 25;    
}

void divoom_light_special_main_rainbow_color(void)
{
    pdivoom_light_special_original_handle->disp_index ++;
    if(pdivoom_light_special_original_handle->disp_index >= 25)
    {
        pdivoom_light_special_original_handle->disp_index = 0;
        pdivoom_light_special_original_handle->color_index ++;
        if(pdivoom_light_special_original_handle->color_index >= sizeof(divoom_light_rainbow_color_table) / sizeof(divoom_light_color_info))
        {
            pdivoom_light_special_original_handle->color_index = 0;
        }
        memcpy(&pdivoom_light_special_original_handle->color, &divoom_light_rainbow_color_table[pdivoom_light_special_original_handle->color_index], sizeof(divoom_light_color_info));
        memcpy(&pdivoom_light_special_original_handle->base_color, &pdivoom_light_special_original_handle->color, sizeof(divoom_light_color_info));
        divoom_light_special_get_rainbow_color_step();
    }
    divoom_light_special_update_rainbow_color();
    divoom_disp_ctrl_fill(pdivoom_light_special_original_handle->disp_buf,&pdivoom_light_special_original_handle->color);
    divoom_screen_disp_rgb32_data(pdivoom_light_special_original_handle->disp_buf);
}

void divoom_light_special_init_mode1_color(void)
{
    uint8 i;

    pdivoom_light_special_original_handle->color_index = 0;
    for(i = 0; i < DIVOOM_DISP_SPECIAL_MODE1_MAX_COLOR; i++)
    {
        divoom_light_special_get_mode1_color_index(&pdivoom_light_special_original_handle->light_color_table[i],pdivoom_light_special_original_handle->color_index);
        pdivoom_light_special_original_handle->color_index ++;
    }
}

void divoom_light_special_init_mode2_color(void)
{
    uint8 i;

    pdivoom_light_special_original_handle->color_index = 0;
    for(i = 0; i < DIVOOM_DISP_SPECIAL_MODE2_MAX_COLOR; i++)
    {
        divoom_light_special_get_mode2_color_index(&pdivoom_light_special_original_handle->light_color_table[i],pdivoom_light_special_original_handle->color_index,DIVOOM_DISP_SPECIAL_MODE2_STEP);
        pdivoom_light_special_original_handle->color_index ++;
    }
}

void divoom_light_special_update_mode2_color(void)
{
    memmove(&pdivoom_light_special_original_handle->light_color_table[0], &pdivoom_light_special_original_handle->light_color_table[1], sizeof(divoom_light_color_info) * (DIVOOM_DISP_SPECIAL_MODE2_MAX_COLOR - 1));
    divoom_light_special_get_mode2_color_index(&pdivoom_light_special_original_handle->light_color_table[DIVOOM_DISP_SPECIAL_MODE2_MAX_COLOR - 1],pdivoom_light_special_original_handle->color_index, DIVOOM_DISP_SPECIAL_MODE2_STEP);
    pdivoom_light_special_original_handle->color_index ++;
}

void divoom_light_special_format_mode2_color(void)
{
    uint8 i, j, color_index;

    for(i = 0; i < SCREEN_WIDTH_32; i ++)
    {
        for(j = 0; j < SCREEN_HEIGHT_32; j ++)
        {
            color_index = divoom_light_special_mode2_get_pos_index(i, j);
            divoom_disp_ctrl_set_dot(i, j, &pdivoom_light_special_original_handle->light_color_table[color_index],pdivoom_light_special_original_handle->disp_buf);
        }
    }
    divoom_light_special_update_mode2_color();
}

/*********************************************************************
  Function:         divoom_light_special_mode1_get_pos_index
  Description:     得到相应坐标的色彩索引
  Input:             x
                        y
  Author:           divoom
**********************************************************************/

uint8 divoom_light_special_mode1_get_pos_index(uint8 x, uint8 y)
{
    uint16 index;
    if(y == 0)
    {
        if(x >= SCREEN_WIDTH_32 - 1)
        {
            index = 0;
        }
        else
        {
            index = SCREEN_WIDTH_32 - 2 -x;
        }
    }
    else
    {
         index = SCREEN_WIDTH_32 - 1 -x + y -1;
    }
    index = index % (sizeof(divoom_light_special_mode1_change_info));
    return divoom_light_special_mode1_change_info[index];
}


void divoom_light_special_update_mode1_color(void)
{
    memmove(&pdivoom_light_special_original_handle->light_color_table[0], &pdivoom_light_special_original_handle->light_color_table[1], sizeof(divoom_light_color_info) * (DIVOOM_DISP_SPECIAL_MODE1_MAX_COLOR - 1));
    memcpy(&pdivoom_light_special_original_handle->light_color_table[DIVOOM_DISP_SPECIAL_MODE1_MAX_COLOR - 1], &pdivoom_light_special_original_handle->light_color_table[DIVOOM_DISP_SPECIAL_MODE1_MAX_COLOR - 2], sizeof(divoom_light_color_info));
    divoom_light_special_get_mode1_color_index(&pdivoom_light_special_original_handle->light_color_table[DIVOOM_DISP_SPECIAL_MODE1_MAX_COLOR - 1],pdivoom_light_special_original_handle->color_index);
    pdivoom_light_special_original_handle->color_index ++;
}

void divoom_light_special_format_mode1_color(void)
{
    uint8 i, j, color_index;
    divoom_light_color_info color;

    for(i = 0; i < SCREEN_WIDTH_32; i ++)
    {
        for(j = 0; j < SCREEN_HEIGHT_32; j ++)
        {
            color_index = divoom_light_special_mode1_get_pos_index(i, j);
            divoom_disp_ctrl_get_dot(i, j, &color,pdivoom_light_special_original_handle->disp_buf);
            if(color.r_c || color.g_c || color.b_c)
            {
                divoom_disp_ctrl_set_dot(i, j, &pdivoom_light_special_original_handle->light_color_table[color_index],pdivoom_light_special_original_handle->disp_buf);
            }
        }
    }
    divoom_light_special_update_mode1_color();
}

void divoom_light_special_mode1_init(void)
{
    divoom_disp_picture_get_rgb_data(DIVOOM_DISP_SPECIAL_LIGHT_MODE_01_PIC, pdivoom_light_special_original_handle->disp_buf);    
    divoom_light_special_format_mode1_color();
}


void divoom_light_special_left_right_init(void)
{
    uint8 i;
    
    pdivoom_light_special_original_handle->disp_index = 0;
    for(i = 0; i <= SCREEN_WIDTH_32 / 2; i ++)
    {
        divoom_light_special_get_light_left_color_index(&pdivoom_light_special_original_handle->light_color_table[i], pdivoom_light_special_original_handle->disp_index,DIVOOM_DISP_SPECIAL_LEFT_RINGHT_STEP);
        pdivoom_light_special_original_handle->disp_index ++;
    }
}


void divoom_light_special_left_right_main(void)
{
    uint8 i, j = 0;


    memset(pdivoom_light_special_original_handle->disp_buf, 0, SCREEN_BUF_SIZE_32);

    for(i = 0; i <= SCREEN_WIDTH_32 / 2; i ++)
    {
        for(j = 0; j < SCREEN_HEIGHT_32; j ++)
        {
            divoom_disp_ctrl_set_dot(SCREEN_WIDTH_32 / 2 - i, j, &pdivoom_light_special_original_handle->light_color_table[i], pdivoom_light_special_original_handle->disp_buf);
            divoom_disp_ctrl_set_dot(SCREEN_WIDTH_32 / 2 + i, j, &pdivoom_light_special_original_handle->light_color_table[i], pdivoom_light_special_original_handle->disp_buf);
        }
    }
    memmove(&pdivoom_light_special_original_handle->light_color_table[1], &pdivoom_light_special_original_handle->light_color_table[0], sizeof(divoom_light_color_info) *SCREEN_WIDTH_32 / 2);
    divoom_light_special_get_light_left_color_index(&pdivoom_light_special_original_handle->light_color_table[0], pdivoom_light_special_original_handle->disp_index, DIVOOM_DISP_SPECIAL_LEFT_RINGHT_STEP);
    pdivoom_light_special_original_handle->disp_index ++;
}



#if 0
void divoom_light_special_random_move_screen(uint8 mode)
{
    uint8 i, j, base;
    divoom_light_color_info color; 
    
    if(mode == 0)
    {
        //up
        base = pdivoom_light_special_original_handle->rand_disp_off * SCREEN_WIDTH_32;
        for(i = 0; i < SCREEN_HEIGHT_32; i++, base += SCREEN_WIDTH_32)
        {
            if(i == (SCREEN_HEIGHT_32 - pdivoom_light_special_original_handle->rand_disp_off))
            {
                base = 0;
            }
            for(j = 0; j < SCREEN_WIDTH_32; j++)
            {
                if(i < (SCREEN_HEIGHT_32 - pdivoom_light_special_original_handle->rand_disp_off))
                {
                    divoom_disp_ctrl_get_data(base + j,pdivoom_light_special_original_handle->back_disp_buf, &color);
                }
                else
                {
                    divoom_disp_ctrl_get_data(base + j,pdivoom_light_special_original_handle->disp_buf, &color);
                }
                divoom_disp_ctrl_set_data_xy(j, i, &color, pdivoom_light_special_original_handle->disp_buf);             
//                divoom_disp_ctrl_set_data(base + j, &color, pdivoom_light_special_original_handle->disp_buf);
            }
            
        }
    }
    else if(mode == 1)
    {
        //down
        base = (SCREEN_HEIGHT_32 -pdivoom_light_special_original_handle->rand_disp_off) * SCREEN_WIDTH_32;
        for(i = 0; i < SCREEN_HEIGHT_32; i++, base += SCREEN_WIDTH_32)
        {
            if(i == pdivoom_light_special_original_handle->rand_disp_off)
            {
                base = 0;
            }
            for(j = 0; j < SCREEN_WIDTH_32; j++)
            {
                if(i < pdivoom_light_special_original_handle->rand_disp_off)
                {
                    divoom_disp_ctrl_get_data(base + j,pdivoom_light_special_original_handle->disp_buf, &color);
                }
                else
                {
                    divoom_disp_ctrl_get_data(base + j,pdivoom_light_special_original_handle->back_disp_buf, &color);
                }
//                divoom_disp_ctrl_set_data(base + j, &color, pdivoom_light_special_original_handle->disp_buf);
                divoom_disp_ctrl_set_data_xy(j, i, &color, pdivoom_light_special_original_handle->disp_buf);             
                
            }
            
        }
    }
    else if(mode == 2)
    {
        //left
        for(i = 0, base = 0; i < SCREEN_HEIGHT_32; i++, base += SCREEN_WIDTH_32)
        {
            for(j = 0; j < SCREEN_WIDTH_32; j++)
            {
                if(j < SCREEN_WIDTH_32 - pdivoom_light_special_original_handle->rand_disp_off)
                {
                    divoom_disp_ctrl_get_data(base + j + pdivoom_light_special_original_handle->rand_disp_off,pdivoom_light_special_original_handle->back_disp_buf, &color);
                }
                else
                {
                    divoom_disp_ctrl_get_data(base + j + pdivoom_light_special_original_handle->rand_disp_off - SCREEN_WIDTH_32,pdivoom_light_special_original_handle->disp_buf, &color);
                }
//                divoom_disp_ctrl_set_data(base + j, &color, pdivoom_light_special_original_handle->disp_buf);
                divoom_disp_ctrl_set_data_xy(j, i, &color, pdivoom_light_special_original_handle->disp_buf);             

            }
            
        }
    }
    else if(mode == 3)
    {
        //right
        for(i = 0, base = 0; i < SCREEN_HEIGHT_32; i++, base += SCREEN_WIDTH_32)
        {
            for(j = 0; j < SCREEN_WIDTH_32; j++)
            {
                if(j < pdivoom_light_special_original_handle->rand_disp_off)
                {
                    divoom_disp_ctrl_get_data(base + j + SCREEN_WIDTH_32 - pdivoom_light_special_original_handle->rand_disp_off,pdivoom_light_special_original_handle->disp_buf, &color);
                }
                else
                {
                    divoom_disp_ctrl_get_data(base + j - pdivoom_light_special_original_handle->rand_disp_off,pdivoom_light_special_original_handle->back_disp_buf, &color);
                }
//                divoom_disp_ctrl_set_data(base + j, &color, pdivoom_light_special_original_handle->disp_buf);
                divoom_disp_ctrl_set_data_xy(j, i, &color, pdivoom_light_special_original_handle->disp_buf);             

            }
            
        }
    }
divoom_screen_disp_rgb_data(pdivoom_light_special_original_handle->disp_buf);    
}
void divoom_light_special_random_main(void)
{
    uint8 index;
    uint16 off;
    divoom_light_color_info color, *prandom_color;

    if(pdivoom_light_special_original_handle->rand_disp_mode & 0x80)
    {
        divoom_light_special_random_move_screen(pdivoom_light_special_original_handle->rand_disp_mode & 0x7F);
        pdivoom_light_special_original_handle->rand_disp_off ++;
        if(pdivoom_light_special_original_handle->rand_disp_off >= SCREEN_HEIGHT_32)
        {
            pdivoom_light_special_original_handle->rand_disp_mode &= 0x7F;
            pdivoom_light_special_original_handle->rand_disp_mode ++;
            if(pdivoom_light_special_original_handle->rand_disp_mode >=4)
            {
                pdivoom_light_special_original_handle->rand_disp_mode = 0;
            }
        }
        return ;
    }
    
    pdivoom_light_special_original_handle->rand_disp_index ++;
//    off = random() % (SCREEN_HEIGHT_32 * SCREEN_WIDTH_32);
//    index = random() %(sizeof(DIVOOM_DISP_RANDOM_DISP_COLOR) / sizeof(divoom_light_color_info));
    off = DIVOOM_LIGHT_SPECIAL_GET_RAND(SCREEN_HEIGHT_32 * SCREEN_WIDTH_32);
    index = DIVOOM_LIGHT_SPECIAL_GET_RAND(sizeof(DIVOOM_DISP_RANDOM_DISP_COLOR) / sizeof(divoom_light_color_info));    
    
    prandom_color = (Pdivoom_light_color_info)&DIVOOM_DISP_RANDOM_DISP_COLOR[index];
    divoom_disp_ctrl_get_data(off,pdivoom_light_special_original_handle->disp_buf, &color);
#if 1    
    if(memcpy(&color, &DIVOOM_DISP_RANDOM_GROUD, 3) == 0)
//    if(color.r_c == DIVOOM_DISP_RANDOM_GROUD.r_c && color.g_c == DIVOOM_DISP_RANDOM_GROUD.g_c && color.b_c == DIVOOM_DISP_RANDOM_GROUD.b_c)
    {
//        divoom_disp_ctrl_set_data(off, prandom_color, pdivoom_light_special_original_handle->disp_buf);    
        divoom_disp_ctrl_set_data(off, (Pdivoom_light_color_info)&DIVOOM_DISP_RANDOM_GROUD, pdivoom_light_special_original_handle->disp_buf);
    }
    else
    {
//        divoom_disp_ctrl_set_data(off, (Pdivoom_light_color_info)&DIVOOM_DISP_RANDOM_GROUD, pdivoom_light_special_original_handle->disp_buf);    
        divoom_disp_ctrl_set_data(off, prandom_color, pdivoom_light_special_original_handle->disp_buf);
    }
#endif
    if(pdivoom_light_special_original_handle->rand_disp_index >= DIVOOM_DISP_RANDOM_GET_RANDOM)
    {        
        pdivoom_light_special_original_handle->rand_disp_time ++;
        if(pdivoom_light_special_original_handle->rand_disp_time >= DIVOOM_DISP_RANDOM_DIS_TIME )
        {            
            pdivoom_light_special_original_handle->rand_disp_mode |= 0x80;
            pdivoom_light_special_original_handle->rand_disp_off = 1;
            pdivoom_light_special_original_handle->rand_disp_time = 0;
        }
        else
        {            
divoom_screen_disp_rgb_data(pdivoom_light_special_original_handle->disp_buf);        
            memcpy(pdivoom_light_special_original_handle->back_disp_buf, pdivoom_light_special_original_handle->disp_buf, SCREEN_BUF_SIZE_32);
            pdivoom_light_special_original_handle->rand_disp_index = 0;
        }
    }    
}
#endif

void divoom_light_special_random_move_screen(uint8 mode)
{
    uint8 i, j, base;
    divoom_light_color_info color; 
    memset(pdivoom_light_special_original_handle->disp_buf, 0, SCREEN_BUF_SIZE_32);
    
    if(mode == 0)
    {
        //up
        base = pdivoom_light_special_original_handle->rand_disp_off * SCREEN_WIDTH_32;
        for(i = 0; i < SCREEN_HEIGHT_32; i++, base += SCREEN_WIDTH_32)
        {
            if(i == (SCREEN_HEIGHT_32 - pdivoom_light_special_original_handle->rand_disp_off))
            {
                base = 0;
            }
            for(j = 0; j < SCREEN_WIDTH_32; j++)
            {
                if(i < (SCREEN_HEIGHT_32 - pdivoom_light_special_original_handle->rand_disp_off))
                {
                    divoom_disp_ctrl_get_data(base + j, &color,pdivoom_light_special_original_handle->back_disp_buf);
                }
                else
                {
                    divoom_disp_ctrl_get_data(base + j, &color,pdivoom_light_special_original_handle->random_buf);
                }
                divoom_disp_ctrl_set_data_xy(j, i, &color, pdivoom_light_special_original_handle->disp_buf);             
            }
            
        }
    }
    else if(mode == 1)
    {
        //down
        base = (SCREEN_HEIGHT_32 -pdivoom_light_special_original_handle->rand_disp_off) * SCREEN_WIDTH_32;
        for(i = 0; i < SCREEN_HEIGHT_32; i++, base += SCREEN_WIDTH_32)
        {
            if(i == pdivoom_light_special_original_handle->rand_disp_off)
            {
                base = 0;
            }
            for(j = 0; j < SCREEN_WIDTH_32; j++)
            {
                if(i < pdivoom_light_special_original_handle->rand_disp_off)
                {
                    divoom_disp_ctrl_get_data(base + j, &color,pdivoom_light_special_original_handle->random_buf);
                }
                else
                {
                    divoom_disp_ctrl_get_data(base + j, &color,pdivoom_light_special_original_handle->back_disp_buf);
                }
                divoom_disp_ctrl_set_data_xy(j, i, &color, pdivoom_light_special_original_handle->disp_buf);             
            }
            
        }
    }
    else if(mode == 2)
    {
        //left
        for(i = 0, base = 0; i < SCREEN_HEIGHT_32; i++, base += SCREEN_WIDTH_32)
        {
            for(j = 0; j < SCREEN_WIDTH_32; j++)
            {
                if(j < SCREEN_WIDTH_32 - pdivoom_light_special_original_handle->rand_disp_off)
                {
                    divoom_disp_ctrl_get_data(base + j + pdivoom_light_special_original_handle->rand_disp_off, &color,pdivoom_light_special_original_handle->back_disp_buf);
                }
                else
                {
                    divoom_disp_ctrl_get_data(base + j + pdivoom_light_special_original_handle->rand_disp_off - SCREEN_WIDTH_32, &color,pdivoom_light_special_original_handle->random_buf);
                }
                divoom_disp_ctrl_set_data_xy(j, i, &color, pdivoom_light_special_original_handle->disp_buf);             
            }
            
        }
    }
    else if(mode == 3)
    {
        //right
        for(i = 0, base = 0; i < SCREEN_HEIGHT_32; i++, base += SCREEN_WIDTH_32)
        {
            for(j = 0; j < SCREEN_WIDTH_32; j++)
            {
                if(j < pdivoom_light_special_original_handle->rand_disp_off)
                {
                    divoom_disp_ctrl_get_data(base + j + SCREEN_WIDTH_32 - pdivoom_light_special_original_handle->rand_disp_off, &color,pdivoom_light_special_original_handle->random_buf);
                }
                else
                {
                    divoom_disp_ctrl_get_data(base + j - pdivoom_light_special_original_handle->rand_disp_off, &color,pdivoom_light_special_original_handle->back_disp_buf);
                }
                divoom_disp_ctrl_set_data_xy(j, i, &color, pdivoom_light_special_original_handle->disp_buf);             
            }
            
        }
    }
}
void divoom_light_special_random_main(void)
{
    uint8 index;
    uint16 off;
    divoom_light_color_info color, *prandom_color;
   // uint16 ram_data = (uint8)Fwl_GetTickCountUs();

    if(pdivoom_light_special_original_handle->rand_disp_mode & 0x80)
    {
        divoom_light_special_random_move_screen(pdivoom_light_special_original_handle->rand_disp_mode & 0x7F);
        pdivoom_light_special_original_handle->rand_disp_off ++;
        if(pdivoom_light_special_original_handle->rand_disp_off >= SCREEN_HEIGHT_32)
        {
            pdivoom_light_special_original_handle->rand_disp_mode &= 0x7F;
            pdivoom_light_special_original_handle->rand_disp_mode ++;
            if(pdivoom_light_special_original_handle->rand_disp_mode >=4)
            {
                pdivoom_light_special_original_handle->rand_disp_mode = 0;
            }
        }
        return ;
    }
    
    pdivoom_light_special_original_handle->rand_disp_index ++;
    off = DIVOOM_LIGHT_SPECIAL_GET_RAND(SCREEN_HEIGHT_32 * SCREEN_WIDTH_32);
    index = DIVOOM_LIGHT_SPECIAL_GET_RAND(sizeof(DIVOOM_DISP_RANDOM_DISP_COLOR) / sizeof(divoom_light_color_info));    


    prandom_color = (Pdivoom_light_color_info)&DIVOOM_DISP_RANDOM_DISP_COLOR[index];
    divoom_disp_ctrl_get_data(off, &color,pdivoom_light_special_original_handle->random_buf);
#if 0    
    if(memcpy(&color, &DIVOOM_DISP_RANDOM_GROUD, 3) == 0)
    {
        divoom_disp_ctrl_set_data(off, (Pdivoom_light_color_info)&DIVOOM_DISP_RANDOM_GROUD, pdivoom_light_special_original_handle->random_buf);
    }
    else
    {
        divoom_disp_ctrl_set_data(off, prandom_color, pdivoom_light_special_original_handle->random_buf);
    }
#endif
    if(color.r_c == DIVOOM_DISP_RANDOM_GROUD.r_c && color.g_c == DIVOOM_DISP_RANDOM_GROUD.g_c && color.b_c == DIVOOM_DISP_RANDOM_GROUD.b_c)
    {
        divoom_disp_ctrl_set_data(off, prandom_color, pdivoom_light_special_original_handle->random_buf);    
    }
    else
    {
        divoom_disp_ctrl_set_data(off, (Pdivoom_light_color_info)&DIVOOM_DISP_RANDOM_GROUD, pdivoom_light_special_original_handle->random_buf);    
    }

    if(pdivoom_light_special_original_handle->rand_disp_index >= DIVOOM_DISP_RANDOM_GET_RANDOM)
    {        
        pdivoom_light_special_original_handle->rand_disp_time ++;
        if(pdivoom_light_special_original_handle->rand_disp_time >= DIVOOM_DISP_RANDOM_DIS_TIME )
        {
            
            pdivoom_light_special_original_handle->rand_disp_mode |= 0x80;
            pdivoom_light_special_original_handle->rand_disp_off = 1;
            pdivoom_light_special_original_handle->rand_disp_time = 0;
        }
        else
        {
            memcpy(pdivoom_light_special_original_handle->disp_buf, pdivoom_light_special_original_handle->random_buf, SCREEN_BUF_SIZE_32);            
            memcpy(pdivoom_light_special_original_handle->back_disp_buf, pdivoom_light_special_original_handle->random_buf, SCREEN_BUF_SIZE_32);
            pdivoom_light_special_original_handle->rand_disp_index = 0;
        }
    }
    
    
    
}


void divoom_light_special_random_init(void)
{
    divoom_disp_ctrl_fill(pdivoom_light_special_original_handle->random_buf, (Pdivoom_light_color_info)&DIVOOM_DISP_RANDOM_GROUD); 
    divoom_disp_ctrl_fill(pdivoom_light_special_original_handle->disp_buf, (Pdivoom_light_color_info)&DIVOOM_DISP_RANDOM_GROUD);   
//    divoom_screen_disp_rgb_data(pdivoom_light_special_original_handle->disp_buf);     
    pdivoom_light_special_original_handle->rand_disp_mode = 0;
    pdivoom_light_special_original_handle->rand_disp_time= 0;
    pdivoom_light_special_original_handle->rand_disp_off= 0;
}


#define DIVOOM_DISP_COUNT_DOWN_REFRESH_TIME  4

#define COUNT_DOWN_FILL_DOT 10
typedef struct _divoom_light_special_count_down_info
{
    uint8 mode;
    uint8 static_tick;
    uint8 preview_tick;
    uint8 tick;
    divoom_light_color_info bg_color; //backgroud
    divoom_light_color_info fr_color; //backgroud
    uint16 disp_id;
}divoom_light_special_count_down_info, *Pdivoom_light_special_count_down_info;
#define COUNT_DOWN_RAMDON_TIME 20
#define COUNT_DOWN_PIC_TIME 20

//时间是以DIVOOM_DISP_COUNT_DOWN_REFRESH_TIME为单位

enum
{
    DIVOOM_DISP_COUNT_DOWN_UP_MODE,
    DIVOOM_DISP_COUNT_DOWN_DOWN_MODE,
    DIVOOM_DISP_COUNT_DOWN_UP_DOWN_MODE,
    DIVOOM_DISP_COUNT_DOWN_LEFT_MODE,
    DIVOOM_DISP_COUNT_DOWN_RIGHT_MODE,
    DIVOOM_DISP_COUNT_DOWN_LEFT_RIGHT_MODE,
    DIVOOM_DISP_COUNT_DOWN_CENTER_MODE,
    DIVOOM_DISP_COUNT_DOWN_CENTER_HORIZ_MODE,
    DIVOOM_DISP_COUNT_DOWN_CENTER_VERTI_MODE,
};
#define DIGIT5_COLOR_IN {255,0,0}   
#define DIGIT5_COLOR_BG {0,0,0} 

#define DIGIT4_COLOR_IN {0,0,255}     
#define DIGIT4_COLOR_BG {0,0,0} 

#define DIGIT3_COLOR_IN {255,0,255}    
#define DIGIT3_COLOR_BG {0,0,0}


#define DIGIT2_COLOR_IN {255,255,0}       
#define DIGIT2_COLOR_BG {0,0,0}


#define DIGIT1_COLOR_IN {0,255,255}    
#define DIGIT1_COLOR_BG {0,0,0} 





#define ALAINK_DISP_COUNT_DOWN_DIGIT_TIME 20
#define ALAINK_DISP_COUNT_DOWN_RAMDON_TIME 130
static const divoom_light_special_count_down_info disp_buf_info[]=
{
    {DIVOOM_DISP_COUNT_DOWN_UP_MODE, ALAINK_DISP_COUNT_DOWN_DIGIT_TIME,0, 15, DIGIT5_COLOR_BG,DIGIT5_COLOR_IN,DIVOOM_DISP_DIGIT_5_PIC},
    {DIVOOM_DISP_COUNT_DOWN_DOWN_MODE, ALAINK_DISP_COUNT_DOWN_DIGIT_TIME, 0, 15, DIGIT4_COLOR_BG, DIGIT4_COLOR_IN,DIVOOM_DISP_DIGIT_4_PIC},
    {DIVOOM_DISP_COUNT_DOWN_LEFT_MODE, ALAINK_DISP_COUNT_DOWN_DIGIT_TIME, 0, 15, DIGIT3_COLOR_BG, DIGIT3_COLOR_IN,DIVOOM_DISP_DIGIT_3_PIC},
    {DIVOOM_DISP_COUNT_DOWN_RIGHT_MODE, ALAINK_DISP_COUNT_DOWN_DIGIT_TIME, 0, 15, DIGIT2_COLOR_BG, DIGIT2_COLOR_IN,DIVOOM_DISP_DIGIT_2_PIC},
    {DIVOOM_DISP_COUNT_DOWN_CENTER_MODE, ALAINK_DISP_COUNT_DOWN_DIGIT_TIME, 0, 6, DIGIT1_COLOR_BG, DIGIT1_COLOR_IN,DIVOOM_DISP_DIGIT_1_PIC},
    {DIVOOM_DISP_COUNT_DOWN_CENTER_HORIZ_MODE, 0, ALAINK_DISP_COUNT_DOWN_RAMDON_TIME, 14, DIGIT5_COLOR_BG, DIGIT5_COLOR_IN,END_DISP_PIC},
    {DIVOOM_DISP_COUNT_DOWN_UP_DOWN_MODE, 0, ALAINK_DISP_COUNT_DOWN_RAMDON_TIME, 8, DIGIT4_COLOR_BG, DIGIT4_COLOR_IN,END_DISP_PIC},
    {DIVOOM_DISP_COUNT_DOWN_CENTER_VERTI_MODE, 0, ALAINK_DISP_COUNT_DOWN_RAMDON_TIME, 14, DIGIT3_COLOR_BG, DIGIT3_COLOR_IN,END_DISP_PIC},
    {DIVOOM_DISP_COUNT_DOWN_LEFT_RIGHT_MODE, 0, ALAINK_DISP_COUNT_DOWN_RAMDON_TIME, 8, DIGIT2_COLOR_BG, DIGIT2_COLOR_IN,END_DISP_PIC},
    {DIVOOM_DISP_COUNT_DOWN_CENTER_MODE, 0, ALAINK_DISP_COUNT_DOWN_RAMDON_TIME, 12, DIGIT1_COLOR_BG, DIGIT1_COLOR_IN,END_DISP_PIC},
};

void divoom_light_special_count_down_rotate_large_pic(uint8 *pic_buf, uint16 index, Pdivoom_light_color_info color)
{
    uint16 base1 , base2;
    uint8 i;

    if(index >= (SCREEN_HEIGHT_32>> 1))
    {
        index = (SCREEN_HEIGHT_32>> 1);
    }
    base1 = ((SCREEN_HEIGHT_32>> 1) - 1- index) * SCREEN_WIDTH_32 ;
    base2 = ((SCREEN_HEIGHT_32 >> 1) + index) * SCREEN_WIDTH_32 ;
    for(i = (SCREEN_WIDTH_32 >> 1) - 1 - index ; i < (SCREEN_WIDTH_32 >> 1) + index + 1; i ++)
    {
        divoom_disp_ctrl_set_data(base1 + i, color, pic_buf);
        divoom_disp_ctrl_set_data(base2 + i, color, pic_buf);
    }

    
    for(i = (SCREEN_WIDTH_32 >> 1) - 1 - index ; i < (SCREEN_WIDTH_32 >> 1) + index + 1; i ++)
    {
        divoom_disp_ctrl_set_data( i * SCREEN_WIDTH_32 + ((SCREEN_WIDTH_32>> 1) - 1- index)  ,color, pic_buf);
        divoom_disp_ctrl_set_data(i * SCREEN_WIDTH_32 +((SCREEN_HEIGHT_32 >> 1) + index) ,color, pic_buf);
    }
}



void divoom_light_special_count_down_cover(uint8 mode, uint8 index, Pdivoom_light_color_info color,uint8 *pic_buf)
{
    uint8 i, j, base, disp_lines, start_line, base2;
    
    if(index >= SCREEN_HEIGHT_32 - 1)
    {
        divoom_disp_ctrl_fill(pic_buf, color);
        return ;
    }
    disp_lines = index + 1;
    switch(mode)
    {
        case DIVOOM_DISP_COUNT_DOWN_UP_MODE:
            start_line = SCREEN_HEIGHT_32 - index - 1;
            for(j = start_line, base = start_line * SCREEN_WIDTH_32; j < start_line + disp_lines && j < SCREEN_HEIGHT_32; j ++, base += SCREEN_WIDTH_32)
            {
                for(i = 0; i < SCREEN_WIDTH_32; i ++)
                {
                    divoom_disp_ctrl_set_data(base + i, color, pic_buf);
                }
            }
            
            break;
            
        case DIVOOM_DISP_COUNT_DOWN_DOWN_MODE:
            start_line = 0;
            for(j = start_line, base = start_line * SCREEN_WIDTH_32; j < start_line + disp_lines && j < SCREEN_HEIGHT_32; j ++, base += SCREEN_WIDTH_32)
            {
                for(i = 0; i < SCREEN_WIDTH_32; i ++)
                {
                    divoom_disp_ctrl_set_data(base + i, color, pic_buf);
                }
            }
            break;
            
        case DIVOOM_DISP_COUNT_DOWN_LEFT_MODE:
            start_line = 0;
            for(j = 0, base = 0; j < SCREEN_HEIGHT_32; j ++, base += SCREEN_WIDTH_32)
            {
                for(i = start_line; i < SCREEN_WIDTH_32 && i < start_line + disp_lines; i ++)
                {
                    divoom_disp_ctrl_set_data(base + i, color,pic_buf);
                }
            }
            break;
            
        case DIVOOM_DISP_COUNT_DOWN_RIGHT_MODE:
            start_line = SCREEN_WIDTH_32 - 1 - index;
            for(j = 0, base = 0; j < SCREEN_HEIGHT_32; j ++, base += SCREEN_WIDTH_32)
            {
                for(i = start_line; i < SCREEN_WIDTH_32 && i < start_line + disp_lines; i ++)
                {
                    divoom_disp_ctrl_set_data(base + i, color,pic_buf);
                }
            }
            break;
            
        case DIVOOM_DISP_COUNT_DOWN_CENTER_MODE:
            if(index >= (SCREEN_HEIGHT_32 >> 1))
            {
                divoom_disp_ctrl_fill(pic_buf, color);
                return ;
            }
            divoom_light_special_count_down_rotate_large_pic(pic_buf,index, color);
            break;
            
        case DIVOOM_DISP_COUNT_DOWN_UP_DOWN_MODE:
            if(index >= (SCREEN_HEIGHT_32 >> 1))
            {
                divoom_disp_ctrl_fill(pic_buf, color);
                return ;
            }
            start_line = (SCREEN_HEIGHT_32 >> 1) - index - 18;
            disp_lines <<= 1;
            for(j = start_line, base = start_line * SCREEN_WIDTH_32; j < start_line + disp_lines && j < SCREEN_HEIGHT_32 ; j ++, base += SCREEN_WIDTH_32)
            {
                for(i = 0; i < SCREEN_WIDTH_32; i ++)
                {
                    divoom_disp_ctrl_set_data(base + i, color,pic_buf);
                }
            }
            
            break;
            
        case DIVOOM_DISP_COUNT_DOWN_LEFT_RIGHT_MODE:
            if(index >= (SCREEN_HEIGHT_32 >> 1))
            {
                divoom_disp_ctrl_fill(pic_buf, color);
                return ;
            }
                        
            start_line = (SCREEN_WIDTH_32 >> 1) - 1 - index;
            disp_lines <<= 1;
            for(j = 0, base = 0; j < SCREEN_HEIGHT_32; j ++, base += SCREEN_WIDTH_32)
            {
                for(i = start_line; i < SCREEN_WIDTH_32  && i < start_line + disp_lines; i ++)
                {
                    divoom_disp_ctrl_set_data(base + i, color,pic_buf);
                }
            }
            
            break;
            
        case DIVOOM_DISP_COUNT_DOWN_CENTER_HORIZ_MODE:
            if(index >= (SCREEN_HEIGHT_32 - 2))
            {
                divoom_disp_ctrl_fill(pic_buf, color);
                return ;
            }
            if(index < (SCREEN_HEIGHT_32 >> 1))
            {
                start_line = (SCREEN_WIDTH_32 >> 1) - 1 - index;
                disp_lines <<= 1;
                for(j = start_line,base = start_line * SCREEN_WIDTH_32; j < start_line + disp_lines; j ++, base += SCREEN_WIDTH_32)
                {
                    divoom_disp_ctrl_set_data(base + start_line, color,pic_buf);
                    divoom_disp_ctrl_set_data(base + SCREEN_WIDTH_32 - 1 - start_line, color,pic_buf);
                }
                
            }
            else
            {
                index -= (SCREEN_HEIGHT_32 >> 1);
                for(i = 0; i < (SCREEN_HEIGHT_32 >> 1); i++)
                {
                    if(i <= index)
                    {
                        continue;
                    }
                    divoom_disp_ctrl_set_data((i - index - 1) * SCREEN_WIDTH_32 + i , color,pic_buf);
                    divoom_disp_ctrl_set_data((i - index - 1) * SCREEN_WIDTH_32 + ((SCREEN_HEIGHT_32  -1 )- i) , color,pic_buf);
                    divoom_disp_ctrl_set_data(((SCREEN_HEIGHT_32  -1 )- (i - index - 1)) * SCREEN_WIDTH_32 + i , color,pic_buf);
                    divoom_disp_ctrl_set_data(((SCREEN_HEIGHT_32  -1 )- (i - index - 1)) * SCREEN_WIDTH_32 + ((SCREEN_HEIGHT_32  -1 )- i) , color,pic_buf);
                }
            }
            
            break;
            
        case DIVOOM_DISP_COUNT_DOWN_CENTER_VERTI_MODE:
            if(index >= (SCREEN_HEIGHT_32 - 2))
            {
                divoom_disp_ctrl_fill(pic_buf, color);
                return ;
            }
            if(index < (SCREEN_HEIGHT_32 >> 1))
            {
                start_line = (SCREEN_WIDTH_32 >> 1) - 1 - index;
                disp_lines <<= 1;
                base = start_line * SCREEN_WIDTH_32;
                base2 = (SCREEN_HEIGHT_32 - 1 - start_line) * SCREEN_WIDTH_32;
                for(j = start_line; j < start_line + disp_lines; j ++)
                {
                    divoom_disp_ctrl_set_data(base + j, color,pic_buf);
                    divoom_disp_ctrl_set_data(base2 + j, color,pic_buf);
                }
                
            }
            else
            {
                index -= (SCREEN_HEIGHT_32 >> 1);
                for(i = 0; i < (SCREEN_HEIGHT_32 >> 1); i++)
                {
                    if(i <= index)
                    {
                        continue;
                    }
                    divoom_disp_ctrl_set_data((i - index - 1) + i  * SCREEN_WIDTH_32 , color,pic_buf);
                    divoom_disp_ctrl_set_data((i - index - 1) + ((SCREEN_HEIGHT_32  -1 )- i) * SCREEN_WIDTH_32 , color,pic_buf);
                    divoom_disp_ctrl_set_data(((SCREEN_HEIGHT_32  -1 )- (i - index - 1)) + i * SCREEN_WIDTH_32 , color,pic_buf);
                    divoom_disp_ctrl_set_data(((SCREEN_HEIGHT_32  -1 )- (i - index - 1))  + ((SCREEN_HEIGHT_32  -1 )- i)* SCREEN_WIDTH_32  , color,pic_buf);
                }
            }
            break;
            
    }
}

#define DIVOOM_DISP_COUNT_DOWN_RAMDON_DOT  10
void divoom_light_special_count_down_get_ramdon_pic(uint8 *pic_buf, Pdivoom_light_color_info color)
{
    uint16 base = 0;
    uint8 i;

    memset(pic_buf, 0, SCREEN_BUF_SIZE_32);
    for(i = 0;  i < DIVOOM_DISP_COUNT_DOWN_RAMDON_DOT; i++)
    {
//        base = random() %(SCREEN_HEIGHT_32 * SCREEN_WIDTH_32);  
        base = DIVOOM_LIGHT_SPECIAL_GET_RAND(SCREEN_HEIGHT_32 * SCREEN_WIDTH_32);  
                
        divoom_disp_ctrl_set_data(base, color, pic_buf);
    }
}

void divoom_light_special_count_down_main(void)
{
    if(pdivoom_light_special_original_handle->coutdown.cur_tick >= pdivoom_light_special_original_handle->coutdown.end_tick)
    {
        pdivoom_light_special_original_handle->coutdown.disp_mode = disp_buf_info[pdivoom_light_special_original_handle->coutdown.disp_id].mode;
        memcpy(&pdivoom_light_special_original_handle->coutdown.color ,&disp_buf_info[pdivoom_light_special_original_handle->coutdown.disp_id].fr_color, 3);
        pdivoom_light_special_original_handle->coutdown.end_tick = disp_buf_info[pdivoom_light_special_original_handle->coutdown.disp_id].tick + disp_buf_info[pdivoom_light_special_original_handle->coutdown.disp_id].preview_tick
             + disp_buf_info[pdivoom_light_special_original_handle->coutdown.disp_id].static_tick;
        pdivoom_light_special_original_handle->coutdown.preview_time = disp_buf_info[pdivoom_light_special_original_handle->coutdown.disp_id].preview_tick + disp_buf_info[pdivoom_light_special_original_handle->coutdown.disp_id].static_tick;
        pdivoom_light_special_original_handle->coutdown.static_time= disp_buf_info[pdivoom_light_special_original_handle->coutdown.disp_id].static_tick;
        if(disp_buf_info[pdivoom_light_special_original_handle->coutdown.disp_id].disp_id == END_DISP_PIC)
        {
            divoom_disp_ctrl_fill(pdivoom_light_special_original_handle->coutdown.picbuf, (Pdivoom_light_color_info)&disp_buf_info[pdivoom_light_special_original_handle->coutdown.disp_id].bg_color);
        }        
        else
        {
            divoom_disp_picture_get_rgb_data(disp_buf_info[pdivoom_light_special_original_handle->coutdown.disp_id].disp_id, pdivoom_light_special_original_handle->coutdown.picbuf);
        }
        memcpy(pdivoom_light_special_original_handle->coutdown.backbuf, pdivoom_light_special_original_handle->coutdown.picbuf, SCREEN_BUF_SIZE_32);
        pdivoom_light_special_original_handle->coutdown.cur_tick = 0;
        memcpy(&pdivoom_light_special_original_handle->coutdown.ramdon_info.bg_color ,&disp_buf_info[pdivoom_light_special_original_handle->coutdown.disp_id].bg_color, 3);
        memcpy(&pdivoom_light_special_original_handle->coutdown.ramdon_info.front_color ,&disp_buf_info[pdivoom_light_special_original_handle->coutdown.disp_id].fr_color, 3);
        pdivoom_light_special_original_handle->coutdown.ramdon_info.step = COUNT_DOWN_FILL_DOT;
        memcpy(&pdivoom_light_special_original_handle->coutdown.color ,&pdivoom_light_special_original_handle->coutdown.ramdon_info.front_color, 3);
        pdivoom_light_special_original_handle->coutdown.ramdon_info.total = 0;
        pdivoom_light_special_original_handle->coutdown.disp_id ++;
        if(pdivoom_light_special_original_handle->coutdown.disp_id == pdivoom_light_special_original_handle->coutdown.pic_num)
        {
            pdivoom_light_special_original_handle->coutdown.disp_id = 0;
        }
    }    
    else if(pdivoom_light_special_original_handle->coutdown.cur_tick >= pdivoom_light_special_original_handle->coutdown.preview_time)
    {
            divoom_light_special_count_down_cover(pdivoom_light_special_original_handle->coutdown.disp_mode,pdivoom_light_special_original_handle->coutdown.cur_tick - pdivoom_light_special_original_handle->coutdown.preview_time,
                &pdivoom_light_special_original_handle->coutdown.color, pdivoom_light_special_original_handle->coutdown.picbuf);
    }
    else if(pdivoom_light_special_original_handle->coutdown.cur_tick >= pdivoom_light_special_original_handle->coutdown.static_time)
    {
        //if(pdivoom_light_special_original_handle->coutdown.cur_tick % 6 == 0)
        {
            divoom_light_special_count_down_get_ramdon_pic(pdivoom_light_special_original_handle->coutdown.picbuf, &pdivoom_light_special_original_handle->coutdown.ramdon_info.front_color);
           // divoom_disp_ctrl_refresh_buf(DIVOOM_DISP_ROLL_RAMDON,pdivoom_light_special_original_handle->coutdown.backbuf,
           //     pdivoom_light_special_original_handle->coutdown.picbuf, (void *)&pdivoom_light_special_original_handle->coutdown.ramdon_info); 
        }
    }
    pdivoom_light_special_original_handle->coutdown.cur_tick ++;
}

void divoom_light_special_count_down_init(void)
{
    memset(&pdivoom_light_special_original_handle->coutdown, 0, sizeof(pdivoom_light_special_original_handle->coutdown));
    pdivoom_light_special_original_handle->coutdown.picbuf = pdivoom_light_special_original_handle->disp_buf;
    pdivoom_light_special_original_handle->coutdown.backbuf = pdivoom_light_special_original_handle->back_disp_buf;
    pdivoom_light_special_original_handle->coutdown.cur_tick = 0;
    pdivoom_light_special_original_handle->coutdown.disp_id = 0;
    pdivoom_light_special_original_handle->coutdown.end_tick = 0;
    pdivoom_light_special_original_handle->coutdown.pic_num = sizeof(disp_buf_info)/sizeof(divoom_light_special_count_down_info);
}


/**
 * @function:    divoom_light_special_original_mode_init
 * @description: 原灯效集合模块开启
 * @Author:      coronaa
 * @date:        2017-06-30
 */
void divoom_light_special_original_mode_init(uint8 mode)
{     
    if(pdivoom_light_special_original_handle)
    {
        DIVOOM_LIGHT_SPECIAL_PRINT("divoom_light_special_original_init: err init: %d\n", mode);
        return;
    }

    pdivoom_light_special_original_handle = (Pdivoom_light_special_ctrl_original)DIVOOM_LIGHT_SPECIAL_MALLOC(sizeof(divoom_light_special_ctrl_original));
    memset(pdivoom_light_special_original_handle, 0, sizeof(divoom_light_special_ctrl_original));

    divoom_com_update_task(pdivoom_light_special_handle->task_id,DIVOOM_DISP_SPECIAL_REFRESH_TIME);//从别跳转过来后更新下刷新时间
    switch(mode)
    {
        case DIVOOM_DISP_SPECIAL_BIAS_MODE:    
            divoom_light_special_init_mode1_color();
            divoom_light_special_mode1_init();
            break;
        case DIVOOM_DISP_SPECIAL_RAINBOW_MODE:    
            divoom_light_special_init_rainbow_color();
            break;            
        case DIVOOM_DISP_SPECIAL_BLOCK_MODE:    
            divoom_light_special_left_right_init();            
            break;
        case DIVOOM_DISP_SPECIAL_DIAMONG_MODE:    
            divoom_light_special_init_mode2_color();            
            break;            
        case DIVOOM_DISP_SPECIAL_RANDOM_MODE:    
            //divoom_light_special_random_init();
            break;
        case DIVOOM_DISP_SPECIAL_COUNT_DOWN_MODE:    
            divoom_light_special_count_down_init();
            break;                      
        default:
            break;
    }

}
/**
 * @function:    divoom_light_special_original_mode_close
 * @description: 原灯效集合模块关闭
 * @Author:      coronaa
 * @date:        2017-06-30
 */
void divoom_light_special_original_mode_close(void)
{
    DIVOOM_LIGHT_SPECIAL_FREE(pdivoom_light_special_original_handle);
    pdivoom_light_special_original_handle = NULL;    
}

/**
 * @function:    divoom_light_special_original_mode_main
 * @description: 
 * @Author:      coronaa
 * @date:        2017-06-30
 */
void divoom_light_special_original_mode_main(uint8 mode)
{
    switch(mode)
    {
        case DIVOOM_DISP_SPECIAL_BIAS_MODE:    
            divoom_light_special_format_mode1_color();
            break;
        case DIVOOM_DISP_SPECIAL_RAINBOW_MODE:    
            divoom_light_special_main_rainbow_color();
            break;            
        case DIVOOM_DISP_SPECIAL_BLOCK_MODE:    
            divoom_light_special_left_right_main();
            break;
        case DIVOOM_DISP_SPECIAL_DIAMONG_MODE:    
            divoom_light_special_format_mode2_color();            
            break;            
        case DIVOOM_DISP_SPECIAL_RANDOM_MODE:    
            //divoom_light_special_random_main();
            break;
        case DIVOOM_DISP_SPECIAL_COUNT_DOWN_MODE:    
            //divoom_light_special_count_down_main();
            break;                      
        default:
            break;
    }
    divoom_screen_disp_rgb32_data(pdivoom_light_special_original_handle->disp_buf);    
}

/*********************************************************************
  Function:         divoom_light_special_main
  Description:      灯效主任务
  Input:            NONE
  Return:           NONE
  Author:           divoom
**********************************************************************/
void divoom_light_special_main(void *data)
{
    switch(pdivoom_light_special_handle->mode)
    {
        case DIVOOM_DISP_SPECIAL_BIAS_MODE:    
        case DIVOOM_DISP_SPECIAL_RAINBOW_MODE:    
        case DIVOOM_DISP_SPECIAL_BLOCK_MODE:    
        case DIVOOM_DISP_SPECIAL_DIAMONG_MODE:    
        case DIVOOM_DISP_SPECIAL_RANDOM_MODE:    
        case DIVOOM_DISP_SPECIAL_COUNT_DOWN_MODE:
            divoom_light_special_original_mode_main(pdivoom_light_special_handle->mode);
            break;

        case DIVOOM_LIGHT_SPECIAL_NEON_MOVE_MODE:
            divoom_light_neon_move_main();
            break;

        case DIVOOM_LIGHT_SPECIAL_WINDMILL_MODE:
            divoom_light_windmill_main();
            break;
        case DIVOOM_LIGHT_SPECIAL_BREATHING_LIGHT_MODE:
            divoom_light_breathing_light_main();
            break;
        case DIVOOM_LIGHT_SPECIAL_COUNT_DOWN_GRADIENT_MODE:
            divoom_light_count_down_grad_main();
            break;
        case DIVOOM_LIGHT_SPECIAL_EXPLODE_MODE:
//            divoom_light_explode_main();
            break;
        case DIVOOM_LIGHT_SPECIAL_PLASMA_MODE:
            divoom_light_plasma_main();            
            break; 
        default:
            pdivoom_light_special_handle->tick ++;
            if(pdivoom_light_special_handle->tick <= 2)
            {
                return;
            }
            divoom_light_specail_divoom_main();
            break;    
    } 
}


/*********************************************************************
  Function:         divoom_light_special_init
  Description:     修改灯效模式
  Input:             mode  模式
  Return:           NONE
  Author:           divoom
**********************************************************************/

void divoom_light_special_init(Pdivoom_disp_ctrl_special_info data)
{
    uint8 mode = 0xff;
    uint32 time;

	divoom_malloc_check_all_ram_in();
    if(pdivoom_light_special_handle)
    {
		if(data)
		{
			mode = data->mode;
		}
        DIVOOM_LIGHT_SPECIAL_PRINT("divoom_light_special_init: err init: %d\n", mode);
        return;
    }
    if(mode >= DIVOOM_DISP_SPECIAL_END_MODE)
    {
        mode = 0;
    }
    
    mode = data->mode;
    pdivoom_light_special_handle = (Pdivoom_light_special_ctrl)DIVOOM_LIGHT_SPECIAL_MALLOC(sizeof(divoom_light_special_ctrl));
    memset(pdivoom_light_special_handle, 0, sizeof(divoom_light_special_ctrl));
    pdivoom_light_special_handle->mode = mode;  //初始化任务
    pdivoom_light_special_handle->time_id = T_U8_MAX;


    time = divoom_light_special_init_mode();       //初始化具体灯效模块
    pdivoom_light_special_handle->task_id = divoom_com_create_task(divoom_light_special_main, time);    
    divoom_light_special_task_id();     //传递taskid到下一级
    divoom_light_special_main(NULL);
    divoom_com_resume_task(pdivoom_light_special_handle->task_id);
    // pdivoom_light_special_handle->task_id = divoom_com_create_task(divoom_light_special_main, DIVOOM_DISP_SPECIAL_REFRESH_TIME);
}


/*********************************************************************
  Function:         divoom_light_special_update_mode
  Description:     修改灯效模式
  Input:             mode  模式
  Return:           NONE
  Author:           divoom
**********************************************************************/

void divoom_light_special_update_mode(uint8 mode)
{
    uint32 time;
    DIVOOM_LIGHT_SPECIAL_PRINT("divoom_light_special_set_mode: %d\n", mode);
    if(pdivoom_light_special_handle == NULL)
    {
        DIVOOM_LIGHT_SPECIAL_PRINT("divoom_light_special_update ERRRRRR\n");
        return;
    }

    if(mode >= DIVOOM_DISP_SPECIAL_END_MODE)
    {
        mode = 0;
    }
    
    if(mode == pdivoom_light_special_handle->mode)
    {
        return;
    }   
    divoom_com_pause_task(pdivoom_light_special_handle->task_id);

    divoom_light_special_close_mode();  //关闭旧任务
    pdivoom_light_special_handle->mode = mode;  //更新任务
    time = divoom_light_special_init_mode();   //更新完mode再进行新任务的初始化
    divoom_com_update_task(pdivoom_light_special_handle->task_id,time);
    divoom_light_special_task_id();     //传递taskid到下一级
    divoom_com_resume_task(pdivoom_light_special_handle->task_id);
}



//void divoom_light_special_start_new_mode(T_TIMER timer_id, uint32 delay)
void divoom_light_special_start_new_mode(void)
{
    uint32 time;
    
    if(pdivoom_light_special_handle == NULL)
    {
        return;
    }
    pdivoom_light_special_handle->time_id = T_U8_MAX;
    time = divoom_light_special_init_mode();   //更新完mode再进行新任务的初始化
    divoom_com_update_task(pdivoom_light_special_handle->task_id,time);
    divoom_light_special_task_id();     //传递taskid到下一级
//    if(ret)
//    {
//        Fwl_MicroTaskResumeAllTask();    
//    }
    divoom_com_resume_task(pdivoom_light_special_handle->task_id);
}


uint8 divoom_light_special_set_mode(Pdivoom_disp_ctrl_special_info data)
{
    uint8 mode ;

    mode = data->mode;
    //DIVOOM_LIGHT_SPECIAL_PRINT("divoom_light_special_set_mode: %d\n", mode);
    if(pdivoom_light_special_handle == NULL)
    {
        DIVOOM_LIGHT_SPECIAL_PRINT("divoom_light_special_update ERRRRRR\n");
        return 0;
    }

    if(mode >= DIVOOM_DISP_SPECIAL_END_MODE)
    {
        mode = 0;
    }
    
    if(mode == pdivoom_light_special_handle->mode)
    {
        return mode;
    }

/*
    Fwl_MicroTaskPauseAllTask();    
    divoom_disp_special_init_mode(mode);
    pdivoom_disp_special_handle->mode = mode;
    divoom_disp_special_main();
    divoom_disp_special_set_refresh_time();
    Fwl_MicroTaskResumeAllTask();
*/
//    ret = Fwl_MicroTaskPauseAllTask();     
    divoom_com_pause_task(pdivoom_light_special_handle->task_id);

    divoom_light_special_close_mode();  //关闭旧任务
    //divoom_screen_send_all();
    if(pdivoom_light_special_handle->time_id != T_U8_MAX)
    {
//        Fwl_TimerStop(pdivoom_light_special_handle->time_id);
    }
//    pdivoom_light_special_handle->time_id = Fwl_TimerStart(100, AK_FALSE, divoom_light_special_start_new_mode);
	divoom_light_special_start_new_mode();

    pdivoom_light_special_handle->mode = mode;  //更新任务
    return mode;
}







void divoom_light_special_play(uint8 status)
{
    if(status == 0)
    {
        divoom_com_pause_task(pdivoom_light_special_handle->task_id);
    }
    else
    {
    //    divoom_set_screen_to_single_color(pdivoom_light_special_handle->color.r_c, pdivoom_light_special_handle->color.g_c, pdivoom_light_special_handle->color.b_c); 
        divoom_com_resume_task(pdivoom_light_special_handle->task_id);
    }
}



/*********************************************************************
  Function:         divoom_light_special_close
  Description:     关闭灯效模式
  Input:            NONE
  Return:           NONE
  Author:           divoom
**********************************************************************/

void divoom_light_special_close(void)
{
    if(pdivoom_light_special_handle == NULL)
    {
        return;
    }
    if(pdivoom_light_special_handle->time_id != T_U8_MAX)
    {
//        Fwl_TimerStop(pdivoom_light_special_handle->time_id);
    }
    divoom_com_pause_task(pdivoom_light_special_handle->task_id);
    divoom_com_destroy_task(pdivoom_light_special_handle->task_id);
    divoom_light_special_close_mode();
    DIVOOM_LIGHT_SPECIAL_FREE(pdivoom_light_special_handle);
    pdivoom_light_special_handle = NULL;
	divoom_malloc_check_all_ram_out(__func__);
}





/**
 * @function:    divoom_light_special_init_mode
 * @description: 灯效中小模块的初始化
 * @Author:      coronaa
 * @date:        2017-07-07
 * @return                      [刷新时间]
 */
uint32 divoom_light_special_init_mode(void)
{
    uint32 time = DIVOOM_DISP_SPECIAL_REFRESH_TIME; 
    
    pdivoom_light_special_handle->tick = 0;
    switch(pdivoom_light_special_handle->mode)
    {
        case DIVOOM_DISP_SPECIAL_BIAS_MODE:    
        //case DIVOOM_DISP_SPECIAL_RAINBOW_MODE:    
        case DIVOOM_DISP_SPECIAL_BLOCK_MODE:    
        case DIVOOM_DISP_SPECIAL_DIAMONG_MODE:    
        case DIVOOM_DISP_SPECIAL_RANDOM_MODE:    
        case DIVOOM_DISP_SPECIAL_COUNT_DOWN_MODE:
            divoom_light_special_original_mode_init(pdivoom_light_special_handle->mode);
            time = DIVOOM_DISP_SPECIAL_REFRESH_TIME;   
            break;


        case DIVOOM_LIGHT_SPECIAL_NEON_MOVE_MODE:
            time = divoom_light_neon_move_init();
            break;

        case DIVOOM_LIGHT_SPECIAL_WINDMILL_MODE:
            time = divoom_light_windmill_init();
            break;
        case DIVOOM_LIGHT_SPECIAL_BREATHING_LIGHT_MODE:
            time = divoom_light_breathing_light_init();
            break;
        case DIVOOM_LIGHT_SPECIAL_COUNT_DOWN_GRADIENT_MODE:
            time = divoom_light_count_down_grad_init();
            break;
        case DIVOOM_LIGHT_SPECIAL_EXPLODE_MODE:
//            time = divoom_light_explode_init();
            break; 
        case DIVOOM_LIGHT_SPECIAL_PLASMA_MODE:
            time = divoom_light_plasma_init();
            break;  
		case DIVOOM_LIGHT_SPECIAL_FLASH_RAINBOW_MODE:
			time = divoom_light_specail_divoom_init(0);
			break;
		case DIVOOM_LIGHT_SPECIAL_FLASH_ROCK_MODE:
			time = divoom_light_specail_divoom_init(1);
			break;
		case DIVOOM_LIGHT_SPECIAL_FLASH_FIRE_MODE:
			time = divoom_light_specail_divoom_init(2);
			break;
		case DIVOOM_LIGHT_SPECIAL_FLASH_BLACK_MODE:
			time = divoom_light_specail_divoom_init(3);
			break;
		case DIVOOM_LIGHT_SPECIAL_FLASH_LINE_MODE:
			time = divoom_light_specail_divoom_init(4);
			break;
		case DIVOOM_LIGHT_SPECIAL_FLASH_CIRCULAR_MODE:
			time = divoom_light_specail_divoom_init(5);
			break;
		case DIVOOM_LIGHT_SPECIAL_FLASH_WATER_MODE:
			time = divoom_light_specail_divoom_init(6);
			break;
		case DIVOOM_LIGHT_SPECIAL_FLASH_BLOCK_MODE:
			time = divoom_light_specail_divoom_init(7);
			break;
        //default:
        //    time = divoom_light_specail_divoom_init();
            break;
    }
    return time;
}



/**
 * @function:    divoom_light_special_close_mode
 * @description: 灯效中小模块的关闭,释放内存
 * @Author:      coronaa
 * @date:        2017-06-29
 */
void divoom_light_special_close_mode(void)
{
    switch(pdivoom_light_special_handle->mode)
    {
        case DIVOOM_DISP_SPECIAL_BIAS_MODE:    
        case DIVOOM_DISP_SPECIAL_RAINBOW_MODE:    
        case DIVOOM_DISP_SPECIAL_BLOCK_MODE:    
        case DIVOOM_DISP_SPECIAL_DIAMONG_MODE:    
        case DIVOOM_DISP_SPECIAL_RANDOM_MODE:    
        case DIVOOM_DISP_SPECIAL_COUNT_DOWN_MODE:
            divoom_light_special_original_mode_close();
            break;
     
        case DIVOOM_LIGHT_SPECIAL_NEON_MOVE_MODE:
            divoom_light_neon_move_close();
            break;

        case DIVOOM_LIGHT_SPECIAL_WINDMILL_MODE:
            divoom_light_windmill_close();
            break;
        case DIVOOM_LIGHT_SPECIAL_BREATHING_LIGHT_MODE:
            divoom_light_breathing_light_close();
            break;
        case DIVOOM_LIGHT_SPECIAL_COUNT_DOWN_GRADIENT_MODE:
            divoom_light_count_down_grad_close();
            break;
        case DIVOOM_LIGHT_SPECIAL_EXPLODE_MODE:
//            divoom_light_explode_close();
            break;
        case DIVOOM_LIGHT_SPECIAL_PLASMA_MODE:
            divoom_light_plasma_close();
            break; 
        default:
            divoom_light_specail_divoom_exit();
            break;           
    }
}

/**
 * @function:    divoom_light_special_task_id
 * @description: 传递task_id
 * @Author:      coronaa
 * @date:        2017-07-03
 */
void divoom_light_special_task_id(void)
{
    switch(pdivoom_light_special_handle->mode)
    {
        // case DIVOOM_DISP_SPECIAL_BIAS_MODE:    
        // case DIVOOM_DISP_SPECIAL_RAINBOW_MODE:    
        // case DIVOOM_DISP_SPECIAL_BLOCK_MODE:    
        // case DIVOOM_DISP_SPECIAL_DIAMONG_MODE:    
        // case DIVOOM_DISP_SPECIAL_RANDOM_MODE:    
        // case DIVOOM_DISP_SPECIAL_COUNT_DOWN_MODE:

        //     break;
             
        case DIVOOM_LIGHT_SPECIAL_NEON_MOVE_MODE:
            divoom_light_neon_move_get_taskid(pdivoom_light_special_handle->task_id);
            break;

        case DIVOOM_LIGHT_SPECIAL_WINDMILL_MODE:
            divoom_light_windmill_get_taskid(pdivoom_light_special_handle->task_id);
            break;
        case DIVOOM_LIGHT_SPECIAL_BREATHING_LIGHT_MODE:
            divoom_light_breathing_light_get_taskid(pdivoom_light_special_handle->task_id);
            break;
        case DIVOOM_LIGHT_SPECIAL_COUNT_DOWN_GRADIENT_MODE:
            divoom_light_count_down_grad_get_taskid(pdivoom_light_special_handle->task_id);
            break;
        case DIVOOM_LIGHT_SPECIAL_EXPLODE_MODE:
      //      divoom_light_explode_get_taskid(pdivoom_light_special_handle->task_id);
            break;
        case DIVOOM_LIGHT_SPECIAL_PLASMA_MODE:
            divoom_light_plasma_get_taskid(pdivoom_light_special_handle->task_id);            
            break;
        default:
            break;    
    }
}
#endif

