
#include "divoom_disp_com.h"
#include "divoom_game_snake.h"
#include "divoom_game_flappy_bird.h"
#include "divoom_game_ball.h"
#include "divoom_game_car.h"
#include "divoom_game_tetris.h"
#include "divoom_game_plane.h"
#include "divoom_game_random.h"
#include "divoom_game_slot_machine.h"
#include "divoom_game_blockhit.h"
#include "divoom_game_gobang.h"
#include "divoom_game_galaxian.h"
#include "divoom_game_frogger.h"
#include "divoom_game_match_pic.h"
#include "divoom_game_supperzzle.h"
#include "divoom_game_tank.h"


#if 0

#define DIVOOM_DISP_GAME_REFRESH_TIME       10




Pdivoom_disp_game_ctrl pdivoom_disp_game_handle = NULL;


static const uint8 divoom_disp_game_result_win[13*32] = 
{
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 2, 2, 0, 0, 2, 2, 0, 0, 2, 2, 0, 0, 0, 2, 2, 0, 0, 0, 2, 2, 0, 0, 0, 0, 2, 2, 0, 0, 0, 
0, 0, 0, 2, 2, 0, 0, 2, 2, 0, 0, 2, 2, 0, 0, 0, 2, 2, 0, 0, 0, 2, 2, 2, 2, 0, 0, 2, 2, 0, 0, 0, 
0, 0, 0, 2, 2, 0, 0, 2, 2, 0, 0, 2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 0, 0, 2, 2, 0, 0, 0, 
0, 0, 0, 2, 2, 0, 0, 2, 2, 0, 0, 2, 2, 0, 0, 0, 2, 2, 0, 0, 0, 2, 2, 2, 2, 0, 0, 2, 2, 0, 0, 0, 
0, 0, 0, 2, 2, 0, 0, 2, 2, 0, 0, 2, 2, 0, 0, 0, 2, 2, 0, 0, 0, 2, 2, 0, 0, 2, 2, 2, 2, 0, 0, 0, 
0, 0, 0, 2, 2, 0, 0, 2, 2, 0, 0, 2, 2, 0, 0, 0, 2, 2, 0, 0, 0, 2, 2, 0, 0, 2, 2, 2, 2, 0, 0, 0, 
0, 0, 0, 2, 2, 0, 0, 2, 2, 0, 0, 2, 2, 0, 0, 0, 2, 2, 0, 0, 0, 2, 2, 0, 0, 2, 2, 2, 2, 0, 0, 0, 
0, 0, 0, 0, 0, 2, 2, 0, 0, 2, 2, 0, 0, 0, 0, 0, 2, 2, 0, 0, 0, 2, 2, 0, 0, 0, 0, 2, 2, 0, 0, 0, 
0, 0, 0, 0, 0, 2, 2, 0, 0, 2, 2, 0, 0, 0, 0, 0, 2, 2, 0, 0, 0, 2, 2, 0, 0, 0, 0, 2, 2, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
};
static const uint8 divoom_disp_game_result_over[13*32] = 
{
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
0, 0, 0, 0, 2, 2, 2, 2, 0, 0, 2, 2, 0, 0, 2, 2, 0, 2, 2, 2, 2, 2, 2, 0, 2, 2, 2, 2, 2, 2, 0, 0, 
0, 0, 0, 2, 2, 2, 2, 2, 2, 0, 2, 2, 0, 0, 2, 2, 0, 2, 2, 2, 2, 2, 2, 0, 2, 2, 2, 2, 2, 2, 0, 0, 
0, 0, 0, 2, 2, 0, 0, 2, 2, 0, 2, 2, 0, 0, 2, 2, 0, 2, 2, 0, 0, 0, 0, 0, 2, 2, 0, 0, 2, 2, 0, 0, 
0, 0, 0, 2, 2, 0, 0, 2, 2, 0, 2, 2, 0, 0, 2, 2, 0, 2, 2, 0, 0, 0, 0, 0, 2, 2, 0, 0, 2, 2, 0, 0, 
0, 0, 0, 2, 2, 0, 0, 2, 2, 0, 2, 2, 0, 0, 2, 2, 0, 2, 2, 2, 2, 2, 2, 0, 2, 2, 0, 0, 2, 2, 0, 0, 
0, 0, 0, 2, 2, 0, 0, 2, 2, 0, 2, 2, 0, 0, 2, 2, 0, 2, 2, 2, 2, 2, 2, 0, 2, 2, 2, 2, 2, 2, 0, 0, 
0, 0, 0, 2, 2, 0, 0, 2, 2, 0, 2, 2, 0, 0, 2, 2, 0, 2, 2, 0, 0, 0, 0, 0, 2, 2, 2, 2, 2, 2, 0, 0, 
0, 0, 0, 2, 2, 0, 0, 2, 2, 0, 2, 2, 0, 0, 2, 2, 0, 2, 2, 0, 0, 0, 0, 0, 2, 2, 2, 2, 0, 0, 0, 0, 
0, 0, 0, 2, 2, 0, 0, 2, 2, 0, 2, 2, 0, 0, 2, 2, 0, 2, 2, 0, 0, 0, 0, 0, 2, 2, 0, 2, 2, 0, 0, 0, 
0, 0, 0, 2, 2, 2, 2, 2, 2, 0, 0, 0, 2, 2, 0, 0, 0, 2, 2, 2, 2, 2, 2, 0, 2, 2, 0, 0, 2, 2, 0, 0, 
0, 0, 0, 0, 2, 2, 2, 2, 0, 0, 0, 0, 2, 2, 0, 0, 0, 2, 2, 2, 2, 2, 2, 0, 2, 2, 0, 0, 2, 2, 0, 0, 
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
};

