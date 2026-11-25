#include "divoom_build_config.h"
#if DIVOOM_PIXEL_EQ_ENABLE
#include "divoom_disp_com.h"
#include "divoom.h"
#include "divoom_pixel_format.h"

#define DIVOOM_LIGHT_MUSIC_MALLOC(size)              divoom_com_malloc(size ,__func__, __LINE__)
#define DIVOOM_LIGHT_MUSIC_FREE(size)                divoom_com_free(size ,__func__, __LINE__)
#define DIVOOM_LIGHT_MUSIC_PRINT                     divoom_print
#define DIVOOM_LIGHT_MUSIC_DBG                       divoom_print
#define DIVOOM_LIGHT_MUSIC_ERR                       divoom_print
#define DIVOOM_LIGHT_MUSIC_GET_RAND(x)               divoom_com_get_random(x)
#define DIVOOM_LIGHT_MUSIC_REFRESH_TIME              3
#define DIVOOM_LIGHT_MUSIC_NO_MUSIC_TIME             1000
#define DIVOOM_LIGHT_MUSIC_MAX_AREA                  15
#define DIVOOM_LIGHT_MUSIC_FREQ_BUF                  16

//动画效果的音乐能量等级
#define DIVOOM_LIGHT_MUSIC_GIF_POWER_LEVEL			16
#define DIVOOM_LIGHT_MUSIC_ROW_PIC_BYTES            (SCREEN_DOT_NUM_32 * DIVOOM_APP_RGB565_BYTES_PER_PX)
/* zstd 最大单次申请约 1.5MB，须看 largest 而非 free */
#define DIVOOM_LIGHT_MUSIC_ZSTD_MIN_LARGEST_KB        1600
/* 旧 32/64 点 EQ 先扩展到 256 点，再由 LVGL 链路缩放到 480×480，避免直接放大后的粗糙锯齿。 */
#define DIVOOM_LIGHT_MUSIC_EQ_CANVAS_WIDTH             SCREEN_WIDTH_256
#define DIVOOM_LIGHT_MUSIC_EQ_BAND_COUNT               SCREEN_WIDTH_64
#define DIVOOM_LIGHT_MUSIC_EQ_RADIAL_CENTER_X          128
#define DIVOOM_LIGHT_MUSIC_EQ_RADIAL_CENTER_Y          136
#define DIVOOM_LIGHT_MUSIC_EQ_RADIAL_RADIUS            46
#define DIVOOM_LIGHT_MUSIC_EQ_PARTICLE_COUNT           24

typedef struct _divoom_light_music_row_pic_item_pos
{
    uint8 start_x;
    uint8 width;
}divoom_light_music_row_pic_item_pos, *Pdivoom_light_music_row_pic_item_pos;

typedef struct _divoom_light_music_row_pic_info
{
    int tol;    //总共有多少列
    divoom_light_music_row_pic_item_pos item_list[32];
    uint8 pic_buf[16][DIVOOM_LIGHT_MUSIC_ROW_PIC_BYTES];  /* 内置 EQ 32x32 行图，仅 2KB/帧 */
} divoom_light_music_row_pic_info, *Pdivoom_light_music_row_pic_info;


typedef struct _divoom_light_music_area
{
    int x;  //区域的X坐标
    int y;  //区域的Y坐标
    int edge; //区域的边长
    divoom_light_color_info color;   //区域的颜色
	uint8 speed;
} divoom_light_music_area, *Pdivoom_light_music_area;
const divoom_light_color_info musice_up_color = {0xFF, 0, 0};
const divoom_light_color_info musice_down_color = {0xFF,0xFF,0xFF};


typedef struct _divoom_light_music_freq_queue
{
    uint8  freq_num;
    uint8  disp_info[DIVOOM_LIGHT_MUSIC_FREQ_BUF][SCREEN_DOT_NUM_256 * DIVOOM_APP_RGB565_BYTES_PER_PX];
} divoom_light_music_freq_queue, *Pdivoom_light_music_freq_queue;

typedef struct _divoom_light_music_ctrl
{
    uint8  fft_flag;
    uint8  mix_mode;
    uint8  tick;
    uint8  static_tick;
    uint8  refresh_flag;
    uint8  play_flag;
    uint8  disp_type;
    uint8  music_power;
    uint8  user_tol_num;
    uint8  freq_sum;
    uint8  sea_num;
    uint8  sea_dir;
    int    disp_width;
    uint32 refresh_time;
    uint32 task_id;
    uint32 index;
    uint32 row_tick;
    uint32 pcm_value;
    divoom_light_music_area area_table[DIVOOM_LIGHT_MUSIC_MAX_AREA];
	uint8   rand_num;
	uint8   rand_use;
//    divoom_light_music_freq_queue freq_queue_info;
    uint8  wave_freq_info[SCREEN_WIDTH_64];
    uint8  disp_info[SCREEN_WIDTH_64];
    uint8  back_disp_info[SCREEN_WIDTH_64];
	uint8 top_pos[SCREEN_WIDTH_64];	//存储红色漂浮点的位置
	uint8 down_pos[SCREEN_WIDTH_64];	//存储底部频率柱的位置
	uint8 play_status;		//音乐播放状态
    uint8  disp_buf[SCREEN_DOT_NUM_256 * DIVOOM_APP_RGB565_BYTES_PER_PX];
//    uint8  out_disp_buf[SCREEN_BUF_SIZE_32];
    divoom_clock_id_info    clock_info;
    divoom_light_color_info top_rgb;
    divoom_light_color_info down_rgb;
    divoom_light_music_row_pic_info prow_pic_info;
    int8   custom_eq_stream_level;  /* 流上当前帧索引，-1 表示未解码/已 reset */
    Pdivoom_http_download_image_file_info  prow_pic_ex_image_handle;
    Pdivoom_http_download_image_file_info pic_handle;
} divoom_light_music_ctrl, *Pdivoom_light_music_ctrl;

typedef struct _divoom_light_music_global_ctrl
{
    void  *lock_handle;
    void  *eq_queue;
}divoom_light_music_global_ctrl, *Pdivoom_light_music_global_ctrl;

Pdivoom_light_music_global_ctrl pdivoom_light_music_gobal_handle = NULL;

void divoom_light_music_init_mode(Pdivoom_light_music_ctrl pdivoom_light_music_handle, uint8 disp_type);

static int divoom_light_music_custom_eq_decode_one(Pdivoom_http_download_image_file_info pic_handle, uint8 *dst_buf)
{
    if(pic_handle == NULL || pic_handle->zstd_handle == NULL)
    {
        return 0;
    }
    if(pic_handle->image_type == DIVOOM_IMAGE_BUF_TYPE_ZSTD_ENCODE)
    {
        return divoom_zstd_manage_decode_rgb_buf(pic_handle->zstd_handle, dst_buf) != NULL;
    }
    if(pic_handle->image_type == DIVOOM_IMAGE_BUF_TYPE_ZSTD_WEBP_ENCODE)
    {
        divoom_webp_zstd_mix_decode_rgb_buf(pic_handle->zstd_handle, dst_buf);
        return 1;
    }
    if(pic_handle->image_type == DIVOOM_IMAGE_BUF_TYPE_GIF_ENCODE)
    {
        if(0 == divoom_gif_lib_get_pic_data(pic_handle->zstd_handle, dst_buf, NULL, NULL, 1, 0))
        {
            divoom_gif_lib_reset(pic_handle->zstd_handle, -1);
            divoom_gif_lib_get_pic_data(pic_handle->zstd_handle, dst_buf, NULL, NULL, 1, 0);
        }
        return 1;
    }
    return 0;
}

static int divoom_light_music_custom_eq_reset_stream(Pdivoom_http_download_image_file_info pic_handle)
{
    if(pic_handle == NULL || pic_handle->zstd_handle == NULL)
    {
        return -1;
    }
    if(pic_handle->image_type == DIVOOM_IMAGE_BUF_TYPE_ZSTD_ENCODE)
    {
        return divoom_zstd_manage_reset_stream(pic_handle->zstd_handle);
    }
    if(pic_handle->image_type == DIVOOM_IMAGE_BUF_TYPE_GIF_ENCODE)
    {
        divoom_gif_lib_reset(pic_handle->zstd_handle, -1);
        return 0;
    }
    return -1;
}

static int divoom_light_music_is_custom_eq_mode(Pdivoom_light_music_ctrl handle)
{
    return handle != NULL &&
        handle->disp_type >= DIVOOM_DISP_MUSIC_END_MODE &&
        handle->disp_type != DIVOOM_DISP_MUSIC_TMP_INDEX &&
        handle->pic_handle != NULL &&
        handle->pic_handle->zstd_handle != NULL;
}

static uint16 divoom_light_music_get_disp_level(Pdivoom_light_music_ctrl handle)
{
    uint16 disp_level;

    if(handle == NULL || handle->user_tol_num == 0)
    {
        return 0;
    }
    disp_level = handle->music_power * handle->user_tol_num /
        DIVOOM_LIGHT_MUSIC_GIF_POWER_LEVEL;
    if(disp_level >= handle->user_tol_num)
    {
        disp_level = handle->user_tol_num - 1;
    }
    return disp_level;
}

static int divoom_light_music_custom_eq_decode_to_disp(Pdivoom_light_music_ctrl pdivoom_light_music_handle, uint16 target_level)
{
    int stream_level;
    Pdivoom_http_download_image_file_info pic_handle;

    if(pdivoom_light_music_handle == NULL)
    {
        return 0;
    }
    pic_handle = pdivoom_light_music_handle->pic_handle;
    if(pic_handle == NULL || pdivoom_light_music_handle->user_tol_num == 0)
    {
        return 0;
    }
    if(target_level >= pdivoom_light_music_handle->user_tol_num)
    {
        target_level = pdivoom_light_music_handle->user_tol_num - 1;
    }

    stream_level = pdivoom_light_music_handle->custom_eq_stream_level;
    if(stream_level == (int)target_level)
    {
        return 1;
    }

    if(stream_level >= 0 && (int)target_level < stream_level)
    {
        if(divoom_light_music_custom_eq_reset_stream(pic_handle) != 0)
        {
            return 0;
        }
        stream_level = -1;
        pdivoom_light_music_handle->custom_eq_stream_level = -1;
    }

    while(stream_level < (int)target_level - 1)
    {
        if(divoom_light_music_custom_eq_decode_one(pic_handle, NULL) == 0)
        {
            return 0;
        }
        stream_level++;
    }

    if(divoom_light_music_custom_eq_decode_one(pic_handle, pdivoom_light_music_handle->disp_buf) == 0)
    {
        return 0;
    }
    pdivoom_light_music_handle->custom_eq_stream_level = (int8)target_level;
    if(pic_handle->image_type == DIVOOM_IMAGE_BUF_TYPE_ZSTD_ENCODE)
    {
        divoom_zstd_manage_idle_read(pic_handle->zstd_handle);
    }
    return 1;
}

static void divoom_light_music_custom_eq_render_first_frame(Pdivoom_light_music_ctrl pdivoom_light_music_handle)
{
    if(!divoom_light_music_is_custom_eq_mode(pdivoom_light_music_handle))
    {
        return;
    }
    divoom_light_music_custom_eq_decode_to_disp(pdivoom_light_music_handle, divoom_light_music_get_disp_level(pdivoom_light_music_handle));
}

void divoom_light_music_update_freq(long int *music_freq);

static void divoom_light_music_get_data_565(uint16 off, Pdivoom_light_color_info color, const uint8 *disp_buf)
{
    uint16 c;

    if(color == NULL)
    {
        return;
    }
    if(disp_buf == NULL)
    {
        memset(color, 0, sizeof(divoom_light_color_info));
        return;
    }
    c = (uint16)disp_buf[off * DIVOOM_APP_RGB565_BYTES_PER_PX] |
        ((uint16)disp_buf[off * DIVOOM_APP_RGB565_BYTES_PER_PX + 1U] << 8);
    color->r_c = (uint8)(((c >> 11) & 0x1FU) << 3);
    color->g_c = (uint8)(((c >> 5) & 0x3FU) << 2);
    color->b_c = (uint8)((c & 0x1FU) << 3);
}

static void divoom_light_music_set_data_565(uint16 off, const divoom_light_color_info *color, uint8 *disp_buf)
{
    uint16 c;

    if(color == NULL || disp_buf == NULL)
    {
        return;
    }
    c = divoom_rgb888_to_rgb565(color->r_c, color->g_c, color->b_c);
    divoom_store_rgb565_le(&disp_buf[off * DIVOOM_APP_RGB565_BYTES_PER_PX], c);
}

static void divoom_light_music_set_dot_565(int16 x, int16 y, int16 width,
    int16 height, const divoom_light_color_info *color, uint8 *disp_buf)
{
    if(x < 0 || y < 0 || x >= width || y >= height)
    {
        return;
    }
    divoom_light_music_set_data_565((uint16)(y * width + x), color, disp_buf);
}

#define divoom_disp_ctrl_get_data(off, color, disp_buf) \
    divoom_light_music_get_data_565((off), (color), (disp_buf))
#define divoom_disp_ctrl_set_data(off, color, disp_buf) \
    divoom_light_music_set_data_565((off), (color), (disp_buf))
#define divoom_disp_ctrl_set_data_xy(x, y, color, disp_buf) \
    divoom_light_music_set_dot_565((x), (y), SCREEN_WIDTH_32, SCREEN_HEIGHT_32, (color), (disp_buf))
#define divoom_disp_ctrl_set_dot(x, y, color, disp_buf) \
    divoom_light_music_set_dot_565((x), (y), SCREEN_WIDTH_32, SCREEN_HEIGHT_32, (color), (disp_buf))
#define divoom_disp_ctrl_set_dot_64(x, y, color, disp_buf) \
    divoom_light_music_set_dot_565((x), (y), SCREEN_WIDTH_64, SCREEN_HEIGHT_64, (color), (disp_buf))

void divoom_light_music_global_init(void)
{
    pdivoom_light_music_gobal_handle = divoom_system_malloc(sizeof(divoom_light_music_global_ctrl));
    pdivoom_light_music_gobal_handle->lock_handle = divoom_system_create_lock();
    pdivoom_light_music_gobal_handle->eq_queue = divoom_queue_init(10, __func__, __LINE__);
}

int divoom_light_music_global_get_handle_compare(void *item1, void *item2)
{
    Pdivoom_clock_id_info clock_info = (Pdivoom_clock_id_info)item1;
    Pdivoom_light_music_ctrl pitem = (Pdivoom_light_music_ctrl)item2;

    return memcmp(clock_info, &pitem->clock_info, sizeof(divoom_clock_id_info));
}

void *divoom_light_music_global_get_handle(Pdivoom_clock_id_info clock_info)
{
    return divoom_queue_get_cmp(pdivoom_light_music_gobal_handle->eq_queue, clock_info, divoom_light_music_global_get_handle_compare);
}

void divoom_light_music_global_insert_handle(void *pitem)
{
    divoom_system_get_lock(pdivoom_light_music_gobal_handle->lock_handle);
    divoom_queue_push(pdivoom_light_music_gobal_handle->eq_queue, pitem);
    if(divoom_queue_items(pdivoom_light_music_gobal_handle->eq_queue) == 1)
    {
	    divoom_eq_noise_mic_fft_start(divoom_light_music_update_freq, DIVOOM_FFT_MIC_EQ);
    }
    divoom_system_free_lock(pdivoom_light_music_gobal_handle->lock_handle);
}
void divoom_light_music_close_inner(Pdivoom_light_music_ctrl handle);

void divoom_light_music_global_remove_handle(Pdivoom_clock_id_info clock_info)
{
    int try_times = 0;
    int lock_acquired = 0;
    Pdivoom_light_music_ctrl pitem_handle;
    

    while(try_times < 100)
    {
        if(divoom_system_try_lock(pdivoom_light_music_gobal_handle->lock_handle) == 0)
        {
            lock_acquired = 1;
            break;
        }
        try_times++;
        divoom_system_delay_ms(50);
    }

    if(lock_acquired == 0)
    {
        divoom_print("music remove lock timeout: %d,%d,%d!\n", clock_info->clock_id, clock_info->clock_pos, clock_info->parent_id);
        return;
    }

    pitem_handle = divoom_queue_pop_cmp(pdivoom_light_music_gobal_handle->eq_queue, clock_info, divoom_light_music_global_get_handle_compare);

    if(divoom_queue_items(pdivoom_light_music_gobal_handle->eq_queue) == 0)
    {
	    divoom_eq_noise_mic_fft_stop();
    }
    
    divoom_system_free_lock(pdivoom_light_music_gobal_handle->lock_handle);

    /*
     * close_inner pauses/destroys the microtask. The microtask callback also
     * takes lock_handle, so it must be allowed to finish before we wait for it.
     */
    if(pitem_handle)
    {
        divoom_light_music_close_inner(pitem_handle);
    }
    else
    {
        divoom_print("err close: %d,%d,%d!\n", clock_info->clock_id, clock_info->clock_pos, clock_info->parent_id);
    }

}

static const uint8 divoom_light_music_freq_info[SCREEN_WIDTH_64] = {2,0,1,0,0,3,0,2,1,0,2,0,3,1,0,3,2,0,1,0,0,3,0,2,1,0,2,0,3,1,0,3,2,0,1,0,0,3,0,2,1,0,2,0,3,1,0,3,2,0,1,0,0,3,0,2,1,0,2,0,3,1,0,3};




