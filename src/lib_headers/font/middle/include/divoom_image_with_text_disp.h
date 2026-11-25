#ifndef _divoom_image_with_text_disp_
#define _divoom_image_with_text_disp_

#define DIVOOM_BACK_QUEUE_MAX	30

enum{
	DIVOOM_TEXT_DISP_ROW, //行
	DIVOOM_TEXT_DISP_COL,	 //列
};

typedef enum{
	DIVOOM_TEXT_EFFECT_LEFT, //左滚
	DIVOOM_TEXT_EFFECT_RIGHT,//右滚
	DIVOOM_TEXT_EFFECT_UP,		//上滚
	DIVOOM_TEXT_EFFECT_DOWN,//下滚
}DIVOOM_TEXT_EFFECT_TYPE;

typedef enum{
	DIVOOM_PIC_EFFECT_NORMAL, //普通模式
	DIVOOM_PIC_EFFECT_UP,		//上滚
	DIVOOM_PIC_EFFECT_DOWN,	//下滚
	DIVOOM_PIC_EFFECT_RIGHT,	//右滚
	DIVOOM_PIC_EFFECT_LEFT,		//左滚
	DIVOOM_PIC_EFFECT_SNOWING, //飘雪
	DIVOOM_PIC_EFFECT_PIC_SCROLL, //画卷
	DIVOOM_PIC_EFFECT_BLINK,	//闪烁
	DIVOOM_PIC_EFFECT_LASER, //激光
	DIVOOM_PIC_EFFECT_DIFFUSION, //缩放
	DIVOOM_PIC_EFFECT_FLY_IN, //飞入
	DIVOOM_PIC_EFFECT_BUBBING, //冒泡表盘
}DIVOOM_PIC_EFFECT_TYPE;

enum{
   	DIVOOM_IMAGE_START_TYPE,	//起始帧
	DIVOOM_IMAGE_PIC_TYPE,		//中间图片帧
    DIVOOM_IMAGE_END_TYPE,       //动画结束帧频，方便底层与上层应用进行沟通
    DIVOOM_IMAGE_ERR_TYPE,       //不存在的错误数据
};

typedef enum{
	DIVOOM_WORD_EFFECT_NORM, //正常的滚动显示
	DIVOOM_WORD_EFFECT_SINGLE_LINE_NOT_MOVE,//单行显示，占满整个文本框就固定不动
	DIVOOM_WORD_EFFECT_MULTI_LINE_MOVE, //多行滚动显示
	DIVOOM_WORD_EFFECT_ALIGN_CENTER, //居中对齐
	DIVOOM_WORD_EFFECT_ALIGN_LEFT, //左对齐
	DIVOOM_WORD_EFFECT_ALIGN_RIGHT,//右对齐
	DIVOOM_WORD_EFFECT_CALENDAR, //万年历显示
}DIVOOM_WORD_EFFCT_TYPE;

#define DIVOOM_WORD_MAX_NUM 1024

typedef struct _divoom_image_font_info
{
	T_U8 pic_width; //字体图片宽度
	T_U8 pic_high; //字体图片高度
	divoom_screen_color_info font_color; //字体颜色
	divoom_screen_color_info shade_color; //阴影颜色
	T_U8 pos_table[SCREEN_DOT_NUM_64]; //字体最大是16*16，字体用1表示 阴影用2表示, 不一定是16*16，根据图片的宽度和高度来存
}divoom_image_font_into, *Pdivoom_image_font_info;

typedef struct _divoom_image_word_font_info
{
	T_U16 unicode16; //当前文字的ＵＩＮＣＯＤＥ
	T_U8 font_id;//哪个字体框
	T_U8 font_flag; //0 点阵字体 1 图像字体
	T_U8 font_info[32]; //存储字体的16*16的点阵信息
	divoom_image_font_into image_font; //存储图像字体信息
}divoom_image_word_font_info, *Pdivoom_image_word_font_info;

