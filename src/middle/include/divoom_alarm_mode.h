#ifndef _DIVOOM_ALARM_MODE_H_
#define _DIVOOM_ALARM_MODE_H_
#include "divoom_type_tivoo.h"

#define SYSTEM_ALARM_NUM    11
#define APP_ALARM_NUM    10
#define SYSTEM_DIALY_NUM    10

enum
{
    DIVOOM_ALARM_TYPE_ALARM,
    DIVOOM_ALARM_TYPE_DIALY,
    DIVOOM_ALARM_TYPE_AI_ALARM,
};

typedef enum
{
	DIVOOM_ALARM_GIF_READY_LOAD,    //需要加载GIF文件
	DIVOOM_ALARM_GIF_OK_LOAD,       //已经加载GIF文件
	DIVOOM_ALARM_GIF_NO_LOAD,       //没有要加载的GIF文件
}DIVOOM_GIF_LOAD_STATUS;

/*
	DIVOOM_ALARM_IOCTL_GET_NEXT_ALARM,       //数据结构体T_DIVOOM_ALARM_ACTIVE_INFO
	DIVOOM_ALARM_IOCTL_READ_CONFIG,          //读数据T_DIVOOM_READ_ALARM_CFG,T_DIVOOM_READ_DIALY_CFG,type&index must have value before read
	DIVOOM_ALARM_IOCTL_UPDATE_CONFIG,        //写数据T_DIVOOM_READ_UPDATE_CFG, T_DIVOOM_UPDATE_DIALY_CFG,type&index must have value before write
	DIVOOM_ALARM_IOCTL_UNACTIVE,             //关闭闹钟，单次闹钟,第几个闹钟,T_U8
	DIVOOM_ALARM_IOCTL_UNACTIVE_SAME,        //关闭相同的单次的闹钟，不能关闭纪念日T_DIVOOM_ALARM_ACTIVE_INFO
   */
typedef enum{
	DIVOOM_ALARM_IOCTL_READ_CONFIG,						//读数据T_DIVOOM_READ_ALARM_CFG,T_DIVOOM_READ_DIALY_CFG,type&index must have value before read
	DIVOOM_ALARM_IOCTL_UPDATE_CONFIG,					//写数据T_DIVOOM_READ_UPDATE_CFG, T_DIVOOM_UPDATE_DIALY_CFG,type&index must have value before write


	DIVOOM_ALARM_IOCTL_LINUX_UPDATE_ALL_CONFIG,			//写全部数据，在linux上可能会写全部的闹钟信息
	DIVOOM_ALARM_IOCTL_LINUX_DEL_ONE_CONFIG,			//删除某一个数据，在linux上可能会需要
	DIVOOM_ALARM_IOCTL_LINUX_DEL_ALL_CONFIG,			//删除全部数据，在linux上可能会需要
}T_DIVOOM_ALARM_IOCTL_COM;

typedef enum{
	DIVOOM_ALARM_GET_POWERON_STATUS,
	DIVOOM_ALARM_SET_POWERON_STATUS,
}DIVOOM_ALARM_STATUS;

typedef struct _divoom_alarm_time_
{
    T_U16   year;           /* 2 byte: 1-9999 */
    T_U8    month;          /* 1-12 */
    T_U8    day;            /* 1-31 */
    T_U8    hour;           /* 0-23 */
    T_U8    minute;         /* 0-59 */
    T_U8    second;         /* 0-59 */
    T_U8    week;           /* 0-6, 0: sunday, 1: monday */
}divoom_alarm_system_date, *Pdivoom_alarm_system_date;

typedef struct{
	char	alarm_name[64];	//闹钟名称
	T_U8    active_status;    // 是否有效,主要应用在队列中删除
	T_U8    on_off;          // 1:on; 0:off
	T_U8    type;			//闹钟类型
	T_U8    index;
    T_U8    cyc_flag;           /* bit1:monday ~ bit7:sunday, 每周的循环时间*/
    int    month;          /* 1-12 */
    int    day;            /* 1-31 */
    int    hour;           /* 0-23 */
    int    minute;         /* 0-59 */
    int    year;           /* 2 byte: 1-9999 */
    int     early_flag;
    int     alarm_id;
	char	image_data[52];	//自定义图片地址
	int     update_time;

	T_U8    mode;           /* SoundType: 0经典 1录音 3蓝牙 4TF */
	T_U8 	volume;
	char	leave_url[52];		//自定义留言，有可能是url,有可能是spiffs路径,看后面怎么定义这个吧
	double	fm;				//电台id
	T_U8 	tip_index;		//提示音id
	T_U8	scene_index;	//场景id

	T_U32	alarm_set_time;

}T_DIVOOM_ALARM_ACTIVE_DO_INFO;