static const divoom_light_color_info divoom_light_music_normal_color_table[] = {
    {0x08,0x08,0x08},
        {0x08,0x08,0x08},
    {0x0f,0x0f,0x0f},
    {0x0f,0x0f,0x0f},
    {0x1f,0x1f,0x1f},
    {0x18,0x18,0x18},
    {0x18,0x18,0x18},
    {0x28,0x28,0x28},
    {0x28,0x28,0x28},
    {0x2f,0x2f,0x2f},
    {0x2f,0x2f,0x2f},
    {0x38,0x38,0x38},
    {0x38,0x38,0x38},
    {0x3f,0x3f,0x3f},
    {0x3f,0x3f,0x3f},
    {0x48,0x48,0x48},
    {0x48,0x48,0x48},
    {0x4f,0x4f,0x4f},
    {0x4f,0x4f,0x4f},
    {0x58,0x58,0x58},
    {0x58,0x58,0x58},
    {0x5f,0x5f,0x5f},
    {0x5f,0x5f,0x5f},
    {0x68,0x68,0x68},
    {0x68,0x68,0x68},
    {0x6f,0x6f,0x6f},
    {0x6f,0x6f,0x6f},
    {0x78,0x78,0x78},
    {0x78,0x78,0x78},
    {0x7f,0x7f,0x7f},
    {0x7f,0x7f,0x7f},
    {0x88,0x88,0x88},
    {0x88,0x88,0x88},
    {0x8f,0x8f,0x8f},
    {0x8f,0x8f,0x8f},
    {0x98,0x98,0x98},
    {0x98,0x98,0x98},
    {0x9f,0x9f,0x9f},
    {0x9f,0x9f,0x9f},
    {0xa8,0xa8,0xa8},
    {0xa8,0xa8,0xa8},
    {0xaf,0xaf,0xaf},
    {0xaf,0xaf,0xaf},
    {0xb8,0xb8,0xb8},
    {0xb8,0xb8,0xb8},
    {0xbf,0xbf,0xbf},
    {0xbf,0xbf,0xbf},
    {0xc8,0xc8,0xc8},
    {0xc8,0xc8,0xc8},
    {0xcf,0xcf,0xcf},
    {0xcf,0xcf,0xcf},
    {0xd8,0xd8,0xd8},
    {0xd8,0xd8,0xd8},
    {0xdf,0xdf,0xdf},
    {0xdf,0xdf,0xdf},
    {0xe8,0xe8,0xe8},
    {0xe8,0xe8,0xe8},
    {0xef,0xef,0xef},
    {0xef,0xef,0xef},
    {0xf8,0xf8,0xf8},
    {0xf8,0xf8,0xf8},
    {0xff,0xff,0xff},
    {0xff,0xff,0xff},
    };




static const divoom_light_color_info divoom_light_music_left_right_color_table[] =
{
    {0xff, 0x00, 0x36},
    {0xff, 0x00, 0xa0},
    {0xe9, 0x00, 0xff},
    {0x83, 0x00, 0xff},
    {0x18, 0x00, 0xff},
    {0x00, 0x58, 0xff},
    {0x00, 0xc9, 0xff},
    {0x00, 0xff, 0xc5},
    {0x00, 0xff, 0x55},
    {0x0e, 0xff, 0x05},
    {0x7c, 0xff, 0x00},
    {0xf5, 0xff, 0x00},
    {0xff, 0x9d, 0x00},
    {0xff, 0x31, 0x00},
    {0xff, 0x00, 0x00},
    {0xff, 0x00, 0x36},
    {0xff, 0x00, 0xa0},
    {0xe9, 0x00, 0xff},
    {0x83, 0x00, 0xff},
    {0x18, 0x00, 0xff},
    {0x00, 0x58, 0xff},
    {0x00, 0xc9, 0xff},
    {0x00, 0xff, 0xc5},
    {0x00, 0xff, 0x55},
    {0x0e, 0xff, 0x05},
    {0x7c, 0xff, 0x00},
    {0xf5, 0xff, 0x00},
    {0xff, 0x9d, 0x00},
    {0xff, 0x31, 0x00},
    {0xff, 0x00, 0x00},
    {0xff, 0x00, 0x36},
    {0xff, 0x00, 0xa0},
};

static const divoom_light_color_info divoom_light_music_up_down_color_table[] =
{
    {245,70,66},
{255,130,17},
{250,170,27},
{255,198,2},
{255,228,24},
{255,255,33},
{232,255,8},
{175,255,18},
{147,255,51},
{114,254,61},
{68,252,64},
{72,250,100},
{71,255,180},
{63,254,236},
{66,255,250},
{60,255,251},
{86,251,253},
{63,250,253},
{71,251,255},
{81,246,255},
{70,238,255},
{84,209,255},
{77,192,249},
{131,179,255},
{169,176,254},
{188,155,251},
{241,148,255},
{242,135,255},
{255,67,248},
{182,35,161},
{112,1,38},
    {245,70,66},
{255,130,17},
{250,170,27},
{255,198,2},
{255,228,24},
{255,255,33},
{232,255,8},
{175,255,18},
{147,255,51},
{114,254,61},
{68,252,64},
{72,250,100},
{71,255,180},
{63,254,236},
{66,255,250},
{60,255,251},
{86,251,253},
{63,250,253},
{71,251,255},
{81,246,255},
{70,238,255},
{84,209,255},
{77,192,249},
{131,179,255},
{169,176,254},
{188,155,251},
{241,148,255},
{242,135,255},
{255,67,248},
{182,35,161},
{112,1,38}
};


static const divoom_light_color_info divoom_light_music_random_color_table[9] =
{
    {0,255,255}, {0X00,98, 255}, {0X2B,0X00,0XFF}, {142, 0, 255},{255, 0, 157},
    {255,49, 0}, {255,190,0}, {193,255,0},  {95,255,0},
};


static const divoom_light_color_info divoom_light_music_sea_top_rgb = {0x0f,0xff,0xff};
static const divoom_light_color_info divoom_light_music_sea_down_rgb = {0,0,0xff};

/* 16 向量以 64 为单位，避免在高刷新 EQ 中引入浮点和三角函数。 */
static const int8 divoom_light_music_forge_direction[16][2] =
{
    {64, 0}, {59, 24}, {45, 45}, {24, 59},
    {0, 64}, {-24, 59}, {-45, 45}, {-59, 24},
    {-64, 0}, {-59, -24}, {-45, -45}, {-24, -59},
    {0, -64}, {24, -59}, {45, -45}, {59, -24}
};

/* 熔炉使用固定高饱和调色盘，避免旧渐变表中的暗色段削弱 480 点显示。 */
static const divoom_light_color_info divoom_light_music_forge_color[8] =
{
    {0U, 244U, 255U}, {22U, 112U, 255U},
    {112U, 42U, 255U}, {255U, 26U, 206U},
    {255U, 58U, 42U}, {255U, 174U, 20U},
    {210U, 255U, 22U}, {34U, 255U, 122U}
};

static void divoom_light_music_scale_color(const divoom_light_color_info *source,
    uint8 gain, divoom_light_color_info *target)
{
    target->r_c = (uint8)(((uint16)source->r_c * gain) / 255U);
    target->g_c = (uint8)(((uint16)source->g_c * gain) / 255U);
    target->b_c = (uint8)(((uint16)source->b_c * gain) / 255U);
}

static void divoom_light_music_set_dot_if_black(uint16 x, uint16 y,
    const divoom_light_color_info *color, uint8 *disp_buf)
{
    uint32 pixel_off;
    uint32 byte_off;

    if(x >= DIVOOM_LIGHT_MUSIC_EQ_CANVAS_WIDTH ||
        y >= DIVOOM_LIGHT_MUSIC_EQ_CANVAS_WIDTH)
    {
        return;
    }
    pixel_off = (uint32)y * DIVOOM_LIGHT_MUSIC_EQ_CANVAS_WIDTH + x;
    byte_off = pixel_off * DIVOOM_APP_RGB565_BYTES_PER_PX;
    if(disp_buf[byte_off] != 0 || disp_buf[byte_off + 1U] != 0)
    {
        return;
    }
    divoom_light_music_set_data_565((uint16)pixel_off, color, disp_buf);
}

static void divoom_light_music_set_forge_dot(int16 x, int16 y,
    const divoom_light_color_info *color, uint8 *disp_buf, uint8 overwrite)
{
    uint32 pixel_off;
    uint32 byte_off;

    if(x < 0 || y < 0 || x >= DIVOOM_LIGHT_MUSIC_EQ_CANVAS_WIDTH ||
        y >= DIVOOM_LIGHT_MUSIC_EQ_CANVAS_WIDTH)
    {
        return;
    }
    pixel_off = (uint32)y * DIVOOM_LIGHT_MUSIC_EQ_CANVAS_WIDTH + (uint32)x;
    byte_off = pixel_off * DIVOOM_APP_RGB565_BYTES_PER_PX;
    if(!overwrite &&
        (disp_buf[byte_off] != 0 || disp_buf[byte_off + 1U] != 0))
    {
        return;
    }
    divoom_light_music_set_data_565((uint16)pixel_off, color, disp_buf);
}

static void divoom_light_music_draw_forge_line(int16 x0, int16 y0,
    int16 x1, int16 y1, const divoom_light_color_info *color,
    uint8 *disp_buf, uint8 overwrite)
{
    int16 dx;
    int16 dy;
    int16 sx;
    int16 sy;
    int16 err;
    int16 err2;

    dx = x1 >= x0 ? (int16)(x1 - x0) : (int16)(x0 - x1);
    dy = y1 >= y0 ? (int16)(y0 - y1) : (int16)(y1 - y0);
    sx = x0 < x1 ? 1 : -1;
    sy = y0 < y1 ? 1 : -1;
    err = (int16)(dx + dy);
    while(1)
    {
        divoom_light_music_set_forge_dot(x0, y0, color, disp_buf, overwrite);
        if(x0 == x1 && y0 == y1)
        {
            break;
        }
        err2 = (int16)(err * 2);
        if(err2 >= dy)
        {
            err = (int16)(err + dy);
            x0 = (int16)(x0 + sx);
        }
        if(err2 <= dx)
        {
            err = (int16)(err + dx);
            y0 = (int16)(y0 + sy);
        }
    }
}

static void divoom_light_music_draw_forge_block(int16 center_x, int16 center_y,
    uint8 half_width, const divoom_light_color_info *color, uint8 *disp_buf)
{
    int16 x;
    int16 y;

    for(y = (int16)(center_y - half_width);
        y <= (int16)(center_y + half_width); y++)
    {
        for(x = (int16)(center_x - half_width);
            x <= (int16)(center_x + half_width); x++)
        {
            divoom_light_music_set_forge_dot(x, y, color, disp_buf, 1U);
        }
    }
}

static void divoom_light_music_draw_forge_diamond(int16 center_x, int16 center_y,
    uint8 radius, const divoom_light_color_info *color, uint8 *disp_buf)
{
    int16 y;
    int16 half_width;

    for(y = (int16)-radius; y <= radius; y++)
    {
        half_width = y < 0 ? (int16)(radius + y) : (int16)(radius - y);
        divoom_light_music_draw_forge_line((int16)(center_x - half_width),
            (int16)(center_y + y), (int16)(center_x + half_width),
            (int16)(center_y + y), color, disp_buf, 1U);
    }
}

static void divoom_light_music_draw_forge_ring(Pdivoom_light_music_ctrl handle,
    int16 center_x, int16 center_y, uint8 radius, uint8 gain,
    uint8 segment_phase)
{
    uint16 segment;
    uint16 next_segment;
    uint8 color_index;
    int16 x0;
    int16 y0;
    int16 x1;
    int16 y1;
    divoom_light_color_info color;

    for(segment = 0; segment < 16U; segment++)
    {
        if((segment + segment_phase + handle->row_tick / 3U) % 5U == 4U)
        {
            continue;
        }
        next_segment = (segment + 1U) % 16U;
        x0 = (int16)(center_x +
            divoom_light_music_forge_direction[segment][0] * radius / 64);
        y0 = (int16)(center_y +
            divoom_light_music_forge_direction[segment][1] * radius / 64);
        x1 = (int16)(center_x +
            divoom_light_music_forge_direction[next_segment][0] * radius / 64);
        y1 = (int16)(center_y +
            divoom_light_music_forge_direction[next_segment][1] * radius / 64);
        color_index = (uint8)((segment / 2U + segment_phase +
            handle->disp_type + handle->row_tick / 5U) %
            (sizeof(divoom_light_music_forge_color) /
            sizeof(divoom_light_music_forge_color[0])));
        divoom_light_music_scale_color(
            &divoom_light_music_forge_color[color_index], gain, &color);
        divoom_light_music_draw_forge_line(x0, y0, x1, y1, &color,
            handle->disp_buf, 1U);
    }
}

/* 外圈低频冲击弧跨出画布边缘，让 480×480 显示空间有向外延伸的层次。 */
static void divoom_light_music_draw_forge_bass_arcs(Pdivoom_light_music_ctrl handle)
{
    uint16 ring;
    uint8 radius;
    uint8 pulse;

    pulse = handle->music_power > 32U ? 4U : (uint8)(handle->music_power / 8U);
    for(ring = 0; ring < 3U; ring++)
    {
        radius = (uint8)(86U + ring * 25U + pulse);
        divoom_light_music_draw_forge_ring(handle, 128, 116, radius,
            (uint8)(94U + ring * 26U), (uint8)(ring * 2U));
    }
}

/* 两条互相穿越的频率带分别承载高低频，打破原先单一竖线的阅读方向。 */
static void divoom_light_music_draw_forge_ribbons(Pdivoom_light_music_ctrl handle)
{
    uint16 x;
    uint16 band;
    int16 cyan_y;
    int16 magenta_y;
    uint8 level;
    uint8 color_index;
    uint8 block_size;
    divoom_light_color_info color;

    for(x = 0; x < DIVOOM_LIGHT_MUSIC_EQ_CANVAS_WIDTH; x += 3U)
    {
        band = x / 4U;
        level = handle->disp_info[band];
        if(level >= SCREEN_HEIGHT_64)
        {
            level = SCREEN_HEIGHT_64 - 1U;
        }
        if((x / 3U + handle->row_tick / 2U) % 7U != 5U)
        {
            cyan_y = (int16)(220 - (int16)(x * 3U / 4U) + level / 5U);
            color_index = (uint8)((band / 12U + handle->row_tick / 8U) % 2U);
            divoom_light_music_scale_color(
                &divoom_light_music_forge_color[color_index],
                (uint8)(158U + level), &color);
            block_size = level > 38U ? 2U : 1U;
            divoom_light_music_draw_forge_block((int16)x, cyan_y, block_size,
                &color, handle->disp_buf);
        }
        if((x / 3U + handle->row_tick / 3U) % 8U != 2U)
        {
            magenta_y = (int16)(34 + (int16)(x / 2U) - level / 6U);
            color_index = (uint8)(3U +
                (band / 12U + handle->row_tick / 9U) % 2U);
            divoom_light_music_scale_color(
                &divoom_light_music_forge_color[color_index],
                (uint8)(148U + level), &color);
            divoom_light_music_draw_forge_block((int16)x, magenta_y, 1U,
                &color, handle->disp_buf);
        }
    }
}

/* 64 个真实频段汇成八片不等长波形花瓣，核心周围每个方向都有独立响应。 */
static void divoom_light_music_draw_forge_petals(Pdivoom_light_music_ctrl handle)
{
    uint16 petal;
    uint16 band;
    uint16 sample;
    uint16 level_sum;
    uint16 step;
    uint8 direction;
    uint8 normal_direction;
    uint8 level;
    uint8 length;
    uint8 half_width;
    uint8 color_index;
    int16 center_x;
    int16 center_y;
    int16 x0;
    int16 y0;
    int16 x1;
    int16 y1;
    divoom_light_color_info color;

    center_x = DIVOOM_LIGHT_MUSIC_EQ_RADIAL_CENTER_X;
    center_y = 116;
    for(petal = 0; petal < 8U; petal++)
    {
        level_sum = 0U;
        for(sample = 0; sample < 8U; sample++)
        {
            band = petal * 8U + sample;
            level_sum += handle->disp_info[band];
        }
        level = (uint8)(level_sum / 8U);
        if(level >= SCREEN_HEIGHT_64)
        {
            level = SCREEN_HEIGHT_64 - 1U;
        }
        direction = (uint8)(petal * 2U);
        normal_direction = (uint8)((direction + 4U) % 16U);
        length = (uint8)(82U + level / 2U);
        for(step = 43U; step < length; step += 5U)
        {
            if((step / 5U + petal + handle->row_tick / 2U) % 7U == 5U)
            {
                continue;
            }
            half_width = (uint8)(4U +
                (level + step + petal * 3U + handle->row_tick / 3U) % 7U);
            x0 = (int16)(center_x +
                divoom_light_music_forge_direction[direction][0] * step / 64);
            y0 = (int16)(center_y +
                divoom_light_music_forge_direction[direction][1] * step / 64);
            x1 = (int16)(x0 -
                divoom_light_music_forge_direction[normal_direction][0] *
                half_width / 64);
            y1 = (int16)(y0 -
                divoom_light_music_forge_direction[normal_direction][1] *
                half_width / 64);
            color_index = (uint8)((petal + step / 15U +
                handle->disp_type + handle->row_tick / 8U) %
                (sizeof(divoom_light_music_forge_color) /
                sizeof(divoom_light_music_forge_color[0])));
            divoom_light_music_scale_color(
                &divoom_light_music_forge_color[color_index],
                (uint8)(202U + level / 2U), &color);
            divoom_light_music_draw_forge_line(x1, y1,
                (int16)(x0 + (x0 - x1)), (int16)(y0 + (y0 - y1)),
                &color, handle->disp_buf, 1U);
        }
    }
}

