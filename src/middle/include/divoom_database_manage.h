#ifndef _divoom_database_manage_h_

#define _divoom_database_manage_h_

#define DIVOOM_SUBSCRIBE_CLEAN_BATCH_SIZE 20
/* Returns files removed, or -1 on failure; deleted_num also reports partial removal. */
int divoom_database_manage_clear_unused_subscribe_batch(
    int mode, uint32 *deleted_ids, int *deleted_num);

int divoom_database_manage_init(char *db_name);
int divoom_database_manage_fresh_db(void);
void divoom_database_manage_add_table_index(char *table_name, char *item_name);
void divoom_database_manage_run_sql_command(char *cmd_name);
int divoom_database_manage_add_new_table(char *table_name, char *table_item);
int divoom_database_manage_insert_new_item(char *table_name, char *item_data);
void divoom_database_manage_update_record(char *table_name, char *item_data);
void divoom_database_manage_remove_record(char *table_name, int id);
void divoom_database_manage_remove_table(char *table_name);
void divoom_database_manage_close(char *db_name);

/*********************************************************************
  Function:        divoom_database_manage_check_count_err
  Description:     检查符合这个命令的记录是否存在，
  Input:           cmd_info
                   
  Return:          存在返回1， 否则返回0
**********************************************************************/

int divoom_database_manage_check_count_err(char *cmd_info);

typedef void DIVOOM_DATABASE_MANAGE_GET_DATA_CALLBACK(void * para, int row, int col, char *data);
int divoom_database_manage_get_record(char *table_name, char *item_data,int id, DIVOOM_DATABASE_MANAGE_GET_DATA_CALLBACK *pcallback, void *para);
int divoom_database_manage_reset(char *db_name);

/*********************************************************************
  Function:        divoom_database_manage_check_column_in_table
  Description:     检查是否包含某个字段，如果没有包括，将增加
  Input:           table_name
                   column_name
                   new_column_definition
  Return:          
**********************************************************************/

int divoom_database_manage_check_column_in_table(char *table_name, char *column_name, char *new_column_definition);

/*********************************************************************
  Function:        divoom_database_manage_insert_new_item
  Description:     向数据库中插入一个相簿
  Input:           id
                   type
                   album_name
  Return:          none
**********************************************************************/

void divoom_database_manage_insert_album_item(int  id, int type, char* album_name);

/*********************************************************************
  Function:        divoom_database_manage_update_album_record
  Description:     更新相簿的名称相簿
  Input:           id
                   album_name
  Return:          none
**********************************************************************/

void divoom_database_manage_update_album_name_record(int  id, char *album_name);

/*********************************************************************
  Function:        divoom_database_manage_delete_album_record
  Description:     删除相簿
  Input:           id
  Return:          none
**********************************************************************/

void divoom_database_manage_delete_album_table(int  id);

/*********************************************************************
  Function:        divoom_database_manage_remove_album_record
  Description:     将相片从相簿中移除
  Input:           album_id
                   pic_id
  Return:          none
**********************************************************************/

void divoom_database_manage_remove_album_record(int  album_id, int pic_id);

/*********************************************************************
  Function:        divoom_database_manage_remove_album_record
  Description:     将相片从相簿中移除
  Input:           album_id
                   pic_id
  Return:          none
**********************************************************************/

int divoom_database_manage_add_album_record(int  album_id, int pic_id);
/* Returns 1 after one atomic commit (existing pairs are skipped), 0 on failure. */
int divoom_database_manage_add_album_records_batch(int album_id,
    const uint32 *pic_list, int pic_num);

/*********************************************************************
  Function:        divoom_database_manage_get_all_pic_from_album_table
  Description:     从相簿中得到所有照片，并按相关排序
  Input:           album_id
                   pic_num   相片数目
                   order_flag   1:按拍摄时间， 按照片时间
                   desc_flag    1:升序与降序
                   like_flag    1: 只取收藏的相片列表
                   hide_flag    1:只取隐藏的相片列表
  Return:          照片ID列表
**********************************************************************/

uint32 *divoom_database_manage_get_all_pic_from_album_table(int  album_id, int *pic_num, int order_flag, int desc_flag, int like_flag, int hide_flag);

/*********************************************************************
  Function:        divoom_database_manage_get_pic_num_from_album_table
  Description:     从相簿中得到所有照片张数
  Input:           album_id
                   hide_flag    1:只取不隐藏的相片；0：所有照片
  Return:          相片张数
**********************************************************************/

int divoom_database_manage_get_pic_num_from_album_table(int  album_id, int hide_flag);

/*********************************************************************
  Function:        divoom_database_manage_get_all_album_info
  Description:     得到所有的相簿相关信息
  Input:           album_id
                   pic_num   相片数目
                   order_flag   1:按拍摄时间， 按照片时间
                   desc_flag    1:升序与降序
  Return:          none
**********************************************************************/

Pdivoom_pic_album_manage_head_item divoom_database_manage_get_all_album_info(int *album_num);

/*********************************************************************
  Function:        divoom_database_manage_get_pic_info
  Description:     得到照片相关信息
  Input:           pic_id
                   pic_info
  Return:          none
**********************************************************************/

