#ifndef __DIVOOM_USER_KEY_H
#define __DIVOOM_USER_KEY_H


/*****************************************************************************
 * Module    : 按键定义配置
 *****************************************************************************/


#define T_KEY_SHORT             0x00
#define T_KEY_SHORT_UP          0x80        // 1000 0000
#define T_KEY_LONG              0xA0        // 1010 0000
#define T_KEY_LONG_UP           0xC0        // 1100 0000
#define T_KEY_HOLD              0xE0        // 1110 0000


//定义为32以上(含32)的按键只支持单按  0x20-7f
//定义为0x1-0x1f(1-31)的支持多种按键方式
#define NO_KEY                  0
#define T_KEY_1                 0x1
#define T_KEY_2                 0x2
#define T_KEY_3                 0x3
#define T_KEY_4                 0x4
#define T_KEY_5                 0x5
#define T_KEY_6                 0x6
#define T_KEY_7                 0x7
#define T_KEY_8                 0x8
#define T_KEY_9                 0x9
#define T_KEY_10                0xa
#define T_KEY_11                0xb
#define T_KEY_12                0xc
#define T_KEY_13                0xd
#define T_KEY_14                0xe
#define T_KEY_15                0xf
#define T_KEY_16                0x10
#define T_KEY_17                0x11
#define T_KEY_18                0x12
#define T_KEY_19                0x13
#define T_KEY_20                0x14
#define T_KEY_21                0x15
#define T_KEY_22                0x16
#define T_KEY_23                0x17
#define T_KEY_24                0x18
#define T_KEY_25                0x19
#define T_KEY_26                0x1a
#define T_KEY_27                0x1b
#define T_KEY_28                0x1c
#define T_KEY_29                0x1d
#define T_KEY_30                0x1e
#define T_KEY_31                0x1f        // 0001 1111


//功能键

#define T_KEY_POWER								T_KEY_1

//短按
#define K_POWER									(T_KEY_POWER| T_KEY_SHORT)
//短按抬起
#define KU_POWER								(T_KEY_POWER| T_KEY_SHORT_UP)
//长按
#define KL_POWER								(T_KEY_POWER| T_KEY_LONG)
//长按抬起
#define KLU_POWER								(T_KEY_POWER| T_KEY_LONG_UP)
//按住不动
#define KH_POWER								(T_KEY_POWER| T_KEY_HOLD)

#define T_KEY_VOL                        T_KEY_1
#define T_KEY_LIGHT                      T_KEY_2
#define T_KEY_MODE                       T_KEY_3

//T_KEY_ENCODER1_CLOCKWISE 正转顺时针 T_KEY_ENCODER1_UNTICLOCKWISE 反转逆时针 light
//T_KEY_ENCODER2_CLOCKWISE 正转顺时针 T_KEY_ENCODER2_UNTICLOCKWISE 反转逆时针 vol
#define T_KEY_CODINGSWITCH1_CLOCKWISE			T_KEY_4
#define T_KEY_CODINGSWITCH1_UNTICLOCKWISE		T_KEY_5
#define T_KEY_CODINGSWITCH2_CLOCKWISE			T_KEY_6
#define T_KEY_CODINGSWITCH2_UNTICLOCKWISE		T_KEY_7
#define T_KEY_ENCODER1_CLOCKWISE				T_KEY_CODINGSWITCH1_CLOCKWISE
#define T_KEY_ENCODER1_UNTICLOCKWISE			T_KEY_CODINGSWITCH1_UNTICLOCKWISE
#define T_KEY_ENCODER2_CLOCKWISE				T_KEY_CODINGSWITCH2_CLOCKWISE
#define T_KEY_ENCODER2_UNTICLOCKWISE			T_KEY_CODINGSWITCH2_UNTICLOCKWISE

#define T_KEY_UPDATE							T_KEY_8

#define T_KEY_TEST_KEY							T_KEY_9


/*****************************************************************************
 * Module    : 按键长短按配置
 *****************************************************************************/
//按键详细定义 -> K_: Key Short Down;   KU_: Key Short Up;   KL_: Key Long Down;   KLU_: Key Long Up;   KH_: Key Hold;

#define KU_VOL                (T_KEY_VOL| T_KEY_SHORT_UP)
#define KL_VOL                (T_KEY_VOL| T_KEY_LONG)
#define KLU_VOL               (T_KEY_VOL| T_KEY_LONG_UP)

#define KU_LIGHT              (T_KEY_LIGHT| T_KEY_SHORT_UP)
#define KL_LIGHT              (T_KEY_LIGHT| T_KEY_LONG)
#define KLU_LIGHT             (T_KEY_LIGHT| T_KEY_LONG_UP)

#define KU_MODE               (T_KEY_MODE| T_KEY_SHORT_UP)
#define KL_MODE               (T_KEY_MODE| T_KEY_LONG)
#define KLU_MODE              (T_KEY_MODE| T_KEY_LONG_UP)

