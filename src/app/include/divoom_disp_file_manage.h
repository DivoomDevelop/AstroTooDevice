#ifndef _DIVOOM_DISP_FILE_MANAGE_H_
#define _DIVOOM_DISP_FILE_MANAGE_H_

/**
 * NAME               divoom_disp_file_manage_get_all_config
 * *  @brief       	获取所有的灯效配置数据
 * *  @param[in] 		buf 缓冲区 size 大小
 * *	 @author         wangft               
 * *  @return        0 失败 1 成功
 * */

int divoom_disp_file_manage_get_all_config(void *buf, int size);

/**
 * NAME               divoom_disp_file_manage_set_all_config
 * *  @brief       	设置所有的灯效配置数据
 * *  @param[in] 		buf 数据 size 大小
 * *	 @author         wangft               
 * *  @return        0 失败 1 成功
 * */

int divoom_disp_file_manage_set_all_config(void *buf, int size);
#endif
