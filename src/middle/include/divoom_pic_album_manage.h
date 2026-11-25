#ifndef _divoom_pic_album_manage_h_
#define _divoom_pic_album_manage_h_

#include "divoom_photo_album_josn.h"
enum{
    DIVOOM_PIC_ALBUM_TYPE_NORMAL = 0,   //普通相簿
    DIVOOM_PIC_ALBUM_TYPE_ALL = 1,      //所有照片
    DIVOOM_PIC_ALBUM_TYPE_LIKE = 2,     //点赞照片
    DIVOOM_PIC_ALBUM_TYPE_HIDE = 3,     //隐藏照片
    DIVOOM_PIC_ALBUM_TYPE_PRELOAD = 4,     //预装照片
};

typedef struct _divoom_pic_album_manage_head_item
{
    uint32 type;        //相簿类型
    uint32 pause_flag;  //是否只播放这张照片
    uint32 id;          //相册ID，这个不能重复使用，一直增加
    int   pic_id;       //上一次播放的ID
    int   width_pic_id;       //上一次播放的ID
    int   high_pic_id;       //上一次播放的ID
    char  album_name[128];  //相册名称，UTF8格式
}divoom_pic_album_manage_head_item, *Pdivoom_pic_album_manage_head_item;


typedef struct _divoom_pic_album_manage_head_data
{
    uint32 album_num;       //相册数目
    Pdivoom_pic_album_manage_head_item  palbum_list;  //相册名称，UTF8格式
}divoom_pic_album_manage_head_data, *Pdivoom_pic_album_manage_head_data;

#define DIVOOM_PIC_ALBUM_MANAGE_LIST_NUMBER 3   //照片流三列


typedef struct _divoom_pic_phone_manage_item_offset //这个文件按ID排序, 这个信息要缓冲实时使用
{
    uint8  pic_video_flag;  //0:照片；1：视频
    uint8  rotate_flag;     //旋转标记，0：正常，1：90；2：180；3：270
    uint8  mirror_flag;     //是否镜像
    uint8  fill_flag;      //填充标记
    uint8  hide_flag;       //0：显示； 1：隐藏
    uint8  like_flag;       //0：未收藏； 1：收藏
    uint8  width_flag;       //0：宽的； 1：长的
    uint32 id;              //文件ID
    double save_time;       //文件保存时间
    uint32 photo_time;      //拍摄时间
    uint32 like_time;       //点赞时间
    uint32 hide_time;      //隐藏时间
}divoom_pic_phone_manage_item_offset, *Pdivoom_pic_phone_manage_item_offset;

typedef struct _divoom_pic_album_manage_list_item
{
    int    read_dir;            //1:向下读； -1：向上读
    uint32 pic_num;            //相册包含多少个照片
    Pdivoom_pic_phone_manage_item_offset pic_list;          //照片ID列表,这个忽略隐藏的标记
}divoom_pic_album_manage_list_item, *Pdivoom_pic_album_manage_list_item;

typedef struct _divoom_pic_album_manage_album_item
{
    uint8  album_type;
    uint8    disp_order;
    uint8    disp_reverse;
    int    read_dir;            //1:向下读； -1：向上读
    uint32 album_id;           //相册ID
    uint32 cache_num;          //缓冲区大小
    uint32 pic_num;            //相册包含多少个照片
    Pdivoom_pic_phone_manage_item_offset pic_list;          //照片ID列表
}divoom_pic_album_manage_album_item, *Pdivoom_pic_album_manage_album_item;


/*********************************************************************
  Function:        divoom_pic_album_manage_get_album_id_list
  Description:     获取相簿id列表
  Input:           
  Return:          NONE
**********************************************************************/
int *divoom_pic_album_manage_get_album_id_list(int *album_num);

/*********************************************************************
  Function:        divoom_pic_album_manage_update_last_play_album_pic_id
  Description:     更新相簿最后播放的照片ID
  Input:           album_id
                   pic_id
  Return:          none
**********************************************************************/

void divoom_pic_album_manage_update_last_play_album_pic_id(int album_id, int pic_id);

/*********************************************************************
  Function:        divoom_pic_album_manage_set_one_play_album_pic_id
  Description:     更新相簿只播放的照片ID
  Input:           album_id
                   pic_id
  Return:          none
**********************************************************************/

