#ifndef _divoom_h_
#define _divoom_h_
#include "divoom_build_config.h"
#include "divoom_paths.h"
#define DIVOOM_CUSTOM_ID    0
/* ato 3499：默认按住说话、松开发送；开源强制关闭，其他表盘不接管。 */
#ifdef DIVOOM_OPENSOURCE_SWITCH
#undef DIVOOM_AI_VOICE_DIAL_TEST_ENABLE
#define DIVOOM_AI_VOICE_DIAL_TEST_ENABLE 0
#elif !defined(DIVOOM_AI_VOICE_DIAL_TEST_ENABLE)
#define DIVOOM_AI_VOICE_DIAL_TEST_ENABLE 1
#endif
//#define DIVOOM_USE_RK_IC
//#define DIVOOM_USE_PIC_THUMB 
//#define DIVOOM_APP_USE_MQTT_CONNECT
//#define DIVOOM_TEST_TUTV_FLAG
//#define DIVOOM_WIFI_RF_TEST
//#define DIVOOM_GET_DEFAULT_ALL_CLOCK_INFO

/* AstroToo 480×480 真机工程（FlowToo Neutral Retro UI） */
#ifndef DIVOOM_PRODUCT_ASTRO_TOO
#define DIVOOM_PRODUCT_ASTRO_TOO
#endif

#define DIVOOM_ASTRO_TOO
#define DIVOOM_TIMES_VIBE
#define DIVOOM_TIME_FRAME_SUPPORT_LOW_LIGHT 8
#ifdef DIVOOM_PRODUCT_ASTRO_TOO
/* LCD 背光最低开档，对齐 FlowToo T113；勿改上面的 8（版本标记） */
#define DIVOOM_SYS_BRIGHT_ON_FLOOR 1
#else
#define DIVOOM_SYS_BRIGHT_ON_FLOOR DIVOOM_TIME_FRAME_SUPPORT_LOW_LIGHT
#endif


#if DIVOOM_SHOW_MODE
/*
 * 展机用独立硬件号，避免和量产 OTA 混升。地区宏见 divoom_build_config.h。
 * 国外 533001（TYPE 533 / BASE 1），国内 534001（TYPE 534 / BASE 1）。
 * 历史：曾共用 530 + 子版本 950/900 → 530950 / 530900。
 */
#if DIVOOM_SHOW_OVERSEAS    //国外版本
/* 展示机软件主版本：国外版本 */
#define DIVOOM_PRODUCT_TYPE         533
#define DIVOOM_HARDWARE_VERSION     DIVOOM_PRODUCT_TYPE
#define DIVOOM_SOFTWARE_VERSION_BASE        2
#else
/* 展示机软件主版本：国内版本 */
#define DIVOOM_PRODUCT_TYPE         534
#define DIVOOM_HARDWARE_VERSION     DIVOOM_PRODUCT_TYPE
#define DIVOOM_SOFTWARE_VERSION_BASE        2
#endif
#else
/* 量产机软件主版本*/
#define DIVOOM_PRODUCT_TYPE         530
#define DIVOOM_HARDWARE_VERSION     DIVOOM_PRODUCT_TYPE
#define DIVOOM_SOFTWARE_VERSION_BASE        77
#endif

/* 完整版本 = HARDWARE_VERSION * 1000 + BASE */
#define DIVOOM_SOFTWARE_VERSION (DIVOOM_HARDWARE_VERSION * 1000 + DIVOOM_SOFTWARE_VERSION_BASE)

/* 仅作文档/兜底参考；JL 包分流以主控 Hardware 排除 + 杰理上报版本前缀核对为准 */
#define DIVOOM_JL_HARDWARE_VERSION 535

#define DIVOOM_LIGHT_APP_HOT_CLOCK_ID             250

#define DIVOOM_TPLAYER_SET_DEFAULT_VOL      10
#define DIVOOM_ROOM_DEL_CLOCK_DATA      (25*1024)       //少于这个空间，将清除表盘数据
#define DIVOOM_ROOM_NOT_DOWNLOAD_MIN    (10*1024)        //少于这个空间，将不给下载新数据
#define DIVOOM_ROOM_WARNING_MIN    (15*1024)        //少于这个空间，将通知App不能下载
/* 提示音地板：get_tone_volume() 在系统音量 1–24 时抬到此值，0 仍静音 */
#define DIVOOM_TONE_VOL_FLOOR 25
/* 关机/连接短音相对提示音档位的比例。63：sink 平方后满档幅度仍约 40，与线性时代一致 */
// #define DIVOOM_POWEROFF_TONE_VOL_PERCENT  63
// #define DIVOOM_CONNECT_TONE_VOL_PERCENT   63
#define DIVOOM_WIFI_NEW_MANAGE
#define DIVOOM_TIME_FRAME_SPEAKER
#define DIVOOM_TIME_FRAME_NO_RGB_LIGHT          /* 无 RGB，与 SPEAKER 组合为 500 系 */
#define DIVOOM_PRODUCT_TIME_FRAME_NO_RGB_SPEAKER
#define DIVOOM_PRODUCT_TIMES_VIBE
/* ============================================================
 * 照片功能总开关 DIVOOM_PHOTO_MANAGE_ENABLE（真机 + 仿真通用）
 *   1 = 启用照片管理（相册 UI / 云·本地相册 / 相册 MQTT / custom·cloud photo）
 *   0 = 关闭照片功能
 * 优先级：编译选项 -D 定义 > 仿真 SIM_HAS_PHOTO_MANAGE > 下方默认值
 * 关闭方法：
 *   - 真机：编译选项加 -DDIVOOM_PHOTO_MANAGE_ENABLE=0（或直接改下方默认值）
 *   - 仿真：product.props / divoom_sim_config.h 设 SIM_HAS_PHOTO_MANAGE=0
 * 注：DIVOOM_SIM_HAS_PHOTO_MANAGE 为兼容旧名的别名，等价于本宏。
 * ========================================================== */

// #define DIVOOM_ASTRO_MANUAL_DATETIME_SET_ENABLE 1   // 需要手动设置时间日期的时候打开该宏

#ifdef DIVOOM_TIME_FRAME_NO_RGB_LIGHT
#define DIVOOM_TIME_FRAME_SUPPORT_LOW_LIGHT 8    //这个是支持低亮度的版本，没有ＲＧＢ的灯的一定要打开, 这个位置不能修改, 
#else
#define DIVOOM_TIME_FRAME_SUPPORT_LOW_LIGHT 8    //这个是支持低亮度的版本，有ＲＧＢ灯的，要区分不同版本，打开这个为511版本，否则是510
#endif

//#define DIVOOM_MQTT_DEBUG_LOG
#include "divoom_type.h"
#include "divoom_board_io_config.h"
#include "divoom_port_fs.h"
#include "divoom_port_os.h"
#include "divoom_port_hw.h"
#include "time.h"
#if DIVOOM_BUILD_RTOS_DEVICE
#include "divoom_port_time.h"
#endif
#include "stdio.h"
#include "stdlib.h"
#if DIVOOM_BUILD_RTOS_DEVICE
static inline int divoom_rtos_malloc_trim(size_t pad)
{
	(void)pad;
	return 0;
}
#define malloc_trim(pad)  divoom_rtos_malloc_trim(pad)
#endif
#include "memory.h"
#include "math.h"
#ifdef DIVOOM_SIMULATOR
#include "divoom_sim_config.h"
/* 模拟器禁用 divoom 小内存池，统一走 CRT malloc/free，与 VS Debug 堆一致；真机仍走池化分配 */
#ifndef SIM_DISABLE_RAM_POOL
#define SIM_DISABLE_RAM_POOL  1
#endif
#ifndef SIM_MALLOC_TRACE
#define SIM_MALLOC_TRACE  0
#endif
#endif
#include <unistd.h>
#ifdef DIVOOM_SIMULATOR
#define RK_FORMAT_BGRA_8888  0
#define RK_FORMAT_RGB_888    1
#define RK_FORMAT_RGBA_8888  2
#define RK_FORMAT_RGB_565    4
#include <wifimg.h>
#ifdef near
#undef near
#endif
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#include <stdbool.h>
#elif defined(DIVOOM_USE_RK_IC)
#include <Rk_wifi.h>
#else
#define RK_FORMAT_BGRA_8888  0
#define RK_FORMAT_RGB_888    1
#define RK_FORMAT_RGBA_8888  2
#define RK_FORMAT_RGB_565    4
#ifdef DIVOOM_NO_LEGACY_WIFIMG
#include "aic_wifi.h"
#else
#include <wifimg.h>
#endif
#endif
#include <dirent.h>
#include <ctype.h>
#include <pthread.h>
#define DIVOOM_EXIT_MENU_TIME_OUT      8//菜单不操作，退出菜单时间

#define DIVOOM_SYSTEM_FILE_VERSION 200      //系统文件版本号，只在完全烧录时修改， 后台系统软件文件中获取

#define DIVOOM_CUSTOME_CLOCK_ID 0
//#define DIVOOM_GET_DEFAULT_CLOCK_CONFIG     //这个打开后，会下载所有经过审核的表盘数据，用来做系统默认数据


#define DIVOOM_CHANNEL_APP_TYPE_USER_TEMP_CLOCK 1
#define DIVOOM_MAX_VOL_VALUE    16
#define DIVOOM_DEVICE_SUSCRIBE_EXPERT_NUM     20
#define DIVOOM_DEVICE_CLOCK_CUSTOME_MAX_IMAGE 256
#define DIVOOM_MAIN_WEATHER_UPDATE_TIME     (1800)
#define DIVOOM_WEATHER_ROUND_TEMP(x) ((int)((x) + ((x) >= 0 ? 0.5 : -0.5)))
#define DIVOOM_LCD_NUM   1
#define DIVOOM_INDEPEDN_DIAL_NUM   1
#define DIVOOM_TURN_DIAL_NUM   10
#define DIVOOM_IMAGE_ID_STEP   (DIVOOM_LCD_NUM * DIVOOM_INDEPEDN_DIAL_NUM)
#define DIVOOM_LCD_NOTICE_ID   2
#define DIVOOM_LCD_NOACTICE_ID   0Xff
#define DIVOOM_USER_DEFINE_GIF_FILE_NUM   10
#define DIVOOM_CLOCK_APP_DEFAULT_ID   394
#define DIVOOM_CLOCK_SUB_DIAL_ID   203
//#define DIVOOM_TEST_CUSTOM_DEVICE
#define DIVOOM_CUSTOM_DEVICE_MAC_ADDR "\x00\x00\x00\xff\xff\xff"
#define DIVOOM_CUSTOM_DEVICE_ID 300001833
#define DIVOOM_PHOTO_MAXALBUM  62
#define DIVOOM_WARNING_ROOM_SIZE    512

#define DIVOOM_DIAL_FONT_MIN_ID 6       //小于这个ID的就给DIVOOM_DIAL_FONT_DEFAULT_ID，这个是因为早期的系统字体被删除了
#define DIVOOM_DIAL_FONT_DEFAULT_ID 28
#define DIVOOM_AI_FONT_DEFAULT_ID 212  //AI大小框主字体；缺字使用表盘默认字体
#define DIVOOM_CLOUD_CLOCK_ID 254 //云图表盘
#define DIVOOM_MUSIC_CLOCK_ID 2212 //音乐表盘
#define DIVOOM_RSS_CLOCK_ID 2329 //RSS
#define DIVOOM_PORTRY_CLOCK_ID 2303 //诗词
#define DIVOOM_ANIMATION_CLOCK_ID 19 //2
#define DIVOOM_USE_IMAGE_LIST_CLOCK_ID 23 //2
#define DIVOOM_TMP_EQ_CLOCK_ID 20//3
#define DIVOOM_TIME_PLAN_CLOCK_ID 2926//398 //2
#define DIVOOM_DIALY_CLOCK_ID 397 //2
#define DIVOOM_VISION_CLOCK_ID 845 //2
#define DIVOOM_SYSTEM_DEFAULT_PLAY_CLOCK_ID 51 //2
#define DIVOOM_DEFAULT_CLOCK_ID 1856
#define DIVOOM_START_CLOCK_ID 1856
#define DIVOOM_CELL_STATE2_CLOCK_ID 118
#define DIVOOM_USER_DEFINE_CLOCK_ID 1
#define DIVOOM_DEVICE_CLOCK_IMAGE_NAME_LEN 50
#define DIVOOM_DEVICE_CLOCK_IMAGE_NAME_MAX_LEN 55
#define DIVOOM_LIGHT_APP_HTTP_IMAGE_NUM	128	//   90
#define DIVOOM_AMAZON_MUSIC_CLOCK_ID 188
#define DIVOOM_PLAY_GIF_COM_LCD_RGB_CACHE_NUM 10
#define DIVOOM_CHANNEL_CLOCK_LIST_NUM 5
#define DIVOOM_TEMP_CLOCK_ID          2         //这个ID系统不会使用，只用来做临时使用
#define DIVOOM_USER_START_CLOCK_ID 60000
#define DIVOOM_MCP_CLOCK_ID DIVOOM_USER_START_CLOCK_ID
#define DIVOOM_PC_START_CLOCK_ID (DIVOOM_MCP_CLOCK_ID + 1)
#define DIVOOM_CUSTOM_START_CLOCK_ID          100000 
#define DIVOOM_CLOUD_START_CLOCK_ID          70000 
#define DIVOOM_CLOUD_END_CLOCK_ID          80000 
#define DIVOOM_EQ_START_CLOCK_ID             80000 
#define DIVOOM_EQ_END_CLOCK_ID              90000 
#define DIVOOM_USER_DEFINE_START_CLOCK_ID    90000 
#define DIVOOM_USER_DEFINE_END_CLOCK_ID    100000 