static void divoom_light_music_draw_forge_reactor(Pdivoom_light_music_ctrl handle)
{
    static const divoom_light_color_info core_orange = {255U, 76U, 16U};
    static const divoom_light_color_info core_yellow = {255U, 224U, 38U};
    static const divoom_light_color_info core_white = {255U, 255U, 240U};
    uint8 pulse;

    pulse = handle->music_power > 24U ? 3U : (uint8)(handle->music_power / 8U);
    divoom_light_music_draw_forge_ring(handle, 128, 116,
        (uint8)(22U + pulse), 226U, 0U);
    divoom_light_music_draw_forge_ring(handle, 128, 116,
        (uint8)(34U + pulse), 178U, 4U);
    divoom_light_music_draw_forge_ring(handle, 128, 116,
        (uint8)(45U + pulse), 118U, 7U);
    divoom_light_music_draw_forge_diamond(128, 116, 11U,
        &core_orange, handle->disp_buf);
    divoom_light_music_draw_forge_diamond(128, 116, 7U,
        &core_yellow, handle->disp_buf);
    divoom_light_music_draw_forge_diamond(128, 116, 3U,
        &core_white, handle->disp_buf);
}

/* 下方倒影只保留低亮度回声和断续能量脊，形成完整上下构图。 */
static void divoom_light_music_draw_forge_echo(Pdivoom_light_music_ctrl handle)
{
    uint16 echo;
    uint8 gain;
    uint8 color_index;
    int16 half_width;
    int16 y;
    divoom_light_color_info color;

    for(echo = 0; echo < 5U; echo++)
    {
        y = (int16)(181 + echo * 12U +
            (handle->row_tick / 3U + echo) % 4U);
        half_width = (int16)(42 - echo * 6U);
        color_index = (uint8)((echo + handle->row_tick / 7U) %
            (sizeof(divoom_light_music_forge_color) /
            sizeof(divoom_light_music_forge_color[0])));
        gain = (uint8)(126U - echo * 12U);
        divoom_light_music_scale_color(
            &divoom_light_music_forge_color[color_index], gain, &color);
        divoom_light_music_draw_forge_line((int16)(128 - half_width), y,
            (int16)(128 - 8), y, &color, handle->disp_buf, 1U);
        divoom_light_music_draw_forge_line((int16)(128 + 8), y,
            (int16)(128 + half_width), y, &color, handle->disp_buf, 1U);
    }
}

/* 12 种概念共享整数绘图原语，但保持完全不同的构图和运动语言。 */
static void divoom_light_music_draw_beat_particles(Pdivoom_light_music_ctrl handle);

static void divoom_light_music_clear_concept(Pdivoom_light_music_ctrl handle)
{
    memset(handle->disp_buf, 0,
        SCREEN_DOT_NUM_256 * DIVOOM_APP_RGB565_BYTES_PER_PX);
}

static uint8 divoom_light_music_get_concept_level(Pdivoom_light_music_ctrl handle,
    uint16 band)
{
    uint8 level;

    level = handle->disp_info[band % DIVOOM_LIGHT_MUSIC_EQ_BAND_COUNT];
    if(level >= SCREEN_HEIGHT_64)
    {
        level = SCREEN_HEIGHT_64 - 1U;
    }
    return level;
}

/* 返回 -amplitude～+amplitude 的三角波，供曲线类效果替代浮点正弦。 */
static int16 divoom_light_music_get_concept_wave(uint16 phase, uint8 amplitude)
{
    uint16 local_phase;
    int16 double_amplitude;

    local_phase = phase % 64U;
    double_amplitude = (int16)(amplitude * 2U);
    if(local_phase < 16U)
    {
        return (int16)(-(int16)amplitude +
            (int16)local_phase * double_amplitude / 16);
    }
    if(local_phase < 48U)
    {
        return (int16)((int16)amplitude -
            (int16)(local_phase - 16U) * double_amplitude / 32);
    }
    return (int16)(-(int16)amplitude +
        (int16)(local_phase - 48U) * double_amplitude / 16);
}

static void divoom_light_music_get_concept_color(uint8 index, uint8 gain,
    divoom_light_color_info *color)
{
    divoom_light_music_scale_color(&divoom_light_music_forge_color[index % 8U],
        gain, color);
}

static void divoom_light_music_draw_concept_ring_color(int16 center_x,
    int16 center_y, uint8 radius, const divoom_light_color_info *color,
    uint8 *disp_buf, uint8 dash_phase)
{
    uint16 segment;
    uint16 next_segment;
    int16 x0;
    int16 y0;
    int16 x1;
    int16 y1;

    for(segment = 0; segment < 16U; segment++)
    {
        if((segment + dash_phase) % 6U == 5U)
        {
            continue;
        }
        next_segment = (segment + 1U) % 16U;
        x0 = (int16)(center_x +
            divoom_light_music_forge_direction[segment][0] * radius / 64);
        y0 = (int16)(center_y +
            divoom_light_music_forge_direction[segment][1] * radius / 64);
        x1 = (int16)(center_x +
            divoom_light_music_forge_direction[next_segment][0] * radius / 64);
        y1 = (int16)(center_y +
            divoom_light_music_forge_direction[next_segment][1] * radius / 64);
        divoom_light_music_draw_forge_line(x0, y0, x1, y1, color,
            disp_buf, 1U);
    }
}

/* 0：轨道反应堆——多层卫星轨道和高亮核心。 */
static void divoom_light_music_draw_concept_orbit(Pdivoom_light_music_ctrl handle)
{
    static const divoom_light_color_info core_orange = {255U, 78U, 10U};
    static const divoom_light_color_info core_white = {255U, 255U, 220U};
    uint16 satellite;
    uint8 direction;
    uint8 radius;
    uint8 level;
    int16 x;
    int16 y;
    divoom_light_color_info color;

    divoom_light_music_clear_concept(handle);
    divoom_light_music_draw_beat_particles(handle);
    divoom_light_music_draw_forge_ring(handle, 128, 128, 34U, 220U, 0U);
    divoom_light_music_draw_forge_ring(handle, 128, 128, 66U, 176U, 2U);
    divoom_light_music_draw_forge_ring(handle, 128, 128, 99U, 138U, 5U);
    for(satellite = 0; satellite < 7U; satellite++)
    {
        level = divoom_light_music_get_concept_level(handle, satellite * 9U);
        direction = (uint8)((satellite * 5U + handle->row_tick / 3U) % 16U);
        radius = (uint8)(43U + satellite % 3U * 30U + level / 6U);
        x = (int16)(128 +
            divoom_light_music_forge_direction[direction][0] * radius / 64);
        y = (int16)(128 +
            divoom_light_music_forge_direction[direction][1] * radius / 64);
        divoom_light_music_get_concept_color((uint8)(satellite + 3U), 255U,
            &color);
        divoom_light_music_draw_forge_block(x, y,
            level > 24U ? 3U : 2U, &color, handle->disp_buf);
    }
    divoom_light_music_draw_forge_diamond(128, 128, 17U,
        &core_orange, handle->disp_buf);
    divoom_light_music_draw_forge_diamond(128, 128, 7U,
        &core_white, handle->disp_buf);
}

/* 1：镜像山脉——频段高度组成上下对称的霓虹地平线。 */
static void divoom_light_music_draw_concept_mountain(Pdivoom_light_music_ctrl handle)
{
    uint16 x;
    uint16 band;
    uint8 level;
    uint8 height;
    int16 wave;
    int16 top_y;
    int16 mirror_y;
    divoom_light_color_info color;
    divoom_light_color_info mirror_color;

    divoom_light_music_clear_concept(handle);
    for(x = 0; x < DIVOOM_LIGHT_MUSIC_EQ_CANVAS_WIDTH; x += 2U)
    {
        band = x / 4U;
        level = divoom_light_music_get_concept_level(handle, band);
        wave = divoom_light_music_get_concept_wave(
            (uint16)(x / 2U + handle->row_tick / 2U), 26U);
        if(wave < 0)
        {
            wave = (int16)-wave;
        }
        height = (uint8)(24U + (uint8)wave + level / 2U);
        top_y = (int16)(126 - height);
        mirror_y = (int16)(130 + height);
        divoom_light_music_get_concept_color((uint8)(band / 8U), 238U, &color);
        divoom_light_music_get_concept_color((uint8)(3U + band / 10U),
            112U, &mirror_color);
        divoom_light_music_draw_forge_line((int16)x, 126, (int16)x, top_y,
            &color, handle->disp_buf, 1U);
        divoom_light_music_draw_forge_line((int16)x, 130, (int16)x, mirror_y,
            &mirror_color, handle->disp_buf, 1U);
    }
    divoom_light_music_get_concept_color(3U, 255U, &color);
    divoom_light_music_draw_forge_line(0, 128, 255, 128, &color,
        handle->disp_buf, 1U);
}

/* 2：频谱雨——下落光滴由频段驱动，落点形成横向涟漪。 */
static void divoom_light_music_draw_concept_rain(Pdivoom_light_music_ctrl handle)
{
    uint16 column;
    uint16 drop;
    uint8 level;
    uint8 color_index;
    uint8 ripple;
    int16 x;
    int16 y;
    int16 width;
    divoom_light_color_info color;
    divoom_light_color_info dim_color;

    divoom_light_music_clear_concept(handle);
    for(column = 0; column < 12U; column++)
    {
        level = divoom_light_music_get_concept_level(handle, column * 5U);
        color_index = (uint8)(column % 8U);
        divoom_light_music_get_concept_color(color_index, 240U, &color);
        divoom_light_music_get_concept_color(color_index, 88U, &dim_color);
        x = (int16)(10 + column * 21U);
        for(drop = 0; drop < 9U + level / 10U; drop++)
        {
            y = (int16)((handle->row_tick * 3U + column * 19U + drop * 17U) %
                188U);
            divoom_light_music_draw_forge_block(x, y,
                drop % 3U == 0U ? 2U : 1U, &color, handle->disp_buf);
        }
        for(ripple = 0; ripple < 3U; ripple++)
        {
            y = (int16)(206 + ripple * 13U);
            width = (int16)(5 + ripple * 7U + level / 8U);
            divoom_light_music_draw_forge_line((int16)(x - width), y,
                (int16)(x + width), y, &dim_color, handle->disp_buf, 1U);
        }
    }
}

/* 3：螺旋星系——四条旋臂沿离散方向旋转，频段决定星块尺寸。 */
static void divoom_light_music_draw_concept_galaxy(Pdivoom_light_music_ctrl handle)
{
    static const divoom_light_color_info core_white = {224U, 255U, 255U};
    uint16 arm;
    uint16 radius;
    uint8 direction;
    uint8 level;
    int16 x;
    int16 y;
    divoom_light_color_info color;

    divoom_light_music_clear_concept(handle);
    divoom_light_music_draw_beat_particles(handle);
    for(arm = 0; arm < 4U; arm++)
    {
        for(radius = 12U; radius < 120U; radius += 4U)
        {
            direction = (uint8)((radius / 7U + arm * 4U +
                handle->row_tick / 3U) % 16U);
            level = divoom_light_music_get_concept_level(handle,
                arm * 16U + radius / 8U);
            x = (int16)(128 +
                divoom_light_music_forge_direction[direction][0] * radius / 64);
            y = (int16)(128 +
                divoom_light_music_forge_direction[direction][1] * radius / 64);
            divoom_light_music_get_concept_color((uint8)(arm + radius / 24U),
                (uint8)(154U + level), &color);
            divoom_light_music_draw_forge_block(x, y,
                level > 32U ? 2U : 1U, &color, handle->disp_buf);
        }
    }
    divoom_light_music_draw_forge_diamond(128, 128, 8U,
        &core_white, handle->disp_buf);
}

/* 4：万花筒花瓣——八组频段以花瓣宽度向中心同步开合。 */
static void divoom_light_music_draw_concept_flower(Pdivoom_light_music_ctrl handle)
{
    divoom_light_music_clear_concept(handle);
    divoom_light_music_draw_beat_particles(handle);
    divoom_light_music_draw_forge_petals(handle);
    divoom_light_music_draw_forge_reactor(handle);
}

/* 5：彩色河流——七条独立相位的横向波带形成纵深流动。 */
static void divoom_light_music_draw_concept_river(Pdivoom_light_music_ctrl handle)
{
    uint16 layer;
    uint16 x;
    uint8 level;
    int16 y;
    int16 previous_x;
    int16 previous_y;
    divoom_light_color_info color;

    divoom_light_music_clear_concept(handle);
    for(layer = 0; layer < 7U; layer++)
    {
        previous_x = 0;
        previous_y = (int16)(26 + layer * 34U);
        divoom_light_music_get_concept_color((uint8)(layer +
            handle->row_tick / 12U), 220U, &color);
        for(x = 0; x < DIVOOM_LIGHT_MUSIC_EQ_CANVAS_WIDTH; x += 3U)
        {
            level = divoom_light_music_get_concept_level(handle,
                x / 4U + layer * 5U);
            y = (int16)(26 + layer * 34U +
                divoom_light_music_get_concept_wave((uint16)(x / 2U +
                layer * 9U + handle->row_tick / 2U),
                (uint8)(9U + level / 6U)));
            divoom_light_music_draw_forge_line(previous_x, previous_y,
                (int16)x, y, &color, handle->disp_buf, 1U);
            if(layer % 2U == 0U)
            {
                divoom_light_music_draw_forge_line(previous_x,
                    (int16)(previous_y + 2), (int16)x, (int16)(y + 2),
                    &color, handle->disp_buf, 1U);
            }
            previous_x = (int16)x;
            previous_y = y;
        }
    }
}

/* 6：城市天际线——每栋建筑对应一组频段，并在地平线下产生倒影。 */
static void divoom_light_music_draw_concept_city(Pdivoom_light_music_ctrl handle)
{
    uint16 building;
    uint16 window_y;
    uint8 level;
    uint8 height;
    int16 x0;
    int16 x1;
    int16 top_y;
    int16 reflection_y;
    divoom_light_color_info color;
    divoom_light_color_info dim_color;

    divoom_light_music_clear_concept(handle);
    for(building = 0; building < 20U; building++)
    {
        level = divoom_light_music_get_concept_level(handle, building * 3U);
        height = (uint8)(32U + (building * 17U + handle->row_tick / 3U) % 48U +
            level / 2U);
        x0 = (int16)(3 + building * 13U);
        x1 = (int16)(x0 + 9);
        top_y = (int16)(151 - height);
        divoom_light_music_get_concept_color((uint8)(building / 3U), 238U,
            &color);
        divoom_light_music_get_concept_color((uint8)(building / 3U), 78U,
            &dim_color);
        divoom_light_music_draw_forge_line(x0, 151, x0, top_y, &color,
            handle->disp_buf, 1U);
        divoom_light_music_draw_forge_line(x1, 151, x1, top_y, &color,
            handle->disp_buf, 1U);
        divoom_light_music_draw_forge_line(x0, top_y, x1, top_y, &color,
            handle->disp_buf, 1U);
        for(window_y = (uint16)(top_y + 7); window_y < 148U; window_y += 9U)
        {
            divoom_light_music_draw_forge_block((int16)(x0 + 3),
                (int16)window_y, 1U, &color, handle->disp_buf);
        }
        reflection_y = (int16)(157 + height / 2U);
        divoom_light_music_draw_forge_line(x0, 157, x0, reflection_y,
            &dim_color, handle->disp_buf, 1U);
        divoom_light_music_draw_forge_line(x1, 157, x1, reflection_y,
            &dim_color, handle->disp_buf, 1U);
    }
    divoom_light_music_get_concept_color(4U, 255U, &color);
    divoom_light_music_draw_forge_line(0, 154, 255, 154, &color,
        handle->disp_buf, 1U);
}

/* 7：声纳雷达——同心扫描圈、旋转扫描束和频段目标点。 */
static void divoom_light_music_draw_concept_radar(Pdivoom_light_music_ctrl handle)
{
    static const divoom_light_color_info radar_green = {38U, 255U, 82U};
    static const divoom_light_color_info radar_dim = {8U, 76U, 28U};
    uint16 target;
    uint8 direction;
    uint8 radius;
    uint8 level;
    int16 x;
    int16 y;

    divoom_light_music_clear_concept(handle);
    divoom_light_music_draw_concept_ring_color(128, 128, 28U,
        &radar_dim, handle->disp_buf, 0U);
    divoom_light_music_draw_concept_ring_color(128, 128, 56U,
        &radar_dim, handle->disp_buf, 1U);
    divoom_light_music_draw_concept_ring_color(128, 128, 86U,
        &radar_dim, handle->disp_buf, 2U);
    divoom_light_music_draw_concept_ring_color(128, 128, 116U,
        &radar_dim, handle->disp_buf, 3U);
    divoom_light_music_draw_forge_line(8, 128, 248, 128, &radar_dim,
        handle->disp_buf, 1U);
    divoom_light_music_draw_forge_line(128, 8, 128, 248, &radar_dim,
        handle->disp_buf, 1U);
    direction = (uint8)((handle->row_tick / 2U) % 16U);
    x = (int16)(128 + divoom_light_music_forge_direction[direction][0] * 116 / 64);
    y = (int16)(128 + divoom_light_music_forge_direction[direction][1] * 116 / 64);
    divoom_light_music_draw_forge_line(128, 128, x, y, &radar_green,
        handle->disp_buf, 1U);
    for(target = 0; target < 6U; target++)
    {
        level = divoom_light_music_get_concept_level(handle, target * 11U);
        direction = (uint8)((target * 5U + 1U) % 16U);
        radius = (uint8)(28U + target * 14U);
        x = (int16)(128 +
            divoom_light_music_forge_direction[direction][0] * radius / 64);
        y = (int16)(128 +
            divoom_light_music_forge_direction[direction][1] * radius / 64);
        divoom_light_music_draw_forge_block(x, y,
            level > 20U ? 3U : 2U, &radar_green, handle->disp_buf);
    }
}

