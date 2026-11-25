#include "divoom.h"
#include "divoom_pic_check.h"


struct _divoom_pic_check_item_node;
typedef struct _divoom_pic_check_item_node *Pdivoom_pic_check_item_node;

//使用多叉树来进行管理,同一级目录从左结点到右结点，按文件名进行排序
typedef struct _divoom_pic_check_item_node
{
    divoom_pic_check_item_data file_info;   //文件信息
    Pdivoom_pic_check_item_node left;       //左结点
    Pdivoom_pic_check_item_node right;      //右结点
    Pdivoom_pic_check_item_node son;        //第一个子结点
    Pdivoom_pic_check_item_node parent;     //父结点
}divoom_pic_check_item_node, *Pdivoom_pic_check_item_node;

typedef struct _divoom_pic_check_ctrl
{
    int  total_items;  //总共有多少项
    Pdivoom_pic_check_item_node phead_node;     //头结点
}divoom_pic_check_ctrl, *Pdivoom_pic_check_ctrl;

char* divoom_pic_check_copy_string(char* file_name);
/*********************************************************************
  Function:        divoom_pic_check_init
  Description:     初始化模块信息
  Input:           none
  Return:          返回句柄
**********************************************************************/

void *divoom_pic_check_init(char *base_dir)
{
    Pdivoom_pic_check_ctrl handle;

    handle = divoom_system_malloc(sizeof(divoom_pic_check_ctrl));
    if(handle)
    {
        handle->total_items = 0;
        handle->phead_node = divoom_system_malloc(sizeof(divoom_pic_check_item_node));
        memset(handle->phead_node, 0, sizeof(divoom_pic_check_item_node));
        handle->phead_node->file_info.file_name = divoom_pic_check_copy_string(base_dir);
    }
    return handle;
}

/*********************************************************************
  Function:        divoom_pic_check_get_current_path
  Description:     从目录路径中分离出当前目录
  Input:           path      目录路径
                   cur_path  当前目录名
  Return:          如果是最后一级就返回空，否则返回下一级
**********************************************************************/

char *divoom_pic_check_get_current_path(char *path, char *cur_path)
{
    char *ret_path, *base_path = path;
    if(path[0] == '/')
    {
        path ++;
    }
    ret_path = strchr(path, '/');
    if(ret_path)
    {
        int len = (int)(ret_path - base_path);
        memcpy(cur_path, base_path, len);
        cur_path[len] = 0;
    }
    else
    {
        strcpy(cur_path, base_path);
    }
    return ret_path;
}

/*********************************************************************
  Function:        divoom_pic_check_copy_string
  Description:     拷贝一个字符串，使用完后要释放内存
  Input:           file_name      
  Return:          返回拷贝数据
**********************************************************************/

char *divoom_pic_check_copy_string(char *file_name)
{
    char* new_name;
    int len = strlen(file_name);

    new_name = divoom_system_malloc(len + 1);
    strcpy(new_name, file_name);
    return new_name;
}

/*********************************************************************
  Function:        divoom_pic_check_get_hash_value
  Description:     获取一个字符串的HASH值
  Input:           file_name      
  Return:          返回拷贝数据
**********************************************************************/

uint32 divoom_pic_check_get_hash_value(char *file_name)
{
    uint32 value = 0;
    
    while(file_name[0])
    {
        value += (uint8)file_name[0];
        file_name ++;
    }
    return value;
}

/*********************************************************************
  Function:        divoom_pic_check_insert_node
  Description:     在这一级树中，先查找是否有结点，如果没有结点，将这个结点插入
  Input:           pfirst_node
                   cur_path
  Return:          查找到的新结点
**********************************************************************/

