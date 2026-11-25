#include "divoom_disp_com.h"
#include "divoom_light_breathing_light.h"
//#include "divoom_light_math.h"


#define DIVOOM_LIGHT_BREATHING_LIGHT_GET_RAND(x)     divoom_com_get_random(x)
#define DIVOOM_LIGHT_BREATHING_LIGHT_MALLOC(size)    divoom_com_malloc(size, __func__, __LINE__)
#define DIVOOM_LIGHT_BREATHING_LIGHT_FREE(size)      divoom_com_free(size, __func__, __LINE__)
#define DIVOOM_LIGHT_BREATHING_LIGHT_PRINT           divoom_print



#define DIVOOM_LIGHT_BREATHING_LIGHT_REFRESH_TIME     7


//#define BREATHING_LIGHT_COLOR_STEP_NUM    30   //


typedef struct _divoom_light_breathing_light_info
{
    uint8 mode;
    uint8 showtime;
} divoom_light_breathing_light_info, *Pdivoom_light_breathing_light_info;

enum
{
    DIVOOM_LIGHT_BREATHING_LIGHT_BREATH_RANDOM_MODE,
    DIVOOM_LIGHT_BREATHING_LIGHT_RANDOM_POINT_MODE,
    DIVOOM_LIGHT_BREATHING_LIGHT_SKY_MODE,
};

static const divoom_light_breathing_light_info disp_buf_breathing_info[]=
{
    {DIVOOM_LIGHT_BREATHING_LIGHT_BREATH_RANDOM_MODE, 10},
    // {DIVOOM_LIGHT_BREATHING_LIGHT_RANDOM_POINT_MODE, 100},
//    {DIVOOM_LIGHT_BREATHING_LIGHT_SKY_MODE, 100},
};



typedef struct _divoom_light_breathing_light_ctrl
{
    uint8 mode;
    uint8 total_disp_num;
    uint8 cur_time;
    uint8 end_time;
    uint8 slow_time;
    uint32 task_id;
    uint8 disp_id;
    uint8 array_len;
    divoom_light_color_info color;
//    divoom_light_color_info color_step_table[BREATHING_LIGHT_COLOR_STEP_NUM];
    uint8 buf[SCREEN_BUF_SIZE_32];
//    uint8 backbuf[SCREEN_BUF_SIZE_32];
    uint8 pre_buf[SCREEN_HEIGHT_32][SCREEN_WIDTH_32];   //存储各点呼吸灯的起始位置,存入随机数
    uint16 color_hue[SCREEN_HEIGHT_32][SCREEN_WIDTH_32];   //存储各点呼吸灯颜色点 
} divoom_light_breathing_light_ctrl, *Pdivoom_light_breathing_light_ctrl;



static Pdivoom_light_breathing_light_ctrl pdivoom_light_breathing_light_handle = NULL;

// static const uint8 divoom_breathing_color_list_fliker[] = {1,4,8,12,15,12,8,4};
static const uint8 divoom_breathing_color_list_fliker[] = {16,64,128,192,255,192,128,64};


#if 0
static const divoom_light_color_info breathing_color_palette_white[] =
{
    {0x0,0x0,0x0},{0x10,0x10,0x10},{0x20,0x20,0x20},{0x30,0x30,0x30},{0x40,0x40,0x40},{0x50,0x50,0x50},
    {0x60,0x60,0x60},{0x70,0x70,0x70},{0x80,0x80,0x80},{0x90,0x90,0x90},{0xa0,0xa0,0xa0},
    {0xb0,0xb0,0xb0},{0xc0,0xc0,0xc0},{0xd0,0xd0,0xd0},{0xe0,0xe0,0xe0},{0xff,0xff,0xff}
};
static const divoom_light_color_info breathing_color_palette_blue[] =
{
    {0,0,0},{0,0,1},{0,0,2},{0,0,3},{0,0,4},{0,0,5},{0,0,6},{0,0,7},{0,1,8},{0,0,9},
    {0,1,10},{0,0,11},{0,0,12},{0,0,13},{0,0,14},{0,1,15}
};


static const uint8 divoom_breathing_color_list_loop[] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1};


//星空
static const uint8 divoom_breathing_color_list_star[] = {0,0,0,15,15,15};



static const uint8 divoom_breathing_color_list_random[] = {1,1,1,3,3,3,6,6,6,3,3,3};
static const uint8 divoom_breathing_color_list_rotatepoint[] = {1,2,3,10,3,2};
static const uint8 divoom_breathing_color_list_star_arround[] = {3,4,6,8,6,3};

