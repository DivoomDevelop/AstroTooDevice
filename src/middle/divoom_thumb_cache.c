#include "divoom.h"
#include "divoom_pic_album_manage.h"
#include "divoom_pixel_format.h"
#ifdef DIVOOM_USE_RK_ID
#include "rk_mpi.h"

#include <rga/im2d.h>
#include <rga/rga.h>
#include <rga/RgaApi.h>
#endif


typedef struct _divoom_thumb_cache_item
{
    int  pic_id;
    int  width;
    int  height;
    uint8 *rgb_buf;
}divoom_thumb_cache_item, *Pdivoom_thumb_cache_item;

typedef struct _divoom_thumb_cache_wait_item
{
    int  pic_id;
    int  image_w;
    int  image_h;
    uint8 *rgb_buf;
    DIVOOM_PIC_ALBUM_MANAGE_READ_OVER *pcallback;
    void *para;
}divoom_thumb_cache_wait_item, *Pdivoom_thumb_cache_wait_item;

typedef struct _divoom_thumb_cache_ctrl
{
    int  read_list_id;
    int  pic_w;
    int  pic_h;
    int wait_flag;
    divoom_clock_id_info pcur_clock_info;
    void *read_sem;
    void *cache_queue;
    void *wait_queue[DIVOOM_PIC_ALBUM_MANAGE_LIST_NUMBER];
    void *task_handle;
}divoom_thumb_cache_ctrl, *Pdivoom_thumb_cache_ctrl;


Pdivoom_thumb_cache_ctrl pdivoom_thumb_cache_handle = NULL;

static int divoom_thumb_cache_get_file_src_type(const char *file_name)
{
    char ext_name[16];

    if(file_name == NULL)
    {
        return RK_FORMAT_BGRA_8888;
    }
    divoom_app_com_get_file_ex_name((char *)file_name, ext_name);
    if(strcasecmp(ext_name, "jpg") == 0 || strcasecmp(ext_name, "jpeg") == 0)
    {
        return RK_FORMAT_RGB_565;
    }
    return RK_FORMAT_BGRA_8888;
}

static size_t divoom_thumb_cache_rgb565_size(void)
{
    return (size_t)pdivoom_thumb_cache_handle->pic_w *
        (size_t)pdivoom_thumb_cache_handle->pic_h *
        DIVOOM_APP_RGB565_BYTES_PER_PX;
}

static void divoom_thumb_cache_write_lvbuf(uint8 *src_rgb565, int src_w,
                                           int src_h, uint8 *dst_rgb565a8,
                                           int dst_w, int dst_h)
{
    if(src_rgb565 == NULL || dst_rgb565a8 == NULL)
    {
        return;
    }
    divoom_rk_mpp_stretch_image_lvfmt(src_rgb565, src_w, src_h,
        RK_FORMAT_RGB_565, dst_rgb565a8, dst_w, dst_h,
        LV_COLOR_FORMAT_RGB565A8);
}

