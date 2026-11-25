#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <webp/decode.h>
#include <webp/encode.h>
#include "webp/demux.h"
#include "webp/mux.h"
#include "divoom.h"
#include "divoom_pixel_format.h"
#include "divoom_aw_pic_decode.h"
#include "divoom_rk_mpp.h"

uint8 *divoom_libwebp_decode_get_frame_buffer(void *handle, int *frame_time,
    int *end_flag, uint8 *sys_buf);
uint8 *divoom_libwebp_decode_get_frame_rgb565a8_buffer(void *handle,
    int *frame_time, int *end_flag, uint8 *sys_buf);
uint8 *divoom_libwebp_decode_get_frame_rgb565_buffer(void *handle, int *frame_time, int *end_flag, uint8 *sys_buf);
void divoom_libwebp_decode_exit(void *handle);

/* 动画文件每次最多读取 16 KiB，避免压缩文件整体进入业务内存。 */
#define DIVOOM_LIBWEBP_FILE_READ_CHUNK_SIZE (16U * 1024U)
#define DIVOOM_LIBWEBP_ANMF_HEADER_SIZE 16U
#define DIVOOM_LIBWEBP_CHUNK_HEADER_SIZE 8U
#define DIVOOM_LIBWEBP_RIFF_HEADER_SIZE 12U
#define DIVOOM_LIBWEBP_NUM_CHANNELS 4U
#define DIVOOM_LIBWEBP_ANIMATION_FLAG 0x02U
#define DIVOOM_LIBWEBP_ALPHA_FLAG 0x10U
#define DIVOOM_LIBWEBP_VP8X_PAYLOAD_SIZE 10U
#define DIVOOM_LIBWEBP_STILL_HEADER_SIZE 30U
#define DIVOOM_LIBWEBP_BACKEND_LEGACY 0U
#define DIVOOM_LIBWEBP_BACKEND_FILE_ANIMATION 1U
#define DIVOOM_LIBWEBP_BACKEND_FILE_STATIC 2U
#define DIVOOM_LIBWEBP_STATUS_UNKNOWN ((VP8StatusCode)-1)

#ifdef WORDS_BIGENDIAN
#define DIVOOM_LIBWEBP_CHANNEL_SHIFT(i) (24 - (i) * 8)
#else
#define DIVOOM_LIBWEBP_CHANNEL_SHIFT(i) ((i) * 8)
#endif

typedef enum
{
    DIVOOM_LIBWEBP_FILE_PROBE_CORRUPT = -2,
    DIVOOM_LIBWEBP_FILE_PROBE_ERROR = -1,
    DIVOOM_LIBWEBP_FILE_PROBE_STATIC = 0,
    DIVOOM_LIBWEBP_FILE_PROBE_ANIMATION = 1
} divoom_libwebp_file_probe_result;

/* 单帧只保存 ANMF 元数据和文件区间，不保存压缩数据。 */
typedef struct
{
    int32 frame_num;
    int32 x_offset;
    int32 y_offset;
    int32 width;
    int32 height;
    int32 duration;
    uint8 has_alpha;
    WebPMuxAnimDispose dispose_method;
    WebPMuxAnimBlend blend_method;
    uint32 stream_offset;
    uint32 stream_size;
    uint32 next_chunk_offset;
} divoom_libwebp_file_frame_info;

/* 每个动画句柄独立持有文件游标和两张合成画布，不共享可变状态。 */
typedef struct
{
    void *file_handle;
    uint32 file_base_offset;
    uint32 file_size;
    uint32 riff_end;
    uint32 first_frame_offset;
    uint32 next_chunk_offset;
    uint32 canvas_size;
    uint32 max_read_size;
    VP8StatusCode last_status;
    int32 width;
    int32 height;
    int32 frame_num;
    int32 next_frame;
    int32 prev_frame_timestamp;
    uint8 prev_frame_valid;
    uint8 prev_frame_was_keyframe;
    uint8 *read_buf;
    uint8 *curr_frame;
    uint8 *prev_frame_disposed;
    divoom_libwebp_file_frame_info prev_frame;
} divoom_libwebp_file_anim_ctrl, *Pdivoom_libwebp_file_anim_ctrl;

/* 静态 WebP 同样按固定小块增量解码，只保留一张输出画布。 */
typedef struct
{
    void *file_handle;
    uint32 file_base_offset;
    uint32 file_size;
    uint32 canvas_size;
    uint32 max_read_size;
    VP8StatusCode last_status;
    int32 width;
    int32 height;
    uint8 decoded;
    uint8 *read_buf;
    uint8 *curr_frame;
} divoom_libwebp_file_static_ctrl, *Pdivoom_libwebp_file_static_ctrl;

static uint32 divoom_libwebp_decode_file_read_le24(const uint8 *data)
{
    return (uint32)data[0] | ((uint32)data[1] << 8) |
        ((uint32)data[2] << 16);
}

static uint32 divoom_libwebp_decode_file_read_le32(const uint8 *data)
{
    return (uint32)data[0] | ((uint32)data[1] << 8) |
        ((uint32)data[2] << 16) | ((uint32)data[3] << 24);
}

static void divoom_libwebp_decode_file_write_le24(uint8 *data, uint32 value)
{
    data[0] = (uint8)(value & 0xFFU);
    data[1] = (uint8)((value >> 8) & 0xFFU);
    data[2] = (uint8)((value >> 16) & 0xFFU);
}

static void divoom_libwebp_decode_file_write_le32(uint8 *data, uint32 value)
{
    data[0] = (uint8)(value & 0xFFU);
    data[1] = (uint8)((value >> 8) & 0xFFU);
    data[2] = (uint8)((value >> 16) & 0xFFU);
    data[3] = (uint8)((value >> 24) & 0xFFU);
}

static int divoom_libwebp_decode_file_tag_equal(const uint8 *data,
    const char *tag)
{
    return memcmp(data, tag, 4) == 0;
}

/* 文件层只允许固定小块读取；短读视为文件被截断或底层 I/O 失败。 */
static int divoom_libwebp_decode_file_read_exact(void *file_handle,
    uint8 *buf, uint32 size, uint32 *max_read_size)
{
    uint32 read_total = 0;

    if(file_handle == NULL || buf == NULL ||
        size > DIVOOM_LIBWEBP_FILE_READ_CHUNK_SIZE)
    {
        return 0;
    }
    if(max_read_size != NULL && *max_read_size < size)
    {
        *max_read_size = size;
    }
    while(read_total < size)
    {
        size_t read_size = divoom_port_file_read(buf + read_total, 1,
            (size_t)(size - read_total), file_handle);

        if(read_size == 0)
        {
            return 0;
        }
        read_total += (uint32)read_size;
    }
    return 1;
}

static int divoom_libwebp_decode_file_read_at(
    Pdivoom_libwebp_file_anim_ctrl phandle, uint32 offset, uint8 *buf,
    uint32 size)
{
    if(phandle == NULL || offset > phandle->file_size ||
        size > phandle->file_size - offset)
    {
        return 0;
    }
    if(phandle->file_base_offset > U32_MAX - offset ||
        divoom_port_file_seek(phandle->file_handle,
        (long)(phandle->file_base_offset + offset),
        DIVOOM_PORT_SEEK_SET) != 0)
    {
        return 0;
    }
    return divoom_libwebp_decode_file_read_exact(phandle->file_handle, buf,
        size, &phandle->max_read_size);
}

/* 校验一个 RIFF chunk 的边界，并返回未补齐长度和下一个 chunk 位置。 */
static int divoom_libwebp_decode_file_get_chunk_bounds(
    Pdivoom_libwebp_file_anim_ctrl phandle, uint32 chunk_offset,
    uint32 limit, uint8 *header, uint32 *payload_size,
    uint32 *next_chunk_offset)
{
    uint32 size;
    uint32 padded_size;

    if(chunk_offset > limit ||
        DIVOOM_LIBWEBP_CHUNK_HEADER_SIZE > limit - chunk_offset)
    {
        return 0;
    }
    if(!divoom_libwebp_decode_file_read_at(phandle, chunk_offset, header,
        DIVOOM_LIBWEBP_CHUNK_HEADER_SIZE))
    {
        return 0;
    }
    size = divoom_libwebp_decode_file_read_le32(header + 4);
    padded_size = size + (size & 1U);
    if(padded_size < size || padded_size >
        limit - chunk_offset - DIVOOM_LIBWEBP_CHUNK_HEADER_SIZE)
    {
        return 0;
    }
    *payload_size = size;
    *next_chunk_offset = chunk_offset + DIVOOM_LIBWEBP_CHUNK_HEADER_SIZE +
        padded_size;
    return 1;
}

/* 解析一个 ANMF，定位内部 ALPH/VP8 数据；压缩内容留在文件中。 */
static int divoom_libwebp_decode_file_parse_frame(
    Pdivoom_libwebp_file_anim_ctrl phandle, uint32 chunk_offset,
    int32 frame_num, divoom_libwebp_file_frame_info *frame)
{
    uint8 chunk_header[DIVOOM_LIBWEBP_CHUNK_HEADER_SIZE];
    uint8 frame_header[DIVOOM_LIBWEBP_ANMF_HEADER_SIZE];
    uint8 feature_buf[64];
    uint32 payload_size;
    uint32 next_chunk_offset;
    uint32 inner_offset;
    uint32 inner_end;
    uint32 image_offset = 0;
    uint32 image_size = 0;
    uint8 alpha_seen = 0;
    uint8 image_seen = 0;
    WebPBitstreamFeatures features;
    VP8StatusCode feature_status;

    if(frame == NULL || !divoom_libwebp_decode_file_get_chunk_bounds(phandle,
        chunk_offset, phandle->riff_end, chunk_header, &payload_size,
        &next_chunk_offset) ||
        !divoom_libwebp_decode_file_tag_equal(chunk_header, "ANMF") ||
        payload_size < DIVOOM_LIBWEBP_ANMF_HEADER_SIZE +
            DIVOOM_LIBWEBP_CHUNK_HEADER_SIZE)
    {
        return 0;
    }
    if(!divoom_libwebp_decode_file_read_at(phandle,
        chunk_offset + DIVOOM_LIBWEBP_CHUNK_HEADER_SIZE, frame_header,
        DIVOOM_LIBWEBP_ANMF_HEADER_SIZE))
    {
        return 0;
    }

    memset(frame, 0, sizeof(*frame));
    frame->frame_num = frame_num;
    frame->x_offset = (int32)(2U *
        divoom_libwebp_decode_file_read_le24(frame_header));
    frame->y_offset = (int32)(2U *
        divoom_libwebp_decode_file_read_le24(frame_header + 3));
    frame->width = (int32)(1U +
        divoom_libwebp_decode_file_read_le24(frame_header + 6));
    frame->height = (int32)(1U +
        divoom_libwebp_decode_file_read_le24(frame_header + 9));
    frame->duration = (int32)
        divoom_libwebp_decode_file_read_le24(frame_header + 12);
    frame->dispose_method = (frame_header[15] & 1U) ?
        WEBP_MUX_DISPOSE_BACKGROUND : WEBP_MUX_DISPOSE_NONE;
    frame->blend_method = (frame_header[15] & 2U) ?
        WEBP_MUX_NO_BLEND : WEBP_MUX_BLEND;
    frame->next_chunk_offset = next_chunk_offset;

    if(frame->width <= 0 || frame->height <= 0 || frame->x_offset < 0 ||
        frame->y_offset < 0 || frame->x_offset > phandle->width ||
        frame->y_offset > phandle->height ||
        frame->width > phandle->width - frame->x_offset ||
        frame->height > phandle->height - frame->y_offset)
    {
        return 0;
    }

    inner_offset = chunk_offset + DIVOOM_LIBWEBP_CHUNK_HEADER_SIZE +
        DIVOOM_LIBWEBP_ANMF_HEADER_SIZE;
    inner_end = chunk_offset + DIVOOM_LIBWEBP_CHUNK_HEADER_SIZE + payload_size;
    while(inner_offset < inner_end)
    {
        uint32 inner_payload_size;
        uint32 next_inner_offset;

        if(!divoom_libwebp_decode_file_get_chunk_bounds(phandle, inner_offset,
            inner_end, chunk_header, &inner_payload_size, &next_inner_offset))
        {
            return 0;
        }
        if(divoom_libwebp_decode_file_tag_equal(chunk_header, "ALPH"))
        {
            if(alpha_seen || image_seen)
            {
                return 0;
            }
            alpha_seen = 1;
            frame->has_alpha = 1;
            frame->stream_offset = inner_offset;
        }
        else if(divoom_libwebp_decode_file_tag_equal(chunk_header, "VP8 ") ||
            divoom_libwebp_decode_file_tag_equal(chunk_header, "VP8L"))
        {
            uint32 feature_size;

            if(image_seen || (alpha_seen &&
                divoom_libwebp_decode_file_tag_equal(chunk_header, "VP8L")))
            {
                return 0;
            }
            image_seen = 1;
            image_offset = inner_offset;
            image_size = next_inner_offset - inner_offset;
            if(frame->stream_offset == 0)
            {
                frame->stream_offset = image_offset;
            }
            frame->stream_size = image_offset + image_size -
                frame->stream_offset;

            /* ANMF 图像数据后不允许再夹带其他内部 chunk。 */
            if(next_inner_offset != inner_end)
            {
                return 0;
            }

            feature_size = image_size < sizeof(feature_buf) ? image_size :
                (uint32)sizeof(feature_buf);
            if(!divoom_libwebp_decode_file_read_at(phandle, image_offset,
                feature_buf, feature_size))
            {
                return 0;
            }
            memset(&features, 0, sizeof(features));
            feature_status = WebPGetFeatures(feature_buf, feature_size,
                &features);
            if(feature_status != VP8_STATUS_OK ||
                features.width != frame->width ||
                features.height != frame->height)
            {
                return 0;
            }
            if(features.has_alpha)
            {
                frame->has_alpha = 1;
            }
            break;
        }
        else
        {
            return 0;
        }
        inner_offset = next_inner_offset;
    }

    return image_seen && frame->stream_size > 0;
}

