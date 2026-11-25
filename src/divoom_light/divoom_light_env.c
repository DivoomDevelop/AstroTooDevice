#include "divoom_disp_com.h"

#if 0
#define DIVOOM_DISP_ENV_MALLOC(size) divoom_com_malloc(size, __func__, __LINE__)
#define DIVOOM_DISP_ENV_FREE(var)    divoom_com_free(var, __func__, __LINE__)
#define DIVOOM_DISP_ENV_DBG     divoom_print
#define DIVOOM_DISP_ENV_ERR     divoom_print

#define DIVOOM_DISP_ENV_REFRESH_TIME   10
#define DIVOOM_DISP_ENV_MODE_TIME      10
#define DIVOOM_DISP_ENV_MODE_TIME_NEW_MODE_1		(25)
#define DIVOOM_DISP_ENV_MODE_TIME_NEW_MODE_2		(35)
#define DIVOOM_DISP_ENV_WEATHER_TIME   5
#define DIVOOM_DISP_ENV_DIGIT_SIZE_X    3
#define DIVOOM_DISP_ENV_DIGIT_SIZE_Y    5
#define DIVOOM_DISP_ENV_DIGIT_SIZE_BIG_X    5
#define DIVOOM_DISP_ENV_DIGIT_SIZE_BIG_Y    7

//led area
#define DIVOOM_DISP_ENV_LED_X_1						(7)
#define DIVOOM_DISP_ENV_LED_X_2						(17)
#define DIVOOM_DISP_ENV_LED_HOUR_Y					(1)
#define DIVOOM_DISP_ENV_LED_MIN_Y					(17)
#define DIVOOM_DISP_ENV_LED_POINT_X					(3)
#define DIVOOM_DISP_ENV_LED_POINT_1_Y				(21)
#define DIVOOM_DISP_ENV_LED_POINT_2_Y				(25)

//italic area
#define DIVOOM_DISP_ENV_ITALIC_Y					(9)
#define DIVOOM_DISP_ENV_ITALIC_X_1					(1)
#define DIVOOM_DISP_ENV_ITALIC_X_2					(8)
#define DIVOOM_DISP_ENV_ITALIC_X_3					(19)
#define DIVOOM_DISP_ENV_ITALIC_X_4					(26)
													//5*14*3
#define DIVOOM_DISP_ENV_ITALIC_W					(5)
#define DIVOOM_DISP_ENV_ITALIC_H					(14)
#define DIVOOM_DISP_ENV_ITALIC_NUM_SIZE				(210)

#if(SCREEN_WIDTH_32 == 32)
//温度的各个位置
#define TEMPERATURE_Y						(6)
//如果没有百位,这个值就是17,没有十位,就是21,依次类推
#define TEMPERATURE_LOW_ZERO_X				(14)
#define TEMPERATURE_LOW_ZERO_Y				(9)
#define TEMPERATURE_FIRST_X					(17)
#define TEMPERATURE_SECOND_X				(20)
#define TEMPERATURE_THIRD_X					(24)
#define TEMPERATURE_POINT_1_X				(18)
#define TEMPERATURE_POINT_1_Y				(16)

#define TEMPERATURE_POINT_2_X				(19)
#define TEMPERATURE_POINT_2_Y				(15)

#define TEMPERATURE_POINT_3_X				(20)
#define TEMPERATURE_POINT_3_Y				(16)

#define TEMPERATURE_POINT_4_X				(19)
#define TEMPERATURE_POINT_4_Y				(17)

#define TEMPERATURE_C_F_X					(23)
#define TEMPERATURE_C_F_Y					(15)


#define ENV_JUST_NUM_LESS_X					(12)
#define ENV_JUST_NUM_LESS_Y					(2)
#define ENV_JUST_NUM_BIG_X					(22)
#define ENV_JUST_NUM_BIG_Y					(18)

#define DIVOOM_DISP_ENV_DISP_START_X		(0)
#define DIVOOM_DISP_ENV_DISP_START_Y		(10)

#define DIVOOM_DISP_ENV_EFFECT_START_Y1		(5)
#define DIVOOM_DISP_ENV_EFFECT_START_Y2		(25)

#define ENV_SQUARE_NUM_LESS_X				(12)
#define ENV_SQUARE_NUM_LESS_Y				(3)
#define ENV_SQUARE_NUM_BIG_X				(21)
#define ENV_SQUARE_NUM_BIG_Y				(17)

//下面这些是四个方块显示效果
/*
   时间		日期
   温度		天气
*/
//第一行显示的位置
#define ENV_FOUR_BLOCK_DISP_ONE_ROW			(1)
//第二行位置
#define ENV_FOUR_BLOCK_DISP_TWO_ROW			(9)
//第三行位置->温度行
#define ENV_FOUR_BLOCK_DISP_THREE_ROW		(18)
//时间的两个点
/*#define ENV_FOUR_BLOCK_DISP_TIME_DOT_X		(3)
#define ENV_FOUR_BLOCK_DISP_TIME_DOT_ONE_Y	(10)
#define ENV_FOUR_BLOCK_DISP_TIME_DOT_TWO_Y	(12)
//日期的点,可能不需要了
#define ENV_FOUR_BLOCK_DISP_DATA_DOT_Y		(11)
#define ENV_FOUR_BLOCK_DISP_DATA_DOT_X		(23)*/

//时间的两个X轴
#define ENV_FOUR_BLOCK_DISP_TIME_BIG_X		(6)
#define ENV_FOUR_BLOCK_DISP_TIME_LITTLE_X	(11)

//日期总共有4种, 总长是 9, 10, 12, 13四种
//每种的起始X轴都不一样
//1 2 2 2
#define ENV_FOUR_BLOCK_DAT_9_ONE			(17)
#define ENV_FOUR_BLOCK_DAT_9_TWO			(22)
#define ENV_FOUR_BLOCK_DAT_9_THREE			(27)

//1 2 2 1
//这个有点特殊,有3+3+4和3+4+3,到时候特殊变化
#define ENV_FOUR_BLOCK_DAT_10_ONE			(17)

//1 1 1 1
#define ENV_FOUR_BLOCK_DAT_12_ONE			(17)
#define ENV_FOUR_BLOCK_DAT_12_TWO			(23)
#define ENV_FOUR_BLOCK_DAT_12_THREE			(27)

//0 1 1 1
#define ENV_FOUR_BLOCK_DAT_13_ONE			(16)
#define ENV_FOUR_BLOCK_DAT_13_TWO			(22)
#define ENV_FOUR_BLOCK_DAT_13_THREE			(27)

//日期的位置, 除了10, 11, 12的1是特殊的,其他的都是3X5大小的
#define ENV_FOUR_BLOCK_DAT_M_ONE			(17)
#define ENV_FOUR_BLOCK_DAT_M_TWO			(19)
#define ENV_FOUR_BLOCK_DAT_D_ONE			(25)
#define ENV_FOUR_BLOCK_DAT_D_TWO			(29)

//大于10
#define ENV_FOUR_BLOCK_DISP_DATA_DOT_BIG_Y		(11)
#define ENV_FOUR_BLOCK_DISP_DATA_DOT_BIG_X		(23)
//小于10
#define ENV_FOUR_BLOCK_DISP_DATA_DOT_LITTLE_Y	(11)
#define ENV_FOUR_BLOCK_DISP_DATA_DOT_LITTLE_X	(22)

//百位,一般都是1
//如果是零下,用上一位的位置表示负号,其中X不变,Y + 2
#define ENV_FOUR_BLOCK_TEMP_H				(6)
#define ENV_FOUR_BLOCK_TEMP_T				(8)
#define ENV_FOUR_BLOCK_TEMP_N				(12)
#define ENV_FOUR_BLOCK_TEMP_C_F_Y			(25)
#define ENV_FOUR_BLOCK_TEMP_C_F_X			(10)

#define ENV_FOUR_BLOCK_WEATHER_START_X		(16)
#define ENV_FOUR_BLOCK_WEATHER_START_Y		(16)

//四个条显示
/*
   时间
   周
   温度
   日期
*/
#define ENV_FOUR_LINE_TIME_Y				(2)
#define ENV_FOUR_LINE_TIME_ONE_X			(3)
#define ENV_FOUR_LINE_TIME_TWO_X			(8)
#define ENV_FOUR_LINE_TIME_THREE_X			(14)
#define ENV_FOUR_LINE_TIME_FOUR_X			(19)

#define ENV_FOUR_LINE_WEEK_Y				(10)
#define ENV_FOUR_LINE_WEEK_X_START			(3)
//间隔2
#define ENV_FOUR_LINE_WEEK_GAP				(2)

//如果是零下,那么取代最大的位数显示﹣, 其中X不变,Y + 2
#define ENV_FOUR_LINE_TEMP_Y				(18)
#define ENV_FOUR_LINE_TEMP_ONE_X			(3)
#define ENV_FOUR_LINE_TEMP_TWO_X			(8)
#define ENV_FOUR_LINE_TEMP_THREE_X			(13)
#define ENV_FOUR_LINE_TEMP_C_F_X			(20)

#define ENV_FOUR_LINE_DATA_Y				(26)
#define ENV_FOUR_LINE_DATA_ONE_X			(3)
#define ENV_FOUR_LINE_DATA_TWO_X			(8)
#define ENV_FOUR_LINE_DATA_THREE_X			(14)
#define ENV_FOUR_LINE_DATA_FOUR_X			(19)

//多彩数字点的位置
//x,y要set4次,x,y;x,y+1;x+1,y;x+1,y+1
/*#define ENV_TIME_COLOURFUL_NUM_P_X			(1)
#define ENV_TIME_COLOURFUL_NUM_P_BIG_Y		(12)
#define ENV_TIME_COLOURFUL_NUM_P_SMALL_Y	(18)
//多彩数字的位置
#define ENV_TIME_COLOURFUL_NUM_BIG_X		(7)
#define ENV_TIME_COLOURFUL_NUM_SMALL_X		(20)
#define ENV_TIME_COLOURFUL_NUM_BIG_Y		(4)
#define ENV_TIME_COLOURFUL_NUM_SMALL_Y		(19)*/

#else
#define DIVOOM_DISP_ENV_DISP_START_X   0
#define DIVOOM_DISP_ENV_DISP_START_Y   5

#define DIVOOM_DISP_ENV_EFFECT_START_Y1   3
#define DIVOOM_DISP_ENV_EFFECT_START_Y2   11
#endif

//数据０到９的显示像点为３*５
static const uint8 divoom_env_dight_small_disp_Info[11][5] = {                                  
    {7,5,5,5,7},
    {2,3,2,2,7},
    {7,4,7,1,7},
    {7,4,7,4,7},
    {5,5,7,4,4},
    {7,1,7,4,7},
    {7,1,7,5,7},
    {7,4,4,4,4},
    {7,5,7,5,7},
    {7,5,7,4,7},
    {0,0,0,0,0},
};
static const uint8 divoom_env_dight_big_disp_Info[11][7] = {
    {14,17,17,17,17,17,14},
    { 4, 6, 5, 4, 4, 4,31},
    {14,17,17, 8, 4, 2,31},
    {14,17,16,14,16,17,14},
    { 8,12,10, 9,31, 8, 8},
    {31, 1, 1,31,16,17,14},
    {14,17, 1,15,17,17,14},
    {31,17,16, 8, 4, 4, 4},
    {14,17,17,14,17,17,14},
    {14,17,17,30,16,17,14},
    { 0, 0, 0, 0, 0, 0, 0},
};

/*typedef struct _divoom_coulourful_num_xy_
{
	uint8 x;
	uint8 y:6;
	//记录当前点是第几个块的颜色,总共4个区域
	uint8 colour_type:2;
}divoom_coulourful_num_xy, *Pdivoom_coulourful_num_xy;*/

/*const static int need_offset[][2] = {
	{-1, 0},
	{-1, -1},
	{-1, 1},
	{0, -1},
	{0, 1},
	{1, -1},
	{1, 0},
	{1, 1},
};*/

//最新的创音大赛的始终分钟地址
#define NEW_MATCH_CLOCK

#if defined NEW_MATCH_CLOCK
#define DIVOOM_LIGHT_ENV_HOUR_DOT	9
#define DIVOOM_LIGHT_ENV_MIN_DOT	13
#else
#define DIVOOM_LIGHT_ENV_HOUR_DOT 2
#define DIVOOM_LIGHT_ENV_MIN_DOT 5
#endif
#define DIVOOM_LIGHT_ENV_HOUR_INDEX_NUM 12
#if defined NEW_MATCH_CLOCK
#define DIVOOM_LIGHT_ENV_MIN_INDEX_NUM 24
#else
#define DIVOOM_LIGHT_ENV_MIN_INDEX_NUM 12
#endif

#if(SCREEN_WIDTH_32 == 16)
const uint8 divoom_light_env_hour_pos[DIVOOM_LIGHT_ENV_HOUR_INDEX_NUM][DIVOOM_LIGHT_ENV_HOUR_DOT][2] =
{
    {{7,5},{7,6}},
    {{8,5},{8,6}},
    {{8,6},{9,6}},
    {{8,7},{9,7}},
    {{8,8},{9,8}},
    {{8,8},{8,9}},
    {{7,8},{7,9}},
    {{6,8},{6,9}},
    {{5,8},{6,8}},
    {{5,7},{6,7}},
    {{5,6},{6,6}},
    {{6,5},{6,6}},
};
#else
#if defined NEW_MATCH_CLOCK
//阴影部分有意义为,再向下移一位,如果当前的颜色==设置的颜色,就不画阴影了
const uint8 divoom_light_env_hour_pos[DIVOOM_LIGHT_ENV_HOUR_INDEX_NUM][DIVOOM_LIGHT_ENV_HOUR_DOT][2] = 
{
	{{15, 7}, {15, 8}, {15, 9}, {15, 10}, {15, 11}, {15, 12}, {15, 13}, {15, 14}, {15, 15}},
	{{20, 8}, {19, 9}, {19, 10}, {18, 11}, {17, 12}, {17, 13}, {16, 14}, {15,15}, {14, 16}},
	{{22, 11}, {21,11}, {20, 12}, {19, 13}, {18, 13}, {17, 14}, {16, 15}, {15, 15}, {14, 16}},
	{{22, 15}, {21, 15}, {20, 15}, {19, 15}, {18, 15}, {17, 15}, {16, 15}, {15, 15}, {14, 15}},
	{{22, 19}, {21, 19}, {20, 18}, {19, 17}, {18, 17}, {17, 16}, {16, 15}, {15, 15}, {14, 14}},
	{{20, 22}, {19, 21}, {19, 20}, {18, 19}, {17, 18}, {17, 17}, {16, 16}, {15, 15}, {14, 14}},
	{{15, 23}, {15, 22}, {15, 21}, {15, 20}, {15, 19}, {15, 18}, {15, 17}, {15, 16}, {15, 14}},
	{{10, 22}, {11, 21}, {11, 20}, {12, 19}, {13, 18}, {13, 17}, {14, 16}, {15, 15}, {16, 14}},
	{{8, 19}, {9, 19}, {10, 18}, {11, 17}, {12, 17}, {13, 16}, {14, 15}, {15, 15}, {16, 14}},
	{{8, 15}, {9, 15}, {10, 15}, {11, 15}, {12, 15}, {13, 15}, {14, 15}, {15, 15}, {16, 15}},
	{{8, 11}, {9, 11}, {10, 12}, {11, 13}, {12, 13}, {13, 14}, {14, 15}, {15, 15}, {16, 16}},
	{{10, 8}, {11, 9}, {11, 10}, {12, 11}, {13, 12}, {13, 13}, {14, 14}, {15, 15}, {16, 16}},
};
#else
const uint8 divoom_light_env_hour_pos[DIVOOM_LIGHT_ENV_HOUR_INDEX_NUM][DIVOOM_LIGHT_ENV_HOUR_DOT][2] =
{
    {{15,11},{15,13}},
    {{17,11},{17,13}},
    {{17,13},{19,13}},
    {{17,15},{17,15}},
    {{17,17},{19,17}},
    {{17,17},{17,19}},
    {{15,17},{15,19}},
	{{14,17},{14,19}},
    {{11,17},{13,17}},
    {{11,15},{13,15}},
    {{11,13},{13,13}},
    {{13,11},{13,13}},
};
#endif
#endif

#if defined NEW_MATCH_CLOCK
#if(SCREEN_WIDTH_32 == 32)
//[][0]是X轴,[][1]是Y轴
//这里是可控制颜色的点
const uint8 divoom_light_env_point_pos[][2] = 
{
	{15, 1},
	{15, 2},
	{8, 3},
	{22, 3},
	{3, 8},
	{27, 8},
	{1, 15},
	{2, 15},
	{28, 15},
	{29, 15},
	{3, 22},
	{27, 22},
	{27, 8},
	{27, 22},
	{15, 28},
	{15, 29},
};

const static int divoom_light_env_italic_nums[] = 
{
	DIVOOM_DISP_ITALIC_NUMS_0,
	DIVOOM_DISP_ITALIC_NUMS_1,
	DIVOOM_DISP_ITALIC_NUMS_2,
	DIVOOM_DISP_ITALIC_NUMS_3,
	DIVOOM_DISP_ITALIC_NUMS_4,
	DIVOOM_DISP_ITALIC_NUMS_5,
	DIVOOM_DISP_ITALIC_NUMS_6,
	DIVOOM_DISP_ITALIC_NUMS_7,
	DIVOOM_DISP_ITALIC_NUMS_8,
	DIVOOM_DISP_ITALIC_NUMS_9,
};

const int divoom_light_env_blue_colourful_minute[] = 
{
	DIVOOM_DISP_BLUE_COLOURFUL_MINUTE_0,
	DIVOOM_DISP_BLUE_COLOURFUL_MINUTE_1,
	DIVOOM_DISP_BLUE_COLOURFUL_MINUTE_2,
	DIVOOM_DISP_BLUE_COLOURFUL_MINUTE_3,
	DIVOOM_DISP_BLUE_COLOURFUL_MINUTE_4,
	DIVOOM_DISP_BLUE_COLOURFUL_MINUTE_5,
	DIVOOM_DISP_BLUE_COLOURFUL_MINUTE_6,
	DIVOOM_DISP_BLUE_COLOURFUL_MINUTE_7,
	DIVOOM_DISP_BLUE_COLOURFUL_MINUTE_8,
	DIVOOM_DISP_BLUE_COLOURFUL_MINUTE_9,
	DIVOOM_DISP_BLUE_COLOURFUL_MINUTE_10,
	DIVOOM_DISP_BLUE_COLOURFUL_MINUTE_11,
};

const int divoom_light_env_blue_colourful_hour[] = 
{
	DIVOOM_DISP_BLUE_COLOURFUL_HOUR_0,
	DIVOOM_DISP_BLUE_COLOURFUL_HOUR_1,
	DIVOOM_DISP_BLUE_COLOURFUL_HOUR_2,
	DIVOOM_DISP_BLUE_COLOURFUL_HOUR_3,
	DIVOOM_DISP_BLUE_COLOURFUL_HOUR_4,
	DIVOOM_DISP_BLUE_COLOURFUL_HOUR_5,
	DIVOOM_DISP_BLUE_COLOURFUL_HOUR_6,
	DIVOOM_DISP_BLUE_COLOURFUL_HOUR_7,
	DIVOOM_DISP_BLUE_COLOURFUL_HOUR_8,
	DIVOOM_DISP_BLUE_COLOURFUL_HOUR_9,
	DIVOOM_DISP_BLUE_COLOURFUL_HOUR_10,
	DIVOOM_DISP_BLUE_COLOURFUL_HOUR_11,
};

const int divoom_light_env_gold_hour[] = 
{
	DIVOOM_DISP_GOLD_HOUR_0,
	DIVOOM_DISP_GOLD_HOUR_1,
	DIVOOM_DISP_GOLD_HOUR_2,
	DIVOOM_DISP_GOLD_HOUR_3,
	DIVOOM_DISP_GOLD_HOUR_4,
	DIVOOM_DISP_GOLD_HOUR_5,
	DIVOOM_DISP_GOLD_HOUR_6,
	DIVOOM_DISP_GOLD_HOUR_7,
	DIVOOM_DISP_GOLD_HOUR_8,
	DIVOOM_DISP_GOLD_HOUR_9,
	DIVOOM_DISP_GOLD_HOUR_10,
	DIVOOM_DISP_GOLD_HOUR_11,
};

const int divoom_light_env_gold_minute[] = 
{
	DIVOOM_DISP_GOLD_MINUTE_0,
	DIVOOM_DISP_GOLD_MINUTE_1,
	DIVOOM_DISP_GOLD_MINUTE_2,
	DIVOOM_DISP_GOLD_MINUTE_3,
	DIVOOM_DISP_GOLD_MINUTE_4,
	DIVOOM_DISP_GOLD_MINUTE_5,
	DIVOOM_DISP_GOLD_MINUTE_6,
	DIVOOM_DISP_GOLD_MINUTE_7,
	DIVOOM_DISP_GOLD_MINUTE_8,
	DIVOOM_DISP_GOLD_MINUTE_9,
	DIVOOM_DISP_GOLD_MINUTE_10,
	DIVOOM_DISP_GOLD_MINUTE_11,
};

const int divoom_light_env_min_pic_id[] = 
{
	DIVOOM_DISP_NEW_CLOCK_SIX_MIN_0,
	DIVOOM_DISP_NEW_CLOCK_SIX_MIN_1,
	DIVOOM_DISP_NEW_CLOCK_SIX_MIN_2,
	DIVOOM_DISP_NEW_CLOCK_SIX_MIN_3,
	DIVOOM_DISP_NEW_CLOCK_SIX_MIN_4,
	DIVOOM_DISP_NEW_CLOCK_SIX_MIN_5,
	DIVOOM_DISP_NEW_CLOCK_SIX_MIN_6,
	DIVOOM_DISP_NEW_CLOCK_SIX_MIN_7,
	DIVOOM_DISP_NEW_CLOCK_SIX_MIN_8,
	DIVOOM_DISP_NEW_CLOCK_SIX_MIN_9,
	DIVOOM_DISP_NEW_CLOCK_SIX_MIN_10,
	DIVOOM_DISP_NEW_CLOCK_SIX_MIN_11,
	DIVOOM_DISP_NEW_CLOCK_SIX_MIN_12,
	DIVOOM_DISP_NEW_CLOCK_SIX_MIN_13,
	DIVOOM_DISP_NEW_CLOCK_SIX_MIN_14,
	DIVOOM_DISP_NEW_CLOCK_SIX_MIN_15,
	DIVOOM_DISP_NEW_CLOCK_SIX_MIN_16,
	DIVOOM_DISP_NEW_CLOCK_SIX_MIN_17,
	DIVOOM_DISP_NEW_CLOCK_SIX_MIN_18,
	DIVOOM_DISP_NEW_CLOCK_SIX_MIN_19,
	DIVOOM_DISP_NEW_CLOCK_SIX_MIN_20,
	DIVOOM_DISP_NEW_CLOCK_SIX_MIN_21,
	DIVOOM_DISP_NEW_CLOCK_SIX_MIN_22,
	DIVOOM_DISP_NEW_CLOCK_SIX_MIN_23,
};

#endif
#endif

#if(SCREEN_WIDTH_32 == 16)
const uint8 divoom_light_env_min_pos[DIVOOM_LIGHT_ENV_MIN_INDEX_NUM][DIVOOM_LIGHT_ENV_MIN_DOT][2] =
{
    {{7,2},{7,3},{7,4},{7,5},{7,6}},
    {{9,3},{9,4},{8,5},{7,6},{0,0}},
    {{11,5},{10,5},{9,6},{8,6},{0,0}},
    {{8,7},{9,7},{10,7},{11,7},{12,7}},
    {{8,7},{9,8},{10,8},{11,9},{0,0}},
    {{8,8},{8,9},{9,10},{9,11},{0,0}},
    {{7,8},{7,9},{7,10},{7,11},{7,12}},
    {{7,8},{6,9},{5,10},{5,11},{0,0}},
    {{6,8},{5,8},{4,9},{3,9},{0,0}},
    {{2,7},{3,7},{4,7},{5,7},{6,7}},
    {{3,5},{4,5},{5,6},{6,7},{0,0}},
    {{5,3},{5,4},{6,5},{6,6},{0,0}},
};
#else
/*#if defined NEW_MATCH_CLOCK
const uint8 divoom_light_env_min_pos[DIVOOM_LIGHT_ENV_MIN_INDEX_NUM][DIVOOM_LIGHT_ENV_MIN_DOT][2] =
{
	{{255, 255}, {15, 4}, {15, 5}, {15, 6}, {15, 7}, {15, 8}, {15, 9}, {15, 10}, {15, 11}, {15, 12}, {15, 13}, {15, 14}, {15, 15}},
	{{255, 255}, {22, 5}, {22, 6}, {21, 6}, {21, 7}, {20, 8}, {19, 9}, {19, 10}, {18, 11}, {17, 12}, {17, 13}, {16, 14}, {15,15}},
	{{255, 255}, {255, 255}, {20, 5}, {20, 6}, {19, 7}, {18, 8}, {18, 9}, {17, 10}, {17, 11}, {16, 12}, {16, 13}, {15, 14}, {15, 15}},
	{{255, 255}, {255, 255}, {255, 255}, {23, 7}, {22, 8}, {21, 9}, {20, 10}, {19, 11}, {18, 12}, {17, 13}, {16, 14}, {15, 15}, {14, 16}},
	{{255, 255}, {25, 9}, {24, 10}, {23, 10}, {22, 11}, {20, 12}, {19, 12}, {19, 13}, {18, 13}, {17, 14}, {16, 14}, {15, 15}, {14, 16}},
	{{26, 12}, {25, 12}, {24, 13}, {24, 13}, {12, 14}, {21, 14}, {20, 14}, {19, 14}, {18, 15}, {17,15}, {16, 15}, {15, 15}, {14, 16}},
	{{255, 255}, {26, 15}, {25, 15}, {24, 15}, {23, 15}, {22, 15}, {21, 15}, {20, 15}, {19, 15}, {18, 15}, {17, 15}, {16, 15}, {15, 15}},
	{{26, 18}, {25, 18}, {24, 17}, {23, 17}, {22, 16}, {21, 16}, {20, 16}, {19, 16}, {18, 15}, {17, 15}, {16, 15}, {15, 15}, {14, 14}},
	{{255, 255}, {25, 21}, {24, 20}, {23, 20}, {22, 19}, {21, 19}, {20, 18}, {19, 18}, {19, 17}, {18, 17}, {17, 16}, {16, 16}, {15, 15}},
	{{255, 255}, {255, 255}, {255, 255}, {255, 255}, {23, 23}, {22, 22}, {21, 21}, {20, 20}, {19, 19}, {18, 18}, {17, 17}, {16, 16}, {15, 15}},
};
#else*/
//下面的运算是每个点会往右和往下往右下扩展,所以间隔为2,小时也是这样的
const uint8 divoom_light_env_min_pos[DIVOOM_LIGHT_ENV_MIN_INDEX_NUM][DIVOOM_LIGHT_ENV_MIN_DOT][2] =
{
    {{15,5},{15,7},{15,9},{15,11},{15,13}},
    {{19,7},{19,9},{17,11},{15,13},{0,0}},
    {{23,11},{21,11},{19,13},{17,13},{0,0}},
    {{25,15},{23,15},{21,15},{19,15},{17,15}},
    {{17,17},{19,19},{21,19},{23,21},{0,0}},
    {{17,17},{17,19},{19,21},{19,23},{0,0}},
    {{15,17},{15,19},{15,21},{15,23},{15,25}},
    {{15,17},{13,19},{11,21},{11,23},{0,0}},
    {{13,17},{11,17},{9,19},{7,19},{0,0}},
    {{5,15},{7,15},{9,15},{11,15},{13,15}},
    {{7,11},{9,11},{11,13},{13,15},{0,0}},
    {{11,7},{11,9},{13,11},{13,13},{0,0}},
};
#endif

const uint8 celsius_disp_sep[] = {1,6,2,2,6};   //3*5
const uint8 celsius_disp_sep2[] = {0x1d,4,4,4,0x1c};   //3*5
const uint8 celsius_pic_disp_sep[] = {4,3,1,3};   //3*5
const uint8 fahrenheit_disp_sep[] = {1,6,2,6,2};    //3*7
const uint8 fahrenheit_disp_sep2[] = {0x1d,4,0x0c,4,4};    //3*7
const uint8 fahrenheit_pic_disp_sep[] = {4,3,1,3,1};    //3*5
const uint8 humidity_disp_sep[] = {0x11, 0x08, 0x04, 0x02,0x11};    //5*5
const uint8 humidity_pic_disp_sep[] = {0x9, 0x4, 0x2, 0x9};    //5*5
const uint8 bright_disp_sep[] = {1,1,3,0,5,2,5};    //3*7
const uint8 noise_disp_sep[] = {0X14,0X14,0X77,0X55,0X77};    //7*5
const uint8 noise_pic_disp_sep[] = {0X0A,0X1B,0X1B};    //7*5
const uint8 date_disp_sep[] = {0,4,2,1,0};    //3*5

const divoom_light_color_info divoom_env_red_color = {255,0,0};
const divoom_light_color_info divoom_env_write_color = {255,255,255};
const divoom_light_color_info divoom_env_org_color = {255,255,0};
const divoom_light_color_info divoom_env_black_color = {0,0,0};
const divoom_light_color_info divoom_env_blue_color = {0,255,255};
const divoom_light_color_info divoom_env_light_blue_color = {39,176,255};


const divoom_light_color_info divoom_env_effect_color_table[] =
{
    {0,255,255}, {0X00,98, 255}, {0X2B,0X00,0XFF}, {142, 0, 255}, {55, 8, 83}, {255, 0, 157}, 
    {219,119,34}, {255,49, 0}, {255,190,0}, {193,255,0}, {8,75,44}, {95,255,0}, 
};

//每一个字块有四个颜色,共有四个字块
//[总共几个][第几个圈的颜色][RGB]
/*const uint8 divoom_light_env_colourful_num[][4][3] = 
{
	{{255,13,35},  {255,98,102},  {158,7,21},   {80,23,24}},
	{{125,224,66}, {208,255,121}, {0,135,31},   {15,66,24}},
	{{255,189,54}, {255,224,129}, {155,107,31}, {78,59,27}},
	{{0,164,233},  {68,222,253},  {0,131,149},  {6,61,75}}
};*/

static const divoom_light_color_info divoom_light_env_mode2_color_table[] =
{
    {255,0,74},
    {255,0,156},
    {247,0,255},
    {189,0,255},
    {123,0,255},
    {57,0,255},
    {0,125,255},
    {0,227,255},
    {0,255,206},
    {0,255,126},
    {123,255,0},
    {181,255,0},
    {255,255,0},
    {255,178,0},
    {255,72,0},
    {255,0,0},
};


typedef struct _divoom_light_env_gif_info
{
    uint16 len;
    uint8  *data;
}divoom_light_env_gif_info, *Pdivoom_light_env_gif_info;


typedef struct _divoom_light_env_ctrl
{
    uint8 flag;
    uint8 task_flag;
//    uint8 index;
    uint8 sys_ms;
    uint8 pic_num;  //天气动画中图片总张数
    uint8 pic_id;   //当前播放天气动画中的第pic_id张
    uint8 hour_flag;   //时间制式
    uint8 temp_flag;   //时间制式
    uint8 refresh_flag;  //显示更新
    uint8 refresh_time_flag;  //时间显示更新
    uint8 disp_type; //显示模式
    uint32 task_id;
	uint8 old_effect_type;
    uint8 effect_type;   //显示效果
    uint8 color_pos;
    uint8 *pic_buf;
    uint8 gif_id;
    uint8 gif_over;
    uint16 gif_time;
    uint32 mode;
    uint32 tick;
    uint32  gif_off;
    int8 celsius_value;
    int fahrenheit_value;    
    uint8 humidity_value;
    uint16 bright_value;
    uint8 weather_type;
    uint32 load_env_info;
    void *decode_handle;    //解压句柄
	divoom_light_color_info color;
	divoom_light_color_info back_color;
    divoom_light_env_time_info   time_info;
    divoom_light_env_date_info   date_info;    
    Pdivoom_light_env_gif_info define_pic_info;     
    void *gif_handle;
    DIVOOM_LIGHT_ENV_READ *penv_read;
    uint8 disp_buf[DIVOOM_IMAGE_RGB_SIZE_64];
    uint8 bak_buf[DIVOOM_IMAGE_ENCODE_MAX_64_SIZE];    
	//Pdivoom_coulourful_num_xy cur_array;
	//Pdivoom_coulourful_num_xy next_array;
}divoom_light_env_ctrl, *Pdivoom_light_env_ctrl;

const static uint16 divoom_light_env_small_weather_picture_table[] = 
{
	DIVOOM_DISP_WEATHER_GIF_CLEARSKY_PIC,
	//DIVOOM_DISP_WEATHER_GIF_FEWCLOUDS_PIC,
	DIVOOM_DISP_WEATHER_GIF_FEWCLOUDS_PIC,
	DIVOOM_DISP_WEATHER_GIF_SCATTEREDCLOUDS_PIC,
	DIVOOM_DISP_WEATHER_GIF_BROKENCLOUDS_PIC,
	DIVOOM_DISP_WEATHER_GIF_SHOWERRAIN_PIC,
	DIVOOM_DISP_WEATHER_GIF_RAIN_PIC,
	DIVOOM_DISP_WEATHER_GIF_THUNDERSTORM_PIC,
	DIVOOM_DISP_WEATHER_GIF_SONW_PIC,
	DIVOOM_DISP_WEATHER_GIF_MIST_PIC,
	DIVOOM_DISP_WEATHER_GIF_CLEARSKY_PIC,
	DIVOOM_DISP_WEATHER_GIF_FEWCLOUDS_PIC,
	DIVOOM_DISP_WEATHER_GIF_SCATTEREDCLOUDS_PIC,
	DIVOOM_DISP_WEATHER_GIF_BROKENCLOUDS_PIC,
	DIVOOM_DISP_WEATHER_GIF_SHOWERRAIN_PIC,
	DIVOOM_DISP_WEATHER_GIF_RAIN_PIC,
	DIVOOM_DISP_WEATHER_GIF_THUNDERSTORM_PIC,
	DIVOOM_DISP_WEATHER_GIF_SONW_PIC,
	DIVOOM_DISP_WEATHER_GIF_MIST_PIC,
};