int divoom_database_manage_get_pic_info(int pic_id, void * pic_info);

/*********************************************************************
  Function:        divoom_database_manage_get_pic_path_name
  Description:     得到照片路径信息
  Input:           pic_id
                   path_name
  Return:          result
**********************************************************************/

int divoom_database_manage_get_pic_path_name(int pic_id, char *path_name);

/*********************************************************************
  Function:        divoom_database_manage_get_pic_id_from_path_name
  Description:     得到照片ID
  Input:           
                   path_name
  Return:          pic_id
**********************************************************************/

int divoom_database_manage_get_pic_id_from_path_name(char *path_name);

/*********************************************************************
  Function:        divoom_database_manage_add_pic_record
  Description:     增加一张照片信息
  Input:           pic_info
  Return:          返回照片ID
**********************************************************************/

int divoom_database_manage_add_pic_record(double save_time, int photo_time, char  *path_name, int photo_x, int photo_y, int user_id,char *pic_path, int video_flag, int width_flag);


/*********************************************************************
  Function:        divoom_database_manage_get_pic_title
  Description:     得到照片标题
  Input:           pic_id
                   pic_title
  Return:          result
**********************************************************************/

int divoom_database_manage_get_pic_title(int pic_id, char *pic_title);

/*********************************************************************
  Function:        divoom_database_manage_update_pic_title
  Description:     修改照片标题
  Input:           pic_info
  Return:          返回照片ID
**********************************************************************/

void divoom_database_manage_update_pic_title(int pic_id, char *pic_title);

enum{
    DIVOOM_PIC_PHONE_ATTR_ROTATE_FLAG,
    DIVOOM_PIC_PHONE_ATTR_MIRROR_FLAG,
    DIVOOM_PIC_PHONE_ATTR_FILL_FLAG,
    DIVOOM_PIC_PHONE_ATTR_HIDE_FLAG,
    DIVOOM_PIC_PHONE_ATTR_LIKE_FLAG,
};

/*********************************************************************
  Function:        divoom_database_manage_update_pic_attr
  Description:     修改照片属性
  Input:           pic_info
  Return:          返回照片ID
**********************************************************************/

void divoom_database_manage_update_pic_attr(Pdivoom_pic_phone_manage_item_offset pic_info, int attr_flag);

/*********************************************************************
  Function:        divoom_database_manage_delete_pic
  Description:     删除照片
  Input:           pic_id
  Return:          none
**********************************************************************/

void divoom_database_manage_delete_pic(int pic_id);
/*********************************************************************
  Function:        divoom_database_manage_reset_pic_info
  Description:     照片数据全部丢失，将删除两个表的内容
  Input:           none
  Return:          none
**********************************************************************/

void divoom_database_manage_reset_pic_info(void);

/*********************************************************************
  Function:        divoom_database_manage_get_album_pic_list
  Description:     从相簿中得到指定数量未隐藏的相片
  Input:           album_id
                   start
                   pic_num   相片数目
  Return:          照片列表ID
**********************************************************************/

uint32 *divoom_database_manage_get_album_pic_list(int  album_id, int start,int *pic_num);


/*********************************************************************
  Function:        divoom_database_manage_insert_music_item
  Description:     插入一首歌曲信息
  Input:           item_data  文件路径
                   root_name  根目录名
  Return:          返回插入的ID
**********************************************************************/

int divoom_database_manage_insert_music_item(char *item_data, const char *root_name);

/*********************************************************************
  Function:        divoom_database_manage_delete_music_item
  Description:     删除某首歌曲
  Input:           id
  Return:          none
**********************************************************************/

void divoom_database_manage_delete_music_item(int id);


/*********************************************************************
  Function:        divoom_database_manage_get_music_list
  Description:     获取所有歌曲
  Input:           pcallback
                   para
  Return:          none
**********************************************************************/

typedef void divoom_database_manage_get_music_list_callback(void *handle, int id, int like, char *file_name, char *root_name);
void divoom_database_manage_get_music_list(divoom_database_manage_get_music_list_callback *pcallback, void *para);
int divoom_database_manage_clear_local_music_reset_id(void);
int divoom_database_manage_get_local_music_count(void);
void divoom_database_manage_txn_begin(void);
void divoom_database_manage_txn_end(void);
void divoom_database_manage_local_music_batch_begin(void);
void divoom_database_manage_local_music_batch_end(void);


/*********************************************************************
  Function:        divoom_database_manage_update_music_like
  Description:     更新某首歌曲的收藏标志
  Input:           id
                   like_flag
  Return:          none
**********************************************************************/

void divoom_database_manage_update_music_like(int id, int like_flag);



/*********************************************************************
  Function:        divoom_database_manage_get_user_define_last_update_time
  Description:     获取自定义最后更新的时间
  Input:           none
  Return:          最后更新时间
**********************************************************************/

int divoom_database_manage_get_user_define_last_update_time(void);


/*********************************************************************
  Function:        divoom_database_manage_get_user_define_pic_list
  Description:     获取某个自定义的所有照片ID
  Input:           clock_id
                   clock_pos
                   parent_id
                   pic_num   相片数目
  Return:          照片列表ID
**********************************************************************/

