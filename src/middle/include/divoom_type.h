#ifndef _DIVOOM_TYPE_H_
#define _DIVOOM_TYPE_H_

typedef unsigned char 	T_U8;

typedef unsigned char 	uint8;
typedef unsigned char 	uint8_t;
typedef signed char 	int8;

typedef unsigned short 	uint16;
typedef unsigned short 	uint16_t;
typedef signed short	int16;

typedef unsigned int    uint32;
typedef unsigned int    uint32_t;
typedef signed int 		int32;

typedef unsigned long   DIVOOM_UINT_ID;
#ifndef bool
typedef uint8 bool;
#endif


#define     AK_FALSE            0
#define     AK_TRUE             1
#define     AK_NULL             NULL


#ifndef false
#define false 0
#endif
#ifndef true
#define true 1
#endif
#define U8_MAX  0XFF
#define U16_MAX  0XFFFF
#define U32_MAX  0XFFFFFFFF

#if 0
#define    BIT0	 (1 << 00)
#define    BIT1	 (1 << 01)
#define    BIT2	 (1 << 02)
#define    BIT3	 (1 << 03)
#define    BIT4	 (1 << 04)
#define    BIT5	 (1 << 05)
#define    BIT6	 (1 << 06)
#define    BIT7	 (1 << 07)
#define    BIT8	 (1 << 08)
#define    BIT9	 (1 << 09)
#define    BIT10	 (1 << 10)
#define    BIT11	 (1 << 11)
#define    BIT12	 (1 << 12)
#define    BIT13	 (1 << 13)
#define    BIT14	 (1 << 14)
#define    BIT15	 (1 << 15)
#endif



typedef unsigned char           T_U8;       /* unsigned 8 bit integer */
typedef char                    T_CHR;      /* char */
typedef unsigned short          T_U16;      /* unsigned 16 bit integer */
typedef unsigned int              T_U32;      /* unsigned 32 bit integer */
typedef unsigned long long      T_U64;      /* unsigned 64 bit integer */
typedef signed char             T_S8;       /* signed 8 bit integer */
typedef signed short            T_S16;      /* signed 16 bit integer */
typedef int                     T_S32;      /* signed 32 bit integer */
typedef signed long long        T_S64;      /* signed 64 bit integer */
typedef void                    T_VOID;     /* void */
typedef float                   T_FLOAT;
typedef volatile unsigned int   V_UINT32;
#define    T_U8_MAX             ((T_U8)0xff)                    // maximum T_U8 value
#define    T_U16_MAX            ((T_U16)0xffff)                    // maximum T_U16 value
#define    T_U32_MAX            ((T_U32)0xffffffff)                // maximum T_U32 value
#define    T_U64_MAX            ((T_U32)0xffffffffffffffff)                // maximum T_U64 value
#define    T_S8_MIN             ((T_S8)(-127-1))                // minimum T_S8 value
#define    T_S8_MAX             ((T_S8)127)                        // maximum T_S8 value
#define    T_S16_MIN            ((T_S16)(-32767L-1L))        // minimum T_S16 value
#define    T_S16_MAX            ((T_S16)(32767L))            // maximum T_S16 value
#define    T_S32_MIN            ((T_S32)(-2147483647L-1L))    // minimum T_S32 value
#define    T_S32_MAX            ((T_S32)(2147483647L))        // maximum T_S32 value
#define    T_S64_MIN            ((T_S32)(-9223372036854775807L-1L))    // minimum T_S64 value
#define    T_S64_MAX            ((T_S32)(9223372036854775807L))        // maximum T_S64 value

/* basal type definition for global area */
typedef T_U8                    T_BOOL;     /* BOOL type */

typedef T_VOID *                T_pVOID;    /* pointer of void data */
typedef const T_VOID *          T_pCVOID;   /* const pointer of void data */

typedef T_CHR *                 T_pSTR;     /* pointer of string */
typedef const T_CHR *           T_pCSTR;    /* const pointer of string */


typedef T_U16                   T_WCHR;     /**< unicode char */
typedef T_U16 *                 T_pWSTR;    /* pointer of unicode string */
typedef const T_U16 *           T_pCWSTR;   /* const pointer of unicode string */


