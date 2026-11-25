#include "divoom_lyric_demand.h"
#include "divoom_port_os.h"
#include "divoom.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/time.h>

#include "bt_manager.h"
#include "bt_dev_list.h"
#include "bt_log.h"
#include "divoom_bt_music.h"
#include "divoom_bt_bip.h"
#include "divoom_jpeg_decode.h"
#include "divoom_bt_lyric_parse.h"
#include "divoom_lyric_metadata.h"
#include "divoom_menu_control.h"
#include "divoom_lvgl_top_audio_tip_menu.h"
#include "divoom_lvgl_lyric_word_ctrl.h"

#define BT_A2DP_ACCEPTE	DIVOOM_TMP_DIR "/bt_a2dp_send"   //蓝牙管理进程写管道
#define BT_A2DP_SEND DIVOOM_TMP_DIR "/bt_a2dp_accepte"

// 蓝牙设备信息保存路径
#define BT_DEVICE_INFO_SAVE_PATH DIVOOM_SD_SYSTEM_DIR"/bt_device_info.cfg"
// 蓝牙配对密码配置路径（与蓝牙进程共用同一文件）
#define BT_PIN_SAVE_PATH         DIVOOM_SD_SYSTEM_DIR"/bt_pin.cfg" 
#define DIVOOM_BLUE_ADDR_SAVE_PATH DIVOOM_SD_SYSTEM_DIR"/blue_addr.cfg"
#define BT_PIN_CODE_LEN          4

// 协议常量
#define DATA_HEADER 0xAA             // 数据头
#define DATA_TAIL 0x55               // 数据尾

enum{
	DIVOOM_BT_DISCONNECTED,
	DIVOOM_BT_CONNECTED,
};

// 数据类型
typedef enum {
    DATA_SONG_INFO = 0x01,  // 歌曲信息（包含播放状态、歌词等）
    DATA_PLAY_CTRL = 0x02,  // 播放控制（如暂停/继续/停止指令）
    DATA_SET_VOL = 0x03,    // 设置音量（+ /- /指定某个值）
	DATA_CONNECT_STATUS = 0x04, //a2dp连接状态
    DATA_AUDIO_OUTPUT_ENABLE = 0x05, // 音频输出使能控制（0:禁用 1:启用）
    DATA_BT_PLAY_REQUEST = 0x06, // 蓝牙播放请求（蓝牙进程->APP进程，通知切换到蓝牙模式）
    DATA_DEVICE_INFO = 0x07, // 蓝牙设备信息（地址+名称，支持中文UTF-8）
    DATA_CONNECT_DEVICE = 0x08, // 连接指定蓝牙设备（APP进程->蓝牙进程）
    DATA_UNPAIR_DEVICE = 0x09, // 取消配对指定蓝牙设备（APP进程->蓝牙进程）
    DATA_DISABLE_BOOT_CONNECT = 0x0A, // 禁用开机自动重连（APP进程->蓝牙进程）
    DATA_EXIT_BT_SYSTEM = 0x0B, // 完全退出蓝牙系统（APP进程->蓝牙进程）
    DATA_START_CLASSIC_BT = 0x0C, // 开启经典蓝牙（APP进程->蓝牙进程）
    DATA_SET_BT_PIN = 0x0D,      // 设置蓝牙配对密码（APP进程->蓝牙进程）：1字节enable+4字节pin
    DATA_SET_ADAPTER_NAME = 0x0F,   // APP->BT：设置本机蓝牙名称（UTF-8，支持中文）
    DATA_LOCAL_ADAPTER_NAME = 0x10,   // BT->APP：本机当前蓝牙名称（如手机连上后同步）
    DATA_BT_DISCONNECT = 0x11,  // APP->BT：断开当前连接
    DATA_BT_DISABLE_CLASSIC = 0x12, // APP->BT：关闭经典蓝牙（保留 BLE）
    DATA_BT_PCM_STREAM = 0x13, /* BT->APP：PCM 供 FFT（播放在 divoom_bt_manage ALSA） */
} DataType;

static BtDeviceInfoManager g_bt_device_manager = {0};  // 全局设备信息管理器

// 根据回复结构体数据解析并填充设备蓝牙信息
// 适用结构体字段：bt_name, bt_br_name, bt_ble_name, ssp_switch, ssp_secre, addr
typedef struct {
    char bt_name[26];
    char bt_br_name[26];
    char bt_ble_name[26];
    uint8_t ssp_switch;
    uint8_t ssp_secret[5];
    char addr[6]; // 6字节原始地址，不是字符串
	uint8_t auto_connect; //1自动重连， 0不
	uint8_t auto_connect_valid; //1=已从JL收到0x15应答
} ParsedBtDeviceInfo;

typedef struct _divoom_bt_music_ctrl {
    uint8 file_buf[512];
    int bt_send_handle;
    int bt_accept_handle;
    int run_flag;
    uint8 volume;                   /* 当前音量 0-100，set 时缓存，蓝牙进程回传时同步 */
	DeviceInfo current_blue_info;
	divoom_bt_lrc_callback lrc_cb;
	void *lrc_user_data;
    SongInfo song_info;
	char prev_lyric[MAX_STR_LEN];   /* 上一句歌词（type=1 更新前缓存） */
	uint8 uart_lyric_phase;         /* 1=歌词相：type=1 当词；0=元数据相：type=1 当歌名 */
	char pending_type1[MAX_STR_LEN]; /* 歌词相暂存的 type=1，切回元数据时可能是新歌名 */
    uint8 lrc_notify_kind;          /* 见 DIVOOM_BT_LRC_NOTIFY_* */
	ParsedBtDeviceInfo bt_info;
    char local_adapter_name[MAX_DEVICE_NAME_LEN]; /* 本机蓝牙显示名，由蓝牙进程 DATA_LOCAL_ADAPTER_NAME 同步 */
    void *lock_handle;
} divoom_bt_music_ctrl, *Pdivoom_bt_music_ctrl;

static Pdivoom_bt_music_ctrl pdivoom_bt_music_ctrl = NULL;
static void *s_net_lyric_handle = NULL;
static uint8 s_uart_lyric_valid = 0;
static divoom_bt_lyric_ctx s_lyric_ctx = {0};
static uint32_t s_net_lyric_retry_ms = 0;
/* 切歌后等这一首自己的总时长包，再发第一次取词。切歌包里的 song_len 可能还是上一首。 */
static uint8 s_lyric_wait_total = 0;
/* 本首网词已确认失败。重拉会把 fetch_state 置回 0，标记留着避免提示闪掉 */
static uint8 s_net_lyric_miss = 0;
/* 下标跟 DIVOOM_LVGL_MENU_LANG_* 对齐，只给歌词失败提示用，不进 strings.xml */
static const char *const s_net_lyric_miss_text[DIVOOM_LVGL_MENU_LANG_END] =
{
	"Lyrics not found",
	"未获取到歌词",
	"未獲取到歌詞",
	"歌詞を取得できません",
	"Kein Liedtext gefunden",
	"ไม่พบเนื้อเพลง",
	"Текст не найден",
	"가사를 가져오지 못했습니다",
	"Không lấy được lời bài hát",
	"Paroles introuvables",
	"Şarkı sözü bulunamadı",
	"Letra no encontrada",
	"Letra não encontrada",
	"Lirik tidak ditemukan",
	"Testo non trovato",
	"Текст не знайдено",
	"Ingen sångtext hittades",
	"Sangtekst ikke fundet",
	"Nie znaleziono tekstu",
};
#define DIVOOM_BT_NET_LYRIC_RETRY_MS 8000u

static void divoom_bt_music_apply_uart_lyric(SongInfo *song_info, const char *text);
static void divoom_bt_music_apply_lyric_out(const divoom_bt_lyric_out *out);

/*
 * 本机加减后 JL/手机经 AVRCP 回传的档常对不齐。与 T113 相同：
 * 时间窗内忽略任意回传，避免 OSD 被拉回去；窗后手机主动调音量再同步。
 */
#define DIVOOM_BT_VOL_LOOPBACK_FILTER_MS 1200
static int s_bt_vol_last_sent = -1;     /* 最近一次本机下发的 0~100；-1=无 */
static uint32_t s_bt_vol_last_sent_ms = 0;

void divoom_bt_music_mark_sent_volume(int old_vol, int vol)
{
	(void)old_vol;
	if (!pdivoom_bt_music_ctrl)
		return;
	if (pdivoom_bt_music_ctrl->lock_handle)
		divoom_system_get_lock(pdivoom_bt_music_ctrl->lock_handle);
	s_bt_vol_last_sent = vol;
	s_bt_vol_last_sent_ms = divoom_system_get_tickms();
	if (pdivoom_bt_music_ctrl->lock_handle)
		divoom_system_free_lock(pdivoom_bt_music_ctrl->lock_handle);
}

static int divoom_bt_music_is_loopback_volume(uint8 vol)
{
	uint32_t now_ms;

	(void)vol;
	if (s_bt_vol_last_sent < 0)
		return 0;
	now_ms = divoom_system_get_tickms();
	return ((int32)(now_ms - s_bt_vol_last_sent_ms) <= DIVOOM_BT_VOL_LOOPBACK_FILTER_MS) ? 1 : 0;
}
// 蓝牙播放请求回调（当手机端开始播放时，通知上层切换到蓝牙模式）
static DIVOOM_BT_PLAY_REQUEST_CB g_bt_play_request_cb = NULL;
static int s_a2dp_connected = 0;
#define DIVOOM_BT_CACHED_PHONE_INFO_MAX DIVOOM_BTCORE_CONNECTED_PHONE_INFO_LEN
static uint8_t s_cached_phone_info[DIVOOM_BT_CACHED_PHONE_INFO_MAX];
static uint16_t s_cached_phone_info_len;
/* AVRCP 播放态 */
static int s_bt_a2dp_playing = 0;

// 前向声明
static void divoom_bt_music_net_lyric_request(void);
static void divoom_bt_music_net_lyric_refresh(void);
static void divoom_bt_music_cleanup(void);
static int divoom_bt_music_reopen_pipes(void);
static int divoom_bt_music_save_device_info_to_file(void);
static int divoom_bt_music_load_device_info_from_file(void);
static int divoom_bt_music_add_or_update_device(const DeviceInfo *info);
#ifdef DIVOOM_TIMES_VIBE
static int divoom_bt_music_is_auto_reconnect_enabled(void);
static void divoom_bt_music_reject_unwanted_connect(void);
#endif

/* 256 表示本地 1 秒推进歌词 1 秒。按上报快慢收，限制在大约 0.5 到 1.5。 */
#define DIVOOM_BT_LYRIC_TIME_RATE_ONE 256
#define DIVOOM_BT_LYRIC_TIME_RATE_MIN 128
#define DIVOOM_BT_LYRIC_TIME_RATE_MAX 384
#define DIVOOM_BT_LYRIC_TIME_SEEK_MS 3000
#define DIVOOM_BT_LYRIC_TIME_HOLD_MS 2000u
#define DIVOOM_BT_LYRIC_TIME_RATE_WALL_MIN_MS 200u
#define DIVOOM_BT_LYRIC_TIME_RATE_WALL_MAX_MS 3000u
static uint32_t s_lyric_time_pos_ms;
static uint32_t s_lyric_time_tick;
static int s_lyric_time_rate = DIVOOM_BT_LYRIC_TIME_RATE_ONE;
static int s_lyric_time_valid;
static int s_lyric_time_playing;

/* AVRCP 的 PLAYING 经常不来。只看 s_bt_a2dp_playing 时，上报间隙的进度补不上。表盘已在播就按播放处理。 */
static int divoom_bt_music_net_lyric_time_playing_now(void)
{
	return (s_bt_a2dp_playing || divoom_music_ctrl_is_playing()) ? 1 : 0;
}

static void divoom_bt_music_net_lyric_time_reset(void)
{
	s_lyric_time_valid = 0;
	s_lyric_time_rate = DIVOOM_BT_LYRIC_TIME_RATE_ONE;
	s_lyric_time_pos_ms = 0;
	s_lyric_time_tick = 0;
	s_lyric_time_playing = 0;
}

static uint32_t divoom_bt_music_net_lyric_time_advance(uint32_t now_ms, uint32_t elapsed_cap)
{
	uint32_t elapsed;
	uint32_t add;

	elapsed = now_ms - s_lyric_time_tick;
	if (elapsed > elapsed_cap)
		elapsed = elapsed_cap;
	add = (elapsed * (uint32_t)s_lyric_time_rate) / DIVOOM_BT_LYRIC_TIME_RATE_ONE;
	return s_lyric_time_pos_ms + add;
}

/* 记下这次上报。和本地推算差得少就按快慢收倍率，差得多就当成拖进度直接对齐。 */
static void divoom_bt_music_net_lyric_time_note(uint32_t time_ms, uint32_t now_ms)
{
	uint32_t wall;
	int pred;
	int err;
	int inst;

	if (!s_lyric_time_valid)
	{
		s_lyric_time_pos_ms = time_ms;
		s_lyric_time_tick = now_ms;
		s_lyric_time_rate = DIVOOM_BT_LYRIC_TIME_RATE_ONE;
		s_lyric_time_valid = 1;
		s_lyric_time_playing = divoom_bt_music_net_lyric_time_playing_now();
		return;
	}
	wall = now_ms - s_lyric_time_tick;
	if (wall > 60000u)
	{
		/* 间隔太长，只对齐到这次上报，不拿这段时间改快慢。 */
		s_lyric_time_rate = DIVOOM_BT_LYRIC_TIME_RATE_ONE;
	}
	else
	{
		pred = (int)s_lyric_time_pos_ms + (int)((wall * (uint32_t)s_lyric_time_rate) / DIVOOM_BT_LYRIC_TIME_RATE_ONE);
		err = (int)time_ms - pred;
		if (err > DIVOOM_BT_LYRIC_TIME_SEEK_MS || err < -DIVOOM_BT_LYRIC_TIME_SEEK_MS)
		{
			/* 和本地推算差了大约 3 秒以上，当成拖了进度，直接用新数字，倍率回到 1。 */
			s_lyric_time_rate = DIVOOM_BT_LYRIC_TIME_RATE_ONE;
		}
		else if (wall >= DIVOOM_BT_LYRIC_TIME_RATE_WALL_MIN_MS && wall <= DIVOOM_BT_LYRIC_TIME_RATE_WALL_MAX_MS)
		{
			/* 按这次进度增量相对本地经过时间收一点。上报偏快就加快，偏慢就放慢，不写死固定偏移。 */
			inst = ((int)time_ms - (int)s_lyric_time_pos_ms) * DIVOOM_BT_LYRIC_TIME_RATE_ONE / (int)wall;
			if (inst < DIVOOM_BT_LYRIC_TIME_RATE_MIN)
				inst = DIVOOM_BT_LYRIC_TIME_RATE_MIN;
			if (inst > DIVOOM_BT_LYRIC_TIME_RATE_MAX)
				inst = DIVOOM_BT_LYRIC_TIME_RATE_MAX;
			s_lyric_time_rate = (s_lyric_time_rate * 3 + inst) / 4;
			if (s_lyric_time_rate < DIVOOM_BT_LYRIC_TIME_RATE_MIN)
				s_lyric_time_rate = DIVOOM_BT_LYRIC_TIME_RATE_MIN;
			if (s_lyric_time_rate > DIVOOM_BT_LYRIC_TIME_RATE_MAX)
				s_lyric_time_rate = DIVOOM_BT_LYRIC_TIME_RATE_MAX;
		}
	}
	s_lyric_time_pos_ms = time_ms;
	s_lyric_time_tick = now_ms;
	s_lyric_time_valid = 1;
	s_lyric_time_playing = divoom_bt_music_net_lyric_time_playing_now();
}

/* 查词用的时间：播放中用上次上报加上本地经过时间。固定提前已关掉，暂停时停住。 */
static uint32_t divoom_bt_music_net_lyric_time_ms(uint32_t reported_ms)
{
	uint32_t now_ms;
	int playing;

	playing = divoom_bt_music_net_lyric_time_playing_now();
	if (!s_lyric_time_valid)
		return reported_ms;
	now_ms = divoom_system_get_tickms();
	if (s_lyric_time_playing && !playing)
	{
		/* 刚暂停：停在已经补上的进度，锚点改到现在，恢复后不会把暂停时长加进去。 */
		s_lyric_time_pos_ms = divoom_bt_music_net_lyric_time_advance(now_ms, DIVOOM_BT_LYRIC_TIME_HOLD_MS);
		s_lyric_time_tick = now_ms;
	}
	else if (!s_lyric_time_playing && playing)
	{
		s_lyric_time_tick = now_ms;
	}
	s_lyric_time_playing = playing;
	/* 暂停停在当前进度，不再往下翻。 */
	if (!playing)
		return s_lyric_time_pos_ms;
	return divoom_bt_music_net_lyric_time_advance(now_ms, DIVOOM_BT_LYRIC_TIME_HOLD_MS);
}

uint32_t divoom_bt_music_get_song_len_ms(void)
{
	if (!pdivoom_bt_music_ctrl)
		return 0;
	return pdivoom_bt_music_ctrl->song_info.song_len;
}

