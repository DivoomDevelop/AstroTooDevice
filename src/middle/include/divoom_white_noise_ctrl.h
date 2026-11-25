#ifndef _divoom_white_noise_ctrl_h_
#define _divoom_white_noise_ctrl_h_


/**
* NAME         divoom_white_noise_ctrl_init
               初始化句柄
* @PARAM       vol_table        所有音源的音量

* @RETURN	   handle
* @RETVAL
*/

void *divoom_white_noise_ctrl_init(int *vol_table);



/**
* NAME         divoom_white_noise_ctrl_update_volume
               更新音量
* @PARAM       vol_table        所有音源的音量
               handle
* @RETURN	none
* @RETVAL
*/

void divoom_white_noise_ctrl_update_volume(void *handle,  int *vol_table);


/**
* NAME         divoom_white_noise_ctrl_read_pcm
               读数据，每次读的数据量必须是以K为单位
* @PARAM       handle
               read_buf
               read_size
* @RETURN	读到的数据量
* @RETVAL
*/

int divoom_white_noise_ctrl_read_pcm(void *handle,  uint8 *read_buf, int read_size);


/**
* NAME         divoom_white_noise_ctrl_exit
               退出并释放所有资源
* @PARAM       handle
* @RETURN	   none
* @RETVAL
*/

void divoom_white_noise_ctrl_exit(void *handle);

/**
* NAME         divoom_white_noise_ctrl_read_loop
               主循环读所有音量大于0的文件内容
* @PARAM       data
* @RETURN	none
* @RETVAL
*/


void divoom_white_noise_ctrl_read_loop(void *data);

int divoom_white_noise_ctrl_is_queue_not_empty(void *handle);


#endif