/***************************************************************************
  double key,third key
***************************************************************************/
#define T_KEY_VOL_DOUBLE                      T_KEY_10
#define T_KEY_VOL_THIRD_HIT                   T_KEY_11

#define DIVOOM_KEY_PRESS_LONG_TIME 3000
#define DIVOOM_KEY_SEND_PRESSING_TIME 100
#define DIVOOM_KEY_PRESS_LONG_LONG_TIME 6000
//enum
//{
	//DIVOOM_KEY_CLOCKWISE = 1,
	//DIVOOM_KEY_ANTICLOCKWISE = 2,
//};

// astrotoo的按键，都取自DIVOOM_KEY_XXX枚举
#define DIVOOM_KEY_ASTROTOO_KEY_UP				DIVOOM_KEY_PREV
#define DIVOOM_KEY_ASTROTOO_KEY_DOWN			DIVOOM_KEY_NEXT
#define DIVOOM_KEY_ASTROTOO_KEY_LEFT			DIVOOM_KEY_MUSIC_PREV
#define DIVOOM_KEY_ASTROTOO_KEY_RIGHT			DIVOOM_KEY_MUSIC_NEXT
#define DIVOOM_KEY_ASTROTOO_KEY_ENTER			DIVOOM_KEY_PLAY
#define DIVOOM_KEY_ASTROTOO_KEY_ESC				DIVOOM_KEY_ESC
#define DIVOOM_KEY_ASTROTOO_KEY_MUSIC_MODE		DIVOOM_KEY_MUSIC_MODE
#define DIVOOM_KEY_ASTROTOO_KEY_LIGVOL_ADD		DIVOOM_KEY_LIGHT_RIGHT
#define DIVOOM_KEY_ASTROTOO_KEY_LIGVOL_SUB		DIVOOM_KEY_LIGHT_LEFT
// #define DIVOOM_KEY_ASTROTOO_KEY_VOL_ADD			DIVOOM_KEY_VOL_ADD		// 短按音量+
// #define DIVOOM_KEY_ASTROTOO_KEY_VOL_SUB			DIVOOM_KEY_VOL_DEC		// 短按音量-
// #define DIVOOM_KEY_ASTROTOO_KEY_LIG_ADD			DIVOOM_KEY_LIGHT_RIGHT	// 长按亮度+
// #define DIVOOM_KEY_ASTROTOO_KEY_LIG_SUB			DIVOOM_KEY_LIGHT_LEFT	// 长按亮度-
#define DIVOOM_KEY_ASTROTOO_KEY_MUSIC_PLAY		DIVOOM_KEY_MUSIC_PLAY

typedef enum 
{
    DIVOOM_KEY_MENU = 0x20,           //灯光变化键
	DIVOOM_KEY_NEXT,    //下一首
	DIVOOM_KEY_PREV,    //上一首
	DIVOOM_KEY_VOL_ADD,    //左键
	DIVOOM_KEY_VOL_DEC,    //右键
	DIVOOM_KEY_PLAY,    //播放键
	DIVOOM_KEY_MODE,    //模式按键
	DIVOOM_KEY_ESC,    //
	DIVOOM_KEY_LIGHT_RIGHT,    //灯右键
	DIVOOM_KEY_LIGHT_LEFT,    //灯左键
	DIVOOM_KEY_LIGHT_ENTER,    //灯确认键
	DIVOOM_KEY_ALL_TEST,//产品测试键

	//新增
	DIVOOM_KEY_MUSIC_MODE,    // KEY_F17, PD12
	DIVOOM_KEY_MUSIC_PREV,    // KEY_F18, PD13
	DIVOOM_KEY_MUSIC_NEXT,    // KEY_F19, PD14
	DIVOOM_KEY_MUSIC_PLAY,    // KEY_F20, PD10
	DIVOOM_KEY_MUSIC_FAVORITE,    // KEY_F21, PD11

	DIVOOM_KEY_NULL = 0xFF,
}DIVOOM_KEY_VALUE;	// 注意不要和 E_KEY_ID 以及 LV_KEY_XXX 用的值冲突

typedef enum {
	DIVOOM_PRESS_SHORT,
	DIVOOM_PRESS_LONG,
	DIVOOM_PRESSING,
	DIVOOM_PRESS_UP,
	DIVOOM_PRESS_DOUBLE,
	DIVOOM_NO_PRESS,
	DIVOOM_PRESS_LONG_LONG,
	DIVOOM_PRESSING_LONG,
}DIVOOM_PRESS_TYPE;	// 前面几个值与 T_ePRESS_TYPE 顺序保持一致

void divoom_user_key_send_key_info(int key_id, int key_status);
/* 注入按键到 LVGL 队列，但不重置展机空闲计时（自动回表盘用）。 */
void divoom_user_key_inject_key_info(int key_id, int key_status);

#endif

