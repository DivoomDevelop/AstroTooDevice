#include "divoom.h"

#include <string.h>

#ifndef DIVOOM_TONE_MIN_AUDIO_BYTES
#define DIVOOM_TONE_MIN_AUDIO_BYTES  256
#endif

#define DIVOOM_VOICE_READ_LEAVE_MSG_HANDLE (T_U32_MAX)
#define DIVOOM_VOICE_READ_ALARM_HANDLE (T_U32_MAX-1)

typedef struct _Voice_Tip_Handle
{
    T_U8  page_buf[DIVOOM_PAGE_SIZE];
	T_U32 BaseAddr;
	T_U32 VoiceOff;
	T_U32 VoiceSize;
	T_U32 BinOff;
}T_VOICE_TIP_HANDLE, *T_PVOICE_TIP_HANDLE;

typedef struct _voice_model_id_info
{
    T_U32 off;
    T_U32 size;
}voice_model_id_info;
//static voice_model_id_info divoom_voice_all_info[DIVOOM_VOICE_ID_MAX];
static voice_model_id_info *divoom_voice_all_info = AK_NULL;

static T_VOICE_TIP_HANDLE *pVoiceTipFileHandle;


#define DIVOOM_VOICE_TON_READ_DIR_PATH	"file:///usr/share/divoom_app/tone/"
#define DIVOOM_VOICE_TON_WRITE_DIR_PATH	"file:///usr/share/divoom_app/tone/"



typedef struct __voice_path_info
{
	uint16 id;
	char name[128];
}t_voice_path_info,*pt_voice_path_info;

