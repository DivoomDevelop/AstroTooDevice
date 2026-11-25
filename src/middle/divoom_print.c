#include "divoom.h"
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/**
 * *  @brief       	divoom_print
 * *  @date       	2014-10-28
 * *  @param[in] 	char * fmt, 可变参数
 * *  @return       void
 * */

#if 1
#define LOG_FILE_EACH_MAX (2*1024)
#define LOG_FILE_SIZE_MAX (7*1024/2)
#define LOG_FILE_UPLOAD_TIME (10)

typedef struct _divoom_log_ctrl
{
    int    debug_size;
    int     cache_size;     //缓冲区数据大小
    int    loop_num;
    int     upload_time;    //上一次上传文件的时间
    FILE    *file_handle;
    void    *lock_handle;
    char    *buf;
    char    topic[64];
    char    out_buf[LOG_FILE_EACH_MAX + 1];
}divoom_log_ctrl, *Pdivoom_log_ctrl;

Pdivoom_log_ctrl pdivoom_log_handle = NULL;

void divoom_log_refresh(void)
{
    if(pdivoom_log_handle && pdivoom_log_handle->file_handle)
    {
        #ifdef DIVOOM_SIMULATOR
        fflush(pdivoom_log_handle->file_handle);
        int fd = fileno(pdivoom_log_handle->file_handle);
        fsync(fd);
        #endif
    }
}

void divoom_log_start(void)
{
    //    return;
    if(pdivoom_log_handle)
    {
        return;
    }
    pdivoom_log_handle = divoom_system_malloc(sizeof(divoom_log_ctrl));
#ifdef DIVOOM_SIMULATOR
    pdivoom_log_handle->file_handle = divoom_port_file_open(DIVOOM_DEBUG_FILE_NAME, "wt");
#endif
    pdivoom_log_handle->lock_handle = divoom_system_create_lock();
    pdivoom_log_handle->debug_size = 0;
    divoom_print_err("start log info!\n");
}

uint8 *divoom_log_get_buffer(int *buf_size)
{
    if(pdivoom_log_handle == NULL)
    {
        return NULL;
    }
    *buf_size = LOG_FILE_EACH_MAX;
    return (uint8 *)pdivoom_log_handle->out_buf;
}

void divoom_log_close(void)
{
    if (pdivoom_log_handle) {
        /* 与 log_write 同锁，等待在途输出结束后关闭 userdata 日志句柄。 */
        divoom_system_get_lock_direct(pdivoom_log_handle->lock_handle);
        if (pdivoom_log_handle->file_handle) {
            divoom_port_file_close(pdivoom_log_handle->file_handle);
            pdivoom_log_handle->file_handle = NULL;
        }
        divoom_system_free_lock_direct(pdivoom_log_handle->lock_handle);
    }
}

void divoom_log_write(const char* tag, const char* fmt, ...)
{
    char *buf;
    va_list ap; 
    int cur_size;
    
    if(pdivoom_log_handle == NULL)
    {
        return;
    }
    divoom_system_get_lock_direct(pdivoom_log_handle->lock_handle);
    pdivoom_log_handle->debug_size += strlen(tag);
#ifdef DIVOOM_SIMULATOR
    /* 文件日志是可选输出端；多实例占用或目录异常时，日志失败不能拖垮主程序。 */
    if(pdivoom_log_handle->file_handle)
    {
        fputs(tag, pdivoom_log_handle->file_handle);
    }
#endif
    buf= pdivoom_log_handle->out_buf;
    
    va_start(ap, fmt);
    vsnprintf(buf, LOG_FILE_EACH_MAX, fmt, ap);
    va_end(ap);
    pdivoom_log_handle->debug_size += strlen(buf);

#ifdef DIVOOM_SIMULATOR
    if(pdivoom_log_handle->file_handle)
    {
        fputs(buf, pdivoom_log_handle->file_handle);
        /* 仿真器：每条日志显式 fflush 但不 fsync，既能让日志及时出现，又避免 _IONBF
         * 在多线程并发 printf 时把性能拖跨 */
        fflush(pdivoom_log_handle->file_handle);
    }
#endif

    //debug文件记录最大１０ｍ
    #if 1
    if(pdivoom_log_handle->debug_size > 10*1024*1024)
    {
        #ifdef DIVOOM_SIMULATOR
        if(pdivoom_log_handle->file_handle)
        {
            divoom_port_file_seek(pdivoom_log_handle->file_handle, 0, SEEK_SET);
            pdivoom_log_handle->loop_num ++;
            fprintf(pdivoom_log_handle->file_handle,"loop number: %d,\n\n", pdivoom_log_handle->loop_num);
        }
        #endif
        pdivoom_log_handle->debug_size = 0;
    }
    #endif
    
    if(divoom_main_update_log_level(-1))
    {
        cur_size = strlen(tag) +  strlen(buf);

        if(pdivoom_log_handle->buf == NULL)
        {
            pdivoom_log_handle->buf = divoom_system_malloc(LOG_FILE_SIZE_MAX + 1);
        }
        if(pdivoom_log_handle->cache_size + cur_size > LOG_FILE_SIZE_MAX ||
            (pdivoom_log_handle->upload_time < time(0)))
        {
            //将发送到服务器
            if(pdivoom_log_handle->cache_size)
            {
                if(strlen(pdivoom_log_handle->topic) == 0)
                {
                    sprintf(pdivoom_log_handle->topic, "divoom/2/%d/log", divoom_main_get_device_id());
                }
                divoom_mqtt_model_send_data(MQTT_SER, (uint8_t *)pdivoom_log_handle->topic, (uint8_t *)pdivoom_log_handle->buf, strlen(pdivoom_log_handle->buf));
            }
            pdivoom_log_handle->cache_size = 0;
            pdivoom_log_handle->upload_time = time(0) + LOG_FILE_UPLOAD_TIME;
        }
        strcpy(&pdivoom_log_handle->buf[pdivoom_log_handle->cache_size], tag);
        pdivoom_log_handle->cache_size += strlen(tag);

        if(pdivoom_log_handle->cache_size + strlen(buf)> LOG_FILE_SIZE_MAX)
        {
            strncpy(&pdivoom_log_handle->buf[pdivoom_log_handle->cache_size], buf, LOG_FILE_SIZE_MAX - pdivoom_log_handle->cache_size - 1);
            divoom_mqtt_model_send_data(MQTT_SER, (uint8_t *)pdivoom_log_handle->topic, (uint8_t *)pdivoom_log_handle->buf, strlen(pdivoom_log_handle->buf));
            pdivoom_log_handle->cache_size = 0;
        }
        else
        {
            strcpy(&pdivoom_log_handle->buf[pdivoom_log_handle->cache_size], buf);
            pdivoom_log_handle->cache_size += strlen(buf);
        }
    }
    
    divoom_system_free_lock_direct(pdivoom_log_handle->lock_handle);
    
}

void divoom_print_buf(uint8 *data, uint16 len)
{
    uint16 i;    
    divoom_print("divoom_print_buf: len: %d\n", len);
    for(i = 0; i < len; i++)
    {
        divoom_print("%02x:%02x ,\n", i, data[i]);
    }
    divoom_print("\n");
}



#endif