static const uint16 divoom_light_env_weather_picture_table[] =    
{
//DIVOOM_DISP_WEATHER_CLEAR_DAY_PIC,        
//DIVOOM_DISP_WEATHER_FEW_CLOUD_DAY_PIC,        
//DIVOOM_DISP_WEATHER_SCATTERED_CLOUD_DAY_PIC,    
//DIVOOM_DISP_WEATHER_BROKEN_CLOUD_DAY_PIC,    
//DIVOOM_DISP_WEATHER_SHOWER_RAIN_DAY_PIC,
//DIVOOM_DISP_WEATHER_RAIN_DAY_PIC,    
//DIVOOM_DISP_WEATHER_THUNDERSTORMS_DAY_PIC,
//DIVOOM_DISP_WEATHER_SNOW_DAY_PIC,    
//DIVOOM_DISP_WEATHER_MIST_DAY_PIC,        
//DIVOOM_DISP_WEATHER_CLEAR_NIGHT_PIC,        
//DIVOOM_DISP_WEATHER_SCATTERED_CLOUD_NIGHT_PIC,            
//DIVOOM_DISP_WEATHER_BROKEN_CLOUD_NIGHT_PIC,        
//DIVOOM_DISP_WEATHER_SHOWER_RAIN_NIGHT_PIC,
//DIVOOM_DISP_WEATHER_RAIN_NIGHT_PIC,
//DIVOOM_DISP_WEATHER_THUNDERSTORMS_NIGHT_PIC,        
//DIVOOM_DISP_WEATHER_SNOW_NIGHT_PIC,
//DIVOOM_DISP_WEATHER_MIST_NIGHT_PIC
//天气类型对应关系
//晴空:new_sunny
//少云
//散云:云散开,云不多
//遮掉大半个天空的乌云
//洗澡一样的雨
//下雨
//雷雨
//下雪
//雾DIVOOM_DISP_WEATHER_GIF_MIST_PIC,
//晴空
//少云
//散云:云散开,云不多
//遮掉大半个天空的乌云
//洗澡一样的雨
//下雨
//雷雨
//下雪
//雾DIVOOM_DISP_WEATHER_GIF_MIST_PIC,

//忘记了要去掉divoom_disp,先这样吧
DIVOOM_DISP_DIVOOM_DISP_WEATHER_GIF_NEW_CLEARSKY_PIC,
DIVOOM_DISP_WEATHER_GIF_FEWCLOUDS_PIC,
DIVOOM_DISP_DIVOOM_DISP_WEATHER_GIF_NEW_SCATTEREDCLOUDS_PIC,
DIVOOM_DISP_DIVOOM_DISP_WEATHER_GIF_NEW_BROKENCLOUDS_PIC,
DIVOOM_DISP_WEATHER_GIF_SHOWERRAIN_PIC,
DIVOOM_DISP_WEATHER_GIF_RAIN_PIC,
DIVOOM_DISP_DIVOOM_DISP_WEATHER_GIF_NEW_THUNDERSTORM_PIC,
DIVOOM_DISP_DIVOOM_DISP_WEATHER_GIF_NEW_SONW_PIC,
DIVOOM_DISP_DIVOOM_DISP_WEATHER_GIF_NEW_MIST_PIC,
DIVOOM_DISP_DIVOOM_DISP_WEATHER_GIF_NEW_CLEARSKY_PIC,
DIVOOM_DISP_WEATHER_GIF_FEWCLOUDS_PIC,
DIVOOM_DISP_DIVOOM_DISP_WEATHER_GIF_NEW_SCATTEREDCLOUDS_PIC,
DIVOOM_DISP_DIVOOM_DISP_WEATHER_GIF_NEW_BROKENCLOUDS_PIC,
DIVOOM_DISP_WEATHER_GIF_SHOWERRAIN_PIC,
DIVOOM_DISP_WEATHER_GIF_RAIN_PIC,
DIVOOM_DISP_DIVOOM_DISP_WEATHER_GIF_NEW_THUNDERSTORM_PIC,
DIVOOM_DISP_DIVOOM_DISP_WEATHER_GIF_NEW_SONW_PIC,
DIVOOM_DISP_DIVOOM_DISP_WEATHER_GIF_NEW_MIST_PIC,

/*DIVOOM_DISP_WEATHER_GIF_CLEARSKY_PIC,
//DIVOOM_DISP_WEATHER_GIF_FEWCLOUDS_PIC,
DIVOOM_DISP_WEATHER_GIF_FEWCLOUDS_PIC,
DIVOOM_DISP_WEATHER_GIF_SCATTEREDCLOUDS_PIC,
DIVOOM_DISP_WEATHER_GIF_BROKENCLOUDS_PIC,
DIVOOM_DISP_WEATHER_GIF_SHOWERRAIN_PIC,
DIVOOM_DISP_WEATHER_GIF_RAIN_PIC,
DIVOOM_DISP_WEATHER_GIF_THUNDERSTORM_PIC,
DIVOOM_DISP_WEATHER_GIF_SONW_PIC,
DIVOOM_DISP_WEATHER_GIF_MIST_PIC,
DIVOOM_DISP_WEATHER_GIF_CLEARSKY_PIC,
DIVOOM_DISP_WEATHER_GIF_FEWCLOUDS_PIC,
DIVOOM_DISP_WEATHER_GIF_SCATTEREDCLOUDS_PIC,
DIVOOM_DISP_WEATHER_GIF_BROKENCLOUDS_PIC,
DIVOOM_DISP_WEATHER_GIF_SHOWERRAIN_PIC,
DIVOOM_DISP_WEATHER_GIF_RAIN_PIC,
DIVOOM_DISP_WEATHER_GIF_THUNDERSTORM_PIC,
DIVOOM_DISP_WEATHER_GIF_SONW_PIC,
DIVOOM_DISP_WEATHER_GIF_MIST_PIC,*/
};
static Pdivoom_light_env_ctrl pdivoom_light_env_handle = NULL;

//0对应小图,1对应大图
Pdivoom_light_env_gif_info divoom_light_env_get_gif_from_table(uint8 index, uint8 flag);
void divoom_light_env_update_weather_gif(void);

Pdivoom_light_color_info divoom_light_env_get_fill_color(void)
{
    if(pdivoom_light_env_handle == NULL)
    {
        return divoom_main_get_time_ptr();
    }
//    else if(pdivoom_light_env_handle->effect_type == 2)
//    {
//        return (Pdivoom_light_color_info)&divoom_env_org_color;
//    }
//	else if(pdivoom_light_env_handle->effect_type == 0)
//	{
//		return &pdivoom_light_env_handle->color;
//	}
    else
    {
//        return (Pdivoom_light_color_info)&divoom_env_write_color;
        //if(pdivoom_light_env_handle->effect_type != DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_COLOR_FLOOR)
        {
		    return &pdivoom_light_env_handle->color;
        }
        //else
        //{
        //    return &pdivoom_light_env_handle->back_color;
        //}
    }

    
}

Pdivoom_light_color_info divoom_light_env_get_back_color(void)
{
        return (Pdivoom_light_color_info)&divoom_env_black_color;
}


Pdivoom_light_color_info divoom_light_env_get_effect_color(void)
{
    return (Pdivoom_light_color_info)&divoom_env_write_color;
}
typedef struct _divoom_light_env_gif_global{
    uint32   index;
    uint32 hot_off;
}divoom_light_env_gif_global, *Pdivoom_light_env_gif_global;

static divoom_light_env_gif_global gdivoom_light_env_gif_ctrl = {0, DIVOOM_PAGE_SIZE};

#define DIVOOM_LIGHT_ENV_GIF_DIS_NUM  10

uint8 divoom_light_env_gif_init(void)
{
    uint8 err_time = 0, end_flag = 0;
    uint16 pic_len;
    
    divoom_light_env_gif_init_retry_table:

    
    if(pdivoom_light_env_handle->gif_handle)
    {
        divoom_load_movie_close(pdivoom_light_env_handle->gif_handle);
        pdivoom_light_env_handle->gif_handle = NULL;
    }
    
    if(gdivoom_light_env_gif_ctrl.hot_off == DIVOOM_PAGE_SIZE)
    {
        gdivoom_light_env_gif_ctrl.index = divoom_app_file_manage_get_next_file_index(DIVOOM_APP_FILE_TYPE_HOT, gdivoom_light_env_gif_ctrl.index);
        if(gdivoom_light_env_gif_ctrl.index == T_U32_MAX)
        {
            gdivoom_light_env_gif_ctrl.index = divoom_app_file_manage_get_next_file_index(DIVOOM_APP_FILE_TYPE_HOT, 0);            
            goto divoom_light_env_gif_init_retry_table;
        }
        if(gdivoom_light_env_gif_ctrl.index == T_U32_MAX)
        {
            divoom_print("%s:%d system no hot file!\n",__func__, __LINE__);
            return 0;
        }
    }
    if(divoom_app_file_manage_check_file_exist(DIVOOM_APP_FILE_TYPE_HOT,  gdivoom_light_env_gif_ctrl.index) == 0)
    {
        //如果文件不存在的话，将播放下一个文件
        gdivoom_light_env_gif_ctrl.index = divoom_app_file_manage_get_next_file_index(DIVOOM_APP_FILE_TYPE_HOT, gdivoom_light_env_gif_ctrl.index);
    }
    pdivoom_light_env_handle->gif_handle = divoom_load_movie_open_read(DIVOOM_APP_FILE_TYPE_HOT,gdivoom_light_env_gif_ctrl.index, NULL, T_U32_MAX);
    if(pdivoom_light_env_handle->gif_handle == NULL)
    {
        divoom_print("%s: it fails to open!: %d, %d, %d\n",__func__, err_time,gdivoom_light_env_gif_ctrl.index, gdivoom_light_env_gif_ctrl.hot_off);
        err_time ++;
        gdivoom_light_env_gif_ctrl.hot_off = DIVOOM_PAGE_SIZE; 
        if(err_time <= 3)
        {
            gdivoom_light_env_gif_ctrl.index = 0;
            goto divoom_light_env_gif_init_retry_table;
        }
        else
        {
            return 0;
        }
    }
    
    
    pdivoom_light_env_handle->pic_buf = pdivoom_light_env_handle->bak_buf;
    pdivoom_light_env_handle->gif_id = 0;
    pdivoom_light_env_handle->gif_over = 0;
    pdivoom_light_env_handle->gif_off = gdivoom_light_env_gif_ctrl.hot_off;
    divoom_load_movie_set_pic_off(pdivoom_light_env_handle->gif_handle, pdivoom_light_env_handle->gif_off);
    pic_len = divoom_load_movie_load(pdivoom_light_env_handle->gif_handle, pdivoom_light_env_handle->pic_buf, &end_flag);
    if(pic_len && end_flag == 0)
    {
        pdivoom_light_env_handle->gif_time = divoom_image_decode_get_time_info(pdivoom_light_env_handle->pic_buf);
        divoom_com_update_task(pdivoom_light_env_handle->task_id, pdivoom_light_env_handle->gif_time / 10);
        divoom_load_movie_set_pic_off(pdivoom_light_env_handle->gif_handle, pdivoom_light_env_handle->gif_off);
    }
    else
    {
        divoom_load_movie_close(pdivoom_light_env_handle->gif_handle);
        pdivoom_light_env_handle->gif_handle = NULL;
        err_time ++;
        gdivoom_light_env_gif_ctrl.hot_off = DIVOOM_PAGE_SIZE; 
        if(err_time <= 3)
        {
            goto divoom_light_env_gif_init_retry_table;
        }
        else
        {
            return 0;
        }
    }
    divoom_print("%s,offset: %d,index: %d offset: %d\n",__func__,  pdivoom_light_env_handle->gif_off,gdivoom_light_env_gif_ctrl.index, gdivoom_light_env_gif_ctrl.hot_off);
    return 1;
    
}

/*********************************************************************
  Function:         divoom_light_env_gif_format_hot
  Description:     系统将对指定区域进行擦除动作
  Input:             
  Output             NONE
**********************************************************************/    
    

void divoom_light_env_gif_format_hot(uint32 file_index)
{
    if(pdivoom_light_env_handle == NULL)
    {
        if(gdivoom_light_env_gif_ctrl.index == file_index)
        {
            gdivoom_light_env_gif_ctrl.index = divoom_app_file_manage_get_next_file_index(DIVOOM_APP_FILE_TYPE_HOT, file_index);
        }
        gdivoom_light_env_gif_ctrl.hot_off = DIVOOM_PAGE_SIZE;
        return;
    }
    
    divoom_com_pause_task(pdivoom_light_env_handle->task_id);
    
    gdivoom_light_env_gif_ctrl.index = divoom_app_file_manage_get_next_file_index(DIVOOM_APP_FILE_TYPE_HOT, file_index);
    gdivoom_light_env_gif_ctrl.hot_off = DIVOOM_PAGE_SIZE;

    if(pdivoom_light_env_handle->gif_handle)
    {
        pdivoom_light_env_handle->gif_id = DIVOOM_LIGHT_ENV_GIF_DIS_NUM + 1;
    }
    
    divoom_com_resume_task(pdivoom_light_env_handle->task_id);

    
        
}

T_BOOL divoom_light_env_gif_main(void)
{
    uint8 end_flag, frame_attr;
    uint16 pic_len, time;
    uint32 cur_off;

    //这里只显示图片，不显示文字
    do{
        cur_off = divoom_load_movie_get_pic_off(pdivoom_light_env_handle->gif_handle);
        pic_len = divoom_load_movie_load(pdivoom_light_env_handle->gif_handle, pdivoom_light_env_handle->pic_buf, &end_flag);
        if(pic_len == 0 || end_flag)
        {
            pdivoom_light_env_handle->gif_id = DIVOOM_LIGHT_ENV_GIF_DIS_NUM + 1;
            pdivoom_light_env_handle->gif_over = 1;
            return 0;
        }
        divoom_image_decode_get_image_info(pdivoom_light_env_handle->pic_buf, &frame_attr);
    }while(frame_attr != DIVOOM_IMAGE_ENCODE_PIC_TYPE);
    
    if(pic_len)
    {
        time = divoom_image_decode_get_time_info(pdivoom_light_env_handle->pic_buf);
        if(time != pdivoom_light_env_handle->gif_time)
        {
            pdivoom_light_env_handle->gif_time = time ;
            divoom_com_update_task(pdivoom_light_env_handle->task_id, pdivoom_light_env_handle->gif_time / 10);
            pdivoom_light_env_handle->gif_id ++;
            if(pdivoom_light_env_handle->gif_id > DIVOOM_LIGHT_ENV_GIF_DIS_NUM)
            {
                if(pdivoom_light_env_handle->gif_over)
                {
                    gdivoom_light_env_gif_ctrl.hot_off = DIVOOM_PAGE_SIZE;
                }
                else
                {
                    gdivoom_light_env_gif_ctrl.hot_off = cur_off;
                }
                return 0;
            }
        }
        divoom_screen_disp_divoom_data(pdivoom_light_env_handle->pic_buf);
    }
    return 1;
}

void divoom_light_env_gif_exit(void)
{
    divoom_load_movie_close(pdivoom_light_env_handle->gif_handle);
    pdivoom_light_env_handle->gif_handle = NULL;
    if(pdivoom_light_env_handle->task_flag)
    {
         divoom_com_update_task(pdivoom_light_env_handle->task_id,DIVOOM_DISP_ENV_REFRESH_TIME/2);
    }
    else
    {
        divoom_com_update_task(pdivoom_light_env_handle->task_id,DIVOOM_DISP_ENV_REFRESH_TIME);
    }
}


void divoom_light_env_fill_mode4(uint8 x, uint8 value, Pdivoom_light_color_info fill_color, Pdivoom_light_color_info back_color)
{
    #define DIVOOM_DISP_ENV_FILL_WIDTH  3
    #define DIVOOM_DISP_ENV_FILL_HEIGH  9
    #define DIVOOM_DISP_ENV_FILL_START  3

    uint8 i, j ;

    for(i = 0; i < DIVOOM_DISP_ENV_FILL_HEIGH; i ++)
    {
        for(j = 0; j < DIVOOM_DISP_ENV_FILL_WIDTH; j++)
        {
            if(i >= DIVOOM_DISP_ENV_FILL_HEIGH - value)
            {
                divoom_disp_ctrl_set_dot(x + j, DIVOOM_DISP_ENV_FILL_START + i,fill_color,pdivoom_light_env_handle->disp_buf);
            }
            else
            {
                divoom_disp_ctrl_set_dot(x + j, DIVOOM_DISP_ENV_FILL_START + i,back_color,pdivoom_light_env_handle->disp_buf);
            }
        }
    }
}

/*********************************************************************
  Function:         divoom_light_env_fill_area
  Description:     填充某个固定的区域
  Input:             start_x       起始位置
                       start_y  ?起始位置
                       size_x        填充大小
                       size_y       填充大小
                       user_data?
                       pget_call?位置是否需要填充的回调
                       pfill_color       填充的着色
                       pback_color     背景的着色
                       disp_buf     显示缓冲
  Return:           void
  Author:           divoom
**********************************************************************/

void divoom_light_env_fill_area(uint8 start_x, uint8 start_y, uint8 size_x, uint8 size_y, uint8 user_data, 
        DIVOOM_DISP_ENV_GET_ACTIVE   *pget_call, uint8 *disp_buf, Pdivoom_light_color_info color)
{
    uint8 i, j;

    for(i = 0; i < size_y; i ++)
    {
        for(j = 0; j < size_x; j ++)
        {
            if(pget_call(j, i, user_data))
            {
                if(color == NULL)
                {
                    divoom_disp_ctrl_set_dot(start_x + j , start_y+ i, divoom_light_env_get_fill_color(), disp_buf);
                }
                else
                {
                    divoom_disp_ctrl_set_dot(start_x + j , start_y+ i, color, disp_buf);
                }
            }
        }
    }
}

bool  divoom_light_env_get_digit(uint8 x, uint8 y, uint8 user_data)
{
    return divoom_env_dight_small_disp_Info[user_data][y] & (1 << x);
}

bool  divoom_light_env_get_big_digit(uint8 x, uint8 y, uint8 user_data)
{
    return divoom_env_dight_big_disp_Info[user_data][y] & (1 << x);
}

bool  divoom_light_env_celsius_sep(uint8 x, uint8 y, uint8 user_data)
{
    return celsius_disp_sep[y] & (1 << x);
}

bool  divoom_light_env_celsius2_sep(uint8 x, uint8 y, uint8 user_data)
{
    return celsius_disp_sep2[y] & (1 << x);
}

bool  divoom_light_env_celsius_pic_sep(uint8 x, uint8 y, uint8 user_data)
{
    return celsius_pic_disp_sep[y] & (1 << x);
}

/*********************************************************
  * Copyright(c)uint8My style
  * Authored by Shaco Ma
  * CREATED TIME: 2017年12月12日  9:08:13
  * FUNCTION: divoom_light_env_disp_digit
  * AUTHOR: Shaco Ma
  * IUPUT: uint8 x, uint8 y, uint16 value, uint8 disp_type, uint8 *buf
  * OUTPUT_MUTE_CHECK_METHOD: uint8 
  * DESCRIPTION: 显示5*7的数字
  *********************************************************/
//增加一个color,是字体的颜色,如果是在env内部使用,可以给NULL,env内部会自动查找当前的颜色设置,在env外部使用,加上颜色设置,
//不然这个函数在divoom_light_env_fill_area不能正确处理当前的env颜色
uint8 divoom_light_env_disp_big_digit(uint8 x, uint8 y, uint16 value, uint8 disp_type, uint8 *buf, uint8 effect_type, Pdivoom_light_color_info color)
{
#if(SCREEN_WIDTH_32 == 32)
	uint8 front_size;
	switch(effect_type)
	{
		case DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_LED_DISP:
			front_size = DIVOOM_8X14_BIG_DIGIT;
			break;
		//case DIVOOM_LIGHT_ENV_TIME_DUSP_EFFECT_ITALIC:
		//	front_size = DIVOOM_4X13_ITALIC;
		//	break;
		case DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_COLOURFUL:
		//case DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_SQUARE:
			front_size = DIVOOM_6X12_DIGIT;
			break;
		//case DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_JUST_NUM:
		//	front_size = DIVOOM_6X12_DIGIT;
		//	break;
		case DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_COLOURFUL_NUM:
			front_size = DIVOOM_7X9_DIGIT;
			break;
		default:
			return 0;
	}
	divoom_disp_digit_fill_area(x, y, value, front_size, buf, (color==NULL)?divoom_light_env_get_fill_color():color);
	return x;
#else
    uint8  i, disp_info[3], start_x = x, start_y = y, flag = 0;

    disp_info[0] = value / 100;
    disp_info[1] = (value % 100) / 10;
    disp_info[2] = value % 10;
    
    for(i = 0; i < 3; i ++)
    {
        if(disp_info[i] || i == 2)
        {
            flag = 1;
        }
        
        if(flag)
        {
			if(effect_type == DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_JUST_NUM)
				divoom_light_env_fill_area(start_x, start_y, DIVOOM_DISP_ENV_DIGIT_SIZE_BIG_X, DIVOOM_DISP_ENV_DIGIT_SIZE_BIG_Y, disp_info[i],
						divoom_light_env_get_big_digit, buf, color);
			else if(effect_type == DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_COLOR_FLOOR)
				divoom_light_env_fill_area(start_x, start_y, DIVOOM_DISP_ENV_DIGIT_SIZE_BIG_X, DIVOOM_DISP_ENV_DIGIT_SIZE_BIG_Y, disp_info[i],
						divoom_light_env_get_big_digit, buf, color);
			/*else if(effect_type == DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_JUST_BAK)
				divoom_light_env_fill_area(start_x, start_y, DIVOOM_DISP_ENV_DIGIT_SIZE_BIG_X, DIVOOM_DISP_ENV_DIGIT_SIZE_BIG_Y, disp_info[i],
						divoom_light_env_get_big_digit, buf, &divoom_env_black_color);*/

//            divoom_disp_digit_fill_area(start_x, start_y,disp_info[i],DIVOOM_3X5_DIGIT, buf,divoom_light_env_get_fill_color());
            
            start_x += DIVOOM_DISP_ENV_DIGIT_SIZE_BIG_X + 1;
        }
        else if(disp_type)
        {
            start_x += DIVOOM_DISP_ENV_DIGIT_SIZE_BIG_X + 1;
        }
    }
    
    return start_x;
#endif
}

/*********************************************************************
  Function:         divoom_light_env_disp_big_digit
  Description:     用三位显示大数据3*5
  Input:             x      起始位置
                       y      起始位置
                       value   显示的值
  Return:           NONE
  Author:           divoom
**********************************************************************/

uint8 divoom_light_env_disp_digit(uint8 x, uint8 y, uint16 value, uint8 disp_type, uint8 *buf)
{
    uint8  i, disp_info[3], start_x = x, start_y = y, flag = 0;

    disp_info[0] = value / 100;
    disp_info[1] = (value % 100) / 10;
    disp_info[2] = value % 10;
    
    for(i = 0; i < 3; i ++)
    {
        if(disp_info[i] || i == 2)
        {
            flag = 1;
        }
        
        if(flag)
        {
            divoom_light_env_fill_area(start_x, start_y, DIVOOM_DISP_ENV_DIGIT_SIZE_X, DIVOOM_DISP_ENV_DIGIT_SIZE_Y,disp_info[i],
                divoom_light_env_get_digit, buf, NULL);

//            divoom_disp_digit_fill_area(start_x, start_y,disp_info[i],DIVOOM_3X5_DIGIT, buf,divoom_light_env_get_fill_color());
            
            start_x += DIVOOM_DISP_ENV_DIGIT_SIZE_X + 1;
        }
        else if(disp_type)
        {
            start_x += DIVOOM_DISP_ENV_DIGIT_SIZE_X + 1;
        }
    }
    
    return start_x;
}

void divoom_light_env_disp_effect(void)
{
    uint8 i, off, max;
    
    if(pdivoom_light_env_handle->effect_type == DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_COLOURFUL)
    {
		//见 DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_COLOURFUL 描述
        max = sizeof(divoom_light_env_mode2_color_table)/sizeof(divoom_light_color_info);
        pdivoom_light_env_handle->color_pos ++;
        if(pdivoom_light_env_handle->color_pos >= max)
        {
            pdivoom_light_env_handle->color_pos = 0;
        }
        for(i = 0, off = pdivoom_light_env_handle->color_pos; i < SCREEN_WIDTH_32; i ++, off ++)
        {
            if(off >= max)
            {
                off = 0;
            }
            divoom_disp_ctrl_set_dot(i, DIVOOM_DISP_ENV_EFFECT_START_Y1, 
					(Pdivoom_light_color_info)&divoom_light_env_mode2_color_table[off], pdivoom_light_env_handle->disp_buf);
#if(SCREEN_WIDTH_32 == 32)
			//彩条做成两行
            divoom_disp_ctrl_set_dot(i, DIVOOM_DISP_ENV_EFFECT_START_Y1 + 1, 
					(Pdivoom_light_color_info)&divoom_light_env_mode2_color_table[off], pdivoom_light_env_handle->disp_buf);
            divoom_disp_ctrl_set_dot(i, DIVOOM_DISP_ENV_EFFECT_START_Y1 + 2, 
					(Pdivoom_light_color_info)&divoom_light_env_mode2_color_table[off], pdivoom_light_env_handle->disp_buf);
#endif
            divoom_disp_ctrl_set_dot(SCREEN_WIDTH_32 - i - 1, DIVOOM_DISP_ENV_EFFECT_START_Y2, 
					(Pdivoom_light_color_info)&divoom_light_env_mode2_color_table[off], pdivoom_light_env_handle->disp_buf);
#if(SCREEN_WIDTH_32 == 32)
            divoom_disp_ctrl_set_dot(SCREEN_WIDTH_32 - i - 1, DIVOOM_DISP_ENV_EFFECT_START_Y2 + 1, 
					(Pdivoom_light_color_info)&divoom_light_env_mode2_color_table[off], pdivoom_light_env_handle->disp_buf);
            divoom_disp_ctrl_set_dot(SCREEN_WIDTH_32 - i - 1, DIVOOM_DISP_ENV_EFFECT_START_Y2 + 2, 
					(Pdivoom_light_color_info)&divoom_light_env_mode2_color_table[off], pdivoom_light_env_handle->disp_buf);
#endif
        }
    }
#if 0
    else if(pdivoom_light_env_handle->effect_type == DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_SQUARE)
    {
        max = sizeof(divoom_light_env_mode2_color_table)/sizeof(divoom_light_color_info) * 3;
        pdivoom_light_env_handle->color_pos ++;
        if(pdivoom_light_env_handle->color_pos >= max)
        {
            pdivoom_light_env_handle->color_pos = 0;
        }
        off = pdivoom_light_env_handle->color_pos;
#if(SCREEN_WIDTH_32 == 32)
		/*
		   0				 31
		   *******************
		   *******************
		*/
        for(i = 0; i < SCREEN_WIDTH_32; i ++)
        {
            divoom_disp_ctrl_set_dot(i, 0, (Pdivoom_light_color_info)&divoom_env_blue_color, pdivoom_light_env_handle->disp_buf);
            divoom_disp_ctrl_set_dot(i, 1, (Pdivoom_light_color_info)&divoom_env_blue_color, pdivoom_light_env_handle->disp_buf);
        }
		/*
		   **2
		   **
		   **29
		*/
        for(i = 2; i < SCREEN_HEIGHT_32; i ++)
        {
            divoom_disp_ctrl_set_dot(SCREEN_WIDTH_32 - 2, i, (Pdivoom_light_color_info)&divoom_env_blue_color, pdivoom_light_env_handle->disp_buf);
            divoom_disp_ctrl_set_dot(SCREEN_WIDTH_32 - 1, i, (Pdivoom_light_color_info)&divoom_env_blue_color, pdivoom_light_env_handle->disp_buf);
        }
		/*
		   2			29
		   **************
		   **************
		*/
        for(i = SCREEN_WIDTH_32 - 2; i > 1; i --)
        {
            divoom_disp_ctrl_set_dot(i, SCREEN_HEIGHT_32 - 1, (Pdivoom_light_color_info)&divoom_env_blue_color, pdivoom_light_env_handle->disp_buf);
            divoom_disp_ctrl_set_dot(i, SCREEN_HEIGHT_32 - 2, (Pdivoom_light_color_info)&divoom_env_blue_color, pdivoom_light_env_handle->disp_buf);
        }
		/*
		   **2
		   **
		   **31
		*/
        for(i = SCREEN_WIDTH_32 - 1; i > 1; i --)
        {
            divoom_disp_ctrl_set_dot(0, i, (Pdivoom_light_color_info)&divoom_env_blue_color, pdivoom_light_env_handle->disp_buf);
            divoom_disp_ctrl_set_dot(1, i, (Pdivoom_light_color_info)&divoom_env_blue_color, pdivoom_light_env_handle->disp_buf);
        }
#else
        for(i = 1; i < SCREEN_WIDTH_32 - 2; i ++, off ++)
        {
            if(off >= max)
            {
                off = 0;
            }
            divoom_disp_ctrl_set_dot(i, 0, (Pdivoom_light_color_info)&divoom_env_blue_color, pdivoom_light_env_handle->disp_buf);
        }
        for(i = 0; i < SCREEN_HEIGHT_32 - 1; i ++, off ++)
        {
            if(off >= max)
            {
                off = 0;
            }
            divoom_disp_ctrl_set_dot(SCREEN_WIDTH_32 - 2, i, (Pdivoom_light_color_info)&divoom_env_blue_color, pdivoom_light_env_handle->disp_buf);
        }
        for(i =SCREEN_WIDTH_32 - 2; i >0; i --, off ++)
        {
            if(off >= max)
            {
                off = 0;
            }
            divoom_disp_ctrl_set_dot(i, SCREEN_HEIGHT_32 - 1, (Pdivoom_light_color_info)&divoom_env_blue_color, pdivoom_light_env_handle->disp_buf);
        }
        for(i = SCREEN_WIDTH_32 - 1; i > 0; i --, off ++)
        {
            if(off >= max)
            {
                off = 0;
            }
            divoom_disp_ctrl_set_dot(1, i, (Pdivoom_light_color_info)&divoom_env_blue_color, pdivoom_light_env_handle->disp_buf);
        }
#endif
    }
#endif
}


const uint16 divoom_light_env_temperature_indicator_pic[] = {
    /*DIVOOM_DISP_TEMPERATURE_INDICATOR_00,
    DIVOOM_DISP_TEMPERATURE_INDICATOR_01,
    DIVOOM_DISP_TEMPERATURE_INDICATOR_02,
    DIVOOM_DISP_TEMPERATURE_INDICATOR_03,
    DIVOOM_DISP_TEMPERATURE_INDICATOR_04,
    DIVOOM_DISP_TEMPERATURE_INDICATOR_05,
    DIVOOM_DISP_TEMPERATURE_INDICATOR_06,
    DIVOOM_DISP_TEMPERATURE_INDICATOR_07,
    DIVOOM_DISP_TEMPERATURE_INDICATOR_08,
    DIVOOM_DISP_TEMPERATURE_INDICATOR_09,
    DIVOOM_DISP_TEMPERATURE_INDICATOR_10,
    DIVOOM_DISP_TEMPERATURE_INDICATOR_11,
    DIVOOM_DISP_TEMPERATURE_INDICATOR_12,
    DIVOOM_DISP_TEMPERATURE_INDICATOR_13,*/
	DIVOOM_DISP_TEMPERATURE_NEW_0,
	DIVOOM_DISP_TEMPERATURE_NEW_1,
	DIVOOM_DISP_TEMPERATURE_NEW_2,
	DIVOOM_DISP_TEMPERATURE_NEW_3,
	DIVOOM_DISP_TEMPERATURE_NEW_4,
	DIVOOM_DISP_TEMPERATURE_NEW_5,
	DIVOOM_DISP_TEMPERATURE_NEW_6,
	DIVOOM_DISP_TEMPERATURE_NEW_7,
	DIVOOM_DISP_TEMPERATURE_NEW_8,
	DIVOOM_DISP_TEMPERATURE_NEW_9,
	DIVOOM_DISP_TEMPERATURE_NEW_10,
	DIVOOM_DISP_TEMPERATURE_NEW_11,
};


const uint8 divoom_light_letter_c_3X5_disp_info[5] = {7,5,4,5,7};
const uint8 divoom_light_letter_f_3X5_disp_info[5] = {7,4,6,4,4};
#if(SCREEN_WIDTH_32 == 32)
const uint8 divoom_light_letter_c_6X10_disp_info[10] = {63, 63, 51, 48, 48, 48, 48, 51, 63, 63};
const uint8 divoom_light_letter_f_6X10_disp_info[10] = {63, 63, 48, 48, 62, 62, 48, 48, 48, 48};

const uint8 divoom_light_letter_c_5X7_disp_info[10] = {31,31,24,24,24,31,31};
const uint8 divoom_light_letter_f_5X7_disp_info[10] = {31,31,24,31,31,24,24};
#endif

/*****************************************************************************
 Prototype    : divoom_light_env_temperature_indicator_get_picid
 Description  : 根据温度分级
 Input        : int16 value  温度值
                uint8 flag   1:华氏温度  0:摄氏度
 Output       : None
 Return Value : 
 
  History        :
  1.Date         : 2017/11/27
    Author       : coronaa
    Modification : Created function
*****************************************************************************/
uint16 divoom_light_env_temperature_indicator_get_picid(int16 value, uint8 flag)
{
//根据温度分级
    uint16 tmp = 0;
    /*if(flag)    // 1为华氏温度
    {
        if(value < -55)
            tmp  =0;
        else if(value < -40) 
            tmp  =1;            
        else if(value < -25) 
            tmp  =2;
        else if(value < -10) 
            tmp  =3;  
        else if(value < 5) 
            tmp  =4;
        else if(value < 20) 
            tmp  =5;  
        else if(value < 35) 
            tmp  =6;  
        else if(value < 50) 
            tmp  =7;  
        else if(value < 65) 
            tmp  =8;  
        else if(value < 80) 
            tmp  =9;
        else if(value < 95) 
            tmp  =10;
        else if(value < 110) 
            tmp  =11;
        else if(value < 125) 
            tmp  =12;        
        else
            tmp = 13;
    }
    else    //0为摄氏度
    {
        if(value < -15)
            tmp  =0;
        else if(value < -10) 
            tmp  =1;            
        else if(value < -5) 
            tmp  =2;
        else if(value < 0) 
            tmp  =3;  
        else if(value < 5) 
            tmp  =4;
        else if(value < 10) 
            tmp  =5;  
        else if(value < 15) 
            tmp  =6;  
        else if(value < 20) 
            tmp  =7;  
        else if(value < 25) 
            tmp  =8;  
        else if(value < 30) 
            tmp  =9;
        else if(value < 35) 
            tmp  =10;
        else if(value < 40) 
            tmp  =11;
        else if(value < 45) 
            tmp  =12;        
        else
            tmp = 13;
    }
	*/
	if(flag)
	{
		//-15对应的是5,根据对应的摄氏度转换
        if(value < 5)
            tmp  =0;
        else if(value < 15)
            tmp  =1;
        else if(value < 26)
            tmp  =2;
        else if(value < 37)
            tmp  =3;
        else if(value < 48)
            tmp  =4;
        else if(value < 59)
            tmp  =5;
        else if(value < 70)
            tmp  =6;
        else if(value < 80)
            tmp  =7;
        else if(value < 91)
            tmp  =8;
        else if(value < 102)
            tmp  =9;
        else if(value < 113)
            tmp  =10;
		else
			tmp = 11;
	}
	else
	{
        if(value < -15)
            tmp  =0;
        else if(value < -9)
            tmp  =1;
        else if(value < -3)
            tmp  =2;
        else if(value < 3)
            tmp  =3;
        else if(value < 9)
            tmp  =4;
        else if(value < 15)
            tmp  =5;
        else if(value < 21)
            tmp  =6;
        else if(value < 27)
            tmp  =7;
        else if(value < 33)
            tmp  =8;
        else if(value < 39)
            tmp  =9;
        else if(value < 45)
            tmp  =10;
		else
            tmp  =11;
	}
    return divoom_light_env_temperature_indicator_pic[tmp];
}    