static const uint8 divoom_breathing_highlight_point[SCREEN_HEIGHT_32][SCREEN_WIDTH_32] =
{
    // 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    // 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    // 0, 0, 2, 0, 0, 0, 0, 0, 0, 2, 0,
    // 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 2,
    // 0, 0, 0, 0, 0, 2, 2, 0, 0, 2, 0,
    // 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 2,
    // 2, 0, 2, 2, 2, 0, 2, 0, 2, 0, 0,
    // 0, 0, 2, 2, 2, 2, 2, 2, 1, 0, 2,
    // 2, 2, 0, 2, 2, 2, 2, 0, 0, 2, 0,
    // 0, 2, 1, 2, 0, 2, 2, 2, 2, 0, 0,
    // 0, 2, 0, 2, 2, 2, 0, 2, 0, 0, 2

   { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
   { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
   { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
   { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
   { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
   { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
   { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
   { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
   { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
   { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
   { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
   { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
   { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
   { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
   { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
   { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }    
};

static const uint8 divoom_breathing_rotate_point_index[SCREEN_HEIGHT_32][SCREEN_WIDTH_32] =
{
    // 2, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0,
    // 0, 0, 1, 0, 0, 0, 0, 0, 3, 0, 0,
    // 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 4,
    // 0, 4, 0, 0, 0, 0, 1, 2, 0, 0, 0,
    // 0, 0, 0, 3, 0, 4, 0, 0, 0, 1, 0,
    // 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    // 0, 0, 0, 2, 0, 3, 0, 0, 0, 0, 0,
    // 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    // 0, 0, 1, 0, 4, 0, 0, 0, 0, 0, 0,
    // 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    // 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    
   { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
   { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
   { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
   { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
   { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
   { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
   { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
   { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
   { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
   { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
   { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
   { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
   { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
   { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
   { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
   { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }    

};

//    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
//    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
//    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
//    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
//    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
//    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
//    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
//    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
//    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
//    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
//    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
//    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
//    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
//    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
//    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
//    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }
#endif

/*********************************************************************
  Function:        divoom_light_breathing_light_color_list_init
  Description:      none

  Input:           none
  Author:          coronaa
  Data:            2017-5-18
**********************************************************************/
void divoom_light_breathing_light_color_list_init(uint8 *list,uint8 size,Pdivoom_light_color_info color_table,divoom_light_color_info * color_palette)
{
    uint8 i;
    for(i  = 0; i < size; i++)
        memcpy(color_table+i, color_palette+list[i], sizeof(divoom_light_color_info));
}

/*********************************************************************
  Function:        divoom_light_breathing_light_get_color_from_list
  Description:
               RGB值范围 :    0   1   2   3   4   5   6   7   8   9   10  11  12  13  14  15
               呼吸灯效果:    例:0 - 15 -0
               函数中用步骤index来控制呼吸灯，例:共30步       0 - 15   14 - 1

  Input:           none
  Author:          coronaa
  Data:            2017-5-17
**********************************************************************/
void divoom_light_breathing_light_get_color_from_list(const uint8 *list,uint8 size,uint8 index,Pdivoom_light_color_info color,const divoom_light_color_info * color_table)
{
    index %= size;
    if(NULL != list)
        memcpy(color, color_table + list[index], sizeof(divoom_light_color_info));
    else
        memcpy(color, color_table + index, sizeof(divoom_light_color_info));
}

/*********************************************************************
  Function:     divoom_light_breathing_light_mode_init
  Description:

  Input:           none
  Author:          coronaa
  Data:            2017-5-18
**********************************************************************/
void divoom_light_breathing_light_mode_init(uint8 mode)
{
    uint8 i,j;
    divoom_light_breathing_light_set_refresh_time();
    switch(mode)
    {
        case DIVOOM_LIGHT_BREATHING_LIGHT_BREATH_RANDOM_MODE:
//填充随机列表
            for(j = 0 ; j < SCREEN_HEIGHT_32; j++)
            {        
                for(i = 0 ; i < SCREEN_WIDTH_32; i++)
                {              
                    pdivoom_light_breathing_light_handle->pre_buf[j][i] = DIVOOM_LIGHT_BREATHING_LIGHT_GET_RAND(15);
                    pdivoom_light_breathing_light_handle->color_hue[j][i] = DIVOOM_LIGHT_BREATHING_LIGHT_GET_RAND(1536);
                }
            }                
            break;
        case DIVOOM_LIGHT_BREATHING_LIGHT_RANDOM_POINT_MODE:

            break;
        case DIVOOM_LIGHT_BREATHING_LIGHT_SKY_MODE:

            break;
        default:
            break;
    }
}




/*********************************************************************
  Function:     divoom_light_breathing_light_mode
  Description:

  Input:           none
  Author:          coronaa
  Data:            2017-5-18
**********************************************************************/
void divoom_light_breathing_light_mode(uint8 mode)
{
    uint16 index,i,j,base,k;
    // uint8 m,n;
    index = pdivoom_light_breathing_light_handle->cur_time;
    switch(mode)
    {
        case DIVOOM_LIGHT_BREATHING_LIGHT_BREATH_RANDOM_MODE:
            k = DIVOOM_ARRAY_LEN(divoom_breathing_color_list_fliker);
            for(j = 0, base = 0; j < SCREEN_HEIGHT_32; j++,base += SCREEN_WIDTH_32)
            {
                for(i = 0; i < SCREEN_WIDTH_32; i++)
                {
                    /*
                    if(j % 2 == 1 && i %2 == 1)
                    {
//                        divoom_light_breathing_light_get_color_from_list(divoom_breathing_color_list_fliker,k,
//                                index+pdivoom_light_breathing_light_handle->pre_buf[j][i],&pdivoom_light_breathing_light_handle->color,breathing_color_palette_white);
                        divoom_CreateHSVColor(0,0,(divoom_breathing_color_list_fliker[(index+pdivoom_light_breathing_light_handle->pre_buf[j][i])%k]),
                                              &pdivoom_light_breathing_light_handle->color);
                    }
                    else
                    {
                        // divoom_CreateHSVColor(0,0,0,&pdivoom_light_breathing_light_handle->color);
                        memset(&pdivoom_light_breathing_light_handle->color,0,sizeof(divoom_light_color_info));
                    }
                    */
                    if(j % 3 == 0 && i %3 == 0)
                    {
//                        divoom_light_breathing_light_get_color_from_list(divoom_breathing_color_list_fliker,k,
//                                index+pdivoom_light_breathing_light_handle->pre_buf[j][i],&pdivoom_light_breathing_light_handle->color,breathing_color_palette_white);
                        divoom_CreateHSVColor(pdivoom_light_breathing_light_handle->color_hue[j][i],180,(divoom_breathing_color_list_fliker[(index+pdivoom_light_breathing_light_handle->pre_buf[j][i])%k]),
                                              &pdivoom_light_breathing_light_handle->color);
                    }
                    else
                    {
                        // divoom_CreateHSVColor(0,0,0,&pdivoom_light_breathing_light_handle->color);
                        memset(&pdivoom_light_breathing_light_handle->color,0,sizeof(divoom_light_color_info));
                    }
                    divoom_disp_ctrl_set_data(base + i, &pdivoom_light_breathing_light_handle->color,pdivoom_light_breathing_light_handle->buf);

                }
            }        
            break;

/*       
        case DIVOOM_LIGHT_BREATHING_LIGHT_RANDOM_POINT_MODE:
            k = DIVOOM_ARRAY_LEN(divoom_breathing_color_list_loop);
            for(j = 0, base = 0; j < SCREEN_HEIGHT_32; j++,base += SCREEN_WIDTH_32)
            {
                for(i = 0; i < SCREEN_WIDTH_32; i++)
                {
                    divoom_light_breathing_light_get_color_from_list(divoom_breathing_color_list_loop,k,
                            DIVOOM_LIGHT_BREATHING_LIGHT_GET_RAND(30),&pdivoom_light_breathing_light_handle->color,breathing_color_palette_white);
                    divoom_disp_ctrl_set_data(base + i, &pdivoom_light_breathing_light_handle->color,pdivoom_light_breathing_light_handle->buf);
                }
            }                
            break;
*/
 /*    
        case DIVOOM_LIGHT_BREATHING_LIGHT_SKY_MODE:
            k = DIVOOM_ARRAY_LEN(divoom_breathing_color_list_random);
            m = DIVOOM_ARRAY_LEN(divoom_breathing_color_list_rotatepoint);
            n = DIVOOM_ARRAY_LEN(divoom_breathing_color_list_star);
            for(j = 0, base = 0; j < SCREEN_HEIGHT_32; j++,base += SCREEN_WIDTH_32)
            {
                for(i = 0; i < SCREEN_WIDTH_32; i++)
                {
                    divoom_light_breathing_light_get_color_from_list(divoom_breathing_color_list_random,k,
                            index+pdivoom_light_breathing_light_handle->pre_buf[j][i],&pdivoom_light_breathing_light_handle->color,breathing_color_palette_blue);

                    if(2 == divoom_breathing_highlight_point[j][i])
                    {
                        divoom_light_breathing_light_get_color_from_list(divoom_breathing_color_list_star_arround,n,
                                index,&pdivoom_light_breathing_light_handle->color,breathing_color_palette_blue);
                    }


                    if(divoom_breathing_rotate_point_index[j][i])
                    {
                        divoom_light_breathing_light_get_color_from_list(divoom_breathing_color_list_rotatepoint,m,
                                index+divoom_breathing_rotate_point_index[j][i],&pdivoom_light_breathing_light_handle->color,breathing_color_palette_blue);
                    }


                    if(1 == divoom_breathing_highlight_point[j][i])
                    {
                        if(breathing_color_palette_blue[divoom_breathing_color_list_star[index%n]].r_c ||
                           breathing_color_palette_blue[divoom_breathing_color_list_star[index%n]].g_c ||
                           breathing_color_palette_blue[divoom_breathing_color_list_star[index%n]].b_c)
                            divoom_light_breathing_light_get_color_from_list(divoom_breathing_color_list_star,n,
                                    index,&pdivoom_light_breathing_light_handle->color,breathing_color_palette_blue);
                    }

                    divoom_disp_ctrl_set_data(base + i, &pdivoom_light_breathing_light_handle->color,pdivoom_light_breathing_light_handle->buf);
                }
            }
            break;
*/
        default:
            break;
    }
}



void divoom_light_breathing_light_main(void)
{
    if(pdivoom_light_breathing_light_handle->cur_time >= pdivoom_light_breathing_light_handle->end_time)
    {
        pdivoom_light_breathing_light_handle->cur_time = 0;
        pdivoom_light_breathing_light_handle->disp_id ++;     //变更模式

        if(pdivoom_light_breathing_light_handle->disp_id >= pdivoom_light_breathing_light_handle->total_disp_num)
        {
            pdivoom_light_breathing_light_handle->disp_id = 0;
        }
        pdivoom_light_breathing_light_handle->mode = disp_buf_breathing_info[pdivoom_light_breathing_light_handle->disp_id].mode;
        pdivoom_light_breathing_light_handle->end_time = disp_buf_breathing_info[pdivoom_light_breathing_light_handle->disp_id].showtime;

        divoom_light_breathing_light_mode_init(pdivoom_light_breathing_light_handle->mode);
//        memset(pdivoom_light_breathing_light_handle->highlight_buf, 0, SCREEN_WIDTH_32 * SCREEN_HEIGHT_32);
    }
    else
    {
        divoom_light_breathing_light_mode(pdivoom_light_breathing_light_handle->mode);
    }

    pdivoom_light_breathing_light_handle->cur_time++;
//    if(0 == pdivoom_light_breathing_light_handle->cur_time % 3)
//        pdivoom_light_breathing_light_handle->slow_time++;
    divoom_screen_disp_rgb32_data(pdivoom_light_breathing_light_handle->buf);
}



uint32 divoom_light_breathing_light_get_refresh_time(void)
{
    uint32 refresh_time  = DIVOOM_LIGHT_BREATHING_LIGHT_REFRESH_TIME;

    switch(pdivoom_light_breathing_light_handle->mode)
    {
        case DIVOOM_LIGHT_BREATHING_LIGHT_BREATH_RANDOM_MODE:
//            refresh_time  = DIVOOM_LIGHT_BREATHING_LIGHT_BREATH_RANDOM_TIME;
            break;
        case DIVOOM_LIGHT_BREATHING_LIGHT_RANDOM_POINT_MODE:

            break;
        case DIVOOM_LIGHT_BREATHING_LIGHT_SKY_MODE:

            break;
        default:
            break;
    }
    return refresh_time;
}
void divoom_light_breathing_light_set_refresh_time(void)
{
    uint32 refresh_time  = divoom_light_breathing_light_get_refresh_time();
    divoom_com_update_task(pdivoom_light_breathing_light_handle->task_id, refresh_time);
}

uint32 divoom_light_breathing_light_init(void)
{
    uint8 i,j;
    pdivoom_light_breathing_light_handle = DIVOOM_LIGHT_BREATHING_LIGHT_MALLOC(sizeof(divoom_light_breathing_light_ctrl));
    if(pdivoom_light_breathing_light_handle == NULL)
    {
        divoom_print("%s:%d: it fails to malloc!\n",__FILE__, __LINE__);
        return 0; 
    }
    memset(pdivoom_light_breathing_light_handle, 0, sizeof(divoom_light_breathing_light_ctrl));
    pdivoom_light_breathing_light_handle->total_disp_num = sizeof(disp_buf_breathing_info)/sizeof(divoom_light_breathing_light_info);

//初始化mode
    pdivoom_light_breathing_light_handle->mode = disp_buf_breathing_info[pdivoom_light_breathing_light_handle->disp_id].mode;
    pdivoom_light_breathing_light_handle->end_time = disp_buf_breathing_info[pdivoom_light_breathing_light_handle->disp_id].showtime;

//填充随机列表
    for(j = 0 ; j < SCREEN_HEIGHT_32; j++)
    {        
        for(i = 0 ; i < SCREEN_WIDTH_32; i++)
        {              
            pdivoom_light_breathing_light_handle->pre_buf[j][i] = DIVOOM_LIGHT_BREATHING_LIGHT_GET_RAND(15);
            pdivoom_light_breathing_light_handle->color_hue[j][i] = DIVOOM_LIGHT_BREATHING_LIGHT_GET_RAND(1536);
        }
    }
            
/*
#if DIVOOM_LIGHT_USE_ANIMATION_MODE
    pdivoom_light_breathing_light_handle->task_id = divoom_light_special_task_id();
    divoom_light_breathing_light_set_refresh_time();
#else
    pdivoom_light_breathing_light_handle->task_id = divoom_com_create_task(divoom_light_breathing_light_main, DIVOOM_LIGHT_BREATHING_LIGHT_REFRESH_TIME);
    divoom_com_resume_task(pdivoom_light_breathing_light_handle->task_id);
#endif
*/
    return divoom_light_breathing_light_get_refresh_time();
}

/*
void divoom_light_breathing_light_play(uint8 play)
{
    if(pdivoom_light_breathing_light_handle)
    {
        if(play)
        {
            divoom_com_resume_task(pdivoom_light_breathing_light_handle->task_id);
        }
        else
        {
            divoom_com_pause_task(pdivoom_light_breathing_light_handle->task_id);
        }
    }
}
*/

void divoom_light_breathing_light_close(void)
{
/*    
#if !DIVOOM_LIGHT_USE_ANIMATION_MODE
    divoom_com_pause_task(pdivoom_light_breathing_light_handle->task_id);
    divoom_com_destroy_task(pdivoom_light_breathing_light_handle->task_id);
#endif
*/
    DIVOOM_LIGHT_BREATHING_LIGHT_FREE(pdivoom_light_breathing_light_handle);
    pdivoom_light_breathing_light_handle = NULL;
}


void divoom_light_breathing_light_get_taskid(uint32 id)
{
    pdivoom_light_breathing_light_handle->task_id = id;
}




/*****************************************************************************
 Prototype    : divoom_light_breathing_light_snow
 Description  : 雪花屏
 Input        : uint16 index        第N步  
                uint8 *start_pos    闪烁的起始位
                uint8 *disp_buf     显示BUF
 Output       : None
 Return Value : 
 
  History        :
  1.Date         : 2017/10/11
    Author       : coronaa
    Modification : Created function
*****************************************************************************/
void divoom_light_breathing_light_snow(uint16 index,uint8 *start_pos,uint8 *disp_buf)            
{
    uint16 i,j,base,k;
    divoom_light_color_info color; 
    k = DIVOOM_ARRAY_LEN(divoom_breathing_color_list_fliker);
    for(j = 0, base = 0; j < SCREEN_HEIGHT_32; j++)
    {
        for(i = 0; i < SCREEN_WIDTH_32; i++,base++)
        {
            if(j % 3 == 0 && i %3 == 0)
            {
                divoom_CreateHSVColor(0,0,(divoom_breathing_color_list_fliker[(index+(*(start_pos+base)))%k]),&color);
            }
            else
            {
                memset(&color,0,sizeof(divoom_light_color_info));
            }
            divoom_disp_ctrl_set_data(base, &color,disp_buf);
        }
    }    
} 

/*****************************************************************************
 Prototype    : divoom_light_breathing_light_get_random_start
 Description  : 获取256个点的随机数
 Input        : uint8 *start_pos  
                uint8 max         
 Output       : None
 Return Value : 
 
  History        :
  1.Date         : 2017/10/12
    Author       : coronaa
    Modification : Created function
*****************************************************************************/
void divoom_light_breathing_light_get_random_start(uint8 *start_pos,uint8 max)  
{
    uint16 i;
    for(i = 0 ; i < SCREEN_DOT_NUM; i++)
    {              
        *(start_pos+i) = DIVOOM_LIGHT_BREATHING_LIGHT_GET_RAND(max);
    }
}    





