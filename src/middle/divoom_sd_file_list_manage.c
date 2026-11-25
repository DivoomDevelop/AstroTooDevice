#include "divoom.h"

/*
  Function:       divoom_sd_file_list_manage_del_list_file
  Description:    删除ＳＤ列表文件
  Input:          handle 指定要更新哪个列表
  Return:         none
  Author:         wangft
  Data:           2020-12-31
*/

void divoom_sd_file_list_manage_del_list_file(void *handle)
{
	Pdivoom_sd_file_list_info file_list_handle = (Pdivoom_sd_file_list_info)handle;
    
    divoom_system_get_lock(file_list_handle->lock_handle);
    file_list_handle->update_flag = 1;
    divoom_app_com_del_file((char *)file_list_handle->file_list_path);
    divoom_system_free_lock(file_list_handle->lock_handle);
}

/*
  Function:       divoom_sd_file_list_manage_update_list
  Description:    更新sd列表信息
  Input:          handle 指定要更新哪个列表
  Return:         none
  Author:         wangft
  Data:           2020-12-31
*/
void divoom_sd_file_list_manage_update_list(void *handle)
{
	Pdivoom_sd_file_list_info file_list_handle = (Pdivoom_sd_file_list_info)handle;
	uint16 i = 0;
    uint32 check_sum = 0;
	Pdivoom_file_id_info file_info = NULL, pfile_list;

    if(handle == NULL)
    {
        return ;
    }
	//先写id_num
    divoom_system_get_lock(file_list_handle->lock_handle);
	if (file_list_handle->id_num && file_list_handle->update_flag)
	{
		file_list_handle->fp = divoom_port_file_open((char *)file_list_handle->file_list_path, "wb"); //打开一个文件，可读写，该文件必须存在
		if (file_list_handle->fp)
		{
            file_list_handle->id_num = divoom_queue_items(file_list_handle->id_list_queue);
			divoom_port_file_write(&file_list_handle->id_num, 1, sizeof(file_list_handle->id_num), file_list_handle->fp); //先写id_num
			check_sum = divoom_app_com_get_check_sum((uint8 *)&file_list_handle->id_num, sizeof(file_list_handle->id_num));    
            pfile_list = divoom_system_malloc(sizeof(divoom_file_id_info) * file_list_handle->id_num);
			for (i = 0; i < file_list_handle->id_num; i++)
			{
				file_info = divoom_queue_get_index_item(file_list_handle->id_list_queue, i);
                if(file_info == NULL)
                {
                    divoom_print("err!tol: %d, %d, %d\n",file_list_handle->id_num, i, divoom_queue_items(file_list_handle->id_list_queue));
                    break;
                }
                memcpy(&pfile_list[i], file_info, sizeof(divoom_file_id_info));
			}
			check_sum += divoom_app_com_get_check_sum((uint8 *)pfile_list, sizeof(divoom_file_id_info)*file_list_handle->id_num);    
			divoom_port_file_write(pfile_list, 1, sizeof(divoom_file_id_info)*file_list_handle->id_num, file_list_handle->fp);//后写id_list
            file_list_handle->update_flag = 0;
			divoom_port_file_write(&check_sum, 1, sizeof(check_sum), file_list_handle->fp); //先写id_num
			divoom_port_file_close(file_list_handle->fp);
            divoom_system_free(pfile_list);
            divoom_print("we will write number: %d, check sum : %d\n", file_list_handle->id_num, check_sum);
		}
	}
    divoom_system_free_lock(file_list_handle->lock_handle);
}

/*
  Function:       divoom_sd_file_list_manage_compare_func
  Description:	  文本框比较函数
  Input:          item1、 item2要比较的数据
  Return:         1 相等 0 不相等
  Author:         wangft
  Data:           2020-12-21
*/
int divoom_sd_file_list_manage_compare_func(void *item1, void *item2)
{
	Pdivoom_file_id_info id_info1 = (Pdivoom_file_id_info)item1;
	Pdivoom_file_id_info id_info2 = (Pdivoom_file_id_info)item2;

	if (id_info1->file_id == id_info2->file_id)
		return 0;
	else 
		return 1;
}

