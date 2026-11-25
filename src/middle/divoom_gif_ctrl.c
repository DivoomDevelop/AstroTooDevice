#include "divoom.h"
#include "divoom_pixel_format.h"

enum{
    DIVOOM_LIGHT_APP_LOCAL_IMAGE_TYPE_APP,
    DIVOOM_LIGHT_APP_LOCAL_IMAGE_TYPE_HTTP,

};

/* URL 下载先落到与 PixelLocalHttp 相同的暂存目录，再交给 Light App 版本化入库。 */
#define DIVOOM_GIF_CTRL_HTTP_IMAGE_STAGE_FORMAT "remote_http_image_%u.bin"


typedef struct _divoom_local_image_info
{
	uint32 total_len; //该动画的总长度
	uint32 offset; //当前分包的偏移
	uint32 user_id;
	uint32 packet_id;
	uint32 pic_len; //当前分包的长度
	uint8 *pic_data; //分包数据
}divoom_local_image_info, *Pdivoom_local_image_info;

typedef struct _divoom_local_image_ctrl
{

	void *fp; //文件句柄
	uint8  image_width;      //图像宽度
	uint8  gif_type;        //图像类型
	uint16  image_speed;      //图像速度
	uint32 write_image_id; //要写的image_id
	uint32 old_image_flag; //记录一下旧包
	int    image_aes_flag;  //数据是否AES加密
	int cur_image_flag;//记录新包
	uint32 local_image_offset; //记录动画的长度
	uint16 local_image_pack_index; //记录是第几个包
	void  *file_handle;
	void *local_image_queue; //先用队列进行保存
	void *local_image_sem;
	void *local_image_mutex;
}divoom_local_image_ctrl, *Pdivoom_local_image_ctrl;

typedef struct _divoom_gif_ctrl_info
{
	uint8 status;   //0:未使用状态，１：使用状态
	uint8 id;    
    uint8 remote_flag;  //类型
    uint8 new_flag;     //是否被使用过
    int data_packet_time;  //数据包
    int  accept_image_id;   //这个是在不断增加，而且是唯一的
    char remote_addr[256];
    char voice_addr[256];
	divoom_draw_remote_json_image_info remote_image_info;
	uint32 remote_stage_generation;
	char file_id[256]; //存储utf8
	DIVOOM_MANAGE_IMAGE_DOWNLOAD_OK_CALLBACK *cb; //通知上层
	divoom_local_image_ctrl local_image_ctrl; //本地图下载控制
}divoom_gif_ctrl_info, *Pdivoom_gif_ctrl_info;


static Pdivoom_gif_ctrl_info gif_ctrl_handle = NULL;
void divoom_gif_ctrl_save_image(Pdivoom_http_download_image_file_info pimage_handle);

/*
  Function:       divoom_gif_ctrl_set_callback
  Description:	  设置callback，当该动画准备好了之后通知light_app
  Input:          NONE
  Return:         cb 回调函数
  Author:         wangft
  Data:           2020-12-18
*/
void divoom_gif_ctrl_set_callback(DIVOOM_MANAGE_IMAGE_DOWNLOAD_OK_CALLBACK *cb)
{
	if (gif_ctrl_handle == NULL)
		return;

	gif_ctrl_handle->cb = cb;
}

void divoom_gif_ctrl_free_local_image_buf(void)
{
	uint16 i = 0;

    if(gif_ctrl_handle->local_image_ctrl.file_handle)
    {
        divoom_port_file_close(gif_ctrl_handle->local_image_ctrl.file_handle);
        gif_ctrl_handle->local_image_ctrl.file_handle = NULL;
    }
	gif_ctrl_handle->local_image_ctrl.local_image_pack_index = 0;
}

