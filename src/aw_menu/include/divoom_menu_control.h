#ifndef _divoom_menu_control_h_
#define _divoom_menu_control_h_
#include <setjmp.h>
#include "lvgl.h"
#include "divoom_lvgl_message_box.h"
#include "divoom_load_message.h"
#include "divoom_lvgl_normal_menu_list.h"
#include "divoom_lvgl_movie_play.h"
#include "divoom_menu_direct_ctrl.h"
#include "divoom_user_key.h"

#ifndef DIVOOM_KEY_ASTROTOO_KEY_UP
#define DIVOOM_KEY_ASTROTOO_KEY_UP DIVOOM_KEY_PREV
#endif
#ifndef DIVOOM_KEY_ASTROTOO_KEY_DOWN
#define DIVOOM_KEY_ASTROTOO_KEY_DOWN DIVOOM_KEY_NEXT
#endif
#ifndef DIVOOM_KEY_ASTROTOO_KEY_LEFT
#define DIVOOM_KEY_ASTROTOO_KEY_LEFT DIVOOM_KEY_MUSIC_PREV
#endif
#ifndef DIVOOM_KEY_ASTROTOO_KEY_RIGHT
#define DIVOOM_KEY_ASTROTOO_KEY_RIGHT DIVOOM_KEY_MUSIC_NEXT
#endif
#ifndef DIVOOM_KEY_ASTROTOO_KEY_ENTER
#define DIVOOM_KEY_ASTROTOO_KEY_ENTER DIVOOM_KEY_PLAY
#endif
#ifndef DIVOOM_KEY_ASTROTOO_KEY_ESC
#define DIVOOM_KEY_ASTROTOO_KEY_ESC DIVOOM_KEY_ESC
#endif

#define DIVOOM_LVGL_KEY_UP    DIVOOM_KEY_ASTROTOO_KEY_UP
#define DIVOOM_LVGL_KEY_DOWN  DIVOOM_KEY_ASTROTOO_KEY_DOWN
#define DIVOOM_LVGL_KEY_LEFT  DIVOOM_KEY_ASTROTOO_KEY_LEFT
#define DIVOOM_LVGL_KEY_RIGHT DIVOOM_KEY_ASTROTOO_KEY_RIGHT
#define DIVOOM_LVGL_KEY_NEXT DIVOOM_KEY_NEXT
#define DIVOOM_LVGL_KEY_PRE DIVOOM_KEY_PREV
#define DIVOOM_LVGL_KEY_ESC DIVOOM_KEY_ASTROTOO_KEY_ESC
#define DIVOOM_LVGL_KEY_ENTER DIVOOM_KEY_ASTROTOO_KEY_ENTER
#define DIVOOM_LVGL_KEY_HOME DIVOOM_KEY_MENU
#define DIVOOM_LVGL_KEY_LIGHT DIVOOM_KEY_MODE
#define DIVOOM_LVGL_KEY_FAVORITE	 DIVOOM_KEY_MUSIC_FAVORITE

#define lv_event_get_target_obj lv_event_get_target
#if LVGL_VERSION_MAJOR >= 9
#if defined(_WIN32) || (defined(DIVOOM_LVGL9_IS_SIM) && DIVOOM_LVGL9_IS_SIM)
#define lv_obj_add_event_cb lv_obj_add_event
#endif
#else
#define lv_obj_add_event lv_obj_add_event_cb
#endif

enum{
    DIVOOM_LVGL_MENU_LANG_ENGLISH,  //英语
    DIVOOM_LVGL_MENU_LANG_SC,       //简体中文
    DIVOOM_LVGL_MENU_LANG_TC,       //繁体
    DIVOOM_LVGL_MENU_LANG_JP,       //日语
    DIVOOM_LVGL_MENU_LANG_Germany,  //德国
    DIVOOM_LVGL_MENU_LANG_Thai,     //泰文
    DIVOOM_LVGL_MENU_LANG_Russian,  //俄语
    DIVOOM_LVGL_MENU_LANG_Korean,   //韩国人
    DIVOOM_LVGL_MENU_LANG_Vietnam,  //越南
    DIVOOM_LVGL_MENU_LANG_French,   //法语
    DIVOOM_LVGL_MENU_LANG_Turkey,       //土耳其
    DIVOOM_LVGL_MENU_LANG_Spanish,  //西班牙
    DIVOOM_LVGL_MENU_LANG_Portuguese,   //葡萄牙的
    DIVOOM_LVGL_MENU_LANG_Indonesian,       //印尼语
    DIVOOM_LVGL_MENU_LANG_Italy,    //意大利
    DIVOOM_LVGL_MENU_LANG_Ukraine,  //乌克兰
    DIVOOM_LVGL_MENU_LANG_Swedish,    //瑞典语
    DIVOOM_LVGL_MENU_LANG_Danish,       //丹麦语 
    DIVOOM_LVGL_MENU_LANG_Poland,       //波兰 
    DIVOOM_LVGL_MENU_LANG_END,       //END
};

typedef struct _divoom_select_language_menu_item
{
    int lang_id;
    char *lang_info;
}divoom_select_language_menu_item, *Pdivoom_select_language_menu_item;

#define DIVOOM_SCREEN_WIDTH  480
#define DIVOOM_SCREEN_HIGH   480

#define DIVOOM_MENU_ANIM_TIME  300
#define DIVOOM_MENU_ANIM_MODE  LV_SCR_LOAD_ANIM_MOVE_LEFT
#define DIVOOM_MENU_LABEL_EDGE_WIDTH (DIVOOM_SCREEN_WIDTH - 32)

