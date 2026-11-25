#include "divoom.h"

#ifdef DIVOOM_TIMES_VIBE
#ifndef DIVOOM_BTCORE_UART_CMD_H
#define DIVOOM_BTCORE_UART_CMD_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * BtCore UART 命令 ID（JL7012 <-> F101）。
 * 终端联调：DIVOOM_BTCORE_UART_CLI + btcore list；文档 tools/btcore_cli/README.md
 * 新增命令：在枚举末尾 DM_BTCORE_GET_BOOT_MODE 之后的注释块上方插入，并同步 btcore list。
 */
typedef enum divoom_btcore_cmd_id {
	/* JL↔T113：SPP/BLE 连接状态，2 字节 type(0=BLE,1=SPP) + flag(1=连接,0=断开) */
	DM_BTCORE_SPPBLE_CONNECT_INFO = 0x01,
	/* JL↔T113：App 数据透传，载荷 type(1B) + len(2B LE) + data(len)；配网用 type=0 */
	DM_BTCORE_APP_DATA_TRANSMIT = 0x02,
	DM_BTCORE_ANCS_DATA_TRANSMIT = 0x03,
	DM_BTCORE_LYRIC_DATA_TRANSMIT = 0x04,
	DM_BTCORE_BT_INFO = 0x05,
	DM_BTCORE_SD_INSERT_STATUS = 0x06,
	DM_BTCORE_SD_PLAY_INFO = 0x07,
	DM_BTCORE_SD_TOTAL_MUSIC = 0x08,
	DM_BTCORE_SD_MUSIC_LIST_ITEM = 0x09,
	DM_BTCORE_SD_MUSIC_LIST = 0x0a,
	DM_BTCORE_SD_MUSIC_PLAYED_NAME = 0x0b,
	DM_BTCORE_SD_MUSIC_POSITION = 0x0c,
	DM_BTCORE_SD_MUSIC_PLAY_MODE = 0x0d,
	DM_BTCORE_SD_MUSIC_PLAYED_ID = 0x0e,
	DM_BTCORE_SD_MUSIC_VOLUME = 0x0f,
	DM_BTCORE_SD_MUSIC_STATUS = 0x10,
	DM_BTCORE_SD_MUSIC_LAST_NEXT = 0x11,
	DM_BTCORE_SERVICE_A2DP_STATUS = 0x12,  	 /* JL→T113 通知 / T113→JL 查询 A2DP 状态 */
	DM_BTCORE_WIRELESS_MIC_STATUS = 0x13,
	DM_BTCORE_KARAOKE_WORK_CTRL = 0x14,
	DM_BTCORE_BT_AUTO_CONNECT = 0x15,
	DM_BTCORE_SAVE_MUSIC_VOL = 0x16,
	DM_BTCORE_FILE_HEAD_TRANSMIT = 0x17,
	DM_BTCORE_FILE_BLOCK_TRANSMIT = 0x18,
	DM_BTCORE_SYSTEM_POWER_OFF = 0x19,
	DM_BTCORE_SYSTEM_RESET = 0x1a,
	DM_BTCORE_CLEAR_BLUE_ADDR = 0x1b,
	DM_BTCORE_GET_BLUE_ADDR = 0x1c,
	DM_BTCORE_UNMUTE = 0x1d,
	DM_BTCORE_UNMUTE_MSG = 0x1e,
	DM_BTCORE_AUDIO_APP_SOURCE_MUTE = 0x1f,
	DM_BTCORE_RECORD = 0x20,
	DM_BTCORE_GET_KARAOKE_REC_STATUS = 0x21,
	DM_BTCORE_KARAOKE_PLAYBACK_STATUS = 0x22,
	DM_BTCORE_KARAOKE_RECORD = 0x23,
	DM_BTCORE_KARAOKE_PLAYBACK = 0x24,
	DM_BTCORE_PLAY_TONE = 0x25,
	DM_BTCORE_BT_CONNECT_TONE = 0x26,
	DM_BTCORE_GET_TONE_PLAY_STATUS = 0x27,
	DM_BTCORE_GET_TONE_PLAYED_ID = 0x28,
	DM_BTCORE_CANCEL_PLAY_TONE = 0x29,
	DM_BTCORE_TONE_VOLUME = 0x2a,
	DM_BTCORE_CHECK_VOICE_READY = 0x2b,
	DM_BTCORE_SET_PLAY_VOLUME = 0x2c,
	DM_BTCORE_APP_NEED_SD_MUSIC_UPDATE = 0x2d,
	DM_BTCORE_PLAY_BLUE_MUSIC = 0x2e,
	DM_BTCORE_MUSIC_LAST_NEXT  = 0x2f,
	DM_BTCORE_PAUSE_RESUME_MUSIC = 0x30,
	DM_BTCORE_MUSIC_VOLUME = 0x31,
	DM_BTCORE_MUSIC_STATUS = 0x32,
	DM_BTCORE_REMIND_BAT_LOW_POWER = 0x33,
	DM_BTCORE_BAT_VOLT_AVR = 0x34,
	DM_BTCORE_USB_IN_OUT_STA = 0x35,
	DM_BTCORE_SYSTEM_TIME = 0x36,
	DM_BTCORE_CHECK_ALARM_ARRIVE = 0x37,
	DM_BTCORE_SET_ALARM_TIME = 0x38,
	DM_BTCORE_WORK_MODE = 0x39,
	DM_BTCORE_BT_CONNECT_CTRL = 0x3a,
	DM_BTCORE_GET_FFT_FREQ_DATA = 0x3b,
	DM_BTCORE_SET_VOCAL_REMOVE = 0x3c,
	DM_BTCORE_FILE_MISSED_PACKETS = 0x3d,
	DM_BTCORE_FILE_END = 0x3e,
	DM_BTCORE_MIC_MUSIC_STATUS = 0x3f,
	DM_BTCORE_FM_STATUS = 0x40,
	DM_BTCORE_CHECK_LINK = 0x41,
	DM_BTCORE_SONGLIST_INFO = 0x42,
	DM_BTCORE_DISP_WORK_MODE = 0x43,
	DM_BTCORE_TIMESTAMP = 0x44,
	DM_BTCORE_PCM_INFO = 0x45,
	DM_BTCORE_PCM_TRANS = 0x46,
	DM_BTCORE_PCM_STEAM_CTRL = 0x47,
	DM_BTCORE_UAC_MUSIC_LAST_NEXT = 0x48,
	DM_BTCORE_BT_CONNECT_STATUS = 0x49,
	DM_BTCORE_BLE_DISCONNECT = 0x4a,
	DM_BTCORE_A2DP_CTRL = 0x4b,
	/* JL→T113：按键事件，cmd_data[0]=id(键值) cmd_data[1]=action，见 divoom_btcore_key_action_t */
	DM_BTCORE_SEND_KEY_SYSTEM_MSG = 0x4c,
	DM_BTCORE_SCAN_MUSIC_LIST = 0x4d, /* 双向：F101→JL 空载荷请求；JL→F101 1B scan_status */
	/* JL→T113：Line-in 插拔状态，cmd_data 1 字节 insert_status */
	DM_BTCORE_LINEIN_INSERT_STATUS = 0x4e,
	/* JL→T113：UAC 插拔状态，cmd_data 1 字节 insert_status */
	DM_BTCORE_UAC_INSERT_STATUS = 0x4f,
	/* 0x50：T113 GET 无载荷 → JL 应答 version 4B LE；JL 主动上报 version 同格式；
	 *      JL 手机端音量同步为 1 字节 level，按 cmd_len 区分 */
	DM_BTCORE_DEVICE_VERSION = 0x50,
	DM_BTCORE_SYNC_BLUE_VOLUME_BY_PHONE = 0x50,
	DM_BTCORE_BT_CONNECTED_PHONE_INFO = 0x51, /* JL→T113：已连接手机蓝牙信息（名称+地址） */
	DM_BTCORE_BT_UNPAIR_DEVICE = 0x52,
	DM_BTCORE_BT_CONNECT_ONE_DEVICE = 0x53,
	DM_BTCORE_SD_MUSIC_NOTIFY_FINISHED = 0x54,
	DM_BTCORE_BAT_VOLT_LEVE = 0x55,
	DM_BTCORE_GET_BOOT_SOURCE = 0x56,
	/* JL→T113：蓝牙音乐时间，type 1=总时间 2=当前播放时间，data 4 字节小端，单位秒 */
	DM_BTCORE_BT_MUSIC_TIME = 0x57,
	DM_BTCORE_BT_BIP_DATA = 0x58,
	DM_BTCORE_OTA_START = 0x59,
	DM_BTCORE_OTA_DATA = 0x5a,
	DM_BTCORE_OTA_END = 0x5b,
	/* JL→T113：SD 卡音乐已就绪（无载荷）；切 TF 模式后收到此命令再发 set_music_id(0x0e) */
	DM_BTCORE_SD_MUSIC_READY = 0x5e,
	/* T113→JL：AIC 主机音频通路已就绪（空载荷）；未收到前 JL 禁止 BT/TF 等 DAC 出声 */
	DM_BTCORE_MAIN_SOC_BOOT_DONE = 0x5f,
	/* T113→JL：BLE 广播开关，data[0] 0=关闭，1=开启 */
	DM_BTCORE_BLE_CTRL = 0x60,
	/* T113→JL：查询开机模式，空载荷；JL 应答 boot_mode 1 字节 */
	DM_BTCORE_GET_BOOT_MODE = 0x61,
	/* AstroToo：T113→JL 切工作模式并带音量（timesvibe 仍用 0x39，不改） */
	DM_BTCORE_WORK_MODE_SET_VOL = 0x62,
	/* JL→F101：MIC 录音 MP3 数据块（简化协议，无 INFO/CTRL/PATCH） */
	DM_BTCORE_REC_STREAM_DATA = 0x63,
	/* JL→F101：MIC 录音结束摘要 */
	DM_BTCORE_REC_STREAM_END = 0x64,
	/* AstroToo：噪音 dB 推流（JL flowtoo 算法）；F101→JL action 0/1，JL→F101 1B db@100ms */
	DM_BTCORE_MIC_NOISE_METER = 0x65,
	/* AstroToo：JL 按键 WS2812 灯效；set [1,mode,on_off]；get [0]→rsp [mode,on_off] */
	DM_BTCORE_KEY_LIGHT_CTRL = 0x66,
	/* JL→F101：通知 F101 进入 USB boot（efex），空载荷，F101 无需应答 */
	DM_BTCORE_ENTER_BOOT_MODE = 0x67,
	/* 对箱快照 5 字节：state, role, flags, remain_sec, fail_reason。
	 * 0x68 应答与 0x6B 推送布局相同。F101 写入 divoom_tws_host 的同一份快照。 */
	DM_BTCORE_TWS_STATUS = 0x68,           /* 查询对箱快照：空载荷，JL 回 5 字节 */
	DM_BTCORE_TWS_PAIR_CTRL = 0x69,        /* 对箱命令：2 字节 [功能, 参数]，JL 先回同一份快照 */
	DM_BTCORE_TWS_INFO = 0x6A,             /* 获取 TWS 详细信息 */
	DM_BTCORE_TWS_EVENT = 0x6B,            /* JL 推送对箱快照（搜索中每秒一次） */
	DM_BTCORE_MEDIA_VOLUME = 0x6C,         /* Vol_Media: op0 get level→rsp; op1 set [level]; op2 set mute [1=mute,0=unmute]; op3 get mute→rsp */
	DM_BTCORE_SOUND_EFFECT_MODE = 0x6D,
	DM_BTCORE_CALL_VOLUME = 0x6E,          /* VolCallGate: op0 get level→rsp; op1 set [level]; op2 set mute [1=mute,0=unmute]; op3 get mute→rsp */
	DM_BTCORE_HFP_CALL_VOLUME = 0x6F,      /* HFP 通话增益 0～15：F101 set [1,level]；get [0]→rsp 1B；JL 主动推 1B */
	/* JL→F101：当前曲元数据（切歌推一包，不跟进度刷）；勿写入蓝牙歌词 0x04 */
	DM_BTCORE_MUSIC_TAG_INFO = 0x70,
	/* F101→JL：IISIN EQ（JL EQ_IISIN / user_eq_iisin），档位 0～16 */
	DM_BTCORE_IISIN_EQ = 0x71,
	/* JL→F101：HFP 通话 UI 推送（来电/去电/接通/挂断）；v1 F101 仅处理来电 alert */
	DM_BTCORE_PHONE_CALL_ALERT = 0x72,
	/* timesvibe HOGP 遥控（0x73～0x76 已占用）；AstroToo 先占位，暂不处理 */
	DM_BTCORE_REMOTE_STATUS = 0x73,        /* HOGP 槽：查询 op；推送 8B conn+saved+addr */
	DM_BTCORE_REMOTE_KEY = 0x74,           /* HOGP 按键，仅 JL→F101，5B */
	DM_BTCORE_REMOTE_BATTERY = 0x75,       /* HOGP 电量，仅 JL→F101，7B */
	DM_BTCORE_REMOTE_INFO = 0x76,          /* HOGP 档案：查询空；推送变长，断开全 0 */
	/* JL→F101：插卡升杰理 dual-bank 进度，1 字节 percent 0～100，单向无应答 */
	DM_BTCORE_TF_OTA_PROGRESS = 0x77,
	/* ↑ 新增 DM_BTCORE_* 请插在上一行之后（cmd id 递增）。
	 * 然后同步 btcore 调试表（二选一）：
	 *   · Cursor 聊天：同步 btcore list
	 *   · 终端：python3 tools/btcore_cli/gen_btcore_cli_cmds.py --sync-meta
	 * 详见 tools/btcore_cli/README.md */
} divoom_btcore_cmd_id_t;

