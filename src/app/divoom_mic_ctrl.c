#include "divoom.h"

#if 0
/*
PCM数据是有符号型，使用到I2s麦MSM261S4030H0R输出为24位有符号数据包含在32位的帧中，为小端模式，数据为补码形式存在
ESP32得到的数据样例 00 00 00 00 00 5F DA F7 00 00 00 00 00 DC AE FB
可以看有24位的有效位，要将其转成通用的S16LE
涉及到有符号型数据转换问题，要保持PCM波形
int24 -> int16

步骤：1. 先得到int32的数据
	  2. 按比例转成int16    即(pcm32_value>>16)
	  3. 使用一个int16变量存下pcm16_value = (int16)pcm32_value
*/


#define DIVOOM_MIC_CTRL_PCM_SAVE_BUF_SIZE	(1000*1024)


#define DIVOOM_MIC_CTRL_BUF_SIZE		(1024)
#define DIVOOM_MIC_CTRL_REAL_BUF_SIZE	(DIVOOM_MIC_CTRL_BUF_SIZE/2)
//#define DIVOOM_MIC_I2S_SAMPLERATE		(8000)
#define DIVOOM_MIC_I2S_SAMPLERATE		(32000)
#define DIVOOM_MIC_I2S_NUM				(0)

//#define DIVOOM_MIC_GET_DATA

typedef struct _divoom_mic_ctrl_str_info
{
    char *mic_buf;
    char *real_mic_data;
	char *pcm_buf;			//存储pcm数据s
	uint8 pcm_save_flag;	//进行保存的标志
	uint32 pcm_size;
    void  *read_mic_sem;
    void  *mic_lock_handle;
	uint8 run_flag;
	uint8 stop_flag;
	DIVOOM_MIC_CTRL_SEND_DATA *cb;
#ifdef DIVOOM_MIC_GET_DATA
	//for test
//	int time;
#endif
}divoom_mic_ctrl_str_info, *Pdivoom_mic_ctrl_str_info;

Pdivoom_mic_ctrl_str_info pdivoom_mic_ctrl_handle = NULL;

static void divoom_mic_ctrl_mic_en(uint8 en)
{
    if(DIVOOM_I2S_CHIP_EN == -1)
    {
        return;
    }
	gpio_set_level(DIVOOM_I2S_CHIP_EN, (en>0?1:0));
}




//读出MIC的数据
int divoom_mic_ctrl_read_mic_data(char *mic_buf, uint32 buf_len)
{
	if(0 == i2s_read(DIVOOM_MIC_I2S_NUM, (void *)mic_buf, buf_len, &buf_len,portMAX_DELAY))
	{
        return buf_len;
	}
    return 0;
}


/**
$ ffmpeg -formats | grep PCM
 DE alaw            PCM A-law
 DE f32be           PCM 32-bit floating-point big-endian
 DE f32le           PCM 32-bit floating-point little-endian
 DE f64be           PCM 64-bit floating-point big-endian
 DE f64le           PCM 64-bit floating-point little-endian
 DE mulaw           PCM mu-law
 DE s16be           PCM signed 16-bit big-endian
 DE s16le           PCM signed 16-bit little-endian
 DE s24be           PCM signed 24-bit big-endian
 DE s24le           PCM signed 24-bit little-endian
 DE s32be           PCM signed 32-bit big-endian
 DE s32le           PCM signed 32-bit little-endian
 DE s8              PCM signed 8-bit
 DE u16be           PCM unsigned 16-bit big-endian
 DE u16le           PCM unsigned 16-bit little-endian
 DE u24be           PCM unsigned 24-bit big-endian
 DE u24le           PCM unsigned 24-bit little-endian
 DE u32be           PCM unsigned 32-bit big-endian
 DE u32le           PCM unsigned 32-bit little-endian
 DE u8              PCM unsigned 8-bit
**/

