#include "divoom.h"
#include "lvgl.h"
#include "divoom_menu_control.h"
#include "divoom_pixel_format.h"

typedef struct _divoom_lvgl_play_gif_gaussian_info
{
    int src_w;
    int src_h;
    uint8 *src_data;
    int dest_w;
    int dest_h;
    uint8 *dest_data;
    int value;
    int gaussian_flag;  //需要做高斯
}divoom_lvgl_play_gif_gaussian_info, *Pdivoom_lvgl_play_gif_gaussian_info;

typedef struct _divoom_lvgl_play_gif_file_ctrl
{
    uint8 run_flag;
    uint8 play_flag;
    uint8 bind_external_image_buf;
    int image_w;
    int image_h;
    int dest_w;
    int dest_h;
    int refresh_flag;
    int file_type;  //文件类型 0:webp; 1:gif
    int end_play_ms;
    int frame_speed;
    int read_frame_num;
    int single_frame_flag;
    int frame_num;
    uint8 *rgb_buf;
    void *exit_sem;
    void *gif_handle;   //解码句柄
    void *image_obj;    //控件句柄
    void *time_handle;
    uint8 *image_buf;
    uint8 *gaussian_buf;
    void  *gaussian_sem;
    divoom_lvgl_play_gif_gaussian_info gaussian_info;
    lv_img_dsc_t *image_data;
    int width;
    int high;
}divoom_lvgl_play_gif_file_ctrl, *Pdivoom_lvgl_play_gif_file_ctrl;

void divoom_lvgl_play_gif_file_format_out_buf(Pdivoom_lvgl_play_gif_file_ctrl  phandle, uint8 *src_buf, int src_w, int src_h, uint8 *out_buf, int out_w, int out_h, int lvgl_lock, int gaussian_flag);

static void divoom_lvgl_play_gif_file_write_lv_img(lv_img_dsc_t *image_data,
    const uint8 *src_bgra, int w, int h)
{
    if(image_data == NULL || image_data->data == NULL || src_bgra == NULL)
        return;

    if(image_data->header.cf == LV_COLOR_FORMAT_RGB565A8)
        divoom_bgra8888_to_rgb565a8_buf(src_bgra, (uint8 *)image_data->data,
            (uint32)w * (uint32)h);
    else if(image_data->header.cf == LV_COLOR_FORMAT_RGB565)
        divoom_xrgb8888_to_rgb565_buf(src_bgra, (uint8 *)image_data->data,
            (uint32)w * (uint32)h);
    else
        memcpy((uint8 *)image_data->data, src_bgra,
            (size_t)w * (size_t)h * 4u);
}

static void divoom_lvgl_play_gif_file_copy_lv_img(lv_img_dsc_t *image_data,
    const uint8 *src_lvfmt)
{
    if(image_data == NULL || image_data->data == NULL || src_lvfmt == NULL)
        return;
    memcpy((uint8 *)image_data->data, src_lvfmt, image_data->data_size);
}

/* OOBE 等小图 GIF：复用 image_data->data，避免重复 malloc 全屏缓冲 */
static void divoom_lvgl_play_gif_file_bind_image_buf(Pdivoom_lvgl_play_gif_file_ctrl phandle)
{
    if(phandle->image_buf != NULL)
    {
        return;
    }
    if(phandle->image_data != NULL && phandle->image_data->data != NULL)
    {
        phandle->image_buf = (uint8 *)phandle->image_data->data;
        return;
    }
    phandle->image_buf = divoom_system_malloc(phandle->image_data->data_size);
}

static int divoom_lvgl_play_gif_file_image_buf_owned(Pdivoom_lvgl_play_gif_file_ctrl phandle)
{
    if(phandle == NULL || phandle->image_buf == NULL || phandle->image_data == NULL)
    {
        return 0;
    }
    return phandle->image_buf != (uint8 *)phandle->image_data->data;
}

