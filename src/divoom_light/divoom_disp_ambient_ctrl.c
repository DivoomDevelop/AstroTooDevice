#include "divoom.h"
#include "divoom_screen_driver.h"
#include "divoom_oneline_led.h"

//#define DIVOOM_RED_GREEN_BLUE_TEST

#define FIREWORM_INDEX 		(255.0)
#define DIVOOM_DISP_FIREWORM_TIME 1//3
#define DIVOOM_DISP_FIREWORM_INTER	(170) 

#define FADE_IN_OUT_INDEX  (255.0)
#define DIVOOM_DISP_FADE_IN_OUT_TIME 	(1)//(19) 
#define DIVOOM_FADE_IN_OUT_START_LEVE 	0

#define WINDMILL_INDEX  (255.0)
#define DIVOOM_DISP_WINDMILL_TIME 1//5//(64)

#define DIVOOM_DISP_COOL_WINDMILL_TIME 1//6//4//2//(44)

#define MIDDLE_WINDMILL_INDEX  (255.0)
#define DIVOOM_DISP_MIDDLE_WINDMILL_TIME 1//10//6//4

#define DIVOOM_DISP_DIFFUSE_TIME 1//5

#define DIVOOM_DISP_RAINSTROM_TIME 15//8//5

#define STARS_BLINK_INDEX 		200//120
#define DIVOOM_DISP_STARS_BREATHE_TIME 1//5

#define DIVOOM_DISP_RAINBOW_TIME 1//(10)

#define FLOW_REPTILE_INDEX		(255.0)
#define DIVOOM_DISP_FLOW_REPTILE_INTER    155//(50)//(127)

#define RGB_SATURA_VALUE0	125
#define RGB_SATURA_VALUE	255
#define RGB_BRIGHT_VALUE	255

#define DIVOOM_COOL_WINDMINL_STEP 		10
#define DIVOOM_DIFFUSE_STEP 		10

#define DIVOOM_METEOR_TIME	15

#define DIVOOM_DISP_DRAW_CTRL_COOL_WINDMILL_TIME 			DIVOOM_DISP_COOL_WINDMILL_TIME
#define DIVOOM_DISP_DRAW_CTRL_MIDDLE_WINDMILL_TIME 			DIVOOM_DISP_MIDDLE_WINDMILL_TIME
#define DIVOOM_DISP_DRAW_CTRL_DIFFUSE_TIME 					DIVOOM_DISP_DIFFUSE_TIME
#define DIVOOM_DISP_DRAW_CTRL_RAINSTROM_TIME 				DIVOOM_DISP_RAINSTROM_TIME
#define DIVOOM_DISP_DRAW_CTRL_WINDMILL_TIME					DIVOOM_DISP_WINDMILL_TIME
#define DIVOOM_DISP_DRAW_CTRL_STARS_BREATHE_TIME			DIVOOM_DISP_STARS_BREATHE_TIME
#define DIVOOM_DISP_DRAW_CTRL_FADE_IN_OUT_TIME				DIVOOM_DISP_FADE_IN_OUT_TIME
#define DIVOOM_DISP_DRAW_CTRL_FIREWORM_TIME					DIVOOM_DISP_FIREWORM_TIME
#define DIVOOM_DISP_DRAW_CTRL_RAINBOW_TIME					DIVOOM_DISP_RAINBOW_TIME
#define DIVOOM_DISP_DRAW_CTRL_SIGNAL_COLOR_TIME				10
#define DIVOOM_DISP_DRAW_CTRL_AREA1_TIME					1
#define DIVOOM_DISP_DRAW_CTRL_CYC_INSIDE_TIME				25//10//7
#define DIVOOM_DISP_DRAW_CTRL_AREA3_TIME					7
#define DIVOOM_DISP_DRAW_CTRL_FLOW_REPTILE_TIME				1
#define DIVOOM_DISP_DRAW_CTRL_RUN_AWAY_TIME					1//5
#define DIVOOM_DISP_DRAW_CTRL_RED_GREEN_BLUE_WHITE_TIME		100
#define DIVOOM_DISP_DRAW_CTRL_FLAME_TIME					10
#define DIVOOM_DISP_DRAW_CTRL_FLAME2_TIME					10
#define DIVOOM_DISP_DRAW_CTRL_PICKUP_LIHGT_TIME				1
#define DIVOOM_DISP_DRAW_CTRL_METEOR_TIME					1
#define DIVOOM_DISP_DRAW_CTRL_WAVE_FLOW_TIME				1

enum
{
    DIVOOM_DISP_DRAW_START_ADD_LIGHT,
    DIVOOM_DISP_DRAW_START_REDUCE_LIGHT,
    DIVOOM_DISP_DRAW_START_DELETE_DOT,
    DIVOOM_DISP_DRAW_START_END,
  	DIVOOM_DISP_DRAW_START_HOLD,
};

static Pdivoom_disp_ambient_ctrl_info pdivoom_disp_ambient_ctrl_handle = NULL;

#define BACK_LIGHT_START_POT 20
#define BACK_LIGHT_END_POT 24

const divoom_disp_draw_diffuse_info draw_area_table[] = {
	{0, 9, {1,2,4,6,9,11,13,15,17}}, //内
	{25, 9, {26,27,29,31,34,36,38,40,42}}, //内
	{3, 8, {5,7,8,10,12,14,16,18}}, //外
	{28, 8, {30,32,33,35,37,39,41,43}}, //外
	{19, 1, {44}}, //中
	{20, 4, {21,22,23,24}}, //背面
};

const divoom_disp_draw_diffuse_info draw_area2_table[] = {
	{0, 11, {0,1,2,4,6,9,11,13,15,17,19}}, //内
	{25, 11, {25,26,27,29,31,34,36,38,40,42,44}}, //内
	{20, 5, {20,21,22,23,24}}, //背面
	{18,9,{18,16,14,12,10,8,7,5,3}}, //外
	{43,9,{43,41,39,37,35,33,32,30,28}}, //外
};

const divoom_disp_draw_diffuse_info draw_diffuse_left_table[] = {
	{13, 0, {0}},
	{14, 0, {0}},
	{15, 0, {0}},
	{16, 0, {0}},
	{17, 0, {0}},
	{18, 0, {0}},
	{19, 0, {0}},
	{20, 0, {0}},
	{12, 0, {0}},
	{21, 0, {0}},
	{11, 0, {0}},
	{10, 0, {0}},
	{22, 0, {0}},
	{9, 1, {23}},
	{8, 0, {0}},
	{7, 0, {0}},
	{6, 1, {24}},
	{5, 0, {0}},
	{4, 0, {0}},
	{25, 0, {0}},
	{3, 0, {0}},
	{2, 0, {0}},
	{1, 0, {0}},
	{0, 0, {0}}
};


const divoom_disp_draw_diffuse_info draw_flame_right_table[] = {
	{44, 0, {44}},
	{43, 0, {0}},
	{42, 0, {42}},
	{41, 0, {40}},
	{40, 0, {0}},
	{39, 0, {38}},
	{38, 0, {37}},
	{37, 0, {36}},
	{36, 0, {35}},
	{35, 0, {34}},
	{34, 0, {33}},
	{33, 0, {0}},
	{20, 0, {0}},
	{21, 0, {0}},
	{22, 0, {0}},
	{23, 0, {0}},
	{24, 0, {0}}
};

const divoom_disp_draw_diffuse_info draw_flame_left_table[] = {
	{19, 0, {0}},
	{18, 0, {0}},
	{17, 0, {0}},
	{16, 0, {0}},
	{15, 0, {15}},
	{14, 0, {0}},
	{13, 0, {13}},
	{12, 0, {12}},
	{11, 0, {11}},
	{10, 0, {10}},
	{9, 0, {9}},
	{8, 0, {8}},
	{20, 0, {0}},
	{21, 0, {0}},
	{22, 0, {0}},
	{23, 0, {0}},
	{24, 0, {0}}
};

const divoom_disp_draw_diffuse_info draw_rainstorm_table[3][9] = 
{
	{
		{18, 0, {0}},
		{15, 1, {1}},
		{12, 1, {3}},
		{9, 1, {6}},
		{21, 0, {0}},
		{24, 0, {0}},
		{30, 1, {35}},
		{27, 1, {38}},
		{44, 1, {41}},
	},

	{
		{17, 1, {0}},
		{14, 0, {0}},
		{11, 1, {4}},
		{8, 1, {7}},
		{22, 0, {0}},
		{32, 1, {33}},
		{29, 1, {36}},
		{39, 0, {0}},
		{25, 1, {42}},
	},
		
	{
		{16, 1, {19}},
		{13, 1, {2}},
		{10, 1, {5}},
		{20, 0, {0}},
		{23, 0, {0}},
		{31, 1, {34}},
		{28, 1, {37}},
		{26, 1, {40}},
		{43, 0, {0}}
	}
};

const divoom_screen_color_info draw_rainbow_color_table[] = 
{
	{255, 0, 0},
	{255, 170, 0},
	{255, 255, 0},
	{173, 255, 0},
	{0, 255, 0},
	{0, 255, 170},
	{0, 255, 255},
	{0, 170, 255},
	{0, 85, 255},
	{170, 0, 255},
	{255, 0, 255},
	{255, 0, 85},
};

#define RAINBOW_COLOR_NUM 11

const divoom_screen_color_info draw_rainbow_color_table2[] = 
{
	{255, 0, 0},
	{255, 255, 0},
	{0, 255, 0},
	{0, 255, 255},
	{0, 0, 255},
	{255, 0, 255},
};

#define RAINBOW_COLOR_NUM2 5

const divoom_screen_color_info draw_flame_color_table[] = { //火焰颜色表
	{255, 84, 0},
	{247, 89, 10},
	{240, 93, 21},
	{230, 90, 20},
	{224, 89, 22},
	{218, 91, 27},
	{214, 92, 34},
	{210, 88, 30},
	{206, 86, 27},
	{200, 80, 18},
	{198, 73, 12},
	{189, 70, 10},
	{181, 67, 11},
	{178, 63, 7},
	{174, 59, 3},
	{169, 70, 24},
	{166, 81, 48}
};


#define FLAME_COLOR_NUM 16

const divoom_screen_color_info draw_rainbow_bot_color_table[5][6] = {
	{
		{255, 0, 0},
		{255, 255, 0},
		{0, 255, 0},
		{0, 255, 255},
		{0, 0, 255},
		{255, 0, 255},
	},
	{
		{255, 0, 0},
		{255, 255, 0},
		{0, 255, 0},
		{0, 255, 255},
		{0, 0, 255},
		{255, 0, 255},
	},
	{
		{255, 0, 0},
		{255, 255, 0},
		{0, 255, 0},
		{0, 255, 255},
		{0, 0, 255},
		{255, 0, 255},
	},
	{
		{255, 0, 0},
		{255, 255, 0},
		{0, 255, 0},
		{0, 255, 255},
		{0, 0, 255},
		{255, 0, 255},
	},
	{
		{255, 0, 0},
		{255, 255, 0},
		{0, 255, 0},
		{0, 255, 255},
		{0, 0, 255},
		{255, 0, 255},
	}
};

#define RAINBOW_BOT_COLOR_NUM 5

divoom_screen_color_info draw_run_away_color_table[] = {
	{255, 0, 0},
	{255, 85, 0},
	{255, 170, 0},
	{255, 255, 0},
	{173, 255, 0},
	{0, 255, 0},
	{0, 255, 170},
	{0, 255, 255},
	{0, 170, 255},
	{0, 85, 255},
	{170, 0, 255},
	{255, 0, 255},
	{255, 0, 85},
	{0, 0, 255},
};

divoom_screen_color_info draw_run_away_color_table2[40];

divoom_screen_color_info draw_signal_color_table[] = {
	{255, 255, 255},
	{255, 0, 0},
	{0, 255, 0},
	{0, 0, 255},
};

uint8 particle_dot_xy[] = {
	8, 7, 32, 33,
	9, 6, 31, 34,
	10, 5, 30, 35,
	11, 4, 29, 36,
	12, 3, 28, 37,
	13, 2, 27, 38,
	15, 1, 26, 40,
	16, 19, 44, 41,
	17, 0, 25, 42,
};

uint8 particle_dot_xy2[] = {
	17, 0, 25, 42,
	16, 19, 44, 41,
	15, 1, 26, 40,
	13, 2, 27, 38,
	12, 3, 28, 37,
	11, 4, 29, 36,
	10, 5, 30, 35,
	9, 6, 31, 34,
	8, 7, 32, 33,
};

const uint8 flow_dot_table[] = {25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};
const uint8 flow_dot_table2[] = {44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18};
const uint8 draw_flow_reptile[] = {18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43};
const uint8 flow_reptile[] = {20, 21, 22, 23, 24};
const uint8 rainbow_bot_table[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25};
const divoom_disp_draw_diffuse_info run_away_bot_table[] = {
	{13, 0, {0}},
	{14, 0, {0}},
	{15, 0, {0}},
	{16, 0, {0}},
	{17, 0, {0}},
	{18, 0, {0}},
	{19, 0, {0}},
	{20, 0, {0}},
	{12, 0, {0}},
	{21, 0, {0}},
	{11, 0, {0}},
	{10, 0, {0}},
	{22, 0, {0}},
	{9, 1, {23}},
	{8, 0, {0}},
	{7, 0, {0}},
	{6, 1, {24}},
	{5, 0, {0}},
	{4, 0, {0}},
	{25, 0, {0}},
	{3, 0, {0}},
	{2, 0, {0}},
	{1, 0, {0}},
	{0, 0, {0}}
};

// 波浪效果的颜色表
static const divoom_screen_color_info wave_color_table[] = {
    {255, 0, 0},     // 鲜红色
    {255, 128, 0},   // 橙色
    {255, 255, 0},   // 亮黄色
    {128, 255, 0},   // 黄绿色
    {0, 255, 0},     // 鲜绿色
    {0, 255, 128},   // 青绿色
    {0, 255, 255},   // 青色
    {0, 128, 255},   // 天蓝色
    {0, 0, 255},     // 蓝色
    {128, 0, 255},   // 紫色
    {255, 0, 255},   // 品红色
    {255, 0, 128},   // 玫瑰红
    {255, 255, 0},   // 黄色
    {0, 255, 0},     // 绿色
    {255, 0, 0},     // 红色
};

#define WAVE_COLOR_NUM (sizeof(wave_color_table)/sizeof(wave_color_table[0]))

// 预计算的sin/cos查找表(角度范围0-360度)
// 值放大1000倍以提高精度
static const int sin_table[360] = {    
    0, 17, 34, 52, 69, 87, 104, 121, 139, 156, 173, 190, 207, 224, 241, 258, 275, 292, 309, 325, 
    342, 358, 374, 390, 406, 422, 438, 453, 469, 484, 500, 515, 529, 544, 559, 573, 587, 601, 615, 629, 
    642, 656, 669, 681, 694, 707, 719, 731, 743, 754, 766, 777, 788, 798, 809, 819, 829, 838, 848, 857, 
    866, 874, 882, 891, 898, 906, 913, 920, 927, 933, 939, 945, 951, 956, 961, 965, 970, 974, 978, 981, 
    984, 987, 990, 992, 994, 996, 997, 998, 999, 999, 1000, 999, 999, 998, 997, 996, 994, 992, 990, 987, 
    984, 981, 978, 974, 970, 965, 961, 956, 951, 945, 939, 933, 927, 920, 913, 906, 898, 891, 882, 874, 
    866, 857, 848, 838, 829, 819, 809, 798, 788, 777, 766, 754, 743, 731, 719, 707, 694, 681, 669, 656, 
    642, 629, 615, 601, 587, 573, 559, 544, 529, 515, 500, 484, 469, 453, 438, 422, 406, 390, 374, 358, 
    342, 325, 309, 292, 275, 258, 241, 224, 207, 190, 173, 156, 139, 121, 104, 87, 69, 52, 34, 17, 
    0, -17, -34, -52, -69, -87, -104, -121, -139, -156, -173, -190, -207, -224, -241, -258, -275, -292, -309, -325, 
    -342, -358, -374, -390, -406, -422, -438, -453, -469, -484, -499, -515, -529, -544, -559, -573, -587, -601, -615, -629, 
    -642, -656, -669, -681, -694, -707, -719, -731, -743, -754, -766, -777, -788, -798, -809, -819, -829, -838, -848, -857, 
    -866, -874, -882, -891, -898, -906, -913, -920, -927, -933, -939, -945, -951, -956, -961, -965, -970, -974, -978, -981, 
    -984, -987, -990, -992, -994, -996, -997, -998, -999, -999, -1000, -999, -999, -998, -997, -996, -994, -992, -990, -987, 
    -984, -981, -978, -974, -970, -965, -961, -956, -951, -945, -939, -933, -927, -920, -913, -906, -898, -891, -882, -874, 
    -866, -857, -848, -838, -829, -819, -809, -798, -788, -777, -766, -754, -743, -731, -719, -707, -694, -681, -669, -656, 
    -642, -629, -615, -601, -587, -573, -559, -544, -529, -515, -500, -484, -469, -453, -438, -422, -406, -390, -374, -358, 
    -342, -325, -309, -292, -275, -258, -241, -224, -207, -190, -173, -156, -139, -121, -104, -87, -69, -52, -34, -17, 

};

// 初始化波浪流动效果
void divoom_disp_ambient_ctrl_wave_flow_init(uint8 k)
{
    uint8 i;
    divoom_wave_point_t *points = pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].wave_points;
    
    // 获取随机起始颜色索引
    uint8 start_color = divoom_com_get_random(WAVE_COLOR_NUM);
    
    // 初始化每个点
    for(i = 0; i < TX_SCREEN_WIDTH; i++) {
        points[i].phase = i * 30;  // 增加相位间隔，使波浪更明显
        points[i].amplitude = 8;   // 增加波动幅度
        // 从随机起始颜色开始，均匀分布颜色
        points[i].color_index = (start_color + (i * WAVE_COLOR_NUM) / TX_SCREEN_WIDTH) % WAVE_COLOR_NUM;
        points[i].speed = 5;       // 增加移动速度
    }
    
    pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].dot_num = TX_SCREEN_WIDTH;
}

// 更新波浪流动效果
void divoom_disp_ambient_ctrl_wave_flow_main(uint8 k)
{
    uint8 i;
    divoom_screen_color_info color;
    divoom_wave_point_t *points = pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].wave_points;
    
    // 清空显示缓冲区
    memset(pdivoom_disp_ambient_ctrl_handle->disp_buf[k], 0, TX_SCREEN_BUF_SIZE);
    
    // 更新每个点的位置和颜色
    for(i = 0; i < pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].dot_num; i++) {
        // 更新相位
        points[i].phase = (points[i].phase + points[i].speed) % 360;
        
        // 计算亮度 - 使用正弦函数创建波浪效果
        // sin_table的值范围是-1000到1000，需要先除以1000得到-1到1的范围
        int sin_value = sin_table[points[i].phase] / 1000;
        // 将正弦值映射到亮度范围，sin_value范围是-1到1，所以需要加1再除以2得到0到1的范围
		uint8_t brightness = 160 + (sin_table[points[i].phase / 10] * points[i].amplitude) / 100;
        
        // 设置颜色
        color = wave_color_table[points[i].color_index];
        color.r_c = (color.r_c * brightness) >> 8;
        color.g_c = (color.g_c * brightness) >> 8;
        color.b_c = (color.b_c * brightness) >> 8;
        
        // 绘制到显示缓冲区
        divoom_disp_ctrl_set_data(i, &color, pdivoom_disp_ambient_ctrl_handle->disp_buf[k]);
    }
}

void divoom_disp_ambient_ctrl_meteor_init(uint8 k)
{
    int i, color_index = divoom_com_get_random(sizeof(draw_rainbow_color_table)/sizeof(draw_rainbow_color_table[0]));
    for (i = 0; i < METEOR_NUM; i++) 
	{
        pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].meteors[i].pos = divoom_com_get_random(26); // 0~25
        pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].meteors[i].dir = divoom_com_get_random(2) ? 1 : -1;
        pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].meteors[i].hue = divoom_com_get_random(H_MAX);
		pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].meteors[i].color_index = color_index;
    }
}

