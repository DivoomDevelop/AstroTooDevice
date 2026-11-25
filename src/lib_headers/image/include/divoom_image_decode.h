#ifndef _divoom_image_decode_h_
#define _divoom_image_decode_h_


typedef struct _divoom_image_color
{
	uint8 r;
	uint8 g;
	uint8 b;
}divoom_image_color, *Pdivoom_image_color;

typedef enum{
	DIVOOM_IMAGE_IFRAME_16, //16*16的Ｉ帧
	DIVOOM_IMAGE_FFRAME_16, //16*16的F帧
	DIVOOM_IMAGE_RFRAME_32, //32*32的 裸数据
	DIVOOM_IMAGE_IFRAME_32, //32*32的Ｉ帧
	DIVOOM_IMAGE_FFRAME_32, //32*32的F帧
	DIVOOM_IMAGE_WORD_32,  //文字相关信息,文字内容为文字长度二字节+utf8文字内容+文字数目+(文字的UNICODE+32字节文字字体信息)*文字数目
	DIVOOM_IMAGE_PIC_EFFECT_32,  //图片效果帧
	DIVOOM_IMAGE_EMPTY_FRAME,   //一个没有数据的空帧，一般是用来
	DIVOOM_IMAGE_FRAME_END_TYPE,
	DIVOOM_IMAGE_WORD_64, //文字相关信息
	DIVOOM_IMAGE_PIC_EFFECT_64, //图片效果帧
	DIVOOM_IMAGE_RFRAME_64, //64*64的裸数据
	DIVOOM_IMAGE_IFRAME_64, //64*64的I帧
	DIVOOM_IMAGE_FFRAME_64, //64*64的F帧
	DIVOOM_IMAGE_TEXT_BACKG_RFRAME_64,//文字框背景图64*64裸数据
	DIVOOM_IMAGE_TEXT_BACKG_IFRAME_64,//文字框背景图64*64I帧
	DIVOOM_IMAGE_TEXT_BACKG_FFRAME_64,//文字框背景图64*64F帧
	DIVOOM_IMAGE_RFRAME_128, //128*128的裸数据
	DIVOOM_IMAGE_OLD_IFRAME_128, //128*128的I帧
	DIVOOM_IMAGE_FFRAME_128, //128*128的F帧
	DIVOOM_IMAGE_IFRAME_128, //128*128的I帧
	DIVOOM_IMAGE_FIX_FRAME_128, //128*128的混合编码帧
}DIVOOM_IMAGE_FRAME_TYPE;
    

#define DIVOOM_IMAGE_COLOR_MAX_NUMBER  127

#define DIVOOM_IMAGE_COLOR_SIZE  3

#define DIVOOM_IMAGE_SMALL_HEIGHT  16
#define DIVOOM_IMAGE_SMALL_WIDTH   16
#define DIVOOM_IMAGE_SMALL_MAX_DOT  (DIVOOM_IMAGE_SMALL_HEIGHT *DIVOOM_IMAGE_SMALL_WIDTH)
#define DIVOOM_IMAGE_SMALL_RGB_SIZE  (DIVOOM_IMAGE_SMALL_MAX_DOT * DIVOOM_IMAGE_COLOR_SIZE)

#define DIVOOM_IMAGE_HEIGHT  32
#define DIVOOM_IMAGE_WIDTH   32
#define DIVOOM_IMAGE_MAX_DOT  (DIVOOM_IMAGE_HEIGHT *DIVOOM_IMAGE_WIDTH)
#define DIVOOM_IMAGE_RGB_SIZE  (DIVOOM_IMAGE_MAX_DOT * DIVOOM_IMAGE_COLOR_SIZE)

#define DIVOOM_IMAGE_HEIGHT_64 64
#define DIVOOM_IMAGE_WIDTH_64	64
#define DIVOOM_IMAGE_MAX_DOT_64 (DIVOOM_IMAGE_HEIGHT_64 * DIVOOM_IMAGE_WIDTH_64)
#define DIVOOM_IMAGE_RGB_SIZE_64 (DIVOOM_IMAGE_MAX_DOT_64 * DIVOOM_IMAGE_COLOR_SIZE)

#define DIVOOM_IMAGE_DECODE_RGB_SIZE_64 (DIVOOM_IMAGE_MAX_DOT_64 * 3)