#define DIVOOM_BTCORE_MEDIA_VOL_OP_GET_LEVEL   0
#define DIVOOM_BTCORE_MEDIA_VOL_OP_SET_LEVEL   1
#define DIVOOM_BTCORE_MEDIA_VOL_OP_SET_MUTE    2
#define DIVOOM_BTCORE_MEDIA_VOL_OP_GET_MUTE    3

#define DIVOOM_BTCORE_CALL_VOL_OP_GET_LEVEL    DIVOOM_BTCORE_MEDIA_VOL_OP_GET_LEVEL
#define DIVOOM_BTCORE_CALL_VOL_OP_SET_LEVEL    DIVOOM_BTCORE_MEDIA_VOL_OP_SET_LEVEL
#define DIVOOM_BTCORE_CALL_VOL_OP_SET_MUTE     DIVOOM_BTCORE_MEDIA_VOL_OP_SET_MUTE
#define DIVOOM_BTCORE_CALL_VOL_OP_GET_MUTE     DIVOOM_BTCORE_MEDIA_VOL_OP_GET_MUTE

#define DIVOOM_BTCORE_HFP_CALL_VOL_OP_GET      0u
#define DIVOOM_BTCORE_HFP_CALL_VOL_OP_SET      1u
#define DIVOOM_BTCORE_HFP_CALL_VOL_MAX        15u

