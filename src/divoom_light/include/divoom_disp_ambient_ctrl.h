#ifndef _divoom_disp_ambient_ctrl_
#define _divoom_disp_ambient_ctrl_


#define TX_SCREEN_WIDTH        DIVOOM_LED_CONTROL_MAX_NUM
#define TX_SCREEN_HEIGHT       1
#define TX_SCREEN_LED_NUM      (TX_SCREEN_WIDTH*TX_SCREEN_HEIGHT)
#define TX_SCREEN_BUF_SIZE     (TX_SCREEN_WIDTH*TX_SCREEN_HEIGHT*3)

enum
{
	DIVOOM_DISP_DRAW_CTRL_PICKUP_LIGHT, //拾音灯
	DIVOOM_DISP_DRAW_CTRL_METEOR_MULTICOLOR, //流星彩色
	DIVOOM_DISP_DRAW_CTRL_COOL_WINDMILL2_MULTICOLOR, //波浪 彩色
	DIVOOM_DISP_DRAW_CTRL_STARS_BREATHE, //星星
	DIVOOM_DISP_DRAW_CTRL_WAVE_FLOW,  // 流动波浪
	DIVOOM_DISP_DRAW_CTRL_RAINBOW_BOT, //流动幻彩
	DIVOOM_DISP_DRAW_CTRL_RUNNING_LIGHT_AWAY, //彩色跑马灯 
	DIVOOM_DISP_DRAW_CTRL_RAINBOW, //彩虹
	DIVOOM_DISP_DRAW_CTRL_MULTICOLOR_END,
	DIVOOM_DISP_DRAW_CTRL_PICKUP_LIGHT_SINGAL, //拾音灯 单色
	DIVOOM_DISP_DRAW_CTRL_METEOR,  //流星单色
	DIVOOM_DISP_DRAW_CTRL_DIFFUSE, //潮汐 单色
	DIVOOM_DISP_DRAW_CTRL_STARS_BREATHE_SIGNAL, //星星 单色
	DIVOOM_DISP_DRAW_CTRL_COOL_WINDMILL2, //波浪 单色
	DIVOOM_DISP_DRAW_CTRL_FORWARD_WINDMILL3, //密语，单点从左到右，再从右到左,单色
	DIVOOM_DISP_DRAW_CTRL_FADE_IN_OUT, //萤火虫 单色
	DIVOOM_DISP_DRAW_CTRL_SIGNAL_COLOR, //常亮 单色
	DIVOOM_DISP_DRAW_CTRL_END,

	DIVOOM_DISP_DRAW_CTRL_COOL_WINDMILL, //心跳， 单点从左到右 单色
	DIVOOM_DISP_DRAW_CTRL_MIDDLE_WINDMILL, //雷达 单色
	DIVOOM_DISP_DRAW_CTRL_FLAME2, //火焰
	DIVOOM_DISP_DRAW_CTRL_COOL_WINDMILL_MULTICOLOR, //心跳， 单点从左到右 彩色
	DIVOOM_DISP_DRAW_CTRL_FLAME, //火焰
	DIVOOM_DISP_DRAW_CTRL_CYC_INSIDE, //内圈旋转
	DIVOOM_DISP_DRAW_CTRL_RAINSTORM, //暴雨 单色
	DIVOOM_DISP_DRAW_CTRL_AREA1, //内圈旋转
	DIVOOM_DISP_DRAW_CTRL_COOL_WINDMILL3, //摇荡， 单点从左到右，再从右到左 单色
	DIVOOM_DISP_DRAW_CTRL_COOL_WINDMILL3_MULTICOLOR, //摇荡， 单点 彩色
	DIVOOM_DISP_DRAW_CTRL_AREA3, //区域3 单色
	DIVOOM_DISP_DRAW_CTRL_FLOW_REPTILE, //流动爬虫, 任务速度太慢不适应
	DIVOOM_DISP_DRAW_CTRL_FORWARD_WINDMILL2,
	DIVOOM_DISP_DRAW_CTRL_FORWARD_WINDMILL, //闪电，单点从左到右
	DIVOOM_DISP_DRAW_CTRL_RED_GREEN_BLUE_WHITE_TEST,
	DIVOOM_DISP_DRAW_CTRL_COOL_WINDMILL_BACK
};

