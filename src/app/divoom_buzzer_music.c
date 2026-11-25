#include "divoom.h"
#include "divoom_buzzer_music.h"
//#include "arch_pwm.h"
//#include "arch_gpio.h"
/*
蜂呜器说明:
音符对应pwm频率
音符节奏拍数对应delay时间，这个delay对应微任务时间，最小单位10ms
*/

#if 0
#define DIVOOM_BUZZER_REFRESH_TIME		1
#define DIVOOM_BUZZER_PLAY_SPEED    40

typedef struct _divoom_buzzer_ctrl
{
    uint8 task_id;
    uint8 song;
    uint16 length;	//总步数
    uint16 index;   //
    uint16 time;    //单位10ms
	const int16 *note;	//音符
	const float *duration;	//节拍
}divoom_buzzer_ctrl, *Pdivoom_buzzer_ctrl;

typedef struct __divoom_buzzer_song{
    uint16 time;    //单位10ms    
    uint16 length;	//总步数
	const int16 *note;	//音符
	const float *duration;	
}T_DIVOOM_BUZZER_SONG;

static Pdivoom_buzzer_ctrl pdivoom_buzzer_handle = NULL;

#define NOTE_D0 -1
#define NOTE_D1 294
#define NOTE_D2 330
#define NOTE_D3 350
#define NOTE_D4 393
#define NOTE_D5 441
#define NOTE_D6 495
#define NOTE_D7 556

#define NOTE_DL1 147
#define NOTE_DL2 165
#define NOTE_DL3 175
#define NOTE_DL4 196
#define NOTE_DL5 221
#define NOTE_DL6 248
#define NOTE_DL7 278

#define NOTE_DH1 589
#define NOTE_DH2 661
#define NOTE_DH3 700
#define NOTE_DH4 786
#define NOTE_DH5 882
#define NOTE_DH6 990
#define NOTE_DH7 112



#define NOTE_G0 -1
#define NOTE_G1 393
#define NOTE_G2 441
#define NOTE_G3 495
#define NOTE_G4 556
#define NOTE_G5 624
#define NOTE_G6 661
#define NOTE_G7 742

#define NOTE_GL1 196
#define NOTE_GL2 221
#define NOTE_GL3 234
#define NOTE_GL4 262
#define NOTE_GL5 294
#define NOTE_GL6 330
#define NOTE_GL7 371

#define NOTE_GH1 786
#define NOTE_GH2 882
#define NOTE_GH3 990
#define NOTE_GH4 1049
#define NOTE_GH5 1178
#define NOTE_GH6 1322
#define NOTE_GH7 1484


#define WHOLE 1
#define HALF 0.5
#define QUARTER 0.25
#define EIGHTH 0.25
#define SIXTEENTH 0.625

static const int16 divoom_buzzer_only_DI[] = 
{
	2731,2731
};

static const float divoom_buzzer_duration_only_DI[]=
{
	1,1,
};

//这部分是用英文对应了拍子，这样后面也比较好看
static const int16 divoom_buzzer_note_gourd_boy[] = 
{
  NOTE_DH1,NOTE_D6,NOTE_D5,NOTE_D6,NOTE_D0,
  NOTE_DH1,NOTE_D6,NOTE_D5,NOTE_DH1,NOTE_D6,NOTE_D0,NOTE_D6,
  NOTE_D6,NOTE_D6,NOTE_D5,NOTE_D6,NOTE_D0,NOTE_D6,
  NOTE_DH1,NOTE_D6,NOTE_D5,NOTE_DH1,NOTE_D6,NOTE_D0,
  
  NOTE_D1,NOTE_D1,NOTE_D3,
  NOTE_D1,NOTE_D1,NOTE_D3,NOTE_D0,
  NOTE_D6,NOTE_D6,NOTE_D6,NOTE_D5,NOTE_D6,
  NOTE_D5,NOTE_D1,NOTE_D3,NOTE_D0,
  NOTE_DH1,NOTE_D6,NOTE_D6,NOTE_D5,NOTE_D6,
  NOTE_D5,NOTE_D1,NOTE_D2,NOTE_D0,
  NOTE_D7,NOTE_D7,NOTE_D5,NOTE_D3,
  NOTE_D5,
  NOTE_DH1,NOTE_D0,NOTE_D6,NOTE_D6,NOTE_D5,NOTE_D5,NOTE_D6,NOTE_D6,
  NOTE_D0,NOTE_D5,NOTE_D1,NOTE_D3,NOTE_D0,
  NOTE_DH1,NOTE_D0,NOTE_D6,NOTE_D6,NOTE_D5,NOTE_D5,NOTE_D6,NOTE_D6,
  NOTE_D0,NOTE_D5,NOTE_D1,NOTE_D2,NOTE_D0,
  NOTE_D3,NOTE_D3,NOTE_D1,NOTE_DL6,
  NOTE_D1,
  NOTE_D3,NOTE_D5,NOTE_D6,NOTE_D6,
  NOTE_D3,NOTE_D5,NOTE_D6,NOTE_D6,
  NOTE_DH1,NOTE_D0,NOTE_D7,NOTE_D5,
  NOTE_D6,
};//这部分就是整首曲子的音符部分，用了一个序列定义为tune，整数