void divoom_disp_ambient_ctrl_meteor_main(uint8 k)
{
    int i, j, index = 0;
    divoom_screen_color_info tail_color = {0}, color = {0};

	pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].step_index++;
	if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].step_index%DIVOOM_METEOR_TIME != 0)
		return;

	if (pdivoom_disp_ambient_ctrl_handle->color_cyc_flag == 0)
	{
		color.r_c = pdivoom_disp_ambient_ctrl_handle->rgb_info.color[0];
		color.g_c = pdivoom_disp_ambient_ctrl_handle->rgb_info.color[1];
		color.b_c = pdivoom_disp_ambient_ctrl_handle->rgb_info.color[2];
	}
	else
	{
		index = pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].meteors[0].color_index;
		color.r_c = draw_rainbow_color_table[index].r_c;
		color.g_c = draw_rainbow_color_table[index].g_c;
		color.b_c = draw_rainbow_color_table[index].b_c;
	}

    memset(pdivoom_disp_ambient_ctrl_handle->disp_buf[k], 0, TX_SCREEN_BUF_SIZE);
    for (i = 0; i < METEOR_NUM; i++) 
    {
        // 检查尾巴是否完全消失
        int tail_end_pos = pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].meteors[i].pos - METEOR_LENGTH * pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].meteors[i].dir;
        if (tail_end_pos < 0 || tail_end_pos > 25) 
		{
            // 尾巴完全消失，更新头部颜色
            uint8 hue = divoom_com_get_random(H_MAX);
            pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].meteors[i].hue = hue;
			if (pdivoom_disp_ambient_ctrl_handle->color_cyc_flag == 1)
			{
				pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].meteors[i].color_index++;
				if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].meteors[i].color_index > 
						sizeof(draw_rainbow_color_table)/sizeof(draw_rainbow_color_table[0]))
				{
					pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].meteors[i].color_index = 0;
				}

				index = pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].meteors[i].color_index;
				color.r_c = draw_rainbow_color_table[index].r_c;
				color.g_c = draw_rainbow_color_table[index].g_c;
				color.b_c = draw_rainbow_color_table[index].b_c;
			}
        }

        // 头部 - 使用当前颜色
        divoom_disp_ctrl_set_data(pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].meteors[i].pos, &color, pdivoom_disp_ambient_ctrl_handle->disp_buf[k]);
        for (j = 1; j < METEOR_LENGTH; j++) 
        {
            int tail_pos = pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].meteors[i].pos - j * pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].meteors[i].dir;
            if (tail_pos < 0 || tail_pos > 25) 
                break;
            
            // 尾巴跟头一样的颜色
            // 保持亮度渐变效果
            int brightness = 255 - (255 * j / METEOR_LENGTH);
			divoom_rgb_hsv_change_brightness(&color, &tail_color, brightness, 255);
            divoom_disp_ctrl_set_data(tail_pos, &tail_color, pdivoom_disp_ambient_ctrl_handle->disp_buf[k]);
        }

        // 移动
        pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].meteors[i].pos += pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].meteors[i].dir * METEOR_SPEED;
        if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].meteors[i].pos < 0 || pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].meteors[i].pos > 25) 
		{
            pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].meteors[i].pos = (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].meteors[i].dir == 1) ? 0 : 25;
            pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].meteors[i].hue = (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].meteors[i].hue +1) % H_MAX; // 换色
        }
    }
}

void divoom_disp_ambient_ctrl_meteor_multicolor_main(uint8 k)
{
    int i, j, index = 0;
    divoom_screen_color_info tail_color = {0}, color = {0};

	pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].step_index++;
	if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].step_index%DIVOOM_METEOR_TIME != 0)
		return;

	index = pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].meteors[0].color_index;
	color.r_c = draw_rainbow_color_table[index].r_c;
	color.g_c = draw_rainbow_color_table[index].g_c;
	color.b_c = draw_rainbow_color_table[index].b_c;

    memset(pdivoom_disp_ambient_ctrl_handle->disp_buf[k], 0, TX_SCREEN_BUF_SIZE);
    for (i = 0; i < METEOR_NUM; i++) 
    {
        // 检查尾巴是否完全消失
        int tail_end_pos = pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].meteors[i].pos - METEOR_LENGTH * pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].meteors[i].dir;
        if (tail_end_pos < 0 || tail_end_pos > 25) 
		{
            // 尾巴完全消失，更新头部颜色
            uint8 hue = divoom_com_get_random(H_MAX);
            pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].meteors[i].hue = hue;
			pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].meteors[i].color_index++;
			if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].meteors[i].color_index > 
					sizeof(draw_rainbow_color_table)/sizeof(draw_rainbow_color_table[0]))
			{
				pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].meteors[i].color_index = 0;
			}

			index = pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].meteors[i].color_index;
			color.r_c = draw_rainbow_color_table[index].r_c;
			color.g_c = draw_rainbow_color_table[index].g_c;
			color.b_c = draw_rainbow_color_table[index].b_c;
        }

        // 头部 - 使用当前颜色
        divoom_disp_ctrl_set_data(pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].meteors[i].pos, &color, pdivoom_disp_ambient_ctrl_handle->disp_buf[k]);

        // 尾巴 - 使用彩虹色表中的不同颜色
        for (j = 1; j < METEOR_LENGTH; j++) 
        {
            int tail_pos = pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].meteors[i].pos - j * pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].meteors[i].dir;
            if (tail_pos < 0 || tail_pos > 25) 
                break;
            
            // 为尾巴的每个部分选择不同的颜色
            int tail_color_index = (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].meteors[i].color_index + j) % 
                                 (sizeof(draw_rainbow_color_table)/sizeof(draw_rainbow_color_table[0]));
            
            tail_color.r_c = draw_rainbow_color_table[tail_color_index].r_c;
            tail_color.g_c = draw_rainbow_color_table[tail_color_index].g_c;
            tail_color.b_c = draw_rainbow_color_table[tail_color_index].b_c;
            
            // 保持亮度渐变效果
            int brightness = 255 - (255 * j / METEOR_LENGTH);
            divoom_rgb_hsv_change_rgb(&tail_color, &tail_color, -1, 255, brightness);
            divoom_disp_ctrl_set_data(tail_pos, &tail_color, pdivoom_disp_ambient_ctrl_handle->disp_buf[k]);
        }

        // 移动
        pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].meteors[i].pos += pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].meteors[i].dir * METEOR_SPEED;
        if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].meteors[i].pos < 0 || pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].meteors[i].pos > 25) 
		{
            pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].meteors[i].pos = (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].meteors[i].dir == 1) ? 0 : 25;
            pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].meteors[i].hue = (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].meteors[i].hue +1) % H_MAX; // 换色
        }
    }
}

void divoom_disp_ambient_ctrl_save_config(void);
int divoom_disp_ambient_ctrl_get_speed_time(uint8 disp_mode, int flag); //初始化变量

void divoom_disp_ambient_ctrl_update_freq(long int *music_freq)
{
    uint8 i,j, time, disp_mode = 0;
	uint16 music_power = 0;
    uint8 disp_info[SCREEN_WIDTH_64];

	if (pdivoom_disp_ambient_ctrl_handle == NULL)
		return;

    for(i = 0; i < SCREEN_WIDTH_64; i ++)
    {
        if(music_freq[i] < 64)
        {
		    disp_info[i] = 0;	
        }
        else if(music_freq[i] < 64 + 8 * 8)
        {
		    disp_info[i] = (music_freq[i] - 64)/8;	
        }            
        else if(music_freq[i] < 64 + 8 * 5 + 32 * 5/2)
        {
		    disp_info[i] = (music_freq[i] - 64 - 8 * 8)*2/5 + 8;	
        }
        else
        {
		    disp_info[i] = (music_freq[i] -64 - 8 * 8 - 32 * 5/2)/6 + 40;	
        }
        
        if(disp_info[i] >= SCREEN_WIDTH_64)
        {
            disp_info[i] = SCREEN_WIDTH_64 - 1;
        }
    }

    for(i = 6,j = 0, music_power = 0; i < 10; i ++)
	{
		if(disp_info[i])
		{
			music_power += disp_info[i];
			j++;	
		}
	}
    if(j)
    {
	    music_power/=j;
    }

	if (pdivoom_disp_ambient_ctrl_handle->color_cyc_flag == 0)
		disp_mode = pdivoom_disp_ambient_ctrl_handle->disp_mode + DIVOOM_DISP_DRAW_CTRL_MULTICOLOR_END + 1;

	if (pdivoom_disp_ambient_ctrl_handle->task_id != 0 && pdivoom_disp_ambient_ctrl_handle->rgb_info.eq_on_off)
	{
		pdivoom_disp_ambient_ctrl_handle->music_power = music_power;
		music_power /= 3;
		if (disp_mode == DIVOOM_DISP_DRAW_CTRL_PICKUP_LIGHT || disp_mode == DIVOOM_DISP_DRAW_CTRL_PICKUP_LIGHT_SINGAL)
		{
			if (pdivoom_disp_ambient_ctrl_handle->music_power > sizeof(draw_diffuse_left_table)/sizeof(draw_diffuse_left_table[0]))
				pdivoom_disp_ambient_ctrl_handle->music_power = sizeof(draw_diffuse_left_table)/sizeof(draw_diffuse_left_table[0]);

			if (pdivoom_disp_ambient_ctrl_handle->music_power < 7)
				pdivoom_disp_ambient_ctrl_handle->music_power = 0;
			return;
		}

//		time = divoom_disp_ambient_ctrl_get_speed_time(pdivoom_disp_ambient_ctrl_handle->disp_mode, 0);
		if (music_power > 10)
			music_power = 10;
		music_power = 10 - music_power;
		if (music_power == 0)
			music_power = 1;
		divoom_com_update_task(pdivoom_disp_ambient_ctrl_handle->task_id, music_power);
	}
	else
	{
		pdivoom_disp_ambient_ctrl_handle->music_power = 0;
	}
}

void divoom_disp_ambient_ctrl_send_rgb_data(uint8 *disp_buf)
{
	divoom_oneline_led_send_rgb_data(disp_buf);
}

void divoom_disp_ambient_ctrl_led_on_off(uint8 on_off)
{
	if (on_off == 0)
	{
		divoom_oneline_led_on_off(0);
	}
	else
	{
		divoom_oneline_led_on_off(1);
	}
}

void divoom_disp_ambient_ctrl_set_all_light_pot_table(Pdivoom_disp_draw_app_data pdisp_draw_ctrl_info, uint8 disp_mode)
{
	uint8 k = 0;
	uint8 i = 0, off = 0, max_dot_num = TX_SCREEN_LED_NUM, index = 0;
	uint8 max_color_num = sizeof(draw_rainbow_color_table)/sizeof(draw_rainbow_color_table[0]);
	divoom_screen_color_info color = {0}, color1, color2;

	color.r_c = pdivoom_disp_ambient_ctrl_handle->rgb_info.color[0];
	color.g_c = pdivoom_disp_ambient_ctrl_handle->rgb_info.color[1];
	color.b_c = pdivoom_disp_ambient_ctrl_handle->rgb_info.color[2];
	pdivoom_disp_ambient_ctrl_handle->color_cyc_flag = pdivoom_disp_ambient_ctrl_handle->rgb_info.color_cyc_flag;

	if (color.r_c == 0 && color.g_c == 0 && color.b_c == 0)
	{
		color.r_c = 0x3d;
		color.g_c = 0x4f;
		color.b_c = 0xff;
	}
	pdisp_draw_ctrl_info->disp_mode = disp_mode;
	if (pdivoom_disp_ambient_ctrl_handle->color_cyc_flag == 0)
		disp_mode = disp_mode + DIVOOM_DISP_DRAW_CTRL_MULTICOLOR_END + 1;

	switch (disp_mode)
	{
		case DIVOOM_DISP_DRAW_CTRL_COOL_WINDMILL:
		case DIVOOM_DISP_DRAW_CTRL_COOL_WINDMILL2:
		case DIVOOM_DISP_DRAW_CTRL_COOL_WINDMILL3:
		case DIVOOM_DISP_DRAW_CTRL_COOL_WINDMILL_MULTICOLOR:
		case DIVOOM_DISP_DRAW_CTRL_COOL_WINDMILL3_MULTICOLOR:
		case DIVOOM_DISP_DRAW_CTRL_COOL_WINDMILL2_MULTICOLOR:
			for (i = 0; i < 1; i++)
			{
				off = 0; 
				divoom_disp_ctrl_set_data(off, &color, pdisp_draw_ctrl_info->app_pic_data);
			}
			break;

		case DIVOOM_DISP_DRAW_CTRL_MIDDLE_WINDMILL:
			divoom_disp_ctrl_set_data(0, &color, pdisp_draw_ctrl_info->app_pic_data);
			divoom_disp_ctrl_set_data(1, &color, pdisp_draw_ctrl_info->app_pic_data);
			break;

		case DIVOOM_DISP_DRAW_CTRL_DIFFUSE:
			divoom_disp_ctrl_set_data(0, &color, pdisp_draw_ctrl_info->app_pic_data); 
			break;

		case DIVOOM_DISP_DRAW_CTRL_RAINSTORM:
			divoom_disp_ctrl_set_data(0, &color, pdisp_draw_ctrl_info->app_pic_data); 
			break;

		case DIVOOM_DISP_DRAW_CTRL_STARS_BREATHE:
			for (i = 0; i < max_dot_num; i++)
			{
				off = i;
				index = divoom_com_get_random(max_color_num);
				divoom_disp_ctrl_set_data(off, &draw_rainbow_color_table[index], pdisp_draw_ctrl_info->app_pic_data);
			}
			break;

		case DIVOOM_DISP_DRAW_CTRL_STARS_BREATHE_SIGNAL:
			for (i = 0; i < max_dot_num; i++)
			{
				off = i;
				divoom_disp_ctrl_set_data(off, &color, pdisp_draw_ctrl_info->app_pic_data);
			}
			break;

		case DIVOOM_DISP_DRAW_CTRL_FORWARD_WINDMILL:
		case DIVOOM_DISP_DRAW_CTRL_FORWARD_WINDMILL2:
		case DIVOOM_DISP_DRAW_CTRL_FORWARD_WINDMILL3:
			for (i = 0; i < 1; i++)
			{
				off = 0; 
				divoom_disp_ctrl_set_data(off, &color, pdisp_draw_ctrl_info->app_pic_data);
			}
			break;

		case DIVOOM_DISP_DRAW_CTRL_RAINBOW:
			index = divoom_com_get_random(max_color_num);
			for (i = 0; i < max_dot_num; i++)
			{
				off = i;
				divoom_disp_ctrl_set_data(off, &draw_rainbow_color_table[index], pdisp_draw_ctrl_info->app_pic_data);
			}
			break;

		case DIVOOM_DISP_DRAW_CTRL_RAINBOW_BOT:
			for (i = 0; i < max_dot_num; i++)
			{
				if (i >= sizeof(rainbow_bot_table)/sizeof(rainbow_bot_table[0]))
					break;
				off = rainbow_bot_table[i];
				divoom_disp_ctrl_set_data(off, &draw_rainbow_bot_color_table[0][0], pdisp_draw_ctrl_info->app_pic_data);
			}
			break;

		case DIVOOM_DISP_DRAW_CTRL_FLAME:
			index = 0;
			for (i = 0; i < sizeof(draw_flame_left_table)/sizeof(draw_flame_left_table[0]); i++)
			{
				//记住单边主点
				off = draw_flame_left_table[i].main_pot;
				if (off == 20)
					index = 0;
				divoom_disp_ctrl_set_data(off, &draw_flame_color_table[index], pdisp_draw_ctrl_info->app_pic_data);
				if (off >= 20)
					index += 4; 
				index++;
				index = index %(FLAME_COLOR_NUM+ 1);
			}	
			break;	
		
		case DIVOOM_DISP_DRAW_CTRL_FLAME2:
			break;

		case DIVOOM_DISP_DRAW_CTRL_FADE_IN_OUT:
			for (i = 0; i < max_dot_num; i++)
			{
				off = i;
				divoom_disp_ctrl_set_data(off, &color, pdisp_draw_ctrl_info->app_pic_data);
			}
			break;

		case DIVOOM_DISP_DRAW_CTRL_SIGNAL_COLOR:
			for (off = 0; off < max_dot_num; off++)
			{
				divoom_disp_ctrl_set_data(off, &color, pdisp_draw_ctrl_info->app_pic_data);
			}
			break;	

		case DIVOOM_DISP_DRAW_CTRL_AREA1:
			for (i = 0; i < max_dot_num; i++)
			{
				off = i;
				index = divoom_com_get_random(max_color_num);
				divoom_disp_ctrl_set_data(off, &draw_rainbow_color_table[index], pdisp_draw_ctrl_info->app_pic_data);
			}
			break;

		case DIVOOM_DISP_DRAW_CTRL_CYC_INSIDE:
		case DIVOOM_DISP_DRAW_CTRL_AREA3:
			index = divoom_com_get_random(max_color_num);
			memcpy(&color1, &draw_rainbow_color_table[index], sizeof(divoom_screen_color_info));

			index = divoom_com_get_random(max_color_num);
			memcpy(&color2, &draw_rainbow_color_table[index], sizeof(divoom_screen_color_info));
			for (i = 0; i < 5; i++)
			{
				if (i >1 && i < 4)
				{
					memcpy(&color, &color1, sizeof(divoom_screen_color_info));
				}
				else if (i == 4)
				{
					memcpy(&color, &color2, sizeof(divoom_screen_color_info));
				}
				off = draw_area2_table[i].main_pot;
				memcpy(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].windmill_color[i], &color, sizeof(divoom_screen_color_info));
				divoom_disp_ctrl_set_data(off, &color, pdisp_draw_ctrl_info->app_pic_data);
			}
			break;

		case DIVOOM_DISP_DRAW_CTRL_RED_GREEN_BLUE_WHITE_TEST:
			break;

		case DIVOOM_DISP_DRAW_CTRL_RUNNING_LIGHT_AWAY:
			break;

		case DIVOOM_DISP_DRAW_CTRL_PICKUP_LIGHT_SINGAL:
			break;
			
		case DIVOOM_DISP_DRAW_CTRL_PICKUP_LIGHT:
			break;

		case DIVOOM_DISP_DRAW_CTRL_METEOR_MULTICOLOR:
		case DIVOOM_DISP_DRAW_CTRL_METEOR:
			break;

		case DIVOOM_DISP_DRAW_CTRL_WAVE_FLOW:
			break;
	}

	pdisp_draw_ctrl_info->update_pic = 1;
}

/*********************************************************************
  Function:        divoom_disp_ambient_ctrl_set_pot_table
  Description:    	对图片进行点位设置
  Input:           	disp_mode 要设置的模式
  Return:           NONE
  Author:           wangft
  Data:            2019-6-19
**********************************************************************/
void divoom_disp_ambient_ctrl_set_pot_table(Pdivoom_disp_draw_app_data pdisp_draw_ctrl_info, uint8 disp_mode)
{
	if (pdivoom_disp_ambient_ctrl_handle->rgb_info.select_light_index == 0)
	{
		divoom_disp_ambient_ctrl_set_all_light_pot_table(pdisp_draw_ctrl_info, disp_mode);
		memcpy(pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[0].app_pic_data, pdisp_draw_ctrl_info->app_pic_data, TX_SCREEN_BUF_SIZE);
	}
}

/*********************************************************************
  Function:        divoom_disp_ambient_ctrl_get_pot_table
  Description:    	对app发送过来的图片进行取点
  Input:           	NONE
  Return:           1 需要回归初始状态 0 保持当前状态
  Author:           wangft
  Data:            2019-6-19
**********************************************************************/
uint8 divoom_disp_ambient_ctrl_get_pot_table(uint8 k)
{
	uint8 i = 0, dot_num = 0;
	divoom_screen_color_info color = {0};
	for (i = 0; i < TX_SCREEN_WIDTH; i++)
	{
		divoom_disp_ctrl_get_data(i, &color, pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].app_pic_data); //取每个点rgb值
		if (color.r_c == 0 && color.g_c == 0 && color.b_c == 0)
		{
			continue;
		}
		else
		{
			pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].off_table[dot_num] = i; //保存点的位置
			memcpy(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color[dot_num], &color, sizeof(divoom_screen_color_info)); //保存颜色
			dot_num++;
		}
	}	
	pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].dot_num = dot_num;
	return 1;
}

void divoom_disp_ambient_ctrl_fade_in_out_init(uint8 k)
{
	uint8 flag = 0, i = 0, num = 0;

	flag = divoom_disp_ambient_ctrl_get_pot_table(k);
	pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].fade_in_out_step_index = 0;
	pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].fade_in_out_index = 0;
	if (flag)
		pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].fade_in_out_index = DIVOOM_FADE_IN_OUT_START_LEVE;
	
	if (pdivoom_disp_ambient_ctrl_handle->color_cyc_flag)
	{
		pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color_index = divoom_com_get_random(RAINBOW_COLOR_NUM);
		for (i = 0; i < pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].dot_num; i++) //点数控制图片张数
			memcpy(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color[i], &draw_rainbow_color_table[pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color_index], sizeof(divoom_screen_color_info));
		num = sizeof(draw_rainbow_color_table)/sizeof(draw_rainbow_color_table[0]);
		pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color_index = (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color_index + 1)%num;
	}
}

