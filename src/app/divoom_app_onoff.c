#if 0
#include "divoom.h"
#include "driver/gpio.h"
#include "freertos/queue.h"
#include "nvs_flash.h"
#ifdef CONFIG_ESP_IDF_V4
#include "esp32/clk.h"
#else
#include "esp_clk.h"
#endif
#include "esp_bt.h"
#include "driver/rtc_io.h"
#include "esp_sleep.h"


#include "divoom_wifi_con.h"

#define DIVOOM_APP_ONOFF_TAG					"DIVOOM_APP_ONOFF"

#define DIVOOM_ONOFF_PRINT(format, ...)			ESP_LOGI(DIVOOM_APP_ONOFF_TAG, format, ##__VA_ARGS__)

//模块需要做的事情：
//需要保存一个下一次的闹钟时间在nvs里面
//1：进入睡眠的时候，设置两个唤醒，ext1的rtcgpio和timer--》timer要根据闹钟时间换算成秒数或者微秒
//2：唤醒:ESP_SLEEP_WAKEUP_EXT1-->需要加延时检测是否是长按开机；ESP_SLEEP_WAKEUP_TIMER-->可以不用判断；其他开机不检测不延时，直接开机
//3：event事件要接收，命令进入休眠，算好时间，设置timer，设置key
//4：rtc_gpio高电平触发唤醒，关机的时候做上升沿触发

//20190419---->最新定义,不再需要开关机等待功能,由GD控制,这边只是按键控制,开机模式控制
//△    因为在之前的测试中发现使用中断会在触发了一百多次中断之后,会发现取消中断无响应,所以该用轮询的方式

//是否使用轮询方式，注释掉是用中断，中断用不了了
#define		DIVOOM_ONOFF_USE_LOOP_ASK

//需要持续按下的时间，单位秒
#define		DIVOOM_ONOFF_CHECK_TIME			(3000)
//按下多少秒触发配网
#define		DIVOOM_ONOFF_CONFIG_WIFI		(3000)
//按下多少秒切换模式
#define		DIVOOM_ONOFF_CHANGE_MODE		(5000)
//触发电平
#define		DIVOOM_ONOFF_CHECK_HIGH			(0)

//io detect的持续时间500ms
#define		DIVOOM_DETECT_IO_TIME			(500)

//200ms
#define		DIVOOM_APP_KEY_SHORT_PRESS		(100)
//2000ms
#define		DIVOOM_APP_KEY_LONG_PRESS		(2000)
#define		DIVOOM_APP_KEY_LONG_LONG_PRESS	(6000)
//100ms
#define		DIVOOM_APP_KEY_LONG_PRESSING	(2000)

#define		DIVOOM_ONOFF_NVS_NAME			"DIVOOM_ONOFF"
#define		DIVOOM_ONOFF_NEXT_TIMER_NVS		"NEXT_TIMER"
//是否是切换模式开机
#define		DIVOOM_ONOFF_IS_CHANNGE_NVS		"IS_CHANNGE"
//是否要改变模式，1进入WiFi模式,切换到WiFi模式，需要释放classic内存，BLE的内存先不释放，在配网模式决定是否释放，2,蓝牙模式,在WiFi配网的时候需要获取一下这边的状态，再决定是否需要初始化WiFi
//所以要求在开机之后先进行网络检测，现在是没有网络配置就开始进入BLE配网，如果读到了正确的WiFi信息，在释放BLE的内存
//蓝牙和WiFi的切换这里只是释放classic内存，BLE的内存在配网部分释放
#define		DIVOOM_ONOFF_CHANGE_MODE_NVS	"CHANGE_MODE"

#define		DIVOOM_ONOFF_TASK_STACK			(4*1024)
#define		DIVOOM_ONOFF_TASK_PRI			ESP_TASKD_EVENT_PRIO

#define		DIVOOM_ONOFF_DELAY_TIME			(20 / portTICK_PERIOD_MS)
#define		DIVOOM_ONOFF_RECHECK			(20 / portTICK_PERIOD_MS)
#define		ESP_INTR_FLAG_DEFAULT			(0)

enum{
	DIVOOM_APP_ONOFF_CHECK_PRESS,				//检测是否按下
	DIVOOM_APP_ONOFF_CONFIRM_PRESS,				//确认是否按下
	DIVOOM_APP_ONOFF_CHECK_RELEASE,				//检测是否松开按键
	DIVOOM_APP_ONOFF_CONFIRM_RELEASE,			//确认已经松开按键
};
//DIVOOM_APP_ONOFF_CHECK_TIME_ON,				//当检测到按下的时间到达DIVOOM_ONOFF_CHECK_TIME，DIVOOM_APP_ONOFF_CHECK_RELEASE执行关机前的操作

typedef struct _divoom_app_onoff_key_list_
{
	//按键数量
	uint8_t key_num;
	//按键信息
	uint8_t key_info[2];
	//接通之后的电平状态
	uint8_t active_level[2];
	DIVOOM_KEY_VALUE key_value;
	struct _divoom_app_onoff_key_list_ *next;
}divoom_app_onoff_key_list, *Pdivoom_app_onoff_key_list;

typedef struct _divoom_app_onoff_adc_key_list_
{
	uint8_t		key_num;
	//按下之后的adc值,上下浮动50mv
	uint32_t	adc_value_max;
	//和 adc_value_max 相差100
	uint32_t	adc_value_min;
	DIVOOM_KEY_VALUE key_value;
	struct _divoom_app_onoff_adc_key_list_ *next;
}divoom_app_onoff_adc_key_list, *Pdivoom_app_onoff_adc_key_list;

typedef struct _divoom_io_detect_list_
{
	//需要detect的gpio
	uint8_t io_gpio;
	//接通之后的电平状态
	uint8_t active_level;
	//前一个状态
	uint8_t pre_level;
	//电平变化的时间
	uint32_t chang_time;
	DIVOOM_IO_TYPE io_value;
	struct _divoom_io_detect_list_ *next;
}divoom_io_detect_list, *Pdivoom_io_detect_list;

typedef struct{
#ifdef DIVOOM_TASK_PRINTF_STACK_INFO
	void                    *divoom_onoff_handle;
#endif
	//task运行标志
	bool					divoom_task_run_flag;
#ifndef DIVOOM_ONOFF_NO_POWER_CONTROL
	//是否是开机闹钟
	bool					alarm_is_open_flag;

	//当前开机的状态，按键开机，和非按键开机
	uint8_t					divoom_power_on_status;
#endif
	//现在是开机检测还是关机检测，1是开机，0是关机，唤醒之后默认是1
	//如果不是按键开机的话,检测完开机是否测试模式就切换
	bool					divoom_onoff_key_status;
	//等待这个参数变为1，开机，0，等待，2按下时间不够，继续睡眠
	//增加状态3
	//被优化?
	//如果是不要开机的话,只有状态2或者0
	volatile uint8_t		divoom_onoff_ple_poweron;

	//是否是蓝牙模式，0是WiFi模式，1是蓝牙模式
	//bool					divoom_now_mode;
	//当前运行模式DIVOOM_APP_ONOFF_WIFI_MODE or DIVOOM_APP_ONOFF_BT_MODE
	uint8					now_power_mode;
	//信号量
	SemaphoreHandle_t		divoom_onoff_lock;
	//nvs handle
	nvs_handle				divoom_nvs_handle;
	//存一下当前需要休眠的时间,只作为开始按下时间不够进入休眠的时间
	uint64_t				next_timer;
	//是否是切换模式开机1-->yes,0-->no
	uint8					is_channge_mode;
	/******************for key & detect io**************************/
	//io key use
	uint32_t				press_time_all;
	uint8_t					cur_press_status;
	//当前是否是在长按状态
	uint8_t					is_long_press;
	//adc use
	//分开处理的话,有可能我们这个io key 只有开关机功能,增加一个处理,如果在按下adc按键的时候,发现按下了 io key,那么就清除当前的 adc key 的状态,普通来说,这俩按键是不会同时
	//按下的,但是有人这么用的话,那只能这么处理,优先响应io key
	uint32_t				adc_press_time_all;
	uint8_t					adc_cur_press_status;
	uint8_t					adc_is_long_press;
	Pdivoom_app_onoff_key_list key_list;
	Pdivoom_io_detect_list	io_list;
	Pdivoom_app_onoff_adc_key_list adc_key_list;
	/******************for key & detect io**************************/
#ifndef DIVOOM_ONOFF_USE_LOOP_ASK
	//传递当前中断的gpio
	xQueueHandle			divoom_onoff_queue;
	gpio_num_t				*key_info;
#endif
}divoom_app_power_struct, *Pdivoom_app_power_struct;

Pdivoom_app_power_struct pdivoom_app_power_handle = NULL;

static void divoom_app_onoff_set_rtcgpio_wakeup(uint32 key_gpio);
//再乘以1000000,这个时间减去当前rtc时间,单位秒
static void divoom_app_onoff_set_timer_wakeup(uint64_t next_timer);
//从alarm获取下一个的闹钟的utc时间，减去当前utc时间（time(0)），得到时间差值
//如果取到的值小于等于当前time(0),需要从新取
static uint32 divoom_app_onoff_get_next_alarm_timer(void);
static void divoom_app_onoff_set_next_wakeup_timer(uint64_t out_value);
static void divoom_app_onoff_set_mode(uint8 next_mode);

#ifndef DIVOOM_ONOFF_USE_LOOP_ASK
void divoom_app_onoff_isr_func(void *arg)
{
	BaseType_t xHigherPriorityTaskWoken;
	ets_printf("\r\ninterrupt!\r\n");
	if(pdivoom_app_power_handle && pdivoom_app_power_handle->divoom_onoff_queue && arg)
	{
		//可能之前是这里有问题,arg
		xQueueSendFromISR(pdivoom_app_power_handle->divoom_onoff_queue, arg, &xHigherPriorityTaskWoken);
		if(xHigherPriorityTaskWoken)
		{
			// Actual macro used here is port specific.
			portYIELD_FROM_ISR();
		}
	}
}
#endif

#ifndef DIVOOM_ONOFF_NO_POWER_CONTROL
//配置开关机时的电源io口设置
//如果不是双电源控制,可以直接返回
void divooom_app_onoff_set_power(uint8 power_status)
{
	uint32 ret = 0;
	if(power_status)
	{
		//配置开机时的电源
		ret = gpio_set_level(DIVOOM_ONOFF_POWERON_POWER, 1);
		//ret = rtc_gpio_set_level(DIVOOM_ONOFF_POWERON_POWER, 1);
		if(ret != ESP_OK)
		{
			DIVOOM_ONOFF_PRINT("Some Wrong in here:%d\n", __LINE__);
			return;
		}
		/*ret = gpio_set_level(DIVOOM_ONOFF_POWEROFF_POWER, 0);
		//ret = rtc_gpio_set_level(DIVOOM_ONOFF_POWEROFF_POWER, 0);
		if(ret != ESP_OK)
		{
			DIVOOM_ONOFF_PRINT("Some Wrong in here:%d\n", __LINE__);
			return;
		}*/
	}
	else
	{
		//配置关机时的电源
		/*ret = gpio_set_level(DIVOOM_ONOFF_POWEROFF_POWER, 1);
		//ret = rtc_gpio_set_level(DIVOOM_ONOFF_POWEROFF_POWER, 1);
		if(ret != ESP_OK)
		{
			DIVOOM_ONOFF_PRINT("Some Wrong in here:%d\n", __LINE__);
			return;
		}
		vTaskDelay(1 / portTICK_PERIOD_MS);*/
		ret = gpio_set_level(DIVOOM_ONOFF_POWERON_POWER, 0);
		//ret = rtc_gpio_set_level(DIVOOM_ONOFF_POWERON_POWER, 0);
		if(ret != ESP_OK)
		{
			DIVOOM_ONOFF_PRINT("Some Wrong in here:%d\n", __LINE__);
			return;
		}
	}
}
#endif

//设置开机gpio属性
//不需要设置拉高拉低,只需要设置输入输出属性,并且要无中断
//另外,切记,ext1唤醒要rtc GPIO
//预留两个参数是为了两个按键同时使用的情况
static void divoom_app_onoff_set_gpio_attr(uint8 gpio_num, uint8 gpio_num1)
{
	int32_t ret = 0;
	gpio_config_t io_conf;

	if(gpio_num != 0xFF)
	{
		memset(&io_conf, 0, sizeof(gpio_config_t));
		io_conf.intr_type = GPIO_INTR_DISABLE;
		io_conf.pin_bit_mask = (1ULL << gpio_num);
		io_conf.mode = GPIO_MODE_INPUT;
		//io_conf.pull_down_en = 1;
#if defined SHACO_TEST
		io_conf.pull_down_en = 1;
#endif
		if((ret = gpio_config(&io_conf)) != 0)
		{
			DIVOOM_ONOFF_PRINT("line:%d, ret = 0x%02x\n", __LINE__, ret);
		}
	}

	if(gpio_num1 != 0xFF)
	{
		memset(&io_conf, 0, sizeof(gpio_config_t));
		io_conf.intr_type = GPIO_INTR_DISABLE;
		io_conf.pin_bit_mask = (1ULL << gpio_num1);
		io_conf.mode = GPIO_MODE_INPUT;
		//io_conf.pull_down_en = 1;
#if defined SHACO_TEST
		io_conf.pull_down_en = 1;
#endif
		if((ret = gpio_config(&io_conf)) != 0)
		{
			DIVOOM_ONOFF_PRINT("line:%d, ret = 0x%02x\n", __LINE__, ret);
		}
	}
}

