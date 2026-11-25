#include "divoom.h"

#if 0
#define DIVOOM_READ_IMAGE_GIF_BUF_SIZE 4096
#define DIVOOM_READ_IMAGE_EACH_BUF_SIZE 512

typedef struct _divoom_read_image_ctrl
{
    void        *lock;
    void        *cur_cache_handle;
    void        *next_cache_handle;
    FILE        *cur_file_handle;
    FILE        *next_file_handle;
    char       pic_buf[DIVOOM_READ_IMAGE_EACH_BUF_SIZE];
    divoom_read_image_file_info cur_file_info;
    divoom_read_image_file_info next_file_info;
}divoom_read_image_ctrl, *Pdivoom_read_image_ctrl;

Pdivoom_read_image_ctrl pdivoom_read_image_handle = NULL;

/**
* NAME       divoom_read_image_init
* @BRIEF	创建读数据的中间模块
* @PARAM	pfirst      当前要读的文件信息
            pnext       下一个文件信息，可以为空，当系统只需要播放一个文件时，像开机文件，闹钟文件
* @RETURN	0：fail;  1:ok
* @RETVAL
*/

int divoom_read_image_init(Pdivoom_read_image_file_info pfirst, Pdivoom_read_image_file_info pnext)
{
    int read_size;
    
    if(pfirst == NULL)
    {
        return 0;
    }
    if(pdivoom_read_image_handle != NULL)
    {
        //如果重复打开，先关闭之前的资源文件，
        if(pdivoom_read_image_handle->cur_file_handle)
        {
            divoom_port_file_close(pdivoom_read_image_handle->cur_file_handle);
        }
        pdivoom_read_image_handle->cur_file_handle = NULL;
        if(pdivoom_read_image_handle->next_file_handle)
        {
            divoom_port_file_close(pdivoom_read_image_handle->next_file_handle);
        }
        pdivoom_read_image_handle->next_file_handle = NULL;
        if(pdivoom_read_image_handle->cur_cache_handle)
        {
           divoom_cyc_buf_exit(pdivoom_read_image_handle->cur_cache_handle);
        }
        pdivoom_read_image_handle->cur_cache_handle = NULL;
        if(pdivoom_read_image_handle->next_cache_handle)
        {
           divoom_cyc_buf_exit(pdivoom_read_image_handle->next_cache_handle);
        }
        pdivoom_read_image_handle->next_cache_handle = NULL;
    }
    else
    {
        pdivoom_read_image_handle = divoom_system_malloc(sizeof(divoom_read_image_ctrl));
        pdivoom_read_image_handle->lock = divoom_system_create_lock();
    }
    sprintf(pdivoom_read_image_handle->pic_buf, "%s%d-%d", DIVOOM_SAVE_GIF_DIR, pfirst->model_id, pfirst->file_id);
    pdivoom_read_image_handle->cur_file_handle = divoom_port_file_open(pdivoom_read_image_handle->pic_buf, "rb");
    if(pdivoom_read_image_handle->cur_file_handle == NULL)
    {
        divoom_read_image_exit();
        return 0;
    }
    if(pfirst->file_size == U32_MAX)
    {
        divoom_port_file_seek(pdivoom_read_image_handle->cur_file_handle, 0, SEEK_END);
        pfirst->file_size = divoom_port_file_tell(pdivoom_read_image_handle->cur_file_handle) - pfirst->file_off;
    }
    divoom_port_file_seek(pdivoom_read_image_handle->cur_file_handle, pfirst->file_off, SEEK_SET);
    
    pdivoom_read_image_handle->cur_cache_handle = divoom_cyc_buf_init(DIVOOM_READ_IMAGE_GIF_BUF_SIZE);
    if(pdivoom_read_image_handle->cur_cache_handle == NULL)
    {
        divoom_read_image_exit();
        return 0;
    }
    memcpy(&pdivoom_read_image_handle->cur_file_info, pfirst, sizeof(divoom_read_image_file_info));
    pdivoom_read_image_handle->cur_file_info.read_size = 0;
    //加载2K数据先
    while(pdivoom_read_image_handle->cur_file_info.read_size < pdivoom_read_image_handle->cur_file_info.file_size &&
        pdivoom_read_image_handle->cur_file_info.read_size < DIVOOM_READ_IMAGE_GIF_BUF_SIZE/2)
    {
        if(pdivoom_read_image_handle->cur_file_info.read_size + DIVOOM_READ_IMAGE_EACH_BUF_SIZE < pdivoom_read_image_handle->cur_file_info.file_size)
        {
            read_size = DIVOOM_READ_IMAGE_EACH_BUF_SIZE;
        }
        else
        {
            read_size = pdivoom_read_image_handle->cur_file_info.file_size - pdivoom_read_image_handle->cur_file_info.read_size;
        }
        read_size = divoom_port_file_read(pdivoom_read_image_handle->pic_buf, 1, read_size, pdivoom_read_image_handle->cur_file_handle);
        if(read_size <= 0)
        {
            break;
        }
        pdivoom_read_image_handle->cur_file_info.read_size += read_size;
        divoom_cyc_buf_write(pdivoom_read_image_handle->cur_cache_handle, pdivoom_read_image_handle->pic_buf, read_size);
    }

    if(pnext)
    {
        sprintf(pdivoom_read_image_handle->pic_buf, "%s%d-%d", DIVOOM_SAVE_GIF_DIR, pnext->model_id, pnext->file_id);
        pdivoom_read_image_handle->next_file_handle = divoom_port_file_open(pdivoom_read_image_handle->pic_buf, "rb");
        if(pdivoom_read_image_handle->next_file_handle == NULL)
        {
            divoom_read_image_exit();
            return 0;
        }
        if(pnext->file_size == U32_MAX)
        {
            divoom_port_file_seek(pdivoom_read_image_handle->next_file_handle, 0, SEEK_END);
            pnext->file_size = divoom_port_file_tell(pdivoom_read_image_handle->next_file_handle) - pnext->file_off;
        }
        divoom_port_file_seek(pdivoom_read_image_handle->next_file_handle, pnext->file_off, SEEK_SET);
        
        pdivoom_read_image_handle->next_cache_handle = divoom_cyc_buf_init(DIVOOM_READ_IMAGE_GIF_BUF_SIZE);
        if(pdivoom_read_image_handle->next_cache_handle == NULL)
        {
            divoom_read_image_exit();
            return 0;
        }
        memcpy(&pdivoom_read_image_handle->next_file_info, pnext, sizeof(divoom_read_image_file_info));
        pdivoom_read_image_handle->next_file_info.read_size = 0;
    }
    
}