/* 8：流星爆发——低频从中心炸开成不等长光束。 */
static void divoom_light_music_draw_concept_starburst(Pdivoom_light_music_ctrl handle)
{
    static const divoom_light_color_info core_white = {255U, 255U, 228U};
    uint16 ray;
    uint16 step;
    uint8 level;
    uint8 length;
    int16 x0;
    int16 y0;
    int16 x1;
    int16 y1;
    divoom_light_color_info color;

    divoom_light_music_clear_concept(handle);
    divoom_light_music_draw_beat_particles(handle);
    for(ray = 0; ray < 16U; ray++)
    {
        level = divoom_light_music_get_concept_level(handle, ray * 4U);
        length = (uint8)(58U + (ray * 13U + handle->row_tick) % 30U +
            level / 2U);
        x0 = (int16)(128 +
            divoom_light_music_forge_direction[ray][0] * 14 / 64);
        y0 = (int16)(128 +
            divoom_light_music_forge_direction[ray][1] * 14 / 64);
        x1 = (int16)(128 +
            divoom_light_music_forge_direction[ray][0] * length / 64);
        y1 = (int16)(128 +
            divoom_light_music_forge_direction[ray][1] * length / 64);
        divoom_light_music_get_concept_color((uint8)(4U + ray / 5U),
            246U, &color);
        divoom_light_music_draw_forge_line(x0, y0, x1, y1, &color,
            handle->disp_buf, 1U);
        for(step = 28U; step < length; step += 22U)
        {
            x1 = (int16)(128 +
                divoom_light_music_forge_direction[ray][0] * step / 64);
            y1 = (int16)(128 +
                divoom_light_music_forge_direction[ray][1] * step / 64);
            divoom_light_music_draw_forge_block(x1, y1, 2U, &color,
                handle->disp_buf);
        }
    }
    divoom_light_music_draw_forge_diamond(128, 128, 11U,
        &core_white, handle->disp_buf);
}

/* 9：双螺旋——两条反相频谱链沿纵轴扭转并由能量横档连接。 */
static void divoom_light_music_draw_concept_helix(Pdivoom_light_music_ctrl handle)
{
    uint16 y;
    uint8 level;
    int16 offset;
    int16 x_left;
    int16 x_right;
    divoom_light_color_info cyan;
    divoom_light_color_info magenta;
    divoom_light_color_info rung;

    divoom_light_music_clear_concept(handle);
    divoom_light_music_get_concept_color(0U, 255U, &cyan);
    divoom_light_music_get_concept_color(3U, 255U, &magenta);
    divoom_light_music_get_concept_color(2U, 104U, &rung);
    for(y = 8U; y < 250U; y += 4U)
    {
        level = divoom_light_music_get_concept_level(handle, y / 4U);
        offset = divoom_light_music_get_concept_wave(
            (uint16)(y / 3U + handle->row_tick / 2U),
            (uint8)(38U + level / 5U));
        x_left = (int16)(128 + offset);
        x_right = (int16)(128 - offset);
        divoom_light_music_draw_forge_block(x_left, (int16)y, 2U,
            &cyan, handle->disp_buf);
        divoom_light_music_draw_forge_block(x_right, (int16)y, 2U,
            &magenta, handle->disp_buf);
        if(y % 16U == 8U)
        {
            divoom_light_music_draw_forge_line(x_left, (int16)y, x_right,
                (int16)y, &rung, handle->disp_buf, 1U);
        }
    }
}

/* 10：熔岩池——四个能量池通过亮桥相连，频段改变池体大小。 */
static void divoom_light_music_draw_concept_lava(Pdivoom_light_music_ctrl handle)
{
    static const int16 pool_pos[4][2] =
    {
        {72, 70}, {181, 58}, {70, 183}, {184, 180}
    };
    static const divoom_light_color_info lava_red = {255U, 38U, 12U};
    static const divoom_light_color_info lava_orange = {255U, 126U, 8U};
    static const divoom_light_color_info lava_yellow = {255U, 236U, 42U};
    uint16 pool;
    uint8 level;
    uint8 radius;

    divoom_light_music_clear_concept(handle);
    divoom_light_music_draw_beat_particles(handle);
    divoom_light_music_draw_forge_line(pool_pos[0][0], pool_pos[0][1],
        pool_pos[1][0], pool_pos[1][1], &lava_red, handle->disp_buf, 1U);
    divoom_light_music_draw_forge_line(pool_pos[0][0], pool_pos[0][1],
        pool_pos[2][0], pool_pos[2][1], &lava_orange, handle->disp_buf, 1U);
    divoom_light_music_draw_forge_line(pool_pos[1][0], pool_pos[1][1],
        pool_pos[3][0], pool_pos[3][1], &lava_orange, handle->disp_buf, 1U);
    divoom_light_music_draw_forge_line(pool_pos[2][0], pool_pos[2][1],
        pool_pos[3][0], pool_pos[3][1], &lava_red, handle->disp_buf, 1U);
    for(pool = 0; pool < 4U; pool++)
    {
        level = divoom_light_music_get_concept_level(handle, pool * 15U);
        radius = (uint8)(19U + level / 5U +
            (handle->row_tick / 3U + pool * 2U) % 5U);
        divoom_light_music_draw_forge_diamond(pool_pos[pool][0],
            pool_pos[pool][1], radius, &lava_red, handle->disp_buf);
        divoom_light_music_draw_forge_diamond(pool_pos[pool][0],
            pool_pos[pool][1], (uint8)(radius * 2U / 3U), &lava_orange,
            handle->disp_buf);
        divoom_light_music_draw_forge_diamond(pool_pos[pool][0],
            pool_pos[pool][1], (uint8)(radius / 3U), &lava_yellow,
            handle->disp_buf);
    }
}

/* 11：示波器——四条不同频率和相位的轨迹在绿色测量网格上运行。 */
static void divoom_light_music_draw_concept_scope(Pdivoom_light_music_ctrl handle)
{
    static const divoom_light_color_info grid_color = {8U, 72U, 28U};
    static const uint8 base_y[4] = {38U, 94U, 152U, 212U};
    uint16 grid;
    uint16 trace;
    uint16 x;
    uint8 level;
    int16 y;
    int16 previous_y;
    divoom_light_color_info color;

    divoom_light_music_clear_concept(handle);
    for(grid = 16U; grid < 256U; grid += 32U)
    {
        divoom_light_music_draw_forge_line((int16)grid, 0, (int16)grid, 255,
            &grid_color, handle->disp_buf, 1U);
        divoom_light_music_draw_forge_line(0, (int16)grid, 255, (int16)grid,
            &grid_color, handle->disp_buf, 1U);
    }
    for(trace = 0; trace < 4U; trace++)
    {
        divoom_light_music_get_concept_color((uint8)(trace * 2U), 255U, &color);
        previous_y = base_y[trace];
        for(x = 0; x < DIVOOM_LIGHT_MUSIC_EQ_CANVAS_WIDTH; x += 2U)
        {
            level = divoom_light_music_get_concept_level(handle,
                x / 4U + trace * 11U);
            y = (int16)(base_y[trace] +
                divoom_light_music_get_concept_wave((uint16)(x *
                (trace + 1U) / 2U + handle->row_tick),
                (uint8)(10U + level / 6U)));
            divoom_light_music_draw_forge_line((int16)(x >= 2U ? x - 2U : 0U),
                previous_y, (int16)x, y, &color, handle->disp_buf, 1U);
            previous_y = y;
        }
    }
}

#include "divoom_light_music_12_dense.inc"
/*dense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-paddingdense-render-padding*/
/*
 * 旧资源位于缓冲区起始处。目标坐标按倒序写入可保证原始像素在被读取前不会被覆盖，
 * 因而无需再申请一块 256×256 临时缓冲。
 */
static void divoom_light_music_expand_legacy_frame(Pdivoom_light_music_ctrl handle,
    uint16 source_width)
{
    int32 dst_x;
    int32 dst_y;
    uint32 src_pixel;
    uint32 dst_pixel;
    uint32 src_byte;
    uint32 dst_byte;

    if(handle == NULL || source_width == 0 ||
        source_width > DIVOOM_LIGHT_MUSIC_EQ_CANVAS_WIDTH)
    {
        return;
    }
    if(source_width == DIVOOM_LIGHT_MUSIC_EQ_CANVAS_WIDTH)
    {
        return;
    }

    for(dst_y = DIVOOM_LIGHT_MUSIC_EQ_CANVAS_WIDTH - 1; dst_y >= 0; dst_y--)
    {
        for(dst_x = DIVOOM_LIGHT_MUSIC_EQ_CANVAS_WIDTH - 1; dst_x >= 0; dst_x--)
        {
            src_pixel = (uint32)(dst_y * source_width /
                DIVOOM_LIGHT_MUSIC_EQ_CANVAS_WIDTH) * source_width +
                (uint32)(dst_x * source_width / DIVOOM_LIGHT_MUSIC_EQ_CANVAS_WIDTH);
            dst_pixel = (uint32)dst_y * DIVOOM_LIGHT_MUSIC_EQ_CANVAS_WIDTH +
                (uint32)dst_x;
            src_byte = src_pixel * DIVOOM_APP_RGB565_BYTES_PER_PX;
            dst_byte = dst_pixel * DIVOOM_APP_RGB565_BYTES_PER_PX;
            handle->disp_buf[dst_byte] = handle->disp_buf[src_byte];
            handle->disp_buf[dst_byte + 1U] = handle->disp_buf[src_byte + 1U];
        }
    }
}

/* 三层透视菱形贯穿整屏，随能量轻微呼吸，形成前后景深而不是单一底图。 */
static void divoom_light_music_draw_energy_tunnel(Pdivoom_light_music_ctrl handle)
{
    static const uint8 radius_table[3] = {60U, 86U, 112U};
    uint16 ring;
    int16 dx;
    int16 dy;
    int16 radius;
    int16 abs_dx;
    uint8 energy;
    uint8 gain;
    uint8 color_index;
    divoom_light_color_info color;

    energy = handle->music_power;
    if(energy > 32U)
    {
        energy = 32U;
    }
    for(ring = 0; ring < 3U; ring++)
    {
        radius = (int16)(radius_table[ring] + energy / 8U);
        color_index = (uint8)((ring * 5U + handle->row_tick / 4U) %
            (sizeof(divoom_light_music_left_right_color_table) /
            sizeof(divoom_light_music_left_right_color_table[0])));
        gain = (uint8)(48U + ring * 24U);
        divoom_light_music_scale_color(
            &divoom_light_music_left_right_color_table[color_index], gain, &color);
        for(dx = (int16)-radius; dx <= radius; dx++)
        {
            abs_dx = dx < 0 ? (int16)-dx : dx;
            if(((uint16)abs_dx + handle->row_tick / 2U + ring * 3U) % 9U >= 6U)
            {
                continue;
            }
            dy = (int16)(radius - abs_dx);
            divoom_light_music_set_dot_if_black(
                (uint16)(DIVOOM_LIGHT_MUSIC_EQ_RADIAL_CENTER_X + dx),
                (uint16)(DIVOOM_LIGHT_MUSIC_EQ_RADIAL_CENTER_Y - dy),
                &color, handle->disp_buf);
            divoom_light_music_set_dot_if_black(
                (uint16)(DIVOOM_LIGHT_MUSIC_EQ_RADIAL_CENTER_X + dx),
                (uint16)(DIVOOM_LIGHT_MUSIC_EQ_RADIAL_CENTER_Y + dy),
                &color, handle->disp_buf);
        }
    }
}

/* 四条镜像波轨覆盖上、中、下景深，真实频段决定轨迹偏移。 */
static void divoom_light_music_draw_wave_tracks(Pdivoom_light_music_ctrl handle)
{
    uint16 x;
    uint16 band;
    uint16 wave;
    uint16 upper_mid_y;
    uint16 lower_mid_y;
    uint16 top_y;
    uint16 bottom_y;
    uint8 level;
    uint8 gain;
    uint8 color_index;
    divoom_light_color_info color;

    for(x = 0; x < DIVOOM_LIGHT_MUSIC_EQ_CANVAS_WIDTH; x++)
    {
        if((x + handle->row_tick) % 6U >= 4U)
        {
            continue;
        }
        band = x / 4U;
        level = handle->disp_info[band];
        if(level >= SCREEN_HEIGHT_64)
        {
            level = SCREEN_HEIGHT_64 - 1;
        }
        wave = (uint16)(level / 5U +
            (x / 12U + handle->row_tick / 3U) % 5U);
        top_y = (uint16)(28U + wave);
        bottom_y = (uint16)(228U - wave);
        upper_mid_y = (uint16)(DIVOOM_LIGHT_MUSIC_EQ_RADIAL_CENTER_Y - 30U - level / 3U);
        lower_mid_y = (uint16)(DIVOOM_LIGHT_MUSIC_EQ_RADIAL_CENTER_Y + 30U + level / 3U);
        color_index = (uint8)((band / 8U + handle->row_tick / 4U) %
            (sizeof(divoom_light_music_left_right_color_table) /
            sizeof(divoom_light_music_left_right_color_table[0])));
        gain = (uint8)(62U + level * 2U);
        divoom_light_music_scale_color(
            &divoom_light_music_left_right_color_table[color_index], gain, &color);
        divoom_light_music_set_dot_if_black(x, top_y, &color, handle->disp_buf);
        divoom_light_music_set_dot_if_black(x, upper_mid_y, &color, handle->disp_buf);
        divoom_light_music_set_dot_if_black(x, lower_mid_y, &color, handle->disp_buf);
        divoom_light_music_set_dot_if_black(x, bottom_y, &color, handle->disp_buf);
    }
}

/* 粒子位置由帧号和频段确定，不依赖随机全局状态，静音时会自然减少。 */
static void divoom_light_music_draw_beat_particles(Pdivoom_light_music_ctrl handle)
{
    uint16 particle;
    uint16 band;
    uint16 x;
    uint16 y;
    uint16 size;
    uint16 px;
    uint16 py;
    uint32 phase;
    uint8 level;
    uint8 gain;
    uint8 color_index;
    divoom_light_color_info color;

    phase = handle->row_tick / 2U;
    for(particle = 0; particle < DIVOOM_LIGHT_MUSIC_EQ_PARTICLE_COUNT; particle++)
    {
        band = (uint16)((particle * 5U + phase) % SCREEN_WIDTH_64);
        level = handle->disp_info[band];
        if(level < 4U)
        {
            /* 静音仅保留八颗暗粒子作为远景层，不伪造强节拍。 */
            if(particle >= 8U)
            {
                continue;
            }
            size = 1U;
            gain = 52U;
        }
        else
        {
            size = level > 32U ? 2U : 1U;
            gain = level > 40U ? 220U : (uint8)(100U + level * 3U);
        }
        x = (uint16)((particle * 47U + phase * 5U + level * 3U) %
            DIVOOM_LIGHT_MUSIC_EQ_CANVAS_WIDTH);
        y = (uint16)(8U + (particle * 67U + phase * 11U) % 240U);
        color_index = (uint8)((particle * 3U + phase) %
            (sizeof(divoom_light_music_left_right_color_table) /
            sizeof(divoom_light_music_left_right_color_table[0])));
        divoom_light_music_scale_color(
            &divoom_light_music_left_right_color_table[color_index], gain, &color);
        for(py = 0; py < size; py++)
        {
            for(px = 0; px < size; px++)
            {
                divoom_light_music_set_dot_if_black((uint16)(x + px),
                    (uint16)(y + py), &color, handle->disp_buf);
            }
        }
    }
}

static void divoom_light_music_finish_legacy_frame(Pdivoom_light_music_ctrl handle,
    uint16 source_width, uint8 fine_spectrum, uint8 decorations)
{
    (void)fine_spectrum;
    (void)decorations;

    /*
     * 12 个内置模式均保留原始 EQ 绘制结果，仅做像素保持扩展。
     * 原始算法直接消费 64 路 FFT 数据，不能再用按 row_tick 自动运动的概念图覆盖。
     */
    divoom_light_music_expand_legacy_frame(handle, source_width);
    handle->disp_width = DIVOOM_LIGHT_MUSIC_EQ_CANVAS_WIDTH;
    handle->row_tick++;
}

                                                                                                                                                                                                                                                                                   int divoom_light_music_compare_color_value(divoom_light_color_info *color1, divoom_light_color_info *color2)
{
	if (color1->r_c > color2->r_c)
	{
		if (color1->r_c > color2->r_c + 5)
		{
			return 0;
		}
	}
	else
	{
		if (color2->r_c > color1->r_c + 5)
		{
			return 0;
		}
	}

	if (color1->g_c > color2->g_c)
	{
		if (color1->g_c > color2->g_c + 5)
		{
			return 0;
		}
	}
	else
	{
		if (color2->g_c > color1->g_c + 5)
		{
			return 0;
		}
	}


	if (color1->b_c > color2->b_c)
	{
		if (color1->b_c > color2->b_c + 5)
		{
			return 0;
		}
	}
	else
	{
		if (color2->b_c > color1->b_c + 5)
		{
			return 0;
		}
	}

	return 1;
}

