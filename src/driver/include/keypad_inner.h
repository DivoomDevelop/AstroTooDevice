/*******************************************************************************
 * @file    keypad.c
 * @brief   keypad function file, provide keypad APIs: init, scan...
 * Copyright (C) 2013 Anyka (GuangZhou) Software Technology Co., Ltd.
 * @author  zhanggaoxin
 * @date    2013-01-25
 * @version 1.0
 * @ref     1080L programer guide
*******************************************************************************/
#include "divoom_type_tivoo.h"


#ifndef __KEYPAD_INNER_H__
#define __KEYPAD_INNER_H__

//长按键按下的时间长
#define LONG_PRESS_TIME             (1000)  ///< 1000ms

//主动检查键值TIMER的时间间隔
#define KEY_SCAN_TIME               (50)//(100)   ///< 100 ms
#define KEY_LOOP_TIME               (10)    ///< 10  ms

#define KEY_LONG_TIME               (100)   ///< 200 ms
#define DOUBLE_KEY_TIME             (500)//ms

#define KEY_LONG_HOLD_TIMES			(LONG_PRESS_TIME+KEY_LONG_TIME)

//长按的时间
#define KEY_LONG_LONG_PRESS_TIMES	(8000+KEY_LONG_TIME)



typedef T_BOOL (*T_fKEYPAD_ENABLE_CBK)(T_BOOL enable);

typedef T_BOOL (*T_fKEYPAD_GETID_CBK)(T_KEY_ID *key_id);

typedef enum
{
    E_KEYCONV_SUCCESS,
    E_KEYCONV_UNSUPPORT,
    E_KEYCONV_FAIL,
    E_KEYCONV_DOUBLE_CHECK
}
E_KEY_CONV_STATUS;


T_BOOL keypad_analog_get_keyid(T_KEY_ID *key_id);


T_BOOL keypad_gpio_get_keyid(T_KEY_ID *key_id);


T_VOID keypad_send_key(T_KEY_ID key_id, T_BOOL long_press, E_KEYPAD_STATUS status);
E_KEY_CONV_STATUS keypad_conv(T_U8 keyId, T_BOOL longPress, T_U8 *keyVal);


#endif