#define DIVOOM_DRAW_REMOTE_MAC_RAM_NUM  15
#define DIVOOM_DRAW_REMOTE_MAC_RAM_SIZE  (10*1024)

#define DIVOOM_RAM_POOL_TOTAL_RAM_NUM  (1024*10)
#define DIVOOM_RAM_POOL_BLOCK_RAM_SIZE  (32)

#define DIVOOM_RAM_POOL_TOTAL_MAX_RAM_NUM  (2048)
/* Keep every pool block aligned for LVGL's 64-bit fields. */
#define DIVOOM_RAM_POOL_BLOCK_MAX_RAM_SIZE  (80)

/* 根据真机长期日志的同时使用峰值，为高频固定尺寸提供独立池；可按类型单独关闭。 */
#ifndef DIVOOM_RAM_POOL_104_ENABLE
#define DIVOOM_RAM_POOL_104_ENABLE             (1)
#endif
#define DIVOOM_RAM_POOL_104_BLOCK_NUM          (128)
#define DIVOOM_RAM_POOL_104_BLOCK_SIZE         (104)

#ifndef DIVOOM_RAM_POOL_160_ENABLE
#define DIVOOM_RAM_POOL_160_ENABLE             (1)
#endif
#define DIVOOM_RAM_POOL_160_BLOCK_NUM          (256)
#define DIVOOM_RAM_POOL_160_BLOCK_SIZE         (160)

#ifndef DIVOOM_RAM_POOL_204_212_ENABLE
#define DIVOOM_RAM_POOL_204_212_ENABLE         (1)
#endif
#define DIVOOM_RAM_POOL_204_212_BLOCK_NUM      (128)
/* 204/212 字节共用一类，物理步长向上对齐到 216 字节，保证每块保持 8 字节对齐。 */
#define DIVOOM_RAM_POOL_204_212_BLOCK_SIZE     (216)
#define DIVOOM_RAM_POOL_EXT_MAX_REQUEST_SIZE   (212)

#define DIVOOM_MAX_CLOCK_NUM 1000

//打开这个宏，一直打开，打开是使用新版本的逻辑
//#define DIVOOM_MQTT_ALIYUN_VERSION_V_2_3_0
#define DIVOOM_MQTT_ALIYUN_VERSION_V_3_0_1

#define DIVOOM_MQTT_LIB_USE_ALIYUN
//如果打开下面的宏，要定义DIVOOM_MQTT_LIB_ALIYUN_DS
//这个宏是获取阿里云的设备密码的宏，开发阶段建议打开，找林道蓝拿到这个设备密码，订阅一切都做好了是默认的WiFi的mac地址，如果有冲突需要修改
#define DIVOOM_MQTT_IS_GET_DS_FROM_ALIYUN

//定义这个说明适用的是测试服务器,关闭这个宏则是正式服务器

#define DIVOOM_WIFI_CONFIG_DEVICE_NAME			"PhotoFrameConfig"

#define DIVOOM_BLUE_BLE_DEVICE_NAME				"AstroToo"

#define DIVOOM_BLUE_BLE_NORMAL_NAME				"DIVOOM_PIXOO64"
#define DIVOOM_BLUE_AUDIO_NORMAL_NAME				"Divoom-audio"


#if 0
#define DIVOOM_MQTT_DEVICE_SET_TOPIC	"divoomTest/2/%d/set"
#define DIVOOM_MQTT_DEVICE_GET_TOPIC	"divoomTest/2/%d/get"
#define DIVOOM_MQTT_DEVICE_LWT_TOPIC	"divoomTest/2/%d/state"	
#define	DIVOOM_SERVER_HTTP_URL	"http://120.76.41.61:9999"//"http://apptest.divoom-gz.com"
#define	DIVOOM_SERVER_HTTP_HOST	"http://apptest.divoom-gz.com"
#else
#define	DIVOOM_SERVER_HTTP_URL	        divoom_main_get_http_server_addr()
#ifdef DIVOOM_TEST_TUTV_FLAG
#define	DIVOOM_SERVER_HTTP_USA_URL	        "https://appc.divoom-gz.com"
#define	DIVOOM_SERVER_HTTP_NORMAL_URL	        "https://appc.divoom-gz.com"
#define	DIVOOM_SERVER_HTTP_CHINA_URL	        "https://appc.divoom-gz.com"
#else
#define	DIVOOM_SERVER_HTTP_USA_URL	        "https://appin.divoom-gz.com"
#define	DIVOOM_SERVER_HTTP_NORMAL_URL	        "https://app.divoom-gz.com"
#define	DIVOOM_SERVER_HTTP_CHINA_URL	        "https://appchina.divoom-gz.com"
#endif
#define	DIVOOM_SERVER_HTTPS_USA_URL	        "https://appc.divoom-gz.com"
#define	DIVOOM_SERVER_HTTPS_NORMAL_URL	        "https://appc.divoom-gz.com"
#define	DIVOOM_SERVER_HTTPS_CHINA_URL	        "https://appc.divoom-gz.com"

#define	DIVOOM_SERVER_HTTP_USA_IP	        "https://47.88.33.110/swoole"
#define	DIVOOM_SERVER_HTTP_NORMAL_IP	        "https://47.88.33.110/swoole"
#define	DIVOOM_SERVER_HTTP_CHINA_IP	        "https://120.76.41.61/swoole"

#define DIVOOM_SERVER_URL divoom_main_get_file_server_addr()
#ifdef DIVOOM_TEST_TUTV_FLAG
#define DIVOOM_SERVER_CHINA_URL "https://appc.divoom-gz.com/"
#define DIVOOM_SERVER_USA_URL "https://appc.divoom-gz.com/"
#define DIVOOM_SERVER_DEFAULT_URL "https://appc.divoom-gz.com/"
#else
#define DIVOOM_SERVER_CHINA_URL "https://fchina.divoom-gz.com/"
#define DIVOOM_SERVER_USA_URL "https://fin.divoom-gz.com/"
#define DIVOOM_SERVER_DEFAULT_URL "https://f.divoom-gz.com/"
#endif
#define DIVOOM_SERVER_CHINA_URLS "https://fchina.divoom-gz.com/"
#define DIVOOM_SERVER_USA_URLS "https://fin.divoom-gz.com/"
#define DIVOOM_SERVER_DEFAULT_URLS "https://f.divoom-gz.com/"
#define DIVOOM_SERVER_CHINA_IP "http://120.76.41.61/"
#define DIVOOM_SERVER_USA_IP "http://47.88.33.110/"
#define DIVOOM_SERVER_DEFAULT_IP "http://47.88.33.110/"

#define DIVOOM_SERVER_CHINA_HTTPS_IP "http://120.76.41.61:9506/"
#define DIVOOM_SERVER_USA_HTTPS_IP "http://47.88.33.110:9506/"

#define DIVOOM_FTP_CHINA_IP "120.76.41.61"
#define DIVOOM_FTP_USA_IP "47.88.33.110"


#endif

//aliyun's
#define DIVOOM_MQTT_LIB_ALIYUN_DS		DIVOOM_TEST_DEVICE_SRT

#define DIVOOM_CONFIG_BOND_KEY   kbWIFICONFIG	
#define DIVOOM_WORK_MODE_SWITCH    kbLIGHT

#define PIXOO_WIFI_SSID "pixoo-wifi"
#define PIXOO_WIFI_PASSWD "divoom123"
#define PIXOO_WIFI_IP "192.168.4.1"
#define PIXOO_MAX_STA_CONN   10
#define DIVOOM_KEY_COMBINE_TIME_OUT 3000
#define DIVOOM_DEVICE_APP_HEARBEAT_TIME_OUT 3000
#define DIVOOM_MINILZO_MAX_PACKET_SIZE (20*1024)

//#define DIVOOM_HARDWARE_TEST_ZHANG		

typedef enum
{
	DIVOOM_ONOFF_MODE_NORMAL = 1,		//正常按键检测开机
	DIVOOM_ONOFF_MODE_TEST = 2,		    //测试模式
	DIVOOM_ONOFF_MODE_CLOCK = 3,		//闹钟开机（divoom_alarm RTC 唤醒）
	DIVOOM_ONOFF_MODE_RESET = 4,		//复位开机，将不播放开机提示音
	DIVOOM_ONOFF_MODE_FORMAT_CONFIG = 5,		    //清除本地配置
	DIVOOM_ONOFF_MODE_SHOW = 6,		    //清除本地配置
	DIVOOM_ONOFF_MODE_POWER_SCHEDULE = 7,	//定时开机（power_control，RTC 唤醒）
}DIVOOM_ONOFF_MODE_TYPE;

enum{
	BATTERY_STAT_NULL		   = 0,
	BATTERY_STAT_NORMAL 	   = 1,
	BATTERY_STAT_LOW_WARN	   = 2,
	BATTERY_STAT_LOW_SHUTDOWN  = 3,
	BATTERY_STAT_EXCEEDVOLT    = 4,
};

enum{
	DIVOOM_SD_NAND_ERR_USR_DATA		   = 1,     //usr分区CRC校验失败
	DIVOOM_SD_NAND_ERR_USR_FORMAT	   = 2,     //usr分区异常，需要格式化usr
	DIVOOM_SD_NAND_ERR_USERDATA 	   = 3,     //usrdata分区数据异常，需要格式化userdata
	DIVOOM_SD_NAND_ERR_SQL_DB	       = 4,     //DIVOOM_THUMB_DATA_BASE_FILE_NAME数据库异常，需要格式化userdata
	DIVOOM_SD_NAND_ERR_PROG            = 5,     //程序区损坏
	DIVOOM_SD_NAND_ERR_UBOOT            = 6,     //uboot损坏
	DIVOOM_SD_NAND_ERR_UBOOT_BACK            = 7,     //uboot备份损坏
};