#define DIVOOM_MENU_LABEL_EDGE_HIGH 56
#define DIVOOM_MENU_LABEL_BTN_HIGH 56
#define DIVOOM_MENU_LABEL_DISP_HIGH 48
#define DIVOOM_MENU_INCLUDE_BAR_HIGH (DIVOOM_SCREEN_HIGH - 64)
#define DIVOOM_MENU_LABEL_ARROW_DISP_WIDTH (DIVOOM_SCREEN_WIDTH - 32)
#define DIVOOM_MENU_LABEL_ICON_DISP_WIDTH  48
#define DIVOOM_MENU_LABEL_NORMAL_DISP_WIDTH 120
#define DIVOOM_MENU_LABEL_LEFT_PAD_DOT 3
#define DIVOOM_MENU_SLIDER_HIGHT 4

#define DIVOOM_MENNU_TEXT_COLOR  0XFFFFFF
#define DIVOOM_MENNU_BACK_GROUD_COLOR  0X000000
#define DIVOOM_MENNU_ACTIVE_BG_COLOR  0XBBBBBB
#define DIVOOM_MENNU_NORMAL_BG_COLOR  0X000000
#define DIVOOM_MENNU_FOCUS_BG_COLOR  0X555863       //聚集背景颜色
#define DIVOOM_MENNU_FOCUS_BORDER_COLOR  0X000000   //聚集边框颜色
#define DIVOOM_MENNU_SLIDER_UNACTIVE_COLOR  0x4F4F55       //滑动条无效的颜色
#define DIVOOM_MENNU_SLIDER_ACTIVE_COLOR  0X969696   //滑动条有效的颜色
#define DIVOOM_MENNU_TEXT_UNACTIVE_COLOR  0Xdcdcdc       //滑动条无效的颜色
#define DIVOOM_MENNU_TEXT_ACTIVE_COLOR  0XFFFFFF   //滑动条有效的颜色

enum{
    DIVOOM_MENU_CONTROL_SREEN_SLIDE_LEFT = 1,    //左
    DIVOOM_MENU_CONTROL_SREEN_SLIDE_RIGHT = 2,    //右
    DIVOOM_MENU_CONTROL_SREEN_SLIDE_UP = 3,    //上
    DIVOOM_MENU_CONTROL_SREEN_SLIDE_DOWN = 4,    //下
    DIVOOM_MENU_CONTROL_SREEN_SLIDE_EMPTY = 5,    //没有滑动
};