void divoom_gif_ctrl_clear_local_image(void)
{
	if (gif_ctrl_handle == NULL)
	{
        return;
	}
	divoom_system_get_lock(gif_ctrl_handle->local_image_ctrl.local_image_mutex);
    gif_ctrl_handle->data_packet_time = time(0);
	gif_ctrl_handle->local_image_ctrl.local_image_offset = 0;
    divoom_print("clear items: %d\n", divoom_queue_items(gif_ctrl_handle->local_image_ctrl.local_image_queue));
	divoom_queue_pop_all(gif_ctrl_handle->local_image_ctrl.local_image_queue, NULL);
    gif_ctrl_handle->local_image_ctrl.cur_image_flag = 0;
	divoom_gif_ctrl_free_local_image_buf();
	divoom_system_free_lock(gif_ctrl_handle->local_image_ctrl.local_image_mutex);
}

/*
  Function:       divoom_gif_ctrl_request_other_pacet
  Description:	  图像数据包不完整，将请求其它的包
  Input:          NONE
  Return:         NONE
*/

int divoom_gif_ctrl_request_other_pacet(void)
{
    uint16 local_image_pack_index;
    divoom_need_local_info image_packet_info;

    if(gif_ctrl_handle->data_packet_time + 1 > time(0))
    {
        divoom_print("pass request!\n");
        return 0;
    }
    gif_ctrl_handle->data_packet_time = time(0);
    local_image_pack_index = gif_ctrl_handle->local_image_ctrl.local_image_pack_index;
    divoom_print("check items: %d, %d\n",local_image_pack_index, divoom_queue_items(gif_ctrl_handle->local_image_ctrl.local_image_queue));
    if(local_image_pack_index || divoom_queue_items(gif_ctrl_handle->local_image_ctrl.local_image_queue))
    {
        #if 0
        if(local_image_pack_index)
        {
            local_image_pack_index --;
            image_packet_info.packet_id = gif_ctrl_handle->local_image_ctrl.local_image_buf[local_image_pack_index].packet_id + 1;
            image_packet_info.user_id = gif_ctrl_handle->local_image_ctrl.local_image_buf[local_image_pack_index].user_id;
        }
        else
        #endif
        {
            Pdivoom_local_image_info pimage_info = divoom_queue_get_index_item(gif_ctrl_handle->local_image_ctrl.local_image_queue, 0);
            if(pimage_info == NULL)
            {
                return 1;
            }
            image_packet_info.packet_id = 0;
            image_packet_info.user_id = pimage_info->user_id;
        }
        divoom_print("we will request app packet: %d, %d\n", image_packet_info.packet_id, image_packet_info.user_id);
        divoom_net_app_send_packet(DIVOOM_NET_COMM_NEED_LOCAL_DATA, DIVOOM_APP_TOPIC,&image_packet_info);
    }
    return 1;
}

Pdivoom_need_local_info divoom_gif_ctrl_get_request_pacet(void)
{
	uint16 local_image_pack_index;
	Pdivoom_need_local_info image_packet_info = divoom_system_malloc(sizeof(divoom_need_local_info));

	if (image_packet_info == NULL)
		return NULL;

	local_image_pack_index = gif_ctrl_handle->local_image_ctrl.local_image_pack_index;
	divoom_print("check items: %d, %d\n",local_image_pack_index, divoom_queue_items(gif_ctrl_handle->local_image_ctrl.local_image_queue));
	if (local_image_pack_index || divoom_queue_items(gif_ctrl_handle->local_image_ctrl.local_image_queue))
	{
        #if 0
		if (local_image_pack_index)
		{
			local_image_pack_index --;
			image_packet_info->packet_id = gif_ctrl_handle->local_image_ctrl.local_image_buf[local_image_pack_index].packet_id + 1;
			image_packet_info->user_id = gif_ctrl_handle->local_image_ctrl.local_image_buf[local_image_pack_index].user_id;
		}
		else
        #endif
		{
			Pdivoom_local_image_info pimage_info = divoom_queue_get_index_item(gif_ctrl_handle->local_image_ctrl.local_image_queue, 0);
			if (pimage_info == NULL)
			{
				divoom_free(image_packet_info);
				return NULL;
			}
			image_packet_info->packet_id = 0;
			image_packet_info->user_id = pimage_info->user_id;
		}
		divoom_print("we will request app packet: %d, %d\n", image_packet_info->packet_id, image_packet_info->user_id);
	}
	return image_packet_info;
}

