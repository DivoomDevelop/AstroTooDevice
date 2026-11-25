#include "divoom.h"


#define DIVOOM_WHITE_NOISE_MAX_VOICE_NUM  8
#define DIVOOM_WHITE_NOISE_VOICE_BUF_SIZE 1024
#define DIVOOM_WHITE_NOISE_CACHE_NUM 20
const char *divoom_white_noise_file_name_table[] =
{
    "1_fengce.pcm",
    "2_frog_sound.pcm",
    "3_firewood.pcm",
    "4_sea_wave.pcm",
    "5_rain_sound.pcm",
    "6_stream.pcm",
    "7_bird_sound.pcm",
    "8_song_bowl.pcm",
};
typedef struct _divoom_white_noise_voice_item
{
    uint8  voice_buf[DIVOOM_WHITE_NOISE_MAX_VOICE_NUM][DIVOOM_WHITE_NOISE_VOICE_BUF_SIZE];
}divoom_white_noise_voice_item, *Pdivoom_white_noise_voice_item;

typedef struct _divoom_white_noise_ctrl_ctrl
{
    void *lock;
    void *file_handle[DIVOOM_WHITE_NOISE_MAX_VOICE_NUM];
    int voice_table[DIVOOM_WHITE_NOISE_MAX_VOICE_NUM];
    void *pcm_queue;
}divoom_white_noise_ctrl_ctrl, *Pdivoom_white_noise_ctrl_ctrl;

/**
* NAME         divoom_white_noise_ctrl_read_file
               循环读一个文件内容，目前只考虑最多读两次
* @PARAM       file_handle
               file_buf
               file_size
* @RETURN	none
* @RETVAL
*/

void divoom_white_noise_ctrl_read_file(void *file_handle, uint8 *file_buf, int file_size)
{
    int read_size, ret;

    read_size = divoom_port_file_read(file_buf, 1, file_size, file_handle);
    if(file_size > read_size)
    {
        divoom_port_file_seek(file_handle, 0, SEEK_SET);
        ret = divoom_port_file_read(&file_buf[read_size], 1, file_size - read_size, file_handle);
    }
}
/**
* NAME         divoom_white_noise_ctrl_read_loop
               主循环读所有音量大于0的文件内容
* @PARAM       data
* @RETURN	none
* @RETVAL
*/


void divoom_white_noise_ctrl_read_loop(void *data)
{
    int i;
    Pdivoom_white_noise_ctrl_ctrl phandle = (Pdivoom_white_noise_ctrl_ctrl)data;
    Pdivoom_white_noise_voice_item pbuf_item;

    if(divoom_queue_is_full(phandle->pcm_queue))
    {
        return;
    }
    pbuf_item = divoom_system_malloc(sizeof(divoom_white_noise_voice_item));
    if(pbuf_item == NULL)
    {
		divoom_print("%s,malloc fail\n",__func__);
        return;
    }
    divoom_system_get_lock(phandle->lock);
    for(i = 0; i < DIVOOM_WHITE_NOISE_MAX_VOICE_NUM; i++)
    {
        if(phandle->file_handle[i])
        {
            divoom_white_noise_ctrl_read_file(phandle->file_handle[i], pbuf_item->voice_buf[i],DIVOOM_WHITE_NOISE_VOICE_BUF_SIZE);
        }
        else
        {
            memset(pbuf_item->voice_buf[i], 0, DIVOOM_WHITE_NOISE_VOICE_BUF_SIZE);
        }
    }
    divoom_queue_push(phandle->pcm_queue, pbuf_item);
    divoom_system_free_lock(phandle->lock);
}

/**
* NAME         divoom_white_noise_ctrl_init
               初始化句柄
* @PARAM       vol_table        所有音源的音量

* @RETURN	   handle
* @RETVAL
*/

void *divoom_white_noise_ctrl_init(int *vol_table)
{
    int i;
    char file_name[128];
    Pdivoom_white_noise_ctrl_ctrl phandle;

    phandle = divoom_system_malloc(sizeof(divoom_white_noise_ctrl_ctrl));

    phandle->lock = divoom_system_create_lock();
	if(vol_table)
	{
		memcpy(phandle->voice_table, vol_table, sizeof(phandle->voice_table));
	}
	else
	{
		memset(phandle->voice_table, 0, sizeof(phandle->voice_table));
	}
    for(i = 0; i < DIVOOM_WHITE_NOISE_MAX_VOICE_NUM; i++)
    {
        strcpy(file_name, "/usr/share/divoom_app/tone/");
        strcat(file_name, divoom_white_noise_file_name_table[i]);
        phandle->file_handle[i] = divoom_port_file_open(file_name, "rb");
		if(NULL == phandle->file_handle[i])
		{
			divoom_print("%s,open file fail:%s",__func__,file_name);
		}
    }
    phandle->pcm_queue = divoom_queue_init(DIVOOM_WHITE_NOISE_CACHE_NUM, __func__, __LINE__);
    //先将缓冲区写到一到再启动播放
    for(i = 0; i < DIVOOM_WHITE_NOISE_CACHE_NUM/2; i++)
    {
        divoom_white_noise_ctrl_read_loop(phandle);
    }
    return phandle;
}