enum{
    DIVOOM_MENU_CONTROL_ID_MAIN = 1,    //系统主界面
    DIVOOM_MENU_CONTROL_ID_PHOTO_MANAGE = 2,    //照片管理
    DIVOOM_MENU_CONTROL_ID_ALBUM_STREAM = 3,    //相册照片流效果
    DIVOOM_MENU_CONTROL_ID_ALBUM_MANAGE = 4,    //图片取消或添加到相册
    DIVOOM_MENU_CONTROL_ID_PHOTO_VIEW = 5,    //查看照片
    DIVOOM_MENU_CONTROL_ID_IMAGE_TEST = 6,    //图像测试
    DIVOOM_MENU_CONTROL_ID_ALBUM_FADE_OUT = 7,    //相册渐隐流效果
    DIVOOM_MENU_CONTROL_ID_ALBUM_PUSH = 8,    //相册推送效果
    DIVOOM_MENU_CONTROL_ID_ALBUM_MAGAZINE = 9,    //相册杂志效果
    DIVOOM_MENU_CONTROL_ID_ALBUM_ORIGAMI = 10,    //相册折纸效果
    DIVOOM_MENU_CONTROL_ID_ALBUM_ROTATE = 11,    //相册旋转效果
    DIVOOM_MENU_CONTROL_ID_ALBUM_MAGAZINE_FADE_OUT = 12,    //相册杂志渐隐效果
    DIVOOM_MENU_CONTROL_ID_ALBUM_MAGAZINE_PUSH = 13,    //相册杂志推送效果
    DIVOOM_MENU_CONTROL_ID_PHOTO_SLIDE_EFFECT = 14,    //相册幻灯片效果
    DIVOOM_MENU_CONTROL_ID_SELECT_ALBUM_PLAY = 15,    //选择相簿
    DIVOOM_MENU_CONTROL_ID_HIDE_PHOTO = 16,    //显示隐藏照片
    DIVOOM_MENU_CONTROL_ID_DELETE_PHOTO = 17,    //删除照片
    DIVOOM_MENU_CONTROL_ID_ADD_PHOTO = 18,    //增加照片
	DIVOOM_MENU_CONTROL_ID_COMPONENT = 19, //组件显示
    DIVOOM_MENU_CONTROL_ID_PHOTO_SETTING = 20,    //照片管理
	DIVOOM_MENU_CONTROL_ID_DIAL_PREVIEW = 21, //表盘预览
    DIVOOM_MENU_CONTROL_ID_MORE_FUNCTION = 22,    //系统功能更多页面
    DIVOOM_MENU_CONTROL_ID_ALARM_LIST_MENU = 23,    //闹钟页面
    DIVOOM_MENU_CONTROL_ID_LYRIC_MENU = 24,    //氛围歌词
    DIVOOM_MENU_CONTROL_ID_WHITE_NOISE_MENU = 25,    //白噪音
    DIVOOM_MENU_CONTROL_ID_SYSTEM_SETING_MENU = 26,    //系统设置 
    DIVOOM_MENU_CONTROL_ID_POMODORO_MENU = 27,    //番茄时间
    DIVOOM_MENU_CONTROL_ID_STOP_WATCH_MENU = 28,    //计时器
    DIVOOM_MENU_CONTROL_ID_COUNT_DOWN_MENU = 29,    //倒计时
    DIVOOM_MENU_CONTROL_ID_SCORE_BOARD_MENU = 30,    //计分板
    DIVOOM_MENU_CONTROL_ID_NOTICE_MESSAGE_MENU = 31,    //留言
    DIVOOM_MENU_CONTROL_ID_NOISE_MEASURE_MENU = 32,    //噪音检测
    DIVOOM_MENU_CONTROL_ID_BLUE_MUSIC_MENU = 33,    //蓝牙音乐
    DIVOOM_MENU_CONTROL_ID_LOCAL_MUSIC_MENU = 34,    //本地音乐
    DIVOOM_MENU_CONTROL_ID_ADD_PICTURE_TO_ALBUM = 35,    //增加相片到相簿
	DIVOOM_MENU_CONTROL_ID_TOMA_EDIT = 36,	//番茄编辑页
	DIVOOM_MENU_CONTROL_ID_TOMA_EDIT_TIME = 37, //番茄编辑时间
	DIVOOM_MENU_CONTROL_ID_TOMA_EDIT_REPEAT = 38, //番茄编辑重复
	DIVOOM_MENU_CONTROL_ID_TOMA_EDIT_BELL = 39, //番茄编辑铃声
	DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_ABOUT = 40, //设置里面的关于页面
	DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_DATE_TIME = 41, //设置里面的时间与日期页面
	DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_TEMPERATURE = 42, //设置里面的温度页面
	DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_LANGUGE = 43, //设置里面的语言页面
	DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_WIFI = 44, //设置里面的WIFI页面
	DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_NIGHTVIEW = 45, //设置里面的夜览页面
	DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_SCREEN_SAVE = 46, //设置里面的屏保页面
	DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_IDLE_POWER_OFF_TIME = 47, //设置里面的空闲关机页面
	DIVOOM_MENU_CONTROL_ID_ALARM_ADD_MENU = 48, //闹钟添加页面
	DIVOOM_MENU_CONTROL_ID_ALARM_EDIT_MENU = 49, //闹钟编辑页面
	DIVOOM_MENU_CONTROL_ID_WHITENOISE = 50, //白噪音页面
	DIVOOM_MENU_CONTROL_ID_WHITE_NOISE_EDIT_TIME = 51, //白噪音时间编辑页面
	DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_RESTORE = 52, //设置里面的还原页面
	DIVOOM_MENU_CONTROL_ID_CONFIG_WIFI = 53, //配网页面
	DIVOOM_MENU_CONTROL_ID_UPDATE_DEVICE_FIRMWARE = 54, //升级页面
	DIVOOM_MENU_CONTROL_ID_COMPONENT2 = 55, //组件显示
	DIVOOM_MENU_CONTROL_ID_COMPONENT3 = 56, //组件显示
	DIVOOM_MENU_CONTROL_ID_COMPONENT4 = 57, //组件显示
	DIVOOM_MENU_CONTROL_ID_COMPONENT5 = 58, //组件显示
	DIVOOM_MENU_CONTROL_ID_COMPONENT6 = 59, //组件显示
	DIVOOM_MENU_CONTROL_ID_COMPONENT7 = 60, //组件显示
	DIVOOM_MENU_CONTROL_ID_COMPONENT8 = 61, //组件显示
	DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_POWER_OFF_TIME = 62, //设置定时开关机页面
	DIVOOM_MENU_CONTROL_ID_ALARM_ACTIVE_MENU = 63, //闹钟激活页面
	DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_BT_SET_SECRET = 64,      //设置蓝牙里面的设置蓝牙密码页面
	DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_BLUETOOTH = 65, //设置里面的蓝牙页面
	DIVOOM_MENU_CONTROL_ID_COUNT_DOWN_START_MENU = 66,    //倒计时启动页面
	DIVOOM_MENU_CONTROL_ID_SYSTEM_POWER_ON = 67, //开机动画
    DIVOOM_MENU_CONTROL_ID_SLIDE_SETING_MENU = 68,    //幻灯片设置 
    DIVOOM_MENU_CONTROL_SCHEDULE_DIAL_ID = 69,    //定时表盘 暂时不用，使用DIVOOM_MENU_CONTROL_SYSTEM_DIAL_ID
    DIVOOM_MENU_CONTROL_TIME_PLAN_DIAL_ID = 70,    //时间计划表盘 暂时不用，使用DIVOOM_MENU_CONTROL_SYSTEM_DIAL_ID
    DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_THEME = 71,   //设置里面的主题外观页面
    DIVOOM_MENU_CONTROL_ID_TOOL = 72,   		//工具列表页面
    DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_WIFI_PROPERTIES = 73,   //设置里面的网络属性页面
    DIVOOM_MENU_CONTROL_ID_ACCEPT_APP_PHOTO_VIEW = 74,    //收到APP发送的照片进行查看照片
    DIVOOM_MENU_CONTROL_ID_TOP_AMBIENT_MENU = 75,	//顶层灯效页面
    DIVOOM_MENU_CONTROL_ID_TOP_WIFI_NOTICE_MENU = 76,	//顶层wifi断开提醒
    DIVOOM_MENU_CONTROL_ID_PRODUCT_TEST_MENU = 77, 	//产品测试页面
    DIVOOM_MENU_CONTROL_ID_PRODUCT_STRESS_TESTING_MENU = 78, 	//产品压力测试页面
    DIVOOM_MENU_CONTROL_ID_RESETING_DEVICE_MENU = 79, 	//恢复出厂设置等待页面
	DIVOOM_MENU_CONTROL_ID_DIAL_PREVIEW2 = 80, //表盘预览2
	DIVOOM_MENU_CONTROL_ID_POWER_OFF_DISP_MENU = 81, //定时开关机开始页面
	DIVOOM_MENU_CONTROL_ID_TOP_REACHED_HOME_MENU = 82, //顶层到达顶层提示
	DIVOOM_MENU_CONTROL_ID_GOOGLE_CALENDAR_MENU = 83, //GOOGLE日历菜单
	DIVOOM_MENU_CONTROL_ID_GOOGLE_CALENDAR_ACTIVE_MENU = 84, //GOOGLE日历激活菜单
	DIVOOM_MENU_CONTROL_ID_TOP_DANMAKU_MENU = 85, //顶层弹幕页面
	DIVOOM_MENU_CONTROL_ID_OUTLOOK_CALENDAR_MENU = 86, //OUTLOOK_日历菜单
	DIVOOM_MENU_CONTROL_ID_OUTLOOK_CALENDAR_ACTIVE_MENU = 87, //OUTLOOK_日历激活菜单
	DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_COLOR_TEMP = 88, //设置里面的色温调节
	DIVOOM_MENU_CONTROL_ID_DEMO_DIAL_MENU = 89, //表盘展示页面
	DIVOOM_MENU_CONTROL_ID_CALENDAR_LIST_MENU = 90, //日历列表页面
	DIVOOM_MENU_CONTROL_ID_AUDIO_SOURCE_MENU = 91, //音源页面
	DIVOOM_MENU_CONTROL_ID_LOCAL_MUSIC_FOLDER_MENU = 92, //TF卡文件夹页面
	DIVOOM_MENU_CONTROL_ID_WHITE_NOISE_LIST = 93, //白噪音列表页面(九宫格)
	DIVOOM_MENU_CONTROL_ID_WHITE_NOISE_PLAY_MAIN = 94, //白噪音播放主页面(壁炉背景+默认播放)
	DIVOOM_MENU_CONTROL_ID_AUX_MUSIC_MENU = 95, //AUX播放页面
	DIVOOM_MENU_CONTROL_ID_UAC_MUSIC_MENU = 96, //UAC播放页面
	DIVOOM_MENU_CONTROL_ID_TOP_AUDIO_MENU = 97, //顶层音源页面
	DIVOOM_MENU_CONTROL_ID_TOP_BATTERY_MENU = 98, //顶层电池电量页面
	DIVOOM_MENU_CONTROL_ID_TOP_AUDIO_TIP_MENU = 99, //顶层音源提示页面
#ifdef DIVOOM_TIME_FRAME_SPEAKER
	DIVOOM_MENU_CONTROL_ID_ALARM_CLOCK_WAKE_HOLD = 100, //闹钟开机黑屏待按键后再进表盘
#endif
	DIVOOM_MENU_CONTROL_ID_LYRIC_BG_LIST_MENU = 101, //氛围歌词背景列表页面
	DIVOOM_MENU_CONTROL_ID_STORE_CLOCK_MARKET = 102, /* 表盘商店分类摘要列表 divoom_lvgl_store_clock_market_menu */
	DIVOOM_MENU_CONTROL_ID_STORE_CLOCK_MARKET_CATEGORY = 103, /* 某分类下级页 divoom_lvgl_store_clock_category_menu */
	DIVOOM_MENU_CONTROL_ID_DIAL_PREVIEW_FAVORITES = 104, /* 收藏表盘预览（列表数据源为 divoom_clock_favorite） */
	DIVOOM_MENU_CONTROL_ID_VOICE_RECORD_TOOL_MENU = 105, /* 工具：录音入口 */
	DIVOOM_MENU_CONTROL_ID_SLEEP_AID = 106, /* 睡眠辅助 */
	DIVOOM_MENU_CONTROL_ID_SLEEP_AID_ACTIVE = 107, /* 助眠音源播放（收藏切换） */
    DIVOOM_MENU_CONTROL_ID_SYSTEM_SETTING_MCP_TOOL = 108, //PC表盘创作&MCP
	DIVOOM_MENU_CONTROL_ID_JL_OTA_UPDATE = 109, /* JL 蓝牙芯片 OTA 升级进度页 */

