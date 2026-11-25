#ifndef __divoom_keypad_h
#define __divoom_keypad_h

#include "ak_keypad.h"


typedef struct
{
    T_KEYPAD_ANALOG_PARAM adkey_param;

    T_KEYPAD_GPIO_PARAM gpio_key_param;

    T_KEY_UPDATE_PARAM testkey;
    T_KEY_UPDATE_PARAM formatkey;
    T_KEY_UPDATE_PARAM showkey;


    T_KEY_CONVERT key_conv[MAX_ADKEY_NUM+MAX_GPIO_KEY_NUM];
}
T_BSP_KEYPAD_CONFIG;


typedef struct _divoom_key_info
{
    uint8 value;
    uint8 type;
}divoom_key_info, *Pdivoom_key_info;


typedef enum
{
    PRESS_SHORT = 0,
    PRESS_LONG,
    PRESSING,
    PRESS_LONG_UP,
    PRESS_DOUBLE,
    PRESS_LONG_LONG,	//长按8秒
    PRESS_TYPE_NUM
}
T_ePRESS_TYPE;


//根据红外按键的键值来指定值
typedef enum {
    kbNULL = INVALID_KEY_ID,
    kbPRE = 0x07,       kbSPEED = 0x08,     kbNEXT = 0x09,      kbVOLSUB = 0x0c,    
    kbNEXTFILE = 0x0d,  kbOK = 0x15,        kbVOLADD = 0x16,    kbBACKWARD = 0x18,  
    kbPREFILE = 0x19,   kbRECORD = 0x40,    kbFUNC = 0x43,      kbBT = 0x44,
    kbMUTE = 0x45,      kbEQ = 0x46,        kbPOWER = 0x47,     kbFORWARD = 0x5e,
    kbLEFT,             kbRIGHT,            kbMODE,             kbVOLUME,         
    kbEFFECT,           kbAPPS,				kbSIRI,             kbDIVOOM_SHOW, kbLIGHT = 0x67,kbCOLOR,
    kbSPPVOL,			kbOMODE,			kbTEST,				kbWIFICONFIG, MAX_KEY_NUM
}
E_KEY_ID;



typedef T_VOID (*T_fKeyMsg_Handle)(T_U8 keyVal, T_U16 pressType);

void divoom_keypad_init(void *sendmsg_func);
void divoom_keypad_set_msgsend_func(void *sendmsg_func);
void *divoom_keypad_get_config(void);


#endif