#ifndef DIVOOM_ONOFF_NO_POWER_CONTROL
//是否需要设置唤醒，如果是reboot参数为0
//先设置唤醒gpio,时间,发送到main模块,做关机前的事情,然后最后发送到此模块的event事件开始进行hibernate sleep
static void divoom_app_onoff_sleep_pre_set()
{
	uint32 next_time = 0;
	divoom_sys_msg_info  msg = {0};

	//关闭GD的显示，把GD断电
	//divoom_set_gd_off();

	//执行进入睡眠动作
	//divoom_sys_model_crl_save_config();

	/*if(flag)
	{*/
		//设置关机按键和时间，时间可能需要再调整，需要松开按键在进入睡眠，先关闭显示
		//如果下一个闹钟的时间是0，证明没有闹钟，不设置
		//201931不需要这些了
		divoom_app_onoff_set_rtcgpio_wakeup(DIVOOM_ONOFF_CHECK_KEY);
		if((next_time = divoom_app_onoff_get_next_alarm_timer()) > 0)
		{
			divoom_app_onoff_set_timer_wakeup(next_time);
		}
	/*}
	else
	{
		//只有在切换模式reboot的时候才需要设置这个参数
		divoom_app_onoff_set_mode(next_mode);
	}*/
	//直接断开电源
	divooom_app_onoff_set_power(0);
	//告诉系统可以关闭显示了，等松开按键就可以进入休眠了
	msg.msg_id = DIVOOM_MAIN_EVENT_KEY_POWEROFF;
	msg.src_id = DIVOOM_MODEL_ONOFF;
	msg.dest_id = DIVOOM_MODEL_MAIN;
    divoom_sys_model_crl_msg_push(&msg);
}
#endif

#ifndef DIVOOM_ONOFF_USE_LOOP_ASK
//设置中断gpio,为了侦测是否有关机动作，上升沿触发
static int32 divoom_app_onoff_set_isr_gpio(gpio_num_t gpio_num, gpio_isr_t isr_handle)
{
	gpio_config_t io_conf;
	uint32 ret = 0;
	*pdivoom_app_power_handle->key_info = gpio_num;
	memset(&io_conf, 0, sizeof(gpio_config_t));
	//上升沿触发
	io_conf.intr_type = GPIO_INTR_POSEDGE;
	io_conf.pin_bit_mask = (1ULL << gpio_num);
	io_conf.mode = GPIO_MODE_INPUT;
	//io_conf.pull_down_en = 1;
	ret = gpio_config(&io_conf);
	if(ret != ESP_OK)
	{
		DIVOOM_ONOFF_PRINT("Error in %d--0x%02x\n", __LINE__, ret);
		return ret;
	}

	ret = gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
	if(ret != ESP_OK)
	{
		DIVOOM_ONOFF_PRINT("Error in %d--0x%02x\n", __LINE__, ret);
		return ret;
	}
	
	ret = gpio_set_intr_type(gpio_num, GPIO_INTR_POSEDGE);
	if(ret != ESP_OK)
		return ret;

	ret = gpio_isr_handler_add(gpio_num, isr_handle, (void *)pdivoom_app_power_handle->key_info);
	if(ret != ESP_OK)
	{
		DIVOOM_ONOFF_PRINT("Error in %d--0x%02x\n", __LINE__, ret);
		return ret;
	}

	DIVOOM_ONOFF_PRINT("divoom_app_onoff_set_isr_gpio!\n");
	return ESP_OK;
}

static uint8 divoom_app_onoff_remove_isr(gpio_num_t gpio_num)
{
	gpio_config_t io_conf;
	uint32 ret = 0;
	memset(&io_conf, 0, sizeof(gpio_config_t));
	//关闭中断
	io_conf.intr_type = GPIO_INTR_DISABLE;
	io_conf.pin_bit_mask = (1ULL << gpio_num);
	io_conf.mode = GPIO_MODE_INPUT;
	//io_conf.pull_down_en = 1;
	ret = gpio_config(&io_conf);
	if(ret != ESP_OK)
	{
		DIVOOM_ONOFF_PRINT("Error in %d--0x%02x\n", __LINE__, ret);
		return ret;
	}

	ret = gpio_set_intr_type(gpio_num, GPIO_INTR_DISABLE);
	if(ret != ESP_OK)
	{
		DIVOOM_ONOFF_PRINT("Error in %d--0x%02x\n", __LINE__, ret);
		return ret;
	}

	ret = gpio_isr_handler_remove(gpio_num);
	if(ret != ESP_OK)
	{
		DIVOOM_ONOFF_PRINT("Error in %d--0x%02x\n", __LINE__, ret);
		return ret;
	}

	gpio_uninstall_isr_service();

	DIVOOM_ONOFF_PRINT("divoom_app_onoff_remove_isr!\n");
	return 1;
}
#endif

//设置ext1唤醒源的rtc gpio
//只能是rtc gpio,否则运行出错:红色Not an RTC IO: GPIO错误信息,但是编译不会出错
static void divoom_app_onoff_set_rtcgpio_wakeup(uint32 key_gpio)
{
	esp_sleep_enable_ext1_wakeup((1ULL << key_gpio), (DIVOOM_ONOFF_CHECK_HIGH)?ESP_EXT1_WAKEUP_ANY_HIGH:ESP_EXT1_WAKEUP_ALL_LOW);
}

//再乘以1000000,这个时间减去当前rtc时间,单位秒,传到下面的是us
//可以在外面不进行时间检测,这里会判断是不是非0
static void divoom_app_onoff_set_timer_wakeup(uint64_t next_timer)
{
	if(next_timer > 0)
		esp_sleep_enable_timer_wakeup(next_timer * 1000000);
}

//检查时间是否够3秒,0-->no,1->yes
//检测按下时间超过DIVOOM_ONOFF_CHECK_TIME，就可以退出并开机
#if 0
static uint8 divoom_app_onoff_check_key_time(uint32 key_num)
{
	uint32 all_time = 0;
	uint8  on_status = DIVOOM_APP_ONOFF_CHECK_PRESS;
	if(key_num == DIVOOM_ONOFF_CHECK_KEY)
	{
		while(1)
		{
			switch(on_status)
			{
				case DIVOOM_APP_ONOFF_CHECK_PRESS:
					{
						if(gpio_get_level(DIVOOM_ONOFF_CHECK_KEY))
						{
							on_status = DIVOOM_APP_ONOFF_CONFIRM_PRESS;
						}
						else
						{
							//收到中断第一次检查还是低电平就有可能是毛刺
							return 0;
						}
					}
					break;
				case DIVOOM_APP_ONOFF_CONFIRM_PRESS:
					{
						if(gpio_get_level(DIVOOM_ONOFF_CHECK_KEY))
						{
							//说明是毛刺，回去继续进行高电平检测
							on_status = DIVOOM_APP_ONOFF_CHECK_RELEASE;
						}
						else
						{
							//不能确认是按下就退出了
							return 0;
						}
					}
					break;
				case DIVOOM_APP_ONOFF_CHECK_RELEASE:
					{
						//确认是否真的是按下了,一直检测是否按下
						if(gpio_get_level(DIVOOM_ONOFF_CHECK_KEY))
						{
							//all_time += 10ms
							all_time += DIVOOM_ONOFF_DELAY_TIME * portTICK_PERIOD_MS;
							if(all_time >= DIVOOM_ONOFF_CHECK_TIME)
							{
								DIVOOM_ONOFF_PRINT("Long Time press, To PowerOn!\n");
								return 1;
							}
						}
						else
						{
							//检测到松开进行松开检测
							on_status = DIVOOM_APP_ONOFF_CONFIRM_RELEASE;
							DIVOOM_ONOFF_PRINT("Confirm key Wrong!\n");
						}
					}
					break;

				case DIVOOM_APP_ONOFF_CONFIRM_RELEASE:
					{
						if(gpio_get_level(DIVOOM_ONOFF_CHECK_KEY))
						{
							//说明是毛刺，回去继续进行高电平检测
							on_status = DIVOOM_APP_ONOFF_CHECK_RELEASE;
						}
						else
						{
							//确定是松开按键了
							if(all_time >= DIVOOM_ONOFF_CHECK_TIME)
							{
								return 1;
							}
							else
							{
								return 0;
							}
						}
					}
					break;
			}
			vTaskDelay(DIVOOM_ONOFF_DELAY_TIME);
		}
	}
	return 0;
}
#endif

/*static void divoom_app_onoff_set_gpio_no_attr(void)
{
	gpio_config_t io_conf;
	uint32 ret = 0;
	memset(&io_conf, 0, sizeof(gpio_config_t));
	//关闭中断
	io_conf.intr_type = GPIO_INTR_DISABLE;
	io_conf.pin_bit_mask = (1ULL << GPIO_NUM_14) | (1ULL << GPIO_NUM_2) | (1ULL << GPIO_NUM_0) | (1ULL << GPIO_NUM_18) | (1ULL << GPIO_NUM_19) | (1ULL << GPIO_NUM_15) | (1ULL << GPIO_NUM_5);
	io_conf.mode = GPIO_MODE_DISABLE;
	io_conf.pull_down_en = 0;
	io_conf.pull_up_en = 0;
	ret = gpio_config(&io_conf);
	if(ret != ESP_OK)
	{
		DIVOOM_ONOFF_PRINT("Error in %d--0x%02x\n", __LINE__, ret);
		return;
	}

	gpio_hold_en(GPIO_NUM_14);
	gpio_hold_en(GPIO_NUM_2);
	gpio_hold_en(GPIO_NUM_0);
	gpio_hold_en(GPIO_NUM_18);
	gpio_hold_en(GPIO_NUM_19);
	gpio_hold_en(GPIO_NUM_15);
	gpio_hold_en(GPIO_NUM_5);
}*/

//into sleep
//开始使用乐鑫的hibernate sleep
#ifndef DIVOOM_ONOFF_NO_POWER_CONTROL
static void divoom_app_onoff_start_sleep(void)
{
	//此函数仅仅在按键开机按下时间不够的时候调用
	divooom_app_onoff_set_power(0);
	//看需求是否增加
#if 0
	rtc_gpio_init(IIC_DATA);
	rtc_gpio_isolate(IIC_DATA);
	MediaHalUninit();
#endif
	//直接rtc_gpio_isolate gpio会拉高功耗
	//看需求是否增加
#if 0
	rtc_gpio_init(IIC_CLK);
	rtc_gpio_isolate(IIC_CLK);
	//gpio14是高电平，要拉低
	rtc_gpio_init(IIS_SCLK);
	rtc_gpio_isolate(IIS_SCLK);

	uart_driver_delete(UART_NUM_1);
	rtc_gpio_init(TXD_PIN);
	rtc_gpio_isolate(TXD_PIN);
	rtc_gpio_init(RXD_PIN);
	rtc_gpio_isolate(RXD_PIN);
#endif
	esp_deep_sleep_start();
}

//从alarm获取下一个的闹钟的utc时间，减去当前utc时间（time(0)），得到时间差值
//如果取到的值小于等于当前time(0),需要从新取
//需要根据alarm模块进行调整
static uint32 divoom_app_onoff_get_next_alarm_timer(void)
{
#if 1
	//这个部分的逻辑还要调整,现获取，然后存到nvs，再返回需要的值
	uint32 next_alarm = 0;
	divoom_sys_msg_info model_msg = {0};
	model_msg.src_id = DIVOOM_MODEL_ONOFF;
	model_msg.dest_id = DIVOOM_MODEL_ALARM;
	model_msg.msg_id = DIVOOM_ALARM_GET_NEXT_ALARM_TIME;

	divoom_sys_model_crl_msg_push(&model_msg);
	next_alarm = model_msg.result;

	divoom_app_onoff_set_next_wakeup_timer((uint64_t)next_alarm);

	return (next_alarm == 0)?next_alarm:(next_alarm - time(0));
#endif
	return 0;
}

#endif

