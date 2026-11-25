#include "divoom.h"
#include <dirent.h>

#define LEN (strlen(DIVOOM_USER_DATA_DIR) + 1)

/**
 * NAME               divoom_file_manage_from_index_get_path
 * *  @brief       	通过下标找带路径的文件
 * *  @param[in] 		sd_index 下标	 file_name 要查找的文件  file_path 存放路径文件名缓冲区
 * *	 @author         wangft               
 * *  @return        成功返回0 失败返回-1
 * */
int divoom_file_manage_from_index_get_path(int sd_index, char *file_name, char *file_path)
{
	FILE *fp;
	char value[256] = {0};
	char *p = NULL;
	int  i = 0, len = 0, ret = -1;
	
	if((fp = divoom_port_file_open(file_name,"r"))==NULL)
	{
		return -1;
	}
	while(feof(fp) == 0)
	{
		memset(value, 0, 256);
		if(NULL == fgets(value, 256, fp))
		{
            break;
		}
		p = value;
		if(p != NULL)
		{
			if(i == sd_index)
			{
				len = strlen(p);
				strcpy(file_path, p);
				*(file_path + len - 1) = '\0';
				ret = 0;
				break;
			}
			i++;
		}
	}
	return ret;
}

/*
  Function:       divoom_file_manage_get_item_num
  Description:	  查看列表有多少个条目
  Input:          NONE
  Return:         条目数
  Author:         wangft
  Data:           2020-12-18
*/
int divoom_file_manage_get_item_num(char *file_name)
{
	FILE *fp;
	char value[256] = {0};
	char *p = NULL;
	int  i = 0;
	
	if((fp = divoom_port_file_open(file_name,"r"))==NULL)
	{
		return -1;
	}
	while(feof(fp) == 0)
	{
		memset(value, 0, 256);
		if(NULL == fgets(value, 256, fp))
		{
            break;
		}
		p = value;
		if(p != NULL)
		{
			i++;
		}
	}
	return i;
}

/**
 * NAME               divoom_file_manage_from_file_get_path
 * *  @brief       	通过文件名找带路径的文件
 * *  @param[in] 		filename 文件名	file_name 要查找的文件  file_path 存放路径文件名缓冲区
 * *	 @author         wangft               
 * *  @return        成功返回0 失败返回-1
 * */
int divoom_file_manage_from_file_get_path(char *filename, char *file_name, char *file_path)
{
	FILE *fp;
	char value[256] = {0};
	char *p = NULL;
	int  len = 0, ret = -1;
	if (file_name == NULL || strlen(filename) == 0)
		return -1;
	if((fp = divoom_port_file_open(filename,"r"))==NULL)
	{
		return -1;
	}
	
	while(feof(fp) == 0)
	{
		memset(value, 0, 256);
		if(NULL == fgets(value, 256, fp))
		{
            break;
		}
		p = value;
		if(p != NULL)
		{
			if (strncmp(p + LEN, file_name, strlen(file_name)) == 0 )
			{
				if (file_path)
				{
					len = strlen(p);
					strcpy(file_path, p);
					*(file_path + len - 1) = '\0';
				}
				ret = 0;
				break;
			}
		}
	}	
	return ret;
}


uint32 divoom_file_manage_checksum(char *file_name)
{
	FILE *fp;
	uint32 check_sum = 0;
	int ch = 0;
	fp = divoom_port_file_open(file_name, "r");
	if (fp == NULL)
	{
		return 0;
	}

	ch = fgetc(fp); 
	while (ch != EOF)
	{
		check_sum += ch;
		ch = fgetc(fp);
	}

	divoom_port_file_close(fp);
	return check_sum;
}