enum{
    DIVOOM_CLOCK_TYPE_NORMAL,
    DIVOOM_CLOCK_TYPE_CLOUD1,
    DIVOOM_CLOCK_TYPE_EQ1,
    DIVOOM_CLOCK_TYPE_USER_DEFINE1,
    DIVOOM_CLOCK_TYPE_USER_DEFINE2,
    DIVOOM_CLOCK_TYPE_USER_DEFINE3,
    DIVOOM_CLOCK_TYPE_USER_DEFINE4,
    DIVOOM_CLOCK_TYPE_USER_DEFINE5,
    DIVOOM_CLOCK_TYPE_USER_DEFINE6,
    DIVOOM_CLOCK_TYPE_USER_DEFINE7,
    DIVOOM_CLOCK_TYPE_USER_DEFINE8,
    DIVOOM_CLOCK_TYPE_USER_DEFINE9,
    DIVOOM_CLOCK_TYPE_USER_DEFINE10,
    DIVOOM_CLOCK_TYPE_CLOUD2,
    DIVOOM_CLOCK_TYPE_CLOUD3,
    DIVOOM_CLOCK_TYPE_CLOUD4,
    DIVOOM_CLOCK_TYPE_CLOUD5,
    DIVOOM_CLOCK_TYPE_CLOUD6,
    DIVOOM_CLOCK_TYPE_CLOUD7,
    DIVOOM_CLOCK_TYPE_CLOUD8,
    DIVOOM_CLOCK_TYPE_CLOUD9,
    DIVOOM_CLOCK_TYPE_CLOUD10,
    DIVOOM_CLOCK_TYPE_EQ2,
    DIVOOM_CLOCK_TYPE_EQ3,
    DIVOOM_CLOCK_TYPE_EQ4,
    DIVOOM_CLOCK_TYPE_EQ5,
    DIVOOM_CLOCK_TYPE_EQ6,
    DIVOOM_CLOCK_TYPE_EQ7,
    DIVOOM_CLOCK_TYPE_EQ8,
    DIVOOM_CLOCK_TYPE_EQ9,
    DIVOOM_CLOCK_TYPE_EQ10,
    DIVOOM_CLOCK_TYPE_DIVOOM_ANIMATION = 31, //APP发过来的动画
    DIVOOM_CLOCK_TYPE_DIVOOM_SYSTEM_DEFAULT = 32, //系统默认播放的一些动画
    DIVOOM_CLOCK_TYPE_DIVOOM_PHOTO_DIAL = 33, //相册表盘
    DIVOOM_CLOCK_TYPE_DIVOOM_VISION_DIAL = 38, //VISION表盘
    DIVOOM_CLOCK_TYPE_DIVOOM_LYRIC_DIAL = 40, //歌词表盘
};

/** CMS 歌词场景表盘 ID 区间（背景+歌词由 lyric_effect 负责，其余元素走表盘配置） */
#define DIVOOM_LYRIC_DIAL_CLOCK_ID_MIN  1611
#define DIVOOM_LYRIC_DIAL_CLOCK_ID_MAX  1622

enum
{
    DIVOOM_NOTICE_LANUAGE_POWER_ON_MSG,         //开机
 	DIVOOM_NOTICE_LANUAGE_CONNECT_WIFI_MSG, //连接WIFI
    DIVOOM_NOTICE_LANUAGE_CONING_WIFI_MSG,      //配置ＷＩＦＩ
    DIVOOM_NOTICE_LANUAGE_DISCON_WIFI_MSG,      //连接ＷＩＦＩ失败
    DIVOOM_NOTICE_LANUAGE_CON_OK_WIFI_MSG,      //连接ＷＩＦＩ成功
    DIVOOM_NOTICE_LANUAGE_BONDING_MSG,             //开始绑定
    DIVOOM_NOTICE_LANUAGE_BOND_OK_MSG,             //绑定成功
    DIVOOM_NOTICE_LANUAGE_BOND_FAIL_MSG,             //绑定失败
    DIVOOM_NOTICE_LANUAGE_BAT_LOW_MSG,             //电量低
    DIVOOM_NOTICE_LANUAGE_CONFIG_UP_MSG,             //配置更新,所有闹钟，纪念日，时间安排设置提醒
    DIVOOM_NOTICE_LANUAGE_END_MSG, 
};

enum{
    DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_MIX,    //混合模式
    DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_FADE_OUT,   //渐隐方式
    DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_PUSH,    //推送方式
    DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_BLINDS,      //百叶效果
    DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_MOSAIC,      //马赛克效果
    DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_NORMAL,    //没有效果
    DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_APP_END_TYPE,        //APP效果结束
    DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_PHOTO_STEAM, //照片流方式
    DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_CREATIVE0,    //创意相框0
    DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_CREATIVE1,    //创意相框1
    DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_CREATIVE2,    //创意相框2
    DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_CREATIVE3,    //创意相框3
    DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_CREATIVE4,    //创意相框4
    DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_CREATIVE5,    //创意相框5
    DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_CREATIVE6,    //创意相框6
    DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_CREATIVE7,    //创意相框7
    DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_CREATIVE8,    //创意相框8
    DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_CREATIVE9,    //创意相框9
    DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_MAGAZINE,    //杂志方式
    DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_MAGAZINE_FOLD,    //折纸方式
    DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_MAGAZINE_FADE_OUT,   //杂志渐隐方式
    DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_MAGAZINE_PUSH,       //杂志推送方式
    DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_ROTATE_ROLLER,     //滚筒旋转方式
    DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_LARGE_IN,    //缩小方式
    DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_ROTATE,     //旋转方式
    DIVOOM_LVGL_PHOTO_SLIDE_EFFECT_END_TYPE,        //效果结束
};

//组件相关类型

//时间样式
enum{
	DIVOOM_CLOCK_SMALL_SET_DS_DIGITAL = 24,//时间样式1
    DIVOOM_CLOCK_SMALL_SET_SMOOTH_LINE = 22,//时间样式2
    DIVOOM_CLOCK_SMALL_SET_WANDER_BRAVE = 14,//时间样式3
    DIVOOM_CLOCK_SMALL_SET_LIBRARY_3_AM = 18,//时间样式4
    DIVOOM_CLOCK_SMALL_SET_ABSOLUTE = 12,//时间样式5
    DIVOOM_CLOCK_SMALL_SET_KEEP_CALM = 20,//时间样式6
    DIVOOM_CLOCK_SMALL_SET_CAREER = 6,//时间样式7
};

//时间样式
enum{
	DIVOOM_CLOCK_SMALL_SET_MONTH_DAY_1 = 1,//1月3日·癸卯年冬月廿二
    DIVOOM_CLOCK_SMALL_SET_MONTH_DAY_2,//距离新年还有38天
    DIVOOM_CLOCK_SMALL_SET_MONTH_DAY_3,//WED 3
    DIVOOM_CLOCK_SMALL_SET_MONTH_DAY_4,//24-1-3
    DIVOOM_CLOCK_SMALL_SET_MONTH_DAY_5,//Wed，Jan 3
};


//小组件，复杂项
enum{
    DIVOOM_CLOCK_SMALL_SET_TODAY_DATA = 1,//今日日期
    DIVOOM_CLOCK_SMALL_SET_CHINAY_DAY,//传统日历
    DIVOOM_CLOCK_SMALL_SET_WEATHER_STAUTS,//天气状况
    DIVOOM_CLOCK_SMALL_SET_REAL_TEMP,//当前温度
    DIVOOM_CLOCK_SMALL_SET_REAL_HUMI,//当前湿度
    DIVOOM_CLOCK_SMALL_SET_MIN_MAX_TEMP,//最高最低温度
    DIVOOM_CLOCK_SMALL_SET_PRESSURE,//大气压
    DIVOOM_CLOCK_SMALL_SET_VISIBILITY,//可见度
    DIVOOM_CLOCK_SMALL_SET_SUNRISE_TIME,//日出时间
    DIVOOM_CLOCK_SMALL_SET_SUNSET_TIME,//日落时间
    DIVOOM_CLOCK_SMALL_SET_WIFI_SINGLE,//WIFI信号
    DIVOOM_CLOCK_SMALL_SET_BATTERY,//电量
    DIVOOM_CLOCK_SMALL_SET_NOISE_VALUE,//噪音
    DIVOOM_CLOCK_SMALL_SET_LUNAR_PHASE,//月相

};

//小组件，复杂项组件内部设备显示所用到的图片ID
enum{
    DIVOOM_IMAGE_SET_ICON_DISP_WEATHER_1 = 1,
    DIVOOM_IMAGE_SET_ICON_DISP_WEATHER_2,
    DIVOOM_IMAGE_SET_ICON_DISP_WEATHER_3,
    DIVOOM_IMAGE_SET_ICON_DISP_WEATHER_4,
    DIVOOM_IMAGE_SET_ICON_DISP_WEATHER_5,
    DIVOOM_IMAGE_SET_ICON_DISP_WEATHER_6,
    DIVOOM_IMAGE_SET_ICON_DISP_WEATHER_7,
    DIVOOM_IMAGE_SET_ICON_DISP_WEATHER_8,
    DIVOOM_IMAGE_SET_ICON_DISP_WEATHER_9,
    DIVOOM_IMAGE_SET_ICON_DISP_WEATHER_10,

    DIVOOM_IMAGE_SET_ICON_DISP_HUMI_1,        //湿度
	DIVOOM_IMAGE_SET_ICON_DISP_HUMI_2,
	DIVOOM_IMAGE_SET_ICON_DISP_HUMI_3,
	DIVOOM_IMAGE_SET_ICON_DISP_HUMI_4,
	DIVOOM_IMAGE_SET_ICON_DISP_HUMI_5,
	DIVOOM_IMAGE_SET_ICON_DISP_HUMI_6,
	DIVOOM_IMAGE_SET_ICON_DISP_HUMI_7,

    DIVOOM_IMAGE_SET_ICON_DISP_MIN_MAX_TEMP_1,//最高最低温度
	DIVOOM_IMAGE_SET_ICON_DISP_MIN_MAX_TEMP_2,
	DIVOOM_IMAGE_SET_ICON_DISP_MIN_MAX_TEMP_3,
	DIVOOM_IMAGE_SET_ICON_DISP_MIN_MAX_TEMP_4,
	DIVOOM_IMAGE_SET_ICON_DISP_MIN_MAX_TEMP_5,
	DIVOOM_IMAGE_SET_ICON_DISP_MIN_MAX_TEMP_6,
	DIVOOM_IMAGE_SET_ICON_DISP_MIN_MAX_TEMP_7,

    DIVOOM_IMAGE_SET_ICON_DISP_PRESSURE_1,    //大气压
	DIVOOM_IMAGE_SET_ICON_DISP_PRESSURE_2,
	DIVOOM_IMAGE_SET_ICON_DISP_PRESSURE_3,
	DIVOOM_IMAGE_SET_ICON_DISP_PRESSURE_4,
	DIVOOM_IMAGE_SET_ICON_DISP_PRESSURE_5,
	DIVOOM_IMAGE_SET_ICON_DISP_PRESSURE_6,
	DIVOOM_IMAGE_SET_ICON_DISP_PRESSURE_7,

    DIVOOM_IMAGE_SET_ICON_DISP_VISIBILITY_1,    //可见度
	DIVOOM_IMAGE_SET_ICON_DISP_VISIBILITY_2,
	DIVOOM_IMAGE_SET_ICON_DISP_VISIBILITY_3,
	DIVOOM_IMAGE_SET_ICON_DISP_VISIBILITY_4,
	DIVOOM_IMAGE_SET_ICON_DISP_VISIBILITY_5,
	DIVOOM_IMAGE_SET_ICON_DISP_VISIBILITY_6,
	DIVOOM_IMAGE_SET_ICON_DISP_VISIBILITY_7,

	DIVOOM_IMAGE_SET_ICON_DISP_SUNRISE, //日出
	DIVOOM_IMAGE_SET_ICON_DISP_SUNSET, //日落

    DIVOOM_IMAGE_SET_ICON_DISP_WIFI_SIGNAL_1,    //wifi信号强度
    DIVOOM_IMAGE_SET_ICON_DISP_WIFI_SIGNAL_2,    //wifi信号强度
    DIVOOM_IMAGE_SET_ICON_DISP_WIFI_SIGNAL_3,    //wifi信号强度
    DIVOOM_IMAGE_SET_ICON_DISP_WIFI_SIGNAL_4,    //wifi信号强度

	DIVOOM_IMAGE_SET_ICON_DISP_BATTERY_1,	 //电池电量 
	DIVOOM_IMAGE_SET_ICON_DISP_BATTERY_2,    //电池电量 
	DIVOOM_IMAGE_SET_ICON_DISP_BATTERY_3,    //电池电量 
	DIVOOM_IMAGE_SET_ICON_DISP_BATTERY_4,    //电池电量 
	DIVOOM_IMAGE_SET_ICON_DISP_BATTERY_5,    //电池电量 

    DIVOOM_IMAGE_SET_ICON_DISP_NOISE_VALUE_1,    //噪音
	DIVOOM_IMAGE_SET_ICON_DISP_NOISE_VALUE_2,
	DIVOOM_IMAGE_SET_ICON_DISP_NOISE_VALUE_3,
	DIVOOM_IMAGE_SET_ICON_DISP_NOISE_VALUE_4,
	DIVOOM_IMAGE_SET_ICON_DISP_NOISE_VALUE_5,
	DIVOOM_IMAGE_SET_ICON_DISP_NOISE_VALUE_6,
	DIVOOM_IMAGE_SET_ICON_DISP_NOISE_VALUE_7,