void divoom_lvgl_play_gif_file_read_frame_buf(Pdivoom_lvgl_play_gif_file_ctrl  phandle, int lvgl_lock, int gaussian_flag)
{
    uint8 *src_bgra = NULL;
    uint8 *dst_bgra = NULL;

    if(phandle->image_buf == NULL)
    {
        phandle->image_buf = divoom_system_malloc(phandle->image_data->data_size);
        if(phandle->image_buf == NULL)
        {
            return;
        }
    }

    if(phandle->file_type == 0)
    {
        if(phandle->dest_w == phandle->image_w && phandle->dest_h == phandle->image_h &&
            phandle->image_data->header.cf == LV_COLOR_FORMAT_RGB565A8)
        {
            divoom_libwebp_decode_get_frame_rgb565a8_buffer(phandle->gif_handle,
                &phandle->frame_speed, NULL, phandle->image_buf);
        }
        else
        {
            src_bgra = divoom_system_malloc((uint32)phandle->image_w *
                (uint32)phandle->image_h * 4U);
            if(src_bgra == NULL)
            {
                return;
            }
            divoom_libwebp_decode_get_rgb_buffer(phandle->gif_handle,
                &phandle->frame_speed, NULL, src_bgra);
            if(phandle->dest_w != phandle->image_w || phandle->dest_h != phandle->image_h)
            {
                dst_bgra = divoom_system_malloc((uint32)phandle->dest_w *
                    (uint32)phandle->dest_h * 4U);
                if(dst_bgra == NULL)
                {
                    divoom_system_free(src_bgra);
                    return;
                }
                divoom_lvgl_play_gif_file_format_out_buf(NULL, src_bgra,
                    phandle->image_w, phandle->image_h, dst_bgra,
                    phandle->dest_w, phandle->dest_h, lvgl_lock, gaussian_flag);
                divoom_lvgl_play_gif_file_write_lv_img(&(lv_img_dsc_t){
                    .header = phandle->image_data->header,
                    .data_size = phandle->image_data->data_size,
                    .data = phandle->image_buf
                }, dst_bgra, phandle->dest_w, phandle->dest_h);
                divoom_system_free(dst_bgra);
            }
            else
            {
                divoom_lvgl_play_gif_file_write_lv_img(&(lv_img_dsc_t){
                    .header = phandle->image_data->header,
                    .data_size = phandle->image_data->data_size,
                    .data = phandle->image_buf
                }, src_bgra, phandle->dest_w, phandle->dest_h);
            }
            divoom_system_free(src_bgra);
        }
    }
    else if(phandle->file_type == 1)
    {
        uint16 gif_speed;
        lv_color_format_t gif_cf = phandle->image_data->header.cf ==
            LV_COLOR_FORMAT_RGB565A8 ? LV_COLOR_FORMAT_RGB565A8 :
            LV_COLOR_FORMAT_RGB565;
        int gif_alpha_flag = gif_cf == LV_COLOR_FORMAT_RGB565A8;
        uint32 src_lv_size = divoom_pixel_format_size(
            (uint32)phandle->image_w, (uint32)phandle->image_h, gif_cf);

        if(phandle->single_frame_flag == 1)
        {
            return;
        }
        if(phandle->image_w == phandle->dest_w && phandle->image_h == phandle->dest_h)
        {
            src_bgra = divoom_system_malloc(src_lv_size);
            if(src_bgra == NULL)
            {
                return;
            }
            if(0 == divoom_gif_lib_get_pic_data(phandle->gif_handle, src_bgra, NULL, &gif_speed, 1, gif_alpha_flag))
            {
                if(phandle->read_frame_num < 2)
                {
                    phandle->single_frame_flag = 1;
                    divoom_system_free(src_bgra);
                    return;
                }
                divoom_gif_lib_reset(phandle->gif_handle, 0);
                divoom_gif_lib_get_pic_data(phandle->gif_handle, src_bgra, NULL, &gif_speed, 1, gif_alpha_flag);
            }
            else if(phandle->read_frame_num < 2)
            {
                phandle->read_frame_num ++;
            }
            divoom_lvgl_play_gif_file_copy_lv_img(&(lv_img_dsc_t){
                .header = phandle->image_data->header,
                .data_size = phandle->image_data->data_size,
                .data = phandle->image_buf
            }, src_bgra);
            divoom_system_free(src_bgra);
        }
        else
        {
            src_bgra = divoom_system_malloc(src_lv_size);
            if(src_bgra == NULL)
            {
                return;
            }
            if(0 == divoom_gif_lib_get_pic_data(phandle->gif_handle, src_bgra, NULL, &gif_speed, 1, gif_alpha_flag))
            {
                if(phandle->read_frame_num < 2)
                {
                    phandle->single_frame_flag = 1;
                    divoom_system_free(src_bgra);
                    return;
                }
                divoom_gif_lib_reset(phandle->gif_handle, 0);
                divoom_gif_lib_get_pic_data(phandle->gif_handle, src_bgra, NULL, &gif_speed, 1, gif_alpha_flag);
            }
            else if(phandle->read_frame_num < 2)
            {
                phandle->read_frame_num ++;
            }
            divoom_rk_mpp_stretch_image_lvfmt(src_bgra,
                phandle->image_w, phandle->image_h, RK_FORMAT_RGB_565,
                phandle->image_buf, phandle->dest_w, phandle->dest_h,
                phandle->image_data->header.cf);
            if(gif_alpha_flag &&
                phandle->image_data->header.cf == LV_COLOR_FORMAT_RGB565A8)
            {
                uint8 *src_alpha = src_bgra +
                    (uint32)phandle->image_w * (uint32)phandle->image_h * 2U;
                uint8 *dst_alpha = phandle->image_buf +
                    (uint32)phandle->dest_w * (uint32)phandle->dest_h * 2U;

                for(int y = 0; y < phandle->dest_h; y++)
                {
                    int src_y = y * phandle->image_h / phandle->dest_h;
                    for(int x = 0; x < phandle->dest_w; x++)
                    {
                        int src_x = x * phandle->image_w / phandle->dest_w;
                        dst_alpha[y * phandle->dest_w + x] =
                            src_alpha[src_y * phandle->image_w + src_x];
                    }
                }
            }
            divoom_system_free(src_bgra);
        }
        phandle->frame_speed = gif_speed;
    }
    phandle->refresh_flag = 1;
}