//添加按键操作,链表存储
static void divoom_app_onoff_add_key_info(void *data, DIVOOM_KEY_IO_TYPE type, uint8 add_del)
{
	if(data == NULL || pdivoom_app_power_handle == NULL)
		return;
	divoom_system_get_lock(pdivoom_app_power_handle->divoom_onoff_lock);
	switch(type)
	{
		case DIVOOM_KEY_ADC_TYPE_KEY:
			{
				Pdivoom_app_onoff_key_adc_info info = (Pdivoom_app_onoff_key_adc_info)data;
				Pdivoom_app_onoff_adc_key_list insert_info = (Pdivoom_app_onoff_adc_key_list)divoom_malloc(sizeof(divoom_app_onoff_adc_key_list), 
						DIVOOM_MODEL_ONOFF, __LINE__, __FILE__);
				insert_info->next = NULL;
				insert_info->key_num = info->key_num;
				insert_info->adc_value_max = info->adc_value_max;
				insert_info->adc_value_min = info->adc_value_min;
				insert_info->key_value = info->key_value;
				if(pdivoom_app_power_handle->adc_key_list == NULL)
				{
					pdivoom_app_power_handle->adc_key_list = insert_info;
				}
				else
				{
					insert_info->next = pdivoom_app_power_handle->adc_key_list;
					pdivoom_app_power_handle->adc_key_list = insert_info;
				}
				//不需要设置属性了,adc io在init就要设置属性
			}
			break;
		case DIVOOM_KEY_IO_TYPE_KEY:
			{
				Pdivoom_app_onoff_key_gpio_info info = (Pdivoom_app_onoff_key_gpio_info)data;
				if(add_del)
				{
					Pdivoom_app_onoff_key_list insert_info = (Pdivoom_app_onoff_key_list)divoom_malloc(sizeof(divoom_app_onoff_key_list), 
							DIVOOM_MODEL_ONOFF, __LINE__, __FILE__);
					insert_info->next = NULL;
					insert_info->key_num = info->key_num;
					insert_info->key_value = info->key_value;
					memcpy(insert_info->key_info, info->key_info, sizeof(info->key_info));
					memcpy(insert_info->active_level, info->active_level, sizeof(info->active_level));
					if(pdivoom_app_power_handle->key_list == NULL)
					{
						pdivoom_app_power_handle->key_list = insert_info;
					}
					else
					{
						/*Pdivoom_app_onoff_key_list head = pdivoom_app_power_handle->key_list;
						  while(head->next != NULL)
						  {
						  head = head->next;
						  }
						  head->next = insert_info;*/
						insert_info = pdivoom_app_power_handle->key_list;
						pdivoom_app_power_handle->key_list = insert_info;
					}

					divoom_app_onoff_set_gpio_attr(info->key_info[0], info->key_info[1]);
				}
#ifdef DIVOOM_ONOFF_TEST_MODE
				else
				{
					Pdivoom_app_onoff_key_list head = pdivoom_app_power_handle->key_list;
					Pdivoom_app_onoff_key_list pre = NULL;
					//删除这个测试模式按键
					while(head != NULL)
					{
						if(info->key_num == head->key_num && info->key_value == head->key_value && 
								info->active_level[0] == head->active_level[0] && info->active_level[1] == head->active_level[1] && 
								info->key_info[0] == head->key_info[0] && info->key_info[1] == head->key_info[1])
						{
							//删除这个节点
							pre->next = head->next;
							divoom_free(head);
						}
						pre = head;
						head = head->next;
					}
				}
#endif
			}
			break;
		case DIVOOM_KEY_IO_TYPE_IO:
			{
				Pdivoom_app_onoff_detect_gpio_info info = (Pdivoom_app_onoff_detect_gpio_info)data;
				Pdivoom_io_detect_list insert_info = (Pdivoom_io_detect_list)divoom_malloc(sizeof(divoom_io_detect_list), DIVOOM_MODEL_ONOFF, __LINE__, __FILE__);
				DIVOOM_MEM_CHECK(DIVOOM_APP_ONOFF_TAG, insert_info, return);
				memset(insert_info, 0, sizeof(divoom_io_detect_list));
				insert_info->next = NULL;
				insert_info->io_gpio = info->io_info;
				insert_info->active_level = info->active_level;
				insert_info->io_value = info->io_value;
				insert_info->pre_level = (info->active_level == 1)?(0):(1);
				if(pdivoom_app_power_handle->io_list == NULL)
				{
					pdivoom_app_power_handle->io_list = insert_info;
				}
				else
				{
					/*Pdivoom_io_detect_list head = pdivoom_app_power_handle->io_list;
					while(head->next != NULL)
					{
						head = head->next;
					}
					head->next = insert_info;*/
					insert_info = pdivoom_app_power_handle->io_list;
					pdivoom_app_power_handle->io_list = insert_info;
				}

				divoom_app_onoff_set_gpio_attr(info->io_info, 0xFF);
			}
			break;
		default:
			break;
	}
	divoom_system_free_lock(pdivoom_app_power_handle->divoom_onoff_lock);
}

//发送按键信息,直接发到main模块处理,先发给当前激活的模块,不处理的话再用全局功能
static void divoom_app_onoff_send_key_info(DIVOOM_KEY_VALUE key_value, DIVOOM_PRESS_TYPE press_type)
{
	Pdivoom_key_info send_key_info = NULL;
	divoom_sys_msg_info model_msg = {0};
	send_key_info = (Pdivoom_key_info)divoom_malloc(sizeof(divoom_key_info), DIVOOM_MODEL_ONOFF, __LINE__, __FILE__);
	DIVOOM_MEM_CHECK(DIVOOM_APP_ONOFF_TAG, send_key_info, return);
	send_key_info->value = key_value;
	send_key_info->type = press_type;

	model_msg.msg_type = 1;
	model_msg.src_id = DIVOOM_MODEL_ONOFF;
	model_msg.dest_id = DIVOOM_MODEL_MAIN;
	model_msg.msg_id = DIVOOM_MAIN_EVENT_KEY_PRESS_EVENT;
	model_msg.data = (void *)send_key_info;

	divoom_sys_model_crl_msg_push(&model_msg);
	DIVOOM_ONOFF_PRINT("%s:%d:Send key info!", __func__, __LINE__);
}
/*
   1，按下key 1，超过200ms小于2000ms，在2000ms内松开发送短按消息，超过200小于2000，限定这个按键，再有其他按键不响应，除非松开这个按键
   2，按下key 1，超过2000ms，发送长按消息，然后每隔DIVOOM_APP_KEY_LONG_PRESSING时间发送一次pressing事件，在松开这个按键之前，限定这个案件，再有其他任何按键不响应，除非松开这个按键
   3，按下key 1，在200ms以内按下组合键1,2的key 2，则开始检测组合键key1key2，在超过200ms，小于2000ms以内松开任何一个按键或者松开两个按键，发送组合键短按时间，未全部松开不检测其他按键
   4，按下key 1，在200ms以内按下组合键1,2的key 2，超过2000ms马上发送组合按键key1key2长按事件，然后每隔DIVOOM_APP_KEY_LONG_PRESSING时间发送一次pressing事件，未全部松开之前不检测其他按键，
		松开其中任何一个或者再按下其他按键，暂停发送pressing事件
*/

//都按下才为按下，如果两个按键只按下了一个，那么判断为这个按键功能不起作用
//需要讨论一下两个按键，都按下了，但是松开了其中一个，1，判断都松开了，然后检测只按下的那个？2，是否应该判断没有松开？
static uint8_t divoom_app_onoff_check_all_key_press(Pdivoom_app_onoff_key_gpio_info key_get)
{
	uint8_t ret = 1, i = 0;
	if(key_get == NULL)
		return 0;
	for(i=0;i<key_get->key_num;i++)
	{
		if(key_get->key_info[i] != 0xFF)
		{
			if(key_get->active_level[i] == gpio_get_level(key_get->key_info[i]))
				ret &= 1;
			else
				ret &= 0;
		}
	}

	//DIVOOM_ONOFF_PRINT("%s:%d------->return %d---->key_get->io_info = %d--%d", __func__, __LINE__, ret, key_get->key_info[0], key_get->key_info[1]);
	return ret;
}

//检验是否都是一样的，或者其中一个是单个按键，另一个是两个按键，单个按键包含在另一个按键里面
//或者是两个都是单个按键，重复了
//目前只支持最多两个按键
//两个相等返回3，其中一个是单个按键返回1,2
static uint8_t divoom_app_onoff_check_is_one_of_another(Pdivoom_app_onoff_key_gpio_info key_one, Pdivoom_app_onoff_key_gpio_info key_two)
{
	uint8_t i = 0;
	if(key_one == NULL || key_two == NULL)
		return 0;
	if(key_one->key_num == key_two->key_num)
	{
		if((strncmp((char *)key_one->active_level, (char *)key_two->active_level, sizeof(key_one->active_level)) == 0) 
				&& (strncmp((char *)key_one->key_info, (char *)key_two->key_info, sizeof(key_one->key_info)) == 0))
			return 3;
		return 0;
	}
	else
	{
		if(key_one->key_num == 1)
		{
			for(i=0;i<key_two->key_num;i++)
			{
				if((key_one->key_info[0] == key_two->key_info[i]) && (key_one->active_level[0] == key_two->active_level[i]))
					return 1;
			}
			return 0;
		}
		else if(key_two->key_num == 1)
		{
			for(i=0;i<key_one->key_num;i++)
			{
				if((key_two->key_info[0] == key_one->key_info[i]) && (key_two->active_level[0] == key_one->active_level[i]))
					return 2;
			}
			return 0;
		}
	}

	return 0;
}

//检测按键是否按下，不要间隔检测了
//检测到一个，再检测到的话就算出错了，这一次检测结束，进行下一次检测
static void divoom_app_onoff_key_read(Pdivoom_app_power_struct check_info, Pdivoom_app_onoff_key_gpio_info key_get)
{
	Pdivoom_app_onoff_key_list head = check_info->key_list;
	divoom_app_onoff_key_gpio_info key_check;
	while(head != NULL)
	{
		memset(&key_check, 0, sizeof(divoom_app_onoff_key_gpio_info));
		key_check.key_num = head->key_num;
		key_check.key_value = head->key_value;
		memcpy(key_check.active_level, head->active_level, sizeof(head->active_level));
		memcpy(key_check.key_info, head->key_info, sizeof(head->key_info));
		if(divoom_app_onoff_check_all_key_press(&key_check))
		{
			//vTaskDelay(DIVOOM_ONOFF_RECHECK);
			//if(divoom_app_onoff_check_all_key_press(key_get))
			if(key_get->key_num == 0xFF)
			{
				//check_info->press_time_all += DIVOOM_ONOFF_RECHECK;
				//memcpy(key_get->active_level, key_check.active_level, sizeof(key_check.active_level));
				//key_get->key_num = key_check.key_num;
				//memcpy(key_get->key_info, key_check.key_info, sizeof(key_check.key_info));
				//break;
				memcpy(key_get, &key_check, sizeof(divoom_app_onoff_key_gpio_info));
			}
			else
			{
				//要区别一下再测到的按键中是否有另一个的单独按键：比如一个注册的是k1，另一个注册的是k1k2，要区分一下，这样如果k1k2都是按下状态，那返回k1k2的按键值
				//如果检测到有两个按键，但是不是在同一个注册进来的，就置为null，不再检测了
				//1--->key_get is just one key,2-->key_check,3---> key_get == key_check
				switch(divoom_app_onoff_check_is_one_of_another(key_get, &key_check))
				{
					case 1:
						{
							memcpy(key_get, &key_check, sizeof(divoom_app_onoff_key_gpio_info));
						}
						break;
					case 2:
							//如果key_get是两个按键，key_check是一个按键，啥也不做
							//如果两个相同，什么也不做
					case 3:
						break;
					default:
						{
							//如果不相等，那证明有问题，不响应
							memset(key_get, 0xFF, sizeof(divoom_app_onoff_key_gpio_info));
							//break;
							return;
						}
				}
			}
		}
		head = head->next;
	}
}

//判断检测到的按键和之前的是否一样
//yes--》return 1，no--》return 0
static uint8_t divoom_app_onoff_check_key_info(Pdivoom_app_onoff_key_gpio_info key_get, Pdivoom_app_onoff_key_gpio_info key_keep)
{
	if(key_get != NULL && key_keep != NULL)
	{
		if((key_get->key_num == key_keep->key_num) && (strncmp((char *)key_get->key_info, (char *)key_keep->key_info, sizeof(key_get->key_info)) == 0)
				 && (strncmp((char *)key_get->active_level, (char *)key_keep->active_level, sizeof(key_get->active_level)) == 0))
		{
			return 1;
		}
	}

	return 0;
}

