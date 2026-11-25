#ifndef _divoom_device_json_h_
#define _divoom_device_json_h_
#include "divoom_voice_json.h"
typedef struct _divoom_device_app_id_item
{
    uint8 status;   //为１时收到心跳要发送DIVOOM_NET_COMM_DEVICE_CON_APP,否则不管
    char ip_addr[32];
    int user_id;
    int con_flag;
    int reset_time;
    int fail_time;
    int send_heart_time;
}divoom_device_app_id_item, *Pdivoom_device_app_id_item;
typedef struct _divoom_device_app_id_list
{
    int num;
    Pdivoom_device_app_id_item list;
    
}divoom_device_app_id_list, *Pdivoom_device_app_id_list;
typedef struct _divoom_device_userlist_name_id_info
{
    int num;
    int user_id[30];
    char user_name[30][64];    
}divoom_device_userlist_name_id_info, *Pdivoom_device_userlist_name_id_info;

typedef struct _divoom_device_ser_mqtt_info_json_request
{
    int server_type;
    int reset_all_flag;
    int mqtt_con_time;
    int bond_flag;
    int product_flag;
    int language;
    int hot_user_id;
    int HardWare;
    int device_id;
	char device_name[64];
}divoom_device_ser_mqtt_info_json_request, *Pdivoom_device_ser_mqtt_info_json_request;

typedef struct _divoom_device_ser_mqtt_info_json
{
    uint32 utc_time;
    int reset_flag;
	int time_zone;
    int log_level;
    int mqtt_port;
	int summer_zone;
    int last_clock_id;
    char lot_str[10];
    char lat_str[10];    
	char dev_ip[32];
	char tzcode[64];
	//char dev_id[64];
	int user_id;
	int new_device;
	int dev_id;
    int server_type;
	char dev_token[512];
}divoom_device_ser_mqtt_info_json, *Pdivoom_device_ser_mqtt_info_json;

typedef struct _divoom_devcice_con_json
{
    int custom_id;
    int hd_version;
    int soft_version;
	int soft_version2;
    char ssid[128];
    char dev_ip[32];
    char net_ip[32];
    char mac_addr[32];
    char net_mask[32];
}divoom_devcice_con_json, *Pdivoom_devcice_con_json;

typedef struct _divoom_devcice_con_respone_json
{
    int  index;
    int  version;
    int  expert_num;
    int  expert_list[DIVOOM_DEVICE_SUSCRIBE_EXPERT_NUM];
}divoom_devcice_con_respone_json, *Pdivoom_devcice_con_respone_json;

typedef struct _divoom_devcice_expert_request_json
{
    int  expert_num;
    int  file_num;
    int  expert_list[DIVOOM_DEVICE_SUSCRIBE_EXPERT_NUM];
    int  upload_time[DIVOOM_DEVICE_SUSCRIBE_EXPERT_NUM];
}divoom_devcice_expert_request_json, *Pdivoom_devcice_expert_request_json;

typedef struct _divoom_devcice_bind_user_json
{
    int user_id;
    char mac_addr[32];
}divoom_devcice_bind_user_json, *Pdivoom_devcice_bind_user_json;


typedef struct _divoom_devcice_con_app_json
{
    int con_flag;
    int user_id;
}divoom_devcice_con_app_json, *Pdivoom_devcice_con_app_json;

typedef struct _divoom_device_con_ret_ext_topic_json
{
	//扩展topic
	char exttopic[64];
}divoom_device_con_ret_ext_topic_json, *Pdivoom_device_con_ret_ext_topic_json;

typedef struct _divoom_devcice_con_ret_json
{
    int utc_time;
    int software_version;
	int exttopic_num;
	int voice_num;
	int time_zone;
    char lot_str[10];
    char lat_str[10];    
    char voice_img_id[64];
	Pdivoom_device_con_ret_ext_topic_json exttopic;
    Pdivoom_voice_json_voice_item_info pvoice_table;
}divoom_devcice_con_ret_json, *Pdivoom_edvcice_con_ret_json;

typedef struct _divoom_devcice_upgrade_json
{
    int hd_version;
    int soft_version;
    char md5[256];
    char file_id[256];
}divoom_devcice_upgrade_json, *Pdivoom_devcice_upgrade_json;

typedef struct _divoom_devcice_image_file
{
    int user_id;
    int image_id;
    int size;
    int utc_time;
    char file_id[56];
    char image_name[200];
}divoom_devcice_image_file, *Pdivoom_devcice_image_file;

typedef struct _divoom_devcice_user_define_item
{
    int clock_id;
    int clock_pos;
    int parent_id;
    int image_id;
    int type;           //0:增加；1：删除
    int utc_time;
    char file_id[56];
    char voice_id[56];
    int author_id;
    char author_name[200];
    char author_file_id[64];
    char image_name[200];
}divoom_devcice_user_define_item, *Pdivoom_devcice_user_define_item;