typedef T_VOID (*PDIVOOM_IMAGE_FREE_CALLBACK)(T_VOID *data);
typedef T_VOID DIVOOM_IMAGE_DISP_CTRL_OVER_CALL_END(T_VOID *para, T_U8 gif_id);
typedef T_VOID (*DIVOOM_FUNC_GET_FONT_INFO_CALLBACL)(Pdivoom_image_word_font_info word_info);
typedef T_VOID (*DIVOOM_IMAGE_FONT_PLAY_OVER_CALLBACK)(T_U8 status);  //1 表示多屏显示完	2 表示单屏显示

struct _divoom_image_info;
typedef T_VOID (*DIVOOM_IMAGE_DECONDE_CALLBACK)(struct _divoom_image_info * image_info, T_U8 *rgb_buf); // rgb_buf存放解码之后的rgb数据

typedef struct _divoom_image_info //图片的队列信息
{
	T_U8 gif_id;	//0 表示背景图 局部图从1开始
	T_U8 frame_attr; //数据包类型【起始帧、中间图片帧、结束帧】
	DIVOOM_PIC_EFFECT_TYPE pic_effect; //图片效果
	T_U8 low_pos; //最低位置,图片最底边的位置
	T_U8 high_pos;//最高位置，图片最上边的位置
	T_U16 speed;	//动画速度
	T_U8 pos_x;	//坐标x起始位置
	T_U8 pos_y;  //坐标y起始位置
	T_U8 pic_width; //图片宽度
	T_U8 pic_high; //图片高度
	T_U8 layer; //该图存在哪个层级
	T_U8 fill_flag; // 1红旗边框要保持底色 0不保证保持底色
	T_U8 large_flag;//1 放大标志 2 根据自定义指针进行适配
	T_U8 image_width;//实际图片宽度
	T_U8 image_type;
	T_U32 image_size;
	DIVOOM_IMAGE_DECONDE_CALLBACK image_decode_callback;
	T_U8 *rgb_data; //图片裸数据
}divoom_image_info, *Pdivoom_image_info;

typedef struct _divoom_image_shade_info
{
	T_U8 pos_x;	//坐标x起始位置
	T_U8 pos_y;  //坐标y起始位置
	T_U8 pic_width; //图片宽度
	T_U8 pic_high; //图片高度
	T_U8 mod; //亮度衰减值
}divoom_image_shade_info, *Pdivoom_image_shade_info;

typedef struct _divoom_calendar_min_text_info
{
	T_S32 unicode_len; //要显示的字串的长度unicode_str有效字符的长度
    T_U16 unicode_str[10]; //要显示的字串
	divoom_screen_color_info color; //字体颜色
}divoom_calendar_min_text_info, *Pdivoom_calendar_min_text_info;

typedef struct _divoom_calendar_mid_text_info
{
	T_U8 min_text_num; //字符串个数
	divoom_calendar_min_text_info min_text[20]; //假设最多20行
}divoom_calendar_mid_text_info, *Pdivoom_calendar_mid_text_info;

typedef struct _divoom_calendar_main_text_info
{
	divoom_calendar_mid_text_info mid_frame[2]; //上下两层 
}divoom_calendar_main_text_info, *Pdivoom_text_calendar_info;

typedef struct _divoom_text_info //文本队列信息
{
    T_U8 x;        //字段的坐标位置
    T_U8 y;        //字段的坐标位置
    T_U8 size;     //字体大小
    T_U8 width;    //显示框的宽度
    T_U8 high;    //显示框的宽度
	DIVOOM_WORD_EFFCT_TYPE word_effct_type; //正常、单行固定、多行滚动、左对齐、右对齐
    DIVOOM_TEXT_EFFECT_TYPE font_effect; //上下左右滚动方式
    T_U8 font_type; //字体
    T_U16 speed;   //字体的速度
    T_U8 color[3];   //字体的颜色ＲＧＢ
    T_U8 image_font_color_flag; //1表示根据指定color设置图像字体显示 0表示默认
  	T_U8 font_id;	//是属于哪个字体框的
  	T_U8 font_flag; //0 点阵字体 1 图像字体 这边显示要特别处理
    T_U16 unicode_len; //要显示的字串的长度unicode_str有效字符的长度
    T_U16 unicode_str[DIVOOM_WORD_MAX_NUM]; //要显示的字串
	T_S8 space_dot_num; //每个字之间空几格
	T_U8 roll_flag; //该字符串是否滚动
	T_U8 draw_flag; //1画板 0 其他
	DIVOOM_IMAGE_FONT_PLAY_OVER_CALLBACK image_font_play_over_callback; //图像字体滚屏显示播放完毕回调
	divoom_calendar_main_text_info calendar_text; //万年历文字信息
    DIVOOM_FUNC_GET_FONT_INFO_CALLBACL get_font_info_callback; //获取文字回调函数
}divoom_text_info, *Pdivoom_text_info;