//发送按键事件
//目前的开关机和mode键直接处理，不需要外报
static void divoom_app_onoff_send_key_status(Pdivoom_app_power_struct check_info, Pdivoom_app_onoff_key_gpio_info key_get, DIVOOM_PRESS_TYPE press_type)
{
	if(key_get == NULL)
		return;
	//开关机键
	//要改为短按开机20180312
	DIVOOM_ONOFF_PRINT("key_get->key_info[0] = %d, press_type = %d", key_get->key_info[0], press_type);
#ifndef DIVOOM_ONOFF_NO_POWER_CONTROL
	if(key_get->key_num == 1 && key_get->key_info[0] == DIVOOM_ONOFF_CHECK_KEY)
	{
		if(check_info->divoom_onoff_key_status == 1)
		{
			//检测到开机键短按或者长按都开机
			if(press_type == DIVOOM_PRESS_SHORT || press_type == DIVOOM_PRESS_LONG)
			{
				DIVOOM_ONOFF_PRINT("Now pressing over 3S, do poweron!\n");
				//divoom_system_get_lock(check_info->divoom_onoff_lock);
				check_info->divoom_onoff_ple_poweron = 1;
				return;
			}
		}
		//开机之后短按，发出去一个
		else if(check_info->divoom_onoff_key_status == 0)
		{
			if(press_type == DIVOOM_PRESS_SHORT)
			{
				divoom_app_onoff_send_key_info(key_get->key_value, press_type);
			}
			else if(press_type == DIVOOM_PRESS_LONG)
			{
				divoom_app_onoff_sleep_pre_set();
			}
		}
	}
#endif
#ifdef DIVOOM_ONOFF_TEST_MODE
	//进入测试模式
#ifdef DIVOOM_ONOFF_NO_POWER_CONTROL
	//仅仅检测开机的时候
	else if(key_get->key_num == 1  && key_get->key_info[1] == DIVOOM_ONOFF_CHECK_MODE_KEY)
	{
		if(check_info->divoom_onoff_key_status == 1 && press_type == DIVOOM_PRESS_LONG)
		{
			//长按,开机
			DIVOOM_ONOFF_PRINT("Now going to test mode!\n");
			//divoom_system_get_lock(check_info->divoom_onoff_lock);
			check_info->divoom_onoff_ple_poweron = 3;
			//divoom_system_free_lock(check_info->divoom_onoff_lock);
			check_info->divoom_onoff_key_status = 0;
		}
	}
#else
	else if(key_get->key_num == 2 && key_get->key_info[0] == DIVOOM_ONOFF_CHECK_KEY && key_get->key_info[1] == DIVOOM_ONOFF_CHECK_MODE_KEY)
	{
		//开机之后短按，不在相应该组合按键
		//else if(check_info->divoom_onoff_kVey_status == 0)
		if(check_info->divoom_onoff_key_status == 1)
		{
			if(press_type == DIVOOM_PRESS_SHORT)
			{
				//如果短按，就继续睡眠
				check_info->divoom_onoff_ple_poweron = 2;
				return;
			}
			else if(press_type == DIVOOM_PRESS_LONG)
			{
				//长按,开机
				DIVOOM_ONOFF_PRINT("Now pressing more than 3s, and going to test mode!\n");
				//divoom_system_get_lock(check_info->divoom_onoff_lock);
				check_info->divoom_onoff_ple_poweron = 3;
				//divoom_system_free_lock(check_info->divoom_onoff_lock);
			}
			check_info->divoom_onoff_key_status = 0;
		}
	}
#endif
#endif
	//mode切换键
	//短按切换模式，长按配网
	/*else if(key_get->key_num == 1 && key_get->key_info[0] == DIVOOM_ONOFF_CHECK_MODE_KEY)
	{
		if(press_type == DIVOOM_PRESS_SHORT)
		{
			//发送事件出去处理
			//divoom_app_onoff_set_next_poweron_mode((check_info->now_power_mode==DIVOOM_APP_ONOFF_WIFI_MODE)?(DIVOOM_APP_ONOFF_BT_MODE):(DIVOOM_APP_ONOFF_WIFI_MODE), 1);
		}
		else if(press_type == DIVOOM_PRESS_LONG)
		{
			//divoom_app_onoff_set_next_poweron_mode(DIVOOM_APP_ONOFF_WIFI_CONFIG_MODE, 1);
		}
	}*/
#ifndef DIVOOM_ONOFF_NO_POWER_CONTROL
	else
#endif
	{
		//异步发送数据
		divoom_app_onoff_send_key_info(key_get->key_value, press_type);
	}
}

//发送io detect信息
static void divoom_app_onoff_send_io_info(Pdivoom_io_detect_list io_info, DIVOOM_IO_INOUT_STATE status)
{
	Pdivoom_app_onoff_io_info io_send_info = NULL;
	if(io_info == NULL)
		return;
	io_send_info = (Pdivoom_app_onoff_io_info)divoom_malloc(sizeof(divoom_app_onoff_io_info), DIVOOM_MODEL_ONOFF, __LINE__, __FILE__);
	DIVOOM_MEM_CHECK(DIVOOM_APP_ONOFF_TAG, io_send_info, return);
	io_send_info->io_value = io_info->io_value;
	io_send_info->now_status = status;

	{
		divoom_sys_msg_info model_msg = {0};
		model_msg.msg_type = 1;
		model_msg.src_id = DIVOOM_MODEL_ONOFF;
		model_msg.dest_id = DIVOOM_MODEL_MAIN;
		model_msg.msg_id = DIVOOM_MAIN_EVENT_IO_DETECT_EVENT;
		model_msg.data = (void *)io_send_info;

		divoom_sys_model_crl_msg_push(&model_msg);
	}
}

//检查io端监测时间
static void divoom_app_onoff_check_io_status(Pdivoom_io_detect_list just_one_io)
{
	//DIVOOM_ONOFF_PRINT("gpio_get_level(just_one_io->io_gpio) = %d, just_one_io->chang_time = %d, just_one_io->pre_level = %d", 
	//		gpio_get_level(just_one_io->io_gpio), just_one_io->chang_time, just_one_io->pre_level);
	if(gpio_get_level(just_one_io->io_gpio) == just_one_io->pre_level)
	{
		just_one_io->chang_time = 0;
		return;
	}
	else
	{
		just_one_io->chang_time += DIVOOM_ONOFF_DELAY_TIME;
		if(just_one_io->chang_time > DIVOOM_DETECT_IO_TIME)
		{
			//一直和前一个状态不一样，超过DIVOOM_DETECT_IO_TIME这个时间，发送事件出去
			just_one_io->pre_level = gpio_get_level(just_one_io->io_gpio);
			//insert or remove
			divoom_app_onoff_send_io_info(just_one_io, (just_one_io->active_level == gpio_get_level(just_one_io->io_gpio))?(DIVOOM_IO_ACTIVE):(DIVOOM_IO_UNACTIVE));
		}
	}
}

//默认的pre_level应该是没有接通的电平，如果当前电平和pre_level不一样，开始计时，超过500ms就认为是状态变化
static void divoom_app_onoff_io_check(Pdivoom_app_power_struct task_handle)
{
	Pdivoom_io_detect_list head = task_handle->io_list;
	while(head != NULL)
	{
		divoom_app_onoff_check_io_status(head);
		head = head->next;
	}
}

static void divoom_app_onoff_key_check(Pdivoom_app_power_struct check_info, Pdivoom_app_onoff_key_gpio_info key_get)
{
	//DIVOOM_ONOFF_PRINT("check_info->cur_press_status = %d", check_info->cur_press_status);
	check_info->press_time_all += DIVOOM_ONOFF_DELAY_TIME ;// + 3; aj 不能加３，否则信息发不出去
	switch(check_info->cur_press_status)
	{
		case DIVOOM_APP_ONOFF_CHECK_PRESS:
			{
				if(divoom_app_onoff_check_all_key_press(key_get))
				{
					check_info->cur_press_status = DIVOOM_APP_ONOFF_CONFIRM_PRESS;
				}
			}
			break;
		case DIVOOM_APP_ONOFF_CONFIRM_PRESS:
			{
				if(divoom_app_onoff_check_all_key_press(key_get))
				{
					//两次检查都是高电平
					check_info->cur_press_status = DIVOOM_APP_ONOFF_CHECK_RELEASE;
				}
				else
				{
					//毛刺
					check_info->cur_press_status = DIVOOM_APP_ONOFF_CHECK_PRESS;
				}
			}
			break;
		case DIVOOM_APP_ONOFF_CHECK_RELEASE:
			{
				//DIVOOM_ONOFF_PRINT("check_info->press_time_all = %d", check_info->press_time_all);
				//确认是否真的是按下了,一直检测是否按下
				if(divoom_app_onoff_check_all_key_press(key_get))
				{
					//check_info->press_time_all += DIVOOM_ONOFF_DELAY_TIME;
					if(check_info->press_time_all >= DIVOOM_APP_KEY_LONG_PRESS && check_info->is_long_press == 0)
					{
						check_info->is_long_press = 1;
						//当按够DIVOOM_APP_KEY_LONG_PRESSms，就直接发送长按
						divoom_app_onoff_send_key_status(check_info, key_get, DIVOOM_PRESS_LONG);
					}
					//一直按下,判断是否是刚好DIVOOM_APP_KEY_LONG_PRESSING的倍数
					else if(check_info->is_long_press == 1 && ((check_info->press_time_all % DIVOOM_APP_KEY_LONG_PRESSING) == 0))
					{
						divoom_app_onoff_send_key_status(check_info, key_get, DIVOOM_PRESSING);
					}
					else if(check_info->is_long_press == 1 &&  check_info->press_time_all >= DIVOOM_APP_KEY_LONG_LONG_PRESS)
					{
						check_info->press_time_all = 0;
						divoom_app_onoff_send_key_status(check_info, key_get, DIVOOM_PRESS_LONG_LONG);
					}
				}
				else
				{
					//检测到松开进行松开检测
					check_info->cur_press_status = DIVOOM_APP_ONOFF_CONFIRM_RELEASE;
					DIVOOM_ONOFF_PRINT("Now to release the KEY!\n");
				}
			}
			break;

		case DIVOOM_APP_ONOFF_CONFIRM_RELEASE:
			{
				if(divoom_app_onoff_check_all_key_press(key_get))
				{
					//说明是毛刺，回去继续进行高电平检测
					check_info->cur_press_status = DIVOOM_APP_ONOFF_CHECK_RELEASE;
				}
				else
				{
					//短按在松开之后再发送
					if(check_info->press_time_all >= DIVOOM_APP_KEY_SHORT_PRESS && check_info->press_time_all < DIVOOM_APP_KEY_LONG_PRESS)
					{
						divoom_app_onoff_send_key_status(check_info, key_get, DIVOOM_PRESS_SHORT);
						//check_info->divoom_onoff_ple_poweron = 2;
					}
#ifdef DIVOOM_ONOFF_TEST_MODE
					else if(check_info->press_time_all >= DIVOOM_APP_KEY_LONG_PRESS)
					{
						//这里才是正式关机
						/*if((key_get->key_num == 1) && (key_get->key_info[0] == DIVOOM_ONOFF_CHECK_KEY)
								&& (check_info->divoom_onoff_key_status == 0))
						{
							//divoom_app_onoff_start_sleep();
						}*/
						if((key_get->key_num == 2) && (key_get->key_info[0] == DIVOOM_ONOFF_CHECK_KEY) && (key_get->key_info[1] == DIVOOM_ONOFF_CHECK_MODE_KEY)
								 && (check_info->divoom_onoff_key_status == 1))
						{
							//转换为检测关机模式
							//按照正常的逻辑，开机之后，松开这俩按键，就开始检测关机了，开机之后，再按这俩按键不响应
							check_info->divoom_onoff_key_status = 0;
							//按键开机之后,要把测试模式开机删除掉,测试模式的这个io口要用于adc检测按键
							divoom_app_onoff_add_key_info(&key_get, DIVOOM_KEY_IO_TYPE_IO, 0);
						}
					}
#endif

#ifndef DIVOOM_ONOFF_NO_POWER_CONTROL
					if((key_get->key_num == 1) && (key_get->key_info[0] == DIVOOM_ONOFF_CHECK_KEY)
							&& (check_info->divoom_onoff_key_status == 1))
					{
						//转换为检测关机模式
						check_info->divoom_onoff_key_status = 0;
					}
					//按下时间不够,少于200ms，啥也不用做
					//else
					//{
					//	check_info->divoom_onoff_ple_poweron = 2;
					//}
#endif

					check_info->press_time_all = 0;
					check_info->cur_press_status = DIVOOM_APP_ONOFF_CHECK_PRESS;
					check_info->is_long_press = 0;
				}
			}
			break;
	}
}

//adc key的话就是单独单独的,如果有两个按键按下,那么对应的是不同的adc值,需要提前算好并add进来
//查询当前是那个按键按下了,就是读取adc值,跟adc按键对比
static uint8 divoom_app_onoff_check_all_adc_key_press(Pdivoom_app_onoff_key_adc_info key_get)
{
	uint16 adc_value;
	if(key_get == NULL)
		return 0;
	//read adc
	adc_value = divoom_adc_collect_key_value();
	//compare in key_get
	if(adc_value >= key_get->adc_value_min && adc_value <= key_get->adc_value_max)
	{
		return 1;
	}
	return 0;
}

//读取链表中的被按下的按键
//可以改一下,先读取一下,看是否是3.3V,只有不是3.3V的时候才进行下面的判断
static void divoom_app_onoff_adc_key_read(Pdivoom_app_power_struct check_info, Pdivoom_app_onoff_key_adc_info adc_key_get)
{
	Pdivoom_app_onoff_adc_key_list head = check_info->adc_key_list;
	divoom_app_onoff_key_adc_info key_check;
	while(head != NULL)
	{
		memset(&key_check, 0, sizeof(divoom_app_onoff_key_adc_info));
		key_check.key_num = head->key_num;
		key_check.key_value = head->key_value;
		key_check.adc_value_max = head->adc_value_max;
		key_check.adc_value_min = head->adc_value_min;
		if(divoom_app_onoff_check_all_adc_key_press(&key_check))
		{
			//对于adc key来说,都是只有一个按键存在的,所以只需要看是否是空的,并找到当前激活的按键
			if(adc_key_get->key_num == 0xFF)
			{
				memcpy(adc_key_get, &key_check, sizeof(divoom_app_onoff_key_adc_info));
			}
		}
		head = head->next;
	}
}

//检测是否相同按键 yes --> 1, no-->0
static uint8 divoom_app_onoff_check_adc_key_info(Pdivoom_app_onoff_key_adc_info adc_key_get, Pdivoom_app_onoff_key_adc_info adc_key_keep)
{
	if(adc_key_get && adc_key_keep)
	{
		if(memcmp(adc_key_get, adc_key_keep, sizeof(divoom_app_onoff_key_adc_info)) == 0)
		{
			return 1;
		}
	}
	return 0;
}

//发送adc 按键信息,不再包含开关机键,只有其他按键信息,不再包含测试按键
//直接发送按键就可以了,因为没有初始化之前,读不到任何按键值
//adc按键不进行任何特殊处理
static void divoom_app_onoff_send_adc_key_status(Pdivoom_app_power_struct check_info, Pdivoom_app_onoff_key_adc_info adc_key_get, DIVOOM_PRESS_TYPE press_type)
{
	if(adc_key_get == NULL || check_info == NULL)
		return;
	//只有在开机之后再响应
	DIVOOM_ONOFF_PRINT("ADC KEY : check_info->divoom_onoff_key_status = %d, press_type = %d", check_info->divoom_onoff_key_status, press_type);
	if(check_info->divoom_onoff_key_status == 0)
		divoom_app_onoff_send_key_info(adc_key_get->key_value, press_type);
}

