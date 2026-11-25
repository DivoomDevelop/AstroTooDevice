#ifndef _divoom_file_h_
#define _divoom_file_h_
typedef enum
{
    DIVOOM_FILE_ID_NEXT,    //后一个文件的ＩＤ
    DIVOOM_FILE_ID_PRE,     //前一个文件的ＩＤ
    DIVOOM_FILE_ID_FIRST,   //打开第一个文件
}DIVOOM_FILE_ID_TYPE;


typedef enum
{
    DIVOOM_FILE_HEAD_INDEX_FLAG_CON_FLAG,     //后续有数据储存位置的文件数据块
    DIVOOM_FILE_HEAD_INDEX_FLAG_FIRST_FLAG,    //文件数据第一块
    DIVOOM_FILE_HEAD_INDEX_FLAG_DATA,    //文件数据第一块
    DIVOOM_FILE_HEAD_INDEX_FLAG_NULL,    //空数据块
}DIVOOM_FILE_HEAD_INDEX_FLAG_INFO;

typedef enum
{
    DIVOOM_FILE_OPEN_FILE_READ,    //读文件
    DIVOOM_FILE_OPEN_FILE_WRITE,     //写文件
}DIVOOM_FILE_OPEN_FILE_MODE;
 

/*********************************************************
Function:		divoom_file_init
Description:		对整个文件系统初始化，此文件系统设计每次只能打开一个文件
Input:			NONE
Return:			NONE
************************************************************/

T_VOID divoom_file_init(T_VOID);

/*********************************************************
Function:		divoom_file_check_file_exist
Description:	查找对应文件的是否存在
Input:           model_id
file_id
Return:          1：存在；0：不存在
************************************************************/

uint8 divoom_file_check_file_exist(uint16 model_id, uint32 file_id);

/*********************************************************
Function:		divoom_file_open_read
Description:		打开文件方便读
Input:			model_id:0:
                           file_id: 
Return:			返回文件的的句柄　
************************************************************/

uint32 divoom_file_open_read(uint32 model_id, uint32 file_id);


/*********************************************************
Function:		divoom_file_read_pic
Description:		从打开的文件中读出一张图片的数据
Input:			handle
                            pic_data:读出图片的缓冲区
Return:			返回读到数据长度
************************************************************/

uint16 divoom_file_read_pic(uint32 handle, uint8 *pic_data);


/*********************************************************
Function:		divoom_file_read_data
Description:		从打开的文件中读出 固定数据长度
Input:			data:
len
Return:			返回数据长度
************************************************************/

uint32 divoom_file_read_data(uint32 handle,uint8 *data, uint32 len);



/*********************************************************
Function:		divoom_file_read_reset
Description:		将文件的指针复位为最开始
Input:			handle:
Return:			none
************************************************************/

T_VOID divoom_file_read_reset(uint32 handle);



/*********************************************************
Function:		divoom_file_get_file_attr
Description:	返回一个字节的文件属性
Input:			handle:0:
file_id:
Return:			返回文件的属性
************************************************************/

uint8 divoom_file_get_file_attr(uint32 handle);


/*********************************************************
Function:		divoom_file_read_reset
Description:		将文件的指针设置为某位置
Input:			handle:
                           offset:  文件偏移，相对文件头
Return:			NONE
************************************************************/

T_VOID divoom_file_read_seek(uint32 handle, uint32 offset);


/*********************************************************
Function:		divoom_file_get_file_ex_info
Description:	返回的文件扩展数据
Input:			handle:0:
file_id:
Return:			返回文件的属性
************************************************************/

uint16 divoom_file_get_file_ex_info(uint32 handle);


/*********************************************************
Function:		divoom_file_open_write
Description:		打开写文件，如果文件存在，将直接删除
Input:			model_id:
                file_id
                file_attr:文件特殊标记
                mode     :0:新文件，1：追加写文件
Return:			返回文件句柄
************************************************************/

uint32 divoom_file_open_write(uint16 model_id, uint32 file_id, uint8 file_attr, uint8 mode);


/*********************************************************
Function:		divoom_file_find_model_file
Description:	查找整个模块的文件信息
Input:			model_id,
                file_list
                max_file    返回的文件最大数目
Return:			none
************************************************************/

T_U16 divoom_file_find_model_file(uint16 model_id, uint32 *file_list, uint16 max_file);


/*********************************************************
Function:		divoom_file_write_data
Description:		写数据到文件
Input:			handle,
                           file_data
                           len,
Return:			返回写入文件的大小
************************************************************/

uint32 divoom_file_write_data(uint32 handle, uint8 *file_data, uint32 len);