int divoom_bt_music_consume_lrc_notify_kind(void)
{
    int kind;

    if (!pdivoom_bt_music_ctrl)
        return DIVOOM_BT_LRC_NOTIFY_PLAYBACK;
    kind = (int)pdivoom_bt_music_ctrl->lrc_notify_kind;
    pdivoom_bt_music_ctrl->lrc_notify_kind = DIVOOM_BT_LRC_NOTIFY_PLAYBACK;
    return kind;
}

void divoom_bt_music_lrc_set_callback(divoom_bt_lrc_callback callback,void *para)
{
    if (pdivoom_bt_music_ctrl) 
	{
		pdivoom_bt_music_ctrl->lrc_cb = callback;
		pdivoom_bt_music_ctrl->lrc_user_data = para;
        if (pdivoom_bt_music_ctrl->lrc_cb)
        {
            pdivoom_bt_music_ctrl->lrc_notify_kind = DIVOOM_BT_LRC_NOTIFY_PLAYBACK;
            pdivoom_bt_music_ctrl->lrc_cb(pdivoom_bt_music_ctrl->lrc_user_data, pdivoom_bt_music_ctrl->song_info.singer, pdivoom_bt_music_ctrl->song_info.song_name, pdivoom_bt_music_ctrl->song_info.lyric, pdivoom_bt_music_ctrl->song_info.play_pos);
        }
	}	
}

// 解析并处理歌曲信息
static void divoom_bt_music_handle_song_info(SongInfo *info) 
{
	int song_changed = 0;

    if (!info) 
	{
        return;
    }

	s_a2dp_connected = 1;
    divoom_print("\nsong：%s - %s\n", info->singer, info->song_name);
    
    // 避免除零错误
    if (info->song_len > 0) 
	{
        divoom_print("pos：%ums/%ums (%.1f%%)\n",    info->play_pos, info->song_len, (float)info->play_pos / info->song_len * 100);
    } 
	else 
	{
        divoom_print("pos：%ums/未知\n", info->play_pos);
    }
    
    divoom_print("vol：%d\n", info->volume);
    divoom_print("lyric：%s\n", info->lyric);

	if (strcmp(info->lyric, pdivoom_bt_music_ctrl->song_info.lyric) != 0)
	{
		strncpy(pdivoom_bt_music_ctrl->prev_lyric, pdivoom_bt_music_ctrl->song_info.lyric,
			sizeof(pdivoom_bt_music_ctrl->prev_lyric) - 1);
		pdivoom_bt_music_ctrl->prev_lyric[sizeof(pdivoom_bt_music_ctrl->prev_lyric) - 1] = '\0';
	}
	if (strcmp(info->song_name, pdivoom_bt_music_ctrl->song_info.song_name) != 0)
	{
		pdivoom_bt_music_ctrl->prev_lyric[0] = '\0';
		s_net_lyric_miss = 0; /* 歌名变了，上一首的失败提示不要带到新歌 */
		divoom_bt_music_net_lyric_time_reset(); /* 换歌后上次的进度锚点和快慢倍率不能带到新歌。 */
		s_uart_lyric_valid = 0;
		pdivoom_bt_music_ctrl->uart_lyric_phase = 0;
		pdivoom_bt_music_ctrl->pending_type1[0] = '\0';
		divoom_bt_lyric_parse_reset(&s_lyric_ctx);
		song_changed = 1;
	}

    memcpy(&pdivoom_bt_music_ctrl->song_info, info, sizeof(SongInfo));
	if (song_changed)
	{
		/* 整包拷贝会把上一首歌词带回来，新词未到前先清掉，避免表盘残留。 */
		pdivoom_bt_music_ctrl->song_info.lyric[0] = '\0';
		strncpy(s_lyric_ctx.title, info->song_name, sizeof(s_lyric_ctx.title) - 1);
		s_lyric_ctx.title[sizeof(s_lyric_ctx.title) - 1] = '\0';
		strncpy(s_lyric_ctx.artist, info->singer, sizeof(s_lyric_ctx.artist) - 1);
		s_lyric_ctx.artist[sizeof(s_lyric_ctx.artist) - 1] = '\0';
		/* 整包拷贝会带上上一首的 song_len。先丢掉，等这一首的总时长包再取词。 */
		pdivoom_bt_music_ctrl->song_info.song_len = 0;
		s_lyric_wait_total = 1;
		if (s_net_lyric_handle)
			divoom_lvgl_lyric_word_ctrl_drop_song(s_net_lyric_handle);
#if 0
		/* 切歌立刻取词会把上一首时长写进 lyric_duration_sec，后面的总时长包改不掉。总时长到了再打开。 */
		divoom_bt_music_net_lyric_request();
#endif
	}
	divoom_bt_music_net_lyric_refresh();
	if (pdivoom_bt_music_ctrl->lrc_cb)
	{
		pdivoom_bt_music_ctrl->lrc_notify_kind = DIVOOM_BT_LRC_NOTIFY_PLAYBACK;
		pdivoom_bt_music_ctrl->lrc_cb(pdivoom_bt_music_ctrl->lrc_user_data, info->singer, info->song_name, info->lyric, info->play_pos);
	}
}

static void divoom_bt_music_apply_uart_lyric(SongInfo *song_info, const char *text)
{
	if (!song_info || !text)
		return;
	if (strcmp(song_info->lyric, text) != 0)
	{
		strncpy(pdivoom_bt_music_ctrl->prev_lyric, song_info->lyric,
			sizeof(pdivoom_bt_music_ctrl->prev_lyric) - 1);
		pdivoom_bt_music_ctrl->prev_lyric[sizeof(pdivoom_bt_music_ctrl->prev_lyric) - 1] = '\0';
	}
	strncpy(song_info->lyric, text, sizeof(song_info->lyric) - 1);
	song_info->lyric[sizeof(song_info->lyric) - 1] = '\0';
	s_uart_lyric_valid = 1;
}

static void divoom_bt_music_apply_lyric_out(const divoom_bt_lyric_out *out)
{
	SongInfo *song_info;

	if (!out || !pdivoom_bt_music_ctrl)
		return;
	song_info = &pdivoom_bt_music_ctrl->song_info;
	if (out->lyric_changed)
		divoom_bt_music_apply_uart_lyric(song_info, out->lyric ? out->lyric : "");
	else if (out->uart_lyric_valid && out->lyric)
		strncpy(song_info->lyric, out->lyric, sizeof(song_info->lyric) - 1);
	song_info->lyric[sizeof(song_info->lyric) - 1] = '\0';
	if (out->song_changed) {
		pdivoom_bt_music_ctrl->prev_lyric[0] = '\0';
		s_net_lyric_miss = 0;
		divoom_bt_music_net_lyric_time_reset(); /* 解析出换歌时同样清掉校正，避免沿用上一首的推进速度。 */
		if (!out->lyric_changed)
			song_info->lyric[0] = '\0'; /* 这次没有新句，清掉表盘上的上一首，等新词到了再写。 */
	}
	strncpy(song_info->song_name, out->title ? out->title : "", sizeof(song_info->song_name) - 1);
	song_info->song_name[sizeof(song_info->song_name) - 1] = '\0';
	strncpy(song_info->singer, out->artist ? out->artist : "", sizeof(song_info->singer) - 1);
	song_info->singer[sizeof(song_info->singer) - 1] = '\0';
	s_uart_lyric_valid = out->uart_lyric_valid;
	pdivoom_bt_music_ctrl->uart_lyric_phase = s_lyric_ctx.lyric_phase;
	if (s_lyric_ctx.burst_type1_valid)
		strncpy(pdivoom_bt_music_ctrl->pending_type1, s_lyric_ctx.burst_type1,
			sizeof(pdivoom_bt_music_ctrl->pending_type1) - 1);
	else
		pdivoom_bt_music_ctrl->pending_type1[0] = '\0';
	pdivoom_bt_music_ctrl->pending_type1[sizeof(pdivoom_bt_music_ctrl->pending_type1) - 1] = '\0';
	if (out->song_changed || (out->title_changed && out->artist_changed))
	{
		/* 歌名包先到，song_len 还是上一首。先作废旧表，等 TOTAL 再取。 */
		song_info->song_len = 0;
		s_lyric_wait_total = 1;
		if (s_net_lyric_handle)
			divoom_lvgl_lyric_word_ctrl_drop_song(s_net_lyric_handle);
#if 0
		/* 这里立刻取词会把上一首时长快照进请求。总时长包到了再打开。 */
		divoom_bt_music_net_lyric_request();
#endif
	}
}

static void divoom_bt_music_net_lyric_ensure(void)
{
	if (s_net_lyric_handle)
	{
		return;
	}
	s_net_lyric_handle = divoom_lvgl_lyric_word_ctrl_init_fetch();
}

static int divoom_bt_music_net_lyric_duration_sec(void)
{
	uint32_t song_len_ms;

	if (!pdivoom_bt_music_ctrl)
	{
		return 0;
	}
	song_len_ms = pdivoom_bt_music_ctrl->song_info.song_len;
	if (song_len_ms == 0)
	{
		return 0;
	}
	return (int)((song_len_ms + 500u) / 1000u);
}

static void divoom_bt_music_net_lyric_request_in(void)
{
	SongInfo *song_info;
    static char last_status_artist[DIVOOM_BT_LYRIC_STR_LEN];

	if (!pdivoom_bt_music_ctrl || s_uart_lyric_valid || divoom_main_net_lyric_region_off())
	{
		return;
	}
	song_info = &pdivoom_bt_music_ctrl->song_info;
	if (!song_info->song_name[0] || !song_info->singer[0])
	{
		return;
	}
    if (divoom_lyric_artist_is_status(song_info->singer))
    {
        if (strcmp(last_status_artist, song_info->singer))
        {
            snprintf(last_status_artist, sizeof(last_status_artist), "%s", song_info->singer);
            divoom_print("[lyric_metadata] skip non-artist status=\"%s\"\n", song_info->singer);
        }
        return;
    }
    last_status_artist[0] = '\0';
	if (s_lyric_wait_total || song_info->song_len == 0)
	{
		return; /* 这一首的总时长还没到。时长为 0 会跳过 ISRC 搜索，不能拿去取词。 */
	}
	if (!s_net_lyric_handle && !divoom_lyric_demand_snapshot())
	{
		return;
	}
	divoom_bt_music_net_lyric_ensure();
	if (!s_net_lyric_handle)
	{
		return;
	}
	divoom_lvgl_lyric_word_ctrl_set_artist_track(s_net_lyric_handle, song_info->singer, song_info->song_name, "", divoom_bt_music_net_lyric_duration_sec());
}

/* 表盘线程与蓝牙通知都可补触发，串行调度避免重复创建取词句柄。 */
static void divoom_bt_music_net_lyric_request(void)
{
    static int busy;
    static int pending;
    divoom_port_critical_enter();
    if(busy)
    {
        pending = 1;
        divoom_port_critical_exit();
        return;
    }
    busy = 1;
    do
    {
        pending = 0;
        divoom_port_critical_exit();
        divoom_bt_music_net_lyric_request_in();
        divoom_port_critical_enter();
    } while(pending);
    busy = 0;
    divoom_port_critical_exit();
}

void divoom_bt_music_lyric_region_changed(void)
{
    if (s_net_lyric_handle)
        divoom_lvgl_lyric_word_ctrl_drop_song(s_net_lyric_handle);
    if (pdivoom_bt_music_ctrl && !s_uart_lyric_valid)
    {
        pdivoom_bt_music_ctrl->song_info.lyric[0] = '\0';
        pdivoom_bt_music_ctrl->prev_lyric[0] = '\0';
    }
    s_net_lyric_miss = 0;
    divoom_bt_music_net_lyric_request();
}

void divoom_bt_music_lyric_demand_changed(void)
{
    s_net_lyric_miss = 0;
    divoom_bt_music_net_lyric_request();
}

static void divoom_bt_music_net_lyric_refresh(void)
{
	SongInfo *song_info;
	char cur[MAX_STR_LEN];
	char prev[MAX_STR_LEN];

	if (!pdivoom_bt_music_ctrl || s_uart_lyric_valid || !s_net_lyric_handle)
	{
		return;
	}
	song_info = &pdivoom_bt_music_ctrl->song_info;
	/* 查句用校正时间。play_pos 仍是手机原始进度，给进度条和回调用。 */
	if (!divoom_lvgl_lyric_word_ctrl_peek_line_at(s_net_lyric_handle, divoom_bt_music_net_lyric_time_ms(song_info->play_pos), cur, sizeof(cur), prev, sizeof(prev)))
	{
		return;
	}
	if (strcmp(song_info->lyric, cur) == 0)
	{
		return;
	}
	if (prev[0])
	{
		strncpy(pdivoom_bt_music_ctrl->prev_lyric, prev, sizeof(pdivoom_bt_music_ctrl->prev_lyric) - 1);
		pdivoom_bt_music_ctrl->prev_lyric[sizeof(pdivoom_bt_music_ctrl->prev_lyric) - 1] = '\0';
	}
	else
	{
		strncpy(pdivoom_bt_music_ctrl->prev_lyric, song_info->lyric, sizeof(pdivoom_bt_music_ctrl->prev_lyric) - 1);
		pdivoom_bt_music_ctrl->prev_lyric[sizeof(pdivoom_bt_music_ctrl->prev_lyric) - 1] = '\0';
	}
	strncpy(song_info->lyric, cur, sizeof(song_info->lyric) - 1);
	song_info->lyric[sizeof(song_info->lyric) - 1] = '\0';
}

void divoom_bt_music_set_title_lyric(uint8 *str, uint8 flag)
{
	char text[MAX_STR_LEN];
	SongInfo *song_info;
	divoom_bt_lyric_out out;
	uint32_t now_ms;

	if (!pdivoom_bt_music_ctrl || str == NULL)
		return;

	strncpy(text, (char *)str, sizeof(text) - 1);
	text[sizeof(text) - 1] = '\0';

	song_info = &pdivoom_bt_music_ctrl->song_info;
	now_ms = divoom_system_get_tickms();
	divoom_bt_lyric_parse_feed(&s_lyric_ctx,
		(flag == 2) ? DIVOOM_BT_LYRIC_EV_TYPE2 : DIVOOM_BT_LYRIC_EV_TYPE1,
		text, 0, now_ms, &out);
	divoom_bt_music_apply_lyric_out(&out);
	divoom_print("bt music parse title=%s artist=%s lyric=%s trace=%s\n",
		out.title ? out.title : "", out.artist ? out.artist : "",
		out.lyric ? out.lyric : "", out.trace ? out.trace : "-");

	if (pdivoom_bt_music_ctrl->lrc_cb)
	{
		pdivoom_bt_music_ctrl->lrc_notify_kind = (flag == 2) ?
		    DIVOOM_BT_LRC_NOTIFY_UART_TITLE : DIVOOM_BT_LRC_NOTIFY_UART_LYRIC;
		pdivoom_bt_music_ctrl->lrc_cb(pdivoom_bt_music_ctrl->lrc_user_data, song_info->singer, song_info->song_name, song_info->lyric, song_info->play_pos);
	}
}

void divoom_bt_music_handle_music_time(uint8 type, uint32 time_ms)
{
	divoom_bt_lyric_out out;
	uint32_t now_ms;

	if (!pdivoom_bt_music_ctrl)
		return;

	now_ms = divoom_system_get_tickms();
	if (type == DIVOOM_BTCORE_BT_MUSIC_TIME_TYPE_TOTAL)
	{
		int fetch_state = 0;

		divoom_bt_lyric_parse_feed(&s_lyric_ctx, DIVOOM_BT_LYRIC_EV_TOTAL,
			"", time_ms, now_ms, &out);
		divoom_bt_music_apply_lyric_out(&out);
		pdivoom_bt_music_ctrl->song_info.song_len = time_ms;
		if (!s_uart_lyric_valid)
		{
			int used_sec = 0;
			int synced = 0;
			int new_sec = (int)((time_ms + 500u) / 1000u);
			int diff;
			/* 取词句柄是 request 里才创建的。切歌不再立刻取词之后，这里不能先要求句柄已经存在，否则永远不会发请求。 */
			if (s_net_lyric_handle)
				divoom_lvgl_lyric_word_ctrl_get_duration(s_net_lyric_handle, &used_sec, &synced);
			diff = new_sec > used_sec ? new_sec - used_sec : used_sec - new_sec;
			if (new_sec > 0 && (s_lyric_wait_total || !s_net_lyric_handle || (!synced && diff > 5)))
			{
				/* 这一首的总时长到了，或和已取用的时长差超过 5 秒且还没有同步词，用新时长重取。 */
				s_lyric_wait_total = 0;
				divoom_bt_music_net_lyric_request();
			}
			else if (s_net_lyric_handle)
			{
				divoom_lvgl_lyric_word_ctrl_get_fetch_state(s_net_lyric_handle, &fetch_state, NULL);
				if (fetch_state == -1)
				{
					if (s_net_lyric_retry_ms == 0 || (now_ms - s_net_lyric_retry_ms) >= DIVOOM_BT_NET_LYRIC_RETRY_MS)
					{
						s_net_lyric_retry_ms = now_ms;
						divoom_bt_music_net_lyric_request();
					}
				}
			}
		}
	}
	else if (type == DIVOOM_BTCORE_BT_MUSIC_TIME_TYPE_CURRENT)
	{
		SongInfo *song_info = &pdivoom_bt_music_ctrl->song_info;

		/* JL 偶发把总时长当作当前位置(type=2)上报，直接忽略 */
		if (song_info->song_len > 0u && time_ms >= song_info->song_len - 200u) {
			return;
		}
		if (s_net_lyric_handle && song_info->play_pos > time_ms && (song_info->play_pos - time_ms) > 5000u)
		{
			divoom_lvgl_lyric_word_ctrl_reset_calib(s_net_lyric_handle, song_info->play_pos, time_ms);
		}

		divoom_bt_lyric_parse_feed(&s_lyric_ctx, DIVOOM_BT_LYRIC_EV_POS,
			"", time_ms, now_ms, &out);
		divoom_bt_music_apply_lyric_out(&out);
		song_info->play_pos = time_ms;
		/* 原始进度留给进度条。查词按这次上报相对上次推算的快慢来跟。 */
		divoom_bt_music_net_lyric_time_note(time_ms, now_ms);
		divoom_bt_music_net_lyric_refresh();
		if (pdivoom_bt_music_ctrl->lrc_cb)
		{
			pdivoom_bt_music_ctrl->lrc_notify_kind = DIVOOM_BT_LRC_NOTIFY_PLAYBACK;
			pdivoom_bt_music_ctrl->lrc_cb(pdivoom_bt_music_ctrl->lrc_user_data,
						      song_info->singer, song_info->song_name,
						      song_info->lyric, song_info->play_pos);
		}
	}
}