enum
{
	DIVOOM_DISP_DRAW_CTRL_RAINBOW_BOT_SIDE, //流动幻彩
	DIVOOM_DISP_DRAW_CTRL_FLAME2_SIDE, //火焰
	DIVOOM_DISP_DRAW_CTRL_STARS_BREATHE_SIDE, //星星
	DIVOOM_DISP_DRAW_CTRL_RAINBOW_SIDE, //彩虹
	DIVOOM_DISP_DRAW_CTRL_FADE_IN_OUT_SIDE, //萤火虫 单色
	DIVOOM_DISP_DRAW_CTRL_SIGNAL_COLOR_SIDE, //常亮 单色
	DIVOOM_DISP_DRAW_CTRL_COOL_WINDMILL_MULTICOLOR_SIDE, //心跳， 单点从左到右 彩色
	DIVOOM_DISP_DRAW_CTRL_COOL_WINDMILL2_MULTICOLOR_SIDE, //波浪 单点 彩色
	DIVOOM_DISP_DRAW_CTRL_COOL_WINDMILL_SIDE, //心跳， 单点从左到右 单色
	DIVOOM_DISP_DRAW_CTRL_DIFFUSE_SIDE, //潮汐 单色
	DIVOOM_DISP_DRAW_CTRL_RUNNING_LIGHT_AWAY_SIDE, //跑马灯
	DIVOOM_DISP_DRAW_CTRL_MIDDLE_WINDMILL_SIDE, //雷达 单色
	DIVOOM_DISP_DRAW_CTRL_END_SIDE,
	DIVOOM_DISP_DRAW_CTRL_FLAME_SIDE, //火焰
	DIVOOM_DISP_DRAW_CTRL_FLOW_REPTILE_SIDE,
	DIVOOM_DISP_DRAW_CTRL_CYC_INSIDE_SIDE, //内圈旋转
	DIVOOM_DISP_DRAW_CTRL_RAINSTORM_SIDE, //暴雨 单色
	DIVOOM_DISP_DRAW_CTRL_COOL_WINDMILL2_SIDE, //波浪 单色
};

enum
{
	DIVOOM_DISP_DRAW_CTRL_RAINBOW_BOT_BACK, //单个灯珠颜色渐变
	DIVOOM_DISP_DRAW_CTRL_FLAME_BACK, //火焰
	DIVOOM_DISP_DRAW_CTRL_STARS_BREATHE_BACK, //星星
	DIVOOM_DISP_DRAW_CTRL_RAINBOW_BACK, //彩虹
	DIVOOM_DISP_DRAW_CTRL_FADE_IN_OUT_BACK, //萤火虫
	DIVOOM_DISP_DRAW_CTRL_SIGNAL_COLOR_BACK, //常亮
	DIVOOM_DISP_DRAW_CTRL_COOL_WINDMILL2_BACK, //波浪	
	DIVOOM_DISP_DRAW_CTRL_FORWARD_WINDMILL2_BACK, //撞子，两个点一个从左，一个从右
	DIVOOM_DISP_DRAW_CTRL_FORWARD_WINDMILL3_BACK, //密语，单点从左到右，再从右到左
	DIVOOM_DISP_DRAW_CTRL_MIDDLE_WINDMILL_BACK, //雷达
	DIVOOM_DISP_DRAW_CTRL_RAINSTORM_BACK, //暴雨
	DIVOOM_DISP_DRAW_CTRL_FLOW_REPTILE_BACK,//流动爬虫
	DIVOOM_DISP_DRAW_CTRL_END_BACK,
	DIVOOM_DISP_DRAW_CTRL_RUNNING_LIGHT_AWAY_BACK, //跑马灯
	DIVOOM_DISP_DRAW_CTRL_DIFFUSE_BACK, //潮汐
	DIVOOM_DISP_DRAW_CTRL_COOL_WINDMILL_MULTICOLOR_BACK, //心跳， 单点从左到右 彩色
	DIVOOM_DISP_DRAW_CTRL_COOL_WINDMILL2_MULTICOLOR_BACK, //波浪 单点 彩色
};

