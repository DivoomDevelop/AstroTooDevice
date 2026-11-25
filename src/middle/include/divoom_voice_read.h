#ifndef _divoom_voice_readREAD_H_
#define _divoom_voice_readREAD_H_

/** 与 flowtoo divoom_tone_service 一致：MP2 裸流也归 MP3 解码路径 */
enum
{
	DIVOOM_TONE_CODING_MP3 = 0,
	DIVOOM_TONE_CODING_AAC = 1,
	DIVOOM_TONE_CODING_WAV = 2,
	DIVOOM_TONE_CODING_NONE,
};

enum
{
    DIVOOM_VOICE_ID_CONNECTED,
    DIVOOM_VOICE_ID_GAMEANSWER,
    DIVOOM_VOICE_ID_GAMEDICE,
    DIVOOM_VOICE_ID_GAMEGUESS,
    DIVOOM_VOICE_ID_GAMEOVER,
    DIVOOM_VOICE_ID_GAMEPLANEOVER,
    DIVOOM_VOICE_ID_GAMETARGE,
    DIVOOM_VOICE_ID_LOWPOWER,
    DIVOOM_VOICE_ID_PAIRING,
    DIVOOM_VOICE_ID_POWEROFF,
    DIVOOM_VOICE_ID_SLEEP_WHITE_NOISE,
    DIVOOM_VOICE_ID_SLEEP_WHISPER,
    DIVOOM_VOICE_ID_SLEEP_WATER_DROP,
    DIVOOM_VOICE_ID_SLEEP_UNDER_WATER,
    DIVOOM_VOICE_ID_SLEEP_THUNDER,
    DIVOOM_VOICE_ID_SLEEP_SUMMER,
    DIVOOM_VOICE_ID_SLEEP_SNOWSTORM,
    DIVOOM_VOICE_ID_SLEEP_SEASHORE,
    DIVOOM_VOICE_ID_SLEEP_ROSA_CREEK,
    DIVOOM_VOICE_ID_SLEEP_RELAX,
    DIVOOM_VOICE_ID_SLEEP_NIGHT_FALL,
    DIVOOM_VOICE_ID_SLEEP_MUSIC_BOX,
    DIVOOM_VOICE_ID_SLEEP_MIDSUMMER,
    DIVOOM_VOICE_ID_SLEEP_MEDITATE,
    DIVOOM_VOICE_ID_SLEEP_LULLABY,
    DIVOOM_VOICE_ID_SLEEP_GARDEN,
    DIVOOM_VOICE_ID_SLEEP_FROGS,
    DIVOOM_VOICE_ID_SLEEP_DIVING,
    DIVOOM_VOICE_ID_SLEEP_COUNTRYSIDE,
    DIVOOM_VOICE_ID_SLEEP_CITY_RAIN,
    DIVOOM_VOICE_ID_SLEEP_CAMPFIRE,
    DIVOOM_VOICE_ID_SLEEP_BIRD_CHIRPING,
    DIVOOM_VOICE_ID_CLOCK_WHALE,
    DIVOOM_VOICE_ID_CLOCK_URBAN,
    DIVOOM_VOICE_ID_CLOCK_RAINSTORM,
    DIVOOM_VOICE_ID_CLOCK_OCEAN_WAVES,
    DIVOOM_VOICE_ID_CLOCK_FOREST,
    DIVOOM_VOICE_ID_CLOCK_ENERGIZE,
    DIVOOM_VOICE_ID_CLOCK_EAGLE, 
    DIVOOM_VOICE_ID_CLOCK_CAMPANULA,
    DIVOOM_VOICE_ID_CLOCK_PIANO,
    DIVOOM_VOICE_ID_CLOCK_NORMAL1,
    DIVOOM_VOICE_ID_CLOCK_NORMAL2,
    DIVOOM_VOICE_ID_CLOCK_NORMAL3,    
    DIVOOM_VOICE_ID_CLOCK_GUITAR,
    DIVOOM_VOICE_ID_SLEEP_NIGHT_THINK,
    DIVOOM_VOICE_ID_TIME_MANAGE,
    DIVOOM_VOICE_ID_GAMEBALLIMPACT,
    DIVOOM_VOICE_ID_GAMEBALLEAT,
    DIVOOM_VOICE_ID_POWER_ON,
    DIVOOM_VOICE_ID_TETRIS_DIE,
    DIVOOM_VOICE_ID_TETRIS_SPEED,
    DIVOOM_VOICE_ID_KALAOKE_NORMAL,    //原音模式
    DIVOOM_VOICE_ID_KALAOKE_ZHUCIREN,  //主持人
    DIVOOM_VOICE_ID_KALAOKE_GESHENG,   //歌神模式
    DIVOOM_VOICE_ID_KALAOKE_LIUXIN,     //流行模式
    DIVOOM_VOICE_ID_KALAOKE_RECORD,     //录音棚模式
    DIVOOM_VOICE_ID_KALAOKE_GUAISHOU,  //怪兽模式
    DIVOOM_VOICE_ID_KALAOKE_WAWA,      //娃娃音模式
    DIVOOM_VOICE_ID_KALAOKE_DIANYIN,     //电音模式
    DIVOOM_VOICE_ID_KALAOKE_DIANYIN1,     //电音模式1
    DIVOOM_VOICE_ID_KALAOKE_DIANYIN2,     //电音模式2
    DIVOOM_VOICE_ID_KALAOKE_KGE,      //k歌模式
    DIVOOM_VOICE_ID_KALAOKE_ZHIBO,     //直播模式
    DIVOOM_VOICE_ID_KALAOKE_GENERAL,   //通用模式
    DIVOOM_VOICE_ID_KALAOKE_EN_NORMAL,    //原音模式
    DIVOOM_VOICE_ID_KALAOKE_EN_ZHUCIREN,  //主持人
    DIVOOM_VOICE_ID_KALAOKE_EN_GESHENG,   //歌神模式
    DIVOOM_VOICE_ID_KALAOKE_EN_LIUXIN,     //流行模式
    DIVOOM_VOICE_ID_KALAOKE_EN_RECORD,     //录音棚模式
    DIVOOM_VOICE_ID_KALAOKE_EN_GUAISHOU,  //怪兽模式
    DIVOOM_VOICE_ID_KALAOKE_EN_WAWA,      //娃娃音模式
//    DIVOOM_VOICE_ID_KALAOKE_EN_DIANYIN,     //电音模式
    DIVOOM_VOICE_ID_KALAOKE_EN_DIANYIN1,     //电音模式1
    DIVOOM_VOICE_ID_KALAOKE_EN_DIANYIN2,     //电音模式2
    DIVOOM_VOICE_ID_KALAOKE_EN_KGE,      //k歌模式
    DIVOOM_VOICE_ID_KALAOKE_EN_ZHIBO,     //直播模式    
    DIVOOM_VOICE_ID_AUTO_MASK,     //入睡后噪音
    DIVOOM_VOICE_ID_RAIN_STEADY,     //入睡后噪音
    DIVOOM_VOICE_ID_FAN_STANDY,     //入睡后噪音 
    DIVOOM_VOICE_ID_CLOCK_SKYLINE,       //Skyline
    DIVOOM_VOICE_ID_CLOCK_COURTYARD,       //Courtyard_Bell
    DIVOOM_VOICE_ID_CLOCK_CITE_SCAPE,       //Cityscape
    DIVOOM_VOICE_ID_CLOCK_SOFT_FRADIENT,       //Soft_Gradient
    DIVOOM_VOICE_ID_CLOCK_MISTY_MEADOW,       //Misty_Meadow
    DIVOOM_VOICE_ID_CLOCK_FOREST_DAWM,       //Forest_Dawn
    DIVOOM_VOICE_ID_END_LOCAL,
    DIVOOM_VOICE_ID_LEAVE_MSG = 0XF0,
    DIVOOM_VOICE_ID_POWERON_VOICE = 0XF2,
	DIVOOM_VOICE_ID_LEAVE_MSG_VOICE_PIXEL,  //留言显示图像