void *divoom_thumb_cache_read_task(void *para)
{
    int empty_num = 0, src_w, src_h;
    char pic_path[256];
    char thumb_path[256];
    Pdivoom_thumb_cache_wait_item pwait_item;
    Pdivoom_thumb_cache_item pcache_item;
    
    while(1)
    {
        if(empty_num >= DIVOOM_PIC_ALBUM_MANAGE_LIST_NUMBER)
        {
            //三个全是空的，就等待
            pdivoom_thumb_cache_handle->wait_flag = 0;
            divoom_system_get_sem(pdivoom_thumb_cache_handle->read_sem);
            pdivoom_thumb_cache_handle->wait_flag = 1;
        }
        pwait_item = divoom_queue_pop(pdivoom_thumb_cache_handle->wait_queue[pdivoom_thumb_cache_handle->read_list_id]);
        pdivoom_thumb_cache_handle->read_list_id ++;
        if(pdivoom_thumb_cache_handle->read_list_id >= DIVOOM_PIC_ALBUM_MANAGE_LIST_NUMBER)
        {
            pdivoom_thumb_cache_handle->read_list_id = 0;
        }
        if(pwait_item == NULL)
        {
            empty_num ++;
            continue;
        }
        empty_num = 0;
        //如果缓冲区已经满了，将从队列中弹出一个元素使用，控制内存使用量
        
        if(divoom_queue_is_full(pdivoom_thumb_cache_handle->cache_queue))
        {
            pcache_item = divoom_queue_pop(pdivoom_thumb_cache_handle->cache_queue);
        }
        else
        {
            pcache_item = divoom_system_malloc(sizeof(divoom_thumb_cache_item) + divoom_thumb_cache_rgb565_size());
            if(pcache_item) pcache_item->rgb_buf = (uint8 *)&pcache_item[1];
        }
        if(pcache_item == NULL)
        {
            divoom_system_free(pwait_item);
            continue;
        }
        pcache_item->pic_id = pwait_item->pic_id;
        if(pdivoom_thumb_cache_handle->pcur_clock_info.clock_id == 0)
        {
            divoom_pic_phone_manage_get_picture_name(pic_path, pwait_item->pic_id);
        }
        else
        {
            divoom_custom_photo_get_pic_name_from_id(pwait_item->pic_id, pic_path);
        }
        divoom_thumb_cache_get_thumb_name(pic_path, thumb_path);
        uint8 * image_read_buf = divoom_rk_mpp_get_jpeg_info(thumb_path, &src_w, &src_h);
        int src_type = divoom_thumb_cache_get_file_src_type(thumb_path);
        pcache_item->width = pdivoom_thumb_cache_handle->pic_w;
        pcache_item->height = pdivoom_thumb_cache_handle->pic_h;
        if(image_read_buf == NULL)
        {
            //缩略图不存在，将创建

            divoom_pic_phone_manage_create_picture_thumb(pic_path, &src_w, &src_h, NULL);
            image_read_buf = divoom_rk_mpp_get_jpeg_info(thumb_path, &src_w, &src_h);
            src_type = divoom_thumb_cache_get_file_src_type(thumb_path);
        }
        if(image_read_buf == NULL)
        {
            /* 原图不存在或解码失败时不得把未初始化缓冲区放入缓存，否则 UI 会显示黑框。 */
            divoom_system_free(pcache_item);
            divoom_system_free(pwait_item);
            continue;
        }
        divoom_rk_mpp_stretch_image_lvfmt(image_read_buf, src_w, src_h, src_type,
            pcache_item->rgb_buf, pcache_item->width, pcache_item->height,
            LV_COLOR_FORMAT_RGB565);
        //memcpy(pcache_item->rgb_buf, image_read_buf ,pcache_item->width * pcache_item->height * 4);
        divoom_system_free(image_read_buf);
        if(pwait_item->pcallback)
        {
            divoom_thumb_cache_write_lvbuf(pcache_item->rgb_buf, pcache_item->width,
                pcache_item->height, pwait_item->rgb_buf, pwait_item->image_w,
                pwait_item->image_h);
            pwait_item->pcallback(pwait_item->para, pwait_item->rgb_buf, pwait_item->image_w, pwait_item->image_h);
        }
        divoom_system_free(pwait_item);
        if(!divoom_queue_push(pdivoom_thumb_cache_handle->cache_queue, pcache_item))
            divoom_system_free(pcache_item);
    }
    return NULL;
}

/*********************************************************************
  Function:        divoom_thumb_cache_set_dest_pic_size
  Description:     设置目标缓冲区照片的大小
  Input:           pic_w     
                   pic_h
  Return:          none
**********************************************************************/

void divoom_thumb_cache_set_dest_pic_size(int pic_w, int pic_h)
{
    if(pdivoom_thumb_cache_handle == NULL) return;
    if(pdivoom_thumb_cache_handle->pic_w != pic_w || pdivoom_thumb_cache_handle->pic_h != pic_h)
    {
        divoom_thumb_cache_clear_all();
        pdivoom_thumb_cache_handle->pic_w = pic_w;
        pdivoom_thumb_cache_handle->pic_h = pic_h;
    }
}


/*********************************************************************
  Function:        divoom_thumb_cache_init
  Description:     模块初始化
  Input:           max_item     缓冲区最大缓冲缩图的数目
  Return:          none
**********************************************************************/