/*
  Function:       divoom_sd_file_list_manage_read_id
  Description:    获取该文件id的文件路径，如果不存在返回NULL
  Input:          handle 指定要更新哪个列表 file_id_info 文件id信息 
  Return:         返回 该id的文件路径
  Author:         wangft
  Data:           2020-12-31
*/
void *divoom_sd_file_list_manage_read_id(void *handle, void *file_id_info)
{
	Pdivoom_sd_file_list_info file_list_handle = (Pdivoom_sd_file_list_info)handle;
	Pdivoom_file_id_info file_info = (Pdivoom_file_id_info)divoom_queue_get_cmp(file_list_handle->id_list_queue, file_id_info, divoom_sd_file_list_manage_compare_func);
	return file_info;
}

/*
  Function:       divoom_sd_file_list_manage_del_list
  Description:    删除指定的id信息
  Input:          handle 指定要删除哪个列表 file_id_info 指定要增加的动画id信息
  Return:         none
  Author:         wangft
  Data:           2020-12-31
*/
void divoom_sd_file_list_manage_del_list(void *handle, void *file_id_info)
{
	//查找队列
	Pdivoom_sd_file_list_info file_list_handle = (Pdivoom_sd_file_list_info)handle;
	Pdivoom_file_id_info file_info = (Pdivoom_file_id_info)divoom_queue_pop_cmp(file_list_handle->id_list_queue, file_id_info, divoom_sd_file_list_manage_compare_func);

	if (file_info)
	{
        divoom_system_get_lock(file_list_handle->lock_handle);
        file_list_handle->id_num --;
        file_list_handle->update_flag = 1;
        divoom_app_com_del_file((char *)file_list_handle->file_list_path);
        divoom_system_free_lock(file_list_handle->lock_handle);
		divoom_free(file_info);
	}
}

/*
  Function:       divoom_sd_file_list_manage_add_list
  Description:    新增加id信息到文件列表
  Input:          handle 指定要增加哪个列表 file_id_info 指定要增加的动画id信息
  Return:         none
  Author:         wangft
  Data:           2020-12-31
*/
void divoom_sd_file_list_manage_add_list(void *handle, void *file_id_info)
{
	Pdivoom_sd_file_list_info file_list_handle = (Pdivoom_sd_file_list_info)handle;
	Pdivoom_file_id_info file_info = divoom_queue_get_cmp(file_list_handle->id_list_queue, file_id_info, divoom_sd_file_list_manage_compare_func);

	if (file_info != NULL)
		return;

	file_info = divoom_system_malloc(sizeof(divoom_file_id_info));
	if (file_info)
	{
		memcpy(file_info, (Pdivoom_file_id_info)file_id_info, sizeof(divoom_file_id_info));
        divoom_system_get_lock(file_list_handle->lock_handle);
        file_list_handle->id_num ++;
        file_list_handle->update_flag = 1;
        divoom_app_com_del_file((char *)file_list_handle->file_list_path);
        divoom_system_free_lock(file_list_handle->lock_handle);
		divoom_queue_push(file_list_handle->id_list_queue, file_info);
	}
}

/*
  Function:       divoom_sd_file_list_manage_get_id_from_file_name
  Description:    从文件名称中获取mode_id 及 file_id
  Input:          file_name 文件名 mode_id 保存mode  file_id 保存文件id
  Return:         none
  Author:         wangft
  Data:           2020-12-31
*/
void divoom_sd_file_list_manage_get_id_from_file_name(char *file_name, uint32 *mode_id, uint32 *file_id)
{
	//文件命名格式：mode_id-file_id.bin
	char *p = NULL;

	if (file_name == NULL)
		return;
	
	*mode_id = atoi(file_name);
	p = strchr(file_name, '-');
	if (p)
	{
		p++;
		*file_id = atoi(p);
	}
}

