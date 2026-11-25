#ifndef _divoom_lvgl_dial_menu2_h_
#define _divoom_lvgl_dial_menu2_h_

typedef void divoom_lvgl_dial_menu_set_obj_level_callback(void *user_data, void *lvgl_obj, int level);
typedef void divoom_lvgl_dial_menu_managed_image_pause_callback(void *user_data);

enum{
    DIVOOM_LVGL_DIAL_MENU_TYPE_LOCAL_IMAGE,     //本地文件，像WEBP文件
    DIVOOM_LVGL_DIAL_MENU_TYPE_DIVOOM_IMAGE,    //DIVOOM社区文件
    DIVOOM_LVGL_DIAL_MENU_TYPE_RGB_IMAGE,       //是外部的RGB缓冲区
    DIVOOM_LVGL_DIAL_MENU_TYPE_DIVOOM_HANDLE,   //DIVOOM社区图像句柄
    DIVOOM_LVGL_DIAL_MENU_TYPE_RGBA_QUEUE_IMAGE, //这个主要用户示本地文件部分图像，解码后放在队列中
    DIVOOM_LVGL_DIAL_MENU_TYPE_TEXT,            //文件信息
    DIVOOM_LVGL_DIAL_MENU_TYPE_DIVOOM_QUEUE,    //divoom社区，循环播放相关图片
    DIVOOM_LVGL_DIAL_MENU_TYPE_DIVOOM_EQ_DIAL,    //divoomEQ表盘，循环播放相关图片
    DIVOOM_LVGL_DIAL_MENU_TYPE_PHOTO_SLIDE,     //相册幻灯片
    DIVOOM_LVGL_DIAL_MENU_TYPE_CLEAR_ALL,       //清除所有缓冲区
    DIVOOM_LVGL_DIAL_MENU_TYPE_FONT_IMAGE,     //图像字体显示
    DIVOOM_LVGL_DIAL_MENU_TYPE_ROTATE_IMAGE,     //旋转图像显示
    DIVOOM_LVGL_DIAL_MENU_TYPE_PLAY_VIDEO,     //视频显示
    DIVOOM_LVGL_DIAL_MENU_TYPE_BUDDY,          //Buddy 表情控件
    DIVOOM_LVGL_DIAL_MENU_TYPE_BACKGROUD,      //AI 背景程序化效果控件
    DIVOOM_LVGL_DIAL_MENU_TYPE_LOCAL_PART_IMAGE, //本地图像局部帧流式解码显示
    DIVOOM_LVGL_DIAL_MENU_TYPE_MUSIC_IMAGE,     //显示歌词/歌名相关信息
    DIVOOM_LVGL_DIAL_MENU_TYPE_MUSIC_MULTILINE, //2212 多行透明歌词控件
    DIVOOM_LVGL_DIAL_MENU_TYPE_ANIMAL_PARADE,   //2228 PNG 流式解码动物巡游控件
    DIVOOM_LVGL_DIAL_MENU_TYPE_WINDOW_WHISPER,  //2746 运行时图形窗影轻语控件
    DIVOOM_LVGL_DIAL_MENU_TYPE_LYRIC_EQ,        //透明 64 频点 EQ 元素，底图由表盘管理
};

enum
{
    DIVOOM_LVGL_DIAL_MENU_BACKGROUD_EFFECT_NONE, // 未登记效果
    DIVOOM_LVGL_DIAL_MENU_BACKGROUD_EFFECT_CELL_STATE2, // 数字生命细胞迭代
    DIVOOM_LVGL_DIAL_MENU_BACKGROUD_EFFECT_SEED_DIFFUSION, // 沃罗诺伊世界
    DIVOOM_LVGL_DIAL_MENU_BACKGROUD_EFFECT_STAR_RAIN_HORIZON, // 地平线效果
    DIVOOM_LVGL_DIAL_MENU_BACKGROUD_EFFECT_ASTRO_KINETIC_SCULPTURE, // Astro Kinetic Sculpture
    DIVOOM_LVGL_DIAL_MENU_BACKGROUD_EFFECT_RAIN_TRACE, // Rain Trace 表盘背景
    DIVOOM_LVGL_DIAL_MENU_BACKGROUD_EFFECT_SILENT_SNOW, // 柔焦雪点
    DIVOOM_LVGL_DIAL_MENU_BACKGROUD_EFFECT_GOLDEN_HOUR, // 黄金时刻
    DIVOOM_LVGL_DIAL_MENU_BACKGROUD_EFFECT_HORIZON_INDICATOR, // 人工地平仪
    DIVOOM_LVGL_DIAL_MENU_BACKGROUD_EFFECT_AFTER_RAIN, // 雨后初晴
    DIVOOM_LVGL_DIAL_MENU_BACKGROUD_EFFECT_METEOR_STAR, // 夜空
    DIVOOM_LVGL_DIAL_MENU_BACKGROUD_EFFECT_LIGHT_PORTAL, // 时光之门
    DIVOOM_LVGL_DIAL_MENU_BACKGROUD_EFFECT_STAR_FIELD, // 星仪图
    DIVOOM_LVGL_DIAL_MENU_BACKGROUD_EFFECT_LIVING_LYRICS, // 生活歌词表盘
    DIVOOM_LVGL_DIAL_MENU_BACKGROUD_EFFECT_RETRO_TV, // 复古电视机
    DIVOOM_LVGL_DIAL_MENU_BACKGROUD_EFFECT_SEA_HORIZON, // 海平线
    DIVOOM_LVGL_DIAL_MENU_BACKGROUD_EFFECT_ORBITAL_SYSTEM, // 抽象轨道
    DIVOOM_LVGL_DIAL_MENU_BACKGROUD_EFFECT_PENDULUM, // 大摆锤
    DIVOOM_LVGL_DIAL_MENU_BACKGROUD_EFFECT_SAND_WAVES, // 流沙
    DIVOOM_LVGL_DIAL_MENU_BACKGROUD_EFFECT_TWILIGHT, // 随本地时间变化的暮光色场
};