void divoom_light_music_format_row_pic_info(Pdivoom_light_music_ctrl pdivoom_light_music_handle, int image_id, int image_ex_id,uint16 centor_pos)
{
	int i, offset, pos, diff_color_flag = 0, pic_num, copy_size;
	uint8 *buf = NULL;
	divoom_light_color_info first_color, cur_color, black_color = { 0, 0, 0 };
    Pdivoom_light_music_row_pic_info pprow_pic_info;
    Pdivoom_http_download_image_file_info pimage_handle;

    if(pdivoom_light_music_handle->prow_pic_ex_image_handle)
    {
        divoom_http_download_image_free_image_handle(pdivoom_light_music_handle->prow_pic_ex_image_handle);
        pdivoom_light_music_handle->prow_pic_ex_image_handle = NULL;
    }
    pprow_pic_info = &pdivoom_light_music_handle->prow_pic_info;
    
    pimage_handle = divoom_manage_image_read(DIVOOM_MANAGE_IMAGE_NOTICE_NET_GIF, image_id, NULL, NULL);
    if(pimage_handle == NULL)
    {
        return;
    }
    divoom_manage_image_format_gif_table(pimage_handle, pprow_pic_info->pic_buf[0]);
    pic_num = pimage_handle->num;
    if(pic_num > DIVOOM_LIGHT_MUSIC_GIF_POWER_LEVEL)
    {
        //pic_buf 只有 16 组，资源帧数异常时必须截断，否则会写穿整个句柄
        pic_num = DIVOOM_LIGHT_MUSIC_GIF_POWER_LEVEL;
    }
    for(i = 0; i < pic_num; i++)
    {
        if(pimage_handle->pimage_table[i] == NULL || pimage_handle->pimage_table[i]->image_data == NULL)
        {
            continue;
        }
        copy_size = DIVOOM_LIGHT_MUSIC_ROW_PIC_BYTES;
        if(pimage_handle->pimage_table[i]->image_size > 0 &&
            pimage_handle->pimage_table[i]->image_size < copy_size)
        {
            copy_size = pimage_handle->pimage_table[i]->image_size;
        }
        memcpy(pprow_pic_info->pic_buf[i], pimage_handle->pimage_table[i]->image_data, copy_size);
    }
    divoom_http_download_image_free_image_handle(pimage_handle);

    if(image_ex_id)
    {    
        pdivoom_light_music_handle->prow_pic_ex_image_handle = divoom_manage_image_read(DIVOOM_MANAGE_IMAGE_NOTICE_NET_GIF, image_ex_id, NULL, NULL);
        if(pdivoom_light_music_handle->prow_pic_ex_image_handle)
        {
            divoom_manage_image_format_gif_table(pdivoom_light_music_handle->prow_pic_ex_image_handle, NULL);
            divoom_print("speed: %d\n", pdivoom_light_music_handle->prow_pic_ex_image_handle->speed);
        }
    }
	pprow_pic_info->tol = 0;
	offset = centor_pos *32;
	buf = pprow_pic_info->pic_buf[0];
	divoom_disp_ctrl_get_data(offset, &first_color, buf);
	pprow_pic_info->item_list[pprow_pic_info->tol].start_x = 0;
	pprow_pic_info->item_list[pprow_pic_info->tol].width = 1;
	pos = 1;
	while (pos < DIVOOM_IMAGE_WIDTH)
	{
		diff_color_flag = 0;
		while (pos < DIVOOM_IMAGE_WIDTH)
		{
			if (pos == DIVOOM_IMAGE_WIDTH - 1)
			{
				pprow_pic_info->item_list[pprow_pic_info->tol].width++;
				pos++;
				break;
			}
			divoom_disp_ctrl_get_data(offset + pos, &cur_color, buf);
			pos++;
			if (divoom_light_music_compare_color_value(&cur_color, &black_color) == 1)
			{
				//下一个点是黑色的，将结束
				break;
			}
			if (divoom_light_music_compare_color_value(&cur_color, &first_color) == 0)
			{
				//下一个点是黑色的，将结束
				diff_color_flag = 1;
			}
			else if (diff_color_flag == 1)
			{
				//颜色相同，但是中间出现过不同的，也认为结束周期
				pos--;
				break;
			}
			pprow_pic_info->item_list[pprow_pic_info->tol].width++;
		}
		pprow_pic_info->tol++;
		//跳过黑色    
		while (pos < DIVOOM_IMAGE_WIDTH)
		{
			divoom_disp_ctrl_get_data(offset + pos, &cur_color, buf);
			if (divoom_light_music_compare_color_value(&cur_color, &black_color) == 0)
			{
				break;
			}
			if (divoom_light_music_compare_color_value(&cur_color, &first_color) == 1)
			{
				break;
			}
			pos++;
		}
		memcpy(&first_color, &cur_color, sizeof(divoom_light_color_info));
		pprow_pic_info->item_list[pprow_pic_info->tol].start_x = pos;
		pprow_pic_info->item_list[pprow_pic_info->tol].width = 0;
	}

    for(i = 0; i < pprow_pic_info->tol; i ++)
    {
        divoom_print("id: %d, start: %d, width: %d\n", i, pprow_pic_info->item_list[i].start_x, pprow_pic_info->item_list[i].width);
    }

}

void divoom_light_music_get_row_pic_disp_buf(Pdivoom_light_music_ctrl pdivoom_light_music_handle, uint8 *disp_buf, uint8 *level_info)
{
    uint8 *pic_buf;
    Pdivoom_light_music_row_pic_info pprow_pic_info = &pdivoom_light_music_handle->prow_pic_info;
    int i, j, k, cur_level, offset, end_pos;
    divoom_light_color_info color_info;

    for(i = 0; i < pprow_pic_info->tol; i ++)
    {
        cur_level = level_info[i * 2]/3; 
        if(cur_level >= 16)
        {
            cur_level = 15;
        }
        offset = 0;
        pic_buf = pprow_pic_info->pic_buf[cur_level];
        for(k = 0; k < SCREEN_WIDTH_32; k++)
        {
            for(j = 0; j < pprow_pic_info->item_list[i].width; j++)
            {
                divoom_disp_ctrl_get_data(offset + j + pprow_pic_info->item_list[i].start_x, &color_info, pic_buf);
                divoom_disp_ctrl_set_data(offset + j + pprow_pic_info->item_list[i].start_x, &color_info, disp_buf);
            }
            offset += SCREEN_WIDTH_32;
        }
        if(i < pprow_pic_info->tol - 1 )
        {
            end_pos = pprow_pic_info->item_list[i + 1].start_x;
        }
        else
        {
            end_pos = SCREEN_WIDTH_32;
        }
        //增加一些黑色
        offset = 0;
        memset(&color_info, 0, sizeof(color_info));
        for(k = 0; k < SCREEN_WIDTH_32; k++)
        {
            for(j = pprow_pic_info->item_list[i].width + pprow_pic_info->item_list[i].start_x; j < end_pos; j++)
            {
                divoom_disp_ctrl_set_data(offset + j , &color_info, disp_buf);
            }
            offset += SCREEN_WIDTH_32;
        }
    }
}
void divoom_light_music_add_row_extern_pic(Pdivoom_light_music_ctrl pdivoom_light_music_handle, uint8 *disp_buf)
{
    int i, disp_pic_id;
    divoom_light_color_info color_info;
    Pdivoom_http_download_image_file_info pex_image_handle;
    Pdivoom_http_download_image_file_item pex_pic_item;
    
    pex_image_handle = pdivoom_light_music_handle->prow_pic_ex_image_handle;
    if(pex_image_handle == NULL || pex_image_handle->num <= 0)
    {
        return;
    }
    disp_pic_id = (pdivoom_light_music_handle->row_tick * DIVOOM_LIGHT_MUSIC_REFRESH_TIME * 10 / pex_image_handle->speed) % pex_image_handle->num;
    pex_pic_item = pex_image_handle->pimage_table[disp_pic_id];
    if(pex_pic_item == NULL || pex_pic_item->image_data == NULL)
    {
        return;
    }

    for(i = 0; i < SCREEN_DOT_NUM_32; i++ )
    {
        divoom_disp_ctrl_get_data(i, &color_info, pex_pic_item->image_data);
        if(color_info.r_c || color_info.g_c || color_info.b_c)
        {
            divoom_disp_ctrl_set_data(i, &color_info, disp_buf);
        }
    }
}

void divoom_light_music_add_data(Pdivoom_light_music_ctrl pdivoom_light_music_handle, uint8 *pcm_data, uint16 len)
{
//    T_S16 *pdata = (T_S16 *)pcm_data;
    T_S16 *pdata = (T_S16 *)pcm_data;        
    uint32 count = len / 2;
    uint32 i;
//    T_S16 temp;
    T_S16 temp;    

//这里必须拆开，handle为空里pdivoom_light_music_handle->play_flag不可访问
//    if(NULL == pdivoom_light_music_handle || pdivoom_light_music_handle->play_flag == 0)
    if(NULL == pdivoom_light_music_handle)
    {
        return ;
    }

    if(pdivoom_light_music_handle->fft_flag != DIVOOM_DISP_MUSIC_RANDOM_MODE)
    {
        return;
    }
    
//    pdivoom_light_music_handle->pcm_value = (uint32)pdata[0] << 16;    
    for (i = 0; i < count; i++)
    {

        temp = pdata[i];
        if (temp < 0)
        {

            pdivoom_light_music_handle->pcm_value += (-temp)*8;
        }
        else
        {
            pdivoom_light_music_handle->pcm_value += temp*8;
        }
    }

//    pdivoom_light_music_handle->refresh_time = divoom_system_get_tickms() + DIVOOM_LIGHT_MUSIC_NO_MUSIC_TIME;
    pdivoom_light_music_handle->refresh_flag = 1;

//    divoom_print("pcm_value:%ld,  %d\n",pdivoom_light_music_handle->pcm_value,len);
}

void divoom_light_music_mic_data(Pdivoom_light_music_ctrl pdivoom_light_music_handle, uint32 value, uint32 max_value)
{

    if(pdivoom_light_music_handle->disp_type == DIVOOM_DISP_MUSIC_NORMAL_MODE || pdivoom_light_music_handle->disp_type == DIVOOM_DISP_MUSIC_CENTER_MODE)
    {
        return ;
    }
    if(value < 35)
    {
        pdivoom_light_music_handle->music_power = 1;
    }
    else
    {
        pdivoom_light_music_handle->music_power =  (95 - (value - 35)) * 16 / 60;
    }
    pdivoom_light_music_handle->refresh_flag = 1;
}

#if 1

const uint8 divoom_music_power_level_table[] = 
{
	14,16,18,20,25,30,36,42,49,50,64,72,81,90,100,110,121,132,144,155,169,186,190,196,210,218,225,230,242,248,250,255
};

const uint8 divoom_music_power_16level_table[] = 
{
//	14,16,25,36,49,64,81,100,121,144,169,180,196,225,230,250,255

//	14,16,18,25,30,40,50,60,70,80,90,100,121,144,190,220,250,255
//	14,16,18,20,22,24,26,28,30,40,80,100,121,144,190,220,250,255

	38,64,80,95,110,120,125,130,135,140,145,155,165,180,220,255

};

void divoom_light_music_update_freq(long int *music_freq)
{
    uint8 i;
	uint8 spectrum_peak = 0;
	uint32 music_power = 0;
    Pdivoom_light_music_ctrl pdivoom_light_music_handle;
#ifndef DIVOOM_TIMES_VIBE
    long int *freq_src = music_freq;
#endif
#ifdef DIVOOM_TIMES_VIBE
    int use_jl_eq = divoom_spectrum_is_active(DIVOOM_SPECTRUM_SILENCE_MS_DEFAULT);
#else
    int use_jl_eq = 0;
#endif

    if(pdivoom_light_music_gobal_handle == NULL || music_freq == NULL)
    {
        return;
    }
    divoom_system_get_lock(pdivoom_light_music_gobal_handle->lock_handle);
    for(int k = 0; k < divoom_queue_items(pdivoom_light_music_gobal_handle->eq_queue); k++)
    {
        pdivoom_light_music_handle = divoom_queue_get_index_item(pdivoom_light_music_gobal_handle->eq_queue, k);
        if(pdivoom_light_music_handle == NULL)
        {
            break;
        }
        pdivoom_light_music_handle->tick = 0;
#ifdef DIVOOM_TIMES_VIBE
        if (use_jl_eq)
        {
            divoom_spectrum_fill(pdivoom_light_music_handle->disp_info, SCREEN_WIDTH_64, 1, SCREEN_WIDTH_64 - 1);
        }
        else
        {
            memset(pdivoom_light_music_handle->disp_info, 0, SCREEN_WIDTH_64);
        }
#else
	    {
	        for(i = 0; i < SCREEN_WIDTH_64; i ++)
	        {
	            if(freq_src[i] < 64)
	            {
	    		    pdivoom_light_music_handle->disp_info[i] = 0;	
	            }
	            else if(freq_src[i] < 64 + 8 * 8)
	            {
	    		    pdivoom_light_music_handle->disp_info[i] = (freq_src[i] - 64)/8;	
	            }            
	            else if(freq_src[i] < 64 + 8 * 8 + 32 * 5/2)
	            {
	    		    pdivoom_light_music_handle->disp_info[i] = (freq_src[i] - 64 - 8 * 8)*2/5 + 8;	
	            }
	            else
	            {
	    		    pdivoom_light_music_handle->disp_info[i] = (freq_src[i] -64 - 8 * 8 - 32 * 5/2)/6 + 40;	
	            }
	            
	            if(pdivoom_light_music_handle->disp_info[i] >= SCREEN_WIDTH_64)
	            {
	                pdivoom_light_music_handle->disp_info[i] = SCREEN_WIDTH_64 - 1;
	            }
	        }
	    }
#endif

	    	pdivoom_light_music_handle->refresh_flag = 1;

        /*
         * 全频段均值与峰值共同驱动资源帧和随机模式。
         * 峰值项保证鼓点只集中在少数频带时仍有足够响应，均值项抑制单点噪声误触发。
         */
        spectrum_peak = 0;
        for(i = 0, music_power = 0; i < SCREEN_WIDTH_64; i++)
        {
            music_power += pdivoom_light_music_handle->disp_info[i];
            if(pdivoom_light_music_handle->disp_info[i] > spectrum_peak)
            {
                spectrum_peak = pdivoom_light_music_handle->disp_info[i];
            }
        }
		music_power = ((music_power / SCREEN_WIDTH_64) * 2U + spectrum_peak) / 9U;
	    	if(music_power >= DIVOOM_LIGHT_MUSIC_GIF_POWER_LEVEL)
        {
	    		music_power = DIVOOM_LIGHT_MUSIC_GIF_POWER_LEVEL;
        }
	    	pdivoom_light_music_handle->music_power = (uint8)music_power;
    }
    divoom_system_free_lock(pdivoom_light_music_gobal_handle->lock_handle);
}

#else
void divoom_light_music_update_freq(long int *music_freq)
{
    uint8 i, tmp;
    if(pdivoom_light_music_handle == NULL)
    {
        return;
    }
    pdivoom_light_music_handle->fft_flag = DIVOOM_DISP_MUSIC_FFT_MODE;
    if(pdivoom_light_music_handle->fft_flag != DIVOOM_DISP_MUSIC_FFT_MODE)
    {
        return;
    }
  //  divoom_print("divoom_light_music_update_freq\n");
   // divoom_print("<s:%d,%d,%d,%d>\n",music_freq[3], music_freq[0],music_freq[1],music_freq[2]);
    pdivoom_light_music_handle->tick = 0;
    for(i = 0; i < SCREEN_WIDTH_32; i ++)
    {
        tmp = music_freq[i];

        if(tmp >= SCREEN_WIDTH_32)
        {
            tmp = SCREEN_WIDTH_32 - 1;
        }
        pdivoom_light_music_handle->disp_info[i] = tmp;

    }
    pdivoom_light_music_handle->refresh_flag = 1;
    pdivoom_light_music_handle->music_power = (pdivoom_light_music_handle->disp_info[3]+ pdivoom_light_music_handle->disp_info[0]+pdivoom_light_music_handle->disp_info[1]+pdivoom_light_music_handle->disp_info[2])/3;
   // divoom_print("<%d,%d,%d,%d>\n",pdivoom_light_music_handle->disp_info[3], pdivoom_light_music_handle->disp_info[0],pdivoom_light_music_handle->disp_info[1],pdivoom_light_music_handle->disp_info[2]);
}
#endif


void divoom_light_music_get_static_pic(Pdivoom_light_music_ctrl pdivoom_light_music_handle)
{
    uint8 tmp;
	pdivoom_light_music_handle->play_status = 0;
    pdivoom_light_music_handle->static_tick ++;
    if((pdivoom_light_music_handle->static_tick & 1))
    {
        tmp = pdivoom_light_music_handle->back_disp_info[0];
        memmove(&pdivoom_light_music_handle->back_disp_info[0], &pdivoom_light_music_handle->back_disp_info[1], SCREEN_WIDTH_64 - 1);
        pdivoom_light_music_handle->back_disp_info[SCREEN_WIDTH_64 - 1] = tmp;
        memcpy(pdivoom_light_music_handle->disp_info, pdivoom_light_music_handle->back_disp_info, SCREEN_WIDTH_64);
    }
#if 0
    if((pdivoom_light_music_handle->disp_type == DIVOOM_DISP_MUSIC_EQ2_MODE) || (pdivoom_light_music_handle->disp_type == DIVOOM_DISP_MUSIC_EQ2_MODE)
         || (pdivoom_light_music_handle->disp_type == DIVOOM_DISP_MUSIC_ROBOT3_MODE)  || (pdivoom_light_music_handle->disp_type == DIVOOM_DISP_MUSIC_ROBOT4_MODE)
          || (pdivoom_light_music_handle->disp_type == DIVOOM_DISP_MUSIC_ROBOT5_MODE) || (pdivoom_light_music_handle->disp_type == DIVOOM_DISP_MUSIC_DANCE_GIRL_MODE))
    {
        pdivoom_light_music_handle->music_power = DIVOOM_LIGHT_MUSIC_GET_RAND(3);
    }
    else if(pdivoom_light_music_handle->disp_type == DIVOOM_DISP_MUSIC_CAMP_FIRE_MODE)
    {
        pdivoom_light_music_handle->music_power = DIVOOM_LIGHT_MUSIC_GET_RAND(3);
    }
    else
    #endif
    {
        pdivoom_light_music_handle->music_power = (pdivoom_light_music_handle->disp_info[0] + pdivoom_light_music_handle->disp_info[1]) /2;
    }
}