void divoom_disp_ambient_ctrl_fade_in_out_main(uint8 k)
{
	uint8 i = 0, off = 0, num = 0;
	divoom_screen_color_info color = {0}, tcolor = {0};
	int index = 0, index2 = 0, quot = 0;
	int inter = FADE_IN_OUT_INDEX;
	
	index = pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].fade_in_out_index%inter;
	quot = pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].fade_in_out_index/inter;
	if (index == 0 && quot > 0)
		index = inter;
	memset(pdivoom_disp_ambient_ctrl_handle->disp_buf[k], 0, TX_SCREEN_BUF_SIZE);
	for (i = 0; i < pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].dot_num; i++) //点数控制图片张数
	{
		off = pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].off_table[i]; //得到位置
		memcpy(&color, &pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color[i], sizeof(divoom_screen_color_info)); //得到该位置颜色
		switch (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].fade_in_out_step_index)
		{
			case 0:
				index2 = index;
				divoom_rgb_hsv_change_brightness(&color, &tcolor, index2, FADE_IN_OUT_INDEX);
				divoom_disp_ctrl_set_data(off, &tcolor, pdivoom_disp_ambient_ctrl_handle->disp_buf[k]);
				break;
			case 1: //保持最高亮度持续时间
				index2 = inter;
				divoom_rgb_hsv_change_brightness(&color, &tcolor, index2, FADE_IN_OUT_INDEX);
				divoom_disp_ctrl_set_data(off, &tcolor, pdivoom_disp_ambient_ctrl_handle->disp_buf[k]);
				if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].fade_in_out_index > 1)
				{
					pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].fade_in_out_index = inter;
					quot = 1;
				}
				break;
			case 2:
				index2 = FADE_IN_OUT_INDEX + DIVOOM_FADE_IN_OUT_START_LEVE - index;
				divoom_rgb_hsv_change_brightness(&color, &tcolor, index2, FADE_IN_OUT_INDEX);
				divoom_disp_ctrl_set_data(off, &tcolor, pdivoom_disp_ambient_ctrl_handle->disp_buf[k]);
//				if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].fade_in_out_index > FADE_IN_OUT_INDEX - 30)
				if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].fade_in_out_index > FADE_IN_OUT_INDEX)
				{
					pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].fade_in_out_index = inter;
					quot = 1;
				}
				break;
		}
	}
	if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].fade_in_out_index%inter == 0 && quot > 0)
	{
		pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].fade_in_out_step_index++;
		if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].fade_in_out_step_index == 3)
			pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].fade_in_out_step_index = 0;
		pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].fade_in_out_index = DIVOOM_FADE_IN_OUT_START_LEVE;

		if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].fade_in_out_step_index == 0)
			pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].fade_in_out_index = 50;

		if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].fade_in_out_step_index == 0 && pdivoom_disp_ambient_ctrl_handle->color_cyc_flag)
		{
			for (i = 0; i < pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].dot_num; i++) //点数控制图片张数
				memcpy(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color[i], &draw_rainbow_color_table[pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color_index], sizeof(divoom_screen_color_info));
			num = sizeof(draw_rainbow_color_table)/sizeof(draw_rainbow_color_table[0]);
			pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color_index = (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color_index + 1)%num;
		}
	}
	pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].fade_in_out_index++;
}

T_BOOL divoom_disp_ambient_ctrl_blink_check_samepos(uint8 pos, uint8 k)
{
    uint8 i;
    for(i = 0; i < pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].star_blink_dot_num; i++)
    {
        if(pos == i)
            continue;
        if(pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[pos].index == pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[i].index)
            return AK_TRUE;
    }
    return AK_FALSE;
}

int divoom_ambient_ctrl_rainbow_slowing_change_color(Pdivoom_screen_color_info  color_src,    Pdivoom_screen_color_info color_dest, Pdivoom_screen_color_info color1, Pdivoom_screen_color_info color2)
{
	uint8 a = 0, r_flag = 0, b_flag = 0, g_flag = 0;
	uint8 inter = 5;
	
	if (color_src->r_c != color_dest->r_c)
	{
		if (color_src->r_c > color_dest->r_c)
		{
			a = color_src->r_c - color_dest->r_c;
			color1->r_c += inter;
			if (color1->r_c >= a)
			{
				color1->r_c = a;
				r_flag = 1;
			}
			color2->r_c = color_src->r_c - color1->r_c;
		}
		else 
		{
			a = color_dest->r_c - color_src->r_c;
			color1->r_c += inter;
			if (color1->r_c >= a)
			{
				color1->r_c = a;
				r_flag = 1;
			}
			color2->r_c = color_src->r_c + color1->r_c;
		}
	}
	else
	{
		color2->r_c = color_dest->r_c;
		r_flag = 1;
	}

	if (color_src->g_c != color_dest->g_c)
	{
		if (color_src->g_c > color_dest->g_c)
		{
			a = color_src->g_c - color_dest->g_c;
			color1->g_c += inter;
			if (color1->g_c >= a)
			{
				color1->g_c = a;
				g_flag = 1;
			}
			color2->g_c = color_src->g_c - color1->g_c;
		}
		else 
		{
			a = color_dest->g_c - color_src->g_c;
			color1->g_c += inter;
			if (color1->g_c >= a)
			{
				color1->g_c = a;
				g_flag = 1;
			}
			color2->g_c = color_src->g_c + color1->g_c;
		}
	}
	else
	{
		color2->g_c = color_dest->g_c;
		g_flag = 1;
	}

	if (color_src->b_c != color_dest->b_c)
	{
		if (color_src->b_c > color_dest->b_c)
		{
			a = color_src->b_c - color_dest->b_c;
			color1->b_c += inter;
			if (color1->b_c >= a)
			{
				color1->b_c = a;
				b_flag = 1;
			}
			color2->b_c = color_src->b_c - color1->b_c;
		}
		else 
		{
			a = color_dest->b_c - color_src->b_c;
			color1->b_c += inter;
			if (color1->b_c >= a)
			{
				color1->b_c = a;
				b_flag = 1;
			}
			color2->b_c = color_src->b_c + color1->b_c;
		}
	}
	else
	{
		color2->b_c = color_dest->b_c;
		b_flag = 1;
	}

	if (r_flag == 1 && g_flag == 1 && b_flag == 1)
	{
		return 1;		
	}

	return 0;
}

uint8 divoom_disp_ambient_ctrl_rainbow_get_color_index(Pdivoom_screen_color_info pcolor)
{
	uint8 r_c, g_c, b_c, i;
	r_c = pcolor->r_c;
	g_c = pcolor->g_c;
	b_c = pcolor->b_c;

	for (i = 0; i <= RAINBOW_COLOR_NUM2; i++)
	{
		if (r_c == draw_rainbow_color_table2[i].r_c && g_c == draw_rainbow_color_table2[i].g_c && b_c == draw_rainbow_color_table2[i].b_c)
			return i;			
	}

	return 0;
}

void divoom_disp_ambient_ctrl_rainbow_init(uint8 k)
{
	uint8 i = 0, off = 0;
	
	divoom_disp_ambient_ctrl_get_pot_table(k);
	//先标记所有点所在的颜色位置
	for (i = 0; i < pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].dot_num; i++)
	{
		off = pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].off_table[i];
		pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].index = off;
		memcpy(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].color, &pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color[i], sizeof(divoom_screen_color_info));
		memcpy(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].frist_color, &pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color[i], sizeof(divoom_screen_color_info));
		pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].cur_color_index = divoom_disp_ambient_ctrl_rainbow_get_color_index(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color[i]);
		memset(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].color1, 0, sizeof(divoom_screen_color_info));
		pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].next_color_index = pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].cur_color_index + 1;
		if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].next_color_index > RAINBOW_COLOR_NUM2)
			pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].next_color_index = 0;
		pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].hsv.h = 0;
	}
	pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_dot_num = pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].dot_num;
}

void divoom_disp_ambient_ctrl_rainbow_main(uint8 k)
{
	uint8 i = 0, off = 0;
	divoom_screen_color_info next_color = {0}, src_color = {0xff, 0x0, 0x0};
	int h_index = 0;

	memset(pdivoom_disp_ambient_ctrl_handle->disp_buf[k], 0, TX_SCREEN_BUF_SIZE);
	for (i = 0; i < pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_dot_num; i++)
	{
		h_index = pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].hsv.h;
		divoom_rgb_hsv_change_rgb(&src_color, &next_color, h_index, RGB_SATURA_VALUE, RGB_BRIGHT_VALUE);

		if (h_index >= H_MAX)
			pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].hsv.h = 0;
		
		pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].hsv.h++;
		off = pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].index;
		divoom_disp_ctrl_set_data(off, &next_color, pdivoom_disp_ambient_ctrl_handle->disp_buf[k]);
	}
}

uint8 divoom_disp_ambient_ctrl_rainbow_bot_get_color_index(Pdivoom_screen_color_info pcolor, uint8 index)
{
	uint8 r_c, g_c, b_c, i;
	r_c = pcolor->r_c;
	g_c = pcolor->g_c;
	b_c = pcolor->b_c;

	for (i = 0; i <= RAINBOW_COLOR_NUM; i++)
	{
		if (r_c == draw_rainbow_bot_color_table[index][i].r_c && g_c == draw_rainbow_bot_color_table[index][i].g_c && b_c == draw_rainbow_bot_color_table[index][i].b_c)
			return i;			
	}

	return 0;
}

uint8 divoom_disp_ambient_ctrl_flame_get_color_index(Pdivoom_screen_color_info pcolor)
{
	uint8 r_c, g_c, b_c, i;
	r_c = pcolor->r_c;
	g_c = pcolor->g_c;
	b_c = pcolor->b_c;

	for (i = 0; i <= FLAME_COLOR_NUM; i++)
	{
		if (r_c == draw_flame_color_table[i].r_c && g_c == draw_flame_color_table[i].g_c && b_c == draw_flame_color_table[i].b_c)
			return i;			
	}

	return 0;
}

void divoom_disp_ambient_ctrl_rainbow_bot_init(uint8 k)
{
	uint8 i = 0, off = 0;
	int h_index = 0;
	
	divoom_disp_ambient_ctrl_get_pot_table(k);
	//先标记所有点所在的颜色位置
	for (i = 0; i < pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].dot_num; i++)
	{
		off = pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].off_table[i];
		pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].index = off;
		pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].hsv.h = h_index%H_MAX;
		h_index += 100;
	}
	pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_dot_num = pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].dot_num;
}

void divoom_disp_ambient_ctrl_rainbow_bot_main(uint8 k)
{
	uint8 i = 0, off = 0;
	divoom_screen_color_info next_color = {0}, src_color = {0xff, 0x0, 0x0};
	int h_index = 0, h_inter = 2;

	memset(pdivoom_disp_ambient_ctrl_handle->disp_buf[k], 0, TX_SCREEN_BUF_SIZE);
	for (i = 0; i < pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_dot_num; i++)
	{
		h_index = pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].hsv.h;
		divoom_rgb_hsv_change_rgb(&src_color, &next_color, h_index, RGB_SATURA_VALUE0, RGB_BRIGHT_VALUE);
		pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].hsv.h += h_inter;

		if (h_index >= H_MAX)
			pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].hsv.h = 0;
		off = pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].index;
		divoom_disp_ctrl_set_data(off, &next_color, pdivoom_disp_ambient_ctrl_handle->disp_buf[k]);
	}
}

void divoom_disp_ambient_ctrl_flame_init(uint8 k)
{
	uint8 dot_num = 0, j = 0;
	divoom_disp_color_info tcolor = {0}, tcolor2 = {0}, color_yellow ={255, 130, 0},  color_red = {255, 70, 20};

	//第一步基本状态：
	dot_num = sizeof(draw_flow_reptile)/sizeof(draw_flow_reptile[0]);
	divoom_rgb_hsv_change_brightness(&color_red, &tcolor, RGB_BRIGHT_VALUE - 100, RGB_BRIGHT_VALUE);
	divoom_rgb_hsv_change_brightness(&color_yellow, &tcolor2, RGB_BRIGHT_VALUE, RGB_BRIGHT_VALUE);

	for (j = 0; j < dot_num; j++)	
	{
		pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].particles[j].index = draw_flow_reptile[j]; 
		memcpy(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].particles[j].color_src[0], &tcolor, sizeof(divoom_disp_color_info));
		memcpy(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].particles[j].color_src[1], &tcolor2, sizeof(divoom_disp_color_info));
		pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].particles[j].dis_flag = 0;
	}

	pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].particles[0].dis_flag = 1;
}

void divoom_disp_ambient_ctrl_flame_main(uint8 k)
{
	uint16 j = 0,  dot_num = 0, off = 0, ret = 0;
	divoom_disp_color_info *color, tcolor = {0}, tcolor2 = {0}, color_red = {255, 70, 20}, color_change[2];

	memset(pdivoom_disp_ambient_ctrl_handle->disp_buf[k], 0, TX_SCREEN_BUF_SIZE);
	dot_num = sizeof(draw_flow_reptile)/sizeof(draw_flow_reptile[0]);
	divoom_rgb_hsv_change_brightness(&color_red, &tcolor, RGB_BRIGHT_VALUE - 100, RGB_BRIGHT_VALUE);

	//显示
	for (j = 0; j < dot_num; j++)	
	{
		//设置基本状态颜色
		off = pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].particles[j].index; 
		if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].particles[j].dis_flag == 1)
		{
			//两种颜色过渡
			memcpy(color_change, pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].particles[j].color_src, 2*sizeof(divoom_screen_color_info));
			color = &pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].particles[j].color1;

			ret = divoom_ambient_ctrl_rainbow_slowing_change_color(&color_change[0], &color_change[1], color, &tcolor2);
			pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].particles[j].color_step++;

			if (ret)
			{
				memset(color, 0, sizeof(divoom_screen_color_info));
				memcpy(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].particles[j].color_src[0], &pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].particles[j].color_src[1], sizeof(divoom_screen_color_info));
				memcpy(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].particles[j].color_src[1], &color_change[0], sizeof(divoom_screen_color_info));

				if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].particles[j].color_status == 0)
				{
					pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].particles[j].color_status = 1;
				}
				else
				{
					pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].particles[j].color_status = 0;
					pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].particles[j].dis_flag = 0;
					pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].particles[j].color_step = 0;
				}
			}
			divoom_disp_ctrl_set_data(off, &tcolor2, pdivoom_disp_ambient_ctrl_handle->disp_buf[k]);
		}
		else
		{
			divoom_disp_ctrl_set_data(off, &tcolor, pdivoom_disp_ambient_ctrl_handle->disp_buf[k]);
		}
	}

	for (j = 0; j < dot_num; j++)	
	{
		if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].particles[j].dis_flag == 1)
		{
			if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].particles[j].color_step == 6)
			{
				if (j < dot_num -1)
				{
					pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].particles[j+1].dis_flag = 1;
					pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].particles[j+1].color_status = 0;
					pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].particles[j+1].color_step = 0;
				}
				else
				{
					pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].particles[0].dis_flag = 1;
					pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].particles[0].color_status = 0;
					pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].particles[0].color_step = 0;
				}
			}
		}
	}
}

#define COLUMN_HEIGHT 9  // 灯柱的高度
#define COLUMN_WIDTH 4    // 灯柱的宽度
#define FLAME_INTENSITY 30 // 火焰的最大强度
#define PARTICLES_NUM 36

void divoom_disp_ambient_ctrl_flame2_init(uint8 k)
{
	int i, j, off = 0, intensity, index = 0, light_ness = 0;
	divoom_screen_color_info color = {255, 100, 0}, color_yellow ={255, 130, 0}, color_red = {255, 70, 20}, tcolor;

	srand(time(NULL)); // 初始化随机数种子
	for (i = 0; i < COLUMN_HEIGHT; i++)
	{
		intensity = i*FLAME_INTENSITY;
		for (j = 0; j < COLUMN_WIDTH; j++) 
		{
			index = i*COLUMN_WIDTH + j;
			off = particle_dot_xy2[index];

			if (i < 3)
			{
				light_ness = RGB_BRIGHT_VALUE - intensity - 10;
				if (i == 0)
					light_ness += 10;
				divoom_rgb_hsv_change_brightness(&color_yellow, &tcolor, light_ness, RGB_BRIGHT_VALUE);
				memcpy(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].particles[index].color_src[0], &tcolor, sizeof(divoom_screen_color_info));
				divoom_rgb_hsv_change_brightness(&color_red, &tcolor, light_ness, RGB_BRIGHT_VALUE);
				memcpy(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].particles[index].color_src[1], &tcolor, sizeof(divoom_screen_color_info));
			}
			else if (i < 6)
				divoom_rgb_hsv_change_brightness(&color, &tcolor, RGB_BRIGHT_VALUE - intensity, RGB_BRIGHT_VALUE);
			else
				divoom_rgb_hsv_change_brightness(&color_red, &tcolor, RGB_BRIGHT_VALUE - intensity, RGB_BRIGHT_VALUE);
			
			divoom_disp_ctrl_set_data(off, &tcolor, pdivoom_disp_ambient_ctrl_handle->disp_buf[k]);
		}
	}

	for (i = 0; i < 5; i++)
	{
		intensity = i*FLAME_INTENSITY;
		light_ness = RGB_BRIGHT_VALUE - intensity - 10;
		index = PARTICLES_NUM + i;
		off = BACK_LIGHT_START_POT + i;
		divoom_rgb_hsv_change_brightness(&color_yellow, &tcolor, light_ness, RGB_BRIGHT_VALUE);
		memcpy(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].particles[index].color_src[0], &tcolor, sizeof(divoom_screen_color_info));
		divoom_rgb_hsv_change_brightness(&color_red, &tcolor, light_ness, RGB_BRIGHT_VALUE);
		memcpy(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].particles[index].color_src[1], &tcolor, sizeof(divoom_screen_color_info));
		divoom_disp_ctrl_set_data(off, &tcolor, pdivoom_disp_ambient_ctrl_handle->disp_buf[k]);
	}
}

void divoom_disp_ambient_ctrl_flame2_main(uint8 k)
{
	int i = 0, j = 0, intensity, off = 0, index = 0, ret;
	divoom_screen_color_info color = {255, 100, 0}, color_red = {255, 70, 20}, tcolor, color_change[2], *color1;

	memset(pdivoom_disp_ambient_ctrl_handle->disp_buf[k], 0, TX_SCREEN_BUF_SIZE);
	for (i = 0; i < COLUMN_HEIGHT; i++)
	{
		intensity = i*FLAME_INTENSITY;
		for (j = 0; j < COLUMN_WIDTH; j++) 
		{
			index = i*COLUMN_WIDTH + j;
			off = particle_dot_xy2[index];
			if (i < 3)
			{
				memcpy(color_change, pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].particles[index].color_src, 2*sizeof(divoom_screen_color_info));
				color1 = &pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].particles[index].color1;

				ret = divoom_ambient_ctrl_rainbow_slowing_change_color(&color_change[0], &color_change[1], color1, &tcolor);
				if (ret)
				{
					memset(color1, 0, sizeof(divoom_screen_color_info));
					memcpy(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].particles[index].color_src[0], &pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].particles[index].color_src[1], sizeof(divoom_screen_color_info));
					memcpy(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].particles[index].color_src[1], &color_change[0], sizeof(divoom_screen_color_info));
				}
			}
			else if (i < 6)
				divoom_rgb_hsv_change_brightness(&color, &tcolor, RGB_BRIGHT_VALUE - intensity - 10, RGB_BRIGHT_VALUE);
			else
				divoom_rgb_hsv_change_brightness(&color_red, &tcolor, RGB_BRIGHT_VALUE - intensity - 10, RGB_BRIGHT_VALUE);

			divoom_disp_ctrl_set_data(off, &tcolor, pdivoom_disp_ambient_ctrl_handle->disp_buf[k]);
		}
	}

	for (i = 0; i < 5; i++)
	{
		index = PARTICLES_NUM + i;
		off = BACK_LIGHT_START_POT + i;
		memcpy(color_change, pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].particles[index].color_src, 2*sizeof(divoom_screen_color_info));
		color1 = &pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].particles[index].color1;
		ret = divoom_ambient_ctrl_rainbow_slowing_change_color(&color_change[0], &color_change[1], color1, &tcolor);
		if (ret)
		{
			memset(color1, 0, sizeof(divoom_screen_color_info));
			memcpy(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].particles[index].color_src[0], &pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].particles[index].color_src[1], sizeof(divoom_screen_color_info));
			memcpy(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].particles[index].color_src[1], &color_change[0], sizeof(divoom_screen_color_info));
		}
		divoom_disp_ctrl_set_data(off, &tcolor, pdivoom_disp_ambient_ctrl_handle->disp_buf[k]);
	}
}


void divoom_disp_ambient_ctrl_flame3_init(uint8 k)
{
	uint8 i = 0;
	
	divoom_disp_ambient_ctrl_get_pot_table(k);
	for (i = 0; i < pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].dot_num; i++)
	{
		memcpy(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].color, &pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color[i], sizeof(divoom_screen_color_info));
		pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].cur_color_index = divoom_disp_ambient_ctrl_flame_get_color_index(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color[i]);
		memset(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].color1, 0, sizeof(divoom_screen_color_info));
		pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].next_color_index = pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].cur_color_index + 1;
		if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].next_color_index > FLAME_COLOR_NUM)
			pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].next_color_index = 0;
	}
	pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_dot_num = pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].dot_num;
}