/*****************************************************************************
 Prototype    : divoom_light_env_disp_temperature_indicator
 Description  : 显示温度计
 Input        : int16 value  温度值
                uint8 flag   1:华氏温度  0:摄氏度
 Output       : None
 Return Value : 
 
  History        :
  1.Date         : 2017/11/27
    Author       : coronaa
    Modification : Created function
*****************************************************************************/
void divoom_light_env_disp_temperature_indicator(int16 value,uint8 flag)
{
    uint16 picid = divoom_light_env_temperature_indicator_get_picid(value, flag);
    divoom_disp_picture_get_rgb_data(picid, pdivoom_light_env_handle->disp_buf);
	//按照固定显示,两条杠显示-,每个数字的位数为固定的,如果有负号,占用前面的数字的位
#if(SCREEN_WIDTH_32 == 32)
	if(value <= -100)
	{
		//先画-号
        value = -value;
        divoom_disp_ctrl_set_data_xy(TEMPERATURE_LOW_ZERO_X, TEMPERATURE_LOW_ZERO_Y, &divoom_env_black_color, pdivoom_light_env_handle->disp_buf);
        divoom_disp_ctrl_set_data_xy(TEMPERATURE_LOW_ZERO_X+1, TEMPERATURE_LOW_ZERO_Y, &divoom_env_black_color, pdivoom_light_env_handle->disp_buf);

		//画百位1,7对应的是1的那个左边的点
        divoom_disp_ctrl_set_data_xy(TEMPERATURE_FIRST_X, TEMPERATURE_Y+1, &divoom_env_black_color, pdivoom_light_env_handle->disp_buf);
		//高度为7
        divoom_light_gfx_drawFastVLine(TEMPERATURE_FIRST_X+1, TEMPERATURE_Y, 7, &divoom_env_black_color, pdivoom_light_env_handle->disp_buf);

		//画十位
        divoom_disp_digit_fill_area(TEMPERATURE_SECOND_X, TEMPERATURE_Y, ((value%100)/10),
            DIVOOM_3X7_DIGIT, pdivoom_light_env_handle->disp_buf,&divoom_env_black_color);
        divoom_disp_digit_fill_area(TEMPERATURE_THIRD_X, TEMPERATURE_Y, value%10,
            DIVOOM_3X7_DIGIT, pdivoom_light_env_handle->disp_buf,&divoom_env_black_color);
	}
	else if(value > -100 && value <= -10)
	{
		//这里的话-号写在百位上面
        value = -value;
        divoom_disp_ctrl_set_data_xy(TEMPERATURE_FIRST_X, TEMPERATURE_LOW_ZERO_Y, &divoom_env_black_color, pdivoom_light_env_handle->disp_buf);
        divoom_disp_ctrl_set_data_xy(TEMPERATURE_FIRST_X+1, TEMPERATURE_LOW_ZERO_Y, &divoom_env_black_color, pdivoom_light_env_handle->disp_buf);

		//画十位
        divoom_disp_digit_fill_area(TEMPERATURE_SECOND_X, TEMPERATURE_Y, (value/10),
            DIVOOM_3X7_DIGIT, pdivoom_light_env_handle->disp_buf,&divoom_env_black_color);
        divoom_disp_digit_fill_area(TEMPERATURE_THIRD_X, TEMPERATURE_Y, value%10,
            DIVOOM_3X7_DIGIT, pdivoom_light_env_handle->disp_buf,&divoom_env_black_color);
	}
	else if(value > -10 && value < 0)
	{
        value = -value;
        divoom_disp_ctrl_set_data_xy(TEMPERATURE_SECOND_X+1, TEMPERATURE_LOW_ZERO_Y, &divoom_env_black_color, pdivoom_light_env_handle->disp_buf);
        divoom_disp_ctrl_set_data_xy(TEMPERATURE_SECOND_X+2, TEMPERATURE_LOW_ZERO_Y, &divoom_env_black_color, pdivoom_light_env_handle->disp_buf);

        divoom_disp_digit_fill_area(TEMPERATURE_THIRD_X, TEMPERATURE_Y, value%10,
            DIVOOM_3X7_DIGIT, pdivoom_light_env_handle->disp_buf,&divoom_env_black_color);
	}
	else if(value >= 0)
	{
		//容错处理
		if(value > 999)
			value = 999;
		//如果某一位为0,那么就不显示
		if(value/100 != 0)
		{
			divoom_disp_ctrl_set_data_xy(TEMPERATURE_FIRST_X, TEMPERATURE_Y+1, &divoom_env_black_color, pdivoom_light_env_handle->disp_buf);
			divoom_light_gfx_drawFastVLine(TEMPERATURE_FIRST_X+1, TEMPERATURE_Y, 7, &divoom_env_black_color, pdivoom_light_env_handle->disp_buf);
		}
		if((value%100)/10 != 0)
		{
			divoom_disp_digit_fill_area(TEMPERATURE_SECOND_X, TEMPERATURE_Y, ((value%100)/10),
					DIVOOM_3X7_DIGIT, pdivoom_light_env_handle->disp_buf,&divoom_env_black_color);
		}
		divoom_disp_digit_fill_area(TEMPERATURE_THIRD_X, TEMPERATURE_Y, value%10,
				DIVOOM_3X7_DIGIT, pdivoom_light_env_handle->disp_buf,&divoom_env_black_color);
	}
	//画*,做图片的时候,多删了,原图现在也没了,只能这样了
	divoom_disp_ctrl_set_data_xy(TEMPERATURE_POINT_1_X, TEMPERATURE_POINT_1_Y, &divoom_env_black_color, pdivoom_light_env_handle->disp_buf);
	divoom_disp_ctrl_set_data_xy(TEMPERATURE_POINT_2_X, TEMPERATURE_POINT_2_Y, &divoom_env_black_color, pdivoom_light_env_handle->disp_buf);
	divoom_disp_ctrl_set_data_xy(TEMPERATURE_POINT_3_X, TEMPERATURE_POINT_3_Y, &divoom_env_black_color, pdivoom_light_env_handle->disp_buf);
	divoom_disp_ctrl_set_data_xy(TEMPERATURE_POINT_4_X, TEMPERATURE_POINT_4_Y, &divoom_env_black_color, pdivoom_light_env_handle->disp_buf);

    if(flag)    
        divoom_disp_font_fill_area(TEMPERATURE_C_F_X, TEMPERATURE_C_F_Y, 5, 7, 
				divoom_light_letter_f_5X7_disp_info, pdivoom_light_env_handle->disp_buf, &divoom_env_black_color);
    else
        divoom_disp_font_fill_area(TEMPERATURE_C_F_X, TEMPERATURE_C_F_Y, 5, 7, 
				divoom_light_letter_c_5X7_disp_info, pdivoom_light_env_handle->disp_buf, &divoom_env_black_color);
#endif
#if 0
    if(value < -9)
    {
        value = -value;
#if(SCREEN_WIDTH_32 == 32)
        divoom_disp_ctrl_set_data_xy(12, 8, &divoom_env_black_color, pdivoom_light_env_handle->disp_buf);
        divoom_disp_ctrl_set_data_xy(12, 9, &divoom_env_black_color, pdivoom_light_env_handle->disp_buf);
        divoom_disp_ctrl_set_data_xy(13, 8, &divoom_env_black_color, pdivoom_light_env_handle->disp_buf);
        divoom_disp_ctrl_set_data_xy(13, 9, &divoom_env_black_color, pdivoom_light_env_handle->disp_buf);
        divoom_disp_ctrl_set_data_xy(14, 8, &divoom_env_black_color, pdivoom_light_env_handle->disp_buf);
        divoom_disp_ctrl_set_data_xy(14, 9, &divoom_env_black_color, pdivoom_light_env_handle->disp_buf);
        divoom_disp_ctrl_set_data_xy(15, 8, &divoom_env_black_color, pdivoom_light_env_handle->disp_buf);
        divoom_disp_ctrl_set_data_xy(15, 9, &divoom_env_black_color, pdivoom_light_env_handle->disp_buf);
        divoom_disp_digit_fill_area(17, 3,(value/10),
            DIVOOM_6X12_DIGIT, pdivoom_light_env_handle->disp_buf,&divoom_env_black_color);
        divoom_disp_digit_fill_area(25, 3,value%10,
            DIVOOM_6X12_DIGIT, pdivoom_light_env_handle->disp_buf,&divoom_env_black_color);
#else
        divoom_disp_ctrl_set_data_xy(6, 4, &divoom_env_black_color, pdivoom_light_env_handle->disp_buf);
        divoom_disp_ctrl_set_data_xy(7, 4, &divoom_env_black_color, pdivoom_light_env_handle->disp_buf);            
        divoom_disp_digit_fill_area(9, 2,(value/10),
            DIVOOM_3X5_DIGIT, pdivoom_light_env_handle->disp_buf,&divoom_env_black_color);        
        divoom_disp_digit_fill_area(13, 2,value%10,
            DIVOOM_3X5_DIGIT, pdivoom_light_env_handle->disp_buf,&divoom_env_black_color);        
#endif
    }else if(value < 0)
    {
#if(SCREEN_WIDTH_32 == 32)
        divoom_disp_ctrl_set_data_xy(12, 8, &divoom_env_black_color, pdivoom_light_env_handle->disp_buf);
        divoom_disp_ctrl_set_data_xy(12, 9, &divoom_env_black_color, pdivoom_light_env_handle->disp_buf);
        divoom_disp_ctrl_set_data_xy(13, 8, &divoom_env_black_color, pdivoom_light_env_handle->disp_buf);
        divoom_disp_ctrl_set_data_xy(13, 9, &divoom_env_black_color, pdivoom_light_env_handle->disp_buf);
        divoom_disp_ctrl_set_data_xy(14, 8, &divoom_env_black_color, pdivoom_light_env_handle->disp_buf);
        divoom_disp_ctrl_set_data_xy(14, 9, &divoom_env_black_color, pdivoom_light_env_handle->disp_buf);
        divoom_disp_ctrl_set_data_xy(15, 8, &divoom_env_black_color, pdivoom_light_env_handle->disp_buf);
        divoom_disp_ctrl_set_data_xy(15, 9, &divoom_env_black_color, pdivoom_light_env_handle->disp_buf);
        divoom_disp_digit_fill_area(20, 4,value%10,
            DIVOOM_6X12_DIGIT, pdivoom_light_env_handle->disp_buf,&divoom_env_black_color);
#else
        value = -value;
        divoom_disp_ctrl_set_data_xy(6, 3, &divoom_env_black_color, pdivoom_light_env_handle->disp_buf);
        divoom_disp_ctrl_set_data_xy(7, 3, &divoom_env_black_color, pdivoom_light_env_handle->disp_buf);            
        divoom_disp_digit_fill_area(10, 2,value%10,
            DIVOOM_3X5_DIGIT, pdivoom_light_env_handle->disp_buf,&divoom_env_black_color);  
#endif

    }else if(value < 10)
    {
#if(SCREEN_WIDTH_32 == 32)
        divoom_disp_digit_fill_area(20, 3,value%10,
            DIVOOM_6X12_DIGIT, pdivoom_light_env_handle->disp_buf,&divoom_env_black_color);
#else
        divoom_disp_digit_fill_area(10, 2,value%10,
            DIVOOM_3X5_DIGIT, pdivoom_light_env_handle->disp_buf,&divoom_env_black_color);  
#endif
    }else if(value < 100)
    {
#if(SCREEN_WIDTH_32 == 32)
        divoom_disp_digit_fill_area(14, 3,value/10,
            DIVOOM_6X12_DIGIT, pdivoom_light_env_handle->disp_buf,&divoom_env_black_color);
        divoom_disp_digit_fill_area(22, 3,value%10,
            DIVOOM_6X12_DIGIT, pdivoom_light_env_handle->disp_buf,&divoom_env_black_color);
#else
        divoom_disp_digit_fill_area(7, 2,(value/10),
            DIVOOM_3X5_DIGIT, pdivoom_light_env_handle->disp_buf,&divoom_env_black_color);        
        divoom_disp_digit_fill_area(11, 2,value%10,
            DIVOOM_3X5_DIGIT, pdivoom_light_env_handle->disp_buf,&divoom_env_black_color);            
#endif
    }else
    {
#if(SCREEN_WIDTH_32 == 32)
        divoom_disp_digit_fill_area(11, 3, 1,
            DIVOOM_6X12_DIGIT, pdivoom_light_env_handle->disp_buf,&divoom_env_black_color);
        divoom_disp_digit_fill_area(18, 3, value/10,
            DIVOOM_6X12_DIGIT, pdivoom_light_env_handle->disp_buf,&divoom_env_black_color);
        divoom_disp_digit_fill_area(25, 3,value%10,
            DIVOOM_6X12_DIGIT, pdivoom_light_env_handle->disp_buf,&divoom_env_black_color);
#else
        divoom_light_gfx_drawFastVLine(6, 2, 5, &divoom_env_black_color, pdivoom_light_env_handle->disp_buf);     
        divoom_disp_digit_fill_area(8, 2,(value/10),
            DIVOOM_3X5_DIGIT, pdivoom_light_env_handle->disp_buf,&divoom_env_black_color);        
        divoom_disp_digit_fill_area(12, 2,value%10,
            DIVOOM_3X5_DIGIT, pdivoom_light_env_handle->disp_buf,&divoom_env_black_color);
#endif
    }
    
    if(flag)    
#if(SCREEN_WIDTH_32 == 32)
        divoom_disp_font_fill_area(20, 18, 6, 10, divoom_light_letter_f_6X10_disp_info, pdivoom_light_env_handle->disp_buf, &divoom_env_black_color);
#else
        divoom_disp_font_fill_area(10,9,3,5,divoom_light_letter_f_3X5_disp_info,pdivoom_light_env_handle->disp_buf,&divoom_env_black_color);            
#endif
    else
#if(SCREEN_WIDTH_32 == 32)
        divoom_disp_font_fill_area(20, 18, 6, 10, divoom_light_letter_c_6X10_disp_info, pdivoom_light_env_handle->disp_buf, &divoom_env_black_color);
#else
        divoom_disp_font_fill_area(10,9,3,5,divoom_light_letter_c_3X5_disp_info,pdivoom_light_env_handle->disp_buf,&divoom_env_black_color);            
#endif
#endif
}


/*********************************************************************
  Function:         divoom_light_env_get_next_mode
  Description:     得到下一个显示模式
  Input:             void
  Return:           只有一个显示模式返回true,OTHER-->true
  Author:           divoom
**********************************************************************/

uint8  divoom_light_env_get_next_mode(void)
{
    uint8 i, ret = false;

    
	if(pdivoom_light_env_handle->refresh_time_flag != 0xFF)
	{
		pdivoom_light_env_handle->disp_type = pdivoom_light_env_handle->refresh_time_flag;
		return true;
	}

	//在这俩个模式只有这两种情况:要么是效果,要么是图片
	if(pdivoom_light_env_handle->mode & (1<<DIVOOM_DISP_ENV_TIME_CELS_DATA))
	{
		if(pdivoom_light_env_handle->disp_type != DIVOOM_DISP_ENV_TIME_CELS_DATA)
		{
			pdivoom_light_env_handle->disp_type = DIVOOM_DISP_ENV_TIME_CELS_DATA;
		}
		else if(pdivoom_light_env_handle->mode & (1<<DIVOOM_DISP_ENV_NEW_PIC))
		{
			pdivoom_light_env_handle->disp_type = DIVOOM_DISP_ENV_NEW_PIC;
		}
        else
        {
            pdivoom_light_env_handle->disp_type = DIVOOM_DISP_ENV_TIME_CELS_DATA;
        }
		return true;
	}
	else if(pdivoom_light_env_handle->mode & (1<<DIVOOM_DISP_ENV_TIME_CELS_DATA_WEATHER))
	{
		if(pdivoom_light_env_handle->disp_type != DIVOOM_DISP_ENV_TIME_CELS_DATA_WEATHER)
		{
			pdivoom_light_env_handle->disp_type = DIVOOM_DISP_ENV_TIME_CELS_DATA_WEATHER;
		}
		else if(pdivoom_light_env_handle->mode & (1<<DIVOOM_DISP_ENV_NEW_PIC))
		{
			pdivoom_light_env_handle->disp_type = DIVOOM_DISP_ENV_NEW_PIC;
		}
        else
        {
            pdivoom_light_env_handle->disp_type = DIVOOM_DISP_ENV_TIME_CELS_DATA_WEATHER;
        }
		return true;
	}

    
    for(i = pdivoom_light_env_handle->disp_type + 1; i < DIVOOM_DISP_ENV_END_TYPE; i ++)
    {
        if(i == DIVOOM_DISP_ENV_CELS && pdivoom_light_env_handle->celsius_value == T_S8_MAX)
        {
            continue;
        }
        if(i == DIVOOM_DISP_ENV_WEATHER && pdivoom_light_env_handle->weather_type == T_U8_MAX)
        {
            continue;
        }
        if(pdivoom_light_env_handle->mode & (1 << i))
        {
            pdivoom_light_env_handle->disp_type = i;
            return true;
        }
    }
    for(i = 0; i < pdivoom_light_env_handle->disp_type + 1; i ++)
    {
        if(i == DIVOOM_DISP_ENV_CELS && pdivoom_light_env_handle->celsius_value == T_S8_MAX)
        {
            continue;
        }
        if(i == DIVOOM_DISP_ENV_WEATHER && pdivoom_light_env_handle->weather_type == T_U8_MAX)
        {
            continue;
        }
        if(pdivoom_light_env_handle->mode & (1 << i))
        {
            if(pdivoom_light_env_handle->disp_type == i)
            {
                ret = true;
            }
            pdivoom_light_env_handle->disp_type = i;
            return true;
        }
    }
    //pdivoom_light_env_handle->disp_type = DIVOOM_DISP_ENV_TIME;
    return ret;
}

void  divoom_light_env_celsius_pic(int value)
{
    divoom_light_color_info fill_color = {255,255,0}, back_color = {255,255,255};

    memset(pdivoom_light_env_handle->disp_buf, 0, SCREEN_BUF_SIZE_32);
    if(value >= 0)
    {
        if(value >= 10)
        {
            divoom_light_env_fill_mode4(2, value/10, &fill_color, &back_color);
        }
        divoom_light_env_fill_mode4(6, value%10, &fill_color, &back_color);
    }
    else
    {
        value = 0 - value;
        if(value >= 10)
        {
            divoom_light_env_fill_mode4(1, value/10, &fill_color, &back_color);
        }
        divoom_light_env_fill_mode4(5, value%10, &fill_color, &back_color);
        divoom_disp_ctrl_set_dot(9 , 3,&fill_color, pdivoom_light_env_handle->disp_buf);
        divoom_disp_ctrl_set_dot(10 , 3,&fill_color, pdivoom_light_env_handle->disp_buf);
    }
    divoom_light_env_fill_area(12, 1, 3, 4,0,divoom_light_env_celsius_pic_sep, pdivoom_light_env_handle->disp_buf, &fill_color);
}

/*********************************************************************
  Function:         divoom_light_env_celsius
  Description:     显示摄温度
  Input:             refresh_flag       是否强制刷新
  Return:           NONE
  Author:           divoom
**********************************************************************/

void  divoom_light_env_celsius(bool refresh_flag)
{
  //  uint8 i, start_x = DIVOOM_DISP_ENV_DISP_START_X;
//    uint16 disp_value;
    int celsius_value = pdivoom_light_env_handle->celsius_value;
    
    pdivoom_light_env_handle->penv_read(DIVOOM_DISP_ENV_CELS, (void *)&celsius_value);
    if((celsius_value != pdivoom_light_env_handle->celsius_value))
    {
        refresh_flag = true;
    }
    pdivoom_light_env_handle->celsius_value = celsius_value;
    if(refresh_flag == false)
    {
        return;
    }

    divoom_light_env_disp_temperature_indicator(celsius_value,0);

/*    
    if(pdivoom_light_env_handle->effect_type == 3)
    {
        divoom_light_env_celsius_pic(celsius_value);
    }
    else if(pdivoom_light_env_handle->effect_type == DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_SQUARE)
    {
        divoom_disp_ctrl_fill(pdivoom_light_env_handle->disp_buf, divoom_light_env_get_back_color());
        if(pdivoom_light_env_handle->celsius_value < 0)
        {
            disp_value = 0 - pdivoom_light_env_handle->celsius_value;
            divoom_disp_ctrl_set_dot(5, 10 ,divoom_light_env_get_fill_color() ,pdivoom_light_env_handle->disp_buf);
            divoom_disp_ctrl_set_dot(6, 10 ,divoom_light_env_get_fill_color() ,pdivoom_light_env_handle->disp_buf);
        }
        else
        {
            disp_value = pdivoom_light_env_handle->celsius_value;
        }
        divoom_light_env_disp_digit(5, 2, disp_value / 10, false, pdivoom_light_env_handle->disp_buf);
        divoom_light_env_disp_digit(9, 2, disp_value % 10, false, pdivoom_light_env_handle->disp_buf);
        divoom_light_env_fill_area(7, 8, 5,5,0,divoom_light_env_celsius2_sep, pdivoom_light_env_handle->disp_buf, NULL);
    }
    else if(pdivoom_light_env_handle->effect_type == DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_CIRCULAR)
    {
        divoom_light_env_disp_temperature_indicator(celsius_value,0);
    }     
    else
    {
        divoom_disp_ctrl_fill(pdivoom_light_env_handle->disp_buf, divoom_light_env_get_back_color());
        if(pdivoom_light_env_handle->celsius_value < 0)
        {
            disp_value = 0 - pdivoom_light_env_handle->celsius_value;
            for(i = 0; i < 3; i ++)
            {
                divoom_disp_ctrl_set_dot(DIVOOM_DISP_ENV_DISP_START_X + i, DIVOOM_DISP_ENV_DISP_START_Y + 2, divoom_light_env_get_fill_color(), pdivoom_light_env_handle->disp_buf);
            }
            start_x = DIVOOM_DISP_ENV_DIGIT_SIZE_X + 1;
            if(disp_value < 10)
            {
                start_x = divoom_light_env_disp_digit(start_x, DIVOOM_DISP_ENV_DISP_START_Y, 0, false, pdivoom_light_env_handle->disp_buf);
            }
            divoom_light_env_disp_digit(start_x, DIVOOM_DISP_ENV_DISP_START_Y, disp_value, false, pdivoom_light_env_handle->disp_buf);
        }
        else
        {
            disp_value = pdivoom_light_env_handle->celsius_value;
            divoom_light_env_disp_digit(0, DIVOOM_DISP_ENV_DISP_START_Y, disp_value, true, pdivoom_light_env_handle->disp_buf);
        }

        divoom_light_env_fill_area(SCREEN_WIDTH_32 - DIVOOM_DISP_ENV_DIGIT_SIZE_X, DIVOOM_DISP_ENV_DISP_START_Y, DIVOOM_DISP_ENV_DIGIT_SIZE_X,DIVOOM_DISP_ENV_DIGIT_SIZE_Y,
             0,divoom_light_env_celsius_sep, pdivoom_light_env_handle->disp_buf, NULL);
    }
*/    

}

bool  divoom_light_env_fahrenheit_sep2(uint8 x, uint8 y, uint8 user_data)
{
    return fahrenheit_disp_sep2[y] & (1 << x);
}

bool  divoom_light_env_fahrenheit_sep(uint8 x, uint8 y, uint8 user_data)
{
    return fahrenheit_disp_sep[y] & (1 << x);
}
bool  divoom_light_env_fahrenheit_pic_sep(uint8 x, uint8 y, uint8 user_data)
{
    return fahrenheit_disp_sep[y] & (1 << x);
}
void  divoom_light_env_fahrenheit_pic(uint16 value)
{
    divoom_light_color_info fill_color = {255,255,0}, back_color = {255,255,255};

    memset(pdivoom_light_env_handle->disp_buf, 0, SCREEN_BUF_SIZE_32);
    if(value >= 100)
    {
        divoom_light_env_fill_mode4(0, value/100, &fill_color, &back_color);
        value = value % 100;
        divoom_light_env_fill_mode4(4, value/10, &fill_color, &back_color);
    }
    else if(value >= 10)
    {
        divoom_light_env_fill_mode4(4, value/10, &fill_color, &back_color);
    }
    divoom_light_env_fill_mode4(8, value%10, &fill_color, &back_color);
    divoom_light_env_fill_area(12, 1, 3, 5,0,divoom_light_env_fahrenheit_pic_sep, pdivoom_light_env_handle->disp_buf, &fill_color);
}

/*********************************************************************
  Function:         divoom_light_env_fahrenheit
  Description:     显示华温度
  Input:             refresh_flag       是否强制刷新
  Return:           NONE
  Author:           divoom
**********************************************************************/

void  divoom_light_env_fahrenheit(bool refresh_flag)
{
    //uint8 i, start_x;
    int fahrenheit_value = pdivoom_light_env_handle->fahrenheit_value;
	//不读的话可能永远都是之前的温度值
    pdivoom_light_env_handle->penv_read(DIVOOM_DISP_ENV_CELS, (void *)&pdivoom_light_env_handle->celsius_value);

    fahrenheit_value = pdivoom_light_env_handle->celsius_value *18/10 + 32;

    if(fahrenheit_value > 199)
    {
        fahrenheit_value = 199;
    }
    //if(fahrenheit_value < 0)
    //{
    //    fahrenheit_value = 0;
    //}
    
    if(fahrenheit_value != pdivoom_light_env_handle->fahrenheit_value)
    {
        refresh_flag = true;
    }
    pdivoom_light_env_handle->fahrenheit_value = fahrenheit_value;
    if(refresh_flag == false)
    {
        return;
    }
    divoom_light_env_disp_temperature_indicator(fahrenheit_value,1);    
/*
    if(pdivoom_light_env_handle->effect_type == 3)
    {
        divoom_light_env_fahrenheit_pic(fahrenheit_value);
    }
    else if(pdivoom_light_env_handle->effect_type == DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_SQUARE)
    {
        divoom_disp_ctrl_fill(pdivoom_light_env_handle->disp_buf, divoom_light_env_get_back_color());
        if(fahrenheit_value > 99)
        {
            fahrenheit_value -= 100;
            for(i = 0; i < DIVOOM_DISP_ENV_DIGIT_SIZE_Y; i++)
            {
                divoom_disp_ctrl_set_dot(3, 2 + i, divoom_light_env_get_fill_color(), pdivoom_light_env_handle->disp_buf);
            }
        }
        divoom_light_env_disp_digit(5,2 ,fahrenheit_value/10, false, pdivoom_light_env_handle->disp_buf);
        divoom_light_env_disp_digit(9,2 ,fahrenheit_value%10, false, pdivoom_light_env_handle->disp_buf);
        divoom_light_env_fill_area(7, 9 ,5,5,
            fahrenheit_value,divoom_light_env_fahrenheit_sep2, pdivoom_light_env_handle->disp_buf, NULL);
    }
    else if(pdivoom_light_env_handle->effect_type == DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_CIRCULAR)
    {
        divoom_light_env_disp_temperature_indicator(fahrenheit_value,1);
    }        
    else
    {    
        divoom_disp_ctrl_fill(pdivoom_light_env_handle->disp_buf, divoom_light_env_get_back_color());

        if(fahrenheit_value >= 100)
        {
            for(i = 0; i < DIVOOM_DISP_ENV_DIGIT_SIZE_Y; i++)
            {
                divoom_disp_ctrl_set_dot(DIVOOM_DISP_ENV_DISP_START_X + 1, DIVOOM_DISP_ENV_DISP_START_Y + i, divoom_light_env_get_fill_color(), pdivoom_light_env_handle->disp_buf);
            }
            fahrenheit_value -= 100;
        }
        start_x = DIVOOM_DISP_ENV_DISP_START_X + 3;
        if(fahrenheit_value >= 10)
        {
            divoom_light_env_disp_digit(start_x,DIVOOM_DISP_ENV_DISP_START_Y ,fahrenheit_value/10, false, pdivoom_light_env_handle->disp_buf);
        }
        if(fahrenheit_value < 10 && pdivoom_light_env_handle->fahrenheit_value >= 100)
        {
            divoom_light_env_disp_digit(start_x,DIVOOM_DISP_ENV_DISP_START_Y ,0, false, pdivoom_light_env_handle->disp_buf);
        }
        start_x = DIVOOM_DISP_ENV_DISP_START_X + 3 + DIVOOM_DISP_ENV_DIGIT_SIZE_X + 1;
        divoom_light_env_disp_digit(start_x,DIVOOM_DISP_ENV_DISP_START_Y ,fahrenheit_value%10, false, pdivoom_light_env_handle->disp_buf);
        
        divoom_light_env_fill_area(SCREEN_WIDTH_32 - 4, DIVOOM_DISP_ENV_DISP_START_Y ,DIVOOM_DISP_ENV_DIGIT_SIZE_X,DIVOOM_DISP_ENV_DIGIT_SIZE_Y,
            fahrenheit_value,divoom_light_env_fahrenheit_sep, pdivoom_light_env_handle->disp_buf, NULL);
    }
*/    

}

bool  divoom_light_env_humidity_sep(uint8 x, uint8 y, uint8 user_data)
{
    return humidity_disp_sep[y] & (1 << x);
}

bool  divoom_light_env_humidity_pic_sep(uint8 x, uint8 y, uint8 user_data)
{
    return humidity_pic_disp_sep[y] & (1 << x);
}

void  divoom_light_env_humidity_pic(uint8 value)
{
    divoom_light_color_info fill_color = {0,0,255}, back_color = {255,255,255};

    memset(pdivoom_light_env_handle->disp_buf, 0, SCREEN_BUF_SIZE_32);
    if(value >= 10)
    {
        divoom_light_env_fill_mode4(2, value/10, &fill_color, &back_color);
    }
    divoom_light_env_fill_mode4(6, value%10, &fill_color, &back_color);
    divoom_light_env_fill_area(11, 1, 4, 4,0,divoom_light_env_humidity_pic_sep, pdivoom_light_env_handle->disp_buf, &fill_color);
}


/*********************************************************************
  Function:         divoom_light_env_humidity
  Description:     显示湿度
  Input:             refresh_flag       是否强制刷新
  Return:           NONE
  Author:           divoom
**********************************************************************/

void  divoom_light_env_humidity(bool refresh_flag)
{
    uint16 humidity_value = pdivoom_light_env_handle->humidity_value;
    
    pdivoom_light_env_handle->penv_read(DIVOOM_DISP_ENV_HUMI, (void *)&humidity_value);

    if(humidity_value >= 100)
    {
        humidity_value = 99;
    }
    if(humidity_value != pdivoom_light_env_handle->humidity_value)
    {
        refresh_flag = true;
    }
    pdivoom_light_env_handle->humidity_value = humidity_value;
    if(refresh_flag == false)
    {
        return;
    }
    
    divoom_disp_ctrl_fill(pdivoom_light_env_handle->disp_buf, divoom_light_env_get_back_color());

    if(pdivoom_light_env_handle->effect_type == 3)
    {
        divoom_light_env_humidity_pic(humidity_value);
    }
    else if(pdivoom_light_env_handle->effect_type == 2)
    {
        divoom_light_env_disp_digit(5, 2 ,humidity_value/10, false, pdivoom_light_env_handle->disp_buf);
        divoom_light_env_disp_digit(9, 2 ,humidity_value%10, false, pdivoom_light_env_handle->disp_buf);
        divoom_light_env_fill_area(7, 8, 5,
            5, 0,divoom_light_env_humidity_sep, pdivoom_light_env_handle->disp_buf, NULL);
    }
    else
    {
        if(humidity_value >= 10)
        {
            divoom_light_env_disp_digit(1, DIVOOM_DISP_ENV_DISP_START_Y ,humidity_value/10, false, pdivoom_light_env_handle->disp_buf);
        }
        divoom_light_env_disp_digit(1 + DIVOOM_DISP_ENV_DIGIT_SIZE_X + 1, DIVOOM_DISP_ENV_DISP_START_Y ,humidity_value%10, false, pdivoom_light_env_handle->disp_buf);
            
        divoom_light_env_fill_area(SCREEN_WIDTH_32 - 5, DIVOOM_DISP_ENV_DISP_START_Y, 5,
            5, 0,divoom_light_env_humidity_sep, pdivoom_light_env_handle->disp_buf, NULL);
    }
    
}

bool  divoom_light_env_bright_sep_digit(uint8 x, uint8 y, uint8 user_data)
{
    return bright_disp_sep[y] & (1 << x);
}

bool  divoom_light_env_noise_sep_digit(uint8 x, uint8 y, uint8 user_data)
{
    return noise_disp_sep[y] & (1 << x);
}
bool  divoom_light_env_noise_pic_sep_digit(uint8 x, uint8 y, uint8 user_data)
{
    return noise_pic_disp_sep[y] & (1 << x);
}

void  divoom_light_env_noise_pic(uint8 value)
{
    divoom_light_color_info fill_color = {255,0,255}, back_color = {255,255,255};

    memset(pdivoom_light_env_handle->disp_buf, 0, SCREEN_BUF_SIZE_32);
    if(value >= 10)
    {
        divoom_light_env_fill_mode4(2, value/10, &fill_color, &back_color);
    }
    divoom_light_env_fill_mode4(6, value%10, &fill_color, &back_color);
    divoom_light_env_fill_area(10, 1, 5, 3,0,divoom_light_env_noise_pic_sep_digit, pdivoom_light_env_handle->disp_buf, &fill_color);
}
#if 0
/*********************************************************************
  Function:         divoom_light_env_noise
  Description:     显示噪音
  Input:             refresh_flag       是否强制刷新
  Return:           NONE
  Author:           divoom
**********************************************************************/

void  divoom_light_env_noise(bool refresh_flag)
{
    uint16 noise_value = pdivoom_light_env_handle->noise_value;
    
    

    if(noise_value > 99)
    {
        noise_value = 99;
    }
    if(noise_value != pdivoom_light_env_handle->noise_value)
    {
        refresh_flag = true;
    }
    pdivoom_light_env_handle->noise_value = noise_value;
    if(refresh_flag == false)
    {
        return;
    }


    if(pdivoom_light_env_handle->effect_type == 3)
    {
        divoom_light_env_noise_pic(noise_value);
    }
    else if(pdivoom_light_env_handle->effect_type == 2)
    {
        divoom_disp_ctrl_fill(pdivoom_light_env_handle->disp_buf, divoom_light_env_get_back_color());
        divoom_light_env_disp_digit(5, 2 ,noise_value/10, false, pdivoom_light_env_handle->disp_buf);
        divoom_light_env_disp_digit(9, 2 ,noise_value%10, false, pdivoom_light_env_handle->disp_buf);       
        divoom_light_env_fill_area(5, 8, 7, 5,0,divoom_light_env_noise_sep_digit, pdivoom_light_env_handle->disp_buf, NULL);
    }
    else
    {
        divoom_disp_ctrl_fill(pdivoom_light_env_handle->disp_buf, divoom_light_env_get_back_color());
        if(noise_value >= 10)
        {
            divoom_light_env_disp_digit(DIVOOM_DISP_ENV_DISP_START_X, DIVOOM_DISP_ENV_DISP_START_Y ,noise_value/10, false, pdivoom_light_env_handle->disp_buf);
        }
        divoom_light_env_disp_digit(DIVOOM_DISP_ENV_DISP_START_X + DIVOOM_DISP_ENV_DIGIT_SIZE_X + 1, DIVOOM_DISP_ENV_DISP_START_Y ,noise_value%10, false, pdivoom_light_env_handle->disp_buf);
       
        divoom_light_env_fill_area(SCREEN_WIDTH_32 - 7, DIVOOM_DISP_ENV_DISP_START_Y, 7, 5,0,divoom_light_env_noise_sep_digit, pdivoom_light_env_handle->disp_buf, NULL);
    }
}
#endif
bool  divoom_light_env_time_active(uint8 x, uint8 y, uint8 user_data)
{
    return (divoom_env_dight_small_disp_Info[user_data][y] & (1 << x));
}