	DIVOOM_IMAGE_SET_ICON_DISP_LUNAR_PHASE_1,
	DIVOOM_IMAGE_SET_ICON_DISP_LUNAR_PHASE_2,
	DIVOOM_IMAGE_SET_ICON_DISP_LUNAR_PHASE_3,
	DIVOOM_IMAGE_SET_ICON_DISP_LUNAR_PHASE_4,
	DIVOOM_IMAGE_SET_ICON_DISP_LUNAR_PHASE_5,
	DIVOOM_IMAGE_SET_ICON_DISP_LUNAR_PHASE_6,
	DIVOOM_IMAGE_SET_ICON_DISP_LUNAR_PHASE_7,
	DIVOOM_IMAGE_SET_ICON_DISP_LUNAR_PHASE_8,
};

#define DIVOOM_GBL_SYSTEM_CHECK_BIT(map, num) (map[((num) >> 3)] & (1 << ((num) & 7)))
#define DIVOOM_GBL_SYSTEM_SET_BIT(map, num) (map[((num) >> 3)] |= (1 << ((num) & 7)))
#define DIVOOM_GBL_SYSTEM_CLR_BIT(map, num) (map[((num) >> 3)] &= ~(1 << ((num) & 7)))

//按照位顺序来排列，如设置map第15位为设置（uint8）map[1]第0位
#define DIVOOM_SEQUENCE_CHECK_BIT(map, num) (map[((num) >> 3)] & (1 << (7-(num) & 7)))
#define DIVOOM_SEQUENCE_SET_BIT(map, num) (map[((num) >> 3)] |= (1 << (7-(num) & 7)))
#define DIVOOM_SEQUENCE_CLR_BIT(map, num) (map[((num) >> 3)] &= ~(1 << (7-(num) & 7)))


typedef struct _divoom_play_gif_image_info
{
    int image_id;
    int model_id;
    int voice_flag;
    int user_id;
    char image_name[256];
}divoom_play_gif_image_info, *Pdivoom_play_gif_image_info;
typedef void (*DIVOOM_PLAY_GIF_COM_GET_NEW_ID)(void *user_data,uint8 area_id, Pdivoom_play_gif_image_info image_info);
typedef int (*DIVOOM_PLAY_GIF_COM_GET_PLAY_TIME)(void *user_data,int area_id);
typedef int (*DIVOOM_PLAY_GIF_COM_GET_CHANGE_FLAG)(void *user_data);

typedef void (*DIVOOM_GET_EQ_DISP_BUF_CALLBACK)(void *user_data,uint8 *disp_buf, int *image_width);

typedef struct divoom__tzrule_struct
{
    char ch;
    int m;
    int n;
    int d;
    int s;
    time_t change;
    long offset; /* Match type of _timezone. */
} divoom_tzrule_type;

#ifdef DIVOOM_SIMULATOR
#undef _daylight
#undef ERROR
#endif

typedef struct __divoom_tzinfo_struct
{
    int __tznorth;
    int __tzyear;
    int _daylight;
    divoom_tzrule_type __tzrule[2];
} divoom_tzinfo_type;


typedef struct _divoom_lvgl_calendar_event_info
{
    uint8 bg_color[3];
    uint8 fr_color[3];
    int start_time;
    int end_time;
    char *title;
    char *msg_info;
}divoom_lvgl_calendar_event_info, *Pdivoom_lvgl_calendar_event_info;

//ceiling项目是不需要开关机控制的,开机直接走,VJ和其他是需要电源控制的
//其实不加也是可以的,这边都是被动等待的,除了相关的gpio会有影响
//如果是天花板项目,需要实时保存
//这两个是互异的,只能存在一个,或者都不存在
//#define DIVOOM_DEVICE_IS_CEILING
//#define DIVOOM_DEVICE_IS_VJ
//#define DIVOOM_SCREENHANDLE_SPI_RECV
//打开这个宏,所有发送到服务器的命令都用http方式


#include <semaphore.h>
#include <sys/time.h>
#include "divoom_system.h" 

typedef struct _divoom_clock_key_info
{
	uint8 disp_type;
    uint8 index;
    uint8 lcd_id;
    uint8 clock_type;
    int clock_id;
}divoom_clock_key_info, *Pdivoom_clock_key_info;

#ifndef G2D_FORMAT_ARGB8888
#define G2D_FORMAT_ARGB8888 0
#endif
#ifndef G2D_FORMAT_ABGR8888
#define G2D_FORMAT_ABGR8888 1
#endif


typedef int DIVOOM_LIGHT_DISP_OVER_CALLBACK(uint8 mode);
typedef void DIVOOM_LED_SDCREEN_DONE_CALLBACK(void *para);
#include "divoom_log.h"
#include "divoom_huffman_decode.h"
#include "divoom_image_decode.h"
#include "divoom_spectrum.h"
//#include "divoom_env_sensor.h"
#include "divoom_microtask.h"
#include "divoom_buzzer_music.h"
#include "divoom_net.h"
#include "divoom_clock_control.h"
#include "divoom_disp_ctrl.h"
#include "divoom_disp_com.h"
#include "divoom_disp_ctrl_app.h"
#include "divoom_disp_file_manage.h"
#include "divoom_app_schedule.h"
#include "divoom_music_fft.h"
#include "divoom_light_app.h"
#include "divoom_voice_app.h"
#include "divoom_net_command.h"
#include "divoom_sleep_aid_config.h"
#include "divoom_net_app.h"
#include "divoom_net_command_json.h"
#include "divoom_net_command_binary.h"
#include "divoom_net_radio.h"
#include "divoom_com.h"
#include "divoom_os_com.h"
#include "divoom_app_com.h"
#include "divoom_sys_data.h"
#include "divoom_sys_model_ctrl.h"
#include "divoom_mqtt_model.h"
#include "divoom_user_key.h"
#include "divoom_net_mqtt_manage.h"
#include "divoom_net_http_manage.h"
#include "divoom_disp_picture.h"
#include "divoom_app_file_manage.h"
#include "divoom_wifi_con.h"
#include "divoom_mic_ctrl.h"
#include "divoom_server_msg_manage.h"
#include "divoom_general_json.h"
#include "divoom_main.h"
#include "divoom_time_plan.h"
#include "divoom_clock_play_plan.h"
#include "divoom_clock_play_plan_json.h"
#include "divoom_format_srv_file.h"
#include "divoom_app_onoff.h"
#include "divoom_ota_server.h"
#include "divoom_utf8.h"
#include "divoom_noise.h"
#include "divoom_alarm_lib.h"
#include "divoom_alarm.h"
#include "divoom_sleep.h"
#include "divoom_game.h"
//#include "divoom_noise_fft.h"
#include "divoom_minilzo.h"
#include "divoom_image_decode.h"

#include "divoom_app_config_wifi.h"
#include "divoom_product_test.h"

#include "divoom_lamp_json.h"
#include "divoom_eq_noise_mic_fft.h"
#include "divoom_app_count_time_down.h"
#include "divoom_keypad.h"
#include "divoom_power_on_manage.h"


#include "divoom_web_get.h"
#include "divoom_image_with_text_disp.h"
#include "divoom_inflow_display.h"

#include "divoom_http_download_image.h"
#include "divoom_net_image_cache.h"
#include "divoom_sd_file_list_manage.h"
#include "divoom_manage_image.h"
#include "divoom_expert_manage.h"
#include "divoom_hot_manage.h"
#include "divoom_favorite_manage.h"
#include "divoom_play_gif_com.h"
#include "divoom_app_tool.h"
#include "divoom_user_define_ctrl.h"
#include "divoom_gif_ctrl.h"
#include "divoom_clock_manage.h"
#include "divoom_font_manage.h"
#include "divoom_system_font.h"
#include "divoom_app_pic_manage.h"
#include "divoom_save_all_config.h"
#include "divoom_exchange_pic.h"
#include "divoom_cache_clock_data.h"
#include "divoom_local_use_pic.h"
#include "divoom_disp_play_net_image.h"
#include "divoom_disp_iffft_text.h"
#include "divoom_user_head_ctrl.h"
#include "divoom_clock_notice.h"
#include "divoom_rrule.h"
#include "divoom_google_calendar.h"
#include "divoom_save_to_sd.h"
#include "divoom_album_manage.h"
#include "divoom_gif_lib.h"
#include "divoom_play_local_gif.h"
#include "divoom_disp_local_gif.h"
#include "divoom_http_font_cache.h"
#include "divoom_twitch_get_data.h"
#include "divoom_qrencode.h"
#include "divoom_clock_schedule.h"
#include "divoom_user_define_schedule.h"
#include "divoom_gif_font.h"
#include "divoom_clock_gif_control.h"
#include "divoom_disp_lcd_clock.h"
#include "divoom_disp_single_clock.h"
#include "divoom_disp_mul_gif_file.h"
#include "divoom_disp_custom_digit_clock.h"
#include "divoom_disp_custom_divoom_gif.h"
#include "divoom_disp_turn_dial.h"
#include "divoom_disp_custom_gif.h"
#include "divoom_lvgl_eq_effect.h"
#include "divoom_cup2022.h"
#include "divoom_disp_combine_dial.h"
#include "divoom_disp_font_dial.h"
#include "divoom_disp_whole_clock.h"
#include "divoom_weather_control.h"
#include "divoom_play_cache_com.h"
#include "divoom_random.h"
#include "divoom_sonos_ctrl.h"
#include "divoom_home_connect_ctrl.h"
//#include "divoom_clock_config.h"
#include "divoom_disp_word_whole.h"
#include "divoom_aes.h"

#include "divoom_pic_album_manage.h"
#include "divoom_thumb_cache.h"
#include "divoom_pic_check.h"
#include "divoom_pic_phone_manage.h"
#include "divoom_bt_ctrl.h"
#include "divoom_onoff.h"
#include "divoom_disp_base_clock.h"
#include "divoom_rk_mpp.h"
#include "divoom_libwebp_decode.h"
#include "divoom_commit_clock_config.h"
#include "divoom_component_manage.h"
#include "divoom_component_config.h"
#include "divoom_ai_talk.h"
#include "divoom_bailian_character_text.h"
#include "divoom_photo_album_josn.h"
#include "divoom_database_manage.h"
#include "divoom_download_aliyun_file.h"
#include "divoom_tomato_time.h"
#include "divoom_local_api_request.h"
#include "divoom_line_in.h"
#include "divoom_uac.h"
#include "divoom_tf_music.h"
#include "divoom_music_ctrl.h"
#include "divoom_local_music_ctrl.h"
#include "divoom_local_movie_play.h"
#include "divoom_decode_movie_review_pic.h"
#include "divoom_voice_read.h"
#include "divoom_ring_buffer.h"
#include "divoom_white_noise.h"
#include "divoom_white_noise_ctrl.h"
#include "divoom_snd_pcm.h"
#include "divoom_mix_pcm_ctrl.h"
#include "divoom_charge.h"
#include "divoom_software_timer.h"
#include "divoom_led_control.h"
#include "divoom_zstd_manage.h"
#include "divoom_webp_zstd_mix.h"
#include "divoom_http_server.h"
#include "divoom_disp_clock_main.h"
#include "divoom_disp_clock.h"
#include "divoom_subscribe_manage.h"
#include "divoom_eq_image_manage.h"
#include "divoom_timelib.h"
#include "divoom_photo_play_ctrl.h"
#include "divoom_disp_ambient_ctrl.h"
#include "divoom_memory_pool.h"
#include "divoom_decode_jpeg_gif.h"
#include "divoom_read_pcm.h"
#include "divoom_pcm_record.h"
#include "divoom_mic_record.h"
#include "divoom_pcm_mp3_encode.h"
#include "divoom_decode_jpg_bg.h"
#include "divoom_user_head_manage.h"
#include "divoom_nba_pic.h"
#include "divoom_image_font_cache.h"
#include "divoom_custom_photo.h"
#include "divoom_clock_image_manage.h"
#include "divoom_speex_record.h"
#include "divoom_update_system_file.h"
#include "divoom_anniversary_clock_reminder.h"
#include "divoom_anniversary_clock_config_deal.h"
#include "divoom_social_clock_reminder.h"
#include "divoom_lvgl_top_control.h"
#include "divoom_system_msg_notice.h"
#include "divoom_outlook_calendar.h"
#include "divoom_google_calendar.h"
#include "divoom_custom_dial_ctrl.h"
#include "divoom_vision_ctrl.h"
#include "divoom_check_system_lock.h"
#include "divoom_userimagelist_manage.h"
#include "divoom_stock_chart.h"
#include "divoom_marble_path.h"
#include "divoom_cloud_photo_ctrl.h"
#include "divoom_input_hotplug.h"
#include "divoom_sd_card_manage.h"
#include "divoom_acm8625_app_init.h"
#include "divoom_acm8625p_amp.h"
#include "divoom_white_noise_player.h"
#include "divoom_power_monitor.h"
#include "divoom_battery_voltage.h"
#include "divoom_btcore_uart_cmd.h"
#include "divoom_btcore_key_light.h"
#include "divoom_btcore_file_rx.h"
#ifdef DIVOOM_TIMES_VIBE
#include "divoom_btcore_file_tx.h"
#include "divoom_ota_jl.h"
#endif
#include "divoom_sleep_aid_json.h"
#include "divoom_sleep_aid_config.h"
#include "divoom_sleep_aid_ctrl.h"

