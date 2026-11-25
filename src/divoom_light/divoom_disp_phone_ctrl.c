#include "divoom_disp_com.h"

//实时画板、文字添加
typedef struct _divoom_disp_phone_ctrl
{
    uint8 pic_type;   //动画大小
    uint8 image_type;
    uint8 http_text_font_table[20];
    uint8 tmp_rgb_buf[SCREEN_BUF_SIZE_64];
	Pdivoom_http_download_image_font_data font_data;
    void *phttp_font_handle;
}divoom_disp_phone_ctrl, *Pdivoom_disp_phone_ctrl;

static Pdivoom_disp_phone_ctrl pdisp_phone_handle = NULL;

extern void divoom_play_com_convert_rgb_net_to_local(int width, uint8 *src_pic_buf, uint8 * rgb_buf);

/*
  Function:       divoom_disp_phone_ctrl_drawing_pad_16
  Description:	  16*16画板
  Input:          dot_info 点阵信息
  Return:         NONE
  Author:         wangft
  Data:           2020-12-18
*/
void divoom_disp_phone_ctrl_drawing_pad_16(Pdivoom_disp_ctrl_real_draw_ctrl        dot_info)
{
}

/*
  Function:       divoom_disp_phone_ctrl_drawing_pad_32
  Description:	  32*32画板
  Input:          dot_info 点阵信息
  Return:         NONE
  Author:         wangft
  Data:           2020-12-18
*/
void divoom_disp_phone_ctrl_drawing_pad_32(Pdivoom_disp_ctrl_real_draw_ctrl        dot_info)
{    
}

/*
  Function:       divoom_disp_phone_ctrl_drawing_pad_64
  Description:	  64*64画板
  Input:          dot_info 点阵信息
  Return:         NONE
  Author:         wangft
  Data:           2020-12-18
*/
void divoom_disp_phone_ctrl_drawing_pad_64(Pdivoom_disp_ctrl_real_draw_ctrl        dot_info)
{
}

/*
  Function:       divoom_disp_phone_ctrl_drawing_pad
  Description:	  画板
  Input:          dot_info 画板的数据
  Return:         NONE
  Author:         wangft
  Data:           2020-12-18
*/
void divoom_disp_phone_ctrl_drawing_pad(Pdivoom_disp_ctrl_real_draw_ctrl       dot_info)
{
}

/*
  Function:       divoom_disp_phone_ctrl_get_font_info
  Description:	  获取字体
  Input:          word_info 存放字体
  Return:         NONE
  Author:         wangft
  Data:           2020-12-18
*/
void divoom_disp_phone_ctrl_get_font_info(Pdivoom_image_word_font_info word_info)
{
}

/*
  Function:       divoom_disp_phone_ctrl_update_font
  Description:	  更新文字
  Input:          attr_info 文本信息
  Return:         NONE
  Author:         wangft
  Data:           2020-12-18
*/
void divoom_disp_phone_ctrl_update_font(Pdivoom_draw_set_info_json_image_info attr_info)
{
}

/*
  Function:       divoom_disp_phone_ctrl_set_pic
  Description:	  读取一张图进行显示
  Input:          image_id 保存到sd卡上的图片id
  Return:         NONE
  Author:         wangft
  Data:           2020-12-18
*/
void divoom_disp_phone_ctrl_set_pic(Pdivoom_http_download_image_file_info image_file_info)
{
}

/*
  Function:       divoom_disp_phone_ctrl_update
  Description:	  更新画板
  Input:          data 画板的数据
  Return:         NONE
  Author:         wangft
  Data:           2020-12-18
*/
void divoom_disp_phone_ctrl_update(void *data)
{
}


void divoom_disp_phone_ctrl_text_get_font_data(Pdivoom_image_word_font_info pword_info)
{
}

void divoom_disp_phone_ctrl_clear_text(void)
{
}

void divoom_disp_phone_ctrl_add_text(void *data)
{
}

/*
  Function:       divoom_disp_phone_ctrl_init
  Description:	  模块初始化
  Input:          dot_info 点阵信息
  Return:         NONE
  Author:         wangft
  Data:           2020-12-18
*/
void divoom_disp_phone_ctrl_init(void *data)
{
}

/*
  Function:       divoom_disp_phone_ctrl_close
  Description:	  模块退出
  Input:          NONE
  Return:         NONE
  Author:         wangft
  Data:           2020-12-18
*/
void divoom_disp_phone_ctrl_close(void)
{
}

