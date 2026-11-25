#ifndef _divoom_play_cache_com_h_
#define _divoom_play_cache_com_h_
typedef void (*DIVOOM_PLAY_CACHE_COM_GET_NEW_ID)(void *handle, T_U8 id, Pdivoom_play_gif_image_info image_info, T_U8 ld_id);
void *divoom_play_cache_com_init(uint8 *lcd_id, uint8 lcd_num, int gif_play_time, DIVOOM_PLAY_CACHE_COM_GET_NEW_ID pcallback, void *para);
void divoom_play_cache_com_golbal_init(void);
void divoom_play_cache_com_exit(void *handle);
/*
  Function:       divoom_play_cache_com_del_text
  Description:	  清除所有文本框
  Input:          NONE
  Return:         NONE
*/
T_VOID divoom_play_cache_com_del_text(void *handle);
T_VOID divoom_play_cache_com_update_info(void *handle,Pdivoom_disp_ctrl_gif_info pdisp_info);
T_VOID divoom_play_cache_com_set_gif_speed(void *handle,uint16 gif_speed);
T_VOID divoom_play_cache_com_update_lcd_list(void *handle,uint8 *lcd_list, uint8 lcd_num);

/*********************************************************************
  Function:        divoom_play_cache_com_get_play_image_id
  Description:     得到播放的ＩＤ
  Input:           
  Return:          none
**********************************************************************/

int divoom_play_cache_com_get_play_image_id(void *handle);

/*********************************************************************
  Function:        divoom_play_cache_com_clear_all_cache
  Description:     清除所有的缓冲区
  Input:           none
  Return:          none
**********************************************************************/

void divoom_play_cache_com_clear_all_cache(void *handle);
void divoom_play_cache_com_wait_notice_start(void *handle);
void divoom_play_cache_com_wait_notice_end(void *handle);

/*********************************************************************
  Function:        divoom_play_cache_com_ctrl_run_status
  Description:     控制运行状态
  Input:           flag
  Return:           NONE
**********************************************************************/

void divoom_play_cache_com_ctrl_run_status(uint8 flag);

#endif