Pdivoom_pic_check_item_node divoom_pic_check_insert_node(Pdivoom_pic_check_item_node pfirst_node, char *cur_path)
{
    int cmp_flag;
    Pdivoom_pic_check_item_node pfind_node = pfirst_node, pnew_node;
    

    while(pfind_node)
    {
        cmp_flag = strcmp(pfind_node->file_info.file_name, cur_path);
        if(cmp_flag == 0)
        {
            return pfind_node;
        }
        else if(cmp_flag > 0)
        {
            //将这个结点插入这个位置
            pnew_node = divoom_system_malloc(sizeof(divoom_pic_check_item_node));
            memset(pnew_node, 0, sizeof(divoom_pic_check_item_node));
            pnew_node->file_info.file_name = divoom_pic_check_copy_string(cur_path);
            pnew_node->parent = pfirst_node->parent;
            if(pfind_node->left == NULL)
            {
                //做为头结点
                pnew_node->right = pfind_node;
                pfind_node->left = pnew_node;
                pfind_node->parent->son = pnew_node;
            }
            else
            {
                //插入一个结点
                pnew_node->left = pfind_node->left;
                pnew_node->right = pfind_node;
                pfind_node->left->right = pnew_node;
                pfind_node->left = pnew_node;
            }
            return pnew_node;
        }
        if(pfind_node->right == NULL)
        {
            //在最后面插入一个结点
            pnew_node = divoom_system_malloc(sizeof(divoom_pic_check_item_node));
            memset(pnew_node, 0, sizeof(divoom_pic_check_item_node));
            pnew_node->file_info.file_name = divoom_pic_check_copy_string(cur_path);
            pnew_node->parent = pfirst_node->parent;
            pfind_node->right = pnew_node;
            pnew_node->left = pfind_node;
            return pnew_node;
        }
        pfind_node = pfind_node->right;
    }
    return NULL;
}


/*********************************************************************
  Function:        divoom_pic_check_add_src_path
  Description:     初始化，增加原始文件
  Input:           handle
                   path
  Return:          下一级的最左边的结点,如果是新结点就返回空
**********************************************************************/

void divoom_pic_check_add_src_path(void *handle, char *path,uint32 pic_id)
{
    char cur_path[256];
    Pdivoom_pic_check_item_node parent_node, first_left, cur_node;
    Pdivoom_pic_check_ctrl phandle = (Pdivoom_pic_check_ctrl)handle;

    parent_node = phandle->phead_node;
    first_left = phandle->phead_node;
    while(path)
    {
        path = divoom_pic_check_get_current_path(path, cur_path);
        if(strlen(cur_path))
        {
            //查找到
            if(first_left  == NULL)
            {
                first_left = divoom_system_malloc(sizeof(divoom_pic_check_item_node));
                memset(first_left, 0, sizeof(divoom_pic_check_item_node));
                first_left->file_info.file_name = divoom_pic_check_copy_string(cur_path);
                first_left->file_info.pic_id = pic_id;
                first_left->parent = parent_node;
                parent_node->son = first_left;
                parent_node = first_left;
                first_left = NULL;
            }
            else
            {
                cur_node = divoom_pic_check_insert_node(first_left, cur_path);
                parent_node = cur_node;
                first_left = cur_node->son;
            }
        }
    }
    phandle->total_items ++;
}

/*********************************************************************
  Function:        divoom_pic_check_find_node
  Description:     在这一级树中，查找是否有此结点
  Input:           pfirst_node
                   cur_path
  Return:          返回查找到的结点
**********************************************************************/

Pdivoom_pic_check_item_node divoom_pic_check_find_node(Pdivoom_pic_check_item_node pfirst_node, char *cur_path)
{
    int cmp_flag;
    Pdivoom_pic_check_item_node pfind_node = pfirst_node, pnew_node;

    while(pfind_node)
    {
        cmp_flag = strcmp(pfind_node->file_info.file_name, cur_path);
        if(cmp_flag == 0)
        {
            return pfind_node;
        }
        else if(cmp_flag > 0)
        {
            return NULL;
        }
        if(pfind_node->right == NULL)
        {
            return NULL;
        }
        pfind_node = pfind_node->right;
    }
    return NULL;
}

/*********************************************************************
  Function:        divoom_pic_check_check_file_exist
  Description:     检查文件是否存在，存在的话将结点的use_flag设置为1
  Input:           handle
                   path
  Return:          1：存在；0：未存在
**********************************************************************/