// 解析并处理播放控制指令
void divoom_bt_music_handle_play_ctrl(uint8 ctrl) 
{
    switch (ctrl) 
    {
        case BTMG_AVRCP_PLAYSTATE_PAUSED:  
        {
            divoom_print("BTMG_AVRCP_PLAYSTATE_PAUSED\n");
            s_bt_a2dp_playing = 0;
            divoom_music_ctrl_set_bt_remote_play_state(0);
            /* 不把 AVRCP 暂停和停止合并成同一个布尔状态。 */
            if (divoom_music_ctrl_get_mode() == DIVOOM_MUSIC_CTRL_MODE_BT)
            {
                divoom_music_ctrl_post_transport_state(DIVOOM_MUSIC_TRANSPORT_PAUSED);
            }
            break;
        }
        case BTMG_AVRCP_PLAYSTATE_PLAYING: 
        {
            divoom_print("BTMG_AVRCP_PLAYSTATE_PLAYING\n");
            s_bt_a2dp_playing = 1;
            divoom_music_ctrl_set_bt_remote_play_state(1);
            break;
        }
        case BTMG_AVRCP_PLAYSTATE_STOPPED:   
        {
            divoom_print("BTMG_AVRCP_PLAYSTATE_STOPPED\n");
            s_bt_a2dp_playing = 0;
            divoom_music_ctrl_set_bt_remote_play_state(0);
            if (divoom_music_ctrl_get_mode() == DIVOOM_MUSIC_CTRL_MODE_BT)
            {
                divoom_music_ctrl_post_transport_state(DIVOOM_MUSIC_TRANSPORT_STOPPED);
            }
            break;
        }
    }
}

// 处理音量设置（蓝牙进程回传或设置结果时更新缓存）
void divoom_bt_music_handle_play_vol(uint8 vol)
{
	int is_loopback;
	if (!pdivoom_bt_music_ctrl)
		return;

#ifdef DIVOOM_TIMES_VIBE
	/* JL 上报 0~16 档，按等比例表转成 0~100（档位 × 100 / 16） */
	if (vol <= DIVOOM_MAX_VOL_VALUE)
	{
		vol = divoom_com_convert_vol(vol);
	}
#endif
    divoom_print("recevie vol: %d\n", vol);
	/* 手机端发来的音量在设备端保存：当前为蓝牙模式则同步设置并保存，否则仅保存供切回 BT 时使用 */
	vol = (vol > 100) ? 100 : vol;
	if (pdivoom_bt_music_ctrl && pdivoom_bt_music_ctrl->lock_handle)
		divoom_system_get_lock(pdivoom_bt_music_ctrl->lock_handle);
	is_loopback = divoom_bt_music_is_loopback_volume(vol);
	if (pdivoom_bt_music_ctrl && pdivoom_bt_music_ctrl->lock_handle)
		divoom_system_free_lock(pdivoom_bt_music_ctrl->lock_handle);
	if (is_loopback)
	{
		divoom_print("BT vol: ignore phone echo vol=%u\n", (unsigned)vol);
		return;
	}
	if (divoom_music_ctrl_get_mode() == DIVOOM_MUSIC_CTRL_MODE_BT)
	{
		divoom_music_ctrl_save_bt_volume(vol, 1);
		divoom_main_set_sys_vol(vol);
	}
}

/*
	BTMG_A2DP_SINK_DISCONNECTED,
    BTMG_A2DP_SINK_CONNECTING,
    BTMG_A2DP_SINK_CONNECTED,
    BTMG_A2DP_SINK_DISCONNECTING,
	BTMG_A2DP_SINK_CONNECT_FAILED,
    BTMG_A2DP_SINK_DISCONNEC_FAILED,

*/
/* A2DP 是否已连接（仅 CONNECTED 为 1，CONNECTING/DISCONNECTED 等为 0），供 UI/状态显示 */
int divoom_bt_music_is_connected(void)
{
	// divoom_print("divoom bt music statu: %d\n", s_a2dp_connected);
	return s_a2dp_connected;
}

int divoom_bt_music_is_a2dp_playing(void)
{
	return s_bt_a2dp_playing ? 1 : 0;
}

static uint8 s_defer_connect_tone_for_poweron = 0;
static uint8 s_poweron_connect_tone_done = 0;
static uint8 s_bt_skip_boot_connect_tip = 1;

/* 老化测试长视频：始终不播连/断提示音 */
static int divoom_bt_music_skip_connect_tone(void)
{
	if (divoom_menu_control_get_active_menu_id() ==
	    DIVOOM_MENU_CONTROL_ID_PRODUCT_STRESS_TESTING_MENU)
		return 1;
	return 0;
}

/* 开机动画正在播/尚未退出：延后连接提示音 */
static int divoom_bt_music_should_defer_connect_tone(void)
{
	if (s_poweron_connect_tone_done)
		return 0;
	return divoom_lvgl_poweron_should_defer_connect_tone() ? 1 : 0;
}

static int divoom_bt_music_play_connect_tone_now(void)
{
	char file_name[128] = {0};
    int volume = divoom_main_get_tone_volume();
	divoom_voice_read_get_filename(DIVOOM_VOICE_ID_CONNECTED, file_name);
#ifdef DIVOOM_CONNECT_TONE_VOL_PERCENT
    volume = volume * DIVOOM_CONNECT_TONE_VOL_PERCENT / 100;
#endif
	divoom_local_music_ctrl_tone_play(file_name,volume, NULL, 0);
	return 1;
}

static void divoom_bt_music_play_connect_tone_if_allowed(int connected_event)
{
	if (divoom_bt_music_skip_connect_tone())
	{
		divoom_print("bt music: skip connect tone menu=%d\n",
			     divoom_menu_control_get_active_menu_id());
		return;
	}

	if (connected_event)
	{
		if (divoom_bt_music_should_defer_connect_tone())
		{
			s_defer_connect_tone_for_poweron = 1;
			divoom_print("bt music: defer connect tone until poweron video over (0x49)\n");
			return;
		}
	}
	else if (divoom_bt_music_should_defer_connect_tone())
	{
		s_defer_connect_tone_for_poweron = 0;
		return;
	}

	divoom_bt_music_play_connect_tone_now();
}

/* 开机已连上不弹「已连接」；断开始终弹。 */
static void divoom_bt_music_show_connect_tip(int connected)
{
	if (divoom_product_test_is_active())
		return;
	if (divoom_bt_music_skip_connect_tone())
		return;
	if (connected) {
		if (s_bt_skip_boot_connect_tip) {
			s_bt_skip_boot_connect_tip = 0;
			return;
		}
		divoom_lvgl_top_audio_tip_menu_init(DIVOOM_AUDIO_TIP_BT_CONNECTED);
	} else {
		s_bt_skip_boot_connect_tip = 0;
		divoom_lvgl_top_audio_tip_menu_init(DIVOOM_AUDIO_TIP_BT_DISCONNECTED);
	}
}

void divoom_bt_music_play_deferred_connect_tone_if_any(void)
{
	if (!s_defer_connect_tone_for_poweron)
		return;
	if (!s_a2dp_connected)
		return;
	divoom_print("bt music: play deferred connect tone after poweron video\n");
	if (!divoom_bt_music_play_connect_tone_now())
	{
		divoom_print("bt music: deferred connect tone skipped, keep defer\n");
		return;
	}
	s_defer_connect_tone_for_poweron = 0;
	s_poweron_connect_tone_done = 1;
}

void divoom_bt_music_sync_poweron_connect_tone_defer(void)
{
	if (s_poweron_connect_tone_done)
		return;
	if (!divoom_lvgl_poweron_should_defer_connect_tone())
		return;
	if (!s_a2dp_connected)
		return;
	s_defer_connect_tone_for_poweron = 1;
	divoom_print("bt music: defer connect tone (already connected before poweron video)\n");
}

void divoom_bt_music_handle_connect_status(uint8 status)
{
	switch (status)
	{
#ifdef DIVOOM_TIMES_VIBE
		case DIVOOM_BT_DISCONNECTED:
			divoom_debug("BTMG_A2DP_SINK_DISCONNECTED");
            s_bt_a2dp_playing = 0;
			if (s_a2dp_connected == 0) {
				s_bt_skip_boot_connect_tip = 0;
				break;
			}
			s_a2dp_connected = 0;
			divoom_bt_bip_on_bt_down();
			divoom_bt_music_play_connect_tone_if_allowed(0);
			divoom_bt_music_show_connect_tip(0);
			break;

		case DIVOOM_BT_CONNECTED:
			s_bt_a2dp_playing = 0;
			divoom_debug("BTMG_A2DP_SINK_CONNECTED");
			if (s_a2dp_connected == 1)
				break;
			s_a2dp_connected = 1;
			divoom_bt_music_fetch_connected_phone_info_from_jl();
			divoom_bt_music_play_connect_tone_if_allowed(1);
			divoom_bt_music_show_connect_tip(1);
			break;
#else
		case BTMG_A2DP_SINK_DISCONNECTED:
			if (s_a2dp_connected == 0) {
				s_bt_skip_boot_connect_tip = 0;
				break;
			}
            s_bt_a2dp_playing = 0;
			divoom_debug("BTMG_A2DP_SINK_DISCONNECTED");
			s_a2dp_connected = 0;
			divoom_bt_music_play_connect_tone_if_allowed(0);
			divoom_bt_music_show_connect_tip(0);
			break;

		case BTMG_A2DP_SINK_CONNECTED:
			if (s_a2dp_connected == 1)
				break;
			s_a2dp_connected = 1;
            s_bt_a2dp_playing = 0;
			divoom_debug("BTMG_A2DP_SINK_CONNECTED");
			divoom_bt_music_play_connect_tone_if_allowed(1);
			divoom_bt_music_show_connect_tip(1);
			break;
#endif
		default:
			/* CONNECTING / DISCONNECTING / FAILED 等均视为未连上 */
			s_a2dp_connected = 0;
            s_bt_a2dp_playing = 0;
			break;
	}
}

// 添加或更新设备信息到数组
static int divoom_bt_music_add_or_update_device(const DeviceInfo *info)
{
    if (!info || strlen(info->bt_addr) != 17)
    {
        divoom_print("Invalid device info\n");
        return -1;
    }

    // 查找是否已存在该设备（根据地址）
    int i;
    for (i = 0; i < g_bt_device_manager.count; i++)
    {
        if (strcmp(g_bt_device_manager.devices[i].bt_addr, info->bt_addr) == 0)
        {
            // 更新现有设备信息
            strncpy(g_bt_device_manager.devices[i].device_name, info->device_name, 
                    MAX_DEVICE_NAME_LEN - 1);
            g_bt_device_manager.devices[i].device_name[MAX_DEVICE_NAME_LEN - 1] = '\0';
            divoom_print("Updated device: %s - %s\n", info->bt_addr, info->device_name);
            return 0;
        }
    }

    // 新设备：未满则追加；已满则淘汰最旧（下标 0）再追加到末尾
    if (g_bt_device_manager.count < MAX_BT_DEVICE_COUNT)
    {
        i = g_bt_device_manager.count;
        g_bt_device_manager.count++;
    }
    else
    {
        divoom_print("Device list full, evict oldest: %s - %s\n",
                     g_bt_device_manager.devices[0].bt_addr,
                     g_bt_device_manager.devices[0].device_name);
        for (i = 0; i < g_bt_device_manager.count - 1; i++)
        {
            memcpy(&g_bt_device_manager.devices[i],
                   &g_bt_device_manager.devices[i + 1],
                   sizeof(DeviceInfo));
        }
        i = g_bt_device_manager.count - 1;
    }

    strncpy(g_bt_device_manager.devices[i].bt_addr,
            info->bt_addr, sizeof(g_bt_device_manager.devices[i].bt_addr) - 1);
    g_bt_device_manager.devices[i].bt_addr[17] = '\0';

    strncpy(g_bt_device_manager.devices[i].device_name,
            info->device_name, MAX_DEVICE_NAME_LEN - 1);
    g_bt_device_manager.devices[i].device_name[MAX_DEVICE_NAME_LEN - 1] = '\0';

    divoom_print("Added new device: %s - %s\n", info->bt_addr, info->device_name);
    return 0;
}

// 保存设备信息到JSON文件
static int divoom_bt_music_save_device_info_to_file(void)
{
    divoom_cJSON *root = NULL;
    divoom_cJSON *device_array = NULL;
    divoom_cJSON *device_item = NULL;
    FILE *fp = NULL;
    char *json_string = NULL;
    int i;

    // 创建JSON根对象
    root = divoom_cJSON_CreateObject();
    if (!root)
    {
        divoom_print("Failed to create JSON root\n");
        return -1;
    }

    // 创建设备数组
    device_array = divoom_cJSON_CreateArray();
    if (!device_array)
    {
        divoom_print("Failed to create device array\n");
        divoom_cJSON_Delete(root);
        return -1;
    }

    // 添加设备数量
    divoom_cJSON_AddNumberToObject(root, "count", g_bt_device_manager.count);

    // 遍历设备数组，添加到JSON
    for (i = 0; i < g_bt_device_manager.count; i++)
    {
        device_item = divoom_cJSON_CreateObject();
        if (!device_item)
        {
            continue;
        }

        divoom_cJSON_AddStringToObject(device_item, "bt_addr", g_bt_device_manager.devices[i].bt_addr);
        divoom_cJSON_AddStringToObject(device_item, "device_name", g_bt_device_manager.devices[i].device_name);
        
        divoom_cJSON_AddItemToArray(device_array, device_item);
    }

    divoom_cJSON_AddItemToObject(root, "devices", device_array);

    // 转换为JSON字符串
    json_string = divoom_cJSON_Print(root);
    if (!json_string)
    {
        divoom_print("Failed to print JSON\n");
        divoom_cJSON_Delete(root);
        return -1;
    }

    // 写入文件
    fp = divoom_port_file_open(BT_DEVICE_INFO_SAVE_PATH, "w");
    if (!fp)
    {
        divoom_print("Failed to open file for writing: %s\n", BT_DEVICE_INFO_SAVE_PATH);
        divoom_system_free(json_string);
        divoom_cJSON_Delete(root);
        return -1;
    }

    fprintf(fp, "%s", json_string);
    divoom_port_file_close(fp);

    divoom_print("Saved %d device(s) to %s\n", g_bt_device_manager.count, BT_DEVICE_INFO_SAVE_PATH);

    // 清理资源
    divoom_system_free(json_string);
    divoom_cJSON_Delete(root);

    return 0;
}