void divoom_pic_album_manage_set_one_play_album_pic_id(int album_id, int pic_id);

/*********************************************************************
  Function:        divoom_pic_album_manage_update_last_play_album_stricker_pic_id
  Description:     更新相簿最后播放的照片ID
  Input:           album_id
                   width_pic_id
                   high_pic_id
  Return:          none
**********************************************************************/

void divoom_pic_album_manage_update_last_play_album_stricker_pic_id(int album_id, int width_pic_id, int high_pic_id);

/*********************************************************************
  Function:        divoom_pic_album_manage_get_last_play_album_pic_id
  Description:     得到这个相簿最后播放的照片ID
  Input:           album_id
  Return:          返回照片ID
**********************************************************************/

int divoom_pic_album_manage_get_last_play_album_pic_id(int album_id, int *pause_flag);
/*********************************************************************
  Function:        divoom_pic_album_manage_get_last_play_album_pic_id
  Description:     得到这个相簿最后播放的照片ID
  Input:           album_id
  Return:          返回照片ID
**********************************************************************/

void divoom_pic_album_manage_get_last_play_album_stricker_pic_id(int album_id, int *width_id, int *high_id);


typedef void DIVOOM_PIC_ALBUM_MANAGE_READ_OVER(void *para, uint8 *rgb_buf,int image_w, int image_h);

/*********************************************************************
  Function:        divoom_pic_album_manage_init
  Description:     初始化模块信息
  Input:           none
  Return:          返回相簿数目
**********************************************************************/

int divoom_pic_album_manage_init(void);

/*********************************************************************
  Function:        divoom_pic_album_manage_clear_all_type
  Description:     清除所有相册
  Input:           none
  Return:          NONE
**********************************************************************/

void divoom_pic_album_manage_clear_all_type(void);

/*********************************************************************
  Function:        divoom_pic_album_manage_get_album_id
  Description:     查找相册的ID
  Input:           album_name 相册名称
  Return:          相册ID
**********************************************************************/

int divoom_pic_album_manage_get_album_id(char *album_name);

/*********************************************************************
  Function:        divoom_pic_album_manage_set_album_cover
  Description:     设置相簿封面s
  Input:           album_id 
                   photo_id
  Return:          none
**********************************************************************/

void divoom_pic_album_manage_set_album_cover(int album_id, int photo_id, int flag, int app_flag);

/*********************************************************************
  Function:        divoom_pic_album_manage_check_album_cover
  Description:     检查相簿封面是否存在
  Input:           album_id 
  Return:          1 or 0
**********************************************************************/

int divoom_pic_album_manage_check_album_cover(int album_id);

/*********************************************************************
  Function:        divoom_pic_album_manage_get_album_id_from_type
  Description:     相簿类型得到第一个相簿ID
  Input:           album_type 相册类型
  Return:          相册ID
**********************************************************************/

int divoom_pic_album_manage_get_album_id_from_type(int album_type);

/*********************************************************************
  Function:        divoom_pic_album_manage_get_album_name
  Description:     得到相册的名称
  Input:           album_name 相册名称
  Return:          相册ID
**********************************************************************/

void divoom_pic_album_manage_get_album_name(int album_id,char *album_name);

/*********************************************************************
  Function:        divoom_pic_album_manage_get_static_album_name
  Description:     得到相册的名称
  Input:           album_id 
  Return:          相册名称
**********************************************************************/

char * divoom_pic_album_manage_get_static_album_name(int album_id);

/*********************************************************************
  Function:        divoom_pic_album_manage_add_type
  Description:     增加一个空相册
  Input:           album_name 相册名称
                   album_id 为0，是本地，否则是APP
  Return:          返回相簿ID,如果已经存在，将返回0
**********************************************************************/

int divoom_pic_album_manage_add_type(char *album_name, int album_id, int alarm_type);


/*********************************************************************
  Function:        divoom_pic_album_manage_del_type
  Description:     删除一个空相册
  Input:           album_name 相册名称
                   up_server  是否通知服务器
  Return:          NONE
**********************************************************************/

void divoom_pic_album_manage_del_type(int album_id, int up_server);