#if defined(_WIN32)
/* 仿真验证入口：使用产品 ID 分派工厂；对象由父对象删除，调用方持有 LVGL 锁。 */
void *divoom_lvgl_dial_menu_debug_create_backgroud(int clock_id, void *parent);
/* second>=0 注入本对象演示秒数，-1 恢复 RTC；可返回三段 RGB888 控制色，成功返回1。 */
int divoom_lvgl_dial_menu_debug_twilight_time(void *obj, int second, uint32 *colors);
#endif

typedef struct _divoom_lvgl_dial_ctr_info
{
	int clock_id;
	int disp_mode; //0 显示模式 1 编辑模式
}divoom_lvgl_dial_ctr_info, *Pdivoom_lvgl_dial_ctr_info;

/*********************************************************************
  Function:        DIVOOM_LVGL_DIAL_MENU_SET_SELECT_CALLBACK
  Description:     组件选择后的回调，通知变更
  Input:           data     
                   item_value   选项的值
                   item_name      选项的名称
  Return:          NONE
**********************************************************************/

typedef void (*DIVOOM_LVGL_DIAL_MENU_SET_SELECT_CALLBACK)(void *data, char *item_value, char  *item_name);
//组件相关信息
typedef struct _divoom_lvgl_dial_menu_set_select_info
{
    int x;
    int y;
    int w;
    int h;
	int radius;
    int item_type;
    uint32 color_value;
	uint32 border_color;
    char item_id[64]; 
	char component_name[10];
    void *lvgl_obj;
    uint8 *image_buf;
    DIVOOM_LVGL_DIAL_MENU_SET_SELECT_CALLBACK pcallback;
}divoom_lvgl_dial_menu_set_select_info, *Pdivoom_lvgl_dial_menu_set_select_info;


typedef int (*DIVOOM_LVGL_DIAL_MENU_FINISH_CALLBACK)(void *data);
/*********************************************************************
  Function:        divoom_lvgl_dial_menu_init
  Description:     初始化模块
  Input:           none
  Return:          NONE
**********************************************************************/

void *divoom_lvgl_dial_menu_init(void *user_data, int menu_id);
/* 供歌词等表盘元素挂到当前表盘根对象，生命周期仍由 dial_menu 管理。 */
void *divoom_lvgl_dial_menu_get_root_obj(void *handle);

/** 歌词表盘：挂载到外部 main_obj，仅提供 disp_clock 所需的 text/image 队列，不初始化 divoom_disp_base_clock */
void divoom_lvgl_dial_menu_init_lyric_shell(void *main_obj, int menu_id, int clock_id);
void divoom_lvgl_dial_menu_exit_lyric_shell(void);

void divoom_lvgl_dial_menu_disp(void *handle, void *active);

typedef struct _divoom_lvgl_dial_menu_stock_chart_info
{
	int x;
	int y;
	int w;
	int h;
	int flag;
	int stock_price_dot; //股票价格点数
	float max_price;
	float min_price;
	float stock_price[DIVOOM_CLOCK_CONTROL_HISOTYR_DAYS];
	int time_stamp[DIVOOM_CLOCK_CONTROL_HISOTYR_DAYS];
	int parent_clock_id;
	uint8_t style;
	uint32_t line_color;
	uint32_t fill_color;
}divoom_lvgl_dial_menu_stock_chart_info, *Pdivoom_lvgl_dial_menu_stock_chart_info;

typedef struct _divoom_lvgl_dial_menu_component_info
{
	int x;
	int y;
	int w;
	int h;
	int radius; //弧度
	int opa; //透明度
	uint32 bg_color; 
}divoom_lvgl_dial_menu_component_info, *Pdivoom_lvgl_dial_menu_component_info;

#define DIVOOM_DIAL_TEXT_WRAP_CENTER 4
#define DIVOOM_DIAL_TEXT_SINGLE_LINE_SCROLL 5

typedef struct _divoom_lvgl_dial_menu_text_item
{
    int x;
    int y;
    int w;
    int h;
 	int sep; 		//字符间隔
	int effect;     //变化显示效果
    int align;      //对齐方式, 
    int mode;       //过长的显示方式
	int long_mode;	//针对比较长的文本 0不滚动 1滚动 2自动换行 3RSS自适应滚动(disp104/农历宜忌24/25)
    int font_id;    //字体ID
    int font_size;  //字体大小，如果为0，将自适应
    uint32 font_color;  //字体颜色 
    uint32 bg_color;    //字体背景颜色
    int hier; //图像层次
    int angle;      //图片顺时针旋转角度
    int transp; //透明度
	int animation; //动画效果
	int disp_type; //显示元素类型
	uint8 ai_text_has_title; //仅指定 AI 语料分类允许按“标题\n正文”解析
    char *disp_info;    //显示字串
}divoom_lvgl_dial_menu_text_item, *Pdivoom_lvgl_dial_menu_text_item;

typedef struct
{
    int x;
    int y;
    int w;
    int h;
    int hier;
    const char *mode_id;
    const char *cfg_dir;
    const char *action_clip_id;
    uint32 action_sequence;
} divoom_lvgl_dial_menu_buddy_item, *Pdivoom_lvgl_dial_menu_buddy_item;

typedef struct _divoom_lvgl_dial_menu_progress_item
{
    int x;
    int y;
    int w;
    int h;
    int hier;
    uint32 front_color;
    uint32 back_color;
}divoom_lvgl_dial_menu_progress_item, *Pdivoom_lvgl_dial_menu_progress_item;