/* 初始化阶段只扫描 chunk 头，统计帧数并验证动画结构。 */
static int divoom_libwebp_decode_file_scan_animation(
    Pdivoom_libwebp_file_anim_ctrl phandle)
{
    uint8 chunk_header[DIVOOM_LIBWEBP_CHUNK_HEADER_SIZE];
    uint32 chunk_offset = DIVOOM_LIBWEBP_RIFF_HEADER_SIZE;
    uint32 payload_size;
    uint32 next_chunk_offset;
    int32 anim_chunk_count = 0;
    int32 vp8x_chunk_count = 0;
    int32 frame_count = 0;

    while(chunk_offset < phandle->riff_end)
    {
        if(!divoom_libwebp_decode_file_get_chunk_bounds(phandle, chunk_offset,
            phandle->riff_end, chunk_header, &payload_size,
            &next_chunk_offset))
        {
            return 0;
        }
        if(divoom_libwebp_decode_file_tag_equal(chunk_header, "VP8X"))
        {
            vp8x_chunk_count++;
            if(vp8x_chunk_count != 1 ||
                chunk_offset != DIVOOM_LIBWEBP_RIFF_HEADER_SIZE)
            {
                return 0;
            }
        }
        else if(divoom_libwebp_decode_file_tag_equal(chunk_header, "ANIM"))
        {
            if(payload_size != 6U)
            {
                return 0;
            }
            anim_chunk_count++;
        }
        else if(divoom_libwebp_decode_file_tag_equal(chunk_header, "ANMF"))
        {
            divoom_libwebp_file_frame_info frame;

            if(anim_chunk_count == 0 ||
                !divoom_libwebp_decode_file_parse_frame(phandle, chunk_offset,
                    frame_count + 1, &frame))
            {
                return 0;
            }
            if(frame_count == 0)
            {
                phandle->first_frame_offset = chunk_offset;
            }
            frame_count++;
        }
        else if(divoom_libwebp_decode_file_tag_equal(chunk_header, "ALPH") ||
            divoom_libwebp_decode_file_tag_equal(chunk_header, "VP8 ") ||
            divoom_libwebp_decode_file_tag_equal(chunk_header, "VP8L"))
        {
            return 0;
        }
        chunk_offset = next_chunk_offset;
    }

    if(chunk_offset != phandle->riff_end || vp8x_chunk_count != 1 ||
        anim_chunk_count != 1 || frame_count <= 0)
    {
        return 0;
    }
    phandle->frame_num = frame_count;
    phandle->next_frame = 1;
    phandle->next_chunk_offset = phandle->first_frame_offset;
    return 1;
}

static void divoom_libwebp_decode_file_anim_delete(
    Pdivoom_libwebp_file_anim_ctrl phandle)
{
    if(phandle == NULL)
    {
        return;
    }
    if(phandle->file_handle != NULL)
    {
        divoom_port_file_close(phandle->file_handle);
    }
    if(phandle->read_buf != NULL)
    {
        divoom_system_free(phandle->read_buf);
    }
    if(phandle->curr_frame != NULL)
    {
        divoom_system_free(phandle->curr_frame);
    }
    if(phandle->prev_frame_disposed != NULL)
    {
        divoom_system_free(phandle->prev_frame_disposed);
    }
    divoom_system_free(phandle);
}

/*
 * 先探测 VP8X 动画标志；静态图返回 STATIC，让原有整文件路径继续处理。
 * 动画图只保留打开的文件句柄、固定读取块和两张合成画布。
 */
static Pdivoom_libwebp_file_anim_ctrl
divoom_libwebp_decode_file_anim_create_region(const char *file_name,
    uint32 file_offset, uint32 region_size,
    divoom_libwebp_file_probe_result *probe_result)
{
    void *file_handle;
    long whole_file_size;
    uint8 header[30];
    uint32 riff_size;
    uint32 vp8x_size;
    uint32 width;
    uint32 height;
    uint64_t canvas_size;
    Pdivoom_libwebp_file_anim_ctrl phandle;

    *probe_result = DIVOOM_LIBWEBP_FILE_PROBE_ERROR;
    file_handle = divoom_port_file_open(file_name, "rb");
    if(file_handle == NULL)
    {
        return NULL;
    }
    if(divoom_port_file_seek(file_handle, 0, DIVOOM_PORT_SEEK_END) != 0)
    {
        divoom_port_file_close(file_handle);
        return NULL;
    }
    whole_file_size = divoom_port_file_tell(file_handle);
    if(whole_file_size <= 0 || file_offset > (uint32)whole_file_size ||
        (region_size == 0U && file_offset == (uint32)whole_file_size) ||
        (region_size != 0U &&
            region_size > (uint32)whole_file_size - file_offset))
    {
        divoom_port_file_close(file_handle);
        return NULL;
    }
    if(region_size == 0U)
    {
        region_size = (uint32)whole_file_size - file_offset;
    }
    if(region_size < 20U)
    {
        *probe_result = DIVOOM_LIBWEBP_FILE_PROBE_CORRUPT;
        divoom_port_file_close(file_handle);
        return NULL;
    }
    if(divoom_port_file_seek(file_handle, (long)file_offset,
            DIVOOM_PORT_SEEK_SET) != 0 ||
        !divoom_libwebp_decode_file_read_exact(file_handle, header,
            20U, NULL))
    {
        divoom_port_file_close(file_handle);
        return NULL;
    }
    if(!divoom_libwebp_decode_file_tag_equal(header, "RIFF") ||
        !divoom_libwebp_decode_file_tag_equal(header + 8, "WEBP"))
    {
        *probe_result = DIVOOM_LIBWEBP_FILE_PROBE_CORRUPT;
        divoom_port_file_close(file_handle);
        return NULL;
    }
    riff_size = divoom_libwebp_decode_file_read_le32(header + 4);
    if(riff_size < 4U || riff_size > U32_MAX - 8U ||
        riff_size + 8U > region_size)
    {
        *probe_result = DIVOOM_LIBWEBP_FILE_PROBE_CORRUPT;
        divoom_port_file_close(file_handle);
        return NULL;
    }
    if(!divoom_libwebp_decode_file_tag_equal(header + 12, "VP8X"))
    {
        divoom_port_file_close(file_handle);
        *probe_result = DIVOOM_LIBWEBP_FILE_PROBE_STATIC;
        return NULL;
    }
    vp8x_size = divoom_libwebp_decode_file_read_le32(header + 16);
    if(vp8x_size != 10U || 20U + vp8x_size > riff_size + 8U ||
        region_size < (uint32)sizeof(header) ||
        !divoom_libwebp_decode_file_read_exact(file_handle, header + 20,
            10U, NULL))
    {
        divoom_port_file_close(file_handle);
        return NULL;
    }
    if((header[20] & DIVOOM_LIBWEBP_ANIMATION_FLAG) == 0)
    {
        divoom_port_file_close(file_handle);
        *probe_result = DIVOOM_LIBWEBP_FILE_PROBE_STATIC;
        return NULL;
    }

    width = 1U + divoom_libwebp_decode_file_read_le24(header + 24);
    height = 1U + divoom_libwebp_decode_file_read_le24(header + 27);
    canvas_size = (uint64_t)width * (uint64_t)height *
        DIVOOM_LIBWEBP_NUM_CHANNELS;
    if(width == 0 || height == 0 || canvas_size == 0 || canvas_size > U32_MAX)
    {
        divoom_port_file_close(file_handle);
        return NULL;
    }

    phandle = divoom_system_malloc(sizeof(divoom_libwebp_file_anim_ctrl));
    if(phandle == NULL)
    {
        divoom_port_file_close(file_handle);
        return NULL;
    }
    phandle->file_handle = file_handle;
    phandle->file_base_offset = file_offset;
    phandle->file_size = region_size;
    phandle->riff_end = riff_size + 8U;
    phandle->width = (int32)width;
    phandle->height = (int32)height;
    phandle->canvas_size = (uint32)canvas_size;

    if(!divoom_libwebp_decode_file_scan_animation(phandle))
    {
        divoom_libwebp_decode_file_anim_delete(phandle);
        return NULL;
    }
    phandle->read_buf = divoom_system_malloc_no_zero(
        DIVOOM_LIBWEBP_FILE_READ_CHUNK_SIZE);
    phandle->curr_frame = divoom_system_malloc(phandle->canvas_size);
    phandle->prev_frame_disposed = divoom_system_malloc(phandle->canvas_size);
    if(phandle->read_buf == NULL || phandle->curr_frame == NULL ||
        phandle->prev_frame_disposed == NULL)
    {
        divoom_libwebp_decode_file_anim_delete(phandle);
        return NULL;
    }

    *probe_result = DIVOOM_LIBWEBP_FILE_PROBE_ANIMATION;
    return phandle;
}

static Pdivoom_libwebp_file_anim_ctrl divoom_libwebp_decode_file_anim_create(
    const char *file_name, divoom_libwebp_file_probe_result *probe_result)
{
    return divoom_libwebp_decode_file_anim_create_region(file_name, 0U, 0U,
        probe_result);
}

static void divoom_libwebp_decode_file_static_delete(
    Pdivoom_libwebp_file_static_ctrl phandle)
{
    if(phandle == NULL)
    {
        return;
    }
    if(phandle->file_handle != NULL)
    {
        divoom_port_file_close(phandle->file_handle);
    }
    if(phandle->read_buf != NULL)
    {
        divoom_system_free(phandle->read_buf);
    }
    if(phandle->curr_frame != NULL)
    {
        divoom_system_free(phandle->curr_frame);
    }
    divoom_system_free(phandle);
}