void divoom_thumb_cache_init(int max_item)
{
    int i;
    Pdivoom_thumb_cache_ctrl handle;

    if(pdivoom_thumb_cache_handle) return;
    handle = divoom_system_malloc(sizeof(*handle));
    if(handle == NULL) return;
    memset(handle, 0, sizeof(*handle));
    handle->cache_queue = divoom_queue_init(max_item, __func__, __LINE__);
    if(handle->cache_queue == NULL) goto failed;
    for(i = 0; i < DIVOOM_PIC_ALBUM_MANAGE_LIST_NUMBER; i++)
    {
        handle->wait_queue[i] = divoom_queue_init(max_item * 2, __func__, __LINE__);
        if(handle->wait_queue[i] == NULL) goto failed;
    }
    handle->pic_w = DIVOOM_THUMB_PIC_WIDTH;
    handle->pic_h = DIVOOM_THUMB_PIC_HIGH;
    handle->read_sem = divoom_system_create_sem(1, 0);
    if(handle->read_sem == NULL) goto failed;
    pdivoom_thumb_cache_handle = handle;
    return;
failed:
    for(i = 0; i < DIVOOM_PIC_ALBUM_MANAGE_LIST_NUMBER; i++)
        if(handle->wait_queue[i]) divoom_queue_destroy(handle->wait_queue[i], NULL);
    if(handle->cache_queue) divoom_queue_destroy(handle->cache_queue, NULL);
    divoom_system_free(handle);
}

/*********************************************************************
  Function:        divoom_thumb_cache_set_clock_info
  Description:     设置表盘信息
  Input:           pclock_info     
  Return:          none
**********************************************************************/

void divoom_thumb_cache_set_clock_info(Pdivoom_clock_id_info pclock_info)
{
    if(pdivoom_thumb_cache_handle == NULL) return;
    divoom_thumb_cache_clear_all();
    if(pclock_info == NULL)
    {
        memset(&pdivoom_thumb_cache_handle->pcur_clock_info, 0, sizeof(divoom_clock_id_info));
    }
    else
    {
        memcpy(&pdivoom_thumb_cache_handle->pcur_clock_info, pclock_info, sizeof(divoom_clock_id_info));
    }
}

int divoom_thumb_cache_read_pic_compare(void *item1, void *item2)
{
    Pdivoom_thumb_cache_item pcache_item = (Pdivoom_thumb_cache_item)item2;

    return pcache_item->pic_id == *(int *)item1 ?0:1;
}

int divoom_thumb_cache_read_pic_compare_wait(void *item1, void *item2)
{
    Pdivoom_thumb_cache_wait_item pcache_item = (Pdivoom_thumb_cache_wait_item)item2;

    return pcache_item->pic_id == *(int *)item1 ?0:1;
}

/*********************************************************************
  Function:        divoom_thumb_cache_get_thumb_name
  Description:     通过文件名得到缩略图的文件名
  Input:           pic_name
                   thumb_name
                   
  Return:          none
**********************************************************************/

void divoom_thumb_cache_get_thumb_name(char *pic_name, char *thumb_name)
{
    int len;
    
    strcpy(thumb_name, DIVOOM_THUMB_BASE_PATH);
    strcat(thumb_name, &pic_name[strlen(DIVOOM_SD_APP_PIC_DIR)]);
    len = strlen(thumb_name) - 1;

    while(len > 0 && thumb_name[len] != '.')
    {
        len --;
    }

    if(len)
    {
        strcpy(&thumb_name[len + 1], "webp");
    }
    
}
/*********************************************************************
  Function:        divoom_thumb_cache_clear_pic
  Description:     删除缩略图缓冲区，并删除缩图文件
  Input:           pic_id
                   list_id
  Return:          none
**********************************************************************/

void divoom_thumb_cache_clear_pic(int pic_id)
{
    if(pdivoom_thumb_cache_handle == NULL) return;
    char pic_path[256];
    char thumb_path[256];
    Pdivoom_thumb_cache_item pcache_item;

    #ifdef DIVOOM_USE_PIC_THUMB
    divoom_pic_phone_manage_get_picture_name(pic_path, pic_id);
    divoom_thumb_cache_get_thumb_name(pic_path, thumb_path);
    divoom_app_com_del_file(thumb_path);
    #endif
    pcache_item = divoom_queue_pop_cmp(pdivoom_thumb_cache_handle->cache_queue, &pic_id, divoom_thumb_cache_read_pic_compare);
    if(pcache_item)
    {
        divoom_system_free(pcache_item);
    }
}
/*********************************************************************
  Function:        divoom_thumb_cache_read_pic
  Description:     读取缩略图RGB数据，此函数不能阻塞，如果数据未准备好，将直接返回，再由回调通知一下
  Input:           pic_id
                   rgb_buf
                   pcallback
                   para
  Return:          1:成功读取  0：未成功读取
**********************************************************************/

