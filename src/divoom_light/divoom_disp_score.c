#include "divoom_disp_com.h"

/*
   ------->x
   |
   |
   |
   |↓y
*/

#if(SCREEN_WIDTH_32 == 32)
#define DIVOOM_DISP_HUNDRED_BLUE_Y			(3)
#define DIVOOM_DISP_HUNDRED_RED_Y			(3)

#define DIVOOM_DISP_TEN_BLUE_Y				(10)
#define DIVOOM_DISP_TEN_RED_Y				(10)

#define DIVOOM_DISP_UNIT_BLUE_Y				(17)
#define DIVOOM_DISP_UNIT_RED_Y				(17)

#define DIVOOM_DISP_BLUE_X					(3)
#define DIVOOM_DISP_RED_X					(18)

//#define	DIVOOM_DISP_FONT					(DIVOOM_8X14_DIGIT)
#define	DIVOOM_DISP_FONT					(DIVOOM_6X11_DIGIT)
#endif

typedef struct _divoom_disp_score_ctrl
{
    uint16 red_score;
    uint16 blue_score;
    uint8 disp_buf[SCREEN_BUF_SIZE_32];
}divoom_disp_score_ctrl, *Pdivoom_disp_score_ctrl;
Pdivoom_disp_score_ctrl divoom_disp_score_handle = NULL;

static const divoom_screen_color_info divoom_disp_score_blue_color = {0x0,0x00,0xff};
static const divoom_screen_color_info divoom_disp_score_red_color = {0xff,0x00,0x00};



void divoom_disp_score_refresh(void)
{
    uint16 red_score, blue_score;

    red_score = divoom_disp_score_handle->red_score;
    blue_score = divoom_disp_score_handle->blue_score;
    memset(divoom_disp_score_handle->disp_buf, 0, SCREEN_BUF_SIZE_32);

    divoom_disp_picture_get_rgb_data(DIVOOM_DISP_NEW_SCORE, divoom_disp_score_handle->disp_buf);
    
#if(SCREEN_WIDTH_32 == 16)
    divoom_disp_watch_digit(0, 0, 4 , 7,  blue_score/100, divoom_disp_score_handle->disp_buf,&divoom_disp_score_blue_color);
    blue_score = blue_score % 100;
    divoom_disp_watch_digit(5, 0, 4 , 7, blue_score/10, divoom_disp_score_handle->disp_buf,&divoom_disp_score_blue_color);
    divoom_disp_watch_digit(10, 0, 4 , 7, blue_score%10, divoom_disp_score_handle->disp_buf,&divoom_disp_score_blue_color);
    
    divoom_disp_watch_digit(0, 8, 4 , 7,red_score/100, divoom_disp_score_handle->disp_buf,&divoom_disp_score_red_color);
    red_score = red_score % 100;
    divoom_disp_watch_digit(5, 8, 4 , 7, red_score/10, divoom_disp_score_handle->disp_buf,&divoom_disp_score_red_color);
    divoom_disp_watch_digit(10, 8, 4 , 7, red_score%10, divoom_disp_score_handle->disp_buf,&divoom_disp_score_red_color);
#else
    divoom_disp_digit_fill_area(DIVOOM_DISP_HUNDRED_BLUE_Y, DIVOOM_DISP_BLUE_X, blue_score/100,
        DIVOOM_DISP_FONT, divoom_disp_score_handle->disp_buf, &divoom_disp_score_blue_color);
	blue_score %= 100;
    divoom_disp_digit_fill_area(DIVOOM_DISP_TEN_BLUE_Y, DIVOOM_DISP_BLUE_X, blue_score/10,
        DIVOOM_DISP_FONT, divoom_disp_score_handle->disp_buf, &divoom_disp_score_blue_color);
    divoom_disp_digit_fill_area(DIVOOM_DISP_UNIT_BLUE_Y, DIVOOM_DISP_BLUE_X, blue_score%10,
        DIVOOM_DISP_FONT, divoom_disp_score_handle->disp_buf, &divoom_disp_score_blue_color);

    divoom_disp_digit_fill_area(DIVOOM_DISP_HUNDRED_RED_Y, DIVOOM_DISP_RED_X, red_score/100,
        DIVOOM_DISP_FONT, divoom_disp_score_handle->disp_buf, &divoom_disp_score_red_color);
	red_score %= 100;
    divoom_disp_digit_fill_area(DIVOOM_DISP_TEN_RED_Y, DIVOOM_DISP_RED_X, red_score/10,
        DIVOOM_DISP_FONT, divoom_disp_score_handle->disp_buf, &divoom_disp_score_red_color);
    divoom_disp_digit_fill_area(DIVOOM_DISP_UNIT_RED_Y, DIVOOM_DISP_RED_X, red_score%10,
        DIVOOM_DISP_FONT, divoom_disp_score_handle->disp_buf, &divoom_disp_score_red_color);
#endif
    
    divoom_screen_disp_rgb32_data(divoom_disp_score_handle->disp_buf);
}

void divoom_disp_score_init(Pdivoom_disp_ctrl_score_ctrl score_info)
{
    uint8 red_score, blue_score;

	divoom_malloc_check_all_ram_in();
    red_score = score_info->red_score;
    blue_score = score_info->blue_score;
    if(divoom_disp_score_handle)
    {
        return;
    }
    divoom_disp_score_handle = divoom_com_malloc(sizeof(divoom_disp_score_ctrl), __func__, __LINE__);
    if(divoom_disp_score_handle == NULL)
    {
        divoom_print("%s:%d: it fails to malloc!\n",__FILE__, __LINE__);
        return ;
    }
    divoom_disp_score_handle->red_score = red_score;
    divoom_disp_score_handle->blue_score = blue_score;
    divoom_disp_score_refresh();
}


void divoom_disp_score_update(Pdivoom_disp_ctrl_score_ctrl score_info)
{
    if(divoom_disp_score_handle)
    {
        divoom_disp_score_handle->red_score = score_info->red_score;
        divoom_disp_score_handle->blue_score = score_info->blue_score;
        divoom_disp_score_refresh();
    }
}

void divoom_disp_score_play(uint8 status)
{
    if(status && divoom_disp_score_handle)
    {
        divoom_screen_disp_rgb32_data(divoom_disp_score_handle->disp_buf);
    }
}

void divoom_disp_score_close(void)
{
    divoom_com_free(divoom_disp_score_handle,__func__,__LINE__);
    divoom_disp_score_handle = NULL;
	divoom_malloc_check_all_ram_out(__func__);
}

