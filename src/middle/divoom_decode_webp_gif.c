/*
 * DIVM-WebP 动画解码模块。
 *
 * DIVM 的偏移表仍只描述“内嵌文件”，不能直接当作显示帧表；本模块逐个
 * 打开内嵌 WebP，并把其中所有动画帧按顺序展开。压缩数据始终从原 DIVM
 * 文件的对应区间按固定小块读取，不为单个 WebP 建立整文件缓存。
 */
#include "divoom.h"
#include "divoom_decode_webp_gif.h"
#include "divoom_libwebp_decode.h"
#include <limits.h>

#define DIVOOM_DECODE_WEBP_GIF_HANDLE_MAGIC      0x57475044U
#define DIVOOM_DECODE_WEBP_GIF_HEADER_SIZE       8U
#define DIVOOM_DECODE_WEBP_GIF_OFFSET_SIZE       4U
#define DIVOOM_DECODE_WEBP_GIF_MAX_SIDE          8192

typedef struct _divoom_decode_webp_gif_ctrl
{
    uint32 handle_magic;
    uint32 file_size;
    uint32 data_offset;
    uint32 *frame_offset;
    int32 *inner_frame_count;
    void *file_handle;
    char *file_name;
    void *inner_decoder;
    uint8 *bgra_frame_buf;
    int32 outer_frame_num;
    int32 outer_frame_id;
    int32 inner_frame_num;
    int32 inner_frame_id;
    int32 total_frame_num;
    int32 frame_speed;
    int32 width;
    int32 height;
} divoom_decode_webp_gif_ctrl, *Pdivoom_decode_webp_gif_ctrl;

static int divoom_decode_webp_gif_get_inner_region(
    Pdivoom_decode_webp_gif_ctrl phandle, int32 outer_frame_id,
    uint32 *file_offset, uint32 *inner_size)
{
    uint32 begin;
    uint32 end;

    if(phandle == NULL || file_offset == NULL || inner_size == NULL ||
        outer_frame_id < 0 || outer_frame_id >= phandle->outer_frame_num)
    {
        return 0;
    }
    begin = phandle->frame_offset[outer_frame_id];
    end = phandle->frame_offset[outer_frame_id + 1];
    if(end <= begin || end > phandle->file_size - phandle->data_offset)
    {
        return 0;
    }
    *inner_size = end - begin;
    *file_offset = phandle->data_offset + begin;
    return *inner_size > 0U;
}

static void divoom_decode_webp_gif_close_inner(
    Pdivoom_decode_webp_gif_ctrl phandle)
{
    if(phandle == NULL)
    {
        return;
    }
    if(phandle->inner_decoder != NULL)
    {
        divoom_libwebp_decode_exit(phandle->inner_decoder);
        phandle->inner_decoder = NULL;
    }
    phandle->inner_frame_num = 0;
    phandle->inner_frame_id = 0;
}

/* 扫描阶段只短暂打开单个 WebP，用于汇总真实显示帧数并校验统一画布。 */
static int divoom_decode_webp_gif_scan_frames(
    Pdivoom_decode_webp_gif_ctrl phandle)
{
    int32 outer_frame_id;
    int32 total_frame_num = 0;

    for(outer_frame_id = 0; outer_frame_id < phandle->outer_frame_num;
        outer_frame_id++)
    {
        uint32 file_offset = 0;
        uint32 inner_size = 0;
        int width;
        int height;
        int frame_num;
        void *decoder;

        if(!divoom_decode_webp_gif_get_inner_region(phandle, outer_frame_id,
            &file_offset, &inner_size))
        {
            return 0;
        }
        decoder = divoom_libwebp_decode_init_file_region(phandle->file_name,
            file_offset, inner_size);
        if(decoder == NULL)
        {
            return 0;
        }
        frame_num = divoom_libwebp_decode_get_frame_info(decoder, &width,
            &height);
        if(frame_num <= 0 || width <= 0 || height <= 0 ||
            width > DIVOOM_DECODE_WEBP_GIF_MAX_SIDE ||
            height > DIVOOM_DECODE_WEBP_GIF_MAX_SIDE)
        {
            divoom_libwebp_decode_exit(decoder);
            return 0;
        }
        if(outer_frame_id == 0)
        {
            phandle->width = width;
            phandle->height = height;
        }
        else if(phandle->width != width || phandle->height != height)
        {
            divoom_libwebp_decode_exit(decoder);
            return 0;
        }
        if(frame_num > INT_MAX - total_frame_num)
        {
            divoom_libwebp_decode_exit(decoder);
            return 0;
        }
        phandle->inner_frame_count[outer_frame_id] = frame_num;
        total_frame_num += frame_num;
        divoom_libwebp_decode_exit(decoder);
    }
    phandle->total_frame_num = total_frame_num;
    return total_frame_num > 0;
}

