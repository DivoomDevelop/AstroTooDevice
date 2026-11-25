/*
 * F101 PNG hardware decode through the Cedarc pngdecode object API.
 */

#include "divoom.h"
#include "middle/include/divoom_aw_pic_decode.h"
#include "lvgl/include/divoom_lvgl_init.h"
#include "divoom_pixel_format.h"
#include <string.h>

uint8 divoom_aw_parse_png_ihdr_info(const char *buf, int buf_len, int *width, int *height)
{
    const unsigned char *p;
    int w;
    int h;

    if (buf == NULL || buf_len < 24 || width == NULL || height == NULL)
    {
        return 0;
    }

    p = (const unsigned char *)buf;
    if (p[0] != 0x89 || p[1] != 'P' || p[2] != 'N' || p[3] != 'G' ||
        p[12] != 'I' || p[13] != 'H' || p[14] != 'D' || p[15] != 'R')
    {
        return 0;
    }

    w = (int)(((uint32)p[16] << 24) | ((uint32)p[17] << 16) |
              ((uint32)p[18] << 8) | (uint32)p[19]);
    h = (int)(((uint32)p[20] << 24) | ((uint32)p[21] << 16) |
              ((uint32)p[22] << 8) | (uint32)p[23]);
    if (w <= 0 || h <= 0 || w > 8192 || h > 8192)
    {
        return 0;
    }

    *width = w;
    *height = h;
    return 1;
}

static uint8 divoom_aw_check_png_lvfmt_out(int width, int height,
    uint8 *out_buf, uint32 out_size, lv_color_format_t cf, uint32 *px_count)
{
    uint32 count;
    uint32 need_size;

    if (width <= 0 || height <= 0 || out_buf == NULL || px_count == NULL ||
        (cf != LV_COLOR_FORMAT_RGB565 && cf != LV_COLOR_FORMAT_RGB565A8))
    {
        return 0;
    }
    count = (uint32)width * (uint32)height;
    if (count == 0 || count > (UINT32_MAX / DIVOOM_APP_RGB565A8_BYTES_PER_PX))
    {
        return 0;
    }
    need_size = divoom_pixel_format_size((uint32)width, (uint32)height, cf);
    if (need_size == 0 || need_size > out_size)
    {
        return 0;
    }
    *px_count = count;
    return 1;
}

static uint8 divoom_aw_png_prepare_out_buf(divoom_aw_png_decode_buf_t *reuse,
    uint32 need_size, uint8 **out_buf)
{
    uint8 *buf;

    if (out_buf == NULL || need_size == 0)
    {
        return 0;
    }

    if (reuse == NULL)
    {
        buf = (uint8 *)divoom_system_malloc(need_size);
        if (buf == NULL)
        {
            return 0;
        }
        *out_buf = buf;
        return 1;
    }

    if (reuse->out_buf != NULL && reuse->out_buf_size >= need_size)
    {
        *out_buf = reuse->out_buf;
        return 1;
    }

    if (reuse->out_buf != NULL && !reuse->out_buf_owner)
    {
        return 0;
    }

    if (reuse->out_buf != NULL)
    {
        divoom_system_free(reuse->out_buf);
        reuse->out_buf = NULL;
        reuse->out_buf_size = 0;
        reuse->out_buf_owner = 0;
    }

    reuse->out_buf = (uint8 *)divoom_system_malloc(need_size);
    if (reuse->out_buf == NULL)
    {
        return 0;
    }
    reuse->out_buf_size = need_size;
    reuse->out_buf_owner = 1;
    *out_buf = reuse->out_buf;
    return 1;
}

#ifndef DIVOOM_SIMULATOR
#include <pngdecode.h>

#define DIVOOM_PNG_DECODE_MAX_TRIES 8
#define DIVOOM_PNG_DECODE_MIN_LARGEST_FREE_KB 256
#define DIVOOM_PNG_DECODE_RETRY_DELAY_MS 500

static uint32 g_divoom_aw_png_retry_after_ms;

/* 仅在图片解码锁内更新；手动查询，不在正常逐帧路径打印。 */
typedef struct
{
    uint32 calls;
    uint32 success;
    uint32 failed;
    int32 width;
    int32 height;
    uint32 input_bytes;
    uint32 vbv_bytes;
} divoom_aw_png_diag_t;
static divoom_aw_png_diag_t g_divoom_aw_png_diag;