void divoom_light_music_get_random_color(Pdivoom_light_color_info color)
{
    uint32 index;

//    index = rand() %(sizeof(divoom_light_music_random_color_table)/sizeof(divoom_light_color_info));
    index = DIVOOM_LIGHT_MUSIC_GET_RAND(sizeof(divoom_light_music_random_color_table)/sizeof(divoom_light_color_info));
    memcpy(color, &divoom_light_music_random_color_table[index], sizeof(divoom_light_color_info));
}

void divoom_light_music_random_update_area_info(Pdivoom_light_music_ctrl pdivoom_light_music_handle)
{
    uint8 i;

    for(i = 0; i < DIVOOM_LIGHT_MUSIC_MAX_AREA; i++)
    {
        if(pdivoom_light_music_handle->area_table[i].edge)
        {
            pdivoom_light_music_handle->area_table[i].y += pdivoom_light_music_handle->area_table[i].speed;
            if(pdivoom_light_music_handle->area_table[i].y >= SCREEN_HEIGHT_32)
            {
                pdivoom_light_music_handle->area_table[i].edge = 0;
				pdivoom_light_music_handle->area_table[i].speed = 1;
				if(pdivoom_light_music_handle->rand_use > 0)
					pdivoom_light_music_handle->rand_use--;
            }
        }
    }
}

void divoom_light_music_random_create_area(Pdivoom_light_music_ctrl pdivoom_light_music_handle, Pdivoom_light_music_area parea_info)
{
	uint8 vol = divoom_com_get_vol();
    parea_info->x = DIVOOM_LIGHT_MUSIC_GET_RAND(SCREEN_WIDTH_32 - 2);
    parea_info->edge = (pdivoom_light_music_handle->music_power + 3) >> 2;//rand() %(4) + 1;
    if(parea_info->edge == 0)
    {
        parea_info->edge = 1;
    }
    parea_info->y = 0 - parea_info->edge;

    divoom_light_music_get_random_color(&parea_info->color);

	if(vol < 5)
	{
		pdivoom_light_music_handle->rand_num = 3;
	}
	else if(vol >= 5 && vol < 10)
	{
		pdivoom_light_music_handle->rand_num = 6;
	}
	else
	{
		pdivoom_light_music_handle->rand_num = 9;
	}

	if(pdivoom_light_music_handle->rand_use < pdivoom_light_music_handle->rand_num)
	{
		parea_info->speed = 2;
		pdivoom_light_music_handle->rand_use++;
	}
	else
	{
		parea_info->speed = 1;
	}
}

void divoom_light_music_line_create_area(Pdivoom_light_music_ctrl pdivoom_light_music_handle, Pdivoom_light_music_area parea_info)
{
	uint8 vol = divoom_com_get_vol();
    parea_info->x = DIVOOM_LIGHT_MUSIC_GET_RAND(SCREEN_WIDTH_32 - 1);
    parea_info->edge = (pdivoom_light_music_handle->music_power + 3) >> 2;//rand() %(4) + 1;
    if(parea_info->edge == 0)
    {
        parea_info->edge = 1;
    }
    parea_info->y = 0 - parea_info->edge;

	if(vol < 5)
	{
		pdivoom_light_music_handle->rand_num = 3;
	}
	else if(vol >= 5 && vol < 10)
	{
		pdivoom_light_music_handle->rand_num = 6;
	}
	else
	{
		pdivoom_light_music_handle->rand_num = 9;
	}

	if(pdivoom_light_music_handle->rand_use < pdivoom_light_music_handle->rand_num)
	{
		parea_info->speed = 2;
		pdivoom_light_music_handle->rand_use++;
	}
	else
	{
		parea_info->speed = 1;
	}
}

uint8 divoom_light_music_line_check_area(Pdivoom_light_music_ctrl pdivoom_light_music_handle, Pdivoom_light_music_area parea_info)
{
    uint8 i;

    if(parea_info->x + parea_info->edge > SCREEN_WIDTH_32)
    {
        return false;

    }

    for(i = 0; i < DIVOOM_LIGHT_MUSIC_MAX_AREA; i++)
    {
        if(pdivoom_light_music_handle->area_table[i].edge == 0)
        {
            continue;
        }
        if(pdivoom_light_music_handle->area_table[i].x == parea_info->x)
        {
            if(parea_info->y + parea_info->edge >= pdivoom_light_music_handle->area_table[i].y)
            {
                return false;
            }
        }
        if(pdivoom_light_music_handle->area_table[i].y <= 0)
        {
            if(pdivoom_light_music_handle->area_table[i].x >= parea_info->x)
            {
                if(pdivoom_light_music_handle->area_table[i].x <= parea_info->x + 1)
                {
                    return false;
                }
            }
            else
            {
                if(pdivoom_light_music_handle->area_table[i].x + 1 >= parea_info->x)
                {
                    return false;
                }
            }
        }
    }

    return true;
}



uint8 divoom_light_music_random_check_area(Pdivoom_light_music_ctrl pdivoom_light_music_handle, Pdivoom_light_music_area parea_info)
{
    uint8 i;

    if(parea_info->x + parea_info->edge > SCREEN_WIDTH_32)
    {
        return false;

    }

    for(i = 0; i < DIVOOM_LIGHT_MUSIC_MAX_AREA; i++)
    {
        if(pdivoom_light_music_handle->area_table[i].edge == 0)//找出有原有对象的图案，并进行比较是否重叠
        {
            continue;
        }
        if(pdivoom_light_music_handle->area_table[i].y <= 0)    //y<=0说明正在开始掉落
        {
            if(pdivoom_light_music_handle->area_table[i].x > parea_info->x) //避免左右方向重叠
            {
                if(parea_info->x + parea_info->edge >= pdivoom_light_music_handle->area_table[i].x)
                {
                    return false;
                }
            }
            else
            {
                if(pdivoom_light_music_handle->area_table[i].x + pdivoom_light_music_handle->area_table[i].edge >= parea_info->x)
                {
                    return false;
                }
            }
        }
    }

    return true;
}


uint8 divoom_light_music_random_insert_area(Pdivoom_light_music_ctrl pdivoom_light_music_handle,Pdivoom_light_music_area parea_info)
{
    uint8 i;

    for(i = 0; i < DIVOOM_LIGHT_MUSIC_MAX_AREA; i++)
    {
        if(pdivoom_light_music_handle->area_table[i].edge == 0)
        {
            memcpy(&pdivoom_light_music_handle->area_table[i], parea_info, sizeof(divoom_light_music_area));
            return true;
        }
    }
    return false;
}

//注意表的最大值小于divoom_light_music_random_color_table个数，目前为0-8
const uint8 music_random_square_color_index[15] = {1,5,2,6,3,3,4,6,7,8,8,6,1,0,5};

void divoom_light_music_random_square_init(Pdivoom_light_music_ctrl pdivoom_light_music_handle)
{
    uint16 i;
	uint8 vol = 8;
	if(vol < 5)
	{
		pdivoom_light_music_handle->rand_num = 3;
	}
	else if(vol >= 5 && vol < 10)
	{
		pdivoom_light_music_handle->rand_num = 6;
	}
	else
	{
		pdivoom_light_music_handle->rand_num = 9;
	}

    memset(pdivoom_light_music_handle->area_table, 0, sizeof(pdivoom_light_music_handle->area_table));

    pdivoom_light_music_handle->area_table[0].x = 0;
    pdivoom_light_music_handle->area_table[0].y = 0;
    pdivoom_light_music_handle->area_table[0].edge = 0;
    pdivoom_light_music_handle->area_table[0].speed = 1;


    pdivoom_light_music_handle->area_table[1].x = 2;
    pdivoom_light_music_handle->area_table[1].y = 4;
    pdivoom_light_music_handle->area_table[1].edge = 2;
    pdivoom_light_music_handle->area_table[1].speed = 1;


    pdivoom_light_music_handle->area_table[2].x = 8;
    pdivoom_light_music_handle->area_table[2].y = 2;
    pdivoom_light_music_handle->area_table[2].edge = 1;
    pdivoom_light_music_handle->area_table[2].speed = 1;



    pdivoom_light_music_handle->area_table[3].x = 16;
    pdivoom_light_music_handle->area_table[3].y = -4;
    pdivoom_light_music_handle->area_table[3].edge = 3;
    pdivoom_light_music_handle->area_table[3].speed = 1;


    pdivoom_light_music_handle->area_table[4].x = 26;
    pdivoom_light_music_handle->area_table[4].y = 0;
    pdivoom_light_music_handle->area_table[4].edge = 1;
    pdivoom_light_music_handle->area_table[4].speed = 1;


    pdivoom_light_music_handle->area_table[5].x = 14;
    pdivoom_light_music_handle->area_table[5].y = 4;
    pdivoom_light_music_handle->area_table[5].edge = 2;
    pdivoom_light_music_handle->area_table[5].speed = 1;




    pdivoom_light_music_handle->area_table[6].x = 22;
    pdivoom_light_music_handle->area_table[6].y = 4;
    pdivoom_light_music_handle->area_table[6].edge = 3;
    pdivoom_light_music_handle->area_table[6].speed = 1;



    pdivoom_light_music_handle->area_table[7].x = 0;
    pdivoom_light_music_handle->area_table[7].y = 10;
    pdivoom_light_music_handle->area_table[7].edge = 2;
    pdivoom_light_music_handle->area_table[7].speed = 1;



    pdivoom_light_music_handle->area_table[8].x = 6;
    pdivoom_light_music_handle->area_table[8].y = 8;
    pdivoom_light_music_handle->area_table[8].edge = 2;
    pdivoom_light_music_handle->area_table[8].speed = 1;



    pdivoom_light_music_handle->area_table[9].x =12;
    pdivoom_light_music_handle->area_table[9].y = 12;
    pdivoom_light_music_handle->area_table[9].edge = 3;
    pdivoom_light_music_handle->area_table[9].speed = 1;



    pdivoom_light_music_handle->area_table[10].x = 22;
    pdivoom_light_music_handle->area_table[10].y = 16;
    pdivoom_light_music_handle->area_table[10].edge = 2;
    pdivoom_light_music_handle->area_table[10].speed = 1;


    pdivoom_light_music_handle->area_table[11].x = 2;
    pdivoom_light_music_handle->area_table[11].y = 26;
    pdivoom_light_music_handle->area_table[11].edge = 1;
    pdivoom_light_music_handle->area_table[11].speed = 1;



    pdivoom_light_music_handle->area_table[12].x = 6;
    pdivoom_light_music_handle->area_table[12].y = 22;
    pdivoom_light_music_handle->area_table[12].edge = 3;
    pdivoom_light_music_handle->area_table[12].speed = 1;

    pdivoom_light_music_handle->area_table[13].x = 18;
    pdivoom_light_music_handle->area_table[13].y = 22;
    pdivoom_light_music_handle->area_table[13].edge = 4;
    pdivoom_light_music_handle->area_table[13].speed = 1;


    pdivoom_light_music_handle->area_table[14].x = 28;
    pdivoom_light_music_handle->area_table[14].y = 26;
    pdivoom_light_music_handle->area_table[14].edge = 1;
    pdivoom_light_music_handle->area_table[14].speed = 1;

	pdivoom_light_music_handle->rand_use = 0;
	for(i=0; i< pdivoom_light_music_handle->rand_num; i++)
	{
		uint8 rand_num = DIVOOM_LIGHT_MUSIC_GET_RAND(DIVOOM_LIGHT_MUSIC_MAX_AREA)%pdivoom_light_music_handle->rand_num;
		if(pdivoom_light_music_handle->area_table[rand_num].speed != 1)
		{
			pdivoom_light_music_handle->area_table[rand_num].speed = 2;
		}
		else
		{
			pdivoom_light_music_handle->area_table[(rand_num + DIVOOM_LIGHT_MUSIC_GET_RAND(rand_num))%pdivoom_light_music_handle->rand_num].speed = 2;
		}
		pdivoom_light_music_handle->rand_use++;
	}

    for(i = 0; i < DIVOOM_LIGHT_MUSIC_MAX_AREA; i++)
    {
//        divoom_light_music_get_random_color(&pdivoom_light_music_handle->area_table[i].color);
        memcpy(&pdivoom_light_music_handle->area_table[i].color,
               &divoom_light_music_random_color_table[music_random_square_color_index[i]], sizeof(divoom_light_color_info));
    }
}



void divoom_light_music_random_square_format(Pdivoom_light_music_ctrl pdivoom_light_music_handle)
{
    uint8 i, j , k;//time;
    divoom_light_music_area area_info;

    //time = pdivoom_light_music_handle->music_power;
    //if(time >= SCREEN_HEIGHT_32 - 1)
    //{
    //    time = 1;
    //}
    //else
    //{
    //    time = SCREEN_HEIGHT_32 - time;
    //}

    divoom_microtask_update(pdivoom_light_music_handle->task_id, DIVOOM_LIGHT_MUSIC_REFRESH_TIME);
    //divoom_print("update time: %d, %d\n", time, pdivoom_light_music_handle->music_power);
    memset(pdivoom_light_music_handle->disp_buf, 0,
        SCREEN_DOT_NUM_32 * DIVOOM_APP_RGB565_BYTES_PER_PX);
    for(i = 0; i < DIVOOM_LIGHT_MUSIC_MAX_AREA; i++)
    {
        if(pdivoom_light_music_handle->area_table[i].edge == 0)
        {
            continue ;
        }
        for(j = 0; j < pdivoom_light_music_handle->area_table[i].edge; j ++)
        {
            for(k = 0; k < pdivoom_light_music_handle->area_table[i].edge; k ++)
            {
//                if(pdivoom_light_music_handle->area_table[i].y + k >= 0 && pdivoom_light_music_handle->area_table[i].y + k < SCREEN_HEIGHT_32)
                {
                    divoom_disp_ctrl_set_data_xy(pdivoom_light_music_handle->area_table[i].x + j, pdivoom_light_music_handle->area_table[i].y + k,
                                              &pdivoom_light_music_handle->area_table[i].color, pdivoom_light_music_handle->disp_buf);
                }
            }
        }
    }
//    while(1)
//    {
//        divoom_light_music_random_create_area(&area_info);
//        if(divoom_light_music_random_check_area(&area_info))
//        {
//            divoom_light_music_random_insert_area(&area_info);
//        }
//        else
//        {
//            break;
//        }
//    }

    for(i = 0; i < 4; i++)
    {
        divoom_light_music_random_create_area(pdivoom_light_music_handle, &area_info);
        if(divoom_light_music_random_check_area(pdivoom_light_music_handle, &area_info))
        {
            if(false == divoom_light_music_random_insert_area(pdivoom_light_music_handle, &area_info))
            {
                break;
            }
        }
        else
        {
            break;
        }
    }

    divoom_light_music_random_update_area_info(pdivoom_light_music_handle);
}



void divoom_light_music_random_line_format(Pdivoom_light_music_ctrl pdivoom_light_music_handle)
{
    uint8 i, k;// time;
    divoom_light_music_area area_info;
    divoom_light_color_info color = {0,255,0};

    //time = pdivoom_light_music_handle->music_power;
    //if(time >= SCREEN_HEIGHT_32 - 1)
    //{
    //    time = 1;
    //}
    //else
    //{
    //    time = SCREEN_HEIGHT_32 - time;
    //}
    divoom_microtask_update(pdivoom_light_music_handle->task_id, DIVOOM_LIGHT_MUSIC_REFRESH_TIME);
    // divoom_print("update time: %d, %d\n", time, pdivoom_light_music_handle->music_power);

    memset(pdivoom_light_music_handle->disp_buf, 0,
        SCREEN_DOT_NUM_32 * DIVOOM_APP_RGB565_BYTES_PER_PX);
    for(i = 0; i < DIVOOM_LIGHT_MUSIC_MAX_AREA; i++)
    {
        if(pdivoom_light_music_handle->area_table[i].edge == 0)
        {
            continue ;
        }
        for(k = 0; k < pdivoom_light_music_handle->area_table[i].edge; k ++)
        {
            if(pdivoom_light_music_handle->area_table[i].y + k >= 0 && pdivoom_light_music_handle->area_table[i].y + k < SCREEN_HEIGHT_32)
            {
                divoom_disp_ctrl_set_dot(pdivoom_light_music_handle->area_table[i].x, pdivoom_light_music_handle->area_table[i].y + k,
                                          &color, pdivoom_light_music_handle->disp_buf);
            }
        }
    }
    for(i = 0; i < 4; i++)
    {
        divoom_light_music_line_create_area(pdivoom_light_music_handle, &area_info);
        if(divoom_light_music_line_check_area(pdivoom_light_music_handle, &area_info))
        {
            if(false == divoom_light_music_random_insert_area(pdivoom_light_music_handle, &area_info))
            {
                break;
            }
        }
        else
        {
            break;
        }
    }
    divoom_light_music_random_update_area_info(pdivoom_light_music_handle);
}