static void divoom_lvgl_play_gif_file_read_frame_buf_reuse(
    Pdivoom_lvgl_play_gif_file_ctrl phandle, int lvgl_lock, int gaussian_flag)
{
    uint8 *src_bgra = NULL;
    uint8 *dst_bgra = NULL;

    divoom_lvgl_play_gif_file_bind_image_buf(phandle);
    if(phandle->image_buf == NULL)
    {
        return;
    }

    if(phandle->file_type == 0)
    {
        if(phandle->dest_w == phandle->image_w && phandle->dest_h == phandle->image_h &&
            phandle->image_data->header.cf == LV_COLOR_FORMAT_RGB565A8)
        {
            divoom_libwebp_decode_get_frame_rgb565a8_buffer(phandle->gif_handle,
                &phandle->frame_speed, NULL, phandle->image_buf);
        }
        else
        {
            src_bgra = divoom_system_malloc((uint32)phandle->image_w *
                (uint32)phandle->image_h * 4U);
            if(src_bgra == NULL)
            {
                return;
            }
            divoom_libwebp_decode_get_rgb_buffer(phandle->gif_handle,
                &phandle->frame_speed, NULL, src_bgra);
            if(phandle->dest_w != phandle->image_w || phandle->dest_h != phandle->image_h)
            {
                dst_bgra = divoom_system_malloc((uint32)phandle->dest_w *
                    (uint32)phandle->dest_h * 4U);
                if(dst_bgra == NULL)
                {
                    divoom_system_free(src_bgra);
                    return;
                }
                divoom_lvgl_play_gif_file_format_out_buf(NULL, src_bgra,
                    phandle->image_w, phandle->image_h, dst_bgra,
                    phandle->dest_w, phandle->dest_h, lvgl_lock, gaussian_flag);
                divoom_lvgl_play_gif_file_write_lv_img(&(lv_img_dsc_t){
                    .header = phandle->image_data->header,
                    .data_size = phandle->image_data->data_size,
                    .data = phandle->image_buf
                }, dst_bgra, phandle->dest_w, phandle->dest_h);
                divoom_system_free(dst_bgra);
            }
            else
            {
                divoom_lvgl_play_gif_file_write_lv_img(&(lv_img_dsc_t){
                    .header = phandle->image_data->header,
                    .data_size = phandle->image_data->data_size,
                    .data = phandle->image_buf
                }, src_bgra, phandle->dest_w, phandle->dest_h);
            }
            divoom_system_free(src_bgra);
        }
    }
    else if(phandle->file_type == 1)
    {
        uint16 gif_speed;
        lv_color_format_t gif_cf = phandle->image_data->header.cf ==
            LV_COLOR_FORMAT_RGB565A8 ? LV_COLOR_FORMAT_RGB565A8 :
            LV_COLOR_FORMAT_RGB565;
        int gif_alpha_flag = gif_cf == LV_COLOR_FORMAT_RGB565A8;
        uint32 src_lv_size = divoom_pixel_format_size(
            (uint32)phandle->image_w, (uint32)phandle->image_h, gif_cf);

        if(phandle->single_frame_flag == 1)
        {
            return;
        }
        if(phandle->image_w == phandle->dest_w && phandle->image_h == phandle->dest_h)
        {
            uint8 *decode_buf = phandle->image_buf;
            int decode_owned = 0;

            if(divoom_lvgl_play_gif_file_image_buf_owned(phandle))
            {
                decode_buf = divoom_system_malloc(src_lv_size);
                if(decode_buf == NULL)
                {
                    return;
                }
                decode_owned = 1;
            }
            if(0 == divoom_gif_lib_get_pic_data(phandle->gif_handle, decode_buf, NULL, &gif_speed, 1, gif_alpha_flag))
            {
                if(phandle->read_frame_num < 2)
                {
                    phandle->single_frame_flag = 1;
                    if(decode_owned)
                    {
                        divoom_system_free(decode_buf);
                    }
                    return;
                }
                divoom_gif_lib_reset(phandle->gif_handle, 0);
                divoom_gif_lib_get_pic_data(phandle->gif_handle, decode_buf, NULL, &gif_speed, 1, gif_alpha_flag);
            }
            else if(phandle->read_frame_num < 2)
            {
                phandle->read_frame_num ++;
            }
            if(decode_owned)
            {
                divoom_lvgl_play_gif_file_copy_lv_img(&(lv_img_dsc_t){
                    .header = phandle->image_data->header,
                    .data_size = phandle->image_data->data_size,
                    .data = phandle->image_buf
                }, decode_buf);
                divoom_system_free(decode_buf);
            }
        }
        else
        {
            src_bgra = divoom_system_malloc(src_lv_size);
            if(src_bgra == NULL)
            {
                return;
            }
            if(0 == divoom_gif_lib_get_pic_data(phandle->gif_handle, src_bgra, NULL, &gif_speed, 1, gif_alpha_flag))
            {
                if(phandle->read_frame_num < 2)
                {
                    phandle->single_frame_flag = 1;
                    divoom_system_free(src_bgra);
                    return;
                }
                divoom_gif_lib_reset(phandle->gif_handle, 0);
                divoom_gif_lib_get_pic_data(phandle->gif_handle, src_bgra, NULL, &gif_speed, 1, gif_alpha_flag);
            }
            else if(phandle->read_frame_num < 2)
            {
                phandle->read_frame_num ++;
            }
            divoom_rk_mpp_stretch_image_lvfmt(src_bgra,
                phandle->image_w, phandle->image_h, RK_FORMAT_RGB_565,
                phandle->image_buf, phandle->dest_w, phandle->dest_h,
                phandle->image_data->header.cf);
            if(gif_alpha_flag &&
                phandle->image_data->header.cf == LV_COLOR_FORMAT_RGB565A8)
            {
                uint8 *src_alpha = src_bgra +
                    (uint32)phandle->image_w * (uint32)phandle->image_h * 2U;
                uint8 *dst_alpha = phandle->image_buf +
                    (uint32)phandle->dest_w * (uint32)phandle->dest_h * 2U;

                for(int y = 0; y < phandle->dest_h; y++)
                {
                    int src_y = y * phandle->image_h / phandle->dest_h;
                    for(int x = 0; x < phandle->dest_w; x++)
                    {
                        int src_x = x * phandle->image_w / phandle->dest_w;
                        dst_alpha[y * phandle->dest_w + x] =
                            src_alpha[src_y * phandle->image_w + src_x];
                    }
                }
            }
            divoom_system_free(src_bgra);
        }
        phandle->frame_speed = gif_speed;
    }
    phandle->refresh_flag = 1;
}

static void divoom_lvgl_play_gif_file_read_frame_dispatch(
    Pdivoom_lvgl_play_gif_file_ctrl phandle, int lvgl_lock, int gaussian_flag)
{
    if(phandle != NULL && phandle->bind_external_image_buf)
    {
        divoom_lvgl_play_gif_file_read_frame_buf_reuse(phandle, lvgl_lock, gaussian_flag);
    }
    else
    {
        divoom_lvgl_play_gif_file_read_frame_buf(phandle, lvgl_lock, gaussian_flag);
    }
}