/*
int16_t pcm[1024] = read in some pcm data;
int32_t pcmval;
for (ctr = 0; ctr < 1024; ctr++) {
    pcmval = pcm[ctr] * 2;
    if (pcmval < 32767 && pcmval > -32768) {
        pcm[ctr] = pcmval
    } else if (pcmval > 32767) {
        pcm[ctr] = 32767;
    } else if (pcmval < -32768) {
        pcm[ctr] = -32768;
    }
}
*/



/*****************************************************************************
 Prototype    : divoom_mic_ctrl_format_s24_to_s16
 Description  : s24 in 32bit转s16 PCM
 Input        : uint8 *out_buf  
                uint8 *in_buf   
                uint32 in_len   
 Output       : None
 Return Value : out_buf len
 
  History        :
  1.Date         : 2021/1/21
    Author       : coronaa
    Modification : Created function
*****************************************************************************/
uint32 divoom_mic_ctrl_format_s24_to_s16(uint8 *out_buf, uint8 *in_buf, uint32 in_len)
{
	uint32 i,count = (in_len>>3);	//In a stereo frame there are 64 SCK cycles 8byte
	int32 *tmp_buf = (int32 *)in_buf,pcms32_value;
	int16 pcms16_value;
	for(i = 0; i < count;i++)
	{
		pcms32_value = tmp_buf[(i<<1)+1];	//只有一个声道有数据，根据实际选声道
		pcms32_value>>=16;
		//数据转成S16后 乘系数
//		pcms32_value>>=(16-5);
		//采样点的数据乘以扩大系数之后的值 小于 -32768 或 大于 32768
		if (pcms32_value > 32767) 
		{
			pcms32_value = 32767;
		}
		else if (pcms32_value < -32768) 
		{
			pcms32_value = -32768;
		}

		pcms16_value = (int16_t)pcms32_value;
		memcpy(out_buf + 2*i,&pcms16_value,2);
	}
	return count*2;
}




void divoom_mic_ctrl_pcm_s16_expand(int16 *pcm_out_buf,int16 *pcm_buf, uint32 pcm_len,int16 ratio)
{
	int16 pcms16_value;
	int32 pcms32_value;
	uint32 i;
	if(NULL == pcm_out_buf)
		pcm_out_buf = pcm_buf;

	for(i = 0;i < pcm_len; i++)
	{
		pcms32_value = pcm_buf[i];
		pcms32_value *= ratio;
		//采样点的数据乘以扩大系数之后的值 小于 -32768 或 大于 32768
		if (pcms32_value > 32767) 
		{
			pcms32_value = 32767;
		}
		else if (pcms32_value < -32768) 
		{
			pcms32_value = -32768;
		}else
		{
			continue;
		}
		pcms16_value = pcms32_value;
		pcm_out_buf[i] = pcms16_value;
	}
}