T_VOID divoom_image_with_text_disp_set_pic_effect(uint8 pic_effect);
void divoom_image_with_text_disp_large_pic_64(T_U8 *rgb_data, T_U8 *rgb_buf, T_U8 pic_size);
T_VOID divoom_image_with_text_disp_set_gif_speed(uint16 gif_speed);

/*
  Function:       divoom_image_with_text_disp_set_shade_image
  Description:	  设置图像阴影显示区
  Input:          flag 1打开阴影显示区 0关闭阴影显示区		 image_shade 要设置的显示区参数
  Return:         NONE
  Author:         wangft
  Data:           2020-12-18
*/
T_VOID divoom_image_with_text_disp_set_shade_image(T_U8 flag, Pdivoom_image_shade_info image_shade);

/*
  Function:       divoom_image_with_text_get_disp_rgb
  Description:	   直接更新rgb
  Input:            flag        1:要更新显示，０：不更新显示
  Return:         缓冲区地址
  Author:         aijun
  Data:           2020-11-06
*/
T_U8 *divoom_image_with_text_get_disp_rgb(T_U8 flag);

/*
  Function:       divoom_image_with_text_disp_del_image
  Description:	  删除局部图
  Input:          gif_id 为0xFF 就全部删除	否则就是删除某个局部动画
  Return:         NONE
  Author:         wangft
  Data:           2020-12-18
*/
T_VOID divoom_image_with_text_disp_del_image(T_U8 gif_id);

/*
  Function:       divoom_image_with_text_disp_get_back_queue
  Description:	  获取背景图队列还有多少可以写
  Input:          NONE
  Return:         NONE
  Author:         wangft
  Data:           2020-12-18
*/
T_U16 divoom_image_with_text_disp_get_back_queue(T_VOID);

/*
  Function:       divoom_image_with_text_disp_write_image
  Description:	  无论背景图还是局部图都用这个接口写
  Input:          要写的数据
  Return:         1 写成功 0 写失败
  Author:         wangft
  Data:           2020-12-18
*/
T_U8 divoom_image_with_text_disp_write_image(Pdivoom_image_info data);

/*
  Function:       divoom_image_with_text_disp_pause
  Description:	  控制播放
  Input:          1暂停播放 0恢复播放
  Return:         NONE
  Author:         wangft
  Data:           2020-12-18
*/
T_VOID divoom_image_with_text_disp_pause(T_U8 status);

/*
  Function:       divoom_image_with_text_disp_del_text
  Description:	  清除所有文本框
  Input:          NONE
  Return:         NONE
  Author:         wangft
  Data:           2020-12-18
*/
T_VOID divoom_image_with_text_disp_del_text(T_VOID);

/*
  Function:       divoom_image_with_text_disp_del_text
  Description:	  清除某个文本框
  Input:          font_id 指定哪个文本框
  Return:         NONE
  Author:         wangft
  Data:           2020-12-18
*/
T_VOID divoom_image_with_text_disp_del_text_frame(T_U8 font_id);

/*
  Function:       divoom_image_with_text_disp_set_back_image_flag
  Description:	  设置标志位
  Input:          1 文字可以直接显示 0还原标志位
  Return:         none
  Author:         wangft
  Data:           2020-12-18
*/
T_VOID divoom_image_with_text_disp_set_back_image_flag(T_U8 flag);