typedef struct _divoom_devcice_image_list
{
    int num;
    int album_id;
    int all_flag;
    int last_update_time;
    int unlike_num;
    int unlike_list[1024];
    Pdivoom_devcice_image_file pfile_list;
}divoom_devcice_image_list, *Pdivoom_devcice_image_list;


typedef struct _divoom_devcice_expert_file_item
{
    int image_id;
    int utc_time;
    char file_id[56];
    char image_name[200];
}divoom_devcice_expert_file_item, *Pdivoom_devcice_expert_file_item;

typedef struct _divoom_devcice_expert_item
{
    int user_id;
    int num;
    Pdivoom_devcice_expert_file_item pfile_list;
}divoom_devcice_expert_item, *Pdivoom_devcice_expert_item;

typedef struct _divoom_devcice_expert_list
{
    int num;
    int update_time;
    Pdivoom_devcice_expert_item puser_list;
}divoom_devcice_expert_list, *Pdivoom_devcice_expert_list;

typedef struct _divoom_devcice_expert_request_item
{
    int num;
    int *user_list;
}divoom_devcice_expert_request_item, *Pdivoom_devcice_expert_request_item;

typedef struct _divoom_devcice_hot_list_request
{
    int size;
    int utc_time;
}divoom_devcice_hot_list_request, *Pdivoom_devcice_hot_list_request;

typedef struct _divoom_devcice_user_define_request
{
    int update_time;
    int type;
}divoom_devcice_user_define_request, *Pdivoom_devcice_user_define_request;


typedef struct _divoom_devcice_user_define_respone
{
    int type;
    int file_num;
    int all_flag;
    Pdivoom_devcice_user_define_item pfile_list;    
}divoom_devcice_user_define_respone, *Pdivoom_devcice_user_define_respone;

typedef struct _divoom_system_font_item
{
    int id;
    int type;
    char url[56];    
    char charset[512];
}divoom_system_font_item, *Pdivoom_system_font_item;

typedef struct _divoom_devcice_time_dial_font_list_respone
{
    int font_num;
    Pdivoom_system_font_item pfont_list;
    int font_not_num;
    int *pfont_not_list;
}divoom_devcice_time_dial_font_list_respone, *Pdivoom_devcice_time_dial_font_list_respone;


typedef struct _divoom_devcice_single_font_list_respone
{
    int font_num;
    int *pfont_list;    
}divoom_devcice_single_font_list_respone, *Pdivoom_devcice_single_font_list_respone;

typedef struct _divoom_app_pic_manage_item
{
    int app_id;
    char addr[56];
}divoom_app_pic_manage_item, *Pdivoom_app_pic_manage_item;


typedef struct _divoom_devcice_time_dial_app_pic_respone
{
    int num;
    Pdivoom_app_pic_manage_item plist;    
}divoom_devcice_time_dial_app_pic_respone, *Pdivoom_devcice_time_dial_app_pic_respone;

typedef struct _divoom_app_clock_list_item
{
    int clock_id;
    int clock_pos;
    int parent_id;
    int update_time;
}divoom_app_clock_list_item, *Pdivoom_app_clock_list_item;


typedef struct _divoom_devcice_time_dial_clock_list_respone
{
    int num;
    Pdivoom_app_clock_list_item plist;    
}divoom_devcice_time_dial_clock_list_respone, *Pdivoom_devcice_time_dial_clock_list_respone;


typedef struct _divoom_devcice_play_buzzer_respone
{
    int on_time;
    int off_time;
    int tol_time;
}divoom_devcice_play_buzzer_respone, *Pdivoom_devcice_play_buzzer_respone;



typedef struct _divoom_clock_attr_item_info
{
    int AppID;
    int DivoomAppType;
    char ImgAddr[56];    
    char *UrlInfo;
    char *Config;
    char *Title;    //使用，分开,使用DIVOOM_CLOCK_DISP_SUPPORT_APP_ITEM_NAME
    char notice_rule[64];
    int  notice_like_count;
    int  notice_value_count;
    char AppName[64];  //人名，或者股票名，使用DIVOOM_CLOCK_DISP_SUPPORT_APP_TITLE
    char AppNameRule[64];  //使用DIVOOM_CLOCK_DISP_SUPPORT_APP_TITLE的解析规则
    int UpdateTime;
    int url_head_num;
    Pdivoom_channel_item_url_head_info purl_head;
    int DispNumber;
    Pdivoom_channel_item_disp_data_info pDispData;    
}divoom_clock_attr_info, *Pdivoom_clock_attr_info;