#define DIVOOM_BTCORE_IISIN_EQ_OP_GET         0u
#define DIVOOM_BTCORE_IISIN_EQ_OP_SET         1u
#define DIVOOM_BTCORE_IISIN_EQ_LEVEL_MIN      0u
#define DIVOOM_BTCORE_IISIN_EQ_LEVEL_MAX     16u

/* DM_BTCORE_PHONE_CALL_ALERT（0x72）JL→F101，单向无应答 */
#define DIVOOM_BTCORE_PHONE_ALERT_ACTION_DISMISS   0u
#define DIVOOM_BTCORE_PHONE_ALERT_ACTION_SHOW      1u
#define DIVOOM_BTCORE_PHONE_ALERT_CALL_INCOMING    0u
#define DIVOOM_BTCORE_PHONE_ALERT_CALL_OUTGOING    1u
#define DIVOOM_BTCORE_PHONE_ALERT_CALL_ACTIVE      2u
#define DIVOOM_BTCORE_PHONE_ALERT_FLAG_HAS_NUMBER  0x01u
#define DIVOOM_BTCORE_PHONE_ALERT_FLAG_HAS_NAME    0x02u
#define DIVOOM_BTCORE_PHONE_ALERT_FLAG_INBAND_RING 0x04u
#define DIVOOM_BTCORE_PHONE_ALERT_NUMBER_MAX       32u
#define DIVOOM_BTCORE_PHONE_ALERT_NAME_MAX         64u
#define DIVOOM_BTCORE_PHONE_ALERT_SHOW_HDR_LEN     3u

/* DM_BTCORE_TWS_STATUS / TWS_EVENT 快照，小端，固定 5 字节 */
#define DIVOOM_BTCORE_TWS_SNAP_LEN             5u
#define DIVOOM_BTCORE_TWS_ST_UNPAIRED          0u
#define DIVOOM_BTCORE_TWS_ST_LISTEN            1u
#define DIVOOM_BTCORE_TWS_ST_SEARCH            2u
#define DIVOOM_BTCORE_TWS_ST_CONNECTED         3u
#define DIVOOM_BTCORE_TWS_ST_RECONNECT         4u
#define DIVOOM_BTCORE_TWS_ROLE_NONE            0u
#define DIVOOM_BTCORE_TWS_ROLE_LEFT            1u
#define DIVOOM_BTCORE_TWS_ROLE_RIGHT           2u
#define DIVOOM_BTCORE_TWS_F_PHONE              0x01u
#define DIVOOM_BTCORE_TWS_F_CALL               0x02u
#define DIVOOM_BTCORE_TWS_F_PAIRED             0x04u
#define DIVOOM_BTCORE_TWS_F_POWER_SYNC         0x08u
#define DIVOOM_BTCORE_TWS_FAIL_NONE            0u
#define DIVOOM_BTCORE_TWS_FAIL_NOT_FOUND       1u
#define DIVOOM_BTCORE_TWS_FAIL_PHONE_WAIT      2u
#define DIVOOM_BTCORE_TWS_FAIL_IN_CALL         3u
/* 0x69 固定 2 字节：功能 + 参数。1 等待，2 配对，3 解除，4 同步关机。
 * 4 的参数是 0 关、1 开。有配对记录时杰理自己回连。 */
#define DIVOOM_BTCORE_TWS_FUNC_LISTEN          1u
#define DIVOOM_BTCORE_TWS_FUNC_PAIR            2u
#define DIVOOM_BTCORE_TWS_FUNC_UNPAIR          3u
#define DIVOOM_BTCORE_TWS_FUNC_POWER_SYNC      4u
#define DIVOOM_BTCORE_TWS_ARG_OFF              0u
#define DIVOOM_BTCORE_TWS_ARG_ON               1u

/* JL 0x39 work_mode：0=蓝牙，8=通话（DIVOOM_WORK_MODE_PHONE），9=SHOW，13=FILE_UPDATE */
#define DIVOOM_BTCORE_WORK_MODE_PHONE         8u
#define DIVOOM_BTCORE_WORK_MODE_SHOW          9u
#define DIVOOM_BTCORE_WORK_MODE_FILE_UPDATE  13u

typedef enum {
	DIVOOM_BTCORE_KEY_ACTION_CLICK = 0,
	DIVOOM_BTCORE_KEY_ACTION_LONG = 1,
	DIVOOM_BTCORE_KEY_ACTION_HOLD = 2,
	DIVOOM_BTCORE_KEY_ACTION_UP = 3,
	DIVOOM_BTCORE_KEY_ACTION_DOUBLE_CLICK = 4,
} divoom_btcore_key_action_t;

