#ifndef _divoom_light_music_h_
#define _divoom_light_music_h_

#define DIVOOM_DISP_MUSIC_TMP_INDEX  254
enum
{
    DIVOOM_DISP_MUSIC_FM_MODE,
    DIVOOM_DISP_MUSIC_FFT_MODE,
    DIVOOM_DISP_MUSIC_RANDOM_MODE,
};

enum
{
    DIVOOM_DISP_MUSIC_CENTER_MODE,
    DIVOOM_DISP_MUSIC_EQ2_MODE,	
    DIVOOM_DISP_MUSIC_EQ4_MODE,
    DIVOOM_DISP_MUSIC_NORMAL_MODE,	
    DIVOOM_DISP_MUSIC_RANDOM_SQUARE2_MODE,      
    DIVOOM_DISP_MUSIC_EQ6_MODE,
    DIVOOM_DISP_MUSIC_EQ7_MODE,
    DIVOOM_DISP_MUSIC_EQ8_MODE,
    DIVOOM_DISP_MUSIC_RANDOM_SQUARE1_MODE,
    DIVOOM_DISP_MUSIC_EQ10_MODE,   
    DIVOOM_DISP_MUSIC_EQ11_MODE,
    DIVOOM_DISP_MUSIC_EQ12_MODE,     
    DIVOOM_DISP_MUSIC_END_MODE, 
};

/*********************************************************************
  Function:         divoom_light_music_init
  Description:     开始音乐模式
  Input:             disp_type    显示类型
                     fft_flag  
                       top    顶点的颜色
                       down   底点的颜色
  Author:           divoom
**********************************************************************/

void divoom_light_music_init(void *index,uint8 fft_flag);



//void divoom_light_music_update_freq(long int *music_freq);
uint8 divoom_light_music_update_mode(void *index);


/*********************************************************************
  Function:         divoom_light_music_close
  Description:     关闭
  Input:             NONE
  Author:           divoom
**********************************************************************/

void divoom_light_music_close(void *user_data);




void divoom_light_music_play(uint8 flag);


void divoom_light_music_get_disp_buf(void *user_data, T_U8 *disp_buf, int *image_width);

#endif

