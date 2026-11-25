/*
 * F101 FreeRTOS：Allwinner jpegdecode / pngdecode 硬件解码封装。
 * 参考《FreeRTOS_RTOS_多媒体_开发指南》JpegDecoder* / PngDecoder* 接口。
 */

#ifdef DIVOOM_SIMULATOR

#include "middle/include/divoom_aw_pic_decode.h"

uint16 divoom_aw_rgb_to_rgb565(uint8 r, uint8 g, uint8 b)
{
    return (uint16)(((uint16)(r & 0xF8U) << 8) |
        ((uint16)(g & 0xFCU) << 3) |
        ((uint16)b >> 3));
}

#else

#include "divoom.h"
#include "middle/include/divoom_aw_pic_decode.h"
#include "lvgl/include/divoom_lvgl_init.h"
#include <jpegdecode.h>
#if defined(CONFIG_ARCH_RISCV_VECTOR) && defined(__riscv_vector)
#include "divoom_nv12_rgb565_rvv.h"
#endif

uint16 divoom_aw_rgb_to_rgb565(uint8 r, uint8 g, uint8 b)
{
    return (uint16)(((uint16)(r & 0xF8) << 8) |
        ((uint16)(g & 0xFC) << 3) |
        ((uint16)b >> 3));
}

static void divoom_aw_nv12_to_rgb565_scalar(const uint8 *y_plane, const uint8 *uv_plane,
    uint8 *rgb565_buf, int width, int height, int y_stride, int uv_stride,
    size_t y_plane_size, size_t uv_plane_size)
{
    int i;
    int j;
    int uv_rows;
    int uv_pairs;
    uint16 *rgb565;

    if (y_plane == NULL || uv_plane == NULL || rgb565_buf == NULL ||
        width <= 0 || height <= 0 || y_stride < width || uv_stride < width ||
        (size_t)height > y_plane_size / (size_t)y_stride)
    {
        return;
    }

    /*
     * jpegdecode.c packs NV12 with display_width bytes per UV row and copies
     * display_height / 2 rows.  Odd image dimensions therefore do not contain
     * a new complete UV pair for the last column/row.  Reuse the nearest valid
     * pair instead of reading the byte just beyond the allocated YUV buffer.
     */
    uv_rows = height / 2;
    if ((size_t)uv_rows > uv_plane_size / (size_t)uv_stride)
    {
        uv_rows = (int)(uv_plane_size / (size_t)uv_stride);
    }
    uv_pairs = width / 2;
    rgb565 = (uint16 *)rgb565_buf;

    for (i = 0; i < height; i++)
    {
        for (j = 0; j < width; j++)
        {
            size_t y_index;
            size_t uv_index;
            int y_value;
            int u_value;
            int v_value;
            int c;
            int d;
            int e;
            int r;
            int g;
            int b;

            y_index = (size_t)i * (size_t)y_stride + (size_t)j;
            y_value = y_plane[y_index];
            if (uv_rows > 0 && uv_pairs > 0)
            {
                int uv_row = i / 2;
                int uv_pair = j / 2;
                if (uv_row >= uv_rows)
                {
                    uv_row = uv_rows - 1;
                }
                if (uv_pair >= uv_pairs)
                {
                    uv_pair = uv_pairs - 1;
                }
                uv_index = (size_t)uv_row * (size_t)uv_stride +
                    (size_t)uv_pair * 2u;
                u_value = uv_plane[uv_index];
                v_value = uv_plane[uv_index + 1u];
            }
            else
            {
                u_value = 128;
                v_value = 128;
            }

            /* JPEG emits full-range YCbCr; applying video-range CSC crushes shadows. */
            c = y_value;
            d = u_value - 128;
            e = v_value - 128;
            r = (256 * c + 359 * e + 128) >> 8;
            g = (256 * c - 88 * d - 183 * e + 128) >> 8;
            b = (256 * c + 454 * d + 128) >> 8;

            if (r < 0)
            {
                r = 0;
            }
            else if (r > 255)
            {
                r = 255;
            }
            if (g < 0)
            {
                g = 0;
            }
            else if (g > 255)
            {
                g = 255;
            }
            if (b < 0)
            {
                b = 0;
            }
            else if (b > 255)
            {
                b = 255;
            }

            rgb565[i * width + j] = divoom_aw_rgb_to_rgb565((uint8)r, (uint8)g, (uint8)b);
        }
    }
}

