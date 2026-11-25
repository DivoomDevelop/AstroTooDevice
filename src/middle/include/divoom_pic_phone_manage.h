#ifndef  _divoom_pic_phone_manage_h_
#define _divoom_pic_phone_manage_h_
#include "divoom_type.h"

#define divoom_PIC_PHONE_manage_BASE_PATH  DIVOOM_SD_APP_PIC_DIR

enum{
    DIVOOM_PIC_PHOTO_SLIDE_DISP_SAVE_TIME,  //保存时间
    DIVOOM_PIC_PHOTO_SLIDE_DISP_PHTOT_TIME, //拍摄时间
    DIVOOM_PIC_PHOTO_SLIDE_DISP_RANDOM,     //随机
    DIVOOM_PIC_PHOTO_SLIDE_DISP_NORMAL,     //正常
    DIVOOM_PIC_PHOTO_SLIDE_DISP_STRICKER,     //墙贴方式
};
typedef struct _divoom_pic_phone_manage_slide_config //幻灯片的配置
{
    int    cur_play_effect_type;     //当前幻灯片播放效果
    uint8  effect_type;     //幻灯片播放效果
    uint8  sub_effect_type;     //子表盘幻灯片播放效果
    uint8  bg_type;         //背景模式
    uint8  fill_flag;       //是否填充像框标记,0：横填充；1：纵填充
    uint8  skip_fill_flag;  //是否忽略单张照片填充标记， 
    uint8  disp_order;      //显示顺序1:
    uint8  disp_reverse;    //显示顺序反转标记0;从新到旧，1：从旧到新
    uint8  clock_flag;      //显示时钟标记
    uint8  weather_flag;    //显示天气标记
    uint8  title_flag;      //显示标题标记
    uint8  video_auto_play; //视频自动播放标记
    uint8  video_play_flag;    //视频播放设置；0：循环播放;1：只播放一次；;2：只播放一次,并继续；
    uint8  video_auto_mute; //视频播放十分钟后自动静音标记
    uint8  video_volume;    //视频播放音量
    uint32 play_time;       //显示速度
    int    album_id;
    int    album_update_time;
}divoom_pic_phone_manage_slide_config, *Pdivoom_pic_phone_manage_slide_config;




/*********************************************************************
  Function:        divoom_pic_phone_manage_update_album_to_pic
  Description:     更新图片的相册关系
  Input:           pic_id       图片ID
                   album_id     相册ID
                   flag         1:增加到相册； 0：从相册中移除
  Return:          none
**********************************************************************/

void divoom_pic_phone_manage_update_album_to_pic(uint32 pic_id, uint32 album_id, int flag);

/*********************************************************************
  Function:        divoom_pic_phone_manage_init
  Description:     初始化图片管理模块，应用程序启动时加载，将所有文件偏移信息加载到内存中
  Input:           base_dir
                   first_flag   是否是第一次，如果第一次就要查找文件
  Return:          none
**********************************************************************/

void divoom_pic_phone_manage_init(char *base_dir, int first_flag);

/*********************************************************************
  Function:        divoom_pic_phone_manage_cache_delete_pic
  Description:     延时删除文件，放在后台处理
  Input:           file_name
  Return:          none
**********************************************************************/

void divoom_pic_phone_manage_cache_delete_pic(char *file_name);

/*********************************************************************
  Function:        divoom_pic_phone_manage_add_all_preload
  Description:     初始化预装相簿数据
  Input:           album_id
  Return:          none
**********************************************************************/

void divoom_pic_phone_manage_add_all_preload(int album_id);
/*********************************************************************
  Function:        divoom_pic_phone_manage_convert_video_volume
  Description:     转换视频音量，目前最大音量要做限制
  Input:           voluem
  Return:          转换后的音量
**********************************************************************/

int divoom_pic_phone_manage_convert_video_volume(int volume);

