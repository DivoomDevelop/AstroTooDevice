#ifndef _divoom_disp_gif_h_
#define _divoom_disp_gif_h_
typedef enum
{
    DIVOOM_DISP_GIF_UPDATE_SPPED = 1,   //显示文字速度
    DIVOOM_DISP_GIF_UPDATE_FONT_EFFECT,  //显示文字效果
    DIVOOM_DISP_GIF_UPDATE_DIS_AREA,    //显示框
    DIVOOM_DISP_GIF_UPDATE_FONT_SIZE,    //显示字体大小
    DIVOOM_DISP_GIF_UPDATE_FONT_COLOR,    //显示字体颜色
    DIVOOM_DISP_GIF_UPDATE_FONT_INFO,    //显示字体信息
    DIVOOM_DISP_GIF_UPDATE_PIC_EFFECT,    //显示图片信息
    DIVOOM_DISP_GIF_UPDATE_GIF_SPEED = 0x30,    //显示动画速度
    DIVOOM_DISP_GIF_UPDATE_FONT_CLEAR,    //清除文字内容
    DIVOOM_DISP_GIF_UPDATE_GIF_CONTENT,    //显示动画内容
    DIVOOM_DISP_GIF_UPDATE_GIF_OFFSET,    //显示动画下载偏移
    DIVOOM_DISP_GIF_UPDATE_ALL_INFO,    //显示动画下载偏移
    DIVOOM_DISP_GIF_UPDATE_CLEAR_GIF,    //清除动画缓冲数据
    DIVOOM_DISP_GIF_UPDATE_NO_INFO,      //不需求更新
}DIVOOM_DISP_GIF_UPDATE_TYPE;
void divoom_disp_gif_init(void *data);
void divoom_disp_gif_update(void * data);
void divoom_disp_gif_close(void);
void divoom_disp_gif_add_text(void *data);
void divoom_disp_gif_clear_text(void);
#endif