void divoom_light_music_refresh_freq(Pdivoom_light_music_ctrl pdivoom_light_music_handle)
{
    uint16 i;
    //divoom_print("fft_flag:%d\n",pdivoom_light_music_handle->fft_flag);
    if(pdivoom_light_music_handle->fft_flag == DIVOOM_DISP_MUSIC_RANDOM_MODE)
    {
        uint8 disp_buf[SCREEN_WIDTH_64] = {0};
        uint32 tmp;

        if(pdivoom_light_music_handle->pcm_value)
        {
			pdivoom_light_music_handle->play_status = 1;
            tmp = divoom_system_get_tickms();
            sprintf((char *)(disp_buf + 1), "%-8lu", (long unsigned int)pdivoom_light_music_handle->pcm_value);            
            disp_buf[0] = (tmp % 15) + '0';
            disp_buf[9] = ((tmp/2) % 15) + '0';              
            disp_buf[10] = ((tmp/5) % 14) + '0';            
            disp_buf[11] = ((tmp/4) % 12) + '0';
            disp_buf[12] = ((tmp/8) % 7) + '0';
            disp_buf[13] = ((tmp/12) % 6) + '0';
            disp_buf[14] = ((tmp/15) % 5) + '0';
            disp_buf[15] = divoom_com_get_random(5) + '0';
            pdivoom_light_music_handle->refresh_flag = 1;
            for(i = 0; i < SCREEN_WIDTH_64; i++)
            {
				if(disp_buf[i] == 32)	//判断是否为空格 ' '   -- 32
					pdivoom_light_music_handle->disp_info[i] = divoom_com_get_random(7);
				else
					pdivoom_light_music_handle->disp_info[i] = disp_buf[i] - '0';

                if(pdivoom_light_music_handle->disp_info[i] == 0)
                {
//                    pdivoom_light_music_handle->disp_info[i] = divoom_com_get_random(5);
                    pdivoom_light_music_handle->disp_info[i] = 1;                    
                }
            }
            pdivoom_light_music_handle->music_power = pdivoom_light_music_handle->disp_info[1];
        }
        else
        {
            divoom_light_music_get_static_pic(pdivoom_light_music_handle);
        }
        pdivoom_light_music_handle->pcm_value = 0;
    }
    else if(pdivoom_light_music_handle->fft_flag == DIVOOM_DISP_MUSIC_FM_MODE)
    {
        for(i = 0; i < SCREEN_WIDTH_32; i ++)
        {
            pdivoom_light_music_handle->disp_info[i] = divoom_com_get_random(SCREEN_HEIGHT_32);
        }
        pdivoom_light_music_handle->refresh_flag = 1;
        pdivoom_light_music_handle->music_power = pdivoom_light_music_handle->disp_info[1];
		pdivoom_light_music_handle->play_status = 1;

    }
    else
    {
		pdivoom_light_music_handle->play_status = 1;
        /* FFT 模式保留 update_freq 计算的全频段能量；其他输入才使用静态兜底。 */
        if(pdivoom_light_music_handle->refresh_flag == 0 &&
            pdivoom_light_music_handle->fft_flag != DIVOOM_DISP_MUSIC_FFT_MODE)
        {
            divoom_light_music_get_static_pic(pdivoom_light_music_handle);
        }
    }
	
}

void divoom_light_music_main(void *data)
{
    Pdivoom_light_music_ctrl pdivoom_light_music_handle = (Pdivoom_light_music_ctrl)data;
    uint16 i,j;
    uint8 fine_spectrum = 1;
    int new_type;
//    uint8 *pic_data = NULL ;
//    uint16 pic_len;

    {
//    divoom_print("tick:%d,   %d\n",pdivoom_light_music_handle->tick,pdivoom_light_music_handle->refresh_flag);   
        if(pdivoom_light_music_handle->refresh_flag == 0)
        {
            if(pdivoom_light_music_handle->tick < 5)
            {
//                divoom_print("tick_return:%d\n",pdivoom_light_music_handle->tick);               
                pdivoom_light_music_handle->tick ++;
                return;
            }
        }
    }
    new_type = divoom_eq_image_manage_get_change_eq_mode(&pdivoom_light_music_handle->clock_info);
    if(pdivoom_light_music_gobal_handle == NULL)
    {
        return;
    }
    divoom_system_get_lock(pdivoom_light_music_gobal_handle->lock_handle);
    if(new_type != -1)
    {        

        divoom_print("change music type: %d, %d\n", new_type, pdivoom_light_music_handle->disp_type);
        pdivoom_light_music_handle->disp_type = new_type;
	    divoom_light_music_init_mode(pdivoom_light_music_handle, new_type);
        divoom_light_music_custom_eq_render_first_frame(pdivoom_light_music_handle);
        
        pdivoom_light_music_handle->refresh_flag = 1;
    }
    divoom_light_music_refresh_freq(pdivoom_light_music_handle);
    pdivoom_light_music_handle->refresh_flag = 0;
    if(!divoom_light_music_is_custom_eq_mode(pdivoom_light_music_handle))
    {
        memset(pdivoom_light_music_handle->disp_buf, 0, SCREEN_DOT_NUM_64 * DIVOOM_APP_RGB565_BYTES_PER_PX);
    }

    switch(pdivoom_light_music_handle->disp_type)
    {
        case DIVOOM_DISP_MUSIC_EQ2_MODE:
        case DIVOOM_DISP_MUSIC_EQ4_MODE:
        case DIVOOM_DISP_MUSIC_EQ7_MODE:
        case DIVOOM_DISP_MUSIC_EQ8_MODE:
        case DIVOOM_DISP_MUSIC_EQ10_MODE:
        case DIVOOM_DISP_MUSIC_EQ11_MODE:
            divoom_light_music_get_row_pic_disp_buf(pdivoom_light_music_handle, pdivoom_light_music_handle->disp_buf, pdivoom_light_music_handle->disp_info);
            divoom_light_music_add_row_extern_pic(pdivoom_light_music_handle, pdivoom_light_music_handle->disp_buf);
            break;
            
        case DIVOOM_DISP_MUSIC_EQ12_MODE:
        case DIVOOM_DISP_MUSIC_NORMAL_MODE:
            for(i = 0; i < SCREEN_WIDTH_64; i++)
            {
				if(pdivoom_light_music_handle->play_status){
					//更新漂浮点的值
					if(pdivoom_light_music_handle->disp_info[i] >= pdivoom_light_music_handle->top_pos[i])
						pdivoom_light_music_handle->top_pos[i] = pdivoom_light_music_handle->disp_info[i];//顶上去最新高度
					else {
						if(pdivoom_light_music_handle->down_pos[i] < pdivoom_light_music_handle->top_pos[i])
							pdivoom_light_music_handle->top_pos[i] -= 1;//漂浮点下落速度1格/次
					}	  
					//更新频率柱的值
					if(pdivoom_light_music_handle->disp_info[i] >= pdivoom_light_music_handle->down_pos[i])
						pdivoom_light_music_handle->down_pos[i] = pdivoom_light_music_handle->disp_info[i];
					else{
						if(pdivoom_light_music_handle->down_pos[i] >= 2)
							pdivoom_light_music_handle->down_pos[i] -= 2; //下落速度2格/次
						else if(pdivoom_light_music_handle->down_pos[i] >= 1)
							pdivoom_light_music_handle->down_pos[i]  = 0; 
					}

	                for(j = 0; j < pdivoom_light_music_handle->down_pos[i]; j ++)
	                {
						divoom_disp_ctrl_set_dot_64(i,SCREEN_WIDTH_64-j-1, &pdivoom_light_music_handle->down_rgb,pdivoom_light_music_handle->disp_buf);
	                }
	                if(pdivoom_light_music_handle->top_pos[i] == 0)
	                {
	                    divoom_disp_ctrl_set_dot_64(i,SCREEN_WIDTH_64 -1, /*&freq_high_color*/&pdivoom_light_music_handle->top_rgb ,pdivoom_light_music_handle->disp_buf);
	                }
	                else if(pdivoom_light_music_handle->top_pos[i] + 2 > SCREEN_WIDTH_64 -1)
	                {
	                    divoom_disp_ctrl_set_dot_64(i,0, /*&freq_high_color*/&pdivoom_light_music_handle->top_rgb ,pdivoom_light_music_handle->disp_buf);
	                }
	                else
	                {
	                    divoom_disp_ctrl_set_dot_64(i, SCREEN_WIDTH_64 - pdivoom_light_music_handle->top_pos[i] - 2 , /*&freq_high_color*/&pdivoom_light_music_handle->top_rgb ,pdivoom_light_music_handle->disp_buf);
	                }
				}else
				{
					for(i = 0; i < SCREEN_WIDTH_64; i++)
					{
						for(j = 0; j < pdivoom_light_music_handle->disp_info[i]; j ++)
						{
							divoom_disp_ctrl_set_dot_64(i,SCREEN_WIDTH_64-j-1, &pdivoom_light_music_handle->down_rgb/*&freq_fill_color*/,pdivoom_light_music_handle->disp_buf);
						}
						if(pdivoom_light_music_handle->disp_info[i] == 0)
						{
							divoom_disp_ctrl_set_dot_64(i,SCREEN_WIDTH_64 -1, /*&freq_high_color*/&pdivoom_light_music_handle->top_rgb ,pdivoom_light_music_handle->disp_buf);
						}
						else if(pdivoom_light_music_handle->disp_info[i] + 2 > SCREEN_WIDTH_64 -1)
						{
							divoom_disp_ctrl_set_dot_64(i,0, /*&freq_high_color*/&pdivoom_light_music_handle->top_rgb ,pdivoom_light_music_handle->disp_buf);
						}
						else
						{
							divoom_disp_ctrl_set_dot_64(i, SCREEN_WIDTH_64 - pdivoom_light_music_handle->disp_info[i] - 2 , /*&freq_high_color*/&pdivoom_light_music_handle->top_rgb ,pdivoom_light_music_handle->disp_buf);
						}
					}
				}
            }
			divoom_light_music_finish_legacy_frame(pdivoom_light_music_handle,
				SCREEN_WIDTH_64, 0, 1);
			divoom_system_free_lock(pdivoom_light_music_gobal_handle->lock_handle);
            //divoom_screen_disp_rgb_data(pdivoom_light_music_handle->disp_buf);
            return;

            break;
        case DIVOOM_DISP_MUSIC_EQ6_MODE:
        case DIVOOM_DISP_MUSIC_CENTER_MODE:
            {
                for(i = 0; i < SCREEN_WIDTH_64; i++)
                {
                    //更新漂浮点的值
                    if(pdivoom_light_music_handle->disp_info[i] >= pdivoom_light_music_handle->top_pos[i])
                        pdivoom_light_music_handle->top_pos[i] = pdivoom_light_music_handle->disp_info[i];//顶上去最新高度
                    else 
					{
                        if(pdivoom_light_music_handle->down_pos[i] < pdivoom_light_music_handle->top_pos[i])
                            pdivoom_light_music_handle->top_pos[i] -= 1;//漂浮点下落速度1格/次
                    }     
                    //更新频率柱的值
                    if(pdivoom_light_music_handle->disp_info[i] >= pdivoom_light_music_handle->down_pos[i])
                        pdivoom_light_music_handle->down_pos[i] = pdivoom_light_music_handle->disp_info[i];
                    else
					{
                        if(pdivoom_light_music_handle->down_pos[i] >= 3)
                            pdivoom_light_music_handle->down_pos[i] -= 3; //下落速度3格/次
                        else if(pdivoom_light_music_handle->down_pos[i] >= 1)
                            pdivoom_light_music_handle->down_pos[i] = 0; 
                    }

                    if(pdivoom_light_music_handle->down_pos[i]/2 == 0)
                    {
                        divoom_disp_ctrl_set_dot_64(i, SCREEN_WIDTH_64/2 - 1, (Pdivoom_light_color_info)&divoom_light_music_up_down_color_table[0], pdivoom_light_music_handle->disp_buf);
                        divoom_disp_ctrl_set_dot_64(i, SCREEN_WIDTH_64/2, (Pdivoom_light_color_info)&divoom_light_music_up_down_color_table[0], pdivoom_light_music_handle->disp_buf);
                    }
                    else
                    {
                        for(j = 0; j < pdivoom_light_music_handle->down_pos[i]/2; j ++)
                        {
                            divoom_disp_ctrl_set_dot_64(i, SCREEN_WIDTH_64/2 - 1 - j, (Pdivoom_light_color_info)&divoom_light_music_up_down_color_table[j], pdivoom_light_music_handle->disp_buf);
                            divoom_disp_ctrl_set_dot_64(i, SCREEN_WIDTH_64/2 + j, (Pdivoom_light_color_info)&divoom_light_music_up_down_color_table[j], pdivoom_light_music_handle->disp_buf);
                        }
                    }
                    if(pdivoom_light_music_handle->top_pos[i]/2 == 0)
                    {
                        divoom_disp_ctrl_set_dot_64(i, SCREEN_WIDTH_64/2 - 1, (Pdivoom_light_color_info)&divoom_light_music_up_down_color_table[SCREEN_WIDTH_64/2 - 1], pdivoom_light_music_handle->disp_buf);
                        divoom_disp_ctrl_set_dot_64(i, SCREEN_WIDTH_64/2 ,(Pdivoom_light_color_info)&divoom_light_music_up_down_color_table[SCREEN_WIDTH_64/2 - 1], pdivoom_light_music_handle->disp_buf);
                    }
                    else if(pdivoom_light_music_handle->top_pos[i]/2 + 2 > SCREEN_WIDTH_64/2 -1)
                    {
                        divoom_disp_ctrl_set_dot_64(i,0, (Pdivoom_light_color_info)&divoom_light_music_up_down_color_table[0] ,pdivoom_light_music_handle->disp_buf);
                        divoom_disp_ctrl_set_dot_64(i,SCREEN_WIDTH_64 - 1,(Pdivoom_light_color_info)&divoom_light_music_up_down_color_table[0] ,pdivoom_light_music_handle->disp_buf);
                    }
                    else
                    {
                        divoom_disp_ctrl_set_dot_64(i, (SCREEN_WIDTH_64 - pdivoom_light_music_handle->top_pos[i])/2 - 2 , (Pdivoom_light_color_info)&divoom_light_music_up_down_color_table[(SCREEN_WIDTH_64 - pdivoom_light_music_handle->top_pos[i])/2 - 2 ],pdivoom_light_music_handle->disp_buf);
                        divoom_disp_ctrl_set_dot_64(i, SCREEN_WIDTH_64 - 1 -((SCREEN_WIDTH_64 - pdivoom_light_music_handle->top_pos[i])/2 - 2) , (Pdivoom_light_color_info)&divoom_light_music_up_down_color_table[(SCREEN_WIDTH_64 - pdivoom_light_music_handle->top_pos[i])/2 - 2 ] ,pdivoom_light_music_handle->disp_buf);
                    }
                }
            }
			divoom_light_music_finish_legacy_frame(pdivoom_light_music_handle,
				SCREEN_WIDTH_64, 0, 1);
			divoom_system_free_lock(pdivoom_light_music_gobal_handle->lock_handle);
            return;
            break;
            
        case DIVOOM_DISP_MUSIC_RANDOM_SQUARE1_MODE:
            fine_spectrum = 0;
            divoom_light_music_random_square_format(pdivoom_light_music_handle);
            break;
        case DIVOOM_DISP_MUSIC_RANDOM_SQUARE2_MODE:
            fine_spectrum = 0;
            divoom_light_music_random_line_format(pdivoom_light_music_handle);
            break;
            
        default :
        {
            uint16 disp_level;
            
            pdivoom_light_music_handle->tick = 0;
            disp_level = divoom_light_music_get_disp_level(pdivoom_light_music_handle);

            if(pdivoom_light_music_handle->disp_type >= DIVOOM_DISP_MUSIC_END_MODE &&
                pdivoom_light_music_handle->disp_type != DIVOOM_DISP_MUSIC_TMP_INDEX &&
                pdivoom_light_music_handle->pic_handle != NULL &&
                pdivoom_light_music_handle->pic_handle->zstd_handle != NULL)
            {
                divoom_light_music_custom_eq_decode_to_disp(pdivoom_light_music_handle, disp_level);
            }
            else if(pdivoom_light_music_handle->pic_handle != NULL &&
                pdivoom_light_music_handle->pic_handle->pimage_table[disp_level] != NULL &&
                pdivoom_light_music_handle->pic_handle->pimage_table[disp_level]->image_data != NULL)
            {
                memcpy(pdivoom_light_music_handle->disp_buf, pdivoom_light_music_handle->pic_handle->pimage_table[disp_level]->image_data, pdivoom_light_music_handle->pic_handle->width * pdivoom_light_music_handle->pic_handle->width * DIVOOM_APP_RGB565_BYTES_PER_PX);
                pdivoom_light_music_handle->disp_width = pdivoom_light_music_handle->pic_handle->width;
                if(pdivoom_light_music_handle->disp_type == DIVOOM_DISP_MUSIC_EQ12_MODE)
                {
                    /* 场景型 EQ12 保留角色原画，只做像素保持扩展，不叠加细频谱。 */
                    divoom_light_music_finish_legacy_frame(pdivoom_light_music_handle,
                        (uint16)pdivoom_light_music_handle->disp_width, 0, 0);
                }
            }
			//divoom_screen_disp_send_rgb_data(pdivoom_light_music_handle->disp_buf,pdivoom_light_music_handle->pic_handle->width);
			divoom_system_free_lock(pdivoom_light_music_gobal_handle->lock_handle);
            return;
        }        
    }
	divoom_light_music_finish_legacy_frame(pdivoom_light_music_handle,
		SCREEN_WIDTH_32, fine_spectrum, 1);
	divoom_system_free_lock(pdivoom_light_music_gobal_handle->lock_handle);
}