void divoom_disp_ambient_ctrl_flame3_main(uint8 k)
{
	uint8 i = 0, j = 0, next_color_index = 0, a = 0, r_flag = 0, b_flag = 0, g_flag = 0, off = 0, color_inter = 1;
	divoom_screen_color_info next_color = {0};

	memset(pdivoom_disp_ambient_ctrl_handle->disp_buf[k], 0, TX_SCREEN_BUF_SIZE);
	pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].step_index++;
	for (i = 0; i < pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_dot_num; i++)
	{
		r_flag = b_flag = g_flag = 0;
		next_color_index = pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].next_color_index;
		memcpy(&next_color, &draw_flame_color_table[next_color_index], sizeof(divoom_screen_color_info));

		if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].flag)
			color_inter = 1;
		else
			color_inter = 1;

		if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].color.r_c != next_color.r_c)
		{
			if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].color.r_c > next_color.r_c)
			{
				a = pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].color.r_c - next_color.r_c;
				pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].color1.r_c += color_inter;
				if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].color1.r_c >= a)
				{
					pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].color1.r_c = a;
					r_flag = 1;
				}
				pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].color2.r_c = pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].color.r_c - pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].color1.r_c;
			}
			else 
			{
				a = next_color.r_c - pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].color.r_c;
				pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].color1.r_c += color_inter;
				if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].color1.r_c >= a)
				{
					pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].color1.r_c = a;
					r_flag = 1;
				}
				pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].color2.r_c = pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].color.r_c + pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].color1.r_c;
			}
		}
		else
		{
			pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].color2.r_c = next_color.r_c;
			r_flag = 1;
		}

		if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].color.g_c != next_color.g_c)
		{
			if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].color.g_c > next_color.g_c)
			{
				a = pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].color.g_c - next_color.g_c;
				pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].color1.g_c += color_inter;
				if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].color1.g_c >= a)
				{
					pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].color1.g_c = a;
					g_flag = 1;
				}
				pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].color2.g_c = pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].color.g_c - pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].color1.g_c;
			}
			else 
			{
				a = next_color.g_c - pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].color.g_c;
				pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].color1.g_c += color_inter;
				if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].color1.g_c >= a)
				{
					pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].color1.g_c = a;
					g_flag = 1;
				}
				pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].color2.g_c = pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].color.g_c + pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].color1.g_c;
			}
		}
		else
		{
			pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].color2.g_c = next_color.g_c;
			g_flag = 1;
		}

		if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].color.b_c != next_color.b_c)
		{
			if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].color.b_c > next_color.b_c)
			{
				a = pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].color.b_c - next_color.b_c;
				pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].color1.b_c += color_inter;
				if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].color1.b_c >= a)
				{
					pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].color1.b_c = a;
					b_flag = 1;
				}
				pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].color2.b_c = pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].color.b_c - pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].color1.b_c;
			}
			else 
			{
				a = next_color.b_c - pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].color.b_c;
				pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].color1.b_c += color_inter;
				if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].color1.b_c >= a)
				{
					pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].color1.b_c = a;
					b_flag = 1;
				}
				pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].color2.b_c = pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].color.b_c + pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].color1.b_c;
			}
		}
		else
		{
			pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].color2.b_c = next_color.b_c;
			b_flag = 1;
		}

		if (r_flag == 1 && g_flag == 1 && b_flag == 1)
		{
			//保存
			memcpy(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].color, &next_color, sizeof(divoom_screen_color_info));
			pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].cur_color_index = next_color_index;
			pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].next_color_index = pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].cur_color_index + 1;
			memset(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].color1, 0, sizeof(divoom_screen_color_info));
			if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].next_color_index > FLAME_COLOR_NUM)
			{
				pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].next_color_index = 0;
				pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].flag = 1;
			}
			else
			{
				pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].flag = 0;
			}
		}

		off = draw_flame_left_table[i].main_pot;
		divoom_disp_ctrl_set_data(off, &pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].color2, pdivoom_disp_ambient_ctrl_handle->disp_buf[k]);
		for (j = 0; j < draw_flame_left_table[i].sub_pot_num; j++)
		{
			off = draw_flame_left_table[i].sub_pot[j];
			divoom_disp_ctrl_set_data(off, &pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].color2, pdivoom_disp_ambient_ctrl_handle->disp_buf[k]);
		}

		off = draw_flame_right_table[i].main_pot;
		divoom_disp_ctrl_set_data(off, &pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].color2, pdivoom_disp_ambient_ctrl_handle->disp_buf[k]);
		for (j = 0; j < draw_flame_right_table[i].sub_pot_num; j++)
		{
			off = draw_flame_right_table[i].sub_pot[j];
			divoom_disp_ctrl_set_data(off, &pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainbow_info[i].color2, pdivoom_disp_ambient_ctrl_handle->disp_buf[k]);
		}
	}
}

void divoom_disp_ambient_ctrl_area3_init(uint8 k)
{
	int i = 0, off = 0;
	
	divoom_disp_ambient_ctrl_get_pot_table(k);
	for (i = 0; i < pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].dot_num; i++)
	{
		off = draw_area2_table[i].main_pot;
		pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table[i] = off;
	}
}

void divoom_disp_ambient_ctrl_area3_main(uint8 k)
{
	uint8 i = 0, off = 0, j = 0;

	memset(pdivoom_disp_ambient_ctrl_handle->disp_buf[k], 0, TX_SCREEN_BUF_SIZE);
	for (i = 0; i < pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].dot_num; i++)
	{
		off = pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table[i]; //得到位置
		for (j = 0; j < draw_area2_table[i].sub_pot_num; j++)
		{
			if (off == draw_area2_table[i].sub_pot[j])
				break;
		}
		j = (j + 1)%draw_area2_table[i].sub_pot_num;
		pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table[i] = draw_area2_table[i].sub_pot[j]; //保存下一个点
		divoom_disp_ctrl_set_data(off, &pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].windmill_color[i], pdivoom_disp_ambient_ctrl_handle->disp_buf[k]);
	}
}

void divoom_disp_ambient_ctrl_cyc_inside_init(uint8 k)
{
	int i = 0, off = 0;

	divoom_disp_ambient_ctrl_get_pot_table(k);
	for (i = 0; i < pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].dot_num; i++)
	{
		off = draw_area2_table[i].main_pot;
		pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table[i] = off;
	}
}

void divoom_disp_ambient_ctrl_cyc_inside_main(uint8 k)
{
	uint8 i = 0, off = 0, j = 0, num = 0;
	divoom_screen_color_info color = {0};

	memset(pdivoom_disp_ambient_ctrl_handle->disp_buf[k], 0, TX_SCREEN_BUF_SIZE);
	for (i = 0; i < pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].dot_num; i++)
	{
		off = pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table[i]; //得到位置
		for (j = 0; j < draw_area2_table[i].sub_pot_num; j++)
		{
			if (off == draw_area2_table[i].sub_pot[j])
				break;
		}

		if (pdivoom_disp_ambient_ctrl_handle->color_cyc_flag && j == 0 )
		{
			if (i < 3) //内圈变色
			{
				memcpy(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].windmill_color[i], &draw_rainbow_color_table[pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color_index], sizeof(divoom_screen_color_info));
				num = sizeof(draw_rainbow_color_table)/sizeof(draw_rainbow_color_table[0]);
				pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color_index = (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color_index + 1)%num;
			}
		}

		if (pdivoom_disp_ambient_ctrl_handle->multicolor_flag)
		{
			if (i < 3) //内圈变色
			{
				memcpy(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].windmill_color[i], &draw_rainbow_color_table[pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color_index], sizeof(divoom_screen_color_info));
				num = sizeof(draw_rainbow_color_table)/sizeof(draw_rainbow_color_table[0]);
				pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color_index = (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color_index + 1)%num;
			}
		}
		
		j = (j + 1)%draw_area2_table[i].sub_pot_num;
		pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table[i] = draw_area2_table[i].sub_pot[j]; //保存下一个点
		pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[off].flag = 1;
		
		memcpy(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[off].color, &pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].windmill_color[i], sizeof(divoom_screen_color_info));
		pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[off].light_index = WINDMILL_INDEX;
	}
	
	for (i = 0; i < TX_SCREEN_WIDTH; i++)
	{
		if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[i].flag == 1)
		{
//			divoom_rgb_hsv_change_brightness(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[i].color, &color, pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[i].light_index, WINDMILL_INDEX);
			divoom_rgb_hsv_change_rgb(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[i].color, &color, -1, RGB_SATURA_VALUE, pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[i].light_index);
			if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[i].light_index >= 10)
				pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[i].light_index -= 10;

			if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[i].light_index < 10)
				pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[i].flag = 0;
			divoom_disp_ctrl_set_data(i, &color, pdivoom_disp_ambient_ctrl_handle->disp_buf[k]);
		}
	}
}

void divoom_disp_ambient_ctrl_area1_init(uint8 k)
{
	uint8 j = 0, off = 0, i = 0;

	divoom_disp_ambient_ctrl_get_pot_table(k);

	pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].star_blink_dot_num = TX_SCREEN_LED_NUM;
	for (i = 0; i < TX_SCREEN_LED_NUM; i++)
	{
		pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[i].flag = 0XFF;
	}

	pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].area_index = AREA0;
	for (i = 0; i < 2; i++) //刚开始内部亮
	{
		off = draw_area_table[i].main_pot;
		memcpy(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[off].color, &pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color[off], sizeof(divoom_screen_color_info)); //保存每个点的颜色
		pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[off].flag = DIVOOM_DISP_DRAW_START_ADD_LIGHT;	
		pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[off].index = off;	//保存每个点的位置
		pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[off].light_index = 0;
	
		for (j = 0; j < draw_area_table[i].sub_pot_num; j++)
		{
			off = draw_area_table[i].sub_pot[j];
			memcpy(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[off].color, &pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color[off], sizeof(divoom_screen_color_info)); //保存每个点的颜色
			pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[off].flag = DIVOOM_DISP_DRAW_START_ADD_LIGHT;	
			pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[off].index = off;	//保存每个点的位置
			pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[off].light_index = 0;
		}
	}

	off = draw_area_table[5].main_pot;
	memcpy(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[off].color, &pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color[off], sizeof(divoom_screen_color_info)); //保存每个点的颜色
	pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[off].flag = DIVOOM_DISP_DRAW_START_ADD_LIGHT;	
	pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[off].index = off;	//保存每个点的位置
	pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[off].light_index = 0;

	for (j = 0; j < draw_area_table[5].sub_pot_num; j++)
	{
		off = draw_area_table[5].sub_pot[j];
		memcpy(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[off].color, &pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color[off], sizeof(divoom_screen_color_info)); //保存每个点的颜色
		pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[off].flag = DIVOOM_DISP_DRAW_START_ADD_LIGHT;	
		pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[off].index = off;	//保存每个点的位置
		pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[off].light_index = 0;
	}
}

void divoom_disp_ambient_ctrl_area1_main(uint8 k)
{
	uint8 i = 0, j = 0, off = 0, index = 0;
	divoom_screen_color_info color = {0};

	memset(pdivoom_disp_ambient_ctrl_handle->disp_buf[k], 0, TX_SCREEN_BUF_SIZE);
	//内部亮
	for (i = 0; i < pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].star_blink_dot_num; i++)
	{
		if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[i].flag == DIVOOM_DISP_DRAW_START_ADD_LIGHT) //变亮过程
		{
			if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[i].light_index < 0)
				pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[i].light_index = 0;
			//改变颜色亮度值				
			divoom_rgb_hsv_change_brightness(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[i].color, &color, pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[i].light_index, STARS_BLINK_INDEX);
			divoom_disp_ctrl_set_data(pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[i].index, &color, pdivoom_disp_ambient_ctrl_handle->disp_buf[k]);
			pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[i].light_index++;
			if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[i].light_index >= STARS_BLINK_INDEX)
			{
				pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[i].flag = DIVOOM_DISP_DRAW_START_REDUCE_LIGHT;
			}
		}
		else if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[i].flag == DIVOOM_DISP_DRAW_START_REDUCE_LIGHT)
		{
			pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[i].light_index--;
			if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[i].light_index < 0)
			{
				pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[i].flag = DIVOOM_DISP_DRAW_START_END;
				pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].area_update_flag = 1;
			}
			else
			{
				//改变颜色亮度值
				divoom_rgb_hsv_change_brightness(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[i].color, &color, pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[i].light_index, STARS_BLINK_INDEX);
				divoom_disp_ctrl_set_data(pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[i].index, &color, pdivoom_disp_ambient_ctrl_handle->disp_buf[k]);
			}
		}
	}

	if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].area_update_flag)
	{
		pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].area_update_flag = 0;
		index = (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].area_index + 1)%(AREAEND -1);
		pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].area_index = index;

		if (index == 0)
		{
			for (i = 0; i < 2; i++) //刚开始内部亮
			{
				off = draw_area_table[i].main_pot;
				memcpy(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[off].color, &pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color[off], sizeof(divoom_screen_color_info)); //保存每个点的颜色
				pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[off].flag = DIVOOM_DISP_DRAW_START_ADD_LIGHT;	
				pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[off].index = off;	//保存每个点的位置
				pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[off].light_index = 0;
			
				for (j = 0; j < draw_area_table[i].sub_pot_num; j++)
				{
					off = draw_area_table[i].sub_pot[j];
					memcpy(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[off].color, &pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color[off], sizeof(divoom_screen_color_info)); //保存每个点的颜色
					pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[off].flag = DIVOOM_DISP_DRAW_START_ADD_LIGHT;	
					pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[off].index = off;	//保存每个点的位置
					pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[off].light_index = 0;
				}
			}
		}
		else if (index == 1)
		{
			for (i = 2; i < 4; i++) //刚开始外部亮
			{
				off = draw_area_table[i].main_pot;
				memcpy(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[off].color, &pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color[off], sizeof(divoom_screen_color_info)); //保存每个点的颜色
				pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[off].flag = DIVOOM_DISP_DRAW_START_ADD_LIGHT;	
				pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[off].index = off;	//保存每个点的位置
				pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[off].light_index = 0;
			
				for (j = 0; j < draw_area_table[i].sub_pot_num; j++)
				{
					off = draw_area_table[i].sub_pot[j];
					memcpy(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[off].color, &pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color[off], sizeof(divoom_screen_color_info)); //保存每个点的颜色
					pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[off].flag = DIVOOM_DISP_DRAW_START_ADD_LIGHT;	
					pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[off].index = off;	//保存每个点的位置
					pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[off].light_index = 0;
				}
			}
		}
		else if (index == 2)
		{
			for (i = 4; i < 5; i++) //刚开始内部亮
			{
				off = draw_area_table[i].main_pot;
				memcpy(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[off].color, &pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color[off], sizeof(divoom_screen_color_info)); //保存每个点的颜色
				pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[off].flag = DIVOOM_DISP_DRAW_START_ADD_LIGHT;	
				pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[off].index = off;	//保存每个点的位置
				pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[off].light_index = 0;
			
				for (j = 0; j < draw_area_table[i].sub_pot_num; j++)
				{
					off = draw_area_table[i].sub_pot[j];
					memcpy(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[off].color, &pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color[off], sizeof(divoom_screen_color_info)); //保存每个点的颜色
					pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[off].flag = DIVOOM_DISP_DRAW_START_ADD_LIGHT;	
					pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[off].index = off;	//保存每个点的位置
					pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[off].light_index = 0;
				}
			}
		}

		off = draw_area_table[5].main_pot;
		memcpy(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[off].color, &pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color[off], sizeof(divoom_screen_color_info)); //保存每个点的颜色
		pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[off].flag = DIVOOM_DISP_DRAW_START_ADD_LIGHT;	
		pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[off].index = off;	//保存每个点的位置
		pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[off].light_index = 0;

		for (j = 0; j < draw_area_table[5].sub_pot_num; j++)
		{
			off = draw_area_table[5].sub_pot[j];
			memcpy(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[off].color, &pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color[off], sizeof(divoom_screen_color_info)); //保存每个点的颜色
			pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[off].flag = DIVOOM_DISP_DRAW_START_ADD_LIGHT;	
			pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[off].index = off;	//保存每个点的位置
			pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[off].light_index = 0;
		}
	}
}

void divoom_disp_ambient_ctrl_star_blink_init(uint8 k)
{
	uint8 j = 0, off = 0, start_num = 0;

	divoom_disp_ambient_ctrl_get_pot_table(k);
	start_num = pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].dot_num/2; 
	start_num += pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].dot_num%2;

	if (start_num < 4)
		start_num = 4;
	
	pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].star_blink_dot_num = start_num;
	for (j = 0; j < pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].star_blink_dot_num; j++) //先抽取部分点个数
	{
		off = divoom_com_get_random(pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].dot_num);
		memcpy(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[j].color, &pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color[off], sizeof(divoom_screen_color_info)); //保存每个点的颜色
		off = pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].off_table[off]; //得到位置
		pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[j].flag = DIVOOM_DISP_DRAW_START_HOLD;	
		pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[j].index = off; 	//保存每个点的位置
		pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[j].light_index = 0;
	}
}

void divoom_disp_ambient_ctrl_star_blink_main(uint8 k)
{
	uint8 i = 0, off = 0;
	divoom_screen_color_info color = {0};

	memset(pdivoom_disp_ambient_ctrl_handle->disp_buf[k], 0, TX_SCREEN_BUF_SIZE);
	for (i = 0; i < pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].star_blink_dot_num; i++)
	{
		if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[i].flag == DIVOOM_DISP_DRAW_START_ADD_LIGHT)//变亮过程
		{
			if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[i].light_index < 0)
				pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[i].light_index = 0;
			//改变颜色亮度值				
			divoom_rgb_hsv_change_brightness(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[i].color, &color, pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[i].light_index, STARS_BLINK_INDEX);
			divoom_disp_ctrl_set_data(pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[i].index, &color, pdivoom_disp_ambient_ctrl_handle->disp_buf[k]);
			pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[i].light_index++;
			if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[i].light_index >= STARS_BLINK_INDEX)
			{
				pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[i].flag = DIVOOM_DISP_DRAW_START_REDUCE_LIGHT;
			}
		}
		else if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[i].flag == DIVOOM_DISP_DRAW_START_REDUCE_LIGHT)
		{
			pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[i].light_index--;
			if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[i].light_index < 0)
			{
				pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[i].flag = DIVOOM_DISP_DRAW_START_END;
				pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[i].time_limite = 0;
			}
			else
			{
				//改变颜色亮度值
				divoom_rgb_hsv_change_brightness(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[i].color, &color, pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[i].light_index, STARS_BLINK_INDEX);
				divoom_disp_ctrl_set_data(pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[i].index, &color, pdivoom_disp_ambient_ctrl_handle->disp_buf[k]);
			}
		}
		else
		{
			if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[i].flag == DIVOOM_DISP_DRAW_START_END)
			{
				pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[i].time_limite++;
				if ((pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[i].time_limite % (1 + i*10)) == 0)
				{
					//随机选择某个点，但是这个不能与正在亮的点相同
					do
					{
						off = divoom_com_get_random(pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].dot_num);
						memcpy(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[i].color, &pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color[off], sizeof(divoom_screen_color_info)); //保存每个点的颜色
						off = pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].off_table[off]; //得到位置
						pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[i].flag = DIVOOM_DISP_DRAW_START_ADD_LIGHT;	
						pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[i].index = off; //保存每个点的位置
						pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[i].light_index = 0;
					}while (divoom_disp_ambient_ctrl_blink_check_samepos(i, k));
				}
			}
			else if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[i].flag == DIVOOM_DISP_DRAW_START_HOLD)
			{
				pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[i].time_limite++;
				if ((pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[i].time_limite % (1 + i*10)) == 0)
				{
					pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[i].flag = DIVOOM_DISP_DRAW_START_ADD_LIGHT;	
					pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].start_info[i].light_index = 0;
				}
			}
			else
			{
				pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].star_blink_dot_num--;
			}
		}
	}
}

void divoom_disp_ambient_ctrl_rainstorm_init(uint8 k)
{
	divoom_disp_ambient_ctrl_get_pot_table(k);
	memset(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainstorm_info, 0, sizeof(divoom_disp_draw_diffuse_ctrl_info));
	pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainstorm_info.main_pot_info[0].main_pot_dis_flag = 1;
	pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainstorm_info.main_pot_info[0].main_pot_dis_dir = 1;
	pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainstorm_info.main_pot_info[0].main_pot_dis_bright = 0;
	pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color_times = 0;
	pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color_index = divoom_com_get_random(RAINBOW_COLOR_NUM);
	memset(pdivoom_disp_ambient_ctrl_handle->disp_buf[k], 0, TX_SCREEN_BUF_SIZE);
}