void divoom_lvgl_play_gif_file_refresh_image_obj(struct _lv_timer_t *user_data);

void* divoom_lvgl_play_gif_file_task_main(void *param)
{
    int run_tick = 0, cur_sys_tick = 0;
    Pdivoom_lvgl_play_gif_file_ctrl phandle = (Pdivoom_lvgl_play_gif_file_ctrl)param;
    
    while(phandle->run_flag)
    {

        if(phandle->time_handle == NULL)
        {
            if(divoom_get_lvgl_try_lock() == 0)
            {
                if(phandle->play_flag && phandle->run_flag)
                {
                    phandle->time_handle = lv_timer_create(divoom_lvgl_play_gif_file_refresh_image_obj, 1, phandle);
                }
                divoom_free_lvgl_lock();
            }
            
        }
        if(phandle->play_flag)
        {
            cur_sys_tick = divoom_system_get_tickms();
            if(phandle->end_play_ms <= cur_sys_tick)
            {
                if(phandle->run_flag && phandle->refresh_flag == 0)
                {
                    divoom_lvgl_play_gif_file_read_frame_dispatch(phandle, 1, 1);
                    phandle->end_play_ms +=  phandle->frame_speed;
                }
                run_tick = 0;
            }
        }
        if(phandle->run_flag)
        {
            divoom_system_delay_ms(5);
        }
    }    
    divoom_system_put_sem(phandle->exit_sem);
    return NULL;
    
}

void divoom_menu_control_init_bgra(uint8 *dest_rgb, int r, int g, int b, int src_w, int src_h)
{
    uint32 value, *bgra_ptr = (uint32 *)dest_rgb;

    value = (0xFF << 24) + (r << 0) + (g << 8) + (b << 16);
    for(int i = 0; i < src_h * src_w; i++)
    {
        *bgra_ptr = value;
        bgra_ptr ++;
    }
}

void divoom_lvgl_play_gif_file_format_out_buf(Pdivoom_lvgl_play_gif_file_ctrl phandle, uint8 *src_buf, int src_w, int src_h, uint8 *out_buf, int out_w, int out_h, int lvgl_lock, int gaussian_flag)
{
    int mid_w, mid_h;
    int src_start_x, src_start_y, src_size_x, src_size_y;
    int dest_start_x, dest_start_y, dest_size_x, dest_size_y;

    if(src_w == out_w && src_h == out_h)
    {
        memcpy((uint8 *)out_buf, src_buf, src_w *src_h *4);
    }
    else
    {
        #if 1
        divoom_menu_control_init_bgra(out_buf, 0, 0, 0, out_w, out_h);
        #else
        Pdivoom_pic_phone_manage_slide_config pslide_config = divoom_pic_phone_manage_get_slide_config();
        

        if(pslide_config->bg_type == 0 && gaussian_flag)
        {
            if(phandle)
            {
                if(phandle->gaussian_buf == NULL)
                {
                    phandle->gaussian_buf = divoom_system_malloc(out_w *out_h *4);
                    divoom_menu_control_get_gaussian_buf(src_buf, src_w, src_h,phandle->gaussian_buf,out_w, out_h, 80, &phandle->run_flag);                        
                }
                memcpy(out_buf, phandle->gaussian_buf, out_w *out_h *4);
            }
            else
            {
                uint8 run_flag = 1;
                divoom_menu_control_get_gaussian_buf(src_buf, src_w, src_h,out_buf,out_w, out_h, 80, &run_flag);  
            }
        }
        else if(pslide_config->bg_type == 1)
        {
            divoom_menu_control_init_bgra(out_buf, 0, 0, 0, out_w, out_h);
        }
        else 
        {
            divoom_menu_control_init_bgra(out_buf, 0xFF, 0xFF, 0xFF, out_w, out_h);
        }
        #endif
        
        if(divoom_rk_mpp_get_stretch_image_width_high(src_w, src_h, out_w, out_h, &mid_w, &mid_h))
        {
            src_start_x = 0;
            src_start_y = 0;
            src_size_x = src_w;
            src_size_y = src_h;
            
            dest_start_x = (out_w - mid_w)/2;
            dest_start_y = (out_h - mid_h)/2;
            dest_size_x = mid_w;
            dest_size_y = mid_h;
        }
        else
        {
            src_start_x = 0;
            src_start_y = 0;
            src_size_x = src_w;
            src_size_y = src_h;
            dest_start_x = 0;
            dest_start_y = 0;
            dest_size_x = out_w;
            dest_size_y = out_h;
        }
        divoom_rk_mpp_crop_stretch_image_direct((phandle && phandle->file_type == 0)?G2D_FORMAT_ABGR8888:G2D_FORMAT_ARGB8888, src_buf , src_w, src_h, src_start_x, src_start_y, src_size_x, src_size_y, out_buf, out_w, out_h, dest_start_x, dest_start_y, dest_size_x, dest_size_y);
    }
}