static void divoom_aw_nv12_to_rgb565_stride(const uint8 *y, const uint8 *uv,
    uint8 *out, int width, int height, int y_stride, int uv_stride,
    size_t y_size, size_t uv_size)
{
#if defined(CONFIG_ARCH_RISCV_VECTOR) && defined(__riscv_vector)
    if(y && uv && out && width > 1 && height > 1 &&
       y_stride >= width && uv_stride >= width && (size_t)height <= y_size / (size_t)y_stride) {
        size_t rows = (size_t)height / 2;
        if(rows > uv_size / (size_t)uv_stride) rows = uv_size / (size_t)uv_stride;
        if(rows) {
            divoom_nv12_rgb565_rvv(y, uv, (uint16_t *)out, width, height, y_stride, uv_stride, (int)rows);
            return;
        }
    }
#endif
    divoom_aw_nv12_to_rgb565_scalar(y, uv, out, width, height, y_stride, uv_stride, y_size, uv_size);
}


uint8 *divoom_aw_decode_jpeg_rgb565_buf(char *buf, int buf_len, int *image_w, int *image_h)
{
    JpegDecoder *decoder = NULL;
    ImgFrame *frame = NULL;
    uint8 *rgb565_buf = NULL;
    int width = 0;
    int height = 0;
    int decode_ok = 0;
    int decoder_locked = 0;
    size_t y_size;
    size_t out_size;

    if (buf == NULL || buf_len <= 0 || image_w == NULL || image_h == NULL)
    {
        return NULL;
    }

    if (divoom_aw_pic_decoder_lock() != 0)
    {
        return NULL;
    }
    decoder_locked = 1;

    decoder = JpegDecoderCreate();
    if (decoder == NULL)
    {
        divoom_print("JpegDecoderCreate fail");
        goto cleanup;
    }

    JpegDecoderSetDataSourceBufNoFree(decoder, buf, buf_len,
        JPEG_DECODE_SCALE_DOWN_1, JpegDecodeOutputDataNV12);
    frame = JpegDecoderGetFrame(decoder);
    if (frame == NULL)
    {
        divoom_print("JpegDecoderGetFrame fail len=%d", buf_len);
        goto cleanup;
    }
    width = frame->mDisplayWidth != 0U ? (int)frame->mDisplayWidth : (int)frame->mWidth;
    height = frame->mDisplayHeight != 0U ? (int)frame->mDisplayHeight : (int)frame->mHeight;
    y_size = (size_t)width * (size_t)height;
    if (width <= 0 || height <= 0 || frame->mYuvData == NULL ||
        frame->mYuvSize < y_size + y_size / 2u)
    {
        divoom_print("invalid jpeg frame w=%d h=%d size=%u",
                     width, height, frame->mYuvSize);
        goto cleanup;
    }

    out_size = (size_t)width * (size_t)height * 2u;
    rgb565_buf = (uint8 *)divoom_system_malloc(out_size);
    if (rgb565_buf == NULL)
    {
        divoom_print("jpeg rgb565 alloc fail size=%u w=%d h=%d", (uint32)out_size, width, height);
        goto cleanup;
    }

    divoom_aw_nv12_to_rgb565_stride(
        frame->mYuvData, frame->mYuvData + y_size,
        rgb565_buf, width, height, width, width,
        y_size, (size_t)frame->mYuvSize - y_size);
    decode_ok = 1;

cleanup:
    if (decoder != NULL)
    {
        JpegDecoderDestory(decoder);
        decoder = NULL;
        frame = NULL;
    }
    if (decoder_locked)
    {
        divoom_aw_pic_decoder_unlock();
    }
    if (!decode_ok && rgb565_buf != NULL)
    {
        divoom_system_free(rgb565_buf);
        rgb565_buf = NULL;
    }
    if (decode_ok)
    {
        *image_w = width;
        *image_h = height;
    }
    return rgb565_buf;
}