void divoom_disp_game_add_32x32result(uint8 *disp_buf,uint8 flag)
{
    int16 i;
	const uint8 *map_table;
//	divoom_screen_color_info color1 = {255,170,0},color2 = {255,255,255};
	divoom_screen_color_info color2 = {0,0,0};
//    memset(pdivoom_tank_handle->disp_buf,0,DIVOOM_GAME_TANK_SCREEN_BUF_SIZE);
//画地图
	if(flag)
		map_table = divoom_disp_game_result_win;
	else
		map_table = divoom_disp_game_result_over;
	for(i = 32*11; i < 32*24;i++)
	{
//		if(map_table[i] == 1)
//		{
//			divoom_disp_ctrl_set_data(i, &color1, disp_buf);
//		}
//		else 
		if(map_table[i-32*11] == 2)
		{
			divoom_disp_ctrl_set_data(i, &color2,disp_buf);
		}
	}
}



static const divoom_screen_color_info divoom_disp_game_sep_color = {0, 0, 0};
static const divoom_screen_color_info divoom_disp_game_result_color = {0xFF, 0xFF, 0};



/**
写印射表
**/
void divoom_game_draw_map_model(int16 start_x, int16 start_y, game_model_info *pmodel, game_board_info *pboard, uint8 type)
{
	int16 i,j,x,y;
    uint16 tmp = 0;
    for(i = 0; i < pmodel->size_y; i ++)
    {
        for(j = 0; j < pmodel->size_x; j ++)
        {
            tmp = (*(pmodel->map + i)) & (1 << (pmodel->size_x-j-1));            
            if(tmp)            
            {
				x = start_x + j;
				y = start_y + i;
				if(x < 0 || y < 0 || x >= pboard->w || y >= pboard->h)
					continue;
				pboard->buf[y * pboard->w + x] = type;
            }
        }
    }
}

int8 divoom_game_check_map_model_type(int16 start_x, int16 start_y, game_model_info *pmodel, game_board_info *pboard,int16 type)
{
	int16 i,j,x,y;
    uint16 tmp = 0;
    for(i = 0; i < pmodel->size_y; i ++)
    {
        for(j = 0; j < pmodel->size_x; j ++)
        {
            tmp = (*(pmodel->map + i)) & (1 << (pmodel->size_x-j-1));            
            if(tmp)            
            {
				x = start_x + j;
				y = start_y + i;
				if(x < 0 || y < 0 || x >= pboard->w || y >= pboard->h)	//超范围
					continue;
				tmp = pboard->buf[y * pboard->w + x];
				if(tmp == type)
					return 1;
            }
        }
    }
	return 0;
}

int8 divoom_game_check_map_model_func(int16 start_x, int16 start_y, game_model_info *pmodel, game_board_info *pboard,DIVOOM_GAME_MODEL_COMPARE_FUNC callback)
{
	int16 i,j,x,y;
    uint16 tmp = 0;
	int8 ret;
    for(i = 0; i < pmodel->size_y; i ++)
    {
        for(j = 0; j < pmodel->size_x; j ++)
        {
            tmp = (*(pmodel->map + i)) & (1 << (pmodel->size_x-j-1));            
            if(tmp)            
            {
				x = start_x + j;
				y = start_y + i;
				if(x < 0 || y < 0 || x >= pboard->w || y >= pboard->h)	//超范围
					continue;
				tmp = pboard->buf[y * pboard->w + x];
				ret = callback(tmp);
				if(0 != ret)
					return ret;
            }
        }
    }
	return 0;
}