static const float divoom_buzzer_duration_gourd_boy[]=
{
  1,1,0.5,0.5,1,
  0.5,0.5,0.5,0.5,1,0.5,0.5,
  0.5,1,0.5,1,0.5,0.5,
  0.5,0.5,0.5,0.5,1,1,
  
  1,1,1+1,
  0.5,1,1+0.5,1,
  1,1,0.5,0.5,1,
  0.5,1,1+0.5,1,
  0.5,0.5,0.5,0.5,1+1,
  0.5,1,1+0.5,1,
  1+1,0.5,0.5,1,
  1+1+1+1,
  0.5,0.5,0.5+0.25,0.25,0.5+0.25,0.25,0.5+0.25,0.25,
  0.5,1,0.5,1,1,
  0.5,0.5,0.5+0.25,0.25,0.5+0.25,0.25,0.5+0.25,0.25,
  0.5,1,0.5,1,1,
  1+1,0.5,0.5,1,
  1+1+1+1,
  0.5,1,0.5,1+1,
  0.5,1,0.5,1+1,
  1+1,0.5,0.5,1,
  1+1+1+1
};

static const int16 divoom_buzzer_note_sky_city[] =  
{  
 NOTE_D0,NOTE_D0,NOTE_D0,NOTE_D6,NOTE_D7,NOTE_DH1,NOTE_D7,NOTE_DH1,NOTE_DH3,NOTE_D7,NOTE_D7,NOTE_D7,NOTE_D3,NOTE_D3,
 NOTE_D6,NOTE_D5,NOTE_D6,NOTE_DH1,NOTE_D5,NOTE_D5,NOTE_D5,NOTE_D3,NOTE_D4,NOTE_D3,NOTE_D4,NOTE_DH1,
 NOTE_D3,NOTE_D3,NOTE_D0,NOTE_DH1,NOTE_DH1,NOTE_DH1,NOTE_D7,NOTE_D4,NOTE_D4,NOTE_D7,NOTE_D7,NOTE_D7,NOTE_D0,NOTE_D6,NOTE_D7,
 NOTE_DH1,NOTE_D7,NOTE_DH1,NOTE_DH3,NOTE_D7,NOTE_D7,NOTE_D7,NOTE_D3,NOTE_D3,NOTE_D6,NOTE_D5,NOTE_D6,NOTE_DH1,
 NOTE_D5,NOTE_D5,NOTE_D5,NOTE_D2,NOTE_D3,NOTE_D4,NOTE_DH1,NOTE_D7,NOTE_D7,NOTE_DH1,NOTE_DH1,NOTE_DH2,NOTE_DH2,NOTE_DH3,NOTE_DH1,NOTE_DH1,NOTE_DH1,
 NOTE_DH1,NOTE_D7,NOTE_D6,NOTE_D6,NOTE_D7,NOTE_D5,NOTE_D6,NOTE_D6,NOTE_D6,NOTE_DH1,NOTE_DH2,NOTE_DH3,NOTE_DH2,NOTE_DH3,NOTE_DH5,
 NOTE_DH2,NOTE_DH2,NOTE_DH2,NOTE_D5,NOTE_D5,NOTE_DH1,NOTE_D7,NOTE_DH1,NOTE_DH3,NOTE_DH3,NOTE_DH3,NOTE_DH3,NOTE_DH3,
 NOTE_D6,NOTE_D7,NOTE_DH1,NOTE_D7,NOTE_DH2,NOTE_DH2,NOTE_DH1,NOTE_D5,NOTE_D5,NOTE_D5,NOTE_DH4,NOTE_DH3,NOTE_DH2,NOTE_DH1,
 NOTE_DH3,NOTE_DH3,NOTE_DH3,NOTE_DH3,NOTE_DH6,NOTE_DH6,NOTE_DH5,NOTE_DH5,NOTE_DH3,NOTE_DH2,NOTE_DH1,NOTE_DH1,NOTE_D0,NOTE_DH1,
 NOTE_DH2,NOTE_DH1,NOTE_DH2,NOTE_DH2,NOTE_DH5,NOTE_DH3,NOTE_DH3,NOTE_DH3,NOTE_DH3,NOTE_DH6,NOTE_DH6,NOTE_DH5,NOTE_DH5,
 NOTE_DH3,NOTE_DH2,NOTE_DH1,NOTE_DH1,NOTE_D0,NOTE_DH1,NOTE_DH2,NOTE_DH1,NOTE_DH2,NOTE_DH2,NOTE_D7,NOTE_D6,NOTE_D6,NOTE_D6,NOTE_D6,NOTE_D7
};//这部分就是整首曲子的音符部分，用了一个序列定义为tune，整数

