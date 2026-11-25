#ifndef _divoom_screen_save_h_
#define _divoom_screen_save_h_

void divoom_screen_save_check_over(void * handle);
void *divoom_screen_save_init(void * handle, uint32 file_size, uint32 check_sum, uint32 version, uint32 id);
void divoom_screen_save_update(void * handle,uint8 *buf, uint32 offset, uint16 len);
void divoom_screen_save_exit(T_pVOID handle);
T_BOOL divoom_screen_save_send_version(void * handle);
uint32 divoom_screen_save_check_next_pos(uint8 cur_pos);
void divoom_screen_save_check_timeout(void * handle);
uint32 divoom_screen_save_get_all_num(void);
uint8 *divoom_screen_save_get_default_config(uint16 *item_size);

/**
* NAME         divoom_screen_save_flush_version
* @BRIEF	当ＨＯＴ文件更新完成后，调用这个函数更新版本号
* @PARAM	num          厂商数目
                    buf 　　厂商所有信息
* @RETURN	ＮＯＮＥ
* @RETVAL
*/

void divoom_screen_save_update_version(uint16 num, uint8 *buf);

#endif