//检查adc按键是否按下,去抖
static void divoom_app_onoff_adc_key_check(Pdivoom_app_power_struct check_info, Pdivoom_app_onoff_key_adc_info adc_key_get)
{
	check_info->adc_press_time_all += DIVOOM_ONOFF_DELAY_TIME + 3;
	switch(check_info->adc_cur_press_status)
	{
		case DIVOOM_APP_ONOFF_CHECK_PRESS:
			{
				if(divoom_app_onoff_check_all_adc_key_press(adc_key_get))
				{
					check_info->adc_cur_press_status = DIVOOM_APP_ONOFF_CONFIRM_PRESS;
				}
			}
			break;
		case DIVOOM_APP_ONOFF_CONFIRM_PRESS:
			{
				if(divoom_app_onoff_check_all_adc_key_press(adc_key_get))
				{
					check_info->adc_cur_press_status = DIVOOM_APP_ONOFF_CONFIRM_RELEASE;
				}
				else
				{
					check_info->adc_cur_press_status = DIVOOM_APP_ONOFF_CONFIRM_PRESS;
				}
			}
			break;
		case DIVOOM_APP_ONOFF_CHECK_RELEASE:
			{
				if(divoom_app_onoff_check_all_adc_key_press(adc_key_get))
				{
					if(check_info->adc_press_time_all >= DIVOOM_APP_KEY_LONG_PRESS && check_info->adc_is_long_press == 0)
					{
						check_info->adc_is_long_press = 1;
						divoom_app_onoff_send_adc_key_status(check_info, adc_key_get, DIVOOM_PRESS_LONG);
					}
					//一直按下,判断是否是刚好DIVOOM_APP_KEY_LONG_PRESSING的倍数
					else if(check_info->adc_is_long_press == 1 && ((check_info->adc_press_time_all % DIVOOM_APP_KEY_LONG_PRESSING) == 0))
					{
						divoom_app_onoff_send_adc_key_status(check_info, adc_key_get, DIVOOM_PRESSING);
					}
					else if(check_info->adc_is_long_press == 1 &&  check_info->adc_press_time_all >= DIVOOM_APP_KEY_LONG_LONG_PRESS)
					{
						check_info->adc_press_time_all = 0;
						divoom_app_onoff_send_adc_key_status(check_info, adc_key_get, DIVOOM_PRESS_LONG_LONG);
					}
				}
				else
				{
					//检测到松开进行松开检测
					check_info->adc_cur_press_status = DIVOOM_APP_ONOFF_CONFIRM_RELEASE;
					DIVOOM_ONOFF_PRINT("Now to release the ADC KEY!\n");
				}
			}
			break;
		case DIVOOM_APP_ONOFF_CONFIRM_RELEASE:
			{
				if(divoom_app_onoff_check_all_adc_key_press(adc_key_get))
				{
					//说明是毛刺，回去继续进行高电平检测
					check_info->adc_cur_press_status = DIVOOM_APP_ONOFF_CHECK_RELEASE;
				}
				else
				{
					//短按在松开之后再发送
					//符合短按特征
					//只负责短按处理,不负责开关机,测试模式等等
					if(check_info->adc_press_time_all >= DIVOOM_APP_KEY_SHORT_PRESS && check_info->adc_press_time_all < DIVOOM_APP_KEY_LONG_PRESS)
					{
						divoom_app_onoff_send_adc_key_status(check_info, adc_key_get, DIVOOM_PRESS_SHORT);
					}
				}
			}
			break;
	}
}

//检查关健健按下之后时间持续
//检测过程中出现两个都按下就从头检测
//pixoowifi需求：按下开机键开机，关机需要长按，mode键，短按切换模式，长按模式内切换，超长按切换WiFi和配网
//开机逻辑：当按下开机键，即开始给芯片供电，芯片上电需要把ON/OFF_IO置高,之后开始检测开关机键的电平，按下超过一段时间就把ON/OFF_IO拉低，松手之后芯片断电
//ON/OFF_IO置高操作在开机之后就响应了，仅需要把关机的动作做了就行
//mode按键正常检测
//睡眠功能全砍掉
static void divoom_app_onoff_task_check_key(void *arg)
{
#ifdef DIVOOM_ONOFF_USE_LOOP_ASK
	uint8 task_state;
	Pdivoom_app_power_struct task_handle = (Pdivoom_app_power_struct)arg;
	divoom_app_onoff_key_gpio_info key_get, key_keep;
	divoom_app_onoff_key_adc_info adc_key_get, adc_key_keep;
	//在完全松开之后在清除key_get,key_keep
	memset(&key_get, 0, sizeof(divoom_app_onoff_key_gpio_info));
	memset(&key_keep, 0, sizeof(divoom_app_onoff_key_gpio_info));
	//使用同样的逻辑,如果变化了就切换为新的按键
	memset(&adc_key_get, 0, sizeof(divoom_app_onoff_key_gpio_info));
	memset(&adc_key_keep, 0, sizeof(divoom_app_onoff_key_gpio_info));
	//task_handle->press_time_all = 0;
	//1，先去读取那些按键被按下
	//2，按照被按下的按键去读取相应的gpio是否导通
	//3，检查检测口状态是否变化（USB等）
	//4,(20190419)关于开关机按键的都要去除掉了,不需要了
	while(task_handle->divoom_task_run_flag)
	{
		divoom_system_get_lock(pdivoom_app_power_handle->divoom_onoff_lock);
		//DIVOOM_ONOFF_PRINT("task_handle->press_time_all = %d, task_handle->key_press = %d", task_handle->press_time_all, task_handle->divoom_onoff_ple_poweron);
		//在200ms检测一个按键，变化了从新开始检测，检测到没有的就全部清除掉
		//按同一个按键结构体，超过200ms，就只检测这一个，变化了就发松开
		divoom_app_onoff_io_check(task_handle);
		//处理gpio按键开始
		if((task_handle->press_time_all <= DIVOOM_APP_KEY_SHORT_PRESS))
		{
			memset(&key_get, 0xFF, sizeof(divoom_app_onoff_key_gpio_info));
			//memset(&key_keep, 0xFF, sizeof(divoom_app_onoff_key_gpio_info));
			divoom_app_onoff_key_read(task_handle, &key_get);
			if(key_get.key_num == 0xFF)
			{
				task_handle->press_time_all = 0;
				task_handle->cur_press_status = DIVOOM_APP_ONOFF_CHECK_PRESS;
				task_handle->is_long_press = 0;
				//divoom_system_free_lock(pdivoom_app_power_handle->divoom_onoff_lock);
				//DIVOOM_ONOFF_PRINT("key_get:%d", key_get.key_info[0]);
				//vTaskDelay(DIVOOM_ONOFF_DELAY_TIME);
				//continue;
			}
			//在200ms内变化了，从心开始检测
			else
			{
				//检查是否改变了，是否按下时间超过200ms
				//如果在按下的时间少于200ms并且变化了，变更为检测最新的检测值
				//如果前一次已经把他存起来了，这次keep get和keep是一样的，就不管了，如果不一样，看时间是否要更新
				if(divoom_app_onoff_check_key_info(&key_get, &key_keep) == 0)
				{
					task_handle->press_time_all = 0;
					task_handle->cur_press_status = DIVOOM_APP_ONOFF_CHECK_PRESS;
					task_handle->is_long_press = 0;
				}
				//清除adc按键相关信息
				task_handle->adc_cur_press_status = 0;
				task_handle->adc_press_time_all = 0;
				task_handle->adc_is_long_press = 0;
			}
			memcpy(&key_keep, &key_get, sizeof(divoom_app_onoff_key_gpio_info));
		}
		//DIVOOM_ONOFF_PRINT("key_get.key_info = %d, task_handle->press_time_all = %d", key_get.key_info[0], task_handle->press_time_all);
		divoom_app_onoff_key_check(task_handle, &key_get);
		//处理gpio按键结束
		//处理adc key开始
		if((task_handle->adc_press_time_all <= DIVOOM_APP_KEY_SHORT_PRESS))
		{
			memset(&adc_key_get, 0xFF, sizeof(divoom_app_onoff_key_gpio_info));
			divoom_app_onoff_adc_key_read(task_handle, &adc_key_get);
			if(adc_key_get.key_num == 0xFF)
			{
				task_handle->adc_press_time_all = 0;
				task_handle->adc_cur_press_status = DIVOOM_APP_ONOFF_CHECK_PRESS;
				task_handle->adc_is_long_press = 0;
			}
			//在200ms内变化了，从新开始检测adc
			else
			{
				//检查是否改变了，是否按下时间超过200ms
				//如果在按下的时间少于200ms并且变化了，变更为检测最新的检测值
				//如果前一次已经把他存起来了，这次keep get和keep是一样的，就不管了，如果不一样，看时间是否要更新
				if(divoom_app_onoff_check_adc_key_info(&adc_key_get, &adc_key_keep) == 0)
				{
					task_handle->adc_press_time_all = 0;
					task_handle->adc_cur_press_status = DIVOOM_APP_ONOFF_CHECK_PRESS;
					task_handle->adc_is_long_press = 0;
				}
				task_handle->adc_cur_press_status = 0;
				task_handle->adc_press_time_all = 0;
				task_handle->adc_is_long_press = 0;
			}
			memcpy(&adc_key_keep, &adc_key_get, sizeof(divoom_app_onoff_key_adc_info));
		}
		divoom_app_onoff_adc_key_check(task_handle, &adc_key_get);
		//处理adc key 结束
		divoom_system_free_lock(pdivoom_app_power_handle->divoom_onoff_lock);
		//增加这个判断，避免出现vTaskDelay里面configASSERT( uxSchedulerSuspended[ xPortGetCoreID() ] == 0 )这里出错
		task_state = xTaskGetSchedulerState();
		if(task_state == taskSCHEDULER_RUNNING)
		{
			vTaskDelay(DIVOOM_ONOFF_DELAY_TIME);
		}
		else if(task_state == taskSCHEDULER_NOT_STARTED)
		{
			DIVOOM_ONOFF_PRINT("---------Cur task is taskSCHEDULER_NOT_STARTED!!!!\n");
		}
		else
		{
			DIVOOM_ONOFF_PRINT("+++++++++Cur task is taskSCHEDULER_SUSPENDED!!!!\n");
		}
	}
#else
	uint8  run_flag = 1;
	uint8  off_status = DIVOOM_APP_ONOFF_CHECK_PRESS;
	uint8  Is_send_sleep_pre = 0;
	uint32 key_info = 0;
	uint32 all_time = 0;
	Pdivoom_app_power_struct task_handle = (Pdivoom_app_power_struct)arg;
	while(task_handle->divoom_task_run_flag)
	{
		key_info = 0;
		while(xQueueReceive(task_handle->divoom_onoff_queue, &key_info, portMAX_DELAY))
		{
			Is_send_sleep_pre = 0;
			run_flag = 1;
			off_status = DIVOOM_APP_ONOFF_CHECK_PRESS;
			all_time = 0;
			DIVOOM_ONOFF_PRINT("key_info = %d\n", key_info);
			if(key_info == DIVOOM_ONOFF_CHECK_KEY)
			{
				//先取消中断，如果时间不够，再打开中断
				divoom_app_onoff_remove_isr(DIVOOM_ONOFF_CHECK_KEY);
				//while(all_time <= DIVOOM_ONOFF_CHECK_TIME)
				while(run_flag)
				{
					switch(off_status)
					{
						case DIVOOM_APP_ONOFF_CHECK_PRESS:
							{
								if(gpio_get_level(DIVOOM_ONOFF_CHECK_KEY))
								{
									off_status = DIVOOM_APP_ONOFF_CONFIRM_PRESS;
								}
								else
								{
									//收到中断第一次检查还是低电平就有可能是毛刺
									run_flag = 0;
									DIVOOM_ONOFF_PRINT("Now has some wrong interrupt!\n");
								}
							}
							break;
						case DIVOOM_APP_ONOFF_CONFIRM_PRESS:
							{
								if(gpio_get_level(DIVOOM_ONOFF_CHECK_KEY))
								{
									//说明是毛刺，回去继续进行高电平检测
									off_status = DIVOOM_APP_ONOFF_CHECK_RELEASE;
								}
								else
								{
									//不能确认是按下就退出了
									run_flag = 0;
								}
							}
							break;
						case DIVOOM_APP_ONOFF_CHECK_RELEASE:
							{
								//确认是否真的是按下了,一直检测是否按下
								if(gpio_get_level(DIVOOM_ONOFF_CHECK_KEY))
								{
									//all_time += 10ms
									if(Is_send_sleep_pre == 0)
										all_time += DIVOOM_ONOFF_DELAY_TIME * portTICK_PERIOD_MS;
									if(all_time >= DIVOOM_ONOFF_CHECK_TIME && Is_send_sleep_pre == 0)
									{
										Is_send_sleep_pre = 1;
										//divoom_app_onoff_sleep_pre_set(1, 0);
										//divoom_app_onoff_sleep_pre_set();
									}
								}
								else
								{
									//检测到松开进行松开检测
									off_status = DIVOOM_APP_ONOFF_CONFIRM_RELEASE;
									DIVOOM_ONOFF_PRINT("Now to release the KEY!\n");
								}
							}
							break;

						case DIVOOM_APP_ONOFF_CONFIRM_RELEASE:
							{
								if(gpio_get_level(DIVOOM_ONOFF_CHECK_KEY))
								{
									//说明是毛刺，回去继续进行高电平检测
									off_status = DIVOOM_APP_ONOFF_CHECK_RELEASE;
								}
								else
								{
									//确定是松开按键了,并且时间超过3秒,进入睡眠
									run_flag = 0;
									if(Is_send_sleep_pre == 1)
									{
										DIVOOM_ONOFF_PRINT("Now To Sleep!\n");
										//divoom_app_onoff_start_sleep();
									}
									/*else
									{
										//时间不够就继续进行假关机检测
										run_flag = 0;
										divoom_app_onoff_set_isr_gpio(DIVOOM_ONOFF_CHECK_KEY, divoom_app_onoff_isr_func);
									}*/
								}
							}
							break;
					}
					vTaskDelay(DIVOOM_ONOFF_DELAY_TIME);
				}
				divoom_app_onoff_set_isr_gpio(DIVOOM_ONOFF_CHECK_KEY, divoom_app_onoff_isr_func);
			}
		}
	}
#endif
}