void divoom_disp_ambient_ctrl_rainstorm_main(uint8 k)
{
	uint8 i = 0, j = 0, off = 0, m = 0, base_lightness = 15;
	divoom_screen_color_info color = {0}, src_color = {0xff, 0, 0};
	uint16 h_index = 0; 

	pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color_times++;
	if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color_times%50 == 0)
	{
		h_index = pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color_times%H_MAX;
		if (pdivoom_disp_ambient_ctrl_handle->color_cyc_flag)
		{
			divoom_rgb_hsv_change_rgb(&src_color, &pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color[0], h_index, RGB_SATURA_VALUE, RGB_BRIGHT_VALUE);
		}
	}

	for (i = 0; i < 3; i++)
	{
		if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainstorm_info.main_pot_info[i].main_pot_dis_flag)
		{
			if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainstorm_info.main_pot_info[i].main_pot_dis_dir == 1)
			{
				if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainstorm_info.main_pot_info[i].main_pot_dis_bright >= 255)
				{
					pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainstorm_info.main_pot_info[i].main_pot_dis_dir = 0;
					pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainstorm_info.main_pot_info[i].main_pot_dis_bright -= base_lightness;
				}
				else
				{
					pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainstorm_info.main_pot_info[i].main_pot_dis_bright += base_lightness;
					if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainstorm_info.main_pot_info[i].main_pot_dis_bright >= 90)
					{
						if (i < 2)
						{
							if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainstorm_info.main_pot_info[i+1].main_pot_dis_flag == 0)
							{
								pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainstorm_info.main_pot_info[i+1].main_pot_dis_flag = 1;
								pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainstorm_info.main_pot_info[i+1].main_pot_dis_bright = 0;
								pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainstorm_info.main_pot_info[i+1].main_pot_dis_dir = 1;
							}
						}
					}
				}
			}
			else if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainstorm_info.main_pot_info[i].main_pot_dis_dir == 0)
			{
				if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainstorm_info.main_pot_info[i].main_pot_dis_bright <= 2*base_lightness)
				{
					pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainstorm_info.main_pot_info[i].main_pot_dis_dir = 1;
					pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainstorm_info.main_pot_info[i].main_pot_dis_bright = 255;
				}
				else
				{
					pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainstorm_info.main_pot_info[i].main_pot_dis_bright -= base_lightness;
				}
			}

			divoom_rgb_hsv_change_brightness(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color[0], &color, pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].rainstorm_info.main_pot_info[i].main_pot_dis_bright, MIDDLE_WINDMILL_INDEX);

			if (i == 0 || i == 2)
			{
				for (j = 0; j < 9; j++)
				{
					off = draw_rainstorm_table[i][j].main_pot;
					divoom_disp_ctrl_set_data(off, &color, pdivoom_disp_ambient_ctrl_handle->disp_buf[k]);

					for (m = 0; m < draw_rainstorm_table[i][j].sub_pot_num; m++)
					{
						divoom_disp_ctrl_set_data(draw_rainstorm_table[i][j].sub_pot[m], &color, pdivoom_disp_ambient_ctrl_handle->disp_buf[k]);
					}
				}
			}
			else if (i == 1)
			{
				for (j = 0; j < 9; j++)
				{
					off = draw_rainstorm_table[i][j].main_pot;
					divoom_disp_ctrl_set_data(off, &color, pdivoom_disp_ambient_ctrl_handle->disp_buf[k]);

					for (m = 0; m < draw_rainstorm_table[i][j].sub_pot_num; m++)
					{
						divoom_disp_ctrl_set_data(draw_rainstorm_table[i][j].sub_pot[m], &color, pdivoom_disp_ambient_ctrl_handle->disp_buf[k]);
					}
				}
			}
		}
	}
}

void divoom_disp_ambient_ctrl_diffuse_init(uint8 k)
{
	divoom_disp_ambient_ctrl_get_pot_table(k);
	memset(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].diffuse_info, 0, sizeof(divoom_disp_draw_diffuse_ctrl_info));
	pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].diffuse_info.main_pot_info[0].main_pot_dis_flag = 1;
	pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].diffuse_info.main_pot_info[0].main_pot_dis_dir = 1;
	pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].diffuse_info.main_pot_info[0].main_pot_dis_bright = 0;
	pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color_index = divoom_com_get_random(RAINBOW_COLOR_NUM);
	memset(pdivoom_disp_ambient_ctrl_handle->disp_buf[k], 0, TX_SCREEN_BUF_SIZE);
}

int divoom_disp_ambient_ctrl_diffuse_check_bot(uint8 k)
{
	uint8 i = 0;
	
	for (i = 0; i < 24; i++)
	{
		if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].diffuse_info.main_pot_info[i].main_pot_dis_dir != 2)
			return 0;
	}
	return 1;
}

void divoom_disp_ambient_ctrl_diffuse_main(uint8 k)
{
	uint8 i = 0, j = 0, num=  0;
	divoom_screen_color_info color = {0};
	uint8 base_lightness = 15;

	pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].step_index++;
	if (divoom_disp_ambient_ctrl_diffuse_check_bot(k))
	{
		if (pdivoom_disp_ambient_ctrl_handle->color_cyc_flag)
		{
			memcpy(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color[0], &draw_rainbow_color_table[pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color_index], sizeof(divoom_screen_color_info));
			num = sizeof(draw_rainbow_color_table)/sizeof(draw_rainbow_color_table[0]);
			pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color_index = (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color_index + 1)%num;
		}

		for (i = 0; i < 24; i++)
		{
			pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].diffuse_info.main_pot_info[i].main_pot_dis_flag = 0;
		}
		
		pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].diffuse_info.main_pot_info[0].main_pot_dis_flag = 1;
		pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].diffuse_info.main_pot_info[0].main_pot_dis_dir = 1;
		pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].diffuse_info.main_pot_info[0].main_pot_dis_bright = 0;
	}

	for (i = 0; i < 24; i++)
	{
		if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].step_index%DIVOOM_DIFFUSE_STEP != 0)
			break;
		
		base_lightness = 15;
		if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].diffuse_info.main_pot_info[i].main_pot_dis_flag)
		{
			if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].diffuse_info.main_pot_info[i].main_pot_dis_dir == 1)
			{
				if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].diffuse_info.main_pot_info[i].main_pot_dis_bright >= 255)
				{
					pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].diffuse_info.main_pot_info[i].main_pot_dis_dir = 0;
					pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].diffuse_info.main_pot_info[i].main_pot_dis_bright -= base_lightness;
				}
				else
				{
					pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].diffuse_info.main_pot_info[i].main_pot_dis_bright += base_lightness;
					if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].diffuse_info.main_pot_info[i].main_pot_dis_bright >= 30)
					{
						if (i < 23)
						{
							if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].diffuse_info.main_pot_info[i+1].main_pot_dis_flag == 0)
							{
								pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].diffuse_info.main_pot_info[i+1].main_pot_dis_flag = 1;
								pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].diffuse_info.main_pot_info[i+1].main_pot_dis_bright = 0;
								pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].diffuse_info.main_pot_info[i+1].main_pot_dis_dir = 1;
							}
						}
					}
				}
			}
			else if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].diffuse_info.main_pot_info[i].main_pot_dis_dir == 0)
			{
				if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].diffuse_info.main_pot_info[i].main_pot_dis_bright >= base_lightness)
					pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].diffuse_info.main_pot_info[i].main_pot_dis_bright -= base_lightness;
				else
				{
					pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].diffuse_info.main_pot_info[i].main_pot_dis_bright = 0;
					pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].diffuse_info.main_pot_info[i].main_pot_dis_dir = 2;
				}
			}

			divoom_rgb_hsv_change_brightness(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color[0], &color, pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].diffuse_info.main_pot_info[i].main_pot_dis_bright, MIDDLE_WINDMILL_INDEX);
//			divoom_rgb_hsv_change_rgb(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color[0], &color, -1, RGB_SATURA_VALUE0, pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].diffuse_info.main_pot_info[i].main_pot_dis_bright);

			divoom_disp_ctrl_set_data(draw_diffuse_left_table[i].main_pot, &color, pdivoom_disp_ambient_ctrl_handle->disp_buf[k]);
			for (j = 0; j < draw_diffuse_left_table[i].sub_pot_num; j++)
			{
				divoom_disp_ctrl_set_data(draw_diffuse_left_table[i].sub_pot[j], &color, pdivoom_disp_ambient_ctrl_handle->disp_buf[k]);
			}
		}
	}
}

void divoom_disp_ambient_ctrl_middle_mindmill_init(uint8 k)
{
	uint8 i = 0, num = 0;
	
	divoom_disp_ambient_ctrl_get_pot_table(k);
	if (k == 1)
	{
		pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table[0] = 10;
		pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table[1] = 35;
	}
	else
	{
		pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table[0] = 19;
		pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table[1] = 44;
	}
	
	for (i = 0; i < 2; i++)
	{
		memset(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].middle_windmill_info[i], 0, sizeof(divoom_disp_draw_cool_windmill_info));
	}

	pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color_index = divoom_com_get_random(RAINBOW_COLOR_NUM);
	if (pdivoom_disp_ambient_ctrl_handle->color_cyc_flag)
	{
		for (i = 0; i < 2; i++)
			memcpy(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color[i], &draw_rainbow_color_table[pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color_index], sizeof(divoom_screen_color_info));
		num = sizeof(draw_rainbow_color_table)/sizeof(draw_rainbow_color_table[0]);
		pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color_index = (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color_index + 1)%num;
	}

	memset(pdivoom_disp_ambient_ctrl_handle->disp_buf[k], 0, TX_SCREEN_BUF_SIZE);
}

void divoom_disp_ambient_ctrl_middle_mindmill_main(uint8 k)
{
	uint8 i = 0, off = 0, j = 0, flag = 0, index = 0, num = 0, brightness = 1;
	divoom_screen_color_info color = {0};


	pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].step_index++;
	if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].step_index%10 == 0)
	{
		for (i = 0; i < 2; i++)
		{
			off = pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table[i]; //得到位置
			flag = 0;

			if (i == 0)
				index = 23;
			else
				index = 21;
			
			if (off == index)
			{
				if (pdivoom_disp_ambient_ctrl_handle->color_cyc_flag)
				{
					memcpy(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color[i], &draw_rainbow_color_table[pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color_index], sizeof(divoom_screen_color_info));
					if (i == 1)
					{
						num = sizeof(draw_rainbow_color_table)/sizeof(draw_rainbow_color_table[0]);
						pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color_index = (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color_index + 1)%num;
					}
				}

				for (j = 0; j < TX_SCREEN_WIDTH; j++)
				{
					if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].middle_windmill_info[i][j].flag == 1)
					{
						flag = 1;
						break;
					}
				}
			}

			if (flag)
				continue;

			if (k == 1)
			{
				if (i == 0)
				{
					if (off <= 10 && off > 0)
					{
						pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table[i] = (off - 1) % TX_SCREEN_WIDTH;
					}
					else if (off == 0)
					{
						pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table[i] = 20;
					}
					else if (off >=20 && off < 35)
					{
						pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table[i] = (off + 1) % TX_SCREEN_WIDTH;
					}
					else if (off == 35)
					{
						pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table[i] = 10;
					}
				}
				else
				{
					if (off <= 35 && off > 20)
					{
						pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table[i] = (off - 1) % TX_SCREEN_WIDTH;
					}
					else if (off == 20)
					{
						pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table[i] = 0;
					}
					else if (off < 10)
					{
						pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table[i] = (off + 1) % TX_SCREEN_WIDTH;
					}
					else if (off == 10)
					{
						pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table[i] = 35;
					}
				}
			}
			else
			{
				if (i == 0)
				{
					if (off <= 19 && off > 0)
					{
						pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table[i] = (off - 1) % TX_SCREEN_WIDTH;
					}
					else if (off == 0)
					{
						pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table[i] = 20;
					}
					else if (off >=20 && off < 44)
					{
						pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table[i] = (off + 1) % TX_SCREEN_WIDTH;
					}
					else if (off == 44)
					{
						pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table[i] = 19;
					}
				}
				else
				{
					if (off <= 44 && off > 20)
					{
						pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table[i] = (off - 1) % TX_SCREEN_WIDTH;
					}
					else if (off == 20)
					{
						pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table[i] = 0;
					}
					else if (off < 19)
					{
						pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table[i] = (off + 1) % TX_SCREEN_WIDTH;
					}
					else if (off == 19)
					{
						pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table[i] = 44;
					}
				}

			}
			pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].middle_windmill_info[i][off].flag = 1;
			memcpy(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].middle_windmill_info[i][off].color, &pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color[i], sizeof(divoom_screen_color_info));
			pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].middle_windmill_info[i][off].light_index = MIDDLE_WINDMILL_INDEX;
		}	
	}

	for (i = 0; i < 2; i++)
	{
		for (off = 0; off < TX_SCREEN_WIDTH; off++)
		{
			if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].middle_windmill_info[i][off].flag == 1)
			{
//				divoom_rgb_hsv_change_brightness(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].middle_windmill_info[i][off].color, &color, pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].middle_windmill_info[i][off].light_index, MIDDLE_WINDMILL_INDEX);
				divoom_rgb_hsv_change_rgb(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].middle_windmill_info[i][off].color, &color, -1, RGB_SATURA_VALUE, pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].middle_windmill_info[i][off].light_index);

				if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].middle_windmill_info[i][off].light_index >= brightness)
					pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].middle_windmill_info[i][off].light_index -= brightness;
				else
					pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].middle_windmill_info[i][off].flag = 0;
				
				divoom_disp_ctrl_set_data(off, &color, pdivoom_disp_ambient_ctrl_handle->disp_buf[k]);
			}
		}
	}
}

void divoom_disp_ambient_ctrl_cool_windmill_init(uint8 k)
{
	uint8 i = 0, off = 0;
	
	divoom_disp_ambient_ctrl_get_pot_table(k);
	memset(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info, 0, sizeof(pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info));
	for (i = 0; i < pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].dot_num; i++)
	{
		off = pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].off_table[i]; 
		pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table[i] = off;
		pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table2[i] = off;
		memcpy(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].windmill_color[i], &pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color[i], sizeof(divoom_screen_color_info));
	}
	pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].windmill_flag = 0;
	pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].windmill_dot_num = pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].dot_num;
	pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color_index = divoom_com_get_random(RAINBOW_COLOR_NUM);
	memset(pdivoom_disp_ambient_ctrl_handle->disp_buf[k], 0, TX_SCREEN_BUF_SIZE);
}


int divoom_disp_ambient_ctrl_cool_windmill_check_dot(uint8 k)
{
	uint8 i = 0;

	for (i = 0; i < TX_SCREEN_WIDTH; i++)
	{
		if (i >= BACK_LIGHT_START_POT && i <= BACK_LIGHT_END_POT)
		{
//			continue;
		}
		if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[i].flag == 1)
			return 1;
	}

	return 0;
}

void divoom_disp_ambient_ctrl_cool_windmill_main(uint8 k)
{
	uint8 i = 0, off = 0, off1 = 0, num = 0, inter_value = 1;
	divoom_screen_color_info color = {0};

	pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].step_index++;
//	memset(pdivoom_disp_ambient_ctrl_handle->disp_buf[k], 0, TX_SCREEN_BUF_SIZE);
	for (i = 0; i < pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].windmill_dot_num; i++) 
	{
		off1 = pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table[i]; //得到位置
		if ((off1 == TX_SCREEN_WIDTH) && divoom_disp_ambient_ctrl_cool_windmill_check_dot(k))
			break;

		if (off1 == TX_SCREEN_WIDTH)
			off1 = 0;

		off = flow_dot_table[off1];
		if ((pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].step_index % DIVOOM_COOL_WINDMINL_STEP) != 0)
			continue;

		pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table[i] = (off1 + 1) % (TX_SCREEN_WIDTH + 1);
		if (k == 1 && pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table[i] > BACK_LIGHT_END_POT)
		{
			pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table[i] = BACK_LIGHT_START_POT;
		}

		if (k == 0 && off >= BACK_LIGHT_START_POT && off <= BACK_LIGHT_END_POT)
		{
			if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[off].flag == 1)
				continue;
		}

		if (pdivoom_disp_ambient_ctrl_handle->color_cyc_flag)
		{
			if (off1== 0)
			{
				memcpy(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].windmill_color[i], &draw_rainbow_color_table[pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color_index], sizeof(divoom_screen_color_info));
				num = sizeof(draw_rainbow_color_table)/sizeof(draw_rainbow_color_table[0]);
				pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color_index = (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color_index + 1)%num;
			}
		}

		if (pdivoom_disp_ambient_ctrl_handle->multicolor_flag)
		{
			memcpy(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].windmill_color[i], &draw_rainbow_color_table[pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color_index], sizeof(divoom_screen_color_info));
			num = sizeof(draw_rainbow_color_table)/sizeof(draw_rainbow_color_table[0]);
			pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color_index = (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color_index + 1)%num;
		}
		
		memcpy(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[off].color, &pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].windmill_color[i], sizeof(divoom_screen_color_info));
		if (k == 0 && off >= BACK_LIGHT_START_POT && off <= BACK_LIGHT_END_POT)
		{
			if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[off].flag == 0)
			{
				pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[off].flag = 1;
				pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[off].light_dir = 0;
				pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[off].light_index = 0;
			}
		}
		else
		{
			pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[off].flag = 1;
			pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[off].light_index = WINDMILL_INDEX;
			pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[off].light_dir = 1;
		}
	}
	
	if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].windmill_dot_num)
	{
		for (i = 0; i < TX_SCREEN_WIDTH; i++)
		{
			if (k == 0 && i >= BACK_LIGHT_START_POT && i <= BACK_LIGHT_END_POT)
				inter_value = 1;
			else
				inter_value = 10;

			if ((pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].step_index % DIVOOM_COOL_WINDMINL_STEP) != 0)
			{
				if (i < BACK_LIGHT_START_POT || i > BACK_LIGHT_END_POT)
					continue;
			}
			
			if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[i].flag == 1)
			{
				if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[i].light_dir == 1)
				{
//					divoom_rgb_hsv_change_brightness(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[i].color, &color, pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[i].light_index, WINDMILL_INDEX);
					divoom_rgb_hsv_change_rgb(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[i].color, &color, -1, RGB_SATURA_VALUE, pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[i].light_index);
					if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[i].light_index >= inter_value)
						pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[i].light_index -= inter_value;
					else
					{
						if (k == 0 && i >= BACK_LIGHT_START_POT && i <= BACK_LIGHT_END_POT)
						{
							pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[i].flag = 0;
							pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[i].light_dir = 0;
							pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[i].light_index = 1;
							memcpy(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[i].color, &pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].windmill_color[0], sizeof(divoom_screen_color_info));
						}
						else
							pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[i].flag = 0;
					}

					divoom_disp_ctrl_set_data(i, &color, pdivoom_disp_ambient_ctrl_handle->disp_buf[k]);
				}
				else
				{
					divoom_rgb_hsv_change_rgb(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[i].color, &color, -1, RGB_SATURA_VALUE, pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[i].light_index);
					pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[i].light_index += inter_value;
					if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[i].light_index > WINDMILL_INDEX)
					{
						pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[i].light_index = WINDMILL_INDEX;
						pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[i].light_dir = 1;
					}
					divoom_disp_ctrl_set_data(i, &color, pdivoom_disp_ambient_ctrl_handle->disp_buf[k]);
				}
			}
		}
	}
}

