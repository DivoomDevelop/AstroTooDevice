#ifndef _divoom_bt_music_
#define _divoom_bt_music_
#ifdef DIVOOM_SIMULATOR
#include "divoom_type.h"
#include "bt_manager.h"
#else
#include "bt_manager.h"
#endif
#define MAX_STR_LEN 256              // 歌曲名/歌手/歌词最大长度（字节）
#define MAX_BT_DEVICE_COUNT 15 // 最多保存15个蓝牙设备信息

// 歌曲信息结构体（数据类型0x01，禁用字节对齐）
#pragma pack(push, 1)
typedef struct {
	btmg_avrcp_play_state_t play_status;
    char song_name[MAX_STR_LEN];  // 歌曲名（UTF-8，以\0结尾）
    char singer[MAX_STR_LEN];     // 歌手名（UTF-8，以\0结尾）
    char lyric[MAX_STR_LEN];      // 当前歌词（UTF-8，以\0结尾）
    uint32_t play_pos;            // 播放位置（毫秒）
    uint32_t song_len;            // 歌曲总长度（毫秒）
    uint8 volume;                 // 音量（0~100）
} SongInfo;

// 设备信息结构体（数据类型0x07，禁用字节对齐）
#define MAX_DEVICE_NAME_LEN 64  // 设备名称最大长度（UTF-8编码，支持中文）
typedef struct {
    char bt_addr[18];              // 蓝牙地址（格式：XX:XX:XX:XX:XX:XX，以\0结尾）
    char device_name[MAX_DEVICE_NAME_LEN];  // 设备名称（UTF-8，以\0结尾，支持中文）
} DeviceInfo;

// 蓝牙设备信息管理（用于保存多个设备）
typedef struct {
    DeviceInfo devices[MAX_BT_DEVICE_COUNT];  // 设备信息数组
    int count;  // 当前保存的设备数量
} BtDeviceInfoManager;
#pragma pack(pop)

typedef void (*divoom_bt_lrc_callback)(void *para, char *singer,char *title, char *artist, int play_time);

/** lrc 回调来源（divoom_bt_music_consume_lrc_notify_kind 读取后清零） */
#define DIVOOM_BT_LRC_NOTIFY_PLAYBACK    0
#define DIVOOM_BT_LRC_NOTIFY_UART_LYRIC  1  /* 0x04 type=1 逐句歌词 */
#define DIVOOM_BT_LRC_NOTIFY_UART_TITLE  2  /* 0x04 type=2 歌名/歌手 */

void divoom_bt_music_lrc_set_callback(divoom_bt_lrc_callback callback,void *para);
/** 取本次 lrc 回调来源并清零 */
int divoom_bt_music_consume_lrc_notify_kind(void);

// 蓝牙播放请求回调类型
// 当手机端开始播放音乐时，蓝牙进程会发送通知，触发此回调
// 上层应在回调中切换到蓝牙模式
typedef void (*DIVOOM_BT_PLAY_REQUEST_CB)(void);

/** 蓝牙进程回传「本机当前蓝牙名称」时回调（UTF-8，含中文） */
typedef void (*DIVOOM_BT_LOCAL_ADAPTER_NAME_CB)(const char *name_utf8);

#ifdef DIVOOM_TIMES_VIBE
int divoom_bt_music_parse_device_info(const uint8_t *raw, size_t raw_len);
int divoom_bt_music_parse_connected_phone_info(const uint8_t *raw, size_t raw_len);
int divoom_bt_music_fetch_connected_phone_info_from_jl(void);

#endif

void divoom_bt_music_set_play_vol_to_device(uint8 value);

void divoom_bt_music_set_title_lyric(uint8 *str, uint8 flag);

void divoom_bt_music_handle_play_ctrl(uint8 ctrl);

void divoom_bt_music_handle_play_vol(uint8 vol);

/* JL 转发蓝牙音乐时间：type 1=总时长，2=当前位置；time_sec 单位秒 */
void divoom_bt_music_handle_music_time(uint8 type, uint32 time_sec);

/*********************************************************************
  Function:        divoom_bt_music_get_device_list
  Description:     获取保存的蓝牙设备信息列表
  Input:           count - 输出参数，返回设备数量
  Return:          返回设备信息数组指针，失败返回NULL
                   注意：返回的指针指向全局数组，不需要释放
**********************************************************************/
const DeviceInfo* divoom_bt_music_get_device_list(int *count);