Pdivoom_image_hot_item divoom_database_manage_get_user_define_pic_list(
    int clock_id, int clock_pos, int parent_id, int *pic_num,
    char **pvoice_list);

/*********************************************************************
  Function:        divoom_database_manage_get_user_define_pic_addr
  Description:     获取某个自定义的照片地址
  Input:           id
                   file_addr
  Return:          1 or 0
**********************************************************************/

int divoom_database_manage_get_user_define_pic_addr(int  id, char *file_addr);

/*********************************************************************
  Function:        divoom_database_manage_update_user_define_pic_addr
  Description:     更新一个资源地址
  Input:           clock_id
                   clock_pos
                   parent_id
                   id
                   file_addr
                   update_time
  Return:          1 or 0
**********************************************************************/

void divoom_database_manage_update_user_define_pic_addr(
    int clock_id, int clock_pos, int parent_id, int id, char *file_addr,
    int update_time, int author_id, char *image_name, char *voice_id);

/*********************************************************************
  Function:        divoom_database_manage_delete_user_define_item
  Description:     删除某个资源
  Input:           clock_id
                   clock_pos
                   parent_id
                   id
                   update_time
  Return:          1 or 0
**********************************************************************/

void divoom_database_manage_delete_user_define_item(int  clock_id, int clock_pos, int parent_id, int  id, int update_time);

/*********************************************************************
  Function:        divoom_database_manage_insert_user_define_pic_item
  Description:     插入某个资源
  Input:           clock_id
                   clock_pos
                   parent_id
                   id
                   file_addr
                   update_time
  Return:          none
**********************************************************************/

void divoom_database_manage_insert_user_define_pic_item(
    int clock_id, int clock_pos, int parent_id, int id, char *file_addr,
    int update_time, int author_id, char *image_name, char *voice_id);


/*********************************************************************
  Function:        divoom_database_manage_empty_user_define_item
  Description:     删除某个表盘资源
  Input:           clock_id
                   clock_pos
                   parent_id
                   update_time
  Return:          1 or 0
**********************************************************************/

void divoom_database_manage_empty_user_define_item(int  clock_id, int clock_pos, int parent_id, int update_time);

/*********************************************************************
  Function:        divoom_database_manage_update_user_define_play_time
  Description:     更新自定义播放时间
  Input:           clock_id
                   clock_pos
                   parent_id
                   play_time
                   show_time
  Return:          none
**********************************************************************/

void divoom_database_manage_update_user_define_play_time(int clock_id, int clock_pos, int parent_id, int play_time, int show_time);


/*********************************************************************
  Function:        divoom_database_manage_get_user_define_play_time
  Description:     获取自定义播放时间
  Input:           clock_id
                   clock_pos
                   parent_id
  Return:          1 or 0
**********************************************************************/

int divoom_database_manage_get_user_define_play_time(int clock_id, int clock_pos, int parent_id, int *show_time);

/*********************************************************************
  Function:        divoom_database_manage_update_user_define_play_time
  Description:     更新自定义播放时间
  Input:           clock_id
                   clock_pos
                   parent_id
                   play_time
  Return:          none
**********************************************************************/

void divoom_database_manage_update_user_define_update_time(int  clock_id, int clock_pos, int parent_id, int update_time);


/*********************************************************************
  Function:        divoom_database_manage_get_subscribe_image_last_update_time
  Description:     获取某个类型订阅最后更新的时间
  Input:           none
  Return:          最后更新时间
**********************************************************************/

int divoom_database_manage_get_subscribe_image_last_update_time(char *table_name, int image_type);

/*********************************************************************
  Function:        divoom_database_manage_get_subscribe_image_pic_list
  Description:     获取某个专辑的所有照片ID
  Input:           clock_id
                   clock_pos
                   parent_id
                   pic_num   相片数目
  Return:          照片列表ID
**********************************************************************/

Pdivoom_image_hot_item divoom_database_manage_get_subscribe_image_pic_list(
    char *table_name, int image_type, int *pic_num);
/*********************************************************************
  Function:        divoom_database_manage_get_subscribe_image_pic_list_by_updatetime
  Description:     获取某个专辑的所有照片ID
  Input:           clock_id
                   clock_pos
                   parent_id
                   pic_num   相片数目
  Return:          照片列表ID
**********************************************************************/

Pdivoom_image_hot_item
divoom_database_manage_get_subscribe_image_pic_list_by_updatetime(
    char *table_name, int image_type, int *pic_num);

/*********************************************************************
  Function:        divoom_database_manage_delete_subscribe_image_item
  Description:     删除专辑的某个资源
  Input:           album_id
                   image_id
  Return:          1 or 0
**********************************************************************/

/* image_type < 0 时，按 image_id 一次删除所有类型的记录。 */
void divoom_database_manage_delete_subscribe_image_item(char *table_name, int  image_type, int  image_id);
/*********************************************************************
  Function:        divoom_database_manage_delete_subscribe_image_all
  Description:     删除专辑的一批资源
  Input:           image_type
  Return:          none
**********************************************************************/

void divoom_database_manage_delete_subscribe_image_all(char *table_name, int  image_type);