static int divoom_lvgl_play_gif_file_scale_rgb565a8(
    const uint8 *src_rgb565a8, int src_w, int src_h,
    lv_img_dsc_t *image_data, int dest_w, int dest_h)
{
    int mid_w, mid_h;
    int dest_start_x = 0;
    int dest_start_y = 0;
    lv_color_format_t dest_cf;

    if(src_rgb565a8 == NULL || image_data == NULL || image_data->data == NULL ||
        src_w <= 0 || src_h <= 0 || dest_w <= 0 || dest_h <= 0 ||
        (image_data->header.cf != LV_COLOR_FORMAT_RGB565A8 &&
         image_data->header.cf != LV_COLOR_FORMAT_RGB565))
    {
        return 0;
    }

    dest_cf = (lv_color_format_t)image_data->header.cf;
    if(!divoom_rk_mpp_get_stretch_image_width_high(src_w, src_h,
        dest_w, dest_h, &mid_w, &mid_h))
    {
        mid_w = dest_w;
        mid_h = dest_h;
    }

    dest_start_x = (dest_w - mid_w) / 2;
    dest_start_y = (dest_h - mid_h) / 2;
    if(!divoom_rk_mpp_crop_stretch_image_lvfmt(
        divoom_rk_mpp_convert_color_type(RK_FORMAT_RGB_565),
        (uint8 *)src_rgb565a8, src_w, src_h, 0, 0, src_w, src_h,
        (uint8 *)image_data->data, dest_w, dest_h, dest_start_x,
        dest_start_y, mid_w, mid_h, LV_COLOR_FORMAT_RGB565))
    {
        return 0;
    }
    if(dest_cf == LV_COLOR_FORMAT_RGB565A8)
    {
        const uint8 *src_alpha = src_rgb565a8 +
            (uint32)src_w * (uint32)src_h *
                DIVOOM_APP_RGB565_BYTES_PER_PX;
        uint8 *dest_alpha = (uint8 *)image_data->data +
            (uint32)dest_w * (uint32)dest_h *
                DIVOOM_APP_RGB565_BYTES_PER_PX;

        memset(dest_alpha, 0xFF, (uint32)dest_w * (uint32)dest_h);
        for(int y = 0; y < mid_h; y++)
        {
            int src_y = y * src_h / mid_h;

            for(int x = 0; x < mid_w; x++)
            {
                int src_x = x * src_w / mid_w;

                dest_alpha[(dest_start_y + y) * dest_w +
                    dest_start_x + x] = src_alpha[src_y * src_w + src_x];
            }
        }
    }

    return 1;
}

void divoom_lvgl_play_gif_file_refresh_image_obj(struct _lv_timer_t *user_data)
{
    Pdivoom_lvgl_play_gif_file_ctrl  phandle = (Pdivoom_lvgl_play_gif_file_ctrl)lv_timer_get_user_data(user_data);
    
    if(phandle && phandle->refresh_flag)
    {
        if(phandle->image_buf)
        {
            divoom_lvgl_play_gif_file_copy_lv_img(phandle->image_data,
                phandle->image_buf);
        }
        lv_obj_invalidate(phandle->image_obj);
        phandle->refresh_flag = 0;
    }
}

/*

if(bg_type == 0)
{
    divoom_menu_control_get_black_buf(bg_buf, DIVOOM_SCREEN_WIDTH ,DIVOOM_SCREEN_HIGH, lv_color_black());
}
else if(bg_type == 1)
{
    divoom_menu_control_get_black_buf(bg_buf, DIVOOM_SCREEN_WIDTH ,DIVOOM_SCREEN_HIGH, lv_color_white());
}
else if(bg_type == 2)
{
    divoom_menu_control_get_gaussian_buf(image_buf, *dest_w, *dest_h, bg_buf, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH, 100);
}
else
{
    divoom_menu_control_get_gaussian_buf(image_buf, *dest_w, *dest_h, bg_buf, DIVOOM_SCREEN_WIDTH, DIVOOM_SCREEN_HIGH, 1000);
}

*/
/*********************************************************
  * FUNCTION		: divoom_lvgl_play_gif_file_create_handle
  * 		        : 创建一个文件播放，设计是按动画走的，目前只支持jpg,gif,webp,png
  * IUPUT			file_name
                    image_obj
                    image_data
                    width
                    high
                    lock_flag
                    single_flag   是否只解码第一张
  * OUTPUT			: handle 
  *********************************************************/

