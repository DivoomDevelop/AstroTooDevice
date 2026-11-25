#ifndef _divoom_spiffs_manage_h_
#define _divoom_spiffs_manage_h_

/**
 * NAME              divoom_spiffs_manage_init
 * *  @brief       	挂载spiffs文件系统到spiffs分区
 * *  @param[in] 		none
 * *	 @author         wangft               
 * *  @return        0 获取失败， 1 获取成功
 * */

int divoom_spiffs_manage_init(void);

/**
 * NAME              divoom_spiffs_manage_exit
 * *  @brief       	卸载spiffs文件系统到spiffs分区
 * *  @param[in] 		none
 * *	 @author         wangft               
 * *  @return       none
 * */

void divoom_spiffs_manage_exit(void);

#endif