    /* AstroToo 480 UI 导航扩展 */
    DIVOOM_MENU_CONTROL_ID_ASTROTOO_SETTINGS_DISPLAY = 110,
    DIVOOM_MENU_CONTROL_ID_ASTROTOO_SETTINGS_MEDIA = 111,
    DIVOOM_MENU_CONTROL_ID_ASTROTOO_SETTINGS_CONNECTION = 112,
    DIVOOM_MENU_CONTROL_ID_ASTROTOO_SETTINGS_PREFERENCES = 113,
    DIVOOM_MENU_CONTROL_ID_ASTROTOO_SETTINGS_SYSTEM = 114,
    DIVOOM_MENU_CONTROL_ID_ASTROTOO_SETTINGS_BRIGHTNESS = 115,
    DIVOOM_MENU_CONTROL_ID_ASTROTOO_SETTINGS_VOLUME = 116,
    DIVOOM_MENU_CONTROL_ID_ASTROTOO_SETTINGS_EQUALIZER = 117,
    DIVOOM_MENU_CONTROL_ID_ASTROTOO_SETTINGS_WIND_UNIT = 118,
    DIVOOM_MENU_CONTROL_ID_ASTROTOO_SETTINGS_DATE_FORMAT = 119,
    DIVOOM_MENU_CONTROL_ID_ASTROTOO_GAMES = 120,
    DIVOOM_MENU_CONTROL_ID_ASTROTOO_PLACEHOLDER = 121,
    DIVOOM_MENU_CONTROL_ID_ASTROTOO_SETTINGS_DATE_TIME_TIME = 122,
    DIVOOM_MENU_CONTROL_ID_ASTROTOO_SETTINGS_DATE_TIME_DATE = 123,
    DIVOOM_MENU_CONTROL_ID_ASTROTOO_OOBE_WELCOME = 124,
    DIVOOM_MENU_CONTROL_ID_ASTROTOO_OOBE_CONFIG_WIFI = 125,
    DIVOOM_MENU_CONTROL_ID_ASTROTOO_SETTINGS_WIFI_ADD = 126,
    DIVOOM_MENU_CONTROL_ID_ASTROTOO_SETTINGS_SLIDESHOW_ORDER = 127,