#include <console.h>
static int32 divoom_aw_png_stat_command(int32 argc, char **argv)
{
    divoom_aw_png_diag_t snapshot;
    (void)argc;
    (void)argv;
    if (divoom_aw_pic_decoder_lock() != 0)
    {
        return -1;
    }
    snapshot = g_divoom_aw_png_diag;
    divoom_aw_pic_decoder_unlock();
    divoom_print_err("[PNG_STAT] calls=%u success=%u failed=%u width=%d height=%d input=%u vbv=%u free_kb=%ld largest_kb=%ld\n",
        snapshot.calls, snapshot.success, snapshot.failed,
        snapshot.width, snapshot.height, snapshot.input_bytes, snapshot.vbv_bytes,
        divoom_system_get_ram_free_size(),
        divoom_system_get_ram_largest_free_block_size());
    return 0;
}
FINSH_FUNCTION_EXPORT_CMD(divoom_aw_png_stat_command, png_stat, PNG decode diagnostics);


void divoom_aw_png_decode_buf_release(divoom_aw_png_decode_buf_t *reuse)
{
    if (reuse == NULL)
    {
        return;
    }

    if (reuse->out_buf != NULL && reuse->out_buf_owner)
    {
        divoom_system_free(reuse->out_buf);
    }
    memset(reuse, 0, sizeof(*reuse));
}

static uint8 *divoom_aw_copy_png_rgba(const VideoPicture *picture,
    int width, int height)
{
    uint8 *out_buf;
    int y;

    if (picture == NULL || picture->pData0 == NULL ||
        picture->nLineStride < width || picture->nHeight < height)
    {
        return NULL;
    }

    out_buf = (uint8 *)divoom_system_malloc((uint32)((size_t)width * (size_t)height * 4u));
    if (out_buf == NULL)
    {
        return NULL;
    }

    /*
     * sunxi PNG 解码器输出的是 32-bit RGBA word；在 F101 小端内存里实际字节为
     * A,B,G,R。divoom_aw_decode_png_buf 的调用方按 R,G,B,A 读取并再转 RGB565/
     * RGB565A8，因此这里必须显式重排通道，不能直接照搬 LVGL 文件解码缓存。
     */
    for (y = 0; y < height; y++)
    {
        const uint8 *src_row = (const uint8 *)picture->pData0 +
            ((size_t)(picture->nTopOffset + y) * (size_t)picture->nLineStride +
             (size_t)picture->nLeftOffset) * 4u;
        uint8 *dst_row = out_buf + (size_t)y * (size_t)width * 4u;
        int x;

        for (x = 0; x < width; x++)
        {
            const uint8 *src = src_row + (size_t)x * 4u;
            uint8 *dst = dst_row + (size_t)x * 4u;

            dst[0] = src[3];
            dst[1] = src[2];
            dst[2] = src[1];
            dst[3] = src[0];
        }
    }
    return out_buf;
}

static uint8 divoom_aw_copy_png_to_lvfmt(const VideoPicture *picture,
    int width, int height, uint8 *out_buf, uint32 out_size, lv_color_format_t cf)
{
    uint32 px_count;
    uint8 *alpha;
    int y;

    if (picture == NULL || picture->pData0 == NULL ||
        picture->nLineStride < width || picture->nHeight < height ||
        !divoom_aw_check_png_lvfmt_out(width, height, out_buf, out_size, cf, &px_count))
    {
        return 0;
    }

    alpha = (cf == LV_COLOR_FORMAT_RGB565A8) ?
        (out_buf + px_count * DIVOOM_APP_RGB565_BYTES_PER_PX) : NULL;
    /*
     * 硬解输出按小端 ABGR 字节排布。这里直接写入调用方 cache buffer，
     * 避免先生成整张 RGBA 再由 Native Lua 二次转换。
     */
    for (y = 0; y < height; y++)
    {
        const uint8 *src_row = (const uint8 *)picture->pData0 +
            ((size_t)(picture->nTopOffset + y) * (size_t)picture->nLineStride +
             (size_t)picture->nLeftOffset) * 4u;
        int x;

        for (x = 0; x < width; x++)
        {
            const uint8 *src = src_row + (size_t)x * 4u;
            uint32 dst_idx = (uint32)y * (uint32)width + (uint32)x;
            uint16 rgb565 = divoom_rgb888_to_rgb565(src[3], src[2], src[1]);

            divoom_store_rgb565_le(out_buf + dst_idx * DIVOOM_APP_RGB565_BYTES_PER_PX,
                rgb565);
            if (alpha != NULL)
            {
                alpha[dst_idx] = src[0];
            }
        }
    }
    return 1;
}