/*
 * 应用层对 BtCore 串口的封装：在 btcore_uart 之上做命令分发，并提供 divoom 命名发送接口。
 * 初始化前需保证底层 btcore_uart 未被其它模块单独占用同一 tty。
 */

typedef struct divoom_btcore_uart_cmd_init_param {
	const char *device;   /* NULL：使用默认 /dev/ttyS2 */
	int         baudrate; /* 0：使用默认 115200 */
} divoom_btcore_uart_cmd_init_param_t;

typedef void (*divoom_btcore_uart_cmd_user_fn)(uint8_t cmd, const uint8_t *cmd_data,
					       uint16_t cmd_len, void *user);

int divoom_btcore_uart_cmd_init(const divoom_btcore_uart_cmd_init_param_t *param);

void divoom_btcore_uart_cmd_deinit(void);

int divoom_btcore_uart_cmd_is_ready(void);

/* T113 已发 DM_BTCORE_SYSTEM_POWER_OFF 后为 1，UART RX 不再分发给业务 */
int divoom_btcore_uart_cmd_is_rx_suppressed(void);

/*
 * RTOS 解析线程：send_wait 同步应答在入队前消费，避免排在慢业务之后超时。
 * 返回 1 表示已作为 sync reply 处理，调用方勿再入队。
 */
int divoom_btcore_uart_cmd_try_take_sync_reply(uint8_t cmd, const uint8_t *cmd_data, uint16_t cmd_len);

/*
 * 命令执行上下文。哪个命令在哪跑由 app 层单点声明（divoom_btcore_uart_cmd_ctx），
 * middle 的接收线程只查表投递，不再各自硬编码一份 cmd 列表。
 */
typedef enum {
	/*
	 * 在 btcore_rx 线程内同步执行，不过队列，省一次入队与一次线程切换。
	 * 契约（新增命令必须逐条满足）：不等锁、不碰文件与 DB、不调 LVGL、不打印、
	 * 不分配动态内存，只做定长内存写入。违反其一就该归 CTX_WORK 或 CTX_SLOW。
	 */
	DIVOOM_BTCORE_CTX_FAST = 0,
	/* 投递到录音流 worker */
	DIVOOM_BTCORE_CTX_STREAM,
	/* 投递到 btcore_work：业务与 UI 状态机 */
	DIVOOM_BTCORE_CTX_WORK,
	/* 投递到 btcore_slow：文件 IO 与 DB，避免堵住轻量业务 */
	DIVOOM_BTCORE_CTX_SLOW,
	/* 协议上无需处理，直接丢弃 */
	DIVOOM_BTCORE_CTX_DROP,
} divoom_btcore_ctx_t;

divoom_btcore_ctx_t divoom_btcore_uart_cmd_ctx(uint8_t cmd);

/*
 * CTX_FAST 的实现体：FFT/噪音/电压/电量/TF 进度等，UI 只读最新值。
 * 由接收线程在查到 CTX_FAST 时同步调用。
 */
void divoom_btcore_uart_cmd_rx_handle_inline(uint8_t cmd, const uint8_t *cmd_data, uint16_t cmd_len);

int divoom_btcore_uart_cmd_send(uint8_t cmd, const uint8_t *cmd_data, uint16_t cmd_len);

/*
 * 发送并等待同 cmd 的应答帧；应答载荷写入 reply_out（最多 reply_out_cap），reply_len_out 填对端载荷长度。
 * 若应答载荷大于 BTCORE_MAX_CMD_DATA_LEN（4096），仅前 4096 字节保存在内部缓冲，reply_len_out 仍为对端声明长度。
 * send_wait 默认 timeout_s=1；OTA 等慢命令请用 send_wait_timeout（见 DIVOOM_BTCORE_OTA_*_TIMEOUT_S）。
 * reply_out / reply_len_out 可为 NULL（只等应答不取数据）。
 * 返回 0 成功；-1 超时或未初始化；其它为 btcore_uart_send 错误码。
 */
int divoom_btcore_uart_cmd_send_wait(uint8_t cmd, const uint8_t *cmd_data, uint16_t cmd_len,
				     uint8_t *reply_out, uint16_t reply_out_cap, uint16_t *reply_len_out);

int divoom_btcore_uart_cmd_send_wait_timeout(uint8_t cmd, const uint8_t *cmd_data, uint16_t cmd_len,
					     uint8_t *reply_out, uint16_t reply_out_cap,
					     uint16_t *reply_len_out, int timeout_s);

/* FFT 频谱：JL 经 DM_BTCORE_GET_FFT_FREQ_DATA 周期推送，收包后交给 divoom_spectrum 保存 */
/* 已废弃：JL 正常运行后每 20ms 主动推 0x3b，F101 不再下发订阅 */
void divoom_btcore_uart_cmd_set_fft_freq_stream(uint8 enable);

/*
 * DM_BTCORE_MIC_NOISE_METER（0x65）AstroToo 噪音检测：
 *   JL→F101：db 1 字节 0~99，约 100ms 周期（JL 正常运行后主动推，F101 不下发）
 */
#define DIVOOM_BTCORE_MIC_NOISE_UART_SILENCE_MS 500

/* 已废弃：F101 不再下发 0x65 订阅 */
void divoom_btcore_uart_cmd_set_noise_meter_stream(uint8 enable);

/* 500ms 内有新包时返回 db，否则返回 0 */
int divoom_btcore_uart_cmd_get_mic_noise_db(void);

/*
 * DM_BTCORE_KEY_LIGHT_CTRL（0x66）AstroToo 按键 RGB 灯效（JL WS2812 ×3）：
 *   F101→JL set：3 字节 [1, mode, on_off] 不改亮度；4 字节 [1, mode, on_off, bright]
 *   F101→JL get：payload [0]；JL 应答 3 字节 [mode, on_off, bright]
 *   mode 合法 0~7 或产测 0x80~0x83；熄屏策略 0x87 SCREEN_OFF（不进用户配置）
 *     0 风车(FFT)  1 淡进淡出  2 流动风车(FFT)  3 星星闪烁
 *     4 彩虹  5 快跑  6 萤火虫  7 纯白常亮
 *     0x80 产测白  0x81 产测红  0x82 产测绿  0x83 产测蓝（与 LCD 屏测同步）
 *     0x87 F101 熄屏：杰理空闲黑灯，插线沿走关机充电环
 *   bright：0~100 全局亮度，默认 100；3 字节设置时不改 JL 侧亮度
 *   开机默认（system_config.cfg）：mode=0，on_off=1，bright=100；由 F101 load 后 set_full 下发
 *   FFT 调速仅 mode 0/2，且 JL 侧有音乐 PCM 播放时生效
 *   高层 API：divoom_btcore_key_light.h
 */

