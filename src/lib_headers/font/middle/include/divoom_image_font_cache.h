#ifndef _divoom_image_font_cache_h_
#define _divoom_image_font_cache_h_

/*********************************************************************
  Function:        divoom_image_font_cache_init
  Description:     初始化字体缓冲模块
  Input:           none
  Return:           NONE
**********************************************************************/

void divoom_image_font_cache_init(void);

/*********************************************************************
  Function:        divoom_image_font_cache_clear_all
  Description:     删除所有缓冲结构 
  Input:           none
  Return:           NONE
**********************************************************************/

void divoom_image_font_cache_clear_all(void);


/*********************************************************************
  Function:        divoom_image_font_cache_clear
  Description:     释放表盘占用的所有资源
  Input:           clock_info
  Return:           NONE
**********************************************************************/

void divoom_image_font_cache_clear(Pdivoom_clock_id_info clock_info);

/*********************************************************************
  Function:        divoom_image_font_cache_get_font_info
  Description:     获取图像字体信息
  Input:           clock_info
                   font_id
                   disp_char
  Return:          返回字体
**********************************************************************/

void *divoom_image_font_cache_get_font_info(Pdivoom_clock_id_info clock_info, int font_id, char disp_char);

#endif