static const float divoom_buzzer_duration_sky_city[]= 
{  
  1,1,1,0.5,0.5,     1+0.5,0.5,1,1,     1,1,1,0.5,0.5,
  1+0.5,0.5,1,1,     1,1,1,1,          1+0.5,0.5,1,1, 
  1,1,0.5,0.5,0.5,0.5,    1+0.5,0.5,1,1,     1,1,1,0.5,0.5,
  1+0.5,0.5,1,1,    1,1,1,0.5,0.5,     1+0.5,0.5,1,1,
  1,1,1,0.5,0.5,    1,0.5,0.25,0.25,0.25,0.5,    0.5,0.5,0.5,0.25,0.5,1,
  0.5,0.5,0.5,0.5,1,1,    1,1,1,0.5,0.5,    1+0.5,0.5,1,1,
  1,1,1,0.5,0.5,    1.5,0.5,1,1,    1,1,1,1,
  0.5,0.5,1,1,0.5,0.5,    1.5,0.25,0.5,1,    1,1,1,1,
  1,1,1,1,    1,1,1,1,    0.5,0.5,1,1,0.5,0.5,
  1,0.5,0.5,1,1,    1,1,1,1,    1,1,1,1,
  0.5,0.5,1,1,0.5,0.5,    1,0.5,0.25,0.5,1,    1,1,1,0.5,0.5
};

static const int16 divoom_buzzer_note_dididi[] = 
{  //这里的didi声为4kz，直接用数字进行表示
    2731,NOTE_D0,
    2731,NOTE_D0,
    2731,NOTE_D0,
    2731,NOTE_D0
};

static const float divoom_buzzer_duration_dididi[]= 
{  
    1,1,
    1,1,
    1,1,
    1,8
};


/*
报警声
static const int16 divoom_buzzer_note_alarm[] = 
{
    NOTE_DH5,NOTE_DH5,NOTE_DH5
    NOTE_D0, 
};

static const float divoom_buzzer_duration_alarm[]= 
{
    1,1,1,
    2,
};
*/


static const T_DIVOOM_BUZZER_SONG divoom_buzzer_song_list[] =
{
    {   //葫芦娃
        DIVOOM_BUZZER_PLAY_SPEED,
        DIVOOM_ARRAY_LEN(divoom_buzzer_note_gourd_boy),
        divoom_buzzer_note_gourd_boy,
        divoom_buzzer_duration_gourd_boy,
    },
    {   //天空之城
        DIVOOM_BUZZER_PLAY_SPEED,
        DIVOOM_ARRAY_LEN(divoom_buzzer_note_sky_city),
        divoom_buzzer_note_sky_city,
        divoom_buzzer_duration_sky_city,
    },
    {   //闹钟dididi
        7,
        DIVOOM_ARRAY_LEN(divoom_buzzer_note_dididi),    
        divoom_buzzer_note_dididi,
        divoom_buzzer_duration_dididi,
    },
	{
		DIVOOM_BUZZER_PLAY_SPEED,
        DIVOOM_ARRAY_LEN(divoom_buzzer_only_DI),
        divoom_buzzer_only_DI,
        divoom_buzzer_duration_only_DI,
	}
};


#include "driver/ledc.h"

#define LEDC_HS_TIMER          LEDC_TIMER_0
#define LEDC_HS_MODE           LEDC_LOW_SPEED_MODE
#define LEDC_HS_CH0_GPIO       DIVOOM_BUZZER_PWM_PIN
#define LEDC_HS_CH0_CHANNEL    LEDC_CHANNEL_0