/*********************************************************************
  Function:        divoom_database_manage_delete_subscribe_image_all
  Description:     删除某个专辑的所有资源
  Input:           album_id
  Return:          1 or 0
**********************************************************************/

void divoom_database_manage_delete_subscribe_image_all(char *table_name, int  image_type);


/*********************************************************************
  Function:        divoom_database_manage_add_subscribe_image_item
  Description:     增加某个资源
  Input:           album_id
                   image_id
                   update_time
                   file_addr
  Return:          1 or 0
**********************************************************************/

void divoom_database_manage_add_subscribe_image_item(char *table_name, int  image_type, int  image_id, int update_time, char *file_addr, int user_id, char *image_name);

/* 仅 HOT 图片写入时持久化 image_size。 */
void divoom_database_manage_add_subscribe_image_item_with_size(
    char *table_name, int image_type, int image_size, int image_id,
    int update_time, char *file_addr, int user_id, char *image_name);

/* 已有 HOT 记录按 image_id 回写尺寸，下次开机才能按这个档位装进队列。 */
void divoom_database_manage_update_subscribe_image_size(char *table_name, int image_id, int image_type, int image_size);

/*********************************************************************
  Function:        divoom_database_manage_update_subscribe_image_name_by_user_id
  Description:     按作者 USERID 批量更新订阅图片记录中的作者名称
  Input:           table_name
                   user_id
                   image_name
  Return:          none
**********************************************************************/

void divoom_database_manage_update_subscribe_image_name_by_user_id(
    char *table_name, int user_id, char *image_name);


/*********************************************************************
  Function:        divoom_database_manage_get_subscribe_image_pic_addr
  Description:     获取某个专辑的照片地址
  Input:           image_id
                   file_addr
  Return:          1 or 0
**********************************************************************/

int divoom_database_manage_get_subscribe_image_pic_addr(char *table_name, int image_id, char *file_addr);



/*********************************************************************
  Function:        divoom_database_manage_get_divoom_image_image_type_list
  Description:     得到所有的image_type列表
  Input:           table_name
                   list_num
  Return:          list
**********************************************************************/

int *divoom_database_manage_get_divoom_image_image_type_list(char *table_name, int *list_num);


/*********************************************************************
  Function:        divoom_database_manage_check_divoom_image_config_exist
  Description:     检查这个配置项，
  Input:           image_id
                   file_addr
  Return:          1 or 0
**********************************************************************/

int divoom_database_manage_check_divoom_image_config_exist(char *table_name, int clock_id, int clock_pos, int parent_id, int image_type);

/*********************************************************************
  Function:        divoom_database_manage_check_divoom_image_config_exist
  Description:     检查这个配置项，
  Input:           image_id
                   file_addr
  Return:          1 or 0
**********************************************************************/

int divoom_database_manage_check_divoom_image_type_exist(char *table_name, int image_type);

/*********************************************************************
  Function:        divoom_database_manage_check_divoom_image_config_exist
  Description:     检查这个配置项，
  Input:           image_id
                   file_addr
  Return:          1 or 0
**********************************************************************/

int divoom_database_manage_add_divoom_image_config_item(char *table_name, int clock_id, int clock_pos, int parent_id, int image_type);


/*********************************************************************
  Function:        divoom_database_manage_update_divoom_image_config_item
  Description:     更新自定义播放时间
  Input:           clock_id
                   clock_pos
                   parent_id
                   image_type
  Return:          none
**********************************************************************/

int divoom_database_manage_update_divoom_image_config_item(char *table_name, int clock_id, int clock_pos, int parent_id, int image_type);

/*********************************************************************
  Function:        divoom_database_manage_delete_divoom_image_config_item
  Description:     删除某个配置项
  Input:           table_name
                   clock_id
                   clock_pos
                   parent_id
                   image_type
  Return:          1 or 0
**********************************************************************/

void divoom_database_manage_delete_divoom_image_config_item(char *table_name, int clock_id, int clock_pos, int parent_id, int image_type);

/*********************************************************************
  Function:        divoom_database_manage_clear_all_divoom_image
  Description:     清空某个配置项
  Input:           table_name
  Return:          1 or 0
**********************************************************************/

void divoom_database_manage_clear_all_divoom_image(char *table_name);


/*********************************************************************
  Function:        divoom_database_manage_update_subscribe_config_record
  Description:     更新记录，不存在就插入
  Input:           table_name
                   clock_id
                   clock_pos
                   parent_id
                   pinfo
  Return:          1 or 0
**********************************************************************/

int divoom_database_manage_update_subscribe_config_record(char *table_name, int clock_id, int clock_pos, int parent_id, Pdivoom_disp_subscribe_data pinfo);
/*********************************************************************
  Function:        divoom_database_manage_update_subscribe_config_record
  Description:     更新记录，不存在就插入
  Input:           table_name
                   clock_id
                   clock_pos
                   parent_id
                   play_time
                   show_time
  Return:          1 or 0
**********************************************************************/

int divoom_database_manage_update_subscribe_play_time_record(char *table_name, int clock_id, int clock_pos, int parent_id, int play_time, int show_time);