int divoom_pic_check_check_file_exist(void *handle, char *path)
{
    char cur_path[256];
    Pdivoom_pic_check_item_node parent_node, first_left, cur_node;
    Pdivoom_pic_check_ctrl phandle = (Pdivoom_pic_check_ctrl)handle;

    parent_node = phandle->phead_node;
    first_left = phandle->phead_node;
    while(path)
    {
        path = divoom_pic_check_get_current_path(path, cur_path);
        if(strlen(cur_path))
        {
            //查找到
            if(first_left  == NULL)
            {
                return 0;
            }
            else
            {
                cur_node = divoom_pic_check_find_node(first_left, cur_path);
                if(cur_node == NULL)
                {
                    return 0;
                }
                cur_node->file_info.use_flag = 1;   //标记已处理
                first_left = cur_node->son; //下一级子结点为第一个查找对象
            }
        }
    }
    return 1;
}
/*********************************************************************
  Function:        divoom_pic_check_find_no_use_node
  Description:     将所有use_flag=0的叶子结点全部压到队列中
  Input:           handle
                   path
  Return:          1：存在；0：未存在
**********************************************************************/

void  divoom_pic_check_find_no_use_node(Pdivoom_pic_check_item_node cur_node, char *base_path, void *del_queue)
{
    int len = strlen(base_path);
    
    if(cur_node->son == NULL)
    {
        //已经是叶子结点
        if(cur_node->file_info.use_flag == 0)
        {
            Pdivoom_pic_check_item_data pitem_info;
            
            pitem_info = divoom_system_malloc(sizeof(divoom_pic_check_item_data));            
            strcat(base_path, cur_node->file_info.file_name);
            pitem_info->file_name = divoom_pic_check_copy_string(base_path);
            pitem_info->pic_id = cur_node->file_info.pic_id;
            divoom_queue_push(del_queue, pitem_info);
        }
    }
    else
    {
        strcat(base_path, cur_node->file_info.file_name);
        divoom_pic_check_find_no_use_node(cur_node->son, base_path, del_queue);
    }
    //继续右结点
    if(cur_node->right)
    {
        base_path[len] = 0;
        divoom_pic_check_find_no_use_node(cur_node->right, base_path, del_queue);
    }
    
}

/*********************************************************************
  Function:        divoom_pic_check_get_next_del_file_path
  Description:     在检查完所有本地查找到的文件后，通过队列方式返回所有已经删除的文件
  Input:           handle
                   path
  Return:          1：存在；0：未存在
**********************************************************************/

void  *divoom_pic_check_get_next_del_file_path(void *handle)
{
    char cur_path[256];
    void *del_file_queue;
    Pdivoom_pic_check_ctrl phandle = (Pdivoom_pic_check_ctrl)handle;
    Pdivoom_pic_check_item_node cur_node;


    if(phandle == NULL)
    {
        return NULL;
    }
    del_file_queue = divoom_queue_init(phandle->total_items + 10, __func__, __LINE__);
    cur_path[0] = 0;
    divoom_pic_check_find_no_use_node(phandle->phead_node, cur_path, del_file_queue);
    return del_file_queue;
    
}

/*********************************************************************
  Function:        divoom_pic_check_find_no_use_node
  Description:     将所有use_flag=0的叶子结点全部压到队列中
  Input:           handle
                   path
  Return:          1：存在；0：未存在
**********************************************************************/

void  divoom_pic_check_free_node(Pdivoom_pic_check_item_node cur_node)
{
    //先删除自己，再依次删除右结点
    if(cur_node->right)
    {
        divoom_pic_check_free_node(cur_node->right);
    }
    if(cur_node->son)
    {
        divoom_pic_check_free_node(cur_node->son);
    }
    divoom_system_free(cur_node->file_info.file_name);
    divoom_system_free(cur_node);
    
}

/*********************************************************************
  Function:        divoom_pic_check_destory
  Description:     将释放所有资源
  Input:           handle
  Return:          none
**********************************************************************/

void divoom_pic_check_destory(void *handle)
{
    Pdivoom_pic_check_ctrl phandle = (Pdivoom_pic_check_ctrl)handle;

    divoom_pic_check_free_node(phandle->phead_node);
    divoom_system_free(phandle);
}