static uint8 *divoom_aw_decode_png_buf_internal(const char *buf, int buf_len,
    int *width, int *height, uint8 **free_before_out_buf,
    divoom_aw_png_frame_callback *frame_callback, void *user_data,
    int *frame_result)
{
    PngDecoder *decoder = NULL;
    VideoPicture *picture = NULL;
    uint8 *frame_buffer = NULL;
    int png_w;
    int png_h;
    int ret;
    int try_count;
    int decode_ok = 0;
    int decoder_locked = 0;
    uint8 *out_buf = NULL;

    if (buf == NULL || buf_len <= 0 || width == NULL || height == NULL)
    {
        return NULL;
    }
    if (!divoom_aw_parse_png_ihdr_info(buf, buf_len, &png_w, &png_h))
    {
        divoom_print_err("png ihdr parse fail\n");
        return NULL;
    }

    if (divoom_aw_pic_decoder_lock() != 0)
    {
        return NULL;
    }
    decoder_locked = 1;
    g_divoom_aw_png_diag.calls++;
    g_divoom_aw_png_diag.width = png_w;
    g_divoom_aw_png_diag.height = png_h;
    g_divoom_aw_png_diag.input_bytes = (uint32)buf_len;
    g_divoom_aw_png_diag.vbv_bytes = 0;

    decoder = PngDecoderCreate();
    if (decoder == NULL)
    {
        divoom_print_err("PngDecoderCreate fail\n");
        goto cleanup;
    }

    PngDecoderSetDataSourceBuf(decoder, (char *)buf, buf_len);
    if (PngDecoderInitial(decoder, (uint32)png_w, (uint32)png_h) < 0)
    {
        divoom_print_err("PNG init failed %dx%d input=%d vbv=%d free=%ldKB largest=%ldKB\n",
            png_w, png_h, buf_len, ((PngDecoderContext *)decoder)->vConfig.nVbvBufferSize,
            divoom_system_get_ram_free_size(),
            divoom_system_get_ram_largest_free_block_size());
        goto cleanup;
    }
    if (PngDecoderSubmit(decoder) != 0)
    {
        divoom_print_err("PngDecoderSubmit fail w=%d h=%d len=%d\n",
                     png_w, png_h, buf_len);
        goto cleanup;
    }

    for (try_count = 0; try_count < DIVOOM_PNG_DECODE_MAX_TRIES; try_count++)
    {
        picture = NULL;
        ret = PngDecoderFrameBuffer(decoder, &picture);
        if (ret == VDECODE_RESULT_NO_FRAME_BUFFER)
        {
            if (frame_buffer == NULL)
            {
                frame_buffer = PngAllocateFrameBuffer(decoder, NULL,
                    (uint32)png_w * (uint32)png_h * 4U);
                if (frame_buffer == NULL ||
                    PngSetFrameBufferToVdec(decoder, frame_buffer) < 0)
                {
                    divoom_print_err("png frame buffer setup fail w=%d h=%d\n",
                                 png_w, png_h);
                    goto cleanup;
                }
            }
            continue;
        }

        if (ret == VDECODE_RESULT_OK ||
            ret == VDECODE_RESULT_KEYFRAME_DECODED ||
            ret == VDECODE_RESULT_FRAME_DECODED)
        {
            if (picture == NULL)
            {
                continue;
            }
            if (picture->pData0 == NULL ||
                picture->nLeftOffset < 0 || picture->nTopOffset < 0 ||
                picture->nLineStride < png_w ||
                picture->nLeftOffset + png_w > picture->nLineStride ||
                picture->nTopOffset + png_h > picture->nHeight)
            {
                divoom_print_err("invalid png picture w=%d h=%d stride=%d\n",
                             picture->nWidth, picture->nHeight,
                             picture->nLineStride);
                goto cleanup;
            }
            decode_ok = 1;
            break;
        }

        divoom_print_err("PngDecoderFrameBuffer fail ret=%d\n", ret);
        goto cleanup;
    }

    if (!decode_ok)
    {
        divoom_print_err("png decode timed out w=%d h=%d\n", png_w, png_h);
        goto cleanup;
    }

    CdcMemFlushCache(((PngDecoderContext *)decoder)->memops,
        picture->pData0, picture->nLineStride * picture->nHeight * 4);
    /*
     * 文件入口的压缩数据到这里已经完成解码，后续仅从 VideoPicture 拷贝。
     * 先释放 file_buf，再申请 RGBA 输出缓冲，避免两块大内存同时占用。
     */
    if (free_before_out_buf != NULL && *free_before_out_buf != NULL)
    {
        divoom_system_free(*free_before_out_buf);
        *free_before_out_buf = NULL;
    }
    if (frame_callback != NULL)
    {
        /* LVGL 直接拷贝硬件 ABGR 帧，避免再申请整幅 RGBA 中转缓冲。 */
        const uint8 *abgr = (const uint8 *)picture->pData0 +
            ((uint32)picture->nTopOffset * (uint32)picture->nLineStride +
             (uint32)picture->nLeftOffset) * 4U;
        *frame_result = frame_callback(abgr, png_w, png_h,
            picture->nLineStride * 4, user_data);
        if (*frame_result == 0)
        {
            *width = png_w;
            *height = png_h;
        }
    }
    else
    {
        out_buf = divoom_aw_copy_png_rgba(picture, png_w, png_h);
    }

cleanup:
    if (picture != NULL)
    {
        PngDecoderReturnFrameBuffer(decoder, picture);
    }
    if (frame_buffer != NULL)
    {
        PngReleaseFrameBufferToVdec(decoder);
        PngFreeFrameBuffer(decoder, frame_buffer);
    }
    if (decoder != NULL)
    {
        g_divoom_aw_png_diag.vbv_bytes =
            (uint32)((PngDecoderContext *)decoder)->vConfig.nVbvBufferSize;
        PngDecoderDestory(decoder);
    }
    if (decoder_locked)
    {
        if ((frame_callback != NULL && *frame_result == 0) || out_buf != NULL)
        {
            g_divoom_aw_png_diag.success++;
        }
        else
        {
            g_divoom_aw_png_diag.failed++;
        }
        divoom_aw_pic_decoder_unlock();
    }
    if (frame_callback != NULL)
    {
        return NULL;
    }
    if (out_buf == NULL)
    {
        divoom_print_err("png frame copy fail w=%d h=%d\n", png_w, png_h);
        return NULL;
    }

    *width = png_w;
    *height = png_h;
    return out_buf;
}