/**
* NAME       divoom_read_image_read_pic
* @BRIEF	从缓冲区中读取一张图片的数据
* @PARAM	pic_buf
* @RETURN	读取数据长度
* @RETVAL
*/

uint16 divoom_read_image_read_pic(uint8 *pic_buf)
{
    uint16 read_len, image_len;
    
    if(pdivoom_read_image_handle == NULL)
    {
        return;
    }
    divoom_system_get_lock(pdivoom_read_image_handle->lock);
    if(pdivoom_read_image_handle->cur_file_info.file_size == pdivoom_read_image_handle->cur_file_info.read_size
        && pdivoom_read_image_handle->cur_file_info.file_size < DIVOOM_READ_IMAGE_GIF_BUF_SIZE && divoom_cyc_buf_get_data_size(pdivoom_read_image_handle->cur_cache_handle) == 0)
    {
        //如果没有数据，并且文件的总大小小于缓冲区大小，将复位缓冲区
        if(divoom_cyc_buf_get_reset_data_size(pdivoom_read_image_handle->cur_cache_handle) == 0)
        {
            //没有加载到数据
            divoom_system_free_lock(pdivoom_read_image_handle->lock);
            return 0;
        }
    }
    read_len = divoom_cyc_buf_read(pdivoom_read_image_handle->cur_cache_handle, pic_buf, DIVOOM_IMAGE_ENCODE_COLOR_DATA_OFF);
    if(read_len <= 0)
    {
        divoom_system_free_lock(pdivoom_read_image_handle->lock);
        return 0;
    }
    else if(read_len < DIVOOM_IMAGE_ENCODE_COLOR_DATA_OFF)
    {
        divoom_cyc_buf_write(pdivoom_read_image_handle->cur_cache_handle, pic_buf, read_len);
        divoom_system_free_lock(pdivoom_read_image_handle->lock);
        return 0;
    }
    else
    {
        image_len = divoom_image_decode_get_pic_len(pic_buf);
        if(divoom_cyc_buf_get_data_size(pdivoom_read_image_handle->cur_cache_handle) < image_len - DIVOOM_IMAGE_ENCODE_COLOR_DATA_OFF)
        {
            //如果数据不足一帧的话，先将数据全部读出来，再将数据回写到缓冲区
            read_len = divoom_cyc_buf_get_data_size(pdivoom_read_image_handle->cur_cache_handle);
            read_len = divoom_cyc_buf_read(pdivoom_read_image_handle->cur_cache_handle, &pic_buf[DIVOOM_IMAGE_ENCODE_COLOR_DATA_OFF], read_len);
            divoom_cyc_buf_write(pdivoom_read_image_handle->cur_cache_handle, pic_buf, read_len + DIVOOM_IMAGE_ENCODE_COLOR_DATA_OFF);
            divoom_system_free_lock(pdivoom_read_image_handle->lock);
            return 0;
        }
        divoom_cyc_buf_read(pdivoom_read_image_handle->cur_cache_handle, &pic_buf[read_len], image_len - DIVOOM_IMAGE_ENCODE_COLOR_DATA_OFF);
    }
    divoom_system_free_lock(pdivoom_read_image_handle->lock);
    return image_len;
}