// 从JSON文件读取设备信息
static int divoom_bt_music_load_device_info_from_file(void)
{
    FILE *fp = NULL;
    long file_size;
    char *file_content = NULL;
    divoom_cJSON *root = NULL;
    divoom_cJSON *device_array = NULL;
    divoom_cJSON *device_item = NULL;
    int i, count;
    int ret = -1;

    // 打开文件
    fp = divoom_port_file_open(BT_DEVICE_INFO_SAVE_PATH, "r");
    if (!fp)
    {
        divoom_print("Device info file not found: %s (will create new one)\n", BT_DEVICE_INFO_SAVE_PATH);
        return 0;  // 文件不存在不算错误，返回0
    }

    // 获取文件大小
    divoom_port_file_seek(fp, 0, SEEK_END);
    file_size = divoom_port_file_tell(fp);
    divoom_port_file_seek(fp, 0, SEEK_SET);

    if (file_size <= 0)
    {
        divoom_print("Device info file is empty\n");
        divoom_port_file_close(fp);
        return 0;
    }

    // 读取文件内容
    file_content = (char *)divoom_system_malloc(file_size + 1);
    if (!file_content)
    {
        divoom_print("Failed to allocate memory for file content\n");
        divoom_port_file_close(fp);
        return -1;
    }

    if (divoom_port_file_read(file_content, 1, file_size, fp) != file_size)
    {
        divoom_print("Failed to read file content\n");
        divoom_system_free(file_content);
        divoom_port_file_close(fp);
        return -1;
    }
    file_content[file_size] = '\0';
    divoom_port_file_close(fp);

    // 解析JSON
    root = divoom_cJSON_Parse(file_content);
    if (!root)
    {
        divoom_print("Failed to parse JSON\n");
        divoom_system_free(file_content);
        return -1;
    }

    // 获取设备数量
    count = divoom_cJSON_GetObjectInt(root, "count");
    if (count < 0 || count > MAX_BT_DEVICE_COUNT)
    {
        divoom_print("Invalid device count: %d\n", count);
        divoom_cJSON_Delete(root);
        divoom_system_free(file_content);
        return -1;
    }

    // 获取设备数组
    device_array = divoom_cJSON_GetObjectItem(root, "devices");
    if (!device_array)
    {
        divoom_print("Invalid device array in JSON\n");
        divoom_cJSON_Delete(root);
        divoom_system_free(file_content);
        return -1;
    }

    // 清空现有设备列表
    g_bt_device_manager.count = 0;

    // 读取设备信息
    for (i = 0; i < count && i < MAX_BT_DEVICE_COUNT; i++)
    {
        device_item = divoom_cJSON_GetArrayItem(device_array, i);
        if (!device_item)
        {
            continue;
        }

        divoom_cJSON *addr_item = divoom_cJSON_GetObjectItem(device_item, "bt_addr");
        divoom_cJSON *name_item = divoom_cJSON_GetObjectItem(device_item, "device_name");

        if (addr_item && name_item)
        {
            const char *addr = addr_item->valuestring;
            const char *name = name_item->valuestring;

            if (addr && name && strlen(addr) == 17)
            {
                strncpy(g_bt_device_manager.devices[g_bt_device_manager.count].bt_addr, 
                        addr, sizeof(g_bt_device_manager.devices[g_bt_device_manager.count].bt_addr) - 1);
                g_bt_device_manager.devices[g_bt_device_manager.count].bt_addr[17] = '\0';

                strncpy(g_bt_device_manager.devices[g_bt_device_manager.count].device_name, 
                        name, MAX_DEVICE_NAME_LEN - 1);
                g_bt_device_manager.devices[g_bt_device_manager.count].device_name[MAX_DEVICE_NAME_LEN - 1] = '\0';

                g_bt_device_manager.count++;
            }
        }
    }

    divoom_print("Loaded %d device(s) from %s\n", g_bt_device_manager.count, BT_DEVICE_INFO_SAVE_PATH);

    // 清理资源
    divoom_cJSON_Delete(root);
    divoom_system_free(file_content);

    return 0;
}

// 解析并处理设备信息（地址+名称，支持中文UTF-8）
static void divoom_bt_music_handle_device_info(const DeviceInfo *info) 
{
    if (!info) 
	{
        divoom_print("device info is NULL\n");
        return;
    }

    divoom_print("蓝牙设备信息:\n");
    divoom_print("  地址: %s\n", info->bt_addr);
    divoom_print("  名称: %s\n", info->device_name);
    
	//保存当前连接的设备信息

	memset(&pdivoom_bt_music_ctrl->current_blue_info, 0, sizeof(pdivoom_bt_music_ctrl->current_blue_info));
	strcpy(pdivoom_bt_music_ctrl->current_blue_info.bt_addr, info->bt_addr);
	strcpy(pdivoom_bt_music_ctrl->current_blue_info.device_name, info->device_name);

    // 添加到设备列表
    if (divoom_bt_music_add_or_update_device(info) == 0)
    {
        // 保存到文件
        divoom_bt_music_save_device_info_to_file();
    }
}

#ifdef DIVOOM_TIMES_VIBE
/*
 * DM_BTCORE_BT_INFO(0x05) 与 BtCore 串口协议对齐：
 * - GET 应答：bt_name/br/ble(26*3) + ssp_switch(1) + ssp_secre(5) + addr(6) = 90
 * - SET 载荷：ctrl(1) + 同上前 84 字节 + Reset_flag(1)
 *   ctrl: 1=设置 0=获取；Reset_flag: 1=重置蓝牙地址，0=无操作
 * 改名/设 PIN 必须 Reset_flag=0，勿把 MAC 字节误当 Reset_flag。
 */
#define DIVOOM_BTCORE_BT_INFO_BODY_LEN (26 + 26 + 26 + 1 + 5 + 6)
#define DIVOOM_BTCORE_BT_INFO_SET_COMMON_LEN (26 + 26 + 26 + 1 + 5)
#define DIVOOM_BTCORE_BT_INFO_SET_PAYLOAD_LEN (1 + DIVOOM_BTCORE_BT_INFO_SET_COMMON_LEN + 1)

static int divoom_bt_music_clear_blue_addr(void);

static int divoom_bt_music_is_auto_reconnect_enabled(void)
{
	Pdivoom_sys_setting_info cfg = divoom_main_get_system_config();

	if (cfg == NULL)
		return 1;

	return cfg->bt_auto_connect ? 1 : 0;
}

static void divoom_bt_music_reject_unwanted_connect(void)
{
	if (divoom_bt_music_is_auto_reconnect_enabled())
		return;

	divoom_print("bt auto reconnect disabled, reject incoming connect\n");
	divoom_bt_music_disconnect_bt();
}

void divoom_bt_music_handle_bt_connect_status(uint8 status)
{
	if (status && !divoom_bt_music_is_auto_reconnect_enabled())
		divoom_bt_music_reject_unwanted_connect();
}

static void divoom_bt_music_fill_bt_name_fallback(void)
{
	const char *saved_name;

	if (!pdivoom_bt_music_ctrl)
		return;

	if (pdivoom_bt_music_ctrl->bt_info.bt_name[0] != '\0')
		return;

	saved_name = divoom_main_get_blue_name();
	if (saved_name == NULL || saved_name[0] == '\0')
		return;

	strncpy(pdivoom_bt_music_ctrl->bt_info.bt_name, saved_name, 25);
	pdivoom_bt_music_ctrl->bt_info.bt_name[25] = '\0';
	strncpy(pdivoom_bt_music_ctrl->bt_info.bt_br_name, saved_name, 25);
	pdivoom_bt_music_ctrl->bt_info.bt_br_name[25] = '\0';
	strncpy(pdivoom_bt_music_ctrl->bt_info.bt_ble_name, saved_name, 25);
	pdivoom_bt_music_ctrl->bt_info.bt_ble_name[25] = '\0';
}

static int divoom_bt_music_ensure_bt_info_ready(void)
{
	uint8_t cmd_data = 0;
	uint8_t reply[96];
	uint16_t reply_len = 0;

	if (!pdivoom_bt_music_ctrl || !divoom_btcore_uart_cmd_is_ready())
		return -1;

	if (pdivoom_bt_music_ctrl->bt_info.bt_name[0] != '\0')
		return 0;

	if (divoom_btcore_uart_cmd_send_wait(DM_BTCORE_BT_INFO, &cmd_data, 1,
					     reply, sizeof(reply), &reply_len) != 0)
		return -1;

	if (divoom_bt_music_parse_device_info(reply, reply_len) != 0)
		return -1;

	return 0;
}

int divoom_bt_music_request_bt_info(void)
{
    uint8_t cmd_data = 0;
    divoom_btcore_uart_cmd_send(DM_BTCORE_BT_INFO, &cmd_data, 1);
    return 0;
}

int divoom_bt_music_request_bt_auto_connect_info(void)
{
    uint8_t cmd_data = 0;
    divoom_btcore_uart_cmd_send(DM_BTCORE_BT_AUTO_CONNECT, &cmd_data, 1);
    return 0;
}

int divoom_bt_music_parse_bt_auto_connect_info(const uint8_t *raw, size_t raw_len)
{
	if (!pdivoom_bt_music_ctrl || !raw || raw_len < 1u)
		return -1;

	pdivoom_bt_music_ctrl->bt_info.auto_connect = raw[0] ? 1 : 0;
	pdivoom_bt_music_ctrl->bt_info.auto_connect_valid = 1;
	divoom_print("bt auto connect from jl: %d\n", pdivoom_bt_music_ctrl->bt_info.auto_connect);
	return 0;
}

// 将ParsedBtDeviceInfo转换为常用DeviceInfo（用于保存/显示等）
// addr格式化为字符串
void divoom_bt_music_parsed_info_to_device_info(const ParsedBtDeviceInfo *parsed, DeviceInfo *dev)
{
    if (!parsed || !dev) return;
    // 以蓝牙地址字符串格式填充（如："AB:CD:EF:12:34:56"）
    snprintf(dev->bt_addr, sizeof(dev->bt_addr), "%02X:%02X:%02X:%02X:%02X:%02X",
             (unsigned char)parsed->addr[0], (unsigned char)parsed->addr[1], (unsigned char)parsed->addr[2],
             (unsigned char)parsed->addr[3], (unsigned char)parsed->addr[4], (unsigned char)parsed->addr[5]);
    // 选择一个合适的名字（以经典/BR优先，用户自定义可能首选bt_name）
    strncpy(dev->device_name, parsed->bt_name, MAX_DEVICE_NAME_LEN - 1);
    dev->device_name[MAX_DEVICE_NAME_LEN - 1] = '\0';
}

static int divoom_bt_music_parse_jl_bt_info_wire(ParsedBtDeviceInfo *info,
						 const uint8_t *raw, size_t raw_len)
{
	size_t offset = 0;

	if (info == NULL || raw == NULL || raw_len < DIVOOM_BTCORE_BT_INFO_BODY_LEN)
		return -1;

	memcpy(info->bt_name, raw + offset, sizeof(info->bt_name));
	info->bt_name[sizeof(info->bt_name) - 1] = '\0';
	offset += sizeof(info->bt_name);
	memcpy(info->bt_br_name, raw + offset, sizeof(info->bt_br_name));
	info->bt_br_name[sizeof(info->bt_br_name) - 1] = '\0';
	offset += sizeof(info->bt_br_name);
	memcpy(info->bt_ble_name, raw + offset, sizeof(info->bt_ble_name));
	info->bt_ble_name[sizeof(info->bt_ble_name) - 1] = '\0';
	offset += sizeof(info->bt_ble_name);
	info->ssp_switch = raw[offset++];
	memcpy(info->ssp_secret, raw + offset, sizeof(info->ssp_secret));
	offset += sizeof(info->ssp_secret);
	memcpy(info->addr, raw + offset, sizeof(info->addr));
	return 0;
}

static void divoom_bt_music_sync_jl_bt_info_to_ctrl(const ParsedBtDeviceInfo *info)
{
	if (!pdivoom_bt_music_ctrl || info == NULL)
		return;

	memcpy(&pdivoom_bt_music_ctrl->bt_info, info, DIVOOM_BTCORE_BT_INFO_BODY_LEN);
	strncpy(pdivoom_bt_music_ctrl->local_adapter_name, info->bt_name,
		MAX_DEVICE_NAME_LEN - 1);
	pdivoom_bt_music_ctrl->local_adapter_name[MAX_DEVICE_NAME_LEN - 1] = '\0';
}

// 从原始回复数据中解析设备信息
int divoom_bt_music_parse_device_info(const uint8_t *raw, size_t raw_len)
{
	ParsedBtDeviceInfo info;

	if (divoom_bt_music_parse_jl_bt_info_wire(&info, raw, raw_len) != 0)
		return -1;

	divoom_bt_music_sync_jl_bt_info_to_ctrl(&info);
	divoom_print("Device blue name: %s\n", info.bt_name);
	divoom_print("Device bt pin: enable=%d, pin=%.4s\n",
		     info.ssp_switch, info.ssp_secret);
    return 0;
}

static size_t divoom_bt_music_utf8_valid_prefix_len(const char *name, size_t len)
{
	size_t i = 0;

	while (i < len)
	{
		unsigned char b = (unsigned char)name[i];
		size_t need;
		size_t j;

		if (b == '\0')
			break;
		if (b < 0x80u)
		{
			i++;
			continue;
		}
		if ((b & 0xE0u) == 0xC0u)
			need = 2;
		else if ((b & 0xF0u) == 0xE0u)
			need = 3;
		else if ((b & 0xF8u) == 0xF0u)
			need = 4;
		else
			break;

		if (i + need > len)
			break;

		for (j = 1; j < need; j++)
		{
			if ((((unsigned char)name[i + j]) & 0xC0u) != 0x80u)
				return i;
		}
		i += need;
	}

	return i;
}

static size_t divoom_bt_music_utf8_safe_trim(char *name, size_t len)
{
	size_t i;

	for (i = 0; i < len; i++)
	{
		if (name[i] == '\0')
			return i;
	}

	return divoom_bt_music_utf8_valid_prefix_len(name, len);
}

static void divoom_bt_music_log_jl_name_bytes(const char *tag, const uint8_t *bytes, size_t len)
{
	char hex[96];
	size_t i;
	size_t pos = 0;
	size_t dump_len = len;

	if (tag == NULL || bytes == NULL)
		return;
	if (dump_len > 26u)
		dump_len = 26u;

	for (i = 0; i < dump_len && pos + 3u < sizeof(hex); i++)
		pos += (size_t)snprintf(hex + pos, sizeof(hex) - pos, "%02x ", bytes[i]);

	divoom_print("%s len=%u hex:%s\n", tag, (unsigned)len, hex);
}

static size_t divoom_bt_music_sanitize_jl_name(char *name, size_t cap)
{
	size_t len;

	if (name == NULL || cap == 0u)
		return 0;

	name[cap - 1] = '\0';
	len = strlen(name);
	if (len >= cap)
		len = cap - 1;
	len = divoom_bt_music_utf8_safe_trim(name, len);
	while (len > 0 && (unsigned char)name[len - 1] <= ' ')
		len--;
	name[len] = '\0';
	return len;
}

static int divoom_bt_music_jl_name_field_match(const char *expect, const char *got)
{
	char got_norm[26];

	if (expect == NULL || got == NULL)
		return 0;

	memset(got_norm, 0, sizeof(got_norm));
	memcpy(got_norm, got, sizeof(got_norm) - 1u);
	divoom_bt_music_sanitize_jl_name(got_norm, sizeof(got_norm));
	return strncmp(expect, got_norm, sizeof(got_norm)) == 0;
}

int divoom_bt_music_parse_connected_phone_info(const uint8_t *raw, size_t raw_len)
{
	DeviceInfo info = {0};
	size_t name_len, safe_len;
	int i, addr_empty = 1;

	if (!raw || raw_len < DIVOOM_BTCORE_BT_ADDR_BIN_LEN)
	{
		divoom_print("connected phone info too short: %zu\n", raw_len);
		return -1;
	}

	if (!pdivoom_bt_music_ctrl)
	{
		if (raw_len <= DIVOOM_BT_CACHED_PHONE_INFO_MAX)
		{
			memcpy(s_cached_phone_info, raw, raw_len);
			s_cached_phone_info_len = (uint16_t)raw_len;
			divoom_print("cache connected phone info len=%zu (bt init pending)\n", raw_len);
		}
		else
		{
			divoom_print("cache connected phone info too long: %zu\n", raw_len);
		}
		return 0;
	}

	for (i = 0; i < DIVOOM_BTCORE_BT_ADDR_BIN_LEN; i++)
	{
		if (raw[i] != 0)
			addr_empty = 0;
	}
	if (addr_empty)
	{
		divoom_print("connected phone info empty addr, len=%zu\n", raw_len);
		return -1;
	}

	snprintf(info.bt_addr, sizeof(info.bt_addr), "%02X:%02X:%02X:%02X:%02X:%02X",
		 (unsigned)raw[0], (unsigned)raw[1], (unsigned)raw[2],
		 (unsigned)raw[3], (unsigned)raw[4], (unsigned)raw[5]);

	if (raw_len <= DIVOOM_BTCORE_BT_ADDR_BIN_LEN)
	{
		divoom_print("connected phone info no name, addr=%s len=%zu\n",
			     info.bt_addr, raw_len);
		return -1;
	}

	name_len = raw_len - DIVOOM_BTCORE_BT_ADDR_BIN_LEN;
	if (name_len > DIVOOM_BTCORE_PHONE_NAME_LEN)
		name_len = DIVOOM_BTCORE_PHONE_NAME_LEN;
	if (name_len >= MAX_DEVICE_NAME_LEN)
		name_len = MAX_DEVICE_NAME_LEN - 1;

	memcpy(info.device_name, raw + DIVOOM_BTCORE_BT_ADDR_BIN_LEN, name_len);
	info.device_name[name_len] = '\0';
	safe_len = divoom_bt_music_utf8_safe_trim(info.device_name, name_len);
	info.device_name[safe_len] = '\0';

	if (info.device_name[0] == '\0')
	{
		divoom_print("connected phone info no name, addr=%s len=%zu\n",
			     info.bt_addr, raw_len);
		return -1;
	}

	divoom_print("bt_name: %s, bt_addr: %s (len=%zu)\n",
		     info.device_name, info.bt_addr, raw_len);
	divoom_bt_music_handle_device_info(&info);
	return 0;
}