static const ledc_timer_config_t ledc_timer = {
	.duty_resolution = LEDC_TIMER_8_BIT, // resolution of PWM duty
	.freq_hz = 5000,					  // frequency of PWM signal
	.speed_mode = LEDC_HS_MODE, 		  // timer mode
	.timer_num = LEDC_HS_TIMER			  // timer index
};
static const ledc_channel_config_t ledc_channel = 
{
	.channel	= LEDC_HS_CH0_CHANNEL,
	.duty		= 128,
	.gpio_num	= LEDC_HS_CH0_GPIO,
	.speed_mode = LEDC_HS_MODE,
	.hpoint 	= 0,
	.timer_sel	= LEDC_HS_TIMER
};

void divoom_buzzer_pwm_init(void)
{
	// Set configuration of timer0 for high speed channels
	ledc_timer_config(&ledc_timer);
	ledc_channel_config(&ledc_channel);
    // Initialize fade service.
    ledc_fade_func_install(0);
}

void divoom_buzzer_pwm_set(uint16 freq)
{
	ledc_set_freq(ledc_channel.speed_mode,ledc_channel.timer_sel,freq);
	ledc_set_duty(ledc_channel.speed_mode, ledc_channel.channel, 128);
	ledc_update_duty(ledc_channel.speed_mode, ledc_channel.channel);
}

void divoom_buzzer_pwm_close(void)
{
	ledc_stop(ledc_channel.speed_mode, ledc_channel.channel,0);
}


/*****************************************************************************
 Prototype    : divoom_buzzer_get_freq
 Description  : 获取当前节拍对应频率
 Input        : uint16 index  
 Output       : None
 Return Value : 
 
  History        :
  1.Date         : 2018/5/9
    Author       : coronaa
    Modification : Created function
*****************************************************************************/
int16 divoom_buzzer_get_freq(uint16 index)
{
	int16 freq;    
    index %= pdivoom_buzzer_handle->length;
	freq = pdivoom_buzzer_handle->note[index];
	return freq;
}

/*****************************************************************************
 Prototype    : dvioom_buzzer_get_time
 Description  : 获取节拍对应持续时间
 Input        : uint16 index  
 Output       : None
 Return Value : 
 
  History        :
  1.Date         : 2018/5/9
    Author       : coronaa
    Modification : Created function
*****************************************************************************/
uint16 dvioom_buzzer_get_time(uint16 index)
{
	uint16 time;
    index %= pdivoom_buzzer_handle->length;
	time = pdivoom_buzzer_handle->time * pdivoom_buzzer_handle->duration[index];
	return time;
}

/*****************************************************************************
 Prototype    : divoom_buzzer_set_time
 Description  : 更新闹钟微任务时间
 Input        : uint32 time  
 Output       : None
 Return Value : 
 
  History        :
  1.Date         : 2018/5/9
    Author       : coronaa
    Modification : Created function
*****************************************************************************/
void divoom_buzzer_set_time(uint32 time)
{
    if(pdivoom_buzzer_handle == NULL)
        return;
    divoom_microtask_update(pdivoom_buzzer_handle->task_id, time);	
}


/*****************************************************************************
 Prototype    : divoom_buzzer_set_song
 Description  : 设定相应音乐任务参数
 Input        : uint8 song  
 Output       : None
 Return Value : 
 
  History        :
  1.Date         : 2018/5/9
    Author       : coronaa
    Modification : Created function
*****************************************************************************/
void divoom_buzzer_set_song(uint8 song)
{
    if(pdivoom_buzzer_handle == NULL)
        return;
    song %= DIVOOM_ARRAY_LEN(divoom_buzzer_song_list);
    pdivoom_buzzer_handle->length = divoom_buzzer_song_list[song].length;
    pdivoom_buzzer_handle->time = divoom_buzzer_song_list[song].time;    
    pdivoom_buzzer_handle->note = divoom_buzzer_song_list[song].note;
    pdivoom_buzzer_handle->duration = divoom_buzzer_song_list[song].duration;
}


/*****************************************************************************
 Prototype    : divoom_buzzer_select_song
 Description  : 选择歌曲
 Input        : uint8 song  
 Output       : None
 Return Value : 
 
  History        :
  1.Date         : 2018/5/9
    Author       : coronaa
    Modification : Created function
*****************************************************************************/
void divoom_buzzer_select_song(uint8 song)
{
    if(pdivoom_buzzer_handle == NULL || song>=DIVOOM_BUZZER_END)
        return;
    divoom_microtask_pause(pdivoom_buzzer_handle->task_id);    
	divoom_buzzer_pwm_close();	//关闭pwm
    pdivoom_buzzer_handle->index = 0;
   	divoom_buzzer_set_song(song);    
    divoom_microtask_resume(pdivoom_buzzer_handle->task_id);
}