typedef struct _divoom_disp_clock_disp_item
{
    int id;     //显示ＩＤ
    int update_time;
    int disp_value;
    int x;
    int y;
    int w;
    int h;
    int type;
    int font_size;
    int font;
    int align;
    int sep;
    uint8 front_color[3];
    uint8 back_color[3];
    int image_id;
    int angle;  //旋转角度
    int hier;   //图层
    int transp; //透明度
	int animation; //动画效果
    char img_addr[52];
    char    item_id[64];        //对应配置的字串
    void *pdisp_handle;
    void *pdisp_handle2;
    void *pdisp_handle3;
    void *pdisp_handle4;
	void *pdisp_handle5;
	void *pdisp_clock_ctrl;
}divoom_disp_clock_disp_item, *Pdivoom_disp_clock_disp_item;

typedef struct _divoom_clock_alarm_info
{
    uint8  status;      //0:关闭；1：打开；2：没有选项
    uint8  voice;
    uint32 start_time;
    uint32 end_time;
    uint32 replay_flag;
}divoom_clock_alarm_info, *Pdivoom_clock_alarm_info;

typedef struct _divoom_clock_manage_item
{
    uint8  load_app_config;  //是否要重新获取APP配置
    uint8  is_app;
    uint8  flag;             //是否需要更新
    uint8  status;          //用户自定义表盘；  0 未公开 1-公开待审用户未回复 2-公开待审管理员未回复 3-审核通过 4-审核不通过
    uint8  dial_config_timeout;  //配置是否过期   
    int    dial_type;        //对应后面的配置设备类型，用来区分不同的表盘
    uint8  clock_type;       //表盘类型,0:为普通表盘，其它的为像素艺术表盘，会有边框
	uint8 	component_type; 	// 1 组件表盘 0非组件表盘
    uint16 ai_schedule_type; // AI 两级调度子类；0 表示按旧字段自动识别
    uint8   style_clock_num;    //有多少个风格表盘
    uint32  clock_id;        //表盘ＩＤ
    uint32  clock_pos;       //在父表盘的位置，如果为最上级就为0
    uint32  parent_clock_id;       //父表盘的ID，非子表盘为0
    uint32  edge_clock_id;   //风格表盘ID,
    uint32  style_clock_list[20];  //这个表盘有的风格列表ID,
    uint32  style_clock_id;  //风格表盘ID,
    uint32  compent_clock_id;  //在AI表盘应用中的组件表盘ID,
    uint32  sys_update_time;    //表盘基本配置更新时间 
    uint32  update_time;     //表盘APP配置更新的时间
    uint32  set_color_table[4];     //组件颜色配置
    char    name_cn[64];            //表盘中文名称
    char    name_en[64];            //表盘英文名称
    char    image_addr[56];     //表盘的图像ＩＤ
    char    *desc_info;         //描述字段
    char    review_addr[56];     //表盘的图像ＩＤ
	char	review_addr2[56]; 	//表盘预览大图
	char    video_addr[56];     //播放表盘前，是否播放视频先
    int     app_num;           //ＡＰＰ对应的数目
    int     disp_item_num;     //显示项数目
    divoom_clock_alarm_info alarm_info; //闹钟相关信息
    Pdivoom_disp_clock_disp_item pdisp_item_list;   //显示列表
    Pdivoom_clock_attr_info papp_list;        //表盘对应的ＡＰＰ配置列表
}divoom_clock_manage_item, *Pdivoom_clock_manage_item;

typedef struct _divoom_device_update_clock_info_request
{
    Pdivoom_clock_manage_item clock_info;
    int classify_id;
    const char *message_info;
}divoom_device_update_clock_info_request, *Pdivoom_device_update_clock_info_request;

typedef struct _divoom_device_clock_list_info
{
    int num;
    int update_time;
    int is_app;
    Pdivoom_clock_manage_item list;
}divoom_device_clock_list_info, *Pdivoom_device_clock_list_info;

typedef struct _divoom_component_list_item
{
    int component_id;
    int update_time;
}divoom_component_list_item, *Pdivoom_component_list_item;

typedef struct _divoom_devcice_time_component_list_respone
{
    int num;
    Pdivoom_component_list_item plist;    
}divoom_devcice_time_component_list_respone, *Pdivoom_devcice_time_component_list_respone;

typedef struct _divoom_device_play_tf_gif_info
{
    int flag;   //２：网络文件；1:本地文件夹，０：本地文件
    char file_name[256];
}divoom_device_play_tf_gif_info, *Pdivoom_device_play_tf_gif_info;


typedef struct _divoom_device_clock_history_info
{
    int num;
    int *list;
}divoom_device_clock_history_info, *Pdivoom_device_clock_history_info;

typedef struct _divoom_device_china_day_info
{
    char LunarDateTime[64];
	char LJie[64];
    char Yi[64];
    char Ji[64];
	char ShenWei[64];
	char Taishen[128];
	char Chong[64];
	char SuiSha[64];
	char WuxingJiazi[64];
	char WuxingNaYear[64];
	char WuxingNaMonth[64];
	char WuxingNaDay[64];
	char PengZu[64];
    char XingWest[64];
    char LYear[64];
    char LMonth[64];
    char LDay[64];
    char TianGanDiZhiYear[64];
    char TianGanDiZhiMonth[64];
    char TianGanDiZhiDay[64];
	char SolarTermName[64];
}divoom_device_china_day_info, *Pdivoom_device_china_day_info;


