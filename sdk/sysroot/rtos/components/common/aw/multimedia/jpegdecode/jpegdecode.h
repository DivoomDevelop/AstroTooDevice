/*
 * Copyright (c) 2008-2016 Allwinner Technology Co. Ltd.
 * All rights reserved.
 *
 * File : jpegdecode.h
 * Description : jpegdecode
 * History :
 *
 */


#ifndef JPEG_DECODE
#define JPEG_DECODE

#ifdef __cplusplus
extern "C" {
#endif

#define USE_ZERO_COPY   0

#include "vdecoder.h"           //* video decode library in "libcedarc/include/"
#include "memoryAdapter.h"
#include "jpg_memadaptor.h"
#define ALIGN_16B(x) (((x) + (15)) & ~(15))
#define ALIGN_32B(x) (((x) + (31)) & ~(31))

typedef enum JpegDecodeScaleDownRatio{
    JPEG_DECODE_SCALE_DOWN_1      = 0, /*no scale down*/
    JPEG_DECODE_SCALE_DOWN_2      = 1, /*scale down 1/2*/
    JPEG_DECODE_SCALE_DOWN_4      = 2, /*scale down 1/4*/
    JPEG_DECODE_SCALE_DOWN_8      = 3, /*scale down 1/8*/
}JpegDecodeScaleDownRatio;
typedef enum JpegDecodeOutputDataType
{
    JpegDecodeOutputDataNV21 = 1,
    JpegDecodeOutputDataNV12 = 2,
    JpegDecodeOutputDataYU12 = 3,
    JpegDecodeOutputDataYV12 = 4,
    JpegDecodeOutputDataRGB565 = 5,
    JpegDecodeOutputDataMB32_420 = 6,
}JpegDecodeOutputDataType;

typedef struct ImgFrame
{
    uint32_t mWidth;                    //the width which contains aligned buffer
    uint32_t mHeight;                  //the height which contains aligned buffer
    uint32_t mDisplayWidth;       //the actural frame width
    uint32_t mDisplayHeight;     //the actural frame height
    uint32_t mYuvSize;              // Number of bytes in mYuvData
    uint8_t* mYuvData;            // Actual yuv data
    int32_t  mRotationAngle;   // rotation angle, clockwise
}ImgFrame;

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
    int            bOutputMB32Format;
}ImgFrameBuffer;

typedef struct JpegDecoderContext
{
    VideoDecoder*               mVideoDecoder;
    ImgFrame                    mImgFrame;
    VideoStreamInfo             vStreamInfo;
    VConfig                     vConfig;
    char*                       mUrl;
    char*                       mSrcBuf;
    int                         mSrcBufLen;
    int                         mSrcBufNeedFree;
    int                         mScaleDownEn;
    int                         mHorizonScaleDownRatio;
    int                         mVerticalScaleDownRatio;
    int                         mDecodedPixFormat;
    int                         mOutputDataType;
    uint32_t                    mDecodedYuvCapacity;
    uint32_t                    mDecodedRgb565Capacity;
    int                         mIonFd;
    uint32_t                    nPhyOffset;
    VideoPicture*               mVideoPicture;
    uint8_t                     mUseZeroCopyBuffer;
    struct                      ScMemOpsS *memops;
}JpegDecoderContext;

typedef void* JpegDecoder;

char * readJpegData(char *path, int *pLen);

JpegDecoder* JpegDecoderCreate();
void JpegDecoderDestory(JpegDecoder* v);
void JpegDecoderSetDataSource(JpegDecoder* v, const char* pUrl,JpegDecodeScaleDownRatio scaleRatio,JpegDecodeOutputDataType outputType);
void JpegDecoderSetDataSourceBuf(JpegDecoder* v, char* buffer,int bufLen,JpegDecodeScaleDownRatio scaleRatio,JpegDecodeOutputDataType outputType);
void JpegDecoderSetDataSourceBufNoFree(JpegDecoder* v, char* buffer,int bufLen,JpegDecodeScaleDownRatio scaleRatio,JpegDecodeOutputDataType outputType);
void JpegDecoderReleaseVideoDecoder(JpegDecoder* v);
ImgFrame *JpegDecoderGetFrame(JpegDecoder* v);

int JpegDecoderInitial(JpegDecoder* v, uint32_t width, uint32_t height);
int JpegDecoderReopen(JpegDecoder* v, uint32_t width, uint32_t height, int *reopen);
uint8_t *JpegAllocateFrameBuffer(JpegDecoder* v, uint32_t size);
void JpegFreeFrameBuffer(JpegDecoder* v, uint8_t* pFbBuffer);
int JpegSetFrameBufferToVdec(JpegDecoder* v, uint8_t *pFbBuffer);
int JpegDecoderSubmit(JpegDecoder* v);
int JpegDecoderFrameBuffer(JpegDecoder* v, VideoPicture **pVideoPicture);
int JpegDecoderReturnFrameBuffer(JpegDecoder* v, VideoPicture *videoPicture);
#ifdef __cplusplus
}
#endif

#endif