/*********************************************************************
  Function:        divoom_bt_music_connect_device
  Description:     连接指定的蓝牙设备，并将设备信息发送给进程A
  Input:           bt_addr - 蓝牙地址（格式：XX:XX:XX:XX:XX:XX）
  Return:          0=成功, -1=失败
**********************************************************************/
int divoom_bt_music_connect_device(const char *bt_addr);

/*********************************************************************
  Function:        divoom_bt_music_unpair_device
  Description:     取消配对指定的蓝牙设备
  Input:           bt_addr - 蓝牙地址（格式：XX:XX:XX:XX:XX:XX）
  Return:          0=成功, -1=失败
**********************************************************************/
int divoom_bt_music_unpair_device(const char *bt_addr);

/*********************************************************************
  Function:        divoom_bt_music_unpair_all_devices
  Description:     取消全部已配对设备。杰理删除全部链路记录，本机列表一并清空。
  Return:          0=成功, -1=失败
**********************************************************************/
int divoom_bt_music_unpair_all_devices(void);

/*********************************************************************
  Function:        divoom_bt_music_disable_boot_connect
  Description:     设置是否禁用开机自动重连
  Input:           disable - 1=禁用开机自动重连，0=启用开机自动重连
  Return:          0=成功, -1=失败
**********************************************************************/
int divoom_bt_music_disable_boot_connect(int disable);

/*********************************************************************
  Function:        divoom_bt_music_get_boot_connect_flag_from_file
  Description:     获取是否禁用开机自动重连
  Input:           void
  Return:          0=启用开机自动重连, 1=禁用开机自动重连
**********************************************************************/
int divoom_bt_music_get_boot_connect_flag_from_file(void);

int divoom_bt_music_request_bt_auto_connect_info(void);

/*********************************************************************
  Function:        divoom_bt_music_get_current_device_info
  Description:     获取当前连接上的设备信息
  Input:           none
  Return:          返回设备信息数组指针，失败返回NULL
                   注意：返回的指针指向全局数组，不需要释放
**********************************************************************/
const DeviceInfo* divoom_bt_music_get_current_device_info(void);

/** 是否已连接手机（仅 A2DP CONNECTED 时返回 1），供 UI 显示；切蓝牙模式不依赖此状态 */
int divoom_bt_music_is_connected(void);

/** 手机 AVRCP 是否正在播。切到蓝牙不会自动起播，playing 图标跟这个，不要沿用 TF/UAC 的 1 */
int divoom_bt_music_is_a2dp_playing(void);

// 初始化蓝牙音乐模块
void divoom_bt_music_init(void);
void divoom_bt_music_play_deferred_connect_tone_if_any(void);

// 反初始化蓝牙音乐模块
void divoom_bt_music_deinit(void);

/*
BTMG_AVRCP_PLAY = 0,
BTMG_AVRCP_PAUSE,
BTMG_AVRCP_STOP,
BTMG_AVRCP_FASTFORWARD,
BTMG_AVRCP_REWIND,
BTMG_AVRCP_FORWARD,
BTMG_AVRCP_BACKWARD,
BTMG_AVRCP_VOL_UP,
BTMG_AVRCP_VOL_DOWN,
*/

// 设置播放状态
void divoom_bt_music_set_play_status(uint8 status);

/*
    0xff 增加音量 
    0xfe 减小音量
    0~100是设定指定音量
*/
void divoom_bt_music_set_play_vol(uint8 value);

/* 本机下发音量后调用；1.2s 内忽略手机/JL 回传，避免 OSD 对打。old_vol 保留兼容，未使用。 */
void divoom_bt_music_mark_sent_volume(int old_vol, int vol);
uint8 divoom_bt_music_get_play_vol(void);
/* 返回模块内部的只读歌曲信息缓冲区，调用方不得释放或长期持有。 */
const char *divoom_bt_music_get_song_name(void);
const char *divoom_bt_music_get_song_singer(void);
const char *divoom_bt_music_get_song_lyric(void);
const char *divoom_bt_music_get_song_prev_lyric(void);
void divoom_bt_music_set_play_pre(void);
void divoom_bt_music_set_play_next(void);
void divoom_bt_music_handle_connect_status(uint8 status);


/*********************************************************************
  Function:        divoom_bt_music_set_audio_enable
  Description:     设置蓝牙音频输出使能
                   当切换到非蓝牙模式时，禁用蓝牙音频输出
                   即使手机端播放，蓝牙管理进程也不会输出音频
  Input:           enable  1:启用音频输出  0:禁用音频输出
  Return:          none
**********************************************************************/
void divoom_bt_music_set_audio_enable(uint8 enable);