void divoom_device_bond_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_device_bond_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

void divoom_device_connect_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_device_connect_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_device_upgrade_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_device_connect_app_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_device_connect_app_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_device_set_blue_name_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_device_set_device_name_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_device_get_blue_name_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_device_get_blue_name_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_device_get_device_id_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_device_get_device_id_request_pack(divoom_cJSON * root, Pdivoom_net_command pcmd_info);

void divoom_get_app_id_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_device_init_to_clear_user_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_device_disconnect_server_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_app_unbind_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_app_del_user_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_get_app_id_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_push_app_id_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

typedef struct _divoom_device_get_user_info_list_request_info
{
    int user_num;
    uint32 *user_list;
}divoom_device_get_user_info_list_request_info, *Pdivoom_device_get_user_info_list_request_info;

typedef struct _divoom_device_get_user_info_list_respone_item
{
    uint32 user_id;
    char user_name[200];
    char file_id[64];
}divoom_device_get_user_info_list_respone_item, *Pdivoom_device_get_user_info_list_respone_item;

typedef struct _divoom_device_get_user_info_list_respone_info
{
    int user_num;
    Pdivoom_device_get_user_info_list_respone_item puser_list;
}divoom_device_get_user_info_list_respone_info, *Pdivoom_device_get_user_info_list_respone_info;

void divoom_device_get_user_info_list_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_device_get_user_info_list_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_device_get_storage_status_respone_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_device_set_storage_status_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_device_save_screen_snapshot(void);
void divoom_device_get_screen_snapshot_respone_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

typedef struct _divoom_device_update_test_file_respone_info
{
    char file_id[512]; 
    char dest_info[1024];
}divoom_device_update_test_file_respone_info, *Pdivoom_device_update_test_file_respone_info;

void divoom_device_update_test_file_respone_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_device_update_test_file_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

void divoom_device_get_password_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_device_get_password_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

typedef struct _divoom_device_public_ser_mqtt_lwt_request_info
{
    int flag; 
    char mac_addr[16];
}divoom_device_public_ser_mqtt_lwt_request_info, *Pdivoom_device_public_ser_mqtt_lwt_request_info;

void divoom_device_public_ser_mqtt_lwt_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_device_app_mqtt_lwt_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_device_app_mqtt_lwt_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

void divoom_device_get_ser_twitter_sign_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_device_get_ser_twitter_sign_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

void divoom_device_get_ser_mqtt_info_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_device_get_ser_mqtt_info_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_device_get_china_day_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

void divoom_device_get_userlist_name_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_device_get_userlist_name_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

void divoom_device_get_hot_last_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_device_get_hot_last_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

void divoom_device_get_hot_list_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_device_get_hot_list_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

void divoom_device_get_expert_list_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

int divoom_device_get_expert_list_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_device_get_expertv2_list_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

void divoom_device_get_expert_last_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

int divoom_device_get_expert_last_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_device_update_expert_last_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_device_exit_subscribe_dis_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

void divoom_device_get_user_define_list_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

int divoom_device_get_user_define_list_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

void divoom_device_get_favorite_last_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_device_get_favorite_last_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

void divoom_device_get_favorite_list_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_device_get_favorite_list_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);


void divoom_device_get_time_dial_font_list_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_device_get_time_dial_font_list_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_device_get_single_font_item_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_device_get_single_font_item_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

void divoom_device_get_time_dial_app_pic_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_device_get_time_dial_app_pic_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

void divoom_device_get_clock_config_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_device_get_clock_config_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
Pdivoom_disp_clock_disp_item divoom_device_get_clock_config_unpack_all_disp_item(divoom_cJSON *root, int *num);
int divoom_device_get_clock_sys_config_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_device_get_clock_app_config_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_device_set_album_time_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
typedef struct _divoom_device_get_album_time_list_info
{
    int number;
    Pdivoom_channel_user_define_schedule_info plist;
}divoom_device_get_album_time_list_info, *Pdivoom_device_get_album_time_list_info;

void divoom_device_get_album_time_list_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

int divoom_device_get_album_time_list_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_device_get_clock_history_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_device_get_clock_history_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

void divoom_device_get_clock_list_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_device_get_clock_list_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

typedef struct _divoom_update_record_request_para
{
    int version;
    int type;    //0-升级失败，1-自动升级，2-app升级
    int download_size;
}divoom_update_record_request_para, *Pdivoom_update_record_request_para;

typedef struct _divoom_exchange_rate_pic_item
{
    int id;
    char flag[8];
    char image_url[56];
}divoom_exchange_rate_pic_item, *Pdivoom_exchange_rate_pic_item;