/*********************************************************************
  Function:        divoom_database_manage_get_subscribe_config_record
  Description:     得到记录
  Input:           table_name
                   clock_id
                   clock_pos
                   parent_id
                   pinfo
  Return:          1 or 0
**********************************************************************/

int divoom_database_manage_get_subscribe_config_record(char *table_name, int clock_id, int clock_pos, int parent_id, Pdivoom_disp_subscribe_data pinfo);

/*********************************************************************
  Function:        divoom_database_manage_get_subscribe_config_update_time
  Description:     得到记录
  Input:           table_name
  Return:          update time
**********************************************************************/

int divoom_database_manage_get_subscribe_config_update_time(char *table_name);

/*********************************************************************
  Function:        divoom_database_manage_get_subscribe_config_update_time
  Description:     得到记录
  Input:           table_name
                   id
  Return:          update time
**********************************************************************/

int divoom_database_manage_get_item_update_time(char *table_name, int id);


/*********************************************************************
  Function:        divoom_database_manage_get_eq_image_list
  Description:     添加一个元素
  Input:           table_name
                   clock_id
                   clock_pos
                   parent_id
                   list_num
  Return:          list
**********************************************************************/

void *divoom_database_manage_get_eq_image_list(char *table_name,int clock_id, int clock_pos, int parent_id, int *list_num);

/*********************************************************************
  Function:        divoom_database_manage_add_eq_image_item
  Description:     添加一个元素
  Input:           table_name
                   clock_id
                   clock_pos
                   parent_id
                   update_time
                   file_id
  Return:          1 or 0
**********************************************************************/

int divoom_database_manage_add_eq_image_item(char *table_name,int clock_id, int clock_pos, int parent_id, int update_time, char *file_id, int user_id, char *image_name);

/*********************************************************************
  Function:        divoom_database_manage_get_eq_image_addr
  Description:     得到某个资源地址
  Input:           table_name
                   image_id
                   file_addr
  Return:          1 or 0
**********************************************************************/

void divoom_database_manage_get_eq_image_addr(char *table_name,int  image_id, char *file_addr);


/*********************************************************************
  Function:        divoom_database_manage_delete_eq_image_item
  Description:     删除某个资源
  Input:           album_id
                   image_id
  Return:          1 or 0
**********************************************************************/

void divoom_database_manage_delete_eq_image_item(char *table_name,int  image_id);

/*********************************************************************
  Function:        divoom_database_manage_get_eq_mod_index
  Description:     获取EQ的选择项
  Input:           table_name
                   clock_id
                   clock_pos
                   parent_id
                   
  Return:          mode
**********************************************************************/

int divoom_database_manage_get_eq_mode_index(char *table_name,int clock_id, int clock_pos, int parent_id);
/*********************************************************************
  Function:        divoom_database_manage_update_eq_mode_index
  Description:     更新EQ的选择项
  Input:           table_name
                   clock_id
                   clock_pos
                   parent_id
                   mode
  Return:          none
**********************************************************************/

void divoom_database_manage_update_eq_mode_index(char *table_name,int clock_id, int clock_pos, int parent_id, int mode);
/*********************************************************************
  Function:        divoom_database_manage_set_subscribe_config_update_time
  Description:     得到记录
  Input:           table_name
  Return:          update time
**********************************************************************/

void divoom_database_manage_set_subscribe_config_update_time(char *table_name, int id, int update_time);



/*********************************************************************
  Function:        divoom_database_manage_get_user_head_list
  Description:     得到最近访问的用户头像的列表
  Input:           user_num  返回得到用户的数目
  Return:          用户列表
**********************************************************************/

uint32 *divoom_database_manage_get_user_head_list(int *user_num);


/*********************************************************************
  Function:        divoom_database_manage_insert_user_head_item
  Description:     向数据库中插入用户信息
  Input:           user_id
                   user_name
                   file_id
  Return:          none
**********************************************************************/

void divoom_database_manage_insert_user_head_item(int user_id, char *user_name, char *file_id);

/*********************************************************************
  Function:        divoom_database_manage_update_user_head_item
  Description:     更新用户的相关信息
  Input:           user_id
                   user_name
                   file_id
  Return:          none
**********************************************************************/

void divoom_database_manage_update_user_head_item(int user_id, char *user_name, char *file_id);


/*********************************************************************
  Function:        divoom_database_manage_update_user_head_time
  Description:     更新用户头像使用时间
  Input:           user_id
  Return:          none
**********************************************************************/

void divoom_database_manage_update_user_head_time(int user_id);
/*********************************************************************
  Function:        divoom_database_manage_get_user_head_item
  Description:     得到用户头像的信息
  Input:           user_id
                   user_name
                   file_id
  Return:          用户列表
**********************************************************************/

int divoom_database_manage_get_user_head_item(int user_id, char *user_name, char *file_id);

/*********************************************************************
  Function:        divoom_database_manage_check_album_include_pic
  Description:     检查相簿是否包含这个相片
  Input:           pic_id
                   album_id   
  Return:          相簿列表
**********************************************************************/

int divoom_database_manage_check_album_include_pic(int album_id,int pic_id);