void divoom_disp_ambient_ctrl_cool_windmill2_main(uint8 k)
{
	uint8 i = 0, off = 0, off1 = 0, num = 0, inter_value = 1;
	divoom_screen_color_info color = {0};

//	memset(pdivoom_disp_ambient_ctrl_handle->disp_buf[k], 0, TX_SCREEN_BUF_SIZE);
	pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].step_index++;
	for (i = 0; i < pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].windmill_dot_num; i++) //点数控制图片张数
	{
		if ((pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].step_index % DIVOOM_COOL_WINDMINL_STEP) != 0)
			continue;
	
		off1 = pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table[i]; //得到位置
		pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table[i] = (off1 + 1) % TX_SCREEN_WIDTH;
		if (k == 1 && pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table[i] > BACK_LIGHT_END_POT)
		{
			pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table[i] = BACK_LIGHT_START_POT;
		}
		off = flow_dot_table[off1];

		if (pdivoom_disp_ambient_ctrl_handle->color_cyc_flag)
		{
			if (off1== 0 ||(k == 1 && off1 == BACK_LIGHT_START_POT))
			{
				memcpy(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].windmill_color[i], &draw_rainbow_color_table[pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color_index], sizeof(divoom_screen_color_info));
				num = sizeof(draw_rainbow_color_table)/sizeof(draw_rainbow_color_table[0]);
				pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color_index = (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color_index + 1)%num;
			}
		}

		if (pdivoom_disp_ambient_ctrl_handle->multicolor_flag)
		{
			memcpy(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].windmill_color[i], &draw_rainbow_color_table[pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color_index], sizeof(divoom_screen_color_info));
			num = sizeof(draw_rainbow_color_table)/sizeof(draw_rainbow_color_table[0]);
			pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color_index = (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color_index + 1)%num;
		}

		if (k == 0 && off >= BACK_LIGHT_START_POT && off <= BACK_LIGHT_END_POT)
		{
			if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[off].flag == 0)
			{
				pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[off].flag = 1;
				pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[off].light_dir = 0;
				pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[off].light_index = 0;
				memcpy(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[off].color, &pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].windmill_color[i], sizeof(divoom_screen_color_info));
			}
		}
		else
		{
			pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[off].light_dir = 1;
			pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[off].flag = 1;
			memcpy(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[off].color, &pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].windmill_color[i], sizeof(divoom_screen_color_info));
			pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[off].light_index = WINDMILL_INDEX;
		}
	}
	
	if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].windmill_dot_num)
	{
		for (i = 0; i < TX_SCREEN_WIDTH; i++)
		{
			if (k == 0 && i >= BACK_LIGHT_START_POT && i <= BACK_LIGHT_END_POT)
				inter_value = 1;
			else
				inter_value = 10;

			if ((pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].step_index % DIVOOM_COOL_WINDMINL_STEP) != 0)
			{
				if (i < BACK_LIGHT_START_POT || i > BACK_LIGHT_END_POT)
					continue;
			}
		
			if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[i].flag == 1)
			{
				if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[i].light_dir == 1)
				{
					divoom_rgb_hsv_change_brightness(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[i].color, &color, pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[i].light_index, WINDMILL_INDEX);
//					divoom_rgb_hsv_change_rgb(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[i].color, &color, -1, RGB_SATURA_VALUE, pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[i].light_index);
					if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[i].light_index >= inter_value)
					{
						pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[i].light_index -= inter_value;
					}
					else
					{
						if (i >= BACK_LIGHT_START_POT && i <= BACK_LIGHT_END_POT)
						{
							pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[i].flag = 1;
							pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[i].light_dir = 0;
							pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[i].light_index = 0;
//							memcpy(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[i].color, &pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].windmill_color[i], sizeof(divoom_screen_color_info));
						}
					}
					divoom_disp_ctrl_set_data(i, &color, pdivoom_disp_ambient_ctrl_handle->disp_buf[k]);
				}
				else
				{
//					divoom_rgb_hsv_change_rgb(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[i].color, &color, -1, RGB_SATURA_VALUE, pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[i].light_index);
					divoom_rgb_hsv_change_brightness(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[i].color, &color, pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[i].light_index, WINDMILL_INDEX);
					pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[i].light_index += inter_value;
					if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[i].light_index > WINDMILL_INDEX)
					{
						pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[i].light_index = WINDMILL_INDEX;
						pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[i].light_dir = 1;
					}
					divoom_disp_ctrl_set_data(i, &color, pdivoom_disp_ambient_ctrl_handle->disp_buf[k]);
				}
			}
		}
	}

	if ((pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].step_index % DIVOOM_COOL_WINDMINL_STEP) != 0)
		return;

	for (i = 0; i < pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].windmill_dot_num; i++) //点数控制图片张数
	{
		off1 = pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table2[i]; //得到位置
		pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table2[i] = (off1 + 1) % TX_SCREEN_WIDTH;
		if (k == 1 && pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table2[i] > BACK_LIGHT_END_POT)
		{
			pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table2[i] = BACK_LIGHT_START_POT;
		}
		off = flow_dot_table2[off1];

		if (k == 0)
		{
			if (off >= BACK_LIGHT_START_POT && off <= BACK_LIGHT_END_POT)
			{
				break;
			}
		}
		
		pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[off].flag = 1;
		pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[off].light_dir = 1;
		memcpy(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[off].color, &pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].windmill_color[i], sizeof(divoom_screen_color_info));
		pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[off].light_index = WINDMILL_INDEX;
	}
	
	if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].windmill_dot_num)
	{
		for (i = 0; i < TX_SCREEN_WIDTH; i++)
		{
			if (k == 0)
			{
				if (i >= BACK_LIGHT_START_POT && i <= BACK_LIGHT_END_POT)
				{
					break;
				}
			}
		
			if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[i].flag == 1)
			{
				divoom_rgb_hsv_change_brightness(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[i].color, &color, pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[i].light_index, WINDMILL_INDEX);
//				divoom_rgb_hsv_change_rgb(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[i].color, &color, -1, RGB_SATURA_VALUE, pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[i].light_index);
				if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[i].light_index >= inter_value)
					pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[i].light_index -= inter_value;
				divoom_disp_ctrl_set_data(i, &color, pdivoom_disp_ambient_ctrl_handle->disp_buf[k]);
			}
		}
	}
}

void divoom_disp_ambient_ctrl_cool_windmill3_main(uint8 k)
{
	uint8 i = 0, off = 0, off1 = 0, num = 0;
	divoom_screen_color_info color = {0};

	memset(pdivoom_disp_ambient_ctrl_handle->disp_buf[k], 0, TX_SCREEN_BUF_SIZE);
	if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].windmill_flag == 0)
	{
		for (i = 0; i < pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].windmill_dot_num; i++) //点数控制图片张数
		{
			off1 = pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table[i]; //得到位置
			pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table[i] = (off1 + 1) % TX_SCREEN_WIDTH;
			if (k == 1)
			{
				if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table[i] > BACK_LIGHT_END_POT)
				{
					pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].windmill_flag = 1;
					pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table2[i] = BACK_LIGHT_START_POT;
				}
			}
			else
			{
				if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table[i] == 44)
				{
					pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].windmill_flag = 1;
					pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table2[i] = 0;
				}
			}
			off = flow_dot_table[off1];

			if (pdivoom_disp_ambient_ctrl_handle->color_cyc_flag)
			{
				if (off1== 0)
				{
					memcpy(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].windmill_color[i], &draw_rainbow_color_table[pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color_index], sizeof(divoom_screen_color_info));
					num = sizeof(draw_rainbow_color_table)/sizeof(draw_rainbow_color_table[0]);
					pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color_index = (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color_index + 1)%num;
				}
			}

			if (pdivoom_disp_ambient_ctrl_handle->multicolor_flag)
			{
				memcpy(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].windmill_color[i], &draw_rainbow_color_table[pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color_index], sizeof(divoom_screen_color_info));
				num = sizeof(draw_rainbow_color_table)/sizeof(draw_rainbow_color_table[0]);
				pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color_index = (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color_index + 1)%num;
			}
			
			pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[off].flag = 1;
			memcpy(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[off].color, &pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].windmill_color[i], sizeof(divoom_screen_color_info));
			pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[off].light_index = WINDMILL_INDEX;
		}
	}
	else 
	{
		for (i = 0; i < pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].windmill_dot_num; i++) //点数控制图片张数
		{
			off1 = pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table2[i]; //得到位置
			pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table2[i] = (off1 + 1) % TX_SCREEN_WIDTH;
			if (k == 1)
			{
				if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table2[i] > BACK_LIGHT_END_POT)
				{
					pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].windmill_flag = 0;
					pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table2[i] = BACK_LIGHT_START_POT;
				}
			}
			else
			{
				if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table2[i] == 44)
				{
					pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].windmill_flag = 0;
					pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table[i] = 0;
				}
			}
			off = flow_dot_table2[off1];

			if (pdivoom_disp_ambient_ctrl_handle->color_cyc_flag)
			{
				if (off1== 0)
				{
					memcpy(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].windmill_color[i], &draw_rainbow_color_table[pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color_index], sizeof(divoom_screen_color_info));
					num = sizeof(draw_rainbow_color_table)/sizeof(draw_rainbow_color_table[0]);
					pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color_index = (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color_index + 1)%num;
				}
			}

			if (pdivoom_disp_ambient_ctrl_handle->multicolor_flag)
			{
				memcpy(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].windmill_color[i], &draw_rainbow_color_table[pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color_index], sizeof(divoom_screen_color_info));
				num = sizeof(draw_rainbow_color_table)/sizeof(draw_rainbow_color_table[0]);
				pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color_index = (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color_index + 1)%num;
			}
			
			pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[off].flag = 1;
			memcpy(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[off].color, &pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].windmill_color[i], sizeof(divoom_screen_color_info));
			pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[off].light_index = WINDMILL_INDEX;
		}
	}
	
	if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].windmill_dot_num)
	{
		for (i = 0; i < TX_SCREEN_WIDTH; i++)
		{
			if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[i].flag == 1)
			{
				divoom_rgb_hsv_change_brightness(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[i].color, &color, pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[i].light_index, WINDMILL_INDEX);
				if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[i].light_index >= 10)
					pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info[i].light_index -= 10;
				divoom_disp_ctrl_set_data(i, &color, pdivoom_disp_ambient_ctrl_handle->disp_buf[k]);
			}
		}
	}
}

void divoom_disp_ambient_ctrl_flow_windmill_init(uint8 k)
{
	uint8 i = 0, off = 0;
	
	divoom_disp_ambient_ctrl_get_pot_table(k);
	memset(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info, 0, sizeof(pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].cool_windmill_info));
	for (i = 0; i < pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].dot_num; i++)
	{
		off = pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].off_table[i]; 
		pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table[i] = off;
		memcpy(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].windmill_color[i], &pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color[i], sizeof(divoom_screen_color_info));
	}
	if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].dot_num == 0)
	{
		pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table[0] = 0;
	}
	pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].windmill_dot_num = pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].dot_num;
}

void divoom_disp_ambient_ctrl_windmill_init(uint8 k)
{
	uint8 i = 0, off = 0;
	
	divoom_disp_ambient_ctrl_get_pot_table(k);
	for (i = 0; i < pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].dot_num; i++)
	{
		off = pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].off_table[i]; 
		pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table[i] = off;
		pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table2[i] = off;
		memcpy(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].windmill_color[i], &pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color[i], sizeof(divoom_screen_color_info));
		memcpy(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].windmill3_info[i].color, &pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color[i], sizeof(divoom_screen_color_info));
		memset(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].windmill3_info[i].color1, 0 , sizeof(divoom_screen_color_info));
	}
	pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].windmill_dot_num = pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].dot_num;
	pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].windmill_flag = 0;
}

void divoom_disp_ambient_ctrl_windmill_main(uint8 k)
{
	uint8 i = 0, off = 0, off1 = 0;
	divoom_screen_color_info color = {0};
	
	memset(pdivoom_disp_ambient_ctrl_handle->disp_buf[k], 0, TX_SCREEN_BUF_SIZE);
	for (i = 0; i < pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].windmill_dot_num; i++) //点数控制图片张数
	{
		off = pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table[i]; //得到当前要走的位置
		memcpy(&color, &pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].windmill_color[i], sizeof(divoom_screen_color_info)); //得到该位置颜色

		//搞一个映射表
		off1 = flow_dot_table[off];
		divoom_disp_ctrl_set_data(off1, &color, pdivoom_disp_ambient_ctrl_handle->disp_buf[k]);
		if (k == 1)
		{
			pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table[i] = (off + 1) % TX_SCREEN_WIDTH;
			if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table[i] > BACK_LIGHT_END_POT)
				pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table[i] = BACK_LIGHT_START_POT;
		}
		else
		{
			if (off1 < TX_SCREEN_WIDTH)
			{
				pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table[i] = (off + 1) % TX_SCREEN_WIDTH;
				
				if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table[i] == 19)
				{
					pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table[i] = 20;
				}
				if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table[i] == 44)
				{
					pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table[i] = 0;
				}
			}
		}
	}
}

void divoom_disp_ambient_ctrl_windmill2_main(uint8 k)
{
	uint8 i = 0, off = 0, off1 = 0, num = 0, ret = 0;
	divoom_screen_color_info *color_src, color_dest = {0}, *color1, *color2;
	
	memset(pdivoom_disp_ambient_ctrl_handle->disp_buf[k], 0, TX_SCREEN_BUF_SIZE);
	for (i = 0; i < pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].windmill_dot_num; i++) //点数控制图片张数
	{
		off = pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table[i]; //得到当前要走的位置
		color_src = &pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].windmill3_info[i].color;
		color1 = &pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].windmill3_info[i].color1;
		color2 = &pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].windmill3_info[i].color2;

		if (pdivoom_disp_ambient_ctrl_handle->color_cyc_flag)
		{
			memcpy(&color_dest, &draw_rainbow_color_table[pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color_index], sizeof(divoom_screen_color_info));
			ret = divoom_ambient_ctrl_rainbow_slowing_change_color(color_src, &color_dest, color1, color2);
			if (ret == 1)
			{
				num = sizeof(draw_rainbow_color_table)/sizeof(draw_rainbow_color_table[0]);
				memcpy(color_src, &color_dest, sizeof(divoom_screen_color_info));
				memset(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].windmill3_info[i].color1, 0, sizeof(divoom_screen_color_info));
				pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color_index = (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color_index + 1)%num;
			}
		}
		else
		{
			color2 = color_src;
		}
		
		//搞一个映射表
		off1 = flow_dot_table[off];
		divoom_disp_ctrl_set_data(off1, color2, pdivoom_disp_ambient_ctrl_handle->disp_buf[k]);

		if (k == 1)
		{
			pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table[i] = (off + 1) % TX_SCREEN_WIDTH;
			if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table[i] > BACK_LIGHT_END_POT)
				pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table[i] = BACK_LIGHT_START_POT;
		}
		else
		{
			if (off1 < TX_SCREEN_WIDTH)
			{
				pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table[i] = (off + 1) % TX_SCREEN_WIDTH;
				if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table[i] == 19)
				{
					pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table[i] = 20;
				}
				if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table[i] == 44)
				{
					pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table[i] = 0;
				}
			}
		}
	
		off = pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table2[i]; //得到当前要走的位置
		if (k == 1)
		{
			pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table2[i] = (off + 1) % TX_SCREEN_WIDTH;
			if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table2[i] > BACK_LIGHT_END_POT)
			{
				pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table2[i] = BACK_LIGHT_START_POT;
			}

			off1 = flow_dot_table2[off];
			divoom_disp_ctrl_set_data(off1, color2, pdivoom_disp_ambient_ctrl_handle->disp_buf[k]);
		}
		else
		{
			//搞一个映射表
			if (off == 0)
				off = 1;
			else if (off == 25)
				off = 26;
			
			off1 = flow_dot_table2[off];
			divoom_disp_ctrl_set_data(off1, color2, pdivoom_disp_ambient_ctrl_handle->disp_buf[k]);
			if (off1 < TX_SCREEN_WIDTH)
			{
				pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table2[i] = (off + 1) % TX_SCREEN_WIDTH;
			}
		}
	}
}

void divoom_disp_ambient_ctrl_windmill3_main(uint8 k)
{
	uint8 i = 0, off = 0, off1 = 0, num = 0, ret  = 0;
	divoom_screen_color_info *color_src, color_dest = {0}, *color1, *color2;
	
	pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].step_index++;
	if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].step_index % 7 != 0)
		return;

	memset(pdivoom_disp_ambient_ctrl_handle->disp_buf[k], 0, TX_SCREEN_BUF_SIZE);
	if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].windmill_flag == 0)
	{
		for (i = 0; i < pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].windmill_dot_num; i++) //点数控制图片张数
		{
			off = pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table[i]; //得到当前要走的位置
			color_src = &pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].windmill3_info[i].color;
			color1 = &pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].windmill3_info[i].color1;
			color2 = &pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].windmill3_info[i].color2;
			if (pdivoom_disp_ambient_ctrl_handle->color_cyc_flag)
			{
				memcpy(&color_dest, &draw_rainbow_color_table[pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color_index], sizeof(divoom_screen_color_info));
				ret = divoom_ambient_ctrl_rainbow_slowing_change_color(color_src, &color_dest, color1, color2);
				if (ret == 1)
				{
					num = sizeof(draw_rainbow_color_table)/sizeof(draw_rainbow_color_table[0]);
					memcpy(color_src, &color_dest, sizeof(divoom_screen_color_info));
					memset(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].windmill3_info[i].color1, 0, sizeof(divoom_screen_color_info));
					pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color_index = (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color_index + 1)%num;
				}
			}
			else
			{
				color2 = color_src;
			}
		
			//搞一个映射表
			off1 = flow_dot_table[off];
			divoom_disp_ctrl_set_data(off1, color2, pdivoom_disp_ambient_ctrl_handle->disp_buf[k]);
			if (k == 1)
			{
				pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table[i] = (off + 1) % TX_SCREEN_WIDTH;
				if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table[i] > BACK_LIGHT_END_POT)
				{
					pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table[i] = BACK_LIGHT_START_POT;
					pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].windmill_flag = 1;
				}
			}
			else
			{
				if (off1 < TX_SCREEN_WIDTH)
				{
					pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table[i] = (off + 1) % TX_SCREEN_WIDTH;
					
					if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table[i] == 19)
					{
						pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table[i] = 20;
					}
					if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table[i] == 44)
					{
						pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table[i] = 0;
						pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].windmill_flag = 1;
					}
				}
			}
		}
	}
	else
	{
		for (i = 0; i < pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].windmill_dot_num; i++) //点数控制图片张数
		{
			off = pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table[i]; //得到当前要走的位置
			color_src = &pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].windmill3_info[i].color;
			color1 = &pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].windmill3_info[i].color1;
			color2 = &pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].windmill3_info[i].color2;
			if (pdivoom_disp_ambient_ctrl_handle->color_cyc_flag)
			{
				memcpy(&color_dest, &draw_rainbow_color_table[pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color_index], sizeof(divoom_screen_color_info));
				ret = divoom_ambient_ctrl_rainbow_slowing_change_color(color_src, &color_dest, color1, color2);

				if (ret == 1)
				{
					num = sizeof(draw_rainbow_color_table)/sizeof(draw_rainbow_color_table[0]);
					memcpy(color_src, &color_dest, sizeof(divoom_screen_color_info));
					memset(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].windmill3_info[i].color1, 0, sizeof(divoom_screen_color_info));
					pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color_index = (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color_index + 1)%num;
				}
			}
			else
			{
				color2 = color_src;
			}

			if (k == 1)
			{
				pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table[i] = (off + 1) % TX_SCREEN_WIDTH;
				if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table[i] > BACK_LIGHT_END_POT)
				{
					pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table[i] = BACK_LIGHT_START_POT;
					pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].windmill_flag = 0;
				}
	
				off1 = flow_dot_table2[off];
				divoom_disp_ctrl_set_data(off1, color2, pdivoom_disp_ambient_ctrl_handle->disp_buf[k]);
			}
			else
			{
				//搞一个映射表
				if (off == 0)
					off = 1;
				else if (off == 25)
					off = 26;

				if (off == 44)
					pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].windmill_flag = 0;

				off1 = flow_dot_table2[off];
				divoom_disp_ctrl_set_data(off1, color2, pdivoom_disp_ambient_ctrl_handle->disp_buf[k]);
				if (off1 < TX_SCREEN_WIDTH)
				{
					pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].next_off_table[i] = (off + 1) % TX_SCREEN_WIDTH;
				}
			}
		}
	}
}

void divoom_disp_ambient_ctrl_running_light_away_init(uint8 k)
{
	int i = 0, dot_num = 0, h_index = 0;
	divoom_screen_color_info tcolor, src_color = {0xff, 0, 0};
	dot_num = sizeof(run_away_bot_table)/sizeof(run_away_bot_table[0]);

	for (i = 0; i < dot_num; i++) //生成颜色表
	{
		h_index = i*64;
		divoom_rgb_hsv_change_rgb(&src_color, &tcolor, h_index, RGB_SATURA_VALUE, -1);
		draw_run_away_color_table2[i].r_c = tcolor.r_c;
		draw_run_away_color_table2[i].g_c = tcolor.g_c;
		draw_run_away_color_table2[i].b_c = tcolor.b_c;
	}
	memset(pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].run_away_info.color_table, 0xff, sizeof(pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].run_away_info.color_table));
	pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].run_away_info.color_table[0] = 0;
	memset(pdivoom_disp_ambient_ctrl_handle->disp_buf[k], 0, TX_SCREEN_BUF_SIZE);
}