static void divoom_bt_music_apply_cached_phone_info(void)
{
	if (s_cached_phone_info_len == 0 || !pdivoom_bt_music_ctrl)
		return;

	divoom_print("apply cached connected phone info len=%u\n",
		     (unsigned)s_cached_phone_info_len);
	divoom_bt_music_parse_connected_phone_info(s_cached_phone_info, s_cached_phone_info_len);
	s_cached_phone_info_len = 0;
}

int divoom_bt_music_fetch_connected_phone_info_from_jl(void)
{
	uint8_t reply[DIVOOM_BTCORE_CONNECTED_PHONE_INFO_LEN];
	uint16_t reply_len = 0;
	int ret;

	if (!pdivoom_bt_music_ctrl || !divoom_btcore_uart_cmd_is_ready())
		return -1;
#if 1
	divoom_btcore_uart_cmd_send(DM_BTCORE_BT_CONNECTED_PHONE_INFO, NULL, 0);
	return 0;
#else
	ret = divoom_btcore_uart_cmd_send_wait(DM_BTCORE_BT_CONNECTED_PHONE_INFO, NULL, 0,
					       reply, (uint16_t)sizeof(reply), &reply_len);
	if (ret != 0)
	{
		divoom_print("btcore connected phone query send/wait failed\n");
		return -1;
	}
	if (reply_len < DIVOOM_BTCORE_CONNECTED_PHONE_INFO_LEN)
	{
		divoom_print("btcore connected phone query short reply: %u\n", (unsigned)reply_len);
		return -1;
	}

	return divoom_bt_music_parse_connected_phone_info(reply, reply_len);
#endif
}
#endif

/* 蓝牙进程同步的本机显示名（DATA_LOCAL_ADAPTER_NAME，UTF-8） */
static void divoom_bt_music_handle_local_adapter_name(const char *name_utf8)
{
    if (!pdivoom_bt_music_ctrl || !name_utf8)
    {
        return;
    }
    strncpy(pdivoom_bt_music_ctrl->local_adapter_name, name_utf8, MAX_DEVICE_NAME_LEN - 1);
    pdivoom_bt_music_ctrl->local_adapter_name[MAX_DEVICE_NAME_LEN - 1] = '\0';
    divoom_print("本机蓝牙名称（同步）: %s\n", pdivoom_bt_music_ctrl->local_adapter_name);
}

// 计算校验位（简单异或校验，覆盖数据类型+数据内容）
static uint8 divoom_bt_music_calc_checksum(const uint8 *data, uint16 len) 
{
    uint8 checksum = 0;
    if (!data) {
        return 0;
    }
    for (uint16 i = 0; i < len; i++) 
    {
        checksum ^= data[i];
    }
    return checksum;
}

// 发送播放控制指令（数据类型0x02）
// 数据头(0xAA)+数据长度+数据类型+数据内容+校验位+数据尾(0x55)
static void divoom_bt_music_play_ctrl(int fd, uint8 cmd_type, uint8 cmd) 
{
    if (fd < 0) 
	{
        return;
    }

    // 使用栈上固定大小缓冲区，避免动态分配
    // 格式: header(1) + len(2) + type(1) + content(1) + checksum(1) + tail(1) = 7
    uint8 buffer[7];
    uint16 content_len = 1;
    uint16 pos = 0;

    // 填充消息
    buffer[pos++] = DATA_HEADER;
    buffer[pos++] = (content_len >> 8) & 0xFF;
    buffer[pos++] = content_len & 0xFF;
    buffer[pos++] = cmd_type;  // 命令类型
    buffer[pos++] = cmd;       // 命令

    // 校验位（数据类型+数据内容）
    buffer[pos++] = divoom_bt_music_calc_checksum(buffer + 3, 1 + content_len);
    buffer[pos++] = DATA_TAIL;

    // 写入管道
    ssize_t written = write(fd, buffer, pos);
    if (written != pos) 
    {
        divoom_print("write play ctrl failed: %s\n", strerror(errno));
    } 
}

// 发送连接设备命令（数据类型0x08）
// 数据头(0xAA)+数据长度+数据类型+蓝牙地址(18字节)+校验位+数据尾(0x55)
static int divoom_bt_music_send_connect_device(int fd, const char *bt_addr)
{
    if (fd < 0 || !bt_addr || strlen(bt_addr) != 17)
    {
        divoom_print("Invalid parameters for connect device\n");
        return -1;
    }

    // 计算数据长度：蓝牙地址18字节（包含\0）
    uint16_t content_len = 18;
    // 总长度：头(1) + 长度(2) + 类型(1) + 地址(18) + 校验(1) + 尾(1) = 24
    uint16_t total_len = 1 + 2 + 1 + content_len + 1 + 1;
    uint8_t buffer[24];
    uint16_t pos = 0;

    // 填充消息
    buffer[pos++] = DATA_HEADER;
    buffer[pos++] = (content_len >> 8) & 0xFF;
    buffer[pos++] = content_len & 0xFF;
    buffer[pos++] = DATA_CONNECT_DEVICE;  // 数据类型

    // 填充蓝牙地址
    strncpy((char *)(buffer + pos), bt_addr, 17);
    buffer[pos + 17] = '\0';
    pos += 18;

    // 计算校验位（数据类型+数据内容）
    buffer[pos++] = divoom_bt_music_calc_checksum(buffer + 3, 1 + content_len);
    buffer[pos++] = DATA_TAIL;

    // 写入管道
    ssize_t written = write(fd, buffer, pos);
    if (written != pos) 
    {
        divoom_print("write connect device failed: %s\n", strerror(errno));
        return -1;
    }

    divoom_print("Sent connect device command: %s\n", bt_addr);
    return 0;
}

// 发送取消配对设备命令（数据类型0x09）
// 数据头(0xAA)+数据长度+数据类型+蓝牙地址(18字节)+校验位+数据尾(0x55)
static int divoom_bt_music_send_unpair_device(int fd, const char *bt_addr)
{
    if (fd < 0 || !bt_addr || strlen(bt_addr) != 17)
    {
        divoom_print("Invalid parameters for unpair device\n");
        return -1;
    }

    // 计算数据长度：蓝牙地址18字节（包含\0）
    uint16_t content_len = 18;
    // 总长度：头(1) + 长度(2) + 类型(1) + 地址(18) + 校验(1) + 尾(1) = 24
    uint16_t total_len = 1 + 2 + 1 + content_len + 1 + 1;
    uint8_t buffer[24];
    uint16_t pos = 0;

    // 填充消息
    buffer[pos++] = DATA_HEADER;
    buffer[pos++] = (content_len >> 8) & 0xFF;
    buffer[pos++] = content_len & 0xFF;
    buffer[pos++] = DATA_UNPAIR_DEVICE;  // 数据类型

    // 填充蓝牙地址
    strncpy((char *)(buffer + pos), bt_addr, 17);
    buffer[pos + 17] = '\0';
    pos += 18;

    // 计算校验位（数据类型+数据内容）
    buffer[pos++] = divoom_bt_music_calc_checksum(buffer + 3, 1 + content_len);
    buffer[pos++] = DATA_TAIL;

    // 写入管道
    ssize_t written = write(fd, buffer, pos);
    if (written != pos) 
    {
        divoom_print("write unpair device failed: %s\n", strerror(errno));
        return -1;
    }

    divoom_print("Sent unpair device command: %s\n", bt_addr);
    return 0;
}

// 发送禁用/启用开机自动重连命令（数据类型0x0A）
// 数据头(0xAA)+数据长度+数据类型+标志(1字节，0=启用，1=禁用)+校验位+数据尾(0x55)
static int divoom_bt_music_send_disable_boot_connect(int fd, int disable)
{
    if (fd < 0)
    {
        divoom_print("Invalid file descriptor for disable boot connect\n");
        return -1;
    }

    // 计算数据长度：标志1字节
    uint16_t content_len = 1;
    // 总长度：头(1) + 长度(2) + 类型(1) + 标志(1) + 校验(1) + 尾(1) = 7
    uint8_t buffer[7];
    uint16_t pos = 0;

    // 填充消息
    buffer[pos++] = DATA_HEADER;
    buffer[pos++] = (content_len >> 8) & 0xFF;
    buffer[pos++] = content_len & 0xFF;
    buffer[pos++] = DATA_DISABLE_BOOT_CONNECT;  // 数据类型
    buffer[pos++] = (disable != 0) ? 1 : 0;     // 标志：0=启用，1=禁用

    // 计算校验位（数据类型+数据内容）
    buffer[pos++] = divoom_bt_music_calc_checksum(buffer + 3, 1 + content_len);
    buffer[pos++] = DATA_TAIL;

    // 写入管道
    ssize_t written = write(fd, buffer, pos);
    if (written != pos) 
    {
        divoom_print("write disable boot connect failed: %s\n", strerror(errno));
        return -1;
    }

    divoom_print("Sent disable boot connect command: %s\n", disable ? "DISABLE" : "ENABLE");
    return 0;
}

// 发送设置蓝牙配对密码命令（数据类型0x0D）
// 数据头(0xAA)+数据长度+数据类型+enable(1字节)+pin(4字节)+校验位+数据尾(0x55)
// pin4 必须为4位数字字符串，如 "1234"
static int divoom_bt_music_send_set_bt_pin(int fd, int enable, const char *pin4)
{
    if (fd < 0)
    {
        divoom_print("Invalid file descriptor for set BT pin\n");
        return -1;
    }
    if (pin4 == NULL || strlen(pin4) < 4)
    {
        divoom_print("Invalid pin: need 4 digits\n");
        return -1;
    }
    int i;
    for (i = 0; i < 4; i++)
    {
        if (pin4[i] < '0' || pin4[i] > '9')
        {
            divoom_print("Invalid pin: must be 4 digits\n");
            return -1;
        }
    }

    uint16_t content_len = 5;  // 1 byte enable + 4 bytes pin
    uint8_t buffer[1 + 2 + 1 + 5 + 1 + 1];
    uint16_t pos = 0;

    buffer[pos++] = DATA_HEADER;
    buffer[pos++] = (content_len >> 8) & 0xFF;
    buffer[pos++] = content_len & 0xFF;
    buffer[pos++] = DATA_SET_BT_PIN;
    buffer[pos++] = (enable != 0) ? 1 : 0;
    memcpy(buffer + pos, pin4, 4);
    pos += 4;

    buffer[pos++] = divoom_bt_music_calc_checksum(buffer + 3, 1 + content_len);
    buffer[pos++] = DATA_TAIL;

    ssize_t written = write(fd, buffer, pos);
    if (written != pos)
    {
        divoom_print("write set BT pin failed: %s\n", strerror(errno));
        return -1;
    }

    divoom_print("Sent set BT pin command: enable=%d, pin=%.4s\n", enable, pin4);
    return 0;
}

// 发送设置本机蓝牙名称（数据类型 0x0F）：UTF-8，含中文，含结尾 NUL
static int divoom_bt_music_send_set_adapter_name(int fd, const char *name_utf8)
{
    if (fd < 0 || !name_utf8)
    {
        divoom_print("set adapter name: invalid param\n");
        return -1;
    }
    size_t len = strlen(name_utf8);
    if (len >= MAX_DEVICE_NAME_LEN)
    {
        len = MAX_DEVICE_NAME_LEN - 1;
    }
    uint16_t content_len = (uint16_t)(len + 1);
    uint16_t total_len = (uint16_t)(1 + 2 + 1 + content_len + 1 + 1);
    uint8_t buffer[1 + 2 + 1 + MAX_DEVICE_NAME_LEN + 1 + 1];
    if (total_len > sizeof(buffer))
    {
        return -1;
    }
    uint16_t pos = 0;
    buffer[pos++] = DATA_HEADER;
    buffer[pos++] = (content_len >> 8) & 0xFF;
    buffer[pos++] = content_len & 0xFF;
    buffer[pos++] = DATA_SET_ADAPTER_NAME;
    memcpy(buffer + pos, name_utf8, len);
    buffer[pos + len] = '\0';
    pos += content_len;
    buffer[pos++] = divoom_bt_music_calc_checksum(buffer + 3, (uint16_t)(1 + content_len));
    buffer[pos++] = DATA_TAIL;
    if (write(fd, buffer, pos) != (ssize_t)pos)
    {
        divoom_print("write set adapter name failed: %s\n", strerror(errno));
        return -1;
    }
    divoom_print("Sent set adapter name (UTF-8)\n");
    return 0;
}

// 发送完全退出蓝牙系统命令（数据类型0x0B）
// 数据头(0xAA)+数据长度+数据类型+校验位+数据尾(0x55)
// 注意：此命令不需要数据内容，content_len=0
static int divoom_bt_music_send_exit_bt_system(int fd)
{
    if (fd < 0)
    {
        divoom_print("Invalid file descriptor for exit BT system\n");
        return -1;
    }

    // 计算数据长度：0字节（不需要数据内容）
    uint16_t content_len = 0;
    // 总长度：头(1) + 长度(2) + 类型(1) + 校验(1) + 尾(1) = 6
    uint8_t buffer[6];
    uint16_t pos = 0;

    // 填充消息
    buffer[pos++] = DATA_HEADER;
    buffer[pos++] = (content_len >> 8) & 0xFF;
    buffer[pos++] = content_len & 0xFF;
    buffer[pos++] = DATA_EXIT_BT_SYSTEM;  // 数据类型

    // 计算校验位（只有数据类型，没有数据内容）
    buffer[pos++] = divoom_bt_music_calc_checksum(buffer + 3, 1 + content_len);
    buffer[pos++] = DATA_TAIL;

    // 写入管道
    ssize_t written = write(fd, buffer, pos);
    if (written != pos) 
    {
        divoom_print("write exit BT system failed: %s\n", strerror(errno));
        return -1;
    }

    divoom_print("Sent exit BT system command\n");
    return 0;
}

// 读取并解析一条消息
// 数据头(0xAA)+数据长度+数据类型+数据内容+校验位+数据尾(0x55)
// 返回值: 1=成功, 0=对端关闭需重连, -1=其他错误, -2=暂无数据
static int divoom_bt_music_read_message(int fd) 
{
    uint8 header;
    static uint8 s_content_buf[4096];
    uint8 *content = NULL;
    int ret = -1;

    // 1. 读取数据头
    ssize_t n = read(fd, &header, 1);
    if (n == 0) 
	{
        return 0;  // 对端关闭，需要重连
    }
    if (n < 0) 
	{
        return -1;  // 其他读取错误
    }
    if (n != 1) 
	{
        return -1;  // 读取错误
    }
    if (header != DATA_HEADER) 
	{
        divoom_print("header is error: 0x%02X\n", header);
        return -1;
    }

    // 2. 读取数据长度（2字节）
    uint8 len_buf[2];
    if (read(fd, len_buf, 2) != 2) 
	{
        divoom_print("read data len error\n");
        return -1;
    }
    uint16 content_len = (len_buf[0] << 8) | len_buf[1];
    
    // 长度合法性检查
    if (content_len == 0 || content_len > 4096) 
	{
        divoom_print("invalid content len: %d\n", content_len);
        return -1;
    }

    // 3. 读取数据类型
    uint8 data_type;
    if (read(fd, &data_type, 1) != 1) 
	{
        divoom_print("read data type error\n");
        return -1;
    }

    // 4. 读取数据内容
    if (content_len <= sizeof(s_content_buf))
    {
        content = s_content_buf;
    }
    else
    {
        content = (uint8*)divoom_system_malloc(content_len);
        if (!content) 
        {
            divoom_print("malloc failed\n");
            return -1;
        }
    }
    
    if (read(fd, content, content_len) != (ssize_t)content_len) 
	{
        divoom_print("read content error\n");
        goto cleanup;
    }

    // 5. 读取校验位
    uint8 checksum;
    if (read(fd, &checksum, 1) != 1) 
	{
        divoom_print("read check sum error\n");
        goto cleanup;
    }

    // 6. 校验数据（数据类型+数据内容）
    // 优化：直接计算校验，避免额外分配临时缓冲区
    uint8 calc_check = data_type;  // 先异或数据类型
    for (uint16 i = 0; i < content_len; i++) 
	{
        calc_check ^= content[i];
    }

    if (calc_check != checksum) 
	{
        divoom_print("check error，data wrong (expect:0x%02X, got:0x%02X)\n", checksum, calc_check);
        goto cleanup;
    }

    // 7. 读取数据尾
    uint8 tail;
    if (read(fd, &tail, 1) != 1 || tail != DATA_TAIL) 
	{
        divoom_print("invalid tail\n");
        goto cleanup;
    }

    // 8. 根据数据类型处理
    switch (data_type) 
	{
        case DATA_SONG_INFO:
            if (content_len != sizeof(SongInfo)) 
			{
                divoom_print("song info is not match (expect:%zu, got:%d)\n", sizeof(SongInfo), content_len);
            } 
			else 
			{
                divoom_bt_music_handle_song_info((SongInfo*)content);
            }
            break;
            
        case DATA_PLAY_CTRL:
            if (content_len >= 1) 
			{
                divoom_bt_music_handle_play_ctrl(content[0]);
            }
            break;
            
        case DATA_SET_VOL:
            if (content_len >= 1) 
			{
                divoom_bt_music_handle_play_vol(content[0]);
            }
            break;

		case DATA_CONNECT_STATUS:
			if (content_len >= 1) 
			{
				divoom_bt_music_handle_connect_status(content[0]);
            }
            break;
            
        case DATA_BT_PLAY_REQUEST:
            // 蓝牙进程通知：手机端开始播放，请求切换到蓝牙模式
            divoom_print("BT play request received, notifying upper layer\n");
            if (g_bt_play_request_cb)
            {
                g_bt_play_request_cb();
            }
            break;
            
        case DATA_DEVICE_INFO:
            // 蓝牙设备信息（地址+名称，支持中文UTF-8）
            if (content_len != sizeof(DeviceInfo)) 
			{
                divoom_print("device info size mismatch (expect:%zu, got:%d)\n", sizeof(DeviceInfo), content_len);
            } 
			else 
			{
                divoom_bt_music_handle_device_info((DeviceInfo*)content);
            }
            break;

        case DATA_LOCAL_ADAPTER_NAME:
            if (content_len >= 1 && content_len <= MAX_DEVICE_NAME_LEN)
            {
                char name_buf[MAX_DEVICE_NAME_LEN];
                memcpy(name_buf, content, content_len);
                name_buf[MAX_DEVICE_NAME_LEN - 1] = '\0';
                divoom_bt_music_handle_local_adapter_name(name_buf);
            }
            break;

        case DATA_BT_PCM_STREAM:
            break;
            
        default:
            divoom_print("unknow data type: 0x%02X\n", data_type);
            break;
    }

    ret = 1;
cleanup:
    if (content && content != s_content_buf) 
    {
        divoom_system_free(content);
    }
    return ret;
}

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
void divoom_bt_music_set_play_status(uint8 status)
{
    if (!pdivoom_bt_music_ctrl || pdivoom_bt_music_ctrl->bt_send_handle <= 0) 
	{
        divoom_print("bt music not initialized\n");
        return;
    }

    divoom_bt_music_play_ctrl(pdivoom_bt_music_ctrl->bt_send_handle, DATA_PLAY_CTRL, status);
	if (divoom_music_ctrl_is_playing() && status == BTMG_AVRCP_PAUSE)
		divoom_system_delay_ms(DIVOOM_PAUSE_BT_MUSIC_DELAY_TIME);//等待手机端暂停
}