typedef struct _divoom_exchange_rate_pic_request_para
{
    int num;
    char flag[20][4];
}divoom_exchange_rate_pic_request_para, *Pdivoom_exchange_rate_pic_request_para;

typedef struct _divoom_exchange_rate_pic_respone_para
{
    int num;
    Pdivoom_exchange_rate_pic_item plist;
}divoom_exchange_rate_pic_respone_para, *Pdivoom_exchange_rate_pic_respone_para;

typedef struct _divoom_team_img_item
{
	int pic_id; //实况
	int pic_id2;//赛程
	int pic_id3; //排名
    char team_name[48];
    char image_url1[56];
    char image_url2[56];
    char image_url3[56];
}divoom_team_img_item, *Pdivoom_team_img_item;

typedef struct _divoom_team_img_respone_para
{
    int num;
	int team_type;
    Pdivoom_team_img_item plist;
}divoom_team_img_respone_para, *Pdivoom_team_img_respone_para;

typedef struct _divoom_bond_user_para
{
    int app_id;
	int lang;
    int gif_mode;
}divoom_bond_user_para, *Pdivoom_bond_user_para;


void divoom_device_get_exchange_rate_pic_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_device_get_exchange_rate_pic_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);


void divoom_device_upload_log_info_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_device_set_log_level_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_device_set_device_mode_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_device_play_buzzer_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_device_set_device_white_banlance_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_device_reset_app_mqtt_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_device_local_image_play_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_device_local_image_del_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_device_update_record_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_device_device_reset_all_config_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_device_get_custom_play_time_list_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_device_get_suscribe_play_time_list_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_device_request_refresh_token_play_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_device_host_like_gallery_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_device_get_album_list_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_device_get_album_list_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_device_play_tf_gif_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_device_screen_ctrl_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_device_update_wifi_status_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_device_screen_ctrl_respone_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_device_get_spotify_code_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_device_get_social_platform_code_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);



typedef struct _DivoomDispItemElement
{
    char imageAddr[64];
    int startXPos;
    int startYPos;
    int sizeX;
    int sizeY;
    int wordSpace;
    int alignMode;
    int dispType;
    int fontType;
    uint8 textColor[3];
    char dispData[128];

}DivoomDispItemElement, *PDivoomDispItemElement;

typedef struct _DivoomSystemConfigJson
{
    int RequestTime;
    int disp_num;
    int AppType;
    DivoomDispItemElement *DispList;
    char BackgroudImgAddr[64];
    int rule_num;
    Pdivoom_channel_item_disp_data_info RuleList;
    char *RequestUrlAddr;
    char *SimuRequestData;
    char *AppRuleName;
    char *AppTitle;
    char *AppConfig;
    int LCDId;
}DivoomSystemConfigJson, *PDivoomSystemConfigJson;

int divoom_device_PC_Clock_Config_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

typedef struct _divoom_device_get_clock_list_para
{
	int start_num;
	int end_num;
}divoom_device_get_clock_list_para, *Pdivoom_device_get_clock_list_para;

typedef struct _divoom_devcice_get_clock_list_respone
{
    int clock_num;
    int custom_num;
    int *pclock_list;
}divoom_devcice_get_clock_list_respone, *Pdivoom_devcice_get_clock_list_respone;

typedef struct _divoom_device_on_off_config_para
{
    uint8 work_status;  //0:未激活状态，1：激活状态； 2：取消状态
    int on_off;
    int start;    
    int end;
}divoom_device_on_off_config_para, *Pdivoom_device_on_off_config_para;

typedef struct _divoom_device_power_control_item
{
    int power_on_id;
    int on_off;    
    int time;
	int cyc_flag[7];
	int power_on_flag;
}divoom_device_power_control_item, *Pdivoom_device_power_control_item;

typedef struct _divoom_device_power_control_list
{
	int power_on_list_num;
	Pdivoom_device_power_control_item power_on_list;
}divoom_device_power_control_list, *Pdivoom_device_power_control_list;

typedef struct _divoom_device_danmaku_text_item
{
	char disp_text[128];
	int text_color;
	int user_id;
}divoom_device_danmaku_text_item, *Pdivoom_device_danmaku_text_item;

typedef struct _divoom_device_danmaku_config_item
{
	int disp_area_flag;
	int speed;
}divoom_device_danmaku_config_item, *Pdivoom_device_danmaku_config_item;

int divoom_device_danmaku_send_face_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_device_danmaku_send_text_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_device_danmaku_set_config_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_device_get_debug_flag_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_device_reset_all_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_device_power_control_set_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_device_power_control_get_list_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_device_power_control_set_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_device_set_on_off_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);


int divoom_device_set_on_off_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_device_test_self_reset_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);


typedef struct _divoom_device_pc_update_clock_item
{
    int num;
    int lcd_id;
    char para[30][128];
}divoom_device_pc_update_clock_item, *Pdivoom_device_pc_update_clock_item;