/**
画游戏得分数字，颜色已定
**/
void divoom_disp_game_draw_small_score_digit(uint8 start_x, uint8 start_y, uint8 user_data, uint8 *disp_buf)
{
#if 1//(SCREEN_WIDTH_32 == 16)
    uint16 i, j, off;
    for(i = 0, off = start_y * DIVOOM_GAME_SMALL_WIDTH; i < 5; i ++, off += DIVOOM_GAME_SMALL_WIDTH)
    {
        for(j = 0; j < 3; j ++)
        {
            if(divoom_disp_game_get_digit(j, i, user_data))
            {
                divoom_disp_ctrl_set_data(start_x + j + off, &divoom_disp_game_sep_color, disp_buf);
            }
        }
    }
#else
    divoom_disp_digit_fill_area(start_x, start_y, user_data,
                                DIVOOM_6X10_FANG_DIGIT, disp_buf, &divoom_disp_game_sep_color);
#endif

}

void divoom_disp_game_draw_small_score(uint16 score, uint8 *disp_buf)
{
    if(score > 99)
    {
        divoom_disp_game_draw_small_score_digit(2, 5, score / 100, disp_buf);
        score %= 100;
        divoom_disp_game_draw_small_score_digit(7, 5, score / 10, disp_buf);
        score %= 10;
        divoom_disp_game_draw_small_score_digit(12, 5, score, disp_buf);
    }
    else if(score > 9)
    {
        divoom_disp_game_draw_small_score_digit(4, 5, score / 10, disp_buf);
        score %= 10;
        divoom_disp_game_draw_small_score_digit(9, 5, score, disp_buf);
    }
    else
    {
        divoom_disp_game_draw_small_score_digit(6, 5, score, disp_buf);
    }
}

void divoom_disp_game_shine_score(uint16 score, uint8 *disp_buf, uint8 flag)
{
    static uint16 index = 0;
	#define DIVOOM_GAME_SHINE_DIGIT		DIVOOM_4X7_DIGIT
    if(DIVOOM_GAME_PIXELS_32X32 == flag)
    {
        if(index++ & 1)
        {
            if(score > 9999)
            {
                divoom_disp_digit_fill_area(0, 13, score / 10000, DIVOOM_GAME_SHINE_DIGIT, disp_buf,&divoom_disp_game_sep_color);
                score %= 10000;
                divoom_disp_digit_fill_area(7, 13, score / 1000, DIVOOM_GAME_SHINE_DIGIT, disp_buf,&divoom_disp_game_sep_color);
                score %= 1000;
                divoom_disp_digit_fill_area(14, 13, score / 100, DIVOOM_GAME_SHINE_DIGIT, disp_buf,&divoom_disp_game_sep_color);
                score %= 100;
                divoom_disp_digit_fill_area(21, 13, score / 10, DIVOOM_GAME_SHINE_DIGIT, disp_buf,&divoom_disp_game_sep_color);
                score %= 10;
                divoom_disp_digit_fill_area(28, 13, score, DIVOOM_GAME_SHINE_DIGIT, disp_buf,&divoom_disp_game_sep_color);
            }
            else if(score > 999)
            {
                divoom_disp_digit_fill_area(2, 13, score / 1000, DIVOOM_GAME_SHINE_DIGIT, disp_buf,&divoom_disp_game_sep_color);
                score %= 1000;
                divoom_disp_digit_fill_area(10, 13, score / 100, DIVOOM_GAME_SHINE_DIGIT, disp_buf,&divoom_disp_game_sep_color);
                score %= 100;
                divoom_disp_digit_fill_area(18, 13, score / 10, DIVOOM_GAME_SHINE_DIGIT, disp_buf,&divoom_disp_game_sep_color);
                score %= 10;
                divoom_disp_digit_fill_area(26, 13, score, DIVOOM_GAME_SHINE_DIGIT, disp_buf,&divoom_disp_game_sep_color);
            }
            else if(score > 99)
            {
                divoom_disp_digit_fill_area(6, 13, score / 100,DIVOOM_GAME_SHINE_DIGIT, disp_buf,&divoom_disp_game_sep_color);
                score %= 100;
                divoom_disp_digit_fill_area(14, 13, score / 10,DIVOOM_GAME_SHINE_DIGIT, disp_buf,&divoom_disp_game_sep_color);
                score %= 10;
                divoom_disp_digit_fill_area(22, 13, score,DIVOOM_GAME_SHINE_DIGIT, disp_buf,&divoom_disp_game_sep_color);
            }
            else if(score > 9)
            {
                divoom_disp_digit_fill_area(9, 13, score / 10,DIVOOM_GAME_SHINE_DIGIT, disp_buf,&divoom_disp_game_sep_color);
                score %= 10;
                divoom_disp_digit_fill_area(19, 13, score,DIVOOM_GAME_SHINE_DIGIT, disp_buf,&divoom_disp_game_sep_color);
            }
            else
            {
                divoom_disp_digit_fill_area(14, 13, score,DIVOOM_GAME_SHINE_DIGIT, disp_buf,&divoom_disp_game_sep_color);
            }
        }
        else
        {
            divoom_disp_ctrl_fill(disp_buf, &divoom_disp_game_result_color);
        }

    }
    else if(DIVOOM_GAME_PIXELS_16X16 == flag)
    {
        if(index++ & 1)
        {
            if(score > 999)
            {
                divoom_disp_game_draw_small_score_digit(1, 5, score / 1000, disp_buf);
                score %= 1000;
                divoom_disp_game_draw_small_score_digit(5, 5, score / 100, disp_buf);
                score %= 100;
                divoom_disp_game_draw_small_score_digit(9, 5, score / 10, disp_buf);
                score %= 10;
                divoom_disp_game_draw_small_score_digit(13, 5, score, disp_buf);
            }
            else if(score > 99)
            {
                divoom_disp_game_draw_small_score_digit(2, 5, score / 100, disp_buf);
                score %= 100;
                divoom_disp_game_draw_small_score_digit(7, 5, score / 10, disp_buf);
                score %= 10;
                divoom_disp_game_draw_small_score_digit(12, 5, score, disp_buf);
            }
            else if(score > 9)
            {
                divoom_disp_game_draw_small_score_digit(4, 5, score / 10, disp_buf);
                score %= 10;
                divoom_disp_game_draw_small_score_digit(9, 5, score, disp_buf);
            }
            else
            {
                divoom_disp_game_draw_small_score_digit(6, 5, score, disp_buf);
            }
        }
        else
        {
            divoom_disp_game_small_screen_ctrl_fill(disp_buf, &divoom_disp_game_result_color);
        }
    }
}