void divoom_mic_ctrl_read_main(void *param)
{
    int ret;
	Pdivoom_mic_ctrl_str_info task_handle = (Pdivoom_mic_ctrl_str_info)param;
    
    while(1)
	{
#if 0
		divoom_i2s_mems_loop();
		divoom_system_delay_ms(10);

#else
		divoom_system_get_sem(task_handle->read_mic_sem);
		task_handle->stop_flag = 1;
		while(task_handle->run_flag)
		{
			divoom_system_get_lock(task_handle->mic_lock_handle);
			memset(task_handle->mic_buf, 0, sizeof(char)*DIVOOM_MIC_CTRL_BUF_SIZE);
			ret = divoom_mic_ctrl_read_mic_data(task_handle->mic_buf, DIVOOM_MIC_CTRL_BUF_SIZE);
            if(ret > 0)
            {
                if(divoom_main_get_mic_switch())
                {
                    memset(task_handle->mic_buf, 0, ret);
                }
            }
			if(ret < 0)
			{
				//读取不到内容
				divoom_system_free_lock(task_handle->mic_lock_handle);
				ESP_LOGI("MIC", "Cat't read data in %s-%d, ret = %d", __func__, __LINE__, ret);
				if(task_handle->run_flag > 0)
				{
					divoom_system_delay_ms(200);
					continue;
				}
				else
				{
					break;
				}
			}
			//if(strlen((char *)task_handle->mic_buf) > 0)
			else
			{
				//对数据进行格式化
				
				int read_len = divoom_mic_ctrl_format_s24_to_s16((uint8 *)task_handle->real_mic_data,(uint8 *)task_handle->mic_buf,ret);
#ifdef DIVOOM_MIC_GET_DATA
				if(divoom_sd_check_state() == DIVOOM_SD_MOUNT_OK)
				{
					char *mic_buf_tmp = task_handle->real_mic_data;
					//写一个固定大小的文件
					if(task_handle->pcm_size < DIVOOM_MIC_CTRL_PCM_SAVE_BUF_SIZE)
					{
						if(ret != DIVOOM_MIC_CTRL_BUF_SIZE)
						{
							divoom_print("%d\n",ret);
						}
						if(read_len + task_handle->pcm_size >= DIVOOM_MIC_CTRL_PCM_SAVE_BUF_SIZE)
						{
							read_len = DIVOOM_MIC_CTRL_PCM_SAVE_BUF_SIZE - task_handle->pcm_size;
						}
						memcpy(task_handle->pcm_buf+task_handle->pcm_size,mic_buf_tmp,read_len);
						task_handle->pcm_size+=read_len;
						task_handle->pcm_save_flag = 1;
					}
					else
					{
						if(task_handle->pcm_save_flag)
						{
							//只存一次
							task_handle->pcm_save_flag  = 0;
							FILE *handle = divoom_port_file_open(DIVOOM_LEGACY_SD_ROOT "/voice.pcm", "wb+");
							if(handle != NULL)
							{
								divoom_port_file_write(task_handle->pcm_buf, task_handle->pcm_size, 1, handle);
								divoom_port_file_close(handle);
							}
							divoom_print(DIVOOM_LEGACY_SD_ROOT "/voice.pcm save finish!");
						}

					}

				}
#endif

#if 0		//测试1S读到的数据量
				static uint32 first_in_flag = 0;
				static uint32 starttime = 0;
				static uint32 data_count = 0;
				uint32 cur_time;
				if(first_in_flag<100)
				{
					first_in_flag++;
					starttime = divoom_system_get_tickms();
//					divoom_print("start:%d\n",starttime);
				}
				else
				{
					cur_time = divoom_system_get_tickms();

					if(cur_time - starttime < 1000)
					{
						data_count += read_len;
						task_handle->pcm_save_flag = 1;
					}
					else
					{
						if(task_handle->pcm_save_flag)
						{
							//只存一次
							task_handle->pcm_save_flag	= 0;
							divoom_print("coronaa 1s pcm count finish:0x%x",data_count);
						}
					}
				}
#endif
				//先根据左右声道把数据整合出来.因为作为32个clk左声道的话,声道选左声道
				//根据datesheet287页表60,I2S_RX_MSB_RIGHT=0 I2S_RX_CHAN_MOD=2 I2S_RX_FIFO_MOD=3,数据是右声道+右声道,刚好硅麦那边也是右声道,感觉是乐鑫这边哪里是反的
				//这样的话,再根据乐鑫这边给到的数据是(8个字节) 右声道高位+右声道低位+右声道高位(无效,没有接)+右声道低位(无效,没有接);所有,每次32位数据(4个8字节)删除后面两位
				//也可以暴力一地,用安凯的方法,把前面的拷贝到后面一份,做成双声道,但是这里只需要单声道,直接拿出来就行了
				//不需要了,错误的,在read的地方已经做好了format
				//ret = divoom_mic_ctrl_format_data(task_handle->mic_buf, ret);
				//ESP_LOGI("MIC", "read data in %s-%d, ret = %d", __func__, __LINE__, ret);

				if(task_handle->cb)
					task_handle->cb((uint8 *)task_handle->real_mic_data, read_len);
			}
			divoom_system_free_lock(task_handle->mic_lock_handle);
			if(task_handle->run_flag > 0)
			{
#ifndef DIVOOM_MIC_GET_DATA
				//延时10ms
				divoom_system_delay_ms(1);
#endif
			}
			else
			{
				break;
			}
		}
		task_handle->run_flag = 0;
		task_handle->stop_flag = 0;
#endif
	}
}

