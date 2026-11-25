/******************************************************************************
  Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 ******************************************************************************
  File Name     : PIXEL_FORMAT_E_g2d_format_convert.h
  Version       : Initial Draft
  Author        : Allwinner BU3-PD2 Team
  Created       : 2017/04/10
  Last Modified :
  Description   : PIXEL_FORMAT_E g2d_data_fmt,g2d_pixel_seq conversion.
  Function List :
  History       :
******************************************************************************/
#ifndef _PIXEL_FORMAT_E_G2D_FORMAT_CONVERT_H_
#define _PIXEL_FORMAT_E_G2D_FORMAT_CONVERT_H_

#include <stdbool.h>
#include <stdint.h>

#include <mm_comm_video.h>

#if defined(CONFIG_KERNEL_FREERTOS) || defined(CONFIG_OS_MELIS)
#include <g2d_driver.h>
#else
#include <sunxi-g2d.h>
#include <linux/videodev2.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

ERRORTYPE convert_PIXEL_FORMAT_E_to_G2dFormat(
    PARAM_IN PIXEL_FORMAT_E ePixelFormat,
    PARAM_OUT g2d_data_fmt *pG2dFormat,
    PARAM_OUT g2d_pixel_seq *pG2dPixelSeq);

ERRORTYPE convert_PIXEL_FORMAT_E_to_g2d_fmt_enh(
    PARAM_IN PIXEL_FORMAT_E ePixelFormat,
    PARAM_OUT g2d_fmt_enh *pG2dFormat);

#if defined(CONFIG_KERNEL_FREERTOS) || defined(CONFIG_OS_MELIS)
ERRORTYPE convert_g2d_color_gmt_to_int(PARAM_IN g2d_color_gmt eSrc, PARAM_OUT int *pDst);
#else
ERRORTYPE convert_v4l2_colorspace_to_g2d_color_gmt(PARAM_IN enum v4l2_colorspace eSrc, PARAM_OUT g2d_color_gmt *pDst);
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* _PIXEL_FORMAT_E_G2D_FORMAT_CONVERT_H_ */