//设置切换模式的nvs值
static void divoom_app_onoff_set_is_channge_mode(uint8 is_channge_mode)
{
	if(pdivoom_app_power_handle == NULL)
	{
		DIVOOM_ONOFF_PRINT("Some Wrong In Here:%d\n", __LINE__);
		return;
	}
	if(nvs_open(DIVOOM_ONOFF_NVS_NAME, NVS_READWRITE, &pdivoom_app_power_handle->divoom_nvs_handle) != ESP_OK)
	{
		nvs_close(pdivoom_app_power_handle->divoom_nvs_handle);
		DIVOOM_ONOFF_PRINT("Some Wrong In Here:%d\n", __LINE__);
		return;
	}
	if(nvs_set_u8(pdivoom_app_power_handle->divoom_nvs_handle, DIVOOM_ONOFF_IS_CHANNGE_NVS, is_channge_mode) != ESP_OK)
	{
		nvs_close(pdivoom_app_power_handle->divoom_nvs_handle);
		DIVOOM_ONOFF_PRINT("Some Wrong In Here:%d\n", __LINE__);
		return;
	}

	nvs_close(pdivoom_app_power_handle->divoom_nvs_handle);
}

//设置下一个开机时的模式：WiFi or BT(just classic bt)
static void divoom_app_onoff_set_mode(uint8 now_power_mode)
{
	if(pdivoom_app_power_handle == NULL)
	{
		DIVOOM_ONOFF_PRINT("Some Wrong In Here:%d\n", __LINE__);
		return;
	}
	if(nvs_open(DIVOOM_ONOFF_NVS_NAME, NVS_READWRITE, &pdivoom_app_power_handle->divoom_nvs_handle) != ESP_OK)
	{
		nvs_close(pdivoom_app_power_handle->divoom_nvs_handle);
		DIVOOM_ONOFF_PRINT("Some Wrong In Here:%d\n", __LINE__);
		return;
	}
	if(nvs_set_u8(pdivoom_app_power_handle->divoom_nvs_handle, DIVOOM_ONOFF_CHANGE_MODE_NVS, now_power_mode) != ESP_OK)
	{
		nvs_close(pdivoom_app_power_handle->divoom_nvs_handle);
		DIVOOM_ONOFF_PRINT("Some Wrong In Here:%d\n", __LINE__);
		return;
	}

	nvs_close(pdivoom_app_power_handle->divoom_nvs_handle);
}

//这个时间是得到的时间差值+当前的rtc时间
//设置下一个唤醒时间
//时间从alarm获取
static void divoom_app_onoff_set_next_wakeup_timer(uint64_t out_value)
{
	if(pdivoom_app_power_handle == NULL)
	{
		DIVOOM_ONOFF_PRINT("Some Wrong In Here:%d\n", __LINE__);
		return;
	}
	if(nvs_open(DIVOOM_ONOFF_NVS_NAME, NVS_READWRITE, &pdivoom_app_power_handle->divoom_nvs_handle) != ESP_OK)
	{
		nvs_close(pdivoom_app_power_handle->divoom_nvs_handle);
		DIVOOM_ONOFF_PRINT("Some Wrong In Here:%d\n", __LINE__);
		return;
	}
	if(nvs_set_u64(pdivoom_app_power_handle->divoom_nvs_handle, DIVOOM_ONOFF_NEXT_TIMER_NVS, out_value) != ESP_OK)
	{
		nvs_close(pdivoom_app_power_handle->divoom_nvs_handle);
		DIVOOM_ONOFF_PRINT("Some Wrong In Here:%d\n", __LINE__);
		return;
	}

	nvs_close(pdivoom_app_power_handle->divoom_nvs_handle);

	return;
}

#if 1
//这个时间是得到的时间差值+当前的rtc时间
//获取当前开机模式
//同时获取下一个闹钟时间
static uint8 divoom_app_onoff_get_ischange_mode_power_mode(uint8 *out_value, uint8 *now_power_mode, uint64_t * next_alarm_time)
{
	int32_t ret = 0;
	if(pdivoom_app_power_handle == NULL)
	{
		DIVOOM_ONOFF_PRINT("Some Wrong In Here:%d\n", __LINE__);
		return 0;
	}
	if((ret = nvs_open(DIVOOM_ONOFF_NVS_NAME, NVS_READONLY, &pdivoom_app_power_handle->divoom_nvs_handle)) != ESP_OK)
	{
		nvs_close(pdivoom_app_power_handle->divoom_nvs_handle);
		DIVOOM_ONOFF_PRINT("Some Wrong In Here:%d, ret = %d\n", __LINE__, ret);
		return 0;
	}
	if((ret = nvs_get_u8(pdivoom_app_power_handle->divoom_nvs_handle, DIVOOM_ONOFF_IS_CHANNGE_NVS, out_value)) != ESP_OK)
	{
		DIVOOM_ONOFF_PRINT("Some Wrong In Here:%d, ret = %d\n", __LINE__, ret);
		*out_value = 0;
	}
	if((ret = nvs_get_u8(pdivoom_app_power_handle->divoom_nvs_handle, DIVOOM_ONOFF_CHANGE_MODE_NVS, now_power_mode)) != ESP_OK)
	{
		DIVOOM_ONOFF_PRINT("Some Wrong In Here:%d, ret = %d\n", __LINE__, ret);
	}
	if((ret = nvs_get_u64(pdivoom_app_power_handle->divoom_nvs_handle, DIVOOM_ONOFF_NEXT_TIMER_NVS, next_alarm_time)) != ESP_OK)
	{
		DIVOOM_ONOFF_PRINT("Some Wrong In Here:%d, ret = %d\n", __LINE__, ret);
	}

	nvs_close(pdivoom_app_power_handle->divoom_nvs_handle);

	return 1;
}
#endif

//增加按键检测,放在开机之后在添加,避免对测试模式有影响
void divoom_app_onoff_add_key_and_detect(void)
{
	divoom_app_onoff_detect_gpio_info detect_io;
	divoom_app_onoff_key_gpio_info mode_key;
	//仅仅为adc按键
	divoom_app_onoff_key_adc_info adc_key;

#if !defined SHACO_TEST
	//adc按键列表.暂时随便添加点
	adc_key.key_num = 34;
	adc_key.key_value = DIVOOM_KEY_CHANNEL_NEXT;
	adc_key.adc_value_max = 1380;
	adc_key.adc_value_min = 1280;
	divoom_app_onoff_add_key_info(&adc_key, DIVOOM_KEY_ADC_TYPE_KEY, 1);
#endif

#ifndef DIVOOM_SCREENHANDLE_SPI_RECV
//天花板项目不需要这个检查线了,同时上点的
	//添加gpio检测关机
	//默认是低电平,1是开机了,发送开机命令,0是准备关机了
	detect_io.active_level = 1;
	detect_io.io_info = DIVOOM_SPI_PIN_NUM_MISO;
	detect_io.io_value = DIVOOM_POWER_DETECT;
	divoom_app_onoff_add_key_info(&detect_io, DIVOOM_KEY_IO_TYPE_IO, 1);
#endif

#if 0
	detect_io.active_level = 1;
	detect_io.io_info = RESET_IO;
	detect_io.io_value = DIVOOM_RESET_DETECT;
	divoom_app_onoff_add_key_info(&detect_io, DIVOOM_KEY_IO_TYPE_IO, 1);
#endif

#if !defined SHACO_TEST
	//添加模式切换键
	mode_key.key_num = 1;
	mode_key.key_info[0] = DIVOOM_ONOFF_CHECK_MODE_KEY;
	mode_key.key_info[1] = 0xFF;
	mode_key.active_level[0] = 0;
	mode_key.active_level[1] = 0xFF;
	mode_key.key_value = DIVOOM_KEY_MODE;
	divoom_app_onoff_add_key_info(&mode_key, DIVOOM_KEY_IO_TYPE_KEY, 1);
#endif
}

void divoom_app_onoff_check_init(void)
{
	uint8 sleep_rest_reason = 0;
#ifndef DIVOOM_ONOFF_NO_POWER_CONTROL
	divoom_app_onoff_key_gpio_info power_on_key;
#endif
	pdivoom_app_power_handle = (Pdivoom_app_power_struct)divoom_malloc(sizeof(divoom_app_power_struct), DIVOOM_MODEL_ONOFF, __LINE__, __FILE__);
	DIVOOM_MEM_CHECK(DIVOOM_APP_ONOFF_TAG, pdivoom_app_power_handle, return);
	memset(pdivoom_app_power_handle, 0, sizeof(divoom_app_power_struct));
	//开始给个初始值，下面读取，读不到再给默认值
	pdivoom_app_power_handle->now_power_mode = 0xFF;

#ifndef DIVOOM_ONOFF_USE_LOOP_ASK
	pdivoom_app_power_handle->key_info = divoom_malloc(sizeof(uint32), DIVOOM_MODEL_ONOFF, __LINE__, __FILE__);
	DIVOOM_MEM_CHECK(DIVOOM_APP_ONOFF_TAG, pdivoom_app_power_handle->key_info, return);

	pdivoom_app_power_handle->alarm_is_open_flag = 0;
	pdivoom_app_power_handle->divoom_onoff_queue = xQueueCreate(10, sizeof(uint32));
	if(pdivoom_app_power_handle->divoom_onoff_queue == NULL)
	{
		divoom_free_line(pdivoom_app_power_handle->key_info, __FILE__, __LINE__);
		divoom_free_line(pdivoom_app_power_handle, __FILE__, __LINE__);
		return;
	}
#endif
	if(divoom_app_onoff_get_ischange_mode_power_mode(&pdivoom_app_power_handle->is_channge_mode, &pdivoom_app_power_handle->now_power_mode, &pdivoom_app_power_handle->next_timer) != 1)
	{
		DIVOOM_ONOFF_PRINT("%s:%d:Can't read time from nvs.May be first power on.\n", __func__, __LINE__);
	}
	divoom_print("pdivoom_app_power_handle->is_channge_mode = %d, pdivoom_app_power_handle->now_power_mode = %d, pdivoom_app_power_handle->next_timer = %lld\n", 
			pdivoom_app_power_handle->is_channge_mode, pdivoom_app_power_handle->now_power_mode, pdivoom_app_power_handle->next_timer);

	pdivoom_app_power_handle->divoom_onoff_lock = divoom_system_create_lock();
	if(pdivoom_app_power_handle->divoom_onoff_lock == NULL)
	{
#ifndef DIVOOM_ONOFF_USE_LOOP_ASK
		vQueueDelete(pdivoom_app_power_handle->divoom_onoff_queue);
		divoom_free_line(pdivoom_app_power_handle->key_info, __FILE__, __LINE__);
#endif
		divoom_free_line(pdivoom_app_power_handle, __FILE__, __LINE__);
	}

#ifndef DIVOOM_ONOFF_NO_POWER_CONTROL
	pdivoom_app_power_handle->divoom_power_on_status = DIVOOM_POWER_ON_OTHER;
#endif
	pdivoom_app_power_handle->divoom_onoff_key_status = 1;
	pdivoom_app_power_handle->divoom_onoff_ple_poweron = 0;

	//放在开机的地方处理
	pdivoom_app_power_handle->divoom_task_run_flag = 1;

	//测试阶段禁用这个宏，默认是进入烧录
#ifdef DIVOOM_ONOFF_TEST_MODE
	//增加开机检测进入测试模式
	power_on_key.key_num = 1;
	power_on_key.key_info[0] = DIVOOM_ONOFF_CHECK_KEY;
	power_on_key.key_info[1] = DIVOOM_ONOFF_CHECK_MODE_KEY;
	power_on_key.active_level[0] = 0;
	power_on_key.active_level[1] = 0;
	power_on_key.key_value = DIVOOM_KEY_ALL_TEST;

	divoom_app_onoff_add_key_info(&power_on_key, DIVOOM_KEY_IO_TYPE_KEY, 1);
#endif
#ifndef DIVOOM_ONOFF_NO_POWER_CONTROL
	//添加开关机按键的检测
	power_on_key.key_num = 1;
	power_on_key.key_info[0] = DIVOOM_ONOFF_CHECK_KEY;
	power_on_key.key_info[1] = 0xFF;
#ifdef SHACO_TEST
	power_on_key.active_level[0] = 1;
#else
	power_on_key.active_level[0] = 0;
#endif
	power_on_key.active_level[1] = 255;
	power_on_key.key_value = DIVOOM_KEY_POWER;

	divoom_app_onoff_add_key_info(&power_on_key, DIVOOM_KEY_IO_TYPE_KEY, 1);
#endif

#ifdef DIVOOM_TASK_PRINTF_STACK_INFO
	pdivoom_app_power_handle->divoom_onoff_handle = divoom_system_create_task_prio(divoom_app_onoff_task_check_key, "onoff_task_check", (void *)pdivoom_app_power_handle, DIVOOM_ONOFF_TASK_STACK, 1);
#else
	divoom_system_create_task_prio(divoom_app_onoff_task_check_key, "onoff_task_check", (void *)pdivoom_app_power_handle, DIVOOM_ONOFF_TASK_STACK, 1);
#endif

	divoom_system_get_lock(pdivoom_app_power_handle->divoom_onoff_lock);
	if(pdivoom_app_power_handle->is_channge_mode == 1)
	{
		//切换模式开机
#ifndef DIVOOM_ONOFF_NO_POWER_CONTROL
		pdivoom_app_power_handle->divoom_power_on_status = DIVOOM_POWER_ON_OTHER;
#endif
		if(pdivoom_app_power_handle->now_power_mode == DIVOOM_APP_ONOFF_WIFI_MODE)
		{
			esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);
			//20190708,没有连上WiFi,不释放BLE,直到连上WiFi在释放BLE的内存
			//esp_bt_controller_mem_release(ESP_BT_MODE_BLE);
			//pdivoom_app_power_handle->divoom_now_mode = 0;
		}
		else if(pdivoom_app_power_handle->now_power_mode == DIVOOM_APP_ONOFF_WIFI_CONFIG_MODE)
		{
			esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);
			//pdivoom_app_power_handle->divoom_now_mode = 0;
		}