/* 创建或更新由表盘菜单统一管理的进度条。 */
void *divoom_lvgl_dial_menu_add_progress_handle(void *handle,
    Pdivoom_lvgl_dial_menu_progress_item pprogress_info, int percent, int visible,
    void *obj, void *user_data, divoom_lvgl_dial_menu_set_obj_level_callback *pcallback);


/*********************************************************************
  Function:        divoom_lvgl_dial_menu_add_stock_chart
  Description:     增加股票曲线
  Input:           handle   第一次为空，以后给第一次使用返回的句柄
                   pimage_info    图像信息，第一次有位置信息以后只关心字串变化，不修改位置
  Return:          handle
**********************************************************************/

void *divoom_lvgl_dial_menu_add_stock_chart(void *handle, Pdivoom_lvgl_dial_menu_stock_chart_info pstock_info, void *obj, divoom_lvgl_dial_menu_set_obj_level_callback *pcallback);


/*********************************************************************
  Function:        divoom_lvgl_dial_menu_add_text_handle
  Description:     添加一个文本显示元素
  Input:           text_handle   第一次为空，以后给第一次使用返回的句柄
                   ptext_info    文本信息，第一次有位置信息以后只关心字串变化，不修改位置
  Return:          handle
**********************************************************************/

void *divoom_lvgl_dial_menu_add_text_handle(void *text_handle, Pdivoom_lvgl_dial_menu_text_item ptext_info, void *obj, Pdivoom_clock_id_info pclock_info, void *user_data, divoom_lvgl_dial_menu_set_obj_level_callback *pcallback);

/* AI 话术文本由 dial menu 负责逐字显示、光标动画及完整生命周期。 */
void *divoom_lvgl_dial_menu_add_ai_text_handle(void *text_handle, Pdivoom_lvgl_dial_menu_text_item ptext_info, void *obj, Pdivoom_clock_id_info pclock_info, void *user_data, divoom_lvgl_dial_menu_set_obj_level_callback *pcallback);

/* 诗词正文按实际文本尺寸整体居中，超出显示区域时才启用往返滚动。 */
void *divoom_lvgl_dial_menu_add_poetry_text_handle(void *text_handle,
	Pdivoom_lvgl_dial_menu_text_item ptext_info, void *obj,
	Pdivoom_clock_id_info pclock_info, void *user_data,
	divoom_lvgl_dial_menu_set_obj_level_callback *pcallback);

/* AI 大/小框文本已完成控件创建并装入待播放文本时返回 1。 */
int divoom_lvgl_dial_menu_ai_text_handle_ready(void *text_handle);

/*********************************************************************
  Function:        divoom_lvgl_dial_menu_clear_text_handle
  Description:     清空文本并隐藏（用于表盘空字段）
  Input:           text_handle
  Return:          none
**********************************************************************/

void divoom_lvgl_dial_menu_clear_text_handle(void *text_handle);

/*********************************************************************
  Function:        divoom_lvgl_dial_menu_show_text_handle
  Description:     显示已隐藏的文本句柄
  Input:           text_handle
  Return:          none
**********************************************************************/

void divoom_lvgl_dial_menu_show_text_handle(void *text_handle);

/*********************************************************************
  Function:        divoom_lvgl_dial_menu_add_move_text_handle
  Description:     添加一个文本动态移动显示元素
  Input:           text_handle   第一次为空，以后给第一次使用返回的句柄
                   ptext_info    文本信息，修改字串，动态修改位置
  Return:          handle
**********************************************************************/
void *divoom_lvgl_dial_menu_add_move_text_handle(void *text_handle, Pdivoom_lvgl_dial_menu_text_item ptext_info, void *obj, Pdivoom_clock_id_info pclock_info, void *user_data, divoom_lvgl_dial_menu_set_obj_level_callback *pcallback);


typedef struct _divoom_lvgl_dial_menu_image_item
{
    int x;
    int y;
    int w;  //图像宽度
    int h;  //图像高度
    int hier; //图像层次
    int rotate_flag;//图像旋转方向
    int rotate_time;    //图像旋转的时间，多长时间旋转完一圈
    int antialias_flag;       //lv_img_set_antialias，图像是否要抗锯齿
    int angle;      //图片顺时针旋转角度
    int transp; //透明度
	int circle_image; //针对头像做圆形显示
	int mul_flag; //多张图片
    char *image_name;   //图片名称
    uint8 *rgb_buf;     //当名称为空时使用这个图像缓冲区，这个为RGB缓冲
    void  *pdivoom_image_handle;    //DIVOOM社区图像句柄
    /* 新增字段放末尾，避免改变 image_name 等既有字段偏移 */
    uint8 front_color[3];   //表盘 color_1，网络图库着色
    uint8 back_color[3];    //表盘 color_2，网络图库抠图色
    int color_tint_flag;    //是否对网络图库应用 front/back 颜色
    lv_color_format_t color_format; //0 自动：480x480 用 RGB565，其它默认 RGB565A8
    uint32 font_color;  //歌词/歌名字体颜色
    int font_size;      //歌词/歌名艺术字号，0 时按显示高度估算
    int font_id;        //歌词/歌名字体 ID，0 时用系统字体
    int align;          //歌词/歌名对齐，DIVOOM_WORD_EFFECT_ALIGN_*
    int sys_font_size;  //歌词/歌名系统字号，0 时跟 font_size
}divoom_lvgl_dial_menu_image_item, *Pdivoom_lvgl_dial_menu_image_item;

typedef struct _divoom_lvgl_dial_menu_calendar_watch_info
{
	int x;
	int y;
	T_U8 w;
	T_U8 h;
	T_U8 unicode_str[14];
	T_U8 color[3];
	T_U8 date;
	T_U8 today_flag;
	T_U8 cur_month_flag; //1 当月的 0 非当月的
}divoom_lvgl_dial_menu_calendar_watch_info, *Pdivoom_lvgl_dial_menu_calendar_watch_info;