/* 静态 WebP 只读文件头取尺寸，压缩图像保留在原文件区间中。 */
static Pdivoom_libwebp_file_static_ctrl
divoom_libwebp_decode_file_static_create_region(const char *file_name,
    uint32 file_offset, uint32 region_size)
{
    void *file_handle;
    long whole_file_size;
    uint8 feature_buf[64];
    uint32 feature_size;
    uint64_t canvas_size;
    WebPBitstreamFeatures features;
    Pdivoom_libwebp_file_static_ctrl phandle;

    file_handle = divoom_port_file_open(file_name, "rb");
    if(file_handle == NULL ||
        divoom_port_file_seek(file_handle, 0, DIVOOM_PORT_SEEK_END) != 0)
    {
        if(file_handle != NULL)
        {
            divoom_port_file_close(file_handle);
        }
        return NULL;
    }
    whole_file_size = divoom_port_file_tell(file_handle);
    if(whole_file_size <= 0 || file_offset > (uint32)whole_file_size ||
        region_size == 0U ||
        region_size > (uint32)whole_file_size - file_offset)
    {
        divoom_port_file_close(file_handle);
        return NULL;
    }
    feature_size = region_size < (uint32)sizeof(feature_buf) ? region_size :
        (uint32)sizeof(feature_buf);
    if(divoom_port_file_seek(file_handle, (long)file_offset,
        DIVOOM_PORT_SEEK_SET) != 0 ||
        !divoom_libwebp_decode_file_read_exact(file_handle, feature_buf,
            feature_size, NULL))
    {
        divoom_port_file_close(file_handle);
        return NULL;
    }
    memset(&features, 0, sizeof(features));
    if(WebPGetFeatures(feature_buf, feature_size, &features) !=
        VP8_STATUS_OK || features.has_animation || features.width <= 0 ||
        features.height <= 0)
    {
        divoom_port_file_close(file_handle);
        return NULL;
    }
    canvas_size = (uint64_t)features.width * (uint64_t)features.height *
        DIVOOM_LIBWEBP_NUM_CHANNELS;
    if(canvas_size == 0 || canvas_size > U32_MAX)
    {
        divoom_port_file_close(file_handle);
        return NULL;
    }

    phandle = divoom_system_malloc(sizeof(divoom_libwebp_file_static_ctrl));
    if(phandle == NULL)
    {
        divoom_port_file_close(file_handle);
        return NULL;
    }
    phandle->file_handle = file_handle;
    phandle->file_base_offset = file_offset;
    phandle->file_size = region_size;
    phandle->width = features.width;
    phandle->height = features.height;
    phandle->canvas_size = (uint32)canvas_size;
    phandle->read_buf = divoom_system_malloc_no_zero(
        DIVOOM_LIBWEBP_FILE_READ_CHUNK_SIZE);
    phandle->curr_frame = divoom_system_malloc_no_zero(phandle->canvas_size);
    if(phandle->read_buf == NULL || phandle->curr_frame == NULL)
    {
        divoom_libwebp_decode_file_static_delete(phandle);
        return NULL;
    }
    return phandle;
}

static int divoom_libwebp_decode_file_static_has_more_frames(
    Pdivoom_libwebp_file_static_ctrl phandle)
{
    return phandle != NULL && !phandle->decoded;
}

static void divoom_libwebp_decode_file_static_reset(
    Pdivoom_libwebp_file_static_ctrl phandle)
{
    if(phandle != NULL)
    {
        phandle->decoded = 0;
    }
}

/* 压缩流按 16 KiB 喂入增量解码器，输出直接写入最终 BGRA 画布。 */
static int divoom_libwebp_decode_file_static_get_next(
    Pdivoom_libwebp_file_static_ctrl phandle, uint8 **buf,
    int32 *timestamp)
{
    WebPIDecoder *decoder;
    VP8StatusCode status = VP8_STATUS_SUSPENDED;
    uint32 remaining;

    if(phandle != NULL)
    {
        phandle->last_status = DIVOOM_LIBWEBP_STATUS_UNKNOWN;
    }
    if(phandle == NULL || buf == NULL || timestamp == NULL ||
        phandle->decoded ||
        divoom_port_file_seek(phandle->file_handle,
            (long)phandle->file_base_offset, DIVOOM_PORT_SEEK_SET) != 0)
    {
        return 0;
    }
    decoder = WebPINewRGB(MODE_bgrA, phandle->curr_frame,
        phandle->canvas_size, phandle->width * DIVOOM_LIBWEBP_NUM_CHANNELS);
    if(decoder == NULL)
    {
        phandle->last_status = VP8_STATUS_OUT_OF_MEMORY;
        return 0;
    }
    remaining = phandle->file_size;
    while(remaining > 0U && status != VP8_STATUS_OK)
    {
        uint32 read_size = remaining > DIVOOM_LIBWEBP_FILE_READ_CHUNK_SIZE ?
            DIVOOM_LIBWEBP_FILE_READ_CHUNK_SIZE : remaining;

        if(!divoom_libwebp_decode_file_read_exact(phandle->file_handle,
            phandle->read_buf, read_size, &phandle->max_read_size))
        {
            WebPIDelete(decoder);
            return 0;
        }
        status = WebPIAppend(decoder, phandle->read_buf, read_size);
        phandle->last_status = status;
        remaining -= read_size;
        if(status != VP8_STATUS_OK && status != VP8_STATUS_SUSPENDED)
        {
            if(status == VP8_STATUS_NOT_ENOUGH_DATA && remaining != 0U)
            {
                phandle->last_status = DIVOOM_LIBWEBP_STATUS_UNKNOWN;
            }
            WebPIDelete(decoder);
            return 0;
        }
    }
    WebPIDelete(decoder);
    if(status != VP8_STATUS_OK)
    {
        /* All declared bytes were read successfully, but the stream is incomplete. */
        phandle->last_status = status == VP8_STATUS_SUSPENDED ?
            VP8_STATUS_NOT_ENOUGH_DATA : status;
        return 0;
    }
    phandle->decoded = 1;
    *timestamp = 0;
    *buf = phandle->curr_frame;
    return 1;
}

static int divoom_libwebp_decode_file_anim_is_full_frame(
    const divoom_libwebp_file_frame_info *frame,
    Pdivoom_libwebp_file_anim_ctrl phandle)
{
    return frame->x_offset == 0 && frame->y_offset == 0 &&
        frame->width == phandle->width && frame->height == phandle->height;
}

static int divoom_libwebp_decode_file_anim_is_keyframe(
    Pdivoom_libwebp_file_anim_ctrl phandle,
    const divoom_libwebp_file_frame_info *frame)
{
    if(frame->frame_num == 1)
    {
        return 1;
    }
    if((!frame->has_alpha || frame->blend_method == WEBP_MUX_NO_BLEND) &&
        divoom_libwebp_decode_file_anim_is_full_frame(frame, phandle))
    {
        return 1;
    }
    if(phandle->prev_frame_valid &&
        phandle->prev_frame.dispose_method == WEBP_MUX_DISPOSE_BACKGROUND &&
        (divoom_libwebp_decode_file_anim_is_full_frame(&phandle->prev_frame,
            phandle) || phandle->prev_frame_was_keyframe))
    {
        return 1;
    }
    return 0;
}

static uint32 divoom_libwebp_decode_file_anim_channelwise_multiply(
    uint32 pixel, uint32 scale)
{
    uint32 mask = 0x00FF00FFU;
    uint32 rb = ((pixel & mask) * scale) >> 8;
    uint32 ag = ((pixel >> 8) & mask) * scale;

    return (rb & mask) | (ag & ~mask);
}

/* MODE_bgrA 为预乘 alpha，沿用 libwebp AnimDecoder 的整数混合公式。 */
static void divoom_libwebp_decode_file_anim_blend_row(uint32 *src,
    const uint32 *dst, int32 pixel_num)
{
    int32 i;

    for(i = 0; i < pixel_num; i++)
    {
        uint8 src_alpha = (uint8)((src[i] >>
            DIVOOM_LIBWEBP_CHANNEL_SHIFT(3)) & 0xFFU);

        if(src_alpha != 0xFFU)
        {
            src[i] += divoom_libwebp_decode_file_anim_channelwise_multiply(
                dst[i], 256U - src_alpha);
        }
    }
}

static void divoom_libwebp_decode_file_anim_zero_rect(uint8 *buf,
    int32 canvas_width, const divoom_libwebp_file_frame_info *frame)
{
    int32 y;
    uint32 stride = (uint32)canvas_width * DIVOOM_LIBWEBP_NUM_CHANNELS;
    uint8 *line = buf + (uint32)frame->y_offset * stride +
        (uint32)frame->x_offset * DIVOOM_LIBWEBP_NUM_CHANNELS;

    for(y = 0; y < frame->height; y++)
    {
        memset(line, 0, (size_t)frame->width * DIVOOM_LIBWEBP_NUM_CHANNELS);
        line += stride;
    }
}

static void divoom_libwebp_decode_file_anim_find_blend_range(
    const divoom_libwebp_file_frame_info *src,
    const divoom_libwebp_file_frame_info *dst, int32 canvas_y,
    int32 *left1, int32 *width1, int32 *left2, int32 *width2)
{
    int32 src_max_x = src->x_offset + src->width;
    int32 dst_max_x = dst->x_offset + dst->width;
    int32 dst_max_y = dst->y_offset + dst->height;

    *left1 = -1;
    *width1 = 0;
    *left2 = -1;
    *width2 = 0;
    if(canvas_y < dst->y_offset || canvas_y >= dst_max_y ||
        src->x_offset >= dst_max_x || src_max_x <= dst->x_offset)
    {
        *left1 = src->x_offset;
        *width1 = src->width;
        return;
    }
    if(src->x_offset < dst->x_offset)
    {
        *left1 = src->x_offset;
        *width1 = dst->x_offset - src->x_offset;
    }
    if(src_max_x > dst_max_x)
    {
        *left2 = dst_max_x;
        *width2 = src_max_x - dst_max_x;
    }
}

static int divoom_libwebp_decode_file_anim_find_next_frame(
    Pdivoom_libwebp_file_anim_ctrl phandle,
    divoom_libwebp_file_frame_info *frame)
{
    uint8 chunk_header[DIVOOM_LIBWEBP_CHUNK_HEADER_SIZE];
    uint32 chunk_offset = phandle->next_chunk_offset;
    uint32 payload_size;
    uint32 next_chunk_offset;

    while(chunk_offset < phandle->riff_end)
    {
        if(!divoom_libwebp_decode_file_get_chunk_bounds(phandle, chunk_offset,
            phandle->riff_end, chunk_header, &payload_size,
            &next_chunk_offset))
        {
            return 0;
        }
        if(divoom_libwebp_decode_file_tag_equal(chunk_header, "ANMF"))
        {
            return divoom_libwebp_decode_file_parse_frame(phandle,
                chunk_offset, phandle->next_frame, frame);
        }
        chunk_offset = next_chunk_offset;
    }
    return 0;
}

static const char *divoom_libwebp_vp8_status_name(VP8StatusCode status);

/* 把 ANMF 内流包成 RIFF+WEBP+VP8X，走 libwebp 官方静态图路径。 */
static int divoom_libwebp_decode_file_anim_append_still_header(WebPIDecoder *decoder,
    const divoom_libwebp_file_frame_info *frame, VP8StatusCode *status)
{
    uint8 header[DIVOOM_LIBWEBP_STILL_HEADER_SIZE];
    uint32 riff_size;

    if(decoder == NULL || frame == NULL || status == NULL || frame->width <= 0 || frame->height <= 0)
    {
        return 0;
    }
    memcpy(header, "RIFF", 4);
    riff_size = 4U + DIVOOM_LIBWEBP_CHUNK_HEADER_SIZE + DIVOOM_LIBWEBP_VP8X_PAYLOAD_SIZE + frame->stream_size;
    divoom_libwebp_decode_file_write_le32(header + 4, riff_size);
    memcpy(header + 8, "WEBP", 4);
    memcpy(header + 12, "VP8X", 4);
    divoom_libwebp_decode_file_write_le32(header + 16, DIVOOM_LIBWEBP_VP8X_PAYLOAD_SIZE);
    memset(header + 20, 0, DIVOOM_LIBWEBP_VP8X_PAYLOAD_SIZE);
    if(frame->has_alpha)
    {
        header[20] = (uint8)DIVOOM_LIBWEBP_ALPHA_FLAG;
    }
    divoom_libwebp_decode_file_write_le24(header + 24, (uint32)frame->width - 1U);
    divoom_libwebp_decode_file_write_le24(header + 27, (uint32)frame->height - 1U);
    *status = WebPIAppend(decoder, header, DIVOOM_LIBWEBP_STILL_HEADER_SIZE);
    return (*status == VP8_STATUS_OK || *status == VP8_STATUS_SUSPENDED);
}