#define DIVOOM_IMAGE_HEIGHT_128 128
#define DIVOOM_IMAGE_WIDTH_128	128
#define DIVOOM_IMAGE_MAX_DOT_128 (DIVOOM_IMAGE_HEIGHT_128 * DIVOOM_IMAGE_WIDTH_128)
#define DIVOOM_IMAGE_RGB_SIZE_128 (DIVOOM_IMAGE_MAX_DOT_128 * DIVOOM_IMAGE_COLOR_SIZE)

#define DIVOOM_IMAGE_DECODE_RGB_SIZE_128 (DIVOOM_IMAGE_MAX_DOT_128 * 3)

#define DIVOOM_IMAGE_ENCODE_FLAG  0Xaa
#define DIVOOM_IMAGE_ENCODE_FLAG_OFF  0
#define DIVOOM_IMAGE_ENCODE_LEN_OFF   1
#define DIVOOM_IMAGE_ENCODE_TIME_OFF  3
#define DIVOOM_IMAGE_ENCODE_TYPE_OFF  5
#define DIVOOM_IMAGE_ENCODE_COLOR_NUM_OFF   6
#define DIVOOM_IMAGE_ENCODE_COLOR_DATA_OFF   7
#define DIVOOM_IMAGE_ENCODE_COLOR_DATA_32_OFF     8//10
#define DIVOOM_IMAGE_ENCODE_RFRAME_SIZE  (DIVOOM_IMAGE_RGB_SIZE + DIVOOM_IMAGE_ENCODE_COLOR_DATA_32_OFF)   //一帧裸编码的长度
#define DIVOOM_IMAGE_ENCODE_MAX_SIZE  (DIVOOM_IMAGE_RGB_SIZE + DIVOOM_IMAGE_ENCODE_COLOR_NUM_OFF + 10)   //一帧编码的最大长度


#define DIVOOM_IMAGE_ENCODE_COLOR_DATA_64_OFF     8 //10
#define DIVOOM_IMAGE_ENCODE_RFRAME_64_SIZE  (DIVOOM_IMAGE_RGB_SIZE_64 + DIVOOM_IMAGE_ENCODE_COLOR_DATA_64_OFF)   //一帧裸编码的长度
#define DIVOOM_IMAGE_ENCODE_MAX_64_SIZE  (DIVOOM_IMAGE_MAX_DOT_64*7/8 + DIVOOM_IMAGE_ENCODE_COLOR_NUM_OFF + 10 + DIVOOM_IMAGE_COLOR_MAX_NUMBER *3)   //一帧编码的最大长度

#define DIVOOM_IMAGE_ENCODE_COLOR_DATA_128_OFF     8 //10
#define DIVOOM_IMAGE_ENCODE_RFRAME_128_SIZE  (DIVOOM_IMAGE_RGB_SIZE_128 + DIVOOM_IMAGE_ENCODE_COLOR_DATA_128_OFF)   //一帧裸编码的长度
#define DIVOOM_IMAGE_ENCODE_MAX_128_SIZE  (DIVOOM_IMAGE_MAX_DOT_128 + DIVOOM_IMAGE_ENCODE_COLOR_NUM_OFF + 10 + DIVOOM_IMAGE_COLOR_MAX_NUMBER *3)   //一帧编码的最大长度

#define DIVOOM_IMAGE_ENCODE_REIVEW_PIC_NUM  5
#define DIVOOM_IMAGE_ENCODE_REIVEW_MAX_SIZE  (4 * 1024)
#define DIVOOM_IMAGE_ENCODE_REIVEW_FIRST_MAX_SIZE  (1200)



typedef enum{
    DIVOOM_IMAGE_ENCODE_PIC_TYPE,       //正常图片数据
  	DIVOOM_IMAGE_ENCODE_WORD_PIC_TYPE, //文本框动画数据
    DIVOOM_IMAGE_ENCODE_WORD_TYPE,      //文字数据
    DIVOOM_IMAGE_ENCODE_REVIEW_TYPE,    //图片预览数据
    DIVOOM_IMAGE_ENCODE_PIC_EFEECT_TYPE,    //图片效果预览数据
    DIVOOM_IMAGE_ENCODE_END_TYPE,       //动画结束帧频，方便底层与上层应用进行沟通
    DIVOOM_IMAGE_ENCODE_ERR_TYPE,       //不存在的错误数据

}DIVOOM_IMAGE_ENCODE_FRAME_TYPE;
/**
* NAME         divoom_image_encode_creat_handle
* @BRIEF	创建编码句柄
* @PARAM	encode_out_buf      这个大小必须是DIVOOM_IMAGE_ENCODE_MAX_SIZE，也可为空由内部创建
                    public_buf             这个ＢＵＦ,必须是8k，也可为空由内部创建
* @RETURN	返回编码句柄
* @RETVAL
*/