/*
  Function:       divoom_gif_ctrl_update_local_image
  Description:	  把本地动画加载到image管理模块
  Input:          image_info 本地动画信息
  Return:         0下载未完成 1表示下载完成
  Author:         wangft
  Data:           2020-12-18
*/
int divoom_gif_ctrl_update_local_image(Pdivoom_draw_local_json_image_info image_info)
{
	if (gif_ctrl_handle == NULL)
		return 0;

    if(image_info->pic_len == 0)
    {
        divoom_print("it is error at:off: %d, len: %d, tol: %d\n", image_info->offset, image_info->pic_len, image_info->total_len);
    }

    gif_ctrl_handle->data_packet_time = time(0);

    if(DIVOOM_LIGHT_APP_LOCAL_IMAGE_TYPE_APP != gif_ctrl_handle->local_image_ctrl.gif_type)
    {
        //不同类型包，将设置清除
        gif_ctrl_handle->local_image_ctrl.cur_image_flag = -1;
    }

	if (image_info->image_flag != gif_ctrl_handle->local_image_ctrl.cur_image_flag)
	{
		if (image_info->image_flag > gif_ctrl_handle->local_image_ctrl.cur_image_flag) //新的动画
		{
			gif_ctrl_handle->local_image_ctrl.cur_image_flag = image_info->image_flag;
			divoom_system_get_lock(gif_ctrl_handle->local_image_ctrl.local_image_mutex);
			gif_ctrl_handle->local_image_ctrl.local_image_offset = 0;
			divoom_queue_pop_all(gif_ctrl_handle->local_image_ctrl.local_image_queue, NULL); //清空
			divoom_gif_ctrl_free_local_image_buf();
			divoom_system_free_lock(gif_ctrl_handle->local_image_ctrl.local_image_mutex);
		}
		else //上一个旧包，要抛弃
		{
			return 0;
		}
	}
    gif_ctrl_handle->local_image_ctrl.gif_type = DIVOOM_LIGHT_APP_LOCAL_IMAGE_TYPE_APP;

    if(image_info->offset < gif_ctrl_handle->local_image_ctrl.local_image_offset)
    {
        divoom_print("we will give up the repeat packet!: %d\n", image_info->offset);
        return 0;
    }

	//把数据写进队列
	Pdivoom_local_image_info new_image = divoom_system_malloc(sizeof(divoom_local_image_info) + image_info->pic_len);
	if (new_image == NULL)
		return 0;
	new_image->pic_data = (uint8 *)&new_image[1];
	new_image->total_len = image_info->total_len;
	new_image->offset = image_info->offset;
	new_image->pic_len = image_info->pic_len;
	new_image->user_id = image_info->user_id;
	new_image->packet_id = image_info->packet_id;
    divoom_print("off: %d, len: %d, tol: %d, %d, %d, %d\n", image_info->offset, image_info->pic_len, image_info->total_len, image_info->image_flag, gif_ctrl_handle->local_image_ctrl.cur_image_flag, new_image->packet_id);
	memcpy(new_image->pic_data, image_info->pic_data, image_info->pic_len);
	divoom_queue_push(gif_ctrl_handle->local_image_ctrl.local_image_queue, new_image);
	divoom_system_put_sem(gif_ctrl_handle->local_image_ctrl.local_image_sem);
	return 1;
}