typedef struct _divoom_pic_id_name_info
{
    int id;
    double time;
    char name[128];
}divoom_pic_id_name_info, *Pdivoom_pic_id_name_info;
/*********************************************************************
  Function:        divoom_database_manage_get_all_pic_from_album_table
  Description:     从相簿中得到所有照片，并按相关排序
  Input:           album_id
                   pic_num   相片数目
                   order_flag   1:按拍摄时间， 按照片时间
                   desc_flag    1:升序与降序
                   like_flag    1: 只取收藏的相片列表
                   hide_flag    1:只取隐藏的相片列表
  Return:          none
**********************************************************************/

Pdivoom_pic_id_name_info divoom_database_manage_get_all_pic_id_and_name(int  album_id, int *pic_num, int num, int order_flag, int desc_flag, double start_time, int start_id);

/*********************************************************************
  Function:        divoom_database_manage_get_first_pic_id
  Description:     从相簿中得到最后一张照片
  Input:           
                   
  Return:          id
**********************************************************************/

int divoom_database_manage_get_first_pic_id(int album_id);
/*********************************************************************
  Function:        divoom_database_manage_get_total_pic_num
  Description:     照片总数
  Input:           album_id
                   
  Return:          照片总数
**********************************************************************/

/* Zero-based display position; ID lookup returns 0, position lookup -1 on failure. */
int divoom_database_manage_get_disp_order_pic_id(int album_id, int order_flag, int reverse_flag, int movie_flag, int pos);
int divoom_database_manage_get_disp_order_pic_pos(int album_id, int order_flag, int reverse_flag, int movie_flag, int pic_id);

int divoom_database_manage_get_total_pic_num(int album_id, int order_flag, int reverse_flag, double save_time, int pic_flag, int movie_flag);

/*********************************************************************
  Function:        divoom_database_manage_get_all_pic_id
  Description:     从相簿中得到所有照片，并按相关排序
  Input:           album_id
                   pic_num   相片数目
  Return:          列表
**********************************************************************/

uint32 *divoom_database_manage_get_all_pic_id(int  album_id, int *pic_num, int order_flag, int desc_flag, double start_time, int pic_flag, int movie_flag);

/*********************************************************************
  Function:        divoom_database_manage_get_max_pic_id
  Description:     得到相片中最大ID的相片ID
  Input:           
                   
  Return:          id
**********************************************************************/

int divoom_database_manage_get_max_pic_id(void);

/*********************************************************************
  Function:        divoom_database_manage_get_all_pic_id_reverse
  Description:     从相簿中得到所有照片，并按相关排序
  Input:           album_id
                   pic_num   相片数目
  Return:          列表
**********************************************************************/

uint32 *divoom_database_manage_get_all_pic_id_reverse(int  album_id, int *pic_num, int order_flag, int desc_flag, double start_time, int movie_flag);

typedef struct _divoom_pic_id_width_info
{
    int width_flag;
    int id;
}divoom_pic_id_width_info, *Pdivoom_pic_id_width_info;

/*********************************************************************
  Function:        divoom_database_manage_get_all_pic_from_album_table
  Description:     从相簿中得到所有照片，并按相关排序
  Input:           album_id
                   pic_num   相片数目
  Return:          列表
**********************************************************************/

Pdivoom_pic_id_width_info divoom_database_manage_get_all_pic_id_width(int  album_id, int *pic_num, int order_flag, int desc_flag, int pic_flag);


/*********************************************************************
  Function:        divoom_database_manage_insert_custom_photo_item
  Description:     插入用户自定义相簿
  Input:           clock_info
                   save_time
                   photo_time
                   file_name
  Return:          返回插入的ID
**********************************************************************/

int divoom_database_manage_insert_custom_photo_item(Pdivoom_clock_id_info clock_info, double save_time, int photo_time, char *file_name);

/*********************************************************************
  Function:        divoom_database_manage_delete_custom_photo_item
  Description:     删除某个记录
  Input:           id
  Return:          none
**********************************************************************/

void divoom_database_manage_delete_custom_photo_item(int id);


/*********************************************************************
  Function:        divoom_database_manage_delete_custom_photo_all
  Description:     清除整个记录
  Input:           clock_info
  Return:          none
**********************************************************************/

void divoom_database_manage_delete_custom_photo_all(Pdivoom_clock_id_info clock_info);

/*********************************************************************
  Function:        divoom_database_manage_get_custom_photo_list
  Description:     获取所有这个表盘的记录，返回ID
  Input:           clock_info
                   pic_num
                   order_flag
                   order_reverse
  Return:          pic list
**********************************************************************/


uint32 *divoom_database_manage_get_custom_photo_list(Pdivoom_clock_id_info clock_info, int *pic_num, int order_flag, int order_reverse, double start_time);
/*********************************************************************
  Function:        divoom_database_manage_get_custom_photo_list_reverse
  Description:     到序获取前面一些记录
  Input:           clock_info
                   pic_num
                   order_flag
                   order_reverse
  Return:          none
**********************************************************************/


uint32 *divoom_database_manage_get_custom_photo_list_reverse(Pdivoom_clock_id_info clock_info, int *pic_num, int order_flag, int order_reverse, double start_time);