void *divoom_image_encode_creat_handle(uint8 * public_buf);

/**
* NAME         divoom_image_encode_add_color_table
* @BRIEF	添加颜色表
* @PARAM	handle      
                    color_info  颜色值的信息RGBRGBRGBRGB
                    color_num  颜色种类RGB为一个
* @RETURN	返回编码句柄
* @RETVAL
*/

void divoom_image_encode_add_color_table(void *handle, uint8 *color_info, int color_num);


/**
* NAME         divoom_image_encode_reset_handle
* @BRIEF	将编码强制为I帧
* @PARAM	无
* @RETURN	返回编码句柄
* @RETVAL
*/

void divoom_image_encode_reset_handle(void *handle);

/**
* NAME         divoom_image_encode_empty_frame
* @BRIEF	输出一个空帧，表示完整的播放已经结束
* @PARAM	encode_buf
                    
* @RETURN	返回编码字节数
* @RETVAL
*/

uint16 divoom_image_encode_empty_frame(uint8 *encode_buf);

/**
* NAME         divoom_image_encode_encode_mul_pic
* @BRIEF	对一帧数据进行编码，只接受n*n的图片的RGB数据，返回的数据内存将是内部的缓冲，不需要释放
* @PARAM	handle          创建的句柄
                    rgb_data       图片的RGB数据， 如果１６的图，一张图有256个点，这个缓冲区大小为256*3*pic_num
                    pic_num        图片张数
                    out_len         输出数据长度
                    time             这个是动画播放时间
                    image_size    图片大小，目前只支持１６,３２,64
                    review_flag    是否生成预览图，在发送动画时要设置这个标记
                    pword_info     文字相关信息，没有将设置为AK_NULL

* @RETURN	返回编码句柄,这个返回的内存，使用后需求释放
* @RETVAL
*/
#define DIVOOM_IMAGE_ENCODE_WORD_MAX_NUM 128
typedef struct _DIVOOM_IMAGE_ENCODE_WORD_FONT
{
    uint16 unicode16;       //当前文字的ＵＩＮＣＯＤＥ
    uint8  font_info[32];   //字体的16*16的点阵信息
    
}DIVOOM_IMAGE_ENCODE_WORD_FONT, *PDIVOOM_IMAGE_ENCODE_WORD_FONT;

typedef struct _DIVOOM_IMAGE_ENCODE_WORD_INFO
{
    uint8 x;        //字段的坐标位置
    uint8 y;        //字段的坐标位置
    uint8 size;     //字体大小
    uint8 width;    //显示框的宽度
    uint8 high;    //显示框的宽度
	uint8 frame_type; //框的样式
    uint8 font_effect; //字体的效果
 	uint8 font_back_gif; //文本动画效果 0 没有背景动画	1 有背景动画
    uint8 reserve;     //保留字段
    uint16 speed;   //字体的速度
    uint8 color[3];   //字体的颜色ＲＧＢ
  	uint8 font_id;	//是属于哪个字体框的
    uint8 color_type;   //字体颜色样式 DIVOOM_WORD_ONE_COLOR 或者 DIVOOM_WORD_MORE_COLOR
    uint16 unicode_len; //要显示的字串的长度unicode_str有效字符的长度
    uint16 unicode_str[DIVOOM_IMAGE_ENCODE_WORD_MAX_NUM]; //要显示的字串
    uint8  word_num;    // FontInfo里面有多少个有效的字,utf8_str 里面不相同的数目
    DIVOOM_IMAGE_ENCODE_WORD_FONT font_info[DIVOOM_IMAGE_ENCODE_WORD_MAX_NUM];   //只存放utf8_str不同字的字体信息，减少数据量
    
}DIVOOM_IMAGE_ENCODE_WORD_INFO, *PDIVOOM_IMAGE_ENCODE_WORD_INFO;