int divoom_thumb_cache_read_pic(int list_id, int pic_id, int asyn_flag, uint8 *rgb_buf, int image_w, int image_h,DIVOOM_PIC_ALBUM_MANAGE_READ_OVER *pcallback, void *para)
{
    if(pdivoom_thumb_cache_handle == NULL || rgb_buf == NULL ||
        list_id < 0 || list_id >= DIVOOM_PIC_ALBUM_MANAGE_LIST_NUMBER) return 0;
    Pdivoom_thumb_cache_item pcache_item;
    Pdivoom_thumb_cache_wait_item pwait_item;

    divoom_print("read pic id: %d\n", pic_id);
    pcache_item = divoom_queue_pop_cmp(pdivoom_thumb_cache_handle->cache_queue, &pic_id, divoom_thumb_cache_read_pic_compare);
    if(pcache_item)
    {
        divoom_thumb_cache_write_lvbuf(pcache_item->rgb_buf, pdivoom_thumb_cache_handle->pic_w,
            pdivoom_thumb_cache_handle->pic_h, rgb_buf, image_w, image_h);
        divoom_system_free(pcache_item);
        //divoom_queue_push(pdivoom_thumb_cache_handle->cache_queue, pcache_item);
        return 1;
    }
    if(asyn_flag)
    {        
        int src_w, src_h;
        char pic_path[256];
        char thumb_path[256];
        

        if(divoom_queue_is_full(pdivoom_thumb_cache_handle->cache_queue))
        {
            pcache_item = divoom_queue_pop(pdivoom_thumb_cache_handle->cache_queue);
        }
        else
        {
            pcache_item = divoom_system_malloc(sizeof(divoom_thumb_cache_item) + divoom_thumb_cache_rgb565_size());
            if(pcache_item) pcache_item->rgb_buf = (uint8 *)&pcache_item[1];
        }
        if(pcache_item == NULL) return 0;
        pcache_item->width = pdivoom_thumb_cache_handle->pic_w;
        pcache_item->height = pdivoom_thumb_cache_handle->pic_h;
        
        
        pcache_item->pic_id = pic_id;
        if(pdivoom_thumb_cache_handle->pcur_clock_info.clock_id == 0)
        {
            divoom_pic_phone_manage_get_picture_name(pic_path, pic_id);
        }
        else
        {
            divoom_custom_photo_get_pic_name_from_id(pic_id, pic_path);
        }
        divoom_thumb_cache_get_thumb_name(pic_path, thumb_path);
        uint8 * image_read_buf = divoom_rk_mpp_get_jpeg_info(thumb_path, &src_w, &src_h);
        int src_type = divoom_thumb_cache_get_file_src_type(thumb_path);
        if(image_read_buf == NULL)
        {
            divoom_pic_phone_manage_create_picture_thumb(pic_path, &src_w, &src_h, NULL);
            image_read_buf = divoom_rk_mpp_get_jpeg_info(thumb_path, &src_w, &src_h);
            src_type = divoom_thumb_cache_get_file_src_type(thumb_path);
        }
        pcache_item->width = pdivoom_thumb_cache_handle->pic_w;
        pcache_item->height = pdivoom_thumb_cache_handle->pic_h;
        if(image_read_buf != NULL)
        {
            divoom_rk_mpp_stretch_image_lvfmt(image_read_buf, src_w, src_h, src_type,
                pcache_item->rgb_buf, pcache_item->width, pcache_item->height,
                LV_COLOR_FORMAT_RGB565);
            divoom_system_free(image_read_buf);
        }
        else
        {
            /* 读取失败时明确返回失败，并保持缓存中只包含有效缩略图。 */
            divoom_system_free(pcache_item);
            return 0;
        }
        divoom_thumb_cache_write_lvbuf(pcache_item->rgb_buf, pdivoom_thumb_cache_handle->pic_w,
            pdivoom_thumb_cache_handle->pic_h, rgb_buf, image_w, image_h);
        if(!divoom_queue_push(pdivoom_thumb_cache_handle->cache_queue, pcache_item))
            divoom_system_free(pcache_item);
        return 1;
    }
    pwait_item = divoom_queue_pop_cmp(pdivoom_thumb_cache_handle->wait_queue[list_id], &pic_id, divoom_thumb_cache_read_pic_compare_wait);
    if(pwait_item == NULL)
    {
        pwait_item = divoom_system_malloc(sizeof(divoom_thumb_cache_wait_item));
        if(pwait_item == NULL) return 0;
        pwait_item->pic_id = pic_id;
    }
    pwait_item->pcallback = pcallback;
    pwait_item->para = para;
    pwait_item->rgb_buf = rgb_buf;
    pwait_item->image_w = image_w;
    pwait_item->image_h = image_h;
    
    if(!divoom_queue_push_head(pdivoom_thumb_cache_handle->wait_queue[list_id], pwait_item))
    {
        divoom_system_free(pwait_item);
        return 0;
    }
    pdivoom_thumb_cache_handle->read_list_id = list_id;
    divoom_system_put_sem(pdivoom_thumb_cache_handle->read_sem);
    
    

    return 0;
}