static const char *divoom_tone_path_table[] =
{
	[DIVOOM_VOICE_ID_CONNECTED] = "connected.mp3",
	[DIVOOM_VOICE_ID_GAMEANSWER] = "gameanswer.mp3",
	[DIVOOM_VOICE_ID_GAMEDICE] = "gamedice.mp3",
	[DIVOOM_VOICE_ID_GAMEGUESS] = "gameguess.mp3",
	[DIVOOM_VOICE_ID_GAMEOVER] = "gameover.mp3",
	[DIVOOM_VOICE_ID_GAMEPLANEOVER] = "gameplaneover.mp3",
	[DIVOOM_VOICE_ID_GAMETARGE] = "gametarge.mp3",
	[DIVOOM_VOICE_ID_LOWPOWER] = "lowpower.mp3",
	[DIVOOM_VOICE_ID_PAIRING] = "pairing.mp3",
	[DIVOOM_VOICE_ID_POWEROFF] = "poweroff.mp3",
	[DIVOOM_VOICE_ID_SLEEP_WHITE_NOISE] = "Sleep_White_Noise.mp3",
	[DIVOOM_VOICE_ID_SLEEP_WHISPER] = "Sleep_Whisper.mp3",
	[DIVOOM_VOICE_ID_SLEEP_WATER_DROP] = "Sleep_Water_Drop.mp3",
	[DIVOOM_VOICE_ID_SLEEP_UNDER_WATER] = "Sleep_Under_Water.mp3",
	[DIVOOM_VOICE_ID_SLEEP_THUNDER] = "Sleep_Thunder.mp3",
	[DIVOOM_VOICE_ID_SLEEP_SUMMER] = "Sleep_Summer.mp3",
	[DIVOOM_VOICE_ID_SLEEP_SNOWSTORM] = "Sleep_Snowstorm.mp3",
	[DIVOOM_VOICE_ID_SLEEP_SEASHORE] = "Sleep_Seashore.mp3",
	[DIVOOM_VOICE_ID_SLEEP_ROSA_CREEK] = "Sleep_Rosa_Creek.mp3",
	[DIVOOM_VOICE_ID_SLEEP_RELAX] = "Sleep_Relax.mp3",
	[DIVOOM_VOICE_ID_SLEEP_NIGHT_FALL] = "Sleep_Night_Fall.mp3",
	[DIVOOM_VOICE_ID_SLEEP_MUSIC_BOX] = "Sleep_Music_Box.mp3",
	[DIVOOM_VOICE_ID_SLEEP_MIDSUMMER] = "Sleep_Midsummer.mp3",
	[DIVOOM_VOICE_ID_SLEEP_MEDITATE] = "Sleep_Meditate.mp3",
	[DIVOOM_VOICE_ID_SLEEP_LULLABY] = "Sleep_Lullaby.mp3",
	[DIVOOM_VOICE_ID_SLEEP_GARDEN] = "Sleep_Garden.mp3",
	[DIVOOM_VOICE_ID_SLEEP_FROGS] = "Sleep_Frogs.mp3",
	[DIVOOM_VOICE_ID_SLEEP_DIVING] = "Sleep_Diving.mp3",
	[DIVOOM_VOICE_ID_SLEEP_COUNTRYSIDE] = "Sleep_Countryside.mp3",
	[DIVOOM_VOICE_ID_SLEEP_CITY_RAIN] = "Sleep_City_Rain.mp3",
	[DIVOOM_VOICE_ID_SLEEP_CAMPFIRE] = "Sleep_Campfire.mp3",
	[DIVOOM_VOICE_ID_SLEEP_BIRD_CHIRPING] = "Sleep_Bird_Chirping.mp3",
	[DIVOOM_VOICE_ID_CLOCK_WHALE] = "Clock_Whale.mp3",
	[DIVOOM_VOICE_ID_CLOCK_URBAN] = "Clock_Urban.mp3",
	[DIVOOM_VOICE_ID_CLOCK_RAINSTORM] = "Clock_Rainstorm.mp3",
	[DIVOOM_VOICE_ID_CLOCK_OCEAN_WAVES] = "Clock_Ocean_Waves.mp3",
	[DIVOOM_VOICE_ID_CLOCK_FOREST] = "Clock_Forest.mp3",
	[DIVOOM_VOICE_ID_CLOCK_ENERGIZE] = "Clock_Energize.mp3",
	[DIVOOM_VOICE_ID_CLOCK_EAGLE] = "Clock_Eagle.mp3",
	[DIVOOM_VOICE_ID_CLOCK_CAMPANULA] = "Clock_Campanula.mp3",
	[DIVOOM_VOICE_ID_CLOCK_PIANO] = "Clock_Piano.mp3",
	[DIVOOM_VOICE_ID_CLOCK_NORMAL1] = "Clock_Normal1.mp3",
	[DIVOOM_VOICE_ID_CLOCK_NORMAL2] = "Clock_Normal2.mp3",
	[DIVOOM_VOICE_ID_CLOCK_NORMAL3] = "Clock_Normal3.mp3",
	[DIVOOM_VOICE_ID_CLOCK_GUITAR] = "alarm_guitar.mp3",
	[DIVOOM_VOICE_ID_SLEEP_NIGHT_THINK] = "sleep_night_think.mp3",
	[DIVOOM_VOICE_ID_TIME_MANAGE] = "time_manage.mp3",
	[DIVOOM_VOICE_ID_GAMEBALLIMPACT] = "gameballimpact.mp3",
	[DIVOOM_VOICE_ID_GAMEBALLEAT] = "gameballeat.mp3",
	[DIVOOM_VOICE_ID_POWER_ON] = "poweron.mp3",
	[DIVOOM_VOICE_ID_TETRIS_DIE] = "game_tetris_die.mp3",
	[DIVOOM_VOICE_ID_TETRIS_SPEED] = "game_tetris_speed.mp3",
	[DIVOOM_VOICE_ID_KALAOKE_NORMAL] = "karaoke_cn_YuanSheng.mp3",
	[DIVOOM_VOICE_ID_KALAOKE_ZHUCIREN] = "karaoke_cn_ZhuCiRen.mp3",
	[DIVOOM_VOICE_ID_KALAOKE_GESHENG] = "karaoke_cn_GeShen.mp3",
	[DIVOOM_VOICE_ID_KALAOKE_LIUXIN] = "karaoke_cn_LiuXing.mp3",
	[DIVOOM_VOICE_ID_KALAOKE_RECORD] = "karaoke_cn_LuYin.mp3",
	[DIVOOM_VOICE_ID_KALAOKE_GUAISHOU] = "karaoke_cn_robot.mp3",
	[DIVOOM_VOICE_ID_KALAOKE_WAWA] = "karaoke_cn_WaWa.mp3",
	[DIVOOM_VOICE_ID_KALAOKE_DIANYIN] = "karaoke_cn_DianYin.mp3",
	[DIVOOM_VOICE_ID_KALAOKE_DIANYIN1] = "karaoke_cn_emd_c_chord.mp3",
	[DIVOOM_VOICE_ID_KALAOKE_DIANYIN2] = "karaoke_cn_emd_d_chord.mp3",
	[DIVOOM_VOICE_ID_KALAOKE_KGE] = "karaoke_cn_KGe.mp3",
	[DIVOOM_VOICE_ID_KALAOKE_ZHIBO] = "karaoke_cn_ZhiBo.mp3",
	[DIVOOM_VOICE_ID_KALAOKE_GENERAL] = "karaoke_General.mp3",
	[DIVOOM_VOICE_ID_KALAOKE_NORMAL] = "karaoke_en_originalmode.mp3",
	[DIVOOM_VOICE_ID_KALAOKE_EN_ZHUCIREN] = "karaoke_en_hostmode.mp3",
	[DIVOOM_VOICE_ID_KALAOKE_EN_GESHENG] = "karaoke_en_singermode.mp3",
	[DIVOOM_VOICE_ID_KALAOKE_EN_LIUXIN] = "karaoke_en_popmode.mp3",
	[DIVOOM_VOICE_ID_KALAOKE_EN_RECORD] = "karaoke_en_studiomode.mp3",
	[DIVOOM_VOICE_ID_KALAOKE_EN_GUAISHOU] = "karaoke_en_robotmode.mp3",
	[DIVOOM_VOICE_ID_KALAOKE_EN_WAWA] = "karaoke_en_kidmode.mp3",
	[DIVOOM_VOICE_ID_KALAOKE_EN_DIANYIN1] = "karaoke_en_edm_c_chord.mp3",
	[DIVOOM_VOICE_ID_KALAOKE_EN_DIANYIN2] = "karaoke_en_edm_d_chord.mp3",
	[DIVOOM_VOICE_ID_KALAOKE_EN_KGE] = "karaoke_en_karaokemode.mp3",
	[DIVOOM_VOICE_ID_KALAOKE_EN_ZHIBO] = "karaoke_en_streamermode.mp3",
	[DIVOOM_VOICE_ID_AUTO_MASK] = "auto_msk.mp3",
	[DIVOOM_VOICE_ID_RAIN_STEADY] = "rain_std.mp3",
	[DIVOOM_VOICE_ID_FAN_STANDY] = "fan_stdy.mp3",
	[DIVOOM_VOICE_ID_CLOCK_SKYLINE] = "1_Skyline.mp3",
	[DIVOOM_VOICE_ID_CLOCK_COURTYARD] = "2_Courtyard_Bell.mp3",
	[DIVOOM_VOICE_ID_CLOCK_CITE_SCAPE] = "3_Cityscape.mp3",
	[DIVOOM_VOICE_ID_CLOCK_SOFT_FRADIENT] = "4_Misty_Meadow.mp3",
	[DIVOOM_VOICE_ID_CLOCK_MISTY_MEADOW] = "5.Soft_Gradient.mp3",
	[DIVOOM_VOICE_ID_CLOCK_FOREST_DAWM] = "6_Forest_Dawn.mp3",
};