typedef struct _DIVOOM_IMAGE_ENCODE_WORD_APP_DATA
{
    uint8 x;        //字段的坐标位置
    uint8 y;        //字段的坐标位置
    uint8 size;     //字体大小
    uint8 width;    //显示框的宽度
    uint8 high;    //显示框的宽度
	uint8 frame_type; //框的样式
    uint8 font_effect; //字体的效果
 	uint8 font_back_gif; //文本动画效果 0 没有背景动画	1 有背景动画
    uint8 mode_flag;     //保留字段
    uint16 speed;   //字体的速度
    uint8 color[3];   //字体的颜色ＲＧＢ
  	uint8 font_id;	//是属于哪个字体框的
    uint8 color_type;   //字体颜色样式 DIVOOM_WORD_ONE_COLOR 或者 DIVOOM_WORD_MORE_COLOR
    uint16 unicode_len; //要显示的字串的长度unicode_str有效字符的长度
}DIVOOM_IMAGE_ENCODE_WORD_APP_DATA, *PDIVOOM_IMAGE_ENCODE_WORD_APP_DATA;

typedef struct _DIVOOM_IMAGE_ENCODE_WORD_INFO2
{
    DIVOOM_IMAGE_ENCODE_WORD_APP_DATA base_info;
    uint16 unicode_len; //要显示的字串的长度unicode_str有效字符的长度
    uint16 unicode_str[DIVOOM_IMAGE_ENCODE_WORD_MAX_NUM]; //要显示的字串
    uint8  word_num;    // FontInfo里面有多少个有效的字,utf8_str 里面不相同的数目
	uint8 *pfont_info; //文字点阵信息
}DIVOOM_IMAGE_ENCODE_WORD_INFO2, *PDIVOOM_IMAGE_ENCODE_WORD_INFO2;
    
uint8 *divoom_image_encode_encode_mul_pic(uint8 *rgb_data, int pic_num, int *out_len, uint16 time, uint8 image_size, uint8 pic_effect, PDIVOOM_IMAGE_ENCODE_WORD_INFO pword_info, uint8 word_num);

/**
* NAME         divoom_image_encode_frame_word
                    仅仅编译一帧文字帧
* @PARAM	buf          
                    pword_info         文字的相关信息
                    width              屏的宽度,32,64
* @RETURN	返回帧长度
* @RETVAL
*/

uint16 divoom_image_encode_frame_word(uint8 *encode_buf, PDIVOOM_IMAGE_ENCODE_WORD_INFO pword_info, int width);


/**
* NAME         divoom_image_encode_encode_pic,如果多张图片，将会是将单张图片数据打包再直接连起来
* @BRIEF	对一帧数据进行编码，只接受16*16的图片的RGB数据，返回的数据内存将是内部的缓冲，不需要释放
* @PARAM	handle          创建的句柄
rgb_data       图片的RGB数据，256个点 一张图
pic_num        图片张数
out_len         输出数据长度
image_size  图片大小，目前只支持１６,３２

* @RETURN	返回编码句柄,输出的指针不能释放，是内部申请的
* @RETVAL
*/

uint8 *divoom_image_encode_encode_pic(void *handle, uint8 *rgb_data, int pic_num, int *out_len, uint16 time, uint8 image_size);



/**
* NAME         divoom_image_encode_encode_mul_pic_28dots
* @BRIEF	对一帧数据进行编码，只接受28个点的图片的RGB数据，返回的数据内存将是内部的缓冲，不需要释放
* @PARAM	
                    rgb_data       图片的RGB数据， 一张图有28个点，这个缓冲区大小为28*3*pic_num
                    pic_num        图片张数
                    out_len         输出数据长度
                    time             这个是动画播放时间
* @RETURN	返回编码数据,这个返回的内存，使用后需求释放
* @RETVAL
*/

uint8 *divoom_image_encode_encode_mul_pic_28dots(uint8 *rgb_data, int pic_num, int *out_len, uint16 time);


/**
* NAME         divoom_image_encode_destoy
* @BRIEF	对一帧数据进行编码，只接受16*16的图片的RGB数据
* @PARAM	handle          创建的句柄
rgb_data       图片的RGB数据，256个点 一张图
pic_num        图片张数
out_len         输出数据长度
* @RETURN	NONE
* @RETVAL
*/