void divoom_disp_game_add_background(uint8 *base_buf, uint8 *back_ground_buf,uint8 flag)
{
    uint16 i, j, off;
    divoom_screen_color_info color;
    if(DIVOOM_GAME_PIXELS_16X16 == flag)
    {
        for(i = 5; i < DIVOOM_GAME_SMALL_HIGH - 4 ; i++)
        {
            off = i * DIVOOM_GAME_SMALL_WIDTH;
            for(j = 0; j < DIVOOM_GAME_SMALL_WIDTH; j ++)
            {
                divoom_disp_ctrl_get_data(off + j,&color, back_ground_buf);
                divoom_disp_ctrl_set_data(off + j, &color, base_buf);
            }
        }
    }
    else if(DIVOOM_GAME_PIXELS_32X32 == flag)
    {
        for(i = 11; i < SCREEN_HEIGHT_32 - 8 ; i++)
        {
            off = i * SCREEN_WIDTH_32;
            for(j = 0; j < SCREEN_WIDTH_32; j ++)
            {
                divoom_disp_ctrl_get_data(off + j,&color, back_ground_buf);
                divoom_disp_ctrl_set_data(off + j, &color, base_buf);
            }
        }
    }
}
void divoom_disp_game_start_page(uint8 *disp_buf,uint8 flag)
{
    uint32 tmp = pdivoom_disp_game_handle->run_times/2;
    if(DIVOOM_GAME_PIXELS_16X16 == flag)
    {
        if(tmp & 1)
        {
            divoom_disp_picture_get_small_rgb_data(DIVOOM_DISP_GAME_START_MENU_MOVE_00, pdivoom_disp_game_handle->tmp_buf);
        }
        else
        {
            divoom_disp_picture_get_small_rgb_data(DIVOOM_DISP_GAME_START_MENU_MOVE_01, pdivoom_disp_game_handle->tmp_buf);
        }
        divoom_disp_game_add_background(disp_buf, pdivoom_disp_game_handle->tmp_buf,DIVOOM_GAME_PIXELS_16X16);
    }
    else if(DIVOOM_GAME_PIXELS_32X32 == flag)
    {
        if(tmp & 1)
        {
            divoom_disp_picture_get_rgb_data(DIVOOM_DISP_GAME_START_MENU_MOVE_00, pdivoom_disp_game_handle->tmp_buf);
        }
        else
        {
            divoom_disp_picture_get_rgb_data(DIVOOM_DISP_GAME_START_MENU_MOVE_01, pdivoom_disp_game_handle->tmp_buf);
        }
        divoom_disp_game_add_background(disp_buf, pdivoom_disp_game_handle->tmp_buf,DIVOOM_GAME_PIXELS_32X32);
    }
    pdivoom_disp_game_handle->run_times ++;
}

void divoom_disp_game_exit_page(uint8 *disp_buf)
{
    divoom_disp_picture_get_rgb_data(DIVOOM_DISP_GAME_EXIT_MENU, disp_buf);
}