void divoom_light_env_time_pic(Pdivoom_light_env_time_info time_info)
{
    uint8 i;
    uint8 hour_pos[12][2] = {{7,5},{8,5},{9,6},{9,7},{9,8},{8,9},{7,9},{6,9},{5,8},{5,7},{5,6},{6,5}};
    uint8 min_pos[12][2] = {{7,1},{10,2},{12,4},{13,7},{12,10},{10,12},{7,13},{4,12},{2,10},{1,7},{2,4},{4,2}};
    divoom_light_color_info min_groud_color = {0,255,0}, min_active_color = {255,255,0}, hour_active_color = {255,255,255}, hour_groud_color = {255,0,255};

    memset(pdivoom_light_env_handle->disp_buf, 0, SCREEN_BUF_SIZE_32);
    
    if(pdivoom_light_env_handle->tick & 1)
    {
        divoom_disp_ctrl_set_dot(7, 7, &hour_groud_color, pdivoom_light_env_handle->disp_buf);
    }

    for(i = 0; i < 12; i ++)
    {
        if((time_info->hour%12) == i)
        {
            divoom_disp_ctrl_set_dot(hour_pos[i][0], hour_pos[i][1], &hour_active_color, pdivoom_light_env_handle->disp_buf);
        }
        else
        {
            divoom_disp_ctrl_set_dot(hour_pos[i][0], hour_pos[i][1], &hour_groud_color, pdivoom_light_env_handle->disp_buf);
        }
        if(((time_info->min + 1)/5 % 12) == i)
        {
            divoom_disp_ctrl_set_dot(min_pos[i][0], min_pos[i][1], &min_active_color, pdivoom_light_env_handle->disp_buf);
        }
        else
        {
            divoom_disp_ctrl_set_dot(min_pos[i][0], min_pos[i][1], &min_groud_color, pdivoom_light_env_handle->disp_buf);
        }
    }
}




#if(SCREEN_WIDTH_32 == 32)
//#define DIVOOM_LIGHT_ENV_CLOCK_HOUR(a) DIVOOM_DISP_DIVOOM_TIME_TYPE_SQURE_CLOCK_HOUR_##a
//#define DIVOOM_LIGHT_ENV_CLOCK_MINUTE(a) DIVOOM_DISP_DIVOOM_TIME_TYPE_SQURE_CLOCK_MINUTE_##a
const uint16 divoom_light_env_clock_hour[12] = 
{
	DIVOOM_DISP_DIVOOM_TIME_TYPE_SQURE_CLOCK_HOUR_00,
	DIVOOM_DISP_DIVOOM_TIME_TYPE_SQURE_CLOCK_HOUR_01,
	DIVOOM_DISP_DIVOOM_TIME_TYPE_SQURE_CLOCK_HOUR_02,
	DIVOOM_DISP_DIVOOM_TIME_TYPE_SQURE_CLOCK_HOUR_03,
	DIVOOM_DISP_DIVOOM_TIME_TYPE_SQURE_CLOCK_HOUR_04,
	DIVOOM_DISP_DIVOOM_TIME_TYPE_SQURE_CLOCK_HOUR_05,
	DIVOOM_DISP_DIVOOM_TIME_TYPE_SQURE_CLOCK_HOUR_06,
	DIVOOM_DISP_DIVOOM_TIME_TYPE_SQURE_CLOCK_HOUR_07,
	DIVOOM_DISP_DIVOOM_TIME_TYPE_SQURE_CLOCK_HOUR_08,
	DIVOOM_DISP_DIVOOM_TIME_TYPE_SQURE_CLOCK_HOUR_09,
	DIVOOM_DISP_DIVOOM_TIME_TYPE_SQURE_CLOCK_HOUR_10,
	DIVOOM_DISP_DIVOOM_TIME_TYPE_SQURE_CLOCK_HOUR_11,
};
const uint16 divoom_light_env_clock_minute[20] = 
{
	DIVOOM_DISP_DIVOOM_TIME_TYPE_SQURE_CLOCK_MINUTE_00,
	DIVOOM_DISP_DIVOOM_TIME_TYPE_SQURE_CLOCK_MINUTE_01,
	DIVOOM_DISP_DIVOOM_TIME_TYPE_SQURE_CLOCK_MINUTE_02,
	DIVOOM_DISP_DIVOOM_TIME_TYPE_SQURE_CLOCK_MINUTE_03,
	DIVOOM_DISP_DIVOOM_TIME_TYPE_SQURE_CLOCK_MINUTE_04,
	DIVOOM_DISP_DIVOOM_TIME_TYPE_SQURE_CLOCK_MINUTE_05,
	DIVOOM_DISP_DIVOOM_TIME_TYPE_SQURE_CLOCK_MINUTE_06,
	DIVOOM_DISP_DIVOOM_TIME_TYPE_SQURE_CLOCK_MINUTE_07,
	DIVOOM_DISP_DIVOOM_TIME_TYPE_SQURE_CLOCK_MINUTE_08,
	DIVOOM_DISP_DIVOOM_TIME_TYPE_SQURE_CLOCK_MINUTE_09,
	DIVOOM_DISP_DIVOOM_TIME_TYPE_SQURE_CLOCK_MINUTE_10,
	DIVOOM_DISP_DIVOOM_TIME_TYPE_SQURE_CLOCK_MINUTE_11,
};
#else
const uint16 divoom_light_env_clock_hour[12] = 
{
    DIVOOM_DISP_CLOCK_HOUR_00,
    DIVOOM_DISP_CLOCK_HOUR_01,
    DIVOOM_DISP_CLOCK_HOUR_02,
    DIVOOM_DISP_CLOCK_HOUR_03,
    DIVOOM_DISP_CLOCK_HOUR_04,
    DIVOOM_DISP_CLOCK_HOUR_05,
    DIVOOM_DISP_CLOCK_HOUR_06,
    DIVOOM_DISP_CLOCK_HOUR_07,
    DIVOOM_DISP_CLOCK_HOUR_08,
    DIVOOM_DISP_CLOCK_HOUR_09,
    DIVOOM_DISP_CLOCK_HOUR_10,
    DIVOOM_DISP_CLOCK_HOUR_11,
};
const uint16 divoom_light_env_clock_minute[20] = 
{
    DIVOOM_DISP_CLOCK_MINUTE_00,
    DIVOOM_DISP_CLOCK_MINUTE_01,
    DIVOOM_DISP_CLOCK_MINUTE_02,
    DIVOOM_DISP_CLOCK_MINUTE_03,
    DIVOOM_DISP_CLOCK_MINUTE_04,
    DIVOOM_DISP_CLOCK_MINUTE_05,
    DIVOOM_DISP_CLOCK_MINUTE_06,
    DIVOOM_DISP_CLOCK_MINUTE_07,
    DIVOOM_DISP_CLOCK_MINUTE_08,
    DIVOOM_DISP_CLOCK_MINUTE_09,
    DIVOOM_DISP_CLOCK_MINUTE_10,
    DIVOOM_DISP_CLOCK_MINUTE_11,
    DIVOOM_DISP_CLOCK_MINUTE_12,
    DIVOOM_DISP_CLOCK_MINUTE_13,
    DIVOOM_DISP_CLOCK_MINUTE_14,
    DIVOOM_DISP_CLOCK_MINUTE_15,
    DIVOOM_DISP_CLOCK_MINUTE_16,
    DIVOOM_DISP_CLOCK_MINUTE_17,
    DIVOOM_DISP_CLOCK_MINUTE_18,
    DIVOOM_DISP_CLOCK_MINUTE_19,
};