int divoom_voice_read_get_filename(uint32 id,char *path)
{
	int ret = 1;
	if(id < DIVOOM_VOICE_ID_END_LOCAL)
	{
		sprintf(path,"%s%s",DIVOOM_VOICE_TON_READ_DIR_PATH,divoom_tone_path_table[id]);
	}
	else
	{
		if(DIVOOM_VOICE_ID_LEAVE_MSG == id)
		{
			divoom_manage_image_download_get_file_name(DIVOOM_MANAGE_IMAGE_LEAVEMSG_FILE, id, path);
		}
		else if(id >= DIVOOM_VOICE_ID_START_ALARM_VOICE && id <= DIVOOM_VOICE_ID_END_ALARM_VOICE) 
		{
			uint32 alarm_id = id - DIVOOM_VOICE_ID_START_ALARM_VOICE;

			divoom_manage_image_download_get_file_name(DIVOOM_MANAGE_IMAGE_ALARM_RECORD, alarm_id, path);
		}
		else if (id >= DIVOOM_VOICE_ID_START_TOMATO_VOICE && id <= DIVOOM_VOICE_ID_END_TOMATO_VOICE)
		{
			divoom_manage_image_download_get_file_name(DIVOOM_MANAGE_IMAGE_LEAVEMSG_FILE, id, path);
		}
		else if (id > DIVOOM_VOICE_ID_SLEEP_AID_RECORD_BASE && id < DIVOOM_VOICE_ID_END_SLEEP_AID_RECORD)
		{
			uint32 record_id = id - DIVOOM_VOICE_ID_SLEEP_AID_RECORD_BASE;

			divoom_manage_image_download_get_file_name(DIVOOM_MANAGE_IMAGE_SLEEP_AID_RECORD,
								   record_id, path);
		}
		else
		{
			ret = 0;
		}
	}
	return ret;
}