	DIVOOM_VOICE_ID_START_ALARM_VOICE = 0X10000000, 
	DIVOOM_VOICE_ID_END_ALARM_VOICE = 0X20000000,
	DIVOOM_VOICE_ID_START_TOMATO_VOICE = 0X20000001,
	DIVOOM_VOICE_ID_END_TOMATO_VOICE = 0X30000000,
	/** 本地睡眠助眠录音：`playable_id = DIVOOM_VOICE_ID_SLEEP_AID_RECORD_BASE + record_id`（record_id 固定 1~5 循环复用）。文件路径见 `DIVOOM_MANAGE_IMAGE_SLEEP_AID_RECORD`。 */
	DIVOOM_VOICE_ID_SLEEP_AID_RECORD_BASE = 0x30000000,
	DIVOOM_VOICE_ID_END_SLEEP_AID_RECORD = 0x31000000,
    DIVOOM_VOICE_ID_MAX,
};

int divoom_voice_read_get_filename(uint32 id,char *path);

/*********************************************************************
  Function:     divoom_voice_readInit
  Description:    ?????
  Input:            
  Return:           
  Author:           AIJUN
  Data:            2013-6-9
**********************************************************************/
T_VOID divoom_voice_read_init(T_VOID);


/*********************************************************************
  Function:     divoom_voice_readOpenHandle
  Description:     打开一个提示音，返回句柄，可以让系统读到提示音数据
  Input:            T_U32 VoiceID  :提示音ID
  Return:           返回提示音句柄
  Author:           AIJUN
  Data:            2013-6-9
**********************************************************************/
T_U32 divoom_voice_read_open_handle(T_U32 VoiceID, T_U32 *voice_size);
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
T_U32 divoom_voice_read_read_handle(T_U32 VoiceHandle, T_U8 *buf, T_U32 size);
/*********************************************************************
  Function:     divoom_voice_readCloseHandle
  Description:     关闭提示音句柄，释放提示音申请的内存
  Input:           T_U32 VoiceHandle  :提示音句柄
  Return:           NONE
  Author:           AIJUN
  Data:            2013-6-9
**********************************************************************/
T_VOID divoom_voice_read_close_handle(T_U32 VoiceHandle);