void divoom_file_manage_get_file_var(char *flie_path, char *key_str, char *strbuf)
{
	FILE *fp = NULL;
	char buf[50] = {0};
	char *s = NULL, *p = NULL;
	
	fp = divoom_port_file_open(flie_path, "r");
	if (fp == NULL)
		return ;

	while(feof(fp) == 0)
	{
		memset(buf, 0, 50);
		if(NULL == fgets(buf, 50, fp))
		{
            break;
		}
		p = buf;
		if(p != NULL)
		{
			if ((s = strstr(p, key_str) )!= NULL)
			{
				p = s + strlen(key_str) + 1;
				strcpy(strbuf, p);
				break;
			}
		}
	}
	divoom_port_file_close(fp);
}


char *divoom_file_manage_get_file_text_str(char *file_path)
{
	FILE *fp = NULL;
	char *ar = NULL;
	int size;
	if(NULL == file_path)
		return NULL;
	fp = divoom_port_file_open(file_path, "r");
	if (fp == NULL)
		return NULL;

	divoom_port_file_seek(fp,0,SEEK_END);
	size = divoom_port_file_tell(fp);
	rewind(fp);
	if(size)
	{
		//申请一块能装下整个文件的空间 
		ar = (char *)divoom_system_malloc(sizeof(char)*size);
		//读文件，每次读一个，共读size次 
		if(ar)
		{
			if(size != divoom_port_file_read(ar,1,size,fp))
			{
			}
//			divoom_print("%s\n",ar);	
		}
	}
	divoom_port_file_close(fp);
	return ar;
}


void divoom_file_manage_set_file_var(char *conf_path, char *set_name, char *set_value)
{
	FILE *fp,*fp1;
	char buf[20][50] = {{0}};
	char temp[50] = {0};
	int k = 0, i = 0, result = 0;
	int len = strlen(set_name);

	fp = divoom_port_file_open(conf_path,"a+");
	if (fp)
	{
		while (feof(fp) == 0)
		{
			memset(temp, 0, sizeof(temp));
			if (fgets(temp, 50, fp) !=0)
			{
				if (strncmp(temp,set_name,len) == 0)
				{
					memset(temp, 0x0, sizeof(temp));
					sprintf(temp, "%s:%s\n", set_name, set_value);
					divoom_print("temp:%s", temp);
					result = 1;
				}				
				strcpy(buf[k], temp);			
				k++;
			}
		}
		divoom_port_file_close(fp);
	}
	if ((fp1 = divoom_port_file_open(conf_path, "w+"))==0)
		return ;	
	for(i = 0; i < k; i++)
	{
		fputs(buf[i],fp1); 
	}			
	if (0 == result)
	{
		fprintf(fp1, "%s:%s\n", set_name, set_value);
	}	
	divoom_port_file_close(fp1);
}

int divoom_file_manage_list_save(char *file_name, char *path_file_name, void *queue_handle)
{
    #if 0
	FILE *fp = NULL, *fpt = NULL;
	int items_num = 0, i = 0;
	Pdivoom_file_list_info file_info = NULL;	
		
	if (file_name == NULL && path_file_name == NULL)
		return -1;

	if (file_name != NULL)
	{
		fp = divoom_port_file_open(file_name, "w+");
		if (fp == NULL)
		{
			return -1;
		}
	}

	if (path_file_name != NULL)
	{
		fpt = divoom_port_file_open(path_file_name, "w+");
		if (fpt == NULL)
		{
			return -1;
		}
	}
	
	items_num = divoom_queue_items(queue_handle);
	for (i = 0; i < items_num; i++)
	{
		file_info = divoom_queue_get_index_item(queue_handle, i);
		if (fp)
			fprintf(fp, "%s\n", file_info->filename);
		if (fpt)
			fprintf(fpt, "%s\n", file_info->pathname);		
	}

	if (fp)
		divoom_port_file_close(fp);
	if (fpt)
		divoom_port_file_close(fpt);
    #endif
	return 0;
}