int divoom_voice_read_file_is_exit(uint32 id)
{
	char path[256] = {0};
	int ret;

	ret = divoom_voice_read_get_filename(id, path);
	if (!ret)
		return 0;
	if (!divoom_port_file_exists(path))
		return 0;
	if (id == (uint32)DIVOOM_VOICE_ID_LEAVE_MSG)
		return divoom_voice_clip_path_coding_type(path) != DIVOOM_TONE_CODING_NONE;
	return 1;
}

int divoom_os_com_path_suffix_is_mp3(const char *path)
{
	size_t n;
	int i;

	if (!path)
		return 0;
	n = strlen(path);
	if (n < 4)
		return 0;
	for (i = 0; i < 4; i++) {
		char a = path[n - 4 + i];
		char b = ".mp3"[i];
		if (a >= 'A' && a <= 'Z')
			a = (char)(a - 'A' + 'a');
		if (a != b)
			return 0;
	}
	return 1;
}

/* 助眠/留言：裸 PCM 前两字节偶发 0xFF Ex 时勿误判为 MP3 */
static int divoom_voice_read_header_is_mpeg_audio(const uint8 *head, T_U32 nbyte)
{
	if (nbyte >= 3U && head[0] == 0x49U && head[1] == 0x44U && head[2] == 0x33U)
		return 1;
	if (nbyte < 4U)
		return 0;
	if (head[0] != 0xFFU || (head[1] & 0xE0U) != 0xE0U)
		return 0;
	if (((head[1] >> 3) & 0x03U) == 0x01U)
		return 0;
	if (((head[1] >> 1) & 0x03U) == 0x00U)
		return 0;
	if (((head[2] >> 2) & 0x03U) == 0x03U)
		return 0;
	return 1;
}

int divoom_voice_check_coding_type(T_U8 *buf, int len)
{
	unsigned layer;

	if (!buf || len < 2)
		return DIVOOM_TONE_CODING_NONE;
	if (len >= 12
	    && buf[0] == 'R' && buf[1] == 'I' && buf[2] == 'F' && buf[3] == 'F'
	    && buf[8] == 'W' && buf[9] == 'A' && buf[10] == 'V' && buf[11] == 'E')
		return DIVOOM_TONE_CODING_WAV;
	if (len >= 3 && buf[0] == 'I' && buf[1] == 'D' && buf[2] == '3')
		return DIVOOM_TONE_CODING_MP3;
	if (buf[0] != 0xFFU || (buf[1] & 0xE0U) != 0xE0U)
		return DIVOOM_TONE_CODING_NONE;
	layer = (unsigned)((buf[1] >> 1) & 0x03U);
	if (layer == 0U)
		return DIVOOM_TONE_CODING_AAC;
	if (divoom_voice_read_header_is_mpeg_audio(buf, (T_U32)len))
		return DIVOOM_TONE_CODING_MP3;
	return DIVOOM_TONE_CODING_NONE;
}

static const char *divoom_voice_clip_fs_path(const char *path)
{
	if (!path)
		return NULL;
	if (strncmp(path, "file://", 7) == 0)
		return path + 7;
	return path;
}