/* Device/UpdatePCParaInfo: DeviceId + ClockId + standard AppName/DispData JSON. */
typedef struct _divoom_device_pc_update_clock_para
{
    int device_id;
    int clock_id;
    char json_data[1];
}divoom_device_pc_update_clock_para, *Pdivoom_device_pc_update_clock_para;

int divoom_device_pc_update_clock_data_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_device_test_json_str_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

void *divoom_device_get_component_config_unpack_all_disp_item(divoom_cJSON *root, int *num);
void divoom_device_get_component_list_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_device_get_component_list_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_device_get_component_config_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_device_get_component_config_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_device_get_my_clock_list_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_device_get_my_clock_list_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_device_get_my_clock_id_list_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_device_get_my_clock_object_list_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_device_white_noise_json_get_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_device_white_noise_json_set_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

typedef struct _divoom_device_send_device_err_info
{
    int err_type;
    int  version;
    char file_id[64];
}divoom_device_send_device_err_info, *Pdivoom_device_send_device_err_info;

void divoom_device_save_device_error_record_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_device_send_device_err_log_get_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

typedef struct _divoom_device_send_device_format_info
{
    int err_type;
    int  version;
    char file_id[64];
}divoom_device_send_device_format_info, *Pdivoom_device_send_device_format_info;

void divoom_device_send_device_format_log_get_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);


typedef struct _divoom_device_get_weather_info
{
    char  Weather[64];
    float CurTemp;
    float MinTemp;
    float MaxTemp;
    int   Pressure;
    int   Humidity;
    int   Visibility;
    float WindSpeed;
}divoom_device_get_weather_info, *Pdivoom_device_get_weather_info;

void divoom_device_get_weather_info_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

typedef struct _divoom_device_get_update_file_list_request_info
{
    int update_time;
    int hardware_version;
}divoom_device_get_update_file_list_request_info, *Pdivoom_device_get_update_file_list_request_info;

typedef struct _divoom_device_get_update_file_list_respone_item
{
    int software_version;
    int update_time;
    int file_version;
    int file_flag;
    char sha_sum[64];
    char file_attr[256];
    char file_name[128];
    char file_id[64];
}divoom_device_get_update_file_list_respone_item, *Pdivoom_device_get_update_file_list_respone_item;

typedef struct _divoom_device_get_update_file_list_respone_info
{
    int num;
    Pdivoom_device_get_update_file_list_respone_item plist;
}divoom_device_get_update_file_list_respone_info, *Pdivoom_device_get_update_file_list_respone_info;

typedef struct _divoom_device_get_update_sys_file_respone_info
{
    int file_version;
    char file_name[128];    
    char sha1_info[64];
}divoom_device_get_update_sys_file_respone_info, *Pdivoom_device_get_update_sys_file_respone_info;

void divoom_device_get_update_file_list_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_device_get_update_file_list_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_device_set_sys_pixel_style_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_device_get_sys_pixel_style_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

int divoom_device_get_rgb_info_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_device_get_rgb_info_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_device_get_team_img_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_device_get_team_img_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_device_add_preload_album_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);


typedef struct _divoom_device_google_calendar_config_respone_info
{
    int pop_flag;
    int event_flag;
    char token[3072];
}divoom_device_google_calendar_config_respone_info, *Pdivoom_device_google_calendar_config_respone_info;

int divoom_device_get_google_calendar_config_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

typedef struct _divoom_device_get_url_info_respone_info
{
    char *url_addr;
    char *body;
    Pdivoom_http_client_url_head_info phead_info;
    int head_num;
    /* 仅歌词 track.search 代拉使用，不序列化；入队时复制为独立 JSON。 */
    const char *music_record_title;
    const char *music_record_artist;
    int music_record_duration;
    
}divoom_device_get_url_info_respone_info, *Pdivoom_device_get_url_info_respone_info;

void divoom_device_get_url_data_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_device_get_request_bond_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

int divoom_device_get_url_data_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);


typedef struct _divoom_custom_dial_ctrl_disp_item
{
    int id;
    int type;
    int x;
    int y;
    int w;
    int h;
    int align;
    int font_size;
    int font_id;
    int font_color;
    int bg_color;
    int request_time;
    int local_flag;
    char *image_addr;
    char *request_url;
    char *rule_info;
    char *text_msg;
}divoom_custom_dial_ctrl_disp_item, *Pdivoom_custom_dial_ctrl_disp_item;

typedef struct _divoom_custom_dial_ctrl_dial_item
{
    char *bg_img_addr;
    int local_flag;
    int disp_num;
    Pdivoom_custom_dial_ctrl_disp_item pdisp_list;
}divoom_custom_dial_ctrl_dial_item, *Pdivoom_custom_dial_ctrl_dial_item;

int divoom_device_enter_custom_dial_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

int divoom_device_exit_custom_dial_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_device_dele_custom_dial_all_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_device_dele_custom_dial_item_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