#ifndef DIVOOM_ONOFF_NO_POWER_CONTROL
		pdivoom_app_power_handle->divoom_onoff_key_status = 0;
		pdivoom_app_power_handle->divoom_onoff_ple_poweron = 1;
#endif
		pdivoom_app_power_handle->is_channge_mode = 0;
		divoom_app_onoff_set_is_channge_mode(pdivoom_app_power_handle->is_channge_mode);
	}
#ifndef DIVOOM_ONOFF_NO_POWER_CONTROL
	else
	{
		//不是切换模式开机就默认给为2,适用于开发板会自动上电开机-->1845行
		pdivoom_app_power_handle->divoom_onoff_ple_poweron = 2;
		//如果不是切换模式开机,那么就只能是按键开机和闹钟开机
		sleep_rest_reason = esp_sleep_get_wakeup_cause();
		switch(sleep_rest_reason)
		{
			case ESP_SLEEP_WAKEUP_EXT1:
				{
					//不需要检测是否开机
					uint64_t wakeup_pin_mask = esp_sleep_get_ext1_wakeup_status();
					divoom_print("wakeup_pin_mask = %lld\n", wakeup_pin_mask);
					//最小是1<<0位
					if(wakeup_pin_mask != 0)
					{
						uint32 pin;
						pin = __builtin_ffsll(wakeup_pin_mask) - 1;
						DIVOOM_ONOFF_PRINT("divoom_wakeup by pin %d\n", pin);
						pdivoom_app_power_handle->divoom_power_on_status = DIVOOM_POWER_ON_BY_KEY;
					}
					else
					{
						DIVOOM_ONOFF_PRINT("Some wrong wakeup in %s:%d\n", __func__, __LINE__);
						goto DIVOOM_APP_ONOFF_GOTO_SLEEP;
					}
					//留给task进行开机判断
					break;
				}
			case ESP_SLEEP_WAKEUP_TIMER:
				{
					//如果是定时器开机，关机闹钟，直接开机
					DIVOOM_ONOFF_PRINT("Open because timer & poweroff alarm:expect time:%lld, reall time:%lld\n", pdivoom_app_power_handle->next_timer, esp_clk_rtc_time());
					pdivoom_app_power_handle->alarm_is_open_flag = 1;
					pdivoom_app_power_handle->divoom_onoff_ple_poweron = 1;
					pdivoom_app_power_handle->divoom_power_on_status = DIVOOM_POWER_ON_OTHER;
					break;
				}
			default:
				{
					//不是按键开机也不是闹钟和切换模式开机的话,就是出错了,直接到睡眠
					DIVOOM_ONOFF_PRINT("HERE POWER ON NO KYE UP, ALARM UP, and MODE CHANGE UP!\n");
					goto DIVOOM_APP_ONOFF_GOTO_SLEEP;
				}
		}
		pdivoom_app_power_handle->divoom_onoff_ple_poweron = 1;
	}
#endif
	divoom_system_free_lock(pdivoom_app_power_handle->divoom_onoff_lock);

	//清除存在nvs里面的配置
	if(pdivoom_app_power_handle->now_power_mode == 255)
	{
		//如果是第一次开机，默认设置为配网状态
#ifdef DIVOOM_WIFI_DEBUG_CONFIG
		pdivoom_app_power_handle->now_power_mode = DIVOOM_APP_ONOFF_WIFI_MODE;
#else
		pdivoom_app_power_handle->now_power_mode = DIVOOM_APP_ONOFF_WIFI_CONFIG_MODE;
#endif
	}
	//divoom_app_onoff_set_mode(255);

	//设置这个按键为关机键,上升沿触发模式，检测到上升沿就发按键到队列，在task里面检测是否一直是高电平
#ifndef DIVOOM_ONOFF_USE_LOOP_ASK
	if(divoom_app_onoff_set_isr_gpio(DIVOOM_ONOFF_CHECK_KEY, divoom_app_onoff_isr_func) != ESP_OK)
	{
		divoom_system_destroy_lock(pdivoom_app_power_handle->divoom_onoff_lock);
		vQueueDelete(pdivoom_app_power_handle->divoom_onoff_queue);
		divoom_free_line(pdivoom_app_power_handle->key_info, __FILE__, __LINE__);
		divoom_free_line(pdivoom_app_power_handle, __FILE__, __LINE__);
		DIVOOM_ONOFF_PRINT("%s:%d,Some wrong in here.\n", __func__, __LINE__);
	}
#endif

	return;
DIVOOM_APP_ONOFF_GOTO_SLEEP:
	DIVOOM_ONOFF_PRINT("%s:%d:Now to goto sleep!\n", __func__, __LINE__);
	divoom_app_onoff_set_rtcgpio_wakeup(DIVOOM_ONOFF_CHECK_KEY);
	divoom_app_onoff_set_timer_wakeup(pdivoom_app_power_handle->next_timer);
	divoom_app_onoff_start_sleep();
	return;
}

#ifdef DIVOOM_TASK_PRINTF_STACK_INFO
void divoom_app_onoff_check_task_stack_info(void)
{
	if(pdivoom_app_power_handle && pdivoom_app_power_handle->divoom_onoff_handle)
	{
		ESP_LOGE("SHACO-DEBUG", "This task is onoff_task_check free stack:%d\n", uxTaskGetStackHighWaterMark(pdivoom_app_power_handle->divoom_onoff_handle));
	}
}
#endif

//等待检测够3秒
//默认返回1为正常开机，返回2为进入测试模式
//需要增加开机检测按键是否按下开机
#ifdef DIVOOM_ONOFF_TEST_MODE
int divoom_app_onoff_wait_check_init(void)
#else
void divoom_app_onoff_wait_check_init(void)
#endif
{
	//等到按键按下三秒，或者不够三秒，进入睡眠
	if(pdivoom_app_power_handle == NULL)
	{
#ifdef DIVOOM_ONOFF_TEST_MODE
		return 1;
#else
		return;
#endif
	}
#ifdef DIVOOM_ONOFF_NO_POWER_CONTROL
#ifdef DIVOOM_ONOFF_TEST_MODE
	if(check_info->divoom_onoff_ple_poweron == 3)
	{
		return 2;
	}
	return 1;
#else
	return;
#endif
#else
	uint8_t now_ple_poweron = 0;
	while(1)
	{
		//divoom_system_get_lock(pdivoom_app_power_handle->divoom_onoff_lock);
		now_ple_poweron = pdivoom_app_power_handle->divoom_onoff_ple_poweron;
		//divoom_system_free_lock(pdivoom_app_power_handle->divoom_onoff_lock);
		if(now_ple_poweron == 1)
		{
			//可以开机，退出循环
			break;
		}
		else if(now_ple_poweron == 2)
		//else
		{
			//按的时间不够，去sleep
			goto DIVOOM_APP_ONOFF_GOTO_SLEEP;
		}
#ifdef DIVOOM_ONOFF_TEST_MODE
		//这里是进入测试模式
		else if(now_ple_poweron == 3)
		{
			return 2;
		}
#endif
		vTaskDelay(DIVOOM_ONOFF_DELAY_TIME / 2);
	}
#ifdef DIVOOM_ONOFF_TEST_MODE
	return 1;
#else
	return;
#endif

DIVOOM_APP_ONOFF_GOTO_SLEEP:
	DIVOOM_ONOFF_PRINT("%s:%d:Now to goto sleep!\n", __func__, __LINE__);
	divoom_app_onoff_set_rtcgpio_wakeup(DIVOOM_ONOFF_CHECK_KEY);
//	divoom_app_onoff_set_timer_wakeup(pdivoom_app_power_handle->next_timer);
	divoom_app_onoff_start_sleep();
#ifdef DIVOOM_ONOFF_TEST_MODE
	return 1;
#else
	return;
#endif
#endif
}

uint8 divoom_app_onoff_event(Pdivoom_sys_msg_info msg)
{
	if(pdivoom_app_power_handle == NULL)
		return 0;
	DIVOOM_ONOFF_PRINT("divoom_app_onoff_event:%d", msg->msg_id);
	switch(msg->msg_id)
	{
		case DIVOOM_APP_ONOFF_CHANGE_MODE:
			{
				divoom_app_onoff_set_next_poweron_mode((pdivoom_app_power_handle->now_power_mode == DIVOOM_APP_ONOFF_WIFI_MODE)?(DIVOOM_APP_ONOFF_WIFI_CONFIG_MODE):(DIVOOM_APP_ONOFF_WIFI_MODE), 1);
			}
			break;
		case DIVOOM_APP_ONOFF_CONFIG_WIFI_MODE:
			{
				divoom_app_onoff_set_next_poweron_mode(DIVOOM_APP_ONOFF_WIFI_CONFIG_MODE, 1);
			}
			break;
		case DIVOOM_APP_ONOFF_SET_KEY_INFO:
			{
				divoom_app_onoff_add_key_info(msg->data, DIVOOM_KEY_IO_TYPE_KEY, 1);
			}
			break;
		case DIVOOM_APP_ONOFF_SET_IO_INFO:
			{
				divoom_app_onoff_add_key_info(msg->data, DIVOOM_KEY_IO_TYPE_IO, 1);
			}
			break;
		case DIVOOM_APP_ONOFF_SOFT_POWEROFF:
			{
#ifndef DIVOOM_ONOFF_NO_POWER_CONTROL
				//软件关机,直接把DIVOOM_ONOFF_POWERON_POWER拉低
				divoom_app_onoff_start_sleep();
#endif
			}
			break;
	}

	return 1;
}

//注册event，开始收按键数据
//初始化相关部分
void divoom_app_onoff_register_event_init(void)
{
	if(pdivoom_app_power_handle == NULL)
		return;
	divoom_sys_model_crl_register(DIVOOM_MODEL_ONOFF, divoom_app_onoff_event);
}

uint8_t divoom_app_onoff_get_cur_power_on_mode(void)
{
	if(pdivoom_app_power_handle != NULL)
	{
#ifdef DIVOOM_WIFI_DEBUG_CONFIG
		return DIVOOM_APP_ONOFF_WIFI_MODE;
#endif
		return pdivoom_app_power_handle->now_power_mode;
	}
	return DIVOOM_APP_ONOFF_WIFI_CONFIG_MODE; //DIVOOM_APP_ONOFF_WIFI_MODE;
}

/*//对外接口，按下配网键调用这个函数
void divoom_app_onoff_config_wifi(void)
{
	//只有在WiFi模式下才能切换模式
	if(pdivoom_app_power_handle && pdivoom_app_power_handle->divoom_now_mode == 0)
	{
		divoom_wifi_model_config_wifi();
	}
	else
	{
		DIVOOM_ONOFF_PRINT("Now mode is bt, can't config wifi!\n");
	}
}*/

//设置下个开机模式,发送关机保存命令，重启
//适用于WiFi配网，模式切换等
void divoom_app_onoff_set_next_poweron_mode(uint8_t next_mode, uint8_t need_reboot)
{
	if(pdivoom_app_power_handle != NULL && next_mode <= DIVOOM_APP_ONOFF_WIFI_CONFIG_MODE)
	{
		uint8 cur_mode = pdivoom_app_power_handle->now_power_mode;
		pdivoom_app_power_handle->now_power_mode = next_mode;
		divoom_app_onoff_set_mode(next_mode);
		if(need_reboot)
		{
			//发送重启命令，目前是配网或者模式切换需要重启
			divoom_sys_msg_info  msg = {0};
			msg.msg_id = DIVOOM_MAIN_EVENT_SOFT_RESTART;
			msg.src_id = DIVOOM_MODEL_ONOFF;
			msg.dest_id = DIVOOM_MODEL_MAIN;
			msg.data = &cur_mode;
			divoom_sys_model_crl_msg_push(&msg);

			//设置是否是切换模式开机为1
			pdivoom_app_power_handle->is_channge_mode = 1;
			divoom_app_onoff_set_is_channge_mode(pdivoom_app_power_handle->is_channge_mode);
			//还是关断一下电源
			//切换模式不能关电源
			//divooom_app_onoff_set_power(0);
			esp_restart();
		}
	}
}