int divoom_file_manage_get_format(char *file_list_format, char file_format_buf[][FILE_FORMAT_LEN])
{	
	char *p = file_list_format;
	char *q = file_list_format;
	int i = 0;

	if (file_list_format == NULL)
		return -1;
	
	while ((p = strchr(p, '.')))
	{
		if ((q = strchr(p, ';')))
		{
			strncpy(file_format_buf[i], p, q-p);
			file_format_buf[i][strlen(file_format_buf[i])] = '\0';
			p++;
			i++;
		}
		else
		{
			strcpy(file_format_buf[i], p);
			file_format_buf[i][strlen(file_format_buf[i])] = '\0';
			i++;
			return i;
		}
	}
	return i;
}

 void divoom_file_manage_get_queue(char *path, char *find_flag, void *queue_handle, DIVOOM_FILE_LIST_CALLBACK pcallback)
{
	DIR *dir = NULL;
	const divoom_port_dirent_t *ptr = NULL;
//	struct stat statbuf; 
	char buf[FILE_FORMAT_NUM][FILE_FORMAT_LEN] = {{0}};
	char /*curpath[FILE_NAME_LEN + 1] = {0},*/ *file_list_format;
	int ret = 0, i = 0 /*, ret_data = 0*/;
	//uint32 file_size = 0;
	Pdivoom_file_list_info new_info = NULL;

    file_list_format = divoom_app_com_copy_string(find_flag, 0);
    divoom_com_str_to_low(file_list_format);
	dir = divoom_port_dir_open(path);
	//divoom_print("path：%s", path);
	if (dir == NULL)
	{
		divoom_print("[%s:%d] open dir error: %s", __func__, __LINE__, path);
        divoom_system_free(file_list_format);
		return ;
	}
	ret = divoom_file_manage_get_format(file_list_format, buf);//获取格式
	if (ret == -1 || ret == 0)
	{
		divoom_print("[%s:%d]the file_list_format is NULL\n",__FILE__,__LINE__);
		divoom_port_dir_close(dir);
        divoom_system_free(file_list_format);
		return ;
	}		 

	while ((ptr = divoom_port_dir_read(dir)) != NULL)
	{
//		divoom_print("path：%s", ptr->name);
//		ret_data = stat(ptr->name, &statbuf);
//		if (!ret_data && S_ISDIR(statbuf.st_mode))
//		{
//			if(0 == strcmp(".", ptr->name) || 0 == strcmp("..", ptr->name)) 
//			{ 
//				continue; 
//			} 
//			sprintf(curpath, "%s/%s", path, ptr->name);
//			divoom_file_manage_get_queue(curpath, file_list_format, queue_handle, pcallback);//递归调用
//		}
        if(strlen(ptr->name) >= FILE_FORMAT_LEN)
        {
            //文件名过长，将直接忽略这个文件
            continue;
        }
        divoom_com_str_to_low(ptr->name);
		for (i = 0; i < ret; i++)
		{
			if (strstr(ptr->name, buf[i])) 
			{	 
				new_info = (Pdivoom_file_list_info)divoom_malloc(sizeof(divoom_file_list_info), DIVOOM_MODEL_FILE, __LINE__, __FILE__);
				strcpy(new_info->filename, ptr->name);		

				//divoom_print("path: %s file_name: %s", new_info->pathname, new_info->filename);
				//暂时没有使用文件大小，
				//if (pcallback)
				//	file_size = pcallback(new_info->pathname);
				new_info->file_size = 1024;
				divoom_queue_push(queue_handle, (void *)new_info);  //push进队列
				break;
			}
		}
	}

	divoom_port_dir_close(dir);
    divoom_system_free(file_list_format);
	return ;
}
  
 void *divoom_file_manage_get_list(char *path, char *file_list_format, DIVOOM_FILE_LIST_CALLBACK pcallback)
 {
	void *queue_handle = divoom_queue_init(10000, __func__, __LINE__);	 

	divoom_file_manage_get_queue(path, file_list_format, queue_handle, pcallback);
	if (!divoom_queue_not_empty(queue_handle))
	{
		divoom_free(queue_handle);
		return NULL;
	}		 
	return queue_handle;
 }


