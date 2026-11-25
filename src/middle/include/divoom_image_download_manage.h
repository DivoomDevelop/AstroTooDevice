#ifndef _divoom_image_download_manage_h_

#define _divoom_image_download_manage_h_

/**
* NAME         divoom_image_download_manage_save
* @BRIEF	   将模块从本地加载之前保存的数据
* @PARAM	   
* @RETURN	 返回NONE
* @RETVAL
*/

void divoom_image_download_manage_load(void);

/**
* NAME         divoom_image_download_manage_save
* @BRIEF	   将模块所有已经加载的图片数据保存
* @PARAM	   
* @RETURN	 返回NONE
* @RETVAL
*/

void divoom_image_download_manage_save(void);


/**
* NAME         divoom_image_download_manage_init
* @BRIEF	   模块初始化，加载所有的以前保存在本地的信息
* @PARAM	   
* @RETURN	 返回NONE
* @RETVAL
*/

void divoom_image_download_manage_init(void);

/**
* NAME         divoom_image_download_manage_start_update
* @BRIEF	   通知模块将进行更新，将所有的元素status状态设置为未使用
* @PARAM	   
* @RETURN	 返回NONE
* @RETVAL
*/

void divoom_image_download_manage_start_update(void);

/**
* NAME         divoom_image_download_manage_add_file
* @BRIEF	   将文件加入缓冲队列，由后台加载文件
* @PARAM	   file_name
* @RETURN	 返回NONE
* @RETVAL
*/

void divoom_image_download_manage_add_file(char *file_name);


/**
* NAME         divoom_image_download_manage_del_file
* @BRIEF	   将文件从缓冲队列移除
* @PARAM	   file_name
* @RETURN	 返回NONE
* @RETVAL
*/

void divoom_image_download_manage_del_file(char *file_name);


/**
* NAME         divoom_image_download_manage_end_update
* @BRIEF	   更新文件列表完成，将未使用的文件从队列中清除
* @PARAM	   
* @RETURN	 返回NONE
* @RETVAL
*/

void divoom_image_download_manage_end_update(void);

/**
* NAME         divoom_image_download_manage_get_file
* @BRIEF	   获取文件数据，
* @PARAM	   file_name
               pic_len      数据长度
* @RETURN	   数据指针
* @RETVAL
*/

uint8 *divoom_image_download_manage_get_file(char *file_name, int *pic_len);


/**
* NAME         divoom_image_download_manage_stop
* @BRIEF	   停止文件下载，结束下载线程
* @PARAM	   
* @RETURN	 返回NONE
* @RETVAL
*/

void divoom_image_download_manage_stop(void);

#endif