int divoom_gif_ctrl_update_remote_by_app_image(Pdivoom_draw_remote_by_app_json_image_info image_info)
{
	if (gif_ctrl_handle == NULL)
		return 0;

    if(image_info->pic_len == 0)
    {
        divoom_print("it is error at:off: %d, len: %d, tol: %d\n", image_info->offset, image_info->pic_len, image_info->total_len);
    }

    gif_ctrl_handle->data_packet_time = time(0);
	if (strncmp(gif_ctrl_handle->file_id, image_info->file_id, image_info->file_id_len) != 0)
	{
		memset(gif_ctrl_handle->file_id, 0, sizeof(gif_ctrl_handle->file_id));
		memcpy(gif_ctrl_handle->file_id, image_info->file_id, image_info->file_id_len);
		divoom_system_get_lock(gif_ctrl_handle->local_image_ctrl.local_image_mutex);
		gif_ctrl_handle->local_image_ctrl.local_image_offset = 0;
		divoom_queue_pop_all(gif_ctrl_handle->local_image_ctrl.local_image_queue, NULL); //清空
		divoom_gif_ctrl_free_local_image_buf();
		divoom_system_free_lock(gif_ctrl_handle->local_image_ctrl.local_image_mutex);
	}
	
    gif_ctrl_handle->local_image_ctrl.gif_type = DIVOOM_LIGHT_APP_LOCAL_IMAGE_TYPE_APP;

    if (image_info->offset < gif_ctrl_handle->local_image_ctrl.local_image_offset)
    {
        divoom_print("we will give up the repeat packet!: %d\n", image_info->offset);
        return 0;
    }

	//把数据写进队列
	Pdivoom_local_image_info new_image = divoom_system_malloc(sizeof(divoom_local_image_info) + image_info->pic_len);
	if (new_image == NULL)
		return 0;
	new_image->pic_data = (uint8 *)&new_image[1];
	new_image->total_len = image_info->total_len;
	new_image->offset = image_info->offset;
	new_image->pic_len = image_info->pic_len;
	new_image->user_id = image_info->user_id;
	new_image->packet_id = image_info->packet_id;
    divoom_print("off: %d, len: %d, tol: %d, %d, %d\n", image_info->offset, image_info->pic_len, image_info->total_len, gif_ctrl_handle->local_image_ctrl.cur_image_flag, new_image->packet_id);
	memcpy(new_image->pic_data, image_info->pic_data, image_info->pic_len);
	divoom_queue_push(gif_ctrl_handle->local_image_ctrl.local_image_queue, new_image);
	divoom_system_put_sem(gif_ctrl_handle->local_image_ctrl.local_image_sem);
	return 1;
}
/*
  Function:       divoom_gif_ctrl_get_http_image_pic_id
  Description:	  得到HTTP动画可以使用的ＰＩＣ　ＩＤ
  Input:          image_info 本地动画信息
  Return:         0下载未完成 1表示下载完成
*/
int divoom_gif_ctrl_get_http_image_pic_id(void)
{
    if(gif_ctrl_handle == NULL)
    {
        return 1;
    }
    return gif_ctrl_handle->local_image_ctrl.cur_image_flag + 1;
}
/*
  Function:       divoom_gif_ctrl_reset_http_gif_id
  Description:	  把HTTP gif id 重置
  Input:          
  Return:         
*/
void divoom_gif_ctrl_reset_http_gif_id(void)
{
    if(gif_ctrl_handle)
    {
        gif_ctrl_handle->local_image_ctrl.cur_image_flag = -1;
    }
}