static int divoom_decode_webp_gif_open_inner(
    Pdivoom_decode_webp_gif_ctrl phandle)
{
    uint32 file_offset = 0;
    uint32 inner_size = 0;
    int width;
    int height;
    int frame_num;

    if(!divoom_decode_webp_gif_get_inner_region(phandle,
        phandle->outer_frame_id, &file_offset, &inner_size))
    {
        return 0;
    }
    phandle->inner_decoder = divoom_libwebp_decode_init_file_region(
        phandle->file_name, file_offset, inner_size);
    if(phandle->inner_decoder == NULL)
    {
        return 0;
    }
    frame_num = divoom_libwebp_decode_get_frame_info(phandle->inner_decoder,
        &width, &height);
    if(frame_num != phandle->inner_frame_count[phandle->outer_frame_id] ||
        phandle->width != width || phandle->height != height)
    {
        divoom_decode_webp_gif_close_inner(phandle);
        return 0;
    }
    phandle->inner_frame_num = frame_num;
    phandle->inner_frame_id = 0;
    return 1;
}

/* WebP timestamp 是累计毫秒；外层 frame_speed 仅作为静态帧或零时长帧兜底。 */
static int divoom_decode_webp_gif_prepare_frame(
    Pdivoom_decode_webp_gif_ctrl phandle, uint8 **frame_buf, int *speed)
{
    int frame_time;
    uint32 buf_size;

    if(phandle->inner_decoder == NULL &&
        !divoom_decode_webp_gif_open_inner(phandle))
    {
        return 0;
    }
    buf_size = (uint32)phandle->width * (uint32)phandle->height * 4U;
    if(phandle->bgra_frame_buf == NULL)
    {
        phandle->bgra_frame_buf = divoom_system_malloc_no_zero(buf_size);
    }
    if(phandle->bgra_frame_buf == NULL ||
        divoom_libwebp_decode_get_frame_buffer(phandle->inner_decoder,
            &frame_time, NULL, phandle->bgra_frame_buf) == NULL)
    {
        return 0;
    }
    if(frame_time <= 0)
    {
        frame_time = phandle->frame_speed > 0 ? phandle->frame_speed : 100;
    }
    phandle->inner_frame_id++;
    *frame_buf = phandle->bgra_frame_buf;
    if(speed != NULL)
    {
        *speed = frame_time;
    }
    return 1;
}

/* 完成当前帧消费后再释放 decoder，保证其内部 BGRA 指针在拷贝期间有效。 */
static int divoom_decode_webp_gif_advance_outer_frame(
    Pdivoom_decode_webp_gif_ctrl phandle)
{
    int end_flag = 0;

    divoom_decode_webp_gif_close_inner(phandle);
    phandle->outer_frame_id++;
    if(phandle->outer_frame_id >= phandle->outer_frame_num)
    {
        phandle->outer_frame_id = 0;
        end_flag = 1;
    }
    return end_flag;
}

/* 完成当前帧消费后再释放 decoder，保证其内部 BGRA 指针在拷贝期间有效。 */
static int divoom_decode_webp_gif_finish_frame(
    Pdivoom_decode_webp_gif_ctrl phandle)
{
    if(phandle->inner_frame_id < phandle->inner_frame_num)
    {
        return 0;
    }
    return divoom_decode_webp_gif_advance_outer_frame(phandle);
}

