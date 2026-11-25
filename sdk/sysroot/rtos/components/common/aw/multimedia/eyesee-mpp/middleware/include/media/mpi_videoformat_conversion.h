/******************************************************************************
  Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 ******************************************************************************
  File Name     : mpi_videoformat.h
  Version       : Initial Draft
  Author        : Allwinner BU3-PD2 Team
  Created       : 2016/12/14
  Last Modified :
  Description   : mpi functions declaration for video format convert.
  Function List :
  History       :
******************************************************************************/
#ifndef __IPCLINUX_MPI_VIDEOFORMAT_H__
#define __IPCLINUX_MPI_VIDEOFORMAT_H__

#include <mm_comm_video.h>
#include <ConfigOption.h>
#if (MPPCFG_VDEC == OPTION_VDEC_ENABLE)
#include <vdecoder.h>
#endif
#if (MPPCFG_VENC == OPTION_VENC_ENABLE)
#include <vencoder.h>
#endif
// #include <linux/videodev2.h>

#ifdef __cplusplus
extern "C" {
#endif

// PIXEL_FORMAT_E map_V4L2_PIX_FMT_to_PIXEL_FORMAT_E(int v4l2PixFmt);
// int map_PIXEL_FORMAT_E_to_V4L2_PIX_FMT(PIXEL_FORMAT_E format);
// VIDEO_FIELD_E map_V4L2_FIELD_to_VIDEO_FIELD_E(enum v4l2_field v4l2Field);
// enum v4l2_field map_VIDEO_FIELD_E_to_V4L2_FIELD(VIDEO_FIELD_E eVideoField);
#if (MPPCFG_VDEC == OPTION_VDEC_ENABLE)
PIXEL_FORMAT_E map_EPIXELFORMAT_to_PIXEL_FORMAT_E(enum EPIXELFORMAT vdecPixelFormat);
enum EPIXELFORMAT map_PIXEL_FORMAT_E_to_EPIXELFORMAT(PIXEL_FORMAT_E pixelFormat);
#endif
#if (MPPCFG_VENC == OPTION_VENC_ENABLE)
VENC_PIXEL_FMT map_PIXEL_FORMAT_E_to_VENC_PIXEL_FMT(PIXEL_FORMAT_E pixelFormat);
#endif

#ifdef __cplusplus
}
#endif

#endif /* __IPCLINUX_MPI_VIDEOFORMAT_H__ */