/*********************************************************************
  Function:        divoom_pic_album_manage_add_item
  Description:     向相册中添加一个相片
  Input:           album_name 相册名称
                   pic_id     相片ID
  Return:          NONE
**********************************************************************/

void divoom_pic_album_manage_add_pic_to_album(int album_id, uint32 pic_id);

/*********************************************************************
  Function:        divoom_pic_album_manage_del_item_from_name
  Description:     通过相册名称向相册中删除一个相片
  Input:           album_name 相册名称
                   pic_id     相片ID
  Return:          NONE
**********************************************************************/

void divoom_pic_album_manage_del_item_from_name(char *album_name, int pic_id);


/*********************************************************************
  Function:        divoom_pic_album_manage_del_item_from_id
  Description:     通过相册ID向相册中删除一个相片
  Input:           album_name 相册名称
                   pic_id     相片ID
  Return:          NONE
**********************************************************************/

void divoom_pic_album_manage_del_item_from_id(int album_id, int pic_id);

/*********************************************************************
  Function:        divoom_pic_album_manage_del_list_from_id
  Description:     通过相册ID向相册中删除一个相片
  Input:           album_id 相册ID
                   pic_list     相片ID
                   pic_num
  Return:          NONE
**********************************************************************/

void divoom_pic_album_manage_del_list_from_id(int album_id, uint32 *pic_list, int pic_num);

/*********************************************************************
  Function:        divoom_pic_album_manage_remove_list_from_id
  Description:     通过相册ID从相册中移除相片
  Input:           album_id 相册ID
                   pic_list     相片ID
                   pic_num
  Return:          NONE
**********************************************************************/

void divoom_pic_album_manage_remove_list_from_id(int album_id, uint32 *pic_list, int pic_num);

/*********************************************************************
  Function:        divoom_pic_album_manage_open_type
  Description:     打开相册用来读相册的数据
  Input:           album_name 相册名称
                   album_id   相簿ID,如果album_name=NULL的时候才有效
  Return:          返回句柄
**********************************************************************/

void *divoom_pic_album_manage_open_type(char *album_name, int album_id);

/*********************************************************************
  Function:        divoom_pic_album_manage_get_pic_offset
  Description:     得到某个偏移位置的信息
  Input:           handle
                   offset
  Return:          返回句柄
**********************************************************************/

Pdivoom_pic_phone_manage_item_offset divoom_pic_album_manage_get_pic_offset(void *handle, int offset);

/*********************************************************************
  Function:        divoom_pic_album_manage_remove_pic_from_album
  Description:     从相簿句柄中移除某个位置的相片信息，一般在隐藏与删除时调用
  Input:           handle
                   pic_id
  Return:          NONE
**********************************************************************/

void divoom_pic_album_manage_remove_pic_from_album(void *handle, int pic_id);

/*********************************************************************
  Function:        divoom_pic_album_manage_get_album_type
  Description:     得到相簿类型
  Input:           handle
  Return:          返回相簿类型
**********************************************************************/

int divoom_pic_album_manage_get_album_type(void *handle);

/*********************************************************************
  Function:        divoom_pic_album_manage_get_total_items
  Description:     获取相册中总共照片的数目
  Input:           phandle 相册名称
                   
  Return:          返回相片数目
**********************************************************************/

int divoom_pic_album_manage_get_total_items(void *phandle);

/*********************************************************************
  Function:        divoom_pic_album_manage_read_pic
  Description:     读相册图片，这个模块不能阻塞，做异步通知
  Input:           phandle 相册名称
                   pos
                   rgb_buf
                   pcallback
                   para
                   dir      1:向后读，-1向前读
                   
  Return:          如果读成功返回1，异步通知返回0
**********************************************************************/
int divoom_pic_album_manage_read_pic(void *phandle, int list_id, int pic_id, int asyn_flag, uint8 *rgb_buf, int image_w, int image_h, DIVOOM_PIC_ALBUM_MANAGE_READ_OVER *pcallback, void *para);

/*********************************************************************
  Function:        divoom_pic_album_manage_pre_read_cache
  Description:     预读相薄中的照片
  Input:           phandle 相册名称
                   list_id
                   rgb_buf
                   pic_id
                   
  Return:          none
**********************************************************************/
void divoom_pic_album_manage_pre_read_cache(void *phandle, int list_id, int pic_id);