/* 当前帧压缩流按 16 KiB 喂给 libwebp，输出直接落到画布对应矩形。 */
static int divoom_libwebp_decode_file_anim_decode_frame(
    Pdivoom_libwebp_file_anim_ctrl phandle,
    const divoom_libwebp_file_frame_info *frame)
{
    WebPDecoderConfig config;
    WebPIDecoder *decoder;
    VP8StatusCode status = VP8_STATUS_SUSPENDED;
    uint32 stride = (uint32)phandle->width * DIVOOM_LIBWEBP_NUM_CHANNELS;
    uint32 out_offset = (uint32)frame->y_offset * stride + (uint32)frame->x_offset * DIVOOM_LIBWEBP_NUM_CHANNELS;
    uint32 read_offset = frame->stream_offset;
    uint32 remaining = frame->stream_size;

    if(!WebPInitDecoderConfig(&config))
    {
        return 0;
    }
    /* 文件动画在预乘画布上合成，输出 RGB565A8 时再统一还原直通颜色。 */
    config.output.colorspace = MODE_bgrA;
    config.output.is_external_memory = 1;
    config.output.u.RGBA.rgba = phandle->curr_frame + out_offset;
    config.output.u.RGBA.stride = (int)stride;
    config.output.u.RGBA.size = phandle->canvas_size - (uint32)frame->y_offset * stride;
    config.options.use_threads = 0;
    decoder = WebPIDecode(NULL, 0, &config);
    if(decoder == NULL)
    {
        phandle->last_status = VP8_STATUS_OUT_OF_MEMORY;
        divoom_print("[webp] file animation WebPIDecode fail frame=%d size=%dx%d\n", frame->frame_num, frame->width, frame->height);
        return 0;
    }
    if(!divoom_libwebp_decode_file_anim_append_still_header(decoder, frame, &status))
    {
        divoom_print("[webp] file animation still header fail status=%d(%s) frame=%d size=%dx%d stream=%u\n", status, divoom_libwebp_vp8_status_name(status), frame->frame_num, frame->width, frame->height, frame->stream_size);
        WebPIDelete(decoder);
        return 0;
    }
    if(phandle->file_base_offset > U32_MAX - read_offset ||
        divoom_port_file_seek(phandle->file_handle,
            (long)(phandle->file_base_offset + read_offset),
            DIVOOM_PORT_SEEK_SET) != 0)
    {
        divoom_print("[webp] file animation seek fail frame=%d offset=%u\n", frame->frame_num, read_offset);
        WebPIDelete(decoder);
        return 0;
    }
    while(remaining > 0 && status != VP8_STATUS_OK)
    {
        uint32 read_size = remaining > DIVOOM_LIBWEBP_FILE_READ_CHUNK_SIZE ? DIVOOM_LIBWEBP_FILE_READ_CHUNK_SIZE : remaining;

        if(!divoom_libwebp_decode_file_read_exact(phandle->file_handle, phandle->read_buf, read_size, &phandle->max_read_size))
        {
            divoom_print("[webp] file animation read fail frame=%d remain=%u\n", frame->frame_num, remaining);
            WebPIDelete(decoder);
            return 0;
        }
        status = WebPIAppend(decoder, phandle->read_buf, read_size);
        phandle->last_status = status;
        read_offset += read_size;
        remaining -= read_size;
        if(status != VP8_STATUS_OK && status != VP8_STATUS_SUSPENDED)
        {
            if(status == VP8_STATUS_NOT_ENOUGH_DATA && remaining != 0U)
            {
                phandle->last_status = DIVOOM_LIBWEBP_STATUS_UNKNOWN;
            }
            divoom_print("[webp] file animation WebPIAppend fail status=%d(%s) frame=%d size=%dx%d remain=%u\n", status, divoom_libwebp_vp8_status_name(status), frame->frame_num, frame->width, frame->height, remaining);
            WebPIDelete(decoder);
            return 0;
        }
    }
    phandle->last_status = status == VP8_STATUS_SUSPENDED ?
        VP8_STATUS_NOT_ENOUGH_DATA : status;
    if(status != VP8_STATUS_OK || config.output.width != frame->width || config.output.height != frame->height)
    {
        divoom_print("[webp] file animation decode fail status=%d(%s) frame=%d want=%dx%d got=%dx%d\n", status, divoom_libwebp_vp8_status_name(status), frame->frame_num, frame->width, frame->height, config.output.width, config.output.height);
        WebPIDelete(decoder);
        return 0;
    }
    WebPIDelete(decoder);
    return 1;
}

static int divoom_libwebp_decode_file_anim_has_more_frames(
    Pdivoom_libwebp_file_anim_ctrl phandle)
{
    return phandle != NULL && phandle->next_frame <= phandle->frame_num;
}

static void divoom_libwebp_decode_file_anim_reset(
    Pdivoom_libwebp_file_anim_ctrl phandle)
{
    if(phandle == NULL)
    {
        return;
    }
    phandle->next_frame = 1;
    phandle->next_chunk_offset = phandle->first_frame_offset;
    phandle->prev_frame_timestamp = 0;
    phandle->prev_frame_valid = 0;
    phandle->prev_frame_was_keyframe = 0;
    memset(&phandle->prev_frame, 0, sizeof(phandle->prev_frame));
}

/* 返回值与 WebPAnimDecoderGetNext 一致，timestamp 为累计毫秒。 */
static int divoom_libwebp_decode_file_anim_get_next(
    Pdivoom_libwebp_file_anim_ctrl phandle, uint8 **buf,
    int32 *timestamp)
{
    divoom_libwebp_file_frame_info frame;
    int32 is_keyframe;

    if(phandle != NULL)
    {
        phandle->last_status = DIVOOM_LIBWEBP_STATUS_UNKNOWN;
    }
    if(phandle == NULL || buf == NULL || timestamp == NULL)
    {
        return 0;
    }
    if(!divoom_libwebp_decode_file_anim_has_more_frames(phandle))
    {
        return 0;
    }
    if(!divoom_libwebp_decode_file_anim_find_next_frame(phandle, &frame))
    {
        divoom_print("[webp] file animation find_next_frame fail next=%d/%d\n", phandle->next_frame, phandle->frame_num);
        return 0;
    }

    is_keyframe = divoom_libwebp_decode_file_anim_is_keyframe(phandle, &frame);
    if(is_keyframe)
    {
        memset(phandle->curr_frame, 0, phandle->canvas_size);
    }
    else
    {
        memcpy(phandle->curr_frame, phandle->prev_frame_disposed,
            phandle->canvas_size);
    }
    if(!divoom_libwebp_decode_file_anim_decode_frame(phandle, &frame))
    {
        return 0;
    }

    if(frame.frame_num > 1 && frame.blend_method == WEBP_MUX_BLEND &&
        !is_keyframe)
    {
        int32 y;

        if(phandle->prev_frame.dispose_method == WEBP_MUX_DISPOSE_NONE)
        {
            for(y = 0; y < frame.height; y++)
            {
                uint32 offset = (uint32)(frame.y_offset + y) *
                    (uint32)phandle->width + (uint32)frame.x_offset;

                divoom_libwebp_decode_file_anim_blend_row(
                    (uint32 *)phandle->curr_frame + offset,
                    (const uint32 *)phandle->prev_frame_disposed + offset,
                    frame.width);
            }
        }
        else
        {
            for(y = 0; y < frame.height; y++)
            {
                int32 canvas_y = frame.y_offset + y;
                int32 left1;
                int32 width1;
                int32 left2;
                int32 width2;

                divoom_libwebp_decode_file_anim_find_blend_range(&frame,
                    &phandle->prev_frame, canvas_y, &left1, &width1,
                    &left2, &width2);
                if(width1 > 0)
                {
                    uint32 offset1 = (uint32)canvas_y *
                        (uint32)phandle->width + (uint32)left1;

                    divoom_libwebp_decode_file_anim_blend_row(
                        (uint32 *)phandle->curr_frame + offset1,
                        (const uint32 *)phandle->prev_frame_disposed + offset1,
                        width1);
                }
                if(width2 > 0)
                {
                    uint32 offset2 = (uint32)canvas_y *
                        (uint32)phandle->width + (uint32)left2;

                    divoom_libwebp_decode_file_anim_blend_row(
                        (uint32 *)phandle->curr_frame + offset2,
                        (const uint32 *)phandle->prev_frame_disposed + offset2,
                        width2);
                }
            }
        }
    }

    phandle->prev_frame_timestamp += frame.duration;
    *timestamp = phandle->prev_frame_timestamp;
    phandle->prev_frame = frame;
    phandle->prev_frame_valid = 1;
    phandle->prev_frame_was_keyframe = (uint8)is_keyframe;
    memcpy(phandle->prev_frame_disposed, phandle->curr_frame,
        phandle->canvas_size);
    if(frame.dispose_method == WEBP_MUX_DISPOSE_BACKGROUND)
    {
        divoom_libwebp_decode_file_anim_zero_rect(
            phandle->prev_frame_disposed, phandle->width, &frame);
    }
    phandle->next_chunk_offset = frame.next_chunk_offset;
    phandle->next_frame++;
    *buf = phandle->curr_frame;
    return 1;
}

static const char *divoom_libwebp_vp8_status_name(VP8StatusCode status)
{
    switch(status)
    {
        case VP8_STATUS_OK:
            return "OK";
        case VP8_STATUS_OUT_OF_MEMORY:
            return "OUT_OF_MEMORY";
        case VP8_STATUS_INVALID_PARAM:
            return "INVALID_PARAM";
        case VP8_STATUS_BITSTREAM_ERROR:
            return "BITSTREAM_ERROR";
        case VP8_STATUS_UNSUPPORTED_FEATURE:
            return "UNSUPPORTED_FEATURE";
        case VP8_STATUS_SUSPENDED:
            return "SUSPENDED";
        case VP8_STATUS_USER_ABORT:
            return "USER_ABORT";
        case VP8_STATUS_NOT_ENOUGH_DATA:
            return "NOT_ENOUGH_DATA";
        default:
            return "UNKNOWN";
    }
}

/* Only explicit bitstream errors (or incomplete fully-read streams) justify deletion.
 * OOM, I/O, unsupported features and unknown failures must preserve the asset. */
static void divoom_libwebp_decode_handle_failure(const char *file_name,
    VP8StatusCode status)
{
    int corrupt = status == VP8_STATUS_BITSTREAM_ERROR ||
        status == VP8_STATUS_NOT_ENOUGH_DATA;

    divoom_print("[webp] decode failure status=%d(%s), action=%s, file=%s\n",
        status, divoom_libwebp_vp8_status_name(status),
        corrupt && file_name != NULL ? "delete corrupt file" : "keep file",
        file_name != NULL ? file_name : "NULL");
    if(corrupt && file_name != NULL)
    {
        divoom_app_com_del_file((char *)file_name);
    }
}

static const char *divoom_libwebp_mux_status_name(WebPMuxError status)
{
    switch(status)
    {
        case WEBP_MUX_OK:
            return "OK";
        case WEBP_MUX_NOT_FOUND:
            return "NOT_FOUND";
        case WEBP_MUX_INVALID_ARGUMENT:
            return "INVALID_ARGUMENT";
        case WEBP_MUX_BAD_DATA:
            return "BAD_DATA";
        case WEBP_MUX_MEMORY_ERROR:
            return "MEMORY_ERROR";
        case WEBP_MUX_NOT_ENOUGH_DATA:
            return "NOT_ENOUGH_DATA";
        default:
            return "UNKNOWN";
    }
}

static VP8StatusCode divoom_libwebp_print_features_status(const char *tag,
    const char *file_name, const uint8 *file_buf, int file_len)
{
    WebPBitstreamFeatures features;
    VP8StatusCode status;
    WebPData webp_data;
    WebPMux *mux;
    WebPMuxError mux_status;
    uint32_t flags = 0;

    memset(&features, 0, sizeof(features));
    if(file_buf == NULL || file_len <= 0)
    {
        divoom_print("[webp] %s features fail: INVALID_PARAM file=%s len=%d\n",
            tag ? tag : "check", file_name ? file_name : "NULL", file_len);
        return VP8_STATUS_INVALID_PARAM;
    }

    status = WebPGetFeatures(file_buf, (size_t)file_len, &features);
    divoom_print("[webp] %s features: status=%d(%s), file=%s, len=%d, w=%d, h=%d, alpha=%d, anim=%d, format=%d\n",
        tag ? tag : "check", status, divoom_libwebp_vp8_status_name(status),
        file_name ? file_name : "NULL", file_len, features.width, features.height,
        features.has_alpha, features.has_animation, features.format);

    WebPDataInit(&webp_data);
    webp_data.bytes = file_buf;
    webp_data.size = (size_t)file_len;
    mux = WebPMuxCreate(&webp_data, 0);
    if(mux == NULL)
    {
        divoom_print("[webp] %s WebPMuxCreate fail: file=%s, len=%d\n",
            tag ? tag : "check", file_name ? file_name : "NULL", file_len);
    }
    else
    {
        mux_status = WebPMuxGetFeatures(mux, &flags);
        divoom_print("[webp] %s WebPMuxGetFeatures: status=%d(%s), flags=0x%08x, file=%s\n",
            tag ? tag : "check", mux_status,
            divoom_libwebp_mux_status_name(mux_status), (unsigned)flags,
            file_name ? file_name : "NULL");
        WebPMuxDelete(mux);
    }
    return status;
}

