#ifndef _DIVOOM_SD_FILE_
#define _DIVOOM_SD_FILE_

typedef struct _divoom_file_id_info
{
	uint32 mode_id;
	uint32 file_id;
}divoom_file_id_info, *Pdivoom_file_id_info;

typedef struct _divoom_sd_file_list_info
{
    int update_flag;
    void *lock_handle;
	FILE *fp; //列表文件句柄
	uint8 file_list_path[64]; //保存一下文件列表路径
	uint16 id_num; //统计file_id数
	void *id_list_queue; //队列便于管理
}divoom_sd_file_list_info, *Pdivoom_sd_file_list_info;

/*
  Function:       divoom_sd_file_list_manage_read_id
  Description:    获取该文件id的文件路径，如果不存在返回NULL
  Input:          handle 指定要更新哪个列表 file_id_info 文件id信息 
  Return:         不存在返回NULL
  Author:         wangft
  Data:           2020-12-31
*/
void *divoom_sd_file_list_manage_read_id(void *handle, void *file_id_info);

/*
  Function:       divoom_sd_file_list_manage_update_list
  Description:    更新sd列表信息, 把文件列表信息写进sd卡
  Input:          handle 指定要更新哪个列表
  Return:         none
  Author:         wangft
  Data:           2020-12-31
*/
void divoom_sd_file_list_manage_update_list(void *handle);

/*
  Function:       divoom_sd_file_list_manage_del_list
  Description:    删除指定的id信息
  Input:          handle 指定要删除哪个列表 file_id_info 指定要增加的动画id信息
  Return:         none
  Author:         wangft
  Data:           2020-12-31
*/
void divoom_sd_file_list_manage_del_list(void *handle, void *file_id_info);

/*
  Function:       divoom_sd_file_list_manage_add_list
  Description:    新增加id信息到文件列表
  Input:          handle 指定要增加哪个列表 file_id_info 指定要增加的动画id信息
  Return:         none
  Author:         wangft
  Data:           2020-12-31
*/
void divoom_sd_file_list_manage_add_list(void *handle, void *file_id_info);

/*
  Function:       divoom_sd_file_list_manage_init
  Description:    获取文件列表
  Input:          file_list_path 文件列表路径
  Return:         返回相应的文件列表的句柄，用来区分是自定义列表还是其他列表
  Author:         wangft
  Data:           2020-12-31
*/
void *divoom_sd_file_list_manage_init(uint8 *file_list_path);

/*
  Function:       divoom_sd_file_list_manage_exit
  Description:    文件列表管理退出
  Input:          none
  Return:         none
  Author:         wangft
  Data:           2020-12-31
*/
void divoom_sd_file_list_manage_exit(void *handle);

/*
  Function:       divoom_sd_file_list_manage_del_list_file
  Description:    删除ＳＤ列表文件
  Input:          handle 指定要更新哪个列表
  Return:         none
  Author:         wangft
  Data:           2020-12-31
*/
void divoom_sd_file_list_manage_del_list_file(void *handle);

#endif