void divoom_image_encode_destoy(void *handle);


/**
* NAME         divoom_image_decode_creat_handle
* @BRIEF	创建解码句柄
* @PARAM
* @RETURN	返回编码句柄
* @RETVAL
*/

void *divoom_image_decode_creat_handle(void);


/**
* NAME         divoom_image_decode_get_pic_data
* @BRIEF	这个接口主 要是配合divoom_image_decode_add_data进行使用，将不断的解码缓冲区中的数据，从第一张到最后一张，如果
没有可解码的数据，将返回0；
* @PARAM	handle          创建的句柄；
* @RETURN	 返回解码的RGB数据
* @RETVAL
*/

uint8 *divoom_image_decode_get_pic_data(void *handle, uint16 *pic_len, uint16 *time);



/**
* NAME         divoom_image_decode_decode_all
* @BRIEF	将
* @PARAM	handle          创建的句柄
                    encode_data       图片的编码后的数据
                    encode_len         数据的长度
                    rgb_data            解码后的RGB数据，这个保证缓冲区大小为256*3*24
                    time                  时间间隔
                    image_size    图片大小，目前只支持１６,３２


* @RETURN	解码的图片张数
* @RETVAL
*/

int divoom_image_decode_decode_all(uint8 *encode_data, uint32 encode_len,uint8 *rgb_data, uint16 *time, uint8 image_size, PDIVOOM_IMAGE_ENCODE_WORD_INFO pword_info);



/**
* NAME         divoom_image_decode_decode_one
* @BRIEF	这个接口只对电影播放有限，每次只解码encode_data里面第一张数据，不对数据进行缓存
* @PARAM	handle          创建的句柄
encode_data       图片的编码后的数据
encode_off         使用缓存器里数据偏移，不关心可以为NULL
rgb_data            解码后的RGB数据
* @RETURN	解码是否成功
* @RETVAL
*/

int divoom_image_decode_decode_one(void *handle, uint8 *encode_data, uint8 *rgb_data, int *encode_off, uint16 *time);

/**
* NAME         divoom_image_decode_add_data
* @BRIEF	向解码缓冲区中增加数据，此应用场景为动画方式，将所有数据一次加载，为简化设计，
此函数如果多次调用，encode_data必须是能够完整解码的数据，如果数据不足，将直接抛弃
* @PARAM	handle          创建的句柄
encode_data       图片的编码后的数据,必须是完成的帧数据
len                编码缓冲区的长度、
cache_flag      缓冲区方式，如果将1,这边会申请内存，再在下一次调用时再释放
如果为O将不申请内存，直接使用encode_data；
* @RETURN	 这个缓冲区可以解码出多少张图片
* @RETVAL
*/

int divoom_image_decode_add_data(void *handle, uint8 *encode_data, int len, int cache_flag);


/**
* NAME         divoom_image_decode_decode_pic
* @BRIEF	这个接口主 要是配合divoom_image_decode_add_data进行使用，将不断的解码缓冲区中的数据，从第一张到最后一张，如果
没有可解码的数据，将返回0；
* @PARAM	handle          创建的句柄；
* @RETURN	 返回解码的RGB数据
* @RETVAL
*/

uint8 divoom_image_decode_decode_pic(void *handle, uint8 *rgb_data, uint16 *time);


/**
* NAME         divoom_image_decode_reset
* @BRIEF	配合divoom_image_decode_decode_pic这个使用，如果已经取完所有数据，想从头开始取，将调用这个接口，内部偏移将会重置
* @PARAM	handle          创建的句柄；
* @RETURN	 返回解码的RGB数据
* @RETVAL
*/

void divoom_image_decode_reset(void *handle);


/**
* NAME         divoom_image_decode_destoy
* @BRIEF	释放所有申请的资源
* @PARAM	handle          创建的句柄；
* @RETURN	 NONE
* @RETVAL
*/

void divoom_image_decode_destoy(void *handle);





/**
* NAME         divoom_image_decode_get_pic_data
* @BRIEF	这个接口主 要是配合divoom_image_decode_add_data进行使用，将不断的解码缓冲区中的数据，从第一张到最后一张，如果
没有可解码的数据，将返回0；
* @PARAM	handle          创建的句柄；
* @RETURN	 返回解码的RGB数据
* @RETVAL
*/