/*
    0xff 增加音量 
    0xfe 减小音量
    0~100是设定指定音量
*/
void divoom_bt_music_set_play_vol(uint8 value)
{
    if (!pdivoom_bt_music_ctrl /*|| pdivoom_bt_music_ctrl->bt_send_handle <= 0*/) 
	{
        divoom_print("bt music not initialized\n");
        return;
    }
    if (value > 100)
		return;

	divoom_bt_music_mark_sent_volume(pdivoom_bt_music_ctrl->volume, value);
	pdivoom_bt_music_ctrl->volume = value;
	divoom_bt_music_play_ctrl(pdivoom_bt_music_ctrl->bt_send_handle, DATA_SET_VOL, value);
	divoom_main_set_sys_vol(value);
}

void divoom_bt_music_set_play_vol_to_device(uint8 value)
{
    if (!pdivoom_bt_music_ctrl)
        return;
    if (value > 100)
        value = 100;
    pdivoom_bt_music_ctrl->volume = value;
}

/*********************************************************************
  Function:        divoom_bt_music_get_play_vol
  Description:     获取当前蓝牙音乐音量（0-100）
  Input:           none
  Return:          当前音量，未初始化返回 0
**********************************************************************/
uint8 divoom_bt_music_get_play_vol(void)
{
    if (!pdivoom_bt_music_ctrl)
        return 0;
    return pdivoom_bt_music_ctrl->volume;
}

const char *divoom_bt_music_get_song_name(void)
{
	if (!pdivoom_bt_music_ctrl)
		return "";
	return pdivoom_bt_music_ctrl->song_info.song_name;
}

const char *divoom_bt_music_get_song_singer(void)
{
	if (!pdivoom_bt_music_ctrl)
		return "";
	return pdivoom_bt_music_ctrl->song_info.singer;
}

static const char *divoom_bt_music_net_lyric_miss_text(void)
{
	int lang = divoom_main_set_lang_flag(-1); /* -1 只读当前语言，不写配置 */

	if (lang < 0 || lang >= DIVOOM_LVGL_MENU_LANG_END)
		lang = DIVOOM_LVGL_MENU_LANG_ENGLISH;
	return s_net_lyric_miss_text[lang];
}

const char *divoom_bt_music_get_song_lyric(void)
{
	int fetch_state = 0;

	if (!pdivoom_bt_music_ctrl)
		return "";
	divoom_bt_music_net_lyric_refresh();
	if (pdivoom_bt_music_ctrl->song_info.lyric[0])
	{
		s_net_lyric_miss = 0; /* 真歌词到了就撤掉失败提示 */
		return pdivoom_bt_music_ctrl->song_info.lyric;
	}
	/* 手机已在推词时不盖这句。网词确认失败才显示；重拉把 state 置回 0 时标记仍在，避免闪空 */
	if (!s_uart_lyric_valid && s_net_lyric_handle && divoom_lyric_demand_snapshot())
	{
		divoom_lvgl_lyric_word_ctrl_get_fetch_state(s_net_lyric_handle, &fetch_state, NULL);
		if (fetch_state == -1)
			s_net_lyric_miss = 1;
		if (s_net_lyric_miss && fetch_state != 1)
			return divoom_bt_music_net_lyric_miss_text();
	}
	return pdivoom_bt_music_ctrl->song_info.lyric;
}

const char *divoom_bt_music_get_song_prev_lyric(void)
{
	if (!pdivoom_bt_music_ctrl)
		return "";
	divoom_bt_music_net_lyric_refresh();
	return pdivoom_bt_music_ctrl->prev_lyric;
}

void divoom_bt_music_set_play_pre(void)
{
#ifdef DIVOOM_TIMES_VIBE
	uint8_t cmd_data = 0;
	divoom_btcore_uart_cmd_send(DM_BTCORE_MUSIC_LAST_NEXT, &cmd_data, 1);
#else
	if (!pdivoom_bt_music_ctrl || pdivoom_bt_music_ctrl->bt_send_handle <= 0) 
	{
		divoom_print("bt music not initialized\n");
		return;
	}
	
    divoom_bt_music_play_ctrl(pdivoom_bt_music_ctrl->bt_send_handle, DATA_PLAY_CTRL, BTMG_AVRCP_BACKWARD);
#endif
}

void divoom_bt_music_set_play_next(void)
{
#ifdef DIVOOM_TIMES_VIBE
	uint8_t cmd_data = 1;
	divoom_btcore_uart_cmd_send(DM_BTCORE_MUSIC_LAST_NEXT, &cmd_data, 1);
#else
	if (!pdivoom_bt_music_ctrl || pdivoom_bt_music_ctrl->bt_send_handle <= 0) 
	{
		divoom_print("bt music not initialized\n");
		return;
	}
	
    divoom_bt_music_play_ctrl(pdivoom_bt_music_ctrl->bt_send_handle, DATA_PLAY_CTRL, BTMG_AVRCP_FORWARD);
#endif
}

/*********************************************************************
  Function:        divoom_bt_music_set_audio_enable
  Description:     设置蓝牙音频输出使能
                   当切换到非蓝牙模式时，禁用蓝牙音频输出
                   即使手机端播放，蓝牙管理进程也不会输出音频
  Input:           enable  1:启用音频输出  0:禁用音频输出
  Return:          none
**********************************************************************/
void divoom_bt_music_set_audio_enable(uint8 enable)
{
    if (!pdivoom_bt_music_ctrl || pdivoom_bt_music_ctrl->bt_send_handle <= 0) 
    {
        divoom_print("bt music not initialized\n");
        return;
    }
    
    divoom_bt_music_play_ctrl(pdivoom_bt_music_ctrl->bt_send_handle, DATA_AUDIO_OUTPUT_ENABLE, enable);
    divoom_print("bt audio output %s\n", enable ? "enabled" : "disabled");
}

/*********************************************************************
  Function:        divoom_bt_music_register_play_request_cb
  Description:     注册蓝牙播放请求回调
                   当手机端开始播放时，会触发此回调
  Input:           cb - 回调函数
  Return:          none
**********************************************************************/
void divoom_bt_music_register_play_request_cb(DIVOOM_BT_PLAY_REQUEST_CB cb)
{
    g_bt_play_request_cb = cb;
    divoom_print("BT play request callback registered\n");
}

int divoom_bt_music_set_local_adapter_name(const char *name_utf8)
{
    if (!pdivoom_bt_music_ctrl || pdivoom_bt_music_ctrl->bt_send_handle <= 0)
    {
        divoom_print("bt music not initialized\n");
        return -1;
    }
    if (!name_utf8 || name_utf8[0] == '\0')
    {
        divoom_print("set_local_adapter_name: empty name\n");
        return -1;
    }
    return divoom_bt_music_send_set_adapter_name(pdivoom_bt_music_ctrl->bt_send_handle, name_utf8);
}

const char *divoom_bt_music_get_local_adapter_name(void)
{
    if (!pdivoom_bt_music_ctrl)
    {
        return "";
    }
    return pdivoom_bt_music_ctrl->local_adapter_name;
}

// 主任务线程
static void* divoom_bt_music_main_task(void *param)
{
    while (pdivoom_bt_music_ctrl && pdivoom_bt_music_ctrl->run_flag)
    {
		// 尝试打开A2DP接收管道（如果未打开）
		if (pdivoom_bt_music_ctrl->bt_accept_handle <= 0) 
		{
			pdivoom_bt_music_ctrl->bt_accept_handle = open(BT_A2DP_ACCEPTE, O_RDONLY);
			if (pdivoom_bt_music_ctrl->bt_accept_handle > 0) 
			{
				divoom_print("AVRCP task: accept fifo opened\n");
			}
		}

		// 尝试打开A2DP发送管道（如果未打开）
		if (pdivoom_bt_music_ctrl->bt_send_handle <= 0) 
		{
			pdivoom_bt_music_ctrl->bt_send_handle = open(BT_A2DP_SEND, O_WRONLY);
			if (pdivoom_bt_music_ctrl->bt_send_handle > 0) 
			{
				divoom_print("AVRCP task: send fifo opened\n");
			}
		}

		if (pdivoom_bt_music_ctrl->bt_accept_handle > 0 && pdivoom_bt_music_ctrl->bt_send_handle > 0) 
		{
			int ret = divoom_bt_music_read_message(pdivoom_bt_music_ctrl->bt_accept_handle);
	       	if (ret == 1) 
			{
				/* 收到 PCM 时需及时消费，避免 FIFO 堵塞 */
	        } 
			else if (ret < 0) 
			{
				// 其他读取错误，等待后重试（避免高频打印导致额外卡顿）
				divoom_system_delay_ms(100);
			}
			else if (ret == 0)
			{
                divoom_print("AVRCP task: peer fifo close\n");
				close(pdivoom_bt_music_ctrl->bt_accept_handle);
				pdivoom_bt_music_ctrl->bt_accept_handle = 0;

				close(pdivoom_bt_music_ctrl->bt_send_handle);
				pdivoom_bt_music_ctrl->bt_send_handle = 0;
			}
		}
        else 
		{
            // 管道未打开，短暂延迟后重试
            divoom_system_delay_ms(100);
        }
    }
    
    divoom_print("bt music task exit\n");
    return NULL;
}

// 资源清理函数
static void divoom_bt_music_cleanup(void)
{
    if (!pdivoom_bt_music_ctrl) 
	{
        return;
    }

    if (pdivoom_bt_music_ctrl->bt_accept_handle >= 0) 
	{
        close(pdivoom_bt_music_ctrl->bt_accept_handle);
        pdivoom_bt_music_ctrl->bt_accept_handle = -1;
    }

    if (pdivoom_bt_music_ctrl->bt_send_handle >= 0) 
	{
        close(pdivoom_bt_music_ctrl->bt_send_handle);
        pdivoom_bt_music_ctrl->bt_send_handle = -1;
    }
    if (pdivoom_bt_music_ctrl->lock_handle)
    {
        divoom_system_destroy_lock(pdivoom_bt_music_ctrl->lock_handle);
        pdivoom_bt_music_ctrl->lock_handle = NULL;
    }
    if (s_net_lyric_handle)
    {
        divoom_lvgl_lyric_word_ctrl_exit(s_net_lyric_handle);
        s_net_lyric_handle = NULL;
    }
    s_uart_lyric_valid = 0;
    divoom_bt_lyric_parse_reset(&s_lyric_ctx);
    if (pdivoom_bt_music_ctrl)
    {
        pdivoom_bt_music_ctrl->uart_lyric_phase = 0;
        pdivoom_bt_music_ctrl->pending_type1[0] = '\0';
    }

    divoom_jpeg_decompress_exit();
    divoom_system_free(pdivoom_bt_music_ctrl);
    pdivoom_bt_music_ctrl = NULL;
}

// 初始化
void divoom_bt_music_init(void)
{
    if (pdivoom_bt_music_ctrl) 
	{
        divoom_print("bt music already initialized\n");
        return;
    }

    divoom_bt_bip_init();
    divoom_jpeg_decompress_init();
    // 初始化设备信息管理器
    memset(&g_bt_device_manager, 0, sizeof(g_bt_device_manager));
    
    // 从文件加载设备信息
    divoom_bt_music_load_device_info_from_file();
    pdivoom_bt_music_ctrl = divoom_system_malloc(sizeof(divoom_bt_music_ctrl));
    if (!pdivoom_bt_music_ctrl) 
	{
        divoom_print("malloc bt music ctrl failed\n");
        return;
    }

    // 初始化结构体
    memset(pdivoom_bt_music_ctrl, 0, sizeof(divoom_bt_music_ctrl));
    divoom_bt_lyric_parse_reset(&s_lyric_ctx);
#ifdef DIVOOM_TIMES_VIBE
	if (divoom_bt_music_sync_saved_settings() != 0)
	{
		divoom_bt_music_request_bt_info();
		divoom_bt_music_request_bt_auto_connect_info();
	}
#endif
    pdivoom_bt_music_ctrl->bt_accept_handle = -1;
    pdivoom_bt_music_ctrl->bt_send_handle = -1;
    pdivoom_bt_music_ctrl->run_flag = 1;
    pdivoom_bt_music_ctrl->lock_handle = divoom_system_create_lock();

#if defined(DIVOOM_SIMULATOR) || defined(_WIN32)
    /* 仿真默认蓝牙已连接并正在播放，预置一首示例歌曲信息 */
    s_a2dp_connected = 1;
    strncpy(pdivoom_bt_music_ctrl->song_info.song_name, "\xE5\x87\xBA\xE7\x8E\xB0\xE5\x8F\x88\xE7\xA6\xBB\xE5\xBC\x80 (live)",
        sizeof(pdivoom_bt_music_ctrl->song_info.song_name) - 1);
    strncpy(pdivoom_bt_music_ctrl->song_info.singer, "\xE6\xA2\x81\xE5\x8D\x9A",
        sizeof(pdivoom_bt_music_ctrl->song_info.singer) - 1);
    strncpy(pdivoom_bt_music_ctrl->song_info.lyric, "\xE8\xB0\x81\xE4\xB9\x9F\xE6\xB2\xA1\xE6\x9C\x89\xE6\x83\xB3\xE8\xBF\x87\xE5\x86\x8D\xE6\x83\xB3\xE5\x9B\x9E\xE6\x9D\xA5",
        sizeof(pdivoom_bt_music_ctrl->song_info.lyric) - 1);
    pdivoom_bt_music_ctrl->song_info.play_pos = 0;
    pdivoom_bt_music_ctrl->song_info.song_len = 240000;
#else
	divoom_bt_music_apply_cached_phone_info();
#endif
#ifndef DIVOOM_TIMES_VIBE 
    divoom_system_create_task(divoom_bt_music_main_task, "bt_music", pdivoom_bt_music_ctrl, DIVOOM_TASK_STACK_BT_MUSIC);
#endif
}

// 反初始化
void divoom_bt_music_deinit(void)
{
    if (!pdivoom_bt_music_ctrl) 
	{
        return;
    }

    // 停止任务
    pdivoom_bt_music_ctrl->run_flag = 0;
    divoom_system_delay_ms(200);
    // 清理资源
    divoom_bt_music_cleanup();
    divoom_print("bt music deinit done\n");
}

/*********************************************************************
  Function:        divoom_bt_music_get_current_device_info
  Description:     获取当前连接上的设备信息
  Input:           none
  Return:          返回设备信息数组指针，失败返回NULL
                   注意：返回的指针指向全局数组，不需要释放
**********************************************************************/
const DeviceInfo* divoom_bt_music_get_current_device_info(void)
{
	
	if (!pdivoom_bt_music_ctrl) 
	{
		return NULL;
	}

    return &pdivoom_bt_music_ctrl->current_blue_info;
}

/*********************************************************************
  Function:        divoom_bt_music_get_device_list
  Description:     获取保存的蓝牙设备信息列表
  Input:           count - 输出参数，返回设备数量
  Return:          返回设备信息数组指针，失败返回NULL
                   注意：返回的指针指向全局数组，不需要释放
**********************************************************************/
const DeviceInfo* divoom_bt_music_get_device_list(int *count)
{
    if (!count)
    {
        divoom_print("Invalid parameter: count is NULL\n");
        return NULL;
    }

    *count = g_bt_device_manager.count;
    
    if (g_bt_device_manager.count == 0)
    {
        return NULL;
    }

    return g_bt_device_manager.devices;
}