/*****************************************************************************
 Prototype    : divoom_buzzer_main
 Description  : 蜂鸣器主任务
 Input        : void *data  
 Output       : None
 Return Value : 
 
  History        :
  1.Date         : 2018/5/9
    Author       : coronaa
    Modification : Created function
*****************************************************************************/
void divoom_buzzer_main(void *data)
{
	int16 freq,time;
	
//	divoom_print("This Is PWM TASK!\n");
	freq = divoom_buzzer_get_freq(pdivoom_buzzer_handle->index);
	time = dvioom_buzzer_get_time(pdivoom_buzzer_handle->index);

    if(freq != NOTE_D0)
    {    
        divoom_buzzer_en_io_set(1);
    	divoom_buzzer_pwm_set(freq);	//设置频率	        
    }
    else
    {
        divoom_buzzer_en_io_set(0);
		divoom_buzzer_pwm_close(); //关闭pwm        
    }
//	divoom_buzzer_pwm_set(freq);	//设置频率	
	divoom_buzzer_set_time(time);	//设置时间	
	if(++pdivoom_buzzer_handle->index >= pdivoom_buzzer_handle->length)
	{
		pdivoom_buzzer_handle->index = 0;
	}
}

/*****************************************************************************
 Description  : 蜂鸣器模块初始化
 Input        : uint8 *data  
 Output       : None
 Return Value : 
 
  History        :
  1.Date         : 2018/5/9
    Author       : coronaa
    Modification : Created function
*****************************************************************************/
void divoom_buzzer_init(uint8 *data)
{
    if(pdivoom_buzzer_handle)
    {
        return ;
    }
    divoom_print("divoom_buzzer_init:!\n");
   	divoom_buzzer_en_io_set(1);	
    pdivoom_buzzer_handle = (Pdivoom_buzzer_ctrl)divoom_system_malloc(sizeof(divoom_buzzer_ctrl));
    memset(pdivoom_buzzer_handle, 0, sizeof(divoom_buzzer_ctrl));

    if(data == AK_NULL)
    {
        divoom_buzzer_set_song(DIVOOM_BUZZER_CLOCK_DIDIDI);
    }else
    {
        divoom_buzzer_set_song(*data);
    }
	divoom_buzzer_pwm_init();
    pdivoom_buzzer_handle->task_id = divoom_microtask_create(divoom_buzzer_main,DIVOOM_BUZZER_REFRESH_TIME);
    divoom_microtask_resume(pdivoom_buzzer_handle->task_id);
}

/*****************************************************************************
 Description  : 关闭蜂鸣器模块
 Input        : void  
 Output       : None
 Return Value : 
 
  History        :
  1.Date         : 2018/5/9
    Author       : coronaa
    Modification : Created function
*****************************************************************************/
void divoom_buzzer_close(void)
{
    if(pdivoom_buzzer_handle == AK_NULL)
    {
        return;
    }
    divoom_microtask_pause(pdivoom_buzzer_handle->task_id);
    divoom_microtask_destroy(pdivoom_buzzer_handle->task_id);
    divoom_free(pdivoom_buzzer_handle);
    pdivoom_buzzer_handle = NULL;
	divoom_buzzer_pwm_close(); //关闭pwm
   	divoom_buzzer_en_io_set(0);
}


/*****************************************************************************
 Prototype    : divoom_buzzer_en_io_set
 Description  : 设置蜂鸣器使能脚
 Input        : uint8 level  
 Output       : None
 Return Value : 
 
  History        :
  1.Date         : 2018/5/21
    Author       : coronaa
    Modification : Created function
*****************************************************************************/
void divoom_buzzer_en_io_set(uint8 level)
{
	return;
/*
    gpio_set_pin_dir(DIVOOM_BUZZER_EN_GPIO, GPIO_DIR_OUTPUT);

    if (level)
    {
        gpio_set_pin_level(DIVOOM_BUZZER_EN_GPIO, GPIO_LEVEL_HIGH);
    }
    else
    {
        gpio_set_pin_level(DIVOOM_BUZZER_EN_GPIO, GPIO_LEVEL_LOW);
    }
*/
}    

void divoom_buzzer_init_io(void)
{
    divoom_buzzer_pwm_init();
	divoom_buzzer_pwm_close(); //关闭pwm
}    
#endif