/*
 * DM_BTCORE_SOUND_EFFECT_MODE（0x6D）AstroToo 音效场景：
 *   F101→JL set：2 字节 [1, jl_mode]
 *   F101→JL get：payload [0]；JL 应答 1 字节 jl_mode
 *   F101 audio_effect 与 JL jl_mode 编号独立，由 apply/get 做映射
 */

/* F101 均衡器 UI / cfg->audio_effect 索引 */
#define DIVOOM_AUDIO_EFFECT_ORIGINAL  0u
#define DIVOOM_AUDIO_EFFECT_HIFI      1u
#define DIVOOM_AUDIO_EFFECT_OUTDOOR   2u
#define DIVOOM_AUDIO_EFFECT_RETRO     3u
#define DIVOOM_AUDIO_EFFECT_GAMING    4u

/* JL 侧 0x6D mode 取值（与 UI audio_effect 索引不同，映射见 .c） */
#define DIVOOM_BTCORE_SOUND_EFFECT_JL_GAMING    0u
#define DIVOOM_BTCORE_SOUND_EFFECT_JL_HIFI      1u
#define DIVOOM_BTCORE_SOUND_EFFECT_JL_ORIGINAL  2u
#define DIVOOM_BTCORE_SOUND_EFFECT_JL_OUTDOOR   3u
#define DIVOOM_BTCORE_SOUND_EFFECT_JL_RETRO     4u

int divoom_btcore_uart_cmd_apply_sound_effect(uint8_t audio_effect_idx);

/* get [0] → JL rsp [jl_mode]，映射为 F101 audio_effect 索引；失败返回 -1 */
int divoom_btcore_uart_cmd_send_get_sound_effect_mode(void);

/*
 * DM_BTCORE_IISIN_EQ（0x71）预留：设置/查询 JL IISIN EQ（EQ_IISIN 节点，0～16）。
 *   F101→JL set：[1, level]
 *   F101→JL get：[0] → JL 应答 1 字节 level
 * 业务尚未接线，先供串口/后续音量曲线调用。
 */
int divoom_btcore_uart_cmd_set_iisin_eq(uint8_t level);
int divoom_btcore_uart_cmd_get_iisin_eq(uint8_t *out_level);

/* DisableMic 且未播音乐：0x3b 应对 UI 归零（JL UART 仍可能推 mic 频谱，但 F101 不消费） */
int divoom_btcore_uart_cmd_mic_fft_suppressed(void);

/*
 * DM_BTCORE_GET_FFT_FREQ_DATA（0x3b）：
 *   JL→F101 周期推送（约 20ms）：data_size 2 字节小端（点数）+ 频谱数据（每点 1 字节，0～64）
 */

/*
 * DM_BTCORE_SET_MUSIC_PLAY_PAUSE_MSG（0x4E）载荷：
 *   字段 state，1 字节：0=暂停，1=播放
 */
#define DIVOOM_BTCORE_PLAY_PAUSE_STATE_PAUSE 0u
#define DIVOOM_BTCORE_PLAY_PAUSE_STATE_PLAY  1u

/*
 * DM_BTCORE_SET_MUSIC_TRACK_MSG（0x4F）载荷：
 *   字段 dir，1 字节：0=上一曲，1=下一曲
 */
#define DIVOOM_BTCORE_TRACK_DIR_PREV 0u
#define DIVOOM_BTCORE_TRACK_DIR_NEXT 1u

/*
 * DM_BTCORE_DEVICE_VERSION（0x50）：
 *   T113→JL 查询：空载荷（cmd_len=0）
 *   JL→T113 应答/上报：version 4 字节小端，如 353001
 *
 * DM_BTCORE_SYNC_BLUE_VOLUME_BY_PHONE（0x50，cmd_len=1）：
 *   字段 level，1 字节：音量 0～100
 */
#define DIVOOM_BTCORE_VOL_LEVEL_MIN 0u
#define DIVOOM_BTCORE_VOL_LEVEL_MAX 100u

int divoom_btcore_uart_cmd_get_jl_device_version(uint32_t *out_version);

/*
 * DM_BTCORE_SD_INSERT_STATUS（0x06）、DM_BTCORE_LINEIN_INSERT_STATUS（0x4E）、
 * DM_BTCORE_UAC_INSERT_STATUS（0x4F）：
 *   JL→T113 上报 / T113 查询应答：insert_status 1 字节，0=未插入，1=已插入
 *   T113→JL 查询：空载荷（cmd_len=0）
 */

/*
 * DM_BTCORE_SD_PLAY_INFO（0x07）JL→T113 半秒推送 TF 播放进度（11 字节）：
 *   cur_time     2 LE，当前播放秒；0xFFFF=解码器未就绪
 *   totle_time   2 LE，总时长秒；0xFFFF=未知
 *   totle_music  2 LE，歌单总数
 *   music_id     2 LE，当前曲目序号
 *   player_mode  1，1=列表循环 2=单曲 3=随机
 *   status       1，非 0=播放中
 *   volume       1，音量 0～100
 */
#define DIVOOM_BTCORE_INSERT_STATUS_ABSENT  0u
#define DIVOOM_BTCORE_INSERT_STATUS_PRESENT 1u

/*
 * DM_BTCORE_SERVICE_A2DP_STATUS（0x12）：
 *   JL→T113 通知 / T113 查询应答：status 1 字节，0=断开，1=连接
 *   T113→JL 查询：空载荷（cmd_len=0）
 */
#define DIVOOM_BTCORE_A2DP_STATUS_DISCONNECTED 0u
#define DIVOOM_BTCORE_A2DP_STATUS_CONNECTED    1u

/*
 * DM_BTCORE_BT_CONNECTED_PHONE_INFO（0x51）手机蓝牙信息：
 *   JL→T113 上报 / T113 查询应答：
 *     bt_addr      6 字节，手机蓝牙 MAC，二进制序与 DM_BTCORE_BT_INFO 中 addr 一致
 *                  （T113 侧格式化为 XX:XX:XX:XX:XX:XX）
 *     phone_name  63 字节，UTF-8，不足补 0x00；有效字符串以 '\0' 结尾
 *   T113→JL 查询：空载荷（cmd_len=0）
 *   载荷总长：69 字节（实际长度可小于 69，name 按剩余字节解析）。
 */
#define DIVOOM_BTCORE_BT_ADDR_BIN_LEN 6u
#define DIVOOM_BTCORE_PHONE_NAME_LEN 63u
#define DIVOOM_BTCORE_CONNECTED_PHONE_INFO_LEN \
	(DIVOOM_BTCORE_BT_ADDR_BIN_LEN + DIVOOM_BTCORE_PHONE_NAME_LEN)