typedef struct _pot_info
{
    uint8 first_dot;
	uint8 second_dot;
}pot_info;

typedef struct _divoom_disp_draw_fast_run_info
{
	uint8 main_pot;   
	pot_info sub_pot[8]; 
}divoom_disp_draw_fast_run_info, *Pdivoom_disp_draw_fast_run_info;

typedef struct _divoom_disp_draw_fast_run_dot_info
{
	uint8 flag; //控制步骤
	uint16 step_index; //移动步骤下标 
	uint16 index; //外循环	
	int32 time_limite; //时间限制
	uint8 off; //记录当前点的位置，以防被替换
	uint8 new_off; //记录点变化之后的位置
	uint8 fast_run_table_index[TX_SCREEN_WIDTH]; //点的下标
}divoom_disp_draw_fast_run_dot_info, *Pdivoom_disp_draw_fast_run_dot_info;

typedef struct _divoom_disp_draw_water_info
{
	uint8 main_pot;   
	uint8 sub_pot[4][9]; 
}divoom_disp_draw_water_info, *Pdivoom_disp_draw_water_info;

typedef struct _divoom_black_box_draw_info
{
	uint8 box_dot[5];
	uint8 head_off; //头点
	uint8 head_index; //头点是哪个位置
	uint8 flag[5];
	uint8 step_index[5]; //每个点自己控制自己的运动步骤
	uint16 index[5]; 
	uint8 dot_index; //表示这条虫是属于哪个点
	uint8 update_flag;
}divoom_black_box_draw_info, *Pdivoom_black_box_draw_info;

typedef struct _divoom_water_drop_table_info
{
	uint8 flag;    //标志该点是否被占用
	uint8 which_dot; //该点是属于哪个类点的
	uint8 dot_index;
	uint8 bright_flag; //1 表示有从最低亮度变到最亮 0 表示没有
}divoom_water_drop_table_info, *Pdivoom_water_drop_table_info;

typedef struct _divoom_disp_draw_app_data
{
	uint8 app_pic_data[TX_SCREEN_BUF_SIZE]; //存储app发送过来的图片
	uint8 update_pic;
	uint8 disp_mode; //灯光效果
	uint16 speed;	//动画速率
	uint8 set_flag;	//区别命令
}divoom_disp_draw_app_data, *Pdivoom_disp_draw_app_data;

typedef struct _divoom_disp_draw_start_info
{
    uint32 time_limite; 		//记录时间
    uint8 flag; 				//变亮还是变暗
    int light_index; 		//亮度变化到哪个位置
    uint8 index;         	//点的位置
 	divoom_screen_color_info color; //点的颜色   
} divoom_disp_draw_start_info, *Pdivoom_disp_draw_start_info;

typedef struct _divoom_disp_draw_firework_info
{
	uint8 main_pot;
	uint8 sub_pot[9];
}divoom_disp_draw_firework_info, *Pdivoom_disp_draw_firework_info;

typedef struct _divoom_disp_draw_diffuse_info
{
	uint8 main_pot;
	uint8 sub_pot_num; //子点个数
	uint8 sub_pot[20];
}divoom_disp_draw_diffuse_info, *Pdivoom_disp_draw_diffuse_info;

typedef struct _divoom_disp_draw_diffuse_pot_info
{
	uint8 main_pot_dis_flag;//标记该点是否显示
	uint8 main_pot_dis_bright;//标记该点当前显示的亮度
	uint8 main_pot_dis_dir; //标记该点是增亮还是减亮 1增加 0减少
}divoom_disp_draw_diffuse_pot_info, *Pdivoom_disp_draw_diffuse_pot_info;