uint8 *divoom_aw_decode_png_buf(const char *buf, int buf_len, int *width, int *height)
{
    return divoom_aw_decode_png_buf_internal(buf, buf_len, width, height, NULL, NULL, NULL, NULL);
}

/* 实现 LVGL 已预留的弱符号入口：共用锁、解码和完整释放路径。 */
int divoom_aw_png_decode_shared(const char *buf, int buf_len,
    int *width, int *height, divoom_aw_png_frame_callback *frame_callback,
    void *user_data)
{
    int frame_result = -1;

    if (frame_callback == NULL)
    {
        return -1;
    }
    divoom_aw_decode_png_buf_internal(buf, buf_len, width, height, NULL,
        frame_callback, user_data, &frame_result);
    return frame_result;
}

static uint8 *divoom_aw_decode_png_buf_to_lvfmt_reuse_internal(const char *buf, int buf_len,
    int *width, int *height, divoom_aw_png_decode_buf_t *reuse,
    lv_color_format_t cf, uint8 **free_before_out_buf)
{
    PngDecoder *decoder = NULL;
    VideoPicture *picture = NULL;
    uint8 *frame_buffer = NULL;
    uint8 *out_buf = NULL;
    uint32 out_size;
    uint32 px_count;
    int png_w;
    int png_h;
    int ret;
    int try_count;
    int decode_ok = 0;
    int decoder_locked = 0;
    int decoder_attempted = 0;
    uint8 success = 0;
    uint32 now_ms;
    long largest_free_kb;

    if (buf == NULL || buf_len <= 0 || width == NULL || height == NULL ||
        reuse == NULL)
    {
        return NULL;
    }
    if (!divoom_aw_parse_png_ihdr_info(buf, buf_len, &png_w, &png_h))
    {
        divoom_print("png ihdr parse fail\n");
        return NULL;
    }
    now_ms = divoom_system_get_tickms();
    if (g_divoom_aw_png_retry_after_ms != 0 &&
        (int32)(now_ms - g_divoom_aw_png_retry_after_ms) < 0)
    {
        return NULL;
    }
    g_divoom_aw_png_retry_after_ms = 0;
    largest_free_kb = divoom_system_get_ram_largest_free_block_size();
    if (largest_free_kb < DIVOOM_PNG_DECODE_MIN_LARGEST_FREE_KB)
    {
        g_divoom_aw_png_retry_after_ms =
            now_ms + DIVOOM_PNG_DECODE_RETRY_DELAY_MS;
        divoom_print("png decode deferred: largest=%ldKB need>=%dKB\n",
            largest_free_kb, DIVOOM_PNG_DECODE_MIN_LARGEST_FREE_KB);
        return NULL;
    }
    if (divoom_aw_pic_decoder_lock() != 0)
    {
        return NULL;
    }
    decoder_locked = 1;

    decoder_attempted = 1;
    decoder = PngDecoderCreate();
    if (decoder == NULL)
    {
        divoom_print("PngDecoderCreate fail\n");
        goto cleanup;
    }

    PngDecoderSetDataSourceBuf(decoder, (char *)buf, buf_len);
    if (PngDecoderInitial(decoder, (uint32)png_w, (uint32)png_h) < 0)
    {
        divoom_print_err("PNG init failed %dx%d input=%d vbv=%d free=%ldKB largest=%ldKB\n",
            png_w, png_h, buf_len, ((PngDecoderContext *)decoder)->vConfig.nVbvBufferSize,
            divoom_system_get_ram_free_size(),
            divoom_system_get_ram_largest_free_block_size());
        goto cleanup;
    }
    if (PngDecoderSubmit(decoder) != 0)
    {
        divoom_print("PngDecoderSubmit fail w=%d h=%d len=%d\n",
                     png_w, png_h, buf_len);
        goto cleanup;
    }

    for (try_count = 0; try_count < DIVOOM_PNG_DECODE_MAX_TRIES; try_count++)
    {
        picture = NULL;
        ret = PngDecoderFrameBuffer(decoder, &picture);
        if (ret == VDECODE_RESULT_NO_FRAME_BUFFER)
        {
            if (frame_buffer == NULL)
            {
                /*
                 * PngAllocateFrameBuffer 返回的是 ImgFrameBuffer 元数据指针，
                 * 不是可跨 decoder 复用的像素缓冲。这里必须让 pngdecode
                 * 自己为当前 decoder 分配/释放 frame buffer；上层只复用最终
                 * RGB565A8 out_buf，避免把元数据指针误当大图缓冲写坏系统堆。
                 */
                frame_buffer = PngAllocateFrameBuffer(decoder, NULL,
                    (uint32)png_w * (uint32)png_h * 4U);
                if (frame_buffer == NULL ||
                    PngSetFrameBufferToVdec(decoder, frame_buffer) < 0)
                {
                    divoom_print("png frame buffer setup fail w=%d h=%d\n",
                                 png_w, png_h);
                    goto cleanup;
                }
            }
            continue;
        }

        if (ret == VDECODE_RESULT_OK ||
            ret == VDECODE_RESULT_KEYFRAME_DECODED ||
            ret == VDECODE_RESULT_FRAME_DECODED)
        {
            if (picture == NULL)
            {
                continue;
            }
            if (picture->pData0 == NULL ||
                picture->nLeftOffset < 0 || picture->nTopOffset < 0 ||
                picture->nLineStride < png_w ||
                picture->nLeftOffset + png_w > picture->nLineStride ||
                picture->nTopOffset + png_h > picture->nHeight)
            {
                divoom_print("invalid png picture w=%d h=%d stride=%d\n",
                             picture->nWidth, picture->nHeight,
                             picture->nLineStride);
                goto cleanup;
            }
            decode_ok = 1;
            break;
        }

        divoom_print("PngDecoderFrameBuffer fail ret=%d\n", ret);
        goto cleanup;
    }

    if (!decode_ok)
    {
        divoom_print("png decode timed out w=%d h=%d\n", png_w, png_h);
        goto cleanup;
    }

    CdcMemFlushCache(((PngDecoderContext *)decoder)->memops,
        picture->pData0, picture->nLineStride * picture->nHeight * 4);
    /*
     * 文件入口传入的 PNG 压缩数据只服务到硬解完成。后续只是把 VideoPicture
     * 拷贝成 RGB565A8，可以先释放文件缓冲，降低 file + work + out 三块同时存在的峰值。
     */
    if (free_before_out_buf != NULL && *free_before_out_buf != NULL)
    {
        divoom_system_free(*free_before_out_buf);
        *free_before_out_buf = NULL;
    }
    out_size = divoom_pixel_format_size((uint32)png_w, (uint32)png_h, cf);
    if (!divoom_aw_png_prepare_out_buf(reuse, out_size, &out_buf) ||
        !divoom_aw_check_png_lvfmt_out(png_w, png_h, out_buf, out_size, cf,
            &px_count))
    {
        divoom_print("png out buf invalid w=%d h=%d size=%u\n", png_w, png_h,
            out_size);
        goto cleanup;
    }
    (void)px_count;
    if (!divoom_aw_copy_png_to_lvfmt(picture, png_w, png_h, out_buf, out_size, cf))
    {
        goto cleanup;
    }
    *width = png_w;
    *height = png_h;
    success = 1;

cleanup:
    if (picture != NULL)
    {
        PngDecoderReturnFrameBuffer(decoder, picture);
    }
    if (frame_buffer != NULL)
    {
        PngReleaseFrameBufferToVdec(decoder);
        PngFreeFrameBuffer(decoder, frame_buffer);
    }
    if (decoder != NULL)
    {
        PngDecoderDestory(decoder);
    }
    if (decoder_locked)
    {
        divoom_aw_pic_decoder_unlock();
    }
    if (success)
    {
        g_divoom_aw_png_retry_after_ms = 0;
    }
    else if (decoder_attempted)
    {
        g_divoom_aw_png_retry_after_ms = divoom_system_get_tickms() +
            DIVOOM_PNG_DECODE_RETRY_DELAY_MS;
    }
    return success ? out_buf : NULL;
}