//------------------------------------------------------------------------------------------------
/*****************************************************************************
 Prototype    : divoom_disp_game_screen_rgb_data
 Description  : 游戏显示中间接口
 Input        : uint8 *data
                uint8 flag   DIVOOM_GAME_PIXELS_32X32：32*32的显示数据直接显示， DIVOOM_GAME_PIXELS_16X16：16*16的数据，转一下
 Output       : None
 Return Value :

  History        :
  1.Date         : 2020/1/16
    Author       : coronaa
    Modification : Created function
*****************************************************************************/
void divoom_disp_game_screen_rgb_data(uint8 *data,uint8 flag)
{
    if(flag == DIVOOM_GAME_PIXELS_32X32)
    {
        divoom_screen_disp_rgb32_data(data);
    }
    else if(flag == DIVOOM_GAME_PIXELS_16X16)
    {
        divoom_disp_ctrl_pic_extension(pdivoom_disp_game_handle->tmp_buf,data, 16,2);
        divoom_screen_disp_rgb32_data(pdivoom_disp_game_handle->tmp_buf);
    }
}

void divoom_disp_game_small_screen_ctrl_fill(uint8 *disp_buf, const divoom_screen_color_info * color)
{
    uint16 off;

    for(off = 0; off < DIVOOM_GAME_SMALL_WIDTH * DIVOOM_GAME_SMALL_HIGH; off ++)
    {
        divoom_disp_ctrl_set_data(off, color, disp_buf);
    }
}

void divoom_disp_game_set_small_dot(int16 x,int16 y, Pdivoom_screen_color_info pcolor, uint8 *disp_buf)
{
    uint16 off;
    if(x < 0 || y < 0 || x >= DIVOOM_GAME_SMALL_WIDTH || y >= DIVOOM_GAME_SMALL_HIGH)
        return;
    off = (y * DIVOOM_GAME_SMALL_WIDTH + x);
    off = (off << 1) + off;
    disp_buf[off] = pcolor->r_c;
    disp_buf[off+1] = pcolor->g_c;
    disp_buf[off+2] = pcolor->b_c;
}


void divoom_disp_game_mode_close(void)
{
    if(pdivoom_disp_game_handle->func_handle_close_callback)
    {
        pdivoom_disp_game_handle->func_handle_loop_callback = NULL;
        pdivoom_disp_game_handle->func_handle_key_callback = NULL;
        pdivoom_disp_game_handle->func_handle_start_callback = NULL;
        pdivoom_disp_game_handle->func_handle_get_statu_callback = NULL;
        pdivoom_disp_game_handle->func_handle_close_callback();
		divoom_game_release_app_key(0);
    }
}

void divoom_disp_game_mode_init(uint8 mode)
{
    uint16 time = 0;
    pdivoom_disp_game_handle->reflesh_time = DIVOOM_DISP_GAME_REFRESH_TIME;
    switch(mode)
    {
        case DIVOOM_DISP_GAME_TARGE:
            time = divoom_disp_game_slot_machine_init(pdivoom_disp_game_handle);
            break;

        case DIVOOM_DISP_GAME_DICE:
        case DIVOOM_DISP_GAME_ANSWER:
        case DIVOOM_DISP_GAME_GUESS:
            time = divoom_disp_game_random_init(pdivoom_disp_game_handle,mode);
            break;
        case DIVOOM_DISP_GAME_PLANE:
            time = divoom_disp_game_plane_init(pdivoom_disp_game_handle);
            break;
        case DIVOOM_DISP_GAME_CAR:
            time = divoom_disp_game_car_init(pdivoom_disp_game_handle);
            break;
        case DIVOOM_DISP_GAME_BALL:
            time = divoom_disp_game_ball_init(pdivoom_disp_game_handle);
            break;
        case DIVOOM_DISP_GAME_SNAKE:
            time = divoom_disp_game_snake_init(pdivoom_disp_game_handle);
            break;
        case DIVOOM_DISP_GAME_FLAPPY_BIRD:
            time = divoom_disp_game_flappy_bird_init(pdivoom_disp_game_handle);
            break;
        case DIVOOM_DISP_GAME_TETRIS:
            time = divoom_disp_game_tetris_init(pdivoom_disp_game_handle);
            break;
        case DIVOOM_DISP_GAME_BLOCKHIT:
            time = divoom_disp_game_blockhit_init(pdivoom_disp_game_handle);
            break;

        case DIVOOM_DISP_GAME_GOBANG:
            time = divoom_disp_game_gobang_init(pdivoom_disp_game_handle);
            break;

        case DIVOOM_DISP_GAME_Galaxian:
            time = divoom_disp_game_galaxian_init(pdivoom_disp_game_handle);
            break;
        case DIVOOM_DISP_GAME_FROGGER:
            time = divoom_disp_game_frogger_init(pdivoom_disp_game_handle);
            break;

        case DIVOOM_DISP_GAME_MatchPic:
            time = divoom_disp_game_matchpic_init(pdivoom_disp_game_handle);
            break;

        case DIVOOM_DISP_GAME_Supperzzle:
            time = divoom_disp_game_supperzzle_init(pdivoom_disp_game_handle);
            break;

        case DIVOOM_DISP_GAME_TANK:
            time = divoom_disp_game_tank_init(pdivoom_disp_game_handle);
            break;		

        default:
            break;
    }
    if(time)
    {
        pdivoom_disp_game_handle->reflesh_time = time;
        if(pdivoom_disp_game_handle->task_id)
            divoom_com_update_task(pdivoom_disp_game_handle->task_id, pdivoom_disp_game_handle->reflesh_time);
    }


}