const uint8 divoom_light_env_time_pic_box[SCREEN_HEIGHT_32][SCREEN_WIDTH_32] =
{
   { 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0 },
   { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
   { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
   { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
   { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
   { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
   { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
   { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
   { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
   { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
   { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
   { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
   { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
   { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
   { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
   { 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0 }
};
#endif

//要修改为三张图片显示
void divoom_light_env_time_pic_mode2(Pdivoom_light_env_time_info time_info)
{
    uint8 hour,min;
	uint16 i;
#if(SCREEN_WIDTH_32 == 16)
    uint16 j,off;
#endif
    hour = time_info->hour%12; //防越界
#if(SCREEN_WIDTH_32 == 32)
    min = (time_info->min/5) % 12; //防越界
#else
    min = (time_info->min/3) % 20; //防越界
#endif
    
    //divoom_print("hour:%d,min:%d\n", hour, min);
    
    divoom_disp_picture_get_rgb_data(divoom_light_env_clock_hour[hour], pdivoom_light_env_handle->disp_buf);
    divoom_disp_picture_get_rgb_data(divoom_light_env_clock_minute[min], pdivoom_light_env_handle->bak_buf);
    for(i = 0; i < SCREEN_BUF_SIZE_32; i++)
    {
        if(pdivoom_light_env_handle->bak_buf[i])
            pdivoom_light_env_handle->disp_buf[i] = pdivoom_light_env_handle->bak_buf[i];
    }

#if(SCREEN_WIDTH_32 == 16)
    for(j = 0,off = 0; j < SCREEN_HEIGHT_32; j++)
    {
        for(i = 0 ; i < SCREEN_WIDTH_32; i++,off++)
        {
            if(divoom_light_env_time_pic_box[j][i])
            {
                divoom_disp_ctrl_set_data_xy(i, j, &pdivoom_light_env_handle->color, pdivoom_light_env_handle->disp_buf);            
            }
        }
    }
#endif
}

//显示金色表盘和蓝色表盘,根据模式选择
//先读到bak,再轮训写到disp
void divoom_disp_time_disp_dial_time(Pdivoom_light_env_time_info time_info, uint8 mode)
{
    uint8 hour_index, min_index;
	int i, j;
	divoom_screen_color_info cur_color;
	uint8 len_min, len_hour;

	if(mode == DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_GOLD_BACKGROUND)
	{
		len_min = sizeof(divoom_light_env_gold_minute)/sizeof(divoom_light_env_gold_minute[0]);
		len_hour = sizeof(divoom_light_env_gold_hour)/sizeof(divoom_light_env_gold_hour[0]);
	}
	else
	{
		len_min = sizeof(divoom_light_env_blue_colourful_minute)/sizeof(divoom_light_env_blue_colourful_minute[0]);
		len_hour = sizeof(divoom_light_env_blue_colourful_hour)/sizeof(divoom_light_env_blue_colourful_hour[0]);
	}

    min_index = (time_info->min + 2) / 5 % len_min;
    if(time_info->min >= 30)
    {
        hour_index = (time_info->hour + 1) % len_hour;
    }
    else
    {
        hour_index = time_info->hour % len_hour;
    }

	memset(pdivoom_light_env_handle->bak_buf, 0, sizeof(pdivoom_light_env_handle->bak_buf));
	if(mode == DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_GOLD_BACKGROUND)
	{
		divoom_disp_picture_get_rgb_data(divoom_light_env_gold_hour[hour_index], pdivoom_light_env_handle->bak_buf);
	}
	else
	{
		divoom_disp_picture_get_rgb_data(divoom_light_env_blue_colourful_hour[hour_index], pdivoom_light_env_handle->bak_buf);
	}

	for(i = 0; i < SCREEN_WIDTH_32; i ++)
	{
		for(j = 0; j < SCREEN_HEIGHT_32; j ++)
		{
			divoom_disp_ctrl_get_dot(i, j, &cur_color, pdivoom_light_env_handle->bak_buf);
			if(cur_color.r_c != 0 || cur_color.g_c != 0 || cur_color.b_c != 0)
			{
				divoom_disp_ctrl_set_data_xy(i, j, &cur_color, pdivoom_light_env_handle->disp_buf);
			}
		}
	}

	//因为蓝色表盘的是背景和时针在一起的,所以这里分针放在下面
	memset(pdivoom_light_env_handle->bak_buf, 0, sizeof(pdivoom_light_env_handle->bak_buf));
	if(mode == DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_GOLD_BACKGROUND)
	{
		divoom_disp_picture_get_rgb_data(divoom_light_env_gold_minute[min_index], pdivoom_light_env_handle->bak_buf);
	}
	else
	{
		divoom_disp_picture_get_rgb_data(divoom_light_env_blue_colourful_minute[min_index], pdivoom_light_env_handle->bak_buf);
	}

	for(i = 0; i < SCREEN_WIDTH_32; i ++)
	{
		for(j = 0; j < SCREEN_HEIGHT_32; j ++)
		{
			divoom_disp_ctrl_get_dot(i, j, &cur_color, pdivoom_light_env_handle->bak_buf);
			if(cur_color.r_c != 0 || cur_color.g_c != 0 || cur_color.b_c != 0)
			{
				divoom_disp_ctrl_set_data_xy(i, j, &cur_color, pdivoom_light_env_handle->disp_buf);
			}
		}
	}

}

//得到是几点,从表里面拿出来相应的图,读取5*14大小(从0,0开始),然后读到的数据在放到display_buf里面
//需要读取的文件长度是5*14*3的大小,写的数据也是这么大
void divoom_disp_time_disp_italic_time(Pdivoom_light_env_time_info time_info)
{
	int i, j;
	divoom_light_color_info cur_color;
	memset(pdivoom_light_env_handle->bak_buf, 0, sizeof(pdivoom_light_env_handle->bak_buf));
	divoom_disp_picture_get_rgb_data(divoom_light_env_italic_nums[time_info->hour/10], pdivoom_light_env_handle->bak_buf);
	for(i=1;i<DIVOOM_DISP_ENV_ITALIC_W+1;i++)
	{
		for(j=0;j<DIVOOM_DISP_ENV_ITALIC_H;j++)
		{
			divoom_disp_ctrl_get_dot(i, j,&cur_color, pdivoom_light_env_handle->bak_buf);
			if(cur_color.r_c != 0 || cur_color.g_c != 0 || cur_color.b_c != 0)
			{
				divoom_disp_ctrl_set_data_xy(i+DIVOOM_DISP_ENV_ITALIC_X_1 - 1, j+DIVOOM_DISP_ENV_ITALIC_Y, &cur_color, pdivoom_light_env_handle->disp_buf);
			}
		}
	}
	memset(pdivoom_light_env_handle->bak_buf, 0, sizeof(pdivoom_light_env_handle->bak_buf));
	divoom_disp_picture_get_rgb_data(divoom_light_env_italic_nums[time_info->hour%10], pdivoom_light_env_handle->bak_buf);
	for(i=1;i<DIVOOM_DISP_ENV_ITALIC_W+1;i++)
	{
		for(j=0;j<DIVOOM_DISP_ENV_ITALIC_H;j++)
		{
			divoom_disp_ctrl_get_dot(i, j,&cur_color, pdivoom_light_env_handle->bak_buf);
			if(cur_color.r_c != 0 || cur_color.g_c != 0 || cur_color.b_c != 0)
			{
				divoom_disp_ctrl_set_data_xy(i+DIVOOM_DISP_ENV_ITALIC_X_2 - 1, j+DIVOOM_DISP_ENV_ITALIC_Y, &cur_color, pdivoom_light_env_handle->disp_buf);
			}
		}
	}
	memset(pdivoom_light_env_handle->bak_buf, 0, sizeof(pdivoom_light_env_handle->bak_buf));
	divoom_disp_picture_get_rgb_data(divoom_light_env_italic_nums[time_info->min/10], pdivoom_light_env_handle->bak_buf);
	for(i=1;i<DIVOOM_DISP_ENV_ITALIC_W+1;i++)
	{
		for(j=0;j<DIVOOM_DISP_ENV_ITALIC_H;j++)
		{
			divoom_disp_ctrl_get_dot(i, j,&cur_color, pdivoom_light_env_handle->bak_buf);
			if(cur_color.r_c != 0 || cur_color.g_c != 0 || cur_color.b_c != 0)
			{
				divoom_disp_ctrl_set_data_xy(i+DIVOOM_DISP_ENV_ITALIC_X_3 - 1, j+DIVOOM_DISP_ENV_ITALIC_Y, &cur_color, pdivoom_light_env_handle->disp_buf);
			}
		}
	}
	memset(pdivoom_light_env_handle->bak_buf, 0, sizeof(pdivoom_light_env_handle->bak_buf));
	divoom_disp_picture_get_rgb_data(divoom_light_env_italic_nums[time_info->min%10], pdivoom_light_env_handle->bak_buf);
	for(i=1;i<DIVOOM_DISP_ENV_ITALIC_W+1;i++)
	{
		for(j=0;j<DIVOOM_DISP_ENV_ITALIC_H;j++)
		{
			divoom_disp_ctrl_get_dot(i, j,&cur_color, pdivoom_light_env_handle->bak_buf);
			if(cur_color.r_c != 0 || cur_color.g_c != 0 || cur_color.b_c != 0)
			{
				divoom_disp_ctrl_set_data_xy(i+DIVOOM_DISP_ENV_ITALIC_X_4 - 1, j+DIVOOM_DISP_ENV_ITALIC_Y, &cur_color, pdivoom_light_env_handle->disp_buf);
			}
		}
	}
}

//显示数字部分
void divoom_disp_time_disp_led_time(uint8 *disp_buf,Pdivoom_light_env_time_info time_info, uint8 dot_flag)
{
	//set dot
	if(dot_flag){        
    	divoom_disp_ctrl_set_dot(DIVOOM_DISP_ENV_LED_POINT_X, DIVOOM_DISP_ENV_LED_POINT_1_Y, divoom_light_env_get_fill_color(), disp_buf);
    	divoom_disp_ctrl_set_dot(DIVOOM_DISP_ENV_LED_POINT_X+1, DIVOOM_DISP_ENV_LED_POINT_1_Y, divoom_light_env_get_fill_color(), disp_buf);
    	divoom_disp_ctrl_set_dot(DIVOOM_DISP_ENV_LED_POINT_X, DIVOOM_DISP_ENV_LED_POINT_1_Y+1, divoom_light_env_get_fill_color(), disp_buf);
    	divoom_disp_ctrl_set_dot(DIVOOM_DISP_ENV_LED_POINT_X+1, DIVOOM_DISP_ENV_LED_POINT_1_Y+1, divoom_light_env_get_fill_color(), disp_buf);
    	divoom_disp_ctrl_set_dot(DIVOOM_DISP_ENV_LED_POINT_X, DIVOOM_DISP_ENV_LED_POINT_2_Y, divoom_light_env_get_fill_color(), disp_buf);
    	divoom_disp_ctrl_set_dot(DIVOOM_DISP_ENV_LED_POINT_X+1, DIVOOM_DISP_ENV_LED_POINT_2_Y, divoom_light_env_get_fill_color(), disp_buf);
    	divoom_disp_ctrl_set_dot(DIVOOM_DISP_ENV_LED_POINT_X, DIVOOM_DISP_ENV_LED_POINT_2_Y+1, divoom_light_env_get_fill_color(), disp_buf);
    	divoom_disp_ctrl_set_dot(DIVOOM_DISP_ENV_LED_POINT_X+1, DIVOOM_DISP_ENV_LED_POINT_2_Y+1, divoom_light_env_get_fill_color(), disp_buf);
	}
	divoom_light_env_disp_big_digit(DIVOOM_DISP_ENV_LED_X_1, DIVOOM_DISP_ENV_LED_HOUR_Y, time_info->hour/10, 
			false, disp_buf, DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_LED_DISP, NULL);
	divoom_light_env_disp_big_digit(DIVOOM_DISP_ENV_LED_X_2, DIVOOM_DISP_ENV_LED_HOUR_Y, time_info->hour%10, 
			false, disp_buf, DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_LED_DISP, NULL);
	divoom_light_env_disp_big_digit(DIVOOM_DISP_ENV_LED_X_1, DIVOOM_DISP_ENV_LED_MIN_Y, time_info->min/10, 
			false, disp_buf, DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_LED_DISP, NULL);
	divoom_light_env_disp_big_digit(DIVOOM_DISP_ENV_LED_X_2, DIVOOM_DISP_ENV_LED_MIN_Y, time_info->min%10, 
			false, disp_buf, DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_LED_DISP, NULL);
}

//阴影部分颜色值143,141,146
//新的效果,先显示底板
void divoom_light_env_time_disp_graph(Pdivoom_light_env_time_info time_info)
{
    uint8 i, hour_index, min_index;
#if defined NEW_MATCH_CLOCK
	uint8 j;//, x, y;
	divoom_light_color_info get_color;
	//这个是指针的颜色,和点的颜色,要固定起来
	divoom_light_color_info set_colour = {0, 25, 91};
	//divoom_light_color_info shadow_color = {143, 141, 146};
#endif
#if !defined NEW_MATCH_CLOCK
    Pdivoom_light_color_info cur_color = divoom_light_env_get_fill_color();
    divoom_light_color_info hour_color = {0xff, 0, 0}, min_color = {0,0,0xff};
#endif

#if defined NEW_MATCH_CLOCK
	//24个
    min_index = ((time_info->min + 2) * 2 / 5) % DIVOOM_LIGHT_ENV_MIN_INDEX_NUM;
#else
    min_index = (time_info->min + 2) / 5 % DIVOOM_LIGHT_ENV_MIN_INDEX_NUM;
#endif
    if(time_info->min >= 30)
    {
        hour_index = (time_info->hour + 1) % DIVOOM_LIGHT_ENV_HOUR_INDEX_NUM;
    }
    else
    {
        hour_index = time_info->hour % DIVOOM_LIGHT_ENV_HOUR_INDEX_NUM;
    }
#if defined NEW_MATCH_CLOCK
	//先画表盘上的点
	for(i=0;i<(sizeof(divoom_light_env_point_pos)/sizeof(divoom_light_env_point_pos[0]));i++)
	{
		divoom_disp_ctrl_set_data_xy(divoom_light_env_point_pos[i][0], divoom_light_env_point_pos[i][1], &set_colour, pdivoom_light_env_handle->disp_buf);
	}
	//更新时针
	for(i=0;i<DIVOOM_LIGHT_ENV_HOUR_DOT;i++)
	{
		divoom_disp_ctrl_set_data_xy(divoom_light_env_hour_pos[hour_index][i][0], divoom_light_env_hour_pos[hour_index][i][1], &set_colour, pdivoom_light_env_handle->disp_buf);
	}
	//更新分针,更新颜色
	memset(pdivoom_light_env_handle->bak_buf, 0, SCREEN_BUF_SIZE_32);
	divoom_disp_picture_get_rgb_data(divoom_light_env_min_pic_id[min_index], pdivoom_light_env_handle->bak_buf);
	for(i = 0; i < SCREEN_WIDTH_32; i ++)
	{
		for(j = 0; j < SCREEN_HEIGHT_32; j ++)
		{
			divoom_disp_ctrl_get_dot(i, j, &get_color, pdivoom_light_env_handle->bak_buf);
			if(get_color.r_c != 0 || get_color.g_c != 0 || get_color.b_c != 0)
			{
				divoom_disp_ctrl_set_data_xy(i, j, &set_colour, pdivoom_light_env_handle->disp_buf);
			}
		}
	}
	//开始画阴影
	//时针的
	/*for(i=0;i<DIVOOM_LIGHT_ENV_HOUR_DOT;i++)
	{
		x = divoom_light_env_hour_pos[hour_index][i][0] + 1;
		y = divoom_light_env_hour_pos[hour_index][i][1] + 1;
		x = (x >= SCREEN_WIDTH_32)?SCREEN_WIDTH_32:x;
		y = (y >= SCREEN_WIDTH_32)?SCREEN_WIDTH_32:y;
		divoom_disp_ctrl_get_data_xy(x, y, &get_color, pdivoom_light_env_handle->disp_buf);
		//不等于当前的阴影颜色,并且不等于当前时针显示的颜色就设置阴影
		if(memcmp(&get_color, &set_colour, sizeof(divoom_light_color_info)) != 0 && memcmp(&get_color, &shadow_color, sizeof(divoom_light_color_info)) != 0)
			divoom_disp_ctrl_set_data_xy(x, y, &shadow_color, pdivoom_light_env_handle->disp_buf);
	}
	//更新分针阴影
	for(i = 0; i < SCREEN_WIDTH_32; i ++)
	{
		for(j = 0; j < SCREEN_HEIGHT_32; j ++)
		{
			divoom_disp_ctrl_get_dot(i, j, &get_color, pdivoom_light_env_handle->bak_buf);
			if(get_color.r_c != 0 || get_color.g_c != 0 || get_color.b_c != 0)
			{
				x = i + 1;
				y = j + 1;
				x = (x >= SCREEN_WIDTH_32)?SCREEN_WIDTH_32:x;
				y = (y >= SCREEN_WIDTH_32)?SCREEN_WIDTH_32:y;
				divoom_disp_ctrl_get_dot(x, y, &get_color, pdivoom_light_env_handle->disp_buf);
				if(memcmp(&get_color, &set_colour, sizeof(divoom_light_color_info)) != 0 && memcmp(&get_color, &shadow_color, sizeof(divoom_light_color_info)) != 0)
					divoom_disp_ctrl_set_data_xy(x, y, &shadow_color, pdivoom_light_env_handle->disp_buf);
			}
		}
	}*/
#else

    //画分钟
    if(cur_color->r_c == 0xff && cur_color->g_c == 0 && cur_color->b_c == 0)
    {
        hour_color.r_c = 0xFF;
        hour_color.g_c = 0xFF;
        hour_color.b_c = 0xFF;
    }

    
    if(cur_color->r_c == 0x0d && cur_color->g_c == 0 && cur_color->b_c == 0xff)
    {
        min_color.r_c = 0xFF;
        min_color.g_c = 0xFF;
        min_color.b_c = 0xFF;
    }

    for(i = 0; i < DIVOOM_LIGHT_ENV_MIN_DOT; i ++)
    {
        if(divoom_light_env_min_pos[min_index][i][0] != 0 || divoom_light_env_min_pos[min_index][i][0] != 0)
        {
#if(SCREEN_WIDTH_32 == 16)
            divoom_disp_ctrl_set_data_xy(divoom_light_env_min_pos[min_index][i][0], 
                divoom_light_env_min_pos[min_index][i][1], (Pdivoom_light_color_info)&min_color, pdivoom_light_env_handle->disp_buf);     
#else
            divoom_disp_ctrl_set_data_xy((divoom_light_env_min_pos[min_index][i][0]), 
                (divoom_light_env_min_pos[min_index][i][1]), (Pdivoom_light_color_info)&min_color, pdivoom_light_env_handle->disp_buf);     
            divoom_disp_ctrl_set_data_xy((divoom_light_env_min_pos[min_index][i][0])+1, 
                (divoom_light_env_min_pos[min_index][i][1]), (Pdivoom_light_color_info)&min_color, pdivoom_light_env_handle->disp_buf);     
            divoom_disp_ctrl_set_data_xy((divoom_light_env_min_pos[min_index][i][0]), 
				(divoom_light_env_min_pos[min_index][i][1])+1, (Pdivoom_light_color_info)&min_color, pdivoom_light_env_handle->disp_buf);     
            divoom_disp_ctrl_set_data_xy((divoom_light_env_min_pos[min_index][i][0])+1,
                (divoom_light_env_min_pos[min_index][i][1])+1, (Pdivoom_light_color_info)&min_color, pdivoom_light_env_handle->disp_buf);     
#endif
        }
    }


    //画时钟
    for(i = 0; i < DIVOOM_LIGHT_ENV_HOUR_DOT; i ++)
    {
        if(divoom_light_env_hour_pos[hour_index][i][0] != 0 || divoom_light_env_hour_pos[hour_index][i][0] != 0)
        {
#if(SCREEN_WIDTH_32 == 16)
            divoom_disp_ctrl_set_data_xy(divoom_light_env_hour_pos[hour_index][i][0], 
                divoom_light_env_hour_pos[hour_index][i][1], (Pdivoom_light_color_info)&hour_color, pdivoom_light_env_handle->disp_buf);     
#else
            divoom_disp_ctrl_set_data_xy((divoom_light_env_hour_pos[hour_index][i][0]), 
					(divoom_light_env_hour_pos[hour_index][i][1]), (Pdivoom_light_color_info)&hour_color, pdivoom_light_env_handle->disp_buf);
            divoom_disp_ctrl_set_data_xy((divoom_light_env_hour_pos[hour_index][i][0])+1, 
					(divoom_light_env_hour_pos[hour_index][i][1]), (Pdivoom_light_color_info)&hour_color, pdivoom_light_env_handle->disp_buf);
            divoom_disp_ctrl_set_data_xy((divoom_light_env_hour_pos[hour_index][i][0]), 
					(divoom_light_env_hour_pos[hour_index][i][1])+1, (Pdivoom_light_color_info)&hour_color, pdivoom_light_env_handle->disp_buf);
            divoom_disp_ctrl_set_data_xy((divoom_light_env_hour_pos[hour_index][i][0])+1, 
					(divoom_light_env_hour_pos[hour_index][i][1])+1, (Pdivoom_light_color_info)&hour_color, pdivoom_light_env_handle->disp_buf);
#endif
        }
    }
#endif

    
}

//用位图char[128],代表整个坐标,用一个二维数组写下一个圈的坐标值,当写了第1个颜色的时候,就把当前的位图置1,并查找8个方向的坐标
//每得到一个坐标,就查询相应的位图有没有被置位,被置位就放弃写到坐标数组,如果没有就写进去,并把该位的位图置1,并对该数组写第一个颜色
//第二个颜色,就只找第一个坐标数组的右边,如果该位被置位,放弃,然后写进去第一个颜色数组,并对这个位写第二个颜色
//第三个颜色,就从数组坐标里面找,每一个都往外扩展8个方向,每一个都要去查找位图有没有被写,没有被写就置位该位的第三个颜色
//八个方向:(x-1, y), (x-1, y-1), (x-1, y+1), (x, y-1), (x, y+1), (x+1, y-1), (x+1, y), (x+1, y+1)
#if 0
void divoom_light_env_time_disp_colourful_num(Pdivoom_light_env_time_info time_info)
{
	divoom_screen_color_info get_colour;
	char bit_map[128];
	uint32 i,j;
	uint16 offset = 0, old_offset = 0;
	divoom_light_color_info num_colour;

	//pdivoom_light_env_handle->cur_array = divoom_com_malloc(sizeof(divoom_coulourful_num_xy)*1024, __func__, __LINE__);
	//pdivoom_light_env_handle->next_array = divoom_com_malloc(sizeof(divoom_coulourful_num_xy)*1024, __func__, __LINE__);

	//画4个点的位置
	num_colour.r_c = divoom_light_env_colourful_num[0][0][0];
	num_colour.g_c = divoom_light_env_colourful_num[0][0][1];
	num_colour.b_c = divoom_light_env_colourful_num[0][0][2];
	divoom_disp_ctrl_set_dot(ENV_TIME_COLOURFUL_NUM_P_X, ENV_TIME_COLOURFUL_NUM_P_BIG_Y, &num_colour, pdivoom_light_env_handle->disp_buf);
	divoom_disp_ctrl_set_dot(ENV_TIME_COLOURFUL_NUM_P_X, ENV_TIME_COLOURFUL_NUM_P_BIG_Y+1, &num_colour, pdivoom_light_env_handle->disp_buf);
	num_colour.r_c = divoom_light_env_colourful_num[1][0][0];
	num_colour.g_c = divoom_light_env_colourful_num[1][0][1];
	num_colour.b_c = divoom_light_env_colourful_num[1][0][2];
	divoom_disp_ctrl_set_dot(ENV_TIME_COLOURFUL_NUM_P_X+1, ENV_TIME_COLOURFUL_NUM_P_BIG_Y, &num_colour, pdivoom_light_env_handle->disp_buf);
	divoom_disp_ctrl_set_dot(ENV_TIME_COLOURFUL_NUM_P_X+1, ENV_TIME_COLOURFUL_NUM_P_BIG_Y+1, &num_colour, pdivoom_light_env_handle->disp_buf);
	num_colour.r_c = divoom_light_env_colourful_num[2][0][0];
	num_colour.g_c = divoom_light_env_colourful_num[2][0][1];
	num_colour.b_c = divoom_light_env_colourful_num[2][0][2];
	divoom_disp_ctrl_set_dot(ENV_TIME_COLOURFUL_NUM_P_X, ENV_TIME_COLOURFUL_NUM_P_SMALL_Y, &num_colour, pdivoom_light_env_handle->disp_buf);
	divoom_disp_ctrl_set_dot(ENV_TIME_COLOURFUL_NUM_P_X, ENV_TIME_COLOURFUL_NUM_P_SMALL_Y+1, &num_colour, pdivoom_light_env_handle->disp_buf);
	num_colour.r_c = divoom_light_env_colourful_num[3][0][0];
	num_colour.g_c = divoom_light_env_colourful_num[3][0][1];
	num_colour.b_c = divoom_light_env_colourful_num[3][0][2];
	divoom_disp_ctrl_set_dot(ENV_TIME_COLOURFUL_NUM_P_X+1, ENV_TIME_COLOURFUL_NUM_P_SMALL_Y, &num_colour, pdivoom_light_env_handle->disp_buf);
	divoom_disp_ctrl_set_dot(ENV_TIME_COLOURFUL_NUM_P_X+1, ENV_TIME_COLOURFUL_NUM_P_SMALL_Y+1, &num_colour, pdivoom_light_env_handle->disp_buf);

	memset(pdivoom_light_env_handle->cur_array, 0, sizeof(divoom_coulourful_num_xy)*1024);
	memset(pdivoom_light_env_handle->next_array, 0, sizeof(divoom_coulourful_num_xy)*1024);

	memset(bit_map, 0, sizeof(bit_map));
	divoom_bitmap_init(bit_map, sizeof(bit_map));

	//先把基础的单线条数字画出来
	//并且把相应的bitmap置位
	num_colour.r_c = divoom_light_env_colourful_num[0][0][0];
	num_colour.g_c = divoom_light_env_colourful_num[0][0][1];
	num_colour.b_c = divoom_light_env_colourful_num[0][0][2];
	divoom_light_env_disp_big_digit(ENV_TIME_COLOURFUL_NUM_BIG_X, ENV_TIME_COLOURFUL_NUM_BIG_Y, time_info->hour/10, 
			false, pdivoom_light_env_handle->disp_buf, DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_COLOURFUL_NUM, &num_colour);
	//只对写字的这一块进行扫描,字体是7*9
	for(i=ENV_TIME_COLOURFUL_NUM_BIG_X;i<(ENV_TIME_COLOURFUL_NUM_BIG_X+7);i++)
	{
		for(j=ENV_TIME_COLOURFUL_NUM_BIG_Y;j<(ENV_TIME_COLOURFUL_NUM_BIG_Y+9);j++)
		{
			if(divoom_bitmap_get_bit(bit_map, sizeof(bit_map), i+j*SCREEN_WIDTH_32) == 0)
			{
				divoom_disp_ctrl_get_dot(i, j, &get_colour, pdivoom_light_env_handle->disp_buf);
				if(get_colour.r_c == num_colour.r_c && get_colour.g_c == num_colour.g_c && get_colour.b_c == num_colour.b_c)
				{
					divoom_bitmap_set_bit(bit_map, sizeof(bit_map), i+j*SCREEN_WIDTH_32);
					pdivoom_light_env_handle->cur_array[offset].x = i;
					pdivoom_light_env_handle->cur_array[offset].y = j;
					pdivoom_light_env_handle->cur_array[offset].colour_type = 0;
					offset++;
				}
			}
		}
	}
	num_colour.r_c = divoom_light_env_colourful_num[1][0][0];
	num_colour.g_c = divoom_light_env_colourful_num[1][0][1];
	num_colour.b_c = divoom_light_env_colourful_num[1][0][2];
	divoom_light_env_disp_big_digit(ENV_TIME_COLOURFUL_NUM_SMALL_X, ENV_TIME_COLOURFUL_NUM_BIG_Y, time_info->hour%10, 
			false, pdivoom_light_env_handle->disp_buf, DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_COLOURFUL_NUM, &num_colour);
	for(i=ENV_TIME_COLOURFUL_NUM_SMALL_X;i<(ENV_TIME_COLOURFUL_NUM_SMALL_X+7);i++)
	{
		for(j=ENV_TIME_COLOURFUL_NUM_BIG_Y;j<(ENV_TIME_COLOURFUL_NUM_BIG_Y+9);j++)
		{
			if(divoom_bitmap_get_bit(bit_map, sizeof(bit_map), i+j*SCREEN_WIDTH_32) == 0)
			{
				divoom_disp_ctrl_get_dot(i, j, &get_colour, pdivoom_light_env_handle->disp_buf);
				if(get_colour.r_c == num_colour.r_c && get_colour.g_c == num_colour.g_c && get_colour.b_c == num_colour.b_c)
				{
					divoom_bitmap_set_bit(bit_map, sizeof(bit_map), i+j*SCREEN_WIDTH_32);
					pdivoom_light_env_handle->cur_array[offset].x = i;
					pdivoom_light_env_handle->cur_array[offset].y = j;
					pdivoom_light_env_handle->cur_array[offset].colour_type = 1;
					offset++;
				}
			}
		}
	}
	num_colour.r_c = divoom_light_env_colourful_num[2][0][0];
	num_colour.g_c = divoom_light_env_colourful_num[2][0][1];
	num_colour.b_c = divoom_light_env_colourful_num[2][0][2];
	divoom_light_env_disp_big_digit(ENV_TIME_COLOURFUL_NUM_BIG_X, ENV_TIME_COLOURFUL_NUM_SMALL_Y, time_info->min/10, 
			false, pdivoom_light_env_handle->disp_buf, DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_COLOURFUL_NUM, &num_colour);
	for(i=ENV_TIME_COLOURFUL_NUM_BIG_X;i<(ENV_TIME_COLOURFUL_NUM_BIG_X+7);i++)
	{
		for(j=ENV_TIME_COLOURFUL_NUM_SMALL_Y;j<(ENV_TIME_COLOURFUL_NUM_SMALL_Y+9);j++)
		{
			if(divoom_bitmap_get_bit(bit_map, sizeof(bit_map), i+j*SCREEN_WIDTH_32) == 0)
			{
				divoom_disp_ctrl_get_dot(i, j, &get_colour, pdivoom_light_env_handle->disp_buf);
				if(get_colour.r_c == num_colour.r_c && get_colour.g_c == num_colour.g_c && get_colour.b_c == num_colour.b_c)
				{
					divoom_bitmap_set_bit(bit_map, sizeof(bit_map), i+j*SCREEN_WIDTH_32);
					pdivoom_light_env_handle->cur_array[offset].x = i;
					pdivoom_light_env_handle->cur_array[offset].y = j;
					pdivoom_light_env_handle->cur_array[offset].colour_type = 2;
					offset++;
				}
			}
		}
	}
	num_colour.r_c = divoom_light_env_colourful_num[3][0][0];
	num_colour.g_c = divoom_light_env_colourful_num[3][0][1];
	num_colour.b_c = divoom_light_env_colourful_num[3][0][2];
	divoom_light_env_disp_big_digit(ENV_TIME_COLOURFUL_NUM_SMALL_X, ENV_TIME_COLOURFUL_NUM_SMALL_Y, time_info->min%10, false, 
			pdivoom_light_env_handle->disp_buf, DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_COLOURFUL_NUM, &num_colour);
	for(i=ENV_TIME_COLOURFUL_NUM_SMALL_X;i<(ENV_TIME_COLOURFUL_NUM_SMALL_X+7);i++)
	{
		for(j=ENV_TIME_COLOURFUL_NUM_SMALL_Y;j<(ENV_TIME_COLOURFUL_NUM_SMALL_Y+9);j++)
		{
			if(divoom_bitmap_get_bit(bit_map, sizeof(bit_map), i+j*SCREEN_WIDTH_32) == 0)
			{
				divoom_disp_ctrl_get_dot(i, j, &get_colour, pdivoom_light_env_handle->disp_buf);
				if(get_colour.r_c == num_colour.r_c && get_colour.g_c == num_colour.g_c && get_colour.b_c == num_colour.b_c)
				{
					divoom_bitmap_set_bit(bit_map, sizeof(bit_map), i+j*SCREEN_WIDTH_32);
					pdivoom_light_env_handle->cur_array[offset].x = i;
					pdivoom_light_env_handle->cur_array[offset].y = j;
					pdivoom_light_env_handle->cur_array[offset].colour_type = 3;
					offset++;
				}
			}
		}
	}

	//根据之前得到的位图和坐标开始计算第一圈的坐标
	old_offset = offset;
	//清理保存的next表
	memset(pdivoom_light_env_handle->next_array, 0, sizeof(divoom_coulourful_num_xy)*1024);
	offset = 0;
	for(i=0;i<old_offset;i++)
	{
		//八个方向
		for(j=0;j<8;j++)
		{
			//扩展的方向坐标没有写,那么就对这个点进行填充第二个颜色
			if(divoom_bitmap_get_bit(bit_map, sizeof(bit_map), pdivoom_light_env_handle->cur_array[i].x + need_offset[j][0] + (pdivoom_light_env_handle->cur_array[i].y + need_offset[j][1])*SCREEN_WIDTH_32) == 0)
			{
				divoom_bitmap_set_bit(bit_map, sizeof(bit_map), pdivoom_light_env_handle->cur_array[i].x + need_offset[j][0] + (pdivoom_light_env_handle->cur_array[i].y + need_offset[j][1])*SCREEN_WIDTH_32);
				//写入新的一个cache表中
				pdivoom_light_env_handle->next_array[offset].x = pdivoom_light_env_handle->cur_array[i].x + need_offset[j][0];
				pdivoom_light_env_handle->next_array[offset].y = pdivoom_light_env_handle->cur_array[i].y + need_offset[j][1];
				pdivoom_light_env_handle->next_array[offset].colour_type = pdivoom_light_env_handle->cur_array[i].colour_type;
				num_colour.r_c = divoom_light_env_colourful_num[pdivoom_light_env_handle->cur_array[i].colour_type][1][0];
				num_colour.g_c = divoom_light_env_colourful_num[pdivoom_light_env_handle->cur_array[i].colour_type][1][1];
				num_colour.b_c = divoom_light_env_colourful_num[pdivoom_light_env_handle->cur_array[i].colour_type][1][2];
				divoom_disp_ctrl_set_dot(pdivoom_light_env_handle->next_array[offset].x, pdivoom_light_env_handle->next_array[offset].y, &num_colour, pdivoom_light_env_handle->disp_buf);
				offset++;
			}
		}
	}

#if 1
	//第二圈只找所有pdivoom_light_env_handle->next_array 里面x+1,y不变的点
	memset(pdivoom_light_env_handle->cur_array, 0, sizeof(divoom_coulourful_num_xy)*1024);
	//保存原来的,第三圈的内容要和第二圈重合起来,才能找第三圈
	memcpy(pdivoom_light_env_handle->cur_array, pdivoom_light_env_handle->next_array, sizeof(divoom_coulourful_num_xy)*1024);
	old_offset = offset;
	for(i=0;i<old_offset;i++)
	{
		if(divoom_bitmap_get_bit(bit_map, sizeof(bit_map), pdivoom_light_env_handle->cur_array[i].x + 1 + pdivoom_light_env_handle->cur_array[i].y*SCREEN_WIDTH_32) == 0)
		{
			divoom_bitmap_set_bit(bit_map, sizeof(bit_map), pdivoom_light_env_handle->cur_array[i].x + 1 + pdivoom_light_env_handle->cur_array[i].y*SCREEN_WIDTH_32);
			pdivoom_light_env_handle->next_array[offset].x = pdivoom_light_env_handle->cur_array[i].x + 1;
			pdivoom_light_env_handle->next_array[offset].y = pdivoom_light_env_handle->cur_array[i].y;
			pdivoom_light_env_handle->next_array[offset].colour_type = pdivoom_light_env_handle->cur_array[i].colour_type;
			//divoom_print("pdivoom_light_env_handle->next_array[%d].x:%d,%d,type:%d\n", offset, pdivoom_light_env_handle->next_array[offset].x, pdivoom_light_env_handle->next_array[offset].y, pdivoom_light_env_handle->next_array[offset].colour_type);
			num_colour.r_c = divoom_light_env_colourful_num[pdivoom_light_env_handle->cur_array[i].colour_type][2][0];
			num_colour.g_c = divoom_light_env_colourful_num[pdivoom_light_env_handle->cur_array[i].colour_type][2][1];
			num_colour.b_c = divoom_light_env_colourful_num[pdivoom_light_env_handle->cur_array[i].colour_type][2][2];
			divoom_disp_ctrl_set_dot(pdivoom_light_env_handle->next_array[offset].x, pdivoom_light_env_handle->next_array[offset].y, &num_colour, pdivoom_light_env_handle->disp_buf);
			offset++;
		}
	}

	//第三圈
	old_offset = offset;
	offset = 0;
	memset(pdivoom_light_env_handle->cur_array, 0, sizeof(divoom_coulourful_num_xy)*1024);
	memcpy(pdivoom_light_env_handle->cur_array, pdivoom_light_env_handle->next_array, sizeof(divoom_coulourful_num_xy)*1024);
	memset(pdivoom_light_env_handle->next_array, 0, sizeof(divoom_coulourful_num_xy)*1024);
	for(i=0;i<old_offset;i++)
	{
		//八个方向
		for(j=0;j<8;j++)
		{
			//扩展的方向坐标没有写,那么就对这个点进行填充第二个颜色
			if(divoom_bitmap_get_bit(bit_map, sizeof(bit_map), (pdivoom_light_env_handle->cur_array[i].x + need_offset[j][0]) + (pdivoom_light_env_handle->cur_array[i].y + need_offset[j][1])*SCREEN_WIDTH_32) == 0)
			{
				divoom_bitmap_set_bit(bit_map, sizeof(bit_map), (pdivoom_light_env_handle->cur_array[i].x + need_offset[j][0]) + (pdivoom_light_env_handle->cur_array[i].y + need_offset[j][1])*SCREEN_WIDTH_32);
				//写入新的一个cache表中
				pdivoom_light_env_handle->next_array[offset].x = pdivoom_light_env_handle->cur_array[i].x + need_offset[j][0];
				pdivoom_light_env_handle->next_array[offset].y = pdivoom_light_env_handle->cur_array[i].y + need_offset[j][1];
				pdivoom_light_env_handle->next_array[offset].colour_type = pdivoom_light_env_handle->cur_array[i].colour_type;
				num_colour.r_c = divoom_light_env_colourful_num[pdivoom_light_env_handle->cur_array[i].colour_type][3][0];
				num_colour.g_c = divoom_light_env_colourful_num[pdivoom_light_env_handle->cur_array[i].colour_type][3][1];
				num_colour.b_c = divoom_light_env_colourful_num[pdivoom_light_env_handle->cur_array[i].colour_type][3][2];
				divoom_disp_ctrl_set_dot(pdivoom_light_env_handle->next_array[offset].x, pdivoom_light_env_handle->next_array[offset].y, &num_colour, pdivoom_light_env_handle->disp_buf);
				offset++;
			}
		}
	}
#else
	//把第二圈和第一圈分开放
	//第二圈只找所有pdivoom_light_env_handle->next_array 里面x+1,y不变的点
	memset(pdivoom_light_env_handle->cur_array, 0, sizeof(divoom_coulourful_num_xy)*1024);
	//保存原来的,第三圈的内容要和第二圈重合起来,才能找第三圈
	memcpy(pdivoom_light_env_handle->cur_array, pdivoom_light_env_handle->next_array, sizeof(divoom_coulourful_num_xy)*1024);
	memset(pdivoom_light_env_handle->next_array, 0, sizeof(divoom_coulourful_num_xy)*1024);
	old_offset = offset;
	offset = 0;
	divoom_print("\n\nTHIS IS 3!,    offset =  %d, old_offset = %d\n", offset, old_offset);
	for(i=0;i<old_offset;i++)
	{
		if(divoom_bitmap_get_bit(bit_map, sizeof(bit_map), pdivoom_light_env_handle->cur_array[i].x + 1 + pdivoom_light_env_handle->cur_array[i].y*SCREEN_WIDTH_32) == 0)
		{
			divoom_bitmap_set_bit(bit_map, sizeof(bit_map), pdivoom_light_env_handle->cur_array[i].x + 1 + pdivoom_light_env_handle->cur_array[i].y*SCREEN_WIDTH_32);
			pdivoom_light_env_handle->next_array[offset].x = pdivoom_light_env_handle->cur_array[i].x + 1;
			pdivoom_light_env_handle->next_array[offset].y = pdivoom_light_env_handle->cur_array[i].y;
			pdivoom_light_env_handle->next_array[offset].colour_type = pdivoom_light_env_handle->cur_array[i].colour_type;
			num_colour.r_c = divoom_light_env_colourful_num[pdivoom_light_env_handle->cur_array[i].colour_type][2][0];
			num_colour.g_c = divoom_light_env_colourful_num[pdivoom_light_env_handle->cur_array[i].colour_type][2][1];
			num_colour.b_c = divoom_light_env_colourful_num[pdivoom_light_env_handle->cur_array[i].colour_type][2][2];
			divoom_disp_ctrl_set_dot(pdivoom_light_env_handle->next_array[offset].x, pdivoom_light_env_handle->next_array[offset].y, &get_colour, pdivoom_light_env_handle->disp_buf);
			offset++;
		}
	}

	//先遍历cur,再遍历next
	//第三圈
	divoom_print("\n\nTHIS IS 4!, offset   =   %d,   old_offset = %d\n", offset, old_offset);
	for(i=0;i<old_offset;i++)
	{
		//八个方向
		for(j=0;j<8;j++)
		{
			//扩展的方向坐标没有写,那么就对这个点进行填充第二个颜色
			if(divoom_bitmap_get_bit(bit_map, sizeof(bit_map), pdivoom_light_env_handle->cur_array[i].x + need_offset[j][0] + (pdivoom_light_env_handle->cur_array[i].y + need_offset[j][1])*SCREEN_WIDTH_32) == 0)
			{
				/*divoom_bitmap_set_bit(bit_map, sizeof(bit_map), (pdivoom_light_env_handle->cur_array[i].x + need_offset[j][0]) + (pdivoom_light_env_handle->cur_array[i].y + need_offset[j][1])*SCREEN_WIDTH_32);
				//写入新的一个cache表中
				pdivoom_light_env_handle->next_array[offset].x = pdivoom_light_env_handle->cur_array[i].x + need_offset[j][0];
				pdivoom_light_env_handle->next_array[offset].y = pdivoom_light_env_handle->cur_array[i].y + need_offset[j][1];
				pdivoom_light_env_handle->next_array[offset].colour_type = pdivoom_light_env_handle->cur_array[i].colour_type;*/
				num_colour.r_c = divoom_light_env_colourful_num[pdivoom_light_env_handle->cur_array[i].colour_type][3][0];
				num_colour.g_c = divoom_light_env_colourful_num[pdivoom_light_env_handle->cur_array[i].colour_type][3][1];
				num_colour.b_c = divoom_light_env_colourful_num[pdivoom_light_env_handle->cur_array[i].colour_type][3][2];
				divoom_disp_ctrl_set_dot(pdivoom_light_env_handle->next_array[offset].x, pdivoom_light_env_handle->next_array[offset].y, &num_colour, pdivoom_light_env_handle->disp_buf);
			}
		}
	}
	for(i=0;i<offset;i++)
	{
		//八个方向
		for(j=0;j<8;j++)
		{
			//扩展的方向坐标没有写,那么就对这个点进行填充第二个颜色
			if(divoom_bitmap_get_bit(bit_map, sizeof(bit_map), pdivoom_light_env_handle->next_array[i].x + need_offset[j][0] + (pdivoom_light_env_handle->next_array[i].y + need_offset[j][1])*SCREEN_WIDTH_32) == 0)
			{
				/*divoom_bitmap_set_bit(bit_map, sizeof(bit_map), (pdivoom_light_env_handle->next_array[i].x + need_offset[j][0]) + (pdivoom_light_env_handle->next_array[i].y + need_offset[j][1])*SCREEN_WIDTH_32);
				//写入新的一个cache表中
				pdivoom_light_env_handle->next_array[offset].x = pdivoom_light_env_handle->next_array[i].x + need_offset[j][0];
				pdivoom_light_env_handle->next_array[offset].y = pdivoom_light_env_handle->next_array[i].y + need_offset[j][1];
				pdivoom_light_env_handle->next_array[offset].colour_type = pdivoom_light_env_handle->next_array[i].colour_type;*/
				num_colour.r_c = divoom_light_env_colourful_num[pdivoom_light_env_handle->next_array[i].colour_type][3][0];
				num_colour.g_c = divoom_light_env_colourful_num[pdivoom_light_env_handle->next_array[i].colour_type][3][1];
				num_colour.b_c = divoom_light_env_colourful_num[pdivoom_light_env_handle->next_array[i].colour_type][3][2];
				divoom_disp_ctrl_set_dot(pdivoom_light_env_handle->next_array[offset].x, pdivoom_light_env_handle->next_array[offset].y, &num_colour, pdivoom_light_env_handle->disp_buf);
			}
		}
	}
#endif
	//divoom_com_free(pdivoom_light_env_handle->cur_array, __func__, __LINE__);
	//divoom_com_free(pdivoom_light_env_handle->next_array, __func__, __LINE__);
}
#endif


/*********************************************************************
  Function:         divoom_light_env_time
  Description:     显示时间
  Input:             refresh_flag       是否强制刷新
  Return:           NONE
  Author:           divoom
**********************************************************************/

void  divoom_light_env_time(bool refresh_flag)
{
#if(SCREEN_WIDTH_32 == 32)
	int i = 0, j = 0;
#endif
    divoom_light_env_time_info cur_info;

    pdivoom_light_env_handle->penv_read(DIVOOM_DISP_ENV_TIME, (void *)&cur_info);
    //if(pdivoom_light_env_handle->effect_type != 2)
    {
        if(pdivoom_light_env_handle->hour_flag == 0)
        {
            if(cur_info.hour != 12)
            {
                cur_info.hour = cur_info.hour % 12;
                if(cur_info.hour == 0)
                {
                    cur_info.hour = 12;
                }
            }
        }
    }
    if(cur_info.hour != pdivoom_light_env_handle->time_info.hour || cur_info.min!= pdivoom_light_env_handle->time_info.min)
    {
        //如果显示数据发生变化，将刷新
        pdivoom_light_env_handle->time_info.hour = cur_info.hour;
        pdivoom_light_env_handle->time_info.min = cur_info.min;
        refresh_flag = true;
    }
    //先清屏
    if(refresh_flag == true)
	{
		uint8 i, j;
		divoom_light_color_info cur_color;
		//divoom_print("\n\n[%s:%d]refresh_flag = %d\n\n", __func__, __LINE__, refresh_flag);
		memset(pdivoom_light_env_handle->disp_buf, 0, sizeof(pdivoom_light_env_handle->disp_buf));
        /*if(pdivoom_light_env_handle->effect_type == DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_COLOR_FLOOR)
        {
            divoom_disp_ctrl_fill(pdivoom_light_env_handle->disp_buf, &pdivoom_light_env_handle->color);
        }
        else */if(pdivoom_light_env_handle->effect_type == DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_GRAPH)
        {
#if defined NEW_MATCH_CLOCK
            divoom_disp_picture_get_rgb_data(DIVOOM_DISP_NEW_CLOCK_SIX, pdivoom_light_env_handle->disp_buf);

            for(i = 0; i < SCREEN_WIDTH_32; i ++)
            {
                for(j = 0; j < SCREEN_HEIGHT_32; j ++)
                {
                    divoom_disp_ctrl_get_dot(i, j,&cur_color, pdivoom_light_env_handle->disp_buf);
					//这里要改表盘的颜色,表盘的颜色就是这个值,ubuntu下用gcolour2取色
                    //if(cur_color.r_c == 0xE1 && cur_color.g_c == 0xDA && cur_color.b_c == 0xEA)
                    if(cur_color.r_c == 0 && cur_color.g_c == 0 && cur_color.b_c == 0)
                    {
                        divoom_disp_ctrl_set_data_xy(i, j, divoom_light_env_get_fill_color(), pdivoom_light_env_handle->disp_buf);
                    }
                }
            }
#else            
			divoom_disp_picture_get_rgb_data(DIVOOM_DISP_DIVOOM_TIME_TYPE_FIVE_BACKGRAND, pdivoom_light_env_handle->bak_buf);
            
            for(i = 0; i < SCREEN_WIDTH_32; i ++)
            {
                for(j = 0; j < SCREEN_HEIGHT_32; j ++)
                {
                    divoom_disp_ctrl_get_dot(i, j,&cur_color, pdivoom_light_env_handle->bak_buf);
					//这个地方太坑了,到底是啥意思啊?怎么判断了三次r_c的值啊?
                    if(cur_color.r_c == 0xFF && cur_color.r_c == 0xFF && cur_color.r_c == 0xFF)
                    {
                        divoom_disp_ctrl_set_data_xy(i, j, divoom_light_env_get_fill_color(), pdivoom_light_env_handle->bak_buf); 
                    }
                }
            }
            //memcpy(pdivoom_light_env_handle->disp_buf, pdivoom_light_env_handle->bak_buf, SCREEN_BUF_SIZE_32);
#endif
        }
		else if(pdivoom_light_env_handle->effect_type == DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_LED_DISP)
		{
			divoom_light_color_info cur_color;
			divoom_light_color_info set_color;
			divoom_disp_picture_get_rgb_data(DIVOOM_DISP_LED_BACKGROUND, pdivoom_light_env_handle->bak_buf);

			memcpy(&set_color, &pdivoom_light_env_handle->color, sizeof(divoom_light_color_info));
			set_color.r_c = set_color.r_c/10;
			set_color.g_c = set_color.g_c/10;
			set_color.b_c = set_color.b_c/10;

			for(i = 0; i < SCREEN_WIDTH_32; i ++)
			{
				for(j = 0; j < SCREEN_HEIGHT_32; j ++)
				{
					divoom_disp_ctrl_get_dot(i, j,&cur_color, pdivoom_light_env_handle->bak_buf);
					if(cur_color.r_c != 0 || cur_color.g_c != 0 || cur_color.b_c != 0)
					{
						divoom_disp_ctrl_set_data_xy(i, j, &set_color, pdivoom_light_env_handle->disp_buf);
					}
				}
			}
		}
		/*else if(pdivoom_light_env_handle->effect_type == DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_COLOURFUL_NUM)
		  {
		  divoom_disp_picture_get_rgb_data(DIVOOM_DISP_CLOCK_COLOURFUL_NUM_BACKGRADE, pdivoom_light_env_handle->disp_buf);
		  }*/
		else if(pdivoom_light_env_handle->effect_type == DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_GOLD_BACKGROUND)
		{
			divoom_disp_picture_get_rgb_data(DIVOOM_DISP_GOLD_BACKGROUND, pdivoom_light_env_handle->bak_buf);

			for(i = 0; i < SCREEN_WIDTH_32; i ++)
			{
				for(j = 0; j < SCREEN_HEIGHT_32; j ++)
				{
					divoom_disp_ctrl_get_dot(i, j, &cur_color, pdivoom_light_env_handle->bak_buf);
					if(cur_color.r_c != 0 || cur_color.g_c != 0 || cur_color.b_c != 0)
					{
						divoom_disp_ctrl_set_data_xy(i, j, &cur_color, pdivoom_light_env_handle->disp_buf);
					}
				}
			}
		}
		else if(pdivoom_light_env_handle->effect_type == DIVOOM_LIGHT_ENV_TIME_DUSP_EFFECT_ITALIC)
		{
			//显示背景
			divoom_disp_picture_get_rgb_data(DIVOOM_DISP_ITALIC_BACKGROUND, pdivoom_light_env_handle->disp_buf);
		}
		else
		{
			divoom_disp_ctrl_fill(pdivoom_light_env_handle->disp_buf, divoom_light_env_get_back_color());
		}
	}

	if(pdivoom_light_env_handle->effect_type == DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_CIRCULAR)
	{
		//        divoom_light_env_time_pic(&cur_info);

        divoom_light_env_time_pic_mode2(&cur_info);
        return ;
    }
	else if(pdivoom_light_env_handle->effect_type == DIVOOM_LIGHT_ENV_TIME_DUSP_EFFECT_ITALIC)
	{
		divoom_disp_time_disp_italic_time(&cur_info);
		return;
	}
	else if(pdivoom_light_env_handle->effect_type == DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_COLOURFUL_NUM)
	{
		//divoom_light_env_time_disp_colourful_num(&cur_info);
		divoom_disp_time_common_get_time_buf(pdivoom_light_env_handle->disp_buf, &cur_info, 1);
        return ;
	}
    else if(pdivoom_light_env_handle->effect_type == DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_GRAPH)
    {
#if !defined NEW_MATCH_CLOCK
        memcpy(pdivoom_light_env_handle->disp_buf, pdivoom_light_env_handle->bak_buf, SCREEN_BUF_SIZE_32);
#endif
        divoom_light_env_time_disp_graph(&cur_info);
        return;
    }
	else if(pdivoom_light_env_handle->effect_type == DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_LED_DISP)
	{
		divoom_disp_time_disp_led_time(pdivoom_light_env_handle->disp_buf, &cur_info, 1);
		return;
	}
	else if(pdivoom_light_env_handle->effect_type == DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_GOLD_BACKGROUND)
	{
		divoom_disp_time_disp_dial_time(&cur_info, DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_GOLD_BACKGROUND);
		return;
	}
	else if(pdivoom_light_env_handle->effect_type == DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_BLUE_COLOURFUL)
	{
		divoom_disp_time_disp_dial_time(&cur_info, DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_BLUE_COLOURFUL);
		return;
	}
    /*else if(pdivoom_light_env_handle->effect_type == DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_SQUARE)
    {
        //显示分隔符
        #if 0
        if(pdivoom_light_env_handle->tick & 1)
        {
            divoom_disp_ctrl_set_dot(3 ,9, (Pdivoom_light_color_info)&divoom_env_black_color, pdivoom_light_env_handle->disp_buf);
            divoom_disp_ctrl_set_dot(3, 11 ,(Pdivoom_light_color_info)&divoom_env_black_color,pdivoom_light_env_handle->disp_buf);
        }
        else
        #endif
        {
#if(SCREEN_WIDTH_32 == 32)
			for(i=5, j = 19;i<5+3;i++)
			{
				divoom_disp_ctrl_set_dot(i, j, divoom_light_env_get_fill_color(), pdivoom_light_env_handle->disp_buf);
				divoom_disp_ctrl_set_dot(i, j+1, divoom_light_env_get_fill_color(), pdivoom_light_env_handle->disp_buf);
				divoom_disp_ctrl_set_dot(i, j+2, divoom_light_env_get_fill_color(), pdivoom_light_env_handle->disp_buf);
			}
			for(i=5, j = 25;i<5+3;i++)
			{
				divoom_disp_ctrl_set_dot(i, j, divoom_light_env_get_fill_color(), pdivoom_light_env_handle->disp_buf);
				divoom_disp_ctrl_set_dot(i, j+1, divoom_light_env_get_fill_color(), pdivoom_light_env_handle->disp_buf);
				divoom_disp_ctrl_set_dot(i, j+2, divoom_light_env_get_fill_color(), pdivoom_light_env_handle->disp_buf);
			}
#else
            divoom_disp_ctrl_set_dot(3 ,10, divoom_light_env_get_fill_color(), pdivoom_light_env_handle->disp_buf);
            divoom_disp_ctrl_set_dot(3 ,12, divoom_light_env_get_fill_color(), pdivoom_light_env_handle->disp_buf);
#endif
        }
        if(refresh_flag == false)
        {
            //如果时间没有改变将退出
            return;
        }
        //显示时间
#if(SCREEN_WIDTH_32 == 32)
		divoom_light_env_disp_big_digit(ENV_SQUARE_NUM_LESS_X, ENV_SQUARE_NUM_LESS_Y, cur_info.hour/10, 
				false, pdivoom_light_env_handle->disp_buf, DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_SQUARE, NULL);
		divoom_light_env_disp_big_digit(ENV_SQUARE_NUM_BIG_X, ENV_SQUARE_NUM_LESS_Y, cur_info.hour%10, 
				false, pdivoom_light_env_handle->disp_buf, DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_SQUARE, NULL);
		divoom_light_env_disp_big_digit(ENV_SQUARE_NUM_LESS_X, ENV_SQUARE_NUM_BIG_Y, cur_info.min/10, 
				false, pdivoom_light_env_handle->disp_buf, DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_SQUARE, NULL);
		divoom_light_env_disp_big_digit(ENV_SQUARE_NUM_BIG_X, ENV_SQUARE_NUM_BIG_Y, cur_info.min%10, false, 
				pdivoom_light_env_handle->disp_buf, DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_SQUARE, NULL);
#else
        divoom_light_env_disp_digit(5, 2 ,cur_info.hour/10, false, pdivoom_light_env_handle->disp_buf);
        divoom_light_env_disp_digit(9, 2 ,cur_info.hour%10, false, pdivoom_light_env_handle->disp_buf);
        divoom_light_env_disp_digit(5, 9 ,cur_info.min/10, false, pdivoom_light_env_handle->disp_buf);
        divoom_light_env_disp_digit(9, 9 ,cur_info.min%10, false, pdivoom_light_env_handle->disp_buf);        
#endif
        return;
    }
    else if(pdivoom_light_env_handle->effect_type == DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_JUST_NUM)
	{
        {
#if(SCREEN_WIDTH_32 == 32)
			//dot
			for(i=5, j = 20;i<5+3;i++)
			{
				divoom_disp_ctrl_set_dot(i, j, divoom_light_env_get_fill_color(), pdivoom_light_env_handle->disp_buf);
				divoom_disp_ctrl_set_dot(i, j+1, divoom_light_env_get_fill_color(), pdivoom_light_env_handle->disp_buf);
				divoom_disp_ctrl_set_dot(i, j+2, divoom_light_env_get_fill_color(), pdivoom_light_env_handle->disp_buf);
			}
			for(i=5, j = 25;i<5+3;i++)
			{
				divoom_disp_ctrl_set_dot(i, j, divoom_light_env_get_fill_color(), pdivoom_light_env_handle->disp_buf);
				divoom_disp_ctrl_set_dot(i, j+1, divoom_light_env_get_fill_color(), pdivoom_light_env_handle->disp_buf);
				divoom_disp_ctrl_set_dot(i, j+2, divoom_light_env_get_fill_color(), pdivoom_light_env_handle->disp_buf);
			}
#else
            divoom_disp_ctrl_set_dot(4, 9, divoom_light_env_get_fill_color(), pdivoom_light_env_handle->disp_buf);
            divoom_disp_ctrl_set_dot(4, 13, divoom_light_env_get_fill_color(), pdivoom_light_env_handle->disp_buf);
#endif
        }
        if(refresh_flag == false)
        {
            //如果时间没有改变将退出
            return;
        }
#if(SCREEN_WIDTH_32 == 32)
		divoom_light_env_disp_big_digit(ENV_JUST_NUM_LESS_X, ENV_JUST_NUM_LESS_Y, cur_info.hour/10, 
				false, pdivoom_light_env_handle->disp_buf, DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_JUST_NUM, NULL);
		divoom_light_env_disp_big_digit(ENV_JUST_NUM_BIG_X, ENV_JUST_NUM_LESS_Y, cur_info.hour%10, 
				false, pdivoom_light_env_handle->disp_buf, DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_JUST_NUM, NULL);
		divoom_light_env_disp_big_digit(ENV_JUST_NUM_LESS_X, ENV_JUST_NUM_BIG_Y, cur_info.min/10, 
				false, pdivoom_light_env_handle->disp_buf, DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_JUST_NUM, NULL);
		divoom_light_env_disp_big_digit(ENV_JUST_NUM_BIG_X, ENV_JUST_NUM_BIG_Y, cur_info.min%10, false, 
				pdivoom_light_env_handle->disp_buf, DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_JUST_NUM, NULL);
#else
		divoom_light_env_disp_big_digit(4, 0, cur_info.hour/10, false, pdivoom_light_env_handle->disp_buf, DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_JUST_NUM, NULL);
		divoom_light_env_disp_big_digit(10, 0, cur_info.hour%10, false, pdivoom_light_env_handle->disp_buf, DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_JUST_NUM, NULL);
		divoom_light_env_disp_big_digit(4, 9, cur_info.min/10, false, pdivoom_light_env_handle->disp_buf, DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_JUST_NUM, NULL);
		divoom_light_env_disp_big_digit(10, 9, cur_info.min%10, false, pdivoom_light_env_handle->disp_buf, DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_JUST_NUM, NULL);
#endif
		return;
	}
    else if(pdivoom_light_env_handle->effect_type == DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_COLOR_FLOOR)
    {
        {
			//dot
#if(SCREEN_WIDTH_32 == 32)
			for(i=5, j = 20;i<5+3;i++)
			{
				divoom_disp_ctrl_set_dot(i, j, divoom_light_env_get_fill_color(), pdivoom_light_env_handle->disp_buf);
				divoom_disp_ctrl_set_dot(i, j+1, divoom_light_env_get_fill_color(), pdivoom_light_env_handle->disp_buf);
				divoom_disp_ctrl_set_dot(i, j+2, divoom_light_env_get_fill_color(), pdivoom_light_env_handle->disp_buf);
			}
			for(i=5, j = 25;i<5+3;i++)
			{
				divoom_disp_ctrl_set_dot(i, j, divoom_light_env_get_fill_color(), pdivoom_light_env_handle->disp_buf);
				divoom_disp_ctrl_set_dot(i, j+1, divoom_light_env_get_fill_color(), pdivoom_light_env_handle->disp_buf);
				divoom_disp_ctrl_set_dot(i, j+2, divoom_light_env_get_fill_color(), pdivoom_light_env_handle->disp_buf);
			}
#else
            divoom_disp_ctrl_set_dot(2,  9, divoom_light_env_get_fill_color(), pdivoom_light_env_handle->disp_buf);
            divoom_disp_ctrl_set_dot(2, 13, divoom_light_env_get_fill_color(), pdivoom_light_env_handle->disp_buf);
#endif
        }
        if(refresh_flag == false)
        {
            //如果时间没有改变将退出
            return;
        }
#if(SCREEN_WIDTH_32 == 32)
		divoom_light_env_disp_big_digit(ENV_JUST_NUM_LESS_X, ENV_JUST_NUM_LESS_Y, cur_info.hour/10, 
				false, pdivoom_light_env_handle->disp_buf, DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_JUST_NUM, NULL);
		divoom_light_env_disp_big_digit(ENV_JUST_NUM_BIG_X, ENV_JUST_NUM_LESS_Y, cur_info.hour%10, 
				false, pdivoom_light_env_handle->disp_buf, DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_JUST_NUM, NULL);
		divoom_light_env_disp_big_digit(ENV_JUST_NUM_LESS_X, ENV_JUST_NUM_BIG_Y, cur_info.min/10, 
				false, pdivoom_light_env_handle->disp_buf, DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_JUST_NUM, NULL);
		divoom_light_env_disp_big_digit(ENV_JUST_NUM_BIG_X, ENV_JUST_NUM_BIG_Y, cur_info.min%10, false, 
				pdivoom_light_env_handle->disp_buf, DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_JUST_NUM, NULL);
#else
		divoom_light_env_disp_big_digit(4, 1, cur_info.hour/10, false, pdivoom_light_env_handle->disp_buf, DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_COLOR_FLOOR, NULL);
		divoom_light_env_disp_big_digit(10, 1, cur_info.hour%10, false, pdivoom_light_env_handle->disp_buf, DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_COLOR_FLOOR, NULL);
		divoom_light_env_disp_big_digit(4, 9, cur_info.min/10, false, pdivoom_light_env_handle->disp_buf, DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_COLOR_FLOOR, NULL);
		divoom_light_env_disp_big_digit(10, 9, cur_info.min%10, false, pdivoom_light_env_handle->disp_buf, DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_COLOR_FLOOR, NULL);
#endif
		return;
    }*/
    else if(pdivoom_light_env_handle->effect_type == DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_MAX_NUMBER)
    {
    }
	/*
	else if(pdivoom_light_env_handle->effect_type == DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_JUST_BAK)
	{
		if(refresh_flag == false)
		{
			//如果时间没有改变将退出
			return;
		}
	{
		//back_color
		memset(pdivoom_light_env_handle->disp_buf, 0, sizeof(pdivoom_light_env_handle->disp_buf));
		divoom_disp_ctrl_fill(pdivoom_light_env_handle->disp_buf, divoom_light_env_get_fill_color());
	}
	{
		//dot
		divoom_disp_ctrl_set_dot(2,  9, &divoom_env_black_color, pdivoom_light_env_handle->disp_buf);
		divoom_disp_ctrl_set_dot(2, 13, &divoom_env_black_color, pdivoom_light_env_handle->disp_buf);
	}
	divoom_light_env_disp_big_digit(4, 0, cur_info.hour/10, false, pdivoom_light_env_handle->disp_buf, DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_JUST_BAK, NULL);
	divoom_light_env_disp_big_digit(10, 0, cur_info.hour%10, false, pdivoom_light_env_handle->disp_buf, DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_JUST_BAK, NULL);
	divoom_light_env_disp_big_digit(4, 9, cur_info.min/10, false, pdivoom_light_env_handle->disp_buf, DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_JUST_BAK, NULL);
	divoom_light_env_disp_big_digit(10, 9, cur_info.min%10, false, pdivoom_light_env_handle->disp_buf, DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_JUST_BAK, NULL);
	return;
	}
	*/
	//pdivoom_light_env_handle->index = cur_info.hour;
	//显示分隔符
#if 0
	if(pdivoom_light_env_handle->tick & 1)
	{
		divoom_disp_ctrl_set_dot(6 ,6, divoom_light_env_get_back_color(), pdivoom_light_env_handle->disp_buf);
		divoom_disp_ctrl_set_dot(6, 8 ,divoom_light_env_get_back_color(),pdivoom_light_env_handle->disp_buf);
	}
	else
#endif
#if(SCREEN_WIDTH_32 == 32)
	//画第一个效果的点
	//这里其实是 DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_COLOURFUL 的数字部分,不知道为什么要这么写
	/*效果分布
	  6行空格
	  **********************************************************
	  **********************************************************
	  3行空格
	  //开始数字部分,高度10
	  //字与字之间空格两列,第一个字和最后一个字与边界空一列
	  3行空格
	  **********************************************************
	  **********************************************************
	  6行空格
	*/
	for(i=15, j = 12;i<15+2;i++)
	{
		divoom_disp_ctrl_set_dot(i, j, divoom_light_env_get_fill_color(), pdivoom_light_env_handle->disp_buf);
		divoom_disp_ctrl_set_dot(i, j+1, divoom_light_env_get_fill_color(), pdivoom_light_env_handle->disp_buf);
	}
	for(i=15, j = 18;i<15+2;i++)
	{
		divoom_disp_ctrl_set_dot(i, j, divoom_light_env_get_fill_color(), pdivoom_light_env_handle->disp_buf);
		divoom_disp_ctrl_set_dot(i, j+1, divoom_light_env_get_fill_color(), pdivoom_light_env_handle->disp_buf);
	}
#else
    {
        divoom_disp_ctrl_set_dot(7 ,6, divoom_light_env_get_fill_color(), pdivoom_light_env_handle->disp_buf);
        divoom_disp_ctrl_set_dot(7 ,8, divoom_light_env_get_fill_color(), pdivoom_light_env_handle->disp_buf);
    }
#endif
    if(refresh_flag == false)
    {
        //如果时间没有改变将退出
        return;
    }
    //显示时间
    //divoom_print("hour:%d, min:%d\n", cur_info.hour, cur_info.min);
//    if(cur_info.hour >= 10)
#if(SCREEN_WIDTH_32 == 32)
	//写时间,按照上面的描述,从第 12行开始,占10行
	divoom_light_env_disp_big_digit(DIVOOM_DISP_ENV_DISP_START_X, DIVOOM_DISP_ENV_DISP_START_Y, cur_info.hour/10, 
			false, pdivoom_light_env_handle->disp_buf, DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_COLOURFUL, NULL);
	divoom_light_env_disp_big_digit(DIVOOM_DISP_ENV_DISP_START_X + 6/*字宽*/ + 1/*字与字之间的宽度*/, DIVOOM_DISP_ENV_DISP_START_Y, cur_info.hour%10, 
			false, pdivoom_light_env_handle->disp_buf, DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_COLOURFUL, NULL);
	divoom_light_env_disp_big_digit(DIVOOM_DISP_ENV_DISP_START_X + 6*2 + 2*3 + 1, DIVOOM_DISP_ENV_DISP_START_Y, cur_info.min/10, 
			false, pdivoom_light_env_handle->disp_buf, DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_COLOURFUL, NULL);
	divoom_light_env_disp_big_digit(DIVOOM_DISP_ENV_DISP_START_X + 6*3 + 2*3 + 1 + 1, DIVOOM_DISP_ENV_DISP_START_Y, cur_info.min%10, 
			false, pdivoom_light_env_handle->disp_buf, DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_COLOURFUL, NULL);
#else
    {
        divoom_light_env_disp_digit(DIVOOM_DISP_ENV_DISP_START_X , DIVOOM_DISP_ENV_DISP_START_Y ,cur_info.hour/10, false, pdivoom_light_env_handle->disp_buf);
        
    }
    divoom_light_env_disp_digit(DIVOOM_DISP_ENV_DISP_START_X + 4, DIVOOM_DISP_ENV_DISP_START_Y ,cur_info.hour%10, false, pdivoom_light_env_handle->disp_buf);
    divoom_light_env_disp_digit(DIVOOM_DISP_ENV_DISP_START_X + 8, DIVOOM_DISP_ENV_DISP_START_Y ,cur_info.min/10, false, pdivoom_light_env_handle->disp_buf);
    divoom_light_env_disp_digit(DIVOOM_DISP_ENV_DISP_START_X + 12, DIVOOM_DISP_ENV_DISP_START_Y ,cur_info.min%10, false, pdivoom_light_env_handle->disp_buf);
#endif
}


/*****************************************************************************
 Prototype    : divoom_light_env_weather
 Description  : 天气动画显示
 Input        : void  
 Output       : None
 Return Value : 
 
  History        :
  1.Date         : 2017/8/26
    Author       : coronaa
    Modification : 改为动画数组
*****************************************************************************/
void divoom_light_env_weather(uint8 refresh_flag)
{
    uint16 time;
	uint8 cur_weather_type = 0;
	//天气显示从这里剥离了,这里应该是每次显示的时候获取一下weather_type,不一样的话从新更新句柄
    pdivoom_light_env_handle->penv_read(DIVOOM_DISP_ENV_WEATHER, (void *)&cur_weather_type);
	if(cur_weather_type == 0)
	{
		divoom_print("%s--%d: Wrong Get DIVOOM_DISP_ENV_WEATHER value!\n", __func__, __LINE__);
	}
	else
	{
		if(cur_weather_type != pdivoom_light_env_handle->weather_type || refresh_flag)
		{
			//更新新的天气type和decode_handle
			pdivoom_light_env_handle->weather_type = cur_weather_type;
			divoom_light_env_update_weather_gif();
		}
	}
    if(pdivoom_light_env_handle->pic_id >= pdivoom_light_env_handle->pic_num)
    {
        pdivoom_light_env_handle->pic_id = 0;
        divoom_image_decode_reset(pdivoom_light_env_handle->decode_handle);
    }
    if(pdivoom_light_env_handle->define_pic_info)
    {
        uint8  *pic_data;
        uint16 pic_len;        
        pic_data = divoom_image_decode_get_pic_data(pdivoom_light_env_handle->decode_handle, &pic_len, &time);
        divoom_screen_disp_divoom_data(pic_data);
    }    
    pdivoom_light_env_handle->pic_id ++;    
}


/*****************************************************************************
 Prototype    : divoom_light_env_get_gif_from_table
 Description  : 从列表取动画数据
 Input        : uint8 index  
 Output       : None
 Return Value : 
 0对应的是老的16*16的天气数据,1对应的是有32的数据
 
  History        :
  1.Date         : 2017/8/26
    Author       : coronaa
    Modification : Created function
*****************************************************************************/
Pdivoom_light_env_gif_info divoom_light_env_get_gif_from_table(uint8 index, uint8 flag)
{
    uint8  *pic_data = NULL;
    uint16 pic_len;
    Pdivoom_light_env_gif_info ptemp_info;    
	//因为APP那边是从1开始的
	index -= 1;
	if(flag == 0)
	{
		if(index >= (sizeof(divoom_light_env_small_weather_picture_table)/sizeof(divoom_light_env_small_weather_picture_table[0])))
		{
			divoom_print("env_gif_index_err:%d\n", index);//越界警告
			return NULL;
		}
	}
	else
	{
		if(index >= (sizeof(divoom_light_env_weather_picture_table)/sizeof(divoom_light_env_weather_picture_table[0])))
		{
			divoom_print("env_gif_index_err:%d\n", index);//越界警告
			return NULL;
		}
	}
	if(flag == 0)
	{
		pic_data = divoom_disp_picture_get_raw_data(divoom_light_env_small_weather_picture_table[index], &pic_len);
	}
	else
	{
		pic_data = divoom_disp_picture_get_raw_data(divoom_light_env_weather_picture_table[index], &pic_len);
	}
    if(pic_data == NULL)
    {
		divoom_print("env_weather_get_err:%d\n", __LINE__);
        return NULL;
    }
    ptemp_info = (Pdivoom_light_env_gif_info)DIVOOM_DISP_ENV_MALLOC(sizeof(divoom_light_env_gif_info));
    if(ptemp_info == NULL)
    {
        divoom_print("%s:%d: it fails to malloc!\n",__FILE__, __LINE__);
        return NULL; 
    }
    if(ptemp_info)
    {
        ptemp_info->len = pic_len;
        ptemp_info->data = pic_data;
    }
    return ptemp_info;    
}


/*****************************************************************************
 Prototype    : divoom_light_env_update_weather_gif
 Description  : 更新动画
 Input        : void  
 Output       : None
 Return Value : 
 
  History        :
  1.Date         : 2017/8/26
    Author       : coronaa
    Modification : Created function
*****************************************************************************/

void divoom_light_env_update_weather_gif_just_small_gif(void)
{
    Pdivoom_light_env_gif_info temp_info;
//    AK_DEBUG_OUTPUT("update_weather_gif\n");    
    if(pdivoom_light_env_handle->define_pic_info)
    {
        DIVOOM_DISP_ENV_FREE(pdivoom_light_env_handle->define_pic_info);
        pdivoom_light_env_handle->define_pic_info = NULL;
    }
    temp_info = divoom_light_env_get_gif_from_table(pdivoom_light_env_handle->weather_type, 0);
    if(temp_info != NULL)    
    {       
        pdivoom_light_env_handle->define_pic_info = temp_info;
        pdivoom_light_env_handle->pic_num = divoom_image_decode_add_data(pdivoom_light_env_handle->decode_handle,temp_info->data ,temp_info->len, false);  
//        AK_DEBUG_OUTPUT("pic_num:%d\n",pdivoom_disp_temp_handle->pic_num);            
    }
    pdivoom_light_env_handle->pic_id = 0;
}   
void divoom_light_env_update_weather_gif(void)
{
    Pdivoom_light_env_gif_info temp_info;
//    AK_DEBUG_OUTPUT("update_weather_gif\n");    
    if(pdivoom_light_env_handle->define_pic_info)
    {
        DIVOOM_DISP_ENV_FREE(pdivoom_light_env_handle->define_pic_info);
        pdivoom_light_env_handle->define_pic_info = NULL;
    }
    temp_info = divoom_light_env_get_gif_from_table(pdivoom_light_env_handle->weather_type, 1);
    if(temp_info != NULL)    
    {       
        pdivoom_light_env_handle->define_pic_info = temp_info;
        pdivoom_light_env_handle->pic_num = divoom_image_decode_add_data(pdivoom_light_env_handle->decode_handle,temp_info->data ,temp_info->len, false);  
//        AK_DEBUG_OUTPUT("pic_num:%d\n",pdivoom_disp_temp_handle->pic_num);            
    }
    pdivoom_light_env_handle->pic_id = 0;
}   

//-----------------------------------------------------------------------------------------------------
//日期显示模块

//字模
const uint8 divoom_light_letter_a_disp_info[5] = {0x07,0x05,0x07,0x05,0x05};	//3X5	A
const uint8 divoom_light_letter_d_disp_info[5] = {0x0E,0x09,0x09,0x09,0x0E};   // 4X5     D
const uint8 divoom_light_letter_e_disp_info[5] = {0x07,0x04,0x07,0x04,0x07};	//3X5	E
const uint8 divoom_light_letter_f_disp_info[5] = {0x07,0x04,0x07,0x04,0x04};	//3X5	F
const uint8 divoom_light_letter_h_disp_info[5] = {0x05,0x05,0x07,0x05,0x05};	//3X5	H
const uint8 divoom_light_letter_i_disp_info[5] = {0x04,0x04,0x04,0x04,0x04};
const uint8 divoom_light_letter_i_disp_info_mid[5] = {0x02,0x02,0x02,0x02,0x02};	//3X5	I
const uint8 divoom_light_letter_m_disp_info[5] = {0x11,0x1b,0x1f,0x15,0x11};   // 5X5     M
const uint8 divoom_light_letter_n_disp_info[5] = {0x09,0x0d,0x0f,0x0b,0x09};   // 4X5     N
const uint8 divoom_light_letter_o_disp_info[5] = {0x07,0x05,0x05,0x05,0x07};	//3X5	  O
const uint8 divoom_light_letter_r_disp_info[5] = {0x0e,0x09,0x0f,0x0a,0x09};   // 4X5     R
const uint8 divoom_light_letter_s_disp_info[5] = {0x07,0x04,0x07,0x01,0x07};	//3X5	 S
const uint8 divoom_light_letter_t_disp_info[5] = {0x07,0x02,0x02,0x02,0x02};	//3X5	T
const uint8 divoom_light_letter_t_5X5_disp_info[5] = {0x1f,0x04,0x04,0x04,0x04};  // 5X5    T
 
const uint8 divoom_light_letter_u_disp_info[5] = {0x05,0x05,0x05,0x05,0x07};	//3X5	  U
const uint8 divoom_light_letter_w_disp_info[5] = {0x11,0x15,0x1f,0x1b,0x11};   // 5X5     W

const uint8 * const divoom_light_env_week_mode1[] = {
    divoom_light_letter_m_disp_info,
    divoom_light_letter_t_5X5_disp_info,
    divoom_light_letter_w_disp_info,
    divoom_light_letter_t_5X5_disp_info,
    divoom_light_letter_f_disp_info,
    divoom_light_letter_s_disp_info,
    divoom_light_letter_s_disp_info,
};

const uint8 * const divoom_light_env_week_mode2[][3] = {
    {
		//DIVOOM_LIGHT_ENV_MON
		//5 + 4 + 4 = 13
        divoom_light_letter_m_disp_info,
        divoom_light_letter_o_disp_info,
        divoom_light_letter_n_disp_info,
    },
    {
		//DIVOOM_LIGHT_ENV_TUE
		//3 + 3 + 3 = 9
        divoom_light_letter_t_disp_info,
        divoom_light_letter_u_disp_info,
        divoom_light_letter_e_disp_info,
    },
    {
		//DIVOOM_LIGHT_ENV_WED
		//5 + 3 + 4 = 12
        divoom_light_letter_w_disp_info,
        divoom_light_letter_e_disp_info,
        divoom_light_letter_d_disp_info,
    },
    {
		//DIVOOM_LIGHT_ENV_THU
		//3 + 3 + 3 = 9
        divoom_light_letter_t_disp_info,
        divoom_light_letter_h_disp_info,
        divoom_light_letter_u_disp_info,
    },
    {
		//DIVOOM_LIGHT_ENV_FRI
		//3 + 4 + 3 = 10
        divoom_light_letter_f_disp_info,
        divoom_light_letter_r_disp_info,
        divoom_light_letter_i_disp_info_mid,
    },
    {
		//DIVOOM_LIGHT_ENV_SAT
		//3 + 3 + 3 = 9
        divoom_light_letter_s_disp_info,
        divoom_light_letter_a_disp_info,
        divoom_light_letter_t_disp_info,
    },
    {
		//DIVOOM_LIGHT_ENV_SUN
		//3 + 4 + 3 = 10
        divoom_light_letter_s_disp_info,
        divoom_light_letter_u_disp_info,
        divoom_light_letter_n_disp_info,
    },
};

//阴影字模，相对正常的字模多出1行空的数据
#if(SCREEN_WIDTH_32 == 16)
const uint8 divoom_light_letter_shadow_a_disp_info[6] = {0x07,0x05,0x07,0x05,0x05,0x00};
const uint8 divoom_light_letter_shadow_d_disp_info[6] = {0x0E,0x09,0x09,0x09,0x0E,0x00};   // 4X6     D
const uint8 divoom_light_letter_shadow_e_disp_info[6] = {0x07,0x04,0x07,0x04,0x07,0x00};
const uint8 divoom_light_letter_shadow_f_disp_info[6] = {0x07,0x04,0x07,0x04,0x04,0x00};
const uint8 divoom_light_letter_shadow_h_disp_info[6] = {0x05,0x05,0x07,0x05,0x05,0x00};
const uint8 divoom_light_letter_shadow_i_disp_info[6] = {0x04,0x04,0x04,0x04,0x04,0x00};
const uint8 divoom_light_letter_shadow_m_disp_info[6] = {0x11,0x1b,0x1f,0x15,0x11,0x00};   // 5X6     M
const uint8 divoom_light_letter_shadow_n_disp_info[6] = {0x09,0x0d,0x0f,0x0b,0x09,0x00};   // 4X6     N
const uint8 divoom_light_letter_shadow_o_disp_info[6] = {0x07,0x05,0x05,0x05,0x07,0x00};
const uint8 divoom_light_letter_shadow_r_disp_info[6] = {0x0e,0x09,0x0f,0x0a,0x09,0x00};   // 4X6     R
const uint8 divoom_light_letter_shadow_s_disp_info[6] = {0x07,0x04,0x07,0x01,0x07,0x00};
const uint8 divoom_light_letter_shadow_t_disp_info[6] = {0x07,0x02,0x02,0x02,0x02,0x00};
const uint8 divoom_light_letter_shadow_t_5X5_disp_info[6] = {0x1f,0x04,0x04,0x04,0x04,0x00};  // 5X6    T 
const uint8 divoom_light_letter_shadow_u_disp_info[6] = {0x05,0x05,0x05,0x05,0x07,0x00};
const uint8 divoom_light_letter_shadow_w_disp_info[6] = {0x11,0x15,0x1f,0x1b,0x11,0x00};   // 5X6     W
#else
//8X12
#define LETTER_SIZE_X			(8)
#define LETTER_SIZE_Y			(12)
const uint8 divoom_light_letter_shadow_a_disp_info[13] = {126, 255, 195, 195, 195, 255, 255, 195, 195, 195, 195, 195, 0};
const uint8 divoom_light_letter_shadow_d_disp_info[13] = {252, 254, 195, 195, 195, 195, 195, 195, 195, 195, 254, 252, 0};
const uint8 divoom_light_letter_shadow_e_disp_info[13] = {255, 255, 192, 192, 192, 255, 255, 192, 192, 192, 255, 255, 0};
const uint8 divoom_light_letter_shadow_f_disp_info[13] = {255, 255, 192, 192, 192, 255, 255, 192, 192, 192, 192, 192, 0};
const uint8 divoom_light_letter_shadow_h_disp_info[13] = {195, 195, 195, 195, 255, 255, 195, 195, 195, 195, 195, 195, 0};
const uint8 divoom_light_letter_shadow_i_disp_info[13] = {60, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 60, 0};
const uint8 divoom_light_letter_shadow_m_disp_info[13] = {195, 231, 231, 255, 255, 219, 219, 195, 195, 195, 195, 195, 0};
const uint8 divoom_light_letter_shadow_n_disp_info[13] = {195, 227, 227, 243, 243, 219, 219, 207, 207, 199, 199, 195, 0};
const uint8 divoom_light_letter_shadow_o_disp_info[13] = {60, 126, 231, 195, 195, 195, 195, 195, 195, 231, 126, 60, 0};
const uint8 divoom_light_letter_shadow_r_disp_info[13] = {252, 254, 195, 195, 195, 255, 254, 248, 220, 206, 199, 195, 0};
const uint8 divoom_light_letter_shadow_s_disp_info[13] = {127, 255, 192, 192, 192, 254, 127, 3, 3, 3, 255, 254, 0};
const uint8 divoom_light_letter_shadow_t_disp_info[13] = {255, 255, 24, 24, 24, 24, 24, 24, 24, 24, 24, 24, 0};
const uint8 divoom_light_letter_shadow_u_disp_info[13] = {195, 195, 195, 195, 195, 195, 195, 195, 195, 195, 255, 126, 0};
const uint8 divoom_light_letter_shadow_w_disp_info[13] = {195, 195, 195, 195, 219, 219, 255, 255, 231, 231, 195, 195, 0};
#endif


const uint8 * const divoom_light_env_week_mode2_shadow[][3] = {
    {
        divoom_light_letter_shadow_m_disp_info,
        divoom_light_letter_shadow_o_disp_info,
        divoom_light_letter_shadow_n_disp_info,
    },
    {
        divoom_light_letter_shadow_t_disp_info,
        divoom_light_letter_shadow_u_disp_info,
        divoom_light_letter_shadow_e_disp_info,
    },
    {
        divoom_light_letter_shadow_w_disp_info,
        divoom_light_letter_shadow_e_disp_info,
        divoom_light_letter_shadow_d_disp_info,
    },
    {
        divoom_light_letter_shadow_t_disp_info,
        divoom_light_letter_shadow_h_disp_info,
        divoom_light_letter_shadow_u_disp_info,
    },
    {
        divoom_light_letter_shadow_f_disp_info,
        divoom_light_letter_shadow_r_disp_info,
        divoom_light_letter_shadow_i_disp_info,
    },
    {
        divoom_light_letter_shadow_s_disp_info,
        divoom_light_letter_shadow_a_disp_info,
        divoom_light_letter_shadow_t_disp_info,
    },
    {
        divoom_light_letter_shadow_s_disp_info,
        divoom_light_letter_shadow_u_disp_info,
        divoom_light_letter_shadow_n_disp_info,
    },
};



/*****************************************************************************
 Prototype    : divoom_light_env_date_draw_week_mode1
 Description  : 主题1 画出周X
 Input        : uint8 week  
 Output       : None
 Return Value : 
 
  History        :
  1.Date         : 2017/10/31
    Author       : coronaa
    Modification : Created function
*****************************************************************************/
void divoom_light_env_date_draw_week_mode1(uint8 week)
{    
    switch(week)
    {
        case DIVOOM_LIGHT_ENV_MON:
        case DIVOOM_LIGHT_ENV_TUE:            
        case DIVOOM_LIGHT_ENV_WED:   
        case DIVOOM_LIGHT_ENV_THU:               
            divoom_disp_font_fill_area(1,5,5,5,divoom_light_env_week_mode1[week],pdivoom_light_env_handle->disp_buf,&divoom_env_red_color);
            break;
        case DIVOOM_LIGHT_ENV_SAT:
        case DIVOOM_LIGHT_ENV_SUN:
        case DIVOOM_LIGHT_ENV_FRI:
            divoom_disp_font_fill_area(2,5,3,5,divoom_light_env_week_mode1[week],pdivoom_light_env_handle->disp_buf,&divoom_env_red_color);
            break;
    }        
}

#if(SCREEN_WIDTH_32 == 32)
void divoom_disp_font_fill_big_area_shadow(uint8 start_x, uint8 start_y, const uint8 *digit_info, uint8 *disp_buf, const divoom_screen_color_info *color)
{
	divoom_disp_font_fill_area_shadow(start_x, start_y, LETTER_SIZE_X, LETTER_SIZE_Y, (uint8 *)digit_info, disp_buf, (divoom_screen_color_info *)color);
}

#endif

/*****************************************************************************
 Prototype    : divoom_light_env_date_draw_week_mode2
 Description  : 主题2 画出周X
 Input        : uint8 week  
 Output       : None
 Return Value : 
 
  History        :
  1.Date         : 2017/10/31
    Author       : coronaa
    Modification : Created function
*****************************************************************************/
void divoom_light_env_date_draw_week_mode2(uint8 week)
{
#if(SCREEN_HEIGHT_32 == 32)
	divoom_disp_font_fill_big_area_shadow(2, 1, divoom_light_env_week_mode2_shadow[week][0], pdivoom_light_env_handle->disp_buf, &divoom_env_red_color);
	divoom_disp_font_fill_big_area_shadow(12, 1, divoom_light_env_week_mode2_shadow[week][1], pdivoom_light_env_handle->disp_buf, &divoom_env_red_color);
	divoom_disp_font_fill_big_area_shadow(22, 1, divoom_light_env_week_mode2_shadow[week][2], pdivoom_light_env_handle->disp_buf, &divoom_env_red_color);
	return;
#endif
    switch(week)
    {
        case DIVOOM_LIGHT_ENV_MON:            
        case DIVOOM_LIGHT_ENV_WED:            
            divoom_disp_font_fill_area_shadow(1,1,5,6,divoom_light_env_week_mode2_shadow[week][0],pdivoom_light_env_handle->disp_buf,&divoom_env_red_color);
            divoom_disp_font_fill_area_shadow(7,1,3,6,divoom_light_env_week_mode2_shadow[week][1],pdivoom_light_env_handle->disp_buf,&divoom_env_red_color);
            divoom_disp_font_fill_area_shadow(11,1,4,6,divoom_light_env_week_mode2_shadow[week][2],pdivoom_light_env_handle->disp_buf,&divoom_env_red_color);            
            
            break;

        case DIVOOM_LIGHT_ENV_TUE:
        case DIVOOM_LIGHT_ENV_THU:
        case DIVOOM_LIGHT_ENV_SAT:
            divoom_disp_font_fill_area_shadow(2,1,3,6,divoom_light_env_week_mode2_shadow[week][0],pdivoom_light_env_handle->disp_buf,&divoom_env_red_color);
            divoom_disp_font_fill_area_shadow(6,1,3,6,divoom_light_env_week_mode2_shadow[week][1],pdivoom_light_env_handle->disp_buf,&divoom_env_red_color);
            divoom_disp_font_fill_area_shadow(10,1,3,6,divoom_light_env_week_mode2_shadow[week][2],pdivoom_light_env_handle->disp_buf,&divoom_env_red_color);            
            break;
            

        case DIVOOM_LIGHT_ENV_SUN:
            divoom_disp_font_fill_area_shadow(2,1,3,6,divoom_light_env_week_mode2_shadow[week][0],pdivoom_light_env_handle->disp_buf,&divoom_env_red_color);
            divoom_disp_font_fill_area_shadow(6,1,3,6,divoom_light_env_week_mode2_shadow[week][1],pdivoom_light_env_handle->disp_buf,&divoom_env_red_color);
            divoom_disp_font_fill_area_shadow(10,1,4,6,divoom_light_env_week_mode2_shadow[week][2],pdivoom_light_env_handle->disp_buf,&divoom_env_red_color);            
            break;
            
        case DIVOOM_LIGHT_ENV_FRI:
            divoom_disp_font_fill_area_shadow(3,1,3,6,divoom_light_env_week_mode2_shadow[week][0],pdivoom_light_env_handle->disp_buf,&divoom_env_red_color);
            divoom_disp_font_fill_area_shadow(7,1,4,6,divoom_light_env_week_mode2_shadow[week][1],pdivoom_light_env_handle->disp_buf,&divoom_env_red_color);
            divoom_disp_font_fill_area_shadow(12,1,3,6,divoom_light_env_week_mode2_shadow[week][2],pdivoom_light_env_handle->disp_buf,&divoom_env_red_color);            
            break;
    }      
}  

/*****************************************************************************
 Prototype    : divoom_light_env_date_mode1
 Description  : 日期显示主题1
 Input        : uint8 day  
                uint8 week  
 Output       : None
 Return Value : 
 
  History        :
  1.Date         : 2017/10/31
    Author       : coronaa
    Modification : Created function
*****************************************************************************/
void divoom_light_env_date_mode1(uint8 day,uint8 week)
{
    divoom_light_color_info color;
    memcpy(&color, &pdivoom_light_env_handle->color,sizeof(divoom_light_color_info));
    divoom_light_env_date_draw_week_mode1(week);
    divoom_disp_digit_fill_area(8, 5,day/10,
        DIVOOM_3X5_DIGIT, pdivoom_light_env_handle->disp_buf,&color);
    divoom_disp_digit_fill_area(12, 5,day%10,
        DIVOOM_3X5_DIGIT, pdivoom_light_env_handle->disp_buf,&color);   
}

/*****************************************************************************
 Prototype    : divoom_light_env_date_mode2
 Description  : 日期显示主题2
 Input        : Pdivoom_light_env_date_info date  
 Output       : None
 Return Value : 
 
  History        :
  1.Date         : 2017/10/31
    Author       : coronaa
    Modification : Created function
*****************************************************************************/
void divoom_light_env_date_mode2(Pdivoom_light_env_date_info date)

{
    divoom_light_color_info color;
    memcpy(&color, &divoom_env_write_color, sizeof(divoom_light_color_info));
    divoom_light_env_date_draw_week_mode2(date->week);    

    if((date->month/10) != 0)   // 10以上月份位置排布不同
    {        
#if(SCREEN_HEIGHT_32 == 32)
        divoom_disp_digit_fill_area_shadow(1, 17, date->month/10,
            DIVOOM_6X12_DIGIT, pdivoom_light_env_handle->disp_buf, &color);

        divoom_disp_digit_fill_area_shadow(8, 17, date->month%10,
            DIVOOM_6X12_DIGIT, pdivoom_light_env_handle->disp_buf, &color);

        divoom_disp_ctrl_set_data_xy(15, 21, &color, pdivoom_light_env_handle->disp_buf);
        divoom_disp_ctrl_set_data_xy(16, 21, &color, pdivoom_light_env_handle->disp_buf);
        divoom_disp_ctrl_set_data_xy(15, 22, &color, pdivoom_light_env_handle->disp_buf);
        divoom_disp_ctrl_set_data_xy(16, 22, &color, pdivoom_light_env_handle->disp_buf);
        divoom_disp_ctrl_set_data_xy(15, 23, &divoom_env_black_color, pdivoom_light_env_handle->disp_buf);
        divoom_disp_ctrl_set_data_xy(16, 23, &divoom_env_black_color, pdivoom_light_env_handle->disp_buf);

        divoom_disp_digit_fill_area_shadow(18, 17,date->day/10,
            DIVOOM_6X12_DIGIT, pdivoom_light_env_handle->disp_buf,&color);
        divoom_disp_digit_fill_area_shadow(25, 17,date->day%10,
            DIVOOM_6X12_DIGIT, pdivoom_light_env_handle->disp_buf,&color);        
#else
        divoom_light_gfx_drawFastVLine(1, 9, 5, &color, pdivoom_light_env_handle->disp_buf);
        divoom_disp_ctrl_set_data_xy(1, 14, &divoom_env_black_color, pdivoom_light_env_handle->disp_buf);
        
        divoom_disp_digit_fill_area_shadow(3, 9,date->month%10,
            DIVOOM_3X5_DIGIT, pdivoom_light_env_handle->disp_buf,&color);
        
        divoom_disp_ctrl_set_data_xy(7, 11, &color, pdivoom_light_env_handle->disp_buf);
        divoom_disp_ctrl_set_data_xy(7, 12, &divoom_env_black_color, pdivoom_light_env_handle->disp_buf);
        
        divoom_disp_digit_fill_area_shadow(9, 9,date->day/10,
            DIVOOM_3X5_DIGIT, pdivoom_light_env_handle->disp_buf,&color);
        divoom_disp_digit_fill_area_shadow(13, 9,date->day%10,
            DIVOOM_3X5_DIGIT, pdivoom_light_env_handle->disp_buf,&color);        
#endif
    }
    else
    {
#if(SCREEN_HEIGHT_32 == 32)
        divoom_disp_digit_fill_area_shadow(4, 17, date->month%10,
            DIVOOM_6X12_DIGIT, pdivoom_light_env_handle->disp_buf, &color);

        divoom_disp_ctrl_set_data_xy(11, 21, &color, pdivoom_light_env_handle->disp_buf);
        divoom_disp_ctrl_set_data_xy(12, 21, &color, pdivoom_light_env_handle->disp_buf);
        divoom_disp_ctrl_set_data_xy(11, 22, &color, pdivoom_light_env_handle->disp_buf);
        divoom_disp_ctrl_set_data_xy(12, 22, &color, pdivoom_light_env_handle->disp_buf);
        divoom_disp_ctrl_set_data_xy(11, 23, &divoom_env_black_color, pdivoom_light_env_handle->disp_buf);
        divoom_disp_ctrl_set_data_xy(12, 23, &divoom_env_black_color, pdivoom_light_env_handle->disp_buf);

        divoom_disp_digit_fill_area_shadow(14, 17, date->day/10,
            DIVOOM_6X12_DIGIT, pdivoom_light_env_handle->disp_buf,&color);
        divoom_disp_digit_fill_area_shadow(23, 17,date->day%10,
            DIVOOM_6X12_DIGIT, pdivoom_light_env_handle->disp_buf,&color);        
#else
        divoom_disp_digit_fill_area_shadow(2, 9,date->month%10,
            DIVOOM_3X5_DIGIT, pdivoom_light_env_handle->disp_buf,&color);
        
        divoom_disp_ctrl_set_data_xy(6, 11, &color, pdivoom_light_env_handle->disp_buf);
        divoom_disp_ctrl_set_data_xy(6, 12, &divoom_env_black_color, pdivoom_light_env_handle->disp_buf);
        
        divoom_disp_digit_fill_area_shadow(8, 9,date->day/10,
            DIVOOM_3X5_DIGIT, pdivoom_light_env_handle->disp_buf,&color);
        divoom_disp_digit_fill_area_shadow(12, 9,date->day%10,
            DIVOOM_3X5_DIGIT, pdivoom_light_env_handle->disp_buf,&color);       
#endif
    }
} 

/*****************************************************************************
 Prototype    : divoom_light_env_date_set_mode
 Description  : 选择日期显示主题
 Input        : uint8 mode  
 Output       : None
 Return Value : 
 
  History        :
  1.Date         : 2017/10/31
    Author       : coronaa
    Modification : Created function
*****************************************************************************/
void divoom_light_env_date_set_mode(uint8 mode)
{

    divoom_disp_picture_get_rgb_data(DIVOOM_DISP_DIVOOM_NEW_DATA_NACKGRAND, pdivoom_light_env_handle->disp_buf);            
    divoom_light_env_date_mode2(&pdivoom_light_env_handle->date_info);             
/*
    switch(mode)
    {
//        case DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_CIRCULAR:
//        case DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_SQUARE:
//            divoom_light_env_date_mode2(pdivoom_light_env_handle->date_info.date,pdivoom_light_env_handle->date_info.week);    
//            break; 
        case DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_COLOURFUL:
            //先清屏
    		memset(pdivoom_light_env_handle->disp_buf, 0, sizeof(pdivoom_light_env_handle->disp_buf));            
            divoom_light_env_date_mode1(pdivoom_light_env_handle->date_info.day,pdivoom_light_env_handle->date_info.week);    
            break;
        default:
            //先解出背景图
//            divoom_disp_picture_get_rgb_data(DIVOOM_DISP_DATE_BACKGROUND,pdivoom_light_env_handle->disp_buf);            
            divoom_disp_picture_decode_rgb_data(pdivoom_light_env_handle->decode_handle,DIVOOM_DISP_DATE_BACKGROUND,pdivoom_light_env_handle->disp_buf);            
            divoom_light_env_date_mode2(&pdivoom_light_env_handle->date_info);             
            break;
    }
*/    
}    

/*****************************************************************************
 Prototype    : divoom_light_env_date
 Description  : 显示日期模块
 Input        : bool refresh_flag  
 Output       : None
 Return Value : 
 
  History        :
  1.Date         : 2017/10/31
    Author       : coronaa
    Modification : Created function
*****************************************************************************/
void  divoom_light_env_date(bool refresh_flag)
{
    divoom_light_env_date_info cur_info;
#if 1
    pdivoom_light_env_handle->penv_read(DIVOOM_DISP_ENV_DATE, (void *)&cur_info);
    
    if(cur_info.day != pdivoom_light_env_handle->date_info.day || cur_info.week!= pdivoom_light_env_handle->date_info.week)
    {
        //如果显示数据发生变化，将刷新
        pdivoom_light_env_handle->date_info.day = cur_info.day;
        pdivoom_light_env_handle->date_info.week = cur_info.week;
        pdivoom_light_env_handle->date_info.month= cur_info.month;        
//        refresh_flag = true;
    }
#endif  
    divoom_light_env_date_set_mode(pdivoom_light_env_handle->effect_type);
}    

#if(SCREEN_WIDTH_32 == 32)
//根据模式,把时间信息写到不同位置
//显示3X5的字母
void divoom_light_env_time_set_mode(Pdivoom_light_env_time_info time_info, uint8 mode, uint8* buf)
{
	if(pdivoom_light_env_handle->hour_flag == 0)
	{
		if(time_info->hour != 12)
		{
			time_info->hour = time_info->hour % 12;
			if(time_info->hour == 0)
			{
				time_info->hour = 12;
			}
		}
	}
	//这个还是做成小于10的时候前面显示0
	switch(mode)
	{
		case DIVOOM_DISP_ENV_TIME_CELS_DATA:
			{
				//set two dot
				divoom_disp_ctrl_set_dot(12, 3, (Pdivoom_screen_color_info)&divoom_env_black_color, buf);
				divoom_disp_ctrl_set_dot(12, 5, (Pdivoom_screen_color_info)&divoom_env_black_color, buf);

				//小时
				divoom_disp_digit_fill_area(ENV_FOUR_LINE_TIME_ONE_X, ENV_FOUR_LINE_TIME_Y, time_info->hour/10, 
						DIVOOM_3X5_DIGIT, buf, (Pdivoom_screen_color_info)&divoom_env_black_color);
				divoom_disp_digit_fill_area(ENV_FOUR_LINE_TIME_TWO_X, ENV_FOUR_LINE_TIME_Y, time_info->hour%10, 
						DIVOOM_3X5_DIGIT, buf, (Pdivoom_screen_color_info)&divoom_env_black_color);
				//分钟
				divoom_disp_digit_fill_area(ENV_FOUR_LINE_TIME_THREE_X, ENV_FOUR_LINE_TIME_Y, time_info->min/10, 
						DIVOOM_3X5_DIGIT, buf, (Pdivoom_screen_color_info)&divoom_env_black_color);
				divoom_disp_digit_fill_area(ENV_FOUR_LINE_TIME_FOUR_X, ENV_FOUR_LINE_TIME_Y, time_info->min%10, 
						DIVOOM_3X5_DIGIT, buf, (Pdivoom_screen_color_info)&divoom_env_black_color);
			}
			break;
		case DIVOOM_DISP_ENV_TIME_CELS_DATA_WEATHER:
			{
				//小时
				divoom_disp_digit_fill_area(ENV_FOUR_BLOCK_DISP_TIME_BIG_X, ENV_FOUR_BLOCK_DISP_ONE_ROW, time_info->hour/10, 
						DIVOOM_3X5_DIGIT, buf, (Pdivoom_screen_color_info)&divoom_env_black_color);
				divoom_disp_digit_fill_area(ENV_FOUR_BLOCK_DISP_TIME_LITTLE_X, ENV_FOUR_BLOCK_DISP_ONE_ROW, time_info->hour%10, 
						DIVOOM_3X5_DIGIT, buf, (Pdivoom_screen_color_info)&divoom_env_black_color);
				//分钟
				divoom_disp_digit_fill_area(ENV_FOUR_BLOCK_DISP_TIME_BIG_X, ENV_FOUR_BLOCK_DISP_TWO_ROW, time_info->min/10, 
						DIVOOM_3X5_DIGIT, buf, (Pdivoom_screen_color_info)&divoom_env_black_color);
				divoom_disp_digit_fill_area(ENV_FOUR_BLOCK_DISP_TIME_LITTLE_X, ENV_FOUR_BLOCK_DISP_TWO_ROW, time_info->min%10, 
						DIVOOM_3X5_DIGIT, buf, (Pdivoom_screen_color_info)&divoom_env_black_color);
			}
			break;
	}
}

//显示温度信息
void divoom_light_env_celsius_set_mode(int celsius_info, uint8 mode, uint8 *buf)
{
	int c_f_x = 0;
	if(pdivoom_light_env_handle->temp_flag == 1)
	{
		celsius_info = celsius_info *18/10 + 32;
	}
	if(celsius_info < -99)
		celsius_info = -99;
	else if(celsius_info > 199)
		celsius_info = 199;
	switch(mode)
	{
		case DIVOOM_DISP_ENV_TIME_CELS_DATA:
			{
				if(celsius_info < -9)
				{
					divoom_disp_ctrl_set_dot(18, 18, (Pdivoom_screen_color_info)&divoom_env_black_color, buf);
					//先写负号
					divoom_disp_ctrl_set_dot(ENV_FOUR_LINE_TEMP_ONE_X, ENV_FOUR_LINE_TEMP_Y + 2, (Pdivoom_screen_color_info)&divoom_env_black_color, buf);
					divoom_disp_ctrl_set_dot(ENV_FOUR_LINE_TEMP_ONE_X + 1, ENV_FOUR_LINE_TEMP_Y + 2, (Pdivoom_screen_color_info)&divoom_env_black_color, buf);
					divoom_disp_ctrl_set_dot(ENV_FOUR_LINE_TEMP_ONE_X + 2, ENV_FOUR_LINE_TEMP_Y + 2, (Pdivoom_screen_color_info)&divoom_env_black_color, buf);

					divoom_disp_digit_fill_area(ENV_FOUR_LINE_TEMP_TWO_X, ENV_FOUR_LINE_TEMP_Y, celsius_info/10, 
							DIVOOM_3X5_DIGIT, buf, (Pdivoom_screen_color_info)&divoom_env_black_color);
					divoom_disp_digit_fill_area(ENV_FOUR_LINE_TEMP_THREE_X, ENV_FOUR_LINE_TEMP_Y, celsius_info%10, 
							DIVOOM_3X5_DIGIT, buf, (Pdivoom_screen_color_info)&divoom_env_black_color);
					c_f_x = 20;
				}
				else if(celsius_info < 0)
				{
					divoom_disp_ctrl_set_dot(13, 18, (Pdivoom_screen_color_info)&divoom_env_black_color, buf);
					divoom_disp_ctrl_set_dot(ENV_FOUR_LINE_TEMP_ONE_X, ENV_FOUR_LINE_TEMP_Y + 2, (Pdivoom_screen_color_info)&divoom_env_black_color, buf);
					divoom_disp_ctrl_set_dot(ENV_FOUR_LINE_TEMP_ONE_X + 1, ENV_FOUR_LINE_TEMP_Y + 2, (Pdivoom_screen_color_info)&divoom_env_black_color, buf);
					divoom_disp_ctrl_set_dot(ENV_FOUR_LINE_TEMP_ONE_X + 2, ENV_FOUR_LINE_TEMP_Y + 2, (Pdivoom_screen_color_info)&divoom_env_black_color, buf);

					divoom_disp_digit_fill_area(ENV_FOUR_LINE_TEMP_TWO_X, ENV_FOUR_LINE_TEMP_Y, celsius_info, 
							DIVOOM_3X5_DIGIT, buf, (Pdivoom_screen_color_info)&divoom_env_black_color);
					c_f_x = 15;
				}
				else if(celsius_info < 10)
				{
					divoom_disp_ctrl_set_dot(13, 18, (Pdivoom_screen_color_info)&divoom_env_black_color, buf);
					//个位的话显示一个0,不然太空旷了
					divoom_disp_digit_fill_area(ENV_FOUR_LINE_TEMP_ONE_X, ENV_FOUR_LINE_TEMP_Y, 0, 
							DIVOOM_3X5_DIGIT, buf, (Pdivoom_screen_color_info)&divoom_env_black_color);
					divoom_disp_digit_fill_area(ENV_FOUR_LINE_TEMP_TWO_X, ENV_FOUR_LINE_TEMP_Y, celsius_info, 
							DIVOOM_3X5_DIGIT, buf, (Pdivoom_screen_color_info)&divoom_env_black_color);
					c_f_x = 15;
				}
				else if(celsius_info < 100)
				{
					divoom_disp_ctrl_set_dot(13, 18, (Pdivoom_screen_color_info)&divoom_env_black_color, buf);
					divoom_disp_digit_fill_area(ENV_FOUR_LINE_TEMP_ONE_X, ENV_FOUR_LINE_TEMP_Y, celsius_info/10, 
							DIVOOM_3X5_DIGIT, buf, (Pdivoom_screen_color_info)&divoom_env_black_color);
					divoom_disp_digit_fill_area(ENV_FOUR_LINE_TEMP_TWO_X, ENV_FOUR_LINE_TEMP_Y, celsius_info%10, 
							DIVOOM_3X5_DIGIT, buf, (Pdivoom_screen_color_info)&divoom_env_black_color);
					c_f_x = 15;
				}
				else
				{
					divoom_disp_ctrl_set_dot(18, 18, (Pdivoom_screen_color_info)&divoom_env_black_color, buf);
					divoom_disp_ctrl_set_dot(ENV_FOUR_LINE_TEMP_ONE_X+1, ENV_FOUR_LINE_TEMP_Y, (Pdivoom_screen_color_info)&divoom_env_black_color, buf);
					divoom_disp_ctrl_set_dot(ENV_FOUR_LINE_TEMP_ONE_X+1, ENV_FOUR_LINE_TEMP_Y + 1, (Pdivoom_screen_color_info)&divoom_env_black_color, buf);
					divoom_disp_ctrl_set_dot(ENV_FOUR_LINE_TEMP_ONE_X+1, ENV_FOUR_LINE_TEMP_Y + 2, (Pdivoom_screen_color_info)&divoom_env_black_color, buf);
					divoom_disp_ctrl_set_dot(ENV_FOUR_LINE_TEMP_ONE_X+1, ENV_FOUR_LINE_TEMP_Y + 3, (Pdivoom_screen_color_info)&divoom_env_black_color, buf);
					divoom_disp_ctrl_set_dot(ENV_FOUR_LINE_TEMP_ONE_X+1, ENV_FOUR_LINE_TEMP_Y + 4, (Pdivoom_screen_color_info)&divoom_env_black_color, buf);

					divoom_disp_digit_fill_area(ENV_FOUR_LINE_TEMP_TWO_X, ENV_FOUR_LINE_TEMP_Y, celsius_info/10, 
							DIVOOM_3X5_DIGIT, buf, (Pdivoom_screen_color_info)&divoom_env_black_color);
					divoom_disp_digit_fill_area(ENV_FOUR_LINE_TEMP_THREE_X, ENV_FOUR_LINE_TEMP_Y, celsius_info%10, 
							DIVOOM_3X5_DIGIT, buf, (Pdivoom_screen_color_info)&divoom_env_black_color);
					c_f_x = 20;
				}
			}
			break;
		case DIVOOM_DISP_ENV_TIME_CELS_DATA_WEATHER:
			{
				if(celsius_info < -9)
				{
					//先写负号
					//当在这个时候,要整体向右移动一位
					divoom_disp_ctrl_set_dot(ENV_FOUR_BLOCK_TEMP_H, ENV_FOUR_BLOCK_DISP_THREE_ROW + 2, (Pdivoom_screen_color_info)&divoom_env_black_color, buf);
					divoom_disp_ctrl_set_dot(ENV_FOUR_BLOCK_TEMP_H+1, ENV_FOUR_BLOCK_DISP_THREE_ROW + 2, (Pdivoom_screen_color_info)&divoom_env_black_color, buf);

					divoom_disp_digit_fill_area(ENV_FOUR_BLOCK_TEMP_T, ENV_FOUR_BLOCK_DISP_THREE_ROW, celsius_info/10, 
							DIVOOM_3X5_DIGIT, buf, (Pdivoom_screen_color_info)&divoom_env_black_color);
					divoom_disp_digit_fill_area(ENV_FOUR_BLOCK_TEMP_N, ENV_FOUR_BLOCK_DISP_THREE_ROW, celsius_info%10, 
							DIVOOM_3X5_DIGIT, buf, (Pdivoom_screen_color_info)&divoom_env_black_color);
				}
				else if(celsius_info < 0)
				{
					divoom_disp_ctrl_set_dot(ENV_FOUR_BLOCK_TEMP_T, ENV_FOUR_BLOCK_DISP_THREE_ROW + 2, (Pdivoom_screen_color_info)&divoom_env_black_color, buf);
					divoom_disp_ctrl_set_dot(ENV_FOUR_BLOCK_TEMP_T, ENV_FOUR_BLOCK_DISP_THREE_ROW + 2, (Pdivoom_screen_color_info)&divoom_env_black_color, buf);

					divoom_disp_digit_fill_area(ENV_FOUR_BLOCK_TEMP_N, ENV_FOUR_BLOCK_DISP_THREE_ROW, celsius_info, 
							DIVOOM_3X5_DIGIT, buf, (Pdivoom_screen_color_info)&divoom_env_black_color);
				}
				else if(celsius_info < 10)
				{
					divoom_disp_digit_fill_area(ENV_FOUR_BLOCK_TEMP_N, ENV_FOUR_BLOCK_DISP_THREE_ROW, celsius_info, 
							DIVOOM_3X5_DIGIT, buf, (Pdivoom_screen_color_info)&divoom_env_black_color);
				}
				else if(celsius_info < 100)
				{
					divoom_disp_digit_fill_area(ENV_FOUR_BLOCK_TEMP_T, ENV_FOUR_BLOCK_DISP_THREE_ROW, celsius_info/10, 
							DIVOOM_3X5_DIGIT, buf, (Pdivoom_screen_color_info)&divoom_env_black_color);
					divoom_disp_digit_fill_area(ENV_FOUR_BLOCK_TEMP_N, ENV_FOUR_BLOCK_DISP_THREE_ROW, celsius_info%10, 
							DIVOOM_3X5_DIGIT, buf, (Pdivoom_screen_color_info)&divoom_env_black_color);
				}
				else
				{
					divoom_disp_ctrl_set_dot(ENV_FOUR_BLOCK_TEMP_H, ENV_FOUR_BLOCK_DISP_THREE_ROW, (Pdivoom_screen_color_info)&divoom_env_black_color, buf);
					divoom_disp_ctrl_set_dot(ENV_FOUR_BLOCK_TEMP_H, ENV_FOUR_BLOCK_DISP_THREE_ROW + 1, (Pdivoom_screen_color_info)&divoom_env_black_color, buf);
					divoom_disp_ctrl_set_dot(ENV_FOUR_BLOCK_TEMP_H, ENV_FOUR_BLOCK_DISP_THREE_ROW + 2, (Pdivoom_screen_color_info)&divoom_env_black_color, buf);
					divoom_disp_ctrl_set_dot(ENV_FOUR_BLOCK_TEMP_H, ENV_FOUR_BLOCK_DISP_THREE_ROW + 3, (Pdivoom_screen_color_info)&divoom_env_black_color, buf);
					divoom_disp_ctrl_set_dot(ENV_FOUR_BLOCK_TEMP_H, ENV_FOUR_BLOCK_DISP_THREE_ROW + 4, (Pdivoom_screen_color_info)&divoom_env_black_color, buf);

					divoom_disp_digit_fill_area(ENV_FOUR_BLOCK_TEMP_T, ENV_FOUR_BLOCK_DISP_THREE_ROW, celsius_info/10, 
							DIVOOM_3X5_DIGIT, buf, (Pdivoom_screen_color_info)&divoom_env_black_color);
					divoom_disp_digit_fill_area(ENV_FOUR_BLOCK_TEMP_N, ENV_FOUR_BLOCK_DISP_THREE_ROW, celsius_info%10, 
							DIVOOM_3X5_DIGIT, buf, (Pdivoom_screen_color_info)&divoom_env_black_color);
				}
			}
			break;
	}
	if(pdivoom_light_env_handle->temp_flag == 1)
	{
		switch(mode)
		{
			case DIVOOM_DISP_ENV_TIME_CELS_DATA:
				divoom_disp_font_fill_area(c_f_x, ENV_FOUR_LINE_TEMP_Y, 3, 5, (uint8 *)&divoom_light_letter_f_3X5_disp_info[0], 
						buf, (Pdivoom_screen_color_info)&divoom_env_black_color);
				break;
			case DIVOOM_DISP_ENV_TIME_CELS_DATA_WEATHER:
				divoom_disp_font_fill_area(ENV_FOUR_BLOCK_TEMP_C_F_X, ENV_FOUR_BLOCK_TEMP_C_F_Y, 3, 5, (uint8 *)&divoom_light_letter_f_3X5_disp_info[0], 
						buf, (Pdivoom_screen_color_info)&divoom_env_black_color);
				break;
		}
	}
	else
	{
		switch(mode)
		{
			case DIVOOM_DISP_ENV_TIME_CELS_DATA:
				divoom_disp_font_fill_area(c_f_x, ENV_FOUR_LINE_TEMP_Y, 3, 5, (uint8 *)&divoom_light_letter_c_3X5_disp_info[0], 
						buf, (Pdivoom_screen_color_info)&divoom_env_black_color);
				break;
			case DIVOOM_DISP_ENV_TIME_CELS_DATA_WEATHER:
				divoom_disp_font_fill_area(ENV_FOUR_BLOCK_TEMP_C_F_X, ENV_FOUR_BLOCK_TEMP_C_F_Y, 3, 5, (uint8 *)&divoom_light_letter_c_3X5_disp_info[0], 
						buf, (Pdivoom_screen_color_info)&divoom_env_black_color);
				break;
		}
	}
}

//这个是四个方块显示
//新加效果日期显示
void divoom_light_env_date_draw_week_mode3(uint8 week)
{
    switch(week)
    {
        case DIVOOM_LIGHT_ENV_MON:
			//13 = 5 + 4 + 4
            divoom_disp_font_fill_area(ENV_FOUR_BLOCK_DAT_13_ONE, ENV_FOUR_BLOCK_DISP_ONE_ROW, 5, 5, (uint8 *)&divoom_light_env_week_mode2[week][0][0], 
					pdivoom_light_env_handle->disp_buf, (Pdivoom_screen_color_info)&divoom_env_black_color);
            divoom_disp_font_fill_area(ENV_FOUR_BLOCK_DAT_13_TWO, ENV_FOUR_BLOCK_DISP_ONE_ROW, 4, 5, (uint8 *)&divoom_light_env_week_mode2[week][1][0], 
					pdivoom_light_env_handle->disp_buf, (Pdivoom_screen_color_info)&divoom_env_black_color);
            divoom_disp_font_fill_area(ENV_FOUR_BLOCK_DAT_13_THREE, ENV_FOUR_BLOCK_DISP_ONE_ROW, 4, 5, (uint8 *)&divoom_light_env_week_mode2[week][2][0], 
					pdivoom_light_env_handle->disp_buf, (Pdivoom_screen_color_info)&divoom_env_black_color);
            break;
        case DIVOOM_LIGHT_ENV_WED:
			//12 = 5 + 3 + 4
            divoom_disp_font_fill_area(ENV_FOUR_BLOCK_DAT_12_ONE, ENV_FOUR_BLOCK_DISP_ONE_ROW, 5, 5, (uint8 *)&divoom_light_env_week_mode2[week][0][0], 
					pdivoom_light_env_handle->disp_buf, (Pdivoom_screen_color_info)&divoom_env_black_color);
            divoom_disp_font_fill_area(ENV_FOUR_BLOCK_DAT_12_TWO, ENV_FOUR_BLOCK_DISP_ONE_ROW, 3, 5, (uint8 *)&divoom_light_env_week_mode2[week][1][0], 
					pdivoom_light_env_handle->disp_buf, (Pdivoom_screen_color_info)&divoom_env_black_color);
            divoom_disp_font_fill_area(ENV_FOUR_BLOCK_DAT_12_THREE, ENV_FOUR_BLOCK_DISP_ONE_ROW, 4, 5, (uint8 *)&divoom_light_env_week_mode2[week][2][0], 
					pdivoom_light_env_handle->disp_buf, (Pdivoom_screen_color_info)&divoom_env_black_color);
            break;
        case DIVOOM_LIGHT_ENV_TUE:
        case DIVOOM_LIGHT_ENV_THU:
        case DIVOOM_LIGHT_ENV_SAT:
			//9
            divoom_disp_font_fill_area(ENV_FOUR_BLOCK_DAT_9_ONE, ENV_FOUR_BLOCK_DISP_ONE_ROW, 3, 5, (uint8 *)&divoom_light_env_week_mode2[week][0][0], 
					pdivoom_light_env_handle->disp_buf, (Pdivoom_screen_color_info)&divoom_env_black_color);
            divoom_disp_font_fill_area(ENV_FOUR_BLOCK_DAT_9_TWO, ENV_FOUR_BLOCK_DISP_ONE_ROW, 3, 5, (uint8 *)&divoom_light_env_week_mode2[week][1][0], 
					pdivoom_light_env_handle->disp_buf, (Pdivoom_screen_color_info)&divoom_env_black_color);
            divoom_disp_font_fill_area(ENV_FOUR_BLOCK_DAT_9_THREE, ENV_FOUR_BLOCK_DISP_ONE_ROW, 3, 5, (uint8 *)&divoom_light_env_week_mode2[week][2][0], 
					pdivoom_light_env_handle->disp_buf, (Pdivoom_screen_color_info)&divoom_env_black_color);
            break;
        case DIVOOM_LIGHT_ENV_SUN:
			//10 = 3 + 4 + 3
            divoom_disp_font_fill_area(ENV_FOUR_BLOCK_DAT_10_ONE, ENV_FOUR_BLOCK_DISP_ONE_ROW, 3, 5, (uint8 *)&divoom_light_env_week_mode2[week][0][0], 
					pdivoom_light_env_handle->disp_buf, (Pdivoom_screen_color_info)&divoom_env_black_color);
            divoom_disp_font_fill_area(ENV_FOUR_BLOCK_DAT_10_ONE + 3 + 2, ENV_FOUR_BLOCK_DISP_ONE_ROW, 3, 5, (uint8 *)&divoom_light_env_week_mode2[week][1][0], 
					pdivoom_light_env_handle->disp_buf, (Pdivoom_screen_color_info)&divoom_env_black_color);
            divoom_disp_font_fill_area(ENV_FOUR_BLOCK_DAT_10_ONE + 3 + 2 + 3 + 2, ENV_FOUR_BLOCK_DISP_ONE_ROW, 4, 5, (uint8 *)&divoom_light_env_week_mode2[week][2][0], 
					pdivoom_light_env_handle->disp_buf, (Pdivoom_screen_color_info)&divoom_env_black_color);
            break;
        case DIVOOM_LIGHT_ENV_FRI:
			//10 = 3 + 4 + 3
            divoom_disp_font_fill_area(ENV_FOUR_BLOCK_DAT_10_ONE, ENV_FOUR_BLOCK_DISP_ONE_ROW, 3, 5, (uint8 *)&divoom_light_env_week_mode2[week][0][0], 
					pdivoom_light_env_handle->disp_buf, (Pdivoom_screen_color_info)&divoom_env_black_color);
            divoom_disp_font_fill_area(ENV_FOUR_BLOCK_DAT_10_ONE + 3 + 2, ENV_FOUR_BLOCK_DISP_ONE_ROW, 4, 5, (uint8 *)&divoom_light_env_week_mode2[week][1][0], 
					pdivoom_light_env_handle->disp_buf, (Pdivoom_screen_color_info)&divoom_env_black_color);
            divoom_disp_font_fill_area(ENV_FOUR_BLOCK_DAT_10_ONE + 3 + 2 + 4 + 2, ENV_FOUR_BLOCK_DISP_ONE_ROW, 3, 5, (uint8 *)&divoom_light_env_week_mode2[week][2][0], 
					pdivoom_light_env_handle->disp_buf, (Pdivoom_screen_color_info)&divoom_env_black_color);
            break;
    }        
}

//这个是四个横条显示
//新加效果日期显示
void divoom_light_env_date_draw_week_mode4(uint8 week)
{
    switch(week)
    {
        case DIVOOM_LIGHT_ENV_MON:
			//13 = 5 + 4 + 4
            divoom_disp_font_fill_area(ENV_FOUR_LINE_WEEK_X_START, ENV_FOUR_LINE_WEEK_Y, 5, 5, (uint8 *)&divoom_light_env_week_mode2[week][0][0], 
					pdivoom_light_env_handle->disp_buf, (Pdivoom_screen_color_info)&divoom_env_black_color);
            divoom_disp_font_fill_area(ENV_FOUR_LINE_WEEK_X_START + ENV_FOUR_LINE_WEEK_GAP + 5, ENV_FOUR_LINE_WEEK_Y, 3, 5, (uint8 *)&divoom_light_env_week_mode2[week][1][0], 
					pdivoom_light_env_handle->disp_buf, (Pdivoom_screen_color_info)&divoom_env_black_color);
            divoom_disp_font_fill_area(ENV_FOUR_LINE_WEEK_X_START + ENV_FOUR_LINE_WEEK_GAP*2 + 8, ENV_FOUR_LINE_WEEK_Y, 4, 5, (uint8 *)&divoom_light_env_week_mode2[week][2][0], 
					pdivoom_light_env_handle->disp_buf, (Pdivoom_screen_color_info)&divoom_env_black_color);
            break;
        case DIVOOM_LIGHT_ENV_WED:
			//12 = 5 + 3 + 4
            divoom_disp_font_fill_area(ENV_FOUR_LINE_WEEK_X_START, ENV_FOUR_LINE_WEEK_Y, 5, 5, (uint8 *)&divoom_light_env_week_mode2[week][0][0], 
					pdivoom_light_env_handle->disp_buf, (Pdivoom_screen_color_info)&divoom_env_black_color);
            divoom_disp_font_fill_area(ENV_FOUR_LINE_WEEK_X_START + 5 + ENV_FOUR_LINE_WEEK_GAP, ENV_FOUR_LINE_WEEK_Y, 3, 5, (uint8 *)&divoom_light_env_week_mode2[week][1][0], 
					pdivoom_light_env_handle->disp_buf, (Pdivoom_screen_color_info)&divoom_env_black_color);
            divoom_disp_font_fill_area(ENV_FOUR_LINE_WEEK_GAP*2 + ENV_FOUR_LINE_WEEK_X_START + 8, ENV_FOUR_LINE_WEEK_Y, 4, 5, (uint8 *)&divoom_light_env_week_mode2[week][2][0], 
					pdivoom_light_env_handle->disp_buf, (Pdivoom_screen_color_info)&divoom_env_black_color);
            break;
        case DIVOOM_LIGHT_ENV_TUE:
        case DIVOOM_LIGHT_ENV_THU:
        case DIVOOM_LIGHT_ENV_SAT:
			//9
            divoom_disp_font_fill_area(ENV_FOUR_LINE_WEEK_X_START, ENV_FOUR_LINE_WEEK_Y, 3, 5, (uint8 *)&divoom_light_env_week_mode2[week][0][0], 
					pdivoom_light_env_handle->disp_buf, (Pdivoom_screen_color_info)&divoom_env_black_color);
            divoom_disp_font_fill_area(ENV_FOUR_LINE_WEEK_X_START + ENV_FOUR_LINE_WEEK_GAP + 3, ENV_FOUR_LINE_WEEK_Y, 3, 5, (uint8 *)&divoom_light_env_week_mode2[week][1][0], 
					pdivoom_light_env_handle->disp_buf, (Pdivoom_screen_color_info)&divoom_env_black_color);
            divoom_disp_font_fill_area(ENV_FOUR_LINE_WEEK_X_START + ENV_FOUR_LINE_WEEK_GAP*2 + 6, ENV_FOUR_LINE_WEEK_Y, 3, 5, (uint8 *)&divoom_light_env_week_mode2[week][2][0], 
					pdivoom_light_env_handle->disp_buf, (Pdivoom_screen_color_info)&divoom_env_black_color);
            break;
        case DIVOOM_LIGHT_ENV_SUN:
			//10 = 3 + 4 + 3
            divoom_disp_font_fill_area(ENV_FOUR_LINE_WEEK_X_START, ENV_FOUR_LINE_WEEK_Y, 3, 5, (uint8 *)&divoom_light_env_week_mode2[week][0][0], 
					pdivoom_light_env_handle->disp_buf, (Pdivoom_screen_color_info)&divoom_env_black_color);
            divoom_disp_font_fill_area(ENV_FOUR_LINE_WEEK_X_START + ENV_FOUR_LINE_WEEK_GAP + 3, ENV_FOUR_LINE_WEEK_Y, 3, 5, (uint8 *)&divoom_light_env_week_mode2[week][1][0], 
					pdivoom_light_env_handle->disp_buf, (Pdivoom_screen_color_info)&divoom_env_black_color);
            divoom_disp_font_fill_area(ENV_FOUR_LINE_WEEK_X_START + ENV_FOUR_LINE_WEEK_GAP*2 + 6, ENV_FOUR_LINE_WEEK_Y, 4, 5, (uint8 *)&divoom_light_env_week_mode2[week][2][0], 
					pdivoom_light_env_handle->disp_buf, (Pdivoom_screen_color_info)&divoom_env_black_color);
            break;
        case DIVOOM_LIGHT_ENV_FRI:
			//10 = 3 + 4 + 3
            divoom_disp_font_fill_area(ENV_FOUR_LINE_WEEK_X_START, ENV_FOUR_LINE_WEEK_Y, 3, 5, (uint8 *)&divoom_light_env_week_mode2[week][0][0], 
					pdivoom_light_env_handle->disp_buf, (Pdivoom_screen_color_info)&divoom_env_black_color);
            divoom_disp_font_fill_area(ENV_FOUR_LINE_WEEK_X_START + ENV_FOUR_LINE_WEEK_GAP + 3, ENV_FOUR_LINE_WEEK_Y, 4, 5, (uint8 *)&divoom_light_env_week_mode2[week][1][0], 
					pdivoom_light_env_handle->disp_buf, (Pdivoom_screen_color_info)&divoom_env_black_color);
            divoom_disp_font_fill_area(ENV_FOUR_LINE_WEEK_X_START + ENV_FOUR_LINE_WEEK_GAP*2 + 7, ENV_FOUR_LINE_WEEK_Y, 3, 5, (uint8 *)&divoom_light_env_week_mode2[week][2][0], 
					pdivoom_light_env_handle->disp_buf, (Pdivoom_screen_color_info)&divoom_env_black_color);
            break;
    }
}

//显示日期
void divoom_light_env_date_draw_date_set_mode(Pdivoom_light_env_date_info date_info, uint8 mode, uint8 *buf)
{
	//一直用两位显示,不够补0
	switch(mode)
	{
		case DIVOOM_DISP_ENV_TIME_CELS_DATA:
			{
				//set dot
				divoom_disp_ctrl_set_dot(12, 30, (Pdivoom_screen_color_info)&divoom_env_black_color, buf);
				//显示月
				divoom_disp_digit_fill_area(ENV_FOUR_LINE_DATA_ONE_X, ENV_FOUR_LINE_DATA_Y, date_info->month/10, 
						DIVOOM_3X5_DIGIT, buf, (Pdivoom_screen_color_info)&divoom_env_black_color);
				divoom_disp_digit_fill_area(ENV_FOUR_LINE_DATA_TWO_X, ENV_FOUR_LINE_DATA_Y, date_info->month%10, 
						DIVOOM_3X5_DIGIT, buf, (Pdivoom_screen_color_info)&divoom_env_black_color);
				//显示天
				divoom_disp_digit_fill_area(ENV_FOUR_LINE_DATA_THREE_X, ENV_FOUR_LINE_DATA_Y, date_info->day/10, 
						DIVOOM_3X5_DIGIT, buf, (Pdivoom_screen_color_info)&divoom_env_black_color);
				divoom_disp_digit_fill_area(ENV_FOUR_LINE_DATA_FOUR_X, ENV_FOUR_LINE_DATA_Y, date_info->day%10, 
						DIVOOM_3X5_DIGIT, buf, (Pdivoom_screen_color_info)&divoom_env_black_color);
			}
			break;
			//大于10的要特殊处理吧
		case DIVOOM_DISP_ENV_TIME_CELS_DATA_WEATHER:
			{
				if(date_info->month > 9)
				{
					//显示月
					divoom_disp_ctrl_set_dot(ENV_FOUR_BLOCK_DAT_M_ONE - 1, ENV_FOUR_BLOCK_DISP_TWO_ROW, (Pdivoom_screen_color_info)&divoom_env_black_color, buf);
					divoom_disp_ctrl_set_dot(ENV_FOUR_BLOCK_DAT_M_ONE - 1, ENV_FOUR_BLOCK_DISP_TWO_ROW + 1, (Pdivoom_screen_color_info)&divoom_env_black_color, buf);
					divoom_disp_ctrl_set_dot(ENV_FOUR_BLOCK_DAT_M_ONE - 1, ENV_FOUR_BLOCK_DISP_TWO_ROW + 2, (Pdivoom_screen_color_info)&divoom_env_black_color, buf);
					divoom_disp_ctrl_set_dot(ENV_FOUR_BLOCK_DAT_M_ONE - 1, ENV_FOUR_BLOCK_DISP_TWO_ROW + 3, (Pdivoom_screen_color_info)&divoom_env_black_color, buf);
					divoom_disp_ctrl_set_dot(ENV_FOUR_BLOCK_DAT_M_ONE - 1, ENV_FOUR_BLOCK_DISP_TWO_ROW + 4, (Pdivoom_screen_color_info)&divoom_env_black_color, buf);

					divoom_disp_ctrl_set_dot(ENV_FOUR_BLOCK_DISP_DATA_DOT_BIG_X, ENV_FOUR_BLOCK_DISP_DATA_DOT_BIG_Y, (Pdivoom_screen_color_info)&divoom_env_black_color, buf);

					divoom_disp_digit_fill_area(ENV_FOUR_BLOCK_DAT_M_TWO, ENV_FOUR_BLOCK_DISP_TWO_ROW, date_info->month%10, 
							DIVOOM_3X5_DIGIT, buf, (Pdivoom_screen_color_info)&divoom_env_black_color);
					//显示天
					divoom_disp_digit_fill_area(ENV_FOUR_BLOCK_DAT_D_ONE, ENV_FOUR_BLOCK_DISP_TWO_ROW, date_info->day/10, 
							DIVOOM_3X5_DIGIT, buf, (Pdivoom_screen_color_info)&divoom_env_black_color);
					divoom_disp_digit_fill_area(ENV_FOUR_BLOCK_DAT_D_TWO, ENV_FOUR_BLOCK_DISP_TWO_ROW, date_info->day%10, 
							DIVOOM_3X5_DIGIT, buf, (Pdivoom_screen_color_info)&divoom_env_black_color);
				}
				else
				{
					//显示月
					divoom_disp_digit_fill_area(ENV_FOUR_BLOCK_DAT_M_ONE - 1, ENV_FOUR_BLOCK_DISP_TWO_ROW, date_info->month%10, 
							DIVOOM_3X5_DIGIT, buf, (Pdivoom_screen_color_info)&divoom_env_black_color);

					divoom_disp_ctrl_set_dot(ENV_FOUR_BLOCK_DISP_DATA_DOT_LITTLE_X, ENV_FOUR_BLOCK_DISP_DATA_DOT_LITTLE_Y, 
							(Pdivoom_screen_color_info)&divoom_env_black_color, buf);

					//显示天
					divoom_disp_digit_fill_area(ENV_FOUR_BLOCK_DAT_D_ONE - 1, ENV_FOUR_BLOCK_DISP_TWO_ROW, date_info->day/10, 
							DIVOOM_3X5_DIGIT, buf, (Pdivoom_screen_color_info)&divoom_env_black_color);
					divoom_disp_digit_fill_area(ENV_FOUR_BLOCK_DAT_D_TWO - 1, ENV_FOUR_BLOCK_DISP_TWO_ROW, date_info->day%10, 
							DIVOOM_3X5_DIGIT, buf, (Pdivoom_screen_color_info)&divoom_env_black_color);
				}
			}
			break;
	}
}

void divoom_light_env_weather_set_mode(uint8 mode, uint8 *buf, uint8 refresh_flag)
{
	uint8 cur_weather_type = 0;
	uint8 i = 0;
	uint32 offset = 0;
    pdivoom_light_env_handle->penv_read(DIVOOM_DISP_ENV_WEATHER, (void *)&cur_weather_type);
	divoom_print("cur_weather_type = %d, pdivoom_light_env_handle->weather_type = %d\n", cur_weather_type, pdivoom_light_env_handle->weather_type);
	if(cur_weather_type == 0 || cur_weather_type == 0xFF)
	{
		divoom_print("%s--%d: Wrong Get DIVOOM_DISP_ENV_WEATHER value:%x!\n", __func__, __LINE__, cur_weather_type);
	}
	else
	{
		if(cur_weather_type != pdivoom_light_env_handle->weather_type || refresh_flag)
		{
			//更新新的天气type和decode_handle
			pdivoom_light_env_handle->weather_type = cur_weather_type;
			divoom_light_env_update_weather_gif_just_small_gif();
		}
	}
    if(pdivoom_light_env_handle->pic_id >= pdivoom_light_env_handle->pic_num)
    {
        pdivoom_light_env_handle->pic_id = 0;
        divoom_image_decode_reset(pdivoom_light_env_handle->decode_handle);
    }
    if(pdivoom_light_env_handle->define_pic_info)
    {
        uint8  *pic_data = NULL;
        uint16 pic_len;
		//拿到了编码数据,解码成RGB,然后拷贝到buf的第16行开始,第16列开始
		switch(mode)
		{
			case DIVOOM_DISP_ENV_TIME_CELS_DATA_WEATHER:
				{
					pic_data = divoom_image_decode_get_pic_data(pdivoom_light_env_handle->decode_handle, &pic_len, NULL);
					if(pic_data == NULL)
					{
						divoom_print("It's Err In %s--%d\n", __func__, __LINE__);
						break;
					}
					divoom_decode_pic_get_rgb(pic_data, pdivoom_light_env_handle->bak_buf, NULL, DIVOOM_IMAGE_SMALL_HEIGHT);
					offset = 0;
					//开始按行拷贝
					for(i=0;i<DIVOOM_IMAGE_SMALL_HEIGHT;i++)
					{
						memcpy(buf+(ENV_FOUR_BLOCK_WEATHER_START_X+(ENV_FOUR_BLOCK_WEATHER_START_Y+i)*DIVOOM_IMAGE_HEIGHT)*DIVOOM_IMAGE_COLOR_SIZE, 
								pdivoom_light_env_handle->bak_buf+offset, DIVOOM_IMAGE_SMALL_WIDTH*DIVOOM_IMAGE_COLOR_SIZE);
						offset += DIVOOM_IMAGE_SMALL_WIDTH*DIVOOM_IMAGE_COLOR_SIZE;
					}
				}
				break;
			default:
				divoom_print("No This Command In %s--%d\n", __func__, __LINE__);
				break;
		}
    }
    pdivoom_light_env_handle->pic_id ++;
}

//要保证两个数据的长度都是一样的
uint8 divoom_light_env_judge_same(char *lhs, char *rhs, uint32 size)
{
	return (strncmp(lhs, rhs, size) == 0)?1:0;
}

//显示四横杠时间,温度,日期
void divoom_light_env_time_cels_data(uint8 refresh_flag)
{
    divoom_light_env_date_info date_info = {0};
    divoom_light_env_time_info time_info = {0};
	int celsius_info = 0;
	//四横杠比较简单,先获取时间,写到前8横,在获取日期,写week到8-16,温度到16-24,日期到24-32
	//判断当前的时间温度日期是否改变,任何一个改变都要从新发送
    pdivoom_light_env_handle->penv_read(DIVOOM_DISP_ENV_TIME, (void *)&time_info);
    pdivoom_light_env_handle->penv_read(DIVOOM_DISP_ENV_DATE, (void *)&date_info);
    pdivoom_light_env_handle->penv_read(DIVOOM_DISP_ENV_CELS, (void *)&celsius_info);
	if(divoom_light_env_judge_same((char *)&time_info, (char *)&pdivoom_light_env_handle->time_info, sizeof(divoom_light_env_time_info)) && 
	  divoom_light_env_judge_same((char *)&date_info, (char *)&pdivoom_light_env_handle->date_info, sizeof(divoom_light_env_date_info)) &&
	  divoom_light_env_judge_same((char *)&celsius_info, (char *)&pdivoom_light_env_handle->celsius_value, sizeof(int)) && !refresh_flag)
	{
		//都相同,不需要更新
		divoom_print("%s--%d\n", __func__, __LINE__);
		return;
	}
	memcpy(&pdivoom_light_env_handle->time_info, &time_info, sizeof(divoom_light_env_time_info));
	memcpy(&pdivoom_light_env_handle->date_info, &date_info, sizeof(divoom_light_env_date_info));
	pdivoom_light_env_handle->celsius_value = celsius_info;
	memset(pdivoom_light_env_handle->disp_buf, 0, sizeof(pdivoom_light_env_handle->disp_buf));
    //divoom_disp_picture_get_rgb_data(DIVOOM_DISP_DIVOOM_TIME_TYPE_NEW_ONE, pdivoom_light_env_handle->disp_buf);
    divoom_disp_picture_get_rgb_data(DIVOOM_DISP_FOUR_LINE_BACKGROUND, pdivoom_light_env_handle->disp_buf);
	divoom_light_env_time_set_mode(&time_info, DIVOOM_DISP_ENV_TIME_CELS_DATA, pdivoom_light_env_handle->disp_buf);
	divoom_light_env_date_draw_week_mode4(date_info.week);
	divoom_light_env_celsius_set_mode(celsius_info, DIVOOM_DISP_ENV_TIME_CELS_DATA, pdivoom_light_env_handle->disp_buf);
	divoom_light_env_date_draw_date_set_mode(&date_info, DIVOOM_DISP_ENV_TIME_CELS_DATA, pdivoom_light_env_handle->disp_buf);
	divoom_screen_disp_rgb32_data(pdivoom_light_env_handle->disp_buf);
}

//显示四方块时间,温度,日期,天气
//因为有天气,所以不需要每次都清空,但是需要
void divoom_light_env_time_cels_data_weather(uint8 refresh_flag)
{
    divoom_light_env_date_info date_info = {0};
    divoom_light_env_time_info time_info = {0};
	int celsius_info = 0;

    pdivoom_light_env_handle->penv_read(DIVOOM_DISP_ENV_TIME, (void *)&time_info);
    pdivoom_light_env_handle->penv_read(DIVOOM_DISP_ENV_DATE, (void *)&date_info);
    pdivoom_light_env_handle->penv_read(DIVOOM_DISP_ENV_CELS, (void *)&celsius_info);

	//任何一个变化了,都要清理一下,重新填入数据,如果是天气的不需要
	//获取时间,日期,温度,天气,前三个按照xy轴写,温度从x16,y16开始拷贝
	if(!divoom_light_env_judge_same((char *)&time_info, (char *)&pdivoom_light_env_handle->time_info, sizeof(divoom_light_env_time_info)) || 
			(!divoom_light_env_judge_same((char *)&date_info, (char *)&pdivoom_light_env_handle->date_info, sizeof(divoom_light_env_date_info))) || 
			(!divoom_light_env_judge_same((char *)&celsius_info, (char *)&pdivoom_light_env_handle->celsius_value, sizeof(int))) || refresh_flag)
	{
		memset(pdivoom_light_env_handle->disp_buf, 0, sizeof(pdivoom_light_env_handle->disp_buf));

		memcpy(&pdivoom_light_env_handle->time_info, &time_info, sizeof(divoom_light_env_time_info));
		memcpy(&pdivoom_light_env_handle->date_info, &date_info, sizeof(divoom_light_env_date_info));
		pdivoom_light_env_handle->celsius_value = celsius_info;

		divoom_disp_picture_get_rgb_data(DIVOOM_DISP_DIVOOM_TIME_TYPE_NEW_TWO, pdivoom_light_env_handle->disp_buf);
		divoom_light_env_time_set_mode(&time_info, DIVOOM_DISP_ENV_TIME_CELS_DATA_WEATHER, pdivoom_light_env_handle->disp_buf);
		divoom_light_env_date_draw_week_mode3(date_info.week);
		divoom_light_env_celsius_set_mode(celsius_info, DIVOOM_DISP_ENV_TIME_CELS_DATA_WEATHER, pdivoom_light_env_handle->disp_buf);
		divoom_light_env_date_draw_date_set_mode(&date_info, DIVOOM_DISP_ENV_TIME_CELS_DATA_WEATHER, pdivoom_light_env_handle->disp_buf);
	}
	divoom_light_env_weather_set_mode(DIVOOM_DISP_ENV_TIME_CELS_DATA_WEATHER, pdivoom_light_env_handle->disp_buf, refresh_flag);
	divoom_screen_disp_rgb32_data(pdivoom_light_env_handle->disp_buf);
}
#endif

//-----------------------------------------------------------------------------------------------------


/*********************************************************************
  Function:         divoom_light_env_main
  Description:     显示的主控制函数，DIVOOM_DISP_ENV_MODE_TIME时间切换一个显示模式
  Input:             void
  Return:           NONE
  Author:           divoom
**********************************************************************/

void divoom_light_env_main(void* data)
{
    bool refresh_flag = false;
	uint8 weather_refresh_flag = 0;
    pdivoom_light_env_handle->sys_ms ++;

    // 1秒执行一次
    if(pdivoom_light_env_handle->sys_ms >= 10 || pdivoom_light_env_handle->refresh_flag || (pdivoom_light_env_handle->refresh_time_flag != 0xFF) || (pdivoom_light_env_handle->disp_type == DIVOOM_DISP_ENV_NEW_PIC))
    {
        pdivoom_light_env_handle->sys_ms  = 0;
        pdivoom_light_env_handle->tick ++;        

        if(((pdivoom_light_env_handle->disp_type == DIVOOM_DISP_ENV_TIME) &&(pdivoom_light_env_handle->tick >= DIVOOM_DISP_ENV_MODE_TIME)) ||
            ((pdivoom_light_env_handle->disp_type == DIVOOM_DISP_ENV_NEW_PIC) &&(pdivoom_light_env_handle->gif_id > DIVOOM_LIGHT_ENV_GIF_DIS_NUM)) ||
            ((pdivoom_light_env_handle->disp_type != DIVOOM_DISP_ENV_TIME && pdivoom_light_env_handle->disp_type != DIVOOM_DISP_ENV_NEW_PIC) 
			 && (pdivoom_light_env_handle->tick >= DIVOOM_DISP_ENV_WEATHER_TIME)) || 
			(pdivoom_light_env_handle->refresh_time_flag != 0xFF) || 
			((pdivoom_light_env_handle->disp_type == DIVOOM_DISP_ENV_TIME_CELS_DATA) &&(pdivoom_light_env_handle->tick >= DIVOOM_DISP_ENV_MODE_TIME_NEW_MODE_1)) ||
			((pdivoom_light_env_handle->disp_type == DIVOOM_DISP_ENV_TIME_CELS_DATA_WEATHER) &&(pdivoom_light_env_handle->tick >= DIVOOM_DISP_ENV_MODE_TIME_NEW_MODE_2)))
        {
            if(pdivoom_light_env_handle->disp_type == DIVOOM_DISP_ENV_NEW_PIC)
            {
                divoom_light_env_gif_exit();
                divoom_disp_time_common_time_init();
            }
            refresh_flag = divoom_light_env_get_next_mode();
            pdivoom_light_env_handle->tick = 0;
            if(pdivoom_light_env_handle->disp_type == DIVOOM_DISP_ENV_NEW_PIC)
            {
                divoom_disp_time_common_time_colse();
                if(divoom_light_env_gif_init()==0)
                {
                    divoom_disp_time_common_time_init();                    
					if(pdivoom_light_env_handle->mode & (1<<DIVOOM_DISP_ENV_TIME_CELS_DATA))
					{
						pdivoom_light_env_handle->disp_type = DIVOOM_DISP_ENV_TIME_CELS_DATA;
					}
					else if(pdivoom_light_env_handle->mode & (1<<DIVOOM_DISP_ENV_TIME_CELS_DATA_WEATHER))
					{
						pdivoom_light_env_handle->disp_type = DIVOOM_DISP_ENV_TIME_CELS_DATA_WEATHER;
					}
					else
					{
						pdivoom_light_env_handle->disp_type = DIVOOM_DISP_ENV_TIME;
					}
                }
            }
            pdivoom_light_env_handle->pic_id  = 0;  
            divoom_image_decode_reset(pdivoom_light_env_handle->decode_handle);
        }
        if(pdivoom_light_env_handle->refresh_flag == true)
        {
            pdivoom_light_env_handle->refresh_flag = false;
            refresh_flag = true;
        }
		if(pdivoom_light_env_handle->refresh_time_flag != 0xFF)
		{
			pdivoom_light_env_handle->refresh_time_flag = 0xFF;
            refresh_flag = true;
		}
		//divoom_print("\n\n[%s:%d] pdivoom_light_env_handle->disp_type = %d\n", __func__, __LINE__, pdivoom_light_env_handle->disp_type);
        switch(pdivoom_light_env_handle->disp_type)
        {
			case DIVOOM_DISP_ENV_TIME_CELS_DATA:
				divoom_light_env_time_cels_data(refresh_flag);
				break;

			case DIVOOM_DISP_ENV_TIME_CELS_DATA_WEATHER:
				divoom_light_env_time_cels_data_weather(refresh_flag);
				break;

            case DIVOOM_DISP_ENV_NEW_PIC:
                break;
                
            case DIVOOM_DISP_ENV_TIME:
                divoom_light_env_time(refresh_flag);
                break;                
                
            case DIVOOM_DISP_ENV_CELS:
                if(refresh_flag)
                {
                    if(pdivoom_light_env_handle->temp_flag == 1)
                    {
                        divoom_light_env_fahrenheit(refresh_flag);
                    }
                    else
                    {
                        divoom_light_env_celsius(refresh_flag);
                    }
                    //避免一直更新,屏幕闪烁的现象
                    divoom_screen_disp_rgb32_data(pdivoom_light_env_handle->disp_buf);                      
                }
                break;
            case DIVOOM_DISP_ENV_DATE:
                if(refresh_flag)
                {
                    divoom_light_env_date(refresh_flag);
                    //避免一直更新,屏幕闪烁的现象                    
                    divoom_screen_disp_rgb32_data(pdivoom_light_env_handle->disp_buf);                      
                }
                break;
                
                /*
            case DIVOOM_DISP_ENV_HUMI:
                divoom_light_env_humidity(refresh_flag);
                break;
                
            case DIVOOM_DISP_ENV_NOISE:
                divoom_light_env_noise(refresh_flag);
                break;
                */
                
        }
    }

    switch(pdivoom_light_env_handle->disp_type)
    {
        case DIVOOM_DISP_ENV_NEW_PIC:
            if(0 == divoom_light_env_gif_main())
            {
                pdivoom_light_env_handle->gif_id = DIVOOM_LIGHT_ENV_GIF_DIS_NUM + 1;
                if(pdivoom_light_env_handle->gif_over)
                {
                    //当前文件播放完了    
                    gdivoom_light_env_gif_ctrl.hot_off = DIVOOM_PAGE_SIZE;   
                }
            }
            break;
            
        case DIVOOM_DISP_ENV_TIME:
            divoom_light_env_disp_effect();
            divoom_screen_disp_rgb32_data(pdivoom_light_env_handle->disp_buf);       
            break;      
        case DIVOOM_DISP_ENV_WEATHER:
            //if((pdivoom_light_env_handle->sys_ms % 5) == 0 || refresh_flag)
			//之前的效果变化了,并且必须其中一个是 DIVOOM_DISP_ENV_TIME_CELS_DATA_WEATHER , 因为这个效果的天气和其他的不一样,这个是32的天气,其他的是16的
			if((pdivoom_light_env_handle->old_effect_type != pdivoom_light_env_handle->effect_type) && 
					((pdivoom_light_env_handle->old_effect_type == DIVOOM_DISP_ENV_TIME_CELS_DATA_WEATHER) || 
					 pdivoom_light_env_handle->effect_type == DIVOOM_DISP_ENV_TIME_CELS_DATA_WEATHER))
			{
				weather_refresh_flag = 1;
			}
			if((pdivoom_light_env_handle->sys_ms % 5) == 0 || weather_refresh_flag)
			{
                divoom_light_env_weather(weather_refresh_flag);
            }
            break;
        case DIVOOM_DISP_ENV_CELS:                 
        case DIVOOM_DISP_ENV_DATE:
//            if(pdivoom_light_env_handle->effect_type == DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_COLOURFUL)
//                divoom_light_env_disp_effect();
//            divoom_screen_disp_rgb_data(pdivoom_light_env_handle->disp_buf);  
            break;       
        default:
            break;    
    }       
/*    
    if((pdivoom_light_env_handle->disp_type == DIVOOM_DISP_ENV_WEATHER))
    {
        if((pdivoom_light_env_handle->sys_ms % 5) ==0)
        {
            divoom_light_env_weather();
        }
    }
    else if((pdivoom_light_env_handle->disp_type == DIVOOM_DISP_ENV_DATE))    
    {
        if(pdivoom_light_env_handle->effect_type == DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_COLOURFUL)
            divoom_light_env_disp_effect();
        divoom_screen_disp_rgb_data(pdivoom_light_env_handle->disp_buf);  
    }
    else
    {
        divoom_light_env_disp_effect();
        divoom_screen_disp_rgb_data(pdivoom_light_env_handle->disp_buf);        
    }
*/    
}


/*********************************************************************
  Function:         divoom_light_env_update_hour_flag
  Description:     更新时间显示模式
  Input:             hour_flag  
  Return:           NONE
  Author:           divoom
**********************************************************************/

void divoom_light_env_update_hour_flag(uint8 hour_flag)
{
    if(pdivoom_light_env_handle)
    {
        divoom_com_pause_task(pdivoom_light_env_handle->task_id);
        pdivoom_light_env_handle->hour_flag = hour_flag;
        pdivoom_light_env_handle->sys_ms = 10;
        divoom_com_resume_task(pdivoom_light_env_handle->task_id);
    }
}
/*********************************************************************
  Function:         divoom_light_env_update_hour_flag
  Description:     更新时间温度制式
  Input:             hour_flag  
  Return:           NONE
  Author:           divoom
**********************************************************************/

void divoom_light_env_update_temp_flag(uint8 temp_flag)
{
    if(pdivoom_light_env_handle)
    {
        divoom_com_pause_task(pdivoom_light_env_handle->task_id);
        pdivoom_light_env_handle->temp_flag = temp_flag;
        pdivoom_light_env_handle->sys_ms = 10;
        pdivoom_light_env_handle->refresh_flag = true;
        divoom_com_resume_task(pdivoom_light_env_handle->task_id);
    }
}
void divoom_light_env_update_env_info(uint8 weather_type, int8 temp_value, uint32 humi_value)
{
    if(pdivoom_light_env_handle)
    {
        divoom_com_pause_task(pdivoom_light_env_handle->task_id);
        pdivoom_light_env_handle->sys_ms = 10;
        pdivoom_light_env_handle->weather_type = weather_type;
        pdivoom_light_env_handle->celsius_value = temp_value;
        pdivoom_light_env_handle->humidity_value = humi_value;
        pdivoom_light_env_handle->refresh_flag = true;

        divoom_light_env_update_weather_gif();        
        divoom_light_env_main(NULL);
        divoom_com_resume_task(pdivoom_light_env_handle->task_id);
    }
}
void divoom_light_env_update_gallery(uint8 flag)
{
    if(pdivoom_light_env_handle)
    {
        if(flag)
        {
            pdivoom_light_env_handle->sys_ms = 10;
            pdivoom_light_env_handle->refresh_flag = true;
            pdivoom_light_env_handle->refresh_time_flag = DIVOOM_DISP_ENV_NEW_PIC;
            pdivoom_light_env_handle->mode |= (1 << DIVOOM_DISP_ENV_NEW_PIC);
        }
        else
        {
            pdivoom_light_env_handle->sys_ms = 10;
            pdivoom_light_env_handle->refresh_flag = true;
            pdivoom_light_env_handle->mode &= ~(1 << DIVOOM_DISP_ENV_NEW_PIC);
            pdivoom_light_env_handle->gif_id = DIVOOM_LIGHT_ENV_GIF_DIS_NUM + 1;
        }
    }
}
/*********************************************************************
  Function:         divoom_light_env_update_mode
  Description:     更新环境显示模式
  Input:             mode   显示模式
                        effect_type 显示效果
                        color    时间显示颜色
  Return:           NONE
  Author:           divoom
**********************************************************************/

void divoom_light_env_update_mode(Pdivoom_disp_ctrl_env_info pevn_info)
{
    uint8 i;
    if(pdivoom_light_env_handle)
    {
        divoom_com_pause_task(pdivoom_light_env_handle->task_id);
        pdivoom_light_env_handle->mode = pevn_info->mode;
        if(pevn_info->mode & ((1 << DIVOOM_DISP_ENV_TIME_CELS_DATA) | (1 << DIVOOM_DISP_ENV_TIME_CELS_DATA_WEATHER)))
        {
            pdivoom_light_env_handle->mode &= 0xf0;
        }
		if(pdivoom_light_env_handle->effect_type != pevn_info->effect_type)
		{
			//这里要对新加的效果进行判断
			switch(pevn_info->effect_type)
			{
				//case DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_JUST_NUM:
				case DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_LED_DISP:
				case DIVOOM_LIGHT_ENV_TIME_DUSP_EFFECT_ITALIC:
				case DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_GOLD_BACKGROUND:
				case DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_BLUE_COLOURFUL:
				case DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_COLOURFUL:
				case DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_GRAPH:
				case DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_CIRCULAR:
				case DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_COLOURFUL_NUM:
					//case DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_COLOR_FLOOR:
					//case DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_GRAPH:
					pdivoom_light_env_handle->refresh_time_flag = DIVOOM_DISP_ENV_TIME;
					break;
				case DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_BLOCK:
					pdivoom_light_env_handle->refresh_time_flag = DIVOOM_DISP_ENV_TIME_CELS_DATA_WEATHER;
					break;
				case DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_LINE:
					pdivoom_light_env_handle->refresh_time_flag = DIVOOM_DISP_ENV_TIME_CELS_DATA;
					break;
					break;
			}
		}
		else if(pdivoom_light_env_handle->effect_type == pevn_info->effect_type && pevn_info->refresh_env_type != 0xFF)
		{
			pdivoom_light_env_handle->refresh_time_flag = pevn_info->refresh_env_type;
		}
		pdivoom_light_env_handle->old_effect_type = pdivoom_light_env_handle->effect_type;
        pdivoom_light_env_handle->effect_type = pevn_info->effect_type;
        pdivoom_light_env_handle->hour_flag = pevn_info->time_flag;
        pdivoom_light_env_handle->temp_flag = pevn_info->temp_flag;
        memcpy(&pdivoom_light_env_handle->color, &pevn_info->color, sizeof(divoom_light_color_info));
        pdivoom_light_env_handle->sys_ms = 10;
        pdivoom_light_env_handle->refresh_flag = true;
		//在这两个效果里面,不要去找第一个显示的,因为这边传过来的的 mode 是33和64,都会默认选上时间的
		//if(pevn_info->effect_type != DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_BLOCK && pevn_info->effect_type != DIVOOM_LIGHT_ENV_TIME_DISP_EFFECT_LINE)
		{
			for(i = 0; i < 8; i++)
			{
				if(pdivoom_light_env_handle->mode & (1<<i))
				{
					pdivoom_light_env_handle->disp_type = i;
					break;
				}
			}
		}
		divoom_print("divoom_light_env_update_mode: mode : %02x, %d, %d, %d, %d, disp_mode:%d, %d\n", 
				(int)pdivoom_light_env_handle->mode, pevn_info->effect_type, pevn_info->color.r_c, pevn_info->color.g_c, pevn_info->color.b_c, pdivoom_light_env_handle->disp_type,pdivoom_light_env_handle->refresh_time_flag);
		//divoom_print("pdivoom_light_env_handle->disp_type = %d\n", pdivoom_light_env_handle->disp_type);
		//divoom_print("pdivoom_light_env_handle->refresh_time_flag = %d\n", pdivoom_light_env_handle->refresh_time_flag);
        divoom_com_resume_task(pdivoom_light_env_handle->task_id);
    }
}

/*********************************************************************
  Function:         divoom_light_env_init
  Description:     启动环境显示模式
  Input:             mode   显示模式
                       pcallback??读环境数据的回调
  Return:           NONE
  Author:           divoom
**********************************************************************/

void divoom_light_env_init(Pdivoom_disp_ctrl_env_info penv_info, 
                                  DIVOOM_LIGHT_GET_ENV_INFO pcallback, uint8 flag)
{
    uint8 i;
	divoom_malloc_check_all_ram_in();
	divoom_disp_time_common_time_init();
    if(pdivoom_light_env_handle)
    {
        return;
    }
    if(penv_info->effect_type> DIVOOM_LIGHT_ENV_DISP_EFFECT_MAX)
    {
        penv_info->effect_type = DIVOOM_LIGHT_ENV_DISP_EFFECT_MAX;
    }
    if(penv_info->mode == 0)
    {
        penv_info->mode = 1;
    }
    pdivoom_light_env_handle = DIVOOM_DISP_ENV_MALLOC(sizeof(divoom_light_env_ctrl));
    memset(pdivoom_light_env_handle, 0, sizeof(divoom_light_env_ctrl));
	pdivoom_light_env_handle->mode = penv_info->mode;
    if(penv_info->mode & ((1 << DIVOOM_DISP_ENV_TIME_CELS_DATA) | (1 << DIVOOM_DISP_ENV_TIME_CELS_DATA_WEATHER)))
    {
        pdivoom_light_env_handle->mode &= 0xf0;
    }
	pdivoom_light_env_handle->old_effect_type = 0xFF;
    pdivoom_light_env_handle->effect_type = penv_info->effect_type;
	//pdivoom_light_env_handle->cur_array = divoom_com_malloc(sizeof(divoom_coulourful_num_xy)*1024, __func__, __LINE__);
	//pdivoom_light_env_handle->next_array = divoom_com_malloc(sizeof(divoom_coulourful_num_xy)*1024, __func__, __LINE__);
    for(i = 0; i < 8; i++)
    {
        if(pdivoom_light_env_handle->mode & (1<<i))
        {
            pdivoom_light_env_handle->disp_type = i;
            break;
        }
    }
    pdivoom_light_env_handle->penv_read = pcallback;
    pdivoom_light_env_handle->hour_flag = penv_info->time_flag;
    pdivoom_light_env_handle->temp_flag = penv_info->temp_flag;
    pdivoom_light_env_handle->flag = 1;
    pdivoom_light_env_handle->refresh_time_flag = 0xFF;
    pdivoom_light_env_handle->sys_ms = 10;
	pdivoom_light_env_handle->tick = 0;
    pdivoom_light_env_handle->task_flag = flag;

    pdivoom_light_env_handle->decode_handle = divoom_image_decode_creat_handle();

	//在task运行之前,先获取一次天气和温度,因为在 divoom_light_env_get_next_mode 里面就对这俩进行了限制
	pdivoom_light_env_handle->penv_read(DIVOOM_DISP_ENV_CELS, (void *)&pdivoom_light_env_handle->celsius_value);
	//if(pdivoom_light_env_handle->mode & (1<<DIVOOM_DISP_ENV_WEATHER))
	{
		//这个比较特殊,如果有需要,先获取一次,并更新需要的信息
		//无论如何都更新一下,避免在检测下一个效果的时候,这个 weather_type 是0xFF无法更新
		pdivoom_light_env_handle->penv_read(DIVOOM_DISP_ENV_WEATHER, (void *)&pdivoom_light_env_handle->weather_type);
		//divoom_print("divoom_light_env_init:pdivoom_light_env_handle->weather_type = %d\n", pdivoom_light_env_handle->weather_type);
		divoom_light_env_update_weather_gif();
	}
    if(flag)
    {
        pdivoom_light_env_handle->task_id = divoom_com_create_task(divoom_light_env_main,DIVOOM_DISP_ENV_REFRESH_TIME/2);
    }
    else
    {
        pdivoom_light_env_handle->task_id = divoom_com_create_task(divoom_light_env_main,DIVOOM_DISP_ENV_REFRESH_TIME);
    }
    pdivoom_light_env_handle->time_info.hour = 30;

    //初始化方框效果2的颜色初始化   
    
    memcpy(&pdivoom_light_env_handle->color, &penv_info->color, sizeof(divoom_light_color_info));
    divoom_com_resume_task(pdivoom_light_env_handle->task_id);
    divoom_print("divoom_light_env_init: mode : %d, %d, %d, %d, %d, disp_mode:%d\n", 
			(int)pdivoom_light_env_handle->mode, penv_info->effect_type, penv_info->color.r_c, penv_info->color.g_c, penv_info->color.b_c, pdivoom_light_env_handle->disp_type);
}

/*********************************************************************
  Function:         divoom_light_env_reset_info
  Description:     将环境数据全部清除，以便显示实时数据，这个用于显示复位
  Input:             none
  Return:           NONE
  Author:           divoom
**********************************************************************/

void divoom_light_env_reset_info(void)
{
    pdivoom_light_env_handle->celsius_value = 26;
    pdivoom_light_env_handle->humidity_value = 0;
    pdivoom_light_env_handle->bright_value = 0;
    pdivoom_light_env_handle->fahrenheit_value = 0;
    pdivoom_light_env_handle->weather_type = 0;
//    pdivoom_light_env_handle->noise_value = 0;
    pdivoom_light_env_handle->time_info.hour = 30;
    pdivoom_light_env_handle->tick = DIVOOM_DISP_ENV_MODE_TIME - 1;
}


/*********************************************************************
  Function:         divoom_light_env_play
  Description:     模块运行控制
  Input:             run_flag    1-->play; 0-->pause
  Return:           NONE
  Author:           divoom
**********************************************************************/

void divoom_light_env_play(uint8 run_flag)
{
    if(pdivoom_light_env_handle)
    {
        pdivoom_light_env_handle->flag = run_flag;
        if(run_flag)
        {
            divoom_light_env_reset_info();
            divoom_com_resume_task(pdivoom_light_env_handle->task_id);
        }
        else
        {
            divoom_com_pause_task(pdivoom_light_env_handle->task_id);
        }
    }
}


/*********************************************************************
  Function:         divoom_light_env_close
  Description:     模块停止，释放资源
  Input:             void
  Return:           NONE
  Author:           divoom
**********************************************************************/

void divoom_light_env_close(void)
{
	divoom_disp_time_common_time_colse();
    if(pdivoom_light_env_handle)
    {
        divoom_com_pause_task(pdivoom_light_env_handle->task_id);
        divoom_light_env_gif_exit();
        divoom_com_destroy_task(pdivoom_light_env_handle->task_id);

        if(pdivoom_light_env_handle->define_pic_info)
        {
            DIVOOM_DISP_ENV_FREE(pdivoom_light_env_handle->define_pic_info);
        } 
        divoom_image_decode_destoy(pdivoom_light_env_handle->decode_handle);

		//divoom_com_free(pdivoom_light_env_handle->cur_array, __func__, __LINE__);
		//divoom_com_free(pdivoom_light_env_handle->next_array, __func__, __LINE__);
        DIVOOM_DISP_ENV_FREE(pdivoom_light_env_handle);
        pdivoom_light_env_handle = NULL;
    }
	divoom_malloc_check_all_ram_out(__func__);
}
#endif