/*********************************************************************
  Function:        divoom_mic_ctrl_init
  Description:    初始化相关信息
  Input:            NONE
  Return:           none
  Author:           albert
**********************************************************************/

void divoom_mic_ctrl_init(void)
{
	i2s_config_t divoom_i2s_config;
    gpio_config_t io_conf;
    i2s_pin_config_t divoom_pin_config;
	if(pdivoom_mic_ctrl_handle)
	{
		return;
	}
    pdivoom_mic_ctrl_handle = divoom_malloc(sizeof(divoom_mic_ctrl_str_info),DIVOOM_MODEL_AD,__LINE__, __FILE__);

    memset(pdivoom_mic_ctrl_handle, 0, sizeof(divoom_mic_ctrl_str_info));
	memset(&divoom_i2s_config, 0, sizeof(i2s_config_t));
    pdivoom_mic_ctrl_handle->read_mic_sem = divoom_system_create_sem(10, 0);
    pdivoom_mic_ctrl_handle->mic_lock_handle = divoom_system_create_lock();
	pdivoom_mic_ctrl_handle->mic_buf = divoom_malloc(sizeof(char)*DIVOOM_MIC_CTRL_BUF_SIZE, DIVOOM_MODEL_AD, __LINE__, __FILE__);
	pdivoom_mic_ctrl_handle->real_mic_data = divoom_malloc(sizeof(char)*DIVOOM_MIC_CTRL_REAL_BUF_SIZE, DIVOOM_MODEL_AD, __LINE__, __FILE__);

#ifdef DIVOOM_MIC_GET_DATA
	pdivoom_mic_ctrl_handle->pcm_buf =  divoom_malloc(sizeof(char)*DIVOOM_MIC_CTRL_PCM_SAVE_BUF_SIZE, DIVOOM_MODEL_AD, __LINE__, __FILE__);
#endif
	//把数据脚做一个下拉处理
	io_conf.pin_bit_mask = (1ULL << DIVOOM_I2S_DATAIN_IO);
	io_conf.intr_type = GPIO_INTR_DISABLE;
	io_conf.mode = GPIO_MODE_INPUT;
	io_conf.pull_up_en = 0;
	io_conf.pull_down_en = 1;
    gpio_config(&io_conf);

	//init i2s
	divoom_i2s_config.mode = I2S_MODE_MASTER | I2S_MODE_RX;
	divoom_i2s_config.sample_rate = DIVOOM_MIC_I2S_SAMPLERATE;
	//只能是32,因为这个是每个ws周期有32个clk
	divoom_i2s_config.bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT;
	//标准的I2S模式
	divoom_i2s_config.communication_format = I2S_COMM_FORMAT_STAND_I2S;
	//看到时候用左右声道哪一个,和硅麦的左右是反的
	divoom_i2s_config.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT;
	divoom_i2s_config.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;
	//这个buf的大小和数量还没有搞清楚
	//假如给2k,那么读2k读出来,新来的数据存放慢了就要被丢弃了?,暂时给个3k有个1k的缓存吧
	//最大只能到1024
	divoom_i2s_config.dma_buf_len = DIVOOM_MIC_CTRL_BUF_SIZE;
	//buf count至少要有两块
	divoom_i2s_config.dma_buf_count = 4;
	divoom_i2s_config.use_apll = true;
	
	i2s_driver_install(DIVOOM_MIC_I2S_NUM, &divoom_i2s_config, 0, NULL);

	//所有的gpio都可以自己配置
	divoom_pin_config.mck_io_num = -1;
	divoom_pin_config.bck_io_num = DIVOOM_I2S_BCK_IO;
	divoom_pin_config.data_in_num = DIVOOM_I2S_DATAIN_IO;
	divoom_pin_config.data_out_num = DIVOOM_I2S_DATA_OUT_IO;
	divoom_pin_config.ws_io_num = DIVOOM_I2S_WS_IO;
	i2s_set_pin(DIVOOM_MIC_I2S_NUM, &divoom_pin_config);


	//设置EN脚,高电平工作,低电平低功耗
	io_conf.pin_bit_mask = 0; //(1ULL << DIVOOM_I2S_CHIP_EN);
	io_conf.intr_type = GPIO_INTR_DISABLE;
	io_conf.mode = GPIO_MODE_OUTPUT;
	io_conf.pull_up_en = 0;
	io_conf.pull_down_en = 0;
    gpio_config(&io_conf);

//    divoom_system_create_task_prio(divoom_mic_ctrl_read_main, "divoom_mic_ctrl", pdivoom_mic_ctrl_handle, 4*1024,1);
	divoom_system_create_task_pro(divoom_mic_ctrl_read_main, "divoom_mic_ctrl", pdivoom_mic_ctrl_handle, 4*1024, DIVOOM_MIC_CTRL_PRIO, 1, 1);

	return;
}