/*********************************************************************
  Function:        divoom_bt_music_register_play_request_cb
  Description:     注册蓝牙播放请求回调
                   当手机端开始播放时，会触发此回调
                   上层应在回调中切换到蓝牙模式
  Input:           cb - 回调函数
  Return:          none
**********************************************************************/
void divoom_bt_music_register_play_request_cb(DIVOOM_BT_PLAY_REQUEST_CB cb);

/*********************************************************************
  Function:        divoom_bt_music_exit_bt_system
  Description:     发送完全退出蓝牙系统命令到蓝牙进程
                   蓝牙进程接收到此命令后会完全退出（包括经典蓝牙和BLE）
  Input:           NONE
  Return:          0=成功, -1=失败
**********************************************************************/
int divoom_bt_music_exit_bt_system(void);

/*********************************************************************
  Function:        divoom_bt_music_start_classic_bt
  Description:     发送开启经典蓝牙命令到蓝牙进程
                   蓝牙进程接收到此命令后会启动经典蓝牙
                   如果蓝牙系统未初始化，会先重新初始化整个蓝牙系统
  Input:           NONE
  Return:          0=成功, -1=失败
**********************************************************************/
int divoom_bt_music_start_classic_bt(void);

/*********************************************************************
  Function:        divoom_bt_music_disconnect_bt
  Description:     发送断开当前蓝牙连接（仅断开与手机的 A2DP 连接，可再次连接）
  Return:          0=成功, -1=失败
**********************************************************************/
int divoom_bt_music_disconnect_bt(void);

/*********************************************************************
  Function:        divoom_bt_music_disable_classic_bt
  Description:     发送关闭经典蓝牙命令（关闭经典蓝牙；BLE 仍可保留）
                   与 divoom_bt_music_exit_bt_system（完全退出含 BLE）不同
  Return:          0=成功, -1=失败
**********************************************************************/
int divoom_bt_music_disable_classic_bt(void);

/*********************************************************************
  Function:        divoom_bt_music_get_pair_pin
  Description:     获取蓝牙配对密码配置（直接读取配置文件，与蓝牙进程共用）
  Input:           enable - 输出，1=已启用，0=未启用；可为 NULL
                  pin4   - 输出，4位密码字符串，至少5字节；可为 NULL
                  maxlen - pin4 缓冲区长度
  Return:          0=成功, -1=文件不存在或读取失败
**********************************************************************/
int divoom_bt_music_get_pair_pin(int *enable, char *pin4, int maxlen);

/*********************************************************************
  Function:        divoom_bt_music_set_pair_pin
  Description:     设置蓝牙配对密码（发送到蓝牙进程并保存）
                   密码设置完成后，后续配对时对端需输入相同4位PIN才能配对成功
  Input:           enable - 1=启用蓝牙密码，0=关闭蓝牙密码
                  pin4   - 4位数字密码字符串，如 "1234"
  Return:          0=成功, -1=失败
**********************************************************************/
int divoom_bt_music_set_pair_pin(int enable, const char *pin4);

/*********************************************************************
  Function:        divoom_bt_music_set_local_adapter_name
  Description:     向蓝牙进程设置本机蓝牙可发现名称（UTF-8，支持中文），写入协议栈显示名
  Input:           name_utf8 - 以 NUL 结尾的 UTF-8 字符串，最长 MAX_DEVICE_NAME_LEN-1 字节
  Return:          0=成功, -1=失败
**********************************************************************/
int divoom_bt_music_set_local_adapter_name(const char *name_utf8);

/*********************************************************************
  Function:        divoom_bt_music_set_jl_adapter_name
  Description:     通过 BtCore UART(DM_BTCORE_BT_INFO) 设置 JL 本机蓝牙名
  Input:           name_utf8 - App 下发原名（内部 sanitize 至约 26 字节）
  Return:          0=成功, -1=失败
**********************************************************************/
int divoom_bt_music_set_jl_adapter_name(const char *name_utf8);

/*********************************************************************
  Function:        divoom_bt_music_get_local_adapter_name
  Description:     获取最近一次由蓝牙进程同步的本机蓝牙名称（手机连上后由对端推送）
  Return:          有效 UTF-8 字符串指针；未同步过可能为空串
**********************************************************************/
const char *divoom_bt_music_get_local_adapter_name(void);

#endif