typedef struct _divoom_lvgl_image_info
{
	int flag; // 1正在使用 0没有使用
	lv_img_dsc_t img_dsc;
}divoom_lvgl_image_info, *Pdivoom_lvgl_image_info;

typedef struct _divoom_lvgl_dial_menu_image_ctrl
{
    int   read_image_time;
    uint8 *point_src_buf;
    uint8 *divoom_rgb_buf;
    int divoom_rgb_buf_size;
    int cur_disp_id;    //只有DIVOOM社区图片才使用这个
	int resource_type;	   //DIVOOM_RESOURCE_TYPE_XXXX
    int image_type;     //DIVOOM_LVGL_DIAL_MENU_TYPE_XXXX
    int apha_flag;      //DIVOOM社区图是否增加透明度
    int area_x;         //显示区域x
    int area_y;         //显示区域y
    int area_w;         //显示区域宽度
    int area_h;         //显示区域高度
    int antialias_flag; //是否使用抗锯齿算法
    int angle_value;    //旋转角度
    int rotate_flag;    //旋转的方向
    int image_strech;   //图像是否要拉伸处理
    int mul_image;    //旋转角度
    int frame_num;
    int frame_time;
    int image_width;
    int image_high;
	int play_time; //显示次数
	int pwebp_handle_free_flag; //pwebp_handle是否需要释放
    char *image_name;
    void *pwebp_handle;          //webp的解码句柄或者DIVOOM社区图像句柄,如果是单张的，使用完了就释放，如果是动画将在最后释放
    void *disp_time_handle; //如果是动画，这个将是动画控制显示定时器
	void *lvgl_top_obj;
	void *lvgl_btn_obj;
    void *lvgl_image_obj;   
    void *lvgl_label_obj;     
    lv_anim_t image_anim;    //旋转动画
    int play_anim_num;    //播放旋转动画次数
    int pre_disp_tick;    //播放旋转动画次数
	Pdivoom_lvgl_image_info lvgl_image_info;
	lv_img_dsc_t *lvgl_image_buf;
    DIVOOM_GET_EQ_DISP_BUF_CALLBACK pread_disp_buf_callback;
    void *user_data;
	void *obj;
    DIVOOM_LVGL_DIAL_MENU_FINISH_CALLBACK pfinish_callback;
    /* 新增字段放末尾，避免改变 area_x/area_w/lvgl_image_buf 等既有字段偏移 */
    uint8 front_color[3];
    uint8 back_color[3];
    int color_tint_flag;
    int local_part_cnt; //每段播放帧数
    int local_part_list_num; //局部段数量
    int local_part_cur_list; //当前局部段索引
    int local_part_cur_offset; //当前段内已播放帧偏移
    int local_part_decode_frame; //解码器当前帧位置
    int local_part_resource_frame_num; //源文件总帧数
    int *local_part_start; //每个局部段起始帧，显示时按需跳帧
    int image_decode_ok;
    int music_font_size;
    int music_font_id;
    int music_align;
    int music_hier;
    uint32 music_font_color;
    divoom_lvgl_dial_menu_set_obj_level_callback *pmusic_level_callback;
    const lv_font_t *music_sys_font;
    const lv_font_t *music_art_font;
    const lv_font_t *music_sys_history_font;
    const lv_font_t *music_art_history_font;
    int music_sys_font_size;
    void *managed_user_data; //通用图像队列托管控件的私有状态
    divoom_lvgl_dial_menu_managed_image_pause_callback *pmanaged_pause_callback;
    int rss_scroll_mode;
    int rss_line_height;
    int rss_content_size;
    uint8 rss_scroll_active;
}divoom_lvgl_dial_menu_image_ctrl, *Pdivoom_lvgl_dial_menu_image_ctrl;



void *divoom_lvgl_dial_menu_get_component_obj(Pdivoom_lvgl_dial_menu_component_info pcomponent_info);
void *divoom_lvgl_dial_menu_create_component_handle(void *phandle, Pdivoom_lvgl_dial_menu_component_info pcomponet_info, void *obj);
void divoom_lvgl_dial_menu_set_component_hidden(void *phandle, int hidden);
void *divoom_lvgl_dial_menu_add_componet_text_handle(void *text_handle, void *pcomponet_obj, Pdivoom_lvgl_dial_menu_text_item ptext_info, void *obj, Pdivoom_clock_id_info pclock_info, void *user_data, divoom_lvgl_dial_menu_set_obj_level_callback *pcallback);
void *divoom_lvgl_dial_menu_add_component_png_handle(void *pdisp_handle, void *pcomponet_obj, Pdivoom_lvgl_dial_menu_image_item pimage_info, int model_id, int image_id, void *obj, void *user_data, divoom_lvgl_dial_menu_set_obj_level_callback *pcallback);
void *divoom_lvgl_dial_menu_draw_calendar_watch(void *handle, void *text_handle, void *pdate_info, void *text_attr, void *obj, Pdivoom_clock_id_info pclock_info, int offset_y, divoom_lvgl_dial_menu_set_obj_level_callback *pcallback);
void *divoom_lvgl_dial_menu_draw_calendar_watch_with_week(void *handle, void *text_handle, void *pdate_info, void *text_attr, void *obj, Pdivoom_clock_id_info pclock_info, int week_high, int week_widht, int today_x, int today_y, int week_type, divoom_lvgl_dial_menu_set_obj_level_callback *pcallback);
void *divoom_lvgl_dial_menu_draw_calendar_watch2(void *handle, void *text_handle, void *pdate_info, void *text_attr, void *obj, Pdivoom_clock_id_info pclock_info, divoom_lvgl_dial_menu_set_obj_level_callback *pcallback);
void *divoom_lvgl_dial_menu_draw_calendar_watch3(void *handle, void *text_handle, void *pdate_info, void *text_attr, void *obj, Pdivoom_clock_id_info pclock_info, divoom_lvgl_dial_menu_set_obj_level_callback *pcallback);
void *divoom_lvgl_dial_menu_draw_calendar_watch_with_week2(void *handle, void *text_handle, void *pdate_info, void *text_attr, void *obj, Pdivoom_clock_id_info pclock_info, int week_high, int week_widht, int today_x, int today_y, divoom_lvgl_dial_menu_set_obj_level_callback *pcallback);
void *divoom_lvgl_dial_menu_add_hour_min_color_text(void *text_handle, int hour, int min, void *text_attr, void *obj, Pdivoom_clock_id_info pclock_info);
void *divoom_lvgl_dial_menu_draw_calendar_watch_with_img(void *handle, void *text_handle, void *pdate_info, void *text_attr, void *obj, Pdivoom_clock_id_info pclock_info, int img_width, int img_offset, divoom_lvgl_dial_menu_set_obj_level_callback *plevel_callback);
void *divoom_lvgl_dial_menu_draw_calendar_watch_with_week_use_img(void *handle, void *text_handle, void *pdate_info, void *text_attr, void *obj, Pdivoom_clock_id_info pclock_info, int week_high, int week_widht, int today_x, int today_y
		, int week_type, int img_width, int img_offset, divoom_lvgl_dial_menu_set_obj_level_callback *plevel_callback);