uint8 *divoom_image_decode_get_pic_data(void *handle, uint16 *pic_len, uint16 *time);



/**
* NAME         divoom_image_encode_get_buf
* @BRIEF	将编码强制为I帧
* @PARAM	无
* @RETURN	返回编码句柄
* @RETVAL
*/

uint8 *divoom_image_encode_get_buf(void *handle);



/**
* NAME         divoom_image_decode_get_time_info
* @BRIEF	从一帧数据中获取时间信息
* @PARAM	pic_data          帧数据
* @RETURN	 返回帧时间间隔
* @RETVAL
*/

uint16 divoom_image_decode_get_time_info(uint8 *pic_data);

/**
* NAME         divoom_image_decode_get_pic_len
* @BRIEF	从一帧数据中获取长度信息
* @PARAM	pic_data          帧数据
* @RETURN	 返回帧长度
* @RETVAL
*/

uint16 divoom_image_decode_get_pic_len(uint8 *pic_data);

/**
* NAME         divoom_image_decode_check_image
* @BRIEF	从一帧数据检测是否是DEMO的编码数据
* @PARAM	pic_data          帧数据
* @RETURN	 返回帧长度
* @RETVAL
*/

uint8 divoom_image_decode_check_image(uint8 *pic_data);

/**
* NAME         divoom_image_decode_check_image
* @BRIEF	检查是否是图片数据
* @PARAM	pic_data          帧数据
* @RETURN	 返回帧长度
* @RETVAL
*/

uint8 divoom_image_decode_check_image(uint8 *pic_data);


/**
* NAME         divoom_image_decode_update_time_info
* @BRIEF	从一帧数据中 设置时间信息
* @PARAM	pic_data          帧数据
* @RETURN	 返回帧时间间隔
* @RETVAL
*/

void divoom_image_decode_update_time_info(uint8 *pic_data, uint16 time);


void divoom_image_decode_update_buf(void *handle, int tol_len);

void divoom_image_decode_update_data(void *handle, uint8 *encode_data, int len);

uint8 divoom_image_decode_check_decode(void *handle);

void divoom_image_encode_update_buf(void *handle, uint8 *buf);

void divoom_image_encode_get_data(uint8 *disp_buf, uint16 off, Pdivoom_image_color color);

/**
* NAME         divoom_image_decode_get_image_info
* @BRIEF	从一帧数据中获取图片相关信息
* @PARAM	pic_data          帧数据
review_flag      图片数据类型
* @RETURN	 返回图片的大小
* @RETVAL
*/

uint16 divoom_image_decode_get_image_info(uint8 *pic_data, uint8 *frame_attr);

/**
* NAME         divoom_image_decode_get_word_info
* @BRIEF	从一帧数据中获取显示文字的相关信息
* @PARAM	pic_data          帧数据
                    disp_word_info          显示的文字字串
                    disp_word_num       ?显示文字缓冲区的大小
                    word_font_info          显示文字的字体信息
                    word_font_num         显示文字的字体的个数
* @RETURN	 返回成功与否
* @RETVAL
*/

uint16 divoom_image_decode_get_word_info(uint8 *pic_data, PDIVOOM_IMAGE_ENCODE_WORD_INFO pword_info);

uint16 divoom_image_decode_get_word_info2(uint8 *pic_data, PDIVOOM_IMAGE_ENCODE_WORD_INFO2 pword_info, PDIVOOM_IMAGE_ENCODE_WORD_APP_DATA psmall_info);




/**
* NAME         divoom_image_decode_Iframe
* @BRIEF	对ＩＦＲＡＭＥ进行解码,这个方式并不创建句柄,这个只给APP使用
* @PARAM	encode_buf          创建的句柄；
                    rgb_buf               输出的ＲＧＢ缓冲区
                    width_flag           图像宽度
* @RETURN	 NONE
* @RETVAL
*/
uint8 divoom_image_decode_Iframe(uint8 *deocde_data, uint8 *rgb_buf, uint8 width_flag);

/**
* NAME         divoom_image_decode_Iframe
* @BRIEF	对ＩＦＲＡＭＥ进行解码,这个方式并不创建句柄， 这个给设备使用
* @PARAM	encode_buf          创建的句柄；
                    rgb_buf               输出的ＲＧＢ缓冲区
                    width_flag           图像宽度
* @RETURN	 NONE
* @RETVAL
*/
uint8 divoom_image_decode_Iframe_in(uint8 *deocde_data, uint8 *rgb_buf, uint8 width_flag);