uint8 *divoom_aw_decode_png_buf_to_lvfmt_reuse(const char *buf, int buf_len,
    int *width, int *height, divoom_aw_png_decode_buf_t *reuse,
    lv_color_format_t cf)
{
    return divoom_aw_decode_png_buf_to_lvfmt_reuse_internal(buf, buf_len,
        width, height, reuse, cf, NULL);
}

uint8 *divoom_aw_decode_png_buf_to_lvfmt(const char *buf, int buf_len,
    int *width, int *height, uint8 *out_buf, uint32 out_size,
    lv_color_format_t cf)
{
    divoom_aw_png_decode_buf_t reuse;

    memset(&reuse, 0, sizeof(reuse));
    reuse.out_buf = out_buf;
    reuse.out_buf_size = out_size;
    return divoom_aw_decode_png_buf_to_lvfmt_reuse(buf, buf_len, width, height,
        &reuse, cf);
}

uint8 *divoom_aw_decode_png_file_to_lvfmt_reuse(const char *filename,
    int *width, int *height, divoom_aw_png_decode_buf_t *reuse,
    lv_color_format_t cf)
{
    int file_len;
    uint8 *file_buf;
    uint8 *out_buf;

    if (filename == NULL || width == NULL || height == NULL || reuse == NULL)
    {
        return NULL;
    }

    file_buf = (uint8 *)divoom_app_com_read_file((char *)filename, &file_len);
    if (file_buf == NULL || file_len < 8)
    {
        if (file_buf != NULL)
        {
            divoom_system_free(file_buf);
        }
        divoom_print("png read fail: %s\n", filename);
        return NULL;
    }

    out_buf = divoom_aw_decode_png_buf_to_lvfmt_reuse_internal((const char *)file_buf,
        file_len, width, height, reuse, cf, &file_buf);
    if (file_buf != NULL)
    {
        divoom_system_free(file_buf);
    }
    return out_buf;
}
uint8 *divoom_aw_decode_png_file(const char *filename, int *width, int *height)
{
    int file_len;
    uint8 *file_buf;
    uint8 *out_buf;

    if (filename == NULL || width == NULL || height == NULL)
    {
        return NULL;
    }

    file_buf = (uint8 *)divoom_app_com_read_file((char *)filename, &file_len);
    if (file_buf == NULL || file_len < 8)
    {
        if (file_buf != NULL)
        {
            divoom_system_free(file_buf);
        }
        divoom_print("png read fail: %s\n", filename);
        return NULL;
    }

    out_buf = divoom_aw_decode_png_buf_internal((const char *)file_buf, file_len,
        width, height, &file_buf, NULL, NULL, NULL);
    if (file_buf != NULL)
    {
        divoom_system_free(file_buf);
    }
    return out_buf;
}