void divoom_light_music_get_disp_buf(void *user_data, T_U8 *disp_buf, int *image_width)
{
    Pdivoom_light_music_ctrl pdivoom_light_music_handle;
    
    divoom_system_get_lock(pdivoom_light_music_gobal_handle->lock_handle);

    pdivoom_light_music_handle = divoom_light_music_global_get_handle(user_data);
    if(pdivoom_light_music_handle == NULL)
    {
        if(disp_buf)
        {
            memset(disp_buf, 0, SCREEN_DOT_NUM_64 * DIVOOM_APP_RGB565_BYTES_PER_PX);
        }
        if(image_width)
        {
            *image_width = 64;
        }
        divoom_system_free_lock(pdivoom_light_music_gobal_handle->lock_handle);
        return;
    }
    if(divoom_light_music_is_custom_eq_mode(pdivoom_light_music_handle) &&
        pdivoom_light_music_handle->custom_eq_stream_level < 0)
    {
        /* 新 EQ 首帧未解完前保持 LVGL 上一帧，避免黑像素被当作透明 */
        if(image_width)
        {
            *image_width = pdivoom_light_music_handle->disp_width;
        }
        divoom_system_free_lock(pdivoom_light_music_gobal_handle->lock_handle);
        return;
    }
    if(disp_buf)
    {
        memcpy(disp_buf, pdivoom_light_music_handle->disp_buf, (uint32)pdivoom_light_music_handle->disp_width * (uint32)pdivoom_light_music_handle->disp_width * DIVOOM_APP_RGB565_BYTES_PER_PX);
    }
    if(image_width)
    {
        *image_width = pdivoom_light_music_handle->disp_width;
 	}
    divoom_system_free_lock(pdivoom_light_music_gobal_handle->lock_handle);
}

void divoom_light_music_init_mode(Pdivoom_light_music_ctrl pdivoom_light_music_handle, uint8 disp_type)
{
    char *image_name;
    int user_id = divoom_main_get_host_id();

    /* 第六、十二项沿用已有中心/普通频谱，只保留菜单项编号。 */
    if(disp_type == DIVOOM_DISP_MUSIC_EQ6_MODE)
    {
        disp_type = DIVOOM_DISP_MUSIC_CENTER_MODE;
    }
    else if(disp_type == DIVOOM_DISP_MUSIC_EQ12_MODE)
    {
        disp_type = DIVOOM_DISP_MUSIC_NORMAL_MODE;
    }

    image_name = divoom_light_app_get_cur_image_name();
    if(image_name == NULL)
    {
        return;
    }
    image_name[0] = 0;
    pdivoom_light_music_handle->row_tick = 0;
    memset(pdivoom_light_music_handle->top_pos, 0, sizeof(pdivoom_light_music_handle->top_pos));
    memset(pdivoom_light_music_handle->down_pos, 0, sizeof(pdivoom_light_music_handle->down_pos));
    if(disp_type < DIVOOM_DISP_MUSIC_END_MODE &&
        disp_type != DIVOOM_DISP_MUSIC_EQ12_MODE)
    {
        memset(pdivoom_light_music_handle->disp_buf, 0, SCREEN_DOT_NUM_64 * DIVOOM_APP_RGB565_BYTES_PER_PX);
    }
    if(disp_type < DIVOOM_DISP_MUSIC_END_MODE &&
        disp_type != DIVOOM_DISP_MUSIC_EQ12_MODE)
    {
        if(pdivoom_light_music_handle->pic_handle)
        {
            divoom_http_download_image_free_image_handle(pdivoom_light_music_handle->pic_handle);
            pdivoom_light_music_handle->pic_handle = NULL;
        }
        pdivoom_light_music_handle->custom_eq_stream_level = -1;
    }
	if(disp_type == DIVOOM_DISP_MUSIC_RANDOM_SQUARE1_MODE || disp_type == DIVOOM_DISP_MUSIC_RANDOM_SQUARE2_MODE)
	{
		divoom_light_music_random_square_init(pdivoom_light_music_handle);
	}
    else if(disp_type ==  DIVOOM_DISP_MUSIC_EQ2_MODE)
    {
        divoom_light_music_format_row_pic_info(pdivoom_light_music_handle, DIVOOM_MANAGE_IMAGE_ID_BASE_NOTICE_NET_PIC + DIVOOM_lOCAL_USE_PIC_EQ_POS_PIC_2, DIVOOM_MANAGE_IMAGE_ID_BASE_NOTICE_NET_PIC + DIVOOM_lOCAL_USE_PIC_EQ_POS_PIC_2_EX, 16);
    }
    else if(disp_type ==  DIVOOM_DISP_MUSIC_EQ4_MODE)
    {
        divoom_light_music_format_row_pic_info(pdivoom_light_music_handle, DIVOOM_MANAGE_IMAGE_ID_BASE_NOTICE_NET_PIC + DIVOOM_lOCAL_USE_PIC_EQ_POS_PIC_4, 0 , 31);
    }
    else if(disp_type ==  DIVOOM_DISP_MUSIC_EQ6_MODE)
    {
        divoom_light_music_format_row_pic_info(pdivoom_light_music_handle, DIVOOM_MANAGE_IMAGE_ID_BASE_NOTICE_NET_PIC + DIVOOM_lOCAL_USE_PIC_EQ_POS_PIC_6,0 ,  31);
    }
    else if(disp_type ==  DIVOOM_DISP_MUSIC_EQ7_MODE)
    {
        divoom_light_music_format_row_pic_info(pdivoom_light_music_handle, DIVOOM_MANAGE_IMAGE_ID_BASE_NOTICE_NET_PIC + DIVOOM_lOCAL_USE_PIC_EQ_POS_PIC_7,0 ,  31);
    }
    else if(disp_type ==  DIVOOM_DISP_MUSIC_EQ8_MODE)
    {
        divoom_light_music_format_row_pic_info(pdivoom_light_music_handle, DIVOOM_MANAGE_IMAGE_ID_BASE_NOTICE_NET_PIC + DIVOOM_lOCAL_USE_PIC_EQ_POS_PIC_8,0 ,  31);
    }
    else if(disp_type ==  DIVOOM_DISP_MUSIC_EQ10_MODE)
    {
        divoom_light_music_format_row_pic_info(pdivoom_light_music_handle, DIVOOM_MANAGE_IMAGE_ID_BASE_NOTICE_NET_PIC + DIVOOM_lOCAL_USE_PIC_EQ_POS_PIC_10,0 ,  16);
    }
    else if(disp_type ==  DIVOOM_DISP_MUSIC_EQ11_MODE)
    {
        divoom_light_music_format_row_pic_info(pdivoom_light_music_handle, DIVOOM_MANAGE_IMAGE_ID_BASE_NOTICE_NET_PIC + DIVOOM_lOCAL_USE_PIC_EQ_POS_PIC_11, DIVOOM_MANAGE_IMAGE_ID_BASE_NOTICE_NET_PIC + DIVOOM_lOCAL_USE_PIC_EQ_POS_PIC_11_EX , 21);
    }
    else if(disp_type >= DIVOOM_DISP_MUSIC_END_MODE || disp_type ==  DIVOOM_DISP_MUSIC_EQ12_MODE)
    {
        if(pdivoom_light_music_handle->pic_handle)
        {
            divoom_http_download_image_free_image_handle(pdivoom_light_music_handle->pic_handle);
            pdivoom_light_music_handle->pic_handle = NULL;
        }
        if(disp_type == DIVOOM_DISP_MUSIC_TMP_INDEX)
        {
            divoom_light_app_app_gif_file_lock_ctrl(1);
            pdivoom_light_music_handle->pic_handle = divoom_manage_image_read(DIVOOM_MANAGE_IMAGE_SERVER_GIF, 1, NULL, NULL);
            divoom_light_app_app_gif_file_lock_ctrl(0);
            user_id = divoom_light_app_get_app_author_id();
            strcpy(image_name, divoom_light_app_get_app_image_name());
        }
        else if(disp_type ==  DIVOOM_DISP_MUSIC_EQ12_MODE)
        {
            pdivoom_light_music_handle->pic_handle = divoom_manage_image_read(DIVOOM_MANAGE_IMAGE_NOTICE_NET_GIF, DIVOOM_MANAGE_IMAGE_ID_BASE_NOTICE_NET_PIC + DIVOOM_lOCAL_USE_PIC_EQ_POS_PIC_12, NULL, NULL);
        }
        else
        {            
            if(divoom_system_get_ram_largest_free_block_size() < DIVOOM_LIGHT_MUSIC_ZSTD_MIN_LARGEST_KB)
            {
                divoom_print("custom_eq skip zstd: largest=%ldKB need>=%dKB free=%ldKB\n", divoom_system_get_ram_largest_free_block_size(), DIVOOM_LIGHT_MUSIC_ZSTD_MIN_LARGEST_KB, divoom_system_get_ram_free_size());
                pdivoom_light_music_handle->disp_type = DIVOOM_DISP_MUSIC_CENTER_MODE;
            }
            else
            {
                pdivoom_light_music_handle->pic_handle = divoom_manage_image_read(DIVOOM_MANAGE_IMAGE_LIGHT_MUSIC_MODE, divoom_eq_image_manage_get_image_id(&pdivoom_light_music_handle->clock_info,disp_type - DIVOOM_DISP_MUSIC_END_MODE ,&user_id, image_name), NULL, NULL);
            }
        }
        if(pdivoom_light_music_handle->pic_handle)
        {
            if(disp_type >= DIVOOM_DISP_MUSIC_END_MODE &&
                disp_type != DIVOOM_DISP_MUSIC_TMP_INDEX)
            {
                /* 自定义 EQ：保留 pic_handle/zstd，渲染时按需解当前帧到 disp_buf */
                pdivoom_light_music_handle->disp_width = pdivoom_light_music_handle->pic_handle->width;
                pdivoom_light_music_handle->user_tol_num = pdivoom_light_music_handle->pic_handle->num;
                pdivoom_light_music_handle->custom_eq_stream_level = -1;
                if(pdivoom_light_music_handle->pic_handle->image_type == DIVOOM_IMAGE_BUF_TYPE_ZSTD_ENCODE && pdivoom_light_music_handle->pic_handle->zstd_handle != NULL)
                {
                    divoom_zstd_manage_set_sync_read_mode(pdivoom_light_music_handle->pic_handle->zstd_handle, 1);
                    divoom_zstd_manage_idle_read(pdivoom_light_music_handle->pic_handle->zstd_handle);
                }
            }
            else
            {
                divoom_manage_image_format_gif_table(pdivoom_light_music_handle->pic_handle, NULL);
                pdivoom_light_music_handle->user_tol_num = pdivoom_light_music_handle->pic_handle->num;
            }
			//增加容错处理
			if(pdivoom_light_music_handle->user_tol_num == 0)
			{
				if(pdivoom_light_music_handle->pic_handle && pdivoom_light_music_handle->disp_type != DIVOOM_DISP_MUSIC_TMP_INDEX)
				{
                    divoom_http_download_image_free_image_handle(pdivoom_light_music_handle->pic_handle);
				}
                pdivoom_light_music_handle->pic_handle = NULL;
				pdivoom_light_music_handle->disp_type = DIVOOM_DISP_MUSIC_CENTER_MODE;
			}            
        }            
        else
        {
            pdivoom_light_music_handle->disp_type = DIVOOM_DISP_MUSIC_CENTER_MODE;
        }
    }
    divoom_light_app_set_cur_image_id(divoom_light_app_set_cur_image_id(0) + 1);
    divoom_light_app_set_cur_author_id(user_id, NULL);
}
/*********************************************************************
  Function:         divoom_light_music_init
  Description:     开始音乐模式
  Input:             disp_type    显示类型
                     fft_flag
                       top    顶点的颜色
                       down   底点的颜色
                       fft_flag
  Author:           divoom
**********************************************************************/

void divoom_light_music_init(void *index, uint8 fft_flag)
{
    Pdivoom_light_music_ctrl pdivoom_light_music_handle;
    divoom_disp_ctrl_music_info ptmp_info;
    uint8 disp_type;

	divoom_malloc_check_all_ram_in();
    pdivoom_light_music_handle = (Pdivoom_light_music_ctrl)DIVOOM_LIGHT_MUSIC_MALLOC(sizeof(divoom_light_music_ctrl));
    if(pdivoom_light_music_handle == NULL)
    {
        divoom_print("%s:%d: it fails to malloc!\n",__FILE__, __LINE__);
        return ; 
    }
    
    memset(pdivoom_light_music_handle, 0, sizeof(divoom_light_music_ctrl));
    pdivoom_light_music_handle->custom_eq_stream_level = -1;
    memcpy(&pdivoom_light_music_handle->clock_info, index, sizeof(divoom_clock_id_info));
    divoom_print("aj test!: %d\n", pdivoom_light_music_handle->clock_info.clock_id);
	disp_type = divoom_eq_image_manage_get_eq_mode(index);
    pdivoom_light_music_handle->pic_handle = NULL;
    pdivoom_light_music_handle->disp_type = disp_type;
    divoom_light_music_init_mode(pdivoom_light_music_handle, disp_type);
   
    
    pdivoom_light_music_handle->task_id = divoom_microtask_create_para(divoom_light_music_main, DIVOOM_LIGHT_MUSIC_REFRESH_TIME, pdivoom_light_music_handle);

    

    pdivoom_light_music_handle->play_flag = 1;
    pdivoom_light_music_handle->refresh_flag = 1;
    pdivoom_light_music_handle->fft_flag = fft_flag;
    pdivoom_light_music_handle->mix_mode = 0xff;
    memcpy(pdivoom_light_music_handle->back_disp_info, divoom_light_music_freq_info, sizeof(pdivoom_light_music_handle->back_disp_info));
    memcpy(&pdivoom_light_music_handle->top_rgb, &musice_up_color, sizeof(divoom_light_color_info));
    memcpy(&pdivoom_light_music_handle->down_rgb, &musice_down_color, sizeof(divoom_light_color_info));
    
    divoom_light_music_main(pdivoom_light_music_handle);
    
    divoom_microtask_resume(pdivoom_light_music_handle->task_id);
    divoom_light_music_global_insert_handle(pdivoom_light_music_handle);
    
}


uint8 divoom_light_music_update_mode(void *index)
{
    Pdivoom_disp_ctrl_music_info pmusic_info;
    divoom_disp_ctrl_music_info ptmp_info;
    Pdivoom_light_music_ctrl pdivoom_light_music_handle;
    
    
    uint8 disp_type;

    divoom_system_get_lock(pdivoom_light_music_gobal_handle->lock_handle);
    pdivoom_light_music_handle = divoom_light_music_global_get_handle(index);
    if(pdivoom_light_music_handle == NULL)
    {
        divoom_system_free_lock(pdivoom_light_music_gobal_handle->lock_handle);
        return 0;
    }
	disp_type = divoom_eq_image_manage_get_eq_mode(index);
    divoom_print("divoom_light_music_update_mode: %d\n", disp_type);
    
    if(pdivoom_light_music_handle)
    {
        if(pdivoom_light_music_handle->disp_type == disp_type && pdivoom_light_music_handle->disp_type < DIVOOM_DISP_MUSIC_END_MODE)
        {
            divoom_system_free_lock(pdivoom_light_music_gobal_handle->lock_handle);
            return disp_type;
        }
        memcpy(&pdivoom_light_music_handle->clock_info, index, sizeof(divoom_clock_id_info));
        pdivoom_light_music_handle->disp_type = disp_type;
	    divoom_light_music_init_mode(pdivoom_light_music_handle, disp_type);
        divoom_light_music_custom_eq_render_first_frame(pdivoom_light_music_handle);
        pdivoom_light_music_handle->refresh_flag = 1;
        divoom_system_free_lock(pdivoom_light_music_gobal_handle->lock_handle);
		divoom_microtask_pause(pdivoom_light_music_handle->task_id);
        divoom_microtask_resume(pdivoom_light_music_handle->task_id);   	
        divoom_microtask_update(pdivoom_light_music_handle->task_id, DIVOOM_LIGHT_MUSIC_REFRESH_TIME*2);      
    }
    else
    {
        divoom_system_free_lock(pdivoom_light_music_gobal_handle->lock_handle);
    }
    return disp_type;
}




/*********************************************************************
  Function:         divoom_light_music_close
  Description:     关闭
  Input:             NONE
  Author:           divoom
**********************************************************************/

void divoom_light_music_close(void *user_data)
{
    divoom_light_music_global_remove_handle(user_data);
}


/*********************************************************************
  Function:         divoom_light_music_close_inner
  Description:     关闭
  Input:             NONE
  Author:           divoom
**********************************************************************/

void divoom_light_music_close_inner(Pdivoom_light_music_ctrl handle)
{
    if(handle == NULL)
    {
        return;
    }
    
    divoom_microtask_pause(handle->task_id);
    divoom_microtask_destroy(handle->task_id);

//    pdivoom_light_music_handle = NULL;
    if(handle->pic_handle)
    {
        divoom_print("free handle:!\n");
        divoom_http_download_image_free_image_handle(handle->pic_handle);
        handle->pic_handle = NULL;
    }
    
    if(handle->prow_pic_ex_image_handle)
    {
        divoom_http_download_image_free_image_handle(handle->prow_pic_ex_image_handle);
        handle->prow_pic_ex_image_handle = NULL;
    }

    handle->custom_eq_stream_level = -1;
    
	//divoom_light_start_blink_close();
	DIVOOM_LIGHT_MUSIC_FREE(handle);
    

	divoom_malloc_check_all_ram_out(__func__);
}
#else
#include "divoom.h"
void divoom_light_music_init(void *index, uint8 fft_flag) { (void)index; (void)fft_flag; }
uint8 divoom_light_music_update_mode(void *index) { (void)index; return 0; }
void divoom_light_music_close(void *user_data) { (void)user_data; }
void divoom_light_music_play(uint8 flag) { (void)flag; }
void divoom_light_music_get_disp_buf(void *user_data, T_U8 *disp_buf, int *image_width) { (void)user_data; (void)disp_buf; if(image_width) *image_width = 0; }
void divoom_light_music_global_init(void) {}
#endif
