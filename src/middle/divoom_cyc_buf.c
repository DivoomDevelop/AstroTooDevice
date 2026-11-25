#include "divoom.h"

typedef struct _divoom_cyc_buf_ctrl
{
    int read_pos;       //数据读的位置
    int write_pos;      //数据写的位置
    int buf_size;      //数据缓冲区大小
    int data_size;      //数据缓冲区有效数据长度
    void  *lock;      //
    uint8 *buf;      //数据缓冲区
}divoom_cyc_buf_ctrl, *Pdivoom_cyc_buf_ctrl;


/**
* NAME         divoom_cyc_buf_init
* @BRIEF	创建环形缓冲区
* @PARAM	buf_size
* @RETURN	返回句柄
* @RETVAL
*/

void *divoom_cyc_buf_init(int buf_size)
{
    Pdivoom_cyc_buf_ctrl pcyc_handle;

    pcyc_handle = divoom_system_malloc(sizeof(divoom_cyc_buf_ctrl) + buf_size);
    if(pcyc_handle == NULL)
    {
        return NULL;
    }
    pcyc_handle->buf = (uint8 *)&pcyc_handle[1];
    pcyc_handle->buf_size = buf_size;
    pcyc_handle->lock = divoom_system_create_lock();
    return pcyc_handle;
}

/**
* NAME       divoom_cyc_buf_get_clear_data
* @BRIEF	清除所有数据
* @PARAM	handle
* @RETURN	none
* @RETVAL
*/

void divoom_cyc_buf_clear_data(void *handle)
{
    Pdivoom_cyc_buf_ctrl pcyc_handle = (Pdivoom_cyc_buf_ctrl)handle;

    if(pcyc_handle == NULL)
    {
        return;
    }
    divoom_system_get_lock(pcyc_handle->lock);
    pcyc_handle->data_size = 0;
    pcyc_handle->write_pos = 0;
    pcyc_handle->read_pos = 0;
    divoom_system_free_lock(pcyc_handle->lock);
}

/**
* NAME         divoom_cyc_buf_read
* @BRIEF	读缓冲区数据
* @PARAM	handle
            buf
            size
* @RETURN	数据长度
* @RETVAL
*/

int divoom_cyc_buf_read(void *handle, uint8 *buf, int size)
{
    Pdivoom_cyc_buf_ctrl pcyc_handle = (Pdivoom_cyc_buf_ctrl)handle;

    if(pcyc_handle == NULL)
    {
        return 0;
    }

    if(pcyc_handle->data_size == 0)
    {
        return 0;
    }
    divoom_system_get_lock(pcyc_handle->lock);
    if(size > pcyc_handle->data_size)
    {
        size = pcyc_handle->data_size;
    }
    if(pcyc_handle->read_pos + size > pcyc_handle->buf_size)
    {
        memcpy(buf, &pcyc_handle->buf[pcyc_handle->read_pos], pcyc_handle->buf_size - pcyc_handle->read_pos);
        memcpy(&buf[pcyc_handle->buf_size - pcyc_handle->read_pos], pcyc_handle->buf, size - (pcyc_handle->buf_size - pcyc_handle->read_pos));
        pcyc_handle->read_pos += size - pcyc_handle->buf_size;
    }
    else
    {
        memcpy(buf, &pcyc_handle->buf[pcyc_handle->read_pos], size);
        pcyc_handle->read_pos += size;
    }
    pcyc_handle->data_size -= size;
    divoom_system_free_lock(pcyc_handle->lock);
    return size;
}


/**
* NAME         divoom_cyc_buf_write
* @BRIEF	写缓冲区数据
* @PARAM	handle
            buf
            size
* @RETURN	数据长度
* @RETVAL
*/

int divoom_cyc_buf_write(void *handle, uint8 *buf, int size)
{
    Pdivoom_cyc_buf_ctrl pcyc_handle = (Pdivoom_cyc_buf_ctrl)handle;

    if(pcyc_handle == NULL)
    {
        return 0;
    }
    if(pcyc_handle->data_size >= pcyc_handle->buf_size)
    {
        return 0;
    }
    divoom_system_get_lock(pcyc_handle->lock);
    if(pcyc_handle->data_size + size >= pcyc_handle->buf_size)
    {
        size = pcyc_handle->buf_size - pcyc_handle->data_size;
    }
    if(pcyc_handle->write_pos + size > pcyc_handle->buf_size)
    {
        memcpy(&pcyc_handle->buf[pcyc_handle->write_pos], buf, pcyc_handle->buf_size - pcyc_handle->write_pos);
        memcpy(pcyc_handle->buf, &buf[pcyc_handle->buf_size - pcyc_handle->write_pos], size - (pcyc_handle->buf_size - pcyc_handle->write_pos));
        pcyc_handle->write_pos = pcyc_handle->write_pos + size - pcyc_handle->buf_size;
    }
    else
    {
        memcpy(&pcyc_handle->buf[pcyc_handle->write_pos], buf, size);
        pcyc_handle->write_pos += size;
    }
    pcyc_handle->data_size += size;
    divoom_system_free_lock(pcyc_handle->lock);
    return size;
}


/**
* NAME         divoom_cyc_buf_get_free_size
* @BRIEF	得到缓冲区剩余空间大小
* @PARAM	handle
* @RETURN	空间大小
* @RETVAL
*/

int divoom_cyc_buf_get_free_size(void *handle)
{
    Pdivoom_cyc_buf_ctrl pcyc_handle = (Pdivoom_cyc_buf_ctrl)handle;

    if(pcyc_handle == NULL)
    {
        return 0;
    }
    return pcyc_handle->buf_size - pcyc_handle->data_size;
}


/**
* NAME       divoom_cyc_buf_get_reset_data_size
* @BRIEF	在数据小于最大缓冲时，可以将缓冲复位
* @PARAM	handle
* @RETURN	none
* @RETVAL
*/

int divoom_cyc_buf_get_reset_data_size(void *handle)
{
    Pdivoom_cyc_buf_ctrl pcyc_handle = (Pdivoom_cyc_buf_ctrl)handle;

    if(pcyc_handle == NULL)
    {
        return 0;
    }
    divoom_system_get_lock(pcyc_handle->lock);
    pcyc_handle->data_size = pcyc_handle->write_pos;
    pcyc_handle->read_pos = 0;
    divoom_system_free_lock(pcyc_handle->lock);
    return pcyc_handle->data_size;
}


/**
* NAME         divoom_cyc_buf_get_data_size
* @BRIEF	得到缓冲区有效数据空间大小
* @PARAM	handle
* @RETURN	空间大小
* @RETVAL
*/

int divoom_cyc_buf_get_data_size(void *handle)
{
    Pdivoom_cyc_buf_ctrl pcyc_handle = (Pdivoom_cyc_buf_ctrl)handle;

    if(pcyc_handle == NULL)
    {
        return 0;
    }
    return pcyc_handle->data_size;
}


/**
* NAME         divoom_cyc_buf_exit
* @BRIEF	释放句柄
* @PARAM	handle
* @RETURN	
* @RETVAL
*/

void divoom_cyc_buf_exit(void *handle)
{
    Pdivoom_cyc_buf_ctrl pcyc_handle = (Pdivoom_cyc_buf_ctrl)handle;
    
    if(handle)
    {
        divoom_system_destroy_lock(pcyc_handle->lock);
        divoom_system_free(handle);
    }
}