#endif /* !DIVOOM_SIMULATOR */

#ifdef DIVOOM_SIMULATOR
#include "lvgl.h"

extern unsigned lodepng_decode32(unsigned char **out, unsigned *w, unsigned *h,
    const unsigned char *in, size_t insize);
extern const char *lodepng_error_text(unsigned code);

void divoom_aw_png_decode_buf_release(divoom_aw_png_decode_buf_t *reuse)
{
    if (reuse == NULL)
    {
        return;
    }

    if (reuse->out_buf != NULL && reuse->out_buf_owner)
    {
        divoom_system_free(reuse->out_buf);
    }
    memset(reuse, 0, sizeof(*reuse));
}

static uint8 *divoom_aw_decode_png_buf_internal(const char *buf, int buf_len,
    int *width, int *height, uint8 **free_before_out_buf)
{
    unsigned char *img_data = NULL;
    uint8 *out_buf = NULL;
    unsigned w = 0;
    unsigned h = 0;
    unsigned err;
    size_t out_size;

    if (buf == NULL || buf_len <= 0 || width == NULL || height == NULL)
    {
        return NULL;
    }

    err = lodepng_decode32(&img_data, &w, &h, (const unsigned char *)buf, (size_t)buf_len);
    if (err != 0)
    {
        divoom_print("lodepng err %u: %s\n", err, lodepng_error_text(err));
        if (img_data != NULL)
        {
            lv_mem_free(img_data);
        }
        return NULL;
    }

    if (w == 0 || h == 0 || w > 8192 || h > 8192)
    {
        if (img_data != NULL)
        {
            lv_mem_free(img_data);
        }
        return NULL;
    }

    /*
     * lodepng_decode32 返回后不再访问压缩输入。文件入口先释放 file_buf，
     * 再申请系统堆输出缓冲，保持与真机相同的内存峰值控制。
     */
    if (free_before_out_buf != NULL && *free_before_out_buf != NULL)
    {
        divoom_system_free(*free_before_out_buf);
        *free_before_out_buf = NULL;
    }
    out_size = (size_t)w * (size_t)h * 4u;
    out_buf = (uint8 *)divoom_system_malloc((uint32)out_size);
    if (out_buf == NULL)
    {
        lv_mem_free(img_data);
        return NULL;
    }

    /* 仿真用 lodepng 软解，复制到系统堆，保持与真机 PNG 解码返回缓冲的释放契约一致。 */
    memcpy(out_buf, img_data, out_size);
    lv_mem_free(img_data);
    *width = (int)w;
    *height = (int)h;
    return out_buf;
}