/*********************************************************************
  Function:        divoom_database_manage_get_custom_local_photo_list
  Description:     获取所有这个表盘的记录，返回ID
  Input:           clock_info
                   pic_num
                   start_time
  Return:          none
**********************************************************************/


Pdivoom_pic_id_name_info divoom_database_manage_get_custom_local_photo_list(Pdivoom_clock_id_info clock_info, int *pic_num,double start_time, int order_flag, int order_reverse);

/*********************************************************************
  Function:        divoom_database_manage_get_total_custom_pic_num
  Description:     照片总数
  Input:           clock_info
                   
  Return:          照片总数
**********************************************************************/

int divoom_database_manage_get_total_custom_pic_num(Pdivoom_clock_id_info clock_info, int order_flag, int reverse_flag, double save_time);


/*********************************************************************
  Function:        divoom_database_manage_get_custom_photo_list
  Description:     获取所有这个表盘的记录，返回ID
  Input:           id
                   file_name
  Return:          true or false
**********************************************************************/


int divoom_database_manage_get_custom_photo_name(int id, char *file_name);

typedef struct _divoom_custom_photo_info
{
    double save_time;
    int photo_time;
}divoom_custom_photo_info, *Pdivoom_custom_photo_info;


/*********************************************************************
  Function:        divoom_database_manage_get_custom_photo_info
  Description:     获取所有这个表盘的记录，返回ID
  Input:           clock_info
                   pic_info
  Return:          none
**********************************************************************/


int divoom_database_manage_get_custom_photo_info(int id, divoom_custom_photo_info *pic_info);


/*********************************************************************
  Function:        divoom_database_manage_reset_calendar_info
  Description:     移除所有信息
  Input:           none
  Return:          none
**********************************************************************/

void divoom_database_manage_reset_calendar_info(void);

/*********************************************************************
  Function:        divoom_database_manage_load_calendar_list_info
  Description:     加载所有日历信息
  Input:           none
  Return:          none
**********************************************************************/

void divoom_database_manage_load_calendar_list_info(void *event_queue);

/*********************************************************************
  Function:        divoom_database_manage_update_calendar_item
  Description:     更新一条日历记录，没有的话就插入
  Input:           start_utc
  Return:          none
**********************************************************************/

void divoom_database_manage_update_calendar_item(void *data);

/*********************************************************************
  Function:        divoom_database_manage_insert_calendar_item
  Description:     插入一个日历记录
  Input:           start_utc
  Return:          none
**********************************************************************/

int divoom_database_manage_insert_calendar_item(void *data);

/*********************************************************************
  Function:        divoom_database_manage_remove_calendar_item
  Description:     删除某个日历
  Input:           start_utc
  Return:          none
**********************************************************************/

void divoom_database_manage_remove_calendar_item(int calendar_id);


/*********************************************************************
  Function:        divoom_database_manage_update_event_item
  Description:     更新一条事件记录，没有的话就插入
  Input:           start_utc
  Return:          none
**********************************************************************/

void divoom_database_manage_update_event_item(int calendar_id, void *data);

/*********************************************************************
  Function:        divoom_database_manage_insert_calendar_item
  Description:     插入一个日历记录
  Input:           start_utc
  Return:          none
**********************************************************************/

int divoom_database_manage_insert_event_item(int calendar_id, void *data);

/*********************************************************************
  Function:        divoom_database_manage_remove_event_item
  Description:     删除事件信息
  Input:           calendar_id
                   id
  Return:          none
**********************************************************************/

void divoom_database_manage_remove_event_item(int calendar_id, char *id);

/*********************************************************************
  Function:        divoom_database_manage_remove_time_out_event
  Description:     删除少于start_utc的事件信息
  Input:           start_utc
  Return:          none
**********************************************************************/

void divoom_database_manage_remove_time_out_event(int start_utc);


/*********************************************************************
  Function:        divoom_database_manage_load_calendar_event_info
  Description:     加载指定日历的在时间范围内的事件
  Input:           none
  Return:          none
**********************************************************************/

void divoom_database_manage_load_calendar_event_info(int calendar_id, void *event_queue, int start_utc, int end_utc);

/*********************************************************************
  Function:        divoom_database_manage_load_calendar_all_event_info
  Description:     加载所有日历的在时间范围内的事件
  Input:           none
  Return:          none
**********************************************************************/

void divoom_database_manage_load_calendar_all_event_info(void **event_queue, int start_utc, int end_utc);


/*********************************************************************
  Function:        divoom_database_manage_insert_vision_photo_item
  Description:     插入用户自定义相簿
  Input:           clock_info
                   save_time
                   photo_time
                   file_name
  Return:          返回插入的ID
**********************************************************************/

int divoom_database_manage_insert_vision_photo_item(char *table_name, int vision_id, char *file_name);


/*********************************************************************
  Function:        divoom_database_manage_insert_vision_photo_item_with_time
  Description:     插入用户自定义相簿
  Input:           clock_info
                   save_time
                   photo_time
                   file_name
  Return:          返回插入的ID
**********************************************************************/

int divoom_database_manage_insert_vision_photo_item_with_time(char *table_name,int vision_id, char *file_name, int update_time);