/*
  Function:       divoom_sd_flie_list_manage_get_file_size
  Description:    获取文件大小
  Input:          file_name 文件名
  Return:         none
  Author:         wangft
  Data:           2020-12-31
*/
uint32 divoom_sd_flie_list_manage_get_file_size(char *file_name)
{
    uint32 file_size = 0;   
    return file_size;  
}

/*
  Function:       divoom_sd_file_list_manage_search_list
  Description:    查找本地动画生成列表
  Input:          handle 句柄
  Return:         none
  Author:         wangft
  Data:           2020-12-31
*/
void divoom_sd_file_list_manage_search_list(void *handle)
{	
	char *cbuf = "*.bin";
	void *file_path_local_queue = NULL;
	uint16 id_num = 0;
	Pdivoom_file_list_info file_info = NULL;

	Pdivoom_sd_file_list_info file_list_handle = (Pdivoom_sd_file_list_info)handle;
	if (strstr((char *)file_list_handle->file_list_path, DIVOOM_USER_RECOMMEND_DIR))
		file_path_local_queue = divoom_file_manage_get_list(DIVOOM_USER_RECOMMEND_DIR, cbuf, divoom_sd_flie_list_manage_get_file_size);

	id_num = divoom_queue_items(file_path_local_queue);
	divoom_print("id_num %d", id_num);
	file_list_handle->id_num = id_num;
	Pdivoom_file_id_info new_id_info = NULL;
	
	while (divoom_queue_not_empty(file_path_local_queue))
	{
		file_info = (Pdivoom_file_list_info)divoom_queue_pop(file_path_local_queue);
		if (file_info)
		{
			new_id_info = (Pdivoom_file_id_info)divoom_system_malloc(sizeof(divoom_file_id_info));
			divoom_sd_file_list_manage_get_id_from_file_name(file_info->filename, &new_id_info->mode_id, &new_id_info->file_id);
			divoom_queue_push(file_list_handle->id_list_queue, new_id_info);
		}
	}

	divoom_queue_destroy(file_path_local_queue, NULL);
    file_list_handle->update_flag = 1;
	divoom_sd_file_list_manage_update_list(file_list_handle);
}

