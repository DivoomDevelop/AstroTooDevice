#if 0
#include "divoom.h"
/*******************************
此文件完成所有的动画的播放动作
********************************/
typedef struct _divoom_gif_ctrl_com_ctrl
{
    T_U8   read_empty_flag;        //缓冲区空才读标记
    T_U8   pic_type;        //储存数据的图片格式
    T_U8   word_frame_flag; //是否已经发送文字帧
    T_U8   frame_type;        //帧类型
    T_U8   empty_flag;        //是否要补空帧
    T_U8   gif_over;          //是否有完整的动画数据
    T_U8   type;
    T_U16   frame_len;        //帧长度
    T_U16   black_frame_flag; //数据读完后是否加个空帧的时间信息
    T_U32  file_index;
    T_U32  start_off;
    T_VOID *read_handle;    //读数据的句柄
    T_U8   buf[DIVOOM_PAGE_SIZE];
    
}divoom_gif_ctrl_com_ctrl, *Pdivoom_gif_ctrl_com_ctrl;

T_VOID divoom_gif_ctrl_com_read_data(T_VOID *handle);


/*********************************************************
  Function:		divoom_gif_ctrl_com_add_black_frame
  Description:	当数据读完后是否加个空帧
  Input:		handle
  Return:		NONE
************************************************************/

T_VOID divoom_gif_ctrl_com_add_black_frame(T_VOID *handle, T_U16 speed)
{
    Pdivoom_gif_ctrl_com_ctrl gif_ctrl = (Pdivoom_gif_ctrl_com_ctrl)handle;

    if(gif_ctrl)
    {
        gif_ctrl->black_frame_flag = speed;
    }
}

/*********************************************************
  Function:		divoom_gif_ctrl_com_init
  Description:		启动动画的播放,这里只考虑播放一组动画，多组动画有问题
  Input:			start_page          动画在flash 的起始位置
                    cyc_flag            是否循环读数据
  Return:			NONE
************************************************************/

T_VOID *divoom_gif_ctrl_com_init(T_U8 type, T_U32 file_index,DIVOOM_DISP_CTRL_OVER_CALL_END *pcallback, T_U32 init_size)
{
    T_U8 buf[DIVOOM_PAGE_SIZE];
    Pdivoom_gif_ctrl_com_ctrl gif_ctrl;
    

    gif_ctrl = divoom_malloc(sizeof(divoom_gif_ctrl_com_ctrl), __func__, __LINE__);
    if(gif_ctrl == AK_NULL)
    {
        return AK_NULL;
    }
    memset(gif_ctrl, 0, sizeof(divoom_gif_ctrl_com_ctrl));
    gif_ctrl->pic_type = divoom_save_led_get_type(type, file_index);
    gif_ctrl->empty_flag = 0;
    gif_ctrl->word_frame_flag = 0;
    gif_ctrl->frame_type = DIVOOM_IMAGE_ENCODE_ERR_TYPE;
    gif_ctrl->type = type;
    gif_ctrl->file_index = file_index;
    gif_ctrl->start_off = T_U32_MAX;
    if(init_size == T_U32_MAX)
    {
        gif_ctrl->gif_over = 1;
    }
    else
    {
        gif_ctrl->gif_over = 0;
    }
    gif_ctrl->read_handle = divoom_load_movie_open_read(type, file_index,buf, init_size);
    if(gif_ctrl->read_handle == AK_NULL)
    {
        ak_debug("%s: the gif file is empty and exit!\n",__func__);
        divoom_free(gif_ctrl, __func__, __LINE__);
        return AK_NULL;
    }
    switch(gif_ctrl->pic_type)
    {
        case DIVOOM_SAVE_LED_PIC:
            divoom_image_with_text_disp_init();
            //divoom_image_with_text_disp_set_word_buf(gif_ctrl->buf);
            if(pcallback)
            {
                divoom_image_with_text_disp_set_callback(pcallback, AK_NULL);
                gif_ctrl->empty_flag = 1;
            }
            if(init_size == T_U32_MAX)
            {
                //先加载一些数据，以便能快速显示出来
                divoom_gif_ctrl_com_read_data(gif_ctrl);
                divoom_gif_ctrl_com_read_data(gif_ctrl);
                divoom_gif_ctrl_com_read_data(gif_ctrl);
                divoom_gif_ctrl_com_read_data(gif_ctrl);
                divoom_gif_ctrl_com_read_data(gif_ctrl);
            }
            divoom_main_regiest_loop(gif_ctrl,divoom_gif_ctrl_com_read_data);
            break;
            
        case DIVOOM_SAVE_LED_LED:
        {            
            divoom_load_movie_close(gif_ctrl->read_handle);
            gif_ctrl->read_handle = AK_NULL;
            divoom_disp_led_init(type, file_index, 0);
        }
            break;
            
        case DIVOOM_SAVE_LED_SAND:
            divoom_load_movie_close(gif_ctrl->read_handle);
            gif_ctrl->read_handle = AK_NULL;
            divoom_light_sand_paint_init(type, file_index, 0);
            break;
            
        case DIVOOM_SAVE_LED_PIC_SCAN:
        {
            divoom_light_pic_scan_para  para_info;

            divoom_load_movie_close(gif_ctrl->read_handle);
            gif_ctrl->read_handle = AK_NULL;
            para_info.file_index = file_index;
            para_info.type = type;
            divoom_light_pic_scan_convert(type, file_index, &para_info);
            divoom_light_pic_scan_init(&para_info);
        }
            break;
            
    }

    return gif_ctrl;
}