uint8 *divoom_aw_decode_png_buf(const char *buf, int buf_len, int *width, int *height)
{
    return divoom_aw_decode_png_buf_internal(buf, buf_len, width, height, NULL);
}

static uint8 *divoom_aw_decode_png_buf_to_lvfmt_reuse_internal(const char *buf,
    int buf_len,
    int *width, int *height, divoom_aw_png_decode_buf_t *reuse,
    lv_color_format_t cf, uint8 **free_before_out_buf)
{
    unsigned char *img_data = NULL;
    uint8 *out_buf = NULL;
    uint32 out_size;
    unsigned w = 0;
    unsigned h = 0;
    unsigned err;
    uint32 px_count;

    if (buf == NULL || buf_len <= 0 || width == NULL || height == NULL ||
        reuse == NULL)
    {
        return NULL;
    }
    err = lodepng_decode32(&img_data, &w, &h, (const unsigned char *)buf, (size_t)buf_len);
    if (err != 0)
    {
        divoom_print("lodepng err %u: %s\n", err, lodepng_error_text(err));
        if (img_data != NULL)
        {
            lv_mem_free(img_data);
        }
        return NULL;
    }

    if (w == 0 || h == 0 || w > 8192 || h > 8192)
    {
        if (img_data != NULL)
        {
            lv_mem_free(img_data);
        }
        return NULL;
    }

    /*
     * lodepng 已把压缩数据完整展开到 img_data，先释放文件缓冲，
     * 再申请或扩展最终 RGB565A8 缓冲。
     */
    if (free_before_out_buf != NULL && *free_before_out_buf != NULL)
    {
        divoom_system_free(*free_before_out_buf);
        *free_before_out_buf = NULL;
    }
    out_size = divoom_pixel_format_size((uint32)w, (uint32)h, cf);
    if (!divoom_aw_png_prepare_out_buf(reuse, out_size, &out_buf) ||
        !divoom_aw_check_png_lvfmt_out((int)w, (int)h, out_buf, out_size, cf,
            &px_count))
    {
        if (img_data != NULL)
        {
            lv_mem_free(img_data);
        }
        return NULL;
    }

    /* lodepng 缓冲由 LVGL 分配，只作为临时源；最终 cache buffer 归调用方持有。 */
    if (cf == LV_COLOR_FORMAT_RGB565A8)
    {
        divoom_rgba8888_to_rgb565a8_buf(img_data, out_buf, px_count);
    }
    else
    {
        uint32 i;

        for (i = 0; i < px_count; i++)
        {
            uint16 c = divoom_rgb888_to_rgb565(img_data[i * 4U],
                img_data[i * 4U + 1U], img_data[i * 4U + 2U]);

            divoom_store_rgb565_le(out_buf + i * DIVOOM_APP_RGB565_BYTES_PER_PX,
                c);
        }
    }
    lv_mem_free(img_data);
    *width = (int)w;
    *height = (int)h;
    return out_buf;
}