typedef struct _divoom_disp_draw_diffuse_ctrl_info
{
	divoom_disp_draw_diffuse_pot_info main_pot_info[24]; //主点信息
}divoom_disp_draw_diffuse_ctrl_info, *Pdivoom_disp_draw_diffuse_ctrl_info;

typedef struct _divoom_disp_draw_rainbow_info
{
	uint8 cur_color_index;  //当前的颜色位置
	uint8 next_color_index;	//下一个颜色位置
    uint8 index;         	//点的位置
 	uint8 light_dir; 		//0 增加 1 减少
 	uint8 flag; 
  	divoom_disp_hsv_info hsv; 	
	divoom_screen_color_info frist_color; //记录当前点最初的颜色
 	divoom_screen_color_info color; //渐变中点的颜色   
 	divoom_screen_color_info color1; //统计渐变的变化值
 	divoom_screen_color_info color2; //计算当前渐变的颜色值
}divoom_disp_draw_rainbow_info, *Pdivoom_disp_draw_rainbow_info;

typedef struct _divoom_disp_draw_windmill_back_info
{
	uint8 flag;
 	divoom_screen_color_info color; //渐变中点的颜色   
 	divoom_screen_color_info color1; //统计渐变的变化值
 	divoom_screen_color_info color2; //计算当前渐变的颜色值
}divoom_disp_draw_windmill_back_info, *Pdivoom_disp_draw_windmill_back_info;


typedef struct _divoom_disp_draw_cool_windmill_info
{
	uint8 flag; //标记该点被亮点通过
	divoom_screen_color_info color; //记住该点的颜色
	uint16 light_index; //记住亮度变化值，当亮点通过时该值为最大值255，接着持续变小
	uint8 light_dir; // 0增加 1减少
}divoom_disp_draw_cool_windmill_info, *Pdivoom_disp_draw_cool_windmill_info;

typedef struct _divoom_disp_draw_light_moves_info
{
	uint8 flag; //标记该点停留下来
	divoom_screen_color_info color; //记住该点的颜色
}divoom_disp_draw_light_moves_info, *Pdivoom_disp_draw_light_moves_info;

typedef struct _divoom_disp_draw_windmill_info
{
	uint8 windmill_dot_num; 			 //记录当前风车正在运动的点数
	divoom_screen_color_info color[TX_SCREEN_WIDTH]; //颜色列表
	uint8 windmill_off_table[TX_SCREEN_WIDTH]; //记录上一次每个点的位置
}divoom_disp_draw_windmill_info, *Pdivoom_disp_draw_windmill_info;

typedef struct _divoom_flow_reptile_info
{
	uint8 flag; //是否显示该点
	uint8 index; //标记这个点在该轨迹中是哪个位置
	uint16 light_index; //决定该点显示的亮度值，当到1/2总亮度的时候如果该点是头点，要确定下一个点的位置信息
	uint8 step_index; //进行的步骤
	uint8 dot; //该点的位置
	uint8 wait_flag; //是否等待
}divoom_flow_reptile_info, *Pdivoom_flow_reptile_info;

typedef struct _divoom_flow_reptile_table_info
{
	uint8 flag;    //标志该点是否被占用
	uint8 which_reptile; //该点是属于哪条虫的
}divoom_flow_reptile_table_info, *Pdivoom_flow_reptile_table_info;

typedef struct _divoom_disp_draw_flow_reptile_info
{
	uint8 flag; //标记该条轨迹是否有点能显示
	uint8 delete_flag; // 1表示删除, 0保留
	uint8 update_color_flag; //改变颜色
	uint16 color_index;
	divoom_screen_color_info update_color; //需要改变的颜色
	divoom_screen_color_info color[10]; //记录当前这条虫的颜色
	divoom_flow_reptile_info dot_info[10]; //该条轨迹下点位置信息，每条虫最多是4个点
	divoom_flow_reptile_info dot_info2[10]; //该条轨迹下点位置信息，每条虫最多是4个点
}divoom_disp_draw_flow_reptile_info, *Pdivoom_disp_draw_flow_reptile_info;