#define DIVOOM_BTCORE_SCAN_MUSIC_LIST_STAT_START  0u
#define DIVOOM_BTCORE_SCAN_MUSIC_LIST_STAT_END    1u

/*
 * DM_BTCORE_SCAN_MUSIC_LIST（0x4d）：
 *   F101→JL：空载荷，请求 JL 重新扫描 TF 并创建歌单文件（UI 手动刷新）。
 *   JL→F101：cmd_data[0]=scan_status（0=开始 1=结束）。
 *     START：仅在即将重写 songlist.bin 时（general_music_list 已通过 skip/sta，开始写文件前）。
 *     END：仅 0x42 歌单相同、无需再传文件。
 *     F101 关窗：有文件靠落地；拔卡靠 0x06；相同靠这条 END；否则 watchdog。
 */

/*
 * DM_BTCORE_SONGLIST_INFO（0x42）歌单同步：
 *   JL→T113 查询：空载荷（cmd_len=0）
 *   T113→JL 应答：songlist_crc16(2) + timestamp(4) + songlist_path_len(1) + songlist_path
 *     songlist_crc16：歌单文件 CRC16（CCITT，初值 0）
 *     timestamp：歌单文件头 0x04~0x07 的 uint32 小端时间戳
 */

/*
 * DM_BTCORE_BAT_VOLT_AVR（0x34）JL→T113 推送：
 *   字段 volt，2 字节小端：电池平均电压，单位 mV
 *   字段 USB_volt，2 字节小端：USB 实时电压，单位 mV
 *   载荷总长：4 字节
 */

/*
 * DM_BTCORE_BAT_VOLT_LEVE（0x55）JL→T113 推送 / T113 查询应答：
 *   字段 level，1 字节：电量档位 0~9
 *   T113→JL 查询：空载荷（cmd_len=0）
 */

/*
 * DM_BTCORE_BT_MUSIC_TIME（0x57）载荷：
 *   字段 type，1 字节：1=总时间，2=当前播放时间
 *   字段 data，4 字节小端：时间值，单位毫秒
 */
#define DIVOOM_BTCORE_BT_MUSIC_TIME_TYPE_TOTAL   1u
#define DIVOOM_BTCORE_BT_MUSIC_TIME_TYPE_CURRENT 2u

/*
 * DM_BTCORE_OTA_START / OTA_DATA / OTA_END（0x59~0x5b）JL 固件 OTA：
 *   START 载荷：fw_version(4 LE) + fw_size(4 LE) + fw_crc32(4 LE) + flags(1)
 *   START 应答：err(1) + resume_offset(4 LE) + fw_size(4 LE)
 *   DATA  载荷：offset(4 LE) + len(2 LE) + payload(len)，单包最大 1024
 *   DATA  应答：err(1) + write_offset(4 LE)
 *   END   载荷：无；应答 err(1)
 */
#define DIVOOM_BTCORE_OTA_PACKET_SIZE       1024u
#define DIVOOM_BTCORE_OTA_FLAG_FORCE_FULL   0x01u
/* JL 擦除/校验较慢，OTA 各阶段 UART 等待秒数（send_wait 默认仅 1s） */
#define DIVOOM_BTCORE_OTA_START_TIMEOUT_S   15
#define DIVOOM_BTCORE_OTA_DATA_TIMEOUT_S    8
#define DIVOOM_BTCORE_OTA_END_TIMEOUT_S     30

/*
 * DM_BTCORE_SD_MUSIC_READY（0x5e）JL→T113 推送：
 *   无载荷；JL 切到 SD 音乐模式并完成就绪后通知 T113，此后 T113 才发 set_music_id(0x0e)
 */

typedef enum divoom_ota_host_err {
	DIVOOM_OTA_HOST_ERR_OK = 0,
	DIVOOM_OTA_HOST_ERR_BUSY = 1,
	DIVOOM_OTA_HOST_ERR_VERSION = 2,
	DIVOOM_OTA_HOST_ERR_NO_SPACE = 3,
	DIVOOM_OTA_HOST_ERR_INIT = 4,
	DIVOOM_OTA_HOST_ERR_OFFSET = 5,
	DIVOOM_OTA_HOST_ERR_WRITE = 6,
	DIVOOM_OTA_HOST_ERR_SIZE = 7,
	DIVOOM_OTA_HOST_ERR_VERIFY = 8,
	DIVOOM_OTA_HOST_ERR_NOT_ACTIVE = 9,
} divoom_ota_host_err_t;

/*
 * DM_BTCORE_SYSTEM_TIME（0x36）本地日历（无星期）：
 *   T113→JL 设置：ctrl=1 + year LE + mon/day/hour/min/sec，共 8 字节
 *   T113→JL 查询：ctrl=0，1 字节
 *   JL→T113 回复/主动推送：ctrl=0 + year LE + mon/day/hour/min/sec，共 8 字节（与设置同布局）
 *   CHECK_LINK 后 JL 用 post_system_time 推一帧；F101 未权威校时时采用。
 */
#define DIVOOM_BTCORE_SYSTEM_TIME_CTRL_SET    1u
#define DIVOOM_BTCORE_SYSTEM_TIME_CTRL_GET    0u
#define DIVOOM_BTCORE_SYSTEM_TIME_PAYLOAD_SET_LEN  8u
#define DIVOOM_BTCORE_SYSTEM_TIME_PAYLOAD_GET_LEN  1u
#define DIVOOM_BTCORE_SYSTEM_TIME_PAYLOAD_REPLY_LEN 8u

typedef struct divoom_btcore_alarm_time {
	uint16_t year;
	uint8_t  month;
	uint8_t  day;
	uint8_t  hour;
	uint8_t  minute;
	uint8_t  second;
} divoom_btcore_alarm_time_t;

/* 设置系统时间；t 为本地日历时刻 */
int divoom_btcore_uart_cmd_send_set_system_time(const divoom_btcore_alarm_time_t *t);

/* 将当前系统时间同步给 JL */
int divoom_btcore_uart_cmd_send_set_system_time_now(void);

/* 查询系统时间 */
int divoom_btcore_uart_cmd_send_get_system_time(uint8_t *reply_out, uint16_t reply_out_cap,
						uint16_t *reply_len_out);

