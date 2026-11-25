#include "divoom.h"
#include "divoom_pixel_format.h"

#if 0
#define DIVOOM_PLAY_GIF_COM_PLAY_TIME 3000
#define DIVOOM_PLAY_GIF_START_GIF_DISP_TIME    2000   //有启动画面时，停留的时间
#define DIVOOM_PLAY_GIF_CACHE_MAX_RAM_SIZE      (1024*1024)    //最大缓冲多少张图片
#define DIVOOM_PLAY_GIF_MAX_PIC_NUM     150


typedef struct _divoom_play_gif_com_image_info 
{
    divoom_image_info image_info;   //图像坐标相关信息
    void *image_data;               //图像数据信息
}divoom_play_gif_com_image_info, *Pdivoom_play_gif_com_image_info;

typedef struct _divoom_play_gif_com_image_queue
{
    int play_end_flag;              //动画播放标记
    int play_end_time;              //动画播放结束时间
    divoom_image_info image_info;   //图像坐标相关信息
    void *id_queue;
    DIVOOM_PLAY_GIF_COM_GET_NEW_ID pcallback;
}divoom_play_gif_com_image_queue, *Pdivoom_play_gif_com_image_queue;


typedef struct _divoom_play_gif_com_image_ctrl 
{
    uint8 run_flag;
    uint8 wait_flag;
    uint8 send_flag;
    uint8 read_flag;
    uint8 exit_flag;
    uint8 no_pic_flag;
    uint8 no_free_cache;
    uint8 time_frame_flag;  //是否增加时间显示
    uint8 encode_flag;
    int   pre_disp_time_info;   //前一次显示时间的时间
    void  *task_handle;
    void  *read_handle;
    int play_tol_time;          //每个动画播放的时间
    int play_end_time;     
    uint32 insert_time;        //播放多久将插入一个时间
    uint32 next_disp_time;
    uint32 cache_pic_num;       //缓冲了多少个图片
    int model_id;
    int disp_pic_id;
    int default_pic_id;         //开始启动时播放的图片
    int cur_play_image_id;
    int pre_read_image_id;
    uint8 tmp_rgb_buf[SCREEN_BUF_SIZE_128];
    void *image_area_queue;
    void *image_cache_queue;
    void *read_sem;
    void *lock_handle;
    void *read_lock_handle;
    struct tm pre_time_disp_date;
    divoom_image_info pic_time_frame;
    divoom_image_info pic_table_frame[DIVOOM_PLAY_GIF_MAX_PIC_NUM];
    DIVOOM_DISP_CTRL_OVER_CALL_END *pover_single_callback;
    DIVOOM_DISP_CTRL_OVER_CALL_END *pover_part_callback;
    Pdivoom_http_download_image_file_info pcur_disp_image_handle;       //当前显示的大背景图
}divoom_play_gif_com_image_ctrl, *Pdivoom_play_gif_com_image_ctrl;


static Pdivoom_play_gif_com_image_ctrl pdivoom_play_gif_com_handle = NULL;

void divoom_play_gif_com_add_time_frame(void);

int divoom_play_gif_com_find_image_area(void *item1, void *item2)
{
    int *image_id = (int *)item1;
    Pdivoom_play_gif_com_image_queue pcache_item = (Pdivoom_play_gif_com_image_queue)item2;

    return *image_id != pcache_item->image_info.gif_id;
    
}

int divoom_play_gif_com_find_play_over_area(void *item1, void *item2)
{
    Pdivoom_play_gif_com_image_queue pcache_item = (Pdivoom_play_gif_com_image_queue)item2;

    return 1 != pcache_item->play_end_flag;
    
}

void divoom_play_gif_com_play_over_callback(void *para, uint8 area_id)
{
    int image_area_id = area_id;
    Pdivoom_play_gif_com_image_queue pcache_item;

    pcache_item = divoom_queue_get_cmp(pdivoom_play_gif_com_handle->image_area_queue, &image_area_id, divoom_play_gif_com_find_image_area);
    if(pcache_item)
    {
        //divoom_print("play over: %d, time: %d, %d\n", area_id, pcache_item->play_end_time, divoom_system_get_tickms());
        if(pcache_item->play_end_time < divoom_system_get_tickms())
        {
            if(divoom_queue_items(pdivoom_play_gif_com_handle->image_cache_queue))
            {
                divoom_image_with_text_disp_del_back_image();
            }
            pcache_item->play_end_flag = 1;
            if(pdivoom_play_gif_com_handle->pover_single_callback)
            {
                pdivoom_play_gif_com_handle->pover_single_callback(area_id);
            }
        }
        
    }
}


void divoom_play_gif_com_play_part_over_callback(void *para, uint8 area_id)
{
    if(pdivoom_play_gif_com_handle->pover_part_callback)
    {
        pdivoom_play_gif_com_handle->pover_part_callback(area_id);
    }
}
void divoom_play_gif_com_decode_rgb_buf(Pdivoom_image_info pimage_info, T_U8 *rgb_buf)
{
    uint8 *decode_buf;
    
    if(pdivoom_play_gif_com_handle)
    {
        if(pimage_info->image_type == DIVOOM_IMAGE_BUF_TYPE_RGB_RAW)
        {
            memcpy(rgb_buf, pimage_info->rgb_data, pimage_info->pic_width * pimage_info->pic_width * 3);
        }
        else if(pimage_info->image_type == DIVOOM_IMAGE_BUF_TYPE_MINILZO_ENCODE)
        {
            uint32 len;

            decode_buf = pdivoom_play_gif_com_handle->tmp_rgb_buf;
            divoom_minilzo_decompress(pimage_info->rgb_data, pimage_info->image_size, decode_buf, &len);
            divoom_play_com_convert_rgb_net_to_local(pimage_info->pic_width, decode_buf, rgb_buf); 
        }
        else if(pimage_info->image_type == DIVOOM_IMAGE_BUF_TYPE_DIVOOM_ENCODE)
        {
            decode_buf = pdivoom_play_gif_com_handle->tmp_rgb_buf;
            divoom_image_decode_Iframe(pimage_info->rgb_data, decode_buf, pimage_info->pic_width);                
            divoom_play_com_convert_rgb_net_to_local(pimage_info->pic_width, decode_buf, rgb_buf);                
        }
        else if(pimage_info->image_type == DIVOOM_IMAGE_BUF_TYPE_JPEG_ENCODE)
        {
            /* JPG 解码层禁止再输出 RGB888；该路径应切换到 RGB565 缓冲。 */
            return;
		}
        else
        {
            return;
        }
    }
}

/*********************************************************
  Function:		divoom_play_gif_com_send_part_image
  Description:	将网络上的ＲＧＢ部分图做为局部发送
  Input:			pimage_base           
                    pimage_handle     
                    flag        1要保持底色 0不保证保持底色
                    start       起始帧
                    num         总帧数，如果是-１，就是全部
  Return:			none
************************************************************/

void divoom_play_gif_com_send_part_image(Pdivoom_image_info pimage_base, Pdivoom_http_download_image_file_info pimage_handle, uint8 flag, int start, int num)
{
    uint8 *encode_buf = NULL, *local_buf = NULL;
    int i, j, pic_size;
    uint32 len;
    Pdivoom_image_info pimage_info; 

    if(pimage_base->pic_high != pimage_handle->width)
    {
        encode_buf = divoom_system_malloc(SCREEN_BUF_SIZE_64*2);
        local_buf = &encode_buf[SCREEN_BUF_SIZE_64];
    }
    pic_size = pimage_base->pic_high * pimage_base->pic_width * 3;   

    if(num == -1)
    {
        num = pimage_handle->num;
    }
    divoom_image_with_text_ctrl_lock(1);
    for(i = start; i < pimage_handle->num && i < start + num; i ++)
    {
        pimage_info = divoom_system_malloc(sizeof(divoom_image_info));
        if(pimage_info == NULL)
        {
            break;
        }
        memcpy(pimage_info, pimage_base, sizeof(divoom_image_info));
        if(i == start)
        {
            pimage_info->frame_attr = DIVOOM_IMAGE_START_TYPE; 
        }
        else if(i == start + num - 1)
        {
            pimage_info->frame_attr = DIVOOM_IMAGE_END_TYPE; 
        }
        else
        {
    	    pimage_info->frame_attr = DIVOOM_IMAGE_PIC_TYPE;
        }
    	pimage_info->speed = pimage_handle->speed;	

		if (pimage_base->pic_high > pimage_handle->width || pimage_base->pic_width > pimage_handle->width)
		{
			pimage_info->large_flag = 1;
			pimage_info->image_width = pimage_handle->width;
			pimage_info->image_type = DIVOOM_IMAGE_BUF_TYPE_RGB_RAW;
			pimage_info->rgb_data = divoom_system_malloc(pimage_handle->width*pimage_handle->width*3); //保存实际图片大小
			if(pimage_info->rgb_data == NULL)
			{
			    divoom_system_free(pimage_info);
			    break;
			}

			if(pimage_handle->pimage_table[i]->image_type == DIVOOM_IMAGE_BUF_TYPE_RGB_RAW)
            {
        	    memcpy(pimage_info->rgb_data, pimage_handle->pimage_table[i]->image_data, pimage_handle->width*pimage_handle->width*3); //图片裸数据
            }
            else if(pimage_handle->pimage_table[i]->image_type == DIVOOM_IMAGE_BUF_TYPE_DIVOOM_ENCODE)
            {
                divoom_image_decode_Iframe(pimage_handle->pimage_table[i]->image_data, pimage_info->rgb_data, pimage_handle->width);                
            }
            else
            {
                len = pimage_handle->pimage_table[i]->image_size;
                divoom_minilzo_decompress(pimage_handle->pimage_table[i]->image_data, pimage_handle->pimage_table[i]->image_size,encode_buf, &len);
                divoom_play_com_convert_rgb_net_to_local(pimage_handle->width, encode_buf, pimage_info->rgb_data);                
            }
		}
		else
		{
        	if(pimage_base->pic_high != pimage_handle->width)
	        {
	            pimage_info->image_type = DIVOOM_IMAGE_BUF_TYPE_RGB_RAW;
	            pimage_info->rgb_data = divoom_system_malloc(pic_size);
	            if(pimage_info->rgb_data == NULL)
	            {
	                divoom_system_free(pimage_info);
	                break;
	            }
	            if(pimage_handle->pimage_table[i]->image_type == DIVOOM_IMAGE_BUF_TYPE_RGB_RAW)
	            {
	                local_buf = pimage_handle->pimage_table[i]->image_data;
	    //    	    memcpy(pimage_info->rgb_data, pimage_handle->pimage_table[i]->image_data, pic_size); //图片裸数据
	            }
	            else if(pimage_handle->pimage_table[i]->image_type == DIVOOM_IMAGE_BUF_TYPE_DIVOOM_ENCODE)
	            {
	                divoom_image_decode_Iframe(pimage_handle->pimage_table[i]->image_data, local_buf, pimage_handle->width);                
	            }
	            else
	            {
	                len = pimage_handle->pimage_table[i]->image_size;
	                divoom_minilzo_decompress(pimage_handle->pimage_table[i]->image_data, pimage_handle->pimage_table[i]->image_size,encode_buf, &len);
	                divoom_play_com_convert_rgb_net_to_local(pimage_handle->width, encode_buf, local_buf);                
	            }
	            for(j = 0; j < pimage_base->pic_high; j ++)
	            {
	                memcpy(&pimage_info->rgb_data[j * pimage_base->pic_width * 3], &local_buf[j * pimage_handle->width * 3], pimage_base->pic_width * 3); //图片裸数据
	            }
	            pimage_info->image_decode_callback = NULL;
	        }
        	else
	        {
	            if(pimage_handle->pimage_table[i]->image_type != DIVOOM_IMAGE_BUF_TYPE_RGB_RAW)
	            {
	                pimage_info->image_decode_callback = divoom_play_gif_com_decode_rgb_buf;
	                pic_size = pimage_handle->pimage_table[i]->image_size;   
	            }
	            else
	            {
	                pimage_info->image_decode_callback = NULL;
	                pic_size = pimage_base->pic_high * pimage_base->pic_width * 3;   
	            }
	            pimage_info->rgb_data = divoom_system_malloc(pic_size);
	            if(pimage_info->rgb_data == NULL)
	            {
	                divoom_system_free(pimage_info);
	                break;
	            }
	            pimage_info->image_type = pimage_handle->pimage_table[i]->image_type;
	            pimage_info->image_size = pimage_handle->pimage_table[i]->image_size;
	            memcpy(pimage_info->rgb_data, pimage_handle->pimage_table[i]->image_data, pic_size); //图片裸数据
	        }
		}
    	pimage_info->fill_flag = flag;
    	divoom_image_with_text_disp_write_image_no_lock(pimage_info);
    } 
    divoom_image_with_text_ctrl_lock(0);
    if(encode_buf)
    {
        divoom_system_free(encode_buf);
    }
    
}