/*********************************************************************
  Function:        divoom_pic_album_manage_clear_cache
  Description:     清除读CACHE
  Input:           phandle 相册名称
                   list_id
                   
  Return:          none
**********************************************************************/
void divoom_pic_album_manage_clear_cache(void *phandle, int list_id);

/*********************************************************************
  Function:        divoom_pic_album_manage_close_type
  Description:     关闭相册句柄，并释放相关资源
  Input:           phandle 相册名称
                   pos
                   rgb_buf
                   pcallback
                   para
                   dir
                   
  Return:          如果读成功返回1，异步通知返回0
**********************************************************************/

void divoom_pic_album_manage_close_type(void *phandle);
/*********************************************************************
  Function:        divoom_pic_album_manage_get_list_pic_info
  Description:     得到照片流第几列的信息
  Input:           phandle 相册名称
                   list_id, 非0就为三列照片流，否则就是全部
                   hide_flag  0:全部照片;1:只获取非隐藏的照片
                   
  Return:          返回照片流相片相关信息
**********************************************************************/

Pdivoom_pic_album_manage_list_item divoom_pic_album_manage_get_list_pic_info(void *phandle, int list_id, int hide_flag);

/*********************************************************************
  Function:        divoom_pic_album_manage_get_all_album_info
  Description:     得到所有相册信息
  Input:           none
  Return:          相册信息
**********************************************************************/

Pdivoom_pic_album_manage_head_data divoom_pic_album_manage_get_all_album_info(void);

/*********************************************************************
  Function:        divoom_pic_album_manage_create_disp_order_handle
  Description:     创建一个显示顺序管理的句柄
  Input:           pdisp_list 显示列表
                   disp_order      //显示顺序 
                   disp_reverse    //显示顺序反转标记 //显示顺序反转标记0;从新到旧，1：从旧到新
  Return:          返回句柄
**********************************************************************/

void *divoom_pic_album_manage_create_disp_order_handle(int album_id, uint8 disp_order, uint8 disp_reverse, int movie_flag);

/*********************************************************************
  Function:        divoom_pic_album_manage_create_disp_order_handle_from_start
  Description:     与 create_disp_order_handle 相同，但从第 0 张拉满列表，
                   不套用上次播放游标。给相册管理网格用；幻灯片续播仍走上面那个。
**********************************************************************/
void *divoom_pic_album_manage_create_disp_order_handle_from_start(int album_id, uint8 disp_order, uint8 disp_reverse, int movie_flag);
/*********************************************************************
  Function:        divoom_pic_album_manage_add_pic_id_from_disp_handle
  Description:     向列表中增加某个照片
  Input:           handle 显示列表
                   pic_id
  Return:          none
**********************************************************************/

void divoom_pic_album_manage_add_pic_id_from_disp_handle(void *handle, int pic_id);

/*********************************************************************
  Function:        divoom_pic_album_manage_remove_pic_id_from_disp_handle
  Description:     从列表中删除某个照片
  Input:           handle 显示列表
                   pic_id
  Return:          none
**********************************************************************/

void divoom_pic_album_manage_remove_pic_id_from_disp_handle(void *handle, int pic_id);

/*********************************************************************
  Function:        divoom_pic_album_manage_get_first_play_info
  Description:     获取第一个播放的信息
  Input:           handle         
                   pic_id
  
  Return:          位置
**********************************************************************/

int divoom_pic_album_manage_get_first_play_info(void *handle, int *pic_id);

/*********************************************************************
  Function:        divoom_pic_album_manage_remove_pic_id_from_disp_handle
  Description:     从列表中删除某个照片
  Input:           handle 显示列表
                   pic_id
  Return:          none
**********************************************************************/

void divoom_pic_album_manage_remove_pic_id_from_disp_handle(void *handle, int pic_id);

/*********************************************************************
  Function:        divoom_pic_album_manage_get_disp_order_pic_id
  Description:     从显示顺序管理的句柄里得到显示的位置的照片ID
  Input:           handle 显示列表
                   pos      //显示顺序
  Return:          文件信息
**********************************************************************/

