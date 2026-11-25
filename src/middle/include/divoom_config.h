#ifndef _divoom_config_h_
#define _divoom_config_h_
typedef enum
{
	DIVOOM_eCFG_SYSTEM = 0,
		
//#ifdef SUPPORT_SDCARD
	DIVOOM_eCFG_SDAUDIO,
//#endif

//#ifdef SUPPORT_USBHOST
	DIVOOM_eCFG_USBAUDIO,
//#endif

//#ifdef SUPPORT_BLUETOOTH
	DIVOOM_eCFG_BTDEV,
	DIVOOM_eCFG_BTPLAYER,
//#endif

//#ifdef SUPPORT_LINEIN_EQ
	DIVOOM_eCFG_LINEIN,
//#endif

/***************回音消除*************/	
//#ifdef SUPPORT_AECS
	DIVOOM_eCFG_AECS, 
//#endif

//#ifdef SUPPORT_ECHO
	DIVOOM_eCFG_ECHO,
//#endif

/***************EQ音效*************/
//#ifdef SUPPORT_EQ 
//	#ifdef SUPPORT_BLUETOOTH
	DIVOOM_eCFG_BT_EQ,
//	#endif
	
	#ifdef SUPPORT_LINEIN_EQ
	DIVOOM_eCFG_LINEIN_EQ,
	#endif

//	#ifdef SUPPORT_SDCARD
	DIVOOM_eCFG_SD_EQ,
//	#endif
//#endif

/***************ASCL音效*************/
//#ifdef SUPPORT_ASLC
//	#ifdef SUPPORT_BLUETOOTH
	DIVOOM_eCFG_BT_ASLC,
//	#endif

	#ifdef SUPPORT_LINEIN_EQ
	DIVOOM_eCFG_LINEIN_ASLC,
	#endif

//	#ifdef SUPPORT_SDCARD
	DIVOOM_eCFG_SD_ASLC,
//	#endif
//#endif

/*************3D enhance音效***********/
//#ifdef SUPPORT_3DENHANCE
//	#ifdef SUPPORT_BLUETOOTH
	DIVOOM_eCFG_BT_3D,
//	#endif
	
//	#ifdef SUPPORT_LINEIN_EQ
	DIVOOM_eCFG_LINEIN_3D,
//	#endif

//	#ifdef SUPPORT_SDCARD
	DIVOOM_eCFG_SD_3D,
//	#endif
//#endif

/***************MvBass音效*************/
//#ifdef SUPPORT_MVBASS
//	#ifdef SUPPORT_BLUETOOTH
	DIVOOM_eCFG_BT_MVBASS,
//	#endif
	
//	#ifdef SUPPORT_LINEIN_EQ
	DIVOOM_eCFG_LINEIN_MVBASS,
//	#endif
	
//	#ifdef SUPPORT_SDCARD
	DIVOOM_eCFG_SD_MVBASS,
//	#endif
//#endif
    DIVOOM_eCFG_RADIO,
	DIVOOM_eCFG_ALARMTIMESAVE,
	DIVOOM_eCFG_ALARMGIF,
	DIVOOM_eCFG_BOOTGIF,
	DIVOOM_eCFG_DIALYTIMESAVE,
	DIVOOM_eCFG_DIALYGIF,
	DIVOOM_eCFG_TIMEMANAGE,
	DIVOOM_eCFG_TIMEMANAGEGIF,
	DIVOOM_eCFG_DIVOOMSLEEP,
	DIVOOM_eCFG_DIVOOMSYSTEMCONF,
	DIVOOM_eCFG_DIVOOMTEMP,
	DIVOOM_eCFG_DIVOOM_ANCS_GIF,
	DIVOOM_eCFG_DIVOOMLIGHT,
	DIVOOM_eCFG_DIVOOMHOTFILE,
	DIVOOM_eCFG_DIVOOMUPDATEFILE,
	DIVOOM_eCFG_DIVOOMRHYTHM_GIF,

	DIVOOM_eCFG_DIVOOMRHYTHM_TMP_GIF,
	DIVOOM_eCFG_DIVOOM_GALLERY_INFO,
	DIVOOM_eCFG_DIVOOM_BLUE_PASSWORD,
	DIVOOM_eCFG_DIVOOM_POWER_VOICE_VOL,
	DIVOOM_eCFG_DIVOOM_POWER_ON_ABORT,
	DIVOOM_eCFG_DIVOOM_USER_DEFINE_CFG,
	DIVOOM_eCFG_DIVOOM_SYS_VOL_CFG,
	DIVOOM_eCFG_DIVOOM_ANCS_SWITCH,
	DIVOOM_eCFG_DIVOOM_CUSTOM_INFO,
	DIVOOM_eCFG_DIVOOM_LIGHT,
    DIVOOM_eCFG_DIVOOMTEMP_CONFIG,
    DIVOOM_eCFG_DIVOOMFONTINFO,
    DIVOOM_eCFG_DIVOOMCOPYDRAW,

	DIVOOM_eCFG_NUM,
}
T_DIVOOM_eCFG_ITEM;


T_VOID divoom_config_auto_resort(T_VOID);
T_VOID divoom_config_power_off(T_VOID);
T_VOID divoom_config_write(T_U8 divoom_item, T_U8 divoom_id, T_U8 *item_data, T_U16 item_size);
T_VOID divoom_config_del(T_U8 divoom_item, T_U8 divoom_id);
T_U8 *divoom_config_read(T_U8 divoom_item, T_U8 divoom_id,T_U16 *item_size);
T_VOID divoom_config_lock_flash(T_U16 item);
T_U16  divoom_config_get_lock_item(T_VOID);
T_VOID divoom_config_clear(T_VOID);
T_VOID divoom_config_check_flash(T_U32 page_addr, T_U32 page_cnt);
T_VOID divoom_config_init(T_VOID);


/*********************************************************
  Function:		divoom_config_get_start_page
  Description:		 得到起始页
  Input:			divoom_item?
  ????   divoom_id         
  Return:			返回文件的起始位置
************************************************************/

T_U32 divoom_config_get_start_page(T_U8 divoom_item, T_U8 divoom_id);

/*********************************************************
  Function:		divoom_config_set_pos
  Description:		设置读句柄的偏移，目前只支持从文件头偏移
  Input:			handle?句柄
  ????   pos          偏移
  Return:			
************************************************************/

T_VOID divoom_config_set_pos(T_VOID *handle, T_U16 pos);


/*********************************************************
  Function:		divoom_config_read_off
  Description:		顺序读文件
  Input:			handle?句柄
  ????   buf          
                          len
  Return:			
************************************************************/

T_U8 divoom_config_read_off(T_VOID *handle, T_U8 *buf, T_U16 len);

/*********************************************************
  Function:		divoom_config_close
  Description:		使用完文件句柄，关闭句柄
  Input:			handle?句柄
  Return:			
************************************************************/

T_VOID divoom_config_close(T_VOID *handle);

/*********************************************************
  Function:		divoom_config_open
  Description:		打开文件句柄
  Input:			divoom_item?
  ????   divoom_id         
  Return:			返回文件句柄
************************************************************/

T_VOID *divoom_config_open(T_U8 divoom_item, T_U8 divoom_id);

/*********************************************************
  Function:		divoom_config_write_step
  Description:		在配置文件为这项分配好空间，但是不写数据，让外面分次写入数据
  Input:			divoom_item
                           divoom_id
                           item_size
  Return:			
************************************************************/

T_U32 divoom_config_write_step(T_U8 divoom_item, T_U8 divoom_id, T_U16 item_size);

#endif