/*********************************************************
Function:		divoom_file_change_file_id
Description:	修改文件句柄的FILEID
Input:			handle,
                file_id
Return:			NONE
************************************************************/

void divoom_file_change_file_id(uint32 handle, int file_id);


/*********************************************************
Function:		divoom_file_close
Description:		关闭正在读写的相关文件,
Input:			handle,
len
Return:			文件大小
************************************************************/

uint32 divoom_file_close(uint32 handle);



/*********************************************************
Function:		divoom_file_del_file
Description:		删除某个文件
Input:			model_id,
                           file_id
Return:			none
************************************************************/

T_VOID divoom_file_del_file(uint16 model_id,uint32 file_id);
/*********************************************************
Function:		divoom_file_del_model_file
Description:		删除整个模块的文件
Input:			model_id,
file_id
Return:			none
************************************************************/

T_VOID divoom_file_del_model_file(uint16 model_id);

/*********************************************************
Function:		divoom_file_del_file_no_flush
Description:	不更新位置与ＩＤ
Input:			model_id,
file_id
Return:			none
************************************************************/

uint16 divoom_file_del_file_no_flush(uint16 model_id, uint32 file_id);


/*********************************************************
Function:		divoom_file_exit
Description:		退出系统，关闭打开的文件
Input:			file_id,
Return:			none
************************************************************/

T_VOID divoom_file_exit(T_VOID);



/*********************************************************
Function:		divoom_file_get_free_size
Description:	返回可用空间
Input:
Return:
************************************************************/

uint32 divoom_file_get_free_size(T_VOID);


/*********************************************************
Function:		divoom_file_get_free_size
Description:	返回已用空间
Input:
Return:
************************************************************/

uint32 divoom_file_get_use_size(T_VOID);


/*********************************************************
Function:		divoom_file_write_data
Description:	带偏移写入到文件，这个主要是配合divoom_file_write_set_file_size使用,每次只能写ＤＩＶＯＯＭ_ＰＡＧＥ_ＳＩＺＥ
Input:			handle,
file_data
len,
Return:			返回写入文件的大小
************************************************************/

uint32 divoom_file_write_off_data(uint32 handle, uint8 *file_data, uint32 offset);


/*********************************************************
Function:		divoom_file_write_set_file_size
Description:	设置要写的文件的大小, 将提前分配一定的空间给用户写只能使用divoom_file_write_off_data读数据
Input:			file_handle:
file_size
Return:			返回成功与失败
************************************************************/

uint8 divoom_file_write_set_file_size(uint32 handle, uint32 file_size);

/*********************************************************
Function:		divoom_file_format
Description:	将整个文件系统格式化,相当于系统恢复默认值
Input:			NONE
Return:			NONE
************************************************************/

T_VOID divoom_file_format(T_VOID *custom_info);

/*********************************************************
Function:		divoom_file_get_file_attr
Description:	返回一个字节的文件属性
Input:			handle:0:
file_id:
Return:			返回文件的属性
************************************************************/

uint32 divoom_file_get_file_size(uint32 handle);

/*********************************************************
Function:		divoom_file_get_file_pos
Description:	返回文件的偏移地址
Input:			handle:0:
file_id:
Return:			返回文件的偏移
************************************************************/

uint32 divoom_file_get_file_pos(uint32 handle);

/*********************************************************
Function:		divoom_file_set_file_ex_info
Description:	设置文件附加信息
Input:			handle:0:
                ex_info:
Return:			NONE
************************************************************/

void divoom_file_set_file_ex_info(uint32 handle, uint16 ex_info);


/*********************************************************
Function:		divoom_file_set_file_attr
Description:	设置文件属性
Input:			handle:0:
                ex_info:
Return:			NONE
************************************************************/

void divoom_file_set_file_attr(uint32 handle, uint8 file_attr);

/*********************************************************
Function:		divoom_file_check_file_writing
Description:	判断这个文件是否是正在写的文件
Input:			model_id:0:
                file_id
file_id:
Return:			没有写文件返回-1：否则返回文件大小
************************************************************/

int divoom_file_check_file_writing(uint32 model_id, uint32 file_id);

/*********************************************************************
  Function:        divoom_file_ctrl_cut_file
  Description:     将文件从file_off开始，大小为cut_size的数据从文件中删除
  Input:           model_id   
                   file_id  
                   file_off
                   cut_size                    
  Return:           none
**********************************************************************/

void divoom_file_ctrl_cut_file(int model_id, int file_id, int file_off, int cut_size);

/*********************************************************
Function:		divoom_file_get_file_mode_id
Description:	得到文件句柄模块ID
Input:			handle,
file_data
len,
Return:			ID
************************************************************/

uint32 divoom_file_get_file_mode_id(uint32 handle);

#endif