/*********************************************************************
  Function:        divoom_database_manage_delete_custom_photo_item
  Description:     删除某个记录
  Input:           id
  Return:          none
**********************************************************************/

void divoom_database_manage_delete_vision_photo_item(char *table_name, int id);

/*********************************************************************
  Function:        divoom_database_manage_update_vision_photo_item_download
  Description:     更新下载标记
  Input:           id
  Return:          none
**********************************************************************/

void divoom_database_manage_update_vision_photo_item_download(char *table_name,int id);


typedef struct _divoom_vision_item
{
    int download_flag;
    int vision_id;
}divoom_vision_item, *Pdivoom_vision_item;

/*********************************************************************
  Function:        divoom_database_manage_get_vision_photo_list
  Description:     获取所有记录，返回ID
  Input:           pic_num
  Return:          list
**********************************************************************/


Pdivoom_vision_item divoom_database_manage_get_vision_photo_list(char *table_name, int *pic_num);


/*********************************************************************
  Function:        divoom_database_manage_get_vision_photo_name
  Description:     获取记录的ＯＳＳ_ＮＡＭＥ
  Input:           oss_name
  Return:          ０　ＯＲ　１
**********************************************************************/


int divoom_database_manage_get_vision_photo_name(char *table_name,int vision,char *oss_name);


/*********************************************************************
  Function:        divoom_database_manage_get_vision_last_update_time
  Description:     获取记录的最大更新时间
  Input:           none
  Return:          update_time
**********************************************************************/


int divoom_database_manage_get_vision_last_update_time(char *table_name);


/*********************************************************************
  Function:        divoom_database_manage_update_divoom_image_config_update_time
  Description:     更新自定义播放时间
  Input:           clock_id
                   clock_pos
                   parent_id
                   image_type
  Return:          none
**********************************************************************/

int divoom_database_manage_update_divoom_image_config_update_time(char *table_name, int image_type,int update_time);
/*********************************************************************
  Function:        divoom_database_manage_get_divoom_image_update_tiem
  Description:     得到最后更新时间，
  Input:           image_id
                   image_type
  Return:          update_time
**********************************************************************/

int divoom_database_manage_get_divoom_image_update_tiem(char *table_name, int image_type);
/*********************************************************************
  Function:        divoom_database_manage_get_divoom_image_type_id
  Description:     得到image_type，
  Input:           image_id
                   file_addr
  Return:          1 or 0
**********************************************************************/

int divoom_database_manage_get_divoom_image_type_id(char *table_name, int clock_id, int clock_pos, int parent_id);

#define DIVOOM_TF_NAME_MAX 128
#define DIVOOM_TF_PATH_MAX 256

typedef struct _divoom_tf_entry {
	int is_dir;
	int id;
	char name[DIVOOM_TF_NAME_MAX];
} divoom_tf_entry;

typedef struct _divoom_tf_folder_info {
	int id;
	int parent_id;
	int dir_cnt;
	int trk_cnt;
	int gord_lo;
	int gord_hi;
	int depth;
	int ord;
	char name[DIVOOM_TF_NAME_MAX];
	char path[DIVOOM_TF_PATH_MAX];
} divoom_tf_folder_info;

int divoom_database_tf_is_rebuilding(void);
int divoom_database_tf_dir_total(int folder_id);
int divoom_database_tf_dir_page(int folder_id, int offset, int limit, divoom_tf_entry *out, int out_cap);
int divoom_database_tf_subtree_count(int folder_id);
int divoom_database_tf_subtree_page(int folder_id, int offset, int limit, divoom_tf_entry *out, int out_cap);
int divoom_database_tf_folder_info(int folder_id, divoom_tf_folder_info *out);
int divoom_database_tf_folder_parent(int folder_id);
int divoom_database_tf_track_path(int track_id, char *buf, int buf_size);
int divoom_database_tf_track_name(int track_id, char *buf, int buf_size);
int divoom_database_tf_track_row(int folder_id, int track_id);
int divoom_database_tf_next_id(int cur_id);
int divoom_database_tf_prev_id(int cur_id);
int divoom_database_tf_random_id(int seed_id);
int divoom_database_tf_min_id(void);
int divoom_database_tf_max_id(void);
int divoom_database_tf_find_id_by_path(const char *music_path);
int divoom_database_tf_set_like(int track_id, int like_flag);
int divoom_database_tf_total(void);
int divoom_database_tf_like_count(void);
int divoom_database_tf_meta_get_u32(const char *key, uint32 *out);
int divoom_database_tf_meta_set_u32(const char *key, uint32 value);
int divoom_database_tf_rebuild_begin(void);
int divoom_database_tf_ensure_folder(int parent_id, const char *name, const char *path, int depth, int *out_id);
int divoom_database_tf_add_track(int track_id, int folder_id, const char *name);
int divoom_database_tf_rebuild_end(void);

/* LAN 只读相簿/媒体 JSON 快照；返回值由调用方释放，失败返回 NULL。 */
divoom_cJSON *divoom_database_manage_get_lan_album_list(void);
divoom_cJSON *divoom_database_manage_get_lan_photo_list(int album_id, int start, int end);

#endif

