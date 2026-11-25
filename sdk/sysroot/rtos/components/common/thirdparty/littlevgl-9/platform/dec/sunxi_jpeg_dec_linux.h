#ifndef SUNXI_JPEG_DEC_LINUX_H
#define SUNXI_JPEG_DEC_LINUX_H

#ifdef __cplusplus
extern "C" {
#endif



#if defined(LV_USE_SUNXI_JPEG) && defined(OS_LINUX)
#include "lvgl.h"
#include "lvgl_private.h"
#include "lv_sunxi_jpeg.h"
#include "vdecoder.h"           //* video decode library in "libcedarc/include/"
#include "memoryAdapter.h"

typedef enum JpegDecodeScaleDownRatio{
    JPEG_DECODE_SCALE_DOWN_1      = 0, /*no scale down*/
    JPEG_DECODE_SCALE_DOWN_2      = 1, /*scale down 1/2*/
    JPEG_DECODE_SCALE_DOWN_4      = 2, /*scale down 1/4*/
    JPEG_DECODE_SCALE_DOWN_8      = 3, /*scale down 1/8*/
}JpegDecodeScaleDownRatio;

typedef struct ImgFrameBuffer
{
    unsigned char* pAddrPhyY;
    unsigned char* pAddrPhyC;
    unsigned char* pAddrVirY;
    unsigned char* pAddrVirC;
    int            nLeft;
    int            nTop;
    int            nWidth;
    int            nHeight;

    int            nShareBufFd;
    int            nBufferSize;
#ifdef CONFIG_SOC_SUN252IW2
    int            bOutputMB32Format;
#endif
}ImgFrameBuffer;

typedef struct JpegDecoderContext
{
    VideoDecoder*      mVideoDecoder;
    char*              mSrcBuf;
    int                mSrcBufLen;
    int                mScaleDownEn;
    int                mHorizonScaleDownRatio;
    int                mVerticalScaleDownRatio;
    int                mDecodedPixFormat;
    struct ScMemOpsS  *memops;
    VideoPicture*      mVideoPicture;
}JpegDecoderContext;

typedef void* JpegDecoder;


int32_t jpeg_decoder_init(void);
lv_draw_buf_t *jpeg_decoder_dec(lv_image_decoder_dsc_t *dsc, uint8_t *data, uint32_t size);
void jpeg_decoder_finish(lv_image_decoder_dsc_t *dsc);
void jpeg_decoder_deinit(void);



#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