/*********************************************************
  Function:		divoom_play_gif_com_set_part_image_callback
  Description:	设置局部图的回调函数
  Input:			id           
                    pimage_handle     
                    pover_callback
  Return:			none
************************************************************/

void divoom_play_gif_com_set_part_image_callback(int id, DIVOOM_DISP_CTRL_OVER_CALL_END *pover_callback)
{

    if(pdivoom_play_gif_com_handle)
    {
        pdivoom_play_gif_com_handle->pover_part_callback = pover_callback;
        if(pover_callback)
        {
            divoom_image_with_text_disp_set_callback(id, divoom_play_gif_com_play_part_over_callback, NULL);
        }
        else
        {
            divoom_image_with_text_disp_set_callback(id, NULL, NULL);
        }
    }
}

/*********************************************************
  Function:		divoom_play_gif_com_send_part_image_list
  Description:	将多组网络上的ＲＧＢ部分图做为局部发送
  Input:			pimage_base           
                    pimage_handle     
                    flag        1要保持底色 0不保证保持底色
                    start_list       起始帧列表
                    num         总帧数，如果是-１，就是全部
                    list_num    总共有多少组
  Return:			none
************************************************************/

void divoom_play_gif_com_send_part_image_list(Pdivoom_image_info pimage_base, Pdivoom_http_download_image_file_info pimage_handle, uint8 flag, int *start_list, int num, int list_num)
{
    uint8 *encode_buf = NULL, *local_buf = NULL;
    int i, j, k, pic_size, start;
    uint32 len;
    Pdivoom_image_info pimage_info; 
    
    pic_size = pimage_base->pic_high * pimage_base->pic_width * 3;   

    if(pimage_base->pic_high != pimage_handle->width)
    {
        encode_buf = divoom_system_malloc(SCREEN_BUF_SIZE_64*2);
        local_buf = &encode_buf[SCREEN_BUF_SIZE_64];
    }

    if(num == -1)
    {
        num = pimage_handle->num;
    }
    divoom_image_with_text_ctrl_lock(1);
    for(k = 0; k < list_num; k ++)
    {
        start = start_list[k];
        for(i = start; i < pimage_handle->num && i < start + num; i ++)
        {
            pimage_info = divoom_system_malloc(sizeof(divoom_image_info));
            if(pimage_info == NULL)
            {
                break;
            }
            memcpy(pimage_info, pimage_base, sizeof(divoom_image_info));
            if(i == start && k == 0)
            {
                pimage_info->frame_attr = DIVOOM_IMAGE_START_TYPE; 
            }
            else
            {
        	    pimage_info->frame_attr = DIVOOM_IMAGE_PIC_TYPE;
            }
        	pimage_info->speed = pimage_handle->speed;	
            if(pimage_base->pic_high != pimage_handle->width)
            {
                pimage_info->image_type = DIVOOM_IMAGE_BUF_TYPE_RGB_RAW;
                pimage_info->rgb_data = divoom_system_malloc(pic_size);
                if(pimage_info->rgb_data == NULL)
                {
                    divoom_system_free(pimage_info);
                    break;
                }
                if(pimage_handle->pimage_table[i]->image_type == DIVOOM_IMAGE_BUF_TYPE_RGB_RAW)
                {
                    local_buf = pimage_handle->pimage_table[i]->image_data;
        //    	    memcpy(pimage_info->rgb_data, pimage_handle->pimage_table[i]->image_data, pic_size); //图片裸数据
                }
                else if(pimage_handle->pimage_table[i]->image_type == DIVOOM_IMAGE_BUF_TYPE_DIVOOM_ENCODE)
                {
                    divoom_image_decode_Iframe(pimage_handle->pimage_table[i]->image_data, encode_buf, pimage_handle->width);                
                    divoom_play_com_convert_rgb_net_to_local(pimage_handle->width, encode_buf, local_buf);                
                }
                else
                {
                    len = pimage_handle->pimage_table[i]->image_size;
                    divoom_minilzo_decompress(pimage_handle->pimage_table[i]->image_data, pimage_handle->pimage_table[i]->image_size,encode_buf, &len);
                    divoom_play_com_convert_rgb_net_to_local(pimage_handle->width, encode_buf, local_buf);                
                }
                for(j = 0; j < pimage_base->pic_high; j ++)
                {
                    memcpy(&pimage_info->rgb_data[j * pimage_base->pic_width * 3], &local_buf[j * pimage_handle->width * 3], pimage_base->pic_width * 3); //图片裸数据
                }
                pimage_info->image_decode_callback = NULL;
            }
            else
            {
                if(pimage_handle->pimage_table[i]->image_type != DIVOOM_IMAGE_BUF_TYPE_RGB_RAW)
                {
                    pimage_info->image_decode_callback = divoom_play_gif_com_decode_rgb_buf;
                    pic_size = pimage_handle->pimage_table[i]->image_size;
                }
                else
                {
                    pic_size = pimage_handle->width * pimage_handle->width * 3;
                    pimage_info->image_decode_callback = NULL;
                }
                pimage_info->rgb_data = divoom_system_malloc(pic_size);
                if(pimage_info->rgb_data == NULL)
                {
                    divoom_system_free(pimage_info);
                    break;
                }
                pimage_info->image_type = pimage_handle->pimage_table[i]->image_type;
                pimage_info->image_size = pic_size;
                memcpy(pimage_info->rgb_data, pimage_handle->pimage_table[i]->image_data, pic_size); //图片裸数据
            }
            pimage_info->fill_flag = flag;
            divoom_image_with_text_disp_write_image_no_lock(pimage_info);
        } 
    }
    divoom_image_with_text_ctrl_lock(0);
    if(encode_buf)
    {
        divoom_system_free(encode_buf);
    }
    
}

/*********************************************************
  Function:		divoom_play_gif_com_get_time_frame
  Description:	增加时间帧显示
  Input:			handle
  Return:			none
************************************************************/


#define DIVOOM_MUL_GIF_CTRL_TIME_FRAME_LEN  (2 * DIVOOM_IMAGE_ENCODE_COLOR_DATA_32_OFF + 10 + DIVOOM_IMAGE_MAX_DOT_64/8)
#define DIVOOM_MUL_GIF_LED_POINT_X					(3)
#define DIVOOM_MUL_GIF_LED_POINT_1_Y				(21)
#define DIVOOM_MUL_GIF_LED_POINT_2_Y				(25)

#define DIVOOM_MUL_GIF_LED_X_1						(7)
#define DIVOOM_MUL_GIF_LED_X_2						(17)
#define DIVOOM_MUL_GIF_LED_HOUR_Y					(1)
#define DIVOOM_MUL_GIF_LED_MIN_Y					(17)

T_VOID divoom_play_gif_com_get_time_frame(uint8 *encode_buf)
{
    #if 0
    T_U16 i, off;
    T_U32 hour, min;
    divoom_light_color_info color = {0xFF, 0xFF, 0};

	struct tm time_now;    
	time_t cur_time;

    time(&cur_time);
	localtime_r(&cur_time, &time_now);


    memset(encode_buf, 0, SCREEN_BUF_SIZE_64);
	
	hour = time_now.tm_hour;
    if(time_now.tm_hour != 12)
    {
        hour = time_now.tm_hour % 12;
        if(hour == 0)
        {
            hour = 12;
        }
    }

	min = time_now.tm_min;

	divoom_disp_digit_fill_area(DIVOOM_MUL_GIF_LED_X_1, DIVOOM_MUL_GIF_LED_HOUR_Y, hour/10, 
			DIVOOM_8X14_BIG_DIGIT, encode_buf, &color);
    
	divoom_disp_digit_fill_area(DIVOOM_MUL_GIF_LED_X_2, DIVOOM_MUL_GIF_LED_HOUR_Y, hour%10, 
			DIVOOM_8X14_BIG_DIGIT, encode_buf, &color);
    
	divoom_disp_digit_fill_area(DIVOOM_MUL_GIF_LED_X_1, DIVOOM_MUL_GIF_LED_MIN_Y, min/10, 
			DIVOOM_8X14_BIG_DIGIT, encode_buf, &color);
    
	divoom_disp_digit_fill_area(DIVOOM_MUL_GIF_LED_X_2, DIVOOM_MUL_GIF_LED_MIN_Y, min%10, 
			DIVOOM_8X14_BIG_DIGIT, encode_buf, &color);
    
    off = DIVOOM_MUL_GIF_LED_POINT_X + DIVOOM_MUL_GIF_LED_POINT_1_Y * SCREEN_WIDTH_32;
    for(i = 0; i < 2; i++)
    {
        divoom_disp_ctrl_set_data(off, &color, encode_buf);
        divoom_disp_ctrl_set_data(off + 1, &color, encode_buf);  
        off += SCREEN_WIDTH_32;
    }
    off = DIVOOM_MUL_GIF_LED_POINT_X + DIVOOM_MUL_GIF_LED_POINT_2_Y * SCREEN_WIDTH_32;
    for(i = 0; i < 2; i++)
    {
        divoom_disp_ctrl_set_data(off, &color, encode_buf);
        divoom_disp_ctrl_set_data(off + 1, &color, encode_buf);
        off += SCREEN_WIDTH_32;
    }
    #endif
}