    DIVOOM_MENU_CONTROL_SYSTEM_DIAL_ID = 0x80,    //系统表盘
    DIVOOM_MENU_CONTROL_ID_ASTROTOO_SETTINGS_WIFI_PASSWORD = 129,
    DIVOOM_MENU_CONTROL_ID_ASTROTOO_SETTINGS_KEY_LIGHT_EFFECTS = 130, /* 偏好 · 按键灯效 */
    DIVOOM_MENU_CONTROL_ID_ASTROTOO_SETTINGS_CONNECTION_CODE = 131, /* 连接与状态 · 设备连接码 */
    DIVOOM_MENU_CONTROL_ID_MIXER = 132, /* App 控制的 13×13 炫音工厂 */
	DIVOOM_MENU_CONTROL_ID_MIX_MUSIC2 = 133, /* Mixer/SetNew 七种 480×480 乐器页 */
	DIVOOM_MENU_CONTROL_ID_POWER_SHUTDOWN = 134, /* 真关机专用黑屏，不读不写文件 */
	DIVOOM_MENU_CONTROL_ID_FILE_TRANSFER = 135, /* /upload 与 HTTP GIF 下载进度 */
	/* 顶栏留言条字体账本：不进菜单栈，避免 clearall 拆 MAIN 时把条上的 24 号字销毁 */
	DIVOOM_MENU_CONTROL_ID_TOP_LEAVE_MSG_NOTICE = 136,
	/* MODE 音源切换 OSD 字体账本：不进菜单栈，避免切表盘 clear 128 时把 toast 24 号字销毁 */
	DIVOOM_MENU_CONTROL_ID_TOP_SOURCE_SWITCH = 137,
	DIVOOM_MENU_CONTROL_ID_ASTROTOO_SETTINGS_TWS = 138,
};
enum{
    DIVOOM_LVGL_PHOTO_MANAGE_VIEW_PICTURE,   //由幻灯片进入照片浏览模式，下面有加号，增加相片，右上脚有设置按钮
    DIVOOM_LVGL_PHOTO_MANAGE_ADD_PICTURE,   //增加照片，这个位置是所有照片
    DIVOOM_LVGL_PHOTO_MANAGE_HIDE_PICTURE,   //显示与隐藏界面，
    DIVOOM_LVGL_PHOTO_MANAGE_DEL_PICTURE,   //删除与移除界面，
    DIVOOM_LVGL_PHOTO_MANAGE_CLOCK_VIEW_PICTURE,   //这个是从表盘过来，要浏览照片
};
    
typedef struct _divoom_lvgl_photo_manage_para
{
    int operate_type;   //操作类型
    int pic_id;
    int    clock_pos;
    int    parent_id;
    uint32 album_id;    //相册ID
}divoom_lvgl_photo_manage_para, *Pdivoom_lvgl_photo_manage_para;

typedef void DIVOOM_LVGL_PHOTO_VIEW_DEL_NOTICE(void *para,int pos);
typedef struct _divoom_lvgl_photo_view_para_info
{
    int cur_pos;    //当前照片在pic_list中的位置,如果没有选中照片，将是-1
    int pic_id;     //照片ID
    int clock_pos;
    int parent_id;
    int album_id;   //相簿ID
    int *pic_num;    //总共有多少张照片
    void *pget_pic_handle;  //照片的列表
    void *para;     //DIVOOM_LVGL_PHOTO_VIEW_DEL_NOTICE的参数
    DIVOOM_LVGL_PHOTO_VIEW_DEL_NOTICE *pdel_callback;
}divoom_lvgl_photo_view_para_info, *Pdivoom_lvgl_photo_view_para_info;


typedef void *DIVOOM_MENU_CONTROL_MENU_INIT(void *user_data, int menu_id);
typedef void DIVOOM_MENU_CONTROL_MENU_LVGL_DISP(void *obj, void *active_obj);
typedef void *DIVOOM_MENU_CONTROL_MENU_LVGL_GET_ACTIVE_OBJ(void *obj);
typedef void DIVOOM_MENU_CONTROL_MENU_LVGL_PAUSE(void *obj);
typedef void DIVOOM_MENU_CONTROL_MENU_LVGL_CLEAN(void *obj);
typedef void DIVOOM_MENU_CONTROL_MENU_LVGL_UPDATE_DISP(void *obj);
typedef void *DIVOOM_MENU_CONTROL_MENU_LVGL_GET_MAIN_OBJ(void *obj);
typedef void DIVOOM_MENU_CONTROL_MENU_LVGL_PLAY_NEXT(void *obj, int dir);
typedef void DIVOOM_MENU_CONTROL_MENU_LVGL_UPDATE_PLAY_TIME(void *obj, int play_time);
typedef void DIVOOM_MENU_CONTROL_MENU_LVGL_GET_PIC_INFO(void *obj, Pdivoom_lvgl_photo_view_para_info pic_info);

typedef struct _divoom_menu_control_item_info
{
    int key_flag;
    int push_flag;
    int id;
    DIVOOM_MENU_CONTROL_MENU_INIT *pinit;
    DIVOOM_MENU_CONTROL_MENU_LVGL_CLEAN *pclean;
    DIVOOM_MENU_CONTROL_MENU_LVGL_DISP *pdisp;
    DIVOOM_MENU_CONTROL_MENU_LVGL_PAUSE *pause;
}divoom_menu_control_item_info, *Pdivoom_menu_control_item_info;

