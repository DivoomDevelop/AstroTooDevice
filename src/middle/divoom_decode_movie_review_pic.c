#include "divoom.h"
#include "divoom_pixel_format.h"

#include <stdint.h>
#include <stdlib.h>

void nv21_to_rgba8888(const uint8_t* nv21, uint8_t* rgba, int width, int height) {
    const uint8_t* y = nv21;
    const uint8_t* v = nv21 + width * height;
    const uint8_t* u = v + 1;
    
    int y_stride = width;
    int uv_stride = width;
    
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            int y_index = i * y_stride + j;
            int uv_index = (i / 2) * uv_stride + (j / 2) * 2;
            
            int y_value = y[y_index];
            int v_value = v[uv_index];
            int u_value = u[uv_index];
            
            // 修正后的 YUV 到 RGB 转换系数
            int c = y_value - 16;
            int d = u_value - 128;
            int e = v_value - 128;
            
            // 使用 BT.601 标准转换系数
            int r = (298 * c + 409 * e + 128) >> 8;
            int g = (298 * c - 100 * d - 208 * e + 128) >> 8;
            int b = (298 * c + 516 * d + 128) >> 8;
            
            // 限制在 0-255 范围内
            r = r < 0 ? 0 : (r > 255 ? 255 : r);
            g = g < 0 ? 0 : (g > 255 ? 255 : g);
            b = b < 0 ? 0 : (b > 255 ? 255 : b);
            
            // 设置 RGBA 值
            int rgba_index = (i * width + j) * 4;
            rgba[rgba_index] = b;     // R
            rgba[rgba_index + 1] = g; // G
            rgba[rgba_index + 2] = r; // B
            rgba[rgba_index + 3] = 255; // A
        }
    }
}

// RGB565 to BGRA8888 conversion
void convert_rgb565_to_bgra8888(const uint16_t *src, uint8 *dst, int width, int height) {
    if (!src || !dst) {
        return;
    }

    for (int i = 0; i < width * height; i++) {
        uint16_t pixel = src[i];

        // Extract RGB565 components
        uint8_t r = (pixel >> 11) & 0x1F; // Red: 5 bits
        uint8_t g = (pixel >> 5) & 0x3F;  // Green: 6 bits
        uint8_t b = pixel & 0x1F;         // Blue: 5 bits

        // Expand to 8 bits
        r = (r << 3) | (r >> 2); // 5-bit to 8-bit
        g = (g << 2) | (g >> 4); // 6-bit to 8-bit
        b = (b << 3) | (b >> 2); // 5-bit to 8-bit

        dst[i*4] = b;
        dst[i*4 + 1] = g;
        dst[i*4 + 2] = r;
        dst[i*4 + 3] = 0xFF;
    }
}

/*********************************************************************
  Function:        divoom_decode_movie_get_movie_first_frame_buf
  Description:     生成视频的预览图的第一帧图像缓冲区
  Input:           movie_name      路径要带file://+本地路径
                   image_w         控件宽度
                   image_h         控件高度
  Return:          第一帧缓冲区
**********************************************************************/