T_VOID divoom_play_gif_com_get_font_info(Pdivoom_image_word_font_info word_info)
{
    int i;

    if(pdivoom_play_gif_com_handle == NULL)
    {
        return;
    }
    
    //divoom_system_get_lock(pdivoom_play_gif_com_handle->lock_handle);
    if(pdivoom_play_gif_com_handle->pcur_disp_image_handle == NULL || pdivoom_play_gif_com_handle->pcur_disp_image_handle->pfont_data == NULL)
    {
        //divoom_system_free_lock(pdivoom_play_gif_com_handle->lock_handle);
        return;
    }
    word_info->font_flag = 0;
    for(i = 0; i < pdivoom_play_gif_com_handle->pcur_disp_image_handle->pfont_data->font_num; i++)
    {
        if(pdivoom_play_gif_com_handle->pcur_disp_image_handle->pfont_data->font_data[i].unicode16 == word_info->unicode16)
        {
            memcpy(word_info->font_info, pdivoom_play_gif_com_handle->pcur_disp_image_handle->pfont_data->font_data[i].font_info, 32);
            break;
        }
    }
    if(i == pdivoom_play_gif_com_handle->pcur_disp_image_handle->pfont_data->font_num)
    {
        memset(word_info->font_info, 0, 32);
    }
    
    //divoom_system_free_lock(pdivoom_play_gif_com_handle->lock_handle);
}

void divoom_play_gif_com_send_font_info(Pdivoom_http_download_image_font_data pfont_data)
{
    if(pfont_data)
    {
        //有文字的话，将发送文字信息
        Pdivoom_text_info ptext_info;
        
        ptext_info = divoom_system_malloc(sizeof(divoom_text_info));
        if(ptext_info)
        {
        	//divoom_print("aj test font!\n");
            ptext_info->x = pfont_data->start_x;        //字段的坐标位置
            ptext_info->y = pfont_data->start_y;        //字段的坐标位置
            ptext_info->size = pfont_data->size;     //字体大小
            ptext_info->width = pfont_data->frameWidth;    //显示框的宽度
            ptext_info->high = pfont_data->frameHeight;    //显示框的宽度
        	ptext_info->word_effct_type = DIVOOM_WORD_EFFECT_NORM; //正常、单行固定、多行滚动
            ptext_info->font_effect = DIVOOM_TEXT_EFFECT_LEFT; //上下左右滚动方式
            ptext_info->speed = pfont_data->speed;   //字体的速度
            memcpy(ptext_info->color ,pfont_data->color, 3); //字体的颜色ＲＧＢ
          	ptext_info->font_id  = 1;	//是属于哪个字体框的
            ptext_info->unicode_len = pfont_data->unicode_len; //要显示的字串的长度unicode_str有效字符的长度
            memcpy(ptext_info->unicode_str, pfont_data->unicode_str, pfont_data->unicode_len * 2); //要显示的字串
        	ptext_info->space_dot_num = 0; //每个字之间空几格
        	ptext_info->roll_flag = 1; //该字符串是否滚动
        	//divoom_print("aj test font!\n");
        	ptext_info->get_font_info_callback = divoom_play_gif_com_get_font_info; 

            divoom_image_with_text_disp_write_text(ptext_info);
        }
    }
}
void divoom_play_gif_com_format_gif_table(Pdivoom_http_download_image_file_info pcur_disp_image_handle)
{
    int i;

    if(pcur_disp_image_handle == NULL)
    {
        return;
    }

    if(pdivoom_play_gif_com_handle->encode_flag == 0)
    {
        divoom_manage_image_format_gif_table(pcur_disp_image_handle, pdivoom_play_gif_com_handle->tmp_rgb_buf);
    }


    for(i = 0; i < pcur_disp_image_handle->num; i++)
    {              
    	pdivoom_play_gif_com_handle->pic_table_frame[i].gif_id = 0;	//0 表示背景图 局部图从1开始
    	pdivoom_play_gif_com_handle->pic_table_frame[i].speed = pcur_disp_image_handle->speed;	//动画速度
    	pdivoom_play_gif_com_handle->pic_table_frame[i].pos_x = 0;	//坐标x起始位置
    	pdivoom_play_gif_com_handle->pic_table_frame[i].pos_y = 0;  //坐标y起始位置
    	pdivoom_play_gif_com_handle->pic_table_frame[i].pic_width = pcur_disp_image_handle->width; //图片宽度
    	pdivoom_play_gif_com_handle->pic_table_frame[i].pic_high = pcur_disp_image_handle->width; //图片高度
    	pdivoom_play_gif_com_handle->pic_table_frame[i].layer = 0; //该图存在哪个层级
    	pdivoom_play_gif_com_handle->pic_table_frame[i].image_size = pcur_disp_image_handle->pimage_table[i]->image_size; //该图存在哪个层级
        pdivoom_play_gif_com_handle->pic_table_frame[i].rgb_data = pcur_disp_image_handle->pimage_table[i]->image_data;
        pdivoom_play_gif_com_handle->pic_table_frame[i].image_type = pcur_disp_image_handle->pimage_table[i]->image_type;
        if(pdivoom_play_gif_com_handle->pic_table_frame[i].image_type == DIVOOM_IMAGE_BUF_TYPE_RGB_RAW)
        {
            pdivoom_play_gif_com_handle->pic_table_frame[i].image_decode_callback = NULL;
        }
        else
        {
            pdivoom_play_gif_com_handle->pic_table_frame[i].image_decode_callback = divoom_play_gif_com_decode_rgb_buf;
        }
    }
}
/*********************************************************
  Function:		divoom_play_gif_com_task_run
  Description:	主循环
  Input:			none
  Return:			none
************************************************************/

void *divoom_play_gif_com_task_run(void *arg) //请求设备的命令
{
    Pdivoom_play_gif_image_info image_id;
    int i, tol, pic_size, disp_over_flag = 0, first_frame_flag;
    uint32 cur_time;
    Pdivoom_image_info pbase_image_info;
    Pdivoom_play_gif_com_image_queue pcache_item;
    Pdivoom_http_download_image_file_info psmall_image_handle;      
    
    while(pdivoom_play_gif_com_handle->run_flag)
    {
        disp_over_flag = 0;
        first_frame_flag = 0;
        divoom_play_gif_com_add_time_frame();
        if(pdivoom_play_gif_com_handle->wait_flag)
        {
            divoom_system_delay_ms(10);
            continue;
        }
        divoom_system_get_lock(pdivoom_play_gif_com_handle->lock_handle);    
        if(pdivoom_play_gif_com_handle->pcur_disp_image_handle == NULL)
        {
            //如果当前显示的背景为空，将加载背景
            pdivoom_play_gif_com_handle->send_flag = 0;
            pdivoom_play_gif_com_handle->pcur_disp_image_handle = divoom_queue_pop(pdivoom_play_gif_com_handle->image_cache_queue);
            if(pdivoom_play_gif_com_handle->pcur_disp_image_handle)
            {
                first_frame_flag = 1;
                divoom_image_with_text_disp_del_text_frame(1);
                divoom_play_gif_com_send_font_info(pdivoom_play_gif_com_handle->pcur_disp_image_handle->pfont_data);
                divoom_play_gif_com_format_gif_table(pdivoom_play_gif_com_handle->pcur_disp_image_handle);
            }
            if(pdivoom_play_gif_com_handle->read_flag)
            {
                divoom_system_put_sem(pdivoom_play_gif_com_handle->read_sem);
            }
            pdivoom_play_gif_com_handle->disp_pic_id = 0;
        }
        if(pdivoom_play_gif_com_handle->pcur_disp_image_handle == NULL)
        {
            //等文件读出来
            divoom_system_free_lock(pdivoom_play_gif_com_handle->lock_handle);    
            divoom_system_delay_ms(100);
            divoom_print("wait loading!\n");
            continue;
        }
        //检查是否有动画播放完成，开始加载另一个动画
        tol = divoom_queue_items(pdivoom_play_gif_com_handle->image_area_queue);
        for(i = 0; i < tol; i ++)
        {
            pcache_item = divoom_queue_get_index_item(pdivoom_play_gif_com_handle->image_area_queue, i);
            if(pcache_item && pcache_item->play_end_flag)
            {
                if(pcache_item->image_info.gif_id == 0 && divoom_image_with_text_disp_get_back_queue() < 4 && divoom_queue_items(pdivoom_play_gif_com_handle->image_cache_queue) == 0)
                {
                    //如果不够空间放下起始帧与时间帧的将下次再处理
                    disp_over_flag = 1;
                    continue;
                }
                //动画播放完成，将要重新加载这个区域的图片,背景图要分步加载，局部图要一次性加载
                pdivoom_play_gif_com_handle->pre_disp_time_info = 0;
                pcache_item->play_end_flag = 0;
                if(pcache_item->image_info.gif_id == 0)
                {
                    //从０开始发送
                    cur_time = divoom_system_get_tickms();
                    pdivoom_play_gif_com_handle->disp_pic_id = 0;
                    if(divoom_queue_items(pdivoom_play_gif_com_handle->image_cache_queue))
                    {
                        divoom_image_with_text_disp_del_text_frame(1);
                        if(pdivoom_play_gif_com_handle->pcur_disp_image_handle)
                        {
                            if(pdivoom_play_gif_com_handle->cache_pic_num >= pdivoom_play_gif_com_handle->pcur_disp_image_handle->num)
                            {
                                pdivoom_play_gif_com_handle->cache_pic_num -= pdivoom_play_gif_com_handle->pcur_disp_image_handle->num;
                            }
                            else
                            {
                                pdivoom_play_gif_com_handle->cache_pic_num = 0;
                            }
                            divoom_http_download_image_free_image_handle(pdivoom_play_gif_com_handle->pcur_disp_image_handle);
                            pdivoom_play_gif_com_handle->send_flag = 0;
                        }
                        //先发送个起始帧
                        first_frame_flag = 1;
                        pdivoom_play_gif_com_handle->pcur_disp_image_handle = divoom_queue_pop(pdivoom_play_gif_com_handle->image_cache_queue);                        
                        divoom_play_gif_com_format_gif_table(pdivoom_play_gif_com_handle->pcur_disp_image_handle);
                        pcache_item->play_end_time = cur_time + pdivoom_play_gif_com_handle->play_tol_time;
                        if(pdivoom_play_gif_com_handle->pcur_disp_image_handle && pdivoom_play_gif_com_handle->pcur_disp_image_handle->pfont_data)
                        {
                            divoom_play_gif_com_send_font_info(pdivoom_play_gif_com_handle->pcur_disp_image_handle->pfont_data);
                        }
                        divoom_system_put_sem(pdivoom_play_gif_com_handle->read_sem);
                    }
                    else
                    {
                        if(pdivoom_play_gif_com_handle->read_flag)
                        {
                            divoom_system_put_sem(pdivoom_play_gif_com_handle->read_sem);
                        }
                    }
                    if(cur_time > pdivoom_play_gif_com_handle->next_disp_time)
                    {
                        //我们将发送一个时间帧
                        
                        pbase_image_info = &pdivoom_play_gif_com_handle->pic_time_frame;
                    	pbase_image_info->gif_id = 0;	//0 表示背景图 局部图从1开始
                    	pbase_image_info->frame_attr = DIVOOM_IMAGE_START_TYPE; //数据包类型【起始帧、中间图片帧、结束帧】
                    	pbase_image_info->speed = DIVOOM_PLAY_GIF_START_GIF_DISP_TIME;	//动画速度
                    	pbase_image_info->pos_x = 0;	//坐标x起始位置
                    	pbase_image_info->pos_y = 0;  //坐标y起始位置
                    	pbase_image_info->pic_width = SCREEN_WIDTH_32; //图片宽度
                    	pbase_image_info->pic_high = SCREEN_WIDTH_32; //图片高度
                    	pbase_image_info->layer = 0; //该图存在哪个层级
                    	pbase_image_info->image_decode_callback = NULL;
                        divoom_play_gif_com_get_time_frame(pbase_image_info->rgb_data);
                        pbase_image_info->fill_flag = 0;
                        divoom_image_with_text_disp_write_image(pbase_image_info);
                        first_frame_flag = 0;
                        pdivoom_play_gif_com_handle->next_disp_time = cur_time + pdivoom_play_gif_com_handle->insert_time;
                    }
                }
                else
                {
                    image_id = divoom_queue_pop(pcache_item->id_queue);
                    if(image_id == NULL)
                    {
                        continue;
                    }
                    psmall_image_handle = divoom_manage_image_read(image_id->model_id, image_id->image_id, NULL, NULL);
                    psmall_image_handle->image_id = image_id->image_id;
                    divoom_play_gif_com_send_part_image(&pcache_item->image_info, psmall_image_handle,0, 0, -1);
                    divoom_http_download_image_free_image_handle(psmall_image_handle);
                    pcache_item->pcallback(pcache_item->image_info.gif_id, image_id);
                    divoom_queue_push(pcache_item->id_queue, image_id);
                }
            }
        }

        //将发送相关大图的图片
        if(pdivoom_play_gif_com_handle->pcur_disp_image_handle && disp_over_flag == 0
            && (pdivoom_play_gif_com_handle->send_flag == 0 || pdivoom_play_gif_com_handle->pcur_disp_image_handle->num >= 1))
        {
            if(divoom_image_with_text_disp_get_back_queue())
            {
                if(pdivoom_play_gif_com_handle->disp_pic_id >= pdivoom_play_gif_com_handle->pcur_disp_image_handle->num)
                {
                    pdivoom_play_gif_com_handle->disp_pic_id = 0;
                }
                
                pbase_image_info = &pdivoom_play_gif_com_handle->pic_table_frame[pdivoom_play_gif_com_handle->disp_pic_id]; 
                pbase_image_info->gif_id = 0;
                pbase_image_info->layer = 0;
                if(first_frame_flag)
                {
                    pbase_image_info->frame_attr = DIVOOM_IMAGE_START_TYPE; 
                }
            	else if(pdivoom_play_gif_com_handle->disp_pic_id == pdivoom_play_gif_com_handle->pcur_disp_image_handle->num - 1)
            	{
            	    pbase_image_info->frame_attr = DIVOOM_IMAGE_END_TYPE; //数据包类型【起始帧、中间图片帧、结束帧】
            	}
                else
                {
            	    pbase_image_info->frame_attr = DIVOOM_IMAGE_PIC_TYPE; //数据包类型【起始帧、中间图片帧、结束帧】
                }
                pbase_image_info->fill_flag = 0;
                divoom_image_with_text_disp_write_image(pbase_image_info);
                pdivoom_play_gif_com_handle->send_flag = 1;
                pdivoom_play_gif_com_handle->disp_pic_id  ++;
            }
        }
        
        if(pdivoom_play_gif_com_handle->no_pic_flag)
        {
            //达一张空的显示图
            
            pbase_image_info = &pdivoom_play_gif_com_handle->pic_time_frame;
        	pbase_image_info->gif_id = 0;	//0 表示背景图 局部图从1开始
        	pbase_image_info->frame_attr = DIVOOM_IMAGE_START_TYPE; //数据包类型【起始帧、中间图片帧、结束帧】
        	pbase_image_info->speed = 1000;	//动画速度
        	pbase_image_info->pos_x = 0;	//坐标x起始位置
        	pbase_image_info->pos_y = 0;  //坐标y起始位置
        	pbase_image_info->pic_width = SCREEN_WIDTH_64; //图片宽度
        	pbase_image_info->pic_high = SCREEN_WIDTH_64; //图片高度
        	pbase_image_info->layer = 0; //该图存在哪个层级
        	pic_size = pbase_image_info->pic_width * pbase_image_info->pic_width * 3;
        	pbase_image_info->rgb_data = divoom_system_malloc(pic_size); //图片裸数据
            pbase_image_info->image_decode_callback = NULL;
        	divoom_disp_picture_get_64rgb_data(DIVOOM_DISP_USER_DEFINE_TV_PIC, pbase_image_info->rgb_data);
            divoom_print("send default image!\n");
            pbase_image_info->fill_flag = 0;
            divoom_image_with_text_disp_write_image(pbase_image_info);
            pdivoom_play_gif_com_handle->no_pic_flag = 0;
        }
        divoom_system_free_lock(pdivoom_play_gif_com_handle->lock_handle);    

        divoom_system_delay_ms(40);
    }
    pdivoom_play_gif_com_handle->exit_flag |= 2;
    divoom_print("exit run!\n");
    return NULL;
    
}

