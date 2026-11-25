#ifndef _divoom_system_setting_json_h_
#define _divoom_system_setting_json_h_
typedef struct _divoom_system_setting_json_system_cfg
{
    int DisableMic;
    int CurClockId;
    int StartUpClockOnOff;
    int RotationFlag;
    int ClockTime;
    int GalleryTime;
    int SingleGalleyTime;
    int ChannelIndex;
    int PowerOnClockId;
    int GalleryShowTimeFlag;
	uint8 Time24Flag;
    uint8 TemperatureMode;
	uint8 WindSpeed_flag;
    uint8 GyrateAngle;
    uint8 ShowTime_flag;
    int   Brightness;
    int   WhiteBalanceR;
    int   WhiteBalanceG;
    int   WhiteBalanceB;
    int   time_format;
	uint8 ShowGridflag;
    uint8 AutoUpdateFlag;
    uint8 MirrorFlag;
	int	  ColorTemp;
    int   PowerMode;
	int   Language;
	int   SoundEffectMode; /* 0~4=音效预设，-1=JSON 未带该字段 */
	int   NotificationSound;
	uint8 SProt;
	float Longitude;
	float Latitude;
    char  TimeZoneValue[64];
	char startup_logo[64];
}divoom_system_setting_json_system_cfg, *Pdivoom_system_setting_json_system_cfg;

typedef struct _divoom_system_setting_json_night_view
{
    uint8 work_status;  //是否在工作状态
	uint8 on_off;       //开关
    uint8 bright;       //亮度
    uint16 start;       //开始时间
    uint16 end;         //结束时间
}divoom_system_setting_json_night_view, *Pdivoom_system_setting_json_night_view;

typedef struct _divoom_system_setting_spirit_conf
{
    int image_index;
    int voice_index;
}divoom_system_setting_spirit_conf, *Pdivoom_system_setting_spirit_conf;

typedef struct _divoom_system_setting_device_location
{
    char Longi[10];
    char Lati[10];
}divoom_system_setting_device_location, *Pdivoom_system_setting_device_location;

typedef struct _divoom_system_setting_device_poweron_pic
{
    char pic[52];
    char voice[52];
}divoom_system_setting_device_poweron_pic, *Pdivoom_system_setting_device_poweron_pic;


#define DIVOOM_SYSTEM_SPIRIT_IMAGE_MAX 4
typedef struct _divoom_system_setting_get_spirit_image_list
{
    char file_id_list[DIVOOM_SYSTEM_SPIRIT_IMAGE_MAX][256];
}divoom_system_setting_get_spirit_image_list, *Pdivoom_system_setting_get_spirit_image_list;



typedef struct _divoom_system_setting_birghtness_conf
{
    int light;
}divoom_system_setting_birghtness_conf, *Pdivoom_system_setting_birghtness_conf;


int divoom_system_setting_json_set_setting_cfg_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_system_setting_json_get_setting_cfg_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_system_setting_json_set_back_color_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_system_setting_json_set_night_view_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_system_setting_json_set_night_view_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

void divoom_system_setting_json_get_setting_cfg_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_system_setting_json_set_setting_cfg_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);


int divoom_system_setting_json_get_spirit_cfg_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_system_setting_json_get_spirit_cfg_respone_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

int divoom_system_setting_json_set_spirit_cfg_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_system_setting_json_set_setting_cfg_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_system_setting_json_set_setting_cfg_respone_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

int divoom_system_setting_json_get_setting_cfg_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_system_setting_json_get_setting_cfg_respone_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_system_setting_json_set_music_cfg_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

int divoom_system_setting_json_set_systime_format_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_system_setting_json_set_systime_format_respone_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

int divoom_system_setting_json_set_sys_shake_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_system_setting_json_set_sys_shake_respone_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

int divoom_system_setting_json_set_sys_sound_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_system_setting_json_set_sys_sound_respone_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

int divoom_system_setting_json_set_sys_engergy_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_system_setting_json_set_sys_engergy_respone_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

int divoom_system_setting_json_set_sys_metric_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_system_setting_json_set_sys_metric_respone_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

int divoom_system_setting_json_set_sys_brightness_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_system_setting_json_set_sys_brightness_respone_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_system_setting_json_set_voice_volumn_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_system_setting_json_set_text_dir_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

int divoom_system_setting_json_set_sys_autopower_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_system_setting_json_set_sys_autopower_respone_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

int divoom_system_setting_json_set_sys_logo_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_system_setting_json_set_sys_poweron_text_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_system_setting_json_set_device_location_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_system_setting_json_set_sys_logo_respone_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

void divoom_system_setting_json_get_sys_brightness_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_system_setting_json_get_sys_brightness_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

int divoom_system_setting_json_set_poweron_mode_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_system_setting_json_set_device_time_zone_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_system_setting_json_set_device_time_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

#endif