typedef struct _divoom_disp_draw_run_away_info
{
	uint8 color_table[TX_SCREEN_WIDTH];
}divoom_disp_draw_run_away_info, *Pdivoom_disp_draw_run_away_info;

typedef struct _divoom_disp_draw_pickup_light_info
{
	uint8 step_index;
	uint8 up_flag; // 1上升 0下降
	uint8 down_pos;
	uint8 last_top; // 上次检测的最高点
	uint8 dot_pos[24];
}divoom_disp_draw_pickup_light_info, *Pdivoom_disp_draw_pickup_light_info;

typedef enum
{
	AREA0,
	AREA1,
	AREA2,
	AREA3,
	AREAEND
}AREA_INDEX;

typedef struct _divoom_particle_info{
	uint8 index;	//点的位置
	uint8 step; 	
	uint8 dis_flag;	//是否可显示 
	uint16 color_step;
	uint8 color_status;
	divoom_screen_color_info color_src[2];
	divoom_screen_color_info color1;
	divoom_screen_color_info static_color; //静态颜色
	divoom_screen_color_info active_color; //动态颜色
} divoom_particle_info, *Pdivoom_particle_info;

#define METEOR_LENGTH 8      // 流星尾巴长度
#define METEOR_NUM    2      // 同时有几颗流星
#define METEOR_SPEED  1      // 每帧移动速度
typedef struct _divoom_meteor_info{
    int pos;      // 当前流星头位置
    int dir;      // 方向（1:正向，-1:反向）
    int hue;      // 色相
	int color_index;
}divoom_meteor_info, *Pdivoom_meteor_info;

// 波浪流动效果的数据结构
typedef struct _divoom_wave_point_t{
    int16_t phase;       // 相位
    int16_t amplitude;   // 波动幅度
    uint8_t color_index; // 颜色索引
    uint8_t speed;       // 移动速度
} divoom_wave_point_t, *Pdivoom_wave_point_t;

typedef struct _divoom_disp_ambient_info
{
	uint8 app_pic_data[TX_SCREEN_BUF_SIZE]; //存储app发送过来的图片
	uint8 off_table[TX_SCREEN_WIDTH]; 	   //保存app发送的图片的点的位置	
	uint8 dot_num;							//点数
	uint8 color_index; //记录颜色下标
	uint32 color_times;
	uint8 next_off_table[TX_SCREEN_WIDTH];	//记住下一次循环每个点要跑的位置
	uint8 next_off_table2[TX_SCREEN_WIDTH];
	uint8 star_blink_dot_num;				//记录随机选择的点数
	uint16 fade_in_out_step_index; //移动步骤下标 
	uint16 fade_in_out_index; //外循环	
	uint8 windmill_dot_num; 			 //记录当前风车正在运动的点数
	uint8 windmill_flag;
	uint8 rainbow_dot_num; //记录彩虹点数
	uint8 area_update_flag; //区域更新标志
	uint16 step_index; 
	uint8 step_bot;
	uint8 step_rand_time;
	AREA_INDEX area_index; //区域下标
	divoom_disp_draw_windmill_back_info windmill_back_info[TX_SCREEN_WIDTH];
	divoom_disp_draw_windmill_back_info windmill3_info[TX_SCREEN_WIDTH];
	divoom_screen_color_info windmill_color[TX_SCREEN_WIDTH]; //记录颜色
	divoom_screen_color_info color[TX_SCREEN_WIDTH]; //颜色列表
	divoom_disp_draw_start_info start_info[TX_SCREEN_WIDTH];
	divoom_disp_draw_rainbow_info rainbow_info[TX_SCREEN_WIDTH];
	divoom_disp_draw_cool_windmill_info cool_windmill_info[TX_SCREEN_WIDTH];
	divoom_disp_draw_cool_windmill_info middle_windmill_info[2][TX_SCREEN_WIDTH];
	divoom_disp_draw_diffuse_ctrl_info diffuse_info;
	divoom_disp_draw_diffuse_ctrl_info rainstorm_info;
	divoom_disp_draw_flow_reptile_info draw_flow_reptile_table;
	divoom_disp_draw_run_away_info run_away_info;
	divoom_particle_info particles[TX_SCREEN_WIDTH]; //灯珠宽度为4，高度最大11
	divoom_wave_point_t wave_points[TX_SCREEN_WIDTH]; 
	divoom_disp_draw_pickup_light_info pickup_light_info; //拾音灯
	divoom_meteor_info meteors[METEOR_NUM]; 
}divoom_disp_ambient_info, *Pdivoom_disp_ambient_info;