static void divoom_decode_webp_gif_bgra_to_rgb565a8(const uint8 *bgra,
    uint8 *rgb565a8, uint32 px_count)
{
    uint8 *alpha = rgb565a8 + px_count * 2U;
    uint32 i;

    for(i = 0; i < px_count; i++)
    {
        uint8 a = bgra[i * 4U + 3U];
        uint32 r = a == 0U ? 0U : ((uint32)bgra[i * 4U + 2U] * 255U +
            (uint32)a / 2U) / (uint32)a;
        uint32 g = a == 0U ? 0U : ((uint32)bgra[i * 4U + 1U] * 255U +
            (uint32)a / 2U) / (uint32)a;
        uint32 b = a == 0U ? 0U : ((uint32)bgra[i * 4U] * 255U +
            (uint32)a / 2U) / (uint32)a;
        uint16 color;

        if(r > 255U)
        {
            r = 255U;
        }
        if(g > 255U)
        {
            g = 255U;
        }
        if(b > 255U)
        {
            b = 255U;
        }
        color = (uint16)((uint16)(r >> 3) << 11) |
            (uint16)((uint16)(g >> 2) << 5) | (uint16)(b >> 3);

        rgb565a8[i * 2U] = (uint8)(color & 0xFFU);
        rgb565a8[i * 2U + 1U] = (uint8)(color >> 8);
        alpha[i] = a;
    }
}

void *divoom_decode_webp_gif_init(char *file_name, int *image_w,
    int *image_h, int *image_num)
{
    uint8 header[DIVOOM_DECODE_WEBP_GIF_HEADER_SIZE];
    long file_size;
    uint32 data_size;
    int32 i;
    Pdivoom_decode_webp_gif_ctrl phandle;

    if(file_name == NULL || image_w == NULL || image_h == NULL)
    {
        return NULL;
    }
    phandle = divoom_system_malloc(sizeof(divoom_decode_webp_gif_ctrl));
    if(phandle == NULL)
    {
        return NULL;
    }
    phandle->handle_magic = DIVOOM_DECODE_WEBP_GIF_HANDLE_MAGIC;
    phandle->file_name = divoom_app_com_copy_string(file_name, 0);
    phandle->file_handle = divoom_port_file_open(file_name, "rb");
    if(phandle->file_name == NULL || phandle->file_handle == NULL ||
        divoom_port_file_read(header, 1, sizeof(header), phandle->file_handle) !=
            sizeof(header) || memcmp(header, "DIVM", 4) != 0)
    {
        divoom_decode_webp_gif_exit(phandle);
        return NULL;
    }
    phandle->outer_frame_num = (int32)divoom_com_unpack_uint16(header + 4);
    phandle->frame_speed = (int32)divoom_com_unpack_uint16(header + 6);
    if(phandle->outer_frame_num <= 0 ||
        (uint32)phandle->outer_frame_num >
            (UINT_MAX - DIVOOM_DECODE_WEBP_GIF_HEADER_SIZE) /
                DIVOOM_DECODE_WEBP_GIF_OFFSET_SIZE)
    {
        divoom_decode_webp_gif_exit(phandle);
        return NULL;
    }
    phandle->data_offset = DIVOOM_DECODE_WEBP_GIF_HEADER_SIZE +
        (uint32)phandle->outer_frame_num * DIVOOM_DECODE_WEBP_GIF_OFFSET_SIZE;
    phandle->frame_offset = divoom_system_malloc(
        sizeof(uint32) * (uint32)(phandle->outer_frame_num + 1));
    phandle->inner_frame_count = divoom_system_malloc(
        sizeof(int32) * (uint32)phandle->outer_frame_num);
    if(phandle->frame_offset == NULL || phandle->inner_frame_count == NULL ||
        divoom_port_file_read(phandle->frame_offset, sizeof(uint32),
            (size_t)phandle->outer_frame_num, phandle->file_handle) !=
                (size_t)phandle->outer_frame_num ||
        divoom_port_file_seek(phandle->file_handle, 0, SEEK_END) != 0)
    {
        divoom_decode_webp_gif_exit(phandle);
        return NULL;
    }
    file_size = divoom_port_file_tell(phandle->file_handle);
    if(file_size <= 0 || (uint32)file_size <= phandle->data_offset)
    {
        divoom_decode_webp_gif_exit(phandle);
        return NULL;
    }
    phandle->file_size = (uint32)file_size;
    data_size = phandle->file_size - phandle->data_offset;
    phandle->frame_offset[phandle->outer_frame_num] = data_size;
    for(i = 0; i < phandle->outer_frame_num; i++)
    {
        if(phandle->frame_offset[i] >= phandle->frame_offset[i + 1] ||
            phandle->frame_offset[i + 1] > data_size)
        {
            divoom_decode_webp_gif_exit(phandle);
            return NULL;
        }
    }
    if(!divoom_decode_webp_gif_scan_frames(phandle))
    {
        divoom_decode_webp_gif_exit(phandle);
        return NULL;
    }

    *image_w = phandle->width;
    *image_h = phandle->height;
    if(image_num != NULL)
    {
        *image_num = phandle->total_frame_num;
    }
    return phandle;
}