/**
* NAME         divoom_image_decode_decode_handle
* @BRIEF	对数据进行解码，提供一帧数据
* @PARAM	handle          创建的句柄
encode_data       图片的编码后的数据
rgb_data            解码后的RGB数据，这个保证缓冲区大小为256*3*24
time                  时间间隔
image_size    图片大小，目前只支持１６,３２


* @RETURN	解码的图片张数
* @RETVAL
*/

int divoom_image_decode_decode_handle(void *decode_handle,uint8 *encode_data, uint8 *rgb_data, uint16 *time, uint8 image_size);

/**
* NAME         divoom_image_encode_frame_RFrame_head
                    仅仅编译裸数据帧的头部
* @PARAM	encode_buf          
            
* @RETURN	返回帧长度
* @RETVAL
*/

uint16 divoom_image_encode_frame_RFrame_head(uint8 *encode_buf);


/**
* NAME         divoom_image_encode_get_frame_data
               得到对应帧数据
* @PARAM	encode_buf 
            buf_len
            frame_type
            frame_len
            
* @RETURN	返回帧数据
* @RETVAL
*/

uint8 *divoom_image_encode_get_frame_data(uint8 *encode_buf, uint16 buf_len, uint8 frame_type,uint16 *frame_len);

/**
* NAME         divoom_image_encode_encode_mul_pic_28dots
* @BRIEF	对一帧数据进行编码，只接受28个点的图片的RGB数据，返回的数据内存将是内部的缓冲，不需要释放
* @PARAM	
                    rgb_data       图片的RGB数据， 一张图有28个点，这个缓冲区大小为28*3*pic_num
                    pic_num        图片张数
                    out_len         输出数据长度
                    time             这个是动画播放时间
* @RETURN	返回编码数据,这个返回的内存，使用后需求释放
* @RETVAL
*/

uint8 *divoom_image_encode_encode_mul_pic_28dots(uint8 *rgb_data, int pic_num, int *out_len, uint16 time);

/**
* NAME         divoom_image_encode_create_full_screen
* @BRIEF	得到一个全屏一个颜色的数据
* @PARAM	data
            color
* @RETURN	数据长度
* @RETVAL
*/

uint16  divoom_image_encode_create_full_screen(uint8 *data, uint8 *color);



typedef struct _divoom_image_decode_frame_item
{
    uint8  image_width;  //图像的宽度
    uint8  word_flag;    //图像是否有文字，有文字
    uint8  pic_num;      //图像图片的张数
    uint16  time;        //动画时间
    uint8  pic_buf[60][DIVOOM_IMAGE_RGB_SIZE]; //图片数据，如果在３２*３２的，图片将分成四张小图，顺序排放
    DIVOOM_IMAGE_ENCODE_WORD_INFO word_info;    //文字相关信息
}divoom_image_decode_frame_item, *Pdivoom_image_decode_frame_item;


/**
* NAME         divoom_image_decode_get_all_frame_init
* @BRIEF	   初始化得到压缩数据的全部作品信息的句柄
* @PARAM	   encode_buf          编码后的数据缓冲,这个缓冲区，要保持到divoom_image_decode_get_all_frame_exit之后
               data_len            缓冲区的长度
* @RETURN	   返回工作句柄
* @RETVAL
*/

void *divoom_image_decode_get_all_frame_init(uint8 *deocde_data, uint32 data_len);

/**
* NAME         divoom_image_decode_get_all_frame_next
* @BRIEF	   得到一个完整的作品数据
* @PARAM	   handle              初始化时的句柄
               
* @RETURN	   返回图像作品的数据，使用后不用释放，由内部自动管理
* @RETVAL
*/

Pdivoom_image_decode_frame_item  divoom_image_decode_get_all_frame_next(void *handle);


/**
* NAME         divoom_image_decode_get_all_frame_exit
* @BRIEF	   释放句柄资源
* @PARAM	   handle              初始化时的句柄
               
* @RETURN	   返回图像作品的数据，
* @RETVAL
*/

void  divoom_image_decode_get_all_frame_exit(void *handle);