/*
  Function:       divoom_gif_ctrl_update_http_image
  Description:	  把HTTP动画加载到image管理模块
  Input:          image_info 本地动画信息
  Return:         0下载未完成 1表示下载完成
*/
int divoom_gif_ctrl_update_http_image(Pdivoom_drawing_json_http_gif_info image_info)
{
    int pic_size;
    
	if (gif_ctrl_handle == NULL)
		return 0;

    if(image_info->pic_num > DIVOOM_LIGHT_APP_HTTP_IMAGE_NUM)
    {
        return 0;
    }
    
    if(DIVOOM_LIGHT_APP_LOCAL_IMAGE_TYPE_HTTP != gif_ctrl_handle->local_image_ctrl.gif_type)
    {
        //不同类型包，将设置清除
        gif_ctrl_handle->local_image_ctrl.cur_image_flag = -1;
    }

	if (image_info->image_flag != gif_ctrl_handle->local_image_ctrl.cur_image_flag)
	{
		if (image_info->image_flag > gif_ctrl_handle->local_image_ctrl.cur_image_flag) //新的动画
		{
			gif_ctrl_handle->local_image_ctrl.cur_image_flag = image_info->image_flag;
			divoom_system_get_lock(gif_ctrl_handle->local_image_ctrl.local_image_mutex);
			gif_ctrl_handle->local_image_ctrl.local_image_offset = 0;
			divoom_queue_pop_all(gif_ctrl_handle->local_image_ctrl.local_image_queue, NULL); //清空
			divoom_gif_ctrl_free_local_image_buf();
			divoom_system_free_lock(gif_ctrl_handle->local_image_ctrl.local_image_mutex);
		}
		else //上一个旧包，要抛弃
		{
			return 0;
		}
	}
    gif_ctrl_handle->local_image_ctrl.gif_type = DIVOOM_LIGHT_APP_LOCAL_IMAGE_TYPE_HTTP;

	//把数据写进队列
	pic_size = image_info->pic_width * image_info->pic_width *
        (int)DIVOOM_APP_RGB565_BYTES_PER_PX;
	Pdivoom_local_image_info new_image = divoom_system_malloc(sizeof(divoom_local_image_info) + pic_size);
	if (new_image == NULL)
		return 0;
	new_image->pic_data = (uint8 *)&new_image[1];
	new_image->total_len = image_info->pic_num * pic_size;
    new_image->offset = image_info->pic_off * pic_size;
	new_image->pic_len = pic_size;
    divoom_print("off: %d, len: %d, tol: %d, %d, %d\n", image_info->pic_off, image_info->pic_width, image_info->pic_num, image_info->image_flag, gif_ctrl_handle->local_image_ctrl.cur_image_flag);
	divoom_rgb888_to_rgb565_buf(image_info->pic_buf, new_image->pic_data,
        (uint32)image_info->pic_width * (uint32)image_info->pic_width);
    gif_ctrl_handle->local_image_ctrl.image_width = image_info->pic_width;
    gif_ctrl_handle->local_image_ctrl.image_speed = image_info->pic_speed;
    if(image_info->pic_off == image_info->pic_num - 1)
    {
        divoom_disp_gif_clear_text();
        divoom_disp_phone_ctrl_clear_text();
    }
	divoom_queue_push(gif_ctrl_handle->local_image_ctrl.local_image_queue, new_image);
	divoom_system_put_sem(gif_ctrl_handle->local_image_ctrl.local_image_sem);
	return 1;
}

/*
  Function:       divoom_gif_ctrl_update_remote_image
  Description:	  把服务器上的动画加载到image管理模块
  Input:          image_info 服务器上动画信息
  Return:         NONE
  Author:         wangft
  Data:           2020-12-18
*/
void divoom_gif_ctrl_update_remote_image(Pdivoom_draw_remote_json_image_info image_info)
{
	if (gif_ctrl_handle == NULL)
		return;
    
    divoom_gif_ctrl_clear_local_image();
    divoom_system_get_lock(gif_ctrl_handle->local_image_ctrl.local_image_mutex);
    gif_ctrl_handle->remote_flag = image_info->flag;

    memcpy(&gif_ctrl_handle->remote_image_info, image_info,
        sizeof(gif_ctrl_handle->remote_image_info));
    strcpy(gif_ctrl_handle->remote_addr, image_info->file_id);  
    strcpy(gif_ctrl_handle->voice_addr, image_info->voice_id);   
    divoom_system_free_lock(gif_ctrl_handle->local_image_ctrl.local_image_mutex);
    divoom_system_put_sem(gif_ctrl_handle->local_image_ctrl.local_image_sem);
}

/*
  Function:       divoom_gif_ctrl_wite_task
  Description:	  利用线程处理队列
  Input:          NONE
  Return:         NONE
  Author:         wangft
  Data:           2020-12-18
*/
int divoom_gif_ctrl_queue_cmp(void *item1, void *item2)
{
	uint32 index = *(uint32 *)item1;
	Pdivoom_local_image_info image_info = (Pdivoom_local_image_info)item2;
	
	if (index == image_info->offset)
	{
		return 0;
	}
	return 1;
}