typedef T_U8 *                  T_pDATA;    /* pointer of data */
typedef const T_U8 *            T_pCDATA;   /* const pointer of data */

typedef T_U32                   T_COLOR;        /* color value */

typedef T_U32                   T_HANDLE;           /* a handle */



#define DEIVOOM_MAX_VOL 15
#define ENUM_TYPE_LEN   2
//错误代码
enum
{
	DIVOOM_SUCCESS,
	DIVOOM_NO_RESOURCE,
	DIVOOM_MODEL_NO_INIT,
	DIVOOM_MODEL_ERROR,
	DIVOOM_MODEL_PARM_ERROR,
	DIVOOM_MALLOC_FAIL,
	DIVOOM_SET_DOMAIN_FAIL,
};

#ifndef NULL
#define NULL (void *)0
#endif

//返回值定义
typedef enum 
{
    SUCCEED = 0x00,                     //成功
    DIVOOM_COMM_FAILURE = 0x01,         //失败
}DIVOOM_COMM_RESULT;

typedef struct _divoom_file_name_info
{
	uint8 len;          //文件名长度
	uint8 *file_name;   //文件名缓冲区
}divoom_file_name_info, *Pdivoom_file_name_info;
struct _divoom_net_command;
typedef struct _divoom_net_command  *Pdivoom_net_command;
typedef void DIVOOM_NET_USER_CALLBACK(Pdivoom_net_command pcmd_info);
typedef void DIVOOM_NET_STATUS_CALLBACK(int status);


typedef void SENSOR_EVENT_CALLBACK(uint8 sensor_type, uint8 sensor_id, void *data);

typedef void divoom_net_command_callback(void *data);
typedef void divoom_net_command_free(Pdivoom_net_command  pcmd_info);

typedef struct _divoom_clock_id_info
{
    uint8 clock_pos;        //在父表盘位置      如果没有父表盘就为0，
    int clock_id;    //表盘ID
    int parent_id;     //父表盘ID    如果没有父表盘就为0
    uint8 data_clock_pos;        //AI数据表盘信息
    int data_clock_id;    //AI数据表盘信息
    int data_parent_id;     //AI数据表盘信息
    uint8 ha_slot;            //HA 子表盘槽位，1 到 4
}divoom_clock_id_info, *Pdivoom_clock_id_info;

typedef struct _divoom_social_platform_clock_id_info
{
    int clock_id;    //表盘ID
    int clock_pos;        //在父表盘位置      如果没有父表盘就为0，
    int parent_id;     //父表盘ID    如果没有父表盘就为0
	int type;		// 0tik-tok
}divoom_social_platform_clock_id_info, *Pdivoom_social_platform_clock_id_info;

typedef struct _divoom_net_command
{
	uint8 type;
	char topic[64];
	int device_id;
	//char ip_host[16];
	uint32 userid;
	uint32 accesstoken;
    int  UTCTime;
	void *client_handle;
	uint16 cmd;
	uint32 cmd_flag;        //命令标识，将为一个随机数
	void   *cmd_data;
	void *ret_data;		//回应的数据
	uint8 wait_ret;		//等待标志位
    void *wait_sem;     //等命令信号通知
    uint32 cmd_time_out;  //命令超时时间
	divoom_net_command_callback *pcallback;     //当收到回应时调用通知应用
	divoom_net_command_free *pfreefunc;         //当命令处理完成后，调用的释放内存
}divoom_net_command;

/* 图片列表统一只保存四项核心元数据，作者名称由持有者动态申请并负责释放。 */
typedef struct _divoom_image_hot_item
{
    uint32 image_size;
    uint32 image_id;
    char   *image_name;
    uint32 user_id;
}divoom_image_hot_item, *Pdivoom_image_hot_item;

/* 旧版本地文件扫描和带语音图片仍使用该定长兼容结构。 */
typedef struct _divoom_image_base_info
{
    uint32 image_id;
    uint32 user_id;
    char   voice_id[64];
    char   image_name[200];
}divoom_image_base_info, *Pdivoom_image_base_info;


#endif
