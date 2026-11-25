#ifndef _DIVOOM_APP_FILE_MANAGE_H_
#define _DIVOOM_APP_FILE_MANAGE_H_

#define DIVOOM_APP_FILE_HOT						"/spiffs/divoom_hot.cfg"
#define DIVOOM_FILE_MUSIC_LIST 				"/spiffs/music_list"
#define DIVOOM_FILE_KEY_INFO 					"/spiffs/sd_file_checksum"
#define DIVOOM_PATH_FILE_MUSIC_LIST 	"/spiffs/path_music_list"

#define DIVOOM_MUSIC_NUMS				"music_file_nums"
#define DIVOOM_MUSIC_CHECKSUM	"music_checksum"

enum
{
	DIVOOM_APP_DOWN_FILE_PROG,              //升级文件
	DIVOOM_APP_DOWN_FILE_SCREEN_SAVE,       //屏保文件
	DIVOOM_APP_DOWN_FILE_HOT_MOVIE,        //hot文件
	DIVOOM_APP_DOWN_FILE_USER_MOVIE,			//用户自定义图片
	DIVOOM_APP_DOWN_FILE_ALARM_VOICE,        //闹钟录音
	DIVOOM_APP_DOWN_FILE_LEAVE_VOICE,        //闹钟录音
	DIVOOM_APP_DOWN_FILE_NULL,
	DIVOOM_APP_SD_CARD_FAT,   
	DIVOOM_APP_SD_CARD_PULL_OUT,
	DIVOOM_APP_SD_CARD_INSERT,
	DIVOOM_APP_SD_CARD_UPDATE_FILE_LIST,
};

void divoom_app_file_manage_init(void);


//----------------------------------------------------------------


typedef enum{
    DIVOOM_APP_FILE_TYPE_CUSTOM = 0,        //客户订制的相关图片数据
    DIVOOM_APP_FILE_TYPE_GIF = 1,           //APP操作的动画
    DIVOOM_APP_FILE_TYPE_ALARM = 2,         //闹钟配置的动画
    DIVOOM_APP_FILE_TYPE_DIALY = 3,         //纪念日配置的动画
    DIVOOM_APP_FILE_TYPE_POWER_ON = 4,      //开机图案
    DIVOOM_APP_FILE_TYPE_MUSIC = 5,         //EQ设置的动画
    DIVOOM_APP_FILE_TYPE_USER_DEFINE = 6,   //自定义像框
    DIVOOM_APP_FILE_TYPE_HOT = 7,           //ＤＩＶＯＯＭ推荐动画
    DIVOOM_APP_FILE_TYPE_ANCS = 8,          //智能提醒图案
    DIVOOM_APP_FILE_TYPE_TIME_MANAGE = 9,   //时间计划
    DIVOOM_APP_FILE_TYPE_FONT_TEMP = 10,    //动画播放的临时字体信息
    DIVOOM_APP_FILE_TYPE_FONT_GIF = 11,     //实时动画的更新的字体数据
    DIVOOM_APP_FILE_TYPE_PHONE_PIC = 12,     //实时画板图片数据
}DIVOOM_APP_FILE_TYPE;


#define divoom_app_file_manage_close_read(a)  ;
void *divoom_app_file_manage_open_read(uint8 type, uint32 file_index, int8 dir);
uint32 divoom_app_file_manage_seek_off(void *handle, uint32 pos);
uint32 divoom_app_file_manage_get_file_size(void *handle);
uint32 divoom_app_file_manage_read_data(void *handle, uint32 read_len, uint8 *buf);
uint32 divoom_app_file_manage_get_next_file_index(uint8 type, uint32 file_index);
uint8 divoom_app_file_manage_check_file_exist(uint8 type, uint32 file_index);



#endif