/*
  Function:        divoom_gif_ctrl_set_image_id
  Description:     设置图像ID
  Input:            
                    
  Return:           none
*/
void divoom_gif_ctrl_set_image_id(int image_id)
{
    gif_ctrl_handle->accept_image_id = image_id;
}

/*
  Function:        divoom_gif_ctrl_get_image_id
  Description:     读取图像ID
  Input:            
                    
  Return:           图像ID
*/
int divoom_gif_ctrl_get_image_id(void)
{
    return gif_ctrl_handle->accept_image_id;
}
#if 0
/*
  Function:        divoom_gif_ctrl_get_local_buffer_callback
  Description:     读取数据的回调
  Input:            buf 存放数据缓冲区
                    offset  数据偏移
                    len     数据长度
                    
  Return:           读到的数据长度
*/
int divoom_gif_ctrl_get_local_buffer_callback(uint8 *buf, int offset, int len)
{
	uint16 i = 0;
	uint32 off1 = 0, off2 = 0, add_len = 0;

	for (i = 0; i < gif_ctrl_handle->local_image_ctrl.local_image_pack_index; i++)
	{
		if (offset >= gif_ctrl_handle->local_image_ctrl.local_image_buf[i].offset && offset < gif_ctrl_handle->local_image_ctrl.local_image_buf[i].offset + 
			gif_ctrl_handle->local_image_ctrl.local_image_buf[i].pic_len) //判断当前offset是否在这个包之内
		{
			if (offset + len <= gif_ctrl_handle->local_image_ctrl.local_image_buf[i].offset + gif_ctrl_handle->local_image_ctrl.local_image_buf[i].pic_len)//获取的数据长度完全存在于当前包
			{
				off1 = offset - gif_ctrl_handle->local_image_ctrl.local_image_buf[i].offset; //在本包内的偏移
				memcpy(&buf[off2], &gif_ctrl_handle->local_image_ctrl.local_image_buf[i].pic_data[off1], len); //拷贝数据到buf
				off2 += len;
				return off2;
			}
			else //只有部分存在于当前包
			{
				off1 = offset - gif_ctrl_handle->local_image_ctrl.local_image_buf[i].offset; //在本包内的偏移
				add_len = gif_ctrl_handle->local_image_ctrl.local_image_buf[i].pic_len - off1;
				memcpy(&buf[off2], &gif_ctrl_handle->local_image_ctrl.local_image_buf[i].pic_data[off1], add_len); //拷贝部分数据到buf
				off2 += add_len; //已经拷贝了这么多数据
				offset += add_len; //偏移也增加相应大小
				len -= add_len; //长度要减少相应大小
			}
		}
	}
	return 0;
}

/*
  Function:       divoom_gif_ctrl_get_image_handle_from_http_buffer
  Description:	  从ＨＴＴＰ的缓冲中得到播放的句柄
  Input:          NONE
  Return:         图像句柄
*/

Pdivoom_http_download_image_file_info divoom_gif_ctrl_get_image_handle_from_http_buffer(int pic_num)
{
    int i, pic_size;
    uint32 image_off = 0;
	Pdivoom_http_download_image_file_info pimage_handle = NULL;
    Pdivoom_http_download_image_file_item pimage_item;
	Pdivoom_local_image_info image_info = NULL;

    pimage_handle = divoom_system_malloc(sizeof(divoom_http_download_image_file_info) + sizeof(Pdivoom_http_download_image_file_item *) * pic_num);
    if(pimage_handle == NULL)
    {
        return NULL;
    }
    pic_size = gif_ctrl_handle->local_image_ctrl.image_width *
        gif_ctrl_handle->local_image_ctrl.image_width *
        (int)DIVOOM_APP_RGB565_BYTES_PER_PX;
    pimage_handle->pimage_table = (Pdivoom_http_download_image_file_item *)&pimage_handle[1];
    pimage_handle->font_flag = 0;
    pimage_handle->width = gif_ctrl_handle->local_image_ctrl.image_width;
    pimage_handle->speed = gif_ctrl_handle->local_image_ctrl.image_speed;
    pimage_handle->num = 0;
    pimage_handle->pfont_data = NULL;
    for(i = 0; i < pic_num; i++)
    {
        image_info = &gif_ctrl_handle->local_image_ctrl.local_image_buf[i];
        if(image_info == NULL)
        {
            break;
        }
        pimage_item = divoom_system_malloc(sizeof(divoom_http_download_image_file_item) + pic_size);
        if(pimage_item == NULL)
        {
            break;
        }
        pimage_item->image_data = (uint8 *)&pimage_item[1];
        memcpy(pimage_item->image_data, image_info->pic_data, pic_size);
        pimage_item->image_type = DIVOOM_IMAGE_BUF_TYPE_RGB565_RAW;
        pimage_item->image_size = pic_size;
        pimage_handle->pimage_table[i] = pimage_item;
        pimage_handle->num ++;
        pimage_handle->cache_szie += image_info->pic_len;
        image_off += image_info->pic_len;
    }


    return pimage_handle;
}