/*********************************************************************
  Function:        divoom_lvgl_dial_menu_add_pinball_marble
  Description:     增加弹球
  Input:           handle   第一次为空，以后给第一次使用返回的句柄
                   pimage_info    图像信息
  Return:          handle
**********************************************************************/

void *divoom_lvgl_dial_menu_add_pinball_marble(void *handle, Pdivoom_lvgl_dial_menu_image_item pimage_info, void *obj, divoom_lvgl_dial_menu_set_obj_level_callback *pcallback);

/*********************************************************************
  Function:        divoom_lvgl_dial_menu_add_buddy
  Description:     表盘 disp497：创建/更新 Buddy 表情控件（内部持 LVGL 锁）
  Input:           handle 首次 NULL，之后传上次返回的 lv_obj 句柄
                   pbuddy_info 位置/尺寸/模式
  Return:          Buddy lv_obj 句柄
**********************************************************************/
void *divoom_lvgl_dial_menu_add_buddy(void *handle, Pdivoom_lvgl_dial_menu_buddy_item pbuddy_info, void *obj, void *user_data, divoom_lvgl_dial_menu_set_obj_level_callback *pcallback);


/*********************************************************************
  Function:        divoom_lvgl_dial_menu_add_local_image_handle
  Description:     添加一个标准图像文件显示元素
  Input:           image_handle   第一次为空，以后给第一次使用返回的句柄
                   pimage_info    图像信息，第一次有位置信息以后只关心字串变化，不修改位置
  Return:          handle
**********************************************************************/

void *divoom_lvgl_dial_menu_add_local_image_handle(void *handle, Pdivoom_lvgl_dial_menu_image_item pimage_info, void *obj, int lock_flag, void *user_data, divoom_lvgl_dial_menu_set_obj_level_callback *pcallback, uint8 *exit_flag, int reload_flag);

/*********************************************************************
  Function:        divoom_lvgl_dial_menu_add_music_info_handle
  Description:     添加一个歌词/歌名显示元素，文本变化时即时刷新并循环滚动
  Input:           handle   第一次为空，以后给第一次使用返回的句柄
                   pimage_info    位置、颜色与显示字串
  Return:          handle
**********************************************************************/
void *divoom_lvgl_dial_menu_add_music_info_handle(void *handle, Pdivoom_lvgl_dial_menu_image_item pimage_info, void *obj, void *user_data, divoom_lvgl_dial_menu_set_obj_level_callback *pcallback);

/* 2212 歌词项：在原显示区域内创建至多 256x256 的透明三行歌词控件。 */
void *divoom_lvgl_dial_menu_add_music_multiline_handle(void *handle,
    Pdivoom_lvgl_dial_menu_image_item pimage_info, void *obj, void *user_data,
    divoom_lvgl_dial_menu_set_obj_level_callback *pcallback);

/* 歌词控件已在 LVGL 线程创建完成时返回 1。 */
int divoom_lvgl_dial_menu_music_handle_ready(void *handle);

void divoom_lvgl_dial_menu_move_image_background(void *handle);

void divoom_lvgl_dial_menu_set_sub_obj_transparent(void *obj, int lock_flag);

/*********************************************************************
  Function:        divoom_lvgl_dial_menu_get_backgroud_effect_by_clock_id
  Description:     通过表盘 ID 获取 AI 背景程序化效果类型；未登记返回 NONE，调用方可回退到图片显示。
  Input:           clock_id 表盘 ID
  Return:          DIVOOM_LVGL_DIAL_MENU_BACKGROUD_EFFECT_XXXX
**********************************************************************/

int divoom_lvgl_dial_menu_get_backgroud_effect_by_clock_id(int clock_id);

/*********************************************************************
  Function:        divoom_lvgl_dial_menu_get_backgroud_update_value
  Description:     返回 505 背景项用于刷新比较的内部版本值；0 表示未登记背景效果。
  Input:           clock_id 表盘 ID
  Return:          内部版本值
**********************************************************************/

int divoom_lvgl_dial_menu_get_backgroud_update_value(int clock_id);

/*********************************************************************
  Function:        divoom_lvgl_dial_menu_has_runtime_backgroud
  Description:     判断表盘是否需要程序化背景接管底图显示；具体效果策略统一收口在 dial menu。
  Input:           clock_id 表盘 ID
  Return:          1 需要运行时背景；0 不需要
**********************************************************************/

int divoom_lvgl_dial_menu_has_runtime_backgroud(int clock_id);