#include "divoom_leave_msg_ctrl.h"

#ifdef DIVOOM_SIMULATOR
#include "divoom_sim_config.h"
#endif

enum{
    DIVOOM_IMAGE_BUF_TYPE_RGB_RAW,
    DIVOOM_IMAGE_BUF_TYPE_MINILZO_ENCODE,
    DIVOOM_IMAGE_BUF_TYPE_DIVOOM_ENCODE,
    DIVOOM_IMAGE_BUF_TYPE_GIF_ENCODE,	//部分缓冲
	DIVOOM_IMAGE_BUF_TYPE_GIF_ENCODE2, //全缓冲
	DIVOOM_IMAGE_BUF_TYPE_PIC_SCAN_MODE,
	DIVOOM_IMAGE_BUF_TYPE_DIVOOM_WORD,  //文字帧
    DIVOOM_IMAGE_BUF_TYPE_JPEG_ENCODE,
    DIVOOM_IMAGE_BUF_TYPE_ZSTD_ENCODE,
    DIVOOM_IMAGE_BUF_TYPE_ZSTD_WEBP_ENCODE,
    DIVOOM_IMAGE_BUF_TYPE_UNKNOW_TYPE,
    DIVOOM_IMAGE_BUF_TYPE_RGB565_RAW,
};

enum{
    DIVOOM_TASK_RUN_STATUS_READ = 1,
    DIVOOM_TASK_RUN_STATUS_RUN = 2,
    DIVOOM_TASK_RUN_STATUS_OTHER = 4,
    DIVOOM_TASK_RUN_STATUS_EXIT = 8,
};

enum
{
	/*
	   1,2,5,6,7,10,11, 14不用了
	*/
	/*no use start↓*/
    DIVOOM_NET_FILE_TYPE_SINGLE_11 = 1,
    DIVOOM_NET_FILE_TYPE_GIF_11 = 2,
	/*no use end↑*/
    DIVOOM_NET_FILE_TYPE_SINGLE_16 = 3,
    DIVOOM_NET_FILE_TYPE_GIF_16 = 4,
	/*no use start↓*/
    DIVOOM_NET_FILE_TYPE_MUL_SINGLE = 5,
    DIVOOM_NET_FILE_TYPE_MUL_GIF = 6,
	//app发送实时的数据过来
    DIVOOM_NET_FILE_TYPE_LED_APP = 7,
	/*no use end↑*/
    DIVOOM_NET_FILE_TYPE_SINGLE_AES = 8,
    DIVOOM_NET_FILE_TYPE_GIF_AES = 9,
	/*no use start↓*/
    DIVOOM_NET_FILE_TYPE_SAND_PAINT = 10,
    DIVOOM_NET_FILE_TYPE_MUL_SAND_PAINT = 11,
	/*no use end↑*/
    DIVOOM_NET_FILE_TYPE_PIC_SCAN = 12,
    DIVOOM_NET_FILE_TYPE_SAND_PAINT_REVIEW = 13,
	/*no use start↓*/
    DIVOOM_NET_FILE_TYPE_MUL_SAND_PAINT_REVIEW = 14,
    DIVOOM_NET_FILE_TYPE_LED_DEVICE = 15,
    DIVOOM_NET_FILE_TYPE_GIF_SET = 16,
	/*no use end↑*/
    DIVOOM_NET_FILE_TYPE_NEW_MUL_SINGLE = 17,
    DIVOOM_NET_FILE_TYPE_NEW_MUL_GIF = 18,
    DIVOOM_NET_FILE_TYPE_NEW_MUL_SAND_PAINT = 19,
    DIVOOM_NET_FILE_TYPE_ALL_MUL_GIF_INFO = 25, //所有图像
    DIVOOM_NET_FILE_TYPE_ALL_DIVOOM_GIF_INFO = 26, //使用新的ＤＩＶＯＯＭ的压缩算法
    DIVOOM_NET_FILE_TYPE_ALL_JPEG_GIF_INFO = 31, //使用JPEG的压缩算法
    DIVOOM_NET_FILE_TYPE_ALL_ZSTD_GIF_INFO = 37, //ZSTD压缩（
    DIVOOM_NET_FILE_TYPE_ALL_MIX_ZSTD_WEPB_INFO = 41, //webp，zstd, jPEG混合
    DIVOOM_NET_FILE_TYPE_ALL_NEW_ZSTD_INFO = 42, //ZSTD压缩（
    DIVOOM_NET_FILE_TYPE_ALL_NEW_GIF_INFO = 43, //使用GIF压缩
};

enum
{
    DIVOOM_PARTITION_ROOTFS_DATA = 1,
    DIVOOM_PARTITION_DIVOOM_CONFIG = 2,
    DIVOOM_PARTITION_USER_DATA = 4,
};


//这个先关闭，打开的话需要调整分区,要减小spiffs的分区大小
#define NEED_MALLOC_NULL_CHECK


#define DIVOOM_READ_ONLY_FONT_END_ID    142
#define DIVOOM_PC_PARA_CLOCK_ID   1018
#define DIVOOM_PHOTO_START_CLOCK_ID   100000

enum{
    DIVOOM_RESOURCE_TYPE_WEBP,
    DIVOOM_RESOURCE_TYPE_GIF,
    DIVOOM_RESOURCE_TYPE_MP4,
    DIVOOM_RESOURCE_TYPE_DIVOOM,
    DIVOOM_RESOURCE_TYPE_JPEG_GIF,  //DIVOOM打包jpeg的动画
    DIVOOM_RESOURCE_TYPE_JPEG,  //标准的JPEG文件
    DIVOOM_RESOURCE_TYPE_IMAGE_FONT,  //图像字体
    DIVOOM_RESOURCE_TYPE_PNG,
    DIVOOM_RESOURCE_TYPE_ERR,
};


#define DIVOOM_SCHEDULE_SCREEN_RESOUCE_TYPE ((1 << DIVOOM_APP_SCHEDULE_RESOURCCE_RGB) | (1 << DIVOOM_APP_SCHEDULE_RESOURCCE_WHITE))
#define DIVOOM_SCHEDULE_VOICE_RESOUCE_TYPE ((1 << DIVOOM_APP_SCHEDULE_RESOURCE_VOICE))
#define DIVOOM_SCHEDULE_RGB_RESOUCE_TYPE ((1 << DIVOOM_APP_SCHEDULE_RESOURCCE_RGB))
#define DIVOOM_SCHEDULE_WHITE_RESOUCE_TYPE ((1 << DIVOOM_APP_SCHEDULE_RESOURCCE_WHITE))


#define DIVOOM_SCHEDULE_ALL_RESOUCE_TYPE (DIVOOM_SCHEDULE_SCREEN_RESOUCE_TYPE | DIVOOM_SCHEDULE_VOICE_RESOUCE_TYPE)

#define DIVOOM_SYSTEM_DEFAULT_FONT_NUM  8
#define DIVOOM_ALBUM_MANAGE_TABLE_NAME   "album_manage"     //社区专辑管理
#define DIVOOM_ALBUM_MANAGE_CONFIG_TABLE_NAME   "album_manage_config"     //社区专辑管理
#define DIVOOM_USER_DEFINE_TABLE_NAME   "user_define"
#define DIVOOM_USER_DEFINE_CONFIG_TABLE_NAME   "user_define_config"
#define DIVOOM_HOT_IMAGE_TABLE_NAME   "hot_manage"
#define DIVOOM_USERIMAGELIST_TABLE_NAME   "image_list_manage"
#define DIVOOM_USERIMAGELIST_CONFIGTABLE_NAME   "image_list_config"
#define DIVOOM_FAVORITE_IMAGE_TABLE_NAME   "favorite_manage"
#define DIVOOM_EXPERT_IMAGE_TABLE_NAME   "expert_manage"
#define DIVOOM_EXPERT_IMAGE_CONFIG_TABLE_NAME   "expert_manage_config"
#define DIVOOM_SUBSCRIBE_IMAGE_TABLE_NAME   "subscribbe_manage"
#define DIVOOM_EQ_IMAGE_TABLE_NAME   "eq_image_manage"
#define DIVOOM_EQ_CONFIG_TABLE_NAME   "eq_config_manage"
#define DIVOOM_CLOCK_UPDATE_TIME_TABLE_NAME   "clock_update_time"
#define DIVOOM_CLOCK_UPDATE_APP_TIME_TABLE_NAME   "clock_app_time"
#define DIVOOM_USER_HEAD_IMAGE_TABLE_NAME   "user_head"
#define DIVOOM_CUSTOM_PHOTO_TABLE_NAME   "custom_photo"     //自定义
#define DIVOOM_CUSTOM_VISION_TABLE_NAME   "vision_photo"     //自定义
#define DIVOOM_CLOUD_PHOTO_TABLE_NAME   "cloud_photo"     //自定义

#define DIVOOM_PLAY_MOIVE_MAX_VOLUME   50
#define DIVOOM_PIC_MANAGE_LOAD_MAX_PIC_NUM 100