void divoom_disp_game_set_game(Pdivoom_disp_ctrl_game_info game_info)
{
    uint8 mode = game_info->mode;
    uint8 key_id = game_info->key_id;
    if(pdivoom_disp_game_handle==NULL)
    {
        return;
    }
//    divoom_print("divoom_disp_game_set_game:cur: %d;new:%d\n", pdivoom_disp_game_handle->game_id , mode);
//    divoom_print("key_id = %d\n", key_id);
    if(pdivoom_disp_game_handle->game_id == mode)
    {
        //分离light和APP层,如果在当前模式,mode相同,就当做是取得成绩
        divoom_disp_game_start();
        return;
    }
    else if(mode == 0xFF)
    {
        //此时为按键信息发过来了
        divoom_disp_game_key_function(key_id);
        return;
    }

    divoom_disp_game_mode_close();
    pdivoom_disp_game_handle->game_id = mode;
    divoom_disp_game_mode_init(pdivoom_disp_game_handle->game_id);

    //divoom_com_play_voice(T_U16_MAX, 0,DIVOOM_DISP_GAME_VOICE, NULL);

}


uint8 divoom_disp_game_get_game_status(void)
{
    uint8 ret = DIVOOM_DISP_GAME_RESULT;
    if(pdivoom_disp_game_handle == NULL)
    {
        return DIVOOM_DISP_GAME_RESULT;
    }

    if(pdivoom_disp_game_handle->func_handle_get_statu_callback)
    {
        ret = pdivoom_disp_game_handle->func_handle_get_statu_callback();
    }
    return ret;

}

void divoom_disp_game_main(void * data)
{
    uint8 game_status = 0;
    if(NULL == pdivoom_disp_game_handle)
        return;

    if(pdivoom_disp_game_handle->func_handle_loop_callback)
    {
        game_status = divoom_disp_game_get_game_status();   //统一显示退出界面菜单
        if(DIVOOM_DISP_GAME_EXIT == game_status)
        {
            divoom_disp_game_exit_page(pdivoom_disp_game_handle->tmp_buf);
            divoom_disp_game_screen_rgb_data(pdivoom_disp_game_handle->tmp_buf,DIVOOM_GAME_PIXELS_32X32);
            return;
        }
        pdivoom_disp_game_handle->func_handle_loop_callback();
    }
}


void divoom_disp_game_init(void *user_data)
{
    uint8 game_type;

    divoom_malloc_check_all_ram_in();
    if(pdivoom_disp_game_handle)
    {
        return;
    }
    if(user_data)
    {
        game_type = *(uint8 *)user_data;
    }
    else
    {
        game_type = DIVOOM_DISP_GAME_TARGE;
    }
    if(game_type > DIVOOM_DISP_GAME_MAX_TYPE)
    {
        divoom_print("divoom_disp_game_init: it err at game type: %d\n", game_type);
        return;
    }
    pdivoom_disp_game_handle = (Pdivoom_disp_game_ctrl)divoom_com_malloc(sizeof(divoom_disp_game_ctrl), __func__, __LINE__);
    if(pdivoom_disp_game_handle == NULL)
    {
        divoom_print("%s:%d: it fails to malloc!\n",__FILE__, __LINE__);
        return ;
    }
    memset(pdivoom_disp_game_handle, 0, sizeof(divoom_disp_game_ctrl));
    pdivoom_disp_game_handle->game_id = game_type;
    pdivoom_disp_game_handle->task_id = divoom_com_create_task(divoom_disp_game_main,DIVOOM_DISP_GAME_REFRESH_TIME);
    divoom_disp_game_mode_init(game_type);
    divoom_com_resume_task(pdivoom_disp_game_handle->task_id);
}



uint8 divoom_disp_game_get_game_mode(void)
{
    if(pdivoom_disp_game_handle==NULL)
    {
        return DIVOOM_DISP_GAME_TARGE;
    }
    return pdivoom_disp_game_handle->game_id;
}

void divoom_disp_game_next_mode(void)
{
    divoom_disp_ctrl_game_info info  = {0};

    uint8 mode;
    if(pdivoom_disp_game_handle==NULL)
    {
        return;
    }
    mode = pdivoom_disp_game_handle->game_id;
    mode++;
    if(mode >= DIVOOM_DISP_GAME_MAX_TYPE)
        mode = 0;
    info.mode  = mode;

    divoom_disp_game_set_game(&info);
}

