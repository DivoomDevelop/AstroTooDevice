#ifndef __divoom_font_h
#define __divoom_font_h

typedef enum{
	DIVOOM_FONT_TYPE_8 = 0,
	DIVOOM_FONT_TYPE_12 = 1,
	DIVOOM_FONT_TYPE_16 = 2,
}DIVOOM_FONT_TYPE;


#define DIVOOM_FONT_LINE_SIZE(length)		((length+7)/8)	//一列占用的大小，以字节为单位，length为字符点阵边的大小，8 bits alignment
#define DIVOOM_FONT_MATRIX_SIZE(length)		(((length+7)/8)*length)

typedef struct __divoom_font_matrix_info
{
    uint16 font_info[16];		//16*16
}divoom_font_matrix_info,*Pdivoom_font_matrix_info;


#define DIVOOM_FONT_BUF_MAX_NUM		500

typedef struct __divoom_font_buffer_info
{
	uint8 write_flag;			//缓存需要进行更新写入的标志
	uint8 font_size;			//文字大小
	uint8 buf_read_id;			//要读取数据的BUF ID
	uint8 buf_write_id;			//要写入数据的BUF ID
	uint16 start_read_id;		//read buffer中word id起始位置	
	uint16 start_write_id;		//write buffer中word id起始位置	
	uint16 word_num;			//字符总数
	uint16 start_id[2];			//两个buf起始点的文字id
	uint16 *unicode_text;		//unicode文字指针
    divoom_font_matrix_info font_info[DIVOOM_FONT_BUF_MAX_NUM];
	Pdivoom_font_matrix_info buf[2];	//两个读写的BUF指针，指向font_info[DIVOOM_FONT_BUF_MAX_NUM]的起点及中点
}divoom_font_buffer_info,*Pdivoom_font_buffer_info;

/*****************************************************************************
 Prototype    : divoom_font_read_matrix_buffer
 Description  : 读取字模缓存第word_id个的数据
 Input        : Pdivoom_font_buffer_info handle  	缓存的句柄
                uint16 word_id      unicode字串中的ID 第word_id个            
 Output       : None
 Return Value : 字模信息的指针
 
  History        :
  1.Date         : 2020/6/20
    Author       : coronaa
    Modification : Created function
*****************************************************************************/
Pdivoom_font_matrix_info divoom_font_read_matrix_buffer(Pdivoom_font_buffer_info handle,uint16 word_id);
/*****************************************************************************
 Prototype    : divoom_font_write_matrix_buffer
 Description  : 更新待读unicode写入缓存区字模数据
 Input        : Pdivoom_font_buffer_info handle  
 Output       : None
 Return Value : 
 
  History        :
  1.Date         : 2020/6/20
    Author       : coronaa
    Modification : Created function
*****************************************************************************/
void divoom_font_write_matrix_buffer(Pdivoom_font_buffer_info handle);
/*****************************************************************************
 Prototype    : divoom_font_init_matrix_buffer
 Description  : 初始化更新缓存区数据：字串，字体大小
 Input        : Pdivoom_font_buffer_info handle  
                uint16 *unicode_text             
                uint16 len                       
                uint8 font_size                  
 Output       : None
 Return Value : 
 
  History        :
  1.Date         : 2020/6/20
    Author       : coronaa
    Modification : Created function
*****************************************************************************/
void divoom_font_init_matrix_buffer(Pdivoom_font_buffer_info handle,uint16 *unicode_text,uint16 len,uint8 font_size);


/*****************************************************************************
 Prototype    : divoom_font_get_unicode_type
 Description  : 分析unicode类型
 Input        : uint16 word  
 Output       : None
 Return Value : 0：基本拉丁文使用8X8;1:字母类：使用12X12;  2：中文及其它使用16X16;	 0xff：不在预置字条符集里
 
  History        :
  1.Date         : 2020/6/15
    Author       : coronaa
    Modification : Created function
*****************************************************************************/
uint8 divoom_font_get_unicode_type(uint16 word);
/*****************************************************************************
 Prototype    : divoom_font_check_use_font_size
 Description  : 得到要使用的字体大小
 Input        : uint16 *unicode_buf  
                uint16 num           
 Output       : None
 Return Value : 
 
  History        :
  1.Date         : 2020/6/15
    Author       : coronaa
    Modification : Created function
*****************************************************************************/
uint8 divoom_font_check_use_font_size(uint16 *unicode_buf,uint16 num);
/*****************************************************************************
 Prototype    : divoom_font_check_support_unicode
 Description  : 查找筛选出支持的字符和选择的字体大小
 Input        : uint16 *unicode_buf  筛选出来的buf
                uint16 *data         原始数据
                uint16 num           长度
                uint8 *font_size	 字体大小     
 Output       : None
 Return Value : 返回有效字符长度
 
  History        :
  1.Date         : 2020/6/19
    Author       : coronaa
    Modification : Created function
*****************************************************************************/
uint16 divoom_font_check_support_unicode(uint16 *unicode_buf,uint16 *data,uint16 num,uint8 *font_size);


void divoom_font_get_fit_word_info(uint16 *unicode_info, uint16 num,uint8 size, uint8 *dis_info);
void divoom_font_get_fit_word_info2(uint16 *unicode_info, uint16 num,uint8 size, Pdivoom_font_matrix_info matrix_buf);
void divoom_font_filebuf_get_fit_word_info(uint16 *unicode_info, uint16 num,uint8 size, Pdivoom_font_matrix_info matrix_buf);

void divoom_font_init(void);

#endif