/*********************************************************
  Function:		divoom_gif_ctrl_com_get_gif_num
  Description:	获取动画读到的张数
  Input:			handle
  Return:			NONE
************************************************************/

T_U8 divoom_gif_ctrl_com_get_gif_num(T_VOID *handle)
{
    Pdivoom_gif_ctrl_com_ctrl gif_ctrl = (Pdivoom_gif_ctrl_com_ctrl)handle;

    if(gif_ctrl && gif_ctrl->read_handle)
    {
        return divoom_load_movie_get_read_cache_pic_num(gif_ctrl->read_handle);
    }
    return 0;
}


/*********************************************************
  Function:		divoom_gif_ctrl_com_reset_gif_num
  Description:	复位动画读到的张数
  Input:			handle
  Return:			NONE
************************************************************/

T_VOID divoom_gif_ctrl_com_reset_gif_num(T_VOID *handle)
{
    Pdivoom_gif_ctrl_com_ctrl gif_ctrl = (Pdivoom_gif_ctrl_com_ctrl)handle;

    if(gif_ctrl && gif_ctrl->read_handle)
    {
        divoom_load_movie_set_read_cache_pic_num(gif_ctrl->read_handle);
    }
}


/*********************************************************
  Function:		divoom_gif_ctrl_com_update_gif_attr
  Description:		设置动画播放的相关属性
  Input:			size          目前动画的实际数量量
  Return:			NONE
************************************************************/

T_VOID divoom_gif_ctrl_com_update_gif_attr(T_VOID *handle, Pdivoom_disp_ctrl_gif_info pgif_attr)
{
    Pdivoom_gif_ctrl_com_ctrl gif_ctrl = (Pdivoom_gif_ctrl_com_ctrl)handle;
    
    if(gif_ctrl)
    {
        switch(pgif_attr->update_type)
        {
            case DIVOOM_DISP_GIF_UPDATE_GIF_CONTENT:
                divoom_load_movie_close(gif_ctrl->read_handle);
                gif_ctrl->pic_type = divoom_save_led_get_type(gif_ctrl->type, gif_ctrl->file_index);
                gif_ctrl->read_handle = divoom_load_movie_open_read(gif_ctrl->type, gif_ctrl->file_index, AK_NULL, DIVOOM_PAGE_SIZE);
                //图片重新加载时，清除一下字体信息
                gif_ctrl->gif_over = 0;
                gif_ctrl->word_frame_flag = 0;
                gif_ctrl->black_frame_flag = 0;
                gif_ctrl->start_off = T_U32_MAX;
                pgif_attr->update_type = DIVOOM_DISP_GIF_UPDATE_CLEAR_GIF;
                divoom_image_with_text_disp_update_info(pgif_attr);   
                pgif_attr->update_type = DIVOOM_DISP_GIF_UPDATE_FONT_CLEAR;
                divoom_image_with_text_disp_update_info(pgif_attr);                
                break;
                
            case DIVOOM_DISP_GIF_UPDATE_GIF_OFFSET:
                if(pgif_attr->gif_tol <= pgif_attr->gif_off)
                {
                    gif_ctrl->gif_over = 1;
                }
                divoom_load_movie_set_image_size(gif_ctrl->read_handle, pgif_attr->gif_off);
                break;
            
            case DIVOOM_DISP_GIF_UPDATE_GIF_SPEED:
                divoom_image_with_text_disp_update_info(pgif_attr);
                break;
                
            default:     
                //如果文字相关信息发生变更，将认为已经读过文字了
                gif_ctrl->word_frame_flag = 1;
                divoom_image_with_text_disp_update_info(pgif_attr);
                break;
        }
    }
}


/*********************************************************
  Function:		divoom_gif_ctrl_com_set_read_quick
  Description:	       只有缓冲区为空的时候才读数据，避免阻塞，有其它的特殊要求时，调用这个，像有声音卡
  Input:		handle
  Return:		NONE
************************************************************/