int divoom_decode_webp_gif_is_handle(void *handle)
{
    Pdivoom_decode_webp_gif_ctrl phandle =
        (Pdivoom_decode_webp_gif_ctrl)handle;

    return phandle != NULL &&
        phandle->handle_magic == DIVOOM_DECODE_WEBP_GIF_HANDLE_MAGIC;
}

int divoom_decode_webp_gif_reset(void *handle)
{
    Pdivoom_decode_webp_gif_ctrl phandle =
        (Pdivoom_decode_webp_gif_ctrl)handle;

    if(!divoom_decode_webp_gif_is_handle(handle))
    {
        return 0;
    }

    /*
     * 仅复位播放游标；保留 DIVM 文件、偏移表和扫描得到的帧数统计，
     * 避免局部动画循环时重新扫描所有内嵌 WebP。
     */
    divoom_decode_webp_gif_close_inner(phandle);
    phandle->outer_frame_id = 0;
    return 1;
}

void divoom_decode_webp_gif_pass_some_frame(void *handle, int frame_num)
{
    Pdivoom_decode_webp_gif_ctrl phandle =
        (Pdivoom_decode_webp_gif_ctrl)handle;
    int32 skip_frame_num;

    if(!divoom_decode_webp_gif_is_handle(handle) || frame_num <= 0 ||
        phandle->total_frame_num <= 0)
    {
        return;
    }
    skip_frame_num = frame_num % phandle->total_frame_num;
    while(skip_frame_num > 0)
    {
        int32 decoded_frame_num = phandle->inner_decoder != NULL ?
            phandle->inner_frame_id : 0;
        int32 remain_frame_num =
            phandle->inner_frame_count[phandle->outer_frame_id] -
                decoded_frame_num;

        if(remain_frame_num <= 0)
        {
            divoom_decode_webp_gif_advance_outer_frame(phandle);
            continue;
        }
        if(skip_frame_num >= remain_frame_num)
        {
            /* 整个 WebP 的剩余帧都被跨过时，只移动文件索引，不解码像素。 */
            skip_frame_num -= remain_frame_num;
            divoom_decode_webp_gif_advance_outer_frame(phandle);
            continue;
        }

        /*
         * 目标落在当前动画 WebP 内部时，必须推进前置帧以恢复 blend/dispose
         * 合成状态；这里只解码最终落点所在 WebP 中不可省略的部分。
         */
        while(skip_frame_num > 0)
        {
            uint8 *frame_buf = NULL;

            if(!divoom_decode_webp_gif_prepare_frame(phandle, &frame_buf,
                NULL))
            {
                return;
            }
            skip_frame_num--;
        }
    }
}

uint8 *divoom_decode_webp_gif_get_malloc_bgra_data(void *handle, int *speed,
    int *end_flag)
{
    Pdivoom_decode_webp_gif_ctrl phandle =
        (Pdivoom_decode_webp_gif_ctrl)handle;
    uint8 *frame_buf = NULL;
    uint8 *out_buf;
    uint32 buf_size;

    if(!divoom_decode_webp_gif_is_handle(handle))
    {
        return NULL;
    }
    buf_size = (uint32)phandle->width * (uint32)phandle->height * 4U;
    out_buf = divoom_system_malloc_no_zero(buf_size);
    if(out_buf == NULL ||
        !divoom_decode_webp_gif_prepare_frame(phandle, &frame_buf, speed))
    {
        if(out_buf != NULL)
        {
            divoom_system_free(out_buf);
        }
        return NULL;
    }
    memcpy(out_buf, frame_buf, buf_size);
    if(end_flag != NULL)
    {
        *end_flag = divoom_decode_webp_gif_finish_frame(phandle);
    }
    else
    {
        divoom_decode_webp_gif_finish_frame(phandle);
    }
    return out_buf;
}