typedef struct _divoom_menu_control_menu_bar
{
    int work_mode;
    int bat_level;
    int hour;
    int min;
    void *time_obj;
    void *blue_obj;
    void *bat_obj;
    lv_style_t menu_bar_style;  //菜单工具栏
    struct _lv_timer_t * menu_bar_time;
}divoom_menu_control_menu_bar, *Pdivoom_menu_control_menu_bar;
/*********************************************************************
  Function:        divoom_menu_control_get_disp_str
  Description:     通过关键字符串，要处理多语言得到显示字串
  Input:           src
                   SRC
  Return:          NONE
**********************************************************************/

char *divoom_menu_control_get_disp_str(const char * src);

/*********************************************************************
  Function:        divoom_menu_control_lv_component_img_set_src
  Description:     组件icon图片显示
  Input:           obj
                   SRC
  Return:          NONE
**********************************************************************/

void divoom_menu_control_lv_component_img_set_src(lv_obj_t * obj, int model_id, int image_id);


/*********************************************************************
  Function:        divoom_menu_control_lv_img_set_src
  Description:     只传相对而言路径，转到LVGL
  Input:           obj
                   SRC
  Return:          NONE
**********************************************************************/

void divoom_menu_control_lv_img_set_src(lv_obj_t * obj, const char * src);

void divoom_menu_control_slider_clear_border_outline_shadow(lv_obj_t *obj);

/*********************************************************************
  Function:        divoom_menu_control_get_img_path2
  Description:     从相对而言路径得到LVGL使用的图像路径
  Input:           src
                   path
  Return:          NONE
**********************************************************************/

void divoom_menu_control_get_img_path2(const char * src, char *path);


/*********************************************************************
  Function:        divoom_menu_control_get_img_path
  Description:     从相对而言路径得到LVGL使用的图像路径
  Input:           src
                   path
  Return:          NONE
**********************************************************************/

void divoom_menu_control_get_img_path(const char * src, char *path);

/*********************************************************************
  Function:        divoom_menu_control_init
  Description:     菜单模块初始化
  Input:           none
  Return:          NONE
**********************************************************************/

void divoom_menu_control_init(void);


/*********************************************************************
  Function:        divoom_menu_control_clearall_load
  Description:     将取消所有菜单队列，只加这个菜单
  Input:           menu_id
  Return:          NONE
**********************************************************************/

void divoom_menu_control_clearall_load(int menu_id, void *user_data, int model_id, int event_id);

/*********************************************************************
  Function:        divoom_menu_control_load_next_with_disp
  Description:     带显示ID的加载MENU方式
  Input:           menu_id
  Return:          NONE
**********************************************************************/

void divoom_menu_control_load_next_with_disp(int menu_id, void *user_data, int model_id, int event_id, int flag);

/*********************************************************************
  Function:        divoom_menu_control_load_next
  Description:     加载菜单，会将当前菜单压入队列
  Input:           menu_id
  Return:          NONE
**********************************************************************/

void divoom_menu_control_load_next(int menu_id, void *user_data, int model_id, int event_id);

/*********************************************************************
  Function:        divoom_menu_control_menu_items
  Description:     当前菜单有多少级
  Input:
  Return:          菜单级数
**********************************************************************/

int divoom_menu_control_menu_items(void);

/*********************************************************************
  Function:        divoom_menu_control_menu_get_first_menu_id
  Description:     获取初始菜单页面ID
  Input:
  Return:          菜单级数
**********************************************************************/

int divoom_menu_control_menu_get_first_menu_id(void);

/*********************************************************************
  Function:        divoom_menu_control_load_next_direct
  Description:     马上加载菜单，会将当前菜单压入队列
  Input:           menu_id
  Return:          NONE
**********************************************************************/

void divoom_menu_control_load_next_direct(int menu_id, void* user_data, int pop_flag, int model_id, int event_id);

#if defined(DIVOOM_SIMULATOR) && DIVOOM_SIMULATOR
void divoom_menu_control_poll(void);
#endif

/*********************************************************************
  Function:        divoom_menu_control_set_language
  Description:     修改语言
  Input:           lang
  Return:          NONE
**********************************************************************/

void divoom_menu_control_set_language(int lang);

/*********************************************************************
  Function:        divoom_menu_control_switch_language
  Description:     异步切换语言并强制重建根菜单到目标菜单的导航链
  Input:           lang            新语言 ID
                   root_menu_id    重建后的根菜单 ID
                   target_menu_id  重建后的当前菜单 ID
  Return:          1:事件已入队，0:参数无效或事件申请失败
**********************************************************************/

int divoom_menu_control_switch_language(int lang, int root_menu_id, int target_menu_id);

/*********************************************************************
  Function:        divoom_menu_control_load_next_pop_cur
  Description:     加载菜单，会将当前菜单弹出队列
  Input:           menu_id
                   user_data  这个外面要申请，在菜单初始化时再由菜单级释放
  Return:          NONE
**********************************************************************/

void divoom_menu_control_load_next_pop_cur(int menu_id, void *user_data, int model_id, int event_id);


/*********************************************************************
  Function:        divoom_menu_control_clear_all_load_inner
  Description:     马上将所有的菜单全部弹出，加载新的菜单
  Input:           none
  Return:          NONE
**********************************************************************/

void divoom_menu_control_clear_all_load_direct(int menu_id, void* user_data, int model_id, int event_id);