uint8 *divoom_decode_movie_get_movie_first_frame_buf(char *movie_name, int *image_w, int *image_h)
{
    #if 0
    uint8 *BGRA_buf, *mYuvData;
    TRetriever* metedataretriever;
    TmetadataRetrieverScaleDownRatio scaleRatio;
    TmetadataRetrieverOutputDataType outputDataTpe;
    int duration = 0, len;
    char *play_movie_name, *jpg_image_name;

    len = strlen(movie_name);
    jpg_image_name = divoom_app_com_copy_string(movie_name, len + 10);
    jpg_image_name[len - 3] = 'w';
    jpg_image_name[len - 2] = 'e';
    jpg_image_name[len - 1] = 'b';
    jpg_image_name[len] = 'p';
    jpg_image_name[len + 1] = 0;
    if(divoom_app_com_check_file_exist(jpg_image_name))
    {
        BGRA_buf = divoom_libwebp_decode_get_image_rgb_buf(jpg_image_name, image_w, image_h);
        if(BGRA_buf)
        {
            divoom_system_free(jpg_image_name);
            return BGRA_buf;
        }
    }
    metedataretriever = TRetrieverCreate();

    if(NULL == metedataretriever)
    {
        divoom_print("create metedataretriever failed\n");
        divoom_system_free(jpg_image_name);
        return NULL;
    }
    
    if(strstr(movie_name, "file://"))
    {
        play_movie_name = divoom_app_com_copy_string(movie_name, 0);
    }
    else
    {
        play_movie_name = divoom_system_malloc(strlen(movie_name) + 10);
        strcpy(play_movie_name, "file://");
        strcat(play_movie_name, movie_name);
    }
    
    scaleRatio = TMETADATA_RETRIEVER_SCALE_DOWN_1;      //输出原始图像，再进行缩放
    outputDataTpe = TmetadataRetrieverOutputDataRGB565;

    TRetrieverSetDataSource(metedataretriever, play_movie_name,scaleRatio,outputDataTpe);
    divoom_print("TRetrieverSetDataSource end\n");

    TRetrieverGetMetaData(metedataretriever, TMETADATA_VIDEO_WIDTH, image_w);
    TRetrieverGetMetaData(metedataretriever, TMETADATA_VIDEO_HEIGHT, image_h);
    TRetrieverGetMetaData(metedataretriever, TMETADATA_DURATION, &duration);
    divoom_print("origin_width = %d,origin_height = %d,duration = %dms\n",*image_w,*image_h,duration);

    duration = duration/2;

    VideoFrame* videoFrame = TRetrieverGetFrameAtTime(metedataretriever,0);
    if(videoFrame == NULL){
        divoom_print("TRetrieverGetFrameAtTime fail: %s\n", play_movie_name);
        TRetrieverDestory(metedataretriever);
        if(*image_w == 0 || *image_h == 0)
        {
            *image_w = 800;
            *image_h = 1280;
        }
        
    	BGRA_buf = divoom_system_malloc((*image_w) * (*image_h) * 4);
        
        return BGRA_buf;
    }
    //修改可能视频显示大小与视频原始大小不一致的问题，取最小值
    if(*image_w > videoFrame->mWidth)
    {
        *image_w = videoFrame->mWidth;
    }
    if(*image_w > videoFrame->mWidth)
    {
        *image_h = videoFrame->mHeight;
    }
    
    
    divoom_print("out size: %d, %d, %d\n", videoFrame->mRGB565Size, videoFrame->mWidth, videoFrame->mHeight);
    BGRA_buf = divoom_system_malloc((*image_w) * (*image_h) * 4);
    convert_rgb565_to_bgra8888((uint16_t *)videoFrame->mRGB565Data, BGRA_buf, *image_w, *image_h);
    TRetrieverDestory(metedataretriever);
    divoom_system_free(play_movie_name);
    divoom_pic_phone_manage_encode_picture(jpg_image_name, *image_w, *image_h, BGRA_buf);
    divoom_system_free(jpg_image_name);
    
    return BGRA_buf;
    #else
    return NULL;
    #endif
}

/*********************************************************************
  Function:        divoom_decode_movie_review_pic_get_buf
  Description:     生成视频的预览图的缓冲区,全志版本，不考虑旋转与镜像
  Input:           movie_name      路径要带file://+本地路径
                   review_name     本地路径
                   image_w         控件宽度
                   image_h         控件高度
                   rotate_flag     旋转
                   mirror_flag     镜像
  Return:          none
**********************************************************************/

void divoom_decode_movie_review_pic_get_buf(char *movie_name, uint8 *image_data, int image_w, int image_h, int rotate_flag, int mirror_flag)
{
    uint8 *BGRA_buf;
    int src_w = 0, src_h = 0;
    
    BGRA_buf = divoom_decode_movie_get_movie_first_frame_buf(movie_name, &src_w, &src_h);
    if(BGRA_buf == NULL || src_w <= 0 || src_h <= 0)
    {
        if(image_data != NULL && image_w > 0 && image_h > 0)
        {
            memset(image_data, 0, (size_t)image_w * (size_t)image_h *
                DIVOOM_APP_ARGB8888_BYTES_PER_PX);
        }
        return;
    }
    
    divoom_rk_mpp_get_disp_image_info(BGRA_buf, src_w, src_h, RK_FORMAT_BGRA_8888, image_data, &image_w, &image_h, RK_FORMAT_BGRA_8888, 0);    
    
    divoom_system_free(BGRA_buf);
}

void divoom_decode_movie_review_pic_get_lvfmt_buf(char *movie_name,
                                                  uint8 *image_data,
                                                  int image_w,
                                                  int image_h,
                                                  int rotate_flag,
                                                  int mirror_flag,
                                                  lv_color_format_t cf)
{
    uint8 *tmp_bgra;

    if(image_data == NULL || image_w <= 0 || image_h <= 0)
    {
        return;
    }
    if(cf != LV_COLOR_FORMAT_RGB565 && cf != LV_COLOR_FORMAT_RGB565A8)
    {
        return;
    }

    tmp_bgra = divoom_system_malloc((size_t)image_w * (size_t)image_h *
        DIVOOM_APP_ARGB8888_BYTES_PER_PX);
    if(tmp_bgra == NULL)
    {
        return;
    }
    divoom_decode_movie_review_pic_get_buf(movie_name, tmp_bgra, image_w,
        image_h, rotate_flag, mirror_flag);
    divoom_rk_mpp_stretch_image_lvfmt(tmp_bgra, image_w, image_h,
        RK_FORMAT_BGRA_8888, image_data, image_w, image_h, cf);
    divoom_system_free(tmp_bgra);
}