#ifdef DIVOOM_TIMES_VIBE
static int divoom_bt_music_parse_bt_addr_bin(const char *bt_addr, uint8_t addr_bin[6])
{
	unsigned int b0, b1, b2, b3, b4, b5;

	if (!bt_addr || !addr_bin || strlen(bt_addr) != 17)
		return -1;
	if (sscanf(bt_addr, "%02X:%02X:%02X:%02X:%02X:%02X",
		   &b0, &b1, &b2, &b3, &b4, &b5) != 6)
		return -1;

	addr_bin[0] = (uint8_t)b0;
	addr_bin[1] = (uint8_t)b1;
	addr_bin[2] = (uint8_t)b2;
	addr_bin[3] = (uint8_t)b3;
	addr_bin[4] = (uint8_t)b4;
	addr_bin[5] = (uint8_t)b5;
	return 0;
}
#endif

/*********************************************************************
  Function:        divoom_bt_music_connect_device
  Description:     连接指定的蓝牙设备，并将设备信息发送给进程A
  Input:           bt_addr - 蓝牙地址（格式：XX:XX:XX:XX:XX:XX）
  Return:          0=成功, -1=失败
**********************************************************************/
int divoom_bt_music_connect_device(const char *bt_addr)
{
#ifdef DIVOOM_TIMES_VIBE
	uint8_t addr_bin[6];

	if (divoom_bt_music_parse_bt_addr_bin(bt_addr, addr_bin) != 0)
	{
		divoom_print("Invalid BT address for 0x53: %s\n", bt_addr ? bt_addr : "NULL");
		return -1;
	}
	divoom_print("connect one device 0x53: %s (%02X:%02X:%02X:%02X:%02X:%02X)\n",
		     bt_addr,
		     addr_bin[0], addr_bin[1], addr_bin[2],
		     addr_bin[3], addr_bin[4], addr_bin[5]);
	divoom_btcore_uart_cmd_send(DM_BTCORE_BT_CONNECT_ONE_DEVICE, addr_bin, 6);
#else
    if (!bt_addr || strlen(bt_addr) != 17)
    {
        divoom_print("Invalid BT address: %s\n", bt_addr ? bt_addr : "NULL");
        return -1;
    }

    if (!pdivoom_bt_music_ctrl || pdivoom_bt_music_ctrl->bt_send_handle <= 0)
    {
        divoom_print("BT music not initialized or send handle invalid\n");
        return -1;
    }

    // 查找设备信息
    DeviceInfo *device_info = NULL;
    int i;
    for (i = 0; i < g_bt_device_manager.count; i++)
    {
        if (strcmp(g_bt_device_manager.devices[i].bt_addr, bt_addr) == 0)
        {
            device_info = &g_bt_device_manager.devices[i];
            break;
        }
    }

    // 如果找不到设备信息，创建一个临时的设备信息（只有地址，名称为空）
    DeviceInfo temp_info = {0};
    if (!device_info)
    {
        strncpy(temp_info.bt_addr, bt_addr, sizeof(temp_info.bt_addr) - 1);
        temp_info.bt_addr[17] = '\0';
        strcpy(temp_info.device_name, "Unknown Device");
        device_info = &temp_info;
        divoom_print("Device not found in list, using temporary info\n");
    }

    // 发送连接命令到进程A
    if (divoom_bt_music_send_connect_device(pdivoom_bt_music_ctrl->bt_send_handle, bt_addr) != 0)
    {
        divoom_print("Failed to send connect command\n");
        return -1;
    }
    divoom_print("Connect device command sent: %s - %s\n", device_info->bt_addr, device_info->device_name);
#endif
    return 0;
}

/*********************************************************************
  Function:        divoom_bt_music_unpair_device
  Description:     取消配对指定的蓝牙设备
  Input:           bt_addr - 蓝牙地址（格式：XX:XX:XX:XX:XX:XX）
  Return:          0=成功, -1=失败
**********************************************************************/
int divoom_bt_music_unpair_device(const char *bt_addr)
{
	char addr_copy[18];

	memset(addr_copy, 0, sizeof(addr_copy));
	if (bt_addr != NULL && bt_addr[0] != '\0' && strlen(bt_addr) == 17)
		strncpy(addr_copy, bt_addr, sizeof(addr_copy) - 1);

#ifdef DIVOOM_TIMES_VIBE
	divoom_btcore_uart_cmd_send(DM_BTCORE_BT_UNPAIR_DEVICE, NULL, 0);
	if (pdivoom_bt_music_ctrl)
		memset(&pdivoom_bt_music_ctrl->current_blue_info, 0,
		       sizeof(pdivoom_bt_music_ctrl->current_blue_info));
#else
	if (addr_copy[0] == '\0')
    {
        divoom_print("Invalid BT address: %s\n", bt_addr ? bt_addr : "NULL");
        return -1;
    }

    if (!pdivoom_bt_music_ctrl || pdivoom_bt_music_ctrl->bt_send_handle <= 0)
    {
        divoom_print("BT music not initialized or send handle invalid\n");
        return -1;
    }

    // 发送取消配对命令到进程A
    if (divoom_bt_music_send_unpair_device(pdivoom_bt_music_ctrl->bt_send_handle, addr_copy) != 0)
    {
        divoom_print("Failed to send unpair command\n");
        return -1;
    }
#endif

    // 从设备列表中移除该设备（如果存在）
    if (addr_copy[0] != '\0')
    {
	int i;

	for (i = 0; i < g_bt_device_manager.count; i++)
	{
	    if (strcmp(g_bt_device_manager.devices[i].bt_addr, addr_copy) == 0)
	    {
		int j;

		for (j = i; j < g_bt_device_manager.count - 1; j++)
		{
		    memcpy(&g_bt_device_manager.devices[j],
			   &g_bt_device_manager.devices[j + 1],
			   sizeof(DeviceInfo));
		}
		g_bt_device_manager.count--;
		divoom_print("Removed device from list: %s\n", addr_copy);

		divoom_bt_music_save_device_info_to_file();
		break;
	    }
	}
    }
    else
    {
	divoom_print("Unpair: no valid bt_addr, skip list remove\n");
    }

    divoom_print("Unpair device command sent: %s\n", addr_copy[0] ? addr_copy : "(empty)");
    return 0;
}

int divoom_bt_music_unpair_all_devices(void)
{
#ifdef DIVOOM_TIMES_VIBE
	divoom_btcore_uart_cmd_send(DM_BTCORE_BT_UNPAIR_DEVICE, NULL, 0);
	if (pdivoom_bt_music_ctrl)
	{
		memset(&pdivoom_bt_music_ctrl->current_blue_info, 0,
		       sizeof(pdivoom_bt_music_ctrl->current_blue_info));
	}
#endif
	s_a2dp_connected = 0;
	s_bt_a2dp_playing = 0;
	memset(&g_bt_device_manager, 0, sizeof(g_bt_device_manager));
	divoom_bt_music_save_device_info_to_file();
	divoom_print("Unpair all paired devices\n");
	return 0;
}

/*********************************************************************
  Function:        divoom_bt_music_disable_boot_connect
  Description:     设置是否禁用开机自动重连（仅下发0x15，不断开当前连接、不清地址）
  Input:           disable - 1=禁用开机自动重连，0=启用开机自动重连
  Return:          0=成功, -1=失败
**********************************************************************/
static int divoom_bt_music_clear_blue_addr(void)
{
#ifdef DIVOOM_TIMES_VIBE
	return divoom_btcore_uart_cmd_send(DM_BTCORE_CLEAR_BLUE_ADDR, NULL, 0);
#else
	return 0;
#endif
}

int divoom_bt_music_disable_boot_connect(int disable)
{
#ifdef DIVOOM_TIMES_VIBE
	uint8_t cmd_data[2] = {0};
	uint8_t reply = 0;
	uint16_t reply_len = 0;

	cmd_data[0] = 1;
	cmd_data[1] = disable ? 0 : 1;
	if (divoom_btcore_uart_cmd_is_ready())
	{
		if (divoom_btcore_uart_cmd_send_wait(DM_BTCORE_BT_AUTO_CONNECT, cmd_data, 2,
						     &reply, 1, &reply_len) == 0 && reply_len >= 1u)
		{
			divoom_bt_music_parse_bt_auto_connect_info(&reply, reply_len);
		}
		else
		{
			divoom_btcore_uart_cmd_send(DM_BTCORE_BT_AUTO_CONNECT, cmd_data, 2);
		}
	}
	if (pdivoom_bt_music_ctrl)
	{
		pdivoom_bt_music_ctrl->bt_info.auto_connect = disable ? 0 : 1;
		pdivoom_bt_music_ctrl->bt_info.auto_connect_valid = 1;
	}
#else
    if (!pdivoom_bt_music_ctrl || pdivoom_bt_music_ctrl->bt_send_handle <= 0)
    {
        divoom_print("BT music not initialized or send handle invalid\n");
        return -1;
    }

    // 发送禁用/启用开机自动重连命令到进程A
    if (divoom_bt_music_send_disable_boot_connect(pdivoom_bt_music_ctrl->bt_send_handle, disable) != 0)
    {
        divoom_print("Failed to send disable boot connect command\n");
        return -1;
    }
#endif
    divoom_print("Disable boot connect command sent: %s\n", disable ? "DISABLE" : "ENABLE");
    return 0;
}

// 从文件读取蓝牙开机自动重连
int divoom_bt_music_get_boot_connect_flag_from_file(void)
{
	int boot_connect_flag = 1;

#ifdef DIVOOM_TIMES_VIBE
	if (!pdivoom_bt_music_ctrl || !pdivoom_bt_music_ctrl->bt_info.auto_connect_valid)
	{
		Pdivoom_sys_setting_info cfg = divoom_main_get_system_config();

		if (cfg != NULL)
			return cfg->bt_auto_connect ? 1 : 0;
		return -1;
	}

	boot_connect_flag = pdivoom_bt_music_ctrl->bt_info.auto_connect ? 1 : 0;
#else
    FILE *fp = NULL;
    
    fp = divoom_port_file_open(DIVOOM_BLUE_ADDR_SAVE_PATH, "r");
    if (fp == NULL)
    {
        divoom_print("No saved BT address file found\n");
        return 1;
    }
    
    // 读取第一行：蓝牙地址
    char line[64] = {0};
    if (fgets(line, sizeof(line), fp) == NULL)
    {
        divoom_print("Failed to read address from file\n");
        divoom_port_file_close(fp);
        return 1;
    }
    
    // 去除换行符
    int len = strlen(line);
    if (len > 0 && line[len - 1] == '\n')
    {
        line[len - 1] = '\0';
    }
    
    // 验证地址格式 (XX:XX:XX:XX:XX:XX)
    if (strlen(line) != 17)
    {
        divoom_print("Invalid address format in file: %s\n", line);
        divoom_port_file_close(fp);
        return 1;
    }
    
    // 读取第二行：自动重连标志（如果存在）
    if (fgets(line, sizeof(line), fp) != NULL)
    {
        // 去除换行符
        len = strlen(line);
        if (len > 0 && line[len - 1] == '\n')
        {
            line[len - 1] = '\0';
        }
        
        // 解析标志
        int flag = atoi(line);
        boot_connect_flag = (flag != 0) ? 1 : 0;
    }
    else
    {
        // 旧格式文件，只有地址，标志默认为0（启用）
        boot_connect_flag = 0;
    }
    
    divoom_port_file_close(fp);
#endif 
    return boot_connect_flag;
}


/*********************************************************************
  Function:        divoom_bt_music_get_pair_pin
  Description:     获取蓝牙配对密码配置（直接读取配置文件）
  Input:           enable - 输出，1=已启用，0=未启用；可为 NULL
                  pin4   - 输出，4位密码字符串，至少5字节；可为 NULL
                  maxlen - pin4 缓冲区长度
  Return:          0=成功, -1=文件不存在或读取失败
**********************************************************************/
int divoom_bt_music_get_pair_pin(int *enable, char *pin4, int maxlen)
{
#ifdef DIVOOM_TIMES_VIBE
	if (!pdivoom_bt_music_ctrl)
	{
		if (enable != NULL)
			*enable = 0;
		if (pin4 != NULL && maxlen > 0)
			pin4[0] = '\0';
		return -1;
	}

	if (enable != NULL)
		*enable = pdivoom_bt_music_ctrl->bt_info.ssp_switch ? 1 : 0;
	if (pin4 != NULL && maxlen > 0)
	{
		int copy_n = (maxlen - 1 >= 4) ? 4 : (maxlen - 1);

		memcpy(pin4, pdivoom_bt_music_ctrl->bt_info.ssp_secret, copy_n);
		pin4[copy_n] = '\0';
	}
#else
    FILE *fp = NULL;
    char line[32] = {0};
    int len;

    fp = divoom_port_file_open(BT_PIN_SAVE_PATH, "r");
    if (fp == NULL)
    {
        if (enable != NULL)
            *enable = 0;
        if (pin4 != NULL && maxlen > 0)
            pin4[0] = '\0';
        return -1;
    }

    if (fgets(line, sizeof(line), fp) == NULL)
    {
        divoom_port_file_close(fp);
        if (enable != NULL)
            *enable = 0;
        if (pin4 != NULL && maxlen > 0)
            pin4[0] = '\0';
        return -1;
    }
    len = strlen(line);
    if (len > 0 && line[len - 1] == '\n')
        line[len - 1] = '\0';
    if (enable != NULL)
        *enable = (atoi(line) != 0) ? 1 : 0;

    if (pin4 != NULL && maxlen > 0)
    {
        pin4[0] = '\0';
        if (fgets(line, sizeof(line), fp) != NULL)
        {
            len = strlen(line);
            if (len > 0 && line[len - 1] == '\n')
                line[len - 1] = '\0';
            strncpy(pin4, line, maxlen - 1);
            pin4[maxlen - 1] = '\0';
            if (strlen(pin4) > (size_t)BT_PIN_CODE_LEN)
                pin4[BT_PIN_CODE_LEN] = '\0';
        }
    }

    divoom_port_file_close(fp);
#endif
    return 0;
}

/*********************************************************************
  Function:        divoom_bt_music_set_pair_pin
  Description:     设置蓝牙配对密码（通过管道发送到蓝牙进程）
                  蓝牙进程会保存并在此后配对时使用该4位PIN
  Input:           enable - 1=启用蓝牙密码，0=关闭蓝牙密码
                  pin4   - 4位数字密码字符串，如 "1234"
  Return:          0=成功, -1=失败
**********************************************************************/
static int divoom_bt_music_query_bt_info_from_jl_into(ParsedBtDeviceInfo *info)
{
	uint8_t cmd_data = 0;
	uint8_t reply[96];
	uint16_t reply_len = 0;
	int attempt;

	if (info == NULL || !divoom_btcore_uart_cmd_is_ready())
		return -1;

	/* SET 0x05 后 JL 可能先回 1 字节应答，需重试 GET 直到拿到完整 90 字节 */
	for (attempt = 0; attempt < 6; attempt++)
	{
		if (attempt > 0)
			divoom_system_delay_ms(80);

		reply_len = 0;
		if (divoom_btcore_uart_cmd_send_wait(DM_BTCORE_BT_INFO, &cmd_data, 1,
						     reply, sizeof(reply), &reply_len) != 0)
			continue;
		if (reply_len < DIVOOM_BTCORE_BT_INFO_BODY_LEN)
			continue;
		if (divoom_bt_music_parse_jl_bt_info_wire(info, reply, reply_len) != 0)
			continue;

		divoom_bt_music_sync_jl_bt_info_to_ctrl(info);
		return 0;
	}

	return -1;
}

static int divoom_bt_music_query_bt_info_from_jl(void)
{
	ParsedBtDeviceInfo info;

	return divoom_bt_music_query_bt_info_from_jl_into(&info);
}

static void divoom_bt_music_fill_jl_name_fields(ParsedBtDeviceInfo *info, const char *name_utf8)
{
	char tmp[26];

	if (info == NULL)
		return;

	memset(tmp, 0, sizeof(tmp));
	if (name_utf8 != NULL && name_utf8[0] != '\0')
	{
		strncpy(tmp, name_utf8, sizeof(tmp) - 1u);
		divoom_bt_music_sanitize_jl_name(tmp, sizeof(tmp));
	}

	memset(info->bt_name, 0, sizeof(info->bt_name));
	memset(info->bt_br_name, 0, sizeof(info->bt_br_name));
	memset(info->bt_ble_name, 0, sizeof(info->bt_ble_name));
	memcpy(info->bt_name, tmp, sizeof(tmp));
	memcpy(info->bt_br_name, tmp, sizeof(tmp));
	memcpy(info->bt_ble_name, tmp, sizeof(tmp));
}