#endif
void divoom_gif_ctrl_aes_decode_file(char *file_name)
{
    int file_len = 0, read_len = 0;
    uint8 *buffer;
    FILE *file_handle;

    buffer = divoom_app_com_read_file(file_name, &file_len);
    if(buffer == NULL)
    {
        return;
    }

    
    switch(buffer[0])
    {   
        case DIVOOM_NET_FILE_TYPE_SINGLE_16:
        case DIVOOM_NET_FILE_TYPE_SINGLE_AES:
            read_len = 1;
            break;

        case DIVOOM_NET_FILE_TYPE_GIF_16:
        case DIVOOM_NET_FILE_TYPE_GIF_AES:
            read_len = 4;
            break;
         
        case DIVOOM_NET_FILE_TYPE_PIC_SCAN:
            read_len = 4;
            break;
            
             
        case DIVOOM_NET_FILE_TYPE_NEW_MUL_SINGLE:
            read_len = 7;
            break;
            
        case DIVOOM_NET_FILE_TYPE_NEW_MUL_GIF:
            read_len = 6;
            break;                
    }
    divoom_aes_do(&buffer[read_len], file_len - read_len);
    divoom_app_com_write_file(file_name, buffer, file_len);
    divoom_system_free(buffer);
}

/* 下载线程只上报字节进度，LVGL 对象仍由表盘线程轮询并更新。 */
static void divoom_gif_ctrl_http_download_progress(void *user_data,
                                                   uint32 read_len,
                                                   uint32 total_len)
{
    divoom_http_server_transfer_progress_update(user_data, read_len, total_len);
}

static int divoom_gif_ctrl_push_http_local_image_event(
    const divoom_draw_remote_json_image_info *remote_info,
    const char *image_stage_name)
{
    Pdivoom_draw_remote_json_image_info event_info;
    divoom_sys_msg_info model_msg = {0};

    event_info = divoom_system_malloc(sizeof(divoom_draw_remote_json_image_info));
    if(event_info == NULL)
    {
        return 0;
    }
    memcpy(event_info, remote_info, sizeof(divoom_draw_remote_json_image_info));
    strcpy(event_info->file_id, image_stage_name);
    event_info->voice_id[0] = 0;

    /* 复用 PixelLocalHttp 的版本切换、旧文件延迟回收和播放入口。 */
    model_msg.msg_type = 1;
    model_msg.dest_id = DIVOOM_MODEL_LIGHT_APP;
    model_msg.msg_id = DIVOOM_LIGHT_APP_EVENT_APP_HTTP_LOCAL_IMAG;
    model_msg.data = (uint8 *)event_info;
    divoom_sys_model_crl_msg_push(&model_msg);
    return 1;
}

