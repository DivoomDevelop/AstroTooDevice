#ifndef _divoom_hot_manage_h_
#define _divoom_hot_manage_h_

/*********************************************************************
  Function:        divoom_hot_manage_init
  Description:     初始化模块，将相关的文件从spi中load出来
  Input:           file_path  SD路径 
                    
  Return:           none
**********************************************************************/

void divoom_hot_manage_init(void);

int divoom_hot_manage_clear_unused_batch(int *deleted_num);

/* 开机下载启动前直接回收 HOT 文件，至少保留五个本地文件。 */
void divoom_hot_manage_clear_files_on_boot(void);

/*********************************************************************
  Function:        divoom_hot_manage_update_last_pic
  Description:     更新最新推荐的图片，这里只更新６４
  Input:           id
                   url
                   utc_time
                    
  Return:          none
**********************************************************************/

void divoom_hot_manage_update_last_pic(uint32 id, char *url, uint32 utc_time);



/*********************************************************************
  Function:        divoom_hot_manage_get_random_pic_id
  Description:     从 HOT 图片中随机取得图片 ID
  Input:           user_id     返回所选图片的作者 USERID
                   image_name  作者名称输出缓冲区
                   max_size    SIZE_128 只选 128 档；SIZE_256_ONLY 只选 256 档；其它值表示小于等于该上限
                    
  Return:          返回图片ＩＤ
**********************************************************************/

#define DIVOOM_HOT_MANAGE_IMAGE_SIZE_16    (1)
#define DIVOOM_HOT_MANAGE_IMAGE_SIZE_32    (2)
#define DIVOOM_HOT_MANAGE_IMAGE_SIZE_64    (3)
#define DIVOOM_HOT_MANAGE_IMAGE_SIZE_128   (16)
#define DIVOOM_HOT_MANAGE_IMAGE_SIZE_256   (32)
#define DIVOOM_HOT_MANAGE_DEFAULT_MAX_SIZE (DIVOOM_HOT_MANAGE_IMAGE_SIZE_256 + 1U)
/* 只选 256 档。这个值不是图片尺寸，SIZE_256 本身仍表示小于等于 256。 */
#define DIVOOM_HOT_MANAGE_IMAGE_SIZE_256_ONLY (0x80000000U)

uint32 divoom_hot_manage_get_random_pic_id(uint32 *user_id, char *image_name, uint32 max_size);

/* 是否至少有一张满足尺寸条件且已经完整落盘的 HOT 图片。 */
uint8 divoom_hot_manage_has_local_pic(uint32 max_size);

/* 文件落盘或删除时同步维护 HOT 的纯内存可用索引。 */
void divoom_hot_manage_update_local_ready(uint32 image_id, uint8 ready);


/*********************************************************************
  Function:        divoom_hot_manage_get_url_addr
  Description:     获取指定文件的网络地址
  Input:           model_id
                   image_id
                   url_addr
                    
  Return:          image_flag
**********************************************************************/

int divoom_hot_manage_get_url_addr(int model_id, uint32 image_id, char *url_addr);


/*********************************************************************
  Function:        divoom_hot_manage_update_hot_list
  Description:     HOT列表吏淅
  Input:           pimage_list
                   
                    
  Return:          返回图片ＩＤ
**********************************************************************/

void divoom_hot_manage_update_hot_list(Pdivoom_devcice_image_list pimage_list);

/* 异步用户信息回包入口：按 USERID 将作者名称补写到 HOT 项和数据库。 */
void divoom_hot_manage_update_author_list(
    Pdivoom_device_get_user_info_list_respone_info puser_info);

/* 接管响应块；成功后调用方必须清空原指针，并可取得实际新增下载数。 */
int divoom_hot_manage_submit_hot_list(Pdivoom_devcice_image_list pimage_list,
    int32 *add_file_num);

/* 下载队列低于水位时补充下一批，允许下载 worker 和提交路径安全并发调用。 */
void divoom_hot_manage_pump_pending(void);
#endif