int divoom_play_gif_com_end_read_task_callback(void)
{
    if(pdivoom_play_gif_com_handle)
    {
        return pdivoom_play_gif_com_handle->run_flag == 0;
    }
    return 0;
}


void *divoom_play_gif_com_task_read(void *arg) //请求设备的命令
{
    Pdivoom_play_gif_image_info image_id;
    int image_area_id;
    Pdivoom_play_gif_com_image_queue pcache_item = NULL;    
    Pdivoom_http_download_image_file_info pimage_cache_data;
    
    while(pdivoom_play_gif_com_handle->run_flag)
    {
        divoom_system_get_sem(pdivoom_play_gif_com_handle->read_sem);
        
        if(pdivoom_play_gif_com_handle->run_flag == 0)
        {
            break;
        }
        
        if(pdivoom_play_gif_com_handle->wait_flag)
        {
            divoom_system_delay_ms(10);
            continue;
        }
        
        image_area_id = 0;
        if(pcache_item == NULL)
        {
            pcache_item = divoom_queue_get_cmp(pdivoom_play_gif_com_handle->image_area_queue, &image_area_id, divoom_play_gif_com_find_image_area);
            if(pcache_item == NULL)
            {
                divoom_system_delay_ms(100);
                continue;
            }
        }
        if(pcache_item->pcallback == NULL)
        {
            divoom_system_delay_ms(100);
            continue;
        }
        while(pdivoom_play_gif_com_handle->run_flag && (divoom_queue_items(pdivoom_play_gif_com_handle->image_cache_queue) == 0))
        {
            divoom_system_get_lock(pdivoom_play_gif_com_handle->read_lock_handle);
            image_id = divoom_queue_pop(pcache_item->id_queue);
            if(image_id == NULL)
            {
                image_id = (divoom_play_gif_image_info *)divoom_system_malloc(sizeof(divoom_play_gif_image_info));
                pcache_item->pcallback(pcache_item->image_info.gif_id, image_id);
                if(pdivoom_play_gif_com_handle->cur_play_image_id == image_id->image_id)
                {
                    //divoom_print("pass same image!(%d)\n", image_id->image_id);
                    image_id->image_id = 0;
                }
                else
                {
                    pdivoom_play_gif_com_handle->cur_play_image_id = image_id->image_id;
                }
            }
			if(image_id->image_id == 0)
			{
                divoom_system_free(image_id);
                //divoom_print("no image!\n");
                divoom_system_free_lock(pdivoom_play_gif_com_handle->read_lock_handle);
                divoom_system_delay_ms(100);
				break;
			}
            if(pdivoom_play_gif_com_handle->pre_read_image_id != image_id->image_id)
            {
                pimage_cache_data = divoom_manage_image_read(image_id->model_id, image_id->image_id, NULL,divoom_play_gif_com_end_read_task_callback);
                if(pimage_cache_data && pimage_cache_data->num > 0)
                {
                    pdivoom_play_gif_com_handle->pre_read_image_id = image_id->image_id;
                    pimage_cache_data->image_id = image_id->image_id;
                    pdivoom_play_gif_com_handle->cache_pic_num += pimage_cache_data->num;
                    divoom_print("finish reading and caching the image: %d, pic num: %d\n", image_id->image_id, pimage_cache_data->num);
                    divoom_queue_push(pdivoom_play_gif_com_handle->image_cache_queue, pimage_cache_data);
                }
                else
                {
                    if(pimage_cache_data)
                    {
                        divoom_http_download_image_free_image_handle(pimage_cache_data);
                    }
                    divoom_print("fails to read: %d\n", image_id->image_id);
                    divoom_system_free(image_id);
                    divoom_system_free_lock(pdivoom_play_gif_com_handle->read_lock_handle);
                    if(pdivoom_play_gif_com_handle->run_flag)
                    {
                        divoom_system_delay_ms(100);
                        continue;
                    }
                    else
                    {
                        break;
                    }
                }
                
                pcache_item->pcallback(pcache_item->image_info.gif_id, image_id);
                if(pdivoom_play_gif_com_handle->cur_play_image_id != image_id->image_id)
                {
                    pdivoom_play_gif_com_handle->cur_play_image_id = image_id->image_id;
                    divoom_queue_push(pcache_item->id_queue, image_id);
                }
                else
                {
                    divoom_system_free(image_id);
                }
            }
            else
            {
                //如果相等，将不加载
                pcache_item->pcallback(pcache_item->image_info.gif_id, image_id);
                if(pdivoom_play_gif_com_handle->cur_play_image_id != image_id->image_id)
                {
                    pdivoom_play_gif_com_handle->cur_play_image_id = image_id->image_id;
                    divoom_queue_push(pcache_item->id_queue, image_id);
                    //pdivoom_play_gif_com_handle->read_flag = 1;
                }
                else
                {
                    if(image_id->image_id != 0)
                    {
                        //pdivoom_play_gif_com_handle->read_flag = 0;
                    }
                    divoom_system_free(image_id);
                }
                divoom_system_free_lock(pdivoom_play_gif_com_handle->read_lock_handle);
                if(pdivoom_play_gif_com_handle->run_flag)
                {
                    divoom_system_delay_ms(100);
                }
                break;
            }
            divoom_system_free_lock(pdivoom_play_gif_com_handle->read_lock_handle);
            if(pdivoom_play_gif_com_handle->run_flag)
            {
                int delay_time = 0 ,cache_items = divoom_queue_items(pdivoom_play_gif_com_handle->image_cache_queue);
                if(cache_items <=  1)
                {
                    delay_time = 100;
                }
                else if(cache_items ==  2)
                {
                    delay_time = 200;
                }
                else
                {                    
                    delay_time = 300;
                }
                while(pdivoom_play_gif_com_handle->run_flag && delay_time)
                {
                    delay_time --;
                    divoom_system_delay_ms(10);
                }
            }
        }
        
    }
    pdivoom_play_gif_com_handle->exit_flag |= 1;  
    divoom_print("exit read!\n");
    return NULL;
}



/*********************************************************************
  Function:        divoom_play_gif_com_reset_all_source
  Description:     将复位所有的资源
  Input:           none
  Return:          none
**********************************************************************/

