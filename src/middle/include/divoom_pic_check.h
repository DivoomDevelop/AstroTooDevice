#ifndef _divoom_pic_check_h_
#define _divoom_pic_check_h_

typedef struct _divoom_pic_check_item_data
{
    uint32 use_flag;    //0：表示未被标记，1：标记已经存在，2:标记新增加
    uint32 pic_id;
    char *file_name;
}divoom_pic_check_item_data, *Pdivoom_pic_check_item_data;

/*********************************************************************
  Function:        divoom_pic_check_init
  Description:     初始化模块信息
  Input:           none
  Return:          返回句柄
**********************************************************************/

void *divoom_pic_check_init(char *base_dir);

/*********************************************************************
  Function:        divoom_pic_check_add_src_path
  Description:     初始化，增加原始文件
  Input:           handle
                   path
  Return:          下一级的最左边的结点,如果是新结点就返回空
**********************************************************************/

void divoom_pic_check_add_src_path(void *handle, char *path,uint32 pic_id);
/*********************************************************************
  Function:        divoom_pic_check_check_file_exist
  Description:     检查文件是否存在，存在的话将结点的use_flag设置为1
  Input:           handle
                   path
  Return:          1：存在；0：未存在
**********************************************************************/

int divoom_pic_check_check_file_exist(void *handle, char *path);
/*********************************************************************
  Function:        divoom_pic_check_get_next_del_file_path
  Description:     在检查完所有本地查找到的文件后，通过队列方式返回所有已经删除的文件
  Input:           handle
                   path
  Return:          1：存在；0：未存在
**********************************************************************/

void  *divoom_pic_check_get_next_del_file_path(void *handle);

/*********************************************************************
  Function:        divoom_pic_check_destory
  Description:     将释放所有资源
  Input:           handle
  Return:          none
**********************************************************************/

void divoom_pic_check_destory(void* handle);

#endif