/*********************************************************************
  Function:        divoom_thumb_cache_clear_wait
  Description:     清除预加载图片的缩略图队列，这个一般在用户滑动方向变化时调用
  Input:           none
  Return:          none
**********************************************************************/

void divoom_thumb_cache_clear_wait(int list_id)
{
    if(pdivoom_thumb_cache_handle == NULL || list_id < 0 ||
        list_id >= DIVOOM_PIC_ALBUM_MANAGE_LIST_NUMBER) return;
    divoom_queue_pop_all(pdivoom_thumb_cache_handle->wait_queue[list_id], NULL);
}

/*********************************************************************
  Function:        divoom_thumb_cache_pre_read
  Description:     预加载图片的缩略图，这个方便后面读取提速，由应用层提前调用
  Input:           pic_id
  Return:          none
**********************************************************************/

void divoom_thumb_cache_pre_read(int list_id, int pic_id)
{
    if(pdivoom_thumb_cache_handle == NULL || list_id < 0 ||
        list_id >= DIVOOM_PIC_ALBUM_MANAGE_LIST_NUMBER) return;
    Pdivoom_thumb_cache_item pcache_item;
    Pdivoom_thumb_cache_wait_item pwait_item;

    //如果预缓冲的图片不在等待队列也不在缓冲队列，将向等待队列压一个读操作
    pwait_item = divoom_queue_get_cmp(pdivoom_thumb_cache_handle->wait_queue[list_id], &pic_id, divoom_thumb_cache_read_pic_compare_wait);
    if(pwait_item ==NULL)
    {
        pcache_item = divoom_queue_get_cmp(pdivoom_thumb_cache_handle->cache_queue, &pic_id, divoom_thumb_cache_read_pic_compare);
        if(pcache_item == NULL)
        {
            pwait_item = divoom_system_malloc(sizeof(divoom_thumb_cache_wait_item));
            if(pwait_item == NULL) return;
            pwait_item->pic_id = pic_id;
            pwait_item->pcallback = NULL;
            pwait_item->para = NULL;
            pwait_item->rgb_buf = NULL;
            if(!divoom_queue_push(pdivoom_thumb_cache_handle->wait_queue[list_id], pwait_item))
            {
                divoom_system_free(pwait_item);
                return;
            }
            divoom_system_put_sem(pdivoom_thumb_cache_handle->read_sem);
        }
    }

}

/*********************************************************************
  Function:        divoom_thumb_cache_clear_all
  Description:     清除这个模块的所有申请的内存，让队列为空,在退出缩略图的使用场景情况下调用
  Input:           none
  Return:          none
**********************************************************************/

void divoom_thumb_cache_clear_all(void)
{
    if(pdivoom_thumb_cache_handle == NULL) return;
    int i;
    for(i = 0; i < DIVOOM_PIC_ALBUM_MANAGE_LIST_NUMBER; i++)
    {
        divoom_queue_pop_all(pdivoom_thumb_cache_handle->wait_queue[i], NULL);
    }
    divoom_queue_pop_all(pdivoom_thumb_cache_handle->cache_queue, NULL);
    for(int i = 0; i < 200 && pdivoom_thumb_cache_handle->wait_flag==1; i++)
    {
        divoom_system_delay_ms(10);
    }
    
    //pdivoom_thumb_cache_handle->pic_w = DIVOOM_THUMB_PIC_WIDTH;
    //pdivoom_thumb_cache_handle->pic_h = DIVOOM_THUMB_PIC_HIGH;
}