int divoom_play_gif_com_reset_all_source(DIVOOM_PLAY_GIF_COM_RESET_CALLBACK *pcallback, void *data, int free_flag) 
{
    Pdivoom_play_gif_com_image_queue pcache_item = NULL, pmain_image_area = NULL;   
    Pdivoom_http_download_image_file_info pimage_cache_data = NULL;
    
    if(pdivoom_play_gif_com_handle == NULL)
    {
        return 0;
    }
    divoom_image_with_text_disp_pause(1);
    divoom_system_get_lock(pdivoom_play_gif_com_handle->lock_handle);    
    divoom_system_get_lock(pdivoom_play_gif_com_handle->read_lock_handle);
    while(divoom_queue_items(pdivoom_play_gif_com_handle->image_cache_queue))
    {
        pimage_cache_data = divoom_queue_pop(pdivoom_play_gif_com_handle->image_cache_queue);
        if(pimage_cache_data == NULL)
        {
            break;
        }
        divoom_http_download_image_free_image_handle(pimage_cache_data);
    }
    if(free_flag)
    {
        divoom_http_download_image_free_image_handle(pdivoom_play_gif_com_handle->pcur_disp_image_handle);
        pdivoom_play_gif_com_handle->pcur_disp_image_handle = NULL;
    }
    pimage_cache_data = NULL;
    if(pcallback)
    {
        pcallback(data);
    }
    divoom_image_with_text_disp_del_image(0XFF);
    divoom_image_with_text_disp_del_text();
    while(divoom_queue_items(pdivoom_play_gif_com_handle->image_area_queue))
    {
        pcache_item = divoom_queue_pop(pdivoom_play_gif_com_handle->image_area_queue);
        if(pcache_item == NULL)
        {
            break;
        }
        if(pcache_item->image_info.gif_id == 0)
        {
            divoom_queue_pop_all(pcache_item->id_queue, NULL);
            pmain_image_area = pcache_item;
        }
        else
        {
            divoom_queue_destroy(pcache_item->id_queue, NULL);
            divoom_system_free(pcache_item);
        }
    }
    if(pmain_image_area)
    {
        divoom_play_gif_image_info pre_pic_id = {0}, pic_id, image_id, *load_pic_id;
        
        divoom_queue_push(pdivoom_play_gif_com_handle->image_area_queue, pmain_image_area);

        if(pmain_image_area->pcallback)
        {
            //将要播放的队列全部填满

            image_id.image_id = 0;
            pdivoom_play_gif_com_handle->pre_read_image_id = 0;
            while(divoom_queue_is_full(pmain_image_area->id_queue)== 0)
            {
                pmain_image_area->pcallback(0, &pic_id);
                divoom_print("area id: %d, pic_id: %d\n", image_id.image_id, pic_id.image_id);
                if(pic_id.image_id == 0 || pic_id.image_id == pre_pic_id.image_id)
                {
                    break;
                }
                load_pic_id = (divoom_play_gif_image_info *)divoom_system_malloc(sizeof(divoom_play_gif_image_info));
                load_pic_id->image_id = pic_id.image_id;
                load_pic_id->model_id = pic_id.model_id;
                divoom_queue_push(pmain_image_area->id_queue, load_pic_id);
                pre_pic_id.image_id = pic_id.image_id;
            }
            if(divoom_queue_items(pmain_image_area->id_queue) == 0)
            {
                //没有背景图
                divoom_print("no pic info!\n");
                pdivoom_play_gif_com_handle->no_pic_flag = 1;
            }
        }
        {
            load_pic_id = divoom_queue_pop(pmain_image_area->id_queue);
			if(load_pic_id)
			{
	            pimage_cache_data = divoom_manage_image_read(load_pic_id->model_id, load_pic_id->image_id, NULL, NULL);
	            if(pimage_cache_data)
	            {
                    pdivoom_play_gif_com_handle->pre_read_image_id = load_pic_id->image_id;
                    pimage_cache_data->image_id = load_pic_id->image_id;
                    pdivoom_play_gif_com_handle->cache_pic_num = pimage_cache_data->num;
	                divoom_queue_push(pdivoom_play_gif_com_handle->image_cache_queue, pimage_cache_data);
                    divoom_play_gif_com_format_gif_table(pimage_cache_data);
                    pmain_image_area->play_end_flag = 1;
	            }
	            else
	            {
	                divoom_print("fails to read: %d\n", load_pic_id->image_id);
	            }
	            divoom_system_free(load_pic_id);
			}
        }
        pdivoom_play_gif_com_handle->read_flag = 1;
        divoom_system_put_sem(pdivoom_play_gif_com_handle->read_sem);
    }
    pcache_item->play_end_flag = 1;    
    
    divoom_system_free_lock(pdivoom_play_gif_com_handle->read_lock_handle);
    divoom_system_free_lock(pdivoom_play_gif_com_handle->lock_handle);
    
    if(pimage_cache_data)
    {
        Pdivoom_image_info pbase_image_info;
        
        pbase_image_info = &pdivoom_play_gif_com_handle->pic_table_frame[0]; 
        pbase_image_info->gif_id = 0;
        pbase_image_info->layer = 0;
        pbase_image_info->frame_attr = DIVOOM_IMAGE_START_TYPE; 
        pbase_image_info->fill_flag = 0;
        divoom_image_with_text_disp_write_image(pbase_image_info);
    }
    pdivoom_play_gif_com_handle->pre_disp_time_info = 0;
    if(pdivoom_play_gif_com_handle->time_frame_flag)
    {
        pdivoom_play_gif_com_handle->time_frame_flag = 0;
        divoom_play_gif_com_ctrl_time_frame(1);
    }
    divoom_image_with_text_disp_pause(0);
    return 1;
}

/*********************************************************************
  Function:        divoom_play_gif_com_clear_all_cache
  Description:     清除所有的缓冲区
  Input:           none
  Return:          none
**********************************************************************/

void divoom_play_gif_com_clear_all_cache(void) 
{
    int image_area_id;
    divoom_play_gif_image_info image_id;
    Pdivoom_play_gif_com_image_queue pcache_item = NULL;   
    Pdivoom_http_download_image_file_info pimage_cache_data;

    if(pdivoom_play_gif_com_handle == NULL)
    {
        divoom_print("exit!\n");
        return;
    }
    divoom_system_get_lock(pdivoom_play_gif_com_handle->lock_handle);
    image_area_id = 0;
    pcache_item = divoom_queue_get_cmp(pdivoom_play_gif_com_handle->image_area_queue, &image_area_id, divoom_play_gif_com_find_image_area);
    if(pcache_item == NULL)
    {
        divoom_system_free_lock(pdivoom_play_gif_com_handle->lock_handle);
        divoom_print("exit!\n");
        return ;
    }
    divoom_system_get_lock(pdivoom_play_gif_com_handle->read_lock_handle);
    divoom_queue_pop_all(pcache_item->id_queue, NULL);
    while(divoom_queue_items(pdivoom_play_gif_com_handle->image_cache_queue))
    {
        pimage_cache_data = divoom_queue_pop(pdivoom_play_gif_com_handle->image_cache_queue);
        if(pimage_cache_data == NULL)
        {
            break;
        }
        divoom_http_download_image_free_image_handle(pimage_cache_data);
    }
    pcache_item->play_end_flag = 1;
    
    pcache_item->pcallback(pcache_item->image_info.gif_id, &image_id);
    if(image_id.image_id != 0)
    {
        pdivoom_play_gif_com_handle->pre_read_image_id = 0;
        pdivoom_play_gif_com_handle->cur_play_image_id = image_id.image_id;
        
        pimage_cache_data = divoom_manage_image_read(image_id.model_id, image_id.image_id, NULL,NULL);
        if(pimage_cache_data)
        {
            pdivoom_play_gif_com_handle->pre_read_image_id = image_id.image_id;
            pimage_cache_data->image_id = image_id.image_id;
            divoom_queue_push(pdivoom_play_gif_com_handle->image_cache_queue, pimage_cache_data);
            pdivoom_play_gif_com_handle->cache_pic_num = pimage_cache_data->num;
            if(pimage_cache_data)
            {
                int back_speed;
                Pdivoom_image_info pbase_image_info;
                
                pbase_image_info = &pdivoom_play_gif_com_handle->pic_time_frame; 
                back_speed = pbase_image_info->speed;
                pbase_image_info->speed = 10;
                pbase_image_info->gif_id = 0;
                pbase_image_info->layer = 0;
                pbase_image_info->frame_attr = DIVOOM_IMAGE_START_TYPE; 
                pbase_image_info->fill_flag = 0;
                pbase_image_info->rgb_data = NULL;
                divoom_image_with_text_disp_write_image(pbase_image_info);
                pbase_image_info->speed = back_speed;
            }
        }
        else
        {
            divoom_print("read err!\n");
        }
    }
    
    divoom_system_free_lock(pdivoom_play_gif_com_handle->read_lock_handle);
    divoom_system_free_lock(pdivoom_play_gif_com_handle->lock_handle);
    divoom_system_put_sem(pdivoom_play_gif_com_handle->read_sem);
    pdivoom_play_gif_com_handle->pre_disp_time_info = 0;
    if(pdivoom_play_gif_com_handle->time_frame_flag)
    {
        pdivoom_play_gif_com_handle->time_frame_flag = 0;
        divoom_play_gif_com_ctrl_time_frame(1);
    }
}

/*********************************************************************
  Function:        divoom_play_gif_com_ctrl_time_frame
  Description:     是否增加时间显示//只有动画才使用这个功能，默认ＩＤ为3;
  Input:           on_off   开关选项
  Return:          返回句柄
**********************************************************************/
#define DIVOOM_PLAY_GIF_COM_TIME_FRAME_ID 3
void divoom_play_gif_com_ctrl_time_frame(int on_off)
{
    divoom_image_shade_info image_shade;
    
    if(pdivoom_play_gif_com_handle)
    {
        divoom_system_get_lock(pdivoom_play_gif_com_handle->lock_handle);
        if(on_off != pdivoom_play_gif_com_handle->time_frame_flag)
        {
            pdivoom_play_gif_com_handle->pre_time_disp_date.tm_hour = 100;
            pdivoom_play_gif_com_handle->pre_time_disp_date.tm_min = 100;
            if(on_off)
            {
            }
            else
            {
                //将关闭
                divoom_image_with_text_disp_set_shade_image(0, NULL);
                pdivoom_play_gif_com_handle->time_frame_flag = 2;
            }
        }
        divoom_system_free_lock(pdivoom_play_gif_com_handle->lock_handle);
    }
}