void divoom_disp_ambient_ctrl_running_light_away_main(uint8 k)
{
	int i = 0, j = 0, off = 0, index = 0, dot_num = 0, color_num = 0;
	uint8 temp_color_table[TX_SCREEN_WIDTH];
	divoom_screen_color_info tcolor;

	memset(temp_color_table, 0xff, sizeof(temp_color_table));
	dot_num = sizeof(run_away_bot_table)/sizeof(run_away_bot_table[0]);
	color_num = sizeof(draw_run_away_color_table2)/sizeof(draw_run_away_color_table2[0]);
	
	pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].step_index++;
	if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].step_index%5 == 0)
	{
		//显示
		for (i = 0; i < dot_num; i++)
		{
			index = pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].run_away_info.color_table[i];
			if (index != 0xff)
			{
				off = run_away_bot_table[i].main_pot;
				divoom_rgb_hsv_change_rgb(&draw_run_away_color_table2[index], &tcolor, -1, RGB_SATURA_VALUE, -1);
				divoom_disp_ctrl_set_data(off, &tcolor, pdivoom_disp_ambient_ctrl_handle->disp_buf[k]);
				for (j = 0; j < run_away_bot_table[i].sub_pot_num; j++)	
				{
					off = run_away_bot_table[i].sub_pot[j];
					divoom_disp_ctrl_set_data(off, &tcolor, pdivoom_disp_ambient_ctrl_handle->disp_buf[k]);
				}
			}
		}

		//位移
		for (i = dot_num - 1; i >= 0; i--)
		{
			index = pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].run_away_info.color_table[i];
			if (i == dot_num - 1)
			{
				temp_color_table[0] = index;
				if (index == 0xff)
				{
					temp_color_table[0] = (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].run_away_info.color_table[0]+ 1)%color_num;
				}
			}
			else
			{
				temp_color_table[i + 1] = index;
			}
		}
		memcpy(pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].run_away_info.color_table, temp_color_table, sizeof(temp_color_table));
	}
}

void divoom_disp_draw_ctrl_flow_reptile_init(uint8 k)
{
	uint8 off = 0;
	
	divoom_disp_ambient_ctrl_get_pot_table(k);
	off = flow_reptile[0];
	pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].draw_flow_reptile_table.flag = 1;
	pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].draw_flow_reptile_table.dot_info[0].dot = off;
	pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].draw_flow_reptile_table.dot_info[0].flag = 1;
	pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].draw_flow_reptile_table.dot_info[0].index = 0; //在轨迹数据表里面的位置
	pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].draw_flow_reptile_table.dot_info[0].light_index = 0;
	pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].draw_flow_reptile_table.dot_info[0].step_index = 0;
	pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].draw_flow_reptile_table.dot_info[0].wait_flag = 1;
	memcpy(&pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].draw_flow_reptile_table.color, &pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].color[0], sizeof(divoom_disp_color_info));
	pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].draw_flow_reptile_table.color_index = 0;
}

void divoom_disp_draw_ctrl_flow_reptile_main(uint8 k)
{
	uint16 j = 0,  dot_num = 0, off = 0, index = 0, dot_index = 0, head_index = 0, h_index = 0;
	divoom_disp_color_info color = {0}, tcolor = {0}, src_color = {0xff, 0x0, 0x0};
	T_FLOAT total_inter = 0.0;

	total_inter = FLOW_REPTILE_INDEX;
	memset(pdivoom_disp_ambient_ctrl_handle->disp_buf[k], 0, TX_SCREEN_BUF_SIZE);
	dot_num = 5;

	//检查一下所有灯
	for (j = 0; j < 5; j++)
	{
		if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].draw_flow_reptile_table.dot_info[j].wait_flag == 1) 
			break;
	}

	if (j == 5) //从头开始
	{
		off = flow_reptile[0];
		pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].draw_flow_reptile_table.flag = 1;
		pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].draw_flow_reptile_table.dot_info[0].dot = off;
		pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].draw_flow_reptile_table.dot_info[0].flag = 1;
		pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].draw_flow_reptile_table.dot_info[0].index = 0; //在轨迹数据表里面的位置
		pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].draw_flow_reptile_table.dot_info[0].light_index = 0;
		pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].draw_flow_reptile_table.dot_info[0].step_index = 0;
		pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].draw_flow_reptile_table.dot_info[0].wait_flag = 1;
		pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].draw_flow_reptile_table.color_index += 50;

		if (pdivoom_disp_ambient_ctrl_handle->color_cyc_flag)
		{
			for (j = 0; j < 5; j++)
			{
				h_index = pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].draw_flow_reptile_table.color_index%H_MAX;
				divoom_rgb_hsv_change_rgb(&src_color, &pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].draw_flow_reptile_table.color[j], h_index, RGB_SATURA_VALUE, RGB_BRIGHT_VALUE);
			}
		}
	}
	
	//让这条轨迹爬起来
	for (j = 0; j < 5; j++)
	{
		if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].draw_flow_reptile_table.dot_info[j].flag == 1) //表示这个点能显示
		{
			memcpy(&color, &pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].draw_flow_reptile_table.color[0], sizeof(divoom_disp_color_info));
			off = pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].draw_flow_reptile_table.dot_info[j].dot;
			if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].draw_flow_reptile_table.dot_info[j].wait_flag == 1) 
			{
				pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].draw_flow_reptile_table.dot_info[j].light_index++;
				switch (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].draw_flow_reptile_table.dot_info[j].step_index)
				{
					case 0:
						index = pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].draw_flow_reptile_table.dot_info[j].light_index;
						divoom_rgb_hsv_change_brightness(&color, &tcolor, index, total_inter);
						divoom_disp_ctrl_set_data(off, &tcolor, pdivoom_disp_ambient_ctrl_handle->disp_buf[k]);
						if (index >= DIVOOM_DISP_FLOW_REPTILE_INTER)
						{
							pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].draw_flow_reptile_table.dot_info[j].step_index = 1;
							head_index = pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].draw_flow_reptile_table.dot_info[j].index; //当前头点在轨迹数据表里面的下标
							head_index = (head_index + 1)%dot_num; //下一个头点在轨迹数据表里面的位置

							dot_index = (j+1)%5; //当前爬虫头点的位置的下一个位置
							if (head_index == 0) //表示从头开始了
							{
								//等待
								break;
							}

							if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].draw_flow_reptile_table.dot_info[dot_index].wait_flag == 1)//正在显示
							{
								pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].draw_flow_reptile_table.dot_info[j].step_index = 0;
								break;
							}
							else 
							{
								pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].draw_flow_reptile_table.dot_info[dot_index].wait_flag = 1;
							}
							
							off = flow_reptile[head_index]; 
							pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].draw_flow_reptile_table.dot_info[dot_index].dot = off; //从数据表里面找到相应的点
							pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].draw_flow_reptile_table.dot_info[dot_index].flag = 1;
							pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].draw_flow_reptile_table.dot_info[dot_index].index = head_index;
							pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].draw_flow_reptile_table.dot_info[dot_index].light_index = 0;
							pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].draw_flow_reptile_table.dot_info[dot_index].step_index = 0;
						}
						break;
						
					case 1:
						index = pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].draw_flow_reptile_table.dot_info[j].light_index;
						divoom_rgb_hsv_change_brightness(&color, &tcolor, index, total_inter);
						divoom_disp_ctrl_set_data(off, &tcolor, pdivoom_disp_ambient_ctrl_handle->disp_buf[k]);
						if (index >= FLOW_REPTILE_INDEX)
						{
							pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].draw_flow_reptile_table.dot_info[j].step_index = 2;
							pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].draw_flow_reptile_table.dot_info[j].light_index = 0;
						}
						break;
						
					case 2:
						index = pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].draw_flow_reptile_table.dot_info[j].light_index; 
						index = total_inter - index; //亮度变化 255~0
						divoom_rgb_hsv_change_brightness(&color, &tcolor, index, total_inter);
						divoom_disp_ctrl_set_data(off, &tcolor, pdivoom_disp_ambient_ctrl_handle->disp_buf[k]);
						if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].draw_flow_reptile_table.dot_info[j].light_index >= total_inter)
						{
							pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].draw_flow_reptile_table.dot_info[j].wait_flag = 0; //等待
						}
						break;
				}
			}
		}
	}
}

void divoom_disp_ambient_ctrl_default_init(uint8 k)
{
	divoom_disp_ambient_ctrl_get_pot_table(k);
}

void divoom_disp_ambient_ctrl_default_main(uint8 k)
{
	memcpy(pdivoom_disp_ambient_ctrl_handle->disp_buf[k], pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].app_pic_data, sizeof(pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].app_pic_data));
}

void divoom_disp_ambient_ctrl_signal_color_cyc_init(uint8 k)
{
	
}

void divoom_disp_ambient_ctrl_signal_color_cyc_main(uint8 k)
{
	static int index = 0;
	uint8 i = 0;

	for (i = 0; i < TX_SCREEN_LED_NUM; i++)
	{
		divoom_screen_set_data(i, &draw_signal_color_table[index], pdivoom_disp_ambient_ctrl_handle->disp_buf[k]);
	}

	index++;
	if (index >= sizeof(draw_signal_color_table)/sizeof(draw_signal_color_table[0]))
	{
		index = 0;
	}
}

void divoom_disp_ambient_ctrl_pickup_light_signal_init(uint8 k)
{
	//分配颜色
	int i = 0, dot_num = 0, h_index = 0;
	divoom_screen_color_info tcolor, src_color = {0xff, 0, 0};
	src_color.r_c = pdivoom_disp_ambient_ctrl_handle->rgb_info.color[0];
	src_color.g_c = pdivoom_disp_ambient_ctrl_handle->rgb_info.color[1];
	src_color.b_c = pdivoom_disp_ambient_ctrl_handle->rgb_info.color[2];
	dot_num = sizeof(draw_diffuse_left_table)/sizeof(draw_diffuse_left_table[0]);

	for (i = 0; i < dot_num; i++) //生成颜色表
	{
		//h_index = i*64;
		//divoom_rgb_hsv_change_rgb(&src_color, &tcolor, h_index, RGB_SATURA_VALUE, -1);
		draw_run_away_color_table2[i].r_c = src_color.r_c;
		draw_run_away_color_table2[i].g_c = src_color.g_c;
		draw_run_away_color_table2[i].b_c = src_color.b_c;
	}
	memset(pdivoom_disp_ambient_ctrl_handle->disp_buf[k], 0, TX_SCREEN_BUF_SIZE);
}


void divoom_disp_ambient_ctrl_pickup_light_init(uint8 k)
{
	//分配颜色
	int i = 0, dot_num = 0, h_index = 0;
	divoom_screen_color_info tcolor, src_color = {0xff, 0, 0};
	dot_num = sizeof(draw_diffuse_left_table)/sizeof(draw_diffuse_left_table[0]);

	for (i = 0; i < dot_num; i++) //生成颜色表
	{
		h_index = i*64;
		divoom_rgb_hsv_change_rgb(&src_color, &tcolor, h_index, RGB_SATURA_VALUE, -1);
		draw_run_away_color_table2[i].r_c = tcolor.r_c;
		draw_run_away_color_table2[i].g_c = tcolor.g_c;
		draw_run_away_color_table2[i].b_c = tcolor.b_c;
	}
	memset(pdivoom_disp_ambient_ctrl_handle->disp_buf[k], 0, TX_SCREEN_BUF_SIZE);
}

void divoom_disp_ambient_ctrl_pickup_light_main(uint8 k)
{
	int i, j, pole_high = 0, base_h = 5;

	pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].pickup_light_info.step_index++;
	if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].pickup_light_info.step_index%2 == 0 && pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].pickup_light_info.up_flag == 0)
	{
		if (pdivoom_disp_ambient_ctrl_handle->music_power >= pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].pickup_light_info.last_top)
		{
			pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].pickup_light_info.last_top = pdivoom_disp_ambient_ctrl_handle->music_power;
			pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].pickup_light_info.up_flag = 1;
		}
		else
		{
			if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].pickup_light_info.down_pos >= 1)
				pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].pickup_light_info.down_pos -= 1; //下落速度1格/次
			if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].pickup_light_info.down_pos == 0)
				pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].pickup_light_info.last_top = 0;
		}
	}

	memset(pdivoom_disp_ambient_ctrl_handle->disp_buf[k], 0, TX_SCREEN_BUF_SIZE);
	if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].pickup_light_info.up_flag) //上升阶段
	{
		pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].pickup_light_info.down_pos++;
		if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].pickup_light_info.down_pos > pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].pickup_light_info.last_top)
		{
			pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].pickup_light_info.up_flag = 0;
			pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].pickup_light_info.down_pos = pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].pickup_light_info.last_top;
		}
	}

	if (pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].pickup_light_info.down_pos < base_h)
		pole_high = base_h;
	else
		pole_high = pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[k].pickup_light_info.down_pos;
	
	for (i = 0; i < pole_high; i++)
	{
		divoom_disp_ctrl_set_data(draw_diffuse_left_table[i].main_pot, &draw_run_away_color_table2[i], pdivoom_disp_ambient_ctrl_handle->disp_buf[k]);
		for (j = 0; j < draw_diffuse_left_table[i].sub_pot_num; j++)
		{
			divoom_disp_ctrl_set_data(draw_diffuse_left_table[i].sub_pot[j], &draw_run_away_color_table2[i], pdivoom_disp_ambient_ctrl_handle->disp_buf[k]);
		}	
	}
}

void divoom_disp_ambient_ctrl_disp_all_light(uint8 disp_mode)
{
	uint8 k = 0, i = 0;
	divoom_led_control_led_info pled_info[TX_SCREEN_WIDTH];

	if (pdivoom_disp_ambient_ctrl_handle->color_cyc_flag == 0)
		disp_mode = disp_mode + DIVOOM_DISP_DRAW_CTRL_MULTICOLOR_END + 1;

	switch (disp_mode)
	{
		case DIVOOM_DISP_DRAW_CTRL_STARS_BREATHE_SIGNAL:
		case DIVOOM_DISP_DRAW_CTRL_STARS_BREATHE:
			divoom_disp_ambient_ctrl_star_blink_main(k);
			break;

		case DIVOOM_DISP_DRAW_CTRL_COOL_WINDMILL_MULTICOLOR:
		case DIVOOM_DISP_DRAW_CTRL_COOL_WINDMILL:
			divoom_disp_ambient_ctrl_cool_windmill_main(k);
			break;

		case DIVOOM_DISP_DRAW_CTRL_COOL_WINDMILL2:
		case DIVOOM_DISP_DRAW_CTRL_COOL_WINDMILL2_MULTICOLOR:
			divoom_disp_ambient_ctrl_cool_windmill2_main(k);
			break;

		case DIVOOM_DISP_DRAW_CTRL_COOL_WINDMILL3:
		case DIVOOM_DISP_DRAW_CTRL_COOL_WINDMILL3_MULTICOLOR:
			divoom_disp_ambient_ctrl_cool_windmill3_main(k);
			break;
		
		case DIVOOM_DISP_DRAW_CTRL_PICKUP_LIGHT_SINGAL:
		case DIVOOM_DISP_DRAW_CTRL_PICKUP_LIGHT:
			divoom_disp_ambient_ctrl_pickup_light_main(k);
			break;

		case DIVOOM_DISP_DRAW_CTRL_RAINBOW:
			divoom_disp_ambient_ctrl_rainbow_main(k);
			break;

		case DIVOOM_DISP_DRAW_CTRL_RAINBOW_BOT:
			divoom_disp_ambient_ctrl_rainbow_bot_main(k);
			break;

		case DIVOOM_DISP_DRAW_CTRL_FADE_IN_OUT:
			divoom_disp_ambient_ctrl_fade_in_out_main(k);
			break;

		case DIVOOM_DISP_DRAW_CTRL_SIGNAL_COLOR:
			divoom_disp_ambient_ctrl_default_main(k);
			break;	

		case DIVOOM_DISP_DRAW_CTRL_RED_GREEN_BLUE_WHITE_TEST:
			divoom_disp_ambient_ctrl_signal_color_cyc_main(k);
			break;

		case DIVOOM_DISP_DRAW_CTRL_MIDDLE_WINDMILL:
			divoom_disp_ambient_ctrl_middle_mindmill_main(k);
			break;

		case DIVOOM_DISP_DRAW_CTRL_DIFFUSE:
			divoom_disp_ambient_ctrl_diffuse_main(k);
			break;

		case DIVOOM_DISP_DRAW_CTRL_RAINSTORM:
			divoom_disp_ambient_ctrl_rainstorm_main(k);
			break;

		case DIVOOM_DISP_DRAW_CTRL_FORWARD_WINDMILL:
			divoom_disp_ambient_ctrl_windmill_main(k);
			break;

		case DIVOOM_DISP_DRAW_CTRL_FORWARD_WINDMILL2:
			divoom_disp_ambient_ctrl_windmill2_main(k);
			break;

		case DIVOOM_DISP_DRAW_CTRL_FORWARD_WINDMILL3:
			divoom_disp_ambient_ctrl_windmill3_main(k);
			break;

		case DIVOOM_DISP_DRAW_CTRL_AREA1:
			divoom_disp_ambient_ctrl_area1_main(k);
			break;

		case DIVOOM_DISP_DRAW_CTRL_CYC_INSIDE:
			divoom_disp_ambient_ctrl_cyc_inside_main(k);
			break;

		case DIVOOM_DISP_DRAW_CTRL_AREA3:
			divoom_disp_ambient_ctrl_area3_main(k);
			break;

		case DIVOOM_DISP_DRAW_CTRL_FLAME:
			divoom_disp_ambient_ctrl_flame_main(k);
			break;

		case DIVOOM_DISP_DRAW_CTRL_FLAME2:
			divoom_disp_ambient_ctrl_flame2_main(k);
			break;

		case DIVOOM_DISP_DRAW_CTRL_RUNNING_LIGHT_AWAY:
			divoom_disp_ambient_ctrl_running_light_away_main(k);
			break;

		case DIVOOM_DISP_DRAW_CTRL_METEOR:
			divoom_disp_ambient_ctrl_meteor_main(k);
			break;

		case DIVOOM_DISP_DRAW_CTRL_METEOR_MULTICOLOR:
			divoom_disp_ambient_ctrl_meteor_multicolor_main(k);
			break;

		case DIVOOM_DISP_DRAW_CTRL_WAVE_FLOW:
			divoom_disp_ambient_ctrl_wave_flow_main(k);
			break;
	}

	if (pdivoom_disp_ambient_ctrl_handle->rgb_info.select_light_index == 0)
	{
		divoom_disp_ambient_ctrl_send_rgb_data(pdivoom_disp_ambient_ctrl_handle->disp_buf[k]);
	}
}

