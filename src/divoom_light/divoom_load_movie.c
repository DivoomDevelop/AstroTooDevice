#include "divoom_disp_com.h"

/*********************************************************
所有保存动画的格式，第一页用四个字节保存整个文件的大小，其它可供保存特殊信息
从第二页开始，所有的数据都是使用divoom_image_decode进行编码的数据
.......
**********************************************************/
#define DIVOOM_LOAD_MOVIE_TABLE_NUM 16

typedef struct _divoom_load_movie_file_ctrl
{
    uint16 cur_pic_id;    //当前文件读的ID
    uint16 tol_pic_id;    //当前统计到的最大图片张数
    uint32 off_table[DIVOOM_LOAD_MOVIE_TABLE_NUM];
    uint32 first_frame_off;
    uint32 tol_size;    //文件总空间大小
    void *file_handle;
}divoom_load_movie_file_ctrl, *Pdivoom_load_movie_file_ctrl;



/*********************************************************
  Function:		divoom_load_movie_open_read
  Description:		打开文件句柄
  Input:			start_page          在ＦＬＡＳＨ上的页地址?
  ????     
  Return:			返回文件句柄
************************************************************/

void *divoom_load_movie_open_read(uint32 type, uint32 index,uint8 *buf, uint32 image_size)
{
    uint8 page_buf[DIVOOM_PAGE_SIZE];
    Pdivoom_load_movie_file_ctrl divoom_handle;
    
    divoom_handle = (Pdivoom_load_movie_file_ctrl)divoom_com_malloc(sizeof(divoom_load_movie_file_ctrl), __func__, __LINE__);
    if(divoom_handle == NULL)
    {
        divoom_print("%s:%d: it fails to malloc!\n",__FILE__, __LINE__);
        return NULL;
    }
    memset(divoom_handle, 0, sizeof(divoom_load_movie_file_ctrl));
    divoom_handle->file_handle = divoom_app_file_manage_open_read(type, index, 0);
    if(divoom_handle->file_handle == NULL)
    {
        divoom_print("%s:%d: it fails to open file: %d, %d!\n",__FILE__, __LINE__, type, index);
        divoom_com_free(divoom_handle, __func__, __LINE__);
        return NULL;
    }
    divoom_app_file_manage_read_data(divoom_handle->file_handle, DIVOOM_PAGE_SIZE, page_buf);
    if(buf)
    {
        memcpy(buf, page_buf + 4, DIVOOM_PAGE_SIZE - 4);
    }
    memcpy(&divoom_handle->tol_size, page_buf, sizeof(divoom_handle->tol_size));
    if(divoom_handle->tol_size == T_U32_MAX)
    {
        if(image_size == T_U32_MAX)
        {
            divoom_print("divoom_load_movie_open_read: it fails to read tol size!%d, %d\n",type, index);
            divoom_app_file_manage_close_read(divoom_handle->file_handle);
            divoom_com_free(divoom_handle, __func__, __LINE__);
            return NULL;
        }
        divoom_handle->tol_size = image_size;
    }
    //divoom_print("divoom_load_movie_open_read: %d\n", divoom_handle->tol_size);
    return divoom_handle;
}


/*********************************************************
  Function:		divoom_load_movie_read_off
  Description:		顺序读文件
  Input:			handle?句柄
  ????   buf          
                          len
  Return:			
************************************************************/

uint16 divoom_load_movie_read_off(void *handle, uint8 *buf, uint16 len)
{
    Pdivoom_load_movie_file_ctrl divoom_handle = (Pdivoom_load_movie_file_ctrl)handle;

    if(handle == NULL)
    {
        return 0;
    }

    return divoom_app_file_manage_read_data(divoom_handle->file_handle, len, buf);
}


/*********************************************************
  Function:		divoom_load_movie_get_pic_size
  Description:		得到当前的文件大小
  Input:			handle?句柄
  Return:			
************************************************************/

uint32 divoom_load_movie_get_pic_size(void *handle)
{
    Pdivoom_load_movie_file_ctrl divoom_handle = (Pdivoom_load_movie_file_ctrl)handle;

    if(handle == NULL)
    {
        return 0;
    }
    return divoom_handle->tol_size;
}


/*********************************************************
  Function:		divoom_load_movie_get_pic_off
  Description:		得到当前的偏移
  Input:			handle?句柄
  Return:			
************************************************************/

uint32 divoom_load_movie_get_pic_off(void *handle)
{
    Pdivoom_load_movie_file_ctrl divoom_handle = (Pdivoom_load_movie_file_ctrl)handle;

    if(handle == NULL)
    {
        return 0;
    }
    return divoom_app_file_manage_seek_off(divoom_handle->file_handle, T_U32_MAX);
}