#ifndef DIVOOM_ONOFF_NO_POWER_CONTROL
//返回是不是按键开机
DIVOOM_POWER_ON_STATUS divoom_app_onoff_get_is_key_poweron(void)
{
	if(pdivoom_app_power_handle != NULL)
	{
		uint8_t ret = 0;
		divoom_system_get_lock(pdivoom_app_power_handle->divoom_onoff_lock);
		ret = pdivoom_app_power_handle->divoom_power_on_status;
		divoom_system_free_lock(pdivoom_app_power_handle->divoom_onoff_lock);
		return ret;
	}

	return DIVOOM_POWER_ON_BY_KEY;
}

bool divoom_app_onoff_get_poweron_alarm_flag()
{
	if(pdivoom_app_power_handle != NULL)
	{
		bool ret = 0;
		divoom_system_get_lock(pdivoom_app_power_handle->divoom_onoff_lock);
		ret = pdivoom_app_power_handle->alarm_is_open_flag;
		divoom_system_free_lock(pdivoom_app_power_handle->divoom_onoff_lock);
		return ret;
	}

	return 0;
}

void divoom_app_onoff_set_poweron_alarm_flag(bool flag)
{
	if(pdivoom_app_power_handle)
	{
		divoom_system_get_lock(pdivoom_app_power_handle->divoom_onoff_lock);
		pdivoom_app_power_handle->alarm_is_open_flag = (flag>=1)?1:0;
		divoom_system_free_lock(pdivoom_app_power_handle->divoom_onoff_lock);
	}
}
#endif



void divoom_app_onoff_init(void)
{
	uint8 sleep_rest_reason = 0;

	//下一个rtc_time:esp_clk_rtc_time(),距离下一个闹钟的时间减去time(0),加上当前的rtc时间,单位精确到秒
	//uint64_t next_timer = 0;

	//divoom_app_onoff_set_gpio_attr(DIVOOM_ONOFF_CHECK_KEY, DIVOOM_ONOFF_CHECK_MODE_KEY);
	//divoom_app_onoff_set_gpio_attr(DIVOOM_ONOFF_CHECK_KEY, 0xFF);

	pdivoom_app_power_handle = (Pdivoom_app_power_struct)divoom_malloc(sizeof(divoom_app_power_struct), DIVOOM_MODEL_ONOFF, __LINE__, __FILE__);
	DIVOOM_MEM_CHECK(DIVOOM_APP_ONOFF_TAG, pdivoom_app_power_handle, return);
	//DIVOOM_ONOFF_PRINT("FIRST MALLOC ADDR:0x%p", pdivoom_app_power_handle);
	memset(pdivoom_app_power_handle, 0, sizeof(divoom_app_power_struct));
	//开始给个初始值，下面读取，读不到再给默认值
	//pdivoom_app_power_handle->now_power_mode = DIVOOM_APP_ONOFF_WIFI_CONFIG_MODE;
	pdivoom_app_power_handle->now_power_mode = 0xFF;


	if(divoom_app_onoff_get_ischange_mode_power_mode(&pdivoom_app_power_handle->is_channge_mode, &pdivoom_app_power_handle->now_power_mode, 
				&pdivoom_app_power_handle->next_timer) != 1)
	{
		DIVOOM_ONOFF_PRINT("%s:%d:Can't read time from nvs.May be first power on.\n", __func__, __LINE__);
	}
	divoom_print("pdivoom_app_power_handle->is_channge_mode = %d, pdivoom_app_power_handle->now_power_mode = %d, pdivoom_app_power_handle->next_timer = %lld\n", 
			pdivoom_app_power_handle->is_channge_mode, pdivoom_app_power_handle->now_power_mode, pdivoom_app_power_handle->next_timer);

	pdivoom_app_power_handle->divoom_onoff_lock = divoom_system_create_lock();
	if(pdivoom_app_power_handle->divoom_onoff_lock == NULL)
	{
		divoom_free_line(pdivoom_app_power_handle, __FILE__, __LINE__);
	}

#ifndef DIVOOM_ONOFF_NO_POWER_CONTROL
	pdivoom_app_power_handle->divoom_power_on_status = DIVOOM_POWER_ON_OTHER;
#endif
	pdivoom_app_power_handle->divoom_onoff_key_status = 1;
	pdivoom_app_power_handle->divoom_onoff_ple_poweron = 0;

	//放在开机的地方处理
	//divooom_app_onoff_set_power(1);
	pdivoom_app_power_handle->divoom_task_run_flag = 1;



#ifdef DIVOOM_TASK_PRINTF_STACK_INFO
//	pdivoom_app_power_handle->divoom_onoff_handle = divoom_system_create_task_prio(divoom_app_onoff_task_check_key, "onoff_task_check", (void *)pdivoom_app_power_handle, DIVOOM_ONOFF_TASK_STACK);
#else
	divoom_system_create_task_prio(divoom_app_onoff_task_check_key, "onoff_task_check", (void *)pdivoom_app_power_handle, DIVOOM_ONOFF_TASK_STACK, 1);
	//xTaskCreate(divoom_app_onoff_task_check_key, "onoff_task_check", DIVOOM_ONOFF_TASK_STACK, (void *)pdivoom_app_power_handle, 1, NULL);
#endif

	divoom_system_get_lock(pdivoom_app_power_handle->divoom_onoff_lock);
	if(pdivoom_app_power_handle->is_channge_mode == 1)
	{
		//切换模式开机
#ifndef DIVOOM_ONOFF_NO_POWER_CONTROL
		pdivoom_app_power_handle->divoom_power_on_status = DIVOOM_POWER_ON_OTHER;
#endif
		if(pdivoom_app_power_handle->now_power_mode == DIVOOM_APP_ONOFF_WIFI_MODE)
		{
			esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);
			//20190708,没有连上WiFi,不释放BLE,直到连上WiFi在释放BLE的内存
			//esp_bt_controller_mem_release(ESP_BT_MODE_BLE);
			//pdivoom_app_power_handle->divoom_now_mode = 0;
		}
		else if(pdivoom_app_power_handle->now_power_mode == DIVOOM_APP_ONOFF_WIFI_CONFIG_MODE)
		{
			esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);
			//pdivoom_app_power_handle->divoom_now_mode = 0;
		}
#ifndef DIVOOM_ONOFF_NO_POWER_CONTROL
		pdivoom_app_power_handle->divoom_onoff_key_status = 0;
		pdivoom_app_power_handle->divoom_onoff_ple_poweron = 1;
#endif
		pdivoom_app_power_handle->is_channge_mode = 0;
		divoom_app_onoff_set_is_channge_mode(pdivoom_app_power_handle->is_channge_mode);
	}
#ifndef DIVOOM_ONOFF_NO_POWER_CONTROL
	else
	{
		//不是切换模式开机就默认给为2,适用于开发板会自动上电开机-->1845行
		pdivoom_app_power_handle->divoom_onoff_ple_poweron = 2;
		//如果不是切换模式开机,那么就只能是按键开机和闹钟开机
		sleep_rest_reason = esp_sleep_get_wakeup_cause();
		switch(sleep_rest_reason)
		{
			case ESP_SLEEP_WAKEUP_EXT1:
				{
					//不需要检测是否开机
					uint64_t wakeup_pin_mask = esp_sleep_get_ext1_wakeup_status();
					divoom_print("wakeup_pin_mask = %lld\n", wakeup_pin_mask);
					//最小是1<<0位
					if(wakeup_pin_mask != 0)
					{
						uint32 pin;
						pin = __builtin_ffsll(wakeup_pin_mask) - 1;
						DIVOOM_ONOFF_PRINT("divoom_wakeup by pin %d\n", pin);
						pdivoom_app_power_handle->divoom_power_on_status = DIVOOM_POWER_ON_BY_KEY;
					}
					else
					{
						DIVOOM_ONOFF_PRINT("Some wrong wakeup in %s:%d\n", __func__, __LINE__);
						goto DIVOOM_APP_ONOFF_GOTO_SLEEP;
					}
					//留给task进行开机判断
					break;
				}
			case ESP_SLEEP_WAKEUP_TIMER:
				{
					//如果是定时器开机，关机闹钟，直接开机
					DIVOOM_ONOFF_PRINT("Open because timer & poweroff alarm:expect time:%lld, reall time:%lld\n", pdivoom_app_power_handle->next_timer, esp_clk_rtc_time());
					pdivoom_app_power_handle->alarm_is_open_flag = 1;
					pdivoom_app_power_handle->divoom_onoff_ple_poweron = 1;
					pdivoom_app_power_handle->divoom_power_on_status = DIVOOM_POWER_ON_OTHER;
					break;
				}
			default:
				{
					//不是按键开机也不是闹钟和切换模式开机的话,就是出错了,直接到睡眠
					DIVOOM_ONOFF_PRINT("HERE POWER ON NO KYE UP, ALARM UP, and MODE CHANGE UP!\n");
					goto DIVOOM_APP_ONOFF_GOTO_SLEEP;
				}
		}
		pdivoom_app_power_handle->divoom_onoff_ple_poweron = 1;
	}
#endif
	divoom_system_free_lock(pdivoom_app_power_handle->divoom_onoff_lock);

	//清除存在nvs里面的配置
	if(pdivoom_app_power_handle->now_power_mode == 255)
	{
		//如果是第一次开机，默认设置为配网状态
#ifdef DIVOOM_WIFI_DEBUG_CONFIG
		pdivoom_app_power_handle->now_power_mode = DIVOOM_APP_ONOFF_WIFI_MODE;
#else
		pdivoom_app_power_handle->now_power_mode = DIVOOM_APP_ONOFF_WIFI_CONFIG_MODE;
#endif
		/***************test*********************
		pdivoom_app_power_handle->now_power_mode = DIVOOM_APP_ONOFF_WIFI_MODE;
		esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT);
		esp_bt_controller_mem_release(ESP_BT_MODE_BLE);
		***************test*********************/
	}
	//divoom_app_onoff_set_mode(255);

	return;
DIVOOM_APP_ONOFF_GOTO_SLEEP:
	DIVOOM_ONOFF_PRINT("%s:%d:Now to goto sleep!\n", __func__, __LINE__);
	divoom_app_onoff_set_rtcgpio_wakeup(DIVOOM_ONOFF_CHECK_KEY);
	divoom_app_onoff_set_timer_wakeup(pdivoom_app_power_handle->next_timer);
	divoom_app_onoff_start_sleep();
	return;
}




//等待检测够3秒
//默认返回1为正常开机，返回2为进入测试模式
//需要增加开机检测按键是否按下开机
int divoom_app_onoff_check_poweron_init(void)
{
//	divoom_print("./****************************************************/");
#if 1
	//等到按键按下三秒，或者不够三秒，进入睡眠
	if(pdivoom_app_power_handle == NULL)
	{
//		return 1;
	}

	uint8_t now_ple_poweron = 0;

	uint32 time = divoom_system_get_tickms();
	uint32 tmp_time = time;
	uint8 PowerKeyStatus = 0;

	while(1)
	{
		tmp_time = divoom_system_get_tickms();
		PowerKeyStatus = gpio_get_level(DIVOOM_ONOFF_CHECK_KEY);

		if(PowerKeyStatus == 0)
		{
			if(tmp_time >= 500)
			{
//				divoom_print("open/****************************************************/");
				return 1;
//				pdivoom_app_power_handle->divoom_onoff_ple_poweron = 1;
			}
		}else
		{
			goto DIVOOM_APP_ONOFF_GOTO_SLEEP;
		}	

		//divoom_system_get_lock(pdivoom_app_power_handle->divoom_onoff_lock);
//		now_ple_poweron = pdivoom_app_power_handle->divoom_onoff_ple_poweron;
		//divoom_system_free_lock(pdivoom_app_power_handle->divoom_onoff_lock);
		if(now_ple_poweron == 1)
		{
			//可以开机，退出循环
			break;
		}
		else if(now_ple_poweron == 2)
		//else
		{
			//按的时间不够，去sleep
			goto DIVOOM_APP_ONOFF_GOTO_SLEEP;
		}
#ifdef DIVOOM_ONOFF_TEST_MODE
		//这里是进入测试模式
		else if(now_ple_poweron == 3)
		{
			return 2;
		}
#endif
		vTaskDelay(DIVOOM_ONOFF_DELAY_TIME / 2);
	}
	return 1;
DIVOOM_APP_ONOFF_GOTO_SLEEP:

#endif
	DIVOOM_ONOFF_PRINT("%s:%d:Now to goto sleep!\n", __func__, __LINE__);
	divoom_app_onoff_set_rtcgpio_wakeup(DIVOOM_ONOFF_CHECK_KEY);
//	divoom_app_onoff_set_timer_wakeup(pdivoom_app_power_handle->next_timer);
	divoom_app_onoff_start_sleep();
	return 1;
}



void divoom_app_onoff_set_sleep(void)
{
	divoom_app_onoff_set_rtcgpio_wakeup(DIVOOM_ONOFF_CHECK_KEY);
//	divoom_app_onoff_set_timer_wakeup(pdivoom_app_power_handle->next_timer);
	divoom_app_onoff_start_sleep();
}


#endif