int divoom_pic_album_manage_get_disp_order_pic_id(void *handle, int pos, Pdivoom_pic_phone_manage_item_offset pic_info);

/*********************************************************************
  Function:        divoom_pic_album_manage_get_disp_last_pic_id
  Description:     获取最后一张要播放的照片
  Input:           handle 显示列表
  Return:          最后照片的ID
**********************************************************************/

int divoom_pic_album_manage_get_disp_last_pic_id(void *handle);

/*********************************************************************
  Function:        divoom_pic_album_manage_get_disp_order_pic_id
  Description:     从显示顺序管理的句柄里得到显示的位置的照片ID
  Input:           handle 显示列表
                   pos      //显示顺序
  Return:          文件信息
**********************************************************************/

int divoom_pic_album_manage_get_pic_info_from_pic_id(void *handle, int pic_id, int *pic_pos, Pdivoom_pic_phone_manage_item_offset pic_info);

/*********************************************************************
  Function:        divoom_pic_album_manage_update_pic_attr
  Description:     变更相片的相关属性
  Input:           handle
                   pic_id
                   attr_flag
                   value
  Return:          none
**********************************************************************/

void divoom_pic_album_manage_update_pic_attr(void *handle, int pic_id,int attr_flag, int value);

/*********************************************************************
  Function:        divoom_pic_album_manage_destory_disp_order_handle
  Description:     释放句柄资源
  Input:           handle 显示列表
  Return:          none
**********************************************************************/

void divoom_pic_album_manage_destory_disp_order_handle(void *handle);

/*********************************************************************
  Function:        divoom_pic_album_manage_get_stricker_disp_order_pic_id
  Description:     从显示顺序管理的句柄里得到显示的位置的照片ID
  Input:           handle 显示列表
                   pos      //显示顺序
                   width_flag  1:宽的，0：长的
  Return:          文件信息
**********************************************************************/

int divoom_pic_album_manage_get_stricker_disp_order_pic_id(void *handle, int pos, int width_flag, Pdivoom_pic_phone_manage_item_offset pic_info);

/*********************************************************************
  Function:        divoom_pic_album_manage_get_disp_order_pic_num
  Description:     得到照片数目
  Input:           handle 显示列表
  Return:          文件信息
**********************************************************************/

int divoom_pic_album_manage_get_disp_order_pic_num(void *handle);



/*********************************************************************
  Function:        divoom_pic_album_manage_del_disp_order_pic_id
  Description:     删除某个位置的图片
  Input:           handle 显示列表
                   pos      //显示顺序
  Return:          文件信息
**********************************************************************/

void divoom_pic_album_manage_del_disp_order_pic_id(void *handle, int pic_id);

/*********************************************************************
  Function:        divoom_pic_album_manage_add_list_to_name
  Description:     向相册中添加多个相片
  Input:           album_id 相册名称
                   pic_list     相片ID
                   pic_num
  Return:          NONE
**********************************************************************/

void divoom_pic_album_manage_add_list_to_name(uint32 album_id, uint32 *pic_list, int pic_num);


/*********************************************************************
  Function:        divoom_pic_album_manage_get_last_album_pic_id
  Description:     得到最后一张照片ID
  Input:           album_id 相册名称
  Return:          NONE
**********************************************************************/

int divoom_pic_album_manage_get_last_album_pic_id(uint32 album_id);

/*********************************************************************
  Function:        divoom_pic_album_manage_add_list_to_name_no_refresh
  Description:     向相册中添加多个相片,但不更新相簿显示，应用在多张照片同时发送时
  Input:           album_id 相册名称
                   pic_list     相片ID
                   pic_num
  Return:          NONE
**********************************************************************/

void divoom_pic_album_manage_add_list_to_name_no_refresh(int album_id, uint32 *pic_list, int pic_num);
/* Preload initialization only: publish memory updates after an atomic DB batch. */
int divoom_pic_album_manage_add_preload_list(int album_id, uint32 *pic_list, int pic_num);

/*********************************************************************
  Function:        divoom_pic_album_manage_update_list
  Description:     从服务器新获取相簿列表
  Input:           palarm_info
  Return:          NONE
**********************************************************************/

void divoom_pic_album_manage_update_list(Pdivoom_photo_album_album_list  palarm_info);