/*********************************************************************
  Function:        divoom_menu_control_return_pre
  Description:     退出当前菜单，返回上一级
  Input:           none
  Return:          NONE
**********************************************************************/

void divoom_menu_control_return_pre(void);

/*********************************************************************
  Function:        divoom_menu_control_redisp_current
  Description:     不换菜单，重新 disp 当前页（短按开屏后把当前表盘刷出来）
  Input:           none
  Return:          NONE
**********************************************************************/

void divoom_menu_control_redisp_current(void);

/*********************************************************************
  Function:        divoom_menu_control_change_slide_effect
  Description:     切换幻灯片效果
  Input:           none
  Return:          NONE
**********************************************************************/

void divoom_menu_control_change_slide_effect(void);

/*********************************************************************
  Function:        divoom_menu_control_return_main
  Description:     返回第一级主菜单
  Input:           none
  Return:          NONE
**********************************************************************/

void divoom_menu_control_return_main(void);

/*********************************************************************
  Function:        divoom_menu_control_exit
  Description:     退出菜单，清除所有资源
  Input:           none
  Return:          NONE
**********************************************************************/

void divoom_menu_control_exit(void);

/*********************************************************************
  Function:        divoom_menu_control_destory_menu_bar
  Description:     退出菜单工具栏
  Input:           phandle
  Return:          NONE
**********************************************************************/

void divoom_menu_control_destory_menu_bar(Pdivoom_menu_control_menu_bar phandle);

/*********************************************************************
  Function:        divoom_menu_control_create_menu_bar
  Description:     创建默认的菜单工具栏
  Input:           phandle
                   main_obj 父窗口句柄
  Return:          NONE
**********************************************************************/

void divoom_menu_control_create_menu_bar(Pdivoom_menu_control_menu_bar phandle, void *main_obj);

/*********************************************************************
  Function:        divoom_menu_control_disp_label_text
  Description:     静态文本的单行与多行的显示适配
  Input:           label_obj
                   disp_str         显示的字符串
                   max_width        显示位置的最大宽度
                   max_high         显示位置的最大高度
                   font_high        显示字体的高度
                   left_pad_x       左边预留的位置
  Return:          NONE
**********************************************************************/

void divoom_menu_control_disp_label_text(void *label_obj, char *disp_str, int max_width,
                                            int max_high, int font_high, int left_pad_x, int align_mode, int menu_id);

/*********************************************************************
  Function:        divoom_menu_control_get_actice_animation_menu_handle
  Description:     获取激活菜单的句柄
  Input:           menu_id
  Return:          NONE
**********************************************************************/

void *divoom_menu_control_get_actice_animation_menu_handle();

void *divoom_menu_control_get_clock_mode_menu_handle();


/*********************************************************************
  Function:        divoom_menu_control_lvgl_ctrl
  Description:     这个函数主要是用于调用LVGL的一些内部函数，避免多任务冲突,
                   只能在非LVGL的菜单中使用，否则将会出现死锁情况
                   非常重要


  Input:           pcallback
                   user_data
  Return:          NONE
**********************************************************************/

typedef void DIVOOM_MENU_CONTROL_LVGL_CTRL_CALLBACK(void *user_data);
void divoom_menu_control_lvgl_ctrl(DIVOOM_MENU_CONTROL_LVGL_CTRL_CALLBACK pcallback, void *user_data);


/*********************************************************************
  Function:        divoom_menu_control_change_font_lang
  Description:     切换语言调用切换字体结构体
  Input:           lang
  Return:          NONE
**********************************************************************/

void divoom_menu_control_change_font_lang(int lang);

/*********************************************************************
  Function:        divoom_menu_control_destroy_system_font
  Description:     释放字体
  Input:           lang 0 菜单字体 1 歌词字体
  Return:          NONE
**********************************************************************/

void divoom_menu_control_destroy_system_font(int lang);

/*********************************************************************
  Function:        divoom_menu_control_load_system_font
  Description:     lang 0 加载菜单字体 1 加载歌词字体
  Input:           lang
  Return:          NONE
**********************************************************************/

void divoom_menu_control_load_system_font(int lang);

/*********************************************************************
  Function:        divoom_menu_control_get_font_info
  Description:     返回对应字号的字体
  Input:           font_size 0xFF表示取歌词字体 否则取菜单字体
  Return:          NONE
**********************************************************************/

void *divoom_menu_control_get_font_info(int font_size, int menu_id);

/*********************************************************************
  Function:        divoom_menu_control_get_font_info2
  Description:     返回对应字号的字体
  Input:           
  Return:          NONE
**********************************************************************/

void *divoom_menu_control_get_font_info2(int font_id, int font_size, int menu_id);
/* 固定字号的普通/粗体分别缓存；失败时按同字号系统字体保底。 */
void *divoom_menu_control_get_font_info_style(int font_id, int font_size,
    uint16 font_style, int menu_id);

/*********************************************************************
  Function:        divoom_menu_control_key_press_notice
  Description:     有按键来，清除空闲时间
  Input:           none
  Return:          NONE
**********************************************************************/

void divoom_menu_control_key_press_notice(void);

/*********************************************************************
  Function:        divoom_menu_control_init_btn_stype
  Description:     初始化菜单样式
  Input:           none
  Return:          NONE
**********************************************************************/

void divoom_menu_control_init_btn_stype(lv_style_t *active, const char *active_img, lv_style_t *unactive, const char *unactive_img, lv_style_t *press, const char *press_img, int radio_value);

/*********************************************************************
  Function:        divoom_menu_control_wait_load_system_font
  Description:     等待歌词字体加载
  Input:           none
  Return:          NONE
**********************************************************************/

void divoom_menu_control_wait_load_system_font(void);