uint8 *divoom_aw_decode_jpeg_rgb565_to_buf(char *buf, int buf_len, int *image_w, int *image_h,
    uint8 *out_buf, uint32 out_size)
{
    JpegDecoder *decoder = NULL;
    ImgFrame *frame = NULL;
    int width = 0;
    int height = 0;
    int decode_ok = 0;
    int decoder_locked = 0;
    size_t y_size;
    size_t need_size;

    if (buf == NULL || buf_len <= 0 || image_w == NULL || image_h == NULL ||
        out_buf == NULL || out_size == 0)
    {
        return NULL;
    }

    if (divoom_aw_pic_decoder_lock() != 0)
    {
        return NULL;
    }
    decoder_locked = 1;

    decoder = JpegDecoderCreate();
    if (decoder == NULL)
    {
        divoom_print("JpegDecoderCreate fail");
        goto cleanup;
    }

    JpegDecoderSetDataSourceBufNoFree(decoder, buf, buf_len,
        JPEG_DECODE_SCALE_DOWN_1, JpegDecodeOutputDataNV12);
    frame = JpegDecoderGetFrame(decoder);
    if (frame == NULL)
    {
        divoom_print("JpegDecoderGetFrame fail len=%d", buf_len);
        goto cleanup;
    }
    width = frame->mDisplayWidth != 0U ? (int)frame->mDisplayWidth : (int)frame->mWidth;
    height = frame->mDisplayHeight != 0U ? (int)frame->mDisplayHeight : (int)frame->mHeight;
    y_size = (size_t)width * (size_t)height;
    if (width <= 0 || height <= 0 || frame->mYuvData == NULL ||
        frame->mYuvSize < y_size + y_size / 2u)
    {
        divoom_print("invalid jpeg frame w=%d h=%d size=%u",
                     width, height, frame->mYuvSize);
        goto cleanup;
    }

    need_size = (size_t)width * (size_t)height * 2u;
    if (need_size == 0 || need_size > (size_t)out_size)
    {
        divoom_print("jpeg external rgb565 buf too small need=%u cap=%u w=%d h=%d",
            (uint32)need_size, out_size, width, height);
        goto cleanup;
    }

    /*
     * Native Lua 页底图复用启动时预留的大块 RGB565 缓冲区，避免游戏运行中
     * 反复 malloc/free 480x480 输出图造成堆碎片。
     */
    divoom_aw_nv12_to_rgb565_stride(
        frame->mYuvData, frame->mYuvData + y_size,
        out_buf, width, height, width, width,
        y_size, (size_t)frame->mYuvSize - y_size);
    decode_ok = 1;

cleanup:
    if (decoder != NULL)
    {
        JpegDecoderDestory(decoder);
    }
    if (decoder_locked)
    {
        divoom_aw_pic_decoder_unlock();
    }
    if (decode_ok)
    {
        *image_w = width;
        *image_h = height;
        return out_buf;
    }
    return NULL;
}

#endif /* !DIVOOM_SIMULATOR */

uint8 divoom_aw_parse_jpeg_sof_info(const char *buf, int buf_len, int *width, int *height)
{
    const unsigned char *p;
    int i;

    if(buf == NULL || buf_len < 4 || width == NULL || height == NULL)
    {
        return 0;
    }
    p = (const unsigned char *)buf;
    if(p[0] != 0xFF || p[1] != 0xD8)
    {
        return 0;
    }
    i = 2;
    while(i + 3 < buf_len)
    {
        int marker;
        int seglen;
        int w;
        int h;

        if(p[i] != 0xFF)
        {
            return 0;
        }
        while(i < buf_len && p[i] == 0xFF)
        {
            i++;
        }
        if(i >= buf_len)
        {
            return 0;
        }
        marker = p[i++];
        if(marker == 0xD8 || marker == 0xD9 || (marker >= 0xD0 && marker <= 0xD7))
        {
            continue;
        }
        if(marker == 0xDA)
        {
            return 0;
        }
        if(i + 1 >= buf_len)
        {
            return 0;
        }
        seglen = ((int)p[i] << 8) | (int)p[i + 1];
        if(seglen < 2 || i + seglen > buf_len)
        {
            return 0;
        }
        if(marker == 0xC0 || marker == 0xC1 || marker == 0xC2 || marker == 0xC3 ||
            marker == 0xC9 || marker == 0xCA || marker == 0xCB)
        {
            if(seglen < 7)
            {
                return 0;
            }
            h = ((int)p[i + 3] << 8) | (int)p[i + 4];
            w = ((int)p[i + 5] << 8) | (int)p[i + 6];
            if(w <= 0 || h <= 0 || w > 8192 || h > 8192)
            {
                return 0;
            }
            *width = w;
            *height = h;
            return 1;
        }
        i += seglen;
    }
    return 0;
}