/*********************************************************************
  Function:        divoom_pic_phone_manage_create_picture_thumb
  Description:     创建缩略图文件，并输出缩略图缓冲区
  Input:           pic_path
                   thumb_path
                   dest_w
                   dest_h
                   rgb_buf
  Return:          none
**********************************************************************/

void divoom_pic_phone_manage_create_picture_thumb(char *pic_path, int *dest_w, int *dest_h, uint8 *rgb_buf);

/*********************************************************************
  Function:        divoom_pic_phone_manage_add_picture
  Description:     增加一个图片，这个一般由APP传文件进行触发
  Input:           pic_path
                   
                   file_time
                   photo_time
                   photo_x
                   photo_y
                   album_id
                   pic_title
  Return:          返回照片ID
**********************************************************************/

int divoom_pic_phone_manage_add_picture(char *pic_path, double file_time, uint32 photo_time, int photo_x, int photo_y, int album_id, int user_id,char *pic_title, int pic_w, int pic_h, int create_thumb_flag);


/*********************************************************************
  Function:        divoom_pic_phone_manage_del_picture
  Description:     删除一个图片，这个是设备照片管理手动删除照片
  Input:           pic_id
  Return:          none
**********************************************************************/

void divoom_pic_phone_manage_del_picture(int pic_id);

/*********************************************************************
  Function:        divoom_pic_phone_manage_del_picture
  Description:     删除一个图片，这个是设备照片管理手动删除照片
  Input:           pic_id
  Return:          none
**********************************************************************/

void divoom_pic_phone_manage_del_err_pic_name(char *file_name);

/*********************************************************************
  Function:        divoom_pic_phone_manage_recreate_list
  Description:     设备在连接U盘后退出或本地文件损坏时再重建照片列表时调用
  Input:           reset_all  1:是否清除相册信息
  Return:          1:成功；0：失败
**********************************************************************/

int divoom_pic_phone_manage_recreate_list(int reset_all);

/*********************************************************************
  Function:        divoom_pic_phone_manage_get_picture_name
  Description:     通过文件ID，得到文件名
  Input:           pic_path
                   pic_id
  Return:          none
**********************************************************************/

void divoom_pic_phone_manage_get_picture_name(char *pic_path, int pic_id);

/*********************************************************************
  Function:        divoom_pic_phone_manage_decode_picture
  Description:     将文件解码到目标大小的缓冲区
  Input:           pic_path
                   dest_w
                   dest_h
                   rgb_buf
  Return:          1：成功；0：失败
**********************************************************************/

uint8 divoom_pic_phone_manage_decode_picture(char *pic_path, int *dest_w, int *dest_h, uint8 *rgb_buf);

/*********************************************************************
  Function:        divoom_pic_phone_manage_encode_picture
  Description:     将图像编码JPEG到目标文件
  Input:           pic_path
                   dest_w
                   dest_h
                   rgb_buf
  Return:          none
**********************************************************************/

void divoom_pic_phone_manage_encode_picture(char *pic_path, int dest_w, int dest_h, uint8 *rgb_buf);

/*********************************************************************
  Function:        divoom_pic_phone_manage_get_all_pic_id
  Description:     得到所有的图片ID
  Input:           pic_num      返回照片数目
  Return:          返回照片列表ID
**********************************************************************/

Pdivoom_pic_phone_manage_item_offset divoom_pic_phone_manage_get_all_pic_id(int *pic_num);

/*********************************************************************
  Function:        divoom_pic_phone_manage_get_first_pic_id
  Description:     得到第一张的图片ID
  Input:           
  Return:          返回照片列表ID
**********************************************************************/

int divoom_pic_phone_manage_get_first_pic_id(int album_id);
/*********************************************************************
  Function:        divoom_pic_phone_manage_get_total_pic_num
  Description:     得到图片总数
  Input:           
  Return:          返回图片总数
**********************************************************************/

int divoom_pic_phone_manage_get_total_pic_num(int album_id);