#define DIVOOM_SERVER_IMAGE_TABLE_NAME   "divoom_server_image"
#define DIVOOM_IOT_USA_DEVICE_START_ID  600000000
#define DIVOOM_TEST_SERVER_URL "http://192.168.1.100/"
#define DIVOOM_MQTT_APP_TOPIC	"DivoomApp"
#define DIVOOM_MQTT_APP_BIN_TOPIC  "DivoomAppBin"
#define DIVOOM_MQTT_TO_APP_TOPIC	"DivoomDevice"
#define DIVOOM_MQTT_TO_DEBUG_TOPIC	"DivoomDebug"
#define DIVOOM_MQTT_APP_LWT_TOPIC 	"DivoomAppLwt"
#define DIVOOM_USER_DATA_DIR 	DIVOOM_USER_ROOT
#define DIVOOM_TF_MUSIC_NAND_DIR DIVOOM_USER_DATA_DIR "/tf_music"
#define DIVOOM_CONFIG_DIR 	DIVOOM_USER_ROOT
#define DIVOOM_RUN_LOG_FILE_PATH DIVOOM_USER_DATA_DIR"/divoom_run.log"
#define DIVOOM_ERR_LOG_FILE_PATH DIVOOM_USER_DATA_DIR"/divoom-config.tar"
#define DIVOOM_APP_SHA256_SUM_FILE_PATH "/usr/share/divoom_app/divoom_app_md5.txt"
#define DIVOOM_RUN_FAIL_LOG_FILE_PATH DIVOOM_USER_DATA_DIR"/divoom_fail.log"
#define DIVOOM_SD_SYSTEM_DIR DIVOOM_CONFIG_DIR"/system"
#define DIVOOM_SD_APP_PIC_DIR DIVOOM_USER_DATA_DIR"/app_pic"
#define DIVOOM_SD_APP_PARA_DIR DIVOOM_USER_DATA_DIR"/app_para"
#define DIVOOM_AI_TALK_SAVE_PATH DIVOOM_SD_SYSTEM_DIR"/ai_talk.cfg"
#define DIVOOM_AI_TALK_CACHE_DIR DIVOOM_SD_SYSTEM_DIR"/ai_talk"
#define DIVOOM_SD_UPDATE_SYS_FILE_DIR DIVOOM_USER_DATA_DIR"/divoom_update"
#define DIVOOM_OTA_RTOS_STAGING_DIR       DIVOOM_USER_DATA_DIR"/ota"
#define DIVOOM_OTA_RTOS_STAGING_DOWNLOAD  DIVOOM_OTA_RTOS_STAGING_DIR"/staging.pkg"
#define DIVOOM_OTA_JL_FW_TMP_PATH         DIVOOM_OTA_RTOS_STAGING_DIR"/jl_firmware.bin"
#define DIVOOM_SD_APP_API_DIR DIVOOM_USER_DATA_DIR"/app_api"
#define DIVOOM_SD_SYSTEM_SYS_CLOCKS_DIR DIVOOM_SD_SYSTEM_DIR"/clocksys"
#define DIVOOM_SD_SYSTEM_APP_CLOCKS_DIR DIVOOM_SD_SYSTEM_DIR"/clockapp"
#define DIVOOM_READ_SYS_CLOCKS_DIR DIVOOM_PRESET_ROOT"/clocksys"
#define DIVOOM_READ_APP_CLOCKS_DIR DIVOOM_PRESET_ROOT"/clockapp"
#define DIVOOM_SD_SYSTEM_COMPONENT_DIR DIVOOM_SD_SYSTEM_DIR"/component"
/* Legacy name: preset recommend images under /usr (read-only) */
#define DIVOOM_SD_RECOMMEND_DIR DIVOOM_PRESET_RECOMMEND_DIR
#define DIVOOM_USER_RECOMMEND_DIR DIVOOM_WRITE_RECOMMEND_DIR
#define DIVOOM_DEBUG_FILE_NAME DIVOOM_USER_DATA_DIR"/debug.txt"
#define DIVOOM_DEBUG_BAKE_FILE_NAME DIVOOM_USER_DATA_DIR"/debug_bak.txt"
#define DIVOOM_VISION_INFO_DIR DIVOOM_USER_DATA_DIR"/divoom_vision"
#define DIVOOM_CLOUD_PHOTO_INFO_DIR DIVOOM_USER_DATA_DIR"/divoom_cloud_photo"
#define DIVOOM_THUMB_INFO_DIR DIVOOM_USER_DATA_DIR"/divoom_thumb"
#define DIVOOM_ALBUM_COVER_INFO_DIR DIVOOM_USER_DATA_DIR"/divoom_thumb/album"
#define DIVOOM_RUN_SOFTWARE_VERSION_SAVE_PATH DIVOOM_SD_SYSTEM_DIR"/divoom_version.cfg"
#define DIVOOM_TIME_PLAN_SAVE_PATH DIVOOM_SD_SYSTEM_DIR"/time_plan.cfg"
#define DIVOOM_CLOCK_PLAY_PLAN_SAVE_PATH DIVOOM_SD_SYSTEM_DIR"/clock_play_plan.cfg"
#define DIVOOM_SYS_CFG_SAVE_PATH DIVOOM_SD_SYSTEM_DIR"/system_config.cfg"
#define DIVOOM_USER_CLOCK_ID_LIST_SAVE_PATH DIVOOM_SD_SYSTEM_DIR"/user_clock_id_list.json"
/** 关机前写入：下次 RTC 闹钟唤醒来自 divoom_alarm('A') 还是定时开机('P')，第二字节为 power_control 槽位索引 */
#define DIVOOM_WAKE_KIND_PATH DIVOOM_SD_SYSTEM_DIR"/wake_kind.bin"
#define DIVOOM_IP6103_WAKE_KIND_PATH DIVOOM_WAKE_KIND_PATH
#define DIVOOM_WAKE_KIND_PATH_LEGACY DIVOOM_WAKE_KIND_PATH
#define DIVOOM_PRODUCT_FLAG_SAVE_PATH "/usr/share/divoom_app/cgi-bin/product_flag.cfg"
#define DIVOOM_SLEEP_SPIFFS_CONFIG	DIVOOM_SD_SYSTEM_DIR"/divoom_sleep_config.cfg"
#define DIVOOM_ALARM_INFO_PATH		DIVOOM_SD_SYSTEM_DIR"/divoom_alarm_config.cfg"
#define DIVOOM_REMINDER_INFO_PATH		DIVOOM_SD_SYSTEM_DIR"/divoom_reminder_config.cfg"
#define DIVOOM_CLOCK_SCHEDULE_SAVE_PATH DIVOOM_SD_SYSTEM_DIR"/clock_schedule.cfg"
#define DIVOOM_HOT_MANAGE_SAVE_PATH DIVOOM_SD_SYSTEM_DIR"/hot_manage.cfg"
#define DIVOOM_ALBUM_MANAGE_SAVE_PATH DIVOOM_SD_SYSTEM_DIR"/album_manage.cfg"
#define DIVOOM_FAVORITE_MANAGE_SAVE_PATH DIVOOM_SD_SYSTEM_DIR"/favorite_manage.cfg"
#define DIVOOM_EXPERT_MANAGE_EXPERT_LIST_PATH DIVOOM_SD_SYSTEM_DIR"/expert_list.cfg"
#define DIVOOM_SYSTEM_FILE_VERSOIN2_PATH "/usr/share/divoom_app/system_file.cfg"
#define DIVOOM_EXPERT_MANAGE_EXPERT_ID_PATH DIVOOM_SD_SYSTEM_DIR"/expert_%d.cfg"
#define DIVOOM_DEVICE_NOTICE_SAVE_PATH DIVOOM_SD_SYSTEM_DIR"/device_notice.cfg"
#define DIVOOM_USER_DEFINE_LIST_SAVE_PATH DIVOOM_SD_SYSTEM_DIR"/user_define.cfg"
#define DIVOOM_DISP_AMBIENT_INFO_SAVE_PATH DIVOOM_SD_SYSTEM_DIR"/ambient_config.cfg"
#define DIVOOM_APP_TOOL_INFO_SAVE_PATH DIVOOM_SD_SYSTEM_DIR"/app_tool_config.cfg"
#define DIVOOM_WIFI_SSID_SAVE_PATH DIVOOM_SD_SYSTEM_DIR"/wifi_ssid.cfg"
#define DIVOOM_LOCAL_GIF_BIN_SAVE_PATH DIVOOM_TMP_DIR "/divoom_gif.bin"
#define DIVOOM_SYS_CLOCK_CONFIG_UPDATE_TIME DIVOOM_SD_SYSTEM_DIR"/sys_clock_time.cfg"
#define DIVOOM_APP_CLOCK_CONFIG_UPDATE_TIME DIVOOM_SD_SYSTEM_DIR"/app_clock_time.cfg"
#define DIVOOM_SYS_COMPONENT_CONFIG_UPDATE_TIME DIVOOM_SD_SYSTEM_DIR"/sys_component_time.cfg"
#define DIVOOM_USER_DEFINE_PIC_VERSION_SAVE_PATH DIVOOM_SD_SYSTEM_DIR"/clock_user.cfg"
#define DIVOOM_ALL_FONT_LIST_SAVE_PATH DIVOOM_SD_SYSTEM_DIR"/font_list.cfg"
#define DIVOOM_ALL_FONT_LIST_BAK_PATH DIVOOM_BAK_SYSTEM_DIR"/font_list.cfg"
#define DIVOOM_ALL_FONT_NAME_SAVE_PATH DIVOOM_SD_SYSTEM_DIR"/font_name.cfg"
#define DIVOOM_ALL_FONT_NAME_BAK_PATH DIVOOM_BAK_SYSTEM_DIR"/font_name.cfg"
#define DIVOOM_ALL_APP_PIC_SAVE_PATH DIVOOM_SD_SYSTEM_DIR"/app_pic.cfg"
#define DIVOOM_DEIVCE_POWERON_PIC_SAVE_PATH DIVOOM_SD_SYSTEM_DIR"/poweron_pic%d.bin"
#define DIVOOM_DEIVCE_CLOCK_USER_HEAD_SAVE_PATH DIVOOM_SD_SYSTEM_DIR"/user_head.bin"
#define DIVOOM_DEIVCE_POWER_FLAG_SAVE_PATH DIVOOM_SD_SYSTEM_DIR"/power_flag.bin"
#define DIVOOM_DEIVCE_DIVOOM_APP_GIF_SAVE_PATH DIVOOM_SD_SYSTEM_DIR"/divoom_gif%d.bin"
#define DIVOOM_DEIVCE_POWERON_DEFAULT_STATIC DIVOOM_SD_SYSTEM_DIR"/poweron_pic_default.bin"
#define DIVOOM_SD_CHECK_FILE_NAME     DIVOOM_SD_SYSTEM_DIR"/divoom_file_check.bin"
#define DIVOOM_RESET_SYS_CLOCK_FILE_NAME     DIVOOM_SD_SYSTEM_DIR"/reset_clock.bin"
#define DIVOOM_TOMATO_INFO_SAVE_PATH DIVOOM_SD_SYSTEM_DIR"/divoom_tomato_config.cfg"
#define DIVOOM_TOMATO_FOCUS_RECORD_SAVE_PATH DIVOOM_SD_SYSTEM_DIR"/divoom_tomato_focus_record.cfg"
#define DIVOOM_TOMATO_TODAY_FOCUS_TOMATO_TOTAL_PATH DIVOOM_SD_SYSTEM_DIR"/divoom_tomato_total.cfg"
#define DIVOOM_PHOTO_SETTING_CFG_PATH DIVOOM_SD_SYSTEM_DIR"/divoom_photo_setting.cfg"
#define DIVOOM_WHITE_NOISE_SAVE_PATH DIVOOM_SD_SYSTEM_DIR"/white_noise.cfg"
#define DIVOOM_WHITE_NOISE_PLAYER_SAVE_PATH DIVOOM_SD_SYSTEM_DIR"/white_noise_player.cfg"
#define DIVOOM_CLOCK_FAVORITE_SAVE_PATH DIVOOM_SD_SYSTEM_DIR"/dial_favorite.cfg"
#define DIVOOM_LYRIC_PLAY_CTRL_PATH DIVOOM_SD_SYSTEM_DIR"/lyric_play.cfg"
#define DIVOOM_EMPTY_ALBUM_PHOTO_NAME "/usr/share/divoom_app/ui/15/15_empty_album.webp"

#define DIVOOM_ALL_EXCHANGE_RATE_PIC_SAVE_PATH DIVOOM_SD_SYSTEM_DIR"/exchange_rate_pic.cfg"
#define DIVOOM_ALL_NBA_PIC_SAVE_PATH DIVOOM_SD_SYSTEM_DIR"/nba_pic.cfg"
#define DIVOOM_ALL_EURCUP_PIC_SAVE_PATH DIVOOM_SD_SYSTEM_DIR"/european_pic.cfg"
#define DIVOOM_ALL_NFL_PIC_SAVE_PATH DIVOOM_SD_SYSTEM_DIR"/nfl_pic.cfg"
#define DIVOOM_ALL_PREMIER_LEAGUE_PIC_SAVE_PATH DIVOOM_SD_SYSTEM_DIR"/premier_league_pic.cfg" 
#define DIVOOM_ALL_LALIGA_PIC_SAVE_PATH DIVOOM_SD_SYSTEM_DIR"/laliga_pic.cfg" 
#define DIVOOM_ALL_SERIEA_PIC_SAVE_PATH DIVOOM_SD_SYSTEM_DIR"/seriea_pic.cfg"
#define DIVOOM_ALL_BUNDESLIGA_PIC_SAVE_PATH DIVOOM_SD_SYSTEM_DIR"/bundesliga_pic.cfg"
#define DIVOOM_ALL_MLB_PIC_SAVE_PATH DIVOOM_SD_SYSTEM_DIR"/mlb_pic.cfg"
#define DIVOOM_ALL_AFL_PIC_SAVE_PATH DIVOOM_SD_SYSTEM_DIR"/afl_pic.cfg"