/**
* NAME         divoom_image_decode_check_pic_iframe
* @BRIEF	检查数据是否是I帧数据
* @PARAM	buf    缓冲区
* @RETURN	 1:是0不是
* @RETVAL
*/

uint8 divoom_image_decode_check_pic_iframe(uint8 *buf);

/**
* NAME         divoom_image_decode_get_effect_type
* @BRIEF	从一帧数据中获取图片效果
* @PARAM	pic_data          帧数据
* @RETURN	图片效果
* @RETVAL
*/

uint8 divoom_image_decode_get_effect_type(uint8 *pic_data);

/**
* NAME      divoom_image_encode_set_data_64
* @BRIEF	把24bit的rgb转成15bit的rgb，并保存到buf
* @PARAM	disp_buf 存储转换之后的rgb 
			off 坐标点
			color 该点颜色rgb
* @RETURN	none
* @RETVAL
*/

void divoom_image_encode_set_data_64(uint8 *disp_buf, uint16 off, Pdivoom_image_color color);

/**
* NAME         divoom_image_decode_decode_get_pic_num
* @BRIEF	获取图片张数
* @PARAM	encode_data       图片的编码后的数据


* @RETURN	解码的图片张数
* @RETVAL
*/

int divoom_image_decode_decode_get_pic_num(uint8 *encode_data, uint32 encode_len);

/**
* NAME         divoom_image_encode_encode_pic64_one
* @BRIEF	对一张64*64图片进行编码
* @PARAM	rgb_data       图片的RGB数据， 如果１６的图，一张图有256个点，这个缓冲区大小为256*3*pic_num
            encode_buf     编码后的ＢＵＦ
* @RETURN	返回编码句柄,这个返回的内存，使用后需求释放
* @RETVAL
*/

int divoom_image_encode_encode_pic64_one(uint8 *net_rgb_data, uint8 *encode_buf);

/**
* NAME         divoom_image_encode_encode_pic128_one
* @BRIEF	对一张128*128图片进行编码
* @PARAM	rgb_data       图片的RGB数据， 
            encode_buf     编码后的ＢＵＦ
* @RETURN	返回编码句柄,这个返回的内存，使用后需求释放
* @RETVAL
*/

int divoom_image_encode_encode_pic128_one(uint8 *net_rgb_data, uint8 *encode_buf);

/**
* NAME         divoom_image_encode_encode_word
                    对动画的文字进行打包，做为压缩数据的一部分
* @PARAM	handle          创建的句柄
                    pword_info         文字的相关信息
* @RETURN	返回帧长度
* @RETVAL
*/

uint16 divoom_image_encode_encode_word(void *handle, PDIVOOM_IMAGE_ENCODE_WORD_INFO pword_info, uint8 *buf);

/**
* NAME         divoom_image_decode_get_pic_width
* @BRIEF	对ＩＦＲＡＭＥ获取数据
* @PARAM	encode_buf          缓冲区
* @RETURN	 图片的大小
* @RETVAL
*/

uint8 divoom_image_decode_get_pic_width(uint8 *deocde_data);

/**
* NAME         divoom_image_decode_check_convert_net
* @BRIEF	   这个图片是否要完成网图与本地图的转换
* @PARAM	encode_buf          创建的句柄；
                    rgb_buf               输出的ＲＧＢ缓冲区
                    width_flag           图像宽度
* @RETURN	 NONE
* @RETVAL
*/

uint8 divoom_image_decode_check_convert_net(uint8 *deocde_data);

/**
* NAME         divoom_image_decode_check_black_pic
* @BRIEF	从一帧数据中s检查是否为全黑色的图
* @PARAM	pic_data          帧数据
* @RETURN	 是否是黑屏
* @RETVAL
*/

uint16 divoom_image_decode_check_black_pic(uint8 *pic_data);

/*********************************************************
  Function:		divoom_play_com_convert_rgb_net_to_local
  Description:	将网络上的ＲＧＢ排法转换为设备显示的大图
  Input:			width           图像宽度
                    src_pic_buf     网络图片的数据
                    rgb_buf         转换可用的ＢＵＦ
  Return:			none
************************************************************/

void divoom_image_decode_convert_rgb_net_to_local(int width, uint8 *src_pic_buf, uint8 * rgb_buf);

#endif