uint8 *divoom_aw_decode_png_buf_to_lvfmt_reuse(const char *buf, int buf_len,
    int *width, int *height, divoom_aw_png_decode_buf_t *reuse,
    lv_color_format_t cf)
{
    return divoom_aw_decode_png_buf_to_lvfmt_reuse_internal(buf, buf_len,
        width, height, reuse, cf, NULL);
}

uint8 *divoom_aw_decode_png_buf_to_lvfmt(const char *buf, int buf_len,
    int *width, int *height, uint8 *out_buf, uint32 out_size,
    lv_color_format_t cf)
{
    divoom_aw_png_decode_buf_t reuse;

    memset(&reuse, 0, sizeof(reuse));
    reuse.out_buf = out_buf;
    reuse.out_buf_size = out_size;
    return divoom_aw_decode_png_buf_to_lvfmt_reuse(buf, buf_len, width, height,
        &reuse, cf);
}

uint8 *divoom_aw_decode_png_file_to_lvfmt_reuse(const char *filename,
    int *width, int *height, divoom_aw_png_decode_buf_t *reuse,
    lv_color_format_t cf)
{
    int file_len;
    uint8 *file_buf;
    uint8 *out_buf;

    if (filename == NULL || width == NULL || height == NULL || reuse == NULL)
    {
        return NULL;
    }

    file_buf = (uint8 *)divoom_app_com_read_file((char *)filename, &file_len);
    if (file_buf == NULL || file_len < 8)
    {
        if (file_buf != NULL)
        {
            divoom_system_free(file_buf);
        }
        divoom_print("png read fail: %s\n", filename);
        return NULL;
    }

    out_buf = divoom_aw_decode_png_buf_to_lvfmt_reuse_internal(
        (const char *)file_buf, file_len, width, height, reuse, cf, &file_buf);
    if (file_buf != NULL)
    {
        divoom_system_free(file_buf);
    }
    return out_buf;
}
uint8 *divoom_aw_decode_png_file(const char *filename, int *width, int *height)
{
    int file_len;
    uint8 *file_buf;
    uint8 *out_buf;

    if (filename == NULL || width == NULL || height == NULL)
    {
        return NULL;
    }

    file_buf = (uint8 *)divoom_app_com_read_file((char *)filename, &file_len);
    if (file_buf == NULL || file_len < 8)
    {
        if (file_buf != NULL)
        {
            divoom_system_free(file_buf);
        }
        divoom_print("png read fail: %s\n", filename);
        return NULL;
    }

    out_buf = divoom_aw_decode_png_buf_internal((const char *)file_buf, file_len,
        width, height, &file_buf);
    if (file_buf != NULL)
    {
        divoom_system_free(file_buf);
    }
    return out_buf;
}
#endif /* DIVOOM_SIMULATOR */