/*********************************************************************
  Function:        divoom_lvgl_dial_menu_get_backgroud_layer_count
  Description:     返回程序化背景占用的底层数量，供表盘信息统一放到背景之上。
  Input:           clock_id 表盘 ID
  Return:          0 无程序化背景；1 普通程序化背景；2 静态底图加透明动态叠层
**********************************************************************/

int divoom_lvgl_dial_menu_get_backgroud_layer_count(int clock_id);

/*********************************************************************
  Function:        divoom_lvgl_dial_menu_need_backgroud_disp_item
  Description:     旧表盘缺少 disp=505 时，由 dial menu 决定是否需要补一个背景显示项。
  Input:           clock_id 表盘 ID；disp_list/disp_num 当前配置显示项
  Return:          1 需要补齐；0 不需要
**********************************************************************/

int divoom_lvgl_dial_menu_need_backgroud_disp_item(int clock_id, void *disp_list, int disp_num);

/*********************************************************************
  Function:        divoom_lvgl_dial_menu_init_backgroud_disp_item
  Description:     初始化合成的 disp=505 背景显示项；对象申请与释放仍由 add/clear 流程管理。
  Input:           disp_item 待初始化的 divoom_disp_clock_disp_item 指针
  Return:          none
**********************************************************************/

void divoom_lvgl_dial_menu_init_backgroud_disp_item(void *disp_item);

/*********************************************************************
  Function:        divoom_lvgl_dial_menu_add_backgroud_handle
  Description:     创建/更新 DIVOOM_CLOCK_DISP_SUPPORT_AI_BACKGROUD 对应的程序化背景控件。
  Input:           handle 首次 NULL，之后传上次返回句柄；pimage_info 提供位置、尺寸、层级；clock_id 用于查效果。
  Return:          handle
**********************************************************************/

void *divoom_lvgl_dial_menu_add_backgroud_handle(void *handle, Pdivoom_lvgl_dial_menu_image_item pimage_info, int clock_id, void *obj, void *user_data, divoom_lvgl_dial_menu_set_obj_level_callback *pcallback);

/*********************************************************************
  Function:        divoom_lvgl_dial_menu_add_adaptive_image_handle
  Description:     添加一个外部图像文件显示元素，按比例缩放
  Input:           image_handle   第一次为空，以后给第一次使用返回的句柄
                   pimage_info    图像信息，第一次有位置信息以后只关心字串变化，不修改位置

  Return:          handle
**********************************************************************/

void *divoom_lvgl_dial_menu_add_adaptive_image_handle(void *handle, Pdivoom_lvgl_dial_menu_image_item pimage_info, void *obj, int lock_flag, void *user_data, divoom_lvgl_dial_menu_set_obj_level_callback *pcallback, uint8 *exit_flag);


/*********************************************************************
  Function:        divoom_lvgl_dial_menu_add_rotate_image_handle
  Description:     添加一个标准图像,通过程序能旋转的，些文件一定是PNG的文件
  Input:           handle   第一次为空，以后给第一次使用返回的句柄
                   pimage_info    图像信息，第一次有位置信息以后只关心字串变化，不修改位置

  Return:          handle
**********************************************************************/

void *divoom_lvgl_dial_menu_add_rotate_image_handle(void *handle, Pdivoom_lvgl_dial_menu_image_item pimage_info, void *obj);

/*********************************************************************
  Function:        divoom_lvgl_dial_menu_add_divoom_image_handle
  Description:     添加一个DIVOOM社区的图像显示元素
  Input:           image_handle   第一次为空，以后给第一次使用返回的句柄
                   pimage_info    图像信息，第一次有位置信息以后只关心字串变化，不修改位置
  Return:          handle
**********************************************************************/

void *divoom_lvgl_dial_menu_add_divoom_image_handle(void *handle, Pdivoom_lvgl_dial_menu_image_item pimage_info, void *obj, void *user_data, divoom_lvgl_dial_menu_set_obj_level_callback *pcallback);

/*********************************************************************
  Function:        divoom_lvgl_dial_menu_add_divoom_image_handle
  Description:     添加一个DIVOOM社区的图像文件显示元素
  Input:           image_handle   第一次为空，以后给第一次使用返回的句柄
                   pimage_info    图像信息，第一次有位置信息以后只关心字串变化，不修改位置
  Return:          handle
**********************************************************************/

void *divoom_lvgl_dial_menu_add_divoom_file_handle(void *handle, Pdivoom_lvgl_dial_menu_image_item pimage_info, void *obj, int apha_flag, void *user_data, divoom_lvgl_dial_menu_set_obj_level_callback *pcallback, int is_app_flag);

/*********************************************************************
  Function:        divoom_lvgl_dial_menu_add_rgbbuf_image_handle
  Description:     添加一个RGB BUFFER的图像显示元素
  Input:           image_handle   第一次为空，以后给第一次使用返回的句柄
                   pimage_info    图像信息，第一次有位置信息以后只关心字串变化，不修改位置
  Return:          handle
**********************************************************************/

void *divoom_lvgl_dial_menu_add_rgbbuf_image_handle(void *handle, Pdivoom_lvgl_dial_menu_image_item pimage_info, void *obj, void *user_data, divoom_lvgl_dial_menu_set_obj_level_callback *pcallback);
/*********************************************************************
  Function:        divoom_lvgl_dial_menu_add_photo_slide_image_handle
  Description:     添加一个相册幻灯片显示
  Input:           handle   第一次为空，以后给第一次使用返回的句柄
                   pimage_info    图像信息，第一次有位置信息以后只关心字串变化，不修改位置
                   start
                   total
  Return:          handle
**********************************************************************/

void *divoom_lvgl_dial_menu_add_photo_slide_image_handle(void *handle, Pdivoom_lvgl_dial_menu_image_item pimage_info, void *obj, int movie_flag, int clock_id, int disp_type, void *user_data, divoom_lvgl_dial_menu_set_obj_level_callback *pcallback);