#define MAX_RGB_NUM 20
typedef struct _divoom_rgb_light_info
{
	uint8 select_effect; //选中的效果
}divoom_rgb_light_info, *Pdivoom_rgb_light_info;

typedef enum{
	ALL_LIGHT_TYPE,
	SIDE_LIGHT_TYPE,
	BACK_LIGHT_TYPE,
}AMBIENT_LIGHT_TYPE;

typedef struct _divoom_channel_set_rgb_info
{
	uint8 on_off; //开关
	uint8 KeyOnOff; //按键灯开关
	uint8 brightness; //亮度
	uint8 eq_on_off; // 1打开 0 关闭
	int color[3]; //单色的rgb
	int color_cyc_flag; //循环标志 1表示循环 0不循环
	AMBIENT_LIGHT_TYPE select_light_index; //当前要显示那种 0 所有灯 1边缘灯	2 背光
	divoom_rgb_light_info rgb_light_list[3]; //0 所有灯 1 边缘灯 2 背光
}divoom_channel_set_rgb_info, *Pdivoom_channel_set_rgb_info;

typedef struct _divoom_disp_ambient_ctrl_info
{
    uint32 task_id;
	uint8 init_flag;
	uint8 disp_mode; //灯光效果
	uint8 color_cyc_flag; //颜色循环标志
	uint8 multicolor_flag; //1 彩色 0 单色
	uint8 music_power;
	divoom_disp_ambient_info ambient_ctrl_info[2];
	uint16 speed[2];//动画速率
	uint16 step_index[2]; 
	uint8 disp_buf[2][TX_SCREEN_BUF_SIZE];
	divoom_channel_set_rgb_info rgb_info; //保存设置信息
}divoom_disp_ambient_ctrl_info, *Pdivoom_disp_ambient_ctrl_info;


/*********************************************************************
  Function:       divoom_disp_ambient_ctrl_set_disp_mode
  Description:	  设置滤镜的显示模式
  Input:          dir 0 上一个 1 下一个
  Return:         none	
  Author:         wangft
  Data:           2020-5-25
**********************************************************************/
void divoom_disp_ambient_ctrl_set_disp_mode(uint8 dir);
void divoom_disp_ambient_ctrl_set_mode(uint8 mode);
//更新纯色屏
void divoom_disp_ambient_ctrl_init_screen(Pdivoom_screen_color_info pcolor);
void divoom_disp_ambient_ctrl_update(Pdivoom_disp_draw_app_data disp_draw_ctrl_info);
void divoom_disp_ambient_ctrl_init(int screen_status);
void divoom_disp_ambient_ctrl_close(void);

uint8 divoom_disp_ambient_ctrl_get_disp_mode(uint8 *total);

void divoom_disp_ambient_ctrl_set_disp_info(Pdivoom_channel_set_rgb_info rgb_info, uint8 screen_status);
Pdivoom_channel_set_rgb_info divoom_disp_ambient_ctrl_get_disp_info(void);
void divoom_disp_ambient_ctrl_set_rgb_on_off(uint8 on_off);
void divoom_disp_ambient_ctrl_set_brightness(uint8 *brightness, uint8 screen_status);
uint8 divoom_disp_ambient_ctrl_get_brightness(void);
void divoom_disp_ambient_ctrl_set_temp_brightness(uint8 brightness);
uint8 divoom_disp_ambient_ctrl_get_key_switch(void);

#endif