static void divoom_libwebp_store_rgb565(uint8 *dst, uint16 c)
{
    dst[0] = (uint8)(c & 0xFFU);
    dst[1] = (uint8)(c >> 8);
}

/* libwebp 的 MODE_bgrA 输出为预乘颜色；LVGL RGB565A8 需要直通颜色。 */
static uint8 divoom_libwebp_unpremultiply_channel(uint8 color, uint8 alpha)
{
    uint32 value;

    if(alpha == 0U)
    {
        return 0U;
    }
    if(alpha == 0xFFU)
    {
        return color;
    }
    value = ((uint32)color * 255U + (uint32)alpha / 2U) / (uint32)alpha;
    return value > 255U ? 255U : (uint8)value;
}

static void divoom_libwebp_bgra_to_rgb565_buf(const uint8 *src_bgra, uint8 *dst_rgb565, int px_count)
{
    int i;

    if(src_bgra == NULL || dst_rgb565 == NULL || px_count <= 0)
    {
        return;
    }
    for(i = 0; i < px_count; i++)
    {
        uint16 c = divoom_aw_rgb_to_rgb565(src_bgra[i * 4 + 2], src_bgra[i * 4 + 1], src_bgra[i * 4 + 0]);
        divoom_libwebp_store_rgb565(&dst_rgb565[i * 2], c);
    }
}

static void divoom_libwebp_bgra_to_rgb565a8_buf(const uint8 *src_bgra, uint8 *dst_rgb565a8, int px_count)
{
    uint8 *alpha;
    int i;

    if(src_bgra == NULL || dst_rgb565a8 == NULL || px_count <= 0)
    {
        return;
    }
    alpha = dst_rgb565a8 + px_count * 2;
    for(i = 0; i < px_count; i++)
    {
        uint8 a = src_bgra[i * 4 + 3];
        uint8 r = divoom_libwebp_unpremultiply_channel(
            src_bgra[i * 4 + 2], a);
        uint8 g = divoom_libwebp_unpremultiply_channel(
            src_bgra[i * 4 + 1], a);
        uint8 b = divoom_libwebp_unpremultiply_channel(
            src_bgra[i * 4 + 0], a);
        uint16 c = divoom_aw_rgb_to_rgb565(r, g, b);

        divoom_libwebp_store_rgb565(&dst_rgb565a8[i * 2], c);
        alpha[i] = a;
    }
}

static void divoom_libwebp_rgba_to_rgb565a8_buf(const uint8 *src_rgba, uint8 *dst_rgb565a8, int px_count)
{
    uint8 *alpha;
    int i;

    if(src_rgba == NULL || dst_rgb565a8 == NULL || px_count <= 0)
    {
        return;
    }
    alpha = dst_rgb565a8 + px_count * 2;
    for(i = 0; i < px_count; i++)
    {
        uint16 c = divoom_aw_rgb_to_rgb565(src_rgba[i * 4 + 0], src_rgba[i * 4 + 1], src_rgba[i * 4 + 2]);
        divoom_libwebp_store_rgb565(&dst_rgb565a8[i * 2], c);
        alpha[i] = src_rgba[i * 4 + 3];
    }
}

static void divoom_libwebp_fit_dims_in_box(int iw, int ih, int max_w, int max_h, int *ow, int *oh)
{
	long long a, b;

	if (iw <= 0 || ih <= 0 || max_w < 1 || max_h < 1)
	{
		*ow = 1;
		*oh = 1;
		return;
	}
	if (iw <= max_w && ih <= max_h)
	{
		*ow = iw;
		*oh = ih;
		return;
	}
	a = (long long)max_h * iw;
	b = (long long)max_w * ih;
	if (a < b)
	{
		*ow = (int)((long long)max_h * iw / ih);
		if (*ow < 1)
			*ow = 1;
		*oh = max_h;
	}
	else
	{
		*ow = max_w;
		*oh = (int)((long long)max_w * ih / iw);
		if (*oh < 1)
			*oh = 1;
	}
}

int divoom_libwebp_decode_static_file_first_565a8(char *file_name,
    uint8 **out_buf, int *out_w, int *out_h)
{
    int file_len = 0;
    int y;
    int x;
    uint8 *file_buf;
    uint8 *rgb565a8_buf;
    uint8 *alpha;
    uint32 pixel_count;
    WebPDecoderConfig config;
    VP8StatusCode status;

    if(file_name == NULL || out_buf == NULL || out_w == NULL || out_h == NULL)
    {
        return -1;
    }
    *out_buf = NULL;
    *out_w = 0;
    *out_h = 0;

    file_buf = divoom_app_com_read_file(file_name, &file_len);
    if(file_buf == NULL || file_len <= 0)
    {
        return -1;
    }
    if(!WebPInitDecoderConfig(&config))
    {
        divoom_system_free(file_buf);
        return -1;
    }

    status = WebPGetFeatures(file_buf, (size_t)file_len, &config.input);
    if(status != VP8_STATUS_OK)
    {
        divoom_system_free(file_buf);
        return -1;
    }
    if(config.input.has_animation)
    {
        divoom_system_free(file_buf);
        return 0;
    }

    /* 直接生成 LVGL RGB565A8 时使用直通 BGRA，避免颜色再次乘 alpha。 */
    config.output.colorspace = MODE_BGRA;
    status = WebPDecode(file_buf, (size_t)file_len, &config);
    divoom_system_free(file_buf);
    if(status != VP8_STATUS_OK)
    {
        divoom_print("[webp] static decode fail: status=%d(%s), file=%s\n",
            status, divoom_libwebp_vp8_status_name(status), file_name);
        WebPFreeDecBuffer(&config.output);
        return -1;
    }

    pixel_count = (uint32)config.output.width * (uint32)config.output.height;
    rgb565a8_buf = divoom_system_malloc_no_zero(
        divoom_pixel_format_size((uint32)config.output.width,
            (uint32)config.output.height, LV_COLOR_FORMAT_RGB565A8));
    if(rgb565a8_buf == NULL)
    {
        WebPFreeDecBuffer(&config.output);
        return -1;
    }

    alpha = rgb565a8_buf + pixel_count * DIVOOM_APP_RGB565_BYTES_PER_PX;
    for(y = 0; y < config.output.height; y++)
    {
        const uint8 *src_line = config.output.u.RGBA.rgba +
            (uint32)y * (uint32)config.output.u.RGBA.stride;
        uint8 *dst_line = rgb565a8_buf +
            (uint32)y * (uint32)config.output.width *
                DIVOOM_APP_RGB565_BYTES_PER_PX;

        for(x = 0; x < config.output.width; x++)
        {
            const uint8 *pixel = src_line + x * 4;
            uint16 color = divoom_aw_rgb_to_rgb565(pixel[2], pixel[1],
                pixel[0]);

            divoom_libwebp_store_rgb565(dst_line +
                x * DIVOOM_APP_RGB565_BYTES_PER_PX, color);
            alpha[(uint32)y * (uint32)config.output.width + (uint32)x] =
                pixel[3];
        }
    }

    *out_buf = rgb565a8_buf;
    *out_w = config.output.width;
    *out_h = config.output.height;
    WebPFreeDecBuffer(&config.output);
    return 1;
}

/*********************************************************************
  预览：尽量在 libwebp 内按 max_w/max_h 缩小首帧；静态图走 WebPDecode 缩放，
        动画仍用 AnimDecoder（整幅画布）再交给上层 stretch。
**********************************************************************/
uint8 *divoom_libwebp_decode_file_first_for_preview(char *file_name, int max_w, int max_h,
	int *out_w, int *out_h)
{
	int file_len;
	uint8 *buf;
	WebPDecoderConfig config;
	VP8StatusCode st;
	int sw, sh, y, dw, dh, stride;
	uint8 *src, *dst;

	if (out_w == NULL || out_h == NULL)
		return NULL;
	if (max_w < 16 || max_h < 16)
		return NULL;

	buf = divoom_app_com_read_file(file_name, &file_len);
	if (buf == NULL || file_len <= 0)
		return NULL;

	if (!WebPInitDecoderConfig(&config))
	{
		divoom_system_free(buf);
		return NULL;
	}
	st = WebPGetFeatures(buf, (size_t)file_len, &config.input);
	if (st == VP8_STATUS_OK && !config.input.has_animation)
	{
		divoom_libwebp_fit_dims_in_box(config.input.width, config.input.height, max_w, max_h, &sw, &sh);
		config.options.use_scaling = 1;
		config.options.scaled_width = sw;
		config.options.scaled_height = sh;
		config.options.no_fancy_upsampling = 1;
		config.output.colorspace = MODE_bgrA;
		st = WebPDecode(buf, (size_t)file_len, &config);
		divoom_system_free(buf);
		if (st != VP8_STATUS_OK)
		{
			WebPFreeDecBuffer(&config.output);
			return NULL;
		}
		dw = config.output.width;
		dh = config.output.height;
		stride = config.output.u.RGBA.stride;
		src = config.output.u.RGBA.rgba;
		dst = divoom_system_malloc((unsigned)(dw * dh * 4));
		if (dst == NULL)
		{
			WebPFreeDecBuffer(&config.output);
			return NULL;
		}
		for (y = 0; y < dh; y++)
			memcpy(dst + (unsigned)(y * dw * 4), src + (unsigned)(y * stride), (unsigned)(dw * 4));
		WebPFreeDecBuffer(&config.output);
		*out_w = dw;
		*out_h = dh;
		return dst;
	}

	{
		Pdivoom_libwebp_decode_ctrl phandle = divoom_system_malloc(sizeof(divoom_libwebp_decode_ctrl));
		WebPData webp_info;
		WebPAnimDecoderOptions options;

		if (phandle == NULL)
		{
			divoom_system_free(buf);
			return NULL;
		}
		memset(phandle, 0, sizeof(divoom_libwebp_decode_ctrl));
		phandle->file_buf = buf;
		phandle->pre_frame_time = -1;
		WebPDataInit(&webp_info);
		webp_info.bytes = buf;
		webp_info.size = (size_t)file_len;
		WebPAnimDecoderOptionsInit(&options);
		options.color_mode = MODE_bgrA;
		options.use_threads = 1;
		phandle->image_handle = WebPAnimDecoderNew(&webp_info, &options);
		if (phandle->image_handle == NULL)
		{
			divoom_system_free(buf);
			divoom_system_free(phandle);
			return NULL;
		}
		{
			WebPAnimInfo pic_info;

			WebPAnimDecoderGetInfo(phandle->image_handle, &pic_info);
			phandle->width = pic_info.canvas_width;
			phandle->high = pic_info.canvas_height;
			phandle->frame_num = pic_info.frame_count;
			if (phandle->frame_num == 0)
			{
				divoom_libwebp_decode_exit(phandle);
				return NULL;
			}
			dst = divoom_libwebp_decode_get_frame_buffer(phandle, NULL, NULL, NULL);
			divoom_libwebp_decode_exit(phandle);
			if (dst)
			{
				*out_w = pic_info.canvas_width;
				*out_h = pic_info.canvas_height;
			}
			return dst;
		}
	}
}