/*
 * DM_BTCORE_SET_ALARM_TIME（0x38）T113→JL 设置/获取闹钟时间：
 *   ctrl    1 字节：1=设置，0=获取
 *   enable  1 字节：1=开启闹钟（需带时间字段），0=关闭闹钟（无需后续字段）
 *   year    2 字节小端：1~9999（enable=1 时）
 *   month   1 字节：1~12
 *   day     1 字节：1~31
 *   hour    1 字节：0~23
 *   minute  1 字节：0~59
 *   second  1 字节：0~59
 *   设置且 enable=1 时载荷 9 字节；设置且 enable=0 时载荷 2 字节；获取时载荷 1 字节（ctrl=0）。
 */
#define DIVOOM_BTCORE_ALARM_TIME_CTRL_SET    1u
#define DIVOOM_BTCORE_ALARM_TIME_CTRL_GET    0u
#define DIVOOM_BTCORE_ALARM_TIME_ENABLE      1u
#define DIVOOM_BTCORE_ALARM_TIME_DISABLE     0u
#define DIVOOM_BTCORE_ALARM_TIME_PAYLOAD_SET_ENABLE_LEN  9u
#define DIVOOM_BTCORE_ALARM_TIME_PAYLOAD_SET_DISABLE_LEN 2u
#define DIVOOM_BTCORE_ALARM_TIME_PAYLOAD_GET_LEN         1u

/* 设置闹钟时间（enable=1）；t 为本地日历时刻 */
int divoom_btcore_uart_cmd_send_set_alarm_time(const divoom_btcore_alarm_time_t *t);

/* 关闭闹钟（enable=0） */
int divoom_btcore_uart_cmd_send_disable_alarm_time(void);

/* 查询闹钟时间；reply 格式同设置应答（由 JL 定义，至少含 ctrl/enable 等） */
int divoom_btcore_uart_cmd_send_get_alarm_time(uint8_t *reply_out, uint16_t reply_out_cap,
					       uint16_t *reply_len_out);

/*
 * DM_BTCORE_WORK_MODE（0x39）AIC→JL 请求载荷：
 *   字段 ctrl，1 字节：0=获取，1=设置
 *   字段 work_mode，1 字节：设置时为目标模式；获取时可填 0
 * JL→AIC 应答载荷：
 *   字段 work_mode，1 字节：当前工作模式（与 SPP work_mode 枚举一致）
 */
#define DIVOOM_BTCORE_WORK_MODE_CTRL_GET 0u
#define DIVOOM_BTCORE_WORK_MODE_CTRL_SET 1u

/*
 * DM_BTCORE_WORK_MODE_SET_VOL（0x62）AstroToo F101→JL（SET only）：
 *   字段 work_mode，1 字节：目标模式（枚举同 0x39）
 *   字段 volume，1 字节：JL 音量档位（与 0x31 SET 的 level 一致）
 * JL 先切模式；在 app_set_current_mode 后同步设音量（见 divoom_os_com_try_apply_work_mode_switch_vol）；
 * 仍经 0x39 上报模式变化；本命令无应答载荷。
 */

/*
 * DM_BTCORE_HFP_CALL_VOLUME（0x6F）HFP 通话增益（非 VolCallGate）：
 *   F101→JL set：[1, level]，level 0～15（与 CALL_DVOL / HFP VGS 一比一）
 *   F101→JL get：[0] → JL 应答 1 字节 level
 *   JL→F101 上报：1 字节 level（切入 PHONE、手机改通话音量）
 */
#define DIVOOM_BTCORE_HFP_CALL_VOL_LEVEL_MIN 0u
#define DIVOOM_BTCORE_HFP_CALL_VOL_LEVEL_MAX DIVOOM_BTCORE_HFP_CALL_VOL_MAX

/*
 * DM_BTCORE_MUSIC_TAG_INFO（0x70）JL→F101 当前曲元数据（小端，UTF-8，无 '\0'）：
 *   source      1B    0=TF  1=BT  2=其它
 *   music_id    2B LE 与 0x07 同一套曲序号；非 TF 填 0
 *   flags       1B    bit0 标题来自标签  bit1 艺术家来自标签  bit2 有专辑
 *   title_len   1B
 *   title       N
 *   artist_len  1B
 *   artist      N
 * 以后可在包尾接 album_len + album；旧上位机按已解析长度停。
 * 第一版只收包校验/打印，不写入 tf_music / 表盘 / 歌词。
 */
#define DIVOOM_BTCORE_MUSIC_TAG_SRC_TF     0u
#define DIVOOM_BTCORE_MUSIC_TAG_SRC_BT     1u
#define DIVOOM_BTCORE_MUSIC_TAG_SRC_OTHER  2u
#define DIVOOM_BTCORE_MUSIC_TAG_FLAG_TITLE   0x01u
#define DIVOOM_BTCORE_MUSIC_TAG_FLAG_ARTIST  0x02u
#define DIVOOM_BTCORE_MUSIC_TAG_FLAG_ALBUM   0x04u
#define DIVOOM_BTCORE_MUSIC_TAG_HDR_LEN      6u

/*
 * DM_BTCORE_IISIN_EQ（0x71）F101→JL IISIN EQ（JL EQ_IISIN / divoom_os_com_set_eq_iisin）：
 *   set：[1, level]，level 0～16（与 Vol_Media / user_eq_iisin 档位一致）
 *   get：[0] → JL 应答 1 字节 level
 */

/*
 * DM_BTCORE_PHONE_CALL_ALERT（0x72）JL→F101 HFP 通话 UI（UTF-8，无 '\0'）：
 *
 *   action      1B    0=DISMISS（挂断/超时关窗，可仅 1 字节）
 *                   1=SHOW/UPDATE（展示或刷新）
 *   call_type   1B    action=1 时有效：0=来电 1=去电 2=通话中
 *   flags       1B    action=1 时有效：
 *                     bit0 含 number_len + number
 *                     bit1 含 name_len + name（通讯录名，可空）
 *                     bit2 手机带 inband ringtone（JL BT_STATUS_INBAND_RINGTONE）
 *   number_len  1B    flags&1；0～32
 *   number      N     号码，常见为数字与 '+'；未知可省略该字段
 *   name_len    1B    flags&2；0～64
 *   name        N     联系人显示名
 *
 * 最小载荷：DISMISS [00]；来电 alert（无号） [01 00 00]。
 * 号码后到、可 UPDATE 同一通：再发 SHOW + flags|1 + number。
 * 与 0x39 WORK_MODE=8 并行：0x39 管音量/模式；0x72 管弹窗语义（可区分来电/去电）。
 *
 * JL 建议触发（对齐 minitoo phone_name / phone_end）：
 *   BT_STATUS_PHONE_NUMBER     → SHOW INCOMING（有号则带 number）
 *   BT_STATUS_PHONE_INCOME     → SHOW INCOMING（无号可先不带 flags）
 *   BT_STATUS_PHONE_OUT        → SHOW OUTGOING（F101 v1 可忽略）
 *   BT_STATUS_PHONE_ACTIVE     → SHOW ACTIVE 或省略（v1 可忽略）
 *   BT_STATUS_PHONE_HANGUP     → DISMISS
 *
 * 示例（L = N+2，N 为命令数据字节数）：
 *   来电动画（N=3）：AA 05 00 72 01 00 00 … BB
 *   带号 10086（N=8）：AA 0A 00 72 01 00 01 05 31 30 30 38 36 … BB
 *   挂断（N=1）：    AA 03 00 72 00 … BB
 */