typedef struct _divoom_custom_dial_ctrl_disp_text_content
{
    int id;
    char *text_msg;
}divoom_custom_dial_ctrl_disp_text_content, *Pdivoom_custom_dial_ctrl_disp_text_content;

typedef struct _divoom_custom_dial_ctrl_dial_text_item
{
    int disp_num;
    Pdivoom_custom_dial_ctrl_disp_text_content pdisp_list;
}divoom_custom_dial_ctrl_dial_text_item, *Pdivoom_custom_dial_ctrl_dial_text_item;

int divoom_device_update_custom_text_disp_list_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

int divoom_device_change_custom_bg_image_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);


typedef struct _divoom_device_vision_item
{
    int id;
    uint8 del_flag;
    int update_time;
    char oss_name[256];
}divoom_device_vision_item, *Pdivoom_device_vision_item;

int divoom_device_vision_add_item_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

typedef struct _divoom_device_vision_update_info
{
    int number;
    Pdivoom_device_vision_item plist;
}divoom_device_vision_update_info, *Pdivoom_device_vision_update_info;

void divoom_device_vision_get_play_time_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_device_vision_set_play_time_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_device_get_vision_update_list_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_device_get_vision_update_list_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

int divoom_device_playlist_send_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);


typedef struct _divoom_device_playlist_image_list_item
{
    int playlist_id;
    int update_time;
    int file_size;
}divoom_device_playlist_image_list_item, *Pdivoom_device_playlist_image_list_item;

void divoom_device_get_playlist_image_list_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

int divoom_device_playlist_image_list_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);


typedef struct _divoom_device_cloud_photo_request_info
{
    int clock_id;
    int product_flag;
    int update_time;
}divoom_device_cloud_photo_request_info, *Pdivoom_device_cloud_photo_request_info;


typedef struct _divoom_device_cloud_photo_item
{
    int id;
    int del_flag;
    int clock_id;
    int oss_flag;
    int update_time;
    char oss_name[256];
}divoom_device_cloud_photo_item, *Pdivoom_device_cloud_photo_item;

typedef struct _divoom_device_cloud_photo_update_info
{
    int number;
    int clock_id;
    Pdivoom_device_cloud_photo_item plist;
}divoom_device_cloud_photo_list_info, *Pdivoom_device_cloud_photo_list_info;

void divoom_device_get_cloud_photo_list_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_device_get_cloud_photo_list_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_device_add_cloud_photo_item_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

typedef struct _divoom_device_cloud_photo_delete_info
{
    int number;
    int clock_id;
    int update_time;
    int *photo_list;
}divoom_device_cloud_photo_delete_info, *Pdivoom_device_cloud_photo_delete_info;

int divoom_device_del_cloud_photo_item_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

typedef struct _divoom_patch_local_holder
{
    char *raw_json;
} divoom_patch_local_holder, *Pdivoom_patch_local_holder;

void divoom_device_patch_local_clock_cmd_free(Pdivoom_net_command pcmd_info);
void divoom_device_get_local_clock_cmd_free(Pdivoom_net_command pcmd_info);
int divoom_device_get_local_clock_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_device_get_local_font_list_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_device_patch_local_clock_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

typedef struct _divoom_device_store_clock_get_list_request
{
	int flag;
	int classify_id;
	int start_num;
	int end_num;
} divoom_device_store_clock_get_list_request, *Pdivoom_device_store_clock_get_list_request;

/** StoreClockGetList ClockList[] 可选字段，与云端下发的 AIAGENT_DESC 一致（GetStoreClockMarketList 等会使用） */
#ifndef DIVOOM_DEVICE_STORE_CLOCK_AIAGENT_DESC_SZ
#define DIVOOM_DEVICE_STORE_CLOCK_AIAGENT_DESC_SZ 768
#endif

typedef struct _divoom_device_store_clock_list_item
{
	int clock_id;
	char clock_name[256];
	char image_pixel_id[256];
	int clock_type;
	int add_flag;
	char aiagent_desc[DIVOOM_DEVICE_STORE_CLOCK_AIAGENT_DESC_SZ];
} divoom_device_store_clock_list_item, *Pdivoom_device_store_clock_list_item;

typedef struct _divoom_device_store_clock_list
{
	int num;
	Pdivoom_device_store_clock_list_item plist;
} divoom_device_store_clock_list, *Pdivoom_device_store_clock_list;

void divoom_device_store_clock_get_list_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_device_store_clock_get_list_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

typedef struct _divoom_device_store_clock_classify_item
{
	int classify_id;
    char file_id[64];
	char classify_name[256];
} divoom_device_store_clock_classify_item, *Pdivoom_device_store_clock_classify_item;

typedef struct _divoom_device_store_clock_classify_list
{
	int num;
	Pdivoom_device_store_clock_classify_item plist;
} divoom_device_store_clock_classify_list, *Pdivoom_device_store_clock_classify_list;