T_U32 divoom_voice_get_file_size(T_U32 VoiceHandle);

/*********************************************************************
  Function:     divoom_voice_readResetReadHandle
  Description:    将从开始读提示音
  Input:            T_U32 VoiceHandle  :提示音句柄
  Return:           NONE
  Author:           AIJUN
  Data:            2013-6-9
**********************************************************************/
T_VOID divoom_voice_read_reset_handle(T_U32 VoiceHandle);


/*********************************************************************
  Function:     divoom_voice_read_check_file_ok
  Description:    检查提示文件是否正常，一般第一个文件的偏移要是1024
  Input:            
  Return:           
  Author:           AIJUN
  Data:            2013-6-9
**********************************************************************/
T_U8 divoom_voice_read_check_file_ok(T_VOID);

int divoom_voice_read_file_is_exit(uint32 id);

/** 路径后缀是否为 .mp3（大小写不敏感），flowtoo: divoom_os_com_path_suffix_is_mp3 */
int divoom_os_com_path_suffix_is_mp3(const char *path);

/**
 * 根据文件头判断编码：MP2/MP3/ID3→MP3；AAC-ADTS→AAC；WAV→WAV；无法识别→NONE。
 * flowtoo: divoom_voice_check_coding_type
 */
int divoom_voice_check_coding_type(T_U8 *buf, int len);

/** 读文件头并调用 divoom_voice_check_coding_type；可播放返回非 NONE */
int divoom_voice_clip_path_coding_type(const char *path);

#endif