/*
 * DM_BTCORE_RECORD（0x20）F101→JL 开始/停止 MIC 录音：
 *   start 1 字节：1=开始，0=停止
 *   mode  1 字节：0=MIC（阶段三仅此项）
 * JL 开始：开麦、MP3 编码；边录边发 0x63；停录 flush 后发 0x64 END。
 * 固定参数：16kHz / mono / 32kbps / 最长 60s（协议不传，双方写死）。
 *
 * DM_BTCORE_REC_STREAM_DATA（0x63）JL→F101，变长：
 *   seq(4 LE) + len(2 LE) + payload(len)；len≤512
 *
 * DM_BTCORE_REC_STREAM_END（0x64）JL→F101，9 字节：
 *   total_bytes(4 LE), total_packets(2 LE), crc16(2 LE), reason(1)
 *   reason：0=正常，1=编码错误，2=发送溢出，3=60s 超时，4=通话抢占，5=UAC 抢占
 *   F101 本地：0x80=start 无首包，0x81=stop 无 END（见 divoom_btcore_rec_stream.h）
 */

/*
 * DM_BTCORE_GET_BOOT_SOURCE（0x56）T113→JL 查询开机源：
 *   请求：空载荷
 *   JL→T113 应答：Boot_source 1 字节，0=正常开机，1=闹钟唤醒
 */
#define DIVOOM_BTCORE_BOOT_SOURCE_NORMAL  0u
#define DIVOOM_BTCORE_BOOT_SOURCE_ALARM   1u

/*
 * DM_BTCORE_GET_BOOT_MODE（0x61）T113→JL 查询开机产测模式：
 *   请求：空载荷
 *   JL→T113 应答：boot_mode 1 字节，0=正常，1=产测（POWER+MODE 组合键）
 */
#define DIVOOM_BTCORE_BOOT_MODE_NORMAL        0u
#define DIVOOM_BTCORE_BOOT_MODE_PRODUCT_TEST 1u
#define DIVOOM_BTCORE_BOOT_MODE_QUERY_TIMEOUT_S 2

int divoom_btcore_uart_cmd_query_boot_mode(uint8_t *out_mode, int timeout_s);

/*
 * DM_BTCORE_CHECK_LINK（0x41）T113→JL 链路握手：
 *   version     4 字节小端 AIC 版本号
 *   fw_path_len 1 字节
 *   fw_path     fw_path_len 字节，JL 固件升级路径
 * JL→F101 应答：1 字节 cnt（本 JL 上电周期内第几次 CHECK_LINK；1=正常，>=2 异常重启）
 */
#ifndef DIVOOM_BTCORE_JL_FW_UPDATE_PATH
#define DIVOOM_BTCORE_JL_FW_UPDATE_PATH DIVOOM_OTA_RTOS_STAGING_DIR "/tf_update.zip"
#endif

int divoom_btcore_uart_cmd_send_check_link(void);

/* 最近一次 CHECK_LINK 应答中的 cnt；尚未握手成功返回 -1 */
int divoom_btcore_uart_cmd_get_host_boot_cnt(uint8_t *out_cnt);

/*
 * DM_BTCORE_BLE_CTRL（0x60）T113→JL BLE 广播开关：
 *   enable 1 字节：0=关闭广播，1=开启广播
 */
#define DIVOOM_BTCORE_BLE_CTRL_OFF 0u
#define DIVOOM_BTCORE_BLE_CTRL_ON  1u

int divoom_btcore_uart_cmd_send_ble_ctrl(uint8_t enable);

/*
 * DM_BTCORE_MAIN_SOC_BOOT_DONE（0x5f）F101→JL，单向无应答：
 *   空载荷：主控开机动画/音频通路已就绪；JL 收到后才允许向 F101 发 0x19。
 *   有 power_on.mp4 时在视频播完后再发（divoom_lvgl_poweron_on_boot_video_exited）；
 *   首启语言/配网跳过开机动画时立即发（divoom_lvgl_poweron_notify_boot_ready）；
 *   产测：首屏出现后满 2s 发 0x5f（menu_idle_timer 驱动 divoom_product_test_try_boot_done）。
 */
int divoom_btcore_uart_cmd_notify_aic_host_audio_ready(void);

/* 仅发送一次；开机视频未播完前勿调用；OOBE 跳过动画时由 notify_boot_ready 立即调用 */
int divoom_btcore_uart_cmd_try_notify_aic_host_audio_ready(void);

/* 是否已向 JL 发送过 DM_BTCORE_MAIN_SOC_BOOT_DONE（0x5f） */
int divoom_btcore_uart_cmd_is_boot_done_notified(void);

/* 未就绪时延后的 DM_BTCORE_SYSTEM_POWER_OFF，就绪后补执行 */
void divoom_btcore_uart_cmd_try_pending_poweroff(void);

/* T113→JL：请求重新扫描并创建歌单文件（空载荷） */
int divoom_btcore_uart_cmd_send_scan_music_list(void);

/* 尚未收到对应命令时返回 -1；否则返回 state/dir/level 的数值 */
int divoom_btcore_uart_cmd_get_play_pause_state(void);
int divoom_btcore_uart_cmd_get_track_dir(void);
int divoom_btcore_uart_cmd_get_music_vol_level(void);

/* 尚未收到对应插拔通知时返回 -1；否则 0=未插入，1=已插入 */
int divoom_btcore_uart_cmd_get_linein_insert_status(void);
int divoom_btcore_uart_cmd_get_uac_insert_status(void);

/*
 * 终端联调（btcore CLI）：本地模拟 JL→F101 收到一帧 BtCore 命令。
 * 启用 DIVOOM_BTCORE_UART_CLI 后可用；详见 tools/btcore_cli/README.md
 */
#ifdef DIVOOM_BTCORE_UART_CLI
void divoom_btcore_uart_cmd_debug_simulate_rx(uint8_t cmd, const uint8_t *cmd_data, uint16_t cmd_len);
#endif

#ifdef __cplusplus
}
#endif

#endif /* DIVOOM_BTCORE_UART_CMD_H */
#endif
