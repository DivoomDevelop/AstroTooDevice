#ifndef  _divoom_read_image_h_
#define _divoom_read_image_h_

typedef struct _divoom_read_image_file_info
{
    uint8  model_id;        //模块ID
    uint8  file_id;         //文件ID
    uint16 read_size;
    uint32 file_size;       //当前文件大小
    uint32 file_off;        //文件偏移
}divoom_read_image_file_info, *Pdivoom_read_image_file_info;


/**
* NAME       divoom_read_image_init
* @BRIEF	创建读数据的中间模块
* @PARAM	pfirst      当前要读的文件信息
            pnext       下一个文件信息，可以为空，当系统只需要播放一个文件时，像开机文件，闹钟文件
* @RETURN	0：fail;  1:ok
* @RETVAL
*/

int divoom_read_image_init(Pdivoom_read_image_file_info pfirst, Pdivoom_read_image_file_info pnext);

/**
* NAME       divoom_read_image_read_pic
* @BRIEF	从缓冲区中读取一张图片的数据
* @PARAM	pic_buf
* @RETURN	读取数据长度
* @RETVAL
*/

uint16 divoom_read_image_read_pic(uint8 *pic_buf);

/**
* NAME       divoom_read_image_update_next
* @BRIEF	增加一个文件，在要切换文件显示前调用
* @PARAM	pnext       下一个文件的信息
* @RETURN	none
* @RETVAL
*/

void divoom_read_image_update_next(Pdivoom_read_image_file_info pnext);

/**
* NAME       divoom_read_image_main_loop
* @BRIEF	主循环调用，用来读数据到缓冲区中
* @PARAM	
* @RETVAL
*/

void divoom_read_image_main_loop(void);

/**
* NAME       divoom_read_image_exit
* @BRIEF	退出，并释放资源
* @PARAM	
* @RETURN	none
* @RETVAL
*/

void divoom_read_image_exit(void);

#endif