void *divoom_lvgl_play_gif_file_create_handle(char *file_name, void *image_obj, lv_img_dsc_t *image_data, int width, int high, int lock_flag, int single_flag)
{
    int src_w = width, src_h = high;
    uint8 *rgb_buf;
    char file_ext_name[16];
    Pdivoom_lvgl_play_gif_file_ctrl  phandle = NULL;

    divoom_app_com_get_file_ex_name(file_name, file_ext_name);
    if(strcasecmp(file_ext_name,"jpg") == 0 || strcasecmp(file_ext_name,"jpeg") == 0)
    {
        divoom_lvgl_play_gif_file_create_handle_jpg_table:
        rgb_buf = divoom_rk_mpp_decode_jpeg_file(file_name, &width, &high);
        if(rgb_buf)
        {
            divoom_rk_mpp_stretch_image_lvfmt(rgb_buf, width, high,
                RK_FORMAT_RGB_565, (uint8 *)image_data->data, src_w, src_h,
                (lv_color_format_t)image_data->header.cf);
            divoom_system_free(rgb_buf);
        }
        else
        {
            divoom_pic_phone_manage_del_err_pic_name(file_name);
        }
    }   
    else if(strcasecmp(file_ext_name,"gif") == 0)
    {
        divoom_lvgl_play_gif_file_create_handle_gif_table:
        phandle = divoom_system_malloc(sizeof(divoom_lvgl_play_gif_file_ctrl));
        if(phandle == NULL)
        {
            return NULL;
        }
        
        phandle->gif_handle = divoom_gif_lib_init(file_name, NULL, 0);
        if(phandle->gif_handle  == NULL)
        {
            divoom_pic_phone_manage_del_err_pic_name(file_name);
            divoom_print("it fails to decode: %s\n", file_name);
            divoom_system_free(phandle);
            return NULL;
        }
        phandle->dest_w = width;
        phandle->dest_h = high;
        phandle->image_data = image_data;
        phandle->image_obj = image_obj;
        phandle->exit_sem = divoom_system_create_sem(1, 0);
        phandle->run_flag = 1;
        phandle->file_type = 1;
        phandle->read_frame_num = 0;
        phandle->frame_num = divoom_gif_lib_get_pic_info(phandle->gif_handle, &phandle->image_w, &phandle->image_h, &phandle->frame_speed );
        if(single_flag)
        {
            divoom_lvgl_play_gif_file_read_frame_buf(phandle, lock_flag, 1);
            if(phandle->image_buf)
                divoom_lvgl_play_gif_file_copy_lv_img(image_data,
                    phandle->image_buf);
            divoom_gif_lib_exit(phandle->gif_handle); 
            if(phandle->rgb_buf)
            {
                divoom_system_free(phandle->rgb_buf);
            }    
            if(phandle->gaussian_buf)
            {
                divoom_system_free(phandle->gaussian_buf);
            }            
            divoom_system_free(phandle);
            return NULL;
        }
        else
        {
            divoom_lvgl_play_gif_file_read_frame_buf(phandle, lock_flag, 1);
            /* Publish the first frame before a slideshow transition can use
             * this image; the refresh timer starts only after start_play(). */
            if(phandle->refresh_flag && phandle->image_buf)
            {
                if(lock_flag)
                {
                    divoom_get_lvgl_lock();
                }
                divoom_lvgl_play_gif_file_copy_lv_img(image_data,
                    phandle->image_buf);
                if(lock_flag)
                {
                    divoom_free_lvgl_lock();
                }
            }
            phandle->end_play_ms = divoom_system_get_tickms() + phandle->frame_speed;  
            divoom_system_create_task(divoom_lvgl_play_gif_file_task_main, "lv_play_gif", phandle, DIVOOM_TASK_STACK_LVGL_PLAY_GIF);
            divoom_print("create: %p\n", phandle);
        }
    }  
    else if(strcasecmp(file_ext_name,"mp4") == 0)
    {
        divoom_lvgl_play_gif_file_create_handle_mp4_table:
        rgb_buf = divoom_decode_movie_get_movie_first_frame_buf(file_name, &src_w, &src_h);
        if(rgb_buf)
        {
            uint8 *tmp_bgra = divoom_system_malloc((uint32)width * (uint32)high * 4U);
            if(tmp_bgra != NULL)
            {
                divoom_lvgl_play_gif_file_format_out_buf(NULL, rgb_buf, src_w, src_h,
                    tmp_bgra, width, high, lock_flag, 1);
                divoom_lvgl_play_gif_file_write_lv_img(image_data, tmp_bgra,
                    width, high);
                divoom_system_free(tmp_bgra);
            }
        }
        divoom_system_free(rgb_buf);
    }
    else if(strcasecmp(file_ext_name,"png") == 0)
    {
        rgb_buf = divoom_rk_mpp_get_jpeg_info(file_name, &width, &high);
        if (rgb_buf && width > 0 && high > 0)
        {
            divoom_lvgl_play_gif_file_write_lv_img(image_data, rgb_buf,
                width, high);
            divoom_system_free(rgb_buf);
        }
        else
        {
            if (rgb_buf)
            {
                divoom_system_free(rgb_buf);
            }
            divoom_pic_phone_manage_del_err_pic_name(file_name);
            divoom_print("it fails to decode: %s\n", file_name);
        }
    }  
    else 
    {
        if(strcasecmp(file_ext_name,"webp"))
        {
            int resource_type = divoom_app_com_get_resouce_type(file_name);

            switch(resource_type)
            {
                case DIVOOM_RESOURCE_TYPE_GIF:
                    goto divoom_lvgl_play_gif_file_create_handle_gif_table;
                    break;
                    
                case DIVOOM_RESOURCE_TYPE_MP4:
                    goto divoom_lvgl_play_gif_file_create_handle_mp4_table;
                    break;
                                        
                case DIVOOM_RESOURCE_TYPE_JPEG_GIF:
                    goto divoom_lvgl_play_gif_file_create_handle_jpg_table;
                    break;
                    
                case DIVOOM_RESOURCE_TYPE_JPEG:
                    goto divoom_lvgl_play_gif_file_create_handle_jpg_table;
                    break;

                default:
                    break;
            }
        }
        else
        {
            int static_decode_ret;
            int static_w;
            int static_h;
            uint8 *static_rgb565a8 = NULL;

            if(image_data == NULL)
            {
                return NULL;
            }
            static_decode_ret = divoom_libwebp_decode_static_file_first_565a8(
                file_name, &static_rgb565a8, &static_w, &static_h);
            if(static_decode_ret > 0)
            {
                if(static_w != width || static_h != high)
                {
                    divoom_lvgl_play_gif_file_scale_rgb565a8(static_rgb565a8,
                        static_w, static_h, image_data, width, high);
                }
                else
                {
                    divoom_rgb565a8_to_lvfmt_nearest(static_rgb565a8,
                        (uint32)static_w, (uint32)static_h,
                        (uint8 *)image_data->data, (uint32)width,
                        (uint32)high,
                        (lv_color_format_t)image_data->header.cf);
                }
                divoom_system_free(static_rgb565a8);
                return NULL;
            }
            if(static_decode_ret < 0)
            {
                divoom_print("it fails to decode static webp: %s\n",
                    file_name);
                return NULL;
            }
        }
        phandle = divoom_system_malloc(sizeof(divoom_lvgl_play_gif_file_ctrl));
        if(phandle == NULL)
        {
            return NULL;
        }
        phandle->gif_handle = divoom_libwebp_decode_init(file_name);

        if(phandle->gif_handle  == NULL)
        {
            divoom_pic_phone_manage_del_err_pic_name(file_name);
            divoom_print("it fails to decode: %s\n", file_name);
            divoom_system_free(phandle);
            return NULL;
        }
        phandle->dest_w = width;
        phandle->dest_h = high;
        phandle->image_data = image_data;
        phandle->image_obj = image_obj;
        phandle->run_flag = 1;
        phandle->file_type = 0;
        phandle->frame_num = divoom_libwebp_decode_get_frame_info(phandle->gif_handle, &phandle->image_w, &phandle->image_h);
        if (phandle->image_w <= 0 || phandle->image_h <= 0)
        {
            divoom_pic_phone_manage_del_err_pic_name(file_name);
            divoom_print("it fails to decode: %s\n", file_name);
            divoom_libwebp_decode_exit(phandle->gif_handle);
            divoom_system_free(phandle);
            return NULL;
        }
        phandle->end_play_ms = divoom_system_get_tickms() + phandle->frame_speed;        
        //暂时不支持WEBP动画
        #if 1
        
        if(phandle->dest_w != phandle->image_w || phandle->dest_h != phandle->image_h)
        {
            uint8 *rgb565a8_buf;
            uint32 rgb565a8_size = divoom_pixel_format_size(
                (uint32)phandle->image_w, (uint32)phandle->image_h,
                LV_COLOR_FORMAT_RGB565A8);

            rgb565a8_buf = divoom_system_malloc(rgb565a8_size);
            if(rgb565a8_buf != NULL)
            {
                divoom_libwebp_decode_get_frame_rgb565a8_buffer(
                    phandle->gif_handle, &phandle->frame_speed, NULL,
                    rgb565a8_buf);
                divoom_lvgl_play_gif_file_scale_rgb565a8(rgb565a8_buf,
                    phandle->image_w, phandle->image_h, image_data,
                    phandle->dest_w, phandle->dest_h);
            }
            divoom_libwebp_decode_exit(phandle->gif_handle);  
            if(phandle->rgb_buf)
            {
                divoom_system_free(phandle->rgb_buf);
            }    
            if(phandle->gaussian_buf)
            {
                divoom_system_free(phandle->gaussian_buf);
            }            
            if(phandle->image_buf)
            {
                divoom_system_free(phandle->image_buf);
            }
            divoom_system_free(phandle);
            if(rgb565a8_buf != NULL)
            {
                divoom_system_free(rgb565a8_buf);
            }
        }
        else
        {
            if(image_data->header.cf == LV_COLOR_FORMAT_RGB565A8)
            {
                divoom_libwebp_decode_get_frame_rgb565a8_buffer(phandle->gif_handle,
                    &phandle->frame_speed, NULL, (uint8 *)image_data->data);
            }
            else
            {
                uint8 *tmp_bgra = divoom_system_malloc((uint32)phandle->dest_w *
                    (uint32)phandle->dest_h * 4U);
                if(tmp_bgra != NULL)
                {
                    divoom_libwebp_decode_get_rgb_buffer(phandle->gif_handle,
                        &phandle->frame_speed, NULL, tmp_bgra);
                    divoom_lvgl_play_gif_file_write_lv_img(image_data,
                        tmp_bgra, phandle->dest_w, phandle->dest_h);
                    divoom_system_free(tmp_bgra);
                }
            }
            divoom_libwebp_decode_exit(phandle->gif_handle);  
            if(phandle->rgb_buf)
            {
                divoom_system_free(phandle->rgb_buf);
            }    
            if(phandle->gaussian_buf)
            {
                divoom_system_free(phandle->gaussian_buf);
            }            
            if(phandle->image_buf)
            {
                divoom_system_free(phandle->image_buf);
            }             
            divoom_system_free(phandle);
        }
        return NULL;
        #else        
        divoom_lvgl_play_gif_file_read_frame_buf(phandle, lock_flag, 1);
        if(phandle->frame_num <=1 )
        {
            //只有一张的，将退出
            divoom_libwebp_decode_exit(phandle->gif_handle);  
            if(phandle->rgb_buf)
            {
                divoom_system_free(phandle->rgb_buf);
            }    
            if(phandle->gaussian_buf)
            {
                divoom_system_free(phandle->gaussian_buf);
            }            
            if(phandle->image_buf)
            {
                divoom_system_free(phandle->image_buf);
            }             
            divoom_system_free(phandle);
            return NULL;
        }
        
        phandle->exit_sem = divoom_system_create_sem(1, 0);
        divoom_system_create_task(divoom_lvgl_play_gif_file_task_main, "lv_play_gif", phandle, DIVOOM_TASK_STACK_LVGL_PLAY_GIF);
        if(lock_flag)
        {
            divoom_get_lvgl_lock();
        }
        phandle->time_handle = lv_timer_create(divoom_lvgl_play_gif_file_refresh_image_obj, 1, phandle);
        if(lock_flag)
        {
            divoom_free_lvgl_lock();
        }
        #endif
        
    }  
    return phandle;
}