uint8 *divoom_libwebp_decode_file_first_for_preview_565a8(char *file_name,
    int max_w, int max_h, int *out_w, int *out_h)
{
	int file_len = 0;
	int sw = 0, sh = 0, dw = 0, dh = 0, stride = 0, y = 0;
	uint8 *buf = NULL;
	uint8 *dst = NULL;
	uint8 *src = NULL;
	WebPDecoderConfig config;
	VP8StatusCode st;

	if (max_w < 16 || max_h < 16)
		return NULL;

	buf = divoom_app_com_read_file(file_name, &file_len);
	if (buf == NULL || file_len <= 0)
		return NULL;

	if (!WebPInitDecoderConfig(&config))
	{
		divoom_system_free(buf);
		return NULL;
	}
	st = WebPGetFeatures(buf, (size_t)file_len, &config.input);
	if (st == VP8_STATUS_OK && !config.input.has_animation)
	{
		sw = max_w;
		sh = max_h;
		config.options.use_scaling = 1;
		config.options.scaled_width = sw;
		config.options.scaled_height = sh;
		config.options.no_fancy_upsampling = 1;
		config.output.colorspace = MODE_BGRA;
		st = WebPDecode(buf, (size_t)file_len, &config);
		divoom_system_free(buf);
		if (st != VP8_STATUS_OK)
		{
			WebPFreeDecBuffer(&config.output);
			return NULL;
		}
		dw = config.output.width;
		dh = config.output.height;
		stride = config.output.u.RGBA.stride;
		src = config.output.u.RGBA.rgba;
		dst = divoom_system_malloc(divoom_pixel_format_size((uint32)dw,
            (uint32)dh, LV_COLOR_FORMAT_RGB565A8));
		if (dst == NULL)
		{
			WebPFreeDecBuffer(&config.output);
			return NULL;
		}
		{
			uint8 *alpha = dst + (uint32)dw * (uint32)dh *
                DIVOOM_APP_RGB565_BYTES_PER_PX;
			int x;
			for (y = 0; y < dh; y++)
			{
				const uint8 *src_line = src + (unsigned)(y * stride);
				uint8 *dst_line = dst + (unsigned)(y * dw *
                    DIVOOM_APP_RGB565_BYTES_PER_PX);
				for (x = 0; x < dw; x++)
				{
					const uint8 *p = src_line + x * 4;
					uint16 c = divoom_aw_rgb_to_rgb565(p[2], p[1], p[0]);
					divoom_libwebp_store_rgb565(dst_line + x *
                        DIVOOM_APP_RGB565_BYTES_PER_PX, c);
					alpha[(uint32)y * (uint32)dw + (uint32)x] = p[3];
				}
			}
		}
		WebPFreeDecBuffer(&config.output);
		*out_w = dw;
		*out_h = dh;
		return dst;
	}

	{
		Pdivoom_libwebp_decode_ctrl phandle = divoom_system_malloc(sizeof(divoom_libwebp_decode_ctrl));
		WebPData webp_info;
		WebPAnimDecoderOptions options;

		if (phandle == NULL)
		{
			divoom_system_free(buf);
			return NULL;
		}
		memset(phandle, 0, sizeof(divoom_libwebp_decode_ctrl));
		phandle->file_buf = buf;
		phandle->pre_frame_time = -1;
		WebPDataInit(&webp_info);
		webp_info.bytes = buf;
		webp_info.size = (size_t)file_len;
		WebPAnimDecoderOptionsInit(&options);
		options.color_mode = MODE_BGRA;
		options.use_threads = 1;
		phandle->image_handle = WebPAnimDecoderNew(&webp_info, &options);
		if (phandle->image_handle == NULL)
		{
			divoom_system_free(buf);
			divoom_system_free(phandle);
			return NULL;
		}
		{
			WebPAnimInfo pic_info;

			WebPAnimDecoderGetInfo(phandle->image_handle, &pic_info);
			phandle->width = pic_info.canvas_width;
			phandle->high = pic_info.canvas_height;
			phandle->frame_num = pic_info.frame_count;
			if (phandle->frame_num == 0)
			{
				divoom_libwebp_decode_exit(phandle);
				return NULL;
			}
			{
				uint8 *frame_buf = divoom_libwebp_decode_get_frame_buffer(phandle,
                    NULL, NULL, NULL);
				if(frame_buf)
				{
					dst = divoom_system_malloc(divoom_pixel_format_size(
                        (uint32)max_w, (uint32)max_h, LV_COLOR_FORMAT_RGB565A8));
					if(dst)
					{
						divoom_rk_mpp_stretch_image_lvfmt(frame_buf,
                            pic_info.canvas_width, pic_info.canvas_height,
                            RK_FORMAT_BGRA_8888, dst, max_w, max_h,
                            LV_COLOR_FORMAT_RGB565A8);
					}
					divoom_system_free(frame_buf);
				}
			}
			divoom_libwebp_decode_exit(phandle);
			if (dst)
			{
				*out_w = max_w;
				*out_h = max_h;
			}
			return dst;
		}
	}
}

/*
#include <stdio.h>
#include <stdlib.h>
#include <webp/decode.h>

// Function to display or process the decoded frame (you can customize this)
void displayFrame(const uint8_t *frameRGBA, int width, int height) {
    // Your code to display or process the frame goes here
    // Example: You might use a graphics library to display the frame
    // (e.g., SDL, OpenGL, etc.)
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <animated_webp_file.webp>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *inputFile = argv[1];

    FILE *file = divoom_port_file_open(inputFile, "rb");
    if (!file) {
        fprintf(stderr, "Error opening file: %s\n", inputFile);
        return EXIT_FAILURE;
    }

    divoom_port_file_seek(file, 0, SEEK_END);
    size_t fileSize = divoom_port_file_tell(file);
    rewind(file);

    uint8_t *fileData = (uint8_t *)malloc(fileSize);
    if (!fileData) {
        fprintf(stderr, "Error allocating memory for file data.\n");
        divoom_port_file_close(file);
        return EXIT_FAILURE;
    }

    if (divoom_port_file_read(fileData, 1, fileSize, file) != fileSize) {
        fprintf(stderr, "Error reading file data.\n");
        free(fileData);
        divoom_port_file_close(file);
        return EXIT_FAILURE;
    }

    divoom_port_file_close(file);

    WebPAnimDecoder *dec = WebPAnimDecoderNew(fileData, fileSize);
    if (dec == NULL) {
        fprintf(stderr, "Error creating animation decoder.\n");
        free(fileData);
        return EXIT_FAILURE;
    }

    int width, height;
    WebPAnimDecoderGetInfo(dec, &width, &height);

    printf("Animation width: %d, height: %d\n", width, height);

    // Main loop for playing the animation
    while (1) {
        if (!WebPAnimDecoderHasMoreFrames(dec)) {
            // If there are no more frames, rewind to the beginning
            WebPAnimDecoderRewind(dec);
        }

        uint8_t *frameRGBA;
        int timestamp;

        if (WebPAnimDecoderGetNext(dec, &frameRGBA, &timestamp) == 0) {
            fprintf(stderr, "Error decoding frame.\n");
            break;
        }

        // Display or process the decoded frame
        displayFrame(frameRGBA, width, height);

        // Free the frame data after use
        free(frameRGBA);

        // You can add a delay here if needed, based on the timestamp
        // For example, use a sleep function to control the frame rate
        // Sleep(100); // Adjust the sleep duration as needed
    }

    WebPAnimDecoderDelete(dec);
    free(fileData);

    return EXIT_SUCCESS;
}

*/


/*********************************************************************
  Function:        divoom_libwebp_decode_image_buf
  Description:     对缓冲区数据进行解码，解码出一帧数据
  Input:           raw_buf   图像缓冲区
                   raw_size
  Return:          输出的RGB缓冲区
**********************************************************************/

void divoom_libwebp_decode_image_buf(uint8 *raw_buf, int raw_size, uint8 *rgb_buf, int image_size)
{
    int frame_time;
    uint8 *out_buf = NULL;
    WebPAnimDecoder *image_handle;
    WebPData webp_info;
    webp_info.bytes = raw_buf;
    webp_info.size = raw_size;
    WebPAnimDecoderOptions options;
    WebPAnimDecoderOptionsInit(&options);  // 初始化选项
    options.color_mode = MODE_rgbA;
    options.use_threads = 0;

    image_handle = WebPAnimDecoderNew(&webp_info, &options);
    if(image_handle == NULL)
    {
        return  ;   
    }
    WebPAnimDecoderGetNext(image_handle, &out_buf, &frame_time);
    if(out_buf)
    {
        for(int i = 0; i < image_size; i++)
        {
            rgb_buf[i*3+0] = out_buf[i*4+0];
            rgb_buf[i*3+1] = out_buf[i*4+1];
            rgb_buf[i*3+2] = out_buf[i*4+2];
        }
    }
    WebPAnimDecoderDelete(image_handle);
}


/*********************************************************************
  Function:        divoom_libwebp_decode_image_buf
  Description:     对缓冲区数据进行解码，解码出一帧数据
  Input:           raw_buf   图像缓冲区
                   raw_size
  Return:          输出的RGB缓冲区
**********************************************************************/

void divoom_libwebp_decode_image_buf_565(uint8 *raw_buf, int raw_size, uint8 *rgb_buf, int image_size)
{
    int frame_time;
    uint8 *out_buf = NULL;
    WebPAnimDecoder *image_handle;
    WebPData webp_info;
    webp_info.bytes = raw_buf;
    webp_info.size = raw_size;
    WebPAnimDecoderOptions options;
    WebPAnimDecoderOptionsInit(&options);  // 初始化选项
    options.color_mode = MODE_rgbA;
    options.use_threads = 0;

    image_handle = WebPAnimDecoderNew(&webp_info, &options);
    if(image_handle == NULL)
    {
        return  ;   
    }
    WebPAnimDecoderGetNext(image_handle, &out_buf, &frame_time);
    if(out_buf)
    {
        uint16 *rgb565_buf = (uint16 *)rgb_buf;
        for(int i = 0; i < image_size; i++)
        {
            rgb565_buf[i] = divoom_aw_rgb_to_rgb565(out_buf[i*4+0],out_buf[i*4+1],out_buf[i*4+2]);
        }
    }
    WebPAnimDecoderDelete(image_handle);
}

void divoom_libwebp_decode_image_buf_565a8(uint8 *raw_buf, int raw_size, uint8 *rgb_buf, int image_size)
{
    int frame_time;
    uint8 *out_buf = NULL;
    WebPAnimDecoder *image_handle;
    WebPData webp_info;
    WebPAnimDecoderOptions options;

    if(raw_buf == NULL || rgb_buf == NULL || image_size <= 0)
    {
        return;
    }

    WebPDataInit(&webp_info);
    webp_info.bytes = raw_buf;
    webp_info.size = raw_size;
    WebPAnimDecoderOptionsInit(&options);
    options.color_mode = MODE_rgbA;
    options.use_threads = 0;

    image_handle = WebPAnimDecoderNew(&webp_info, &options);
    if(image_handle == NULL)
    {
        return;
    }
    WebPAnimDecoderGetNext(image_handle, &out_buf, &frame_time);
    if(out_buf)
    {
        divoom_libwebp_rgba_to_rgb565a8_buf(out_buf, rgb_buf, image_size);
    }
    WebPAnimDecoderDelete(image_handle);
}

/*********************************************************************
  Function:        divoom_libwebp_decode_image_buf_width_info
  Description:     对缓冲区数据进行解码，解码出一帧数据
  Input:           raw_buf   图像缓冲区
                   raw_size
                   image_width      输出图像宽度
                   image_high       输出图像高度
  Return:          输出的RGB缓冲区
**********************************************************************/

uint8 *divoom_libwebp_decode_image_buf_width_info(uint8 *raw_buf, int raw_size, int *image_width, int *image_high)
{
    int frame_time;
    uint8 *out_buf = NULL, *rgb_buf = NULL;
    WebPAnimDecoder *image_handle;
    WebPData webp_info;
    webp_info.bytes = raw_buf;
    webp_info.size = raw_size;
    WebPAnimDecoderOptions options;
    WebPAnimDecoderOptionsInit(&options);
    options.color_mode = MODE_bgrA;
    options.use_threads = 0;

    image_handle = WebPAnimDecoderNew(&webp_info, &options);
    if(image_handle == NULL)
    {
        return  NULL;   
    }
    WebPAnimInfo pic_info;
    WebPAnimDecoderGetInfo(image_handle, &pic_info);
    *image_high  = pic_info.canvas_height;
    *image_width = pic_info.canvas_width;
    WebPAnimDecoderGetNext(image_handle, &out_buf, &frame_time);
    if(out_buf)
    {
        rgb_buf = divoom_system_malloc(pic_info.canvas_height * pic_info.canvas_width * 4);
        memcpy(rgb_buf, out_buf, pic_info.canvas_height * pic_info.canvas_width * 4);
    }
    WebPAnimDecoderDelete(image_handle);

    return rgb_buf;
}

/*********************************************************************
  Function:        divoom_libwebp_decode_get_image_rgb_buf
  Description:     从文件中解码出一帧RGB数据出来
  Input:           file_name   图像文件名
  Return:          RGB缓冲区
**********************************************************************/