/*********************************************************************
  Function:        divoom_menu_control_check_menu_font
  Description:     检查歌词字体是否加载
  Input:           none
  Return:          1 or 0
**********************************************************************/

int divoom_menu_control_check_menu_font(void);
/*********************************************************************
  Function:        divoom_menu_control_get_active_menu_id
  Description:     获取激活菜单的ID
  Input:           
  Return:          菜单的ID
**********************************************************************/
int divoom_menu_control_get_active_menu_id();

int divoom_menu_control_is_initialized(void);

/*********************************************************************
  Function:        divoom_menu_control_get_stack_depth
  Description:     菜单栈深度（menu_queue 项数，不含当前页）
  Input:           none
  Return:          栈深度
**********************************************************************/
int divoom_menu_control_get_stack_depth(void);
/*********************************************************************
  Function:        divoom_menu_control_get_del_queue_count
  Description:     待销毁菜单队列长度
  Input:           none
  Return:          del_queue 项数
**********************************************************************/
int divoom_menu_control_get_del_queue_count(void);
/*********************************************************************
  Function:        divoom_menu_control_is_enter_config_wifi
  Description:     是否进入配网状态
  Input:           
  Return:          1 or 0
**********************************************************************/

int divoom_menu_control_is_enter_config_wifi(void);

/*********************************************************************
  Function:        divoom_menu_control_get_slide_stype
  Description:     判断用户的手势
  Input:           x1
                   y1
                   x2
                   y2
  Return:          NONE
**********************************************************************/

int divoom_menu_control_get_slide_stype(int x1, int y1, int x2, int y2);

/*********************************************************************
  Function:        divoom_menu_control_get_gaussian_buf
  Description:     得到高斯模糊的图像
  Input:           in_buf
                   in_w
                   in_h
                   out_buf
                   out_w
                   out_h
  Return:          NONE
**********************************************************************/

void divoom_menu_control_get_gaussian_buf(uint8 *in_buf, int in_w, int in_h, uint8 *out_buf, int out_w, int out_h, int value, uint8 *exit_flag);
/*********************************************************************
  Function:        divoom_menu_control_get_black_buf
  Description:     得到某种颜色的底图
  Input:           in_buf
                   in_w
                   in_h
                   color_info
  Return:          NONE
**********************************************************************/

void divoom_menu_control_get_black_buf(uint8 *in_buf, int in_w, int in_h, lv_color_t color_info);

/*********************************************************************
  Function:        divoom_menu_control_init_lable_style
  Description:     创建LABLE的样式
  Input:           label_style
  Return:          NONE
**********************************************************************/

void divoom_menu_control_init_lable_style(lv_style_t *label_style);

/*********************************************************************
  Function:        divoom_menu_control_remove_menu
  Description:     弹出某个菜单
  Input:           menu_id
  Return:          NONE
**********************************************************************/

void divoom_menu_control_remove_menu(int menu_id);

/*********************************************************************
  Function:        divoom_menu_control_init_btn_stype
  Description:     初始化菜单样式
  Input:           normal  未选中状态
                   select_press   选中按了确定键状态
                   press   选中还未按确定键状态
  Return:          NONE
**********************************************************************/

void divoom_menu_control_init_btn_select_stype(lv_style_t *normal, lv_style_t *select_press, lv_style_t *press);

/*********************************************************************
  Function:        divoom_get_lvgl_lock
  Description:     对LVGL进行加锁，可以在外面程序操作LVGL函数
  Input:           NONE
  Return:          NONE
**********************************************************************/

void divoom_get_lvgl_lock(void);

/*********************************************************************
  Function:        divoom_get_lvgl_try_lock
  Description:     对LVGL进行加锁，加锁成功返回0，
  Input:           NONE
  Return:          加锁成功返回0，否则返回其它
**********************************************************************/

int divoom_get_lvgl_try_lock(void);

/*********************************************************************
  Function:        divoom_free_lvgl_lock
  Description:     对LVGL进行解锁
  Input:           NONE
  Return:          NONE
**********************************************************************/

void divoom_free_lvgl_lock(void);

/*********************************************************************
  Function:        divoom_menu_control_change_disp_mode
  Description:     修改显示模式
  Input:           video_flag  1：显示视频； 0：标准显示
  Return:          NONE
**********************************************************************/

void divoom_menu_control_change_disp_mode(void *main_obj, int video_flag);

/* 持 LVGL 锁，新根屏加载后调用：释放本次切页快照，避免与视频解码缓冲重叠。 */
void divoom_menu_control_release_switch_snapshot(void);

int divoom_menu_control_set_video_ui_overlay_argb1555(void *phy_addr,
    int x, int y, int width, int height);
/* 与 1555 同样占用每像素 2 字节，保留 16 级透明度供歌词边缘混合。 */
int divoom_menu_control_set_video_ui_overlay_argb4444(void *phy_addr,
    int x, int y, int width, int height);
int divoom_menu_control_disable_video_ui_overlay(void);

enum {
    DIVOOM_VIDEO_TOAST_BATTERY,
    DIVOOM_VIDEO_TOAST_AUDIO,
    DIVOOM_VIDEO_TOAST_COUNT
};
/* LVGL thread only; pass NULL before deleting/changing the toast object. */
void divoom_menu_control_update_video_toast(int slot, void *obj);

/*********************************************************************
  Function:        divoom_menu_control_init_bgra
  Description:     将缓冲区清除成某一种单一元素
  Input:           dest_rgb  
                   r
                   g
                   b
                   src_w
                   src_h
  Return:          NONE
**********************************************************************/

void divoom_menu_control_init_bgra(uint8 *dest_rgb, int r, int g, int b, int src_w, int src_h);

#endif