/*********************************************************************
  Function:        divoom_lvgl_dial_menu_add_play_menu_handle
  Description:     添加一个控件显示元素,控件主动自己刷新，所以此函数只调用一次
  Input:           menu_id    控件类型
  Return:          handle
**********************************************************************/


void *divoom_lvgl_dial_menu_add_play_menu_handle(int menu_id);
/*********************************************************************
  Function:        divoom_lvgl_dial_menu_exit
  Description:     添加一个控件显示元素,控件主动自己刷新，所以此函数只调用一次
  Input:           menu_id    控件类型
  Return:          handle
**********************************************************************/


void divoom_lvgl_dial_menu_exit(void *handle);

/*********************************************************************
  Function:        divoom_lvgl_dial_menu_pause
  Description:     在控件
  Input:           menu_id    控件类型
  Return:          handle
**********************************************************************/


void divoom_lvgl_dial_menu_pause(void *handle);

/*********************************************************************
  Function:        divoom_lvgl_dial_menu_add_local_part_image_handle
  Description:     添加一个标准图像文件部分帧做为显示元素，将部分帧全部解码出来显示
  Input:           image_handle   第一次为空，以后给第一次使用返回的句柄
                   pimage_info    图像信息，第一次有位置信息以后只关心字串变化，不修改位置
  Return:          handle
**********************************************************************/

void *divoom_lvgl_dial_menu_add_local_part_image_handle(void *handle, Pdivoom_lvgl_dial_menu_image_item pimage_info, int *start, int cnt, int list_num, void *obj, int play_speed, void *user_data, divoom_lvgl_dial_menu_set_obj_level_callback *pcallback);

/*********************************************************************
  Function:        divoom_lvgl_dial_menu_remove_image_handle
  Description:     删除一个图像句柄
  Input:           handle   句柄
  Return:          none
**********************************************************************/

void divoom_lvgl_dial_menu_remove_image_handle(void *handle);

/*********************************************************************
  Function:        divoom_lvgl_dial_menu_add_local_average_image_handle
  Description:     添加一个标准图像文件平均帧做为显示元素，将部分帧全部解码出来显示
  Input:           image_handle   第一次为空，以后给第一次使用返回的句柄
                   pimage_info    图像信息，第一次有位置信息以后只关心字串变化，不修改位置
                   start
                   total
  Return:          handle
**********************************************************************/

void *divoom_lvgl_dial_menu_add_local_average_image_handle(void *handle, Pdivoom_lvgl_dial_menu_image_item pimage_info, int start, int total, void *obj, int play_speed, void *user_data, divoom_lvgl_dial_menu_set_obj_level_callback *pcallback);

/*********************************************************************
  Function:        divoom_lvgl_dial_menu_add_divoom_image_queue
  Description:     添加一个DIVOOM社区的图像队列显示元素
  Input:           image_handle   这个考虑调用一次
                   pimage_info    图像信息，第一次有位置信息以后只关心字串变化，不修改位置
                   pcallback      获取图像的回调函数
  Return:          handle
**********************************************************************/

void *divoom_lvgl_dial_menu_add_divoom_eq_info(void *handle, Pdivoom_lvgl_dial_menu_image_item pimage_info, DIVOOM_GET_EQ_DISP_BUF_CALLBACK pcallback, void *obj, void *user_data,void *clock_handle,  divoom_lvgl_dial_menu_set_obj_level_callback *plevel_callback);
/* 将新 EQ 元素加入表盘图像队列，沿用配置层级、暂停与销毁生命周期。 */
void *divoom_lvgl_dial_menu_add_lyric_eq_info(void *handle,
    Pdivoom_lvgl_dial_menu_image_item pimage_info, void *obj, int clock_id,
    uint32 color, void *clock_handle, divoom_lvgl_dial_menu_set_obj_level_callback *plevel_callback);
/*********************************************************************
  Function:        divoom_lvgl_dial_menu_add_divoom_image_queue
  Description:     添加一个DIVOOM社区的图像队列显示元素
  Input:           image_handle   这个考虑调用一次
                   pimage_info    图像信息，第一次有位置信息以后只关心字串变化，不修改位置
                   pcallback      获取图像的回调函数
                   gif_time       单个作品的播放时间
  Return:          handle
**********************************************************************/

void *divoom_lvgl_dial_menu_add_divoom_image_queue(void *handle, Pdivoom_lvgl_dial_menu_image_item pimage_info, DIVOOM_PLAY_GIF_COM_GET_NEW_ID pcallback, DIVOOM_PLAY_GIF_COM_GET_PLAY_TIME gif_time_callback, int index, void *obj, void *user_data, void *clock_handle, divoom_lvgl_dial_menu_set_obj_level_callback *plevel_callback, DIVOOM_PLAY_GIF_COM_GET_CHANGE_FLAG pchange_callback);
/*********************************************************************
  Function:        divoom_lvgl_dial_menu_del_image_disp_handle
  Description:     删除某个图像元素
  Input:           pimage_handle   
  Return:          handle
**********************************************************************/

void divoom_lvgl_dial_menu_del_image_disp_handle(void *pimage_handle, int lock_flag);
void divoom_lvgl_dial_menu_del_component_disp_handle(void *pcomponent_handle, int lock_flag);

/*********************************************************************
  Function:        divoom_lvgl_dial_menu_clear_all
  Description:     删除所有元素
  Input:           none   
  Return:          none
**********************************************************************/

void divoom_lvgl_dial_menu_clear_all(void *handle,int lock_flag);

/*********************************************************************
  Function:        divoom_lvgl_dial_menu_set_finish_image_handle_callback
  Description:     设置播放完的回调函数
  Input:           handle   
                   pcallback    
                   user_data
  Return:          NONE
**********************************************************************/

void divoom_lvgl_dial_menu_set_finish_image_handle_callback(void *handle, DIVOOM_LVGL_DIAL_MENU_FINISH_CALLBACK pcallback, void *user_data);