uint8 *divoom_libwebp_decode_get_image_rgb_buf(char *file_name, int *width, int *high)
{
    uint8 *rgb_buf;
    Pdivoom_libwebp_decode_ctrl phandle = divoom_libwebp_decode_init(file_name);

    if(phandle == NULL)
    {
        return NULL;
    }
    *width = phandle->width;
    *high = phandle->high;
    rgb_buf = divoom_libwebp_decode_get_frame_buffer(phandle, NULL, NULL, NULL);
    divoom_libwebp_decode_exit(phandle);
    return rgb_buf;
}

/*********************************************************************
  Function:        divoom_libwebp_decode_init
  Description:     创建WEBP的解码句柄
  Input:           file_name   图像文件名
  Return:          解码句柄
**********************************************************************/

void *divoom_libwebp_decode_init(char *file_name)
{
    int file_len;
    int expected_file_len;
    Pdivoom_libwebp_decode_ctrl phandle;
    Pdivoom_libwebp_file_anim_ctrl file_anim_handle;
    divoom_libwebp_file_probe_result probe_result;
    WebPData webp_info;
    WebPAnimDecoderOptions options;
    WebPAnimInfo pic_info;
    WebPBitstreamFeatures features;
    VP8StatusCode status;

    if(file_name == NULL)
    {
        return NULL;
    }
    phandle = divoom_system_malloc(sizeof(divoom_libwebp_decode_ctrl));
    if(phandle == NULL)
    {
        return  NULL;
    }
    phandle->pre_frame_time = -1;
    phandle->backend_type = DIVOOM_LIBWEBP_BACKEND_LEGACY;

    /* 动画优先建立文件后端；静态图明确回落到原有整文件实现。 */
    file_anim_handle = divoom_libwebp_decode_file_anim_create(file_name,
        &probe_result);
    if(probe_result == DIVOOM_LIBWEBP_FILE_PROBE_ANIMATION)
    {
        phandle->backend_type = DIVOOM_LIBWEBP_BACKEND_FILE_ANIMATION;
        phandle->image_handle = file_anim_handle;
        phandle->file_len = (int)file_anim_handle->file_size;
        phandle->width = file_anim_handle->width;
        phandle->high = file_anim_handle->height;
        phandle->frame_num = file_anim_handle->frame_num;
        phandle->file_name = divoom_app_com_copy_string(file_name, 0);
        divoom_print("[webp] file animation: file=%s, len=%d, frames=%d, size=%dx%d, read_chunk=%u\n",
            file_name, phandle->file_len, phandle->frame_num, phandle->width,
            phandle->high, DIVOOM_LIBWEBP_FILE_READ_CHUNK_SIZE);
        return phandle;
    }
    if(probe_result == DIVOOM_LIBWEBP_FILE_PROBE_ERROR ||
        probe_result == DIVOOM_LIBWEBP_FILE_PROBE_CORRUPT)
    {
        divoom_system_free(phandle);
        divoom_libwebp_decode_handle_failure(file_name,
            probe_result == DIVOOM_LIBWEBP_FILE_PROBE_CORRUPT ?
            VP8_STATUS_BITSTREAM_ERROR : DIVOOM_LIBWEBP_STATUS_UNKNOWN);
        return NULL;
    }

    /* read_file returns bytes actually read, including a partial read on I/O failure. */
    expected_file_len = divoom_app_com_get_file_size(file_name);
    phandle->file_buf = divoom_app_com_read_file(file_name, &file_len);
    if(phandle->file_buf == NULL || expected_file_len <= 0 ||
        file_len != expected_file_len)
    {
        if(phandle->file_buf != NULL)
        {
            divoom_system_free(phandle->file_buf);
        }
        divoom_system_free(phandle);
        divoom_libwebp_decode_handle_failure(file_name, DIVOOM_LIBWEBP_STATUS_UNKNOWN);
        return  NULL;        
    }
    phandle->file_len = file_len;
    /* This is the complete file buffer; short headers are not an incremental read. */
    status = WebPGetFeatures(phandle->file_buf, (size_t)file_len, &features);
    if(status != VP8_STATUS_OK)
    {
        divoom_libwebp_decode_handle_failure(file_name, status);
        divoom_system_free(phandle->file_buf);
        divoom_system_free(phandle);
        return NULL;
    }
    //divoom_libwebp_print_features_status("init", file_name, phandle->file_buf,
    //    phandle->file_len);
    WebPDataInit(&webp_info);
    webp_info.bytes = phandle->file_buf;
    webp_info.size = file_len;
    WebPAnimDecoderOptionsInit(&options);
    options.color_mode = MODE_bgrA;
    options.use_threads = 0;
    phandle->image_handle = WebPAnimDecoderNew(&webp_info, &options);
    if(phandle->image_handle == NULL)
    {
        divoom_print("[webp] WebPAnimDecoderNew fail: file=%s, len=%d\n",
            file_name ? file_name : "NULL", phandle->file_len);
        divoom_system_print_ram_info(__FILE__, __LINE__);
        divoom_system_free(phandle->file_buf);
        divoom_system_free(phandle);
        /* New() does not expose its cause; a valid header does not prove corruption. */
        divoom_libwebp_decode_handle_failure(file_name, DIVOOM_LIBWEBP_STATUS_UNKNOWN);
        return  NULL;   
    }
    WebPAnimDecoderGetInfo(phandle->image_handle, &pic_info);
    phandle->high  = pic_info.canvas_height;
    phandle->width = pic_info.canvas_width;
    phandle->frame_num = pic_info.frame_count;
    //divoom_print("wepb color: %d\n", pic_info.bgcolor);
    if(phandle->frame_num == 0)
    {
        divoom_print("[webp] frame_num is 0: file=%s, len=%d, w=%d, h=%d\n",
            file_name ? file_name : "NULL", phandle->file_len,
            phandle->width, phandle->high);
        divoom_libwebp_decode_exit(phandle);
        divoom_libwebp_decode_handle_failure(file_name, VP8_STATUS_BITSTREAM_ERROR);
        return  NULL;   
    }
    phandle->file_name = divoom_app_com_copy_string(file_name, 0);

    return phandle;
}

void *divoom_libwebp_decode_init_file_region(char *file_name,
    uint32 file_offset, uint32 file_size)
{
    Pdivoom_libwebp_decode_ctrl phandle;
    Pdivoom_libwebp_file_anim_ctrl file_anim_handle;
    Pdivoom_libwebp_file_static_ctrl file_static_handle;
    divoom_libwebp_file_probe_result probe_result;

    if(file_name == NULL || file_size == 0U)
    {
        return NULL;
    }
    phandle = divoom_system_malloc(sizeof(divoom_libwebp_decode_ctrl));
    if(phandle == NULL)
    {
        return NULL;
    }
    phandle->pre_frame_time = -1;
    file_anim_handle = divoom_libwebp_decode_file_anim_create_region(
        file_name, file_offset, file_size, &probe_result);
    if(probe_result == DIVOOM_LIBWEBP_FILE_PROBE_ANIMATION)
    {
        phandle->backend_type = DIVOOM_LIBWEBP_BACKEND_FILE_ANIMATION;
        phandle->image_handle = file_anim_handle;
        phandle->width = file_anim_handle->width;
        phandle->high = file_anim_handle->height;
        phandle->frame_num = file_anim_handle->frame_num;
    }
    else if(probe_result == DIVOOM_LIBWEBP_FILE_PROBE_STATIC)
    {
        file_static_handle = divoom_libwebp_decode_file_static_create_region(
            file_name, file_offset, file_size);
        if(file_static_handle == NULL)
        {
            divoom_system_free(phandle);
            return NULL;
        }
        phandle->backend_type = DIVOOM_LIBWEBP_BACKEND_FILE_STATIC;
        phandle->image_handle = file_static_handle;
        phandle->width = file_static_handle->width;
        phandle->high = file_static_handle->height;
        phandle->frame_num = 1;
    }
    else
    {
        divoom_system_free(phandle);
        return NULL;
    }
    phandle->file_len = (int)file_size;
    return phandle;
}



/*********************************************************************
  Function:        divoom_libwebp_decode_get_frame_info
  Description:     获取动画的相关信息
  Input:           handle   解码句柄
                   width    图像宽度
                   high     图像高度
  Return:          返回图像帧数
**********************************************************************/

int divoom_libwebp_decode_get_frame_info(void *handle, int *width, int *high)
{
    Pdivoom_libwebp_decode_ctrl phandle = (Pdivoom_libwebp_decode_ctrl)handle;
    
    if(phandle == NULL)
    {
        divoom_print("handle is NULL!\n");
        return 0;
    }
    *width = phandle->width;
    *high = phandle->high;
    //divoom_print("pic info: w: %d, h: %d, num: %d!\n", phandle->width, phandle->high, phandle->frame_num);
    return phandle->frame_num;
}   

static int divoom_libwebp_decode_has_more_frames(
    Pdivoom_libwebp_decode_ctrl phandle)
{
    if(phandle->backend_type == DIVOOM_LIBWEBP_BACKEND_FILE_ANIMATION)
    {
        return divoom_libwebp_decode_file_anim_has_more_frames(
            (Pdivoom_libwebp_file_anim_ctrl)phandle->image_handle);
    }
    if(phandle->backend_type == DIVOOM_LIBWEBP_BACKEND_FILE_STATIC)
    {
        return divoom_libwebp_decode_file_static_has_more_frames(
            (Pdivoom_libwebp_file_static_ctrl)phandle->image_handle);
    }
    return WebPAnimDecoderHasMoreFrames(
        (WebPAnimDecoder *)phandle->image_handle);
}

static void divoom_libwebp_decode_reset_frames(
    Pdivoom_libwebp_decode_ctrl phandle)
{
    if(phandle->backend_type == DIVOOM_LIBWEBP_BACKEND_FILE_ANIMATION)
    {
        divoom_libwebp_decode_file_anim_reset(
            (Pdivoom_libwebp_file_anim_ctrl)phandle->image_handle);
    }
    else if(phandle->backend_type == DIVOOM_LIBWEBP_BACKEND_FILE_STATIC)
    {
        divoom_libwebp_decode_file_static_reset(
            (Pdivoom_libwebp_file_static_ctrl)phandle->image_handle);
    }
    else
    {
        WebPAnimDecoderReset((WebPAnimDecoder *)phandle->image_handle);
    }
}

static int divoom_libwebp_decode_get_next_frame(
    Pdivoom_libwebp_decode_ctrl phandle, uint8 **out_buf,
    int32 *frame_timestamp)
{
    int result;
    VP8StatusCode status;

    if(phandle->backend_type == DIVOOM_LIBWEBP_BACKEND_FILE_ANIMATION)
    {
        Pdivoom_libwebp_file_anim_ctrl file_handle = phandle->image_handle;
        result = divoom_libwebp_decode_file_anim_get_next(file_handle,
            out_buf, frame_timestamp);
        status = file_handle->last_status;
    }
    else if(phandle->backend_type == DIVOOM_LIBWEBP_BACKEND_FILE_STATIC)
    {
        Pdivoom_libwebp_file_static_ctrl file_handle = phandle->image_handle;
        result = divoom_libwebp_decode_file_static_get_next(file_handle,
            out_buf, frame_timestamp);
        status = file_handle->last_status;
    }
    else
    {
        result = WebPAnimDecoderGetNext(
            (WebPAnimDecoder *)phandle->image_handle, out_buf,
            (int *)frame_timestamp);
#ifdef DIVOOM_SIMULATOR
        /* 仿真链接的 vcpkg WebP 无真机扩展状态接口；失败原因未知时保留资源，避免误删。 */
        status = result ? VP8_STATUS_OK : DIVOOM_LIBWEBP_STATUS_UNKNOWN;
#else
        status = WebPAnimDecoderGetLastStatus(
            (WebPAnimDecoder *)phandle->image_handle);
#endif
    }
    if(!result)
    {
        divoom_libwebp_decode_handle_failure(phandle->file_name, status);
    }
    return result;
}

/*********************************************************************
  Function:        divoom_libwebp_decode_pass_some_frame
  Description:     跳过一些帧
  Input:           handle   解码句柄
                   frame_num  跳过的帧数
  Return:          none
**********************************************************************/