#define DIVOOM_ALL_EXCHANGE_RATE_PIC_SAVE_BAK_PATH DIVOOM_BAK_SYSTEM_DIR"/exchange_rate_pic.cfg"
#define DIVOOM_ALL_NBA_PIC_SAVE_BAK_PATH DIVOOM_BAK_SYSTEM_DIR"/nba_pic.cfg"
#define DIVOOM_ALL_EURCUP_PIC_SAVE_BAK_PATH DIVOOM_BAK_SYSTEM_DIR"/european_pic.cfg"
#define DIVOOM_ALL_NFL_PIC_SAVE_BAK_PATH DIVOOM_BAK_SYSTEM_DIR"/nfl_pic.cfg"
#define DIVOOM_ALL_PREMIER_LEAGUE_PIC_SAVE_BAK_PATH DIVOOM_BAK_SYSTEM_DIR"/premier_league_pic.cfg" 
#define DIVOOM_ALL_LALIGA_PIC_SAVE_BAK_PATH DIVOOM_BAK_SYSTEM_DIR"/laliga_pic.cfg" 
#define DIVOOM_ALL_SERIEA_PIC_SAVE_BAK_PATH DIVOOM_BAK_SYSTEM_DIR"/seriea_pic.cfg"
#define DIVOOM_ALL_BUNDESLIGA_PIC_SAVE_BAK_PATH DIVOOM_BAK_SYSTEM_DIR"/bundesliga_pic.cfg"
#define DIVOOM_ALL_MLB_PIC_SAVE_BAK_PATH DIVOOM_BAK_SYSTEM_DIR"/mlb_pic.cfg"
#define DIVOOM_ALL_AFL_PIC_SAVE_BAK_PATH DIVOOM_BAK_SYSTEM_DIR"/afl_pic.cfg"



#define DIVOOM_CLOCK_MANAGE_IMG DIVOOM_SD_SYSTEM_DIR"/clock_url_image.cfg"
#define DIVOOM_GOOGLE_CALENDAR_CONFIG DIVOOM_SD_SYSTEM_DIR"/google_calendar_config.cfg"
#define DIVOOM_OUTLOOK_CALENDAR_CONFIG DIVOOM_SD_SYSTEM_DIR"/outlook_calendar_config.cfg"
#define DIVOOM_STORE_CLOCK_PREFETCH_SAVE_PATH DIVOOM_SD_SYSTEM_DIR"/store_clock_prefetch.cfg"
#define DIVOOM_STORE_CLOCK_MARKET_SAMPLES_SAVE_PATH DIVOOM_SD_SYSTEM_DIR"/store_clock_market_samples.cfg"
#define DIVOOM_STORE_CLOCK_MARKET_FULL_SAVE_PATH DIVOOM_SD_SYSTEM_DIR"/store_clock_market_full.cfg"
#define DIVOOM_STORE_CLOCK_MARKET_CLASSIFY_SAVE_PATH DIVOOM_SD_SYSTEM_DIR"/store_clock_market_classify.cfg"

#define DIVOOM_SLEEP_AID_CONFIG_PATH DIVOOM_SD_SYSTEM_DIR"/sleep_aid_%02d.cfg"
#define DIVOOM_SLEEP_AID_LANGUAGE_PATH DIVOOM_SD_SYSTEM_DIR"/sleep_lang_%02d_%s.cfg"
#define DIVOOM_SLEEP_AID_FAV_PATH DIVOOM_SD_SYSTEM_DIR"/sleep_fav.cfg"
#define DIVOOM_SLEEP_AID_SETTINGS_PATH DIVOOM_SD_SYSTEM_DIR"/sleep_aid_settings.cfg"
#define DIVOOM_SLEEP_AID_RECORD_INDEX_PATH DIVOOM_SD_SYSTEM_DIR"/sleep_aid_record_index.cfg"
#define DIVOOM_SLEEP_AID_UPDATE_TIME_PATH DIVOOM_SD_SYSTEM_DIR"/sleep_update.cfg"

#define DIVOOM_SLEEP_AID_CONFIG_BAK_PATH DIVOOM_BAK_SYSTEM_DIR"/sleep_aid_%02d.cfg"
#define DIVOOM_SLEEP_AID_LANGUAGE_BAK_PATH DIVOOM_BAK_SYSTEM_DIR"/sleep_lang_%02d_%s.cfg"
#define DIVOOM_SLEEP_AID_FAV_BAK_PATH DIVOOM_BAK_SYSTEM_DIR"/sleep_fav.cfg"
#define DIVOOM_SLEEP_AID_SETTINGS_BAK_PATH DIVOOM_BAK_SYSTEM_DIR"/sleep_aid_settings.cfg"
#define DIVOOM_SLEEP_AID_RECORD_INDEX_BAK_PATH DIVOOM_BAK_SYSTEM_DIR"/sleep_aid_record_index.cfg"
#define DIVOOM_SLEEP_AID_UPDATE_TIME_BAK_PATH DIVOOM_BAK_SYSTEM_DIR"/sleep_update.cfg"

#define DIVOOM_SYSTEM_NET_FILE_PATH "AstroToo/usr/"


#define DIVOOM_NOT_SUPPORT_LANG_PATH "/usr/china.cfg"
#define DIVOOM_DEVICE_SCREEN_SNAPSHOT_PATH DIVOOM_SD_APP_PIC_DIR"/snapshot.bmp"


//下面是内部用到的数据相关信息
#define DIVOOM_THUMB_DATA_BASE_FILE_NAME DIVOOM_USER_DATA_DIR"/pic_db.bin"
#define DIVOOM_MUSIC_DATA_BASE_FILE_NAME DIVOOM_USER_DATA_DIR"/music.db"
#define DIVOOM_TF_MUSIC_DATA_BASE_FILE_NAME DIVOOM_USER_DATA_DIR"/tf_music.db"
#define DIVOOM_ALBUM_HEAD_TABLE_NAME "album_head"
#define DIVOOM_ALBUM_DATA_TABLE_NAME "album_pic"
#define DIVOOM_PIC_DATA_TABLE_NAME "pic_info"

#define DIVOOM_SYSTEM_SC_FONT_MENU_ID  30
#define DIVOOM_SYSTEM_OTHER_FONT_MENU_ID 68
#define DIVOOM_SYSTEM_TC_FONT_MENU_ID 70
#define DIVOOM_SYSTEM_JP_FONT_MENU_ID 72
#define DIVOOM_SYSTEM_TH_FONT_MENU_ID 74
#define DIVOOM_SYSTEM_KR_FONT_MENU_ID 76
#define DIVOOM_SYSTEM_FONT_MENU_ID DIVOOM_DIAL_FONT_DEFAULT_ID
#define DIVOOM_RECENT_CLOCK_DISP_TIME 1200

//#define DIVOOM_SERVER_HTTP_ADDR "http://app.divoom-gz.com/Index/getClientInfo"
//-----------------------------------------
/*局域网HTTP升级服务*/
//指引文件地址
#define DIVOOM_TEST_SERVER_HTTP_ROOT_ADDR "http://192.168.13.2:82/divoom_ota_server/product/AstroToo/"
#define DIVOOM_TEST_SERVER_HTTP_LIST_GUIDE_ADDR  DIVOOM_TEST_SERVER_HTTP_ROOT_ADDR"divoom_update_info.json"
//-----------------------------------------
#define DIVOOM_MQTT_LIB_ALIYUN_PATH	DIVOOM_SD_SYSTEM_DIR"/divoom_mqtt_lib/divoom_mqtt_aliyun_devicesecret"


#define DIVOOM_PRODUCT_TEST_WIFI_SSID		"Divoom_Ota_Server"
#define DIVOOM_PRODUCT_TEST_WIFI_PASSWD		"divoom2020"
#define DIVOOM_PRODUCT_TEST_AGING_AUDIO_PATH	DIVOOM_PRESET_ROOT "/tone/timebox-mini-d1.mp3"
#define DIVOOM_PRODUCT_TEST_AGING_AUDIO_VOLUME	57
#define DIVOOM_PRODUCT_TEST_AGING_AUDIO_MAX_SECONDS	(8 * 60 * 60)
#define DIVOOM_PRODUCT_TEST_AGING_BOOT_FLAG	DIVOOM_USER_ROOT "/.upgrade_aging_boot"
#define DIVOOM_PRODUCT_TEST_AGING_BOOT_FLAG_TMP	DIVOOM_USER_ROOT "/.upgrade_aging_boot.tmp"
#define DIVOOM_PRODUCT_DEBUG_WIFI_SSID		"Divoom_N_Ota_Server"

#define DIVOOM_LIGHT_KEY_CONTROL_PATH	DIVOOM_SD_SYSTEM_DIR"/divoom_light_key_control.cfg"
//#define DIVOOM_ALARM_MUSIC_BASE_PATH	"http://m.divoom-gz.com/source/sound/clock/"
//#define DIVOOM_SLEEP_MUSIC_BASE_PATH	"http://m.divoom-gz.com/source/sound/sleep/"
//#define DIVOOM_DIALY_MUSIC_BASE_PATH	"http://m.divoom-gz.com/source/sound/celebrations/"
#define DIVOOM_DEFAULT_FILE_IN_FLASH    "/spiffs/TimesGateFlash.bin"


#define DIVOOM_POWER_ON_MANAGE_CFG DIVOOM_SD_SYSTEM_DIR"/poweron_manage.cfg"
#define DIVOOM_BLUE_INFO_CFG DIVOOM_SD_SYSTEM_DIR"/blue_info.cfg"


/* WPA / WiFi paths: see divoom_paths.h (DIVOOM_WPA_SUPPLICANT_CONF, DIVOOM_USER_WIFI_DIR) */

#define DIVOOM_PHOTO_SLIDE_EFFECT_TIME  1500
#define DIVOOM_PHOTO_SLIDE_MIN_TIME  3000
#define DIVOOM_PHOTO_SLIDE_MAX_TIME  20000
#define DIVOOM_DOWNLOAD_FILE_MIN_RAM_SIZE  512 //最小内存下载文件的，低于这个值就不下载文件

#define DIVOOM_LVGL_TOP_MAIN_MENU_EXIT_TIME 	10
#define DIVOOM_LVGL_TOP_AMBIENT_MENU_EXIT_TIME  30
#define DIVOOM_LVGL_TOP_BRIGHT_VOLUME_MENU_EXIT_TIME  3
#define DIVOOM_LVGL_TOP_BRIGHT_VOLUME_MENU_FORCE_MUSIC  4

//退出当前正在激活的模块
#define DIVOOM_APP_EXIT_CUR_SCHEDULE			0XF2

#define DIVOOM_DEVICE_EVENT_START_COM           0XF0
#define DIVOOM_DEVICE_EVENT_TIME_INIT_OK           0XF1
#define DIVOOM_DEVICE_SYSTEM_RESET_ALL                  0XF6        //复位整个系统
#define DIVOOM_DEVICE_WIFI_CON_SRV               0XF7        //连接上服务器
#define DIVOOM_DEVICE_CHARGE_STATUS             0XF8        //USB充电状态发生变化
#define DIVOOM_DEVICE_WIFI_CON_ID               0XF9        //连接上wifi
#define DIVOOM_DEVICE_WIFI_DISCON_ID            0XFA        //断开wifi

#define DIVOOM_DEVICE_CON_SRV_ID            0XFB        //连接上服务器通知
#define DIVOOM_APP_SD_STATUS_OUT_ID         0XFC        //sd卡状态变化
#define DIVOOM_APP_SD_STATUS_IN_ID         0XFD        //sd卡状态变化
#define DIVOOM_APP_KEY_EVENT_ID         0XFE        //按键事件
#define DIVOOM_APP_CONFIG_SAVE_EVENT_ID 0Xff        //模块保存配置文件
#define DIVOOM_DEVICE_MAX_HEIGHT    128
#define DIVOOM_DEVICE_MAX_WIDTH     128

#define DIVOOM_HTTP_READ_ITEM_SIZE (12*1024)
#define DIVOOM_HTTP_READ_MAX_SIZE (512*1024)


#define DIVOOM_NET_SEND_ASYNC

#define DIVOOM_DISP_NET_CHANNEL_CONTENT_TIME 2000