T_VOID divoom_gif_ctrl_com_set_read_quick(T_VOID *handle)
{
    Pdivoom_gif_ctrl_com_ctrl gif_ctrl = (Pdivoom_gif_ctrl_com_ctrl)handle;

    if(gif_ctrl)
    {
        gif_ctrl->read_empty_flag = 1;
    }
    
}


T_VOID divoom_gif_ctrl_com_read_data(T_VOID *handle)
{
    T_U32 free_size, frame_len, read_len, buf_len;
    T_U8 *write_buf,color[3] = {0};
    Pdivoom_gif_ctrl_com_ctrl gif_ctrl = (Pdivoom_gif_ctrl_com_ctrl)handle;
    
    if(gif_ctrl->pic_type == DIVOOM_SAVE_LED_PIC)
    {
        if(gif_ctrl->read_empty_flag && (divoom_image_with_text_disp_get_cyc_buf_state()!=CYCBUF_EMPTY))
        {
            return;
        }
     
        //不断地读数据   
        free_size = divoom_image_with_text_disp_check_cyc_buf();
        if(free_size < DIVOOM_IMAGE_ENCODE_COLOR_DATA_32_OFF + 2)
        {
            return;
        }
        
        if(free_size > 2)
        {
            free_size -= 2;
        }
        frame_len = divoom_load_movie_get_next_frame_len(gif_ctrl->read_handle);
        if(frame_len == 0 || frame_len > DIVOOM_IMAGE_ENCODE_MAX_64_SIZE)
        {
            if(gif_ctrl->frame_type == DIVOOM_IMAGE_ENCODE_REVIEW_TYPE && (divoom_image_with_text_disp_get_cyc_buf_state()!=CYCBUF_EMPTY))
            {
                return;
            }
            if(gif_ctrl->empty_flag)
            {
                frame_len = divoom_image_encode_empty_frame(gif_ctrl->buf);
                if(frame_len < free_size)
                {
                    ak_debug("%s: we will add empty frame!\n",__func__);
                    if(gif_ctrl->start_off == T_U32_MAX)
                    {
                        divoom_load_movie_reset(gif_ctrl->read_handle);
                    }
                    else
                    {
                        divoom_load_movie_set_pic_off(gif_ctrl->read_handle, gif_ctrl->start_off);
                    }
                    divoom_image_with_text_disp_write(gif_ctrl->buf, frame_len);
                }
                return;
            }
            if(gif_ctrl->start_off == T_U32_MAX)
            {
                divoom_load_movie_reset(gif_ctrl->read_handle);
            }
            else
            {
                divoom_load_movie_set_pic_off(gif_ctrl->read_handle, gif_ctrl->start_off);
            }
            if(gif_ctrl->black_frame_flag)
            {
                frame_len = divoom_image_encode_create_full_screen(gif_ctrl->buf, &color);
                if(free_size > frame_len)
                {
                    //增加一个黑屏帧，产生闪烁的效果
                    divoom_image_decode_update_time_info(gif_ctrl->buf, gif_ctrl->black_frame_flag);
                    divoom_image_with_text_disp_write(gif_ctrl->buf, frame_len);
                }
                return;
            }
            frame_len = divoom_load_movie_get_next_frame_len(gif_ctrl->read_handle);
        }
        if(frame_len > 0 && free_size > frame_len)
        {
            uint8 frame_attr;
            uint32 gif_off;

            gif_off = divoom_load_movie_get_pic_off(gif_ctrl->read_handle);
            read_len = divoom_load_movie_read_part(gif_ctrl->read_handle, gif_ctrl->buf, DIVOOM_IMAGE_ENCODE_COLOR_DATA_OFF);
            if(read_len < DIVOOM_IMAGE_ENCODE_COLOR_DATA_OFF)
            {
                return;
            }
            frame_len = divoom_image_decode_get_pic_len(gif_ctrl->buf);

            
            divoom_image_decode_get_image_info(gif_ctrl->buf, &frame_attr);
            if(DIVOOM_IMAGE_ENCODE_REVIEW_TYPE == frame_attr)
            {
                divoom_load_movie_set_pic_off(gif_ctrl->read_handle, gif_off + frame_len);
                return ;
            }
            if(frame_attr == DIVOOM_IMAGE_ENCODE_WORD_TYPE && gif_ctrl->frame_type == DIVOOM_IMAGE_ENCODE_WORD_TYPE && 
                gif_ctrl->frame_len == DIVOOM_IMAGE_ENCODE_COLOR_DATA_32_OFF && frame_len == DIVOOM_IMAGE_ENCODE_COLOR_DATA_32_OFF)
            {
                //空帧，将直接跳过
                return;
            }
            if(frame_attr == DIVOOM_IMAGE_ENCODE_WORD_TYPE)
            {
                if(gif_ctrl->word_frame_flag)
                {
                    //文字帧只发送一次
                    ak_debug("%s: we will pass the word frame!\n", __func__);
                    divoom_load_movie_set_pic_off(gif_ctrl->read_handle, gif_off + frame_len);
                    return;
                }
                
            }
            else if(frame_attr == DIVOOM_IMAGE_ENCODE_PIC_TYPE)
            {
                //只要出现动画，将文字框标识设置
                if(gif_ctrl->start_off == T_U32_MAX)
                {
                    gif_ctrl->start_off = gif_off;
                }
                gif_ctrl->word_frame_flag = 1;
            }
            
            if(gif_ctrl->black_frame_flag)
            {
                divoom_image_decode_update_time_info(gif_ctrl->buf, gif_ctrl->black_frame_flag);
            }
            
            gif_ctrl->frame_type = frame_attr;
            gif_ctrl->frame_len = frame_len;
            //先写长度，后面再分部写数据
            divoom_image_with_text_disp_write_len(frame_len);
            divoom_image_with_text_disp_write_part(gif_ctrl->buf, read_len);
            frame_len -= read_len;
            while(frame_len)
            {
                write_buf = divoom_image_with_text_disp_get_write_buf(&buf_len);
                if(frame_len > buf_len)
                {
                    read_len = buf_len;
                }
                else
                {
                    read_len = frame_len;
                }
                read_len = divoom_load_movie_read_part(gif_ctrl->read_handle, write_buf, read_len);
                divoom_image_with_text_disp_update_write_len(read_len);
                frame_len -= read_len;
            }            
        }
    }
}