void divoom_disp_game_pre_mode(void)
{
    divoom_disp_ctrl_game_info info  = {0};
    uint8 mode;
    if(pdivoom_disp_game_handle==NULL)
    {
        return;
    }
    mode = pdivoom_disp_game_handle->game_id;
    if(mode > 0)
    {
        mode --;
    }
    else
    {
        mode = DIVOOM_DISP_GAME_MAX_TYPE -1;
    }
    info.mode  = mode;
    divoom_disp_game_set_game(mode);
}


void divoom_disp_game_start(void)
{
    if(pdivoom_disp_game_handle == NULL)
        return;

    if(pdivoom_disp_game_handle->func_handle_start_callback)
    {
        pdivoom_disp_game_handle->func_handle_start_callback();
    }
}

void divoom_disp_game_play(uint8 play)
{
    if(play == 0)
    {
        divoom_com_pause_task(pdivoom_disp_game_handle->task_id);
    }
    else
    {
        divoom_com_resume_task(pdivoom_disp_game_handle->task_id);
    }
//    divoom_disp_game_play_voice(pdivoom_disp_game_handle->game_id, play);
}

//正常
void divoom_disp_game_close(void)
{
    if(pdivoom_disp_game_handle == NULL)
    {
        return;
    }
    divoom_com_pause_task(pdivoom_disp_game_handle->task_id);
    divoom_disp_game_mode_close();
    //divoom_com_play_voice(T_U16_MAX, 0,DIVOOM_DISP_GAME_VOICE, NULL);
    divoom_com_destroy_task(pdivoom_disp_game_handle->task_id);
    divoom_com_free(pdivoom_disp_game_handle, __func__, __LINE__);
    pdivoom_disp_game_handle = NULL;
    divoom_malloc_check_all_ram_out(__func__);
}


void divoom_disp_game_key_function(uint8 key_id)
{

    if(pdivoom_disp_game_handle == NULL)
        return;
//    divoom_print("%s:%d\n",__func__,key_id);
    if(pdivoom_disp_game_handle->func_handle_key_callback)
    {
        pdivoom_disp_game_handle->func_handle_key_callback(key_id);
    }
}

T_BOOL divoom_disp_game_is_true(void)
{
    if(pdivoom_disp_game_handle)
    {
        return AK_TRUE;
    }
    else
    {
        return AK_FALSE;
    }
}

//在退出状态下使用按键操作确认退出游戏
void divoom_disp_game_exit(void)
{


}


void divoom_game_send_key_msg(uint8 key)
{
#if 0
	divoom_disp_ctrl_game_info game_info = {0};
	divoom_sys_msg_info msg_info = {0};
	game_info.mode = 0xFF;
	game_info.key_id = key;
	msg_info.data = (uint8 *)&game_info;
	msg_info.dest_id = DIVOOM_MODEL_APP_GAME;
	msg_info.msg_id = DIVOOM_APP_GAME_PLAY_KEY_INFO;
	divoom_sys_model_crl_msg_push(&msg_info);
#endif
}


typedef struct __game_repeat_key_ctrl
{
	uint8 time_id;
	uint8 key;
}game_repeat_key_ctrl,*Pgame_repeat_key_ctrl;

game_repeat_key_ctrl key_ctrl;

//	SPP_APP_KEY_LEFT = 1,			//向左
//	SPP_APP_KEY_RIGHT,			//向右
//	SPP_APP_KEY_UP ,		 //向上
//	SPP_APP_KEY_DOWN,			//向下
//	SPP_APP_KEY_OK, 			//确定

void divoom_game_key_push_callback(T_S32 timerHandle, uint32 delay)
{
	if(NULL == pdivoom_disp_game_handle)
		return;
#if 0
	if(timerHandle != 0xFF)
	{
		if(key_ctrl.time_id == timerHandle && 0xff != key_ctrl.time_id)
		{			
			divoom_game_send_key_msg(key_ctrl.key);
			key_ctrl.time_id = anyka_to_divoom_set_system_timer(100,0,divoom_game_key_push_callback);
		}
	}
#endif
}

void divoom_game_key_push_fast_callback(T_S32 timerHandle, uint32 delay)
{
	if(NULL == pdivoom_disp_game_handle)
		return;
#if 0

	if(timerHandle != 0xFF)
	{
		if(key_ctrl.time_id == timerHandle && 0xff != key_ctrl.time_id)
		{			
			divoom_game_send_key_msg(key_ctrl.key);
			key_ctrl.time_id = anyka_to_divoom_set_system_timer(80,0,divoom_game_key_push_callback);
		}
	}
#endif

}


void divoom_game_key_first_push_callback(T_S32 timerHandle, uint32 delay)
{
	if(NULL == pdivoom_disp_game_handle)
		return;
#if 0

	if(timerHandle != 0xFF)
	{
		if(key_ctrl.time_id == timerHandle && 0xff != key_ctrl.time_id)
		{
			divoom_game_send_key_msg(key_ctrl.key);
			key_ctrl.time_id =  anyka_to_divoom_set_system_timer(100,0,divoom_game_key_push_callback);
		}
	}
#endif
}