/**
* NAME       divoom_read_image_update_next
* @BRIEF	增加一个文件，在要切换文件显示前调用
* @PARAM	pnext       下一个文件的信息
* @RETURN	none
* @RETVAL
*/

void divoom_read_image_update_next(Pdivoom_read_image_file_info pnext)
{
    void *back_cache_buf, *back_file_handle;
    divoom_read_image_file_info back_file_info;
    
    if(pdivoom_read_image_handle == NULL)
    {
        return;
    }
    divoom_system_get_lock(pdivoom_read_image_handle->lock);
    if(pdivoom_read_image_handle->next_cache_handle == NULL)
    {
        //这个位置出错了
        divoom_print("it is err and please check it!\n");
        divoom_system_free_lock(pdivoom_read_image_handle->lock);
        return;
    }
    
    //先让数据能读到，不影响显示
    back_cache_buf = pdivoom_read_image_handle->cur_cache_handle;
    back_file_handle = pdivoom_read_image_handle->cur_file_handle;
    memcpy(&back_file_info , &pdivoom_read_image_handle->cur_file_info, sizeof(divoom_read_image_file_info));
    pdivoom_read_image_handle->cur_cache_handle = pdivoom_read_image_handle->next_cache_handle;
    pdivoom_read_image_handle->cur_file_handle = pdivoom_read_image_handle->next_file_handle;
    memcpy(&pdivoom_read_image_handle->cur_file_info, &pdivoom_read_image_handle->next_file_info, sizeof(divoom_read_image_file_info));
    pdivoom_read_image_handle->next_cache_handle = NULL;    
    divoom_system_free_lock(pdivoom_read_image_handle->lock);

    
    //打开第二个缓冲文件
    if(back_file_info.model_id == pnext->model_id &&
        back_file_info.file_id == pnext->file_id)
    {
        //不用再打开文件，用之前的文件句柄,并对当前的与下一个的数据信息交换
        divoom_print("we will use the previos handle!\n");
        pdivoom_read_image_handle->next_file_handle = (FILE *)back_file_handle;
    }
    else
    {
        divoom_print("we will reopen the file handle!\n");
        divoom_port_file_close(back_file_handle);
        sprintf(pdivoom_read_image_handle->pic_buf, "%s%d-%d", DIVOOM_SAVE_GIF_DIR, pnext->model_id, pnext->file_id);
        pdivoom_read_image_handle->next_file_handle = divoom_port_file_open(pdivoom_read_image_handle->pic_buf, "rb");
        if(pdivoom_read_image_handle->next_file_handle == NULL)
        {
            divoom_print("it fails to open the file: %s!\n", pdivoom_read_image_handle->pic_buf);
            memcpy(pnext, &back_file_info, sizeof(divoom_read_image_file_info));            
            sprintf(pdivoom_read_image_handle->pic_buf, "%s%d-%d", DIVOOM_SAVE_GIF_DIR, pnext->model_id, pnext->file_id);
            pdivoom_read_image_handle->next_file_handle = divoom_port_file_open(pdivoom_read_image_handle->pic_buf, "rb");
        }
    }
    if(pnext->file_size == U32_MAX)
    {
        divoom_port_file_seek(pdivoom_read_image_handle->next_file_handle, 0, SEEK_END);
        pnext->file_size = divoom_port_file_tell(pdivoom_read_image_handle->next_file_handle) - pnext->file_off;
    }
    divoom_port_file_seek(pdivoom_read_image_handle->next_file_handle, pnext->file_off, SEEK_SET);
    memcpy(&pdivoom_read_image_handle->next_file_info, pnext, sizeof(divoom_read_image_file_info));
    divoom_cyc_buf_clear_data(back_cache_buf);
    pdivoom_read_image_handle->next_file_info.read_size = 0;
    pdivoom_read_image_handle->next_cache_handle = back_cache_buf;
}