uint8 *divoom_decode_webp_gif_get_bgra_data(void *handle, int *speed,
    int *end_flag)
{
    Pdivoom_decode_webp_gif_ctrl phandle =
        (Pdivoom_decode_webp_gif_ctrl)handle;
    uint8 *frame_buf = NULL;
    uint32 buf_size;

    if(!divoom_decode_webp_gif_is_handle(handle))
    {
        return NULL;
    }
    buf_size = (uint32)phandle->width * (uint32)phandle->height * 4U;
    if(phandle->bgra_frame_buf == NULL)
    {
        phandle->bgra_frame_buf = divoom_system_malloc_no_zero(buf_size);
    }
    if(phandle->bgra_frame_buf == NULL ||
        !divoom_decode_webp_gif_prepare_frame(phandle, &frame_buf, speed))
    {
        return NULL;
    }
    memcpy(phandle->bgra_frame_buf, frame_buf, buf_size);
    if(end_flag != NULL)
    {
        *end_flag = divoom_decode_webp_gif_finish_frame(phandle);
    }
    else
    {
        divoom_decode_webp_gif_finish_frame(phandle);
    }
    return phandle->bgra_frame_buf;
}

uint8 *divoom_decode_webp_gif_get_frame_rgb565a8_buffer(void *handle,
    int *speed, int *end_flag, uint8 *sys_buf)
{
    Pdivoom_decode_webp_gif_ctrl phandle =
        (Pdivoom_decode_webp_gif_ctrl)handle;
    uint8 *frame_buf = NULL;
    uint8 *out_buf = sys_buf;
    uint32 px_count;

    if(!divoom_decode_webp_gif_is_handle(handle))
    {
        return NULL;
    }
    px_count = (uint32)phandle->width * (uint32)phandle->height;
    if(out_buf == NULL)
    {
        out_buf = divoom_system_malloc_no_zero(px_count * 3U);
    }
    if(out_buf == NULL ||
        !divoom_decode_webp_gif_prepare_frame(phandle, &frame_buf, speed))
    {
        if(sys_buf == NULL && out_buf != NULL)
        {
            divoom_system_free(out_buf);
        }
        return NULL;
    }
    /* LVGL RGB565A8 使用 RGB565 平面 + Alpha 平面，不是像素交错布局。 */
    divoom_decode_webp_gif_bgra_to_rgb565a8(frame_buf, out_buf, px_count);
    if(end_flag != NULL)
    {
        *end_flag = divoom_decode_webp_gif_finish_frame(phandle);
    }
    else
    {
        divoom_decode_webp_gif_finish_frame(phandle);
    }
    return out_buf;
}

void divoom_decode_webp_gif_exit(void *handle)
{
    Pdivoom_decode_webp_gif_ctrl phandle =
        (Pdivoom_decode_webp_gif_ctrl)handle;

    if(!divoom_decode_webp_gif_is_handle(handle))
    {
        return;
    }
    divoom_decode_webp_gif_close_inner(phandle);
    if(phandle->file_handle != NULL)
    {
        divoom_port_file_close(phandle->file_handle);
    }
    if(phandle->frame_offset != NULL)
    {
        divoom_system_free(phandle->frame_offset);
    }
    if(phandle->inner_frame_count != NULL)
    {
        divoom_system_free(phandle->inner_frame_count);
    }
    if(phandle->bgra_frame_buf != NULL)
    {
        divoom_system_free(phandle->bgra_frame_buf);
    }
    if(phandle->file_name != NULL)
    {
        divoom_system_free(phandle->file_name);
    }
    phandle->handle_magic = 0;
    divoom_system_free(phandle);
}