/*********************************************************
  Function:		divoom_load_movie_set_image_size
  Description:		设置当前图像的空间大小
  Input:			handle?句柄
  Return:			
************************************************************/

void divoom_load_movie_set_image_size(void *handle, uint32 size)
{
    Pdivoom_load_movie_file_ctrl divoom_handle = (Pdivoom_load_movie_file_ctrl)handle;

    if(handle == NULL)
    {
        return ;
    }
    divoom_handle->tol_size = size;
}

/*********************************************************
  Function:		divoom_load_movie_set_pic_off
  Description:		设置当前的偏移
  Input:			handle?句柄
  Return:			
************************************************************/

void divoom_load_movie_set_pic_off(void *handle, uint32 off)
{
    Pdivoom_load_movie_file_ctrl divoom_handle = (Pdivoom_load_movie_file_ctrl)handle;

    if(handle == NULL)
    {
        return ;
    }
    if(off < DIVOOM_PAGE_SIZE)
    {
        off = DIVOOM_PAGE_SIZE;
    }
    divoom_app_file_manage_seek_off(divoom_handle->file_handle, off);
}

/*********************************************************
  Function:		divoom_load_movie_get_next_frame_len
  Description:		得到下一帧数据的长度
  Input:			handle?句柄
  ????   buf          
                          len
  Return:			
************************************************************/

uint16 divoom_load_movie_get_next_frame_len(void *handle)
{
    uint8 buf[DIVOOM_IMAGE_ENCODE_COLOR_NUM_OFF + 1];
    uint32 file_off, len, file_size;
    Pdivoom_load_movie_file_ctrl divoom_handle = (Pdivoom_load_movie_file_ctrl)handle;

    file_size = divoom_app_file_manage_get_file_size(divoom_handle->file_handle); 
    file_off = divoom_app_file_manage_seek_off(divoom_handle->file_handle, T_U32_MAX);
    len = divoom_app_file_manage_read_data(divoom_handle->file_handle, DIVOOM_IMAGE_ENCODE_COLOR_NUM_OFF, buf);
    divoom_app_file_manage_seek_off(divoom_handle->file_handle, file_off);
    if((buf[0] != 0xAA) || len != DIVOOM_IMAGE_ENCODE_COLOR_NUM_OFF )
    {
        if(len != 0)
        {
            divoom_print("%s: it is err len: %d, %d, %d\n", __func__, len, buf[0], file_off);
        }
        if(divoom_handle->tol_size > file_off)
        {
            divoom_print("%s:%d cut len: %d, %d\n", __func__, __LINE__,divoom_handle->tol_size, file_off);
            divoom_handle->tol_size = file_off;
        }
        return 0;
    }
    len = divoom_com_unpack_uint16(&buf[DIVOOM_IMAGE_ENCODE_LEN_OFF]);
    if(len > DIVOOM_IMAGE_ENCODE_MAX_64_SIZE || file_off + len >divoom_handle->tol_size || file_off + len > file_size)
    {
        divoom_print("%s:%d cut len: %d, %d, %d, %d\n", __func__, __LINE__,divoom_handle->tol_size, file_off, len, file_size);
        divoom_handle->tol_size = file_off;
        return 0;
    }
    return len;
}

/*********************************************************
  Function:		divoom_load_movie_load_first_pic
  Description:		读第一张图片
  Input:			handle 句柄
                    buf          
                    word_flag 是否有文字帧
                    image_size   图片的大小
  Return:		读数据的长度	,如果为0 读失败
************************************************************/

uint16 divoom_load_movie_load_first_pic(void *handle, uint8 *buf, uint8 *word_flag, uint16 *image_size)
{
    uint8 end_flag = 0, frame_attr = 0;
    uint16 pic_len, pic_size;

    divoom_load_movie_reset(handle);
    while(end_flag == 0)
    {
        pic_len = divoom_load_movie_load(handle, buf, &end_flag);
        if(pic_len == 0)
        {
            break;
        }
        if(end_flag == 0)
        {
            pic_size = divoom_image_decode_get_image_info(buf, &frame_attr);
            if(frame_attr == DIVOOM_IMAGE_ENCODE_PIC_TYPE)
            {
                if(image_size)
                {
                    *image_size = pic_size;
                }
                return pic_len;
            }
            if(word_flag &&frame_attr == DIVOOM_IMAGE_ENCODE_WORD_TYPE && pic_len != DIVOOM_IMAGE_ENCODE_COLOR_DATA_32_OFF)
            {
                *word_flag = 1;
            }
        }
    }
    return 0;
}