/*
  Function:       divoom_image_with_text_disp_write_text
  Description:	  往文本队列里面写文本数据
  Input:          要写的数据
  Return:         1 写成功 0 写失败
  Author:         wangft
  Data:           2020-12-18
*/
T_U8 divoom_image_with_text_disp_write_text(Pdivoom_text_info data);

/*
  Function:       divoom_image_with_text_disp_set_callback
  Description:    更新字体相关信息DIVOOM_DISP_GIF_UPDATE_GIF_CONTENT和DIVOOM_DISP_GIF_UPDATE_GIF_OFFSET,不处理
				  DIVOOM_DISP_GIF_UPDATE_SPPED,   //显示文字速度
				  DIVOOM_DISP_GIF_UPDATE_FONT_EFFECT,  //显示效果
				  DIVOOM_DISP_GIF_UPDATE_DIS_AREA,    //显示框
				  DIVOOM_DISP_GIF_UPDATE_FONT_SIZE,    //显示字体大小
				  DIVOOM_DISP_GIF_UPDATE_FONT_COLOR,    //显示字体颜色
				  DIVOOM_DISP_GIF_UPDATE_FONT_INFO,    //显示字体信息
				  DIVOOM_DISP_GIF_UPDATE_GIF_SPEED = 0x30,    //显示动画速度
				  DIVOOM_DISP_GIF_UPDATE_FONT_CLEAR,    //清除文字内容
				  DIVOOM_DISP_GIF_UPDATE_GIF_CONTENT,    //显示动画内容
				  DIVOOM_DISP_GIF_UPDATE_GIF_OFFSET,    //显示动画下载偏移
				  DIVOOM_DISP_GIF_UPDATE_NO_INFO,      //不需求更新
  Input:          pdisp_info
  Return:         none
  Data:           2020-12-18
*/
T_VOID divoom_image_with_text_disp_update_info(Pdivoom_disp_ctrl_gif_info pdisp_info);

/*
  Function:       divoom_image_with_text_disp_set_callback
  Description:    设置播放完整作品的回调函数
  				gif_id 针对哪个动画的回调
  Input:          pcallback
                     para
  Return:         none
  Data:           2019-11-21
*/
T_VOID divoom_image_with_text_disp_set_callback(T_U8 gif_id, DIVOOM_IMAGE_DISP_CTRL_OVER_CALL_END *pcallback, T_VOID *para);

/*
  Function:       divoom_image_with_text_disp_init
  Description:    图片带文字显示模块初始化
  Input:          pcallback 内存释放回调函数
  Return:         NONE
  Author:         wangft
  Data:           2020-12-18
*/
T_VOID divoom_image_with_text_disp_init(PDIVOOM_IMAGE_FREE_CALLBACK pcallback);

/*
  Function:       divoom_image_with_text_disp_exit
  Description:    模块退出
  Input:          NONE
  Return:         NONE
  Author:         wangft
  Data:           2020-12-18
*/
T_VOID divoom_image_with_text_disp_exit(T_VOID);

/*
  Function:       divoom_image_with_text_disp_del_back_image
  Description:	  删除背景图
  Input:		  NONE
  Return:         NONE
  Author:         wangft
  Data:           2020-12-18
*/
T_VOID divoom_image_with_text_disp_del_back_image(T_VOID);

/*
  Function:       divoom_image_with_text_ctrl_lock
  Description:	  对锁进行控制
  Input:          flag; 1: lock;0:unlock
  Return:         NONE
*/
T_VOID divoom_image_with_text_ctrl_lock(T_U8 flag);

/*
  Function:       divoom_image_with_text_disp_write_image_no_lock
  Description:	  不带锁的,无论背景图还是局部图都用这个接口写，用gif_type来区分是局部图还是背景图
  Input:          要写的数据
  Return:         1 写成功 0 写失败
  Author:         wangft
  Data:           2020-12-18
*/
T_U8 divoom_image_with_text_disp_write_image_no_lock(Pdivoom_image_info data);

#endif