/*
  Function:       divoom_sd_file_list_manage_init
  Description:    获取文件列表
  Input:          file_list_path 文件列表路径
  Return:         返回相应的文件列表的句柄，用来区分是自定义列表还是其他列表
  Author:         wangft
  Data:           2020-12-31
*/
void *divoom_sd_file_list_manage_init(uint8 *file_list_path)
{
    uint32 check_sum = 0, raw_check_sum, read_len;
	uint16 id_num = 0, i = 0, retry_time = 0;;
	char file_list_name[64] = {0};
    Pdivoom_file_id_info id_list = NULL;
	Pdivoom_sd_file_list_info file_list_handle = divoom_system_malloc(sizeof(divoom_sd_file_list_info));

	memset(file_list_handle, 0, sizeof(divoom_sd_file_list_info));
	file_list_handle->id_list_queue = divoom_queue_init(40000, __func__, __LINE__);
    file_list_handle->update_flag = 0;
    file_list_handle->lock_handle = divoom_system_create_lock();
    retry_divoom_sd_file_list_manage_init_table:

	sprintf(file_list_name, "%s/file_list", file_list_path); //拼接文件列表名
	strcpy((char *)file_list_handle->file_list_path, file_list_name);

	if (!divoom_port_file_exists((char *)file_list_name)) //不存在
		file_list_handle->fp = divoom_port_file_open((char *)file_list_name, "wb");//新建一个文件，已存在的文件将内容清空，可读写
	else
		file_list_handle->fp = divoom_port_file_open((char *)file_list_name, "rb"); //打开一个文件，可读写，该文件必须存在
	if (file_list_handle->fp == NULL) 
	{
		divoom_print("[%s:%d] fopen file error\n", __func__, __LINE__);
        if(retry_time == 0)
        {
            divoom_print("[%s:%d] we will format system\n", __func__, __LINE__);
            goto retry_divoom_sd_file_list_manage_init_table;
        }
        retry_time ++;
		divoom_queue_destroy(file_list_handle->id_list_queue, NULL);
        divoom_system_destroy_lock(file_list_handle->lock_handle);
		divoom_free(file_list_handle);
		return NULL;
	}
	//先读两字节，获取id有多少个
	
	if(0 == divoom_port_file_read(&id_num, 1, sizeof(uint16), file_list_handle->fp))
	{
        goto divoom_sd_file_list_manage_init_err_table;
	}
    check_sum = divoom_app_com_get_check_sum((uint8 *)&id_num, sizeof(id_num));    
	divoom_print("id_num %d", id_num);
	if (id_num > 0 && id_num < 40000)
	{
		id_list = divoom_system_malloc(id_num * sizeof(divoom_file_id_info));
		if (id_list)
		{
	        read_len = divoom_port_file_read(id_list, 1, id_num * sizeof(divoom_file_id_info), file_list_handle->fp);
            if(read_len != id_num * sizeof(divoom_file_id_info))
            {
                divoom_print("file is error1\n");
                check_sum = 0;
                raw_check_sum = 1;
            }
            else
            {
                check_sum += divoom_app_com_get_check_sum((uint8 *)id_list, id_num * sizeof(divoom_file_id_info));    
    	        if(0 == divoom_port_file_read(&raw_check_sum, 1, sizeof(raw_check_sum), file_list_handle->fp))
                {
                    goto divoom_sd_file_list_manage_init_err_table;
                }
            }
			Pdivoom_file_id_info new_id_info = NULL;
            if(raw_check_sum != check_sum)
            {
                divoom_sd_file_list_manage_init_err_table:
                divoom_print("check sum is err; %d, %d\n",raw_check_sum, check_sum);
    			divoom_free(id_list); //用完释放
        		divoom_port_file_close(file_list_handle->fp); //先关闭
        		divoom_sd_file_list_manage_search_list(file_list_handle);
            }
            else
            {
    			for (i = 0; i < id_num; i++)
    			{
    				new_id_info = (Pdivoom_file_id_info)divoom_system_malloc(sizeof(divoom_file_id_info));
    				memcpy(new_id_info, &id_list[i], sizeof(divoom_file_id_info));
    				//divoom_print("%d	%d	%d	%s", new_id_info->mode_id, new_id_info->file_id, new_id_info->file_size, new_id_info->file_path);
    				divoom_queue_push(file_list_handle->id_list_queue, new_id_info);
    			}
    			file_list_handle->id_num = id_num;
    			divoom_free(id_list); //用完释放
    			divoom_port_file_close(file_list_handle->fp);
            }
		}
	}
	else 
	{
		//获取文件读取重建列表
		divoom_port_file_close(file_list_handle->fp); //先关闭
		divoom_sd_file_list_manage_search_list(file_list_handle);
	}
	return file_list_handle;
}

/*
  Function:       divoom_sd_file_list_manage_exit
  Description:    文件列表管理退出
  Input:          none
  Return:         none
  Author:         wangft
  Data:           2020-12-31
*/
void divoom_sd_file_list_manage_exit(void *handle)
{
	Pdivoom_sd_file_list_info file_list_handle = (Pdivoom_sd_file_list_info)handle;

	if (file_list_handle)
	{
		divoom_queue_destroy(file_list_handle->id_list_queue, NULL);
		if (file_list_handle->fp)
			divoom_port_file_close(file_list_handle->fp);
        divoom_system_destroy_lock(file_list_handle->lock_handle);
		divoom_free(file_list_handle);
	}
}