/*********************************************************************
  Function:        divoom_pic_album_manage_get_album_local_pic_list
  Description:     APP获取相簿相片列表
  Input:           album_id
                   start
                   end
  Return:          NONE
**********************************************************************/

void divoom_pic_album_manage_get_album_local_pic_list(int album_id, int start, int end);

/*********************************************************************
  Function:        divoom_pic_album_manage_get_album_id
  Description:     查找相册的ID
  Input:           album_name 相册名称
  Return:          相册ID
**********************************************************************/

void divoom_pic_album_manage_update_album_name(int album_id, char *album_name);

Pdivoom_pic_album_manage_album_item divoom_pic_album_manage_load_album_item(int album_id);
/*********************************************************************
  Function:        divoom_pic_album_manage_check_album_id
  Description:     确认相册的ID，如果不存在返回全部照片
  Input:           album_id 
  Return:          相册ID
**********************************************************************/

int divoom_pic_album_manage_check_album_id(int album_id);

/*********************************************************************
  Function:        divoom_pic_album_manage_get_refresh_album_info
  Description:     获取相簿是否发生变化，并清除变化标记
  Input:           none
  Return:          1 or 0
**********************************************************************/

int divoom_pic_album_manage_get_refresh_album_info(void);

/*********************************************************************
  Function:        divoom_pic_album_manage_set_refresh_album_info
  Description:     设置相簿是否发生变化标记
  Input:           none
  Return:          none
**********************************************************************/

void divoom_pic_album_manage_set_refresh_album_info(void);

/*********************************************************************
  Function:        divoom_pic_album_manage_check_album_cover_exist
  Description:     得到相簿是否存在封面
  Input:           album_id 相册名称
  Return:          1 or 0
**********************************************************************/

int divoom_pic_album_manage_check_album_cover_exist(uint32 album_id);

/*********************************************************************
  Function:        divoom_pic_album_manage_clear_empty_album_covers
  Description:     张数为 0 的相册删除封面缓存 webp，下次有图会重新生成
  Input:           none
  Return:          none
**********************************************************************/
void divoom_pic_album_manage_clear_empty_album_covers(void);

/*********************************************************************
  Function:        divoom_pic_album_manage_get_album_disp_rgba_buf
  Description:     得到相簿
  Input:           album_id 相册名称
                   pic_list     相片ID
                   pic_num
  Return:          NONE
**********************************************************************/

int divoom_pic_album_manage_get_album_disp_rgba_buf(uint32 album_id, int width, int height, uint8 *rgb_buf);

typedef struct _divoom_pic_album_manage_delete_info_item
{
    int type;   //0:删除； 1：移除
    int album_id; 
    int pic_num;
    uint32 *pic_list;
}divoom_pic_album_manage_delete_info_item, *Pdivoom_pic_album_manage_delete_info_item;


/*********************************************************************
  Function:        divoom_pic_album_manage_del_list_from_id
  Description:     检查删除相片是否在当前播放的相簿中
  Input:           album_id 
                   pic_list     
                   pic_num
  Return:          NONE
**********************************************************************/

int divoom_pic_album_manage_check_delete_pic(int album_id, uint32 *pic_list, int pic_num);

/*********************************************************************
  Function:        divoom_pic_album_manage_check_remove
  Description:     检查移除的相片是否在当前播放的相簿中，如果在，将添加到移除队列
  Input:           album_id 
                   pic_list     
                   pic_num
  Return:          NONE
**********************************************************************/

int divoom_pic_album_manage_check_remove(int album_id, uint32 *pic_list, int pic_num);

/*********************************************************************
  Function:        divoom_pic_album_manage_get_remove_info
  Description:     得到移除队列相关信息
  Input:           none
  Return:          handle
**********************************************************************/

Pdivoom_pic_album_manage_delete_info_item divoom_pic_album_manage_get_remove_info(void);
/*********************************************************************
  Function:        divoom_pic_album_manage_do_remove_info
  Description:     处理移除队列动作
  Input:           none
  Return:          none
**********************************************************************/

void divoom_pic_album_manage_do_remove_info(Pdivoom_pic_album_manage_delete_info_item pitem_info);

#endif