int divoom_voice_clip_path_coding_type(const char *path)
{
	const char *fs_path;
	uint8 file_head[32];
	FILE *fp;
	int nr;
	int coding;

	fs_path = divoom_voice_clip_fs_path(path);
	if (!fs_path || fs_path[0] == '\0')
		return DIVOOM_TONE_CODING_NONE;
	if (divoom_app_com_get_file_size((char *)fs_path) < DIVOOM_TONE_MIN_AUDIO_BYTES)
		return DIVOOM_TONE_CODING_NONE;
	fp = divoom_port_file_open((char *)fs_path, "rb");
	if (!fp)
		return DIVOOM_TONE_CODING_NONE;
	memset(file_head, 0, sizeof(file_head));
	nr = (int)divoom_port_file_read(file_head, 1, sizeof(file_head), fp);
	divoom_port_file_close(fp);
	if (nr < 2)
		return DIVOOM_TONE_CODING_NONE;
	coding = divoom_voice_check_coding_type(file_head, nr);
	return coding;
}



/*********************************************************************
  Function:     divoom_voice_readInit
  Description:    ?????
  Input:            
  Return:           
  Author:           AIJUN
  Data:            2013-6-9
**********************************************************************/
T_VOID divoom_voice_read_init(T_VOID)
{
}

/*********************************************************************
  Function:     divoom_voice_read_check_file_ok
  Description:    检查提示文件是否正常，一般第一个文件的偏移要是1024
  Input:            
  Return:           
  Author:           AIJUN
  Data:            2013-6-9
**********************************************************************/
T_U8 divoom_voice_read_check_file_ok(T_VOID)
{
	return 1;
}

/*********************************************************************
  Function:     divoom_voice_get_voice_info
  Description:  得到一个提示音在ＦＬＡＳＨ的相关信息
  Input:            T_U32 VoiceID  :提示音ID
                    start
                    size
  Return:           返回提示音句柄
  Author:           AIJUN
  Data:            2013-6-9
**********************************************************************/

T_BOOL divoom_voice_get_voice_info(T_U32 VoiceID, T_U32 *start, T_U32 *size)
{
}


T_BOOL divoom_voice_read_open_voice_file(T_U32 VoiceID, T_PVOICE_TIP_HANDLE pVoiceHandle)
{


}

/*********************************************************************
  Function:     divoom_voice_read_open_handle
  Description:     打开一个提示音，返回句柄，可以让系统读到提示音数据
  Input:            T_U32 VoiceID  :提示音ID
  Return:           返回提示音句柄
  Author:           AIJUN
  Data:            2013-6-9
**********************************************************************/

T_U32 divoom_voice_read_open_handle(T_U32 VoiceID, T_U32 *voice_size)
{
	return 0;
}

void divoom_voice_read_seek(T_U32 VoiceHandle, T_U32 seek, int mode)
{
}

T_U32 divoom_voice_get_file_size(T_U32 VoiceHandle)
{
	return 0;
}


/*********************************************************************
  Function:     divoom_voice_readResetReadHandle
  Description:    将从开始读提示音
  Input:            T_U32 VoiceHandle  :提示音句柄
  Return:           NONE
  Author:           AIJUN
  Data:            2013-6-9
**********************************************************************/
T_VOID divoom_voice_read_reset_handle(T_U32 VoiceHandle)
{


}

/*********************************************************************
  Function:     divoom_voice_readReadHandle
  Description:     读提示音数据，给音频库解码
  Input:            T_U32 VoiceHandle  :提示音句柄
  			  T_pVOID buf :提示音缓冲器
  			  T_U32 size    :提示音缓冲器大小
  Return:           返回提示音读到的数据大小
  Author:           AIJUN
  Data:            2013-6-9
**********************************************************************/
T_U32 divoom_voice_read_read_handle(T_U32 VoiceHandle, T_U8 * buf, T_U32 size)
{
	return 0;
}

/*********************************************************************
  Function:     divoom_voice_readCloseHandle
  Description:     关闭提示音句柄，释放提示音申请的内存
  Input:           T_U32 VoiceHandle  :提示音句柄
  Return:           NONE
  Author:           AIJUN
  Data:            2013-6-9
**********************************************************************/
T_VOID divoom_voice_read_close_handle(T_U32 VoiceHandle)
{
	
}




