/* @brief   keypad head file
 * Copyright (C) 2013 Anyka (GuangZhou) Software Technology Co., Ltd.
 * @author  
 * @date 
 * @version 1.0
*******************************************************************************/

#ifndef __AK_KEYPAD_H__
#define __AK_KEYPAD_H__

//#include "hal_int_msg.h"

#define MAX_ADKEY_NUM           8
#define MAX_GPIO_KEY_NUM        4

#define IM_LIBK_KEYPAD          0x82//(IM_USER + 0x2)

#define INVALID_KEY_ID          (0xff)  ///< invalid key ID

typedef struct
{
    T_U8    msg;        //Specifies the message identifier
    T_U8    bparam;     //Specifies additional information about the message. 
                        //The exact meaning depends on the value of the msg member
    T_U16   hwparam;    //Specifies additional information about the message. 
                        //The exact meaning depends on the value of the msg member
}DIVOOM_ST_MSG;
T_BOOL post_int_message(T_U8 msg, T_U8 bparam, T_U16 hwparam);
T_BOOL peek_int_message(DIVOOM_ST_MSG *pstmsg);



typedef T_U8            T_KEY_ID;

/**
 * @brief keypad param define
 */
typedef struct
{
    T_U16               m_min;              ///< AD按键变化的最小值
    T_U16               m_max;              ///< AD按键变化的最大值
	T_U8 				key_id;
} T_KEY_ANALOG;

typedef struct
{
    T_U8                m_ain_id;           ///< AD按键连接到哪个ain
    T_U8                m_adkey_count;      ///< AD按键的数量
    T_U16               m_ad_val_min;       ///< 有效按键的最小值
    T_U16               m_ad_val_max;       ///< 有效按键的最大值
    T_KEY_ANALOG        m_adkey_val_array[MAX_ADKEY_NUM];  ///< 指向AD按键的映身表
} 
T_KEYPAD_ANALOG_PARAM;

typedef struct
{
    T_U8                gpio;              ///按键的GPIO号
    T_U8                polarity;          ///GPIO 触发电平
	T_U8 				key_id;
} T_KEY_GPIO;

typedef struct
{
    T_U8               m_gpiokey_count;
    T_KEY_GPIO         m_gpiokey_array[MAX_GPIO_KEY_NUM];
}T_KEYPAD_GPIO_PARAM;

typedef struct
{
	T_U8                key_en;
	T_U8				key_id;
	T_U8				check_tick; //100ms
}T_KEY_UPDATE_PARAM;

typedef struct {
    T_U8 key_id;
    T_U8 short_id;
    T_U8 long_id;
    T_U8 double_id;
}
T_KEY_CONVERT;

/**
 * @brief key status define
 */
typedef enum
{
    eKEYDOWN,                         ///< key press down
    eKEYPRESS,                        ///< key press down and hold 
    eKEYUP,                           ///< key up
    eKEYDOUBLE,                       ///< key double press
    eKEYPRESSLONG,					  ///< key press for long time
    eKEYSTATUS_NUM                    ///< num of key status
} 
E_KEYPAD_STATUS;

/*******************************************************************************
 * @brief  register analog keypad
 * @author  liaozhijun
 * @date    2017-03-01
 * @param   param analog key parameter
 * @return  T_BOOL
*******************************************************************************/
T_BOOL ak_keypad_analog_register(T_KEYPAD_ANALOG_PARAM *param);

/*******************************************************************************
 * @brief  register gpio keypad
 * @author  liaozhijun
 * @date    2017-03-01
 * @param   param gpio key parameter
 * @return  T_BOOL
*******************************************************************************/
T_BOOL ak_keypad_gpio_register(T_KEYPAD_GPIO_PARAM *param);

/*******************************************************************************
 * @brief   initialize keypad
 * Function keypad_init() must be called before call any other keypad functions
 * @author  liaozhijun
 * @date    2017-03-01
 * @param  
 * @return  T_BOOL
 * @retval  AK_TRUE:init successfully; AK_FALSE:init failed.
*******************************************************************************/
T_BOOL ak_keypad_init(T_VOID);

/*******************************************************************************
 * @brief   set key value convert table
 * @author  liaozhijun
 * @date    2017-03-01
 * @param   conv_table key value convert table
 * @param   key_num key numbers
 * @return  T_VOID
*******************************************************************************/
T_VOID ak_keypad_set_convtable(T_KEY_CONVERT *conv_table, T_U8 key_num);


T_VOID keypad_state_scan(T_U32 delay);


#endif