void divoom_game_key_first_push_fast_callback(T_S32 timerHandle, uint32 delay)
{
	if(NULL == pdivoom_disp_game_handle)
		return;
#if 0
	if(timerHandle != 0xFF)
	{
		if(key_ctrl.time_id == timerHandle && 0xff != key_ctrl.time_id)
		{
			divoom_game_send_key_msg(key_ctrl.key);
			key_ctrl.time_id =  anyka_to_divoom_set_system_timer(80,0,divoom_game_key_push_fast_callback);
		}
	}
#endif
}


//按下按键
void divoom_game_push_app_key(uint8 key)
{
	uint16 time = 500;
	if(NULL == pdivoom_disp_game_handle)
		return;
#if 0
	key_ctrl.key = key;
    switch(pdivoom_disp_game_handle->game_id)
    {
        case DIVOOM_DISP_GAME_TETRIS:
        case DIVOOM_DISP_GAME_GOBANG:
        case DIVOOM_DISP_GAME_FROGGER:
        case DIVOOM_DISP_GAME_Supperzzle:
			time = 400;
            break;
		case DIVOOM_DISP_GAME_Galaxian:
		case DIVOOM_DISP_GAME_BALL:
		case DIVOOM_DISP_GAME_TANK:
		case DIVOOM_DISP_GAME_CAR:
		case DIVOOM_DISP_GAME_PLANE:
			time = 160;
			key_ctrl.time_id = anyka_to_divoom_set_system_timer(time,0,divoom_game_key_first_push_fast_callback);
			return;
        default:
//			time = 500;
            break;
    }
	key_ctrl.time_id = anyka_to_divoom_set_system_timer(time,0,divoom_game_key_first_push_callback);
#endif
}

//释放按键
void divoom_game_release_app_key(uint8 key)
{
	if(NULL == pdivoom_disp_game_handle)
		return;
	if(key == key_ctrl.key)	//如果抬起最后一下的按键
		key_ctrl.time_id = 0xff;
}


#endif

//数据０到９的显示像点为３*５
static const uint8 dight_small_disp_Info[11][5] =
{
    {7,5,5,5,7},
    {2,2,2,2,2},
    {7,4,7,1,7},
    {7,4,7,4,7},
    {5,5,7,4,4},
    {7,1,7,4,7},
    {7,1,7,5,7},
    {7,4,4,4,4},
    {7,5,7,5,7},
    {7,5,7,4,7},
    {0,0,0,0,0},
};

T_BOOL  divoom_disp_game_get_digit(uint8 x, uint8 y, uint8 user_data)
{
    return dight_small_disp_Info[user_data][y] & (1 << x);
}

/*
画数字，形参包含颜色信息
*/
void divoom_disp_game_draw_small_digit(uint8 start_x, uint8 start_y, uint8 user_data, Pdivoom_screen_color_info color, uint8 *disp_buf)
{
    uint8 i, j, off;

    for(i = 0, off = start_y * DIVOOM_GAME_SMALL_WIDTH; i < 5; i ++, off += DIVOOM_GAME_SMALL_WIDTH)
    {
        for(j = 0; j < 3; j ++)
        {
            if(divoom_disp_game_get_digit(j, i, user_data))
            {
                divoom_disp_ctrl_set_data(start_x + j + off, color, disp_buf);
            }
        }
    }
}


void divoom_disp_game_font_fill_area(int16 start_x, int16 start_y, uint8 size_x, uint8 size_y, uint8 *digit_info, uint8 *disp_buf, Pdivoom_screen_color_info color)
{
    uint16 i, j, tmp = 0;
    for(i = 0; i < size_y; i ++)
    {
        for(j = 0; j < size_x; j ++)
        {
            tmp = (*(digit_info + i)) & (1 << (size_x-j-1));            
            if(tmp)            
            {
                if(color != NULL)
                {
                    divoom_disp_ctrl_set_data_xy(start_x + j , start_y+ i, color, disp_buf);
                }
            }
        }
    }
}

void divoom_disp_game_font_fill_16area(int16 start_x, int16 start_y, uint8 size_x, uint8 size_y, uint16 *digit_info, uint8 *disp_buf, Pdivoom_screen_color_info color)
{
    uint16 i, j, tmp = 0;
    for(i = 0; i < size_y; i ++)
    {
        for(j = 0; j < size_x; j ++)
        {
            tmp = (*(digit_info + i)) & (1 << (size_x-j-1));            
            if(tmp)            
            {
                if(color != NULL)
                {
                    divoom_disp_ctrl_set_data_xy(start_x + j , start_y+ i, color, disp_buf);
                }
            }
        }
    }
}