void divoom_play_gif_com_add_time_frame(void)
{
    char time_info[10];
    time_t cur_time;
	struct tm cur_date;   
    Pdivoom_text_info ptext_info;
    
    if(pdivoom_play_gif_com_handle->time_frame_flag == 0)
    {
    }
    else if(pdivoom_play_gif_com_handle->time_frame_flag == 2)
    {
        pdivoom_play_gif_com_handle->time_frame_flag = 0;
        divoom_image_with_text_disp_del_text_frame(DIVOOM_PLAY_GIF_COM_TIME_FRAME_ID);
        divoom_image_with_text_disp_set_shade_image(0, NULL);
    }
    else
    {
        cur_time = time(0);
        if(cur_time != pdivoom_play_gif_com_handle->pre_disp_time_info)
        {
	        localtime_r(&cur_time, &cur_date);
            if(cur_date.tm_hour != pdivoom_play_gif_com_handle->pre_time_disp_date.tm_hour ||  cur_date.tm_min != pdivoom_play_gif_com_handle->pre_time_disp_date.tm_min)
            {
                ptext_info = divoom_system_malloc(sizeof(divoom_text_info));
                if(ptext_info)
                {
                    int hour;                    
                    
                    pdivoom_play_gif_com_handle->pre_time_disp_date.tm_hour = cur_date.tm_hour;
                    pdivoom_play_gif_com_handle->pre_time_disp_date.tm_min = cur_date.tm_min;
                    hour = cur_date.tm_hour;
                    if(divoom_main_get_time_flag() == 0)
                    {
                        if(hour != 12)
                        {
                            hour = hour % 12;
                            if(hour == 0)
                            {
                                hour = 12;
                            }
                        }
                    }
                    sprintf(time_info,"%02d:%02d",hour,cur_date.tm_min);
                    ptext_info->x = 44;        //字段的坐标位置
                    ptext_info->y = 2;        //字段的坐标位置
                    ptext_info->size = 5;     //字体大小
                    ptext_info->width = 19;    //显示框的宽度
                    ptext_info->high = 5;    //显示框的宽度
                	ptext_info->word_effct_type = DIVOOM_WORD_EFFECT_ALIGN_CENTER; //正常、单行固定、多行滚动、左对齐、右对齐
                    ptext_info->speed = 100;   //字体的速度
                    ptext_info->color[0] = 0xFF;   //字体的颜色ＲＧＢ
                    ptext_info->color[1] = 0xFF;   //字体的颜色ＲＧＢ
                    ptext_info->color[2] = 0xFF;   //字体的颜色ＲＧＢ
                    ptext_info->image_font_color_flag = 1;
                  	ptext_info->font_id = DIVOOM_PLAY_GIF_COM_TIME_FRAME_ID;	//是属于哪个字体框的
                  	ptext_info->font_flag = 1; //0 点阵字体 1 图像字体 这边显示要特别处理
                    ptext_info->unicode_len = divoom_app_com_utf8_to_unicode((char *)ptext_info->unicode_str, time_info, DIVOOM_WORD_MAX_NUM)/2; //要显示的字串的长度unicode_str有效字符的长度
                	ptext_info->space_dot_num = 1; //每个字之间空几格
                	ptext_info->roll_flag = 0; //该字符串是否滚动
                    ///ptext_info->get_font_info_callback = divoom_font_manage_get_font_data; //获取文字回调函数
                    divoom_image_with_text_disp_write_text(ptext_info);
                }
            }
            pdivoom_play_gif_com_handle->pre_disp_time_info = cur_time;
        }
    }
}

/*********************************************************************
  Function:        divoom_play_gif_com_update_disp_time
  Description:     更新插入显示时间的间隔
  Input:           insert_time   插入显示时间的间隔
  Return:          返回句柄
**********************************************************************/

void divoom_play_gif_com_update_disp_time(int insert_time)
{
    if(pdivoom_play_gif_com_handle)
    {
        divoom_system_get_lock(pdivoom_play_gif_com_handle->lock_handle);
        pdivoom_play_gif_com_handle->insert_time = insert_time;
        if(insert_time == 0)
        {
            pdivoom_play_gif_com_handle->next_disp_time = T_U32_MAX;
        }
        else
        {
            pdivoom_play_gif_com_handle->next_disp_time = divoom_system_get_tickms() + insert_time;
        }
        divoom_system_free_lock(pdivoom_play_gif_com_handle->lock_handle);
        return;
    }
}

/*********************************************************************
  Function:        divoom_play_gif_com_init
  Description:     模块初始化
  Input:           insert_time   插入显示时间的间隔,为0将不显示时间
                   gif_time      动画显示最少时间
                   pic_id        开始播放的内置图片ＩＤ
  Return:          返回句柄
**********************************************************************/

void divoom_play_gif_com_init(int insert_time, int gif_time, int pic_id)
{
    if(pdivoom_play_gif_com_handle)
    {
        return;
    }

    pdivoom_play_gif_com_handle = divoom_system_malloc(sizeof(divoom_play_gif_com_image_ctrl));
    if(pdivoom_play_gif_com_handle == NULL)
    {
        return;
    }
    pdivoom_play_gif_com_handle->run_flag = 1;
    pdivoom_play_gif_com_handle->encode_flag = 1;
    pdivoom_play_gif_com_handle->play_tol_time = gif_time;
    pdivoom_play_gif_com_handle->image_area_queue = divoom_queue_init(10);
    pdivoom_play_gif_com_handle->image_cache_queue = divoom_queue_init(100);
    pdivoom_play_gif_com_handle->read_sem = divoom_system_create_sem(1, 0);
    pdivoom_play_gif_com_handle->lock_handle = divoom_system_create_lock();
    pdivoom_play_gif_com_handle->read_lock_handle = divoom_system_create_lock();
    pdivoom_play_gif_com_handle->insert_time = insert_time;
    pdivoom_play_gif_com_handle->no_pic_flag = 0;
    pdivoom_play_gif_com_handle->pic_time_frame.rgb_data = divoom_system_malloc(SCREEN_BUF_SIZE_64); //图片裸数据
    if(insert_time == 0)
    {
        pdivoom_play_gif_com_handle->next_disp_time = T_U32_MAX;
    }
    else
    {
        pdivoom_play_gif_com_handle->next_disp_time = divoom_system_get_tickms() + insert_time;
    }
    pdivoom_play_gif_com_handle->default_pic_id = pic_id;
    divoom_image_with_text_disp_init(NULL);
    divoom_print("start play!\n");
    pdivoom_play_gif_com_handle->read_flag = 0;
    pdivoom_play_gif_com_handle->task_handle = divoom_system_create_task(divoom_play_gif_com_task_run, "gif_com_run", NULL, DIVOOM_TASK_STACK_GIF_COM_RUN);
    pdivoom_play_gif_com_handle->read_handle = divoom_system_create_task(divoom_play_gif_com_task_read, "gif_com_read", NULL, DIVOOM_TASK_STACK_GIF_COM_READ);
}

/*********************************************************************
  Function:        divoom_play_gif_com_set_single_image_buffer
  Description:     标准的播放某个固定的ＢＵＦＦＥＲ的数据,像提醒什么的,也将数据打包成Pdivoom_http_download_image_file_info
                   方便系统统一处理
  Input:           handle
                   image_size, 16,32,64, 
                   image_buf  图片的ＲＧＢ　ＢＵＦ
                   speed      播放速度，以ms为单位
                   image_num  图片数目
                   tol_time   播放总时间
                   pcallback  播放完成的回调
  Return:           NONE
**********************************************************************/

void divoom_play_gif_com_set_single_image_buffer(int image_size, uint8 *image_buf,
                                          int speed, int image_num, int tol_time,DIVOOM_DISP_CTRL_OVER_CALL_END *pcallback)
{
    int i, pic_size;
    Pdivoom_http_download_image_file_info pimage_info;
    Pdivoom_http_download_image_file_item pimage_item;

    if(pdivoom_play_gif_com_handle == NULL)
    {
        return;
    }

    divoom_system_get_lock(pdivoom_play_gif_com_handle->lock_handle);

    if(pdivoom_play_gif_com_handle->pcur_disp_image_handle)
    {
        divoom_http_download_image_free_image_handle(pdivoom_play_gif_com_handle->pcur_disp_image_handle);
        pdivoom_play_gif_com_handle->pcur_disp_image_handle = NULL;
    }
    
    pimage_info = divoom_system_malloc(sizeof(divoom_http_download_image_file_info) + sizeof(Pdivoom_http_download_image_file_item) * image_num);
    if(pimage_info == NULL)
    {
        divoom_system_free_lock(pdivoom_play_gif_com_handle->lock_handle);
        return ;
    }
    pimage_info->pimage_table = (Pdivoom_http_download_image_file_item *)&pimage_info[1];
    pimage_info->font_flag = 0;
    pimage_info->num = image_num;
    pimage_info->width = image_size;
    pimage_info->speed = speed;

    pic_size = image_size * image_size * 3;
    for(i = 0; i < image_num; i++)
    {
        pimage_item = divoom_system_malloc(sizeof(divoom_http_download_image_file_item) + pic_size);
        if(pimage_info->pimage_table[i] == NULL)
        {
            divoom_print("%s:%d: it fails to malloc!\n", __FUNCTION__, __LINE__);
            divoom_http_download_image_free_image_handle(pimage_info);
            divoom_system_free_lock(pdivoom_play_gif_com_handle->lock_handle);
            return;
        }
        pimage_item->image_type = 0;
        pimage_item->image_size = image_size;
        pimage_item->image_data = (uint8 *)&pimage_item[1];
        memcpy(pimage_item->image_data, &image_buf[i * pic_size], pic_size);
        pimage_info->pimage_table[i] = pimage_item;        
    }
    pdivoom_play_gif_com_handle->pcur_disp_image_handle = pimage_info;
    pdivoom_play_gif_com_handle->play_tol_time = tol_time;
    pdivoom_play_gif_com_handle->play_end_time = tol_time + divoom_system_get_tickms();
    pdivoom_play_gif_com_handle->pover_single_callback = pcallback;
    divoom_system_free_lock(pdivoom_play_gif_com_handle->lock_handle);
}


/*********************************************************************
  Function:        divoom_play_gif_com_set_part_image_buffer
  Description:     设置局部图的相关信息
  Input:           handle
                   image_id   局部区域的标号
                   start_x    图片在大图中x的位置
                   start_y    图片在大图中y的位置
                   width      图片在大图中的宽度
                   height     图片在大图中的高度
                   image_buf  图片的缓冲区
                   speed      播放速度，以ms为单位
                   image_num  图片数目
                   layer      图层
  Return:           NONE
**********************************************************************/

void divoom_play_gif_com_set_part_image_buffer(int image_id, int start_x, int start_y, int width, int height, 
                                                uint8 *image_buf,int speed, int image_num, int layer)
{
    int i, pic_size;
    Pdivoom_image_info pimage_info;
    
    pic_size = width * height * 3;    
    divoom_image_with_text_ctrl_lock(1);
    for(i = 0; i < image_num; i ++)
    {
        pimage_info = divoom_system_malloc(sizeof(divoom_image_info));
        pimage_info->rgb_data = divoom_system_malloc(pic_size);
    	pimage_info->gif_id = image_id;	
        if(i == 0)
        {
            pimage_info->frame_attr = DIVOOM_IMAGE_START_TYPE;
        }
        else
        {
    	    pimage_info->frame_attr = DIVOOM_IMAGE_PIC_TYPE;
        }
    	pimage_info->pic_effect = 0; 
    	pimage_info->speed = speed;	
    	pimage_info->pos_x = start_x;	//坐标x起始位置
    	pimage_info->pos_y = start_y;  //坐标y起始位置
    	pimage_info->pic_width = width; //图片宽度
    	pimage_info->pic_high = height; //图片高度
    	pimage_info->layer = layer; //该图存在哪个层级
    	memcpy(pimage_info->rgb_data, &image_buf[pic_size * i], pic_size); //图片裸数据
        pimage_info->fill_flag = 0;
        divoom_image_with_text_disp_write_image_no_lock(pimage_info);
    }
    divoom_image_with_text_ctrl_lock(0);
    
}


/*********************************************************************
  Function:        divoom_play_gif_com_add_image_handle
  Description:     更新播放句柄
  Input:           pimage_handle
                   
  Return:           NONE
**********************************************************************/