/*********************************************************************
  Function:        divoom_pic_phone_manage_del_picture_thumb
  Description:     删除缩略图文件
  Input:           pic_path
  Return:          none
**********************************************************************/

void divoom_pic_phone_manage_del_picture_thumb(char *pic_path);

/*********************************************************************
  Function:        divoom_pic_phone_manage_get_album_list
  Description:     通过文件ID，得到所属的相册列表
  Input:           pic_id
                   album_list
  Return:          相册个数
**********************************************************************/

int divoom_pic_phone_manage_get_album_list(int pic_id, int *album_list);

/*********************************************************************
  Function:        divoom_pic_phone_manage_get_album_list
  Description:     更新对应照片的相册列表
  Input:           pic_id
                   album_num
                   album_list
  Return:          none
**********************************************************************/

void divoom_pic_phone_manage_update_album_list(int pic_id, uint32 *album_list, int album_num);


/*********************************************************************
  Function:        divoom_pic_phone_manage_get_pic_hide_info
  Description:     得到所有的图片ID
  Input:           pic_num      返回照片数目
                   hide_flag     0:返回所有的照片，1：只返回没有隐藏的照片
  Return:          返回照片列表ID
**********************************************************************/

void divoom_pic_phone_manage_get_pic_hide_info(uint32 *pic_list, Pdivoom_pic_phone_manage_item_offset pic_info, int pic_num);
/*********************************************************************
  Function:        divoom_pic_phone_manage_get_pic_offset_info
  Description:     得到图片相关信息
  Input:           pic_id      
  Return:          返回照片信息
**********************************************************************/

void divoom_pic_phone_manage_get_pic_offset_info(int pic_id, Pdivoom_pic_phone_manage_item_offset pic_info);


/*********************************************************************
  Function:        divoom_pic_phone_manage_set_preload_album_flag
  Description:     设置预装相簿的ID
  Input:           flag      
  Return:          是否在拷贝预装
**********************************************************************/

int divoom_pic_phone_manage_set_preload_album_flag(int flag);
/*********************************************************************
  Function:        divoom_pic_phone_manage_set_all_album_cover_pic
  Description:     设置全部照片封面
  Input:           flag      
  Return:          返回当前值
**********************************************************************/

int divoom_pic_phone_manage_set_all_album_cover_pic(int flag);

/*********************************************************************
  Function:        divoom_pic_phone_manage_get_slide_config
  Description:     得到幻灯片播放配置
  Input:           none
  Return:          配置
**********************************************************************/

Pdivoom_pic_phone_manage_slide_config divoom_pic_phone_manage_get_slide_config(void);

/*********************************************************************
  Function:        divoom_pic_phone_manage_update_pic_attr
  Description:     更新图片相关的属性
  Input:           pitem_info       图片信息
                   number   数目
  Return:          1:更新成功       0：没有要更新的图片
**********************************************************************/

int divoom_pic_phone_manage_update_pic_attr(Pdivoom_pic_phone_manage_item_offset pitem_info, int number, int attr_flag);
/*********************************************************************
  Function:        divoom_pic_phone_manage_update_album_update_time
  Description:     更新相簿定时最新时间
  Input:           update_time
  Return:          最新时间
**********************************************************************/

int divoom_pic_phone_manage_update_album_update_time(int update_time);

/*********************************************************************
  Function:        divoom_pic_phone_manage_remove_pic_list
  Description:     删除一批图片
  Input:           pic_list       图片ID
                   pic_num   
  Return:          none
**********************************************************************/

int divoom_pic_phone_manage_remove_pic_list(int *pic_list, int pic_num);


/*********************************************************************
  Function:        divoom_pic_phone_manage_remove_all_pic
  Description:     删除所有图片
  Input:           none   
                   
  Return:          none
**********************************************************************/

void divoom_pic_phone_manage_remove_all_pic(void);