int divoom_libwebp_decode_seek_frame(void *handle, int frame)
{
    Pdivoom_libwebp_decode_ctrl phandle = handle;
    uint8 *pixels;
    int i, timestamp;
    if(!phandle || frame < 0 || frame >= phandle->frame_num)
        return 0;

    divoom_libwebp_decode_reset_frames(phandle);
    phandle->pre_frame_time = -1;
    for(i = 0; i < frame; i++)
    {
        if(!divoom_libwebp_decode_get_next_frame(phandle, &pixels, &timestamp))
            return 0;
        phandle->pre_frame_time = timestamp;
    }
    return 1;
}

void divoom_libwebp_decode_pass_some_frame(void *handle,  int frame_num)
{
    int i, frame_time;
    uint8 *out_buf;
    Pdivoom_libwebp_decode_ctrl phandle = (Pdivoom_libwebp_decode_ctrl)handle;
    
    if(phandle == NULL)
    {
        return ;
    }
    for(i = 0; i < frame_num; i++)
    {
        if(!divoom_libwebp_decode_has_more_frames(phandle))
        {
            // If there are no more frames, rewind to the beginning
            divoom_libwebp_decode_reset_frames(phandle);
            return;
        }
        out_buf = NULL;
        if(!divoom_libwebp_decode_get_next_frame(phandle, &out_buf,
            (int32 *)&frame_time))
        {
            return;
        }
        phandle->pre_frame_time = frame_time;
    }
}

/*********************************************************************
  Function:        divoom_libwebp_decode_get_frame_buffer
  Description:     得到帧显示缓冲区，如果是动画可以循环调用此函数，它将自动循环，
                   注意这个返回的BUFFER，要外面进行释放，否则会有内存泄露
  Input:           handle   解码句柄
                   frame_time  这帧持续时间
  Return:          返回图像帧缓冲区
**********************************************************************/

uint8 *divoom_libwebp_decode_get_frame_buffer(void *handle,  int *frame_time, int *end_flag, uint8 *sys_buf)
{
    int cur_frame_time;
    uint8 *out_buf, *new_buf;
    Pdivoom_libwebp_decode_ctrl phandle = (Pdivoom_libwebp_decode_ctrl)handle;
    
    if(phandle == NULL)
    {
        return NULL;
    }
    if(!divoom_libwebp_decode_has_more_frames(phandle))
    {
        // If there are no more frames, rewind to the beginning
        phandle->pre_frame_time = -1;
        divoom_libwebp_decode_reset_frames(phandle);
        if(end_flag)
        {
            *end_flag = 1;
        }
    }
    else
    {
        if(end_flag)
        {
            *end_flag = 0;
        }
    }
    if(!divoom_libwebp_decode_get_next_frame(phandle, &out_buf,
        (int32 *)&cur_frame_time))
    {
        divoom_print("Error decoding frame.\n");
        if(sys_buf == NULL)
        {
            new_buf = (uint8 *)divoom_system_malloc_no_zero(phandle->width * phandle->high * 4);
        }
        else
        {
            new_buf = sys_buf;
        }
        return new_buf;
    }
    if(frame_time)
    {
        if(phandle->pre_frame_time == -1)
        {
            *frame_time = cur_frame_time;
        }
        else
        {
            *frame_time = cur_frame_time - phandle->pre_frame_time;
        }
    }
    phandle->pre_frame_time = cur_frame_time;
    if(sys_buf == NULL)
    {
        new_buf = (uint8 *)divoom_system_malloc_no_zero(phandle->width * phandle->high * 4);
    }
    else
    {
        new_buf = sys_buf;
    }
    if(new_buf)
    {
        memcpy(new_buf, out_buf, phandle->width *phandle->high*4);
    }
    return new_buf;
}

uint8 *divoom_libwebp_decode_get_frame_rgb565a8_buffer(void *handle, int *frame_time, int *end_flag, uint8 *sys_buf)
{
    int cur_frame_time;
    int px_count;
    uint8 *out_buf;
    uint8 *new_buf;
    Pdivoom_libwebp_decode_ctrl phandle = (Pdivoom_libwebp_decode_ctrl)handle;

    if(phandle == NULL)
    {
        return NULL;
    }
    if(!divoom_libwebp_decode_has_more_frames(phandle))
    {
        phandle->pre_frame_time = -1;
        divoom_libwebp_decode_reset_frames(phandle);
        if(end_flag)
        {
            *end_flag = 1;
        }
    }
    else
    {
        if(end_flag)
        {
            *end_flag = 0;
        }
    }
    if(!divoom_libwebp_decode_get_next_frame(phandle, &out_buf,
        (int32 *)&cur_frame_time))
    {
        divoom_print("[webp] Error decoding frame: frame=%d/%d, size=%dx%d, file=%s, len=%d\n",
            phandle->frame_id, phandle->frame_num, phandle->width, phandle->high,
            phandle->file_name ? phandle->file_name : "NULL", phandle->file_len);
        if(phandle->backend_type == DIVOOM_LIBWEBP_BACKEND_LEGACY)
        {
            divoom_libwebp_print_features_status("get_next_fail",
                phandle->file_name, phandle->file_buf, phandle->file_len);
        }
        else
        {
            Pdivoom_libwebp_file_anim_ctrl file_anim_handle =
                (Pdivoom_libwebp_file_anim_ctrl)phandle->image_handle;

            divoom_print("[webp] file animation read max=%u bytes\n",
                file_anim_handle->max_read_size);
        }
        divoom_system_print_ram_info(__FILE__, __LINE__);
        return NULL;
    }
    if(frame_time)
    {
        if(phandle->pre_frame_time == -1)
        {
            *frame_time = cur_frame_time;
        }
        else
        {
            *frame_time = cur_frame_time - phandle->pre_frame_time;
        }
    }
    phandle->pre_frame_time = cur_frame_time;
    px_count = phandle->width * phandle->high;
    if(sys_buf == NULL)
    {
        new_buf = (uint8 *)divoom_system_malloc_no_zero(px_count * 3);
    }
    else
    {
        new_buf = sys_buf;
    }
    if(new_buf)
    {
        divoom_libwebp_bgra_to_rgb565a8_buf(out_buf, new_buf, px_count);
    }
    phandle->frame_id ++;
    return new_buf;
}

uint8 *divoom_libwebp_decode_get_frame_rgb565_buffer(void *handle, int *frame_time, int *end_flag, uint8 *sys_buf)
{
    int cur_frame_time;
    int px_count;
    uint8 *out_buf;
    uint8 *new_buf;
    Pdivoom_libwebp_decode_ctrl phandle = (Pdivoom_libwebp_decode_ctrl)handle;

    if(phandle == NULL)
    {
        return NULL;
    }
    if(!divoom_libwebp_decode_has_more_frames(phandle))
    {
        phandle->pre_frame_time = -1;
        divoom_libwebp_decode_reset_frames(phandle);
        if(end_flag)
        {
            *end_flag = 1;
        }
    }
    else
    {
        if(end_flag)
        {
            *end_flag = 0;
        }
    }
    if(!divoom_libwebp_decode_get_next_frame(phandle, &out_buf, (int32 *)&cur_frame_time))
    {
        divoom_print("[webp] Error decoding frame: frame=%d/%d, size=%dx%d, file=%s, len=%d\n",
            phandle->frame_id, phandle->frame_num, phandle->width, phandle->high,
            phandle->file_name ? phandle->file_name : "NULL", phandle->file_len);
        if(phandle->backend_type == DIVOOM_LIBWEBP_BACKEND_LEGACY)
        {
            divoom_libwebp_print_features_status("get_next_fail", phandle->file_name, phandle->file_buf, phandle->file_len);
        }
        else
        {
            Pdivoom_libwebp_file_anim_ctrl file_anim_handle = (Pdivoom_libwebp_file_anim_ctrl)phandle->image_handle;

            divoom_print("[webp] file animation read max=%u bytes\n", file_anim_handle->max_read_size);
        }
        divoom_system_print_ram_info(__FILE__, __LINE__);
        return NULL;
    }
    if(frame_time)
    {
        if(phandle->pre_frame_time == -1)
        {
            *frame_time = cur_frame_time;
        }
        else
        {
            *frame_time = cur_frame_time - phandle->pre_frame_time;
        }
    }
    phandle->pre_frame_time = cur_frame_time;
    px_count = phandle->width * phandle->high;
    if(sys_buf == NULL)
    {
        new_buf = (uint8 *)divoom_system_malloc_no_zero(px_count * 2);
    }
    else
    {
        new_buf = sys_buf;
    }
    if(new_buf)
    {
        divoom_libwebp_bgra_to_rgb565_buf(out_buf, new_buf, px_count);
    }
    phandle->frame_id ++;
    return new_buf;
}

/*********************************************************************
  Function:        divoom_libwebp_decode_get_rgb_buffer
  Description:     得到帧显示缓冲区，如果是动画可以循环调用此函数，它将自动循环，
                   
  Input:           handle   解码句柄
                   frame_time  这帧持续时间
                   end_flag
                   rgb_buf
  Return:          成功返回1， 否则返回0
**********************************************************************/

int divoom_libwebp_decode_get_rgb_buffer(void *handle,  int *frame_time, int *end_flag, uint8 *rgb_buf)
{
    int cur_frame_time;
    uint8 *out_buf;
    Pdivoom_libwebp_decode_ctrl phandle = (Pdivoom_libwebp_decode_ctrl)handle;
    
    if(phandle == NULL || rgb_buf == NULL)
    {
        return 0;
    }
    if(!divoom_libwebp_decode_has_more_frames(phandle))
    {
        // If there are no more frames, rewind to the beginning
        phandle->pre_frame_time = -1;
        divoom_libwebp_decode_reset_frames(phandle);
        if(end_flag)
        {
            *end_flag = 1;
        }
    }
    else
    {
        if(end_flag)
        {
            *end_flag = 0;
        }
    }
    if(!divoom_libwebp_decode_get_next_frame(phandle, &out_buf,
        (int32 *)&cur_frame_time))
    {
        divoom_print("Error decoding frame.\n");
        return 0;
    }
    if(frame_time)
    {
        if(phandle->pre_frame_time == -1)
        {
            *frame_time = cur_frame_time;
        }
        else
        {
            *frame_time = cur_frame_time - phandle->pre_frame_time;
        }
    }
    phandle->pre_frame_time = cur_frame_time;
    memcpy(rgb_buf, out_buf, phandle->width *phandle->high*4);
    return 1;
}

/*********************************************************************
  Function:        divoom_libwebp_decode_exit
  Description:     释放解码句柄，释放所有资源
  Input:           handle   解码句柄
  Return:          none
**********************************************************************/

void divoom_libwebp_decode_exit(void *handle)
{
    Pdivoom_libwebp_decode_ctrl phandle = (Pdivoom_libwebp_decode_ctrl)handle;

    if(phandle == NULL)
    {
        return;
    }

    if(phandle->backend_type == DIVOOM_LIBWEBP_BACKEND_LEGACY)
    {
        divoom_system_free(phandle->file_buf);
        WebPAnimDecoderDelete((WebPAnimDecoder *)phandle->image_handle);
    }
    else if(phandle->backend_type == DIVOOM_LIBWEBP_BACKEND_FILE_ANIMATION)
    {
        divoom_libwebp_decode_file_anim_delete(
            (Pdivoom_libwebp_file_anim_ctrl)phandle->image_handle);
    }
    else
    {
        divoom_libwebp_decode_file_static_delete(
            (Pdivoom_libwebp_file_static_ctrl)phandle->image_handle);
    }
    if(phandle->file_name != NULL)
    {
        divoom_system_free(phandle->file_name);
    }
    divoom_system_free(phandle);
}

/*********************************************************************
  Function:        divoom_libwebp_encode_image
  Description:     将图像缓冲区编码成webp格式的图像
  Input:           file_name   图像文件名
                   image_buf   图像缓冲区
                   image_width
                   image_high
  Return:          解码句柄
**********************************************************************/

int divoom_libwebp_encode_image(char *file_name, uint8 *image_buf, int image_width, int image_high)
{
    int out_size;
    uint8 *out_buf= NULL;
    
    if(image_buf == NULL)
    {
        return 0;
    }
    out_size = WebPEncodeBGRA(image_buf, image_width, image_high, image_width*4, 80, &out_buf);
    if(out_size > 0 && out_buf != NULL)
    {
        divoom_app_com_write_file(file_name, out_buf, out_size);
        free(out_buf);
        return divoom_app_com_get_file_size(file_name) > 0 ? 1 : 0;
    }
    return 0;
}