/*********************************************************************
  Function:        divoom_lvgl_dial_menu_update_clock_info
  Description:     更新表盘显示
  Input:           data    
                   lock_flag   是否加lvgl锁
  Return:          none
**********************************************************************/


void divoom_lvgl_dial_menu_update_clock_info(void *data, int lock_flag);


/*********************************************************************
  Function:        divoom_lvgl_dial_menu_clear_item
  Description:     清除非组件对象，在lvgl内部调用
  Input:           
                  
  Return:          NONE
  Author: 		   wangft
**********************************************************************/

void divoom_lvgl_dial_menu_clear_item(int flag);

/*********************************************************************
  Function:        divoom_lvgl_dial_menu_pause_image_handle
  Description:     暂时某个图像的播放
  Input:           
                  
  Return:          NONE
  Author: 		   wangft
**********************************************************************/

void divoom_lvgl_dial_menu_pause_image_handle(void *handle);

/* 将自管理 LVGL 图像控件登记到通用图像队列。队列负责 PAUSE 和删除根对象，
 * 控件通过根对象 DELETE 事件释放其私有资源。 */
void *divoom_lvgl_dial_menu_add_managed_image_handle(int image_type,
    void *image_obj, void *obj, void *user_data,
    divoom_lvgl_dial_menu_managed_image_pause_callback *pause_callback);

/*********************************************************************
  Function:        divoom_lvgl_dial_menu_del_obj
  Description:     删除子表盘对象
  Input:           obj
  Return:          void
  Author:		   wangft
**********************************************************************/

void divoom_lvgl_dial_menu_del_obj(void *obj);

/* 仅清理某表盘在 dial menu 中登记的文字/图片等子项（均为 main_obj 的子对象），不删除 main_obj 本身。
 * 用于 main_obj 被多处共享（如歌词浮层）时，避免 del_obj 误删 main_obj 造成二次释放。 */
void divoom_lvgl_dial_menu_clear_all_in(void *handle, int flag);
/* After all clock workers stop: release display memory, retaining the menu shell. */
void divoom_lvgl_dial_menu_release_clock_display(void);

/* 凭 LVGL 对象指针清理/删除（表盘句柄已释放、仅持有 obj 时使用）。
 * 用于切换表盘：先 disp_clock_close 停线程释放句柄，再凭捕获的 obj 删除 LVGL，避免线程未停就删对象。 */
void divoom_lvgl_dial_menu_clear_all_in_by_obj(void *obj, int flag);
void divoom_lvgl_dial_menu_del_obj_by_ptr(void *obj);


void *divoom_lvgl_dial_menu_add_obj(int x, int y, int w, int h, int flag, int angle);

/*********************************************************************
  Function:        divoom_lvgl_dial_menu_pause_all_in
  Description:     暂停所有所有显示元素
  Input:           
					obj 针对某个对象 
  Return:          NONE
**********************************************************************/

void divoom_lvgl_dial_menu_pause_all_in(void *handle);
int divoom_lvgl_dial_menu_check_handle_exist(void);


/*********************************************************************
  Function:        divoom_lvgl_dial_menu_add_photo_slide_image_handle
  Description:     添加一个相册幻灯片显示
  Input:           handle   第一次为空，以后给第一次使用返回的句柄
                   pimage_info    图像信息，第一次有位置信息以后只关心字串变化，不修改位置
                   start
                   total
  Return:          handle
**********************************************************************/

void *divoom_lvgl_dial_menu_add_play_video_handle(void *handle, Pdivoom_lvgl_dial_menu_image_item pimage_info, void *obj);

void *divoom_lvgl_dial_menu_open_resource_handle(char *file_name, int *resource_type, int *out_w, int *out_h, int *out_num);

/* 释放 open_resource_handle 返回的解码句柄；resource_type 必须使用打开时的输出值。 */
void divoom_lvgl_dial_menu_release_resource_handle(void *resource_handle, int resource_type);

#if defined(DIVOOM_SIMULATOR)
/* 白盒生命周期探针：统计当前 dial menu 中 Buddy 持有的解码图像缓冲。 */
int divoom_lvgl_dial_menu_whitebox_get_image_buffer_count(void);
/* 白盒生命周期探针：只统计 AI 控件实际拥有的 timer，不包含全局 LVGL timer。 */
int divoom_lvgl_dial_menu_whitebox_get_ai_timer_count(void);
/* Buddy 状态覆盖测试入口；返回 1 表示当前表盘存在 Buddy 控件。 */
int divoom_lvgl_dial_menu_whitebox_set_buddy_state(const char *state_id);
int divoom_lvgl_dial_menu_whitebox_reset_buddy_coverage(void);
uint32 divoom_lvgl_dial_menu_whitebox_get_buddy_coverage(void);
int divoom_lvgl_dial_menu_whitebox_trigger_buddy_action(const char *clip_id);
uint32 divoom_lvgl_dial_menu_whitebox_get_buddy_action_start_count(void);
int divoom_lvgl_dial_menu_whitebox_set_buddy_paused(uint8 paused);
int divoom_lvgl_dial_menu_whitebox_get_buddy_timer_count(void);
int divoom_lvgl_dial_menu_whitebox_get_buddy_active_channel_count(void);
uint32 divoom_lvgl_dial_menu_whitebox_get_buddy_decode_fail_count(void);
int divoom_lvgl_dial_menu_whitebox_get_ai_text_layout(uint8 big_frame,
    int32 *tail_line_count, int32 *y_offset);
int divoom_lvgl_dial_menu_whitebox_check_ai_typography(uint8 big_frame,
    int32 font_id, int32 font_size, int32 letter_space, int32 line_pitch);
int divoom_lvgl_dial_menu_whitebox_refresh_ai_big_text(void);
#endif


#endif