/*
  Function:       divoom_gif_ctrl_wite_task
  Description:	  利用线程处理队列
  Input:          NONE
  Return:         NONE
  Author:         wangft
  Data:           2020-12-18
*/
void *divoom_gif_ctrl_wite_task(void *param)
{
    char image_stage_name[64];
    char image_stage_file[128];

    while(1)
    {
        divoom_system_get_sem(gif_ctrl_handle->local_image_ctrl.local_image_sem);
        if(strlen(gif_ctrl_handle->remote_addr))
        {
            int32 download_flag;
            int32 event_push_flag = 0;
            int32 file_size = 0;

		    divoom_system_get_lock(gif_ctrl_handle->local_image_ctrl.local_image_mutex);
            gif_ctrl_handle->remote_stage_generation++;
            if(gif_ctrl_handle->remote_stage_generation == 0)
            {
                gif_ctrl_handle->remote_stage_generation++;
            }
            snprintf(image_stage_name, sizeof(image_stage_name),
                DIVOOM_GIF_CTRL_HTTP_IMAGE_STAGE_FORMAT,
                gif_ctrl_handle->remote_stage_generation);
            snprintf(image_stage_file, sizeof(image_stage_file), "%s/%s",
                DIVOOM_SD_APP_PIC_DIR, image_stage_name);
            divoom_app_com_mk_dir(DIVOOM_SD_APP_PIC_DIR);
            divoom_app_com_del_file(image_stage_file);

            /* 暂存文件不触碰当前解码句柄，网络下载期间无需占用 GIF 文件锁。 */
            divoom_http_server_transfer_progress_begin(gif_ctrl_handle);
            divoom_main_push_model_event_id(DIVOOM_MODEL_LIGHT_APP,
                DIVOOM_LIGHT_APP_EVENT_APP_SEND_PHOTO);
            download_flag = divoom_http_client_down_file_no_delay(
                image_stage_file, gif_ctrl_handle->remote_addr, 1, &file_size,
                divoom_gif_ctrl_http_download_progress, gif_ctrl_handle);
            divoom_http_server_transfer_progress_finish(gif_ctrl_handle);
            if(download_flag)
            {
                event_push_flag = divoom_gif_ctrl_push_http_local_image_event(
                    &gif_ctrl_handle->remote_image_info, image_stage_name);
            }
            if(gif_ctrl_handle->remote_flag)
            {
                divoom_net_app_send_packet(DIVOOM_NET_COMM_DEVICE_LOCAL_IMAGE_DEL, DIVOOM_SERVER_TOPIC, gif_ctrl_handle->remote_addr);
                gif_ctrl_handle->remote_flag = 0;
            }
            gif_ctrl_handle->remote_addr[0] = 0;
            gif_ctrl_handle->voice_addr[0] = 0;
            if(download_flag == 0 || event_push_flag == 0)
            {
                /* 失败暂存文件不能被后续事件误认成完整资源。 */
                divoom_app_com_del_file(image_stage_file);
            }
	    	divoom_system_free_lock(gif_ctrl_handle->local_image_ctrl.local_image_mutex);
        }
    }
    return  NULL;
}

/*
  Function:       divoom_gif_ctrl_clear_cache
  Description:	  清除ＧＩＦ的缓冲
  Input:          NONE
  Return:         NONE
*/
void divoom_gif_ctrl_clear_cache(void)
{
    return;
}

/*
  Function:       divoom_gif_ctrl_init
  Description:	  对要显示的gif进行管理
  Input:          NONE
  Return:         NONE
  Author:         wangft
  Data:           2020-12-18
*/
void divoom_gif_ctrl_init(void)
{
	if (gif_ctrl_handle)
		return;

	gif_ctrl_handle = divoom_system_malloc(sizeof(divoom_gif_ctrl_info));
	memset(gif_ctrl_handle, 0, sizeof(divoom_gif_ctrl_info));
    gif_ctrl_handle->accept_image_id = 1;
	gif_ctrl_handle->local_image_ctrl.local_image_queue = divoom_queue_init(200, __func__, __LINE__);
	gif_ctrl_handle->local_image_ctrl.local_image_mutex = divoom_system_create_lock();
	gif_ctrl_handle->local_image_ctrl.local_image_sem = divoom_system_create_sem(1, 0);
    gif_ctrl_handle->local_image_ctrl.gif_type = 0xff;
	divoom_system_create_task(divoom_gif_ctrl_wite_task, "gif_write", NULL, DIVOOM_TASK_STACK_GIF_WRITE);
}