/**
* NAME         divoom_white_noise_ctrl_update_volume
               更新音量
* @PARAM       vol_table        所有音源的音量
               handle
* @RETURN	none
* @RETVAL
*/

void divoom_white_noise_ctrl_update_volume(void *handle,  int *vol_table)
{
    Pdivoom_white_noise_ctrl_ctrl phandle = (Pdivoom_white_noise_ctrl_ctrl)handle;

    if(phandle == NULL)
    {
        return;
    }

    divoom_system_get_lock(phandle->lock);
    memcpy(phandle->voice_table, vol_table, sizeof(phandle->voice_table));
    divoom_system_free_lock(phandle->lock);
}

int divoom_white_noise_ctrl_is_queue_not_empty(void *handle)
{
    Pdivoom_white_noise_ctrl_ctrl phandle = (Pdivoom_white_noise_ctrl_ctrl)handle;
    if(phandle == NULL)
    {
        return 0;
    }
	return divoom_queue_not_empty(phandle->pcm_queue);
}

/**
* NAME         divoom_white_noise_ctrl_read_pcm
               读数据，每次读的数据量必须是以K为单位
* @PARAM       handle
               read_buf
               read_size
* @RETURN	读到的数据量
* @RETVAL
*/

int divoom_white_noise_ctrl_read_pcm(void *handle,  uint8 *read_buf, int read_size)
{
    T_S16 *dest_pcm, *src_pcm;
    int i, j, real_size = 0;
    Pdivoom_white_noise_ctrl_ctrl phandle = (Pdivoom_white_noise_ctrl_ctrl)handle;
    Pdivoom_white_noise_voice_item pbuf_item;

    if(phandle == NULL)
    {
        return 0;
    }

    divoom_system_get_lock(phandle->lock);
    while(real_size + DIVOOM_WHITE_NOISE_VOICE_BUF_SIZE <= read_size)
    {
        pbuf_item = divoom_queue_pop(phandle->pcm_queue);
        if(pbuf_item == NULL)
        {
            break;
        }
        dest_pcm = (T_S16 *)&read_buf[real_size];
        for(j = 0; j < DIVOOM_WHITE_NOISE_VOICE_BUF_SIZE/sizeof(T_S16); j ++)
        {
            T_S32 sum = 0;

            for(i = 0; i < DIVOOM_WHITE_NOISE_MAX_VOICE_NUM; i++)
            {
                if(phandle->voice_table[i])
                {
                    src_pcm = (T_S16 *)pbuf_item->voice_buf[i];
                    sum += (T_S32)src_pcm[j] * phandle->voice_table[i] / 100;
                }
            }
            if (sum > 32767)
            {
                sum = 32767;
            }
            else if (sum < -32768)
            {
                sum = -32768;
            }
            dest_pcm[j] = (T_S16)sum;
        }
		divoom_system_free(pbuf_item);
        real_size += DIVOOM_WHITE_NOISE_VOICE_BUF_SIZE;
    }
    divoom_system_free_lock(phandle->lock);
    return real_size;
}


/**
* NAME         divoom_white_noise_ctrl_exit
               退出并释放所有资源
* @PARAM       handle
* @RETURN	   none
* @RETVAL
*/

void divoom_white_noise_ctrl_exit(void *handle)
{
    int i;
    Pdivoom_white_noise_ctrl_ctrl phandle = (Pdivoom_white_noise_ctrl_ctrl)handle;

    if(phandle == NULL)
    {
        return;
    }
    divoom_system_get_lock(phandle->lock);
    for(i = 0; i < DIVOOM_WHITE_NOISE_MAX_VOICE_NUM; i++)
    {
        if(phandle->file_handle[i])
        {
            divoom_port_file_close(phandle->file_handle[i]);
        }
    }
    divoom_queue_destroy(phandle->pcm_queue, NULL);
    divoom_system_destroy_lock(phandle->lock);
    divoom_system_free(phandle);

}