/*********************************************************
  Function:		divoom_load_movie_read_part
  Description:		顺序读数据
  Input:			handle:句柄
                    buf    
                    read_len
                          len
  Return:			
************************************************************/

uint16 divoom_load_movie_read_part(void *handle, uint8 *buf, uint16 read_len)
{
    Pdivoom_load_movie_file_ctrl divoom_handle = (Pdivoom_load_movie_file_ctrl)handle;

    if(handle == NULL)
    {
        return 0;
    }
    return divoom_app_file_manage_read_data(divoom_handle->file_handle, read_len, buf);
}

/*********************************************************
  Function:		divoom_load_movie_load
  Description:		读一帧数据
  Input:			handle?句柄
  ????   buf          
                          len
  Return:			
************************************************************/

uint16 divoom_load_movie_load(void *handle, uint8 *buf, uint8 *end_flag)
{
    uint32 cur_off;
    uint16 len = T_U16_MAX, read_len, try_time = 0;
    Pdivoom_load_movie_file_ctrl divoom_handle = (Pdivoom_load_movie_file_ctrl)handle;

    if(handle == NULL)
    {
        return 0;
    }
    divoom_load_movie_load_retry_table:
    cur_off = divoom_app_file_manage_seek_off(divoom_handle->file_handle, T_U32_MAX);
    if(cur_off >= divoom_handle->tol_size)
    {
        divoom_handle->cur_pic_id = 0;
        divoom_app_file_manage_seek_off(divoom_handle->file_handle, divoom_handle->first_frame_off?divoom_handle->first_frame_off:DIVOOM_PAGE_SIZE);
        if(end_flag)
        {
            *end_flag = 1;
        }
    }
    else
    {
        if(end_flag)
        {
            *end_flag = 0;
        }
    }
    read_len = divoom_app_file_manage_read_data(divoom_handle->file_handle, DIVOOM_IMAGE_ENCODE_COLOR_DATA_OFF, buf);
    if(read_len != DIVOOM_IMAGE_ENCODE_COLOR_DATA_OFF)
    {
        divoom_print("divoom_load_movie_load: fail:len:%d\n",__LINE__);
        goto divoom_load_movie_load_read_err_table;
    }
    len = divoom_com_unpack_uint16(&buf[DIVOOM_IMAGE_ENCODE_LEN_OFF]);
    if(len > DIVOOM_IMAGE_ENCODE_MAX_64_SIZE)
    {
        divoom_print("divoom_load_movie_load: fail:len:%d\n",len);
        goto divoom_load_movie_load_read_err_table;
    }
    read_len = len;
    len = divoom_load_movie_read_off(handle, &buf[DIVOOM_IMAGE_ENCODE_COLOR_DATA_OFF], len-DIVOOM_IMAGE_ENCODE_COLOR_DATA_OFF);
    if((buf[0] != 0xAA) || len < read_len - DIVOOM_IMAGE_ENCODE_COLOR_DATA_OFF)
    {
        divoom_print("divoom_load_movie_load: fail:flag %d; %d!=%d, off: %d\n", buf[0], len,read_len - DIVOOM_IMAGE_ENCODE_COLOR_DATA_OFF, cur_off);
        goto divoom_load_movie_load_read_err_table;
    }
    else
    {
        uint8 frame_attr;
        
        divoom_image_decode_get_image_info(buf, &frame_attr);
        if(divoom_handle->first_frame_off == 0 && (frame_attr == DIVOOM_IMAGE_ENCODE_PIC_TYPE))
        {
            divoom_handle->first_frame_off = cur_off;
        }
        if(frame_attr == DIVOOM_IMAGE_ENCODE_PIC_TYPE || frame_attr == DIVOOM_IMAGE_ENCODE_WORD_TYPE)
        {
            divoom_handle->cur_pic_id ++;
            if(divoom_handle->cur_pic_id >= divoom_handle->tol_pic_id)
            {
                divoom_handle->tol_pic_id = divoom_handle->cur_pic_id ;
            }
        }
        return read_len;
    }
    divoom_load_movie_load_read_err_table:
    if(end_flag)
    {
        *end_flag = 1;
    }
    divoom_handle->cur_pic_id = 0;
    divoom_handle->tol_size = divoom_app_file_manage_seek_off(divoom_handle->file_handle, T_U32_MAX);
    divoom_app_file_manage_seek_off(divoom_handle->file_handle, divoom_handle->first_frame_off?divoom_handle->first_frame_off:DIVOOM_PAGE_SIZE);
    try_time ++;
    if(try_time <= 1)
    {
        goto divoom_load_movie_load_retry_table;
    }
    return 0;
}