void divoom_play_gif_com_add_image_handle(void *pimage_handle)
{    
    int image_id = 0;
    Pdivoom_play_gif_com_image_queue pimage_area;
    Pdivoom_http_download_image_file_info pimage_info = (Pdivoom_http_download_image_file_info)pimage_handle;

    if(pdivoom_play_gif_com_handle == NULL || pimage_handle == NULL)
    {
        return;
    }

    divoom_system_get_lock(pdivoom_play_gif_com_handle->lock_handle);

    pdivoom_play_gif_com_handle->no_free_cache = 1;

    pimage_area = divoom_queue_get_cmp(pdivoom_play_gif_com_handle->image_area_queue, &image_id, divoom_play_gif_com_find_image_area);
    if(pimage_area == NULL)
    {
        pimage_area = divoom_system_malloc(sizeof(divoom_play_gif_com_image_queue));
        if(pimage_area == NULL)
        {
            divoom_system_free_lock(pdivoom_play_gif_com_handle->lock_handle);
            return;
        }
        pimage_area->id_queue = divoom_queue_init(10);
        pimage_area->play_end_flag = 1;             //开始设置为播放完成,方便线程加载
    	pimage_area->image_info.gif_id = image_id;	//0 表示背景图 局部图从1开始
    	pimage_area->image_info.pos_x = 0;	//坐标x起始位置
    	pimage_area->image_info.pos_y = 0;  //坐标y起始位置
    	pimage_area->image_info.pic_width = SCREEN_HEIGHT_64; //图片宽度
    	pimage_area->image_info.pic_high = SCREEN_HEIGHT_64; //图片高度
    	pimage_area->image_info.layer = 0; //该图存在哪个层级
        pimage_area->pcallback = NULL;
        divoom_queue_push(pdivoom_play_gif_com_handle->image_area_queue, pimage_area);
    }
    else
    {
        pimage_area->play_end_flag = 1;             //开始设置为播放完成,方便线程加载
        divoom_queue_pop_all(pimage_area->id_queue, NULL);
    }
    if(image_id == 0)
    {        
        pdivoom_play_gif_com_handle->play_tol_time = T_U32_MAX;
    }
    
    divoom_queue_push(pdivoom_play_gif_com_handle->image_cache_queue, pimage_handle);
    divoom_system_free_lock(pdivoom_play_gif_com_handle->lock_handle);
    divoom_print("send image!%d, \n",pimage_info->num);
}

/*********************************************************************
  Function:        divoom_play_gif_com_create_image_area
  Description:     创建局部图的相关信息
  Input:           handle
                   image_id   局部区域的标号
                   pic_id     图片的ＩＤ,这个ＩＤ要能在divoom_manage_image_read中得到相关数据才行
                   start_x    图片在大图中x的位置
                   start_y    图片在大图中y的位置
                   image_size 图片宽度
                   layer      图层
                   pcallback  获取下一个pic_id
  Return:           NONE
**********************************************************************/

void divoom_play_gif_com_create_image_area(int image_id, int start_x, int start_y, 
                                      int image_size, int layer, DIVOOM_PLAY_GIF_COM_GET_NEW_ID pcallback,DIVOOM_DISP_CTRL_OVER_CALL_END *povercallback)
{
    uint8 new_flag = 0;
    divoom_play_gif_image_info *load_pic_id, pic_id;
    Pdivoom_play_gif_com_image_queue pimage_area;

    if(pdivoom_play_gif_com_handle == NULL)
    {
        return;
    }
    divoom_system_get_lock(pdivoom_play_gif_com_handle->lock_handle);

    pimage_area = divoom_queue_get_cmp(pdivoom_play_gif_com_handle->image_area_queue, &image_id, divoom_play_gif_com_find_image_area);
    if(pimage_area == NULL)
    {
        pimage_area = divoom_system_malloc(sizeof(divoom_play_gif_com_image_queue));
        if(pimage_area == NULL)
        {
            divoom_system_free_lock(pdivoom_play_gif_com_handle->lock_handle);
            return;
        }
        pimage_area->id_queue = divoom_queue_init(10);
        new_flag = 1;
    }
    
	pimage_area->image_info.gif_id = image_id;	//0 表示背景图 局部图从1开始
	pimage_area->image_info.pos_x = start_x;	//坐标x起始位置
	pimage_area->image_info.pos_y = start_y;  //坐标y起始位置
	pimage_area->image_info.pic_width = image_size; //图片宽度
	pimage_area->image_info.pic_high = image_size; //图片高度
	pimage_area->image_info.layer = layer; //该图存在哪个层级
    pimage_area->pcallback = pcallback;
    pdivoom_play_gif_com_handle->pover_single_callback = povercallback;
    pimage_area->play_end_flag = 1;             //开始设置为播放完成,方便线程加载
    divoom_queue_pop_all(pimage_area->id_queue, NULL);
    if(pcallback)
    {
        //将要播放的队列全部填满
        int pre_pic_id = 0;
        
        pdivoom_play_gif_com_handle->cur_play_image_id = 0;
        while(divoom_queue_is_full(pimage_area->id_queue)== 0)
        {
            pcallback(image_id, &pic_id);
            divoom_print("area id: %d, pic_id: %d\n", image_id, pic_id.image_id);
            if(pic_id.image_id == 0 || pic_id.image_id == pre_pic_id)
            {
                break;
            }
            load_pic_id = (Pdivoom_play_gif_image_info)divoom_system_malloc(sizeof(divoom_play_gif_image_info));
            memcpy(load_pic_id, &pic_id, sizeof(pic_id));
            divoom_queue_push(pimage_area->id_queue, load_pic_id);
            pre_pic_id = pic_id.image_id;
            pdivoom_play_gif_com_handle->cur_play_image_id = pre_pic_id;
        }
        if(image_id == 0)
        {
            if(divoom_queue_items(pimage_area->id_queue) == 0)
            {
                //没有背景图
                divoom_print("no pic info!\n");
                pdivoom_play_gif_com_handle->no_pic_flag = 1;
            }
        }
    }
    divoom_image_with_text_disp_set_callback((T_U8)image_id, divoom_play_gif_com_play_over_callback, NULL);
    
    if(image_id == 0)
    {
        //启动读线程
        if(pdivoom_play_gif_com_handle->default_pic_id != -1)
        {
            Pdivoom_image_info pbase_image_info;
            
            pbase_image_info = divoom_system_malloc(sizeof(divoom_image_info));
            if(pbase_image_info)
            {
            	pbase_image_info->gif_id = 0;	//0 表示背景图 局部图从1开始
            	pbase_image_info->frame_attr = DIVOOM_IMAGE_PIC_TYPE; //数据包类型【起始帧、中间图片帧、结束帧】
            	pbase_image_info->speed = 1000;	//动画速度
            	pbase_image_info->pos_x = 0;	//坐标x起始位置
            	pbase_image_info->pos_y = 0;  //坐标y起始位置
            	pbase_image_info->pic_width = SCREEN_WIDTH_16; //图片宽度
            	pbase_image_info->pic_high = SCREEN_WIDTH_16; //图片高度
            	pbase_image_info->layer = 0; //该图存在哪个层级
            	pbase_image_info->rgb_data = divoom_system_malloc(SCREEN_BUF_SIZE_16); //图片裸数据
            	if(pbase_image_info->rgb_data)
            	{
                    divoom_disp_picture_get_small_rgb_data(pdivoom_play_gif_com_handle->default_pic_id, pbase_image_info->rgb_data);
                    pbase_image_info->fill_flag = 0;
                    divoom_image_with_text_disp_write_image(pbase_image_info);
            	}

            }
        }
        //if(pdivoom_play_gif_com_handle->pcur_disp_image_handle == NULL)
        {
            Pdivoom_http_download_image_file_info pimage_cache_data;

            load_pic_id = divoom_queue_pop(pimage_area->id_queue);
			if(load_pic_id)
			{
	            pimage_cache_data = divoom_manage_image_read(load_pic_id->model_id, load_pic_id->image_id, NULL,divoom_play_gif_com_end_read_task_callback);
	            if(pimage_cache_data)
	            {
                    pimage_cache_data->image_id = load_pic_id->image_id;
                    pdivoom_play_gif_com_handle->pre_read_image_id = load_pic_id->image_id;
	                divoom_queue_push(pdivoom_play_gif_com_handle->image_cache_queue, pimage_cache_data);
                    pdivoom_play_gif_com_handle->cache_pic_num = pimage_cache_data->num;
	            }
	            else
	            {
	                divoom_print("fails to read: %d\n", load_pic_id->image_id);
	            }
	            divoom_system_free(load_pic_id);
			}
        }
        if(new_flag)
        {
            divoom_queue_push(pdivoom_play_gif_com_handle->image_area_queue, pimage_area);
            new_flag = 0;
        }
        pdivoom_play_gif_com_handle->read_flag = 1;
        divoom_system_put_sem(pdivoom_play_gif_com_handle->read_sem);
    }
    if(new_flag)
    {
        divoom_queue_push(pdivoom_play_gif_com_handle->image_area_queue, pimage_area);
    }
    divoom_system_free_lock(pdivoom_play_gif_com_handle->lock_handle);
    
    
}



/*********************************************************************
  Function:        divoom_play_gif_com_update_each_gif_time
  Description:     更新单个动画的显示时间
  Input:           gif_time
                   
  Return:          none
**********************************************************************/

void divoom_play_gif_com_update_each_gif_time(int gif_time)
{
    int image_id = 0;
    Pdivoom_play_gif_com_image_queue pcache_item;
    if(pdivoom_play_gif_com_handle == NULL)
    {
        return ;
    }
    divoom_system_get_lock(pdivoom_play_gif_com_handle->lock_handle);
    pcache_item = divoom_queue_get_cmp(pdivoom_play_gif_com_handle->image_area_queue, &image_id, divoom_play_gif_com_find_image_area);
    if(pcache_item)
    {
        pcache_item->play_end_time = divoom_system_get_tickms() + gif_time;
    }
    pdivoom_play_gif_com_handle->play_tol_time = gif_time;
    divoom_system_free_lock(pdivoom_play_gif_com_handle->lock_handle);
}

/*********************************************************************
  Function:        divoom_play_gif_com_update_text
  Description:     更新字休信息
  Input:           handle
                   ptext_info
  Return:          如果之前没有字体，返回0,否则返回１
**********************************************************************/