typedef struct{
	T_U8    on_off;          // 1:on; 0:off
    T_U8    hour;           /* 0-23 */
    T_U8    minute;         /* 0-59 */
    T_U8    week;           /* bit1:monday ~ bit7:sunday*/
    T_U8    music_mode;           //ALARM_MODE
	T_U8 	trigger_mode;  //ALARM_TRIGGER_MODE
	//T_U8    app_mode;
	//T_U8 	app_trigger_mode;  //
	//T_U8    fm_freq[2];
	T_U8 	volume;
	//T_BOOL  flag;         //标志位，设置为1
    //T_U8    check_code;
	char	*leave_url;		//自定义留言，有可能是url,有可能是spiffs路径,看后面怎么定义这个吧
	char	*image_url;		//自定义图片地址，有可能是url,有可能是spiffs路径,看后面怎么定义这个吧
	T_U32	alarm_set_time;
}T_DIVOOM_ALARM_CFG;

typedef struct{
	T_U8    on_off;          // 1:on; 0:off
	T_U8	month;			/* 1-12 */
	T_U8	day;			/* 1-31 */
    T_U8    hour;           /* 0-23 */
    T_U8    minute;         /* 0-59 */
	//T_U8    title_name[64]; //考虑去掉,因为设备不会修改纪念日，所以不需要存名字了
    T_U8    trigger_mode;
    //T_U8    check_code;
	//char	*image_name;		//自定义图片名字,只需要数据就行了
	T_U32	image_len;			//自定义图片数据长度
	char	*image_url;		//自定义图片地址，有可能是url,有可能是spiffs路径,看后面怎么定义这个吧
	T_U32	alarm_set_time;
}T_DIVOOM_DIALY_CFG;

typedef struct{
	T_U8               type;
	T_U8               index;
	T_DIVOOM_ALARM_CFG alarm_info;
}T_DIVOOM_READ_ALARM_CFG;

typedef struct{
	T_U8               type;
	T_U8               index;
	T_DIVOOM_DIALY_CFG dialy_info;
}T_DIVOOM_READ_DIALY_CFG;

typedef struct{
	T_U8               type;
	T_U8               index;
	T_DIVOOM_DIALY_CFG dialy_info;
}T_DIVOOM_UPDATE_DIALY_CFG;

typedef struct{
	T_U8               type;
	T_U8               index;
	T_DIVOOM_ALARM_CFG alarm_info;
}T_DIVOOM_UPDATE_ALARM_CFG;

typedef struct{
	T_U8               type;
	T_U8               index;
}T_DIVOOM_DEL_ONE_ALARM_DIALY_CFG;

typedef struct{
	T_U8							num;
	T_DIVOOM_ALARM_ACTIVE_DO_INFO	*info;//T_DIVOOM_ALARM_ACTIVE_DO_INFO + T_DIVOOM_ALARM_ACTIVE_DO_INFO + ...
}T_DIVOOM_UPDATE_LINUX_ALL_CFG;

/*typedef struct{
	T_U8						dialy_num;
	T_DIVOOM_UPDATE_DIALY_CFG	dialy_info[SYSTEM_DIALY_NUM];
}T_DIVOOM_UPDATE_LINUX_ALL_DIALY_CFG;*/

typedef struct{
	T_U8               type;
	T_U8               index;
	T_DIVOOM_ALARM_CFG alarm_info;
}T_DIVOOM_UPDATE_ALL_ALARM_CFG;

typedef T_DIVOOM_ALARM_CFG  divoom_alarm_info;
typedef T_DIVOOM_ALARM_CFG  *Pdivoom_alarm_info;
typedef T_DIVOOM_DIALY_CFG  divoom_dialy_info;
typedef T_DIVOOM_DIALY_CFG  *Pdivoom_dialy_info;

//typedef T_VOID (*DIVOOM_ALARM_CB_VOICE_OVER)(T_VOID);
typedef T_VOID (*DIVOOM_ALARM_ACTIVE_CALLBACK)(T_VOID);

/********************************************************需要外部注册提供的函数*****************************************************************/
//print
typedef T_VOID  (*DIVOOM_ALARM_CB_PRINT)(T_CHR *format, ...);
//读取配置
typedef T_U32   (*DIVOOM_ALARM_CB_CONFIG_READ)(T_U8 mode, T_VOID *buf, T_U8 index, int size);
//写进配置
typedef T_U32   (*DIVOOM_ALARM_CB_CONFIG_WRITE)(T_U8 mode, T_VOID *buf, T_U8 index, int size);
//删除某个配置
typedef T_VOID  (*DIVOOM_ALARM_CB_CONFIG_DEL_ONE)(T_U8 mode, T_U8 index);
//删除全部配置
typedef T_VOID	(*DIVOOM_ALARM_CB_CONFIG_DEL_ALL)(T_U8 mode);
//free func
typedef T_VOID  (*DIVOOM_ALARM_CB_FREE)(T_pVOID var);
//malloc func
typedef T_pVOID (*DIVOOM_ALARM_CB_MALLOC)(T_U32 size, T_CHR *file_name, int line);