/*********************************************************************
  Function:        divoom_mic_ctrl_request
  Description:    增加一路MIC数据的请求，如果为编码方式，将停止向其它非编码通路送数据
  Input:            pcallback    数据输出回调函数
                       encode_flag    是否需要编码
  Return:           none
  Author:           albert
**********************************************************************/
//开始请求,打开task,开始读取,独到的数据放到pcallback里面
void divoom_mic_ctrl_request(DIVOOM_MIC_CTRL_SEND_DATA *pcallback)
{
	if(pdivoom_mic_ctrl_handle == NULL)
	{
		return;
	}


	if(pdivoom_mic_ctrl_handle->run_flag)
	{
		pdivoom_mic_ctrl_handle->run_flag = 0;
		while(pdivoom_mic_ctrl_handle->stop_flag == 1)
		{
			divoom_system_delay_ms(1);
		}
	}
	divoom_print("we will start mic!\n");
	divoom_system_get_lock(pdivoom_mic_ctrl_handle->mic_lock_handle);
	if(pcallback)
		pdivoom_mic_ctrl_handle->cb = pcallback;
	memset(pdivoom_mic_ctrl_handle->mic_buf, 0, sizeof(char)*DIVOOM_MIC_CTRL_BUF_SIZE);
	pdivoom_mic_ctrl_handle->run_flag = 1;
#ifdef DIVOOM_MIC_GET_DATA
//	pdivoom_mic_ctrl_handle->time = time(0);
#endif
	divoom_system_free_lock(pdivoom_mic_ctrl_handle->mic_lock_handle);
	divoom_mic_ctrl_mic_en(1);
	divoom_system_put_sem(pdivoom_mic_ctrl_handle->read_mic_sem);
}


/*********************************************************************
  Function:        divoom_mic_ctrl_release
  Description:    取消之前申请的MIC数据通路
  Input:            pcallback    数据输出回调函数
                       
  Return:           none
  Author:           albert
**********************************************************************/

//暂停task的mic数据就行了
void divoom_mic_ctrl_release(void)
{
	if(pdivoom_mic_ctrl_handle == NULL)
	{
		return;
	}
	if(pdivoom_mic_ctrl_handle->run_flag)
	{
		pdivoom_mic_ctrl_handle->run_flag = 0;
		while(pdivoom_mic_ctrl_handle->stop_flag == 1)
		{
			divoom_system_delay_ms(1);
		}
	}
	ESP_LOGI("MIC", "%s--%d", __func__, __LINE__);
	divoom_system_get_lock(pdivoom_mic_ctrl_handle->mic_lock_handle);
	pdivoom_mic_ctrl_handle->cb = NULL;
	divoom_system_free_lock(pdivoom_mic_ctrl_handle->mic_lock_handle);
	divoom_mic_ctrl_mic_en(0);
}

/*********************************************************************
  Function:        divoom_mic_ctrl_exit
  Description:    退出相关模块
  Input:            pcallback    数据输出回调函数
                       
  Return:           none
  Author:           albert
**********************************************************************/

void divoom_mic_ctrl_exit(void)
{
}
#else

void divoom_mic_ctrl_init(void)
{

}

void divoom_mic_ctrl_exit(void)
{
}

#endif