/**
* NAME       divoom_read_image_read_flash
* @BRIEF	从文件中读取数据到缓冲区中
* @PARAM	pfile_info
            file_handle
            buf_cache
* @RETURN	none
* @RETVAL
*/

void divoom_read_image_read_flash(Pdivoom_read_image_file_info pfile_info, FILE *file_handle, void *buf_cache)
{
    int read_size = DIVOOM_READ_IMAGE_EACH_BUF_SIZE;
    
    if(pfile_info->file_size > DIVOOM_READ_IMAGE_GIF_BUF_SIZE)
    {
        if(pfile_info->read_size == pfile_info->file_size)
        {
            divoom_port_file_seek(file_handle, pfile_info->file_off, SEEK_SET);
            pfile_info->read_size = 0;            
        }
    }
    else if(pfile_info->read_size >= pfile_info->file_size)
    {
        //数据已经全部读完了
        return;
    }
    
    if(pfile_info->read_size + DIVOOM_READ_IMAGE_EACH_BUF_SIZE > pfile_info->file_size)
    {
        read_size = pfile_info->file_size - pfile_info->read_size;
    }
    read_size = divoom_port_file_read(pdivoom_read_image_handle->pic_buf, 1, read_size, file_handle);
    if(read_size)
    {
        divoom_cyc_buf_write(buf_cache, pdivoom_read_image_handle->pic_buf, read_size);
        pfile_info->read_size += read_size;
    }
}


/**
* NAME       divoom_read_image_main_loop
* @BRIEF	主循环调用，用来读数据到缓冲区中
* @PARAM	
* @RETVAL
*/

void divoom_read_image_main_loop(void)
{
    if(pdivoom_read_image_handle == NULL)
    {
        return;
    }
    divoom_system_get_lock(pdivoom_read_image_handle->lock);
    if(divoom_cyc_buf_get_free_size(pdivoom_read_image_handle->cur_cache_handle) > DIVOOM_READ_IMAGE_EACH_BUF_SIZE &&
        (!(pdivoom_read_image_handle->cur_file_info.file_size < DIVOOM_READ_IMAGE_GIF_BUF_SIZE && 
        pdivoom_read_image_handle->cur_file_info.file_size == pdivoom_read_image_handle->cur_file_info.read_size)))
    {
        //当前缓冲区足够，并且还有数据没有读出来
        divoom_read_image_read_flash(&pdivoom_read_image_handle->cur_file_info, pdivoom_read_image_handle->cur_file_handle, pdivoom_read_image_handle->cur_cache_handle);
    }
    else if(pdivoom_read_image_handle->next_cache_handle && divoom_cyc_buf_get_free_size(pdivoom_read_image_handle->next_cache_handle) > DIVOOM_IMAGE_ENCODE_MAX_SIZE &&
        pdivoom_read_image_handle->next_file_info.file_size > pdivoom_read_image_handle->next_file_info.read_size)
    {
        divoom_read_image_read_flash(&pdivoom_read_image_handle->next_file_info, pdivoom_read_image_handle->next_file_handle, pdivoom_read_image_handle->next_cache_handle);
    }
    divoom_system_free_lock(pdivoom_read_image_handle->lock);
}

/**
* NAME       divoom_read_image_exit
* @BRIEF	退出，并释放资源
* @PARAM	
* @RETURN	none
* @RETVAL
*/

void divoom_read_image_exit(void)
{
    if(pdivoom_read_image_handle == NULL)
    {
        return;
    }
    divoom_system_get_lock(pdivoom_read_image_handle->lock);
    if(pdivoom_read_image_handle->cur_file_handle)
    {
        divoom_port_file_close(pdivoom_read_image_handle->cur_file_handle);
    }
    pdivoom_read_image_handle->cur_file_handle = NULL;
    if(pdivoom_read_image_handle->next_file_handle)
    {
        divoom_port_file_close(pdivoom_read_image_handle->next_file_handle);
    }
    pdivoom_read_image_handle->next_file_handle = NULL;
    if(pdivoom_read_image_handle->cur_cache_handle)
    {
       divoom_cyc_buf_exit(pdivoom_read_image_handle->cur_cache_handle);
    }
    pdivoom_read_image_handle->cur_cache_handle = NULL;
    if(pdivoom_read_image_handle->next_cache_handle)
    {
       divoom_cyc_buf_exit(pdivoom_read_image_handle->next_cache_handle);
    }
    pdivoom_read_image_handle->next_cache_handle = NULL;

    divoom_system_destroy_lock(pdivoom_read_image_handle->lock);

    divoom_system_free(pdivoom_read_image_handle);
    pdivoom_read_image_handle = NULL;
}
#endif


