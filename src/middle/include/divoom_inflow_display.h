#ifndef _DIVOOM_INFLOW_DISPLAY_
#define _DIVOOM_INFLOW_DISPLAY_

typedef struct _divoom_inflow_image_info //图片的队列信息
{
	T_U8 index;  //下标序号，针对多个信息流来进行编序
	T_U8 type;  //类型 【16、32、64】
	T_U8 speed;	//动画速度
	T_VOID *rgb_data; //图片裸数据
}divoom_inflow_image_info, *Pdivoom_inflow_image_info;

typedef struct _divoom_inflow_text_info //文本队列信息
{
	T_U8 index;    //下标序号，针对多个信息流来进行编序
    T_U8 size;     //字体大小
    T_U8 width;    //显示框的宽度
    T_U8 high;    //显示框的宽度
    T_U8 font_effect; //字体的效果
    T_U8 mode_flag;     //保留字段
    T_U16 speed;   //字体的速度
    T_U8 color[3];   //字体的颜色ＲＧＢ
  	T_U8 font_id;	//是属于哪个字体框的
    T_U16 unicode_len; //要显示的字串的长度unicode_str有效字符的长度
    T_U16 unicode_str[DIVOOM_WORD_MAX_NUM]; //要显示的字串
    T_U8 word_num;    //FontInfo里面有多少个有效的字,utf8_str 里面不相同的数目
	T_U8 *pfont_info; //文字点阵信息
}divoom_inflow_text_info, *Pdivoom_inflow_text_info;

/*
  Function:       divoom_inflow_display_del_small_image_queue
  Description:	  清空局部图队列
  Input:          data  NULL表示清空所有队列否则清空某一个队列
  Return:         NONE
  Author:         wangft
  Data:           2020-12-21
*/
T_VOID divoom_inflow_display_del_small_image_queue(T_VOID *data);

/*
  Function:       divoom_inflow_display_write_small_image_queue
  Description:	  往局部图队列里面写数据
  Input:          要写的数据
  Return:         1 写成功 0 写失败
  Author:         wangft
  Data:           2020-12-21
*/
T_U8 divoom_inflow_display_write_small_image_queue(T_VOID *data);

/*
  Function:       divoom_inflow_display_update_text_info_queue
  Description:	  往文本队列里面写文本数据
  Input:          要写的数据
  Return:         1 写成功 0 写失败
  Author:         wangft
  Data:           2020-12-21
*/
T_U8 divoom_inflow_display_update_text_info_queue(T_VOID *data);

/*
  Function:       divoom_inflow_display_write_text_info_queue
  Description:	  往文本队列里面写文本数据
  Input:          要写的数据
  Return:         1 写成功 0 写失败
  Author:         wangft
  Data:           2020-12-21
*/
T_U8 divoom_inflow_display_write_text_info_queue(T_VOID *data);

/*
  Function:       divoom_inflow_display_init
  Description:    模块初始化
  Input:          NONE
  Return:         NONE
  Author:         wangft
  Data:           2020-12-21
*/
T_VOID divoom_inflow_display_init(T_VOID);

/*
  Function:       divoom_inflow_display_exit
  Description:    模块退出
  Input:          NONE
  Return:         NONE
  Author:         wangft
  Data:           2020-12-21
*/
T_VOID divoom_inflow_display_exit(T_VOID);

#endif