/*********************************************************************
  Function:        divoom_pic_phone_manage_get_picture_disp_buf
  Description:     从文件解码出来的图像得到显示图像
  Input:           file_name
                   image_buf
                   dest_w
                   dest_h
                   rotate_flag  旋转角度
                   fill_flag    填充类型
                   bg_buf   //背景缓冲区，不需要为空
                   bg_type  //背景类型
  Return:          1：成功；0：失败
**********************************************************************/

int divoom_pic_phone_manage_get_picture_disp_buf(char *file_name, uint8 *image_buf, int *dest_w, int *dest_h, int rotate_flag, int fill_flag, int mirror_flag, uint8 *bg_buf, int bg_type);

/*********************************************************************
  Function:        divoom_pic_phone_manage_decode_picture_rgb_buf
  Description:     从文件解码出来的图像得到显示图像
  Input:           pic_id
                   image_buf
                   dest_w
                   dest_h
  Return:          1：成功；0：失败
**********************************************************************/

int divoom_pic_phone_manage_decode_picture_rgb_buf(int pic_id, uint8 *image_buf, int *dest_w, int *dest_h);




/*********************************************************************
  Function:        divoom_pic_phone_manage_stretch_picture_disp_buf
  Description:     从文件解码出来的图像缩放到指定大小
  Input:           file_name
                   image_buf
                   dest_w
                   dest_h
                   rotate_flag  旋转角度
                   mirror_flag  镜像模式
  Return:          1：成功；0：失败
**********************************************************************/

int divoom_pic_phone_manage_stretch_picture_disp_buf(char *file_name, uint8 *image_buf, int dest_w, int dest_h, int rotate_flag, int mirror_flag);
/*********************************************************************
  Function:        divoom_pic_phone_manage_stretch_bgra_buf
  Description:     将图像缩放到指定大小
  Input:           jpeg_buf
                   image_w
                   image_h
                   image_buf
                   dest_w
                   dest_h
                   rotate_flag  旋转角度
                   mirror_flag  镜像模式
  Return:          none
**********************************************************************/

void divoom_pic_phone_manage_stretch_bgra_buf(uint8 *jpeg_buf, int image_w, int image_h,uint8 *image_buf, int dest_w, int dest_h, int rotate_flag, int mirror_flag);

/*********************************************************************
  Function:        divoom_pic_phone_manage_set_picture_like_attr
  Description:     设置照片的收藏属性
  Input:           pic_id           图片ID
                   like_flag       
  Return:          none
**********************************************************************/
void divoom_pic_phone_manage_set_picture_like_attr(int pic_id, int like_flag);

/*********************************************************************
  Function:        divoom_pic_phone_manage_set_picture_hide_attr
  Description:     设置照片的隐藏属性
  Input:           pic_id           图片ID
                   hide_flag        0:显示，1：隐藏
  Return:          none
**********************************************************************/

void divoom_pic_phone_manage_set_picture_hide_attr(int pic_id, int hide_flag);


/*********************************************************************
  Function:        divoom_pic_phone_manage_set_picture_mirror_attr
  Description:     设置照片的镜像属性
  Input:           pic_id           图片ID
                   mirror_flag        
  Return:          none
**********************************************************************/

void divoom_pic_phone_manage_set_picture_mirror_attr(int pic_id, int mirror_flag);

/*********************************************************************
  Function:        divoom_pic_phone_manage_set_picture_hide_attr
  Description:     设置照片的填充属性
  Input:           pic_id           图片ID
                   fill_flag
  Return:          none
**********************************************************************/

void divoom_pic_phone_manage_set_picture_fill_attr(int pic_id, int fill_flag);

/*********************************************************************
  Function:        divoom_pic_phone_manage_set_picture_rotate_attr
  Description:     设置照片的旋转属性
  Input:           pic_id           图片ID
                   rotate_flag
  Return:          none
**********************************************************************/

void divoom_pic_phone_manage_set_picture_rotate_attr(int pic_id, int rotate_flag);