typedef struct _divoom_decode_movie_review_pic_attr_info
{
    char *movie_name;
    char *review_name;
    int image_w;
    int image_h;
    int rotate_flag;
    int mirror_flag;
}divoom_decode_movie_review_pic_attr_info, *Pdivoom_decode_movie_review_pic_attr_info;
/*********************************************************************
  Function:        divoom_decode_movie_review_pic_create_delay_run
  Description:     生成视频的预览图的定时回调
                   para
  Return:          none
**********************************************************************/
int divoom_decode_movie_review_pic_create_delay_run(void *para)
{
    if(divoom_local_movie_play_is_playing() == 0)
    {
        Pdivoom_decode_movie_review_pic_attr_info ppic_info = (Pdivoom_decode_movie_review_pic_attr_info)para;
        uint8 *image_buf = divoom_system_malloc(ppic_info->image_w * ppic_info->image_h * 4);

        if(image_buf)
        {
            divoom_decode_movie_review_pic_get_buf(ppic_info->movie_name, image_buf, ppic_info->image_w, ppic_info->image_h, ppic_info->rotate_flag, ppic_info->mirror_flag);
            divoom_pic_phone_manage_encode_picture(ppic_info->review_name, ppic_info->image_w, ppic_info->image_h, image_buf);
            divoom_system_free(image_buf);
        }
        divoom_system_free(ppic_info->movie_name);
        divoom_system_free(ppic_info->review_name);
        divoom_system_free(ppic_info);
    }
    else
    {
        //如果在播放视频，将继续等待, 向DIVOOM_MAIN
        divoom_sys_msg_info model_msg= {0};

        model_msg.msg_type = 1;
    	model_msg.src_id = DIVOOM_MODEL_NET_CHANNEL; 
    	model_msg.dest_id = DIVOOM_MODEL_MAIN; 
    	model_msg.msg_id = DIVOOM_MAIN_EVENT_CREATE_MOIVE_REVIEW_PIC;
    	model_msg.data = para;
    	divoom_sys_model_crl_msg_push(&model_msg);
    }
    return 0;
}

/*********************************************************************
  Function:        divoom_decode_movie_review_pic_create
  Description:     生成视频的预览图
  Input:           movie_name      路径要带file://+本地路径
                   review_name     本地路径
                   image_w         控件宽度
                   image_h         控件高度
                   rotate_flag     旋转
                   mirror_flag     镜像
  Return:          none
**********************************************************************/

void divoom_decode_movie_review_pic_create(char *movie_name, char *review_name, int image_w, int image_h, int rotate_flag, int mirror_flag)
{
    //如果播放视频的时候创建预览图，会导致系统性资源不够
    int  len;
    char *jpg_image_name;

    len = strlen(movie_name);
    jpg_image_name = divoom_app_com_copy_string(movie_name, len + 10);
    jpg_image_name[len - 3] = 'w';
    jpg_image_name[len - 2] = 'e';
    jpg_image_name[len - 1] = 'b';
    jpg_image_name[len] = 'p';
    jpg_image_name[len + 1] = 0;
    //APP发了视频预览图过来，或者不在播放视频就直接创建预览图
    if(divoom_local_movie_play_is_playing() == 0 || divoom_app_com_check_file_exist(jpg_image_name))
    {
        uint8 *image_buf = divoom_system_malloc(image_w * image_h * 4);

        if(image_buf)
        {
            
            divoom_decode_movie_review_pic_get_buf(movie_name, image_buf, image_w, image_h, rotate_flag, mirror_flag);
            
            divoom_pic_phone_manage_encode_picture(review_name, image_w, image_h, image_buf);
            
            divoom_system_free(image_buf);
        }
    }
    else
    {
        //将延后一些创建预览图
        Pdivoom_decode_movie_review_pic_attr_info ppic_info;

        ppic_info = divoom_system_malloc(sizeof(divoom_decode_movie_review_pic_attr_info));
        if(ppic_info)
        {
            ppic_info->movie_name = divoom_app_com_copy_string(movie_name, 0);
            ppic_info->review_name = divoom_app_com_copy_string(review_name, 0);
            ppic_info->image_w = image_w;
            ppic_info->image_h = image_h;
            ppic_info->rotate_flag = rotate_flag;
            ppic_info->mirror_flag = mirror_flag;
            divoom_software_timer_create_timer(4, divoom_decode_movie_review_pic_create_delay_run, ppic_info);
        }
    }
    divoom_system_free(jpg_image_name);
}