//pthread_mutex_init
typedef T_VOID  (*DIVOOM_ALARM_CB_LOCK_INIT)(T_VOID);
//pthread_mutex_deinit
typedef T_VOID  (*DIVOOM_ALARM_CB_LOCK_DEINIT)(T_VOID);
//pthread_mutex_lock
typedef T_VOID  (*DIVOOM_ALARM_CB_LOCK_LOCK)(T_VOID);
//pthread_mutex_unlock
typedef T_VOID  (*DIVOOM_ALARM_CB_LOCK_UNLOCK)(T_VOID);

//当天0点到现在的时间
typedef T_U32   (*DIVOOM_ALARM_CB_GET_TIME_SEC)(T_VOID);
//得到日期参数，为了最终的对比
typedef T_VOID  (*DIVOOM_ALARM_CB_GET_SYSTEM_DATA)(Pdivoom_alarm_system_date date);
//获取秒数，传入时间结构体，不同平台各自处理
typedef T_U32   (*DIVOOM_ALARM_CB_TIME_TO_SECOND)(Pdivoom_alarm_system_date date);
//秒数转化成年月日类型
typedef T_VOID  (*DIVOOM_ALARM_CB_SECOND_TO_TIME)(Pdivoom_alarm_system_date date, T_U32 second);

//设置下一个闹钟的时间，参数Pdivoom_alarm_system_date
typedef T_VOID  (*DIVOOM_ALARM_CB_SET_RTC_TIME)(T_U8 flag, T_DIVOOM_ALARM_ACTIVE_DO_INFO *date, DIVOOM_ALARM_ACTIVE_CALLBACK active_on);
//闹钟时间到了之后，外部需要做的事情
typedef T_VOID  (*DIVOOM_ALARM_CB_ALARM_ACTIVE_DO)(T_DIVOOM_ALARM_ACTIVE_DO_INFO *active_do_info);

//更改闹钟类型,通过参数看是设置还是获取,跟系统参数有关,参数DIVOOM_ALARM_GET_STATUS，返回1,是开机闹钟，0不是，DIVOOM_ALARM_SET_STATUS,设置为非开机闹钟，改为普通闹钟，可以不关心返回值
typedef T_U8    (*DIVOOM_ALARM_CB_ALARM_STATUS)(DIVOOM_ALARM_STATUS flag, T_U8 status);

//一些需要运行的函数，在闹钟模块，耗时比较长
typedef T_VOID  (*DIVOOM_ALARM_CB_NEED_DO)(T_VOID);

//增加一个发送单次闹钟关闭的函数,包括单次闹钟的数据和多个重复闹钟时间到了全部关闭
//接收闹钟id,整合数据,进行上传服务器
typedef T_VOID (*DIVOOM_ALARM_UNACTIVE_ALARM)(T_U8 *alarm_id_list, T_U8 alarm_id_list_len);
/********************************************************需要外部注册提供的函数*****************************************************************/


typedef struct _divoom_alarm_callback_
{
	DIVOOM_ALARM_CB_PRINT                 print;
	DIVOOM_ALARM_CB_CONFIG_READ           read;
	DIVOOM_ALARM_CB_CONFIG_WRITE          write;
	DIVOOM_ALARM_CB_CONFIG_DEL_ONE		  del_one;
	DIVOOM_ALARM_CB_CONFIG_DEL_ALL		  del_all;
	DIVOOM_ALARM_CB_FREE                  free;
	DIVOOM_ALARM_CB_MALLOC                malloc;

	DIVOOM_ALARM_CB_LOCK_INIT             lock_init;
	DIVOOM_ALARM_CB_LOCK_DEINIT           lock_deinit;
	DIVOOM_ALARM_CB_LOCK_LOCK             lock_lock;
	DIVOOM_ALARM_CB_LOCK_UNLOCK           lock_unlock;

	DIVOOM_ALARM_CB_GET_TIME_SEC          get_system_sec;
	DIVOOM_ALARM_CB_GET_SYSTEM_DATA       get_system_date;
	DIVOOM_ALARM_CB_TIME_TO_SECOND        time_to_sec;
	DIVOOM_ALARM_CB_SECOND_TO_TIME        sec_to_time;

	DIVOOM_ALARM_CB_SET_RTC_TIME          set_rtc_time;
	DIVOOM_ALARM_CB_ALARM_ACTIVE_DO       alarm_do;

	DIVOOM_ALARM_CB_ALARM_STATUS          power_on_status;

	DIVOOM_ALARM_CB_NEED_DO               need_do;
	DIVOOM_ALARM_UNACTIVE_ALARM			  unactive_once_same_alarm;
}divoom_alarm_callback, *Pdivoom_alarm_callback;

T_VOID divoom_alarm_lib_init(Pdivoom_alarm_callback callback);
T_VOID divoom_alarm_lib_deinit(T_VOID);
T_VOID divoom_alarm_lib_ioctl(T_DIVOOM_ALARM_IOCTL_COM cmd, T_VOID *data);
#endif