void divoom_device_store_clock_get_classify_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_device_store_clock_get_classify_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

typedef struct _divoom_device_get_font_name_item
{
	int font_id;
	char font_name[256];
} divoom_device_get_font_name_item, *Pdivoom_device_get_font_name_item;

typedef struct _divoom_device_get_font_name_list
{
	int num;
	Pdivoom_device_get_font_name_item plist;
} divoom_device_get_font_name_list, *Pdivoom_device_get_font_name_list;

int divoom_device_get_font_name_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

int divoom_device_update_clock_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_device_update_clock_config_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

typedef struct _divoom_device_get_loacl_clock_list
{
	int num;
	int *plist;
} divoom_device_get_loacl_clock_list, *Pdivoom_device_get_loacl_clock_list;

int divoom_device_get_local_clock_list_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_device_remove_del_local_clock_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_device_remove_del_clock_file_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_device_remove_del_local_clock_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_device_not_support_language_request_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_device_get_no_ban_flag_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

#define DIVOOM_AI_TALK_MSG_BATCH_NUM 10
#define DIVOOM_AI_TALK_LANGUAGE_MAX_LEN 12
#define DIVOOM_AI_TALK_FILE_ID_MAX_LEN 128

typedef struct _divoom_device_ai_classfy_item
{
	int clock_id;
	int category_id;
	int count;
	uint32 revision;
	char language[DIVOOM_AI_TALK_LANGUAGE_MAX_LEN];
} divoom_device_ai_classfy_item, *Pdivoom_device_ai_classfy_item;

typedef struct _divoom_device_ai_classfy_list
{
	int num;
	Pdivoom_device_ai_classfy_item plist;
} divoom_device_ai_classfy_list, *Pdivoom_device_ai_classfy_list;

typedef struct _divoom_device_ai_talk_request_info
{
	int clock_id;
	int category_id;
	int msg_num;
	uint32 request_sequence;
	uint32 revision;
	char language[DIVOOM_AI_TALK_LANGUAGE_MAX_LEN];
	int msg_id[DIVOOM_AI_TALK_MSG_BATCH_NUM];
} divoom_device_ai_talk_request_info, *Pdivoom_device_ai_talk_request_info;

typedef struct _divoom_device_ai_talk_item
{
	int msg_id;
	char file_id[DIVOOM_AI_TALK_FILE_ID_MAX_LEN];
	char *content;
} divoom_device_ai_talk_item, *Pdivoom_device_ai_talk_item;

typedef struct _divoom_device_ai_talk_list
{
	int clock_id;
	int category_id;
	int msg_num;
	uint32 request_sequence;
	uint32 revision;
	char language[DIVOOM_AI_TALK_LANGUAGE_MAX_LEN];
	Pdivoom_device_ai_talk_item plist;
} divoom_device_ai_talk_list, *Pdivoom_device_ai_talk_list;

void divoom_device_get_ai_classfy_list_respone_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
void divoom_device_get_ai_talk_list_respone_pack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_device_get_ai_classfy_list_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_device_get_ai_talk_list_request_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

typedef struct _divoom_device_share_code_info
{
	uint32 share_code;
	uint32 expire_time;
} divoom_device_share_code_info, *Pdivoom_device_share_code_info;

int divoom_device_get_share_code_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);
int divoom_device_app_share_device_respone_unpack(divoom_cJSON *root, Pdivoom_net_command pcmd_info);

/* 国内电台协议只返回轻量值对象；不把cJSON节点交给目录/页面线程。 */
#define DIVOOM_DEVICE_RADIO_LIST_MAX 100
#define DIVOOM_DEVICE_RADIO_CLASSIFY_MAX 128
typedef struct
{
    int32 classify_id;
    char name[192];
} divoom_device_radio_classify;
typedef struct
{
    int32 count;
    divoom_device_radio_classify items[];
} divoom_device_radio_classify_list;
typedef struct
{
    uint32 radio_id;
    char name[192];
    char url[256];
} divoom_device_radio_item;
typedef struct
{
    int32 classify_id;
    int32 count;
    divoom_device_radio_item items[];
} divoom_device_radio_list;
/* Radio/GetRadioList请求数据为int32 ClassifyId；DeviceId由公共封包器添加。 */
void divoom_device_radio_list_request_pack(divoom_cJSON *root, Pdivoom_net_command command);
/* TestFlag 为 int32 0/1，DeviceId 沿用公共设备字段。 */
void divoom_device_save_test_flag_request_pack(divoom_cJSON *root, Pdivoom_net_command command);
int divoom_device_save_test_flag_response_unpack(divoom_cJSON *root, Pdivoom_net_command command);
int divoom_device_radio_classify_response_unpack(divoom_cJSON *root, Pdivoom_net_command command);
int divoom_device_radio_list_response_unpack(divoom_cJSON *root, Pdivoom_net_command command);

#endif