/*********************************************************
  Function:		divoom_load_movie_close
  Description:		使用完文件句柄，关闭句柄
  Input:			handle?句柄
  Return:			
************************************************************/

void divoom_load_movie_close(void *handle)
{
    Pdivoom_load_movie_file_ctrl divoom_handle = (Pdivoom_load_movie_file_ctrl)handle;
    
    if(divoom_handle)
    {
        divoom_app_file_manage_close_read(divoom_handle->file_handle);
        divoom_com_free(handle, __func__, __LINE__);
    }
}

/*********************************************************
  Function:		divoom_load_movie_set_pos
  Description:		将读句柄指针回头
  Input:			handle?句柄
  Return:			
************************************************************/

void divoom_load_movie_reset(void *handle)
{
    Pdivoom_load_movie_file_ctrl divoom_handle = (Pdivoom_load_movie_file_ctrl)handle;

    if(divoom_handle)
    {
        divoom_handle->cur_pic_id = 0;      
         divoom_app_file_manage_seek_off(divoom_handle->file_handle, divoom_handle->first_frame_off?divoom_handle->first_frame_off:DIVOOM_PAGE_SIZE);
    }
}

/*********************************************************
  Function:		divoom_load_movie_set_read_cache_pic_num
  Description:		重置已经加载的图片张图
  Input:			handle 句柄
  Return:			
************************************************************/

void divoom_load_movie_set_read_cache_pic_num(void *handle)
{
    Pdivoom_load_movie_file_ctrl divoom_handle = (Pdivoom_load_movie_file_ctrl)handle;

    if(handle)
    {
        divoom_handle->tol_pic_id = 0;
    }
    
}


/*********************************************************
  Function:		divoom_load_movie_get_read_cache_pic_num
  Description:		得到已经加载的图片张图
  Input:			handle 句柄
  Return:			
************************************************************/

uint8 divoom_load_movie_get_read_cache_pic_num(void *handle)
{
    Pdivoom_load_movie_file_ctrl divoom_handle = (Pdivoom_load_movie_file_ctrl)handle;

    if(handle)
    {
        return divoom_handle->tol_pic_id;
    }
    return 0;
    
}


/*********************************************************
  Function:		divoom_load_movie_get_pic_num
  Description:		得到总共的图片张图
  Input:			handle?句柄
  Return:			
************************************************************/

uint8 divoom_load_movie_get_pic_num(void *handle)
{
    Pdivoom_load_movie_file_ctrl divoom_handle = (Pdivoom_load_movie_file_ctrl)handle;
    uint8 pic_buf[DIVOOM_IMAGE_ENCODE_COLOR_DATA_OFF];
    uint8 frame_attr = 0;
    uint16 len, pic_num = 0;
    uint32 pic_off;

    divoom_load_movie_reset(handle);

    while(pic_num < DIVOOM_LOAD_MOVIE_TABLE_NUM)
    {
        pic_off = divoom_app_file_manage_seek_off(divoom_handle->file_handle, T_U32_MAX);
        len = divoom_load_movie_read_part(handle, pic_buf, DIVOOM_IMAGE_ENCODE_COLOR_DATA_OFF);
        if(len < DIVOOM_IMAGE_ENCODE_COLOR_DATA_OFF)
        {            
            divoom_print("%s:exit: read: %d;\n",__func__, pic_num);
            break;
        }
        divoom_image_decode_get_image_info(pic_buf, &frame_attr);
        if(frame_attr == DIVOOM_IMAGE_ENCODE_PIC_TYPE)
        {
            divoom_handle->off_table[pic_num] = pic_off;
            pic_num ++;
        }
        len = divoom_image_decode_get_pic_len(pic_buf);
        divoom_app_file_manage_seek_off(divoom_handle->file_handle, pic_off + len);
    }
    divoom_load_movie_reset(handle);
    return pic_num;    

}


/*********************************************************
  Function:		divoom_load_movie_check_single_pic
  Description:	检查数据是否为单张动画
  Input:			handle  句柄
  Return:		是否是单张图片	
************************************************************/