T_VOID divoom_gif_ctrl_com_exit(T_VOID *handle)
{
    Pdivoom_gif_ctrl_com_ctrl gif_ctrl = (Pdivoom_gif_ctrl_com_ctrl)handle;

    if(gif_ctrl)
    {
        switch(gif_ctrl->pic_type)
        {
            case DIVOOM_SAVE_LED_PIC:
                divoom_main_unregiest_loop(divoom_gif_ctrl_com_read_data);
                divoom_image_with_text_disp_exit();
                divoom_load_movie_close(gif_ctrl->read_handle);
                break;
                
            case DIVOOM_SAVE_LED_LED:
                divoom_disp_led_close();
                break;
                
            case DIVOOM_SAVE_LED_SAND:
                divoom_light_sand_paint_exit();
                break;
                
            case DIVOOM_SAVE_LED_PIC_SCAN:
                divoom_light_pic_scan_close();
                break;
                
        }
        divoom_free(gif_ctrl, __func__, __LINE__);
    }
}
#else
#include "divoom.h"




/*********************************************************
  Function:		divoom_gif_ctrl_com_add_black_frame
  Description:	当数据读完后是否加个空帧
  Input:		handle
  Return:		NONE
************************************************************/

T_VOID divoom_gif_ctrl_com_add_black_frame(T_VOID *handle, T_U16 speed)
{}

/*********************************************************
  Function:		divoom_gif_ctrl_com_init
  Description:		启动动画的播放,这里只考虑播放一组动画，多组动画有问题
  Input:			start_page          动画在flash 的起始位置
                    cyc_flag            是否循环读数据
  Return:			NONE
************************************************************/

T_VOID *divoom_gif_ctrl_com_init(T_U8 type, T_U32 file_index,DIVOOM_DISP_CTRL_OVER_CALL_END *pcallback, T_U32 init_size)
{
	return NULL;
}




/*********************************************************
  Function:		divoom_gif_ctrl_com_get_gif_num
  Description:	获取动画读到的张数
  Input:			handle
  Return:			NONE
************************************************************/

T_U8 divoom_gif_ctrl_com_get_gif_num(T_VOID *handle)
{

    return 0;
}


/*********************************************************
  Function:		divoom_gif_ctrl_com_reset_gif_num
  Description:	复位动画读到的张数
  Input:			handle
  Return:			NONE
************************************************************/

T_VOID divoom_gif_ctrl_com_reset_gif_num(T_VOID *handle)
{

}


/*********************************************************
  Function:		divoom_gif_ctrl_com_update_gif_attr
  Description:		设置动画播放的相关属性
  Input:			size          目前动画的实际数量量
  Return:			NONE
************************************************************/

T_VOID divoom_gif_ctrl_com_update_gif_attr(T_VOID *handle, Pdivoom_disp_ctrl_gif_info pgif_attr)
{}


/*********************************************************
  Function:		divoom_gif_ctrl_com_set_read_quick
  Description:	       只有缓冲区为空的时候才读数据，避免阻塞，有其它的特殊要求时，调用这个，像有声音卡
  Input:		handle
  Return:		NONE
************************************************************/

T_VOID divoom_gif_ctrl_com_set_read_quick(T_VOID *handle)
{}


T_VOID divoom_gif_ctrl_com_read_data(T_VOID *handle)
{}

T_VOID divoom_gif_ctrl_com_exit(T_VOID *handle)
{}

#endif