static size_t divoom_bt_music_pack_jl_bt_info_set(uint8_t *buffer, size_t buf_len,
						  const ParsedBtDeviceInfo *info,
						  uint8_t reset_flag)
{
	size_t pos = 0;

	if (buffer == NULL || info == NULL ||
	    buf_len < DIVOOM_BTCORE_BT_INFO_SET_PAYLOAD_LEN)
		return 0;

	buffer[pos++] = 1; /* ctrl: SET */
	memcpy(buffer + pos, info->bt_name, sizeof(info->bt_name));
	pos += sizeof(info->bt_name);
	memcpy(buffer + pos, info->bt_br_name, sizeof(info->bt_br_name));
	pos += sizeof(info->bt_br_name);
	memcpy(buffer + pos, info->bt_ble_name, sizeof(info->bt_ble_name));
	pos += sizeof(info->bt_ble_name);
	buffer[pos++] = info->ssp_switch;
	memcpy(buffer + pos, info->ssp_secret, sizeof(info->ssp_secret));
	pos += sizeof(info->ssp_secret);
	buffer[pos++] = reset_flag ? 1u : 0u;
	return pos;
}

int divoom_bt_music_set_jl_adapter_name(const char *name_utf8)
{
	uint8_t buffer[DIVOOM_BTCORE_BT_INFO_SET_PAYLOAD_LEN] = {0};
	ParsedBtDeviceInfo info;
	char sanitized_name[26];
	char expect_name[26];
	char last_got[26];
	size_t pos = 0;
	int retry;

	if (name_utf8 == NULL || name_utf8[0] == '\0')
	{
		divoom_print("set jl bt name: empty name\n");
		return -1;
	}

	divoom_print("set jl bt name input: %s\n", name_utf8);
	divoom_bt_music_log_jl_name_bytes("set jl bt name input",
					  (const uint8_t *)name_utf8, strlen(name_utf8));

	memset(sanitized_name, 0, sizeof(sanitized_name));
	strncpy(sanitized_name, name_utf8, sizeof(sanitized_name) - 1u);
	if (divoom_bt_music_sanitize_jl_name(sanitized_name, sizeof(sanitized_name)) == 0u)
	{
		divoom_print("set jl bt name: empty name after sanitize\n");
		return -1;
	}
	divoom_bt_music_log_jl_name_bytes("set jl bt name sanitized",
					  (const uint8_t *)sanitized_name, strlen(sanitized_name));
	if (!divoom_btcore_uart_cmd_is_ready())
	{
		divoom_print("set jl bt name: uart not ready\n");
		return -1;
	}

	if (pdivoom_bt_music_ctrl)
	{
		if (divoom_bt_music_ensure_bt_info_ready() != 0)
			divoom_bt_music_fill_bt_name_fallback();
		memcpy(&info, &pdivoom_bt_music_ctrl->bt_info, DIVOOM_BTCORE_BT_INFO_BODY_LEN);
	}
	else if (divoom_bt_music_query_bt_info_from_jl_into(&info) != 0)
	{
		divoom_print("set jl bt name: query jl failed\n");
		return -1;
	}

	divoom_bt_music_fill_jl_name_fields(&info, sanitized_name);
	memcpy(expect_name, info.bt_name, sizeof(expect_name));
	divoom_bt_music_log_jl_name_bytes("set jl bt name tx bt_name",
					  (const uint8_t *)info.bt_name, sizeof(info.bt_name));
	/* 仅改名：Reset_flag=0，避免 JL 清蓝牙地址 */
	pos = divoom_bt_music_pack_jl_bt_info_set(buffer, sizeof(buffer), &info, 0);
	if (pos == 0u)
	{
		divoom_print("set jl bt name: pack failed\n");
		return -1;
	}

	memset(last_got, 0, sizeof(last_got));
	for (retry = 0; retry < 3; retry++)
	{
		ParsedBtDeviceInfo verify;

		if (retry > 0)
			divoom_system_delay_ms(150);

		divoom_btcore_uart_cmd_send(DM_BTCORE_BT_INFO, buffer, (uint16_t)pos);
		divoom_system_delay_ms(250);

		if (divoom_bt_music_query_bt_info_from_jl_into(&verify) != 0)
			continue;

		memcpy(last_got, verify.bt_name, sizeof(last_got));
		if (divoom_bt_music_jl_name_field_match(expect_name, verify.bt_name))
		{
			divoom_bt_music_sync_jl_bt_info_to_ctrl(&verify);
			divoom_print("set jl bt name ok: %s\n", sanitized_name);
			return 0;
		}
	}

	divoom_print("set jl bt name verify failed: expect=%s got=%s\n",
		     expect_name, last_got[0] != '\0' ? last_got : "(none)");
	return -1;
}

int divoom_bt_music_set_pair_pin(int enable, const char *pin4)
{
#ifdef DIVOOM_TIMES_VIBE
	if (!pdivoom_bt_music_ctrl)
		return -1;

	uint8_t buffer[DIVOOM_BTCORE_BT_INFO_SET_PAYLOAD_LEN] = {0};
	size_t pos = 0;

	if (pin4 == NULL || strlen(pin4) < 4)
	{
		divoom_print("pin4 must be 4 digits\n");
		return -1;
	}

	if (divoom_bt_music_ensure_bt_info_ready() != 0)
		divoom_bt_music_fill_bt_name_fallback();

	{
		ParsedBtDeviceInfo info;

		memcpy(&info, &pdivoom_bt_music_ctrl->bt_info, DIVOOM_BTCORE_BT_INFO_BODY_LEN);
		info.ssp_switch = enable ? 1 : 0;
		memset(info.ssp_secret, 0, sizeof(info.ssp_secret));
		memcpy(info.ssp_secret, pin4, 4);
		/* 仅改 PIN：Reset_flag=0 */
		pos = divoom_bt_music_pack_jl_bt_info_set(buffer, sizeof(buffer), &info, 0);
		if (pos == 0u)
		{
			divoom_print("set bt pair pin: pack failed\n");
			return -1;
		}
	}
	if (divoom_btcore_uart_cmd_is_ready())
	{
		int retry;
		int verified = 0;

		/* 杰理对 0x05 SET 通常不回包，send_wait 会超时；改为 SET 后 GET 校验 */
		for (retry = 0; retry < 3; retry++)
		{
			if (retry > 0)
				divoom_system_delay_ms(100);

			divoom_btcore_uart_cmd_send(DM_BTCORE_BT_INFO, buffer, (uint16_t)pos);
			divoom_system_delay_ms(150);

			if (divoom_bt_music_query_bt_info_from_jl() != 0)
				continue;

			if (pdivoom_bt_music_ctrl->bt_info.ssp_switch == (enable ? 1 : 0) &&
			    (!enable ||
			     strncmp(pdivoom_bt_music_ctrl->bt_info.ssp_secret, pin4, 4) == 0))
			{
				verified = 1;
				break;
			}
		}
		if (!verified)
		{
			divoom_print("set bt pair pin verify failed\n");
			return -1;
		}
	}
	else
	{
		divoom_btcore_uart_cmd_send(DM_BTCORE_BT_INFO, buffer, (uint16_t)pos);
		pdivoom_bt_music_ctrl->bt_info.ssp_switch = enable ? 1 : 0;
		memcpy(pdivoom_bt_music_ctrl->bt_info.ssp_secret, pin4, 4);
		pdivoom_bt_music_ctrl->bt_info.ssp_secret[4] = '\0';
	}
#else
    if (!pdivoom_bt_music_ctrl || pdivoom_bt_music_ctrl->bt_send_handle <= 0)
    {
        divoom_print("BT music not initialized or send handle invalid\n");
        return -1;
    }
    if (pin4 == NULL || strlen(pin4) < 4)
    {
        divoom_print("pin4 must be 4 digits\n");
        return -1;
    }

    if (divoom_bt_music_send_set_bt_pin(pdivoom_bt_music_ctrl->bt_send_handle, enable, pin4) != 0)
    {
        divoom_print("Failed to send set BT pin command\n");
        return -1;
    }
#endif
    divoom_print("Set BT pair pin command sent: enable=%d, pin=%.4s\n", enable, pin4);
    return 0;
}

int divoom_bt_music_apply_saved_pin_settings(void)
{
	Pdivoom_sys_setting_info cfg = divoom_main_get_system_config();

	if (cfg == NULL || cfg->bt_password[0] == '\0')
		return -1;

	return divoom_bt_music_set_pair_pin(cfg->bt_secret_switch, cfg->bt_password);
}

static void divoom_bt_music_push_sys_config_bt_flags_to_jl(Pdivoom_sys_setting_info cfg)
{
	if (cfg == NULL)
		return;

	if (pdivoom_bt_music_ctrl &&
	    (pdivoom_bt_music_ctrl->bt_info.ssp_switch != (cfg->bt_secret_switch ? 1 : 0) ||
	     (cfg->bt_secret_switch &&
	      strncmp(pdivoom_bt_music_ctrl->bt_info.ssp_secret,
		      cfg->bt_password, 4) != 0)))
	{
		if (divoom_bt_music_apply_saved_pin_settings() != 0)
			divoom_print("sync saved bt pin failed\n");
	}

	divoom_bt_music_disable_boot_connect(cfg->bt_auto_connect ? 0 : 1);
}

int divoom_bt_music_sync_saved_settings(void)
{
	Pdivoom_sys_setting_info psys_config;

	if (!divoom_btcore_uart_cmd_is_ready())
		return -1;

	/* 仅 GET 一次 BT_INFO 供比对；开机以 F101 cfg 推杰理，避免额外 send_wait/写盘卡住开机动画/提示音 */
	if (divoom_bt_music_query_bt_info_from_jl() != 0)
		divoom_bt_music_fill_bt_name_fallback();

	psys_config = divoom_main_get_system_config();
	if (psys_config == NULL)
		return -1;

	divoom_bt_music_push_sys_config_bt_flags_to_jl(psys_config);
	divoom_print("sync saved bt settings: auto_connect=%d pin_enable=%d\n",
		     psys_config->bt_auto_connect, psys_config->bt_secret_switch);
	return 0;
}

/*********************************************************************
  Function:        divoom_bt_music_exit_bt_system
  Description:     发送完全退出蓝牙系统命令到蓝牙进程
  Input:           NONE
  Return:          0=成功, -1=失败
**********************************************************************/
int divoom_bt_music_exit_bt_system(void)
{
    if (!pdivoom_bt_music_ctrl || pdivoom_bt_music_ctrl->bt_send_handle <= 0)
    {
        divoom_print("BT music not initialized or send handle invalid\n");
        return -1;
    }

    // 发送完全退出蓝牙系统命令到蓝牙进程
    if (divoom_bt_music_send_exit_bt_system(pdivoom_bt_music_ctrl->bt_send_handle) != 0)
    {
        divoom_print("Failed to send exit BT system command\n");
        return -1;
    }

    divoom_print("Exit BT system command sent\n");
    return 0;
}

// 发送开启经典蓝牙命令（数据类型0x0C）
// 数据头(0xAA)+数据长度+数据类型+校验位+数据尾(0x55)
// 注意：此命令不需要数据内容，content_len=0
static int divoom_bt_music_send_start_classic_bt(int fd)
{
    if (fd < 0)
    {
        divoom_print("Invalid file descriptor for start classic BT\n");
        return -1;
    }

    // 计算数据长度：0字节（不需要数据内容）
    uint16_t content_len = 0;
    // 总长度：头(1) + 长度(2) + 类型(1) + 校验(1) + 尾(1) = 6
    uint8_t buffer[6];
    uint16_t pos = 0;

    // 填充消息
    buffer[pos++] = DATA_HEADER;
    buffer[pos++] = (content_len >> 8) & 0xFF;
    buffer[pos++] = content_len & 0xFF;
    buffer[pos++] = DATA_START_CLASSIC_BT;  // 数据类型

    // 计算校验位（只有数据类型，没有数据内容）
    buffer[pos++] = divoom_bt_music_calc_checksum(buffer + 3, 1 + content_len);
    buffer[pos++] = DATA_TAIL;

    // 写入管道
    ssize_t written = write(fd, buffer, pos);
    if (written != pos) 
    {
        divoom_print("write start classic BT failed: %s\n", strerror(errno));
        return -1;
    }

    divoom_print("Sent start classic BT command\n");
    return 0;
}

/* 无 payload 的 A2DP 管道命令（data_type 为 0x0B/0x0C/0x11/0x12 等） */
static int divoom_bt_music_send_no_payload(int fd, uint8_t data_type)
{
    uint16_t content_len = 0;
    uint8_t buffer[6];
    uint16_t pos = 0;

    buffer[pos++] = DATA_HEADER;
    buffer[pos++] = (content_len >> 8) & 0xFF;
    buffer[pos++] = content_len & 0xFF;
    buffer[pos++] = data_type;
    buffer[pos++] = divoom_bt_music_calc_checksum(buffer + 3, (uint16_t)(1 + content_len));
    buffer[pos++] = DATA_TAIL;
    if (write(fd, buffer, pos) != (ssize_t)pos)
    {
        divoom_print("write no-payload cmd 0x%02X failed: %s\n", data_type, strerror(errno));
        return -1;
    }
    return 0;
}

/*********************************************************************
  Function:        divoom_bt_music_start_classic_bt
  Description:     发送开启经典蓝牙命令到蓝牙进程
                   蓝牙进程接收到此命令后会启动经典蓝牙
  Input:           NONE
  Return:          0=成功, -1=失败
**********************************************************************/
int divoom_bt_music_start_classic_bt(void)
{
    if (!pdivoom_bt_music_ctrl || pdivoom_bt_music_ctrl->bt_send_handle <= 0)
    {
        divoom_print("BT music not initialized or send handle invalid\n");
        return -1;
    }

    // 发送开启经典蓝牙命令到蓝牙进程
    if (divoom_bt_music_send_start_classic_bt(pdivoom_bt_music_ctrl->bt_send_handle) != 0)
    {
        divoom_print("Failed to send start classic BT command\n");
        return -1;
    }

    divoom_print("Start classic BT command sent\n");
    return 0;
}

/*********************************************************************
  Function:        divoom_bt_music_disconnect_bt
  Description:     发送断开当前蓝牙连接命令（仅断开与手机的连接，经典蓝牙仍可再配对）
  Return:          0=成功, -1=失败
**********************************************************************/
int divoom_bt_music_disconnect_bt(void)
{
#ifdef DIVOOM_TIMES_VIBE
	uint8_t cmd_data[7] = {0};
	cmd_data[0] = 0;
	divoom_btcore_uart_cmd_send(DM_BTCORE_BT_CONNECT_CTRL, cmd_data, 7);	
#else
    if (!pdivoom_bt_music_ctrl || pdivoom_bt_music_ctrl->bt_send_handle <= 0)
    {
        divoom_print("BT music not initialized or send handle invalid\n");
        return -1;
    }
    if (divoom_bt_music_send_no_payload(pdivoom_bt_music_ctrl->bt_send_handle, DATA_BT_DISCONNECT) != 0)
    {
        divoom_print("Failed to send disconnect BT command\n");
        return -1;
    }
#endif
    divoom_print("Disconnect BT command sent\n");
    return 0;
}

/*********************************************************************
  Function:        divoom_bt_music_connect_bt
  Description:     连接蓝牙：有已配对地址则 0x53 重连，否则 0x3a 通用连接
  Return:          0=成功, -1=失败
**********************************************************************/
int divoom_bt_music_connect_bt(void)
{
	const char *addr = NULL;
	const DeviceInfo *cur;
	const DeviceInfo *list;
	int count;

	cur = divoom_bt_music_get_current_device_info();
	if (cur != NULL && cur->bt_addr[0] != '\0')
		addr = cur->bt_addr;
	else
	{
		list = divoom_bt_music_get_device_list(&count);
		if (list != NULL && count > 0 && list[count - 1].bt_addr[0] != '\0')
			addr = list[count - 1].bt_addr;
	}

	if (addr != NULL)
		return divoom_bt_music_connect_device(addr);

#ifdef DIVOOM_TIMES_VIBE
	{
		uint8_t cmd_data[7] = {0};

		cmd_data[0] = 1;
		divoom_btcore_uart_cmd_send(DM_BTCORE_BT_CONNECT_CTRL, cmd_data, 7);
	}
#else
	if (!pdivoom_bt_music_ctrl || pdivoom_bt_music_ctrl->bt_send_handle <= 0)
	{
		divoom_print("BT music not initialized or send handle invalid\n");
		return -1;
	}
	if (divoom_bt_music_start_classic_bt() != 0)
		return -1;
#endif
	divoom_print("Connect BT command sent\n");
	return 0;
}

/*********************************************************************
  Function:        divoom_bt_music_disable_classic_bt
  Description:     发送关闭经典蓝牙命令（关闭经典蓝牙射频；BLE 仍可用）
                   需再次使用 divoom_bt_music_start_classic_bt 打开
  Return:          0=成功, -1=失败
**********************************************************************/
int divoom_bt_music_disable_classic_bt(void)
{
    if (!pdivoom_bt_music_ctrl || pdivoom_bt_music_ctrl->bt_send_handle <= 0)
    {
        divoom_print("BT music not initialized or send handle invalid\n");
        return -1;
    }
    if (divoom_bt_music_send_no_payload(pdivoom_bt_music_ctrl->bt_send_handle, DATA_BT_DISABLE_CLASSIC) != 0)
    {
        divoom_print("Failed to send disable classic BT command\n");
        return -1;
    }
    divoom_print("Disable classic BT command sent\n");
    return 0;
}