enum{
    DIVOOM_DISP_TIME_FORMAT_YEAR_MONTH_DAY,
    DIVOOM_DISP_TIME_FORMAT_DAY_MONTH_YEAR,
    DIVOOM_DISP_TIME_FORMAT_MONTH_DAY_YEAR,
    DIVOOM_DISP_TIME_FORMAT_YEAR_MONTH_DAY_DOT,
    DIVOOM_DISP_TIME_FORMAT_DAY_MONTH_YEAR_DOT,
    DIVOOM_DISP_TIME_FORMAT_MONTH_DAY_YEAR_DOT,
    DIVOOM_DISP_TIME_FORMAT_YEAR_MONTH_DAY_LINE,
    DIVOOM_DISP_TIME_FORMAT_DAY_MONTH_YEAR_LINE, 
    DIVOOM_DISP_TIME_FORMAT_MONTH_DAY_YEAR_LINE, 
    DIVOOM_DISP_TIME_FORMAT_END_TYPE,
};

enum{
    DIVOOM_DISP_CUSTOM_DISIG_CLOCK_DISP_HOUR_DEC,
    DIVOOM_DISP_CUSTOM_DISIG_CLOCK_DISP_HOUR_UINT,
    DIVOOM_DISP_CUSTOM_DISIG_CLOCK_DISP_TIME_SEP,
    DIVOOM_DISP_CUSTOM_DISIG_CLOCK_DISP_MIN_DEC,
    DIVOOM_DISP_CUSTOM_DISIG_CLOCK_DISP_MIN_UNIT,
};

enum{
    DIVOOM_CHANNEL_APP_TYPE_STANDARD = 0,           //标准
    DIVOOM_CHANNEL_APP_TYPE_WEATHER = 1,            //天气
    DIVOOM_CHANNEL_APP_TYPE_TIME = 2,               //时钟
    DIVOOM_CHANNEL_APP_TYPE_WATCH = 3,              //记时器
    DIVOOM_CHANNEL_APP_TYPE_DOWN_COUNT = 4,         //倒计时
    DIVOOM_CHANNEL_APP_TYPE_SCORE_BOARD = 5,        //计分板
    DIVOOM_CHANNEL_APP_TYPE_STOCK_INFO = 6,         //股票
    DIVOOM_CHANNEL_APP_TYPE_EXCHANGE_RATE = 25,      //汇率
    DIVOOM_CHANNEL_APP_TYPE_DIVOOM_EAPERT = 8,      //DIVOOM达人订阅
    DIVOOM_CHANNEL_APP_TYPE_DIVOOM_WORK = 9,        //DIVOOM作品频道订阅
    DIVOOM_CHANNEL_APP_TYPE_NET_RADIO = 10,        //网络电台
    DIVOOM_CHANNEL_APP_TYPE_MESSAGE_BOARD = 11,        //消息提示框
    DIVOOM_CHANNEL_APP_TYPE_IFFT_BUTTON = 12,        //IFFT按键频道
    DIVOOM_CHANNEL_APP_TYPE_BLACK_SCREEN = 13,        //黑屏
    DIVOOM_CHANNEL_APP_TYPE_IEX_STOCK = 14,         //iex美国股票
    DIVOOM_CHANNEL_APP_TYPE_DIGIT_BTC = 15,         //BTC
    DIVOOM_CHANNEL_APP_TYPE_GAME_LOL = 16,         //LOL
    DIVOOM_CHANNEL_APP_TYPE_GAME_APEX = 17,         //APEX
    DIVOOM_CHANNEL_APP_TYPE_USER_DEFINE = 18,         //用户自定义表盘
    DIVOOM_CHANNEL_APP_TYPE_IEX_STOCK_LINE = 19,         //iex美国股票曲线
    DIVOOM_CHANNEL_APP_TYPE_AMAZON_MUSIC = 21,         //AMAZON_MUSIC类型
    DIVOOM_CHANNEL_APP_TYPE_COV19 = 22,         //COV19类型
    DIVOOM_CHANNEL_APP_TYPE_IEX_STOCK_SINGLE = 23,         //iex美国股票
    DIVOOM_CHANNEL_APP_TYPE_STOCK_SINGLE = 24,         //股票详细信息
    DIVOOM_CHANNEL_APP_TYPE_RSS_NEWS = 26,         //rss新闻表盘
    DIVOOM_CHANNEL_APP_TYPE_ARRAY_DATA = 27,         //获取数据列表
    DIVOOM_CHANNEL_APP_TYPE_DIVOOM_DIGIT_GIF = 28,         //DIVOOM图像数字表盘
    DIVOOM_CHANNEL_APP_TYPE_DIVOOM_COMBINE_DIAL = 29,         //DIVOOM自定义组合表盘
    DIVOOM_CHANNEL_APP_TYPE_CUP2022 = 30,         //世界杯2022
    DIVOOM_CHANNEL_APP_TYPE_DIGIT_VIRTUAL_CURRENCY = 31,         //数字虚拟货币
    DIVOOM_CHANNEL_APP_TYPE_PC_CLOCK_PARA = 32,         //由外部推数据过来
    DIVOOM_CHANNEL_APP_TYPE_USER_CLOCK_NORMAIL = 33,         //用户创建默认的数据解析规则
    DIVOOM_CHANNEL_APP_TYPE_USER_CLOCK_CUSTOME = 34,         //用户创建自己的数据解析规则
    DIVOOM_CHANNEL_APP_TYPE_SPECAIL_ALBUM = 35,             //正方形相簿
    DIVOOM_CHANNEL_APP_TYPE_SPECAIL_NO_VIDEO_ALBUM = 36,             //正方形不加视频相簿
    DIVOOM_CHANNEL_APP_TYPE_FRAME_ALBUM = 37,             //幻灯片相簿
    DIVOOM_CHANNEL_APP_TYPE_VISION = 38,                    //VISION相簿
    DIVOOM_CHANNEL_APP_TYPE_AI_ALBUM = 39,                  //ai相簿
};
enum
{
    DIVOOM_IMAGE_LOCAL_TYPE_NORMAL, 
    DIVOOM_IMAGE_LOCAL_TYPE_SAND_PAINT,
    DIVOOM_IMAGE_LOCAL_TYPE_PIC_SCAN, 
};
    

enum 
{
    DIVOOM_MODEL_MAIN = 1,          //系统管理模块
    DIVOOM_MODEL_NET_MUSIC,     //WIFI音箱音乐控制模块
    DIVOOM_MODEL_SD_MUSIC,     //SD音箱音乐控制模块
    DIVOOM_MODEL_COLLECT_MUSIC,     //收藏音乐模块
    DIVOOM_MODEL_BLUE_MUSIC,     //WIFI音箱音乐控制模块
    DIVOOM_MODEL_LINEIN,        //line in模块
    DIVOOM_MODEL_LEAVE_MSG,     //留言管理模块
    DIVOOM_MODEL_IPC,           //摄像头管理模块
    DIVOOM_MODEL_COUNT_DOWN,    //倒计时管理模块
    DIVOOM_MODEL_ALARM,         //闹钟模块
    DIVOOM_MODEL_LED_EDIT,			//led编辑器
    DIVOOM_MODEL_GIF_PICTURE,   //图像控制
    DIVOOM_MODEL_LIGHT_APP,    //缺省灯光控制
    DIVOOM_MODEL_KEY,           //按键控制模块
    DIVOOM_MODEL_SENSOR_DEVICE, //设备管理模块
    DIVOOM_MODEL_NET,          //网络模块
    DIVOOM_MODEL_SLEEP,          //睡眠模块
    DIVOOM_MODEL_TIME_PLAN,      //时间管理模块
    DIVOOM_MODEL_UART,          //串口模块
    DIVOOM_MODEL_UPDATE,          //升级模块
    DIVOOM_MODEL_WIFI, 			//WIFI管理模块
    DIVOOM_MODEL_GAME, 			//游戏管理模块
    DIVOOM_MODEL_FILE,			//文件管理模块
    DIVOOM_MODEL_LOCAL_MUSIC,	//本地音乐模块
    DIVOOM_MODEL_AUDIO_MIX,		//混音模块
    DIVOOM_MODEL_CJSON, 			//JSON模块
    DIVOOM_MODEL_LOAD_VOICE, 			//本地音乐模块
    DIVOOM_MODEL_POWER_OFF_CHARGE, 		//关机充电模块
    DIVOOM_MODEL_ECO, 		    //节能模块
    DIVOOM_MODEL_NULL, 			//空模块
    DIVOOM_MODEL_AD, 			//AD模块
    DIVOOM_MODEL_DA, 			//DA模块
    DIVOOM_MODEL_THIRD_LIB, 			//第三方库
    DIVOOM_MODEL_CYC_BUF, 			//环状缓冲区
    DIVOOM_MODEL_WEATHER_DIS, 			//显示天气模块
    DIVOOM_MODEL_ARTNET, 			//显示天气模块
	DIVOOM_MODEL_SER_CON,			//连接服务器模块
	DIVOOM_MODEL_ONOFF,				//休眠和休眠开机处理模块
	DIVOOM_MODEL_POWER_ON,			//开机处理模块
	DIVOOM_MODEL_DINGDANG_VOICE,  //叮当语音模块
	DIVOOM_MODEL_COUNT_TIME_DOWM,	//倒计时模块
	DIVOOM_MODEL_CONFIG_WIFI,	//配置WiFi
	DIVOOM_MODEL_OTA,
	DIVOOM_MODEL_NOISE,         //噪音检测模块
	DIVOOM_MODEL_PRODUCT_TEST,  //产品测试模块
    DIVOOM_MODEL_REMINDER,         //提醒模块
    DIVOOM_MODEL_UDP,
	DIVOOM_MODEL_LIGHT_CONTROL,		//按键切换灯效模块
	DIVOOM_MODEL_NET_RADIO,			//radio fm model
	DIVOOM_MODEL_VOICE_APP,			//信息瀑的音乐模块
	DIVOOM_MODEL_NET_CHANNEL,     //信息瀑的网络显示频道
	DIVOOM_MODEL_NOTICE_CHANNEL,     //信息瀑的网络通知频道
	DIVOOM_MODEL_HTTP_SERVER,		//http 服务器模块
    DIVOOM_MODEL_APP_TOOL,          //工具模块包括智能提醒，计时器，记分板，倒计时, 定时天气; 
	DIVOOM_MODEL_FONT_MANAGE,		//字体信息管理
	DIVOOM_MODEL_APP_TOMATO_TIME, //番茄时钟
	DIVOOM_MODEL_APP_WHITE_NOISE, //白噪音
	DIVOOM_MODEL_APP_MUSIC_CTRL, //音乐管理模块
	DIVOOM_MODEL_SLEEP_AID,      //睡眠辅助（助眠）模块
    DIVOOM_MODEL_CLOCK_PLAY_PLAN, //播放计划模块
    DIVOOM_MODEL_END,          //模块结束
};

enum
{
    
    DIVOOM_MUSIC_HOST_LOCAL,    //本地音乐，在LINUX系统内置
    DIVOOM_MUSIC_HOST_SD,       //本地SD音乐
    DIVOOM_MUSIC_HOST_WIFI,  //wifi音乐
    DIVOOM_MUSIC_HOST_BLUE,     //本地蓝牙音乐
    DIVOOM_MUSIC_HOST_XIMALAYA, //喜马拉雅音源
    DIVOOM_MUSIC_HOST_VTUNER,   //网络电台
    DIVOOM_MUSIC_HOST_SPOTIFY,  //spolify
    DIVOOM_MUSIC_HOST_ALEXA,    //alexa
    DIVOOM_MUSIC_HOST_IFLYTEK,  //科大讯飞
    DIVOOM_MUSIC_HOST_DIVOOM,   //DIVOOM的网络音源
    DIVOOM_MUSIC_HOST_COLLECT,   //DIVOOM的收藏音乐
	DIVOOM_MUSIC_HOST_AIRPALAY,
	DIVOOM_MUSIC_HOST_DLNA,
	DIVOOM_MUSIC_HOST_NULL,     //
}; 


#define  MAX_DELAY		(portTickType)portMAX_DELAY
#define  DIVOOM_BASE_UTC_TIME 1514779200
//这个是测试栈剩余空间的
//#define DIVOOM_TASK_PRINTF_STACK_INFO
#ifdef DIVOOM_TASK_PRINTF_STACK_INFO
#include "mqtt_client.h"
#include "divoom_mqtt_model.h"
#include "divoom_server_msg_manage.h"
#endif
#endif


//调试阶段打开这个宏，不需要配置即可
//把这个宏放在这里,为了能够让divoom_app_onoff.c也用得到
//#define DIVOOM_WIFI_DEBUG_CONFIG