int divoom_disp_ambient_ctrl_get_all_light_speed(uint8 disp_mode, int flag)
{
	uint8 k = 0;

	if (pdivoom_disp_ambient_ctrl_handle->color_cyc_flag == 0)
		disp_mode = disp_mode + DIVOOM_DISP_DRAW_CTRL_MULTICOLOR_END + 1;

	switch (disp_mode)
	{
		case DIVOOM_DISP_DRAW_CTRL_STARS_BREATHE_SIGNAL:
		case DIVOOM_DISP_DRAW_CTRL_STARS_BREATHE:
			if (flag == 1)
				divoom_disp_ambient_ctrl_star_blink_init(k);
			return DIVOOM_DISP_DRAW_CTRL_STARS_BREATHE_TIME;

		case DIVOOM_DISP_DRAW_CTRL_COOL_WINDMILL:
		case DIVOOM_DISP_DRAW_CTRL_COOL_WINDMILL_MULTICOLOR:
		case DIVOOM_DISP_DRAW_CTRL_COOL_WINDMILL2:
		case DIVOOM_DISP_DRAW_CTRL_COOL_WINDMILL3:
		case DIVOOM_DISP_DRAW_CTRL_COOL_WINDMILL3_MULTICOLOR:
		case DIVOOM_DISP_DRAW_CTRL_COOL_WINDMILL2_MULTICOLOR:
			if (flag == 1)
				divoom_disp_ambient_ctrl_cool_windmill_init(k);
			return DIVOOM_DISP_DRAW_CTRL_COOL_WINDMILL_TIME;

		case DIVOOM_DISP_DRAW_CTRL_RAINBOW:
			if (flag == 1)
				divoom_disp_ambient_ctrl_rainbow_init(k);
			return DIVOOM_DISP_DRAW_CTRL_RAINBOW_TIME;

		case DIVOOM_DISP_DRAW_CTRL_RAINBOW_BOT:
			if (flag == 1)
				divoom_disp_ambient_ctrl_rainbow_bot_init(k);
			return DIVOOM_DISP_DRAW_CTRL_RAINBOW_TIME;

		case DIVOOM_DISP_DRAW_CTRL_FORWARD_WINDMILL:
		case DIVOOM_DISP_DRAW_CTRL_FORWARD_WINDMILL2:
		case DIVOOM_DISP_DRAW_CTRL_FORWARD_WINDMILL3:
			if (flag == 1)
				divoom_disp_ambient_ctrl_windmill_init(k);
			return DIVOOM_DISP_DRAW_CTRL_WINDMILL_TIME;
			
		case DIVOOM_DISP_DRAW_CTRL_PICKUP_LIGHT:
			if (flag == 1)			
				divoom_disp_ambient_ctrl_pickup_light_init(k);
			return DIVOOM_DISP_DRAW_CTRL_PICKUP_LIHGT_TIME;

		case DIVOOM_DISP_DRAW_CTRL_PICKUP_LIGHT_SINGAL:
			if (flag == 1)			
				divoom_disp_ambient_ctrl_pickup_light_signal_init(k);
			return DIVOOM_DISP_DRAW_CTRL_PICKUP_LIHGT_TIME;

		case DIVOOM_DISP_DRAW_CTRL_FADE_IN_OUT:
			if (flag == 1)
				divoom_disp_ambient_ctrl_fade_in_out_init(k);
			return DIVOOM_DISP_DRAW_CTRL_FADE_IN_OUT_TIME;

		case DIVOOM_DISP_DRAW_CTRL_SIGNAL_COLOR:
			return DIVOOM_DISP_DRAW_CTRL_SIGNAL_COLOR_TIME;

		case DIVOOM_DISP_DRAW_CTRL_MIDDLE_WINDMILL:
			if (flag == 1)
				divoom_disp_ambient_ctrl_middle_mindmill_init(k);
			return DIVOOM_DISP_DRAW_CTRL_MIDDLE_WINDMILL_TIME;

		case DIVOOM_DISP_DRAW_CTRL_DIFFUSE:
			if (flag == 1)
				divoom_disp_ambient_ctrl_diffuse_init(k);
			return DIVOOM_DISP_DRAW_CTRL_DIFFUSE_TIME;

		case DIVOOM_DISP_DRAW_CTRL_RAINSTORM:
			if (flag == 1)
				divoom_disp_ambient_ctrl_rainstorm_init(k);
			return DIVOOM_DISP_DRAW_CTRL_RAINSTROM_TIME;

		case DIVOOM_DISP_DRAW_CTRL_AREA1:
			if (flag == 1)
				divoom_disp_ambient_ctrl_area1_init(k);
			return DIVOOM_DISP_DRAW_CTRL_AREA1_TIME;

		case DIVOOM_DISP_DRAW_CTRL_CYC_INSIDE:
			if (flag == 1)
				divoom_disp_ambient_ctrl_cyc_inside_init(k);
			return DIVOOM_DISP_DRAW_CTRL_CYC_INSIDE_TIME;

		case DIVOOM_DISP_DRAW_CTRL_AREA3:
			if (flag == 1)
				divoom_disp_ambient_ctrl_area3_init(k);
			return DIVOOM_DISP_DRAW_CTRL_AREA3_TIME;

		case DIVOOM_DISP_DRAW_CTRL_RED_GREEN_BLUE_WHITE_TEST:
			return DIVOOM_DISP_DRAW_CTRL_RED_GREEN_BLUE_WHITE_TIME;

		case DIVOOM_DISP_DRAW_CTRL_FLAME:
			if (flag == 1)
				divoom_disp_ambient_ctrl_flame_init(k);
			return DIVOOM_DISP_DRAW_CTRL_FLAME_TIME;

		case DIVOOM_DISP_DRAW_CTRL_FLAME2:
			if (flag == 1)
				divoom_disp_ambient_ctrl_flame2_init(k);
			return DIVOOM_DISP_DRAW_CTRL_FLAME2_TIME;

		case DIVOOM_DISP_DRAW_CTRL_RUNNING_LIGHT_AWAY:
			if (flag == 1)
				divoom_disp_ambient_ctrl_running_light_away_init(k);
			return DIVOOM_DISP_DRAW_CTRL_RUN_AWAY_TIME;

		case DIVOOM_DISP_DRAW_CTRL_METEOR_MULTICOLOR:
		case DIVOOM_DISP_DRAW_CTRL_METEOR:
			if (flag == 1)
				divoom_disp_ambient_ctrl_meteor_init(k);
			return DIVOOM_DISP_DRAW_CTRL_METEOR_TIME;

		case DIVOOM_DISP_DRAW_CTRL_WAVE_FLOW:
			if (flag == 1)
				divoom_disp_ambient_ctrl_wave_flow_init(k);
			return DIVOOM_DISP_DRAW_CTRL_WAVE_FLOW_TIME;
	}	

	return 0;
}

int divoom_disp_ambient_ctrl_get_speed_time(uint8 disp_mode, int flag) //初始化变量
{
	if (pdivoom_disp_ambient_ctrl_handle->rgb_info.select_light_index == 0)
	{
		return divoom_disp_ambient_ctrl_get_all_light_speed(disp_mode, flag)*10;
	}
	return 0;
}

void divoom_disp_ambient_ctrl_main(void *data)
{
	if (pdivoom_disp_ambient_ctrl_handle->rgb_info.select_light_index == 0)
	{
		divoom_disp_ambient_ctrl_disp_all_light(pdivoom_disp_ambient_ctrl_handle->disp_mode);
	}
}

uint8 divoom_disp_ambient_ctrl_get_key_switch(void)
{
	if (pdivoom_disp_ambient_ctrl_handle == NULL)
		return 1;
	
	return pdivoom_disp_ambient_ctrl_handle->rgb_info.KeyOnOff;
}

uint8 divoom_disp_ambient_ctrl_get_brightness(void)
{
	if (pdivoom_disp_ambient_ctrl_handle == NULL)
		return 0;
	
	return pdivoom_disp_ambient_ctrl_handle->rgb_info.brightness;
}

void divoom_disp_ambient_ctrl_set_brightness_inner(uint8 *brightness, uint8 screen_status)
{
	if (pdivoom_disp_ambient_ctrl_handle == NULL)
		return;


	if (brightness)
	{
		pdivoom_disp_ambient_ctrl_handle->rgb_info.brightness = *brightness;
		divoom_disp_ambient_ctrl_save_config();
	}	

	if (/*pdivoom_disp_ambient_ctrl_handle->rgb_info.on_off == 0 || */screen_status == 0)
	{
		divoom_oneline_led_on_off(0);
		return;
	}
    if(screen_status)
    {
		divoom_oneline_led_set_bright(pdivoom_disp_ambient_ctrl_handle->rgb_info.brightness);
    }
}

void divoom_disp_ambient_ctrl_set_brightness(uint8 *brightness, uint8 screen_status)
{
    divoom_disp_ambient_ctrl_set_brightness_inner(brightness, screen_status);
}

/*********************************************************************
  Function:       divoom_disp_ambient_ctrl_set_temp_brightness
  Description:	  设置一个临时亮度
  Input:          brightness 亮度值0~100
  Return:         none	
  Author:         wangft
  Data:           2025-3-5
**********************************************************************/

void divoom_disp_ambient_ctrl_set_temp_brightness(uint8 brightness)
{
	divoom_oneline_led_set_bright(brightness);
}

void divoom_disp_ambient_ctrl_set_rgb_on_off(uint8 on_off)
{
	if (pdivoom_disp_ambient_ctrl_handle == NULL)
		return;

	if (on_off == 0)
	{
		divoom_disp_ambient_ctrl_led_on_off(0);
	}
	else
	{
		divoom_disp_ambient_ctrl_set_brightness(NULL, 1);
	}
}

/*********************************************************************
  Function:       divoom_disp_ambient_ctrl_update
  Description:	  更新显示模式
  Input:          disp_draw_ctrl_info 要设置显示的参数
  Return:         none	
  Author:         wangft
  Data:           2020-5-25
**********************************************************************/
void divoom_disp_ambient_ctrl_update(Pdivoom_disp_draw_app_data disp_draw_ctrl_info)
{
	int time = 0;
	
	if(pdivoom_disp_ambient_ctrl_handle->task_id != 0)
	{
		divoom_com_pause_task(pdivoom_disp_ambient_ctrl_handle->task_id);
		time = divoom_disp_ambient_ctrl_get_speed_time(pdivoom_disp_ambient_ctrl_handle->disp_mode, 1);
		divoom_com_update_task(pdivoom_disp_ambient_ctrl_handle->task_id, time);
		divoom_com_resume_task(pdivoom_disp_ambient_ctrl_handle->task_id);
	}
}

void divoom_disp_ambient_ctrl_save_config(void)
{
    FILE *file_handle;
    uint32 check_sum = 0, read_bytes;

    file_handle = fopen(DIVOOM_DISP_AMBIENT_INFO_SAVE_PATH, "wb");
    if(file_handle == NULL)
    {
        return ;
    }

	read_bytes = fwrite(&pdivoom_disp_ambient_ctrl_handle->rgb_info, sizeof(pdivoom_disp_ambient_ctrl_handle->rgb_info), 1, file_handle);
	check_sum += divoom_app_com_get_check_sum((uint8 *)&pdivoom_disp_ambient_ctrl_handle->rgb_info, sizeof(pdivoom_disp_ambient_ctrl_handle->rgb_info));
    read_bytes = fwrite(&check_sum,sizeof(check_sum), 1, file_handle);
    fclose(file_handle);
}

void divoom_disp_ambient_ctrl_set_mode(uint8 mode)
{
	divoom_disp_draw_app_data disp_draw_ctrl_info = {0};
	int light_index = 0, total_mode = 0;
	
	if(NULL == pdivoom_disp_ambient_ctrl_handle)
		return;

	total_mode = DIVOOM_DISP_DRAW_CTRL_MULTICOLOR_END;
	if(pdivoom_disp_ambient_ctrl_handle->task_id != 0)
	{
		divoom_com_pause_task(pdivoom_disp_ambient_ctrl_handle->task_id);
	}

	if (mode >= total_mode)
	{
		pdivoom_disp_ambient_ctrl_handle->disp_mode = 0;
	}
	else
	{
		pdivoom_disp_ambient_ctrl_handle->disp_mode = mode;
	}

	light_index = pdivoom_disp_ambient_ctrl_handle->rgb_info.select_light_index;
	pdivoom_disp_ambient_ctrl_handle->rgb_info.rgb_light_list[light_index].select_effect = pdivoom_disp_ambient_ctrl_handle->disp_mode;

	divoom_disp_ambient_ctrl_save_config();
	divoom_disp_ambient_ctrl_set_pot_table(&disp_draw_ctrl_info, pdivoom_disp_ambient_ctrl_handle->disp_mode);
	divoom_disp_ambient_ctrl_update(&disp_draw_ctrl_info);
}


/*********************************************************************
  Function:       divoom_disp_ambient_ctrl_set_disp_mode
  Description:	  设置滤镜的显示模式
  Input:          dir 0 上一个 1 下一个
  Return:         none	
  Author:         wangft
  Data:           2020-5-25
**********************************************************************/
void divoom_disp_ambient_ctrl_set_disp_mode(uint8 dir)
{
	divoom_disp_draw_app_data disp_draw_ctrl_info = {0};
	int light_index = 0, total_mode = 0;
	
	if(NULL == pdivoom_disp_ambient_ctrl_handle)
		return;

	total_mode = DIVOOM_DISP_DRAW_CTRL_MULTICOLOR_END;
	if(pdivoom_disp_ambient_ctrl_handle->task_id != 0)
	{
		divoom_com_pause_task(pdivoom_disp_ambient_ctrl_handle->task_id);
	}

	if (dir)
	{
		pdivoom_disp_ambient_ctrl_handle->disp_mode++;
		if (pdivoom_disp_ambient_ctrl_handle->disp_mode >= total_mode)
		{
			pdivoom_disp_ambient_ctrl_handle->disp_mode = 0;
		}
	}
	else
	{
		if (pdivoom_disp_ambient_ctrl_handle->disp_mode == 0)
		{
			pdivoom_disp_ambient_ctrl_handle->disp_mode = total_mode - 1;
		}
		else
		{
			pdivoom_disp_ambient_ctrl_handle->disp_mode--;
		}
	}

	light_index = pdivoom_disp_ambient_ctrl_handle->rgb_info.select_light_index;
	pdivoom_disp_ambient_ctrl_handle->rgb_info.rgb_light_list[light_index].select_effect = pdivoom_disp_ambient_ctrl_handle->disp_mode;

	divoom_disp_ambient_ctrl_save_config();
	divoom_disp_ambient_ctrl_set_pot_table(&disp_draw_ctrl_info, pdivoom_disp_ambient_ctrl_handle->disp_mode);
	divoom_disp_ambient_ctrl_update(&disp_draw_ctrl_info);
	divoom_net_app_send_packet(DIVOOM_NET_COMM_CHANNEL_SET_RGB_INFO, DIVOOM_SERVER_TOPIC, (void *)&pdivoom_disp_ambient_ctrl_handle->rgb_info);
}

uint8 divoom_disp_ambient_ctrl_get_disp_mode(uint8 *total)
{
	if(NULL == pdivoom_disp_ambient_ctrl_handle)
		return 0xff;
	if(total)
	{
		*total = DIVOOM_DISP_DRAW_CTRL_END;
	}
	return pdivoom_disp_ambient_ctrl_handle->disp_mode;
}

/*********************************************************************
  Function:       divoom_disp_ambient_ctrl_set_disp_info
  Description:	  设置显示信息
  Input:          rgb_info 显示的信息
  Return:         none	
  Author:         wangft
  Data:           2022-12-9
**********************************************************************/

void divoom_disp_ambient_ctrl_set_disp_info(Pdivoom_channel_set_rgb_info rgb_info, uint8 screen_status)
{
	int i = 0, total_mode = 0;
	divoom_disp_draw_app_data disp_draw_ctrl_info = {0};

	if(NULL == pdivoom_disp_ambient_ctrl_handle)
		return;

	if(pdivoom_disp_ambient_ctrl_handle->task_id != 0)
	{
		divoom_com_pause_task(pdivoom_disp_ambient_ctrl_handle->task_id);
	}

	divoom_print("%d, %d, %x, %x, %x, %d, select_effect: %d\n", rgb_info->brightness, rgb_info->eq_on_off, rgb_info->color[0], rgb_info->color[1], rgb_info->color[2],
		rgb_info->color_cyc_flag, rgb_info->rgb_light_list[0].select_effect);
	memcpy(&pdivoom_disp_ambient_ctrl_handle->rgb_info, rgb_info, sizeof(pdivoom_disp_ambient_ctrl_handle->rgb_info));

	if(divoom_product_test_is_active() == 0)	//产品测试不保存
	{
		divoom_disp_ambient_ctrl_save_config();
	}
	divoom_disp_ambient_ctrl_set_brightness(NULL, screen_status);
	i = pdivoom_disp_ambient_ctrl_handle->rgb_info.select_light_index;
	total_mode = DIVOOM_DISP_DRAW_CTRL_MULTICOLOR_END;
	if (pdivoom_disp_ambient_ctrl_handle->rgb_info.rgb_light_list[i].select_effect >= total_mode)
	{
		pdivoom_disp_ambient_ctrl_handle->disp_mode = 0;
		pdivoom_disp_ambient_ctrl_handle->rgb_info.rgb_light_list[i].select_effect = 0;
	}
	else
	{
		pdivoom_disp_ambient_ctrl_handle->disp_mode = pdivoom_disp_ambient_ctrl_handle->rgb_info.rgb_light_list[i].select_effect;
	}

	divoom_disp_ambient_ctrl_set_pot_table(&disp_draw_ctrl_info, pdivoom_disp_ambient_ctrl_handle->disp_mode);
	divoom_disp_ambient_ctrl_update(&disp_draw_ctrl_info);
}

/*********************************************************************
  Function:       divoom_disp_ambient_ctrl_get_disp_info
  Description:	  获取显示信息
  Input:          none
  Return:         当前设备的显示信息	
  Author:         wangft
  Data:           2022-12-9
**********************************************************************/

Pdivoom_channel_set_rgb_info divoom_disp_ambient_ctrl_get_disp_info(void)
{
	if(NULL == pdivoom_disp_ambient_ctrl_handle)
		return NULL;

	return &pdivoom_disp_ambient_ctrl_handle->rgb_info;
}

void divoom_disp_ambient_ctrl_init_screen(Pdivoom_screen_color_info pcolor)
{
	int off = 0;
	if(NULL == pdivoom_disp_ambient_ctrl_handle)
		return;
	pdivoom_disp_ambient_ctrl_handle->disp_mode = DIVOOM_DISP_DRAW_CTRL_SIGNAL_COLOR;
	divoom_disp_ambient_ctrl_update(NULL);
	for(off = 0;off < TX_SCREEN_LED_NUM;off++)
	{
		divoom_disp_ctrl_set_data(off, pcolor, pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[0].app_pic_data);
	}
	divoom_disp_ambient_ctrl_send_rgb_data(pdivoom_disp_ambient_ctrl_handle->ambient_ctrl_info[0].app_pic_data);
}

void divoom_disp_ambient_ctrl_load_default_config(void)
{
	int i = 0;

	//设置一下默认参数
	pdivoom_disp_ambient_ctrl_handle->rgb_info.on_off = 1;
	pdivoom_disp_ambient_ctrl_handle->rgb_info.KeyOnOff = 1;
	pdivoom_disp_ambient_ctrl_handle->rgb_info.brightness = 100;
	pdivoom_disp_ambient_ctrl_handle->rgb_info.select_light_index = 0;
	pdivoom_disp_ambient_ctrl_handle->rgb_info.color_cyc_flag = 1;
	pdivoom_disp_ambient_ctrl_handle->rgb_info.eq_on_off = 1;

	pdivoom_disp_ambient_ctrl_handle->rgb_info.color[0] = 0xff;
	pdivoom_disp_ambient_ctrl_handle->rgb_info.color[1] = 0x1f;
	pdivoom_disp_ambient_ctrl_handle->rgb_info.color[2] = 0x3d;
	for (i = 0; i < 3; i++)
	{
		pdivoom_disp_ambient_ctrl_handle->rgb_info.rgb_light_list[i].select_effect = 0;
	}
}

void divoom_disp_ambient_ctrl_load_config(void)
{
    FILE *file_handle;
    uint32 check_sum = 0, raw_check_sum, read_bytes;

    if(pdivoom_disp_ambient_ctrl_handle == NULL)
    {
        return;
    }

    file_handle = fopen(DIVOOM_DISP_AMBIENT_INFO_SAVE_PATH, "rb");
    if(file_handle == NULL)
    {
		divoom_disp_ambient_ctrl_load_default_config();
        return ;
    }

	read_bytes = fread(&pdivoom_disp_ambient_ctrl_handle->rgb_info, sizeof(pdivoom_disp_ambient_ctrl_handle->rgb_info), 1, file_handle);
	check_sum = divoom_app_com_get_check_sum((uint8 *)&pdivoom_disp_ambient_ctrl_handle->rgb_info, sizeof(pdivoom_disp_ambient_ctrl_handle->rgb_info));
	read_bytes = fread(&raw_check_sum, sizeof(raw_check_sum), 1, file_handle);
    fclose(file_handle);

    if(raw_check_sum != check_sum)
    {
        divoom_app_com_del_file(DIVOOM_DISP_AMBIENT_INFO_SAVE_PATH);
        divoom_disp_ambient_ctrl_load_default_config();
    }
    if(pdivoom_disp_ambient_ctrl_handle->rgb_info.KeyOnOff > 1)
    {
        pdivoom_disp_ambient_ctrl_handle->rgb_info.KeyOnOff = 1;
    }
}

void divoom_disp_ambient_ctrl_init(int screen_status)
{
	uint16 time = 0, light_index = 0;
	divoom_disp_draw_app_data draw_ctrl_info = {0};
	
	if (pdivoom_disp_ambient_ctrl_handle)
	{
		return;
	}
	
	pdivoom_disp_ambient_ctrl_handle = (Pdivoom_disp_ambient_ctrl_info)divoom_system_malloc(sizeof(divoom_disp_ambient_ctrl_info));
	if (pdivoom_disp_ambient_ctrl_handle == NULL)
	{
		divoom_print("[%s:%d] divoom_malloc error\n", __func__, __LINE__);
		return;
	}
	divoom_oneline_led_init(screen_status);
	divoom_disp_ambient_ctrl_load_config();
	pdivoom_disp_ambient_ctrl_handle->task_id = 0;
	light_index = pdivoom_disp_ambient_ctrl_handle->rgb_info.select_light_index;
	pdivoom_disp_ambient_ctrl_handle->disp_mode = pdivoom_disp_ambient_ctrl_handle->rgb_info.rgb_light_list[light_index].select_effect;
	divoom_disp_ambient_ctrl_set_brightness_inner(NULL, screen_status);
	divoom_disp_ambient_ctrl_set_pot_table(&draw_ctrl_info, pdivoom_disp_ambient_ctrl_handle->disp_mode);
	time = divoom_disp_ambient_ctrl_get_speed_time(pdivoom_disp_ambient_ctrl_handle->disp_mode, 1);
	pdivoom_disp_ambient_ctrl_handle->task_id = divoom_com_create_task(divoom_disp_ambient_ctrl_main, time);
	divoom_com_resume_task(pdivoom_disp_ambient_ctrl_handle->task_id);
}

void divoom_disp_ambient_ctrl_close(void)
{
	if(pdivoom_disp_ambient_ctrl_handle)
	{
		if(pdivoom_disp_ambient_ctrl_handle->task_id != 0)
		{
			divoom_com_pause_task(pdivoom_disp_ambient_ctrl_handle->task_id);
			divoom_com_destroy_task(pdivoom_disp_ambient_ctrl_handle->task_id);
			pdivoom_disp_ambient_ctrl_handle->task_id = 0;
		}
		divoom_system_free(pdivoom_disp_ambient_ctrl_handle);
		pdivoom_disp_ambient_ctrl_handle = AK_NULL;
	}
}