uint8 divoom_load_movie_check_single_pic(void *handle)
{
    uint8 pic_buf[DIVOOM_IMAGE_ENCODE_COLOR_DATA_OFF];
    uint16 len = 0, pic_num = 0, read_len;
    uint32 pic_off;
    uint8 frame_attr, frame_id = 0;
    Pdivoom_load_movie_file_ctrl divoom_handle = (Pdivoom_load_movie_file_ctrl)handle;

    divoom_load_movie_reset(handle);

    while(pic_num < 2)
    {
        pic_off = divoom_app_file_manage_seek_off(divoom_handle->file_handle, T_U32_MAX);
        read_len = divoom_load_movie_read_part(handle, pic_buf, DIVOOM_IMAGE_ENCODE_COLOR_DATA_OFF);
        if(read_len < DIVOOM_IMAGE_ENCODE_COLOR_DATA_OFF)
        {
            
            divoom_print("%s:exit: read: %d;\n",__func__, frame_id);
            break;
        }
        divoom_image_decode_get_image_info(pic_buf, &frame_attr);
        if(frame_attr == DIVOOM_IMAGE_ENCODE_ERR_TYPE)
        {
            divoom_print("%s:exit: err frame: %d;\n",__func__, frame_id);
            break;
        }
        divoom_print("%s:read: %d;type: %d, flag: %d, len: %d\n",__func__, frame_id, frame_attr, pic_buf[DIVOOM_IMAGE_ENCODE_TYPE_OFF], len);
        if(frame_attr == DIVOOM_IMAGE_ENCODE_PIC_TYPE)
        {
            pic_num ++;
        }
        if(frame_attr == DIVOOM_IMAGE_ENCODE_REVIEW_TYPE)
        {
            divoom_print("%s:find review: %d\n", __func__, frame_id);
            pic_num = 2;
        }
        len = divoom_image_decode_get_pic_len(pic_buf);
        if(len == 0)
        {
            divoom_print("%s:exit: err len: %d;\n",__func__, len);
            break;
        }
        divoom_app_file_manage_seek_off(divoom_handle->file_handle, pic_off + len);
        frame_id ++;
    }
    divoom_print("%s:end review: %d, num: %d\n", __func__, frame_id, pic_num);
    divoom_load_movie_reset(handle);
    return pic_num == 1;    

}


/*********************************************************
  Function:		divoom_load_movie_get_pic_info
  Description:		读出第Ｎ帧数据
  Input:			handle?句柄
  Return:			
************************************************************/

uint8 divoom_load_movie_get_pic_info(void *handle, uint16 pic_id, uint16 *pic_len, uint8 *pic_data)
{
    uint16 read_len;
    Pdivoom_load_movie_file_ctrl divoom_handle = (Pdivoom_load_movie_file_ctrl)handle;

    
    if(pic_id <= DIVOOM_LOAD_MOVIE_TABLE_NUM)
    {
        divoom_app_file_manage_seek_off(divoom_handle->file_handle, divoom_handle->off_table[pic_id]);
    }
    else
    {
        divoom_load_movie_reset(handle);
    }
    
    read_len = divoom_app_file_manage_read_data(divoom_handle->file_handle, DIVOOM_IMAGE_ENCODE_COLOR_DATA_OFF, pic_data);
    if(read_len != DIVOOM_IMAGE_ENCODE_COLOR_DATA_OFF)
    {
        return AK_FALSE;
    }
    *pic_len = divoom_com_unpack_uint16(&pic_data[DIVOOM_IMAGE_ENCODE_LEN_OFF]);
    read_len = divoom_app_file_manage_read_data(divoom_handle->file_handle, *pic_len - DIVOOM_IMAGE_ENCODE_COLOR_DATA_OFF, &pic_data[DIVOOM_IMAGE_ENCODE_COLOR_DATA_OFF]);
    if(read_len != *pic_len - DIVOOM_IMAGE_ENCODE_COLOR_DATA_OFF)
    {
        return AK_FALSE;
    }
    return AK_TRUE;
}


/*********************************************************
  Function:		divoom_load_movie_get_pic_data
  Description:		读出第Ｎ帧数据
  Input:			handle?句柄
  Return:			
************************************************************/

uint8 divoom_load_movie_get_pic_data(void *handle, uint16 pic_id, uint16 *pic_len, uint8  *pic_data)
{
    Pdivoom_load_movie_file_ctrl divoom_handle = (Pdivoom_load_movie_file_ctrl)handle;
    
//    divoom_load_gif_reset(handle);
    if(pic_id <= DIVOOM_LOAD_MOVIE_TABLE_NUM)
    {
        divoom_app_file_manage_seek_off(divoom_handle->file_handle, divoom_handle->off_table[pic_id]);
    }
    else
    {
        return 0;
    }
    *pic_len = divoom_load_movie_load(handle, pic_data, NULL);            
    return 1;
}