int divoom_play_gif_com_update_text(Pdivoom_draw_set_info_json_image_info ptext_info)
{
    int ret = 0;
    
    if(pdivoom_play_gif_com_handle == NULL || ptext_info->unicode_len == 0)
    {
        return ret;
    }
    divoom_system_get_lock(pdivoom_play_gif_com_handle->lock_handle);
    if(pdivoom_play_gif_com_handle->pcur_disp_image_handle->pfont_data == NULL)
    {
        ret = 0;
        pdivoom_play_gif_com_handle->pcur_disp_image_handle->pfont_data = divoom_system_malloc(sizeof(divoom_http_download_image_font_data));        
    }
    else
    {
        ret = 1;
    }
    pdivoom_play_gif_com_handle->pcur_disp_image_handle->pfont_data->size = ptext_info->text_size;                     //字体大小
    pdivoom_play_gif_com_handle->pcur_disp_image_handle->pfont_data->font = ptext_info->text_font;                     //字体类型
    pdivoom_play_gif_com_handle->pcur_disp_image_handle->pfont_data->speed = ptext_info->text_speed;                    //文字速度  
    pdivoom_play_gif_com_handle->pcur_disp_image_handle->pfont_data->textEffect = ptext_info->text_effect;               //文字效果
    pdivoom_play_gif_com_handle->pcur_disp_image_handle->pfont_data->picEffect = ptext_info->pic_effect;                //图片效果
    pdivoom_play_gif_com_handle->pcur_disp_image_handle->pfont_data->start_x = ptext_info->text_start_x;                  //框的起始x
    pdivoom_play_gif_com_handle->pcur_disp_image_handle->pfont_data->start_y = ptext_info->text_start_y;                  //框的起始y
    pdivoom_play_gif_com_handle->pcur_disp_image_handle->pfont_data->frameWidth = ptext_info->text_frame_width;               //框的高度
    pdivoom_play_gif_com_handle->pcur_disp_image_handle->pfont_data->frameHeight = ptext_info->text_frame_hight;              //框的高度
    pdivoom_play_gif_com_handle->pcur_disp_image_handle->pfont_data->color[0] = ptext_info->text_color[0];               //字体颜色
    pdivoom_play_gif_com_handle->pcur_disp_image_handle->pfont_data->color[1] = ptext_info->text_color[1];               //字体颜色
    pdivoom_play_gif_com_handle->pcur_disp_image_handle->pfont_data->color[2] = ptext_info->text_color[2];               //字体颜色
    pdivoom_play_gif_com_handle->pcur_disp_image_handle->pfont_data->unicode_len = ptext_info->unicode_len;           //显示字体长度
    memcpy(pdivoom_play_gif_com_handle->pcur_disp_image_handle->pfont_data->unicode_str, ptext_info->unicode_str, ptext_info->unicode_len*2);      //显示的字体unicode

    //divoom_system_font_get_unicode_font_info(pdivoom_play_gif_com_handle->pcur_disp_image_handle->pfont_data);
    divoom_system_free_lock(pdivoom_play_gif_com_handle->lock_handle);
    if(ret == 0)
    {
        //有文字的话，将发送文字信息
        Pdivoom_text_info ptext_info;
        Pdivoom_http_download_image_font_data pfont_data = pdivoom_play_gif_com_handle->pcur_disp_image_handle->pfont_data;

        ptext_info = divoom_system_malloc(sizeof(divoom_text_info));
        if(ptext_info)
        {
            ptext_info->x = pfont_data->start_x;        //字段的坐标位置
            ptext_info->y = pfont_data->start_y;        //字段的坐标位置
            ptext_info->size = pfont_data->size;     //字体大小
            ptext_info->width = pfont_data->frameWidth;    //显示框的宽度
            ptext_info->high = pfont_data->frameHeight;    //显示框的宽度
        	ptext_info->word_effct_type = DIVOOM_WORD_EFFECT_NORM; //正常、单行固定、多行滚动
            ptext_info->font_effect = DIVOOM_TEXT_EFFECT_LEFT; //上下左右滚动方式
            ptext_info->speed = pfont_data->speed;   //字体的速度
            memcpy(ptext_info->color ,pfont_data->color, 3); //字体的颜色ＲＧＢ
          	ptext_info->font_id  = 1;	//是属于哪个字体框的
            ptext_info->unicode_len = pfont_data->unicode_len; //要显示的字串的长度unicode_str有效字符的长度
            memcpy(ptext_info->unicode_str, pfont_data->unicode_str, pfont_data->unicode_len * 2); //要显示的字串
        	ptext_info->space_dot_num = 0; //每个字之间空几格
        	ptext_info->roll_flag = 1; //该字符串是否滚动
        	ptext_info->get_font_info_callback = divoom_play_gif_com_get_font_info; 

            divoom_image_with_text_disp_write_text(ptext_info);
        }
    }
    return ret;
}

/*********************************************************************
  Function:        divoom_play_gif_com_set_text
  Description:     设置局部文字播放相关信息
  Input:           handle
                   ptext_info
  Return:           NONE
**********************************************************************/

void divoom_play_gif_com_set_text(Pdivoom_text_info ptext_info)
{
    Pdivoom_text_info pnew_info;

    pnew_info = divoom_system_malloc(sizeof(divoom_text_info));
    if(pnew_info)
    {
        memcpy(pnew_info, ptext_info, sizeof(divoom_text_info));
        divoom_image_with_text_disp_write_text(pnew_info);
    }
}


/*********************************************************************
  Function:        divoom_play_gif_com_get_play_image_id
  Description:     得到播放的ＩＤ
  Input:           
  Return:          none
**********************************************************************/

int divoom_play_gif_com_get_play_image_id(void)
{
    int image_id;
    if(pdivoom_play_gif_com_handle == NULL)
    {
        return 0;
    }
    
    divoom_system_get_lock(pdivoom_play_gif_com_handle->lock_handle);
    if(pdivoom_play_gif_com_handle->pcur_disp_image_handle == NULL)
    {
        divoom_system_free_lock(pdivoom_play_gif_com_handle->lock_handle);
        return 0;
    }
    image_id = pdivoom_play_gif_com_handle->pcur_disp_image_handle->image_id;
    divoom_system_free_lock(pdivoom_play_gif_com_handle->lock_handle);
    return image_id;
}



/*********************************************************************
  Function:        divoom_play_gif_com_exit_image
  Description:     模块退出，释放所有资源
  Input:           
  Return:          none
**********************************************************************/

Pdivoom_http_download_image_file_info divoom_play_gif_com_exit_image(void)
{
    Pdivoom_play_gif_com_image_queue pimage_area;
    Pdivoom_http_download_image_file_info pimage_cache , pcur_play;
    
    if(pdivoom_play_gif_com_handle == NULL)
    {
        return NULL;
    }
    pdivoom_play_gif_com_handle->exit_flag = 0;
    pdivoom_play_gif_com_handle->run_flag = 0;
    divoom_system_put_sem(pdivoom_play_gif_com_handle->read_sem);
    divoom_system_delay_ms(1);
    while(pdivoom_play_gif_com_handle->exit_flag != 3)
    {
        divoom_system_delay_ms(10);
    }
    divoom_print("star exit!\n");
    divoom_image_with_text_disp_exit();
    divoom_print("end play!\n");
    
    while(1)
    {
        pimage_area = divoom_queue_pop(pdivoom_play_gif_com_handle->image_area_queue);
        if(pimage_area == NULL)
        {
            break;
        }
        divoom_queue_destroy(pimage_area->id_queue, NULL);
        divoom_system_free(pimage_area);
    }
    divoom_print("end play!\n");
    divoom_queue_destroy(pdivoom_play_gif_com_handle->image_area_queue, NULL);
    pcur_play = pdivoom_play_gif_com_handle->pcur_disp_image_handle;
    if(pdivoom_play_gif_com_handle->pcur_disp_image_handle && pdivoom_play_gif_com_handle->no_free_cache == 0)
    {        
        divoom_http_download_image_free_image_handle(pdivoom_play_gif_com_handle->pcur_disp_image_handle);
        pcur_play = NULL;
        pdivoom_play_gif_com_handle->pcur_disp_image_handle = NULL;
    }
    
    while(1)
    {
        pimage_cache = divoom_queue_pop(pdivoom_play_gif_com_handle->image_cache_queue);
        if(pimage_cache == NULL)
        {
            break;
        }
        if(pdivoom_play_gif_com_handle->no_free_cache)
        {
            divoom_http_download_image_free_image_handle(pcur_play);
            pcur_play = pimage_cache;
        }
        else
        {
            divoom_http_download_image_free_image_handle(pimage_cache);
        }
    }
    divoom_print("end play!\n");
    divoom_system_destroy_sem(pdivoom_play_gif_com_handle->read_sem);
    divoom_system_destroy_lock(pdivoom_play_gif_com_handle->lock_handle);
    divoom_system_destroy_lock(pdivoom_play_gif_com_handle->read_lock_handle);
    divoom_queue_destroy(pdivoom_play_gif_com_handle->image_cache_queue, NULL);
    divoom_system_free(pdivoom_play_gif_com_handle->pic_time_frame.rgb_data);
    divoom_system_free(pdivoom_play_gif_com_handle);
    pdivoom_play_gif_com_handle = NULL;
    divoom_print("end play!\n");
    return pcur_play;
}

void divoom_play_gif_com_wait_notice_start(void)
{
    #if 1
    //divoom_disp_Play_net_image_image_info notice_info = {0};

    divoom_image_with_text_disp_pause(1);

    //notice_info.image_id = DIVOOM_MANAGE_IMAGE_ID_BASE_NOTICE_NET_PIC + DIVOOM_lOCAL_USE_PIC_NOTICE_LOADING_WAIT_MSG;
    //notice_info.model_id = DIVOOM_MANAGE_IMAGE_NOTICE_NET_GIF;
   // divoom_disp_Play_net_image_init(&notice_info);

    if(pdivoom_play_gif_com_handle)
    {
        pdivoom_play_gif_com_handle->wait_flag = 1;
    }
    #endif
}

void divoom_play_gif_com_wait_notice_end(void)
{
#if 1
    //divoom_disp_Play_net_image_exit();
    if(pdivoom_play_gif_com_handle)
    {
        pdivoom_play_gif_com_handle->wait_flag = 0;
    }
    divoom_system_delay_ms(50);
    divoom_image_with_text_disp_pause(0);
#endif
}
#endif

/*********************************************************
  Function:		divoom_play_com_convert_rgb_net_to_local
  Description:	将网络上的ＲＧＢ排法转换为设备显示的大图
  Input:			width           图像宽度
                    src_pic_buf     网络图片的数据
                    rgb_buf         转换可用的ＢＵＦ
  Return:			none
************************************************************/

void divoom_play_com_convert_rgb_net_to_local(int width, uint8 *src_pic_buf, uint8 * rgb_buf)
{
    int j, k, m;
    divoom_light_color_info color;
    
    if(width == SCREEN_WIDTH_32)
    {
        for(k = 0; k < 4; k ++)
        {
            for(j = 0; j < 16; j ++)
            {
                for(m = 0; m < 16; m ++)
                {
                    divoom_disp_ctrl_get_data(j * 16 + m,&color, &src_pic_buf[768*k]);
                    divoom_disp_ctrl_set_data((j + k/2*16) * 32 + m + (k%2)*16,&color, rgb_buf);
                }
            }
        }
    }
    else if(width == SCREEN_WIDTH_64)
    {
        for(k = 0; k < 16; k ++)
        {
            for(j = 0; j < 16; j ++)
            {
                for(m = 0; m < 16; m ++)
                {
                    divoom_disp_ctrl_get_data(j * 16 + m,&color, &src_pic_buf[768*k]);
                    divoom_disp_ctrl_set_data((j + k/4*16) * 64 + m + (k%4)*16,&color, rgb_buf);
                }
            }
        }
    }
	else if (width == SCREEN_WIDTH_128)
	{
		for(k = 0; k < 64; k ++)
        {
            for(j = 0; j < 16; j ++)
            {
                for(m = 0; m < 16; m ++)
                {
                    divoom_disp_ctrl_get_data(j * 16 + m,&color, &src_pic_buf[768*k]);
                    divoom_disp_ctrl_set_data((j + k/8*16) * 128 + m + (k%8)*16,&color, rgb_buf);
                }
            }
        }
	}
    else
    {
        memcpy(rgb_buf, src_pic_buf, width * width * 3);
    }
}