/*********************************************************************
  Function:        divoom_pic_phone_manage_app_update_slide_config
  Description:     APP设置相关幻灯片属性
  Input:           pconfig_info           图片ID
                   
  Return:          none
**********************************************************************/

void divoom_pic_phone_manage_app_update_slide_config(Pdivoom_photo_album_album_config_info pconfig_info);

/*********************************************************************
  Function:        divoom_pic_phone_manage_get_like_pic_id
  Description:     得到所有的收藏图片ID
  Input:           pic_num      返回照片数目
  Return:          返回照片列表ID
**********************************************************************/

Pdivoom_pic_phone_manage_item_offset divoom_pic_phone_manage_get_like_pic_id(int *pic_num);

/*********************************************************************
  Function:        divoom_pic_phone_manage_update_album_to_pic
  Description:     更新一批相片到相簿
  Input:           pic_list       图片ID
                   pic_num
                   album_id     相册ID
  Return:          none
**********************************************************************/

void divoom_pic_phone_manage_add_piclist_to_album(uint32 *pic_list, int pic_num,  uint32 album_id);

/*********************************************************************
  Function:        divoom_pic_phone_manage_remove_piclist_from_album
  Description:     从相簿中移除一批相片
  Input:           pic_list       图片ID
                   pic_num
                   album_id     相册ID
  Return:          none
**********************************************************************/

void divoom_pic_phone_manage_remove_piclist_from_album(uint32 *pic_list, int pic_num,  uint32 album_id);

/*********************************************************************
  Function:        divoom_pic_phone_manage_convert_rgb_buf
  Description:     转换输出BUFFER
  Input:           src_rgb           
                   dest_rgb
                   image_w  
                   image_h
                   rotate_flag
                   mirror_flag                   
  Return:          none
**********************************************************************/

void divoom_pic_phone_manage_convert_rgb_buf(uint8 *src_rgb, uint8 *dest_rgb, int image_w, int image_h, int rotate_flag, int mirror_flag);

/*********************************************************************
  Function:        divoom_pic_phone_manage_convert_slide_speed_to_percent
  Description:     将显示速度转化为百分比
  Input:           速度
  Return:          百分比
**********************************************************************/

int divoom_pic_phone_manage_convert_slide_speed_to_percent(int play_speed);

/*********************************************************************
  Function:        divoom_pic_phone_manage_convert_percent_to_slide_speed
  Description:     将百分比转化为显示速度
  Input:           百分比
  Return:          速度
**********************************************************************/

int divoom_pic_phone_manage_convert_percent_to_slide_speed(int play_speed);


/*********************************************************************
  Function:        divoom_pic_phone_manage_get_hide_pic_id
  Description:     得到所有的隐藏图片ID
  Input:           pic_num      返回照片数目
  Return:          返回照片列表ID
**********************************************************************/

Pdivoom_pic_phone_manage_item_offset divoom_pic_phone_manage_get_hide_pic_id(int *pic_num);

/*********************************************************************
  Function:        divoom_pic_phone_manage_get_unhide_pic_id
  Description:     得到所有的没有隐藏图片ID
  Input:           pic_num      返回照片数目
  Return:          返回照片列表ID
**********************************************************************/

Pdivoom_pic_phone_manage_item_offset divoom_pic_phone_manage_get_unhide_pic_id(int *pic_num);

/*********************************************************************
  Function:        divoom_pic_phone_manage_save_slide_config
  Description:     相片配置发生变化，将保存配置
  Input:           
  Return:          NONE
**********************************************************************/
void divoom_pic_phone_manage_save_slide_config(int send_flag);

/*********************************************************************
  Function:        divoom_pic_phone_manage_get_pic_id_list
  Description:     获取所有图片的ID
  Input:           pic_num    返回图片张数
  Return:          图片指针
**********************************************************************/

uint32 *divoom_pic_phone_manage_get_pic_id_list(int start, int *pic_num);

#endif