/*********************************************************
  * FUNCTION		: divoom_lvgl_play_gif_file_start_play
  * 		        : 开始播放
  * IUPUT			handle
  * OUTPUT			none
  *********************************************************/

void divoom_lvgl_play_gif_file_start_play(void *handle)
{
    Pdivoom_lvgl_play_gif_file_ctrl  phandle = (Pdivoom_lvgl_play_gif_file_ctrl)handle;
    
    if(phandle == NULL)
    {
        return ;
    }
    phandle->play_flag = 1;
}

/*********************************************************
  * FUNCTION		: divoom_lvgl_play_gif_file_destory_handle
  * 		        : 释放相关资源
  * IUPUT			handle
  * OUTPUT			none
  *********************************************************/

/* Caller holds the LVGL lock. Joining the decoder is a separate phase. */
void divoom_lvgl_play_gif_file_retire_handle(void *handle)
{
    Pdivoom_lvgl_play_gif_file_ctrl phandle = handle;

    if(phandle == NULL)
    {
        return;
    }
    phandle->play_flag = 0;
    phandle->run_flag = 0;
    if(phandle->time_handle)
    {
        lv_timer_del(phandle->time_handle);
        phandle->time_handle = NULL;
    }
}

void divoom_lvgl_play_gif_file_destory_handle(void *handle, int lock_flag)
{
    Pdivoom_lvgl_play_gif_file_ctrl  phandle = (Pdivoom_lvgl_play_gif_file_ctrl)handle;

    if(phandle == NULL)
    {
        return ;
    }
    phandle->play_flag = 0;
    phandle->run_flag = 0;
    if(lock_flag)
    {
        divoom_get_lvgl_lock();
    }
    divoom_print("exit: %p\n", phandle);
    divoom_lvgl_play_gif_file_retire_handle(phandle);
    if(lock_flag)
    {
        divoom_free_lvgl_lock();
    }
    divoom_system_get_sem(phandle->exit_sem);
    divoom_system_destroy_sem(phandle->exit_sem);
    if(phandle->file_type == 0)
    {
        divoom_libwebp_decode_exit(phandle->gif_handle);  
    }
    else if(phandle->file_type == 1)
    {
        divoom_gif_lib_exit(phandle->gif_handle);  
    }
    if(phandle->gaussian_buf)
    {
        divoom_system_free(phandle->gaussian_buf);
    }
    if(phandle->rgb_buf)
    {
        divoom_system_free(phandle->rgb_buf);
    }
    if(phandle->image_buf)
    {
        divoom_system_free(phandle->image_buf);
    }
    divoom_system_free(phandle);
}

