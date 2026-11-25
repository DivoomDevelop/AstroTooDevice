#ifndef _DIVOOM_FILE_MANAGE
#define _DIVOOM_FILE_MANAGE

#define FILE_NAME_LEN 256
#define FILE_FORMAT_NUM 20
#define FILE_FORMAT_LEN 20

typedef struct _divoom_file_list_info
{
	char filename[FILE_FORMAT_LEN];
	uint32 time;
	uint32 file_size;
}divoom_file_list_info, *Pdivoom_file_list_info;

typedef uint32 (*DIVOOM_FILE_LIST_CALLBACK)(char *file_name);

/**
 * NAME               divoom_file_manage_from_index_get_path
 * *  @brief       	通过下标找带路径的文件
 * *  @param[in] 		sd_index 下标	 file_name 要查找的文件  file_path 存放路径文件名缓冲区
 * *	 @author         wangft               
 * *  @return        成功返回0 失败返回-1
 * */
 
int divoom_file_manage_from_index_get_path(int sd_index, char *file_name, char *file_path);

/*
  Function:       divoom_file_manage_get_item_num
  Description:	  查看列表有多少个条目
  Input:          NONE
  Return:         条目数
  Author:         wangft
  Data:           2020-12-18
*/
int divoom_file_manage_get_item_num(char *file_name);

/**
 * NAME               divoom_file_manage_from_file_get_path
 * *  @brief       	通过文件名找带路径的文件
 * *  @param[in] 		filename 文件名	file_name 要查找的文件  file_path 存放路径文件名缓冲区
 * *	 @author         wangft               
 * *  @return        成功返回0 失败返回-1
 * */
 
int divoom_file_manage_from_file_get_path(char *filename, char *file_name, char *file_path);

/**
 * NAME     divoom_file_manage_checksum
 * @BRIEF		??????checksum
 * @PARAM		file_name ???
 * AUTHOR	 wangft
 * @RETURN	checksum???
 * @RETVAL	
 */

uint32 divoom_file_manage_checksum(char *file_name);

/**
 * NAME     divoom_file_manage_get_file_var
 * @BRIEF		????????
 * @PARAM		flie_path ??? key_str ?????? strbuf ????
 * AUTHOR	 wangft
 * @RETURN	none
 * @RETVAL	
 */

void divoom_file_manage_get_file_var(char *flie_path, char *key_str, char *strbuf);

/**
 * NAME     divoom_file_manage_set_file_var
 * @BRIEF		????????
 * @PARAM		conf_path ??? set_name ?????? set_value ?????
 * AUTHOR	 wangft
 * @RETURN	none
 * @RETVAL	
 */

void divoom_file_manage_set_file_var(char *conf_path, char *set_name, char *set_value);



/**
 * NAME     divoom_file_manage_list_save
 * @BRIEF	??????
 * @PARAM	file_name ?????? path_file_name ?????? queue_handle ????
 * AUTHOR	 wangft
 * @RETURN	 -1 ?? 0 ??
 * @RETVAL	
 */
int divoom_file_manage_list_save(char *file_name, char *path_file_name, void *queue_handle);

/**
 * NAME         divoom_file_manage_get_format
 * @BRIEF	??????
 * @PARAM	file_list_format: ???????????  file_format_buf:???????????
 * AUTHOR	 wangft
 * @RETURN	?????????
 * @RETVAL	
 */

int divoom_file_manage_get_format(char *file_list_format, char file_format_buf[][FILE_FORMAT_LEN]);

/**
  * NAME	 divoom_file_manage_get_queue
  * @BRIEF	 ???????????
  * @PARAM	 path: ????  file_list_format: ??????? queue_handle: ??
  * AUTHOR	 wangft
  * @RETURN  NONE
  * @RETVAL  
  */

 void divoom_file_manage_get_queue(char *path, char *file_list_format, void *queue_handle, DIVOOM_FILE_LIST_CALLBACK pcallback);


/**
  * NAME	 divoom_file_manage_get_list
  * @BRIEF	 ???????????
  * @PARAM	 path: ????  file_list_format: ???????
  * AUTHOR	 wangft
  * @RETURN  queue_handle ??
  * @RETVAL  
  */
  
 void *divoom_file_manage_get_list(char *path, char *file_list_format, DIVOOM_FILE_LIST_CALLBACK pcallback);


/*****************************************************************************
 Prototype    : divoom_file_manage_get_file_text_str
 Description  : 获取文件所有字符
 Input        : char *file_path
 Output       : None
 Return Value : char *		返回字符串，需要手动free
 
  History        :
  1.Date         : 2021/2/22
    Author       : coronaa
    Modification : Created function
*****************************************************************************/
char *divoom_file_manage_get_file_text_str(char *file_path);

#endif

