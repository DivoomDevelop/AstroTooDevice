/*
 * Copyright (c) 2008-2016 Allwinner Technology Co. Ltd.
 * All rights reserved.
 *
 * File : pngdecode.h
 * Description : pngdecode
 * History :
 *
 */


#ifndef PNG_DECODE
#define PNG_DECODE

#ifdef __cplusplus
extern "C" {
#endif

#include "vdecoder.h"           //* video decode library in "libcedarc/include/"
#include "memoryAdapter.h"
#include "png_memapaptor.h"
#define USE_ZERO_COPY   1

typedef struct ImgFrame {
	uint32_t mWidth;                    //the width which contains aligned buffer
	uint32_t mHeight;                  //the height which contains aligned buffer
	uint32_t mRGBASize;            // Number of bytes in mRGB565Data
	uint8_t *mRGBAData;            // Actual RGB565 data
} ImgFrame;

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

typedef struct PngDecoderContext {
	VideoDecoder *mVideoDecoder;
	ImgFrame mImgFrame;
    VideoStreamInfo             vStreamInfo;
    VConfig                     vConfig;

	char *mUrl;
	char *mSrcBuf;
	int mSrcBufLen;
	struct ScMemOpsS *memops;

	int                         mScaleDownEn;
    int                         mHorizonScaleDownRatio;
    int                         mVerticalScaleDownRatio;
    int                         mDecodedPixFormat;

	uint32_t                    nPhyOffset;
    VideoPicture*               mVideoPicture;
    uint8_t                     mUseZeroCopyBuffer;
} PngDecoderContext;

typedef void *PngDecoder;

PngDecoder *PngDecoderCreate(void);
void PngDecoderDestory(PngDecoder *v);
void PngDecoderSetDataSource(PngDecoder *v, const char *pUrl);
void PngDecoderSetDataSourceBuf(PngDecoder *v, char *buffer, int bufLen);
ImgFrame *PngDecoderGetFrame(PngDecoder *v, int width, int height);

int PngDecoderInitial(PngDecoder *v, uint32_t width, uint32_t height);
void PngDecoderDeinit(PngDecoder *v);
int PngDecoderSubmit(PngDecoder* v);
uint8_t *PngAllocateFrameBuffer(PngDecoder* v, uint8_t* pBuffer,uint32_t size);
void PngFreeFrameBuffer(PngDecoder* v, uint8_t* pFbBuffer);
void PngFreeFrameBufferEx(PngDecoder* v, uint8_t* pFbBuffer);
int PngSetFrameBufferToVdec(PngDecoder* v, uint8_t *pFbBuffer);
int PngReleaseFrameBufferToVdec(PngDecoder* v);
int PngDecoderFrameBuffer(PngDecoder* v, VideoPicture **pVideoPicture);
int PngDecoderReturnFrameBuffer(PngDecoder* v, VideoPicture *videoPicture);
int PngDecoderReopen(PngDecoder* v, uint32_t width, uint32_t height, int *reopen);

#ifdef __cplusplus
}
#endif

#endif