void *divoom_lvgl_play_gif_file_create_handle_reuse_image_buf(char *file_name,
    void *image_obj, lv_img_dsc_t *image_data, int width, int high,
    int lock_flag, int single_flag)
{
    Pdivoom_lvgl_play_gif_file_ctrl phandle;

    if(file_name == NULL || image_obj == NULL || image_data == NULL ||
        image_data->data == NULL)
    {
        return NULL;
    }
    if(divoom_app_com_get_resouce_type(file_name) != DIVOOM_RESOURCE_TYPE_GIF)
    {
        divoom_print("reuse_image_buf only supports GIF: %s\n", file_name);
        return NULL;
    }

    phandle = divoom_system_malloc(sizeof(divoom_lvgl_play_gif_file_ctrl));
    if(phandle == NULL)
    {
        return NULL;
    }
    memset(phandle, 0, sizeof(*phandle));

    phandle->gif_handle = divoom_gif_lib_init(file_name, NULL, 0);
    if(phandle->gif_handle == NULL)
    {
        divoom_pic_phone_manage_del_err_pic_name(file_name);
        divoom_print("it fails to decode: %s\n", file_name);
        divoom_system_free(phandle);
        return NULL;
    }

    phandle->bind_external_image_buf = 1;
    phandle->dest_w = width;
    phandle->dest_h = high;
    phandle->image_data = image_data;
    phandle->image_obj = image_obj;
    phandle->exit_sem = divoom_system_create_sem(1, 0);
    phandle->run_flag = 1;
    phandle->file_type = 1;
    phandle->read_frame_num = 0;
    phandle->frame_num = divoom_gif_lib_get_pic_info(phandle->gif_handle,
        &phandle->image_w, &phandle->image_h, &phandle->frame_speed);

    if(single_flag)
    {
        divoom_lvgl_play_gif_file_read_frame_buf_reuse(phandle, lock_flag, 1);
        if(phandle->image_buf)
        {
            divoom_lvgl_play_gif_file_copy_lv_img(image_data, phandle->image_buf);
        }
        divoom_gif_lib_exit(phandle->gif_handle);
        if(phandle->rgb_buf)
        {
            divoom_system_free(phandle->rgb_buf);
        }
        if(phandle->gaussian_buf)
        {
            divoom_system_free(phandle->gaussian_buf);
        }
        divoom_system_free(phandle);
        return NULL;
    }

    divoom_lvgl_play_gif_file_read_frame_buf_reuse(phandle, lock_flag, 1);
    phandle->end_play_ms = divoom_system_get_tickms() + phandle->frame_speed;
    divoom_system_create_task(divoom_lvgl_play_gif_file_task_main,
        "lv_play_gif", phandle, DIVOOM_TASK_STACK_LVGL_PLAY_GIF);
    divoom_print("create reuse: %p\n", phandle);
    return phandle;
}

void divoom_lvgl_play_gif_file_destory_handle_reuse_image_buf(void *handle, int lock_flag)
{
    Pdivoom_lvgl_play_gif_file_ctrl phandle = (Pdivoom_lvgl_play_gif_file_ctrl)handle;

    if(phandle == NULL)
    {
        return;
    }
    phandle->play_flag = 0;
    phandle->run_flag = 0;
    if(lock_flag)
    {
        divoom_get_lvgl_lock();
    }
    divoom_print("exit reuse: %p\n", phandle);
    if(phandle->time_handle)
    {
        lv_timer_del(phandle->time_handle);
    }
    if(lock_flag)
    {
        divoom_free_lvgl_lock();
    }
    divoom_system_get_sem(phandle->exit_sem);
    divoom_system_destroy_sem(phandle->exit_sem);
    if(phandle->file_type == 0)
    {
        divoom_libwebp_decode_exit(phandle->gif_handle);
    }
    else if(phandle->file_type == 1)
    {
        divoom_gif_lib_exit(phandle->gif_handle);
    }
    if(phandle->gaussian_buf)
    {
        divoom_system_free(phandle->gaussian_buf);
    }
    if(phandle->rgb_buf)
    {
        divoom_system_free(phandle->rgb_buf);
    }
    if(phandle->image_buf &&
        phandle->image_buf != (uint8 *)phandle->image_data->data)
    {
        divoom_system_free(phandle->image_buf);
    }
    phandle->image_buf = NULL;
    divoom_system_free(phandle);
}
